#if !defined(MANAGEDDEFERREDCOMMANDS_HPP)
#define MANAGEDDEFERREDCOMMANDS_HPP

#include "cutil/basics.hpp"
#include "cutil/append_buffer.hpp"
#include "Archetype.hpp"
namespace ECS
{
    struct EntityComponentStore;
    struct ManagedDeferredCommands final {
        EntityComponentStore* ecs;
        append_buffer commandBuffer;
        enum Command : uint32_t
        {
            IncrementSharedComponentVersion = 0,
            PatchManagedEntities,
            PatchManagedEntitiesForPrefabs,
            AddReference,
            RemoveReference,
            CloneManagedComponents,
            CloneCompanionComponents,
            FreeManagedComponents,
            SetManagedComponentCapacity
        };

        ManagedDeferredCommands(EntityComponentStore* _ecs):ecs{_ecs},commandBuffer(1024) {}        

        inline void Reset(){
            commandBuffer.reset();
        }

        void incrementComponentOrderVersion(Archetype* archetype,SharedComponentValues sharedComponentValues)
        {
            for (uint32_t i = 0; i < archetype->numSharedComponents(); i++)
            {
                SharedComponentIndex sharedComponentIndex = sharedComponentValues[i];
                if (EntityComponentStore.IsUnmanagedSharedComponentIndex(sharedComponentIndex))
                {
                    ECS->IncrementSharedComponentVersion_Unmanaged(sharedComponentIndex);
                }
                else
                {
                    CommandBuffer.Add<int>((int)Command.IncrementSharedComponentVersion);
                    CommandBuffer.Add<int>(sharedComponentIndex);
            }
        }

        public void PatchEntities(Archetype* archetype, ChunkIndex chunk, int entityCount,
            NativeArray<EntityRemapUtility.EntityRemapInfo> remapping)
        {
            // In every case this is called ManagedChangesTracker.Playback() is called in the same calling function.
            // There is no question of lifetime. So the pointer is safely deferred.

            CommandBuffer.Add<int>((int)Command.PatchManagedEntities);
            CommandBuffer.Add<IntPtr>((IntPtr)archetype);
            CommandBuffer.Add<ChunkIndex>(chunk);
            CommandBuffer.Add<int>(entityCount);
            CommandBuffer.Add<IntPtr>((IntPtr)remapping.GetUnsafePtr());
        }

        public void PatchEntitiesForPrefab(Archetype* archetype, ChunkIndex chunk, int indexInChunk, int allocatedCount,
            Entity* remapSrc, Entity* remapDst, int remappingCount, AllocatorManager.AllocatorHandle allocator)
        {
            // We are deferring the patching so we need a copy of the remapping info since we can't be certain of its lifetime.
            // We will free this ptr in the ManagedComponentStore.PatchEntitiesForPrefab call

            var numManagedComponents = archetype->NumManagedComponents;
            var totalComponentCount = numManagedComponents * allocatedCount;
            var remapSrcSize = UnsafeUtility.SizeOf<Entity>() * remappingCount;
            var remapDstSize = UnsafeUtility.SizeOf<Entity>() * remappingCount * allocatedCount;
            var managedComponentSize = totalComponentCount * sizeof(int);

            var remapSrcCopy = (byte*)Memory.Unmanaged.Allocate(remapSrcSize + remapDstSize + managedComponentSize, 16, Allocator.Temp);
            var remapDstCopy = remapSrcCopy + remapSrcSize;
            var managedComponents = (int*)(remapDstCopy + remapDstSize);

            UnsafeUtility.MemCpy(remapSrcCopy, remapSrc, remapSrcSize);
            UnsafeUtility.MemCpy(remapDstCopy, remapDst, remapDstSize);

            var firstManagedComponent = archetype->FirstManagedComponent;
            for (int i = 0; i < numManagedComponents; ++i)
            {
                int indexInArchetype = i + firstManagedComponent;

                if (archetype->Types[indexInArchetype].HasEntityReferences)
                {
                    var a = (int*)ChunkDataUtility.GetComponentDataRO(chunk, archetype, 0, indexInArchetype);
                    for (int ei = 0; ei < allocatedCount; ++ei)
                        managedComponents[ei * numManagedComponents + i] = a[ei + indexInChunk];
                }
                else
                {
                    for (int ei = 0; ei < allocatedCount; ++ei)
                        managedComponents[ei * numManagedComponents + i] = 0; // 0 means do not remap

                }
            }

            CommandBuffer.Add<int>((int)Command.PatchManagedEntitiesForPrefabs);
            CommandBuffer.Add<IntPtr>((IntPtr)remapSrcCopy);
            CommandBuffer.Add<int>(allocatedCount);
            CommandBuffer.Add<int>(remappingCount);
            CommandBuffer.Add<int>(archetype->NumManagedComponents);
            CommandBuffer.Add<int>(allocator.Value);
        }

        public void AddReference(int index, int numRefs = 1)
        {
            if (index == 0)
                return;
            CommandBuffer.Add<int>((int)Command.AddReference);
            CommandBuffer.Add<int>(index);
            CommandBuffer.Add<int>(numRefs);
        }

        public void RemoveReference(int index, int numRefs = 1)
        {
            if (index == 0)
                return;
            CommandBuffer.Add<int>((int)Command.RemoveReference);
            CommandBuffer.Add<int>(index);
            CommandBuffer.Add<int>(numRefs);
        }

        public void CloneManagedComponentBegin(int* srcIndices, int componentCount, int instanceCount)
        {
            CommandBuffer.Add<int>((int)Command.CloneManagedComponents);
            CommandBuffer.AddArray<int>(srcIndices, componentCount);
            CommandBuffer.Add<int>(instanceCount);
            CommandBuffer.Add<int>(instanceCount * componentCount);
        }

        public void CloneManagedComponentAddDstIndices(int* dstIndices, int count)
        {
            CommandBuffer.Add(dstIndices, count * sizeof(int));
        }

        public void CloneCompanionComponentBegin(int* srcIndices, int componentCount, Entity* dstEntities, int instanceCount, int* dstCompanionReferenceIndices, int* dstCompanionLinkIds)
        {
            CommandBuffer.Add<int>((int)Command.CloneCompanionComponents);
            CommandBuffer.AddArray<int>(srcIndices, componentCount);
            CommandBuffer.AddArray<Entity>(dstEntities, instanceCount);
            CommandBuffer.AddArray<int>(dstCompanionReferenceIndices, dstCompanionReferenceIndices == null ? 0 : instanceCount);
            CommandBuffer.AddArray<int>(dstCompanionLinkIds, dstCompanionLinkIds == null ? 0 : instanceCount);
            CommandBuffer.Add<int>(instanceCount * componentCount);
        }

        public void CloneCompanionComponentAddDstIndices(int* dstIndices, int count)
        {
            CommandBuffer.Add(dstIndices, count * sizeof(int));
        }

        public int BeginFreeManagedComponentCommand()
        {
            CommandBuffer.Add<int>((int)Command.FreeManagedComponents);

            CommandBuffer.Add<int>(-1); // this will contain the array count
            return CommandBuffer.Length - sizeof(int);
        }

        public void AddToFreeManagedComponentCommand(int managedComponentIndex)
        {
            CommandBuffer.Add<int>(managedComponentIndex);
        }

        public void EndDeallocateManagedComponentCommand(int handle)
        {
            int count = (CommandBuffer.Length - handle) / sizeof(int) - 1;
            if (count == 0)
            {
                CommandBuffer.Length -= sizeof(int) * 2;
            }
            else
            {
                int* countInCommand = (int*)(CommandBuffer.Ptr + handle);
                Assert.AreEqual(-1, *countInCommand);
                *countInCommand = count;
            }
        }

        public void SetManagedComponentCapacity(int capacity)
        {
            CommandBuffer.Add<int>((int)Command.SetManagedComponentCapacity);
            CommandBuffer.Add<int>(capacity);
        }
    };
} // namespace ECS

#endif // MANAGEDDEFERREDCOMMANDS_HPP
