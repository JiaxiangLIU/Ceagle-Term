#include "VectorWaitList.h"
#include "DFS/Absref/AbstractState.h"

namespace ceagle {

template <>
struct VectorWaitList<std::shared_ptr<AbstractState>>::equal_to {
    bool operator()(const std::shared_ptr<AbstractState> item1, const std::shared_ptr<AbstractState> item2) {
        if (item1 == nullptr || item2 == nullptr) return true;
        return *item1 == *item2;
    }
};

} // end namespace ceagle
