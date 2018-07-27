/*
 * LunarGlobe - globe_app.hpp
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

class GlobeTexture;
class GlobeShader;

class GlobeResourceManager {
   public:
    GlobeResourceManager(const GlobeApp* app, const std::string& directory);
    ~GlobeResourceManager();

    GlobeTexture* LoadTexture(const std::string& texture_name, VkCommandBuffer command_buffer);
    void FreeTexture(GlobeTexture* texture);
    void FreeAllTextures();
    GlobeShader* LoadShader(const std::string& shader_prefix);
    void FreeShader(GlobeShader* shader);
    void FreeAllShaders();

    bool AllocateDeviceBufferMemory(VkBuffer vk_buffer, VkMemoryPropertyFlags vk_memory_properties,
                                    VkDeviceMemory& vk_device_memory, VkDeviceSize& vk_allocated_size) const;
    bool AllocateDeviceImageMemory(VkImage vk_image, VkMemoryPropertyFlags vk_memory_properties, VkDeviceMemory& vk_device_memory,
                                   VkDeviceSize& vk_allocated_size) const;
    void FreeDeviceMemory(VkDeviceMemory& vk_device_memory) const;

    bool UseStagingBuffer() const { return _uses_staging_buffer; }
    VkFormatProperties GetVkFormatProperties(VkFormat format) const;

   private:
    bool SelectMemoryTypeUsingRequirements(VkMemoryRequirements requirements, VkFlags required_flags, uint32_t& type) const;

    VkInstance _vk_instance;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    VkPhysicalDeviceMemoryProperties _vk_physical_device_memory_properties;
    bool _uses_staging_buffer;
    std::string _base_directory;
    std::vector<GlobeTexture*> _textures;
    std::vector<GlobeShader*> _shaders;
};
