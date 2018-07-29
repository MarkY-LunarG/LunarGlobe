######################################################################################
# globe

add_library(globe "")

target_sources(globe
               PRIVATE
                   globe_vulkan_headers.hpp
                   globe_app.hpp
                   globe_app.cpp
                   globe_logger.hpp
                   globe_logger.cpp
                   globe_event.hpp
                   globe_event.cpp
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
              )

target_include_directories(globe
                           PUBLIC
                               ${VULKAN_HEADERS_INSTALL_DIR}/include
                               ${GLOBE_INCLUDE_DIRS}
                               ${PROJECT_SOURCE_DIR}/submodules/stb
                               ${PROJECT_SOURCE_DIR}/submodules/glm
                          )
if(GLOBE_WSI_SELECTION STREQUAL "XCB")
    target_include_directories(globe PUBLIC ${XCB_INCLUDE_DIRS})
elseif(GLOBE_WSI_SELECTION STREQUAL "XLIB")
    target_include_directories(globe PUBLIC ${X11_INCLUDE_DIR})
elseif(GLOBE_WSI_SELECTION STREQUAL "WAYLAND")
    target_include_directories(globe PUBLIC ${WAYLAND_CLIENT_INCLUDE_DIR})
endif()

target_compile_definitions(globe
                            PUBLIC
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -DWIN32 >
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -D_WIN32 >
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -DWIN32_LEAN_AND_MEAN >
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -D_CRT_SECURE_NO_WARNINGS >
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -D_USE_MATH_DEFINES >
                                $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: -DVK_USE_PLATFORM_WIN32_KHR >
                                $<$<STREQUAL: "XCB" , "${GLOBE_WSI_SELECTION}" >: -DVK_USE_PLATFORM_XCB_KHR >
                                $<$<STREQUAL: "XLIB" , "${GLOBE_WSI_SELECTION}" >: -DVK_USE_PLATFORM_XLIB_KHR >
                                $<$<STREQUAL: "WAYLAND" , "${GLOBE_WSI_SELECTION}" >: -DVK_USE_PLATFORM_WAYLAND_KHR >
                          )

target_link_libraries(globe
                      PUBLIC
                          ${LIBVK}
                          $<$<STREQUAL: "Windows" , "${CMAKE_SYSTEM_NAME}" >: Shcore.lib >
                          $<$<STREQUAL: "XCB" , "${GLOBE_WSI_SELECTION}" >: "${XCB_LIBRARIES}" >
                          $<$<STREQUAL: "XLIB" , "${GLOBE_WSI_SELECTION}" >: "${X11_LIBRARIES}" >
                          $<$<STREQUAL: "WAYLAND" , "${GLOBE_WSI_SELECTION}" >: "${WAYLAND_CLIENT_LIBRARIES}" >
                      )
