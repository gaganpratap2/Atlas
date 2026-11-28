#pragma once

#include <cstdint>

#include "atlas/market_data/sequence_tracker.hpp"

#include "atlas/market_data/sequence_policy.hpp"

namespace atlas::market_data {

enum class FeedHealth : std::uint8_t {
    Healthy,
    GapDetected,
    OutOfOrder
};

struct FeedHealthSnapshot {
    FeedHealth health;
    SequenceNumber next_expected;
    SequenceNumber last_received;

    std::uint64_t accepted_count;
    std::uint64_t duplicate_count;
    std::uint64_t gap_count;
    std::uint64_t out_of_order_count;
};

class FeedHealthTracker final {
public:
    FeedHealthSnapshot observe(
        const SequenceDecision& decision) noexcept;

    FeedHealthSnapshot snapshot() const noexcept;

    void reset() noexcept;

private:
    FeedHealth health_{FeedHealth::Healthy};

    SequenceNumber last_received_{0};

    std::uint64_t accepted_count_{0};
    std::uint64_t duplicate_count_{0};
    std::uint64_t gap_count_{0};
    std::uint64_t out_of_order_count_{0};

    SequenceNumber next_expected_{0};
};

}