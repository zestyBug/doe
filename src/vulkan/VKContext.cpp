#include "vulkan/VKContext.hpp"
#include "cutil/basics.hpp"
#include <vector>
#define arrayCount(X) (sizeof(X)/sizeof(*X))
using namespace ECS;

class VulkanException : public std::exception {
public:
    VulkanException(VkResult result, const char* expression);
    ~VulkanException(){};
    /** Returns a C-style character string describing the general cause of
     *  the current error (the same string passed to the ctor).  */
    const char* what() const noexcept {return message.get();};
private:
    static const char* VkResultToString(VkResult result);
    std::unique_ptr<char[]> message;
};
VulkanException::VulkanException(VkResult result, const char* expression):std::exception{}
{
    this->message = std::make_unique<char[]>(512);
    snprintf(this->message.get(),511,"%s: %s",expression,VkResultToString(result));
}
const char* VulkanException::VkResultToString(VkResult result){
    switch (result){
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR:
        return "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
        return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
        return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
    default:
        return "UNKNOWN_ERROR";
    }
}

VkBool32 VKContext::TestSurfaceSupport(VkPhysicalDevice pd, VkSurfaceKHR surface){
    VkResult res;
    {
        VkSurfaceCapabilitiesKHR capability;
        res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd,surface,&capability);
        if(res)
            throw VulkanException(res, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    }
    {
        uint32_t surface_format_count = 0;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, NULL);
        if (res)
            throw VulkanException(res, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        if (surface_format_count == 0)
            return 1;
        std::vector<VkSurfaceFormatKHR> surface_formats{ surface_format_count };
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, surface_formats.data());
        if (res)
            throw VulkanException(res, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
        // the surface has no preferred format.
        if (surface_format_count == 1) {
            if (surface_formats.at(0).format == VK_FORMAT_UNDEFINED)
                goto success;
        }
        for (auto& sf : surface_formats)
            if (sf.format == VK_FORMAT_B8G8R8A8_SRGB)
                goto success;
        //"Unsupported Surface Format";
    }
    return 1;
success:
    {
        uint32_t presentMode_count;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentMode_count, NULL);
        if (res)
            throw VulkanException(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");
        std::vector<VkPresentModeKHR> presentModes{ presentMode_count };
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentMode_count, presentModes.data());
        if (res)
            throw VulkanException(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }
    return 0;
}
VKContext::~VKContext(){
    vkDeviceWaitIdle(this->device);
    if(instance){
        if(device){
            if(surface){
                vkDestroySurfaceKHR(instance,surface,NULL);
            }
            vkDestroyDevice(device,NULL);
        }
        vkDestroyInstance(instance,NULL);
    }
}
// create a instance with required extentions and layers
void VKContext::initialize(){
    if(VKInitialize())
        throw std::runtime_error("VKInitialize");
    // initialize the VkApplicationInfo structure
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Sample",
        .applicationVersion = 1,
        .pEngineName = "DOE",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_0,
    };
    char const* const exts[] = { 
        "VK_KHR_get_physical_device_properties2", 
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME
    #endif
    };
    char const* const lays[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    // initialize the VkInstanceCreateInfo structure
    const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = arrayCount(lays),
        .ppEnabledLayerNames = lays,
        .enabledExtensionCount = arrayCount(exts),
        .ppEnabledExtensionNames = exts,
    };
    VkResult res = vkCreateInstance(&inst_info, nullptr, &instance);
    if (res)
        throw VulkanException(res, "vkCreateInstance");
}
#if defined(VK_USE_PLATFORM_WIN32_KHR)
void VKContext::createSurface(GLFWwindow *handle){
    if(VKInitializeWInstance(this->instance))
        throw std::runtime_error("VKInitializeWInstance");
    VkWin32SurfaceCreateInfoKHR cInfo {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = glfwGetWin32Instance(),
        .hwnd = glfwGetWin32Window(handle),
    };
    VkResult res = vkCreateWin32SurfaceKHR(this->instance, &cInfo, NULL, &this->surface);
    if(res)
        throw VulkanException(res, "vkCreateWin32SurfaceKHR");
}
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
void VKContext::createSurface(GLFWwindow *handle){
    if(VKInitializeWInstance(this->instance))
        throw std::runtime_error("VKInitializeWInstance");
    VkXlibSurfaceCreateInfoKHR cInfo {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .dpy = glfwGetX11Display(),
        .window = glfwGetX11Window(handle),
    };
    VkResult res = vkCreateXlibSurfaceKHR(this->instance, &cInfo, NULL, &this->surface);
    if(res)
        throw VulkanException(res, "vkCreateXlibSurfaceKHR");
}
#endif
// selects a physical device, create a logical device and command pool of that device
// requires surface to check compatibility
void VKContext::selectDevice(){
    std::vector<VkPhysicalDevice> physical_devices;
    VkResult res;
    {
        uint32_t gpu_count = 0;
        res = vkEnumeratePhysicalDevices(this->instance, &gpu_count, NULL);
        if (res)
            throw VulkanException(res, "vkEnumeratePhysicalDevices");
        if (!gpu_count)
            throw VulkanException(res, "vkEnumeratePhysicalDevices: no gpu ws found");
        physical_devices.resize(gpu_count);
        res = vkEnumeratePhysicalDevices(this->instance, &gpu_count, physical_devices.data());
        if (res)
            throw VulkanException(res, "vkEnumeratePhysicalDevices");
    }

    float queue_priorities[1] = {1.0f};
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };
    char const* const exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = arrayCount(exts),
        .ppEnabledExtensionNames = exts,
        .pEnabledFeatures = NULL,
    };

    for (auto& pd : physical_devices)
    {
        VkPhysicalDeviceProperties physical_device_prop = {};
        uint32_t queue_family_count = 0;

        vkGetPhysicalDeviceProperties(pd, &physical_device_prop);
        printf("Device Name: %s\n", physical_device_prop.deviceName);

        vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, NULL);
        if (!queue_family_count)
            continue;
        std::vector<VkQueueFamilyProperties> queue_family_prop{ queue_family_count };
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, queue_family_prop.data());
        if (TestSurfaceSupport(pd, surface))
            continue;

        for (unsigned int i = 0; i < queue_family_count; i++) {
            VkBool32 supported = false;
            // some devices are made for computation and does not support graphical commands
            if (!(queue_family_prop[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                continue;
            if (!(queue_family_prop[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
                continue;
        #ifdef VK_USE_PLATFORM_WIN32_KHR
            if(!vkGetPhysicalDeviceWin32PresentationSupportKHR(pd,i))
                continue;
        #endif
            // A Device may not be plugged into a monitor or not have any graphcal output
            // which could make direct interactions with displayable images difficult or impossible.
            // ie: it can render but result image must be copied to another Device that can render to display.
            res = vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &supported);
            if (res)
                throw VulkanException(res, "vkGetPhysicalDeviceSurfaceSupportKHR");
            if (!supported)
                continue;
            queue_info.queueFamilyIndex = i;
            this->queueFamilyIndex = i;
            this->pdevice = pd;
            goto exit_loop;
        }
        // gpu was not suitable
    }
    throw VulkanException(res, "no suitable GPU was found");
exit_loop:
    res = vkCreateDevice(this->pdevice, &device_info, NULL, &this->device);
    if (res)
        throw VulkanException(res, "vkCreateDevice");
    if(VKInitializeWDevice(this->device))
        throw std::runtime_error("VKInitializeWDevice");
    vkGetDeviceQueue(this->device, this->queueFamilyIndex, 0, &this->queue);
}
void VKContext::initSwapchain(){
}