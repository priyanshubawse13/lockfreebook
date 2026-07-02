#pragma once
#include "order.hpp"
#include <atomic>
#include <map>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

/**
 * OrderBook
 *
 * Lock-free BBO tracking via std::atomic<PriceLevel>.
 * Full price-level map protected by a shared_mutex for concurrent readers.
 * Order add/cancel/modify on the hot path minimise lock duration.
 */
class OrderBook {
public:
    OrderBook();

    // Returns true on success, false on invalid order
    bool add_order(const Order& order);
    bool cancel_order(uint64_t order_id);
    bool modify_order(uint64_t order_id, uint32_t new_quantity);

    // Lock-free O(1) best bid / best offer
    std::optional<PriceLevel> best_bid() const;
    std::optional<PriceLevel> best_ask() const;

    // Depth snapshot (acquires shared lock)
    std::map<double, PriceLevel, std::greater<double>> bid_levels() const;
    std::map<double, PriceLevel>                       ask_levels() const;

    uint64_t total_orders() const { return total_orders_.load(std::memory_order_relaxed); }

private:
    void update_bbo_atomic(Side side);

    // Price-level maps (guarded by shared_mutex)
    mutable std::shared_mutex mtx_;
    std::map<double, PriceLevel, std::greater<double>> bids_; // descending
    std::map<double, PriceLevel>                       asks_; // ascending
    std::unordered_map<uint64_t, Order>                orders_;

    // Lock-free BBO cache
    struct AtomicBBO {
        std::atomic<double>   price{0.0};
        std::atomic<uint32_t> quantity{0};
    };
    AtomicBBO best_bid_;
    AtomicBBO best_ask_;

    std::atomic<uint64_t> total_orders_{0};
};
