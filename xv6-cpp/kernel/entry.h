#pragma once

// xv6::kernel_stack_size should be preferred over this macro in C++ code, but this macro is needed for assembler code.
#define KSTACKSIZE 4096

#ifndef __ASSEMBLER__

#include "types.h"

namespace xv6 {
    constexpr size_t kernel_stack_size = KSTACKSIZE;
}

#endif
