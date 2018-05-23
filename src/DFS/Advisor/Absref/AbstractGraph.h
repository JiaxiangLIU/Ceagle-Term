#ifndef CEAGLE_ABSREF_ABSTRACTGRAPH_H
#define CEAGLE_ABSREF_ABSTRACTGRAPH_H

#include "DFS/Advisor/Absref/AbstractState.h"

namespace ceagle {

// Abstract Graph
// i.e. the Abstract Reachable Set
class AbstractGraph {
public:
    virtual ~AbstractGraph() = default;
    virtual void init(std::shared_ptr<AbstractState> root) {
    }
    virtual void add(std::shared_ptr<AbstractState> state) {
    }
};

}

#endif
