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

#include "gravity_logger.hpp"
#include "gravity_event.hpp"
#include "gravity_window.hpp"
#include "gravity_submit_manager.hpp"

#include "linmath.h"
#include "object_type_string_helper.h"

#include "inttypes.h"

#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "gravity"
#define APP_LONG_NAME "The Vulkan Gravity Demo Program"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

static char bitmap_file[] = "lunarg.ppm";
static char *tex_files[] = {bitmap_file};

static int validation_error = 0;

struct vktexgravity_vs_uniform {
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

void dumpMatrix(const char *note, mat4x4 MVP) {
    int i;

    printf("%s: \n", note);
    for (i = 0; i < 4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, vec4 vector) {
    printf("%s: \n", note);
    printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

struct Demo {
    GravityWindow *gravity_window;
    GravitySubmitManager *gravity_submit_mgr;
    bool prepared;
    bool use_staging_buffer;
    bool was_minimized;
    bool is_minimized;
    bool paused;

    bool VK_GOOGLE_display_timing_enabled;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    const char *extension_names[64];
    const char *enabled_layers[64];

    uint32_t width, height;

    uint32_t swapchainImageCount;
    SwapchainImageResources *swapchain_image_resources;
    VkPresentModeKHR presentMode;

    VkCommandPool cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];
    struct texture_object staging_texture;

    VkCommandBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;

    bool quit;
    int32_t curFrame;
    int32_t frameCount;
    bool validate;
    bool use_break;
    bool suppress_popups;

    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;

    uint32_t current_buffer;
    uint32_t queue_family_count;
};

// Forward declaration:
static void demo_resize(struct Demo *demo);

static bool memory_type_from_properties(struct Demo *demo, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((demo->memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

static void demo_flush_init_cmd(struct Demo *demo) {
    VkResult U_ASSERT_ONLY err;

    // This function could get called twice if the texture uses a staging buffer
    // In that case the second call should be ignored
    if (demo->cmd == VK_NULL_HANDLE) return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    VkFence fence;
    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = NULL;
    fence_ci.flags = 0;
    err = vkCreateFence(demo->device, &fence_ci, NULL, &fence);
    assert(!err);

    std::vector<VkCommandBuffer> command_buffers;
    command_buffers.push_back(demo->cmd);
    demo->gravity_submit_mgr->Submit(command_buffers, fence, true);
    vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, &demo->cmd);
    vkDestroyFence(demo->device, fence, NULL);
    demo->cmd = VK_NULL_HANDLE;
}

static void demo_set_image_layout(struct Demo *demo, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
                                  VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask, VkPipelineStageFlags src_stages,
                                  VkPipelineStageFlags dest_stages) {
    assert(demo->cmd);

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = srcAccessMask;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange = {aspectMask, 0, 1, 0, 1};

    switch (new_image_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            image_memory_barrier.dstAccessMask = 0;
            break;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    vkCmdPipelineBarrier(demo->cmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);
}

static void demo_draw_build_cmd(struct Demo *demo, VkCommandBuffer cmd_buf) {
    VkDebugUtilsLabelEXT label;
    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkClearValue clear_values[2] = {{}, {}};
    VkRenderPassBeginInfo rp_begin = {};
    memset(&label, 0, sizeof(label));
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
    rp_begin.renderPass = demo->render_pass;
    demo->gravity_submit_mgr->GetFramebuffer(demo->current_buffer, rp_begin.framebuffer);
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = demo->width;
    rp_begin.renderArea.extent.height = demo->height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;
    VkResult U_ASSERT_ONLY err;

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);

    if (demo->validate && nullptr != demo->SetDebugUtilsObjectNameEXT && nullptr != demo->CmdBeginDebugUtilsLabelEXT) {
        // Set a name for the command buffer
        VkDebugUtilsObjectNameInfoEXT cmd_buf_name = {};
        cmd_buf_name.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        cmd_buf_name.pNext = nullptr;
        cmd_buf_name.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        cmd_buf_name.objectHandle = (uint64_t)cmd_buf;
        cmd_buf_name.pObjectName = "GravityDrawCommandBuf";
        demo->SetDebugUtilsObjectNameEXT(demo->device, &cmd_buf_name);

        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "DrawBegin";
        label.color[0] = 0.4f;
        label.color[1] = 0.3f;
        label.color[2] = 0.2f;
        label.color[3] = 0.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    assert(!err);
    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    if (demo->validate && nullptr != demo->CmdBeginDebugUtilsLabelEXT) {
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "InsideRenderPass";
        label.color[0] = 8.4f;
        label.color[1] = 7.3f;
        label.color[2] = 6.2f;
        label.color[3] = 7.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout, 0, 1,
                            &demo->swapchain_image_resources[demo->current_buffer].descriptor_set, 0, NULL);
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)demo->height;
    viewport.width = (float)demo->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    if (demo->validate && nullptr != demo->CmdBeginDebugUtilsLabelEXT) {
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "ActualDraw";
        label.color[0] = -0.4f;
        label.color[1] = -0.3f;
        label.color[2] = -0.2f;
        label.color[3] = -0.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    if (demo->validate && nullptr != demo->CmdEndDebugUtilsLabelEXT) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }

    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmd_buf);
    if (demo->validate && nullptr != demo->CmdEndDebugUtilsLabelEXT) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }

    demo->gravity_submit_mgr->InsertPresentCommandsToBuffer(cmd_buf);

    if (demo->validate && nullptr != demo->CmdEndDebugUtilsLabelEXT) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }
    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
}

void demo_update_data_buffer(struct Demo *demo) {
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;
    VkResult U_ASSERT_ONLY err;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(demo->spin_angle));
    mat4x4_mul(MVP, VP, demo->model_matrix);

    err = vkMapMemory(demo->device, demo->swapchain_image_resources[demo->current_buffer].uniform_memory, 0, VK_WHOLE_SIZE, 0,
                      (void **)&pData);
    assert(!err);

    memcpy(pData, (const void *)&MVP[0][0], matrixSize);

    vkUnmapMemory(demo->device, demo->swapchain_image_resources[demo->current_buffer].uniform_memory);
}

static void demo_draw(struct Demo *demo) {
    demo->gravity_submit_mgr->AcquireNextImageIndex(demo->current_buffer);

    demo_update_data_buffer(demo);

    demo->gravity_submit_mgr->SubmitAndPresent();
}

static void demo_prepare_depth(struct Demo *demo) {
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.pNext = nullptr;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = depth_format;
    image.extent = {demo->width, demo->height, 1};
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image.flags = 0;

    VkImageViewCreateInfo view = {};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = nullptr;
    view.image = VK_NULL_HANDLE;
    view.format = depth_format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.flags = 0;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image, NULL, &demo->depth.image);
    assert(!err);

    vkGetImageMemoryRequirements(demo->device, demo->depth.image, &mem_reqs);
    assert(!err);

    demo->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    demo->depth.mem_alloc.pNext = NULL;
    demo->depth.mem_alloc.allocationSize = mem_reqs.size;
    demo->depth.mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       &demo->depth.mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(demo->device, &demo->depth.mem_alloc, NULL, &demo->depth.mem);
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(demo->device, demo->depth.image, demo->depth.mem, 0);
    assert(!err);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateImageView(demo->device, &view, NULL, &demo->depth.view);
    assert(!err);
}

/* Load a ppm file into memory */
bool loadTexture(const char *filename, void *void_data, VkSubresourceLayout *layout, int32_t *width, int32_t *height) {
    uint8_t *rgba_data = reinterpret_cast<uint8_t *>(void_data);
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename)].UTF8String;
#endif

#ifdef __ANDROID__
#include <lunarg.ppm.h>
    char *cPtr;
    cPtr = (char *)lunarg_ppm;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "P6\n", 3)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", width, height);
    if (rgba_data == NULL) {
        return true;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "255\n", 4)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;

    for (int y = 0; y < *height; y++) {
        uint8_t *rowPtr = rgba_data;
        for (int x = 0; x < *width; x++) {
            memcpy(rowPtr, cPtr, 3);
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
            cPtr += 3;
        }
        rgba_data += layout->rowPitch;
    }

    return true;
#else
    FILE *fPtr = fopen(filename, "rb");
    char header[256], *cPtr, *tmp;

    if (!fPtr) return false;

    cPtr = fgets(header, 256, fPtr);  // P6
    if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
        fclose(fPtr);
        return false;
    }

    do {
        cPtr = fgets(header, 256, fPtr);
        if (cPtr == NULL) {
            fclose(fPtr);
            return false;
        }
    } while (!strncmp(header, "#", 1));

    sscanf(header, "%u %u", width, height);
    if (rgba_data == NULL) {
        fclose(fPtr);
        return true;
    }
    tmp = fgets(header, 256, fPtr);  // Format
    (void)tmp;
    if (cPtr == NULL || strncmp(header, "255\n", 3)) {
        fclose(fPtr);
        return false;
    }

    for (int y = 0; y < *height; y++) {
        uint8_t *rowPtr = rgba_data;
        for (int x = 0; x < *width; x++) {
            size_t s = fread(rowPtr, 3, 1, fPtr);
            (void)s;
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
        }
        rgba_data += layout->rowPitch;
    }
    fclose(fPtr);
    return true;
#endif
}

static void demo_prepare_texture_image(struct Demo *demo, const char *filename, struct texture_object *tex_obj,
                                       VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props) {
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    int32_t tex_width;
    int32_t tex_height;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    if (!loadTexture(filename, NULL, NULL, &tex_width, &tex_height)) {
        GravityLogger::getInstance().LogFatalError("Failed to load textures");
    }

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = usage;
    image_create_info.flags = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkMemoryRequirements mem_reqs;

    err = vkCreateImage(demo->device, &image_create_info, NULL, &tex_obj->image);
    assert(!err);

    vkGetImageMemoryRequirements(demo->device, tex_obj->image, &mem_reqs);

    tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tex_obj->mem_alloc.pNext = nullptr;
    tex_obj->mem_alloc.allocationSize = mem_reqs.size;
    tex_obj->mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, required_props, &tex_obj->mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(demo->device, &tex_obj->mem_alloc, NULL, &(tex_obj->mem));
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(demo->device, tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        VkImageSubresource subres = {};
        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.mipLevel = 0;
        subres.arrayLayer = 0;
        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres, &layout);

        err = vkMapMemory(demo->device, tex_obj->mem, 0, tex_obj->mem_alloc.allocationSize, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        vkUnmapMemory(demo->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void demo_destroy_texture_image(struct Demo *demo, struct texture_object *tex_objs) {
    /* clean up staging resources */
    vkFreeMemory(demo->device, tex_objs->mem, NULL);
    vkDestroyImage(demo->device, tex_objs->image, NULL);
}

static void demo_prepare_textures(struct Demo *demo) {
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    uint32_t i;

    vkGetPhysicalDeviceFormatProperties(demo->gpu, tex_format, &props);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        VkResult U_ASSERT_ONLY err;

        if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i], VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            // Nothing in the pipeline needs to be complete to start, and don't allow fragment
            // shader to run until layout transition completes
            demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                  demo->textures[i].imageLayout, static_cast<VkAccessFlagBits>(0),
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            demo->staging_texture.image = 0;
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */

            memset(&demo->staging_texture, 0, sizeof(demo->staging_texture));
            demo_prepare_texture_image(demo, tex_files[i], &demo->staging_texture, VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i], VK_IMAGE_TILING_OPTIMAL,
                                       (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            demo_set_image_layout(demo, demo->staging_texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<VkAccessFlagBits>(0),
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<VkAccessFlagBits>(0),
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageCopy copy_region = {};
            copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.srcOffset = {0, 0, 0};
            copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.dstOffset = {0, 0, 0};
            copy_region.extent.width = demo->staging_texture.tex_width;
            copy_region.extent.height = demo->staging_texture.tex_height;
            copy_region.extent.depth = 1;
            vkCmdCopyImage(demo->cmd, demo->staging_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, demo->textures[i].image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            demo_set_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  demo->textures[i].imageLayout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        } else {
            /* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.pNext = nullptr;
        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.mipLodBias = 0.0f;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.maxAnisotropy = 1;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = 0.0f;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler.unnormalizedCoordinates = VK_FALSE;

        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = nullptr;
        view.image = VK_NULL_HANDLE;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = tex_format;
        view.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A,
        };
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        view.flags = 0;

        /* create sampler */
        err = vkCreateSampler(demo->device, &sampler, NULL, &demo->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = demo->textures[i].image;
        err = vkCreateImageView(demo->device, &view, NULL, &demo->textures[i].view);
        assert(!err);
    }
}

void demo_prepare_gravity_data_buffers(struct Demo *demo) {
    VkBufferCreateInfo buf_info;
    VkMemoryRequirements mem_reqs;
    VkMemoryAllocateInfo mem_alloc;
    uint8_t *pData;
    mat4x4 MVP, VP;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;
    struct vktexgravity_vs_uniform data;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_mul(MVP, VP, demo->model_matrix);
    memcpy(data.mvp, MVP, sizeof(MVP));
    //    dumpMatrix("MVP", MVP);

    for (unsigned int i = 0; i < 12 * 3; i++) {
        data.position[i][0] = g_vertex_buffer_data[i * 3];
        data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
        data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2 * i];
        data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);

    for (unsigned int i = 0; i < demo->swapchainImageCount; i++) {
        err = vkCreateBuffer(demo->device, &buf_info, NULL, &demo->swapchain_image_resources[i].uniform_buffer);
        assert(!err);

        vkGetBufferMemoryRequirements(demo->device, demo->swapchain_image_resources[i].uniform_buffer, &mem_reqs);

        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = mem_reqs.size;
        mem_alloc.memoryTypeIndex = 0;

        pass = memory_type_from_properties(demo, mem_reqs.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           &mem_alloc.memoryTypeIndex);
        assert(pass);

        err = vkAllocateMemory(demo->device, &mem_alloc, NULL, &demo->swapchain_image_resources[i].uniform_memory);
        assert(!err);

        err = vkMapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, 0, VK_WHOLE_SIZE, 0, (void **)&pData);
        assert(!err);

        memcpy(pData, &data, sizeof data);

        vkUnmapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory);

        err = vkBindBufferMemory(demo->device, demo->swapchain_image_resources[i].uniform_buffer,
                                 demo->swapchain_image_resources[i].uniform_memory, 0);
        assert(!err);
    }
}

static void demo_prepare_descriptor_layout(struct Demo *demo) {
    VkDescriptorSetLayoutBinding layout_bindings[2] = {{}, {}};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = nullptr;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = DEMO_TEXTURE_COUNT;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = nullptr;
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = layout_bindings;
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(demo->device, &descriptor_layout, NULL, &demo->desc_layout);
    assert(!err);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &demo->desc_layout;

    err = vkCreatePipelineLayout(demo->device, &pPipelineLayoutCreateInfo, NULL, &demo->pipeline_layout);
    assert(!err);
}

static void demo_prepare_render_pass(struct Demo *demo) {
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
    attachments[0].format = demo->gravity_submit_mgr->GetSwapchainVkFormat();
    attachments[0].flags = 0;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format = demo->depth.format;
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
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(demo->device, &rp_info, NULL, &demo->render_pass);
    assert(!err);
}

static VkShaderModule demo_prepare_shader_module(struct Demo *demo, const uint32_t *code, size_t size) {
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkResult U_ASSERT_ONLY err;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = code;

    err = vkCreateShaderModule(demo->device, &moduleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

static void demo_prepare_vs(struct Demo *demo) {
    const uint32_t vs_code[] = {
#include "gravity.vert.inc"
    };
    demo->vert_shader_module = demo_prepare_shader_module(demo, vs_code, sizeof(vs_code));
}

static void demo_prepare_fs(struct Demo *demo) {
    const uint32_t fs_code[] = {
#include "gravity.frag.inc"
    };
    demo->frag_shader_module = demo_prepare_shader_module(demo, fs_code, sizeof(fs_code));
}

static void demo_prepare_pipeline(struct Demo *demo) {
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    demo_prepare_vs(demo);
    demo_prepare_fs(demo);

    // Two stages: vs and fs
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo->vert_shader_module;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo->frag_shader_module;
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->device, &pipelineCache, NULL, &demo->pipelineCache);
    assert(!err);

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.stageCount = ARRAY_SIZE(shaderStages);
    pipeline.pStages = shaderStages;
    pipeline.renderPass = demo->render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = demo->render_pass;

    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, NULL, &demo->pipeline);
    assert(!err);

    vkDestroyShaderModule(demo->device, demo->frag_shader_module, NULL);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module, NULL);
}

static void demo_prepare_descriptor_pool(struct Demo *demo) {
    VkDescriptorPoolSize type_counts[2] = {{}, {}};
    type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_counts[0].descriptorCount = demo->swapchainImageCount;
    type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_counts[1].descriptorCount = demo->swapchainImageCount * DEMO_TEXTURE_COUNT;
    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = nullptr;
    descriptor_pool.maxSets = demo->swapchainImageCount;
    descriptor_pool.poolSizeCount = 2;
    descriptor_pool.pPoolSizes = type_counts;
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(demo->device, &descriptor_pool, NULL, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct Demo *demo) {
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2] = {{}, {}};
    VkResult U_ASSERT_ONLY err;

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = demo->desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &demo->desc_layout;

    VkDescriptorBufferInfo buffer_info;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(struct vktexgravity_vs_uniform);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    for (unsigned int i = 0; i < demo->swapchainImageCount; i++) {
        err = vkAllocateDescriptorSets(demo->device, &alloc_info, &demo->swapchain_image_resources[i].descriptor_set);
        assert(!err);
        buffer_info.buffer = demo->swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    }
}

static void demo_prepare(struct Demo *demo) {
    demo->gravity_submit_mgr->CreateSwapchain();

    VkResult U_ASSERT_ONLY err;
    if (demo->cmd_pool == VK_NULL_HANDLE) {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = demo->gravity_submit_mgr->GetGraphicsQueueIndex();
        cmd_pool_info.flags = 0;
        err = vkCreateCommandPool(demo->device, &cmd_pool_info, NULL, &demo->cmd_pool);
        assert(!err);
    }

    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = nullptr;
    cmd.commandPool = demo->cmd_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.commandBufferCount = 1;
    err = vkAllocateCommandBuffers(demo->device, &cmd, &demo->cmd);
    assert(!err);
    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = nullptr;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = nullptr;
    err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    assert(!err);

    if (demo->is_minimized) {
        demo->prepared = false;
        return;
    }

    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_gravity_data_buffers(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_render_pass(demo);
    demo_prepare_pipeline(demo);

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);

    demo->gravity_submit_mgr->CreateFramebuffers(demo->render_pass, demo->depth.view);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
        demo->current_buffer = i;
        VkCommandBuffer current;
        demo->gravity_submit_mgr->GetRenderCommandBuffer(i, current);
        demo_draw_build_cmd(demo, current);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    demo_flush_init_cmd(demo);
    if (demo->staging_texture.image) {
        demo_destroy_texture_image(demo, &demo->staging_texture);
    }

    demo->current_buffer = 0;
    demo->prepared = true;
}

static void demo_cleanup(struct Demo *demo) {
    uint32_t i;

    demo->prepared = false;
    vkDeviceWaitIdle(demo->device);

    // If the window is currently minimized, demo_resize has already done some cleanup for us.
    if (!demo->is_minimized) {
        vkDestroyDescriptorPool(demo->device, demo->desc_pool, NULL);

        vkDestroyPipeline(demo->device, demo->pipeline, NULL);
        vkDestroyPipelineCache(demo->device, demo->pipelineCache, NULL);
        vkDestroyRenderPass(demo->device, demo->render_pass, NULL);
        vkDestroyPipelineLayout(demo->device, demo->pipeline_layout, NULL);
        vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout, NULL);

        for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
            vkDestroyImageView(demo->device, demo->textures[i].view, NULL);
            vkDestroyImage(demo->device, demo->textures[i].image, NULL);
            vkFreeMemory(demo->device, demo->textures[i].mem, NULL);
            vkDestroySampler(demo->device, demo->textures[i].sampler, NULL);
        }

        vkDestroyImageView(demo->device, demo->depth.view, NULL);
        vkDestroyImage(demo->device, demo->depth.image, NULL);
        vkFreeMemory(demo->device, demo->depth.mem, NULL);

        demo->gravity_submit_mgr->DestroySwapchain();
        for (i = 0; i < demo->swapchainImageCount; i++) {
            vkDestroyBuffer(demo->device, demo->swapchain_image_resources[i].uniform_buffer, NULL);
            vkFreeMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, NULL);
        }
        free(demo->swapchain_image_resources);
    }
    vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);
    vkDeviceWaitIdle(demo->device);
    if (demo->gravity_submit_mgr) {
        delete demo->gravity_submit_mgr;
        demo->gravity_submit_mgr = nullptr;
    }
    vkDestroyDevice(demo->device, NULL);
    GravityLogger::getInstance().DestroyInstanceDebugInfo(demo->inst);
    delete demo->gravity_window;

    vkDestroyInstance(demo->inst, NULL);
}

static void demo_resize(struct Demo *demo) {
    uint32_t i;

    // Don't react to resize until after first initialization.
    if (demo->was_minimized) {
        demo_prepare(demo);
        return;
    }

    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
    demo->prepared = false;
    vkDeviceWaitIdle(demo->device);

    vkDestroyDescriptorPool(demo->device, demo->desc_pool, NULL);

    vkDestroyPipeline(demo->device, demo->pipeline, NULL);
    vkDestroyPipelineCache(demo->device, demo->pipelineCache, NULL);
    vkDestroyRenderPass(demo->device, demo->render_pass, NULL);
    vkDestroyPipelineLayout(demo->device, demo->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(demo->device, demo->textures[i].view, NULL);
        vkDestroyImage(demo->device, demo->textures[i].image, NULL);
        vkFreeMemory(demo->device, demo->textures[i].mem, NULL);
        vkDestroySampler(demo->device, demo->textures[i].sampler, NULL);
    }

    vkDestroyImageView(demo->device, demo->depth.view, NULL);
    vkDestroyImage(demo->device, demo->depth.image, NULL);
    vkFreeMemory(demo->device, demo->depth.mem, NULL);

    demo->gravity_submit_mgr->Resize();
    for (i = 0; i < demo->swapchainImageCount; i++) {
        vkDestroyBuffer(demo->device, demo->swapchain_image_resources[i].uniform_buffer, NULL);
        vkFreeMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, NULL);
    }
    vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);
    demo->cmd_pool = VK_NULL_HANDLE;

    demo->width = demo->gravity_submit_mgr->CurrentWidth();
    demo->height = demo->gravity_submit_mgr->CurrentHeight();

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    demo_prepare(demo);
}

struct Demo g_demo;
void AppForceResize() { demo_resize(&g_demo); }

static bool demo_process_events(struct Demo *demo) {
    std::vector<GravityEvent> current_events;
    if (!GravityEventList::getInstance().GetEvents(current_events)) {
        return false;
    }
    for (auto &cur_event : current_events) {
        switch (cur_event.Type()) {
            case GRAVITY_EVENT_KEY_RELEASE:
                switch (cur_event._data.key) {
                    case GRAVITY_KEYNAME_ESCAPE:
                        demo->quit = true;
                        break;
                    case GRAVITY_KEYNAME_ARROW_LEFT:
                        demo->spin_angle -= demo->spin_increment;
                        break;
                    case GRAVITY_KEYNAME_ARROW_RIGHT:
                        demo->spin_angle += demo->spin_increment;
                        break;
                    case GRAVITY_KEYNAME_SPACE:
                        demo->paused = !demo->paused;
                        break;
                    default:
                        break;
                }
                break;
            case GRAVITY_EVENT_WINDOW_DRAW:
                if (!demo->is_minimized) {
                    demo_draw(demo);
                    demo->curFrame++;
                    if (demo->frameCount != INT_MAX && demo->curFrame == demo->frameCount) {
                        GravityEvent quit_event(GRAVITY_EVENT_QUIT);
                        GravityEventList::getInstance().InsertEvent(quit_event);
                    }
                }
                break;
            case GRAVITY_EVENT_WINDOW_RESIZE:
                // Only resize if the data is different
                if (demo->width != cur_event._data.resize.width || demo->height != cur_event._data.resize.height) {
                    demo->was_minimized = demo->width == 0 || demo->height == 0;
                    demo->is_minimized = cur_event._data.resize.width == 0 || cur_event._data.resize.height == 0;
                    demo->width = cur_event._data.resize.width;
                    demo->height = cur_event._data.resize.height;
                    demo_resize(demo);
                } else {
                    GravityLogger::getInstance().LogInfo("Redundant resize call");
                }
                break;
            case GRAVITY_EVENT_QUIT:
                demo->quit = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
                PostQuitMessage(validation_error);
#endif
                break;
            default:
                break;
        }
    }
    return true;
}

#if defined(VK_USE_PLATFORM_XLIB_KHR)

static void demo_run_xlib(struct Demo *demo) {
    while (!demo->quit) {
        if (demo->paused) {
            demo->gravity_window->HandleXlibEvent();
        }
        demo->gravity_window->HandleAllXlibEvents();
        demo_process_events(demo);

        if (!demo->is_minimized) {
	    demo_draw(demo);
	    demo->curFrame++;
	    if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount) {
	        GravityEvent quit_event(GRAVITY_EVENT_QUIT);
	        GravityEventList::getInstance().InsertEvent(quit_event);
	    }
        }
    }
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)

static void demo_run_xcb(struct Demo *demo) {
    while (!demo->quit) {
        if (demo->paused) {
            demo->gravity_window->HandlePausedXcbEvent();
        }
        demo->gravity_window->HandleAllXcbEvents();
        demo_process_events(demo);

        if (!demo->is_minimized) {
            demo_draw(demo);
            demo->curFrame++;
            if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount) {
                demo->quit = true;
            }
        }
    }
}

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)

static void demo_run(struct Demo *demo) {
    while (!demo->quit) {
        if (demo->paused) {
            demo->gravity_window->HandlePausedWaylandEvent();
        } else {
            demo->gravity_window->HandleActiveWaylandEvents();
        }
        demo_process_events(demo);
        if (!demo->is_minimized) {
            demo_draw(demo);
            demo->curFrame++;
            if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount) demo->quit = true;
        }
    }
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)

static void demo_run(struct Demo *demo) {
    if (!demo->prepared) {
        return;
    }

    demo_draw(demo);
    demo->curFrame++;
}

#endif

static void demo_init_vk(struct Demo *demo) {
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult err;
    demo->enabled_extension_count = 0;
    demo->enabled_layer_count = 0;
    demo->is_minimized = false;
    demo->cmd_pool = VK_NULL_HANDLE;

    void *next_ptr = nullptr;
    std::vector<std::string> instance_layers;
    std::vector<std::string> instance_extensions;
    if (logger.PrepareCreateInstanceItems(instance_layers, instance_extensions, &next_ptr) &&
        demo->gravity_window->PrepareCreateInstanceItems(instance_layers, instance_extensions, &next_ptr) &&
        demo->gravity_submit_mgr->PrepareCreateInstanceItems(instance_layers, instance_extensions, &next_ptr)) {
        for (uint32_t i = 0; i < instance_layers.size(); i++) {
            demo->enabled_layers[demo->enabled_layer_count++] = instance_layers[i].c_str();
        }
        for (uint32_t i = 0; i < instance_extensions.size(); i++) {
            demo->extension_names[demo->enabled_extension_count++] = instance_extensions[i].c_str();
        }
    }

    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr;
    app.pApplicationName = APP_SHORT_NAME;
    app.applicationVersion = 0;
    app.pEngineName = APP_SHORT_NAME;
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = next_ptr;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = demo->enabled_layer_count;
    inst_info.ppEnabledLayerNames = demo->enabled_layers;
    inst_info.enabledExtensionCount = demo->enabled_extension_count;
    inst_info.ppEnabledExtensionNames = demo->extension_names;

    uint32_t gpu_count;

    err = vkCreateInstance(&inst_info, NULL, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        logger.LogFatalError(
            "vkCreateInstance failed: Cannot find a compatible Vulkan installable "
            "client driver (ICD).\n\nPlease look at the Getting Started guide for "
            "additional information.");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        logger.LogFatalError(
            "vkCreateInstance failed: Cannot find a specified extension library.\n"
            "Make sure your layers path is set appropriately.");
    } else if (err) {
        logger.LogFatalError(
            "vkCreateInstance failed: Do you have a compatible Vulkan installable "
            "client driver (ICD) installed?\nPlease look at the Getting Started "
            "guide for additional information.");
    }

    if (!logger.ReleaseCreateInstanceItems(&next_ptr) || !demo->gravity_window->ReleaseCreateInstanceItems(&next_ptr) ||
        !demo->gravity_submit_mgr->ReleaseCreateInstanceItems(&next_ptr)) {
        logger.LogFatalError("Failed cleaning up after creating instance");
    }
    logger.CreateInstanceDebugInfo(demo->inst);

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);

    if (gpu_count > 0) {
        VkPhysicalDevice *physical_devices = new VkPhysicalDevice[gpu_count];
        err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, physical_devices);
        assert(!err);
        /* For gravity demo we just grab the first physical device */
        demo->gpu = physical_devices[0];
        delete[] physical_devices;
    } else {
        logger.LogFatalError(
            "vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
    }

    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(demo->gpu, &physDevFeatures);
}

static void demo_create_device(struct Demo *demo) {
    VkResult U_ASSERT_ONLY err;
    GravityLogger &logger = GravityLogger::getInstance();
    VkDeviceCreateInfo device_create_info = {};

    /* Look for device extensions */
    demo->enabled_extension_count = 0;
    memset(demo->extension_names, 0, sizeof(demo->extension_names));

    demo->gravity_submit_mgr = new GravitySubmitManager(demo->gravity_window, demo->inst, demo->gpu);
    assert(demo->gravity_submit_mgr != nullptr);

    void *next_ptr = nullptr;
    std::vector<std::string> device_extensions;
    if (demo->gravity_submit_mgr->PrepareCreateDeviceItems(device_create_info, device_extensions, &next_ptr)) {
        for (uint32_t i = 0; i < device_extensions.size(); i++) {
            demo->extension_names[demo->enabled_extension_count++] = device_extensions[i].c_str();
        }
    }

    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.pEnabledFeatures = nullptr;  // If specific features are required, pass them in here
    device_create_info.enabledExtensionCount = demo->enabled_extension_count;
    device_create_info.ppEnabledExtensionNames = demo->extension_names;
    device_create_info.pNext = next_ptr;
    err = vkCreateDevice(demo->gpu, &device_create_info, NULL, &demo->device);
    assert(!err);

    if (!demo->gravity_submit_mgr->ReleaseCreateDeviceItems(device_create_info, &next_ptr)) {
        logger.LogFatalError("Failed cleaning up after creating device");
    }
}

static void demo_init_vk_swapchain(struct Demo *demo) {
    demo_create_device(demo);

    if (!demo->gravity_submit_mgr->PrepareForSwapchain(demo->device, 3, demo->presentMode, VK_FORMAT_B8G8R8A8_SRGB,
                                                       VK_FORMAT_B8G8R8A8_UNORM)) {
        assert(false);
    }
    demo->swapchainImageCount = demo->gravity_submit_mgr->NumSwapchainImages();
    demo->swapchain_image_resources =
        (SwapchainImageResources *)malloc(demo->swapchainImageCount * sizeof(SwapchainImageResources));
    assert(demo->swapchain_image_resources);

    demo->quit = false;
    demo->curFrame = 0;

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(demo->gpu, &demo->memory_properties);
}

static void demo_init(struct Demo *demo, int argc, char **argv) {
    GravityLogger &logger = GravityLogger::getInstance();

    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));
    demo->presentMode = VK_PRESENT_MODE_FIFO_KHR;
    demo->frameCount = INT32_MAX;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--use_staging") == 0) {
            demo->use_staging_buffer = true;
            continue;
        }
        if ((strcmp(argv[i], "--present_mode") == 0) && (i < argc - 1)) {
            demo->presentMode = static_cast<VkPresentModeKHR>(atoi(argv[i + 1]));
            i++;
            continue;
        }
        if (strcmp(argv[i], "--break") == 0) {
            GravityLogger::getInstance().EnableBreakOnError(true);
            demo->use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) {
            GravityLogger::getInstance().EnableValidation(true);
            demo->validate = true;
            continue;
        }
        if (strcmp(argv[i], "--xlib") == 0) {
            fprintf(stderr, "--xlib is deprecated and no longer does anything");
            continue;
        }
        if (strcmp(argv[i], "--c") == 0 && demo->frameCount == INT32_MAX && i < argc - 1 &&
            sscanf(argv[i + 1], "%d", &demo->frameCount) == 1 && demo->frameCount >= 0) {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--suppress_popups") == 0) {
            demo->suppress_popups = true;
            logger.EnablePopups(false);
            continue;
        }
        if (strcmp(argv[i], "--display_timing") == 0) {
            demo->VK_GOOGLE_display_timing_enabled = true;
            continue;
        }

#if defined(ANDROID)
        GravityLogger::getInstance().LogFatalError("Usage: gravity [--validate]");
#else
        fprintf(stderr,
                "Usage:\n  %s\t[--use_staging] [--validate] [--break]\n"
                "\t[--c <framecount>] [--suppress_popups] [--display_timing]\n"
                "\t[--present_mode <present mode enum>]\n"
                "\t <present_mode_enum>\tVK_PRESENT_MODE_IMMEDIATE_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_MAILBOX_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_FIFO_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_FIFO_RELAXED_KHR = %d\n",
                APP_SHORT_NAME, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR,
                VK_PRESENT_MODE_FIFO_RELAXED_KHR);
        fflush(stderr);
        exit(1);
#endif
    }

    demo->paused = false;
    demo->is_minimized = false;
    demo->was_minimized = false;

    demo->gravity_window = new GravityWindow(APP_SHORT_NAME);
    if (!GravityEventList::getInstance().Alloc(100)) {
        GravityLogger::getInstance().LogFatalError("Failed allocating space for events");
    }

    demo_init_vk(demo);

    demo->width = 500;
    demo->height = 500;

    demo->spin_angle = 4.0f;
    demo->spin_increment = 0.2f;

    mat4x4_perspective(demo->projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
    mat4x4_identity(demo->model_matrix);

    demo->projection_matrix[1][1] *= -1;  // Flip projection matrix from GL to Vulkan orientation.
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
// Include header required for parsing the command line options.
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    bool done;  // flag saying when app is complete
    int argc;
    char **argv;
    Demo demo;

    // Use the CommandLine functions to get the command line arguments.
    // Unfortunately, Microsoft outputs
    // this information as wide characters for Unicode, and we simply want the
    // Ascii version to be compatible
    // with the non-Windows side.  So, we have to convert the information to
    // Ascii character strings.
    LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (NULL == commandLineArgs) {
        argc = 0;
    }

    if (argc > 0) {
        argv = (char **)malloc(sizeof(char *) * argc);
        if (argv == NULL) {
            argc = 0;
        } else {
            for (int iii = 0; iii < argc; iii++) {
                size_t wideCharLen = wcslen(commandLineArgs[iii]);
                size_t numConverted = 0;

                argv[iii] = (char *)malloc(sizeof(char) * (wideCharLen + 1));
                if (argv[iii] != NULL) {
                    wcstombs_s(&numConverted, argv[iii], wideCharLen + 1, commandLineArgs[iii], wideCharLen + 1);
                }
            }
        }
    } else {
        argv = NULL;
    }

    demo_init(&demo, argc, argv);

    // Free up the items we had to allocate for the command line arguments.
    if (argc > 0 && argv != NULL) {
        for (int iii = 0; iii < argc; iii++) {
            if (argv[iii] != NULL) {
                free(argv[iii]);
            }
        }
        free(argv);
    }

    demo.gravity_window->SetHInstance(hInstance);
    demo.gravity_window->CreatePlatformWindow(demo.inst, demo.gpu, demo.width, demo.height);
    demo_init_vk_swapchain(&demo);

    demo_prepare(&demo);

    done = false;  // initialize loop condition variable

    // main message loop
    while (!demo.quit) {
        MSG msg = { 0 };
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)  // check for a quit message
        {
            GravityEvent quit_event(GRAVITY_EVENT_QUIT);
            GravityEventList::getInstance().InsertEvent(quit_event);
        } else {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            demo_process_events(&demo);
            if (demo.quit) {
                break;
            }
            if (!demo.is_minimized) {
                demo_draw(&demo);
            }
        }
    }

    demo_cleanup(&demo);

    return 0;
}

#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)

static void demo_main(struct Demo *demo, void *view) {
    const char *argv[] = {"GravitySample"};
    int argc = sizeof(argv) / sizeof(char *);

    demo_init(demo, argc, (char **)argv);
    demo->gravity_window->SetMoltenVkView(view);
    demo_init_vk_swapchain(demo);
    demo_prepare(demo);
    demo->spin_angle = 0.4f;
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/log.h>
#include <android_native_app_glue.h>
#include "android_util.h"

static bool initialized = false;
static bool active = false;
struct Demo demo;

static int32_t processInput(struct android_app *app, AInputEvent *event) { return 0; }

static void processCommand(struct android_app *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            if (app->window) {
                // We're getting a new window.  If the app is starting up, we
                // need to initialize.  If the app has already been
                // initialized, that means that we lost our previous window,
                // which means that we have a lot of work to do.  At a minimum,
                // we need to destroy the swapchain and surface associated with
                // the old window, and create a new surface and swapchain.
                // However, since there are a lot of other objects/state that
                // is tied to the swapchain, it's easiest to simply cleanup and
                // start over (i.e. use a brute-force approach of re-starting
                // the app)
                if (demo.prepared) {
                    demo_cleanup(&demo);
                }

                // Parse Intents into argc, argv
                // Use the following key to send arguments, i.e.
                // --es args "--validate"
                const char key[] = "args";
                char *appTag = (char *)APP_SHORT_NAME;
                int argc = 0;
                char **argv = get_args(app, key, appTag, &argc);

                __android_log_print(ANDROID_LOG_INFO, appTag, "argc = %i", argc);
                for (int i = 0; i < argc; i++) __android_log_print(ANDROID_LOG_INFO, appTag, "argv[%i] = %s", i, argv[i]);

                demo_init(&demo, argc, argv);

                // Free the argv malloc'd by get_args
                for (int i = 0; i < argc; i++) free(argv[i]);

                demo.window = (void *)app->window;
                demo_init_vk_swapchain(&demo);
                demo_prepare(&demo);
                initialized = true;
            }
            break;
        }
        case APP_CMD_GAINED_FOCUS: {
            active = true;
            break;
        }
        case APP_CMD_LOST_FOCUS: {
            active = false;
            break;
        }
    }
}

void android_main(struct android_app *app) {
#ifdef ANDROID
    int vulkanSupport = InitVulkan();
    if (vulkanSupport == 0) return;
#endif

    demo.prepared = false;

    app->onAppCmd = processCommand;
    app->onInputEvent = processInput;

    while (1) {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollAll(active ? 0 : -1, NULL, &events, (void **)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                demo_cleanup(&demo);
                return;
            }
        }
        demo_process_events(&demo);
        if (initialized && active) {
            demo_run(&demo);
        }
    }
}
#else
int main(int argc, char **argv) {
    demo_init(&g_demo, argc, argv);
    g_demo.gravity_window->CreatePlatformWindow(g_demo.inst, g_demo.gpu, g_demo.width, g_demo.height);

    demo_init_vk_swapchain(&g_demo);

    demo_prepare(&g_demo);

#if defined(VK_USE_PLATFORM_XCB_KHR)
    demo_run_xcb(&g_demo);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    demo_run_xlib(&g_demo);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    demo_run(&g_demo);
#endif

    demo_cleanup(&g_demo);

    return validation_error;
}
#endif
