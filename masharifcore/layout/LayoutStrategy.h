#ifndef LAYOUTSTRATEGY_H
#define LAYOUTSTRATEGY_H

#include <masharifcore/macros.h>



namespace
_NAMESPACE {
    class Node;
    struct LayoutStrategy {
    protected:
        Node *container;

    public:
        explicit LayoutStrategy(Node *node) : container(node) {
        }

        virtual ~LayoutStrategy() = default;


        virtual void layout(float availableWidth, float availableHeight) = 0;
    };
}
#endif //LAYOUTSTRATEGY_H
