#pragma once

#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

bool is_valid(const Trade& trade);
bool is_valid(const Quote& quote);
bool is_valid(const OrderBookUpdate& update);
bool is_valid(const OrderAdd &orderAdd);
bool is_valid(const OrderCancel &orderCancel);

}  // namespace atlas::market_data