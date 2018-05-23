#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"
#include "SMT/Middleware/popen.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>

using namespace std;
using namespace ceagle::smt;
#ifndef EQUAL
#define EQUAL(expect, actual) \
    cout << "expect: " << expect << endl << "actual: " << actual << endl; \
    assert(expect == actual);
#endif

int main() {
    //{
    //char* const args[2] = {"z3", "-in"};
    //Popen p("/usr/local/bin/z3", args);
    //p << "(echo \"1\")\n";
    //ostringstream out;
    //out << p;
    //EQUAL(out.str(), "1\n")
    //}
    auto test1 = []() {
        cout << "test1" << endl;
        auto s = Z3RawSolver();
        s.add(Var("a", BoolSort()) == BoolValue(true));
        EQUAL(CheckResult::SAT, s.checkSat())
        s.push();
        s.add(Var("a", BoolSort()) == BoolValue(false));
        EQUAL(CheckResult::UNSAT, s.checkSat())
        s.pop();
        EQUAL(CheckResult::SAT, s.checkSat())
        auto model = s.getModel();
        EQUAL("true", model["a"])
        EQUAL("a: true\n", model.str())
    };
    test1();
    auto test2 = []() {
        cout << "test2" << endl;
        auto s = Z3RawSolver();
        s.push();
        s.add(Var("a", BoolSort()) == BoolValue(true));
        s.pop();
        s.add(Var("a", BoolSort()) == BoolValue(true));
        EQUAL(CheckResult::SAT, s.checkSat())
        auto model = s.getModel();
        EQUAL("true", model["a"])
    };
    auto test3 = []() {
        cout << "test3" << endl;
        auto s = Z3RawSolver();
        s.add(BoolValue(true) == BoolValue(false));
        EQUAL(CheckResult::UNSAT, s.checkSat())
        s.reset();
        EQUAL(CheckResult::SAT, s.checkSat())
    };
    auto test4 = []() {
        auto s = Z3RawSolver();
        Expr e = BoolValue(true) && BoolValue(true);
        e = e && !bvslt(zext(BitValue(0, 8), 8, 32), BitValue(8, 32));
        e = e && (Var("c", BvSort(8)) == BitValue(0, 8));
        e = e && (Var("cmp", BoolSort()) == bvslt(zext(BitValue(0, 8), 8, 32), BitValue(8, 32)));
        e = e && (Var("conv", BvSort(32)) == zext(BitValue(0, 8), 8, 32));
        s.add(e);
        EQUAL(CheckResult::UNSAT, s.checkSat())
    };
    auto test5 = []() {
        auto s = Z3RawSolver();
        Expr e = BoolValue(true) && BoolValue(true);
        e = e && !bvslt(zext(BitValue(0, 8), 8, 32), BitValue(8, 32));
        e = e && (Var("c", BvSort(8)) == BitValue(0, 8));
        e = e && (Var("cmp", BoolSort()) == bvslt(sext(BitValue(0, 8), 8, 32), BitValue(8, 32)));
        e = e && (Var("lll", BvSort(32)) == sext(BitValue(0, 8), 8, 32));
        s.add(e);
        EQUAL(CheckResult::UNSAT, s.checkSat())
    };
    try {
        test2();
        test3();
        test4();
        test5();
    } catch(string err) {
        cout << err;
        throw err;
    }
    return 0;
}
