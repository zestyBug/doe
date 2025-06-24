cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)
project(DOE)

include(CMakeDependentOption)

option(DEFINE_DEBUG "Runtime debug checks" OFF)
cmake_dependent_option(GLFW_BUILD_WIN32 "Build support for Win32" ON "WIN32" OFF)
cmake_dependent_option(GLFW_BUILD_X11 "Build support for X11" ON "UNIX;NOT APPLE" OFF)
option(GLFW_BUILD_WAYLAND "Build support for Wayland" OFF)

set(CMAKE_CXX_STANDARD 17)
include_directories(include external)

if (DEFINE_DEBUG)
	add_compile_definitions(DEBUG)
endif()

add_executable(DOE src/main.cpp)

target_sources(DOE PRIVATE include/ThreadPool.hpp
		src/ThreadPool.cpp
		include/ECS/Archetype.hpp 
		src/ECS/Archetype.cpp
		include/ECS/ArchetypeVersionManager.hpp 
		src/ECS/ArchetypeVersionManager.cpp
		include/ECS/defs.hpp 
		src/ECS/defs.cpp
		include/ECS/DependencyManager.hpp 
		src/ECS/DependencyManager.cpp
		include/ECS/SystemManager.hpp 
		src/ECS/SystemManager.cpp
		include/ECS/ChunkJobFunction.hpp 
		src/ECS/ChunkJobFunction.cpp
		include/ECS/EntityComponentManager.hpp 
		src/ECS/EntityComponentManager.cpp)

target_sources(DOE PRIVATE external/glfw/glfw3.h
		external/glfw/glfw3native.h
		src/glfw/internal.h 
		src/glfw/platform.h 
		src/glfw/mappings.h
		src/glfw/context.c 
		src/glfw/init.c 
		src/glfw/input.c 
		src/glfw/monitor.c 
		src/glfw/platform.c
		src/glfw/window.c)


if (GLFW_BUILD_WIN32)
    target_compile_definitions(DOE PRIVATE _GLFW_WIN32)
    target_sources(DOE PRIVATE src/glfw/win32_platform.h
			src/glfw/win32_joystick.h
			src/glfw/win32_init.c
			src/glfw/win32_joystick.c
			src/glfw/win32_monitor.c
			src/glfw/win32_window.c)
endif()

if (GLFW_BUILD_X11 AND NOT GLFW_BUILD_WAYLAND)
    target_compile_definitions(DOE PRIVATE _GLFW_X11)
    target_sources(DOE PRIVATE src/glfw/x11_platform.h
			src/glfw/x11_init.c
			src/glfw/x11_monitor.c
			src/glfw/x11_window.c)
endif()

if (GLFW_BUILD_WAYLAND)
    target_compile_definitions(DOE PRIVATE _GLFW_WAYLAND)
    target_sources(DOE PRIVATE src/glfw/wl_platform.h
			src/glfw/wl_init.c
			src/glfw/wl_monitor.c
			src/glfw/wl_window.c)
endif()

if (GLFW_BUILD_X11 OR GLFW_BUILD_WAYLAND)
    target_sources(DOE PRIVATE src/glfw/posix_poll.h
			src/glfw/posix_poll.c
			src/glfw/posix_time.h
			src/glfw/posix_time.c
			src/glfw/posix_module.c
			src/glfw/xkb_unicode.h
			src/glfw/xkb_unicode.c)
endif()

