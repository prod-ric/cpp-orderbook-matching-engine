#pragma once

#include "OrderBook.h"
#include "Order.h"
#include "Trade.h"
#include "ObjectPool.h"

#include <vector>

namespace engine {

class MatchingEngine {
public:
    // Pre-allocate pool at construction — default 2 million order slots
    explicit MatchingEngine(size_t poolSize = 2'000'000)
        : orderPool_(poolSize)
    {}

    // Submit a new limit order — returns any trades that occurred
    std::vector<Trade> submitLimit(OrderId id, Side side, Price price, Quantity qty);

    // Submit a market order — returns any trades that occurred
    std::vector<Trade> submitMarket(OrderId id, Side side, Quantity qty);

    // Cancel an existing order
    bool cancel(OrderId id);

    // Access the book (for printing, market data, etc.)
    const OrderBook& book() const { return book_; }
    OrderBook& book() { return book_; }

    // Stats
    size_t totalTrades() const { return tradeCount_; }
    size_t totalOrders() const { return orderCount_; }

private:
    OrderBook book_;

    // Pre-allocated memory pool — no heap allocation during trading
    ObjectPool<Order> orderPool_;

    size_t tradeCount_ = 0;
    size_t orderCount_ = 0;
};

} // namespace engine
