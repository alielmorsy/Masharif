#include "NormalFlowStrategy.h"

#include "LayoutContext.h"
#include "Node.h"

#include <algorithm>
#include <cmath>

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
    // Percentage paddings resolve against the container's own containing block, never against
    // anything derived here -- Node::PaddingLeft and friends hold that reference.
    const float containerPadLeft = container.PaddingLeft();
    const float containerPadTop = container.PaddingTop();

    // Shrink the incoming available space to this container's own content box, mirroring
    // FlexLayoutStrategy::ShrinkAvailableToContentBox. Without this, block/flex children resolve their
    // AUTO width against the raw viewport instead of the container, overflowing a narrow container.
    float availW = availableWidth;
    float availH = availableHeight;
    {
        const auto &dim = containerStyle.GetDimensions();
        const bool widthExplicit = dim.Width.Unit == CSSUnit::Px || dim.Width.Unit == CSSUnit::Percent;
        const bool heightExplicit = dim.Height.Unit == CSSUnit::Px || dim.Height.Unit == CSSUnit::Percent;
        if (widthExplicit && !std::isnan(container.GetLayout().ComputedWidth))
            availW = std::max(0.0f, container.GetLayout().ComputedWidth
                                    - container.PaddingBorderHorizontal());
        if (heightExplicit && !std::isnan(container.GetLayout().ComputedHeight))
            availH = std::max(0.0f, container.GetLayout().ComputedHeight
                                    - container.PaddingBorderVertical());
    }

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
        const auto display = childStyle.GetDimensions().Display;

        // availW is this container's content box, which is exactly the containing block every
        // percentage padding and margin on the child resolves against.
        child->SetPercentBasis(availW);

        // A block-level box with `width: auto` FILLS its containing block; shrink-to-fit belongs to
        // floats, inline-level boxes and out-of-flow boxes. FlexLayoutStrategy gates its AUTO-main-axis
        // shrink-wrap on MainSizeIsDefinite(), so a block-level flex child needs that flag set or it
        // collapses to its content width and takes its whole subtree with it (grow items resolving
        // against a 0-wide container). Conditions, all necessary:
        //   - Flex only. A Block child routes back here, which never reads the flag; InlineFlex is
        //     inline-level, where shrink-to-fit IS correct.
        //   - Row main axis only. The flag means "the child's MAIN axis is definite", and only the
        //     inline axis fills — a column container's AUTO height must still be content-sized.
        //   - Definite available width. There is nothing to fill otherwise.
        const bool fillsInlineAxis = display == OuterDisplay::Flex
                                     && childStyle.GetFlex().IsRow()
                                     && !std::isnan(availW);
        if (fillsInlineAxis) child->m_mainSizeDefinite = true;
        child->LayoutImpl(ctx, availW, availH);
        if (fillsInlineAxis) child->m_mainSizeDefinite = false;

        if (display == OuterDisplay::Block || display == OuterDisplay::Flex) {
            if (!line.Empty()) {
                LayoutLine(line, currentY);
                currentY += lineHeight;
                line.Clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }

            // ComputeDimensions already shrank an AUTO-width child by both its margins; without
            // adding margin-left here that reserved space silently shifts to the right edge
            // instead of padding the left one.
            childLayout.LocalX = containerPadLeft + containerBorder.WidthLeft.Value
                                  + childMargin.Left.Value;
            childLayout.LocalY = currentY + containerPadTop + containerBorder.WidthTop.Value;
            currentY += childLayout.ComputedHeight + childMargin.Top.Value + childMargin.Bottom.Value;
        } else if (display == OuterDisplay::InlineBlock || display == OuterDisplay::InlineFlex) {
            const float childWidth = childLayout.ComputedWidth + childMargin.Left.Value + childMargin.Right.Value;

            if (currentX + childWidth > availW && !line.Empty()) {
                LayoutLine(line, currentY);
                currentY += lineHeight;
                line.Clear();
                currentX = 0.0f;
                lineHeight = 0.0f;
            }
            line.Append(child.get());
            currentX += childWidth;
            lineHeight = std::max(lineHeight,
                                  childLayout.ComputedHeight + childMargin.Top.Value + childMargin.Bottom.Value +
                                  child->PaddingBorderVertical());
        }
    }

    if (!line.Empty())
        LayoutLine(line, currentY);
}
