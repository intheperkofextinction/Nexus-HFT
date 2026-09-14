#include <iostream>
#include <vector>
#include <iomanip>
#include "ResiliencyEngine.hpp"
#include "RDTSCClock.hpp"
#include "ItchParser.hpp"

int main() {
    ResiliencyEngine<1024> resiliency;
    size_t processed_count = 0;

    auto callback = [&](const uint8_t* payload, uint16_t len) {
        (void)payload;
        (void)len;
        ++processed_count;
    };

    std::cout << "--- [Module 1: MoldUDP64 Sequence Tracking & Resiliency Benchmark] ---" << std::endl;

    // Build raw packet buffers
    constexpr size_t total_packets = 10000;
    struct DummyPacket {
        MoldUDP64Header header;
        ITCHAddOrderPacket payload;
    };

    std::vector<DummyPacket> packets(total_packets);

    for (size_t i = 0; i < total_packets; ++i) {
        std::memcpy(packets[i].header.session, "SESSION001", 10);
        packets[i].header.sequence_number = __builtin_bswap64(i + 1); // 1-indexed seq
        packets[i].header.message_count   = __builtin_bswap16(1);
    }

    uint64_t start_cycles = RDTSCClock::now();

    // 1. Process 1000 in-order packets
    for (size_t i = 0; i < 1000; ++i) {
        resiliency.process_packet(reinterpret_cast<const uint8_t*>(&packets[i]), sizeof(DummyPacket), callback);
    }

    // 2. Simulate Out-of-Order Arrival: Deliver Packet 1003, 1002, then missing 1001
    resiliency.process_packet(reinterpret_cast<const uint8_t*>(&packets[1002]), sizeof(DummyPacket), callback); // Seq 1003
    resiliency.process_packet(reinterpret_cast<const uint8_t*>(&packets[1001]), sizeof(DummyPacket), callback); // Seq 1002
    resiliency.process_packet(reinterpret_cast<const uint8_t*>(&packets[1000]), sizeof(DummyPacket), callback); // Seq 1001 (triggers gap drain!)

    // 3. Process remaining packets
    for (size_t i = 1003; i < total_packets; ++i) {
        resiliency.process_packet(reinterpret_cast<const uint8_t*>(&packets[i]), sizeof(DummyPacket), callback);
    }

    uint64_t end_cycles = RDTSCClock::now();
    uint64_t total_cycles = end_cycles - start_cycles;
    double avg_cycles = static_cast<double>(total_cycles) / total_packets;

    std::cout << "Total Packets Processed   : " << processed_count << std::endl;
    std::cout << "Detected Gap Events       : " << resiliency.get_gap_count() << std::endl;
    std::cout << "Total Cycle Time          : " << total_cycles << " CPU Cycles" << std::endl;
    std::cout << "Resiliency Overhead/Packet: " << std::fixed << std::setprecision(2) 
              << avg_cycles << " cycles (~" 
              << RDTSCClock::cycles_to_ns(static_cast<uint64_t>(avg_cycles)) << " ns)" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    return 0;
}
