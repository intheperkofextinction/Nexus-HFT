#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "OrderBook.hpp"
#include "MatchingEngine.hpp"

int main() {
    L3OrderBook<10000> book;
    MatchingEngine matcher(book);
    std::vector<ExecutionReport> fills;
    fills.reserve(100);

    constexpr size_t NUM_SAMPLES = 100000;
    constexpr size_t WARMUP_RUNS = 10000;
    std::vector<uint64_t> latencies_cycles;
    latencies_cycles.reserve(NUM_SAMPLES);

    // Warmup phase: Prime I-Cache and CPU branch predictors
    for (size_t i = 0; i < WARMUP_RUNS; ++i) {
        book.add_order(101, 1500000, 100, 1);
        matcher.match_order(2000 + i, 1500000, 100, 0, fills);
        book.cancel_order(2000 + i);
    }

    // Benchmark Phase
    for (size_t i = 0; i < NUM_SAMPLES; ++i) {
        book.add_order(101, 1500000, 100, 1);

        uint64_t t0 = RDTSCClock::now();
        matcher.match_order(5000000 + i, 1500000, 100, 0, fills);
        uint64_t t1 = RDTSCClock::now();

        latencies_cycles.push_back(t1 - t0);
        book.cancel_order(5000000 + i);
    }

    // Sort to calculate percentiles
    std::sort(latencies_cycles.begin(), latencies_cycles.end());

    auto get_pct = [&](double pct) {
        size_t idx = static_cast<size_t>((pct / 100.0) * NUM_SAMPLES);
        return latencies_cycles[std::min(idx, NUM_SAMPLES - 1)];
    };

    std::cout << "--- [Module 4: Tail Latency Percentile Profile (100k Runs)] ---" << std::endl;
    std::cout << "p50   (Median) : " << get_pct(50.0)  << " cycles (~" << RDTSCClock::cycles_to_ns(get_pct(50.0))  << " ns)" << std::endl;
    std::cout << "p90            : " << get_pct(90.0)  << " cycles (~" << RDTSCClock::cycles_to_ns(get_pct(90.0))  << " ns)" << std::endl;
    std::cout << "p99            : " << get_pct(99.0)  << " cycles (~" << RDTSCClock::cycles_to_ns(get_pct(99.0))  << " ns)" << std::endl;
    std::cout << "p99.9          : " << get_pct(99.9)  << " cycles (~" << RDTSCClock::cycles_to_ns(get_pct(99.9))  << " ns)" << std::endl;
    std::cout << "Max Latency    : " << latencies_cycles.back() << " cycles (~" << RDTSCClock::cycles_to_ns(latencies_cycles.back()) << " ns)" << std::endl;
    std::cout << "---------------------------------------------------------------" << std::endl;

    return 0;
}
