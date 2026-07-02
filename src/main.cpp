#include "order_book.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>

static constexpr int NUM_ORDERS   = 100'000;
static constexpr int NUM_THREADS  = 4;

int main() {
    OrderBook book;
    std::atomic<uint64_t> next_id{1};
    std::atomic<bool> done{false};

    // Producer: feeds random limit orders
    auto producer = [&]() {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> price_dist(99.0, 101.0);
        std::uniform_int_distribution<uint32_t> qty_dist(1, 100);
        std::uniform_int_distribution<int> side_dist(0, 1);

        for (int i = 0; i < NUM_ORDERS; ++i) {
            Order o;
            o.order_id = next_id.fetch_add(1, std::memory_order_relaxed);
            o.side     = side_dist(rng) ? Side::BID : Side::ASK;
            o.price    = std::round(price_dist(rng) * 100.0) / 100.0;
            o.quantity = qty_dist(rng);
            o.type     = OrderType::LIMIT;
            book.add_order(o);
        }
        done.store(true, std::memory_order_release);
    };

    // Consumers: continuously query BBO
    auto consumer = [&]() {
        uint64_t queries = 0;
        while (!done.load(std::memory_order_acquire)) {
            auto bb = book.best_bid();
            auto ba = book.best_ask();
            (void)bb; (void)ba;
            ++queries;
        }
        std::cout << "Consumer queries: " << queries << "\n";
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < NUM_THREADS - 1; ++i)
        threads.emplace_back(consumer);
    threads.emplace_back(producer);

    for (auto& t : threads) t.join();

    std::cout << "Total orders processed: " << book.total_orders() << "\n";
    auto bb = book.best_bid();
    auto ba = book.best_ask();
    if (bb) std::cout << "Best bid: " << bb->price << " x " << bb->quantity << "\n";
    if (ba) std::cout << "Best ask: " << ba->price << " x " << ba->quantity << "\n";
    return 0;
}
