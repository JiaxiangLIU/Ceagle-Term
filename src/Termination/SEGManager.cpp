/*
 * SEGManager.cpp
 *
 *  Created on: Dec 30, 2016
 *      Author: jiaxiang
 */

#include "SEGManager.h"
#include <iostream>
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "SMT/Middleware/AST.h"
#include <unordered_set>
#include <system_error>
#include "llvm/Support/FileSystem.h"

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "Utils.h"

extern int g_debug_info;

namespace ceagle {

using namespace boost;
using namespace std;
using namespace llvm;

SEGManager::SEGManager() {
}

SEGManager::SEGManager(Module* m) {
    this->module = m;

    BasicBlock* mainEntry = m->getFunction("main")->begin();
    BasicBlock::iterator inst = mainEntry->begin();
    SEGState* entryState = new SEGState({mainEntry, inst});

    SEG::vertex_descriptor v = add_vertex(entryState, this->seg);
    this->entry = v;

    this->solver = new smt::Z3RawSolver(true);
}

SEG SEGManager::getSEG() {
    return this->seg;
}

int SEGManager::getVarCnt() {
    return this->varCnt;
}

std::list<SEGState*> SEGManager::evaluate(SEGState* state) {
    std::list<SEGState*> stateList;

    if (Value* ai = dyn_cast<AllocaInst>(state->pc.i)) {
    // Inst: alloca

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr v1 = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        smt::Expr v2 = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(v1);
        symVars.insert(v2);
        newState->LVi = LocalVars(state->LVi);
        newState->LVi[ai] = v1;
        newState->ALi = AllocationList(state->ALi);
        newState->ALi.insert(make_pair(v1, v2));
        newState->KB = ExprSet(state->KB);
        newState->KB.insert(v2 == (v1 + smt::IntValue(3)));
        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);;

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of alloca
    } else if (StoreInst* si = dyn_cast<StoreInst>(state->pc.i)) {
    // Inst: store

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        // check if address allocated
        bool exist = false;
        smt::Expr condition;
        Value* ad = si->getPointerOperand();
        Value* t = si->getValueOperand();
        for (auto v = state->ALi.begin(); v != state->ALi.end(); v++) {
            condition = (*v).first <= state->LVi[ad];
            condition = condition && ((state->LVi[ad] + smt::IntValue(3)) <= (*v).second);
            if (implies(state->FO, condition)) {
                exist = true;
                break;
            }
        }


        if (!exist) { // address not allocated, causing an ERR
            errs() << "store instruction access to un-allocated address\n";
            errs() << "instruction: " << *si << "\n";
            exit(-1);
        } else { // we can generate a new state now
            newState->LVi = LocalVars(state->LVi);
            newState->ALi = AllocationList(state->ALi);
            newState->KB = ExprSet(state->KB);

            smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
            symVars.insert(w);

            // construct KB
            newState->KB.insert(w == state->getLVi(t));

            // construct PT
            for (auto i = state->PT.begin(); i != state->PT.end(); i++) {
                condition = disjoint(
                        make_pair(state->LVi[ad], state->LVi[ad] + smt::IntValue(3)),
                        make_pair((*i).ad, (*i).ad + smt::IntValue(3)));
                if (implies(state->FO, condition)) {
                    newState->PT.insert(*i);
                }
            }
            MemCell cell = {state->LVi[ad], Type::i32, w};
            newState->PT.insert(cell);

            // compute the FO predicates
            newState->FO = newState->getPredicate(this->solver);

            if (g_debug_info >= 3) {
                showEvalResult(state, newState);
            }

            stateList.push_back(newState);
        }

    // end of store
    } else if (BranchInst* bi = dyn_cast<BranchInst>(state->pc.i)) {
    // Inst: br

        if (bi->isUnconditional()) { // Unconditional branch
            BasicBlock* blk = bi->getSuccessor(0);
            //BasicBlock::iterator inst = blk->begin();
            BasicBlock::iterator inst = firstNonPhi(blk);
            std::pair<LocalVars, ExprSet> compPhiRes = computePhi(state, bi->getParent(), blk);
            ProgramCounter next = {blk, inst};
            SEGState* newState = new SEGState(next);

            //newState->LVi = LocalVars(state->LVi);
            newState->LVi = compPhiRes.first;
            newState->ALi = AllocationList(state->ALi);

            newState->KB = ExprSet(state->KB);
            for (auto j = compPhiRes.second.begin(); j != compPhiRes.second.end(); j++) {
                newState->KB.insert(*j);
            }

            newState->PT = PointerTable(state->PT);
            newState->FO = newState->getPredicate(this->solver);

            if (g_debug_info >= 3) {
                showEvalResult(state, newState);
            }

            stateList.push_back(newState);

        // end of Unconditional br
        } else { // Conditional branch

            Value* t = bi->getCondition();
            smt::Expr trueCond = state->getLVi(t) == smt::IntValue(1);
            smt::Expr falseCond = state->getLVi(t) == smt::IntValue(0);

            if (implies(state->FO, trueCond)) { // case: t == 1
                BasicBlock* blk = bi->getSuccessor(0);
                //BasicBlock::iterator inst = blk->begin();
                BasicBlock::iterator inst = firstNonPhi(blk);
                ProgramCounter next = {blk, inst};
                SEGState* newState = new SEGState(next);

                std::pair<LocalVars, ExprSet> compPhiRes = computePhi(state, bi->getParent(), blk);

                //newState->LVi = LocalVars(state->LVi);
                newState->LVi = compPhiRes.first;
                newState->ALi = AllocationList(state->ALi);

                newState->KB = ExprSet(state->KB);
                for (auto j = compPhiRes.second.begin(); j != compPhiRes.second.end(); j++) {
                    newState->KB.insert(*j);
                }

                newState->PT = PointerTable(state->PT);
                newState->FO = newState->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showEvalResult(state, newState);
                }

                stateList.push_back(newState);

            // end of case t==1
            } else if (implies(state->FO, falseCond)) { // case: t == 0
                BasicBlock* blk = bi->getSuccessor(1);
                //BasicBlock::iterator inst = blk->begin();
                BasicBlock::iterator inst = firstNonPhi(blk);
                ProgramCounter next = {blk, inst};
                SEGState* newState = new SEGState(next);

                std::pair<LocalVars, ExprSet> compPhiRes = computePhi(state, bi->getParent(), blk);

                //newState->LVi = LocalVars(state->LVi);
                newState->LVi = compPhiRes.first;
                newState->ALi = AllocationList(state->ALi);

                newState->KB = ExprSet(state->KB);
                for (auto j = compPhiRes.second.begin(); j != compPhiRes.second.end(); j++) {
                    newState->KB.insert(*j);
                }

                newState->PT = PointerTable(state->PT);
                newState->FO = newState->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showEvalResult(state, newState);
                }

                stateList.push_back(newState);

            // end of case t==0
            } else { // not enough information, need to do refinement

                SEGState* newStateTrue = new SEGState(state->pc);
                newStateTrue->LVi = LocalVars(state->LVi);
                newStateTrue->ALi = AllocationList(state->ALi);
                newStateTrue->KB = ExprSet(state->KB);
                newStateTrue->PT = PointerTable(state->PT);
                newStateTrue->KB.insert(trueCond);
                newStateTrue->FO = newStateTrue->getPredicate(this->solver);

                SEGState* newStateFalse = new SEGState(state->pc);
                newStateFalse->LVi = LocalVars(state->LVi);
                newStateFalse->ALi = AllocationList(state->ALi);
                newStateFalse->KB = ExprSet(state->KB);
                newStateFalse->PT = PointerTable(state->PT);
                newStateFalse->KB.insert(falseCond);
                newStateFalse->FO = newStateFalse->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showRefResult(state, newStateTrue, newStateFalse);
                }

                stateList.push_back(newStateTrue);
                stateList.push_back(newStateFalse);

            // end of br refinement
            }

        // end of Conditional br
        }

    // end of br
    } else if (LoadInst* li = dyn_cast<LoadInst>(state->pc.i)) {
    // Inst: load

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        // check if address allocated
        bool exist = false;
        smt::Expr condition;
        Value* ad = li->getPointerOperand();
        for (auto v = state->ALi.begin(); v != state->ALi.end(); v++) {
            condition = (*v).first <= state->LVi[ad];
            condition = condition && ((state->LVi[ad] + smt::IntValue(3)) <= (*v).second);
            if (implies(state->FO, condition)) {
                exist = true;
                break;
            }
        }

        if (!exist) { // address not allocated, causing an ERR
            errs() << "load instruction access to un-allocated address\n";
            errs() << "instruction: " << *li << "\n";
            exit(-1);
        } else { // we can generate a new state now
            newState->LVi = LocalVars(state->LVi);
            newState->ALi = AllocationList(state->ALi);
            newState->KB = ExprSet(state->KB);
            newState->PT = PointerTable(state->PT);

            smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
            symVars.insert(w);

            newState->LVi[li] = w;

            // construct PT
            MemCell cell = {state->LVi[ad], Type::i32, w};
            newState->PT.insert(cell);

            // compute the FO predicates
            newState->FO = newState->getPredicate(this->solver);

            if (g_debug_info >= 3) {
                showEvalResult(state, newState);
            }

            stateList.push_back(newState);
        }

    // end of load
    } else if (ICmpInst* ici = dyn_cast<ICmpInst>(state->pc.i)) {
    // Inst: icmp

        CmpInst::Predicate pred = ici->getPredicate();
        Value* t1 = ici->getOperand(0);
        Value* t2 = ici->getOperand(1);
        smt::Expr condition;

        bool support = true;
        switch (pred) {
        case CmpInst::ICMP_EQ:
            condition = state->getLVi(t1) == state->getLVi(t2);
            break;

        case CmpInst::ICMP_NE:
            condition = !(state->getLVi(t1) == state->getLVi(t2));
            break;

        case CmpInst::ICMP_SGT:
            condition = state->getLVi(t1) > state->getLVi(t2);
            break;

        case CmpInst::ICMP_SGE:
            condition = state->getLVi(t1) >= state->getLVi(t2);
            break;

        case CmpInst::ICMP_SLT:
            condition = state->getLVi(t1) < state->getLVi(t2);
            break;

        case CmpInst::ICMP_SLE:
            condition = state->getLVi(t1) <= state->getLVi(t2);
            break;

        default:
            // there exist non-supported instructions
            support = false;
            break;
        }

        if (support) {
        // if the comparison is supported, then we generate new state(s)

            if (implies(state->FO, condition)) { // condition holds, icmp returns true
                BasicBlock::iterator inst = state->pc.i;
                inst++;
                ProgramCounter next = {state->pc.b, inst};
                SEGState* newState = new SEGState(next);

                smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
                symVars.insert(w);

                newState->LVi = LocalVars(state->LVi);
                newState->ALi = AllocationList(state->ALi);
                newState->KB = ExprSet(state->KB);
                newState->PT = PointerTable(state->PT);

                newState->LVi[ici] = w;
                newState->KB.insert(w == smt::IntValue(1));

                // compute the FO predicates
                newState->FO = newState->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showEvalResult(state, newState);
                }

                stateList.push_back(newState);

            // end of case: condition holds
            } else if (implies(state->FO, !condition)) { // condition fails, icmp returns false
                BasicBlock::iterator inst = state->pc.i;
                inst++;
                ProgramCounter next = {state->pc.b, inst};
                SEGState* newState = new SEGState(next);

                smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
                symVars.insert(w);

                newState->LVi = LocalVars(state->LVi);
                newState->ALi = AllocationList(state->ALi);
                newState->KB = ExprSet(state->KB);
                newState->PT = PointerTable(state->PT);

                newState->LVi[ici] = w;
                newState->KB.insert(w == smt::IntValue(0));

                // compute the FO predicates
                newState->FO = newState->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showEvalResult(state, newState);
                }

                stateList.push_back(newState);

            // end of case: condition fails
            } else { // not enough infomation, need to do refinement

                SEGState* newStateTrue = new SEGState(state->pc);
                newStateTrue->LVi = LocalVars(state->LVi);
                newStateTrue->ALi = AllocationList(state->ALi);
                newStateTrue->KB = ExprSet(state->KB);
                newStateTrue->PT = PointerTable(state->PT);
                newStateTrue->KB.insert(condition);
                newStateTrue->FO = newStateTrue->getPredicate(this->solver);

                SEGState* newStateFalse = new SEGState(state->pc);
                newStateFalse->LVi = LocalVars(state->LVi);
                newStateFalse->ALi = AllocationList(state->ALi);
                newStateFalse->KB = ExprSet(state->KB);
                newStateFalse->PT = PointerTable(state->PT);
                newStateFalse->KB.insert(negative(condition));
                newStateFalse->FO = newStateFalse->getPredicate(this->solver);

                if (g_debug_info >= 3) {
                    showRefResult(state, newStateTrue, newStateFalse);
                }

                stateList.push_back(newStateTrue);
                stateList.push_back(newStateFalse);

	        // end of refinement
            }
        }

    // end of icmp
    } else if (BinaryOperator* bo = dyn_cast<BinaryOperator>(state->pc.i)) {
    // Inst: binary operators, e.g. add, sub, etc.

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        Value* t1 = bo->getOperand(0);
        Value* t2 = bo->getOperand(1);
        smt::Expr expr;

        switch (bo->getOpcode()) {
        case Instruction::BinaryOps::Add:
            expr = state->getLVi(t1) + state->getLVi(t2);
            break;

        case Instruction::BinaryOps::Sub:
            expr = state->getLVi(t1) - state->getLVi(t2);
            break;

        case Instruction::BinaryOps::Mul:
            expr = state->getLVi(t1) * state->getLVi(t2);
            break;

        default:
            // non-supported binary operator
            return stateList;
        }

        newState->LVi = LocalVars(state->LVi);
        newState->LVi[bo] = w;
        newState->ALi = AllocationList(state->ALi);
        newState->KB = ExprSet(state->KB);
        newState->KB.insert(w == expr);
        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of binary operators
    } else if (ReturnInst* ri = dyn_cast<ReturnInst>(state->pc.i)) {
    // Inst: ret
    // TODO: more intricate return operations needed
    // NOTE: Now we only consider single function

        if (g_debug_info >= 3) {
            showEvalResult(state, NULL);
        }

        stateList.push_back(NULL);

    // end of ret
    } else if (CallInst* ci = dyn_cast<CallInst>(state->pc.i)) {
    // Inst: call @fuctionname()

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        newState->LVi = LocalVars(state->LVi);
        newState->LVi[ci] = w;
        newState->ALi = AllocationList(state->ALi);
        newState->KB = ExprSet(state->KB);
        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of call
    } else if (BitCastInst* bci = dyn_cast<BitCastInst>(state->pc.i)) {
    // Inst: bitcast

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        newState->LVi = LocalVars(state->LVi);
        newState->LVi[bci] = w;
        newState->ALi = AllocationList(state->ALi);

        newState->KB = ExprSet(state->KB);
        Value* t = bci->getOperand(0);
        newState->KB.insert(w == state->getLVi(t));

        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of bitcast
    } else if (SExtInst* sei = dyn_cast<SExtInst>(state->pc.i)) {
    // Inst: sext

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        newState->LVi = LocalVars(state->LVi);
        newState->LVi[sei] = w;
        newState->ALi = AllocationList(state->ALi);

        newState->KB = ExprSet(state->KB);
        Value* t = sei->getOperand(0);
        newState->KB.insert(w == state->getLVi(t));

        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of sext
    } else if (ZExtInst* zei = dyn_cast<ZExtInst>(state->pc.i)) {
    // Inst: zext

        BasicBlock::iterator inst = state->pc.i;
        inst++;
        ProgramCounter next = {state->pc.b, inst};
        SEGState* newState = new SEGState(next);

        smt::Expr w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        newState->LVi = LocalVars(state->LVi);
        newState->LVi[zei] = w;
        newState->ALi = AllocationList(state->ALi);

        newState->KB = ExprSet(state->KB);
        Value* t = zei->getOperand(0);
        newState->KB.insert(w == state->getLVi(t));

        newState->PT = PointerTable(state->PT);
        newState->FO = newState->getPredicate(this->solver);

        if (g_debug_info >= 3) {
            showEvalResult(state, newState);
        }

        stateList.push_back(newState);

    // end of zext
    }
    

    return stateList;
}

smt::Expr SEGManager::disjoint(const AllocationCell& v, const AllocationCell& w) {
    return (v.second < w.first) || (w.second < v.first);
}

bool SEGManager::implies(const ExprSet& predicates, const smt::Expr& argument) {
    return Utils::implies(predicates, argument, this->solver);
}

void SEGManager::show() {
    errs() << "entry: " << *(seg[entry].ptrState->pc.i) << "\n";

    SEG::vertex_iterator vertexIt, vertexEnd;
    SEG::adjacency_iterator neighbourIt, neighbourEnd;
    tie(vertexIt, vertexEnd) = vertices(seg);
    for (; vertexIt != vertexEnd; ++vertexIt)
     {
       errs() << seg[*vertexIt].label << " is connected with: ";
       tie(neighbourIt, neighbourEnd) = adjacent_vertices(*vertexIt, seg);
       for (; neighbourIt != neighbourEnd; ++neighbourIt)
         errs() << seg[*neighbourIt].label << " by " << Utils::toString(seg[edge(*vertexIt, *neighbourIt, seg).first].type) << ", ";
       errs() << "\n";
      }
}

void SEGManager::toDOT(std::string fn) {
    //ofstream out(fn + ".dot");
    std::error_code ec;
    raw_fd_ostream out(fn + ".dot", ec, sys::fs::OpenFlags::F_RW);

    //if (out.is_open()) {
    out << "digraph seg {" << "\n";

       /*
       out << "  A1 [shape=box]" << endl;
       out << "  B2 [shape=box]" << endl;
       out << "  C3 [shape=box]" << endl;
       out << "  D4 [shape=box]" << endl;
       */
    SEG::vertex_iterator vertexIt, vertexEnd;
    tie(vertexIt, vertexEnd) = vertices(seg);
    SEGState* s;
    for (; vertexIt != vertexEnd; ++vertexIt) {
        s = seg[*vertexIt].ptrState;
        out << "  " << seg[*vertexIt].label << " [shape=box, label=\"" << seg[*vertexIt].label << "\n";
        if (s == NULL) {
            out << "END" << "\n";
        } else {
            out << "Next inst (p): " << *(s->pc.i) << "\n";

            out << "Local vars (LV):\n";
            for (auto i = s->LVi.begin(); i != s->LVi.end(); i++) {
                if (!(i == s->LVi.begin())) {
                    out << ", ";
                }
                out << (*i).first << " -> " << (*i).second.str();
            }
            out << "\n";

            out << "Local alloca list (AL):\n";
            for (auto i = s->ALi.begin(); i != s->ALi.end(); i++) {
                if (!(i == s->ALi.begin())) {
                    out << ", ";
                }
                out << "[ " << (*i).first.str() << " , " << (*i).second.str() << " ]";
            }
            out << "\n";

            out << "Knowledge base (KB):\n";

            // the detailed version:
            for (auto i = s->KB.begin(); i != s->KB.end(); i++) {
                if (!(i == s->KB.begin())) {
                    out << ", ";
                }
                out << (*i).str();
            }
            out << "\n";

            // the simplified version:
            //out << "{...}\n";

            out << "Memory table (PT):\n";
            for (auto i = s->PT.begin(); i != s->PT.end(); i++) {
                if (!(i == s->PT.begin())) {
                    out << ", ";
                }
                out << (*i).ad.str() << " --[" << Utils::toString((*i).ty) << "]--> " << (*i).val.str();
            }
            out << "\n";

            /*
            llvm::errs() << "Predicates (FO):\n";
            for (auto i = this->FO.begin(); i != this->FO.end(); i++) {
                llvm::errs() << "    " << *i << "\n";
                //llvm::errs() << "    " << Utils::toMidFixString(*i) << "\n";
            }
            */
        }
        out << "\"] " << "\n";
    }
    out << "\n";


    SEG::edge_iterator edgeIt, edgeEnd;
    tie(edgeIt, edgeEnd) = edges(seg);
    for (; edgeIt != edgeEnd; ++edgeIt) {
        out << "  " << seg[source(*edgeIt,seg)].label
            << " -> " << seg[target(*edgeIt,seg)].label;
        if (seg[*edgeIt].type == EdgeType::refinement) {
            out << " [color=green,label=\"refine\",fontcolor=green]";
        } else if (seg[*edgeIt].type == EdgeType::generalization) {
            out << " [color=red,label=\"generalize\",fontcolor=red]";
        }
        out << "\n";
    }

       /*
       out << "  A1 -> { B2 C3 }" << endl;
       out << "  B2 -> D4" << endl;
       */
    out << "}" << "\n";

    out.close();
    //}
}

void SEGManager::toKITTEL(std::string fn) {
    errs() << "Generating KITTeL file...\n";

    ExprList varList, expList;
    smt::Expr v;
    SEGState *a, *b;
    Substitution mu;
    std::list<ExprSet> conds;

    for (int i = 0; i < this->varCnt; i++) {
        v = smt::Var("v" + to_string(i), smt::IntSort());
        varList.push_back(v);
    }

    ofstream out(fn + ".kittel");
    if (out.is_open()) {
        SEG::edge_iterator edgeIt, edgeEnd;
        tie(edgeIt, edgeEnd) = edges(seg);

        for (; edgeIt != edgeEnd; ++edgeIt) {

            // compute the expression list
            expList = varList;
            a = seg[source(*edgeIt,seg)].ptrState;
            if (seg[*edgeIt].type == EdgeType::generalization) {
                b = seg[target(*edgeIt,seg)].ptrState;
                mu = b->computeMu(a->LVi);
                expList = Utils::substitute(mu, expList);
            }

            // eliminate != and or in FO to get conditions
            conds = Utils::elimOr(Utils::elimNEq(a->FO));
            if (g_debug_info >= 2) {
                std::cout << "Before and after reduced: " << conds.size() << " / ";
            }
            conds = Utils::reduce(conds, this->solver);
            if (g_debug_info >= 2) {
                std::cout << conds.size() << std::endl;
            }

            // for each cond in the conditions, construct a rewrite rule
            for (auto cond = conds.begin(); cond != conds.end(); cond++) {

                // lhs
                out << seg[source(*edgeIt,seg)].label << "(";
                for (auto i = varList.begin(); i != varList.end(); i++) {
                    if (i == varList.begin()) {
                        out << (*i).str();
                    } else {
                        out << ", " << (*i).str();
                    }
                }
                out << ")";

                out << " -> ";

                // rhs
                out << seg[target(*edgeIt,seg)].label << "(";
                for (auto i = expList.begin(); i != expList.end(); i++) {
                    if (i == expList.begin()) {
                        out << (*i).str();
                    } else {
                        out << ", " << (*i).str();
                    }
                }
                out << ") ";

                // constraint
                if (!((*cond).empty())) {
                    out << "[ ";
                    for (auto i = (*cond).begin(); i != (*cond).end(); i++) {
                        if (i == (*cond).begin()) {
                            out << Utils::toMidFixString(*i);
                        } else {
                            out << " /\\ ";
                            out << Utils::toMidFixString(*i);
                        }
                    }
                    out << " ]";
                }

                out << endl;
            }
        }

        out.close();
    }

    errs() << "KITTeL file Generation Complete!\n";
}

void SEGManager::show(SEG g) {
    //errs() << "entry: " << *(seg[entry].ptrState->pc.i) << "\n";

    SEG::vertex_iterator vertexIt, vertexEnd;
    SEG::adjacency_iterator neighbourIt, neighbourEnd;
    tie(vertexIt, vertexEnd) = vertices(g);
    for (; vertexIt != vertexEnd; ++vertexIt)
     {
       errs() << g[*vertexIt].label << " is connected with: ";
       tie(neighbourIt, neighbourEnd) = adjacent_vertices(*vertexIt, g);
       for (; neighbourIt != neighbourEnd; ++neighbourIt)
         errs() << g[*neighbourIt].label << " by " << Utils::toString(g[edge(*vertexIt, *neighbourIt, g).first].type) << ", ";
       errs() << "\n";
      }
}

// Given a predicate Expr, return its negative predicate:
// == --> !=
// != --> ==
// >  --> <=
// >= --> <
// <  --> >=
// <= --> >
smt::Expr SEGManager::negative(const smt::Expr& e) {
    auto root = e.getRoot();
    if (smt::OperatorNode* op = dynamic_cast<smt::OperatorNode*>(root->getValue())) {
        smt::inode<smt::Node> *child, *lhs, *rhs;
        smt::Expr ne;
        switch (op->getOpcode()) {
        case smt::OPCODE::EQU:
            ne = !e;
            break;
        case smt::OPCODE::NOT:
            child = root->getChild();
            ne = smt::Expr(*child);
            break;
        case smt::OPCODE::GT:
            lhs = root->getChild();
            rhs = lhs->getNext();
            ne = smt::Expr(*lhs) <= smt::Expr(*rhs);
            break;
        case smt::OPCODE::GE:
            lhs = root->getChild();
            rhs = lhs->getNext();
            ne = smt::Expr(*lhs) < smt::Expr(*rhs);
            break;
        case smt::OPCODE::LT:
            lhs = root->getChild();
            rhs = lhs->getNext();
            ne = smt::Expr(*lhs) >= smt::Expr(*rhs);
            break;
        case smt::OPCODE::LE:
            lhs = root->getChild();
            rhs = lhs->getNext();
            ne = smt::Expr(*lhs) > smt::Expr(*rhs);
            break;
        default:
            ne = e;
        }
        return ne;
    } else
        return e;
}
  
LocalVars SEGManager::constructFreshLV(LocalVars lv) {
    LocalVars freshLV;
    smt::Expr freshSym;
    for (auto i = lv.begin(); i != lv.end(); i++) {
        freshSym = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(freshSym);
        freshLV[(*i).first] = freshSym;
    }
    return freshLV;
}

SEGState* SEGManager::merge(SEGState* a, SEGState* b) {
    SEGState* c = new SEGState(a->pc);
    c->LVi = constructFreshLV(a->LVi);

    // we need mu_a and mu_b to rename variables
    // note: the mua and mub here corresponds to mu_a^{-1} and mu_b^{-1} in the paper
    Substitution mua = a->computeMu(c->LVi);
    Substitution mub = b->computeMu(c->LVi);

    c->PT = Utils::intersection(Utils::substitute(mua, a->PT), Utils::substitute(mub, b->PT));
    c->ALi = Utils::intersection(Utils::substitute(mua, a->ALi), Utils::substitute(mub, b->ALi));

    ExprSet muaExFOa = Utils::substitute(mua, a->extendPredicate(this->solver));
    ExprSet mubFOb = Utils::substitute(mub, b->FO);
    for (auto i = muaExFOa.begin(); i != muaExFOa.end(); i++) {
        if (implies(mubFOb, *i))
            c->KB.insert(*i);
    }

    c->FO = c->getPredicate(this->solver);

    if (g_debug_info >= 3) {
        showMergeResult(a, b, c);
    }

    return c;
}

void SEGManager::constructSEG() {
    Path path;
    path.push_back(entry);
    errs() << "Constructing SEG...\n";
    expandSEG(path);
    errs() << "SEG Construction Complete!\n";
}

void SEGManager::expandSEG(Path path) {
    SEG::vertex_descriptor u, v, genTarget;
    std::list<SEG::vertex_descriptor> list;
    bool shouldGen;
    while(path.size() != 0) {
        v = path.back();
        if (!seg[v].visited) { // un-visited node
            // if v is an end node, go back directly
            if (seg[v].ptrState == NULL) {
                seg[v].visited = true;
                path.pop_back();
                continue;
            }
            // if v is not an end, decide whether we should generalize
            list = findSamePC(path, v);
            shouldGen = false;
            for (auto i = list.begin(); i != list.end(); i++) {
                if (seg[*i].ptrState->getDomain() == seg[v].ptrState->getDomain()
                        && hasInEvalEdge(v) && !hasInRefEdge(*i)) {
                    shouldGen = true;
                    genTarget = *i;
                    break;
                }
            }
            // Now we know what we should do
            if (shouldGen) { // we should generalize
                procGeneralization(v, genTarget, path);
            } else { // we should not generalize, we do evaluation
                procEvaluation(v, path);
            }
        // end of if (!seg[v].visited)
        } else { // v is visited
            list = firstUnvisitedChild(v);
            if (list.size() == 0) {
                path.pop_back();    // pop v out
            } else {
                u = list.back();
                path.push_back(u);
            }
        } // end of else (v is visited)
    } // end of while
}

std::list<SEG::vertex_descriptor> SEGManager::findSamePC(Path path, SEG::vertex_descriptor v) {
    std::list<SEG::vertex_descriptor> l;
    if (seg[v].ptrState == NULL)
        return l;
    path.pop_back();    // first pop v out
    for (auto i = path.rbegin(); i != path.rend(); i++) {
        if (std::equal_to<ceagle::ProgramCounter>{}(seg[*i].ptrState->pc, seg[v].ptrState->pc)) {
            l.push_back(*i);
        }
    }
    return l;
}

bool SEGManager::hasInEvalEdge(SEG::vertex_descriptor v) {
    SEG::in_edge_iterator i, e;
    tie(i, e) = in_edges(v, seg);
    for ( ; i != e; i++) {
        if (seg[*i].type == EdgeType::evaluation) {
            return true;
        }
    }
    return false;
}

bool SEGManager::hasInRefEdge(SEG::vertex_descriptor v) {
    SEG::in_edge_iterator i, e;
    tie(i, e) = in_edges(v, seg);
    for ( ; i != e; i++) {
        if (seg[*i].type == EdgeType::refinement) {
            return true;
        }
    }
    return false;
}

std::list<SEG::vertex_descriptor> SEGManager::firstUnvisitedChild(SEG::vertex_descriptor v) {
    std::list<SEG::vertex_descriptor> l;
    SEG::adjacency_iterator i, e;
    tie(i, e) = adjacent_vertices(v, seg);
    for ( ; i != e; i++) {
        if (!seg[*i].visited) {
            l.push_back(*i);
            break;
        }
    }
    return l;
}

void SEGManager::procGeneralization(SEG::vertex_descriptor v,
        SEG::vertex_descriptor genTarget, Path& path) {
    if (seg[genTarget].ptrState->isGeneralizationOf(seg[v].ptrState,
            this->solver)) {
        seg[v].visited = true;
        add_edge(v, genTarget, {EdgeType::generalization}, seg);
        path.pop_back();    // pop v out
    } else {    // merge
        while (!(path.back() == genTarget)) {
            path.pop_back();
        }
        clear_out_edges(genTarget, seg);
        SEGState* c = merge(seg[genTarget].ptrState, seg[v].ptrState);

        SEG::vertex_descriptor vc = add_vertex(c, seg);

        if (path.size() <= 1) {
            add_edge(genTarget, vc, {EdgeType::generalization}, seg);
            path.push_back(vc);
        } else {
            path.pop_back();    // pop genTarget out
            SEG::vertex_descriptor previous = path.back();  // previous vertex in path
            if (seg[edge(previous, genTarget, seg).first].type
                    == EdgeType::generalization) {
                remove_edge(previous, genTarget, seg);
                add_edge(previous, vc, {EdgeType::generalization}, seg);
                path.push_back(vc);
            } else {    // do not delete genTarget
                path.push_back(genTarget);
                add_edge(genTarget, vc, {EdgeType::generalization}, seg);
                path.push_back(vc);
            }
        }
    }
}

void SEGManager::procEvaluation(SEG::vertex_descriptor v, Path& path) {
    seg[v].visited = true;
    std::list<SEGState*> sl = evaluate(seg[v].ptrState);
    if (sl.size() == 0) {
        errs() << "procEvaluation Unknown!!!\n";
        errs() << "Unsupported instruction: " << *(seg[v].ptrState->pc.i) << "\n";
        exit(-1);
    } else if (sl.size() == 1) {
        SEGState* w = sl.back();
        SEG::vertex_descriptor vw = add_vertex(w, seg);
        add_edge(v, vw, {EdgeType::evaluation}, seg);
        path.push_back(vw);
    } else {    // size == 2
        SEGState* w1 = sl.front();
        SEGState* w2 = sl.back();

        SEG::vertex_descriptor vw1 = add_vertex(w1, seg);
        add_edge(v, vw1, {EdgeType::refinement}, seg);

        SEG::vertex_descriptor vw2 = add_vertex(w2, seg);
        add_edge(v, vw2, {EdgeType::refinement}, seg);

        path.push_back(vw1);
    }
}

SEG::vertex_descriptor SEGManager::add_vertex(SEGState* s, SEG& g) {
    SEG::vertex_descriptor v = boost::add_vertex(g);
    g[v].ptrState = s;
    g[v].visited = false;
    g[v].label = "s" + to_string(stateNum++);
    return v;
}

void SEGManager::simplifySEG() {
    errs() << "Simplifying SEG...\n";

    std::map<SEG::vertex_descriptor, bool> visited;
    std::map<SEG::vertex_descriptor, SEG::vertex_descriptor> mapV;
    SEG::vertex_iterator i, e;
    tie(i, e) = vertices(seg);
    for ( ; i != e; i++) {
        visited[*i] = false;
    }
    SEG dest;
    SEG::vertex_descriptor destEntry = boost::add_vertex(seg[entry], dest);
    mapV[entry] = destEntry;
    dfs(entry, dest, destEntry, visited, mapV);
    this->seg = dest;
    this->entry = destEntry;

    errs() << "SEG Simplification Complete!\n";
}

void SEGManager::dfs(SEG::vertex_descriptor srcCur, SEG& dest, SEG::vertex_descriptor destCur,
        std::map<SEG::vertex_descriptor, bool>& visited,
        std::map<SEG::vertex_descriptor, SEG::vertex_descriptor>& mapV) {
    if (visited[srcCur]) {
        return;
    } else {
        visited[srcCur] = true;
        SEG::adjacency_iterator i, e;
        SEG::vertex_descriptor destChild;
        tie(i, e) = adjacent_vertices(srcCur, seg);
        for ( ; i != e; i++) {
            if (visited[*i]) {
                destChild = mapV[*i];
            } else {
                destChild = boost::add_vertex(seg[*i], dest);
                mapV[*i] = destChild;
            }
            add_edge(destCur, destChild, seg[edge(srcCur, *i, seg).first], dest);
            dfs(*i, dest, destChild, visited, mapV);
        }
    }
}

void SEGManager::showEvalResult(SEGState* v, SEGState* w) {
    errs() << "*========== State Before Evaluation =========*\n";
    v->show();
    errs() << "*========== State After Evaluation ==========*\n";
    if (w == NULL) {
        errs() << "*---------- NULL ----------*\n";
    } else {
        w->show();
    }
    errs() << "*============================================*\n\n";
}

void SEGManager::showRefResult(SEGState* v, SEGState* w1, SEGState* w2) {
    errs() << "*========== State Before Refinement =========*\n";
    v->show();
    errs() << "*========== States After Refinement =========*\n";
    errs() << "*---------- State 1 ---------*\n";
    w1->show();
    errs() << "*---------- State 2 ---------*\n";
    w2->show();
    errs() << "*============================*\n\n";
}

void SEGManager::showMergeResult(SEGState* a, SEGState* b, SEGState* c) {
    errs() << "*========== State 1 Before Merging =========*\n";
    a->show();
    errs() << "*========== State 2 Before Merging =========*\n";
    b->show();
    errs() << "*========== Merging State =========*\n";
    c->show();
    errs() << "*==================================*\n\n";
}

llvm::BasicBlock::iterator SEGManager::firstNonPhi(llvm::BasicBlock* b) {
    BasicBlock::iterator i = b->begin();
    while (dyn_cast<PHINode>(i)) {
        //errs() << "PHINode: " << *i << "\n";
        i++;
    }
    return i;
}

std::pair<LocalVars, ExprSet> SEGManager::computePhi(SEGState* state,
        llvm::BasicBlock* curBlk, llvm::BasicBlock* nextBlk) {
    LocalVars effectPhi;
    ExprSet kbPhi;
    BasicBlock::iterator i = nextBlk->begin();
    PHINode* pi;
    smt::Expr w;
    smt::Expr expr;

    while (pi = dyn_cast<PHINode>(i)) {
        w = smt::Var("v" + to_string(varCnt++), smt::IntSort());
        symVars.insert(w);

        effectPhi[pi] = w;
        expr = (w == state->getLVi(pi->getIncomingValueForBlock(curBlk)));
        kbPhi.insert(expr);

        //errs() << "compute PHINode: " << *i << "\n";
        i++;
    }

    LocalVars lvResult = LocalVars(state->LVi);
    for (auto j = effectPhi.begin(); j != effectPhi.end(); j++) {
        lvResult[(*j).first] = (*j).second;
    }

    return make_pair(lvResult, kbPhi);
}
}


