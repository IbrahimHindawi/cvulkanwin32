#ifndef UNICODE
#   define UNICODE
#endif

#include <windows.h>
#include "core.h"

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan_core.h>

#define assert(expr) if (!(expr)) { __debugbreak(); }

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LPSTR GetLastErrorAsString()
{
    DWORD errorMessageID = GetLastError();
    if(errorMessageID == 0) {
        return NULL;
    }
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    return messageBuffer;
}

VkBool32 VKAPI_PTR DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
    return true;
}

void SetupVulkanInstance(HWND window_handle, VkInstance *out_instance, VkSurfaceKHR *out_surface) {
    u32 count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, NULL);
    assert(result == VK_SUCCESS);
    assert(count > 0);

    VkLayerProperties *instance_layers = malloc(sizeof(*instance_layers) * count);
    result = vkEnumerateInstanceLayerProperties(&count, instance_layers);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    assert(result == VK_SUCCESS);
    assert(count > 0);

    VkExtensionProperties *instance_extension = malloc(sizeof(*instance_extension) * count);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, instance_extension);
    const char **extension_names = malloc(sizeof(*extension_names) * count);
    const char *layers[] = { "VK_LAYER_NV_optimus" };
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
    const char *extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report" };
#else
    const char *extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#endif
    {
        VkApplicationInfo app_info = {0};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello Vulkan";
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instance_create_info = {0};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.flags = 0;
        instance_create_info.pNext = 0;
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.enabledLayerCount= 1;
        instance_create_info.ppEnabledLayerNames = layers;
        instance_create_info.enabledExtensionCount = 2;
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
        instance_create_info.enabledExtensionCount = 3;
#endif
        instance_create_info.ppEnabledExtensionNames = extensions;

        VkResult result = vkCreateInstance(&instance_create_info, NULL, out_instance);
        assert(result == VK_SUCCESS); // failed to create VK instance!
        assert(*out_instance != NULL);
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
#endif
    }
    HINSTANCE hInstance = GetModuleHandle(NULL);
    VkWin32SurfaceCreateInfoKHR surface_create_info = {0};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hinstance = hInstance;
    surface_create_info.hwnd = window_handle;
    assert(*out_surface == NULL);

    result = vkCreateWin32SurfaceKHR(*out_instance, &surface_create_info, NULL, out_surface);
    assert(result == VK_SUCCESS); // could not crate surface
    assert(out_surface != NULL); // could not crate surface

// #define ENABLE_VULKAN_DEBUG_CALLBACK
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
{
    VkDebugReportCallbackEXT error_callback = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT warning_callback = VK_NULL_HANDLE;
    PFN_vkCreateDebugReportCallbackEXT vk_create_debug_report_callback_ext = VK_NULL_HANDLE;
    *(void **)&vk_create_debug_report_callback_ext = vkGetInstanceProcAddr(*out_instance, "vkCreateDebugReportCallbackEXT");
    assert(vk_create_debug_report_callback_ext);

    VkDebugReportCallbackCreateInfoEXT callback_create_info = {0};
    callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
    callback_create_info.pfnCallback = &DebugReportCallback;

    VkResult result = vkCreateDebugReportCallbackEXT(*out_instance, &callback_create_info, NULL, &error_callback);
    assert(result);
    callback_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callback_create_info.pfnCallback = &DebugReportCallback;

    result = vkCreateDebugReportCallbackEXT(*out_instance, &callback_create_info, NULL, &warning_callback);
    assert(result);

}
#endif

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *pCmdLine, int nCmdShow) {

    WNDCLASS window_class = {0};
    const wchar_t class_name[] = L"Sample Window Class";
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = hInstance;
    window_class.lpszClassName = class_name;
    window_class.style = CS_HREDRAW | CS_VREDRAW;

    WORD r = RegisterClass(&window_class);
    if(r == 0) {
        DWORD error = GetLastError();
        LPSTR errormessage = GetLastErrorAsString();
        printf("%lu:%s\n", error, errormessage);
        return 0;
    }

    
    HWND window_handle = CreateWindowEx(0, class_name, L"Win64 Vulkan", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    if(window_handle == NULL) {
        DWORD error = GetLastError();
        LPSTR errormessage = GetLastErrorAsString();
        printf("%lu:%s\n", error, errormessage);
        return 0;
    }
    ShowWindow(window_handle, nCmdShow);

    VkInstance out_instance = NULL;
    VkSurfaceKHR out_surface = NULL;
    SetupVulkanInstance(window_handle, &out_instance, &out_surface);

    MSG message = {0};
    while (GetMessage(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
