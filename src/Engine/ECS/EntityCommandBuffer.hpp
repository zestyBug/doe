#if !defined(ENTITYCOMMANDBUFFER_HPP)
#define ENTITYCOMMANDBUFFER_HPP 1

#include "defs.hpp"
#include <vector>

namespace DOTS
{

class EntityCommand {
    struct command {
        DOTS::Entity entity;
        DOTS::compid_t comid;
        enum type : char {
            createEntity = 0,
            deleteEntity,
            removeComponent,
            setComponent,
        } command;
        void *data;
    };
    std::vector<command> buffer;
public:
    EntityCommand(){
        this->buffer.reserve(8);
    }
    void copyEntity(DOTS::Entity source,bool is_static){
        buffer.push_back(command{
            .entity = source,
            .comid = 0,
            .command = command::createEntity,
            .data = nullptr
        });
    }
    void Instantiate(const char* name){
        buffer.push_back(command{
            .entity = DOTS::null_entity,
            .comid = 0,
            .command = command::createEntity,
            .data = (void*)name,
        });
    }
    void createEntity(DOTS::compid_t coms,DOTS::Entity parent = DOTS::null_entity){
        buffer.push_back(command{
            .entity = parent,
            .comid = coms,
            .command = command::createEntity,
            .data = nullptr
        });
    }
    void destroyEntity(DOTS::Entity e){
        buffer.push_back(command{
            .entity = e,
            .comid = 0,
            .command = command::deleteEntity,
            .data = nullptr
        });
    }
    // create or modify value
    void setComponent(DOTS::Entity e, DOTS::compid_t index, void *value){
        buffer.push_back(command{
            .entity = e,
            .comid = index,
            .command = command::setComponent,
            .data = value,
        });
    }
    void removeComponent(DOTS::Entity e, DOTS::compid_t index){
        buffer.push_back(command{
            .entity = e,
            .comid = index,
            .command = command::removeComponent,
            .data = nullptr
        });
    }
};

} // namespace DOTS


#endif // ENTITYCOMMANDBUFFER_HPP
