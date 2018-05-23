#ifndef CEAGLE_ARG_VISITOR_H
#define CEAGLE_ARG_VISITOR_H

#include <llvm/IR/Module.h>

#include <functional>
#include <memory>

namespace ceagle {

template <class Abstractor, class WaitList, class ARGNode>
class ARG {
protected:
    virtual void targetHook(const ARGNode&) {};
public:
    typedef std::function<bool(const ARGNode&)> TargetMatcher;
    virtual void expand(WaitList& waitList, Abstractor& abstractor, TargetMatcher isTarget) {
        while (!waitList.empty()) {
            auto currentState = waitList.chooseAndRemove();
            for (auto& nextState : abstractor.next(currentState)) {
                if (!stop(nextState)) {
                    waitList.add(nextState);
                    this->add(currentState, nextState);
                }
                if (isTarget(nextState)) {
                    targetHook(nextState);
                    return;
                }
            }
        }
        waitList = WaitList();
        return;
    }
    virtual bool stop(const ARGNode& state) const = 0;
    virtual void add(const ARGNode& from, const ARGNode& to) = 0;
}; // class ARG

} // namespace ceagle

#endif
