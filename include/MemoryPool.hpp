#pragma once
#include <cstdint>
#include <array>
#include <cstddef>
#include <new>

// 64-Byte Cache-Line Aligned Order Node
struct alignas(64) OrderNode {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    uint32_t next_idx;    // Intrusive index pointer for doubly linked list
    uint32_t prev_idx;    // Intrusive index pointer
    uint8_t  side;        // 0 = Buy, 1 = Sell
    char     padding[35]; // Enforces 64-byte structural size matching L1 Cache Line
};

template <std::size_t MaxOrders>
class MemoryPool {
public:
    MemoryPool() noexcept {
        // Pre-link all slots into an O(1) free-list stack at startup
        for (std::size_t i = 0; i < MaxOrders - 1; ++i) {
            pool_[i].next_idx = static_cast<uint32_t>(i + 1);
        }
        pool_[MaxOrders - 1].next_idx = FREE_LIST_END;
        free_head_ = 0;
    }

    // O(1) Allocation - Zero Syscalls
    inline OrderNode* allocate() noexcept {
        if (free_head_ == FREE_LIST_END) [[unlikely]] {
            return nullptr; // Pool exhausted
        }
        uint32_t allocated_idx = free_head_;
        free_head_ = pool_[allocated_idx].next_idx;
        return &pool_[allocated_idx];
    }

    // O(1) Deallocation - Returns node back to pre-allocated arena
    inline void deallocate(OrderNode* ptr) noexcept {
        uint32_t idx = static_cast<uint32_t>(ptr - pool_.data());
        ptr->next_idx = free_head_;
        free_head_ = idx;
    }

    inline uint32_t get_index(const OrderNode* ptr) const noexcept {
        return static_cast<uint32_t>(ptr - pool_.data());
    }

    inline OrderNode* get_node(uint32_t idx) noexcept {
        return &pool_[idx];
    }

private:
    static constexpr uint32_t FREE_LIST_END = 0xFFFFFFFF;
    alignas(64) std::array<OrderNode, MaxOrders> pool_;
    uint32_t free_head_{0};
};
