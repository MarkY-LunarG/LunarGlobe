//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    samples/04_push_constants.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <string>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <thread>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>

#include "inttypes.h"
#include "globe/globe_logger.hpp"
#include "globe/globe_event.hpp"
#include "globe/globe_window.hpp"
#include "globe/globe_submit_manager.hpp"
#include "globe/globe_shader.hpp"
#include "globe/globe_texture.hpp"
#include "globe/globe_resource_manager.hpp"
#include "globe/globe_app.hpp"
#include "globe/globe_main.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class PushConstantApp : public GlobeApp {
   public:
    PushConstantApp();
    ~PushConstantApp();

    virtual void CleanupCommandObjects(bool is_resize) override;

   protected:
    virtual bool Setup() override;
    virtual bool Update(float diff_ms) override;
    virtual bool Draw() override;
    void UpdateEllipseCenter();

   private:
    VkDescriptorSetLayout _vk_descriptor_set_layout;
    VkPipelineLayout _vk_pipeline_layout;
    GlobeVulkanBuffer _vertex_buffer;
    GlobeVulkanBuffer _index_buffer;
    GlobeVulkanBuffer _uniform_buffer;
    VkDescriptorPool _vk_descriptor_pool;
    VkDescriptorSet _vk_descriptor_set;
    VkPipeline _vk_pipeline;
    VkDeviceSize _vk_uniform_vec4_alignment;
    uint8_t *_uniform_mapped_data;
    GlobeTexture *_texture_1;
    GlobeTexture *_texture_2;
    glm::vec4 _ellipse_center;
    glm::vec4 _movement_dir;
    uint8_t *_push_constants;
    size_t _push_constants_size;
};

PushConstantApp::PushConstantApp() {
    _vk_descriptor_set_layout = VK_NULL_HANDLE;
    _vk_pipeline_layout = VK_NULL_HANDLE;
    _vk_render_pass = VK_NULL_HANDLE;
    _vertex_buffer.vk_size = 0;
    _vertex_buffer.vk_buffer = VK_NULL_HANDLE;
    _vertex_buffer.vk_memory = VK_NULL_HANDLE;
    _index_buffer.vk_size = 0;
    _index_buffer.vk_buffer = VK_NULL_HANDLE;
    _index_buffer.vk_memory = VK_NULL_HANDLE;
    _uniform_buffer.vk_size = 0;
    _uniform_buffer.vk_buffer = VK_NULL_HANDLE;
    _uniform_buffer.vk_memory = VK_NULL_HANDLE;
    _vk_descriptor_pool = VK_NULL_HANDLE;
    _vk_descriptor_set = VK_NULL_HANDLE;
    _vk_pipeline = VK_NULL_HANDLE;
    _texture_1 = nullptr;
    _texture_2 = nullptr;
}

PushConstantApp::~PushConstantApp() { Cleanup(); }

void PushConstantApp::CleanupCommandObjects(bool is_resize) {
    if (!_is_minimized) {
        if (nullptr != _push_constants) {
            delete[] _push_constants;
            _push_constants = nullptr;
        }
        if (VK_NULL_HANDLE != _vk_pipeline) {
            vkDestroyPipeline(_vk_device, _vk_pipeline, nullptr);
            _vk_pipeline = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _vk_descriptor_set) {
            vkFreeDescriptorSets(_vk_device, _vk_descriptor_pool, 1, &_vk_descriptor_set);
            _vk_descriptor_set = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _vk_descriptor_pool) {
            vkDestroyDescriptorPool(_vk_device, _vk_descriptor_pool, nullptr);
            _vk_descriptor_pool = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _uniform_buffer.vk_buffer) {
            vkUnmapMemory(_vk_device, _uniform_buffer.vk_memory);
            vkDestroyBuffer(_vk_device, _uniform_buffer.vk_buffer, nullptr);
            _uniform_buffer.vk_buffer = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _index_buffer.vk_buffer) {
            vkDestroyBuffer(_vk_device, _index_buffer.vk_buffer, nullptr);
            _index_buffer.vk_buffer = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _vertex_buffer.vk_buffer) {
            vkDestroyBuffer(_vk_device, _vertex_buffer.vk_buffer, nullptr);
            _vertex_buffer.vk_buffer = VK_NULL_HANDLE;
        }
        _globe_resource_mgr->FreeDeviceMemory(_uniform_buffer.vk_memory);
        _globe_resource_mgr->FreeDeviceMemory(_index_buffer.vk_memory);
        _globe_resource_mgr->FreeDeviceMemory(_vertex_buffer.vk_memory);
        if (VK_NULL_HANDLE != _vk_render_pass) {
            vkDestroyRenderPass(_vk_device, _vk_render_pass, nullptr);
            _vk_render_pass = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _vk_pipeline_layout) {
            vkDestroyPipelineLayout(_vk_device, _vk_pipeline_layout, nullptr);
            _vk_pipeline_layout = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != _vk_descriptor_set_layout) {
            vkDestroyDescriptorSetLayout(_vk_device, _vk_descriptor_set_layout, nullptr);
            _vk_descriptor_set_layout = VK_NULL_HANDLE;
        }
    }
    GlobeApp::CleanupCommandObjects(is_resize);
}

static const float g_quad_vertex_buffer_data[] = {
    1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Vertex 0 Pos/Texture Coord
    -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Vertex 1 Pos/Texture Coord
    -1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // Vertex 2 Pos/Texture Coord
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // Vertex 3 Pos/Texture Coord
};
static const uint32_t g_quad_index_buffer_data[] = {0, 1, 2, 2, 3, 0};

bool PushConstantApp::Setup() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    VkCommandPool vk_setup_command_pool;
    VkCommandBuffer vk_setup_command_buffer;
    if (!GlobeApp::PreSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }

    // Determine the alignment required to store the minimum amount of data
    VkDeviceSize vk_uniform_alignment = _vk_phys_device_properties.limits.minUniformBufferOffsetAlignment;

    if (!_is_minimized) {
        uint8_t *mapped_data;

        if (nullptr == _texture_1) {
            _texture_1 = _globe_resource_mgr->LoadTexture("kootenay_winter_stream.png", false);
            if (nullptr == _texture_1) {
                logger.LogError("Failed loading kootenay_winter_stream.png texture");
                return false;
            }
        }

        if (nullptr == _texture_2) {
            _texture_2 = _globe_resource_mgr->LoadTexture("cks_memorial_taipei_pond.png", false);
            if (nullptr == _texture_2) {
                logger.LogError("Failed loading cks_memorial_taipei_lake.png texture");
                return false;
            }
        }

        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
        VkDescriptorSetLayoutBinding cur_binding = {};
        cur_binding.binding = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
        cur_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        cur_binding.descriptorCount = 1;
        cur_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        cur_binding.pImmutableSamplers = nullptr;
        descriptor_set_layout_bindings.push_back(cur_binding);
        cur_binding = {};
        cur_binding.binding = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
        cur_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cur_binding.descriptorCount = 1;
        cur_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        cur_binding.pImmutableSamplers = nullptr;
        descriptor_set_layout_bindings.push_back(cur_binding);
        cur_binding = {};
        cur_binding.binding = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
        cur_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cur_binding.descriptorCount = 1;
        cur_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        cur_binding.pImmutableSamplers = nullptr;
        descriptor_set_layout_bindings.push_back(cur_binding);

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {};
        descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout.pNext = nullptr;
        descriptor_set_layout.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
        descriptor_set_layout.pBindings = descriptor_set_layout_bindings.data();
        if (VK_SUCCESS !=
            vkCreateDescriptorSetLayout(_vk_device, &descriptor_set_layout, nullptr, &_vk_descriptor_set_layout)) {
            logger.LogFatalError("Failed to create descriptor set layout");
            return false;
        }

        // Set up our push constant range, which mirrors the declaration of
        std::vector<VkPushConstantRange> push_constant_ranges;
        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = 12;
        push_constant_ranges.push_back(push_constant_range);

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &_vk_descriptor_set_layout;
        if (VK_SUCCESS !=
            vkCreatePipelineLayout(_vk_device, &pipeline_layout_create_info, nullptr, &_vk_pipeline_layout)) {
            logger.LogFatalError("Failed to create pipeline layout layout");
            return false;
        }

        // The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
        // because at the start of the renderpass, we don't care about their contents.
        // At the start of the subpass, the color attachment's layout will be transitioned
        // to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
        // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
        // the renderpass, the color attachment's layout will be transitioned to
        // LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
        // the renderpass, no barriers are necessary.
        VkAttachmentDescription attachment_descriptions[2];
        VkAttachmentReference color_attachment_reference = {};
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depth_attachment_reference = {};
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment_descriptions[0] = {};
        attachment_descriptions[0].format = _globe_submit_mgr->GetSwapchainVkFormat();
        attachment_descriptions[0].flags = 0;
        attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment_descriptions[1] = {};
        attachment_descriptions[1].format = _depth_buffer.vk_format;
        attachment_descriptions[1].flags = 0;
        attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.flags = 0;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = nullptr;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_attachment_reference;
        subpass_description.pResolveAttachments = nullptr;
        subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = nullptr;
        VkRenderPassCreateInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = 2;
        render_pass_create_info.pAttachments = attachment_descriptions;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass_description;
        render_pass_create_info.dependencyCount = 0;
        render_pass_create_info.pDependencies = nullptr;
        if (VK_SUCCESS != vkCreateRenderPass(_vk_device, &render_pass_create_info, NULL, &_vk_render_pass)) {
            logger.LogFatalError("Failed to create renderpass");
            return false;
        }

        // Create the vertex buffer
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_create_info.size = sizeof(g_quad_vertex_buffer_data);
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.flags = 0;
        if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_vertex_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create vertex buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _vertex_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _vertex_buffer.vk_memory, _vertex_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate vertex buffer memory");
            return false;
        }
        if (VK_SUCCESS !=
            vkMapMemory(_vk_device, _vertex_buffer.vk_memory, 0, _vertex_buffer.vk_size, 0, (void **)&mapped_data)) {
            logger.LogFatalError("Failed to map vertex buffer memory");
            return false;
        }
        memcpy(mapped_data, g_quad_vertex_buffer_data, sizeof(g_quad_vertex_buffer_data));
        vkUnmapMemory(_vk_device, _vertex_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _vertex_buffer.vk_buffer, _vertex_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind vertex buffer memory");
            return false;
        }

        // Create the index buffer
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_create_info.size = sizeof(g_quad_index_buffer_data);
        if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_index_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create index buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _index_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _index_buffer.vk_memory, _index_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate index buffer memory");
            return false;
        }
        if (VK_SUCCESS !=
            vkMapMemory(_vk_device, _index_buffer.vk_memory, 0, _index_buffer.vk_size, 0, (void **)&mapped_data)) {
            logger.LogFatalError("Failed to map index buffer memory");
            return false;
        }
        memcpy(mapped_data, g_quad_index_buffer_data, sizeof(g_quad_index_buffer_data));
        vkUnmapMemory(_vk_device, _index_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _index_buffer.vk_buffer, _index_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind index buffer memory");
            return false;
        }

        _ellipse_center = glm::vec4(0.2f, 0.2f, 0.f, 0.f);
        _movement_dir = glm::vec4(0.01f, 0.01f, 0.f, 0.f);
        _vk_uniform_vec4_alignment = (sizeof(glm::vec4) + vk_uniform_alignment - 1) & ~(vk_uniform_alignment - 1);

        // The smallest submit size is an atom, so we need to make sure we're at least as big as that per
        // uniform buffer submission.
        if (_vk_uniform_vec4_alignment < _vk_phys_device_properties.limits.nonCoherentAtomSize) {
            _vk_uniform_vec4_alignment = _vk_phys_device_properties.limits.nonCoherentAtomSize;
        }

        // Create the uniform buffer containing the mvp matrix
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.size = _vk_uniform_vec4_alignment * _swapchain_count;
        if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_uniform_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create uniform buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _uniform_buffer.vk_buffer, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _uniform_buffer.vk_memory, _uniform_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate uniform buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkMapMemory(_vk_device, _uniform_buffer.vk_memory, 0, _uniform_buffer.vk_size, 0,
                                      (void **)&_uniform_mapped_data)) {
            logger.LogFatalError("Failed to map uniform buffer memory");
            return false;
        }
        memcpy(_uniform_mapped_data, &_ellipse_center, sizeof(_ellipse_center));

        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _uniform_buffer.vk_buffer, _uniform_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind uniform buffer memory");
            return false;
        }

        std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
        VkDescriptorPoolSize pool_size = {};
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_size.descriptorCount = 1;
        descriptor_pool_sizes.push_back(pool_size);
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = 2;
        descriptor_pool_sizes.push_back(pool_size);
        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = nullptr;
        descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptor_pool_create_info.maxSets = 2;
        descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
        if (VK_SUCCESS !=
            vkCreateDescriptorPool(_vk_device, &descriptor_pool_create_info, nullptr, &_vk_descriptor_pool)) {
            logger.LogFatalError("Failed to create descriptor pool");
            return false;
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = _vk_descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &_vk_descriptor_set_layout;
        if (VK_SUCCESS != vkAllocateDescriptorSets(_vk_device, &descriptor_set_allocate_info, &_vk_descriptor_set)) {
            logger.LogFatalError("Failed to allocate descriptor set");
            return false;
        }

        std::vector<VkDescriptorImageInfo> descriptor_image_infos;
        VkDescriptorImageInfo image_info = {};
        image_info.sampler = _texture_1->GetVkSampler();
        image_info.imageView = _texture_1->GetVkImageView();
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptor_image_infos.push_back(image_info);
        image_info = {};
        image_info.sampler = _texture_2->GetVkSampler();
        image_info.imageView = _texture_2->GetVkImageView();
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptor_image_infos.push_back(image_info);

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _uniform_buffer.vk_buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(glm::vec4);

        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        VkWriteDescriptorSet write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.dstSet = _vk_descriptor_set;
        write_set.dstBinding = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_sets.push_back(write_set);
        write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.dstSet = _vk_descriptor_set;
        write_set.dstBinding = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.descriptorCount = static_cast<uint32_t>(descriptor_image_infos.size());
        write_set.pImageInfo = descriptor_image_infos.data();
        write_descriptor_sets.push_back(write_set);

        vkUpdateDescriptorSets(_vk_device, static_cast<uint32_t>(write_descriptor_sets.size()),
                               write_descriptor_sets.data(), 0, nullptr);

        // Ensure we have enough room for push constant data
        _push_constants_size = sizeof(int32_t) + (sizeof(float) * 2);
        if (_push_constants_size > _vk_phys_device_properties.limits.maxPushConstantsSize) {
            logger.LogFatalError("Not able to support required number of push constants");
            return false;
        }

        // Create our push constant data, which matches shader expectations
        _push_constants = new uint8_t[_push_constants_size];
        if (nullptr == _push_constants) {
            logger.LogFatalError("Failed to allocate space for push constants");
            return false;
        }

        int32_t *selection = reinterpret_cast<int32_t *>(_push_constants);
        *selection = 0;
        float *rad_x_sqd = reinterpret_cast<float *>(&_push_constants[sizeof(int32_t)]);
        *rad_x_sqd = 0.03f;
        float *rad_y_sqd = reinterpret_cast<float *>(&_push_constants[sizeof(int32_t) + sizeof(float)]);
        *rad_y_sqd = 0.12f;
        vkCmdPushConstants(vk_setup_command_buffer, _vk_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           static_cast<uint32_t>(_push_constants_size), _push_constants);

        // Viewport and scissor dynamic state
        VkDynamicState dynamic_state_enables[2];
        dynamic_state_enables[0] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamic_state_enables[1] = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
        pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipeline_dynamic_state_create_info.dynamicStateCount = 2;
        pipeline_dynamic_state_create_info.pDynamicStates = dynamic_state_enables;

        // No need to tell it anything about input state right now.
        VkVertexInputBindingDescription vertex_input_binding_description = {};
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_input_binding_description.stride = sizeof(float) * 8;
        VkVertexInputAttributeDescription vertex_input_attribute_description[2];
        vertex_input_attribute_description[0] = {};
        vertex_input_attribute_description[0].binding = 0;
        vertex_input_attribute_description[0].location = 0;
        vertex_input_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertex_input_attribute_description[0].offset = 0;
        vertex_input_attribute_description[1] = {};
        vertex_input_attribute_description[1].binding = 0;
        vertex_input_attribute_description[1].location = 1;
        vertex_input_attribute_description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertex_input_attribute_description[1].offset = sizeof(float) * 4;
        VkPipelineVertexInputStateCreateInfo pipline_vert_input_state_create_info = {};
        pipline_vert_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipline_vert_input_state_create_info.pNext = NULL;
        pipline_vert_input_state_create_info.flags = 0;
        pipline_vert_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipline_vert_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipline_vert_input_state_create_info.vertexAttributeDescriptionCount = 2;
        pipline_vert_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_description;

        // Just render a triangle strip
        VkPipelineInputAssemblyStateCreateInfo pipline_input_assembly_state_create_info = {};
        pipline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Fill mode with culling
        VkPipelineRasterizationStateCreateInfo pipeline_raster_state_create_info = {};
        pipeline_raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_raster_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        pipeline_raster_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_raster_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_raster_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_raster_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_raster_state_create_info.lineWidth = 1.0f;

        // No color blending
        VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
        pipeline_color_blend_attachment_state.colorWriteMask = 0xf;
        pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.attachmentCount = 1;
        pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

        // Setup viewport and scissor
        VkRect2D scissor_rect = {};
        scissor_rect.offset.x = 0;
        scissor_rect.offset.y = 0;
        scissor_rect.extent.width = _width;
        scissor_rect.extent.height = _height;
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)_width;
        viewport.height = (float)_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissor_rect;

        // Depth stencil state
        VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.front = pipeline_depth_stencil_state_create_info.back;

        // No multisampling
        VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pSampleMask = nullptr;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        GlobeShader *multi_tex_shader = _globe_resource_mgr->LoadShader("position_multi_texture_ellipse_pushconst");
        if (nullptr == multi_tex_shader) {
            logger.LogFatalError("Failed to load position_multi_texture_ellipse_pushconst shaders");
            return false;
        }
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
        multi_tex_shader->GetPipelineShaderStages(pipeline_shader_stage_create_info);

        VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = {};
        gfx_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gfx_pipeline_create_info.layout = _vk_pipeline_layout;
        gfx_pipeline_create_info.pVertexInputState = &pipline_vert_input_state_create_info;
        gfx_pipeline_create_info.pInputAssemblyState = &pipline_input_assembly_state_create_info;
        gfx_pipeline_create_info.pRasterizationState = &pipeline_raster_state_create_info;
        gfx_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        gfx_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        gfx_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        gfx_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        gfx_pipeline_create_info.stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_info.size());
        gfx_pipeline_create_info.pStages = pipeline_shader_stage_create_info.data();
        gfx_pipeline_create_info.renderPass = _vk_render_pass;
        gfx_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(_vk_device, VK_NULL_HANDLE, 1, &gfx_pipeline_create_info, nullptr,
                                                    &_vk_pipeline)) {
            logger.LogFatalError("Failed to create graphics pipeline");
            return false;
        }

        _globe_resource_mgr->FreeShader(multi_tex_shader);
    }

    if (!GlobeApp::PostSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }
    _globe_submit_mgr->AttachRenderPassAndDepthBuffer(_vk_render_pass, _depth_buffer.vk_image_view);
    _current_buffer = 0;

    return true;
}

bool PushConstantApp::Update(float diff_ms) {
    static float cur_time_diff = 0;
    cur_time_diff += diff_ms;
    if (cur_time_diff > 2000.f) {
        int32_t *selection = reinterpret_cast<int32_t *>(_push_constants);
        *selection = *selection + 1;
        if (*selection > 3) {
            *selection = 0;
        }
        float *rad_x_sqd = reinterpret_cast<float *>(&_push_constants[sizeof(int32_t)]);
        *rad_x_sqd = ((rand() % 110) + 1) * 0.001f;
        float *rad_y_sqd = reinterpret_cast<float *>(&_push_constants[sizeof(int32_t) + sizeof(float)]);
        *rad_y_sqd = ((rand() % 110) + 1) * 0.003f;
        cur_time_diff = 0.f;
    }
    bool boundary_hit = false;
    _ellipse_center += _movement_dir;
    if (_ellipse_center.x > 1.0f) {
        _ellipse_center.x = 1.0f;
        boundary_hit = true;
    }
    if (_ellipse_center.x < 0.0f) {
        _ellipse_center.x = 0.0f;
        boundary_hit = true;
    }
    if (_ellipse_center.y > 1.0f) {
        _ellipse_center.y = 1.0f;
        boundary_hit = true;
    }
    if (_ellipse_center.y < 0.0f) {
        _ellipse_center.y = 0.0f;
        boundary_hit = true;
    }
    if (boundary_hit) {
        _movement_dir.x = 0.001f * (static_cast<float>((rand() % 9) + 1));
        if (rand() % 2 == 0 && _ellipse_center.x > 0.1f) {
            _movement_dir.x = -_movement_dir.x;
        }
        _movement_dir.y = 0.001f * (static_cast<float>((rand() % 9) + 1));
        if (rand() % 2 == 0 && _ellipse_center.y > 0.1f) {
            _movement_dir.y = -_movement_dir.y;
        }
    }
    return true;
}

bool PushConstantApp::Draw() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    VkCommandBuffer vk_render_command_buffer;
    VkFramebuffer vk_framebuffer;
    _globe_submit_mgr->AcquireNextImageIndex(_current_buffer);
    _globe_submit_mgr->GetCurrentRenderCommandBuffer(vk_render_command_buffer);
    _globe_submit_mgr->GetCurrentFramebuffer(vk_framebuffer);

    if (!UpdateOverlay(_current_buffer)) {
        logger.LogFatalError("Failed to update overlay");
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    VkClearValue clear_values[2];
    VkRenderPassBeginInfo render_pass_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    clear_values[0] = {};
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 0.0f;
    clear_values[1] = {};
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _vk_render_pass;
    render_pass_begin_info.framebuffer = vk_framebuffer;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent.width = _width;
    render_pass_begin_info.renderArea.extent.height = _height;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;
    if (VK_SUCCESS != vkBeginCommandBuffer(vk_render_command_buffer, &command_buffer_begin_info)) {
        logger.LogFatalError("Failed to begin command buffer for draw commands for framebuffer");
    }

    vkCmdPushConstants(vk_render_command_buffer, _vk_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       static_cast<uint32_t>(_push_constants_size), _push_constants);

    vkCmdBeginRenderPass(vk_render_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Update dynamic viewport state
    VkViewport viewport = {};
    viewport.height = (float)_height;
    viewport.width = (float)_width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(vk_render_command_buffer, 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = _width;
    scissor.extent.height = _height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(vk_render_command_buffer, 0, 1, &scissor);

    uint32_t dynamic_offset = _current_buffer * static_cast<uint32_t>(_vk_uniform_vec4_alignment);
    vkCmdBindDescriptorSets(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline_layout, 0, 1,
                            &_vk_descriptor_set, 1, &dynamic_offset);
    vkCmdBindPipeline(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline);

    VkDeviceSize offset = (_vk_uniform_vec4_alignment * _current_buffer);
    memcpy(_uniform_mapped_data + offset, &_ellipse_center, sizeof(_ellipse_center));
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = _uniform_buffer.vk_memory;
    memoryRange.size = _vk_uniform_vec4_alignment;
    memoryRange.offset = offset;
    vkFlushMappedMemoryRanges(_vk_device, 1, &memoryRange);

    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdBindVertexBuffers(vk_render_command_buffer, 0, 1, &_vertex_buffer.vk_buffer, &vert_buffer_offset);
    vkCmdBindIndexBuffer(vk_render_command_buffer, _index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vk_render_command_buffer, 6, 1, 0, 0, 1);

    DrawOverlay(vk_render_command_buffer, _current_buffer);

    vkCmdEndRenderPass(vk_render_command_buffer);
    if (VK_SUCCESS != vkEndCommandBuffer(vk_render_command_buffer)) {
        logger.LogFatalError("Failed to end command buffer");
        return false;
    }

    _globe_submit_mgr->InsertPresentCommandsToBuffer(vk_render_command_buffer);

    _globe_submit_mgr->SubmitAndPresent(VK_NULL_HANDLE);

    return GlobeApp::Draw();
}

static PushConstantApp *g_app = nullptr;

GLOBE_APP_MAIN() {
    GlobeInitStruct init_struct = {};
    GLOBE_APP_MAIN_BEGIN(init_struct)
    init_struct.app_name = "Globe App - Push Constant";
    init_struct.version.major = 0;
    init_struct.version.minor = 1;
    init_struct.version.patch = 0;
    init_struct.width = 900;
    init_struct.height = 600;
    init_struct.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    init_struct.num_swapchain_buffers = 3;
    init_struct.ideal_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    init_struct.secondary_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    g_app = new PushConstantApp();
    g_app->Init(init_struct);
    g_app->Run();
    g_app->Exit();

    GLOBE_APP_MAIN_END(0)
}
