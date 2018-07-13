/*
 * LunarGravity - gravity_app.hpp
 *
 * Copyright (C) 2018 LunarG, Inc.
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
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#include "gravity_event.hpp"
#include "gravity_submit_manager.hpp"
#include "gravity_shader.hpp"
#include "gravity_texture.hpp"
#include "gravity_resource_manager.hpp"
#include "gravity_app.hpp"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
const char directory_symbol = '\\';
#else
const char directory_symbol = '/';
#endif

GravityResourceManager::~GravityResourceManager() {
    FreeAllTextures();
    FreeAllShaders();
}

GravityTexture* GravityResourceManager::LoadTexture(const std::string& texture_name, VkCommandBuffer command_buffer) {
    std::string texture_dir = _base_directory;
    texture_dir += directory_symbol;
    texture_dir += "textures";
    texture_dir += directory_symbol;
    GravityTexture* texture = GravityTexture::LoadFromFile(_app, command_buffer, texture_name, texture_dir);
    if (nullptr != texture) {
        _textures.push_back(texture);
    }
    return texture;
}

GravityShader* GravityResourceManager::LoadShader(const std::string& shader_prefix) {
    VkInstance vk_instance = VK_NULL_HANDLE;
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
    VkDevice vk_device = VK_NULL_HANDLE;
    _app->GetVkInfo(vk_instance, vk_physical_device, vk_device);

    std::string shader_dir = _base_directory;
    shader_dir += directory_symbol;
    shader_dir += "shaders";
    shader_dir += directory_symbol;
    GravityShader* shader = GravityShader::LoadFromFile(vk_device, shader_prefix, shader_dir);
    if (nullptr != shader) {
        _shaders.push_back(shader);
    }
    return shader;
}

void GravityResourceManager::FreeAllTextures() {
    for (auto texture : _textures) {
        delete texture;
    }
    _textures.clear();
}

void GravityResourceManager::FreeTexture(GravityTexture* texture) {
    for (uint32_t tex_index = 0; tex_index < _textures.size(); ++tex_index) {
        if (_textures[tex_index] == texture) {
            _textures.erase(_textures.begin() + tex_index);
        }
    }
}

void GravityResourceManager::FreeAllShaders() {
    for (auto shader : _shaders) {
        delete shader;
    }
    _shaders.clear();
}

void GravityResourceManager::FreeShader(GravityShader* shader) {
    for (uint32_t shd_index = 0; shd_index < _shaders.size(); ++shd_index) {
        if (_shaders[shd_index] == shader) {
            _shaders.erase(_shaders.begin() + shd_index);
        }
    }
}
