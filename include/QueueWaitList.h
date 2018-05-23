#ifndef CEAGLE_QUEUE_WAIT_LIST_H
#define CEAGLE_QUEUE_WAIT_LIST_H

#include "WaitList.h"

#include <deque>

namespace ceagle {

template <typename item>
class QueueWaitList : public WaitList<item, std::deque<item>> {
    typedef std::deque<item> container;
    using WaitList<item, container>::data;
public:
    using typename WaitList<item, container>::equal_to;
    using WaitList<item, container>::WaitList;
    item chooseAndRemove() override {
        auto front = data.front();
        data.pop_front();
        return front;
    }
};

}

#endif //CEAGLE_QUEUEWAITLIST_H
