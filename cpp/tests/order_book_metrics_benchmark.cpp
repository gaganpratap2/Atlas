#include <chrono>
#include <cstdint>
#include <iostream>

#include "atlas/order_book/order_book.hpp"

int main() {
    using namespace atlas::market_data;
    using namespace atlas::order_book;

    constexpr std::uint64_t iterations = 1'000'000;
    constexpr std::size_t levels = 5;

    OrderBook book(1);

    for (int i = 0; i < 20; ++i) {
        book.apply(OrderBookUpdate{
            1,
            Side::Buy,
            10'000 - i * 10,
            static_cast<Quantity>(100 + i),
            static_cast<Timestamp>(i + 1)
        });

        book.apply(OrderBookUpdate{
            1,
            Side::Sell,
            10'100 + i * 10,
            static_cast<Quantity>(200 + i),
            static_cast<Timestamp>(i + 1)
        });
    }

    std::uint64_t checksum = 0;

    const auto start = std::chrono::steady_clock::now();

    for (std::uint64_t i = 0; i < iterations; ++i) {
        const auto top = book.top_of_book();
        const auto metrics = book.metrics(levels);

        checksum += static_cast<std::uint64_t>(top->bid.price);
        checksum += static_cast<std::uint64_t>(top->ask.price);
        checksum += metrics.total_bid_quantity;
        checksum += metrics.total_ask_quantity;
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
    std::cout << "Depth levels: " << levels << '\n';
    std::cout << "Elapsed ns: " << elapsed_ns << '\n';
    std::cout << "Avg ns/iteration: " << ns_per_iteration << '\n';
    std::cout << "Checksum: " << checksum << '\n';

    return 0;
}