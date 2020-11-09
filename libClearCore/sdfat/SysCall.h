/**
 * Copyright (c) 2011-2018 Bill Greiman
 * This file is part of the SdFat library for SD memory cards.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef SysCall_h
#define SysCall_h
/**
 * \file
 * \brief SysCall class
 */
#include "SysTiming.h"

//------------------------------------------------------------------------------
#ifndef F
    /** Define macro for strings stored in flash. */
    #define F(str) (str)
#endif  // F
//------------------------------------------------------------------------------
/** \return the time in milliseconds. */
inline uint16_t curTimeMS() {
    return Milliseconds();
}
//------------------------------------------------------------------------------
/**
 * \class SysCall
 * \brief SysCall - Class to wrap system calls.
 */
class SysCall {
public:
    /** Halt execution of this thread. */
    static void halt() {
        while (1) {
            yield();
        }
    }
    /** Yield to other threads. */
    static void yield();
};
inline void SysCall::yield() {}
#endif  // SysCall_h
