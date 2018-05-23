#include <cassert>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include "SMT/Middleware/AST.h"
using namespace std;
using namespace ceagle::smt;

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#include <vector>
#include <regex>
#include <exception>
#include <list>

struct assertion_error {};

template<typename T>
inline void EQUAL(const T& expect, const T& actual, const std::string& message="") {
    if (!(expect == actual)) {
        if (message != "") {
            cout << message << endl;
        }
        cout << "expect: " << expect << endl << "actual: " << actual << endl;
        throw assertion_error();
    }
}

template<typename T>
inline void RANGE_EQUAL(const T& expect, const T& actual, const std::string& message="") {
    if (!equal(expect.begin(), expect.end(), actual.begin(), actual.end())) {
        if (message != "") {
            cout << message << endl;
        }
        cout << "expect: [";
        for(auto& item: expect) {
            cout << item << " ";
        }
        cout << "]" << endl << "actual: [";
        for(auto& item: actual) {
            cout << item << " ";
        }
        cout << "]" << endl;
        throw assertion_error();
    }
}

class TestEvaluator : public Evaluator<int> {
public:
    virtual int handleAdd(const int& left, const int& right) override {
        return left + right;
    }
    virtual int handleVar(VarNode* var) override {
        assert(false);
    }
    virtual int handleIntValue(ValueNode<int>* node) override {
        return node->getValue();
    }
    virtual int handleBoolValue(ValueNode<bool>* node) override {
        assert(false);
    }
    virtual int handleBit1Value(BitValueNode<1>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit2Value(BitValueNode<2>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit4Value(BitValueNode<4>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit8Value(BitValueNode<8>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit16Value(BitValueNode<16>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit32Value(BitValueNode<32>* node) override {
        auto value = node->getValue();
        return value.to_ulong();
    }
    virtual int handleBit64Value(BitValueNode<64>* node) override {
        assert(false);
    }
};

int main() {
    ostringstream ostr;

    auto a = Var("a", IntSort());
    ostr << a.sort();
    EQUAL(ostr.str(), "Int"s);
    RANGE_EQUAL({"a"}, a.vars(), LINE_STRING);

    {
    auto a = Var("a", ArraySort(IntSort()));
    ostr = ostringstream();
    ostr << a.sort();
    EQUAL(ostr.str(), "(Array Int Int)"s);
    RANGE_EQUAL({"a"}, a.vars(), LINE_STRING);
    }

    {
    auto a = Var("a", ArraySort(IntSort(), BvSort(32)));
    ostr = ostringstream();
    ostr << a.sort();
    EQUAL(ostr.str(), "(Array (_ BitVec 32) Int)"s);
    RANGE_EQUAL({"a"}, a.vars(), LINE_STRING);
    }

    {
    auto a = Var("a", ArraySort(IntSort(), BvSort(32)));
    ostr = ostringstream();
    ostr << Sort(a.sort());
    EQUAL(ostr.str(), "(Array (_ BitVec 32) Int)"s);
    RANGE_EQUAL({"a"}, a.vars(), LINE_STRING);
    }

    Expr b = a;
    ostr = ostringstream();
    ostr << a.sort();
    EQUAL(ostr.str(), "Int"s);
    RANGE_EQUAL({"a"}, a.vars(), LINE_STRING);
    ostr = ostringstream();
    ostr << b.sort();
    EQUAL(ostr.str(), "Int"s);
    RANGE_EQUAL({"a"}, b.vars(), LINE_STRING);

    auto value = IntValue(10);
    auto expr = (a==value);
    ostr = ostringstream();
    ostr << a.sort();
    EQUAL(ostr.str(), "Int"s);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(= a 10)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Bool"s);

    auto expr_c = expr;
    EQUAL(expr_c.sort().str(), "Bool"s);

    ostr = ostringstream();
    expr = (Var("a", IntSort()) == value);
    ostr << expr;
    EQUAL(ostr.str(), "(= a 10)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Bool"s);

    b = a + value;
    ostr = ostringstream();
    ostr << a.sort();
    EQUAL(ostr.str(), "Int"s);
    ostr = ostringstream();
    ostr << b.sort();
    EQUAL(ostr.str(), "Int"s);
    expr = b;
    ostr = ostringstream();
    ostr << b.sort();
    EQUAL(ostr.str(), "Int"s);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(+ a 10)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Int"s);

    expr = ((b == IntValue(10)) && BoolValue(true)) == BoolValue(false);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(= (and (= (+ a 10) 10) true) false)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Bool"s);

    expr = !BoolValue(true);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(not true)"s);
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Bool"s);

    expr = BitValue(10, 32);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "#b00000000000000000000000000001010"s);

    expr = bvadd(Var("a", BvSort(32)), Var("b", BvSort(32)));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(bvadd a b)"s);
    RANGE_EQUAL({"a", "b"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "(_ BitVec 32)"s);

    ostr = ostringstream();
    ostr << expr.param(":named", "f1");
    EQUAL(ostr.str(), "(! (bvadd a b) :named f1)"s);
    RANGE_EQUAL({"a", "b"}, expr.vars());

    expr = bvneg(Var("a", BvSort(32)));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(bvneg a)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = bvxor(Var("a", BvSort(32)), Var("b", BvSort(32)));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(bvxor a b)"s);
    RANGE_EQUAL({"a", "b"}, expr.vars());

    expr = bvult(Var("a", BvSort(32)), Var("b", BvSort(32)));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(bvult a b)"s);
    RANGE_EQUAL({"a", "b"}, expr.vars());

    {
    auto f = FloatValue(5.2f);
    ostr = ostringstream();
    ostr << f;
    EQUAL(ostr.str(), "(fp #b0 #b10000001 #b01001100110011001100110)"s);
    RANGE_EQUAL({}, f.vars());
    }

    {
    auto f = FloatValue(-5.2f);
    ostr = ostringstream();
    ostr << f;
    EQUAL(ostr.str(), "(fp #b1 #b10000001 #b01001100110011001100110)"s);
    RANGE_EQUAL({}, f.vars());
    }

    auto d = FloatValue(5.2);
    ostr = ostringstream();
    ostr << d;
    EQUAL(ostr.str(), "(fp #b0 #b10000000001 #b0100110011001100110011001100110011001100110011001101)"s);
    RANGE_EQUAL({}, d.vars());

    expr = fpadd(Var("a", FloatSort()), Var("b", FloatSort()));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(fp.add RNE a b)"s);
    RANGE_EQUAL({"a", "b"}, expr.vars());

    expr = zext(Var("a", BvSort(1)), 1, 32);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ zero_extend 31) a)"s);
    EQUAL(expr.sort().str(), "(_ BitVec 32)"s);
    auto expr2 = expr;
    EQUAL(expr2.sort().str(), "(_ BitVec 32)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = sext(Var("a", BvSort(1)), 1, 32);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ sign_extend 31) a)"s);
    EQUAL(expr.sort().str(), "(_ BitVec 32)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = extract(Var("a", BvSort(32)), 32, 8);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ extract 7 0) a)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = concat(Var("a", BvSort(16)), Var("b", BvSort(32)));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(concat a b)"s);
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "(_ BitVec 48)"s);

    expr = fptofp(Var("a", FloatSort()), FloatSort(11, 53));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ to_fp 11 53) RNE a)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = bvcasttofp(Var("a", BvSort(64)), FloatSort(11, 53));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ to_fp 11 53) a)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "(_ FloatingPoint 11 53)"s);

    expr = fptosbv(Var("a", FloatSort()), 32);
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "((_ fp.to_sbv 32) RTZ a)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = fptoieee(Var("a", FloatSort()));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(to_ieee_bv a)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "(_ BitVec 32)"s);

    expr = ite(Var("a", BoolSort()), IntValue(1), IntValue(0));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(ite a 1 0)"s);
    RANGE_EQUAL({"a"}, expr.vars());

    expr = array_select(Var("a", ArraySort(IntSort())), IntValue(1));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(select a 1)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "Int"s);

    expr = array_store(Var("a", ArraySort(IntSort())), IntValue(1), IntValue(2));
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(store a 1 2)"s);
    RANGE_EQUAL({"a"}, expr.vars());
    ostr = ostringstream();
    ostr << expr.sort();
    EQUAL(ostr.str(), "(Array Int Int)"s);

    // test replace
    expr = Var("a", IntSort()) == IntValue(1);
    expr = expr.replace({{"a", Var("a", IntSort()) + IntValue(1)}});
    ostr = ostringstream();
    ostr << expr;
    EQUAL(ostr.str(), "(= (+ a 1) 1)"s, LINE_STRING);

    // test evaluate
    TestEvaluator evaluator;
    expr = IntValue(1) + IntValue(2);
    EQUAL(evaluator.evaluate(expr), 3);

    // test tokenlizer
    Tokenizer tokenizer;
    auto result = tokenizer.tokenize("true");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::BoolValue);

    result = tokenizer.tokenize("false");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::BoolValue);

    result = tokenizer.tokenize("false true");
    EQUAL(result.size(), 2ul);
    auto it = result.begin();
    EQUAL(it->type, TokenType::BoolValue);
    ++it;
    EQUAL(it->type, TokenType::BoolValue);

    result = tokenizer.tokenize("1");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::IntValue);

    result = tokenizer.tokenize("12");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::IntValue);

    result = tokenizer.tokenize("#b11");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::BvValue);

    result = tokenizer.tokenize("x");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::Var);

    result = tokenizer.tokenize("*");
    EQUAL(result.size(), 1ul);
    EQUAL(result.begin()->type, TokenType::Bop);

    result = tokenizer.tokenize("()");
    EQUAL(result.size(), 2ul);
    it = result.begin();
    EQUAL(it->type, TokenType::OpenBracket);
    ++it;
    EQUAL(it->type, TokenType::CloseBracket);

    result = tokenizer.tokenize("(+ x 1)");
    EQUAL(result.size(), 5ul);

    Parser p({{"x", IntSort()}});
    expr = p.parse("(+ (+ 1 x) (+ 1 x))");
    EQUAL(expr.str(), "(+ (+ 1 x) (+ 1 x))"s);

    expr = p.parse("(not (>= x 2))");
    EQUAL(expr.str(), "(not (>= x 2))"s);

    expr = p.parse("#xff");
    EQUAL(expr.str(), "#b11111111"s);

    // test parse
    //expr = Expr::parse("true", {});
    //EQUAL(BoolValue("true").str(), expr.str());

    //expr = Expr::parse("false", {});
    //EQUAL(BoolValue("false").str(), expr.str());

    return 0;
}
