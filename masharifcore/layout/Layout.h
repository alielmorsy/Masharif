#pragma once

#include <cstdint>

namespace masharif {
    struct Layout {
        /// Absolute position, re-derived each frame as parentAbs + local; idempotent so a
        /// skipped (clean) subtree still lands correctly when an ancestor moves.
        float ComputedX = 0;
        float ComputedY = 0;

        /// Position relative to the parent's content origin, written by the layout strategies
        /// and kept stable across reuse (ComputedX/Y are derived from it, not accumulated).
        float LocalX = 0;
        float LocalY = 0;

        float ComputedWidth = 0.0f;
        float ComputedHeight = 0.0f;
        float ComputedFlexBasis = 0.0f;

        /// Number of times a layout strategy actually ran for this node (monotonic across
        /// frames). Cheap observability: benchmarks assert on it to prove incremental
        /// behaviour independent of wall-clock noise.
        std::uint32_t StrategyRuns = 0;
    };
}
