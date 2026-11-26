#pragma once

#include<cstdint>

namespace atlas::market_data{

using InstrumentId = std::uint32_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Timestamp = std::uint64_t;
using OrderId = std::uint64_t;

enum class Side : std::uint8_t {
    Buy , 
    Sell
};


enum class EventType : std::uint8_t {
    Trade,
    Quote,
    OrderBookUpdate,
    OrderAdd,
    OrderCancel
};

struct Trade {
    InstrumentId instrument_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};

struct Quote {
    InstrumentId instrument_id;
    Price bid_price;
    Quantity bid_quantity;
    Price ask_price;
    Quantity ask_quantity;
    Timestamp timestamp;
};

struct OrderBookUpdate {
    InstrumentId instrument_id;
    Side side;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};

struct OrderAdd {
    InstrumentId instrument_id;
    OrderId order_id;
    Side side;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};


struct OrderCancel {
    InstrumentId instrument_id;
    OrderId order_id;
    Timestamp timestamp;
};


}