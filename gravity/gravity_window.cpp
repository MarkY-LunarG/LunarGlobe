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

#include <cstring>

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-server-protocol.h>
#endif

#include "gravity_logger.hpp"
#include "gravity_event.hpp"
#include "gravity_window.hpp"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

// Necessary Forward declarations
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)

static void registry_handle_global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    GravityWindow *window = reinterpret_cast<GravityWindow *>(data);
    window->HandleGlobalRegistration(data, registry, id, interface, version);
}
static void registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}
static const wl_registry_listener registry_listener = {registry_handle_global, registry_handle_global_remove};

#endif

GravityWindow::GravityWindow(GravityApp *app, const std::string &name)
    : _associated_app(app), _name(name), _is_fullscreen(false), _vk_surface(VK_NULL_HANDLE) {
#ifdef VK_USE_PLATFORM_XCB_KHR
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    const char *display_envar = getenv("DISPLAY");
    if (display_envar == NULL || display_envar[0] == '\0') {
        GravityLogger::getInstance().LogFatalError("Environment variable DISPLAY requires a valid value.\nExiting ...");
        exit(1);
    }

    _native_win_info.connection = xcb_connect(NULL, &scr);
    if (xcb_connection_has_error(_native_win_info.connection) > 0) {
        GravityLogger::getInstance().LogFatalError("Cannot find XCB Connection!\nExiting ...");
        exit(1);
    }

    setup = xcb_get_setup(_native_win_info.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) {
        xcb_screen_next(&iter);
    }

    _native_win_info.screen = iter.data;

    xcb_flush(_native_win_info.connection);

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    _native_win_info.display = wl_display_connect(NULL);

    if (_native_win_info.display == NULL) {
        GravityLogger::getInstance().LogFatalError("Cannot find Wayland connection.\nExiting ...");
        exit(1);
    }

    _native_win_info.registry = wl_display_get_registry(_native_win_info.display);
    wl_registry_add_listener(_native_win_info.registry, &registry_listener, this);
    wl_display_dispatch(_native_win_info.display);
#endif
}

GravityWindow::~GravityWindow() {
    DestroyVkSurface(_native_win_info.vk_instance, _vk_surface);

#if defined(VK_USE_PLATFORM_XLIB_KHR)
    XDestroyWindow(_native_win_info.display, _native_win_info.xlib_window);
    XCloseDisplay(_native_win_info.display);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    xcb_destroy_window(_native_win_info.connection, _native_win_info.xcb_window);
    xcb_disconnect(_native_win_info.connection);
    free(_native_win_info.atom_wm_delete_window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    wl_keyboard_destroy(_native_win_info.keyboard);
    wl_pointer_destroy(_native_win_info.pointer);
    wl_seat_destroy(_native_win_info.seat);
    wl_shell_surface_destroy(_native_win_info.shell_surface);
    wl_surface_destroy(_native_win_info.window);
    wl_shell_destroy(_native_win_info.shell);
    wl_compositor_destroy(_native_win_info.compositor);
    wl_registry_destroy(_native_win_info.registry);
    wl_display_disconnect(_native_win_info.display);
#endif
}

bool GravityWindow::PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                               void **next) {
    GravityLogger &logger = GravityLogger::getInstance();
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
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        if (!strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        if (!strcmp(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
        if (!strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (!strcmp(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
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
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_IOS_MVK)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_MVK_IOS_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_MVK_MACOS_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_DISPLAY_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_XLIB_SURFACE_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#endif
    }

    return found_surface_ext && found_platform_surface_ext;
}

bool GravityWindow::ReleaseCreateInstanceItems(void **next) { return true; }

bool GravityWindow::CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device,
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
    result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    for (uint32_t i = 0; i < extension_count; i++) {
    }

    return true;
}

bool GravityWindow::CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) {
    GravityLogger &logger = GravityLogger::getInstance();

    if (_vk_surface != VK_NULL_HANDLE) {
        logger.LogInfo("GravityWindow::CreateVkSurface but surface already created.  Using existing one.");
        surface = _vk_surface;
        return true;
    }

// Create a WSI surface for the window:
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = _native_win_info.instance_handle;
    createInfo.hwnd = _native_win_info.window_handle;

    PFN_vkCreateWin32SurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateWin32SurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VkWaylandSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.display = _native_win_info.display;
    createInfo.surface = _native_win_info.window;

    PFN_vkCreateWaylandSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateWaylandSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkAndroidSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.window = (struct ANativeWindow *)(_native_win_info.window);

    PFN_vkCreateAndroidSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateAndroidSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.dpy = _native_win_info.display;
    createInfo.window = _native_win_info.xlib_window;

    PFN_vkCreateXlibSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateXlibSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = _native_win_info.connection;
    createInfo.window = _native_win_info.xcb_window;

    PFN_vkCreateXcbSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateXcbSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_IOS_MVK)
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

bool GravityWindow::DestroyVkSurface(VkInstance instance, VkSurfaceKHR &surface) {
    PFN_vkDestroySurfaceKHR fpDestroySurface =
        reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR"));
    if (nullptr == fpDestroySurface) {
        GravityLogger::getInstance().LogError("Failed to destroy VkSurfaceKHR");
        return false;
    }
    fpDestroySurface(instance, _vk_surface, nullptr);
    _vk_surface = VK_NULL_HANDLE;
    return true;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// MS-Windows event handling function:
LRESULT CALLBACK GravityWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GravityEventType gravity_event_type = GRAVITY_EVENT_NONE;
    switch (uMsg) {
        case WM_CLOSE:
            GravityEventList::getInstance().InsertEvent(GravityEvent(GRAVITY_EVENT_QUIT));
            break;
        case WM_PAINT: {
            // The validation callback calls MessageBox which can generate paint
            // events - don't make more Vulkan calls if we got here from the
            // callback
            GravityEventList::getInstance().InsertEvent(GravityEvent(GRAVITY_EVENT_WINDOW_DRAW));
            break;
        }
        case WM_GETMINMAXINFO:  // set window's minimum size
        {
            GravityWindow *gravity_window = reinterpret_cast<GravityWindow *>(lParam);
            const NativeWindowInfo native_win_info = gravity_window->GetNativeWinInfo();
            ((MINMAXINFO *)lParam)->ptMinTrackSize = native_win_info.minsize;
            return 0;
        }
        case WM_KEYDOWN: {
            FILE *fp = fopen("C:\\dev\\vulkan\\mark.txt", "at");
            if (fp) {
                fprintf(fp, "window->keydown (%d)\n", (uint32_t)(lParam)&0xf);
                fclose(fp);
            }
            gravity_event_type = GRAVITY_EVENT_KEY_PRESS;
            break;
        }
        case WM_KEYUP: {
            FILE *fp = fopen("C:\\dev\\vulkan\\mark.txt", "at");
            if (fp) {
                fprintf(fp, "window->keyup (%d)\n", (uint32_t)(lParam)&0xf);
                fclose(fp);
            }
            gravity_event_type = GRAVITY_EVENT_KEY_RELEASE;
            break;
        }
        case WM_SIZE: {
            // Resize the application to the new window size.
            GravityEvent *event = new GravityEvent(GRAVITY_EVENT_WINDOW_RESIZE);
            uint32_t width = lParam & 0xffff;
            uint32_t height = (lParam & 0xffff0000) >> 16;
            event->_data.resize.width = width;
            event->_data.resize.height = height;
            GravityEventList::getInstance().InsertEvent(*event);
            delete event;
            break;
        }
        default:
            break;
    }
    if (gravity_event_type == GRAVITY_EVENT_KEY_RELEASE) {
        switch (wParam) {
            case VK_ESCAPE: {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ESCAPE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_LEFT: {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_LEFT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_RIGHT: {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_RIGHT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_SPACE: {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_SPACE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
        return 0;
    } else {
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }
}

#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR

void GravityWindow::HandleXlibEvent() {
    XEvent xlib_event;
    GravityEventType gravity_event_type = GRAVITY_EVENT_NONE;
    XNextEvent(_native_win_info.display, &xlib_event);
    switch (xlib_event.type) {
        case ClientMessage:
            if ((Atom)xlib_event.xclient.data.l[0] == _native_win_info.xlib_wm_delete_window) {
                GravityEvent event(GRAVITY_EVENT_QUIT);
                GravityEventList::getInstance().InsertEvent(event);
            }
            break;
        case KeyPress:
            gravity_event_type = GRAVITY_EVENT_KEY_PRESS;
            break;
        case KeyRelease:
            gravity_event_type = GRAVITY_EVENT_KEY_RELEASE;
            break;
        case ConfigureNotify: {
            uint32_t xlib_width = static_cast<uint32_t>(xlib_event.xconfigure.width);
            uint32_t xlib_height = static_cast<uint32_t>(xlib_event.xconfigure.height);
            if ((_width != xlib_width) || (_height != xlib_height)) {
                GravityEvent *event = new GravityEvent(GRAVITY_EVENT_WINDOW_RESIZE);
                _width = xlib_width;
                _height = xlib_height;
                event->_data.resize.width = _width;
                event->_data.resize.height = _height;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            break;
        }
        default:
            break;
    }
    if (gravity_event_type == GRAVITY_EVENT_KEY_PRESS || gravity_event_type == GRAVITY_EVENT_KEY_RELEASE) {
        switch (xlib_event.xkey.keycode) {
            case 0x9:  // Escape
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ESCAPE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x71:  // left arrow key
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_LEFT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x72:  // right arrow key
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_RIGHT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x41:  // space bar
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_SPACE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
    }
}

void GravityWindow::HandleAllXlibEvents() {
    while (XPending(_native_win_info.display) > 0) {
        HandleXlibEvent();
    }
}

#endif
#ifdef VK_USE_PLATFORM_XCB_KHR

void GravityWindow::HandleXcbEvent(xcb_generic_event_t *xcb_event) {
    GravityEventType gravity_event_type = GRAVITY_EVENT_NONE;
    int key_id = 0;
    uint8_t event_code = xcb_event->response_type & 0x7f;
    switch (event_code) {
        case XCB_EXPOSE:
            // TODO: Resize window
            break;
        case XCB_CLIENT_MESSAGE:
            if ((*(xcb_client_message_event_t *)xcb_event).data.data32[0] == (*_native_win_info.atom_wm_delete_window).atom) {
                GravityEvent *event = new GravityEvent(GRAVITY_EVENT_QUIT);
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            break;
        case XCB_KEY_PRESS: {
            const xcb_key_press_event_t *key = (const xcb_key_press_event_t *)xcb_event;
            key_id = key->detail;
            gravity_event_type = GRAVITY_EVENT_KEY_PRESS;
        } break;
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)xcb_event;
            key_id = key->detail;
            gravity_event_type = GRAVITY_EVENT_KEY_RELEASE;
        } break;
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)xcb_event;
            if ((_width != cfg->width) || (_height != cfg->height)) {
                GravityEvent *event = new GravityEvent(GRAVITY_EVENT_WINDOW_RESIZE);
                _width = cfg->width;
                _height = cfg->height;
                event->_data.resize.width = _width;
                event->_data.resize.height = _height;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            }
        } break;
        default:
            break;
    }
    if (gravity_event_type == GRAVITY_EVENT_KEY_PRESS || gravity_event_type == GRAVITY_EVENT_KEY_RELEASE) {
        switch (key_id) {
            case 0x9:  // Escape
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ESCAPE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x71:  // left arrow key
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_LEFT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x72:  // right arrow key
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_ARROW_RIGHT;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x41:  // space bar
            {
                GravityEvent *event = new GravityEvent(gravity_event_type);
                event->_data.key = GRAVITY_KEYNAME_SPACE;
                GravityEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
    }
}

void GravityWindow::HandlePausedXcbEvent() {
    xcb_generic_event_t *event = xcb_wait_for_event(_native_win_info.connection);
    HandleXcbEvent(event);
    free(event);
}

void GravityWindow::HandleActiveXcbEvent() {
    xcb_generic_event_t *event = xcb_poll_for_event(_native_win_info.connection);
    HandleXcbEvent(event);
    free(event);
}

void GravityWindow::HandleAllXcbEvents() {
    xcb_generic_event_t *event;
    while (nullptr != (event = xcb_poll_for_event(_native_win_info.connection))) {
        HandleXcbEvent(event);
        free(event);
    }
}

#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR

static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t sx,
                                 wl_fixed_t sy) {}

static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface) {}

static void pointer_handle_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {}

static void pointer_handle_button(void *data, wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button,
                                  uint32_t state) {
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        GravityWindow *window = reinterpret_cast<GravityWindow *>(data);
        window->MoveSurface(serial);
    }
}

static void pointer_handle_axis(void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}

static const wl_pointer_listener pointer_listener = {
    pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button, pointer_handle_axis,
};

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {}

static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface, wl_array *keys) {}

static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface) {}

static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    GravityEventType gravity_event_type = GRAVITY_EVENT_NONE;
    switch (state) {
        case WL_KEYBOARD_KEY_STATE_PRESSED:
            gravity_event_type = GRAVITY_EVENT_KEY_PRESS;
            break;
        case WL_KEYBOARD_KEY_STATE_RELEASED:
            gravity_event_type = GRAVITY_EVENT_KEY_RELEASE;
            break;
        default:
            return;
    }
    switch (key) {
        case KEY_ESC:  // Escape
        {
            GravityEvent *event = new GravityEvent(gravity_event_type);
            event->_data.key = GRAVITY_KEYNAME_ESCAPE;
            GravityEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_LEFT:  // left arrow key
        {
            GravityEvent *event = new GravityEvent(gravity_event_type);
            event->_data.key = GRAVITY_KEYNAME_ARROW_LEFT;
            GravityEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_RIGHT:  // right arrow key
        {
            GravityEvent *event = new GravityEvent(gravity_event_type);
            event->_data.key = GRAVITY_KEYNAME_ARROW_RIGHT;
            GravityEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_SPACE:  // space bar
        {
            GravityEvent *event = new GravityEvent(gravity_event_type);
            event->_data.key = GRAVITY_KEYNAME_SPACE;
            GravityEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
    }
}

static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {}

static const wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap, keyboard_handle_enter, keyboard_handle_leave, keyboard_handle_key, keyboard_handle_modifiers,
};

static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps) {
    // Subscribe to pointer events
    GravityWindow *window = reinterpret_cast<GravityWindow *>(data);
    window->HandleSeatCapabilities(data, seat, static_cast<wl_seat_capability>(caps));
}

static const wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

void GravityWindow::MoveSurface(uint32_t serial) {
    wl_shell_surface_move(_native_win_info.shell_surface, _native_win_info.seat, serial);
}

void GravityWindow::HandleSeatCapabilities(void *data, wl_seat *seat, wl_seat_capability caps) {
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !_native_win_info.pointer) {
        _native_win_info.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(_native_win_info.pointer, &pointer_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && _native_win_info.pointer) {
        wl_pointer_destroy(_native_win_info.pointer);
        _native_win_info.pointer = NULL;
    }
    // Subscribe to keyboard events
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        _native_win_info.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(_native_win_info.keyboard, &keyboard_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(_native_win_info.keyboard);
        _native_win_info.keyboard = NULL;
    }
}

void GravityWindow::HandleGlobalRegistration(void *data, wl_registry *registry, uint32_t id, const char *interface,
                                             uint32_t version) {
    (void)version;
    // pickup wayland objects when they appear
    if (strcmp(interface, "wl_compositor") == 0) {
        _native_win_info.compositor = reinterpret_cast<wl_compositor *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_compositor_interface), 1));
    } else if (strcmp(interface, "wl_shell") == 0) {
        _native_win_info.shell = reinterpret_cast<wl_shell *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_shell_interface), 1));
    } else if (strcmp(interface, "wl_seat") == 0) {
        _native_win_info.seat = reinterpret_cast<wl_seat *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_seat_interface), 1));
        wl_seat_add_listener(_native_win_info.seat, &seat_listener, this);
    }
}

void GravityWindow::HandlePausedWaylandEvent() {
    wl_display_dispatch(_native_win_info.display);  // block and wait for input
}

void GravityWindow::HandleActiveWaylandEvents() {
    wl_display_dispatch_pending(_native_win_info.display);  // don't block
}

static void handle_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
    (void)data;
    wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
    (void)data;
    (void)shell_surface;
    (void)edges;
    (void)width;
    (void)height;
}

static void handle_popup_done(void *data, wl_shell_surface *shell_surface) {
    (void)data;
    (void)shell_surface;
}

static const wl_shell_surface_listener shell_surface_listener = {handle_ping, handle_configure, handle_popup_done};

#endif

bool GravityWindow::CreatePlatformWindow(VkInstance instance, VkPhysicalDevice phys_device, uint32_t width, uint32_t height) {
    GravityLogger &logger = GravityLogger::getInstance();
    _width = width;
    _height = height;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    WNDCLASSEX win_class;

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = GravityWindowProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = _native_win_info.instance_handle;
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = _name.c_str();
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        logger.LogFatalError("Unexpected error trying to start the application!");
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    _native_win_info.window_handle =
        CreateWindowEx(0, _name.c_str(), _name.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU, 100, 100, wr.right - wr.left,
                       wr.bottom - wr.top, NULL, NULL, _native_win_info.instance_handle, this);
    if (!_native_win_info.window_handle) {
        logger.LogFatalError("Cannot create a window in which to draw!");
        exit(1);
    }
    // Window client area size must be at least 1 pixel high, to prevent crash.
    _native_win_info.minsize.x = GetSystemMetrics(SM_CXMINTRACK);
    _native_win_info.minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;

#elif defined(VK_USE_PLATFORM_XCB_KHR)

    uint32_t value_mask, value_list[32];

    _native_win_info.xcb_window = xcb_generate_id(_native_win_info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = _native_win_info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(_native_win_info.connection, XCB_COPY_FROM_PARENT, _native_win_info.xcb_window, _native_win_info.screen->root,
                      0, 0, _width, _height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _native_win_info.screen->root_visual, value_mask,
                      value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(_native_win_info.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(_native_win_info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(_native_win_info.connection, 0, 16, "WM_DELETE_WINDOW");
    _native_win_info.atom_wm_delete_window = xcb_intern_atom_reply(_native_win_info.connection, cookie2, 0);

    xcb_change_property(_native_win_info.connection, XCB_PROP_MODE_REPLACE, _native_win_info.xcb_window, (*reply).atom, 4, 32, 1,
                        &(*_native_win_info.atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(_native_win_info.connection, _native_win_info.xcb_window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(_native_win_info.connection, _native_win_info.xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                         coords);

#elif defined(VK_USE_PLATFORM_XLIB_KHR)

    XInitThreads();
    _native_win_info.display = XOpenDisplay(NULL);
    long visualMask = VisualScreenMask;
    int numberOfVisuals;
    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = DefaultScreen(_native_win_info.display);
    XVisualInfo *visualInfo = XGetVisualInfo(_native_win_info.display, visualMask, &vInfoTemplate, &numberOfVisuals);

    Colormap colormap = XCreateColormap(_native_win_info.display, RootWindow(_native_win_info.display, vInfoTemplate.screen),
                                        visualInfo->visual, AllocNone);

    XSetWindowAttributes windowAttributes = {};
    windowAttributes.colormap = colormap;
    windowAttributes.background_pixel = 0xFFFFFFFF;
    windowAttributes.border_pixel = 0;
    windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

    _native_win_info.xlib_window =
        XCreateWindow(_native_win_info.display, RootWindow(_native_win_info.display, vInfoTemplate.screen), 0, 0, width, height, 0,
                      visualInfo->depth, InputOutput, visualInfo->visual, CWBackPixel | CWBorderPixel | CWEventMask | CWColormap,
                      &windowAttributes);

    XSelectInput(_native_win_info.display, _native_win_info.xlib_window, windowAttributes.event_mask);
    XMapWindow(_native_win_info.display, _native_win_info.xlib_window);
    XFlush(_native_win_info.display);
    _native_win_info.xlib_wm_delete_window = XInternAtom(_native_win_info.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(_native_win_info.display, _native_win_info.xlib_window, &_native_win_info.xlib_wm_delete_window, 1);

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)

    _native_win_info.window = wl_compositor_create_surface(_native_win_info.compositor);
    if (!_native_win_info.window) {
        logger.LogFatalError("Can not create wayland_surface from compositor!");
        exit(1);
    }

    _native_win_info.shell_surface = wl_shell_get_shell_surface(_native_win_info.shell, _native_win_info.window);
    if (!_native_win_info.shell_surface) {
        logger.LogFatalError("Can not get shell_surface from wayland_surface!");
        exit(1);
    }
    wl_shell_surface_add_listener(_native_win_info.shell_surface, &shell_surface_listener, this);
    wl_shell_surface_set_toplevel(_native_win_info.shell_surface);
    wl_shell_surface_set_title(_native_win_info.shell_surface, _name.c_str());

#endif

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

bool GravityWindow::DestroyPlatformWindow() { return true; }
