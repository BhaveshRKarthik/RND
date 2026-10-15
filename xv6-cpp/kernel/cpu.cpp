#include "cpu.h"
#include "riscv.h"

namespace xv6 {
    [[noreturn]] void panic(const char*);

    std::array<cpu, cpu_count> cpu::m_cpus = {};

    cpu& cpu::current() {
        // The m_cpus array is indexed by riscv::tp, which is guaranteed to be unique for each CPU.
        if (riscv::intr_enabled()) panic("CPU::current::intr_enabled");
        return m_cpus[riscv::tp];
    }

    void cpu::push_intr_off() {
        // Disable interrupts first, so we cannot be rescheduled while using current().
        // exchange_and returns the old sstatus, so we learn the previous SIE state atomically.

        const bool was_on = riscv::sstatus.exchange_and(~riscv::sstatus::sie) & riscv::sstatus::sie;

        cpu& c = m_cpus[riscv::tp];
        if (c.m_noff == 0) c.m_intena = was_on;
        c.m_noff++;
    }

    void cpu::pop_intr_off() {
        if (riscv::intr_enabled()) panic("cpu::pop_intr_off::interruptible");
        cpu& c = m_cpus[riscv::tp];
        if (c.m_noff < 1) panic("cpu::pop_intr_off::unbalanced");
        c.m_noff--;
        if (c.m_noff == 0 && c.m_intena) riscv::sstatus |= riscv::sstatus::sie;
    }

} // namespace xv6
