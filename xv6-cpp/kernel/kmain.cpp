#include "riscv.h"
#include "types.h"
#include "uart.h"

#include "print.h" // activates the printing mechanism: format_parser, printers, kprint, kprint_sync

using namespace xv6::print_literals; // operator""_fmt

// print test. Run on ONE hart (other harts are parked):
//   make qemu CPUS=1      (or: qemu-system-riscv64 -machine virt -bios none -kernel <img> -m 128M -smp 1 -nographic -monitor none)
// The test exits QEMU by itself through the test-finisher device: exit status 0 = everything passed,
// 1 = a check failed, or a trap/panic/timeout happened.
//
// Part 1 (T1) checks the parser and the printers without touching the hardware: each case formats into a
// capture buffer with format_to() and compares the text. Only failures print, plus one summary line.
// Part 2 (T2-T5) sends output through the real UART with kprint_sync() (polled) and kprint() (TX interrupts
// through the PLIC, sleep()/wakeup()), which needs the same setup as the UART test: M-mode setup like xv6's
// start.c (traps and interrupts delegated to S-mode, mcounteren.TM set), kernelvec.S, and a working spinlock.
//
// Expected output:
//   == print test ==
//   T1 formatting: <n> checks, 0 failed
//   -- T2 kprint_sync (polled), next line must read: sync: 42 0xff abc true
//   sync: 42 0xff abc true
//   -- T3 kprint (interrupt-driven), next line must read: irq: 42 0xff abc true
//   irq: 42 0xff abc true
//   [PASS] ...   (T3 checks)
//   -- T4 wide fields, next line: "right" right-aligned in 40 columns, '|', "left" left-aligned in 40 columns, '|'
//   <that line>
//   -- T5 explicit uart, next line must read: explicit uart: 0x00000000deadbeef
//   explicit uart: 0x00000000deadbeef
//   ALL PASS
//
// It defines its own sleep()/wakeup()/consoleintr() stubs and the uart0 object; if your tree already links
// real ones, drop those or you will get duplicate symbols (-DUART_DEFINED_ELSEWHERE skips uart0).

using init_array_fn = void (*)(void);
extern init_array_fn __init_array_start;
extern init_array_fn __init_array_end;

extern "C" void kernelvec();

namespace {
    // QEMU virt test finisher: 0x5555 = pass (exit 0), (code << 16) | 0x3333 = fail (exit status `code`).
    [[noreturn]] void finish(bool ok) {
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
        for (;;) __asm volatile("wfi");
    }

    volatile uint32_t* plic_reg(uintptr_t offset) {
        return reinterpret_cast<volatile uint32_t*>(PLIC_BASE + offset);
    }
    void plic_init_hart0() {
        *plic_reg(UART0_IRQ * 4) = 1;               // priority > threshold, otherwise it never fires
        *plic_reg(0x2080)        = 1u << UART0_IRQ; // S-mode enable bits, hart 0
        *plic_reg(0x201000)      = 0;               // S-mode priority threshold, hart 0
    }

    // ---- state shared between the stubs, the trap handler and the test body
    volatile uint64_t wake_seq     = 0; // bumped by wakeup(); sleep() waits for it to change
    volatile uint64_t sleep_calls  = 0;
    volatile uint64_t wakeup_calls = 0;
    volatile uint64_t stray_irqs   = 0;
    volatile uint64_t uart_irqs    = 0;

    void run_init_array() {
        for (auto p = &__init_array_start; p < &__init_array_end; ++p) (*p)();
    }

    // ---- capture: formats into a buffer instead of the UART
    using xv6::detail::print::print_context;

    struct capture {
        char data[256];
        size_t len    = 0;
        bool overflow = false;
    };

} // namespace

namespace xv6 {
    void panic(const char* str) {
        kprint_sync("xv6::panic {}"_fmt, str);
        while (true) {
            __asm volatile("wfi");
        }
    }
} // namespace xv6

#ifndef UART_DEFINED_ELSEWHERE
ns16550a::UART<UART0_BASE> ns16550a::uart0{ "uart0" };
#endif

// ---- hooks that uart.h expects from the rest of the kernel -------------------------------------------

// Single-hart stand-in for xv6's sleep(): drop the lock, wait for a wakeup(), re-take the lock.
// The sequence number is sampled while the lock is held (interrupts off), so a wakeup that arrives
// right after the release cannot be lost. Times out instead of hanging if the interrupt never comes.
void sleep(void*, xv6::spinlock& lk) {
    if (!lk.holding()) xv6::panic("sleep() called without holding the lock");
    sleep_calls        = sleep_calls + 1;
    const uint64_t seq = wake_seq;
    lk.release();
    if (!riscv::intr_enabled()) xv6::panic("sleep() with interrupts off: the wakeup could never arrive");
    const uint64_t deadline = riscv::time + 20'000'000; // 2 s
    while (wake_seq == seq) {
        if (riscv::time > deadline) xv6::panic("sleep() timed out: no wakeup, the UART TX interrupt never arrived");
    }
    lk.acquire();
}

void wakeup(void*) {
    wakeup_calls = wakeup_calls + 1;
    wake_seq     = wake_seq + 1;
}

// consoleintr() is declared in uart.h; nothing consumes input in this test
void consoleintr(int) {}

// ---- S-mode trap handler (entered through kernelvec.S): external interrupts go to the UART,
// everything else is a test failure --------------------------------------------------------------------
extern "C" void kerneltrap() {
    const uint64_t cause    = riscv::scause;
    const bool is_interrupt = (cause >> 63) != 0;
    const uint64_t code     = cause & 0xff;

    if (is_interrupt && code == 9) {              // supervisor external interrupt
        const uint32_t irq = *plic_reg(0x201004); // claim
        if (irq == UART0_IRQ) {
            uart_irqs = uart_irqs + 1;
            ns16550a::uart0.intr();
        } else if (irq != 0) {
            stray_irqs = stray_irqs + 1;
        }
        if (irq != 0) *plic_reg(0x201004) = irq; // complete
        return;
    }
    if (is_interrupt && code == 5) { // supervisor timer: silence it and carry on
        riscv::stimecmp = ~0ULL;
        return;
    }

    xv6::kprint_sync("TRAP scause={} sepc={} stval={}\n"_fmt, cause, +riscv::sepc, +riscv::stval);
    finish(false);
}

namespace xv6 {
    void kmain() {
        const uint64_t hart = riscv::tp;
        if (hart == 0) run_init_array();

        riscv::stvec    = reinterpret_cast<uint64_t>(&kernelvec);
        riscv::stimecmp = ~0ULL; // no timer interrupts during the test
        riscv::sie |= riscv::sie::seie;
        plic_init_hart0();
        riscv::intr_enable();

        if (hart == 0) {
            xv6::kprint("Hello World! {:>10} abcd\n"_fmt, true);
            xv6::kprint("Hello World! {:>10} abcd\n"_fmt, false);
            finish(true);
        }
    }
} // namespace xv6
