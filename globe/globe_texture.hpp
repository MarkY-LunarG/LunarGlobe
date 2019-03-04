//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_texture.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

#include <gli/gli.hpp>

struct GlobeTextureLevel {
    uint32_t width;
    uint32_t height;
    uint32_t data_size;
    uint32_t offset;
};

struct GlobeStandardTextureData {
    std::vector<uint8_t> raw_data;
    std::vector<GlobeTextureLevel> levels;
};

struct GlobeTextureData {
    bool setup_for_render_target;
    bool is_color;
    bool is_depth;
    bool is_stencil;
    uint32_t width;
    uint32_t height;
    uint32_t num_mip_levels;
    VkSampleCountFlagBits vk_sample_count;
    VkFormat vk_format;
    VkFormatProperties vk_format_props;
    VkSampler vk_sampler;
    VkImage vk_image;
    VkImageLayout vk_image_layout;
    VkDeviceMemory vk_device_memory;
    VkDeviceSize vk_allocated_size;
    VkImageView vk_image_view;
    bool uses_standard_data;
    union {
        GlobeStandardTextureData* standard_data;
        gli::texture2d* gli_texture_2d;
    };
};

class GlobeResourceManager;
class GlobeSubmitManager;

class GlobeTexture {
   public:
    static GlobeTexture* LoadFromStandardFile(GlobeResourceManager* resource_manager,
                                              GlobeSubmitManager* submit_manager, VkDevice vk_device,
                                              bool generate_mipmaps, const std::string& texture_name,
                                              const std::string& directory);
    static GlobeTexture* LoadFromKtxFile(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                         VkDevice vk_device, bool generate_mipmaps, const std::string& texture_name,
                                         const std::string& directory);
    static bool InitFromContent(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                VkDevice vk_device, const std::string& texture_name, GlobeTextureData& texture_data);
    static GlobeTexture* CreateRenderTarget(GlobeResourceManager* resource_manager, VkDevice vk_device, uint32_t width,
                                            uint32_t height, VkFormat vk_format);

    GlobeTexture(GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& texture_name,
                 GlobeTextureData* texture_data);
    ~GlobeTexture();

    uint32_t NextPowerOfTwo(uint32_t value);

    uint32_t Width() { return _width; }
    uint32_t Height() { return _height; }
    VkFormat GetVkFormat() { return _vk_format; }
    VkSampleCountFlagBits GetVkSampleCount() { return _vk_sample_count; }
    VkSampler GetVkSampler() { return _vk_sampler; }
    VkImage GetVkImage() { return _vk_image; }
    VkImageView GetVkImageView() { return _vk_image_view; }
    VkImageLayout GetVkImageLayout() { return _vk_image_layout; }
    VkAttachmentDescription GenVkAttachmentDescription();
    VkAttachmentReference GenVkAttachmentReference(uint32_t attachment_index);

   protected:
    bool _setup_for_render_target;
    bool _is_color;
    bool _is_depth;
    bool _is_stencil;
    bool _has_mipmaps;
    GlobeResourceManager* _globe_resource_mgr;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    std::string _texture_name;
    uint32_t _width;
    uint32_t _height;
    uint32_t _num_mip_levels;
    VkSampleCountFlagBits _vk_sample_count;
    VkFormat _vk_format;
    VkSampler _vk_sampler;
    VkImage _vk_image;
    VkImageLayout _vk_image_layout;
    VkDeviceMemory _vk_device_memory;
    VkDeviceSize _vk_allocated_size;
    VkImageView _vk_image_view;
};
