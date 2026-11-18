
#include <cassert>
#include <cstdint>
#include <exception>
#include <variant>
#include <vector>

#include <filesystem>
#include <fstream>
#include "atlas/market_data/recording.hpp"

#include "atlas/market_data/binary_recorder.hpp"
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




{
    using namespace atlas::market_data;

    const std::filesystem::path path =
        "atlas_day4_test_recording.bin";

    std::filesystem::remove(path);

    BinaryMarketDataRecorder recorder;

    // 1. Check that recording before opening throws an exception.
    bool threw_before_open = false;

    try {
        recorder.record(
            Trade{
                1,
                10117,
                100,
                1'000'000
            }
        );
    } catch (const std::runtime_error&) {
        threw_before_open = true;
    }

    assert(threw_before_open);

    // 2. Open the binary recording file.
    recorder.open(path);

    // 3. Create a Trade event.
    const Trade trade{
        1,
        10117,
        100,
        1'000'000
    };

    // 4. Record the Trade event.
    recorder.record(trade);

    // 5. Close the recording file.
    recorder.close();

    // 6. Check that the file exists and contains data.
    assert(std::filesystem::exists(path));
    assert(std::filesystem::file_size(path) > 0);

    // 7. Open the file for reading.
    std::ifstream input(
        path,
        std::ios::binary
    );

    assert(input.is_open());

    // 8. Read the first four bytes (magic number).
    std::uint8_t magic_bytes[4]{};

    input.read(
        reinterpret_cast<char*>(magic_bytes),
        sizeof(magic_bytes)
    );

    assert(input);

    // 9. Reconstruct the little-endian magic number.
    const std::uint32_t magic =
        static_cast<std::uint32_t>(magic_bytes[0]) |
        (static_cast<std::uint32_t>(magic_bytes[1]) << 8) |
        (static_cast<std::uint32_t>(magic_bytes[2]) << 16) |
        (static_cast<std::uint32_t>(magic_bytes[3]) << 24);

    // 10. Verify the magic number.
    assert(magic == kRecordingMagic);

    // 11. Close the input file.
    input.close();

    // 12. Remove the test file.
    std::filesystem::remove(path);
}


    return 0;
}
