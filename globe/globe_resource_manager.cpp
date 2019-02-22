//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_resource_manager.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_submit_manager.hpp"
#include "globe_shader.hpp"
#include "globe_texture.hpp"
#include "globe_font.hpp"
#include "globe_model.hpp"
#include "globe_resource_manager.hpp"
#include "globe_app.hpp"

#include <algorithm>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
const char directory_symbol = '\\';
#else
const char directory_symbol = '/';
#endif

GlobeResourceManager::GlobeResourceManager(const GlobeApp* app, const std::string& directory,
                                           uint32_t queue_family_index) {
    _parent_app = app;
    _parent_app->GetVkInfo(_vk_instance, _vk_physical_device, _vk_device);
    _uses_staging_buffer = app->UsesStagingBuffer();
    _base_directory = directory;

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(_vk_physical_device, &_vk_physical_device_memory_properties);

    // Create a command pool for targeted command buffers;
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex = queue_family_index;
    vkCreateCommandPool(_vk_device, &cmd_pool_create_info, nullptr, &_vk_cmd_pool);
}

GlobeResourceManager::~GlobeResourceManager() {
    vkFreeCommandBuffers(_vk_device, _vk_cmd_pool, _targeted_vk_cmd_buffers.size(), _targeted_vk_cmd_buffers.data());
    vkDestroyCommandPool(_vk_device, _vk_cmd_pool, nullptr);
    FreeAllTextures();
    FreeAllShaders();
    FreeAllModels();
    FreeAllFonts();
}

// Texture management methods
// --------------------------------------------------------------------------------------------------------------

GlobeTexture* GlobeResourceManager::LoadTexture(const std::string& texture_name, VkCommandBuffer vk_command_buffer,
                                                bool generate_mipmaps) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    std::string texture_dir = _base_directory;
    texture_dir += directory_symbol;
    texture_dir += "textures";
    texture_dir += directory_symbol;
    size_t period_location = texture_name.rfind('.', texture_name.length());
    std::string file_extension;
    if (period_location != std::string::npos) {
        file_extension = texture_name.substr(period_location + 1, texture_name.length() - period_location);
        std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::tolower);
    } else {
        logger.LogError("LoadTexture called with texture name missing file extension");
        return nullptr;
    }
    GlobeTexture* texture;
    if (file_extension == "jpg" || file_extension == "jpeg" || file_extension == "png") {
        texture = GlobeTexture::LoadFromStandardFile(this, _parent_app->SubmitManager(), _vk_device, vk_command_buffer,
                                                     generate_mipmaps, texture_name, texture_dir);
    } else {
        texture = GlobeTexture::LoadFromKtxFile(this, _parent_app->SubmitManager(), _vk_device, vk_command_buffer,
                                                generate_mipmaps, texture_name, texture_dir);
    }
    if (nullptr != texture) {
        _textures.push_back(texture);
    }
    return texture;
}

GlobeTexture* GlobeResourceManager::CreateRenderTargetTexture(VkCommandBuffer vk_command_buffer, uint32_t width,
                                                              uint32_t height, VkFormat vk_format) {
    GlobeTexture* texture =
        GlobeTexture::CreateRenderTarget(this, _vk_device, vk_command_buffer, width, height, vk_format);
    if (nullptr != texture) {
        _textures.push_back(texture);
    }
    return texture;
}

void GlobeResourceManager::FreeAllTextures() {
    for (auto texture : _textures) {
        delete texture;
    }
    _textures.clear();
}

void GlobeResourceManager::FreeTexture(GlobeTexture* texture) {
    for (uint32_t tex_index = 0; tex_index < _textures.size(); ++tex_index) {
        if (_textures[tex_index] == texture) {
            _textures.erase(_textures.begin() + tex_index);
        }
    }
}

bool GlobeResourceManager::InsertImageLayoutTransitionBarrier(VkCommandBuffer vk_command_buffer, VkImage vk_image,
                                                              VkImageSubresourceRange vk_subresource_range,
                                                              VkPipelineStageFlags vk_starting_stage,
                                                              VkImageLayout vk_starting_layout,
                                                              VkPipelineStageFlags vk_target_stage,
                                                              VkImageLayout vk_target_layout) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    // Based on the starting layout, define what needs to be done prior to being ready to
    // transition.
    switch (vk_starting_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_GENERAL:
            if (vk_target_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                // If we're targeting a shader read, then even though we're in an undefined state,
                // we really need to start after any potention host writes or transfers have completed.
                image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            } else {
                // Nothing to do here.
            }
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // If it was used as a color attachment, make sure any color attachment
            // writes have completed.
            image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // If it was used as a depth/stencil attachment, make sure any depth/stencil
            // attachment writes have completed.
            image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // If the image was previously being read by a shader, we need to make sure
            // any shader reads have completed from that image before we can continue.
            image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // If the image was previously used as the source of a transfer operation, we
            // need to make sure any reads have completed from that image before we can
            // continue.
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // If the image was previously used as the destination of a transfer operation, we
            // need to make sure any writes have completed from that image before we can
            // continue.
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // If pre-initialized, make sure any writes from the host are complete.
            image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        default:
            // Unhandled source image layout transition
            std::string error_msg = "InsertImageLayoutTransitionBarrier - Unhandled starting layout transition ";
            error_msg += std::to_string(vk_starting_layout);
            logger.LogError(error_msg);
            return false;
    }

    // Based on what layout we're targeting, we need to make sure we wait until
    // certain actions complete before we can say we're finished.
    switch (vk_target_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_GENERAL:
            // Nothing to do here.
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // If we're going to use it for a color attachment, then we need
            // to make sure we transition to the point we can write to it.
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // If we're going to use it for a depth/stencil attachment, then we need
            // to make sure we transition to the point we can write to it.
            image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // If we're going to read this in a shader, we want to make sure we've
            // transitioned to the point we can perform a shader read.
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // If we're going to use it for a transfer source, then we need
            // to make sure we transition to the point we can perform a transfer read.
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // If we're going to use it for a transfer destination, then we need
            // to make sure we transition to the point we can write.
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            // If we're going to use it for a present source, then we need
            // to make sure we transition to the point we can perform a memory read.
            image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            // Unhandled target image layout transition
            std::string error_msg = "InsertImageLayoutTransitionBarrier - Unhandled target layout transition ";
            error_msg += std::to_string(vk_target_layout);
            logger.LogError(error_msg);
            return false;
    }

    image_memory_barrier.oldLayout = vk_starting_layout;
    image_memory_barrier.newLayout = vk_target_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = vk_image;
    image_memory_barrier.subresourceRange = vk_subresource_range;

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(vk_command_buffer, vk_starting_stage, vk_target_stage, 0, 0, nullptr, 0, nullptr, 1,
                         &image_memory_barrier);
    return true;
}

// Font management methods
// --------------------------------------------------------------------------------------------------------------

GlobeFont* GlobeResourceManager::LoadFontMap(const std::string& font_name, VkCommandBuffer vk_command_buffer,
                                             uint32_t font_size) {
    std::string font_dir = _base_directory;
    font_dir += directory_symbol;
    font_dir += "fonts";
    font_dir += directory_symbol;
    GlobeFont* font = GlobeFont::LoadFontMap(this, _parent_app->SubmitManager(), _vk_device, vk_command_buffer,
                                             font_size, font_name, font_dir);
    if (nullptr != font) {
        _fonts.push_back(font);
    }
    return font;
}

void GlobeResourceManager::FreeAllFonts() {
    for (auto font : _fonts) {
        delete font;
    }
    _fonts.clear();
}

void GlobeResourceManager::FreeFont(GlobeFont* font) {
    for (uint32_t font_index = 0; font_index < _fonts.size(); ++font_index) {
        if (_fonts[font_index] == font) {
            _fonts.erase(_fonts.begin() + font_index);
        }
    }
}

// Shader management methods
// --------------------------------------------------------------------------------------------------------------

GlobeShader* GlobeResourceManager::LoadShader(const std::string& shader_prefix) {
    std::string shader_dir = _base_directory;
    shader_dir += directory_symbol;
    shader_dir += "shaders";
    shader_dir += directory_symbol;
    GlobeShader* shader = GlobeShader::LoadFromFile(_vk_device, shader_prefix, shader_dir);
    if (nullptr != shader) {
        _shaders.push_back(shader);
    }
    return shader;
}

void GlobeResourceManager::FreeAllShaders() {
    for (auto shader : _shaders) {
        delete shader;
    }
    _shaders.clear();
}

void GlobeResourceManager::FreeShader(GlobeShader* shader) {
    for (uint32_t shd_index = 0; shd_index < _shaders.size(); ++shd_index) {
        if (_shaders[shd_index] == shader) {
            delete _shaders[shd_index];
            _shaders.erase(_shaders.begin() + shd_index);
        }
    }
}

// Model management methods
// --------------------------------------------------------------------------------------------------------------

GlobeModel* GlobeResourceManager::LoadModel(const std::string& sub_dir, const std::string& model_name,
                                            const GlobeComponentSizes& sizes) {
    std::string model_dir = _base_directory;
    model_dir += directory_symbol;
    model_dir += "models";
    model_dir += directory_symbol;
    model_dir += sub_dir;
    model_dir += directory_symbol;
    GlobeModel* model = GlobeModel::LoadModelFile(this, _vk_device, sizes, model_name, model_dir);
    if (nullptr != model) {
        _models.push_back(model);
    }
    return model;
}

void GlobeResourceManager::FreeAllModels() {
    for (auto model : _models) {
        delete model;
    }
    _models.clear();
}

void GlobeResourceManager::FreeModel(GlobeModel* model) {
    for (uint32_t model_index = 0; model_index < _models.size(); ++model_index) {
        if (_models[model_index] == model) {
            delete _models[model_index];
            _models.erase(_models.begin() + model_index);
        }
    }
}

// Memory management methods
// --------------------------------------------------------------------------------------------------------------

bool GlobeResourceManager::AllocateDeviceBufferMemory(VkBuffer vk_buffer, VkMemoryPropertyFlags vk_memory_properties,
                                                      VkDeviceMemory& vk_device_memory,
                                                      VkDeviceSize& vk_allocated_size) const {
    GlobeLogger& logger = GlobeLogger::getInstance();

    VkMemoryRequirements vk_memory_requirements = {};
    vkGetBufferMemoryRequirements(_vk_device, vk_buffer, &vk_memory_requirements);
    uint32_t memory_type_index = 0;
    if (!SelectMemoryTypeUsingRequirements(vk_memory_requirements, vk_memory_properties, memory_type_index)) {
        logger.LogError("Failed selecting memory type for buffer memory");
        return false;
    }

    VkMemoryAllocateInfo vk_memory_alloc_info = {};
    vk_memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vk_memory_alloc_info.pNext = nullptr;
    vk_memory_alloc_info.allocationSize = vk_memory_requirements.size;
    vk_memory_alloc_info.memoryTypeIndex = memory_type_index;
    if (VK_SUCCESS != vkAllocateMemory(_vk_device, &vk_memory_alloc_info, nullptr, &vk_device_memory)) {
        logger.LogError("Failed allocating device buffer memory");
        return false;
    }
    vk_allocated_size = vk_memory_requirements.size;
    return true;
}

bool GlobeResourceManager::AllocateDeviceImageMemory(VkImage vk_image, VkMemoryPropertyFlags vk_memory_properties,
                                                     VkDeviceMemory& vk_device_memory,
                                                     VkDeviceSize& vk_allocated_size) const {
    GlobeLogger& logger = GlobeLogger::getInstance();

    VkMemoryRequirements vk_memory_requirements = {};
    vkGetImageMemoryRequirements(_vk_device, vk_image, &vk_memory_requirements);
    uint32_t memory_type_index = 0;
    if (!SelectMemoryTypeUsingRequirements(vk_memory_requirements, vk_memory_properties, memory_type_index)) {
        logger.LogError("Failed selecting memory type for image memory");
        return false;
    }

    VkMemoryAllocateInfo vk_memory_alloc_info = {};
    vk_memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vk_memory_alloc_info.pNext = nullptr;
    vk_memory_alloc_info.allocationSize = vk_memory_requirements.size;
    vk_memory_alloc_info.memoryTypeIndex = memory_type_index;
    if (VK_SUCCESS != vkAllocateMemory(_vk_device, &vk_memory_alloc_info, nullptr, &vk_device_memory)) {
        logger.LogError("Failed allocating device image memory");
        return false;
    }
    vk_allocated_size = vk_memory_requirements.size;
    return true;
}

void GlobeResourceManager::FreeDeviceMemory(VkDeviceMemory& vk_device_memory) const {
    vkFreeMemory(_vk_device, vk_device_memory, nullptr);
}

VkFormatProperties GlobeResourceManager::GetVkFormatProperties(VkFormat format) const {
    VkFormatProperties format_properties = {};
    vkGetPhysicalDeviceFormatProperties(_vk_physical_device, format, &format_properties);
    return format_properties;
}

bool GlobeResourceManager::SelectMemoryTypeUsingRequirements(VkMemoryRequirements requirements, VkFlags required_flags,
                                                             uint32_t& type) const {
    uint32_t type_bits = requirements.memoryTypeBits;
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((type_bits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((_vk_physical_device_memory_properties.memoryTypes[i].propertyFlags & required_flags) ==
                required_flags) {
                type = i;
                return true;
            }
        }
        type_bits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

// Command buffer utilities.  These are provided because often we want separate command buffers
// for targeted workloads.
bool GlobeResourceManager::AllocateCommandBuffer(VkCommandBufferLevel level, VkCommandBuffer& command_buffer) {
    GlobeLogger& logger = GlobeLogger::getInstance();

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = _vk_cmd_pool;
    command_buffer_allocate_info.level = level;
    command_buffer_allocate_info.commandBufferCount = 1;
    if (VK_SUCCESS != vkAllocateCommandBuffers(_vk_device, &command_buffer_allocate_info, &command_buffer)) {
        logger.LogFatalError("GlobeResourceManager::AllocateCommandBuffer - Failed to allocate command buffer");
        return false;
    }
    return true;
}

bool GlobeResourceManager::FreeCommandBuffer(VkCommandBuffer& command_buffer) {
    _targeted_vk_cmd_buffers.erase(
        std::remove(_targeted_vk_cmd_buffers.begin(), _targeted_vk_cmd_buffers.end(), command_buffer),
        _targeted_vk_cmd_buffers.end());
    vkFreeCommandBuffers(_vk_device, _vk_cmd_pool, 1, &command_buffer);
    command_buffer = VK_NULL_HANDLE;
    return true;
}
