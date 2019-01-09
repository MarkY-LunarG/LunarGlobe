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

#if defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)

#error("Unsupported/Unfinished target")

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_window_apple.hpp"

GlobeWindowApple::GlobeWindowApple(GlobeApp *app, const std::string &name)
    : GlobeWindow(app, name) {
}

GlobeWindowApple::~GlobeWindowApple() {
}

bool GlobeWindowApple::PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                             void **next) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    uint32_t extension_count = 0;

    // Determine the number of instance extensions supported
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    // Query the available instance extensions
    std::vector<VkExtensionProperties> extension_properties;

    extension_properties.resize(extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    bool found_surface_ext = false;
    bool found_platform_surface_ext = false;

    for (uint32_t i = 0; i < extension_count; i++) {
        if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_surface_ext = true;
            extensions.push_back(extension_properties[i].extensionName);
        }
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        if (!strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_IOS_MVK)
        if (!strcmp(VK_MVK_IOS_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        if (!strcmp(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#endif
    }

    if (!found_platform_surface_ext) {
#if defined(VK_USE_PLATFORM_IOS_MVK)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_MVK_IOS_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_MVK_MACOS_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_DISPLAY_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#endif
    }

    return found_surface_ext && found_platform_surface_ext;
}

bool GlobeWindowApple::CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    if (_vk_surface != VK_NULL_HANDLE) {
        logger.LogInfo("GlobeWindowApple::CreateVkSurface but surface already created.  Using existing one.");
        surface = _vk_surface;
        return true;
    }

// Create a WSI surface for the window:
#if defined(VK_USE_PLATFORM_IOS_MVK)
    VkIOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pView = _native_win_info.view;

    PFN_vkCreateIOSSurfaceMVK fpCreateSurface =
        reinterpret_cast<PFN_vkCreateIOSSurfaceMVK>(vkGetInstanceProcAddr(instance, "vkCreateIOSSurfaceMVK"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateIOSSurfaceMVK");
        return false;
    }
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    VkMacOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pView = _native_win_info.view;

    PFN_vkCreateMacOSSurfaceMVK fpCreateSurface =
        reinterpret_cast<PFN_vkCreateMacOSSurfaceMVK>(vkGetInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateMacOSSurfaceMVK");
        return false;
    }
#endif

    return true;
}

bool GlobeWindow::CreatePlatformWindow(VkInstance instance, VkPhysicalDevice phys_device, uint32_t width,
                                       uint32_t height) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    _width = width;
    _height = height;
    _vk_instance = instance;
    _vk_physical_device = phys_device;

    // TODO:

    _window_created = true;

    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    if (!CreateVkSurface(instance, phys_device, vk_surface)) {
        logger.LogError("Failed to create vulkan surface for window");
        return false;
    }
    _native_win_info.vk_instance = instance;
    _native_win_info.vk_physical_device = phys_device;

    logger.LogInfo("Created Platform Window");
    return true;
}
