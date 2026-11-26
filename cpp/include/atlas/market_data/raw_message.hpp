#pragma once
#include<cstdint>

namespace atlas::market_data {
    enum class RawMessageType : std::uint8_t {
    Trade = 1,
    Quote = 2,
    OrderBookUpdate = 3,
    OrderAdd = 4,
    OrderCancel = 5
};

    struct RawMessageHeader {
        std::uint16_t message_length;
        RawMessageType message_type;
    };
}