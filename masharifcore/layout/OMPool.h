#ifndef OMPOOL_H
#define OMPOOL_H
#include <masharifcore/macros.h>
#include "om.h"

namespace
NAMESPACE {
    union OMPoolNode {
        LowLevelNode low;
        HighLevelNode high;
        OMPoolNode *next;

        OMPoolNode() {
        }
    };

    class OMPool {
    private:
        std::vector<OMPoolNode *> chunks;
        OMPoolNode *free_list = nullptr;
        size_t chunk_size = 1024;

        void allocate_chunk();

    public:
        OMPool() { allocate_chunk(); }
        ~OMPool() { for (auto chunk: chunks) delete[] chunk; }

        template<typename T>
        T *allocate();

        void deallocate(void *ptr);
    };
}
#endif //OMPOOL_H
