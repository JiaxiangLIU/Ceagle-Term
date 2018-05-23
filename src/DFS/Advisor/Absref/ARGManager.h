#ifndef CEAGLE_ABSREF_ARGMANAGER_H
#define CEAGLE_ABSREF_ARGMANAGER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <llvm/IR/BasicBlock.h>

#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"
#include "SMT/Middleware/AST.h"

namespace ceagle{

using namespace smt;

//typedef std::string LocationTy;
typedef const llvm::BasicBlock * LocationTy;

class ARGManager;
class ARGNode;

class ARGManager {
protected:
    std::size_t size = 0;
    std::unique_ptr<ARGNode> root;
    ARGNode* current = nullptr;
    friend class ARGManagerTest;
    std::map<LocationTy, std::vector<ARGNode*>> locationMap;
    std::map<int, std::string> predicates;
    std::map<LocationTy, std::vector<int>> loc2preds;
    std::unique_ptr<Solver> solver;
    std::vector<ARGNode*>& getARGNodesByLocation(LocationTy location);
    Expr getGuard(const llvm::Module&, LocationTy start, LocationTy end, int phiSelector = -1);
    Expr getTrans(LocationTy block);
    ARGNode* abstract(const llvm::Module&, ARGNode* before, LocationTy end, int phiSelector = -1);
public:
    ARGManager() : solver(new Z3RawSolver(true)) { }
    ARGNode* expand(const llvm::Module&, LocationTy child);
    bool empty() const {
        if (root) {
            return false;
        }
        return true;
    }
    std::vector<LocationTy> next();
    void init(LocationTy child);
    bool complete ();
    void backward();
    bool reachable(const llvm::Module&);
    std::vector<LocationTy> getCurrentPath() const;
    std::vector<ARGNode*> getAbsPath() const;
    void refine(const llvm::Module&);
    Expr getGuard(LocationTy start, LocationTy end);
    virtual ~ARGManager() = default;
    static int getPhiIndex(LocationTy start, LocationTy end);
};

class ARGNode {
protected:
    LocationTy location;
    std::set<int> absState;
    ARGNode* parent = nullptr;
    std::unique_ptr<ARGNode> firstChild;
    ARGNode* lastChild = nullptr;
    std::unique_ptr<ARGNode> right;
    friend class ARGNodeTest;
    bool _complete = false;
    bool _empty = false;
public:
    ARGNode(LocationTy name, std::set<int> state={}, bool empty=false) : location(name), absState(state), _empty(empty) { }
    virtual ~ARGNode() = default;
    LocationTy getLocation() const { return location; }
    void reset(ARGNode* node) { this->absState = node->absState; this->_complete = node->_complete; this->_empty = node->_complete; }
    std::vector<ARGNode*> getChilds() const;
    bool isEmpty() const { return _empty; }
    void complete(bool val=true) { this->_complete = val; }
    bool isComplete() const { return this->_complete; }
    bool cover(ARGNode *) const;
    void addChild(std::unique_ptr<ARGNode>);
    inline auto& getAbsState() { return absState; }
    inline void setAbsState(std::set<int> newState) { this->absState = newState; }
    inline ARGNode* getParent() { return parent; }
    inline ARGNode* getLastChild() { return lastChild; }
    operator Expr() const {
        Expr result = BoolValue(true);
        for (const auto index : absState) {
            bool flag = index >= 0;
            bool index_ = flag ? index : -index;
            Expr var = Var("b!" + std::to_string(index_), BoolSort());
            result = result && (flag ? var : !var);
        }
        return result;
    }
};

}

#endif
