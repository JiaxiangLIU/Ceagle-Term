#ifndef CEAGLE_SMT_Z3RAWSOLVER_H
#define CEAGLE_SMT_Z3RAWSOLVER_H
#include <sstream>
#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/popen.h"
namespace ceagle {

namespace smt {

static char z3_args_1[] = "z3";
static char z3_args_2[] = "-in";
static char* const z3_args[3] = {z3_args_1, z3_args_2};
//static char* const z3_args[3] = {"z3", "-in"};

class Z3RawSolver : public RawSolver {
    std::ostringstream po;
public:
    struct popen_exception {};
public:
    Z3RawSolver(bool interp=false) {
        //po << "(set-option :global-decls true)\n";
        if (interp) {
            po << "(set-option :produce-interpolants true)\n";
        }
    }
    virtual std::vector<Expr> getInterp(const std::vector<std::string>& names) override;
    virtual void add(Expr expr, std::string group="") override;
    virtual void add(const std::string &) override;
    virtual void push() override;
    virtual void pop() override;
    virtual void reset() override;
    virtual CheckResult checkSat() override;
    virtual Model getModel() override;
    std::string getDebugInfo() const { return po.str(); }
    std::string run(std::string filename);
private:
    void checkOutput(std::string output);
    std::string declareConst(const std::string& name, const Sort& s) const;
};

}

}
#endif
