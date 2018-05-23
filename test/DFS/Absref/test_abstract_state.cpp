#include <string>

#include <llvm/IR/LLVMContext.h>
#include "DFS/Absref/AbstractState.h"

#include "test/support.h"

size_t tab_level = 0;

void DESCRIBE(std::string message, std::function<void()>block) {
    tab_level ++;
    std::cout << std::string(tab_level, '\t') << message << std::endl;
    block();
    tab_level --;
}

auto IT =  &DESCRIBE;

using namespace ceagle;
using namespace llvm;

int main() {
    DESCRIBE("AbstractState", []{
        IT("should be a class", []{
            AbstractState* state;
        });
        IT("can be cout", []{
            AbstractState state;
            std::cout << state;
        });
        IT("should correct cout", []{
            AbstractState state;
            std::ostringstream o_str;
            o_str << state;
            EQUAL(o_str.str(), "true");

            o_str = std::ostringstream();
            state = AbstractState({1, 2});
            o_str << state;
            EQUAL(o_str.str(), "b1 && b2")

            o_str = std::ostringstream();
            state = AbstractState({-1, 2});
            o_str << state;
            EQUAL(o_str.str(), "!b1 && b2")
        });
        IT("can compare neq", []{
            AbstractState state1;
            AbstractState state2;
            EQUAL(state1 == state2, true);
        });
        IT("can correct compare", []{
            AbstractState state1({1, 2});
            AbstractState state2({2, 3});
            EQUAL(state1 == state2, false);

            state1 = AbstractState({1, 2});
            state2 = AbstractState({2, 1});
            EQUAL(state1 == state2, true);
        });
        IT("can get precision", []{
            AbstractState state1({1, 2});
            std::set<int> expect{1, 2};
            EQUALSET(state1.getPrecision(), expect);
        });
        IT("can get precision", []{
            AbstractState state1({-1, 2});
            std::set<int> expect{1, 2};
            EQUALSET(state1.getPrecision(), expect);
        });
        DESCRIBE("cover", []{
            IT("should have same location", []{
                LLVMContext& context = getGlobalContext();
                BasicBlock* bb1 = BasicBlock::Create(context);
                BasicBlock* bb2 = BasicBlock::Create(context);
                AbstractState s1(bb1);
                AbstractState s2(bb2);
                EQUAL(s1.cover(s2), false);
                EQUAL(s2.cover(s1), false);
                EQUAL(s1.cover(s1), true);
                EQUAL(s2.cover(s2), true);
            });
            IT("should correct", []{
                LLVMContext& context = getGlobalContext();
                BasicBlock* bb1 = BasicBlock::Create(context);
                BasicBlock* bb2 = BasicBlock::Create(context);
                AbstractState s1(bb1, {1, 2});
                AbstractState s2(bb1, {1});
                EQUAL(s2.cover(s1), true);
                EQUAL(s1.cover(s2), false);
            });
        });
        DESCRIBE("next", []{
            auto module = fileToModule("../test/res/jain_1_true_unreach-call.ll", "clang-3.7");
            auto mainFunction = module->getFunction("main");
            auto it = mainFunction->begin();
            std::shared_ptr<AbstractState> start(new AbstractState(&(*it)));
            std::vector<Expr> s;
            Expr e = (Var("y", BvSort(32)) == BitValue(1, 32));
            s.push_back(e);
            Precision p(s);
            auto nextLocation = &(*(++it));
            p.addPrecision(nextLocation, 1);
            PredicateAbstractor abstractor(p);
            auto result = abstractor.next(start);
            EQUAL(result.size(), 1);
            auto nextState = *result[0];
            AbstractState expect(nextLocation, {1});
            EQUAL(nextState, expect);
        });
        DESCRIBE("AbstractReachedSet", [](){
            DESCRIBE("getPath", [](){
               std::shared_ptr<AbstractState> state(new AbstractState());
               AbstractReachedSet reached(state);;
               std::shared_ptr<AbstractState> state2(new AbstractState());
               reached.add(state, state2);
               auto path = reached.getPath(state2);
               EQUAL(path.size(), 2);
               std::shared_ptr<AbstractState> state3(new AbstractState());
               reached.add(state2, state3);
               path = reached.getPath(state3);
               EQUAL(path.size(), 3);
            });
        });
    });
    return 0;
}
