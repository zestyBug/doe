#include "ECS/SystemManager.hpp"
void add_initial_systems(ECS::SystemState &);
int main(int argc, char *argv[])
{
    ECS::SystemState engine;
    add_initial_systems(engine);
    while (true)
    {
        auto view = engine.manager.systems.view();
        if(view.empty())
            return 0;
        for (const auto value:view)
            if(ECS::System *s = view.get(value).get();s != nullptr && s->onUpdate != nullptr)
                s->onUpdate(s,engine);
    }
}