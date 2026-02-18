#pragma once

#include "Align.h"
#include "CSSValue.h"
#include "FlexDirection.h"
#include "Justify.h"


namespace masharif {
    struct Gap {
        CSSValue row;
        CSSValue column;
    };

    struct CSSFlex {
        JustifyContent justifyContent = JustifyContent::FlexStart;
        AlignItems alignItems = AlignItems::Stretch;
        AlignContent alignContent = AlignContent::Stretch;
        AlignItems alignSelf = AlignItems::AUTO_ALIGN;
        CSSValue flexBasis;
        float flexGrow = 0.0f;
        float flexShrink = 1.0f;
        FlexDirection direction = FlexDirection::Row;
        FlexWrap wrap = FlexWrap::NoWrap;
        int order = 0;
        Gap gap;

        [[nodiscard]] bool isRow() const {
            return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
        }

        [[nodiscard]] bool isReverse() const {
            return direction == FlexDirection::RowReverse || direction == FlexDirection::ColumnReverse;
        }
    };
}
