//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_vulkan_headers.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <vulkan/vulkan_core.h>

#if defined(VK_USE_PLATFORM_MIR_KHR)
#error "Globe does not have code for Mir at this time"
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

struct GlobeBasicVulkanStruct {
    VkStructureType sType;
    void *pNext;
};
