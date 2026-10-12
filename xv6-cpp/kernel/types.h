#pragma once

#include <stddef.h>
#include <stdint.h>
#include <utility>

#define MACRO_CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) MACRO_CONCAT_IMPL(x, y)

namespace utils {
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
        defer_guard_impl(Func&& f) : m_func(std::forward<Func>(f)) {}
        ~defer_guard_impl() { m_func(); }

    private:
        Func m_func;
    };

} // namespace utils

#define defer ::utils::defer_guard_impl CONCAT(_defer_guard_, __COUNTER__) = [&]()
