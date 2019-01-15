#
# Project:                 LunarGlobe
# SPDX-License-Identifier: Apache-2.0
#
# File:                    globe/CMakeLists.txt
# Copyright(C):            2018-2019; LunarG, Inc.
# Author(s):               Mark Young <marky@lunarg.com>
#

# First, all the global settings
#
add_library(globe STATIC "")

target_sources(globe PRIVATE
                   globe_vulkan_headers.hpp
                   globe_glm_include.hpp
                   globe_basic_types.hpp
                   globe_app.hpp
                   globe_app.cpp
                   globe_camera.hpp
                   globe_camera.cpp
                   globe_logger.hpp
                   globe_logger.cpp
                   globe_event.hpp
                   globe_event.cpp
                   globe_clock.hpp
                   globe_clock.cpp
                   globe_window.hpp
                   globe_window.cpp
                   globe_resource_manager.hpp
                   globe_resource_manager.cpp
                   globe_shader.hpp
                   globe_shader.cpp
                   globe_texture.hpp
                   globe_texture.cpp
                   globe_submit_manager.hpp
                   globe_submit_manager.cpp
                   globe_model.hpp
                   globe_model.cpp
              )

target_include_directories(globe PUBLIC
                               ${VULKAN_HEADERS_INSTALL_DIR}/include
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${PROJECT_SOURCE_DIR}/submodules/stb
                               ${PROJECT_SOURCE_DIR}/submodules/glm
                               ${PROJECT_SOURCE_DIR}/submodules/assimp/include
                               ${CMAKE_BINARY_DIR}/submodules/assimp/include
                          )

target_compile_options(globe PUBLIC
                          -std=c++11
                      )

target_link_libraries(globe PUBLIC
                          ${LIBVK}
                          assimp
                     )

# Target specific settings
#
if(${GLOBE_WINDOWS_ENABLED})
    target_include_directories(globe PUBLIC
                                   ${CMAKE_CURRENT_SOURCE_DIR}/windows
                              )
    target_compile_definitions(globe PUBLIC
                                  -DWIN32
                                  -D_WIN32
                                  -DWIN32_LEAN_AND_MEAN
                                  -D_CRT_SECURE_NO_WARNINGS
                                  -D_USE_MATH_DEFINES
                                  -DVK_USE_PLATFORM_WIN32_KHR
                            )
    target_sources(globe PRIVATE
                       windows/globe_window_windows.hpp
                       windows/globe_window_windows.cpp
                       windows/globe_clock_windows.hpp>
                  )
    target_link_libraries(globe PUBLIC
                              Shcore.lib
                         )

elseif(${GLOBE_XCB_ENABLED})
    target_include_directories(globe PUBLIC
                                  ${XCB_INCLUDE_DIRS}
                                  ${CMAKE_CURRENT_SOURCE_DIR}/linux)
    target_compile_definitions(globe PUBLIC
                                  -DVK_USE_PLATFORM_XCB_KHR
                              )
    target_sources(globe PRIVATE
                       linux/globe_window_linux.hpp
                       linux/globe_window_linux.cpp
                       linux/globe_clock_linux.hpp
                  )
    target_link_libraries(globe PUBLIC
                            "${XCB_LIBRARIES}"
                         )
elseif(${GLOBE_XLIB_ENABLED})
    target_include_directories(globe PUBLIC
                                  ${X11_INCLUDE_DIR}
                                  ${CMAKE_CURRENT_SOURCE_DIR}/linux)
    target_compile_definitions(globe PUBLIC
                                  -DVK_USE_PLATFORM_XLIB_KHR
                              )
    target_sources(globe PRIVATE
                       linux/globe_window_linux.hpp
                       linux/globe_window_linux.cpp
                       linux/globe_clock_linux.hpp
                  )
    target_link_libraries(globe PUBLIC
                            "${X11_LIBRARIES}"
                         )
elseif(${GLOBE_WAYLAND_ENABLED})
    target_include_directories(globe PUBLIC
                                  ${WAYLAND_CLIENT_INCLUDE_DIR}
                                  ${CMAKE_CURRENT_SOURCE_DIR}/linux)
    target_compile_definitions(globe PUBLIC
                                  -DVK_USE_PLATFORM_WAYLAND_KHR
                              )
    target_sources(globe PRIVATE
                       linux/globe_window_linux.hpp
                       linux/globe_window_linux.cpp
                       linux/globe_clock_linux.hpp
                  )
    target_link_libraries(globe PUBLIC
                              "${WAYLAND_CLIENT_LIBRARIES}"
                         )
endif()
