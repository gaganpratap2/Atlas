#pragma once

#include <variant>

#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

using MarketEvent = std::variant<
    Trade,
    Quote,
    OrderBookUpdate,
    OrderAdd,
    OrderCancel
>;

struct SequencedMarketEvent {
    SequenceNumber sequence_number;
    MarketEvent event;
};

}