#include "MatchingEngine.h"
#include <iostream>
#include <cassert>

using namespace engine;

// Simple test framework
int testsPassed = 0;
int testsFailed = 0;

void check(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "  PASS: " << testName << "\n";
        testsPassed++;
    } else {
        std::cout << "  FAIL: " << testName << "\n";
        testsFailed++;
    }
}

void testBasicLimitOrder() {
    std::cout << "\n--- Test: Basic Limit Order ---\n";
    MatchingEngine engine;

    auto trades = engine.submitLimit(1, Side::Buy, 10000, 50);
    check(trades.empty(), "Buy order with empty book produces no trades");
    check(engine.book().orderCount() == 1, "Order is resting in book");
    check(engine.book().bestBid().value() == 10000, "Best bid is correct");
}

void testSimpleMatch() {
    std::cout << "\n--- Test: Simple Match ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Buy, 10000, 50);
    auto trades = engine.submitLimit(2, Side::Sell, 10000, 50);

    check(trades.size() == 1, "One trade produced");
    check(trades[0].quantity == 50, "Full quantity traded");
    check(trades[0].price == 10000, "Trade at correct price");
    check(engine.book().orderCount() == 0, "Book is empty after full match");
}

void testPartialFill() {
    std::cout << "\n--- Test: Partial Fill ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Buy, 10000, 100);
    auto trades = engine.submitLimit(2, Side::Sell, 10000, 30);

    check(trades.size() == 1, "One trade produced");
    check(trades[0].quantity == 30, "Partial fill quantity correct");
    check(engine.book().orderCount() == 1, "Remaining order still in book");
    check(engine.book().bestBid().value() == 10000, "Best bid still exists");
}

void testPriceTimePriority() {
    std::cout << "\n--- Test: Price-Time Priority ---\n";
    MatchingEngine engine;

    // Two sells at same price — first one should match first
    engine.submitLimit(1, Side::Sell, 10000, 50);  // arrived first
    engine.submitLimit(2, Side::Sell, 10000, 50);  // arrived second

    auto trades = engine.submitLimit(3, Side::Buy, 10000, 50);

    check(trades.size() == 1, "One trade produced");
    check(trades[0].sellOrderId == 1, "First sell order matched (time priority)");
}

void testPricePriority() {
    std::cout << "\n--- Test: Price Priority ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Sell, 10100, 50);  // worse price
    engine.submitLimit(2, Side::Sell, 10000, 50);  // better price

    auto trades = engine.submitLimit(3, Side::Buy, 10100, 50);

    check(trades.size() == 1, "One trade produced");
    check(trades[0].sellOrderId == 2, "Better-priced sell matched first");
    check(trades[0].price == 10000, "Trade at resting order's price");
}

void testNoMatchWhenPricesDontCross() {
    std::cout << "\n--- Test: No Match When Prices Don't Cross ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Sell, 10100, 50);
    auto trades = engine.submitLimit(2, Side::Buy, 10000, 50);

    check(trades.empty(), "No trades when buy < ask");
    check(engine.book().orderCount() == 2, "Both orders resting");
    check(engine.book().spread().value() == 100, "Spread is 1.00 (100 ticks)");
}

void testMarketOrder() {
    std::cout << "\n--- Test: Market Order ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Sell, 10000, 50);
    engine.submitLimit(2, Side::Sell, 10100, 50);

    auto trades = engine.submitMarket(3, Side::Buy, 75);

    check(trades.size() == 2, "Market order matched across two price levels");
    check(trades[0].quantity == 50, "First fill: 50 @ 100.00");
    check(trades[1].quantity == 25, "Second fill: 25 @ 101.00");
}

void testCancel() {
    std::cout << "\n--- Test: Cancel Order ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Buy, 10000, 50);
    check(engine.book().orderCount() == 1, "Order in book");

    bool cancelled = engine.cancel(1);
    check(cancelled, "Cancel returned true");
    check(engine.book().orderCount() == 0, "Book is empty after cancel");

    bool cancelAgain = engine.cancel(1);
    check(!cancelAgain, "Can't cancel same order twice");
}

void testMultipleFillsAtSameLevel() {
    std::cout << "\n--- Test: Multiple Fills at Same Level ---\n";
    MatchingEngine engine;

    engine.submitLimit(1, Side::Sell, 10000, 30);
    engine.submitLimit(2, Side::Sell, 10000, 40);
    engine.submitLimit(3, Side::Sell, 10000, 50);

    auto trades = engine.submitLimit(4, Side::Buy, 10000, 100);

    check(trades.size() == 3, "Three trades (one per resting order)");
    check(trades[0].quantity == 30, "First order fully filled");
    check(trades[1].quantity == 40, "Second order fully filled");
    check(trades[2].quantity == 30, "Third order partially filled");
    check(engine.book().orderCount() == 1, "One sell order remains");
}

int main() {
    std::cout << "=== Matching Engine Tests ===\n";

    testBasicLimitOrder();
    testSimpleMatch();
    testPartialFill();
    testPriceTimePriority();
    testPricePriority();
    testNoMatchWhenPricesDontCross();
    testMarketOrder();
    testCancel();
    testMultipleFillsAtSameLevel();

    std::cout << "\n=== Results: " << testsPassed << " passed, "
              << testsFailed << " failed ===\n";

    return testsFailed > 0 ? 1 : 0;
}
