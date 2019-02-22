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
#include "globe_basic_types.hpp"

class GlobeApp;
class GlobeTexture;
class GlobeFont;
class GlobeShader;
class GlobeModel;

class GlobeResourceManager {
   public:
    GlobeResourceManager(const GlobeApp* app, const std::string& directory, uint32_t queue_family_index);
    ~GlobeResourceManager();

    GlobeTexture* LoadTexture(const std::string& texture_name, VkCommandBuffer vk_command_buffer,
                              bool generate_mipmaps);
    GlobeTexture* CreateRenderTargetTexture(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                            VkFormat vk_format);
    void FreeTexture(GlobeTexture* texture);
    void FreeAllTextures();
    bool InsertImageLayoutTransitionBarrier(VkCommandBuffer command_vk_buffer, VkImage vk_image,
                                            VkImageSubresourceRange vk_subresource_range,
                                            VkPipelineStageFlags vk_starting_stage, VkImageLayout vk_starting_layout,
                                            VkPipelineStageFlags vk_target_stage, VkImageLayout vk_target_layout);

    GlobeFont* LoadFontMap(const std::string& font_name, VkCommandBuffer vk_command_buffer, uint32_t font_size);
    void FreeFont(GlobeFont* font);
    void FreeAllFonts();

    GlobeShader* LoadShader(const std::string& shader_prefix);
    void FreeShader(GlobeShader* shader);
    void FreeAllShaders();

    GlobeModel* LoadModel(const std::string& sub_dir, const std::string& model_name, const GlobeComponentSizes& sizes);
    void FreeModel(GlobeModel* model);
    void FreeAllModels();

    bool AllocateDeviceBufferMemory(VkBuffer vk_buffer, VkMemoryPropertyFlags vk_memory_properties,
                                    VkDeviceMemory& vk_device_memory, VkDeviceSize& vk_allocated_size) const;
    bool AllocateDeviceImageMemory(VkImage vk_image, VkMemoryPropertyFlags vk_memory_properties,
                                   VkDeviceMemory& vk_device_memory, VkDeviceSize& vk_allocated_size) const;
    void FreeDeviceMemory(VkDeviceMemory& vk_device_memory) const;

    bool AllocateCommandBuffer(VkCommandBufferLevel level, VkCommandBuffer& command_buffer);
    bool FreeCommandBuffer(VkCommandBuffer& command_buffer);

    bool UseStagingBuffer() const { return _uses_staging_buffer; }
    VkFormatProperties GetVkFormatProperties(VkFormat format) const;

   private:
    bool SelectMemoryTypeUsingRequirements(VkMemoryRequirements requirements, VkFlags required_flags,
                                           uint32_t& type) const;

    const GlobeApp* _parent_app;
    VkInstance _vk_instance;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    VkPhysicalDeviceMemoryProperties _vk_physical_device_memory_properties;
    bool _uses_staging_buffer;
    std::string _base_directory;
    std::vector<GlobeTexture*> _textures;
    std::vector<GlobeFont*> _fonts;
    std::vector<GlobeShader*> _shaders;
    std::vector<GlobeModel*> _models;
    VkCommandPool _vk_cmd_pool;
    std::vector<VkCommandBuffer> _targeted_vk_cmd_buffers;
};
