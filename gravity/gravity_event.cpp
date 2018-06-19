/*
 * LunarGravity - gravity_event.cpp
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

#include "gravity_logger.hpp"
#include "gravity_event.hpp"

GravityEventList::GravityEventList() : _current(0), _next(0) {}

GravityEventList::~GravityEventList() {}

bool GravityEventList::Alloc(uint16_t size) {
    _mutex.lock();
    _list.resize(size);
    _mutex.unlock();
    return true;
}

bool GravityEventList::SpaceAvailable() {
    bool available = false;
    if (_list.size() > 0) {
        if (_current != _next) {
            if (_current != 0) {
                if (_next != (_current - 1)) {
                    available = true;
                }
            } else {
                if (_next != (_list.size() - 1)) {
                    available = true;
                }
            }
        } else {
            available = true;
        }
    }
    return available;
}

bool GravityEventList::HasEvents() {
    bool events = false;
    if (_list.size() > 0 && _current != _next) {
        events = true;
    }

    return events;
}

bool GravityEventList::InsertEvent(GravityEvent &event) {
    bool space = false;
    _mutex.lock();
    space = SpaceAvailable();
    if (space) {
        _list[_next] = event;
        _next = (_next + 1) % _list.size();
        std::string message = "Inserting event ";
        message += std::to_string(event.Type());
        GravityLogger::getInstance().LogWarning(message);
    } else {
        GravityLogger::getInstance().LogError("Out of space for Event!!!");
    }
    _mutex.unlock();
    return space;
}

bool GravityEventList::GetEvents(std::vector<GravityEvent> &current_events) {
    bool removed = false;
    _mutex.lock();
    while (HasEvents()) {
        current_events.push_back(_list[_current]);
        _current = (_current + 1) % _list.size();
        removed = true;
    }
    _mutex.unlock();
    return removed;
}
