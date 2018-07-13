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
 * Author: Mark Young, LunarG <marky@lunarg.com>
 */

#include "gettime.h"

#include "gravity_event.hpp"
#include "gravity_logger.hpp"
#include "gravity_submit_manager.hpp"
#include "gravity_app.hpp"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

GravitySubmitManager::GravitySubmitManager(GravityApp *app, GravityWindow *window, VkInstance instance, VkPhysicalDevice phys_device)
{
    _app = app;
    _window = window;
    _vk_instance = instance;
    _vk_physical_device = phys_device;
    _vk_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    _vk_surface = VK_NULL_HANDLE;
    _vk_swapchain = VK_NULL_HANDLE;
    _num_images = 0;
    _cur_image = 0;
    _cur_wait_index = 0;
    _current_width = window->Width();
    _current_height = window->Height();

    _found_google_display_timing_extension = false;
}

GravitySubmitManager::~GravitySubmitManager() {}

bool GravitySubmitManager::PrepareCreateDeviceItems(VkDeviceCreateInfo &device_create_info, std::vector<std::string> &extensions, void **next)
{
    GravityLogger &logger = GravityLogger::getInstance();
    uint32_t extension_count = 0;

    // Determine the number of instance extensions supported
    VkResult result = vkEnumerateDeviceExtensionProperties(_vk_physical_device, nullptr, &extension_count, nullptr);
    if (VK_SUCCESS != result || 0 >= extension_count)
    {
        logger.LogError("Failed to query number of available device extensions");
        return false;
    }

    // Query the available instance extensions
    std::vector<VkExtensionProperties> extension_properties;

    extension_properties.resize(extension_count);
    result = vkEnumerateDeviceExtensionProperties(_vk_physical_device, nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count)
    {
        logger.LogError("Failed to query available device extensions");
        return false;
    }

    bool found_swapchain_extension = false;

    for (uint32_t i = 0; i < extension_count; i++)
    {
        if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extension_properties[i].extensionName))
        {
            found_swapchain_extension = true;
            extensions.push_back(extension_properties[i].extensionName);
        }

        if (!strcmp(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME, extension_properties[i].extensionName))
        {
            _found_google_display_timing_extension = true;
            extensions.push_back(extension_properties[i].extensionName);
        }
    }

    if (!found_swapchain_extension)
    {
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                             " extension.\n\nDo you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
    }

    // Only create the surface the first time
    if (VK_NULL_HANDLE == _vk_surface)
    {
        if (!_window->CreateVkSurface(_vk_instance, _vk_physical_device, _vk_surface))
        {
            logger.LogFatalError("Failed to create vk surface!");
            return false;
        }
    }

    // Find out which queues we need.  If there's a graphics queue that also supports present, then
    // we want that.  Otherwise, we will need one of each.
    std::vector<VkQueueFamilyProperties> queue_family_props;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_vk_physical_device, &queue_family_count, nullptr);
    queue_family_props.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_vk_physical_device, &queue_family_count, queue_family_props.data());

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            vkGetInstanceProcAddr(_vk_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    if (nullptr == fpGetPhysicalDeviceSurfaceSupportKHR)
    {
        logger.LogError("Failed to get vkGetPhysicalDeviceSurfaceSupportKHR function pointer");
        return false;
    }

    // Iterate over each queue to learn whether it supports presenting:
    std::vector<VkBool32> supports_present;
    supports_present.resize(32);
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        fpGetPhysicalDeviceSurfaceSupportKHR(_vk_physical_device, i, _vk_surface, &supports_present[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphics_queue_family_index = UINT32_MAX;
    uint32_t present_queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        if ((queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphics_queue_family_index == UINT32_MAX)
            {
                graphics_queue_family_index = i;
            }

            if (supports_present[i] == VK_TRUE)
            {
                graphics_queue_family_index = i;
                present_queue_family_index = i;
                break;
            }
        }
    }

    if (present_queue_family_index == UINT32_MAX)
    {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < queue_family_count; ++i)
        {
            if (supports_present[i] == VK_TRUE)
            {
                present_queue_family_index = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX)
    {
        logger.LogFatalError("Could not find both graphics and present queues\n");
        return false;
    }

    _graphics_queue_family_index = graphics_queue_family_index;
    _present_queue_family_index = present_queue_family_index;

    _GetPhysicalDeviceSurfaceCapabilities = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        vkGetInstanceProcAddr(_vk_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    _GetPhysicalDeviceSurfacePresentModes = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        vkGetInstanceProcAddr(_vk_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (nullptr == _GetPhysicalDeviceSurfaceCapabilities || nullptr == _GetPhysicalDeviceSurfacePresentModes)
    {
        logger.LogError("Failed to get physical device commands necessary for swapchain creation");
        return false;
    }

    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo *queues = new VkDeviceQueueCreateInfo[2];
    queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[0].pNext = NULL;
    queues[0].queueFamilyIndex = _graphics_queue_family_index;
    queues[0].queueCount = 1;
    queues[0].pQueuePriorities = queue_priorities;
    queues[0].flags = 0;
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = queues;
    if (UsesSeparatePresentQueue())
    {
        queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[1].pNext = NULL;
        queues[1].queueFamilyIndex = _present_queue_family_index;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = queue_priorities;
        queues[1].flags = 0;
        device_create_info.queueCreateInfoCount = 2;
    }

    // We need the swapchain extension, but nothing else.
    return found_swapchain_extension;
}

bool GravitySubmitManager::ReleaseCreateDeviceItems(VkDeviceCreateInfo device_create_info, void **next)
{
    delete[] device_create_info.pQueueCreateInfos;
    return true;
}

bool GravitySubmitManager::SelectBestColorFormatAndSpace(VkFormat prefered_format, VkFormat secondary_format)
{
    GravityLogger &logger = GravityLogger::getInstance();

    // Get the list of VkFormat's that are supported:
    std::vector<VkSurfaceFormatKHR> possible_surface_formats;
    uint32_t num_possible_formats = 0;

    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            vkGetInstanceProcAddr(_vk_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    if (nullptr == fpGetPhysicalDeviceSurfaceFormatsKHR)
    {
        logger.LogError("Failed to get vkGetPhysicalDeviceSurfaceFormatsKHR function pointer");
        return false;
    }
    if (VK_SUCCESS != fpGetPhysicalDeviceSurfaceFormatsKHR(_vk_physical_device, _vk_surface, &num_possible_formats, NULL))
    {
        logger.LogError("Failed to get query number of device surface formats supported");
        return false;
    }
    possible_surface_formats.resize(num_possible_formats);
    if (VK_SUCCESS != fpGetPhysicalDeviceSurfaceFormatsKHR(_vk_physical_device, _vk_surface, &num_possible_formats,
                                                           possible_surface_formats.data()))
    {
        logger.LogError("Failed to get query device surface formats supported");
        return false;
    }
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (num_possible_formats == 1 && possible_surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        _vk_format = prefered_format;
        _vk_color_space = possible_surface_formats[0].colorSpace;
    }
    else
    {
        // Use SRGB if present, otherwise whatever is first
        _vk_format = VK_FORMAT_UNDEFINED;
        for (uint32_t format = 0; format < num_possible_formats; ++format)
        {
            if (prefered_format == possible_surface_formats[format].format ||
                (_vk_format == VK_FORMAT_UNDEFINED && secondary_format == possible_surface_formats[format].format))
            {
                _vk_format = possible_surface_formats[format].format;
                _vk_color_space = possible_surface_formats[0].colorSpace;
            }
        }
    }

    return true;
}

bool GravitySubmitManager::PrepareForSwapchain(VkDevice device, uint8_t num_images, VkPresentModeKHR present_mode,
                                               VkFormat prefered_format, VkFormat secondary_format)
{
    GravityLogger &logger = GravityLogger::getInstance();

    _vk_device = device;

    // If the desired present mode is one that we haven't checked yet, look in the list of present moes
    // and make sure it is present.
    if (_vk_present_mode != present_mode)
    {
        logger.LogInfo("Querying if present mode is available.");
        uint32_t count = 0;
        std::vector<VkPresentModeKHR> present_modes;
        if (VK_SUCCESS != _GetPhysicalDeviceSurfacePresentModes(_vk_physical_device, _vk_surface, &count, nullptr) || count == 0)
        {
            logger.LogError("Failed querying number of surface present modes");
            return false;
        }
        present_modes.resize(count);
        if (VK_SUCCESS != _GetPhysicalDeviceSurfacePresentModes(_vk_physical_device, _vk_surface, &count, present_modes.data()) ||
            count == 0)
        {
            logger.LogError("Failed querying surface present modes");
            return false;
        }

        for (uint32_t pm = 0; pm < count; ++pm)
        {
            if (present_modes[pm] == present_mode)
            {
                _vk_present_mode = present_mode;
                break;
            }
        }
        if (present_mode != _vk_present_mode)
        {
            logger.LogError("Failed querying surface present modes");
            return false;
        }
    }

    _CreateSwapchain = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(_vk_device, "vkCreateSwapchainKHR"));
    _DestroySwapchain = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(_vk_device, "vkDestroySwapchainKHR"));
    _GetSwapchainImages = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(_vk_device, "vkGetSwapchainImagesKHR"));
    _AcquireNextImage = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(_vk_device, "vkAcquireNextImageKHR"));
    _QueuePresent = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(_vk_device, "vkQueuePresentKHR"));
    if (nullptr == _CreateSwapchain || nullptr == _DestroySwapchain || nullptr == _GetSwapchainImages ||
        nullptr == _AcquireNextImage || nullptr == _QueuePresent)
    {
        logger.LogError("Failed accessing swapchain device functions");
        return false;
    }

    if (_found_google_display_timing_extension)
    {
        _GetRefreshCycleDuration = reinterpret_cast<PFN_vkGetRefreshCycleDurationGOOGLE>(
            vkGetDeviceProcAddr(_vk_device, "vkGetRefreshCycleDurationGOOGLE"));
        _GetPastPresentationTiming = reinterpret_cast<PFN_vkGetPastPresentationTimingGOOGLE>(
            vkGetDeviceProcAddr(_vk_device, "vkGetPastPresentationTimingGOOGLE"));
        if (nullptr == _GetRefreshCycleDuration || nullptr == _GetPastPresentationTiming)
        {
            logger.LogError("Failed to get Google display timing commands for swapchain creation");
            return false;
        }
    }

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    if (VK_SUCCESS != _GetPhysicalDeviceSurfaceCapabilities(_vk_physical_device, _vk_surface, &surface_capabilities))
    {
        logger.LogError("Failed to query physical device surface capabilities");
        return false;
    }

    if (num_images < surface_capabilities.minImageCount)
    {
        _num_images = surface_capabilities.minImageCount;
    }
    else if ((surface_capabilities.maxImageCount > 0) && (num_images > surface_capabilities.maxImageCount))
    {
        _num_images = surface_capabilities.maxImageCount;
    }
    else
    {
        _num_images = num_images;
    }

    _pre_transform_flags = {};
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        _pre_transform_flags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        _pre_transform_flags = surface_capabilities.currentTransform;
    }

    if (!SelectBestColorFormatAndSpace(prefered_format, secondary_format))
    {
        return false;
    }

    vkGetDeviceQueue(_vk_device, _graphics_queue_family_index, 0, &_graphics_queue);
    if (!UsesSeparatePresentQueue())
    {
        _present_queue = _graphics_queue;
    }
    else
    {
        vkGetDeviceQueue(_vk_device, _present_queue_family_index, 0, &_present_queue);
    }

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    _vk_fences.resize(_num_images);
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (uint32_t i = 0; i < _num_images; i++)
    {
        if (VK_SUCCESS != vkCreateFence(_vk_device, &fence_create_info, nullptr, &_vk_fences[i]))
        {
            std::string error_msg = "Failed to allocate the ";
            error_msg += std::to_string(i);
            error_msg += " fence for swapchain submit management";
            logger.LogFatalError(error_msg);
            return false;
        }
    }

    return true;
}

bool GravitySubmitManager::CreateSwapchain()
{
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult result = VK_SUCCESS;
    VkSwapchainKHR old_vk_swapchain = _vk_swapchain;
    uint32_t index = 0;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    if (VK_SUCCESS != _GetPhysicalDeviceSurfaceCapabilities(_vk_physical_device, _vk_surface, &surface_capabilities))
    {
        logger.LogError("Failed to query physical device surface capabilities");
        return false;
    }

    // Width/height of surface can be affected by swapchain capabilities of the
    // physical device.  So adjust them if necessary.
    if (surface_capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        _current_width = surface_capabilities.currentExtent.width;
        _current_height = surface_capabilities.currentExtent.height;
    }
    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR composite_alpha_flags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = nullptr;
    swapchain_ci.surface = _vk_surface;
    swapchain_ci.minImageCount = _num_images;
    swapchain_ci.imageFormat = _vk_format;
    swapchain_ci.imageColorSpace = _vk_color_space;
    swapchain_ci.imageExtent.width = _current_width;
    swapchain_ci.imageExtent.height = _current_height;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.preTransform = _pre_transform_flags;
    swapchain_ci.compositeAlpha = composite_alpha_flags;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;
    swapchain_ci.presentMode = _vk_present_mode;
    swapchain_ci.oldSwapchain = old_vk_swapchain;
    swapchain_ci.clipped = true;
    result = _CreateSwapchain(_vk_device, &swapchain_ci, nullptr, &_vk_swapchain);
    if (VK_SUCCESS != result)
    {
        logger.LogFatalError("Failed to create swapchain!");
        return false;
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (old_vk_swapchain != VK_NULL_HANDLE)
    {
        _DestroySwapchain(_vk_device, old_vk_swapchain, nullptr);
    }

    uint32_t actual_image_count = 0;
    if (VK_SUCCESS != _GetSwapchainImages(_vk_device, _vk_swapchain, &actual_image_count, nullptr) || 0 == actual_image_count)
    {
        logger.LogFatalError("Failed getting number of swapchain images");
        return false;
    }
    _num_images = actual_image_count;
    _vk_images.resize(_num_images);
    _vk_image_views.resize(_num_images);
    _vk_framebuffers.resize(_num_images);
    if (VK_SUCCESS != _GetSwapchainImages(_vk_device, _vk_swapchain, &actual_image_count, _vk_images.data()) ||
        _num_images != actual_image_count)
    {
        _num_images = 0;
        logger.LogFatalError("Failed getting number of swapchain images");
        return false;
    }

    _image_acquired_semaphores.resize(_num_images);
    _draw_complete_semaphores.resize(_num_images);
    _image_ownership_semaphores.resize(_num_images);

    for (uint8_t i = 0; i < _num_images; i++)
    {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = nullptr;
        color_image_view.format = _vk_format;
        color_image_view.components = {};
        color_image_view.components.r = VK_COMPONENT_SWIZZLE_R, color_image_view.components.g = VK_COMPONENT_SWIZZLE_G,
        color_image_view.components.b = VK_COMPONENT_SWIZZLE_B, color_image_view.components.a = VK_COMPONENT_SWIZZLE_A,
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.flags = 0;
        color_image_view.image = _vk_images[i];

        if (VK_SUCCESS != vkCreateImageView(_vk_device, &color_image_view, nullptr, &_vk_image_views[i]))
        {
            std::string err_message = "Failed to retrieve swapchain image ";
            err_message += std::to_string(i);
            logger.LogFatalError(err_message);
            return false;
        }

        // Create semaphores to synchronize acquiring presentable buffers before
        // rendering and waiting for drawing to be complete before presenting
        VkSemaphoreCreateInfo semaphore_creat_info = {};
        semaphore_creat_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_creat_info.pNext = nullptr;
        semaphore_creat_info.flags = 0;

        if (VK_SUCCESS != vkCreateSemaphore(_vk_device, &semaphore_creat_info, nullptr, &_image_acquired_semaphores[i]))
        {
            std::string err_message = "Failed to retrieve swapchain image acquire semaphore ";
            err_message += std::to_string(i);
            logger.LogFatalError(err_message);
            return false;
        }
        if (VK_SUCCESS != vkCreateSemaphore(_vk_device, &semaphore_creat_info, nullptr, &_draw_complete_semaphores[i]))
        {
            std::string err_message = "Failed to retrieve swapchain draw complete semaphore ";
            err_message += std::to_string(i);
            logger.LogFatalError(err_message);
            return false;
        }
        if (UsesSeparatePresentQueue())
        {
            if (VK_SUCCESS != vkCreateSemaphore(_vk_device, &semaphore_creat_info, nullptr, &_image_ownership_semaphores[i]))
            {
                std::string err_message = "Failed to retrieve swapchain image ownership semaphore ";
                err_message += std::to_string(i);
                logger.LogFatalError(err_message);
                return false;
            }
        }
    }

    if (_found_google_display_timing_extension)
    {
        VkRefreshCycleDurationGOOGLE refresh_cycle_duration = {};
        if (VK_SUCCESS != _GetRefreshCycleDuration(_vk_device, _vk_swapchain, &refresh_cycle_duration))
        {
            logger.LogFatalError("Failed call to vkGetRefreshCycleDurationGOOGLE");
            return false;
        }
        _refresh_duration = refresh_cycle_duration.refreshDuration;
        _syncd_with_actual_presents = false;
        _target_IPD = _refresh_duration;
        _refresh_duration_multiplier = 1;
        _prev_desired_present_time = 0;
        _next_present_id = 1;
    }

    // Create the command pool to be used by the swapchain manager.
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext = nullptr;
    cmd_pool_create_info.queueFamilyIndex = _graphics_queue_family_index;
    cmd_pool_create_info.flags = 0;
    if (VK_SUCCESS != vkCreateCommandPool(_vk_device, &cmd_pool_create_info, NULL, &_vk_command_pool))
    {
        logger.LogFatalError("Failed to create swapchain command pool");
        return false;
    }

    // Create at least one command buffer per swapchain image so that we can push render data
    // through more efficiently.  Also, create a present swapchain command buffer if we have a
    // separate render and present queue.
    _vk_render_command_buffers.resize(_num_images);
    if (UsesSeparatePresentQueue())
    {
        _vk_present_command_buffers.resize(_num_images);
    }
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = _vk_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    for (index = 0; index < _num_images; ++index)
    {
        if (VK_SUCCESS != vkAllocateCommandBuffers(_vk_device, &command_buffer_allocate_info, &_vk_render_command_buffers[index]))
        {
            std::string error_msg = "Failed to allocate swapchain render command buffer ";
            error_msg += std::to_string(index);
            logger.LogFatalError(error_msg);
            return false;
        }
        if (UsesSeparatePresentQueue())
        {
            if (VK_SUCCESS !=
                vkAllocateCommandBuffers(_vk_device, &command_buffer_allocate_info, &_vk_present_command_buffers[index]))
            {
                std::string error_msg = "Failed to allocate swapchain present command buffer ";
                error_msg += std::to_string(index);
                logger.LogFatalError(error_msg);
                return false;
            }

            // Make sure we setup a pipeline barrier so thta we transition appropriately first.
            VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = nullptr;
            cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            cmd_buf_info.pInheritanceInfo = nullptr;
            if (VK_SUCCESS != vkBeginCommandBuffer(_vk_present_command_buffers[index], &cmd_buf_info))
            {
                std::string error_msg = "Failed to begin present command buffer ";
                error_msg += std::to_string(index);
                logger.LogFatalError(error_msg);
                return false;
            }

            VkImageMemoryBarrier image_ownership_barrier = {};
            image_ownership_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_ownership_barrier.pNext = nullptr;
            image_ownership_barrier.srcAccessMask = 0;
            image_ownership_barrier.dstAccessMask = 0;
            image_ownership_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            image_ownership_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            image_ownership_barrier.srcQueueFamilyIndex = _graphics_queue_family_index;
            image_ownership_barrier.dstQueueFamilyIndex = _present_queue_family_index;
            image_ownership_barrier.image = _vk_images[index];
            image_ownership_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            vkCmdPipelineBarrier(_vk_present_command_buffers[index], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &image_ownership_barrier);
            if (VK_SUCCESS != vkEndCommandBuffer(_vk_present_command_buffers[index]))
            {
                std::string error_msg = "Failed to end present command buffer ";
                error_msg += std::to_string(index);
                logger.LogFatalError(error_msg);
                return false;
            }
        }
    }

    return true;
}

bool GravitySubmitManager::DetachSwapchain()
{
    uint32_t index;

    vkFreeCommandBuffers(_vk_device, _vk_command_pool, static_cast<uint32_t>(_vk_render_command_buffers.size()), _vk_render_command_buffers.data());
    _vk_render_command_buffers.clear();
    if (UsesSeparatePresentQueue())
    {
        vkFreeCommandBuffers(_vk_device, _vk_command_pool, static_cast<uint32_t>(_vk_present_command_buffers.size()), _vk_present_command_buffers.data());
        _vk_present_command_buffers.clear();
    }
    vkDestroyCommandPool(_vk_device, _vk_command_pool, nullptr);

    for (index = 0; index < _vk_image_views.size(); ++index)
    {
        vkDestroyFramebuffer(_vk_device, _vk_framebuffers[index], nullptr);
        vkDestroyImageView(_vk_device, _vk_image_views[index], nullptr);
        vkDestroySemaphore(_vk_device, _image_acquired_semaphores[index], nullptr);
        vkDestroySemaphore(_vk_device, _draw_complete_semaphores[index], nullptr);
        if (UsesSeparatePresentQueue())
        {
            vkDestroySemaphore(_vk_device, _image_ownership_semaphores[index], nullptr);
        }
    }
    _window->DestroyVkSurface(_vk_instance, _vk_surface);
    _vk_images.clear();
    _vk_image_views.clear();
    _image_acquired_semaphores.clear();
    _draw_complete_semaphores.clear();
    _image_ownership_semaphores.clear();
    return true;
}

bool GravitySubmitManager::Resize()
{
    if (!DetachSwapchain())
    {
        return false;
    }

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    if (VK_SUCCESS != _GetPhysicalDeviceSurfaceCapabilities(_vk_physical_device, _vk_surface, &surface_capabilities))
    {
        GravityLogger::getInstance().LogError("Failed to query physical device surface capabilities");
        return false;
    }
    // Width/height of surface can be affected by swapchain capabilities of the
    // physical device.  So adjust them if necessary.
    if (surface_capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        _current_width = surface_capabilities.currentExtent.width;
        _current_height = surface_capabilities.currentExtent.height;
    }
    return true;
}

bool GravitySubmitManager::DestroySwapchain()
{
    // Wait for fences from present operations
    for (uint32_t i = 0; i < _num_images; i++)
    {
        vkWaitForFences(_vk_device, 1, &_vk_fences[i], VK_TRUE, UINT64_MAX);
        vkDestroyFence(_vk_device, _vk_fences[i], nullptr);
    }

    DetachSwapchain();
    _DestroySwapchain(_vk_device, _vk_swapchain, nullptr);
    return true;
}

bool GravitySubmitManager::AcquireNextImageIndex(uint32_t &index)
{
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult result = VK_INCOMPLETE;

    // Ensure no more than FRAME_LAG renderings are outstanding
    vkWaitForFences(_vk_device, 1, &_vk_fences[_cur_wait_index], VK_TRUE, UINT64_MAX);
    vkResetFences(_vk_device, 1, &_vk_fences[_cur_wait_index]);

    do
    {
        // Get the index of the next available swapchain image:
        result = _AcquireNextImage(_vk_device, _vk_swapchain, UINT64_MAX, _image_acquired_semaphores[_cur_wait_index], VK_NULL_HANDLE,
                                   &index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // _swapchain is out of date (e.g. the window was resized) and must be recreated:
            _app->Resize();
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            // _swapchain is not as optimal as it could be, but the platform's
            // presentation engine will still present the image correctly.
            break;
        }
        else if (VK_SUCCESS != result)
        {
            logger.LogFatalError("Failed to acquire next swapchain image.");
            return false;
        }
    } while (result != VK_SUCCESS);
    _cur_image = index;
    return true;
}

static bool ActualTimeLate(uint64_t desired, uint64_t actual, uint64_t rdur)
{
    // The desired time was the earliest time that the present should have
    // occured.  In almost every case, the actual time should be later than the
    // desired time.  We should only consider the actual time "late" if it is
    // after "desired + rdur".
    if (actual <= desired)
    {
        // The actual time was before or equal to the desired time.  This will
        // probably never happen, but in case it does, return false since the
        // present was obviously NOT late.
        return false;
    }
    uint64_t deadline = actual + rdur;
    if (actual > deadline)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#define MILLION 1000000L
#define BILLION 1000000000L
static bool CanPresentEarlier(uint64_t earliest, uint64_t actual, uint64_t margin, uint64_t rdur)
{
    if (earliest < actual)
    {
        // Consider whether this present could have occured earlier.  Make sure
        // that earliest time was at least 2msec earlier than actual time, and
        // that the margin was at least 2msec:
        uint64_t diff = actual - earliest;
        if ((diff >= (2 * MILLION)) && (margin >= (2 * MILLION)))
        {
            // This present could have occured earlier because both: 1) the
            // earliest time was at least 2 msec before actual time, and 2) the
            // margin was at least 2msec.
            return true;
        }
    }
    return false;
}
bool GravitySubmitManager::AdjustPresentTiming()
{
    if (_found_google_display_timing_extension)
    {
        // Look at what happened to previous presents, and make appropriate
        // adjustments in timing:
        uint32_t count = 0;

        if (VK_SUCCESS != _GetPastPresentationTiming(_vk_device, _vk_swapchain, &count, nullptr))
        {
            GravityLogger::getInstance().LogFatalError(
                "AdjustPresentTiming failed determining number of past present timings available.");
            return false;
        }
        if (count)
        {
            std::vector<VkPastPresentationTimingGOOGLE> past_presentations_timings;
            past_presentations_timings.resize(count);
            if (VK_SUCCESS != _GetPastPresentationTiming(_vk_device, _vk_swapchain, &count, past_presentations_timings.data()))
            {
                GravityLogger::getInstance().LogFatalError(
                    "AdjustPresentTiming failed determining past present timings available.");
                return false;
            }

            bool early = false;
            bool late = false;
            bool calibrate_next = false;
            for (uint32_t i = 0; i < count; i++)
            {
                if (!_syncd_with_actual_presents)
                {
                    // This is the first time that we've received an
                    // actualPresentTime for this swapchain.  In order to not
                    // perceive these early frames as "late", we need to sync-up
                    // our future desiredPresentTime's with the
                    // actualPresentTime(s) that we're receiving now.
                    calibrate_next = true;

                    // So that we don't suspect any pending presents as late,
                    // record them all as suspected-late presents:
                    _last_late_id = _next_present_id - 1;
                    _last_early_id = 0;
                    _syncd_with_actual_presents = true;
                    break;
                }
                else if (CanPresentEarlier(past_presentations_timings[i].earliestPresentTime,
                                           past_presentations_timings[i].actualPresentTime,
                                           past_presentations_timings[i].presentMargin, _refresh_duration))
                {
                    // This image could have been presented earlier.  We don't want
                    // to decrease the target_IPD until we've seen early presents
                    // for at least two seconds.
                    if (_last_early_id == past_presentations_timings[i].presentID)
                    {
                        // We've now seen two seconds worth of early presents.
                        // Flag it as such, and reset the counter:
                        early = true;
                        _last_early_id = 0;
                    }
                    else if (_last_early_id == 0)
                    {
                        // This is the first early present we've seen.
                        // Calculate the presentID for two seconds from now.
                        uint64_t lastEarlyTime = past_presentations_timings[i].actualPresentTime + (2 * BILLION);
                        uint32_t howManyPresents =
                            (uint32_t)((lastEarlyTime - past_presentations_timings[i].actualPresentTime) / _target_IPD);
                        _last_early_id = past_presentations_timings[i].presentID + howManyPresents;
                    }
                    else
                    {
                        // We are in the midst of a set of early images,
                        // and so we won't do anything.
                    }
                    late = false;
                    _last_late_id = 0;
                }
                else if (ActualTimeLate(past_presentations_timings[i].desiredPresentTime,
                                        past_presentations_timings[i].actualPresentTime, _refresh_duration))
                {
                    // This image was presented after its desired time.  Since
                    // there's a delay between calling vkQueuePresentKHR and when
                    // we get the timing data, several presents may have been late.
                    // Thus, we need to threat all of the outstanding presents as
                    // being likely late, so that we only increase the target_IPD
                    // once for all of those presents.
                    if ((_last_late_id == 0) || (_last_late_id < past_presentations_timings[i].presentID))
                    {
                        late = true;
                        // Record the last suspected-late present:
                        _last_late_id = _next_present_id - 1;
                    }
                    else
                    {
                        // We are in the midst of a set of likely-late images,
                        // and so we won't do anything.
                    }
                    early = false;
                    _last_early_id = 0;
                }
                else
                {
                    // Since this image was not presented early or late, reset
                    // any sets of early or late presentIDs:
                    early = false;
                    late = false;
                    calibrate_next = true;
                    _last_early_id = 0;
                    _last_late_id = 0;
                }
            }

            if (early)
            {
                // Since we've seen at least two-seconds worth of presnts that
                // could have occured earlier than desired, let's decrease the
                // target_IPD (i.e. increase the frame rate):
                //
                // TODO(ianelliott): Try to calculate a better target_IPD based
                // on the most recently-seen present (this is overly-simplistic).
                _refresh_duration_multiplier--;
                if (_refresh_duration_multiplier == 0)
                {
                    // This should never happen, but in case it does, don't
                    // try to go faster.
                    _refresh_duration_multiplier = 1;
                }
                _target_IPD = _refresh_duration * _refresh_duration_multiplier;
            }
            if (late)
            {
                // Since we found a new instance of a late present, we want to
                // increase the target_IPD (i.e. decrease the frame rate):
                //
                // TODO(ianelliott): Try to calculate a better target_IPD based
                // on the most recently-seen present (this is overly-simplistic).
                _refresh_duration_multiplier++;
                _target_IPD = _refresh_duration * _refresh_duration_multiplier;
            }

            if (calibrate_next)
            {
                int64_t multiple = _next_present_id - past_presentations_timings[count - 1].presentID;
                _prev_desired_present_time = (past_presentations_timings[count - 1].actualPresentTime + (multiple * _target_IPD));
            }
        }
    }
    else
    {
        std::string error_msg = "AdjustPresentTiming() called in swapchain manager, but ";
        error_msg += VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME;
        error_msg += " extension is not present or enabled.  Ignoring using this functionality.";
        GravityLogger::getInstance().LogWarning(error_msg);
    }
    return true;
}

bool GravitySubmitManager::GetCurrentRenderCommandBuffer(VkCommandBuffer &command_buffer)
{
    if (_vk_render_command_buffers.size() <= _cur_image)
    {
        GravityLogger::getInstance().LogFatalError(
            "GetCurrentRenderCommandBuffer() attempting to access swapchain render command buffer that does not exist");
        return false;
    }
    command_buffer = _vk_render_command_buffers[_cur_image];
    return true;
}

bool GravitySubmitManager::GetRenderCommandBuffer(uint32_t index, VkCommandBuffer &command_buffer)
{
    if (_vk_render_command_buffers.size() <= index)
    {
        GravityLogger::getInstance().LogFatalError(
            "GetRenderCommandBuffer() attempting to access swapchain render command buffer that does not exist");
        return false;
    }
    command_buffer = _vk_render_command_buffers[index];
    return true;
}

bool GravitySubmitManager::GetCurrentFramebuffer(VkFramebuffer &framebuffer)
{
    if (_vk_framebuffers.size() <= _cur_image)
    {
        GravityLogger::getInstance().LogFatalError(
            "GetCurrentFramebuffer() attempting to access swapchain framebuffer that does not exist");
        return false;
    }
    framebuffer = _vk_framebuffers[_cur_image];
    return true;
}

bool GravitySubmitManager::GetFramebuffer(uint32_t index, VkFramebuffer &framebuffer)
{
    if (_vk_framebuffers.size() <= index)
    {
        GravityLogger::getInstance().LogFatalError(
            "GetFramebuffer() attempting to access swapchain framebuffer that does not exist");
        return false;
    }
    framebuffer = _vk_framebuffers[index];
    return true;
}

bool GravitySubmitManager::AttachRenderPassAndDepthBuffer(VkRenderPass render_pass, VkImageView depth_image_view)
{
    VkImageView attached_image_views[2];
    attached_image_views[1] = depth_image_view;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;
    fb_info.renderPass = render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attached_image_views;
    fb_info.width = _current_width;
    fb_info.height = _current_height;
    fb_info.layers = 1;
    for (uint8_t i = 0; i < _num_images; i++)
    {
        attached_image_views[0] = _vk_image_views[i];
        if (VK_SUCCESS != vkCreateFramebuffer(_vk_device, &fb_info, NULL, &_vk_framebuffers[i]))
        {
            std::string error_msg = "Failed to create framebuffer for swapchain index ";
            error_msg += std::to_string(i);
            GravityLogger::getInstance().LogFatalError(error_msg);
            return false;
        }
    }
    return true;
}

bool GravitySubmitManager::InsertPresentCommandsToBuffer(VkCommandBuffer command_buffer)
{
    if (UsesSeparatePresentQueue())
    {
        // We have to transfer ownership from the graphics queue family to the
        // present queue family to be able to present.  Note that we don't have
        // to transfer from present queue family back to graphics queue family at
        // the start of the next frame because we don't care about the image's
        // contents at that point.
        VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_memory_barrier.srcQueueFamilyIndex = _graphics_queue_family_index;
        image_memory_barrier.dstQueueFamilyIndex = _present_queue_family_index;
        image_memory_barrier.image = _vk_images[_cur_image];
        image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &image_memory_barrier);
    }
    return true;
}

bool GravitySubmitManager::Submit(std::vector<VkCommandBuffer> command_buffers, VkFence &fence, bool immediately_wait)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers = command_buffers.data();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    if (VK_SUCCESS != vkQueueSubmit(_graphics_queue, 1, &submit_info, fence))
    {
        GravityLogger::getInstance().LogError("GravitySubmitManager::Submit failed to submit to graphics queue");
        return false;
    }

    if (immediately_wait)
    {
        if (VK_SUCCESS != vkWaitForFences(_vk_device, 1, &fence, VK_TRUE, UINT64_MAX))
        {
            GravityLogger::getInstance().LogError(
                "GravitySubmitManager::Submit failed to wait for submitted work on graphics queue to complete");
            return false;
        }
    }
    return true;
}

bool GravitySubmitManager::SubmitAndPresent()
{
    if (_found_google_display_timing_extension)
    {
        // Look at what happened to previous presents, and make appropriate
        // adjustments in timing:
        AdjustPresentTiming();

        // Note: a real application would position its geometry to that it's in
        // the correct location for when the next image is presented.  It might
        // also wait, so that there's less latency between any input and when
        // the next image is rendered/presented.  This demo program is so
        // simple that it doesn't do either of those.
    }

    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_image_acquired_semaphores[_cur_wait_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_draw_complete_semaphores[_cur_wait_index];
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_vk_render_command_buffers[_cur_image];
    if (VK_SUCCESS != vkQueueSubmit(_graphics_queue, 1, &submit_info, _vk_fences[_cur_wait_index]))
    {
        GravityLogger::getInstance().LogFatalError("SubmitAndPresent(): Render vkQueueSubmit failed.");
        return false;
    }

    if (UsesSeparatePresentQueue())
    {
        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signalling the ownership released semaphore when finished
        submit_info.pWaitSemaphores = &_draw_complete_semaphores[_cur_wait_index];
        submit_info.pSignalSemaphores = &_image_ownership_semaphores[_cur_wait_index];
        submit_info.pCommandBuffers = &_vk_present_command_buffers[_cur_image];
        if (VK_SUCCESS != vkQueueSubmit(_present_queue, 1, &submit_info, VK_NULL_HANDLE))
        {
            GravityLogger::getInstance().LogFatalError("SubmitAndPresent(): Present vkQueueSubmit failed.");
            return false;
        }
    }

    VkPresentInfoKHR present_info = {};
    VkPresentTimeGOOGLE present_time = {};
    VkPresentTimesInfoGOOGLE present_times_info = {};

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_vk_swapchain;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &_cur_image;
    if (UsesSeparatePresentQueue())
    {
        present_info.pWaitSemaphores = &_image_ownership_semaphores[_cur_wait_index];
    }
    else
    {
        present_info.pWaitSemaphores = &_draw_complete_semaphores[_cur_wait_index];
    }

    if (_found_google_display_timing_extension)
    {
        if (_prev_desired_present_time == 0)
        {
            // This must be the first present for this swapchain.
            //
            // We don't know where we are relative to the presentation engine's
            // display's refresh cycle.  We also don't know how long rendering
            // takes.  Let's make a grossly-simplified assumption that the
            // desiredPresentTime should be half way between now and
            // now+target_IPD.  We will adjust over time.
            uint64_t cur_time = getTimeInNanoseconds();
            if (cur_time == 0)
            {
                // Since we didn't find out the current time, don't give a
                // desiredPresentTime:
                present_time.desiredPresentTime = 0;
            }
            else
            {
                present_time.desiredPresentTime = cur_time + (_target_IPD >> 1);
            }
        }
        else
        {
            present_time.desiredPresentTime = (_prev_desired_present_time + _target_IPD);
        }
        present_time.presentID = _next_present_id++;
        _prev_desired_present_time = present_time.desiredPresentTime;

        present_times_info.sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE;
        present_times_info.pNext = present_info.pNext;
        present_times_info.swapchainCount = 1;
        present_times_info.pTimes = &present_time;
        present_info.pNext = &present_times_info;
    }
    VkResult result = _QueuePresent(_present_queue, &present_info);
    if (VK_ERROR_OUT_OF_DATE_KHR == result)
    {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        _app->Resize();
    }
    else if (VK_SUBOPTIMAL_KHR == result)
    {
        // swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else if (VK_SUCCESS != result)
    {
        GravityLogger::getInstance().LogFatalError("vkQueuePresentKHR failed.");
        return false;
    }
    ++_cur_wait_index;
    _cur_wait_index = ((_cur_wait_index) % _num_images);
    return true;
}
