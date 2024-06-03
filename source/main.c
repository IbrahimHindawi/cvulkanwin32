#ifndef UNICODE
#   define UNICODE
#endif
#define NO_STDIO_REDIRECT

#include <core.h>

#include <windows.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan_core.h>

#include "meta/gen/hkArray_core.h"
#include "meta/gen/hkArray_VkLayerProperties.h"
#include "meta/gen/hkArray_VkExtensionProperties.h"
#include "meta/gen/hkArray_VkPhysicalDevice.h"
#include "meta/gen/hkArray_VkImage.h"
#include "meta/gen/hkArray_VkImageView.h"
#include "meta/gen/hkArray_VkQueueFamilyProperties.h"

#define assert(expr) if (!(expr)) { __debugbreak(); }

VkInstance g_instance;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_logical_device;
VkQueue g_graphics_queue;

str validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

static int __stdcall OutputDebugF(const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);

	int NumCharsWritten = OutputDebugF(Format, Args);

	va_end(Args);

	return NumCharsWritten;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

LPSTR getLastErrorAsString()
{
    DWORD errorMessageID = GetLastError();
    if(errorMessageID == 0) {
        return NULL;
    }
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    return messageBuffer;
}

VkBool32 VKAPI_PTR debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
    return true;
}

void setupVulkanInstance(HWND window_handle, VkInstance *out_instance, VkSurfaceKHR *out_surface) {
    u32 count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, NULL);
    assert(result == VK_SUCCESS);
    assert(count > 0);

    hkArray_VkLayerProperties instance_layers = hkarray_VkLayerProperties_create(count);
    result = vkEnumerateInstanceLayerProperties(&count, instance_layers.data);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    assert(result == VK_SUCCESS);
    assert(count > 0);

    hkArray_VkExtensionProperties instance_extension = hkarray_VkExtensionProperties_create(count);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, instance_extension.data);

    hkArray_strptr extension_names = hkarray_strptr_create(count);
    str layers[] = { "VK_LAYER_NV_optimus" };
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
    str extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report" };
#else
    str extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
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
    callback_create_info.pfnCallback = &debugReportCallback;

    VkResult result = vkCreateDebugReportCallbackEXT(*out_instance, &callback_create_info, NULL, &error_callback);
    assert(result == VK_SUCCESS);
    callback_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callback_create_info.pfnCallback = &debugReportCallback;

    result = vkCreateDebugReportCallbackEXT(*out_instance, &callback_create_info, NULL, &warning_callback);
    assert(result == VK_SUCCESS);

}
#endif
    hkarray_VkLayerProperties_destroy(&instance_layers);
    hkarray_VkExtensionProperties_destroy(&instance_extension);
    hkarray_strptr_destroy(&extension_names);
}

void setupPhysicalDevice(VkInstance instance, VkPhysicalDevice *out_physical_device, VkDevice *out_device) {
    u32 device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    assert(result == VK_SUCCESS);
    assert(device_count != 0);

    // get device handles
    hkArray_VkPhysicalDevice physical_devices = hkarray_VkPhysicalDevice_create(device_count);
    result = vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data);
    assert(result == VK_SUCCESS);
    assert(device_count >= 0);

    *out_physical_device = physical_devices.data[0];
    for (u32 i = 0; i < device_count; ++i) {
        VkPhysicalDeviceProperties device_properties;
        memset(&device_properties, 0, sizeof(device_properties));
        vkGetPhysicalDeviceProperties(physical_devices.data[i], &device_properties);
        printf("Driver Version: %d.\n", device_properties.driverVersion);
        printf("Device Name: %s.\n", device_properties.deviceName);
        printf("API Version: %d.%d.%d.\n", 
            (device_properties.apiVersion>>22)&0x3FF,
            (device_properties.apiVersion>>12)&0x3FF,
            (device_properties.apiVersion&0xFFF)
        );
    }

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(*out_physical_device, &memory_properties);

    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = 0;
    queue_create_info.queueCount = 1;
    float queue_priorities[] = { 1.0f };
    queue_create_info.pQueuePriorities = queue_priorities;

    str device_extensions[] = { "VK_KHR_swapchain" };
    str layers[] = { "VK_LAYER_NV_optimus" };

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = layers;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;

    VkPhysicalDeviceFeatures features = {0};
    features.shaderClipDistance = VK_TRUE;

    device_create_info.pEnabledFeatures = &features;

    result = vkCreateDevice(*out_physical_device, &device_create_info, NULL, out_device);
    assert(result == VK_SUCCESS);
}

bool checkValidationLayerSupport() {
    bool layer_found = false;

    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    hkArray_VkLayerProperties available_layers = hkarray_VkLayerProperties_create(layer_count);
    VkResult result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data);
    assert(result == VK_SUCCESS);

    for (i32 validation_layer_count = 0; validation_layer_count < sizeofarray(validation_layers); ++validation_layer_count) {
        for (i32 available_layer_count = 0; available_layer_count < available_layers.length; ++available_layer_count) {
            if (strcmp(validation_layers[validation_layer_count], available_layers.data[available_layer_count].layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if(!layer_found) {
            layer_found = false;
        }
    }
    hkarray_VkLayerProperties_destroy(&available_layers);
    return layer_found;
}

// INSTANCE
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html
//
void setupVulkanInstance2() {
    if (enable_validation_layers && !checkValidationLayerSupport()) {
        // validation layers requested, but not available!
        __debugbreak();
    }
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    create_info.enabledExtensionCount = glfwExtensionCount;
    create_info.ppEnabledExtensionNames = glfwExtensions;

    if (enable_validation_layers) {
        create_info.enabledLayerCount = sizeofarray(validation_layers);
        create_info.ppEnabledLayerNames = validation_layers;
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&create_info, NULL, &g_instance);
    assert(result == VK_SUCCESS);
}

// PHYSICAL
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html
//
structdef(QueueFamilyIndices) {
    u32 graphics_family;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, bool *out_has_value) {
    QueueFamilyIndices indices = {0};
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    hkArray_VkQueueFamilyProperties queue_families = hkarray_VkQueueFamilyProperties_create(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data);

    for (int i = 0; i < queue_families.length; ++i) {
        if (queue_families.data->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            *out_has_value = true;
            break;
        }
    }
    hkarray_VkQueueFamilyProperties_destroy(&queue_families);
    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    // VkPhysicalDeviceProperties device_properties;
    // vkGetPhysicalDeviceProperties(g_physical_device, &device_properties);

    // VkPhysicalDeviceFeatures device_features;
    // vkGetPhysicalDeviceFeatures(device, &device_features);

    bool has_value = false;
    QueueFamilyIndices indices = findQueueFamilies(device, &has_value);
    printf("graphics family = %d.\n", indices.graphics_family);

    return has_value;
}

void setupPhysicalDevice2() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(g_instance, &device_count, NULL);
    assert(device_count != 0);

    hkArray_VkPhysicalDevice physical_devices = hkarray_VkPhysicalDevice_create(device_count);
    VkResult result = vkEnumeratePhysicalDevices(g_instance, &device_count, physical_devices.data);
    assert(result == VK_SUCCESS);

    for (int i = 0; i < physical_devices.length; ++i) {
        if (isDeviceSuitable(physical_devices.data[i])) {
            g_physical_device = physical_devices.data[i];
            break;
        }
    }
    assert(g_physical_device != VK_NULL_HANDLE);
}

// LOGICAL
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/04_Logical_device_and_queues.html
void createLogicalDevice() {
    bool has_value = false;
    QueueFamilyIndices indices = findQueueFamilies(g_physical_device, &has_value);

    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = indices.graphics_family;
    queue_create_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = 0;
    create_info.enabledLayerCount = 0;
    // if (enableValidationLayers) {
    //     create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     create_info.ppEnabledLayerNames = validationLayers.data();
    // } else {
    //     create_info.enabledLayerCount = 0;
    // }
    VkResult result = vkCreateDevice(g_physical_device, &create_info, NULL, &g_logical_device);
    assert(result == VK_SUCCESS);

    vkGetDeviceQueue(g_logical_device, indices.graphics_family, 0, &g_graphics_queue);
}

void SetupSwapChain(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, int *out_width, int *out_height, VkSwapchainKHR *out_swap_chain, VkImage **out_present_images, VkImageView **out_present_image_views) {
    {
        VkSurfaceCapabilitiesKHR surface_capabilities = {0};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

        VkExtent2D surface_resolution = surface_capabilities.currentExtent;
        *out_width = surface_resolution.width;
        *out_height = surface_resolution.height;

        VkSwapchainCreateInfoKHR swapchain_create_info = {0};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface = surface;
        swapchain_create_info.minImageCount = 2;
        swapchain_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swapchain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        swapchain_create_info.imageExtent = surface_resolution;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        swapchain_create_info.clipped = true;
        swapchain_create_info.oldSwapchain = NULL;

        VkResult result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, out_swap_chain);
        assert(result == VK_SUCCESS);
    }
    {
        u32 image_count = 0;
        vkGetSwapchainImagesKHR(device, *out_swap_chain, &image_count, NULL);
        assert(image_count == 2);

        hkArray_VkImage present_images = hkarray_VkImage_create(image_count);
        *out_present_images = present_images.data;

        VkResult result = vkGetSwapchainImagesKHR(device, *out_swap_chain, &image_count, NULL);
        assert(result == VK_SUCCESS);

        result = vkGetSwapchainImagesKHR(device, *out_swap_chain, &image_count, *out_present_images);
        assert(result == VK_SUCCESS);

        hkarray_VkImage_destroy(&present_images);
    }
    {
        hkArray_VkImageView present_image_views = hkarray_VkImageView_create(2);
        *out_present_image_views = present_image_views.data;
        for (u32 i = 0; i < 2; ++i) {
            VkImageViewCreateInfo image_view_create_info = {0};
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.image = (*out_present_images)[i];
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
            image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_create_info.subresourceRange.baseMipLevel = 0;
            image_view_create_info.subresourceRange.levelCount = 1;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount = 1;
            VkResult result = vkCreateImageView(device, &image_view_create_info, NULL, &(*out_present_image_views)[i]);
            assert(result == VK_SUCCESS);
            hkarray_VkImageView_destroy(&present_image_views);
        }
    }

}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    // AttachConsole(ATTACH_PARENT_PROCESS);
    OutputDebugString(L"Vulkan.\n");

    // WNDCLASS window_class = {0};
    // const wchar_t class_name[] = L"Sample Window Class";
    // window_class.lpfnWndProc = windowProc;
    // window_class.hInstance = hInstance;
    // window_class.lpszClassName = class_name;
    // window_class.style = CS_HREDRAW | CS_VREDRAW;

    // WORD r = RegisterClass(&window_class);
    // if(r == 0) {
    //     DWORD error = GetLastError();
    //     LPSTR errormessage = getLastErrorAsString();
    //     printf("%lu:%s\n", error, errormessage);
    //     return 0;
    // }

    // 
    // HWND window_handle = CreateWindowEx(0, class_name, L"Win64 Vulkan", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    // if(window_handle == NULL) {
    //     DWORD error = GetLastError();
    //     LPSTR errormessage = getLastErrorAsString();
    //     printf("%lu:%s\n", error, errormessage);
    //     return 0;
    // }
    // ShowWindow(window_handle, nCmdShow);
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int width = 800;
    int height = 600;

    GLFWwindow *window_handle = glfwCreateWindow(width, height, "Vulkan", NULL, NULL);
    assert(window_handle != NULL);

    // VkInstance instance = NULL;
    // VkSurfaceKHR surface = NULL;
    // setupVulkanInstance(window_handle, &instance, &surface);
    setupVulkanInstance2();

    // VkPhysicalDevice physical_device = NULL;
    // VkDevice device = NULL;
    // setupPhysicalDevice(instance, &physical_device, &device);
    setupPhysicalDevice2();


    // VkSwapchainKHR swap_chain = NULL;
    // VkImage *present_images = NULL;
    // VkImageView *present_image_views = NULL;
    // SetupSwapChain(device, physical_device, surface, &width, &height, &swap_chain, &present_images, &present_image_views);

    while (!glfwWindowShouldClose(window_handle)) {
        glfwPollEvents();
    }

    // MSG message = {0};
    // while (GetMessage(&message, NULL, 0, 0) > 0) {
    //     TranslateMessage(&message);
    //     DispatchMessage(&message);
    // }

    return 0;
}
