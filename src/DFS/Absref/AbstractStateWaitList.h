#ifndef CEAGLE_ABSREF_WAIT_LIST_H
#define CEAGLE_ABSREF_WAIT_LIST_H

#include "DFS/Absref/AbstractState.h"
#include "VectorWaitList.h"

#include <vector>
#include <memory>

namespace ceagle {

class AbstractState;

class AbstractStateWaitList : public VectorWaitList<std::shared_ptr<AbstractState>> {
    using VectorWaitList::VectorWaitList;
};

}

#endif
