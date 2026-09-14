#include <iostream>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "AsyncLogger.hpp"

int main() {
    constexpr size_t LOG_COUNT = 100000;
    NonBlockingLogger logger("hft_execution.bin", 2); // Offload I/O to core 2

    uint64_t start_cycles = RDTSCClock::now();

    for (size_t i = 0; i < LOG_COUNT; ++i) {
        // Hot-Path push into non-blocking logger
        while (!logger.log(RDTSCClock::now(), 5000 + i, 1502500, 100)) {
            asm volatile("pause" ::: "memory");
        }
    }

    uint64_t end_cycles = RDTSCClock::now();
    uint64_t total_cycles = end_cycles - start_cycles;
    double avg_cycles = static_cast<double>(total_cycles) / LOG_COUNT;

    std::cout << "--- [Phase 4: Non-Blocking Async Binary Logger Benchmark] ---" << std::endl;
    std::cout << "Total Binary Logs Pushed : " << LOG_COUNT << std::endl;
    std::cout << "Total Execution Time     : " << total_cycles << " CPU Cycles" << std::endl;
    std::cout << "Hot-Path Logging Latency : " << std::fixed << std::setprecision(2) 
              << avg_cycles << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    return 0;
}

