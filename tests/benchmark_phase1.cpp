#include <iostream>
#include <iomanip>
#include <vector>
#include "RDTSCClock.hpp"
#include "ItchParser.hpp"
#include "PcapReader.hpp"

int main() {
    constexpr size_t PACKET_COUNT = 10000;
    
    std::vector<uint8_t> raw_stream = NetworkStreamSimulator::generate_mock_stream(PACKET_COUNT);
    const size_t packet_size = sizeof(ITCHAddOrderPacket);

    ITCHAddOrderPacket out_packet{};
    uint64_t total_cycles = 0;

    uint64_t start_cycles = RDTSCClock::now();

    for (size_t i = 0; i < PACKET_COUNT; ++i) {
        const uint8_t* raw_packet_ptr = raw_stream.data() + (i * packet_size);
        ZeroCopyParser::parse_add_order(raw_packet_ptr, out_packet);
    }

    uint64_t end_cycles = RDTSCClock::now();
    total_cycles = end_cycles - start_cycles;

    double avg_cycles_per_pkt = static_cast<double>(total_cycles) / PACKET_COUNT;

    std::cout << "--- [Phase 1: Stream Ingestion Benchmark] ---" << std::endl;
    std::cout << "Total Packets Parsed  : " << PACKET_COUNT << std::endl;
    std::cout << "Total Execution Time  : " << total_cycles << " CPU Cycles" << std::endl;
    std::cout << "Average Latency/Packet: " << std::fixed << std::setprecision(2) 
              << avg_cycles_per_pkt << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles_per_pkt)) << " ns)" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    return 0;
}
