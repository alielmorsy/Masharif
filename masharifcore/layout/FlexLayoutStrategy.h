#ifndef FLEXLAYOUTSTRATEGY_H
#define FLEXLAYOUTSTRATEGY_H
#include <memory>

#include "LayoutStrategy.h"

#include "masharifcore/macros.h"
#include <vector>

namespace
_NAMESPACE {
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
        FlexLine calculateFlexLine(std::vector<std::shared_ptr<Node> >::iterator &iterator,
                                   std::vector<std::shared_ptr<Node> >::iterator &end, float totalMainAxisSize,
                                   size_t lineCount);

        void updateCrossSize(std::vector<FlexLine> &lines);
    };
}


#endif //FLEXLAYOUTSTRATEGY_H
