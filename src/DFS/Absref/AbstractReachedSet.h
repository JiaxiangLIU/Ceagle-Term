#ifndef CEAGLE_ABSREF_ABSTRACT_REACHED_SET_H
#define CEAGLE_ABSREF_ABSTRACT_REACHED_SET_H

#include <map>
#include "DFS/Absref/PredicateAbstractor.h"
#include "DFS/Absref/AbstractState.h"
#include "DFS/Absref/Precision.h"
#include "DFS/Absref/AbstractStateWaitList.h"
#include "ARG.h"
#include "DFS/Absref/AbstractReachedSet.h"

#include <functional>
#include <memory>

namespace ceagle {

class AbstractState;
class PredicateAbstractor;
class AbstractStateWaitList;

class AbstractReachedSet : public ARG<PredicateAbstractor, AbstractStateWaitList, std::shared_ptr<AbstractState>> {
protected:
    std::map<std::shared_ptr<AbstractState>, std::shared_ptr<AbstractState>> parentMap;
    std::map<llvm::BasicBlock*, std::vector<std::shared_ptr<AbstractState>>> locationMap;
    std::shared_ptr<AbstractState> _start;
    virtual void targetHook(const std::shared_ptr<AbstractState>&) override ;
public:
    AbstractReachedSet() {}
    AbstractReachedSet(std::shared_ptr<AbstractState> start);
    virtual void add(const std::shared_ptr<AbstractState>& previous, const std::shared_ptr<AbstractState>& state) override ;
    std::vector<std::shared_ptr<AbstractState>> getPath(std::shared_ptr<AbstractState> end);
    std::vector<std::shared_ptr<AbstractState>> getRelated(const std::shared_ptr<AbstractState>& state) const;
    std::map<llvm::BasicBlock*, Expr> getInvariants(Precision* p);

    std::vector<std::shared_ptr<AbstractState>> errorPath;

    virtual bool stop(const std::shared_ptr<AbstractState> &state) const override ;
};

}

#endif
