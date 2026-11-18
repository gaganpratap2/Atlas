#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>

#include "atlas/market_data/binary_recorder.hpp"
#include "atlas/market_data/binary_replayer.hpp"

int main() {
    using namespace atlas::market_data;

    const std::filesystem::path path =
        "atlas_replay_benchmark.bin";

    constexpr std::size_t iterations = 1'000'000;

    {
        BinaryMarketDataRecorder recorder;
        recorder.open(path);

        for (std::size_t i = 0; i < iterations; ++i) {
            recorder.record(
                Trade{
                    1,
                    10117,
                    100,
                    static_cast<Timestamp>(
                        1'000'000 + i
                    )
                }
            );
        }

        recorder.close();
    }

    BinaryMarketDataReplayer replayer;
    replayer.open(path);

    std::size_t count = 0;
    std::uint64_t checksum = 0;

    const auto start =
        std::chrono::steady_clock::now();

    replayer.replay(
        [&](const MarketEvent& event) {
            const auto& trade =
                std::get<Trade>(event);

            checksum += trade.quantity;
            ++count;
        }
    );

    const auto end =
        std::chrono::steady_clock::now();

    replayer.close();

    const auto elapsed =
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(end - start);

    const double avg_ns =
        static_cast<double>(elapsed.count()) /
        static_cast<double>(count);

    std::cout
        << "Events: " << count << '\n'
        << "Elapsed ns: " << elapsed.count() << '\n'
        << "Avg ns/event: " << avg_ns << '\n'
        << "Checksum: " << checksum << '\n';

    std::filesystem::remove(path);

    return 0;
}