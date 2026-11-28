#include "atlas/market_data/sequence_policy.hpp"

namespace atlas::market_data {

SequenceDecision SequencePolicy::evaluate(
    SequenceNumber sequence) noexcept {

    const auto result = tracker_.observe(sequence);

    switch (result.status) {
    case SequenceStatus::First:
    case SequenceStatus::InOrder:
        return {
            .action = SequenceAction::Accept,
            .result = result
        };

    case SequenceStatus::Duplicate:
        return {
            .action = SequenceAction::IgnoreDuplicate,
            .result = result
        };

    case SequenceStatus::OutOfOrder:
        return {
            .action = SequenceAction::RejectOutOfOrder,
            .result = result
        };

    case SequenceStatus::Gap:
        return {
            .action = SequenceAction::GapDetected,
            .result = result
        };
    }

    return {
        .action = SequenceAction::RejectOutOfOrder,
        .result = result
    };
}

void SequencePolicy::reset() noexcept {
    tracker_.reset();
}

const SequenceTracker& SequencePolicy::tracker() const noexcept {
    return tracker_;
}

}