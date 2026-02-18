//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "Node.h"

#include <algorithm>
#include <vector>

using namespace masharif;

void Node::removeChild(SharedNode &child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
        _style.dirty = true;
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
    auto computedX = _layout.computedX;
    auto computedY = _layout.computedY;
    for (auto &child: children) {
        auto &position = child->style().dimensions().position;
        if (position != PositionType::Static &&
            position != PositionType::Relative) {
            child->startUpdatingPositions();
            continue; // Skip non-static/relative positioned elements
        }
        DEF_NODE_LAYOUT(child);
        childLayout.computedX += computedX;
        childLayout.computedY += computedY;

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
    startUpdatingPositions();
}

void Node::layoutImpl(float availableWidth, float availableHeight) {
    if (_style.dimensions().display == OuterDisplay::None) {
        _layout.computedWidth = _layout.computedHeight = 0.0f;
        return;
    }
    if (_style.dirty)
        computeDimensions(availableWidth, availableHeight);
    bool isAnyChildDirty = _style.dirty;
    if (!isAnyChildDirty) {
        for (const auto &child: children) {
            isAnyChildDirty = isAnyChildDirty | child->_style.dirty;
        }
    }

    if (isAnyChildDirty) {
        layoutStrategy->layout(availableWidth, availableHeight);
        if (auto &position = _style.dimensions().position; position == PositionType::Relative) {
            auto &offset = _style.offsets();
            _layout.computedX += offset.left.resolveValue(availableWidth) - offset.right.resolveValue(availableWidth);
            _layout.computedY += offset.top.resolveValue(availableHeight) - offset.bottom.resolveValue(availableHeight);
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
                auto position = childStyle.dimensions().position;
                auto &childMargin = childStyle.margin();
                if (position == PositionType::Static || position == PositionType::Relative) {
                    maxChildBottom = std::max(maxChildBottom,
                                              childLayout.computedY + childLayout.computedHeight + childMargin.bottom.
                                              value
                                              +
                                              childMargin.top.value);
                }
            }

            auto &border = containerStyle.border();
            _layout.computedHeight = maxChildBottom + _style.padding().top.value + _style.padding().bottom.value
                                     +
                                     border.widthTop.value + border.widthBottom.value;
        }
    }

    _style.dirty = false;
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
        computedWidth = availableWidth * (width.value / 100.0f);
    } else {
        if (display == OuterDisplay::Block || display == OuterDisplay::Flex) {
            auto &margin = _style.margin();
            auto &padding = _style.padding();
            auto &border = _style.border();
            float totalHorizontal = margin.left.resolveValue(availableWidth) +
                                    margin.right.resolveValue(availableWidth) +
                                    padding.left.value + padding.right.value +
                                    border.widthLeft.value + border.widthRight.value;
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
                                                        childStyle.margin().left.resolveValue(0) + childStyle.margin(). right.resolveValue(0));
            }
        }
    }
    auto &stylePadding = _style.padding();
    auto &styleBorder = _style.border();
    float horizontalPadding = stylePadding.left.value + stylePadding.right.value +
                              styleBorder.widthLeft.value + styleBorder.widthRight.value;
    float verticalPadding = stylePadding.top.value + stylePadding.bottom.value +
                            styleBorder.widthTop.value + styleBorder.widthBottom.value;

    if (!std::isnan(computedWidth)) {
        if (minWidth.unit != CSSUnit::AUTO)
            computedWidth = std::max(computedWidth, minWidth.resolveValue(availableWidth));
        if (maxWidth.unit != CSSUnit::AUTO)
            computedWidth = std::min(computedWidth, maxWidth.resolveValue(availableWidth));
        computedWidth += horizontalPadding;
    }


    if (height.unit == CSSUnit::PX) {
        computedHeight = height.value;
    } else if (height.unit == CSSUnit::PERCENT) {
        if (_parent) {
            auto &parentHeight = _parent->style().dimensions().height;
            if (height.unit != CSSUnit::AUTO) {
                computedHeight = availableHeight * (parentHeight.value / 100.0f);
            }
        }
    }
    if (!std::isnan(computedHeight)) {
        if (minHeight.unit != CSSUnit::AUTO)
            computedHeight = std::max(computedHeight, minHeight.resolveValue(availableHeight));
        if (maxHeight.unit != CSSUnit::AUTO)
            computedHeight = std::min(computedHeight, maxHeight.resolveValue(availableHeight));
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
              + ancestor->_style.border().widthLeft.value
              + ancestor->_style.padding().left.value;

        cbY = ancestor->_layout.computedY
              + ancestor->_style.border().widthTop.value
              + ancestor->_style.padding().top.value;;
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
