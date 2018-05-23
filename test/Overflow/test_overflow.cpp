#include <cstdio>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
using namespace llvm;

#include "test/support.h"

#include "Overflow/overflow.h"

#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/popen.h"

using namespace ceagle;
using namespace ceagle::smt;

size_t tab_level = 0;

void DESCRIBE(std::string message="", std::function<void()>block=[]{}) {
        tab_level ++;
            std::cout << std::string(tab_level, '\t') << message << std::endl;
                block();
                    tab_level --;
}

auto IT =  &DESCRIBE;

int main() {
    DESCRIBE("OverflowResult", []{
        IT("should be an enum class", []{
            OverflowResult result;
            result = OverflowResult::Unknown;
            result = OverflowResult::Overflow;
            result = OverflowResult::NoOverflow;
        });
    });
    DESCRIBE("verifyOverflow", []{
        DESCRIBE("benchmack", []{
            std::vector<std::string> fileList = {
"AdditionIntMax_false-no-overflow.c.i",
"Multiplication_false-no-overflow.c.i",
"NoConversion_false-no-overflow.c.i",
"AdditionIntMin_false-no-overflow.c.i",
"ConversionToSignedInt_true-no-overflow.c.i",
"Division_false-no-overflow.c.i",
"IntegerPromotion_true-no-overflow.c.i",
"Multiplication_true-no-overflow.c.i",
"NoNegativeIntegerConstant_true-no-overflow.c.i",
"PostfixDecrement_false-no-overflow.c.i",
"PostfixIncrement_false-no-overflow.c.i",
"PrefixDecrement_false-no-overflow.c.i",
"PrefixIncrement_false-no-overflow.c.i",
"UnaryMinus_false-no-overflow.c.i",
"UsualArithmeticConversions_true-no-overflow.c.i"
            };
            std::string pathPrefix = "../examples/sv/signedintegeroverflow-regression/";
            for (std::string& file: fileList) {
                std::cout << "file: " << file << "\n";
                auto result = verifyOverflow(pathPrefix + file, "clang-3.7");
                EQUAL(result, (file.find("true") != std::string::npos) ? OverflowResult::NoOverflow : OverflowResult::Overflow);
            }
        });
    });
    return 0;
}
