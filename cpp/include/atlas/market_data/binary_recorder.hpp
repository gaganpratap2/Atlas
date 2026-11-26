#pragma once

#include <filesystem>
#include <fstream>

#include "atlas/market_data/recorder.hpp"

namespace atlas::market_data{
    class BinaryMarketDataRecorder final : public MarketDataRecorder{
    public:
        ~BinaryMarketDataRecorder() override;

        void open(const std::filesystem::path& path) override;

        void record(const MarketEvent& event) override;

        void close() override;

    private:
        void write_u8(std::uint8_t value);
        void write_u16(std::uint16_t value);
        void write_u32(std::uint32_t value);
        void write_i64(std::int64_t value);
        void write_u64(std::uint64_t value);

        void write_trade(const Trade& trade);
        void write_quote(const Quote& quote);
        void write_order_book_update(const OrderBookUpdate& update);
        void write_order_add(const market_data::OrderAdd& order);
        void write_order_cancel(const OrderCancel& order);

        std::ofstream output_;
    };
}