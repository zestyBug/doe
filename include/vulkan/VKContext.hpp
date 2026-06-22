#if !defined(VKCONTEXT_HPP)
#define VKCONTEXT_HPP

#include "vulkan/wrapper.h"
#include "ECS/Base/Constants.hpp"
#include "cutil/basics.hpp"
#include "glfw/glfw3.h"

namespace ECS
{
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
    // A kind of bootstrap class for vulkan
    struct VKContext {
        // a context, so OS can handle many apps
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        uint32_t imageCount = 0;
        VkQueue queue = VK_NULL_HANDLE;
        VkFence queueFence = VK_NULL_HANDLE;
        VkSemaphore imageSemaphore = VK_NULL_HANDLE;
        VkSemaphore queueSemaphore = VK_NULL_HANDLE;
        // actual physical device,
        // this refrence can be used to obtain info about device
        VkPhysicalDevice pdevice = VK_NULL_HANDLE;
		// a logical device, a refrence to a real device queue.
		// by having this, you are officialy using a GPU
		VkDevice device = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkRenderPass renderpass = VK_NULL_HANDLE;
        VkExtent2D surfaceExtend;
        VkImageView bufferView[Constants::MaximumSwapchainImageCount];
        VkFramebuffer frambuffer[Constants::MaximumSwapchainImageCount];
        VkImage bufferImage[Constants::MaximumSwapchainImageCount];
        static VkBool32 TestSurfaceSupport(VkPhysicalDevice pd, VkSurfaceKHR surface);
        VKContext();
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
        void initRender();
        void resetSwapchain();
        int render();
    };
} // namespace ECS

#endif // VKCONTEXT_HPP