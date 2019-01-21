//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_font.hpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"
#include "globe_texture.hpp"
#include "globe_basic_types.hpp"
#include "globe_glm_include.hpp"

#define GLOBE_FONT_STARTING_ASCII_CHAR 32
#define GLOBE_FONT_ENDING_ASCII_CHAR 126

struct GlobeFontCharData {
    float width;
    float left_u;
    float top_v;
    float right_u;
    float bottom_v;
};

struct GlobeFontData {
    GlobeTextureData texture_data;
    float generated_size;
    std::vector<GlobeFontCharData> char_data;
};

struct GlobeFontStringData {
    std::string text_string;
    glm::vec3 starting_pos;
    uint32_t queue_family_index;
    uint32_t num_vertices;
    uint32_t num_indices;
    std::vector<float> vertex_data;
    GlobeVulkanBuffer vertex_buffer;
    std::vector<uint32_t> index_data;
    GlobeVulkanBuffer index_buffer;
};

class GlobeFont : public GlobeTexture {
   public:
    static GlobeFont* LoadFontMap(GlobeResourceManager* resource_manager, VkDevice vk_device,
                                  VkCommandBuffer vk_command_buffer, uint32_t character_pixel_size,
                                  const std::string& font_name, const std::string& directory);

    GlobeFont(GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& font_name,
              GlobeFontData* font_data);
    ~GlobeFont();

    bool LoadIntoRenderPass(VkRenderPass render_pass, float viewport_width, float viewport_height);
    void UnloadFromRenderPass();
    int32_t AddString(const std::string& text_string, std::vector<glm::vec3> colors, glm::vec3 starting_pos,
                      glm::vec3 text_direction, glm::vec3 text_up, float model_space_char_height,
                      uint32_t queue_family_index);
    bool UpdateStringText(int32_t string_index, const std::string& text_string);
    void RemoveString(int32_t string_index);
    void RemoveAllStrings();
    void DrawString(VkCommandBuffer command_buffer, glm::mat4 mvp, uint32_t string_index);
    void DrawStrings(VkCommandBuffer command_buffer, glm::mat4 mvp);

   private:
    static GlobeFont* GenerateFont(GlobeResourceManager* resource_manager, VkDevice vk_device,
                                   VkCommandBuffer vk_command_buffer, const std::string& font_name,
                                   GlobeFontData& font_data, VkFormat format, VkFormatProperties format_props,
                                   std::vector<uint8_t>& font_bitmap);

    std::string _font_name;
    float _generated_size;
    std::vector<GlobeFontCharData> _char_data;
    std::vector<GlobeFontStringData> _string_data;
    VkDescriptorSetLayout _vk_descriptor_set_layout;
    VkPipelineLayout _vk_pipeline_layout;
    VkDescriptorPool _vk_descriptor_pool;
    VkDescriptorSet _vk_descriptor_set;
    VkPipeline _vk_pipeline;
};
