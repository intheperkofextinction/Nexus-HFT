#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <algorithm>
#include "RDTSCClock.hpp"
#include "ItchParser.hpp"
#include "PcapEngine.hpp"
#include "OrderBook.hpp"
#include "MatchingEngine.hpp"
#include "ResiliencyEngine.hpp"
#include "SPSCQueue.hpp"

struct MarketEvent {
    uint64_t timestamp;
    uint64_t order_id;
    uint32_t price;
    uint32_t shares;
    uint8_t  side;
};

void render_dashboard(size_t ingested, size_t executed, uint32_t best_bid, uint32_t best_ask, double p50_ns, double p99_ns, uint64_t gaps) {
    std::cout << "\033[2J\033[1;1H"; // Clear terminal screen
    std::cout << "========================================================================\n";
    std::cout << "                  NEXUS-HFT REAL-TIME ENGINE DASHBOARD                  \n";
    std::cout << "========================================================================\n";
    std::cout << " [MARKET DATA INGESTION]                                                \n";
    std::cout << "   Total Ingested Frames : " << std::setw(10) << ingested << " packets                   \n";
    std::cout << "   Sequence Gap Events   : " << std::setw(10) << gaps << " gaps detected              \n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << " [L3 ORDER BOOK TOP-OF-BOOK]                                            \n";
    std::cout << "   Best Bid Price        : $" << std::fixed << std::setprecision(2) << (best_bid / 10000.0) << "              \n";
    std::cout << "   Best Ask Price        : $" << std::fixed << std::setprecision(2) << (best_ask == UINT32_MAX ? 0.0 : best_ask / 10000.0) << "              \n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << " [EXECUTION & TAIL LATENCY PERFORMANCE]                                 \n";
    std::cout << "   Total Executed Trades : " << std::setw(10) << executed << " fills                     \n";
    std::cout << "   p50 Latency (Median)  : " << std::setw(10) << std::fixed << std::setprecision(2) << p50_ns << " ns                    \n";
    std::cout << "   p99 Latency (Tail)    : " << std::setw(10) << std::fixed << std::setprecision(2) << p99_ns << " ns                    \n";
    std::cout << "========================================================================\n";
}

int main() {
    std::string pcap_file = "nasdaq_itch_sample.pcap";
    SPSCQueue<MarketEvent, 8192> event_queue;
    L3OrderBook<10000> order_book;
    MatchingEngine matcher(order_book);
    ResiliencyEngine<1024> resiliency;

    std::atomic<bool> running{true};
    std::vector<ExecutionReport> fills;
    std::vector<uint64_t> latencies;
    latencies.reserve(10000);

    // Engine Worker Thread
    std::thread engine_thread([&]() {
        MarketEvent event;
        while (running) {
            if (event_queue.pop(event)) {
                uint64_t t0 = RDTSCClock::now();
                matcher.match_order(event.order_id, event.price, event.shares, event.side, fills);
                uint64_t t1 = RDTSCClock::now();

                latencies.push_back(t1 - t0);
            } else {
                asm volatile("pause" ::: "memory");
            }
        }
        // Drain remaining queue events
        while (event_queue.pop(event)) {
            uint64_t t0 = RDTSCClock::now();
            matcher.match_order(event.order_id, event.price, event.shares, event.side, fills);
            uint64_t t1 = RDTSCClock::now();
            latencies.push_back(t1 - t0);
        }
    });

    // Ingestion Feed
    ITCHAddOrderPacket parsed_pkt{};
    size_t total_ingested = PcapEngine::process_file(pcap_file, [&](const uint8_t* raw_payload) {
        ZeroCopyParser::parse_add_order(raw_payload, parsed_pkt);
        MarketEvent event{
            parsed_pkt.timestamp,
            parsed_pkt.order_id,
            parsed_pkt.price,
            parsed_pkt.shares,
            static_cast<uint8_t>(parsed_pkt.buy_sell_indicator == 'B' ? 0 : 1)
        };
        while (!event_queue.push(event)) {
            asm volatile("pause" ::: "memory");
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;
    engine_thread.join();

    // Compute Metrics
    double p50_ns = 0.0;
    double p99_ns = 0.0;
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        p50_ns = RDTSCClock::cycles_to_ns(latencies[static_cast<size_t>(latencies.size() * 0.50)]);
        p99_ns = RDTSCClock::cycles_to_ns(latencies[static_cast<size_t>(latencies.size() * 0.99)]);
    }

    render_dashboard(
        total_ingested,
        matcher.get_total_executions(),
        order_book.get_best_bid(),
        order_book.get_best_ask(),
        p50_ns,
        p99_ns,
        resiliency.get_gap_count()
    );

    return 0;
}
