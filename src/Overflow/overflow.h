#ifndef CEAGLE_OVERFLOW_H
#define CEAGLE_OVERFLOW_H

#include "SMT/Middleware/AST.h"

#include "Util/cpp-multiprecision/multiprecision/bigint.hpp"

#include <llvm/IR/Module.h>
#include <ostream>
#include <string>

namespace ceagle {

typedef hqythu::bigint::BigInt<256> Bigint;

using namespace smt;

enum class OverflowResult {
    Unknown = 0,
    Overflow,
    NoOverflow
};

std::ostream& operator<<(std::ostream& out, OverflowResult& result);

OverflowResult verifyOverflow(std::string fileName, std::string compilerName);
OverflowResult compilerCheck(std::string fileName, std::string compilerName);
OverflowResult evalCheck(llvm::Module* module);
OverflowResult evilCheck(llvm::Module* module);

class OverflowEvaluator : public smt::Evaluator<Bigint> {
    std::size_t bits;
    Bigint lowerBound;
    Bigint upperBound;
    void checkOverflow(const Bigint& v) {
        if (bits == 0) return;
        std::cout << "checkOverflow" << std::endl;
        std::cout << v.to_hex_string() << std::endl;
        std::cout << upperBound.to_hex_string() << std::endl;
        std::cout << lowerBound.to_hex_string() << std::endl;
        if ( v> upperBound || v < lowerBound) {
            throw std::overflow_error("");
        }
    }
    void calBound() {
        if (bits == 0) return;
        lowerBound = Bigint(- (1 << (bits - 1)));
        upperBound = Bigint((1 << (bits - 1)) - 1);
    }
public:
    virtual Bigint evaluate(Expr expr) override {
        auto sort = expr.sort();
        auto bvsort = static_cast<BvSort*>(&sort);
        bits = bvsort->getSize();
        calBound();
        return Evaluator<Bigint>::evaluate(expr);
    }
    virtual Bigint handleAdd(const Bigint& left, const Bigint& right) {
        return left + right;
    }
    virtual Bigint handleBvAdd(const Bigint& left, const Bigint& right) override {
        auto result = left + right;
        checkOverflow(result);
        return result;
    }
    virtual Bigint handleBvSub(const Bigint& left, const Bigint& right) override {
        auto result = left - right;
        checkOverflow(result);
        return result;
    }
    virtual Bigint handleBvSExt(const Bigint& value, const Bigint& addSize) override {
        bits += addSize.to_int();
        calBound();
        return value;
    }
    virtual Bigint handleBvExtract(const Bigint& value, const Bigint& to, const Bigint& from) override {
        bits = to.to_int() - from.to_int() + 1;
        calBound();
        return value;
    }
    virtual Bigint handleBvSDiv(const Bigint& left, const Bigint& right) override {
        auto result = left / right;
        checkOverflow(result);
        return result;
    }
    virtual Bigint handleVar(VarNode* var) {
         assert(false);
        // assign var to IntMax - 1
//        return INT_MAX;
    }
    virtual Bigint handleIntValue(ValueNode<int>* node) {
        return node->getValue();
    }
    virtual Bigint handleBoolValue(ValueNode<bool>* node) {
        return node->getValue();
    }
    virtual Bigint handleUnsignedLongValue(ValueNode<unsigned long>* node) {
        return node->getValue();
    }
    virtual Bigint handleBit1Value(BitValueNode<1>* node) {
        return node->getValue().to_ulong();
    }
    virtual Bigint handleBit2Value(BitValueNode<2>* node) {
        return node->getValue().to_ulong();
    }
    virtual Bigint handleBit4Value(BitValueNode<4>* node) {
        return node->getValue().to_ulong();
    }
    virtual Bigint handleBit8Value(BitValueNode<8>* node) {
        return node->getValue().to_ulong();
    }
    virtual Bigint handleBit16Value(BitValueNode<16>* node) {
        return node->getValue().to_ulong();
    }
    virtual Bigint handleBit32Value(BitValueNode<32>* node) {
        return int(node->getValue().to_ulong());
    }
    virtual Bigint handleBit64Value(BitValueNode<64>* node) {
        return node->getValue().to_ulong();
    }
};


}

#endif
