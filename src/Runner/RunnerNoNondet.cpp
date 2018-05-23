#include "RunnerNoNondet.h"
// fork and exec
#ifndef DARWIN
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

extern std::string g_compiler_name;

namespace ceagle {

void RunnerNoNondet::run(std::string rawName, std::string file) {
    if (file.find("extern double") != std::string::npos) {
        auto loc = file.find("int main");
        file = file.substr(loc);
    }
    std::regex r1(R"(extern.*__VERIFIER_error.*;)");
    std::regex r2(R"(void __VERIFIER_assert.*)");
    std::regex r3(R"(__VERIFIER_assert)");
    std::regex r4(R"(__VERIFIER_error\(\))");
    std::regex r5(R"(extern.*__VERIFIER_assume.*;)");
    std::regex r6(R"(__VERIFIER_assume\((.*)\);)");
    file = std::regex_replace(file, r1, "");
    file = std::regex_replace(file, r2, "");
    file = std::regex_replace(file, r5, "");
    file = std::regex_replace(file, r3, "assert");
    file = std::regex_replace(file, r4, "assert(0)");
    file = std::regex_replace(file, r6, "if(!($1)) return 0;");

    std::string fn = rawName + ".se.c";
    std::ofstream of(fn);
    of << "#include <assert.h>" << std::endl;
    of << "#include <math.h>" << std::endl;
    of << file;
    of.close();

    std::string bn = rawName + ".se";
    // seems that clang has some issue with builtin function
    std::string command = "gcc"  " -w " + fn + " -lm -o " + bn + " 2>&1";
    FILE* p = popen(command.c_str(), "r");
    auto pstatus = pclose(p);
    auto status = WEXITSTATUS(pstatus);
    if (status != 0) {
        std::cout << "RunnerNoNondet cannot compiler " + rawName << std::endl;
        this->m_result.message.push_back("UNKNOWN");
        return;
    }
    std::string execn = "./" + bn;
    // run the generated bin
    status = 0;
    int result = 0;
    int pid = fork();
    if (pid < 0) {
        std::cout << "RunnerNoNondet se there is something wrong with fork" << std::endl;
    } else if (pid == 0) {
        std::cout << "RunnerNoNondet se running " << bn << std::endl;
        std::string execn = "./" + bn;
        result = execlp(execn.c_str(), execn.c_str(), NULL);
        std::cout << "RunnerNoNondet se run result " << result << " errno " << strerror(errno) << std::endl;
        exit(result);
    } else {
        std::cout << "RunnerNoNondet se else" << std::endl;
        while (wait(&status) != pid) ;
    }
    std::cout << "RunnerNoNondet se finally " << status << std::endl;

    if (status == 0) {
        // the bin runs well without assertion failure
        this->m_result.message.push_back("TRUE");
        this->m_result.status = 1;
    } else {
        this->m_result.message.push_back("FALSE");
        this->m_result.status = -11;
    }
    // clean
    {
        std::string command = "rm " + fn;
        FILE* p = popen(command.c_str(), "r");
        auto pstatus = pclose(p);
        command = "rm " + bn;
        p = popen(command.c_str(), "r");
        pstatus = pclose(p);
    }
    std::cout << "RunnerNoNondet has result of "  << this->m_result.matched << " " << this->m_result.message[0] << std::endl;
}

} // end namespace ceagle
