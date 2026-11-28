
#include "atlas/market_data/sequence_tracker.hpp"

namespace atlas::market_data {

SequenceResult SequenceTracker::observe(
    SequenceNumber sequence) noexcept {

    if (!initialized_) {
        initialized_ = true;
        last_received_ = sequence;
        next_expected_ = sequence + 1;

        return {
            SequenceStatus::First,
            sequence,
            sequence
        };
    }

    if (sequence == next_expected_) {
        const SequenceNumber expected = next_expected_;

        last_received_ = sequence;
        next_expected_ = sequence + 1;

        return {
            SequenceStatus::InOrder,
            expected,
            sequence
        };
    }

    if (sequence > next_expected_) {
        // Gap detected. Preserve the missing sequence.
        return {
            SequenceStatus::Gap,
            next_expected_,
            sequence
        };
    }

    // Any sequence below next_expected_ is a duplicate
    // under the current Day 9 Step 6 contract.
    return {
        SequenceStatus::Duplicate,
        next_expected_,
        sequence
    };
}

void SequenceTracker::reset() noexcept {
    initialized_ = false;
    next_expected_ = 0;
    last_received_ = 0;
}

bool SequenceTracker::initialized() const noexcept {
    return initialized_;
}

SequenceNumber SequenceTracker::next_expected() const noexcept {
    return next_expected_;
}

} // namespace atlas::market_data
