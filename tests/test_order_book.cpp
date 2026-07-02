#include "order_book.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(OrderBookTest, AddBidAsk) {
    OrderBook book;
    EXPECT_TRUE(book.add_order({1, Side::BID, 100.0, 10}));
    EXPECT_TRUE(book.add_order({2, Side::ASK, 101.0, 5}));
    auto bb = book.best_bid();
    auto ba = book.best_ask();
    ASSERT_TRUE(bb.has_value());
    ASSERT_TRUE(ba.has_value());
    EXPECT_DOUBLE_EQ(bb->price, 100.0);
    EXPECT_DOUBLE_EQ(ba->price, 101.0);
    EXPECT_EQ(bb->quantity, 10u);
    EXPECT_EQ(ba->quantity, 5u);
}

TEST(OrderBookTest, CancelOrder) {
    OrderBook book;
    book.add_order({1, Side::BID, 100.0, 10});
    EXPECT_TRUE(book.cancel_order(1));
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.cancel_order(999));
}

TEST(OrderBookTest, ModifyOrder) {
    OrderBook book;
    book.add_order({1, Side::BID, 100.0, 10});
    EXPECT_TRUE(book.modify_order(1, 25));
    auto bb = book.best_bid();
    ASSERT_TRUE(bb.has_value());
    EXPECT_EQ(bb->quantity, 25u);
}

TEST(OrderBookTest, MultiplePriceLevels) {
    OrderBook book;
    book.add_order({1, Side::BID, 100.0, 10});
    book.add_order({2, Side::BID, 100.5, 5});  // better bid
    book.add_order({3, Side::ASK, 101.0, 8});
    book.add_order({4, Side::ASK, 101.5, 3});
    EXPECT_DOUBLE_EQ(book.best_bid()->price, 100.5);
    EXPECT_DOUBLE_EQ(book.best_ask()->price, 101.0);
}

TEST(OrderBookTest, InvalidOrder) {
    OrderBook book;
    EXPECT_FALSE(book.add_order({1, Side::BID, -1.0, 10}));
    EXPECT_FALSE(book.add_order({2, Side::ASK, 100.0, 0}));
}

TEST(OrderBookTest, ConcurrentStress) {
    OrderBook book;
    constexpr int N = 50'000;
    std::vector<std::thread> threads;

    // Writers
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < N / 4; ++i) {
                uint64_t id = static_cast<uint64_t>(t * N + i + 1);
                book.add_order({id, (i % 2 == 0) ? Side::BID : Side::ASK,
                                100.0 + (i % 10) * 0.01,
                                static_cast<uint32_t>(i % 100 + 1)});
            }
        });
    }
    // Readers
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < N; ++i) {
                (void)book.best_bid();
                (void)book.best_ask();
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_GT(book.total_orders(), 0u);
}
