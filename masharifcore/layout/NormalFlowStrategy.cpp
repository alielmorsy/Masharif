//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "NormalFlowStrategy.h"

using namespace NAMESPACE;

#define RESOLVE_VALUE(value, parent_dimension) (value.resolveValue(parent_dimension))

// Macro for computing total dimension including margin, padding, and border
#define COMPUTE_TOTAL_DIMENSION(layout, style, parent_height) \
(layout.computedHeight + \
RESOLVE_VALUE(style.margin().top, parent_height) + \
RESOLVE_VALUE(style.margin().bottom, parent_height) + \
RESOLVE_VALUE(style.padding().top, parent_height) + \
RESOLVE_VALUE(style.padding().bottom, parent_height) + \
RESOLVE_VALUE(style.border().widthTop, parent_height) + \
RESOLVE_VALUE(style.border().widthBottom, parent_height))

inline bool should_position_node(Node* node) {
    auto position = node->style().dimensions().position;
    return position == PositionType::Static || position == PositionType::Relative;
}
void compute_child_position(Node *child) {
    float computedX = 0.0f;
    float computedY = 0.0f;

    Node *container = child->parent();
    const auto &containerStyle = child->style();
    auto &childLayout = child->layout();
    auto &childStyle = child->style();

    auto &childPadding = childStyle.padding();
    float availableWidth = childLayout.computedWidth - childPadding.left.value - childPadding.right.value;
    float availableParentWidth = container->layout().computedWidth;
    float availableParentHeight = container->layout().computedHeight;

    if (childStyle.dimensions().display == OuterDisplay::Block) {
        SharedNode prev = child->prevSibling;
        if (prev) {
            auto &prevLayout = prev->layout();
            auto &prevStyle = prev->style();
            if (prevStyle.dimensions().display == OuterDisplay::Block) {
                auto &margin = prevStyle.margin();
                computedY = prevLayout.computedY + prevLayout.computedHeight +
                            RESOLVE_VALUE(margin.bottom, 0) +
                            RESOLVE_VALUE(margin.top, 0);
            } else {
                float currentX = 0.0f;
                float currentY = 0.0f;
                float lineHeight = 0.0f;
                std::vector<SharedNode> currentLine;

                for (auto &sibling: container->children) {
                    auto &siblingLayout = sibling->layout();
                    auto &siblingStyle = sibling->style();
                    auto siblingPosition = siblingStyle.dimensions().position;
                    if (siblingPosition != PositionType::Static && siblingPosition != PositionType::Relative) {
                        continue;
                    }

                    if (siblingStyle.dimensions().display == OuterDisplay::InlineBlock) {
                        auto &siblingMargin = siblingStyle.margin();
                        float siblingWidth = siblingLayout.computedWidth
                                             + siblingMargin.left.value
                                             + siblingMargin.right.value;

                        if (currentX + siblingWidth > availableWidth && !currentLine.empty()) {
                            currentY += lineHeight;
                            currentLine.clear();
                            currentX = 0.0f;
                            lineHeight = 0.0f;
                        }

                        currentLine.push_back(sibling);
                        currentX += siblingWidth;
                        auto &margin = siblingStyle.margin();
                        auto &padding = siblingStyle.padding();
                        auto &border = siblingStyle.border();
                        lineHeight = std::max(lineHeight, siblingLayout.computedHeight
                                                          + margin.top.resolveValue(availableParentHeight)
                                                          + margin.bottom.resolveValue(availableParentHeight)
                                                          + padding.top.resolveValue(availableParentHeight)
                                                          + padding.bottom.resolveValue(availableParentHeight)
                                                          + border.widthTop.resolveValue(availableParentHeight)
                                                          + border.widthBottom.resolveValue(availableParentHeight));
                    }
                }
                computedY = currentY + (currentLine.empty() ? 0.0f : lineHeight) + child->style().margin().top.value;
            }
        }
        computedX = containerStyle.padding().left.resolveValue(0) + containerStyle.border().widthLeft.resolveValue(0);
    } else if (childStyle.dimensions().display == OuterDisplay::InlineBlock) {
        float currentX = 0.0f;
        float currentY = 0.0f;
        float lineHeight = 0.0f;
        std::vector<SharedNode> currentLine;

        for (auto &sibling: container->children) {
            if (sibling.get() == child) break;
            auto &siblingLayout = sibling->layout();
            auto &siblingStyle = sibling->style();
            auto siblingPosition = siblingStyle.dimensions().position;
            if (siblingPosition != PositionType::Static && siblingPosition != PositionType::Relative) {
                continue;
            }
            if (siblingStyle.dimensions().display == OuterDisplay::InlineBlock) {
                auto &siblingMargin = siblingStyle.margin();
                auto &siblingPadding = siblingStyle.padding();
                auto &siblingBorder = siblingStyle.border();
                float siblingWidth = siblingLayout.computedWidth + siblingMargin.left.value + siblingMargin.right.value;
                if (currentX + siblingWidth > availableWidth && !currentLine.empty()) {
                    currentY += lineHeight;
                    currentLine.clear();
                    currentX = 0.0f;
                    lineHeight = 0.0f;
                }
                currentLine.push_back(sibling);
                currentX += siblingWidth;
                lineHeight = std::max(lineHeight, siblingLayout.computedHeight + siblingMargin.top.value +
                                                  siblingMargin.bottom.value + siblingPadding.top.value +
                                                  siblingPadding.bottom.value + siblingBorder.widthTop.value +
                                                  siblingBorder.widthBottom.value);
            } else if (siblingStyle.dimensions().display == OuterDisplay::Block) {
                if (!currentLine.empty()) {
                    currentY += lineHeight;
                    currentLine.clear();
                    currentX = 0.0f;
                    lineHeight = 0.0f;
                }
                currentY += siblingLayout.computedHeight + siblingStyle.margin().top.resolveValue(availableParentHeight)
                        + siblingStyle.margin().bottom.resolveValue(availableParentHeight);
            }
        }

        float childWidth = childLayout.computedWidth + childStyle.margin().left.resolveValue(availableParentWidth) +
                           childStyle.margin().right.resolveValue(availableParentWidth);
        if (currentX + childWidth > availableWidth && !currentLine.empty()) {
            currentY += lineHeight;
            currentX = 0.0f;
        }
        computedX = currentX;
        computedY = currentY;
    }

    childLayout.computedX = computedX;
    childLayout.computedY = computedY;
}

bool NormalFlowStrategy::preLayout(float availableWidth, float availableHeight) {
    auto &layout = container->layout();
    float oldX = layout.computedX;
    float oldY = layout.computedY;
    float oldWidth = layout.computedWidth;

    auto &width = container->style().dimensions().width;
    if (width.unit == CSSUnit::AUTO) {
        layout.computedWidth = availableWidth;
    } else {
        layout.computedWidth = width.value;
    }
    layout.computedX = 0.0f; // Simplified: assumes positioning relative to parent
    layout.computedY = 0.0f;

    // If this node has a parent, compute its position based on parent's layout strategy
    if (container->parent()) {
        compute_child_position(container);
    }

    return layout.computedX != oldX || layout.computedY != oldY || layout.computedWidth != oldWidth;
}

bool NormalFlowStrategy::postLayout() {
    auto &layout = container->layout();
    const float oldHeight = layout.computedHeight;
    float maxChildBottom = 0.0f;

    for (const auto &child: container->children) {
        const auto position = child->style().dimensions().position;
        const auto &childLayout = child->layout();
        if (position == PositionType::Static || position == PositionType::Relative) {
            auto &margin = child->style().margin();
            float childBottom = childLayout.computedY + childLayout.computedHeight + margin.bottom.value + margin.top.
                                value;
            maxChildBottom = std::max(maxChildBottom, childBottom);
        }
    }
    auto &height = container->style().dimensions().height;
    if (height.unit == CSSUnit::AUTO) {
        auto &padding = container->style().padding();
        auto &border = container->style().border();
        layout.computedHeight = maxChildBottom + padding.top.value + padding.bottom.value + border.widthBottom.value;
    } else {
        layout.computedHeight = height.value;
    }

    return layout.computedHeight != oldHeight;
}

void NormalFlowStrategy::layout(float availableWidth, float availableHeight) {
    preLayout(availableWidth, availableHeight);

    for (auto &child: container->children) {
        const auto position = child->style().dimensions().position;
        const auto &childLayout = child->layout();
        if (child->style().dimensions().display != OuterDisplay::None &&
            (position == PositionType::Static || position == PositionType::Relative)) {
            auto &padding = container->style().padding();
            const float childAvailWidth = childLayout.computedWidth - padding.left.value - padding.right.value;
            child->layoutImpl(childAvailWidth, availableHeight);
        } else if (position != PositionType::Static && position != PositionType::Relative) {
            container->outOfFlowChildren.push_back(child);
        }
    }

    postLayout();
}
