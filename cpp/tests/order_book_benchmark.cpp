#include <cassert>
#include <cstdint>
#include <iostream>
#include <chrono>

#include "atlas/order_book/order_book.hpp"

int main() {
    using namespace atlas::market_data;
    using namespace atlas::order_book;

    constexpr std::uint64_t iterations = 1'000'000;

    OrderBook book(1);

    const auto start = std::chrono::steady_clock::now();

    std::uint64_t checksum = 0;

    for (std::uint64_t i = 0; i < iterations; ++i) {
        const Price bid_price = 10'000 + static_cast<Price>(i % 100);
        const Price ask_price = 10'100 + static_cast<Price>(i % 100);

        book.apply(OrderBookUpdate{
            1,
            Side::Buy,
            bid_price,
            100 + (i % 50),
            i + 1
        });

        book.apply(OrderBookUpdate{
            1,
            Side::Sell,
            ask_price,
            200 + (i % 50),
            i + 1
        });

        const auto top = book.top_of_book();

        assert(top);

        checksum += static_cast<std::uint64_t>(top->bid.price);
        checksum += static_cast<std::uint64_t>(top->ask.price);
    }

    const auto end = std::chrono::steady_clock::now();

    const auto elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();

    const double ns_per_iteration =
        static_cast<double>(elapsed_ns) /
        static_cast<double>(iterations);

    std::cout << "Iterations: " << iterations << '\n';
    std::cout << "Elapsed ns: " << elapsed_ns << '\n';
    std::cout << "Avg ns/iteration: " << ns_per_iteration << '\n';
    std::cout << "Checksum: " << checksum << '\n';

    return 0;
}