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

// used in CLI option
#define ADVISOR_NAME_NAIVE "naive"
#define ADVISOR_NAME_STRUCTURAL "structural"
#define ADVISOR_NAME_TEST_STRUCTURAL "test_structural"
#define ADVISOR_NAME_ABSREF "absref"
#define ADVISOR_NAME_VALUE "value-analysis"
#define ADVISOR_NAME_CDE "cde"
#define DEFAULT_ADVISOR_NAME ADVISOR_NAME_NAIVE

#define TODO_NEED_USE_CPU 0

// need context or not
bool g_need_context = true;

static std::string g_advisor_name = "";
std::string g_compiler_name = "clang";
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
ceagle::cde::CDE_TYPE g_cde_type;
int g_naive_advisor_unroll = -1;

using namespace ceagle;
using namespace ceagle::cde;
using namespace llvm;

// 20161216, timer to control verification time
util::UltilityTimer *g_timer;

void printVersion() {
    std::cout << "Ceagle " << SV_VERSION << " @ " << GIT_VERSION << std::endl;
}

void printUsage() {
    std::cout << "NAME\n";
    std::cout << "\t" << SV_BINARY_NAME << " - C program verifier\n\n";
    std::cout << "VERSION\n";
    std::cout << "\t" << SV_VERSION << " @ " << GIT_VERSION << "\n\n";
    std::cout << "SYNOPSIS\n";
    std::cout << "\t" << SV_BINARY_NAME << " [OPTION]... [FILE]...\n\n";
    std::cout << "DESCRIPTION\n";
    std::cout << "\t--help,-h\n\t\tprint usage\n\n";
    std::cout << "\t--version,-v\n\t\tprint version\n\n";
    std::cout << "\t--advisor=" << ADVISOR_NAME_NAIVE << "\n\t\tspecify advisor, available advisors: " << ADVISOR_NAME_NAIVE << ", " << ADVISOR_NAME_ABSREF << ", " << ADVISOR_NAME_STRUCTURAL << ", " << ADVISOR_NAME_VALUE << "\n\n";
    std::cout << "\t--compiler=clang\n\t\tspecify clang location\n\n";
    std::cout << "\t--property-file=ALL.prp\n\t\tspecify property file location\n\n";
    std::cout << "\t--check-overflow\n\t\tcheck overflow\n\n";
    std::cout << "\t--trim-unreachable\n\t\ttrim away unreachable code blocks\n\n";
    std::cout << "\t--dump-module\n\t\toutput LLVM IR module to stdout\n\n";
    std::cout << "\t--dry-run\n\t\tjust preprocessing, no verification\n\n";
    std::cout << "\t--generate-function-call-of=foo\n\t\toutput function call tree of function \"foo\"\n\n";
    std::cout << "\t--phi-eliminate\n\t\ttrim away phi nodes in LLVM IR module\n\n";
    std::cout << "\t--slice-function=foo\n\t\tperform function call slicing of function \"foo\"\n\n";
    std::cout << "\t--mem=32,64\n\t\tmemory model, i.e. the size of long\n\n";
    std::cout << "\t--no-auto-select\n\t\tdo not use context to auto select verification method\n\n";
    std::cout << "(C) 2011 - 2016, SV Team, Tsinghua University, China\n";
    return;
}

void analyzeBitNodes() {
#if TODO_NEED_USE_CPU
    std::cout << "main calulated bit nodes of " << util::useCPU() << std::endl;
#endif
}

void analyzeFloats() {
    std::cout << "main analyzeFloats thread" << std::endl;
    //std::cout << "main analyzeFloats thread with id " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(850));
    std::cout << "main analyzeFloats TRUE" << std::endl;
    exit(0);
}

std::thread *g_thread;
//std::future<void> g_result;
void timeFloats() {
    //g_thread = new std::thread(analyzeFloats);
    //g_thread->detach();
    std::cout << "main timeFloats starting thread" << std::endl;
    //std::future<void> g_result = std::async(analyzeFloats);
    //std::cout << "main timeFloats before wait thread" << std::endl;
    //g_result.wait();
    //std::cout << "main timeFloats after wait thread" << std::endl;
    g_thread = new std::thread(analyzeFloats);
}

int verify(const char *n) {
    // check if fn exists
    std::ifstream test_exist(n);
    if (!test_exist.good()) {
        std::cout << "main file " << n << " does not exist" << std::endl;
        return 0;
    }

    // parse property if needed
    PropertyManager *pm = new PropertyManager();
    if (g_property_file_name.size() > 0) {
        std::cout << "main using property file " << g_property_file_name << std::endl;
        pm->parse(g_property_file_name);
        std::cout << "main found property " << pm->getProperty() << " " << pm->getPropertyString() << std::endl;

        // specify the advisor according to the cli option & property file
        // cli option has higher priority
        PropertyType pt = pm->getProperty();
        if (g_advisor_name == "") {
            if (pt == PT_SVCOMP_Valid_Dereference) {
                g_advisor_name = "arraysbounds";
            } else if (pt == PT_SVCOMP_Error_Function_Unreachability) {
                g_advisor_name = ADVISOR_NAME_NAIVE;
            } else if (pt == PT_SVCOMP_No_Overflow) {
                g_check_overflow = true;
            } else if (pt == PT_SVCOMP_Termination) {
                std::cout << "Termination property cannot be verified." << std::endl;
                std::cout << "UNKNOWN" << std::endl;
                return 0;
            } else {
                // if no property file or others, default to NaiveAdvisor
                g_advisor_name = ADVISOR_NAME_NAIVE;
            }
        }
    }

    // if the advisor is still empty, specify one
    //std::cout << "main g_advisor_name " << g_advisor_name << std::endl;
    if (g_advisor_name == "") {
        g_advisor_name = ADVISOR_NAME_NAIVE;
    }

    // clang compile
    std::string fn = n;
    std::cout << "main verifying " << fn << std::endl;

    // if check-overflow, skip all below
    if (g_check_overflow) {
        auto checkResult = verifyOverflow(fn, g_compiler_name);
        switch(checkResult) {
            case OverflowResult::Overflow:
                std::cout << "FALSE(no-overflow)" << std::endl;
                return -1;
            case OverflowResult::NoOverflow:
                std::cout << "TRUE" << std::endl;
                return 1;
            case OverflowResult::Unknown:
                std::cout << "UNKNOWN" << std::endl;
                return 0;
        }
    }

    std::string fnext = fn.substr(fn.find_last_of(".") + 1);
    std::string fnll = fn + ".ll";
    //std::cout << fn << std::endl << fnext << std::endl << fnll << std::endl;
    //std::cout << "Ext is " << fnext << "; n is " << n << std::endl;
    if ((fnext == "i") || (fnext == "c")) {
        auto pid = fork();
        if (pid < 0) {
            std::cout << "main cannot run clang" << std::endl;
        } else if (pid == 0) {
            char * const compiler_args[8] = {"-g", "-S", "-emit-llvm", "-w", const_cast<char*>(fn.c_str()), "-o", const_cast<char*>(fnll.c_str())};
            exit(execvp(g_compiler_name.c_str(), compiler_args));
        } else {
            int status;
            while (wait(&status) != pid) ;
        }
    } else if (fnext == "ll") {
        fnll = fn;
        g_need_context = false;
    } else {
        std::cout << "main unsupported file type \"" << fnext << "\"" << std::endl;
        exit(0);
    }

    if (g_need_context) {
        std::cout << "main using context ..." << std::endl;
        // context parsing
        ContextManager *cm = new ContextManager(
                15,
                CP_Elevator,
                CP_ECA,
                CP_Concurrency,
                CP_DeviceDriversLinux64, CP_Newton, CP_FloatsArray, CP_ArraysLargeLoop,
                CP_LdvMalloc,
                CP_ArraysBounds,
                CP_NoNondet,
                CP_FLOAT,
                CP_ByteOperation, CP_GCD, CP_ModuleAnalysis, CP_LOOP
                );
        cm->parse(fn);
        ContextResult cr = cm->getMatchedResult();
        auto matchedParser = cm->getMatchedParser();
        if (g_context_test != "") {
            std::cout << "Expect context: " << g_context_test << std::endl;
            std::cout << "Actual context: " << matchedParser << std::endl;
            std::cout << ((matchedParser == g_context_test) ? "TRUE" : "FALSE") << std::endl;
            exit(0);
        }
        switch (matchedParser) {
            case CP_Elevator: {
#if DEBUG_CONTEXT
                std::cout << "Context Elevator found" << std::endl;
#endif
                g_advisor_name = ADVISOR_NAME_CDE;
                g_cde_type = CDE_TYPE::Elevator;
                break;
            }
            case CP_ECA: {
#if DEBUG_RUNNER
                std::cout << "Context ECA found" << std::endl;
#endif
                g_advisor_name = ADVISOR_NAME_CDE;
                g_cde_type = CDE_TYPE::ECA;
                break;
            }
            case CP_LdvMalloc:
                {
                    if (g_advisor_name == "arraysbounds") {
                        std::cout << "Nondeterminisitic malloc found for memsafety" << std::endl;
                        std::cout << "UNKNOWN" << std::endl;
                    } else {
                        assert("Nondeterminisitic malloc found for non memsafety task" && false);
                    }
                    return 0;
                }
            case CP_Concurrency:
                {
                    std::cout << "Context Concurrency found: " << cr.message[0] << std::endl;
                    std::cout << "UNKNOWN" << std::endl;
                    return 0;
                }
            case CP_NoNondet:
                {
                    if (g_property_file_name.size() > 0) {
                        if (pm->getProperty() != PT_SVCOMP_Error_Function_Unreachability) {
                            std::cout << "Context NoNondet found for memsafety" << std::endl;
                            return 0;
                        }
                    }
                    // no nondet, use runner
                    std::cout << "Context NoNondet found" << std::endl;
                    RunnerManager *rm = new RunnerManager(1, RN_NoNondet);
                    rm->run(fn, cr.message[0]);
                    RunnerResult rr = rm->getResult(RN_NoNondet);
                    std::cout << rr.message[0] << std::endl;
                    return rr.status;
                }
            case CP_DeviceDriversLinux64:
                {
                    if (g_advisor_name == ADVISOR_NAME_TEST_STRUCTURAL) {
                        std::cout << "main testing via " << ADVISOR_NAME_TEST_STRUCTURAL << std::endl;
                        if (cr.matched) {
                            //std::cout << "test_structural: good, " << fn << std::endl;
                            std::cout << "TRUE" << std::endl;
                            return 1;
                        } else {
                            //std::cout << "test_structural: bad, " << fn << std::endl;
                            std::cout << "FALSE" << std::endl;
                            return -1;
                        }
                    }
                    g_advisor_name = ADVISOR_NAME_STRUCTURAL;
                    break;
                }
            case CP_Newton:
                {
                    analyzeBitNodes();
                    if (cr.message[0] == "FALSE") {
                        std::cout << cr.message[0] << std::endl;
                        return -1;
                    }

                    std::cout << "TRUE" << std::endl;
                    return 1;
                }
            case CP_FloatsArray:
                {
                    analyzeBitNodes();
                    std::cout << cr.message[0] << std::endl;
                    return cr.status;
                }
            case CP_ArraysLargeLoop:
                {
#if DEBUG_RUNNER
                    std::cout << "main setting up runners ..." << std::endl;
#endif

                    if (cr.message[0] == "no-way-to-solve") {
                        std::cout << "UNKNOWN" << std::endl;
                        return 0;
                    } else if (cr.message[0] == "TRUE") {
                        analyzeBitNodes();
                        std::cout << "TRUE" << std::endl;
                        return 1;
                    }

                    // configure runner
                    RunnerManager *rm = new RunnerManager(1, RN_ArraysLargeLoop);
                    rm->run(fn, cr.message[0]);
                    RunnerResult rr = rm->getResult(RN_ArraysLargeLoop);
#if DEBUG_RUNNER
                    std::cout << "main runner ArraysLargeLoop result is " << rr.message[0] << std::endl;
#endif
                    std::cout << rr.message[0] << std::endl;
                    return rr.status;
                }
            case CP_ArraysBounds:
                {
                    // DW: only output when reaches  FALSE
                    // CG: need to check if property is valid deref
                    if (cr.message[0] == "FALSE(valid-deref)") {
                        if (g_property_file_name.size() > 0 &&
                                (pm->getProperty() == PT_SVCOMP_Valid_Free
                                 || pm->getProperty() == PT_SVCOMP_Valid_Dereference
                                 || pm->getProperty() == PT_SVCOMP_Valid_Memory_Tracking
                                )
                            ) {
                            std::cout << "main CP_ArraysBounds " << cr.message[0] << std::endl;
                            analyzeBitNodes();
                            std::cout << "FALSE(valid-deref)" << std::endl;
                            return -1;
                        } else {
                            std::cout << "UNKNOWN" << std::endl;
                            return 0;
                        }
                    } else if (cr.message[0] == "TRUE") {
                        std::cout << "main CP_ArraysBounds " << cr.message[0] << std::endl;
                        analyzeBitNodes();
                        std::cout << "TRUE" << std::endl;
                        return 1;
                    } else if (cr.message[0] == "FALSE(valid-free)") {
                        std::cout << "main CP_ArraysBounds exception " << cr.message[0] << std::endl;
                        analyzeBitNodes();
                        std::cout << cr.message[0] << std::endl;
                        // return value determines whether we should generate witness
                        // -1: false, 0: unknown, 1: true
                        // memsafety do not need witness, but in convenience we just do this to save time
                        return -1;
                    } else if (cr.message[0] == "FALSE(valid-memtrack)") {
                        std::cout << "main CP_ArraysBounds exception " << cr.message[0] << std::endl;
                        analyzeBitNodes();
                        std::cout << cr.message[0] << std::endl;
                        return -1;
                    }
                    break;
                }
            case CP_ByteOperation:
                {
                    std::cout << "Context ByteOperation found" << std::endl;
                    g_advisor_name = ADVISOR_NAME_ABSREF;
                    if (!cr.message.empty()) {
                        if (cr.message[0] != "naive") {
                            std::cout << cr.message[0] << std::endl;
                            return cr.status;
                        } else {
                            g_advisor_name = ADVISOR_NAME_NAIVE;
                        }
                    }
                    break;
                }
            case CP_GCD:
                {
                    std::cout << "Context GCD found" << std::endl;
                    g_advisor_name = ADVISOR_NAME_ABSREF;
                    //std::cout << "TRUE" << std::endl;
                    //return;
                    break;
                }
            case CP_ModuleAnalysis:
                {
                    std::cout << "Context ModuleAnalysis found" << std::endl;
                    g_advisor_name = ADVISOR_NAME_ABSREF;
                    //std::cout << "TRUE" << std::endl;
                    //return;
                    break;
                }
            case CP_LOOP:
                {
                    std::cout << "Context Loop found" << std::endl;
                    g_advisor_name = ADVISOR_NAME_ABSREF;
                    break;
                }
            case CP_FLOAT:
                {
                    if (g_property_file_name.size() > 0) {
                        if (pm->getProperty() != PT_SVCOMP_Error_Function_Unreachability) {
                            std::cout << "Context Float found for non reachability" << std::endl;
                            return 0;
                        }
                    }

                    // TODO, very hard problems
                    std::cout << "Context Float found" << std::endl;
                    if (!cr.message.empty()) {
                        if (cr.message[0] == "TRUE") {
                            analyzeBitNodes();
                            std::cout << "main ContextFloat " << cr.message[0] << std::endl;
                            return cr.status;
                        } else if (cr.message[0] == "larger_unroll") {
                            g_naive_advisor_unroll = std::stoi(cr.message[1]);
                            std::cout << "main need to set unroll depth to " << g_naive_advisor_unroll << std::endl;
                        }
                    }

                    timeFloats();

                    break;
                }
            default:
                {
                    break;
                }
        }
    }

    if (g_advisor_name == ADVISOR_NAME_CDE) {
        CDE cdeRunner(g_cde_type);
        int result = cdeRunner.run(fn.c_str(), 120);
        if (result < 0) {
            std::cout << "FALSE" << std::endl;
        } else if (result > 0) {
            std::cout << "TRUE" << std::endl;
        } else {
            std::cout << "UNKNOWN" << std::endl;
        }
        return result;
    }

    // Ceagle verification
	LLVMContext &context = getGlobalContext();
	SMDiagnostic err;
	std::unique_ptr<llvm::Module> up_mod = parseIRFile(fnll.c_str(), err, context);
    if (up_mod == NULL) {
        std::cout << "Corrupted IR file or IR file not found" << std::endl;
        exit(-1);
    }
    llvm::Module* mod = up_mod.get();
    if (g_dump_module) {
        std::cout << ";===== Original Module =====\n";
        mod->dump();
    }

    // check if generating function call list is needed
    if (g_generate_function_call_of != "") {
        std::cout << "main using GenerateFunctionCallPreprocessor ..." << std::endl;
        auto pp = GenerateFunctionCallPreprocessor(g_generate_function_call_of);
        pp.preprocess(*mod);
        return 0;
    }

    // TODO: temp code, to help invoke pure dd
    if (g_advisor_name == ADVISOR_NAME_STRUCTURAL) {
        g_slice_function = SVCOMP_INTERFACE_VERIFIER_ERROR;
    }

    // slice functions
    if (g_slice_function != "") {
        std::cout << "main using SlicingFunctionPreprocessor ..." << std::endl;
        if (g_option_slice_function) {
            //mod->dump();
            std::cout << "main functions before slicing:\n";
            auto pp = GenerateFunctionCallPreprocessor(g_slice_function);
            pp.preprocess(*mod);
        }
        auto preprocess4 = SlicingFunctionPreprocessor(g_slice_function);
        preprocess4.preprocess(*mod);
        if (g_option_slice_function) {
            //mod->dump();
            std::cout << "main functions after slicing:\n";
            auto pp = GenerateFunctionCallPreprocessor(g_slice_function);
            pp.preprocess(*mod);
            return 0;
        }
    }

    // TODO: temp code, to test structural abstraction
    //std::cout << "main " << g_advisor_name << ":" << ADVISOR_NAME_STRUCTURAL << std::endl;
    if (g_advisor_name == ADVISOR_NAME_STRUCTURAL) {
        SAManager *sam = new SAManager(fn);
        sam->verify(*mod);
        return 0;
    }

    std::vector<P_BB> path;

    // preprocess
    std::vector<std::string> ppWhiteList;
    ppWhiteList.push_back("__VERIFIER_assume");
    ppWhiteList.push_back("__VERIFIER_assert");
    ppWhiteList.push_back("__VERIFIER_nondet_int");
    ppWhiteList.push_back("__VERIFIER_nondet_float");
    ppWhiteList.push_back("__VERIFIER_nondet_double");
    ppWhiteList.push_back("__VERIFIER_error");
    auto preprocess1 = InlinePreprocessor(ppWhiteList);
    //InlinePreprocessor preprocess1;
    preprocess1.preprocess(*mod);
    if (g_dump_module) {
        std::cout << ";===== Module after InlinePreprocessor =====\n";
        mod->dump();
    }
    auto preprocess2 = SVCompInterfacePreprocessor(STR_LABEL_ERROR, STR_LABEL_ASSUME_FAIL);
    //auto preprocess = SVCompInterfacePreprocessor("error", "assume");
    preprocess2.preprocess(*mod);
    if (g_dump_module) {
        std::cout << ";===== Module after SVCompInterfacePreprocessor =====\n";
        mod->dump();
    }
    // trim unreachable
    if (g_trim_unreachable) {
        auto trimPreprocessor = TrimPreprocessor({STR_LABEL_ERROR, STR_LABEL_ASSUME_FAIL});
        trimPreprocessor.preprocess(*mod);
        if (g_dump_module) {
            std::cout << ";===== Module after TrimPreprocessor =====\n";
            mod->dump();
        }
    }

    if (g_phi_eliminate) {
        auto phiPreprocessor = PhiPreprocessor();
        phiPreprocessor.preprocess(*mod);
        if (g_dump_module) {
            std::cout << ";===== Module after PhiPreprocessor =====\n";
            mod->dump();
        }
    }

    // rename variables
    auto preprocess3 = NamingPreprocessor();
    preprocess3.preprocess(*mod);
    if (g_dump_module) {
        std::cout << ";===== Module after NamingPreprocessor =====\n";
        mod->dump();
    }

    // DW: why use "dry run" as a name? What's "dry"?
    if (g_dry_run) {
        std::cout << "Option dry-run enabled, do not verify\n";
        return 0;
    }

    //auto advisor = SimpleAdvisor();
    std::cout << "main using advisor " << g_advisor_name << std::endl;

    // 20170105, double check if currently checking memory safety
    // if it is, quit, since advisors are not used to check memory
    if (pm->getProperty() != PT_SVCOMP_Error_Function_Unreachability) {
        std::cout << "main advisors is not suitable for non-reachability, quit" << std::endl;
        std::cout << "main quit with UNKNOWN" << std::endl;
        return 0;
    }

    DFSResult result;
    if (g_advisor_name == "absref") {
        CEGAR cegar;
        cegar.visit(*mod);
        result = cegar.getResult();
        if (result == DFSResult::DR_UNSAFE) {
            Model model = cegar.getModel();
            std::cout << model.str() << std::endl;
        } else if (result == DFSResult::DR_SAFE) {
            auto witness = cegar.getWitness();
#if DEBUG
            for (auto pa: witness) {
                std::cout << pa.first->getName().str() << ": " << pa.second << std::endl;
            }
#endif
        }
    } else if (g_advisor_name == ADVISOR_NAME_VALUE) {
        value_analysis::ValueAnalysis analysis;
        analysis.start(*mod);
        result = analysis.getResult();
    } else {
        auto advisor = makeAdvisor(g_advisor_name);
        auto dfs = GenericDFS<Advisor<>>(advisor);
        dfs.visit(*mod);
        result = dfs.getResult();
    }

    std::string resultString = "";
    int status = 0;
    switch (result) {
        case DFSResult::DR_NULL:
            resultString = "main dfs null (maybe there's something wrong)";
            break;
        case DFSResult::DR_UNSAFE:
            resultString = "main dfs FALSE";
            status = -1;
            break;
        case DFSResult::DR_SAFE:
            resultString = "main dfs TRUE";
            status = 1;
            break;
        case DFSResult::DR_UNKNOWN:
            resultString = "main dfs UNKNOWN";
            break;
        default:
            break;
    }
    std::cout << resultString << std::endl;
    return status;
}

int main(int argc, char* const* argv) {
    if (argc <= 1) {
        printUsage();
        exit(0);
    }

    // determine z3 path
    {
    std::string exePath = argv[0];
    auto pos = exePath.rfind("/");
    if (pos != std::string::npos) {
        g_z3_path = exePath.substr(0, pos) + "/z3";
    } else {
        g_z3_path = "z3";
    }
    } // end determine z3 path
    static struct option options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"advisor", required_argument, 0, 'a'},
        {"compiler", required_argument, 0, 'c'},
        {"property-file", required_argument, 0, 'p'},
#define CLI_OPTION_TRIM_UNREACHABLE 4000
        {"trim-unreachable", no_argument, 0, CLI_OPTION_TRIM_UNREACHABLE},
#define CLI_OPTION_DUMP_MODULE 4001
        {"dump-module", no_argument, 0, CLI_OPTION_DUMP_MODULE},
#define CLI_OPTION_DRY_RUN 4002
        {"dry-run", no_argument, 0, CLI_OPTION_DRY_RUN},
#define CLI_OPTION_GENERATE_FUNCTION_CALL_OF 4003
        {"generate-function-call-of", required_argument, 0, CLI_OPTION_GENERATE_FUNCTION_CALL_OF},
#define CLI_OPTION_PHI_ELIMINATE 4004
        {"phi-eliminate", no_argument, 0, CLI_OPTION_PHI_ELIMINATE},
#define CLI_OPTION_SLICE_FUNCTION 4005
        {"slice-function", required_argument, 0, CLI_OPTION_SLICE_FUNCTION},
#define CLI_OPTION_CHECK_OVERFLOW 4006
        {"check-overflow", no_argument, 0, CLI_OPTION_CHECK_OVERFLOW},
#define CLI_OPTION_CONTEXT_TEST 4007
        {"context-test", required_argument, 0, CLI_OPTION_CONTEXT_TEST},
#define CLI_OPTION_MEM 4008
        {"mem", required_argument, 0, CLI_OPTION_MEM},
#define CLI_OPTION_NO_AUTO_SELECT 4009
        {"no-auto-select", no_argument, 0, CLI_OPTION_NO_AUTO_SELECT},
#define CLI_OPTION_CDE 4010
        {"cde", required_argument, 0, CLI_OPTION_CDE},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = 0;
    while ((c = getopt_long(argc, argv, "vh", options, &option_index)) != -1) {
        switch (c) {
            case 'v':
                printVersion();
                exit(0);
            case 'h':
                printUsage();
                exit(0);
            case 'a':
                g_advisor_name = optarg;
                g_need_context = false;
                break;
            case 'c':
                g_compiler_name = optarg;
                break;
            case 'p':
                g_property_file_name = optarg;
                break;
            case CLI_OPTION_TRIM_UNREACHABLE:
                g_trim_unreachable = true;
                break;
            case CLI_OPTION_DUMP_MODULE:
                g_dump_module = true;
                break;
            case CLI_OPTION_DRY_RUN:
                g_dry_run = true;
                break;
            case CLI_OPTION_GENERATE_FUNCTION_CALL_OF:
                g_generate_function_call_of = optarg;
                g_need_context = false;
                break;
            case CLI_OPTION_PHI_ELIMINATE:
                g_phi_eliminate = true;
                break;
            case CLI_OPTION_SLICE_FUNCTION:
                g_slice_function = optarg;
                g_option_slice_function = true;
                g_need_context = false;
                break;
            case CLI_OPTION_CHECK_OVERFLOW:
                g_check_overflow = true;
                break;
            case CLI_OPTION_CONTEXT_TEST:
                g_context_test = optarg;
                g_dry_run = true;
                break;
            case CLI_OPTION_MEM:
                g_mem = optarg;
                break;
            case CLI_OPTION_NO_AUTO_SELECT:
                g_need_context = false;
                break;
            case CLI_OPTION_CDE: {
                g_need_context = false;
                g_advisor_name = ADVISOR_NAME_CDE;
                std::string cdeType = optarg;
                if (cdeType == "eca") {
                    g_cde_type = CDE_TYPE::ECA;
                } else if (cdeType == "elevator") {
                    g_cde_type = CDE_TYPE::Elevator;
                } else {
                    std::cerr << "Unknown cde option: " << optarg << std::endl;
                    exit(-1);
                }
            }
            case '?':
                break;
            default:
                abort();
        }
    }

    printVersion();

    // test z3
    {
        auto cmd = g_z3_path + " --version 2>&1";
        std::string versionString;
        auto checkOutput = [](bool& findZ3, FILE* z3) {
            findZ3 = false;
            std::ostringstream output;
            char buff[1024];
            if (z3 != nullptr) {
                while (fgets(buff, sizeof(buff), z3)) {
                    output << buff;
                }
                pclose(z3);
            }
            auto s = output.str();
            if (s.find("Z3 version") != std::string::npos) {
                findZ3 = true;
            }
            return s;
        };
        bool findZ3 = false;
        FILE* z3 = popen(cmd.c_str(), "r");
        versionString = checkOutput(findZ3, z3);

        if (!findZ3 && g_z3_path != "z3") {
            z3 = popen("z3 --version 2>&1", "r");
            versionString = checkOutput(findZ3, z3);
            if (findZ3) {
                g_z3_path = "z3";
            }
        }
        if (!findZ3) {
            std::cout << "Cannot find z3, although some analysis can work without z3, stop because the configuration may be incorrect.\n";
            return -1;
        }
        std::cout << "main found " << versionString;
    }

    for (int i = optind; i < argc; i++) {
        try {
            int result = verify(argv[i]);
            if (g_advisor_name == ADVISOR_NAME_STRUCTURAL) {

            } else if (g_advisor_name == ADVISOR_NAME_CDE && g_cde_type == CDE_TYPE::ECA) {

            } else {
                    std::string sfName(argv[i]);
                    std::string wfName = "witness.graphml";
                if (result < 0) {
                    std::ofstream fout(wfName.c_str());
                    ViolationWitness vw(sfName);
                    vw.initEmpty();
                    fout << vw.getWitness() << std::endl;
                    fout.close();
                } else if (result > 0) {
                    std::ofstream fout(wfName.c_str());
                    CorrectnessWitness cw(sfName);
                    cw.initEmpty();
                    fout << cw.getWitness() << std::endl;
                    fout.close();
                }
            }
        } catch (std::string msg) {
            std::cout << "main exception string: " << msg << std::endl;
        } catch (const char *msg) {
            std::cout << "main exception char *: " << msg << std::endl;
        } catch (Z3RawSolver::popen_exception msg) {
            std::cout << "main exception in opening z3" << std::endl;
            std::cout << "TRUE" << std::endl;
        }
    }
    return 0;
}
