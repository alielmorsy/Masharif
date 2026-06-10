//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "Node.h"

#include <algorithm>
#include <vector>

using namespace masharif;

// Treat NaN as equal to NaN so the layout memo recognises an unchanged AUTO
// placeholder size (NaN) as a cache hit rather than a perpetual miss.
static bool SameSize(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    return a == b;
}

void Node::removeChild(SharedNode &child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
        _style.dirty = true;
    }
}

void Node::markDirtyToRoot() {
    _style.dirty = true;
    for (Node *p = _parent; p && !p->_descendantDirty; p = p->_parent) {
        p->_descendantDirty = true;
    }
}


static Node *findRelativeParent(Node *child) {
    auto current = child;
    while (true) {
        auto position = child->style().dimensions().position;
        if (position == PositionType::Relative)
            return current;
        if (!current->parent()) return current;
        current = current->parent();
    }
}

void Node::startUpdatingPositions() {
    // End-of-frame: this node's layout (and that of everything below it) is now
    // committed, so clear the dirty channels here rather than in layoutImpl. Clearing
    // mid-solve would hide a subtree's change from the later definite-size pass.
    _style.dirty = false;
    _descendantDirty = false;

    const float absX = _layout.computedX;
    const float absY = _layout.computedY;
    for (auto &child: children) {
        auto &position = child->style().dimensions().position;
        if (position != PositionType::Static &&
            position != PositionType::Relative) {
            child->startUpdatingPositions();
            continue;
        }
        DEF_NODE_LAYOUT(child);
        // Derive the absolute position from the stable local position instead of
        // accumulating in place. This is idempotent, so a clean subtree whose re-solve
        // was skipped still lands at the right absolute coordinates when an ancestor moves.
        childLayout.computedX = absX + childLayout.localX;
        childLayout.computedY = absY + childLayout.localY;

        child->startUpdatingPositions();
        child->positionOutOfFlowChildren();
    }
}

void Node::positionOutOfFlowChildren() {
    for (const auto &child: outOfFlowChildren) {
        if constexpr (true) {
            Node *ancestor = nullptr;
            float refWidth, refHeight;
            auto position = child->style().dimensions().position;
            if (position == PositionType::Fixed) {
                // Fixed uses viewport as containing block
                //TODO: Get viewport
                refWidth = 0;
                refHeight = 0;
            } else {
                // Find nearest positioned ancestor for absolute/sticky
                ancestor = findRelativeParent(child.get());
                refWidth = ancestor->_layout.computedWidth;
                refHeight = ancestor->_layout.computedHeight;
            }

            // Use viewport dimensions for fixed, ancestor's for others
            const float childAvailableWidth = refWidth;
            const float childAvailableHeight = refHeight;

            // Layout child with correct available space and viewport
            child->layoutImpl(childAvailableWidth, childAvailableHeight);

            // Position the child
            if (position == PositionType::Sticky) {
                child->handleStickyPosition(refWidth, refWidth);
            } else {
                child->positionOutOfFlowChild(ancestor, refWidth, refHeight);
            }
        }
    }
    outOfFlowChildren.clear();
}

void Node::calculate(float availableWidth, float availableHeight) {
    layoutImpl(availableWidth, availableHeight);
    // The root's local origin is its absolute origin (no parent to offset against);
    // startUpdatingPositions then derives every descendant's absolute position from it.
    _layout.computedX = _layout.localX;
    _layout.computedY = _layout.localY;
    startUpdatingPositions();
}

void Node::layoutImpl(float availableWidth, float availableHeight) {
    if (_style.dimensions().display == OuterDisplay::None) {
        _layout.computedWidth = _layout.computedHeight = 0.0f;
        return;
    }

    const bool spaceSame = SameSize(availableWidth, _lastAvailW) &&
                           SameSize(availableHeight, _lastAvailH);

    // A directly-dirtied child still forces a re-solve even without an explicit
    // markDirtyToRoot call, preserving the engine's standalone contract (set a
    // child dirty, call calculate). markDirtyToRoot covers the deep case.
    bool anyDirectChildDirty = false;
    for (const auto &child: children) {
        if (child->_style.dirty) { anyDirectChildDirty = true; break; }
    }

    // Full reuse: nothing in this subtree changed and the available space is identical
    // to the last solve, so the cached _layout (incl. computedFlexBasis and local
    // positions) is still valid. dirty is cleared at end of frame, not here.
    if (spaceSame && !_style.dirty && !_descendantDirty && !anyDirectChildDirty)
        return;

    _lastAvailW = availableWidth;
    _lastAvailH = availableHeight;

    if (_style.dirty || !spaceSame)
        computeDimensions(availableWidth, availableHeight);

    layoutStrategy->layout(availableWidth, availableHeight);
    if (auto &position = _style.dimensions().position; position == PositionType::Relative) {
        auto &offset = _style.offsets();
        _layout.localX += offset.left.resolveValue(availableWidth) - offset.right.resolveValue(availableWidth);
        _layout.localY += offset.top.resolveValue(availableHeight) - offset.bottom.resolveValue(availableHeight);
    }
    auto &containerStyle = _style;
    // Only override height for Block/InlineBlock. Flex layout handles its own height.
    auto display = containerStyle.dimensions().display;
    bool isBlock = display == OuterDisplay::Block || display == OuterDisplay::InlineBlock;

    if (isBlock && containerStyle.dimensions().height.unit == CSSUnit::AUTO) {
        float maxChildBottom = 0.0f;
        for (const auto &child: children) {
            DEF_NODE_LAYOUT(child);
            DEF_NODE_STYLE(child);
            const auto position = childStyle.dimensions().position;
            auto &childMargin = childStyle.margin();
            if (position == PositionType::Static || position == PositionType::Relative) {
                maxChildBottom = std::max(maxChildBottom,
                                          childLayout.localY + childLayout.computedHeight + childMargin.bottom + childMargin.top);
            }
        }

        auto &border = containerStyle.border();
        _layout.computedHeight = maxChildBottom + _style.padding().top + _style.padding().bottom
                                 +
                                 border.widthTop + border.widthBottom;
    }
}

void Node::markSubtreeDirtyForRelayout() {
    _style.dirty = true;
    for (auto &child: children) {
        child->markSubtreeDirtyForRelayout();
    }
}

void Node::layoutContentsWithDefiniteSize(float borderBoxWidth, float borderBoxHeight) {
    if (_style.dimensions().display == OuterDisplay::None) return;
    if (children.empty()) return;                       // leaf: nothing to re-lay-out
    if (std::isnan(borderBoxWidth) || std::isnan(borderBoxHeight)) return;

    // Adopt the size decided by the flex parent (main axis) and updateCrossSize
    // (cross axis). These are border-box values.
    _layout.computedWidth = borderBoxWidth;
    _layout.computedHeight = borderBoxHeight;

    // Reuse the cached subtree layout when nothing in this subtree changed and the
    // definite size matches the last definite-size pass. This replaces the old
    // unconditional markSubtreeDirtyForRelayout(), which re-solved every subtree on
    // every frame and defeated all caching. When the size differs (e.g. an AUTO child
    // collapsed against NaN during the flex-basis phase) the layoutImpl memo below
    // misses on the space change and re-expands the subtree, so no force-dirty needed.
    bool anyDirectChildDirty = false;
    for (const auto &child: children) {
        if (child->_style.dirty) { anyDirectChildDirty = true; break; }
    }
    if (!_style.dirty && !_descendantDirty && !anyDirectChildDirty &&
        SameSize(borderBoxWidth, _lastDefW) && SameSize(borderBoxHeight, _lastDefH))
        return;
    _lastDefW = borderBoxWidth;
    _lastDefH = borderBoxHeight;

    // Convert border-box -> content-box for the available space handed to the
    // strategy. computeDimensions uses content-box semantics (it re-adds
    // padding+border), so subtract them here exactly once. Mirrors the
    // horizontal/vertical padding sums in computeDimensions().
    auto &padding = _style.padding();
    auto &border = _style.border();
    float horizontal = padding.left + padding.right + border.widthLeft + border.widthRight;
    float vertical = padding.top + padding.bottom + border.widthTop + border.widthBottom;
    float contentWidth = std::max(0.0f, borderBoxWidth - horizontal);
    float contentHeight = std::max(0.0f, borderBoxHeight - vertical);

    // Drive the strategy DIRECTLY rather than via layoutImpl: layoutImpl would
    // re-apply the Block AUTO-height override (see below), discarding the
    // stretched/grown size we just adopted.
    layoutStrategy->layout(contentWidth, contentHeight);

    // Re-assert the definite border box (a flex strategy may rewrite it from the
    // content-box available size; guard against rounding drift).
    _layout.computedWidth = borderBoxWidth;
    _layout.computedHeight = borderBoxHeight;
}

void Node::computeDimensions(float availableWidth, float availableHeight) {
    auto &dimensions = _style.dimensions();
    auto &width = dimensions.width;
    auto &height = dimensions.height;
    auto &minWidth = dimensions.minWidth;
    auto &maxWidth = dimensions.maxWidth;

    auto &minHeight = dimensions.minHeight;
    auto &maxHeight = dimensions.maxHeight;
    auto display = dimensions.display;
    float computedWidth = NAN, computedHeight = NAN;
    if (width.unit == CSSUnit::PX) {
        computedWidth = width.value;
    } else if (width.unit == CSSUnit::PERCENT) {
        computedWidth = availableWidth * (width / 100.0f);
    } else {
        if (display == OuterDisplay::Block || display == OuterDisplay::Flex) {
            auto &margin = _style.margin();
            auto &padding = _style.padding();
            auto &border = _style.border();
            float totalHorizontal = margin.left.resolveValue(availableWidth) +
                                    margin.right.resolveValue(availableWidth) +
                                    padding.left + padding.right +
                                    border.widthLeft + border.widthRight;
            if (std::isnan(availableWidth)) {
                computedWidth = 0.0f;
            } else {
                computedWidth = std::max(0.0f, availableWidth - totalHorizontal);
            }
        } else if (display == OuterDisplay::Inline || display == OuterDisplay::InlineBlock) {
            computedWidth = 100.0f; // Placeholder
            for (const auto &child: children) {
                child->layoutImpl(availableWidth, availableHeight);
                DEF_NODE_LAYOUT(child);
                DEF_NODE_STYLE(child);
                computedWidth = std::max(computedWidth, childLayout.computedWidth +
                                                        childStyle.margin().left.resolveValue(0) + childStyle.margin().
                                                        right.resolveValue(0));
            }
        }
    }
    auto &stylePadding = _style.padding();
    auto &styleBorder = _style.border();
    float horizontalPadding = stylePadding.left + stylePadding.right +
                              styleBorder.widthLeft + styleBorder.widthRight;
    float verticalPadding = stylePadding.top + stylePadding.bottom +
                            styleBorder.widthTop + styleBorder.widthBottom;

    // An explicit PX/PERCENT size specifies the BORDER box (border-box sizing):
    // padding and border inset the content rather than enlarging the element. The
    // AUTO branches above instead produced a *content* size (fill = available
    // minus padding/border, or shrink-to-fit content), so only those need padding
    // and border re-added to reach the border box.
    const bool widthIsExplicit = (width.unit == CSSUnit::PX || width.unit == CSSUnit::PERCENT);
    const bool heightIsExplicit = (height.unit == CSSUnit::PX || height.unit == CSSUnit::PERCENT);

    if (!std::isnan(computedWidth)) {
        if (minWidth.unit != CSSUnit::AUTO)
            computedWidth = std::max(computedWidth, minWidth.resolveValue(availableWidth));
        if (maxWidth.unit != CSSUnit::AUTO)
            computedWidth = std::min(computedWidth, maxWidth.resolveValue(availableWidth));
        if (!widthIsExplicit)
            computedWidth += horizontalPadding;
    }


    if (height.unit == CSSUnit::PX) {
        computedHeight = height.value;
    } else if (height.unit == CSSUnit::PERCENT) {
        if (!std::isnan(availableHeight)) {
            computedHeight = availableHeight * (height.value / 100.0f);
        }
    }
    if (!std::isnan(computedHeight)) {
        if (minHeight.unit != CSSUnit::AUTO)
            computedHeight = std::max(computedHeight, minHeight.resolveValue(availableHeight));
        if (maxHeight.unit != CSSUnit::AUTO)
            computedHeight = std::min(computedHeight, maxHeight.resolveValue(availableHeight));
        if (!heightIsExplicit)
            computedHeight += verticalPadding;
    }

    _layout.computedWidth = computedWidth;
    _layout.computedHeight = computedHeight;
}


void Node::positionOutOfFlowChild(Node *ancestor, float refWidth, float refHeight) {
    // 1. Calculate containing block's padding edge coordinates
    float cbX = 0.0f, cbY = 0.0f;
    if (ancestor) {
        cbX = ancestor->_layout.computedX
              + ancestor->_style.border().widthLeft
              + ancestor->_style.padding().left;

        cbY = ancestor->_layout.computedY
              + ancestor->_style.border().widthTop
              + ancestor->_style.padding().top;;
    }

    auto &dimensions = _style.dimensions();
    const bool hasLeft = dimensions.left.unit != CSSUnit::AUTO;
    const bool hasRight = dimensions.right.unit != CSSUnit::AUTO;
    const bool hasTop = dimensions.top.unit != CSSUnit::AUTO;
    const bool hasBottom = dimensions.bottom.unit != CSSUnit::AUTO;


    if (hasLeft || hasRight) {
        if (hasLeft) {
            _layout.computedX = cbX + dimensions.left.resolveValue(refWidth);
        } else {
            _layout.computedX = cbX + refWidth
                                - dimensions.right.resolveValue(refWidth)
                                - _layout.computedWidth;
        }
    }

    // 4. Vertical positioning
    if (hasTop || hasBottom) {
        if (hasTop) {
            _layout.computedY = cbY + dimensions.top.resolveValue(refHeight);
        } else {
            _layout.computedY = cbY + refHeight
                                - dimensions.bottom.resolveValue(refHeight)
                                - _layout.computedHeight;
        }
    }

    // 5. Handle default positioning (static position)
    if (!hasLeft && !hasRight) {
        _layout.computedX = cbX; // + getStaticPositionX(child);
    }

    if (!hasTop && !hasBottom) {
        _layout.computedY = cbY; //+ getStaticPositionY(child);
    }
}

void Node::handleStickyPosition(float refWidth, float refHeight) {
    // Assuming we have access to scroll values
    float scrollY = 0; //getScrollY();

    auto &dimensions = _style.dimensions();

    float stickyTop = dimensions.top.resolveValue(refHeight);
    float stickyLeft = dimensions.left.resolveValue(refHeight);

    // Calculate new position within parent bounds
    float parentTop = _layout.computedY; // Parent's top in layout
    float parentBottom = _layout.computedY + _layout.computedHeight;

    float newStickyY = std::max(parentTop + stickyTop, scrollY + stickyTop);
    newStickyY = std::min(newStickyY, parentBottom - _layout.computedHeight);

    // Apply new computed Y position
    _layout.computedY = newStickyY;
}
