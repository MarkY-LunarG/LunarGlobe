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
 * Author: Mark Young, LunarG <marky@lunarg.com>
 */

#pragma once

#include <vector>
#include <string>

#include "globe_vulkan_headers.hpp"
#include "globe_window.hpp"

typedef struct {
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkDeviceSize vk_allocated_size;
    VkDescriptorSet descriptor_set;
} SwapchainImageResources;

class GlobeApp;

class GlobeSubmitManager {
   public:
    GlobeSubmitManager(GlobeApp *app, GlobeWindow *window, VkInstance instance, VkPhysicalDevice phys_device);
    ~GlobeSubmitManager();

    bool PrepareCreateDeviceItems(VkDeviceCreateInfo &device_create_info, std::vector<std::string> &extensions,
                                  void **next);
    bool ReleaseCreateDeviceItems(VkDeviceCreateInfo device_create_info, void **next);

    uint32_t GetGraphicsQueueIndex() { return _graphics_queue_family_index; }

    bool PrepareForSwapchain(VkDevice device, uint8_t num_images, VkPresentModeKHR present_mode,
                             VkFormat prefered_format, VkFormat secondary_format);
    bool CreateSwapchain();
    bool DetachSwapchain();
    bool DestroySwapchain();
    VkFormat GetSwapchainVkFormat() { return _vk_format; }
    bool AttachRenderPassAndDepthBuffer(VkRenderPass render_pass, VkImageView depth_image_view);
    bool Resize();
    uint32_t CurrentWidth() { return _current_width; }
    uint32_t CurrentHeight() { return _current_height; }

    VkSwapchainKHR GetVkSwapchain() { return _vk_swapchain; }
    uint8_t NumSwapchainImages() { return _num_images; }
    bool GetCurrentRenderCommandBuffer(VkCommandBuffer &command_buffer);
    bool GetRenderCommandBuffer(uint32_t index, VkCommandBuffer &command_buffer);
    bool GetCurrentFramebuffer(VkFramebuffer &framebuffer);
    bool GetFramebuffer(uint32_t index, VkFramebuffer &framebuffer);

    bool AcquireNextImageIndex(uint32_t &index);
    bool AdjustPresentTiming();
    bool InsertPresentCommandsToBuffer(VkCommandBuffer command_buffer);
    bool Submit(VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore,
                VkFence fence, bool immediately_wait);
    bool SubmitAndPresent(VkSemaphore wait_semaphore);

   private:
    GlobeApp *_app;
    GlobeWindow *_window;
    VkInstance _vk_instance;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    VkPresentModeKHR _vk_present_mode;
    VkFormat _vk_format;
    VkColorSpaceKHR _vk_color_space;
    VkSurfaceKHR _vk_surface;
    uint32_t _graphics_queue_family_index;
    VkQueue _graphics_queue;
    uint32_t _present_queue_family_index;
    VkQueue _present_queue;
    VkSurfaceTransformFlagBitsKHR _pre_transform_flags;
    VkSwapchainKHR _vk_swapchain;
    uint32_t _num_images;
    uint32_t _cur_image;
    std::vector<VkImage> _vk_images;
    std::vector<VkImageView> _vk_image_views;
    uint32_t _cur_wait_index;
    std::vector<VkFence> _vk_fences;
    std::vector<VkFramebuffer> _vk_framebuffers;
    VkCommandPool _vk_command_pool;
    std::vector<VkCommandBuffer> _vk_render_command_buffers;
    std::vector<VkCommandBuffer> _vk_present_command_buffers;
    uint32_t _current_width;
    uint32_t _current_height;

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR _GetPhysicalDeviceSurfaceCapabilities;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR _GetPhysicalDeviceSurfacePresentModes;
    PFN_vkCreateSwapchainKHR _CreateSwapchain;
    PFN_vkDestroySwapchainKHR _DestroySwapchain;
    PFN_vkGetSwapchainImagesKHR _GetSwapchainImages;
    PFN_vkAcquireNextImageKHR _AcquireNextImage;
    PFN_vkQueuePresentKHR _QueuePresent;

    bool _found_google_display_timing_extension;
    PFN_vkGetRefreshCycleDurationGOOGLE _GetRefreshCycleDuration;
    PFN_vkGetPastPresentationTimingGOOGLE _GetPastPresentationTiming;
    bool _syncd_with_actual_presents;
    uint64_t _refresh_duration;
    uint64_t _refresh_duration_multiplier;
    uint64_t _target_IPD;  // image present duration (inverse of frame rate)
    uint64_t _prev_desired_present_time;
    uint32_t _next_present_id;
    uint32_t _last_early_id;  // 0 if no early images
    uint32_t _last_late_id;   // 0 if no late images

    std::vector<VkSemaphore> _image_acquired_semaphores;
    std::vector<VkSemaphore> _draw_complete_semaphores;
    std::vector<VkSemaphore> _image_ownership_semaphores;

    bool SelectBestColorFormatAndSpace(VkFormat prefered_format, VkFormat secondary_format);
    bool UsesSeparatePresentQueue() const { return _graphics_queue_family_index != _present_queue_family_index; }
};
