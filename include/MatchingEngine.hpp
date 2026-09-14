#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include "OrderBook.hpp"

struct ExecutionReport {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint32_t fill_price;
    uint32_t fill_qty;
};

class MatchingEngine {
public:
    MatchingEngine(L3OrderBook<10000>& book) : book_(book), total_executions_(0) {}

    // Match aggressive limit order against passive book
    inline bool match_order(uint64_t order_id, uint32_t price, uint32_t qty, uint8_t side, std::vector<ExecutionReport>& fills) noexcept {
        // side 0 = BUY (matches against lowest ASK), side 1 = SELL (matches against highest BID)
        uint32_t remaining_qty = qty;

        while (remaining_qty > 0) {
            uint32_t best_opposing_price = (side == 0) ? book_.get_best_ask() : book_.get_best_bid();
            if (best_opposing_price == 0) break; // Opposing side empty

            // Check if prices cross (BUY price >= Best Ask OR SELL price <= Best Bid)
            bool crosses = (side == 0) ? (price >= best_opposing_price) : (price <= best_opposing_price);
            if (!crosses) break;

            // Fetch passive liquidity order at top level
            OrderNode* passive_order = book_.get_head_at_price(best_opposing_price, side ^ 1);
            if (!passive_order) break;

            uint32_t fill_qty = std::min(remaining_qty, passive_order->shares);
            
            // Generate Trade Execution
            fills.push_back(ExecutionReport{
                (side == 0) ? order_id : passive_order->order_id,
                (side == 0) ? passive_order->order_id : order_id,
                best_opposing_price,
                fill_qty
            });

            remaining_qty -= fill_qty;
            passive_order->shares -= fill_qty;
            ++total_executions_;

            // If passive order is fully filled, remove from intrusive book
            if (passive_order->shares == 0) {
                book_.cancel_order(passive_order->order_id);
            }
        }

        // If order partially filled or unfilled, rest remaining quantity in book
        if (remaining_qty > 0) {
            book_.add_order(order_id, price, remaining_qty, side);
            return false; // Rested
        }

        return true; // Fully Executed
    }

    [[nodiscard]] uint64_t get_total_executions() const noexcept { return total_executions_; }

private:
    L3OrderBook<10000>& book_;
    uint64_t total_executions_;
};
