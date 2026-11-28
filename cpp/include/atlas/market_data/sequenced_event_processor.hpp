#pragma once

#include "atlas/market_data/processor.hpp"
#include "atlas/market_data/sequence_policy.hpp"
#include "atlas/market_data/feed_health.hpp"

namespace atlas::market_data {

class SequencedEventProcessor final {
public:
    explicit SequencedEventProcessor(EventProcessor& processor);

    SequenceDecision process(
        const SequencedMarketEvent& event);

    void reset() noexcept;

    const SequencePolicy& sequence_policy() const noexcept;
    const FeedHealthTracker& feed_health() const noexcept;

private:
    EventProcessor& processor_;
    SequencePolicy sequence_policy_;
    FeedHealthTracker feed_health_;
};

}