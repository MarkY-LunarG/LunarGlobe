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

#ifdef VK_USE_PLATFORM_ANDROID_KHR

#error("Unfinished/Unsupported target")

#pragma once

#include "globe_window.hpp"


class GlobeWindow {
   public:
    GlobeWindow(GlobeApp *associated_app, const std::string &name);
    virtual ~GlobeWindow();

    bool CreatePlatformWindow(VkInstance, VkPhysicalDevice phys_device, uint32_t width, uint32_t height);
    bool DestroyPlatformWindow();

    bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                    void **next);
    bool ReleaseCreateInstanceItems(void **next);

    bool CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface);
    bool DestroyVkSurface(VkInstance instance, VkSurfaceKHR &surface);

    void SetAndroidNativeWindow(ANativeWindow *android_native_window) {
        _native_window = android_native_window;
    }

   private:
    struct ANativeWindow *_native_window;
};

#endif // VK_USE_PLATFORM_ANDROID_KHR
