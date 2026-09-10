#if !defined(VKCONTEXT_HPP)
#define VKCONTEXT_HPP

#include "vulkan/wrapper.h"
#include "ECS/Base/Constants.hpp"
#include "cutil/basics.hpp"

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
    /// @brief A bootstrap class for vulkan.
    /// @warning This class must has nothing to do with the rest of the engine, including ECS, resources, systems.
    struct VKContext {
        // a context, so OS can handle many apps
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        uint32_t imageCount = 0;
        VkQueue queue = VK_NULL_HANDLE;
        VkFence queueFence = VK_NULL_HANDLE;
        VkSemaphore queueSemaphore = VK_NULL_HANDLE;
        // actual physical device,
        // this refrence can be used to obtain info about device
        VkPhysicalDevice pdevice = VK_NULL_HANDLE;
		// a logical device, a refrence to a real device queue.
		// by having this, you are officialy using a GPU
		VkDevice device = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkRenderPass renderpass = VK_NULL_HANDLE;
        VkDescriptorPool dpool = VK_NULL_HANDLE;
        VkCommandPool cpool = VK_NULL_HANDLE;
        VkExtent2D surfaceExtend;
        VkImageView bufferView[Constants::MaximumSwapchainImageCount];
        VkFramebuffer frambuffer[Constants::MaximumSwapchainImageCount];
        VkImage bufferImage[Constants::MaximumSwapchainImageCount];
        VkSemaphore imageSemaphores[Constants::MaximumSwapchainImageCount];
        static VkBool32 TestSurfaceSupport(VkPhysicalDevice pd, VkSurfaceKHR surface);
        VKContext();
        ~VKContext();
        // create a instance with required extentions and layers
        void initialize();
        // create surface base off of the window. unlike swapchain, it can only be created once.
        void createSurface();
        // selects a physical device, create a logical device and command pool of that device
        // requires surface to check compatibility
        void selectDevice();
        void initRender();
        void resetSwapchain();
        int render();
    };
} // namespace ECS

#endif // VKCONTEXT_HPP