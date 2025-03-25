#include "OMPool.h"
using namespace NAMESPACE;

void OMPool::allocate_chunk() {
    OMPoolNode *chunk = new OMPoolNode[chunk_size];
    chunks.push_back(chunk);
    for (size_t i = 0; i < chunk_size - 1; ++i) {
        chunk[i].next = &chunk[i + 1];
    }
    chunk[chunk_size - 1].next = free_list;
    free_list = chunk;
}

template<typename T>
T * OMPool::allocate() {
    if (!free_list) allocate_chunk();
    OMPoolNode *node = free_list;
    free_list = free_list->next;
    return reinterpret_cast<T *>(node);
}

void OMPool::deallocate(void *ptr)  {
    if (!ptr) return;
    auto node = static_cast<OMPoolNode *>(ptr);
    node->next = free_list;
    free_list = node;
}