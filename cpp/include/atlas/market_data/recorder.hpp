#pragma once

#include <filesystem>

#include "atlas/market_data/event.hpp"

namespace atlas::market_data {

class MarketDataRecorder {
public:
    virtual ~MarketDataRecorder() = default;

    virtual void open(const std::filesystem::path& path) = 0;

    virtual void record(const MarketEvent& event) = 0;

    virtual void close() = 0;
};

} 