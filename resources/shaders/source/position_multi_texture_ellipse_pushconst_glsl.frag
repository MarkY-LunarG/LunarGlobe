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

layout(push_constant) uniform push_block {
    int   selection;
    float radius_x_sqd;
    float radius_y_sqd;
} push_constant_block;

layout(std140, binding = 0) uniform uni_buf {
    vec4 ellipse_center;
} uniform_buf;

layout (location = 0) in vec2 tex_coord;
layout (location = 0) out vec4 uFragColor;

void main() {
    float x_val = tex_coord.s - uniform_buf.ellipse_center.x;
    float x_val_sqd = x_val * x_val;
    float y_val = tex_coord.t - uniform_buf.ellipse_center.y;
    float y_val_sqd = y_val * y_val;
    float test_val = (x_val_sqd / push_constant_block.radius_x_sqd) + (y_val_sqd / push_constant_block.radius_y_sqd);

    vec4 result_color = texture(tex1_sampler, tex_coord.xy);
    if (push_constant_block.selection == 0) {
        if (test_val < 1) {
            result_color = texture(tex2_sampler, tex_coord.xy);
        }
    } else if (push_constant_block.selection == 1) {
        if (test_val >= 1) {
            result_color = texture(tex2_sampler, tex_coord.xy);
        }
    } else if (push_constant_block.selection == 2) {
        if (test_val < 1.2 && test_val > 0.8) {
            float modifier1 = (test_val - 0.8) * 2.5;
            float modifier2 = (1.2 - test_val) * 2.5;
            result_color *= modifier1;
            result_color += (modifier2 * texture(tex2_sampler, tex_coord.xy));
        } else if (test_val <= 0.8) {
            result_color = texture(tex2_sampler, tex_coord.xy);
        }
    } else {
        if (test_val < 1.2 && test_val > 0.8) {
            float modifier1 = (test_val - 0.8) * 2.5;
            float modifier2 = (1.2 - test_val) * 2.5;
            result_color *= modifier2;
            result_color += (modifier1 * texture(tex2_sampler, tex_coord.xy));
        } else if (test_val <= 0.8) {
            result_color = texture(tex1_sampler, tex_coord.xy);
        } else {
            result_color = texture(tex2_sampler, tex_coord.xy);
        }
    }
    uFragColor = result_color;
}