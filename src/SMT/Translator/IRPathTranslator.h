#ifndef CEAGLE_IR_PATH_TRANSLATOR_H
#define CEAGLE_IR_PATH_TRANSLATOR_H

#include "Translator.h"

#include "llvm/IR/Constants.h"

using llvm::Value;
using llvm::Constant;
using llvm::Type;
using std::vector;
using std::map;
using std::string;
using std::to_string;
using ceagle::smt::Expr;
using ceagle::smt::Var;

namespace ceagle {

namespace mm {
	
class MemVar {
	virtual void dummy() {}
};

class PointerVar : public MemVar {
public:
	MemVar* pointingVar;
	PointerVar* prevAddrPointer;
	PointerVar* nextAddrPointer;
	vector<Expr*> offsets;
	Type* originTy;
	PointerVar(MemVar* var) {
		pointingVar = var;
		prevAddrPointer = 0;
		nextAddrPointer = 0;
		offsets = vector<Expr*>();
		originTy = 0;
	}
};

class DataVar : public MemVar {
public:
	Expr* expr;
	DataVar(Expr* e) {
		expr = e;
	}
};


class StructVar : public MemVar {
public:
	vector<MemVar*> elements;
	StructVar(int numElements) {
		for (int i = 0; i < numElements; ++ i) {
			elements.push_back(0);
		}
	}
};

}

using namespace mm;

class IRPathTranslator : public Translator<> {
	int cnt = 0;
	PointerVar* lastAllocatedMemVar = 0;
	map<const Value*, string> value2ExprName;
	map<const Value*, MemVar*> value2MemVar;
	void allocaVar(const Value* val, const Constant* initVal);
	Expr* createExpr(Type* type, string name = "");
	void calculateValue(const Value* val, string name = "");
	Expr* calculateBinaryInst(const Value* op1, const Value* op2, int opCode);
	Expr* calculateBasicCastInst(const Value* op, int opCode, Type* srcTy, Type* destTy);
	Expr* calculateCmpInst(const Value* op1, const Value* op2, int predicate);
	void throwStrForValue(string str, const Value* val = 0);
	void throwStrForValue(string str, Type* type);
public:
	virtual Expr path2SMTM(const Module& g, const vector<const BasicBlock*> path, int firstPHISelector = -1);
	virtual Expr path2SMTM(const Module& g, const vector<const BasicBlock*> path, map<const Value*, Expr*>& valueExprMap, int firstPHISelector = -1);
	void getVarExpr(const Module &g, const BasicBlock* block, map<string, Expr*> &var2Expr);
};

}

#endif
