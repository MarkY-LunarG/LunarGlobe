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

#include "gravity_window.hpp"
#include "gravity_submit_manager.hpp"

struct GravityVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

struct GravityInitStruct
{
    std::string app_name;
    std::vector<std::string> command_line_args;
    GravityVersion version;
    uint32_t width;
    uint32_t height;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HINSTANCE windows_instance;
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    void *molten_view;
#endif
    VkPresentModeKHR present_mode;
    uint32_t num_swapchain_buffers;
    VkFormat ideal_swapchain_format;
    VkFormat secondary_swapchain_format;
};

struct GravityDepthBuffer
{
    VkFormat vk_format;
    VkImage vk_image;
    VkMemoryAllocateInfo vk_mem_alloc_info;
    VkDeviceMemory vk_device_memory;
    VkImageView vk_image_view;
};

struct GravityTexture
{
    uint32_t width;
    uint32_t height;
    VkFormat vk_format;
    VkSampler vk_sampler;
    VkImage vk_image;
    VkImageLayout vk_image_layout;
    VkMemoryAllocateInfo vk_mem_alloc_info;
    VkDeviceMemory vk_device_memory;
    VkImageView vk_image_view;
    std::vector<uint8_t> raw_data;
};

class GravityApp
{
  public:
    GravityApp();
    ~GravityApp();

    virtual bool Init(GravityInitStruct &init_struct);
    virtual void Resize();
    virtual bool Run();
    virtual bool Draw();
    virtual void CleanupCommandObjects(bool is_resize);
    virtual void Exit();
    bool Prepared() { return _prepared; }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    void SetAndroidNativeWindow(ANativeWindow *android_native_window) { _android_native_window = android_native_window; }
    void SetFocused(bool focused) { _focused = focused; }
#endif

  protected:
    virtual bool Setup() = 0;
    bool PreSetup();
    bool PostSetup();
    bool ProcessEvents();
    virtual void HandleEvent(GravityEvent& event);
    virtual bool SelectMemoryTypeUsingRequirements(VkMemoryRequirements requirements, VkFlags required_flags, uint32_t &type);
    bool TransitionVkImageLayout(VkCommandBuffer cmd_buf, VkImage image, VkImageAspectFlags aspect_mask, VkImageLayout old_image_layout,
                                      VkImageLayout new_image_layout, VkAccessFlagBits src_access_mask, VkPipelineStageFlags src_stages,
                                      VkPipelineStageFlags dest_stages);

    std::string _name;
    GravityVersion _app_version;
    GravityVersion _engine_version;
    GravityWindow *_gravity_window;
    GravitySubmitManager *_gravity_submit_mgr;
    uint32_t _width;
    uint32_t _height;
    bool _prepared;
    bool _uses_staging_buffer;
    bool _uses_staging_texture;
    bool _was_minimized;
    bool _is_minimized;
    bool _focused;
    bool _is_paused;
    bool _must_exit;
    bool _google_display_timing_enabled;
    uint64_t _current_frame;
    uint32_t _current_buffer;
    bool _exit_on_frame;
    uint64_t _exit_frame;
    VkInstance _vk_instance;
    VkPhysicalDevice _vk_phys_device;
    VkDevice _vk_device;
    VkPresentModeKHR _vk_present_mode;
    uint32_t _swapchain_count;
    std::vector<SwapchainImageResources> _swapchain_resources;
    VkFormat _vk_swapchain_format;
    VkPhysicalDeviceMemoryProperties _memory_properties;
    VkCommandPool _vk_cmd_pool;
    VkCommandBuffer _vk_cmd_buffer;
    GravityDepthBuffer _depth_buffer;
    VkRenderPass _vk_render_pass;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ANativeWindow *_android_native_window;
#endif
};
