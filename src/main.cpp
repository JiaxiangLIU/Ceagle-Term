/*
 * main.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: jiaxiang
 */

// wait support
#ifndef DARWIN
#include <sys/wait.h>
#endif
// options support
#include <unistd.h>
#include <getopt.h>

// C++ headers
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <cstdlib>

// LLVM headers
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/SourceMgr.h>

// internal headers
#include "Property/PropertyManager.h"
#include "Context/ContextManager.h"
#include "Runner/RunnerManager.h"
#include "DFS/DFS.h"
#include "DFS/CEGAR.h"
#include "DFS/GenericDFS.h"
#include "DFS/IRBlockDFS.h"
#include "DFS/Advisor/Advisor.h"
#include "DFS/Advisor/ArraysBoundsAdvisor.h"
#include "ValueAnalysis/ValueAnalysis.h"

// preprocessors
#include "Preprocess/InlinePreprocessor.h"
#include "Preprocess/SVCompInterfacePreprocessor.h"
#include "Preprocess/NamingPreprocessor.h"
#include "Preprocess/PhiPreprocessor.h"
#include "Preprocess/TrimPreprocessor.h"
#include "Preprocess/GenerateFunctionCallPreprocessor.h"
#include "Preprocess/SlicingFunctionPreprocessor.h"

// structural abstraction
#include "StructuralAbstraction/SAManager.h"

// overflow checker
#include "Overflow/overflow.h"

// cde
#include "CDE/CDE.h"

#include "Witness/Witness.h"
#include "Witness/ViolationWitness.h"
#include "Witness/CorrectnessWitness.h"

// CMake headers
#include "Config.h"

// For debug macro
#include "debug.h"
#include "macros.h"

// Ultilities
#include "Util/StringUtil.h"

#include "Termination/SEGManager.h"
#include "Termination/SEGState.h"
#include "Termination/Utils.h"

// used in CLI option
#define ADVISOR_NAME_NAIVE "naive"
#define ADVISOR_NAME_STRUCTURAL "structural"
#define ADVISOR_NAME_TEST_STRUCTURAL "test_structural"
#define ADVISOR_NAME_ABSREF "absref"
#define ADVISOR_NAME_VALUE "value-analysis"
#define DEFAULT_ADVISOR_NAME ADVISOR_NAME_NAIVE

#define TODO_NEED_USE_CPU 0

// need context or not
bool g_need_context = true;

static std::string g_advisor_name = "";
std::string g_compiler_name = "/Users/jiaxiang/llvm-3.7.0.src/build/bin/clang";
//std::string g_compiler_name = "clang";
std::string g_property_file_name = "";
bool g_trim_unreachable = false;
bool g_dump_module = false;
bool g_dry_run = false;
std::string g_generate_function_call_of = "";
bool g_phi_eliminate = false;
bool g_option_slice_function = false;
std::string g_slice_function = "";
bool g_check_overflow = false;
std::string g_context_test = "";
std::string g_mem = "64";
extern std::string g_z3_path;

// for debugging termination, by jiaxiang
extern int g_debug_info;

clock_t g_ceagle_start = clock();

using namespace ceagle;
using namespace ceagle::cde;
using namespace llvm;

int main(int argc, char* const* argv) {

    /*
    smt::Expr v[8];
    for (int i = 0; i < 6; i++) {
        v[i] = smt::Var("v" + to_string(i), smt::IntSort());
    }
    ExprSet es;
    //es.insert(smt::Var("v", smt::IntSort()) == smt::Var("v", smt::IntSort()));
    //es.insert(smt::Var("v", smt::IntSort()) >= smt::Var("v", smt::IntSort()));
    for (int i = 0; i < 6; i=i+2) {
        //es.insert((v[i]==v[i]) || (v[i+1]==v[i+1]));
    }
    for (auto i = es.begin(); i != es.end(); i++) {
        std::cout << (*i).str() << std::endl;
    }

    std::list<ExprSet> result;
    result = Utils::elimOr(es);

    for (auto i = result.begin(); i != result.end(); i++) {
        std::cout << "Set:" << std::endl;
        for (auto j = (*i).begin(); j != (*i).end(); j++) {
            std::cout << "  " << (*j).str() << std::endl;
        }
    }
    exit(0); */

    if (argc <= 1) {
        std::cout << "Please specify a file to verify." << std::endl;
        exit(0);
    }

    auto inputfile = argv[1];

    // check if file exists
    std::ifstream test_exist(inputfile);
    if (!test_exist.good()) {
        std::cout << "Input file " << inputfile << " does not exist" << std::endl;
        exit(0);
    }



    // Clang compile
    std::string fn = inputfile;
    std::cout << "Verifying " << fn << " ..." << std::endl;

    std::string fnext = fn.substr(fn.find_last_of(".") + 1);
    std::string fnll = fn + ".ll";
    if ((fnext == "i") || (fnext == "c")) {
        auto pid = fork();
        if (pid < 0) {
            std::cout << "main cannot run Clang" << std::endl;
            exit(-1);
        } else if (pid == 0) {
            char * const compiler_args[8] = {"-g", "-S", "-emit-llvm", "-w", const_cast<char*>(fn.c_str()), "-o", const_cast<char*>(fnll.c_str())};
            exit(execvp(g_compiler_name.c_str(), compiler_args));
        } else {
            int status;
            while (wait(&status) != pid) ;
        }
    } else if (fnext == "ll") {
        fnll = fn;
    } else {
        std::cout << "Input file type \"" << fnext << "\" unsupported!" << std::endl;
        exit(0);
    }

    // Verification begins HERE

    // read the ll file
    // begin
    LLVMContext &context = getGlobalContext();
    SMDiagnostic err;
    std::unique_ptr<llvm::Module> up_mod = parseIRFile(fnll.c_str(), err, context);
    if (up_mod == NULL) {
        std::cout << "Corrupted IR file or IR file not found" << std::endl;
        exit(-1);
    }
    llvm::Module* module = up_mod.get();
    //std::cout << ";===== Original Module =====\n";
    //module->dump();
    // end

    // initialize the SEG manager
    SEGManager sm = SEGManager(module);

    /*
    SEGState* p1 = sm.evaluate(sm.seg[sm.entry].ptrState).front(); // alloca
    SEGState* p2 = sm.evaluate(p1).front(); // store
    SEGState* p3 = sm.evaluate(p2).front(); // br
    SEGState* p4 = sm.evaluate(p3).front(); // load
    SEGState* p5 = sm.evaluate(p4).front(); // icmp slt
    SEGState* p6 = sm.evaluate(p5).front(); // br i1 cmp
    SEGState* p7 = sm.evaluate(p6).front(); // load
    SEGState* p8 = sm.evaluate(p7).front(); // add
    SEGState* p9 = sm.evaluate(p8).front(); // store
    SEGState* p10 = sm.evaluate(p9).front(); // br
    SEGState* p11 = sm.evaluate(p10).front(); // load
    std::list<SEGState*> sl = sm.evaluate(p11); // icmp slt, need to refine
    SEGState* p12 = sl.front(); // state 1 from icmp slt
    SEGState* p13 = sl.back(); // state 2 from icmp slt

    // first branch from state 1
    SEGState* p14 = sm.evaluate(p12).front(); // icmp slt
    SEGState* p15 = sm.evaluate(p14).front(); // br i1 cmp
    SEGState* p16 = sm.evaluate(p15).front(); // load
    SEGState* p17 = sm.evaluate(p16).front(); // add
    // p17 is a store instuction, it should do a generalization with p8

    bool yes = p8->isGeneralizationOf(p17, sm.solver);
    errs() << "p8 " << (yes ? "is" : "is not") << " a generalization of p17\n";

    bool yes2 = p17->isGeneralizationOf(p8, sm.solver);
    errs() << "p17 " << (yes2 ? "is" : "is not") << " a generalization of p8\n";

    SEGState* p18 = sm.merge(p8, p17);
    errs() << "p18 " << (p18->isGeneralizationOf(p8, sm.solver) ? "is" : "is not") << " a generalization of p8\n";
    errs() << "p18 " << (p18->isGeneralizationOf(p17, sm.solver) ? "is" : "is not") << " a generalization of p17\n";

    SEGState* p19 = sm.evaluate(p18).front(); // store
    SEGState* p20 = sm.evaluate(p19).front(); // br
    SEGState* p21 = sm.evaluate(p20).front(); // load
    sl = sm.evaluate(p21); // icmp slt, need to refine
    SEGState* p22 = sl.front(); // state 1 from icmp slt
    SEGState* p23 = sl.back(); // state 2 from icmp slt

    // first branch from state 1
    SEGState* p24 = sm.evaluate(p22).front(); // icmp slt
    SEGState* p25 = sm.evaluate(p24).front(); // br i1 cmp
    SEGState* p26 = sm.evaluate(p25).front(); // load
    SEGState* p27 = sm.evaluate(p26).front(); // add
    // p27 is again a store instruction, it should do a generalization with p18
    // note: not p8, since p18 generalizes p8, we should use a more general state in higher priority
    yes = p18->isGeneralizationOf(p27, sm.solver);
    errs() << "p18 " << (yes ? "is" : "is not") << " a generalization of p27\n";

    // second branch from state 2
    SEGState* p28 = sm.evaluate(p23).front(); // icmp slt
    SEGState* p29 = sm.evaluate(p28).front(); // br i1 cmp
    SEGState* p30 = sm.evaluate(p29).front(); // ret
    errs() << "p30 " << (p30 == NULL ? "is" : "is not") << " NULL\n";

*/

    sm.constructSEG();
    if (g_debug_info >= 2) {
        sm.show();
        errs() << "num of seg:" << num_vertices(sm.getSEG()) << "\n";
    }
    if (g_debug_info > 0) {
        sm.toDOT(fnll + ".seg.origin");
    }

    sm.simplifySEG();
    if (g_debug_info >= 2) {
        errs() << "simplified version:\n";
        sm.show();
        errs() << "num of simplified seg: " << num_vertices(sm.getSEG()) << "\n";
    }

    if (g_debug_info >= 2) {
        errs() << "num of vars: " << sm.getVarCnt() << "\n";
    }

    if (g_debug_info > 0) {
        sm.toDOT(fnll + ".seg");
    }

    sm.toKITTEL(fnll);

    return 0;

}


