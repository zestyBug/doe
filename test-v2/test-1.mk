CXX = g++
CC = gcc

WARNS=-Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
INCLUDE=-Iexternal -Iinclude
#LOADLIBES ?=

OBJ=obj/test-v2
BIN=bin/test-v2
LDFLAGS=-m64
# -fsanitize=address 
#LDLIBS=-static-libasan
CPPFLAGS=-DDEBUG -DVERBOSE -std=c++17 -g3 -O0 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE)
CFLAGS=-std=c17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE)
# -fsanitize=address


cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test-v2
systemDir=src/system

export


$(includeDir)/ECS/Base/Chunk.hpp: $(cutilDir)/basics.hpp
$(includeDir)/ECS/Base/Entity.hpp: $(cutilDir)/basics.hpp $(includeDir)/ECS/Base/Chunk.hpp
$(includeDir)/ECS/Base/TypeID.hpp: $(includeDir)/ECS/Base/Entity.hpp $(cutilDir)/basics.hpp $(cutilDir)/span.hpp $(cutilDir)/static_array.hpp
$(includeDir)/ECS/Base/Version.hpp: $(cutilDir)/basics.hpp
$(includeDir)/ECS/Archetype.hpp: $(includeDir)/ECS/Base/TypeID.hpp $(cutilDir)/basics.hpp $(cutilDir)/HashHelper.hpp $(cutilDir)/mark_ptr.hpp $(includeDir)/ECS/Base/Chunk.hpp $(includeDir)/ECS/ChunkListMap.hpp
$(includeDir)/ECS/ArchetypeChunkData.hpp: $(cutilDir)/basics.hpp $(includeDir)/ECS/Base/Chunk.hpp $(includeDir)/ECS/SharedComponent.hpp $(includeDir)/ECS/Base/Version.hpp
$(includeDir)/ECS/ArchetypeListMap.hpp: $(cutilDir)/basics.hpp $(cutilDir)/HashHelper.hpp $(includeDir)/ECS/Archetype.hpp
$(includeDir)/ECS/ChunkListMap.hpp: $(cutilDir)/basics.hpp $(cutilDir)/HashHelper.hpp $(includeDir)/ECS/Base/Chunk.hpp $(includeDir)/ECS/SharedComponent.hpp
$(includeDir)/ECS/ChunkStore.hpp: $(includeDir)/ECS/Base/Chunk.hpp $(cutilDir)/span.hpp
$(includeDir)/ECS/EntityComponentStore.hpp: $(includeDir)/ECS/Base/Entity.hpp $(includeDir)/ECS/Base/Chunk.hpp $(includeDir)/ECS/Archetype.hpp $(cutilDir)/span.hpp $(includeDir)/ECS/ArchetypeListMap.hpp $(includeDir)/ECS/EntityStore.hpp $(includeDir)/ECS/ChunkStore.hpp
$(includeDir)/ECS/EntityStore.hpp: $(includeDir)/ECS/Base/Chunk.hpp $(includeDir)/ECS/Base/TypeID.hpp $(includeDir)/ECS/Base/Entity.hpp
$(includeDir)/ECS/SharedComponent.hpp: $(cutilDir)/basics.hpp








$(OBJ)/$(srcDir)/ECS/Archetype.o: $(srcDir)/ECS/Archetype.cpp $(includeDir)/ECS/Archetype.hpp $(includeDir)/ECS/EntityComponentStore.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(srcDir)/ECS/ArchetypeChunkData.o: $(srcDir)/ECS/ArchetypeChunkData.cpp $(includeDir)/ECS/ArchetypeChunkData.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(srcDir)/ECS/ChunkListMap.o: $(srcDir)/ECS/ChunkListMap.cpp $(includeDir)/ECS/ChunkListMap.hpp $(includeDir)/ECS/Archetype.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(srcDir)/ECS/EntityComponentStore.o: $(srcDir)/ECS/EntityComponentStore.cpp $(includeDir)/ECS/EntityComponentStore.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(srcDir)/ECS/TypeID.o: $(srcDir)/ECS/TypeID.cpp $(includeDir)/ECS/Base/TypeID.hpp $(cutilDir)/basics.hpp $(includeDir)/ECS/Base/Entity.hpp $(cutilDir)/HashHelper.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(cutilDir)/HashHelper.o: $(cutilDir)/HashHelper.cpp $(cutilDir)/HashHelper.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(testDir)/test-1.o: $(testDir)/test-1.cpp $(includeDir)/ECS/Base/TypeID.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

OBJS= \
	$(OBJ)/src/ECS/Archetype.o \
	$(OBJ)/src/ECS/ArchetypeChunkData.o \
	$(OBJ)/src/ECS/ChunkListMap.o \
	$(OBJ)/src/ECS/EntityComponentStore.o \
	$(OBJ)/src/ECS/TypeID.o \
	$(OBJ)/$(cutilDir)/HashHelper.o

$(BIN)/test-1: $(OBJ)/$(testDir)/test-1.o $(OBJS)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(BIN)/test-1