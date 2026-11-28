#pragma once

#include <cstdint>

#include "atlas/market_data/sequence_tracker.hpp"

namespace atlas::market_data {

enum class SequenceAction : std::uint8_t {
    Accept,
    IgnoreDuplicate,
    RejectOutOfOrder,
    GapDetected
};

struct SequenceDecision {
    SequenceAction action;
    SequenceResult result;
};

class SequencePolicy final {
public:
    SequenceDecision evaluate(SequenceNumber sequence) noexcept;

    void reset() noexcept;

    const SequenceTracker& tracker() const noexcept;

private:
    SequenceTracker tracker_;
};

}