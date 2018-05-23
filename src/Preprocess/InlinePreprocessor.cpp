#include "InlinePreprocessor.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

void removeAllSuccessorsPhiUses(BasicBlock* block);

namespace ceagle {

void InlinePreprocessor::preprocess(Module& module) {
	Function* mainFunction = module.getFunction("main");
	LLVMContext &context = getGlobalContext();
	BasicBlock* inlineWarningBlock = BasicBlock::Create(context, this->inlineWarningLabel, mainFunction);
	
	// insert inline warning block
    if (mainFunction->getReturnType()->isVoidTy()) {
        ReturnInst::Create(context, inlineWarningBlock);
    } else {
        ReturnInst::Create(context, ConstantInt::get(Type::getInt32Ty(context), -1, true), inlineWarningBlock);
    }
	
	// maps counting recursion depth and call end
	std::map<Function*, int> funcCallDepths;
	std::map<BasicBlock*, Function*> funcCallEnds;
	
	for (Function::iterator fi = mainFunction->begin(), fe = mainFunction->end(); fi != fe; ++ fi) {
		if (funcCallEnds.count(fi)) {
			-- funcCallDepths[funcCallEnds[fi]];
		}
		
		for (BasicBlock::iterator bi = fi->begin(), be = fi->end(); bi != be; ++ bi) {
			if (CallInst* inst = dyn_cast<CallInst>(bi)) {
				Function* func = inst->getCalledFunction();
				
				if (!func || func->isDeclaration()) {
					continue;
				}
				
				if (std::find(this->inlineWhitelist.begin(), this->inlineWhitelist.end(), func->getName()) != this->inlineWhitelist.end()) {
					continue;
				}
				
				if (funcCallDepths[func] >= maxRecursionDepth) {
					removeAllSuccessorsPhiUses(fi);
					BranchInst* inlineWarningBr = BranchInst::Create(inlineWarningBlock);
					ReplaceInstWithInst(fi->getTerminator(), inlineWarningBr);
					break;
				}
				
				++ funcCallDepths[func];
				
				ValueToValueMapTy VMap;
				Function* funcCopy = CloneFunction(func, VMap, false);
				std::vector<AllocaInst*> funcNewArgs;
				Function::iterator funcEntry = funcCopy->begin();
				
				for (Function::arg_iterator fai = funcCopy->arg_begin(), fae = funcCopy->arg_end(); fai != fae; ++ fai) {
					// allocate variables to store argument values later
					AllocaInst* allocaArg = new AllocaInst(fai->getType());
					fi->getInstList().insert(inst, allocaArg);
					// inlined code uses allocated variables instead of original arguments
					LoadInst* loadArg = new LoadInst(allocaArg);
					funcEntry->getInstList().insert(funcEntry->begin(), loadArg);
					fai->replaceAllUsesWith(loadArg);
					funcNewArgs.push_back(allocaArg);
				}
				
				// store argument values into allocated variables above
				std::vector<AllocaInst*>::iterator argIter = funcNewArgs.begin();
				for (Use &U : inst->arg_operands()) {
					if (argIter == funcNewArgs.end()) {
						break;
					}
					Value* V = U.get();
					StoreInst* storeArg = new StoreInst(V, *argIter);
					fi->getInstList().insert(inst, storeArg);
					++ argIter;
				}
				
				// allocate a new variable to store return value if function returns
				if (!funcCopy->getReturnType()->isVoidTy()) {
					AllocaInst* funcRet = new AllocaInst(funcCopy->getReturnType());
					fi->getInstList().insert(inst, funcRet);
					LoadInst* loadRet = new LoadInst(funcRet);
					fi->getInstList().insertAfter(inst, loadRet);
					inst->replaceAllUsesWith(loadRet);
					for (Function::iterator fi2 = funcCopy->begin(), fe2 = funcCopy->end(); fi2 != fe2; ++ fi2) {
						BasicBlock::iterator blockEnd = fi2->end();
						-- blockEnd;
						if (ReturnInst* retInst = dyn_cast<ReturnInst>(blockEnd)) {
							StoreInst* storeRet = new StoreInst(retInst->getReturnValue(), funcRet);
							fi2->getInstList().insert(retInst, storeRet);
						}
					}
				}
				
				// insert function code, reconstruct branch relations
				BasicBlock *newBlock = fi->splitBasicBlock(bi);
				for (Function::iterator fi3 = funcCopy->begin(), fe3 = funcCopy->end(); fi3 != fe3; ++ fi3) {
					BasicBlock::iterator blockEnd = fi3->end();
					-- blockEnd;
					if (dyn_cast<ReturnInst>(blockEnd)) {
						BranchInst* retBr = BranchInst::Create(newBlock);
						ReplaceInstWithInst(blockEnd, retBr);
					}
				}
				Function::iterator fi4 = fi;
				++ fi4;
				mainFunction->getBasicBlockList().splice(fi4, funcCopy->getBasicBlockList());
				fi->getTerminator()->setSuccessor(0, funcEntry);
				newBlock->begin()->eraseFromParent();
				
				funcCallEnds[newBlock] = func;
				break;
			}
		}
	}
}

void InlinePreprocessor::setRecursionDepth(int depth) {
	this->maxRecursionDepth = depth;
}

void InlinePreprocessor::removeAllSuccessorsPhiUses(BasicBlock* block) {
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