//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_overlay.hpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include "globe_resource_manager.hpp"
#include "globe_submit_manager.hpp"
#include "globe_font.hpp"
#include "globe_overlay.hpp"

GlobeOverlay::GlobeOverlay(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                           VkDevice vk_device)
    : _resource_mgr(resource_manager),
      _submit_mgr(submit_manager),
      _vk_device(vk_device),
      _vk_render_pass(VK_NULL_HANDLE) {}

GlobeOverlay::~GlobeOverlay() {
    for (const auto font_element : _fonts) {
        font_element.second->UnloadFromRenderPass();
        _resource_mgr->FreeFont(font_element.second);
    }
}

void GlobeOverlay::UpdateViewport(float viewport_width, float viewport_height) {
    _viewport_width = viewport_width;
    _viewport_height = viewport_height;
}

bool GlobeOverlay::SetRenderPass(VkRenderPass render_pass) {
    if (VK_NULL_HANDLE != _vk_render_pass || VK_NULL_HANDLE == render_pass) {
        for (const auto font_element : _fonts) {
            font_element.second->UnloadFromRenderPass();
        }
    }
    _vk_render_pass = render_pass;
    if (VK_NULL_HANDLE != render_pass) {
        for (const auto font_element : _fonts) {
            font_element.second->LoadIntoRenderPass(_vk_render_pass, _viewport_width, _viewport_height);
        }
    }
    return true;
}

bool GlobeOverlay::LoadFont(const std::string& font_name, float max_height) {
    auto font_present = _fonts.find(font_name);
    if (font_present == _fonts.end()) {
        _fonts[font_name] = _resource_mgr->LoadFontMap(font_name, max_height);
        if (VK_NULL_HANDLE != _vk_render_pass) {
            _fonts[font_name]->LoadIntoRenderPass(_vk_render_pass, _viewport_width, _viewport_height);
        }
    } else if (font_present->second->Size() < max_height) {
        _resource_mgr->FreeFont(font_present->second);
        _fonts[font_name] = _resource_mgr->LoadFontMap(font_name, max_height);
    }
    return (_fonts[font_name] != nullptr);
}

int32_t GlobeOverlay::AddScreenSpaceStaticText(const std::string& font_name, float font_height, float x, float y,
                                               const glm::vec3& fg_color, const glm::vec4& bg_color,
                                               const std::string& text) {
    auto font_present = _fonts.find(font_name);
    if (font_present == _fonts.end()) {
        return -1;
    }
    glm::vec3 starting_pos(x, y, 0.f);
    glm::vec3 text_dir(1.f, 0.f, 0.f);
    glm::vec3 up_dir(0.f, -1.f, 0.f);
    float text_height = font_height / _viewport_height;
    return _fonts[font_name]->AddStaticString(text, fg_color, bg_color, starting_pos, text_dir, up_dir, text_height,
                                              _submit_mgr->GetGraphicsQueueIndex());
}

int32_t GlobeOverlay::AddScreenSpaceDynamicText(const std::string& font_name, float font_height, float x, float y,
                                                const glm::vec3& fg_color, const glm::vec4& bg_color,
                                                const std::string& text, uint32_t copies) {
    auto font_present = _fonts.find(font_name);
    if (font_present == _fonts.end()) {
        return -1;
    }
    glm::vec3 starting_pos(x, y, 0.f);
    glm::vec3 text_dir(1.f, 0.f, 0.f);
    glm::vec3 up_dir(0.f, -1.f, 0.f);
    float text_height = font_height / _viewport_height;
    return _fonts[font_name]->AddDynamicString(text, fg_color, bg_color, starting_pos, text_dir, up_dir, text_height,
                                               _submit_mgr->GetGraphicsQueueIndex(), copies);
}

bool GlobeOverlay::UpdateDynamicText(const std::string& font_name, int32_t string_index, const std::string& text,
                                     uint32_t copy) {
    auto font_present = _fonts.find(font_name);
    if (font_present == _fonts.end()) {
        return false;
    }
    return font_present->second->UpdateStringText(string_index, text, copy);
}

bool GlobeOverlay::Draw(VkCommandBuffer command_buffer, uint32_t copy) {
    glm::mat4 identity(1.f);
    for (const auto font_element : _fonts) {
        font_element.second->DrawStrings(command_buffer, identity, copy);
    }
    return true;
}
