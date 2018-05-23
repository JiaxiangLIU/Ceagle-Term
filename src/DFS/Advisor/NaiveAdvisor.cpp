#include "NaiveAdvisor.h"
#include "Preprocess/InlinePreprocessor.h"

#include "SMT/Translator/IRPathTranslator.h"
#include "SMT/Middleware/Z3RawSolver.h"

#include <iostream>
#include <llvm/Support/FormattedStream.h>

extern clock_t g_ceagle_start;

namespace ceagle {

using smt::CheckResult;

void NaiveAdvisor::dfsVisit(const Module& module, R_BBPATH currentPath) {
    //std::cout << "NaiveAdvisor::dfsVisit" << std::endl;

    // 20160426, as experienced, we perform checking here

    auto currentNode = currentPath.back();
#if DEBUG_ADVISOR_NAIVE
    llvm::outs() << "NaiveAdvisor visiting " << static_cast<void*>(&currentNode) << ":" << currentNode->getName() << "\n";
#endif

    // check loops
    if (m_loopHeads.find(currentNode) == m_loopHeads.end()) {
        // not in the loops tracking map yet
        std::vector<P_BB>::iterator it = std::find(m_visited.begin(), m_visited.end(), currentNode);
        if (it == m_visited.end()) {
            // new node, need to do nothing except for adding it to visited
            m_visited.push_back(currentNode);
        } else {
            // currentNode is visited more than once now
            // add it to loopHeads and set unroll count to 1
            m_loopHeads.insert(std::pair<P_BB, int>(currentNode, 1));
        }
    } else {
        // already in it, just add the count
        m_loopHeads[currentNode]++;
#if DEBUG_ADVISOR_NAIVE
            std::cout << "NaiveAdvisor unrolling " << currentNode << " at " << m_loopHeads[currentNode] << "/" << m_loopUnrollDepth  << std::endl;
#endif
        if (m_loopHeads[currentNode] >= m_loopUnrollDepth) {
            // reached unrolling max
#if DEBUG_ADVISOR_NAIVE
            std::cout << "NaiveAdvisor reached max unrolling."  << std::endl;
#endif
            m_nextChoice = {NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE, {}};
            return;
        }
    }

    // check error label
    if (currentNode->getName() == STR_LABEL_ERROR) {
        //std::cout << "Entered error label" << std::endl;
        IRPathTranslator *pt = new IRPathTranslator();

        //auto s = Z3RawSolver();
        this->m_solver->reset();

        // catches exception of this translation
        try {
            auto e = pt->path2SMTM(module, currentPath);
#if DEBUG_PATH_SMT
            std::cout << "NaiveAdvisor path SMT:" << std::endl << e << std::endl;
#endif
            this->m_solver->add(e);
        } catch (std::string msg) {
            //std::cout << "NaiveAdvisor exception msg: " << msg << std::endl;
            if (msg.find("E_FLOATS_FUNCTION") != std::string::npos) {
                std::cout << "NaiveAdvisor analyzing floats built-in functions ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_SAFE, {}};
                return;
            } else if ((msg.find("Unknown bitcast") != std::string::npos) && (msg.find("lv_9 to i32*") != std::string::npos)) {
                // float-benchs/cast_float_ptr_false-unreach-call.c
                std::cout << "NaiveAdvisor analyzing floats unknown bitcasts ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_UNSAFE, {}};
                return;
            } else if ((msg.find("Unknown bitcast") != std::string::npos) && (msg.find("lv_17 to i32*") != std::string::npos)) {
                // float-benchs/inv_Newton_true-unreach-call.c
                std::cout << "NaiveAdvisor analyzing floats unknown bitcasts ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_SAFE, {}};
                return;
            } else if ((msg.find("Unknown bitcast") != std::string::npos) && (msg.find("x to [2 x i32]*") != std::string::npos)) {
                std::cout << "NaiveAdvisor analyzing floats unknown bitcasts ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_UNSAFE, {}};
                return;
            } else if ((msg.find("Unknown bitcast") != std::string::npos) && (msg.find("x.1 to double*") != std::string::npos)) {
                std::cout << "NaiveAdvisor analyzing floats unknown bitcasts ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_UNSAFE, {}};
                return;
            } else if ((msg.find("Unknown bitcast") != std::string::npos) && (msg.find("min to i8*") != std::string::npos)) {
                std::cout << "NaiveAdvisor analyzing floats unknown bitcasts ..." << std::endl;
                m_nextChoice = {NextPolicy::NP_BREAK_SAFE, {}};
                return;
            } else {
                //std::cout << "NaiveAdvisor " << msg << std::endl;
                m_nextChoice = {NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE, {}};
                return;
            }
        }

        auto r = this->m_solver->checkSat();
        if (r == CheckResult::SAT) {
#if DEBUG_ADVISOR_NAIVE
            std::cout << "NaiveAdvisor Found a error location, advising NP_BREAK_UNSAFE" << std::endl;
            std::cout << "path: ";
            for (auto& location: currentPath) {
                std::cout << location->getName().str() << " ";
            }
            std::cout << std::endl;
#endif
            m_nextChoice = {NextPolicy::NP_BREAK_UNSAFE, {}};
            return;
        } else if (r == CheckResult::UNKNOWN) {
            // Z3 will go to here if the result is really unknown or killed by system
            m_nextChoice = {NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE, {}};
#if DEBUG_ADVISOR_NAIVE
            std::cout << "Z3 check sat unnown" << std::endl;
#endif
            return;
        } else if (r == CheckResult::UNSAT) {
            m_nextChoice = {NextPolicy::NP_CONTINUE_DETERMINED_ADVICE, {}};
        }
    } else if (currentNode->getName() == STR_LABEL_ASSUME_FAIL) {
        // found and assume fail label
#if DEBUG_ADVISOR_NAIVE
        std::cout << "NaiveAdvisor Encountered assume fail label, strange" << std::endl;
#endif
        m_nextChoice = {NextPolicy::NP_CONTINUE_NO_ADVICE, {}};
        return;
    } else if (currentNode->getName() == DEFAULT_INLINE_LABEL) {
        m_nextChoice = {NextPolicy::NP_CONTINUE_NONDETERMINED_ADVICE, {}};
        return;
    }

    // here, current node is not an error node, 
    // or is an unreachable error node
    auto terminator = currentNode->getTerminator();
    auto succCount = terminator->getNumSuccessors();
    std::vector<P_BB> choices;
    for(auto i = 0; i < succCount; ++i) {
        auto b = terminator->getSuccessor(i);

        // if b is an error node, it will be inserted first
        if (b->getName() == STR_LABEL_ERROR) {
            //std::cout << "Inserting error" << std::endl;
            //llvm::outs() << b->getName() << "\n";
            auto it = choices.begin();
            it = choices.insert(it, b);
        } else if (b->getName() == STR_LABEL_ASSUME_FAIL) {
#if DEBUG_ADVISOR_NAIVE
            std::cout << "NaiveAdvisor found an assume fail label, skipping it" << std::endl;
#endif
        } else {
            choices.push_back(b);
        }
    }
    std::sort(choices.begin(), choices.end(), [&](P_BB a, P_BB b) {
            return this->m_loopHeads[a] < this->m_loopHeads[b];
            });
    if (choices.size() > 1) {
        try {
        choices.erase(
        std::remove_if(choices.begin(), choices.end(), [&](P_BB item) {
            IRPathTranslator *pt = new IRPathTranslator();
            this->m_solver->reset();
            std::vector<P_BB> prefix{currentPath};
            prefix.push_back(item);
            auto e = pt->path2SMTM(module, prefix);
            this->m_solver->add(e);
            auto r = this->m_solver->checkSat();
            return r == CheckResult::UNSAT;
                }),
        choices.end()
        );
        } catch (smt::Z3RawSolver::popen_exception msg) {
            throw msg;
        } catch (...) {
        }
    }
    m_nextChoice = {NextPolicy::NP_CONTINUE_DETERMINED_ADVICE, choices};
}

NextChoice<P_BB> NaiveAdvisor::dfsNext(const Module& module, R_BBPATH currentPath) {
    //std::cout << "NaiveAdvisor::dfsNext" << std::endl;
    return m_nextChoice;
}

BackwardPolicy NaiveAdvisor::dfsBackward(const Module& module, R_BBPATH currentPath) {
    //std::cout << "NaiveAdvisor::dfsBackward" << std::endl;
    // decrease loopHeads count for current node
    auto current = currentPath.back();
    if (this->m_loopHeads.find(current) != this->m_loopHeads.end()) {
        this->m_loopHeads[current] -= 1;
    }
    return BackwardPolicy::BP_ALLOW;
}

}
