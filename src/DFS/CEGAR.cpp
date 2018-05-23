#include "DFS/CEGAR.h"

namespace ceagle {

llvm::BasicBlock *CEGAR::entry(llvm::Module &module) const {
    return module.getFunction("main")->begin();
}

bool CEGAR::isTarget(const std::shared_ptr<AbstractState> &state) const {
    return state->name().find("ERROR") != std::string::npos;
}

void CEGAR::refine(std::vector<std::shared_ptr<AbstractState>> path) {
    abstractor.refine(path);
}

bool CEGAR::isFeasible(std::vector<std::shared_ptr<AbstractState>> path) {
    std::vector<BasicBlock*> irPath;
    for (auto& state: path) {
        irPath.push_back(state->location());
    }
    std::unique_ptr<VisitorTranslator> translator(new VisitorTranslator());
    auto expr = translator->path2SMTM(irPath);
    std::unique_ptr<Z3RawSolver> solver(new Z3RawSolver());
    solver->add(expr);
    auto result = solver->checkSat();
    if (result == CheckResult::UNKNOWN) {
        unknownErrorPathFound = true;
        return false;
    }
    if (result == CheckResult::SAT) {
        model = solver->getModel();
        return true;
    }
    return false;
}

void CEGAR::reportSafe() {
    if (unknownErrorPathFound) {
        result = DFSResult::DR_UNKNOWN;
    } else {
        result = DFSResult::DR_SAFE;
    }
}

void CEGAR::reportError() {
    result = DFSResult::DR_UNSAFE;
}

DFSResult CEGAR::getResult() {
    return result;
}

Model CEGAR::getModel() {
    return model;
}

decltype(CEGAR::witness) CEGAR::getWitness() {
    return witness;
}

std::vector<std::shared_ptr<AbstractState>> CEGAR::getErrorPath() {
    return reached.errorPath;
}

void CEGAR::visit(llvm::Module &module) {
    this->module = &module;
    auto e0 = entry(module);
    std::shared_ptr<AbstractState> start(new AbstractState(e0));
    reached= AbstractReachedSet(start);
    AbstractStateWaitList waitList(start);
    while (true) {
        reached.expand(waitList, this->abstractor, [&](const std::shared_ptr<AbstractState>& state){
            return this->isTarget(state);
        });
        if (waitList.empty()) {
            this->witness = reached.getInvariants(&abstractor.getPrecision());
            return reportSafe();
        } else {
            auto path = reached.errorPath;
            if (!isNewPath(path)) {
                throw "refine failed";
            }
            this->errorPath = path;
            if (isFeasible(path)) {
                return reportError();
            } else {
                refine(path);
                start.reset(new AbstractState(e0));
                reached = AbstractReachedSet(start);
                waitList = AbstractStateWaitList(start);
            }
        }
    }
}

bool CEGAR::isNewPath(const std::vector<std::shared_ptr<AbstractState>> & newPath) const {
    bool identical = true;
    if (newPath.size() == errorPath.size()) {
        for(auto it1 = newPath.begin(), it2 = errorPath.begin(), et1 = newPath.end(); it1 != et1; ++it1, ++it2) {
            if ((*it1)->location() != (*it2)->location()) {
                identical = false;
                break;
            }
        }
    } else {
        identical = false;
    }
    return !identical;
}
} // end namespace ceagle
