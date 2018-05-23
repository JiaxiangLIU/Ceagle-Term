// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"

#include "Callgraph.h"
#include "Modifies.h"
#include "PointsTo.h"
#include "VariableSlicer.h"
#include "Matcher.h"

using namespace llvm;

static cl::opt<std::string>
FileName("dexi-criterion-file", cl::desc("File name of slicing criterion"));

static cl::opt<int>
LineNumber("dexi-criterion-line", cl::desc("Line number of slicing criterion"));

static cl::opt<std::string>
SlicingVariable("variable", cl::desc("Variable name of slicing criterion"));

static cl::opt<std::string>
SlicingFunction("dexi-criterion-function", cl::desc("Function name of slicing criterion"));

static cl::opt<bool>
OutputLine("dexi-output-line",
        cl::desc("Instead of output the slice as instructions, output the line numbers."));

static cl::opt<bool>
ApplyForward("dexi-forward-slice",
        cl::desc("Apply forward-slice (default is backward-slice)."));

namespace llvm { namespace slicing { namespace detail {

}}}

namespace llvm { namespace slicing {
    typedef SmallVector<const Function *, 20> WorkList;

    static bool setAdd(WorkList &list, const Function * elem)
    {
        bool included = false;
        for (WorkList::iterator WI = list.begin(), WE = list.end(); WI != WE; ++WI) {
            if (*WI == elem) {
                included = true;
                break;
            }
        }
        if (!included) {
            list.push_back(elem);
        }
        return !included;
    }

    void VariableSlicer::buildDicts(const ptr::PointsToSets &PS)
    {
        typedef Module::iterator FunctionsIter;
        for (FunctionsIter f = module.begin(); f != module.end(); ++f)
            if (!f->isDeclaration() && !memoryManStuff(&*f))
                for (inst_iterator i = inst_begin(*f);
                        i != inst_end(*f); i++)
                    if (CallInst const* c =
                            dyn_cast<CallInst const>(&*i)) {
                        if (isInlineAssembly(c)) {
                            errs() << "ERROR: Inline assembler detected in " <<
                                f->getName() << ", skipping\n";
                            continue;
                        }
                        typedef std::vector<const Function *> FunCon;
                        FunCon G;
                        // A CallInst may have multiple possible call target due to
                        // function pointer
                        getCalledFunctions(c, PS, std::back_inserter(G));

                        for (FunCon::const_iterator g = G.begin();
                                g != G.end(); ++g) {
                            Function const* const h = *g;
                            if (!memoryManStuff(h) && !h->isDeclaration()) {
                                funcsToCalls.insert(std::make_pair(h, c));
                                callsToFuncs.insert(std::make_pair(c, h));
                            }
                        }
                    }
    }

    VariableSlicer::VariableSlicer(ModulePass *MP, Module &M,
            const ptr::PointsToSets &PS,
            const callgraph::Callgraph &CG,
            const mods::Modifies &MOD) : mp(MP), module(M), slicers(), initFuns(), 
    funcsToCalls(), callsToFuncs(), ps(PS), cg(CG), mod(MOD), matcher(M) {
        //parseCriteria();
        // DW: use the new sliceVariable function
        sliceVariable();
        buildDicts(ps);
    }

    // DW: new slicing function
    // DW: options: FileName, LineNumber, SlicingVariable, SlicingFunction
    void VariableSlicer::sliceVariable() {
        // DW: gets the current function
        for (Module::iterator f = module.begin(); f != module.end(); ++f) {
            // ----
            Function *F = &*f;
            // ----
            //Function *F = module.getFunction("main");
            if (F == NULL) {
                errs() << "VariableSlicer: no matching function at " << FileName << ":" << FileName << " found\n";
                return;
            }
            errs() << "Matching function: " << F->getName() << "\n";
            FunctionStaticSlicer *FSS = new FunctionStaticSlicer(*F, mp, ps, mod, ApplyForward);
            //Instruction *inst;
            //inst_iterator ii = inst_begin(F);
            bool found = false;
            errs() << "Matching instruction: \n";
            // DW: find the line number here
            int count= LineNumber;

            for (inst_iterator I = inst_begin(*F), E = inst_end(*F); I != E; --E, --count) {
                // DW: the inst_end is always NULL, skip it
                if (E == inst_end(*F)) {
                    continue;
                }
                //const Instruction *inst = &*E;
                Instruction *inst = &*E;
                if (inst != NULL) {
                    //errs() << "VariableSlicer: found instruction " << inst << ":" << inst->getName() << "\n";
                    //errs() << "VariableSlicer: found instruction " << inst << "\n";
                } else {
                    //errs() << "VariableSlicer: null inst, continue\n";
                    continue;
                }

                //InsInfo * insInfo = new InsInfo(inst, ps, mod);
                const Value *LHS = NULL;
                if (const LoadInst *LI = dyn_cast<LoadInst>(inst)) {
                    LHS = LI->getPointerOperand();
                } else if (const StoreInst * SI = dyn_cast<StoreInst>(inst)) {
                    LHS = SI->getPointerOperand();
                }
                if (LHS && LHS->hasName()) {
                    //errs() << "VariableSlicer: checking variable " << LHS->getName() << "\n";
                }
                if (LHS && LHS->hasName() && LHS->getName().equals_lower(SlicingVariable)) {
                    errs() << "VariableSlicer: found variable " << LHS->getName() << "\n";
                    inst->dump();
                    FSS->addInitialCriterion(inst, LHS);
                    found = true;
                    // DW: if found, break
                    break;
                }
            }

            if (!found) {
                errs() << "VariableSlicer: no matching instruction for variable " << SlicingVariable << "\n";
                continue;
            }
            initFuns.push_back(F);
            slicers.insert(Slicers::value_type(F, FSS));
        }
    }

    // DW: options: FileName, LineNumber, SlicingVariable, SlicingFunction
    void VariableSlicer::parseCriteria() {
        // DW: gets the current function
        // ----
        Function *F = module.getFunction(SlicingFunction.c_str());
        // ----
        //Function *F = module.getFunction("main");
        if (F == NULL) {
            errs() << "No matching function at " << FileName << ":" << LineNumber << " found\n";
            return;
        }
        errs() << "Matching function: " << F->getName() << "\n";
        FunctionStaticSlicer *FSS = new FunctionStaticSlicer(*F, mp, ps, mod, ApplyForward);
        //Instruction *inst;
        //inst_iterator ii = inst_begin(F);
        bool found = false;
        errs() << "Matching instruction: \n";
        // DW: find the line number here
        int count= 0;

        for (inst_iterator I = inst_begin(*F), E = inst_end(*F); I != E; ++I, ++count) {
            const Instruction *inst = &*I;

            // while ((inst = matcher.matchInstruction(ii, F, scope)) != NULL) {
            if (count == 1) {
                //InsInfo * insInfo = new InsInfo(inst, ps, mod);
                const Value *LHS = NULL;
                if (const LoadInst *LI = dyn_cast<LoadInst>(inst)) {
                    LHS = LI->getPointerOperand();
                } else if (const StoreInst * SI = dyn_cast<StoreInst>(inst)) {
                    LHS = SI->getPointerOperand();
                }
                if (LHS && LHS->hasName() && LHS->getName().equals_lower(SlicingVariable)) {
                    inst->dump();
                    FSS->addInitialCriterion(inst, LHS);
                    found = true;
                }
            }
        }

        if (!found) {
            errs() << "No matching instruction for variable " << SlicingVariable << "\n";
            return;
        }
        initFuns.push_back(F);
        slicers.insert(Slicers::value_type(F, FSS));
    }


    VariableSlicer::~VariableSlicer() {
        for (Slicers::const_iterator I = slicers.begin(), E = slicers.end();
                I != E; ++I)
            delete I->second;
    }

    FunctionStaticSlicer * VariableSlicer::getFSS(const Function * F) {
        Slicers::iterator si;
        si = slicers.find(F);
        if (si == slicers.end()) {
            Function *f = const_cast<Function *>(F);
            FunctionStaticSlicer *FSS = new FunctionStaticSlicer(*f, mp, ps, mod, ApplyForward);
            slicers.insert(Slicers::value_type(F, FSS));
            return FSS;
        }
        return si->second;
    }

    void VariableSlicer::computeSlice() {
        WorkList Q(initFuns);
        WorkList P;
        WorkList ALL;

        // Backward:  DOWN*(UP*({C}))
        // Forward:   UP*(DOWN*({C}))

        // Phase 1
        //   Backward:  UP*({C})
        //   Forward:   DOWN*({C})

        while (!Q.empty()) {
            for (WorkList::iterator f = Q.begin(); f != Q.end(); ++f) {
                FunctionStaticSlicer *fss = getFSS(*f);
                fss->calculateStaticSlice();
                // fss->dump(matcher, OutputLine);
                if (setAdd(P, *f)) {
                    ALL.push_back(*f);
                }
            }
            WorkList tmp;
            for (WorkList::iterator f = Q.begin(); f != Q.end(); ++f) {
                if (!ApplyForward)
                    emitToCalls(*f, std::inserter(tmp, tmp.end()));
                else
                    emitToForwardExits(*f, std::inserter(tmp, tmp.end()));
            }
            std::swap(tmp,Q);
        }

        // Phase 2
        //     Backward: DOWN*(XXX)
        //     Forward:  UP*(XXX)
        while (!P.empty()) {
            for (WorkList::iterator f = P.begin(); f != P.end(); ++f) {
                FunctionStaticSlicer *fss = getFSS(*f);
                fss->calculateStaticSlice();
                // fss->dump(matcher, OutputLine);
                setAdd(ALL, *f);
            }
            WorkList tmp;
            for (WorkList::iterator f = P.begin(); f != P.end(); ++f) {
                if (!ApplyForward)
                    emitToExits(*f, std::inserter(tmp, tmp.end()));
                else
                    emitToForwardCalls(*f, std::inserter(tmp, tmp.end()));
            }
            std::swap(tmp,P);
        }

        for (WorkList::iterator f = ALL.begin(); f != ALL.end(); ++f) {
            FunctionStaticSlicer *fss = getFSS(*f);
            fss->dump(matcher, OutputLine);
        }
    }

    bool VariableSlicer::sliceModule() {
        bool modified = false;
        errs() << "VariableSlicer::sliceModule slicers " << slicers.size() << "\n";
        for (Slicers::iterator s = slicers.begin(); s != slicers.end(); ++s)
            modified |= s->second->slice();
        if (modified)
            for (Module::iterator I = module.begin(), E = module.end(); I != E; ++I)
                if (!I->isDeclaration())
                    FunctionStaticSlicer::removeUndefs(mp, *I);
        return modified;
    }
}
}

namespace {
    class Slicer : public ModulePass {
        public:
            static char ID;

            Slicer() : ModulePass(ID) {}

            virtual bool runOnModule(Module &M);

            void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.addRequired<PostDominatorTree>();
                AU.addRequired<PostDominanceFrontier>();
            }
    };
}

static RegisterPass<Slicer> X("slice-variable", "Slices the variable in all functions");
char Slicer::ID;

bool Slicer::runOnModule(Module &M) {
    ptr::PointsToSets PS;
    {
        ptr::ProgramStructure P(M);
        computePointsToSets(P, PS);
    }

    callgraph::Callgraph CG(M, PS);

    mods::Modifies MOD;
    {
        mods::ProgramStructure P1(M);
        computeModifies(P1, CG, PS, MOD);
    }

    slicing::VariableSlicer SS(this, M, PS, CG, MOD);
    //SS.computeSlice();
    //return false;
    return SS.sliceModule();
}

