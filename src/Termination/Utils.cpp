/*
 * utils.cpp
 *
 *  Created on: Jan 18, 2017
 *      Author: jiaxiang
 */

#include "Utils.h"
#include "llvm/Support/raw_ostream.h"

// debug information : by jiaxiang
int g_debug_info = 2;

namespace ceagle {

PointerTable Utils::intersection(const PointerTable& a, const PointerTable& b) {
    PointerTable pt;
    for (auto i = a.begin(); i != a.end(); i++) {
        if (b.count(*i) == 1)
            pt.insert(*i);
    }
    return pt;
}

AllocationList Utils::intersection(const AllocationList& a, const AllocationList& b) {
    AllocationList al;
    for (auto i = a.begin(); i != a.end(); i++) {
        if (b.count(*i) == 1)
            al.insert(*i);
    }
    return al;
}

std::string Utils::toString(Type type) {
    if (type == Type::i1) {
            return "i1";
    } else if (type == Type::i8) {
        return "i8";
    } else if (type == Type::i16) {
        return "i16";
    } else {
        return "i32";
    }
}

std::string Utils::toString(EdgeType type) {
    if (type == EdgeType::evaluation) {
            return "evaluation";
    } else if (type == EdgeType::generalization) {
        return "generalization";
    } else {
        return "refinement";
    }
}

// TODO: the NOT node (in fact, the only possibility is NotEq) has to be done!!!
std::string Utils::toMidFixString(smt::Expr expr) {
    auto root = expr.getRoot();
    std::string str;
    if (root) {
        str = toMidFixString(*root);
    }
    return str;
}

std::string Utils::toMidFixString(const smt::inode<smt::Node>& node) {
    std::ostringstream out;
    bool swap = false;

    if (node.getValue()->isLeaf()) {
        out << *node.getValue();
    } else {

        smt::inode<smt::Node> *child = node.getChild();

        // speical case : multiplication(*), we have to put the constant to the left
        if (smt::OperatorNode* op = dynamic_cast<smt::OperatorNode*>(node.getValue())) {
            if (op->getOpcode() == smt::OPCODE::MULTIPLY) {
                smt::Expr lhs = smt::Expr(*child);
                if (smt::IntValue* iv = dynamic_cast<smt::IntValue*>(&lhs)) {
                    swap = false;
                } else {
                    swap = true;
                }
            }
        }

        if (swap) {
            smt::inode<smt::Node> *rhs = child;
            child = child->getNext();

            out << toMidFixString(*child);
            out << " " << *node.getValue() << " ";
            out << toMidFixString(*rhs);
        } else {
            out << toMidFixString(*child);
            out << " " << *node.getValue() << " ";
            child = child->getNext();
            out << toMidFixString(*child);
        }
    }
    return out.str();
}

bool Utils::implies (const ExprSet& predicates, const smt::Expr& argument, smt::Solver* solver) {
    solver->reset();
    for (auto j = predicates.begin(); j != predicates.end(); j++) {
        solver->add(*j);
    }
    solver->add(!argument);
    auto result = solver->checkSat();
    if (result == smt::CheckResult::UNSAT)
        return true;
    else if (result == smt::CheckResult::SAT)
        return false;
    else {
        llvm::errs() << "SMT check unknown\n";
        exit(-1);
    }
}

bool Utils::implies (const ExprSet& predicates, const ExprSet& arguments, smt::Solver* solver) {
    solver->reset();
    for (auto j = predicates.begin(); j != predicates.end(); j++) {
        solver->add(*j);
    }

    smt::Expr arg = smt::BoolValue(true);
    for (auto i = arguments.begin(); i != arguments.end(); i++) {
        arg = arg && (*i);
    }

    solver->add(!arg);
    auto result = solver->checkSat();
    if (result == smt::CheckResult::UNSAT)
        return true;
    else if (result == smt::CheckResult::SAT)
        return false;
    else {
        llvm::errs() << "SMT check unknown\n";
        exit(-1);
    }
}

smt::Expr Utils::substitute(const Substitution& mu, const smt::Expr& e) {
    return e.replace(mu);
}

ExprSet Utils::substitute(const Substitution& mu, const ExprSet& es) {
    ExprSet result;
    for (auto i = es.begin(); i != es.end(); i++) {
        result.insert(substitute(mu, *i));
    }
    return result;
}

ExprList Utils::substitute(const Substitution& mu, const ExprList& el) {
    ExprList result;
    for (auto i = el.begin(); i != el.end(); i++) {
        result.push_back(substitute(mu, *i));
    }
    return result;
}

AllocationCell Utils::substitute(const Substitution& mu, const AllocationCell& ac) {
    return std::make_pair(substitute(mu, ac.first), substitute(mu, ac.second));
}

AllocationList Utils::substitute(const Substitution& mu, const AllocationList& al) {
    AllocationList result;
    for (auto i = al.begin(); i != al.end(); i++) {
        result.insert(substitute(mu, *i));
    }
    return result;
}

MemCell Utils::substitute(const Substitution& mu, const MemCell& mc) {
    return {substitute(mu, mc.ad), mc.ty, substitute(mu, mc.val)};
}

PointerTable Utils::substitute(const Substitution& mu, const PointerTable& pt) {
    PointerTable result;
    for (auto i = pt.begin(); i != pt.end(); i++) {
        result.insert(substitute(mu, *i));
    }
    return result;
}

bool Utils::isEq(const smt::Expr& e) {
    auto root = e.getRoot();
    if (smt::OperatorNode* op = dynamic_cast<smt::OperatorNode*>(root->getValue())) {
        if (op->getOpcode() == smt::OPCODE::EQU) {
            return true;
        } else
            return false;
    } else
        return false;
}

bool Utils::isNEq(const smt::Expr& e) {
    auto root = e.getRoot();
    if (smt::OperatorNode* op = dynamic_cast<smt::OperatorNode*>(root->getValue())) {
        if (op->getOpcode() == smt::OPCODE::NOT) {
            return isEq(smt::Expr(*(root->getChild())));
        } else
            return false;
    } else
        return false;
}

std::pair<smt::Expr, smt::Expr> Utils::splitEq(const smt::Expr& eq) {
    smt::inode<smt::Node>* lhs = eq.getRoot()->getChild();
    smt::inode<smt::Node>* rhs = lhs->getNext();
    smt::Expr ge = smt::Expr(*lhs) >= smt::Expr(*rhs);
    smt::Expr le = smt::Expr(*lhs) <= smt::Expr(*rhs);
    return std::make_pair(ge, le);
}

std::pair<smt::Expr, smt::Expr> Utils::splitNEq(const smt::Expr& neq) {
    smt::inode<smt::Node>* lhs = neq.getRoot()->getChild()->getChild();
    smt::inode<smt::Node>* rhs = lhs->getNext();
    smt::Expr gt = smt::Expr(*lhs) > smt::Expr(*rhs);
    smt::Expr lt = smt::Expr(*lhs) < smt::Expr(*rhs);
    return std::make_pair(gt, lt);
}

ExprSet Utils::elimNEq(const ExprSet& es) {
    ExprSet result;
    std::pair<smt::Expr, smt::Expr> ep;
    smt::Expr gt, lt;

    for (auto i = es.begin(); i != es.end(); i++) {
        if (isNEq(*i)) {
            ep = splitNEq(*i);
            gt = ep.first;
            lt = ep.second;
            result.insert(gt || lt);
        } else {
            result.insert(*i);
        }
    }

    return result;
}

bool Utils::isOr(const smt::Expr& e) {
    auto root = e.getRoot();
    if (smt::OperatorNode* op = dynamic_cast<smt::OperatorNode*>(root->getValue())) {
        if (op->getOpcode() == smt::OPCODE::OR) {
            return true;
        } else
            return false;
    } else
        return false;
}

std::pair<smt::Expr, smt::Expr> Utils::splitOr(const smt::Expr& orExpr) {
    smt::inode<smt::Node>* lhs = orExpr.getRoot()->getChild();
    smt::inode<smt::Node>* rhs = lhs->getNext();
    smt::Expr e1 = smt::Expr(*lhs);
    smt::Expr e2 = smt::Expr(*rhs);
    return std::make_pair(e1, e2);
}

std::list<ExprSet> Utils::combineOrs(ExprList orList, std::list<ExprSet> sets) {
    if (orList.size() == 0) {
        return sets;
    } else {
        smt::Expr orExpr = orList.front();
        orList.pop_front();

        auto temp = splitOr(orExpr);
        smt::Expr e1 = temp.first;
        smt::Expr e2 = temp.second;

        std::list<ExprSet> rec, result;
        rec = combineOrs(orList, sets);
        ExprSet es1, es2;

        for (auto i = rec.begin(); i != rec.end(); i++) {
            es1 = ExprSet(*i);
            es1.insert(e1);
            result.push_back(es1);
            es2 = ExprSet(*i);
            es2.insert(e2);
            result.push_back(es2);
        }

        return result;
    }
}

std::list<ExprSet> Utils::concat(ExprSet base, std::list<ExprSet> sets) {
    for (auto i = sets.begin(); i != sets.end(); i++) {
        (*i).insert(base.begin(), base.end());
    }
    return sets;
}

std::list<ExprSet> Utils::elimOr(const ExprSet& es) {
    ExprSet nonOr;
    ExprList orList;

    for (auto i = es.begin(); i != es.end(); i++) {
        if (isOr(*i)) {
            orList.push_back(*i);
        } else {
            nonOr.insert(*i);
        }
    }

    std::list<ExprSet> result;
    ExprSet empty;
    result.push_back(empty);

    result = combineOrs(orList, result);
    result = concat(nonOr, result);

    return result;
}

std::list<ExprSet> Utils::reduce(const std::list<ExprSet>& sets, smt::Solver* solver) {
    std::list<ExprSet> result;
    smt::Expr f = smt::BoolValue(false);
    for (auto i = sets.begin(); i != sets.end(); i++) {
        if (!implies(*i, f, solver)) {
            result.push_back(*i);
        }
    }
    return result;
}


}


