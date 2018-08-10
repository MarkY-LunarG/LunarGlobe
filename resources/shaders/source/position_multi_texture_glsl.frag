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
layout (binding = 1) uniform sampler2D tex1_sampler;
layout (binding = 2) uniform sampler2D tex2_sampler;

layout(std140, binding = 0) uniform buf {
    vec2 ellipse_center;
} my_uniform_buf;

layout (location = 0) in vec4 tex_coord;
layout (location = 0) out vec4 uFragColor;

void main() {
    float rad_x_sqd = 0.03;
    float rad_y_sqd = 0.12;
    float x_val = tex_coord.s - my_uniform_buf.ellipse_center.x;
    float x_val_sqd = x_val * x_val;
    float y_val = tex_coord.t - my_uniform_buf.ellipse_center.y;
    float y_val_sqd = y_val * y_val;
    float test_val = (x_val_sqd / rad_x_sqd) + (y_val_sqd / rad_y_sqd);
    if (test_val < 1) {
        uFragColor = texture(tex2_sampler, tex_coord.xy);
    } else {
        uFragColor = texture(tex1_sampler, tex_coord.xy);
    }
}