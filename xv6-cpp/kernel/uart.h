#pragma once

#include <span>
#include <type_traits>

#include "config.h"
#include "flag_enum.h"
#include "spinlock.h"
#include "types.h"

void sleep(void* chan, xv6::spinlock&);
void wakeup(void* chan);

namespace ns16550a {

    namespace detail::uart {
        enum class uart_access_type {
            read_write,
            read_only,
            write_only,
        };

        struct no_fields {
            using flag_type = uint8_t;
        };

        template<typename Fields>
        concept valid_fields_type = std::is_same_v<typename Fields::flag_type, uint8_t> ||
                                    (std::is_same_v<std::underlying_type_t<typename Fields::flag_type>, uint8_t>); // is_enum_v subsumed indirectly

    } // namespace detail::uart

    using detail::uart::uart_access_type;

    template<uintptr_t Addr, uart_access_type Access, detail::uart::valid_fields_type Fields = detail::uart::no_fields>
    struct Reg : Fields, private utils::immovable<Reg<Addr, Access, Fields>> {
        using flag_type = typename Fields::flag_type;

        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }

        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
        void operator=(flag_type value) noexcept requires(Access != uart_access_type::read_only) { write(value); }

    private:
        static volatile uint8_t* ptr() { return reinterpret_cast<volatile uint8_t*>(Addr); }
    };

    // interrupt enable register
    struct IER {
        enum flag_type : uint8_t {
            disabled = 0,
            // receiver interrupts
            rx_enable = 1 << 0,
            // transmit interrupts
            tx_enable = 1 << 1,
        };
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    };

    // FIFO control register
    struct FCR {
        enum flag_type : uint8_t {
            fifo_enable = 1 << 0,
            // clear the content of the two FIFOs
            fifo_clear = 3 << 1,
        };
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    };

    // line control register
    struct LCR {
        enum flag_type : uint8_t {
            eight_bits = 3 << 0,
            // special mode to set baud rate
            baud_latch = 1 << 7,
        };
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    };

    // line status register
    struct LSR {
        enum flag_type : uint8_t {
            // input is waiting to be read from RHR
            rx_ready = 1 << 0,
            // THR can accept another character to send
            tx_idle = 1 << 5
        };
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    };

    template<uintptr_t base_addr>
    class UART : utils::immovable<UART<base_addr>> {
    public:
        UART(const char* uartname) : m_tx_lock(uartname) {
            // disable interrupts
            IER = IER::disabled;
            // special mode to set baud rate
            LCR = LCR::baud_latch;
            // baud rate setup for 38.4K
            DLL = 0x03;
            DLM = 0x00;
            // leave set-baud mode, and set word length to 8 bits, no parity.
            LCR = LCR::eight_bits;
            // reset and enable FIFOs.
            FCR = FCR::fifo_enable | FCR::fifo_clear;
            // enable transmit and receive interrupts.
            IER = IER::tx_enable | IER::rx_enable;
        }

        void write(std::span<const char> buf) {
            with_spinlock(m_tx_lock) {
                for (char c : buf) {
                    while (m_tx_busy) {
                        sleep(&m_tx_chan, m_tx_lock);
                    }
                    THR       = static_cast<uint8_t>(c);
                    m_tx_busy = true;
                }
            }
        }

        void putc_sync(char c) {
            while (!(LSR.read() & LSR::tx_idle));
            THR = static_cast<uint8_t>(c);
        }

        size_t read(std::span<char> buf) {
            size_t written_count = 0;
            for (char& c : buf) {
                const int r = getc();
                if (r < 0) break;
                c = static_cast<char>(r);
                written_count++;
            }
            return written_count;
        }

        int getc() {
            // is input ready?
            if (LSR.read() & LSR::rx_ready) return RHR.read();
            return -1;
        }

        void intr() {
            (void)ISR.read();
            with_spinlock(m_tx_lock) {
                if (LSR.read() & LSR::tx_idle) {
                    m_tx_busy = false;
                    wakeup(&m_tx_chan);
                }
            }

            for (int c = getc(); c != -1; c = getc()) consoleintr(c);
        }

    private:
        // receive holding register (for input bytes)
        [[no_unique_address]] Reg<base_addr + 0, uart_access_type::read_only> RHR = {};
        // transmit holding register (for output bytes)
        [[no_unique_address]] Reg<base_addr + 0, uart_access_type::write_only> THR = {};
        // divisor latch, low byte [LSB] (only while lcr.dlab is set)
        [[no_unique_address]] Reg<base_addr + 0, uart_access_type::read_write> DLL = {};


        // interrupt enable register
        [[no_unique_address]] Reg<base_addr + 1, uart_access_type::read_write, struct IER> IER = {};
        // divisor latch, high byte [MSB] (only while lcr.dlab is set)
        [[no_unique_address]] Reg<base_addr + 1, uart_access_type::read_write> DLM = {};

        // interrupt status register
        [[no_unique_address]] Reg<base_addr + 2, uart_access_type::read_only> ISR = {};
        // FIFO control register
        [[no_unique_address]] Reg<base_addr + 2, uart_access_type::write_only, struct FCR> FCR = {};


        // line control register
        [[no_unique_address]] Reg<base_addr + 3, uart_access_type::read_write, struct LCR> LCR = {};

        // line status register
        [[no_unique_address]] Reg<base_addr + 5, uart_access_type::read_only, struct LSR> LSR = {};

    private:
        bool m_tx_busy = false;
        char m_tx_chan = 0;
        xv6::spinlock m_tx_lock;
    };

    extern UART<UART0_BASE> uart0;
} // namespace ns16550a
