#pragma once

#include "Types.h"
#include "Order.h"
#include "Trade.h"

#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <optional>

namespace engine {

// Result of a match operation — includes trades AND pointers to filled resting orders
// so the engine can release them back to the pool
struct MatchResult {
    std::vector<Trade> trades;
    std::vector<Order*> filledOrders;  // resting orders that were fully filled
};

// A single price level — holds all orders at one price
// Uses std::list because we need fast removal from the middle (when orders cancel)
struct PriceLevel {
    Price price;
    std::list<Order*> orders;    // FIFO queue of orders at this price
    Quantity totalQuantity = 0;  // total remaining qty at this level

    explicit PriceLevel(Price p) : price(p) {}

    void addOrder(Order* order) {
        orders.push_back(order);
        order->bookPosition = std::prev(orders.end()); // store iterator for O(1) removal
        totalQuantity += order->remaining;
    }

    void removeOrder(Order* order) {
        totalQuantity -= order->remaining;
        orders.erase(order->bookPosition);
    }

    bool empty() const { return orders.empty(); }
};

class OrderBook {
public:
    OrderBook() = default;

    // === Core operations ===
    // Add a limit order to the book (after matching is attempted)
    void addOrder(Order* order);

    // Cancel an order by ID
    bool cancelOrder(OrderId id);

    // === Matching ===
    // Try to match an incoming order against resting orders
    // Returns trades and pointers to filled resting orders
    MatchResult match(Order& incomingOrder);

    // === Market data ===
    std::optional<Price> bestBid() const;
    std::optional<Price> bestAsk() const;
    std::optional<Price> spread() const;

    // How many orders are in the book
    size_t orderCount() const { return orderLookup_.size(); }

    // How many price levels on each side
    size_t bidLevelCount() const { return bids_.size(); }
    size_t askLevelCount() const { return asks_.size(); }

    // Print the book for debugging
    void printBook(int depth = 5) const;

private:
    // Bids: sorted high to low (best bid = highest price = first)
    // std::map with std::greater gives us descending order
    std::map<Price, PriceLevel, std::greater<Price>> bids_;

    // Asks: sorted low to high (best ask = lowest price = first)
    // std::map default is ascending, which is what we want
    std::map<Price, PriceLevel> asks_;

    // Fast lookup: order ID → which side and price level it's in
    std::unordered_map<OrderId, Order*> orderLookup_;

    // Internal helpers
    MatchResult matchBuy(Order& order);
    MatchResult matchSell(Order& order);
    void addToAsks(Order* order);
    void addToBids(Order* order);
};

} // namespace engine
