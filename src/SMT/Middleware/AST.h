#ifndef CEAGLE_SMT_AST_H
#define CEAGLE_SMT_AST_H

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)


#include <vector>
#include <regex>
#include <exception>
#include <functional>
#include <list>
#include <string>
#include <memory>
#include <ostream>
#include <cstddef>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <bitset>
#include <set>
#include <map>
#include "SMT/Middleware/Sort.h"

namespace ceagle {

namespace smt {

class Node;
class Sort;

class Sort {
    friend class ArraySort;
protected:
    std::unique_ptr<SortImpl> impl;
    Sort(SortImpl* i) : impl(i) {
    }
protected:
    virtual void print(std::ostream& out) const {
        impl->print(out);
    }
public:
    Sort() : impl(new SortImpl()) {
    }
    Sort(const Sort& other) {
        impl.reset(other.impl->clone());
    }
    Sort(Sort&& other) {
        impl = std::move(other.impl);
    }
    friend std::ostream& operator<<(std::ostream & out, const Sort& sort) {
        sort.print(out);
        return out;
    }
    operator std::string() const {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    std::string str() const {
        return *this;
    }
    Sort& operator=(Sort&& other) {
        impl = std::move(other.impl);
        return *this;
    }
    SortType type() const { return impl->type(); }
    Sort getElementSort() const {
        if (this->type() != SortType::Array) {
            throw "getElementSort on none array sort";
        }
        return Sort(impl->getElementSort()->clone());
    }
    virtual ~Sort() = default;
    static std::map<std::string, Sort> varSortMap;
};

class BoolSort : public Sort {
public:
    BoolSort() {
        impl.reset(new BoolSortImpl());
    }
};

class NumberSort : public Sort {
protected:
    NumberSort() { }
};

class IntSort : public NumberSort {
public:
    IntSort() {
        impl.reset(new IntSortImpl());
    }
};

class BvSort : public NumberSort {
public:
    BvSort(std::size_t size) {
        impl.reset(new BvSortImpl(size));
    }
    std::size_t getSize() const {
        return static_cast<const BvSortImpl*>(impl.get())->getSize();
    }
};

class ArraySort : public Sort {
public:
    ArraySort(Sort elementSort, Sort indexSort = IntSort()) {
        impl.reset(new ArraySortImpl(elementSort.impl.release(), indexSort.impl.release()));
    }
};

template<class NodeTy>
class inode {
    std::unique_ptr<inode<NodeTy>> next;
    std::unique_ptr<inode<NodeTy>> child;
    inode<NodeTy>* lastChild;
    std::unique_ptr<NodeTy> value;
    void setLastChild() {
        lastChild = nullptr;
        if(child) {
            lastChild = child.get();
            while(lastChild->next) {
                lastChild = lastChild->next.get();
            }
        }
    }
public:
    inode(NodeTy* v) : next(), child(), lastChild(nullptr), value(v) { }
    inode(const inode<NodeTy>& other) {
        if (other.next) {
            next.reset(new inode<NodeTy>(*other.next));
        }
        if (other.child) {
            child.reset(new inode<NodeTy>(*other.child));
            setLastChild();
        }
        if (other.value) {
            value.reset(other.value->clone());
        }
    }
    inode& replaceSubTree(inode other) {
        this->child = std::move(other.child);
        this->value= std::move(other.value);
        this->lastChild = other.lastChild;
        return *this;
    }
    void setChild(inode<NodeTy> * item) {
        this->child.reset(item);
        setLastChild();
    }
    void setNext(inode<NodeTy> * next) {
        this->next.reset(next);
    }
    bool isLeaf() const {
        return child.get() == nullptr;
    }
    NodeTy* getValue() const {
        return value.get();
    }
    inode<NodeTy>* getChild() const {
        return child.get();
    }
    inode<NodeTy>* getNext() const {
        return next.get();
    }
    void pushChild(inode<NodeTy> * item) {
        lastChild->setNext(item);
        lastChild = lastChild->next.get();
    }
    const Sort& getSort() const {
        return value->getSort();
    }
    void setSort(Sort s) {
        value->setSort(std::move(s));
    }
    friend std::ostream& operator<<(std::ostream& out, const inode<Node>& node);
    virtual ~inode() = default;
};

enum class OPCODE {
    EQU = 1,
    ADD,
    MINUS,
    MULTIPLY,
    DEVIDE,
    REM,
    TO_INT,
    LT,
    GT,
    LE,
    GE,
    // propositional
    AND,
    OR,
    NOT,
    // bitvector
    BV_ADD,
    BV_SUB,
    BV_NEG,
    BV_MUL,
    BV_UREM,
    BV_SREM,
    BV_SDIV,
    BV_UDIV,
    BV_SMOD,
    BV_SHL,
    BV_LSHR,
    BV_ASHR,
    BV_OR,
    BV_AND,
    BV_NOT,
    BV_NAND,
    BV_NOR,
    BV_XOR,
    BV_ULE,
    BV_ULT,
    BV_UGE,
    BV_UGT,
    BV_SLE,
    BV_SLT,
    BV_SGE,
    BV_SGT,
    // float point
    FP_ADD,
    FP_SUB,
    FP_MUL,
    FP_DIV,
    FP_REM,
    FP_EQ,
    FP_LEQ,
    FP_LT,
    FP_GEQ,
    FP_GT,
    // convertion
    BV_ZEXT,
    BV_SEXT,
    BV_EXTRACT,
    BV_CONCAT,
    FP_TOFP,
    FP_TOSBV,
    FP_TOUBV,
    FP_TOFPU,
    FP_TOIEEE,
    // dash _
    DASH,
    // mark !
    MARK,
    // ite
    ITE,
    // array
    SELECT,
    STORE
};

inline std::ostream& operator<<(std::ostream & out, const OPCODE& code) {
#define OPCODE_OUTPUT(op, output) \
    case op: \
             out << #output ; \
    break;
    switch(code) {
        OPCODE_OUTPUT(OPCODE::EQU, =)
        OPCODE_OUTPUT(OPCODE::ADD, +)
        OPCODE_OUTPUT(OPCODE::MINUS, -)
        OPCODE_OUTPUT(OPCODE::MULTIPLY, *)
        OPCODE_OUTPUT(OPCODE::DEVIDE, /)
        OPCODE_OUTPUT(OPCODE::REM, rem)
        OPCODE_OUTPUT(OPCODE::TO_INT, to_int)
        OPCODE_OUTPUT(OPCODE::LT, <)
        OPCODE_OUTPUT(OPCODE::GT, >)
        OPCODE_OUTPUT(OPCODE::LE, <=)
        OPCODE_OUTPUT(OPCODE::GE, >=)
        OPCODE_OUTPUT(OPCODE::AND, and)
        OPCODE_OUTPUT(OPCODE::OR, or)
        OPCODE_OUTPUT(OPCODE::NOT, not)
        OPCODE_OUTPUT(OPCODE::BV_ADD, bvadd)
        OPCODE_OUTPUT(OPCODE::BV_SUB, bvsub)
        OPCODE_OUTPUT(OPCODE::BV_NEG, bvneg)
        OPCODE_OUTPUT(OPCODE::BV_MUL, bvmul)
        OPCODE_OUTPUT(OPCODE::BV_UDIV, bvudiv)
        OPCODE_OUTPUT(OPCODE::BV_SDIV, bvsdiv)
        OPCODE_OUTPUT(OPCODE::BV_UREM, bvurem)
        OPCODE_OUTPUT(OPCODE::BV_SREM, bvsrem)
        OPCODE_OUTPUT(OPCODE::BV_SMOD, bvsmod)
        OPCODE_OUTPUT(OPCODE::BV_SHL, bvshl)
        OPCODE_OUTPUT(OPCODE::BV_LSHR, bvlshr)
        OPCODE_OUTPUT(OPCODE::BV_ASHR, bvashr)
        OPCODE_OUTPUT(OPCODE::BV_OR, bvor)
        OPCODE_OUTPUT(OPCODE::BV_AND, bvand)
        OPCODE_OUTPUT(OPCODE::BV_NOT, bvnot)
        OPCODE_OUTPUT(OPCODE::BV_NAND, bvnand)
        OPCODE_OUTPUT(OPCODE::BV_NOR, bvnor)
        OPCODE_OUTPUT(OPCODE::BV_XOR, bvxor)
        OPCODE_OUTPUT(OPCODE::BV_ULE, bvule)
        OPCODE_OUTPUT(OPCODE::BV_ULT, bvult)
        OPCODE_OUTPUT(OPCODE::BV_UGE, bvuge)
        OPCODE_OUTPUT(OPCODE::BV_UGT, bvugt)
        OPCODE_OUTPUT(OPCODE::BV_SLE, bvsle)
        OPCODE_OUTPUT(OPCODE::BV_SLT, bvslt)
        OPCODE_OUTPUT(OPCODE::BV_SGE, bvsge)
        OPCODE_OUTPUT(OPCODE::BV_SGT, bvsgt)
        OPCODE_OUTPUT(OPCODE::FP_ADD, fp.add)
        OPCODE_OUTPUT(OPCODE::FP_SUB, fp.sub)
        OPCODE_OUTPUT(OPCODE::FP_MUL, fp.mul)
        OPCODE_OUTPUT(OPCODE::FP_DIV, fp.div)
        OPCODE_OUTPUT(OPCODE::FP_REM, fp.rem)
        OPCODE_OUTPUT(OPCODE::FP_EQ, fp.eq)
        OPCODE_OUTPUT(OPCODE::FP_LEQ, fp.leq)
        OPCODE_OUTPUT(OPCODE::FP_LT, fp.lt)
        OPCODE_OUTPUT(OPCODE::FP_GEQ, fp.geq)
        OPCODE_OUTPUT(OPCODE::FP_GT, fp.gt)
        OPCODE_OUTPUT(OPCODE::BV_ZEXT, zero_extend)
        OPCODE_OUTPUT(OPCODE::BV_SEXT, sign_extend)
        OPCODE_OUTPUT(OPCODE::BV_EXTRACT, extract)
        OPCODE_OUTPUT(OPCODE::BV_CONCAT, concat)
        OPCODE_OUTPUT(OPCODE::FP_TOFP, to_fp)
        OPCODE_OUTPUT(OPCODE::FP_TOSBV, fp.to_sbv)
        OPCODE_OUTPUT(OPCODE::FP_TOUBV, fp.to_ubv)
        OPCODE_OUTPUT(OPCODE::FP_TOFPU, to_fp_unsigned)
        OPCODE_OUTPUT(OPCODE::FP_TOIEEE, to_ieee_bv)
        OPCODE_OUTPUT(OPCODE::DASH, _)
        OPCODE_OUTPUT(OPCODE::MARK, !)
        OPCODE_OUTPUT(OPCODE::ITE, ite)
        OPCODE_OUTPUT(OPCODE::SELECT, select)
        OPCODE_OUTPUT(OPCODE::STORE, store)
    }
    return out;
}

enum class RoundingMode {
    RNE,
    RNA,
    RTP,
    RTN,
    RTZ
};

inline std::ostream& operator<<(std::ostream & out, const RoundingMode& mode) {
#define OUTPUT_CASE(mode) \
    case RoundingMode::mode: \
               out << #mode ; \
    break;
    switch(mode) {
        OUTPUT_CASE(RNE)
        OUTPUT_CASE(RNA)
        OUTPUT_CASE(RTP)
        OUTPUT_CASE(RTN)
        OUTPUT_CASE(RTZ)
    }
    return out;
}

class FloatSort : public NumberSort {
public:
    FloatSort() : FloatSort(8, 24) {
    }
    FloatSort(std::size_t e, std::size_t s) {
        impl.reset(new FloatSortImpl(e, s));
    }
    std::size_t getEb() const {
        return static_cast<FloatSortImpl*>(impl.get())->getEb();
    }
    std::size_t getSb() const {
        return static_cast<FloatSortImpl*>(impl.get())->getSb();
    }
};

class RealSort : public NumberSort {
public:
    RealSort() {
        impl.reset(new RealSortImpl());
    }
};

class Node {
protected:
    Sort sort;
    virtual void print(std::ostream& out) const = 0;
    Node(Sort s) : sort(std::move(s)) {
    }
    Node() {
    }
public:
    virtual bool isLeaf() const = 0;
    virtual Node* clone() const = 0;
    virtual std::set<std::string> vars() const {
        return {};
    }
    friend std::ostream& operator<<(std::ostream& out, const Node& node) {
        node.print(out);
        return out;
    }
    const Sort& getSort() const {
        return sort;
    }
    void setSort(Sort s) {
        sort = std::move(s);
    }
    virtual ~Node() = default;
};

class VarNode : public Node {
    std::string _name;
protected:
    virtual void print(std::ostream& out) const override {
        out << name();
    }
public:
    VarNode(std::string n, Sort s) : Node(std::move(s)), _name(n) {
    }
    virtual bool isLeaf() const override {
        return true;
    }
    virtual Node* clone() const override {
        return new VarNode(name(), sort);
    }
    std::string name() const {
        return _name;
    }
    virtual std::set<std::string> vars() const override {
        return {_name};
    }
};

template<typename T>
class ValueNode : public Node {
protected:
    T value;
    virtual void print(std::ostream& out) const {
        out << value;
    }
public:
    ValueNode(T v, Sort s) : Node(std::move(s)), value(v) {
    }
    ValueNode(T v) : value(v) {
    }
    virtual bool isLeaf() const {
        return true;
    }
    virtual Node* clone() const {
        return new ValueNode<T>(this->value, this->sort);
    }
    T getValue() const {
        return value;
    }
};

template<std::size_t N>
class BitValueNode : public ValueNode<std::bitset<N>> {
protected:
    virtual void print(std::ostream& out) const {
        out << "#b" << this->value;
    }
    BitValueNode(std::bitset<N> value) : ValueNode<std::bitset<N>>(value, BvSort(N)) {
    }
public:
    BitValueNode(unsigned long long val) : BitValueNode(std::bitset<N>(val)) {
    }
    virtual Node* clone() const {
        return new BitValueNode<N>(this->value);
    }

};

class Float32ValueNode : public ValueNode<float> {
protected:
    virtual void print(std::ostream& out) const {
        auto value = this->value;
        auto i = std::bitset<32>(*reinterpret_cast<int*>(&value)).to_string();
        out << "(fp #b" << i[0] << " #b" << i.substr(1, 8) << " #b" << i.substr(9, 23) << ")";
    }
public:
    Float32ValueNode(float value) : ValueNode<float>(value, FloatSort()) {
    }
    virtual Node* clone() const {
        return new Float32ValueNode(this->value);
    }
};

class Float64ValueNode : public ValueNode<double> {
protected:
    virtual void print(std::ostream& out) const {
        auto value = this->value;
        auto i = std::bitset<64>(*reinterpret_cast<long long*>(&value)).to_string();
        out << "(fp #b" << i[0] << " #b" << i.substr(1, 11) << " #b" << i.substr(12, 52) << ")";
    }
public:
    Float64ValueNode(double value) : ValueNode<double>(value, FloatSort(11, 53)) {
    }
    virtual Node* clone() const {
        return new Float64ValueNode(this->value);
    }
};

class RoundingModeNode : public ValueNode<RoundingMode> {
public:
    RoundingModeNode(RoundingMode mode) : ValueNode<RoundingMode>(mode) {
    }
    virtual Node* clone() const {
        return new RoundingModeNode(this->value);
    }
};

class OperatorNode : public Node {
protected:
    OPCODE opcode;
    virtual void print(std::ostream& out) const override {
        out << opcode;
    }
    OperatorNode(const OperatorNode& other) : Node(other.sort), opcode(other.opcode) {
    }
public:
    OperatorNode(OPCODE op) : opcode(op) { }
    virtual bool isLeaf() const override {
        return false;
    }
    virtual Node* clone() const override {
        return new OperatorNode(*this);
    }
    OPCODE getOpcode() const {
        return opcode;
    }
};

class ArgumentOperatorNode : public OperatorNode {
public:
    ArgumentOperatorNode(OPCODE op) : OperatorNode(op) { }
    virtual bool isLeaf() const override {
        return true;
    }
    virtual Node* clone() const override {
        return new ArgumentOperatorNode(this->opcode);
    }
};

class HighOrderOperatorNode : public Node {
protected:
    std::unique_ptr<inode<Node>> _root;
    virtual void print(std::ostream& out) const override {
        if (_root) {
            out << *_root;
        }
    }
public:
    HighOrderOperatorNode(std::unique_ptr<inode<Node>> root) : _root(std::move(root)) {
    }
    HighOrderOperatorNode(inode<Node>* root) {
        _root.reset(root);
    }
    virtual std::set<std::string> vars() const override;
    virtual bool isLeaf() const override {
        return false;
    }
    virtual Node* clone() const override {
        return new HighOrderOperatorNode(std::unique_ptr<inode<Node>>(new inode<Node>(*_root)));
    }
    OPCODE getOpcode() const {
        auto value = _root->getChild()->getValue();
        if (auto op = dynamic_cast<OperatorNode*>(value)) {
            return op->getOpcode();
        } else {
            assert(false);
        }
    }
    inode<Node>* getRoot() const {
        return _root.get();
    }
};

class Expr {
    friend class Solver;
    friend class RawSolver;
protected:
    std::unique_ptr<inode<Node>> _root;
public:
    virtual ~Expr() = default;
    Expr(const Expr& expr) : _root(new inode<Node>(*expr._root)) {
        this->_root->setSort(expr.sort());
    }
    Expr() : _root() { }
    Expr(inode<Node> root) : _root(new inode<Node>(root)) {
        this->_root->setSort(root.getSort());
    }
    Expr(Expr&& expr) : _root(std::move(expr._root)) { }
    Expr& operator=(Expr&& right) {
        _root = std::move(right._root);
        return *this;
    }
    Expr& operator=(const Expr& right) {
        _root.reset(new inode<Node>(*right._root));
        this->_root->setSort(right.sort());
        return *this;
    }
    std::set<std::string> vars() const;
    Expr replace(std::map<std::string, Expr> varMap) const;
    const Sort& sort() const { return _root->getSort(); }
    Expr param(std::string name, std::string value) & {
        auto new_root = new inode<Node>(new OperatorNode(OPCODE::MARK));
        new_root->setSort(this->_root->getSort());
        new_root->setChild(this->_root.release());
        this->_root.reset(new_root);
        this->_root->pushChild(new inode<Node>(new ValueNode<std::string>(name)));
        this->_root->pushChild(new inode<Node>(new ValueNode<std::string>(value)));
        return *this;
    }
    Expr param(std::string name, std::string value) && {
        auto new_root = new inode<Node>(new OperatorNode(OPCODE::MARK));
        new_root->setSort(this->_root->getSort());
        new_root->setChild(this->_root.release());
        this->_root.reset(new_root);
        this->_root->pushChild(new inode<Node>(new ValueNode<std::string>(name)));
        this->_root->pushChild(new inode<Node>(new ValueNode<std::string>(value)));
        return std::move(*this);
    }
    template<OPCODE opcode>
    static Expr float_func(Expr& left, Expr& right, RoundingMode mode=RoundingMode::RNE) {
        Expr expr;
        expr._root.reset(new inode<Node>(new OperatorNode(opcode)));
        expr._root->setSort(left._root->getSort());
        auto child = new inode<Node>(new RoundingModeNode(mode));
        left._root->setNext(right._root.release());
        child->setNext(left._root.release());
        expr._root->setChild(child);
        return expr;
    }
#define UNARY_OP(op, opcode) \
    friend Expr operator op(Expr operand) { \
        Expr expr; \
        expr._root.reset(new inode<Node>(new OperatorNode(opcode))); \
        expr._root->setSort(BoolSort()); \
        expr._root->setChild(operand._root.release()); \
        return expr; \
    }
#define UNARY_FUNC(name, opcode) \
    friend Expr name(Expr operand) { \
        Expr expr; \
        expr._root.reset(new inode<Node>(new OperatorNode(opcode))); \
        switch (opcode) { \
            default: \
                expr._root->setSort(operand._root->getSort()); \
                break; \
            case OPCODE::TO_INT:\
                expr._root->setSort(IntSort()); \
                break; \
        } \
        expr._root->setChild(operand._root.release()); \
        return expr; \
    }
#define BINARY_OP(op, opcode) \
    friend Expr operator op(Expr left, Expr right) { \
        Expr expr; \
        expr._root.reset(new inode<Node>(new OperatorNode(opcode))); \
        switch(opcode) { \
            case OPCODE::EQU:case OPCODE::AND:case OPCODE::OR:case OPCODE::LT:case OPCODE::GT:case OPCODE::LE:case OPCODE::GE: \
                expr._root->setSort(BoolSort()); \
                break; \
            case OPCODE::ADD:case OPCODE::MINUS:case OPCODE::MULTIPLY:case OPCODE::DEVIDE: \
                expr._root->setSort(left._root->getSort()); \
                break; \
        } \
        left._root->setNext(right._root.release()); \
        expr._root->setChild(left._root.release()); \
        return expr; \
    }
#define BINARY_FUNC(name, opcode) \
    friend Expr name(Expr left, Expr right) { \
        Expr expr; \
        expr._root.reset(new inode<Node>(new OperatorNode(opcode))); \
        switch(opcode) { \
            case OPCODE::DEVIDE:\
                expr._root->setSort(RealSort()); \
                break; \
            case OPCODE::REM: \
            case OPCODE::BV_ADD:\
            case OPCODE::BV_SUB:\
            case OPCODE::BV_MUL:\
            case OPCODE::BV_UDIV:\
            case OPCODE::BV_SDIV:\
            case OPCODE::BV_UREM:\
            case OPCODE::BV_SREM:\
            case OPCODE::BV_SMOD:\
            case OPCODE::BV_OR:\
            case OPCODE::BV_AND:\
            case OPCODE::BV_NAND:\
            case OPCODE::BV_NOR:\
            case OPCODE::BV_XOR:\
            case OPCODE::BV_SHL:\
            case OPCODE::BV_LSHR:\
            case OPCODE::BV_ASHR:\
            case OPCODE::FP_REM:\
                expr._root->setSort(left._root->getSort()); \
                break; \
            case OPCODE::BV_ULE:\
            case OPCODE::BV_ULT:\
            case OPCODE::BV_UGE:\
            case OPCODE::BV_UGT:\
            case OPCODE::BV_SLE:\
            case OPCODE::BV_SLT:\
            case OPCODE::BV_SGE:\
            case OPCODE::BV_SGT:\
            case OPCODE::FP_EQ:\
            case OPCODE::FP_LEQ:\
            case OPCODE::FP_LT:\
            case OPCODE::FP_GEQ:\
            case OPCODE::FP_GT:\
                expr._root->setSort(BoolSort()); \
                break; \
            case OPCODE::SELECT:\
                expr._root->setSort(left._root->getSort().getElementSort()); \
                break; \
            defalt: \
                throw "unhandled sort when constructing expr"; \
                break; \
        } \
        left._root->setNext(right._root.release()); \
        expr._root->setChild(left._root.release()); \
        return expr; \
    }
#define FLOAT_FUNC(name, opcode) \
    friend Expr name(Expr left, Expr right, RoundingMode mode=RoundingMode::RNE) { \
        return Expr::float_func<opcode>(left, right, mode); \
    }
    BINARY_OP(==, OPCODE::EQU)
    BINARY_OP(>, OPCODE::GT)
    BINARY_OP(<, OPCODE::LT)
    BINARY_OP(>=, OPCODE::GE)
    BINARY_OP(<=, OPCODE::LE)
    BINARY_OP(+, OPCODE::ADD)
    BINARY_OP(-, OPCODE::MINUS)
    BINARY_OP(*, OPCODE::MULTIPLY)
    BINARY_OP(/, OPCODE::DEVIDE)
    BINARY_FUNC(rem, OPCODE::REM)
    BINARY_OP(&&, OPCODE::AND)
    BINARY_OP(||, OPCODE::OR)
    UNARY_OP(!, OPCODE::NOT)
    UNARY_FUNC(to_int, OPCODE::TO_INT)
    // bitvector
    BINARY_FUNC(bvadd, OPCODE::BV_ADD)
    BINARY_FUNC(bvsub, OPCODE::BV_SUB)
    UNARY_FUNC(bvneg, OPCODE::BV_NEG)
    BINARY_FUNC(bvmul, OPCODE::BV_MUL)
    BINARY_FUNC(bvudiv, OPCODE::BV_UDIV)
    BINARY_FUNC(bvsdiv, OPCODE::BV_SDIV)
    BINARY_FUNC(bvurem, OPCODE::BV_UREM)
    BINARY_FUNC(bvsrem, OPCODE::BV_SREM)
    BINARY_FUNC(bvsmod, OPCODE::BV_SMOD)
    BINARY_FUNC(bvshl, OPCODE::BV_SHL)
    BINARY_FUNC(bvlshr, OPCODE::BV_LSHR)
    BINARY_FUNC(bvashr, OPCODE::BV_ASHR)
    BINARY_FUNC(bvor, OPCODE::BV_OR)
    BINARY_FUNC(bvand, OPCODE::BV_AND)
    UNARY_FUNC(bvnot, OPCODE::BV_NOT)
    BINARY_FUNC(bvnand, OPCODE::BV_NAND)
    BINARY_FUNC(bvnor, OPCODE::BV_NOR)
    BINARY_FUNC(bvxor, OPCODE::BV_XOR)
    BINARY_FUNC(bvule, OPCODE::BV_ULE)
    BINARY_FUNC(bvult, OPCODE::BV_ULT)
    BINARY_FUNC(bvuge, OPCODE::BV_UGE)
    BINARY_FUNC(bvugt, OPCODE::BV_UGT)
    BINARY_FUNC(bvsle, OPCODE::BV_SLE)
    BINARY_FUNC(bvslt, OPCODE::BV_SLT)
    BINARY_FUNC(bvsge, OPCODE::BV_SGE)
    BINARY_FUNC(bvsgt, OPCODE::BV_SGT)
    // floating point
    FLOAT_FUNC(fpadd, OPCODE::FP_ADD)
    FLOAT_FUNC(fpsub, OPCODE::FP_SUB)
    FLOAT_FUNC(fpmul, OPCODE::FP_MUL)
    FLOAT_FUNC(fpdiv, OPCODE::FP_DIV)
    BINARY_FUNC(fprem, OPCODE::FP_REM)
    BINARY_FUNC(fpeq, OPCODE::FP_EQ)
    BINARY_FUNC(fpleq, OPCODE::FP_LEQ)
    BINARY_FUNC(fplt, OPCODE::FP_LT)
    BINARY_FUNC(fpgeq, OPCODE::FP_GEQ)
    BINARY_FUNC(fpgt, OPCODE::FP_GT)
    // array
    BINARY_FUNC(array_select, OPCODE::SELECT)
    friend Expr array_store(Expr arr, Expr index, Expr value) {
        Expr expr;
        expr._root.reset(new inode<Node>(new OperatorNode(OPCODE::STORE)));
        expr._root->setSort(arr._root->getSort());
        index._root->setNext(value._root.release());
        arr._root->setNext(index._root.release());
        expr._root->setChild(arr._root.release());
        return expr;
    }
    // type convertion
private:
    friend Expr ext(Expr expr, OPCODE op, size_t extend) {
        auto highRoot = new inode<Node>(new OperatorNode(OPCODE::DASH));
        auto child = new inode<Node>(new ArgumentOperatorNode(op));
        child->setNext(new inode<Node>(new ValueNode<size_t>(extend)));
        highRoot->setChild(child);
        Expr result;
        result._root.reset(new inode<Node>(new HighOrderOperatorNode(highRoot)));
        result._root->setChild(expr._root.release());
        return result;
    }
public:
    friend Expr zext(Expr expr, size_t from, size_t to) {
        assert("zext from must less than to" && (from < to));
        auto result = ext(std::move(expr), OPCODE::BV_ZEXT, to - from);
        result._root->setSort(BvSort(to));
        return result;
    }
    friend Expr sext(Expr expr, size_t from, size_t to) {
        assert("sext from must less than to" && (from < to));
        auto result = ext(std::move(expr), OPCODE::BV_SEXT, to - from);
        result._root->setSort(BvSort(to));
        return result;
    }
    friend Expr extract(Expr expr, size_t from, size_t to) {
        assert("extract from must greater than to" && (from > to));
        assert("extract to must greater than zero" && (to > 0));
        auto highRoot = new inode<Node>(new OperatorNode(OPCODE::DASH));
        auto child = new inode<Node>(new ArgumentOperatorNode(OPCODE::BV_EXTRACT));
        auto next = new inode<Node>(new ValueNode<size_t>(to - 1));
        next->setNext(new inode<Node>(new ValueNode<size_t>(0)));
        child->setNext(next);
        highRoot->setChild(child);
        Expr result;
        result._root.reset(new inode<Node>(new HighOrderOperatorNode(highRoot)));
        result._root->setChild(expr._root.release());
        result._root->setSort(BvSort(to));
        return result;
    }
    friend Expr concat(Expr left, Expr right) {
        auto sort1 = left.sort();
        auto sort2 = right.sort();
        assert(sort1.type() == SortType::Bv && sort2.type() == SortType::Bv);
        BvSort* bv1 = static_cast<BvSort*>(&sort1);
        BvSort* bv2 = static_cast<BvSort*>(&sort2);
        Expr expr;
        expr._root.reset(new inode<Node>(new OperatorNode(OPCODE::BV_CONCAT)));
        expr._root->setSort(BvSort(bv1->getSize() + bv2->getSize()));
        left._root->setNext(right._root.release());
        expr._root->setChild(left._root.release());
        return expr;
    }
private:
    friend Expr tofp(Expr expr, const Sort& sort, OPCODE op, RoundingMode rm=RoundingMode::RNE) {
        if (sort.type() == SortType::Float) {
            auto fpsort = static_cast<const FloatSort*>(&sort);
            auto eb = fpsort->getEb();
            auto sb = fpsort->getSb();
            auto highRoot = new inode<Node>(new OperatorNode(OPCODE::DASH));
            auto child = new inode<Node>(new ArgumentOperatorNode(op));
            auto next = new inode<Node>(new ValueNode<size_t>(eb));
            next->setNext(new inode<Node>(new ValueNode<size_t>(sb)));
            child->setNext(next);
            highRoot->setChild(child);
            Expr result;
            result._root.reset(new inode<Node>(new HighOrderOperatorNode(highRoot)));
            child = new inode<Node>(new RoundingModeNode(rm));
            child->setNext(expr._root.release());
            result._root->setChild(child);
            result._root->setSort(*fpsort);
            return result;
        } else {
            throw "sort must be float";
        }
    }
public:
    friend Expr bvcasttofp(Expr expr,const Sort& sort) {
        if (sort.type() == SortType::Float) {
            auto fpsort = static_cast<const FloatSort*>(&sort);
            auto eb = fpsort->getEb();
            auto sb = fpsort->getSb();
            auto highRoot = new inode<Node>(new OperatorNode(OPCODE::DASH));
            auto child = new inode<Node>(new ArgumentOperatorNode(OPCODE::FP_TOFP));
            auto next = new inode<Node>(new ValueNode<size_t>(eb));
            next->setNext(new inode<Node>(new ValueNode<size_t>(sb)));
            child->setNext(next);
            highRoot->setChild(child);
            Expr result;
            result._root.reset(new inode<Node>(new HighOrderOperatorNode(highRoot)));
            result._root->setChild(expr._root.release());
            result._root->setSort(*fpsort);
            return result;
        } else {
            throw "sort must be float";
        }
    }
    friend Expr fptofp(Expr expr, const Sort& sort, RoundingMode rm=RoundingMode::RNE) {
        return tofp(std::move(expr), sort, OPCODE::FP_TOFP, rm);
    }
private:
    friend Expr fptobv(Expr expr, size_t m, OPCODE op, RoundingMode rm=RoundingMode::RTZ) {
        auto highRoot = new inode<Node>(new OperatorNode(OPCODE::DASH));
        auto child = new inode<Node>(new ArgumentOperatorNode(op));
        child->setNext(new inode<Node>(new ValueNode<size_t>(m)));
        highRoot->setChild(child);
        Expr result;
        result._root.reset(new inode<Node>(new HighOrderOperatorNode(highRoot)));
        child = new inode<Node>(new RoundingModeNode(rm));
        child->setNext(expr._root.release());
        result._root->setChild(child);
        result._root->setSort(BvSort(m));
        return result;
    }
public:
    friend Expr fptoieee(Expr operand) {
        const auto& sort = operand.sort();
        if (sort.type() == SortType::Float) {
            auto fpsort = static_cast<const FloatSort*>(&sort);
            Expr expr;
            expr._root.reset(new inode<Node>(new OperatorNode(OPCODE::FP_TOIEEE)));
            expr._root->setSort(BvSort(fpsort->getEb() +fpsort->getSb()));
            expr._root->setChild(operand._root.release());
            return expr;
        } else {
            throw "sort must be float";
        }
    }
    friend Expr fptosbv(Expr expr, size_t m, RoundingMode rm=RoundingMode::RTZ) {
        return fptobv(std::move(expr), m, OPCODE::FP_TOSBV, rm);
    }
    friend Expr fptoubv(Expr expr, size_t m, RoundingMode rm=RoundingMode::RTZ) {
        return fptobv(std::move(expr), m, OPCODE::FP_TOUBV, rm);
    }
    friend Expr sbvtofp(Expr expr, const Sort& sort, RoundingMode rm=RoundingMode::RNE) {
        return fptofp(std::move(expr), sort, rm);
    }
    friend Expr ubvtofp(Expr expr, const Sort& sort, RoundingMode rm=RoundingMode::RNE) {
        return tofp(std::move(expr), sort, OPCODE::FP_TOFPU, rm);
    }
    friend Expr ite(Expr cond, Expr e1, Expr e2) {
        Expr expr;
        expr._root.reset(new inode<Node>(new OperatorNode(OPCODE::ITE)));
        expr._root->setSort(e1._root->getSort());
        e1._root->setNext(e2._root.release());
        cond._root->setNext(e1._root.release());
        expr._root->setChild(cond._root.release());
        return expr;
    }
    friend std::ostream& operator<<(std::ostream&, const Expr&);
    operator std::string() const {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    std::string str() const {
        return *this;
    }
    inode<Node>* getRoot() const {
        return _root.get();
    }
};

class Var : public Expr {
public:
    Var(std::string name, Sort sort);
};

class IntValue : public Expr {
public:
    IntValue(int value) {
        _root.reset(new inode<Node>(new ValueNode<int>(value, IntSort())));
    }
};

class BitValue : public Expr {
public:
    BitValue(unsigned long long value, const std::size_t N) {
#define CASE_N(i) \
        case i: \
                _root.reset(new inode<Node>(new BitValueNode<i>(value))); \
        break;
        switch(N) {
            CASE_N(1)
            CASE_N(2)
            CASE_N(4)
            CASE_N(8)
            CASE_N(16)
            CASE_N(24)
            CASE_N(32)
            CASE_N(64)
            CASE_N(128)
            default:
                throw "unsupported length";
        }
    }
};

class BoolValue : public Expr {
public:
    explicit BoolValue(bool value) {
        _root.reset(new inode<Node>(new ValueNode<bool>(value, BoolSort())));
    }
};

class FloatValue : public Expr {
public:
    explicit FloatValue(float value) {
        _root.reset(new inode<Node>(new Float32ValueNode(value)));
    }
    explicit FloatValue(double value) {
        _root.reset(new inode<Node>(new Float64ValueNode(value)));
    }
};

template<typename Ret>
class Evaluator {
public:
    virtual Ret evaluate(Expr expr) {
        return evaluate(expr.getRoot());
    }
    Ret evaluate(inode<Node>* root) {
        auto node = root->getValue();
        if (auto var = dynamic_cast<VarNode*>(node)) {
            return handleVar(var);
        } else if (auto value = dynamic_cast<ValueNode<int>*>(node)) {
            return handleIntValue(value);
        } else if (auto value = dynamic_cast<ValueNode<bool>*>(node)) {
            return handleBoolValue(value);
        } else if (auto value = dynamic_cast<ValueNode<unsigned long>*>(node)) {
            return handleUnsignedLongValue(value);
        } else if (auto value = dynamic_cast<BitValueNode<1>*>(node)) {
            return handleBit1Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<2>*>(node)) {
            return handleBit2Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<4>*>(node)) {
            return handleBit4Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<8>*>(node)) {
            return handleBit8Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<16>*>(node)) {
            return handleBit16Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<32>*>(node)) {
            return handleBit32Value(value);
        } else if (auto value = dynamic_cast<BitValueNode<64>*>(node)) {
            return handleBit64Value(value);
        } else if (auto op = dynamic_cast<OperatorNode*>(node)) {
            return handleOperator(op, root);
        } else if (auto op = dynamic_cast<HighOrderOperatorNode*>(node)) {
            return handleHighOrderOperator(op, root);
        } else {
            assert(false && __FILE__ && __func__ && LINE_STRING && "unhandled node type");
        }
    }
    virtual Ret handleHighOrderOperator(HighOrderOperatorNode* op, inode<Node>* root) {
        auto opcode = op->getOpcode();
        switch(opcode) {
            default:
                assert(false);
            case OPCODE::BV_SEXT:
                return handleBvSExt(evaluate(root->getChild()), evaluate(op->getRoot()->getChild()->getNext()));
            case OPCODE::BV_EXTRACT:
                return handleBvExtract(evaluate(root->getChild()), evaluate(op->getRoot()->getChild()->getNext()), evaluate(op->getRoot()->getChild()->getNext()->getNext()));
        }
    }
    virtual Ret handleOperator(OperatorNode* op, inode<Node>* root) {
        switch(op->getOpcode()) {
            default:
                assert(false && __FILE__ && __func__ && LINE_STRING && "unhandled opcode");
            case OPCODE::ADD:
                return handleAdd(evaluate(root->getChild()), evaluate(root->getChild()->getNext()));
            case OPCODE::BV_ADD:
                return handleBvAdd(evaluate(root->getChild()), evaluate(root->getChild()->getNext()));
            case OPCODE::BV_SUB:
                return handleBvSub(evaluate(root->getChild()), evaluate(root->getChild()->getNext()));
            case OPCODE::BV_SDIV:
                return handleBvSDiv(evaluate(root->getChild()), evaluate(root->getChild()->getNext()));
        }
    }
    virtual Ret handleAdd(const Ret& left, const Ret& right) {
        assert(false && "no implmented");
    }
    virtual Ret handleBvSExt(const Ret& left, const Ret& right) {
        assert(false && "no implmented");
    }
    virtual Ret handleBvExtract(const Ret& left, const Ret& to, const Ret& from) {
        assert(false && "no implmented");
    }
    virtual Ret handleBvAdd(const Ret& left, const Ret& right) {
        assert(false && "no implmented");
    }
    virtual Ret handleBvSub(const Ret& left, const Ret& right) {
        assert(false && "no implmented");
    }
    virtual Ret handleBvSDiv(const Ret& left, const Ret& right) {
        assert(false && "no implmented");
    }
    virtual Ret handleVar(VarNode* var) = 0;
    virtual Ret handleIntValue(ValueNode<int>* node) = 0;
    virtual Ret handleUnsignedLongValue(ValueNode<unsigned long>* node) {
        assert(false && "no implmented");
    }
    virtual Ret handleBoolValue(ValueNode<bool>* node) = 0;
    virtual Ret handleBit1Value(BitValueNode<1>* node) = 0;
    virtual Ret handleBit2Value(BitValueNode<2>* node) = 0;
    virtual Ret handleBit4Value(BitValueNode<4>* node) = 0;
    virtual Ret handleBit8Value(BitValueNode<8>* node) = 0;
    virtual Ret handleBit16Value(BitValueNode<16>* node) = 0;
    virtual Ret handleBit32Value(BitValueNode<32>* node) = 0;
    virtual Ret handleBit64Value(BitValueNode<64>* node) = 0;
};

// expr -> var | value | ( term
// term -> bop expr ) | uop expr expr )
// var -> [_a-zA-Z][_a-zA-Z0-9]*
// value -> boolValue | intValue | bvValue
// boolValue -> true | false
// intValue -> [1-9][0-9]*
// bvValue -> 0b[0-1]+

enum class TokenType {
    Var = 1,
    BoolValue,
    IntValue,
    BvValue,
    Uop,
    Bop,
    OpenBracket,
    CloseBracket,
    EMPTY
};

class ParseException : public std::exception {
    std::string _str;
public:
    ParseException(std::string str) : _str(str) { }
    virtual const char* what() const noexcept override {
        return ("Parse Error " + _str).c_str();
    }
};

std::ostream& operator<<(std::ostream& out, TokenType type);

struct Token {
    TokenType type;
    std::string seq;
};

class Tokenizer {
    std::vector<std::pair<TokenType, std::regex>> patterns;
public:
    Tokenizer();
    std::list<Token> tokenize(std::string str);
};

class Parser {
    Tokenizer tokenizer;
    std::map<std::string, OPCODE> opMap;
    std::map<std::string, Sort> vars;
    std::list<Token> tokens;
    Token lookahead;
    void nextToken();
    Expr expression();
    Expr term();
    Expr createBop(const Token& token, const Expr& left, const Expr& right) const;
    Expr createUop(const Token& token, const Expr& expr) const;
public:
    Parser(std::map<std::string, Sort> _vars);
    Expr parse(std::string str);
    Expr parse(std::list<Token> tokens);
};

} // end namespace std

} // end namespace ceagle

namespace std {
    template<> struct hash<ceagle::smt::Expr> {
        typedef ceagle::smt::Expr argument_type;
        typedef std::size_t result_type;
        result_type operator() (argument_type const& s) const {
            return std::hash<std::string>{}(s.str());
        }
    };
    template<> struct equal_to<ceagle::smt::Expr> {
        typedef ceagle::smt::Expr T;
        bool operator()( const T& lhs, const T& rhs ) const {
            return lhs.str() == rhs.str() && lhs.sort().str() == rhs.sort().str();
        }
    };
};



#endif
