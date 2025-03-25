#ifndef NORMALFLOWSTRATEGY_H
#define NORMALFLOWSTRATEGY_H

#include "LayoutStrategy.h"
#include "masharifcore/macros.h"

namespace
NAMESPACE {
    class NormalFlowStrategy : public LayoutStrategy {
    public:
        explicit NormalFlowStrategy(Node *node)
            : LayoutStrategy(node) {
        }

        ~NormalFlowStrategy() override;

        bool preLayout(float availableWidth, float availableHeight) override;

        bool postLayout() override;

        void layout(float availableWidth, float availableHeight) override;
    };
} // NAMESPACE

#endif //NORMALFLOWSTRATEGY_H
