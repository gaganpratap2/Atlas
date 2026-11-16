#pragma once

#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

bool is_valid(const Trade& trade);
bool is_valid(const Quote& quote);
bool is_valid(const OrderBookUpdate& update);

}  // namespace atlas::market_data