#pragma once

#include <cstdint>

#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

enum class FeedRecoveryState : std::uint8_t {
    Live,
    GapDetected,
    Recovering,
    Recovered
};

struct FeedRecoverySnapshot {
    FeedRecoveryState state;
    SequenceNumber missing_sequence;
    SequenceNumber recovery_sequence;
};

class FeedRecoveryTracker final {
public:
    FeedRecoverySnapshot observe(
        SequenceNumber expected,
        SequenceNumber received) noexcept;

    void begin_recovery() noexcept;

    void reset() noexcept;

    FeedRecoverySnapshot snapshot() const noexcept;

private:
    FeedRecoveryState state_{FeedRecoveryState::Live};
    SequenceNumber missing_sequence_{0};
    SequenceNumber recovery_sequence_{0};
};

} // namespace atlas::market_data