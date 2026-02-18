#ifndef OFFSETS_H
#define OFFSETS_H
#include "CSSValue.h"

namespace masharif {
    struct Edge {
        CSSValue left{0, CSSUnit::PX};
        CSSValue top{0, CSSUnit::PX};
        CSSValue bottom{0, CSSUnit::PX};
        CSSValue right{0, CSSUnit::PX};
    };
}
#endif
