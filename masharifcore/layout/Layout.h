#ifndef LAYOUT_H
#define LAYOUT_H
#include <memory>
#include <vector>
#include <masharifcore/macros.h>

namespace
NAMESPACE {


    struct Layout {
        float computedX = NAN;
        float computedY = NAN;
        float computedWidth, computedHeight;
        float computedFlexBasis;

    };
}
#endif
