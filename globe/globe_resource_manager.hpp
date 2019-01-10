//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_resource_manager.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

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

    GlobeTexture* LoadTexture(const std::string& texture_name, VkCommandBuffer vk_command_buffer);
    GlobeTexture* CreateRenderTargetTexture(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                            VkFormat vk_format);
    void FreeTexture(GlobeTexture* texture);
    void FreeAllTextures();
    GlobeShader* LoadShader(const std::string& shader_prefix);
    void FreeShader(GlobeShader* shader);
    void FreeAllShaders();

    bool AllocateDeviceBufferMemory(VkBuffer vk_buffer, VkMemoryPropertyFlags vk_memory_properties,
                                    VkDeviceMemory& vk_device_memory, VkDeviceSize& vk_allocated_size) const;
    bool AllocateDeviceImageMemory(VkImage vk_image, VkMemoryPropertyFlags vk_memory_properties,
                                   VkDeviceMemory& vk_device_memory, VkDeviceSize& vk_allocated_size) const;
    void FreeDeviceMemory(VkDeviceMemory& vk_device_memory) const;

    bool UseStagingBuffer() const { return _uses_staging_buffer; }
    VkFormatProperties GetVkFormatProperties(VkFormat format) const;

   private:
    bool SelectMemoryTypeUsingRequirements(VkMemoryRequirements requirements, VkFlags required_flags,
                                           uint32_t& type) const;

    VkInstance _vk_instance;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    VkPhysicalDeviceMemoryProperties _vk_physical_device_memory_properties;
    bool _uses_staging_buffer;
    std::string _base_directory;
    std::vector<GlobeTexture*> _textures;
    std::vector<GlobeShader*> _shaders;
};
