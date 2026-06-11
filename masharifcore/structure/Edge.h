#pragma once

#include "CSSValue.h"

namespace masharif {
    struct Edge {
        CSSValue Left{0, CSSUnit::Px};
        CSSValue Top{0, CSSUnit::Px};
        CSSValue Bottom{0, CSSUnit::Px};
        CSSValue Right{0, CSSUnit::Px};
    };
}
