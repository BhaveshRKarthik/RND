#pragma once

#include <array>

#include "config.h"
#include "context.h"
#include "types.h"

namespace xv6 {
    class cpu : private utils::immovable<cpu> {
    public:
        static void push_intr_off();
        static void pop_intr_off();
        static cpu& current();

    public:
        static uint8_t panicking, panicked;

    private:
        cpu() = default;

    private:
        static std::array<cpu, cpu_count> m_cpus;

    private:
        void* m_proc      = nullptr;
        context m_context = {};
        uint32_t m_noff   = 0;
        bool m_intena     = true;
    };

} // namespace xv6
