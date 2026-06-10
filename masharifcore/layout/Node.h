#pragma once

#include <cmath>
#include <memory>
#include <masharifcore/structure/Style.h>


#include "Layout.h"

#include "NormalFlowStrategy.h"
#include "FlexLayoutStrategy.h"


namespace masharif {
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

        // Re-lay-out this node's subtree against an already-resolved, definite
        // border-box size (decided by a flex parent's main-axis resolution and
        // updateCrossSize's cross-axis stretch). Needed because the flex-basis
        // phase measures AUTO-size items against NaN and collapses their
        // subtrees to 0; this drives them back out at the final size.
        void layoutContentsWithDefiniteSize(float borderBoxWidth, float borderBoxHeight);

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

        void addChild(const SharedNode &child) {
            children.push_back(child);
            child->_parent = this;
            _style.dirty = true;
        }

        void removeChild(SharedNode &child);

        // Mark this node dirty and flag every ancestor as having a dirty descendant,
        // up to the root. Required because layoutImpl only inspects DIRECT children's
        // dirty state, so without this a deep change would be stranded and never
        // re-laid-out. The upward counterpart to markSubtreeDirtyForRelayout.
        void markDirtyToRoot();

        // True only while layoutContentsWithDefiniteSize drives the strategy: the parent
        // has already resolved this node's border box (main = flex-basis/grow, cross =
        // stretch), so the flex strategy must NOT re-derive an AUTO main-axis size by
        // shrink-to-fit — doing so re-collapses a cross-stretched container (e.g. a Row of
        // flex-grow children) back to its 0 flex-basis and starves the grow distribution.
        [[nodiscard]] bool mainSizeIsDefinite() const { return _mainSizeDefinite; }

    private:
        // Force this node and every descendant dirty so a relayout actually
        // re-runs (layoutImpl/strategies are gated on _style.dirty).
        void markSubtreeDirtyForRelayout();

        Node *_parent = nullptr;
        Style _style{};
        Layout _layout{};

        // Precise incremental-layout state. _descendantDirty is set by markDirtyToRoot
        // on every ancestor of a changed node; a node with neither _style.dirty nor
        // _descendantDirty (and unchanged available space) can reuse its cached layout.
        bool _descendantDirty = false;

        // Set by layoutContentsWithDefiniteSize for the duration of its strategy call; see
        // mainSizeIsDefinite().
        bool _mainSizeDefinite = false;

        // Memo of the inputs the last solve ran against, so a clean subtree is only
        // re-solved when the space it is measured against actually changes. NAN means
        // "never laid out", which forces the first solve (the node also starts dirty).
        float _lastAvailW = NAN, _lastAvailH = NAN;   ///< layoutImpl available space
        float _lastDefW   = NAN, _lastDefH   = NAN;   ///< layoutContentsWithDefiniteSize size
    };
}
