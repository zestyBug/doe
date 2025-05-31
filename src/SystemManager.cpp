#include "ECS/SystemManager.hpp"
using namespace ECS;



void DependencyManager::dummyExecute(){
    for(auto&j:registeredJobs){
        printf("Job: \"%s\", Dependencies: [",j.name);
        for(JobHandle d:j.deps){
            printf("%i,",d);
        }
        printf("]\n");
        j.jobFunc(j.context,span<void*>{},0);
    }
}
