#ifndef OM_H
#define OM_H
#include <cstdint>
#include <vector>

#include "OMPool.h"
#include "masharifcore/macros.h"

namespace
NAMESPACE {
    struct HighLevelNode;

    //Created from the spinless paper
    struct LowLevelNode {
        uint32_t label;
        LowLevelNode *prev;
        LowLevelNode *next;
        HighLevelNode *parent;

        LowLevelNode(const uint32_t label, LowLevelNode *p, LowLevelNode *n, HighLevelNode *par)
            : label(label), prev(p), next(n), parent(par) {
        }
    };

    struct HighLevelNode {
        uint32_t label;
        HighLevelNode *prev;
        HighLevelNode *next;
        std::vector<LowLevelNode *> low_level_list;

        HighLevelNode(uint32_t lbl, HighLevelNode *p, HighLevelNode *n)
            : label(lbl), prev(p), next(n) {
        }
    };

    inline bool operator<(const LowLevelNode &a, const LowLevelNode &b);
    LowLevelNode* create_om_after(OMPool& pool, LowLevelNode* after);
}
#endif
