#pragma once

#include <stddef.h>
#include <stdint.h>
#include <utility>

// if user is using visual studio as ide, make intellisense happier
#if defined(_MSC_VER) && defined(__INTELLISENSE__)
#define no_unique_address msvc::no_unique_address
#define __attribute__(...)
#endif

#define XV6_UTILS_MACRO_CONCAT_IMPL(x, y) x##y
#define XV6_UTILS_CONCAT(x, y) XV6_UTILS_MACRO_CONCAT_IMPL(x, y)

namespace utils {
    template<typename T>
    struct immovable {
        constexpr immovable()                  = default;
        immovable(const immovable&)            = delete;
        immovable(immovable&&)                 = delete;
        immovable& operator=(const immovable&) = delete;
        immovable& operator=(immovable&&)      = delete;
    };

    // no exceptions = simpler code
    template<typename Func>
    struct defer_guard_impl {
        constexpr defer_guard_impl(Func&& f) : m_func(std::forward<Func>(f)) {}
        constexpr ~defer_guard_impl() { m_func(); }

    private:
        Func m_func;
    };
    
    template<typename Lock>
    class lock_guard {
        lock_guard(Lock& lock) : m_lock(lock) { m_lock.lock(); }
        ~lock_guard() { m_lock.unlock(); }
    private:
        Lock& m_lock;
    };
} // namespace utils

#define defer ::utils::defer_guard_impl XV6_UTILS_CONCAT(_defer_guard_, __COUNTER__) = [&]()

#define with_lock(lock)                                                                                                                                    \
    if (::xv6::lock_guard XV6_UTILS_CONCAT(_xv6_spinlock_guard_, __COUNTER__){ lock }; false) {                                                            \
    } else
