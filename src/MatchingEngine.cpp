#include "MatchingEngine.h"

namespace engine {

std::vector<Trade> MatchingEngine::submitLimit(OrderId id, Side side, Price price, Quantity qty) {
    // Create the order (heap allocated, owned by us via unique_ptr)
    auto order = std::make_unique<Order>(id, side, OrderType::Limit, price, qty);
    Order* rawPtr = order.get(); // raw pointer for the book to use
    //show the pointer address for debugging
    //std::cout << "Submitting order with ID " << id << " at address " << rawPtr << "\n";
    
    orderCount_++;

    // Try to match first
    auto trades = book_.match(*rawPtr);
    tradeCount_ += trades.size();

    // If order still has remaining quantity, add it to the book as a resting order
    if (!rawPtr->isFilled()) {
        book_.addOrder(rawPtr);
    }

    // Store ownership regardless (keeps memory alive until engine is destroyed)
    orders_.push_back(std::move(order));

    return trades;
}

std::vector<Trade> MatchingEngine::submitMarket(OrderId id, Side side, Quantity qty) {
    auto order = std::make_unique<Order>(id, side, OrderType::Market, 0, qty);
    Order* rawPtr = order.get();

    orderCount_++;

    // Market orders just match — they never rest in the book
    auto trades = book_.match(*rawPtr);
    tradeCount_ += trades.size();

    // If not fully filled, the remaining quantity is lost (no resting for market orders)
    if (!rawPtr->isFilled()) {
        // In a real system you might log this or notify the sender
    }

    orders_.push_back(std::move(order));

    return trades;
}

bool MatchingEngine::cancel(OrderId id) {
    return book_.cancelOrder(id);
}

} // namespace engine
