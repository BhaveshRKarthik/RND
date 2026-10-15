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
    // ---- raw console: pokes the UART directly so the report does not depend on the code under test.
    // Only ever used outside of kprint()/uart0.write(), so it cannot disturb the TX-idle interrupt handshake.
    volatile unsigned char* const raw_thr = reinterpret_cast<volatile unsigned char*>(UART0_BASE);

    void raw_putc(char c) {
        *raw_thr = static_cast<unsigned char>(c);
    }
    void raw_puts(const char* s) {
        while (*s) raw_putc(*s++);
    }
    void raw_write(std::string_view s) {
        for (char c : s) raw_putc(c);
    }
    void raw_putdec(uint64_t v) {
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

    // ---- capture: formats into a buffer instead of the UART
    using xv6::detail::print::print_context;

    struct capture {
        char data[256];
        size_t len    = 0;
        bool overflow = false;
    };

    void capture_write(std::span<const char> data, void* user) {
        auto& cap = *static_cast<capture*>(user);
        for (char c : data) {
            if (cap.len < sizeof(cap.data))
                cap.data[cap.len++] = c;
            else
                cap.overflow = true;
        }
    }

    // compared by hand: std::string_view's operator== would call memcmp, which the kernel does not have
    bool same_text(std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i != a.size(); ++i) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }

    uint64_t fmt_checks = 0;

    // Formats args with fmt and compares the result to `expected`. The expected text is taken as an array so its
    // length is known without strlen.
    template<size_t M, typename Fmt, typename... Args>
    void check_fmt(const char* name, const char (&expected)[M], Fmt fmt, const Args&... args) {
        capture cap;
        const print_context ctx(capture_write, &cap);
        xv6::detail::print::format_to(ctx, fmt, args...);

        const std::string_view want(expected, M - 1);
        const std::string_view got(cap.data, cap.len);
        ++fmt_checks;
        if (cap.overflow || !same_text(got, want)) {
            ++failures;
            raw_puts("[FAIL] ");
            raw_puts(name);
            raw_puts("\n       expected: \"");
            raw_write(want);
            raw_puts("\"\n       got:      \"");
            raw_write(got);
            raw_puts("\"\n");
        }
    }

    void test_formatting() {
        // literals and escaped braces
        check_fmt("empty format", "", ""_fmt);
        check_fmt("plain literal", "hello", "hello"_fmt);
        check_fmt("escaped braces", "a{b}c", "a{{b}}c"_fmt);
        check_fmt("four open braces", "{{", "{{{{"_fmt);
        check_fmt("doubled close brace", "}", "}}"_fmt);
        check_fmt("close brace run after text", "x}}y", "x}}}}y"_fmt);
        check_fmt("braces around a field", "{7}", "{{{}}}"_fmt, 7);
        check_fmt("braces around a field with spec", "{   5}", "{{{:>4}}}"_fmt, 5);

        // argument indexing
        check_fmt("automatic indexing", "1 2", "{} {}"_fmt, 1, 2);
        check_fmt("manual indexing", "b a b", "{1} {0} {1}"_fmt, 'a', 'b');
        check_fmt("repeated manual index", "77", "{0}{0}"_fmt, 7);
        check_fmt("extra arguments are ignored", "1", "{}"_fmt, 1, 2);

        // integers
        constexpr int64_t i64_min = -9223372036854775807L - 1;
        check_fmt("int", "42", "{}"_fmt, 42);
        check_fmt("negative int", "-42", "{}"_fmt, -42);
        check_fmt("int64 min", "-9223372036854775808", "{}"_fmt, i64_min);
        check_fmt("uint64 max", "18446744073709551615", "{}"_fmt, ~uint64_t{ 0 });
        check_fmt("uint8", "200", "{}"_fmt, static_cast<uint8_t>(200));
        check_fmt("int8", "-5", "{}"_fmt, static_cast<int8_t>(-5));
        check_fmt("decimal type", "42", "{:d}"_fmt, 42);
        check_fmt("hex", "ff", "{:x}"_fmt, 255);
        check_fmt("hex upper", "FF", "{:X}"_fmt, 255);
        check_fmt("hex alternate", "0xff", "{:#x}"_fmt, 255);
        check_fmt("hex alternate upper", "0XFF", "{:#X}"_fmt, 255);
        check_fmt("binary", "101", "{:b}"_fmt, 5);
        check_fmt("binary alternate", "0b101", "{:#b}"_fmt, 5);
        check_fmt("octal", "10", "{:o}"_fmt, 8);
        check_fmt("octal alternate", "010", "{:#o}"_fmt, 8);
        check_fmt("octal alternate of zero", "0", "{:#o}"_fmt, 0);
        check_fmt("width, numbers align right", "   42", "{:5}"_fmt, 42);
        check_fmt("align left", "42   ", "{:<5}"_fmt, 42);
        check_fmt("align center", " 42  ", "{:^5}"_fmt, 42);
        check_fmt("fill and align", "****42", "{:*>6}"_fmt, 42);
        check_fmt("zero padding", "00042", "{:05}"_fmt, 42);
        check_fmt("zero padding keeps the sign first", "-0042", "{:05}"_fmt, -42);
        check_fmt("plus sign", "+42", "{:+}"_fmt, 42);
        check_fmt("space sign", " 42", "{: }"_fmt, 42);
        check_fmt("alternate hex with zero padding", "0x00ff", "{:#06x}"_fmt, 255);
        check_fmt("explicit align cancels zero flag", "42   ", "{:<05}"_fmt, 42);
        check_fmt("integer as character", "A", "{:c}"_fmt, 65);
        check_fmt("integer too large for a character", "?", "{:c}"_fmt, 300);

        // char
        check_fmt("char", "x", "{}"_fmt, 'x');
        check_fmt("char as number", "65", "{:d}"_fmt, 'A');
        check_fmt("char width, left by default", "x  ", "{:3}"_fmt, 'x');
        check_fmt("char align right", "  x", "{:>3}"_fmt, 'x');

        // bool
        check_fmt("true", "true", "{}"_fmt, true);
        check_fmt("false", "false", "{}"_fmt, false);
        check_fmt("bool as number", "1", "{:d}"_fmt, true);
        check_fmt("bool align right", "  true", "{:>6}"_fmt, true);
        check_fmt("bool with s", "false", "{:s}"_fmt, false);

        // strings
        const char* null_str = nullptr;
        char mutable_buf[]   = "mutable";
        char* mutable_str    = mutable_buf;
        const std::string_view view("xyz", 3);
        check_fmt("string literal", "abc", "{}"_fmt, "abc");
        check_fmt("string precision", "abc", "{:.3}"_fmt, "abcdef");
        check_fmt("string align right", "   abc", "{:>6}"_fmt, "abc");
        check_fmt("string left by default", "abc   ", "{:6}"_fmt, "abc");
        check_fmt("string center with fill", "--abc--", "{:-^7}"_fmt, "abc");
        check_fmt("nullptr string", "(null)", "{}"_fmt, null_str);
        check_fmt("char*", "mutable", "{}"_fmt, mutable_str);
        check_fmt("string_view", "xyz", "{}"_fmt, view);
        check_fmt("string_view precision", "xy", "{:.2}"_fmt, view);

        // pointers
        void* const ptr          = reinterpret_cast<void*>(uintptr_t{ 0x1234 });
        void* const null_ptr     = nullptr;
        const int* const int_ptr = reinterpret_cast<const int*>(uintptr_t{ 0x10 });
        check_fmt("pointer", "0x1234", "{}"_fmt, ptr);
        check_fmt("pointer with p", "0x1234", "{:p}"_fmt, ptr);
        check_fmt("pointer with P", "0X1234", "{:P}"_fmt, ptr);
        check_fmt("null pointer", "0x0", "{}"_fmt, null_ptr);
        check_fmt("pointer zero padding", "0x001234", "{:08}"_fmt, ptr);
        check_fmt("pointer align right", "    0x1234", "{:>10}"_fmt, ptr);
        check_fmt("const int pointer", "0x10", "{}"_fmt, int_ptr);

        // several arguments of different types
        check_fmt("mixed arguments", "name: 0xbeef true", "{}: {:#x} {}"_fmt, "name", 0xbeef, true);
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

        //detail::print::cause_compile_fail<"Hello">();

        raw_puts("== print test ==\n");

        // T1: parser and printers, no hardware involved
        test_formatting();
        raw_puts("T1 formatting: ");
        raw_putdec(fmt_checks);
        raw_puts(" checks, ");
        raw_putdec(static_cast<uint64_t>(failures));
        raw_puts(" failed\n");

        // T2: kprint_sync, polled
        raw_puts("-- T2 kprint_sync (polled), next line must read: sync: 42 0xff abc true\n");
        kprint_sync("sync: {} {:#x} {} {}\n"_fmt, 42, 255, "abc", true);

        // T3: kprint, interrupt-driven transmit through sleep()/wakeup()
        raw_puts("-- T3 kprint (interrupt-driven), next line must read: irq: 42 0xff abc true\n");
        {
            const uint64_t sleeps_before = sleep_calls;
            const uint64_t irqs_before   = uart_irqs;
            const bool intr_before       = riscv::intr_enabled();

            kprint("irq: {} {:#x} {} {}\n"_fmt, 42, 255, "abc", true);

            check("T3 kprint() leaves the interrupt state as it found it", riscv::intr_enabled() == intr_before);
            check("T3 kprint() went through the TX interrupt path (sleep/wakeup)", sleep_calls > sleeps_before);
            check("T3 UART interrupts were delivered through the PLIC", uart_irqs > irqs_before);
        }

        // T4: more characters than any FIFO holds, nothing may be dropped or reordered
        raw_puts("-- T4 wide fields, next line: \"right\" right-aligned in 40 columns, '|', \"left\" left-aligned in 40 columns, '|'\n");
        kprint("{:>40}|{:<40}|\n"_fmt, "right", "left");
        check("T4 wide kprint() returned", true);

        // T5: kprint with an explicit uart argument
        raw_puts("-- T5 explicit uart, next line must read: explicit uart: 0x00000000deadbeef\n");
        kprint(ns16550a::uart0, "explicit uart: {:#018x}\n"_fmt, uint64_t{ 0xdeadbeef });
        check("T5 kprint(uart, ...) returned", true);

        check("no stray PLIC interrupts", stray_irqs == 0);

        raw_puts(failures == 0 ? "ALL PASS\n" : "SOME CHECKS FAILED\n");
        finish(failures == 0);
    }
} // namespace xv6
