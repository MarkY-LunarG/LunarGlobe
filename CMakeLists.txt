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

set(GLOBE_WINDOWS_ENABLED 0)
set(GLOBE_XCB_ENABLED 0)
set(GLOBE_XLIB_ENABLED 0)
set(GLOBE_WAYLAND_ENABLED 0)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(DisplayServer Win32)
    set(GLOBE_WINDOWS_ENABLED 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT GLOBE_WSI_SELECTION)
        set(GLOBE_WSI_SELECTION "XCB")
    endif()

    if (GLOBE_WSI_SELECTION STREQUAL "XCB")
        set(GLOBE_XCB_ENABLED 1)
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for globe build but not building Xcb support" )
        endif()
    elseif(GLOBE_WSI_SELECTION STREQUAL "XLIB")
        set(GLOBE_XLIB_ENABLED 1)
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for globe build but not building Xlib support" )
        endif()
    elseif(GLOBE_WSI_SELECTION STREQUAL "WAYLAND")
        set(GLOBE_WAYLAND_ENABLED 1)
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for globe build but not building Wayland support" )
        endif()
    endif()
endif()

######################################################################################
# build macro

macro(SINGLE_SOURCE_EXECUTABLE test_name)
    Message(STATUS "Building single source executable ${test_name}")
    add_executable(${test_name} "")
    target_sources(${test_name}
                   PRIVATE
                       ${test_name}.cpp
                  )
    target_include_directories(${test_name}
                               PUBLIC
                                  ${PROJECT_SOURCE_DIR}
                              )
    target_compile_options(globe
                            PUBLIC
                               -std=c++17
                          )
    target_link_libraries(${test_name}
                          PUBLIC
                              globe
                          )
endmacro()

add_subdirectory(globe)
add_subdirectory(samples)
add_subdirectory(apps)
