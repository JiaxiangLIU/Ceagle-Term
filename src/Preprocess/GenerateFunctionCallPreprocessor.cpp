#include "GenerateFunctionCallPreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/Support/FormattedStream.h>

namespace ceagle {

void outputIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        llvm::outs() << "  ";
    }
}

void GenerateFunctionCallPreprocessor::outputFunction(Function *f, int indent) {
    /*
    llvm::outs() << "GenerateFunctionCallPreprocessor [" << this->m_targetFunctionName << "] is " << targetFunction << "\n";
    for (Value::use_iterator i = targetFunction->use_begin(), e = targetFunction->use_end(); i != e; ++i) {
        llvm::outs() << "GenerateFunctionCallPreprocessor [" << this->m_targetFunctionName << "] is used in Value: " << *i << "\n";
        if (Instruction *Inst = dyn_cast<Instruction>(*i)) {
            llvm::outs() << "GenerateFunctionCallPreprocessor [" << this->m_targetFunctionName << "] is used in instruction " << *Inst << "\n";
        } else if (Function *v = dyn_cast<Function>(*i)) {
            llvm::outs() << "GenerateFunctionCallPreprocessor [" << this->m_targetFunctionName << "] is used in function " << *v << "\n";
        }
    }
    */

    // control visit times
    if (this->m_functionVisitTimes.find(f) == this->m_functionVisitTimes.end()) {
        std::vector<Function *>::iterator it = std::find(this->m_functionVisited.begin(), this->m_functionVisited.end(), f);
        if (it == this->m_functionVisited.end()) {
            this->m_functionVisited.push_back(f);
        } else {
            this->m_functionVisitTimes.insert(std::pair<Function *, int>(f, 1));
        }
    } else {
        this->m_functionVisitTimes[f]++;
        if (this->m_functionVisitTimes[f] >= this->m_maxVisitTimes) {
            return;
        }
    }

    outputIndent(indent);
    //llvm::outs() << f->getName() << ".\n";
    llvm::outs() << f->getName() << "\n";
    if (f->getNumUses() <= 0) {
        outputIndent(indent);
        llvm::outs() << "  <end>\n";
        return;
    }
    for (Value::user_iterator i = f->user_begin(), e = f->user_end(); i != e; ++i) {
        //llvm::outs() << "use " << (*i)->getUser() << ", " << (*i)->getUser()->getName() << "\n";
        //llvm::outs() << "use " << (*i) << ", " << (*i)->getName() << ".\n";
        if (Instruction *v= dyn_cast<Instruction>(*i)) {
            //llvm::outs() << "GenerateFunctionCallPreprocessor [" << this->m_targetFunctionName << "] is used in instruction " << *Inst << "\n";
            //outputIndent(indent + 1);
            //llvm::outs() << "instruction " << v << ", " << v->getName() << ".\n";
            Function *vf  = v->getParent()->getParent();
            outputFunction(vf, indent + 1);
            //outputIndent(indent + 1);
            //llvm::outs() << "function " << vf << ", " << vf->getName() << ".\n";
        } else if (Function *v = dyn_cast<Function>(*i)) {
            //llvm::outs() << "function " << v << ", " << v->getName() << ".\n";
            llvm::outs() << "<error>\n";
            //outputFunction(v, indent + 1);
        }
    }
}

void GenerateFunctionCallPreprocessor::preprocess(Module& module) {
	Function* targetFunction = module.getFunction(this->m_targetFunctionName);
    if (!targetFunction) {
        llvm::outs() << "GenerateFunctionCallPreprocessor::preprocess function " << this->m_targetFunctionName << " not found\n";
        return;
    }
    llvm::outs() << "GenerateFunctionCallPreprocessor parents for " << this->m_targetFunctionName << ":\n";
    this->outputFunction(targetFunction, 0);
}

}
