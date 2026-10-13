#include "riscv.h"
#include "types.h"

#ifdef __INTELLISENSE__
#define __attribute__(...)
#endif

// uart.h calls consoleintr() from inside a template body, so it has to be declared before uart.h is included.
void consoleintr(int c);

#include "uart.h"

// UART smoke test. Run on ONE hart (other harts are parked):
//   qemu-system-riscv64 -machine virt -bios none -kernel <img> -m 128M -smp 1 -nographic -monitor none
// The test exits QEMU by itself through the test-finisher device: exit status 0 = every check passed,
// 1 = a check failed, or a trap/panic/timeout happened. Receive is opt-in (-DTEST_RX, optionally
// -DRX_COUNT=n): you then have to type or pipe n characters into the serial console.
//
// What it assumes from the code that runs before kmain():
//   - M-mode setup like xv6's start.c: traps and interrupts delegated to S-mode, mcounteren.TM set
//     (the test reads `time` for timeouts), no timer interrupts wanted.
//   - spinlock works (push_off/pop_off, mycpu() through tp).
// It defines its own sleep()/wakeup()/consoleintr() stubs and the uart0 object; if your tree already
// links real ones, drop those or you will get duplicate symbols (-DUART_DEFINED_ELSEWHERE skips uart0).
//
// Expected output (the [PASS] lines are produced by the checks, the rest you compare by eye):
//   == uart test ==
//   -- T1 putc_sync, next line must read: putc_sync ok
//   putc_sync ok
//   -- T2 write(), next line must read: write ok
//   write ok
//   -- T3 long write, next line must be 200 digits cycling 0-9
//   01234567890123456789...
//   [PASS] ...   (one line per check)
//   ALL PASS

using init_array_fn = void (*)(void);
extern init_array_fn __init_array_start;
extern init_array_fn __init_array_end;

#ifndef RX_COUNT
#define RX_COUNT 5
#endif

namespace {
    // ---- raw console: pokes the UART directly so the report does not depend on the class under test.
    // Only ever used outside of uart0.write(), so it cannot disturb the TX-idle interrupt handshake.
    volatile unsigned char* const raw_thr = reinterpret_cast<volatile unsigned char*>(UART0_BASE);

    void raw_putc(char c) {
        *raw_thr = static_cast<unsigned char>(c);
    }
    void raw_puts(const char* s) {
        while (*s) raw_putc(*s++);
    }
    [[maybe_unused]] void raw_putdec(uint64_t v) {
        char b[21];
        int n = 0;
        do {
            b[n++] = char('0' + v % 10);
            v /= 10;
        } while (v);
        while (n) raw_putc(b[--n]);
    }
    void raw_puthex(uint64_t v) {
        raw_puts("0x");
        for (int i = 60; i >= 0; i -= 4) raw_putc("0123456789abcdef"[(v >> i) & 0xf]);
    }

    // QEMU virt test finisher: 0x5555 = pass (exit 0), (code << 16) | 0x3333 = fail (exit status `code`).
    [[noreturn]] void finish(bool ok) {
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
        for (;;) __asm volatile("wfi");
    }

    [[noreturn]] void park() {
        for (;;) __asm volatile("wfi");
    }

    uint64_t now() {
        return riscv::time; // 10 MHz on QEMU virt
    }

    // ---- QEMU virt PLIC (same layout as xv6's memlayout.h), hart 0, S-mode context.
    constexpr uintptr_t PLIC_BASE = 0x0c000000UL;
    constexpr uint32_t UART_IRQ   = 10;
    static_assert(UART0_BASE == 0x10000000UL, "this test hard-codes the QEMU virt UART IRQ and PLIC layout");

    volatile uint32_t* plic_reg(uintptr_t offset) {
        return reinterpret_cast<volatile uint32_t*>(PLIC_BASE + offset);
    }
    void plic_init_hart0() {
        *plic_reg(UART_IRQ * 4) = 1;              // priority > threshold, otherwise it never fires
        *plic_reg(0x2080)       = 1u << UART_IRQ; // S-mode enable bits, hart 0
        *plic_reg(0x201000)     = 0;              // S-mode priority threshold, hart 0
    }

    // ---- state shared between the stubs, the trap handler and the test body
    volatile uint64_t wake_seq     = 0; // bumped by wakeup(); sleep() waits for it to change
    volatile uint64_t sleep_calls  = 0;
    volatile uint64_t wakeup_calls = 0;
    volatile uint64_t stray_irqs   = 0;
    volatile uint64_t uart_irqs    = 0;
    volatile uint64_t rx_count     = 0;
    char rx_buf[64];

    // Set by a dynamic initializer, so it is only true once .init_array has run. The volatile write
    // makes the compiler unable to turn this into constant initialization.
    volatile uint32_t ctor_probe;
    bool ctor_ran = (ctor_probe = 1, true);

    int failures = 0;
    void check(const char* what, bool ok) {
        raw_puts(ok ? "[PASS] " : "[FAIL] ");
        raw_puts(what);
        raw_putc('\n');
        if (!ok) failures++;
    }

    void run_init_array() {
        for (auto p = &__init_array_start; p < &__init_array_end; ++p) (*p)();
    }
} // namespace

namespace xv6 {
    // weak, so a real panic() in your tree wins at link time
    [[noreturn]] __attribute__((weak)) void panic(const char* m) {
        raw_puts("PANIC: ");
        raw_puts(m);
        raw_putc('\n');
        finish(false);
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
    const uint64_t deadline = now() + 20'000'000; // 2 s
    while (wake_seq == seq) {
        if (now() > deadline) xv6::panic("sleep() timed out: no wakeup, the UART TX interrupt never arrived");
    }
    lk.acquire();
}

void wakeup(void*) {
    wakeup_calls = wakeup_calls + 1;
    wake_seq     = wake_seq + 1;
}

void consoleintr(int c) {
    if (rx_count < sizeof(rx_buf)) rx_buf[rx_count] = static_cast<char>(c);
    rx_count = rx_count + 1;
}

extern "C" void kernelvec(); 

// ---- S-mode trap handler: external interrupts go to the UART, everything else is a test failure ------
extern "C" void kerneltrap() {
    const uint64_t cause    = riscv::scause;
    const bool is_interrupt = (cause >> 63) != 0;
    const uint64_t code     = cause & 0xff;

    if (is_interrupt && code == 9) {              // supervisor external interrupt
        const uint32_t irq = *plic_reg(0x201004); // claim
        if (irq == UART_IRQ) {
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

    raw_puts("TRAP scause=");
    raw_puthex(cause);
    raw_puts(" sepc=");
    raw_puthex(riscv::sepc);
    raw_puts(" stval=");
    raw_puthex(riscv::stval);
    raw_putc('\n');
    finish(false);
}

namespace xv6 {
    void kmain() {
        const uint64_t hart = riscv::tp;
        if (hart != 0) park();

        if (!ctor_ran) run_init_array(); // uart0 is a global with a constructor
        if (!ctor_ran) xv6::panic("global constructors did not run");

        riscv::stvec    = reinterpret_cast<uint64_t>(&kernelvec);
        riscv::stimecmp = ~0ULL; // no timer interrupts during the test
        riscv::sie |= riscv::sie::seie;
        plic_init_hart0();
        riscv::intr_enable();

        raw_puts("== uart test ==\n");

        // T1: polled output
        raw_puts("-- T1 putc_sync, next line must read: putc_sync ok\n");
        for (const char* s = "putc_sync ok\n"; *s; ++s) ns16550a::uart0.putc_sync(*s);

        // T2: interrupt-driven transmit through sleep()/wakeup()
        raw_puts("-- T2 write(), next line must read: write ok\n");
        {
            static constexpr char msg[]  = "write ok\n";
            constexpr size_t len         = sizeof(msg) - 1;
            const uint64_t sleeps_before = sleep_calls;
            const uint64_t irqs_before   = uart_irqs;
            const bool intr_before       = riscv::intr_enabled();

            ns16550a::uart0.write(std::span<const char>(msg, len));

            check("T2 write() leaves the interrupt state as it found it", riscv::intr_enabled() == intr_before);
            // every character after the first waits for the TX-idle interrupt
            check("T2 write() slept once per extra character (TX interrupt path used)", sleep_calls - sleeps_before >= len - 1);
            check("T2 UART interrupts were delivered through the PLIC", uart_irqs > irqs_before);
        }

        // T3: more characters than any FIFO holds, nothing may be dropped or reordered
        raw_puts("-- T3 long write, next line must be 200 digits cycling 0-9\n");
        {
            char big[201];
            for (int i = 0; i < 200; i++) big[i] = char('0' + i % 10);
            big[200] = '\n';
            ns16550a::uart0.write(std::span<const char>(big, sizeof(big)));
            check("T3 long write() returned", true);
        }

        // T4: write() again right after a write(): the busy flag must not be stuck
        {
            static constexpr char msg[] = "second write ok\n";
            ns16550a::uart0.write(std::span<const char>(msg, sizeof(msg) - 1));
            check("T4 back-to-back write() did not get stuck", true);
        }

        // T5: receive path with nothing pending
#ifndef TEST_RX
        {
            char rb[8];
            check("T5 getc() returns -1 when no input is pending", ns16550a::uart0.getc() == -1);
            check("T5 read() returns 0 when no input is pending", ns16550a::uart0.read(rb) == 0);
        }
#else
        raw_puts("-- T5 rx: type or pipe ");
        raw_putdec(RX_COUNT);
        raw_puts(" characters into the serial console\n");
        {
            const uint64_t deadline = now() + 100'000'000; // 10 s
            while (rx_count < RX_COUNT && now() < deadline) {
            }
            check("T5 characters reached consoleintr() through the RX interrupt", rx_count >= RX_COUNT);
            raw_puts("   received: ");
            const size_t n = rx_count < sizeof(rx_buf) ? static_cast<size_t>(rx_count) : sizeof(rx_buf);
            for (size_t i = 0; i < n; i++) raw_putc(rx_buf[i] >= 32 && rx_buf[i] < 127 ? rx_buf[i] : '?');
            raw_putc('\n');
        }
#endif

        check("no stray PLIC interrupts", stray_irqs == 0);

        raw_puts(failures == 0 ? "ALL PASS\n" : "SOME CHECKS FAILED\n");
        finish(failures == 0);
    }
} // namespace xv6
