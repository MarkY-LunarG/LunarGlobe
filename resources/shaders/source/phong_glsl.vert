//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    phong_glsl.frag
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform A {
    mat4 model_matrix;
} push_constant_block;

layout(binding = 0) uniform B {
    mat4 projection;
    mat4 view;
    vec4 light_position;
    vec4 light_color;
} uniform_buf;

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_normal;
layout (location = 2) in vec4 in_diffuse;
layout (location = 3) in vec4 in_ambient;
layout (location = 4) in vec4 in_specular;
layout (location = 5) in vec4 in_emissive;
layout (location = 6) in vec4 in_shininess;

layout (location = 0) out vec4 light_direction;
layout (location = 1) out vec4 out_eye_dir;
layout (location = 2) out vec4 out_normal;
layout (location = 3) out vec4 out_reflect;
layout (location = 4) out vec4 out_diffuse;
layout (location = 5) out vec4 out_ambient_emissive;
layout (location = 6) out vec4 out_specular;
layout (location = 7) out vec4 out_shininess;

void main() 
{
    // Calculate vertex position first
    mat4 model_view = uniform_buf.view * push_constant_block.model_matrix;
    vec4 view_position = model_view * in_position;
    gl_Position = uniform_buf.projection * view_position;
    out_eye_dir = normalize(-view_position);

    // Now work out the modified normal
    mat3 normal_mat = transpose(inverse(mat3(model_view)));
    out_normal = vec4(normalize(normal_mat * in_normal.xyz), 1.0);

    // Calculate the surface to light vector
    vec4 light_pos = uniform_buf.view * uniform_buf.light_position;
    vec3 light_vec = light_pos.xyz - view_position.xyz;
    light_direction = vec4(normalize(light_vec), 1.0);

    // Determine a reflection vector
    out_reflect = vec4(reflect(-light_direction.xyz, out_normal.xyz), 1.0);

    out_ambient_emissive = (in_ambient * uniform_buf.light_color) + in_emissive;
    out_diffuse          = in_diffuse * uniform_buf.light_color;
    out_specular         = in_specular * uniform_buf.light_color;
    out_shininess        = in_shininess;
}
