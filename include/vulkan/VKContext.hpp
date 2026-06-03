#if !defined(VKCONTEXT_HPP)
#define VKCONTEXT_HPP

#include "vulkan/wrapper.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"

namespace ECS
{
    // A kind of bootstrap class for vulkan
    struct VKContext {
        // a context, so OS can handle many apps
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        VkQueue queue = VK_NULL_HANDLE;
        // actual physical device,
        // this refrence can be used to obtain info about device
        VkPhysicalDevice pdevice = VK_NULL_HANDLE;
		// a logical device, a refrence to a real device queue.
		// by having this, you are officialy using a GPU
		VkDevice device = VK_NULL_HANDLE;
        static VkBool32 TestSurfaceSupport(VkPhysicalDevice pd, VkSurfaceKHR surface);
        ~VKContext();
        // create a instance with required extentions and layers
        void initialize();
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        void createSurface(GLFWwindow *handle);
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        void createSurface(GLFWwindow *handle);
        #else
        #error undefined platform
        #endif
        // selects a physical device, create a logical device and command pool of that device
        // requires surface to check compatibility
        void selectDevice();
        void initDevice();
    };
} // namespace ECS

#endif // VKCONTEXT_HPP