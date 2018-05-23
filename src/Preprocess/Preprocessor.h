#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "llvm/IR/Module.h"

using llvm::Module;

namespace ceagle {

class Preprocessor {
public:
    virtual void preprocess(Module& module) {
    }
    virtual ~Preprocessor() = default;
};

}

#endif
