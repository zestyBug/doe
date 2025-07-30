#if !defined(VULKANAPP_HPP)
#define VULKANAPP_HPP

#include "glfw/glfw3.h"
#include "vulkan_wrapper.h"

struct VulkanApp {
static constexpr char const* APP_SHORT_NAME = "VKProject";
static const char* GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity);
static const char* GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type);
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

	
	VkResult initInstance();
	VkInstance instance = VK_NULL_HANDLE;

	VkResult initSurface();
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	GLFWwindow* window;

	VkResult TestPhysicalDeviceInfo(VkPhysicalDevice pd);
	VkResult autoSelectDevice();
	// physical device
	VkPhysicalDevice pdevice = VK_NULL_HANDLE;
	
	VkResult initDevice();
	// logical device
	VkDevice ldevice = VK_NULL_HANDLE;

	VkResult initQueue();
	struct {
		uint32_t index = 0;
		VkQueue queue = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;
	} graphic,transfer;

	VkResult test();
};

#endif // VULKANAPP_HPP
