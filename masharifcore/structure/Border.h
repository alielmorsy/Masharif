#pragma once

#include "CSSValue.h"

namespace masharif {
    struct BorderProperties {
        CSSValue WidthTop{0};
        CSSValue WidthBottom{0};
        CSSValue WidthLeft{0};
        CSSValue WidthRight{0};
    };
}
