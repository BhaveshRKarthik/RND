#pragma once

// xv6::kernel_stack_size should be preferred over this macro in C++ code, but this macro is needed for assembler code.
#define KSTACKSIZE 4096

// xv6::cpu_count should be preferred over this macro in C++ code, but this macro is needed? for assembler code.
#define NCPU 8

#define UART0_BASE 0x1000'0000

#ifndef __ASSEMBLER__

#include "types.h"

namespace xv6 {
    constexpr size_t kernel_stack_size = KSTACKSIZE;
    constexpr size_t cpu_count         = NCPU;
} // namespace xv6

#endif
