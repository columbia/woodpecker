#ifndef KLEE_BRANCH_TREE_H
#define KLEE_BRANCH_TREE_H

#include <set>
#include "klee/ExecutionState.h"

namespace klee {
class BranchNode  {
public:
  size_t dynInstrIdx;
  ExecutionState *childStates[2];
  BranchNode *childBranches[2];
  BranchNode *parentBranch;
  BranchNode();
  ~BranchNode();
  void print(int depth, bool singleNode = false);
};

class BranchTree {
public:
 
  /** Initially NULL, allocated when the first branch is added. **/
  BranchNode *root;

  BranchTree();
  ~BranchTree();
 
  /** Given an execution state and a branch identified by dynamic
  instruction index @br, find all the states reachable in the the other
  sub tree of the branch. This interface is for path slicer. **/
  bool findPrunedStates(ExecutionState *curState, size_t dynInstrIdx,
    std::set<ExecutionState *>& prunedStates);
 
  /** Add a branch instruction identified by dynamic instruction
  index @br to the branch tree. This interface is for fork() in KLEE. **/
  void addBranch(ExecutionState *curState, size_t dynInstrIdx,
    ExecutionState *otherState);

  void removeState(ExecutionState *curState);

  void print();

  void print(ExecutionState *curState, size_t dynInstrIdx,
  	std::set<ExecutionState *> &prunedStates);

protected:
  /** Start from current node (as the root node), recursively find all child states. **/
  void findChildStates(BranchNode *curNode, std::set<ExecutionState *> &childStates);

};
}

#endif

