#include "FlexLayoutStrategy.h"

#include "LayoutContext.h"
#include "Node.h"
#include "masharifcore/structure/CSSValue.h"
#include "masharifcore/structure/Justify.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>

using namespace masharif;

namespace {
    /// CSS resolves EVERY percentage margin -- including top/bottom -- against the containing
    /// block's WIDTH, never its height. `widthRef` must always be a width, regardless of which
    /// pair of edges (Left/Right for a row's main axis, Top/Bottom for a column's) is being summed.
    float NeededMainAxisMargin(const bool isRow, const MarginEdge &margin, const float widthRef) {
        if (isRow)
            return margin.Left.ResolveValue(widthRef) + margin.Right.ResolveValue(widthRef);
        return margin.Top.ResolveValue(widthRef) + margin.Bottom.ResolveValue(widthRef);
    }
}

/// One flex solve for one container. Phases run in CSS order; the recursive phases
/// (MeasureItemBases, RelayoutItemsAtDefiniteSize) re-enter child solves that share the
/// same context arenas, so they follow the ArenaSlice re-indexing rule strictly.
class FlexLayoutStrategy::Solver {
public:
    Solver(Node &container, LayoutContext &ctx,
           const float availableWidth, const float availableHeight)
        : m_Container(container),
          m_Ctx(ctx),
          m_Style(container.GetStyle()),
          m_Layout(container.GetLayout()),
          m_IsRow(m_Style.GetFlex().IsRow()),
          m_IsReverse(m_Style.GetFlex().IsReverse()),
          m_AvailableWidth(availableWidth),
          m_AvailableHeight(availableHeight),
          m_Items(ctx.InFlowItems),
          m_Lines(ctx.Lines) {
    }

    void Run() {
        CollectAndOrderItems();
        ShrinkAvailableToContentBox();
        MeasureItemBases();
        ResolveContainerSize();

        const float availableMainAxisSize = m_IsRow ? m_AvailableWidth : m_AvailableHeight;
        const auto &border = m_Style.GetBorder();
        // crossPos is the container's own content-box origin along the cross axis: padding AND
        // border inset it, same as the absolute-positioning containing-block origin does.
        float crossPos = m_IsRow
                             ? m_Container.PaddingTop() + border.WidthTop.Value
                             : m_Container.PaddingLeft() + border.WidthLeft.Value;

        // Invariant across the line loop: the container's main size and paddings do not
        // change while lines are built and placed.
        const float containerMainSize = m_IsRow ? m_Layout.ComputedWidth : m_Layout.ComputedHeight;
        // A percentage padding resolves against the CONTAINING BLOCK's width on all four sides --
        // neither `containerMainSize` (this container's HEIGHT for a column) nor this box's own
        // width. Node::PaddingLeft and friends hold that reference. Border is added alongside
        // padding: both inset the content-box origin from the border-box edge.
        const float padStart = m_IsRow
                                   ? m_Container.PaddingLeft() + border.WidthLeft.Value
                                   : m_Container.PaddingTop() + border.WidthTop.Value;
        const float padEnd = m_IsRow
                                 ? m_Container.PaddingRight() + border.WidthRight.Value
                                 : m_Container.PaddingBottom() + border.WidthBottom.Value;
        const float availableSpace = containerMainSize - padStart - padEnd;

        while (m_NextItem < m_Items.Count()) {
            const std::size_t lineIndex = m_Lines.Count();
            m_Lines.Append(BuildLine(availableMainAxisSize));
            ResolveFlexibleLengths(m_Lines[lineIndex], availableSpace);
            // Re-enters child solves, which append to the shared line arena: index, never hold a
            // FlexLine reference across it.
            RemeasureFlexedItems(lineIndex);
            PositionLineOnMainAxis(m_Lines[lineIndex], availableSpace, containerMainSize, padStart, padEnd,
                                   crossPos);
            crossPos += m_Lines[lineIndex].CrossSize;
        }

        ReShrinkWrapCrossSize();
        ResolveAutoCrossSizeFromLines();
        AlignLinesOnCrossAxis();
        RelayoutItemsAtDefiniteSize();
    }

private:
    [[nodiscard]] std::size_t LineItemCount(const FlexLine &line) const noexcept {
        return line.ItemEnd - line.ItemBegin;
    }

    /// Split children into the in-flow item slice and the container's out-of-flow list,
    /// then order the items by CSS `order` when any item overrides the default.
    void CollectAndOrderItems() {
        // Detect any non-default `order` while collecting, instead of a second O(n) any_of
        // scan over m_Items. Accumulate ONLY in the in-flow branch so the flag's domain
        // matches the items the sort sees (out-of-flow children are never ordered).
        bool anyOrder = false;
        for (auto &child: m_Container.m_Children) {
            // display:none generates no box at all — skip it for BOTH in-flow and out-of-flow
            // layout. It must not enter m_OutOfFlowChildren (it would be measured and positioned
            // as a 0x0 box) nor m_Items.
            if (child->GetStyle().GetDimensions().Display == OuterDisplay::None)
                continue;
            const auto pos = child->GetStyle().GetDimensions().Position;
            if (pos != PositionType::Static && pos != PositionType::Relative) {
                m_Container.m_OutOfFlowChildren.push_back(child.get());
            } else {
                m_Items.Append(child.get());
                anyOrder = anyOrder || child->GetStyle().GetFlex().Order != 0;
            }
        }

        if (anyOrder) {
            std::ranges::stable_sort(m_Items.begin(), m_Items.end(), {},
                                     [](Node *n) { return n->GetStyle().GetFlex().Order; });
        }
    }

    /// Shrink available space to the container's content box when the container has an
    /// explicit size, so 100%-sized children of a padded container don't overflow.
    void ShrinkAvailableToContentBox() {
        const auto &dim = m_Style.GetDimensions();
        const bool widthExplicit = dim.Width.Unit == CSSUnit::Px || dim.Width.Unit == CSSUnit::Percent;
        const bool heightExplicit = dim.Height.Unit == CSSUnit::Px || dim.Height.Unit == CSSUnit::Percent;
        if (widthExplicit && !std::isnan(m_Layout.ComputedWidth)) {
            m_AvailableWidth = m_Layout.ComputedWidth - m_Container.PaddingBorderHorizontal();
        }
        if (heightExplicit && !std::isnan(m_Layout.ComputedHeight)) {
            m_AvailableHeight = m_Layout.ComputedHeight - m_Container.PaddingBorderVertical();
        }
    }

    /// Clamp a candidate main-axis size to the item's own min/max on that axis. Reference is the
    /// container's available main size, matching what ResolveFlexibleLengths resolves against.
    [[nodiscard]] float ClampToMainMinMax(Node *child, float value) const {
        const auto &dims = child->GetStyle().GetDimensions();
        const float ref = m_IsRow ? m_AvailableWidth : m_AvailableHeight;
        const CSSValue &maxEdge = m_IsRow ? dims.MaxWidth : dims.MaxHeight;
        const CSSValue &minEdge = m_IsRow ? dims.MinWidth : dims.MinHeight;
        if (maxEdge.Unit != CSSUnit::Auto) value = std::min(value, maxEdge.ResolveValue(ref));
        if (minEdge.Unit != CSSUnit::Auto) value = std::max(value, minEdge.ResolveValue(ref));
        return value;
    }

    /// Same, on the cross axis. Nothing else applies a cross min/max to a flex item: the item's own
    /// ComputeDimensions skips the clamp for an AUTO size (there is no number to clamp yet) and the
    /// basis measure below runs with ignoreMinMax, so without this a `min-height` on a row's child --
    /// or a `min-width` on a column's -- is parsed, resolved and then never honoured.
    [[nodiscard]] float ClampToCrossMinMax(Node *child, float value) const {
        const auto &dims = child->GetStyle().GetDimensions();
        const float ref = m_IsRow ? m_AvailableHeight : m_AvailableWidth;
        const CSSValue &maxEdge = m_IsRow ? dims.MaxHeight : dims.MaxWidth;
        const CSSValue &minEdge = m_IsRow ? dims.MinHeight : dims.MinWidth;
        if (maxEdge.Unit != CSSUnit::Auto) value = std::min(value, maxEdge.ResolveValue(ref));
        if (minEdge.Unit != CSSUnit::Auto) value = std::max(value, minEdge.ResolveValue(ref));
        return value;
    }

    /// Compute each item's flex basis, and accumulate the container's content total from each item's
    /// *hypothetical main size* -- the basis clamped by that item's own min/max (Flexbox 9.2 step 4).
    ///
    /// The two are separate on purpose. ComputedFlexBasis keeps the TRUE, unclamped flex base size,
    /// because the freeze loop in ResolveFlexibleLengths reads it as its starting point and weights
    /// shrinking by it -- clamping it here would hand a grow item a floor it then grows *on top of*
    /// (a min-width:60 grow-1 item next to a plain grow-1 item resolves 80/20 instead of 60/40) and
    /// would skew the shrink ratio between two items that differ only by a max constraint.
    ///
    /// m_TotalMainSize, on the other hand, must be the clamped total: ResolveContainerSize runs
    /// immediately after this and freezes a shrink-to-fit (AUTO) main axis at it, while
    /// ResolveFlexibleLengths later clamps the items upward and PositionLineOnMainAxis lays them out
    /// at those clamped sizes. Summed unclamped, the container ends shorter than the children it
    /// positions and every trailing sibling lands inside its predecessor. One pixel per row is
    /// invisible; a column of twenty min-height rows draws its next sibling across its own last rows.
    void MeasureItemBases() {
        const std::size_t count = m_Items.Count();
        for (std::size_t i = 0; i < count; ++i) {
            Node *child = m_Items[i]; // copy out: the recursive solve below may grow the arena

            // The item's containing block is THIS container's content box. Handed over before the
            // solve, because the AUTO-main branch just below deliberately measures at NaN width and
            // a percentage padding must not resolve against that (nor against the item's own size).
            child->SetPercentBasis(m_AvailableWidth);

            float childAvailW = m_AvailableWidth;
            float childAvailH = m_AvailableHeight;
            const auto &dim = child->GetStyle().GetDimensions();
            if (m_IsRow && dim.Width.Unit == CSSUnit::Auto)
                childAvailW = std::numeric_limits<float>::quiet_NaN();
            if (!m_IsRow && dim.Height.Unit == CSSUnit::Auto)
                childAvailH = std::numeric_limits<float>::quiet_NaN();

            child->LayoutImpl(m_Ctx, childAvailW, childAvailH, /*ignoreMinMax=*/true);

            auto &childLayout = child->GetLayout();
            const auto &childStyle = child->GetStyle();

            const float basisRef = m_IsRow ? m_AvailableWidth : m_AvailableHeight;
            if (childStyle.GetFlex().FlexBasis.Unit == CSSUnit::Auto) {
                const float resolved = m_IsRow ? childLayout.ComputedWidth : childLayout.ComputedHeight;
                childLayout.ComputedFlexBasis = std::isnan(resolved) ? 0.0f : resolved;
            } else {
                const float pb = m_IsRow ? child->PaddingBorderHorizontal()
                                         : child->PaddingBorderVertical();
                childLayout.ComputedFlexBasis = childStyle.GetFlex().FlexBasis.ResolveValue(basisRef) + pb;
            }

            const float hypothetical = ClampToMainMinMax(child, childLayout.ComputedFlexBasis);

            // The measured cross size is the item's own AUTO result, which ignored its cross min/max
            // twice over (see ClampToCrossMinMax). Clamp it and write it back, so the line's cross
            // size, align-items placement and the definite relayout all see the same number.
            const float measuredCross = m_IsRow ? childLayout.ComputedHeight : childLayout.ComputedWidth;
            if (!std::isnan(measuredCross)) {
                const float clampedCross = ClampToCrossMinMax(child, measuredCross);
                if (m_IsRow) childLayout.ComputedHeight = clampedCross;
                else childLayout.ComputedWidth = clampedCross;
            }

            // Main-axis margins occupy main-axis space alongside the basis (BuildLine counts them
            // when packing a line), so they belong in the content total: a shrink-to-fit (AUTO) main
            // axis must enclose its children's MARGIN boxes, not just their border boxes.
            m_TotalMainSize += hypothetical
                               + NeededMainAxisMargin(m_IsRow, childStyle.GetMargin(), m_AvailableWidth);
            m_MaxCrossSize = std::max(m_MaxCrossSize,
                                      m_IsRow ? childLayout.ComputedHeight : childLayout.ComputedWidth);
        }

        // Inter-item gaps occupy main-axis space too (BuildLine adds gapSize between items); fold the
        // single line's (count-1) gaps into the content total, or an AUTO main axis collapses by exactly
        // the gaps and its children spill past it. Mirrors BuildLine's gap axis: row-gap stacks a column,
        // column-gap a row. (NoWrap single line — the only shape this engine emits.)
        if (count > 1)
        {
            const auto &gaps = m_Style.GetFlex().Gaps;
            m_TotalMainSize += static_cast<float>(count - 1)
                               * (m_IsRow ? gaps.Column.ResolveValue(m_AvailableWidth)
                                          : gaps.Row.ResolveValue(m_AvailableHeight));
        }
    }

    /// Resolve NaN available space and size both axes that are still AUTO from content: the main
    /// axis to the items' total (unless the parent has already fixed it — MainSizeIsDefinite), the
    /// cross axis to the tallest/widest item (unless the parent has already stretched it —
    /// CrossSizeIsDefinite). Neither AUTO axis fills the space it was offered: on the main axis
    /// that is shrink-to-fit, on the cross axis it is Flexbox 9.4's hypothetical cross size, which
    /// only the parent's `align-self: stretch` may afterwards expand.
    void ResolveContainerSize() {
        const float pbRow = m_Container.PaddingBorderHorizontal();
        const float pbCol = m_Container.PaddingBorderVertical();

        if (std::isnan(m_AvailableWidth)) {
            if (m_Style.GetDimensions().Width.Unit == CSSUnit::Auto) {
                m_Layout.ComputedWidth = (m_IsRow ? m_TotalMainSize : m_MaxCrossSize) + pbRow;
                if (!m_IsRow) m_CrossShrinkWrapped = true;
            }
            m_AvailableWidth = m_Layout.ComputedWidth - pbRow;
        } else if (m_IsRow && m_Style.GetDimensions().Width.Unit == CSSUnit::Auto) {
            // AUTO main axis shrink-wraps to content -- UNLESS the parent already fixed this box's
            // main size (a block-level box filling its containing block, or a definite-size
            // distribution). Then keep the width already resolved for it and only re-derive the
            // content box from it: the incoming available space is the PARENT's content box, so it
            // still carries this box's own margins, padding and border.
            if (!m_Container.MainSizeIsDefinite())
                m_Layout.ComputedWidth = m_TotalMainSize + pbRow;
            else if (std::isnan(m_Layout.ComputedWidth))
                m_Layout.ComputedWidth = m_AvailableWidth;
            m_AvailableWidth = m_Layout.ComputedWidth - pbRow;
        } else if (!m_IsRow && m_Style.GetDimensions().Width.Unit == CSSUnit::Auto
                   && m_Container.IsFlexItem() && !m_Container.CrossSizeIsDefinite()) {
            // The column's AUTO cross size, mirroring the height branch below: content-sized here,
            // widened to the line only by the parent's `align-self: stretch`. ComputeDimensions has
            // already filled ComputedWidth with the whole available extent by this point, because
            // that IS the rule for a block-level box -- hence the IsFlexItem() guard, without which
            // this would re-collapse every normal-flow flex container (see gh#3).
            m_Layout.ComputedWidth = m_MaxCrossSize + pbRow;
            m_CrossShrinkWrapped = true;
            m_AvailableWidth = m_Layout.ComputedWidth - pbRow;
        } else if (std::isnan(m_Layout.ComputedWidth)) {
            m_Layout.ComputedWidth = m_AvailableWidth;
        }

        if (std::isnan(m_AvailableHeight)) {
            if (m_Style.GetDimensions().Height.Unit == CSSUnit::Auto) {
                m_Layout.ComputedHeight = (!m_IsRow ? m_TotalMainSize : m_MaxCrossSize) + pbCol;
                if (m_IsRow) m_CrossShrinkWrapped = true;
            }
            m_AvailableHeight = m_Layout.ComputedHeight - pbCol;
        } else if (!m_IsRow && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto
                   && !m_Container.MainSizeIsDefinite()) {
            m_Layout.ComputedHeight = m_TotalMainSize + pbCol;
            m_AvailableHeight = m_Layout.ComputedHeight - pbCol;
        } else if (m_IsRow && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto
                   && !m_Container.CrossSizeIsDefinite()) {
            // AUTO cross size is CONTENT-based (Flexbox 9.4: the hypothetical cross size). Expanding
            // it to the line is `align-self: stretch`, and that is the PARENT's job -- its
            // AlignLinesOnCrossAxis stretches the item and then re-solves it through
            // LayoutContentsWithDefiniteSize, which is the CrossSizeIsDefinite case excluded here.
            // Filling the available cross extent at this point pre-empts that decision: the box
            // takes the whole line whatever the parent's align-items says, so it is both the wrong
            // size and left with a zero-length gap for center/flex-end to move it inside.
            m_Layout.ComputedHeight = m_MaxCrossSize + pbCol;
            m_CrossShrinkWrapped = true;
            m_AvailableHeight = m_Layout.ComputedHeight - pbCol;
        } else if (std::isnan(m_Layout.ComputedHeight)) {
            m_Layout.ComputedHeight = m_AvailableHeight;
        }
    }

    /// Greedily pack items into one flex line starting at m_NextItem. Advances the
    /// cursor past every item consumed; on wrap it is left at the next line's first item.
    [[nodiscard]] FlexLine BuildLine(const float lineMainSize) {
        const CSSFlex &flex = m_Style.GetFlex();
        const bool isWrap = flex.Wrap == FlexWrap::Wrap ||
                            flex.Wrap == FlexWrap::WrapReverse;
        const auto &gaps = flex.Gaps;
        const float gapSize = m_IsRow
                                  ? gaps.Column.ResolveValue(m_Layout.ComputedWidth)
                                  : gaps.Row.ResolveValue(m_Layout.ComputedHeight);

        float takenSize = 0;
        bool foundFirst = false;
        float totalFlexGrow = 0, totalFlexShrinkScaled = 0;
        int numberOfAutoMargin = 0;
        float totalCrossSize = 0;

        const std::size_t itemBegin = m_NextItem;
        const std::size_t total = m_Items.Count();

        for (; m_NextItem < total; ++m_NextItem) {
            Node *child = m_Items[m_NextItem];
            const auto &childStyle = child->GetStyle();
            const auto &childLayout = child->GetLayout();

            const auto &margin = childStyle.GetMargin();
            if (m_IsRow) {
                if (margin.Left.Unit == CSSUnit::Auto) numberOfAutoMargin++;
                if (margin.Right.Unit == CSSUnit::Auto) numberOfAutoMargin++;
                totalCrossSize = std::max(totalCrossSize, childLayout.ComputedHeight);
            } else {
                totalCrossSize = std::max(totalCrossSize, childLayout.ComputedWidth);
                if (margin.Top.Unit == CSSUnit::Auto) numberOfAutoMargin++;
                if (margin.Bottom.Unit == CSSUnit::Auto) numberOfAutoMargin++;
            }

            const float neededMargin = NeededMainAxisMargin(m_IsRow, margin, lineMainSize);
            float newTaken = takenSize + neededMargin + childLayout.ComputedFlexBasis;
            if (foundFirst) newTaken += gapSize;

            if (newTaken > lineMainSize && foundFirst && isWrap)
                break; // the cursor is NOT advanced — this item opens the next line

            foundFirst = true;
            const CSSFlex &childFlex = childStyle.GetFlex();
            totalFlexGrow += childFlex.FlexGrow;
            totalFlexShrinkScaled += childFlex.FlexShrink * childLayout.ComputedFlexBasis;
            takenSize = newTaken;
        }

        return {
            .ItemBegin = itemBegin,
            .ItemEnd = m_NextItem,
            .NumberOfAutoMargin = numberOfAutoMargin,
            .TakenSize = takenSize,
            .TotalFlexGrow = totalFlexGrow,
            .TotalFlexShrinkScaledFactors = totalFlexShrinkScaled,
            .CrossSize = totalCrossSize
        };
    }

    /// CSS flexible-length resolution: distribute free space by grow/shrink factors,
    /// clamp to min/max, freeze violators, repeat until stable.
    void ResolveFlexibleLengths(const FlexLine &line, const float availableSpace) {
        const std::size_t n = LineItemCount(line);
        if (n == 0) return;

        const float remainingSpace = availableSpace - line.TakenSize;

        const bool isGrowing = remainingSpace > 0 && line.TotalFlexGrow > 0;
        const bool isShrinking = remainingSpace < 0 && line.TotalFlexShrinkScaledFactors != 0;

        // Inflexible line (neither growing nor shrinking): the distribution term is provably
        // zero, so the full freeze loop reduces to a single min/max clamp per item that
        // converges immediately. Skip the baseSizes/frozen arenas and the loop entirely. The
        // clamp block below is byte-identical to the one in the freeze loop (max-clamp first,
        // then min-clamp, so min wins when min > max) and uses the same resolve references.
        if (!isGrowing && !isShrinking) {
            for (std::size_t i = 0; i < n; i++) {
                Node *child = m_Items[line.ItemBegin + i];
                const auto &dims = child->GetStyle().GetDimensions();
                float clamped = child->GetLayout().ComputedFlexBasis;
                if (m_IsRow) {
                    if (dims.MaxWidth.Unit != CSSUnit::Auto)
                        clamped = std::min(clamped, dims.MaxWidth.ResolveValue(m_AvailableWidth));
                    if (dims.MinWidth.Unit != CSSUnit::Auto)
                        clamped = std::max(clamped, dims.MinWidth.ResolveValue(m_AvailableWidth));
                } else {
                    if (dims.MaxHeight.Unit != CSSUnit::Auto)
                        clamped = std::min(clamped, dims.MaxHeight.ResolveValue(m_AvailableHeight));
                    if (dims.MinHeight.Unit != CSSUnit::Auto)
                        clamped = std::max(clamped, dims.MinHeight.ResolveValue(m_AvailableHeight));
                }
                child->GetLayout().ComputedFlexBasis = clamped;
            }
            return;
        }

        ArenaSlice<float> baseSizes(m_Ctx.BaseSizes);
        ArenaSlice<std::uint8_t> frozen(m_Ctx.Frozen);
        baseSizes.Resize(n);
        frozen.Resize(n);

        // Item main-axis margins are invariant across the freeze loop below (they depend only
        // on style + width, never on the basis/frozen state the loop mutates), so sum them ONCE
        // here instead of re-resolving every item on every iteration. Percentage margins always
        // resolve against the container's WIDTH (CSS rule), not `availableSpace` -- which is a
        // height for a column container.
        float fixedMainMargin = 0;
        for (std::size_t i = 0; i < n; i++) {
            Node *child = m_Items[line.ItemBegin + i];
            baseSizes[i] = child->GetLayout().ComputedFlexBasis;
            fixedMainMargin += NeededMainAxisMargin(m_IsRow, child->GetStyle().GetMargin(), m_AvailableWidth);
            const auto &flex = child->GetStyle().GetFlex();
            frozen[i] = static_cast<std::uint8_t>(
                (isGrowing && flex.FlexGrow == 0) || (isShrinking && flex.FlexShrink == 0) ? 1 : 0);

            // "Size inflexible items: freeze, setting its target main size to its HYPOTHETICAL main
            // size" (Flexbox 9.7 step 2). The distinction is the whole point for an item frozen out of
            // the distribution: the loop below never revisits it, so its min/max would otherwise apply
            // nowhere at all -- a min-height row sharing a line with one grow sibling would render at
            // its content height and ignore the floor. Unfrozen items keep the unclamped base size,
            // which is what the grow/shrink weighting is defined over.
            if (frozen[i]) child->GetLayout().ComputedFlexBasis = ClampToMainMinMax(child, baseSizes[i]);
        }

        const float gapSize = m_IsRow
                                  ? m_Style.GetFlex().Gaps.Column.ResolveValue(m_Layout.ComputedWidth)
                                  : m_Style.GetFlex().Gaps.Row.ResolveValue(m_Layout.ComputedHeight);

        for (;;) {
            // Single pass: compute free space + sum unfrozen factors.
            float unfrozenFlexGrow = 0;
            float unfrozenScaledShrink = 0;
            // Item margins consume main-axis space exactly like gaps and bases do (BuildLine
            // already counts them in line.TakenSize), so subtract the precomputed total here —
            // otherwise a grow item also absorbs its siblings' margins and shoves the trailing
            // items off the container.
            float freeSpace = availableSpace - fixedMainMargin;
            if (n > 1) freeSpace -= static_cast<float>(n - 1) * gapSize;

            for (std::size_t i = 0; i < n; i++) {
                Node *child = m_Items[line.ItemBegin + i];
                if (frozen[i]) {
                    freeSpace -= child->GetLayout().ComputedFlexBasis; // frozen: current (clamped) basis
                } else {
                    const auto &flex = child->GetStyle().GetFlex();
                    unfrozenFlexGrow += flex.FlexGrow;
                    unfrozenScaledShrink += flex.FlexShrink * baseSizes[i];
                    freeSpace -= baseSizes[i]; // unfrozen: original basis
                }
            }

            // Single pass: distribute free space + clamp to min/max + freeze violators.
            bool anyNewFreezes = false;
            for (std::size_t i = 0; i < n; i++) {
                if (frozen[i]) continue;
                Node *child = m_Items[line.ItemBegin + i];

                float newSize = baseSizes[i];
                if (isGrowing && unfrozenFlexGrow > 0) {
                    newSize += (child->GetStyle().GetFlex().FlexGrow / unfrozenFlexGrow) * freeSpace;
                } else if (isShrinking && unfrozenScaledShrink != 0) {
                    const float sf = child->GetStyle().GetFlex().FlexShrink * baseSizes[i];
                    newSize += (sf / unfrozenScaledShrink) * freeSpace;
                }

                const auto &dims = child->GetStyle().GetDimensions();
                float clamped = newSize;
                if (m_IsRow) {
                    if (dims.MaxWidth.Unit != CSSUnit::Auto)
                        clamped = std::min(clamped, dims.MaxWidth.ResolveValue(m_AvailableWidth));
                    if (dims.MinWidth.Unit != CSSUnit::Auto)
                        clamped = std::max(clamped, dims.MinWidth.ResolveValue(m_AvailableWidth));
                } else {
                    if (dims.MaxHeight.Unit != CSSUnit::Auto)
                        clamped = std::min(clamped, dims.MaxHeight.ResolveValue(m_AvailableHeight));
                    if (dims.MinHeight.Unit != CSSUnit::Auto)
                        clamped = std::max(clamped, dims.MinHeight.ResolveValue(m_AvailableHeight));
                }

                if (clamped != newSize) {
                    frozen[i] = 1;
                    anyNewFreezes = true;
                    newSize = clamped;
                }
                child->GetLayout().ComputedFlexBasis = newSize;
            }

            if (!anyNewFreezes) break;
        }
    }

    /// Place the line's items along the main axis: justify-content offset/gap, auto
    /// margins, then per-item local position and main size.
    void PositionLineOnMainAxis(const FlexLine &line, const float availableSpace,
                                const float containerMainSize, const float padStart,
                                const float padEnd, const float crossPos) {
        const std::size_t itemCount = LineItemCount(line);

        float gap = m_IsRow
                        ? m_Style.GetFlex().Gaps.Column.ResolveValue(m_AvailableWidth)
                        : m_Style.GetFlex().Gaps.Row.ResolveValue(m_AvailableHeight);

        // Everything the line has already spent on the main axis: the items' resolved main sizes,
        // the (count-1) inter-item gaps, and the items' own non-auto main-axis margins. Gaps and
        // margins occupy main-axis space exactly as the item boxes do, so none of it is free -- and
        // free space is what BOTH auto margins (8.1, below) and justify-content divide up.
        // Counting gaps/margins as free lands every justify-content value other than flex-start
        // off by the total gap (or a fraction of it) and pushes the line past its container's end
        // edge. ResolveFlexibleLengths already measures free space this way for grow/shrink.
        //
        // Measured AFTER flexible lengths resolved -- 9.7 runs before 8.1, so a grow item takes the
        // free space first and an auto margin only gets what survives; measuring before would hand
        // the same pixels out twice. `gap` is still the declared gap here: the distributive
        // justify-content values fold their share into it further down, and that share IS this free
        // space -- counting it twice would leave nothing for the auto margins to take.
        // Percentage margins resolve against the container's WIDTH on every side, hence
        // m_AvailableWidth even when the main axis is a column's.
        float takenAfterResolve = itemCount > 1 ? gap * static_cast<float>(itemCount - 1) : 0.0f;
        for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i) {
            takenAfterResolve += m_Items[i]->GetLayout().ComputedFlexBasis;
            takenAfterResolve += NeededMainAxisMargin(m_IsRow, m_Items[i]->GetStyle().GetMargin(),
                                                     m_AvailableWidth);
        }

        const float remainingSpace = availableSpace - takenAfterResolve;

        // CSS Flexbox 8.1: POSITIVE free space on the line is split equally between every auto
        // main-axis margin on it. line.NumberOfAutoMargin counts MARGINS, not the items carrying
        // one, so a lone `margin-right: auto` takes the whole of the free space while a symmetric
        // `margin: 0 auto` takes half on each side. Negative free space has nothing to give: the
        // auto margins resolve to 0 and the line overflows, rather than every auto margin taking a
        // share of the overflow and dragging the line backwards off its own container.
        const float autoMarginShare = line.NumberOfAutoMargin > 0 && remainingSpace > 0.0f
                                          ? remainingSpace / static_cast<float>(line.NumberOfAutoMargin)
                                          : 0.0f;

        const float mainStartPos = m_IsReverse ? (containerMainSize - padEnd) : padStart;

        // Auto margins absorb ALL the positive free space before justify-content is consulted, so a
        // line where they took any leaves justify-content nothing to distribute. Offsetting on top
        // of margins that already spent those pixels pushed the line past the container's end edge.
        float mainOffset = 0;
        if (autoMarginShare <= 0.0f) switch (m_Style.GetFlex().Justify) {
            case JustifyContent::FlexStart:
                break;
            case JustifyContent::FlexEnd:
                mainOffset += remainingSpace;
                break;
            case JustifyContent::FlexCenter:
                mainOffset += remainingSpace / 2.0f;
                break;
            case JustifyContent::SpaceBetween:
                if (itemCount > 1)
                    gap += remainingSpace / static_cast<float>(itemCount - 1);
                break;
            case JustifyContent::SpaceAround:
                mainOffset += remainingSpace / static_cast<float>(itemCount * 2);
                gap += remainingSpace / static_cast<float>(itemCount);
                break;
            case JustifyContent::SpaceEvenly:
                mainOffset += remainingSpace / static_cast<float>(itemCount + 1);
                gap += remainingSpace / static_cast<float>(itemCount + 1);
                break;
            case JustifyContent::Stretch:
                break;
        }

        float currentPosition = mainStartPos + (m_IsReverse ? -mainOffset : mainOffset);

        for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i) {
            Node *child = m_Items[i];
            auto &childLayout = child->GetLayout();
            const auto &childStyle = child->GetStyle();

            // A side takes the auto share ONLY if that side is the one declared `auto`; the other
            // side keeps its resolved value. Writing the share into both sides regardless is what
            // made `margin-right: auto` move the item itself forward by the whole free space
            // instead of leaving it flush and pushing its siblings away from it.
            //
            // Percentage margins always resolve against the container's WIDTH, including
            // Top/Bottom in a column container -- never m_AvailableHeight.
            const CSSValue &mainStartEdge = m_IsRow
                                                ? childStyle.GetMargin().Left
                                                : childStyle.GetMargin().Top;
            const CSSValue &mainEndEdge = m_IsRow
                                              ? childStyle.GetMargin().Right
                                              : childStyle.GetMargin().Bottom;
            const float marginStart = mainStartEdge.Unit == CSSUnit::Auto
                                          ? autoMarginShare
                                          : mainStartEdge.ResolveValue(m_AvailableWidth);
            const float marginEnd = mainEndEdge.Unit == CSSUnit::Auto
                                        ? autoMarginShare
                                        : mainEndEdge.ResolveValue(m_AvailableWidth);

            if (m_IsRow) {
                childLayout.LocalY = crossPos;
                childLayout.LocalX = m_IsReverse
                                         ? currentPosition - childLayout.ComputedFlexBasis - marginEnd
                                         : currentPosition + marginStart;
                childLayout.ComputedWidth = childLayout.ComputedFlexBasis;
            } else {
                childLayout.LocalX = crossPos;
                childLayout.LocalY = m_IsReverse
                                         ? currentPosition - childLayout.ComputedFlexBasis - marginEnd
                                         : currentPosition + marginStart;
                childLayout.ComputedHeight = childLayout.ComputedFlexBasis;
            }

            const float direction = m_IsReverse ? -1.0f : 1.0f;
            currentPosition += direction * (marginStart + childLayout.ComputedFlexBasis + marginEnd + gap);
        }
    }

    /// The gap BETWEEN flex lines, which is the mirror of BuildLine's gap between items: the lines of
    /// a row container stack vertically and are therefore separated by row-gap, while its items run
    /// horizontally and are separated by column-gap.
    [[nodiscard]] float LineGap() const {
        const auto &gaps = m_Style.GetFlex().Gaps;
        return m_IsRow
                   ? gaps.Row.ResolveValue(m_Layout.ComputedHeight)
                   : gaps.Column.ResolveValue(m_Layout.ComputedWidth);
    }

    /// Re-measure the items whose cross size depends on the main size they were just given.
    ///
    /// MeasureItemBases measures an AUTO-main item against NaN -- max-content, which for a wrapping
    /// container means "everything on one line". ResolveFlexibleLengths then hands that item a
    /// different main size, and a wrapping container answers a narrower main axis with MORE lines: the
    /// cross size measured in the basis phase is a one-line answer to a question the layout no longer
    /// asks. Nothing downstream re-asked it -- RelayoutItemsAtDefiniteSize feeds the stale cross size
    /// back in as DEFINITE, which is also what silences ResolveAutoCrossSizeFromLines inside the item
    /// -- so the extra lines were laid out correctly and drawn straight past the item's own bottom
    /// edge, and past the container's with it.
    ///
    /// Runs between ResolveFlexibleLengths (which leaves the final main size in ComputedFlexBasis) and
    /// PositionLineOnMainAxis (which consumes `crossPos`, so a line's cross size must be final before
    /// the next line is placed). Line cross sizes are recomputed from scratch rather than accumulated:
    /// growing an item's main axis removes lines exactly as shrinking it adds them.
    void RemeasureFlexedItems(const std::size_t lineIndex) {
        const std::size_t begin = m_Lines[lineIndex].ItemBegin;
        const std::size_t end = m_Lines[lineIndex].ItemEnd;
        const std::uint64_t generation = m_Container.m_generation;

        bool anyRemeasured = false;
        for (std::size_t i = begin; i < end; ++i) {
            Node *child = m_Items[i]; // copy out: the recursive solve below may grow the arena
            const auto &dims = child->GetStyle().GetDimensions();

            // An explicit cross size is the author's number, not a measurement: nothing to re-ask.
            if ((m_IsRow ? dims.Height : dims.Width).Unit != CSSUnit::Auto) continue;

            const float mainSize = child->GetLayout().ComputedFlexBasis;
            const float measuredMain = m_IsRow
                                           ? child->GetLayout().ComputedWidth
                                           : child->GetLayout().ComputedHeight;
            if (std::isnan(mainSize) || mainSize == measuredMain) continue;
            // A collapsed main axis would wrap every item onto its own line. CSS floors a flex item
            // at its min-content size, which this engine does not compute, so the honest answer at
            // zero is the basis phase's: keep it, and overflow horizontally as before.
            if (mainSize <= 0.0f) continue;
            if (!child->CrossSizeDependsOnMainSize(generation)) continue;

            const float cross = child->MeasureCrossAtDefiniteMain(m_Ctx, m_IsRow, mainSize);
            if (std::isnan(cross)) continue;

            // Through the same clamp the basis phase applies, so the line's cross size, align-items
            // placement and the definite relayout keep seeing one number.
            const float clamped = ClampToCrossMinMax(child, cross);
            if (m_IsRow) child->GetLayout().ComputedHeight = clamped;
            else child->GetLayout().ComputedWidth = clamped;
            anyRemeasured = true;
        }

        if (!anyRemeasured) return;

        float lineCross = 0;
        for (std::size_t i = begin; i < end; ++i) {
            const auto &childLayout = m_Items[i]->GetLayout();
            const float itemCross = m_IsRow ? childLayout.ComputedHeight : childLayout.ComputedWidth;
            if (!std::isnan(itemCross)) lineCross = std::max(lineCross, itemCross);
        }
        m_Lines[lineIndex].CrossSize = lineCross;
        m_CrossNeedsReResolve = true;
    }

    /// Re-shrink-wrap an AUTO cross axis that RemeasureFlexedItems has just invalidated.
    ///
    /// ResolveContainerSize shrink-wraps the cross axis to m_MaxCrossSize before a single line exists,
    /// and therefore before any item's main size is final -- so a wrapping child that measured one
    /// line tall there and two lines tall after the distribution left the container frozen one line
    /// short. A multi-line container gets the same correction from ResolveAutoCrossSizeFromLines,
    /// which runs next and overrides this; a single line has no other owner.
    ///
    /// The max is recomputed over every item rather than folded in, because a re-measure moves an
    /// item's cross size in both directions and a running maximum cannot come back down.
    void ReShrinkWrapCrossSize() {
        if (!m_CrossNeedsReResolve || !m_CrossShrinkWrapped) return;

        float maxCross = 0;
        const std::size_t count = m_Items.Count();
        for (std::size_t i = 0; i < count; ++i) {
            const auto &childLayout = m_Items[i]->GetLayout();
            const float itemCross = m_IsRow ? childLayout.ComputedHeight : childLayout.ComputedWidth;
            if (!std::isnan(itemCross)) maxCross = std::max(maxCross, itemCross);
        }
        m_MaxCrossSize = maxCross;

        if (m_IsRow) {
            m_Layout.ComputedHeight = m_MaxCrossSize + m_Container.PaddingBorderVertical();
            m_AvailableHeight = m_MaxCrossSize;
        } else {
            m_Layout.ComputedWidth = m_MaxCrossSize + m_Container.PaddingBorderHorizontal();
            m_AvailableWidth = m_MaxCrossSize;
        }
    }

    /// Flexbox 9.4 step 15: a container whose cross size is AUTO takes the SUM of its flex lines'
    /// cross sizes, not the largest of them.
    ///
    /// MeasureItemBases accumulates `m_MaxCrossSize` as a running max and ResolveContainerSize
    /// shrink-wraps the AUTO cross axis to it. That is exactly right for one line -- which, as the
    /// comment in MeasureItemBases says, used to be the only shape this engine emitted -- and it is
    /// exactly one line too short for anything that wraps. Nothing detected the shortfall, because a
    /// flex line is positioned from its own running offset: the extra lines were laid out correctly
    /// and simply drawn past the bottom of a container that had never been told it needed the room.
    ///
    /// Runs after the line loop because that is the first moment the line count exists, and before
    /// AlignLinesOnCrossAxis because align-content divides the container's cross size against it.
    void ResolveAutoCrossSizeFromLines() {
        const std::size_t lineCount = m_Lines.Count();

        // One line is already what the shrink-wrap measured; a definite cross size is the author's
        // and overflow is then the correct outcome.
        if (lineCount < 2) return;

        const CSSValue &crossDim = m_IsRow ? m_Style.GetDimensions().Height
                                           : m_Style.GetDimensions().Width;
        if (crossDim.Unit != CSSUnit::Auto || m_Container.CrossSizeIsDefinite()) return;

        float total = 0;
        for (std::size_t li = 0; li < lineCount; ++li) total += m_Lines[li].CrossSize;
        total += static_cast<float>(lineCount - 1) * LineGap();

        const float pb = m_IsRow ? m_Container.PaddingBorderVertical()
                                 : m_Container.PaddingBorderHorizontal();

        // Through the same clamp every other cross size goes through, so min-height still floors the
        // result and max-height still caps it -- a capped container overflows, which is what a stated
        // maximum means.
        const float resolved = ClampToCrossMinMax(&m_Container, total + pb);

        if (m_IsRow) {
            m_Layout.ComputedHeight = resolved;
            m_AvailableHeight = resolved - pb;
        } else {
            m_Layout.ComputedWidth = resolved;
            m_AvailableWidth = resolved - pb;
        }
    }

    /// align-content across lines + per-item cross sizing (stretch), auto cross margins
    /// and align-items/align-self placement.
    void AlignLinesOnCrossAxis() {
        const std::size_t lineCount = m_Lines.Count();
        if (lineCount == 0) return;

        const float containerCrossSize = m_IsRow ? m_Layout.ComputedHeight : m_Layout.ComputedWidth;

        // This function is the authoritative writer of every item's cross position (it runs after,
        // and overwrites, the `crossPos` the line loop handed to PositionLineOnMainAxis), so the
        // border term belongs here too: paddingStart is the container's content-box origin on the
        // cross axis, and border insets it exactly like padding does. A percentage padding resolves
        // against the CONTAINING BLOCK's width on all four sides -- which is what Node::PaddingTop
        // and friends use, and is NOT this container's own ComputedWidth.
        const auto &border = m_Style.GetBorder();
        const float paddingStart = m_IsRow
                                       ? m_Container.PaddingTop() + border.WidthTop.Value
                                       : m_Container.PaddingLeft() + border.WidthLeft.Value;
        const float paddingEnd = m_IsRow
                                     ? m_Container.PaddingBottom() + border.WidthBottom.Value
                                     : m_Container.PaddingRight() + border.WidthRight.Value;

        // Gaps between lines are occupied space like the lines themselves: left out of this total,
        // align-content hands the gap's pixels out a second time as free space and the lines drift
        // apart by exactly the gap.
        const float lineGap = lineCount > 1 ? LineGap() : 0.0f;
        const float totalLineGap = static_cast<float>(lineCount - 1) * lineGap;

        float totalLineCrossSize = 0;
        for (std::size_t li = 0; li < lineCount; ++li)
            totalLineCrossSize += m_Lines[li].CrossSize;

        float extraSpace = std::max(
            0.0f, containerCrossSize - paddingStart - paddingEnd - totalLineCrossSize - totalLineGap);
        float crossOffset = paddingStart;
        float lineSpacing = 0;
        const bool isWrapReverse = m_Style.GetFlex().Wrap == FlexWrap::WrapReverse;

        if (lineCount == 1) {
            const float availableCross = containerCrossSize - paddingStart - paddingEnd;
            // A single line in a container with a DEFINITE cross size is the container's content
            // box: an item taller than it overflows, it does not enlarge the line (CSS Flexbox
            // single-line clamp). Without this a tall sibling stretches every align-stretch
            // sibling past the container, shoving Expanded-anchored trailing children off-surface
            // (the bad.png sidebar/footer). An AUTO cross axis still shrink-wraps to the tallest.
            const CSSValue& crossDim = m_IsRow ? m_Style.GetDimensions().Height
                                               : m_Style.GetDimensions().Width;
            const bool crossDefinite = crossDim.Unit != CSSUnit::Auto || m_Container.CrossSizeIsDefinite();
            if (std::isnan(m_Lines[0].CrossSize) || m_Lines[0].CrossSize < availableCross ||
                (crossDefinite && m_Lines[0].CrossSize > availableCross)) {
                m_Lines[0].CrossSize = availableCross;
                extraSpace = 0;
            }
        }

        switch (m_Style.GetFlex().ContentAlign) {
            case AlignContent::FlexCenter:
                crossOffset += extraSpace / 2.0f;
                break;
            case AlignContent::FlexEnd:
                crossOffset += extraSpace;
                break;
            case AlignContent::SpaceBetween:
                if (lineCount > 1)
                    lineSpacing = extraSpace / static_cast<float>(lineCount - 1);
                break;
            case AlignContent::SpaceAround:
                lineSpacing = extraSpace / static_cast<float>(lineCount);
                crossOffset += lineSpacing / 2.0f;
                break;
            case AlignContent::SpaceEvenly:
                lineSpacing = extraSpace / static_cast<float>(lineCount + 1);
                crossOffset += lineSpacing;
                break;
            case AlignContent::Stretch:
                if (lineCount > 1 && extraSpace > 0) {
                    const float add = extraSpace / static_cast<float>(lineCount);
                    for (std::size_t li = 0; li < lineCount; ++li)
                        m_Lines[li].CrossSize += add;
                }
                break;
            default:
                break;
        }

        for (std::size_t li = 0; li < lineCount; ++li) {
            const FlexLine &line = m_Lines[li];
            const float lineCrossStart = isWrapReverse
                                             ? (containerCrossSize - crossOffset - line.CrossSize)
                                             : crossOffset;

            for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i) {
                Node *child = m_Items[i];
                const auto &childStyle = child->GetStyle();
                const auto &dimension = childStyle.GetDimensions();
                const auto &margin = childStyle.GetMargin();
                auto &childLayout = child->GetLayout();
                float childCrossSize = m_IsRow ? childLayout.ComputedHeight : childLayout.ComputedWidth;

                AlignItems alignment = childStyle.GetFlex().AlignSelf != AlignItems::AutoAlign
                                           ? childStyle.GetFlex().AlignSelf
                                           : m_Style.GetFlex().Align;

                // Cross-axis margins resolved ONCE (read-only in this loop), reused by both the
                // stretch sizing and the placement below — row uses Top/Bottom, column uses
                // Left/Right. Auto resolves to 0; the auto-margin branch overrides these locals.
                // marginStart/marginEnd stay mutable for that override.
                //
                // Percentage margins always resolve against the container's WIDTH (CSS rule, all
                // four sides) -- for a row container that's Top/Bottom, so line.CrossSize (a
                // height-like quantity there) is the wrong reference; a column's cross edges are
                // Left/Right, where line.CrossSize already approximates width.
                const CSSValue &crossStartEdge = m_IsRow ? margin.Top : margin.Left;
                const CSSValue &crossEndEdge = m_IsRow ? margin.Bottom : margin.Right;
                const float crossMarginRef = m_IsRow ? m_Layout.ComputedWidth : line.CrossSize;
                float marginStart = crossStartEdge.ResolveValue(crossMarginRef);
                float marginEnd = crossEndEdge.ResolveValue(crossMarginRef);

                if (alignment == AlignItems::Stretch &&
                    (m_IsRow
                         ? dimension.Height.Unit == CSSUnit::Auto
                         : dimension.Width.Unit == CSSUnit::Auto)) {
                    // Flexbox 9.4 step 11: the stretched cross size is clamped by the item's own
                    // used min and max cross size. Nothing downstream re-applies them -- the item's
                    // ComputeDimensions skips the clamp for an AUTO size, there being no number to
                    // clamp yet -- so leaving it unclamped is how a max-height on a row's child (or a
                    // max-width on a column's) gets parsed, resolved and then never honoured. Clamped
                    // BEFORE the > 0 guard so a min-height still applies to a line that has no room.
                    const float stretchedSize =
                            ClampToCrossMinMax(child, line.CrossSize - (marginStart + marginEnd));
                    if (stretchedSize > 0) {
                        if (m_IsRow) childLayout.ComputedHeight = stretchedSize;
                        else childLayout.ComputedWidth = stretchedSize;
                        childCrossSize = stretchedSize;
                    }
                }

                const bool hasAutoStart = crossStartEdge.Unit == CSSUnit::Auto;
                const bool hasAutoEnd = crossEndEdge.Unit == CSSUnit::Auto;
                const int autoMarginCount = (hasAutoStart ? 1 : 0) + (hasAutoEnd ? 1 : 0);

                const float availableForAuto = line.CrossSize - childCrossSize - (marginStart + marginEnd);
                if (autoMarginCount > 0 && availableForAuto > 0) {
                    const float autoSize = availableForAuto / autoMarginCount;
                    if (hasAutoStart) marginStart = autoSize;
                    if (hasAutoEnd) marginEnd = autoSize;
                    alignment = AlignItems::FlexStart;
                } else {
                    if (hasAutoStart) marginStart = 0;
                    if (hasAutoEnd) marginEnd = 0;
                }

                float itemCrossPos = 0;
                switch (alignment) {
                    case AlignItems::FlexStart:
                        itemCrossPos = lineCrossStart + marginStart;
                        break;
                    case AlignItems::FlexEnd:
                        itemCrossPos = lineCrossStart + line.CrossSize - childCrossSize - marginEnd;
                        break;
                    case AlignItems::FlexCenter:
                        itemCrossPos = lineCrossStart + (line.CrossSize - childCrossSize) / 2.0f
                                       + (marginStart - marginEnd) / 2.0f;
                        break;
                    // NOT IMPLEMENTED: this engine has no text/baseline metrics to align against,
                    // so Baseline intentionally falls back to FlexStart rather than silently
                    // landing in `default` alongside Stretch. See README.md's Alignment section.
                    case AlignItems::Baseline:
                    default: // Stretch + fallback
                        itemCrossPos = lineCrossStart + marginStart;
                        break;
                }

                if (m_IsRow) childLayout.LocalY = itemCrossPos;
                else childLayout.LocalX = itemCrossPos;
            }

            crossOffset += line.CrossSize + lineSpacing;
            if (li + 1 < lineCount) crossOffset += lineGap;
        }
    }

    /// Re-lay-out each item at its now-definite border box. During the basis phase,
    /// AUTO-size items collapsed their subtrees to 0 (measured against NaN); this pass
    /// expands them at the resolved size.
    void RelayoutItemsAtDefiniteSize() {
        const std::size_t lineCount = m_Lines.Count();
        for (std::size_t li = 0; li < lineCount; ++li) {
            const FlexLine line = m_Lines[li]; // copy out: the recursive solve grows the arena
            for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i) {
                Node *child = m_Items[i];
                const auto &childLayout = child->GetLayout();
                // ResolveContainerSize may have moved this container's content box since the basis
                // phase stated the item's percentage reference, and LayoutContentsWithDefiniteSize
                // hands the item its OWN content width -- against which `10%` is a tenth of the
                // wrong box. Re-state the containing block before the definite solve.
                child->SetPercentBasis(m_AvailableWidth);
                child->LayoutContentsWithDefiniteSize(m_Ctx, childLayout.ComputedWidth,
                                                      childLayout.ComputedHeight);
            }
        }
    }

    Node &m_Container;
    LayoutContext &m_Ctx;
    Style &m_Style;
    ::Layout &m_Layout;
    const bool m_IsRow;
    const bool m_IsReverse;
    float m_AvailableWidth;
    float m_AvailableHeight;
    float m_TotalMainSize = 0;
    float m_MaxCrossSize = 0;
    /// ResolveContainerSize shrink-wrapped the cross axis to m_MaxCrossSize (AUTO cross dimension,
    /// NaN available cross), so that number is this solve's to correct.
    bool m_CrossShrinkWrapped = false;
    /// RemeasureFlexedItems moved at least one item's cross size after that shrink-wrap.
    bool m_CrossNeedsReResolve = false;
    ArenaSlice<Node *> m_Items;
    ArenaSlice<FlexLine> m_Lines;
    std::size_t m_NextItem = 0;
};

void FlexLayoutStrategy::Layout(Node &container, LayoutContext &ctx,
                                const float availableWidth, const float availableHeight) const {
    // The out-of-flow list must reflect exactly this run: the strategy can run more than
    // once per frame (basis + definite passes), and stale entries would be positioned twice.
    container.m_OutOfFlowChildren.clear();

    Solver(container, ctx, availableWidth, availableHeight).Run();
}
