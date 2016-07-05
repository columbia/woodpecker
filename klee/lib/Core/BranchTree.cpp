
#include <assert.h>
#include <iostream>
#include "klee/BranchTree.h"

using namespace std;
using namespace klee;

#define DBG_BRANCH_TREE 0
#define BR_TREE_CERR if(DBG_BRANCH_TREE)std::cerr

BranchNode::BranchNode() {
  dynInstrIdx = (size_t)-1;
  childStates[0] = NULL;
  childStates[1] = NULL;
  childBranches[0] = NULL;
  childBranches[1] = NULL;
  parentBranch = NULL;
}

BranchNode::~BranchNode() {
  
}

void BranchNode::print(int depth, bool singleNode) {
  if (!DBG_BRANCH_TREE)
    return;
  
  std::string startSep = "";
  for (int i = 0; i < depth; i++)
    startSep += "  ";
  
  std::cerr << startSep << "BranchNode: " << (void *)this << " : Instr Idx: " << std::dec << (int)dynInstrIdx << " : ";

  if (childStates[0])
    std::cerr << startSep << (void *)this << " -State[0]-> " << (void *)childStates[0]
      << " (isPruned " << childStates[0]->isPruned << ").\n";
  if (childStates[1])
    std::cerr << startSep << (void *)this << " -State[1]-> " << (void *)childStates[1]
      << " (isPruned " << childStates[1]->isPruned << ").\n";
  
  if (childBranches[0]) {
    std::cerr << startSep << (void *)this << " -Br[0]-> " << (void *)childBranches[0] << ".\n";
    if (!singleNode)
      childBranches[0]->print(depth+1);
    std::cerr << "\n";
  }
  if (childBranches[1]) {
    std::cerr << startSep << (void *)this << " -Br[1]-> " << (void *)childBranches[1] << ".\n";
    if (!singleNode)
      childBranches[1]->print(depth+1);
    std::cerr << "\n";
  }
}

BranchTree::BranchTree() {
  root = NULL;
}

BranchTree::~BranchTree() {

}

void BranchTree::findChildStates(BranchNode *curNode, std::set<ExecutionState *> &childStates) {
  assert(curNode);
  curNode->print(0, true);
  
  // End condition.
  if (curNode->childStates[0]) {
    childStates.insert(curNode->childStates[0]);
  }
  if (curNode->childStates[1]) {
    childStates.insert(curNode->childStates[1]);
  }

  // Recursive.
  std::set<ExecutionState *> leftChildStates;
  std::set<ExecutionState *> rightChildStates;
  if (curNode->childBranches[0]) {
    findChildStates(curNode->childBranches[0], leftChildStates);
  }
  if (curNode->childBranches[1]) {
    findChildStates(curNode->childBranches[1], rightChildStates);
  }
  childStates.insert(leftChildStates.begin(), leftChildStates.end());
  childStates.insert(rightChildStates.begin(), rightChildStates.end());
}


bool BranchTree::findPrunedStates(ExecutionState *curState, size_t dynInstrIdx,
  std::set<ExecutionState *>& prunedStates) {
  BranchNode *leafBranch = curState->parentBranch;
  std::set<ExecutionState *> newPrunedStates;
  BR_TREE_CERR << "BranchTree::findPrunedStates curState: " << (void *)curState
    << ", parentBranch: " << (void *)leafBranch
    << ", dynInstrIdx: " << std::dec << dynInstrIdx << "\n";

  // If there is no symbolic branch at all, just return.
  if (!root)
    return false;
  BranchNode *prevBrNode = NULL;
  BranchNode *curBrNode = leafBranch;
  do {
    // If have found the target node.
    if (curBrNode->dynInstrIdx == dynInstrIdx) {
      if (curBrNode->childStates[0] == curState) {
        if (curBrNode->childStates[1])
          newPrunedStates.insert(curBrNode->childStates[1]);
        if (curBrNode->childBranches[1])
          findChildStates(curBrNode->childBranches[1], newPrunedStates);
      }
      
      if (curBrNode->childStates[1] == curState) {
        if (curBrNode->childStates[0])
          newPrunedStates.insert(curBrNode->childStates[0]);
        if (curBrNode->childBranches[0])
          findChildStates(curBrNode->childBranches[0], newPrunedStates);
      }
      
      if (prevBrNode) {
        if (curBrNode->childBranches[0] == prevBrNode) {
          if (curBrNode->childStates[1])
            newPrunedStates.insert(curBrNode->childStates[1]);
          if (curBrNode->childBranches[1])
            findChildStates(curBrNode->childBranches[1], newPrunedStates);
        }
      
        if (curBrNode->childBranches[1] == prevBrNode) {
          if (curBrNode->childStates[0])
            newPrunedStates.insert(curBrNode->childStates[0]);
          if (curBrNode->childBranches[0])
            findChildStates(curBrNode->childBranches[0], newPrunedStates);
        }
      }
      
      /* This handles both branches in external calls and internal branches.
          If it is branches in external calls, then 'continue' backtracking, until finds 
          a node with different dynInstrIdx;
          If it is internal branch, then its previous node must have different dynInstrIdx with
          curent node, then it can also stop.
      */
      if (!curBrNode->parentBranch ||
        curBrNode->dynInstrIdx != curBrNode->parentBranch->dynInstrIdx) {
        print(curState, dynInstrIdx, newPrunedStates);
        prunedStates.insert(newPrunedStates.begin(), newPrunedStates.end());
        BR_TREE_CERR << "BRANCH FOUND, SYMBOLIC BRANCH " << dynInstrIdx << ".\n";
        if (newPrunedStates.find(curState) != newPrunedStates.end())
          abort();
        return newPrunedStates.size() > 0;
      }
    }

    // If not found the target node yet, backtrack to parent.
    prevBrNode = curBrNode;
    curBrNode = curBrNode->parentBranch;
  } while (curBrNode != NULL); // Hit the root.

  BR_TREE_CERR << "BRANCH NOT FOUND, CONCRETE BRANCH " << dynInstrIdx << ".\n";
  // It could be a concrete branch, so this path is fine.
  return false;
}

void BranchTree::removeState(ExecutionState *curState) {
  if (!root)	// If not path exploration (run klee without --init-env), then just return.
    return;
  int i;
  BranchNode *leafBranch = curState->parentBranch;
  assert(leafBranch); // Invariant: each state must have a parent if it is attached to the branch tree.
  for (i = 0; i < 2; i++)
    if (leafBranch->childStates[i] == curState)
      break;
  assert(i == 0 || i == 1);
  BR_TREE_CERR << "BranchTree::removeState (isPruned " << curState->isPruned<< "): "
    << (void *)curState << endl;
  leafBranch->childStates[i] = NULL;
  curState->parentBranch = NULL;
  print();
}

void BranchTree::addBranch(ExecutionState *curState, size_t dynInstrIdx,
  ExecutionState *otherState) {
  if (!root) {
    root = new BranchNode();
    root->dynInstrIdx = dynInstrIdx;
    root->childStates[0] = curState;
    root->childStates[1] = otherState;
    curState->parentBranch = root;
    otherState->parentBranch = root;
  } else {
    BranchNode *child = new BranchNode();
    child->dynInstrIdx = dynInstrIdx;
    child->childStates[0] = curState;
    child->childStates[1] = otherState;
    BranchNode *parentNode = curState->parentBranch;
    assert(parentNode != NULL);
    child->parentBranch = parentNode;
    if (parentNode->childStates[0] == curState) {
      parentNode->childBranches[0] = child;
      parentNode->childStates[0] = NULL;
    } else {
      assert(parentNode->childStates[1] == curState);
      parentNode->childBranches[1] = child;
      parentNode->childStates[1] = NULL;
    }
    curState->parentBranch = child;
    otherState->parentBranch = child;    
  }
  assert(curState->parentBranch && otherState->parentBranch);
  BR_TREE_CERR << "BranchTree::addBranch curState " << (void *)curState 
    << ", parent " << curState->parentBranch
    << "; otherState " << (void *)otherState 
    << ", parent " << otherState->parentBranch << endl;
}

void BranchTree::print() {
  if (!DBG_BRANCH_TREE)
    return;
  std::cerr << "\n\n\n\n========START========\n\n";
  if (root)
    root->print(0);
  std::cerr << "\n\n=========END=========\n\n\n\n";
}

void BranchTree::print(ExecutionState *curState, size_t dynInstrIdx,
  std::set<ExecutionState *> &prunedStates) {
  std::set<ExecutionState *>::iterator itr;
  // Always print this.
  std::cerr << "PRUNED STATES: curState: " << (void *)curState 
    << ": dynInstrIdx: " << dynInstrIdx 
    << ": prunedStates [" << std::dec << prunedStates.size() << "] : ";
  for (itr = prunedStates.begin(); itr != prunedStates.end(); itr++) {
    std::cerr << (void *)(*itr) << "; ";
  }
  std::cerr << "\n";
}


