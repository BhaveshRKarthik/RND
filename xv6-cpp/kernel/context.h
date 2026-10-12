#pragma once

#include "types.h"
#include <type_traits>

namespace xv6 {

    // [TODO]: hardware floating point is not supported in the kernel. Nothing here saves or restores
    // FP state, and the kernel is built without F/D (-march=rv64imac_zicsr_zifencei -mabi=lp64), so
    // float/double code compiles to soft-float libcalls. (Soft-float needs none of the below; it is
    // merely slower.) To use real FP registers in any kernel path, a scheduler included, you must:
    //   1. Build the kernel with F/D, or the compiler will not emit FP instructions at all.
    //   2. Add the callee-saved FP registers (fs0-fs11) to Context and to the offsets in swtch.S;
    //      otherwise values live across a swtch() are silently clobbered.
    //   3. Make kernelvec.S save and restore every FP register and fcsr: a kernel trap can arrive at
    //      any instruction, and the handler is ordinary code that may use FP itself.
    //   4. Manage sstatus.FS: with it Off, the first FP instruction raises an illegal-instruction
    //      trap, and the Dirty state tells you when a save is needed.
    //   5. If user programs may use FP too, save their FP state in the trapframe (trampoline.S).

    struct context {
        uint64_t ra;
        uint64_t sp;

        // callee-saved
        uint64_t s0;
        uint64_t s1;
        uint64_t s2;
        uint64_t s3;
        uint64_t s4;
        uint64_t s5;
        uint64_t s6;
        uint64_t s7;
        uint64_t s8;
        uint64_t s9;
        uint64_t s10;
        uint64_t s11;
    };

    static_assert(std::is_standard_layout_v<context>, "context should be standard layout.");
    static_assert(sizeof(context) == 14 * 8, "context size has changed. verify swtch.S before updating this static_assert. "
                                             "If floating point registers have been added, add static asserts for their offsets as well.");

#define XV6_ASSERT_CONTEXT_OFFSET(member, expected_offset) \
    static_assert(offsetof(context, member) == expected_offset, "context::"#member" offset has changed. verify swtch.S before updating this static_assert.")

    XV6_ASSERT_CONTEXT_OFFSET(ra, 0);
    XV6_ASSERT_CONTEXT_OFFSET(sp, 8);
    XV6_ASSERT_CONTEXT_OFFSET(s0, 16);
    XV6_ASSERT_CONTEXT_OFFSET(s1, 24);
    XV6_ASSERT_CONTEXT_OFFSET(s2, 32);
    XV6_ASSERT_CONTEXT_OFFSET(s3, 40);
    XV6_ASSERT_CONTEXT_OFFSET(s4, 48);
    XV6_ASSERT_CONTEXT_OFFSET(s5, 56);
    XV6_ASSERT_CONTEXT_OFFSET(s6, 64);
    XV6_ASSERT_CONTEXT_OFFSET(s7, 72);
    XV6_ASSERT_CONTEXT_OFFSET(s8, 80);
    XV6_ASSERT_CONTEXT_OFFSET(s9, 88);
    XV6_ASSERT_CONTEXT_OFFSET(s10, 96);
    XV6_ASSERT_CONTEXT_OFFSET(s11, 104);

    extern "C" void swtch(context& from_ctx, context& to_ctx);

} // namespace xv6
