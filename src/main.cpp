#include "MatchingEngine.h"
#include <iostream>

using namespace engine;

int main() {
    MatchingEngine engine;

    std::cout << "=== Matching Engine Demo ===\n\n";

    // --- Step 1: Build up the order book with some resting orders ---
    std::cout << "Step 1: Adding resting limit orders...\n";

    // Sell orders (asks)
    engine.submitLimit(1, Side::Sell, 10200, 50);   // Sell 50 @ 102.00
    engine.submitLimit(2, Side::Sell, 10150, 30);   // Sell 30 @ 101.50
    engine.submitLimit(3, Side::Sell, 10100, 100);  // Sell 100 @ 101.00
    engine.submitLimit(4, Side::Sell, 10100, 40);   // Sell 40 @ 101.00 (second order at same price)

    // Buy orders (bids)
    engine.submitLimit(5, Side::Buy, 10000, 75);    // Buy 75 @ 100.00
    engine.submitLimit(6, Side::Buy, 9950, 20);     // Buy 20 @ 99.50
    engine.submitLimit(7, Side::Buy, 9900, 40);     // Buy 40 @ 99.00

    engine.book().printBook();

    // --- Step 2: Send in a buy order that crosses the spread ---
    std::cout << "Step 2: Aggressive buy — 80 @ 101.00 (should match against asks)\n";

    auto trades = engine.submitLimit(8, Side::Buy, 10100, 80);

    for (const auto& trade : trades) {
        std::cout << "  " << trade << "\n";
    }

    engine.book().printBook();

    // --- Step 3: Send a market order ---
    std::cout << "Step 3: Market sell — 50 (should match against best bid)\n";

    trades = engine.submitMarket(9, Side::Sell, 50);

    for (const auto& trade : trades) {
        std::cout << "  " << trade << "\n";
    }

    engine.book().printBook();

    // --- Step 4: Cancel an order ---
    std::cout << "Step 4: Cancel order 7 (Buy 40 @ 99.00)\n";

    bool cancelled = engine.cancel(7);
    std::cout << "  Cancelled: " << (cancelled ? "yes" : "no") << "\n";

    engine.book().printBook();

    // --- Stats ---
    std::cout << "Total orders processed: " << engine.totalOrders() << "\n";
    std::cout << "Total trades executed: " << engine.totalTrades() << "\n";

    return 0;
}
