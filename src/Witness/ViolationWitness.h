#ifndef CEAGLE_VIOLATIONWITNESS_H
#define CEAGLE_VIOLATIONWITNESS_H

#include "Witness.h"

#include <llvm/IR/BasicBlock.h>

using llvm::BasicBlock;

namespace ceagle {

    class ViolationWitness : public Witness {
    public:
        ViolationWitness(string srcFile, const map<string, string> &taskInfo) : Witness(srcFile, taskInfo) {
            this->taskInfo["witness-type"] = "violation_witness";
        }
        ViolationWitness(string srcFile) : ViolationWitness(srcFile, map<string, string>()) {}
        void initWithPath(vector<const BasicBlock *> errorPath, map<string, string> assumptions);
        void initEmpty();
    };

}

#endif