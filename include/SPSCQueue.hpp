#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <new>

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2 for fast bitwise modulo");
public:
    SPSCQueue() : head_(0), tail_(0) {}

    // Producer Thread Enqueue (Hot-Path)
    inline bool push(const T& item) noexcept {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        if (capacity_ - (current_tail - head_cache_) == 0) {
            head_cache_ = head_.load(std::memory_order_acquire);
            if (capacity_ - (current_tail - head_cache_) == 0) [[unlikely]] {
                return false; // Queue full
            }
        }
        buffer_[current_tail & mask_] = item;
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    // Consumer Thread Dequeue (Hot-Path)
    inline bool pop(T& item) noexcept {
        const auto current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_cache_) {
            tail_cache_ = tail_.load(std::memory_order_acquire);
            if (current_head == tail_cache_) [[unlikely]] {
                return false; // Queue empty
            }
        }
        item = buffer_[current_head & mask_];
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

private:
    static constexpr std::size_t mask_ = Capacity - 1;
    static constexpr std::size_t capacity_ = Capacity;

    alignas(64) std::array<T, Capacity> buffer_;

    // Prevent False Sharing: Enforce 64-byte alignment on atomic indices
    alignas(64) std::atomic<std::size_t> head_;
    std::size_t head_cache_{0};

    alignas(64) std::atomic<std::size_t> tail_;
    std::size_t tail_cache_{0};
};
