//
// Created by Ali Elmorsy on 3/24/2025.
//

#ifndef FLEX_H
#define FLEX_H
#include "Align.h"
#include "CSSValue.h"
#include "FlexDirection.h"
#include "Justify.h"


namespace
_NAMESPACE {
    struct Gap {
        CSSValue row;
        CSSValue column;
    };

    struct CSSFlex {
        JustifyContent justifyContent = JustifyContent::FlexStart;
        AlignItems alignItems = AlignItems::FlexStart;
        AlignContent alignContent = AlignContent::Stretch;
        AlignItems alignSelf = AlignItems::AUTO_ALIGN;
        CSSValue flexBasis;
        float flexGrow = 0.0f;
        float flexShrink = 1.0f;
        FlexDirection direction = FlexDirection::Row;
        FlexWrap wrap = FlexWrap::NoWrap;
        Gap gap;

        [[nodiscard]] bool isRow() const {
            return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
        }

        [[nodiscard]] bool isReverse() const {
            return direction == FlexDirection::RowReverse || direction == FlexDirection::ColumnReverse;
        }


    };
}

#endif //FLEX_H
