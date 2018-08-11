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

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
} my_uniform_buf;

layout (location = 0) out vec3 frag_color;

void main() 
{
   frag_color  = in_color;
   gl_Position = my_uniform_buf.mvp * vec4(in_position, 1.0);
}