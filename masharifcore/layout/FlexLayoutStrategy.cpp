//
// Created by Ali Elmorsy on 3/25/2025.
//

#include "FlexLayoutStrategy.h"

#include "Node.h"
#include "masharifcore/structure/CSSValue.h"
#include "masharifcore/structure/Justify.h"
using namespace _NAMESPACE;

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

    for (auto &child: children) {
        child->layoutImpl(availableWidth, availableHeight);

        DEF_NODE_LAYOUT(child);
        DEF_NODE_STYLE(child);

        float basisReference = isRow ? availableWidth : availableHeight;
        if (childStyle.flex().flexBasis.unit == CSSUnit::AUTO) {
            childLayout.computedFlexBasis = isRow ? childLayout.computedWidth : childLayout.computedHeight;
        } else {
            childLayout.computedFlexBasis = childStyle.flex().flexBasis.resolveValue(basisReference);
        }

        totalMainAxisSize += childLayout.computedFlexBasis;
        totalCrossAxisSize = std::max(totalCrossAxisSize,
                                      isRow ? childLayout.computedHeight : childLayout.computedWidth);
    }

    if (std::isnan(availableWidth)) {
        availableWidth = containerLayout.computedWidth = isRow ? totalMainAxisSize : totalCrossAxisSize;
    }
    if (std::isnan(availableHeight)) {
        availableHeight = containerLayout.computedHeight = !isRow ? totalMainAxisSize : totalCrossAxisSize;
    }

    auto currentIterator = children.begin();
    auto end = children.end();
    size_t lineCount = 0;
    float availableMainAxisSize = isRow ? availableWidth : availableHeight;
    float crossPos = 0;
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
        float originalRemainingSpace = availableSpace - line.takenSize;
        float remainingSpace = originalRemainingSpace;

        // Flex grow logic
        if (remainingSpace > 0 && line.numberOfAutoMargin == 0 && line.totalFlexGrow > 0) {
            float takenAfterGrowing = 0;
            for (auto &child: line.flexItems) {
                DEF_NODE_LAYOUT(child);
                DEF_NODE_STYLE(child);

                float growAmount = (childStyle.flex().flexGrow / line.totalFlexGrow) * remainingSpace;
                float newSize = childLayout.computedFlexBasis + growAmount;

                if (isRow && childStyle.dimensions().maxWidth.unit != CSSUnit::AUTO) {
                    newSize = std::min(newSize, childStyle.dimensions().maxWidth.resolveValue(availableWidth));
                }

                childLayout.computedFlexBasis = newSize;
                takenAfterGrowing += newSize;
            }
            remainingSpace = availableSpace - takenAfterGrowing;
        } else if (remainingSpace > 0 && line.numberOfAutoMargin > 0) {
            remainingSpace = 0; // Preserve for auto margins
        }

        // Flex shrink logic
        if (remainingSpace < 0 && line.totalFlexShrinkScaledFactors != 0) {
            float takenAfterShrink = 0;
            float shrinkSpace = -remainingSpace;
            for (auto &child: line.flexItems) {
                DEF_NODE_LAYOUT(child);
                DEF_NODE_STYLE(child);

                float shrinkAmount = (childStyle.flex().flexShrink * childLayout.computedFlexBasis / line.
                                      totalFlexShrinkScaledFactors) * shrinkSpace;
                float newSize = childLayout.computedFlexBasis - shrinkAmount;

                if (isRow && childStyle.dimensions().minWidth.unit != CSSUnit::AUTO) {
                    newSize = std::max(newSize, childStyle.dimensions().minWidth.resolveValue(availableWidth));
                }

                childLayout.computedFlexBasis = newSize;
                takenAfterShrink += newSize;
            }
            remainingSpace = availableSpace - takenAfterShrink;
        }

        float gap = isRow
                        ? containerStyle.flex().gap.row.resolveValue(availableWidth)
                        : containerStyle.flex().gap.column.resolveValue(availableHeight);
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
            } else {
                childLayout.computedX = crossPos;
                childLayout.computedY = isReverse
                                            ? currentPosition - childLayout.computedFlexBasis - marginEnd
                                            : currentPosition + marginStart;
            }

            int reverseMultiply = (isReverse ? -1 : 1);
            currentPosition += reverseMultiply * (marginStart + childLayout.computedFlexBasis + marginEnd + gap);
        }


        crossPos += line.crossSize;
    }

    updateCrossSize(lines);

    // for (auto &child: children) {
    //     child->updateStackingContext();
    // }
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
                        ? gap.row.resolveValue(containerLayout.computedWidth)
                        : gap.column.resolveValue(containerLayout.computedHeight);

    float takenSize = 0;

    if (isRow) {
        takenSize += containerStyle.padding().left.resolveValue(totalMainAxisSize) +
                containerStyle.padding().right.resolveValue(totalMainAxisSize);
    } else {
        takenSize += containerStyle.padding().top.resolveValue(totalMainAxisSize) +
                containerStyle.padding().bottom.resolveValue(totalMainAxisSize);
    }

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
        .flexItems = std::move(items),
        .numberOfAutoMargin = numberOfAutoMargin,
        .takenSize = takenSize,
        .totalFlexGrow = totalFlexGrow,
        .totalFlexShrinkScaledFactors = totalFlexShrinkScaledFactors,
        .crossSize = totalCrossSize
    };
}

// void FlexLayoutStrategy::updateCrossSize(std::vector<FlexLine> &lines) {
//     if (lines.empty()) return;
//
//     DEF_NODE_STYLE(container); // Access container style
//     DEF_NODE_LAYOUT(container); // Access container style
//     const bool isRow = containerStyle.flex().isRow();
//     float containerCrossSize = isRow ? containerLayout.computedHeight : containerLayout.computedWidth;
//
//     float totalLineCrossSize = 0;
//     bool isReverse = containerStyle.flex().wrap == FlexWrap::WrapReverse;
//     for (const auto &line: lines) {
//         totalLineCrossSize += line.crossSize;
//     }
//
//     float extraSpace = std::max(0.0f, containerCrossSize - totalLineCrossSize);
//     float crossOffset = 0;
//     float lineSpacing = 0;
//
//     switch (containerStyle.flex().alignContent) {
//         case AlignContent::FlexCenter:
//             crossOffset = extraSpace / 2;
//             break;
//         case AlignContent::FlexEnd:
//             crossOffset = extraSpace;
//             break;
//         case AlignContent::SpaceBetween:
//             if (lines.size() > 1) {
//                 lineSpacing = extraSpace / (lines.size() - 1);
//             }
//             break;
//         case AlignContent::SpaceAround:
//             if (!lines.empty()) {
//                 lineSpacing = extraSpace / lines.size();
//                 crossOffset = lineSpacing / 2;
//             }
//             break;
//         case AlignContent::SpaceEvenly:
//             if (!lines.empty()) {
//                 lineSpacing = extraSpace / (lines.size() + 1);
//                 crossOffset = lineSpacing;
//             }
//             break;
//         case AlignContent::Stretch:
//             if (!lines.empty() && extraSpace > 0) {
//                 float additionalSize = extraSpace / lines.size();
//                 for (auto &line: lines) {
//                     line.crossSize += additionalSize;
//                 }
//             }
//             break;
//         default:
//             break;
//     }
//
//     for (auto &line: lines) {
//         for (auto child: line.flexItems) {
//             DEF_NODE_STYLE(child); // Access child's style
//             auto &dimension = childStyle.dimensions();
//             if (dimension.display == OuterDisplay::None) continue;
//             if (dimension.position != PositionType::Static &&
//                 dimension.position != PositionType::Relative) {
//                 continue;
//             }
//
//
//             DEF_NODE_LAYOUT(child); // Access child's layout
//
//             float childCrossSize = isRow ? childLayout.computedHeight : childLayout.computedWidth;
//
//             AlignItems alignment = (childStyle.flex().alignSelf != AlignItems::AUTO_ALIGN)
//                                        ? childStyle.flex().alignSelf
//                                        : containerStyle.flex().alignItems;
//
//             if (alignment == AlignItems::Stretch &&
//                 (isRow
//                      ? dimension.height.unit == CSSUnit::AUTO
//                      : dimension.width.unit == CSSUnit::AUTO)) {
//                 float marginsCross = isRow
//                                          ? (childStyle.margin().top.resolveValue(line.crossSize) +
//                                             childStyle.margin().bottom.resolveValue(line.crossSize))
//                                          : (childStyle.margin().left.resolveValue(line.crossSize) +
//                                             childStyle.margin().right.resolveValue(line.crossSize));
//
//                 float stretchedSize = line.crossSize - marginsCross;
//                 if (stretchedSize > 0) {
//                     if (isRow) {
//                         childLayout.computedHeight = stretchedSize;
//                     } else {
//                         childLayout.computedWidth = stretchedSize;
//                     }
//                     childCrossSize = stretchedSize;
//                 }
//             }
//
//             float marginStart = isRow
//                                     ? childStyle.margin().top.resolveValue(line.crossSize)
//                                     : childStyle.margin().left.resolveValue(line.crossSize);
//             float marginEnd = isRow
//                                   ? childStyle.margin().bottom.resolveValue(line.crossSize)
//                                   : childStyle.margin().right.resolveValue(line.crossSize);
//
//             bool hasAutoMarginStart = isRow
//                                           ? childStyle.margin().top.unit == CSSUnit::AUTO
//                                           : childStyle.margin().left.unit == CSSUnit::AUTO;
//             bool hasAutoMarginEnd = isRow
//                                         ? childStyle.margin().bottom.unit == CSSUnit::AUTO
//                                         : childStyle.margin().right.unit == CSSUnit::AUTO;
//             int autoMarginCount = (hasAutoMarginStart ? 1 : 0) + (hasAutoMarginEnd ? 1 : 0);
//
//             float availableSpace = line.crossSize - childCrossSize - (marginStart + marginEnd);
//
//             if (autoMarginCount > 0 && availableSpace > 0) {
//                 float autoMarginSize = availableSpace / autoMarginCount;
//                 if (hasAutoMarginStart) marginStart = autoMarginSize;
//                 if (hasAutoMarginEnd) marginEnd = autoMarginSize;
//                 alignment = AlignItems::FlexStart;
//             } else {
//                 if (hasAutoMarginStart) marginStart = 0;
//                 if (hasAutoMarginEnd) marginEnd = 0;
//             }
//
//             float itemCrossPos = 0;
//             switch (alignment) {
//                 case AlignItems::FlexStart:
//                     itemCrossPos = crossOffset + marginStart;
//                     break;
//                 case AlignItems::FlexEnd:
//                     itemCrossPos = crossOffset + line.crossSize - childCrossSize - marginEnd;
//                     break;
//                 case AlignItems::FlexCenter:
//                     itemCrossPos = crossOffset + (line.crossSize - childCrossSize) / 2 + (marginStart - marginEnd) / 2;
//                     break;
//                 case AlignItems::Stretch:
//                     itemCrossPos = crossOffset + marginStart;
//                     break;
//                 default:
//                     itemCrossPos = crossOffset + marginStart;
//                     break;
//             }
//
//             if (isRow) {
//                 childLayout.computedY = itemCrossPos;
//             } else {
//                 childLayout.computedX = itemCrossPos;
//             }
//         }
//
//         crossOffset += line.crossSize + lineSpacing;
//     }
// }

void FlexLayoutStrategy::updateCrossSize(std::vector<FlexLine> &lines) {
    if (lines.empty()) return;

    DEF_NODE_STYLE(container);
    DEF_NODE_LAYOUT(container);
    const bool isRow = containerStyle.flex().isRow();
    float containerCrossSize = isRow ? containerLayout.computedHeight : containerLayout.computedWidth;

    float totalLineCrossSize = 0;
    bool isReverse = containerStyle.flex().wrap == FlexWrap::WrapReverse;
    for (const auto &line : lines) {
        totalLineCrossSize += line.crossSize;
    }

    float extraSpace = std::max(0.0f, containerCrossSize - totalLineCrossSize);
    float crossOffset = 0;
    float lineSpacing = 0;

    switch (containerStyle.flex().alignContent) {
        case AlignContent::FlexCenter:
            crossOffset = extraSpace / 2;
            break;
        case AlignContent::FlexEnd:
            crossOffset = extraSpace;
            break;
        case AlignContent::SpaceBetween:
            if (lines.size() > 1) {
                lineSpacing = extraSpace / (lines.size() - 1);
            }
            break;
        case AlignContent::SpaceAround:
            if (!lines.empty()) {
                lineSpacing = extraSpace / lines.size();
                crossOffset = lineSpacing / 2;
            }
            break;
        case AlignContent::SpaceEvenly:
            if (!lines.empty()) {
                lineSpacing = extraSpace / (lines.size() + 1);
                crossOffset = lineSpacing;
            }
            break;
        case AlignContent::Stretch:
            if (!lines.empty() && extraSpace > 0) {
                float additionalSize = extraSpace / lines.size();
                for (auto &line : lines) {
                    line.crossSize += additionalSize;
                }
            }
            break;
        default:
            break;
    }

    for (auto &line : lines) {
        // Calculate the line's cross start position considering wrap reverse
        float lineCrossStart = isReverse ? (containerCrossSize - crossOffset - line.crossSize) : crossOffset;

        for (auto child : line.flexItems) {
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
                    itemCrossPos = lineCrossStart + (line.crossSize - childCrossSize) / 2 + (marginStart - marginEnd) / 2;
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