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

#include <cstring>

#include "gravity_logger.hpp"
#include "gravity_event.hpp"
#include "gravity_submit_manager.hpp"
#include "gravity_shader.hpp"
#include "gravity_texture.hpp"
#include "gravity_resource_manager.hpp"
#include "gravity_app.hpp"

static bool LoadFile(const std::string& filename, uint32_t& width, uint32_t& height, VkFormat& vk_format,
                     std::vector<uint8_t>& texture_data) {
    vk_format = VK_FORMAT_R8G8B8A8_UNORM;
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename.c_str())].UTF8String;
#elif defined(__ANDROID__)
#include <lunarg.ppm.h>
    char *cPtr;
    cPtr = (char *)lunarg_ppm;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "P6\n", 3)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", &width, &height);
    texture_data.resize(width * height * 4);
    uint8_t *rgba_data = texture_data.data()();
    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "255\n", 4)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;

    uint8_t *row_ptr = rgba_data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            memcpy(row_ptr, cPtr, 3);
            row_ptr[3] = 255;
            row_ptr += 4;
            cPtr += 3;
        }
    }
#else
    FILE* fPtr = fopen(filename.c_str(), "rb");
    char header[256], *cPtr, *tmp;

    if (!fPtr) {
        return false;
    }

    cPtr = fgets(header, 256, fPtr);  // P6
    if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
        fclose(fPtr);
        return false;
    }

    do {
        cPtr = fgets(header, 256, fPtr);
        if (cPtr == NULL) {
            fclose(fPtr);
            return false;
        }
    } while (!strncmp(header, "#", 1));

    sscanf(header, "%u %u", &width, &height);
    texture_data.resize(width * height * 4);
    uint8_t* rgba_data = texture_data.data();
    tmp = fgets(header, 256, fPtr);  // Format
    (void)tmp;
    if (cPtr == NULL || strncmp(header, "255\n", 3)) {
        fclose(fPtr);
        return false;
    }

    uint8_t* row_ptr = rgba_data;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            size_t s = fread(row_ptr, 3, 1, fPtr);
            (void)s;
            row_ptr[3] = 255;
            row_ptr += 4;
        }
    }
    fclose(fPtr);
#endif
    return true;
}

static bool TransitionVkImageLayout(VkCommandBuffer cmd_buf, VkImage image, VkImageAspectFlags aspect_mask,
                                             VkImageLayout old_image_layout, VkImageLayout new_image_layout,
                                             VkAccessFlagBits src_access_mask, VkPipelineStageFlags src_stages,
                                             VkPipelineStageFlags dest_stages) {
    GravityLogger& logger = GravityLogger::getInstance();
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

GravityTexture* GravityTexture::LoadFromFile(const GravityApp* app, VkCommandBuffer command_buffer, const std::string& texture_name,
                                             const std::string& directory) {
    GravityLogger& logger = GravityLogger::getInstance();
    GravityTextureData texture_data = {};
    std::string texture_file_name = directory;
    texture_file_name += texture_name;

    if (!LoadFile(texture_file_name, texture_data.width, texture_data.height, texture_data.vk_format, texture_data.raw_data)) {
        std::string error_message = "Failed to load texture for file \"";
        error_message += texture_file_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    VkInstance vk_instance = VK_NULL_HANDLE;
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
    VkDevice vk_device = VK_NULL_HANDLE;
    VkFormatProperties vk_format_props;
    app->GetVkInfo(vk_instance, vk_physical_device, vk_device);
    vkGetPhysicalDeviceFormatProperties(vk_physical_device, texture_data.vk_format, &vk_format_props);

    bool uses_staging = false;
    GravityTextureData staging_texture_data = texture_data;
    GravityTextureData* target_texture_data = &texture_data;
    VkImageUsageFlags loading_image_usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((vk_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !app->UsesStagingBuffer()) {
        loading_image_usage_flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        target_texture_data = &staging_texture_data;
        uses_staging = true;
    }

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
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
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

    VkMemoryRequirements texture_mem_reqs = {};
    vkGetImageMemoryRequirements(vk_device, target_texture_data->vk_image, &texture_mem_reqs);
    target_texture_data->vk_mem_alloc_info = {};
    target_texture_data->vk_mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    target_texture_data->vk_mem_alloc_info.pNext = nullptr;
    target_texture_data->vk_mem_alloc_info.allocationSize = texture_mem_reqs.size;
    if (!app->SelectMemoryTypeUsingRequirements(texture_mem_reqs,
                                                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                                                target_texture_data->vk_mem_alloc_info.memoryTypeIndex)) {
        logger.LogError("Failed to find memory type supporting necessary texture requirements");
        return nullptr;
    }

    if (VK_SUCCESS !=
        vkAllocateMemory(vk_device, &target_texture_data->vk_mem_alloc_info, nullptr, &target_texture_data->vk_device_memory)) {
        logger.LogError("Failed to allocate memory for texture image");
        return nullptr;
    }

    if (VK_SUCCESS != vkBindImageMemory(vk_device, target_texture_data->vk_image, target_texture_data->vk_device_memory, 0)) {
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
                                  target_texture_data->vk_mem_alloc_info.allocationSize, 0, &data)) {
        logger.LogError("Failed to map memory for copying over texture image");
        return nullptr;
    }
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(data);
    uint8_t* row_ptr = target_texture_data->raw_data.data();
    uint32_t from_size = target_texture_data->width * 4;
    uint32_t to_size = target_texture_data->width * 4;
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
        if (!TransitionVkImageLayout(command_buffer, staging_texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                     static_cast<VkAccessFlagBits>(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT)) {
            logger.LogError("Failed to transition staging image to transfer format");
            return nullptr;
        }

        texture_data.width = staging_texture_data.width;
        texture_data.height = staging_texture_data.height;
        texture_data.vk_format = staging_texture_data.vk_format;
        texture_data.vk_mem_alloc_info = staging_texture_data.vk_mem_alloc_info;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (VK_SUCCESS != vkCreateImage(vk_device, &image_create_info, NULL, &texture_data.vk_image)) {
            std::string error_message = "Failed to setup optimized tiled texture target for \"";
            error_message += texture_name;
            error_message += "\"";
            logger.LogError(error_message);
            return nullptr;
        }
        vkGetImageMemoryRequirements(vk_device, texture_data.vk_image, &texture_mem_reqs);
        texture_data.vk_mem_alloc_info.allocationSize = texture_mem_reqs.size;
        if (!app->SelectMemoryTypeUsingRequirements(texture_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                    texture_data.vk_mem_alloc_info.memoryTypeIndex)) {
            logger.LogError("Failed to find memory type supporting necessary tiled texture requirements");
            return nullptr;
        }

        if (VK_SUCCESS != vkAllocateMemory(vk_device, &texture_data.vk_mem_alloc_info, nullptr, &texture_data.vk_device_memory)) {
            logger.LogError("Failed to allocate memory for tiled texture image");
            return nullptr;
        }

        if (VK_SUCCESS != vkBindImageMemory(vk_device, texture_data.vk_image, texture_data.vk_device_memory, 0)) {
            logger.LogError("Failed to binding memory to tiled texture image");
            return nullptr;
        }

        texture_data.vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (!TransitionVkImageLayout(command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
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
        vkCmdCopyImage(command_buffer, staging_texture_data.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture_data.vk_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
        if (!TransitionVkImageLayout(command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     texture_data.vk_image_layout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT))
        {
            logger.LogError("Failed to transition resulting texture to shader readable after staging copy");
            return nullptr;
        }
    } else {
        if (!TransitionVkImageLayout(command_buffer, texture_data.vk_image, VK_IMAGE_ASPECT_COLOR_BIT,
                                     VK_IMAGE_LAYOUT_PREINITIALIZED, texture_data.vk_image_layout, static_cast<VkAccessFlagBits>(0),
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
            logger.LogError("Failed to transition texture image to shader readable format");
            return nullptr;
        }
        texture_data.staging_texture_data = nullptr;
    }

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
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

    if (VK_SUCCESS != vkCreateSampler(vk_device, &sampler_create_info, nullptr, &texture_data.vk_sampler))
    {
        logger.LogError("Failed creating texture sampler for primary texture");
        return nullptr;
    }

    image_view_create_info.image = texture_data.vk_image;
    if (VK_SUCCESS != vkCreateImageView(vk_device, &image_view_create_info, nullptr, &texture_data.vk_image_view))
    {
        logger.LogError("Failed creating texture image view for primary texture");
        return nullptr;
    }

    return new GravityTexture(vk_physical_device, vk_device, texture_name, &texture_data);
}

GravityTexture::GravityTexture(VkPhysicalDevice vk_phys_device, VkDevice vk_device, const std::string& texture_name,
                               GravityTextureData* texture_data)
    : _vk_physical_device(vk_phys_device), _vk_device(vk_device), _texture_name(texture_name) {
    _width = texture_data->width;
    _height = texture_data->height;
    _vk_format = texture_data->vk_format;
    _vk_sampler = texture_data->vk_sampler;
    _vk_image = texture_data->vk_image;
    _vk_image_layout = texture_data->vk_image_layout;
    _vk_mem_alloc_info = texture_data->vk_mem_alloc_info;
    _vk_device_memory = texture_data->vk_device_memory;
    _vk_image_view = texture_data->vk_image_view;
    if (nullptr != texture_data->staging_texture_data) {
        _uses_staging_texture = true;
        _staging_texture = new GravityTexture(vk_phys_device, vk_device, texture_name, texture_data->staging_texture_data);
    } else {
        _uses_staging_texture = false;
        _staging_texture = nullptr;
    }
}

GravityTexture::~GravityTexture() {
    if (VK_NULL_HANDLE != _vk_sampler) {
        vkDestroySampler(_vk_device, _vk_sampler, nullptr);
        _vk_sampler = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != _vk_image_view) {
        vkDestroyImageView(_vk_device, _vk_image_view, nullptr);
        _vk_image_view = VK_NULL_HANDLE;
    }
    vkDestroyImage(_vk_device, _vk_image, nullptr);
    vkFreeMemory(_vk_device, _vk_device_memory, nullptr);
}

bool GravityTexture::DeleteStagingTexture() {
    if (_uses_staging_texture) {
        delete _staging_texture;
        _staging_texture = nullptr;
        _uses_staging_texture = false;
    }
    return true;
}
