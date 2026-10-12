# qemu -kernel loads the kernel at 0x80000000
# and causes each hart (i.e. CPU) to jump there.
# kernel.ld causes the following code to
# be placed at 0x80000000.
.section .text
.global _entry

#include "entry.h"

_entry:
        # set up a stack for C++.

        # kernel_stack_base is declared in start.cpp,
        # with a KSTACKSIZE-byte stack per CPU.
        # sp = (kernel_stack_base + KSTACKSIZE) + (hartid  * KSTACKSIZE)

        la sp, kernel_stack_base + KSTACKSIZE # top of stack for cpu0
        li a0, KSTACKSIZE
        csrr a1, mhartid
        mul a0, a0, a1
        add sp, sp, a0

        # jump to start() in start.c
        call start

        # catch the return from start() and spin.
spin:
        j spin
