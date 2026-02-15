#include "MatchingEngine.h"

namespace engine {

std::vector<Trade> MatchingEngine::submitLimit(OrderId id, Side side, Price price, Quantity qty) {
    // Acquire from the pool — no heap allocation, just grab a pre-allocated slot
    Order* order = orderPool_.acquire(id, side, OrderType::Limit, price, qty);

    orderCount_++;

    // Try to match first
    auto result = book_.match(*order);
    tradeCount_ += result.trades.size();

    // Release filled resting orders back to the pool
    for (Order* filled : result.filledOrders) {
        orderPool_.release(filled);
    }

    // If order still has remaining quantity, add it to the book as a resting order
    if (!order->isFilled()) {
        book_.addOrder(order);
    } else {
        // Fully filled — return the slot to the pool immediately
        orderPool_.release(order);
    }

    return result.trades;
}

std::vector<Trade> MatchingEngine::submitMarket(OrderId id, Side side, Quantity qty) {
    Order* order = orderPool_.acquire(id, side, OrderType::Market, 0, qty);

    orderCount_++;

    // Market orders just match — they never rest in the book
    auto result = book_.match(*order);
    tradeCount_ += result.trades.size();

    // Release filled resting orders back to the pool
    for (Order* filled : result.filledOrders) {
        orderPool_.release(filled);
    }

    // Market orders never rest — always release back to pool
    orderPool_.release(order);

    return result.trades;
}

bool MatchingEngine::cancel(OrderId id) {
    return book_.cancelOrder(id);
}

} // namespace engine
