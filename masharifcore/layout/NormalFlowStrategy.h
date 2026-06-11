#pragma once

#include "LayoutStrategy.h"

namespace masharif {
    class NormalFlowStrategy final : public LayoutStrategy {
    public:
        void Layout(Node &container, LayoutContext &ctx,
                    float availableWidth, float availableHeight) const override;
    };
}
