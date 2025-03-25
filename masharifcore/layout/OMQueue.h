#ifndef OMQUEUE_H
#define OMQUEUE_H
#include <masharifcore/macros.h>

#include "om.h"

namespace
NAMESPACE {
    enum class PQType { PRE, POST, INIT };

    struct PQEntry {
        struct LayoutNode *node;
        PQType type;
        LowLevelNode *om;

        PQEntry(LayoutNode *n, PQType t, LowLevelNode *o) : node(n), type(t), om(o) {
        }
    };

    struct PQComparator {
        bool operator()(const PQEntry &a, const PQEntry &b) const {
            return *(b.om) < *(a.om); // Min-heap: reverse for the smallest OM first
        }
    };
}
#endif //OMQUEUE_H
