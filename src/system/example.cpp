#include "example.hpp"
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
#include "imgui.h"

ECS::SystemRegister<ExampleSystem> _{};
static bool isOpen = true;

struct test_2 : ECS::IComponentData, ECS::IManagedComponentData
{
    test_2(){
        value = 69;
    }
    int value;
};
DEF_TYPE(test_2)

void ExampleSystem::OnFixedUpdate(ECS::DOE&){
    counter++;
    if(counter == 1000000)
        ECS::JobsUtility::signalQuit();
}
void ExampleSystem::OnUpdate(ECS::DOE&){
    ImGuiIO& io=ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    if (ImGui::Begin("winmain", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        const float label_width_base = ImGui::GetFontSize() * 12;               // Some amount of width for label, based on font size.
        const float label_width_max = ImGui::GetContentRegionAvail().x * 0.40f; // ...but always leave some room for framed widgets.
        const float label_width = std::min(label_width_base, label_width_max);
        ImGui::PushItemWidth(-label_width); 
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                ImGui::MenuItem("Main menu bar", NULL, nullptr);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::Text("dear imgui says hello!");
        ECS::sharedEngine->eqm.iterate(ECS::sharedEngine->ecs, this->query, [](span<const void*> offset){
            ImGui::Text("X: %i\n",((test_2*)offset[0])->value);
        });
    }
    ImGui::End();
}
ExampleSystem::ExampleSystem(ECS::DOE &e):ISystem{e}{
    {
        ECS::EntityQueryBuilder builder;
        builder.withAllRW(ECS::getTypeID<test_2>());
        this->query = ECS::sharedEngine->eqm.createEntityQuery(builder,ECS::sharedEngine->ecs);
        ECS::Archetype *arch = ECS::sharedEngine->ecs.getOrCreateArchetype(ECS::componentTypes<ECS::Entity,test_2>());
        ECS::Entity ents[10];
        ECS::sharedEngine->ecs.createEntities(*arch,{ents,10});
        //ECS::sharedEngine->ecs.getArchetype
    }
}
void ExampleSystem::OnDestroy(ECS::DOE&){
}
ExampleSystem::~ExampleSystem(){
}