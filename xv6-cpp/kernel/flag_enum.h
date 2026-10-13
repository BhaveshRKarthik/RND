#pragma once

#include "types.h"

#define XV6_FLAG_ENUM_OPERATORS_CLASS(Enum)                                                                                                                    \
    friend inline constexpr Enum operator|(Enum lhs, Enum rhs) {                                                                                               \
        return static_cast<Enum>(std::to_underlying(lhs) | std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    friend inline constexpr Enum operator&(Enum lhs, Enum rhs) {                                                                                               \
        return static_cast<Enum>(std::to_underlying(lhs) & std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    friend inline constexpr Enum operator~(Enum value) {                                                                                                       \
        return static_cast<Enum>(~std::to_underlying(value));                                                                                                  \
    }                                                                                                                                                          \
    friend inline constexpr Enum operator^(Enum lhs, Enum rhs) {                                                                                               \
        return static_cast<Enum>(std::to_underlying(lhs) ^ std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    friend inline constexpr Enum& operator|=(Enum& lhs, Enum rhs) {                                                                                            \
        return lhs = lhs | rhs;                                                                                                                                \
    }                                                                                                                                                          \
    friend inline constexpr Enum& operator&=(Enum& lhs, Enum rhs) {                                                                                            \
        return lhs = lhs & rhs;                                                                                                                                \
    }                                                                                                                                                          \
    friend inline constexpr Enum& operator^=(Enum& lhs, Enum rhs) {                                                                                            \
        return lhs = lhs ^ rhs;                                                                                                                                \
    }

#define XV6_FLAG_ENUM_OPERATORS_GLOBAL(Enum)                                                                                                                   \
    inline constexpr Enum operator|(Enum lhs, Enum rhs) {                                                                                                      \
        return static_cast<Enum>(std::to_underlying(lhs) | std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    inline constexpr Enum operator&(Enum lhs, Enum rhs) {                                                                                                      \
        return static_cast<Enum>(std::to_underlying(lhs) & std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    inline constexpr Enum operator~(Enum value) {                                                                                                              \
        return static_cast<Enum>(~std::to_underlying(value));                                                                                                  \
    }                                                                                                                                                          \
    inline constexpr Enum operator^(Enum lhs, Enum rhs) {                                                                                                      \
        return static_cast<Enum>(std::to_underlying(lhs) ^ std::to_underlying(rhs));                                                                           \
    }                                                                                                                                                          \
    inline constexpr Enum& operator|=(Enum& lhs, Enum rhs) {                                                                                                   \
        return lhs = lhs | rhs;                                                                                                                                \
    }                                                                                                                                                          \
    inline constexpr Enum& operator&=(Enum& lhs, Enum rhs) {                                                                                                   \
        return lhs = lhs & rhs;                                                                                                                                \
    }                                                                                                                                                          \
    inline constexpr Enum& operator^=(Enum& lhs, Enum rhs) {                                                                                                   \
        return lhs = lhs ^ rhs;                                                                                                                                \
    }
