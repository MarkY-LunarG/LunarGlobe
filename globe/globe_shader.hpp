//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_shader.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

enum GlobeShaderStageId {
    GLOBE_SHADER_STAGE_ID_VERTEX = 0,
    GLOBE_SHADER_STAGE_ID_TESSELLATION_CONTROL,
    GLOBE_SHADER_STAGE_ID_TESSELLATION_EVALUATION,
    GLOBE_SHADER_STAGE_ID_GEOMETRY,
    GLOBE_SHADER_STAGE_ID_FRAGMENT,
    GLOBE_SHADER_STAGE_ID_COMPUTE,
    GLOBE_SHADER_STAGE_ID_NUM_STAGES
};

struct GlobeShaderStageInitData {
    bool valid;
    std::vector<uint32_t> spirv_content;
};

struct GlobeShaderStage {
    bool valid;
    VkShaderStageFlagBits vk_shader_flag;
    VkShaderModule vk_shader_module;
};

class GlobeShader {
   public:
    static GlobeShader* LoadFromFile(VkDevice vk_device, const std::string& shader_name, const std::string& directory);

    GlobeShader(VkDevice vk_device, const std::string& shader_name,
                const GlobeShaderStageInitData shader_data[GLOBE_SHADER_STAGE_ID_NUM_STAGES]);
    ~GlobeShader();

    bool IsValid() { return _initialized; }
    bool GetPipelineShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& pipeline_stages) const;

   private:
    bool _initialized;
    VkDevice _vk_device;
    std::string _shader_name;
    GlobeShaderStage _shader_data[GLOBE_SHADER_STAGE_ID_NUM_STAGES];
};
