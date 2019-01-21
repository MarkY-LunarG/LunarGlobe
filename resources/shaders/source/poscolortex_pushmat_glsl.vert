//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_texture.cpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform push_block {
    mat4 mvp;
} push_constant_block;

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec4 in_tex_coord;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_tex_coord;

void main() 
{
    out_color = in_color;
    out_tex_coord = in_tex_coord.xy;
    gl_Position = push_constant_block.mvp * in_position;
}
