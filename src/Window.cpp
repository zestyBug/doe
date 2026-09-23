#include "Window.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"

void ECS::Window::gFunc(void *arg)
{
    VkResult res;
    uint32_t img_index_cache;
    bool recreate = true;
    ECS::VKContext &vk = ((Window*)arg)->vk;
    //VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &((Window*)arg)->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vk.queueSemaphore,
	};
    VkPresentInfoKHR pinfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk.queueSemaphore,
		.swapchainCount = 1,
	};
    while(((Window*)arg)->running.load())
    {
        if(recreate){
            vk.resetSwapchain();
            recreate = false;
        }

        if (vk.surfaceExtend.width == 0 || vk.surfaceExtend.height == 0 || vk.swapchain == VK_NULL_HANDLE){
            recreate = true;
            uv_sem_wait(&((Window*)arg)->glock);
            continue;
        }

        img_index_cache = ((Window*)arg)->img_index;
        res = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageSemaphores[img_index_cache], 0, (uint32_t *)&((Window*)arg)->img_index);
        if (res == VK_ERROR_OUT_OF_DATE_KHR){
            recreate = true;
            continue;
        } else if (res != VK_SUBOPTIMAL_KHR && res != VK_SUCCESS)
            throw ECS::VulkanException(res, "vkAcquireNextImageKHR");

        res = vkResetFences(vk.device, 1, &vk.queueFence);
        if (res) throw ECS::VulkanException(res, "vkResetFences");

        info.pWaitSemaphores = vk.imageSemaphores + img_index_cache;

        JobsUtility::signalRender();

        uv_sem_wait(&((Window*)arg)->glock);
        if(!((Window*)arg)->running.load())
            break;

        res = vkQueueSubmit(vk.queue, 1, &info, vk.queueFence);
        if (res) throw ECS::VulkanException(res, "vkQueueSubmit");

        pinfo.pSwapchains = &vk.swapchain;
		pinfo.pImageIndices = (uint32_t *)&((Window*)arg)->img_index;
	    res = vkQueuePresentKHR(vk.queue, &pinfo);
        /* res == VK_ERROR_SURFACE_LOST_KHR || */ 
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR){
            recreate = true;
        } else if (res)
            throw ECS::VulkanException(res, "vkQueuePresentKHR");
        res = vkWaitForFences(vk.device, 1, &vk.queueFence, VK_TRUE, UINT64_MAX);
        if (res)
            throw ECS::VulkanException(res, "vkWaitForFences");
    }
}
void ECS::Window::contextInit() {
    vk.initialize();
    vk.createSurface();
    vk.selectDevice();
    vk.initRender();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    {
        ImGuiIO& io=ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options
        io.BackendPlatformUserData = nullptr;
        io.BackendPlatformName = "imgui_impl_my";
    }
    {
        ImGui_ImplVulkan_InitInfo info {
            .ApiVersion = VK_API_VERSION_1_1,
            .Instance =       vk.instance,
            .PhysicalDevice = vk.pdevice,
            .Device =         vk.device,
            .QueueFamily =    vk.queueFamilyIndex,
            .Queue =          vk.queue,
            .DescriptorPool = vk.dpool,                  // See requirements in note above; ignored if using DescriptorPoolSize > 0
            .RenderPass =     vk.renderpass,                 // Ignored if using dynamic rendering
            .MinImageCount = 2,                          // >= 2
            .ImageCount = 2,                             // >= MinImageCount
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,        // 0 defaults to VK_SAMPLE_COUNT_1_BIT
            // (Optional)
            .PipelineCache = VK_NULL_HANDLE,
            .Subpass = 0,
            // (Optional) Set to create internal descriptor pool instead of using DescriptorPool
            .DescriptorPoolSize = 0,
            .UseDynamicRendering = 0,
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr,
            .MinAllocationSize = 1048576,
        };
        ImGui_ImplVulkan_Init(&info);
    }
    {
        VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vk.cpool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(vk.device, &allocInfo, &commandBuffer);
    }
    running = true;
    uv_sem_init(&this->glock,0);
    uv_thread_create(&this->gthread, &gFunc, this);
}
void ECS::Window::contextDestroy(){
    running.store(false);
    uv_sem_post(&this->glock);
    uv_thread_join(&this->gthread);
    uv_sem_destroy(&this->glock);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}
void ECS::Window::beginFrame(){
    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult res;
    ImGuiIO& io=ImGui::GetIO();
    io.DeltaTime = sharedEngine->updateDelta;
    io.DisplaySize = ImVec2(vk.surfaceExtend.width, vk.surfaceExtend.height);
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    vkResetCommandPool(vk.device, vk.cpool, 0);
    res = vkBeginCommandBuffer(commandBuffer, &begin_info);
    if (res)
        throw ECS::VulkanException(res, "vkBeginCommandBuffer");
}
void ECS::Window::endFrame(){
    ImGui::Render();
    {
        VkClearValue clearValues[2] = {
            { .color = { { 0.1f, 0.2f, 0.3f, 1.0f } } },
            { .depthStencil = { 1.0f, 0 } }
        };
        VkRenderPassBeginInfo renderPassInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = vk.renderpass,
            .framebuffer = vk.frambuffer[img_index],
            .renderArea = {
                .offset = {0, 0},
                .extent = vk.surfaceExtend
            },
            .clearValueCount = 1,
            .pClearValues = clearValues
        };
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)vk.surfaceExtend.width,
            .height = (float)vk.surfaceExtend.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = vk.surfaceExtend
        };
        vkCmdSetScissor(commandBuffer,0,1,&scissor);
    }
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),this->commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
    uv_sem_post(&this->glock);
}