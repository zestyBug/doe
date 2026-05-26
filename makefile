all: main


CXX = g++
CC = gcc

WARNS=-Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
INCLUDE=-Iexternal -Iinclude



ifndef DEBUG

OBJ=obj/release
BIN=bin/release
LDFLAGS=-m64 -s -march=native
CPPFLAGS=-DNDEBUG -std=c++17 -g0 -O3 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE) -MMD -MP -march=native
CFLAGS=-std=c17 -g0 -O3 -m64 $(WARNS) $(INCLUDE) -MMD -MP -march=native

else

OBJ=obj/test-v2
BIN=bin/test-v2
# -fsanitize=address -static-libasan
LDFLAGS=-m64
# -fsanitize=address
CPPFLAGS=-DDEBUG -std=c++17 -g3 -O0 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE) -MMD -MP
CFLAGS=-std=c17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE) -MMD -MP

endif


cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test-v2
systemDir=src/system

#export



$(OBJ)/$(srcDir)/system/%.o: $(srcDir)/system/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/ECS/%.o: $(srcDir)/ECS/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(srcDir)/libuv/%.o: $(srcDir)/libuv/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(libuv_la_CFLAGS) -c $< -o $@
$(OBJ)/$(cutilDir)/%.o: $(cutilDir)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@
$(OBJ)/$(testDir)/test-%.o: $(testDir)/test-%.cpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/main.o: $(srcDir)/main.cpp
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $< -o $@
$(OBJ)/$(srcDir)/glfw/%.o: $(srcDir)/glfw/%.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
$(OBJ)/$(srcDir)/glcorearb.o: $(srcDir)/glcorearb.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@






OBJS_GLFW= \
    $(OBJ)/$(srcDir)/glfw/context.o \
	$(OBJ)/$(srcDir)/glfw/init.o \
	$(OBJ)/$(srcDir)/glfw/input.o \
	$(OBJ)/$(srcDir)/glfw/monitor.o \
	$(OBJ)/$(srcDir)/glfw/platform.o \
	$(OBJ)/$(srcDir)/glfw/window.o
ifeq ($(OS),Windows_NT)
	OBJS_GLFW+= \
		$(OBJ)/$(srcDir)/glfw/win32_init.o \
		$(OBJ)/$(srcDir)/glfw/win32_module.o \
		$(OBJ)/$(srcDir)/glfw/win32_monitor.o \
		$(OBJ)/$(srcDir)/glfw/win32_time.o \
		$(OBJ)/$(srcDir)/glfw/win32_window.o
	CPPFLAGS+=-D_GLFW_WIN32
# LDFLAGS+=-lgdi32
else
	OBJS_GLFW+= \
		$(OBJ)/$(srcDir)/glfw/posix_module.o \
		$(OBJ)/$(srcDir)/glfw/posix_poll.o \
		$(OBJ)/$(srcDir)/glfw/posix_time.o \
		$(OBJ)/$(srcDir)/glfw/x11_init.o \
		$(OBJ)/$(srcDir)/glfw/x11_monitor.o \
		$(OBJ)/$(srcDir)/glfw/x11_window.o \
		$(OBJ)/$(srcDir)/glfw/xkb_unicode.o
	CPPFLAGS+=-D_GLFW_X11
	LDFLAGS+=-lGL
#install libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev 
endif






libuv_la_CFLAGS=-I$(srcDir)/libuv
libuv_la_SOURCES= \
	$(OBJ)/$(srcDir)/libuv/idna.o \
	$(OBJ)/$(srcDir)/libuv/inet.o \
	$(OBJ)/$(srcDir)/libuv/strscpy.o \
	$(OBJ)/$(srcDir)/libuv/strtok.o \
	$(OBJ)/$(srcDir)/libuv/threadpool.o \
	$(OBJ)/$(srcDir)/libuv/uv-common.o \
	$(OBJ)/$(srcDir)/libuv/snprintf.o \
	$(OBJ)/$(srcDir)/libuv/sscanf.o \
	$(OBJ)/$(srcDir)/libuv/timer.o \
	$(OBJ)/$(srcDir)/libuv/uv-data-getter-setters.o \
	$(OBJ)/$(srcDir)/libuv/version.o

ifeq ($(OS),Windows_NT)

libuv_la_CFLAGS += -I$(top_srcdir)/src/win \
               -DWIN32_LEAN_AND_MEAN \
               -D_WIN32_WINNT=0x0602
LDFLAGS += -lws2_32 -lwsock32 -lgdi32 -liphlpapi -luserenv
libuv_la_SOURCES += \
	$(OBJ)/$(srcDir)/libuv/win/async.o \
	$(OBJ)/$(srcDir)/libuv/win/core.o \
	$(OBJ)/$(srcDir)/libuv/win/detect-wakeup.o \
	$(OBJ)/$(srcDir)/libuv/win/dl.o \
	$(OBJ)/$(srcDir)/libuv/win/error.o \
	$(OBJ)/$(srcDir)/libuv/win/fs.o \
	$(OBJ)/$(srcDir)/libuv/win/getaddrinfo.o \
	$(OBJ)/$(srcDir)/libuv/win/getnameinfo.o \
	$(OBJ)/$(srcDir)/libuv/win/handle.o \
	$(OBJ)/$(srcDir)/libuv/win/loop-watcher.o \
	$(OBJ)/$(srcDir)/libuv/win/poll.o \
	$(OBJ)/$(srcDir)/libuv/win/stream.o \
	$(OBJ)/$(srcDir)/libuv/win/tcp.o \
	$(OBJ)/$(srcDir)/libuv/win/thread.o \
	$(OBJ)/$(srcDir)/libuv/win/udp.o \
	$(OBJ)/$(srcDir)/libuv/win/util.o \
	$(OBJ)/$(srcDir)/libuv/win/winapi.o \
	$(OBJ)/$(srcDir)/libuv/win/winsock.o
# $(OBJ)/$(srcDir)/libuv/win/signal.o \
# $(OBJ)/$(srcDir)/libuv/win/process.o
# $(OBJ)/$(srcDir)/libuv/win/process-stdio.o

else  # !WINNT

libuv_la_SOURCES += \
	$(OBJ)/$(srcDir)/libuv/unix/async.o \
	$(OBJ)/$(srcDir)/libuv/unix/core.o \
	$(OBJ)/$(srcDir)/libuv/unix/dl.o \
	$(OBJ)/$(srcDir)/libuv/unix/fs.o \
	$(OBJ)/$(srcDir)/libuv/unix/getaddrinfo.o \
	$(OBJ)/$(srcDir)/libuv/unix/getnameinfo.o \
	$(OBJ)/$(srcDir)/libuv/unix/loop-watcher.o \
	$(OBJ)/$(srcDir)/libuv/unix/loop.o \
	$(OBJ)/$(srcDir)/libuv/unix/poll.o \
	$(OBJ)/$(srcDir)/libuv/unix/stream.o \
	$(OBJ)/$(srcDir)/libuv/unix/tcp.o \
	$(OBJ)/$(srcDir)/libuv/unix/thread.o \
	$(OBJ)/$(srcDir)/libuv/unix/udp.o

libuv_la_CFLAGS += -D_GNU_SOURCE
libuv_la_SOURCES += \
	$(OBJ)/$(srcDir)/libuv/unix/linux-core.o \
	$(OBJ)/$(srcDir)/libuv/unix/linux-syscalls.o \
	$(OBJ)/$(srcDir)/libuv/unix/procfs-exepath.o \
	$(OBJ)/$(srcDir)/libuv/unix/proctitle.o \
	$(OBJ)/$(srcDir)/libuv/unix/epoll.o

endif  # WINNT


SYSS= \
	$(OBJ)/$(srcDir)/system/example.o \

OBJS= \
	$(OBJ)/$(srcDir)/ECS/ResourceManager.o \
	$(OBJ)/$(srcDir)/ECS/AssetsManager.o \
	$(OBJ)/$(srcDir)/ECS/Archetype.o \
	$(OBJ)/$(srcDir)/ECS/ArchetypeChunkData.o \
	$(OBJ)/$(srcDir)/ECS/ArchetypeListMap.o \
	$(OBJ)/$(srcDir)/ECS/ChunkListMap.o \
	$(OBJ)/$(srcDir)/ECS/EntityComponentStore.o \
	$(OBJ)/$(srcDir)/ECS/TypeID.o \
	$(OBJ)/$(srcDir)/ECS/ThreadPool.o \
	$(OBJ)/$(srcDir)/ECS/ComponentDependencyManager.o \
	$(OBJ)/$(srcDir)/ECS/ChunkListChanges.o \
	$(OBJ)/$(srcDir)/ECS/EntityQueryManager.o \
	$(OBJ)/$(srcDir)/ECS/JobChunk.o \
	$(OBJ)/$(cutilDir)/HashHelper.o

DEPS = $(OBJS:.o=.d)
DEPS += $(SYSS:.o=.d)
DEPS += $(libuv_la_SOURCES:.o=.d)
DEPS += $(OBJ)/$(testDir)/test-6.d
-include $(DEPS)

$(BIN)/test-7: $(OBJ)/$(testDir)/test-7.o $(libuv_la_SOURCES) $(OBJ)/$(srcDir)/ECS/TypeID.o $(OBJ)/$(cutilDir)/HashHelper.o $(OBJ)/$(srcDir)/ECS/AssetsManager.o $(OBJ)/$(srcDir)/ECS/ResourceManager.o
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@
$(BIN)/test-6: $(OBJ)/$(testDir)/test-6.o $(libuv_la_SOURCES) $(OBJ)/$(srcDir)/ECS/TypeID.o $(OBJ)/$(cutilDir)/HashHelper.o $(OBJ)/$(srcDir)/ECS/AssetsManager.o
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@
$(BIN)/test-5: $(OBJ)/$(testDir)/test-5.o $(OBJS) $(libuv_la_SOURCES) $(OBJS_GLFW)
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@
$(BIN)/test-%: $(OBJ)/$(testDir)/test-%.o $(OBJS) $(OBJ)/$(srcDir)/ECS/TypeID.o
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@
$(BIN)/main: $(OBJ)/main.o $(OBJS) $(libuv_la_SOURCES) $(OBJS_GLFW) $(SYSS)
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@

test-1: $(BIN)/test-1
test-2: $(BIN)/test-2
test-3: $(BIN)/test-3
test-4: $(BIN)/test-4
test-5: $(BIN)/test-5
test-6: $(BIN)/test-6
test-7: $(BIN)/test-7
main: $(BIN)/main

clean:
	@echo "Cleaning up..."
	@$(RM) -rf obj bin
