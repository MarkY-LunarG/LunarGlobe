cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# This must come before the project command.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project (LunarGravity)

find_package(Vulkan)

if(APPLE)
    # CMake versions 3 or later need CMAKE_MACOSX_RPATH defined.
    # This avoids the CMP0042 policy message.
    set(CMAKE_MACOSX_RPATH 1)
    # The "install" target for MacOS fixes up bundles in place.
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
endif()

# Output warning if vulkan headers submodule contents are not present
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/Vulkan-Headers/include/vulkan/vulkan_core.h")
    message(FATAL_ERROR "Please run 'git submodule update --init' before running cmake")
endif()

# Header file for CMake settings
include_directories("${PROJECT_SOURCE_DIR}/submodules/Vulkan-Headers/include")

add_subdirectory(gravity)
add_subdirectory(apps)
