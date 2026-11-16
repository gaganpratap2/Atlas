#include "atlas/market_data/decoder.hpp"
#include "atlas/market_data/raw_message.hpp"

#include <stdexcept>

namespace atlas::market_data {

namespace {

template <typename T>
T read_le(std::span<const std::uint8_t> bytes, std::size_t offset) {  // Its job is to read bytes and convert them into an integer.
    constexpr std::size_t size = sizeof(T);

    if (offset + size > bytes.size()) {
        throw std::runtime_error("truncated market-data message");
    }

    T value{};

    for (std::size_t i = 0; i < size; ++i) {
        value |= static_cast<T>(bytes[offset + i]) << (8 * i);
    }

    return value;
}

}  // namespace

MarketEvent Decoder::decode(
    std::span<const std::uint8_t> bytes) const {  //It accepts raw bytes and returns a MarketEvent.

    if (bytes.size() < 3) {
        throw std::runtime_error("message too short");
    }

    const auto message_length =
        read_le<std::uint16_t>(bytes, 0);

    if (message_length != bytes.size()) {
        throw std::runtime_error("message length mismatch");
    }

    const auto message_type =
        static_cast<RawMessageType>(bytes[2]);

    switch (message_type) {
        case RawMessageType::Trade:
            if (bytes.size() != 31) {
                throw std::runtime_error("invalid trade size");
            }

            return Trade{
                .instrument_id =
                    read_le<std::uint32_t>(bytes, 3),

                .price =
                    read_le<std::int64_t>(bytes, 7),

                .quantity =
                    read_le<std::uint64_t>(bytes, 15),

                .timestamp =
                    read_le<std::uint64_t>(bytes, 23)
            };

        case RawMessageType::Quote:
            if (bytes.size() != 47) {
                throw std::runtime_error("invalid quote size");
            }

            return Quote{
                .instrument_id =
                    read_le<std::uint32_t>(bytes, 3),

                .bid_price =
                    read_le<std::int64_t>(bytes, 7),

                .bid_quantity =
                    read_le<std::uint64_t>(bytes, 15),

                .ask_price =
                    read_le<std::int64_t>(bytes, 23),

                .ask_quantity =
                    read_le<std::uint64_t>(bytes, 31),

                .timestamp =
                    read_le<std::uint64_t>(bytes, 39)
            };

        default:
            throw std::runtime_error(
                "unsupported market-data message type");
    }
}

}  // namespace atlas::market_data