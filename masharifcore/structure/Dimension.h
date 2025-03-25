//
// Created by Ali Elmorsy on 3/25/2025.
//

#ifndef DIMENSION_H
#define DIMENSION_H
#include "CSSValue.h"
#include <masharifcore/macros.h>

namespace
NAMESPACE {
    struct Dimensions {
        OuterDisplay display = OuterDisplay::Block;
        CSSValue width;
        CSSValue height;
        CSSValue minWidth;
        CSSValue minHeight;
        CSSValue maxWidth;
        CSSValue maxHeight;
        CSSValue top, right, bottom, left;
        PositionType position = PositionType::Static;
    };
}

#endif //DIMENSION_H
