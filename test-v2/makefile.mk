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
CPPFLAGS=-DDEBUG -DVERBOSE -std=c++17 -g3 -O0 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE) -MMD -MP
CFLAGS=-std=c17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE) -MMD -MP
# -fsanitize=address


cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test-v2
systemDir=src/system

export




$(OBJ)/$(srcDir)/ECS/Archetype.o: $(srcDir)/ECS/Archetype.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/ArchetypeChunkData.o: $(srcDir)/ECS/ArchetypeChunkData.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/ChunkListMap.o: $(srcDir)/ECS/ChunkListMap.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/EntityComponentStore.o: $(srcDir)/ECS/EntityComponentStore.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/TypeID.o: $(srcDir)/ECS/TypeID.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/ThreadPool.o: $(srcDir)/ECS/ThreadPool.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/ChunkListChanges.o: $(srcDir)/ECS/ChunkListChanges.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(cutilDir)/HashHelper.o: $(cutilDir)/HashHelper.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(testDir)/test-%.o: $(testDir)/test-%.cpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

OBJS= \
	$(OBJ)/$(srcDir)/ECS/Archetype.o \
	$(OBJ)/$(srcDir)/ECS/ArchetypeChunkData.o \
	$(OBJ)/$(srcDir)/ECS/ChunkListMap.o \
	$(OBJ)/$(srcDir)/ECS/EntityComponentStore.o \
	$(OBJ)/$(srcDir)/ECS/TypeID.o \
	$(OBJ)/$(srcDir)/ECS/ThreadPool.o \
	$(OBJ)/$(srcDir)/ECS/ChunkListChanges.o \
	$(OBJ)/$(cutilDir)/HashHelper.o

DEPS = $(OBJS:.o=.d)
-include $(DEPS)

$(BIN)/test-%: $(OBJ)/$(testDir)/test-%.o $(OBJS)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@


test-1: $(BIN)/test-1
test-2: $(BIN)/test-2
test-3: $(BIN)/test-3
test-4: $(BIN)/test-4