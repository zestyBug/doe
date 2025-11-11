CXX = g++
CC = gcc

WARNS=-Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
INCLUDE=-Iexternal -Iinclude
#LOADLIBES ?=

ifndef DEBUG
OBJ=obj/Release
BIN=bin/Release
LDFLAGS=-m64 -s -march=native
CXXFLAGS=-std=c++17 -g0 -O3 -m64 -fexceptions -march=native $(INCLUDE)
CFLAGS=-std=c17 -g0 -O3 -m64 -fexceptions -march=native $(INCLUDE)
else
OBJ=obj/Debug
BIN=bin/Debug
LDFLAGS=-m64
# -fsanitize=address 
#LDLIBS=-static-libasan
CPPFLAGS=-DDEBUG
CXXFLAGS=-std=c++17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE)
CFLAGS=-std=c17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE)
# -fsanitize=address
endif


cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test
systemDir=src/system

export

headerGLFW= \
	external/glfw/glfw3.h \
	external/glfw/glfw3native.h \
	$(srcDir)/glfw/platform.h \
	$(srcDir)/glfw/mappings.h \
	$(srcDir)/glfw/internal.h

OBJS_GLFW= \
    $(OBJ)/src/glfw/context.o \
	$(OBJ)/src/glfw/init.o \
	$(OBJ)/src/glfw/input.o \
	$(OBJ)/src/glfw/monitor.o \
	$(OBJ)/src/glfw/platform.o \
	$(OBJ)/src/glfw/window.o

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
	$(cutilDir)/mark_ptr.hpp \
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
	$(srcDir)/ECS/ChunkJobFunction.hpp \
	$(includeDir)/ECS/DependencyManager.hpp \
	$(includeDir)/ECS/ResourceGC.hpp \
	$(includeDir)/VulkanApp.hpp

OBJ_ECS=$(OBJ)/src/ECS/Archetype.o \
		$(OBJ)/src/ECS/ArchetypeVersionManager.o \
		$(OBJ)/src/ECS/defs.o \
		$(OBJ)/src/ECS/EntityComponentManager.o \
		$(OBJ)/src/ECS/SystemManager.o \
		$(OBJ)/src/ThreadPool.o \
		$(OBJ)/src/ECS/ChunkJobFunction.o \
		$(OBJ)/src/ECS/DependencyManager.o \
		$(OBJ)/src/ECS/ResourceGC.o \
		$(OBJ)/$(cutilDir)/HashHelper.o

OBJ_DOE=$(OBJ)/external/vulkan_wrapper.o \
		$(OBJ)/src/VulkanApp.o



ifeq ($(OS),Windows_NT)
    headerGLFW+= \
		$(srcDir)/glfw/win32_joystick.h \
		$(srcDir)/glfw/win32_platform.h \
		$(srcDir)/glfw/win32_time.h
	OBJS_GLFW+= \
		$(OBJ)/src/glfw/win32_init.o \
		$(OBJ)/src/glfw/win32_module.o \
		$(OBJ)/src/glfw/win32_monitor.o \
		$(OBJ)/src/glfw/win32_time.o \
		$(OBJ)/src/glfw/win32_window.o
	CPPFLAGS+=-D_GLFW_WIN32 -DVK_USE_PLATFORM_WIN32_KHR
	LDLIBS+=-lgdi32
else
	headerGLFW+= \
		$(srcDir)/glfw/xkb_unicode.h \
		$(srcDir)/glfw/x11_platform.h \
		$(srcDir)/glfw/wl_platform.h \
		$(srcDir)/glfw/posix_time.h \
		$(srcDir)/glfw/posix_poll.h \
		$(srcDir)/glfw/linux_joystick.h
	OBJS_GLFW+= \
		$(OBJ)/src/glfw/posix_module.o \
		$(OBJ)/src/glfw/posix_poll.o \
		$(OBJ)/src/glfw/posix_time.o \
		$(OBJ)/src/glfw/x11_init.o \
		$(OBJ)/src/glfw/x11_monitor.o \
		$(OBJ)/src/glfw/x11_window.o \
		$(OBJ)/src/glfw/xkb_unicode.o
	CPPFLAGS+=-D_GLFW_X11 -DVK_USE_PLATFORM_XLIB_KHR
# libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev 
endif




$(OBJ)/src/glfw/%.o: $(srcDir)/glfw/%.c $(headerGLFW)
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OBJ)/src/ECS/%.o: $(srcDir)/ECS/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/src/%.o: $(srcDir)/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/src/main.o: $(srcDir)/main.cpp $(headerGLFW) $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/$(cutilDir)/%.o: $(cutilDir)/%.cpp $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/external/vulkan_wrapper.o: external/vulkan_wrapper.c external/vulkan_wrapper.h
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/test/%.o: $(testDir)/%.cpp $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ)/test/threadpool.o: $(testDir)/threadpool.cpp include/ThreadPool.hpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

include $(systemDir)/makefile.mk

OBJ_system ?= 

$(BIN)/%: $(OBJ)/test/%.o $(OBJ_ECS)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(BIN)/threadpool: $(OBJ)/test/threadpool.o $(OBJ)/src/ThreadPool.o
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(BIN)/main: $(OBJ)/src/main.o  $(OBJS_GLFW) $(OBJ_ECS) $(OBJ_DOE) $(OBJ_system)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@


test1: $(BIN)/ecs
test2: $(BIN)/job
test3: $(BIN)/thread
test4: $(BIN)/threadpool
test5: $(BIN)/version
test6: $(BIN)/resourcegc
test7: $(BIN)/structs
main: $(BIN)/main