#pragma once
#include <cstdint>
#include <x86intrin.h>

class RDTSCClock {
public:
    // Reads the CPU Time-Stamp Counter using __rdtscp.
    // __rdtscp serializes execution to prevent the CPU from reordering instructions 
    // across the measurement boundary.
    static inline uint64_t now() noexcept {
        unsigned int aux;
        return __rdtscp(&aux);
    }

    // Utility to convert raw cycles into approximate nanoseconds
    // You can adjust cpu_ghz to match your CPU clock speed (e.g., 3.5 GHz)
    static constexpr double cycles_to_ns(uint64_t cycles, double cpu_ghz = 3.5) noexcept {
        return static_cast<double>(cycles) / cpu_ghz;
    }
};
