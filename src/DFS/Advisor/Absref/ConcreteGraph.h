#ifndef CEAGLE_ABSREF_CONCRETEGRAPH_H
#define CEAGLE_ABSREF_CONCRETEGRAPH_H

namespace ceagle {

template<class NodeTy>
class ConcreteGraph {
public:
    virtual NodeTy& entry() const = 0;
    virtual NodeTy& forward() = 0;
    virtual NodeTy& backward() = 0;
    virtual typename NodeTy::ilist& path() = 0;
    virtual ~ConcreteGraph() = default;
};

}

#endif
