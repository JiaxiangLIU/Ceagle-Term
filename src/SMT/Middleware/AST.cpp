#include "AST.h"
#include <vector>
#include <set>
#include <string>
#include <map>

using namespace std;

namespace {

using namespace ceagle::smt;

void visitNode(inode<Node>* start, std::function<void(inode<Node>*)> f) {
    std::vector<inode<Node>*> waitList;
    waitList.push_back(start);
    while(waitList.size() > 0) {
        auto current = waitList.back();
        waitList.pop_back();
        auto next = current->getNext();
        auto child = current->getChild();
        if (next != nullptr) {
            waitList.push_back(next);
        }
        if (child != nullptr) {
            waitList.push_back(child);
        }
        f(current);
    }
}

set<string> findVars(inode<Node>* start) {
    std::set<std::string> results;
    auto f = [&results](inode<Node>* node) {
        auto value = node->getValue();
        auto vars = value->vars();
        results.insert(vars.begin(), vars.end());
    };
    visitNode(start, f);
    return results;
}

} // end namespace

namespace ceagle {

namespace smt {

ostream& operator<<(ostream& out, const Expr& expr){
    auto& root = expr._root;
    if (root) {
        out << *root;
    }
    return out;
}

ostream& operator<<(ostream& out, const inode<Node>& node) {
    if (node.value->isLeaf()) {
        out << *node.value;
    } else {
        out << "(" << *node.value;
        auto child = node.child.get();
        while (child) {
            out << " " << *child;
            child = child->next.get();
        }
        out << ")";
    }
    return out;
}

template <>
void ValueNode<bool>::print(std::ostream& out) const {
    if (value) {
        out << "true";
    } else {
        out << "false";
    }
}

std::set<std::string> Expr::vars() const {
    return findVars(_root.get());
}

Expr Expr::replace(std::map<std::string, Expr>varMap) const {
    Expr result(*this);
    visitNode(result._root.get(),[varMap](inode<Node>* node) mutable {
        auto value = node->getValue();
        if (auto varNode = dynamic_cast<VarNode*>(value)) {
            auto varName = varNode->name();
            if (varMap.count(varName) == 1) {
                node->replaceSubTree(*(varMap[varName]._root));
            }
        }
    });
    return result;
}

std::set<std::string> HighOrderOperatorNode::vars() const {
    return findVars(_root.get());
}

std::ostream& operator<<(std::ostream& out, TokenType type) {
    switch (type) {
    case TokenType::Var:
        out << "Var";
        break;
    case TokenType::BoolValue:
        out << "BoolValue";
        break;
    case TokenType::IntValue:
        out << "IntValue";
        break;
    case TokenType::BvValue:
        out << "BvValue";
        break;
    case TokenType::Uop:
        out << "Uop";
        break;
    case TokenType::Bop:
        out << "Bop";
        break;
    case TokenType::OpenBracket:
        out << "(";
        break;
    case TokenType::CloseBracket:
        out << ")";
        break;
    case TokenType::EMPTY:
        out << "";
        break;
    }
    return out;
}


Tokenizer::Tokenizer() {
    patterns.push_back({TokenType::BoolValue, std::regex("^(true|false)")});
    patterns.push_back({TokenType::IntValue, std::regex("^([1-9][0-9]*)")});
    patterns.push_back({TokenType::BvValue, std::regex("^(#b[0-1]+|#x[0-9a-f]+)")});
    patterns.push_back({TokenType::Uop, std::regex("^(bvneg|not|bvnot)")});
    patterns.push_back({TokenType::Bop, std::regex("^(=|\\+|-|\\*|/|>=?|<=?|and|or|bvadd|bvsub|bvmul|bvudiv|bvsdiv|bvurem|bvsrem|bvsmod|bvshl|bvlshr|bvashr|bvor|bvand|bvnand|bvnor|bvxor|bvule|bvult|bvule|bvuge|bvugt|bvsle|bvslt|bvsge|bvsgt|concat)")});
    patterns.push_back({TokenType::OpenBracket, std::regex("^(\\()")});
    patterns.push_back({TokenType::CloseBracket, std::regex("^(\\))")});
    patterns.push_back({TokenType::Var, std::regex("^([a-zA-Z][\\._a-zA-Z0-9]*|_[\\._a-zA-Z0-9]+)")});
}
std::list<Token> Tokenizer::tokenize(std::string str) {
    std::list<Token> result;
    // trim leading space
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
    while(str != "") {
        std::smatch base_match;
        bool find = false;
        for (auto p: patterns) {
            if (std::regex_search(str, base_match, p.second)) {
                if (base_match.size() >=  2) {
                    find = true;
                    std::ssub_match base_sub_match = base_match[1];
                    std::string base = base_sub_match.str();
                    result.push_back({p.first, base});
                    str = std::regex_replace(str, p.second, "", std::regex_constants::format_first_only);
                    break;
                }
            }
        }
        if (!find) {
            throw ParseException("Unknown token at " + str);
        }
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
    }
    return result;
}

Parser::Parser(std::map<std::string, Sort> _vars) :
    tokenizer(),
#define item(code, str) \
    {#str, code}
    opMap{
        item(OPCODE::EQU, =),
        item(OPCODE::ADD, +),
        item(OPCODE::MINUS, -),
        item(OPCODE::MULTIPLY, *),
        item(OPCODE::DEVIDE, /),
        item(OPCODE::REM, rem),
        item(OPCODE::TO_INT, to_int),
        item(OPCODE::LT, <),
        item(OPCODE::GT, >),
        item(OPCODE::LE, <=),
        item(OPCODE::GE, >=),
        item(OPCODE::AND, and),
        item(OPCODE::OR, or),
        item(OPCODE::NOT, not),
        item(OPCODE::BV_ADD, bvadd),
        item(OPCODE::BV_SUB, bvsub),
        item(OPCODE::BV_NEG, bvneg),
        item(OPCODE::BV_MUL, bvmul),
        item(OPCODE::BV_UDIV, bvudiv),
        item(OPCODE::BV_SDIV, bvsdiv),
        item(OPCODE::BV_UREM, bvurem),
        item(OPCODE::BV_SREM, bvsrem),
        item(OPCODE::BV_SMOD, bvsmod),
        item(OPCODE::BV_SHL, bvshl),
        item(OPCODE::BV_LSHR, bvlshr),
        item(OPCODE::BV_ASHR, bvashr),
        item(OPCODE::BV_OR, bvor),
        item(OPCODE::BV_AND, bvand),
        item(OPCODE::BV_NOT, bvnot),
        item(OPCODE::BV_NAND, bvnand),
        item(OPCODE::BV_NOR, bvnor),
        item(OPCODE::BV_XOR, bvxor),
        item(OPCODE::BV_ULE, bvule),
        item(OPCODE::BV_ULT, bvult),
        item(OPCODE::BV_UGE, bvuge),
        item(OPCODE::BV_UGT, bvugt),
        item(OPCODE::BV_SLE, bvsle),
        item(OPCODE::BV_SLT, bvslt),
        item(OPCODE::BV_SGE, bvsge),
        item(OPCODE::BV_SGT, bvsgt),
        item(OPCODE::FP_ADD, fp.add),
        item(OPCODE::FP_SUB, fp.sub),
        item(OPCODE::FP_MUL, fp.mul),
        item(OPCODE::FP_DIV, fp.div),
        item(OPCODE::FP_REM, fp.rem),
        item(OPCODE::FP_EQ, fp.eq),
        item(OPCODE::FP_LEQ, fp.leq),
        item(OPCODE::FP_LT, fp.lt),
        item(OPCODE::FP_GEQ, fp.geq),
        item(OPCODE::FP_GT, fp.gt),
        item(OPCODE::BV_ZEXT, zero_extend),
        item(OPCODE::BV_SEXT, sign_extend),
        item(OPCODE::BV_EXTRACT, extract),
        item(OPCODE::BV_CONCAT, concat),
        item(OPCODE::FP_TOFP, to_fp),
        item(OPCODE::FP_TOSBV, fp.to_sbv),
        item(OPCODE::FP_TOUBV, fp.to_ubv),
        item(OPCODE::FP_TOFPU, to_fp_unsigned),
        item(OPCODE::FP_TOIEEE, to_ieee_bv),
        item(OPCODE::DASH, _),
        item(OPCODE::MARK, !),
        item(OPCODE::ITE, ite),
        item(OPCODE::SELECT, select),
        item(OPCODE::STORE, store)
    },
    vars(_vars) { }

void Parser::nextToken() {
    tokens.pop_front();
    if (tokens.empty()) {
        lookahead = {TokenType::EMPTY, ""};
    } else {
        lookahead = tokens.front();
    }
}

Expr Parser::expression() {
    if (lookahead.type == TokenType::OpenBracket) {
        nextToken();
        return term();
    } else if (lookahead.type == TokenType::IntValue) {
        IntValue value(std::stoll(lookahead.seq));
        nextToken();
        return value;
    } else if (lookahead.type == TokenType::BoolValue) {
        BoolValue value(lookahead.seq == "true");
        nextToken();
        return value;
    } else if (lookahead.type == TokenType::BvValue) {
        if (lookahead.seq[1] == 'b') {
            auto str = lookahead.seq;
            str.replace(0, 2, "");
            unsigned long long value = std::stoull(str, nullptr, 2);
            size_t size = str.size();
            BitValue bv(value, size);
            nextToken();
            return bv;
        } else if (lookahead.seq[1] == 'x') {
            auto str = lookahead.seq;
            str.replace(0, 2, "");
            unsigned long long value = std::stoull(str, nullptr, 16);
            size_t size = str.size() * 4;
            BitValue bv(value, size);
            nextToken();
            return bv;
        } else {
            throw ParseException("Unknown BitValue: " + lookahead.seq);
        }
    } else if (lookahead.type == TokenType::Var) {
        if (vars.count(lookahead.seq) == 0) {
            throw ParseException("Unknown var: " + lookahead.seq);
        }
        Var var(lookahead.seq, vars[lookahead.seq]);
        nextToken();
        return var;
    } else {
        throw ParseException("Unhandled token type");
    }
}

Expr Parser::term() {
    if(lookahead.type == TokenType::Bop) {
        auto op = lookahead;
        nextToken();
        Expr left = expression();
        Expr right = expression();
        if (lookahead.type != TokenType::CloseBracket) {
            throw ParseException("Unclosed brackets at: (" + lookahead.seq + " " + left.str() + " " + right.str());
        }
        nextToken();
        return createBop(op, left, right);
    } else if (lookahead.type == TokenType::Uop) {
        auto op = lookahead;
        nextToken();
        Expr expr = expression();
        if (lookahead.type != TokenType::CloseBracket) {
            throw ParseException("Unclosed brackets at: (" + lookahead.seq + " " + expr.str());
        }
        nextToken();
        return createUop(op, expr);
    } else {
        throw ParseException("Unexpected token: " + lookahead.seq);
    }
}

Expr Parser::createBop(const Token& token, const Expr& left, const Expr& right) const {
    auto op = opMap.at(token.seq);
#define OperatorCase(cond, oper) \
    case cond: \
        return left oper right

#define FuncCase(cond, func) \
    case cond: \
        return func(left, right)
    switch (op) {
        OperatorCase(OPCODE::EQU, ==);
        OperatorCase(OPCODE::ADD, +);
        OperatorCase(OPCODE::MINUS, -);
        OperatorCase(OPCODE::MULTIPLY, *);
        OperatorCase(OPCODE::DEVIDE, /);
        OperatorCase(OPCODE::AND, &&);
        OperatorCase(OPCODE::OR, ||);
        OperatorCase(OPCODE::GT, >);
        OperatorCase(OPCODE::GE, >=);
        OperatorCase(OPCODE::LT, <);
        OperatorCase(OPCODE::LE, <=);
        FuncCase(OPCODE::BV_ADD, bvadd);
        FuncCase(OPCODE::BV_SUB, bvsub);
        FuncCase(OPCODE::BV_MUL, bvmul);
        FuncCase(OPCODE::BV_UDIV, bvudiv);
        FuncCase(OPCODE::BV_SDIV, bvsdiv);
        FuncCase(OPCODE::BV_UREM, bvurem);
        FuncCase(OPCODE::BV_SREM, bvsrem);
        FuncCase(OPCODE::BV_SMOD, bvsmod);
        FuncCase(OPCODE::BV_SHL, bvshl);
        FuncCase(OPCODE::BV_LSHR, bvlshr);
        FuncCase(OPCODE::BV_ASHR, bvashr);
        FuncCase(OPCODE::BV_OR, bvor);
        FuncCase(OPCODE::BV_AND, bvand);
        FuncCase(OPCODE::BV_NAND, bvnand);
        FuncCase(OPCODE::BV_NOR, bvnor);
        FuncCase(OPCODE::BV_XOR, bvxor);
        FuncCase(OPCODE::BV_ULE, bvule);
        FuncCase(OPCODE::BV_ULT, bvult);
        FuncCase(OPCODE::BV_UGE, bvuge);
        FuncCase(OPCODE::BV_UGT, bvugt);
        FuncCase(OPCODE::BV_SLE, bvsle);
        FuncCase(OPCODE::BV_SLT, bvslt);
        FuncCase(OPCODE::BV_SGE, bvsge);
        FuncCase(OPCODE::BV_SGT, bvsgt);
        FuncCase(OPCODE::BV_CONCAT, concat);
        default:
            throw ParseException("Unhandled Bop: " + token.seq);
    }
}

Expr Parser::createUop(const Token& token, const Expr& expr) const {
    auto op = opMap.at(token.seq);
    switch(op) {
        case OPCODE::NOT:
            return !expr;
        case OPCODE::BV_NEG:
            return bvneg(expr);
        case OPCODE::BV_NOT:
            return bvnot(expr);
        default:
            throw ParseException("Unhandled Uop: " + token.seq);
    }
}

Expr Parser::parse(std::list<Token> tokens) {
    this->tokens = tokens;
    this->lookahead = this->tokens.front();
    Expr result = expression();
    if (lookahead.type!= TokenType::EMPTY) {
        std::ostringstream str;
        throw ParseException("Unexpected symbol found: " + lookahead.seq);
    }
    return result;
}

Expr Parser::parse(std::string str) {
    return parse(tokenizer.tokenize(str));
}

std::map<std::string, Sort> Sort::varSortMap = {};

Var::Var(std::string name, Sort sort) {
    if (name == "conv") {
        "debugger";
    }
    auto varNode = new VarNode(name, std::move(sort));
    _root.reset(new inode<Node>(varNode));
    Sort::varSortMap[name] = Sort(varNode->getSort());
}

}

}
