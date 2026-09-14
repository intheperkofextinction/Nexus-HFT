#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include "ItchParser.hpp"

// Class simulating network stream ingestion from raw byte buffers or file dumps
class NetworkStreamSimulator {
public:
    // Generate a stream of N ITCH 5.0 packets sequentially in memory
    static std::vector<uint8_t> generate_mock_stream(size_t packet_count) {
        std::vector<uint8_t> stream_buffer(packet_count * sizeof(ITCHAddOrderPacket));
        
        for (size_t i = 0; i < packet_count; ++i) {
            ITCHAddOrderPacket pkt{};
            pkt.message_type = 'A';
            pkt.stock_locate = __builtin_bswap16(static_cast<uint16_t>(i % 10));
            pkt.tracking_num = __builtin_bswap16(static_cast<uint16_t>(i));
            pkt.timestamp    = __builtin_bswap64(1000000000ULL + i);
            pkt.order_id     = __builtin_bswap64(100000 + i);
            pkt.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
            pkt.shares       = __builtin_bswap32(100 * ((i % 5) + 1));
            pkt.price        = __builtin_bswap32(1500000 + static_cast<uint32_t>(i * 10));
            std::memcpy(pkt.stock, "NVDA    ", 8);

            std::memcpy(stream_buffer.data() + (i * sizeof(ITCHAddOrderPacket)), &pkt, sizeof(ITCHAddOrderPacket));
        }

        return stream_buffer;
    }
};
