#include "atlas/market_data/feed_health.hpp"

namespace atlas::market_data {


FeedHealthSnapshot FeedHealthTracker::observe(
    const SequenceDecision& decision) noexcept {

    switch (decision.action) {

    case SequenceAction::Accept:
        ++accepted_count_;
        last_received_ = decision.result.received;
        next_expected_ = decision.result.expected + 1;
        break;

    case SequenceAction::IgnoreDuplicate:
        ++duplicate_count_;
        last_received_ = decision.result.received;
        break;

    case SequenceAction::GapDetected:
        ++gap_count_;
        last_received_ = decision.result.received;
        next_expected_ = decision.result.expected;
        health_ = FeedHealth::GapDetected;
        break;

    case SequenceAction::RejectOutOfOrder:
        ++out_of_order_count_;
        last_received_ = decision.result.received;
        health_ = FeedHealth::OutOfOrder;
        break;
    }

    return snapshot();
}


FeedHealthSnapshot FeedHealthTracker::snapshot() const noexcept {
    return {
        .health = health_,
        .next_expected = next_expected_,
        .last_received = last_received_,
        .accepted_count = accepted_count_,
        .duplicate_count = duplicate_count_,
        .gap_count = gap_count_,
        .out_of_order_count = out_of_order_count_
    };
}

void FeedHealthTracker::reset() noexcept {
    health_ = FeedHealth::Healthy;
    last_received_ = 0;
    accepted_count_ = 0;
    duplicate_count_ = 0;
    gap_count_ = 0;
    out_of_order_count_ = 0;
    next_expected_ = 0;
}

}