#ifndef LIB_SPINLOCK_H
#define LIB_SPINLOCK_H

#include <stdint.h>

typedef struct spinlock
{
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT {.locked = 0}

static inline void spinlock_init(spinlock_t *lock)
{
    lock->locked = 0;
}

#define spinlock_acquire(lock)                               \
    do                                                       \
    {                                                        \
        while (__sync_lock_test_and_set(&(lock)->locked, 1)) \
        {                                                    \
            while ((lock)->locked)                           \
                ;                                            \
        }                                                    \
    } while (0)

#define spinlock_release(lock)                \
    do                                        \
    {                                         \
        __sync_lock_release(&(lock)->locked); \
    } while (0)

#endif // LIB_SPINLOCK_H