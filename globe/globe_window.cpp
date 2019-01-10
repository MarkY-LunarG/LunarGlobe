//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_window.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_window.hpp"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

GlobeWindow::GlobeWindow(GlobeApp *app, const std::string &name, bool start_fullscreen)
    : _associated_app(app),
      _name(name),
      _is_fullscreen(start_fullscreen),
      _window_created(false),
      _vk_instance(VK_NULL_HANDLE),
      _vk_surface(VK_NULL_HANDLE) {}

GlobeWindow::~GlobeWindow() { DestroyVkSurface(_vk_instance, _vk_surface); }

bool GlobeWindow::CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device,
                                                   std::vector<std::string> &extensions) {
    uint32_t extension_count = 0;

    // Determine the number of device extensions supported
    VkResult result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    // Query the available instance extensions
    std::vector<VkExtensionProperties> extension_properties;
    extension_properties.resize(extension_count);
    result =
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    for (uint32_t i = 0; i < extension_count; i++) {
    }

    return true;
}

bool GlobeWindow::DestroyVkSurface(VkInstance instance, VkSurfaceKHR &surface) {
    PFN_vkDestroySurfaceKHR fpDestroySurface =
        reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR"));
    if (nullptr == fpDestroySurface) {
        GlobeLogger::getInstance().LogError("Failed to destroy VkSurfaceKHR");
        return false;
    }
    fpDestroySurface(instance, _vk_surface, nullptr);
    _vk_surface = VK_NULL_HANDLE;
    return true;
}

bool GlobeWindow::DestroyPlatformWindow() {
    _window_created = false;
    return true;
}
