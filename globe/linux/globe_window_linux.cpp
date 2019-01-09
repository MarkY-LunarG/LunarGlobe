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

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)

#include <cstring>

#ifdef VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xatom.h>
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-server-protocol.h>
#endif

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_window_linux.hpp"

// Necessary Forward declarations
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)

static void registry_handle_global(void *data, wl_registry *registry, uint32_t id, const char *interface,
                                   uint32_t version) {
    GlobeWindowLinux *window = reinterpret_cast<GlobeWindowLinux *>(data);
    window->HandleGlobalRegistration(data, registry, id, interface, version);
}
static void registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}
static const wl_registry_listener registry_listener = {registry_handle_global, registry_handle_global_remove};

#endif

GlobeWindowLinux::GlobeWindowLinux(GlobeApp *app, const std::string &name, bool start_fullscreen)
    : GlobeWindow(app, name, start_fullscreen) {
#ifdef VK_USE_PLATFORM_XCB_KHR
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    const char *display_envar = getenv("DISPLAY");
    if (display_envar == NULL || display_envar[0] == '\0') {
        GlobeLogger::getInstance().LogFatalError("Environment variable DISPLAY requires a valid value.\nExiting ...");
        exit(1);
    }

    _connection = xcb_connect(NULL, &scr);
    if (xcb_connection_has_error(_connection) > 0) {
        GlobeLogger::getInstance().LogFatalError("Cannot find XCB Connection!\nExiting ...");
        exit(1);
    }

    setup = xcb_get_setup(_connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) {
        xcb_screen_next(&iter);
    }

    _screen = iter.data;

    xcb_flush(_connection);

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    _display = wl_display_connect(NULL);

    if (_display == NULL) {
        GlobeLogger::getInstance().LogFatalError("Cannot find Wayland connection.\nExiting ...");
        exit(1);
    }

    _registry = wl_display_get_registry(_display);
    wl_registry_add_listener(_registry, &registry_listener, this);
    wl_display_dispatch(_display);
#endif
}

GlobeWindowLinux::~GlobeWindowLinux() {
#if defined(VK_USE_PLATFORM_XCB_KHR)
    xcb_destroy_window(_connection, _xcb_window);
    xcb_disconnect(_connection);
    free(_atom_wm_delete_window);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    XDestroyWindow(_display, _xlib_window);
    XCloseDisplay(_display);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    wl_keyboard_destroy(_keyboard);
    wl_pointer_destroy(_pointer);
    wl_seat_destroy(_seat);
    wl_shell_surface_destroy(_shell_surface);
    wl_surface_destroy(_window);
    wl_shell_destroy(_shell);
    wl_compositor_destroy(_compositor);
    wl_registry_destroy(_registry);
    wl_display_disconnect(_display);
#endif
}

bool GlobeWindowLinux::PrepareCreateInstanceItems(std::vector<std::string> &layers,
                                                  std::vector<std::string> &extensions, void **next) {
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
#if defined(VK_USE_PLATFORM_XCB_KHR)
        if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        if (!strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
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
#endif
    }

    if (!found_platform_surface_ext) {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
        logger.LogFatalError("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_DISPLAY_EXTENSION_NAME
                             " extension.\n\n"
                             "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                             "Please look at the Getting Started guide for additional information.");
#endif
    }

    return found_surface_ext && found_platform_surface_ext;
}

bool GlobeWindowLinux::CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    if (_vk_surface != VK_NULL_HANDLE) {
        logger.LogInfo("GlobeWindowLinux::CreateVkSurface but surface already created.  Using existing one.");
        surface = _vk_surface;
        return true;
    }

// Create a WSI surface for the window:
#if defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = _connection;
    createInfo.window = _xcb_window;

    PFN_vkCreateXcbSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateXcbSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.dpy = _display;
    createInfo.window = _xlib_window;

    PFN_vkCreateXlibSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateXlibSurfaceKHR");
        return false;
    }
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VkWaylandSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.display = _display;
    createInfo.surface = _window;

    PFN_vkCreateWaylandSurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateWaylandSurfaceKHR");
        return false;
    }
#endif

    return true;
}

#ifdef VK_USE_PLATFORM_XCB_KHR

void GlobeWindowLinux::HandleXcbEvent(xcb_generic_event_t *xcb_event) {
    GlobeEventType globe_event_type = GLOBE_EVENT_NONE;
    int key_id = 0;
    uint8_t event_code = xcb_event->response_type & 0x7f;
    switch (event_code) {
        case XCB_EXPOSE:
            // TODO: Resize window
            break;
        case XCB_CLIENT_MESSAGE:
            if ((*(xcb_client_message_event_t *)xcb_event).data.data32[0] == (*_atom_wm_delete_window).atom) {
                GlobeEvent *event = new GlobeEvent(GLOBE_EVENT_QUIT);
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            break;
        case XCB_KEY_PRESS: {
            const xcb_key_press_event_t *key = (const xcb_key_press_event_t *)xcb_event;
            key_id = key->detail;
            globe_event_type = GLOBE_EVENT_KEY_PRESS;
        } break;
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)xcb_event;
            key_id = key->detail;
            globe_event_type = GLOBE_EVENT_KEY_RELEASE;
        } break;
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)xcb_event;
            if ((_width != cfg->width) || (_height != cfg->height)) {
                GlobeEvent *event = new GlobeEvent(GLOBE_EVENT_WINDOW_RESIZE);
                _width = cfg->width;
                _height = cfg->height;
                event->_data.resize.width = _width;
                event->_data.resize.height = _height;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
        } break;
        default:
            break;
    }
    if (globe_event_type == GLOBE_EVENT_KEY_PRESS || globe_event_type == GLOBE_EVENT_KEY_RELEASE) {
        switch (key_id) {
            case 0x9:  // Escape
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ESCAPE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x71:  // left arrow key
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_LEFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x72:  // right arrow key
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_RIGHT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x41:  // space bar
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_SPACE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
    }
}

void GlobeWindowLinux::HandlePausedXcbEvent() {
    xcb_generic_event_t *event = xcb_wait_for_event(_connection);
    HandleXcbEvent(event);
    free(event);
}

void GlobeWindowLinux::HandleActiveXcbEvent() {
    xcb_generic_event_t *event = xcb_poll_for_event(_connection);
    HandleXcbEvent(event);
    free(event);
}

void GlobeWindowLinux::HandleAllXcbEvents() {
    xcb_generic_event_t *event;
    while (nullptr != (event = xcb_poll_for_event(_connection))) {
        HandleXcbEvent(event);
        free(event);
    }
}

#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR

void GlobeWindowLinux::HandleXlibEvent() {
    XEvent xlib_event;
    GlobeEventType globe_event_type = GLOBE_EVENT_NONE;
    XNextEvent(_display, &xlib_event);
    switch (xlib_event.type) {
        case ClientMessage:
            if ((Atom)xlib_event.xclient.data.l[0] == _xlib_wm_delete_window) {
                GlobeEvent event(GLOBE_EVENT_QUIT);
                GlobeEventList::getInstance().InsertEvent(event);
            }
            break;
        case KeyPress:
            globe_event_type = GLOBE_EVENT_KEY_PRESS;
            break;
        case KeyRelease:
            globe_event_type = GLOBE_EVENT_KEY_RELEASE;
            break;
        case ConfigureNotify: {
            uint32_t xlib_width = static_cast<uint32_t>(xlib_event.xconfigure.width);
            uint32_t xlib_height = static_cast<uint32_t>(xlib_event.xconfigure.height);
            if ((_width != xlib_width) || (_height != xlib_height)) {
                GlobeEvent *event = new GlobeEvent(GLOBE_EVENT_WINDOW_RESIZE);
                _width = xlib_width;
                _height = xlib_height;
                event->_data.resize.width = _width;
                event->_data.resize.height = _height;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            break;
        }
        default:
            break;
    }
    if (globe_event_type == GLOBE_EVENT_KEY_PRESS || globe_event_type == GLOBE_EVENT_KEY_RELEASE) {
        switch (xlib_event.xkey.keycode) {
            case 0x9:  // Escape
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ESCAPE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x71:  // left arrow key
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_LEFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x72:  // right arrow key
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_RIGHT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x41:  // space bar
            {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_SPACE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
    }
}

void GlobeWindowLinux::HandleAllXlibEvents() {
    while (XPending(_display) > 0) {
        HandleXlibEvent();
    }
}

#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR

static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t sx,
                                 wl_fixed_t sy) {
    GlobeWindowLinux *window = reinterpret_cast<GlobeWindowLinux *>(data);
    if (window->IsFullScreen()) {
        wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
    }
}

static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface) {}

static void pointer_handle_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {}

static void pointer_handle_button(void *data, wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button,
                                  uint32_t state) {
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        GlobeWindowLinux *window = reinterpret_cast<GlobeWindowLinux *>(data);
        window->MoveSurface(serial);
    }
}

static void pointer_handle_axis(void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}

static const wl_pointer_listener pointer_listener = {
    pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button, pointer_handle_axis,
};

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {}

static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface,
                                  wl_array *keys) {}

static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface) {}

static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t state) {
    GlobeEventType globe_event_type = GLOBE_EVENT_NONE;
    switch (state) {
        case WL_KEYBOARD_KEY_STATE_PRESSED:
            globe_event_type = GLOBE_EVENT_KEY_PRESS;
            break;
        case WL_KEYBOARD_KEY_STATE_RELEASED:
            globe_event_type = GLOBE_EVENT_KEY_RELEASE;
            break;
        default:
            return;
    }
    switch (key) {
        case KEY_ESC:  // Escape
        {
            GlobeEvent *event = new GlobeEvent(globe_event_type);
            event->_data.key = GLOBE_KEYNAME_ESCAPE;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_LEFT:  // left arrow key
        {
            GlobeEvent *event = new GlobeEvent(globe_event_type);
            event->_data.key = GLOBE_KEYNAME_ARROW_LEFT;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_RIGHT:  // right arrow key
        {
            GlobeEvent *event = new GlobeEvent(globe_event_type);
            event->_data.key = GLOBE_KEYNAME_ARROW_RIGHT;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
        case KEY_SPACE:  // space bar
        {
            GlobeEvent *event = new GlobeEvent(globe_event_type);
            event->_data.key = GLOBE_KEYNAME_SPACE;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
        } break;
    }
}

static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {}

static const wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
    keyboard_handle_key,    keyboard_handle_modifiers,
};

static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps) {
    // Subscribe to pointer events
    GlobeWindowLinux *window = reinterpret_cast<GlobeWindowLinux *>(data);
    window->HandleSeatCapabilities(data, seat, static_cast<wl_seat_capability>(caps));
}

static const wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

void GlobeWindowLinux::MoveSurface(uint32_t serial) { wl_shell_surface_move(_shell_surface, _seat, serial); }

void GlobeWindowLinux::HandleSeatCapabilities(void *data, wl_seat *seat, wl_seat_capability caps) {
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !_pointer) {
        _pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(_pointer, &pointer_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && _pointer) {
        wl_pointer_destroy(_pointer);
        _pointer = NULL;
    }
    // Subscribe to keyboard events
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        _keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(_keyboard, &keyboard_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(_keyboard);
        _keyboard = NULL;
    }
}

void GlobeWindowLinux::HandleGlobalRegistration(void *data, wl_registry *registry, uint32_t id, const char *interface,
                                                uint32_t version) {
    (void)version;
    // pickup wayland objects when they appear
    if (strcmp(interface, "wl_compositor") == 0) {
        _compositor = reinterpret_cast<wl_compositor *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_compositor_interface), 1));
    } else if (strcmp(interface, "wl_shell") == 0) {
        _shell = reinterpret_cast<wl_shell *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_shell_interface), 1));
    } else if (strcmp(interface, "wl_seat") == 0) {
        _seat = reinterpret_cast<wl_seat *>(
            wl_registry_bind(registry, id, reinterpret_cast<const wl_interface *>(&wl_seat_interface), 1));
        wl_seat_add_listener(_seat, &seat_listener, this);
    }
}

void GlobeWindowLinux::HandlePausedWaylandEvent() {
    wl_display_dispatch(_display);  // block and wait for input
}

void GlobeWindowLinux::HandleActiveWaylandEvents() {
    wl_display_dispatch_pending(_display);  // don't block
}

static void handle_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
    (void)data;
    wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width,
                             int32_t height) {
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

static void configure_callback(void *data, struct wl_callback *callback, uint32_t time) {
    GlobeWindowLinux *window = reinterpret_cast<GlobeWindowLinux *>(data);
    wl_callback_destroy(callback);
}

static struct wl_callback_listener configure_callback_listener = {
    configure_callback,
};

static const wl_shell_surface_listener shell_surface_listener = {handle_ping, handle_configure, handle_popup_done};

#endif

bool GlobeWindowLinux::CreatePlatformWindow(VkInstance instance, VkPhysicalDevice phys_device, uint32_t width,
                                            uint32_t height) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    _width = width;
    _height = height;
    _vk_instance = instance;
    _vk_physical_device = phys_device;

#if defined(VK_USE_PLATFORM_XCB_KHR)

    uint32_t value_mask, value_list[32];

    _xcb_window = xcb_generate_id(_connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = _screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(_connection, XCB_COPY_FROM_PARENT, _xcb_window, _screen->root, 0, 0, _width, _height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, _screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(_connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(_connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(_connection, 0, 16, "WM_DELETE_WINDOW");
    _atom_wm_delete_window = xcb_intern_atom_reply(_connection, cookie2, 0);

    xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _xcb_window, (*reply).atom, XCB_ATOM_ATOM, 32, 1,
                        &(*_atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(_connection, _xcb_window);

    if (_is_fullscreen) {
        xcb_intern_atom_cookie_t wm_state_ck = xcb_intern_atom(_connection, 0, 13, "_NET_WM_STATE");
        reply = xcb_intern_atom_reply(_connection, wm_state_ck, 0);
        xcb_intern_atom_cookie_t wm_state_fs_ck = xcb_intern_atom(_connection, 0, 24, "_NET_WM_STATE_FULLSCREEN");
        xcb_intern_atom_reply_t *reply_fs = xcb_intern_atom_reply(_connection, wm_state_fs_ck, 0);
        xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _xcb_window, (*reply).atom, XCB_ATOM_ATOM, 32, 1,
                            &(*reply_fs).atom);
        free(reply);
        free(reply_fs);
    } else {
        // Force the x/y coordinates to 100,100 results are identical in consecutive
        // runs
        const uint32_t coords[] = {100, 100};
        xcb_configure_window(_connection, _xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    }

#elif defined(VK_USE_PLATFORM_XLIB_KHR)

    XInitThreads();
    _display = XOpenDisplay(NULL);
    long visualMask = VisualScreenMask;
    int numberOfVisuals;
    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = DefaultScreen(_display);
    XVisualInfo *visualInfo = XGetVisualInfo(_display, visualMask, &vInfoTemplate, &numberOfVisuals);

    Colormap colormap =
        XCreateColormap(_display, RootWindow(_display, vInfoTemplate.screen), visualInfo->visual, AllocNone);

    XSetWindowAttributes windowAttributes = {};
    windowAttributes.colormap = colormap;
    windowAttributes.background_pixel = 0xFFFFFFFF;
    windowAttributes.border_pixel = 0;
    windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

    _xlib_window = XCreateWindow(_display, RootWindow(_display, vInfoTemplate.screen), 0, 0, width, height, 0,
                                 visualInfo->depth, InputOutput, visualInfo->visual,
                                 CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);

    if (_is_fullscreen) {
        Atom fullscreen_atom = XInternAtom(_display, "_NET_WM_STATE_FULLSCREEN", True);
        if (fullscreen_atom == None) {
            logger.LogFatalError("Failed retrieving fullscreen atom");
            exit(1);
        }
        XChangeProperty(_display, _xlib_window, XInternAtom(_display, "_NET_WM_STATE", True), XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&fullscreen_atom, 1);

        int compos_win_value = 1;
        XChangeProperty(_display, _xlib_window, XInternAtom(_display, "_HILDON_NON_COMPOSITED_WINDOW", False),
                        XA_INTEGER, 32, PropModeReplace, (unsigned char *)&compos_win_value, 1);
    }

    XSelectInput(_display, _xlib_window, windowAttributes.event_mask);
    XMapWindow(_display, _xlib_window);
    XFlush(_display);
    _xlib_wm_delete_window = XInternAtom(_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(_display, _xlib_window, &_xlib_wm_delete_window, 1);

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)

    _window = wl_compositor_create_surface(_compositor);
    if (!_window) {
        logger.LogFatalError("Can not create wayland_surface from compositor!");
        exit(1);
    }

    _shell_surface = wl_shell_get_shell_surface(_shell, _window);
    if (!_shell_surface) {
        logger.LogFatalError("Can not get shell_surface from wayland_surface!");
        exit(1);
    }
    wl_shell_surface_add_listener(_shell_surface, &shell_surface_listener, this);
    wl_surface_set_user_data(_window, this);
    wl_shell_surface_set_title(_shell_surface, _name.c_str());

    if (_is_fullscreen) {
        // TODO: Not working yet.
        wl_shell_surface_set_fullscreen(_shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, nullptr);
    } else {
        wl_shell_surface_set_toplevel(_shell_surface);
    }

#endif

    _window_created = true;

    if (!CreateVkSurface(instance, phys_device, _vk_surface)) {
        logger.LogError("Failed to create vulkan surface for window");
        return false;
    }
    _vk_instance = instance;
    _vk_physical_device = phys_device;

    logger.LogInfo("Created Platform Window");
    return true;
}

#endif  // defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
