#ifndef CEAGLE_VALUE_ANALYSIS_H
#define CEAGLE_VALUE_ANALYSIS_H

#include <map>
#include <memory>
#include <functional>
#include <set>

#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include "Abstractor.h"
#include "Analysis.h"
#include "ARG.h"
#include "QueueWaitList.h"

#include "SMT/Middleware/AST.h"

namespace ceagle {

using smt::BitValueNode;
using smt::VarNode;
using smt::ValueNode;
using smt::Expr;
using smt::Var;

namespace value_analysis {

struct Value {
    bool init = false;
    std::set<long> includes;

    bool operator==(const Value& other) const;

    bool cover(const Value& other) const;

    Expr toExpr(Var var) const;

    Value() {}
    Value(long one) : init(true), includes{one} {
    }
};

struct ValueState {
    llvm::BasicBlock* location;
    std::map<llvm::Value*, Value> state;
    int phiSelector = -1;
    bool operator==(const ValueState& other) const;

    ValueState(llvm::BasicBlock *location);

    ValueState(llvm::BasicBlock *location, const std::map<llvm::Value *, Value> &state, int phiSelector);

    ValueState() : ValueState(nullptr) {}

    bool cover(const ValueState& other) const;

    struct constraint_error {};

    std::map<llvm::Value*, Value> stateConstraint(Expr condition) const throw(constraint_error);
};

class ValueEvaluator : public smt::Evaluator<Value> {
    std::map<std::string, Value>& assignment;
public:
    ValueEvaluator(std::map<std::string, Value>& _assignment) : assignment(_assignment) {}

    Value handleBoolValue(ValueNode<bool> *node) override;

    Value handleBit1Value(BitValueNode<1> *node) override;

    Value handleBit2Value(BitValueNode<2> *node) override;

    Value handleBit4Value(BitValueNode<4> *node) override;

    Value handleBit8Value(BitValueNode<8> *node) override;

    Value handleBit16Value(BitValueNode<16> *node) override;

    Value handleBit32Value(BitValueNode<32> *node) override;

    Value handleBit64Value(BitValueNode<64> *node) override;

    Value handleIntValue(ValueNode<int> *node) override;

    Value handleVar(VarNode *var) override;

    Value handleBvAdd(const Value &left, const Value &right) override;

    Value handleBvSub(const Value &left, const Value &right) override;
};

typedef std::shared_ptr<ValueState> Node;

class ValueStateWaitList : public QueueWaitList<Node> {
public:
    using QueueWaitList<Node>::QueueWaitList;
};

class ValueAnalysis : public Analysis, public ARG<ValueAnalysis, ValueStateWaitList, Node>, public Abstractor<Node> {
    std::map<llvm::BasicBlock*, std::vector<Node>> locationMap;
    std::map<Node, Node> parentMap;
    DFSResult dfsResult = DFSResult::DR_UNKNOWN;
public:
    virtual void start(llvm::Module& module) override;

    std::vector<Node> next(const Node &item) override;

    bool stop(const Node &state) const override;

    void add(const Node &from, const Node &to) override;

    DFSResult getResult() override;
}; // class ValueAnalysis

} // namespace value analysis

} // namespace ceagle

#endif
