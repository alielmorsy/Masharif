#include "Node.h"

#include <algorithm>
#include <vector>

using namespace masharif;

namespace
{
    /// NaN compares equal to NaN here so an unchanged AUTO placeholder (NaN) is a cache hit.
    bool SameSize(float a, float b)
    {
        if (std::isnan(a) && std::isnan(b)) return true;
        return a == b;
    }
}

void Node::removeChild(SharedNode& child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
        _style.dirty = true;
    }
}

void Node::markDirtyToRoot()
{
    _style.dirty = true;
    for (Node* p = _parent; p && !p->_descendantDirty; p = p->_parent)
    {
        p->_descendantDirty = true;
    }
}


static Node* findRelativeParent(Node* child)
{
    auto current = child;
    while (true)
    {
        auto position = child->style().dimensions().position;
        if (position == PositionType::Relative)
            return current;
        if (!current->parent()) return current;
        current = current->parent();
    }
}

void Node::startUpdatingPositions()
{
    // Clear dirty at end of frame (not mid-solve, which would hide a change from the
    // later definite-size pass).
    _style.dirty = false;
    _descendantDirty = false;

    const float absX = _layout.computedX;
    const float absY = _layout.computedY;
    for (auto& child : children)
    {
        auto& position = child->style().dimensions().position;
        if (position != PositionType::Static &&
            position != PositionType::Relative)
        {
            child->startUpdatingPositions();
            continue;
        }
        DEF_NODE_LAYOUT(child);
        // Derive absolute from stable local (idempotent: a skipped clean subtree still
        // lands correctly when an ancestor moves).
        childLayout.computedX = absX + childLayout.localX;
        childLayout.computedY = absY + childLayout.localY;

        child->startUpdatingPositions();
        child->positionOutOfFlowChildren();
    }
}

void Node::positionOutOfFlowChildren()
{
    for (const auto& child : outOfFlowChildren)
    {
        Node* ancestor = nullptr;
        float refWidth, refHeight;
        auto position = child->style().dimensions().position;
        if (position == PositionType::Fixed)
        {
            // TODO: resolve against the viewport as containing block.
            refWidth = 0;
            refHeight = 0;
        }
        else
        {
            ancestor = findRelativeParent(child.get());
            refWidth = ancestor->_layout.computedWidth;
            refHeight = ancestor->_layout.computedHeight;
        }

        child->layoutImpl(refWidth, refHeight);

        if (position == PositionType::Sticky)
        {
            child->handleStickyPosition(refWidth, refWidth);
        }
        else
        {
            child->positionOutOfFlowChild(ancestor, refWidth, refHeight);
        }
    }
    outOfFlowChildren.clear();
}

void Node::calculate(float availableWidth, float availableHeight)
{
    layoutImpl(availableWidth, availableHeight);
    // Root's local origin is its absolute origin; descendants derive theirs from it.
    _layout.computedX = _layout.localX;
    _layout.computedY = _layout.localY;
    startUpdatingPositions();
}

void Node::layoutImpl(float availableWidth, float availableHeight)
{
    if (_style.dimensions().display == OuterDisplay::None)
    {
        _layout.computedWidth = _layout.computedHeight = 0.0f;
        return;
    }

    const bool spaceSame = SameSize(availableWidth, _lastAvailW) &&
        SameSize(availableHeight, _lastAvailH);

    // A directly-dirtied child forces a re-solve even without markDirtyToRoot, preserving the
    // standalone contract (set a child dirty, call calculate); markDirtyToRoot covers the deep case.
    bool anyDirectChildDirty = false;
    for (const auto& child : children)
    {
        if (child->_style.dirty)
        {
            anyDirectChildDirty = true;
            break;
        }
    }

    // Full reuse: nothing changed and the space matches the last solve, so the cached _layout
    // is still valid (dirty is cleared at end of frame, not here).
    if (spaceSame && !_style.dirty && !_descendantDirty && !anyDirectChildDirty)
    {
        // Report the content-box size, not a transient grow/shrink value an ancestor's resolve may
        // have left in computedWidth/Height (a re-solving parent reads it for AUTO flex-basis).
        _layout.computedWidth = _implW;
        _layout.computedHeight = _implH;
        return;
    }

    _lastAvailW = availableWidth;
    _lastAvailH = availableHeight;

    if (_style.dirty || !spaceSame)
        computeDimensions(availableWidth, availableHeight);

    layoutStrategy->layout(availableWidth, availableHeight);

    // A flex node with an AUTO main axis was just shrink-wrapped to content; flag it so a later
    // definite-size pass re-distributes instead of reusing the collapse (the _lastDef gate only
    // sees size, not the collapse).
    if (!_mainSizeDefinite)
    {
        const auto& dims = _style.dimensions();
        const bool isFlex = dims.display != OuterDisplay::Block && dims.display != OuterDisplay::InlineBlock;
        const bool mainAuto = _style.flex().isRow()
                                  ? dims.width.unit == CSSUnit::AUTO
                                  : dims.height.unit == CSSUnit::AUTO;
        if (isFlex && mainAuto) _collapsedSinceDefinite = true;
    }

    if (auto& position = _style.dimensions().position; position == PositionType::Relative)
    {
        auto& offset = _style.offsets();
        _layout.localX += offset.left.resolveValue(availableWidth) - offset.right.resolveValue(availableWidth);
        _layout.localY += offset.top.resolveValue(availableHeight) - offset.bottom.resolveValue(availableHeight);
    }
    auto& containerStyle = _style;
    // Only Block/InlineBlock get an AUTO-height override; flex handles its own height.
    auto display = containerStyle.dimensions().display;
    bool isBlock = display == OuterDisplay::Block || display == OuterDisplay::InlineBlock;

    if (isBlock && containerStyle.dimensions().height.unit == CSSUnit::AUTO)
    {
        float maxChildBottom = 0.0f;
        for (const auto& child : children)
        {
            DEF_NODE_LAYOUT(child);
            DEF_NODE_STYLE(child);
            const auto position = childStyle.dimensions().position;
            auto& childMargin = childStyle.margin();
            if (position == PositionType::Static || position == PositionType::Relative)
            {
                maxChildBottom = std::max(maxChildBottom,
                                          childLayout.localY + childLayout.computedHeight + childMargin.bottom +
                                          childMargin.top);
            }
        }

        auto& border = containerStyle.border();
        _layout.computedHeight = maxChildBottom + _style.padding().top + _style.padding().bottom
            +
            border.widthTop + border.widthBottom;
    }

    // Remember this run's content-box size for the reuse early-out (the parent may mutate
    // computedWidth/Height via flex grow/shrink before then).
    _implW = _layout.computedWidth;
    _implH = _layout.computedHeight;
}

void Node::markSubtreeDirtyForRelayout()
{
    _style.dirty = true;
    for (auto& child : children)
    {
        child->markSubtreeDirtyForRelayout();
    }
}

void Node::layoutContentsWithDefiniteSize(float borderBoxWidth, float borderBoxHeight)
{
    if (_style.dimensions().display == OuterDisplay::None) return;
    if (children.empty()) return; // leaf: nothing to re-lay-out
    if (std::isnan(borderBoxWidth) || std::isnan(borderBoxHeight)) return;

    // Adopt the border-box size decided by the flex parent (main) and updateCrossSize (cross).
    _layout.computedWidth = borderBoxWidth;
    _layout.computedHeight = borderBoxHeight;

    // Reuse the cached subtree when nothing changed and the definite size matches the last pass.
    // On a size change the layoutImpl memo below misses and re-expands, so no force-dirty needed.
    bool anyDirectChildDirty = false;
    for (const auto& child : children)
    {
        if (child->_style.dirty)
        {
            anyDirectChildDirty = true;
            break;
        }
    }
    if (!_collapsedSinceDefinite &&
        !_style.dirty && !_descendantDirty && !anyDirectChildDirty &&
        SameSize(borderBoxWidth, _lastDefW) && SameSize(borderBoxHeight, _lastDefH))
        return;
    // This definite re-distribution supersedes any shrink-wrap an intervening layoutImpl left.
    _collapsedSinceDefinite = false;
    _lastDefW = borderBoxWidth;
    _lastDefH = borderBoxHeight;

    // Border-box -> content-box for the strategy (computeDimensions re-adds padding+border,
    // so subtract them here exactly once).
    auto& padding = _style.padding();
    auto& border = _style.border();
    float horizontal = padding.left + padding.right + border.widthLeft + border.widthRight;
    float vertical = padding.top + padding.bottom + border.widthTop + border.widthBottom;
    float contentWidth = std::max(0.0f, borderBoxWidth - horizontal);
    float contentHeight = std::max(0.0f, borderBoxHeight - vertical);

    // Drive the strategy directly (layoutImpl would re-apply the Block AUTO-height override and
    // discard the adopted size). mainSizeIsDefinite tells flex to fill, not shrink-wrap.
    _mainSizeDefinite = true;
    layoutStrategy->layout(contentWidth, contentHeight);
    _mainSizeDefinite = false;

    // Re-assert the definite border box (the strategy may rewrite it; guard rounding drift).
    _layout.computedWidth = borderBoxWidth;
    _layout.computedHeight = borderBoxHeight;
}

void Node::computeDimensions(float availableWidth, float availableHeight)
{
    auto& dimensions = _style.dimensions();
    auto& width = dimensions.width;
    auto& height = dimensions.height;
    auto& minWidth = dimensions.minWidth;
    auto& maxWidth = dimensions.maxWidth;

    auto& minHeight = dimensions.minHeight;
    auto& maxHeight = dimensions.maxHeight;
    auto display = dimensions.display;
    float computedWidth = NAN, computedHeight = NAN;
    if (width.unit == CSSUnit::PX)
    {
        computedWidth = width.value;
    }
    else if (width.unit == CSSUnit::PERCENT)
    {
        computedWidth = availableWidth * (width / 100.0f);
    }
    else
    {
        if (display == OuterDisplay::Block || display == OuterDisplay::Flex)
        {
            auto& margin = _style.margin();
            auto& padding = _style.padding();
            auto& border = _style.border();
            float totalHorizontal = margin.left.resolveValue(availableWidth) +
                margin.right.resolveValue(availableWidth) +
                padding.left + padding.right +
                border.widthLeft + border.widthRight;
            if (std::isnan(availableWidth))
            {
                computedWidth = 0.0f;
            }
            else
            {
                computedWidth = std::max(0.0f, availableWidth - totalHorizontal);
            }
        }
        else if (display == OuterDisplay::Inline || display == OuterDisplay::InlineBlock)
        {
            computedWidth = 100.0f; // Placeholder
            for (const auto& child : children)
            {
                child->layoutImpl(availableWidth, availableHeight);
                DEF_NODE_LAYOUT(child);
                DEF_NODE_STYLE(child);
                computedWidth = std::max(computedWidth, childLayout.computedWidth +
                                         childStyle.margin().left.resolveValue(0) + childStyle.margin().
                                         right.resolveValue(0));
            }
        }
    }
    auto& stylePadding = _style.padding();
    auto& styleBorder = _style.border();
    float horizontalPadding = stylePadding.left + stylePadding.right +
        styleBorder.widthLeft + styleBorder.widthRight;
    float verticalPadding = stylePadding.top + stylePadding.bottom +
        styleBorder.widthTop + styleBorder.widthBottom;

    // Explicit PX/PERCENT sizes are border-box (padding+border inset the content); the AUTO
    // branches produced a content size, so only those re-add padding+border below.
    const bool widthIsExplicit = (width.unit == CSSUnit::PX || width.unit == CSSUnit::PERCENT);
    const bool heightIsExplicit = (height.unit == CSSUnit::PX || height.unit == CSSUnit::PERCENT);

    if (!std::isnan(computedWidth))
    {
        if (minWidth.unit != CSSUnit::AUTO)
            computedWidth = std::max(computedWidth, minWidth.resolveValue(availableWidth));
        if (maxWidth.unit != CSSUnit::AUTO)
            computedWidth = std::min(computedWidth, maxWidth.resolveValue(availableWidth));
        if (!widthIsExplicit)
            computedWidth += horizontalPadding;
    }


    if (height.unit == CSSUnit::PX)
    {
        computedHeight = height.value;
    }
    else if (height.unit == CSSUnit::PERCENT)
    {
        if (!std::isnan(availableHeight))
        {
            computedHeight = availableHeight * (height.value / 100.0f);
        }
    }
    if (!std::isnan(computedHeight))
    {
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


void Node::positionOutOfFlowChild(Node* ancestor, float refWidth, float refHeight)
{
    // Containing block's padding-edge origin.
    float cbX = 0.0f, cbY = 0.0f;
    if (ancestor)
    {
        cbX = ancestor->_layout.computedX
            + ancestor->_style.border().widthLeft
            + ancestor->_style.padding().left;

        cbY = ancestor->_layout.computedY
            + ancestor->_style.border().widthTop
            + ancestor->_style.padding().top;
    }

    auto& dimensions = _style.dimensions();
    const bool hasLeft = dimensions.left.unit != CSSUnit::AUTO;
    const bool hasRight = dimensions.right.unit != CSSUnit::AUTO;
    const bool hasTop = dimensions.top.unit != CSSUnit::AUTO;
    const bool hasBottom = dimensions.bottom.unit != CSSUnit::AUTO;


    if (hasLeft || hasRight)
    {
        if (hasLeft)
        {
            _layout.computedX = cbX + dimensions.left.resolveValue(refWidth);
        }
        else
        {
            _layout.computedX = cbX + refWidth
                - dimensions.right.resolveValue(refWidth)
                - _layout.computedWidth;
        }
    }

    if (hasTop || hasBottom)
    {
        if (hasTop)
        {
            _layout.computedY = cbY + dimensions.top.resolveValue(refHeight);
        }
        else
        {
            _layout.computedY = cbY + refHeight
                - dimensions.bottom.resolveValue(refHeight)
                - _layout.computedHeight;
        }
    }

    // Fall back to the static position when neither offset is set.
    if (!hasLeft && !hasRight)
    {
        _layout.computedX = cbX;
    }

    if (!hasTop && !hasBottom)
    {
        _layout.computedY = cbY;
    }
}

void Node::handleStickyPosition(float refWidth, float refHeight)
{
    // TODO: wire a real scroll offset; sticky is only clamped within the parent bounds for now.
    float scrollY = 0;

    auto& dimensions = _style.dimensions();
    float stickyTop = dimensions.top.resolveValue(refHeight);

    float parentTop = _layout.computedY;
    float parentBottom = _layout.computedY + _layout.computedHeight;

    float newStickyY = std::max(parentTop + stickyTop, scrollY + stickyTop);
    newStickyY = std::min(newStickyY, parentBottom - _layout.computedHeight);
    _layout.computedY = newStickyY;
}
