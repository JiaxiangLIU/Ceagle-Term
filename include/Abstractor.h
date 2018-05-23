#ifndef CEAGLE_ABSTRACTOR_H
#define CEAGLE_ABSTRACTOR_H

#include <vector>

namespace ceagle {

template <class item>
class Abstractor {
public:
    virtual std::vector<item> next(const item&) = 0;
};

}

#endif //CEAGLE_ABSTRACTOR_H
