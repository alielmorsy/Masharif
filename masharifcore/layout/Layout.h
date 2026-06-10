#pragma once

#include <memory>
#include <masharifcore/macros.h>

namespace masharif {
    struct Layout {
        /// Absolute position, re-derived each frame as parentAbs + local; idempotent so a
        /// skipped (clean) subtree still lands correctly when an ancestor moves.
        float computedX = 0;
        float computedY = 0;

        /// Position relative to the parent's content origin, written by the layout strategies
        /// and kept stable across reuse (computedX/Y are derived from it, not accumulated).
        float localX = 0;
        float localY = 0;

        float computedWidth = 0.0f;
        float computedHeight = 0.0f;
        float computedFlexBasis = 0.0f;
    };
}
