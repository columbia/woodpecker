//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Internal/ADT/TreeStream.h"
#include "klee/CheckerMgr.h"
#include "klee/Log.h"

// FIXME: We do not want to be exposing these? :(
#include "../../lib/Core/AddressSpace.h"
#include "klee/Internal/Module/KInstIterator.h"
#include "klee/BranchTree.h"

#include <map>
#include <set>
#include <vector>
#include <queue>

namespace klee {
  class Array;
  class CallPathNode;
  struct Cell;
  struct KFunction;
  struct KInstruction;
  class MemoryObject;
  class PTreeNode;
  struct InstructionInfo;
  class BranchNode;

std::ostream &operator<<(std::ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject*> allocas;
  Cell *locals;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is setup to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

class ThreadState {
private:
  ExecutionState *parent;

public:
  typedef std::vector<StackFrame> stack_ty;

  // pc - pointer to current instruction stream
  KInstIterator pc, prevPC;
  stack_ty stack;
  int id;

  unsigned incomingBBIndex;

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();
  void dumpStack(std::ostream &out) const;
  void setParent(ExecutionState *_parent) { parent = _parent; }

  ThreadState(ExecutionState *_parent, int _id) : parent(_parent), id(_id) {}

};

struct MutexInfo {
    bool acquired;
    std::queue<int> waiters;
    int owner;

    MutexInfo() : acquired(false) {}
};

struct RWLockInfo {
    bool reading, writing;
    std::queue<int> write_waiters, read_waiters;
    int writer;
    std::map<int, int> readers;

    RWLockInfo() : reading(false), writing(false) {}
};

struct CondInfo {
    std::queue<int> waiters;
    uint64_t binded;
};

struct FileInfo {
    int fd;
    int refcnt;
};

class ResMgr {
private:
    Log log;
    std::map<int, FileInfo *> files;
    int unrefFile(FileInfo *info);

public:
    ResMgr();
    ResMgr(const ResMgr& other);
    virtual ~ResMgr();

    int openFile(const char *path, int flags, mode_t mode);
    int closeFile(int fd);
};

class ExecutionState {
public:
  typedef std::map< int, ThreadState > threads_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState&); 
  std::map< std::string, std::string > fnAliases;

public:
  bool fakeState;
  // Are we currently underconstrained?  Hack: value is size to make fake
  // objects.
  unsigned underConstrained;
  unsigned depth;
  
  threads_ty threads;
  std::set<int> blockedThreads;
  int lastTid;
  int maxThreadNumber;
  std::map< uint64_t, MutexInfo > mutexes;
  std::map< uint64_t, CondInfo > conds;
  std::map< uint64_t, RWLockInfo > rwlocks;
  CheckerMgr localCheckers;
  ResMgr resMgr;
  
  ref<Expr> mallocFailCounter;
  int mallocNum;
  bool failMalloc;
  llvm::Function *curFunc;
  std::vector< ref<Expr> > *curArguments;

  /// For path slicer, mark a state as pruned without actually pruning it. For evaluation.

  bool isPruned;
  BranchNode *parentBranch;

  ConstraintManager constraints;
  mutable double queryCost;
  double weight;
  AddressSpace addressSpace;
  TreeOStream pathOS, symPathOS;
  unsigned instsSinceCovNew;
  bool coveredNew;

  /// Disables forking, set by user code.
  bool forkDisabled;

  std::map<const std::string*, std::set<unsigned> > coveredLines;
  PTreeNode *ptreeNode;

  /// ordered list of symbolics: used to generate test cases. 
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector< std::pair<const MemoryObject*, const Array*> > symbolics;

  /// Set of used array names.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  // Used by the checkpoint/rollback methods for fake objects.
  // FIXME: not freeing things on branch deletion.
  MemoryMap shadowObjects;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

  int createThread(KFunction *kf);
  void stepBack();

  ThreadState *lastThread();
  ThreadState *getThread(int tid);
  
private:
  ExecutionState() : fakeState(false), underConstrained(0), ptreeNode(0) {}

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState& state);

  ~ExecutionState();
  
  ExecutionState *branch();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) { 
    constraints.addConstraint(e); 
  }

  bool merge(const ExecutionState &b);
  void dumpStack(std::ostream &out) const;
};

}

#endif
