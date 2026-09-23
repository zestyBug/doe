cmake_minimum_required (VERSION 3.14)

project(DOE)

#install build-essential libx11-dev libxi-dev libxcursor-dev libxrandr-dev libxinerama-dev

option(DEBUG "Build with debug features" ON)
if(DEBUG)
    add_compile_definitions(DEBUG)
else()
    add_compile_definitions(NDEBUG)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif()

add_compile_definitions(
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

add_library(ECS STATIC
	src/ECS/ResourceManager.cpp
	src/ECS/AssetsManager.cpp
	src/ECS/Archetype.cpp
	src/ECS/ArchetypeChunkData.cpp
	src/ECS/ArchetypeListMap.cpp
	src/ECS/ChunkListMap.cpp
	src/ECS/EntityComponentStore.cpp
	src/ECS/TypeID.cpp
	src/ECS/ThreadPool.cpp
	src/ECS/ComponentDependencyManager.cpp
	src/ECS/ChunkListChanges.cpp
	src/ECS/EntityQueryManager.cpp
	src/ECS/JobChunk.cpp
	src/ECS/ChunkStore.cpp
	src/ECS/SharedComponentStore.cpp
	src/ECS/EntityStore.cpp
)
if(MSVC)
    target_compile_options(ECS PRIVATE
        /W4
        /WX
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ECS PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wno-unused-parameter
        -fno-rtti
        -fexceptions
    )
endif()
target_compile_features(ECS PRIVATE
    cxx_std_17
)



add_library(imgui STATIC
    src/imgui/imgui.cpp
	src/imgui/imgui_widgets.cpp
	src/imgui/imgui_tables.cpp
	src/imgui/imgui_impl_vulkan.cpp
	src/imgui/imgui_draw.cpp
)
if(MSVC)
    target_compile_options(ECS PRIVATE
        /W2
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ECS PRIVATE
        -Wall
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(imgui PRIVATE
        X11
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(imgui PRIVATE
        LDFLAGS+=-lX11 -lXi -lXcursor
    )
endif()

target_compile_features(imgui PRIVATE
    cxx_std_17
)

target_compile_definitions(imgui PRIVATE   
    IMGUI_DISABLE_FILE_FUNCTIONS
)



add_library(libuv STATIC
	src/libuv/idna.c
	src/libuv/inet.c
	src/libuv/strscpy.c
	src/libuv/strtok.c
	src/libuv/threadpool.c
	src/libuv/uv-common.c
	src/libuv/sscanf.c
	src/libuv/timer.c
	src/libuv/uv-data-getter-setters.c
	src/libuv/version.c
)
target_compile_definitions(libuv PRIVATE   
    IMGUI_DISABLE_FILE_FUNCTIONS
)
target_compile_features(libuv PRIVATE
    c_std_17
)
target_include_directories(libuv PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/libuv/
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(libuv PRIVATE
        src/libuv/unix/async.c
        src/libuv/unix/core.c
        src/libuv/unix/dl.c
        src/libuv/unix/fs.c
        src/libuv/unix/getaddrinfo.c
        src/libuv/unix/getnameinfo.c
        src/libuv/unix/loop-watcher.c
        src/libuv/unix/loop.c
        src/libuv/unix/poll.c
        src/libuv/unix/stream.c
        src/libuv/unix/tcp.c
        src/libuv/unix/thread.c
        src/libuv/unix/udp.c
        src/libuv/unix/linux-core.c
        src/libuv/unix/linux-syscalls.c
        src/libuv/unix/procfs-exepath.c
        src/libuv/unix/proctitle.c
        src/libuv/unix/epoll.c
    )
    target_compile_definitions(libuv PRIVATE   
        _GNU_SOURCE
    )
    target_link_libraries(libuv PRIVATE
        X11
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(libuv PRIVATE
        src/libuv/win/async.c
        src/libuv/win/core.c
        src/libuv/win/detect-wakeup.c
        src/libuv/win/dl.c
        src/libuv/win/error.c
        src/libuv/win/fs.c
        src/libuv/win/getaddrinfo.c
        src/libuv/win/getnameinfo.c
        src/libuv/win/handle.c
        src/libuv/win/loop-watcher.c
        src/libuv/win/poll.c
        src/libuv/win/stream.c
        src/libuv/win/tcp.c
        src/libuv/win/thread.c
        src/libuv/win/udp.c
        src/libuv/win/util.c
        src/libuv/win/winapi.c
        src/libuv/win/winsock.c
    )
    target_compile_definitions(libuv PRIVATE   
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0602
    )
    target_link_libraries(libuv PRIVATE
        ws2_32 wsock32 gdi32 iphlpapi userenv
    )
endif()


# $(BIN)/test-7: $(OBJ)/$(testDir)/test-7.o $(libuv_la_SOURCES) $(OBJ)/$(srcDir)/ECS/TypeID.o $(OBJ)/$(cutilDir)/HashHelper.o $(OBJ)/$(srcDir)/ECS/AssetsManager.o $(OBJ)/$(srcDir)/ECS/ResourceManager.o
# $(BIN)/test-6: $(OBJ)/$(testDir)/test-6.o $(libuv_la_SOURCES) $(OBJ)/$(srcDir)/ECS/TypeID.o $(OBJ)/$(cutilDir)/HashHelper.o $(OBJ)/$(srcDir)/ECS/AssetsManager.o
# $(BIN)/test-5: $(OBJ)/$(testDir)/test-5.o $(OBJS) $(libuv_la_SOURCES) $(OBJS_GLFW)
# $(BIN)/test-%: $(OBJ)/$(testDir)/test-%.o $(OBJS) $(OBJ)/$(srcDir)/ECS/TypeID.o

add_executable(engine
    src/vulkan/wrapper.cpp
	src/vulkan/VKContext.cpp
    src/cutil/basics.cpp
    src/cutil/HashHelper.cpp
    src/cutil/nostdio.c
    src/system/example.cpp
    src/Window.cpp
)

if(MSVC)
    target_compile_options(engine PRIVATE
        /W4
        /WX
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(engine PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wno-unused-parameter
        -fno-rtti
        -fexceptions
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(engine PRIVATE
        src/main_linux.cpp
    )
    target_link_libraries(engine PRIVATE
        X11 Xi Xcursor
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(engine PRIVATE
        src/main_win32.cpp
    )
    target_link_libraries(engine PRIVATE
        gdi32
    )
endif()

target_compile_features(engine PRIVATE
    cxx_std_17
    c_std_17
)

target_link_libraries(engine PRIVATE
    ECS
    imgui
    libuv
)

