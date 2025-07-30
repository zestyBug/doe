#include "VulkanApp.hpp"
#include "glfw/glfw3native.h"
#include <vector>
#include <stdio.h>

#define arrayCount(X) (sizeof(X)/sizeof(*X))
#define checkError(CC) res = (CC); if(res != VK_SUCCESS){fname=#CC,line=__LINE__;goto err;}

VkResult VulkanApp::initInstance() {
    const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
	const VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = NULL
	};

	// initialize the VkApplicationInfo structure
	const VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = APP_SHORT_NAME,
		.applicationVersion = 1,
		.pEngineName = "DOE",
		.engineVersion = 1,
		.apiVersion = VK_API_VERSION_1_0,
	};
	
	char const* const exts[] = { 
		"VK_KHR_get_physical_device_properties2", 
        VK_KHR_SURFACE_EXTENSION_NAME,
	#if   defined(_GLFW_WIN32)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#elif defined(_GLFW_X11)
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
	#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME 
    };
	char const* const lays[] = { "VK_LAYER_KHRONOS_validation" };

	// initialize the VkInstanceCreateInfo structure
	const VkInstanceCreateInfo inst_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &MessengerCreateInfo,
		.flags = 0,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = arrayCount(lays),
		.ppEnabledLayerNames = lays,
		.enabledExtensionCount = arrayCount(exts),
		.ppEnabledExtensionNames = exts,
	};

	checkError(vkCreateInstance(&inst_info, NULL, &this->instance));
    return res;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
VkResult VulkanApp::initSurface() {
	const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = GetModuleHandle(0);//hInstance;
	createInfo.hwnd = glfwGetWin32Window(window);
	checkError(vkCreateWin32SurfaceKHR(this->instance, &createInfo, NULL, &surface));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	VkXlibSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.dpy = glfwGetX11Display(window);//hInstance;
	createInfo.window = glfwGetX11Window(window);
	checkError(vkCreateWin32SurfaceKHR(this->instance, &createInfo, NULL, &surface));
#else
	#error unexpected device
#endif
	return res;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
VkResult VulkanApp::TestPhysicalDeviceInfo(VkPhysicalDevice pd) {
    const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
	{
		uint32_t surface_format_count = 0;
		checkError(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, this->surface, &surface_format_count, NULL));
		if (surface_format_count == 0)
			return VK_ERROR_UNKNOWN;
		std::vector<VkSurfaceFormatKHR> surface_formats{ surface_format_count };
		checkError(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, this->surface, &surface_format_count, surface_formats.data()));
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
	return VK_ERROR_UNKNOWN;
success:
	{
		uint32_t presentMode_count;
		checkError(vkGetPhysicalDeviceSurfacePresentModesKHR(pd, this->surface, &presentMode_count, NULL));
		std::vector<VkPresentModeKHR> presentModes{ presentMode_count };
		checkError(vkGetPhysicalDeviceSurfacePresentModesKHR(pd, this->surface, &presentMode_count, presentModes.data()));
	}
	return VK_SUCCESS;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
VkResult VulkanApp::autoSelectDevice()
{
	const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
	uint32_t gpu_count = 0;
	checkError(vkEnumeratePhysicalDevices(this->instance, &gpu_count, NULL));	
	if(gpu_count == 0)
		return VK_ERROR_UNKNOWN;
	else {
		VkPhysicalDevice physical_devices[gpu_count];
		checkError(vkEnumeratePhysicalDevices(this->instance, &gpu_count, physical_devices));
		uint32_t graphic_queue_index = 0;
		uint32_t transfer_queue_index = 0;

		for (VkPhysicalDevice pd : physical_devices)
		{
			VkPhysicalDeviceProperties physical_device_prop = {};
			vkGetPhysicalDeviceProperties(pd, &physical_device_prop);
			printf("Device: %s %u\n", physical_device_prop.deviceName, physical_device_prop.deviceID);

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, NULL);
			if (queue_family_count == 0) continue;
			VkQueueFamilyProperties queue_family_prop[queue_family_count];

			vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, queue_family_prop);
			if (TestPhysicalDeviceInfo(pd)) continue;
			
			for (unsigned int i = 0; i < queue_family_count; i++)
			{
				// some devices are made for computation and does not support graphical commands
				if (!(queue_family_prop[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
				if(queue_family_prop[i].queueCount < 1) continue;
				graphic_queue_index = i;
				goto search_for_transform_queue;
			}
			// gpu was not suitable
			continue;
		search_for_transform_queue:

			// search for transfer only queue
			for (unsigned int i = 0; i < queue_family_count; i++)
				if (i != graphic_queue_index && (queue_family_prop[i].queueFlags & VK_QUEUE_TRANSFER_BIT)){
					transfer_queue_index = i;
					goto search_for_present_queue;
				}
			if (queue_family_prop[graphic_queue_index].queueFlags & VK_QUEUE_TRANSFER_BIT)			
				if(queue_family_prop[graphic_queue_index].queueCount > 2){
					transfer_queue_index = graphic_queue_index;
					goto search_for_present_queue;
				}
			// gpu was not suitable
			continue;

		search_for_present_queue:
			{
				VkBool32 supported = false;
				// A Device may not be plugged into a monitor or not have any graphcal output
				// which could make direct interactions with displayable images difficult or impossible.
				// ie: it can render but result image must be copied to another Device that can render to display.
				checkError(vkGetPhysicalDeviceSurfaceSupportKHR(pd, graphic_queue_index, this->surface, &supported));
				if (supported)
					goto exit_loop;
				// gpu was not suitable
				continue;
			}
		exit_loop:
			this->pdevice = pd;
			this->graphic.index = graphic_queue_index;
			this->transfer.index = transfer_queue_index;
			return VK_SUCCESS;
		}
	}
	return VK_ERROR_UNKNOWN;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
VkResult VulkanApp::initDevice(){
	const char *fname;
	VkResult res = VK_ERROR_UNKNOWN;
    uint32_t line;

	float priority[1] = {1.0f};

	VkDeviceQueueCreateInfo queue_info[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = this->graphic.index,
			.queueCount = 1,
			.pQueuePriorities = priority,
		},{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = this->transfer.index,
			.queueCount = 1,
			.pQueuePriorities = priority,
		}
	};
	char const* const exts[] = { "VK_KHR_push_descriptor", VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.queueCreateInfoCount = arrayCount(queue_info),
		.pQueueCreateInfos = queue_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = arrayCount(exts),
		.ppEnabledExtensionNames = exts,
		.pEnabledFeatures = NULL,
	};
	if(this->transfer.index == this->graphic.index){
		queue_info[0].queueCount = 2;
		device_info.queueCreateInfoCount = 1;
	}

	checkError(vkCreateDevice(this->pdevice, &device_info, NULL, &this->ldevice));
	return VK_SUCCESS;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
VkResult VulkanApp::initQueue(){
	const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
	
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};

	vkGetDeviceQueue(
		this->ldevice,
		this->graphic.index,
		0,
		&this->graphic.queue);
	vkGetDeviceQueue(
		this->ldevice,
		this->transfer.index,
		this->transfer.index == this->graphic.index ? 1 : 0,
		&this->transfer.queue);

	checkError(vkCreateFence(this->ldevice, &fence_info, 0, &this->graphic.fence));
	checkError(vkCreateFence(this->ldevice, &fence_info, 0, &this->transfer.fence));
	return VK_SUCCESS;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}

VkResult VulkanApp::test(){
	const char *fname;
	VkResult res = VK_SUCCESS;
    uint32_t line;
	//checkError();
	return VK_SUCCESS;
err:
    printf("%s:%d %s:%i\n",__FILE__,line,fname,res);
    return res;
}
const char* VulkanApp::GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity)
{
	switch (Severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:return "Verbose";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:   return "Info";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:return "Warning";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:  return "Error";
	default: return "NO SUCH SEVERITY!";
	}
}
const char* VulkanApp::GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type)
{
	switch (Type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device address binding";
	default: return "NO SUCH TYPE!";
	}
}
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	printf("Debug callback: \n%s\n", pCallbackData->pMessage);
	printf("  Severity %s\n", GetDebugSeverityStr(Severity));
	printf("  Type %s\n", GetDebugType(Type));
	printf("  Objects ");
	for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
		printf("%llx ", pCallbackData->pObjects[i].objectHandle);
	}
	printf("\n");
	return VK_FALSE;  // The calling function should not be aborted
}
