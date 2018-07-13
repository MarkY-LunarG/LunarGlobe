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

#include "gravity_vulkan_headers.hpp"

class GravityApp;

struct NativeWindowInfo {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HINSTANCE instance_handle;
    HWND window_handle;
    POINT minsize;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    Display *display;
    Window xlib_window;
    Atom xlib_wm_delete_window;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    Display *display;
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *window;
    struct wl_shell *shell;
    struct wl_shell_surface *shell_surface;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    struct ANativeWindow *window;
#endif
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    void *view;
#endif
    VkInstance vk_instance;
    VkPhysicalDevice vk_physical_device;
};

class GravityWindow {
   public:
    GravityWindow(GravityApp *associated_app, const std::string &name);
    virtual ~GravityWindow();

#ifdef VK_USE_PLATFORM_WIN32_KHR
    void SetHInstance(HINSTANCE win_hinstance) { _native_win_info.instance_handle = win_hinstance; }
    void RedrawOsWindow() { RedrawWindow(_native_win_info.window_handle, NULL, NULL, RDW_INTERNALPAINT); }
#endif

    bool CreatePlatformWindow(VkInstance, VkPhysicalDevice phys_device, uint32_t width, uint32_t height);
    bool DestroyPlatformWindow();

    bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions, void **next);
    bool ReleaseCreateInstanceItems(void **next);

    bool CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device, std::vector<std::string> &extensions);
    bool CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface);
    bool DestroyVkSurface(VkInstance instance, VkSurfaceKHR &surface);

    const NativeWindowInfo &GetNativeWinInfo() const { return _native_win_info; }
    bool IsFullScreen() { return _is_fullscreen; }
    uint32_t Width() const { return _width; }
    uint32_t Height() const { return _height; }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    void SetAndroidNativeWindow(ANativeWindow *android_native_window) { _native_win_info.window = android_native_window; }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    void HandleXlibEvent();
    void HandleAllXlibEvents();
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    void HandlePausedXcbEvent();
    void HandleActiveXcbEvent();
    void HandleAllXcbEvents();
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    void MoveSurface(uint32_t serial);
    void HandleSeatCapabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps);
    void HandleGlobalRegistration(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    void HandlePausedWaylandEvent();
    void HandleActiveWaylandEvents();
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    void SetMoltenVkView(void *view) { _view = view; }
#endif

   private:
#ifdef VK_USE_PLATFORM_XCB_KHR
    void HandleXcbEvent(xcb_generic_event_t *xcb_event);
#endif

    GravityApp *_associated_app;
    std::string _name;
    uint32_t _width;
    uint32_t _height;
    bool _is_fullscreen;
    NativeWindowInfo _native_win_info;
    VkSurfaceKHR _vk_surface;
};
