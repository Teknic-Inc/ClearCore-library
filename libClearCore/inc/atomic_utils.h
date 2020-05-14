/*
 * atomic_utils.h
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