#include "GraphicSystem.hpp"
#include "Window.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
align_ptr<ECS::GraphicSystem> ECS::graphics;

void ECS::GraphicSystem::gFunc(void *arg)
{
    GraphicSystem &thiz = *(GraphicSystem*)arg;
    VKContext     &vk   = thiz.vk;
    uint32_t counter = 0;
    uint32_t recreate = 1;
    uint32_t swapchainIndex;
    uint32_t frameInflightIndex = 0;
    VkResult res;
    VKContext::FrameInFlight FIFB;
    VKContext::FrameResource FRB;
    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    const VkClearValue clearValues[2] = {
        { .color = { { 0.1f, 0.2f, 0.3f, 1.0f } } },
        { .depthStencil = { 1.0f, 0 } }
    };
    vkResetCommandPool(vk.device, vk.cpool, 0);
    FIFB = vk.frameInFlights[frameInflightIndex];
    JobsUtility::signalRender();

    while(true)
    {
        /**
         * 1- wait for signal that render commands are ready
         * // recreate the swapchain(+surface) if required. go to 5 on failure.
         * 2- culling (optional)
         * 3- sorting
         * 4- record vulkan commands
         * 5- signal for new render command buffer
         * 6- reset fences and submit to the queue
         * // if failed, go to 1
         * object managment
         * obtain a new frame (wait if no frame is available)
         * wait fo the queue fence of that image index
         */
        uv_sem_wait(&thiz.glock);
        if(!sharedWindow.running)
            break;

        if(counter>100){
            counter = 0;
            recreate = 1;
        }
        if(recreate){
            vkDeviceWaitIdle(thiz.vk.device);
            vk.resetSwapchain(recreate == 2);
            if (unlikely(vk.surfaceExtend.width == 0 || vk.surfaceExtend.height == 0 || vk.swapchain == VK_NULL_HANDLE)){
                recreate = 2;
                continue;
            }
            res = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, FIFB.imageSemaphore, 0, &swapchainIndex);
            if(unlikely(res != VK_SUCCESS)) {
                if (res == VK_ERROR_OUT_OF_DATE_KHR){
                    recreate = 1;
                    continue;
                } else if (res == VK_ERROR_SURFACE_LOST_KHR){
                    recreate = 2;
                    continue;
                } else if (res == VK_SUBOPTIMAL_KHR)
                    counter++;
                else
                    throw ECS::VulkanException(res, "vkAcquireNextImageKHR");
            }
            recreate=false;
            counter = 0;
            FRB = vk.frameResources[swapchainIndex];
        }


        res = vkBeginCommandBuffer(FIFB.commandBuffer, &begin_info);
        if (res)
            throw ECS::VulkanException(res, "vkBeginCommandBuffer");
        ImGui::Render();
        {
            VkRenderPassBeginInfo renderPassInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = vk.renderpass,
                .framebuffer = FRB.frambuffer,
                .renderArea = {
                    .offset = {0, 0},
                    .extent = vk.surfaceExtend
                },
                .clearValueCount = 1,
                .pClearValues = clearValues
            };
            vkCmdBeginRenderPass(FIFB.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float)vk.surfaceExtend.width,
                .height = (float)vk.surfaceExtend.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            };
            vkCmdSetViewport(FIFB.commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = vk.surfaceExtend
            };
            vkCmdSetScissor(FIFB.commandBuffer,0,1,&scissor);
        }
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),FIFB.commandBuffer);
        vkCmdEndRenderPass(FIFB.commandBuffer);
        vkEndCommandBuffer(FIFB.commandBuffer);

        JobsUtility::signalRender();

        {
            res = vkResetFences(vk.device, 1, &FIFB.queueFence);
            if (res)
                throw ECS::VulkanException(res, "vkResetFences");

            const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &FIFB.imageSemaphore,
                .pWaitDstStageMask = &wait_stage,
                .commandBufferCount = 1,
                .pCommandBuffers = &FIFB.commandBuffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &FIFB.queueSemaphore,
            };
            res = vkQueueSubmit(vk.queue, 1, &info, FIFB.queueFence);
            if (res)
                throw ECS::VulkanException(res, "vkQueueSubmit");

            VkPresentInfoKHR pinfo {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &FIFB.queueSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &vk.swapchain,
                .pImageIndices = &swapchainIndex,
            };
            res = vkQueuePresentKHR(vk.queue, &pinfo);
            if(unlikely(res != VK_SUCCESS))
            {
                if (res == VK_ERROR_OUT_OF_DATE_KHR){
                    recreate = 1;
                    continue;
                }else if(res == VK_ERROR_SURFACE_LOST_KHR){
                    recreate = 2;
                    continue;
                }else if (res == VK_SUBOPTIMAL_KHR)
                    counter++;
                else
                    throw ECS::VulkanException(res, "vkQueuePresentKHR");
            }
        }

        frameInflightIndex = (frameInflightIndex+1)%Constants::FrameInFlightCount;
        FIFB = vk.frameInFlights[frameInflightIndex];
        res = vkWaitForFences(vk.device, 1, &FIFB.queueFence, VK_TRUE, UINT64_MAX);
        if (res)
            throw ECS::VulkanException(res, "vkWaitForFences");
        res = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, FIFB.imageSemaphore, 0, &swapchainIndex);
        if(unlikely(res != VK_SUCCESS))
        {
            if (res == VK_ERROR_OUT_OF_DATE_KHR){
                recreate = 1;
                continue;
            } else if (res == VK_ERROR_SURFACE_LOST_KHR){
                recreate = 2;
                continue;
            } else if (res == VK_SUBOPTIMAL_KHR)
                counter++;
            else
                throw ECS::VulkanException(res, "vkAcquireNextImageKHR");
        }
        FRB = vk.frameResources[swapchainIndex];
    }
}
void ECS::GraphicSystem::contextInit() {
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
    uv_sem_init(&this->glock,0);
    uv_thread_create(&this->gthread, &gFunc, this);
}
void ECS::GraphicSystem::contextDestroy(){
    uv_sem_post(&this->glock);
    uv_thread_join(&this->gthread);
    uv_sem_destroy(&this->glock);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}
void ECS::GraphicSystem::beginFrame(){
    ImGuiIO& io=ImGui::GetIO();
    io.DeltaTime = sharedEngine->updateDelta;
    io.DisplaySize = ImVec2((float)vk.surfaceExtend.width, (float)vk.surfaceExtend.height);
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}
void ECS::GraphicSystem::endFrame(){
    uv_sem_post(&this->glock);
}