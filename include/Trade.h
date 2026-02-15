#pragma once

#include "Types.h"
#include <iostream>

namespace engine {

struct Trade {
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price price;
    Quantity quantity;
    Timestamp timestamp;

    Trade(OrderId buyId, OrderId sellId, Price price, Quantity qty)
        : buyOrderId(buyId)
        , sellOrderId(sellId)
        , price(price)
        , quantity(qty)
        , timestamp(now())
    {}

    // Print trade for debugging
    friend std::ostream& operator<<(std::ostream& os, const Trade& trade) {
        os << "Trade: buyer=" << trade.buyOrderId
           << " seller=" << trade.sellOrderId
           << " price=" << trade.price
           << " qty=" << trade.quantity;
        return os;
    }
};

} // namespace engine
