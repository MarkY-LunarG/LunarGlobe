/*
 * LunarGlobe - globe_app.hpp
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

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include "linux/globe_window_linux.hpp"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include "windows/globe_window_windows.hpp"
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include "android/globe_window_android.hpp"
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
#include "apple/globe_window_apple.hpp"
#else
#include "globe_window.hpp"
#endif

struct GlobeVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

struct GlobeInitStruct {
    std::string app_name;
    std::vector<std::string> command_line_args;
    GlobeVersion version;
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

struct GlobeDepthBuffer {
    VkFormat vk_format;
    VkImage vk_image;
    VkDeviceMemory vk_device_memory;
    VkDeviceSize vk_allocated_size;
    VkImageView vk_image_view;
};

class GlobeResourceManager;
class GlobeSubmitManager;
class GlobeClock;

class GlobeApp {
   public:
    GlobeApp();
    ~GlobeApp();

    virtual bool Init(GlobeInitStruct &init_struct);
    virtual void Resize();
    virtual bool Run();
    virtual bool Update(float diff_ms) = 0;
    virtual bool Draw();
    void PreCleanup();
    void PostCleanup();
    virtual void Cleanup();
    virtual void CleanupCommandObjects(bool is_resize);
    virtual void Exit();
    bool Prepared() { return _prepared; }
    void GetVkInfo(VkInstance &instance, VkPhysicalDevice &phys_device, VkDevice &device) const {
        instance = _vk_instance, phys_device = _vk_phys_device, device = _vk_device;
    }
    bool UsesStagingBuffer() const { return _uses_staging_buffer; }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    void SetAndroidNativeWindow(ANativeWindow *android_native_window) {
        _android_native_window = android_native_window;
    }
    void SetFocused(bool focused) { _focused = focused; }
#endif

   protected:
    virtual bool Setup() = 0;
    bool PreSetup(VkCommandPool &vk_setup_command_pool, VkCommandBuffer &vk_setup_command_buffer);
    bool PostSetup(VkCommandPool &vk_setup_command_pool, VkCommandBuffer &vk_setup_command_buffer);
    bool ProcessEvents();
    virtual void HandleEvent(GlobeEvent &event);
    bool TransitionVkImageLayout(VkCommandBuffer cmd_buf, VkImage image, VkImageAspectFlags aspect_mask,
                                 VkImageLayout old_image_layout, VkImageLayout new_image_layout,
                                 VkAccessFlagBits src_access_mask, VkPipelineStageFlags src_stages,
                                 VkPipelineStageFlags dest_stages);

    std::string _name;
    GlobeVersion _app_version;
    GlobeVersion _engine_version;
#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    GlobeWindowLinux *_globe_window;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    GlobeWindowWindows *_globe_window;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    GlobeWindowAndroid *_globe_window;
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    GlobeWindowApple *_globe_window;
#else
    GlobeWindow *_globe_window;
#endif
    GlobeResourceManager *_globe_resource_mgr;
    GlobeSubmitManager *_globe_submit_mgr;
    GlobeClock *_globe_clock;
    uint32_t _width;
    uint32_t _height;
    bool _prepared;
    bool _uses_staging_buffer;
    bool _was_minimized;
    bool _is_minimized;
    bool _focused;
    bool _is_paused;
    bool _must_exit;
    bool _start_fullscreen;
    bool _google_display_timing_enabled;
    uint64_t _current_frame;
    uint32_t _current_buffer;
    bool _exit_on_frame;
    uint64_t _exit_frame;
    VkInstance _vk_instance;
    VkPhysicalDevice _vk_phys_device;
    VkPhysicalDeviceFeatures _vk_phys_device_features;
    VkPhysicalDeviceProperties _vk_phys_device_properties;
    VkDevice _vk_device;
    VkPresentModeKHR _vk_present_mode;
    uint32_t _swapchain_count;
    VkFormat _vk_swapchain_format;
    VkCommandPool _vk_setup_command_pool;
    VkCommandBuffer _vk_setup_command_buffer;
    GlobeDepthBuffer _depth_buffer;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ANativeWindow *_android_native_window;
#endif
    std::string _resource_directory;
};
