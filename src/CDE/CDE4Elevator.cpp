#include <fstream>
#include <regex>
#include <iostream>
#include <sstream>
#include "CDE.h"

struct popen_exception {};

namespace ceagle {

namespace cde {

int runElevator(const char *sourceFile, int timeLimit) {
    std::string rawName(sourceFile);
    std::ifstream ifs(rawName);
    std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    std::regex r1(R"(extern.*__VERIFIER_error.*;)");
    std::regex r2(R"(extern.*__VERIFIER_nondet_int.*;)");
    std::regex r3(R"(extern.*__assert_fail.*;)");
    std::regex r4(R"(__inline)");
    std::regex r5(R"(typedef\s+unsigned\s+int\s+size_t\s*;)");
    file = std::regex_replace(file, r1, R"(void __VERIFIER_error() { printf("error\n");exit(1) ;})");
    file = std::regex_replace(file, r2, R"(int __VERIFIER_nondet_int() { printf("nondet\n");exit(2) ;})");
    file = std::regex_replace(file, r3, "");
    file = std::regex_replace(file, r4, "");
    file = std::regex_replace(file, r5, "");

    std::string fn = rawName + ".se.c";
    std::ofstream of(fn);
    of << "#include <stdlib.h>" << std::endl;
    of << "#include <stdio.h>" << std::endl;
    of << R"(void __assert_fail(char const *__assertion , char const *__file ,unsigned int __line ,char const *__function ) {
    printf("error\n") ;
    exit(3);
}
)";
    of << file;
    of.close();
    ifs.close();

    std::string bn = rawName + ".se";

    std::string command = "gcc"  " -w " + fn + " -lm -o " + bn + " 2>&1";
    FILE* p = popen(command.c_str(), "r");
    auto pstatus = pclose(p);
    auto status = WEXITSTATUS(pstatus);
    if (status != 0) {
        std::cout << "Cannot compiler " + fn << std::endl;
#if DEBUG
#else
        command = "rm " + fn;
        system(command.c_str());
#endif
        return 0;
    }
    std::string execn = "./" + bn;
    command = "timeout " + std::to_string(timeLimit) + " " + execn;
    p = popen(command.c_str(), "r");
    if (p == nullptr) {
        std::cout << "runCDE popen z3 failed on file: " << rawName << ", reason: " << std::strerror(errno) << std::endl;
        throw popen_exception{};
    }
    char buff[1024];
    std::ostringstream out;
    while (fgets(buff, sizeof(buff), p)) {
        out << buff;
    }
    pstatus = pclose(p);
    auto runResult = out.str();
    if (runResult.find("error") != std::string::npos) {
        return -1;
    } else if (runResult.find("nondet") != std::string::npos) {
        return 0;
    } else {
        return 1;
    }
}

}
}

