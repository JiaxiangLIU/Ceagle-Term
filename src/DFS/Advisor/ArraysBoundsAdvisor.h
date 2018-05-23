#ifndef CEAGLE_ARRAYS_BOUNDS_ADVISOR_H
#define CEAGLE_ARRAYS_BOUNDS_ADVISOR_H

#include "Advisor.h"

namespace ceagle {

class ArraysBoundsAdvisor : public Advisor<> {
    NextChoice<P_BB> m_nextChoice;
public:
    void dfsVisit(const Module& module, R_BBPATH currentPath);
    NextChoice<P_BB> dfsNext(const Module& module, R_BBPATH currentPath);
    BackwardPolicy dfsBackward(const Module& module, R_BBPATH currentPath);
};

}

#endif
