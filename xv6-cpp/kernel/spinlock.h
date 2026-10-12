#pragma once

#include "types.h"

namespace xv6 {
    struct spinlock {
        explicit constexpr spinlock(const char* name) : m_name(name) {}
        void lock();
        void unlock();
        bool holding() const;
        const char* name() const { return m_name; }

    private:
        enum lock_status : uint32_t {
            unlocked = 0,
            locked = 1,
        };

        lock_status m_locked = unlocked; // Is the lock held?
        uint32_t m_holder = -1; // The cpu holding the lock.

        // For debugging:
        const char* m_name;    // Name of lock.
    };
} // namespace xv6
