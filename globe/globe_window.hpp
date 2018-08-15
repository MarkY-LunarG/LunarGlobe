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

#pragma once

#include <vector>
#include <string>
#include <cmath>

#include "globe_vulkan_headers.hpp"

class GlobeApp;

class GlobeWindow {
   public:
    GlobeWindow(GlobeApp *associated_app, const std::string &name);
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
