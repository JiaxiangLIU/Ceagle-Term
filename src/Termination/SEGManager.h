/*
 * SEGManager.h
 *
 *  Created on: Dec 30, 2016
 *      Author: jiaxiang
 */

#ifndef SRC_TERMINATION_SEGMANAGER_H_
#define SRC_TERMINATION_SEGMANAGER_H_

#include "boost/graph/adjacency_list.hpp"
#include "BasicTypes.h"
#include "SEGState.h"
#include <string>
#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"

namespace ceagle {

using namespace boost;

struct VertexProperty {
	SEGState* ptrState;
    bool visited;
    std::string label;
};

struct EdgeProperty {
    EdgeType type;
};

typedef adjacency_list<listS, listS, bidirectionalS,
        VertexProperty, EdgeProperty> SEG;

typedef std::vector<SEG::vertex_descriptor> Path;

class SEGManager {

private:
    llvm::Module* module;
    SEG seg;
    SEG::vertex_descriptor entry;
    std::map<SEGState*, SEG::vertex_descriptor> mapS2V;
    ExprSet symVars;

    int varCnt = 0;
    int stateNum = 0;
    smt::Solver* solver;

public:
    SEGManager();
    SEGManager(llvm::Module* m);

    int getVarCnt();
    SEG getSEG();

    void constructSEG();
    void simplifySEG();

    void show();
    void show(SEG g);

    void toDOT(std::string fn);
    void toKITTEL(std::string fn);

private:
    std::list<SEGState*> evaluate(SEGState* state);
    SEGState* merge(SEGState* a, SEGState* b);

    smt::Expr disjoint(const AllocationCell& c1, const AllocationCell& c2);
    smt::Expr negative(const smt::Expr& e);

    bool implies(const ExprSet& predicates, const smt::Expr& argument);
    LocalVars constructFreshLV(LocalVars lv);

    std::list<SEG::vertex_descriptor> findSamePC(Path path, SEG::vertex_descriptor v);
    bool hasInEvalEdge(SEG::vertex_descriptor v);
    bool hasInRefEdge(SEG::vertex_descriptor v);
    std::list<SEG::vertex_descriptor> firstUnvisitedChild(SEG::vertex_descriptor v);

    void procGeneralization(SEG::vertex_descriptor v,
            SEG::vertex_descriptor genTarget, Path& path);
    void procEvaluation(SEG::vertex_descriptor v, Path& path);
    void expandSEG(Path path);

    SEG::vertex_descriptor add_vertex(SEGState* s, SEG& g);
    void dfs(SEG::vertex_descriptor current, SEG& dest, SEG::vertex_descriptor destCur,
            std::map<SEG::vertex_descriptor, bool>& visited,
            std::map<SEG::vertex_descriptor, SEG::vertex_descriptor>& mapV);

    void showEvalResult(SEGState* v, SEGState* w);
    void showRefResult(SEGState* v, SEGState* w1, SEGState* w2);
    void showMergeResult(SEGState* a, SEGState* b, SEGState* c);

    llvm::BasicBlock::iterator firstNonPhi(llvm::BasicBlock* b);
    std::pair<LocalVars, ExprSet> computePhi(SEGState* state,
            llvm::BasicBlock* curBlk, llvm::BasicBlock* nextBlk);

};

}

#endif /* SRC_TERMINATION_SEGMANAGER_H_ */
