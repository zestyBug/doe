#include "ECS/ComponentDependencyManager.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Base/Constants.hpp"
#include "ECS/EntityQueryManager.hpp"
using namespace ECS;

#if !ENABLE_SIMPLE_SYSTEM_DEPENDENCIES

ComponentDependencyManager::ComponentDependencyManager(){
    memset(typeArrayIndices.data(),0xFFFFFFFF,sizeof(typeArrayIndices));
}
uint32_t ComponentDependencyManager::getTypeArrayIndex(TypeID type)
{
    uint32_t arrayIndex = typeArrayIndices[type.index()];
    if (arrayIndex != NullTypeIndex)
        return arrayIndex;
    if(type.isZeroSized())
        throw std::runtime_error("getTypeArrayIndex(): dependancy on zero sized component is not allowed");
    arrayIndex = dependencyHandlesCount++;
    typeArrayIndices[type.index()] = (uint16_t)arrayIndex;
    dependencyHandles[arrayIndex].type = type;
    dependencyHandles[arrayIndex].numReadFences = 0;
    dependencyHandles[arrayIndex].writeFence = JobHandle();

    return arrayIndex;
}
JobHandle ComponentDependencyManager::getDependency(const EntityQueryData &query)
{
    const uint32_t counter = query.firstNoneIndex;
    if(counter >= Constants::MaximumQueryTypesCount)
        throw std::invalid_argument("getDependency(): too big of a query, stack may overflow");
    const EntityQueryData::TypeQuery *queries = query.queries.get();
    const EntityQueryData::TypeQuery *queries_end = queries + counter;
    if(queries == queries_end)
        return JobHandle();
    // wish we dont stack overflow
    JobHandle allHandles[counter * MaximumReadJobHandle];
    uint32_t allHandleCount = 0;

    while(queries != queries_end){
        if(queries->flags & EntityQueryData::TypeQuery::WriteFlag) {
            uint32_t typeArrayIndex = typeArrayIndices[queries->type.index()];
            if (typeArrayIndex == NullTypeIndex){
                queries++;
                continue;
            }
            JobHandle *readFences = readJobFences[typeArrayIndex];
            uint32_t numReadFences = dependencyHandles[typeArrayIndex].numReadFences;
            allHandles[allHandleCount++] = dependencyHandles[typeArrayIndex].writeFence;
            while(numReadFences--)
                allHandles[allHandleCount++] = readFences[numReadFences];
        } else{
            uint32_t typeArrayIndex = typeArrayIndices[queries->type.index()];
            if (typeArrayIndex != NullTypeIndex)
                allHandles[allHandleCount++] = dependencyHandles[typeArrayIndex].writeFence;
        }
        queries++;
    }
    return JobsUtility::combineDependencies(const_span<JobHandle>{allHandles, allHandleCount});
}
JobHandle ComponentDependencyManager::addDependency(JobHandle dependency, const EntityQueryData &query)
{
    const EntityQueryData::TypeQuery *queries = query.queries.get();
    const EntityQueryData::TypeQuery *queries_end = queries + query.firstNoneIndex;
    if(queries == queries_end)
        return dependency;

    while(queries != queries_end){
        if(queries->flags & EntityQueryData::TypeQuery::WriteFlag){
            dependencyHandles[getTypeArrayIndex(queries->type)].writeFence = dependency;
        }else{
            uint32_t reader = getTypeArrayIndex(queries->type);
            readJobFences[reader][dependencyHandles[reader].numReadFences++] = dependency;
            if (dependencyHandles[reader].numReadFences == MaximumReadJobHandle)
                combineReadDependencies(reader);
        }
        queries++;
    }
    return dependency;
}
JobHandle ComponentDependencyManager::combineReadDependencies(uint32_t typeArrayIndex)
{
    JobHandle combined = JobsUtility::combineDependencies({readJobFences[typeArrayIndex], dependencyHandles[typeArrayIndex].numReadFences});

    readJobFences[typeArrayIndex][0] = combined;
    dependencyHandles[typeArrayIndex].numReadFences = 1;

    return combined;
}
void ComponentDependencyManager::clear(){
    for (uint32_t i = 0; i < dependencyHandlesCount; ++i)
        typeArrayIndices[dependencyHandles[i].type.index()] = NullTypeIndex;
    dependencyHandlesCount = 0;
}

#endif