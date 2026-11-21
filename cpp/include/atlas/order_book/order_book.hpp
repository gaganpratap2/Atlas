#include<cstdint>
#include<map>
#include<optional>
#include <vector>

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

    struct OrderBookSnapshot {
        market_data::InstrumentId instrument_id;
        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
    };

    struct BookMetrics {
        Quantity total_bid_quantity;
        Quantity total_ask_quantity;
        double imbalance;
        double weighted_mid_price;
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


        std::vector<PriceLevel> bid_depth() const;
        std::vector<PriceLevel> ask_depth() const;
        Quantity total_bid_quantity() const;
        Quantity total_ask_quantity() const;

        OrderBookSnapshot snapshot() const;

        std::vector<PriceLevel> bid_depth(std::size_t levels) const;
        std::vector<PriceLevel> ask_depth(std::size_t levels) const;

        BookMetrics metrics(std::size_t levels) const;

    private:
        market_data::InstrumentId instrument_id_;
        std::map<Price, Quantity, std::greater<Price>> bids_;   // sorted in descending order of price.
        std::map<Price, Quantity> asks_;    
    };
}