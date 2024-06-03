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

GLFWwindow *g_window;
VkInstance g_instance;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_logical_device;
VkQueue g_graphics_queue;
VkDebugUtilsMessengerEXT g_debug_messenger;

str validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

LPSTR getLastErrorAsString()
{
    DWORD errorMessageID = GetLastError();
    if(errorMessageID == 0) {
        return NULL;
    }
    LPSTR messageBuffer = NULL;
    // size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    return messageBuffer;
}

VkBool32 VKAPI_PTR debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
    return true;
}

// VALIDATION
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html
//
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

hkArray_str getRequiredExtensions() {
    u32 glfw_extension_count = 0;
    strptr glfw_extensions = NULL;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    assert(glfw_extensions != NULL);

    hkArray_str extensions = hkarray_str_create(0);
    for (int i = 0; i < glfw_extension_count; ++i) {
        // u32 len = (u32)strlen(glfw_extensions[i]) + 1;
        // memcpy(&extensions.data[i], &glfw_extensions[i], len);
        extensions.data = hkarray_str_append_ptr(&extensions, glfw_extensions[i]);
    }
    if (enable_validation_layers) {
        extensions.data = hkarray_str_append_ptr(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    printf("ValidationLayer::%s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *create_info) {
    create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info->pfnUserCallback = debugCallback;
    create_info->pUserData = NULL; // Optional
}

void setupDebugMessenger() {
    if(!enable_validation_layers) { 
        return; 
    }
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    populateDebugMessengerCreateInfo(&create_info);

    if (CreateDebugUtilsMessengerEXT(g_instance, &create_info, NULL, &g_debug_messenger) != VK_SUCCESS) {
        printf("failed to set up debug messenger!");
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

// INSTANCE
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html
//
void setupVulkanInstance() {
    if (enable_validation_layers && !checkValidationLayerSupport()) {
        printf("ValidationLayers::Requested, but not available!\n");
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

    hkArray_str extensions = getRequiredExtensions();
    create_info.enabledExtensionCount = (u32)extensions.length;
    create_info.ppEnabledExtensionNames = extensions.data;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    if (enable_validation_layers) {
        create_info.enabledLayerCount = sizeofarray(validation_layers);
        create_info.ppEnabledLayerNames = validation_layers;
        populateDebugMessengerCreateInfo(&debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = NULL;
    }

    VkResult result = vkCreateInstance(&create_info, NULL, &g_instance);
    assert(result == VK_SUCCESS);

    hkarray_str_destroy(&extensions);
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
    printf("indices: %d\n", indices.graphics_family);
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

void setupPhysicalDevice() {
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
void setupLogicalDevice() {
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

// int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
int main() {
    printf("Vulkan App Init.\n");
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int width = 800;
    int height = 600;

    g_window = glfwCreateWindow(width, height, "Vulkan", NULL, NULL);
    assert(g_window != NULL);

    setupVulkanInstance();
    setupDebugMessenger();
    setupPhysicalDevice();
    setupLogicalDevice();

    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
    }

    vkDestroyDevice(g_logical_device, NULL);
    if (enable_validation_layers) {
        DestroyDebugUtilsMessengerEXT(g_instance, g_debug_messenger, NULL);
    }
    vkDestroyInstance(g_instance, NULL);
    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}
