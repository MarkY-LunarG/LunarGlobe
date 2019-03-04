//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    samples/06_offscreen_rendering.cpp
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
#include "globe/globe_camera.hpp"
#include "globe/globe_event.hpp"
#include "globe/globe_window.hpp"
#include "globe/globe_submit_manager.hpp"
#include "globe/globe_shader.hpp"
#include "globe/globe_texture.hpp"
#include "globe/globe_resource_manager.hpp"
#include "globe/globe_app.hpp"
#include "globe/globe_main.hpp"

struct VulkanTarget {
    uint32_t width;
    uint32_t height;
    VkFramebuffer vk_framebuffer;
    VkRenderPass vk_render_pass;
    VkSemaphore vk_semaphore;
    VkDescriptorSetLayout vk_descriptor_set_layout;
    VkPipelineLayout vk_pipeline_layout;
    VkDescriptorPool vk_descriptor_pool;
    VkDescriptorSet vk_descriptor_set;
    VkPipeline vk_pipeline;
    GlobeVulkanBuffer vertex_buffer;
    GlobeVulkanBuffer index_buffer;
    GlobeVulkanBuffer uniform_buffer;
    uint8_t *uniform_map;
    VkCommandPool vk_command_pool;
    std::vector<VkCommandBuffer> vk_command_buffers;
};

class OffscreenRenderingApp : public GlobeApp {
   public:
    OffscreenRenderingApp();
    ~OffscreenRenderingApp();

    virtual void CleanupCommandObjects(bool is_resize) override;

   protected:
    bool CreateOffscreenTarget(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                               VkFormat vk_color_format, VkFormat vk_depth_stencil_format);
    virtual bool Setup() override;
    virtual bool Update(float diff_ms) override;
    virtual bool Draw() override;
    void UpdateEllipseCenter();

   private:
    void CalculateOffscreenModelMatrices(void);
    void CleanupVulkanTarget(VulkanTarget &target);
    void DetermineNewColor();

    VulkanTarget _onscreen_target;
    VulkanTarget _offscreen_target;
    GlobeTexture *_offscreen_color;
    GlobeTexture *_offscreen_depth;
    uint8_t *_offscreen_constants;
    uint32_t _vk_uniform_frame_size;
    uint32_t _vk_min_uniform_alignment;
    GlobeCamera _offscreen_camera;
    float _offscreen_camera_distance;
    float _offscreen_camera_step;
    float _offscreen_pyramid_orbit_rotation;
    float _offscreen_pyramid_orientation_rotation;
    glm::mat4 _offscreen_pyramid_mat;
    float _offscreen_diamond_orbit_rotation;
    float _offscreen_diamond_orientation_rotation;
    glm::mat4 _offscreen_diamond_mat;
    GlobeCamera _onscreen_camera;
    glm::mat4 _onscreen_cube_mat;
    float _onscreen_cube_orientation_rotation;
};

// On screen textured cube that will have the offscreen
// surface rendered onto while it spins.
static const float g_onscreen_cube_data[] = {
    -0.5f, 0.5f,  -0.5f, 1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Front  Vert 0
    0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    0.5f,  -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
    0.5f,  0.5f,  -0.5f, 1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Right  Vert 0
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    0.5f,  -0.5f, 0.5f,  1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    0.5f,  -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
    -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Left   Vert 0
    -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    -0.5f, -0.5f, 0.5f,  1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Back   Vert 0
    -0.5f, 0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    -0.5f, -0.5f, 0.5f,  1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Top    Vert 0
    0.5f,  -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    0.5f,  -0.5f, 0.5f,  1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    -0.5f, -0.5f, 0.5f,  1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
    -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.f, 1.0f,  // Bottom Vert 0
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 0.f, 1.0f,  //        Vert 1
    0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 1.0f, 0.f, 1.0f,  //        Vert 2
    -0.5f, 0.5f,  -0.5f, 1.0f, 0.0f, 1.0f, 0.f, 1.0f,  //        Vert 3
};
static const uint32_t g_onscreen_cube_index_data[] = {0,  2,  1,  2,  0,  3,  4,  6,  5,  6,  4,  7,
                                                      8,  10, 9,  8,  11, 10, 12, 14, 13, 12, 15, 14,
                                                      16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22};

static const float g_offscreen_vertex_data[] = {
    // clang-format off
     // Vertex x, y, z          Color r, g, b
    // Diamond 6 verts, Defined 24 times
     0.0f, -0.5f,  0.0f,        1.0f, 0.0f, 0.0f,
    -0.5f,  0.0f, -0.5f,        1.0f, 0.5f, 0.0f,
     0.5f,  0.0f, -0.5f,        1.0f, 1.0f, 0.0f,
     0.5f,  0.0f,  0.5f,        0.5f, 0.5f, 0.0f,
    -0.5f,  0.0f,  0.5f,        0.5f, 1.0f, 0.0f,
     0.0f,  0.5f,  0.0f,        0.0f, 1.0f, 0.0f,
     // Pyramid 5 verts, Defined 18 times
     0.0f, -0.5f,  0.0f,        0.0f, 0.3f, 1.0f,
    -0.5f,  0.5f, -0.5f,        0.0f, 0.6f, 1.0f,
     0.5f,  0.5f, -0.5f,        0.0f, 0.9f, 1.0f,
     0.5f,  0.5f,  0.5f,        0.0f, 0.6f, 1.0f,
    -0.5f,  0.5f,  0.5f,        0.0f, 0.3f, 1.0f,
    // clang-format on
};
static const uint32_t g_offscreen_index_data[] = {0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1,  4, 5, 1, 2,  5,  2, 3, 5, 3, 4,
                                                  5, 4, 1, 6, 8, 7, 6, 9, 8, 6, 10, 9, 6, 7, 10, 10, 7, 9, 9, 7, 8};

OffscreenRenderingApp::OffscreenRenderingApp() {
    _onscreen_target = {};
    _offscreen_target = {};
    _offscreen_color = nullptr;
    _offscreen_depth = nullptr;
    _offscreen_diamond_orbit_rotation = 0.f;
    _offscreen_diamond_orientation_rotation = 0.f;
    _offscreen_pyramid_orbit_rotation = 0.f;
    _offscreen_pyramid_orientation_rotation = 0.f;

    _offscreen_camera_distance = 3.f;
    _offscreen_camera_step = 0.05f;
    _offscreen_camera.SetPerspectiveProjection(1.0f, 45.f, 1.0f, 100.f);
    _offscreen_camera.SetCameraPosition(0.f, 0.f, -_offscreen_camera_distance);
    _offscreen_diamond_orbit_rotation = 90.f;
    _offscreen_diamond_orientation_rotation = 0.f;
    _offscreen_pyramid_orbit_rotation = 0.f;
    _offscreen_pyramid_orientation_rotation = 0.f;
    _onscreen_camera.SetPerspectiveProjection(1.0f, 45.f, 1.0f, 100.f);
    _onscreen_camera.SetCameraPosition(1.3f, -0.3f, -2.f);
    _onscreen_camera.SetCameraOrientation(33.f, 5.f, 10.f);
    _onscreen_cube_orientation_rotation = 0.f;
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
    if (VK_NULL_HANDLE != target.uniform_buffer.vk_memory) {
        vkUnmapMemory(_vk_device, target.uniform_buffer.vk_memory);
        _globe_resource_mgr->FreeDeviceMemory(target.uniform_buffer.vk_memory);
    }
    if (VK_NULL_HANDLE != target.index_buffer.vk_memory) {
        _globe_resource_mgr->FreeDeviceMemory(target.index_buffer.vk_memory);
        target.index_buffer.vk_memory = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.vertex_buffer.vk_memory) {
        _globe_resource_mgr->FreeDeviceMemory(target.vertex_buffer.vk_memory);
        target.vertex_buffer.vk_memory = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != target.uniform_buffer.vk_buffer) {
        vkDestroyBuffer(_vk_device, target.uniform_buffer.vk_buffer, nullptr);
        target.uniform_buffer.vk_buffer = VK_NULL_HANDLE;
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
    if (VK_NULL_HANDLE != target.vk_descriptor_pool) {
        vkDestroyDescriptorPool(_vk_device, target.vk_descriptor_pool, nullptr);
        target.vk_descriptor_pool = VK_NULL_HANDLE;
    }
    if (target.vk_command_buffers.size() > 0) {
        vkFreeCommandBuffers(_vk_device, target.vk_command_pool,
                             static_cast<uint32_t>(target.vk_command_buffers.size()), target.vk_command_buffers.data());
        target.vk_command_buffers.clear();
    }
    vkDestroyCommandPool(_vk_device, target.vk_command_pool, nullptr);
    target.vk_command_pool = VK_NULL_HANDLE;
}

void OffscreenRenderingApp::CleanupCommandObjects(bool is_resize) {
    if (!_is_minimized) {
        CleanupVulkanTarget(_offscreen_target);
        CleanupVulkanTarget(_onscreen_target);
    }
    GlobeApp::CleanupCommandObjects(is_resize);
}

void OffscreenRenderingApp::CalculateOffscreenModelMatrices(void) {
    // Update the Diamond and Pyramid rotation
    glm::mat4 identity_mat = glm::mat4(1);
    glm::vec3 x_orbit_vec(1.f, 0.f, 0.f);
    glm::vec3 y_orbit_vec(0.f, 1.f, 0.f);
    _offscreen_pyramid_mat =
        glm::rotate(identity_mat, glm::radians(_offscreen_diamond_orientation_rotation), x_orbit_vec);
    _offscreen_pyramid_mat = glm::translate(_offscreen_pyramid_mat, y_orbit_vec);
    _offscreen_pyramid_mat =
        glm::rotate(_offscreen_pyramid_mat, glm::radians(_offscreen_diamond_orbit_rotation), x_orbit_vec);
    _offscreen_diamond_mat =
        glm::rotate(identity_mat, glm::radians(_offscreen_diamond_orientation_rotation), y_orbit_vec);
    _offscreen_diamond_mat = glm::translate(_offscreen_diamond_mat, x_orbit_vec);
    _offscreen_diamond_mat =
        glm::rotate(_offscreen_diamond_mat, glm::radians(_offscreen_diamond_orbit_rotation), y_orbit_vec);
}

bool OffscreenRenderingApp::CreateOffscreenTarget(VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                                  VkFormat vk_color_format, VkFormat vk_depth_stencil_format) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    uint8_t *mapped_data;

    _offscreen_target.width = width;
    _offscreen_target.height = height;

    // Setup the model matrices
    CalculateOffscreenModelMatrices();

    // Create an offscreen color and depth render target
    _offscreen_color = _globe_resource_mgr->CreateRenderTargetTexture(_offscreen_target.width, _offscreen_target.height,
                                                                      vk_color_format);
    if (nullptr == _offscreen_color) {
        logger.LogError("Failed creating color render target texture");
        return false;
    }

    if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
        _offscreen_depth = _globe_resource_mgr->CreateRenderTargetTexture(
            _offscreen_target.width, _offscreen_target.height, vk_depth_stencil_format);
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

    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
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
    buffer_create_info.size = sizeof(g_offscreen_vertex_data);
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
            _offscreen_target.vertex_buffer.vk_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            _offscreen_target.vertex_buffer.vk_memory, _offscreen_target.vertex_buffer.vk_size)) {
        logger.LogFatalError("Failed to allocate offscreen vertex buffer memory");
        return false;
    }
    if (VK_SUCCESS != vkMapMemory(_vk_device, _offscreen_target.vertex_buffer.vk_memory, 0,
                                  _offscreen_target.vertex_buffer.vk_size, 0, (void **)&mapped_data)) {
        logger.LogFatalError("Failed to map offscreen vertex buffer memory");
        return false;
    }
    memcpy(mapped_data, g_offscreen_vertex_data, sizeof(g_offscreen_vertex_data));
    vkUnmapMemory(_vk_device, _offscreen_target.vertex_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _offscreen_target.vertex_buffer.vk_buffer,
                                         _offscreen_target.vertex_buffer.vk_memory, 0)) {
        logger.LogFatalError("Failed to bind offscreen vertex buffer memory");
        return false;
    }

    // Create the offscreen index buffer
    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.size = sizeof(g_offscreen_index_data);
    if (VK_SUCCESS !=
        vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_offscreen_target.index_buffer.vk_buffer)) {
        logger.LogFatalError("Failed to create offscreen index buffer");
        return false;
    }
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
    memcpy(mapped_data, g_offscreen_index_data, sizeof(g_offscreen_index_data));
    vkUnmapMemory(_vk_device, _offscreen_target.index_buffer.vk_memory);
    if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _offscreen_target.index_buffer.vk_buffer,
                                         _offscreen_target.index_buffer.vk_memory, 0)) {
        logger.LogFatalError("Failed to bind offscreen index buffer memory");
        return false;
    }

    // Create the uniform buffer that will store all the appropriate matrices.  I do swapchain count * 2
    // so that the offscreen render can use it's own information and then the on-screen render can use
    // separate info so that the shaders are simpler.
    buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.size = _vk_uniform_frame_size * _swapchain_count;
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
                                  (void **)&_offscreen_target.uniform_map)) {
        logger.LogFatalError("Failed to map offscreen uniform buffer memory");
        return false;
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
    descriptor_buffer_info.range = _vk_uniform_frame_size;

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = _offscreen_target.vk_descriptor_set;
    write_set.dstBinding = 0;
    write_set.dstArrayElement = 0;
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
    vertex_input_binding_description.stride = sizeof(float) * 6;
    VkVertexInputAttributeDescription vertex_input_attribute_description[2];
    vertex_input_attribute_description[0] = {};
    vertex_input_attribute_description[0].binding = 0;
    vertex_input_attribute_description[0].location = 0;
    vertex_input_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description[0].offset = 0;
    vertex_input_attribute_description[1] = {};
    vertex_input_attribute_description[1].binding = 0;
    vertex_input_attribute_description[1].location = 1;
    vertex_input_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description[1].offset = sizeof(float) * 3;
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

    GlobeShader *position_color_shader = _globe_resource_mgr->LoadShader("position_mvp_color");
    if (nullptr == position_color_shader) {
        logger.LogFatalError("Failed to load position_mvp_color shaders");
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
        vkCreateCommandPool(_vk_device, &cmd_pool_create_info, NULL, &_offscreen_target.vk_command_pool)) {
        logger.LogFatalError("Failed to create offscreen command pool");
        return false;
    }

    // Create a custom command buffer per swapchain image for the offscreen surface to
    // perform its command prior to be submitted, synced up, and then used
    uint32_t num_swapchain_images = _globe_submit_mgr->NumSwapchainImages();
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = _offscreen_target.vk_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = num_swapchain_images;
    _offscreen_target.vk_command_buffers.resize(num_swapchain_images);
    if (VK_SUCCESS !=
        vkAllocateCommandBuffers(_vk_device, &command_buffer_allocate_info, &_offscreen_target.vk_command_buffers[0])) {
        std::string error_msg = "Failed to allocate ";
        error_msg += std::to_string(num_swapchain_images);
        error_msg += " offscreen render command buffers";
        logger.LogFatalError(error_msg);
        return false;
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

    _vk_min_uniform_alignment =
        static_cast<uint32_t>(_vk_phys_device_properties.limits.minUniformBufferOffsetAlignment);
    if (_vk_min_uniform_alignment < static_cast<uint32_t>(_vk_phys_device_properties.limits.nonCoherentAtomSize)) {
        _vk_min_uniform_alignment = static_cast<uint32_t>(_vk_phys_device_properties.limits.nonCoherentAtomSize);
    }
    _vk_uniform_frame_size = sizeof(glm::mat4) * 2;
    _vk_uniform_frame_size += (_vk_min_uniform_alignment - 1);
    _vk_uniform_frame_size &= ~(_vk_min_uniform_alignment - 1);

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
        cur_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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

        std::vector<VkPushConstantRange> push_constant_ranges;
        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(glm::mat4);
        push_constant_ranges.push_back(push_constant_range);

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
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
        if (VK_SUCCESS != vkCreateRenderPass(_vk_device, &render_pass_create_info, NULL, &_vk_render_pass)) {
            logger.LogFatalError("Failed to create renderpass");
            return false;
        }
        _onscreen_target.vk_render_pass = _vk_render_pass;

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

        GlobeShader *multi_tex_shader = _globe_resource_mgr->LoadShader("position_mvp_texture");
        if (nullptr == multi_tex_shader) {
            logger.LogFatalError("Failed to load position_mvp_texture shaders");
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
        buffer_create_info.size = sizeof(g_onscreen_cube_data);
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
        memcpy(mapped_data, g_onscreen_cube_data, sizeof(g_onscreen_cube_data));
        vkUnmapMemory(_vk_device, _onscreen_target.vertex_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _onscreen_target.vertex_buffer.vk_buffer,
                                             _onscreen_target.vertex_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind vertex buffer memory");
            return false;
        }

        // Create the index buffer
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_create_info.size = sizeof(g_onscreen_cube_index_data);
        if (VK_SUCCESS !=
            vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_onscreen_target.index_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create index buffer");
            return false;
        }
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
        memcpy(mapped_data, g_onscreen_cube_index_data, sizeof(g_onscreen_cube_index_data));
        vkUnmapMemory(_vk_device, _onscreen_target.index_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _onscreen_target.index_buffer.vk_buffer,
                                             _onscreen_target.index_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind index buffer memory");
            return false;
        }

        // Create the uniform buffer that will store all the appropriate matrices.  I do swapchain count * 2
        // so that the offscreen render can use it's own information and then the on-screen render can use
        // separate info so that the shaders are simpler.
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.size = _vk_uniform_frame_size * _swapchain_count;
        if (VK_SUCCESS !=
            vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &_onscreen_target.uniform_buffer.vk_buffer)) {
            logger.LogFatalError("Failed to create onscreen uniform buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                _onscreen_target.uniform_buffer.vk_buffer,
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                _onscreen_target.uniform_buffer.vk_memory, _onscreen_target.uniform_buffer.vk_size)) {
            logger.LogFatalError("Failed to allocate onscreen uniform buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkMapMemory(_vk_device, _onscreen_target.uniform_buffer.vk_memory, 0,
                                      _onscreen_target.uniform_buffer.vk_size, 0,
                                      (void **)&_onscreen_target.uniform_map)) {
            logger.LogFatalError("Failed to map onscreen uniform buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, _onscreen_target.uniform_buffer.vk_buffer,
                                             _onscreen_target.uniform_buffer.vk_memory, 0)) {
            logger.LogFatalError("Failed to bind onscreen uniform buffer memory");
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

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _onscreen_target.uniform_buffer.vk_buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = _vk_uniform_frame_size;

        std::vector<VkDescriptorImageInfo> descriptor_image_infos;
        VkDescriptorImageInfo image_info = {};
        image_info.sampler = _offscreen_color->GetVkSampler();
        image_info.imageView = _offscreen_color->GetVkImageView();
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor_image_infos.push_back(image_info);

        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        VkWriteDescriptorSet write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.dstSet = _onscreen_target.vk_descriptor_set;
        write_set.dstBinding = 0;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_set.descriptorCount = 1;
        write_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_sets.push_back(write_set);
        write_set = {};
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

#define INCREMENT_ROTATION_VALUE(var, inc) \
    var += inc;                            \
    if (var > 360.f) var -= 360.f;         \
    if (var < 0.f) var += 360.f

bool OffscreenRenderingApp::Update(float diff_ms) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    _globe_submit_mgr->AcquireNextImageIndex(_current_buffer);

    static float cur_time_diff = 0;
    cur_time_diff += diff_ms;
    if (cur_time_diff > 9.f) {
        _offscreen_camera_distance += _offscreen_camera_step;
        if ((_offscreen_camera_step > 0.f && _offscreen_camera_distance > 12.f) ||
            (_offscreen_camera_step < 0.f && _offscreen_camera_distance < 3.f)) {
            _offscreen_camera_step = -_offscreen_camera_step;
        }
        _offscreen_camera.SetCameraPosition(0.f, 0.f, -_offscreen_camera_distance);

        INCREMENT_ROTATION_VALUE(_offscreen_pyramid_orbit_rotation, 0.3f);
        INCREMENT_ROTATION_VALUE(_offscreen_pyramid_orientation_rotation, 0.9f);
        INCREMENT_ROTATION_VALUE(_offscreen_diamond_orbit_rotation, -0.3f);
        INCREMENT_ROTATION_VALUE(_offscreen_diamond_orientation_rotation, -0.9f);
        INCREMENT_ROTATION_VALUE(_onscreen_cube_orientation_rotation, 0.2f);
        CalculateOffscreenModelMatrices();
        _onscreen_cube_mat =
            glm::rotate(glm::mat4(1), glm::radians(_onscreen_cube_orientation_rotation), glm::vec3(1.f, 0.f, 0.f));
        cur_time_diff = 0.f;
    }

    // Copy the latest matrices into the uniform buffer object
    VkDeviceSize offset = _vk_uniform_frame_size * _current_buffer;

    // First offscreen
    uint8_t *uniform_map = _offscreen_target.uniform_map + offset;
    memcpy(uniform_map, _offscreen_camera.ProjectionMatrix(), sizeof(glm::mat4));
    uniform_map += sizeof(glm::mat4);
    glm::mat4 view_mat = _offscreen_camera.ViewMatrix();
    memcpy(uniform_map, &view_mat, sizeof(glm::mat4));
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _offscreen_target.uniform_buffer.vk_memory;
    memory_range.size = _vk_uniform_frame_size;
    memory_range.offset = offset;
    vkFlushMappedMemoryRanges(_vk_device, 1, &memory_range);

    // Now onscreen
    uniform_map = _onscreen_target.uniform_map + offset;
    memcpy(uniform_map, _onscreen_camera.ProjectionMatrix(), sizeof(glm::mat4));
    uniform_map += sizeof(glm::mat4);
    view_mat = _onscreen_camera.ViewMatrix();
    memcpy(uniform_map, &view_mat, sizeof(glm::mat4));
    memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _onscreen_target.uniform_buffer.vk_memory;
    memory_range.size = _vk_uniform_frame_size;
    memory_range.offset = offset;
    vkFlushMappedMemoryRanges(_vk_device, 1, &memory_range);

    if (!UpdateOverlay(_current_buffer)) {
        logger.LogFatalError("Failed to update overlay");
    }
    return true;
}

bool OffscreenRenderingApp::Draw() {
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
    clear_values[0].color.float32[0] = 0.f;
    clear_values[0].color.float32[1] = 0.f;
    clear_values[0].color.float32[2] = 0.1f;
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
        vkBeginCommandBuffer(_offscreen_target.vk_command_buffers[_current_buffer], &command_buffer_begin_info)) {
        logger.LogFatalError("Failed to begin command buffer for offscreen draw commands for framebuffer");
    }

    vkCmdBeginRenderPass(_offscreen_target.vk_command_buffers[_current_buffer], &render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    // Update dynamic viewport state
    VkViewport viewport = {};
    viewport.height = (float)_offscreen_target.height;
    viewport.width = (float)_offscreen_target.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(_offscreen_target.vk_command_buffers[_current_buffer], 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = _offscreen_target.width;
    scissor.extent.height = _offscreen_target.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(_offscreen_target.vk_command_buffers[_current_buffer], 0, 1, &scissor);

    uint32_t dynamic_offset = _current_buffer * _vk_uniform_frame_size;
    vkCmdBindDescriptorSets(_offscreen_target.vk_command_buffers[_current_buffer], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _offscreen_target.vk_pipeline_layout, 0, 1, &_offscreen_target.vk_descriptor_set, 1,
                            &dynamic_offset);
    vkCmdBindPipeline(_offscreen_target.vk_command_buffers[_current_buffer], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      _offscreen_target.vk_pipeline);

    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdBindVertexBuffers(_offscreen_target.vk_command_buffers[_current_buffer], 0, 1,
                           &_offscreen_target.vertex_buffer.vk_buffer, &vert_buffer_offset);
    vkCmdBindIndexBuffer(_offscreen_target.vk_command_buffers[_current_buffer],
                         _offscreen_target.index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(_offscreen_target.vk_command_buffers[_current_buffer], _offscreen_target.vk_pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &_offscreen_pyramid_mat);

    vkCmdDrawIndexed(_offscreen_target.vk_command_buffers[_current_buffer], 24, 1, 0, 0, 1);
    vkCmdPushConstants(_offscreen_target.vk_command_buffers[_current_buffer], _offscreen_target.vk_pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &_offscreen_diamond_mat);
    vkCmdDrawIndexed(_offscreen_target.vk_command_buffers[_current_buffer], 18, 1, 24, 0, 1);

    vkCmdEndRenderPass(_offscreen_target.vk_command_buffers[_current_buffer]);
    if (VK_SUCCESS != vkEndCommandBuffer(_offscreen_target.vk_command_buffers[_current_buffer])) {
        logger.LogFatalError("Failed to end command buffer");
        return false;
    }

    VkFence offscreen_fence;
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = 0;
    if (VK_SUCCESS != vkCreateFence(_vk_device, &fence_create_info, nullptr, &offscreen_fence)) {
        logger.LogFatalError("Failed to allocate the off-screen fence sync fence");
        return false;
    }

    _globe_submit_mgr->Submit(_offscreen_target.vk_command_buffers[_current_buffer], VK_NULL_HANDLE,
                              _offscreen_target.vk_semaphore, offscreen_fence, true);

    vkDestroyFence(_vk_device, offscreen_fence, nullptr);

    command_buffer_begin_info = {};
    render_pass_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    clear_values[0] = {};
    clear_values[0].color.float32[0] = 0.3f;
    clear_values[0].color.float32[1] = 0.3f;
    clear_values[0].color.float32[2] = 0.3f;
    clear_values[0].color.float32[3] = 0.3f;
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

    dynamic_offset = _current_buffer * _vk_uniform_frame_size;
    vkCmdBindDescriptorSets(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _onscreen_target.vk_pipeline_layout, 0, 1, &_onscreen_target.vk_descriptor_set, 1,
                            &dynamic_offset);
    vkCmdBindPipeline(vk_render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _onscreen_target.vk_pipeline);

    vkCmdBindVertexBuffers(vk_render_command_buffer, 0, 1, &_onscreen_target.vertex_buffer.vk_buffer,
                           &vert_buffer_offset);
    vkCmdBindIndexBuffer(vk_render_command_buffer, _onscreen_target.index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(vk_render_command_buffer, _onscreen_target.vk_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                       &_onscreen_cube_mat);
    vkCmdDrawIndexed(vk_render_command_buffer, sizeof(g_onscreen_cube_index_data) / sizeof(uint32_t), 1, 0, 0, 1);

    DrawOverlay(vk_render_command_buffer, _current_buffer);

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
