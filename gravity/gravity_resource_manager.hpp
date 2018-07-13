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

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

class GravityTexture;
class GravityShader;

class GravityResourceManager {
  public:
    GravityResourceManager(const GravityApp* app, const std::string& directory) { _app = app; _base_directory = directory; }
    ~GravityResourceManager();

    GravityTexture* LoadTexture(const std::string& texture_name, VkCommandBuffer command_buffer);
    void FreeTexture(GravityTexture* texture);
    void FreeAllTextures();
    GravityShader* LoadShader(const std::string& shader_prefix);
    void FreeShader(GravityShader* shader);
    void FreeAllShaders();

  private:
    const GravityApp* _app;
    std::string _base_directory;
    std::vector<GravityTexture*> _textures;
    std::vector<GravityShader*> _shaders;
};
