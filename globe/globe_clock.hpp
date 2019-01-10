//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_clock.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <cstdint>

class GlobeClock {
   public:
    static GlobeClock* CreateClock();

    GlobeClock() { m_paused = true; }
    virtual ~GlobeClock() {}

    virtual void Start() = 0;
    virtual void StartGameTime() = 0;
    virtual void GetTimeDiffMS(float &comp_diff, float &game_diff) = 0;
    virtual void SleepMs(uint32_t milliseconds) = 0;
    void PauseGameTime() { m_paused = true; }

   protected:
    bool m_paused;
};
