#pragma once

#include "context.h"
#include "types.h"
#include <array>


namespace xv6 {
    constexpr size_t NCPU = 8; // maximum number of CPUs

    struct cpu : private utils::immovable {
    public:
        static void push_intr_off();
        static void pop_intr_off();
        static cpu& current();

    private:
        static std::array<cpu, NCPU> m_cpus;

        void* m_proc      = nullptr;
        context m_context = {};
        uint32_t m_noff   = 0;
        bool m_intena     = true;
    };

} // namespace xv6
