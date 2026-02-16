#pragma once

#include "Types.h"
#include <string>
#include <list>

namespace engine {

struct Order {
    OrderId id;
    Side side;
    OrderType type;
    Price price;          // in ticks (ignored for market orders)
    Quantity quantity;     // original quantity
    Quantity remaining;    // how much is left to fill
    Timestamp timestamp;

    // added a bookPosition to track the order's position in the order book for efficient cancellations
    std::list<Order*>::iterator bookPosition;

    // Constructor for a new order
    Order(OrderId id, Side side, OrderType type, Price price, Quantity quantity)
        : id(id)
        , side(side)
        , type(type)
        , price(price)
        , quantity(quantity)
        , remaining(quantity)
        , timestamp(now())
    {}

    // Is this order fully filled?
    bool isFilled() const { return remaining == 0; }

    // Fill some quantity, returns how much was actually filled
    Quantity fill(Quantity qty) {
        Quantity filled = std::min(qty, remaining);
        remaining -= filled;
        return filled;
    }
};

} // namespace engine
