#include "atlas/order_book/order_book.hpp"

#include <stdexcept>

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


}