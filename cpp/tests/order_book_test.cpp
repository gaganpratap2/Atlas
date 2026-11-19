
#include <cassert>
#include <stdexcept>

#include "atlas/order_book/order_book.hpp"

int main() {
    using namespace atlas::market_data;
    using namespace atlas::order_book;

    OrderBook book(1);

    // Initially, the order book should be empty.
    assert(book.bid_level_count() == 0);
    assert(book.ask_level_count() == 0);

    // Add two bid levels.
    book.apply(OrderBookUpdate{
        1, Side::Buy, 10000, 50, 1
    });

    book.apply(OrderBookUpdate{
        1, Side::Buy, 9990, 100, 2
    });

    // Add two ask levels.
    book.apply(OrderBookUpdate{
        1, Side::Sell, 10010, 75, 3
    });

    book.apply(OrderBookUpdate{
        1, Side::Sell, 10020, 125, 4
    });

    // Check the number of price levels.
    assert(book.bid_level_count() == 2);
    assert(book.ask_level_count() == 2);

    // Check best bid.
    auto bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 10000);
    assert(bid->quantity == 50);

    // Check best ask.
    auto ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10010);
    assert(ask->quantity == 75);

    // Check spread.
    const auto spread = book.spread();

    assert(spread.has_value());
    assert(*spread == 10);

    // Check mid price.
    const auto mid = book.mid_price();

    assert(mid.has_value());
    assert(*mid == 10005);

    // Check invalid instrument ID.
    bool threw = false;

    try {
        book.apply(OrderBookUpdate{
            2, Side::Buy, 10000, 10, 5
        });
    }
    catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);

    // Update an existing bid level.
    book.apply(OrderBookUpdate{
        1, Side::Buy, 10000, 75, 6
    });

    bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 10000);
    assert(bid->quantity == 75);

    // Remove the best bid.
    book.apply(OrderBookUpdate{
        1, Side::Buy, 10000, 0, 7
    });

    bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 9990);
    assert(bid->quantity == 100);

    // Remove the remaining bid.
    book.apply(OrderBookUpdate{
        1, Side::Buy, 9990, 0, 8
    });

    assert(!book.best_bid());
    assert(book.bid_level_count() == 0);

    // Update the best ask.
    book.apply(OrderBookUpdate{
        1, Side::Sell, 10010, 100, 9
    });

    ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10010);
    assert(ask->quantity == 100);

    // Remove the best ask.
    book.apply(OrderBookUpdate{
        1, Side::Sell, 10010, 0, 10
    });

    ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10020);
    assert(ask->quantity == 125);

    // Remove the final ask.
    book.apply(OrderBookUpdate{
        1, Side::Sell, 10020, 0, 11
    });

    assert(!book.best_ask());
    assert(book.ask_level_count() == 0);

    // Removing a nonexistent level is harmless.
    book.apply(OrderBookUpdate{
        1, Side::Buy, 12345, 0, 12
    });

    assert(book.bid_level_count() == 0);


    // A crossed book is representable because this class mirrors
// external market-data updates rather than matching orders.
    // A crossed book is representable because this class mirrors
    // external market-data updates rather than matching orders.

    book.apply(OrderBookUpdate{
        1, Side::Buy, 10010, 50, 13
    });

    book.apply(OrderBookUpdate{
        1, Side::Sell, 10000, 25, 14
    });

    // Check crossed book.
    bid = book.best_bid();
    ask = book.best_ask();

    assert(bid.has_value());
    assert(ask.has_value());

    assert(bid->price == 10010);
    assert(bid->quantity == 50);

    assert(ask->price == 10000);
    assert(ask->quantity == 25);

    // Check negative spread.
    auto crossed_spread = book.spread();

    assert(crossed_spread.has_value());
    assert(*crossed_spread == -10);

    // Check top of book.
    auto top = book.top_of_book();

    assert(top.has_value());

    assert(top->bid.price == 10010);
    assert(top->bid.quantity == 50);

    assert(top->ask.price == 10000);
    assert(top->ask.quantity == 25);

    return 0;
}
