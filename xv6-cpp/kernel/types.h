#pragma once

#include <stddef.h>
#include <stdint.h>
#include <utility>

// if user is using visual studio as ide, make intellisense happier
#if defined(_MSC_VER) && defined(__INTELLISENSE__)
#define no_unique_address msvc::no_unique_address
#define __atomic_exchange_n(...) 1
#define __atomic_load_n(...) 1
#define __atomic_store_n(...) ((void)1)
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

} // namespace utils

#define defer ::utils::defer_guard_impl XV6_UTILS_CONCAT(_defer_guard_, __COUNTER__) = [&]()
