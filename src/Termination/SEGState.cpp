/*
 * SEGState.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: jiaxiang
 */

#include "SEGState.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "Utils.h"

extern int g_debug_info;

namespace ceagle {

SEGState::SEGState(ProgramCounter p) : pc(p) {
}

void SEGState::show() {
    llvm::errs() << "Next inst (p): " << *(this->pc.i) << "\n";
    llvm::errs() << "Local vars (LV):\n";
    for (auto i = this->LVi.begin(); i != this->LVi.end(); i++) {
        llvm::errs() << "    " << (*i).first << " -> " << (*i).second.str() << "\n";
    }
    llvm::errs() << "Local alloca list (AL):\n";
    for (auto i = this->ALi.begin(); i != this->ALi.end(); i++) {
        llvm::errs() << "    [ " << (*i).first.str() << " , " << (*i).second.str() << " ]\n";
    }
    llvm::errs() << "Knowledge base (KB):\n";
    for (auto i = this->KB.begin(); i != this->KB.end(); i++) {
        llvm::errs() << "    " << (*i).str() << "\n";
    }
    llvm::errs() << "Memory table (PT):\n";
    for (auto i = this->PT.begin(); i != this->PT.end(); i++) {
        llvm::errs() << "    " << (*i).ad.str() << " --[" << Utils::toString((*i).ty) << "]--> " << (*i).val.str() << "\n";
    }
    llvm::errs() << "Predicates (FO):\n";
    for (auto i = this->FO.begin(); i != this->FO.end(); i++) {
        llvm::errs() << "    " << *i << "\n";
        //llvm::errs() << "    " << Utils::toMidFixString(*i) << "\n";
    }

}

ExprSet SEGState::getPredicate(smt::Solver* solver) {
    // put KB in
    ExprSet predicates = ExprSet(this->KB);

    smt::Expr expr;

    // put pairs of AL_i in
    for (auto v = this->ALi.begin(); v != this->ALi.end(); v++) {
            expr = smt::IntValue(1) <= (*v).first;
            predicates.insert(expr);
            expr = (*v).first <= (*v).second;
            predicates.insert(expr);
    }

    // put relations between pairs of AL_i in
    for (auto v = this->ALi.begin(); v != this->ALi.end(); v++) {
        auto w = v;
        for (w++; w != this->ALi.end(); w++) {
            if ((*v).first.str().compare((*w).first.str())
                || (*v).second.str().compare((*w).second.str())) {
                expr = (*v).second < (*w).first;
                expr = expr || ((*w).second < (*v).first);
                predicates.insert(expr);
            }
        }
    }

    // put pairs of PT in
    for (auto v = this->PT.begin(); v != this->PT.end(); v++) {
            expr = (*v).ad > smt::IntValue(0);
            predicates.insert(expr);
    }

    // put relations between pairs of PT in (deriving ==)
    int originalSize, newSize;
    smt::Expr condition;
    do {
        originalSize = predicates.size();

        // may enlarge predicates, or may not
        for (auto v = this->PT.begin(); v != this->PT.end(); v++) {
            auto w = v;
            for (w++; w != this->PT.end(); w++) {
                if ((*v).ty == (*w).ty) { // types must be the same
                    if ((*v).ad.str().compare((*w).ad.str())
                        || (*v).val.str().compare((*w).val.str())) { // not the same pairs
                        condition = (*v).ad == (*w).ad;
                        if (Utils::implies(predicates, condition, solver)) {
                            expr = (*v).val == (*w).val;
                            predicates.insert(expr);
                        }
                    }
                }
            }
        }

        newSize = predicates.size();
    } while (newSize > originalSize);


    // put relations between pairs of PT in (deriving !=)
    do {
        originalSize = predicates.size();

        // may enlarge predicates, or may not
        for (auto v = this->PT.begin(); v != this->PT.end(); v++) {
            auto w = v;
            for (w++; w != this->PT.end(); w++) {
                if ((*v).ty == (*w).ty) { // types must be the same
                    if ((*v).ad.str().compare((*w).ad.str())
                        || (*v).val.str().compare((*w).val.str())) { // not the same pairs
                        condition = !((*v).val == (*w).val);
                        if (Utils::implies(predicates, condition, solver)) {
                            expr = !((*v).ad == (*w).ad);
                            predicates.insert(expr);
                        }
                    }
                }
            }
        }

        newSize = predicates.size();
    } while (newSize > originalSize);


    return predicates;
}

ExprSet SEGState::extendPredicate(smt::Solver* solver) {
    // TODO: in this version, we did not put the relations v1 - v2 = k(v3 - v4) into the set

    // <a> \subsetof <<a>>
    ExprSet predicates = ExprSet(this->FO);

    std::pair<smt::Expr, smt::Expr> ep;
    smt::Expr ge, le, gt, lt;

    for (auto i = this->FO.begin(); i != this->FO.end(); i++) {
        if (Utils::isEq(*i)) { // the equations from <a>
            ep = Utils::splitEq(*i);
            ge = ep.first;
            le = ep.second;
            predicates.insert(ge);
            predicates.insert(le);
        } else if (Utils::isNEq(*i)) { // non-equations from <a>
            ep = Utils::splitNEq(*i);
            gt = ep.first;
            lt = ep.second;

            if (Utils::implies(this->FO, gt, solver)) { // if t1 > t2 holds
                predicates.insert(gt);
            } else if (Utils::implies(this->FO, lt, solver)) { // if t1 < t2 holds
                predicates.insert(lt);
            }
        }
    }

    return predicates;
}

smt::Expr SEGState::getLVi(llvm::Value* v) {
    if (llvm::ConstantInt* ci = llvm::dyn_cast<llvm::ConstantInt>(v)) {
        //if (ci->getBitWidth() <= 32)
        return smt::IntValue(ci->getSExtValue());
    } else {
        return this->LVi[v];
    }
}

Domain SEGState::getDomain() {
    Domain d;
    for (auto i = this->LVi.begin(); i != this->LVi.end(); i++) {
        d.insert((*i).first);
    }
    return d;
}

// compute mu : this.lv -> lv
Substitution SEGState::computeMu(LocalVars lv) {
    Substitution mu;
    Domain d = this->getDomain();
    for (auto i = d.begin(); i != d.end(); i++) {
        mu[this->LVi[*i].str()] = lv[*i];
    }
    return mu;
}


bool SEGState::isGeneralizationOf(SEGState* s, smt::Solver* solver) {
    // Domains should be the same
    if (this->getDomain() != s->getDomain()) {
        if (g_debug_info >= 3) {
            llvm::errs() << "unsatisfied: domain\n";
        }
        return false;
    }

    Substitution mu = this->computeMu(s->LVi);

    // <s> => mu(this.KB)
    if (!Utils::implies(s->FO, Utils::substitute(mu, this->KB), solver)) {
        if (g_debug_info >= 3) {
            llvm::errs() << "unsatisfied: <a> => mu(KB')\n";
        }
        return false;
    }

    // for all [v1,v2] \in this.ALi , [mu(v1), mu(v2)] \in s.ALi
    for (auto i = this->ALi.begin(); i != this->ALi.end(); i++) {
        if (s->ALi.count(Utils::substitute(mu, *i)) == 0) {
            if (g_debug_info >= 3) {
                llvm::errs() << "unsatisfied: " << (*i).first << "," << (*i).second << " in AL\n";
            }
            return false;
        }
    }

    // for all (v1 -> v2) \in this.PT, (mu(v1) -> mu(v2)) \in s.PT
    for (auto i = this->PT.begin(); i != this->PT.end(); i++) {
        if (s->PT.count(Utils::substitute(mu, *i)) == 0) {
            if (g_debug_info >= 3) {
                llvm::errs() << "unsatisfied: " << (*i).ad << "->" << (*i).val << " in PT\n";
            }
            return false;
        }
    }

    // satisfies all the above conditions
    return true;
}

}
