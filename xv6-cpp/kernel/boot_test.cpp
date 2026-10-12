#include "cpu.h"
#include "riscv.h"

namespace xv6 { extern "C" char kernel_stack_base[]; }

namespace {
volatile unsigned char* const uart = reinterpret_cast<volatile unsigned char*>(0x10000000UL);
void putc_(char c) { *uart = static_cast<unsigned char>(c); }
void puts_(const char* s) { while (*s) putc_(*s++); }
void puthex(uint64_t v) { puts_("0x"); for (int i = 60; i >= 0; i -= 4) putc_("0123456789abcdef"[(v >> i) & 0xf]); }
void putdec(uint64_t v) { char b[21]; int n = 0; do { b[n++] = char('0' + v % 10); v /= 10; } while (v); while (n) putc_(b[--n]); }
uint32_t print_lock_, done_;
void lock_()   { while (__atomic_exchange_n(&print_lock_, 1u, __ATOMIC_ACQUIRE)) {} }
void unlock_() { __atomic_store_n(&print_lock_, 0u, __ATOMIC_RELEASE); }
}

namespace xv6 {
[[noreturn]] void panic(const char* m) { lock_(); puts_("PANIC: "); puts_(m); putc_('\n'); for (;;) {} }
}

extern "C" __attribute__((interrupt("supervisor"), aligned(4))) void test_trap() {
    lock_(); puts_("TRAP scause="); puthex(riscv::scause); puts_(" sepc="); puthex(riscv::sepc);
    puts_(" stval="); puthex(riscv::stval); putc_('\n'); for (;;) {}
}

void main() {
    using namespace xv6;
    riscv::stvec = reinterpret_cast<uint64_t>(&test_trap);

    const uint64_t hart = riscv::tp;
    uint64_t sp; __asm volatile("mv %0, sp" : "=r"(sp));
    const uint64_t base = reinterpret_cast<uint64_t>(kernel_stack_base) + hart * 4096;

    const bool     intr0 = riscv::intr_enabled();
    const uint64_t sie_v = riscv::sie;
    const uint64_t satp_v = riscv::satp;
    const uint64_t t0  = riscv::time;
    const uint64_t cmp = riscv::stimecmp;
    riscv::stimecmp = ~0ULL;                       // no timer interrupts for the rest of the test
    const uint64_t t1  = riscv::time;

    cpu& c = cpu::current();                       // interrupts are off here, so this must not panic

    cpu::push_intr_off(); const bool a1 = riscv::intr_enabled(); cpu::pop_intr_off(); const bool a2 = riscv::intr_enabled();

    riscv::intr_enable();
    const bool b0 = riscv::intr_enabled();
    cpu::push_intr_off(); const bool b1 = riscv::intr_enabled();
    cpu::push_intr_off(); const bool b2 = riscv::intr_enabled();
    cpu::pop_intr_off();  const bool b3 = riscv::intr_enabled();
    cpu::pop_intr_off();  const bool b4 = riscv::intr_enabled();
    riscv::intr_disable();

    lock_();
    puts_("hart "); putdec(hart);
    puts_(" | sp in own stack: "); puts_((sp > base && sp <= base + 4096) ? "yes" : "NO");
    puts_(" | SIE at boot: "); putdec(intr0);
    puts_(" | sie="); puthex(sie_v);
    puts_(" | satp="); puthex(satp_v);
    puts_(" | time runs: "); puts_(t1 >= t0 ? "yes" : "NO");
    puts_(" | stimecmp ahead of time: "); puts_(cmp > t0 ? "yes" : "NO");
    puts_(" | cpu@"); puthex(reinterpret_cast<uint64_t>(&c));
    puts_(" | push/pop from off: "); putdec(a1); putdec(a2);
    puts_(" | push/pop from on: "); putdec(b0); putdec(b1); putdec(b2); putdec(b3); putdec(b4);
    putc_('\n');
    unlock_();

    __atomic_fetch_add(&done_, 1u, __ATOMIC_SEQ_CST);
    if (hart != 0) for (;;) {}

    while (__atomic_load_n(&done_, __ATOMIC_SEQ_CST) != 3) {}
    lock_(); puts_("all 3 harts reported; now an unbalanced pop_intr_off (expect a panic):\n"); unlock_();
    cpu::pop_intr_off();
    lock_(); puts_("NO PANIC (unexpected)\n"); unlock_();
    for (;;) {}
}
