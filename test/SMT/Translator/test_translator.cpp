#include <iostream>
// LLVM headers
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/SourceMgr.h>

#include "test/support.h"
#include "DFS/Advisor/Advisor.h"
#include "SMT/Middleware/Z3RawSolver.h"
#include "SMT/Translator/IRPathTranslator.h"
#include "SMT/Translator/Translator.h"
#include "Preprocess/InlinePreprocessor.h"
#include "Preprocess/SVCompInterfacePreprocessor.h"

using namespace ceagle;
using namespace ceagle::smt;
using namespace llvm;

int main() {
    try {
    auto translator = new IRPathTranslator();
    LLVMContext &context = getGlobalContext();
    SMDiagnostic err;
    auto module = parseIRFile("../test/res/byte_add_1_true_unreach-call.ll", err, context);
    if (module == nullptr) {
        std::cout << "Corrupted IR file or IR file not found" << std::endl;
        llvm::errs() << err.getMessage();
        exit(-1);
    }
    // preprocess
    std::vector<std::string> ppWhiteList;
    ppWhiteList.push_back("__VERIFIER_assume");
    ppWhiteList.push_back("__VERIFIER_assert");
    ppWhiteList.push_back("__VERIFIER_nondet_int");
    ppWhiteList.push_back("__VERIFIER_nondet_float");
    ppWhiteList.push_back("__VERIFIER_nondet_double");
    ppWhiteList.push_back("__VERIFIER_error");
    auto preprocess1 = InlinePreprocessor(ppWhiteList);
    //InlinePreprocessor preprocess1;
    preprocess1.preprocess(*module);
    auto preprocess2 = SVCompInterfacePreprocessor(STR_LABEL_ERROR, STR_LABEL_ASSUME_FAIL);
    //auto preprocess = SVCompInterfacePreprocessor("error", "assume");
    preprocess2.preprocess(*module);
    Function* mainFunction = module->getFunction("main");
    auto it = mainFunction->begin();
    std::vector<const BasicBlock*> path = {it};
    it++;
    path.push_back(it);
    it++;
    path.push_back(it);
    auto result = translator->path2SMTM(*module, {path[1], path[2]});
    auto solver = new Z3RawSolver();
    solver->add(result);
    std::cout << solver->getDebugInfo();


    // test valueMap
    {
        auto module = parseIRFile("../test/res/AdditionIntMin_false-no-overflow.c.ll", err, context);
        assert(module);
        std::vector<std::string> ppWhiteList;
        ppWhiteList.push_back("__VERIFIER_assume");
        ppWhiteList.push_back("__VERIFIER_assert");
        ppWhiteList.push_back("__VERIFIER_nondet_int");
        ppWhiteList.push_back("__VERIFIER_nondet_float");
        ppWhiteList.push_back("__VERIFIER_nondet_double");
        ppWhiteList.push_back("__VERIFIER_error");
        auto preprocess1 = InlinePreprocessor(ppWhiteList);
        //InlinePreprocessor preprocess1;
        preprocess1.preprocess(*module);
        auto preprocess2 = SVCompInterfacePreprocessor(STR_LABEL_ERROR, STR_LABEL_ASSUME_FAIL);
        //auto preprocess = SVCompInterfacePreprocessor("error", "assume");
        preprocess2.preprocess(*module);
        auto translator = new IRPathTranslator();
        std::map<const Value*, Expr *> valueMap;
        auto mainFunction = module->getFunction("main");
        auto entry = mainFunction->begin();
        std::vector<const BasicBlock*> path = {entry, nullptr};
        for (auto& inst: *entry) {
            switch(inst.getOpcode()) {
                case Instruction::Alloca:
                    valueMap[&inst] = nullptr;
            }
        }
        translator->path2SMTM(*module, path, valueMap);
        std::map<std::string, Expr> result;
        for (auto p: valueMap) {
            result[p.first->getName().str()] = *p.second;
        }
        Expr expect = BitValue(0, 32);
        EQUAL(result["retval"].str(), expect.str());
        EQUAL(result["minInt"].str(), BitValue(-2147483648, 32).str());
        EQUAL(result["x"].str(), bvadd(bvadd(BitValue(-2147483648, 32), BitValue(-1, 32)), BitValue(23, 32)).str());
    }
    } catch (std::string e) { std::cout << e << std::endl; }
    {
        LLVMContext &context = getGlobalContext();
        SMDiagnostic err;
        auto module = parseIRFile("../test/res/test_and.ll", err, context);
        assert(module);
        auto mainFunction = module->getFunction("main");
        auto it = mainFunction->begin(); //entry
        std::vector<const BasicBlock*> path;
        path.push_back(&(*it));
        ++it;
        path.push_back(&(*it));
        auto translator = new IRPathTranslator();
        auto cond = translator->path2SMTM(*module, path);
        auto expect = BoolValue(true) && !(bvand(Var("x", BvSort(32)), BitValue(1, 32)) == BitValue(0, 32));
        EQUAL(cond.str(), expect.str());
    }
    return 0;
}
