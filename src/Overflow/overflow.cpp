#include "Overflow/overflow.h"
#include "SMT/Translator/IRPathTranslator.h"
#include "Preprocess/InlinePreprocessor.h"
#include "Preprocess/NamingPreprocessor.h"

#include <cassert>
#include <cstdio>
#include <sstream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <fstream>
#include <regex>

using namespace llvm;

namespace ceagle {

std::ostream& operator<<(std::ostream& out, OverflowResult& result) {
    switch(result) {
        case OverflowResult::Unknown:
            out << "UNKNOWN";
            break;
        case OverflowResult::NoOverflow:
            out << "NO_OVERFLOW";
            break;
        case OverflowResult::Overflow:
            out << "OVERFLOW";
            break;
    }
    return out;
}

OverflowResult compilerCheck(std::string fileName, std::string compilerName) {
    std::string fnll = fileName + ".ll";
    std::string command = compilerName + " -g -S -emit-llvm " + fileName + " -o " + fnll + " 2>&1";
    FILE* p = popen(command.c_str(), "r");
    assert(p);
    char buff[1024];
    std::ostringstream out;
    while (fgets(buff, sizeof(buff), p)) {
        out << buff;
    }
    pclose(p);
    std::string output = out.str();
    if (output.find("overflow in expression;") != std::string::npos) {
        return OverflowResult::Overflow;
    }
    return OverflowResult::NoOverflow;
}

OverflowResult evalCheck(Module* module) {
    auto mainFunction = module->getFunction("main");
    for (auto& b : *mainFunction) {
        auto translator = new IRPathTranslator();
        std::vector<const BasicBlock*> path = {&b, nullptr};
        std::map<const Value*, Expr *> valueMap;
        for (auto& inst: b) {
            switch(inst.getOpcode()) {
                case Instruction::Alloca:
                    valueMap[&inst] = nullptr;
            }
        }
        translator->path2SMTM(*module, path, valueMap);
        OverflowEvaluator evaluator;
        for (auto p: valueMap) {
            try {
                evaluator.evaluate(*p.second);
            } catch (std::overflow_error) {
                return OverflowResult::Overflow;
            }
        }
    }
    return OverflowResult::NoOverflow;
}

OverflowResult evilCheck(Module* module) {
    auto mainFunction = module->getFunction("main");
    for (auto& b : *mainFunction) {
        for (auto& inst: b) {
            switch(inst.getOpcode()) {
                case Instruction::Trunc:
                    return OverflowResult::NoOverflow;
            }
        }
    }
    return OverflowResult::Unknown;
}

OverflowResult verifyOverflow(std::string fileName, std::string compilerName) {
    // check loops
    std::ifstream ifs(fileName);
    std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    std::regex loopPattern(R"((while|for)\s*\()");
    std::smatch match;
    if (std::regex_search(file, match, loopPattern)) {
        return OverflowResult::Unknown;
    }
    OverflowResult result = compilerCheck(fileName, compilerName);
    if (result == OverflowResult::Overflow) {
        return result;
    }
    std::string fnll = fileName + ".ll";
    LLVMContext &context = getGlobalContext();
    SMDiagnostic err;
    std::unique_ptr<Module> module = parseIRFile(fnll.c_str(), err, context);
    assert(module);
    // inline
    std::vector<std::string> ppWhiteList;
    ppWhiteList.push_back("__VERIFIER_assume");
    ppWhiteList.push_back("__VERIFIER_assert");
    ppWhiteList.push_back("__VERIFIER_nondet_int");
    ppWhiteList.push_back("__VERIFIER_nondet_float");
    ppWhiteList.push_back("__VERIFIER_nondet_double");
    ppWhiteList.push_back("__VERIFIER_error");
    auto preprocess1 = InlinePreprocessor(ppWhiteList);
    preprocess1.preprocess(*module);
    auto preprocess3 = NamingPreprocessor();
    preprocess3.preprocess(*module);

    result = evilCheck(module.get());
    if (result != OverflowResult::Unknown) {
        return result;
    }
    result = evalCheck(module.get());
    if (result == OverflowResult::Overflow) {
        return result;
    }
    return OverflowResult::NoOverflow;
}

}
