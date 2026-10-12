#pragma once

#ifndef __ASSEMBLER__

#include "types.h"

// Bitwise operators for a CSR's flag_type. Use inside the CSR's field struct, after the enum:
//   XV6_RISCV_FLAG_TYPE_OPERATORS;
// The trailing static_assert(true) exists so the caller's ';' closes a declaration instead of being a stray semicolon.
#define XV6_RISCV_FLAG_TYPE_OPERATORS                                                                                                                          \
    friend inline constexpr flag_type operator|(flag_type lhs, flag_type rhs) {                                                                                \
        return static_cast<flag_type>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));                                                                \
    }                                                                                                                                                          \
    friend inline constexpr flag_type operator&(flag_type lhs, flag_type rhs) {                                                                                \
        return static_cast<flag_type>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs));                                                                \
    }                                                                                                                                                          \
    friend inline constexpr flag_type operator~(flag_type value) {                                                                                             \
        return static_cast<flag_type>(~static_cast<uint64_t>(value));                                                                                          \
    }                                                                                                                                                          \
    friend inline constexpr flag_type operator^(flag_type lhs, flag_type rhs) {                                                                                \
        return static_cast<flag_type>(static_cast<uint64_t>(lhs) ^ static_cast<uint64_t>(rhs));                                                                \
    }                                                                                                                                                          \
    friend inline constexpr flag_type& operator|=(flag_type& lhs, flag_type rhs) {                                                                             \
        return lhs = lhs | rhs;                                                                                                                                \
    }                                                                                                                                                          \
    friend inline constexpr flag_type& operator&=(flag_type& lhs, flag_type rhs) {                                                                             \
        return lhs = lhs & rhs;                                                                                                                                \
    }                                                                                                                                                          \
    friend inline constexpr flag_type& operator^=(flag_type& lhs, flag_type rhs) {                                                                             \
        return lhs = lhs ^ rhs;                                                                                                                                \
    }

namespace riscv {

    namespace detail::csr {
        // is csr_addr a valid CSR address?
        // 0x000-0xFFF are standard valid CSR addresses.
        template<uint16_t csr_addr>
        concept valid = csr_addr <= 0xFFF;

        // is csr_addr a readable CSR address?
        // 0x000-0xFFF are standard valid CSR addresses, all are readable.
        template<uint16_t csr_addr>
        concept readable = valid<csr_addr>;

        // is csr_addr a writable CSR address?

        template<uint16_t csr_addr>
        concept writable = valid<csr_addr> && (((csr_addr >> 10) & 0b11) != 0b11);
    } // namespace detail::csr

    struct no_fields {
        using flag_type = uint64_t;
    };

    // CSR (Control and Status Register) wrapper. Provides read/write access to a CSR at compile-time constant address csr_addr.
    template<uint16_t csr_addr, typename Fields = no_fields>
    struct CSR : Fields, private utils::immovable {
        static_assert(detail::csr::valid<csr_addr>, "CSR address must be less than 4096");

        using flag_type = Fields::flag_type;

        constexpr CSR() = default;

        operator flag_type() const requires(detail::csr::readable<csr_addr>) {
            uint64_t value;
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
            return static_cast<flag_type>(value);
        }

        void operator=(flag_type value) const requires(detail::csr::writable<csr_addr>) {
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
        }

        // csr |= mask  (set bits)
        void operator|=(flag_type mask) const requires(detail::csr::writable<csr_addr>) {
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
        }

        // csr &= mask  (clear every bit NOT in mask)
        void operator&=(flag_type mask) const requires(detail::csr::writable<csr_addr>) {
            __asm volatile("csrc %0, %1" : : "i"(csr_addr), "rK"(~static_cast<uint64_t>(mask)) : "memory");
        }

        // conceptually equivalent to std::exchange(reg, value) but atomic.
        flag_type exchange(flag_type value) const requires(detail::csr::writable<csr_addr>) {
            uint64_t old_value;
            __asm volatile("csrrw %0, %1, %2" : "=r"(old_value) : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
            return static_cast<flag_type>(old_value);
        }

        // conceptually equivalent to std::exchange(reg, reg | mask) but atomic.
        flag_type exchange_or(flag_type mask) const requires(detail::csr::writable<csr_addr>) {
            uint64_t old_value;
            __asm volatile("csrrs %0, %1, %2" : "=r"(old_value) : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
            return static_cast<flag_type>(old_value);
        }

        // conceptually equivalent to std::exchange(reg, reg & mask) but atomic.
        flag_type exchange_and(flag_type mask) const requires(detail::csr::writable<csr_addr>) {
            uint64_t old_value;
            __asm volatile("csrrc %0, %1, %2" : "=r"(old_value) : "i"(csr_addr), "rK"(~static_cast<uint64_t>(mask)) : "memory");
            return static_cast<flag_type>(old_value);
        }
    };

    /** ------------------------------------------------------------------
    // Field definitions. Each struct shares its name with the CSR variable below,
    // so enumerators are reachable as both  mstatus::mpp_s  and  mstatus.mpp_s.
    // ------------------------------------------------------------------ **/

    // Machine Status Register
    struct mstatus {
        enum flag_type : uint64_t {
            /// previous privilege mode mask
            mpp_mask = 3ULL << 11,
            /// machine mode
            mpp_m = 3ULL << 11,
            /// supervisor mode
            mpp_s = 1ULL << 11,
            /// user mode
            mpp_u = 0ULL << 11,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Supervisor Status Register
    struct sstatus {
        enum flag_type : uint64_t {
            /// previous mode, 1=Supervisor, 0=User
            spp = 1ULL << 8,
            /// Supervisor Previous Interrupt Enable
            spie = 1ULL << 5,
            /// Supervisor Interrupt Enable
            sie = 1ULL << 1,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Supervisor Interrupt Pending
    struct sip {
        enum flag_type : uint64_t {
            /// supervisor software interrupt (xv6 clears this for timer ticks)
            ssip = 1ULL << 1,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Supervisor Interrupt Enable
    struct sie {
        enum flag_type : uint64_t {
            /// external
            seie = 1ULL << 9,
            /// timer
            stie = 1ULL << 5,
            /// software
            ssie = 1ULL << 1,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Machine-mode Interrupt Enable
    struct mie {
        enum flag_type : uint64_t {
            /// supervisor timer
            stie = 1ULL << 5,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Machine Environment Configuration Register
    struct menvcfg {
        enum flag_type : uint64_t {
            /// Sstc: enable stimecmp
            stce = 1ULL << 63,
            /// hardware A/D bit updates
            adue = 1ULL << 61,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Machine-mode Counter-Enable
    struct mcounteren {
        enum flag_type : uint64_t {
            /// cycle
            cy = 1ULL << 0,
            /// time
            tm = 1ULL << 1,
            /// instret
            ir = 1ULL << 2,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Physical Memory Protection config (bits for entry 0 live in the low byte of pmpcfg0)
    struct pmpcfg0 {
        enum flag_type : uint64_t {
            /// read
            r = 1ULL << 0,
            /// write
            w = 1ULL << 1,
            /// execute
            x = 1ULL << 2,
            /// address matching mode mask
            a_mask = 3ULL << 3,
            /// off (disabled)
            a_off = 0ULL << 3,
            /// top of range
            a_tor = 1ULL << 3,
            /// naturally aligned 4-byte region
            a_na4 = 2ULL << 3,
            /// naturally aligned power-of-two region
            a_napot = 3ULL << 3,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Supervisor address translation and protection
    struct satp {
        enum flag_type : uint64_t {
            /// address translation mode mask
            mode_mask = 15ULL << 60,
            /// no translation or protection
            mode_bare = 0ULL << 60,
            /// 39-bit virtual addressing with 3-level page table
            mode_sv39 = 8ULL << 60,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Supervisor Trap Cause
    struct scause {
        enum flag_type : uint64_t {
            /// set for interrupts, clear for exceptions
            interrupt = 1ULL << 63,
        };
        XV6_RISCV_FLAG_TYPE_OPERATORS;
    };

    // Thread Pointer (tp) register
    struct tp_t : private utils::immovable {
        constexpr tp_t() = default;
        operator uint64_t() const {
            uint64_t x;
            __asm volatile("mv %0, tp" : "=r"(x));
            return x;
        }
        void operator=(uint64_t value) const {
            __asm volatile("mv tp, %0" : : "r"(value)); // rK not valid for mv
        }
    };

    // ------------------------------------------------------------------
    // CSR instances
    // ------------------------------------------------------------------

    /* Machine-level */

    // Machine Status
    inline constexpr CSR<0x300, struct mstatus> mstatus = {};
    // Machine Exception Delegation
    inline constexpr CSR<0x302> medeleg = {};
    // Machine Interrupt Delegation
    inline constexpr CSR<0x303> mideleg = {};
    // Machine Interrupt Enable
    inline constexpr CSR<0x304, struct mie> mie = {};
    // Machine Counter-Enable
    inline constexpr CSR<0x306, struct mcounteren> mcounteren = {};
    // Machine Environment Config
    inline constexpr CSR<0x30A, struct menvcfg> menvcfg = {};
    // Machine Exception PC
    inline constexpr CSR<0x341> mepc = {};
    // PMP config 0
    inline constexpr CSR<0x3A0, struct pmpcfg0> pmpcfg0 = {};
    // PMP address 0
    inline constexpr CSR<0x3B0> pmpaddr0 = {};
    // Hart ID (read-only)
    inline constexpr CSR<0xF14> mhartid = {};

    /* Supervisor-level */

    // Supervisor Status
    inline constexpr CSR<0x100, struct sstatus> sstatus = {};
    // Supervisor Interrupt Enable
    inline constexpr CSR<0x104, struct sie> sie = {};
    // Supervisor Trap-Vector Base (low 2 bits = mode)
    inline constexpr CSR<0x105> stvec = {};
    // Supervisor Exception PC
    inline constexpr CSR<0x141> sepc = {};
    // Supervisor Trap Cause
    inline constexpr CSR<0x142, struct scause> scause = {};
    // Supervisor Trap Value
    inline constexpr CSR<0x143> stval = {};
    // Supervisor Interrupt Pending
    inline constexpr CSR<0x144, struct sip> sip = {};
    // Supervisor Timer Compare (Sstc)
    inline constexpr CSR<0x14D> stimecmp = {};
    // Address translation and protection
    inline constexpr CSR<0x180, struct satp> satp = {};

    /* User-level (read-only shadows) */

    // Timer (read-only)
    inline constexpr CSR<0xC01> time = {};

    // Thread Pointer (tp) register
    inline constexpr tp_t tp = {};

    // equivalent of the old MAKE_SATP(pagetable): Sv39 mode + physical page number
    constexpr satp::flag_type make_satp(uint64_t pagetable_addr) {
        return static_cast<satp::flag_type>(satp::mode_sv39 | (pagetable_addr >> 12));
    }

    // are device supervisor interrupts enabled?
    inline bool intr_enabled() {
        return sstatus & sstatus::sie;
    }

    // turn on device supervisor interrupts
    inline void intr_enable() {
        sstatus |= sstatus::sie;
    }

    // turn off device supervisor interrupts
    inline void intr_disable() {
        sstatus &= ~sstatus::sie;
    }
} // namespace riscv

#endif // __ASSEMBLER__

// #define PGSIZE  4096 // bytes per page
// #define PGSHIFT 12   // bits of offset within a page
//
// #define PGROUNDUP(sz)  (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
// #define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))
//
// #define PTE_V (1L << 0) // valid
// #define PTE_R (1L << 1)
// #define PTE_W (1L << 2)
// #define PTE_X (1L << 3)
// #define PTE_U (1L << 4) // user can access
//
//// shift a physical address to the right place for a PTE.
// #define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)
//
// #define PTE2PA(pte) (((pte) >> 10) << 12)
//
// #define PTE_FLAGS(pte) ((pte) & 0x3FF)
//
//// extract the three 9-bit page table indices from a virtual address.
// #define PXMASK         0x1FF // 9 bits
// #define PXSHIFT(level) (PGSHIFT + (9 * (level)))
// #define PX(level, va)  ((((uint64)(va)) >> PXSHIFT(level)) & PXMASK)
//
//// one beyond the highest possible virtual address.
//// MAXVA is actually one bit less than the max allowed by
//// Sv39, to avoid having to sign-extend virtual addresses
//// that have the high bit set.
// #define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
