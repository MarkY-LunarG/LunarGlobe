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

#include <vulkan/vulkan_core.h>

#if defined(VK_USE_PLATFORM_MIR_KHR)
#error "Gravity does not have code for Mir at this time"
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan_android.h>
#endif

#ifdef VK_USE_PLATFORM_IOS_MVK
#include <vulkan/vulkan_ios.h>
#endif

#ifdef VK_USE_PLATFORM_MACOS_MVK
#include <vulkan/vulkan_macos.h>
#endif

#ifdef VK_USE_PLATFORM_VI_NN
#include <vulkan/vulkan_vi.h>
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#include <vulkan/vulkan_wayland.h>
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <vulkan/vulkan_xlib_xrandr.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>
#endif

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#define APP_NAME_STR_LEN 80
#endif  // _WIN32

#include <vulkan/vk_sdk_platform.h>

struct GravityBasicVulkanStruct {
    VkStructureType sType;
    void *pNext;
};
