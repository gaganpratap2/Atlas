#include <cassert>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <variant>
#include <vector>

#include "atlas/market_data/binary_recorder.hpp"
#include "atlas/market_data/binary_replayer.hpp"
#include "atlas/market_data/counting_processor.hpp"
#include "atlas/market_data/decoder.hpp"
#include "atlas/market_data/event.hpp"
#include "atlas/market_data/recording.hpp"
#include "atlas/market_data/test_feed.hpp"
#include "atlas/market_data/types.hpp"
#include "atlas/market_data/validator.hpp"

int main() {

    using namespace atlas::market_data;

    // =========================================================
    // 1. Test Trade validation
    // =========================================================

    Trade trade{
        .instrument_id = 1,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1'000'000
    };

    assert(is_valid(trade));

    std::cerr << "TEST 1: Trade validation\n";

    // =========================================================
    // 2. Test Quote validation
    // =========================================================

    Quote quote{
        .instrument_id = 1,
        .bid_price = 10100,
        .bid_quantity = 50,
        .ask_price = 10125,
        .ask_quantity = 75,
        .timestamp = 1'000'001
    };

    assert(is_valid(quote));

    std::cerr << "TEST 2: Quote validation\n";

    // =========================================================
    // 3. Test OrderBookUpdate validation
    // =========================================================

    OrderBookUpdate update{
        .instrument_id = 1,
        .side = Side::Buy,
        .price = 10100,
        .quantity = 25,
        .timestamp = 1'000'002
    };

    assert(is_valid(update));

    std::cerr << "TEST 3: OrderBookUpdate validation\n";

    // =========================================================
    // 4. Test invalid Trade
    // =========================================================

    Trade invalid_trade{
        .instrument_id = 0,
        .price = 10125,
        .quantity = 100,
        .timestamp = 1'000'000
    };

    assert(!is_valid(invalid_trade));

    std::cerr << "TEST 4: Invalid Trade validation\n";

    // =========================================================
    // 5. Test crossed Quote
    // =========================================================

    Quote crossed_quote{
        .instrument_id = 1,
        .bid_price = 10200,
        .bid_quantity = 50,
        .ask_price = 10100,
        .ask_quantity = 75,
        .timestamp = 1'000'003
    };

    assert(!is_valid(crossed_quote));

    std::cerr << "TEST 5: Crossed Quote validation\n";

    // =========================================================
    // 6. Test MarketEvent variant
    // =========================================================

    MarketEvent event = trade;

    assert(std::holds_alternative<Trade>(event));

    event = quote;

    assert(std::holds_alternative<Quote>(event));

    event = update;

    assert(
        std::holds_alternative<OrderBookUpdate>(event)
    );

    std::cerr << "TEST 6: MarketEvent variant\n";

    // =========================================================
    // 7. Test CountingProcessor
    // =========================================================

    CountingProcessor processor;

    processor.process(trade);
    processor.process(quote);
    processor.process(update);

    assert(processor.trade_count() == 1);
    assert(processor.quote_count() == 1);
    assert(processor.order_book_update_count() == 1);

    std::cerr << "TEST 7: CountingProcessor\n";

    // =========================================================
    // 8. Test MarketDataFeed
    // =========================================================

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
    assert(
        feed_processor.order_book_update_count() == 1
    );

    std::cerr << "TEST 8: MarketDataFeed\n";

    // =========================================================
    // 9. Test Decoder - OrderBookUpdate
    // =========================================================

    std::vector<std::uint8_t> book_update_bytes{
        32, 0,       // message length
        3,           // OrderBookUpdate

        1, 0, 0, 0,  // instrument_id = 1

        0,           // Buy

        0x70, 0x27, 0, 0,
        0, 0, 0, 0,  // price = 10096

        25, 0, 0, 0,
        0, 0, 0, 0,  // quantity = 25

        0x41, 0x42, 0x0f, 0,
        0, 0, 0, 0   // timestamp = 1,000,001
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
        std::get<OrderBookUpdate>(
            decoded_update
        );

    assert(decoded_book_update.instrument_id == 1);
    assert(decoded_book_update.side == Side::Buy);
    assert(decoded_book_update.price == 10096);
    assert(decoded_book_update.quantity == 25);
    assert(
        decoded_book_update.timestamp == 1'000'001
    );

    std::cerr << "TEST 9: OrderBookUpdate decoder\n";

    // =========================================================
    // 10. Test invalid message length
    // =========================================================

    bool invalid_length_threw = false;

    try {

        std::vector<std::uint8_t> invalid{
            31, 0, 1
        };

        decoder.decode(invalid);

    } catch (const std::exception&) {

        invalid_length_threw = true;
    }

    assert(invalid_length_threw);

    std::cerr << "TEST 10: Invalid message length\n";

    // =========================================================
    // 11. Test invalid OrderBookUpdate side
    // =========================================================

    auto invalid_side = book_update_bytes;

    // Header = 3 bytes
    // instrument_id = bytes 3..6
    // side = byte 7
    invalid_side[7] = 99;

    bool invalid_side_threw = false;

    try {

        decoder.decode(invalid_side);

    } catch (const std::exception&) {

        invalid_side_threw = true;
    }

    assert(invalid_side_threw);

    std::cerr << "TEST 11: Invalid OrderBookUpdate side\n";

    // =========================================================
    // 12. Test binary recorder
    // =========================================================

    {
        const std::filesystem::path path =
            "atlas_day4_test_recording.bin";

        std::filesystem::remove(path);

        BinaryMarketDataRecorder recorder;

        // Recording before open must fail.
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

        // Open recording.
        recorder.open(path);

        const Trade recording_trade{
            1,
            10117,
            100,
            1'000'000
        };

        recorder.record(recording_trade);

        recorder.close();

        // Verify file.
        assert(std::filesystem::exists(path));
        assert(std::filesystem::file_size(path) > 0);

        std::ifstream input(
            path,
            std::ios::binary
        );

        assert(input.is_open());

        // Read magic.
        std::uint8_t magic_bytes[4]{};

        input.read(
            reinterpret_cast<char*>(magic_bytes),
            sizeof(magic_bytes)
        );

        assert(input);

        const std::uint32_t magic =
            static_cast<std::uint32_t>(magic_bytes[0]) |
            (static_cast<std::uint32_t>(
                magic_bytes[1]) << 8) |
            (static_cast<std::uint32_t>(
                magic_bytes[2]) << 16) |
            (static_cast<std::uint32_t>(
                magic_bytes[3]) << 24);

        assert(magic == kRecordingMagic);

        input.close();

        std::filesystem::remove(path);

        std::cerr << "TEST 12: Binary recorder\n";
    }

    // =========================================================
    // 13. Test binary recorder -> replayer
    // =========================================================

    {
        const std::filesystem::path path =
            "atlas_day4_replay_test.bin";

        std::filesystem::remove(path);

        BinaryMarketDataRecorder recorder;

        recorder.open(path);

        const Trade replay_trade{
            1,
            10117,
            100,
            1'000'000
        };

        const Quote replay_quote{
            1,
            10100,
            50,
            10120,
            75,
            1'000'001
        };

        const OrderBookUpdate replay_update{
            1,
            Side::Buy,
            10096,
            25,
            1'000'002
        };

        recorder.record(replay_trade);
        recorder.record(replay_quote);
        recorder.record(replay_update);

        recorder.close();

        // Replay.
        BinaryMarketDataReplayer replayer;

        replayer.open(path);

        std::size_t event_count = 0;

        replayer.replay(
            [&](const MarketEvent& replay_event) {

                switch (event_count) {

                case 0: {

                    assert(
                        std::holds_alternative<Trade>(
                            replay_event
                        )
                    );

                    const auto& value =
                        std::get<Trade>(
                            replay_event
                        );

                    assert(value.instrument_id == 1);
                    assert(value.price == 10117);
                    assert(value.quantity == 100);
                    assert(
                        value.timestamp == 1'000'000
                    );

                    break;
                }

                case 1: {

                    assert(
                        std::holds_alternative<Quote>(
                            replay_event
                        )
                    );

                    const auto& value =
                        std::get<Quote>(
                            replay_event
                        );

                    assert(value.instrument_id == 1);
                    assert(value.bid_price == 10100);
                    assert(value.bid_quantity == 50);
                    assert(value.ask_price == 10120);
                    assert(value.ask_quantity == 75);
                    assert(
                        value.timestamp == 1'000'001
                    );

                    break;
                }

                case 2: {

                    assert(
                        std::holds_alternative<
                            OrderBookUpdate
                        >(
                            replay_event
                        )
                    );

                    const auto& value =
                        std::get<OrderBookUpdate>(
                            replay_event
                        );

                    assert(value.instrument_id == 1);
                    assert(value.side == Side::Buy);
                    assert(value.price == 10096);
                    assert(value.quantity == 25);
                    assert(
                        value.timestamp == 1'000'002
                    );

                    break;
                }

                default:
                    assert(false);
                }

                ++event_count;
            }
        );

        assert(event_count == 3);

        replayer.close();

        std::filesystem::remove(path);

        std::cerr << "TEST 13: Recorder -> Replayer\n";
    }

    // =========================================================
    // 14. Test invalid recording header
    // =========================================================

    {
        const std::filesystem::path path =
            "atlas_invalid_recording.bin";

        std::filesystem::remove(path);

        {
            std::ofstream output(
                path,
                std::ios::binary |
                std::ios::trunc
            );

            const std::uint8_t bytes[] = {
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00
            };

            output.write(
                reinterpret_cast<const char*>(
                    bytes
                ),
                sizeof(bytes)
            );
        }

        BinaryMarketDataReplayer replayer;

        bool threw = false;

        try {

            replayer.open(path);

        } catch (const std::runtime_error&) {

            threw = true;
        }

        assert(threw);

        std::filesystem::remove(path);

        std::cerr << "TEST 14: Invalid recording header\n";
    }

    // =========================================================
    // 15. Test deterministic replay
    // =========================================================

    {
        const std::filesystem::path path =
            "atlas_deterministic_replay_test.bin";

        std::filesystem::remove(path);

        BinaryMarketDataRecorder recorder;

        recorder.open(path);

        const Trade deterministic_trade{
            7,
            15000,
            10,
            2'000'000
        };

        const Quote deterministic_quote{
            7,
            14990,
            20,
            15010,
            30,
            2'000'001
        };

        const OrderBookUpdate deterministic_update{
            7,
            Side::Sell,
            15005,
            15,
            2'000'002
        };

        recorder.record(deterministic_trade);
        recorder.record(deterministic_quote);
        recorder.record(deterministic_update);

        recorder.close();

        BinaryMarketDataReplayer replayer;

        replayer.open(path);

        std::vector<MarketEvent> replayed_events;

        replayer.replay(
            [&](const MarketEvent& replay_event) {
                replayed_events.push_back(
                    replay_event
                );
            }
        );

        replayer.close();

        assert(replayed_events.size() == 3);

        assert(
            std::holds_alternative<Trade>(
                replayed_events[0]
            )
        );

        assert(
            std::holds_alternative<Quote>(
                replayed_events[1]
            )
        );

        assert(
            std::holds_alternative<OrderBookUpdate>(
                replayed_events[2]
            )
        );

        const auto& replayed_trade =
            std::get<Trade>(
                replayed_events[0]
            );

        assert(
            replayed_trade.instrument_id == 7
        );

        assert(
            replayed_trade.price == 15000
        );

        assert(
            replayed_trade.quantity == 10
        );

        assert(
            replayed_trade.timestamp == 2'000'000
        );

        const auto& replayed_quote =
            std::get<Quote>(
                replayed_events[1]
            );

        assert(
            replayed_quote.instrument_id == 7
        );

        assert(
            replayed_quote.bid_price == 14990
        );

        assert(
            replayed_quote.bid_quantity == 20
        );

        assert(
            replayed_quote.ask_price == 15010
        );

        assert(
            replayed_quote.ask_quantity == 30
        );

        assert(
            replayed_quote.timestamp == 2'000'001
        );

        const auto& replayed_update =
            std::get<OrderBookUpdate>(
                replayed_events[2]
            );

        assert(
            replayed_update.instrument_id == 7
        );

        assert(
            replayed_update.side == Side::Sell
        );

        assert(
            replayed_update.price == 15005
        );

        assert(
            replayed_update.quantity == 15
        );

        assert(
            replayed_update.timestamp == 2'000'002
        );

        std::filesystem::remove(path);

        std::cerr << "TEST 15: Deterministic replay\n";
    }

    // =========================================================
    // 16. Test replay twice
    // =========================================================

    {
        const std::filesystem::path path =
            "atlas_replay_twice_test.bin";

        std::filesystem::remove(path);

        BinaryMarketDataRecorder recorder;

        recorder.open(path);

        recorder.record(
            Trade{
                3,
                12000,
                5,
                3'000'000
            }
        );

        recorder.record(
            Quote{
                3,
                11990,
                10,
                12010,
                12,
                3'000'001
            }
        );

        recorder.close();

        BinaryMarketDataReplayer replayer;

        // First replay.
        replayer.open(path);

        std::vector<MarketEvent> first_replay;

        replayer.replay(
            [&](const MarketEvent& replay_event) {
                first_replay.push_back(
                    replay_event
                );
            }
        );

        replayer.close();

        // Second replay.
        replayer.open(path);

        std::vector<MarketEvent> second_replay;

        replayer.replay(
            [&](const MarketEvent& replay_event) {
                second_replay.push_back(
                    replay_event
                );
            }
        );

        replayer.close();

        assert(first_replay.size() == 2);
        assert(second_replay.size() == 2);

        // First Trade.
        assert(
            std::holds_alternative<Trade>(
                first_replay[0]
            )
        );

        assert(
            std::holds_alternative<Trade>(
                second_replay[0]
            )
        );

        const auto& first_trade =
            std::get<Trade>(
                first_replay[0]
            );

        const auto& second_trade =
            std::get<Trade>(
                second_replay[0]
            );

        assert(
            first_trade.instrument_id ==
            second_trade.instrument_id
        );

        assert(
            first_trade.price ==
            second_trade.price
        );

        assert(
            first_trade.quantity ==
            second_trade.quantity
        );

        assert(
            first_trade.timestamp ==
            second_trade.timestamp
        );

        // Second Quote.
        assert(
            std::holds_alternative<Quote>(
                first_replay[1]
            )
        );

        assert(
            std::holds_alternative<Quote>(
                second_replay[1]
            )
        );

        const auto& first_quote =
            std::get<Quote>(
                first_replay[1]
            );

        const auto& second_quote =
            std::get<Quote>(
                second_replay[1]
            );

        assert(
            first_quote.instrument_id ==
            second_quote.instrument_id
        );

        assert(
            first_quote.bid_price ==
            second_quote.bid_price
        );

        assert(
            first_quote.bid_quantity ==
            second_quote.bid_quantity
        );

        assert(
            first_quote.ask_price ==
            second_quote.ask_price
        );

        assert(
            first_quote.ask_quantity ==
            second_quote.ask_quantity
        );

        assert(
            first_quote.timestamp ==
            second_quote.timestamp
        );

        std::filesystem::remove(path);

        std::cerr << "TEST 16: Replay twice\n";
    }

    
// =========================================================
// 17. Test Decoder - OrderCancel
// =========================================================

std::vector<std::uint8_t> order_cancel_bytes{
    23, 0,
    5,

    1, 0, 0, 0,

    42, 0, 0, 0,
    0, 0, 0, 0,

    100, 0, 0, 0,
    0, 0, 0, 0
};

MarketEvent decoded_cancel =
    decoder.decode(order_cancel_bytes);

const auto* cancel =
    std::get_if<OrderCancel>(&decoded_cancel);

assert(cancel != nullptr);
assert(cancel->instrument_id == 1);
assert(cancel->order_id == 42);
assert(cancel->timestamp == 100);

std::cerr << "TEST 17: OrderCancel decoder\n";


// =========================================================
// 18. Test Invalid OrderCancel Message Length
// =========================================================

bool invalid_cancel_length_threw = false;

try {
    auto invalid_cancel = order_cancel_bytes;

    invalid_cancel[0] = 22;

    decoder.decode(invalid_cancel);

} catch (const std::exception&) {
    invalid_cancel_length_threw = true;
}

assert(invalid_cancel_length_threw);

std::cerr << "TEST 18: Invalid OrderCancel message length\n";



// =========================================================
// 19. Test All Five Event Types - Recording and Replay
// =========================================================

{
    const std::filesystem::path path =
        "atlas_all_events_test.bin";

    std::filesystem::remove(path);

    const std::vector<MarketEvent> original{
        Trade{
            .instrument_id = 1,
            .price = 10000,
            .quantity = 10,
            .timestamp = 100
        },

        Quote{
            .instrument_id = 1,
            .bid_price = 9999,
            .bid_quantity = 20,
            .ask_price = 10001,
            .ask_quantity = 30,
            .timestamp = 101
        },

        OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 9998,
            .quantity = 15,
            .timestamp = 102
        },

        OrderAdd{
            .instrument_id = 1,
            .order_id = 42,
            .side = Side::Sell,
            .price = 10002,
            .quantity = 25,
            .timestamp = 103
        },

        OrderCancel{
            .instrument_id = 1,
            .order_id = 42,
            .timestamp = 104
        }
    };

    BinaryMarketDataRecorder recorder;

    recorder.open(path);

    for (const auto& event : original) {
        std::visit(
            [&](const auto& value) {
                recorder.record(value);
            },
            event
        );
    }

    recorder.close();

    // Replay recorded events.

    BinaryMarketDataReplayer replayer;

    replayer.open(path);

    std::vector<MarketEvent> replayed;

    replayer.replay(
        [&](const MarketEvent& event) {
            replayed.push_back(event);
        }
    );

    replayer.close();

    // Verify event count.

    assert(replayed.size() == 5);

    // Verify event types.

    assert(std::holds_alternative<Trade>(replayed[0]));
    assert(std::holds_alternative<Quote>(replayed[1]));
    assert(std::holds_alternative<OrderBookUpdate>(replayed[2]));
    assert(std::holds_alternative<OrderAdd>(replayed[3]));
    assert(std::holds_alternative<OrderCancel>(replayed[4]));

    // Verify Trade.

    const auto& trade =
        std::get<Trade>(replayed[0]);

    assert(trade.instrument_id == 1);
    assert(trade.price == 10000);
    assert(trade.quantity == 10);
    assert(trade.timestamp == 100);

    // Verify Quote.

    const auto& quote =
        std::get<Quote>(replayed[1]);

    assert(quote.instrument_id == 1);
    assert(quote.bid_price == 9999);
    assert(quote.bid_quantity == 20);
    assert(quote.ask_price == 10001);
    assert(quote.ask_quantity == 30);
    assert(quote.timestamp == 101);

    // Verify OrderBookUpdate.

    const auto& update =
        std::get<OrderBookUpdate>(replayed[2]);

    assert(update.instrument_id == 1);
    assert(update.side == Side::Buy);
    assert(update.price == 9998);
    assert(update.quantity == 15);
    assert(update.timestamp == 102);

    // Verify OrderAdd.

    const auto& add =
        std::get<OrderAdd>(replayed[3]);

    assert(add.instrument_id == 1);
    assert(add.order_id == 42);
    assert(add.side == Side::Sell);
    assert(add.price == 10002);
    assert(add.quantity == 25);
    assert(add.timestamp == 103);

    // Verify OrderCancel.

    const auto& cancel =
        std::get<OrderCancel>(replayed[4]);

    assert(cancel.instrument_id == 1);
    assert(cancel.order_id == 42);
    assert(cancel.timestamp == 104);

    std::filesystem::remove(path);

    std::cerr << "TEST 19: All five event types recording and replay\n";
}



    // =========================================================
    // All tests passed
    // =========================================================


    std::cout
        << "All market-data tests passed\n";



    return 0;
}