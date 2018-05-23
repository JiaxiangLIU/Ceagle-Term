#include "DFS/Advisor/TestAdvisor.h"
#include "Util/IRUtil.h"

namespace ceagle {

void TestAdvisor::dfsVisit(const llvm::Module& module, const std::vector<const llvm::BasicBlock*>& path) {
    auto& current = path.back();
    std::cout << "=====\ndfsVisit <" << current->getName().str() << ">\n";
    switch(visited[current]) {
        case Status::NEW:
            visited[current] = Status::VISITED;
            break;
        case Status::VISITED:
            visited[current] = Status::REVISITED;
            break;
        default:
            break;
    }
    std::cout << "current path [";
    for (auto & bb: path) {
        std::cout << bb->getName().str() << ", ";
    }
    std::cout << "]\n";
}

NextChoice<const llvm::BasicBlock*> TestAdvisor::dfsNext(const Module& module, const std::vector<const BasicBlock*>& path) {
    auto& current = path.back();
    if (visited[current] == Status::REVISITED) {
        std::cout << "Already visited\n";
        return {NextPolicy::NP_CONTINUE_DETERMINED_ADVICE, {}};
    }
    auto successors = util::getSuccessors(current);
    std::cout << "dfsNext: [";
    for (auto& bb: successors) {
        std::cout << bb->getName().str() << ", ";
    }
    std::cout << "]\n";
    return {NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE, successors};
}

BackwardPolicy TestAdvisor::dfsBackward(const Module& module, const std::vector<const BasicBlock*>& path) {
    std::cout << "dfsBackward: <" << path.back()->getName().str() << ">\n";
    std::cout << "current path: [";
    for (auto& bb: path) {
        std::cout << bb->getName().str() << ", ";
    }
    std::cout << "]\n";
    return BackwardPolicy::BP_ALLOW;
}

}
