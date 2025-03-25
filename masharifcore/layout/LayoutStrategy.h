#ifndef LAYOUTSTRATEGY_H
#define LAYOUTSTRATEGY_H

#include <masharifcore/macros.h>

#include "Node.h"

namespace
NAMESPACE {
    struct LayoutStrategy {
    protected:
        Node *container;

    public:
        explicit LayoutStrategy(Node *node) : container(node) {
        }

        virtual ~LayoutStrategy() = default;

        virtual bool preLayout(float availableWidth, float availableHeight) = 0;

        virtual bool postLayout() = 0;

        virtual void layout(float availableWidth, float availableHeight) = 0;
    };
}
#endif //LAYOUTSTRATEGY_H
