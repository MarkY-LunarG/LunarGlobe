######################################################################################
# globe

add_library(globe "")

target_sources(globe
               PRIVATE
                   globe_vulkan_headers.hpp
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
                   $<${GLOBE_WINDOWS_ENABLED}: windows/globe_window_windows.hpp windows/globe_window_windows.cpp windows/globe_clock_windows.hpp>
                   $<$<OR:${GLOBE_XCB_ENABLED},${GLOBE_XLIB_ENABLED},${GLOBE_WAYLAND_ENABLED}>: linux/globe_window_linux.hpp linux/globe_window_linux.cpp linux/globe_clock_linux.hpp >
                   globe_resource_manager.hpp
                   globe_resource_manager.cpp
                   globe_shader.hpp
                   globe_shader.cpp
                   globe_texture.hpp
                   globe_texture.cpp
                   globe_submit_manager.hpp
                   globe_submit_manager.cpp
              )

target_include_directories(globe
                           PUBLIC
                               ${VULKAN_HEADERS_INSTALL_DIR}/include
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               $<${GLOBE_WINDOWS_ENABLED}: ${CMAKE_CURRENT_SOURCE_DIR}/windows >
                               $<$<OR:${GLOBE_XCB_ENABLED},${GLOBE_XLIB_ENABLED},${GLOBE_WAYLAND_ENABLED}>: ${CMAKE_CURRENT_SOURCE_DIR}/linux >
                               ${PROJECT_SOURCE_DIR}/submodules/stb
                               ${PROJECT_SOURCE_DIR}/submodules/glm
                          )
if(${GLOBE_XCB_ENABLED})
    target_include_directories(globe PUBLIC ${XCB_INCLUDE_DIRS})
elseif(${GLOBE_XLIB_ENABLED})
    target_include_directories(globe PUBLIC ${X11_INCLUDE_DIR})
elseif(${GLOBE_WAYLAND_ENABLED})
    target_include_directories(globe PUBLIC ${WAYLAND_CLIENT_INCLUDE_DIR})
endif()

target_compile_options(globe
                        PUBLIC
                           -std=c++11
                      )

target_compile_definitions(globe
                            PUBLIC
                                $<${GLOBE_WINDOWS_ENABLED}: -DWIN32 >
                                $<${GLOBE_WINDOWS_ENABLED}: -D_WIN32 >
                                $<${GLOBE_WINDOWS_ENABLED}: -DWIN32_LEAN_AND_MEAN >
                                $<${GLOBE_WINDOWS_ENABLED}: -D_CRT_SECURE_NO_WARNINGS >
                                $<${GLOBE_WINDOWS_ENABLED}: -D_USE_MATH_DEFINES >
                                $<${GLOBE_WINDOWS_ENABLED}: -DVK_USE_PLATFORM_WIN32_KHR >
                                $<${GLOBE_XCB_ENABLED}: -DVK_USE_PLATFORM_XCB_KHR >
                                $<${GLOBE_XLIB_ENABLED}: -DVK_USE_PLATFORM_XLIB_KHR >
                                $<${GLOBE_WAYLAND_ENABLED}: -DVK_USE_PLATFORM_WAYLAND_KHR >
                          )

target_link_libraries(globe
                      PUBLIC
                          ${LIBVK}
                          $<${GLOBE_WINDOWS_ENABLED}: Shcore.lib >
                          $<${GLOBE_XCB_ENABLED}: "${XCB_LIBRARIES}" >
                          $<${GLOBE_XLIB_ENABLED}: "${X11_LIBRARIES}" >
                          $<${GLOBE_WAYLAND_ENABLED}: "${WAYLAND_CLIENT_LIBRARIES}" >
                      )
