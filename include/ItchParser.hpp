#pragma once
#include <cstdint>
#include <cstring>
#include "RDTSCClock.hpp"

// Force 1-byte alignment so the C++ memory layout matches the exact network binary packet layout.
// No padding bytes inserted by the compiler.
#pragma pack(push, 1)
struct ITCHAddOrderPacket {
    char     message_type;       // 'A'
    uint16_t stock_locate;       // Security locate ID
    uint16_t tracking_num;       // Internal tracking ID
    uint64_t timestamp;          // Nanoseconds since midnight
    uint64_t order_id;           // Unique reference number
    char     buy_sell_indicator; // 'B' = Buy, 'S' = Sell
    uint32_t shares;             // Quantity
    char     stock[8];           // Ticker symbol (e.g., "AAPL    ")
    uint32_t price;              // Price formatted with 4 decimal places
};
#pragma pack(pop)

class ZeroCopyParser {
public:
    // Direct zero-copy parser: Converts incoming binary network bytes into our struct 
    // and converts Big-Endian network format to Little-Endian host CPU format.
    static inline bool parse_add_order(const uint8_t* raw_buffer, ITCHAddOrderPacket& out_packet) noexcept {
        // Step 1: Memory copy direct from packet buffer (compiles to efficient SIMD register moves)
        std::memcpy(&out_packet, raw_buffer, sizeof(ITCHAddOrderPacket));

        // Step 2: Convert network byte order (Big Endian) to CPU host byte order (Little Endian)
        out_packet.stock_locate = __builtin_bswap16(out_packet.stock_locate);
        out_packet.tracking_num = __builtin_bswap16(out_packet.tracking_num);
        out_packet.timestamp    = __builtin_bswap64(out_packet.timestamp);
        out_packet.order_id     = __builtin_bswap64(out_packet.order_id);
        out_packet.shares       = __builtin_bswap32(out_packet.shares);
        out_packet.price        = __builtin_bswap32(out_packet.price);

        return out_packet.message_type == 'A';
    }
};
