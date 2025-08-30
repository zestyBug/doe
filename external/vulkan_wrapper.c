// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// This file is generated.

#ifdef VK_USE_PLATFORM_WIN32_KHR
    #include <windows.h>
    #define VKLibraryName "vulkan-1.dll"
    #define VKLibraryHandle HMODULE
    #define VKLibraryOpen(N) LoadLibraryA(N)
    #define VKLibraryError() "ERROR-UNKNOWN"
    #define VKLibraryClose(HANDLE) !FreeLibrary(HANDLE)
    #ifdef _MSC_VER
        #define PROC_LEGACY(V) res|=(V=(decltype(V))GetProcAddress(libvulkan,#V))==NULL
    #else
        #define PROC_LEGACY(V) res|=(V=(__typeof__(V))GetProcAddress(libvulkan,#V))==NULL
    #endif
#else
    #include <dlfcn.h>
    #if defined(__OpenBSD__) || defined(__NetBSD__)
        #define VKLibraryName "libvulkan.so"
    #else
        #define VKLibraryName "libvulkan.so.1"
    #endif
    #define VKLibraryHandle void*
    #define VKLibraryOpen(N) dlopen(N, RTLD_NOW | RTLD_LOCAL)
    #define VKLibraryError() dlerror()
    #define VKLibraryClose(HANDLE) dlclose(HANDLE)
    // GETPROCADDR
    #define PROC_LEGACY(V) res|=(V=(__typeof__(V))dlsym(libvulkan,#V))==NULL
#endif


#include "vulkan_wrapper.h"




#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static VKLibraryHandle libvulkan=0;

int VKDestroy(void) {
    return VKLibraryClose(libvulkan);
}
int VKInitialized(void){
    return libvulkan != 0;
}
int VKInitialize(void) {
    int res=0;

    // makes an executable object file specified by file available to the calling program.
    libvulkan = VKLibraryOpen(VKLibraryName);
    if (!libvulkan)
    {
        //printf("unable to load %s\n",VKLibraryName);
        return 1;
    }

    // Minimum Vulkan supported functions.
    // obtain the address of a symbol defined within an object.

    PROC_LEGACY(vkCreateInstance);
    PROC_LEGACY(vkDestroyInstance);
    PROC_LEGACY(vkGetInstanceProcAddr);
    PROC_LEGACY(vkEnumerateInstanceExtensionProperties);
    PROC_LEGACY(vkEnumerateInstanceLayerProperties);
    if(res)
        return 2;
    return 0;
}


#undef PROC_LEGACY
#if defined(_MSC_VER)
    #define PROC_LEGACY(V) res|=(V=(decltype(V))vkGetInstanceProcAddr(pd,#V))==NULL
#else
    #define PROC_LEGACY(V) res|=(V=(__typeof__(V))vkGetInstanceProcAddr(pd,#V))==NULL
#endif

int VKInitializeWInstance(VkInstance pd) {
    int res=0;    
    PROC_LEGACY(vkEnumeratePhysicalDevices);
    
    PROC_LEGACY(vkGetPhysicalDeviceFeatures);
    PROC_LEGACY(vkGetPhysicalDeviceFormatProperties);
    PROC_LEGACY(vkGetPhysicalDeviceImageFormatProperties);
    PROC_LEGACY(vkGetPhysicalDeviceProperties);
    PROC_LEGACY(vkGetPhysicalDeviceQueueFamilyProperties);
    PROC_LEGACY(vkGetPhysicalDeviceMemoryProperties);
    PROC_LEGACY(vkGetPhysicalDeviceSparseImageFormatProperties);

    //PROC_LEGACY(vkGetPhysicalDeviceDisplayPropertiesKHR);
    //PROC_LEGACY(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
    //PROC_LEGACY(vkGetDisplayPlaneSupportedDisplaysKHR);
    //PROC_LEGACY(vkGetDisplayModePropertiesKHR);
    //PROC_LEGACY(vkCreateDisplayModeKHR);
    //PROC_LEGACY(vkGetDisplayPlaneCapabilitiesKHR);
    //PROC_LEGACY(vkCreateDisplayPlaneSurfaceKHR);

    PROC_LEGACY(vkGetDeviceProcAddr);
    PROC_LEGACY(vkCreateDevice);
    PROC_LEGACY(vkDestroyDevice);
    PROC_LEGACY(vkEnumerateDeviceExtensionProperties);
    PROC_LEGACY(vkEnumerateDeviceLayerProperties);

    PROC_LEGACY(vkGetPhysicalDeviceSurfaceSupportKHR);
    PROC_LEGACY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    PROC_LEGACY(vkGetPhysicalDeviceSurfaceFormatsKHR);
    PROC_LEGACY(vkGetPhysicalDeviceSurfacePresentModesKHR);
    
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PROC_LEGACY(vkCreateXlibSurfaceKHR);
    PROC_LEGACY(vkGetPhysicalDeviceXlibPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    PROC_LEGACY(vkCreateXcbSurfaceKHR);
    PROC_LEGACY(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PROC_LEGACY(vkCreateWaylandSurfaceKHR);
    PROC_LEGACY(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_MIR_KHR
    PROC_LEGACY(vkCreateMirSurfaceKHR);
    PROC_LEGACY(vkGetPhysicalDeviceMirPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PROC_LEGACY(vkCreateAndroidSurfaceKHR);
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    PROC_LEGACY(vkCreateWin32SurfaceKHR);
    PROC_LEGACY(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif
#ifdef USE_DEBUG_EXTENTIONS
    PROC_LEGACY(vkCreateDebugReportCallbackEXT);
    PROC_LEGACY(vkDestroyDebugReportCallbackEXT);
    PROC_LEGACY(vkDebugReportMessageEXT);
#endif
    PROC_LEGACY(vkDestroySurfaceKHR);
    if(res)
        return 2;
    return 0;
}


#undef PROC_LEGACY
#if defined(_MSC_VER)
        #define PROC_LEGACY(V) res|=(V=(decltype(V))vkGetDeviceProcAddr(dv,#V))==NULL
#else
    #define PROC_LEGACY(V) res|=(V=(__typeof__(V))vkGetDeviceProcAddr(dv,#V))==NULL
#endif

int VKInitializeWDevice(VkDevice dv) {
    int res=0;
    PROC_LEGACY(vkGetDeviceQueue);
    PROC_LEGACY(vkQueueSubmit);
    PROC_LEGACY(vkQueueWaitIdle);
    PROC_LEGACY(vkDeviceWaitIdle);
    PROC_LEGACY(vkAllocateMemory);
    PROC_LEGACY(vkFreeMemory);
    PROC_LEGACY(vkMapMemory);
    PROC_LEGACY(vkUnmapMemory);
    PROC_LEGACY(vkFlushMappedMemoryRanges);
    PROC_LEGACY(vkInvalidateMappedMemoryRanges);
    PROC_LEGACY(vkGetDeviceMemoryCommitment);
    PROC_LEGACY(vkBindBufferMemory);
    PROC_LEGACY(vkBindImageMemory);
    PROC_LEGACY(vkGetBufferMemoryRequirements);
    PROC_LEGACY(vkGetImageMemoryRequirements);
    PROC_LEGACY(vkGetImageSparseMemoryRequirements);

    PROC_LEGACY(vkQueueBindSparse);
    PROC_LEGACY(vkCreateFence);
    PROC_LEGACY(vkDestroyFence);
    PROC_LEGACY(vkResetFences);
    PROC_LEGACY(vkGetFenceStatus);
    PROC_LEGACY(vkWaitForFences);
    PROC_LEGACY(vkCreateSemaphore);
    PROC_LEGACY(vkDestroySemaphore);
    PROC_LEGACY(vkCreateEvent);
    PROC_LEGACY(vkDestroyEvent);
    PROC_LEGACY(vkGetEventStatus);
    PROC_LEGACY(vkSetEvent);
    PROC_LEGACY(vkResetEvent);
    PROC_LEGACY(vkCreateQueryPool);
    PROC_LEGACY(vkDestroyQueryPool);
    PROC_LEGACY(vkGetQueryPoolResults);
    PROC_LEGACY(vkCreateBuffer);
    PROC_LEGACY(vkDestroyBuffer);
    PROC_LEGACY(vkCreateBufferView);
    PROC_LEGACY(vkDestroyBufferView);
    PROC_LEGACY(vkCreateImage);
    PROC_LEGACY(vkDestroyImage);
    PROC_LEGACY(vkGetImageSubresourceLayout);
    PROC_LEGACY(vkCreateImageView);
    PROC_LEGACY(vkDestroyImageView);
    PROC_LEGACY(vkCreateShaderModule);
    PROC_LEGACY(vkDestroyShaderModule);
    PROC_LEGACY(vkCreatePipelineCache);
    PROC_LEGACY(vkDestroyPipelineCache);
    PROC_LEGACY(vkGetPipelineCacheData);
    PROC_LEGACY(vkMergePipelineCaches);
    PROC_LEGACY(vkCreateGraphicsPipelines);
    PROC_LEGACY(vkCreateComputePipelines);
    PROC_LEGACY(vkDestroyPipeline);
    PROC_LEGACY(vkCreatePipelineLayout);
    PROC_LEGACY(vkDestroyPipelineLayout);
    PROC_LEGACY(vkCreateSampler);
    PROC_LEGACY(vkDestroySampler);
    PROC_LEGACY(vkCreateDescriptorSetLayout);
    PROC_LEGACY(vkDestroyDescriptorSetLayout);
    PROC_LEGACY(vkCreateDescriptorPool);
    PROC_LEGACY(vkDestroyDescriptorPool);
    PROC_LEGACY(vkResetDescriptorPool);
    PROC_LEGACY(vkAllocateDescriptorSets);
    PROC_LEGACY(vkFreeDescriptorSets);
    PROC_LEGACY(vkUpdateDescriptorSets);
    PROC_LEGACY(vkCreateFramebuffer);
    PROC_LEGACY(vkDestroyFramebuffer);
    PROC_LEGACY(vkCreateRenderPass);
    PROC_LEGACY(vkDestroyRenderPass);
    PROC_LEGACY(vkGetRenderAreaGranularity);
    PROC_LEGACY(vkCreateCommandPool);
    PROC_LEGACY(vkDestroyCommandPool);
    PROC_LEGACY(vkResetCommandPool);
    PROC_LEGACY(vkAllocateCommandBuffers);
    PROC_LEGACY(vkFreeCommandBuffers);
    PROC_LEGACY(vkBeginCommandBuffer);
    PROC_LEGACY(vkEndCommandBuffer);
    PROC_LEGACY(vkResetCommandBuffer);
    PROC_LEGACY(vkCmdBindPipeline);
    PROC_LEGACY(vkCmdSetViewport);
    PROC_LEGACY(vkCmdSetScissor);
    PROC_LEGACY(vkCmdSetLineWidth);
    PROC_LEGACY(vkCmdSetDepthBias);
    PROC_LEGACY(vkCmdSetBlendConstants);
    PROC_LEGACY(vkCmdSetDepthBounds);
    PROC_LEGACY(vkCmdSetStencilCompareMask);
    PROC_LEGACY(vkCmdSetStencilWriteMask);
    PROC_LEGACY(vkCmdSetStencilReference);
    PROC_LEGACY(vkCmdBindDescriptorSets);
    PROC_LEGACY(vkCmdBindIndexBuffer);
    PROC_LEGACY(vkCmdBindVertexBuffers);
    PROC_LEGACY(vkCmdDraw);
    PROC_LEGACY(vkCmdDrawIndexed);
    PROC_LEGACY(vkCmdDrawIndirect);
    PROC_LEGACY(vkCmdDrawIndexedIndirect);
    PROC_LEGACY(vkCmdDispatch);
    PROC_LEGACY(vkCmdDispatchIndirect);
    PROC_LEGACY(vkCmdCopyBuffer);
    PROC_LEGACY(vkCmdCopyImage);
    PROC_LEGACY(vkCmdBlitImage);
    PROC_LEGACY(vkCmdCopyBufferToImage);
    PROC_LEGACY(vkCmdCopyImageToBuffer);
    PROC_LEGACY(vkCmdUpdateBuffer);
    PROC_LEGACY(vkCmdFillBuffer);
    PROC_LEGACY(vkCmdClearColorImage);
    PROC_LEGACY(vkCmdClearDepthStencilImage);
    PROC_LEGACY(vkCmdClearAttachments);
    PROC_LEGACY(vkCmdResolveImage);
    PROC_LEGACY(vkCmdSetEvent);
    PROC_LEGACY(vkCmdResetEvent);
    PROC_LEGACY(vkCmdWaitEvents);
    PROC_LEGACY(vkCmdPipelineBarrier);
    PROC_LEGACY(vkCmdBeginQuery);
    PROC_LEGACY(vkCmdEndQuery);
    PROC_LEGACY(vkCmdResetQueryPool);
    PROC_LEGACY(vkCmdWriteTimestamp);
    PROC_LEGACY(vkCmdCopyQueryPoolResults);
    PROC_LEGACY(vkCmdPushConstants);
    PROC_LEGACY(vkCmdBeginRenderPass);
    PROC_LEGACY(vkCmdNextSubpass);
    PROC_LEGACY(vkCmdEndRenderPass);
    PROC_LEGACY(vkCmdExecuteCommands);

    PROC_LEGACY(vkCreateSwapchainKHR);
    PROC_LEGACY(vkDestroySwapchainKHR);
    PROC_LEGACY(vkGetSwapchainImagesKHR);
    PROC_LEGACY(vkAcquireNextImageKHR);
    PROC_LEGACY(vkQueuePresentKHR);

    //PROC_LEGACY(vkCreateSharedSwapchainsKHR);

    if(res)
        return 2;
    return 0;
}


#if defined(_MSC_VER)
    #define PROC_EXT(P,V) res|=(P=(decltype(P))vkGetDeviceProcAddr(dv,#V))==NULL
#else
    #define PROC_EXT(P,V) res|=(P=(__typeof__(P))vkGetDeviceProcAddr(dv,#V))==NULL
#endif

int VKInitializeExtension(VkDevice dv) {
    int res = 0;
    
    
    PROC_LEGACY(vkGetImageMemoryRequirements2);
    if(!vkGetImageMemoryRequirements2)
        PROC_EXT(vkGetImageMemoryRequirements2, vkGetImageMemoryRequirements2KHR);

    PROC_LEGACY(vkGetBufferMemoryRequirements2);
    if(!vkGetBufferMemoryRequirements2)
        PROC_EXT(vkGetBufferMemoryRequirements2, vkGetBufferMemoryRequirements2KHR);

    PROC_LEGACY(vkGetImageSparseMemoryRequirements2);
    if(!vkGetImageSparseMemoryRequirements2)
        PROC_EXT(vkGetImageSparseMemoryRequirements2, vkGetImageSparseMemoryRequirements2KHR);

    PROC_LEGACY(vkBindBufferMemory2);
    if(!vkBindBufferMemory2)
        PROC_EXT(vkBindBufferMemory2, vkBindBufferMemory2KHR);

    PROC_LEGACY(vkBindImageMemory2);
    if(!vkBindImageMemory2)
        PROC_EXT(vkBindImageMemory2, vkBindImageMemory2KHR);

    PROC_LEGACY(vkCmdPushDescriptorSetKHR);
    if (res)
        return 2;
    return 0;
}


// No Vulkan support, do not set function addresses
PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkDestroyDevice vkDestroyDevice;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueueWaitIdle vkQueueWaitIdle;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkMapMemory vkMapMemory;
PFN_vkUnmapMemory vkUnmapMemory;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
PFN_vkBindBufferMemory vkBindBufferMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkQueueBindSparse vkQueueBindSparse;
PFN_vkCreateFence vkCreateFence;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkResetFences vkResetFences;
PFN_vkGetFenceStatus vkGetFenceStatus;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkCreateEvent vkCreateEvent;
PFN_vkDestroyEvent vkDestroyEvent;
PFN_vkGetEventStatus vkGetEventStatus;
PFN_vkSetEvent vkSetEvent;
PFN_vkResetEvent vkResetEvent;
PFN_vkCreateQueryPool vkCreateQueryPool;
PFN_vkDestroyQueryPool vkDestroyQueryPool;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
PFN_vkCreateBuffer vkCreateBuffer;
PFN_vkDestroyBuffer vkDestroyBuffer;
PFN_vkCreateBufferView vkCreateBufferView;
PFN_vkDestroyBufferView vkDestroyBufferView;
PFN_vkCreateImage vkCreateImage;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkCreatePipelineCache vkCreatePipelineCache;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
PFN_vkMergePipelineCaches vkMergePipelineCaches;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateComputePipelines vkCreateComputePipelines;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkCreateSampler vkCreateSampler;
PFN_vkDestroySampler vkDestroySampler;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
PFN_vkResetDescriptorPool vkResetDescriptorPool;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkResetCommandPool vkResetCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
PFN_vkCmdDispatch vkCmdDispatch;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
PFN_vkCmdCopyImage vkCmdCopyImage;
PFN_vkCmdBlitImage vkCmdBlitImage;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
PFN_vkCmdFillBuffer vkCmdFillBuffer;
PFN_vkCmdClearColorImage vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
PFN_vkCmdClearAttachments vkCmdClearAttachments;
PFN_vkCmdResolveImage vkCmdResolveImage;
PFN_vkCmdSetEvent vkCmdSetEvent;
PFN_vkCmdResetEvent vkCmdResetEvent;
PFN_vkCmdWaitEvents vkCmdWaitEvents;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
PFN_vkCmdBeginQuery vkCmdBeginQuery;
PFN_vkCmdEndQuery vkCmdEndQuery;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
PFN_vkCmdPushConstants vkCmdPushConstants;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdNextSubpass vkCmdNextSubpass;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;

#ifdef VK_USE_PLATFORM_XLIB_KHR
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_MIR_KHR
PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR;
PFN_vkGetPhysicalDeviceMirPresentationSupportKHR vkGetPhysicalDeviceMirPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif
PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;


PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkGetDeviceBufferMemoryRequirements vkGetDeviceBufferMemoryRequirements;
PFN_vkGetDeviceImageMemoryRequirements vkGetDeviceImageMemoryRequirements;

PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2;
PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2;
PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2;
PFN_vkBindBufferMemory2 vkBindBufferMemory2;
PFN_vkBindImageMemory2 vkBindImageMemory2;

PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;


#ifdef __cplusplus
}
#endif // __cplusplus
