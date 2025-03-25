#pragma once
#include "../macros.h"
ENUM_BEGIN(JustifyContent) {
    Stretch = -1,
    FlexStart = 0,
    FlexCenter = 1,
    FlexEnd = 2,
    SpaceBetween = 3,
    SpaceAround = 4,
    SpaceEvenly = 5
} ENUM_END(JustifyContent);


ENUM_TO_STRING(JustifyContent,
               ENUM_CASE(JustifyContent::Stretch)
               ENUM_CASE(JustifyContent::FlexStart)
               ENUM_CASE(JustifyContent::FlexCenter)
               ENUM_CASE(JustifyContent::FlexEnd)
               ENUM_CASE(JustifyContent::SpaceBetween)
               ENUM_CASE(JustifyContent::SpaceAround)
               ENUM_CASE(JustifyContent::SpaceEvenly)
)
