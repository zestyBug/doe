all: main


LCPP=g++ -std=c++17
LFLAG=-g0 -O3 -m64 -fexceptions  -march=native
LFLAG_DEBUG=-g3 -O0 -m64 -fexceptions -DDEBUG
# -fsanitize=address
CCPP=g++
CFLAG=-m64 -s -march=native
CFLAG_DEBUG=-m64
# -fsanitize=address -static-libasan

LFLAG_WARN=-Wall -Wconversion -Wextra -Wfatal-errors -Wshadow

INCLUDE=-Iexternal -Iinclude

OBJ_DEBUG=obj/Debug
BIN_DEBUG=bin/Debug

cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test


headerCutil= \
	$(cutilDir)/bitset.hpp \
	$(cutilDir)/HashHelper.hpp \
	$(cutilDir)/prototype.hpp \
	$(cutilDir)/range.hpp \
	$(cutilDir)/semaphore.hpp \
	$(cutilDir)/small_vector.hpp \
	$(cutilDir)/span.hpp \
	$(cutilDir)/static_array.hpp \
	$(cutilDir)/string_view.hpp \
	$(cutilDir)/unique_ptr.hpp \
	$(cutilDir)/basics.hpp \
	$(cutilDir)/set.hpp \
	$(cutilDir)/mini_test.hpp \
	external/entt/advanced_array.hpp

headerInclude=$(includeDir)/ECS/Archetype.hpp \
	$(includeDir)/ECS/ArchetypeVersionManager.hpp \
	$(includeDir)/ECS/defs.hpp \
	$(includeDir)/ECS/EntityCommandBuffer.hpp \
	$(includeDir)/ECS/EntityComponentManager.hpp \
	$(includeDir)/ECS/SystemManager.hpp \
	$(includeDir)/ThreadPool.hpp \
	$(includeDir)/ECS/ChunkJobFunction.hpp \
	$(includeDir)/ECS/DependencyManager.hpp

DOBJS=$(OBJ_DEBUG)/src/ECS/Archetype.o \
		$(OBJ_DEBUG)/src/ECS/ArchetypeVersionManager.o \
		$(OBJ_DEBUG)/src/ECS/defs.o \
		$(OBJ_DEBUG)/src/ECS/EntityComponentManager.o \
		$(OBJ_DEBUG)/src/ECS/SystemManager.o \
		$(OBJ_DEBUG)/src/ThreadPool.o \
		$(OBJ_DEBUG)/src/ECS/ChunkJobFunction.o \
		$(OBJ_DEBUG)/src/ECS/DependencyManager.o

$(OBJ_DEBUG)/src/ECS/SystemManager.o: $(srcDir)/ECS/SystemManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/Archetype.o: $(srcDir)/ECS/Archetype.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/ArchetypeVersionManager.o: $(srcDir)/ECS/ArchetypeVersionManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/defs.o: $(srcDir)/ECS/defs.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/EntityComponentManager.o: $(srcDir)/ECS/EntityComponentManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ThreadPool.o: $(srcDir)/ThreadPool.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/ChunkJobFunction.o: $(srcDir)/ECS/ChunkJobFunction.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ECS/DependencyManager.o: $(srcDir)/ECS/DependencyManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@


$(OBJ_DEBUG)/src/os/main_linux.o: $(srcDir)/os/main_linux.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@


$(OBJ_DEBUG)/test/ecs.o: $(testDir)/ecs.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/test/job.o: $(testDir)/job.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/test/thread.o: $(testDir)/thread.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/test/threadpool.o: $(testDir)/threadpool.cpp include/ThreadPool.hpp
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/system/linux_init.o: src/system/linux_init.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/test/version.o: $(testDir)/version.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@



$(BIN_DEBUG)/ecs: $(OBJ_DEBUG)/test/ecs.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

$(BIN_DEBUG)/job: $(OBJ_DEBUG)/test/job.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

$(BIN_DEBUG)/thread: $(OBJ_DEBUG)/test/thread.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

$(BIN_DEBUG)/threadpool: $(OBJ_DEBUG)/test/threadpool.o $(OBJ_DEBUG)/src/ThreadPool.o
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

$(BIN_DEBUG)/version: $(OBJ_DEBUG)/test/version.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

$(BIN_DEBUG)/linux: $(OBJ_DEBUG)/src/os/main_linux.o $(OBJ_DEBUG)/src/system/linux_init.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

test1: $(BIN_DEBUG)/ecs
test2: $(BIN_DEBUG)/job
test3: $(BIN_DEBUG)/thread
test4: $(BIN_DEBUG)/threadpool
test5: $(BIN_DEBUG)/version
test6: $(BIN_DEBUG)/linux

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DEBUG) $(BIN_DEBUG)
