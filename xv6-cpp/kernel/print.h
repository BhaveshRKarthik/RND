#pragma once

#include <array>
#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>

#include "printer.h"
#include "types.h"
#include "uart.h"

namespace xv6 {
    namespace detail::print {

        struct token {
            uint32_t arg_idx;
            uint32_t offset;
            uint32_t len;
        };

        // The parser reports every token it finds to a sink. The first pass uses counting_sink to learn how many
        // tokens there are, the second uses array_sink to store them in an array of exactly that size.
        struct counting_sink {
            size_t count = 0;
            constexpr void push(const token&) { ++count; }
        };

        template<size_t Count>
        struct array_sink {
            std::array<token, Count> tokens{};
            size_t count = 0;
            constexpr void push(const token& t) { tokens[count++] = t; }
            constexpr ~array_sink() {
                if (count != Count) {
                    cause_compile_fail<"array_sink count mismatch!">();
                }
            }
        };

        template<size_t N, typename Sink>
        consteval void parse_format(const fixed_string<N>& format, size_t num_args, Sink& sink) {
            enum class indexing {
                unknown,
                automatic,
                manual,
            };

            const size_t len = N - 1; // the terminating null is not part of the format

            // the literal token that is currently being built; invariant: open => lit_end == i
            bool open        = false;
            size_t lit_begin = 0;
            size_t lit_end   = 0;

            indexing mode      = indexing::unknown;
            uint32_t next_auto = 1; // number of replacement fields seen so far + 1

            auto push = [&](uint32_t arg_idx, size_t offset, size_t length) {
                sink.push(token{ arg_idx, static_cast<uint32_t>(offset), static_cast<uint32_t>(length) });
            };

            auto flush_literal = [&] {
                if (!open) return;
                push(0, lit_begin, lit_end - lit_begin);
                open = false;
            };

            // Parses the replacement field starting at `begin` (format[begin] == '{'), emits its token and
            // returns the index just past the closing '}'. A field is '{' [digits] [':' spec] '}', where
            // spec contains no '{' and ends at the first '}'.
            auto parse_block = [&](size_t begin) -> size_t {
                size_t p = begin + 1;

                bool has_index = false;
                uint64_t index = 0;
                while (p < len && format[p] >= '0' && format[p] <= '9') {
                    has_index = true;
                    index     = index * 10 + static_cast<uint64_t>(format[p] - '0');
                    if (index >= 0xFFFF'FFFF) {
                        cause_compile_fail<"argument index in format string is too large">();
                    }
                    ++p;
                }
                if (p >= len) {
                    cause_compile_fail<"unterminated '{' in format string">();
                }

                size_t spec_offset = p; // index of '}' when there is no specifier
                size_t spec_length = 0;
                if (format[p] == ':') {
                    ++p;
                    spec_offset = p;
                    while (p < len && format[p] != '}') {
                        if (format[p] == '{') {
                            cause_compile_fail<"'{' is not allowed inside a replacement field">();
                        }
                        ++p;
                    }
                    if (p >= len) {
                        cause_compile_fail<"unterminated '{' in format string">();
                    }
                    spec_length = p - spec_offset;
                } else if (format[p] != '}') {
                    cause_compile_fail<"invalid argument id in replacement field (expected digits, ':' or '}')">();
                }
                // format[p] == '}'

                uint32_t arg_idx = 0;
                if (has_index) {
                    if (mode == indexing::automatic) {
                        cause_compile_fail<"cannot mix automatic and manual argument indexing">();
                    }
                    mode    = indexing::manual;
                    arg_idx = static_cast<uint32_t>(index + 1);
                } else {
                    if (mode == indexing::manual) {
                        cause_compile_fail<"cannot mix automatic and manual argument indexing">();
                    }
                    mode    = indexing::automatic;
                    arg_idx = next_auto++;
                }
                if (arg_idx > num_args) {
                    cause_compile_fail<"format string refers to an argument that was not provided">();
                }

                push(arg_idx, spec_offset, spec_length);
                return p + 1;
            };

            size_t i = 0;
            while (i < len) {
                const char c = format[i];

                if (c != '{' && c != '}') {
                    // ordinary character: open a literal token or extend the open one
                    if (!open) {
                        open      = true;
                        lit_begin = i;
                    }
                    ++i;
                    lit_end = i;
                    continue;
                }

                // a run of k identical braces
                size_t k = 1;
                while (i + k < len && format[i + k] == c) ++k;
                const size_t half = k / 2;
                const bool odd    = (k % 2) != 0;

                if (c == '}' && odd) {
                    cause_compile_fail<"unmatched '}' in format string">();
                }

                const bool was_open = open;
                if (!was_open) {
                    // start of a token: keep the second half of the pairs, [k/2, (k/2)*2)
                    if (half != 0) {
                        open      = true;
                        lit_begin = i + half;
                        lit_end   = i + 2 * half;
                    }
                } else {
                    // end of a literal region: keep the first half of the pairs, [0, k/2)
                    lit_end = i + half;
                }

                if (c == '{' && odd) {
                    // the last '{' opens a replacement field, so the literal built so far ends right before it
                    flush_literal();
                    i = parse_block(i + 2 * half);
                } else {
                    // even run: after a region that was already open, the skipped second halves leave a gap,
                    // so that token ends here; at the start of a token it stays open and keeps growing
                    if (was_open) flush_literal();
                    i += k;
                }
            }
            flush_literal();
        }

        // Pass 1: the minimum number of tokens the format string breaks into. All validation errors are
        // reported here, because this runs first.
        template<size_t N>
        consteval size_t count_tokens(const fixed_string<N>& format, size_t num_args) {
            counting_sink sink;
            parse_format(format, num_args, sink);
            return sink.count;
        }

        // Pass 2: the tokens themselves, in an array of exactly Count elements (Count == count_tokens(...)).
        template<size_t Count, size_t N>
        consteval std::array<token, Count> make_tokens(const fixed_string<N>& format, size_t num_args) {
            array_sink<Count> sink;
            parse_format(format, num_args, sink);
            return sink.tokens;
        }

        template<size_t N, fixed_string<N> format, size_t num_args>
        class format_parser {
            static_assert(N - 1 <= 0xFFFF'FFFF, "format string is too long for 32-bit token offsets");

        public:
            format_parser() = delete;

            // minimum number of tokens the format string breaks into
            static constexpr size_t token_count = count_tokens(format, num_args);

            static constexpr std::array<token, token_count> tokens = make_tokens<token_count>(format, num_args);

            // the text a token refers to: the literal for arg_idx == 0, the format specifier otherwise
            static consteval std::string_view text(const token& t) { return std::string_view(format.str + t.offset, t.len); }
        };

        // Carries a format string in its type. Created by operator""_fmt, consumed by kprint().
        struct format_string_tag {};

        template<fixed_string F>
        struct format_string : format_string_tag {
            // the parser for this string when it is used with NumArgs arguments
            template<size_t NumArgs>
            using parser = format_parser<F.size(), F, NumArgs>;
        };

        // String literals arrive as arrays (char[N]); they are printed through printer<const char*>.
        template<typename T>
        using printer_key_t = std::conditional_t<std::is_array_v<T>, const std::remove_extent_t<T>*, T>;

        // The I-th (zero-based) argument of the pack.
        template<size_t I, typename T, typename... Rest>
        constexpr decltype(auto) nth_arg(const T& first, const Rest&... rest) {
            if constexpr (I == 0)
                return (first);
            else
                return nth_arg<I - 1>(rest...);
        }

        // Emits token I of Parser. The token is a compile-time constant, so the argument it refers to, and
        // therefore the printer type, is resolved at compile time; no runtime dispatch is left.
        template<typename Parser, size_t I, typename... Args>
        void print_token(const print_context& ctx, [[maybe_unused]] const Args&... args) {
            constexpr token t = Parser::tokens[I];
            if constexpr (t.arg_idx == 0) {
                ctx.write(Parser::text(t));
            } else {
                const auto& arg = nth_arg<t.arg_idx - 1>(args...);

                constexpr auto p = printer<printer_key_t<std::remove_cvref_t<decltype(arg)>>>(Parser::text(t));
                p.print(arg, ctx);
            }
        }

        template<typename Parser, size_t... I, typename... Args>
        void print_tokens([[maybe_unused]] const print_context& ctx, std::index_sequence<I...>, [[maybe_unused]] const Args&... args) {
            (print_token<Parser, I>(ctx, args...), ...);
        }

        // Formats `args` according to `fmt` and writes the result to ctx. Format errors (unmatched braces, bad
        // index, too few arguments, ...) are compile errors from format_parser.
        template<typename Fmt, typename... Args>
        requires std::is_base_of_v<format_string_tag, Fmt> void format_to(const print_context& ctx, Fmt, const Args&... args) {
            using parser = typename Fmt::template parser<sizeof...(Args)>;
            print_tokens<parser>(ctx, std::make_index_sequence<parser::token_count>{}, args...);
        }

        // Where kernel output goes. Polled and lock-free, so it is usable from traps and panic, but output of
        // two harts can interleave per character.
        template<uintptr_t uart_base_addr>
        inline void console_write_sync(std::span<const char> data, void* uart_ptr) {
            auto& uart = *reinterpret_cast<ns16550a::UART<uart_base_addr>*>(uart_ptr);
            for (char c : data) uart.putc_sync(c);
        }

        template<uintptr_t uart_base_addr>
        inline void console_write(std::span<const char> data, void* uart_ptr) {
            auto& uart = *reinterpret_cast<ns16550a::UART<uart_base_addr>*>(uart_ptr);
            uart.write(data);
        }
    } // namespace detail::print


    // `_fmt` is visible inside namespace xv6, and elsewhere through `using namespace xv6::literals;`
    inline namespace literals {
        inline namespace print_literals {
            template<detail::print::fixed_string S>
            consteval auto operator""_fmt() {
                return detail::print::format_string<S>{};
            }
        }
    } // namespace literals

    template<uintptr_t uart_base_addr, std::derived_from<detail::print::format_string_tag> Fmt, typename... Args>
    void kprint(ns16550a::UART<uart_base_addr>& uart, Fmt fmt, const Args&... args) {
        const detail::print::print_context ctx(detail::print::console_write<uart_base_addr>, &uart);
        detail::print::format_to(ctx, fmt, args...);
    }

    template<std::derived_from<detail::print::format_string_tag> Fmt, typename... Args>
    void kprint(Fmt fmt, const Args&... args) {
        kprint(ns16550a::uart0, fmt, args...);
    }

    template<uintptr_t uart_base_addr, std::derived_from<detail::print::format_string_tag> Fmt, typename... Args>
    void kprint_sync(ns16550a::UART<uart_base_addr>& uart, Fmt fmt, const Args&... args) {
        const detail::print::print_context ctx(detail::print::console_write_sync<uart_base_addr>, &uart);
        detail::print::format_to(ctx, fmt, args...);
    }

    template<std::derived_from<detail::print::format_string_tag> Fmt, typename... Args>
    void kprint_sync(Fmt fmt, const Args&... args) {
        kprint_sync(ns16550a::uart0, fmt, args...);
    }
} // namespace xv6
