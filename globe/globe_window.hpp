//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_window.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <vector>
#include <string>
#include <cmath>

#include "globe_vulkan_headers.hpp"

class GlobeApp;

class GlobeWindow {
   public:
    GlobeWindow(GlobeApp *associated_app, const std::string &name, bool start_fullscreen);
    virtual ~GlobeWindow();

    virtual bool CreatePlatformWindow(VkInstance, VkPhysicalDevice phys_device, uint32_t width, uint32_t height) = 0;
    virtual bool DestroyPlatformWindow();
    virtual bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                            void **next) {
        return true;
    }
    virtual bool ReleaseCreateInstanceItems(void **next) { return true; }
    virtual bool CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) = 0;
    virtual bool DestroyVkSurface(VkInstance instance, VkSurfaceKHR &surface);
    VkSurfaceKHR GetVkSurface() { return _vk_surface; }

    bool IsValid() { return _window_created; }
    bool IsFullScreen() { return _is_fullscreen; }
    uint32_t Width() const { return _width; }
    uint32_t Height() const { return _height; }

    bool CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device,
                                          std::vector<std::string> &extensions);

   protected:
    GlobeApp *_associated_app;
    std::string _name;
    uint32_t _width;
    uint32_t _height;
    bool _is_fullscreen;
    bool _window_created;
    VkInstance _vk_instance;
    VkPhysicalDevice _vk_physical_device;
    VkSurfaceKHR _vk_surface;
};
