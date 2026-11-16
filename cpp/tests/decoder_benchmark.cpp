#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "atlas/market_data/decoder.hpp"

int main() {
    using namespace atlas::market_data;

    const std::vector<std::uint8_t> message{
        31, 0,
        1,

        1, 0, 0, 0,

        0x85, 0x27, 0, 0,
        0, 0, 0, 0,

        100, 0, 0, 0,
        0, 0, 0, 0,

        0x40, 0x42, 0x0f, 0,
        0, 0, 0, 0
    };

    constexpr std::size_t iterations = 1'000'000;

    Decoder decoder;

    std::size_t checksum = 0;

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        const auto event = decoder.decode(message);

        const auto& trade =
            std::get<Trade>(event);

        checksum += trade.quantity;
    }

    const auto end = std::chrono::steady_clock::now();

    const auto elapsed =
        std::chrono::duration_cast<
            std::chrono::nanoseconds>(
            end - start);

    std::cout
        << "Iterations: " << iterations << '\n'
        << "Elapsed ns: " << elapsed.count() << '\n'
        << "Avg ns/event: "
        << static_cast<double>(elapsed.count())
            / iterations
        << '\n'
        << "Checksum: " << checksum << '\n';

    return 0;
}