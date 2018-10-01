/*
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
 * Author: Mark Young <marky@lunarg.com>
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

struct VulkanBuffer {
    VkBuffer vk_buffer;
    VkDeviceMemory vk_memory;
    VkDeviceSize vk_size;
};

struct VulkanTarget {
    uint32_t width;
    uint32_t height;
    VkRenderPass vk_render_pass;
    VkFramebuffer vk_framebuffer;
    VkSemaphore vk_semaphore;
    VkDescriptorSetLayout vk_descriptor_set_layout;
    VkPipelineLayout vk_pipeline_layout;
    VkDescriptorPool vk_descriptor_pool;
    VkDescriptorSet vk_descriptor_set;
    VkPipeline vk_pipeline;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    VkDeviceSize vk_uniform_vec4_alignment;
    VulkanBuffer uniform_buffer;
    uint8_t *uniform_mapped_data;
    uint32_t num_vertices;
    VkCommandPool _vk_command_pool;
    std::vector<VkCommandBuffer> _vk_command_buffers;
};

class OffscreenRenderingApp : public GlobeApp {
   public:
    OffscreenRenderingApp();
    ~OffscreenRenderingApp();

    virtual void Cleanup() override;

   protected:
    bool CreateOffscreenTarget(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                               VkFormat vk_color_format, VkFormat vk_depth_stencil_format);
    virtual bool Setup() override;
    virtual bool Update(float diff_ms) override;
    virtual bool Draw() override;
    void UpdateEllipseCenter();

   private:
    void CleanupVulkanTarget(VulkanTarget &target);
    void DetermineNewColor();

    uint32_t _last_buffer;
    VulkanTarget _onscreen_target;
    VulkanTarget _offscreen_target;
    GlobeTexture *_offscreen_color;
    GlobeTexture *_offscreen_depth;
    int32_t _color_index;
    int32_t _selected_index;
    glm::vec4 _color_0;
    glm::vec4 _color_1;
};

void OffscreenRenderingApp::DetermineNewColor() {
    if (++_selected_index >= 18) {
        _selected_index = -1;
        if (++_color_index > 12) {
            _color_index = 0;
        }
        switch (_color_index) {
            case 0:
            default:
                _color_0 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                break;
            case 1:
                _color_0 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case 2:
                _color_0 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                break;
            case 3:
                _color_0 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                break;
            case 4:
                _color_0 = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                break;
            case 5:
                _color_0 = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            case 6:
                _color_0 = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                break;
            case 7:
                _color_0 = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                break;
            case 8:
                _color_0 = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case 9:
                _color_0 = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                break;
            case 10:
                _color_0 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                break;
            case 11:
                _color_0 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                break;
            case 12:
                _color_0 = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                _color_1 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                break;
        }
    }
}

OffscreenRenderingApp::OffscreenRenderingApp() {
    _onscreen_target = {};
    _offscreen_target = {};
    _offscreen_color = nullptr;
    _offscreen_depth = nullptr;
    _selected_index = 0xFFFF;
    _color_index = 0xFFFF;
    _color_0 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    _color_1 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    DetermineNewColor();
}

OffscreenRenderingApp::~OffscreenRenderingApp() { Cleanup(); }

void OffscreenRenderingApp::CleanupVulkanTarget(VulkanTarget &target) {
    if (VK_NULL_HANDLE != target.vk_pipeline) {
        vkDestroyPipeline(_vk_device, target.vk_pipeline, nullptr);
        target.vk_pipeline = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vk_semaphore) {
        vkDestroySemaphore(_vk_device, target.vk_semaphore, nullptr);
        target.vk_semaphore = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vk_framebuffer) {
        vkDestroyFramebuffer(_vk_device, target.vk_framebuffer, nullptr);
        target.vk_framebuffer = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vk_render_pass) {
        vkDestroyRenderPass(_vk_device, target.vk_render_pass, nullptr);
        target.vk_render_pass = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vk_pipeline_layout) {
        vkDestroyPipelineLayout(_vk_device, target.vk_pipeline_layout, nullptr);
        target.vk_pipeline_layout = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.uniform_buffer.vk_buffer) {
        vkUnmapMemory(_vk_device, target.uniform_buffer.vk_memory);
        vkDestroyBuffer(_vk_device, target.uniform_buffer.vk_buffer, nullptr);
        target.uniform_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.index_buffer.vk_memory) {
        _globe_resource_mgr->FreeDeviceMemory(target.index_buffer.vk_memory);
        target.index_buffer.vk_memory = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vertex_buffer.vk_memory) {
        _globe_resource_mgr->FreeDeviceMemory(target.vertex_buffer.vk_memory);
        target.vertex_buffer.vk_memory = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.index_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, target.index_buffer.vk_buffer, nullptr);
        target.index_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vertex_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, target.vertex_buffer.vk_buffer, nullptr);
        target.vertex_buffer.vk_buffer = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vk_descriptor_set_layout) {
        vkDestroyDescriptorSetLayout(_vk_device, target.vk_descriptor_set_layout, nullptr);
        target.vk_descriptor_set_layout = VK_NULL_HANDLE;
    }
    if (target._vk_command_buffers.size() > 0) {
        vkFreeCommandBuffers(_vk_device, target._vk_command_pool, static_cast<uint32_t>(target._vk_command_buffers.size()),
                             target._vk_command_buffers.data());
        target._vk_command_buffers.clear();
    }
    vkDestroyCommandPool(_vk_device, target._vk_command_pool, nullptr);
    target._vk_command_pool = VK_NULL_HANDLE;
}

void OffscreenRenderingApp::Cleanup() {
    GlobeApp::PreCleanup();
    CleanupVulkanTarget(_offscreen_target);
    CleanupVulkanTarget(_onscreen_target);
    GlobeApp::PostCleanup();
}

// Textured triangles drawn in a cube-isometric style view (using the texture from the offscreen
// render target).
static const float g_screen_textured_quad_data[] = {
    0.0f,  0.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // Vert 0
    -0.7f, -0.3f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // Vert 1
    -0.7f, 0.4f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Vert 2
    0.0f,  0.7f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Vert 3
    0.7f,  -0.3f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // Vert 4
    0.0f,  0.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // Vert 5
    0.0f,  0.7f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Vert 6
    0.7f,  0.4f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Vert 7
    0.0f,  -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // Vert 8
    -0.7f, -0.3f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // Vert 9
    0.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Vert 10
    0.7f,  -0.3f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Vert 11
};
static const uint32_t g_screen_textured_quad_index_data[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11};

// Offscreen triangles to render (in rough infinity sign).
static float g_offscreen_color_quad_data[] = {
    0.5f,  0.0f,  0.0f, 1.0f,  // 0 (right center)
    0.0f,  0.0f,  0.0f, 1.0f,  // 1
    0.1f,  -0.4f, 0.0f, 1.0f,  // 2
    0.5f,  -0.5f, 0.0f, 1.0f,  // 3
    0.9f,  -0.4f, 0.0f, 1.0f,  // 4
    1.0f,  0.0f,  0.0f, 1.0f,  // 5
    0.9f,  0.4f,  0.0f, 1.0f,  // 6
    0.5f,  0.5f,  0.0f, 1.0f,  // 7
    0.1f,  0.4f,  0.0f, 1.0f,  // 8
    -0.5f, 0.0f,  0.0f, 1.0f,  // 9 (left center)
    0.0f,  0.0f,  0.0f, 1.0f,  // 10
    -0.1f, -0.4f, 0.0f, 1.0f,  // 11
    -0.5f, -0.5f, 0.0f, 1.0f,  // 12
    -0.9f, -0.4f, 0.0f, 1.0f,  // 13
    -1.0f, 0.0f,  0.0f, 1.0f,  // 14
    -0.9f, 0.4f,  0.0f, 1.0f,  // 15
    -0.5f, 0.5f,  0.0f, 1.0f,  // 16
    -0.1f, 0.4f,  0.0f, 1.0f,  // 17
    0.0f,  0.0f,  0.0f, 1.0f,  // 18
};
static const uint32_t g_offscreen_color_quad_index_data[] = {0, 2,  1,  0, 3,  2,  0, 4,  3,  0, 5,  4,  0, 6,  5,
                                                             0, 7,  6,  0, 8,  7,  0, 1,  8,  9, 10, 11, 9, 11, 12,
                                                             9, 12, 13, 9, 13, 14, 9, 14, 15, 9, 15, 16, 9, 16, 17,
                                                             9, 17, 18};

bool OffscreenRenderingApp::CreateOffscreenTarget(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                                  VkFormat vk_color_format, VkFormat vk_depth_stencil_format) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    uint8_t *mapped_data;

    _offscreen_target.width = width;
    _offscreen_target.height = height;

    // Create an offscreen color and depth render target
    _offscreen_color = _globe_resource_mgr->CreateRenderTargetTexture(vk_command_buffer, _offscreen_target.width,
                                                                      _offscreen_target.height, vk_color_format);
    if (nullptr == _offscreen_color) {
        logger.LogError("Failed creating color render target texture");
        return false;
    }

    if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
        _offscreen_depth = _globe_resource_mgr->CreateRenderTargetTexture(
            vk_command_buffer, _offscreen_target.width, _offscreen_target.height, vk_depth_stencil_format);
        if (nullptr == _offscreen_depth) {
            logger.LogError("Failed creating depth render target texture");
            return false;
        }
    } else {
        _offscreen_depth = nullptr;
    }

    // We need to define two subpass dependencies.  One that forces all work to complete
    // Before we can attach the color buffer, and one that forces the color buffer to be
    // fully attached and transitioned before we begin reading from it.
    std::vector<VkSubpassDependency> offscreen_subpass_dependencies;
    offscreen_subpass_dependencies.resize(2);
    offscreen_subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    offscreen_subpass_dependencies[0].dstSubpass = 0;
    offscreen_subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    offscreen_subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    offscreen_subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    offscreen_subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    offscreen_subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    offscreen_subpass_dependencies[1].srcSubpass = 0;
    offscreen_subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    offscreen_subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    offscreen_subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    offscreen_subpass_dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    offscreen_subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    offscreen_subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Generate the appropriate attachment information for each
    std::vector<VkAttachmentDescription> offscreen_attachment_desc;
    offscreen_attachment_desc.push_back(_offscreen_color->GenVkAttachmentDescription());
    VkAttachmentReference offscreen_color_ref = _offscreen_color->GenVkAttachmentReference(0);
    VkAttachmentReference offscreen_depth_ref;
    if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
        offscreen_attachment_desc.push_back(_offscreen_depth->GenVkAttachmentDescription());
        offscreen_depth_ref = _offscreen_depth->GenVkAttachmentReference(1);
    }

    // Create a subpass description for the offscreen targets
    VkSubpassDescription offscreen_subpass_description = {};
    offscreen_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    offscreen_subpass_description.colorAttachmentCount = 1;
    offscreen_subpass_description.pColorAttachments = &offscreen_color_ref;
    if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
        offscreen_subpass_description.pDepthStencilAttachment = &offscreen_depth_ref;
    } else {
        offscreen_subpass_description.pDepthStencilAttachment = nullptr;
    }

    // Create the offscreen renderpass
    VkRenderPassCreateInfo offscreen_render_pass_create_info = {};
    offscreen_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    offscreen_render_pass_create_info.attachmentCount = static_cast<uint32_t>(offscreen_attachment_desc.size());
    offscreen_render_pass_create_info.pAttachments = offscreen_attachment_desc.data();
    offscreen_render_pass_create_info.subpassCount = 1;
    offscreen_render_pass_create_info.pSubpasses = &offscreen_subpass_description;
    offscreen_render_pass_create_info.dependencyCount = static_cast<uint32_t>(offscreen_subpass_dependencies.size());
    offscreen_render_pass_create_info.pDependencies = offscreen_subpass_dependencies.data();
    if (VK_SUCCESS != vkCreateRenderPass(_vk_device, &offscreen_render_pass_create_info, nullptr,
                                         &_offscreen_target.vk_render_pass)) {
        logger.LogFatalError("Failed to create offscreen render pass");
        return false;
    }

    // Now attach the offscreen images to a framebuffer
    std::vector<VkImageView> image_views;
    image_views.push_back(_offscreen_color->GetVkImageView());
    if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
        image_views.push_back(_offscreen_depth->GetVkImageView());
    }
    VkFramebufferCreateInfo offscreen_framebuf_create_info = {};
    offscreen_framebuf_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    offscreen_framebuf_create_info.pNext = nullptr;
    offscreen_framebuf_create_info.renderPass = _offscreen_target.vk_render_pass;
    offscreen_framebuf_create_info.attachmentCount = static_cast<uint32_t>(image_views.size());
    offscreen_framebuf_create_info.pAttachments = image_views.data();
    offscreen_framebuf_create_info.width = _offscreen_target.width;
    offscreen_framebuf_create_info.height = _offscreen_target.height;
    offscreen_framebuf_create_info.layers = 1;
    if (VK_SUCCESS !=
        vkCreateFramebuffer(_vk_device, &offscreen_framebuf_create_info, nullptr, &_offscreen_target.vk_framebuffer)) {
        logger.LogFatalError("Failed to create offscreen framebuffer");
        return false;
    }

    // Create semaphores to synchronize usage of the offscreen render target content
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    if (VK_SUCCESS != vkCreateSemaphore(_vk_device, &semaphore_create_info, nullptr, &_offscreen_target.vk_semaphore)) {
        logger.LogFatalError("Failed to create offscreen semaphore");
        return false;
    }

    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
    VkDescriptorSetLayoutBinding cur_binding = {};
    cur_binding.binding = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    cur_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    cur_binding.descriptorCount = 1;
    cur_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cur_binding.pImmutableSamplers = nullptr;
    descriptor_set_layout_bindings.push_back(cur_binding);

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {};
    descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout.pNext = nullptr;
    descriptor_set_layout.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    descriptor_set_layout.pBindings = descriptor_set_layout_bindings.data();
    if (VK_SUCCESS != vkCreateDescriptorSetLayout(_vk_device, &descriptor_set_layout, nullptr,
                                                  &_offscreen_target.vk_descriptor_set_layout)) {
        logger.LogFatalError("Failed to create offscreen render target descriptor set layout");
        return false;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &_offscreen_target.vk_descriptor_set_layout;
    if (VK_SUCCESS != vkCreatePipelineLayout(_vk_device, &pipeline_layout_create_info, nullptr,
                                             &_offscreen_target.vk_pipeline_layout)) {
        logger.LogFatalError("Failed to create offscreen render target pipeline layout layout");
        return false;
    }

    // Create the offscreen vertex buffer
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.size = sizeof(g_offscreen_color_quad_data);
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;
    if (VK_SUCCESS !=
        vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_offscreen_target.vertex_buffer.vk_buffer)) {
        logger.LogFatalError("Failed to create vertex buffer");
        return false;
    }
    if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
            _offscreen_target.vertex_buffer.vk_buffer,
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            _offscreen_target.vertex_buffer.vk_memory, _offscreen_target.vertex_buffer.vk_size)) {
        logger.LogFatalError("Failed to allocate offscreen vertex buffer memory");
        return false;
    }
    if (VK_SUCCESS != vkMapMemory(_vk_device, _offscreen_target.vertex_buffer.vk_memory, 0,
                                  _offscreen_target.vertex_buffer.vk_size, 0, (void **)&mapped_data)) {
        logger.LogFatalError("Failed to map offscreen vertex buffer memory");
        return false;
    }
    memcpy(mapped_data, g_offscreen_color_quad_data, sizeof(g_offscreen_color_quad_data));
    vkUnmapMemory(_vk_device, _offscreen_target.vertex_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _offscreen_target.vertex_buffer.vk_buffer,
                                         _offscreen_target.vertex_buffer.vk_memory, 0)) {
        logger.LogFatalError("Failed to bind offscreen vertex buffer memory");
        return false;
    }

    // Create the offscreen index buffer
    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.size = sizeof(g_offscreen_color_quad_index_data);
    if (VK_SUCCESS !=
        vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_offscreen_target.index_buffer.vk_buffer)) {
        logger.LogFatalError("Failed to create offscreen index buffer");
        return false;
    }
    _offscreen_target.num_vertices = sizeof(g_offscreen_color_quad_index_data) / sizeof(uint32_t);
    if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
            _offscreen_target.index_buffer.vk_buffer,
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            _offscreen_target.index_buffer.vk_memory, _offscreen_target.index_buffer.vk_size)) {
        logger.LogFatalError("Failed to allocate offscreen index buffer memory");
        return false;
    }
    if (VK_SUCCESS != vkMapMemory(_vk_device, _offscreen_target.index_buffer.vk_memory, 0,
                                  _offscreen_target.index_buffer.vk_size, 0, (void **)&mapped_data)) {
        logger.LogFatalError("Failed to map offscreen index buffer memory");
        return false;
    }
    memcpy(mapped_data, g_offscreen_color_quad_index_data, sizeof(g_offscreen_color_quad_index_data));
    vkUnmapMemory(_vk_device, _offscreen_target.index_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _offscreen_target.index_buffer.vk_buffer,
                                         _offscreen_target.index_buffer.vk_memory, 0)) {
        logger.LogFatalError("Failed to bind offscreen index buffer memory");
        return false;
    }

    VkDeviceSize required_data_size = sizeof(float) * 8 + sizeof(int32_t);
    VkDeviceSize vk_uniform_alignment = _vk_phys_device_properties.limits.minUniformBufferOffsetAlignment;
    _offscreen_target.vk_uniform_vec4_alignment =
        (required_data_size + vk_uniform_alignment - 1) & ~(vk_uniform_alignment - 1);

    // The smallest submit size is an atom, so we need to make sure we're at least as big as that per
    // uniform buffer submission.
    if (_offscreen_target.vk_uniform_vec4_alignment < _vk_phys_device_properties.limits.nonCoherentAtomSize) {
        _offscreen_target.vk_uniform_vec4_alignment = _vk_phys_device_properties.limits.nonCoherentAtomSize;
    }

    // Create the uniform buffer containing the mvp matrix
    buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.size = _offscreen_target.vk_uniform_vec4_alignment * _swapchain_count;
    if (VK_SUCCESS !=
        vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_offscreen_target.uniform_buffer.vk_buffer)) {
        logger.LogFatalError("Failed to create offscreen uniform buffer");
        return false;
    }
    if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
            _offscreen_target.uniform_buffer.vk_buffer,
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            _offscreen_target.uniform_buffer.vk_memory, _offscreen_target.uniform_buffer.vk_size)) {
        logger.LogFatalError("Failed to allocate offscreen uniform buffer memory");
        return false;
    }
    if (VK_SUCCESS != vkMapMemory(_vk_device, _offscreen_target.uniform_buffer.vk_memory, 0,
                                  _offscreen_target.uniform_buffer.vk_size, 0,
                                  (void **)&_offscreen_target.uniform_mapped_data)) {
        logger.LogFatalError("Failed to map offscreen uniform buffer memory");
        return false;
    }
    for (uint32_t index = 0; index < _swapchain_count; ++index) {
        uint32_t offset = index * static_cast<uint32_t>(_offscreen_target.vk_uniform_vec4_alignment);
        uint8_t* mapped_addr = _offscreen_target.uniform_mapped_data + offset;
        memcpy(mapped_addr, &_color_0, sizeof(glm::vec4));
        mapped_addr += sizeof(glm::vec4);
        memcpy(mapped_addr, &_color_1, sizeof(glm::vec4));
        mapped_addr += sizeof(glm::vec4);
        memcpy(mapped_addr, &_selected_index, sizeof(int32_t));
    }
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _offscreen_target.uniform_buffer.vk_buffer,
                                         _offscreen_target.uniform_buffer.vk_memory, 0)) {
        logger.LogFatalError("Failed to bind offscreen uniform buffer memory");
        return false;
    }

    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_size.descriptorCount = 1;
    descriptor_pool_sizes.push_back(pool_size);
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = 2;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
    if (VK_SUCCESS != vkCreateDescriptorPool(_vk_device, &descriptor_pool_create_info, nullptr,
                                             &_offscreen_target.vk_descriptor_pool)) {
        logger.LogFatalError("Failed to create offscreen descriptor pool");
        return false;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = NULL;
    descriptor_set_allocate_info.descriptorPool = _offscreen_target.vk_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_offscreen_target.vk_descriptor_set_layout;
    if (VK_SUCCESS !=
        vkAllocateDescriptorSets(_vk_device, &descriptor_set_allocate_info, &_offscreen_target.vk_descriptor_set)) {
        logger.LogFatalError("Failed to allocate offscreen descriptor set");
        return false;
    }

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = _offscreen_target.uniform_buffer.vk_buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = required_data_size;

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = _offscreen_target.vk_descriptor_set;
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_sets.push_back(write_set);
    vkUpdateDescriptorSets(_vk_device, static_cast<uint32_t>(write_descriptor_sets.size()),
                           write_descriptor_sets.data(), 0, nullptr);

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
    vertex_input_binding_description.stride = sizeof(float) * 4;
    VkVertexInputAttributeDescription vertex_input_attribute_description[2];
    vertex_input_attribute_description[0] = {};
    vertex_input_attribute_description[0].binding = 0;
    vertex_input_attribute_description[0].location = 0;
    vertex_input_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[0].offset = 0;
    VkPipelineVertexInputStateCreateInfo pipline_vert_input_state_create_info = {};
    pipline_vert_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipline_vert_input_state_create_info.pNext = NULL;
    pipline_vert_input_state_create_info.flags = 0;
    pipline_vert_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipline_vert_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipline_vert_input_state_create_info.vertexAttributeDescriptionCount = 1;
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
    scissor_rect.extent.width = _offscreen_target.width;
    scissor_rect.extent.height = _offscreen_target.height;
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_offscreen_target.width;
    viewport.height = (float)_offscreen_target.height;
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

    GlobeShader *position_color_shader = _globe_resource_mgr->LoadShader("position_uniform_buf_color");
    if (nullptr == position_color_shader) {
        logger.LogFatalError("Failed to load position_uniform_buf_color shaders");
        return false;
    }
    std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
    position_color_shader->GetPipelineShaderStages(pipeline_shader_stage_create_info);

    VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = {};
    gfx_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gfx_pipeline_create_info.layout = _offscreen_target.vk_pipeline_layout;
    gfx_pipeline_create_info.pVertexInputState = &pipline_vert_input_state_create_info;
    gfx_pipeline_create_info.pInputAssemblyState = &pipline_input_assembly_state_create_info;
    gfx_pipeline_create_info.pRasterizationState = &pipeline_raster_state_create_info;
    gfx_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    gfx_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    gfx_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    gfx_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    gfx_pipeline_create_info.stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_info.size());
    gfx_pipeline_create_info.pStages = pipeline_shader_stage_create_info.data();
    gfx_pipeline_create_info.renderPass = _offscreen_target.vk_render_pass;
    gfx_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    if (VK_SUCCESS != vkCreateGraphicsPipelines(_vk_device, VK_NULL_HANDLE, 1, &gfx_pipeline_create_info, nullptr,
                                                &_offscreen_target.vk_pipeline)) {
        logger.LogFatalError("Failed to create graphics pipeline");
        return false;
    }

    _globe_resource_mgr->FreeShader(position_color_shader);

    // Create the command pool to be used by the offscreen surfaces.
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext = nullptr;
    cmd_pool_create_info.queueFamilyIndex = _globe_submit_mgr->GetGraphicsQueueIndex();
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (VK_SUCCESS !=
        vkCreateCommandPool(_vk_device, &cmd_pool_create_info, NULL, &_offscreen_target._vk_command_pool)) {
        logger.LogFatalError("Failed to create offscreen command pool");
        return false;
    }

    // Create a custom command buffer per swapchain image for the offscreen surface to
    // perform its command prior to be submitted, synced up, and then used
    uint32_t num_swapchain_images = _globe_submit_mgr->NumSwapchainImages();
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = _offscreen_target._vk_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = num_swapchain_images;
    _offscreen_target._vk_command_buffers.resize(num_swapchain_images);
    for (uint32_t cmd_buf = 0; cmd_buf < num_swapchain_images; ++cmd_buf) {
        if (VK_SUCCESS != vkAllocateCommandBuffers(_vk_device, &command_buffer_allocate_info,
                                                   &_offscreen_target._vk_command_buffers[cmd_buf])) {
            std::string error_msg = "Failed to allocate offscreen render command buffer ";
            error_msg += std::to_string(cmd_buf);
            logger.LogFatalError(error_msg);
            _offscreen_target._vk_command_buffers.resize(cmd_buf);
            return false;
        }
    }

    return true;
}

bool OffscreenRenderingApp::Setup() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    VkCommandPool vk_setup_command_pool;
    VkCommandBuffer vk_setup_command_buffer;
    if (!GlobeApp::PreSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }

    if (!_is_minimized) {
        uint8_t *mapped_data;

        if (!CreateOffscreenTarget(vk_setup_command_buffer, 500, 500, VK_FORMAT_B8G8R8A8_UNORM,
                                   VK_FORMAT_D24_UNORM_S8_UINT)) {
            logger.LogError("Failed setting up offscreen render target");
            return false;
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
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(_vk_device, &descriptor_set_layout, nullptr,
                                                      &_onscreen_target.vk_descriptor_set_layout)) {
            logger.LogFatalError("Failed to create descriptor set layout");
            return false;
        }

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &_onscreen_target.vk_descriptor_set_layout;
        if (VK_SUCCESS != vkCreatePipelineLayout(_vk_device, &pipeline_layout_create_info, nullptr,
                                                 &_onscreen_target.vk_pipeline_layout)) {
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
        if (VK_SUCCESS !=
            vkCreateRenderPass(_vk_device, &render_pass_create_info, NULL, &_onscreen_target.vk_render_pass)) {
            logger.LogFatalError("Failed to create renderpass");
            return false;
        }

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

        GlobeShader *multi_tex_shader = _globe_resource_mgr->LoadShader("position_texture");
        if (nullptr == multi_tex_shader) {
            logger.LogFatalError("Failed to load position_texture shaders");
            return false;
        }
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
        multi_tex_shader->GetPipelineShaderStages(pipeline_shader_stage_create_info);

        VkGraphicsPipelineCreateInfo gfx_pipeline_create_info = {};
        gfx_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gfx_pipeline_create_info.layout = _onscreen_target.vk_pipeline_layout;
        gfx_pipeline_create_info.pVertexInputState = &pipline_vert_input_state_create_info;
        gfx_pipeline_create_info.pInputAssemblyState = &pipline_input_assembly_state_create_info;
        gfx_pipeline_create_info.pRasterizationState = &pipeline_raster_state_create_info;
        gfx_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        gfx_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        gfx_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        gfx_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        gfx_pipeline_create_info.stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_info.size());
        gfx_pipeline_create_info.pStages = pipeline_shader_stage_create_info.data();
        gfx_pipeline_create_info.renderPass = _onscreen_target.vk_render_pass;
        gfx_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(_vk_device, VK_NULL_HANDLE, 1, &gfx_pipeline_create_info, nullptr,
                                                    &_onscreen_target.vk_pipeline)) {
            logger.LogFatalError("Failed to create graphics pipeline");
            return false;
        }

        _globe_resource_mgr->FreeShader(multi_tex_shader);
        // Create the vertex buffer
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_create_info.size = sizeof(g_screen_textured_quad_data);
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.flags = 0;
        if (VK_SUCCESS !=
            vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_onscreen_target.vertex_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create vertex buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _onscreen_target.vertex_buffer.vk_buffer,
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _onscreen_target.vertex_buffer.vk_memory, _onscreen_target.vertex_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate vertex buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkMapMemory(_vk_device, _onscreen_target.vertex_buffer.vk_memory, 0,
                                      _onscreen_target.vertex_buffer.vk_size, 0, (void **)&mapped_data)) {
            logger.LogFatalError("Failed to map vertex buffer memory");
            return false;
        }
        memcpy(mapped_data, g_screen_textured_quad_data, sizeof(g_screen_textured_quad_data));
        vkUnmapMemory(_vk_device, _onscreen_target.vertex_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _onscreen_target.vertex_buffer.vk_buffer,
                                             _onscreen_target.vertex_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind vertex buffer memory");
            return false;
        }

        // Create the index buffer
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_create_info.size = sizeof(g_screen_textured_quad_index_data);
        if (VK_SUCCESS !=
            vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_onscreen_target.index_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create index buffer");
            return false;
        }
        _onscreen_target.num_vertices = sizeof(g_screen_textured_quad_index_data) / sizeof(uint32_t);
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _onscreen_target.index_buffer.vk_buffer,
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _onscreen_target.index_buffer.vk_memory, _onscreen_target.index_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate index buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkMapMemory(_vk_device, _onscreen_target.index_buffer.vk_memory, 0,
                                      _onscreen_target.index_buffer.vk_size, 0, (void **)&mapped_data)) {
            logger.LogFatalError("Failed to map index buffer memory");
            return false;
        }
        memcpy(mapped_data, g_screen_textured_quad_index_data, sizeof(g_screen_textured_quad_index_data));
        vkUnmapMemory(_vk_device, _onscreen_target.index_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _onscreen_target.index_buffer.vk_buffer,
                                             _onscreen_target.index_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind index buffer memory");
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
        if (VK_SUCCESS != vkCreateDescriptorPool(_vk_device, &descriptor_pool_create_info, nullptr,
                                                 &_onscreen_target.vk_descriptor_pool)) {
            logger.LogFatalError("Failed to create descriptor pool");
            return false;
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = _onscreen_target.vk_descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &_onscreen_target.vk_descriptor_set_layout;
        if (VK_SUCCESS !=
            vkAllocateDescriptorSets(_vk_device, &descriptor_set_allocate_info, &_onscreen_target.vk_descriptor_set)) {
            logger.LogFatalError("Failed to allocate descriptor set");
            return false;
        }

        std::vector<VkDescriptorImageInfo> descriptor_image_infos;
        VkDescriptorImageInfo image_info = {};
        image_info.sampler = _offscreen_color->GetVkSampler();
        image_info.imageView = _offscreen_color->GetVkImageView();
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptor_image_infos.push_back(image_info);

        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        VkWriteDescriptorSet write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.dstSet = _onscreen_target.vk_descriptor_set;
        write_set.dstBinding = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.descriptorCount = static_cast<uint32_t>(descriptor_image_infos.size());
        write_set.pImageInfo = descriptor_image_infos.data();
        write_descriptor_sets.push_back(write_set);

        vkUpdateDescriptorSets(_vk_device, static_cast<uint32_t>(write_descriptor_sets.size()),
                               write_descriptor_sets.data(), 0, nullptr);
    }

    if (!GlobeApp::PostSetup(vk_setup_command_pool, vk_setup_command_buffer)) {
        return false;
    }
    _globe_submit_mgr->AttachRenderPassAndDepthBuffer(_onscreen_target.vk_render_pass, _depth_buffer.vk_image_view);
    _current_buffer = 0;

    return true;
}

bool OffscreenRenderingApp::Update(float diff_ms) {
    static float cur_time_diff = 0;
    cur_time_diff += diff_ms;
    if (cur_time_diff > 30.f) {
        DetermineNewColor();
        cur_time_diff = 0.f;
    }
    return true;
}

bool OffscreenRenderingApp::Draw() {
    GlobeLogger &logger = GlobeLogger::getInstance();

    _last_buffer = _current_buffer;
    VkCommandBuffer vk_render_command_buffer;
    VkFramebuffer vk_framebuffer;
    _globe_submit_mgr->AcquireNextImageIndex(_current_buffer);
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
    clear_values[0].color.float32[0] = 0.3f;
    clear_values[0].color.float32[1] = 0.3f;
    clear_values[0].color.float32[2] = 0.3f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1] = {};
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _offscreen_target.vk_render_pass;
    render_pass_begin_info.framebuffer = _offscreen_target.vk_framebuffer;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent.width = _offscreen_target.width;
    render_pass_begin_info.renderArea.extent.height = _offscreen_target.height;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;
    if (VK_SUCCESS !=
        vkBeginCommandBuffer(_offscreen_target._vk_command_buffers[_current_buffer], &command_buffer_begin_info)) {
        logger.LogFatalError("Failed to begin command buffer for offscreen draw commands for framebuffer");
    }

    vkCmdBeginRenderPass(_offscreen_target._vk_command_buffers[_current_buffer], &render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = (_offscreen_target.vk_uniform_vec4_alignment * _current_buffer);
    uint8_t* uniform_map = _offscreen_target.uniform_mapped_data + offset;
    memcpy(uniform_map, &_color_0, sizeof(glm::vec4));
    uniform_map += sizeof(glm::vec4);
    memcpy(uniform_map, &_color_1, sizeof(glm::vec4));
    uniform_map += sizeof(glm::vec4);
    memcpy(uniform_map, &_selected_index, sizeof(int32_t));
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = _offscreen_target.uniform_buffer.vk_memory;
    memoryRange.size = _offscreen_target.vk_uniform_vec4_alignment;
    memoryRange.offset = offset;
    vkFlushMappedMemoryRanges(_vk_device, 1, &memoryRange);

    // Update dynamic viewport state
    VkViewport viewport = {};
    viewport.height = (float)_offscreen_target.height;
    viewport.width = (float)_offscreen_target.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(_offscreen_target._vk_command_buffers[_current_buffer], 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = _offscreen_target.width;
    scissor.extent.height = _offscreen_target.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(_offscreen_target._vk_command_buffers[_current_buffer], 0, 1, &scissor);

    uint32_t dynamic_offset = _current_buffer * static_cast<uint32_t>(_offscreen_target.vk_uniform_vec4_alignment);
    vkCmdBindDescriptorSets(_offscreen_target._vk_command_buffers[_current_buffer], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _offscreen_target.vk_pipeline_layout, 0, 1, &_offscreen_target.vk_descriptor_set, 1,
                            &dynamic_offset);
    vkCmdBindPipeline(_offscreen_target._vk_command_buffers[_current_buffer], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      _offscreen_target.vk_pipeline);

    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdBindVertexBuffers(_offscreen_target._vk_command_buffers[_current_buffer], 0, 1,
                           &_offscreen_target.vertex_buffer.vk_buffer, &vert_buffer_offset);
    vkCmdBindIndexBuffer(_offscreen_target._vk_command_buffers[_current_buffer],
                         _offscreen_target.index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(_offscreen_target._vk_command_buffers[_current_buffer], _offscreen_target.num_vertices, 1, 0, 0,
                     1);

    vkCmdEndRenderPass(_offscreen_target._vk_command_buffers[_current_buffer]);
    if (VK_SUCCESS != vkEndCommandBuffer(_offscreen_target._vk_command_buffers[_current_buffer])) {
        logger.LogFatalError("Failed to end command buffer");
        return false;
    }

    _globe_submit_mgr->Submit(_offscreen_target._vk_command_buffers[_current_buffer], VK_NULL_HANDLE,
                              _offscreen_target.vk_semaphore, VK_NULL_HANDLE, false);

    command_buffer_begin_info = {};
    render_pass_begin_info = {};
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
    render_pass_begin_info.renderPass = _onscreen_target.vk_render_pass;
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
    viewport = {};
    viewport.height = (float)_height;
    viewport.width = (float)_width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(vk_render_command_buffer, 0, 1, &viewport);

    // Update dynamic scissor state
    scissor = {};
    scissor.extent.width = _width;
    scissor.extent.height = _height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(vk_render_command_buffer, 0, 1, &scissor);

    dynamic_offset = static_cast<uint32_t>(offset);
    vkCmdBindDescriptorSets(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _onscreen_target.vk_pipeline_layout, 0, 1, &_onscreen_target.vk_descriptor_set, 1,
                            &dynamic_offset);
    vkCmdBindPipeline(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _onscreen_target.vk_pipeline);

    vkCmdBindVertexBuffers(vk_render_command_buffer, 0, 1, &_onscreen_target.vertex_buffer.vk_buffer,
                           &vert_buffer_offset);
    vkCmdBindIndexBuffer(vk_render_command_buffer, _onscreen_target.index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vk_render_command_buffer, _onscreen_target.num_vertices, 1, 0, 0, 1);
    vkCmdEndRenderPass(vk_render_command_buffer);
    if (VK_SUCCESS != vkEndCommandBuffer(vk_render_command_buffer)) {
        logger.LogFatalError("Failed to end command buffer");
        return false;
    }

    _globe_submit_mgr->InsertPresentCommandsToBuffer(vk_render_command_buffer);

    _globe_submit_mgr->SubmitAndPresent(_offscreen_target.vk_semaphore);

    return GlobeApp::Draw();
}

static OffscreenRenderingApp *g_app = nullptr;

GLOBE_APP_MAIN() {
    GlobeInitStruct init_struct = {};
    GLOBE_APP_MAIN_BEGIN(init_struct)
    init_struct.app_name = "Globe App - Offscreen Rendering";
    init_struct.version.major = 0;
    init_struct.version.minor = 1;
    init_struct.version.patch = 0;
    init_struct.width = 500;
    init_struct.height = 500;
    init_struct.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    init_struct.num_swapchain_buffers = 3;
    init_struct.ideal_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    init_struct.secondary_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    g_app = new OffscreenRenderingApp();
    g_app->Init(init_struct);
    g_app->Run();
    g_app->Exit();

    GLOBE_APP_MAIN_END(0)
}
