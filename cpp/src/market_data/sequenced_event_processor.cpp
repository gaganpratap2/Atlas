#include "atlas/market_data/sequenced_event_processor.hpp"

namespace atlas::market_data {

SequencedEventProcessor::SequencedEventProcessor(
    EventProcessor& processor)
    : processor_(processor) {}

SequenceDecision SequencedEventProcessor::process(
    const SequencedMarketEvent& event) {

    const auto decision =
        sequence_policy_.evaluate(
            event.sequence_number);

    feed_health_.observe(decision);

    if (decision.action == SequenceAction::Accept) {
        processor_.process(event.event);
    }

    return decision;
}

void SequencedEventProcessor::reset() noexcept {
    sequence_policy_.reset();
    feed_health_.reset();
}

const SequencePolicy&
SequencedEventProcessor::sequence_policy() const noexcept {
    return sequence_policy_;
}

const FeedHealthTracker&
SequencedEventProcessor::feed_health() const noexcept {
    return feed_health_;
}

}




// git commit -m"Add FeedRecoveryTracker to monitor feed recovery states"