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

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

enum GravityShaderStageId {
    GRAVITY_SHADER_STAGE_ID_VERTEX = 0,
    GRAVITY_SHADER_STAGE_ID_TESSELLATION_CONTROL,
    GRAVITY_SHADER_STAGE_ID_TESSELLATION_EVALUATION,
    GRAVITY_SHADER_STAGE_ID_GEOMETRY,
    GRAVITY_SHADER_STAGE_ID_FRAGMENT,
    GRAVITY_SHADER_STAGE_ID_COMPUTE,
    GRAVITY_SHADER_STAGE_ID_NUM_STAGES
};

struct GravityShaderStageInitData {
    bool valid;
    std::vector<uint32_t> spirv_content;
};

struct GravityShaderStage {
    bool                  valid;
    VkShaderStageFlagBits vk_shader_flag;
    VkShaderModule        vk_shader_module;
};

class GravityShader {
  public:
    static GravityShader* LoadFromFile(VkDevice vk_device, const std::string& shader_name, const std::string& directory);

    GravityShader(VkDevice vk_device, const std::string& shader_name, const GravityShaderStageInitData shader_data[GRAVITY_SHADER_STAGE_ID_NUM_STAGES]);
    ~GravityShader();

    bool IsValid() { return _initialized; }
    bool GetPipelineShaderStages(std::vector<VkPipelineShaderStageCreateInfo> &pipeline_stages) const;

  private:
    bool _initialized;
    VkDevice _vk_device;
    std::string _shader_name;
    GravityShaderStage _shader_data[GRAVITY_SHADER_STAGE_ID_NUM_STAGES];
};
