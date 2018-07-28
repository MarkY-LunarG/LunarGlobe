cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# This must come before the project command.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project (LunarGlobe)

find_package(Vulkan)

# ~~~
# The vulkan loader search is:
#     User-supplied setting of CMAKE_PREFIX_PATH
#     VULKAN_LOADER_INSTALL_DIR defined via cmake option
#     VULKAN_LOADER_INSTALL_DIR defined via environment variable
#     Default findVulkan operation if the VULKAN_SDK environment variable is defined
# ~~~
set(VULKAN_LOADER_INSTALL_DIR "LOADER-NOTFOUND" CACHE PATH "Absolute path to a Vulkan-Loader install directory")

if(VULKAN_LOADER_INSTALL_DIR)
    message(STATUS "VULKAN_LOADER_INSTALL_DIR specified, using find_package to locate Vulkan")
elseif(ENV{VULKAN_LOADER_INSTALL_DIR})
    message(STATUS "VULKAN_LOADER_INSTALL_DIR environment variable specified, using find_package to locate Vulkan")
endif()
set(
    CMAKE_PREFIX_PATH
    ${CMAKE_PREFIX_PATH};${VULKAN_LOADER_INSTALL_DIR};${VULKAN_HEADERS_INSTALL_DIR};$ENV{VULKAN_LOADER_INSTALL_DIR};$ENV{VULKAN_HEADERS_INSTALL_DIR}
    )
find_package(Vulkan)
set(LIBVK "Vulkan::Vulkan")
MESSAGE(STATUS "Vulkan -> ${LIBVK}")

# Cmake command line option overrides environment variable
if(NOT GLSLANG_INSTALL_DIR)
    set(GLSLANG_INSTALL_DIR $ENV{GLSLANG_INSTALL_DIR})
endif()
message(STATUS "Using glslang install located at ${GLSLANG_INSTALL_DIR}")
set(SPIRV_TOOLS_BINARY_ROOT "${GLSLANG_INSTALL_DIR}/lib"
    CACHE PATH "User defined path to the SPIRV-Tools binaries for this project")
set(SPIRV_TOOLS_OPT_BINARY_ROOT "${GLSLANG_INSTALL_DIR}/lib"
    CACHE PATH "User defined path to the SPIRV-Tools-opt binaries for this project")
set(GLSLANG_SPIRV_INCLUDE_DIR "${GLSLANG_INSTALL_DIR}/include" CACHE PATH "Path to glslang spirv headers")
set(SPIRV_TOOLS_INCLUDE_DIR "${GLSLANG_INSTALL_DIR}/include" CACHE PATH "Path to spirv tools headers")
set(GLSLANG_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")
set(GLSLANG_DEBUG_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")
set(SPIRV_TOOLS_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")
set(SPIRV_TOOLS_DEBUG_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")
set(SPIRV_TOOLS_OPT_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")
set(SPIRV_TOOLS_OPT_DEBUG_SEARCH_PATH "${GLSLANG_INSTALL_DIR}/lib")

find_library(GLSLANG_LIB NAMES glslang HINTS ${GLSLANG_SEARCH_PATH})
find_library(OGLCompiler_LIB NAMES OGLCompiler HINTS ${GLSLANG_SEARCH_PATH})
find_library(OSDependent_LIB NAMES OSDependent HINTS ${GLSLANG_SEARCH_PATH})
find_library(HLSL_LIB NAMES HLSL HINTS ${GLSLANG_SEARCH_PATH})
find_library(SPIRV_LIB NAMES SPIRV HINTS ${GLSLANG_SEARCH_PATH})
find_library(SPIRV_REMAPPER_LIB NAMES SPVRemapper HINTS ${GLSLANG_SEARCH_PATH})
find_library(SPIRV_TOOLS_LIB NAMES SPIRV-Tools HINTS ${SPIRV_TOOLS_SEARCH_PATH})
find_library(SPIRV_TOOLS_OPT_LIB NAMES SPIRV-Tools-opt HINTS ${SPIRV_TOOLS_OPT_SEARCH_PATH})

if(NOT DEFINED GLSLANG_INSTALL_DIR)
    message(STATUS "Using cmake find_program to look for glslangValidator")
    find_program(GLSLANG_VALIDATOR NAMES glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin"
        )
else()
    message(STATUS "Using GLSLANG_INSTALL_DIR to look for glslangValidator")
    find_program(GLSLANG_VALIDATOR names glslangValidator
        HINTS "${GLSLANG_INSTALL_DIR}/bin"
        )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    set(GLOBE_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for globe (XCB, XLIB, WAYLAND, DISPLAY)")

    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        find_package(X11 REQUIRED)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT GLOBE_WSI_SELECTION)
        set(GLOBE_WSI_SELECTION "XCB")
    endif()

    if (GLOBE_WSI_SELECTION STREQUAL "XCB")
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for globe build but not building Xcb support" )
        endif()
    elseif(GLOBE_WSI_SELECTION STREQUAL "XLIB")
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for globe build but not building Xlib support" )
        endif()
    elseif(GLOBE_WSI_SELECTION STREQUAL "WAYLAND")
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for globe build but not building Wayland support" )
        endif()
    endif()
endif()

add_subdirectory(globe)
add_subdirectory(apps)
