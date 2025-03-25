//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "NormalFlowStrategy.h"

#include "Node.h"
using namespace _NAMESPACE;


void NormalFlowStrategy::layout(float availableWidth, float availableHeight) {
    float currentX = 0.0f;
    float currentY = 0.0f;
    float lineHeight = 0.0f;
    std::vector<Node*> currentLine;
    DEF_NODE_LAYOUT(container);
    DEF_NODE_STYLE(container);
    auto &containerBorder = containerStyle.border();
    auto containerPadding = container->style().padding();
    for (const auto &child: container->children) {
        DEF_NODE_STYLE(child);

        auto position = childStyle.dimensions().position;
        if (position != PositionType::Static &&
            position != PositionType::Relative) {
            container->outOfFlowChildren.push_back(child);
            continue; // Skip non-static/relative positioned elements
        }
        DEF_NODE_LAYOUT(child);
        auto &childMargin = childStyle.margin();
        auto &childPadding = childStyle.padding();

        child->layoutImpl(availableWidth, availableHeight);

        auto display = childStyle.dimensions().display;
        if (display == OuterDisplay::Block) {
            if (!currentLine.empty()) {
                layoutLine(currentLine, currentY);
                currentY += lineHeight;
                currentLine.clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }

            childLayout.computedX = containerPadding.left.value + containerBorder.widthLeft.value;
            childLayout.computedY = currentY + containerPadding.top.value + containerBorder.widthTop.value;
            currentY += childLayout.computedHeight + childMargin.top.value + childMargin.bottom.value;
        }
        // Handle inline-block elements
        else if (display == OuterDisplay::InlineBlock) {
            float childWidth = childLayout.computedWidth + childMargin.left.value + childMargin.right.value;

            // Wrap to a new line if necessary
            if (currentX + childWidth > availableWidth && !currentLine.empty()) {
                layoutLine(currentLine, currentY);
                currentY += lineHeight;
                currentLine.clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }
            auto &childBorder = childStyle.border();
            currentLine.push_back(child.get());
            currentX += childWidth;
            lineHeight = std::max(lineHeight,
                                  childLayout.computedHeight + childMargin.top.value + childMargin.bottom.value +
                                  childPadding.top.value + childPadding.bottom.value +
                                  childBorder.widthTop.value + childBorder.widthBottom.value);
        }
    }

    if (!currentLine.empty()) {
        layoutLine(currentLine, currentY);
    }

    if (containerStyle.dimensions().height.unit == CSSUnit::AUTO) {
        float maxChildBottom = 0.0f;
        for (const auto &child: container->children) {
            DEF_NODE_LAYOUT(child);
            DEF_NODE_STYLE(child);
            auto position = childStyle.dimensions().position;
            auto &childMargin = childStyle.margin();
            if (position == PositionType::Static || position == PositionType::Relative) {
                maxChildBottom = std::max(maxChildBottom,
                                          childLayout.computedY + childLayout.computedHeight + childMargin.bottom.value
                                          +
                                          childMargin.top.value);
            }
        }

        auto &border = containerStyle.border();
        containerLayout.computedHeight = maxChildBottom + containerPadding.top.value + containerPadding.bottom.value +
                                         border.widthTop.value + border.widthBottom.value;
    }
}

void NormalFlowStrategy::layoutLine(std::vector<Node*> &line, float y) {
    float x = 0.0f; // Simple left alignment
    for (auto &child: line) {
        DEF_NODE_LAYOUT(child);
        DEF_NODE_STYLE(child);
        childLayout.computedX = x;
        childLayout.computedY = y;
        x += childLayout.computedWidth + childStyle.margin().left.value + childStyle.margin().right.value;
    }
}
