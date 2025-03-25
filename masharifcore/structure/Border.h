#ifndef BORDER_H
#define BORDER_H
#include "Color.h"
#include "CSSValue.h"
#include "../macros.h"
ENUM_BEGIN(BorderStyle) {
    Solid = 0,
    Dashed,
    Dotted,
    Double
}ENUM_END(BorderStyle);

ENUM_TO_STRING(BorderStyle,
               ENUM_CASE(BorderStyle::Solid)
               ENUM_CASE(BorderStyle::Dashed)
               ENUM_CASE(BorderStyle::Dotted)
               ENUM_CASE(BorderStyle::Double)
)

namespace
NAMESPACE {
    struct BorderProperties {
        CSSValue widthTop{0};
        CSSValue widthBottom{0};
        CSSValue widthLeft{0};
        CSSValue widthHeight{0};
        Color color{0, 0, 0};


        BorderStyle style = BorderStyle::Solid;


        CSSValue topLeftRadius{0};
        CSSValue topRightRadius{0};
        CSSValue bottomLeftRadius{0};
        CSSValue bottomRightRadius{0};
    };
}
#endif //BORDER_H
