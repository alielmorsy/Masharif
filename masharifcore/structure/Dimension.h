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
        CSSValue Top{0}, Right{0}, Bottom{0}, Left{0};
        PositionType Position = PositionType::Static;
    };
}
