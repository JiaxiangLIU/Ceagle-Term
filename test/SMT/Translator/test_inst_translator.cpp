#include <functional>
#include <string>
#include <iostream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>

#include "SMT/Translator/InstTranslator.h"

#include "test/support.h"

using namespace llvm;
using namespace ceagle;

size_t tab_level = 0;

#define DISABLE(name, block) {}

void DESCRIBE(std::string message, std::function<void()>block) {
    tab_level ++;
    std::cout << std::string(tab_level, '\t') << message << std::endl;
    block();
    tab_level --;
}

auto IT =  &DESCRIBE;


int main() {
    DISABLE("InstTranslator", []{
        std::string pathPrefix = "../examples/sv/signedintegeroverflow-regression/";
        std::string fileName = pathPrefix + "AdditionIntMin_false-no-overflow.c.i";
        std::unique_ptr<Module> module = fileToModule(fileName, "clang-3.7");
        assert(module);
        auto mainFunction = module->getFunction("main");
        auto& entry = *mainFunction->begin();

        InstTranslator translator;
        auto it = entry.begin();
        Expr actual = translator.translate(it);
        Expr expect = smt::Var("retval", smt::BvSort(32));
#define CHECK_EXPR(actual, expect) \
        EQUAL(actual.str(), expect.str()); \
        EQUAL(actual.sort().str(), expect.sort().str())
        CHECK_EXPR(actual, expect);
        it++;
        actual = translator.translate(it);
        expect = smt::Var("minInt", smt::BvSort(32));
        CHECK_EXPR(actual, expect);
        it++;
        actual = translator.translate(it);
        expect = smt::Var("x", smt::BvSort(32));
        CHECK_EXPR(actual, expect);
        it++;
        actual = translator.translate(it);
        expect = smt::BitValue(0, 32);
        CHECK_EXPR(actual, expect);
        CHECK_EXPR(translator.getValueMap()["retval"], expect);
        it++;
        actual = translator.translate(it);
        expect = smt::Expr();
        EQUAL(actual.str(), expect.str());
        it++;
        actual = translator.translate(it);
        expect = smt::BitValue(-2147483648LL, 32);
        CHECK_EXPR(actual, expect);
        CHECK_EXPR(translator.getValueMap()["minInt"], expect);
        it++;
        actual = translator.translate(it);
        expect = smt::Expr();
        EQUAL(actual.str(), expect.str());
        it++;
        actual = translator.translate(it);
        expect = smt::Var("minInt", smt::BvSort(32));
        CHECK_EXPR(actual, expect);
        it++;
        actual = translator.translate(it);
        expect = bvadd(smt::Var("minInt", smt::BvSort(32)), smt::BitValue(-1, 32));
        CHECK_EXPR(actual, expect);
        CHECK_EXPR(translator.getValueMap()["add"], expect);
        it++;
        actual = translator.translate(it);
        expect = bvadd(smt::Var("add", smt::BvSort(32)), smt::BitValue(23, 32));
        CHECK_EXPR(actual, expect);
        CHECK_EXPR(translator.getValueMap()["add1"], expect);
        it++;
        actual = translator.translate(it);
        expect = Var("add1", BvSort(32));
        CHECK_EXPR(actual, expect);
        CHECK_EXPR(translator.getValueMap()["x"], expect);
        it++;
        actual = translator.translate(it);
        expect = Var("x", BvSort(32));
        CHECK_EXPR(actual, expect);
        it++;
        actual = translator.translate(it);
        expect = smt::Expr();
        EQUAL(actual.str(), expect.str());
        it++;
        actual = translator.translate(it);
        expect = smt::Expr();
        EQUAL(actual.str(), expect.str());
    });
    DESCRIBE("TranslatorVisitor", []{
        auto module = fileToModule("../test/res/parity_true-unreach-call.ll", "clang-3.7");
        auto mainFunction = module->getFunction("main");
        auto it = mainFunction->begin();
        IT("translate entry", [&]{
            TranslatorVisitor visitor;
            auto &entry = *it;
            visitor.visit(entry);
            EQUAL(visitor.get("retval"), BitValue(0, 32));
            EQUAL(visitor.get("v"), Var("call", BvSort(32)));
            EQUAL(visitor.get("v1"), Var("call", BvSort(32)));
            // EQUAL(visitor.get("v2"), Var("v2", BvSort(32)));
            EQUAL(visitor.get("parity1"), BitValue(0, 8));
            EQUAL(visitor.get("lv_0"), Var("call", BvSort(32)));
        });
        IT("translate while.cond", [&]{
            ++it; //while.cond
            auto &whileCond = *it;
            TranslatorVisitor visitor;
            visitor.visit(whileCond);
            EQUAL(visitor.get("lv_1"), Var("v1", BvSort(32)));
            EQUAL(visitor.get("cmp"), !(Var("v1", BvSort(32)) == BitValue(0, 32)));
            EQUAL(visitor.getCondition(), visitor.get("cmp"));
        });
        IT("translate while.body", [&]{
            ++it;
            auto &whileBody = *it;
            TranslatorVisitor visitor;
            visitor.visit(whileBody);
            Expr expect = Var("parity1", BvSort(8));
            EQUAL(visitor.get("lv_2"), expect);
            expect = sext(expect, 8, 32);
            EQUAL(visitor.get("conv"), expect);
            expect = expect == BitValue(0, 32);
            EQUAL(visitor.get("cmp1"), expect);
        });
        IT("translate if.then", [&]{
            ++it;
            auto &ifThen = *it;
            TranslatorVisitor visitor;
            visitor.visit(ifThen);
            Expr expect = BitValue(1, 8);
            EQUAL(visitor.get("parity1"), expect);
        });
        IT("translate if.else", [&]{
            ++it;
            auto &ifElse = *it;
            TranslatorVisitor visitor;
            visitor.visit(ifElse);
            Expr expect = BitValue(0, 8);
            EQUAL(visitor.get("parity1"), expect);
        });
        IT("translate if.end", [&]{
            ++it;
            auto &block = *it;
            TranslatorVisitor visitor;
            visitor.visit(block);
            Expr expect = Var("v1", BvSort(32));
            EQUAL(visitor.get("lv_3"), expect);
            EQUAL(visitor.get("lv_4"), expect);
            auto sub = bvsub(expect, BitValue(1, 32));
            EQUAL(visitor.get("sub"), sub);
            auto _and = bvand(expect, sub);
            EQUAL(visitor.get("and"), _and);
            auto v1 = _and;
            EQUAL(visitor.get("v1"), _and);
        });
        IT("translate while.end", [&]{
            ++it;
            auto &block = *it;
            TranslatorVisitor visitor;
            visitor.visit(block);
            auto lv_5 = Var("v", BvSort(32));
            Expr v2 = lv_5;
            auto parity2 = BitValue(0, 8);
            auto lv_6 = v2;
            auto lv_7 = v2;
            auto shr = bvlshr(lv_7, BitValue(1, 32));
            auto _xor = bvxor(lv_6, shr);
            v2 = _xor;
            auto lv_8 = v2;
            auto lv_9 = v2;
            auto shr3 = bvlshr(lv_9, BitValue(2, 32));
            auto xor4 = bvxor(lv_8, shr3);
            v2 = xor4;
            auto lv_10 = v2;
            auto and5 = bvand(lv_10, BitValue(286331153, 32));
            auto mul = bvmul(and5, BitValue(286331153, 32));
            v2 = mul;
            auto lv_11 = v2;
            auto shr6 = bvlshr(lv_11, BitValue(28, 32));
            auto and7 = bvand(shr6, BitValue(1, 32));
            auto cmp8 = and7 == BitValue(0, 32);
            EQUAL(visitor.get("cmp8"), cmp8);
        });
    });
    DESCRIBE("translator for gcd", []{
        auto module = fileToModule("../test/res/test_gcd.ll", "clang-3.7");
        assert(module);
        auto mainFunction = module->getFunction("main");
        auto it = mainFunction->begin(); //entry
        std::vector<BasicBlock*> path;
        path.push_back(&(*it));
        ++it; // land.lhs.true
        path.push_back(&(*it));
        std::map<Value*, Expr> valueMap;
        auto translator = new VisitorTranslator();
        auto cond = translator->path2SMTM(path, valueMap);
        for (auto pa: valueMap) {
            std::cout << pa.first->getName().str() << " " << pa.second << " " << pa.second.sort() << "\n";
        }
    });
    DESCRIBE("VisitorTranslator", []{
        auto module = fileToModule("../test/res/parity_true-unreach-call.ll", "clang-3.7");
        assert(module);
        auto mainFunction = module->getFunction("main");
        auto it = mainFunction->begin(); //entry
        std::vector<BasicBlock*> path;
        //path.push_back(&(*it));
        ++it; // while.cond
        path.push_back(&(*it));
        ++it; // while.body
        ++it; // if.then
        ++it; // if.else
        ++it; // if.end
        ++it; // while.end
        path.push_back(&(*it));
        auto translator = new VisitorTranslator();
        auto cond = translator->path2SMTM(path);
        auto whileCond = !(!(Var("v1", BvSort(32)) == BitValue(0, 32)));
        auto expect = BoolValue(true) && whileCond;
        EQUAL(cond.str(), expect.str());

        ++it; // if.then.10
        ++it; // if.else.11
        path.push_back(&(*it));
        translator = new VisitorTranslator();
        cond = translator->path2SMTM(path);
        auto lv_5 = Var("v", BvSort(32));
        Expr v2 = lv_5;
        auto parity2 = BitValue(0, 8);
        auto lv_6 = v2;
        auto lv_7 = v2;
        auto shr = bvlshr(lv_7, BitValue(1, 32));
        auto _xor = bvxor(lv_6, shr);
        v2 = _xor;
        auto lv_8 = v2;
        auto lv_9 = v2;
        auto shr3 = bvlshr(lv_9, BitValue(2, 32));
        auto xor4 = bvxor(lv_8, shr3);
        v2 = xor4;
        auto lv_10 = v2;
        auto and5 = bvand(lv_10, BitValue(286331153, 32));
        auto mul = bvmul(and5, BitValue(286331153, 32));
        v2 = mul;
        auto lv_11 = v2;
        auto shr6 = bvlshr(lv_11, BitValue(28, 32));
        auto and7 = bvand(shr6, BitValue(1, 32));
        auto cmp8 = and7 == BitValue(0, 32);
        auto ifElseCond = !cmp8;
        expect = BoolValue(true) && whileCond && ifElseCond;
        EQUAL(cond.str(), expect.str());

        ++it; // if.end.12
        path.push_back(&(*it));
        ++it; // bb_14
        ++it; // inline
        ++it; // error
        path.push_back(&(*it));
        translator = new VisitorTranslator();
        cond = translator->path2SMTM(path);
    });
    return 0;
}

