#ifndef CEAGLE_SIMPLE_ADVISOR_H
#define CEAGLE_SIMPLE_ADVISOR_H

#include "Advisor.h"

namespace ceagle {

class SimpleAdvisor : public Advisor<> {
public:
    void dfsVisit(const Module& module, R_BBPATH currentPath);
    NextChoice<P_BB> dfsNext(const Module& module, R_BBPATH currentPath);
    BackwardPolicy dfsBackward(const Module& module, R_BBPATH currentPath);
};

}

#endif
