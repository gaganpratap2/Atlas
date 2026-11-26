#include "atlas/market_data/validator.hpp"

namespace atlas::market_data {

bool is_valid(const Trade& trade) {
    return trade.instrument_id != 0 &&
           trade.price > 0 &&
           trade.quantity > 0 &&
           trade.timestamp > 0;
}

bool is_valid(const Quote& quote) {
    return quote.instrument_id != 0 &&
           quote.bid_price > 0 &&
           quote.ask_price > 0 &&
           quote.bid_quantity > 0 &&
           quote.ask_quantity > 0 &&
           quote.bid_price <= quote.ask_price &&
           quote.timestamp > 0;
}

bool is_valid(const OrderBookUpdate& update) {
    return update.instrument_id != 0 &&
           update.price > 0 &&
           update.timestamp > 0;
}

bool is_valid(const OrderAdd& message) {
    return message.instrument_id != 0 &&
           message.order_id != 0 &&
           message.price > 0 &&
           message.quantity > 0 &&
           (message.side == Side::Buy ||
            message.side == Side::Sell) &&
           message.timestamp > 0;
}

bool is_valid(const OrderCancel& message) {
    return message.instrument_id != 0 &&
           message.order_id != 0 &&
           message.timestamp > 0;
}

}  