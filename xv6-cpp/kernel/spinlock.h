#pragma once

#include "types.h"

namespace xv6 {
    class spinlock : private utils::immovable<spinlock> {
    public:
        explicit spinlock(const char* name) : m_name(name) {}
        void acquire();
        void release();
        bool holding() const;
        const char* name() const { return m_name; }

    private:
        enum lock_status : uint32_t {
            unlocked = 0,
            locked   = 1,
        };

        lock_status m_locked = unlocked;                  // Is the lock held?
        uint32_t m_holder    = static_cast<uint32_t>(-1); // The cpu holding the lock.

        // For debugging:
        const char* m_name; // Name of lock.
    };

    class spinlock_guard : private utils::immovable<spinlock_guard> {
    public:
        explicit spinlock_guard(spinlock& l) : m_lock(l) { m_lock.acquire(); }
        ~spinlock_guard() { m_lock.release(); }

    private:
        spinlock& m_lock;
    };
} // namespace xv6

#define with_spinlock(lock)                                                                                                                                    \
    if (::xv6::spinlock_guard XV6_UTILS_CONCAT(_xv6_spinlock_guard_, __COUNTER__){ lock }; false) {                                                            \
    } else
