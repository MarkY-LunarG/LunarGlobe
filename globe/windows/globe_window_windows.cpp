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

#if defined(VK_USE_PLATFORM_WIN32_KHR)

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_window_windows.hpp"

#include "shellscalingapi.h"

GlobeWindowWindows::GlobeWindowWindows(GlobeApp *app, const std::string &name) : GlobeWindow(app, name) {}

GlobeWindowWindows::~GlobeWindowWindows() {}

bool GlobeWindowWindows::PrepareCreateInstanceItems(std::vector<std::string> &layers,
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
        if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
    }

    if (!found_platform_surface_ext) {
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
    }

    return found_surface_ext && found_platform_surface_ext;
}

bool GlobeWindowWindows::CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    if (_vk_surface != VK_NULL_HANDLE) {
        logger.LogInfo("GlobeWindowWindows::CreateVkSurface but surface already created.  Using existing one.");
        surface = _vk_surface;
        return true;
    }

    // Create a WSI surface for the window:
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = _instance_handle;
    createInfo.hwnd = _window_handle;

    PFN_vkCreateWin32SurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateWin32SurfaceKHR");
        return false;
    }

    return true;
}

// MS-Windows event handling function:
static LRESULT CALLBACK GlobeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GlobeEventType globe_event_type = GLOBE_EVENT_NONE;
    switch (uMsg) {
        case WM_CREATE: {
            CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
            GlobeWindowWindows *globe_window = reinterpret_cast<GlobeWindowWindows *>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)globe_window);
        } break;
        case WM_CLOSE:
            GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_QUIT));
            break;
        case WM_PAINT: {
            // The validation callback calls MessageBox which can generate paint
            // events - don't make more Vulkan calls if we got here from the
            // callback
            GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_WINDOW_DRAW));
            break;
        }
        case WM_GETMINMAXINFO:  // set window's minimum size
        {
            GlobeWindowWindows *globe_window = reinterpret_cast<GlobeWindowWindows *>(GetWindowLongPtr(hWnd, 0));
            if (nullptr != globe_window) {
                ((MINMAXINFO *)lParam)->ptMinTrackSize = globe_window->Minsize();
            }
        }
            return 0;
        case WM_KEYDOWN: {
            globe_event_type = GLOBE_EVENT_KEY_PRESS;
            break;
        }
        case WM_KEYUP: {
            globe_event_type = GLOBE_EVENT_KEY_RELEASE;
            break;
        }
        case WM_SIZE: {
            // Resize the application to the new window size.
            GlobeEvent *event = new GlobeEvent(GLOBE_EVENT_WINDOW_RESIZE);
            uint32_t width = lParam & 0xffff;
            uint32_t height = (lParam & 0xffff0000) >> 16;
            event->_data.resize.width = width;
            event->_data.resize.height = height;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
            break;
        }
        default:
            break;
    }
    if (globe_event_type == GLOBE_EVENT_KEY_RELEASE) {
        switch (wParam) {
            case VK_ESCAPE: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ESCAPE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_LEFT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_LEFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_RIGHT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_RIGHT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_SPACE: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_SPACE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
        }
        return 0;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

bool GlobeWindowWindows::CreatePlatformWindow(VkInstance instance, VkPhysicalDevice phys_device, uint32_t width,
                                              uint32_t height) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    _width = width;
    _height = height;
    _vk_instance = instance;
    _vk_physical_device = phys_device;

    int windows_error = GetLastError();
    DWORD windows_extended_style = {};
    DWORD windows_style = {};
    WNDCLASSEX win_class_ex = {};
    if (NULL == _instance_handle) {
        _instance_handle = GetModuleHandle(NULL);
    }

    // Initialize the window class structure:
    win_class_ex.cbSize = sizeof(WNDCLASSEX);
    win_class_ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    win_class_ex.lpfnWndProc = GlobeWindowProc;
    win_class_ex.cbClsExtra = 0;
    win_class_ex.cbWndExtra = 0;
    win_class_ex.hInstance = _instance_handle;
    win_class_ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class_ex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    win_class_ex.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class_ex.hbrBackground = NULL;
    win_class_ex.lpszMenuName = NULL;
    win_class_ex.lpszClassName = _name.c_str();
    // Register window class:
    if (!RegisterClassEx(&win_class_ex)) {
        logger.LogFatalError("Unexpected error trying to start the application!");
        exit(1);
    }
    windows_error = GetLastError();
    if (_is_fullscreen) {
        DEVMODE dev_mode = {};
        dev_mode.dmSize = sizeof(DEVMODE);
        dev_mode.dmPelsWidth = width;
        dev_mode.dmPelsHeight = height;
        dev_mode.dmBitsPerPel = 32;
        dev_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&dev_mode, CDS_FULLSCREEN)) {
            _is_fullscreen = false;
        } else {
            windows_extended_style = WS_EX_APPWINDOW;
            windows_style = WS_POPUP | WS_VISIBLE;
            ShowCursor(FALSE);
        }
    }
    if (!_is_fullscreen) {
        windows_extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        windows_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    }

    // Create window with the registered class:
    RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    if (!AdjustWindowRectEx(&wr, windows_style, FALSE, windows_extended_style)) {
        logger.LogFatalError("Cannot adjust window size!");
        exit(1);
    }
    windows_error = GetLastError();
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_SYSTEM_DPI_AWARE);
    _window_handle =
        CreateWindowEx(windows_extended_style, _name.c_str(), _name.c_str(), windows_style, 0, 0, wr.right - wr.left,
                       wr.bottom - wr.top, NULL, NULL, _instance_handle, this);
    windows_error = GetLastError();
    if (!_window_handle) {
        logger.LogFatalError("Cannot create a window in which to draw!");
        exit(1);
    }
    // Window client area size must be at least 1 pixel high, to prevent crash.
    _minsize.x = GetSystemMetrics(SM_CXMINTRACK);
    _minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;

    _window_created = true;

    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    if (!CreateVkSurface(instance, phys_device, vk_surface)) {
        logger.LogError("Failed to create vulkan surface for window");
        return false;
    }

    logger.LogInfo("Created Platform Window");
    return true;
}

bool GlobeWindowWindows::DestroyPlatformWindow() {
    if (_is_fullscreen) {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(TRUE);
    }
    return GlobeWindow::DestroyPlatformWindow();
}

#endif
