#include "atlas/order_book/order_book_processor.hpp"

#include <type_traits>
#include <variant>

namespace atlas::order_book {

OrderBookProcessor::OrderBookProcessor(
    market_data::InstrumentId instrument_id)
    : order_book_(instrument_id) {
}

void OrderBookProcessor::process(
    const market_data::MarketEvent& event) {

    std::visit(
        [this](const auto& message) {
            using T = std::decay_t<decltype(message)>;

            if constexpr (
                std::is_same_v<T, market_data::OrderBookUpdate>) {

                order_book_.apply(message);
            }
        },
        event);
}

const OrderBook& OrderBookProcessor::order_book() const noexcept {
    return order_book_;
}

} // namespace atlas::order_book