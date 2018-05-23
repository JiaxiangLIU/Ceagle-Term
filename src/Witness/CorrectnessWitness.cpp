#include "CorrectnessWitness.h"

namespace ceagle {

void CorrectnessWitness::initEmpty() {
    Node* n = new Node(1, 0);
    this->add(n);
}

void CorrectnessWitness::initWithModule(const llvm::Module& mod, map<BasicBlock*, string> invariants, map<string, string> varNameInInvariants) {
    initEmpty();
}

}