#pragma once

#include <masharifcore/structure/BoxInfo.h>

namespace masharif {
    class Node;
    struct LayoutContext;

    /// Stateless layout algorithm. Implementations carry no per-node state: the container
    /// is passed per call and all scratch lives in the LayoutContext, so one immutable
    /// instance per algorithm serves every node.
    class LayoutStrategy {
    public:
        virtual ~LayoutStrategy() = default;

        virtual void Layout(Node &container, LayoutContext &ctx,
                            float availableWidth, float availableHeight) const = 0;

        /// The algorithm for a display type: Block/InlineBlock lay out in normal flow,
        /// everything else as flex.
        [[nodiscard]] static const LayoutStrategy &For(OuterDisplay display) noexcept;
    };
}
