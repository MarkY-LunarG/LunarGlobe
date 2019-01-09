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

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)

#include "globe_window.hpp"

class GlobeWindowLinux : public GlobeWindow {
   public:
    GlobeWindowLinux(GlobeApp *associated_app, const std::string &name, bool start_fullscreen);
    virtual ~GlobeWindowLinux();

    virtual bool CreatePlatformWindow(VkInstance, VkPhysicalDevice phys_device, uint32_t width,
                                      uint32_t height) override;
    virtual bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                            void **next) override;
    virtual bool CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) override;

#if defined(VK_USE_PLATFORM_XCB_KHR)
    void HandlePausedXcbEvent();
    void HandleActiveXcbEvent();
    void HandleAllXcbEvents();
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    void HandleXlibEvent();
    void HandleAllXlibEvents();
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    void MoveSurface(uint32_t serial);
    void HandleSeatCapabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps);
    void HandleGlobalRegistration(void *data, struct wl_registry *registry, uint32_t id, const char *interface,
                                  uint32_t version);
    void HandlePausedWaylandEvent();
    void HandleActiveWaylandEvents();
#endif

   private:
#ifdef VK_USE_PLATFORM_XCB_KHR
    void HandleXcbEvent(xcb_generic_event_t *xcb_event);

    Display *_display;
    xcb_connection_t *_connection;
    xcb_screen_t *_screen;
    xcb_window_t _xcb_window;
    xcb_intern_atom_reply_t *_atom_wm_delete_window;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    Display *_display;
    Window _xlib_window;
    Atom _xlib_wm_delete_window;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    struct wl_display *_display;
    struct wl_registry *_registry;
    struct wl_compositor *_compositor;
    struct wl_surface *_window;
    struct wl_shell *_shell;
    struct wl_shell_surface *_shell_surface;
    struct wl_seat *_seat;
    struct wl_pointer *_pointer;
    struct wl_keyboard *_keyboard;
#endif
};

#endif  // defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
