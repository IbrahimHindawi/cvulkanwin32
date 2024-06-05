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

#include <meta/gen/hkArray_core.h>
#include <meta/gen/hkArray_VkLayerProperties.h>
#include <meta/gen/hkArray_VkExtensionProperties.h>
#include <meta/gen/hkArray_VkPhysicalDevice.h>
#include <meta/gen/hkArray_VkImage.h>
#include <meta/gen/hkArray_VkImageView.h>
#include <meta/gen/hkArray_VkQueueFamilyProperties.h>
#include <meta/gen/hkArray_VkDeviceQueueCreateInfo.h>
#include <meta/gen/hkArray_VkExtensionProperties.h>
#include <meta/gen/hkArray_VkSurfaceFormatKHR.h>
#include <meta/gen/hkArray_VkPresentModeKHR.h>

#define assert(expr) if (!(expr)) { __debugbreak(); }

GLFWwindow *g_window;
VkInstance g_instance;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_logical_device;
VkQueue g_graphics_queue;
VkDebugUtilsMessengerEXT g_debug_messenger;
VkSurfaceKHR g_surface;
VkQueue g_present_queue;
VkSwapchainKHR g_swap_chain;
hkArray_VkImage g_swap_chain_images;
VkFormat g_swap_chain_image_format;
VkExtent2D g_swap_chain_extent;
hkArray_VkImageView g_swap_chain_image_views;

str validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
str device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
    for (u32 i = 0; i < glfw_extension_count; ++i) {
        // u32 len = (u32)strlen(glfw_extensions[i]) + 1;
        // memcpy(&extensions.data[i], &glfw_extensions[i], len);
        extensions.data = hkarray_str_append(&extensions, glfw_extensions[i]);
    }
    if (enable_validation_layers) {
        extensions.data = hkarray_str_append(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

// SWAPCHAIN
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html
//
structdef(SwapChainSupportDetails) {
    VkSurfaceCapabilitiesKHR capabilities;
    hkArray_VkSurfaceFormatKHR formats;
    hkArray_VkPresentModeKHR present_modes;
};

structdef(QueueFamilyIndices) {
    u32 graphics_family;
    u32 present_family;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, bool *out_has_value) {
    QueueFamilyIndices indices = {0};

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    hkArray_VkQueueFamilyProperties queue_families = hkarray_VkQueueFamilyProperties_create(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data);

    bool graphics_family_has_value = false;
    bool present_family_has_value = false;
    for (int i = 0; i < queue_families.length; ++i) {
        if (queue_families.data->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            graphics_family_has_value = true;
        }
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_surface, &present_support);

        if (present_support) {
            indices.present_family = i;
            present_family_has_value = true;
        }
        if (graphics_family_has_value && present_family_has_value) {
            *out_has_value = true;
            break;
        }
    }
    hkarray_VkQueueFamilyProperties_destroy(&queue_families);
    return indices;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_surface, &details.capabilities);

    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, NULL);
    if (format_count != 0) {
        details.formats = hkarray_VkSurfaceFormatKHR_create(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, details.formats.data);
    }

    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &present_mode_count, NULL);
    if (present_mode_count != 0) {
        details.present_modes = hkarray_VkPresentModeKHR_create(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &present_mode_count, details.present_modes.data);
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(hkArray_VkSurfaceFormatKHR avaiable_formats) {
    for (u32 avaiable_formats_count = 0; avaiable_formats_count < avaiable_formats.length; ++avaiable_formats_count) {
        if (avaiable_formats.data[avaiable_formats_count].format == VK_FORMAT_B8G8R8A8_SRGB && avaiable_formats.data[avaiable_formats_count].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return avaiable_formats.data[avaiable_formats_count];
        }
    }
    // If that also fails then we could start ranking the available formats based on how "good" they are, 
    // but in most cases it’s okay to just settle with the first format that is specified.
    return avaiable_formats.data[0];
}

VkPresentModeKHR chooseSwapPresentMode(hkArray_VkPresentModeKHR available_present_modes) {
    for (u32 available_present_modes_count = 0; available_present_modes_count < available_present_modes.length; ++available_present_modes_count) {
        if (available_present_modes.data[available_present_modes_count] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes.data[available_present_modes_count];
        }
    }
    // I personally think that VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern. 
    // It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images that are 
    // as up-to-date as possible right until the vertical blank. On mobile devices, where energy usage is more important, 
    // you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead.
    return VK_PRESENT_MODE_FIFO_KHR;
}

// #define hkMath_clamp_gen(type) type clamp_#type((type) value, (type) min, (type) max) { if (value < min) { return min; } else if(value > max) { return max; } else { return value; } }
// hkMath_clamp_gen(u32);

u32 clamp_u32(u32 value, u32 min, u32 max) { if (value < min) { return min; } else if(value > max) { return max; } else { return value; } }

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        i32 width, height;
        glfwGetFramebufferSize(g_window, &width, &height);
        VkExtent2D actual_extent = { (u32)width, (u32)height };
        actual_extent.width = clamp_u32(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = clamp_u32(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extent;
    }
}

void setupSwapChain() {
    VkResult result;
    // should I generate a new meta type to combine the `destroy` procs?
    SwapChainSupportDetails swap_chain_support = querySwapChainSupport(g_physical_device);
    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);
    u32 image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = g_surface;

    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    bool has_value = false;
    QueueFamilyIndices indices = findQueueFamilies(g_physical_device, &has_value);
    uint32_t queueFamilyIndices[] = { indices.graphics_family, indices.present_family };

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = NULL; // Optional
    }
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(g_logical_device, &create_info, NULL, &g_swap_chain);
    assert(result == VK_SUCCESS);

    g_swap_chain_images = hkarray_VkImage_create(image_count);
    result = vkGetSwapchainImagesKHR(g_logical_device, g_swap_chain, &image_count, NULL);
    assert(result == VK_SUCCESS);
    hkarray_VkImage_resize(&g_swap_chain_images, image_count);
    vkGetSwapchainImagesKHR(g_logical_device, g_swap_chain, &image_count, g_swap_chain_images.data);
    g_swap_chain_image_format = surface_format.format;
    g_swap_chain_extent = extent;
}

void setupImageViews() {
    VkResult result = false;
    g_swap_chain_image_views = hkarray_VkImageView_create(g_swap_chain_images.length);
    for (u32 swap_chain_images_count = 0; swap_chain_images_count < g_swap_chain_images.length; ++swap_chain_images_count) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = g_swap_chain_images.data[swap_chain_images_count];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = g_swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        vkCreateImageView(g_logical_device, &create_info, NULL, &g_swap_chain_image_views.data[swap_chain_images_count]);
        assert(result == VK_SUCCESS);
    }
}

// PHYSICAL
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html
//

void setupSurface() {
    if (glfwCreateWindowSurface(g_instance, g_window, NULL, &g_surface) != VK_SUCCESS) {
        printf("Application::Failed to create window surface.\n");
        __debugbreak();
    }
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    bool layer_found = false;

    u32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    hkArray_VkExtensionProperties available_extensions = hkarray_VkExtensionProperties_create(extension_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions.data);

    for (i32 device_extensions_count = 0; device_extensions_count < sizeofarray(device_extensions); ++device_extensions_count) {
        for (i32 available_layer_count = 0; available_layer_count < available_extensions.length; ++available_layer_count) {
            if (strcmp(device_extensions[device_extensions_count], available_extensions.data[available_layer_count].extensionName) == 0) {
                layer_found = true;
                break;
            }
        }
        if(!layer_found) {
            layer_found = false;
        }
    }
    hkarray_VkExtensionProperties_destroy(&available_extensions);
    return layer_found;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    // VkPhysicalDeviceProperties device_properties;
    // vkGetPhysicalDeviceProperties(g_physical_device, &device_properties);

    // VkPhysicalDeviceFeatures device_features;
    // vkGetPhysicalDeviceFeatures(device, &device_features);

    bool queue_families_has_value = false;
    QueueFamilyIndices indices = findQueueFamilies(device, &queue_families_has_value);
    bool extensions_supported = checkDeviceExtensionSupport(device);
    printf("DeviceCheck::QueueFamilyIndices={.graphics_family:%d, .presernt_family:%d}.\n", indices.graphics_family, indices.present_family);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
        swap_chain_adequate = swap_chain_support.present_modes.length > 0 && swap_chain_support.formats.length > 0;
        hkarray_VkSurfaceFormatKHR_destroy(&swap_chain_support.formats);
        hkarray_VkPresentModeKHR_destroy(&swap_chain_support.present_modes);
    }

    return queue_families_has_value && extensions_supported && swap_chain_adequate;
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

    hkArray_VkDeviceQueueCreateInfo queue_create_infos = hkarray_VkDeviceQueueCreateInfo_create(0);
    float queue_priority = 1.0f;
    for (i32 i = 0; i < 1; ++i) {
        VkDeviceQueueCreateInfo queue_create_info = {0};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.data = hkarray_VkDeviceQueueCreateInfo_append(&queue_create_infos, queue_create_info);
    }

    // VkDeviceQueueCreateInfo queue_create_info = {0};
    // queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // queue_create_info.queueFamilyIndex = indices.graphics_family;
    // queue_create_info.queueCount = 1;
    // float queue_priority = 1.0f;
    // queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data;
    create_info.queueCreateInfoCount = (u32)queue_create_infos.length;

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = sizeofarray(device_extensions);
    create_info.ppEnabledExtensionNames = device_extensions;
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
    vkGetDeviceQueue(g_logical_device, indices.present_family, 0, &g_present_queue);
    hkarray_VkDeviceQueueCreateInfo_destroy(&queue_create_infos);
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
    setupSurface();
    setupPhysicalDevice();
    setupLogicalDevice();
    setupSwapChain();
    setupImageViews();

    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
    }

    for (u32 swap_chain_images_count = 0; swap_chain_images_count < g_swap_chain_images.length; ++swap_chain_images_count) {
        vkDestroyImageView(g_logical_device, g_swap_chain_image_views.data[swap_chain_images_count], NULL);
    }
    vkDestroySwapchainKHR(g_logical_device, g_swap_chain, NULL);
    vkDestroyDevice(g_logical_device, NULL);
    if (enable_validation_layers) { DestroyDebugUtilsMessengerEXT(g_instance, g_debug_messenger, NULL); }
    vkDestroySurfaceKHR(g_instance, g_surface, NULL);
    vkDestroyInstance(g_instance, NULL);

    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}
