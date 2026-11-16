#include <cassert>

#include "atlas/market_data/types.hpp"
#include "atlas/market_data/validator.hpp"
#include "atlas/market_data/event.hpp"
#include "atlas/market_data/counting_processor.hpp"
#include <vector>
#include "atlas/market_data/test_feed.hpp"

int main() {
    using namespace atlas::market_data;

    Trade trade{
        .instrument_id = 1,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1000000
    };

    assert(is_valid(trade));

    Quote quote{
        .instrument_id = 1,
        .bid_price = 10100,
        .bid_quantity = 50,
        .ask_price = 10125,
        .ask_quantity = 75,
        .timestamp = 1000001
    };

    assert(is_valid(quote));

    OrderBookUpdate update{
        .instrument_id = 1,
        .side = Side::Buy,
        .price = 10100,
        .quantity = 25,
        .timestamp = 1000002
    };

    assert(is_valid(update));

    Trade invalid_trade{
        .instrument_id = 0,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1000000
    };

    assert(!is_valid(invalid_trade));

    Quote crossed_quote{
        .instrument_id = 1,
        .bid_price = 10200,
        .bid_quantity = 50,
        .ask_price = 10100,
        .ask_quantity = 75,
        .timestamp = 1000003
    };

    assert(!is_valid(crossed_quote));


    MarketEvent event = trade;

assert(std::holds_alternative<Trade>(event));

event = quote;

assert(std::holds_alternative<Quote>(event));

event = update;

assert(std::holds_alternative<OrderBookUpdate>(event));

    CountingProcessor processor;

processor.process(trade);
processor.process(quote);
processor.process(update);

assert(processor.trade_count() == 1);
assert(processor.quote_count() == 1);
assert(processor.order_book_update_count() == 1);

std::vector<MarketEvent> events{
    trade,
    quote,
    update
};

CountingProcessor feed_processor;

TestMarketDataFeed feed(events);

feed.set_handler(
    [&](const MarketEvent& event) {
        feed_processor.process(event);
    }
);

feed.start();

assert(feed_processor.trade_count() == 1);
assert(feed_processor.quote_count() == 1);
assert(feed_processor.order_book_update_count() == 1);

    return 0;
}