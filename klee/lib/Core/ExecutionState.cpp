//===-- ExecutionState.cpp ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/ExecutionState.h"

#include "klee/Internal/Module/Cell.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"

#include "klee/Expr.h"

#include "Memory.h"

#include "llvm/Function.h"
#include "llvm/Support/CommandLine.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <map>
#include <set>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

using namespace llvm;
using namespace klee;

namespace { 
  cl::opt<bool>
  DebugLogStateMerge("debug-log-state-merge");
}

/***/

StackFrame::StackFrame(KInstIterator _caller, KFunction *_kf)
  : caller(_caller), kf(_kf), callPathNode(0), 
    minDistToUncoveredOnReturn(0), varargs(0) {
  locals = new Cell[kf->numRegisters];
}

StackFrame::StackFrame(const StackFrame &s) 
  : caller(s.caller),
    kf(s.kf),
    callPathNode(s.callPathNode),
    allocas(s.allocas),
    minDistToUncoveredOnReturn(s.minDistToUncoveredOnReturn),
    varargs(s.varargs) {
  locals = new Cell[s.kf->numRegisters];
  for (unsigned i=0; i<s.kf->numRegisters; i++)
    locals[i] = s.locals[i];
}

StackFrame::~StackFrame() { 
  delete[] locals; 
}

/***/

ExecutionState::ExecutionState(KFunction *kf) 
  : fakeState(false),
    underConstrained(false),
    depth(0),
//    pc(kf->instructions),
//    prevPC(pc),
    lastTid(-1),
    maxThreadNumber(-1),
    failMalloc(false),
    isPruned(false),
    parentBranch(NULL),
    queryCost(0.), 
    weight(1),
    instsSinceCovNew(0),
    coveredNew(false),
    forkDisabled(false),
    ptreeNode(0) {
  createThread(kf);
}

ExecutionState::ExecutionState(const std::vector<ref<Expr> > &assumptions) 
  : fakeState(true),
    underConstrained(false),
    lastTid(-1),
    failMalloc(false),
    isPruned(false),
    parentBranch(NULL),
    constraints(assumptions),
    queryCost(0.),
    ptreeNode(0) {
}

ExecutionState::~ExecutionState() {
  for (unsigned int i=0; i<symbolics.size(); i++)
  {
    const MemoryObject *mo = symbolics[i].first;
    assert(mo->refCount > 0);
    mo->refCount--;
    if (mo->refCount == 0)
      delete mo;
  }

  for (threads_ty::iterator it = threads.begin(); it != threads.end(); it++)
      while (!(*it).second.stack.empty()) (*it).second.popFrame();
}

ExecutionState::ExecutionState(const ExecutionState& state)
  : fnAliases(state.fnAliases),
    fakeState(state.fakeState),
    underConstrained(state.underConstrained),
    depth(state.depth),
    threads(state.threads),
    blockedThreads(state.blockedThreads),
    lastTid(state.lastTid),
    maxThreadNumber(state.maxThreadNumber),
    mutexes(state.mutexes),
    conds(state.conds),
    rwlocks(state.rwlocks),
    localCheckers(state.localCheckers),
    resMgr(state.resMgr),
    mallocFailCounter(state.mallocFailCounter),
    mallocNum(state.mallocNum),
    failMalloc(state.failMalloc),
    curFunc(state.curFunc),
    curArguments(state.curArguments),
    isPruned(state.isPruned),
    parentBranch(NULL),
    constraints(state.constraints),
    queryCost(state.queryCost),
    weight(state.weight),
    addressSpace(state.addressSpace),
    pathOS(state.pathOS),
    symPathOS(state.symPathOS),
    instsSinceCovNew(state.instsSinceCovNew),
    coveredNew(state.coveredNew),
    forkDisabled(state.forkDisabled),
    coveredLines(state.coveredLines),
    ptreeNode(state.ptreeNode),
    symbolics(state.symbolics),
    arrayNames(state.arrayNames),
    shadowObjects(state.shadowObjects)
{
  for (threads_ty::iterator it = threads.begin(); it != threads.end(); it++)
      (*it).second.setParent(this);
  for (unsigned int i=0; i<symbolics.size(); i++)
    symbolics[i].first->refCount++;
}

ExecutionState *ExecutionState::branch() {
  depth++;
  ExecutionState *falseState = new ExecutionState(*this);
  falseState->coveredNew = false;
  falseState->coveredLines.clear();

  weight *= .5;
  falseState->weight -= weight;

  return falseState;
}

void ThreadState::pushFrame(KInstIterator caller, KFunction *kf) {
//    std::cerr << "Pushing frame..." << std::endl;
  stack.push_back(StackFrame(caller,kf));
}

void ThreadState::popFrame() {
//    std::cerr << "Popping frame..." << std::endl;
  StackFrame &sf = stack.back();
  for (std::vector<const MemoryObject*>::iterator it = sf.allocas.begin(), 
         ie = sf.allocas.end(); it != ie; ++it)
    parent->addressSpace.unbindObject(*it);
  stack.pop_back();
}

void ExecutionState::addSymbolic(const MemoryObject *mo, const Array *array) { 
  mo->refCount++;
  symbolics.push_back(std::make_pair(mo, array));
}
///

std::string ExecutionState::getFnAlias(std::string fn) {
  std::map < std::string, std::string >::iterator it = fnAliases.find(fn);
  if (it != fnAliases.end())
    return it->second;
  else return "";
}

void ExecutionState::addFnAlias(std::string old_fn, std::string new_fn) {
  fnAliases[old_fn] = new_fn;
}

void ExecutionState::removeFnAlias(std::string fn) {
  fnAliases.erase(fn);
}

/**/

std::ostream &klee::operator<<(std::ostream &os, const MemoryMap &mm) {
  os << "{";
  MemoryMap::iterator it = mm.begin();
  MemoryMap::iterator ie = mm.end();
  if (it!=ie) {
    os << "MO" << it->first->id << ":" << it->second;
    for (++it; it!=ie; ++it)
      os << ", MO" << it->first->id << ":" << it->second;
  }
  os << "}";
  return os;
}

bool ExecutionState::merge(const ExecutionState &b) {
  if (DebugLogStateMerge)
    std::cerr << "-- attempting merge of A:" 
               << this << " with B:" << &b << "--\n";

    // XXX is it even possible for these to differ? does it matter? probably
    // implies difference in object states?
  if (symbolics!=b.symbolics)
      return false;

  if (threads.size() != b.threads.size())
      return false;

  // do per-thread comparison
  for (threads_ty::iterator it = threads.begin(); it != threads.end(); it++)
  {
    ThreadState &thr = (*it).second;
    int tid = (*it).first;
    if (b.threads.find(tid) == b.threads.end())
        return false;

    const ThreadState &b_thr = (*b.threads.find(tid)).second;
    if (thr.pc != b_thr.pc)
      return false;

    {
        std::vector<StackFrame>::const_iterator itA = thr.stack.begin();
        std::vector<StackFrame>::const_iterator itB = b_thr.stack.begin();
        while (itA!=thr.stack.end() && itB!=b_thr.stack.end()) {
            // XXX vaargs?
            if (itA->caller!=itB->caller || itA->kf!=itB->kf)
                return false;
            ++itA;
            ++itB;
        }
        if (itA!=thr.stack.end() || itB!=b_thr.stack.end())
            return false;
    }
  }

  std::set< ref<Expr> > aConstraints(constraints.begin(), constraints.end());
  std::set< ref<Expr> > bConstraints(b.constraints.begin(), 
                                     b.constraints.end());
  std::set< ref<Expr> > commonConstraints, aSuffix, bSuffix;
  std::set_intersection(aConstraints.begin(), aConstraints.end(),
                        bConstraints.begin(), bConstraints.end(),
                        std::inserter(commonConstraints, commonConstraints.begin()));
  std::set_difference(aConstraints.begin(), aConstraints.end(),
                      commonConstraints.begin(), commonConstraints.end(),
                      std::inserter(aSuffix, aSuffix.end()));
  std::set_difference(bConstraints.begin(), bConstraints.end(),
                      commonConstraints.begin(), commonConstraints.end(),
                      std::inserter(bSuffix, bSuffix.end()));
  if (DebugLogStateMerge) {
    std::cerr << "\tconstraint prefix: [";
    for (std::set< ref<Expr> >::iterator it = commonConstraints.begin(), 
           ie = commonConstraints.end(); it != ie; ++it)
      std::cerr << *it << ", ";
    std::cerr << "]\n";
    std::cerr << "\tA suffix: [";
    for (std::set< ref<Expr> >::iterator it = aSuffix.begin(), 
           ie = aSuffix.end(); it != ie; ++it)
      std::cerr << *it << ", ";
    std::cerr << "]\n";
    std::cerr << "\tB suffix: [";
    for (std::set< ref<Expr> >::iterator it = bSuffix.begin(), 
           ie = bSuffix.end(); it != ie; ++it)
      std::cerr << *it << ", ";
    std::cerr << "]\n";
  }

  // We cannot merge if addresses would resolve differently in the
  // states. This means:
  // 
  // 1. Any objects created since the branch in either object must
  // have been free'd.
  //
  // 2. We cannot have free'd any pre-existing object in one state
  // and not the other

  if (DebugLogStateMerge) {
    std::cerr << "\tchecking object states\n";
    std::cerr << "A: " << addressSpace.objects << "\n";
    std::cerr << "B: " << b.addressSpace.objects << "\n";
  }
    
  std::set<const MemoryObject*> mutated;
  MemoryMap::iterator ai = addressSpace.objects.begin();
  MemoryMap::iterator bi = b.addressSpace.objects.begin();
  MemoryMap::iterator ae = addressSpace.objects.end();
  MemoryMap::iterator be = b.addressSpace.objects.end();
  for (; ai!=ae && bi!=be; ++ai, ++bi) {
    if (ai->first != bi->first) {
      if (DebugLogStateMerge) {
        if (ai->first < bi->first) {
          std::cerr << "\t\tB misses binding for: " << ai->first->id << "\n";
        } else {
          std::cerr << "\t\tA misses binding for: " << bi->first->id << "\n";
        }
      }
      return false;
    }
    if (ai->second != bi->second) {
      if (DebugLogStateMerge)
        std::cerr << "\t\tmutated: " << ai->first->id << "\n";
      mutated.insert(ai->first);
    }
  }
  if (ai!=ae || bi!=be) {
    if (DebugLogStateMerge)
      std::cerr << "\t\tmappings differ\n";
    return false;
  }
  
  // merge stack

  ref<Expr> inA = ConstantExpr::alloc(1, Expr::Bool);
  ref<Expr> inB = ConstantExpr::alloc(1, Expr::Bool);
  for (std::set< ref<Expr> >::iterator it = aSuffix.begin(), 
         ie = aSuffix.end(); it != ie; ++it)
    inA = AndExpr::create(inA, *it);
  for (std::set< ref<Expr> >::iterator it = bSuffix.begin(), 
         ie = bSuffix.end(); it != ie; ++it)
    inB = AndExpr::create(inB, *it);

  for (threads_ty::iterator it = threads.begin(); it != threads.end(); it++)
  {
      ThreadState &thr = (*it).second;
      const ThreadState &b_thr = (*b.threads.find((*it).first)).second;

      // XXX should we have a preference as to which predicate to use?
      // it seems like it can make a difference, even though logically
      // they must contradict each other and so inA => !inB

      std::vector<StackFrame>::iterator itA = thr.stack.begin();
      std::vector<StackFrame>::const_iterator itB = b_thr.stack.begin();
      for (; itA!=thr.stack.end(); ++itA, ++itB) {
          StackFrame &af = *itA;
          const StackFrame &bf = *itB;
          for (unsigned i=0; i<af.kf->numRegisters; i++) {
              ref<Expr> &av = af.locals[i].value;
              const ref<Expr> &bv = bf.locals[i].value;
              if (av.isNull() || bv.isNull()) {
                  // if one is null then by implication (we are at same pc)
                  // we cannot reuse this local, so just ignore
              } else {
                  av = SelectExpr::create(inA, av, bv);
              }
          }
      }
  }

  for (std::set<const MemoryObject*>::iterator it = mutated.begin(), 
         ie = mutated.end(); it != ie; ++it) {
    const MemoryObject *mo = *it;
    const ObjectState *os = addressSpace.findObject(mo);
    const ObjectState *otherOS = b.addressSpace.findObject(mo);
    assert(os && !os->readOnly && 
           "objects mutated but not writable in merging state");
    assert(otherOS);

    ObjectState *wos = addressSpace.getWriteable(mo, os);
    for (unsigned i=0; i<mo->size; i++) {
      ref<Expr> av = wos->read8(i);
      ref<Expr> bv = otherOS->read8(i);
      wos->write(i, SelectExpr::create(inA, av, bv));
    }
  }

  constraints = ConstraintManager();
  for (std::set< ref<Expr> >::iterator it = commonConstraints.begin(), 
         ie = commonConstraints.end(); it != ie; ++it)
    constraints.addConstraint(*it);
  constraints.addConstraint(OrExpr::create(inA, inB));

  return true;
}

void ExecutionState::dumpStack(std::ostream &out) const {
    for (threads_ty::const_iterator it = threads.begin(); it != threads.end(); it++)
    {
        (*it).second.dumpStack(out);
    }
}

void ThreadState::dumpStack(std::ostream &out) const {
  unsigned idx = 0;
  const KInstruction *target = prevPC;
  for (stack_ty::const_reverse_iterator
         it = stack.rbegin(), ie = stack.rend();
       it != ie; ++it) {
    const StackFrame &sf = *it;
    Function *f = sf.kf->function;
    const InstructionInfo &ii = *target->info;
    out << "\t#" << idx++ 
        << " " << std::setw(8) << std::setfill('0') << ii.assemblyLine
        << " in " << f->getName().str() << " (";
    // Yawn, we could go up and print varargs if we wanted to.
    unsigned index = 0;
    for (Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
         ai != ae; ++ai) {
      if (ai!=f->arg_begin()) out << ", ";

      out << ai->getName().str();
      // XXX should go through function
      ref<Expr> value = sf.locals[sf.kf->getArgRegister(index++)].value; 
      if (isa<ConstantExpr>(value))
        out << "=" << value;
    }
    out << ")";
    if (ii.file != "")
      out << " at " << ii.file << ":" << ii.line;
    out << "\n";
    target = sf.caller;
  }
}

int ExecutionState::createThread(KFunction *kf)
{
    int tid = maxThreadNumber + 1;
    maxThreadNumber++;
    threads.insert(std::pair<int, ThreadState>(tid, *new ThreadState(this, tid)));

    ThreadState &thr = (*threads.find(tid)).second;
    thr.pc = kf->instructions;
    thr.prevPC = thr.pc;

    thr.pushFrame(0, kf);

    if (lastTid < 0)
        lastTid = tid;

    return tid;
}

void ExecutionState::stepBack()
{
    if (lastTid >= 0)
    {
        if (threads.find(lastTid) != threads.end())
            (*threads.find(lastTid)).second.pc = (*threads.find(lastTid)).second.prevPC;
    }
}

ThreadState* ExecutionState::lastThread()
{
    if (lastTid >= 0)
        if (threads.find(lastTid) != threads.end())
            return &(*threads.find(lastTid)).second;
    return NULL;
}

ThreadState* ExecutionState::getThread(int tid)
{
    threads_ty::iterator it = threads.find(tid);
    if (it == threads.end())
        return NULL;
    else
        return &(*it).second;
}

ResMgr::ResMgr()
: log("ExecutionState.ResMgr") {
}

//#define RESMGR_DEBUG

ResMgr::ResMgr(const ResMgr& other)
: files(other.files), log("ExecutionState.ResMgr")
{
    for (std::map<int, FileInfo *>::iterator it = files.begin();
            it != files.end(); it++) {
#ifdef RESMGR_DEBUG
        log.debug("inc refcnt for %d", it->first);
#endif
        it->second->refcnt++;
#ifdef RESMGR_DEBUG
        log.debug("refcnt: %d", it->second->refcnt);
#endif
    }
}

ResMgr::~ResMgr() {
    for (std::map<int, FileInfo *>::iterator it = files.begin();
            it != files.end(); it++) {
        unrefFile(it->second);
    }
}

int ResMgr::unrefFile(FileInfo *info) {
#ifdef RESMGR_DEBUG
    log.debug("dec refcnt for %d", info->fd);
#endif
    info->refcnt--;
#ifdef RESMGR_DEBUG
    log.debug("refcnt: %d", info->refcnt);
#endif
    if (info->refcnt == 0) {
#ifdef RESMGR_DEBUG
        log.debug("close file %d", info->fd);
#endif
        close(info->fd);
        free(info);
        return 1;
    }
    return 0;
}

int ResMgr::openFile(const char *path, int flags, mode_t mode) {
    int fd = open(path, flags, mode);
    if (fd >= 0) {
        FileInfo *info = new FileInfo();
        info->fd = fd;
        info->refcnt = 1;
#ifdef RESMGR_DEBUG
        log.debug( "init refcnt for %d to 1", fd);
#endif
        files[fd] = info;
    }
    return fd;
}

int ResMgr::closeFile(int fd) {
    if (files.find(fd) != files.end()) {
        unrefFile(files[fd]);
        files.erase(fd);
        return 0;
    } else {
        log.err("closeFile: file %d not found. opened elsewhere?", fd);
        return -1;
    }
}
