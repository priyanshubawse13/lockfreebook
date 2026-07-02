#pragma once
#include <cstdint>
#include <string>

enum class Side : uint8_t { BID, ASK };

enum class OrderType : uint8_t { LIMIT, CANCEL, MODIFY };

struct Order {
    uint64_t  order_id;
    Side      side;
    double    price;
    uint32_t  quantity;
    OrderType type = OrderType::LIMIT;
};

struct PriceLevel {
    double   price    = 0.0;
    uint32_t quantity = 0;   // total quantity at this level
    uint64_t count    = 0;   // number of resting orders
};
