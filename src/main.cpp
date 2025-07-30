#include "ECS/SystemManager.hpp"
#include "system/example.hpp"
#include "VulkanApp.hpp"

int main(){
	VulkanApp app;
	if(VKInitialize()) return 1;
	if(app.initInstance()) return 1;
	if(VKInitializeWInstance(app.instance)) return 1;
    if (!glfwInit()) return 1;
	app.window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
	if (!app.window) return 1;
	if(app.initSurface()) return 1;
	if(app.autoSelectDevice()) return 1;
	if(app.initDevice()) return 1;
	if(VKInitializeWDevice(app.ldevice)) return 1;
	if(app.initQueue()) return 1;
    glfwSetWindowSizeLimits(app.window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
    {
        ECS::SystemState engine;
        engine.context = &app;
		{
			ECS::example_system *ptr=allocator<ECS::example_system>().allocate(1);
			allocator<ECS::example_system>().construct(ptr);
			engine.manager.systems.emplace(engine.manager.systems.create(),ptr);
		}

        while (!glfwWindowShouldClose(app.window))
        {
            //glfwPollEvents();
            //glfwWaitEventsTimeout(0.7);
            glfwWaitEvents();
            auto view = engine.manager.systems.view();
            for (const auto value:view)
                if(ECS::System *s = view.get(value).get();s != nullptr)
                    s->onUpdate(engine);
        }
    }
    glfwDestroyWindow(app.window);
    glfwTerminate();
    return 0;
}