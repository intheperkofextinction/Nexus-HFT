#include <iostream>
#include <thread>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "SPSCQueue.hpp"

struct Message {
    uint64_t timestamp;
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
};

int main() {
    constexpr size_t ITERATIONS = 100000;
    // Ring buffer size must be power of 2
    SPSCQueue<Message, 1024> queue;

    uint64_t start_cycles = 0;
    uint64_t end_cycles = 0;

    // Consumer Thread Function
    std::thread consumer([&]() {
        Message msg;
        size_t popped = 0;
        while (popped < ITERATIONS) {
            if (queue.pop(msg)) {
                ++popped;
            } else {
                asm volatile("pause" ::: "memory"); // Low latency CPU yield hint
            }
        }
        end_cycles = RDTSCClock::now();
    });

    // Producer Thread (Hot Path)
    start_cycles = RDTSCClock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        Message msg{RDTSCClock::now(), 1000 + i, 15000, 100};
        while (!queue.push(msg)) {
            asm volatile("pause" ::: "memory");
        }
    }

    consumer.join();

    uint64_t total_cycles = end_cycles - start_cycles;
    double avg_cycles = static_cast<double>(total_cycles) / ITERATIONS;

    std::cout << "--- [Phase 3: SPSC Lock-Free Queue Inter-Thread Benchmark] ---" << std::endl;
    std::cout << "Total Messages Passed : " << ITERATIONS << std::endl;
    std::cout << "Total Execution Time  : " << total_cycles << " CPU Cycles" << std::endl;
    std::cout << "Average Latency/Msg   : " << std::fixed << std::setprecision(2) 
              << avg_cycles << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    return 0;
}
