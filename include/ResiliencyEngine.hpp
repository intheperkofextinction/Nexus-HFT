#pragma once
#include <cstdint>
#include <array>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

// MoldUDP64 Header (20 bytes)
#pragma pack(push, 1)
struct MoldUDP64Header {
    char     session[10];
    uint64_t sequence_number;
    uint16_t message_count;
};
#pragma pack(pop)

template <size_t MAX_GAP_BUFFER_SIZE = 1024>
class ResiliencyEngine {
public:
    struct StashedPacket {
        uint64_t sequence_num{0};
        uint16_t payload_len{0};
        uint8_t  payload[128]; // Max ITCH packet size
        bool     valid{false};
    };

    ResiliencyEngine() : expected_sequence_(1), gap_count_(0), dropped_count_(0) {
        gap_buffer_.fill(StashedPacket{});
    }

    // Hot Path Packet Process
    template <typename ProcessFunc>
    inline void process_packet(const uint8_t* raw_packet, size_t len, ProcessFunc&& on_in_order_payload) noexcept {
        if (len < sizeof(MoldUDP64Header)) return;

        const auto* header = reinterpret_cast<const MoldUDP64Header*>(raw_packet);
        uint64_t seq = __builtin_bswap64(header->sequence_number);
        uint16_t count = __builtin_bswap16(header->message_count);

        if (count == 0) return; // Heartbeat packet

        const uint8_t* payload_ptr = raw_packet + sizeof(MoldUDP64Header);
        uint16_t payload_len = len - sizeof(MoldUDP64Header);

        // Scenario 1: Perfect In-Order Packet
        if (__builtin_expect(seq == expected_sequence_, 1)) {
            on_in_order_payload(payload_ptr, payload_len);
            ++expected_sequence_;

            // Drain any contiguous packets sitting in gap buffer
            drain_gap_buffer(on_in_order_payload);
            return;
        }

        // Scenario 2: Duplicate / Stale Packet
        if (seq < expected_sequence_) {
            ++dropped_count_;
            return;
        }

        // Scenario 3: Out-of-Order / Gap Detected
        if (seq > expected_sequence_) {
            ++gap_count_;
            size_t slot = seq % MAX_GAP_BUFFER_SIZE;
            
            if (payload_len <= 128) {
                gap_buffer_[slot].sequence_num = seq;
                gap_buffer_[slot].payload_len = payload_len;
                std::memcpy(gap_buffer_[slot].payload, payload_ptr, payload_len);
                gap_buffer_[slot].valid = true;
            }
        }
    }

    [[nodiscard]] uint64_t get_expected_seq() const noexcept { return expected_sequence_; }
    [[nodiscard]] uint64_t get_gap_count() const noexcept { return gap_count_; }
    [[nodiscard]] uint64_t get_dropped_count() const noexcept { return dropped_count_; }

private:
    template <typename ProcessFunc>
    inline void drain_gap_buffer(ProcessFunc&& on_in_order_payload) noexcept {
        while (true) {
            size_t slot = expected_sequence_ % MAX_GAP_BUFFER_SIZE;
            if (!gap_buffer_[slot].valid || gap_buffer_[slot].sequence_num != expected_sequence_) {
                break;
            }

            // Fire callback on backfilled gap packet
            on_in_order_payload(gap_buffer_[slot].payload, gap_buffer_[slot].payload_len);
            gap_buffer_[slot].valid = false;
            ++expected_sequence_;
        }
    }

    alignas(64) uint64_t expected_sequence_;
    alignas(64) uint64_t gap_count_;
    alignas(64) uint64_t dropped_count_;
    std::array<StashedPacket, MAX_GAP_BUFFER_SIZE> gap_buffer_;
};

