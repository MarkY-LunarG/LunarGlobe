/*
 * Copyright (c) 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform push_block {
    mat4 model_matrix;
} push_constant_block;

layout(std140, binding = 0) uniform uni_buf {
    mat4 projection;
    mat4 view;
} uniform_buf;

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_tex_coord;

layout (location = 0) out vec2 out_tex_coord;

void main() 
{
   out_tex_coord = in_tex_coord.st;
   gl_Position = uniform_buf.projection * uniform_buf.view * push_constant_block.model_matrix * in_position;
}
