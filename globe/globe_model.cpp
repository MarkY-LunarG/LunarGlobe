//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_model.cpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <cstring>
#include <algorithm>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_app.hpp"
#include "globe_resource_manager.hpp"
#include "globe_model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

void GlobeModel::CopyVertexComponentData(std::vector<float>& buffer, float* data, bool data_valid, uint8_t copy_comps,
                                         uint8_t max_comps, bool flip_y) {
    uint8_t comp;
    const float default_values[4] = {0.f, 0.f, 0.f, 1.f};
    if (data_valid) {
        uint8_t max_copy = (max_comps < copy_comps) ? max_comps : copy_comps;
        for (comp = 0; comp < max_copy; ++comp) {
            if (comp == 1 && flip_y) {
                buffer.push_back(-*data);
            } else {
                buffer.push_back(*data);
            }
            ++data;
        }
        for (; comp < copy_comps; ++comp) {
            buffer.push_back(default_values[comp]);
        }
    } else {
        for (comp = 0; comp < copy_comps; ++comp) {
            buffer.push_back(default_values[comp]);
        }
    }
}

GlobeModel* GlobeModel::LoadDaeModelFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                         const GlobeComponentSizes& sizes, const std::string& model_name,
                                         const std::string& directory) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    std::string model_file_name = directory;
    model_file_name += model_name;

    Assimp::Importer importer = {};
    const aiScene* scene_data = importer.ReadFile(
        model_file_name.c_str(), (aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices |
                                  aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals));
    if (nullptr == scene_data) {
        std::string error_message = "Failed to load model for file \"";
        error_message += model_file_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }

    BoundingBox bounding_box = {};
    bounding_box.min = glm::vec4(9999999.f, 9999999.f, 9999999.f, 9999999.f);
    bounding_box.max = glm::vec4(-9999999.f, -9999999.f, -9999999.f, -9999999.f);
    bounding_box.size = glm::vec4(0.f, 0.f, 0.f, 0.f);
    std::vector<MeshInfo> meshes;
    meshes.resize(scene_data->mNumMeshes);
    std::vector<float> vertex_data;
    std::vector<uint32_t> index_data;
    float default_values[4] = {0.f, 0.f, 0.f, 1.f};
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    for (uint32_t cur_mesh = 0; cur_mesh < scene_data->mNumMeshes; ++cur_mesh) {
        const aiMesh* ai_mesh = scene_data->mMeshes[cur_mesh];

        meshes[cur_mesh] = {};
        meshes[cur_mesh].vertex_start = vertex_count;
        meshes[cur_mesh].vertex_count = ai_mesh->mNumVertices;
        meshes[cur_mesh].index_start = index_count;

        // Grab the main types of color
        memset(meshes[cur_mesh].material_info.diffuse_color, 0, 3 * sizeof(float));
        meshes[cur_mesh].material_info.diffuse_color[3] = 1.f;
        memset(meshes[cur_mesh].material_info.ambient_color, 0, 3 * sizeof(float));
        meshes[cur_mesh].material_info.ambient_color[3] = 1.f;
        memset(meshes[cur_mesh].material_info.specular_color, 0, 3 * sizeof(float));
        meshes[cur_mesh].material_info.specular_color[3] = 1.f;
        memset(meshes[cur_mesh].material_info.emissive_color, 0, 3 * sizeof(float));
        meshes[cur_mesh].material_info.emissive_color[3] = 1.f;

        aiMaterial* mtl = scene_data->mMaterials[ai_mesh->mMaterialIndex];
        aiColor4D color = {};
        aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &color);
        meshes[cur_mesh].material_info.diffuse_color[0] = color[0];
        meshes[cur_mesh].material_info.diffuse_color[1] = color[1];
        meshes[cur_mesh].material_info.diffuse_color[2] = color[2];
        meshes[cur_mesh].material_info.diffuse_color[3] = color[3];
        color = {};
        aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &color);
        meshes[cur_mesh].material_info.ambient_color[0] = color[0];
        meshes[cur_mesh].material_info.ambient_color[1] = color[1];
        meshes[cur_mesh].material_info.ambient_color[2] = color[2];
        meshes[cur_mesh].material_info.ambient_color[3] = color[3];
        color = {};
        aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &color);
        meshes[cur_mesh].material_info.specular_color[0] = color[0];
        meshes[cur_mesh].material_info.specular_color[1] = color[1];
        meshes[cur_mesh].material_info.specular_color[2] = color[2];
        meshes[cur_mesh].material_info.specular_color[3] = color[3];
        color = {};
        aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &color);
        meshes[cur_mesh].material_info.emissive_color[0] = color[0];
        meshes[cur_mesh].material_info.emissive_color[1] = color[1];
        meshes[cur_mesh].material_info.emissive_color[2] = color[2];
        meshes[cur_mesh].material_info.emissive_color[3] = color[3];
        ai_real shininess = 0.f;
        ai_real strength = 0.f;
        uint32_t item_count = 1;
        aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &item_count);
        item_count = 1;
        aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &item_count);
        meshes[cur_mesh].material_info.shininess[0] = shininess;
        meshes[cur_mesh].material_info.shininess[1] = strength;

        // Grab the vertex info
        for (uint32_t vert = 0; vert < ai_mesh->mNumVertices; ++vert) {
            CopyVertexComponentData(vertex_data, &(ai_mesh->mVertices[vert].x), true, sizes.position, 3, true);
            CopyVertexComponentData(vertex_data, &(ai_mesh->mNormals[vert].x), true, sizes.normal, 3, true);
            CopyVertexComponentData(vertex_data, meshes[cur_mesh].material_info.diffuse_color, true,
                                    sizes.diffuse_color, 4);
            CopyVertexComponentData(vertex_data, meshes[cur_mesh].material_info.ambient_color, true,
                                    sizes.ambient_color, 4);
            CopyVertexComponentData(vertex_data, meshes[cur_mesh].material_info.specular_color, true,
                                    sizes.specular_color, 4);
            CopyVertexComponentData(vertex_data, meshes[cur_mesh].material_info.emissive_color, true,
                                    sizes.emissive_color, 4);
            CopyVertexComponentData(vertex_data, meshes[cur_mesh].material_info.shininess, true, sizes.shininess, 2);
            for (uint8_t tc = 0; tc < 3; ++tc) {
                CopyVertexComponentData(vertex_data, &(ai_mesh->mTextureCoords[tc][vert].x),
                                        ai_mesh->HasTextureCoords(tc), sizes.texcoord[tc], 2);
            }
            CopyVertexComponentData(vertex_data, &(ai_mesh->mTangents[vert].x), ai_mesh->HasTangentsAndBitangents(),
                                    sizes.tangent, 3, true);
            CopyVertexComponentData(vertex_data, &(ai_mesh->mBitangents[vert].x), ai_mesh->HasTangentsAndBitangents(),
                                    sizes.bitangent, 3, true);

            // Update the bounding box if necessary.
            if (bounding_box.min.x > ai_mesh->mVertices[vert].x) {
                bounding_box.min.x = ai_mesh->mVertices[vert].x;
            }
            if (bounding_box.min.y > -ai_mesh->mVertices[vert].y) {
                bounding_box.min.y = -ai_mesh->mVertices[vert].y;
            }
            if (bounding_box.min.z > ai_mesh->mVertices[vert].z) {
                bounding_box.min.z = ai_mesh->mVertices[vert].z;
            }
            if (bounding_box.max.x < ai_mesh->mVertices[vert].x) {
                bounding_box.max.x = ai_mesh->mVertices[vert].x;
            }
            if (bounding_box.max.y < -ai_mesh->mVertices[vert].y) {
                bounding_box.max.y = -ai_mesh->mVertices[vert].y;
            }
            if (bounding_box.max.z < ai_mesh->mVertices[vert].z) {
                bounding_box.max.z = ai_mesh->mVertices[vert].z;
            }
        }
        bounding_box.size.x = bounding_box.max.x - bounding_box.min.x;
        bounding_box.size.y = bounding_box.max.y - bounding_box.min.y;
        bounding_box.size.z = bounding_box.max.z - bounding_box.min.z;
        bounding_box.size.w = bounding_box.max.w - bounding_box.min.w;

        uint32_t last_mesh_start = static_cast<uint32_t>(index_data.size());
        meshes[cur_mesh].index_start += last_mesh_start;
        for (uint32_t face_index = 0; face_index < ai_mesh->mNumFaces; ++face_index) {
            const aiFace& cur_face = ai_mesh->mFaces[face_index];
            if (cur_face.mNumIndices != 3) {
                assert(false);
                continue;
            }
            for (uint8_t vert_index = 0; vert_index < cur_face.mNumIndices; ++vert_index) {
                index_data.push_back(last_mesh_start + cur_face.mIndices[vert_index]);
                meshes[cur_mesh].index_count++;
            }
        }

        vertex_count += meshes[cur_mesh].vertex_count;
        index_count += meshes[cur_mesh].index_count;
    }

    GlobeModel* model =
        new GlobeModel(resource_manager, vk_device, model_name, sizes, meshes, bounding_box, vertex_data, index_data);
    if (model != nullptr && !model->IsValid()) {
        delete model;
        model = nullptr;
    }
    return model;
}

GlobeModel* GlobeModel::LoadModelFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                      const GlobeComponentSizes& sizes, const std::string& model_name,
                                      const std::string& directory) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    size_t period_pos = model_name.find_last_of(".");
    std::string model_suffix = model_name.substr(period_pos + 1);

    // Lower-case the suffix for easy comparison
    std::transform(model_suffix.begin(), model_suffix.end(), model_suffix.begin(), ::tolower);

    if (model_suffix == "dae") {
        return LoadDaeModelFile(resource_manager, vk_device, sizes, model_name, directory);
    } else {
        std::string error_message = "Failed to load unknown model type ";
        error_message += model_suffix;
        error_message += " model (";
        error_message += directory + model_name;
        error_message += ")";
        logger.LogFatalError(error_message);
        return nullptr;
    }
}

GlobeModel::GlobeModel(const GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& model_name,
                       const GlobeComponentSizes& sizes, std::vector<MeshInfo>& meshes, const BoundingBox& bounding_box,
                       std::vector<float>& vertices, std::vector<uint32_t>& indices)
    : _globe_resource_mgr(resource_manager),
      _vk_device(vk_device),
      _model_name(model_name),
      _bounding_box(bounding_box) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    uint8_t tc = 0;
    uint8_t* mapped_data = nullptr;

    _meshes.swap(meshes);
    _vertices.swap(vertices);
    _indices.swap(indices);
    _vertex_buffer.vk_size = 0;
    _vertex_buffer.vk_buffer = VK_NULL_HANDLE;
    _vertex_buffer.vk_memory = VK_NULL_HANDLE;
    _index_buffer.vk_size = 0;
    _index_buffer.vk_buffer = VK_NULL_HANDLE;
    _index_buffer.vk_memory = VK_NULL_HANDLE;

    // Create and fill in the vertex buffer
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.size = _vertices.size() * sizeof(float);
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;
    if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_vertex_buffer.vk_buffer)) {
        std::string error_message = "Failed to create model ";
        error_message += model_name;
        error_message += "\'s vertex buffer";
        logger.LogFatalError(error_message);
        return;
    }
    if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
            _vertex_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            _vertex_buffer.vk_memory, _vertex_buffer.vk_size)) {
        std::string error_message = "Failed to allocate model ";
        error_message += model_name;
        error_message += "\'s vertex buffer memory";
        logger.LogFatalError(error_message);
        return;
    }
    if (VK_SUCCESS !=
        vkMapMemory(_vk_device, _vertex_buffer.vk_memory, 0, _vertex_buffer.vk_size, 0, (void**)&mapped_data)) {
        std::string error_message = "Failed to map model ";
        error_message += model_name;
        error_message += "\'s vertex buffer memory";
        logger.LogFatalError(error_message);
        return;
    }
    memcpy(mapped_data, _vertices.data(), _vertices.size() * sizeof(float));
    vkUnmapMemory(_vk_device, _vertex_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _vertex_buffer.vk_buffer, _vertex_buffer.vk_memory, 0)) {
        std::string error_message = "Failed to bind model ";
        error_message += model_name;
        error_message += "\'s vertex buffer memory";
        logger.LogFatalError(error_message);
        return;
    }

    // Create and fill in the index buffer
    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.size = _indices.size() * sizeof(uint32_t);
    if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_index_buffer.vk_buffer)) {
        std::string error_message = "Failed to create model ";
        error_message += model_name;
        error_message += "\'s index buffer";
        logger.LogFatalError(error_message);
        return;
    }
    if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
            _index_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            _index_buffer.vk_memory, _index_buffer.vk_size)) {
        std::string error_message = "Failed to allocate model ";
        error_message += model_name;
        error_message += "\'s index buffer memory";
        logger.LogFatalError(error_message);
        return;
    }
    if (VK_SUCCESS !=
        vkMapMemory(_vk_device, _index_buffer.vk_memory, 0, _index_buffer.vk_size, 0, (void**)&mapped_data)) {
        std::string error_message = "Failed to map model ";
        error_message += model_name;
        error_message += "\'s index buffer memory";
        logger.LogFatalError(error_message);
        return;
    }
    memcpy(mapped_data, _indices.data(), _indices.size() * sizeof(uint32_t));
    vkUnmapMemory(_vk_device, _index_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _index_buffer.vk_buffer, _index_buffer.vk_memory, 0)) {
        std::string error_message = "Failed to bind model ";
        error_message += model_name;
        error_message += "\'s index buffer memory";
        logger.LogFatalError(error_message);
        return;
    }

    // No need to tell it anything about input state right now.
    VkFormat data_formats[5] = {VK_FORMAT_UNDEFINED, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
                                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    uint32_t cur_binding = 0;
    uint32_t cur_location = 0;
    uint32_t cur_offset = 0;

    VkVertexInputAttributeDescription vert_input_attrib_desc = {};
    vert_input_attrib_desc.binding = cur_binding;
    vert_input_attrib_desc.location = cur_location++;
    vert_input_attrib_desc.format = data_formats[sizes.position];
    vert_input_attrib_desc.offset = cur_offset;
    cur_offset += (sizes.position * sizeof(float));
    _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    if (sizes.normal > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.normal];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.normal * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.diffuse_color > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.diffuse_color];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.diffuse_color * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.ambient_color > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.ambient_color];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.ambient_color * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.specular_color > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.specular_color];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.specular_color * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.emissive_color > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.emissive_color];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.emissive_color * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.shininess > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.shininess];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.shininess * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    for (tc = 0; tc < 3; ++tc) {
        if (sizes.texcoord[tc] > 0) {
            vert_input_attrib_desc = {};
            vert_input_attrib_desc.binding = cur_binding;
            vert_input_attrib_desc.location = cur_location++;
            vert_input_attrib_desc.format = data_formats[sizes.texcoord[tc]];
            vert_input_attrib_desc.offset = cur_offset;
            cur_offset += (sizes.texcoord[tc] * sizeof(float));
            _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
        }
    }
    if (sizes.tangent > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.tangent];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.tangent * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }
    if (sizes.bitangent > 0) {
        vert_input_attrib_desc = {};
        vert_input_attrib_desc.binding = cur_binding;
        vert_input_attrib_desc.location = cur_location++;
        vert_input_attrib_desc.format = data_formats[sizes.bitangent];
        vert_input_attrib_desc.offset = cur_offset;
        cur_offset += (sizes.bitangent * sizeof(float));
        _vk_vert_attrib_desc.push_back(vert_input_attrib_desc);
    }

    _vk_vert_binding_desc = {};
    _vk_vert_binding_desc.binding = 0;
    _vk_vert_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    _vk_vert_binding_desc.stride = cur_offset;

    _vk_pipeline_vert_create_info = {};
    _vk_pipeline_vert_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    _vk_pipeline_vert_create_info.pNext = NULL;
    _vk_pipeline_vert_create_info.flags = 0;
    _vk_pipeline_vert_create_info.vertexBindingDescriptionCount = 1;
    _vk_pipeline_vert_create_info.pVertexBindingDescriptions = &_vk_vert_binding_desc;
    _vk_pipeline_vert_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(_vk_vert_attrib_desc.size());
    _vk_pipeline_vert_create_info.pVertexAttributeDescriptions = _vk_vert_attrib_desc.data();

    _is_valid = true;
}

GlobeModel::~GlobeModel() {
    if (VK_NULL_HANDLE != _index_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, _index_buffer.vk_buffer, nullptr);
        _index_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != _vertex_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, _vertex_buffer.vk_buffer, nullptr);
        _vertex_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    _globe_resource_mgr->FreeDeviceMemory(_index_buffer.vk_memory);
    _globe_resource_mgr->FreeDeviceMemory(_vertex_buffer.vk_memory);
}

void GlobeModel::GetSize(float& x, float& y, float& z) {
    x = _bounding_box.size[0];
    y = _bounding_box.size[1];
    z = _bounding_box.size[2];
}

void GlobeModel::GetCenter(float& x, float& y, float& z) {
    x = (_bounding_box.max[0] + _bounding_box.min[0]) * 0.5f;
    y = (_bounding_box.max[1] + _bounding_box.min[1]) * 0.5f;
    z = (_bounding_box.max[2] + _bounding_box.min[2]) * 0.5f;
}

void GlobeModel::FillInPipelineInfo(VkGraphicsPipelineCreateInfo& graphics_pipeline_c_i) {
    graphics_pipeline_c_i.pVertexInputState = &_vk_pipeline_vert_create_info;
}

void GlobeModel::Draw(VkCommandBuffer& command_buffer) {
    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer.vk_buffer, &vert_buffer_offset);
    vkCmdBindIndexBuffer(command_buffer, _index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 1);
}
