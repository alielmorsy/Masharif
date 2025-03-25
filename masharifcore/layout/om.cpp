#include "om.h"
using namespace NAMESPACE;

bool masharif::operator<(const LowLevelNode &a, const LowLevelNode &b) {
    if (a.parent == b.parent) return a.label < b.label;
    return a.parent->label < b.parent->label;
}

LowLevelNode *create_om_after(OMPool &pool, LowLevelNode *after) {
    LowLevelNode *new_node = pool.allocate<LowLevelNode>();
    new_node->label = after->label + 1; // Simple label assignment
    new_node->prev = after;
    new_node->next = after->next;
    if (after->next) after->next->prev = new_node;
    after->next = new_node;
    new_node->parent = after->parent;
    //TODO:
    //new_node->data = nullptr;
    return new_node;
}
