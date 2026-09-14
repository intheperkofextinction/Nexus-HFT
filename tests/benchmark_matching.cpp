#include <iostream>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "OrderBook.hpp"
#include "MatchingEngine.hpp"

int main() {
    L3OrderBook<10000> book;
    MatchingEngine matcher(book);
    std::vector<ExecutionReport> fills;
    fills.reserve(1000);

    // 1. Pre-populate passive Resting Orders on the Ask side
    // Sell 100 shares @ $150.00 (1500000), $150.05 (1500500), $150.10 (1501000)
    book.add_order(101, 1500000, 100, 1); // Ask
    book.add_order(102, 1500500, 200, 1); // Ask
    book.add_order(103, 1501000, 300, 1); // Ask

    constexpr size_t MATCH_RUNS = 100000;
    uint64_t start_cycles = RDTSCClock::now();

    // 2. Cross the book with aggressive BUY order sweeping liquidity
    for (size_t i = 0; i < MATCH_RUNS; ++i) {
        // Incoming aggressive BUY @ $150.05 for 150 shares
        fills.clear();
        matcher.match_order(2000 + i, 1500500, 150, 0, fills);
        
        // Reset book state for loop iteration
        book.cancel_order(2000 + i);
        if (book.get_best_ask() == 0) {
            book.add_order(101, 1500000, 100, 1);
            book.add_order(102, 1500500, 200, 1);
        }
    }

    uint64_t end_cycles = RDTSCClock::now();
    uint64_t total_cycles = end_cycles - start_cycles;
    double avg_cycles = static_cast<double>(total_cycles) / MATCH_RUNS;

    std::cout << "--- [Module 2: Matching Engine & Price-Cross Benchmark] ---" << std::endl;
    std::cout << "Total Orders Matched/Tested : " << MATCH_RUNS << std::endl;
    std::cout << "Total Trade Executions      : " << matcher.get_total_executions() << std::endl;
    std::cout << "Matching Engine Latency     : " << std::fixed << std::setprecision(2) 
              << avg_cycles << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

    return 0;
}
