if(NOT DEFINED GLSLANG_REPO_ROOT)
    message(STATUS "Using cmake find_program to look for glslangValidator")
    find_program(GLSLANG_VALIDATOR NAMES glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin"
        )
else()
    message(STATUS "Using glslang_repo_root to look for glslangValidator")
    find_program(GLSLANG_VALIDATOR names glslangValidator
        HINTS "${GLSLANG_REPO_ROOT}/build/standalone/release"
        HINTS "${GLSLANG_REPO_ROOT}/build/standalone/debug"
        HINTS "${GLSLANG_REPO_ROOT}/build/StandAlone"
        HINTS "${GLSLANG_REPO_ROOT}/dbuild/StandAlone"
        HINTS "${GLSLANG_REPO_ROOT}/build32/standalone/release"
        HINTS "${GLSLANG_REPO_ROOT}/build32/standalone/debug"
        )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    option(BUILD_WSI_MIR_SUPPORT "Build Mir WSI support" OFF)
    set(GRAVITY_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for gravity (XCB, XLIB, WAYLAND, MIR, DISPLAY)")

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

    if (BUILD_WSI_MIR_SUPPORT)
        find_package(Mir REQUIRED)
    endif()

endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
    if (NOT MSVC_VERSION LESS 1900)
        # Enable control flow guard
        message(STATUS "Building gravity with control flow guard")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/guard:cf>")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT GRAVITY_WSI_SELECTION)
        set(GRAVITY_WSI_SELECTION "XCB")
    endif()

    if (GRAVITY_WSI_SELECTION STREQUAL "XCB")
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for gravity build but not building Xcb support" )
        endif()
        set(GRAVITY_INCLUDE_DIRS
            ${XCB_INCLUDE_DIRS}
            ${GRAVITY_INCLUDE_DIRS}
        )
        link_libraries(${XCB_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    elseif(GRAVITY_WSI_SELECTION STREQUAL "XLIB")
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for gravity build but not building Xlib support" )
        endif()
        set(GRAVITY_INCLUDE_DIRS
            ${X11_INCLUDE_DIR}
            ${GRAVITY_INCLUDE_DIRS}
        )
        link_libraries(${X11_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    elseif(GRAVITY_WSI_SELECTION STREQUAL "WAYLAND")
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for gravity build but not building Wayland support" )
        endif()
        set(GRAVITY_INCLUDE_DIRS
            ${WAYLAND_CLIENT_INCLUDE_DIR}
            ${GRAVITY_INCLUDE_DIRS}
        )
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    elseif(GRAVITY_WSI_SELECTION STREQUAL "MIR")
        if (NOT BUILD_WSI_MIR_SUPPORT)
            message( FATAL_ERROR "Selected MIR for gravity build but not building Mir support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        set(GRAVITY_INCLUDE_DIRS
            ${MIR_INCLUDE_DIR}
            ${GRAVITY_INCLUDE_DIRS}
        )
    elseif(GRAVITY_WSI_SELECTION STREQUAL "DISPLAY")
        add_definitions(-DVK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message( FATAL_ERROR "Unrecognized value for GRAVITY_WSI_SELECTION: ${GRAVITY_WSI_SELECTION}" )
     endif()

    link_libraries(${API_LOWERCASE} m)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

file(GLOB TEXTURES
  "${PROJECT_SOURCE_DIR}/gravity/*.ppm"
  )
file(COPY ${TEXTURES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(APPLE)
    # TODO: Set libs for apple
else()
    if(DEFINED LOADER_REPO_ROOT)
        message(STATUS "Using user-supplied path to locate Vulkan")
        if(WIN32)
            set (LOADER_SEARCH_PATHS
                "${LOADER_REPO_ROOT}/${BUILDTGT_DIR}/loader/${DEBUG_DECORATION}"
                "${LOADER_REPO_ROOT}/${BUILDTGT_DIR}/loader/${RELEASE_DECORATION}"
                )
        elseif(UNIX)
            set (LOADER_SEARCH_PATHS
                "${LOADER_REPO_ROOT}/${BUILDTGT_DIR}/loader"
                )
        endif()

        find_library(LIBVK NAMES vulkan vulkan-1
            HINTS ${LOADER_SEARCH_PATHS}
            )
        message(STATUS "Found Vulkan: ${LIBVK}")
    else()
        message(STATUS "Using find_package to locate Vulkan")
        find_package(Vulkan)
        set (LIBVK "Vulkan::Vulkan")
    endif()
endif()

if(WIN32)
    # Use static MSVCRT libraries
    foreach(configuration in CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
                             CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${configuration} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
        endif()
    endforeach()
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})


if(WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
endif()

# MacOS setup
if(APPLE)
    include(macOS/common.cmake)
endif()

include_directories(
    ${GRAVITY_INCLUDE_DIRS}
    )

######################################################################################
# gravity

set(GRAVITY_SOURCE_FILES
    gravity_vulkan_headers.hpp
    gravity_app.hpp
    gravity_app.cpp
    gravity_logger.hpp
    gravity_logger.cpp
    gravity_event.hpp
    gravity_event.cpp
    gravity_window.hpp
    gravity_window.cpp
    gravity_resource_manager.hpp
    gravity_resource_manager.cpp
    gravity_shader.hpp
    gravity_shader.cpp
    gravity_texture.hpp
    gravity_texture.cpp
    gravity_submit_manager.hpp
    gravity_submit_manager.cpp
)

if(APPLE)
elseif(NOT WIN32)
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_library(gravity ${GRAVITY_SOURCE_FILES})
        target_link_libraries(gravity ${LIBVK})
    endif()
else()
    if (CMAKE_CL_64)
        set (LIB_DIR "Win64")
    else()
        set (LIB_DIR "Win32")
    endif()

    add_library(gravity ${GRAVITY_SOURCE_FILES})
    target_link_libraries(gravity ${LIBVK})
endif()
