#
# Project:                 LunarGlobe
# SPDX-License-Identifier: Apache-2.0
#
# File:                    samples/CMakeLists.txt
# Copyright(C):            2018-2019; LunarG, Inc.
# Author(s):               Mark Young <marky@lunarg.com>
#

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
SINGLE_SOURCE_EXECUTABLE(05_simple_glm)
SINGLE_SOURCE_EXECUTABLE(06_offscreen_rendering)
SINGLE_SOURCE_EXECUTABLE(07_simple_model_glm)
SINGLE_SOURCE_EXECUTABLE(08_simple_ttf)
