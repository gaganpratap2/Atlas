#include "atlas/market_data/binary_replayer.hpp"

#include <cstddef>
#include <stdexcept>

#include "atlas/market_data/recording.hpp"
#include "atlas/market_data/validator.hpp"

namespace atlas::market_data {

BinaryMarketDataReplayer::~BinaryMarketDataReplayer() {
    close();
}

void BinaryMarketDataReplayer::open(
    const std::filesystem::path& path) {

    close();

    input_.open(
        path,
        std::ios::binary);

    if (!input_.is_open()) {
        throw std::runtime_error(
            "failed to open market-data recording");
    }

    const auto magic = read_u32();
    const auto version = read_u16();

    if (magic != kRecordingMagic) {
        close();

        throw std::runtime_error(
            "invalid market-data recording magic");
    }

    if (version != kRecordingVersion) {
        close();

        throw std::runtime_error(
            "unsupported market-data recording version");
    }
}

void BinaryMarketDataReplayer::replay(
    ReplayHandler handler) {

    if (!input_.is_open()) {
        throw std::runtime_error(
            "replayer is not open");
    }

    if (!handler) {
        throw std::invalid_argument(
            "replay handler is empty");
    }

    while (
        input_.peek() !=
        std::char_traits<char>::eof()) {

        handler(read_event());
    }
}

void BinaryMarketDataReplayer::close() {
    if (input_.is_open()) {
        input_.close();
    }
}

std::uint8_t BinaryMarketDataReplayer::read_u8() {

    char byte = 0;

    if (!input_.get(byte)) {
        throw std::runtime_error(
            "truncated market-data recording");
    }

    return static_cast<std::uint8_t>(
        static_cast<unsigned char>(byte));
}

std::uint16_t BinaryMarketDataReplayer::read_u16() {

    std::uint16_t value = 0;

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        value |=
            static_cast<std::uint16_t>(
                read_u8())
            << (8 * i);
    }

    return value;
}

std::uint32_t BinaryMarketDataReplayer::read_u32() {

    std::uint32_t value = 0;

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        value |=
            static_cast<std::uint32_t>(
                read_u8())
            << (8 * i);
    }

    return value;
}

std::uint64_t BinaryMarketDataReplayer::read_u64() {

    std::uint64_t value = 0;

    for (std::size_t i = 0;
         i < sizeof(value);
         ++i) {

        value |=
            static_cast<std::uint64_t>(
                read_u8())
            << (8 * i);
    }

    return value;
}

std::int64_t BinaryMarketDataReplayer::read_i64() {

    return static_cast<std::int64_t>(
        read_u64());
}

MarketEvent BinaryMarketDataReplayer::read_event() {

    const auto record_length = read_u32();

    const auto event_type =
        static_cast<RecordedEventType>(
            read_u8());

    switch (event_type) {

    case RecordedEventType::Trade: {

        constexpr std::uint32_t expected_length = 29;

        if (record_length != expected_length) {
            throw std::runtime_error(
                "invalid Trade record length");
        }

        Trade trade{
            read_u32(),
            read_i64(),
            read_u64(),
            read_u64()
        };

        return trade;
    }

    case RecordedEventType::Quote: {

        constexpr std::uint32_t expected_length = 45;

        if (record_length != expected_length) {
            throw std::runtime_error(
                "invalid Quote record length");
        }

        Quote quote{
            read_u32(),
            read_i64(),
            read_u64(),
            read_i64(),
            read_u64(),
            read_u64()
        };

        return quote;
    }

    case RecordedEventType::OrderBookUpdate: {

        constexpr std::uint32_t expected_length = 30;

        if (record_length != expected_length) {
            throw std::runtime_error(
                "invalid OrderBookUpdate record length");
        }

        const auto instrument_id = read_u32();

        const auto raw_side = read_u8();

        if (raw_side > 1) {
            throw std::runtime_error(
                "invalid OrderBookUpdate side");
        }

        const auto price = read_i64();
        const auto quantity = read_u64();
        const auto timestamp = read_u64();

        OrderBookUpdate update{
            instrument_id,
            raw_side == 0
                ? Side::Buy
                : Side::Sell,
            price,
            quantity,
            timestamp
        };

        return update;
    }

    case RecordedEventType::OrderAdd: {
    constexpr std::uint32_t expected_length = 38;

    if (record_length != expected_length) {
        throw std::runtime_error("invalid OrderAdd record length");
    }

    const auto instrument_id = read_u32();
    const auto order_id = read_u64();
    const auto side_value = read_u8();
    const auto price = read_i64();
    const auto quantity = read_u64();
    const auto timestamp = read_u64();

    if (side_value > static_cast<std::uint8_t>(Side::Sell)) {
        throw std::runtime_error("invalid OrderAdd side");
    }

    OrderAdd order{
        .instrument_id = instrument_id,
        .order_id = order_id,
        .side = static_cast<Side>(side_value),
        .price = price,
        .quantity = quantity,
        .timestamp = timestamp
    };

    return order;
}

case RecordedEventType::OrderCancel: {
    constexpr std::uint32_t expected_length = 21;

    if (record_length != expected_length) {
        throw std::runtime_error(
            "invalid OrderCancel record length");
    }

    const auto instrument_id = read_u32();
    const auto order_id = read_u64();
    const auto timestamp = read_u64();

    return OrderCancel{
        .instrument_id = instrument_id,
        .order_id = order_id,
        .timestamp = timestamp
    };
}

    default:

        throw std::runtime_error(
            "unsupported recorded event type");
    }
}

} // namespace atlas::market_data