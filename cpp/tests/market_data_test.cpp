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
#include "atlas/market_data/sequence_tracker.hpp"
#include "atlas/market_data/sequence_policy.hpp"
#include "atlas/market_data/sequenced_event_processor.hpp"
#include "atlas/order_book/order_book_processor.hpp"
#include "atlas/market_data/feed_health.hpp"

#include "atlas/market_data/feed_recovery.hpp"



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





{
    SequenceTracker tracker;

    auto result = tracker.observe(100);
    assert(result.status == SequenceStatus::First);
    assert(tracker.next_expected() == 101);

    result = tracker.observe(101);
    assert(result.status == SequenceStatus::InOrder);
    assert(tracker.next_expected() == 102);

    // Gap: tracker must not advance.
    result = tracker.observe(105);
    assert(result.status == SequenceStatus::Gap);
    assert(result.expected == 102);
    assert(result.received == 105);
    assert(tracker.next_expected() == 102);

    // Repeated gap.
    result = tracker.observe(105);
    assert(result.status == SequenceStatus::Gap);

    // Older sequence is duplicate.
    result = tracker.observe(100);
    assert(result.status == SequenceStatus::Duplicate);

    // Recover missing sequences in order.
    result = tracker.observe(102);
    assert(result.status == SequenceStatus::InOrder);

    result = tracker.observe(103);
    assert(result.status == SequenceStatus::InOrder);

    result = tracker.observe(104);
    assert(result.status == SequenceStatus::InOrder);

    // Resend previously rejected sequence.
    result = tracker.observe(105);
    assert(result.status == SequenceStatus::InOrder);

    assert(tracker.next_expected() == 106);

    tracker.reset();

    assert(!tracker.initialized());
    assert(tracker.next_expected() == 0);

    result = tracker.observe(500);
    assert(result.status == SequenceStatus::First);
    assert(tracker.next_expected() == 501);

    std::cerr << "TEST 19A: SequenceTracker PASSED\n";
}








    // =========================================================
    // 20. Test Sequenced Trade Decoder
    // =========================================================

    {
        // Normal 31-byte Trade message.
        std::vector<std::uint8_t> trade_bytes{
            31, 0,                  // Message length
            1,                      // Trade message type

            1, 0, 0, 0,             // instrument_id = 1

            0x8d, 0x27, 0, 0,
            0, 0, 0, 0,             // price = 10125

            100, 0, 0, 0,
            0, 0, 0, 0,             // quantity = 100

            0x40, 0x42, 0x0f, 0,
            0, 0, 0, 0              // timestamp = 1000000
        };

        // Prepend 8-byte little-endian sequence number.
        const std::uint64_t sequence_number = 12345;

        std::vector<std::uint8_t> sequenced_bytes;

        for (int i = 0; i < 8; ++i) {
            sequenced_bytes.push_back(
                static_cast<std::uint8_t>(
                    (sequence_number >> (i * 8)) & 0xFF
                )
            );
        }

        sequenced_bytes.insert(
            sequenced_bytes.end(),
            trade_bytes.begin(),
            trade_bytes.end()
        );

        const auto decoded =
            decoder.decode_sequenced(sequenced_bytes);

        assert(decoded.sequence_number == 12345);

        const auto* decoded_trade =
            std::get_if<Trade>(&decoded.event);

        assert(decoded_trade != nullptr);

        assert(decoded_trade->instrument_id == 1);
        assert(decoded_trade->price == 10125);
        assert(decoded_trade->quantity == 100);
        assert(decoded_trade->timestamp == 1'000'000);

        std::cerr
            << "TEST 20: Sequenced Trade decoder\n";
    }


    // =========================================================
    // 21. Test Truncated Sequence Wrapper
    // =========================================================

    {
        std::vector<std::uint8_t> bytes(7);

        bool threw = false;

        try {

            decoder.decode_sequenced(bytes);

        } catch (const std::runtime_error&) {

            threw = true;
        }

        assert(threw);

        std::cerr
            << "TEST 21: Truncated sequence wrapper\n";
    }


    // =========================================================
    // 22. Test SequenceTracker + Decoder Integration
    // =========================================================

    {
        SequenceTracker tracker;

        Decoder sequenced_decoder;

        // Helper: prepend an 8-byte little-endian sequence.
        auto add_sequence = [](
            std::uint64_t sequence,
            std::vector<std::uint8_t> payload
        ) {

            std::vector<std::uint8_t> result;

            for (int i = 0; i < 8; ++i) {

                result.push_back(
                    static_cast<std::uint8_t>(
                        (sequence >> (i * 8)) & 0xFF
                    )
                );
            }

            result.insert(
                result.end(),
                payload.begin(),
                payload.end()
            );

            return result;
        };


        // -----------------------------------------------------
        // First message: Trade, sequence 1001
        // -----------------------------------------------------

        std::vector<std::uint8_t> trade_payload{
            31, 0,
            1,

            1, 0, 0, 0,

            0x10, 0x27, 0, 0,
            0, 0, 0, 0,

            10, 0, 0, 0,
            0, 0, 0, 0,

            100, 0, 0, 0,
            0, 0, 0, 0
        };

        auto first_bytes =
            add_sequence(1001, trade_payload);

        const auto first =
            sequenced_decoder.decode_sequenced(first_bytes);

        assert(first.sequence_number == 1001);

        const auto* first_trade =
            std::get_if<Trade>(&first.event);

        assert(first_trade != nullptr);

        assert(first_trade->instrument_id == 1);
        assert(first_trade->price == 10000);
        assert(first_trade->quantity == 10);
        assert(first_trade->timestamp == 100);

        const auto r1 =
            tracker.observe(first.sequence_number);

        assert(r1.status == SequenceStatus::First);


        // -----------------------------------------------------
        // Second message: Quote, sequence 1002
        // -----------------------------------------------------

        std::vector<std::uint8_t> quote_payload{
            47, 0,
            2,

            1, 0, 0, 0,

            0x0f, 0x27, 0, 0,
            0, 0, 0, 0,             // bid_price = 9999

            20, 0, 0, 0,
            0, 0, 0, 0,             // bid_quantity = 20

            0x11, 0x27, 0, 0,
            0, 0, 0, 0,             // ask_price = 10001

            30, 0, 0, 0,
            0, 0, 0, 0,             // ask_quantity = 30

            101, 0, 0, 0,
            0, 0, 0, 0              // timestamp = 101
        };

        auto second_bytes =
            add_sequence(1002, quote_payload);

        const auto second =
            sequenced_decoder.decode_sequenced(second_bytes);

        assert(second.sequence_number == 1002);

        const auto* second_quote =
            std::get_if<Quote>(&second.event);

        assert(second_quote != nullptr);

        assert(second_quote->instrument_id == 1);
        assert(second_quote->bid_price == 9999);
        assert(second_quote->bid_quantity == 20);
        assert(second_quote->ask_price == 10001);
        assert(second_quote->ask_quantity == 30);
        assert(second_quote->timestamp == 101);

        const auto r2 =
            tracker.observe(second.sequence_number);

        assert(r2.status == SequenceStatus::InOrder);


        // -----------------------------------------------------
        // Third message: OrderAdd, sequence 1004
        // -----------------------------------------------------

        std::vector<std::uint8_t> order_add_payload{
            40, 0,
            4,

            1, 0, 0, 0,             // instrument_id = 1

            42, 0, 0, 0,
            0, 0, 0, 0,             // order_id = 42

            1,                      // Sell

            0x12, 0x27, 0, 0,
            0, 0, 0, 0,             // price = 10002

            25, 0, 0, 0,
            0, 0, 0, 0,             // quantity = 25

            103, 0, 0, 0,
            0, 0, 0, 0              // timestamp = 103
        };

        auto third_bytes =
            add_sequence(1004, order_add_payload);

        const auto third =
            sequenced_decoder.decode_sequenced(third_bytes);

        assert(third.sequence_number == 1004);

        const auto* third_add =
            std::get_if<OrderAdd>(&third.event);

        assert(third_add != nullptr);

        assert(third_add->instrument_id == 1);
        assert(third_add->order_id == 42);
        assert(third_add->side == Side::Sell);
        assert(third_add->price == 10002);
        assert(third_add->quantity == 25);
        assert(third_add->timestamp == 103);

        const auto r3 =
            tracker.observe(third.sequence_number);

        assert(r3.status == SequenceStatus::Gap);
        assert(r3.expected == 1003);
        assert(r3.received == 1004);

        // The event must remain intact after sequence tracking.
        assert(std::holds_alternative<OrderAdd>(third.event));

        std::cerr
            << "TEST 22: SequenceTracker + Decoder integration\n";
    }



 
{
    SequencePolicy policy;

    auto decision = policy.evaluate(100);
    assert(decision.action == SequenceAction::Accept);
    assert(decision.result.status == SequenceStatus::First);

    decision = policy.evaluate(101);
    assert(decision.action == SequenceAction::Accept);
    assert(decision.result.status == SequenceStatus::InOrder);

    decision = policy.evaluate(101);
    assert(decision.action == SequenceAction::IgnoreDuplicate);
    assert(decision.result.status == SequenceStatus::Duplicate);

    // Gap.
    decision = policy.evaluate(105);
    assert(decision.action == SequenceAction::GapDetected);
    assert(decision.result.status == SequenceStatus::Gap);
    assert(decision.result.expected == 102);

    // Old sequence is duplicate, not out of order.
    decision = policy.evaluate(100);
    assert(decision.action == SequenceAction::IgnoreDuplicate);
    assert(decision.result.status == SequenceStatus::Duplicate);

    // Recover in order.
    decision = policy.evaluate(102);
    assert(decision.action == SequenceAction::Accept);

    decision = policy.evaluate(103);
    assert(decision.action == SequenceAction::Accept);

    decision = policy.evaluate(104);
    assert(decision.action == SequenceAction::Accept);

    decision = policy.evaluate(105);
    assert(decision.action == SequenceAction::Accept);

    assert(policy.tracker().next_expected() == 106);

    policy.reset();

    assert(!policy.tracker().initialized());
    assert(policy.tracker().next_expected() == 0);

    decision = policy.evaluate(500);
    assert(decision.action == SequenceAction::Accept);
    assert(decision.result.status == SequenceStatus::First);

    std::cerr << "TEST: SequencePolicy PASSED\n";
}



        
    // =========================================================
    // 23. Test SequencedEventProcessor + OrderBookProcessor
    // =========================================================

    
{
    atlas::order_book::OrderBookProcessor order_book_processor(1);

    SequencedEventProcessor processor(
        order_book_processor
    );

    // First event: Accept
    SequencedMarketEvent first{
        .sequence_number = 100,
        .event = OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 10000,
            .quantity = 10,
            .timestamp = 100
        }
    };

    auto decision = processor.process(first);

    assert(decision.action == SequenceAction::Accept);

    // Second event: Accept
    SequencedMarketEvent second{
        .sequence_number = 101,
        .event = OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 10001,
            .quantity = 20,
            .timestamp = 101
        }
    };

    decision = processor.process(second);

    assert(decision.action == SequenceAction::Accept);

    // Duplicate: Ignore
    SequencedMarketEvent duplicate{
        .sequence_number = 101,
        .event = OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 9990,
            .quantity = 999,
            .timestamp = 999
        }
    };

    decision = processor.process(duplicate);

    assert(decision.action == SequenceAction::IgnoreDuplicate);

    const auto unchanged =
        order_book_processor.order_book().best_bid();

    assert(unchanged.has_value());
    assert(unchanged->price == 10001);
    assert(unchanged->quantity == 20);

    // Gap: Detect, do not apply
    SequencedMarketEvent gap{
        .sequence_number = 103,
        .event = OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 10002,
            .quantity = 30,
            .timestamp = 103
        }
    };

    decision = processor.process(gap);

    assert(decision.action == SequenceAction::GapDetected);

    const auto after_gap =
        order_book_processor.order_book().best_bid();

    assert(after_gap.has_value());
    assert(after_gap->price == 10001);
    assert(after_gap->quantity == 20);

    // Previously accepted sequence: Duplicate
    SequencedMarketEvent old{
        .sequence_number = 100,
        .event = OrderBookUpdate{
            .instrument_id = 1,
            .side = Side::Buy,
            .price = 9000,
            .quantity = 999,
            .timestamp = 9000
        }
    };

    decision = processor.process(old);

    assert(decision.action == SequenceAction::IgnoreDuplicate);

    const auto after_old =
        order_book_processor.order_book().best_bid();

    assert(after_old.has_value());
    assert(after_old->price == 10001);
    assert(after_old->quantity == 20);

    std::cerr
        << "TEST 23: SequencedEventProcessor + "
        << "OrderBookProcessor PASSED\n";
}


        
    // =========================================================
    // 24. Test FeedHealthTracker
    // =========================================================

{
    FeedHealthTracker health_tracker;
    SequencePolicy health_policy;

    auto decision = health_policy.evaluate(100);
    auto health = health_tracker.observe(decision);

    assert(health.health == FeedHealth::Healthy);
    assert(health.accepted_count == 1);

    decision = health_policy.evaluate(101);
    health = health_tracker.observe(decision);

    assert(health.health == FeedHealth::Healthy);
    assert(health.accepted_count == 2);

    // Duplicate.
    decision = health_policy.evaluate(101);
    health = health_tracker.observe(decision);

    assert(health.duplicate_count == 1);
    assert(health.accepted_count == 2);

    // Gap.
    decision = health_policy.evaluate(103);
    health = health_tracker.observe(decision);

    assert(health.health == FeedHealth::GapDetected);
    assert(health.gap_count == 1);

    // Older sequence is duplicate.
    decision = health_policy.evaluate(100);
    health = health_tracker.observe(decision);

    assert(health.duplicate_count == 2);
    assert(health.out_of_order_count == 0);
    assert(health.gap_count == 1);
    assert(health.accepted_count == 2);

    const auto snapshot = health_tracker.snapshot();

    assert(snapshot.accepted_count == 2);
    assert(snapshot.duplicate_count == 2);
    assert(snapshot.gap_count == 1);
    assert(snapshot.out_of_order_count == 0);

    health_tracker.reset();

    const auto reset_health = health_tracker.snapshot();

    assert(reset_health.health == FeedHealth::Healthy);
    assert(reset_health.accepted_count == 0);
    assert(reset_health.duplicate_count == 0);
    assert(reset_health.gap_count == 0);
    assert(reset_health.out_of_order_count == 0);
    assert(reset_health.next_expected == 0);
    assert(reset_health.last_received == 0);

    std::cerr << "TEST 24: FeedHealthTracker\n";
}



    // =========================================================
// 25. Test SequenceTracker: Older Sequence -> Duplicate
// =========================================================

{
    SequenceTracker tracker;

    tracker.observe(100);
    tracker.observe(101);

    const auto duplicate = tracker.observe(100);

    assert(duplicate.status == SequenceStatus::Duplicate);
    assert(duplicate.expected == 102);
    assert(duplicate.received == 100);

    assert(tracker.next_expected() == 102);

    std::cerr << "TEST 25: SequenceTracker Duplicate Detection\n";
}



{
    atlas::order_book::OrderBookProcessor book_processor(1);

    SequencedEventProcessor processor(book_processor);

    auto make_event = [](std::uint64_t sequence) {
        return SequencedMarketEvent{
            .sequence_number = sequence,
            .event = OrderBookUpdate{
                .instrument_id = 1,
                .side = Side::Buy,
                .price = 10000,
                .quantity = 10,
                .timestamp = sequence
            }
        };
    };

    auto decision = processor.process(make_event(100));
    assert(decision.action == SequenceAction::Accept);

    decision = processor.process(make_event(101));
    assert(decision.action == SequenceAction::Accept);

    decision = processor.process(make_event(103));
    assert(decision.action == SequenceAction::GapDetected);

    // Recover missing sequence.
    decision = processor.process(make_event(102));
    assert(decision.action == SequenceAction::Accept);

    // Resume previously rejected sequence.
    decision = processor.process(make_event(103));
    assert(decision.action == SequenceAction::Accept);

    const auto health = processor.feed_health().snapshot();

    assert(health.accepted_count == 4);
    assert(health.gap_count == 1);
    assert(health.next_expected == 104);
    assert(health.last_received == 103);

    // Historical gap remains recorded.
    assert(health.health == FeedHealth::GapDetected);

    std::cerr
        << "TEST 26: SequencedEventProcessor recovery\n";
}



// =========================================================
// 27. Test FeedRecoveryTracker detects and recovers from gap
// =========================================================

{
    FeedRecoveryTracker recovery;

    // Initial state must be Live.
    auto snapshot = recovery.snapshot();

    assert(snapshot.state == FeedRecoveryState::Live);

    // Detect missing sequence 102.
    snapshot = recovery.observe(102, 105);

    assert(snapshot.state == FeedRecoveryState::GapDetected);
    assert(snapshot.missing_sequence == 102);
    assert(snapshot.recovery_sequence == 0);

    // Begin recovery.
    recovery.begin_recovery();

    snapshot = recovery.snapshot();

    assert(snapshot.state == FeedRecoveryState::Recovering);

    // Receive the missing sequence.
    snapshot = recovery.observe(102, 102);

    assert(snapshot.state == FeedRecoveryState::Recovered);
    assert(snapshot.missing_sequence == 102);
    assert(snapshot.recovery_sequence == 102);

    std::cerr
        << "TEST 27: FeedRecoveryTracker gap detection and recovery PASSED\n";
}


// =========================================================
// 28. Test FeedRecoveryTracker reset returns to Live
// =========================================================

{
    FeedRecoveryTracker recovery;

    // Create a gap.
    recovery.observe(102, 105);

    // Start recovery.
    recovery.begin_recovery();

    // Reset tracker.
    recovery.reset();

    const auto snapshot = recovery.snapshot();

    assert(snapshot.state == FeedRecoveryState::Live);

    assert(snapshot.missing_sequence == 0);
    assert(snapshot.recovery_sequence == 0);

    std::cerr
        << "TEST 28: FeedRecoveryTracker reset PASSED\n";
}




    // =========================================================
    // All tests passed
    // =========================================================


    std::cout
        << "All market-data tests passed\n";



    return 0;
}