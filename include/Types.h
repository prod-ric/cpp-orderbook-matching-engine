#pragma once

#include <cstdint>
#include <chrono>

namespace engine {

// === Price representation ===
// We use integers to avoid floating point errors
// Price is in "ticks" — e.g., if tick size is 0.01, then $100.50 = 10050
using Price = int64_t;
using Quantity = uint32_t;
using OrderId = uint64_t;

// === Order side ===
enum class Side : uint8_t {
    Buy,
    Sell
};

// === Order type ===
enum class OrderType : uint8_t {
    Limit,
    Market
};

// === Timestamp ===
using Timestamp = std::chrono::steady_clock::time_point;

inline Timestamp now() {
    return std::chrono::steady_clock::now();
}

} // namespace engine
