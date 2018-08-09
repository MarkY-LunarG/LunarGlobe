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

struct GlobeTextureData {
    uint32_t width;
    uint32_t height;
    uint32_t num_components;
    VkFormat vk_format;
    VkFormatProperties vk_format_props;
    VkSampler vk_sampler;
    VkImage vk_image;
    VkImageLayout vk_image_layout;
    VkDeviceMemory vk_device_memory;
    VkDeviceSize vk_allocated_size;
    VkImageView vk_image_view;
    std::vector<uint8_t> raw_data;
    GlobeTextureData* staging_texture_data;
};

class GlobeResourceManager;

class GlobeTexture {
   public:
    static GlobeTexture* LoadFromFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                        VkCommandBuffer vk_command_buffer, const std::string& texture_name,
                                        const std::string& directory);

    GlobeTexture(const GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& texture_name,
                   GlobeTextureData* texture_data);
    ~GlobeTexture();

    uint32_t Width() { return _width; }
    uint32_t Height() { return _height; }
    bool UsesStagingTexture() { return _uses_staging_texture; }
    GlobeTexture* StagingTexture() { return _staging_texture; }
    bool DeleteStagingTexture();
    VkFormat GetVkFormat() { return _vk_format; }
    VkSampler GetVkSampler() { return _vk_sampler; }
    VkImage GetVkImage() { return _vk_image; }
    VkImageView GetVkImageView() { return _vk_image_view; }
    VkImageLayout GetVkImageLayout() { return _vk_image_layout; }

   private:
    const GlobeResourceManager* _globe_resource_mgr;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    std::string _texture_name;
    bool _uses_staging_texture;
    GlobeTexture* _staging_texture;
    uint32_t _width;
    uint32_t _height;
    VkFormat _vk_format;
    VkSampler _vk_sampler;
    VkImage _vk_image;
    VkImageLayout _vk_image_layout;
    VkDeviceMemory _vk_device_memory;
    VkDeviceSize _vk_allocated_size;
    VkImageView _vk_image_view;
};
