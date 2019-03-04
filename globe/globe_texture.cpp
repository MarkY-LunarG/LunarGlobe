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
#include "globe_submit_manager.hpp"
#include "globe_basic_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

uint32_t GlobeTexture::NextPowerOfTwo(uint32_t value) {
    value--;
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    return ++value;
}

static VkFormat GliFormatToVkFormat(gli::format gli_format) {
    switch (gli_format) {
        // R formats
        case gli::FORMAT_R8_UNORM_PACK8:
            return VK_FORMAT_R8_UNORM;
        case gli::FORMAT_R8_SNORM_PACK8:
            return VK_FORMAT_R8_SNORM;
        case gli::FORMAT_R8_USCALED_PACK8:
            return VK_FORMAT_R8_USCALED;
        case gli::FORMAT_R8_SSCALED_PACK8:
            return VK_FORMAT_R8_SSCALED;
        case gli::FORMAT_R8_UINT_PACK8:
            return VK_FORMAT_R8_UINT;
        case gli::FORMAT_R8_SINT_PACK8:
            return VK_FORMAT_R8_SINT;
        case gli::FORMAT_R8_SRGB_PACK8:
            return VK_FORMAT_R8_SRGB;
        // RGB formats
        case gli::FORMAT_RGB8_UNORM_PACK8:
            return VK_FORMAT_R8G8B8_UNORM;
        case gli::FORMAT_RGB8_SNORM_PACK8:
            return VK_FORMAT_R8G8B8_SNORM;
        case gli::FORMAT_RGB8_USCALED_PACK8:
            return VK_FORMAT_R8G8B8_USCALED;
        case gli::FORMAT_RGB8_SSCALED_PACK8:
            return VK_FORMAT_R8G8B8_SSCALED;
        case gli::FORMAT_RGB8_UINT_PACK8:
            return VK_FORMAT_R8G8B8_UINT;
        case gli::FORMAT_RGB8_SINT_PACK8:
            return VK_FORMAT_R8G8B8_SINT;
        case gli::FORMAT_RGB8_SRGB_PACK8:
            return VK_FORMAT_R8G8B8_SRGB;
        // BGR formats
        case gli::FORMAT_BGR8_UNORM_PACK8:
            return VK_FORMAT_B8G8R8_UNORM;
        case gli::FORMAT_BGR8_SNORM_PACK8:
            return VK_FORMAT_B8G8R8_SNORM;
        case gli::FORMAT_BGR8_USCALED_PACK8:
            return VK_FORMAT_B8G8R8_USCALED;
        case gli::FORMAT_BGR8_SSCALED_PACK8:
            return VK_FORMAT_B8G8R8_SSCALED;
        case gli::FORMAT_BGR8_UINT_PACK8:
            return VK_FORMAT_B8G8R8_UINT;
        case gli::FORMAT_BGR8_SINT_PACK8:
            return VK_FORMAT_B8G8R8_SINT;
        case gli::FORMAT_BGR8_SRGB_PACK8:
            return VK_FORMAT_B8G8R8_SRGB;
        // RGBA8 formats
        case gli::FORMAT_RGBA8_UNORM_PACK8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case gli::FORMAT_RGBA8_SNORM_PACK8:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case gli::FORMAT_RGBA8_USCALED_PACK8:
            return VK_FORMAT_R8G8B8A8_USCALED;
        case gli::FORMAT_RGBA8_SSCALED_PACK8:
            return VK_FORMAT_R8G8B8A8_SSCALED;
        case gli::FORMAT_RGBA8_UINT_PACK8:
            return VK_FORMAT_R8G8B8A8_UINT;
        case gli::FORMAT_RGBA8_SINT_PACK8:
            return VK_FORMAT_R8G8B8A8_SINT;
        case gli::FORMAT_RGBA8_SRGB_PACK8:
            return VK_FORMAT_R8G8B8A8_SRGB;
        // BGRA8 formats
        case gli::FORMAT_BGRA8_UNORM_PACK8:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case gli::FORMAT_BGRA8_SNORM_PACK8:
            return VK_FORMAT_B8G8R8A8_SNORM;
        case gli::FORMAT_BGRA8_USCALED_PACK8:
            return VK_FORMAT_B8G8R8A8_USCALED;
        case gli::FORMAT_BGRA8_SSCALED_PACK8:
            return VK_FORMAT_B8G8R8A8_SSCALED;
        case gli::FORMAT_BGRA8_UINT_PACK8:
            return VK_FORMAT_B8G8R8A8_UINT;
        case gli::FORMAT_BGRA8_SINT_PACK8:
            return VK_FORMAT_B8G8R8A8_SINT;
        case gli::FORMAT_BGRA8_SRGB_PACK8:
            return VK_FORMAT_B8G8R8A8_SRGB;
        // RGB ETC2 Compression Formats
        case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8:
            return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8:
            return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        // RGBA ETC2 Compression Formats
        case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8:
            return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8:
            return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        // RGBA ASTC Compression Formats
        case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16:
            return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16:
            return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    }
    return VK_FORMAT_UNDEFINED;
}

static uint32_t PreviousPowerOfTwo(uint32_t number) {
    if (!number) {
        return 0;
    }
    uint32_t power = 1;
    while (power < number) {
        uint32_t new_power = power << 1;
        if (new_power > number) {
            return power;
        }
        if (new_power == 0x80000000) {
            return new_power;
        }
        power = new_power;
    }
    return 0;
}

static void SampleSourceForMipmap(const uint8_t* src, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
                                  uint8_t* dst) {
    uint32_t next_x = x + 1;
    if (next_x >= width) {
        next_x = x;
    }
    uint32_t next_y = y + 1;
    if (next_y >= height) {
        next_y = y;
    }
    uint32_t cum_dst[4];
    cum_dst[0] = src[((y * height + x) * 4)];
    cum_dst[1] = src[((y * height + x) * 4) + 1];
    cum_dst[2] = src[((y * height + x) * 4) + 2];
    cum_dst[3] = src[((y * height + x) * 4) + 3];
    cum_dst[0] += src[((y * height + next_x) * 4)];
    cum_dst[1] += src[((y * height + next_x) * 4) + 1];
    cum_dst[2] += src[((y * height + next_x) * 4) + 2];
    cum_dst[3] += src[((y * height + next_x) * 4) + 3];
    cum_dst[0] += src[((next_y * height + x) * 4)];
    cum_dst[1] += src[((next_y * height + x) * 4) + 1];
    cum_dst[2] += src[((next_y * height + x) * 4) + 2];
    cum_dst[3] += src[((next_y * height + x) * 4) + 3];
    cum_dst[0] += src[((next_y * height + next_x) * 4)];
    cum_dst[1] += src[((next_y * height + next_x) * 4) + 1];
    cum_dst[2] += src[((next_y * height + next_x) * 4) + 2];
    cum_dst[3] += src[((next_y * height + next_x) * 4) + 3];
    dst[0] = static_cast<uint8_t>(cum_dst[0] >> 2);
    dst[1] = static_cast<uint8_t>(cum_dst[1] >> 2);
    dst[2] = static_cast<uint8_t>(cum_dst[2] >> 2);
    dst[3] = static_cast<uint8_t>(cum_dst[3] >> 2);
}

static bool GenerateMipmaps(GlobeStandardTextureData& texture_data, uint32_t start_width, uint32_t start_height) {
    if (texture_data.levels.size() != 1) {
        return false;
    }

    uint32_t last_width = start_width;
    uint32_t last_height = start_height;
    uint32_t last_offset = 0;
    uint32_t cur_width = PreviousPowerOfTwo(start_width);
    uint32_t cur_height = PreviousPowerOfTwo(start_height);
    uint32_t cur_offset = start_width * start_height * 4;
    while (cur_width >= 1 && cur_height >= 1) {
        GlobeTextureLevel level_data = {};
        level_data.width = cur_width;
        level_data.height = cur_height;
        level_data.data_size = cur_width * cur_height * 4;
        level_data.offset = cur_offset;
        texture_data.levels.push_back(level_data);
        texture_data.raw_data.resize(texture_data.raw_data.size() + level_data.data_size);

        uint8_t* dst_ptr = texture_data.raw_data.data() + cur_offset;
        uint8_t* src_ptr = texture_data.raw_data.data() + last_offset;
        for (uint32_t row = 0; row < cur_height; ++row) {
            for (uint32_t col = 0; col < cur_width; ++col) {
                SampleSourceForMipmap(src_ptr, last_width, last_height, col * 2, row * 2,
                                      &dst_ptr[(row * cur_height + col) * 4]);
            }
        }

        cur_offset += cur_width * cur_height * 4;
        last_width = cur_width;
        last_height = cur_height;
        cur_width >>= 1;
        cur_height >>= 1;
    }
    return true;
}

static bool LoadStandardFile(GlobeResourceManager* resource_manager, const std::string& filename,
                             GlobeTextureData& texture_data) {
    GlobeLogger& logger = GlobeLogger::getInstance();

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(__ANDROID__))
// filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename.c_str())].UTF8String;
#error("Unsupported platform")
#elif defined(__ANDROID__)
#else
    FILE* file_ptr = fopen(filename.c_str(), "rb");
    if (nullptr == file_ptr) {
        return false;
    }
    int32_t int_width = 0;
    int32_t int_height = 0;
    int32_t num_channels = 0;
    uint8_t* image_data = stbi_load_from_file(file_ptr, &int_width, &int_height, &num_channels, 0);
    if (nullptr == image_data || int_width <= 0 || int_height <= 0 || num_channels <= 0) {
        if (nullptr != image_data) {
            stbi_image_free(image_data);
        }
        std::string error_msg = "LoadStandardFile - Failed loading image ";
        error_msg += filename;
        error_msg += ", one or more of width/height/num_channels is incorrect";
        logger.LogError(error_msg);
        return false;
    }
    texture_data.setup_for_render_target = false;
    texture_data.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
    texture_data.width = static_cast<uint32_t>(int_width);
    texture_data.height = static_cast<uint32_t>(int_height);

    texture_data.uses_standard_data = true;
    texture_data.standard_data = new GlobeStandardTextureData();
    if (texture_data.standard_data == nullptr) {
        std::string error_string = "LoadStandardFile - loading texture contents for image ";
        error_string += filename;
        logger.LogError(error_string);
        return false;
    }
    // TODO: Brainpain - Generate mipmaps if needed
    texture_data.num_mip_levels = 1;
    GlobeTextureLevel level_data = {};
    level_data.width = int_width;
    level_data.height = int_height;
    level_data.data_size = int_width * int_height * 4;
    level_data.offset = 0;
    texture_data.standard_data->levels.push_back(level_data);
    texture_data.standard_data->raw_data.resize(level_data.data_size);
    if (num_channels != 4) {
        uint8_t* dst_ptr = reinterpret_cast<uint8_t*>(texture_data.standard_data->raw_data.data());
        uint8_t* src_ptr = image_data;
        for (int32_t row = 0; row < int_height; ++row) {
            for (int32_t col = 0; col < int_width; ++col) {
                int32_t comp = 0;
                for (; comp < num_channels; ++comp) {
                    *dst_ptr++ = *src_ptr++;
                }
                while (comp++ < 4) {
                    if (comp == 4) {
                        *dst_ptr++ = 255;
                    } else {
                        *dst_ptr++ = 0;
                    }
                }
            }
        }
    } else {
        memcpy(texture_data.standard_data->raw_data.data(), image_data, int_width * int_height * 4 * sizeof(uint8_t));
    }

    stbi_image_free(image_data);
    fclose(file_ptr);
#endif
    return true;
}

static bool LoadKtxFile(GlobeResourceManager* resource_manager, bool generate_mipmaps, const std::string& filename,
                        GlobeTextureData& texture_data) {
    GlobeLogger& logger = GlobeLogger::getInstance();

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(__ANDROID__))
// filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename.c_str())].UTF8String;
#error("Unsupported platform")
#elif defined(__ANDROID__)
#else
    gli::texture2d* gli_texture = new gli::texture2d(gli::load(filename.c_str()));
    if (gli_texture->empty()) {
        std::string error_msg = "GlobeTexture::LoadKtxFile failed for file ";
        error_msg += filename;
        error_msg += " because it either does not exist or is invalid";
        logger.LogError(error_msg);
        return false;
    }

    texture_data.is_color = true;
    texture_data.is_stencil = false;
    texture_data.is_depth = false;
    texture_data.vk_sample_count = VK_SAMPLE_COUNT_1_BIT;
    texture_data.setup_for_render_target = false;
    texture_data.width = static_cast<uint32_t>(gli_texture[0].extent().x);
    texture_data.height = static_cast<uint32_t>(gli_texture[0].extent().y);
    if (generate_mipmaps) {
        texture_data.num_mip_levels = static_cast<uint32_t>(gli_texture->levels());
    } else {
        texture_data.num_mip_levels = 1;
    }
    texture_data.vk_format = GliFormatToVkFormat(gli_texture->format());
    texture_data.vk_format_props = resource_manager->GetVkFormatProperties(texture_data.vk_format);
    if (0 == (texture_data.vk_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
        0 == (texture_data.vk_format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        std::string error_msg = "GlobeTexture::LoadKtxFile failed for file ";
        error_msg += filename;
        error_msg += " because it uses an unsupported format";
        logger.LogError(error_msg);
    }
    texture_data.gli_texture_2d = gli_texture;
#endif

    return true;
}

bool GlobeTexture::InitFromContent(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                   VkDevice vk_device, const std::string& texture_name,
                                   GlobeTextureData& texture_data) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    bool uses_staging = resource_manager->UseStagingBuffer();
    uint32_t num_mip_levels = texture_data.num_mip_levels;
    uint8_t* raw_data;
    size_t raw_data_size;
    GlobeVulkanBuffer staging_buffer;

    if (texture_data.uses_standard_data) {
        raw_data = texture_data.standard_data->raw_data.data();
        raw_data_size = texture_data.standard_data->raw_data.size();
    } else {
        raw_data = static_cast<uint8_t*>(texture_data.gli_texture_2d->data());
        raw_data_size = texture_data.gli_texture_2d->size();
    }

    VkCommandBuffer texture_copy_cmd_buf;
    if (!resource_manager->AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, texture_copy_cmd_buf)) {
        std::string error_msg = "InitFromContent - Failed allocating command buffer for copying texture  \"";
        error_msg += texture_name;
        error_msg += "\"";
        logger.LogError(error_msg);
        return false;
    }
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (VK_SUCCESS != vkBeginCommandBuffer(texture_copy_cmd_buf, &cmd_buf_begin_info)) {
        std::string error_msg = "InitFromContent - Failed beginning command buffer for copying texture  \"";
        error_msg += texture_name;
        error_msg += "\"";
        logger.LogError(error_msg);
        return false;
    }

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.levelCount = num_mip_levels;
    image_subresource_range.layerCount = 1;

    VkImageUsageFlags loading_image_usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (uses_staging) {
        // Create a temporary staging buffer that is host-visible that we can copy
        // the image data into and then transfer over to the GPU in its most
        // efficient memory layout.
        buffer_create_info.size = raw_data_size;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.flags = 0;
        if (VK_SUCCESS != vkCreateBuffer(vk_device, &buffer_create_info, NULL, &staging_buffer.vk_buffer)) {
            std::string error_msg = "InitFromContent - Failed to create staging buffer for texture \"";
            error_msg += texture_name;
            error_msg += "\"";
            logger.LogError(error_msg);
            return false;
        }
        if (!resource_manager->AllocateDeviceBufferMemory(
                staging_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                staging_buffer.vk_memory, staging_buffer.vk_size)) {
            std::string error_msg = "InitFromContent - Failed to allocate memory for staging buffer for texture \"";
            error_msg += texture_name;
            error_msg += "\"";
            logger.LogError(error_msg);
            return false;
        }
        if (VK_SUCCESS != vkBindBufferMemory(vk_device, staging_buffer.vk_buffer, staging_buffer.vk_memory, 0)) {
            std::string error_msg = "InitFromContent - Failed to bind memory to staging buffer for texture \"";
            error_msg += texture_name;
            error_msg += "\"";
            logger.LogError(error_msg);
            return false;
        }
        uint8_t* mapped_staging_memory;
        if (VK_SUCCESS != vkMapMemory(vk_device, staging_buffer.vk_memory, 0, staging_buffer.vk_size, 0,
                                      reinterpret_cast<void**>(&mapped_staging_memory))) {
            std::string error_msg = "InitFromContent - Failed to map memory for staging buffer for texture \"";
            error_msg += texture_name;
            error_msg += "\"";
            logger.LogError(error_msg);
            return false;
        }
        memcpy(mapped_staging_memory, raw_data, raw_data_size);
        vkUnmapMemory(vk_device, staging_buffer.vk_memory);

        // Now define a texture image and memory for the resulting optimally tiled texture
        // on the device itself instead of unoptimal host-visible memory.
        VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = texture_data.vk_format;
        image_create_info.extent = {texture_data.width, texture_data.height, 1};
        image_create_info.mipLevels = num_mip_levels;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = loading_image_usage_flags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &texture_data.vk_image)) {
            std::string error_message = "InitFromContent - Failed to create resulting image for texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }
        if (!resource_manager->AllocateDeviceImageMemory(texture_data.vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                         texture_data.vk_device_memory,
                                                         texture_data.vk_allocated_size)) {
            std::string error_message = "InitFromContent - Failed allocating memory for resulting image for texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }
        if (VK_SUCCESS != vkBindImageMemory(vk_device, texture_data.vk_image, texture_data.vk_device_memory, 0)) {
            std::string error_message = "InitFromContent - Failed binding memory for resulting image for texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }

        // Make sure we delay until we can copy over the contents of the texture read.
        if (!resource_manager->InsertImageLayoutTransitionBarrier(
                texture_copy_cmd_buf, texture_data.vk_image, image_subresource_range, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
            std::string error_message =
                "InitFromContent - Failed to add layout transition image barrier for texture \"";
            error_message += texture_name;
            error_message += "\" to destination transfer state";
            logger.LogError(error_message);
            return false;
        }

        // We now need to setup a copy for each miplevel in the texture.
        std::vector<VkBufferImageCopy> vk_buffer_image_copies;
        vk_buffer_image_copies.resize(num_mip_levels);
        uint32_t current_buffer_offset = 0;
        for (uint32_t mip = 0; mip < num_mip_levels; ++mip) {
            vk_buffer_image_copies[mip] = {};
            vk_buffer_image_copies[mip].bufferOffset = current_buffer_offset;
            vk_buffer_image_copies[mip].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_buffer_image_copies[mip].imageSubresource.mipLevel = mip;
            vk_buffer_image_copies[mip].imageSubresource.layerCount = 1;

            if (texture_data.uses_standard_data) {
                vk_buffer_image_copies[mip].imageExtent.width =
                    static_cast<uint32_t>(texture_data.standard_data->levels[mip].width);
                vk_buffer_image_copies[mip].imageExtent.height =
                    static_cast<uint32_t>(texture_data.standard_data->levels[mip].height);
                vk_buffer_image_copies[mip].imageExtent.depth = 1;
                // Update the offset to be after the current texture
                current_buffer_offset += static_cast<uint32_t>(texture_data.standard_data->levels[mip].data_size);
            } else {
                vk_buffer_image_copies[mip].imageExtent.width =
                    static_cast<uint32_t>((*texture_data.gli_texture_2d)[mip].extent().x);
                vk_buffer_image_copies[mip].imageExtent.height =
                    static_cast<uint32_t>((*texture_data.gli_texture_2d)[mip].extent().y);
                vk_buffer_image_copies[mip].imageExtent.depth = 1;
                // Update the offset to be after the current texture
                current_buffer_offset += static_cast<uint32_t>((*texture_data.gli_texture_2d)[mip].size());
            }
        }

        // Now, copy all the mip-map levels from the staging buffer into the final image.
        vkCmdCopyBufferToImage(texture_copy_cmd_buf, staging_buffer.vk_buffer, texture_data.vk_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(vk_buffer_image_copies.size()), vk_buffer_image_copies.data());

        // Make sure we delay until we can copy over the contents of the texture read.
        if (!resource_manager->InsertImageLayoutTransitionBarrier(
                texture_copy_cmd_buf, texture_data.vk_image, image_subresource_range, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            std::string error_message =
                "InitFromContent - Failed to add layout transition image barrier for texture \"";
            error_message += texture_name;
            error_message += "\" to shader read state";
            logger.LogError(error_message);
            return false;
        }
    } else {
        // Define a texture for linear formatting in host visible/coherent memory.
        VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = texture_data.vk_format;
        image_create_info.extent = {texture_data.width, texture_data.height, 1};
        image_create_info.mipLevels = num_mip_levels;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        image_create_info.usage = loading_image_usage_flags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &texture_data.vk_image)) {
            std::string error_message = "InitFromContent - Failed to create resulting image for linear texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }
        if (!resource_manager->AllocateDeviceImageMemory(
                texture_data.vk_image, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                texture_data.vk_device_memory, texture_data.vk_allocated_size)) {
            std::string error_message =
                "InitFromContent - Failed allocating memory for resulting image for linear texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }
        if (VK_SUCCESS != vkBindImageMemory(vk_device, texture_data.vk_image, texture_data.vk_device_memory, 0)) {
            std::string error_message =
                "InitFromContent - Failed binding memory for resulting image for linear texture \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return false;
        }

        uint8_t* mapped_staging_memory;
        if (VK_SUCCESS != vkMapMemory(vk_device, texture_data.vk_device_memory, 0, texture_data.vk_allocated_size, 0,
                                      reinterpret_cast<void**>(&mapped_staging_memory))) {
            std::string error_msg = "InitFromContent - Failed to map memory for staging buffer for linear texture \"";
            error_msg += texture_name;
            error_msg += "\"";
            logger.LogError(error_msg);
            return false;
        }
        memcpy(mapped_staging_memory, raw_data, raw_data_size);
        vkUnmapMemory(vk_device, texture_data.vk_device_memory);

        // Make sure the image is loaded.
        if (!resource_manager->InsertImageLayoutTransitionBarrier(
                texture_copy_cmd_buf, texture_data.vk_image, image_subresource_range, VK_PIPELINE_STAGE_HOST_BIT,
                VK_IMAGE_LAYOUT_PREINITIALIZED, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            std::string error_message =
                "InitFromContent - Failed to add layout transition image barrier for texture \"";
            error_message += texture_name;
            error_message += "\" to shader read state";
            logger.LogError(error_message);
            return false;
        }
    }

    if (VK_SUCCESS != vkEndCommandBuffer(texture_copy_cmd_buf)) {
        std::string error_message = "InitFromContent - Failed to end copy command buffer for texture \"";
        error_message += texture_name;
        error_message += "\"";
        logger.LogError(error_message);
        return false;
    }

    if (!submit_manager->Submit(texture_copy_cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, true)) {
        std::string error_message = "InitFromContent - Failed submitting command buffer for copying into texture \"";
        error_message += texture_name;
        error_message += "\"";
        logger.LogError(error_message);
        return false;
    }
    if (!resource_manager->FreeCommandBuffer(texture_copy_cmd_buf)) {
        std::string error_message = "InitFromContent - Failed freeing command buffer for copying into texture \"";
        error_message += texture_name;
        error_message += "\"";
        logger.LogError(error_message);
        return false;
    }

    if (uses_staging) {
        vkDestroyBuffer(vk_device, staging_buffer.vk_buffer, nullptr);
        resource_manager->FreeDeviceMemory(staging_buffer.vk_memory);
    }

    // We're now ready to be read by a shader
    texture_data.vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
        logger.LogError("InitFromContent - Failed creating texture sampler for primary texture");
        return false;
    }

    image_view_create_info.image = texture_data.vk_image;
    if (VK_SUCCESS != vkCreateImageView(vk_device, &image_view_create_info, nullptr, &texture_data.vk_image_view)) {
        logger.LogError("InitFromContent - Failed creating texture image view for primary texture");
        return false;
    }
    return true;
}

GlobeTexture* GlobeTexture::LoadFromStandardFile(GlobeResourceManager* resource_manager,
                                                 GlobeSubmitManager* submit_manager, VkDevice vk_device,
                                                 bool generate_mipmaps, const std::string& texture_name,
                                                 const std::string& directory) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    GlobeTextureData texture_data = {};
    std::string texture_file_name = directory;
    texture_file_name += texture_name;

    if (!LoadStandardFile(resource_manager, texture_file_name, texture_data)) {
        std::string error_message = "LoadFromStandardFile: Failed to load texture for file \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    GlobeTexture* texture_pointer = nullptr;
    if (!InitFromContent(resource_manager, submit_manager, vk_device, texture_name, texture_data)) {
        std::string error_message = "LoadFromStandardFile: Failed to setting up texture for Vulkan \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
    } else {
        texture_pointer = new GlobeTexture(resource_manager, vk_device, texture_name, &texture_data);
    }
    delete texture_data.standard_data;
    return texture_pointer;
}

GlobeTexture* GlobeTexture::LoadFromKtxFile(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                            VkDevice vk_device, bool generate_mipmaps, const std::string& texture_name,
                                            const std::string& directory) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    GlobeTextureData texture_data = {};
    std::string texture_file_name = directory;
    texture_file_name += texture_name;

    if (!LoadKtxFile(resource_manager, generate_mipmaps, texture_file_name, texture_data)) {
        std::string error_message = "LoadFromKtxFile - Failed to load texture for file \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    if (!InitFromContent(resource_manager, submit_manager, vk_device, texture_name, texture_data)) {
        std::string error_message = "LoadFromKtxFile - Failed to setting up texture for Vulkan \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
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

GlobeTexture* GlobeTexture::CreateRenderTarget(GlobeResourceManager* resource_manager, VkDevice vk_device,
                                               uint32_t width, uint32_t height, VkFormat vk_format) {
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

GlobeTexture::GlobeTexture(GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& texture_name,
                           GlobeTextureData* texture_data)
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
