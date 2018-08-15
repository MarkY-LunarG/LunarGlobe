
######################################################################################
# Resources

file(GLOB RESOURCES
  "${PROJECT_SOURCE_DIR}/resources/*"
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(COPY ${RESOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)

######################################################################################
# Samples

SINGLE_SOURCE_EXECUTABLE(01_first_triangle)
SINGLE_SOURCE_EXECUTABLE(02_dynamic_uniform_buffer)
SINGLE_SOURCE_EXECUTABLE(03_multi_texture)
SINGLE_SOURCE_EXECUTABLE(04_push_constants)
