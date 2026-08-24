#pragma once

#include "CSSValue.h"
#include "BoxInfo.h"

namespace masharif {
    struct Dimensions {
        OuterDisplay Display = OuterDisplay::Block;
        CSSValue Width;
        CSSValue Height;
        CSSValue MinWidth{0};
        CSSValue MinHeight{0};
        CSSValue MaxWidth;
        CSSValue MaxHeight;
        // CSS initial value for all four inset properties is `auto` (not `0`): a caller that
        // sets only `Right`/`Bottom` must leave `Left`/`Top` reading as unset, or the
        // has-this-side checks in Node::PositionOutOfFlowChild silently pick the untouched
        // side instead of the one actually anchored.
        CSSValue Top, Right, Bottom, Left;
        PositionType Position = PositionType::Static;
    };
}
