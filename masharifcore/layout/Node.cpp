#include "Node.h"

#include "LayoutContext.h"
#include "LayoutStrategy.h"

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

    /// Containing block for an absolutely positioned node: the nearest *positioned*
    /// (non-Static) ancestor — CSS semantics — falling back to the root.
    Node* FindContainingBlock(Node* child)
    {
        Node* current = child->Parent();
        if (!current) return child;
        while (current->Parent() &&
               current->GetStyle().GetDimensions().Position == PositionType::Static)
        {
            current = current->Parent();
        }
        return current;
    }
}

void Node::RemoveChild(SharedNode& child)
{
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end())
    {
        m_Children.erase(it);
        MarkDirtyToRoot();
    }
}

void Node::MarkDirtyToRoot()
{
    m_Style.Dirty = true;
    for (Node* p = m_Parent; p && !p->m_DescendantDirty; p = p->m_Parent)
    {
        p->m_DescendantDirty = true;
    }
}

void Node::StartUpdatingPositions(LayoutContext& ctx)
{
    // Clear dirty at end of frame (not mid-solve, which would hide a change from the
    // later definite-size pass).
    m_Style.Dirty = false;
    m_DescendantDirty = false;
    m_PositionsDirty = false;

    const float absX = m_Layout.ComputedX;
    const float absY = m_Layout.ComputedY;
    for (auto& child : m_Children)
    {
        auto& position = child->GetStyle().GetDimensions().Position;
        if (position != PositionType::Static &&
            position != PositionType::Relative)
        {
            // Out-of-flow subtrees are solved, positioned AND walked by
            // PositionOutOfFlowChildren — touching them here would clear their dirty
            // flags before that solve runs.
            continue;
        }
        auto& childLayout = child->m_Layout;
        // Derive absolute from stable local (idempotent: a skipped clean subtree still
        // lands correctly when an ancestor moves).
        const float newX = absX + childLayout.LocalX;
        const float newY = absY + childLayout.LocalY;
        // NaN-safe: NaN != NaN forces a visit, never a skip.
        const bool originChanged = newX != childLayout.ComputedX || newY != childLayout.ComputedY;
        childLayout.ComputedX = newX;
        childLayout.ComputedY = newY;

        // Recurse only where something can have changed: the subtree moved, was re-solved
        // (m_PositionsDirty), or carries dirt to clear. MarkDirtyToRoot flags every ancestor
        // and a strategy only runs while all ancestors' strategies are on the stack, so a
        // flagged node is always reachable through flagged ancestors — skipped subtrees are
        // flag-free by construction. Idle frames touch only the clean frontier.
        if (originChanged || child->m_PositionsDirty || child->m_Style.Dirty || child->m_DescendantDirty)
        {
            child->StartUpdatingPositions(ctx);
            child->PositionOutOfFlowChildren(ctx);
        }
    }
}

void Node::PositionOutOfFlowChildren(LayoutContext& ctx)
{
    for (const auto& child : m_OutOfFlowChildren)
    {
        Node* ancestor = nullptr;
        float refWidth, refHeight;
        auto position = child->GetStyle().GetDimensions().Position;
        if (position == PositionType::Fixed)
        {
            // TODO: resolve against the viewport as containing block.
            refWidth = 0;
            refHeight = 0;
        }
        else
        {
            ancestor = FindContainingBlock(child.get());
            refWidth = ancestor->m_Layout.ComputedWidth;
            refHeight = ancestor->m_Layout.ComputedHeight;
        }

        child->LayoutImpl(ctx, refWidth, refHeight);

        if (position == PositionType::Sticky)
        {
            child->HandleStickyPosition(refWidth, refHeight);
        }
        else
        {
            child->PositionOutOfFlowChild(ancestor, refWidth, refHeight);
        }

        // The main walk skips out-of-flow subtrees entirely; derive their descendants'
        // absolute coordinates from the origin just set, and consume nested out-of-flow
        // lists. This runs in the same frame — no one-frame lag, no stale fix-ups.
        child->StartUpdatingPositions(ctx);
        child->PositionOutOfFlowChildren(ctx);
    }
    m_OutOfFlowChildren.clear();
}

void Node::Calculate(float availableWidth, float availableHeight)
{
    m_Generation = BumpTreeGeneration();
    LayoutContext ctx;
    LayoutImpl(ctx, availableWidth, availableHeight);
    // Root's local origin is its absolute origin; descendants derive theirs from it.
    m_Layout.ComputedX = m_Layout.LocalX;
    m_Layout.ComputedY = m_Layout.LocalY;
    StartUpdatingPositions(ctx);
    // The walk consumes every descendant's out-of-flow list; the root's own list has no
    // other consumer, so it is handled here.
    PositionOutOfFlowChildren(ctx);
}

void Node::LayoutImpl(float availableWidth, float availableHeight, bool ignoreMinMax)
{
    m_Generation = BumpTreeGeneration();
    LayoutContext ctx;
    LayoutImpl(ctx, availableWidth, availableHeight, ignoreMinMax);
}

const Node::MeasureCacheEntry* Node::FindMeasure(float availW, float availH, bool ignoreMinMax) const
{
    if (m_Generation == 0) return nullptr; // never solved under a frame stamp
    for (const auto& entry : m_MeasureCache)
    {
        if (entry.Generation == m_Generation &&
            entry.IgnoreMinMax == ignoreMinMax &&
            SameSize(entry.AvailW, availW) && SameSize(entry.AvailH, availH))
            return &entry;
    }
    return nullptr;
}

void Node::RecordMeasure(float availW, float availH, bool ignoreMinMax, float resultW, float resultH)
{
    m_MeasureCache[m_MeasureCacheNext] = {m_Generation, availW, availH, ignoreMinMax, resultW, resultH};
    m_MeasureCacheNext = static_cast<std::uint8_t>((m_MeasureCacheNext + 1) % MeasureCacheSize);
}

void Node::LayoutImpl(LayoutContext& ctx, float availableWidth, float availableHeight, bool ignoreMinMax)
{
    PullGeneration();

    if (m_Style.GetDimensions().Display == OuterDisplay::None)
    {
        m_Layout.ComputedWidth = m_Layout.ComputedHeight = 0.0f;
        return;
    }

    const bool spaceSame = SameSize(availableWidth, m_LastAvailW) &&
        SameSize(availableHeight, m_LastAvailH);

    // Full reuse: nothing changed and the space matches the last solve, so the cached layout
    // is still valid (dirty is cleared at end of frame, not here). Style::Modify propagates
    // m_DescendantDirty through every ancestor, so these two flags are the whole contract.
    if (spaceSame && !m_Style.Dirty && !m_DescendantDirty)
    {
        // Report the content-box size, not a transient grow/shrink value an ancestor's resolve
        // may have left in ComputedWidth/Height (a re-solving parent reads it for flex basis).
        m_Layout.ComputedWidth = m_ImplW;
        m_Layout.ComputedHeight = m_ImplH;
        // Make the result replayable for this frame: if a later full solve at different
        // inputs overwrites m_LastAvail, a repeat call at these inputs must not re-solve.
        if (!FindMeasure(availableWidth, availableHeight, ignoreMinMax))
            RecordMeasure(availableWidth, availableHeight, ignoreMinMax, m_ImplW, m_ImplH);
        return;
    }

    // Within-frame replay: dirty means "solve at least once this frame", and this frame has
    // already solved at exactly these inputs — the subtree reflects an identical-input run,
    // so only the content size needs restoring. This is what collapses the former
    // O(2^depth) double-recursion (basis + definite passes) to one solve per distinct input.
    if (const MeasureCacheEntry* hit = FindMeasure(availableWidth, availableHeight, ignoreMinMax))
    {
        m_Layout.ComputedWidth = hit->ResultW;
        m_Layout.ComputedHeight = hit->ResultH;
        return;
    }

    m_LastAvailW = availableWidth;
    m_LastAvailH = availableHeight;

    if (m_Style.Dirty || !spaceSame)
        ComputeDimensions(ctx, availableWidth, availableHeight, ignoreMinMax);

    LayoutStrategy::For(m_Style.GetDimensions().Display).Layout(*this, ctx, availableWidth, availableHeight);
    ++m_Layout.StrategyRuns;

    // Descendants now reflect this available-space run, not the last definite distribution;
    // the next definite pass must re-run even if its size memo matches. (Subsumes the old
    // shrink-wrapped-AUTO-main-axis special case.)
    m_StrategyRanSinceDefinite = true;
    m_PositionsDirty = true;

    if (auto& position = m_Style.GetDimensions().Position; position == PositionType::Relative)
    {
        auto& offset = m_Style.GetOffsets();
        m_Layout.LocalX += offset.Left.ResolveValue(availableWidth) - offset.Right.ResolveValue(availableWidth);
        m_Layout.LocalY += offset.Top.ResolveValue(availableHeight) - offset.Bottom.ResolveValue(availableHeight);
    }

    // Only Block/InlineBlock get an AUTO-height override; flex handles its own height.
    const auto display = m_Style.GetDimensions().Display;
    const bool isBlock = display == OuterDisplay::Block || display == OuterDisplay::InlineBlock;

    if (isBlock && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto)
    {
        float maxChildBottom = 0.0f;
        for (const auto& child : m_Children)
        {
            auto& childLayout = child->m_Layout;
            auto& childStyle = child->m_Style;
            const auto position = childStyle.GetDimensions().Position;
            auto& childMargin = childStyle.GetMargin();
            if (position == PositionType::Static || position == PositionType::Relative)
            {
                maxChildBottom = std::max(maxChildBottom,
                                          childLayout.LocalY + childLayout.ComputedHeight + childMargin.Bottom +
                                          childMargin.Top);
            }
        }

        auto& border = m_Style.GetBorder();
        m_Layout.ComputedHeight = maxChildBottom + m_Style.GetPadding().Top + m_Style.GetPadding().Bottom
            +
            border.WidthTop + border.WidthBottom;
    }

    // Remember this run's content-box size for the reuse early-out (the parent may mutate
    // ComputedWidth/Height via flex grow/shrink before then), and make it replayable for
    // repeat same-input calls within this frame.
    m_ImplW = m_Layout.ComputedWidth;
    m_ImplH = m_Layout.ComputedHeight;
    RecordMeasure(availableWidth, availableHeight, ignoreMinMax, m_ImplW, m_ImplH);
}

void Node::LayoutContentsWithDefiniteSize(LayoutContext& ctx, float borderBoxWidth, float borderBoxHeight)
{
    PullGeneration();

    if (m_Style.GetDimensions().Display == OuterDisplay::None) return;
    if (m_Children.empty()) return; // leaf: nothing to re-lay-out
    if (std::isnan(borderBoxWidth) || std::isnan(borderBoxHeight)) return;

    // Adopt the border-box size decided by the flex parent (main) and the cross-axis stretch.
    m_Layout.ComputedWidth = borderBoxWidth;
    m_Layout.ComputedHeight = borderBoxHeight;

    // The memo is only meaningful while the descendants still reflect the last definite
    // distribution; any impl-path strategy run since then repositioned them.
    const bool stillDefinite = SameSize(borderBoxWidth, m_LastDefW) &&
        SameSize(borderBoxHeight, m_LastDefH) &&
        !m_StrategyRanSinceDefinite;

    // Reuse across frames when the subtree is clean, or within the frame when this exact
    // distribution already ran this generation (the flex parent's second pass).
    if (stillDefinite &&
        ((!m_Style.Dirty && !m_DescendantDirty) || (m_Generation != 0 && m_DefGeneration == m_Generation)))
        return;

    m_StrategyRanSinceDefinite = false;
    m_LastDefW = borderBoxWidth;
    m_LastDefH = borderBoxHeight;
    m_DefGeneration = m_Generation;

    // Border-box -> content-box for the strategy (ComputeDimensions re-adds padding+border,
    // so subtract them here exactly once).
    auto& padding = m_Style.GetPadding();
    auto& border = m_Style.GetBorder();
    const float horizontal = padding.Left + padding.Right + border.WidthLeft + border.WidthRight;
    const float vertical = padding.Top + padding.Bottom + border.WidthTop + border.WidthBottom;
    const float contentWidth = std::max(0.0f, borderBoxWidth - horizontal);
    const float contentHeight = std::max(0.0f, borderBoxHeight - vertical);

    // Drive the strategy directly (LayoutImpl would re-apply the Block AUTO-height override
    // and discard the adopted size). MainSizeIsDefinite tells flex to fill, not shrink-wrap.
    m_MainSizeDefinite = true;
    LayoutStrategy::For(m_Style.GetDimensions().Display).Layout(*this, ctx, contentWidth, contentHeight);
    m_MainSizeDefinite = false;
    ++m_Layout.StrategyRuns;
    m_PositionsDirty = true;

    // Re-assert the definite border box (the strategy may rewrite it; guard rounding drift).
    m_Layout.ComputedWidth = borderBoxWidth;
    m_Layout.ComputedHeight = borderBoxHeight;
}

void Node::ComputeDimensions(LayoutContext& ctx, float availableWidth, float availableHeight, bool ignoreMinMax)
{
    auto& dimensions = m_Style.GetDimensions();
    auto& width = dimensions.Width;
    auto& height = dimensions.Height;
    auto& minWidth = dimensions.MinWidth;
    auto& maxWidth = dimensions.MaxWidth;

    auto& minHeight = dimensions.MinHeight;
    auto& maxHeight = dimensions.MaxHeight;
    const auto display = dimensions.Display;
    float computedWidth = NAN, computedHeight = NAN;
    if (width.Unit == CSSUnit::Px)
    {
        computedWidth = width.Value;
    }
    else if (width.Unit == CSSUnit::Percent)
    {
        computedWidth = availableWidth * (width / 100.0f);
    }
    else
    {
        if (display == OuterDisplay::Block || display == OuterDisplay::Flex)
        {
            auto& margin = m_Style.GetMargin();
            auto& padding = m_Style.GetPadding();
            auto& border = m_Style.GetBorder();
            const float totalHorizontal = margin.Left.ResolveValue(availableWidth) +
                margin.Right.ResolveValue(availableWidth) +
                padding.Left + padding.Right +
                border.WidthLeft + border.WidthRight;
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
            computedWidth = 100.0f; // Placeholder: known limitation, no shrink-to-fit floor yet
            for (const auto& child : m_Children)
            {
                child->LayoutImpl(ctx, availableWidth, availableHeight);
                auto& childLayout = child->m_Layout;
                auto& childStyle = child->m_Style;
                computedWidth = std::max(computedWidth, childLayout.ComputedWidth +
                                         childStyle.GetMargin().Left.ResolveValue(0) + childStyle.GetMargin().
                                         Right.ResolveValue(0));
            }
        }
    }
    auto& stylePadding = m_Style.GetPadding();
    auto& styleBorder = m_Style.GetBorder();
    const float horizontalPadding = stylePadding.Left + stylePadding.Right +
        styleBorder.WidthLeft + styleBorder.WidthRight;
    const float verticalPadding = stylePadding.Top + stylePadding.Bottom +
        styleBorder.WidthTop + styleBorder.WidthBottom;

    // Explicit Px/Percent sizes are border-box (padding+border inset the content); the AUTO
    // branches produced a content size, so only those re-add padding+border below.
    const bool widthIsExplicit = (width.Unit == CSSUnit::Px || width.Unit == CSSUnit::Percent);
    const bool heightIsExplicit = (height.Unit == CSSUnit::Px || height.Unit == CSSUnit::Percent);

    if (!std::isnan(computedWidth))
    {
        if (!ignoreMinMax)
        {
            if (minWidth.Unit != CSSUnit::Auto)
                computedWidth = std::max(computedWidth, minWidth.ResolveValue(availableWidth));
            if (maxWidth.Unit != CSSUnit::Auto)
                computedWidth = std::min(computedWidth, maxWidth.ResolveValue(availableWidth));
        }
        if (!widthIsExplicit)
            computedWidth += horizontalPadding;
    }


    if (height.Unit == CSSUnit::Px)
    {
        computedHeight = height.Value;
    }
    else if (height.Unit == CSSUnit::Percent)
    {
        if (!std::isnan(availableHeight))
        {
            computedHeight = availableHeight * (height.Value / 100.0f);
        }
    }
    if (!std::isnan(computedHeight))
    {
        if (!ignoreMinMax)
        {
            if (minHeight.Unit != CSSUnit::Auto)
                computedHeight = std::max(computedHeight, minHeight.ResolveValue(availableHeight));
            if (maxHeight.Unit != CSSUnit::Auto)
                computedHeight = std::min(computedHeight, maxHeight.ResolveValue(availableHeight));
        }
        if (!heightIsExplicit)
            computedHeight += verticalPadding;
    }

    m_Layout.ComputedWidth = computedWidth;
    m_Layout.ComputedHeight = computedHeight;
}


void Node::PositionOutOfFlowChild(Node* ancestor, float refWidth, float refHeight)
{
    // Containing block's padding-edge origin.
    float cbX = 0.0f, cbY = 0.0f;
    if (ancestor)
    {
        cbX = ancestor->m_Layout.ComputedX
            + ancestor->m_Style.GetBorder().WidthLeft
            + ancestor->m_Style.GetPadding().Left;

        cbY = ancestor->m_Layout.ComputedY
            + ancestor->m_Style.GetBorder().WidthTop
            + ancestor->m_Style.GetPadding().Top;
    }

    auto& dimensions = m_Style.GetDimensions();
    const bool hasLeft = dimensions.Left.Unit != CSSUnit::Auto;
    const bool hasRight = dimensions.Right.Unit != CSSUnit::Auto;
    const bool hasTop = dimensions.Top.Unit != CSSUnit::Auto;
    const bool hasBottom = dimensions.Bottom.Unit != CSSUnit::Auto;


    if (hasLeft || hasRight)
    {
        if (hasLeft)
        {
            m_Layout.ComputedX = cbX + dimensions.Left.ResolveValue(refWidth);
        }
        else
        {
            m_Layout.ComputedX = cbX + refWidth
                - dimensions.Right.ResolveValue(refWidth)
                - m_Layout.ComputedWidth;
        }
    }

    if (hasTop || hasBottom)
    {
        if (hasTop)
        {
            m_Layout.ComputedY = cbY + dimensions.Top.ResolveValue(refHeight);
        }
        else
        {
            m_Layout.ComputedY = cbY + refHeight
                - dimensions.Bottom.ResolveValue(refHeight)
                - m_Layout.ComputedHeight;
        }
    }

    // Fall back to the static position when neither offset is set.
    if (!hasLeft && !hasRight)
    {
        m_Layout.ComputedX = cbX;
    }

    if (!hasTop && !hasBottom)
    {
        m_Layout.ComputedY = cbY;
    }
}

void Node::HandleStickyPosition(float /*refWidth*/, float refHeight)
{
    // TODO: wire a real scroll offset; sticky is only clamped within the parent bounds for now.
    const float scrollY = 0;

    auto& dimensions = m_Style.GetDimensions();
    const float stickyTop = dimensions.Top.ResolveValue(refHeight);

    const float parentTop = m_Layout.ComputedY;
    const float parentBottom = m_Layout.ComputedY + m_Layout.ComputedHeight;

    float newStickyY = std::max(parentTop + stickyTop, scrollY + stickyTop);
    newStickyY = std::min(newStickyY, parentBottom - m_Layout.ComputedHeight);
    m_Layout.ComputedY = newStickyY;
}
