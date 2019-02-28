//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    samples/07_simple_model_glm.cpp
// Copyright(C):            2019; LunarG, Inc.
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
#include <sstream>

#include "inttypes.h"
#include "globe/globe_logger.hpp"
#include "globe/globe_camera.hpp"
#include "globe/globe_event.hpp"
#include "globe/globe_window.hpp"
#include "globe/globe_submit_manager.hpp"
#include "globe/globe_model.hpp"
#include "globe/globe_shader.hpp"
#include "globe/globe_texture.hpp"
#include "globe/globe_resource_manager.hpp"
#include "globe/globe_app.hpp"
#include "globe/globe_main.hpp"

class SimpleModelApp : public GlobeApp {
   public:
    SimpleModelApp();
    ~SimpleModelApp();

    virtual void Cleanup() override;

   protected:
    virtual bool Setup() override;
    virtual bool Update(float diff_ms) override;
    virtual bool Draw() override;

   private:
    void CalculateModelMatrices(void);

    VkDescriptorSetLayout _vk_descriptor_set_layout;
    VkPipelineLayout _vk_pipeline_layout;
    VkRenderPass _vk_render_pass;
    GlobeVulkanBuffer _uniform_buffer;
    GlobeModel *_model;
    uint8_t *_uniform_map;
    VkDescriptorPool _vk_descriptor_pool;
    VkDescriptorSet _vk_descriptor_set;
    VkPipeline _vk_pipeline;
    GlobeCamera _camera;
    float _camera_distance;
    float _camera_step;
    uint32_t _vk_uniform_frame_size;
    uint32_t _vk_min_uniform_alignment;
    float _model_orbit_rotation;
    float _model_orientation_rotation;
    glm::mat4 _model_mat;
    glm::vec4 _light_pos;
    glm::vec4 _light_color;
};

SimpleModelApp::SimpleModelApp() {
    _vk_descriptor_set_layout = VK_NULL_HANDLE;
    _vk_pipeline_layout = VK_NULL_HANDLE;
    _vk_render_pass = VK_NULL_HANDLE;
    _uniform_buffer.vk_size = 0;
    _uniform_buffer.vk_buffer = VK_NULL_HANDLE;
    _uniform_buffer.vk_memory = VK_NULL_HANDLE;
    _uniform_map = nullptr;
    _vk_descriptor_pool = VK_NULL_HANDLE;
    _vk_descriptor_set = VK_NULL_HANDLE;
    _vk_pipeline = VK_NULL_HANDLE;
    _camera_distance = 15.f;
    _camera_step = 0.05f;
    _camera.SetPerspectiveProjection(1.0f, 45.f, 1.0f, 100.f);
    _camera.SetCameraPosition(0.f, 0.f, -_camera_distance);
    _model_orbit_rotation = 90.f;
    _model_orientation_rotation = 0.f;
    _model = nullptr;
    _light_pos = glm::vec4(0.f, -10.f, 10.f, 1.f);
    _light_color = glm::vec4(1.f, 1.f, 1.f, 1.f);
}

SimpleModelApp::~SimpleModelApp() { Cleanup(); }

void SimpleModelApp::CalculateModelMatrices(void) {
    // Update model rotation
    glm::mat4 identity_mat = glm::mat4(1);
    glm::vec3 x_orbit_vec(1.f, 0.f, 0.f);
    glm::vec3 y_orbit_vec(0.f, 1.f, 0.f);
    float center_x = 0.f;
    float center_y = 0.f;
    float center_z = 0.f;
    _model->GetCenter(center_x, center_y, center_z);
    _model_mat = glm::translate(identity_mat, glm::vec3(-center_x, -center_y, -center_z));
    _model_mat = glm::rotate(_model_mat, glm::radians(_model_orientation_rotation), y_orbit_vec);
    _model_mat = glm::translate(_model_mat, x_orbit_vec);
    _model_mat = glm::rotate(_model_mat, glm::radians(_model_orbit_rotation), y_orbit_vec);
}

void SimpleModelApp::Cleanup() {
    GlobeApp::PreCleanup();
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
    if (nullptr != _uniform_map) {
        vkUnmapMemory(_vk_device, _uniform_buffer.vk_memory);
        _uniform_map = nullptr;
    }
    if (VK_NULL_HANDLE != _uniform_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, _uniform_buffer.vk_buffer, nullptr);
        _uniform_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    if (nullptr != _model) {
        _globe_resource_mgr->FreeModel(_model);
        _model = nullptr;
    }
    _globe_resource_mgr->FreeDeviceMemory(_uniform_buffer.vk_memory);
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
    GlobeApp::PostCleanup();
}

bool SimpleModelApp::Setup() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    VkCommandPool vk_setup_command_pool;
    VkCommandBuffer vk_setup_command_buffer;
    if (!GlobeApp::PreSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }

    // Setup the model matrices
    _vk_min_uniform_alignment =
        static_cast<uint32_t>(_vk_phys_device_properties.limits.minUniformBufferOffsetAlignment);
    if (_vk_min_uniform_alignment < static_cast<uint32_t>(_vk_phys_device_properties.limits.nonCoherentAtomSize)) {
        _vk_min_uniform_alignment = static_cast<uint32_t>(_vk_phys_device_properties.limits.nonCoherentAtomSize);
    }
    _vk_uniform_frame_size = sizeof(glm::mat4) * 2 + sizeof(glm::vec4) * 2;
    _vk_uniform_frame_size += (_vk_min_uniform_alignment - 1);
    _vk_uniform_frame_size &= ~(_vk_min_uniform_alignment - 1);

    if (!_is_minimized) {
        GlobeComponentSizes sizes = {};
        sizes.position = 4;
        sizes.normal = 4;
        sizes.diffuse_color = 4;
        sizes.ambient_color = 4;
        sizes.specular_color = 4;
        sizes.emissive_color = 4;
        sizes.shininess = 4;

        _model = _globe_resource_mgr->LoadModel("sascha_willems", "chinesedragon.dae", sizes);
        if (nullptr == _model) {
            logger.LogFatalError("Failed to load model file");
            return false;
        }

        uint8_t *mapped_data;

        VkDescriptorSetLayoutBinding descriptor_set_layout_bindings = {};
        descriptor_set_layout_bindings.binding = 0;
        descriptor_set_layout_bindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_layout_bindings.descriptorCount = 1;
        descriptor_set_layout_bindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_layout_bindings.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {};
        descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout.pNext = nullptr;
        descriptor_set_layout.bindingCount = 1;
        descriptor_set_layout.pBindings = &descriptor_set_layout_bindings;
        if (VK_SUCCESS !=
            vkCreateDescriptorSetLayout(_vk_device, &descriptor_set_layout, nullptr, &_vk_descriptor_set_layout)) {
            logger.LogFatalError("Failed to create descriptor set layout");
            return false;
        }

        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &_vk_descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
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
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.size = _vk_uniform_frame_size * _swapchain_count;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.flags = 0;

        // Create the uniform buffer containing the mvp matrix
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
        if (VK_SUCCESS !=
            vkMapMemory(_vk_device, _uniform_buffer.vk_memory, 0, VK_WHOLE_SIZE, 0, (void **)&_uniform_map)) {
            logger.LogFatalError("Failed to map uniform buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _uniform_buffer.vk_buffer, _uniform_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind uniform buffer memory");
            return false;
        }

        VkDescriptorPoolSize descriptor_pool_size = {};
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_pool_size.descriptorCount = 1;
        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = nullptr;
        descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptor_pool_create_info.maxSets = 2;
        descriptor_pool_create_info.poolSizeCount = 1;
        descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
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

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _uniform_buffer.vk_buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = _vk_uniform_frame_size;
        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = NULL;
        write_descriptor_set.dstSet = _vk_descriptor_set;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.dstBinding = 0;
        vkUpdateDescriptorSets(_vk_device, 1, &write_descriptor_set, 0, nullptr);

        // Viewport and scissor dynamic state
        VkDynamicState dynamic_state_enables[2];
        dynamic_state_enables[0] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamic_state_enables[1] = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
        pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipeline_dynamic_state_create_info.dynamicStateCount = 2;
        pipeline_dynamic_state_create_info.pDynamicStates = dynamic_state_enables;

        // Just render a triangle strip
        VkPipelineInputAssemblyStateCreateInfo pipline_input_assembly_state_create_info = {};
        pipline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Fill mode with culling
        VkPipelineRasterizationStateCreateInfo pipeline_raster_state_create_info = {};
        pipeline_raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_raster_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        pipeline_raster_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

        GlobeShader *cube_shader = _globe_resource_mgr->LoadShader("phong");
        if (nullptr == cube_shader) {
            logger.LogFatalError("Failed to load phong shaders");
            return false;
        }
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
        cube_shader->GetPipelineShaderStages(pipeline_shader_stage_create_info);

        VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = {};
        gfx_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gfx_pipeline_create_info.layout = _vk_pipeline_layout;
        _model->FillInPipelineInfo(gfx_pipeline_create_info);
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

        _globe_resource_mgr->FreeShader(cube_shader);
    }

    if (!GlobeApp::PostSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }
    _globe_submit_mgr->AttachRenderPassAndDepthBuffer(_vk_render_pass, _depth_buffer.vk_image_view);
    _current_buffer = 0;

    return true;
}

#define INCREMENT_ROTATION_VALUE(var, inc) \
    var += inc;                            \
    if (var > 360.f) var -= 360.f;         \
    if (var < 0.f) var += 360.f

bool SimpleModelApp::Update(float diff_ms) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    _globe_submit_mgr->AcquireNextImageIndex(_current_buffer);

    static float cur_time_diff = 0.f;
    cur_time_diff += diff_ms;
    if (cur_time_diff > 9.f) {
        _camera_distance += _camera_step;
        if ((_camera_step > 0.f && _camera_distance > 23.f) || (_camera_step < 0.f && _camera_distance < 15.f)) {
            _camera_step = -_camera_step;
        }
        _camera.SetCameraPosition(0.f, 0.f, -_camera_distance);

        INCREMENT_ROTATION_VALUE(_model_orbit_rotation, -0.3f);
        INCREMENT_ROTATION_VALUE(_model_orientation_rotation, -0.9f);
        CalculateModelMatrices();
        cur_time_diff = 0.f;
    }

    glm::mat4 view_mat = _camera.ViewMatrix();
    uint8_t *cur_uniform_pointer = _uniform_map + (_vk_uniform_frame_size * _current_buffer);
    memcpy(cur_uniform_pointer, _camera.ProjectionMatrix(), sizeof(glm::mat4));
    cur_uniform_pointer += sizeof(glm::mat4);
    memcpy(cur_uniform_pointer, &view_mat, sizeof(glm::mat4));
    cur_uniform_pointer += sizeof(glm::mat4);
    memcpy(cur_uniform_pointer, &_light_pos, sizeof(glm::vec4));
    cur_uniform_pointer += sizeof(glm::vec4);
    memcpy(cur_uniform_pointer, &_light_color, sizeof(glm::vec4));

    VkMappedMemoryRange mapped_range = {};
    mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_range.memory = _uniform_buffer.vk_memory;
    mapped_range.offset = (_vk_uniform_frame_size * _current_buffer);
    mapped_range.size = _vk_uniform_frame_size;
    vkFlushMappedMemoryRanges(_vk_device, 1, &mapped_range);

    return true;
}

bool SimpleModelApp::Draw() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    VkCommandBuffer vk_render_command_buffer;
    VkFramebuffer vk_framebuffer;
    _globe_submit_mgr->GetCurrentRenderCommandBuffer(vk_render_command_buffer);
    _globe_submit_mgr->GetCurrentFramebuffer(vk_framebuffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    VkClearValue clear_values[2];
    VkRenderPassBeginInfo render_pass_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    clear_values[0] = {};
    clear_values[0].color.float32[0] = 0.6f;
    clear_values[0].color.float32[1] = 0.6f;
    clear_values[0].color.float32[2] = 0.6f;
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

    uint32_t dynamic_offset = _current_buffer * static_cast<uint32_t>(_vk_uniform_frame_size);
    vkCmdBindDescriptorSets(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline_layout, 0, 1,
                            &_vk_descriptor_set, 1, &dynamic_offset);
    vkCmdBindPipeline(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline);

    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdPushConstants(vk_render_command_buffer, _vk_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &_model_mat);
    _model->Draw(vk_render_command_buffer);

    vkCmdEndRenderPass(vk_render_command_buffer);
    if (VK_SUCCESS != vkEndCommandBuffer(vk_render_command_buffer)) {
        logger.LogFatalError("Failed to end command buffer");
        return false;
    }

    _globe_submit_mgr->InsertPresentCommandsToBuffer(vk_render_command_buffer);
    _globe_submit_mgr->SubmitAndPresent(VK_NULL_HANDLE);

    return GlobeApp::Draw();
}

static SimpleModelApp *g_app = nullptr;

GLOBE_APP_MAIN() {
    GlobeInitStruct init_struct = {};
    GLOBE_APP_MAIN_BEGIN(init_struct)
    init_struct.app_name = "Globe App - Simple Model GLM Sample";
    init_struct.version.major = 0;
    init_struct.version.minor = 1;
    init_struct.version.patch = 0;
    init_struct.width = 500;
    init_struct.height = 500;
    init_struct.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    init_struct.num_swapchain_buffers = 3;
    init_struct.ideal_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    init_struct.secondary_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    g_app = new SimpleModelApp();
    g_app->Init(init_struct);
    g_app->Run();
    g_app->Exit();

    GLOBE_APP_MAIN_END(0)
}
