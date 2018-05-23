#ifndef CEAGLE_ABSREF_ADVISOR_H
#define CEAGLE_ABSREF_ADVISOR_H

#include <map>
#include <string>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h>

#include "DFS/Advisor/Advisor.h"
#include "DFS/Advisor/Absref/ARGManager.h"

namespace ceagle {

using namespace smt;

// Algorithm:
//
// dfsNext
//
// for choice : argManager->nextLocations()
//  absChoice = abstract(choice)
//  if(absChoice.
//

// Algorithm:
//
//  - init currentAbs
//
//  - dfsVisit block
//   - if not revisit:
//    - currentAbs.expand(block)
//   - else:
//    - revisit = false
//   - nextChoices = currentAbs.next()
//
//  - dfsBackward block
//   - if currentAbs.complete():
//    - currentAbs.backward()
//    - return allow
//   - else:
//    - revisit = true
//    - return forbidden
//
//  - expand(block)
//   - if not init:
//    - init(block)
//   - else:
//    - solver.reset()
//    - solver.add(this)
//    - solver.add(getGuard(this, block))
//    - solver.add(getTrans(block))
//    - newNode = new(block)
//    - this.addChild(newNode)
//    - this.set(newNode)
//    - for p in predicates:
//     - solver.push()
//     - solver.add(!p)
//     - result = solver.check()
//     - solver.pop()
//     - if (result == unsat):
//      - this.add(p)
//      - continue
//     - else:
//      - this.add(p)
//      - result = solver.check()
//      - if (result == unsat):
//       - this.add(p)
//
// - next
//  - if !Property.validate(this):
//   - if this.checkConcretePath() == sat:
//    - return unsafe
//   - else:
//    - this.refine()
//  - return getChoices()
//
// - getChoices
//  - choices = {}
//  if this.empty()
//   return choices
//  - solver.reset()
//  - solver.add(this)
//  - for b in this.getNextBlocks():
//   - solver.push()
//   - solver.add(getGuard(this, b))
//   - check = solver.check()
//   - solver.pop()
//   - if check == sat:
//    - choices.add(b)
//  - return choices
typedef const llvm::BasicBlock * LocationTy;
class AbsrefAdvisor : public Advisor<> {
    size_t nameExprCount = 0;
    bool revisit = false;
    bool deadEnd = false;
    std::set<LocationTy> errorSet;
    // temp
    bool isError(const BasicBlock*) const;
protected:
    std::unique_ptr<ARGManager> argManager;
    NextChoice<const BasicBlock*> nextChoices;
public:
    AbsrefAdvisor() : argManager(new ARGManager()) { }
    virtual void dfsVisit(const Module&, const std::vector<const BasicBlock*>&) override;
    virtual NextChoice<const BasicBlock*> dfsNext(const Module&, const std::vector<const BasicBlock*>&) override;
    virtual BackwardPolicy dfsBackward(const Module&, const std::vector<const BasicBlock*>&) override;
};

}

#endif
