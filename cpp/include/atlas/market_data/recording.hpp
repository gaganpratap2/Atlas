#pragma once
#include<cstdint>

namespace atlas::market_data{

constexpr std::uint32_t kRecordingMagic = 0x41544C53;
constexpr std::uint16_t kRecordingVersion = 1;

    enum class RecordedEventType : std::uint8_t {
        Trade = 1,
        Quote = 2,
        OrderBookUpdate = 3
    };

    struct RecordingHeader {
        std::uint32_t magic;
        std::uint16_t version;
    };

}