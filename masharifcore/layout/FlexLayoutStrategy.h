#pragma once

#include <memory>

#include "LayoutStrategy.h"

#include "masharifcore/macros.h"
#include <vector>

namespace masharif {
    class Node;

    struct FlexLine {
        std::vector<Node *> flexItems;
        int numberOfAutoMargin;
        float takenSize;
        float totalFlexGrow;
        float totalFlexShrinkScaledFactors;
        float crossSize;
    };

    class FlexLayoutStrategy : public LayoutStrategy {
    public:
        explicit FlexLayoutStrategy(Node *node)
            : LayoutStrategy(node) {
        }

        ~FlexLayoutStrategy() override = default;

        void layout(float availableWidth, float availableHeight) override;

    private:
        FlexLine calculateFlexLine(std::vector<Node *>::iterator &iterator,
                                   std::vector<Node *>::iterator &end, float totalMainAxisSize);

        void resolveFlexibleLengths(FlexLine &line, float availableSpace, bool isRow, float availableWidth,
                                    float availableHeight);

        void updateCrossSize(std::vector<FlexLine> &lines);
    };
}
