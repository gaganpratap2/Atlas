#include "atlas/market_data/binary_replayer.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>

#include "atlas/market_data/recording.hpp"

namespace atlas::market_data {

BinaryMarketDataReplayer::~BinaryMarketDataReplayer() {
    close();
}

void BinaryMarketDataReplayer::open(
    const std::filesystem::path& path
) {
    close();

    input_.open(
        path,
        std::ios::binary
    );

    if (!input_.is_open()) {
        throw std::runtime_error(
            "failed to open market-data recording"
        );
    }

    const auto magic = read_u32();
    const auto version = read_u16();

    if (magic != kRecordingMagic) {
        close();

        throw std::runtime_error(
            "invalid market-data recording magic"
        );
    }

    if (version != kRecordingVersion) {
        close();

        throw std::runtime_error(
            "unsupported market-data recording version"
        );
    }
}

void BinaryMarketDataReplayer::replay(
    ReplayHandler handler
) {
    if (!input_.is_open()) {
        throw std::runtime_error(
            "replayer is not open"
        );
    }

    if (!handler) {
        throw std::invalid_argument(
            "replay handler is empty"
        );
    }

    while (input_.peek() != std::char_traits<char>::eof()) {
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
            "truncated market-data recording"
        );
    }

    return static_cast<std::uint8_t>(
        static_cast<unsigned char>(byte)  //The conversion to unsigned char ensures the byte is interpreted as a value from 0 to 255.
    );
}

std::uint16_t BinaryMarketDataReplayer::read_u16() {
    std::uint16_t value = 0;

    for (std::size_t i = 0; i < sizeof(value); ++i) {
        value |=
            static_cast<std::uint16_t>(read_u8())
            << (8 * i);
    }

    return value;
}

std::uint32_t BinaryMarketDataReplayer::read_u32() {
    std::uint32_t value = 0;

    for (std::size_t i = 0; i < sizeof(value); ++i) {
        value |=
            static_cast<std::uint32_t>(read_u8())
            << (8 * i);
    }

    return value;
}

std::uint64_t BinaryMarketDataReplayer::read_u64() {
    std::uint64_t value = 0;

    for (std::size_t i = 0; i < sizeof(value); ++i) {
        value |=
            static_cast<std::uint64_t>(read_u8())
            << (8 * i);
    }

    return value;
}

std::int64_t BinaryMarketDataReplayer::read_i64() {
    return static_cast<std::int64_t>(
        read_u64()
    );
}

MarketEvent BinaryMarketDataReplayer::read_event() {
    const auto record_length = read_u32();
    const auto event_type =
        static_cast<RecordedEventType>(read_u8());

    switch (event_type) {
    case RecordedEventType::Trade: {
        constexpr std::uint32_t expected_length = 29;

        if (record_length != expected_length) {
            throw std::runtime_error(
                "invalid Trade record length"
            );
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
                "invalid Quote record length"
            );
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
            "invalid OrderBookUpdate record length"
        );
    }

    // Read fields in exactly the same order as the recorder.
    const auto instrument_id = read_u32();

    const auto raw_side = read_u8();

    if (raw_side > 1) {
        throw std::runtime_error(
            "invalid OrderBookUpdate side"
        );
    }

    const auto price = read_i64();
    const auto quantity = read_u64();
    const auto timestamp = read_u64();

    OrderBookUpdate update{
        instrument_id,
        raw_side == 0 ? Side::Buy : Side::Sell,
        price,
        quantity,
        timestamp
    };

    return update;
}

    default:
        throw std::runtime_error(
            "unsupported recorded event type"
        );
    }
}

} 