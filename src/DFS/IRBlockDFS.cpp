#include "IRBlockDFS.h"

using llvm::Function;

namespace ceagle {

void IRBlockDFS::visit(const Module& module) {
	Function* mainFunction = module.getFunction("main");
	visitBlock(module, mainFunction->begin());
	if (this->result == DFSResult::DR_NULL) {
		this->result = DFSResult::DR_SAFE;
	}
}

void IRBlockDFS::visitBlock(const Module& module, const BasicBlock* block) {
	this->currentPath.push_back(block);
	this->advisor->dfsVisit(module, currentPath);
	
	NextChoice<const BasicBlock*> next = this->advisor->dfsNext(module, this->currentPath);
	switch (next.flag) {
		case NextPolicy::NP_NULL:
			break;
		case NextPolicy::NP_CONTINUE_NO_ADVICE:
			for (unsigned i = 0; i < block->getTerminator()->getNumSuccessors(); ++ i) {
				visitBlock(module, block->getTerminator()->getSuccessor(i));
				if (this->terminate) {
					break;
				}
			}
			break;
		case NextPolicy::NP_CONTINUE_DETERMINED_ADVICE:
			for (const BasicBlock* b : next.choices) {
				visitBlock(module, b);
				if (this->terminate) {
					break;
				}
			}
			break;
		case NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE:
			this->result = DFSResult::DR_UNKNOWN;
			for (const BasicBlock* b : next.choices) {
				visitBlock(module, b);
				if (this->terminate) {
					break;
				}
			}
			break;
		case NextPolicy::NP_BREAK_UNSAFE:
			this->terminate = 1;
			this->result = DFSResult::DR_UNSAFE;
			break;
		case NextPolicy::NP_BREAK_SAFE:
			this->terminate = 1;
			this->result = DFSResult::DR_SAFE;
			break;
		case NextPolicy::NP_BREAK_UNKNOWN:
			this->terminate = 1;
			this->result = DFSResult::DR_UNKNOWN;
			break;
		default:
			break;
	}
	
	if (!this->terminate) {
        auto backwards = this->advisor->dfsBackward(module, currentPath);
		this->currentPath.pop_back();
		switch (backwards) {
			case BackwardPolicy::BP_NULL:
				break;
			case BackwardPolicy::BP_ALLOW:
				break;
			case BackwardPolicy::BP_FORBIDDEN:
				visitBlock(module, block);
				break;
			default:
				break;
		}
	}
}

DFSResult IRBlockDFS::getResult() {
	return this->result;
}

}
