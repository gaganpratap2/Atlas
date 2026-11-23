#pragma once

#include "atlas/market_data/processor.hpp"
#include "atlas/order_book/order_book.hpp"

namespace atlas::order_book{

class OrderBookProcessor final : public market_data::EventProcessor {
public:
    explicit OrderBookProcessor(market_data::InstrumentId instrument_id);

    void process(const market_data::MarketEvent& event) override;

    const OrderBook& order_book() const noexcept;

private:
    OrderBook order_book_;
};
}