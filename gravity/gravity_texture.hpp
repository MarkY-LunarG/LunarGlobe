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

struct GravityTextureData {
    uint32_t width;
    uint32_t height;
    VkFormat vk_format;
    VkSampler vk_sampler;
    VkImage vk_image;
    VkImageLayout vk_image_layout;
    VkMemoryAllocateInfo vk_mem_alloc_info;
    VkDeviceMemory vk_device_memory;
    VkImageView vk_image_view;
    std::vector<uint8_t> raw_data;
    GravityTextureData* staging_texture_data;
};

class GravityApp;

class GravityTexture {
   public:
    static GravityTexture* LoadFromFile(const GravityApp* app, VkCommandBuffer command_buffer, const std::string& texture_name,
                                        const std::string& directory);

    GravityTexture(VkPhysicalDevice vk_phys_device, VkDevice vk_device, const std::string& texture_name,
                   GravityTextureData* texture_data);
    ~GravityTexture();

    uint32_t Width() { return _width; }
    uint32_t Height() { return _height; }
    bool UsesStagingTexture() { return _uses_staging_texture; }
    GravityTexture* StagingTexture() { return _staging_texture; }
    bool DeleteStagingTexture();
    VkFormat GetVkFormat() { return _vk_format; }
    VkSampler GetVkSampler() { return _vk_sampler; }
    VkImage GetVkImage() { return _vk_image; }
    VkImageView GetVkImageView() { return _vk_image_view; }
    VkImageLayout GetVkImageLayout() { return _vk_image_layout; }

   private:
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    std::string _texture_name;
    bool _uses_staging_texture;
    GravityTexture* _staging_texture;
    uint32_t _width;
    uint32_t _height;
    VkFormat _vk_format;
    VkSampler _vk_sampler;
    VkImage _vk_image;
    VkImageLayout _vk_image_layout;
    VkMemoryAllocateInfo _vk_mem_alloc_info;
    VkDeviceMemory _vk_device_memory;
    VkImageView _vk_image_view;
};
