#include <stdexcept>

#include "atlas/market_data/binary_recorder.hpp"
#include "atlas/market_data/recording.hpp"

namespace atlas::market_data {

BinaryMarketDataRecorder::~BinaryMarketDataRecorder() {
    close();
}

void BinaryMarketDataRecorder::open(
    const std::filesystem::path& path) {

    close();

    output_.open(
        path,
        std::ios::binary | std::ios::trunc);

    if (!output_.is_open()) {
        throw std::runtime_error(
            "failed to open market_data recording");
    }

    write_u32(kRecordingMagic);
    write_u16(kRecordingVersion);
}

void BinaryMarketDataRecorder::record(
    const MarketEvent& event) {

    if (!output_.is_open()) {
        throw std::runtime_error(
            "recorder is not open");
    }

    std::visit(
        [this](const auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, Trade>) {
    write_trade(value);
} else if constexpr (std::is_same_v<T, Quote>) {
    write_quote(value);
} else if constexpr (std::is_same_v<T, OrderBookUpdate>) {
    write_order_book_update(value);
} else if constexpr (std::is_same_v<T, OrderAdd>) {
    write_order_add(value);
}else if constexpr (std::is_same_v<T, OrderCancel>) {
    write_order_cancel(value);
}
        },
        event);

    if (!output_) {
        throw std::runtime_error(
            "failed while writing market-data recording");
    }
}

void BinaryMarketDataRecorder::close() {
    if (output_.is_open()) {
        output_.flush();
        output_.close();
    }
}

void BinaryMarketDataRecorder::write_u8(
    std::uint8_t value) {

    output_.put(static_cast<char>(value));
}

void BinaryMarketDataRecorder::write_u16(
    std::uint16_t value) {

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        write_u8(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFFU));
    }
}

void BinaryMarketDataRecorder::write_u32(
    std::uint32_t value) {

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        write_u8(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFFU));
    }
}

void BinaryMarketDataRecorder::write_i64(
    std::int64_t value) {

    write_u64(
        static_cast<std::uint64_t>(value));
}

void BinaryMarketDataRecorder::write_u64(
    std::uint64_t value) {

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        write_u8(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFFULL));
    }
}

void BinaryMarketDataRecorder::write_trade(
    const Trade& trade) {

    // type(1) + instrument(4) + price(8)
    // + quantity(8) + timestamp(8)
    // = 29 bytes
    constexpr std::uint32_t record_length = 29;

    write_u32(record_length);

    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::Trade));

    write_u32(trade.instrument_id);
    write_i64(trade.price);
    write_u64(trade.quantity);
    write_u64(trade.timestamp);
}

void BinaryMarketDataRecorder::write_quote(
    const Quote& quote) {

    // type(1) + instrument(4) + bid_price(8)
    // + bid_quantity(8) + ask_price(8)
    // + ask_quantity(8) + timestamp(8)
    // = 45 bytes
    constexpr std::uint32_t record_length = 45;

    write_u32(record_length);

    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::Quote));

    write_u32(quote.instrument_id);
    write_i64(quote.bid_price);
    write_u64(quote.bid_quantity);
    write_i64(quote.ask_price);
    write_u64(quote.ask_quantity);
    write_u64(quote.timestamp);
}

void BinaryMarketDataRecorder::write_order_book_update(
    const OrderBookUpdate& update) {

    // type(1) + instrument(4) + side(1)
    // + price(8) + quantity(8) + timestamp(8)
    // = 30 bytes
    constexpr std::uint32_t record_length = 30;

    write_u32(record_length);

    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::OrderBookUpdate));

    write_u32(update.instrument_id);

    write_u8(
        static_cast<std::uint8_t>(
            update.side));

    write_i64(update.price);
    write_u64(update.quantity);
    write_u64(update.timestamp);
}



void BinaryMarketDataRecorder::write_order_add(
    const market_data::OrderAdd& order) {

    constexpr std::uint32_t record_length = 38;

    write_u32(record_length);

    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::OrderAdd));

    write_u32(order.instrument_id);
    write_u64(order.order_id);
    write_u8(
        static_cast<std::uint8_t>(order.side));
    write_i64(order.price);
    write_u64(order.quantity);
    write_u64(order.timestamp);
}


void BinaryMarketDataRecorder::write_order_cancel(
    const OrderCancel& order) {

    constexpr std::uint32_t record_length = 21;

    write_u32(record_length);
    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::OrderCancel));

    write_u32(order.instrument_id);
    write_u64(order.order_id);
    write_u64(order.timestamp);
}



} // namespace atlas::market_data