#include <algorithm>
#include <map>
#include <memory>
#include "DFS/Advisor/Absref/ARGManager.h"
#include "Util/IRUtil.h"
#include "SMT/Translator/IRPathTranslator.h"
#include "SMT/Translator/Translator.h"
#include "llvm/IR/Instructions.h"

#define ARGMANAGER_DEBUG 1

using namespace std;

namespace ceagle {

void ARGManager::init(LocationTy child) {
    root.reset(new ARGNode(child));
    current = root.get();
}

ARGNode* ARGManager::abstract(const llvm::Module& module, ARGNode* before, LocationTy end, int phiSelector) {
    auto start = before->getLocation();
    solver->reset();
    solver->add(*before);
    for (const auto index: before->getAbsState()) {
        solver->add("(= b!" + std::to_string(std::abs(index)) + " " + predicates[index] + ")");
    }
    solver->add(getGuard(module, start, end, phiSelector));
    auto checkResult = solver->checkSat();
    if (checkResult == CheckResult::UNKNOWN) {
        throw SmtException("check guard unknown");
    }
    if (checkResult == CheckResult::UNSAT) {
        return new ARGNode(end, {}, true);
    }
    std::set<int> absState;
    auto precision = loc2preds[end];
    if (precision.size() == 0) {
        precision = loc2preds[current->getLocation()];
        loc2preds[end] = precision;
    }
    //solver->add(getTrans(end));
    for (auto index: precision) {
        solver->push();
        solver->add(!Var("b!!" + std::to_string(index), BoolSort()));
        solver->add("(= b!!" + std::to_string(index) + " " + predicates[index] + ")");
        auto checkResult = solver->checkSat();
        solver->pop();
        if (checkResult == CheckResult::UNKNOWN) {
            throw SmtException(("smt check abstract " + std::to_string(index) + " unknown").c_str());
        }
        if (checkResult == CheckResult::UNSAT) {
            absState.insert(index);
            continue;
        }
        solver->push();
        solver->add(Var("b!!" + std::to_string(index), BoolSort()));
        solver->add("(= b!!" + std::to_string(index) + " " + predicates[index] + ")");
        checkResult = solver->checkSat();
        solver->pop();
        if (checkResult == CheckResult::UNKNOWN) {
            throw SmtException(("smt check abstract " + std::to_string(index) + " unknown").c_str());
        }
        if (checkResult == CheckResult::UNSAT) {
            absState.insert(-index);
        }
    }
    return new ARGNode(end, absState);
}

Expr ARGManager::getGuard(const llvm::Module& module, LocationTy start, LocationTy end, int phiSelector) {
    std::unique_ptr<Translator<>> translator(new IRPathTranslator());
    return translator->path2SMTM(module, {start, end}, phiSelector);
}

ARGNode* ARGManager::expand(const llvm::Module& module, LocationTy child) {
    auto phiSelector = -1;
    if (current->getParent() != nullptr) {
        phiSelector = getPhiIndex(current->getParent()->getLocation(), current->getLocation());
    }
    current->addChild(unique_ptr<ARGNode>(abstract(module, current, child, phiSelector)));
    current = current->getLastChild();
    locationMap[child].push_back(current);
    return current;
}

std::vector<LocationTy> ARGManager::next() {
    if (current->isEmpty()) {
        current->complete();
        return {};
    }
    auto previousARGNodes = getARGNodesByLocation(current->getLocation());
    for (const auto & previousARGNode : previousARGNodes) {
        if (previousARGNode == current) continue;
        if (previousARGNode->cover(current)) {
            current->complete();
            return {};
        }
    }
    auto currentLocation = current->getLocation();
    auto nextLocations = util::getSuccessors(currentLocation);
    auto childs = current->getChilds();
    std::vector<LocationTy> choice;
    for (size_t i = 0; i < nextLocations.size(); ++i) {
        if ( i >= childs.size() || !childs[i]->isComplete() ) {
            choice.push_back(nextLocations[i]);
        }
    }
    if ( choice.size() == 0 ) {
        current->complete();
    }
    return choice;
}

bool ARGManager::complete() {
    if ( current->isComplete() ) {
        return true;
    }
    auto childs = current->getChilds();
    bool complete = true;
    for (auto child: childs) {
        if (!child->isComplete()) {
            complete = false;
            break;
        }
    }
    current->complete(complete);
    return complete;
}

void ARGManager::backward() {
    current = current->getParent();
}

bool ARGManager::reachable(const llvm::Module& module) {
    solver->reset();
    std::unique_ptr<Translator<>> translator(new IRPathTranslator());
    auto path = getCurrentPath();
#if ARGMANAGER_DEBUG
    std::cout << "ARGManager path\n";
    for (auto& loc: path) {
        std::cout << loc->getName().str() << " ";
    }
    std::cout << "\n";
#endif
    auto pathSize = path.size();
    std::vector<std::string> interpNames;
    for (auto i = 0; i < pathSize - 1; i++) {
        auto phiSelector = (i==0)?-1:getPhiIndex(path[i-1], path[i]);
        auto terminator = path[i]->getTerminator();
        auto branchCount = terminator->getNumSuccessors();
        std::map<const Value*, Expr*> valueMap;
        // add all store value to value map
        for (auto value: util::getChangedVariables(path[i])) {
            valueMap[value] = nullptr;
        }
        auto expr = translator->path2SMTM(module, {path[i], path[i + 1]}, valueMap, phiSelector);
#if ARGMANAGER_DEBUG
        std::cout << path[i]->getName().str() << ":" << valueMap.size();
#endif
        for (auto& p : valueMap) {
            expr = expr && Var(p.first->getName(), p.second->sort()) == *p.second;
        }
#if ARGMANAGER_DEBUG
        std::cout << path[i]->getName().str() << ":" << expr << "\n";
#endif
        solver->add(expr, "f" + std::to_string(i));
        interpNames.push_back("f" + std::to_string(i));
    }
    auto result = solver->checkSat();
    if (result == CheckResult::UNKNOWN) {
        throw SmtException("smt check refine unknown");
    }
    if (result == CheckResult::SAT) {
        return true;
    }
    auto interp = solver->getInterp(interpNames);
    auto start = predicates.size();
    for (auto & inter : interp) {
        auto index = predicates.size();
        predicates.insert(std::pair<int, std::string>(index, inter));
    }
    auto absPath = getAbsPath();
    for (auto i = start; i < pathSize - 1 + start; ++i) {
        auto node = absPath[i - start];
        auto preds = loc2preds[node->getLocation()];
        preds.push_back(i);
    }
    return false;
}

void ARGManager::refine(const llvm::Module& module) {
    auto absPath = getAbsPath();
    for (size_t i = 1; i < absPath.size(); i++) {
        auto before = absPath[i-1];
        auto end = absPath[i]->getLocation();
        auto phiSelector = -1;
        if (i > 1) {
            phiSelector = getPhiIndex(absPath[i-2]->getLocation(), before->getLocation());
        }
        std::unique_ptr<ARGNode> newArgNode(abstract(module, before, end));
        absPath[i]->reset(newArgNode.get());
        absPath[i]->complete(false);
        for (ARGNode* absLoc: locationMap[absPath[i]->getLocation()]) {
            while (absLoc != root.get()) {
                if (!absLoc->isComplete() || absLoc->isEmpty()) {
                    break;
                }
                absLoc->complete(false);
                absLoc = absLoc->getParent();
            }
        }
    }
}

std::vector<ARGNode*>& ARGManager::getARGNodesByLocation(LocationTy location) {
    return locationMap[location];
}

std::vector<LocationTy> ARGManager::getCurrentPath() const {
    auto it = this->current;
    std::vector<LocationTy> result;
    while(it != nullptr) {
        result.push_back(it->getLocation());
        it = it->getParent();
    }
    return std::vector<LocationTy>(result.rbegin(), result.rend());
}

std::vector<ARGNode*> ARGManager::getAbsPath() const {
    auto it = this->current;
    std::vector<ARGNode*> result;
    while(it != nullptr) {
        result.push_back(it);
        it = it->getParent();
    }
    return std::vector<ARGNode*>(result.rbegin(), result.rend());
}

std::vector<ARGNode*> ARGNode::getChilds() const {
    std::vector<ARGNode*> result;
    auto it = firstChild.get();
    while (it != nullptr) {
        result.push_back(it);
        it = it->right.get();
    }
    return result;
}

bool ARGNode::cover(ARGNode* node) const {
    if (this->location != node->location) {
        return false;
    }
    return includes(this->absState.begin(), this->absState.end(), node->absState.begin(), node->absState.end());
}

void ARGNode::addChild(std::unique_ptr<ARGNode> child) {
    child->parent = this;
    if(firstChild) {
        lastChild->right = std::move(child);
        lastChild = lastChild->right.get();
    } else {
        firstChild = std::move(child);
        lastChild = firstChild.get();
    }
}

int ARGManager::getPhiIndex(LocationTy start, LocationTy end) {
    auto firstInst = end->begin();
    auto result = -1;
    if (firstInst->getOpcode() == 48) { // 48 is phi
        const llvm::PHINode* inst = llvm::dyn_cast<llvm::PHINode>(firstInst);
        auto incomingNum = inst->getNumIncomingValues();
        for (auto j = 0; j < incomingNum; j++) {
            if (inst->getIncomingBlock(j) == start) {
                result = j;
                break;
            }
        }
    }
    return result;
}

}
