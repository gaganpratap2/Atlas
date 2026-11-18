#pragma once
#include<filesystem>
#include <functional>
#include "atlas/market_data/event.hpp"

namespace atlas::market_data{
    using ReplayHandler = std::function<void(const MarketEvent&)>;

    class MarketDataReplayer {
    public:
        virtual~MarketDataReplayer() = default;

        virtual void open(const std::filesystem::path& path) = 0;
        virtual void replay(ReplayHandler handler) = 0;
        virtual void close() = 0;
    };
}