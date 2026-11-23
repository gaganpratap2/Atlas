#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "atlas/market_data/decoder.hpp"
#include "atlas/market_data/validator.hpp"
#include "atlas/order_book/order_book_processor.hpp"

using namespace atlas;

namespace {

void write_u16_le(
    std::vector<std::uint8_t>& bytes,
    std::uint16_t value) {

    bytes.push_back(
        static_cast<std::uint8_t>(value & 0xFF));

    bytes.push_back(
        static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void write_u32_le(
    std::vector<std::uint8_t>& bytes,
    std::uint32_t value) {

    for (int i = 0; i < 4; ++i) {
        bytes.push_back(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFF));
    }
}

void write_u64_le(
    std::vector<std::uint8_t>& bytes,
    std::uint64_t value) {

    for (int i = 0; i < 8; ++i) {
        bytes.push_back(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFF));
    }
}

void write_i64_le(
    std::vector<std::uint8_t>& bytes,
    std::int64_t value) {

    write_u64_le(
        bytes,
        static_cast<std::uint64_t>(value));
}

std::vector<std::uint8_t> make_order_book_update(
    std::uint32_t instrument_id,
    std::uint8_t side,
    std::int64_t price,
    std::uint64_t quantity,
    std::uint64_t timestamp) {

    std::vector<std::uint8_t> bytes;

    // message length = 32 bytes
    write_u16_le(bytes, 32);

    // message type = OrderBookUpdate
    bytes.push_back(3);

    write_u32_le(bytes, instrument_id);
    bytes.push_back(side);
    write_i64_le(bytes, price);
    write_u64_le(bytes, quantity);
    write_u64_le(bytes, timestamp);

    return bytes;
}

} // namespace

int main() {

    constexpr market_data::InstrumentId instrument_id = 1;

    market_data::Decoder decoder;
    order_book::OrderBookProcessor processor(instrument_id);

    // ------------------------------------------------------------
    // Buy  @ 10000 x 50
    // ------------------------------------------------------------

    {
        auto bytes = make_order_book_update(
            instrument_id,
            0,       // Buy
            10000,
            50,
            100);

        auto event = decoder.decode(
            std::span<const std::uint8_t>(
                bytes.data(),
                bytes.size()));

        assert(market_data::is_valid(
    std::get<market_data::OrderBookUpdate>(event)
));

        processor.process(event);
    }

    // ------------------------------------------------------------
    // Buy @ 9995 x 100
    // ------------------------------------------------------------

    {
        auto bytes = make_order_book_update(
            instrument_id,
            0,       // Buy
            9995,
            100,
            101);

        auto event = decoder.decode(
            std::span<const std::uint8_t>(
                bytes.data(),
                bytes.size()));

        assert(market_data::is_valid(
    std::get<market_data::OrderBookUpdate>(event)
));

        processor.process(event);
    }

    // ------------------------------------------------------------
    // Sell @ 10005 x 75
    // ------------------------------------------------------------

    {
        auto bytes = make_order_book_update(
            instrument_id,
            1,       // Sell
            10005,
            75,
            102);

        auto event = decoder.decode(
            std::span<const std::uint8_t>(
                bytes.data(),
                bytes.size()));

        assert(market_data::is_valid(
    std::get<market_data::OrderBookUpdate>(event)
));

        processor.process(event);
    }

    // ------------------------------------------------------------
    // Sell @ 10010 x 125
    // ------------------------------------------------------------

    {
        auto bytes = make_order_book_update(
            instrument_id,
            1,       // Sell
            10010,
            125,
            103);

        auto event = decoder.decode(
            std::span<const std::uint8_t>(
                bytes.data(),
                bytes.size()));

        assert(market_data::is_valid(
    std::get<market_data::OrderBookUpdate>(event)
));

        processor.process(event);
    }

    const auto& book = processor.order_book();

    // ------------------------------------------------------------
    // Top of book
    // ------------------------------------------------------------

    const auto top = book.top_of_book();

    assert(top.has_value());

    assert(top->bid.price == 10000);
    assert(top->bid.quantity == 50);

    assert(top->ask.price == 10005);
    assert(top->ask.quantity == 75);

    // ------------------------------------------------------------
    // Spread
    // ------------------------------------------------------------

    const auto spread = book.spread();

    assert(spread.has_value());
    assert(*spread == 5);

    // ------------------------------------------------------------
    // Depth
    // ------------------------------------------------------------

    const auto bids = book.bid_depth(2);
    const auto asks = book.ask_depth(2);

    assert(bids.size() == 2);
    assert(asks.size() == 2);

    assert(bids[0].price == 10000);
    assert(bids[0].quantity == 50);

    assert(bids[1].price == 9995);
    assert(bids[1].quantity == 100);

    assert(asks[0].price == 10005);
    assert(asks[0].quantity == 75);

    assert(asks[1].price == 10010);
    assert(asks[1].quantity == 125);

    // ------------------------------------------------------------
    // Trade must not modify the order book
    // ------------------------------------------------------------

    market_data::Trade trade{
        .instrument_id = instrument_id,
        .price = 10005,
        .quantity = 10,
        .timestamp = 104
    };

    market_data::MarketEvent trade_event = trade;

    processor.process(trade_event);

    const auto top_after_trade = book.top_of_book();

    assert(top_after_trade.has_value());

    assert(top_after_trade->bid.price == 10000);
    assert(top_after_trade->bid.quantity == 50);

    assert(top_after_trade->ask.price == 10005);
    assert(top_after_trade->ask.quantity == 75);

    std::cout
        << "Market data -> decoder -> validator -> "
        << "order book integration test passed\n";

    return 0;
}