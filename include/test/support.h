#ifndef CEAGLE_TEST_SUPPORT_H
#define CEAGLE_TEST_SUPPORT_H
#include <cassert>
#include <iostream>
#include <sstream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

#ifndef EQUAL
#define EQUAL(actual, expect) { \
    auto actual_val = actual; \
    auto expect_val = expect; \
    std::cout << "expect: " << expect_val << "\nactual: " << actual_val << "\n"; \
    assert(std::equal_to<decltype(actual_val)>{}(actual_val, expect_val)); \
}
#endif
#ifndef EQUALSET
#define EQUALSET(expect, actual) \
{ \
    std::cout << "expect: "; \
    for (auto &i : expect) { \
        std::cout << i << " "; \
    } \
    std::cout << "\nactual: "; \
    for (auto &i : actual) { \
        std::cout << i << " "; \
    } \
    std::cout << "\n"; \
    assert(expect == actual); \
}
#endif
#ifndef ISNULL
#define ISNULL(actual) \
    std::cout << "expect: " << "nullptr" << "\nactual: " << actual << "\n"; \
    assert(actual == nullptr);

#endif
inline std::unique_ptr<llvm::Module> fileToModule(std::string fileName, std::string compilerName) {
    std::string fnext = fileName.substr(fileName.find_last_of(".") + 1);
    std::string fnll = fileName;
    if (fnext != "ll") {
        fnll = fileName + ".ll";
        std::string command = compilerName + " -g -S -emit-llvm " + fileName + " -o " + fnll;
        FILE* p = popen(command.c_str(), "r");
        assert(p);
        pclose(p);
    }
    llvm::LLVMContext &context = llvm::getGlobalContext();
    llvm::SMDiagnostic err;
    std::unique_ptr<llvm::Module> module = llvm::parseIRFile(fnll.c_str(), err, context);
    assert(module);
    return std::move(module);
}

#endif
