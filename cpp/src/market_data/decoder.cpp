
#include "atlas/market_data/decoder.hpp"
#include "atlas/market_data/raw_message.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>

namespace atlas::market_data {

namespace {

// Reads an integer from raw bytes in little-endian format.
template <typename T>
T read_le(std::span<const std::uint8_t> bytes, std::size_t offset) {

    constexpr std::size_t size = sizeof(T);

    if (offset > bytes.size() || size > bytes.size() - offset) {
        throw std::runtime_error("truncated market-data message");
    }

    using UnsignedT = std::make_unsigned_t<T>;

    UnsignedT value{};

    for (std::size_t i = 0; i < size; ++i) {
        value |= static_cast<UnsignedT>(bytes[offset + i])
                 << (8 * i);
    }

    return static_cast<T>(value);
}

}  // namespace

MarketEvent Decoder::decode(
    std::span<const std::uint8_t> bytes) const {

    // Minimum message size: length + type.
    if (bytes.size() < 3) {
        throw std::runtime_error("message too short");
    }

    // Read message length.
    const auto message_length =
        read_le<std::uint16_t>(bytes, 0);

    if (message_length != bytes.size()) {
        throw std::runtime_error("message length mismatch");
    }

    // Read message type.
    const auto message_type =
        static_cast<RawMessageType>(bytes[2]);

    switch (message_type) {

        case RawMessageType::Trade: {

            if (bytes.size() != 31) {
                throw std::runtime_error("invalid trade size");
            }

            return Trade{
                .instrument_id = read_le<std::uint32_t>(bytes, 3),
                .price = read_le<std::int64_t>(bytes, 7),
                .quantity = read_le<std::uint64_t>(bytes, 15),
                .timestamp = read_le<std::uint64_t>(bytes, 23)
            };
        }

        case RawMessageType::Quote: {

            if (bytes.size() != 47) {
                throw std::runtime_error("invalid quote size");
            }

            return Quote{
                .instrument_id = read_le<std::uint32_t>(bytes, 3),
                .bid_price = read_le<std::int64_t>(bytes, 7),
                .bid_quantity = read_le<std::uint64_t>(bytes, 15),
                .ask_price = read_le<std::int64_t>(bytes, 23),
                .ask_quantity = read_le<std::uint64_t>(bytes, 31),
                .timestamp = read_le<std::uint64_t>(bytes, 39)
            };
        }

        case RawMessageType::OrderBookUpdate: {

            if (bytes.size() != 32) {
                throw std::runtime_error(
                    "invalid order-book update size");
            }

            const auto raw_side =
                read_le<std::uint8_t>(bytes, 7);

            if (raw_side > 1) {
                throw std::runtime_error(
                    "invalid order-book side");
            }

            return OrderBookUpdate{
                .instrument_id = read_le<std::uint32_t>(bytes, 3),

                .side = raw_side == 0
                    ? Side::Buy
                    : Side::Sell,

                .price = read_le<std::int64_t>(bytes, 8),
                .quantity = read_le<std::uint64_t>(bytes, 16),
                .timestamp = read_le<std::uint64_t>(bytes, 24)
            };
        }

        case RawMessageType::OrderAdd: {

            if (bytes.size() != 40) {
                throw std::runtime_error(
                    "invalid OrderAdd message length");
            }

            OrderAdd message{
                .instrument_id = read_le<std::uint32_t>(bytes, 3),
                .order_id = read_le<std::uint64_t>(bytes, 7),

                .side = static_cast<Side>(
                    read_le<std::uint8_t>(bytes, 15)),

                .price = read_le<std::int64_t>(bytes, 16),
                .quantity = read_le<std::uint64_t>(bytes, 24),
                .timestamp = read_le<std::uint64_t>(bytes, 32)
            };

            if (message.side != Side::Buy &&
                message.side != Side::Sell) {

                throw std::runtime_error(
                    "invalid OrderAdd side");
            }

            return message;
        }


        case RawMessageType::OrderCancel: {
    constexpr std::size_t expected_length = 23;

    if (message_length != expected_length) {
        throw std::runtime_error("invalid OrderCancel message length");
    }

    const auto instrument_id =
        read_le<std::uint32_t>(bytes, 3);

    const auto order_id =
        read_le<std::uint64_t>(bytes, 7);

    const auto timestamp =
        read_le<std::uint64_t>(bytes, 15);

    return OrderCancel{
        .instrument_id = instrument_id,
        .order_id = order_id,
        .timestamp = timestamp
    };
}



        default:

            throw std::runtime_error(
                "unsupported market-data message type");
    }
}


SequencedMarketEvent Decoder::decode_sequenced(
    std::span<const std::uint8_t> bytes) const {

    constexpr std::size_t sequence_size = sizeof(SequenceNumber);

    if (bytes.size() < sequence_size) {
        throw std::runtime_error(
            "truncated sequenced market-data message");
    }

    const auto sequence_number =
        read_le<SequenceNumber>(bytes, 0);

    const auto event =
        decode(bytes.subspan(sequence_size));

    return SequencedMarketEvent{
        .sequence_number = sequence_number,
        .event = event
    };
}

}  // namespace atlas::market_data
