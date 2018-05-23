/*
 * utils.h
 *
 *  Created on: Jan 18, 2017
 *      Author: jiaxiang
 */

#ifndef SRC_TERMINATION_UTILS_H_
#define SRC_TERMINATION_UTILS_H_

#include "BasicTypes.h"
#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Solver.h"

namespace ceagle {

class Utils {

public:
    static std::string toString(Type type);
    static std::string toString(EdgeType type);
    static std::string toMidFixString(smt::Expr expr);

    static PointerTable intersection(const PointerTable& a, const PointerTable& b);
    static AllocationList intersection(const AllocationList& a, const AllocationList& b);

    static bool implies(const ExprSet& predicates, const smt::Expr& argument, smt::Solver* solver);
    static bool implies(const ExprSet& predicates, const ExprSet& arguments, smt::Solver* solver);

    static smt::Expr substitute(const Substitution& mu, const smt::Expr& e);
    static ExprSet substitute(const Substitution& mu, const ExprSet& es);
    static ExprList substitute(const Substitution& mu, const ExprList& el);
    static AllocationCell substitute(const Substitution& mu, const AllocationCell& ac);
    static AllocationList substitute(const Substitution& mu, const AllocationList& al);
    static MemCell substitute(const Substitution& mu, const MemCell& mc);
    static PointerTable substitute(const Substitution& mu, const PointerTable& pt);

    static bool isEq(const smt::Expr& e);
    static bool isNEq(const smt::Expr& e);
    static std::pair<smt::Expr, smt::Expr> splitEq(const smt::Expr& eq);
    static std::pair<smt::Expr, smt::Expr> splitNEq(const smt::Expr& neq);
    static ExprSet elimNEq(const ExprSet& es);

    static std::list<ExprSet> elimOr(const ExprSet& es);
    static std::list<ExprSet> reduce(const std::list<ExprSet>& sets, smt::Solver* solver);



private:
    static std::string toMidFixString(const smt::inode<smt::Node>& node);

    static bool isOr(const smt::Expr& e);
    static std::pair<smt::Expr, smt::Expr> splitOr(const smt::Expr& orExpr);
    static std::list<ExprSet> combineOrs(ExprList orList, std::list<ExprSet> sets);
    static std::list<ExprSet> concat(ExprSet base, std::list<ExprSet> sets);


};

}

#endif /* SRC_TERMINATION_UTILS_H_ */
