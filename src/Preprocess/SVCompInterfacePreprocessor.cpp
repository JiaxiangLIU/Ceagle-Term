#include "SVCompInterfacePreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace ceagle {
	
void SVCompInterfacePreprocessor::preprocess(Module& module) {
	Function* mainFunction = module.getFunction("main");
	LLVMContext &context = getGlobalContext();
	BasicBlock* errorBlock = BasicBlock::Create(context, this->errorLabel, mainFunction);
	BasicBlock* assumeFailBlock = BasicBlock::Create(context, this->assumeFailLabel, mainFunction);
	
	if (mainFunction->getReturnType()->isVoidTy()) {
        ReturnInst::Create(context, errorBlock);
		ReturnInst::Create(context, assumeFailBlock);
    } else {
        ReturnInst::Create(context, ConstantInt::get(Type::getInt32Ty(context), -1, true), errorBlock);
		ReturnInst::Create(context, ConstantInt::get(Type::getInt32Ty(context), -1, true), assumeFailBlock);
    }
	
	for (Function::iterator fi = mainFunction->begin(), fe = mainFunction->end(); fi != fe; ++ fi) {
		for (BasicBlock::iterator bi = fi->begin(), be = fi->end(); bi != be; ++ bi) {
			if (CallInst* inst = dyn_cast<CallInst>(bi)) {
				Function* func = inst->getCalledFunction();
				
				if (!func) {
					continue;
				}
				
				if (func->getName() == SVCOMP_INTERFACE_VERIFIER_ERROR) {
					removeAllSuccessorsPhiUses(fi);
					BranchInst* errorBr = BranchInst::Create(errorBlock);
					ReplaceInstWithInst(fi->getTerminator(), errorBr);
					inst->eraseFromParent();
					break;
				} else if (func->getName() == SVCOMP_INTERFACE_VERIFIER_ASSUME) {
					++ bi;
                    BasicBlock* newBlock = fi->splitBasicBlock(bi);
					
					if (CastInst* castInst = dyn_cast<CastInst>(inst->getArgOperand(0))) {
						if (castInst->isIntegerCast() && castInst->getSrcTy()->isIntegerTy(1)) {
							BranchInst* assumeBr = BranchInst::Create(newBlock, assumeFailBlock, castInst->getOperand(0));
							ReplaceInstWithInst(fi->getTerminator(), assumeBr);
							inst->eraseFromParent();
							break;
						}
					}
					
                    TruncInst* assumeTrunc = new TruncInst(inst->getArgOperand(0), Type::getInt1Ty(context));
                    assumeTrunc->insertBefore(inst);
                    BranchInst* assumeBr = BranchInst::Create(newBlock, assumeFailBlock, assumeTrunc);
                    ReplaceInstWithInst(fi->getTerminator(), assumeBr);
                    inst->eraseFromParent();
                    break;
				} else if (func->getName() == SVCOMP_INTERFACE_VERIFIER_ASSERT) {
					++ bi;
                    BasicBlock* newBlock = fi->splitBasicBlock(bi);
					
					if (CastInst* castInst = dyn_cast<CastInst>(inst->getArgOperand(0))) {
						if (castInst->isIntegerCast() && castInst->getSrcTy()->isIntegerTy(1)) {
							BranchInst* assertBr = BranchInst::Create(newBlock, errorBlock, castInst->getOperand(0));
							ReplaceInstWithInst(fi->getTerminator(), assertBr);
							inst->eraseFromParent();
							break;
						}
					}
					
                    TruncInst* assertTrunc = new TruncInst(inst->getArgOperand(0), Type::getInt1Ty(context));
                    assertTrunc->insertBefore(inst);
                    BranchInst* assertBr = BranchInst::Create(newBlock, errorBlock, assertTrunc);
                    ReplaceInstWithInst(fi->getTerminator(), assertBr);
                    inst->eraseFromParent();
                    break;
				}
			}
		}
	}
}

void SVCompInterfacePreprocessor::removeAllSuccessorsPhiUses(BasicBlock* block) {
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
