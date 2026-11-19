#include<cstdint>
#include<map>
#include<optional>

#include "atlas/market_data/event.hpp"

namespace atlas::order_book {
    
    using Price = market_data::Price;
    using Quantity = market_data::Quantity;
    
    struct PriceLevel {
        Price price;
        Quantity quantity;
    };

    struct TopOfBook {
        PriceLevel bid;
        PriceLevel ask;
    };

    class OrderBook {
    public:
        explicit OrderBook(market_data::InstrumentId instrument_id);
        void apply(const market_data::OrderBookUpdate &update);

        std::optional<PriceLevel> best_bid() const;
        std::optional<PriceLevel> best_ask() const;
        std::optional<Price> spread() const;
        std::optional<Price> mid_price() const;
        std::size_t bid_level_count() const;
        std::size_t ask_level_count() const;
        std::optional<TopOfBook> top_of_book() const;

    private:
        market_data::InstrumentId instrument_id_;
        std::map<Price, Quantity, std::greater<Price>> bids_;   // sorted in descending order of price.
        std::map<Price, Quantity> asks_;    
    };

}