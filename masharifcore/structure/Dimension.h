#pragma once

#include "CSSValue.h"
#include "BoxInfo.h"

namespace masharif {
    struct Dimensions {
        OuterDisplay display = OuterDisplay::Block;
        CSSValue width;
        CSSValue height;
        CSSValue minWidth{0};
        CSSValue minHeight{0};
        CSSValue maxWidth;
        CSSValue maxHeight;
        CSSValue top{0}, right{0}, bottom{0}, left{0};
        PositionType position = PositionType::Static;
    };
}
