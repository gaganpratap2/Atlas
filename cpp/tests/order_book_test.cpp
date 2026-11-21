#include <cassert>
#include <stdexcept>

#include "atlas/order_book/order_book.hpp"

int main() {
    using namespace atlas::market_data;
    using namespace atlas::order_book;

    OrderBook book(1);

    // ============================================================
    // 1. Initially, the order book should be empty.
    // ============================================================

    assert(book.bid_level_count() == 0);
    assert(book.ask_level_count() == 0);

    assert(book.bid_depth().empty());
    assert(book.ask_depth().empty());

    assert(book.total_bid_quantity() == 0);
    assert(book.total_ask_quantity() == 0);

    // ============================================================
    // 2. Add two bid levels.
    // ============================================================

    book.apply(OrderBookUpdate{1, Side::Buy, 10000, 50, 1});
    book.apply(OrderBookUpdate{1, Side::Buy, 9990, 100, 2});

    // ============================================================
    // 3. Add two ask levels.
    // ============================================================

    book.apply(OrderBookUpdate{1, Side::Sell, 10010, 75, 3});
    book.apply(OrderBookUpdate{1, Side::Sell, 10020, 125, 4});

    // Check number of price levels.
    assert(book.bid_level_count() == 2);
    assert(book.ask_level_count() == 2);

    // ============================================================
    // 4. Check best bid / ask / spread / mid.
    // ============================================================

    auto bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 10000);
    assert(bid->quantity == 50);

    auto ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10010);
    assert(ask->quantity == 75);

    auto spread = book.spread();

    assert(spread.has_value());
    assert(*spread == 10);

    auto mid = book.mid_price();

    assert(mid.has_value());
    assert(*mid == 10005);

    // ============================================================
    // 5. Check invalid instrument ID.
    // ============================================================

    bool threw = false;

    try {
        book.apply(OrderBookUpdate{
            2,
            Side::Buy,
            10000,
            10,
            5
        });
    }
    catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);

    // ============================================================
    // 6. Check depth and total quantities.
    // ============================================================

    auto bids = book.bid_depth();

    assert(bids.size() == 2);

    assert(bids[0].price == 10000);
    assert(bids[0].quantity == 50);

    assert(bids[1].price == 9990);
    assert(bids[1].quantity == 100);

    auto asks = book.ask_depth();

    assert(asks.size() == 2);

    assert(asks[0].price == 10010);
    assert(asks[0].quantity == 75);

    assert(asks[1].price == 10020);
    assert(asks[1].quantity == 125);

    assert(book.total_bid_quantity() == 150);
    assert(book.total_ask_quantity() == 200);

    // ============================================================
    // 7. Update an existing bid level.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10000,
        75,
        6
    });

    auto updated_snapshot = book.snapshot();

    assert(updated_snapshot.instrument_id == 1);
    assert(updated_snapshot.bids.size() == 2);

    assert(updated_snapshot.bids[0].price == 10000);
    assert(updated_snapshot.bids[0].quantity == 75);

    bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 10000);
    assert(bid->quantity == 75);

    bids = book.bid_depth();

    assert(bids.size() == 2);

    assert(bids[0].price == 10000);
    assert(bids[0].quantity == 75);

    assert(bids[1].price == 9990);
    assert(bids[1].quantity == 100);

    assert(book.total_bid_quantity() == 175);

    // ============================================================
    // Snapshot must be a copy, not a live view.
    // ============================================================

    updated_snapshot.bids[0].quantity = 999999;

    // Modifying the snapshot must not modify the live order book.
    bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 10000);
    assert(bid->quantity == 75);

    // ============================================================
    // Remove the best bid.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10000,
        0,
        7
    });

    bid = book.best_bid();

    assert(bid.has_value());
    assert(bid->price == 9990);
    assert(bid->quantity == 100);

    bids = book.bid_depth();

    assert(bids.size() == 1);
    assert(bids[0].price == 9990);
    assert(bids[0].quantity == 100);

    assert(book.total_bid_quantity() == 100);

    // ============================================================
    // Remove the remaining bid.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        9990,
        0,
        8
    });

    assert(!book.best_bid());
    assert(book.bid_level_count() == 0);

    // ============================================================
    // 8. Update the best ask.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10010,
        100,
        9
    });

    ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10010);
    assert(ask->quantity == 100);

    asks = book.ask_depth();

    assert(asks.size() == 2);

    assert(asks[0].price == 10010);
    assert(asks[0].quantity == 100);

    assert(asks[1].price == 10020);
    assert(asks[1].quantity == 125);

    assert(book.total_ask_quantity() == 225);

    // ============================================================
    // Remove the best ask.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10010,
        0,
        10
    });

    ask = book.best_ask();

    assert(ask.has_value());
    assert(ask->price == 10020);
    assert(ask->quantity == 125);

    // ============================================================
    // Remove the final ask.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10020,
        0,
        11
    });

    assert(!book.best_ask());
    assert(book.ask_level_count() == 0);

    // ============================================================
    // 9. Empty-book behavior.
    // ============================================================

    assert(book.bid_depth().empty());
    assert(book.ask_depth().empty());

    assert(book.total_bid_quantity() == 0);
    assert(book.total_ask_quantity() == 0);

    assert(!book.best_bid());
    assert(!book.best_ask());

    assert(!book.spread());
    assert(!book.mid_price());

    // Removing a nonexistent level is harmless.
    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        12345,
        0,
        12
    });

    assert(book.bid_level_count() == 0);
    assert(book.bid_depth().empty());
    assert(book.total_bid_quantity() == 0);

    // ============================================================
    // 10. Crossed book is representable.
    //
    // This class mirrors external market-data updates rather
    // than matching orders.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10010,
        50,
        13
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10000,
        25,
        14
    });

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

    // ============================================================
    // Check top of book.
    // ============================================================

    auto top = book.top_of_book();

    assert(top.has_value());

    assert(top->bid.price == 10010);
    assert(top->bid.quantity == 50);

    assert(top->ask.price == 10000);
    assert(top->ask.quantity == 25);

    // ============================================================
    // 11. Check crossed-book depth.
    // ============================================================

    bids = book.bid_depth();
    asks = book.ask_depth();

    assert(bids.size() == 1);

    assert(bids[0].price == 10010);
    assert(bids[0].quantity == 50);

    assert(asks.size() == 1);

    assert(asks[0].price == 10000);
    assert(asks[0].quantity == 25);

    assert(book.total_bid_quantity() == 50);
    assert(book.total_ask_quantity() == 25);

    // Add another bid and ask level.
    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        9990,
        100,
        15
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10020,
        125,
        16
    });

    bids = book.bid_depth();
    asks = book.ask_depth();

    assert(bids.size() == 2);

    assert(bids[0].price == 10010);
    assert(bids[0].quantity == 50);

    assert(bids[1].price == 9990);
    assert(bids[1].quantity == 100);

    assert(asks.size() == 2);

    assert(asks[0].price == 10000);
    assert(asks[0].quantity == 25);

    assert(asks[1].price == 10020);
    assert(asks[1].quantity == 125);

    assert(book.total_bid_quantity() == 150);
    assert(book.total_ask_quantity() == 150);

    // ============================================================
    // 12. Update existing crossed-book bid.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10010,
        75,
        17
    });

    bids = book.bid_depth();

    assert(bids.size() == 2);

    assert(bids[0].price == 10010);
    assert(bids[0].quantity == 75);

    assert(book.total_bid_quantity() == 175);

    // Remove the best bid.
    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10010,
        0,
        18
    });

    bids = book.bid_depth();

    assert(bids.size() == 1);

    assert(bids[0].price == 9990);
    assert(bids[0].quantity == 100);

    assert(book.total_bid_quantity() == 100);

    // ============================================================
    // Update existing crossed-book ask.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10000,
        50,
        19
    });

    asks = book.ask_depth();

    assert(asks.size() == 2);

    assert(asks[0].price == 10000);
    assert(asks[0].quantity == 50);

    assert(book.total_ask_quantity() == 175);

    // Remove the best ask.
    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10000,
        0,
        20
    });

    asks = book.ask_depth();

    assert(asks.size() == 1);

    assert(asks[0].price == 10020);
    assert(asks[0].quantity == 125);

    assert(book.total_ask_quantity() == 125);

    // ============================================================
    // 13. Remove all remaining levels.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        9990,
        0,
        21
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10020,
        0,
        22
    });

    // Final empty-book verification.
    assert(book.bid_depth().empty());
    assert(book.ask_depth().empty());

    assert(book.bid_level_count() == 0);
    assert(book.ask_level_count() == 0);

    assert(book.total_bid_quantity() == 0);
    assert(book.total_ask_quantity() == 0);

    assert(!book.best_bid());
    assert(!book.best_ask());

    assert(!book.spread());
    assert(!book.mid_price());

    // ============================================================
    // 14. Snapshot test.
    //
    // Rebuild the book because the previous section intentionally
    // removed every level.
    // ============================================================

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        10000,
        50,
        23
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Buy,
        9990,
        100,
        24
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10010,
        75,
        25
    });

    book.apply(OrderBookUpdate{
        1,
        Side::Sell,
        10020,
        125,
        26
    });

    auto snapshot = book.snapshot();

    // Check snapshot instrument ID.
    assert(snapshot.instrument_id == 1);

    // Check bid snapshot.
    assert(snapshot.bids.size() == 2);

    assert(snapshot.bids[0].price == 10000);
    assert(snapshot.bids[0].quantity == 50);

    assert(snapshot.bids[1].price == 9990);
    assert(snapshot.bids[1].quantity == 100);

    // Check ask snapshot.
    assert(snapshot.asks.size() == 2);

    assert(snapshot.asks[0].price == 10010);
    assert(snapshot.asks[0].quantity == 75);

    assert(snapshot.asks[1].price == 10020);
    assert(snapshot.asks[1].quantity == 125);

    // ============================================================
    // Snapshot must be independent from the live order book.
    // ============================================================

    snapshot.bids[0].quantity = 999999;

    // The snapshot changed.
    assert(snapshot.bids[0].quantity == 999999);

    // But the live order book must remain unchanged.
    auto live_bid = book.best_bid();

    assert(live_bid.has_value());
    assert(live_bid->price == 10000);
    assert(live_bid->quantity == 50);

    // Also verify live depth remains unchanged.
    bids = book.bid_depth();

    assert(bids.size() == 2);
    assert(bids[0].price == 10000);
    assert(bids[0].quantity == 50);

    assert(bids[1].price == 9990);
    assert(bids[1].quantity == 100);

    // Verify live ask side remains unchanged as well.
    asks = book.ask_depth();

    assert(asks.size() == 2);

    assert(asks[0].price == 10010);
    assert(asks[0].quantity == 75);

    assert(asks[1].price == 10020);
    assert(asks[1].quantity == 125);

    // Final totals.
    assert(book.total_bid_quantity() == 150);
    assert(book.total_ask_quantity() == 200);

    auto top_bid_levels = book.bid_depth(1);

assert(top_bid_levels.size() == 1);
assert(top_bid_levels[0].price == 10000);
assert(top_bid_levels[0].quantity == 50);

auto two_bid_levels = book.bid_depth(2);

assert(two_bid_levels.size() == 2);
assert(two_bid_levels[0].price == 10000);
assert(two_bid_levels[1].price == 9990);

auto top_ask_levels = book.ask_depth(1);

assert(top_ask_levels.size() == 1);
assert(top_ask_levels[0].price == 10010);
assert(top_ask_levels[0].quantity == 75);

assert(book.bid_depth(0).empty());
assert(book.ask_depth(0).empty());

auto excessive_bid_depth = book.bid_depth(100);

assert(excessive_bid_depth.size() == book.bid_level_count());

auto excessive_ask_depth = book.ask_depth(100);

assert(excessive_ask_depth.size() == book.ask_level_count());

const auto metrics = book.metrics(2);

assert(metrics.total_bid_quantity == 150);
assert(metrics.total_ask_quantity == 200);

assert(metrics.imbalance > -0.142858);
assert(metrics.imbalance < -0.142856);

assert(metrics.weighted_mid_price > 10004.28);
assert(metrics.weighted_mid_price < 10004.29);

const auto top_metrics = book.metrics(1);

assert(top_metrics.total_bid_quantity == 50);
assert(top_metrics.total_ask_quantity == 75);

assert(top_metrics.imbalance > -0.200001);
assert(top_metrics.imbalance < -0.199999);

    return 0;
}