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

        /// Re-lay-out this subtree at an already-resolved definite border-box size (set by the
        /// flex parent's main-axis resolution + updateCrossSize's cross-axis stretch). Needed
        /// because the flex-basis phase measures AUTO items against NaN and collapses them to 0.
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

        /// Mark this node dirty and flag every ancestor's _descendantDirty up to the root.
        /// layoutImpl only inspects DIRECT children, so a deep change needs this to be seen.
        void markDirtyToRoot();

        /// True only while layoutContentsWithDefiniteSize drives the strategy: the parent has
        /// resolved this node's border box, so the flex strategy must NOT shrink-to-fit an AUTO
        /// main axis (that would re-collapse a cross-stretched grow container to its 0 basis).
        [[nodiscard]] bool mainSizeIsDefinite() const { return _mainSizeDefinite; }

    private:
        /// Force this node and every descendant dirty so a relayout actually re-runs
        /// (layoutImpl/strategies are gated on _style.dirty).
        void markSubtreeDirtyForRelayout();

        Node *_parent = nullptr;
        Style _style{};
        Layout _layout{};

        /// Set by markDirtyToRoot on every ancestor of a changed node. A node with neither
        /// _style.dirty nor _descendantDirty (and unchanged space) reuses its cached layout.
        bool _descendantDirty = false;

        /// Set by layoutContentsWithDefiniteSize for the duration of its strategy call;
        /// see mainSizeIsDefinite().
        bool _mainSizeDefinite = false;

        /// Set by layoutImpl when it shrink-wraps an AUTO main axis, cleared by the definite-size
        /// re-layout. Forces that re-layout to run even when its _lastDef size is unchanged —
        /// otherwise a shrink-wrapped grow container would reuse its collapsed layout.
        bool _collapsedSinceDefinite = false;

        /// Memo of the space each pass last ran against, so a clean subtree is re-solved only
        /// when that space changes. NAN means "never laid out" and forces the first solve.
        float _lastAvailW = NAN, _lastAvailH = NAN;   ///< layoutImpl available space
        float _lastDefW   = NAN, _lastDefH   = NAN;   ///< layoutContentsWithDefiniteSize size

        /// Content-box size from the last full layoutImpl run (before any parent flex grow/shrink).
        /// Restored on the reuse early-out so a clean child reports its content size for flex-basis
        /// derivation rather than a transient grown/collapsed value left by an ancestor's resolve.
        float _implW = NAN, _implH = NAN;
    };
}
