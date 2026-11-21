#include "atlas/order_book/order_book.hpp"

#include <stdexcept>
#include <algorithm>

namespace atlas::order_book {
    OrderBook::OrderBook(market_data::InstrumentId instrument_id) : instrument_id_(instrument_id){

        if (instrument_id == 0){
            throw std::invalid_argument(
                "order book instrument id must be non-zero"
            );
        }
    }   

    void OrderBook::apply(const market_data::OrderBookUpdate &update){
        if (update.instrument_id != instrument_id_) {
        throw std::invalid_argument(
            "order book instrument mismatch"
        );
    }

    if (update.price <= 0 ){
        throw std::invalid_argument(
            "order book price must be positive"
        );
    }

    if (update.side == market_data::Side::Buy) {

    if (update.quantity == 0) {
        bids_.erase(update.price);
    }
    else {
        bids_[update.price] = update.quantity;
    }

}
else {

    if (update.quantity == 0) {
        asks_.erase(update.price);
    }
    else {
        asks_[update.price] = update.quantity;
    }

}
}

std::optional<PriceLevel> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }

    const auto& [price , quantity] = *bids_.begin();

    return PriceLevel{
        price,
        quantity
    };
}

std::optional<PriceLevel> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }

    const auto& [price, quantity] = *asks_.begin();

    return PriceLevel{
        price,
        quantity
    };
}

std::optional<Price> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();

    if (!bid || !ask) {
        return std::nullopt;
    }

    return ask->price - bid->price;
}

std::optional<Price> OrderBook::mid_price() const {
    const auto bid = best_bid();
    const auto ask = best_ask();

    if (!bid || !ask) {
        return std::nullopt;
    }

    return bid->price +
        (ask->price - bid->price) / 2;
}

std::size_t OrderBook::bid_level_count() const {
    return bids_.size();
}

std::size_t OrderBook::ask_level_count() const {
    return asks_.size();
}

std::optional<TopOfBook> OrderBook::top_of_book() const {
    const auto bid = best_bid();
    const auto ask = best_ask();

    if (!bid || !ask) {
        return std::nullopt;
    }

    return TopOfBook{*bid, *ask};
}


std::vector<PriceLevel> OrderBook::bid_depth() const {
    return bid_depth(bids_.size());
}

std::vector<PriceLevel> OrderBook::ask_depth() const {
    return ask_depth(asks_.size());
}

Quantity OrderBook::total_bid_quantity() const {
    Quantity total = 0;

    for (const auto& [price, quantity] : bids_) {
        (void)price;   //(void)price; is used to tell the C++ compiler that the variable price is intentionally unused.
        total += quantity;
    }

    return total;
}

Quantity OrderBook::total_ask_quantity() const {
    Quantity total = 0;

    for (const auto& [price, quantity] : asks_) {
        (void)price;
        total += quantity;
    }

    return total;
}


OrderBookSnapshot OrderBook::snapshot() const{
    return OrderBookSnapshot{
        instrument_id_,
        bid_depth(),
        ask_depth()
    };
}

std::vector<PriceLevel> OrderBook::bid_depth(
    std::size_t levels) const{

    std::vector<PriceLevel> result;

    const std::size_t count = std::min(levels , bids_.size());

    result.reserve(count);

    auto it = bids_.begin();

    for(std::size_t i =0 ; i < count ; ++i , ++it){
        result.push_back(PriceLevel{
            it->first,
            it->second
        });
    }
    return result;
}

std::vector<PriceLevel> OrderBook::ask_depth(
    std::size_t levels) const {

    std::vector<PriceLevel> result;

    const std::size_t count =
        std::min(levels, asks_.size());

    result.reserve(count);

    auto it = asks_.begin();

    for (std::size_t i = 0; i < count; ++i, ++it) {
        result.push_back(PriceLevel{
            it->first,
            it->second
        });
    }

    return result;
}


BookMetrics OrderBook::metrics(std::size_t levels) const {

    /*
+1.0
Only bid quantity

+0.5
More bid quantity

0.0
Equal bid and ask quantities

-0.5
More ask quantity

-1.0
Only ask quantity
    */


    const auto bids = bid_depth(levels);
    const auto asks = ask_depth(levels);

    Quantity bid_quantity = 0;
    Quantity ask_quantity = 0;

    for (const auto& level : bids) {
        bid_quantity += level.quantity;
    }

    for (const auto& level : asks) {
        ask_quantity += level.quantity;
    }

    const Quantity total_quantity =
        bid_quantity + ask_quantity;

    double imbalance = 0.0;

    if (total_quantity != 0) {
        imbalance =
            static_cast<double>(bid_quantity) -
            static_cast<double>(ask_quantity);

        imbalance /= static_cast<double>(total_quantity);
    }

    double weighted_mid_price = 0.0;

    if (bid_quantity != 0 && ask_quantity != 0) {
        const auto bid = bids.front();
        const auto ask = asks.front();

        const double numerator =
            static_cast<double>(ask.price) *
                static_cast<double>(bid_quantity)
            +
            static_cast<double>(bid.price) *
                static_cast<double>(ask_quantity);

        weighted_mid_price =
            numerator /
            static_cast<double>(total_quantity);
    }

    return BookMetrics{
        bid_quantity,
        ask_quantity,
        imbalance,
        weighted_mid_price
    };
}


}