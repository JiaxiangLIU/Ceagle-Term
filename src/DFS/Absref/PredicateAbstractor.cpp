#include "DFS/Absref/PredicateAbstractor.h"
#include "DFS/Absref/AbstractState.h"
#include "Util/IRUtil.h"
#include "SMT/Translator/InstTranslator.h"

#define DEBUG_ABSTRACTOR 1

using namespace ceagle::util;

std::vector<std::shared_ptr<ceagle::AbstractState>> ceagle::PredicateAbstractor::next(
        const std::shared_ptr<AbstractState> &state) {
    auto nextLocations = getSuccessorsWithPhiSelector(state->_location);
    std::vector<std::shared_ptr<AbstractState>> result;
    auto vars = getChangedVariables(state->_location);
    std::map<llvm::Value*, Expr> valueMap;
    std::map<std::string, Expr> replaceMap;
    for (auto p: nextLocations) {
        std::unique_ptr<VisitorTranslator> translator(new VisitorTranslator());
        Expr condition = translator->path2SMTM({state->_location, p.second}, valueMap, state->_phiSelector);
        for (auto var: valueMap) {
            replaceMap[var.first->getName()] = var.second;
#if 0
            std::cout << var.first->getName().str() << std::endl;
            std::cout << var.second << std::endl;
#endif
        }
        auto nextState = this->abstract(*state, replaceMap, condition, p.second);
        if (nextState != nullptr) {
            nextState->setPhiSelector(p.first);
            result.push_back(std::shared_ptr<AbstractState>(nextState));
        }
    }
    return result;
}

namespace ceagle {

using namespace smt;

PredicateAbstractor::PredicateAbstractor(Precision& _p) : p(_p) { }

void PredicateAbstractor::refine(std::vector<std::shared_ptr<AbstractState>> path) {
    // first we need to restrict the path to sat part and a node which is not sat
    std::vector<llvm::BasicBlock*> concretePath;
    for (auto state: path) {
        concretePath.push_back(state->location());
    }
    concretePath.pop_back();
    auto flag = CheckResult::UNSAT;
    while (flag != CheckResult::SAT) {
        VisitorTranslator v;
        auto c = v.path2SMTM(concretePath);
        auto solver = makeSolver();
        solver->add(c);
        flag = solver->checkSat();
        if (flag != CheckResult::SAT) {
            concretePath.pop_back();
            path.pop_back();
        }
    }
    if (path.size() < 2) return;
    auto it = path.begin();
    auto rit = path.rbegin();
    auto ret = path.rend();
    Expr phi = BoolValue(false);
    while((++rit) != ret) {
        // interpolant from [it, rit], rit
        // that is [it, rit.base()), rit
        // which is vm1 && c1 to wp(phi, vm2) && c2
        // phi init to true on last block before error
        std::vector<std::shared_ptr<AbstractState>> left(it, rit.base());
        std::vector<std::shared_ptr<AbstractState>> right((rit+1).base(), rit.base() + 1);
        std::vector<llvm::BasicBlock*> leftPath;
        std::vector<llvm::BasicBlock*> rightPath;
        std::map<std::string, Expr> vm1;
        std::map<std::string, Expr> vm2;
        // get vm1 and c1
        std::map<llvm::Value*, Expr> valueMap;
        for (auto state: left) {
            leftPath.push_back(state->location());
        }
        std::unique_ptr<VisitorTranslator> translator(new VisitorTranslator());
        auto c1 = translator->path2SMTM(leftPath, valueMap);
        for (auto pa : valueMap) {
            if (pa.first->hasName()) {
                vm1[pa.first->getName()] = pa.second;
            }
        }
        // get vm2 and c2
        translator.reset(new VisitorTranslator());
        for (auto state: right) {
            rightPath.push_back(state->location());
        }
        valueMap.clear();
        auto c2 = translator->path2SMTM(rightPath, valueMap, right[0]->getPhiSelector());
        for (auto pa : valueMap) {
            if (pa.first->hasName()) {
                vm2[pa.first->getName()] = pa.second;
            }
        }
        auto refines = refine(vm1, c1, (c2 && !phi.replace(vm2)));
        auto newPredicates = refines[0];
#if DEBUG_ABSTRACTOR
        std::cout << (*rit)->location()->getName().str() << ": ";
        std::cout << newPredicates << std::endl;
#endif
        auto index = p.add(newPredicates);
        p.addPrecision((*rit)->location(), index);
        phi = newPredicates;
    }
}

std::vector<Expr> PredicateAbstractor::refine(std::map<std::string, Expr> vm1, Expr c1, Expr c2) {
    auto solver = makeSolver(true);
    Expr e1 = c1;
    for (auto &pa: vm1) {
        e1 = e1 && (Var(pa.first, pa.second.sort()) == pa.second);
    }
    solver->add(e1, "f1");
    solver->add(c2, "f2");
    auto checkResult = solver->checkSat();
    assert(checkResult == CheckResult::UNSAT && "interpolant must be unsat formula");
    try {
        auto result = solver->getInterp({"f1", "f2"});
        if (result.size() == 0) {
#if DEBUG_ABSTRACTOR
            std::cout << "Interpolanting failed." << std::endl;
#endif
            return {e1};
        }
        return result;
    } catch (ParseException) {
#if DEBUG_ABSTRACTOR
        std::cout << "Interpolanting failed." << std::endl;
#endif
        return {e1};
    }
}

AbstractState* PredicateAbstractor::abstract(AbstractState & start, std::map<std::string, Expr > valueMap, Expr condition, llvm::BasicBlock* nextLocation) {
    auto solver = makeSolver();
    auto startPrecision = p.getPrecisionForLocation(start.location());
    for (auto i: startPrecision) {
        solver->add(Var("b!" + std::to_string(i), BoolSort()) == p.get(i));
    }
    solver->add(start);
    solver->add(condition);
    auto checkResult = solver->checkSat();
    if (checkResult == CheckResult::UNSAT) {
        return nullptr;
    }
    std::set<int> nextState;
    decltype(startPrecision) endPrecision;
    if (nextLocation != nullptr) {
        endPrecision = p.getPrecisionForLocation(nextLocation);
    }
    if (endPrecision.empty()) {
        endPrecision = startPrecision;
    }
    for (auto i: endPrecision) {
        auto expr = p.get(i);
        expr = expr.replace(valueMap);
        solver->push();
        solver->add(!expr);
        auto result = solver->checkSat();
        if (result == CheckResult::UNSAT) {
            nextState.insert(i);
        }
        solver->pop();
        solver->push();
        solver->add(expr);
        result = solver->checkSat();
        if (result == CheckResult::UNSAT) {
            nextState.insert(-i);
        }
        solver->pop();
    }
    return new AbstractState(nextLocation, nextState);
}

}
