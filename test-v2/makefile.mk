CXX = g++
CC = gcc

WARNS=-Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
INCLUDE=-Iexternal -Iinclude



ifndef DEBUG

OBJ=obj/release
BIN=bin/release
LDFLAGS=-m64 -s
# -fsanitize=address 
#LDLIBS=-static-libasan
CPPFLAGS=-std=c++17 -g0 -O3 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE) -MMD -MP
CFLAGS=-std=c17 -g0 -O3 -m64 $(WARNS) $(INCLUDE) -MMD -MP
# -fsanitize=address

else

OBJ=obj/test-v2
BIN=bin/test-v2
LDFLAGS=-m64
CPPFLAGS=-DDEBUG -DVERBOSE -std=c++17 -g3 -O0 -m64 -fno-rtti -fexceptions $(WARNS) $(INCLUDE) -MMD -MP
CFLAGS=-std=c17 -g3 -O0 -m64 -fexceptions $(WARNS) $(INCLUDE) -MMD -MP

endif


cutilDir=external/cutil
includeDir=include
srcDir=src
testDir=test-v2
systemDir=src/system

export




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


libuv_la_CFLAGS=-I$(srcDir)/libuv
libuv_la_SOURCES= \
	$(OBJ)/$(srcDir)/libuv/idna.o \
	$(OBJ)/$(srcDir)/libuv/inet.o \
	$(OBJ)/$(srcDir)/libuv/fs-poll.o \
	$(OBJ)/$(srcDir)/libuv/strscpy.o \
	$(OBJ)/$(srcDir)/libuv/strtok.o \
	$(OBJ)/$(srcDir)/libuv/threadpool.o \
	$(OBJ)/$(srcDir)/libuv/uv-common.o \
	$(OBJ)/$(srcDir)/libuv/timer.o \
	$(OBJ)/$(srcDir)/libuv/uv-data-getter-setters.o \
	$(OBJ)/$(srcDir)/libuv/version.o

ifdef WINNT

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
	$(OBJ)/$(srcDir)/libuv/win/fs-event.o \
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

endif  # WINNT

ifdef LINUX
libuv_la_CFLAGS += -D_GNU_SOURCE
libuv_la_SOURCES += \
	$(OBJ)/$(srcDir)/libuv/unix/linux-core.o \
	$(OBJ)/$(srcDir)/libuv/unix/linux-inotify.o \
	$(OBJ)/$(srcDir)/libuv/unix/linux-syscalls.o \
	$(OBJ)/$(srcDir)/libuv/unix/procfs-exepath.o \
	$(OBJ)/$(srcDir)/libuv/unix/proctitle.o \
	$(OBJ)/$(srcDir)/libuv/unix/epoll.o
endif

OBJS= \
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
DEPS += $(libuv_la_SOURCES:.o=.d)
-include $(DEPS)

$(BIN)/test-5: $(OBJ)/$(testDir)/test-5.o $(OBJS) $(libuv_la_SOURCES)
	mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@


test-1: $(BIN)/test-1
test-2: $(BIN)/test-2
test-3: $(BIN)/test-3
test-4: $(BIN)/test-4
test-5: $(BIN)/test-5