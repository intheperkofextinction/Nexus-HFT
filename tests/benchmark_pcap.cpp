#include <iostream>
#include <iomanip>
#include "RDTSCClock.hpp"
#include "PcapEngine.hpp"
#include "ItchParser.hpp"

int main() {
    std::string pcap_file = "nasdaq_itch_sample.pcap";
    ITCHAddOrderPacket parsed_pkt{};

    std::cout << "--- [PCAP File Ingestion Benchmark] ---" << std::endl;

    uint64_t start_cycles = RDTSCClock::now();

    size_t count = PcapEngine::process_file(pcap_file, [&](const uint8_t* payload) {
        ZeroCopyParser::parse_add_order(payload, parsed_pkt);
    });

    uint64_t end_cycles = RDTSCClock::now();
    uint64_t total_cycles = end_cycles - start_cycles;

    if (count > 0) {
        double avg_cycles = static_cast<double>(total_cycles) / count;
        std::cout << "Successfully Ingested : " << count << " frames from PCAP" << std::endl;
        std::cout << "Total Reading Time    : " << total_cycles << " CPU Cycles" << std::endl;
        std::cout << "Avg Latency/Frame     : " << std::fixed << std::setprecision(2) 
                  << avg_cycles << " cycles (~" 
                  << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    } else {
        std::cerr << "Failed to read PCAP file. Did you generate it?" << std::endl;
    }
    std::cout << "---------------------------------------" << std::endl;

    return 0;
}

