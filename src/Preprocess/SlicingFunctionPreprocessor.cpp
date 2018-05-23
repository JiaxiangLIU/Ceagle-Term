#include "SlicingFunctionPreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/Support/FormattedStream.h>

namespace ceagle {

// true: this function can be invoked by main
// false: this function can NOT be invoked by main
bool SlicingFunctionPreprocessor::visitFunction(Function *f) {
    std::vector<Function *>::iterator it = std::find(this->m_functionsVisited.begin(), this->m_functionsVisited.end(), f);
    if (it == this->m_functionsVisited.end()) {
        //this->m_functionsVisited.push_back(f);

        // new function, first time visiting
        // do not add it for now, check it's parents first to determine whether add this
    } else if ((*it)->getName() == this->m_targetFunctionName) {
    } else {
        //llvm::outs() << "SlicingFunctionPreprocesso revisit of a function " << (*it)->getName() << "\n";
        return true;
    }

    if (f->getNumUses() <= 0) {
        //llvm::outs() << "SlicingFunctionPreprocessor::visitFunction end is " << f->getName() << "\n";
        if (f->getName() == "main") {
            //llvm::outs() << "SlicingFunctionPreprocessor::visitFunction adding " << f->getName() << "\n";
            this->m_functionsVisited.push_back(f);
            return true;
        } else {
            //llvm::outs() << "SlicingFunctionPreprocessor::visitFunction ignoring " << f->getName() << "\n";
            return false;
        }
    }

    //llvm::outs() << "SlicingFunctionPreprocessor visit function " << f->getName() << " " << f->getNumUses() << "\n";
    bool hasAtLeastOnePathToMain = false;
    for (Value::user_iterator i = f->user_begin(), e = f->user_end(); i != e; ++i) {
        if (Instruction *v= dyn_cast<Instruction>(*i)) {
            Function *vf  = v->getParent()->getParent();
            if (this->visitFunction(vf)) {
                //llvm::outs() << "SlicingFunctionPreprocessor::visitFunction adding " << f->getName() << "\n";
                this->m_functionsVisited.push_back(f);
                hasAtLeastOnePathToMain = true;
            } else {
                //llvm::outs() << "SlicingFunctionPreprocessor::visitFunction ignoring " << f->getName() << "\n";
            }
        } else if (Function *v = dyn_cast<Function>(*i)) {
            llvm::outs() << "<error>\n";
        }
    }
    return hasAtLeastOnePathToMain;
}

void SlicingFunctionPreprocessor::removeFunction(Function *f) {
    f->replaceAllUsesWith(UndefValue::get((Type *)f->getType()));
    f->eraseFromParent();
}

void SlicingFunctionPreprocessor::removeFunctions(std::vector<Function *> vf) {
    for (int i = 0; i < vf.size(); i++) {
        this->removeFunction(vf[i]);
    }
}

void SlicingFunctionPreprocessor::outputFunctions(std::vector<Function *> vf) {
    for (int i = 0; i < vf.size(); i++) {
        llvm::outs() << vf[i]->getName() << " ";
    }
    llvm::outs() << "\n";
}

void SlicingFunctionPreprocessor::preprocess(Module& module) {
	Function* targetFunction = module.getFunction(this->m_targetFunctionName);
    llvm::outs() << "SlicingFunctionPreprocessor slicing for " << targetFunction->getName() << ":\n";

    //this->m_functionsVisited.push_back(targetFunction);
    this->visitFunction(targetFunction);

    //llvm::outs() << "SlicingFunctionPreprocessor found:\n";
    //this->outputFunctions(this->m_functionsVisited);

    for (Module::iterator it = module.begin(), ite = module.end(); it != ite; ++ it) {
        std::vector<Function *>::iterator itf = std::find(this->m_functionsVisited.begin(), this->m_functionsVisited.end(), &(*it));
        if (itf == this->m_functionsVisited.end()) {
            //llvm::outs() << "SlicingFunctionPreprocessor should delete " << it->getName() << "\n";
            this->m_functionsToDelete.push_back(&(*it));
        }
    }

    //llvm::outs() << "SlicingFunctionPreprocessor to delete:\n";
    //this->outputFunctions(this->m_functionsToDelete);
    this->removeFunctions(this->m_functionsToDelete);
}

}
