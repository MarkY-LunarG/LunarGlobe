//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_texture.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_submit_manager.hpp"
#include "globe_shader.hpp"
#include "globe_texture.hpp"
#include "globe_resource_manager.hpp"
#include "globe_app.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static bool LoadFile(const GlobeResourceManager* resource_manager, const std::string& filename,
                     GlobeTextureData& texture_data) {
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(__ANDROID__))
// filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename.c_str())].UTF8String;
#error("Unsupported platform")
#elif defined(__ANDROID__)
#else
    FILE* fPtr = fopen(filename.c_str(), "rb");
    if (nullptr == fPtr) {
        return false;
    }
    int32_t int_width = 0;
    int32_t int_height = 0;
    int32_t num_channels = 0;
    uint8_t* image_data = stbi_load_from_file(fPtr, &int_width, &int_height, &num_channels, 0);
    if (nullptr == image_data || int_width <= 0 || int_height <= 0 || num_channels <= 0) {
        if (nullptr != image_data) {
            stbi_image_free(image_data);
        }
        return false;
    }
    texture_data.setup_for_render_target = false;
    texture_data.width = static_cast<uint32_t>(int_width);
    texture_data.height = static_cast<uint32_t>(int_height);
    bool requires_padding = false;
    int32_t old_num_channels = num_channels;
    switch (num_channels) {
        case 1:
            texture_data.vk_format = VK_FORMAT_R8_UNORM;
            break;
        case 2:
            texture_data.vk_format = VK_FORMAT_R8G8_UNORM;
            break;
        case 3:
            texture_data.vk_format = VK_FORMAT_R8G8B8_UNORM;
            break;
        case 4:
            texture_data.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            break;
    }
    texture_data.vk_format_props = resource_manager->GetVkFormatProperties(texture_data.vk_format);
    if (0 == (texture_data.vk_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
        0 == (texture_data.vk_format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        texture_data.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
        requires_padding = true;
        num_channels = 4;
        texture_data.vk_format_props = resource_manager->GetVkFormatProperties(texture_data.vk_format);
    }
    texture_data.num_components = static_cast<uint32_t>(num_channels);
    texture_data.raw_data.resize(int_width * int_height * num_channels);

    if (requires_padding) {
        uint8_t* dst_ptr = reinterpret_cast<uint8_t*>(texture_data.raw_data.data());
        uint8_t* src_ptr = image_data;
        for (int32_t row = 0; row < int_height; ++row) {
            for (int32_t col = 0; col < int_width; ++col) {
                int32_t comp = 0;
                for (; comp < old_num_channels; ++comp) {
                    *dst_ptr++ = *src_ptr++;
                }
                while (comp++ < num_channels) {
                    if (comp == 4) {
                        *dst_ptr++ = 255;
                    } else {
                        *dst_ptr++ = 0;
                    }
                }
            }
        }
    } else {
        memcpy(texture_data.raw_data.data(), image_data, int_width * int_height * num_channels * sizeof(uint8_t));
    }

    stbi_image_free(image_data);
    fclose(fPtr);
#endif
    return true;
}

static bool TransitionVkImageLayout(VkCommandBuffer cmd_buf, VkImage image, VkImageAspectFlags aspect_mask,
                                    VkImageLayout old_image_layout, VkImageLayout new_image_layout,
                                    VkAccessFlagBits src_access_mask, VkPipelineStageFlags src_stages,
                                    VkPipelineStageFlags dest_stages) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    if (VK_NULL_HANDLE == new_image_layout) {
        logger.LogFatalError("TransitionVkImageLayout called with no created command buffer");
        return false;
    }

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange = {aspect_mask, 0, 1, 0, 1};

    switch (new_image_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            image_memory_barrier.dstAccessMask = 0;
            break;
    }

    vkCmdPipelineBarrier(cmd_buf, src_stages, dest_stages, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
    return true;
}

GlobeTexture* GlobeTexture::LoadFromFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                         VkCommandBuffer vk_command_buffer, const std::string& texture_name,
                                         const std::string& directory) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    GlobeTextureData texture_data = {};
    std::string texture_file_name = directory;
    texture_file_name += texture_name;

    if (!LoadFile(resource_manager, texture_file_name, texture_data)) {
        std::string error_message = "Failed to load texture for file \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    bool uses_staging = false;
    GlobeTextureData staging_texture_data = texture_data;
    GlobeTextureData* target_texture_data = &texture_data;
    VkImageUsageFlags loading_image_usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (!(texture_data.vk_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
        resource_manager->UseStagingBuffer()) {
        loading_image_usage_flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        target_texture_data = &staging_texture_data;
        uses_staging = true;
    }

    texture_data.is_color = true;
    texture_data.is_stencil = false;
    texture_data.is_depth = false;
    texture_data.vk_sample_count = VK_SAMPLE_COUNT_1_BIT;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = target_texture_data->vk_format;
    image_create_info.extent.width = target_texture_data->width;
    image_create_info.extent.height = target_texture_data->height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = texture_data.vk_sample_count;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = loading_image_usage_flags;
    image_create_info.flags = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &target_texture_data->vk_image)) {
        std::string error_message = "Failed to load texture from file \"";
        error_message += texture_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    if (!resource_manager->AllocateDeviceImageMemory(
            target_texture_data->vk_image, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            target_texture_data->vk_device_memory, target_texture_data->vk_allocated_size)) {
        logger.LogFatalError("Failed allocating target texture image to memory");
        return nullptr;
    }

    if (VK_SUCCESS !=
        vkBindImageMemory(vk_device, target_texture_data->vk_image, target_texture_data->vk_device_memory, 0)) {
        logger.LogError("Failed to binding memory to texture image");
        return nullptr;
    }

    VkImageSubresource image_subresource = {};
    image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource.mipLevel = 0;
    image_subresource.arrayLayer = 0;
    VkSubresourceLayout vk_subresource_layout;

    vkGetImageSubresourceLayout(vk_device, target_texture_data->vk_image, &image_subresource, &vk_subresource_layout);

    void* data;
    if (VK_SUCCESS != vkMapMemory(vk_device, target_texture_data->vk_device_memory, 0,
                                  target_texture_data->vk_allocated_size, 0, &data)) {
        logger.LogError("Failed to map memory for copying over texture image");
        return nullptr;
    }
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(data);
    uint8_t* row_ptr = target_texture_data->raw_data.data();
    uint32_t from_size = target_texture_data->width * texture_data.num_components;
    uint32_t to_size = target_texture_data->width * texture_data.num_components;
    for (uint32_t y = 0; y < target_texture_data->height; y++) {
        memcpy(data_ptr, row_ptr, from_size);
        row_ptr += from_size;
        data_ptr += vk_subresource_layout.rowPitch;
    }
    vkUnmapMemory(vk_device, target_texture_data->vk_device_memory);

    target_texture_data->raw_data.clear();
    target_texture_data->vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // If we need to place this in tiled memory, we need to use a staging texture.
    if (uses_staging) {
        if (!TransitionVkImageLayout(vk_command_buffer, staging_texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                     static_cast<VkAccessFlagBits>(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT)) {
            logger.LogError("Failed to transition staging image to transfer format");
            return nullptr;
        }

        texture_data.width = staging_texture_data.width;
        texture_data.height = staging_texture_data.height;
        texture_data.vk_format = staging_texture_data.vk_format;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &texture_data.vk_image)) {
            std::string error_message = "Failed to setup optimized tiled texture target for \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return nullptr;
        }

        if (!resource_manager->AllocateDeviceImageMemory(texture_data.vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                         texture_data.vk_device_memory,
                                                         texture_data.vk_allocated_size)) {
            logger.LogFatalError("Failed allocating tiled target texture image to memory");
            return nullptr;
        }

        if (VK_SUCCESS != vkBindImageMemory(vk_device, texture_data.vk_image, texture_data.vk_device_memory, 0)) {
            logger.LogError("Failed to binding memory to tiled texture image");
            return nullptr;
        }

        texture_data.vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (!TransitionVkImageLayout(vk_command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     static_cast<VkAccessFlagBits>(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT)) {
            logger.LogError("Failed to transition resulting image to accept staging content");
            return nullptr;
        }
        texture_data.staging_texture_data = &staging_texture_data;

        VkImageCopy image_copy = {};
        image_copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        image_copy.srcOffset = {0, 0, 0};
        image_copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        image_copy.dstOffset = {0, 0, 0};
        image_copy.extent.width = texture_data.width;
        image_copy.extent.height = texture_data.height;
        image_copy.extent.depth = 1;
        vkCmdCopyImage(vk_command_buffer, staging_texture_data.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       texture_data.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
        if (!TransitionVkImageLayout(vk_command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture_data.vk_image_layout,
                                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
            logger.LogError("Failed to transition resulting texture to shader readable after staging copy");
            return nullptr;
        }
    } else {
        if (!TransitionVkImageLayout(vk_command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_PREINITIALIZED, texture_data.vk_image_layout,
                                     static_cast<VkAccessFlagBits>(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
            logger.LogError("Failed to transition texture image to shader readable format");
            return nullptr;
        }
        texture_data.staging_texture_data = nullptr;
    }

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 1;
    sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.image = VK_NULL_HANDLE;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = texture_data.vk_format;
    image_view_create_info.components = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    image_view_create_info.flags = 0;

    if (VK_SUCCESS != vkCreateSampler(vk_device, &sampler_create_info, nullptr, &texture_data.vk_sampler)) {
        logger.LogError("Failed creating texture sampler for primary texture");
        return nullptr;
    }

    image_view_create_info.image = texture_data.vk_image;
    if (VK_SUCCESS != vkCreateImageView(vk_device, &image_view_create_info, nullptr, &texture_data.vk_image_view)) {
        logger.LogError("Failed creating texture image view for primary texture");
        return nullptr;
    }

    return new GlobeTexture(resource_manager, vk_device, texture_name, &texture_data);
}

static bool IsStencilFormat(VkFormat vk_format) {
    switch (vk_format) {
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
            break;
        default:
            return false;
    }
}

static bool IsDepthFormat(VkFormat vk_format) {
    switch (vk_format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
            break;
        default:
            return false;
    }
}

GlobeTexture* GlobeTexture::CreateRenderTarget(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                               VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                               VkFormat vk_format) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    bool is_depth = IsDepthFormat(vk_format);
    bool is_stencil = IsStencilFormat(vk_format);
    bool is_color = !is_depth && !is_stencil;
    std::string texture_name = "rendertarget_";
    if (is_color) {
        texture_name += "color";
    } else {
        if (is_depth) {
            texture_name += "depth";
            if (is_stencil) {
                texture_name += "_stencil";
            }
        } else if (is_stencil) {
            texture_name += "stencil";
        }
    }
    texture_name += "_";
    texture_name += std::to_string(width);
    texture_name += "_";
    texture_name += std::to_string(height);
    texture_name += "_";
    texture_name += std::to_string(static_cast<uint32_t>(vk_format));

    VkFormatProperties vk_format_props = resource_manager->GetVkFormatProperties(vk_format);
    if (0 == (vk_format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        return nullptr;
    }
    GlobeTextureData texture_data = {};
    texture_data.setup_for_render_target = true;
    texture_data.is_color = is_color;
    texture_data.is_stencil = is_stencil;
    texture_data.is_depth = is_depth;
    texture_data.vk_format = vk_format;
    texture_data.width = width;
    texture_data.height = height;
    texture_data.vk_sample_count = VK_SAMPLE_COUNT_1_BIT;

    if (is_color) {
        texture_data.vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else {
        texture_data.vk_image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = vk_format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = texture_data.vk_sample_count;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (is_color) {
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    } else {
        image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &texture_data.vk_image)) {
        std::string error_msg = "Failed to create image for render target ";
        error_msg += texture_name;
        logger.LogError(error_msg);
        return nullptr;
    }

    if (!resource_manager->AllocateDeviceImageMemory(texture_data.vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                     texture_data.vk_device_memory, texture_data.vk_allocated_size)) {
        std::string error_msg = "Failed to allocate memory for render target ";
        error_msg += texture_name;
        logger.LogError(error_msg);
        return nullptr;
    }

    if (VK_SUCCESS != vkBindImageMemory(vk_device, texture_data.vk_image, texture_data.vk_device_memory, 0)) {
        std::string error_msg = "Failed to bind image to memory for render target ";
        error_msg += texture_name;
        logger.LogError(error_msg);
        return nullptr;
    }

    if (is_color) {
        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = nullptr;
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 1;
        sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = 0.0f;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
        if (VK_SUCCESS != vkCreateSampler(vk_device, &sampler_create_info, nullptr, &texture_data.vk_sampler)) {
            std::string error_msg = "Failed to create image sampler for render target ";
            error_msg += texture_name;
            logger.LogError(error_msg);
            return nullptr;
        }
    }

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = texture_data.vk_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = texture_data.vk_format;
    image_view_create_info.components = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };
    image_view_create_info.subresourceRange = {};
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    if (is_color) {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        image_view_create_info.subresourceRange.aspectMask = 0;
        if (is_depth) {
            image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (is_stencil) {
            image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    image_view_create_info.image = texture_data.vk_image;
    if (VK_SUCCESS != vkCreateImageView(vk_device, &image_view_create_info, nullptr, &texture_data.vk_image_view)) {
        std::string error_msg = "Failed to create imageview for render target ";
        error_msg += texture_name;
        logger.LogError(error_msg);
        return nullptr;
    }

    return new GlobeTexture(resource_manager, vk_device, texture_name, &texture_data);
}

VkAttachmentDescription GlobeTexture::GenVkAttachmentDescription() {
    VkAttachmentDescription texture_attach_desc = {};
    texture_attach_desc.format = _vk_format;
    texture_attach_desc.samples = _vk_sample_count;
    texture_attach_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    texture_attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    texture_attach_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    texture_attach_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    texture_attach_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (_is_color) {
        texture_attach_desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else {
        texture_attach_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    return texture_attach_desc;
}

VkAttachmentReference GlobeTexture::GenVkAttachmentReference(uint32_t attachment_index) {
    VkAttachmentReference texture_attach_ref = {};
    texture_attach_ref.attachment = attachment_index;
    if (_is_color) {
        texture_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else {
        texture_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    return texture_attach_ref;
}

GlobeTexture::GlobeTexture(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                           const std::string& texture_name, GlobeTextureData* texture_data)
    : _globe_resource_mgr(resource_manager), _vk_device(vk_device), _texture_name(texture_name) {
    _setup_for_render_target = texture_data->setup_for_render_target;
    _is_color = texture_data->is_color;
    _is_depth = texture_data->is_depth;
    _is_stencil = texture_data->is_stencil;
    _width = texture_data->width;
    _height = texture_data->height;
    _vk_sample_count = texture_data->vk_sample_count;
    _vk_format = texture_data->vk_format;
    _vk_sampler = texture_data->vk_sampler;
    _vk_image = texture_data->vk_image;
    _vk_image_layout = texture_data->vk_image_layout;
    _vk_allocated_size = texture_data->vk_allocated_size;
    _vk_device_memory = texture_data->vk_device_memory;
    _vk_image_view = texture_data->vk_image_view;
    if (nullptr != texture_data->staging_texture_data) {
        _uses_staging_texture = true;
        _staging_texture =
            new GlobeTexture(resource_manager, vk_device, texture_name, texture_data->staging_texture_data);
    } else {
        _uses_staging_texture = false;
        _staging_texture = nullptr;
    }
}

GlobeTexture::~GlobeTexture() {
    if (VK_NULL_HANDLE != _vk_sampler) {
        vkDestroySampler(_vk_device, _vk_sampler, nullptr);
        _vk_sampler = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != _vk_image_view) {
        vkDestroyImageView(_vk_device, _vk_image_view, nullptr);
        _vk_image_view = VK_NULL_HANDLE;
    }
    vkDestroyImage(_vk_device, _vk_image, nullptr);
    _globe_resource_mgr->FreeDeviceMemory(_vk_device_memory);
}

bool GlobeTexture::DeleteStagingTexture() {
    if (_uses_staging_texture) {
        delete _staging_texture;
        _staging_texture = nullptr;
        _uses_staging_texture = false;
    }
    return true;
}
