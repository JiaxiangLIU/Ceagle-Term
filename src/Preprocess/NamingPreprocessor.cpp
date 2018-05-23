#include "NamingPreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/Support/FormattedStream.h>

using namespace llvm;

namespace ceagle {

void attachNamesToAllValuesAndStructs(Module* mod) {
	int count = 0;
	
	for (Module::global_iterator mgi = mod->global_begin(), mge = mod->global_end(); mgi != mge; ++ mgi) {
        std::string name = "";
		if (mgi->hasName()) {
			name = mgi->getName().str();
		} else {
			name = "gv_" + std::to_string(count++);
		}
        //llvm::outs() << "Global name: " << name << "\n";
	}
	
	count = 0;
	
	Function* mainFunction = mod->getFunction("main");
	for (Function::iterator fi = mainFunction->begin(), fe = mainFunction->end(); fi != fe; ++ fi) {
        // no need to name basic blocks
        // but you need to count them
        if (!fi->hasName()) {
            std::string name = "bb_" + std::to_string(count++);
            fi->setName(name);
        }
		
		for (BasicBlock::iterator bi = fi->begin(), be = fi->end(); bi != be; ++ bi) {
            std::string name = "";
			if (!bi->getType()->isVoidTy()) {
				if (bi->hasName()) {
					name = bi->getName().str();
                    // Fix for smt name
                    if (name == "rem" || name == "xor" || name == "and" || name == "or") {
                        bi->setName("_" + name);
                    }
				} else {
					name = "lv_" + std::to_string(count++);
                    bi->setName(name);
                    //llvm::outs() << name << ":" << bi->getName() << "\n";
				}
			}
		}
	}
}

void NamingPreprocessor::preprocess(Module& module) {
	Function* mainFunction = module.getFunction("main");
	LLVMContext &context = getGlobalContext();
	
	attachNamesToAllValuesAndStructs(&module);
}

}
