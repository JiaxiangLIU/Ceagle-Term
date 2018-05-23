#include "DFS/Absref/PredicateAbstractor.h"
#include "DFS/Absref/AbstractReachedSet.h"

bool ceagle::AbstractReachedSet::stop(const std::shared_ptr<AbstractState> &state) const {
    for (auto& reach: this->getRelated(state)) {
        if (reach->cover(*state)) {
            return true;
        }
    }
    return false;
}

using namespace llvm;

namespace ceagle {

AbstractReachedSet::AbstractReachedSet(std::shared_ptr<AbstractState> start) : _start(start) {
    locationMap[start->location()] = {start};
}
void AbstractReachedSet::add(const std::shared_ptr<AbstractState>& previous, const std::shared_ptr<AbstractState>& state) {
    parentMap[state] = previous;
    auto& v = locationMap[state->location()];
    v.push_back(state);
}
std::vector<std::shared_ptr<AbstractState>> AbstractReachedSet::getPath(std::shared_ptr<AbstractState> end) {
    std::vector<std::shared_ptr<AbstractState>> path;
    while(end != _start) {
        path.push_back(end);
        end = parentMap[end];
    }
    path.push_back(end);
    return std::vector<std::shared_ptr<AbstractState>>(path.rbegin(), path.rend());
}
std::vector<std::shared_ptr<AbstractState>> AbstractReachedSet::getRelated(const std::shared_ptr<AbstractState>& state) const {
    try {
        return locationMap.at(state->location());
    } catch (std::out_of_range) {
        return {};
    }

}

std::map<BasicBlock*, Expr> AbstractReachedSet::getInvariants(Precision* p) {
    decltype(getInvariants(nullptr)) result;
    for (auto pa: locationMap) {
        result[pa.first] = BoolValue(false);
        for (auto state: pa.second) {
            Expr stateExpr = BoolValue(true);
            for (auto index: state->getPrecision()) {
                if (index > 0) {
                    stateExpr = stateExpr && p->get(index);
                } else if (index < 0) {
                    stateExpr = stateExpr && !p->get(index);
                } else {
                    assert(false);
                }
            }
            result[pa.first] = result[pa.first] || stateExpr;
        }
    }
    return result;
}

void AbstractReachedSet::targetHook(const std::shared_ptr<AbstractState>& state) {
    errorPath = this->getPath(state);
}

}
