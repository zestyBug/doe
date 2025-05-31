all: main


LCPP=g++ -std=c++17
LFLAG=-g0 -O3 -m64 -fexceptions  -march=native
LFLAG_DEBUG=-g3 -Og -m64 -fexceptions
# -fsanitize=address
# -fsanitize=address -static-libasan
CCPP=g++
CFLAG_debug=-m64
CFLAG=-m64 -s -march=native

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
	$(includeDir)/ECS/ThreadPool.hpp \
	$(includeDir)/ECS/DependencyManager.hpp

DOBJS=$(OBJ_DEBUG)/src/Archetype.o \
		$(OBJ_DEBUG)/src/ArchetypeVersionManager.o \
		$(OBJ_DEBUG)/src/defs.o \
		$(OBJ_DEBUG)/src/EntityComponentManager.o \
		$(OBJ_DEBUG)/src/SystemManager.o \
		$(OBJ_DEBUG)/src/ThreadPool.o \
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



$(BIN_DEBUG)/ecs: $(OBJ_DEBUG)/test/ecs.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_debug)

$(BIN_DEBUG)/job: $(OBJ_DEBUG)/test/job.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_debug)

$(BIN_DEBUG)/thread: $(OBJ_DEBUG)/test/thread.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_debug)

test1: $(BIN_DEBUG)/ecs
test2: $(BIN_DEBUG)/job
test3: $(BIN_DEBUG)/thread

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DEBUG) $(BIN_DEBUG)
