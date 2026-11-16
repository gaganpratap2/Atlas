#pragma once

#include <vector>

#include "atlas/market_data/feed.hpp"

namespace atlas::market_data {

class TestMarketDataFeed final : public MarketDataFeed {
public:
    explicit TestMarketDataFeed(std::vector<MarketEvent> events);

    void set_handler(EventHandler handler) override;
    void start() override;
    void stop() override;

private:
    std::vector<MarketEvent> events_;
    EventHandler handler_;
    bool running_{false};
};

}  // namespace atlas::market_data