#ifndef CEAGLE_NAIVE_ADVISOR_H
#define CEAGLE_NAIVE_ADVISOR_H

#include <memory>

#include "Advisor.h"

#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"

#define NAIVE_ADVISOR_UNROLL_DEPTH_DEFAULT 12

extern int g_naive_advisor_unroll;

namespace ceagle {

class NaiveAdvisor : public Advisor<> {
    NextChoice<P_BB> m_nextChoice;
    std::vector<P_BB> m_visited;
    std::map<P_BB, int> m_loopHeads; // probable loop heads, stores blocks that are visited more than once
    std::unique_ptr<smt::Solver> m_solver;
public:
    int m_loopUnrollDepth = NAIVE_ADVISOR_UNROLL_DEPTH_DEFAULT;

    NaiveAdvisor() : m_solver(new smt::Z3RawSolver()) {
        if (g_naive_advisor_unroll > NAIVE_ADVISOR_UNROLL_DEPTH_DEFAULT) {
            this->m_loopUnrollDepth = g_naive_advisor_unroll;
            std::cout << "NaiveAdvisor setting unroll depth to " << this->m_loopUnrollDepth << std::endl;
        }
    }

    void dfsVisit(const Module& module, R_BBPATH currentPath);
    NextChoice<P_BB> dfsNext(const Module& module, R_BBPATH currentPath);
    BackwardPolicy dfsBackward(const Module& module, R_BBPATH currentPath);
};

}

#endif
