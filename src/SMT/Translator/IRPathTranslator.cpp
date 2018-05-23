#include "IRPathTranslator.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FormattedStream.h"

#include <fstream>

#define PRINT_PATH 0

using namespace llvm;
using namespace ceagle::mm;
using namespace ceagle::smt;
using std::string;
using std::ofstream;
using std::endl;

namespace ceagle {

Expr* IRPathTranslator::createExpr(Type* type, string name) {
	if (name == "") {
		++ cnt;
		string name = "vtmp" + to_string(cnt);
	}
	
	if (type->isPointerTy()) {
		type = dyn_cast<PointerType>(type)->getElementType();
	}
	
	if (IntegerType* intTy = dyn_cast<IntegerType>(type)) {
		unsigned bits = intTy->getBitWidth();
		if (bits == 1) {
			return new Var(name, BoolSort());
		} else {
			return new Var(name, BvSort(bits));
		}
	} else if (type->isFloatTy()) {
		return new Var(name, FloatSort(8, 24));
	} else if (type->isDoubleTy()) {
		return new Var(name, FloatSort(11, 53));
	} else if (type->isX86_FP80Ty()) {
		return new Var(name, FloatSort(15, 65));
	} else if (type->isArrayTy()) {
		type = type->getArrayElementType();
		Sort sort;
		int depth = 1;
		while (1) {
			if (type->isArrayTy()) {
				type = type->getArrayElementType();
				++ depth;
			} else if (type->isIntegerTy() || type->isFloatingPointTy()) {
				if (type->isIntegerTy(1)) {
					sort = BoolSort();
				} else if (type->isIntegerTy()) {
					sort = BvSort(dyn_cast<IntegerType>(type)->getBitWidth());
				} else if (type->isFloatTy()) {
					sort = FloatSort(8, 24);
				} else if (type->isDoubleTy()) {
					sort = FloatSort(11, 53);
				} else if (type->isX86_FP80Ty()) {
					sort = FloatSort(15, 65);
				}
				break;
			} else {
				throwStrForValue("[IRPathTranslator.cpp createExpr] Unsupported element type of array: ", type);
			}
		}
		for (int i = 0; i < depth; ++ i) {
			sort = ArraySort(sort, BvSort(32));
		}
		return new Var(name, sort);
	} else {
		throwStrForValue("[IRPathTranslator.cpp createExpr] Unknown type: ", type);
		return 0;
	}
}

void IRPathTranslator::allocaVar(const Value* val, const Constant* initVal) {
	if (!dyn_cast<PointerType>(val->getType())) {
		throwStrForValue("[IRPathTranslator.cpp allocaVar] Call allocaVar with non-pointer value: ", val);
	}
	
	PointerVar* pointerVar = 0;
	MemVar* memVar = 0;
	
	if (initVal && !dyn_cast<UndefValue>(initVal)) {
		calculateValue(initVal, val->getName());
		memVar = value2MemVar[initVal];
	} else {
		Type* type = dyn_cast<PointerType>(val->getType())->getElementType();
		if (type->isPointerTy()) {
			memVar = new PointerVar(0);
		} else if (type->isStructTy()) {
			// throwStrForValue("[IRPathTranslator.cpp allocaVar] Cannot declare variable for struct.");
            const Constant* emptySt = UndefValue::get(type);
            int size = type->getStructNumElements();
            StructVar* stVar = new StructVar(size);
            for (int i = 0; i < size; ++ i) {
                Constant* ele = emptySt->getAggregateElement(i);
                calculateValue(ele, val->getName());
                stVar->elements[i] = value2MemVar[ele];
            }
            memVar = stVar;
		} else if (type->isIntegerTy() || type->isFloatingPointTy() || type->isArrayTy()) {
			memVar = new DataVar(createExpr(val->getType(), val->getName()));
		} else {
			throwStrForValue("[IRPathTranslator.cpp allocaVar] Unsupported pointed type of value: ", val);
		}
	}
	
	pointerVar = new PointerVar(memVar);
	
	if (lastAllocatedMemVar) {
		lastAllocatedMemVar->nextAddrPointer = pointerVar;
	}
	pointerVar->prevAddrPointer = lastAllocatedMemVar;
	lastAllocatedMemVar = pointerVar;
	
	value2MemVar[val] = pointerVar;
}

void IRPathTranslator::calculateValue(const Value* val, string name) {
	if (const Constant* c = dyn_cast<Constant>(val)) {
		if (const ConstantExpr* ce = dyn_cast<ConstantExpr>(c)) {
			SmallVector<Value *, 4> ValueOperands(ce->op_begin(), ce->op_end());
			ArrayRef<Value*> Ops(ValueOperands);
			
			Instruction* inst = 0;
			switch (ce->getOpcode()) {
				case 8 ... 25: {
					BinaryOperator *BO = BinaryOperator::Create((Instruction::BinaryOps)(ce->getOpcode()), Ops[0], Ops[1]);
					if (isa<OverflowingBinaryOperator>(BO)) {
						BO->setHasNoUnsignedWrap(OverflowingBinaryOperator::NoUnsignedWrap);
						BO->setHasNoSignedWrap(OverflowingBinaryOperator::NoSignedWrap);
					}
					if (isa<PossiblyExactOperator>(BO)) {
						BO->setIsExact(PossiblyExactOperator::IsExact);
					}
					inst = BO;
					break;
				}
				case 29: {
					const auto *GO = dyn_cast<GEPOperator>(ce);
					if (GO->isInBounds()) {
						inst = GetElementPtrInst::CreateInBounds(GO->getSourceElementType(), Ops[0], Ops.slice(1));
					} else {
						inst = GetElementPtrInst::Create(GO->getSourceElementType(), Ops[0], Ops.slice(1));
					}
					break;
				}
				case 33 ... 45:
					inst = CastInst::Create((Instruction::CastOps)(ce->getOpcode()), Ops[0], ce->getType());
					break;
				case 46 ... 47:
					inst = CmpInst::Create((Instruction::OtherOps)(ce->getOpcode()), (CmpInst::Predicate)(ce->getPredicate()), Ops[0], Ops[1]);
					break;
				case 50:
					inst = SelectInst::Create(Ops[0], Ops[1], Ops[2]);
					break;
				case 54:
					inst = ExtractElementInst::Create(Ops[0], Ops[1]);
					break;
				case 55:
					inst = InsertElementInst::Create(Ops[0], Ops[1], Ops[2]);
					break;
				case 56:
					inst = new ShuffleVectorInst(Ops[0], Ops[1], Ops[2]);
					break;
				case 57:
					inst = ExtractValueInst::Create(Ops[0], ce->getIndices());
					break;
				case 58:
					inst = InsertValueInst::Create(Ops[0], Ops[1], ce->getIndices());
					break;
				default:
					throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown opCode of constant expr: ", ce);
					break;
			}
			
			calculateValue(inst);
			value2MemVar[val] = value2MemVar[inst];
		} else if (const ConstantInt* ci = dyn_cast<ConstantInt>(c)) {
			int bits = ci->getBitWidth();
			if (bits == 1) {
				if (ci->getZExtValue() == 0) {
					value2MemVar[val] = new DataVar(new BoolValue(0));
				} else {
					value2MemVar[val] = new DataVar(new BoolValue(1));
				}
			} else {
				value2MemVar[val] = new DataVar(new BitValue(ci->getZExtValue(), bits));
			}
		} else if (const ConstantFP* cfp = dyn_cast<ConstantFP>(c)) {
			if (cfp->getType()->isFloatTy()) {
				value2MemVar[val] = new DataVar(new FloatValue(cfp->getValueAPF().convertToFloat()));
			} else if (cfp->getType()->isDoubleTy()) {
				value2MemVar[val] = new DataVar(new FloatValue(cfp->getValueAPF().convertToDouble()));
			} else {
				throwStrForValue("[IRPathTranslator.cpp calculateValue] Unsupported floating point type of constant: ", cfp);
			}
		} else if (c->getType()->isStructTy()) {
			int size = c->getType()->getStructNumElements();
			StructVar* stVar = new StructVar(size);
			for (int i = 0; i < size; ++ i) {
				Constant* ele = c->getAggregateElement(i);
				calculateValue(ele);
				stVar->elements[i] = value2MemVar[ele];
			}
			value2MemVar[val] = stVar;
		} else if (c->getType()->isArrayTy()) {
			// array constant, use var instead of value
			Expr* expr = createExpr(c->getType(), name);
			for (int i = 0; i < c->getType()->getArrayNumElements(); ++ i) {
				if (dyn_cast<UndefValue>(c->getAggregateElement(i))) {
					continue;
				} else {
					Constant* ele = c->getAggregateElement(i);
					calculateValue(ele);
					expr = new Expr(array_store(*expr, BitValue(i, 32), *(dynamic_cast<DataVar*>(value2MemVar[ele])->expr)));
				}
			}
			value2MemVar[val] = new DataVar(expr);
		} else if (dyn_cast<ConstantPointerNull>(c)) {
			value2MemVar[val] = new PointerVar(0);
		} else if (dyn_cast<GlobalVariable>(c)) {
		} else if (dyn_cast<UndefValue>(c)) {
			if (c->getType()->isIntegerTy() || c->getType()->isFloatingPointTy()) {
				value2MemVar[val] = new DataVar(createExpr(c->getType(), name));
			} else {
				throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown type of UndefValue: ", c);
			}
		} else {
			throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown constant: ", c);
		}
	} else if (const Instruction* inst = dyn_cast<Instruction>(val)) {
		switch (inst->getOpcode()) {
			case 8 ... 25:
				value2MemVar[val] = new DataVar(calculateBinaryInst(inst->getOperand(0), inst->getOperand(1), inst->getOpcode()));
				break;
			case 29: {
				Value* op = inst->getOperand(0);
				PointerVar* pVar = new PointerVar(dynamic_cast<PointerVar*>(value2MemVar[op])->pointingVar);
				pVar->originTy = dynamic_cast<PointerVar*>(value2MemVar[op])->originTy;
				pVar->offsets.insert(pVar->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[op])->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[op])->offsets.end());
				
				Value* index = inst->getOperand(1);
				ConstantInt* constIndex = dyn_cast<ConstantInt>(index);
				if (!constIndex || constIndex->getZExtValue() != 0) {
					if (pVar->offsets.empty()) {
						throwStrForValue("[IRPathTranslator.cpp calculateValue] Offsets of pointer not pointing to array element in gep: ", inst);
					} else {
						if (dyn_cast<Constant>(index)) {
							calculateValue(index);
						}
						Expr* expr = dynamic_cast<DataVar*>(value2MemVar[index])->expr;
						int bits = dyn_cast<IntegerType>(index->getType())->getBitWidth();
						if (bits > 32) {
							expr = new Expr(extract(*expr, bits, 32));
						} else if (bits < 32) {
							expr = new Expr(zext(*expr, bits, 32));
						}
						if (pVar->originTy && pVar->originTy->isIntegerTy(8) && dyn_cast<PointerType>(op->getType())->getPointerElementType()->isIntegerTy(32)) {
							expr = new Expr(bvmul(*expr, BitValue(4, 32)));
						}
						expr = new Expr(bvadd(*(pVar->offsets.back()), *expr));
						pVar->offsets.pop_back();
						pVar->offsets.push_back(expr);
					}
				}
				
				Type* type = op->getType()->getPointerElementType();
				for (int i = 2; i < inst->getNumOperands(); ++ i) {
					index = inst->getOperand(i);
					if (type->isStructTy()) {
						if (constIndex = dyn_cast<ConstantInt>(index)) {
							pVar->pointingVar = dynamic_cast<StructVar*>(pVar->pointingVar)->elements[constIndex->getZExtValue()];
							type = type->getStructElementType(constIndex->getZExtValue());
						} else {
							throwStrForValue("[IRPathTranslator.cpp calculateValue] Non-constant index for struct in gep: ", inst);
						}
					} else if (type->isArrayTy()) {
						if (dyn_cast<Constant>(index)) {
							calculateValue(index);
						}
						Expr* expr = dynamic_cast<DataVar*>(value2MemVar[index])->expr;
						int bits = dyn_cast<IntegerType>(index->getType())->getBitWidth();
						if (bits > 32) {
							expr = new Expr(extract(*expr, bits, 32));
						} else if (bits < 32) {
							expr = new Expr(zext(*expr, bits, 32));
						}
						pVar->offsets.push_back(expr);
						type = type->getArrayElementType();
					} else {
						throwStrForValue("[IRPathTranslator.cpp calculateValue] Getting into non-struct-or-array value in gep: ", inst);
					}
				}
				
				value2MemVar[val] = pVar;
				break;
			}
			case 33 ... 41:
				value2MemVar[val] = new DataVar(calculateBasicCastInst(inst->getOperand(0), inst->getOpcode(), dyn_cast<CastInst>(inst)->getSrcTy(), dyn_cast<CastInst>(inst)->getDestTy()));
				break;
			case 44: {
				const BitCastInst* bcInst = dyn_cast<BitCastInst>(inst);
				Type* srcTy = bcInst->getSrcTy();
				Type* destTy = bcInst->getDestTy();
				
				if (srcTy->isPointerTy() && destTy->isPointerTy() && srcTy->getPointerElementType()->isIntegerTy(32) && destTy->getPointerElementType()->isFloatTy()) {
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bcInst->getOperand(0)]);
					PointerVar* pVar = new PointerVar(opPointer->pointingVar);
					pVar->offsets.insert(pVar->offsets.begin(), opPointer->offsets.begin(), opPointer->offsets.end());
					if (opPointer->originTy && opPointer->originTy->isFloatTy()) {
						pVar->originTy = 0;
					} else {
						pVar->originTy = srcTy->getPointerElementType();
					}
					value2MemVar[val] = pVar;
				} else if (srcTy->isPointerTy() && destTy->isPointerTy() && srcTy->getPointerElementType()->isFloatTy() && destTy->getPointerElementType()->isIntegerTy(32)) {
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bcInst->getOperand(0)]);
					PointerVar* pVar = new PointerVar(opPointer->pointingVar);
					pVar->offsets.insert(pVar->offsets.begin(), opPointer->offsets.begin(), opPointer->offsets.end());
					if (opPointer->originTy && opPointer->originTy->isIntegerTy(32)) {
						pVar->originTy = 0;
					} else {
						pVar->originTy = srcTy->getPointerElementType();
					}
					value2MemVar[val] = pVar;
				} else if (srcTy->isPointerTy() && destTy->isPointerTy() && srcTy->getPointerElementType()->isStructTy() && srcTy->getPointerElementType()->getStructElementType(0)->isFloatTy() && destTy->getPointerElementType()->isFloatTy()) {
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bcInst->getOperand(0)]);
					value2MemVar[val] = new PointerVar(dynamic_cast<StructVar*>(opPointer->pointingVar)->elements[0]);
				} else if (srcTy->isPointerTy() && destTy->isPointerTy() && srcTy->getPointerElementType()->isStructTy() && srcTy->getPointerElementType()->getStructElementType(0)->isFloatTy() && destTy->getPointerElementType()->isIntegerTy(32)) {
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bcInst->getOperand(0)]);
					PointerVar* pVar = new PointerVar(dynamic_cast<StructVar*>(opPointer->pointingVar)->elements[0]);
					pVar->originTy = srcTy->getPointerElementType()->getStructElementType(0);
					value2MemVar[val] = pVar;
				} else if (srcTy->isPointerTy() && destTy->isPointerTy() && srcTy->getPointerElementType()->isIntegerTy(8) && destTy->getPointerElementType()->isIntegerTy(32)) {
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bcInst->getOperand(0)]);
					PointerVar* pVar = new PointerVar(opPointer->pointingVar);
					pVar->offsets.insert(pVar->offsets.begin(), opPointer->offsets.begin(), opPointer->offsets.end());
					pVar->originTy = srcTy->getPointerElementType();
					value2MemVar[val] = pVar;
				} else {
					throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown bitcast: ", inst);
				}
				
				break;
			}
			case 46 ... 47:
				value2MemVar[val] = new DataVar(calculateCmpInst(inst->getOperand(0), inst->getOperand(1), dyn_cast<CmpInst>(inst)->getPredicate()));
				break;
			default:
				throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown instruction ", inst);
				break;
		}
	} else {
		throwStrForValue("[IRPathTranslator.cpp calculateValue] Unknown value: ", val);
	}
}

Expr* IRPathTranslator::calculateBinaryInst(const Value* op1, const Value* op2, int opCode) {
	if (dyn_cast<Constant>(op1)) {
		calculateValue(op1);
	}
	if (dyn_cast<Constant>(op2)) {
		calculateValue(op2);
	}
	
	DataVar* dVar1 = dynamic_cast<DataVar*>(value2MemVar[op1]);
	DataVar* dVar2 = dynamic_cast<DataVar*>(value2MemVar[op2]);
	if (!dVar1 || !dVar2) {
		throwStrForValue("[IRPathTranslator.cpp calculateBinaryInst] Non-DataVar MemVar of operand.");
	}
	
	Expr* expr1 = dVar1->expr;
	Expr* expr2 = dVar2->expr;
	if (op1->getType()->isIntegerTy(1) && op2->getType()->isIntegerTy(1)) {
		throwStrForValue("[IRPathTranslator.cpp calculateBinaryInst] Binary calculation of i1.");
	}
	
	Expr* expr = 0;
	switch (opCode) {
		case 8:
			expr = new Expr(bvadd(*expr1, *expr2));
			break;
		case 9:
			expr = new Expr(fpadd(*expr1, *expr2));
			break;
		case 10:
			expr = new Expr(bvsub(*expr1, *expr2));
			break;
		case 11:
			expr = new Expr(fpsub(*expr1, *expr2));
			break;
		case 12:
			expr = new Expr(bvmul(*expr1, *expr2));
			break;
		case 13:
			expr = new Expr(fpmul(*expr1, *expr2));
			break;
		case 14:
			expr = new Expr(bvudiv(*expr1, *expr2));
			break;
		case 15:
			expr = new Expr(bvsdiv(*expr1, *expr2));
			break;
		case 16:
			expr = new Expr(fpdiv(*expr1, *expr2));
			break;
		case 17:
			expr = new Expr(bvurem(*expr1, *expr2));
			break;
		case 18:
			expr = new Expr(bvsrem(*expr1, *expr2));
			break;
		case 19:
			expr = new Expr(fprem(*expr1, *expr2));
			break;
		case 20:
			expr = new Expr(bvshl(*expr1, *expr2));
			break;
		case 21:
			expr = new Expr(bvlshr(*expr1, *expr2));
			break;
		case 22:
			expr = new Expr(bvashr(*expr1, *expr2));
			break;
		case 23:
			expr = new Expr(bvand(*expr1, *expr2));
			break;
		case 24:
			expr = new Expr(bvor(*expr1, *expr2));
			break;
		case 25:
			expr = new Expr(bvxor(*expr1, *expr2));
			break;
		default:
			throwStrForValue("[IRPathTranslator.cpp calculateBinaryInst] Invalid opCode: " + to_string(opCode));
			break;
	}
	
	return expr;
}

Expr* IRPathTranslator::calculateBasicCastInst(const Value* op, int opCode, Type* srcTy, Type* destTy) {
	if (dyn_cast<Constant>(op)) {
		calculateValue(op);
	}
	
	DataVar* dVar = dynamic_cast<DataVar*>(value2MemVar[op]);
	if (!dVar) {
		throwStrForValue("[IRPathTranslator.cpp calculateBasicCastInst] Non-DataVar MemVar of operand.");
	}
	
	Expr* expr = dVar->expr;
	if (srcTy->isIntegerTy(1)) {
		expr = new Expr(ite(*expr, BitValue(1, 1), BitValue(0, 1)));
	}
	
	switch (opCode) {
		case 33:
			expr = new Expr(extract(*expr, dyn_cast<IntegerType>(srcTy)->getBitWidth(), dyn_cast<IntegerType>(destTy)->getBitWidth()));
			break;
		case 34:
			expr = new Expr(zext(*expr, dyn_cast<IntegerType>(srcTy)->getBitWidth(), dyn_cast<IntegerType>(destTy)->getBitWidth()));
			break;
		case 35:
			expr = new Expr(sext(*expr, dyn_cast<IntegerType>(srcTy)->getBitWidth(), dyn_cast<IntegerType>(destTy)->getBitWidth()));
			break;
		case 36:
			expr = new Expr(fptoubv(*expr, dyn_cast<IntegerType>(destTy)->getBitWidth()));
			break;
		case 37:
			expr = new Expr(fptosbv(*expr, dyn_cast<IntegerType>(destTy)->getBitWidth()));
			break;
		case 38:
			if (destTy->isFloatTy()) {
				expr = new Expr(ubvtofp(*expr, FloatSort(8, 24)));
			} else if (destTy->isDoubleTy()) {
				expr = new Expr(ubvtofp(*expr, FloatSort(11, 53)));
			} else if (destTy->isX86_FP80Ty()) {
				expr = new Expr(ubvtofp(*expr, FloatSort(15, 65)));
			} else {
				throwStrForValue("[IRPathTranslator.cpp calculateBasicCastInst] Unknown fp type to cast.");
			}
			break;
		case 39:
			if (destTy->isFloatTy()) {
				expr = new Expr(sbvtofp(*expr, FloatSort(8, 24)));
			} else if (destTy->isDoubleTy()) {
				expr = new Expr(sbvtofp(*expr, FloatSort(11, 53)));
			} else if (destTy->isX86_FP80Ty()) {
				expr = new Expr(sbvtofp(*expr, FloatSort(15, 65)));
			} else {
				throwStrForValue("[IRPathTranslator.cpp calculateBasicCastInst] Unknown fp type to cast.");
			}
			break;
		case 40:
		case 41:
			if (destTy->isFloatTy()) {
				expr = new Expr(fptofp(*expr, FloatSort(8, 24)));
			} else if (destTy->isDoubleTy()) {
				expr = new Expr(fptofp(*expr, FloatSort(11, 53)));
			} else if (destTy->isX86_FP80Ty()) {
				expr = new Expr(fptofp(*expr, FloatSort(15, 65)));
			} else {
				throwStrForValue("[IRPathTranslator.cpp calculateBasicCastInst] Unknown fp type to cast.");
			}
			break;
	}
	
	if (destTy->isIntegerTy(1)) {
		expr = new Expr(*expr == BitValue(1, 1));
	}
	
	return expr;
}

Expr* IRPathTranslator::calculateCmpInst(const Value* op1, const Value* op2, int predicate) {
	if (dyn_cast<Constant>(op1)) {
		calculateValue(op1);
	}
	if (dyn_cast<Constant>(op2)) {
		calculateValue(op2);
	}
	
	DataVar* dVar1 = dynamic_cast<DataVar*>(value2MemVar[op1]);
	DataVar* dVar2 = dynamic_cast<DataVar*>(value2MemVar[op2]);
	if (!dVar1 || !dVar2) {
		throwStrForValue("[IRPathTranslator.cpp calculateCmpInst] Non-DataVar MemVar of operand.");
	}
	
	Expr* expr1 = dVar1->expr;
	Expr* expr2 = dVar2->expr;
	Expr* expr = 0;
	switch (predicate) {
		case CmpInst::ICMP_EQ:
			expr = new Expr(*expr1 == *expr2);
			break;
		case CmpInst::ICMP_NE:
			expr = new Expr(!(*expr1 == *expr2));
			break;
		case CmpInst::ICMP_UGT:
			expr = new Expr(bvugt(*expr1, *expr2));
			break;
		case CmpInst::ICMP_SGT:
			expr = new Expr(bvsgt(*expr1, *expr2));
			break;
		case CmpInst::ICMP_ULT:
			expr = new Expr(bvult(*expr1, *expr2));
			break;
		case CmpInst::ICMP_SLT:
			expr = new Expr(bvslt(*expr1, *expr2));
			break;
		case CmpInst::ICMP_UGE:
			expr = new Expr(bvuge(*expr1, *expr2));
			break;
		case CmpInst::ICMP_SGE:
			expr = new Expr(bvsge(*expr1, *expr2));
			break;
		case CmpInst::ICMP_ULE:
			expr = new Expr(bvule(*expr1, *expr2));
			break;
		case CmpInst::ICMP_SLE:
			expr = new Expr(bvsle(*expr1, *expr2));
			break;
		case CmpInst::FCMP_OEQ:
		case CmpInst::FCMP_UEQ:
			expr = new Expr(fpeq(*expr1, *expr2));
			break;
		case CmpInst::FCMP_ONE:
		case CmpInst::FCMP_UNE:
			expr = new Expr(!fpeq(*expr1, *expr2));
			break;
		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
			expr = new Expr(fpgt(*expr1, *expr2));
			break;
		case CmpInst::FCMP_OLT:
		case CmpInst::FCMP_ULT:
			expr = new Expr(fplt(*expr1, *expr2));
			break;
		case CmpInst::FCMP_OGE:
		case CmpInst::FCMP_UGE:
			expr = new Expr(fpgeq(*expr1, *expr2));
			break;
		case CmpInst::FCMP_OLE:
		case CmpInst::FCMP_ULE:
			expr = new Expr(fpleq(*expr1, *expr2));
			break;
		case CmpInst::FCMP_FALSE:
			expr = new BoolValue(0);
			break;
		case CmpInst::FCMP_TRUE:
			expr = new BoolValue(1);
			break;
		case CmpInst::FCMP_ORD:
		case CmpInst::FCMP_UNO:
		default:
			throwStrForValue("[IRPathTranslator.cpp calculateCmpInst] Invalid predicate.");
			break;
	}
	
	return expr;
}

Expr IRPathTranslator::path2SMTM(const Module& g, const std::vector<const BasicBlock*> path, int firstPHISelector) {
	#if PRINT_PATH
	ofstream fout("IRPathTranslatorLog.txt", ofstream::out);
	fout << "--------------------PATH TO TRANSLATE--------------------" << endl;
	for (int i = 0; i < path.size(); ++ i) {
		string str;
		raw_string_ostream rso(str);
		path[i]->print(rso);
		fout << str << endl;
	}
	#endif
	
	for (Module::const_global_iterator mgi = g.global_begin(), mge = g.global_end(); mgi != mge; ++ mgi) {
		#if PRINT_PATH
		string str;
		raw_string_ostream rso(str);
		mgi->print(rso);
		fout << ">>>" << str << endl;
		#endif
		allocaVar(mgi, mgi->getInitializer());
	}
	
	lastAllocatedMemVar = 0;
	
	Expr pathCondition = BoolValue(1);
	map<const Instruction*, bool> knownInst;
    int nondetCount = 0;
	for (int i = 0; i < path.size() - 1; ++ i) {
		for (BasicBlock::const_iterator bi = path[i]->begin(), be = path[i]->end(); bi != be; ++ bi) {
			#if PRINT_PATH
			string str;
			raw_string_ostream rso(str);
			bi->print(rso);
			fout << ">>>" << str << endl;
			#endif
			
			for (int j = 0; j < bi->getNumOperands(); ++ j) {
				if (const Instruction* inst = dyn_cast<Instruction>(bi->getOperand(j))) {
					if (!knownInst[inst]) {
						if (inst->getType()->isIntegerTy() || inst->getType()->isFloatingPointTy() || inst->getType()->isArrayTy()) {
							value2MemVar[inst] = new DataVar(createExpr(inst->getType(), inst->getName()));
						} else if (inst->getType()->isPointerTy()) {
							allocaVar(inst, 0);
						} else {
							throwStrForValue("[IRPathTranslator.cpp path2SMTM] Non-int-or-fp-or-array-or-pointer undefined value in inst: ", bi);
						}
					}
				}
			}
			knownInst[bi] = 1;
			
			switch (bi->getOpcode()) {
				// ret
				case 1:
					break;
				// br
				case 2: {
					const BranchInst* inst = dyn_cast<BranchInst>(bi);
					if (inst->isConditional()) {
						Value* cond = inst->getCondition();
						if (dyn_cast<Constant>(cond)) {
							calculateValue(cond);
						}
						if (path[i + 1] == inst->getSuccessor(0)) {
							pathCondition = pathCondition && *(dynamic_cast<DataVar*>(value2MemVar[cond])->expr);
						}
						if (path[i + 1] == inst->getSuccessor(1)) {
							pathCondition = pathCondition && !(*(dynamic_cast<DataVar*>(value2MemVar[cond])->expr));
						}
					}
					break;
				}
				// switch
				case 3: {
					const SwitchInst* inst = dyn_cast<SwitchInst>(bi);
					Value* cond = inst->getCondition();
					if (dyn_cast<Constant>(cond)) {
						calculateValue(cond);
					}
					Expr subexpr = BoolValue(1);
					for (SwitchInst::ConstCaseIt sci = inst->case_begin(), end = inst->case_end(); sci != end; ++ sci) {
						if (path[i + 1] == sci.getCaseSuccessor()) {
							subexpr = subexpr || (*(dynamic_cast<DataVar*>(value2MemVar[cond])->expr) == *(dynamic_cast<DataVar*>(value2MemVar[sci.getCaseValue()])->expr));
						}
					}
					pathCondition = pathCondition && subexpr;
					break;
				}
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
				// fadd
				case 9:
				// sub
				case 10:
				// fsub
				case 11:
				// mul
				case 12:
				// fmul
				case 13:
				// udiv
				case 14:
				// sdiv
				case 15:
				// fdiv
				case 16:
				// urem
				case 17:
				// srem
				case 18:
				// frem
				case 19:
				// shl
				case 20:
				// lshr
				case 21:
				// ashr
				case 22:
				// and
				case 23:
				// or
				case 24:
				// xor
				case 25:
					calculateValue(bi);
					break;
				// alloca
				case 26: {
					const AllocaInst* inst = dyn_cast<AllocaInst>(bi);
					if (inst->isArrayAllocation()) {
						PointerVar* pVar = new PointerVar(new DataVar(createExpr(ArrayType::get(inst->getAllocatedType(), 1), inst->getName())));
						pVar->offsets.push_back(new BitValue(0, 32));
						value2MemVar[bi] = pVar;
					} else {
						allocaVar(bi, 0);
					}
					break;
				}
				// load
				case 27: {
					if (dyn_cast<Constant>(bi->getOperand(0))) {
						calculateValue(bi->getOperand(0));
					}
					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bi->getOperand(0)]);
				
					if (bi->getType()->isIntegerTy() || bi->getType()->isFloatingPointTy()) {
						Expr* expr = dynamic_cast<DataVar*>(opPointer->pointingVar)->expr;
						if (!opPointer->offsets.empty()) {
							for (int i = 0; i < opPointer->offsets.size(); ++ i) {
								expr = new Expr(array_select(*expr, *(opPointer->offsets[i])));
							}
						}
						if (opPointer->originTy) {
							if (bi->getType()->isIntegerTy(32) && opPointer->originTy->isFloatTy() && expr->sort().type() == SortType::Float) {
								expr = new Expr(fptoieee(*expr));
							} else if (bi->getType()->isFloatTy() && opPointer->originTy->isIntegerTy(32) && expr->sort().type() == SortType::Bv) {
								expr = new Expr(bvcasttofp(*expr, FloatSort(8, 24)));
							} else if (bi->getType()->isIntegerTy(32) && opPointer->originTy->isIntegerTy(8)) {
								if (opPointer->offsets.empty()) {
									throwStrForValue("[IRPathTranslator.cpp path2SMTM] Load value with type of larger size than in memory: ", bi);
								} else {
									expr = dynamic_cast<DataVar*>(opPointer->pointingVar)->expr;
									Expr expr1 = zext(array_select(*expr, *(opPointer->offsets[0])), 8, 32);
									Expr expr2 = zext(array_select(*expr, bvadd(*(opPointer->offsets[0]), BitValue(1, 32))), 8, 32);
									Expr expr3 = zext(array_select(*expr, bvadd(*(opPointer->offsets[0]), BitValue(2, 32))), 8, 32);
									Expr expr4 = zext(array_select(*expr, bvadd(*(opPointer->offsets[0]), BitValue(3, 32))), 8, 32);
									expr = new Expr(bvadd(expr1, bvadd(bvshl(expr2, BitValue(8, 32)), bvadd(bvshl(expr3, BitValue(16, 32)), bvshl(expr4, BitValue(24, 32))))));
								}
							}
						}
						value2MemVar[bi] = new DataVar(new Expr(*expr));
					} else if (bi->getType()->isPointerTy()) {
						PointerVar* pVar = new PointerVar(dynamic_cast<PointerVar*>(opPointer->pointingVar)->pointingVar);
						pVar->originTy = dynamic_cast<PointerVar*>(opPointer->pointingVar)->originTy;
						pVar->offsets.insert(pVar->offsets.begin(), dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.begin(), dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.end());
						value2MemVar[bi] = pVar;
					} else {
						throwStrForValue("[IRPathTranslator.cpp path2SMTM] Load non-int, fp, pointer value: ", bi);
					}
					
					break;
				}
				// store
				case 28: {
					const Value* opValue1 = bi->getOperand(0);
                    const Value* opValue2 = bi->getOperand(1);

					if (dyn_cast<Argument>(opValue1)) {
						break;
					}

                    if (dyn_cast<Constant>(opValue2)) {
                        calculateValue(opValue2);
                    }

					PointerVar* opPointer = dynamic_cast<PointerVar*>(value2MemVar[bi->getOperand(1)]);

					if (opPointer->originTy && opPointer->originTy->isIntegerTy(8)) {
						throwStrForValue("[IRPathTranslator.cpp path2SMTM] Store in pointer from bitcast which modifies type size: ", bi);
					}

					if (dyn_cast<Constant>(opValue1)) {
						calculateValue(opValue1);
					}
					
					if (opValue1->getType()->isIntegerTy() || opValue1->getType()->isFloatingPointTy()) {
						if (opPointer->offsets.empty()) {
							if (opPointer->pointingVar) {
								dynamic_cast<DataVar*>(opPointer->pointingVar)->expr = new Expr(*(dynamic_cast<DataVar*>(value2MemVar[opValue1])->expr));
							} else {
								opPointer->pointingVar = new DataVar(new Expr(*(dynamic_cast<DataVar*>(value2MemVar[opValue1])->expr)));
							}
						} else {
							Expr* expr = new Expr(*(dynamic_cast<DataVar*>(value2MemVar[opValue1])->expr));
							for (int i = 0; i < opPointer->offsets.size(); ++ i) {
								Expr* arr = dynamic_cast<DataVar*>(opPointer->pointingVar)->expr;
								for (int j = 0; j < opPointer->offsets.size() - i - 1; ++ j) {
									arr = new Expr(array_select(*arr, *(opPointer->offsets[i])));
								}
								expr = new Expr(array_store(*arr, *(opPointer->offsets[i]), *expr));
							}
							dynamic_cast<DataVar*>(opPointer->pointingVar)->expr = new Expr(*expr);
						}
					} else if (opValue1->getType()->isPointerTy()) {
						if (opPointer->pointingVar) {
							dynamic_cast<PointerVar*>(opPointer->pointingVar)->pointingVar = dynamic_cast<PointerVar*>(value2MemVar[opValue1])->pointingVar;
							dynamic_cast<PointerVar*>(opPointer->pointingVar)->originTy = dynamic_cast<PointerVar*>(value2MemVar[opValue1])->originTy;
							dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.insert(dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[opValue1])->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[opValue1])->offsets.end());
						} else {
							opPointer->pointingVar = new PointerVar(dynamic_cast<PointerVar*>(value2MemVar[opValue1])->pointingVar);
							dynamic_cast<PointerVar*>(opPointer->pointingVar)->originTy = dynamic_cast<PointerVar*>(value2MemVar[opValue1])->originTy;
							dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.insert(dynamic_cast<PointerVar*>(opPointer->pointingVar)->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[opValue1])->offsets.begin(), dynamic_cast<PointerVar*>(value2MemVar[opValue1])->offsets.end());
						}
					} else {
						throwStrForValue("[IRPathTranslator.cpp path2SMTM] Store non-int, fp, pointer value: ", bi);
					}
					break;
				}
				// getelementptr
				case 29:
					calculateValue(bi);
					break;
				// fence
				case 30:
				// atomiccmpxchg
				case 31:
				// atomicrmw
				case 32:
					break;
				// trunc
				case 33:
				// zext
				case 34:
				//sext
				case 35:
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
					calculateValue(bi);
					break;
				// ptrtoint
				case 42:
				// inttoptr
				case 43:
					break;
				// bitcast
				case 44:
					calculateValue(bi);
					break;
				// addrspacecast
				case 45:
					break;
				// icmp
				case 46:
				// fcmp
				case 47:
					calculateValue(bi);
					break;
				// phi
				case 48: {
					const PHINode* inst = dyn_cast<PHINode>(bi);
					if (i > 0) {
						if (dyn_cast<Constant>(inst->getIncomingValueForBlock(path[i - 1]))) {
							calculateValue(inst->getIncomingValueForBlock(path[i - 1]));
						}
						value2MemVar[bi] = value2MemVar[inst->getIncomingValueForBlock(path[i - 1])];
					} else {
						if (firstPHISelector > -1) {
							if (dyn_cast<Constant>(inst->getIncomingValue(firstPHISelector))) {
								calculateValue(inst->getIncomingValue(firstPHISelector));
							}
							value2MemVar[bi] = value2MemVar[inst->getIncomingValue(firstPHISelector)];
						} else {
							throwStrForValue("[IRPathTranslator.cpp path2SMTM] PHI in first node of path.");
						}
					}
					break;
				}
				// call
				case 49: {
                    const CallInst* inst = dyn_cast<CallInst>(bi);
                    Function* func = inst->getCalledFunction();
                    if (!func) {
                    } else {
                        if (func->getName() == "fesetround" || func->getName() == "sin" || func->getName() == "__finite" || func->getName() == "__finitef" || func->getName() == "__finitel" || func->getName() == "__fpclassify" || func->getName() == "__fpclassifyf" || func->getName() == "__isinf" || func->getName() == "__isinff" || func->getName() == "__isinfl" || func->getName() == "__isnan" || func->getName() == "__isnanf" || func->getName() == "__isnanl" || func->getName() == "__signbit" || func->getName() == "__signbitf" || func->getName() == "__signbitl" || func->getName() == "ceil" || func->getName() == "copysign" || func->getName() == "floor" || func->getName() == "remainder" || func->getName() == "round" || func->getName() == "trunc" || func->getName() == "modff" || func->getName() == "nan") {
                            throwStrForValue("[IRPathTranslator.cpp path2SMTM] E_FLOATS_FUNCTION");
                        } else if (func->getName().substr(0, 18) == "__VERIFIER_nondet_") {
							value2MemVar[bi] = new DataVar(createExpr(bi->getType(), (inst->getName().str() + to_string(++ nondetCount))));
						} else if (func->getName() == "malloc") {
							PointerType* pType = dyn_cast<PointerType>(bi->getType());
							if (pType && (pType->getPointerElementType()->isIntegerTy() || pType->getPointerElementType()->isFloatingPointTy())) {
								PointerVar* pVar = new PointerVar(new DataVar(createExpr(ArrayType::get(pType->getPointerElementType(), 1), inst->getName())));
								pVar->offsets.push_back(new BitValue(0, 32));
								value2MemVar[bi] = pVar;
							} else {
								throwStrForValue("[IRPathTranslator.cpp path2SMTM] Unknown type to malloc: ", bi);
							}
						} else if (func->getName() == "llvm.stacksave") {
							value2MemVar[bi] = new PointerVar(0);
						} else if (func->getName() == "llvm.dbg.declare" || func->getName() == "printf") {
                            // no operation
                        } else {
							throwStrForValue("[IRPathTranslator.cpp path2SMTM] Unhandled function call: " + func->getName().str());
						}
                    }
                    break;
                }
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
	
	#if PRINT_PATH
	fout << "--------------------TRANSLATED EXPR--------------------" << endl;
	fout << pathCondition << endl;
	fout.close();
	#endif
	
	return pathCondition;
}

Expr IRPathTranslator::path2SMTM(const Module& g, const std::vector<const BasicBlock*> path, map<const Value*, Expr*>& valueExprMap, int firstPHISelector) {
	Expr expr = path2SMTM(g, path, firstPHISelector);
	for (map<const Value*, Expr*>::iterator it = valueExprMap.begin(); it != valueExprMap.end(); ++ it) {
		MemVar* memVar = value2MemVar[it->first];
		while (1) {
			if (DataVar* dVar = dynamic_cast<DataVar*>(memVar)) {
				valueExprMap[it->first] = dVar->expr;
				break;
			} else if (PointerVar* pVar = dynamic_cast<PointerVar*>(memVar)) {
				memVar = pVar->pointingVar;
			} else {
				throwStrForValue("[IRPathTranslator.cpp path2SMTM] Cannot get expr of struct value: ", it->first);
			}
		}
	}
	return expr;
}

void IRPathTranslator::getVarExpr(const Module &g, const BasicBlock *block, map<string, Expr *> &var2Expr) {
	vector<const BasicBlock*> path = vector<const BasicBlock*>();
	path.push_back(block);
	if (block->getTerminator()->getNumSuccessors() == 0) {
		path.push_back(block);
	} else {
		path.push_back(block->getTerminator()->getSuccessor(0));
	}
	path2SMTM(g, path);

	for (map<string, Expr*>::iterator it = var2Expr.begin(); it != var2Expr.end(); ++ it) {
		for (BasicBlock::const_iterator bi = block->begin(), be = block->end(); bi != be; ++ bi) {
			if (bi->getName() == it->first) {
				MemVar* mVar = value2MemVar[bi];
                while (1) {
                    if (DataVar* dVar = dynamic_cast<DataVar*>(mVar)) {
                        var2Expr[it->first] = dVar->expr;
                        break;
                    } else if (PointerVar* pVar = dynamic_cast<PointerVar*>(mVar)) {
                        mVar = pVar->pointingVar;
                    } else {
                        throwStrForValue("[IRPathTranslator.cpp getVarExpr] Cannot get expr of struct value");
                    }
                }
                break;
			}
		}
	}
}

void IRPathTranslator::throwStrForValue(string str, const Value* val) {
	if (val) {
		string valStr;
		raw_string_ostream rso(valStr);
		val->print(rso);
		throw(str + valStr);
	} else {
		throw(str);
	}
}

void IRPathTranslator::throwStrForValue(string str, Type* type) {
	if (type) {
		string typeStr;
		raw_string_ostream rso(typeStr);
		type->print(rso);
		throw(str + typeStr);
	} else {
		throw(str);
	}
}

}
