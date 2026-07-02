#include "order_book.hpp"
#include <stdexcept>

OrderBook::OrderBook() = default;

bool OrderBook::add_order(const Order& order) {
    if (order.quantity == 0 || order.price <= 0.0) return false;

    {
        std::unique_lock lock(mtx_);
        orders_[order.order_id] = order;
        auto& level = (order.side == Side::BID)
            ? bids_[order.price]
            : asks_[order.price];
        level.price     = order.price;
        level.quantity += order.quantity;
        level.count    += 1;
    }
    total_orders_.fetch_add(1, std::memory_order_relaxed);
    update_bbo_atomic(order.side);
    return true;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    std::unique_lock lock(mtx_);
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return false;

    const Order& o = it->second;
    auto& map = (o.side == Side::BID) ? (std::map<double,PriceLevel,std::greater<double>>&)bids_
                                        : (std::map<double,PriceLevel,std::greater<double>>&)asks_;
    // Use the right map per side
    if (o.side == Side::BID) {
        auto lit = bids_.find(o.price);
        if (lit != bids_.end()) {
            lit->second.quantity -= o.quantity;
            lit->second.count    -= 1;
            if (lit->second.count == 0) bids_.erase(lit);
        }
    } else {
        auto lit = asks_.find(o.price);
        if (lit != asks_.end()) {
            lit->second.quantity -= o.quantity;
            lit->second.count    -= 1;
            if (lit->second.count == 0) asks_.erase(lit);
        }
    }
    Side side = o.side;
    orders_.erase(it);
    lock.unlock();
    update_bbo_atomic(side);
    return true;
}

bool OrderBook::modify_order(uint64_t order_id, uint32_t new_quantity) {
    if (new_quantity == 0) return cancel_order(order_id);
    Side side;
    {
        std::unique_lock lock(mtx_);
        auto it = orders_.find(order_id);
        if (it == orders_.end()) return false;
        Order& o = it->second;
        int64_t delta = static_cast<int64_t>(new_quantity) - static_cast<int64_t>(o.quantity);
        if (o.side == Side::BID) {
            bids_[o.price].quantity = static_cast<uint32_t>(
                static_cast<int64_t>(bids_[o.price].quantity) + delta);
        } else {
            asks_[o.price].quantity = static_cast<uint32_t>(
                static_cast<int64_t>(asks_[o.price].quantity) + delta);
        }
        side = o.side;
        o.quantity = new_quantity;
    }
    update_bbo_atomic(side);
    return true;
}

std::optional<PriceLevel> OrderBook::best_bid() const {
    double p = best_bid_.price.load(std::memory_order_acquire);
    if (p <= 0.0) return std::nullopt;
    return PriceLevel{p, best_bid_.quantity.load(std::memory_order_acquire), 0};
}

std::optional<PriceLevel> OrderBook::best_ask() const {
    double p = best_ask_.price.load(std::memory_order_acquire);
    if (p <= 0.0) return std::nullopt;
    return PriceLevel{p, best_ask_.quantity.load(std::memory_order_acquire), 0};
}

std::map<double, PriceLevel, std::greater<double>> OrderBook::bid_levels() const {
    std::shared_lock lock(mtx_); return bids_;
}
std::map<double, PriceLevel> OrderBook::ask_levels() const {
    std::shared_lock lock(mtx_); return asks_;
}

void OrderBook::update_bbo_atomic(Side side) {
    std::shared_lock lock(mtx_);
    if (side == Side::BID) {
        if (!bids_.empty()) {
            const auto& [p, lv] = *bids_.begin();
            best_bid_.price.store(p, std::memory_order_release);
            best_bid_.quantity.store(lv.quantity, std::memory_order_release);
        } else {
            best_bid_.price.store(0.0, std::memory_order_release);
            best_bid_.quantity.store(0,   std::memory_order_release);
        }
    } else {
        if (!asks_.empty()) {
            const auto& [p, lv] = *asks_.begin();
            best_ask_.price.store(p, std::memory_order_release);
            best_ask_.quantity.store(lv.quantity, std::memory_order_release);
        } else {
            best_ask_.price.store(0.0, std::memory_order_release);
            best_ask_.quantity.store(0,   std::memory_order_release);
        }
    }
}
