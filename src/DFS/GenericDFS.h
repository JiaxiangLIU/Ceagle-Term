#ifndef CEAGLE_GENERIC_DFS_H
#define CEAGLE_GENERIC_DFS_H

#include "DFS.h"
#include "Util/IRUtil.h"

namespace ceagle {

class BasicBlockWrapper {
    const llvm::BasicBlock* basicBlock;
public:
    class ilist : public std::vector<BasicBlockWrapper> {
    public:
        ilist() = default;
        ilist(std::vector<const BasicBlock*> l) {
            for(auto& item: l) {
                this->push_back(*item);
            }
        }
        operator const std::vector<const BasicBlock*>() const {
            auto result = std::vector<const BasicBlock*>();
            for (auto &node: *this) {
                const BasicBlock& bb = node;
                result.push_back(&bb);
            }
            return result;
        }
    };
    BasicBlockWrapper(const llvm::BasicBlock& basicBlock) {
        this->basicBlock = &basicBlock;
    }
    ilist getSuccessors() const {
        return util::getSuccessors(this->basicBlock);
    }
    operator const llvm::BasicBlock&() const { return *basicBlock; }
};

class ModuleWrapper {
    const llvm::Module* module;
public:
    ModuleWrapper(const llvm::Module& module) {
        this->module = &module;
    }
    const BasicBlockWrapper entry() const {
        return BasicBlockWrapper(*this->module->getFunction("main")->begin());
    }
    operator const llvm::Module&() const { return *module; }
};

template<class Advisor, class GraphTy=ModuleWrapper, class NodeTy=BasicBlockWrapper>
class GenericDFS {
	bool terminate = 0;
	DFSResult result = DFSResult::DR_NULL;
    typename NodeTy::ilist currentPath;
protected:
    Advisor *advisor;
public:
	GenericDFS(Advisor *advisor) {
        this->advisor = advisor;
    }
	virtual void visit(const GraphTy& module) {
	    visitNode(module, module.entry());
	    if (this->result == DFSResult::DR_NULL) {
	    	this->result = DFSResult::DR_SAFE;
	    }
    }
	void visitNode(const GraphTy& module, const NodeTy& block) {
    	this->currentPath.push_back(block);
    	this->advisor->dfsVisit(module, currentPath);

    	auto next = this->advisor->dfsNext(module, this->currentPath);
    	switch (next.flag) {
    		case NextPolicy::NP_NULL:
    			break;
    		case NextPolicy::NP_CONTINUE_NO_ADVICE:
                for (auto& successor: block.getSuccessors()) {
                    visitNode(module, successor);
                    if (this->terminate) {
                        break;
                    }
                }
    			break;
    		case NextPolicy::NP_CONTINUE_DETERMINED_ADVICE:
    			for (auto& b : next.choices) {
    				visitNode(module, *b);
    				if (this->terminate) {
    					break;
    				}
    			}
    			break;
    		case NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE:
    			this->result = DFSResult::DR_UNKNOWN;
    			for (auto& b : next.choices) {
    				visitNode(module, *b);
    				if (this->terminate) {
    					break;
    				}
    			}
    			break;
    		case NextPolicy::NP_BREAK_UNSAFE:
    			this->terminate = true;
    			this->result = DFSResult::DR_UNSAFE;
    			break;
    		case NextPolicy::NP_BREAK_SAFE:
    			this->terminate = true;
    			this->result = DFSResult::DR_SAFE;
    			break;
    		case NextPolicy::NP_BREAK_UNKNOWN:
    			this->terminate = true;
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
    				visitNode(module, block);
    				break;
    			default:
    				break;
    		}
    	}
    }
	virtual DFSResult getResult() {
	    return this->result;
    }
};

}

#endif
