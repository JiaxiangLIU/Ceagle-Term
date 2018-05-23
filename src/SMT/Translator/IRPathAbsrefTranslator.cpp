#include "IRPathAbsrefTranslator.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

using namespace llvm;
using namespace ceagle::smt;

namespace ceagle {

Expr IRPathAbsrefTranslator::getExpr(const Value* val) {
	if (const Instruction* inst = dyn_cast<Instruction>(val)) {
		Type* type = inst->getType();
		while (type->isPointerTy()) {
			type = type->getPointerElementType();
		}

		Sort sort;
		if (type->isIntegerTy(1)) {
			sort = BoolSort();
		} else if (type->isIntegerTy()) {
			sort = BvSort(type->getIntegerBitWidth());
		} else if (type->isFloatingPointTy()) {
			throw("float");
		} else if (type->isArrayTy()) {
			if (type->getArrayElementType()->isIntegerTy(1)) {
				throw("array i1");
			} else if (type->getArrayElementType()->isIntegerTy()) {
				sort = ArraySort(BvSort(type->getArrayElementType()->getIntegerBitWidth()), BvSort(32));
			} else {
				throw("array");
			}
		} else {
			throw("unknown type");
		}

		return Var(val->getName(), sort);
	} else if (const ConstantInt* ci = dyn_cast<ConstantInt>(val)) {
		if (ci->getType()->isIntegerTy(1)) {
			if (ci->getZExtValue() == 0) {
				return BoolValue(0);
			} else {
				return BoolValue(1);
			}
		} else {
			return BitValue(ci->getZExtValue(), ci->getType()->getIntegerBitWidth());
		}
	} else {
		throw("value");
	}
}

Expr IRPathAbsrefTranslator::path2SMTM(const Module& g, const std::vector<const BasicBlock*> path, int firstPHISelector) {
	Expr expr = BoolValue(1);
	for (int i = 0; i < path.size(); ++ i) {
		for (BasicBlock::const_iterator bi = path[i]->begin(), be = path[i]->end(); bi != be; ++ bi) {
			switch (bi->getOpcode()) {
				// ret
				case 1:
				// br
				case 2:
				// switch
				case 3:
				// indiretbr
				case 4:
				// invoke
				case 5:
				// resume
				case 6:
				// unreachable
				case 7:
					break;
                // add
                case 8:
                    expr = expr && (getExpr(bi) == bvadd(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // fadd
                case 9:
                    break;
                // sub
                case 10:
                    expr = expr && (getExpr(bi) == bvsub(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // fsub
                case 11:
                    break;
                // mul
                case 12:
                    expr = expr && (getExpr(bi) == bvmul(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // fmuladd
                case 13:
                    break;
                // udiv
                case 14:
                    expr = expr && (getExpr(bi) == bvudiv(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // sdiv
                case 15:
                    expr = expr && (getExpr(bi) == bvsdiv(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // fdiv
                case 16:
                    break;
                // urem
                case 17:
                    expr = expr && (getExpr(bi) == bvurem(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // srem
                case 18:
                    expr = expr && (getExpr(bi) == bvsrem(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // frem
                case 19:
                    break;
                // shl
                case 20:
                    expr = expr && (getExpr(bi) == bvshl(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // lshr
                case 21:
                    expr = expr && (getExpr(bi) == bvlshr(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // ashr
                case 22:
                    expr = expr && (getExpr(bi) == bvashr(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // and
                case 23:
                    expr = expr && (getExpr(bi) == bvadd(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // or
                case 24:
                    expr = expr && (getExpr(bi) == bvor(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // xor
                case 25:
                    expr = expr && (getExpr(bi) == bvxor(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                    break;
                // alloca
                case 26:
                    break;
                // load
                case 27:
                    expr = expr && (getExpr(bi) == getExpr(bi->getOperand(0)));
                    break;
                // store
                case 28:
                    expr = expr && (getExpr(bi->getOperand(1)) == getExpr(bi->getOperand(0)));
                    break;
                // getelementptr
                case 29: {
                    const ConstantInt* ci = dyn_cast<ConstantInt>(bi->getOperand(1));
                    if (bi->getNumOperands() == 3 && ci && ci->getZExtValue() == 0) {
                        expr = expr && (getExpr(bi) == array_select(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(2))));
                    } else {
                        throw("gep");
                    }
                    break;
                }
                // fence
                case 30:
                // atomiccmpxchg
                case 31:
                // atomicrmw
                case 32:
                    break;
                // trunc
                case 33: {
                    Type* srcTy = dyn_cast<CastInst>(bi)->getSrcTy();
                    Type* destTy = dyn_cast<CastInst>(bi)->getDestTy();
                    Expr tmpExpr = extract(getExpr(bi->getOperand(0)), srcTy->getIntegerBitWidth(), destTy->getIntegerBitWidth());
                    if (destTy->isIntegerTy(1)) {
                        tmpExpr = (tmpExpr == BitValue(1, 1));
                    }
                    expr = expr && (getExpr(bi) == tmpExpr);
                    break;
                }
                // zext
                case 34: {
                    Type* srcTy = dyn_cast<CastInst>(bi)->getSrcTy();
                    Type* destTy = dyn_cast<CastInst>(bi)->getDestTy();
                    Expr tmpExpr = getExpr(bi->getOperand(0));
                    if (srcTy->isIntegerTy(1)) {
                        tmpExpr = ite(tmpExpr, BitValue(1, 1), BitValue(0, 1));
                    }
                    tmpExpr = zext(tmpExpr, srcTy->getIntegerBitWidth(), destTy->getIntegerBitWidth());
                    expr = expr && (getExpr(bi) == tmpExpr);
                    break;
                }
                // sext
                case 35: {
                    Type* srcTy = dyn_cast<CastInst>(bi)->getSrcTy();
                    Type* destTy = dyn_cast<CastInst>(bi)->getDestTy();
                    Expr tmpExpr = getExpr(bi->getOperand(0));
                    if (srcTy->isIntegerTy(1)) {
                        tmpExpr = ite(tmpExpr, BitValue(1, 1), BitValue(0, 1));
                    }
                    tmpExpr = sext(tmpExpr, srcTy->getIntegerBitWidth(), destTy->getIntegerBitWidth());
                    expr = expr && (getExpr(bi) == tmpExpr);
                    break;
                }
                // fptoui
                case 36:
                // fptosi
                case 37:
                // uitofp
                case 38:
                // sitofp
                case 39:
                // fptrunc
                case 40:
                // fpext
                case 41:
                // ptrtoint
                case 42:
                // inttoptr
                case 43:
                // bitcast
                case 44:
                // addrspacecast
                case 45:
                    break;
                // icmp
                case 46:
                    switch (dyn_cast<CmpInst>(bi)->getPredicate()) {
                        case CmpInst::ICMP_EQ:
                            expr = expr && (getExpr(bi) == (getExpr(bi->getOperand(0)) == getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_NE:
                            expr = expr && (getExpr(bi) == !(getExpr(bi->getOperand(0)) == getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_UGT:
                            expr = expr && (getExpr(bi) == bvugt(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_SGT:
                            expr = expr && (getExpr(bi) == bvsgt(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_ULT:
                            expr = expr && (getExpr(bi) == bvult(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_SLT:
                            expr = expr && (getExpr(bi) == bvslt(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_UGE:
                            expr = expr && (getExpr(bi) == bvuge(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_SGE:
                            expr = expr && (getExpr(bi) == bvsge(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_ULE:
                            expr = expr && (getExpr(bi) == bvule(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        case CmpInst::ICMP_SLE:
                            expr = expr && (getExpr(bi) == bvsle(getExpr(bi->getOperand(0)), getExpr(bi->getOperand(1))));
                            break;
                        default:
                            break;
                    }
                    break;
                // fcmp
                case 47:
                    break;
                // phi
                case 48: {
                    const PHINode* inst = dyn_cast<PHINode>(bi);
                    if (i > 0) {
                        expr = expr && (getExpr(bi) == getExpr(inst->getIncomingValueForBlock(path[i - 1])));
                    } else {
                        expr = expr && (getExpr(bi) == getExpr(inst->getIncomingValue(firstPHISelector)));
                    }
                    break;
                }
                // call
                case 49:
                    break;
                // select
                case 50:
                // userop1
                case 51:
                // userop2
                case 52:
                // vaarg
                case 53:
                // extractelement
                case 54:
                // insertelement
                case 55:
                // shufflevector
                case 56:
                // extractvalue
                case 57:
                // insertvalue
                case 58:
                // landingpad
                case 59:
                    break;
			}
		}
	}
	return expr;
}

}
