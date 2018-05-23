#include <Util/IRUtil.h>
#include "SMT/Middleware/Solver.h"
#include "SMT/Translator/InstTranslator.h"
#include "ValueAnalysis/ValueAnalysis.h"

#include <llvm/IR/Constant.h>

namespace ceagle {

namespace value_analysis {

void ValueAnalysis::start(llvm::Module &module) {
    auto entry = module.getFunction("main")->begin();
    Node init(new ValueState(entry));
    for (auto& gVar: module.globals()) {
        if (auto constantInt = dynamic_cast<llvm::ConstantInt*>(gVar.getInitializer())) {
            Value v;
            auto value = constantInt->getZExtValue();
            if (!constantInt->isNegative()) {
                if (constantInt->isZero()) {
                    v = Value(0);
                }
                v = Value(value);
            } else {
                v = Value(constantInt->getSExtValue());
            }
            init->state[&gVar] = v;
        } else {
            std::cerr << "Error: Value Analysis cannot handle non int type" << std::endl;
        }
    }
    ValueStateWaitList waitList(init);
    expand(waitList, *this, [&](const Node& node) {
        return node->location->getName().find("ERROR") != std::string::npos;
    });
    if (waitList.empty()) {
        dfsResult = DFSResult::DR_SAFE;
    } else {
        dfsResult = DFSResult::DR_UNSAFE;
    }
}

std::vector<Node> ValueAnalysis::next(const Node &item) {
    auto nextLocations = util::getSuccessorsWithPhiSelector(item->location);
    std::vector<Node> result;
    std::map<llvm::Value*, Expr> valueMap;
    // construct current assignment
    std::map<std::string, Value> assignment;
    for (auto & p: item->state) {
        assignment[p.first->getName()] = p.second;
    }
    for (auto & p: nextLocations) {
        auto assignmentCopy = assignment;
        std::map<llvm::Value*, Value> newState = item->state;
        std::unique_ptr<VisitorTranslator> translator(new VisitorTranslator());
        Expr condition = translator->path2SMTM({item->location, p.second}, valueMap, item->phiSelector);
        try {
            for (auto &v : item->stateConstraint(condition)) {
                assert(v.first->hasName());
                assignmentCopy[v.first->getName()] = v.second;
                newState[v.first] = v.second;
            }
        } catch (ValueState::constraint_error) {
            continue;
        }
        ValueEvaluator evaluator(assignmentCopy);
        for (auto &v: valueMap) {
            newState[v.first] = evaluator.evaluate(v.second);
        }
        result.push_back(Node(new ValueState(p.second, newState, p.first)));
    }
    return result;
}

bool ValueAnalysis::stop(const Node &state) const {
    auto location = state->location;
    try {
        auto relatedStates = locationMap.at(location);
        for (auto &relatedState : relatedStates) {
            if (relatedState->cover(*state)) {
                return false;
            }
        }
        return true;
    } catch (std::out_of_range) {
        return false;
    }
}

void ValueAnalysis::add(const Node &from, const Node &to) {
    auto& relatedStates = locationMap[to->location];
    relatedStates.push_back(to);
    parentMap[to] = from;
}

DFSResult ValueAnalysis::getResult() {
    return dfsResult;
}

bool ValueState::operator==(const ValueState &other) const {
    return this->location == other.location && this->state == other.state;
}

ValueState::ValueState(llvm::BasicBlock *location) : location(location) {}

bool ValueState::cover(const ValueState &other) const {
    if (this->location != other.location) return false;
    if (this->phiSelector != other.phiSelector) return false;
    for (auto& p: other.state) {
        auto& var = p.first;
        auto& otherValue = p.second;
        if (this->state.count(var) > 0) {
            auto& thisValue = this->state.at(var);
            if (otherValue.init) {
                if (thisValue.init) {
                    if (!thisValue.cover(otherValue)) {
                        return false;
                    }
                }
            } else {
                if (thisValue.init) {
                    // other is non det, this is not
                    return false;
                }
            }
        }
    }
    return true;
}

std::map<llvm::Value *, Value> ValueState::stateConstraint(Expr condition) const throw(struct constraint_error) {
    std::set<std::string> vars(condition.vars());
    auto solver = smt::makeSolver();
    solver->add(condition);
    std::vector<llvm::Value *> constraintVars;
    for (auto& p: this->state) {
        if (vars.count(p.first->getName()) > 0) {
            constraintVars.push_back(p.first);
            auto expr = p.second.toExpr(Var(p.first->getName(), IntSort()));
            solver->add(expr);
        }
    }
    auto result = solver->checkSat();
    auto newState = state;
    if (result == CheckResult::UNSAT) {
        throw constraint_error{};
    } else if (result == CheckResult::UNKNOWN) {
        std::cerr << "Warning: Value Analysis check sat unknown." << std::endl;
    } else {
        auto model = solver->getModel();
        for (auto & var: constraintVars) {
            newState[var] = Value(std::stol(model[var->getName()]));
            solver->add( !newState[var].toExpr(Var(var->getName(), IntSort())));
        }
        result = solver->checkSat();
        if (result == CheckResult::SAT) {
            std::cerr << "Warning: Value Analysis may not complete." << std::endl;
        }
    }
    return newState;
}

ValueState::ValueState(llvm::BasicBlock *location, const std::map<llvm::Value *, Value> &state, int phiSelector)
        : location(location), state(state), phiSelector(phiSelector) {}

bool Value::operator==(const Value &other) const {
    return (this->init == other.init && this->includes == other.includes);
}

bool Value::cover(const Value &other) const {
    return std::includes(this->includes.begin(), this->includes.end(), other.includes.begin(), other.includes.end());
}

Expr Value::toExpr(Var var) const {
    Expr result = BoolValue(true);
    for (auto i: includes) {
        result = result || (var == IntValue(i));
    }
}

Value ValueEvaluator::handleBoolValue(ValueNode<bool> *node) {
    auto value = node->getValue();
    return Value(value ? 1 : 0);
}

Value ValueEvaluator::handleBit1Value(BitValueNode<1> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit2Value(BitValueNode<2> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit4Value(BitValueNode<4> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit8Value(BitValueNode<8> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit16Value(BitValueNode<16> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit32Value(BitValueNode<32> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleBit64Value(BitValueNode<64> *node) {
    auto value = (node->getValue().to_ulong());
    return Value(*reinterpret_cast<long *>(& value));
}

Value ValueEvaluator::handleVar(VarNode *var) {
    auto name = var->name();
    return assignment[name];
}

Value ValueEvaluator::handleIntValue(ValueNode<int> *node) {
    return Value(node->getValue());
}

Value ValueEvaluator::handleBvAdd(const Value &left, const Value &right) {
    Value result;
    for (auto& l: left.includes) {
        for (auto& r: right.includes) {
            result.includes.insert(l + r);
        }
    }
    return result;
}

Value ValueEvaluator::handleBvSub(const Value &left, const Value &right) {
    Value result;
    for (auto& l: left.includes) {
        for (auto& r: right.includes) {
            result.includes.insert(l - r);
        }
    }
    return result;
}
} // end namespace value analysis

} // end namespace ceagle
