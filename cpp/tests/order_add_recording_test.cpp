#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <variant>
#include <vector>

#include "atlas/market_data/binary_recorder.hpp"
#include "atlas/market_data/binary_replayer.hpp"
#include "atlas/market_data/types.hpp"

using namespace atlas;

int main() {
    const std::filesystem::path path =
        "atlas_order_add_recording_test.bin";

    // ------------------------------------------------------------
    // 1. Build original mixed event sequence
    // ------------------------------------------------------------

    market_data::Trade trade{
        .instrument_id = 1,
        .price = 10005,
        .quantity = 10,
        .timestamp = 1000
    };

    market_data::OrderAdd order_add_1{
        .instrument_id = 1,
        .order_id = 12345,
        .side = market_data::Side::Buy,
        .price = 10000,
        .quantity = 25,
        .timestamp = 1001
    };

    market_data::Quote quote{
        .instrument_id = 1,
        .bid_price = 10000,
        .bid_quantity = 50,
        .ask_price = 10005,
        .ask_quantity = 75,
        .timestamp = 1002
    };

    market_data::OrderBookUpdate book_update{
        .instrument_id = 1,
        .side = market_data::Side::Sell,
        .price = 10005,
        .quantity = 75,
        .timestamp = 1003
    };

    market_data::OrderAdd order_add_2{
        .instrument_id = 1,
        .order_id = 67890,
        .side = market_data::Side::Sell,
        .price = 10010,
        .quantity = 40,
        .timestamp = 1004
    };

    std::vector<market_data::MarketEvent> original_events{
        trade,
        order_add_1,
        quote,
        book_update,
        order_add_2
    };

    // ------------------------------------------------------------
    // 2. Record events
    // ------------------------------------------------------------

    {
        market_data::BinaryMarketDataRecorder recorder;

        recorder.open(path);

        for (const auto& event : original_events) {
            recorder.record(event);
        }

        recorder.close();
    }

    assert(std::filesystem::exists(path));
    assert(std::filesystem::file_size(path) > 0);

    // ------------------------------------------------------------
    // 3. Replay events
    // ------------------------------------------------------------

    std::vector<market_data::MarketEvent> replayed_events;

    {
        market_data::BinaryMarketDataReplayer replayer;

        replayer.open(path);

        replayer.replay(
            [&](const market_data::MarketEvent& event) {
                replayed_events.push_back(event);
            });

        replayer.close();
    }

    // ------------------------------------------------------------
    // 4. Verify event count
    // ------------------------------------------------------------

    assert(replayed_events.size() == original_events.size());

    // ------------------------------------------------------------
    // 5. Verify event type sequence
    // ------------------------------------------------------------

    assert(
        std::holds_alternative<market_data::Trade>(
            replayed_events[0]));

    assert(
        std::holds_alternative<market_data::OrderAdd>(
            replayed_events[1]));

    assert(
        std::holds_alternative<market_data::Quote>(
            replayed_events[2]));

    assert(
        std::holds_alternative<market_data::OrderBookUpdate>(
            replayed_events[3]));

    assert(
        std::holds_alternative<market_data::OrderAdd>(
            replayed_events[4]));

    // ------------------------------------------------------------
    // 6. Verify first OrderAdd
    // ------------------------------------------------------------

    {
        const auto& order =
            std::get<market_data::OrderAdd>(
                replayed_events[1]);

        assert(order.instrument_id == 1);
        assert(order.order_id == 12345);
        assert(order.side == market_data::Side::Buy);
        assert(order.price == 10000);
        assert(order.quantity == 25);
        assert(order.timestamp == 1001);
    }

    // ------------------------------------------------------------
    // 7. Verify second OrderAdd
    // ------------------------------------------------------------

    {
        const auto& order =
            std::get<market_data::OrderAdd>(
                replayed_events[4]);

        assert(order.instrument_id == 1);
        assert(order.order_id == 67890);
        assert(order.side == market_data::Side::Sell);
        assert(order.price == 10010);
        assert(order.quantity == 40);
        assert(order.timestamp == 1004);
    }

    // ------------------------------------------------------------
    // 8. Verify existing Trade survived round trip
    // ------------------------------------------------------------

    {
        const auto& replayed_trade =
            std::get<market_data::Trade>(
                replayed_events[0]);

        assert(replayed_trade.instrument_id == 1);
        assert(replayed_trade.price == 10005);
        assert(replayed_trade.quantity == 10);
        assert(replayed_trade.timestamp == 1000);
    }

    // ------------------------------------------------------------
    // 9. Verify existing Quote survived round trip
    // ------------------------------------------------------------

    {
        const auto& replayed_quote =
            std::get<market_data::Quote>(
                replayed_events[2]);

        assert(replayed_quote.instrument_id == 1);
        assert(replayed_quote.bid_price == 10000);
        assert(replayed_quote.bid_quantity == 50);
        assert(replayed_quote.ask_price == 10005);
        assert(replayed_quote.ask_quantity == 75);
        assert(replayed_quote.timestamp == 1002);
    }

    // ------------------------------------------------------------
    // 10. Verify existing OrderBookUpdate survived round trip
    // ------------------------------------------------------------

    {
        const auto& replayed_update =
            std::get<market_data::OrderBookUpdate>(
                replayed_events[3]);

        assert(replayed_update.instrument_id == 1);
        assert(replayed_update.side == market_data::Side::Sell);
        assert(replayed_update.price == 10005);
        assert(replayed_update.quantity == 75);
        assert(replayed_update.timestamp == 1003);
    }

    // ------------------------------------------------------------
    // 11. Replay again to verify deterministic replay
    // ------------------------------------------------------------

    std::vector<market_data::MarketEvent> replayed_again;

    {
        market_data::BinaryMarketDataReplayer replayer;

        replayer.open(path);

        replayer.replay(
            [&](const market_data::MarketEvent& event) {
                replayed_again.push_back(event);
            });

        replayer.close();
    }

    assert(replayed_again.size() == replayed_events.size());

    for (std::size_t i = 0;
         i < replayed_events.size();
         ++i) {

        assert(
            replayed_again[i].index() ==
            replayed_events[i].index());
    }

    // ------------------------------------------------------------
    // 12. Cleanup
    // ------------------------------------------------------------

    std::filesystem::remove(path);

    std::cout
        << "OrderAdd recording/replay round-trip test passed\n";

    return 0;
}