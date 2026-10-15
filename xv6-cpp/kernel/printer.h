#pragma once

#include "types.h"

#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>

#include "uart.h"

namespace xv6::detail::print {

    template<size_t N>
    struct fixed_string {
        static_assert(N != 0, "Atleast null character is required");

    private:
        template<size_t... Idx>
        consteval fixed_string(const char (&str)[N], std::index_sequence<Idx...>) : str{ str[Idx]... } {
            if (str[N - 1]) *static_cast<char*>(nullptr) = 0;
        }

    public:
        consteval fixed_string(const char (&str)[N]) : fixed_string(str, std::make_index_sequence<N>{}) {}

        consteval size_t size() const { return N; }
        constexpr char operator[](size_t idx) const { return str[idx]; }

    public:
        char str[N];
    };

    using namespace std::string_view_literals; // operator ""sv

    // prevent from compiling in runtime context (please dont define this function)
    template<fixed_string str>
    void this_function_does_not_exist_to_cause_an_error();

    template<fixed_string str>
    inline constexpr void cause_compile_fail() {
        if consteval {
            // prevent from compiling in constexpr context
            const_cast<char&>(str.str[0]) = 0;
        } else {
            this_function_does_not_exist_to_cause_an_error<str>();
        }
    }

    class print_context {
    public:
        using write_fn_t = void (*)(std::span<const char>, void*);
        constexpr print_context(write_fn_t out_fn, void* user_data) : out_fn(out_fn), user_data(user_data) {}

        void write(std::span<const char> data) const { out_fn(data, user_data); }
        void write(std::string_view str) const { write(std::span<const char>(str)); }
        void write(char c) const { write(std::span<const char>(&c, 1)); }

    private:
        write_fn_t out_fn;
        void* user_data;
    };

    // has void printer<T>::parse(std::string_view) and void printer<T>::print(const T&, const print_context&)
    template<typename T>
    class printer;


    // Supported format specifier, after the ':' of a replacement field (std::format's standard spec without the
    // 'L' flag and without nested "{}" widths):
    //     [[fill]align] [sign] ['#'] ['0'] [width] ['.' precision] [type]
    //     align: '<' left, '>' right, '^' center       sign: '+', '-', ' '
    //     integers (and char/bool with an integer type): b B d o x X c      text: s
    //     bool: s b B d o x X      char: c b B d o x X      strings: s (precision truncates)
    // Differences from std::format: a nullptr const char* prints "(null)", and 'c' on an integer accepts 0..255
    // and prints '?' for anything else, since a kernel printer cannot throw at run time.
    //
    // parse() is constexpr and reports a bad specifier with `cause_compile_fail`, so if the printer is built in a
    // constant expression a malformed specifier is a compile error. print() is const and never allocates.

    namespace builtin {
        enum class align_t : uint8_t {
            none,
            left,
            right,
            center,
        };

        enum class sign_t : uint8_t {
            minus,
            plus,
            space,
        };

        struct format_spec {
            char fill          = ' ';
            align_t align      = align_t::none;
            sign_t sign        = sign_t::minus;
            bool alternate     = false;
            bool zero          = false;
            bool has_precision = false;
            char type          = '\0'; // '\0' when no presentation type was given
            uint32_t width     = 0;
            uint32_t precision = 0;
        };

        consteval bool is_align_char(char c) {
            return c == '<' || c == '>' || c == '^';
        }

        consteval align_t to_align(char c) {
            return c == '<' ? align_t::left : (c == '>' ? align_t::right : align_t::center);
        }

        consteval uint32_t parse_number(std::string_view s, size_t& i) {
            uint64_t value = 0;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                value = value * 10 + static_cast<uint64_t>(s[i] - '0');
                if (value > 0xFFFF) cause_compile_fail<"width or precision in format specifier is too large">();
                ++i;
            }
            return static_cast<uint32_t>(value);
        }

        // `types` lists the presentation types the argument accepts; no type at all is always accepted.
        consteval format_spec parse_spec(std::string_view s, std::string_view types) {
            format_spec spec;
            size_t i = 0;

            // [[fill]align]: a fill character is only recognised when an align character follows it
            if (s.size() >= 2 && is_align_char(s[1])) {
                spec.fill  = s[0];
                spec.align = to_align(s[1]);
                i          = 2;
            } else if (!s.empty() && is_align_char(s[0])) {
                spec.align = to_align(s[0]);
                i          = 1;
            }

            if (i < s.size() && (s[i] == '+' || s[i] == '-' || s[i] == ' ')) {
                spec.sign = s[i] == '+' ? sign_t::plus : (s[i] == ' ' ? sign_t::space : sign_t::minus);
                ++i;
            }
            if (i < s.size() && s[i] == '#') {
                spec.alternate = true;
                ++i;
            }
            if (i < s.size() && s[i] == '0') {
                spec.zero = true;
                ++i;
            }

            spec.width = parse_number(s, i);

            if (i < s.size() && s[i] == '.') {
                ++i;
                const size_t start = i;
                spec.precision     = parse_number(s, i);
                if (i == start) cause_compile_fail<"missing precision after '.' in format specifier">();
                spec.has_precision = true;
            }

            if (i < s.size()) {
                spec.type = s[i++];
                if (i != s.size()) cause_compile_fail<"unexpected characters at the end of format specifier">();
                bool accepted = false;
                for (char t : types) {
                    if (t == spec.type) accepted = true;
                }
                if (!accepted) cause_compile_fail<"presentation type is not supported for this argument">();
            }
            return spec;
        }

        consteval void require_text_spec(const format_spec& spec) {
            if (spec.sign != sign_t::minus || spec.alternate || spec.zero) cause_compile_fail<"sign, '#' and '0' are not allowed for this argument">();
        }

        consteval void require_no_precision(const format_spec& spec) {
            if (spec.has_precision) cause_compile_fail<"precision is not allowed for this argument">();
        }

        // ---- output ---------------------------------------------------------------------------------------

        inline void write_fill(const print_context& ctx, char c, size_t count) {
            char chunk[16];
            for (char& b : chunk) b = c;
            while (count != 0) {
                const size_t n = count < sizeof(chunk) ? count : sizeof(chunk);
                ctx.write(std::span<const char>(chunk, n));
                count -= n;
            }
        }

        // Writes `text` padded to the field width. `default_align` applies when the spec has no alignment.
        inline void write_aligned(const print_context& ctx, const format_spec& spec, align_t default_align, std::string_view text) {
            const size_t pad    = spec.width > text.size() ? spec.width - text.size() : 0;
            const align_t align = spec.align == align_t::none ? default_align : spec.align;

            size_t left = 0;
            if (align == align_t::right)
                left = pad;
            else if (align == align_t::center)
                left = pad / 2;
            const size_t right = pad - left;

            write_fill(ctx, spec.fill, left);
            ctx.write(text);
            write_fill(ctx, spec.fill, right);
        }

        inline void print_string(const print_context& ctx, const format_spec& spec, std::string_view text) {
            if (spec.has_precision && spec.precision < text.size()) text = std::string_view(text.data(), spec.precision);
            write_aligned(ctx, spec, align_t::left, text);
        }

        inline void print_integer(const print_context& ctx, const format_spec& spec, uint64_t magnitude, bool negative) {
            if (spec.type == 'c') {
                const char c = (!negative && magnitude <= 0xFF) ? static_cast<char>(magnitude) : '?';
                write_aligned(ctx, spec, align_t::left, std::string_view(&c, 1));
                return;
            }

            unsigned base = 10;
            if (spec.type == 'b' || spec.type == 'B')
                base = 2;
            else if (spec.type == 'o')
                base = 8;
            else if (spec.type == 'x' || spec.type == 'X')
                base = 16;
            const bool upper = spec.type == 'B' || spec.type == 'X';

            // digits, least significant first (64 digits is the worst case, in binary)
            const char* alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            char digits[64];
            size_t n       = 0;
            uint64_t value = magnitude;
            do {
                digits[n++] = alphabet[value % base];
                value /= base;
            } while (value != 0);

            // text = sign and base prefix, followed by the digits (at most 3 + 64 characters)
            char text[72];
            size_t prefix = 0;
            if (negative)
                text[prefix++] = '-';
            else if (spec.sign == sign_t::plus)
                text[prefix++] = '+';
            else if (spec.sign == sign_t::space)
                text[prefix++] = ' ';
            if (spec.alternate) {
                if (base == 16 || base == 2) {
                    text[prefix++] = '0';
                    text[prefix++] = base == 16 ? (upper ? 'X' : 'x') : (upper ? 'B' : 'b');
                } else if (base == 8 && magnitude != 0) {
                    text[prefix++] = '0';
                }
            }
            for (size_t j = 0; j != n; ++j) text[prefix + j] = digits[n - 1 - j];
            const size_t total = prefix + n;

            if (spec.zero && spec.align == align_t::none) {
                // zeros go between the prefix and the digits; an explicit alignment cancels the '0' flag
                const size_t pad = spec.width > total ? spec.width - total : 0;
                ctx.write(std::span<const char>(text, prefix));
                write_fill(ctx, '0', pad);
                ctx.write(std::span<const char>(text + prefix, n));
            } else {
                write_aligned(ctx, spec, align_t::right, std::string_view(text, total));
            }
        }

        inline void print_signed(const print_context& ctx, const format_spec& spec, int64_t value) {
            const bool negative = value < 0;
            // negating in unsigned arithmetic is also correct for the most negative value
            const uint64_t magnitude = negative ? uint64_t{ 0 } - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
            print_integer(ctx, spec, magnitude, negative);
        }

        inline void print_unsigned(const print_context& ctx, const format_spec& spec, uint64_t value) {
            print_integer(ctx, spec, value, false);
        }

    } // namespace builtin


    // char: the character itself by default, or its numeric value with an integer presentation type
    template<>
    class printer<char> {
    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "bBcdoxX")) {
            builtin::require_no_precision(m_spec);
            if (m_spec.type == '\0' || m_spec.type == 'c') builtin::require_text_spec(m_spec);
        }

        void print(const char& c, const print_context& ctx) const {
            if (m_spec.type == '\0' || m_spec.type == 'c') {
                builtin::write_aligned(ctx, m_spec, builtin::align_t::left, std::string_view(&c, 1));
            } else {
                builtin::print_signed(ctx, m_spec, static_cast<int64_t>(c));
            }
        }

    private:
        builtin::format_spec m_spec;
    };

    // bool: "true"/"false" by default, or 0/1 with an integer presentation type
    template<>
    class printer<bool> {
    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "sbBdoxX")) {
            builtin::require_no_precision(m_spec);
            if (m_spec.type == '\0' || m_spec.type == 's') builtin::require_text_spec(m_spec);
        }

        void print(const bool& value, const print_context& ctx) const {
            if (m_spec.type == '\0' || m_spec.type == 's') {
                builtin::write_aligned(ctx, m_spec, builtin::align_t::left, value ? std::string_view("true", 4) : std::string_view("false", 5));
            } else {
                builtin::print_unsigned(ctx, m_spec, value ? 1 : 0);
            }
        }

    private:
        builtin::format_spec m_spec;
    };

    // const char*: null-terminated string; nullptr prints "(null)"
    template<>
    class printer<const char*> {
    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "s")) { builtin::require_text_spec(m_spec); }

        void print(const char* const& str, const print_context& ctx) const {
            if (str == nullptr) {
                builtin::print_string(ctx, m_spec, std::string_view("(null)", 6));
                return;
            }
            // counted by hand: std::string_view's constructor would call strlen, which the kernel does not have
            size_t length = 0;
            while (str[length] != '\0') ++length;
            builtin::print_string(ctx, m_spec, std::string_view(str, length));
        }

    private:
        builtin::format_spec m_spec;
    };

    template<>
    class printer<std::string_view> {
    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "s")) { builtin::require_text_spec(m_spec); }

        void print(const std::string_view& str, const print_context& ctx) const { builtin::print_string(ctx, m_spec, str); }

    private:
        builtin::format_spec m_spec;
    };

    // every other integer type; char and bool above are full specializations, so they win over this one
    template<std::integral T>
    class printer<T> {
        static_assert(sizeof(T) <= sizeof(uint64_t), "integers wider than 64 bits are not supported");

    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "bBcdoxX")) {
            builtin::require_no_precision(m_spec);
            if (m_spec.type == 'c') builtin::require_text_spec(m_spec);
        }

        void print(const T& value, const print_context& ctx) const {
            if constexpr (std::is_signed_v<T>)
                builtin::print_signed(ctx, m_spec, static_cast<int64_t>(value));
            else
                builtin::print_unsigned(ctx, m_spec, static_cast<uint64_t>(value));
        }

    private:
        builtin::format_spec m_spec;
    };

    // char*: printed exactly like const char*
    template<>
    class printer<char*> : public printer<const char*> {
    public:
        void print(char* const& str, const print_context& ctx) const { printer<const char*>::print(str, ctx); }
    };

    // Any other pointer (void*, T*, const T*, function pointers): the address as 0x-prefixed hex, like "{:#x}" of
    // the address. Accepts [[fill]align]['0'][width]['p' | 'P']; 'P' uses "0X" and uppercase digits. Sign, '#' and
    // precision are rejected, as in std::format. char* and const char* are the full specializations above and
    // win over this one; nullptr prints as 0x0.
    template<typename T>
    class printer<T*> {
    public:
        consteval printer(std::string_view spec_text) : m_spec(builtin::parse_spec(spec_text, "pP")) {
            builtin::require_no_precision(m_spec);
            if (m_spec.sign != builtin::sign_t::minus || m_spec.alternate) cause_compile_fail<"sign and '#' are not allowed for pointers">();
            m_spec.type      = m_spec.type == 'P' ? 'X' : 'x';
            m_spec.alternate = true;
        }

        void print(T* const& ptr, const print_context& ctx) const { builtin::print_unsigned(ctx, m_spec, reinterpret_cast<uintptr_t>(ptr)); }

    private:
        builtin::format_spec m_spec;
    };

    template<>
    class printer<std::nullptr_t> : public printer<const void*> {
    public:
        void print(const std::nullptr_t&, const print_context& ctx) const { printer<const void*>::print(nullptr, ctx); }
    };
} // namespace xv6::detail::print
