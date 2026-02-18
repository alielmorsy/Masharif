#ifndef NORMALFLOWSTRATEGY_H
#define NORMALFLOWSTRATEGY_H



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

    void compute_child_position(Node *child);
} // NAMESPACE

#endif //NORMALFLOWSTRATEGY_H
