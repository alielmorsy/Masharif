#pragma once
#include "../macros.h"
namespace masharif {
    ENUM_BEGIN(AlignContent) {
        Stretch = -1,
        FlexStart = 0,
        FlexCenter = 1,
        FlexEnd = 2,
        SpaceBetween = 3,
        SpaceAround = 4,
        SpaceEvenly = 5
    } ENUM_END(AlignContent);


    ENUM_BEGIN(AlignItems) {
        AUTO_ALIGN = -2,
        Stretch = -1,
        FlexStart = 0,
        FlexCenter = 1,
        FlexEnd = 2,
        Baseline = 3
    } ENUM_END(AlignItems);


    ENUM_TO_STRING(AlignContent,
                   ENUM_CASE(AlignContent::Stretch)
                   ENUM_CASE(AlignContent::FlexStart)
                   ENUM_CASE(AlignContent::FlexCenter)
                   ENUM_CASE(AlignContent::FlexEnd)
                   ENUM_CASE(AlignContent::SpaceBetween)
                   ENUM_CASE(AlignContent::SpaceAround)
                   ENUM_CASE(AlignContent::SpaceEvenly)
    )

    // String conversion for AlignItems
    ENUM_TO_STRING(AlignItems,
                   ENUM_CASE(AlignItems::Stretch)
                   ENUM_CASE(AlignItems::FlexStart)
                   ENUM_CASE(AlignItems::FlexCenter)
                   ENUM_CASE(AlignItems::FlexEnd)
                   ENUM_CASE(AlignItems::Baseline)
    )
}