#include "FlexLayoutStrategy.h"

#include "LayoutContext.h"
#include "Node.h"
#include "masharifcore/structure/CSSValue.h"
#include "masharifcore/structure/Justify.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>

using namespace masharif;

namespace {
    float NeededMainAxisMargin(const bool isRow, const MarginEdge &margin, const float mainAxisSize) {
        if (isRow)
            return margin.Left.ResolveValue(mainAxisSize) + margin.Right.ResolveValue(mainAxisSize);
        return margin.Top.ResolveValue(mainAxisSize) + margin.Bottom.ResolveValue(mainAxisSize);
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
        float crossPos = m_IsRow
                             ? m_Style.GetPadding().Top.ResolveValue(m_AvailableWidth)
                             : m_Style.GetPadding().Left.ResolveValue(m_AvailableWidth);

        // Invariant across the line loop: the container's main size and paddings do not
        // change while lines are built and placed.
        const float containerMainSize = m_IsRow ? m_Layout.ComputedWidth : m_Layout.ComputedHeight;
        const auto &padding = m_Style.GetPadding();
        const float padStart = m_IsRow
                                   ? padding.Left.ResolveValue(containerMainSize)
                                   : padding.Top.ResolveValue(containerMainSize);
        const float padEnd = m_IsRow
                                 ? padding.Right.ResolveValue(containerMainSize)
                                 : padding.Bottom.ResolveValue(containerMainSize);
        const float availableSpace = containerMainSize - padStart - padEnd;

        while (m_NextItem < m_Items.Count()) {
            m_Lines.Append(BuildLine(availableMainAxisSize));
            FlexLine &line = m_Lines[m_Lines.Count() - 1];
            ResolveFlexibleLengths(line, availableSpace);
            PositionLineOnMainAxis(line, availableSpace, containerMainSize, padStart, padEnd, crossPos);
            crossPos += line.CrossSize;
        }

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
            const auto pos = child->GetStyle().GetDimensions().Position;
            const auto display = child->GetStyle().GetDimensions().Display;
            if (pos != PositionType::Static && pos != PositionType::Relative) {
                m_Container.m_OutOfFlowChildren.push_back(child.get());
            } else if (display != OuterDisplay::None) {
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
        const auto &pad = m_Style.GetPadding();
        const auto &bor = m_Style.GetBorder();
        const auto &dim = m_Style.GetDimensions();
        const bool widthExplicit = dim.Width.Unit == CSSUnit::Px || dim.Width.Unit == CSSUnit::Percent;
        const bool heightExplicit = dim.Height.Unit == CSSUnit::Px || dim.Height.Unit == CSSUnit::Percent;
        if (widthExplicit && !std::isnan(m_Layout.ComputedWidth)) {
            m_AvailableWidth = m_Layout.ComputedWidth
                               - pad.Left.Value - pad.Right.Value
                               - bor.WidthLeft.Value - bor.WidthRight.Value;
        }
        if (heightExplicit && !std::isnan(m_Layout.ComputedHeight)) {
            m_AvailableHeight = m_Layout.ComputedHeight
                                - pad.Top.Value - pad.Bottom.Value
                                - bor.WidthTop.Value - bor.WidthBottom.Value;
        }
    }

    /// Compute each item's flex basis. Min/max constraints are suppressed here
    /// (ignoreMinMax) because the flex algorithm applies them in ResolveFlexibleLengths.
    void MeasureItemBases() {
        const std::size_t count = m_Items.Count();
        for (std::size_t i = 0; i < count; ++i) {
            Node *child = m_Items[i]; // copy out: the recursive solve below may grow the arena

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
                const auto &pad = childStyle.GetPadding();
                const auto &[wTop, wBottom, wLeft, wRight] = childStyle.GetBorder();
                const float pb = m_IsRow
                                     ? pad.Left.Value + pad.Right.Value + wLeft.Value + wRight.Value
                                     : pad.Top.Value + pad.Bottom.Value + wTop.Value + wBottom.Value;
                childLayout.ComputedFlexBasis = childStyle.GetFlex().FlexBasis.ResolveValue(basisRef) + pb;
            }

            // Main-axis margins occupy main-axis space alongside the basis (BuildLine counts them
            // when packing a line), so they belong in the content total: a shrink-to-fit (AUTO) main
            // axis must enclose its children's MARGIN boxes, not just their border boxes.
            m_TotalMainSize += childLayout.ComputedFlexBasis
                               + NeededMainAxisMargin(m_IsRow, childStyle.GetMargin(), basisRef);
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

    /// Resolve NaN available space and shrink-wrap an AUTO main axis to content
    /// (unless the parent has already fixed this node's size — MainSizeIsDefinite).
    void ResolveContainerSize() {
        const auto &p = m_Style.GetPadding();
        const auto &b = m_Style.GetBorder();
        const float pbRow = p.Left.Value + p.Right.Value + b.WidthLeft.Value + b.WidthRight.Value;
        const float pbCol = p.Top.Value + p.Bottom.Value + b.WidthTop.Value + b.WidthBottom.Value;

        if (std::isnan(m_AvailableWidth)) {
            if (m_Style.GetDimensions().Width.Unit == CSSUnit::Auto)
                m_Layout.ComputedWidth = (m_IsRow ? m_TotalMainSize : m_MaxCrossSize) + pbRow;
            m_AvailableWidth = m_Layout.ComputedWidth - pbRow;
        } else if (m_IsRow && m_Style.GetDimensions().Width.Unit == CSSUnit::Auto
                   && !m_Container.MainSizeIsDefinite()) {
            m_Layout.ComputedWidth = m_TotalMainSize + pbRow;
            m_AvailableWidth = m_Layout.ComputedWidth - pbRow;
        } else if (std::isnan(m_Layout.ComputedWidth)) {
            m_Layout.ComputedWidth = m_AvailableWidth;
        }

        if (std::isnan(m_AvailableHeight)) {
            if (m_Style.GetDimensions().Height.Unit == CSSUnit::Auto)
                m_Layout.ComputedHeight = (!m_IsRow ? m_TotalMainSize : m_MaxCrossSize) + pbCol;
            m_AvailableHeight = m_Layout.ComputedHeight - pbCol;
        } else if (!m_IsRow && m_Style.GetDimensions().Height.Unit == CSSUnit::Auto
                   && !m_Container.MainSizeIsDefinite()) {
            m_Layout.ComputedHeight = m_TotalMainSize + pbCol;
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
        if (line.NumberOfAutoMargin > 0) return;

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
        // on style + availableSpace, never on the basis/frozen state the loop mutates), so sum
        // them ONCE here instead of re-resolving every item on every iteration. Reference is
        // availableSpace, matching the former per-iteration call.
        float fixedMainMargin = 0;
        for (std::size_t i = 0; i < n; i++) {
            Node *child = m_Items[line.ItemBegin + i];
            baseSizes[i] = child->GetLayout().ComputedFlexBasis;
            fixedMainMargin += NeededMainAxisMargin(m_IsRow, child->GetStyle().GetMargin(), availableSpace);
            const auto &flex = child->GetStyle().GetFlex();
            frozen[i] = static_cast<std::uint8_t>(
                (isGrowing && flex.FlexGrow == 0) || (isShrinking && flex.FlexShrink == 0) ? 1 : 0);
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

        float takenAfterResolve = 0;
        for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i)
            takenAfterResolve += m_Items[i]->GetLayout().ComputedFlexBasis;

        const float originalRemainingSpace = availableSpace - line.TakenSize;
        const float remainingSpace = availableSpace - takenAfterResolve;

        float gap = m_IsRow
                        ? m_Style.GetFlex().Gaps.Column.ResolveValue(m_AvailableWidth)
                        : m_Style.GetFlex().Gaps.Row.ResolveValue(m_AvailableHeight);

        const float mainStartPos = m_IsReverse ? (containerMainSize - padEnd) : padStart;

        float mainOffset = 0;
        switch (m_Style.GetFlex().Justify) {
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

        int autoMarginItems = line.NumberOfAutoMargin;
        float autoRemainingSpace = originalRemainingSpace;
        float currentPosition = mainStartPos + (m_IsReverse ? -mainOffset : mainOffset);

        for (std::size_t i = line.ItemBegin; i < line.ItemEnd; ++i) {
            Node *child = m_Items[i];
            auto &childLayout = child->GetLayout();
            const auto &childStyle = child->GetStyle();

            const bool hasAutoMargins =
                    (m_IsRow && (childStyle.GetMargin().Left.Unit == CSSUnit::Auto ||
                                 childStyle.GetMargin().Right.Unit == CSSUnit::Auto)) ||
                    (!m_IsRow && (childStyle.GetMargin().Top.Unit == CSSUnit::Auto ||
                                  childStyle.GetMargin().Bottom.Unit == CSSUnit::Auto));

            float marginStart = 0, marginEnd = 0;
            if (hasAutoMargins) {
                const float itemSpace = autoMarginItems > 0 ? autoRemainingSpace / autoMarginItems : 0.0f;
                marginStart = itemSpace;
                marginEnd = itemSpace;
                autoRemainingSpace -= itemSpace;
                autoMarginItems--;
            } else {
                marginStart = m_IsRow
                                  ? childStyle.GetMargin().Left.ResolveValue(m_AvailableWidth)
                                  : childStyle.GetMargin().Top.ResolveValue(m_AvailableHeight);
                marginEnd = m_IsRow
                                ? childStyle.GetMargin().Right.ResolveValue(m_AvailableWidth)
                                : childStyle.GetMargin().Bottom.ResolveValue(m_AvailableHeight);
            }

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

    /// align-content across lines + per-item cross sizing (stretch), auto cross margins
    /// and align-items/align-self placement.
    void AlignLinesOnCrossAxis() {
        const std::size_t lineCount = m_Lines.Count();
        if (lineCount == 0) return;

        const float containerCrossSize = m_IsRow ? m_Layout.ComputedHeight : m_Layout.ComputedWidth;
        const float width = m_Layout.ComputedWidth;

        const float paddingStart = m_IsRow
                                       ? m_Style.GetPadding().Top.ResolveValue(width)
                                       : m_Style.GetPadding().Left.ResolveValue(width);
        const float paddingEnd = m_IsRow
                                     ? m_Style.GetPadding().Bottom.ResolveValue(width)
                                     : m_Style.GetPadding().Right.ResolveValue(width);

        float totalLineCrossSize = 0;
        for (std::size_t li = 0; li < lineCount; ++li)
            totalLineCrossSize += m_Lines[li].CrossSize;

        float extraSpace = std::max(0.0f, containerCrossSize - paddingStart - paddingEnd - totalLineCrossSize);
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

                // Cross-axis margins resolved ONCE against line.CrossSize (read-only in this
                // loop), reused by both the stretch sizing and the placement below — row uses
                // Top/Bottom, column uses Left/Right. Auto resolves to 0; the auto-margin branch
                // overrides these locals. marginStart/marginEnd stay mutable for that override.
                const CSSValue &crossStartEdge = m_IsRow ? margin.Top : margin.Left;
                const CSSValue &crossEndEdge = m_IsRow ? margin.Bottom : margin.Right;
                float marginStart = crossStartEdge.ResolveValue(line.CrossSize);
                float marginEnd = crossEndEdge.ResolveValue(line.CrossSize);

                if (alignment == AlignItems::Stretch &&
                    (m_IsRow
                         ? dimension.Height.Unit == CSSUnit::Auto
                         : dimension.Width.Unit == CSSUnit::Auto)) {
                    const float stretchedSize = line.CrossSize - (marginStart + marginEnd);
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
                    default: // Stretch + fallback
                        itemCrossPos = lineCrossStart + marginStart;
                        break;
                }

                if (m_IsRow) childLayout.LocalY = itemCrossPos;
                else childLayout.LocalX = itemCrossPos;
            }

            crossOffset += line.CrossSize + lineSpacing;
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
