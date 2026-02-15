#include "MatchingEngine.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

using namespace engine;

// Helper to measure a single operation in nanoseconds
template <typename Func>
long long timeNs(Func&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

void printStats(const std::string& label, std::vector<long long>& latencies) {
    if (latencies.empty()) {
        std::cout << "  " << label << ": no data\n";
        return;
    }

    std::sort(latencies.begin(), latencies.end());

    long long sum = std::accumulate(latencies.begin(), latencies.end(), 0LL);
    double avg = static_cast<double>(sum) / latencies.size();

    std::cout << "  " << label << ":\n";
    std::cout << "    Count:   " << latencies.size() << "\n";
    std::cout << "    Avg:     " << static_cast<int>(avg) << " ns\n";
    std::cout << "    Median:  " << latencies[latencies.size() / 2] << " ns\n";
    std::cout << "    p95:     " << latencies[static_cast<size_t>(latencies.size() * 0.95)] << " ns\n";
    std::cout << "    p99:     " << latencies[static_cast<size_t>(latencies.size() * 0.99)] << " ns\n";
    std::cout << "    Max:     " << latencies.back() << " ns\n";
    std::cout << "\n";
}

int main() {
    const int NUM_ORDERS = 1'000'000;
    std::mt19937 rng(42);

    // Tighter price range = more matches = more interesting
    std::uniform_int_distribution<Price> priceDist(9900, 10100);
    std::uniform_int_distribution<Quantity> qtyDist(1, 100);
    std::uniform_int_distribution<int> sideDist(0, 1);

    // ============================================================
    // BENCHMARK 1: Overall throughput
    // ============================================================
    std::cout << "=== Benchmark 1: Overall Throughput ===\n\n";
    {
        MatchingEngine engine;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_ORDERS; ++i) {
            Side side = sideDist(rng) == 0 ? Side::Buy : Side::Sell;
            engine.submitLimit(static_cast<OrderId>(i), side, priceDist(rng), qtyDist(rng));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        double ordersPerSec = (static_cast<double>(NUM_ORDERS) / durationUs) * 1'000'000;
        std::cout << "  Orders:     " << NUM_ORDERS << "\n";
        std::cout << "  Trades:     " << engine.totalTrades() << "\n";
        std::cout << "  Time:       " << durationUs / 1000 << " ms\n";
        std::cout << "  Throughput: " << static_cast<int>(ordersPerSec) << " orders/sec\n\n";
    }

    // ============================================================
    // BENCHMARK 2: Per-operation latency distribution
    // ============================================================
    std::cout << "=== Benchmark 2: Latency Distribution ===\n\n";
    {
        MatchingEngine engine;
        std::vector<long long> insertLatencies;
        std::vector<long long> matchLatencies;

        insertLatencies.reserve(NUM_ORDERS);
        matchLatencies.reserve(NUM_ORDERS);

        rng.seed(42);

        for (int i = 0; i < NUM_ORDERS; ++i) {
            Side side = sideDist(rng) == 0 ? Side::Buy : Side::Sell;
            Price price = priceDist(rng);
            Quantity qty = qtyDist(rng);

            std::vector<Trade> trades;
            long long ns = timeNs([&]() {
                trades = engine.submitLimit(static_cast<OrderId>(i), side, price, qty);
            });

            if (trades.empty()) {
                insertLatencies.push_back(ns);
            } else {
                matchLatencies.push_back(ns);
            }
        }

        printStats("Insert (no match)", insertLatencies);
        printStats("Match (produced trades)", matchLatencies);
    }

    // ============================================================
    // BENCHMARK 3: Cancel latency
    // ============================================================
    std::cout << "=== Benchmark 3: Cancel Latency ===\n\n";
    {
        MatchingEngine engine;
        const int NUM_CANCEL = 100'000;

        for (int i = 0; i < NUM_CANCEL; ++i) {
            engine.submitLimit(static_cast<OrderId>(i), Side::Buy, 9000 + (i % 100), 50);
        }
        for (int i = 0; i < NUM_CANCEL; ++i) {
            engine.submitLimit(static_cast<OrderId>(NUM_CANCEL + i), Side::Sell, 11000 + (i % 100), 50);
        }

        std::cout << "  Book size: " << engine.book().orderCount() << " orders\n";

        std::vector<long long> cancelLatencies;
        cancelLatencies.reserve(NUM_CANCEL);

        for (int i = 0; i < NUM_CANCEL; ++i) {
            long long ns = timeNs([&]() {
                engine.cancel(static_cast<OrderId>(i));
            });
            cancelLatencies.push_back(ns);
        }

        printStats("Cancel", cancelLatencies);
    }

    // ============================================================
    // BENCHMARK 4: Heap vs contiguous memory
    // ============================================================
    std::cout << "=== Benchmark 4: Memory Allocation Cost ===\n\n";
    {
        const int N = 500'000;

        // Heap: each order is a separate allocation
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::unique_ptr<Order>> heapOrders;
        heapOrders.reserve(N);
        for (int i = 0; i < N; ++i) {
            heapOrders.push_back(std::make_unique<Order>(i, Side::Buy, OrderType::Limit, 10000, 50));
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto heapNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // Contiguous: all orders in one block of memory
        start = std::chrono::high_resolution_clock::now();
        std::vector<Order> contiguousOrders;
        contiguousOrders.reserve(N);
        for (int i = 0; i < N; ++i) {
            contiguousOrders.emplace_back(i, Side::Buy, OrderType::Limit, 10000, 50);
        }
        end = std::chrono::high_resolution_clock::now();
        auto contiguousNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << "  Creating " << N << " orders:\n";
        std::cout << "    Heap (unique_ptr):   " << heapNs / 1'000'000 << " ms  (" << heapNs / N << " ns/order)\n";
        std::cout << "    Contiguous (vector): " << contiguousNs / 1'000'000 << " ms  (" << contiguousNs / N << " ns/order)\n";
        std::cout << "    Speedup:             " << std::fixed << std::setprecision(1) << static_cast<double>(heapNs) / contiguousNs << "x\n\n";
    }

    // ============================================================
    // BENCHMARK 5: Impact of book depth on std::map
    // ============================================================
    std::cout << "=== Benchmark 5: Impact of Book Depth ===\n\n";
    {
        for (int depth : {100, 1000, 10000, 50000}) {
            MatchingEngine engine;

            for (int i = 0; i < depth; ++i) {
                engine.submitLimit(static_cast<OrderId>(i), Side::Buy, 5000 + i, 50);
                engine.submitLimit(static_cast<OrderId>(depth + i), Side::Sell, 15000 + i, 50);
            }

            const int TRIALS = 100'000;
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < TRIALS; ++i) {
                OrderId id = static_cast<OrderId>(2 * depth + i);
                if (i % 2 == 0) {
                    engine.submitLimit(id, Side::Buy, 5000 + (i % depth), 10);
                } else {
                    engine.submitLimit(id, Side::Sell, 15000 + (i % depth), 10);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto avgNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / TRIALS;

            std::cout << "  Depth " << std::setw(6) << depth
                      << " levels:  " << avgNs << " ns/insert\n";
        }
        std::cout << "\n";
    }

    return 0;
}
