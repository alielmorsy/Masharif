#pragma once

#include "LayoutStrategy.h"
#include "masharifcore/macros.h"
#include <vector>

namespace masharif {
    class NormalFlowStrategy : public LayoutStrategy {
    public:
        explicit NormalFlowStrategy(Node *node)
            : LayoutStrategy(node) {
        }

        ~NormalFlowStrategy() = default;


        void layout(float availableWidth, float availableHeight) override;

        void layoutLine(std::vector<Node *> &lines, float y);
    };
}
