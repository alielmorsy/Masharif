#ifndef LAYOUT_H
#define LAYOUT_H
#include <memory>
#include <masharifcore/macros.h>

namespace
_NAMESPACE {


    struct Layout {
        float computedX = 0;
        float computedY = 0;
        float computedWidth, computedHeight;
        float computedFlexBasis;

    };
}
#endif
