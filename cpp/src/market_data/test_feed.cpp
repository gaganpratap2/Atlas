#include "atlas/market_data/test_feed.hpp"

#include <utility>

namespace atlas::market_data {

TestMarketDataFeed::TestMarketDataFeed(std::vector<MarketEvent> events)
    : events_(std::move(events)) {}

void TestMarketDataFeed::set_handler(EventHandler handler) {
    handler_ = std::move(handler);
}

void TestMarketDataFeed::start() {
    if (running_) {
        return;
    }

    running_ = true;

    if (!handler_) {
        return;
    }

    for (const auto& event : events_) {
        if (!running_) {
            break;
        }

        handler_(event);
    }
}

void TestMarketDataFeed::stop() {
    running_ = false;
}

}  // namespace atlas::market_data