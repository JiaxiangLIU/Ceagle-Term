#ifndef CEAGLE_IR_BLOCK_DFS_H
#define CEAGLE_IR_BLOCK_DFS_H

#include "DFS.h"

namespace ceagle {
	
class IRBlockDFS : public DFS<> {
	bool terminate = 0;
	DFSResult result = DFSResult::DR_NULL;
	std::vector<const BasicBlock*> currentPath;
public:
	IRBlockDFS(Advisor<> *advisor) : DFS(advisor) {}
	virtual void visit(const Module& module);
	void visitBlock(const Module& module, const BasicBlock* block);
	virtual DFSResult getResult();
};

}

#endif
