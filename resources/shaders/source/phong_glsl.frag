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

layout (location = 0) in vec4 interp_light_dir;
layout (location = 1) in vec4 interp_eye_dir;
layout (location = 2) in vec4 interp_normal;
layout (location = 3) in vec4 interp_reflect;
layout (location = 4) in vec4 interp_diffuse;
layout (location = 5) in vec4 interp_ambient_emissive;
layout (location = 6) in vec4 interp_specular;
layout (location = 7) in vec4 interp_shininess;

layout (location = 0) out vec4 out_color;

void main() {
    vec3  normal           = normalize(interp_normal.xyz);
    vec3  reflected_light  = normalize(interp_reflect.xyz);
    vec3  light_dir        = normalize(interp_light_dir.xyz);
    vec3  eye_dir          = normalize(interp_eye_dir.xyz);
    float light_dot_normal = max(dot(normal, light_dir), 0.0);
    vec4  diffuse_comp     = vec4(0.0, 0.0, 0.0, 0.0);
    vec4  specular_comp    = vec4(0.0, 0.0, 0.0, 0.0);

    // Only calculate diffuse and specular lighting portions
    // if the surface is even remotely facing the light
    if (light_dot_normal > 0.0) {
        diffuse_comp = interp_diffuse * light_dot_normal;

        float specular_angle = max(dot(reflected_light, eye_dir), 0.0);
        float specular_mult  = pow(specular_angle, interp_shininess.x);
        specular_comp = interp_specular * specular_mult;
    }

    out_color = interp_ambient_emissive + diffuse_comp + specular_comp;
}
