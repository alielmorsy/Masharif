#pragma once

#include "Align.h"
#include "CSSValue.h"
#include "FlexDirection.h"
#include "Justify.h"


namespace masharif {
    struct Gap {
        CSSValue Row;
        CSSValue Column;
    };

    struct CSSFlex {
        /// justify-content (main axis). Named Justify because the member would otherwise
        /// shadow its own enum type.
        JustifyContent Justify = JustifyContent::FlexStart;
        /// align-items (cross axis); Align for the same shadowing reason.
        AlignItems Align = AlignItems::Stretch;
        /// align-content (line packing); ContentAlign for the same shadowing reason.
        AlignContent ContentAlign = AlignContent::Stretch;
        AlignItems AlignSelf = AlignItems::AutoAlign;
        CSSValue FlexBasis;
        float FlexGrow = 0.0f;
        float FlexShrink = 1.0f;
        FlexDirection Direction = FlexDirection::Row;
        FlexWrap Wrap = FlexWrap::NoWrap;
        int Order = 0;
        /// row-gap / column-gap; Gaps because the member would shadow the Gap type.
        Gap Gaps;

        [[nodiscard]] constexpr bool IsRow() const {
            return Direction == FlexDirection::Row || Direction == FlexDirection::RowReverse;
        }

        [[nodiscard]] constexpr bool IsReverse() const {
            return Direction == FlexDirection::RowReverse || Direction == FlexDirection::ColumnReverse;
        }
    };
}
