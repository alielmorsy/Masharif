#ifndef NODE_H
#define NODE_H
#include <memory>
#include <masharifcore/macros.h>
#include <masharifcore/structure/Style.h>

#include "Layout.h"
#include "om.h"


namespace
NAMESPACE {
    class Node;
    using SharedNode = std::shared_ptr<Node>;

    class Node {
    public:
        LowLevelNode *om_pre = nullptr; // OM label for pre-order computation
        LowLevelNode *om_post = nullptr;
        SharedNode prevSibling;
        SharedNode nextSibling;

        std::vector<SharedNode> children;
        std::vector<SharedNode> outOfFlowChildren;
        Style &style() { return _style; }
        Layout &layout() { return _layout; }
        void markDeleted() { deleted = true; }

        void layout(float availableWidth, float availableHeight);

        void layoutImpl(float availableWidth, float availableHeight);

        void computeDimensions(float availableWidth, float availableHeight);

        [[nodiscard]] Node *parent() const {
            return _parent;
        }

        [[nodiscard]] SharedNode firstChild() const {
            return children[0];
        }

        [[nodiscard]] SharedNode lastChild() const {
            return children.back();
        }

    private:
        bool deleted = false;
        Node *_parent = nullptr;
        Style _style{};
        Layout _layout{};

    };
}

#endif //NODE_H
