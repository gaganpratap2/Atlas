#include "atlas/market_data/counting_processor.hpp"

#include<variant>
namespace atlas::market_data {
    void CountingProcessor::process(const MarketEvent &event){
        std::visit(
            [this](const auto& value){
                using T = std::decay_t<decltype(value)>;  // It determines the actual type of value and stores that type's name in T.

                if constexpr (std::is_same_v<T, Trade>){
                    ++trade_count_;
                }else if constexpr (std:: is_same_v<T , Quote>){
                    ++quote_count_;
                } else if constexpr (std::is_same_v<T, OrderBookUpdate>) {
                ++order_book_update_count_;
                }
            },
            event
        );
    }

    std::size_t CountingProcessor::trade_count() const noexcept {
    return trade_count_;
}

std::size_t CountingProcessor::quote_count() const noexcept {
    return quote_count_;
}

std::size_t CountingProcessor::order_book_update_count() const noexcept {
    return order_book_update_count_;
}

}