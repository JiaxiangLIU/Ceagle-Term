#ifndef CEAGLE_SMT_SOLVER_H
#define CEAGLE_SMT_SOLVER_H
#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Model.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <exception>
namespace ceagle {

namespace smt {

enum class CheckResult {
    SAT,
    UNSAT,
    UNKNOWN
};

class SmtException : std::exception {
    const char* reason;
public:
    SmtException(const char* msg) : reason(msg) {
    }
    virtual const char* what() const noexcept {
        return reason;
    }
};

inline std::ostream& operator<<(std::ostream& out, const CheckResult& result) {
    switch(result) {
        case CheckResult::SAT:
            out << "sat";
            break;
        case CheckResult::UNSAT:
            out << "unsat";
            break;
        case CheckResult::UNKNOWN:
            out << "unknown";
            break;
    }
    return out;
}

class Solver {
public:
    virtual void add(Expr expr, std::string group="") {
    }
    virtual void add(const std::string& raw) {
    }
    virtual void push() {
    }
    virtual void pop() {
    }
    virtual CheckResult checkSat() {
        return CheckResult::UNKNOWN;
    }
    virtual Model getModel() {
        throw "not implemented";
    }
    virtual std::vector<Expr> getInterp(const std::vector<std::string>& names) {
        throw "not implemented";
    }
    virtual void reset() {
        throw "not implemented";
    }
    virtual ~Solver() = default;
};

class RawSolver : public Solver {
protected:
    std::map<std::string, Sort> vars;
    std::vector<std::vector<std::string>> push_pop = {{}};
    void processVar(const Expr& expr, std::function<void(const std::string&, const Sort&)>& handle);
private:
    void processVar(const inode<Node>& node, std::function<void(const std::string&, const Sort&)>& handle);
public:
    virtual void push() override;
    virtual void pop() override;
    virtual void reset() override;
};

std::shared_ptr<Solver> makeSolver(bool interp=false);

}

}
#endif
