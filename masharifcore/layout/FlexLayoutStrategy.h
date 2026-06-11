#pragma once

#include "LayoutStrategy.h"

namespace masharif {
    class FlexLayoutStrategy final : public LayoutStrategy {
    public:
        void Layout(Node &container, LayoutContext &ctx,
                    float availableWidth, float availableHeight) const override;

    private:
        /// One flex solve for one container; defined in the .cpp. Nested so it shares this
        /// strategy's friend access to Node ([class.access.nest]).
        class Solver;
    };
}
