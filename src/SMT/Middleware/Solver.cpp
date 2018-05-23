#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"
#include <cassert>

namespace ceagle {
namespace smt {

void RawSolver::processVar(const Expr& expr, std::function<void(const std::string&, const Sort&)>& handle) {
    this->processVar(*expr._root, handle);
}

void RawSolver::processVar(const inode<Node>& node, std::function<void(const std::string&, const Sort&)>& handle) {
    if (node.isLeaf()) {
        if(auto var = dynamic_cast<const VarNode*>(node.getValue())) {
            if (this->vars.count(var->name()) == 0) {
                // first time
                this->vars.insert(std::pair<std::string, Sort>(var->name(), var->getSort()));
                this->push_pop.back().push_back(var->name());
                handle(var->name(), var->getSort());
            } else {
            }
        }
    } else {
        auto child = node.getChild();
        do {
            processVar(*child, handle);
        } while((child = child->getNext()) != nullptr);
    }
}

void RawSolver::push() {
    this->push_pop.push_back(std::vector<std::string>());
}

void RawSolver::pop() {
    assert("cannot call pop before push" && this->push_pop.size() > 0);
    auto& last = this->push_pop.back();
    for(const auto& item: last) {
        this->vars.erase(item);
    }
    push_pop.pop_back();
}

void RawSolver::reset() {
    vars.clear();
    push_pop.clear();
    push_pop.push_back({});
}

std::shared_ptr<Solver> makeSolver(bool interp) {
    return std::shared_ptr<Solver>(new Z3RawSolver(interp));
}

}
}
