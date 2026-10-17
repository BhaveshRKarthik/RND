#include "spinlock.h"
#include "cpu.h"
#include "riscv.h"

namespace xv6 {
    [[noreturn]] void panic(const char*); // TODO: take this from its own header once printf/console is rewritten

    void spinlock::lock() {
        cpu::push_intr_off(); // disable interrupts to avoid deadlock with an interrupt handler taking this lock
        if (holding()) panic("spinlock::lock::already_held");

        // On RISC-V the exchange is an amoor.w.aq: the acquire ordering keeps the critical
        // section's loads and stores from moving above it. While the lock is taken we spin on a
        // plain load (test-and-test-and-set) so waiting harts do not hammer the bus with AMOs.
        while (m_locked.test_and_set(std::memory_order_acquire)) {
            while (m_locked.test(std::memory_order_relaxed));
        }

        // Atomic (relaxed) only so that a concurrent holding() on another hart is not a data race.
        m_holder.store(static_cast<uint32_t>(riscv::tp), std::memory_order_relaxed);
    }

    void spinlock::release() {
        if (!holding()) panic("spinlock::release::not_held");
        m_holder.store(no_holder, std::memory_order_relaxed);
        // The release store makes everything written in the critical section visible to the next
        // holder before the lock reads as free; on RISC-V it is a full `fence` and a plain store.
        m_locked.clear(std::memory_order_release);
        cpu::pop_intr_off();
    }

    bool spinlock::holding() const {
        // If another hart holds the lock, m_holder is that hart's id and never ours.
        return m_locked.test(std::memory_order_relaxed) && (m_holder.load(std::memory_order_relaxed) == riscv::tp);
    }

} // namespace xv6
