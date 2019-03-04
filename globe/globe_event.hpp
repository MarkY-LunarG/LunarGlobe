//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_event.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <iostream>
#include <cstdlib>
#include <mutex>
#include <cstring>
#include <vector>

enum GlobeEventType {
    GLOBE_EVENT_NONE = 0,
    GLOBE_EVENT_QUIT,
    GLOBE_EVENT_PLATFORM_PAUSE_START,
    GLOBE_EVENT_PLATFORM_PAUSE_STOP,
    GLOBE_EVENT_WINDOW_DRAW,
    GLOBE_EVENT_WINDOW_RESIZE,
    GLOBE_EVENT_WINDOW_CLOSE,
    GLOBE_EVENT_KEY_PRESS,
    GLOBE_EVENT_KEY_RELEASE,
    GLOBE_EVENT_MOUSE_PRESS,
    GLOBE_EVENT_MOUSE_RELEASE,
    // First app defined event ID, all before this number
    // are reserved for the Globe framework.
    GLOBE_EVENT_FIRST_OPEN = 0x00001000,
};

enum GlobeMouseButton {
    GLOBE_MOUSEBUTTON_NONE = 0x00,
    GLOBE_MOUSEBUTTON_LEFT = 0x01,
    GLOBE_MOUSEBUTTON_MIDDLE = 0x02,
    GLOBE_MOUSEBUTTON_RIGHT = 0x04,
};

enum GlobeKeyName {
    GLOBE_KEYNAME_A = 0,
    GLOBE_KEYNAME_B,
    GLOBE_KEYNAME_C,
    GLOBE_KEYNAME_D,
    GLOBE_KEYNAME_E,
    GLOBE_KEYNAME_F,
    GLOBE_KEYNAME_G,
    GLOBE_KEYNAME_H,
    GLOBE_KEYNAME_I,
    GLOBE_KEYNAME_J,
    GLOBE_KEYNAME_K,
    GLOBE_KEYNAME_L,
    GLOBE_KEYNAME_M,
    GLOBE_KEYNAME_N,
    GLOBE_KEYNAME_O,
    GLOBE_KEYNAME_P,
    GLOBE_KEYNAME_Q,
    GLOBE_KEYNAME_R,
    GLOBE_KEYNAME_S,
    GLOBE_KEYNAME_T,
    GLOBE_KEYNAME_U,
    GLOBE_KEYNAME_V,
    GLOBE_KEYNAME_W,
    GLOBE_KEYNAME_X,
    GLOBE_KEYNAME_Y,
    GLOBE_KEYNAME_Z,
    GLOBE_KEYNAME_SPACE,
    GLOBE_KEYNAME_0,
    GLOBE_KEYNAME_1,
    GLOBE_KEYNAME_2,
    GLOBE_KEYNAME_3,
    GLOBE_KEYNAME_4,
    GLOBE_KEYNAME_5,
    GLOBE_KEYNAME_6,
    GLOBE_KEYNAME_7,
    GLOBE_KEYNAME_8,
    GLOBE_KEYNAME_9,
    GLOBE_KEYNAME_BACK_QUOTE,
    GLOBE_KEYNAME_DASH,
    GLOBE_KEYNAME_EQUAL,
    GLOBE_KEYNAME_PLUS,
    GLOBE_KEYNAME_LEFT_BRACKET,
    GLOBE_KEYNAME_RIGHT_BRACKET,
    GLOBE_KEYNAME_SEMICOLON,
    GLOBE_KEYNAME_QUOTE,
    GLOBE_KEYNAME_ENTER,
    GLOBE_KEYNAME_COMMA,
    GLOBE_KEYNAME_PERIOD,
    GLOBE_KEYNAME_FORWARD_SLASH,
    GLOBE_KEYNAME_BACKSLASH,
    GLOBE_KEYNAME_TAB,
    GLOBE_KEYNAME_BACKSPACE,
    GLOBE_KEYNAME_LEFT_CTRL,
    GLOBE_KEYNAME_RIGHT_CTRL,
    GLOBE_KEYNAME_LEFT_SHIFT,
    GLOBE_KEYNAME_RIGHT_SHIFT,
    GLOBE_KEYNAME_LEFT_ALT,
    GLOBE_KEYNAME_RIGHT_ALT,
    GLOBE_KEYNAME_LEFT_OS,
    GLOBE_KEYNAME_RIGHT_OS,
    GLOBE_KEYNAME_PAGE_UP,
    GLOBE_KEYNAME_PAGE_DOWN,
    GLOBE_KEYNAME_HOME,
    GLOBE_KEYNAME_END,
    GLOBE_KEYNAME_ARROW_UP,
    GLOBE_KEYNAME_ARROW_DOWN,
    GLOBE_KEYNAME_ARROW_LEFT,
    GLOBE_KEYNAME_ARROW_RIGHT,
    GLOBE_KEYNAME_ESCAPE,
    GLOBE_KEYNAME_INSERT,
    GLOBE_KEYNAME_DELETE,
    GLOBE_KEYNAME_CLEAR,
    GLOBE_KEYNAME_F1,
    GLOBE_KEYNAME_F2,
    GLOBE_KEYNAME_F3,
    GLOBE_KEYNAME_F4,
    GLOBE_KEYNAME_F5,
    GLOBE_KEYNAME_F6,
    GLOBE_KEYNAME_F7,
    GLOBE_KEYNAME_F8,
    GLOBE_KEYNAME_F9,
    GLOBE_KEYNAME_F10,
    GLOBE_KEYNAME_F11,
    GLOBE_KEYNAME_F12
};

struct GlobeResizeEventData {
    uint16_t width;
    uint16_t height;
};

union GlobeEventData {
    GlobeResizeEventData resize;
    GlobeKeyName key;
    uint32_t mouse_button;
    // Provide a pointer for the application to create/release it's
    // own data per event
    void *generic;
};

class GlobeEvent {
   public:
    GlobeEvent() {
        _type = GLOBE_EVENT_NONE;
        memset(&_data, 0, sizeof(GlobeEventData));
    }
    GlobeEvent(GlobeEventType type) {
        _type = type;
        memset(&_data, 0, sizeof(GlobeEventData));
    }

    GlobeEventType Type() { return _type; }
    GlobeEventData _data;

   private:
    GlobeEventType _type;
};

class GlobeEventList {
   public:
    static GlobeEventList &getInstance() {
        static GlobeEventList instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    GlobeEventList(GlobeEventList const &) = delete;
    void operator=(GlobeEventList const &) = delete;

    bool Alloc(uint16_t size);
    bool SpaceAvailable();
    bool HasEvents();
    bool InsertEvent(GlobeEvent &event);
    bool GetEvents(std::vector<GlobeEvent> &current_events);

   private:
    GlobeEventList();
    ~GlobeEventList();

    std::vector<GlobeEvent> _list;
    uint16_t _current;
    uint16_t _next;
    std::mutex _mutex;
};
