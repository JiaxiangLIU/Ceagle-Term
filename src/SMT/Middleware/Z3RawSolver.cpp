#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Z3RawSolver.h"
#include "SMT/Middleware/Model.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>
#include <ctime>
#include <cerrno>
#include <cstring>

#define DEBUG_Z3 0

using namespace ceagle;
using namespace ceagle::smt;

std::string g_z3_path = "z3";

std::string random_string( size_t length )
{
    auto randchar = []() -> char {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

std::string getFileName() {
    char const *folder = getenv("TMPDIR");
    if (folder == 0) folder = "/tmp";
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream s;
    s << std::put_time(&tm, "%dY-%m-%d-%H-%M-%S");
    std::string tempfile = std::string(folder) + "/" + s.str() + random_string(8) + ".smt2";
    return tempfile;
}

std::string Z3RawSolver::run(std::string filename) {
    std::string command = g_z3_path + " " + filename;
    FILE* p = popen(command.c_str(), "r");
    if (p == nullptr) {
        std::cout << "Z3RawSolver::run popen z3 failed on file: " << filename << ", reason: " << std::strerror(errno) << std::endl;
        throw popen_exception{};
    }
    char buff[1024];
    std::ostringstream out;
    while (fgets(buff, sizeof(buff), p)) {
        out << buff;
    }
    pclose(p);
    auto result = out.str();
    checkOutput(result);
    return result;
}

void Z3RawSolver::add(Expr expr, std::string group) {
    std::function<void(const std::string& name, const Sort&s)> handle = [&](const std::string& name, const Sort& s) {
        po << declareConst(name, s) << "\n";
    };
    this->processVar(expr, handle);
    if(group != "") {
        expr = expr.param(":named", group);
    }
    po << "(assert " << expr << ")\n";
}

void Z3RawSolver::add(const std::string& raw) {
    po << "(assert " << raw << ")\n";
}

void Z3RawSolver::push() {
    RawSolver::push();
    po << "(push)\n";
}

void Z3RawSolver::pop() {
    RawSolver::pop();
    po << "(pop)\n";
}

void Z3RawSolver::reset() {
    RawSolver::reset();
    // po << "(reset)\n";
    po = std::ostringstream();
}

CheckResult Z3RawSolver::checkSat() {
#if DEBUG_Z3
    std::cout << "========== debug info ==========\n";
    std::cout << po.str();
    std::cout << "================================\n";
#endif
    auto filename = getFileName();
    std::ofstream file;
    file.open(filename);
    file << po.str();
    file << "(check-sat)\n";
    file.close();
    auto result = run(filename);
    if (result.substr(0,3) == "sat") {
        return CheckResult::SAT;
    } else if (result.substr(0,5) == "unsat") {
        return CheckResult::UNSAT;
    } else {
        return CheckResult::UNKNOWN;
    }
}

Model Z3RawSolver::getModel() {
    std::map<std::string, std::string> model;
    auto filename = getFileName();
    std::ofstream file;
    file.open(filename);
    file << po.str();
    file << "(check-sat)\n";
    std::vector<std::string> varOrder;
    for(const auto& item: vars) {
        auto name = item.first;
        varOrder.push_back(name);
        file << "(eval " << name << ")\n";
    }
    file.close();
    auto result = run(filename);
    std::istringstream iss(result);
    std::string line;
    auto it = varOrder.begin();
    std::getline(iss, line, '\n');
    while (std::getline(iss, line, '\n')) {
        model[*it] = line;
        ++it;
    }
    return Model(model);
}

std::vector<Expr> Z3RawSolver::getInterp(const std::vector<std::string>& names) {
#if DEBUG_Z3
    std::cout << "========== debug info ==========\n";
    std::cout << po.str();
    std::cout << "================================\n";
#endif
    assert("interpolant names must greater equal than 2" && names.size() >= 2);
    auto filename = getFileName();
    std::ofstream file;
    file.open(filename);
    file << po.str();
    file << "(check-sat)\n";
    file << "(get-interpolant";
    for(const auto& name: names) {
        file << " " << name;
    }
    file << ")\n";
    file.close();
    std::vector<Expr> result;
    auto output = run(filename);
    checkOutput(output);
    std::stringstream s(output);
    std::string line;std::getline(s, line, '\n');
    Parser parser(vars);
    while(std::getline(s, line, '\n')) {
        result.push_back(parser.parse(line));
    }
    return result;
}

void Z3RawSolver::checkOutput(std::string output) {
    if (output.substr(0,6) == "(error") {
        throw output + ";debug info\n" + po.str();
    }
}

std::string Z3RawSolver::declareConst(const std::string& name, const Sort& s) const {
    std::ostringstream out;
    auto sstr = s.str();
    // if (sstr == "" && (name == "conv" || name == "conv2" || name == "conv3" || name == "conv16" || name == "conv17" || name == "_rem" || name == "conv3" || name == "conv4" || name == "conv.1" || name == "conv4.5" ||name == "conv4.6" || name == "conv12" || name == "conv8" || name == "conv9")) {
    // if ( sstr == "" && (name.substr(0, 4) == "conv" || name == "_rem") ) {
    //    sstr = "(_ BitVec 32)";
    // }
    if (sstr == "") {
        sstr = Sort::varSortMap[name].str();
    }
    out << "(declare-const " << name << " " << sstr << ")";
    return out.str();
}
