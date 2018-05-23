/*
 * BasicTypes.h
 *
 *  Created on: Jan 9, 2017
 *      Author: jiaxiang
 */

#ifndef SRC_TERMINATION_BASICTYPES_H_
#define SRC_TERMINATION_BASICTYPES_H_

#include <llvm/IR/BasicBlock.h>
#include "SMT/Middleware/AST.h"
#include <unordered_set>
#include <cstdlib>

namespace ceagle {

enum class Type {
    i1 = 0,
    i8,
    i16,
    i32
};

struct MemCell {
    smt::Expr ad;
    Type ty;
    smt::Expr val;
};

typedef std::pair<smt::Expr, smt::Expr> AllocationCell;

}

namespace std {

template<> struct hash<ceagle::MemCell> {
    typedef ceagle::MemCell argument_type;
    typedef std::size_t result_type;
    result_type operator() (argument_type const& s) const {
        result_type const h1 (std::hash<ceagle::smt::Expr>{}(s.ad));
        result_type const h2 (std::hash<ceagle::smt::Expr>{}(s.val));
        return h1 ^ (h2 << 1);
    }
};

template<> struct equal_to<ceagle::MemCell> {
    typedef ceagle::MemCell T;
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs.ty == rhs.ty && lhs.ad.str() == rhs.ad.str() && lhs.val.str() == rhs.val.str();
    }
};

template<> struct hash<ceagle::AllocationCell> {
    typedef ceagle::AllocationCell argument_type;
    typedef std::size_t result_type;
    result_type operator() (argument_type const& s) const {
        result_type const h1 (std::hash<ceagle::smt::Expr>{}(s.first));
        result_type const h2 (std::hash<ceagle::smt::Expr>{}(s.second));
        return h1 ^ (h2 << 1);
    }
};

template<> struct equal_to<ceagle::AllocationCell> {
    typedef ceagle::AllocationCell T;
    bool operator()( const T& lhs, const T& rhs ) const {
        return std::equal_to<ceagle::smt::Expr>{}(lhs.first, rhs.first)
                && std::equal_to<ceagle::smt::Expr>{}(lhs.second, rhs.second);
    }
};

}

namespace ceagle {

//typedef std::pair<llvm::BasicBlock*, llvm::BasicBlock::iterator> ProgramCounter;

struct ProgramCounter {
    llvm::BasicBlock* b;
    llvm::BasicBlock::iterator i;
};

typedef std::map<llvm::Value*, smt::Expr> LocalVars;
typedef std::unordered_set<AllocationCell> AllocationList;
typedef std::unordered_set<smt::Expr> KnowledgeBase;
typedef std::unordered_set<MemCell> PointerTable;
typedef std::unordered_set<smt::Expr> ExprSet;
typedef std::set<llvm::Value*> Domain;
typedef std::map<std::string, smt::Expr> Substitution;
typedef std::list<smt::Expr> ExprList;



enum class EdgeType {
    evaluation = 0,
    refinement,
    generalization
};

}

namespace std {
template<> struct equal_to<ceagle::ProgramCounter> {
    typedef ceagle::ProgramCounter T;
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs.b == rhs.b && lhs.i == rhs.i;
    }
};
}

#endif /* SRC_TERMINATION_BASICTYPES_H_ */
