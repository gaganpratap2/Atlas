#pragma once

#include <cstdint>
#include "atlas/market_data/types.hpp"

namespace atlas::market_data {

enum class SequenceStatus : std::uint8_t {
    First,
    InOrder,
    Gap,
    Duplicate,
    OutOfOrder
};

struct SequenceResult {
    SequenceStatus status;
    SequenceNumber expected;
    SequenceNumber received;
};

class SequenceTracker final {
public:
    SequenceResult observe(SequenceNumber sequence) noexcept;

    void reset() noexcept;

    bool initialized() const noexcept;
    SequenceNumber next_expected() const noexcept;

private:
    bool initialized_{false};
    SequenceNumber next_expected_{0};
    SequenceNumber last_received_{0};
};

}