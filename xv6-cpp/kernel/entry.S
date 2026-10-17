# qemu -kernel loads the kernel at 0x80000000
# and causes each hart (i.e. CPU) to jump there.
# kernel.ld causes the following code to
# be placed at 0x80000000.
.section .text
.global _entry

#include "config.h"

_entry:
        # set up a stack for C++.

        # kernel_stack_base is declared in start.cpp,
        # with a KERNEL_STACK_SIZE-byte stack per CPU.
        # sp = (kernel_stack_base + KERNEL_STACK_SIZE) + (hartid  * KERNEL_STACK_SIZE)

        la sp, kernel_stack_base + KERNEL_STACK_SIZE # top of stack for cpu0
        li a0, KERNEL_STACK_SIZE
        csrr a1, mhartid
        mul a0, a0, a1
        add sp, sp, a0

        # jump to start() in start.c
        call start

        # catch the return from start() and spin.
spin:
        j spin
