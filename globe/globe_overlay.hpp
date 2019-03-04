//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_overlay.hpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <unordered_map>

#include "globe_basic_types.hpp"
#include "globe_glm_include.hpp"
#include "globe_vulkan_headers.hpp"

class GlobeResourceManager;
class GlobeSubmitManager;
class GlobeFont;

class GlobeOverlay {
   public:
    GlobeOverlay(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager, VkDevice vk_device);
    ~GlobeOverlay();

    void UpdateViewport(float viewport_width, float viewport_height);
    bool SetRenderPass(VkRenderPass render_pass);
    bool LoadFont(const std::string& font_name, float max_height);
    int32_t AddScreenSpaceStaticText(const std::string& font_name, float font_height, float x, float y,
                                     const glm::vec3& fg_color, const glm::vec4& bg_color, const std::string& text);
    int32_t AddScreenSpaceDynamicText(const std::string& font_name, float font_height, float x, float y,
                                      const glm::vec3& fg_color, const glm::vec4& bg_color, const std::string& text,
                                      uint32_t copies);
    bool UpdateDynamicText(const std::string& font_name, int32_t string_index, const std::string& text,
                           uint32_t copy = 0);
    bool Draw(VkCommandBuffer command_buffer, uint32_t copy);

   private:
    GlobeResourceManager* _resource_mgr;
    GlobeSubmitManager* _submit_mgr;
    VkDevice _vk_device;
    VkRenderPass _vk_render_pass;
    float _viewport_width;
    float _viewport_height;
    std::unordered_map<std::string, GlobeFont*> _fonts;
};
