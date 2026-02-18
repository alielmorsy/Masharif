#include "FlexLayoutStrategy.h"

#include "Node.h"
#include "masharifcore/structure/CSSValue.h"
#include "masharifcore/structure/Justify.h"
#include <algorithm>
#include <cmath>
#include <limits>
using namespace masharif;

static float calculatedNeededMargin(bool isRow, const MarginEdge &margin, float mainAxisSize) {
    float neededMargin = 0;
    if (isRow) {
        neededMargin = margin.left.resolveValue(mainAxisSize) + margin.right.resolveValue(mainAxisSize);
    } else {
        neededMargin = margin.top.resolveValue(mainAxisSize) + margin.bottom.resolveValue(mainAxisSize);
    }
    return neededMargin;
}

void FlexLayoutStrategy::layout(float availableWidth, float availableHeight) {
    auto &children = container->children;
    float totalMainAxisSize = 0;
    float totalCrossAxisSize = 0;

    DEF_NODE_LAYOUT(container);
    DEF_NODE_STYLE(container);

    const bool isRow = containerStyle.flex().isRow();
    const bool isReverse = containerStyle.flex().isReverse();

    // 1. Separate out-of-flow children and build in-flow list
    std::vector<std::shared_ptr<Node> > inFlowChildren;
    inFlowChildren.reserve(children.size());
    for (auto &child: children) {
        auto pos = child->style().dimensions().position;
        if (pos != PositionType::Static && pos != PositionType::Relative) {
            container->outOfFlowChildren.push_back(child);
        } else {
            inFlowChildren.push_back(child);
        }
    }

    // 2. Sort by CSS order property (stable to preserve DOM order for equal values)
    std::stable_sort(inFlowChildren.begin(), inFlowChildren.end(),
                     [](const std::shared_ptr<Node> &a, const std::shared_ptr<Node> &b) {
                         return a->style().flex().order < b->style().flex().order;
                     });

    // 3. Compute flex basis for each inflow child
    for (auto &child: inFlowChildren) {
        float childAvailableWidth = availableWidth;
        float childAvailableHeight = availableHeight;

        const auto &dim = child->style().dimensions(); // Access style directly
        if (isRow && dim.width.unit == CSSUnit::AUTO) childAvailableWidth = std::numeric_limits<float>::quiet_NaN();
        if (!isRow && dim.height.unit == CSSUnit::AUTO) childAvailableHeight = std::numeric_limits<float>::quiet_NaN();

        // Temporarily disable min/max constraints to get pure content size for flex basis
        auto &childDims = child->style().modify<Dimensions>();
        auto savedMinWidth = childDims.minWidth;
        auto savedMinHeight = childDims.minHeight;
        auto savedMaxWidth = childDims.maxWidth;
        auto savedMaxHeight = childDims.maxHeight;

        childDims.minWidth = {0};
        childDims.minHeight = {0};
        childDims.maxWidth = {};
        childDims.maxHeight = {};

        child->layoutImpl(childAvailableWidth, childAvailableHeight);

        // Restore constraints
        childDims.minWidth = savedMinWidth;
        childDims.minHeight = savedMinHeight;
        childDims.maxWidth = savedMaxWidth;
        childDims.maxHeight = savedMaxHeight;

        DEF_NODE_LAYOUT(child);
        DEF_NODE_STYLE(child);

        float basisReference = isRow ? availableWidth : availableHeight;
        if (childStyle.flex().flexBasis.unit == CSSUnit::AUTO) {
            float resolved = isRow ? childLayout.computedWidth : childLayout.computedHeight;
            childLayout.computedFlexBasis = std::isnan(resolved) ? 0 : resolved;
        } else {
            auto &p = childStyle.padding();
            auto &b = childStyle.border();
            float paddingBorder = isRow
                                      ? p.left.value + p.right.value + b.widthLeft.value + b.widthRight.value
                                      : p.top.value + p.bottom.value + b.widthTop.value + b.widthBottom.value;
            childLayout.computedFlexBasis = childStyle.flex().flexBasis.resolveValue(basisReference) + paddingBorder;
        }

        totalMainAxisSize += childLayout.computedFlexBasis;
        totalCrossAxisSize = std::max(totalCrossAxisSize,
                                      isRow ? childLayout.computedHeight : childLayout.computedWidth);
    }

    auto &p = containerStyle.padding();
    auto &b = containerStyle.border();
    float paddingBorderRow = p.left.value + p.right.value + b.widthLeft.value + b.widthRight.value;
    float paddingBorderCol = p.top.value + p.bottom.value + b.widthTop.value + b.widthBottom.value;

    if (std::isnan(availableWidth)) {
        if (containerStyle.dimensions().width.unit == CSSUnit::AUTO) {
            availableWidth = containerLayout.computedWidth =
                             (isRow ? totalMainAxisSize : totalCrossAxisSize) + paddingBorderRow;
        } else {
            availableWidth = containerLayout.computedWidth - paddingBorderRow;
        }
    }
    if (std::isnan(availableHeight)) {
        if (containerStyle.dimensions().height.unit == CSSUnit::AUTO) {
            availableHeight = containerLayout.computedHeight =
                              (!isRow ? totalMainAxisSize : totalCrossAxisSize) + paddingBorderCol;
        } else {
            availableHeight = containerLayout.computedHeight - paddingBorderCol;
        }
    }

    // 4. Build flex lines from inFlowChildren
    auto currentIterator = inFlowChildren.begin();
    auto end = inFlowChildren.end();
    size_t lineCount = 0;
    float availableMainAxisSize = isRow ? availableWidth : availableHeight;
    float crossPos = isRow
                         ? containerStyle.padding().top.resolveValue(availableWidth)
                         : containerStyle.padding().left.resolveValue(availableWidth);
    std::vector<FlexLine> lines;

    for (; currentIterator != end; lineCount++) {
        FlexLine line = calculateFlexLine(currentIterator, end, availableMainAxisSize, lineCount);
        lines.push_back(line);

        float containerMainSize = isRow ? containerLayout.computedWidth : containerLayout.computedHeight;
        auto &padding = containerStyle.padding();
        float paddingStart = isRow
                                 ? padding.left.resolveValue(containerMainSize)
                                 : padding.top.resolveValue(containerMainSize);
        float paddingEnd = isRow
                               ? padding.right.resolveValue(containerMainSize)
                               : padding.bottom.resolveValue(containerMainSize);
        float availableSpace = containerMainSize - paddingStart - paddingEnd;

        // 5. Resolve flexible lengths (multi-pass, handles min/max violations)
        resolveFlexibleLengths(line, availableSpace, isRow, availableWidth, availableHeight);

        // Recompute remaining space after flex resolution
        float takenAfterResolve = 0;
        for (auto &child: line.flexItems) {
            takenAfterResolve += child->layout().computedFlexBasis;
        }
        float originalRemainingSpace = availableSpace - line.takenSize;
        float remainingSpace = availableSpace - takenAfterResolve;

        // Handle auto margins consuming space
        if (remainingSpace > 0 && line.numberOfAutoMargin > 0) {
            // Auto margins will consume the space in the positioning loop below
        }

        float gap = isRow
                        ? containerStyle.flex().gap.column.resolveValue(availableWidth)
                        : containerStyle.flex().gap.row.resolveValue(availableHeight);
        float mainStartPos = paddingStart;
        if (isReverse) {
            mainStartPos = containerMainSize - paddingEnd;
        }

        // Justify-content positioning
        float mainOffset = 0;
        switch (containerStyle.flex().justifyContent) {
            case JustifyContent::FlexStart:
                mainOffset += 0;
                break;
            case JustifyContent::FlexEnd:
                mainOffset += remainingSpace;
                break;
            case JustifyContent::FlexCenter:
                mainOffset += remainingSpace / 2;
                break;
            case JustifyContent::SpaceBetween:
                if (line.flexItems.size() > 1) {
                    gap += remainingSpace / (line.flexItems.size() - 1);
                }
                break;
            case JustifyContent::SpaceAround:
                mainOffset += remainingSpace / (line.flexItems.size() * 2);
                gap += remainingSpace / line.flexItems.size();
                break;
            case JustifyContent::SpaceEvenly:
                mainOffset += remainingSpace / (line.flexItems.size() + 1);
                gap += remainingSpace / (line.flexItems.size() + 1);
                break;
            case JustifyContent::Stretch:
                break;
        }

        int autoMarginItems = line.numberOfAutoMargin;
        float autoRemainingSpace = originalRemainingSpace;
        float currentPosition = mainStartPos + (isReverse ? -mainOffset : mainOffset);

        for (auto child: line.flexItems) {
            DEF_NODE_LAYOUT(child);
            DEF_NODE_STYLE(child);

            float marginStart = 0, marginEnd = 0;
            bool hasAutoMargins = (isRow && (childStyle.margin().left.unit == CSSUnit::AUTO ||
                                             childStyle.margin().right.unit == CSSUnit::AUTO)) ||
                                  (!isRow && (childStyle.margin().top.unit == CSSUnit::AUTO ||
                                              childStyle.margin().bottom.unit == CSSUnit::AUTO));

            if (hasAutoMargins) {
                int autoCount = 0;
                if (isRow) {
                    if (childStyle.margin().left.unit == CSSUnit::AUTO) autoCount++;
                    if (childStyle.margin().right.unit == CSSUnit::AUTO) autoCount++;
                } else {
                    if (childStyle.margin().top.unit == CSSUnit::AUTO) autoCount++;
                    if (childStyle.margin().bottom.unit == CSSUnit::AUTO) autoCount++;
                }

                float itemSpace = autoMarginItems > 0 ? autoRemainingSpace / autoMarginItems : 0;
                marginStart = autoCount == 2 ? itemSpace : (autoCount == 1 ? itemSpace : 0);
                marginEnd = autoCount == 2 ? itemSpace : (autoCount == 1 ? itemSpace : 0);

                autoRemainingSpace -= itemSpace;
                autoMarginItems--;
            } else {
                marginStart = isRow
                                  ? childStyle.margin().left.resolveValue(availableWidth)
                                  : childStyle.margin().top.resolveValue(availableHeight);
                marginEnd = isRow
                                ? childStyle.margin().right.resolveValue(availableWidth)
                                : childStyle.margin().bottom.resolveValue(availableHeight);
            }

            if (isRow) {
                childLayout.computedY = crossPos;
                childLayout.computedX = isReverse
                                            ? currentPosition - childLayout.computedFlexBasis - marginEnd
                                            : currentPosition + marginStart;
                childLayout.computedWidth = childLayout.computedFlexBasis;
            } else {
                childLayout.computedX = crossPos;
                childLayout.computedY = isReverse
                                            ? currentPosition - childLayout.computedFlexBasis - marginEnd
                                            : currentPosition + marginStart;
                childLayout.computedHeight = childLayout.computedFlexBasis;
            }

            int reverseMultiply = (isReverse ? -1 : 1);
            currentPosition += reverseMultiply * (marginStart + childLayout.computedFlexBasis + marginEnd + gap);
        }


        crossPos += line.crossSize;
    }

    updateCrossSize(lines);
}


FlexLine FlexLayoutStrategy::calculateFlexLine(std::vector<std::shared_ptr<Node> >::iterator &iterator,
                                               std::vector<std::shared_ptr<Node> >::iterator &end,
                                               float totalMainAxisSize,
                                               size_t lineCount) {
    DEF_NODE_STYLE(container);
    DEF_NODE_LAYOUT(container);

    bool isRow = containerStyle.flex().isRow();
    bool isWrap = containerStyle.flex().wrap == FlexWrap::Wrap || containerStyle.flex().wrap == FlexWrap::WrapReverse;
    auto &gap = containerStyle.flex().gap;

    float gapSize = isRow
                        ? gap.column.resolveValue(containerLayout.computedWidth)
                        : gap.row.resolveValue(containerLayout.computedHeight);

    float takenSize = 0;

    bool foundFirst = false;
    float totalFlexGrow = 0, totalFlexShrinkScaledFactors = 0;
    int numberOfAutoMargin = 0;
    std::vector<Node *> items;
    float totalCrossSize = 0;

    for (; iterator != end; ++iterator) {
        auto child = iterator->get();


        DEF_NODE_STYLE(child);
        if (childStyle.dimensions().display == OuterDisplay::None) continue;
        DEF_NODE_LAYOUT(child);

        auto &margin = childStyle.margin();

        if (isRow) {
            if (margin.left.unit == CSSUnit::AUTO) numberOfAutoMargin++;
            if (margin.right.unit == CSSUnit::AUTO) numberOfAutoMargin++;
            totalCrossSize = std::max(totalCrossSize, childLayout.computedHeight);
        } else {
            totalCrossSize = std::max(totalCrossSize, childLayout.computedWidth);
            if (margin.top.unit == CSSUnit::AUTO) numberOfAutoMargin++;
            if (margin.bottom.unit == CSSUnit::AUTO) numberOfAutoMargin++;
        }

        float neededMargin = calculatedNeededMargin(isRow, margin, totalMainAxisSize);
        float newTaken = takenSize + neededMargin + childLayout.computedFlexBasis;

        if (foundFirst) {
            newTaken += gapSize;
        }

        if (newTaken > totalMainAxisSize && foundFirst && isWrap) {
            break;
        }

        foundFirst = true;
        items.push_back(child);
        totalFlexGrow += childStyle.flex().flexGrow;
        totalFlexShrinkScaledFactors += childStyle.flex().flexShrink * childLayout.computedFlexBasis;
        takenSize = newTaken;
    }

    return {
        std::move(items), // flexItems (std::vector<Node *>)
        numberOfAutoMargin,
        takenSize,
        totalFlexGrow,
        totalFlexShrinkScaledFactors,
        totalCrossSize
    };
}

// ── Multi-pass "Resolve Flexible Lengths" (W3C Flexbox §9.7) ──────────────
//
// The algorithm distributes free space proportionally and then checks each
// item against its min/max constraints. If an item violates a constraint it
// is "frozen" at the clamped size, and the loop restarts to redistribute the
// leftover among the remaining unfrozen items.
//
void FlexLayoutStrategy::resolveFlexibleLengths(FlexLine &line, float availableSpace, bool isRow,
                                                float availableWidth, float availableHeight) {
    if (line.flexItems.empty()) return;

    float remainingSpace = availableSpace - line.takenSize;

    // Nothing to grow/shrink, or auto-margins will consume the space
    if (line.numberOfAutoMargin > 0) return;

    bool isGrowing = remainingSpace > 0 && line.totalFlexGrow > 0;
    bool isShrinking = remainingSpace < 0 && line.totalFlexShrinkScaledFactors != 0;
    // We cannot return early here just because remainingSpace is 0, because
    // min/max constraints might still need to be enforced (e.g. max-width < flex-basis).
    // The loop infra handles the "do nothing" case via isGrowing/isShrinking flags.
    // if (!isGrowing && !isShrinking) return;

    // Track which items are frozen (true = frozen, false = still flexible)
    std::vector<bool> frozen(line.flexItems.size(), false);

    // Store the original (hypothetical) flex basis so we can reset between passes
    std::vector<float> baseSizes(line.flexItems.size());
    for (size_t i = 0; i < line.flexItems.size(); i++) {
        baseSizes[i] = line.flexItems[i]->layout().computedFlexBasis;
    }

    // Freeze zero-flex-factor items per W3C §9.7
    for (size_t i = 0; i < line.flexItems.size(); i++) {
        auto &s = line.flexItems[i]->style().flex();
        if ((isGrowing && s.flexGrow == 0) || (isShrinking && s.flexShrink == 0)) {
            frozen[i] = true;
        }
    }

    // Iterative loop – re-runs until no new violations
    for (;;) {
        // 1. Compute totals for unfrozen items only
        float unfrozenFlexGrow = 0;
        float unfrozenScaledShrink = 0;
        float frozenSize = 0;

        for (size_t i = 0; i < line.flexItems.size(); i++) {
            if (frozen[i]) {
                frozenSize += line.flexItems[i]->layout().computedFlexBasis;
            } else {
                auto &s = line.flexItems[i]->style().flex();
                unfrozenFlexGrow += s.flexGrow;
                unfrozenScaledShrink += s.flexShrink * baseSizes[i];
            }
        }

        float freeSpace = availableSpace - frozenSize;
        for (size_t i = 0; i < line.flexItems.size(); i++) {
            if (!frozen[i]) freeSpace -= baseSizes[i];
        }

        DEF_NODE_STYLE(container);
        DEF_NODE_LAYOUT(container);
        auto &gap = containerStyle.flex().gap;
        float gapSize = isRow
                            ? gap.column.resolveValue(containerLayout.computedWidth)
                            : gap.row.resolveValue(containerLayout.computedHeight);

        if (line.flexItems.size() > 1) {
            freeSpace -= (line.flexItems.size() - 1) * gapSize;
        }

        // 2. Distribute free space to unfrozen items
        for (size_t i = 0; i < line.flexItems.size(); i++) {
            if (frozen[i]) continue;
            auto *child = line.flexItems[i];
            auto &childStyle = child->style();

            float newSize = baseSizes[i];
            if (isGrowing && unfrozenFlexGrow > 0) {
                float ratio = childStyle.flex().flexGrow / unfrozenFlexGrow;
                newSize = baseSizes[i] + ratio * freeSpace;
            } else if (isShrinking && unfrozenScaledShrink != 0) {
                float scaledFactor = childStyle.flex().flexShrink * baseSizes[i];
                float ratio = scaledFactor / unfrozenScaledShrink;
                newSize = baseSizes[i] + ratio * freeSpace; // freeSpace is negative
            }

            child->layout().computedFlexBasis = newSize;
        }

        // 3. Check for min/max violations, freeze violators
        bool anyNewFreezes = false;

        for (size_t i = 0; i < line.flexItems.size(); i++) {
            if (frozen[i]) continue;
            auto *child = line.flexItems[i];
            auto &dims = child->style().dimensions();
            float current = child->layout().computedFlexBasis;
            float clamped = current;

            if (isRow) {
                if (dims.maxWidth.unit != CSSUnit::AUTO) {
                    clamped = std::min(clamped, dims.maxWidth.resolveValue(availableWidth));
                }
                if (dims.minWidth.unit != CSSUnit::AUTO) {
                    clamped = std::max(clamped, dims.minWidth.resolveValue(availableWidth));
                }
            } else {
                if (dims.maxHeight.unit != CSSUnit::AUTO) {
                    clamped = std::min(clamped, dims.maxHeight.resolveValue(availableHeight));
                }
                if (dims.minHeight.unit != CSSUnit::AUTO) {
                    clamped = std::max(clamped, dims.minHeight.resolveValue(availableHeight));
                }
            }

            if (clamped != current) {
                child->layout().computedFlexBasis = clamped;
                frozen[i] = true;
                anyNewFreezes = true;
            }
        }

        // 4. If no violations occurred, freeze everything and exit
        if (!anyNewFreezes) break;

        // Otherwise loop again to redistribute among the remaining unfrozen items
    }
}

void FlexLayoutStrategy::updateCrossSize(std::vector<FlexLine> &lines) {
    if (lines.empty()) return;

    DEF_NODE_STYLE(container);
    DEF_NODE_LAYOUT(container);
    const bool isRow = containerStyle.flex().isRow();
    float containerCrossSize = isRow ? containerLayout.computedHeight : containerLayout.computedWidth;

    float width = containerLayout.computedWidth;
    float paddingStart = isRow
                             ? containerStyle.padding().top.resolveValue(width)
                             : containerStyle.padding().left.resolveValue(width);
    float paddingEnd = isRow
                           ? containerStyle.padding().bottom.resolveValue(width)
                           : containerStyle.padding().right.resolveValue(width);

    float totalLineCrossSize = 0;
    bool isReverse = containerStyle.flex().wrap == FlexWrap::WrapReverse;
    for (const auto &line: lines) {
        totalLineCrossSize += line.crossSize;
    }

    float extraSpace = std::max(0.0f, containerCrossSize - paddingStart - paddingEnd - totalLineCrossSize);
    float crossOffset = paddingStart;
    float lineSpacing = 0;

    // If there is only one line (e.g. nowrap, or just happens to fit), 
    // and the container has a definite cross size, that single line should fill the container
    // so that align-items: stretch works against the full container height.
    if (lines.size() == 1) {
        float availableCross = containerCrossSize - paddingStart - paddingEnd;
        if (std::isnan(lines[0].crossSize) || lines[0].crossSize < availableCross) {
            lines[0].crossSize = availableCross;
            extraSpace = 0; // consumed by the line
        }
    }

    switch (containerStyle.flex().alignContent) {
        case AlignContent::FlexCenter:
            crossOffset += extraSpace / 2;
            break;
        case AlignContent::FlexEnd:
            crossOffset += extraSpace;
            break;
        case AlignContent::SpaceBetween:
            if (lines.size() > 1) {
                lineSpacing = extraSpace / (lines.size() - 1);
            }
            break;
        case AlignContent::SpaceAround:
            if (!lines.empty()) {
                lineSpacing = extraSpace / lines.size();
                crossOffset += lineSpacing / 2;
            }
            break;
        case AlignContent::SpaceEvenly:
            if (!lines.empty()) {
                lineSpacing = extraSpace / (lines.size() + 1);
                crossOffset += lineSpacing;
            }
            break;
        case AlignContent::Stretch:
            // Only distribute space if we have multiple lines (checked implicitly or expclitly in spec).
            // Single line case handled above or by default fill if wrap is involved? 
            // Actually, if we have multiple lines and stretch, they grow.
            if (lines.size() > 1 && extraSpace > 0) {
                float additionalSize = extraSpace / lines.size();
                for (auto &line: lines) {
                    line.crossSize += additionalSize;
                }
            }
            break;
        default:
            break;
    }

    for (auto &line: lines) {
        // Calculate the line's cross start position considering wrap reverse
        float lineCrossStart = isReverse ? (containerCrossSize - crossOffset - line.crossSize) : crossOffset;

        for (auto child: line.flexItems) {
            DEF_NODE_STYLE(child);
            auto &dimension = childStyle.dimensions();
            if (dimension.display == OuterDisplay::None) continue;
            if (dimension.position != PositionType::Static &&
                dimension.position != PositionType::Relative) {
                continue;
            }

            DEF_NODE_LAYOUT(child);
            float childCrossSize = isRow ? childLayout.computedHeight : childLayout.computedWidth;

            AlignItems alignment = (childStyle.flex().alignSelf != AlignItems::AUTO_ALIGN)
                                       ? childStyle.flex().alignSelf
                                       : containerStyle.flex().alignItems;

            if (alignment == AlignItems::Stretch &&
                (isRow
                     ? dimension.height.unit == CSSUnit::AUTO
                     : dimension.width.unit == CSSUnit::AUTO)) {
                float marginsCross = isRow
                                         ? (childStyle.margin().top.resolveValue(line.crossSize) +
                                            childStyle.margin().bottom.resolveValue(line.crossSize))
                                         : (childStyle.margin().left.resolveValue(line.crossSize) +
                                            childStyle.margin().right.resolveValue(line.crossSize));

                float stretchedSize = line.crossSize - marginsCross;
                if (stretchedSize > 0) {
                    if (isRow) {
                        childLayout.computedHeight = stretchedSize;
                    } else {
                        childLayout.computedWidth = stretchedSize;
                    }
                    childCrossSize = stretchedSize;
                }
            }

            float marginStart = isRow
                                    ? childStyle.margin().top.resolveValue(line.crossSize)
                                    : childStyle.margin().left.resolveValue(line.crossSize);
            float marginEnd = isRow
                                  ? childStyle.margin().bottom.resolveValue(line.crossSize)
                                  : childStyle.margin().right.resolveValue(line.crossSize);

            bool hasAutoMarginStart = isRow
                                          ? childStyle.margin().top.unit == CSSUnit::AUTO
                                          : childStyle.margin().left.unit == CSSUnit::AUTO;
            bool hasAutoMarginEnd = isRow
                                        ? childStyle.margin().bottom.unit == CSSUnit::AUTO
                                        : childStyle.margin().right.unit == CSSUnit::AUTO;
            int autoMarginCount = (hasAutoMarginStart ? 1 : 0) + (hasAutoMarginEnd ? 1 : 0);

            float availableSpace = line.crossSize - childCrossSize - (marginStart + marginEnd);

            if (autoMarginCount > 0 && availableSpace > 0) {
                float autoMarginSize = availableSpace / autoMarginCount;
                if (hasAutoMarginStart) marginStart = autoMarginSize;
                if (hasAutoMarginEnd) marginEnd = autoMarginSize;
                alignment = AlignItems::FlexStart;
            } else {
                if (hasAutoMarginStart) marginStart = 0;
                if (hasAutoMarginEnd) marginEnd = 0;
            }

            float itemCrossPos = 0;
            switch (alignment) {
                case AlignItems::FlexStart:
                    itemCrossPos = lineCrossStart + marginStart;
                    break;
                case AlignItems::FlexEnd:
                    itemCrossPos = lineCrossStart + line.crossSize - childCrossSize - marginEnd;
                    break;
                case AlignItems::FlexCenter:
                    itemCrossPos = lineCrossStart + (line.crossSize - childCrossSize) / 2 + (marginStart - marginEnd) /
                                   2;
                    break;
                case AlignItems::Stretch:
                    itemCrossPos = lineCrossStart + marginStart;
                    break;
                default:
                    itemCrossPos = lineCrossStart + marginStart;
                    break;
            }

            if (isRow) {
                childLayout.computedY = itemCrossPos;
            } else {
                childLayout.computedX = itemCrossPos;
            }
        }

        crossOffset += line.crossSize + lineSpacing;
    }
}
