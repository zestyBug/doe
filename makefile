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

headerGLFW= \
	external/glfw/glfw3.h \
	external/glfw/glfw3native.h \
	src/glfw/xkb_unicode.h \
	src/glfw/x11_platform.h \
	src/glfw/wl_platform.h \
	src/glfw/posix_time.h \
	src/glfw/posix_poll.h \
	src/glfw/platform.h \
	src/glfw/linux_joystick.h \
	src/glfw/mappings.h \
	src/glfw/internal.h

DOBJS_GLFW= \
    $(OBJ_DEBUG)/src/glfw/context.o \
	$(OBJ_DEBUG)/src/glfw/init.o \
	$(OBJ_DEBUG)/src/glfw/input.o \
	$(OBJ_DEBUG)/src/glfw/monitor.o \
	$(OBJ_DEBUG)/src/glfw/platform.o \
	$(OBJ_DEBUG)/src/glfw/posix_module.o \
	$(OBJ_DEBUG)/src/glfw/posix_poll.o \
	$(OBJ_DEBUG)/src/glfw/posix_time.o \
	$(OBJ_DEBUG)/src/glfw/window.o \
	$(OBJ_DEBUG)/src/glfw/x11_init.o \
	$(OBJ_DEBUG)/src/glfw/x11_monitor.o \
	$(OBJ_DEBUG)/src/glfw/x11_window.o \
	$(OBJ_DEBUG)/src/glfw/xkb_unicode.o

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

headerInclude= \
	$(includeDir)/ECS/Archetype.hpp \
	$(includeDir)/ECS/ArchetypeVersionManager.hpp \
	$(includeDir)/ECS/defs.hpp \
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

$(OBJ_DEBUG)/src/glfw/%.o: $(srcDir)/glfw/%.c $(headerGLFW)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -D_GLFW_X11 -c $< -o $@

$(OBJ_DEBUG)/src/ECS/%.o: $(srcDir)/ECS/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/%.o: $(srcDir)/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/src/main.o: $(srcDir)/main.cpp $(headerGLFW) $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@


$(OBJ_DEBUG)/test/%.o: $(testDir)/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(LCPP) $(INCLUDE) $(LFLAG_WARN) $(LFLAG_DEBUG) -c $< -o $@

$(OBJ_DEBUG)/test/threadpool.o: $(testDir)/threadpool.cpp include/ThreadPool.hpp
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

$(BIN_DEBUG)/main: $(DOBJS_GLFW) $(OBJ_DEBUG)/src/main.o $(OBJ_DEBUG)/src/system/linux_init.o $(DOBJS)
	mkdir -p $(@D)
	$(CCPP) -o $@ $^ $(CFLAG_DEBUG)

test1: $(BIN_DEBUG)/ecs
test2: $(BIN_DEBUG)/job
test3: $(BIN_DEBUG)/thread
test4: $(BIN_DEBUG)/threadpool
test5: $(BIN_DEBUG)/version
main: $(BIN_DEBUG)/main

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DEBUG) $(BIN_DEBUG)
