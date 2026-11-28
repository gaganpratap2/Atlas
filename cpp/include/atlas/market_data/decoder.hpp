#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "atlas/market_data/event.hpp"

namespace atlas::market_data {

class Decoder { 
public:
    MarketEvent decode(std::span<const std::uint8_t> bytes) const;
    SequencedMarketEvent decode_sequenced( std::span<const std::uint8_t> bytes) const;
};

}  // namespace atlas::market_data