#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include "ItchParser.hpp"

// PCAP Global Header
struct pcap_hdr {
    uint32_t magic_number{0xa1b2c3d4};
    uint16_t version_major{2};
    uint16_t version_minor{4};
    int32_t  thiszone{0};
    uint32_t sigfigs{0};
    uint32_t snaplen{65535};
    uint32_t network{1}; // Ethernet
};

// PCAP Packet Record Header
struct pcaprec_hdr {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

int main() {
    std::ofstream pcap_file("nasdaq_itch_sample.pcap", std::ios::binary);
    
    pcap_hdr ghdr{};
    pcap_file.write(reinterpret_cast<const char*>(&ghdr), sizeof(ghdr));

    constexpr size_t PACKET_COUNT = 5000;
    
    // 14 bytes Ethernet + 20 bytes IP + 8 bytes UDP = 42 bytes network header
    constexpr uint32_t net_header_len = 42;
    constexpr uint32_t payload_len = sizeof(ITCHAddOrderPacket);
    constexpr uint32_t total_pkt_len = net_header_len + payload_len;

    std::vector<uint8_t> frame(total_pkt_len, 0);

    // Mock Network Headers (Ethernet / IP / UDP)
    // Frame bytes 0-13: Ethernet, 14-33: IP, 34-41: UDP Header
    frame[12] = 0x08; frame[13] = 0x00; // IPv4
    frame[23] = 17; // UDP Protocol

    for (size_t i = 0; i < PACKET_COUNT; ++i) {
        pcaprec_hdr phdr{};
        phdr.ts_sec = 1700000000 + static_cast<uint32_t>(i / 1000);
        phdr.ts_usec = (i % 1000) * 1000;
        phdr.incl_len = total_pkt_len;
        phdr.orig_len = total_pkt_len;

        pcap_file.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));

        // Construct ITCH Payload
        ITCHAddOrderPacket pkt{};
        pkt.message_type = 'A';
        pkt.stock_locate = __builtin_bswap16(static_cast<uint16_t>(i % 5));
        pkt.tracking_num = __builtin_bswap16(static_cast<uint16_t>(i));
        pkt.timestamp    = __builtin_bswap64(1000000000ULL + i);
        pkt.order_id     = __builtin_bswap64(500000 + i);
        pkt.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
        pkt.shares       = __builtin_bswap32(100 * ((i % 10) + 1));
        pkt.price        = __builtin_bswap32(1800000 + static_cast<uint32_t>(i * 5));
        std::memcpy(pkt.stock, "AAPL    ", 8);

        std::memcpy(frame.data() + net_header_len, &pkt, sizeof(ITCHAddOrderPacket));
        pcap_file.write(reinterpret_cast<const char*>(frame.data()), total_pkt_len);
    }

    std::cout << "Successfully generated 'nasdaq_itch_sample.pcap' with " 
              << PACKET_COUNT << " binary network frames." << std::endl;
    return 0;
}
