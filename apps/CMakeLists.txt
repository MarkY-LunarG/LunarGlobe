######################################################################################
# resourcess

file(GLOB RESOURCES
  "${PROJECT_SOURCE_DIR}/resources/*"
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(COPY ${RESOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)


######################################################################################
# shaders

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
# applications


add_executable(globe_cube "")
target_sources(globe_cube
               PRIVATE
                   globe_cube.cpp
                   ${SHADER_FILES}
              )
target_include_directories(globe_cube
                           PUBLIC
                               ${PROJECT_SOURCE_DIR}
                          )
target_link_libraries(globe_cube
                      PUBLIC
                          globe
                      )

add_executable(01_first_triangle "")
target_sources(01_first_triangle
               PRIVATE
                   01_first_triangle.cpp
                   ${SHADER_FILES}
              )
target_include_directories(01_first_triangle
                           PUBLIC
                               ${PROJECT_SOURCE_DIR}
                          )
target_link_libraries(01_first_triangle
                      PUBLIC
                          globe
                      )

add_executable(02_dynamic_uniform_buffer "")
target_sources(02_dynamic_uniform_buffer
               PRIVATE
                   02_dynamic_uniform_buffer.cpp
                   ${SHADER_FILES}
              )
target_include_directories(02_dynamic_uniform_buffer
                           PUBLIC
                               ${PROJECT_SOURCE_DIR}
                          )
target_link_libraries(02_dynamic_uniform_buffer
                      PUBLIC
                          globe
                      )

add_executable(03_multi_texture "")
target_sources(03_multi_texture
               PRIVATE
                   03_multi_texture.cpp
                   ${SHADER_FILES}
              )
target_include_directories(03_multi_texture
                           PUBLIC
                               ${PROJECT_SOURCE_DIR}
                          )
target_link_libraries(03_multi_texture
                      PUBLIC
                          globe
                      )
