#ifndef BORDER_H
#define BORDER_H
#include "../../../../Amara/utils/css/Color.h"
#include "CSSValue.h"
#include "../macros.h"


namespace
_NAMESPACE {
    struct BorderProperties {
        CSSValue widthTop{0};
        CSSValue widthBottom{0};
        CSSValue widthLeft{0};
        CSSValue widthRight{0};




        CSSValue topLeftRadius{0};
        CSSValue topRightRadius{0};
        CSSValue bottomLeftRadius{0};
        CSSValue bottomRightRadius{0};
    };
}
#endif //BORDER_H
