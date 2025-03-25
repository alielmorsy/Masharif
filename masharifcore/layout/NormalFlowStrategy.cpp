//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "NormalFlowStrategy.h"

using namespace NAMESPACE;



// Macro for computing total dimension including margin, padding, and border
#define COMPUTE_TOTAL_DIMENSION(node, parent_height) \
(node##Layout.computedHeight + \
RESOLVE_VALUE(node##Style.margin().top, parent_height) + \
RESOLVE_VALUE(node##Style.margin().bottom, parent_height) + \
RESOLVE_VALUE(node##Style.padding().top, parent_height) + \
RESOLVE_VALUE(node##Style.padding().bottom, parent_height) + \
RESOLVE_VALUE(node##Style.border().widthTop, parent_height) + \
RESOLVE_VALUE(node##Style.border().widthBottom, parent_height))



inline bool should_position_node(Node* node) {
    auto position = node->style().dimensions().position;
    return position == PositionType::Static || position == PositionType::Relative;
}

void compute_child_position(Node *child) {
    float computedX = 0.0f;
    float computedY = 0.0f;

    Node *container = child->parent();
    DEF_NODE_LAYOUT(container);
    DEF_NODE_STYLE(container);
    DEF_NODE_LAYOUT(child);
    DEF_NODE_STYLE(child);

    auto &childPadding = childStyle.padding();
    float availableWidth = childLayout.computedWidth - childPadding.left.value - childPadding.right.value;
    float availableParentWidth = containerLayout.computedWidth;
    float availableParentHeight = containerLayout.computedHeight;

    if (childStyle.dimensions().display == OuterDisplay::Block) {
        SharedNode prev = child->prevSibling;
        if (prev) {
            DEF_NODE_LAYOUT(prev);
            DEF_NODE_STYLE(prev);
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
                    DEF_NODE_LAYOUT(sibling);
                    DEF_NODE_STYLE(sibling);
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
                        lineHeight = std::max(lineHeight, COMPUTE_TOTAL_DIMENSION(sibling, availableParentHeight));
                    }
                }
                computedY = currentY + (currentLine.empty() ? 0.0f : lineHeight) + childStyle.margin().top.value;
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
            DEF_NODE_LAYOUT(sibling);
            DEF_NODE_STYLE(sibling);
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
                lineHeight = std::max(lineHeight, COMPUTE_TOTAL_DIMENSION(sibling, availableParentHeight));
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
