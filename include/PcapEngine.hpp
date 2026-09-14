#pragma once
#include <pcap.h>
#include <iostream>
#include <functional>
#include "ItchParser.hpp"

class PcapEngine {
public:
    static size_t process_file(const std::string& filename, std::function<void(const uint8_t*)> callback) {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t* handle = pcap_open_offline(filename.c_str(), errbuf);

        if (!handle) {
            std::cerr << "Error opening PCAP file: " << errbuf << std::endl;
            return 0;
        }

        struct pcap_pkthdr* header;
        const u_char* packet_data;
        size_t processed_count = 0;

        // 42-byte offset: Ethernet (14) + IP (20) + UDP (8)
        constexpr size_t UDP_OFFSET = 42; 

        while (pcap_next_ex(handle, &header, &packet_data) == 1) {
            if (header->caplen >= UDP_OFFSET + sizeof(ITCHAddOrderPacket)) {
                const uint8_t* itch_payload = packet_data + UDP_OFFSET;
                callback(itch_payload);
                ++processed_count;
            }
        }

        pcap_close(handle);
        return processed_count;
    }
};

