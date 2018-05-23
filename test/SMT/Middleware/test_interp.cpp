#include <cassert>
#include <iostream>
#include <sstream>
#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"
using namespace std;
using namespace ceagle::smt;
#ifndef EQUAL
#define EQUAL(actual, expect) \
    cout << "expect: " << expect << endl << "actual: " << actual << endl; \
    assert(expect == actual);
#endif
int main() {
    auto test = []() {
        auto s = Z3RawSolver(true);
        s.add(std::move((Var("a", IntSort()) == Var("b", IntSort())).param(":named", "f1")));
        s.add(std::move((Var("a", IntSort()) == Var("c", IntSort())).param(":named", "f2")));
        s.add(std::move((Var("b", IntSort()) == Var("d", IntSort())).param(":named", "f3")));
        s.add(std::move((!(Var("c", IntSort()) == Var("d", IntSort()))).param(":named", "f4")));
        EQUAL(CheckResult::UNSAT, s.checkSat())
        auto result = s.getInterp({"f1", "f2", "f3", "f4"});
        EQUAL(result.size(), 3)
        EQUAL((string)result[0], "(= a b)")
        EQUAL((string)result[1], "(= b c)")
        EQUAL((string)result[2], "(= c d)")
    };
    test();
    return 0;
}
