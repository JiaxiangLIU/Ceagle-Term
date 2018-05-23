#ifndef CEAGLE_WAIT_LIST_H
#define CEAGLE_WAIT_LIST_H

#include <vector>

namespace ceagle {

template <typename item, class container>
class WaitList {
protected:
    container data;
public:
    WaitList() : data() {}
    WaitList(item one) : data{one} {}
    struct equal_to {
        constexpr bool operator()(const item& item1, const item& item2) const {
            return item1 == item2;
        }
    };
    virtual bool empty() const {
        return data.empty();
    }
    virtual item chooseAndRemove() = 0;
    virtual void add(item state) {
        auto equal = equal_to();
        for (auto &d: data) {
            if (equal(state, d)) return;
        }
        data.push_back(state);
    }
};

}

#endif //SV_CEAGLE_WAITLIST_H
