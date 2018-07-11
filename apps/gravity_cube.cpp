/*
 * Copyright (c) 2018 The Khronos Group Inc.
 * Copyright (c) 2018 Valve Corporation
 * Copyright (c) 2018 LunarG, Inc.
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
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Ian Elliott <ian@LunarG.com>
 * Author: Ian Elliott <ianelliott@google.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Gwan-gyeong Mun <elongbug@gmail.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: Bill Hollings <bill.hollings@brenwill.com>
 */

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
#include "gravity/linmath.h"
#include "gravity/object_type_string_helper.h"
#include "gravity/gravity_logger.hpp"
#include "gravity/gravity_event.hpp"
#include "gravity/gravity_window.hpp"
#include "gravity/gravity_submit_manager.hpp"
#include "gravity/gravity_app.hpp"
#include "gravity/gravity_main.hpp"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

struct vktexgravity_vs_uniform
{
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};


//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
// clang-format off
static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};
// clang-format on

class CubeApp : public GravityApp
{
  public:
    CubeApp();
    ~CubeApp();

  protected:
    virtual bool Setup();
    virtual void CleanupCommandObjects(bool is_resize);
    virtual bool Draw();

  private:
    bool BuildDrawCmdBuffer(uint32_t framebuffer_index);
    bool LoadTextures();
    bool LoadTextureFromFile(const std::string &filename, VkImageTiling tiling, VkImageUsageFlags usage_flags,
                             GravityTexture &texture);
    bool LoadTextureFromFile(const std::string &filename, GravityTexture &target_texture,
                             bool &uses_staging, GravityTexture &staging_texture);
    virtual void HandleEvent(GravityEvent& event);

    bool _uses_staging_texture;
    mat4x4 _projection_matrix;
    mat4x4 _view_matrix;
    mat4x4 _model_matrix;
    std::vector<GravityTexture> _textures;
    GravityTexture _staging_texture;
    VkDescriptorSetLayout _vk_desc_set_layout;
    VkPipelineLayout _vk_pipeline_layout;
    VkPipelineCache _vk_pipeline_cache;
    VkPipeline _vk_pipeline;
    VkShaderModule _vk_vert_shader_module;
    VkShaderModule _vk_frag_shader_module;
    VkDescriptorPool _vk_desc_pool;

    float _spin_angle;
    float _spin_increment;
};

CubeApp::CubeApp()
{
    _uses_staging_texture = false;
    _vk_desc_set_layout = VK_NULL_HANDLE;
    _vk_pipeline_layout = VK_NULL_HANDLE;
    _vk_pipeline_cache = VK_NULL_HANDLE;
    _vk_pipeline = VK_NULL_HANDLE;
    _vk_vert_shader_module = VK_NULL_HANDLE;
    _vk_frag_shader_module = VK_NULL_HANDLE;
    _vk_desc_pool = VK_NULL_HANDLE;

#if defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    _spin_angle = 0.4f;
#else
    _spin_angle = 4.0f;
#endif
    _spin_increment = 0.2f;

    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    mat4x4_perspective(_projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    _projection_matrix[1][1] *= -1; // Flip projection matrix from GL to Vulkan orientation.

    mat4x4_look_at(_view_matrix, eye, origin, up);
    mat4x4_identity(_model_matrix);
}

CubeApp::~CubeApp()
{
}

bool CubeApp::BuildDrawCmdBuffer(uint32_t framebuffer_index)
{
    GravityLogger &logger = GravityLogger::getInstance();
    VkCommandBuffer cmd_buf;
    VkFramebuffer frame_buf;
    _gravity_submit_mgr->GetRenderCommandBuffer(framebuffer_index, cmd_buf);
    _gravity_submit_mgr->GetFramebuffer(framebuffer_index, frame_buf);

    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkClearValue clear_values[2] = {{}, {}};
    VkRenderPassBeginInfo rp_begin = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = nullptr;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmd_buf_info.pInheritanceInfo = nullptr;
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = nullptr;
    rp_begin.renderPass = _vk_render_pass;
    rp_begin.framebuffer = frame_buf;
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = _width;
    rp_begin.renderArea.extent.height = _height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buf, &cmd_buf_info)) {
        std::string error_message = "Failed to begin command buffer for draw commands for framebuffer ";
        error_message += std::to_string(framebuffer_index);
        logger.LogFatalError(error_message);
    }
    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline_layout, 0, 1,
                            &_swapchain_resources[framebuffer_index].descriptor_set, 0, nullptr);
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)_height;
    viewport.width = (float)_width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = _width;
    scissor.extent.height = _height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);

    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmd_buf);

    _gravity_submit_mgr->InsertPresentCommandsToBuffer(cmd_buf);
    if (VK_SUCCESS != vkEndCommandBuffer(cmd_buf)) {
        std::string error_message = "Failed to end command buffer for draw commands for framebuffer ";
        error_message += std::to_string(framebuffer_index);
        logger.LogFatalError(error_message);
    }

    return true;
}


bool CubeApp::Setup()
{
    GravityLogger &logger = GravityLogger::getInstance();

    if (!GravityApp::PreSetup())
    {
        return false;
    }

    if (!_is_minimized) {
        LoadTextures();

        uint8_t *pData;
        mat4x4 MVP, VP;
        struct vktexgravity_vs_uniform data;

        mat4x4_mul(VP, _projection_matrix, _view_matrix);
        mat4x4_mul(MVP, VP, _model_matrix);
        memcpy(data.mvp, MVP, sizeof(MVP));

        for (unsigned int i = 0; i < 12 * 3; i++)
        {
            data.position[i][0] = g_vertex_buffer_data[i * 3];
            data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
            data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
            data.position[i][3] = 1.0f;
            data.attr[i][0] = g_uv_buffer_data[2 * i];
            data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
            data.attr[i][2] = 0;
            data.attr[i][3] = 0;
        }

        VkBufferCreateInfo buffer_create_info  = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.size = sizeof(data);

        for (unsigned int i = 0; i < _swapchain_count; i++)
        {
            if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, nullptr, &_swapchain_resources[i].uniform_buffer)) {
                std::string error_message = "Failed to create buffer for swapchain image ";
                error_message += std::to_string(i);
                logger.LogFatalError(error_message);
                return false;
            }

            VkMemoryRequirements memory_requirements = {};
            vkGetBufferMemoryRequirements(_vk_device, _swapchain_resources[i].uniform_buffer, &memory_requirements);

            VkMemoryAllocateInfo memory_alloc_info = {};
            memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_alloc_info.pNext = NULL;
            memory_alloc_info.allocationSize = memory_requirements.size;
            memory_alloc_info.memoryTypeIndex = 0;

            if (!SelectMemoryTypeUsingRequirements(memory_requirements, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), memory_alloc_info.memoryTypeIndex))
            {
                logger.LogFatalError("Failed to find memory type supporting necessary buffer requirements");
                return false;
            }

            if (VK_SUCCESS != vkAllocateMemory(_vk_device, &memory_alloc_info, NULL, &_swapchain_resources[i].uniform_memory)) {
                logger.LogFatalError("Failed to allocate memory for buffer");
                return false;
            }

            if (VK_SUCCESS != vkMapMemory(_vk_device, _swapchain_resources[i].uniform_memory, 0, VK_WHOLE_SIZE, 0, (void **)&pData)) {
                logger.LogFatalError("Failed to map memory for buffer");
                return false;
            }

            memcpy(pData, &data, sizeof data);

            vkUnmapMemory(_vk_device, _swapchain_resources[i].uniform_memory);

            if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _swapchain_resources[i].uniform_buffer,
                                    _swapchain_resources[i].uniform_memory, 0)) {
                logger.LogFatalError("Failed to find memory type supporting necessary buffer requirements");
                return false;
            }
        }

        VkDescriptorSetLayoutBinding layout_bindings[2] = {{}, {}};
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layout_bindings[0].pImmutableSamplers = nullptr;
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = nullptr;
        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 2;
        descriptor_layout.pBindings = layout_bindings;

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(_vk_device, &descriptor_layout, nullptr, &_vk_desc_set_layout)) {
            logger.LogFatalError("Failed to create descriptor set layout");
            return false;
        }

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &_vk_desc_set_layout;

        if (VK_SUCCESS != vkCreatePipelineLayout(_vk_device, &pPipelineLayoutCreateInfo, nullptr, &_vk_pipeline_layout)) {
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
        VkAttachmentDescription attachments[2] = {{}, {}};
        VkAttachmentReference color_reference = {};
        VkAttachmentReference depth_reference = {};
        VkSubpassDescription subpass = {};
        VkRenderPassCreateInfo rp_info = {};
        attachments[0].format = _gravity_submit_mgr->GetSwapchainVkFormat();
        attachments[0].flags = 0;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[1].format = _depth_buffer.vk_format;
        attachments[1].flags = 0;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depth_reference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.flags = 0;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 0;
        rp_info.pDependencies = nullptr;

        if (VK_SUCCESS != vkCreateRenderPass(_vk_device, &rp_info, NULL, &_vk_render_pass)) {
            logger.LogFatalError("Failed to create renderpass");
            return false;
        }

        VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = {};
        VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
        VkPipelineVertexInputStateCreateInfo pipline_vert_input_state_create_info = {};
        VkPipelineInputAssemblyStateCreateInfo pipline_input_assembly_state_create_info = {};
        VkPipelineRasterizationStateCreateInfo pipeline_raster_state_create_info = {};
        VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
        VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
        VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
        VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
        VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
        VkPipelineDynamicStateCreateInfo dynamicState = {};

        memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pDynamicStates = dynamicStateEnables;

        gfx_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gfx_pipeline_create_info.layout = _vk_pipeline_layout;

        pipline_vert_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        pipline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        pipeline_raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_raster_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        pipeline_raster_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_raster_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_raster_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_raster_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_raster_state_create_info.lineWidth = 1.0f;

        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState att_state[1];
        memset(att_state, 0, sizeof(att_state));
        att_state[0].colorWriteMask = 0xf;
        att_state[0].blendEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.attachmentCount = 1;
        pipeline_color_blend_state_create_info.pAttachments = att_state;

        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.viewportCount = 1;
        dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
        pipeline_viewport_state_create_info.scissorCount = 1;
        dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

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

        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pSampleMask = nullptr;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        const uint32_t vs_code[] = {
#include "gravity_cube.vert.inc"
        };
        const uint32_t fs_code[] = {
#include "gravity_cube.frag.inc"
        };

        VkShaderModuleCreateInfo moduleCreateInfo;
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pNext = NULL;
        moduleCreateInfo.flags = 0;
        moduleCreateInfo.codeSize = sizeof(vs_code);
        moduleCreateInfo.pCode = vs_code;
        if (VK_SUCCESS != vkCreateShaderModule(_vk_device, &moduleCreateInfo, nullptr, &_vk_vert_shader_module)) {
            logger.LogFatalError("Failed to create vertex shader module");
            return false;
        }
        moduleCreateInfo.codeSize = sizeof(fs_code);
        moduleCreateInfo.pCode = fs_code;
        if (VK_SUCCESS != vkCreateShaderModule(_vk_device, &moduleCreateInfo, nullptr, &_vk_frag_shader_module)) {
            logger.LogFatalError("Failed to create fragment shader module");
            return false;
        }

        // Two stages: vs and fs
        VkPipelineShaderStageCreateInfo shaderStages[2];
        memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = _vk_vert_shader_module;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = _vk_frag_shader_module;
        shaderStages[1].pName = "main";

        pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        if (VK_SUCCESS != vkCreatePipelineCache(_vk_device, &pipeline_cache_create_info, nullptr, &_vk_pipeline_cache)) {
            logger.LogFatalError("Failed to create pipeline cache");
            return false;
        }

        gfx_pipeline_create_info.pVertexInputState = &pipline_vert_input_state_create_info;
        gfx_pipeline_create_info.pInputAssemblyState = &pipline_input_assembly_state_create_info;
        gfx_pipeline_create_info.pRasterizationState = &pipeline_raster_state_create_info;
        gfx_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        gfx_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        gfx_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        gfx_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        gfx_pipeline_create_info.stageCount = ARRAY_SIZE(shaderStages);
        gfx_pipeline_create_info.pStages = shaderStages;
        gfx_pipeline_create_info.renderPass = _vk_render_pass;
        gfx_pipeline_create_info.pDynamicState = &dynamicState;

        if (VK_SUCCESS != vkCreateGraphicsPipelines(_vk_device, _vk_pipeline_cache, 1, &gfx_pipeline_create_info, nullptr, &_vk_pipeline)) {
            logger.LogFatalError("Failed to create graphics pipeline");
            return false;
        }

        vkDestroyShaderModule(_vk_device, _vk_frag_shader_module, nullptr);
        vkDestroyShaderModule(_vk_device, _vk_vert_shader_module, nullptr);

        VkDescriptorPoolSize type_counts[2] = {{}, {}};
        type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        type_counts[0].descriptorCount = _swapchain_count;
        type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        type_counts[1].descriptorCount = _swapchain_count;
        VkDescriptorPoolCreateInfo descriptor_pool = {};
        descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool.pNext = nullptr;
        descriptor_pool.maxSets = _swapchain_count;
        descriptor_pool.poolSizeCount = 2;
        descriptor_pool.pPoolSizes = type_counts;

        if (VK_SUCCESS != vkCreateDescriptorPool(_vk_device, &descriptor_pool, nullptr, &_vk_desc_pool)) {
            logger.LogFatalError("Failed to create descriptor pool");
            return false;
        }

        VkDescriptorImageInfo descriptor_image_info = {};
        VkWriteDescriptorSet writes[2] = {{}, {}};

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = _vk_desc_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &_vk_desc_set_layout;

        VkDescriptorBufferInfo buffer_info;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(struct vktexgravity_vs_uniform);

        descriptor_image_info.sampler = _textures[0].vk_sampler;
        descriptor_image_info.imageView = _textures[0].vk_image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &buffer_info;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &descriptor_image_info;

        for (unsigned int i = 0; i < _swapchain_count; i++)
        {
            if (VK_SUCCESS != vkAllocateDescriptorSets(_vk_device, &alloc_info, &_swapchain_resources[i].descriptor_set)) {
                logger.LogFatalError("Failed to allocate descriptor set");
                return false;
            }
            buffer_info.buffer = _swapchain_resources[i].uniform_buffer;
            writes[0].dstSet = _swapchain_resources[i].descriptor_set;
            writes[1].dstSet = _swapchain_resources[i].descriptor_set;
            vkUpdateDescriptorSets(_vk_device, 2, writes, 0, nullptr);
        }

        _gravity_submit_mgr->AttachRenderPassAndDepthBuffer(_vk_render_pass, _depth_buffer.vk_image_view);
        for (uint32_t i = 0; i < _swapchain_count; i++)
        {
            if (!BuildDrawCmdBuffer(i)) {
                return false;
            }
        }
    }
    _current_buffer = 0;

    if (!GravityApp::PostSetup())
    {
        return false;
    }

    if (_uses_staging_texture &&
        VK_NULL_HANDLE != _staging_texture.vk_device_memory &&
        VK_NULL_HANDLE != _staging_texture.vk_image)
    {
        vkFreeMemory(_vk_device, _staging_texture.vk_device_memory, NULL);
        vkDestroyImage(_vk_device, _staging_texture.vk_image, NULL);
        _staging_texture = {};
        _uses_staging_texture = false;
    }

    return true;
}

void CubeApp::CleanupCommandObjects(bool is_resize)
{
    if (!_is_minimized)
    {
        for (auto texture : _textures)
        {
            vkDestroySampler(_vk_device, texture.vk_sampler, nullptr);
            vkDestroyImageView(_vk_device, texture.vk_image_view, nullptr);
            vkDestroyImage(_vk_device, texture.vk_image, nullptr);
            vkFreeMemory(_vk_device, texture.vk_device_memory, nullptr);
        }
        _textures.clear();
        if (_uses_staging_texture &&
            VK_NULL_HANDLE != _staging_texture.vk_device_memory &&
            VK_NULL_HANDLE != _staging_texture.vk_image)
        {
            vkFreeMemory(_vk_device, _staging_texture.vk_device_memory, NULL);
            vkDestroyImage(_vk_device, _staging_texture.vk_image, NULL);
            _staging_texture = {};
            _uses_staging_texture = false;
        }
        vkDestroyDescriptorPool(_vk_device, _vk_desc_pool, NULL);

        vkDestroyPipeline(_vk_device, _vk_pipeline, NULL);
        vkDestroyPipelineCache(_vk_device, _vk_pipeline_cache, NULL);
        vkDestroyRenderPass(_vk_device, _vk_render_pass, NULL);
        vkDestroyPipelineLayout(_vk_device, _vk_pipeline_layout, NULL);
        vkDestroyDescriptorSetLayout(_vk_device, _vk_desc_set_layout, NULL);
    }
    GravityApp::CleanupCommandObjects(is_resize);
}

bool CubeApp::LoadTextureFromFile(const std::string &filename, GravityTexture &gravity_texture,
                                  bool &uses_staging, GravityTexture &staging_texture)
{
    GravityLogger &logger = GravityLogger::getInstance();

    VkFormatProperties vk_format_props;
    vkGetPhysicalDeviceFormatProperties(_vk_phys_device, gravity_texture.vk_format, &vk_format_props);

    VkImageUsageFlags loading_texture_usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT;
    GravityTexture *target_texture = &gravity_texture;
    uses_staging = false;
    if ((vk_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !_uses_staging_buffer)
    {
        loading_texture_usage_flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        target_texture = &staging_texture;
        uses_staging = true;
    }

    // Textures loaded from files are treated as RGBA with 8-bit components
    target_texture->vk_format = VK_FORMAT_R8G8B8A8_UNORM;

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename)].UTF8String;
#endif

#ifdef __ANDROID__
#include <lunarg.ppm.h>
    char *cPtr;
    cPtr = (char *)lunarg_ppm;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "P6\n", 3))
    {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", &target_texture->width, &target_texture->height);
    target_texture->raw_data.resize(target_texture->width * target_texture->height * 4);
    uint8_t *rgba_data = target_texture->raw_data.data();
    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "255\n", 4))
    {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;

    uint8_t *row_ptr = rgba_data;
    for (int y = 0; y < target_texture->height; y++)
    {
        for (int x = 0; x < target_texture->width; x++)
        {
            memcpy(row_ptr, cPtr, 3);
            row_ptr[3] = 255;
            row_ptr += 4;
            cPtr += 3;
        }
    }
#else
    FILE *fPtr = fopen(filename.c_str(), "rb");
    char header[256], *cPtr, *tmp;

    if (!fPtr)
        return false;

    cPtr = fgets(header, 256, fPtr); // P6
    if (cPtr == NULL || strncmp(header, "P6\n", 3))
    {
        fclose(fPtr);
        return false;
    }

    do
    {
        cPtr = fgets(header, 256, fPtr);
        if (cPtr == NULL)
        {
            fclose(fPtr);
            return false;
        }
    } while (!strncmp(header, "#", 1));

    sscanf(header, "%u %u", &target_texture->width, &target_texture->height);
    target_texture->raw_data.resize(target_texture->width * target_texture->height * 4);
    uint8_t *rgba_data = target_texture->raw_data.data();
    tmp = fgets(header, 256, fPtr); // Format
    (void)tmp;
    if (cPtr == NULL || strncmp(header, "255\n", 3))
    {
        fclose(fPtr);
        return false;
    }

    uint8_t *row_ptr = rgba_data;
    for (uint32_t y = 0; y < target_texture->height; y++)
    {
        for (uint32_t x = 0; x < target_texture->width; x++)
        {
            size_t s = fread(row_ptr, 3, 1, fPtr);
            (void)s;
            row_ptr[3] = 255;
            row_ptr += 4;
        }
    }
    fclose(fPtr);
#endif

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = target_texture->vk_format;
    image_create_info.extent.width = target_texture->width;
    image_create_info.extent.height = target_texture->height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = loading_texture_usage_flags;
    image_create_info.flags = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    if (VK_SUCCESS != vkCreateImage(_vk_device, &image_create_info, NULL, &target_texture->vk_image))
    {
        std::string error_message = "Failed to load texture from file \"";
        error_message += filename;
        error_message += "\"";
        logger.LogFatalError(error_message);
        return false;
    }

    VkMemoryRequirements texture_mem_reqs = {};
    vkGetImageMemoryRequirements(_vk_device, target_texture->vk_image, &texture_mem_reqs);
    target_texture->vk_mem_alloc_info = {};
    target_texture->vk_mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    target_texture->vk_mem_alloc_info.pNext = nullptr;
    target_texture->vk_mem_alloc_info.allocationSize = texture_mem_reqs.size;
    if (!SelectMemoryTypeUsingRequirements(texture_mem_reqs, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), target_texture->vk_mem_alloc_info.memoryTypeIndex))
    {
        logger.LogFatalError("Failed to find memory type supporting necessary texture requirements");
        return false;
    }

    if (VK_SUCCESS != vkAllocateMemory(_vk_device, &target_texture->vk_mem_alloc_info, nullptr, &target_texture->vk_device_memory))
    {
        logger.LogFatalError("Failed to allocate memory for texture image");
        return false;
    }

    if (VK_SUCCESS != vkBindImageMemory(_vk_device, target_texture->vk_image, target_texture->vk_device_memory, 0))
    {
        logger.LogFatalError("Failed to binding memory to texture image");
        return false;
    }

    VkImageSubresource image_subresource = {};
    image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource.mipLevel = 0;
    image_subresource.arrayLayer = 0;
    VkSubresourceLayout vk_subresource_layout;

    vkGetImageSubresourceLayout(_vk_device, target_texture->vk_image, &image_subresource, &vk_subresource_layout);

    void *data;
    if (VK_SUCCESS != vkMapMemory(_vk_device, target_texture->vk_device_memory, 0, target_texture->vk_mem_alloc_info.allocationSize, 0, &data))
    {
        logger.LogFatalError("Failed to map memory for copying over texture image");
        return false;
    }
    uint8_t *data_ptr = reinterpret_cast<uint8_t *>(data);
    row_ptr = target_texture->raw_data.data();
    uint32_t from_size = target_texture->width * 4;
    uint32_t to_size = target_texture->width * 4;
    for (uint32_t y = 0; y < target_texture->height; y++)
    {
        memcpy(data_ptr, row_ptr, from_size);
        row_ptr += from_size;
        data_ptr += vk_subresource_layout.rowPitch;
    }
    vkUnmapMemory(_vk_device, target_texture->vk_device_memory);

    target_texture->raw_data.clear();
    target_texture->vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // If we need to place this in tiled memory, we need to use a staging texture.
    if (uses_staging)
    {
        if (!TransitionVkImageLayout(_vk_cmd_buffer, staging_texture.vk_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<VkAccessFlagBits>(0),
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT))
        {
            logger.LogFatalError("Failed to transition staging image to transfer format");
            return false;
        }

        gravity_texture.width = staging_texture.width;
        gravity_texture.height = staging_texture.height;
        gravity_texture.vk_format = staging_texture.vk_format;
        gravity_texture.vk_mem_alloc_info = staging_texture.vk_mem_alloc_info;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (VK_SUCCESS != vkCreateImage(_vk_device, &image_create_info, NULL, &gravity_texture.vk_image))
        {
            std::string error_message = "Failed to setup optimized tiled texture target for \"";
            error_message += filename;
            error_message += "\"";
            logger.LogFatalError(error_message);
            return false;
        }
        vkGetImageMemoryRequirements(_vk_device, gravity_texture.vk_image, &texture_mem_reqs);
        target_texture->vk_mem_alloc_info.allocationSize = texture_mem_reqs.size;
        if (!SelectMemoryTypeUsingRequirements(texture_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gravity_texture.vk_mem_alloc_info.memoryTypeIndex))
        {
            logger.LogFatalError("Failed to find memory type supporting necessary tiled texture requirements");
            return false;
        }

        if (VK_SUCCESS != vkAllocateMemory(_vk_device, &gravity_texture.vk_mem_alloc_info, nullptr, &gravity_texture.vk_device_memory))
        {
            logger.LogFatalError("Failed to allocate memory for tiled texture image");
            return false;
        }

        if (VK_SUCCESS != vkBindImageMemory(_vk_device, gravity_texture.vk_image, gravity_texture.vk_device_memory, 0))
        {
            logger.LogFatalError("Failed to binding memory to tiled texture image");
            return false;
        }

        gravity_texture.vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (!TransitionVkImageLayout(_vk_cmd_buffer, gravity_texture.vk_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<VkAccessFlagBits>(0),
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT))
        {
            logger.LogFatalError("Failed to transition resulting image to accept staging content");
            return false;
        }
    }
    else
    {
        if (!TransitionVkImageLayout(_vk_cmd_buffer, gravity_texture.vk_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                     gravity_texture.vk_image_layout, static_cast<VkAccessFlagBits>(0),
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT))
        {
            logger.LogFatalError("Failed to transition texture image to shader readable format");
            return false;
        }
    }

    return true;
}

bool CubeApp::LoadTextures()
{
    GravityLogger &logger = GravityLogger::getInstance();

    _textures.resize(1);
    if (!LoadTextureFromFile("lunarg.ppm", _textures[0], _uses_staging_texture, _staging_texture))
    {
        logger.LogFatalError("Failed to load textures");
        return false;
    }

    if (_uses_staging_buffer)
    {
        VkImageCopy copy_region = {};
        copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy_region.srcOffset = {0, 0, 0};
        copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy_region.dstOffset = {0, 0, 0};
        copy_region.extent.width = _staging_texture.width;
        copy_region.extent.height = _staging_texture.height;
        copy_region.extent.depth = 1;
        vkCmdCopyImage(_vk_cmd_buffer, _staging_texture.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _textures[0].vk_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
        if (!TransitionVkImageLayout(_vk_cmd_buffer, _textures[0].vk_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     _textures[0].vk_image_layout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT))
        {
            logger.LogFatalError("Failed to transition resulting texture to shader readable after staging copy");
            return false;
        }
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
    image_view_create_info.format = _textures[0].vk_format;
    image_view_create_info.components = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    image_view_create_info.flags = 0;

    if (VK_SUCCESS != vkCreateSampler(_vk_device, &sampler_create_info, nullptr, &_textures[0].vk_sampler))
    {
        logger.LogFatalError("Failed creating texture sampler for primary texture");
        return false;
    }

    image_view_create_info.image = _textures[0].vk_image;
    if (VK_SUCCESS != vkCreateImageView(_vk_device, &image_view_create_info, nullptr, &_textures[0].vk_image_view))
    {
        logger.LogFatalError("Failed creating texture image view for primary texture");
        return false;
    }

    return true;
}

bool CubeApp::Draw()
{
    GravityLogger &logger = GravityLogger::getInstance();
    _gravity_submit_mgr->AcquireNextImageIndex(_current_buffer);

    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;

    mat4x4_mul(VP, _projection_matrix, _view_matrix);

    // Rotate around the Y axis
    mat4x4_dup(Model, _model_matrix);
    mat4x4_rotate(_model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(_spin_angle));
    mat4x4_mul(MVP, VP, _model_matrix);

    if (VK_SUCCESS != vkMapMemory(_vk_device, _swapchain_resources[_current_buffer].uniform_memory, 0, VK_WHOLE_SIZE, 0,
                      (void **)&pData)) {
        logger.LogFatalError("Failed to map uniform buffer memory");
        return false;
    }
    memcpy(pData, (const void *)&MVP[0][0], matrixSize);
    vkUnmapMemory(_vk_device, _swapchain_resources[_current_buffer].uniform_memory);

    _gravity_submit_mgr->SubmitAndPresent();
    return true;
}

void CubeApp::HandleEvent(GravityEvent& event) {
    switch (event.Type())
    {
    case GRAVITY_EVENT_KEY_RELEASE:
        switch (event._data.key)
        {
        case GRAVITY_KEYNAME_ARROW_LEFT:
            _spin_angle -= _spin_increment;
            break;
        case GRAVITY_KEYNAME_ARROW_RIGHT:
            _spin_angle += _spin_increment;
            break;
        }
        break;
    default:
        break;
    }
    GravityApp::HandleEvent(event);
}

static CubeApp *g_app = nullptr;

GRAVITY_APP_MAIN()
{
    GravityInitStruct init_struct = {};

#if 0
    GRAVITY_APP_MAIN_BEGIN(init_struct)
#else


#endif
    init_struct.app_name = "Gravity App 1 - Cube";
    init_struct.version.major = 0;
    init_struct.version.minor = 1;
    init_struct.version.patch = 0;
    init_struct.width = 500;
    init_struct.height = 500;
    init_struct.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    init_struct.num_swapchain_buffers = 3;
    init_struct.ideal_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    init_struct.secondary_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    g_app = new CubeApp();
    g_app->Init(init_struct);
    g_app->Run();
    g_app->Exit();

    GRAVITY_APP_MAIN_END(0)
}
