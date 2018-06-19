/*
 * LunarGravity - gravity_event.hpp
 *
 * Copyright (C) 2018 LunarG, Inc.
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
 * Author: Mark Young <marky@lunarg.com>
 */

#pragma once

#include <iostream>
#include <cstdlib>
#include <mutex>
#include <cstring>
#include <vector>

enum GravityEventType {
    GRAVITY_EVENT_NONE = 0,
    GRAVITY_EVENT_QUIT,
    GRAVITY_EVENT_PLATFORM_PAUSE_START,
    GRAVITY_EVENT_PLATFORM_PAUSE_STOP,
    GRAVITY_EVENT_WINDOW_DRAW,
    GRAVITY_EVENT_WINDOW_RESIZE,
    GRAVITY_EVENT_WINDOW_CLOSE,
    GRAVITY_EVENT_KEY_PRESS,
    GRAVITY_EVENT_KEY_RELEASE,
    // First app defined event ID, all before this number
    // are reserved for the Gravity framework.
    GRAVITY_EVENT_FIRST_OPEN = 0x00001000,
};

enum GravityKeyName {
    GRAVITY_KEYNAME_A = 0,
    GRAVITY_KEYNAME_B,
    GRAVITY_KEYNAME_C,
    GRAVITY_KEYNAME_D,
    GRAVITY_KEYNAME_E,
    GRAVITY_KEYNAME_F,
    GRAVITY_KEYNAME_G,
    GRAVITY_KEYNAME_H,
    GRAVITY_KEYNAME_I,
    GRAVITY_KEYNAME_J,
    GRAVITY_KEYNAME_K,
    GRAVITY_KEYNAME_L,
    GRAVITY_KEYNAME_M,
    GRAVITY_KEYNAME_N,
    GRAVITY_KEYNAME_O,
    GRAVITY_KEYNAME_P,
    GRAVITY_KEYNAME_Q,
    GRAVITY_KEYNAME_R,
    GRAVITY_KEYNAME_S,
    GRAVITY_KEYNAME_T,
    GRAVITY_KEYNAME_U,
    GRAVITY_KEYNAME_V,
    GRAVITY_KEYNAME_W,
    GRAVITY_KEYNAME_X,
    GRAVITY_KEYNAME_Y,
    GRAVITY_KEYNAME_Z,
    GRAVITY_KEYNAME_SPACE,
    GRAVITY_KEYNAME_0,
    GRAVITY_KEYNAME_1,
    GRAVITY_KEYNAME_2,
    GRAVITY_KEYNAME_3,
    GRAVITY_KEYNAME_4,
    GRAVITY_KEYNAME_5,
    GRAVITY_KEYNAME_6,
    GRAVITY_KEYNAME_7,
    GRAVITY_KEYNAME_8,
    GRAVITY_KEYNAME_9,
    GRAVITY_KEYNAME_TILDA,
    GRAVITY_KEYNAME_DASH,
    GRAVITY_KEYNAME_PLUS,
    GRAVITY_KEYNAME_LEFT_BRACKET,
    GRAVITY_KEYNAME_RIGHT_BRACKET,
    GRAVITY_KEYNAME_COLON,
    GRAVITY_KEYNAME_QUOTE,
    GRAVITY_KEYNAME_COMMA,
    GRAVITY_KEYNAME_PERIOD,
    GRAVITY_KEYNAME_FORWARD_SLASH,
    GRAVITY_KEYNAME_BACKSLASH,
    GRAVITY_KEYNAME_TAB,
    GRAVITY_KEYNAME_BACKSPACE,
    GRAVITY_KEYNAME_LEFT_CTRL,
    GRAVITY_KEYNAME_RIGHT_CTRL,
    GRAVITY_KEYNAME_LEFT_SHIFT,
    GRAVITY_KEYNAME_RIGHT_SHIFT,
    GRAVITY_KEYNAME_LEFT_ALT,
    GRAVITY_KEYNAME_RIGHT_ALT,
    GRAVITY_KEYNAME_PAGE_UP,
    GRAVITY_KEYNAME_PAGE_DOWN,
    GRAVITY_KEYNAME_HOME,
    GRAVITY_KEYNAME_END,
    GRAVITY_KEYNAME_ARROW_UP,
    GRAVITY_KEYNAME_ARROW_DOWN,
    GRAVITY_KEYNAME_ARROW_LEFT,
    GRAVITY_KEYNAME_ARROW_RIGHT,
    GRAVITY_KEYNAME_ESCAPE,
    GRAVITY_KEYNAME_F1,
    GRAVITY_KEYNAME_F2,
    GRAVITY_KEYNAME_F3,
    GRAVITY_KEYNAME_F4,
    GRAVITY_KEYNAME_F5,
    GRAVITY_KEYNAME_F6,
    GRAVITY_KEYNAME_F7,
    GRAVITY_KEYNAME_F8,
    GRAVITY_KEYNAME_F9,
    GRAVITY_KEYNAME_F10,
    GRAVITY_KEYNAME_F11,
    GRAVITY_KEYNAME_F12
};

struct GravityResizeEventData {
    uint16_t width;
    uint16_t height;
};

union GravityEventData {
    GravityResizeEventData resize;
    GravityKeyName key;
    // Provide a pointer for the application to create/release it's
    // own data per event
    void *generic;
};

class GravityEvent {
   public:
    GravityEvent() {
        _type = GRAVITY_EVENT_NONE;
        memset(&_data, 0, sizeof(GravityEventData));
    }
    GravityEvent(GravityEventType type) { _type = type; }

    GravityEventType Type() { return _type; }
    GravityEventData _data;

   private:
    GravityEventType _type;
};

class GravityEventList {
   public:
    static GravityEventList &getInstance() {
        static GravityEventList instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    GravityEventList(GravityEventList const &) = delete;
    void operator=(GravityEventList const &) = delete;

    bool Alloc(uint16_t size);
    bool SpaceAvailable();
    bool HasEvents();
    bool InsertEvent(GravityEvent &event);
    bool GetEvents(std::vector<GravityEvent> &current_events);

   private:
    GravityEventList();
    ~GravityEventList();

    std::vector<GravityEvent> _list;
    uint16_t _current;
    uint16_t _next;
    std::mutex _mutex;
};
