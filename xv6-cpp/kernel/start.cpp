#include "cpu.h"
#include "config.h"
#include "riscv.h"
#include "types.h"


namespace xv6 {
    void kmain();

    extern "C" {
    // alignas 16 required for ABI. can make stricter (say to kernel_stack_size/4096/etc)
    alignas(16) char kernel_stack_base[kernel_stack_size * cpu_count];
    }

    extern "C" void start() {
        // set M Previous Privilege mode to Supervisor, for mret.
        riscv::mstatus &= ~riscv::mstatus::mpp_mask;
        riscv::mstatus |= riscv::mstatus::mpp_s;

        // set M Exception Program Counter to main, for mret.
        // requires gcc -mcmodel=medany
        riscv::mepc = reinterpret_cast<uint64_t>(kmain);

        // disable paging for now.
        riscv::satp = riscv::satp::mode_bare;

        // delegate all interrupts and exceptions to supervisor mode.
        riscv::medeleg = 0xFFFF;
        riscv::mideleg = 0xFFFF;
        riscv::sie |= riscv::sie::seie | riscv::sie::stie;

        // configure Physical Memory Protection to give supervisor mode
        // access to all of physical memory.
        riscv::pmpaddr0 = 0x3FFFFFFFFFFFFFULL;
        riscv::pmpcfg0  = riscv::pmpcfg0::r | riscv::pmpcfg0::w | riscv::pmpcfg0::x | riscv::pmpcfg0::a_tor;

        // enable hardware updates of page table A and D bits
        riscv::menvcfg |= riscv::menvcfg::adue;

        
        // ask each hart to generate timer interrupts but dont ask for the first one
        // enable the sstc extension (i.e. stimecmp).
        riscv::menvcfg |= riscv::menvcfg::stce;
        // allow supervisor to use stimecmp and time.
        riscv::mcounteren |= riscv::mcounteren::tm;

        // keep each CPU's hartid in its tp register, for cpuid().
        riscv::tp = riscv::mhartid;

        // switch to supervisor mode and jump to main().
        __asm volatile("mret");
    }

} // namespace xv6
