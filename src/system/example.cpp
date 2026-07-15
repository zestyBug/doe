#include "example.hpp"
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"


ECS::SystemRegister<ExampleSystem> _{};
static std::atomic<bool> running;
void ExampleSystem::OnFixedUpdate(ECS::DOE&){
    counter++;
    if(counter == 100)
        ECS::JobsUtility::signalQuit();
}
void ExampleSystem::OnUpdate(ECS::DOE&){
    uv_sem_post(&this->glock);
}
void ExampleSystem::gFunc(void *arg)
{
    VkResult res;
    uint32_t img_index = 0;
    uint32_t img_index_cache;
    bool recreate = true;
    ECS::VKContext &vk = ((ExampleSystem*)arg)->vk;
    //VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 0,
        //.pCommandBuffers = *commands,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vk.queueSemaphore,
	};
    VkPresentInfoKHR pinfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk.queueSemaphore,
		.swapchainCount = 1,
	};
    goto begining;
    while(running)
    {
    again:
        if(recreate){
            vk.resetSwapchain();
            recreate = false;
        }

        if (vk.surfaceExtend.width == 0 || vk.surfaceExtend.height == 0 || vk.swapchain == VK_NULL_HANDLE){
            recreate = true;
            goto again;
        }

        img_index_cache = img_index;
        res = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageSemaphores[0], 0, &img_index);
        if (res == VK_ERROR_OUT_OF_DATE_KHR){
            recreate = true;
            goto again;
        } else if (res != VK_SUBOPTIMAL_KHR && res != VK_SUCCESS)
            throw ECS::VulkanException(res, "vkAcquireNextImageKHR");

        res = vkResetFences(vk.device, 1, &vk.queueFence);
        if (res) throw ECS::VulkanException(res, "vkResetFences");

        info.pWaitSemaphores = vk.imageSemaphores;
        res = vkQueueSubmit(vk.queue, 1, &info, vk.queueFence);
        if (res) throw ECS::VulkanException(res, "vkQueueSubmit");

        pinfo.pSwapchains = &vk.swapchain;
		pinfo.pImageIndices = &img_index;
	    res = vkQueuePresentKHR(vk.queue, &pinfo);
        /* res == VK_ERROR_SURFACE_LOST_KHR || */ 
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR){
            recreate = true;
        } else if (res)
            throw ECS::VulkanException(res, "vkQueuePresentKHR");
        ECS::JobsUtility::signalRender();
        res = vkWaitForFences(vk.device, 1, &vk.queueFence, VK_TRUE, UINT64_MAX);
        if (res)
            throw ECS::VulkanException(res, "vkWaitForFences");
    begining:
        uv_sem_wait(&((ExampleSystem*)arg)->glock);
    }
}
ExampleSystem::ExampleSystem(ECS::DOE &e):ISystem{e}{
    vk.initialize();
    vk.createSurface();
    vk.selectDevice();
    vk.initRender();
    running = true;
    uv_sem_init(&this->glock,1);
    uv_thread_create(&this->gthread, &gFunc, this);
}
void ExampleSystem::OnDestroy(ECS::DOE&){
    uv_sem_post(&this->glock);
}
ExampleSystem::~ExampleSystem(){
    running = false;
    uv_sem_post(&this->glock);
    uv_thread_join(&this->gthread);
    uv_sem_destroy(&this->glock);
}
struct example_system {
    ECS::Version systemVersion = 0;
};