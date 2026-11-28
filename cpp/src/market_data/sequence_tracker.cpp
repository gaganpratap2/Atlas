#include "atlas/market_data/sequence_tracker.hpp"

namespace atlas::market_data{

    SequenceResult SequenceTracker::observe(
        SequenceNumber sequence ) noexcept {
            if (!initialized_){
                initialized_ = true;
                next_expected_ = sequence + 1;

            return {
                .status = SequenceStatus::First,
                .expected = sequence,
                .received = sequence
            };
        }

        const SequenceNumber expected = next_expected_;

        if (sequence == expected) {
            next_expected_ = sequence + 1;

            return {
                .status = SequenceStatus::InOrder,
                .expected = expected,
                .received = sequence
            }; 
        }

        if(sequence > expected){
            next_expected_ = sequence + 1;
            return {
                .status = SequenceStatus::Gap,
                .expected = expected,
                .received = sequence
        }   ;
        }

        if (sequence + 1 == expected) {
        return {
            .status = SequenceStatus::Duplicate,
            .expected = expected,
            .received = sequence
        };
    }

    return {
        .status = SequenceStatus::OutOfOrder,
        .expected = expected,
        .received = sequence
    };
}

void SequenceTracker::reset() noexcept {
    initialized_ = false;
    next_expected_ = 0;
}

bool SequenceTracker::initialized() const noexcept {
    return initialized_;
}

SequenceNumber SequenceTracker::next_expected() const noexcept {
    return next_expected_;
}
}