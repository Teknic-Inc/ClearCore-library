/*
 * Copyright (c) 2026 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
    \file SysTimer.h
    \brief ClearCore timer/stopwatch utility functions
**/

#ifndef __SYSTIMER_H__
#define __SYSTIMER_H__

#include "SysTiming.h"

namespace ClearCore {

/**
    \class SysTimer
    \brief Abstract stopwatch/timer base (Start/Elapsed/Completed) with selectable time base.

    <b>Use this when:</b> you want a stopwatch object (start, delay, completed, elapsed) and you’ll choose the time base via a derived class.

    <b>Pick a derived class:</b>
    - SysTimerMs for millisecond-scale delays
    - SysTimerUs for microsecond-scale delays.
**/
class SysTimer {

protected:
    // Timer state variables
    uint32_t m_timerStart = 0;
    uint32_t m_timerDelay = 0;
    
    /**
        \brief Default constructor
    **/
    SysTimer() {}

    /**
        \brief Construct and specify the timer's delay

        \param[in] delay The delay for the timer in the time base of
        the timer class being instantiated
    **/
    SysTimer(uint32_t delay) {
        m_timerDelay = delay;
    }

public:
    /**
        \brief Set the delay of the timer

        \param[in] delay The delay for the timer in the time base of
        the timer class
    **/
    void SetDelay(uint32_t delay) {
        m_timerDelay = delay;
    }

    /**
        \brief Starts the timer with a delay argument

        \param[in] delay The delay for the timer in the time base of
        the timer class
    **/
    void Start(uint32_t delay) {
        m_timerDelay = delay;
        m_timerStart = Now();
    }

    /**
        \brief Starts the timer with the default or configured delay
    **/
    void Start() {
        m_timerStart = Now();
    }

    /**
        \brief Check to see if the timer has completed

        \note The timer will roll over after UINT32_MAX ticks have passed
        since starting the timer. You should be mindful of this rollover
        when choosing a time base for an application (every ~49.7 days for
        milliseconds, every ~71.5 minutes for microseconds)

        \return True if the timer has completed
    **/
    bool Completed() {
        return (Now() - m_timerStart) >= m_timerDelay;
    }

    /**
        \brief Check how much time has elapsed since the start of the timer

        \note The elapsed time will roll over after UINT32_MAX ticks have passed
        since starting the timer. You should be mindful of this rollover
        when choosing a time base for an application (every ~49.7 days for
        milliseconds, every ~71.5 minutes for microseconds)

        \return The time elapsed in the time base of the timer class
    **/
    uint32_t Elapsed() {
        return Now() - m_timerStart;
    }

private:
    /**
        \brief Check the current time 

        \note If extending this class to use another time base,
        this function should be overridden to return a value which
        increments every tick of the desired time base (i.e. every
        second, millisecond, microsecond, etc.)

        \return The current time in the time base of the timer class
    **/
    virtual uint32_t Now() = 0;

}; // SysTimer

/**
    \class SysTimerMs
    \brief Stopwatch/timer using millisecond time base (recommended for most timeouts).

    <b>Use this when:</b> your delays/timeouts are human-scale (10 ms to minutes+), and you don’t need microsecond resolution.

    <b>Time base:</b> milliseconds.

    <b>Rollover guidance:</b> this timer will rollover every ~49.7 days - safe for long-running timers compared to microseconds (microsecond base rolls faster).
**/
class SysTimerMs : public SysTimer {

public:
    /**
        \copydoc SysTimer::SysTimer()
    **/
    SysTimerMs() : SysTimer() {}

    /**
        \brief Construct and specify the timer's delay

        \param[in] delay The delay for the timer in milliseconds
    **/
    SysTimerMs(uint32_t delay) : SysTimer(delay) {
        m_timerStart = Now();
    }

private:
    /**
        \copydoc SysTimer::Now()

        Returns the current time in milliseconds
    **/
    uint32_t Now() override {
        return Milliseconds();
    }

}; // SysTimerMs

/**
    \class SysTimerUs
    \brief Stopwatch/timer using microsecond time base (short, high-resolution timing).

    <b>Use this when:</b> you need short, tight timing (sub-millisecond delays, pulse measurement windows, quick debounces).

    <b>Time base:</b> microseconds.

    <b>Rollover caution:</b> this timer will rollover every ~71.5 minutes - best for shorter, more precise intervals.
**/
class SysTimerUs : public SysTimer {

public:
    /**
        \copydoc SysTimer::SysTimer()
    **/
    SysTimerUs() : SysTimer() {}

    /**
        \brief Construct and specify the timer's delay

        \param[in] delay The delay for the timer in microseconds
    **/
    SysTimerUs(uint32_t delay) : SysTimer(delay) {
        m_timerStart = Now();
    }

private:
    /**
        \copydoc SysTimer::Now()

        Returns the current time in microseconds
    **/
    uint32_t Now() override {
        return Microseconds();
    }

}; // SysTimerUs

} // ClearCore namespace

#endif //__SYSTIMER_H__
