#pragma once

#include <functional>

#include "atlas/market_data/event.hpp"

namespace atlas::market_data {

using EventHandler = std::function<void(const MarketEvent&)>;
// EventHandler is another name for a function that accepts a MarketEvent and returns nothing.

class MarketDataFeed {
public:
    virtual ~MarketDataFeed() = default;

    virtual void set_handler(EventHandler handler) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

}  // namespace atlas::market_data