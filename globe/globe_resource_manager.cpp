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
#include "globe_model.hpp"
#include "globe_resource_manager.hpp"
#include "globe_app.hpp"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
const char directory_symbol = '\\';
#else
const char directory_symbol = '/';
#endif

GlobeResourceManager::GlobeResourceManager(const GlobeApp* app, const std::string& directory) {
    app->GetVkInfo(_vk_instance, _vk_physical_device, _vk_device);
    _uses_staging_buffer = app->UsesStagingBuffer();
    _base_directory = directory;

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(_vk_physical_device, &_vk_physical_device_memory_properties);
}

GlobeResourceManager::~GlobeResourceManager() {
    FreeAllTextures();
    FreeAllShaders();
}

GlobeTexture* GlobeResourceManager::LoadTexture(const std::string& texture_name, VkCommandBuffer vk_command_buffer) {
    std::string texture_dir = _base_directory;
    texture_dir += directory_symbol;
    texture_dir += "textures";
    texture_dir += directory_symbol;
    GlobeTexture* texture = GlobeTexture::LoadFromFile(this, _vk_device, vk_command_buffer, texture_name, texture_dir);
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
