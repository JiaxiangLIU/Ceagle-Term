#include "DFS/CEGAR.h"
#include "test/support.h"

#include <llvm/IR/Module.h>

#include <string>
#include <functional>
#include <iostream>
size_t tab_level = 0;

void DESCRIBE(std::string message, std::function<void()>block) {
    tab_level ++;
    std::cout << std::string(tab_level, '\t') << message << std::endl;
    block();
    tab_level --;
}

auto IT =  &DESCRIBE;

using namespace llvm;
using namespace ceagle;

int main() {
    DESCRIBE("cegar", []{
//        auto module = fileToModule("../test/res/parity_true-unreach-call.ll", "clang-3.7");
//        CEGAR cegar;
//        cegar.visit(*module);
//        for (auto b: cegar.getErrorPath()) {
//            std::cout << b->location()->getName().str() << " ";
//        }
//        std::cout << "\n";
//        auto result = cegar.getResult();
//        assert(result == DFSResult::DR_SAFE);
    });
    return 0;
}
