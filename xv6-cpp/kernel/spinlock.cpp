#include "spinlock.h"
#include "cpu.h"
#include "riscv.h"

#ifdef __INTELLISENSE__
#define __atomic_exchange_n(...) 1
#define __atomic_load_n(...) 1
#define __atomic_store_n(...) 1
#endif

namespace xv6 {
    [[noreturn]] void panic(const char*); // TODO: take this from its own header once printf/console is rewritten

    void spinlock::lock() {
        cpu::push_intr_off(); // disable interrupts to avoid deadlock with an interrupt handler taking this lock
        if (holding()) panic("Spinlock::lock: already held");

        // On RISC-V the exchange is an amoswap.w.aq: the acquire ordering keeps the critical
        // section's loads and stores from moving above it. While the lock is taken we spin on a
        // plain load (test-and-test-and-set) so waiting harts do not hammer the bus with AMOs.
        while (__atomic_exchange_n(&m_locked, locked, __ATOMIC_ACQUIRE) != unlocked) {
            while (__atomic_load_n(&m_locked, __ATOMIC_RELAXED) != unlocked);
        }
        // Atomic (relaxed) only so that a concurrent holding() on another hart is not a data race.
        __atomic_store_n(&m_holder, riscv::tp, __ATOMIC_RELAXED);
    }

    void spinlock::unlock() {
        if (!holding()) panic("Spinlock::unlock::not_held");
        __atomic_store_n(&m_holder, -1, __ATOMIC_RELAXED);
        // The release store makes everything written in the critical section visible to the next
        // holder before the lock reads as free; on RISC-V it is a `fence rw,w` and a plain store.
        __atomic_store_n(&m_locked, unlocked, __ATOMIC_RELEASE);
        cpu::pop_intr_off();
    }

    bool spinlock::holding() const {
        // If another hart holds the lock, m_holder is that hart's id and never ours.
        return (__atomic_load_n(&m_locked, __ATOMIC_RELAXED) == locked) && (__atomic_load_n(&m_holder, __ATOMIC_RELAXED) == riscv::tp);
    }

} // namespace xv6
