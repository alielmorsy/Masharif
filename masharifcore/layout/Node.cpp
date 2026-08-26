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

    /// Static-position offset along the main axis for an auto-inset out-of-flow child,
    /// mirroring how justify-content places an in-flow item (single-item semantics: the
    /// distributive values collapse to start/center). See PositionLineOnMainAxis.
    float MainAxisStatic(JustifyContent justify, float freeSpace)
    {
        switch (justify)
        {
        case JustifyContent::FlexEnd: return freeSpace;
        case JustifyContent::FlexCenter:
        case JustifyContent::SpaceAround:
        case JustifyContent::SpaceEvenly: return freeSpace * 0.5f;
        default: return 0.0f; // FlexStart, SpaceBetween, Stretch
        }
    }

    /// Cross-axis equivalent, mirroring align-items / align-self. See AlignLinesOnCrossAxis.
    float CrossAxisStatic(AlignItems align, float freeSpace)
    {
        switch (align)
        {
        case AlignItems::FlexEnd: return freeSpace;
        case AlignItems::FlexCenter: return freeSpace * 0.5f;
        default: return 0.0f; // FlexStart, Stretch, Baseline, AutoAlign
        }
    }

    /// Available space to measure an out-of-flow child on one axis. An explicit size keeps the
    /// containing-block extent (its percentage basis). An AUTO size pinned by BOTH insets fills
    /// the gap between them. An AUTO size otherwise shrink-to-fits its content (CSS abs/shrink-to-
    /// fit), signalled to the solver with NaN — handing it a definite extent instead would stretch
    /// the box to the whole containing block (the overlay-fills-the-screen bug).
    float OutOfFlowAvailable(const CSSValue& size, const CSSValue& start, const CSSValue& end, float ref)
    {
        if (size.Unit != CSSUnit::Auto) return ref;
        if (start.Unit != CSSUnit::Auto && end.Unit != CSSUnit::Auto)
            return std::max(0.0f, ref - start.ResolveValue(ref) - end.ResolveValue(ref));
        return NAN;
    }

    /// CSS 10.6.4: an out-of-flow box with `height: auto` and BOTH `top` and `bottom` set resolves
    /// its height to fill the gap between them rather than shrinking to its content.
    /// OutOfFlowAvailable already hands such a box that gap as its available height; this predicate
    /// is what stops the normal-flow AUTO-height rule (ApplyBlockAutoHeight, which sees a childless
    /// box as zero-height) from overwriting it. The width side needs no equivalent: the AUTO-width
    /// branch of ComputeDimensions already fills the available space it is given.
    bool AutoHeightFillsInsetGap(const Dimensions& d)
    {
        if (d.Position == PositionType::Static || d.Position == PositionType::Relative) return false;
        return d.Height.Unit == CSSUnit::Auto
            && d.Top.Unit != CSSUnit::Auto && d.Bottom.Unit != CSSUnit::Auto;
    }

    /// One axis of a `position: relative` shift: the start inset moves the box positively, the
    /// end inset negatively, and an AUTO inset contributes nothing (ResolveValue maps Auto to 0).
    /// `reference` is the containing block's extent ON THIS AXIS -- unlike percentage margins and
    /// paddings, which always resolve against width, a percentage `top`/`bottom` inset resolves
    /// against the containing block's height.
    float RelativeShift(const CSSValue& start, const CSSValue& end, float reference)
    {
        return start.ResolveValue(reference) - end.ResolveValue(reference);
    }

    /// One axis of the CSS 2.1 10.3.7 / 10.6.4 auto-margin resolution for an out-of-flow box:
    /// the offset to add to the start inset. Auto margins only ever receive space when the axis is
    /// OVER-CONSTRAINED -- both insets AND the size all non-auto -- because every other combination
    /// has an auto value of its own to solve for and resolves the margins to zero instead.
    /// `leftover` is what the containing block has left over after the two insets, the box's border
    /// box, and any non-auto margin: both margins auto split it (the `margin: auto` centring idiom),
    /// a lone auto margin takes all of it.
    /// `ltrSplitFloor` marks the horizontal axis, where 10.3.7 refuses to split a NEGATIVE leftover
    /// and instead zeroes margin-left (ltr) so margin-right absorbs it whole; 10.6.4 has no such
    /// carve-out, so the vertical axis splits unconditionally.
    float AutoMarginOffset(const CSSValue& startMargin, const CSSValue& endMargin,
                           float leftover, bool ltrSplitFloor)
    {
        const bool autoStart = startMargin.Unit == CSSUnit::Auto;
        const bool autoEnd = endMargin.Unit == CSSUnit::Auto;
        if (autoStart && autoEnd)
            return ltrSplitFloor && leftover < 0.0f ? 0.0f : leftover * 0.5f;
        if (autoStart) return leftover;
        return 0.0f; // only the end margin is auto (it absorbs the leftover), or neither is
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
    for (Node* p = m_Parent; p && !p->m_descendantDirty; p = p->m_Parent)
    {
        p->m_descendantDirty = true;
    }
}

void Node::StartUpdatingPositions(LayoutContext& ctx)
{
    // Clear dirty at end of frame (not mid-solve, which would hide a change from the
    // later definite-size pass).
    m_Style.Dirty = false;
    m_descendantDirty = false;
    m_positionsDirty = false;

    const float absX = m_Layout.ComputedX;
    const float absY = m_Layout.ComputedY;

    // Containing block for a relative child's percentage insets: this node's content box.
    const float cbWidth = std::max(0.0f, m_Layout.ComputedWidth
                                         - m_Style.GetPadding().Left - m_Style.GetPadding().Right
                                         - m_Style.GetBorder().WidthLeft - m_Style.GetBorder().WidthRight);
    const float cbHeight = std::max(0.0f, m_Layout.ComputedHeight
                                          - m_Style.GetPadding().Top - m_Style.GetPadding().Bottom
                                          - m_Style.GetBorder().WidthTop - m_Style.GetBorder().WidthBottom);

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
        // A display:none subtree generates no boxes: its strategy never ran this frame, so its
        // descendants' out-of-flow lists may be stale (and, with raw-pointer storage, dangling).
        // Do not derive positions for it or walk into it.
        if (child->GetStyle().GetDimensions().Display == OuterDisplay::None)
        {
            continue;
        }
        auto& childLayout = child->m_Layout;

        // `position: relative` is applied HERE rather than while the box was being laid out:
        // the strategies overwrite LocalX/LocalY wholesale, and CSS defines the shift as purely
        // visual anyway -- the box keeps its normal-flow position for every other purpose, so
        // siblings (which stack from LocalY/ComputedHeight, untouched below) never see it.
        float relX = 0.0f, relY = 0.0f;
        if (position == PositionType::Relative)
        {
            const auto& insets = child->m_Style.GetDimensions();
            relX = RelativeShift(insets.Left, insets.Right, cbWidth);
            relY = RelativeShift(insets.Top, insets.Bottom, cbHeight);
        }

        // Derive absolute from stable local (idempotent: a skipped clean subtree still
        // lands correctly when an ancestor moves).
        const float newX = absX + childLayout.LocalX + relX;
        const float newY = absY + childLayout.LocalY + relY;
        // NaN-safe: NaN != NaN forces a visit, never a skip.
        const bool originChanged = newX != childLayout.ComputedX || newY != childLayout.ComputedY;
        childLayout.ComputedX = newX;
        childLayout.ComputedY = newY;

        // Recurse only where something can have changed: the subtree moved, was re-solved
        // (m_positionsDirty), or carries dirt to clear. MarkDirtyToRoot flags every ancestor
        // and a strategy only runs while all ancestors' strategies are on the stack, so a
        // flagged node is always reachable through flagged ancestors — skipped subtrees are
        // flag-free by construction. Idle frames touch only the clean frontier.
        if (originChanged || child->m_positionsDirty || child->m_Style.Dirty || child->m_descendantDirty)
        {
            child->StartUpdatingPositions(ctx);
            child->PositionOutOfFlowChildren(ctx);
        }
    }
}

void Node::PositionOutOfFlowChildren(LayoutContext& ctx)
{
    // A display:none node generates no boxes, and its strategy did not run this frame
    // (LayoutImpl returns early), so m_OutOfFlowChildren was not rebuilt and may hold stale —
    // with raw-pointer storage, even dangling — entries. Drop them and skip the hidden subtree.
    if (m_Style.GetDimensions().Display == OuterDisplay::None)
    {
        m_OutOfFlowChildren.clear();
        return;
    }
    for (Node* const child : m_OutOfFlowChildren)
    {
        Node* ancestor = nullptr;
        float refWidth, refHeight;
        auto position = child->GetStyle().GetDimensions().Position;
        if (position == PositionType::Fixed)
        {
            // The containing block is the surface/viewport — the root node (it is pinned to the
            // surface extent). Resolve against the root's content box exactly like an absolute child
            // of the root, so Fixed pins to (or centres within) the viewport regardless of nesting.
            ancestor = child;
            while (ancestor->Parent()) ancestor = ancestor->Parent();
            refWidth = ancestor->m_Layout.ComputedWidth;
            refHeight = ancestor->m_Layout.ComputedHeight;
        }
        else
        {
            ancestor = FindContainingBlock(child);
            refWidth = ancestor->m_Layout.ComputedWidth;
            refHeight = ancestor->m_Layout.ComputedHeight;
        }

        // Measure against the containing block, but let an AUTO axis shrink-to-fit instead of
        // stretching to fill it (the cross axis would otherwise fill the whole block).
        const auto& cdim = child->GetStyle().GetDimensions();
        const float availW = OutOfFlowAvailable(cdim.Width, cdim.Left, cdim.Right, refWidth);
        const float availH = OutOfFlowAvailable(cdim.Height, cdim.Top, cdim.Bottom, refHeight);

        const bool widthPinned = cdim.Width.Unit == CSSUnit::Auto &&
            cdim.Left.Unit != CSSUnit::Auto && cdim.Right.Unit != CSSUnit::Auto;
        const bool heightPinned = cdim.Height.Unit == CSSUnit::Auto &&
            cdim.Top.Unit != CSSUnit::Auto && cdim.Bottom.Unit != CSSUnit::Auto;
        const bool childIsRow = child->GetStyle().GetFlex().IsRow();
        child->m_mainSizeDefinite = childIsRow ? widthPinned : heightPinned;
        child->m_crossSizeDefinite = childIsRow ? heightPinned : widthPinned;

        child->LayoutImpl(ctx, availW, availH);

        child->m_mainSizeDefinite = false;
        child->m_crossSizeDefinite = false;

        if (position == PositionType::Sticky)
        {
            child->HandleStickyPosition(refWidth, refHeight);
        }
        else
        {
            child->PositionOutOfFlowChild(ancestor, refWidth, refHeight);
        }

        // The main walk skips out-of-flow subtrees entirely; derive their descendants'
        // absolute coordinates from the origin just set, and re-position nested out-of-flow
        // lists. This runs in the same frame — no one-frame lag, no stale fix-ups.
        child->StartUpdatingPositions(ctx);
        child->PositionOutOfFlowChildren(ctx);
    }
}

void Node::Calculate(float availableWidth, float availableHeight)
{
    m_generation = BumpTreeGeneration();
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
    m_generation = BumpTreeGeneration();
    LayoutContext ctx;
    LayoutImpl(ctx, availableWidth, availableHeight, ignoreMinMax);
}

const Node::MeasureCacheEntry* Node::FindMeasure(float availW, float availH, bool ignoreMinMax) const
{
    if (m_generation == 0) return nullptr; // never solved under a frame stamp
    for (const auto& entry : m_measureCache)
    {
        if (entry.Generation == m_generation &&
            entry.IgnoreMinMax == ignoreMinMax &&
            SameSize(entry.AvailW, availW) && SameSize(entry.AvailH, availH))
            return &entry;
    }
    return nullptr;
}

void Node::RecordMeasure(float availW, float availH, bool ignoreMinMax, float resultW, float resultH)
{
    m_measureCache[m_measureCacheNext] = {m_generation, availW, availH, ignoreMinMax, resultW, resultH};
    m_measureCacheNext = static_cast<std::uint8_t>((m_measureCacheNext + 1) % MeasureCacheSize);
}

void Node::LayoutImpl(LayoutContext& ctx, float availableWidth, float availableHeight, bool ignoreMinMax)
{
    PullGeneration();

    if (m_Style.GetDimensions().Display == OuterDisplay::None)
    {
        m_Layout.ComputedWidth = m_Layout.ComputedHeight = 0.0f;
        return;
    }

    const bool spaceSame = SameSize(availableWidth, m_lastAvailW) &&
        SameSize(availableHeight, m_lastAvailH);

    // Full reuse: nothing changed and the space matches the last solve, so the cached layout
    // is still valid (dirty is cleared at end of frame, not here). Style::Modify propagates
    // m_DescendantDirty through every ancestor, so these two flags are the whole contract.
    if (spaceSame && !m_Style.Dirty && !m_descendantDirty)
    {
        // Report the content-box size, not a transient grow/shrink value an ancestor's resolve
        // may have left in ComputedWidth/Height (a re-solving parent reads it for flex basis).
        m_Layout.ComputedWidth = m_implW;
        m_Layout.ComputedHeight = m_implH;
        // Make the result replayable for this frame: if a later full solve at different
        // inputs overwrites m_LastAvail, a repeat call at these inputs must not re-solve.
        if (!FindMeasure(availableWidth, availableHeight, ignoreMinMax))
            RecordMeasure(availableWidth, availableHeight, ignoreMinMax, m_implW, m_implH);
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

    m_lastAvailW = availableWidth;
    m_lastAvailH = availableHeight;

    if (m_Style.Dirty || !spaceSame)
        ComputeDimensions(ctx, availableWidth, availableHeight, ignoreMinMax);

    LayoutStrategy::For(m_Style.GetDimensions().Display).Layout(*this, ctx, availableWidth, availableHeight);
    ++m_Layout.StrategyRuns;

    // Descendants now reflect this available-space run, not the last definite distribution;
    // the next definite pass must re-run even if its size memo matches. (Subsumes the old
    // shrink-wrapped-AUTO-main-axis special case.)
    m_strategyRanSinceDefinite = true;
    m_positionsDirty = true;

    // NOTE: `position: relative` offsets are deliberately NOT applied here. Every strategy
    // assigns its children's LocalX/LocalY wholesale after this returns (NormalFlowStrategy's
    // block branch, PositionLineOnMainAxis, AlignLinesOnCrossAxis), so a shift written at this
    // point is overwritten before anything reads it. It is applied in StartUpdatingPositions
    // instead, which is also where CSS puts it semantically: a relative box is laid out in
    // normal flow and only then visually offset, affecting nothing else's layout.

    // Only Block/InlineBlock get an AUTO-height override; flex handles its own height.
    const auto display = m_Style.GetDimensions().Display;
    const bool isBlock = display == OuterDisplay::Block || display == OuterDisplay::InlineBlock;

    if (isBlock && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto
        && !AutoHeightFillsInsetGap(m_Style.GetDimensions()))
        ApplyBlockAutoHeight();

    // Remember this run's content-box size for the reuse early-out (the parent may mutate
    // ComputedWidth/Height via flex grow/shrink before then), and make it replayable for
    // repeat same-input calls within this frame.
    m_implW = m_Layout.ComputedWidth;
    m_implH = m_Layout.ComputedHeight;
    RecordMeasure(availableWidth, availableHeight, ignoreMinMax, m_implW, m_implH);
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
    const bool stillDefinite = SameSize(borderBoxWidth, m_lastDefW) &&
        SameSize(borderBoxHeight, m_lastDefH) &&
        !m_strategyRanSinceDefinite;

    // Reuse across frames when the subtree is clean, or within the frame when this exact
    // distribution already ran this generation (the flex parent's second pass).
    if (stillDefinite &&
        ((!m_Style.Dirty && !m_descendantDirty) || (m_generation != 0 && m_defGeneration == m_generation)))
        return;

    m_strategyRanSinceDefinite = false;
    m_lastDefW = borderBoxWidth;
    m_lastDefH = borderBoxHeight;
    m_defGeneration = m_generation;

    // Border-box -> content-box for the strategy (ComputeDimensions re-adds padding+border,
    // so subtract them here exactly once).
    auto& padding = m_Style.GetPadding();
    auto& border = m_Style.GetBorder();
    const float horizontal = padding.Left + padding.Right + border.WidthLeft + border.WidthRight;
    const float vertical = padding.Top + padding.Bottom + border.WidthTop + border.WidthBottom;
    const float contentWidth = std::max(0.0f, borderBoxWidth - horizontal);
    const float contentHeight = std::max(0.0f, borderBoxHeight - vertical);

    // Drive the strategy directly (LayoutImpl would re-apply the Block AUTO-height override
    // and discard the adopted size). MainSizeIsDefinite tells flex to fill, not shrink-wrap;
    // CrossSizeIsDefinite tells a single flex line to clamp to this border box, not grow to a
    // taller item (the parent fixed both axes here).
    m_mainSizeDefinite = true;
    m_crossSizeDefinite = true;
    LayoutStrategy::For(m_Style.GetDimensions().Display).Layout(*this, ctx, contentWidth, contentHeight);
    m_mainSizeDefinite = false;
    m_crossSizeDefinite = false;
    ++m_Layout.StrategyRuns;
    m_positionsDirty = true;

    // Re-assert the definite border box (the strategy may rewrite it; guard rounding drift).
    m_Layout.ComputedWidth = borderBoxWidth;
    m_Layout.ComputedHeight = borderBoxHeight;
}

void Node::ApplyBlockAutoHeight()
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

bool Node::CrossSizeDependsOnMainSize(const std::uint64_t generation)
{
    if (generation != 0 && m_wrapScanGeneration == generation) return m_wrapInSubtree;
    m_wrapScanGeneration = generation;
    m_wrapInSubtree = false;

    const auto& dimensions = m_Style.GetDimensions();
    if (dimensions.Display == OuterDisplay::None) return false;
    // A leaf has nothing to reflow: `flex-wrap` on a childless box is a declaration about lines
    // that will never exist.
    if (m_Children.empty()) return false;

    const bool normalFlow = dimensions.Display == OuterDisplay::Block ||
        dimensions.Display == OuterDisplay::InlineBlock;
    if (!normalFlow && m_Style.GetFlex().Wrap != FlexWrap::NoWrap)
    {
        m_wrapInSubtree = true;
        return true;
    }

    for (const auto& child : m_Children)
    {
        // An out-of-flow child is sized against its containing block, not against this subtree's
        // main axis, and contributes nothing to this box's content size either way.
        const auto position = child->GetStyle().GetDimensions().Position;
        if (position != PositionType::Static && position != PositionType::Relative) continue;
        if (child->CrossSizeDependsOnMainSize(generation))
        {
            m_wrapInSubtree = true;
            break;
        }
    }
    return m_wrapInSubtree;
}

float Node::MeasureCrossAtDefiniteMain(LayoutContext& ctx, const bool mainIsHorizontal,
                                       const float mainBorderBox)
{
    PullGeneration();

    if (m_Style.GetDimensions().Display == OuterDisplay::None) return NAN;
    if (std::isnan(mainBorderBox)) return NAN;

    const auto display = m_Style.GetDimensions().Display;
    const bool isBlock = display == OuterDisplay::Block || display == OuterDisplay::InlineBlock;
    // A normal-flow box's width is handed to it, never derived from its height, so a column's block
    // item has nothing to re-measure. Only the block-in-a-row direction (height from content) does.
    if (isBlock && !mainIsHorizontal) return NAN;

    // Both flex passes of a frame ask at the same main size, and the answer is a pure function of it:
    // replay the second. Deliberately NOT the m_measureCache -- that keys on available space alone,
    // and a run with the main axis pinned resolves differently from a same-space run that
    // shrink-wraps it.
    if (m_generation != 0 && m_crossMeasureGeneration == m_generation
        && SameSize(mainBorderBox, m_crossMeasureMain))
        return m_crossMeasureCross;

    auto& padding = m_Style.GetPadding();
    auto& border = m_Style.GetBorder();
    const float horizontal = padding.Left + padding.Right + border.WidthLeft + border.WidthRight;
    const float vertical = padding.Top + padding.Bottom + border.WidthTop + border.WidthBottom;
    const float contentMain = std::max(0.0f, mainBorderBox - (mainIsHorizontal ? horizontal : vertical));

    // The main axis is handed in; the cross axis is the question, and NaN available space is what
    // makes the strategy shrink-wrap it -- the same input the basis pass uses.
    if (mainIsHorizontal)
    {
        m_Layout.ComputedWidth = mainBorderBox;
        m_Layout.ComputedHeight = NAN;
    }
    else
    {
        m_Layout.ComputedHeight = mainBorderBox;
        m_Layout.ComputedWidth = NAN;
    }

    // MainSizeIsDefinite, and ONLY that: the strategy must not shrink-wrap the main axis back to
    // content, and must stay free to resolve the cross axis from the lines it builds --
    // CrossSizeIsDefinite would clamp it to the very number this call exists to replace.
    m_mainSizeDefinite = true;
    LayoutStrategy::For(display).Layout(*this, ctx,
                                        mainIsHorizontal ? contentMain : NAN,
                                        mainIsHorizontal ? NAN : contentMain);
    m_mainSizeDefinite = false;
    ++m_Layout.StrategyRuns;

    // Driving the strategy directly skips LayoutImpl's normal-flow height override, which is where a
    // block's AUTO height actually comes from.
    if (isBlock && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto)
        ApplyBlockAutoHeight();

    // The descendants now reflect this run rather than the last definite distribution, so the
    // definite pass that follows must re-run even when its size memo matches.
    m_strategyRanSinceDefinite = true;
    m_positionsDirty = true;

    // Re-assert the main axis the caller fixed (the strategy may rewrite it) and report the cross
    // axis it just resolved. m_implW/m_implH are deliberately left alone: they hold the max-content
    // basis measure, which is what flex-basis derivation must keep seeing next frame.
    const float cross = mainIsHorizontal ? m_Layout.ComputedHeight : m_Layout.ComputedWidth;
    if (mainIsHorizontal) m_Layout.ComputedWidth = mainBorderBox;
    else m_Layout.ComputedHeight = mainBorderBox;

    m_crossMeasureGeneration = m_generation;
    m_crossMeasureMain = mainBorderBox;
    m_crossMeasureCross = cross;
    return cross;
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
    else if (AutoHeightFillsInsetGap(dimensions) && !std::isnan(availableHeight))
    {
        // availableHeight is already the top..bottom gap (Node.cpp's OutOfFlowAvailable). Shaped
        // like the AUTO-width branch above: subtract margins, padding and border here, because the
        // `!heightIsExplicit` clause below re-adds padding+border — the two together net out to
        // "fill the gap, less this box's own margins". Percentage margins resolve against the
        // containing block's WIDTH on every side, hence availableWidth as their reference.
        auto& margin = m_Style.GetMargin();
        computedHeight = std::max(0.0f, availableHeight
                                        - margin.Top.ResolveValue(availableWidth)
                                        - margin.Bottom.ResolveValue(availableWidth)
                                        - verticalPadding);
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

    // CSS 2.1 10.3.7 / 10.6.4: both insets AND an explicit size on one axis over-constrain it, and
    // whatever is left inside the inset box goes to that axis's `auto` margins -- the standard
    // `margin: auto` centring idiom. Only auto margins are read here; a non-auto margin on an
    // out-of-flow box still does not shift it, which is a separate gap.
    // Percentage margins resolve against the containing block's WIDTH on all four sides, so
    // refWidth is the reference even for the vertical pair.
    const auto& margin = m_Style.GetMargin();
    float autoMarginX = 0.0f, autoMarginY = 0.0f;
    if (hasLeft && hasRight && dimensions.Width.Unit != CSSUnit::Auto)
    {
        autoMarginX = AutoMarginOffset(margin.Left, margin.Right,
                                       refWidth
                                       - dimensions.Left.ResolveValue(refWidth)
                                       - dimensions.Right.ResolveValue(refWidth)
                                       - margin.Left.ResolveValue(refWidth)
                                       - margin.Right.ResolveValue(refWidth)
                                       - m_Layout.ComputedWidth,
                                       true);
    }
    if (hasTop && hasBottom && dimensions.Height.Unit != CSSUnit::Auto)
    {
        autoMarginY = AutoMarginOffset(margin.Top, margin.Bottom,
                                       refHeight
                                       - dimensions.Top.ResolveValue(refHeight)
                                       - dimensions.Bottom.ResolveValue(refHeight)
                                       - margin.Top.ResolveValue(refWidth)
                                       - margin.Bottom.ResolveValue(refWidth)
                                       - m_Layout.ComputedHeight,
                                       false);
    }

    // The box's own margins are terms in the same constraint equation as the insets, so the margin
    // on the anchoring side sits between that inset and the border box: `left: 10; margin-left: 5`
    // puts the border box at 15, and an end-anchored box is pulled BACK from its end inset by its
    // trailing margin. Auto margins resolve to 0 through ResolveValue, which is what leaves the
    // over-constrained distribution above as their only source of space.
    if (hasLeft || hasRight)
    {
        if (hasLeft)
        {
            m_Layout.ComputedX = cbX + dimensions.Left.ResolveValue(refWidth)
                + margin.Left.ResolveValue(refWidth) + autoMarginX;
        }
        else
        {
            m_Layout.ComputedX = cbX + refWidth
                - dimensions.Right.ResolveValue(refWidth)
                - margin.Right.ResolveValue(refWidth)
                - m_Layout.ComputedWidth;
        }
    }

    if (hasTop || hasBottom)
    {
        if (hasTop)
        {
            m_Layout.ComputedY = cbY + dimensions.Top.ResolveValue(refHeight)
                + margin.Top.ResolveValue(refWidth) + autoMarginY;
        }
        else
        {
            m_Layout.ComputedY = cbY + refHeight
                - dimensions.Bottom.ResolveValue(refHeight)
                - margin.Bottom.ResolveValue(refWidth)
                - m_Layout.ComputedHeight;
        }
    }

    // Auto-inset axes: place at the static position the containing block's flex alignment
    // implies (CSS/Yoga static position) instead of always pinning to the content origin. Only
    // a flex containing block contributes alignment; any other display keeps the top-left
    // origin. Auto insets opt in here — a 0px inset takes the hasLeft/hasTop branches above and
    // still resolves to the origin, so existing absolute layouts are unchanged.
    const bool autoX = !hasLeft && !hasRight;
    const bool autoY = !hasTop && !hasBottom;
    if (autoX || autoY)
    {
        // CSS defines the static position as where the box's MARGIN edge would have landed in flow,
        // so the container's alignment distributes the margin box and the leading margin then insets
        // the border box from it. Every inset on this axis is auto, so no auto margin can have been
        // fed here (that needs an over-constrained axis) and ResolveValue maps them to 0.
        const float marginLeft = margin.Left.ResolveValue(refWidth);
        const float marginTop = margin.Top.ResolveValue(refWidth);
        const float outerWidth = marginLeft + m_Layout.ComputedWidth
            + margin.Right.ResolveValue(refWidth);
        const float outerHeight = marginTop + m_Layout.ComputedHeight
            + margin.Bottom.ResolveValue(refWidth);

        float staticX = cbX + marginLeft, staticY = cbY + marginTop;
        if (ancestor && ancestor->m_Style.GetDimensions().Display == OuterDisplay::Flex)
        {
            const CSSFlex& flex = ancestor->m_Style.GetFlex();
            const auto& bor = ancestor->m_Style.GetBorder();
            const auto& pad = ancestor->m_Style.GetPadding();
            const float contentW = std::max(0.0f, refWidth - bor.WidthLeft - bor.WidthRight - pad.Left - pad.Right);
            const float contentH = std::max(0.0f, refHeight - bor.WidthTop - bor.WidthBottom - pad.Top - pad.Bottom);
            const float freeX = contentW - outerWidth;
            const float freeY = contentH - outerHeight;

            const AlignItems self = m_Style.GetFlex().AlignSelf; // child's own align-self wins
            const AlignItems cross = self != AlignItems::AutoAlign ? self : flex.Align;

            if (flex.IsRow()) // main = X, cross = Y
            {
                staticX = cbX + marginLeft + MainAxisStatic(flex.Justify, freeX);
                staticY = cbY + marginTop + CrossAxisStatic(cross, freeY);
            }
            else // main = Y, cross = X
            {
                staticY = cbY + marginTop + MainAxisStatic(flex.Justify, freeY);
                staticX = cbX + marginLeft + CrossAxisStatic(cross, freeX);
            }
        }

        if (autoX) m_Layout.ComputedX = staticX;
        if (autoY) m_Layout.ComputedY = staticY;
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
