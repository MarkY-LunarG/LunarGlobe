//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_shader.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <cstring>
#include <fstream>
#include <string>
#include <sstream>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_submit_manager.hpp"
#include "globe_shader.hpp"
#include "globe_texture.hpp"
#include "globe_resource_manager.hpp"
#include "globe_app.hpp"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
const char directory_symbol = '\\';
#else
const char directory_symbol = '/';
#endif
static const char main_shader_func_name[] = "main";

GlobeShader* GlobeShader::LoadFromFile(VkDevice vk_device, const std::string& shader_name,
                                       const std::string& directory) {
    GlobeShaderStageInitData shader_data[GLOBE_SHADER_STAGE_ID_NUM_STAGES] = {{}, {}, {}, {}, {}, {}};

    for (uint32_t stage = 0; stage < GLOBE_SHADER_STAGE_ID_NUM_STAGES; ++stage) {
        std::string full_shader_name = directory;
        full_shader_name += directory_symbol;
        full_shader_name += shader_name;
        switch (stage) {
            case GLOBE_SHADER_STAGE_ID_VERTEX:
                full_shader_name += "-vs.spv";
                break;
            case GLOBE_SHADER_STAGE_ID_TESSELLATION_CONTROL:
                full_shader_name += "-cs.spv";
                break;
            case GLOBE_SHADER_STAGE_ID_TESSELLATION_EVALUATION:
                full_shader_name += "-es.spv";
                break;
            case GLOBE_SHADER_STAGE_ID_GEOMETRY:
                full_shader_name += "-gs.spv";
                break;
            case GLOBE_SHADER_STAGE_ID_FRAGMENT:
                full_shader_name += "-fs.spv";
                break;
            case GLOBE_SHADER_STAGE_ID_COMPUTE:
                full_shader_name += "-cp.spv";
                break;
            default:
                continue;
        }
        std::ifstream* infile = nullptr;
        std::string line;
        std::stringstream strstream;
        size_t shader_spv_size;
        infile = new std::ifstream(full_shader_name.c_str(), std::ifstream::in | std::ios::binary);
        if (nullptr == infile || infile->fail()) {
            continue;
        }
        strstream << infile->rdbuf();
        infile->close();
        delete infile;

        // Read the file contents
        shader_spv_size = strstream.str().size();
        shader_data[stage].spirv_content.resize((shader_spv_size / sizeof(uint32_t)));
        memcpy(shader_data[stage].spirv_content.data(), strstream.str().c_str(), shader_spv_size);
        shader_data[stage].valid = true;
    }

    return new GlobeShader(vk_device, shader_name, shader_data);
}

GlobeShader::GlobeShader(VkDevice vk_device, const std::string& shader_name,
                         const GlobeShaderStageInitData shader_data[GLOBE_SHADER_STAGE_ID_NUM_STAGES])
    : _initialized(true), _vk_device(vk_device), _shader_name(shader_name) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    uint32_t num_loaded_shaders = 0;
    for (uint32_t stage = 0; stage < GLOBE_SHADER_STAGE_ID_NUM_STAGES; ++stage) {
        if (shader_data[stage].valid) {
            _shader_data[stage].vk_shader_flag = static_cast<VkShaderStageFlagBits>(1 << stage);

            VkShaderModuleCreateInfo shader_module_create_info;
            shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shader_module_create_info.pNext = nullptr;
            shader_module_create_info.codeSize = shader_data[stage].spirv_content.size() * sizeof(uint32_t);
            shader_module_create_info.pCode = shader_data[stage].spirv_content.data();
            shader_module_create_info.flags = 0;
            VkResult vk_result = vkCreateShaderModule(vk_device, &shader_module_create_info, NULL,
                                                      &_shader_data[stage].vk_shader_module);
            if (VK_SUCCESS != vk_result) {
                _initialized = false;
                _shader_data[stage].valid = false;
                std::string error_msg = "GlobeTexture::Read failed to read shader ";
                error_msg += shader_name;
                error_msg += " with error ";
                error_msg += vk_result;
                logger.LogError(error_msg);
            } else {
                num_loaded_shaders++;
                _shader_data[stage].valid = true;
            }
        } else {
            _shader_data[stage].valid = false;
        }
    }
    if (num_loaded_shaders == 0 && _initialized) {
        _initialized = false;
    }
}

GlobeShader::~GlobeShader() {
    if (_initialized) {
        for (uint32_t stage = 0; stage < GLOBE_SHADER_STAGE_ID_NUM_STAGES; ++stage) {
            if (_shader_data[stage].valid) {
                vkDestroyShaderModule(_vk_device, _shader_data[stage].vk_shader_module, nullptr);
                _shader_data[stage].valid = false;
                _shader_data[stage].vk_shader_module = VK_NULL_HANDLE;
            }
        }
        _initialized = false;
    }
}

bool GlobeShader::GetPipelineShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& pipeline_stages) const {
    if (!_initialized) {
        return false;
    }
    VkPipelineShaderStageCreateInfo cur_stage_create_info = {};
    cur_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cur_stage_create_info.pName = main_shader_func_name;
    for (uint32_t stage = 0; stage < GLOBE_SHADER_STAGE_ID_NUM_STAGES; ++stage) {
        if (_shader_data[stage].valid) {
            cur_stage_create_info.stage = _shader_data[stage].vk_shader_flag;
            cur_stage_create_info.module = _shader_data[stage].vk_shader_module;
            pipeline_stages.push_back(cur_stage_create_info);
        }
    }
    return true;
}