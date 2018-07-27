set(GLOBE_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}
)

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
    set(GLOBE_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for globe (XCB, XLIB, WAYLAND, MIR, DISPLAY)")

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
        message(STATUS "Building globe with control flow guard")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/guard:cf>")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT GLOBE_WSI_SELECTION)
        set(GLOBE_WSI_SELECTION "XCB")
    endif()

    if (GLOBE_WSI_SELECTION STREQUAL "XCB")
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for globe build but not building Xcb support" )
        endif()
        set(GLOBE_INCLUDE_DIRS
            ${XCB_INCLUDE_DIRS}
            ${GLOBE_INCLUDE_DIRS}
        )
        link_libraries(${XCB_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    elseif(GLOBE_WSI_SELECTION STREQUAL "XLIB")
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for globe build but not building Xlib support" )
        endif()
        set(GLOBE_INCLUDE_DIRS
            ${X11_INCLUDE_DIR}
            ${GLOBE_INCLUDE_DIRS}
        )
        link_libraries(${X11_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    elseif(GLOBE_WSI_SELECTION STREQUAL "WAYLAND")
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for globe build but not building Wayland support" )
        endif()
        set(GLOBE_INCLUDE_DIRS
            ${WAYLAND_CLIENT_INCLUDE_DIR}
            ${GLOBE_INCLUDE_DIRS}
        )
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    elseif(GLOBE_WSI_SELECTION STREQUAL "MIR")
        if (NOT BUILD_WSI_MIR_SUPPORT)
            message( FATAL_ERROR "Selected MIR for globe build but not building Mir support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        set(GLOBE_INCLUDE_DIRS
            ${MIR_INCLUDE_DIR}
            ${GLOBE_INCLUDE_DIRS}
        )
    elseif(GLOBE_WSI_SELECTION STREQUAL "DISPLAY")
        add_definitions(-DVK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message( FATAL_ERROR "Unrecognized value for GLOBE_WSI_SELECTION: ${GLOBE_WSI_SELECTION}" )
     endif()

    link_libraries(${API_LOWERCASE} m)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

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
    ${GLOBE_INCLUDE_DIRS}
    )

file(GLOB RESOURCES
  "${PROJECT_SOURCE_DIR}/resources/*"
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(COPY ${RESOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)

# Define macro used for building shader SPIR-V output
macro(generate_globe_spv input output)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${output}
        COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${output} ${PROJECT_SOURCE_DIR}/resources/shaders/source/${input}
        DEPENDS ${GLSLANG_VALIDATOR} ${PROJECT_SOURCE_DIR}/resources/shaders/source/${input}
        COMMENT "Converting ${PROJECT_SOURCE_DIR}/resources/shaders/source/${input} to ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${output}"
    )
endmacro()

file(GLOB VERTEX_SHADER_FILES "${PROJECT_SOURCE_DIR}/resources/shaders/source/*.vert")
foreach(CUR_VERTEX_SHADER_FILE ${VERTEX_SHADER_FILES})
    get_filename_component(VERTEX_SHADER_GLSL ${CUR_VERTEX_SHADER_FILE} NAME_WE)
    STRING(REPLACE "_glsl" "-vs" VERTEX_SHADER_SPIRV ${VERTEX_SHADER_GLSL} )
    generate_globe_spv(${VERTEX_SHADER_GLSL}.vert ${VERTEX_SHADER_SPIRV}.spv)
    set(SHADER_FILES
        ${SHADER_FILES}
        ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${VERTEX_SHADER_SPIRV}.spv
       )
endforeach(CUR_VERTEX_SHADER_FILE)

file(GLOB FRAGMENT_SHADER_FILES "${PROJECT_SOURCE_DIR}/resources/shaders/source/*.frag")
foreach(CUR_FRAGMENT_SHADER_FILE ${FRAGMENT_SHADER_FILES})
    get_filename_component(FRAGMENT_SHADER_GLSL ${CUR_FRAGMENT_SHADER_FILE} NAME_WE)
    STRING(REPLACE "_glsl" "-fs" FRAGMENT_SHADER_SPIRV ${FRAGMENT_SHADER_GLSL} )
    generate_globe_spv(${FRAGMENT_SHADER_GLSL}.frag ${FRAGMENT_SHADER_SPIRV}.spv)
    set(SHADER_FILES
        ${SHADER_FILES}
        ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/${FRAGMENT_SHADER_SPIRV}.spv
       )
endforeach(CUR_FRAGMENT_SHADER_FILE)

######################################################################################
# globe

if(APPLE)
elseif(NOT WIN32)
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(globe_cube globe_cube.cpp ${SHADER_FILES})
        target_link_libraries(globe_cube ${LIBVK} globe)

        add_executable(01_first_triangle 01_first_triangle.cpp ${SHADER_FILES})
        target_link_libraries(01_first_triangle ${LIBVK} globe)

        add_executable(02_dynamic_uniform_buffer 02_dynamic_uniform_buffer.cpp ${SHADER_FILES})
        target_link_libraries(02_dynamic_uniform_buffer ${LIBVK} globe)
    endif()
else()
    if (CMAKE_CL_64)
        set (LIB_DIR "Win64")
    else()
        set (LIB_DIR "Win32")
    endif()

    add_executable(globe_cube WIN32 globe_cube.cpp ${SHADER_FILES})
    target_link_libraries(globe_cube ${LIBVK} globe)

    add_executable(01_first_triangle WIN32 01_first_triangle.cpp ${SHADER_FILES})
    target_link_libraries(01_first_triangle ${LIBVK} globe)

    add_executable(02_dynamic_uniform_buffer WIN32 02_dynamic_uniform_buffer.cpp ${SHADER_FILES})
    target_link_libraries(02_dynamic_uniform_buffer ${LIBVK} globe)
endif()

