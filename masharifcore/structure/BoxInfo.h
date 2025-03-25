#pragma once
#include "../macros.h"

namespace
_NAMESPACE {
    ENUM_BEGIN(OuterDisplay) { None, Block, Inline, InlineBlock, Flex, InlineFlex } ENUM_END(OuterDisplay);

    ENUM_BEGIN(InnerLayout) { NormalFlow, Flex, Grid } ENUM_END(InnerLayout);

    ENUM_BEGIN(PositionType) { Static, Relative, Absolute, Fixed, Sticky } ENUM_END(PositionType);

    ENUM_TO_STRING(OuterDisplay,
                   ENUM_CASE(OuterDisplay::None)
                   ENUM_CASE(OuterDisplay::Block)
                   ENUM_CASE(OuterDisplay::Inline)
                   ENUM_CASE(OuterDisplay::InlineBlock)
                   ENUM_CASE(OuterDisplay::Flex)
                   ENUM_CASE(OuterDisplay::InlineFlex)
    );
    ENUM_TO_STRING(PositionType,
                   ENUM_CASE(PositionType::Static)
                   ENUM_CASE(PositionType::Relative)
                   ENUM_CASE(PositionType::Absolute)
                   ENUM_CASE(PositionType::Fixed)
                   ENUM_CASE(PositionType::Sticky)
    );
}
