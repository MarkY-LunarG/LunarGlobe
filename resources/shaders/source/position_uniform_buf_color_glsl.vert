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

layout(std140, binding = 0) uniform buf {
    vec4 color_1;
    vec4 color_2;
    uint second_color_index;
} uniform_buf;

layout (location = 0) in vec4 in_position;
layout (location = 0) out vec4 out_color;

void main() 
{
    if (gl_VertexIndex > uniform_buf.second_color_index) {
        out_color = uniform_buf.color_1;
    } else {
        out_color = uniform_buf.color_2;
    }
    gl_Position = in_position;
}
