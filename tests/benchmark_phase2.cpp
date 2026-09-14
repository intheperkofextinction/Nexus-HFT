#include <iostream>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "OrderBook.hpp"

int main() {
    constexpr size_t TEST_ORDERS = 10000;
    L3OrderBook<TEST_ORDERS + 100> order_book;

    uint64_t start_cycles = RDTSCClock::now();

    for (size_t i = 0; i < TEST_ORDERS; ++i) {
        order_book.add_order(1000 + i, 15000 + (i % 50), 100, i % 2);
        // Prevent compiler optimization of loop body
        asm volatile("" : : : "memory");
    }

    uint64_t end_cycles = RDTSCClock::now();
    uint64_t total_cycles = end_cycles - start_cycles;
    double avg_cycles = static_cast<double>(total_cycles) / TEST_ORDERS;

    std::cout << "--- [Phase 2: Zero-Allocation Memory Pool & L3 Book Benchmark] ---" << std::endl;
    std::cout << "Total Orders Inserted : " << TEST_ORDERS << std::endl;
    std::cout << "Total Execution Time  : " << total_cycles << " CPU Cycles" << std::endl;
    std::cout << "Average Insertion Cost: " << std::fixed << std::setprecision(2) 
              << avg_cycles << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    std::cout << "Node Size (aligned)   : " << sizeof(OrderNode) << " bytes" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    return 0;
}

