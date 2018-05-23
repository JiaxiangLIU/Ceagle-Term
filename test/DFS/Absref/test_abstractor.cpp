#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "test/support.h"

#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Solver.h"

#include "DFS/Absref/AbstractState.h"
#include "DFS/Absref/PredicateAbstractor.h"

size_t tab_level = 0;

void DESCRIBE(std::string message, std::function<void()>block) {
    tab_level ++;
    std::cout << std::string(tab_level, '\t') << message << std::endl;
    block();
    tab_level --;
}

auto IT =  &DESCRIBE;


using namespace ceagle;
using namespace ceagle::smt;

int main() {
    DESCRIBE("Precision", []{
        DESCRIBE("constructor", []{
            IT("has empty constructor", []{
                Precision p;
            });
            IT("has constructor of vector<Expr>", []{
                std::vector<Expr> s;
                Precision p(s);
            });
        });
        IT("cat get by index", []{
            std::vector<Expr> s;
            Expr e = (Var("x", IntSort()) == IntValue(1));
            s.push_back(e);
            Precision p(s);
            EQUAL(p.get(1).str(), e.str());
        });
    });
    DESCRIBE("Abstractor", []{
        DESCRIBE("Abstractor::abstract", []{
            IT("should return same AbstractState for empty BasicBlock", []{
                Precision p;
                PredicateAbstractor abstractor(p);
                AbstractState state1;
                EQUAL(*abstractor.abstract(state1), state1);
            });
            IT("should return correct AbstractState for unsat condition", []{
                // p: x=1
                // state !b1
                // map: {}
                // cond: x = 1
                std::vector<Expr> s;
                Expr e = (Var("x", IntSort()) == IntValue(1));
                s.push_back(e);
                Precision p(s);
                p.addPrecision(nullptr, 1);
                AbstractState state({-1});
                auto cond = Var("x", IntSort()) == IntValue(1);
                PredicateAbstractor abstractor(p);
                ISNULL(abstractor.abstract(state, {}, cond));
            });
            IT("should work when predicat is false", []{
                // p: false
                // state: true
                // map: {}
                // cond: true
                // expect: !b1
                std::vector<Expr> s;
                Expr e = BoolValue(false);
                s.push_back(e);
                AbstractState start;
                Precision p(s);
                p.addPrecision(nullptr, 1);
                PredicateAbstractor a(p);
                auto result = a.abstract(start, {});
                AbstractState expect({-1});
                EQUAL(*result, expect);
            });
            IT("should return correct AbstractState", []{
                // precision: b1 (= x 1)
                // state: b1
                // map: x = x + 1
                // after: !b1
                std::vector<Expr> s;
                Expr e = (Var("x", IntSort()) == IntValue(1));
                s.push_back(e);
                AbstractState state({1});
                Expr newX = (Var("x", IntSort()) + IntValue(1));
                std::map<std::string, Expr > valueMap;
                valueMap["x"] = newX;
                Precision p(s);
                p.addPrecision(nullptr, 1);
                PredicateAbstractor abstractor(p);
                auto result = abstractor.abstract(state, valueMap);
                AbstractState expect({-1});
                EQUAL(*result, expect);
            });
        });
        DESCRIBE("refine", []{
            Expr c1 = Var("x", IntSort()) > IntValue(0);
            IT("should refine value map and condition", []{
            Precision p;
            PredicateAbstractor a(p);
            std::map<std::string, Expr> vm1 = {
                {"retval", IntValue(0)},
                {"x", IntValue(0)}
            };
            Expr c1 = BoolValue(true);
            Expr c2 = Var("x", IntSort()) >= IntValue(2);
            auto result = a.refine(vm1, c1, c2);
            EQUAL(result.size(), 1);
            EQUAL(result[0].str(), (!(Var("x", IntSort()) >= IntValue(2))).str());
            });
            IT("should refine value map and condition 2", []{
            Precision p;
            PredicateAbstractor a(p);
            std::map<std::string, Expr> vm1 = {
                {"retval", IntValue(0)},
                {"x", IntValue(0)},
                {"lv_0", IntValue(0)},
                {"cmp", IntValue(0) < IntValue(2)}
            };
            Expr c1 = IntValue(0) >= IntValue(2);
            Expr c2 = BoolValue(true);
            auto result = a.refine(vm1, c1, c2);
            EQUAL(result.size(), 1);
            EQUAL(result[0].str(), BoolValue(false).str());
            });
            IT("should refine AbstractState path", []{
                auto module = fileToModule("../test/res/simple.ll", "clang-3.7");
                Precision p;
                PredicateAbstractor a(p);
                auto mainFunction = module->getFunction("main");
                auto it = mainFunction->begin();
                auto entry = &(*it);
                ++it;
                auto whileCond = &(*it);
                ++it; //while.body
                ++it; //if.then
                ++it; //if.else
                ++it; //if.end
                ++it; //while.end
                auto whileEnd = &(*it);
                ++it; //return
                ++it; //inline
                ++it; //error
                auto err = &(*it);
                std::shared_ptr<AbstractState> s1(new AbstractState(entry));
                std::shared_ptr<AbstractState> s2(new AbstractState(whileCond));
                std::shared_ptr<AbstractState> s3(new AbstractState(whileEnd));
                std::shared_ptr<AbstractState> s4(new AbstractState(err));
                a.refine({s1, s2, s3, s4});
                Precision& after = a.getPrecision();
                std::cout << after.get(1) << std::endl;
                std::cout << after.get(2) << std::endl;
                EQUAL(after.size(), 2);
            });
        });
    });
    return 0;
}
