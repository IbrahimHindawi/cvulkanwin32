#ifndef UNICODE
#   define UNICODE
#endif
#define NO_STDIO_REDIRECT

#include <windows.h>
#include <core.h>
#include <fileops.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan_core.h>

//---------------------------------------------------------------------------------------------------
// haikal metaprogramming monomorphization codegen
//---------------------------------------------------------------------------------------------------
// haikal@hkArray:i8:p
// haikal@hkArray:i16:p
// haikal@hkArray:i32:p
// haikal@hkArray:i64:p
// haikal@hkArray:u8:p
// haikal@hkArray:u16:p
// haikal@hkArray:u32:p
// haikal@hkArray:u64:p
// haikal@hkArray:f32:p
// haikal@hkArray:f64:p
// haikal@hkArray:str:p
// haikal@hkArray:VkPhysicalDevice:p
// haikal@hkArray:VkImage:p
// haikal@hkArray:VkImageView:p
// haikal@hkArray:VkFramebuffer:p
// haikal@hkArray:VkPresentModeKHR:e
// haikal@hkArray:VkLayerProperties:s
// haikal@hkArray:VkExtensionProperties:s
// haikal@hkArray:VkQueueFamilyProperties:s
// haikal@hkArray:VkDeviceQueueCreateInfo:s
// haikal@hkArray:VkSurfaceFormatKHR:s

#include <hkArray.h>

#include <hkArray.c>


#define assert(expr) if (!(expr)) { __debugbreak(); }

GLFWwindow *g_window;
VkInstance g_instance;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_logical_device;
VkQueue g_graphics_queue;
VkDebugUtilsMessengerEXT g_debug_messenger;
VkSurfaceKHR g_surface;
VkQueue g_present_queue;
VkSwapchainKHR g_swapchain;
hkArray_VkImage g_swapchain_images;
VkFormat g_swapchain_image_format;
VkExtent2D g_swapchain_extent;
hkArray_VkImageView g_swapchain_image_views;
VkRenderPass g_render_pass;
VkPipelineLayout g_pipeline_layout;
VkPipeline g_graphics_pipeline;
str validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
str device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
hkArray_VkFramebuffer g_swapchain_framebuffers;
VkCommandPool g_command_pool;
VkCommandBuffer g_command_buffer;
VkSemaphore g_image_available_semaphore;
VkSemaphore g_render_finished_semaphore;
VkFence g_in_flight_fence;

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

    hkArray_VkLayerProperties available_layers = hkArray_VkLayerProperties_create(layer_count);
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
    hkArray_VkLayerProperties_destroy(&available_layers);
    return layer_found;
}

hkArray_str getRequiredExtensions() {
    u32 glfw_extension_count = 0;
    strptr glfw_extensions = NULL;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    assert(glfw_extensions != NULL);

    hkArray_str extensions = hkArray_str_create(0);
    for (u32 i = 0; i < glfw_extension_count; ++i) {
        // u32 len = (u32)strlen(glfw_extensions[i]) + 1;
        // memcpy(&extensions.data[i], &glfw_extensions[i], len);
        extensions.data = hkArray_str_append(&extensions, glfw_extensions[i]);
    }
    if (enable_validation_layers) {
        extensions.data = hkArray_str_append(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

    hkArray_str_destroy(&extensions);
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

    hkArray_VkQueueFamilyProperties queue_families = hkArray_VkQueueFamilyProperties_create(queue_family_count);
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
    hkArray_VkQueueFamilyProperties_destroy(&queue_families);
    return indices;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_surface, &details.capabilities);

    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, NULL);
    if (format_count != 0) {
        details.formats = hkArray_VkSurfaceFormatKHR_create(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, details.formats.data);
    }

    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &present_mode_count, NULL);
    if (present_mode_count != 0) {
        details.present_modes = hkArray_VkPresentModeKHR_create(present_mode_count);
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
#define concat(a, b) a ## b 
#define hkMath_clamp_procgen(type) \
    type concat(hkMath_clamp_, type)(type value, type min, type max) { if (value < min) { return min; } else if(value > max) { return max; } else { return value; } }
hkMath_clamp_procgen(f32)
hkMath_clamp_procgen(f64)
hkMath_clamp_procgen(u8)
hkMath_clamp_procgen(u16)
hkMath_clamp_procgen(u32)
hkMath_clamp_procgen(u64)
hkMath_clamp_procgen(i8)
hkMath_clamp_procgen(i16)
hkMath_clamp_procgen(i32)
hkMath_clamp_procgen(i64)
// hkMath_clamp_gen(u32);

u32 clamp_u32(u32 value, u32 min, u32 max) { if (value < min) { return min; } else if(value > max) { return max; } else { return value; } }

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
    hkMath_clamp_i32(1, 1, 1);
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        i32 width, height;
        glfwGetFramebufferSize(g_window, &width, &height);
        VkExtent2D actual_extent = { (u32)width, (u32)height };
        actual_extent.width = hkMath_clamp_u32(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = hkMath_clamp_u32(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extent;
    }
}

void setupSwapChain() {
    VkResult result;
    // should I generate a new meta type to combine the `destroy` procs?
    SwapChainSupportDetails swapchain_support = querySwapChainSupport(g_physical_device);
    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swapchain_support.present_modes);
    VkExtent2D extent = chooseSwapExtent(swapchain_support.capabilities);
    u32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
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
    uint32_t queue_family_indices[] = { indices.graphics_family, indices.present_family };

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = NULL; // Optional
    }
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(g_logical_device, &create_info, NULL, &g_swapchain);
    assert(result == VK_SUCCESS);

    g_swapchain_images = hkArray_VkImage_create(image_count);
    result = vkGetSwapchainImagesKHR(g_logical_device, g_swapchain, &image_count, NULL);
    assert(result == VK_SUCCESS);
    hkArray_VkImage_resize(&g_swapchain_images, image_count);
    vkGetSwapchainImagesKHR(g_logical_device, g_swapchain, &image_count, g_swapchain_images.data);
    g_swapchain_image_format = surface_format.format;
    g_swapchain_extent = extent;
}

void setupImageViews() {
    VkResult result = false;
    g_swapchain_image_views = hkArray_VkImageView_create(g_swapchain_images.length);
    for (u32 swapchain_images_count = 0; swapchain_images_count < g_swapchain_images.length; ++swapchain_images_count) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = g_swapchain_images.data[swapchain_images_count];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = g_swapchain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        vkCreateImageView(g_logical_device, &create_info, NULL, &g_swapchain_image_views.data[swapchain_images_count]);
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

    hkArray_VkExtensionProperties available_extensions = hkArray_VkExtensionProperties_create(extension_count);
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
    hkArray_VkExtensionProperties_destroy(&available_extensions);
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

    bool swapchain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swapchain_support = querySwapChainSupport(device);
        swapchain_adequate = swapchain_support.present_modes.length > 0 && swapchain_support.formats.length > 0;
        hkArray_VkSurfaceFormatKHR_destroy(&swapchain_support.formats);
        hkArray_VkPresentModeKHR_destroy(&swapchain_support.present_modes);
    }

    return queue_families_has_value && extensions_supported && swapchain_adequate;
}

void setupPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(g_instance, &device_count, NULL);
    assert(device_count != 0);

    hkArray_VkPhysicalDevice physical_devices = hkArray_VkPhysicalDevice_create(device_count);
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

    hkArray_VkDeviceQueueCreateInfo queue_create_infos = hkArray_VkDeviceQueueCreateInfo_create(0);
    float queue_priority = 1.0f;
    for (i32 i = 0; i < 1; ++i) {
        VkDeviceQueueCreateInfo queue_create_info = {0};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.data = hkArray_VkDeviceQueueCreateInfo_append(&queue_create_infos, queue_create_info);
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
    hkArray_VkDeviceQueueCreateInfo_destroy(&queue_create_infos);
}

//
// Graphics Pipeline
// 
VkShaderModule createShaderModule() {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = fops_buffer_alloc_len;
    create_info.pCode = (u32 *)fops_buffer;
    VkResult result = false;
    VkShaderModule shader_module = {0};
    result = vkCreateShaderModule(g_logical_device, &create_info, NULL, &shader_module);
    assert(result == VK_SUCCESS);
    return shader_module;
}

void setupGraphicsPipeline() {
    // str vertShaderCode = 
    fops_read_bin("assets/shaders/shader.vert.spv");
    VkShaderModule vert_shader_module = createShaderModule();
    fops_read_bin("assets/shaders/shader.frag.spv");
    VkShaderModule frag_shader_module = createShaderModule();

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = (u32)sizeofarray(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    VkResult result = vkCreatePipelineLayout(g_logical_device, &pipeline_layout_info, NULL, &g_pipeline_layout);
    assert(result == VK_SUCCESS);

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = g_pipeline_layout;
    pipeline_info.renderPass = g_render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    result = vkCreateGraphicsPipelines(g_logical_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &g_graphics_pipeline);
    assert(result == VK_SUCCESS);

    vkDestroyShaderModule(g_logical_device, frag_shader_module, NULL);
    vkDestroyShaderModule(g_logical_device, vert_shader_module, NULL);
}

void setupRenderPass() {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = g_swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(g_logical_device, &render_pass_info, NULL, &g_render_pass);
    assert(result == VK_SUCCESS);
}

void setupFramebuffers() {
    g_swapchain_framebuffers = hkArray_VkFramebuffer_create(g_swapchain_image_views.length);
    for (usize swapchain_image_views_count = 0; swapchain_image_views_count < g_swapchain_image_views.length; swapchain_image_views_count++) {
        VkResult result = false;
        VkImageView attachments[] = { g_swapchain_image_views.data[swapchain_image_views_count] };

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = g_render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = g_swapchain_extent.width;
        framebuffer_info.height = g_swapchain_extent.height;
        framebuffer_info.layers = 1;

        vkCreateFramebuffer(g_logical_device, &framebuffer_info, NULL, &g_swapchain_framebuffers.data[swapchain_image_views_count]);
        assert(result == VK_SUCCESS);
    }
}

void setupCommandPool() {
    bool has_value = false;
    QueueFamilyIndices queue_family_indices = findQueueFamilies(g_physical_device, &has_value);

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family;

    VkResult result = vkCreateCommandPool(g_logical_device, &pool_info, NULL, &g_command_pool);
    assert(result == VK_SUCCESS);
}

void setupCommandBuffer() {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = g_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(g_logical_device, &alloc_info, &g_command_buffer);
    assert(result == VK_SUCCESS);
}

void recordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0; // Optional
    begin_info.pInheritanceInfo = NULL; // Optional

    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS;
    assert(result == VK_SUCCESS);

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = g_render_pass;
    render_pass_info.framebuffer = g_swapchain_framebuffers.data[image_index];
    // check this:
    VkOffset2D offset = {0};
    render_pass_info.renderArea.offset = offset;
    render_pass_info.renderArea.extent = g_swapchain_extent;
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(g_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(g_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_graphics_pipeline);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)(g_swapchain_extent.width);
    viewport.height = (f32)(g_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(g_command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    // check this:
    scissor.offset = offset;
    scissor.extent = g_swapchain_extent;
    vkCmdSetScissor(g_command_buffer, 0, 1, &scissor);
    vkCmdDraw(g_command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(g_command_buffer);

    result = vkEndCommandBuffer(g_command_buffer);
    assert(result == VK_SUCCESS);
}

void setupSyncObjects() {
    VkSemaphoreCreateInfo semaphore_create_info = {0};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult semaphore_result = vkCreateSemaphore(g_logical_device, &semaphore_create_info, NULL, &g_image_available_semaphore);
    VkResult semaphore_result2 = vkCreateSemaphore(g_logical_device, &semaphore_create_info, NULL, &g_render_finished_semaphore);
    VkResult fence_result = vkCreateFence(g_logical_device, &fence_create_info, NULL, &g_in_flight_fence);
    assert(semaphore_result == VK_SUCCESS || semaphore_result2 == VK_SUCCESS || fence_result == VK_SUCCESS);
}

void drawFrame() {
    vkWaitForFences(g_logical_device, 1, &g_in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_logical_device, 1, &g_in_flight_fence);

    u32 image_index = 0;
    vkAcquireNextImageKHR(g_logical_device, g_swapchain, UINT64_MAX, g_image_available_semaphore, VK_NULL_HANDLE, &image_index);
    vkResetCommandBuffer(g_command_buffer, 0);
    recordCommandBuffer(g_command_buffer, image_index);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {g_image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &g_command_buffer;

    VkSemaphore signal_semaphores[] = {g_render_finished_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    VkResult result = vkQueueSubmit(g_graphics_queue, 1, &submit_info, g_in_flight_fence);
    assert(result == VK_SUCCESS);

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {g_swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;

    present_info.pImageIndices = &image_index;

    present_info.pResults = NULL;
    vkQueuePresentKHR(g_present_queue, &present_info);

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
    setupRenderPass();
    setupGraphicsPipeline();
    setupFramebuffers();
    setupCommandPool();
    setupCommandBuffer();
    setupSyncObjects();

    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(g_logical_device);

    vkDestroySemaphore(g_logical_device, g_image_available_semaphore, NULL);
    vkDestroySemaphore(g_logical_device, g_render_finished_semaphore, NULL);
    vkDestroyFence(g_logical_device, g_in_flight_fence, NULL);
    vkDestroyCommandPool(g_logical_device, g_command_pool, NULL);
    for (u32 swapchain_framebuffer_count = 0; swapchain_framebuffer_count < g_swapchain_framebuffers.length; ++swapchain_framebuffer_count) {
        vkDestroyFramebuffer(g_logical_device, g_swapchain_framebuffers.data[swapchain_framebuffer_count], NULL);
    }
    vkDestroyPipeline(g_logical_device, g_graphics_pipeline, NULL);
    vkDestroyPipelineLayout(g_logical_device, g_pipeline_layout, NULL);
    vkDestroyRenderPass(g_logical_device, g_render_pass, NULL);
    for (u32 swapchain_images_count = 0; swapchain_images_count < g_swapchain_images.length; ++swapchain_images_count) {
        vkDestroyImageView(g_logical_device, g_swapchain_image_views.data[swapchain_images_count], NULL);
    }
    vkDestroySwapchainKHR(g_logical_device, g_swapchain, NULL);
    vkDestroyDevice(g_logical_device, NULL);
    if (enable_validation_layers) { DestroyDebugUtilsMessengerEXT(g_instance, g_debug_messenger, NULL); }
    vkDestroySurfaceKHR(g_instance, g_surface, NULL);
    vkDestroyInstance(g_instance, NULL);

    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}
