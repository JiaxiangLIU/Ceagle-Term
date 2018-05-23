#ifndef CEAGLE_CORRECTWITNESS_H
#define CEAGLE_CORRECTWITNESS_H

#include "Witness.h"

#include <llvm/IR/BasicBlock.h>

using llvm::BasicBlock;

namespace ceagle {

    class CorrectnessWitness : public Witness {
    public:
        CorrectnessWitness(string srcFile, const map<string, string>& taskInfo) : Witness(srcFile, taskInfo) {
            this->taskInfo["witness-type"] = "correctness_witness";
        }
        CorrectnessWitness(string srcFile) : CorrectnessWitness(srcFile, map<string, string>()) {}
        void initWithModule(const llvm::Module& mod, map<BasicBlock*, string> invariants, map<string, string> varNameInInvariants);
        void initEmpty();
    };

}

#endif
