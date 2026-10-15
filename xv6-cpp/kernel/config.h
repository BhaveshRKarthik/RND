#pragma once

// xv6::kernel_stack_size should be preferred over this macro in C++ code, but this macro is needed for assembler code.
#define KERNEL_STACK_SIZE 4096

// xv6::cpu_count should be preferred over this macro in C++ code, but this macro is needed? for assembler code.
#define CPU_COUNT 8

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAX_VIRTUAL_ADDRESS (1L << (9 + 9 + 9 + 12 - 1))

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0
// 10001000 -- virtio disk
// 80000000 -- qemu's boot ROM loads the kernel here,
//             then jumps here.
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// qemu puts UART registers here in physical memory.
#define UART0_BASE 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0_BASE 0x10001000
#define VIRTIO0_IRQ 1

// core-local interrupt controller (CLINT)
#define CLINT_BASE 0x02000000L
//#define CLINT(hart) (CLINT_BASE + (hart) * 4)

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC_BASE 0x0c000000L
//#define PLIC_PRIORITY (PLIC + 0x0)
//#define PLIC_PENDING (PLIC + 0x1000)
//#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart) * 0x100)
//#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart) * 0x2000)
//#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart) * 0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNEL_BASE 0x80000000L
#define PHYSICAL_ADDRESS_END (KERNEL_BASE + 128 * 1024 * 1024)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXIMUM_VIRTUAL_ADDRESS - PAGE_SIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#ifndef __ASSEMBLER__

#include "types.h"

namespace xv6 {
    constexpr size_t kernel_stack_size = KERNEL_STACK_SIZE;
    constexpr size_t cpu_count         = CPU_COUNT;
} // namespace xv6

#endif
