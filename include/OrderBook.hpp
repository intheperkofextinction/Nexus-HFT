#pragma once
#include <cstdint>
#include <array>
#include <cstring>
#include <algorithm>

struct OrderNode {
    uint64_t order_id{0};
    uint32_t price{0};
    uint32_t shares{0};
    uint8_t  side{0}; // 0 = BUY, 1 = SELL
    OrderNode* next{nullptr};
    OrderNode* prev{nullptr};
    bool active{false};
};

template <size_t MAX_ORDERS = 10000>
class L3OrderBook {
public:
    L3OrderBook() : best_bid_(0), best_ask_(UINT32_MAX) {
        nodes_.fill(OrderNode{});
        bid_heads_.fill(nullptr);
        ask_heads_.fill(nullptr);
        price_counts_.fill(0);
    }

    inline void add_order(uint64_t order_id, uint32_t price, uint32_t shares, uint8_t side) noexcept {
        size_t slot = order_id % MAX_ORDERS;
        OrderNode* node = &nodes_[slot];

        node->order_id = order_id;
        node->price = price;
        node->shares = shares;
        node->side = side;
        node->active = true;
        node->next = nullptr;
        node->prev = nullptr;

        size_t price_slot = price % 10000;
        ++price_counts_[price_slot];

        if (side == 0) { // BUY
            if (price > best_bid_) best_bid_ = price;
            link_node(&bid_heads_[price_slot], node);
        } else { // SELL
            if (price < best_ask_) best_ask_ = price;
            link_node(&ask_heads_[price_slot], node);
        }
    }

    inline void cancel_order(uint64_t order_id) noexcept {
        size_t slot = order_id % MAX_ORDERS;
        OrderNode* node = &nodes_[slot];
        if (!node->active) return;

        size_t price_slot = node->price % 10000;
        OrderNode** head = (node->side == 0) ? &bid_heads_[price_slot] : &ask_heads_[price_slot];

        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (*head == node) *head = node->next;

        node->active = false;
        if (price_counts_[price_slot] > 0) --price_counts_[price_slot];

        // Reset top-of-book levels when empty
        if (node->side == 0 && node->price == best_bid_ && *head == nullptr) {
            best_bid_ = 0;
        } else if (node->side == 1 && node->price == best_ask_ && *head == nullptr) {
            best_ask_ = UINT32_MAX;
        }
    }

    [[nodiscard]] inline uint32_t get_best_bid() const noexcept {
        return (best_bid_ == 0) ? 0 : best_bid_;
    }

    [[nodiscard]] inline uint32_t get_best_ask() const noexcept {
        return (best_ask_ == UINT32_MAX) ? 0 : best_ask_;
    }

    [[nodiscard]] inline OrderNode* get_head_at_price(uint32_t price, uint8_t side) noexcept {
        size_t price_slot = price % 10000;
        return (side == 0) ? bid_heads_[price_slot] : ask_heads_[price_slot];
    }

private:
    inline void link_node(OrderNode** head, OrderNode* node) noexcept {
        if (!*head) {
            *head = node;
        } else {
            OrderNode* curr = *head;
            while (curr->next) curr = curr->next;
            curr->next = node;
            node->prev = curr;
        }
    }

    alignas(64) std::array<OrderNode, MAX_ORDERS> nodes_;
    alignas(64) std::array<OrderNode*, 10000> bid_heads_;
    alignas(64) std::array<OrderNode*, 10000> ask_heads_;
    alignas(64) std::array<uint32_t, 10000> price_counts_;
    uint32_t best_bid_;
    uint32_t best_ask_;
};
