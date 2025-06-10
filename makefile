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
	$(cutilDir)/advancedArray.h \
	$(cutilDir)/advancedQueue.hpp \
	$(cutilDir)/aStack.hpp \
	$(cutilDir)/bitset.hpp \
	$(cutilDir)/gc_ptr.hpp \
	$(cutilDir)/HashHelper.hpp \
	$(cutilDir)/prototype.hpp \
	$(cutilDir)/QueueQueue.hpp \
	$(cutilDir)/range.hpp \
	$(cutilDir)/semaphore.hpp \
	$(cutilDir)/small_vector.hpp \
	$(cutilDir)/span.hpp \
	$(cutilDir)/static_array.hpp \
	$(cutilDir)/string_view.hpp \
	$(cutilDir)/unique_ptr.hpp \
	$(cutilDir)/basics.hpp \
	$(cutilDir)/set.hpp \
	external/advanced_array/advanced_array.hpp

headerInclude=$(includeDir)/ECS/Archetype.hpp \
	$(includeDir)/ECS/ArchetypeVersionManager.hpp \
	$(includeDir)/ECS/defs.hpp \
	$(includeDir)/ECS/EntityCommandBuffer.hpp \
	$(includeDir)/ECS/EntityComponentManager.hpp \
	$(includeDir)/ECS/SystemManager.hpp \
	$(includeDir)/ECS/ThreadPool.hpp \
	$(includeDir)/ECS/ChunkJobFunction.hpp \
	$(includeDir)/ECS/DependencyManager.hpp

DOBJS=$(OBJ_DEBUG)/src/Archetype.o \
		$(OBJ_DEBUG)/src/ArchetypeVersionManager.o \
		$(OBJ_DEBUG)/src/defs.o \
		$(OBJ_DEBUG)/src/EntityComponentManager.o \
		$(OBJ_DEBUG)/src/SystemManager.o \
		$(OBJ_DEBUG)/src/ThreadPool.o \
		$(OBJ_DEBUG)/src/ChunkJobFunction.o \
		$(OBJ_DEBUG)/src/DependencyManager.o

$(OBJ_DEBUG)/src/SystemManager.o: $(srcDir)/SystemManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/Archetype.o: $(srcDir)/Archetype.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ArchetypeVersionManager.o: $(srcDir)/ArchetypeVersionManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/defs.o: $(srcDir)/defs.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/EntityComponentManager.o: $(srcDir)/EntityComponentManager.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ThreadPool.o: $(srcDir)/ThreadPool.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/ChunkJobFunction.o: $(srcDir)/ChunkJobFunction.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/DependencyManager.o: $(srcDir)/DependencyManager.cpp $(headerInclude) $(headerCutil)
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

$(OBJ_DEBUG)/test/threadpool.o: $(testDir)/threadpool.cpp include/ECS/ThreadPool.hpp
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

test1: $(BIN_DEBUG)/ecs
test2: $(BIN_DEBUG)/job
test3: $(BIN_DEBUG)/thread
test4: $(BIN_DEBUG)/threadpool
test5: $(BIN_DEBUG)/version

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DEBUG) $(BIN_DEBUG)
