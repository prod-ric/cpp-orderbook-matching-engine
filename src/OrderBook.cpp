#include "OrderBook.h"
#include <iostream>
#include <iomanip>

namespace engine {

// === Add an order to the resting book ===
void OrderBook::addOrder(Order* order) {
    if (order->side == Side::Buy) {
        addToBids(order);
    } else {
        addToAsks(order);
    }
    orderLookup_[order->id] = order;
}

void OrderBook::addToBids(Order* order) {
    // If no price level exists yet, create one
    auto it = bids_.find(order->price);
    if (it == bids_.end()) {
        auto [newIt, _] = bids_.emplace(order->price, PriceLevel(order->price));
        it = newIt;
    }
    it->second.addOrder(order);
}

void OrderBook::addToAsks(Order* order) {
    auto it = asks_.find(order->price);
    if (it == asks_.end()) {
        auto [newIt, _] = asks_.emplace(order->price, PriceLevel(order->price));
        it = newIt;
    }
    it->second.addOrder(order);
}

// === Cancel an order ===
bool OrderBook::cancelOrder(OrderId id) {
    auto it = orderLookup_.find(id);
    if (it == orderLookup_.end()) {
        return false; // order not found
    }

    Order* order = it->second;

    if (order->side == Side::Buy) {
        auto levelIt = bids_.find(order->price);
        if (levelIt != bids_.end()) {
            levelIt->second.removeOrder(order);
            if (levelIt->second.empty()) {
                bids_.erase(levelIt); // remove empty price level
            }
        }
    } else {
        auto levelIt = asks_.find(order->price);
        if (levelIt != asks_.end()) {
            levelIt->second.removeOrder(order);
            if (levelIt->second.empty()) {
                asks_.erase(levelIt);
            }
        }
    }

    orderLookup_.erase(it);
    return true;
}

// === Matching logic ===
MatchResult OrderBook::match(Order& incomingOrder) {
    if (incomingOrder.side == Side::Buy) {
        return matchBuy(incomingOrder);
    } else {
        return matchSell(incomingOrder);
    }
}

// Incoming BUY matches against resting ASKS (sells)
// A buy matches if the buy price >= ask price
MatchResult OrderBook::matchBuy(Order& buyOrder) {
    MatchResult result;

    // Walk through asks from lowest price up
    while (!asks_.empty() && buyOrder.remaining > 0) {
        auto& [askPrice, level] = *asks_.begin(); // best (lowest) ask

        // Check if prices cross — can we match?
        if (buyOrder.type == OrderType::Limit && buyOrder.price < askPrice) {
            break; // buy price too low, no more matches possible
        }

        // Match against orders at this price level (FIFO)
        while (!level.orders.empty() && buyOrder.remaining > 0) {
            Order* restingOrder = level.orders.front();

            // Determine fill quantity
            Quantity fillQty = std::min(buyOrder.remaining, restingOrder->remaining);

            // Execute the fill
            buyOrder.fill(fillQty);
            restingOrder->fill(fillQty);
            level.totalQuantity -= fillQty;

            // Record the trade (trades happen at the resting order's price)
            result.trades.emplace_back(buyOrder.id, restingOrder->id, askPrice, fillQty);

            // If resting order is fully filled, remove it and track for pool release
            if (restingOrder->isFilled()) {
                orderLookup_.erase(restingOrder->id);
                level.orders.pop_front();
                result.filledOrders.push_back(restingOrder);
            }
        }

        // If price level is empty, remove it
        if (level.empty()) {
            asks_.erase(asks_.begin());
        }
    }

    return result;
}

// Incoming SELL matches against resting BIDS (buys)
// A sell matches if the sell price <= bid price
MatchResult OrderBook::matchSell(Order& sellOrder) {
    MatchResult result;

    while (!bids_.empty() && sellOrder.remaining > 0) {
        auto& [bidPrice, level] = *bids_.begin(); // best (highest) bid

        if (sellOrder.type == OrderType::Limit && sellOrder.price > bidPrice) {
            break;
        }

        while (!level.orders.empty() && sellOrder.remaining > 0) {
            Order* restingOrder = level.orders.front();

            Quantity fillQty = std::min(sellOrder.remaining, restingOrder->remaining);

            sellOrder.fill(fillQty);
            restingOrder->fill(fillQty);
            level.totalQuantity -= fillQty;

            result.trades.emplace_back(restingOrder->id, sellOrder.id, bidPrice, fillQty);

            if (restingOrder->isFilled()) {
                orderLookup_.erase(restingOrder->id);
                level.orders.pop_front();
                result.filledOrders.push_back(restingOrder);
            }
        }

        if (level.empty()) {
            bids_.erase(bids_.begin());
        }
    }

    return result;
}

// === Market data ===
std::optional<Price> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    auto bid = bestBid();
    auto ask = bestAsk();
    if (bid && ask) return *ask - *bid;
    return std::nullopt;
}

// === Debug printing ===
void OrderBook::printBook(int depth) const {
    std::cout << "\n========== ORDER BOOK ==========\n";

    // Print asks (reversed so highest price is on top)
    std::vector<std::pair<Price, const PriceLevel*>> askLevels;
    int count = 0;
    for (const auto& [price, level] : asks_) {
        if (count++ >= depth) break;
        askLevels.push_back({price, &level});
    }

    for (auto it = askLevels.rbegin(); it != askLevels.rend(); ++it) {
        std::cout << "  ASK  " << std::setw(8) << it->first
                  << "  |  qty: " << std::setw(6) << it->second->totalQuantity
                  << "  |  orders: " << it->second->orders.size() << "\n";
    }

    std::cout << "  -------- spread: ";
    if (auto s = spread()) {
        std::cout << *s;
    } else {
        std::cout << "N/A";
    }
    std::cout << " --------\n";

    count = 0;
    for (const auto& [price, level] : bids_) {
        if (count++ >= depth) break;
        std::cout << "  BID  " << std::setw(8) << price
                  << "  |  qty: " << std::setw(6) << level.totalQuantity
                  << "  |  orders: " << level.orders.size() << "\n";
    }

    std::cout << "================================\n\n";
}

} // namespace engine
