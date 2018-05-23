#ifndef PHI_PREPROCESSOR_H
#define PHI_PREPROCESSOR_H

#include "Preprocess/Preprocessor.h"
#include "llvm/Transforms/Scalar.h"
#include <memory>

namespace ceagle {

class PhiPreprocessor : public Preprocessor {
public:
    virtual void preprocess(llvm::Module& module) override {
        std::unique_ptr<llvm::FunctionPass> pass;
        for (auto& f : module) {
            pass.reset(llvm::createDemoteRegisterToMemoryPass());
            pass->runOnFunction(f);
        }
    }
};

}

#endif
