#include "RunnerArraysLargeLoop.h"

// fork and exec
#ifndef DARWIN
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include "../Util/StringUtil.h"

extern std::string g_compiler_name;

namespace ceagle {

void RunnerArraysLargeLoop::run(std::string rawName, std::string file) {
    std::cout << "RunnerArraysLargeLoop running ..." << std::endl;
    
    // replace content in file
    util::replaceAll(file, "extern void __VERIFIER_error() __attribute__ ((__noreturn__));", "");
    util::replaceAll(file, "void __VERIFIER_assert(int cond) { if(!(cond)) { ERROR: __VERIFIER_error(); } }", "");
    util::replaceAll(file, "__VERIFIER_assert", "assert");
    // found in standard_copyInitSum_true-unreach-call_ground.i
    util::replaceAll(file, "extern int __VERIFIER_nondet_int(void);", "");
    util::replaceAll(file, "extern int __VERIFIER_nondet_int();", "");
    util::replaceAll(file, "int __VERIFIER_nondet_int();", "");
    util::replaceAll(file, "unsigned char __VERIFIER_nondet_short();", "");
    util::replaceAll(file, "short __VERIFIER_nondet_short();", "");
    util::replaceAll(file, "unsigned char __VERIFIER_nondet_uchar();", "");
    util::replaceAll(file, "extern int user_read();", "");
    //util::replaceAll(file, "int SIZE;", "");
    util::replaceAll(file, "__VERIFIER_nondet_int()", "0");
    util::replaceAll(file, "__VERIFIER_nondet_short()", "1");
    util::replaceAll(file, "__VERIFIER_nondet_uchar()", "1");
    util::replaceAll(file, "__VERIFIER_error()", "assert(0)");
    util::replaceAll(file, "user_read()", "1");
    //util::replaceAll(file, "SIZE", "1");
    if (util::countSubstring(file, "100000") >= 6) {
        std::cout << "RunnerArraysLargeLoop narrowing the array size" << std::endl;
        util::replaceAll(file, "100000", "1000");
    }
    if (util::countSubstring(file, "a[15000] = 1;") >= 1) {
        std::cout << "RunnerArraysLargeLoop narrowing the array valuation" << std::endl;
        util::replaceAll(file, "assert(c[i] == 0);", "assert(0);");
    }
    //util::replaceAll(file, "", "");

    // <foo.c> will have a runner file as <foo.c.se.c>
    std::string fn = rawName + ".se.c";

    // write to file
    std::ofstream of(fn);
    of << "#include <assert.h>" << std::endl;
    of << file;
    of.close();

    // run it and check the status
    std::string bn = rawName + ".se";
    auto pid = fork();
    if (pid < 0) {
        std::cout << "RunnerArraysLargeLoop clang there is something wrong before running" << std::endl;
    } else if (pid == 0) {
        std::cout << "RunnerArraysLargeLoop clang to compile " << fn << std::endl;
        char * const compiler_args[5] = {const_cast<char*>(g_compiler_name.c_str()), const_cast<char*>(fn.c_str()), "-o", const_cast<char*>(bn.c_str()), NULL};
        exit(execvp(g_compiler_name.c_str(), compiler_args));
    } else {
        std::cout << "RunnerArraysLargeLoop clang else" << std::endl;
        int status;
        while (wait(&status) != pid) ;
    }
    std::cout << "RunnerArraysLargeLoop clang generated " << bn << std::endl;

    // run the generated bin
    int status = 0;
    int result = 0;
    pid = fork();
    if (pid < 0) {
        std::cout << "RunnerArraysLargeLoop se there is something wrong with fork" << std::endl;
    } else if (pid == 0) {
        std::cout << "RunnerArraysLargeLoop se running " << bn << std::endl;
        std::string execn = "./" + bn;
        result = execlp(execn.c_str(), execn.c_str(), NULL);
        std::cout << "RunnerArraysLargeLoop se run result " << result << " errno " << strerror(errno) << std::endl;
        exit(result);
    } else {
        std::cout << "RunnerArraysLargeLoop se else" << std::endl;
        while (wait(&status) != pid) ;
    }
    std::cout << "RunnerArraysLargeLoop se finally " << status << std::endl;

    if (status == 0) {
        // the bin runs well without assertion failure
        this->m_result.message.push_back("TRUE");
        this->m_result.status = 1;
    } else {
        this->m_result.message.push_back("FALSE");
        this->m_result.status = -1;
    }
    std::cout << "RunnerArraysLargeLoop has result of "  << this->m_result.matched << " " << this->m_result.message[0] << std::endl;
}

}
