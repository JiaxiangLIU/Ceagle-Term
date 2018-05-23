#ifndef CEAGLE_ABSREF_ABSTRACTSTATE_H
#define CEAGLE_ABSREF_ABSTRACTSTATE_H

#include <memory>
#include <vector>

namespace ceagle {

class AbstractState {
protected:
    llvm::BasicBlock* location;
public:
    AbstractState(llvm::BasicBlock * block) {
        location = block;
    }
    virtual ~AbstractState() = default;
    virtual std::vector<std::shared_ptr<AbstractState>> next() {
        // calculate abstract
        return {};
    }
    llvm::BasicBlock* getLocation() const {
        return location;
    }
};

}

#endif
