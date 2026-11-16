#pragma once

#include "atlas/market_data/event.hpp"

namespace atlas::market_data {

class EventProcessor {
public:
    virtual ~EventProcessor() = default;

    virtual void process(const MarketEvent& event) = 0;
};

}  // namespace atlas::market_data