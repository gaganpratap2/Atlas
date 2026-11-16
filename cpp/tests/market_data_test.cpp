
#include <cassert>
#include <cstdint>
#include <exception>
#include <variant>
#include <vector>

#include "atlas/market_data/types.hpp"
#include "atlas/market_data/validator.hpp"
#include "atlas/market_data/event.hpp"
#include "atlas/market_data/counting_processor.hpp"
#include "atlas/market_data/test_feed.hpp"
#include "atlas/market_data/decoder.hpp"

int main() {
    using namespace atlas::market_data;

    // =========================================
    // 1. Test Trade validation
    // =========================================

    Trade trade{
        .instrument_id = 1,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1000000
    };

    assert(is_valid(trade));

    // =========================================
    // 2. Test Quote validation
    // =========================================

    Quote quote{
        .instrument_id = 1,
        .bid_price = 10100,
        .bid_quantity = 50,
        .ask_price = 10125,
        .ask_quantity = 75,
        .timestamp = 1000001
    };

    assert(is_valid(quote));

    // =========================================
    // 3. Test OrderBookUpdate validation
    // =========================================

    OrderBookUpdate update{
        .instrument_id = 1,
        .side = Side::Buy,
        .price = 10100,
        .quantity = 25,
        .timestamp = 1000002
    };

    assert(is_valid(update));

    // =========================================
    // 4. Test invalid Trade
    // =========================================

    Trade invalid_trade{
        .instrument_id = 0,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1000000
    };

    assert(!is_valid(invalid_trade));

    // =========================================
    // 5. Test crossed Quote
    // =========================================

    Quote crossed_quote{
        .instrument_id = 1,
        .bid_price = 10200,
        .bid_quantity = 50,
        .ask_price = 10100,
        .ask_quantity = 75,
        .timestamp = 1000003
    };

    assert(!is_valid(crossed_quote));

    // =========================================
    // 6. Test MarketEvent variant
    // =========================================

    MarketEvent event = trade;

    assert(std::holds_alternative<Trade>(event));

    event = quote;

    assert(std::holds_alternative<Quote>(event));

    event = update;

    assert(std::holds_alternative<OrderBookUpdate>(event));

    // =========================================
    // 7. Test CountingProcessor
    // =========================================

    CountingProcessor processor;

    processor.process(trade);
    processor.process(quote);
    processor.process(update);

    assert(processor.trade_count() == 1);
    assert(processor.quote_count() == 1);
    assert(processor.order_book_update_count() == 1);

    // =========================================
    // 8. Test MarketDataFeed
    // =========================================

    std::vector<MarketEvent> events{
        trade,
        quote,
        update
    };

    CountingProcessor feed_processor;

    TestMarketDataFeed feed(events);

    feed.set_handler(
        [&](const MarketEvent& feed_event) {
            feed_processor.process(feed_event);
        }
    );

    feed.start();

    assert(feed_processor.trade_count() == 1);
    assert(feed_processor.quote_count() == 1);
    assert(feed_processor.order_book_update_count() == 1);

    // =========================================
    // 9. Test Decoder
    // =========================================

    std::vector<std::uint8_t> book_update_bytes{
        32, 0,       // Message length
        3,           // OrderBookUpdate

        1, 0, 0, 0,  // instrument_id = 1

        0,           // Buy

        0x70, 0x27, 0, 0,
        0, 0, 0, 0,  // price = 10096

        25, 0, 0, 0,
        0, 0, 0, 0,  // quantity = 25

        0x41, 0x42, 0x0f, 0,
        0, 0, 0, 0   // timestamp = 1000001
    };

    Decoder decoder;

    MarketEvent decoded_update =
        decoder.decode(book_update_bytes);

    assert(
        std::holds_alternative<OrderBookUpdate>(
            decoded_update
        )
    );

    const auto& decoded_book_update =
        std::get<OrderBookUpdate>(decoded_update);

    assert(decoded_book_update.instrument_id == 1);
    assert(decoded_book_update.side == Side::Buy);
    assert(decoded_book_update.price == 10096);
    assert(decoded_book_update.quantity == 25);
    assert(decoded_book_update.timestamp == 1'000'001);

    // =========================================
    // 10. Test invalid message length
    // =========================================

    bool threw = false;

    try {
        std::vector<std::uint8_t> invalid{
            31, 0, 1
        };

        decoder.decode(invalid);

    } catch (const std::exception&) {
        threw = true;
    }

    assert(threw);

    // =========================================
    // 11. Test invalid OrderBookUpdate side
    // =========================================

    auto invalid_side = book_update_bytes;

    invalid_side[7] = 99;

    bool invalid_side_threw = false;

    try {
        decoder.decode(invalid_side);

    } catch (const std::exception&) {
        invalid_side_threw = true;
    }

    assert(invalid_side_threw);

    return 0;
}
