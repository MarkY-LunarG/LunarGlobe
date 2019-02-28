//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_texture.cpp
// Copyright(C):            2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_submit_manager.hpp"
#include "globe_shader.hpp"
#include "globe_font.hpp"
#include "globe_resource_manager.hpp"
#include "globe_app.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

GlobeFont* GlobeFont::GenerateFont(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                   VkDevice vk_device, VkCommandBuffer vk_command_buffer, const std::string& font_name,
                                   GlobeFontData& font_data) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    if (!InitFromContent(resource_manager, submit_manager, vk_device, vk_command_buffer, font_name,
                         font_data.texture_data)) {
        std::string error_message = "GenerateFont - Failed to setting up font for Vulkan \"";
        error_message += font_name;
        error_message += "\"";
        logger.LogError(error_message);
        return nullptr;
    }
    delete font_data.texture_data.standard_data;

    GlobeFont* font = new GlobeFont(resource_manager, vk_device, font_name, &font_data);
    if (nullptr == font) {
        logger.LogError("GenerateFont - Failed creating GlobeFont");
        return nullptr;
    }
    return font;
}

GlobeFont* GlobeFont::LoadFontMap(GlobeResourceManager* resource_manager, GlobeSubmitManager* submit_manager,
                                  VkDevice vk_device, VkCommandBuffer vk_command_buffer, uint32_t character_pixel_size,
                                  const std::string& font_name, const std::string& directory) {
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(__ANDROID__))
// filename = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@(filename.c_str())].UTF8String;
#error("Unsupported platform")
#elif defined(__ANDROID__)
#else
    // https://stackoverflow.com/questions/51276586/how-to-render-text-in-directx9-with-stb-truetype
    // http://www.cs.unh.edu/~cs770/lwjgl-javadoc/lwjgl-stb/org/lwjgl/stb/STBTruetype.html#stbtt_GetCodepointBitmapBox-org.lwjgl.stb.STBTTFontinfo-int-float-float-java.nio.IntBuffer-java.nio.IntBuffer-java.nio.IntBuffer-java.nio.IntBuffer-
    GlobeLogger& logger = GlobeLogger::getInstance();
    GlobeFontData font_data = {};
    std::string font_file_name = directory;
    font_file_name += font_name;

    FILE* file_ptr = fopen(font_file_name.c_str(), "rb");
    if (nullptr == file_ptr) {
        std::string error_string = "LoadFontMap - Failed to open file ";
        error_string += font_file_name;
        logger.LogError(error_string);
        return nullptr;
    }

    // Determine how big the file is and then read it.
    fseek(file_ptr, 0, SEEK_END);
    size_t file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);
    std::vector<uint8_t> file_contents;
    file_contents.resize(file_size);
    fread(file_contents.data(), file_size, 1, file_ptr);
    fclose(file_ptr);

    // Initialize the font based on the file contents
    stbtt_fontinfo font_info = {};
    if (!stbtt_InitFont(&font_info, file_contents.data(), 0)) {
        std::string error_string = "LoadFontMap - loading font contents for file ";
        error_string += font_file_name;
        logger.LogError(error_string);
        return nullptr;
    }

    // Reserve enough space for the font bitmap for 100 characters of
    // character_pixel_size * character_pixel_size.  Really, we need 95
    // characters, but I want to have some spacing between rows.
    int32_t bitmap_width = character_pixel_size * 10;
    int32_t bitmap_height = character_pixel_size * 10;
    std::vector<uint8_t> font_bitmap;
    font_bitmap.resize(bitmap_width * bitmap_height);

    // Determine the scaling required for the font to be the size we want.
    float font_scale = stbtt_ScaleForPixelHeight(&font_info, character_pixel_size);

    // We'll do a padding of 2 pixels between each character and take one for each character.
    int32_t padding = 2;

    // Determine the font properties and adjust by scale.
    int32_t font_ascent;
    int32_t font_descent;
    int32_t font_line_gap;
    int32_t row_increment;
    stbtt_GetFontVMetrics(&font_info, &font_ascent, &font_descent, &font_line_gap);
    font_ascent = static_cast<int32_t>(static_cast<float>(font_ascent) * font_scale);
    font_descent = static_cast<int32_t>(static_cast<float>(font_descent) * font_scale);
    font_line_gap = static_cast<int32_t>(static_cast<float>(font_line_gap) * font_scale);
    row_increment = font_ascent - font_descent + font_line_gap + padding;

    int32_t x = padding;
    int32_t cur_row_y = padding;
    int32_t cur_char_left;
    int32_t cur_char_right;
    int32_t cur_char_bottom;
    int32_t cur_char_top;
    int32_t char_width;
    int32_t max_y = 0;
    font_data.char_data.resize(GLOBE_FONT_ENDING_ASCII_CHAR - GLOBE_FONT_STARTING_ASCII_CHAR + 1);
    for (uint8_t ascii_char = GLOBE_FONT_STARTING_ASCII_CHAR; ascii_char <= GLOBE_FONT_ENDING_ASCII_CHAR;
         ++ascii_char) {
        GlobeFontCharData* char_data = &font_data.char_data[ascii_char - GLOBE_FONT_STARTING_ASCII_CHAR];
        // Get the horizontal metric for this character.
        // The returned values are expressed in unscaled coordinates.
        stbtt_GetCodepointHMetrics(&font_info, ascii_char, &char_width, nullptr);
        char_width = static_cast<int32_t>(static_cast<float>(char_width) * font_scale);

        // Get the character's font characteristic's bounding box
        cur_char_left = 0;
        cur_char_right = 0;
        cur_char_bottom = 0;
        cur_char_top = 0;
        stbtt_GetCodepointBitmapBox(&font_info, ascii_char, font_scale, font_scale, &cur_char_left, &cur_char_bottom,
                                    &cur_char_right, &cur_char_top);

        if ((x + char_width) > bitmap_width) {
            cur_row_y += row_increment;
            x = padding;
        }

        // Determine the height of the current character.
        int32_t y = cur_row_y + font_ascent + cur_char_bottom;
        int32_t offset = x + (y * bitmap_width);
        int32_t next_x = x + char_width;

        // Render the current ascii character to the bitmap.
        int32_t char_bitmap_width = cur_char_right - cur_char_left;
        int32_t char_bitmap_height = cur_char_top - cur_char_bottom;
        stbtt_MakeCodepointBitmap(&font_info, font_bitmap.data() + offset, char_bitmap_width, char_bitmap_height,
                                  bitmap_width, font_scale, font_scale, ascii_char);

        // Make sure to also take into account the kerning as well (assume this is followed by
        // a character that goes up to the left like '[').
        int32_t char_kerning = stbtt_GetCodepointKernAdvance(&font_info, ascii_char, 91);
        next_x += static_cast<int32_t>(static_cast<float>(char_kerning) * font_scale) + padding;

        char_data->left_u = static_cast<float>(x - 1);
        char_data->top_v = static_cast<float>(cur_row_y - 1);
        char_data->right_u = static_cast<float>(next_x - 1);
        char_data->bottom_v = static_cast<float>(cur_row_y + row_increment - 2);
        char_data->width = char_width;

        x = next_x;
        if (y + char_bitmap_height + padding > max_y) {
            max_y = y + char_bitmap_height + padding;
        }
    }
    bitmap_height = max_y;
    font_data.generated_size = static_cast<float>(character_pixel_size);

    float inv_x = 1.f / static_cast<float>(bitmap_width);
    float inv_y = 1.f / static_cast<float>(bitmap_height);
    for (auto& char_data : font_data.char_data) {
        char_data.left_u *= inv_x;
        char_data.top_v = (char_data.top_v * inv_y);
        char_data.right_u *= inv_x;
        char_data.bottom_v = (char_data.bottom_v * inv_y);
    }

#if 0  // Debug Font loading (output to PPM to make sure it looks right)
    std::string file_name = font_name + ".ppm";
    FILE *fp = fopen(file_name.c_str(), "wt");
    if (fp) {
        fprintf(fp, "P3\n");
        fprintf(fp, "%d %d\n", bitmap_width, max_y);
        fprintf(fp, "255\n");

        for (uint32_t row = 0; row < max_y; ++row) {
            for (uint32_t col = 0; col < bitmap_width; ++col) {
                uint8_t cur_value = font_bitmap[row * bitmap_width + col];
                fprintf(fp, "%3d %3d %3d", cur_value, cur_value, cur_value);
                if ((col % 4) == 3) {
                    fprintf(fp, "\n");
                } else {
                    fprintf(fp, "     ");
                }
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
#endif

    font_data.texture_data.uses_standard_data = true;
    font_data.texture_data.standard_data = new GlobeStandardTextureData();
    if (font_data.texture_data.standard_data == nullptr) {
        std::string error_string = "LoadFontMap - loading font contents for file ";
        error_string += font_file_name;
        logger.LogError(error_string);
        return nullptr;
    }

    font_data.texture_data.width = bitmap_width;
    font_data.texture_data.height = bitmap_height;
    font_data.texture_data.num_mip_levels = 1;
    font_data.texture_data.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
    font_data.texture_data.vk_format_props = resource_manager->GetVkFormatProperties(font_data.texture_data.vk_format);
    GlobeTextureLevel level_data = {};
    level_data.width = bitmap_width;
    level_data.height = bitmap_height;
    level_data.data_size = bitmap_width * bitmap_height * 4;
    font_data.texture_data.standard_data->levels.push_back(level_data);
    font_data.texture_data.standard_data->raw_data.resize(level_data.data_size);
    uint8_t* dst_ptr = reinterpret_cast<uint8_t*>(font_data.texture_data.standard_data->raw_data.data());
    uint8_t* src_ptr = font_bitmap.data();
    for (int32_t row = 0; row < font_data.texture_data.height; ++row) {
        for (int32_t col = 0; col < font_data.texture_data.width; ++col) {
            *dst_ptr++ = *src_ptr;
            *dst_ptr++ = *src_ptr;
            *dst_ptr++ = *src_ptr++;
            *dst_ptr++ = 255;
        }
    }

    return GenerateFont(resource_manager, submit_manager, vk_device, vk_command_buffer, font_name, font_data);
#endif
}

GlobeFont::GlobeFont(GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& font_name,
                     GlobeFontData* font_data)
    : GlobeTexture(resource_manager, vk_device, font_name, &font_data->texture_data), _font_name(font_name) {
    _generated_size = font_data->generated_size;
    _char_data = std::move(font_data->char_data);
    _vk_descriptor_set_layout = VK_NULL_HANDLE;
    _vk_pipeline_layout = VK_NULL_HANDLE;
    _vk_descriptor_pool = VK_NULL_HANDLE;
    _vk_descriptor_set = VK_NULL_HANDLE;
    _vk_pipeline = VK_NULL_HANDLE;
}

GlobeFont::~GlobeFont() {
    RemoveAllStrings();
    UnloadFromRenderPass();
}

bool GlobeFont::LoadIntoRenderPass(VkRenderPass render_pass, float viewport_width, float viewport_height) {
    GlobeLogger& logger = GlobeLogger::getInstance();

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
    descriptor_set_layout_binding.binding = 1;
    descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_binding.descriptorCount = 1;
    descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {};
    descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout.pNext = nullptr;
    descriptor_set_layout.bindingCount = 1;
    descriptor_set_layout.pBindings = &descriptor_set_layout_binding;
    if (VK_SUCCESS !=
        vkCreateDescriptorSetLayout(_vk_device, &descriptor_set_layout, nullptr, &_vk_descriptor_set_layout)) {
        logger.LogError("GlobeFont failed to create descriptor set layout");
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
    if (VK_SUCCESS != vkCreatePipelineLayout(_vk_device, &pipeline_layout_create_info, nullptr, &_vk_pipeline_layout)) {
        logger.LogError("GlobeFont failed to create pipeline layout layout");
        return false;
    }

    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 1;
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &pool_size;
    if (VK_SUCCESS != vkCreateDescriptorPool(_vk_device, &descriptor_pool_create_info, nullptr, &_vk_descriptor_pool)) {
        logger.LogError("GlobeFont failed to create descriptor pool");
        return false;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = NULL;
    descriptor_set_allocate_info.descriptorPool = _vk_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_vk_descriptor_set_layout;
    if (VK_SUCCESS != vkAllocateDescriptorSets(_vk_device, &descriptor_set_allocate_info, &_vk_descriptor_set)) {
        logger.LogError("GlobeFont failed to allocate descriptor set");
        return false;
    }

    VkDescriptorImageInfo image_info = {};
    image_info.sampler = GetVkSampler();
    image_info.imageView = GetVkImageView();
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = _vk_descriptor_set;
    write_set.dstBinding = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_set.descriptorCount = 1;
    write_set.pImageInfo = &image_info;
    vkUpdateDescriptorSets(_vk_device, 1, &write_set, 0, nullptr);

    // No need to tell it anything about input state right now.
    VkVertexInputBindingDescription vertex_input_binding_description = {};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding_description.stride = sizeof(float) * 16;
    VkVertexInputAttributeDescription vertex_input_attribute_description[4];
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
    vertex_input_attribute_description[2] = {};
    vertex_input_attribute_description[2].binding = 0;
    vertex_input_attribute_description[2].location = 2;
    vertex_input_attribute_description[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[2].offset = sizeof(float) * 8;
    vertex_input_attribute_description[3] = {};
    vertex_input_attribute_description[3].binding = 0;
    vertex_input_attribute_description[3].location = 3;
    vertex_input_attribute_description[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[3].offset = sizeof(float) * 12;
    VkPipelineVertexInputStateCreateInfo pipline_vert_input_state_create_info = {};
    pipline_vert_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipline_vert_input_state_create_info.pNext = NULL;
    pipline_vert_input_state_create_info.flags = 0;
    pipline_vert_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipline_vert_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipline_vert_input_state_create_info.vertexAttributeDescriptionCount = 4;
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

    // Enable color blending
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
    pipeline_color_blend_attachment_state.blendEnable = VK_TRUE;
    pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

    // Setup viewport and scissor
    VkRect2D scissor_rect = {};
    scissor_rect.offset.x = 0;
    scissor_rect.offset.y = 0;
    scissor_rect.extent.width = viewport_width;
    scissor_rect.extent.height = viewport_height;
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewport_width;
    viewport.height = (float)viewport_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = &viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = &scissor_rect;

    // Depth stencil state (no depth for font rendering)
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_NEVER;
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

    GlobeShader* font_shader = _globe_resource_mgr->LoadShader("poscolortex_pushmat");
    if (nullptr == font_shader) {
        logger.LogError("GlobeFont failed to load poscolortex_pushmat shaders");
        return false;
    }
    std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
    font_shader->GetPipelineShaderStages(pipeline_shader_stage_create_info);

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
    gfx_pipeline_create_info.renderPass = render_pass;
    gfx_pipeline_create_info.pDynamicState = nullptr;
    if (VK_SUCCESS !=
        vkCreateGraphicsPipelines(_vk_device, VK_NULL_HANDLE, 1, &gfx_pipeline_create_info, nullptr, &_vk_pipeline)) {
        logger.LogError("GlobeFont failed to create graphics pipeline");
        return false;
    }

    _globe_resource_mgr->FreeShader(font_shader);
    return true;
}

void GlobeFont::UnloadFromRenderPass() {
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
    if (VK_NULL_HANDLE != _vk_pipeline_layout) {
        vkDestroyPipelineLayout(_vk_device, _vk_pipeline_layout, nullptr);
        _vk_pipeline_layout = VK_NULL_HANDLE;
    }
    if (_vk_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_vk_device, _vk_descriptor_set_layout, nullptr);
        _vk_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

int32_t GlobeFont::AddString(const std::string& text_string, glm::vec3 fg_color, glm::vec4 bg_color,
                             glm::vec3 starting_pos, glm::vec3 text_direction, glm::vec3 text_up,
                             float model_space_char_height, uint32_t queue_family_index) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    int32_t string_index = -1;
    if (text_string.length() > 0 && model_space_char_height > 0) {
        GlobeFontStringData string_data = {};
        string_data.queue_family_index = queue_family_index;
        string_data.text_string = text_string;
        string_data.starting_pos = starting_pos;
        string_data.num_vertices = static_cast<uint32_t>(text_string.length()) * 4;
        string_data.num_indices = static_cast<uint32_t>(text_string.length()) * 6;
        glm::vec3 cur_pos = starting_pos;
        float size_multiplier = model_space_char_height / _generated_size;
        if (size_multiplier > 1.1f) {
            logger.LogWarning("Font is being scaled up to a point pixelation may be obvious");
        }
        uint32_t cur_index = 0;
        for (uint32_t char_index = 0; char_index < text_string.length(); ++char_index) {
            GlobeFontCharData* char_data = &_char_data[text_string[char_index] - GLOBE_FONT_STARTING_ASCII_CHAR];

            // Determine the adjusted character width based on the generated font character
            // width and the size scale multiplier.
            float character_width = char_data->width * size_multiplier;

            // Pre-calculate the other vertex positions for the character quad.
            glm::vec3 top_left_pos = cur_pos + (text_up * model_space_char_height);
            glm::vec3 top_right_pos =
                cur_pos + (text_direction * character_width) + (text_up * model_space_char_height);
            glm::vec3 bottom_right_pos = cur_pos + (text_direction * character_width);

            // Bottom left
            string_data.vertex_data.push_back(cur_pos[0]);
            string_data.vertex_data.push_back(cur_pos[1]);
            string_data.vertex_data.push_back(cur_pos[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(fg_color[0]);
            string_data.vertex_data.push_back(fg_color[1]);
            string_data.vertex_data.push_back(fg_color[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(bg_color[0]);
            string_data.vertex_data.push_back(bg_color[1]);
            string_data.vertex_data.push_back(bg_color[2]);
            string_data.vertex_data.push_back(bg_color[3]);
            string_data.vertex_data.push_back(char_data->left_u);
            string_data.vertex_data.push_back(char_data->bottom_v);
            string_data.vertex_data.push_back(0.f);
            string_data.vertex_data.push_back(1.f);

            // Bottom right
            string_data.vertex_data.push_back(bottom_right_pos[0]);
            string_data.vertex_data.push_back(bottom_right_pos[1]);
            string_data.vertex_data.push_back(bottom_right_pos[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(fg_color[0]);
            string_data.vertex_data.push_back(fg_color[1]);
            string_data.vertex_data.push_back(fg_color[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(bg_color[0]);
            string_data.vertex_data.push_back(bg_color[1]);
            string_data.vertex_data.push_back(bg_color[2]);
            string_data.vertex_data.push_back(bg_color[3]);
            string_data.vertex_data.push_back(char_data->right_u);
            string_data.vertex_data.push_back(char_data->bottom_v);
            string_data.vertex_data.push_back(0.f);
            string_data.vertex_data.push_back(1.f);

            // Top right
            string_data.vertex_data.push_back(top_right_pos[0]);
            string_data.vertex_data.push_back(top_right_pos[1]);
            string_data.vertex_data.push_back(top_right_pos[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(fg_color[0]);
            string_data.vertex_data.push_back(fg_color[1]);
            string_data.vertex_data.push_back(fg_color[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(bg_color[0]);
            string_data.vertex_data.push_back(bg_color[1]);
            string_data.vertex_data.push_back(bg_color[2]);
            string_data.vertex_data.push_back(bg_color[3]);
            string_data.vertex_data.push_back(char_data->right_u);
            string_data.vertex_data.push_back(char_data->top_v);
            string_data.vertex_data.push_back(0.f);
            string_data.vertex_data.push_back(1.f);

            // Top left
            string_data.vertex_data.push_back(top_left_pos[0]);
            string_data.vertex_data.push_back(top_left_pos[1]);
            string_data.vertex_data.push_back(top_left_pos[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(fg_color[0]);
            string_data.vertex_data.push_back(fg_color[1]);
            string_data.vertex_data.push_back(fg_color[2]);
            string_data.vertex_data.push_back(1.f);
            string_data.vertex_data.push_back(bg_color[0]);
            string_data.vertex_data.push_back(bg_color[1]);
            string_data.vertex_data.push_back(bg_color[2]);
            string_data.vertex_data.push_back(bg_color[3]);
            string_data.vertex_data.push_back(char_data->left_u);
            string_data.vertex_data.push_back(char_data->top_v);
            string_data.vertex_data.push_back(0.f);
            string_data.vertex_data.push_back(1.f);

            // Update indices
            string_data.index_data.push_back(cur_index);
            string_data.index_data.push_back(cur_index + 1);
            string_data.index_data.push_back(cur_index + 2);
            string_data.index_data.push_back(cur_index);
            string_data.index_data.push_back(cur_index + 2);
            string_data.index_data.push_back(cur_index + 3);

            // Update index and pos
            cur_index += 4;
            cur_pos = bottom_right_pos;
        }

        // Create the vertex buffer
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_create_info.size = string_data.vertex_data.size() * sizeof(float);
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.flags = 0;
        if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &string_data.vertex_buffer.vk_buffer)) {
            logger.LogError("Failed to create vertex buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                string_data.vertex_buffer.vk_buffer,
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                string_data.vertex_buffer.vk_memory, string_data.vertex_buffer.vk_size)) {
            logger.LogError("Failed to allocate vertex buffer memory");
            return false;
        }
        uint8_t* mapped_data;
        if (VK_SUCCESS != vkMapMemory(_vk_device, string_data.vertex_buffer.vk_memory, 0,
                                      string_data.vertex_buffer.vk_size, 0, (void**)&mapped_data)) {
            logger.LogError("Failed to map vertex buffer memory");
            return false;
        }
        memcpy(mapped_data, string_data.vertex_data.data(), string_data.vertex_data.size() * sizeof(float));
        vkUnmapMemory(_vk_device, string_data.vertex_buffer.vk_memory);
        if (VK_SUCCESS != vkBindBufferMemory(_vk_device, string_data.vertex_buffer.vk_buffer,
                                             string_data.vertex_buffer.vk_memory, 0)) {
            logger.LogError("Failed to bind vertex buffer memory");
            return false;
        }

        // Create the index buffer
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_create_info.size = string_data.index_data.size() * sizeof(uint32_t);
        if (VK_SUCCESS != vkCreateBuffer(_vk_device, &buffer_create_info, NULL, &string_data.index_buffer.vk_buffer)) {
            logger.LogError("Failed to create index buffer");
            return false;
        }
        if (!_globe_resource_mgr->AllocateDeviceBufferMemory(
                string_data.index_buffer.vk_buffer,
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                string_data.index_buffer.vk_memory, string_data.index_buffer.vk_size)) {
            logger.LogError("Failed to allocate index buffer memory");
            return false;
        }
        if (VK_SUCCESS != vkMapMemory(_vk_device, string_data.index_buffer.vk_memory, 0,
                                      string_data.index_buffer.vk_size, 0, (void**)&mapped_data)) {
            logger.LogError("Failed to map index buffer memory");
            return false;
        }
        memcpy(mapped_data, string_data.index_data.data(), string_data.index_data.size() * sizeof(uint32_t));
        vkUnmapMemory(_vk_device, string_data.index_buffer.vk_memory);
        if (VK_SUCCESS !=
            vkBindBufferMemory(_vk_device, string_data.index_buffer.vk_buffer, string_data.index_buffer.vk_memory, 0)) {
            logger.LogError("Failed to bind index buffer memory");
            return false;
        }

        string_index = static_cast<int32_t>(_string_data.size());
        _string_data.push_back(string_data);
    }
    return string_index;
}

bool GlobeFont::UpdateStringText(int32_t string_index, const std::string& text_string) {
    GlobeLogger& logger = GlobeLogger::getInstance();
    std::string error_message;

    if (string_index >= 0 && string_index <= _string_data.size()) {
        GlobeFontStringData& string_data = _string_data[string_index];
        if (string_data.text_string.length() != text_string.length()) {
            error_message = "UpdateStringText - Incoming string is ";
            error_message += std::to_string(text_string.length());
            error_message += " in length, but attempting to replace string ";
            error_message += std::to_string(string_data.text_string.length());
            error_message += " in length.";
            logger.LogError(error_message);
            return false;
        }
        float* mapped_data;
        if (VK_SUCCESS != vkMapMemory(_vk_device, string_data.vertex_buffer.vk_memory, 0,
                                      string_data.vertex_buffer.vk_size, 0, (void**)&mapped_data)) {
            logger.LogError("UpdateStringText - Failed to map vertex buffer memory");
        }
        uint32_t mapped_index = 0;
        for (uint32_t char_index = 0; char_index < text_string.length(); ++char_index) {
            GlobeFontCharData* char_data = &_char_data[text_string[char_index] - GLOBE_FONT_STARTING_ASCII_CHAR];
            mapped_data[mapped_index + 12] = char_data->left_u;
            mapped_data[mapped_index + 13] = char_data->bottom_v;
            mapped_data[mapped_index + 28] = char_data->right_u;
            mapped_data[mapped_index + 29] = char_data->bottom_v;
            mapped_data[mapped_index + 44] = char_data->right_u;
            mapped_data[mapped_index + 45] = char_data->top_v;
            mapped_data[mapped_index + 60] = char_data->left_u;
            mapped_data[mapped_index + 61] = char_data->top_v;
            mapped_index += 64;
        }
        vkUnmapMemory(_vk_device, string_data.vertex_buffer.vk_memory);

        // TODO: This is only needed if memory is not host coherent.
        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.memory = string_data.vertex_buffer.vk_memory;
        memoryRange.size = VK_WHOLE_SIZE;
        memoryRange.offset = 0;
        vkFlushMappedMemoryRanges(_vk_device, 1, &memoryRange);
        return true;
    }

    error_message = "UpdateStringText - Attempting to update non-existant font string_index ";
    error_message += std::to_string(string_index);
    logger.LogError(error_message);
    return false;
}

void GlobeFont::RemoveString(int32_t string_index) {
    if (string_index >= 0 && string_index <= _string_data.size()) {
        if (VK_NULL_HANDLE != _string_data[string_index].index_buffer.vk_buffer) {
            vkDestroyBuffer(_vk_device, _string_data[string_index].index_buffer.vk_buffer, nullptr);
            _string_data[string_index].index_buffer.vk_buffer = VK_NULL_HANDLE;
        }
        _globe_resource_mgr->FreeDeviceMemory(_string_data[string_index].index_buffer.vk_memory);
        if (VK_NULL_HANDLE != _string_data[string_index].vertex_buffer.vk_buffer) {
            vkDestroyBuffer(_vk_device, _string_data[string_index].vertex_buffer.vk_buffer, nullptr);
            _string_data[string_index].vertex_buffer.vk_buffer = VK_NULL_HANDLE;
        }
        _globe_resource_mgr->FreeDeviceMemory(_string_data[string_index].vertex_buffer.vk_memory);
        _string_data.erase(_string_data.begin() + string_index);
    }
}

void GlobeFont::RemoveAllStrings() {
    while (_string_data.size()) {
        RemoveString(0);
    }
}

void GlobeFont::DrawString(VkCommandBuffer command_buffer, glm::mat4 mvp, uint32_t string_index) {
    if (string_index >= _string_data.size()) {
        return;
    }

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline_layout, 0, 1,
                            &_vk_descriptor_set, 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk_pipeline);
    const VkDeviceSize vert_buffer_offset = 0;
    vkCmdPushConstants(command_buffer, _vk_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &mvp);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_string_data[string_index].vertex_buffer.vk_buffer,
                           &vert_buffer_offset);
    vkCmdBindIndexBuffer(command_buffer, _string_data[string_index].index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer, _string_data[string_index].num_indices, 1, 0, 0, 1);
}

void GlobeFont::DrawStrings(VkCommandBuffer command_buffer, glm::mat4 mvp) {
    for (uint32_t string_index = 0; string_index < _string_data.size(); ++string_index) {
        DrawString(command_buffer, mvp, string_index);
    }
}
