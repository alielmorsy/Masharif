#ifndef NODE_H
#define NODE_H
#include <assert.h>
#include <memory>
#include <masharifcore/macros.h>
#include <masharifcore/structure/Style.h>


#include "Layout.h"

#include "NormalFlowStrategy.h"
#include "FlexLayoutStrategy.h"


namespace
_NAMESPACE {
    class Node;
    using SharedNode = std::shared_ptr<Node>;

    class Node {
    public:
        Layout &layout() { return _layout; }

        std::vector<SharedNode> children;
        std::vector<SharedNode> outOfFlowChildren;
        Style &style() { return _style; }

        std::shared_ptr<LayoutStrategy> layoutStrategy;

        explicit Node(const OuterDisplay display = OuterDisplay::Block) {
            setDisplay(display);
        }

        void startUpdatingPositions();

        void positionOutOfFlowChildren();

        void calculate(float availableWidth, float availableHeight);

        void layoutImpl(float availableWidth, float availableHeight);

        void computeDimensions(float availableWidth, float availableHeight);

        void positionOutOfFlowChild(Node *ancestor, float refWidth, float refHeight);

        void handleStickyPosition(float refWidth, float refHeight);

        [[nodiscard]] Node *parent() const {
            return _parent;
        }

        void setParent(Node *parent) {
            _parent = parent;
        }

        void setParent(SharedNode &parent) {
            _parent = parent.get();
        }

        [[nodiscard]] SharedNode firstChild() const {
            return children[0];
        }

        [[nodiscard]] SharedNode lastChild() const {
            return children.back();
        }

        void setDisplay(OuterDisplay display) {
            style().modify<Dimensions>().display = display;
            if (display == OuterDisplay::Block || display == OuterDisplay::InlineBlock) {
                layoutStrategy = std::make_shared<NormalFlowStrategy>(this);
            } else {
                layoutStrategy = std::make_shared<FlexLayoutStrategy>(this);
            }
        }

        void addChild(SharedNode &child) {
            children.push_back(child);
            child->_parent = this;
            _style.dirty = true;
        }

        void removeChild(SharedNode &child);

    private:
        Node *_parent = nullptr;
        Style _style{};
        Layout _layout{};
    };
}

#endif //NODE_H
