#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "atlas/market_data/replayer.hpp"
#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

class BinaryMarketDataReplayer final
    : public MarketDataReplayer {

public:

    ~BinaryMarketDataReplayer() override;

    void open(
        const std::filesystem::path& path) override;

    void replay(
        ReplayHandler handler) override;

    void close() override;

private:

    std::uint8_t read_u8();

    std::uint16_t read_u16();

    std::uint32_t read_u32();

    std::int64_t read_i64();

    std::uint64_t read_u64();

    MarketEvent read_event();

    std::ifstream input_;
};

} // namespace atlas::market_data