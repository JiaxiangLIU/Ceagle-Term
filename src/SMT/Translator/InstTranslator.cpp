#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

#include "SMT/Translator/InstTranslator.h"

#include <exception>
#include <cmath>

using namespace llvm;
using std::exception;
using std::pow;

namespace ceagle {

class TranslateException : exception {
    std::string _msg;
public:
    TranslateException(std::string msg) {
        _msg = msg;
    }
    TranslateException(std::string msg, Value* value) {
        std::string valStr;
        if (value) {
            raw_string_ostream rso(valStr);
            value->print(rso);
        }
        _msg = msg + valStr;
    }
    TranslateException(std::string msg, Type* type) {
        std::string valStr;
        if (type) {
            raw_string_ostream rso(valStr);
            type->print(rso);
        }
        _msg = msg + valStr;
    }
    virtual const char* what() const noexcept override {
        return _msg.c_str();
    }
};

namespace smt {

Expr operator+(Expr left, Expr right);
Expr operator-(Expr left, Expr right);
Expr operator*(Expr left, Expr right);
Expr operator/(Expr left, Expr right);
Expr rem(Expr left, Expr right);
Expr bvadd(Expr left, Expr right);
Expr bvsub(Expr left, Expr right);
Expr bvmul(Expr left, Expr right);
Expr bvudiv(Expr left, Expr right);
Expr bvsdiv(Expr left, Expr right);
Expr bvurem(Expr left, Expr right);
Expr bvsrem(Expr left, Expr right);
Expr bvshl(Expr left, Expr right);
Expr bvlshr(Expr left, Expr right);
Expr bvashr(Expr left, Expr right);
Expr bvand(Expr left, Expr right);
Expr bvor(Expr left, Expr right);
Expr bvxor(Expr left, Expr right);


Expr operator>(Expr left, Expr right);
Expr operator>=(Expr left, Expr right);
Expr operator<(Expr left, Expr right);
Expr operator<=(Expr left, Expr right);
Expr bvugt(Expr left, Expr right);
Expr bvuge(Expr left, Expr right);
Expr bvult(Expr left, Expr right);
Expr bvule(Expr left, Expr right);
Expr bvsgt(Expr left, Expr right);
Expr bvsge(Expr left, Expr right);
Expr bvslt(Expr left, Expr right);
Expr bvsle(Expr left, Expr right);
}

Sort TranslatorVisitor::typeToSort(llvm::Type* type) {
    auto typeID = type->getTypeID();
    switch(typeID) {
        default:
            throw TranslateException("Unsupported type: ", type);
        case Type::IntegerTyID: {
            ;
            IntegerType* integerType = dyn_cast<IntegerType>(type);
            auto bits = integerType->getBitWidth();
            if (bits == 1) {
                return BoolSort();
            }
            if (encodeIntAsBv) {
                return BvSort(bits);
            } else {
                return IntSort();
            }
        }
    }
}

Expr TranslatorVisitor::constantToExpr(Constant* constant) {
    if (auto constantInt = dynamic_cast<ConstantInt*>(constant)) {
        auto bits = constantInt->getBitWidth();
        auto value = constantInt->getZExtValue();
        if (bits == 1) {
            if (value) {
                return BoolValue(true);
            } else {
                return BoolValue(false);
            }
        }
        if (encodeIntAsBv) {
            return BitValue(value, bits);
        } else {
            if (!constantInt->isNegative()) {
                if (constantInt->isZero()) {
                    return IntValue(0);
                }
                return IntValue(value);
            } else {
                if (constantInt->isZero()) {
                    return IntValue(0);
                }
                if (bits == 64) {
                    return IntValue(constantInt->getSExtValue());
                } else if (bits == 32) {
                    return IntValue(*reinterpret_cast<int32_t *>(&value));
                } else if (bits == 16) {
                    return IntValue(*reinterpret_cast<int16_t *>(&value));
                } else if (bits == 8) {
                    return IntValue(*reinterpret_cast<int8_t *>(&value));
                } else if (bits == 4) {
                    return IntValue(*reinterpret_cast<char *>(&value));
                } else if (bits == 2) {
                    return IntValue(-1);
                }
            }
        }
    } else {
        throw TranslateException("Unsupport constant type: ", constant);
    }
}

#if 0
Expr InstTranslator::constantToValue(Constant* constant) {
    return constantToExpr(constant);
}

Expr InstTranslator::translate(Value* value) {
    if (value->hasName()) {
        auto name = value->getName();
        auto type = value->getType();
        auto sort = typeToSort(type);
        return Var(name, sort);
    }
    if (auto inst = dyn_cast<Instruction>(value)) {
        return translate(inst);
    } else if (auto constant = dyn_cast<Constant>(value)) {
        return constantToValue(constant);
    }
}

Expr InstTranslator::translate(Instruction* inst) {
    switch (inst->getOpcode()) {
        case Instruction::Alloca: {
            AllocaInst* alloca = dyn_cast<AllocaInst>(inst);
            auto name = alloca->getName();
            auto type = alloca->getAllocatedType();
            auto sort = typeToSort(type);
            return Var(name, sort);
        }
        case Instruction::Store: {
            StoreInst* storeInst = dyn_cast<StoreInst>(inst);
            auto valueOperand = storeInst->getValueOperand();
            auto pointerOperand = storeInst->getPointerOperand();
            auto result = translate(valueOperand);
            valueMap[pointerOperand->getName()] = result;
            return result;
        }
        case Instruction::Ret:
        case Instruction::Call: {
            return Expr();
        }
        case Instruction::Load: {
            auto loadInst = dyn_cast<LoadInst>(inst);
            auto pointerOperand = loadInst->getPointerOperand();
            return translate(dyn_cast<Instruction>(pointerOperand));
        }
        case Instruction::Add: {
            auto op1 = inst->getOperand(0);
            auto op2 = inst->getOperand(1);
            auto result = bvadd(translate(op1), translate(op2));
            if (inst->hasName()) {
                valueMap[inst->getName()] = result;
            }
            return result;
        }
    }
    assert(false && "Unhandled opcode in " && __func__);
}
#endif

Expr TranslatorVisitor::get(std::string name) {
    return valueMap.at(nameMap.at(name));
}

Expr TranslatorVisitor::get(Value* value) {
    return valueMap.at(value);
}

void TranslatorVisitor::set(llvm::Value* var, const Expr& value) {
    if(var->hasName()) {
        nameMap[var->getName()] = var;
    }
    valueMap[var] = value;
}

void TranslatorVisitor::createVar(Instruction* inst, Type* type) {
    if (valueMap.count(inst) == 0) {
        if (!inst->hasName()) {
            throw TranslateException("Cannot create var without type", inst);
        }
        auto s = typeToSort(type);
        auto name = inst->getName().str();
        if (isa<CallInst>(inst)) {
            if (name == "__VERIFIER_nondet_int") {
                name = name + std::to_string(nonDetCount);
                nonDetCount ++;
            }
        }
        Var var(name, s);
        if (s.type() == SortType::Int) {
            auto integerType = dyn_cast<IntegerType>(type);
            auto size = integerType->getBitWidth();
                rangeConstraint = rangeConstraint && (var >= IntValue(- (1 << (size - 1)))) && (var <= IntValue((1 << (size - 1)) - 1));
        }
        valueMap[inst] = var;
        nameMap[name] = inst;
    }
}

void TranslatorVisitor::createVar(Instruction* inst) {
    auto type = inst->getType();
    return createVar(inst, type);
}

void TranslatorVisitor::createPointerVar(Instruction* inst) {
    auto type = inst->getType();
    if (auto pointerType = dyn_cast<PointerType>(type)) {
        return createVar(inst, pointerType->getElementType());
    }
    throw TranslateException("Cannot create pointer var for non pointer type: ", inst);
}

void TranslatorVisitor::visitStoreInst(StoreInst &I) {
    auto valueOp = I.getValueOperand();
    auto pointerOp = I.getPointerOperand();
    if (!isa<Instruction>(pointerOp)) {
        throw TranslateException("Store to none Instruction: ", &I);
    }
    auto var = dyn_cast<Instruction>(pointerOp);

    if (auto constant = dynamic_cast<Constant*>(valueOp)) {
        Expr value = constantToExpr(constant);
        set(var, value);
    } else if (auto inst = dynamic_cast<Instruction*>(valueOp)) {
        createVar(inst);
        set(var, valueMap[inst]);
    } else {
        throw TranslateException("Unexpected store value", valueOp);
    }
}

void TranslatorVisitor::visitLoadInst (LoadInst &I) {
    auto pointerOp = I.getPointerOperand();
    if (auto inst = dyn_cast<Instruction>(pointerOp)) {
        createPointerVar(inst);
        set(&I, valueMap[inst]);
    } else {
        throw TranslateException("Load from non-instruction: ", &I);
    }
}

void TranslatorVisitor::visitAllocaInst (AllocaInst &I) {
    createPointerVar(&I);
}

void TranslatorVisitor::visitICmpInst (ICmpInst &I) {
    auto op1 = I.getOperand(0);
    auto op2 = I.getOperand(1);
    if (auto constant = dyn_cast<Constant>(op1)) {
        set(op1, constantToExpr(constant));
    }
    if (auto constant = dyn_cast<Constant>(op2)) {
        set(op2, constantToExpr(constant));
    }
    std::map<CmpInst::Predicate, decltype(&bvugt)> predicateToFunc = {
        {CmpInst::ICMP_UGT, bvugt},
        {CmpInst::ICMP_UGE, bvuge},
        {CmpInst::ICMP_ULT, bvult},
        {CmpInst::ICMP_ULE, bvule},
        {CmpInst::ICMP_SGT, bvsgt},
        {CmpInst::ICMP_SGE, bvsge},
        {CmpInst::ICMP_SLT, bvslt},
        {CmpInst::ICMP_SLE, bvsle}
    };
    std::map<CmpInst::Predicate, decltype(&ceagle::smt::operator>)> predicateToFuncInt {
        {CmpInst::ICMP_UGT, ceagle::smt::operator>},
        {CmpInst::ICMP_UGE, ceagle::smt::operator>=},
        {CmpInst::ICMP_ULT, ceagle::smt::operator<},
        {CmpInst::ICMP_ULT, ceagle::smt::operator<=},
        {CmpInst::ICMP_SGT, ceagle::smt::operator>},
        {CmpInst::ICMP_SGE, ceagle::smt::operator>=},
        {CmpInst::ICMP_SLT, ceagle::smt::operator<},
        {CmpInst::ICMP_SLT, ceagle::smt::operator<=},
    };
    auto predicate = I.getPredicate();
    switch (predicate) {
        default:
            throw TranslateException("Invalid predicate: ", &I);
        case CmpInst::ICMP_NE:
            set(&I, !(get(op1) == get(op2)));
            break;
        case CmpInst::ICMP_EQ:
            set(&I, get(op1) == get(op2));
            break;
        case CmpInst::ICMP_UGT:
        case CmpInst::ICMP_UGE:
        case CmpInst::ICMP_ULT:
        case CmpInst::ICMP_ULE:
        case CmpInst::ICMP_SGT:
        case CmpInst::ICMP_SGE:
        case CmpInst::ICMP_SLT:
        case CmpInst::ICMP_SLE:
            if (encodeIntAsBv) {
                set(&I, predicateToFunc.at(predicate)(get(op1), get(op2)));
            } else {
                set(&I, predicateToFuncInt.at(predicate)(get(op1), get(op2)));
            }
            break;
    }
}

void TranslatorVisitor::visitSExtInst (SExtInst &I) {
    if (I.getNumUses() == 0) return;
    auto from = dyn_cast<IntegerType>(I.getSrcTy())->getBitWidth();
    auto to = dyn_cast<IntegerType>(I.getDestTy())->getBitWidth();
    auto op1 = I.getOperand(0);
    Expr expr;
    if (encodeIntAsBv) {
        auto origin = get(op1);
        if (from == 1) {
            // origin is bool
            origin = ite(origin, BitValue(1, 1), BitValue(0, 1));
        }
        expr = sext(origin, from, to);
    } else {
        expr = ite(get(op1) >=IntValue(0), get(op1), get(op1) * IntValue(pow(2, to - from)));
    }
    set(&I, expr);
}

void TranslatorVisitor::visitZExtInst (ZExtInst &I) {
    if (I.getNumUses() == 0) return;
    auto from = dyn_cast<IntegerType>(I.getSrcTy())->getBitWidth();
    auto to = dyn_cast<IntegerType>(I.getDestTy())->getBitWidth();
    auto op1 = I.getOperand(0);
    Expr expr;
    if (encodeIntAsBv) {
        auto origin = get(op1);
        if (from == 1) {
            // origin is bool
            origin = ite(origin, BitValue(1, 1), BitValue(0, 1));
        }
        expr = sext(origin, from, to);
    } else {
        expr = get(op1);
    }
    set(&I, expr);
}

void TranslatorVisitor::visitCallInst (llvm::CallInst &I) {
    if (I.getNumUses() == 0) return;
    auto func = I.getCalledFunction();
    auto funcName = func->getName();
    if (funcName.startswith("__VERIFIER_nondet_")) {
        createVar(&I);
    }
}
void TranslatorVisitor::visitTruncInst (llvm::TruncInst &I) {
    auto from = dyn_cast<IntegerType>(I.getSrcTy())->getBitWidth();
    auto to = dyn_cast<IntegerType>(I.getDestTy())->getBitWidth();
    auto op1 = I.getOperand(0);
    Expr result;
    if (encodeIntAsBv) {
        result = extract(get(op1), from, to);
        if (to == 1) {
            result = result == BitValue(1, 1);
        }
    } else {
        result = rem(get(op1), IntValue(pow(2, to)));
        if (to == 1) {
            result = result == IntValue(1);
        }
    }
    set(&I, result);
}

void TranslatorVisitor::visitBinaryOperator (BinaryOperator &I) {
    auto op1 = I.getOperand(0);
    auto op2 = I.getOperand(1);
    if (auto constant = dyn_cast<Constant>(op1)) {
        set(op1, constantToExpr(constant));
    }
    if (auto constant = dyn_cast<Constant>(op2)) {
        set(op2, constantToExpr(constant));
    }
    auto opcode = I.getOpcode();
    std::map<Instruction::BinaryOps, decltype(&bvadd)> codeToFunc = {
        {Instruction::Add, bvadd},
        {Instruction::Sub, bvsub},
        {Instruction::Mul, bvmul},
        {Instruction::UDiv, bvudiv},
        {Instruction::SDiv, bvsdiv},
        {Instruction::URem, bvurem},
        {Instruction::SRem, bvsrem},
        {Instruction::Shl, bvshl},
        {Instruction::LShr, bvlshr},
        {Instruction::AShr, bvashr},
        {Instruction::And, bvand},
        {Instruction::Or, bvor},
        {Instruction::Xor, bvxor}
    };
    std::map<Instruction::BinaryOps, decltype(&ceagle::smt::operator+)> codeToFuncInt = {
        {Instruction::Add, ceagle::smt::operator+},
        {Instruction::Sub, ceagle::smt::operator-},
        {Instruction::Mul, ceagle::smt::operator*},
        {Instruction::UDiv, ceagle::smt::operator/},
        {Instruction::SDiv, ceagle::smt::operator/},
        {Instruction::URem, ceagle::smt::rem},
        {Instruction::SRem, ceagle::smt::rem},
        //{Instruction::Shl, bvshl},
        //{Instruction::LShr, bvlshr},
        //{Instruction::AShr, bvashr},
        //{Instruction::And, bvand},
        //{Instruction::Or, bvor},
        //{Instruction::Xor, bvxor}
    };
    if (encodeIntAsBv) {
        set(&I, codeToFunc.at(opcode)(get(op1), get(op2)));
    } else {
        set(&I, codeToFuncInt.at(opcode)(get(op1), get(op2)));
    }
}

void TranslatorVisitor::visitBranchInst (BranchInst &I) {
    if (I.isConditional()) {
        br = get(I.getCondition());
    } else {
        br = BoolValue(true);
    }
}

void TranslatorVisitor::visitPHINode (PHINode &I) {
    if (phiSelector == -1) {
        throw TranslateException("Phi selector is -1: ", &I);
    }
    auto value = I.getIncomingValue(phiSelector);
    if (auto constant = dyn_cast<Constant>(value)) {
        set(&I, constantToExpr(constant));
    } else if (auto inst = dyn_cast<Instruction>(value)) {
        createVar(inst);
        set(&I, get(inst));
    } else {
        throw TranslateException("Phi select non constant non instruction", &I);
    }
}

Expr VisitorTranslator::path2SMTM(std::vector<BasicBlock*> path, int firstPHISelector) {
    std::map<Value*, Expr> valueExprMap;
    return path2SMTM(path, valueExprMap, firstPHISelector);
}

Expr VisitorTranslator::path2SMTM(std::vector<BasicBlock*> path, std::map<Value*, Expr>& valueExprMap, int firstPHISelector) {
    if (path.size() < 2) return BoolValue(true);
    auto it = path.begin();
    auto et = path.end();
    Expr condition = BoolValue(true);
    while ((it + 1) != et) {
        auto current = const_cast<BasicBlock*>(*it);
        ++it;
        visitor.setPhiSelector(firstPHISelector);
        visitor.visit(current);
        auto br = visitor.getCondition();
        auto rangeConstraint = visitor.getRangeConstraint();
        auto& lastInst = current->back();
        if (auto brInst = dyn_cast<BranchInst>(&lastInst)) {
            if (brInst->getSuccessor(0) == *it) {
                condition = condition && br;
            } else if (brInst->getSuccessor(1) == *it) {
                condition = condition && !br;
            } else {
                throw TranslateException("Not an edge from: ", current);
            }
            if (rangeConstraint.str() != "true") {
                condition = condition && rangeConstraint;
            }
        } else {
            throw TranslateException("Block end without br: ", current);
        }
        auto& nextFirstInst = (*it)->front();
        if (auto phi = dyn_cast<PHINode>(&nextFirstInst)) {
            // TODO we only handle first phi here
            firstPHISelector = phi->getBasicBlockIndex(current);
        } else {
            firstPHISelector = -1;
        }
    }
    valueExprMap = visitor.getValueMap();
    return condition;
}

}
