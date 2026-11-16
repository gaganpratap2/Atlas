#pragma once

#include <cstddef>

#include "atlas/market_data/event.hpp"
#include "atlas/market_data/processor.hpp"

namespace atlas::market_data {

class CountingProcessor final : public EventProcessor {
public:
    void process(const MarketEvent& event) override;

    std::size_t trade_count() const noexcept;
    std::size_t quote_count() const noexcept;
    std::size_t order_book_update_count() const noexcept;

private:
    std::size_t trade_count_{0};
    std::size_t quote_count_{0};
    std::size_t order_book_update_count_{0};
};

}  // namespace atlas::market_data