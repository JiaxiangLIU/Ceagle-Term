#ifndef CEAGLE_VECTOR_WAIT_LIST_H
#define CEAGLE_VECTOR_WAIT_LIST_H

#include <vector>
#include <functional>

#include "WaitList.h"

namespace ceagle {

template <class item>
class VectorWaitList : public WaitList<item, std::vector<item>> {
    typedef std::vector<item> container;
    using WaitList<item, container>::data;
public:
    using typename WaitList<item, container>::equal_to;
    using WaitList<item, container>::WaitList;
    item chooseAndRemove() override {
        auto back = data.back();
        data.pop_back();
        return back;
    }
}; // class VectorWaitList

} // namespace ceagle

#endif //CEAGLE_VECTORWAITLIST_H
