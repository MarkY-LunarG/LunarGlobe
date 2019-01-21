//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_basic_types.hpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

struct GlobeComponentSizes {
    uint8_t position;
    uint8_t normal;
    uint8_t diffuse_color;
    uint8_t ambient_color;
    uint8_t specular_color;
    uint8_t emissive_color;
    uint8_t shininess;
    uint8_t texcoord[3];
    uint8_t tangent;
    uint8_t bitangent;
};

struct GlobeVulkanBuffer {
    VkBuffer vk_buffer;
    VkDeviceMemory vk_memory;
    VkDeviceSize vk_size;
};
