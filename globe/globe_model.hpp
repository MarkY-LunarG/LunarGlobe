//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_model.hpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <vector>
#include <limits>

#include "vulkan/vulkan_core.h"

#include "globe_glm_include.hpp"

class GlobeResourceManager;

class GlobeModel {
   public:
    struct BoundingBox {
        glm::vec4 size = {glm::vec4(0.f)};
        glm::vec4 min = {glm::vec4(std::numeric_limits<float>::max())};
        glm::vec4 max = {glm::vec4(std::numeric_limits<float>::min())};
    };

    struct ComponentSizes {
        uint8_t position;
        uint8_t normal;
        uint8_t diffuse_color;
        uint8_t ambient_color;
        uint8_t specular_color;
        uint8_t emissive_color;
        uint8_t texcoord[3];
        uint8_t tangent;
        uint8_t bitangent;
    };

    struct MaterialInfo {
        float diffuse_color[3];
        float ambient_color[3];
        float specular_color[3];
        float emissive_color[3];
    };

    struct MeshInfo {
        MaterialInfo material_info;
        uint32_t vertex_start;
        uint32_t vertex_count;
        uint32_t index_start;
        uint32_t index_count;
    };

    struct VulkanBuffer {
        VkBuffer vk_buffer;
        VkDeviceMemory vk_memory;
        VkDeviceSize vk_size;
    };

    static GlobeModel* LoadFromFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                    VkCommandBuffer vk_command_buffer, ComponentSizes sizes,
                                    const std::string& model_name, const std::string& directory);

    GlobeModel(const GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& model_name,
               ComponentSizes sizes, std::vector<MeshInfo>& meshes, const BoundingBox& bounding_box,
               std::vector<float>& vertices, std::vector<uint32_t>& indices);
    ~GlobeModel();

    bool IsValid() { return _is_valid; }
    void GetSize(float& x, float& y, float& z);
    void GetCenter(float& x, float& y, float& z);

    void FillInPipelineInfo(VkGraphicsPipelineCreateInfo& graphics_pipeline_c_i);
    void Draw(VkCommandBuffer& command_buffer);

   private:
    static void CopyVertexComponentData(std::vector<float>& buffer, float* data, bool data_valid, uint8_t copy_comps,
                                        uint8_t max_comps);

    bool _is_valid;
    VkDevice _vk_device;
    const GlobeResourceManager* _globe_resource_mgr;
    std::string _model_name;
    std::vector<MeshInfo> _meshes;
    VulkanBuffer _vertex_buffer;
    VulkanBuffer _index_buffer;
    std::vector<float> _vertices;
    std::vector<uint32_t> _indices;
    BoundingBox _bounding_box;
    VkVertexInputBindingDescription _vk_vert_binding_desc;
    std::vector<VkVertexInputAttributeDescription> _vk_vert_attrib_desc;
    VkPipelineVertexInputStateCreateInfo _vk_pipeline_vert_create_info;
};
