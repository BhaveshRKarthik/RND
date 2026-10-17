#pragma once

#include "types.h"
#include <atomic>

namespace xv6 {
    class spinlock : private utils::immovable<spinlock> {
    public:
        explicit spinlock(const char* name) : m_name(name) {}
        void lock();
        void release();
        bool holding() const;
        const char* name() const { return m_name; }

    private:
        std::atomic_uint32_t m_locked{}; // Is the lock held?

        constexpr static uint32_t no_holder = static_cast<uint32_t>(-1);
        std::atomic_uint32_t m_holder = no_holder; // The cpu holding the lock.

        // For debugging:
        const char* m_name; // Name of lock.
    };

} // namespace xv6
