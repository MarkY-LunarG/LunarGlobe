//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_clock.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include "linux/globe_clock_linux.hpp"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include "windows/globe_clock_windows.hpp"
#endif

GlobeClock* GlobeClock::CreateClock() {
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    return new GlobeClockLinux();
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    return new GlobeClockWindows();
#endif
    return nullptr;
}
