/*
 * Copyright (c) 2020 Teknic, Inc.
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

/*
 * atomic_utils.h
 *
 * Defines macros to aid in atomic gcc operations
 */

#ifndef __ATOMIC_UTILS_H_
#define __ATOMIC_UTILS_H_

#define atomic_store_n(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_RELEASE)

#define atomic_or_fetch(ptr, val)                                              \
    __atomic_or_fetch(ptr, val, __ATOMIC_ACQ_REL)
#define atomic_fetch_or(ptr, val)                                              \
    __atomic_fetch_or(ptr, val, __ATOMIC_ACQ_REL)

#define atomic_xor_fetch(ptr, val)                                             \
    __atomic_xor_fetch(ptr, val, __ATOMIC_ACQ_REL)

#define atomic_fetch_and(ptr, val)                                             \
 __atomic_fetch_and(ptr, val, __ATOMIC_ACQ_REL)
#define atomic_and_fetch(ptr, val)                                             \
    __atomic_and_fetch(ptr, val, __ATOMIC_ACQ_REL)

#define atomic_fetch_add(ptr, val)                                             \
 __atomic_fetch_add(ptr, val, __ATOMIC_ACQ_REL)
#define atomic_add_fetch(ptr, val)                                             \
    __atomic_add_fetch(ptr, val, __ATOMIC_ACQ_REL)

#define atomic_load(ptr, ret)                                                  \
    __atomic_load(ptr, ret, __ATOMIC_CONSUME)

#define atomic_load_n(ptr) __atomic_load_n(ptr, __ATOMIC_CONSUME)
#define atomic_load_n_relaxed(ptr) __atomic_load_n(ptr, __ATOMIC_RELAXED)

#define atomic_exchange(ptr, val, ret)                                         \
    __atomic_exchange(ptr, val, ret, __ATOMIC_ACQ_REL)
#define atomic_exchange_n(ptr, val)                                            \
    __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL)

#define atomic_test_and_set(ptr) __atomic_test_and_set(ptr, __ATOMIC_ACQUIRE)

#define atomic_clear(ptr) __atomic_clear(ptr, __ATOMIC_RELEASE)

#define atomic_test_and_set_acqrel(ptr) __atomic_test_and_set(ptr, __ATOMIC_ACQ_REL)

#define atomic_clear_seqcst(ptr) __atomic_clear(ptr, __ATOMIC_SEQ_CST)

#endif /* __ATOMICGCC_H_ */