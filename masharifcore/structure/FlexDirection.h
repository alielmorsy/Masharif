#pragma once
#include "../macros.h"

namespace
_NAMESPACE {
    ENUM_BEGIN(FlexDirection) {
        Column = 0,
        ColumnReverse = 1,
        Row = 2,
        RowReverse = 3
    } ENUM_END(FlexDirection);

    // String conversion for FlexDirection
    ENUM_TO_STRING(FlexDirection,
                   ENUM_CASE(FlexDirection::Column)
                   ENUM_CASE(FlexDirection::ColumnReverse)
                   ENUM_CASE(FlexDirection::Row)
                   ENUM_CASE(FlexDirection::RowReverse)
    )
    ENUM_BEGIN(FlexWrap) {
        NoWrap = 0,
        Wrap = 1,
        WrapReverse = 2
    } ENUM_END(Wrap);

    ENUM_TO_STRING(FlexWrap,
                   ENUM_CASE(FlexWrap::NoWrap)
                   ENUM_CASE(FlexWrap::Wrap)
                   ENUM_CASE(FlexWrap::WrapReverse)
    )
}
