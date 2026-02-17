//
// Created by Ali Elmorsy on 3/25/2025.
//

#ifndef DIMENSION_H
#define DIMENSION_H
#include "CSSValue.h"
#include <masharifcore/macros.h>

namespace
_NAMESPACE {
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

#endif //DIMENSION_H
