#include "atlas/market_data/feed_recovery.hpp"

namespace atlas::market_data {

FeedRecoverySnapshot FeedRecoveryTracker::observe(
    SequenceNumber expected,
    SequenceNumber received) noexcept {

    if (received == expected) {
        if (state_ == FeedRecoveryState::GapDetected ||
            state_ == FeedRecoveryState::Recovering) {

            state_ = FeedRecoveryState::Recovered;
            recovery_sequence_ = received;
        }

        return snapshot();
    }

    if (received > expected) {
        state_ = FeedRecoveryState::GapDetected;
        missing_sequence_ = expected;

        return snapshot();
    }

    return snapshot();
}

void FeedRecoveryTracker::begin_recovery() noexcept {
    if (state_ == FeedRecoveryState::GapDetected) {
        state_ = FeedRecoveryState::Recovering;
    }
}

void FeedRecoveryTracker::reset() noexcept {
    state_ = FeedRecoveryState::Live;
    missing_sequence_ = 0;
    recovery_sequence_ = 0;
}

FeedRecoverySnapshot FeedRecoveryTracker::snapshot() const noexcept {
    return {
        state_,
        missing_sequence_,
        recovery_sequence_
    };
}

} // namespace atlas::market_data