#include "CeagleInterfacePreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace ceagle {

void CeagleInterfacePreprocessor::preprocess(Module& module) {
	Function* mainFunction = module.getFunction("main");
	LLVMContext &context = getGlobalContext();
	BasicBlock* assertFailBlock = BasicBlock::Create(context, this->assertFailLabel, mainFunction);
	BasicBlock* assumeFailBlock = BasicBlock::Create(context, this->assumeFailLabel, mainFunction);
	
    if (mainFunction->getReturnType()->isVoidTy()) {
        ReturnInst::Create(context, assertFailBlock);
		ReturnInst::Create(context, assumeFailBlock);
    } else {
        ReturnInst::Create(context, ConstantInt::get(Type::getInt32Ty(context), -1, true), assertFailBlock);
		ReturnInst::Create(context, ConstantInt::get(Type::getInt32Ty(context), -1, true), assumeFailBlock);
    }
	
	for (Function::iterator fi = mainFunction->begin(), fe = mainFunction->end(); fi != fe; ++ fi) {
		for (BasicBlock::iterator bi = fi->begin(), be = fi->end(); bi != be; ++ bi) {
			if (CallInst* inst = dyn_cast<CallInst>(bi)) {
				Function* func = inst->getCalledFunction();
				
				if (!func) {
					continue;
				}
				
				if (func->getName() == this->assertFuncName) {
					++ bi;
                    BasicBlock* newBlock = fi->splitBasicBlock(bi);
                    TruncInst* assertTrunc = new TruncInst(inst->getArgOperand(0), Type::getInt1Ty(context));
                    assertTrunc->insertBefore(inst);
                    BranchInst* assertBr = BranchInst::Create(newBlock, assertFailBlock, assertTrunc);
                    ReplaceInstWithInst(fi->getTerminator(), assertBr);
                    inst->eraseFromParent();
                    break;
				} else if (func->getName() == this->assumeFuncName) {
					++ bi;
                    BasicBlock* newBlock = fi->splitBasicBlock(bi);
                    TruncInst* assumeTrunc = new TruncInst(inst->getArgOperand(0), Type::getInt1Ty(context));
                    assumeTrunc->insertBefore(inst);
                    BranchInst* assumeBr = BranchInst::Create(newBlock, assumeFailBlock, assumeTrunc);
                    ReplaceInstWithInst(fi->getTerminator(), assumeBr);
                    inst->eraseFromParent();
                    break;
				}
			}
		}
	}
}

void CeagleInterfacePreprocessor::removeAllSuccessorsPhiUses(BasicBlock* block) {
	TerminatorInst* inst = block->getTerminator();
	for (unsigned i = 0; i < inst->getNumSuccessors(); ++ i) {
		BasicBlock::iterator bi = inst->getSuccessor(i)->begin();
		while (dyn_cast<PHINode>(bi)) {
			PHINode* phi = dyn_cast<PHINode>(bi);
			phi->removeIncomingValue(block, false);
			
			if (phi->getNumIncomingValues() == 0) {
				phi->replaceAllUsesWith(UndefValue::get(phi->getType()));
				phi->eraseFromParent();
			}
			
			++ bi;
		}
	}
}

}