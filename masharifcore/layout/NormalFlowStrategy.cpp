#include "NormalFlowStrategy.h"

#include "LayoutContext.h"
#include "Node.h"

#include <algorithm>

using namespace masharif;

namespace {
    void LayoutLine(ArenaSlice<Node *> &line, const float y) {
        float x = 0.0f;
        const std::size_t count = line.Count();
        for (std::size_t i = 0; i < count; ++i) {
            Node *child = line[i];
            auto &childLayout = child->GetLayout();
            const auto &childStyle = child->GetStyle();
            childLayout.LocalX = x;
            childLayout.LocalY = y;
            x += childLayout.ComputedWidth + childStyle.GetMargin().Left.Value + childStyle.GetMargin().Right.Value;
        }
    }
}

void NormalFlowStrategy::Layout(Node &container, LayoutContext &ctx,
                                const float availableWidth, const float availableHeight) const {
    // The out-of-flow list must reflect exactly this run; see FlexLayoutStrategy::Layout.
    container.m_OutOfFlowChildren.clear();

    float currentX = 0.0f;
    float currentY = 0.0f;
    float lineHeight = 0.0f;
    ArenaSlice<Node *> line(ctx.InFlowItems);

    const auto &containerStyle = container.GetStyle();
    const auto &containerBorder = containerStyle.GetBorder();
    const auto &containerPadding = containerStyle.GetPadding();

    for (const auto &child: container.m_Children) {
        const auto &childStyle = child->GetStyle();

        // display:none generates no box — skip entirely (in-flow and out-of-flow alike), so it
        // never enters m_OutOfFlowChildren.
        if (childStyle.GetDimensions().Display == OuterDisplay::None)
            continue;

        const auto position = childStyle.GetDimensions().Position;
        if (position != PositionType::Static &&
            position != PositionType::Relative) {
            container.m_OutOfFlowChildren.push_back(child.get());
            continue;
        }
        auto &childLayout = child->GetLayout();
        const auto &childMargin = childStyle.GetMargin();
        const auto &childPadding = childStyle.GetPadding();

        child->LayoutImpl(ctx, availableWidth, availableHeight);

        const auto display = childStyle.GetDimensions().Display;
        if (display == OuterDisplay::Block || display == OuterDisplay::Flex) {
            if (!line.Empty()) {
                LayoutLine(line, currentY);
                currentY += lineHeight;
                line.Clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }

            childLayout.LocalX = containerPadding.Left.Value + containerBorder.WidthLeft.Value;
            childLayout.LocalY = currentY + containerPadding.Top.Value + containerBorder.WidthTop.Value;
            currentY += childLayout.ComputedHeight + childMargin.Top.Value + childMargin.Bottom.Value;
        } else if (display == OuterDisplay::InlineBlock || display == OuterDisplay::InlineFlex) {
            const float childWidth = childLayout.ComputedWidth + childMargin.Left.Value + childMargin.Right.Value;

            if (currentX + childWidth > availableWidth && !line.Empty()) {
                LayoutLine(line, currentY);
                currentY += lineHeight;
                line.Clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }
            const auto &childBorder = childStyle.GetBorder();
            line.Append(child.get());
            currentX += childWidth;
            lineHeight = std::max(lineHeight,
                                  childLayout.ComputedHeight + childMargin.Top.Value + childMargin.Bottom.Value +
                                  childPadding.Top.Value + childPadding.Bottom.Value +
                                  childBorder.WidthTop.Value + childBorder.WidthBottom.Value);
        }
    }

    if (!line.Empty())
        LayoutLine(line, currentY);
}
