//===-- SpecialFunctionHandler.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Common.h"

#include "Memory.h"
#include "SpecialFunctionHandler.h"
#include "TimingSolver.h"

#include "klee/ExecutionState.h"

#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"

#include "Executor.h"
#include "MemoryManager.h"
#include "StatsTracker.h"

#include "llvm/Module.h"
#include "llvm/ADT/Twine.h"

#include <errno.h>

using namespace llvm;
using namespace klee;

/// \todo Almost all of the demands in this file should be replaced
/// with terminateState calls.

///

struct HandlerInfo {
  const char *name;
  SpecialFunctionHandler::Handler handler;
  bool doesNotReturn; /// Intrinsic terminates the process
  bool hasReturnValue; /// Intrinsic has a return value
  bool doNotOverride; /// Intrinsic should not be used if already defined
};

// FIXME: We are more or less committed to requiring an intrinsic
// library these days. We can move some of this stuff there,
// especially things like realloc which have complicated semantics
// w.r.t. forking. Among other things this makes delayed query
// dispatch easier to implement.
HandlerInfo handlerInfo[] = {
#define add(name, handler, ret) { name, \
                                  &SpecialFunctionHandler::handler, \
                                  false, ret, false }
#define addDNR(name, handler) { name, \
                                &SpecialFunctionHandler::handler, \
                                true, false, false }
  addDNR("__assert_rtn", handleAssertFail),
  addDNR("__assert_fail", handleAssertFail),
  addDNR("_assert", handleAssert),
  addDNR("abort", handleAbort),
  addDNR("_exit", handleExit),
  { "exit", &SpecialFunctionHandler::handleExit, true, false, true },
  addDNR("klee_abort", handleAbort),
  addDNR("klee_silent_exit", handleSilentExit),  
  addDNR("klee_report_error", handleReportError),

  add("calloc", handleCalloc, true),
  add("free", handleFree, false),
  add("klee_assume", handleAssume, false),
  add("klee_check_memory_access", handleCheckMemoryAccess, false),
  add("klee_get_valuef", handleGetValue, true),
  add("klee_get_valued", handleGetValue, true),
  add("klee_get_valuel", handleGetValue, true),
  add("klee_get_valuell", handleGetValue, true),
  add("klee_get_value_i32", handleGetValue, true),
  add("klee_get_value_i64", handleGetValue, true),
  add("klee_define_fixed_object", handleDefineFixedObject, false),
  add("klee_get_obj_size", handleGetObjSize, true),
  add("klee_get_errno", handleGetErrno, true),
  add("klee_is_symbolic", handleIsSymbolic, true),
  add("klee_make_symbolic", handleMakeSymbolic, false),
  add("klee_mark_global", handleMarkGlobal, false),
  add("klee_merge", handleMerge, false),
  add("klee_prefer_cex", handlePreferCex, false),
  add("klee_print_expr", handlePrintExpr, false),
  add("klee_print_range", handlePrintRange, false),
  add("klee_set_forking", handleSetForking, false),
  add("klee_stack_trace", handleStackTrace, false),
  add("klee_warning", handleWarning, false),
  add("klee_warning_once", handleWarningOnce, false),
  add("klee_alias_function", handleAliasFunction, false),
  add("malloc", handleMalloc, true),
  add("realloc", handleRealloc, true),

  // operator delete[](void*)
  add("_ZdaPv", handleDeleteArray, false),
  // operator delete(void*)
  add("_ZdlPv", handleDelete, false),

  // operator new[](unsigned int)
  add("_Znaj", handleNewArray, true),
  // operator new(unsigned int)
  add("_Znwj", handleNew, true),

  // FIXME-64: This is wrong for 64-bit long...

  // operator new[](unsigned long)
  add("_Znam", handleNewArray, true),
  // operator new(unsigned long)
  add("_Znwm", handleNew, true),

  add("klee_notify_counter", handleNotifyCounter, true),
  add("klee_sys_open", handleSysOpen, true),
  add("klee_sys_close", handleSysClose, true),

  add("pthread_create", handlePthreadCreate, true),

  add("pthread_mutex_init", handlePthreadMutexInit, true),
  add("pthread_mutex_destroy", handlePthreadMutexDestroy, true),
  add("pthread_mutex_lock", handlePthreadMutexLock, true),
  add("pthread_mutex_unlock", handlePthreadMutexUnlock, true),
  add("pthread_mutex_trylock", handlePthreadMutexTrylock, true),

  add("pthread_cond_init", handlePthreadCondInit, true),
  add("pthread_cond_destroy", handlePthreadCondDestroy, true),
  add("pthread_cond_wait", handlePthreadCondWait, true),
  add("pthread_cond_signal", handlePthreadCondSignal, true),
  add("pthread_cond_broadcast", handlePthreadCondBroadcast, true),
#undef addDNR
#undef add  
};

SpecialFunctionHandler::SpecialFunctionHandler(Executor &_executor) 
  : executor(_executor) {}


void SpecialFunctionHandler::prepare() {
  unsigned N = sizeof(handlerInfo)/sizeof(handlerInfo[0]);

  for (unsigned i=0; i<N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);
    
    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
  
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      // Make sure NoReturn attribute is set, for optimization and
      // coverage counting.
      if (hi.doesNotReturn)
        f->addFnAttr(Attribute::NoReturn);

      // Change to a declaration since we handle internally (simplifies
      // module and allows deleting dead code).
      if (!f->isDeclaration())
        f->deleteBody();
    }
  }
}

void SpecialFunctionHandler::bind() {
  unsigned N = sizeof(handlerInfo)/sizeof(handlerInfo[0]);

  for (unsigned i=0; i<N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);
    
    if (f && (!hi.doNotOverride || f->isDeclaration()))
      handlers[f] = std::make_pair(hi.handler, hi.hasReturnValue);
  }
}


bool SpecialFunctionHandler::handle(ExecutionState &state, 
                                    ThreadState &thr,
                                    Function *f,
                                    KInstruction *target,
                                    std::vector< ref<Expr> > &arguments) {
  handlers_ty::iterator it = handlers.find(f);
  if (it != handlers.end()) {    
    Handler h = it->second.first;
    bool hasReturnValue = it->second.second;
     // FIXME: Check this... add test?
    if (!hasReturnValue && !target->inst->use_empty()) {
      executor.terminateStateOnExecError(state, 
                                         "expected return value from void special function");
    } else {
      (this->*h)(state, thr, target, arguments);
    }
    return true;
  } else {
    return false;
  }
}

/****/

// reads a concrete string from memory
std::string 
SpecialFunctionHandler::readStringAtAddress(ExecutionState &state, 
                                            ref<Expr> addressExpr) {
  ObjectPair op;
  addressExpr = executor.toUnique(state, addressExpr);
  ref<ConstantExpr> address = cast<ConstantExpr>(addressExpr);
  if (!state.addressSpace.resolveOne(address, op))
    assert(0 && "XXX out of bounds / multiple resolution unhandled");
  bool res;
  assert(executor.solver->mustBeTrue(state, 
                                     EqExpr::create(address, 
                                                    op.first->getBaseExpr()),
                                     res) &&
         res &&
         "XXX interior pointer unhandled");
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;

  char *buf = new char[mo->size];

  unsigned i;
  for (i = 0; i < mo->size - 1; i++) {
    ref<Expr> cur = os->read8(i);
    cur = executor.toUnique(state, cur);
    assert(isa<ConstantExpr>(cur) && 
           "hit symbolic char while reading concrete string");
    buf[i] = cast<ConstantExpr>(cur)->getZExtValue(8);
  }
  buf[i] = 0;
  
  std::string result(buf);
  delete[] buf;
  return result;
}

/****/

void SpecialFunctionHandler::handleAbort(ExecutionState &state,
                           ThreadState &thr,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==0 && "invalid number of arguments to abort");

  //XXX:DRE:TAINT
  if(state.underConstrained) {
    std::cerr << "TAINT: skipping abort fail\n";
    executor.terminateState(state);
  } else {
    executor.terminateStateOnError(state, "abort failure", "abort.err");
  }
}

void SpecialFunctionHandler::handleExit(ExecutionState &state,
                           ThreadState &thr,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to exit");
  executor.terminateStateOnExit(state);
}

void SpecialFunctionHandler::handleSilentExit(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to exit");
  executor.terminateState(state);
}

void SpecialFunctionHandler::handleAliasFunction(ExecutionState &state,
             ThreadState &thr,
						 KInstruction *target,
						 std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 && 
         "invalid number of arguments to klee_alias_function");
  std::string old_fn = readStringAtAddress(state, arguments[0]);
  std::string new_fn = readStringAtAddress(state, arguments[1]);
  //std::cerr << "Replacing " << old_fn << "() with " << new_fn << "()\n";
  if (old_fn == new_fn)
    state.removeFnAlias(old_fn);
  else state.addFnAlias(old_fn, new_fn);
}

void SpecialFunctionHandler::handleAssert(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==3 && "invalid number of arguments to _assert");  
  
  //XXX:DRE:TAINT
  if(state.underConstrained) {
    std::cerr << "TAINT: skipping assertion:" 
               << readStringAtAddress(state, arguments[0]) << "\n";
    executor.terminateState(state);
  } else
    executor.terminateStateOnError(state, 
                                   "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
                                   "assert.err");
}

void SpecialFunctionHandler::handleAssertFail(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==4 && "invalid number of arguments to __assert_fail");
  
  //XXX:DRE:TAINT
  if(state.underConstrained) {
    std::cerr << "TAINT: skipping assertion:" 
               << readStringAtAddress(state, arguments[0]) << "\n";
    executor.terminateState(state);
  } else
    executor.terminateStateOnError(state, 
                                   "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
                                   "assert.err");
}

void SpecialFunctionHandler::handleReportError(ExecutionState &state,
                                               ThreadState &thr,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==4 && "invalid number of arguments to klee_report_error");
  
  // arguments[0], arguments[1] are file, line
  
  //XXX:DRE:TAINT
  if(state.underConstrained) {
    std::cerr << "TAINT: skipping klee_report_error:"
               << readStringAtAddress(state, arguments[2]) << ":"
               << readStringAtAddress(state, arguments[3]) << "\n";
    executor.terminateState(state);
  } else
    executor.terminateStateOnError(state, 
                                   readStringAtAddress(state, arguments[2]),
                                   readStringAtAddress(state, arguments[3]).c_str());
}

void SpecialFunctionHandler::handleMerge(ExecutionState &state,
                           ThreadState &thr,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  // nop
}

void SpecialFunctionHandler::handleNew(ExecutionState &state,
                         ThreadState &thr,
                         KInstruction *target,
                         std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to new");

  executor.executeAlloc(state, &thr, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDelete(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // FIXME: Should check proper pairing with allocation type (malloc/free,
  // new/delete, new[]/delete[]).

  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to delete");
  executor.executeFree(state, thr.id, arguments[0]);
}

void SpecialFunctionHandler::handleNewArray(ExecutionState &state,
                              ThreadState &thr,
                              KInstruction *target,
                              std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to new[]");
  executor.executeAlloc(state, &thr, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDeleteArray(ExecutionState &state,
                                 ThreadState &thr,
                                 KInstruction *target,
                                 std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to delete[]");
  executor.executeFree(state, thr.id, arguments[0]);
}

void SpecialFunctionHandler::handleMalloc(ExecutionState &state,
                                  ThreadState &thr,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to malloc");
  if (state.failMalloc) {
      state.mallocNum++;
      ref<Expr> equ = EqExpr::create(state.mallocFailCounter, ConstantExpr::create(state.mallocNum, Expr::Int32));
      Executor::StatePair failpair = executor.fork(state,
                                               thr.id,
                                               equ,
                                               true);

      if (failpair.first) {
          // fail!
          executor.bindLocal(target, thr, ConstantExpr::create(0, Expr::Int32));
          if (failpair.first != &state)
              executor.callChecker(*failpair.first, target, thr.id);
          failpair.first->failMalloc = false;
      }

      if (failpair.second) {
          executor.executeAlloc(*failpair.second, failpair.second->getThread(thr.id), arguments[0], false, target);
          if (failpair.second != &state)
              executor.callChecker(*failpair.second, target, thr.id);
      }
  } else {
      executor.executeAlloc(state, &thr, arguments[0], false, target);
  }
}

void SpecialFunctionHandler::handleAssume(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_assume");
  
  ref<Expr> e = arguments[0];
  
  if (e->getWidth() != Expr::Bool)
    e = NeExpr::create(e, ConstantExpr::create(0, e->getWidth()));
  
  bool res;
  bool success = executor.solver->mustBeFalse(state, e, res);
  assert(success && "FIXME: Unhandled solver failure");
  if (res) {
    executor.terminateStateOnError(state, 
                                   "invalid klee_assume call (provably false)",
                                   "user.err");
  } else {
    executor.addConstraint(state, e);
  }
}

void SpecialFunctionHandler::handleIsSymbolic(ExecutionState &state,
                                ThreadState &thr,
                                KInstruction *target,
                                std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_is_symbolic");

  executor.bindLocal(target, thr, 
                     ConstantExpr::create(!isa<ConstantExpr>(arguments[0]),
                                          Expr::Int32));
}

void SpecialFunctionHandler::handlePreferCex(ExecutionState &state,
                                             ThreadState &thr,
                                             KInstruction *target,
                                             std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_prefex_cex");

  ref<Expr> cond = arguments[1];
  if (cond->getWidth() != Expr::Bool)
    cond = NeExpr::create(cond, ConstantExpr::alloc(0, cond->getWidth()));

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, thr.id, arguments[0], rl, "prefex_cex");
  
  assert(rl.size() == 1 &&
         "prefer_cex target must resolve to precisely one object");

  rl[0].first.first->cexPreferences.push_back(cond);
}

void SpecialFunctionHandler::handlePrintExpr(ExecutionState &state,
                                  ThreadState &thr,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_print_expr");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  std::cerr << msg_str << ":" << arguments[1] << "\n";
}

void SpecialFunctionHandler::handleSetForking(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_set_forking");
  ref<Expr> value = executor.toUnique(state, arguments[0]);
  
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    state.forkDisabled = CE->isZero();
  } else {
    executor.terminateStateOnError(state, 
                                   "klee_set_forking requires a constant arg",
                                   "user.err");
  }
}

void SpecialFunctionHandler::handleStackTrace(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  state.dumpStack(std::cout);
}

void SpecialFunctionHandler::handleWarning(ExecutionState &state,
                                           ThreadState &thr,
                                           KInstruction *target,
                                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_warning");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning("%s: %s", thr.stack.back().kf->function->getName().data(), 
               msg_str.c_str());
}

void SpecialFunctionHandler::handleWarningOnce(ExecutionState &state,
                                               ThreadState &thr,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_warning_once");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning_once(0, "%s: %s", thr.stack.back().kf->function->getName().data(),
                    msg_str.c_str());
}

void SpecialFunctionHandler::handlePrintRange(ExecutionState &state,
                                  ThreadState &thr,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_print_range");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  std::cerr << msg_str << ":" << arguments[1];
  if (!isa<ConstantExpr>(arguments[1])) {
    // FIXME: Pull into a unique value method?
    ref<ConstantExpr> value;
    bool success = executor.solver->getValue(state, arguments[1], value);
    assert(success && "FIXME: Unhandled solver failure");
    bool res;
    success = executor.solver->mustBeTrue(state, 
                                          EqExpr::create(arguments[1], value), 
                                          res);
    assert(success && "FIXME: Unhandled solver failure");
    if (res) {
      std::cerr << " == " << value;
    } else { 
      std::cerr << " ~= " << value;
      std::pair< ref<Expr>, ref<Expr> > res =
        executor.solver->getRange(state, arguments[1]);
      std::cerr << " (in [" << res.first << ", " << res.second <<"])";
    }
  }
  std::cerr << "\n";
}

void SpecialFunctionHandler::handleGetObjSize(ExecutionState &state,
                                  ThreadState &thr,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_get_obj_size");
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, thr.id, arguments[0], rl, "klee_get_obj_size");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    executor.bindLocal(target, *it->second->getThread(thr.id), 
                       ConstantExpr::create(it->first.first->size, Expr::Int32));
  }
}

void SpecialFunctionHandler::handleGetErrno(ExecutionState &state,
                                            ThreadState &thr,
                                            KInstruction *target,
                                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==0 &&
         "invalid number of arguments to klee_get_errno");
  executor.bindLocal(target, thr,
                     ConstantExpr::create(errno, Expr::Int32));
}

void SpecialFunctionHandler::handleCalloc(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==2 &&
         "invalid number of arguments to calloc");

  ref<Expr> size = MulExpr::create(arguments[0],
                                   arguments[1]);
  executor.executeAlloc(state, &thr, size, false, target, true);
}

void SpecialFunctionHandler::handleRealloc(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==2 &&
         "invalid number of arguments to realloc");
  ref<Expr> address = arguments[0];
  ref<Expr> size = arguments[1];

  Executor::StatePair zeroSize = executor.fork(state,
                                               thr.id,
                                               Expr::createIsZero(size), 
                                               true);
  
  if (zeroSize.first) { // size == 0
    executor.executeFree(*zeroSize.first, thr.id, address, target);   
    if (zeroSize.first != &state) {
        executor.callChecker(*zeroSize.first, target, thr.id);
    }
  }
  if (zeroSize.second) { // size != 0
    Executor::StatePair zeroPointer = executor.fork(*zeroSize.second, 
                                                    thr.id,
                                                    Expr::createIsZero(address), 
                                                    true);
    
    if (zeroPointer.first) { // address == 0
      executor.executeAlloc(*zeroPointer.first, zeroPointer.first->getThread(thr.id), size, false, target);
      if (zeroPointer.first != &state) {
          executor.callChecker(*zeroPointer.first, target, thr.id);
      }
    } 
    if (zeroPointer.second) { // address != 0
      Executor::ExactResolutionList rl;
      executor.resolveExact(*zeroPointer.second, thr.id, address, rl, "realloc");
      
      for (Executor::ExactResolutionList::iterator it = rl.begin(), 
             ie = rl.end(); it != ie; ++it) {
        executor.executeAlloc(*it->second, it->second->getThread(thr.id), size, false, target, false, 
                              it->first.second);
        if (&(*it->second) != &state) {
            executor.callChecker(*it->second, target, thr.id);
        }
      }
    }
  }
}

void SpecialFunctionHandler::handleFree(ExecutionState &state,
                          ThreadState &thr,
                          KInstruction *target,
                          std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 &&
         "invalid number of arguments to free");
  executor.executeFree(state, thr.id, arguments[0]);
}

void SpecialFunctionHandler::handleCheckMemoryAccess(ExecutionState &state,
                                                     ThreadState &thr,
                                                     KInstruction *target,
                                                     std::vector<ref<Expr> > 
                                                       &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_check_memory_access");

  ref<Expr> address = executor.toUnique(state, arguments[0]);
  ref<Expr> size = executor.toUnique(state, arguments[1]);
  if (!isa<ConstantExpr>(address) || !isa<ConstantExpr>(size)) {
    executor.terminateStateOnError(state, 
                                   "check_memory_access requires constant args",
                                   "user.err");
  } else {
    ObjectPair op;

    if (!state.addressSpace.resolveOne(cast<ConstantExpr>(address), op)) {
      executor.terminateStateOnError(state,
                                     "check_memory_access: memory error",
                                     "ptr.err",
                                     executor.getAddressInfo(state, address));
    } else {
      ref<Expr> chk = 
        op.first->getBoundsCheckPointer(address, 
                                        cast<ConstantExpr>(size)->getZExtValue());
      if (!chk->isTrue()) {
        executor.terminateStateOnError(state,
                                       "check_memory_access: memory error",
                                       "ptr.err",
                                       executor.getAddressInfo(state, address));
      }
    }
  }
}

void SpecialFunctionHandler::handleGetValue(ExecutionState &state,
                                            ThreadState &thr,
                                            KInstruction *target,
                                            std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_get_value");

  executor.executeGetValue(state, thr, arguments[0], target);
}

void SpecialFunctionHandler::handleDefineFixedObject(ExecutionState &state,
                                                     ThreadState &thr,
                                                     KInstruction *target,
                                                     std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[0]) &&
         "expect constant address argument to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[1]) &&
         "expect constant size argument to klee_define_fixed_object");
  
  uint64_t address = cast<ConstantExpr>(arguments[0])->getZExtValue();
  uint64_t size = cast<ConstantExpr>(arguments[1])->getZExtValue();
  MemoryObject *mo = executor.memory->allocateFixed(address, size, thr.prevPC->inst);
  executor.bindObjectInState(state, NULL, mo, false);
  mo->isUserSpecified = true; // XXX hack;
}

void SpecialFunctionHandler::handleMakeSymbolic(ExecutionState &state,
                                                ThreadState &thr,
                                                KInstruction *target,
                                                std::vector<ref<Expr> > &arguments) {
  std::string name;

  // FIXME: For backwards compatibility, we should eventually enforce the
  // correct arguments.
  if (arguments.size() == 2) {
    name = "unnamed";
  } else {
    // FIXME: Should be a user.err, not an assert.
    assert(arguments.size()==3 &&
           "invalid number of arguments to klee_make_symbolic");  
    name = readStringAtAddress(state, arguments[2]);
  }

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, thr.id, arguments[0], rl, "make_symbolic");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    mo->setName(name);
    
    const ObjectState *old = it->first.second;
    ExecutionState *s = it->second;
    
    if (old->readOnly) {
      executor.terminateStateOnError(*s, 
                                     "cannot make readonly object symbolic", 
                                     "user.err");
      return;
    } 

    // FIXME: Type coercion should be done consistently somewhere.
    bool res;
    bool success =
      executor.solver->mustBeTrue(*s, 
                                  EqExpr::create(ZExtExpr::create(arguments[1],
                                                                  Context::get().getPointerWidth()),
                                                 mo->getSizeExpr()),
                                  res);
    assert(success && "FIXME: Unhandled solver failure");
    
    if (res) {
      executor.executeMakeSymbolic(*s, mo, name);
    } else {      
      executor.terminateStateOnError(*s, 
                                     "wrong size given to klee_make_symbolic[_name]", 
                                     "user.err");
    }
  }
}

void SpecialFunctionHandler::handleMarkGlobal(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_mark_global");  

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, thr.id, arguments[0], rl, "mark_global");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    assert(!mo->isLocal);
    mo->isGlobal = true;
  }
}

void SpecialFunctionHandler::handlePthreadCreate(ExecutionState &state,
                                              ThreadState &thr,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
    ConstantExpr *thread_func = dyn_cast<ConstantExpr>(arguments[2]);

    Value *tfunc = (Value *)thread_func->getZExtValue();
    Function *f = executor.getTargetFunction(tfunc, state);
    assert(f->arg_size() == 1);

    KFunction *kf = executor.kmodule->functionMap[f];

    int tid = state.createThread(kf);

    if (executor.statsTracker)
        executor.statsTracker->framePushed(state, 0, tid);

    for (unsigned i = 0, e = f->arg_size(); i != e; i++)
        executor.bindArgument(kf, i, *state.getThread(tid), arguments[3]);
}

uint64_t* SpecialFunctionHandler::getRealArgs(ExecutionState &state,
                                          std::vector<ref<Expr> > &arguments)
{
    // BEGIN: copy from callExternalFunction()

    // normal external function handling path
    // allocate 128 bits for each argument (+return value) to support fp80's;
    // we could iterate through all the arguments first and determine the exact
    // size we need, but this is faster, and the memory usage isn't significant.
    uint64_t *args = (uint64_t*) malloc(2*sizeof(*args) * (arguments.size() + 1));
    memset(args, 0, 2 * sizeof(*args) * (arguments.size() + 1));
    unsigned wordIndex = 2;
    for (std::vector<ref<Expr> >::iterator ai = arguments.begin(), 
            ae = arguments.end(); ai!=ae; ++ai) {
        if (false) { //AllowExternalSymCalls) { // don't bother checking uniqueness
            ref<ConstantExpr> ce;
            bool success = executor.solver->getValue(state, *ai, ce);
            assert(success && "FIXME: Unhandled solver failure");
            (void) success;
            ce->toMemory(&args[wordIndex]);
            wordIndex += (ce->getWidth()+63)/64;
        } else {
            ref<Expr> arg = executor.toUnique(state, *ai);
            if (ConstantExpr *ce = dyn_cast<ConstantExpr>(arg)) {
                // XXX kick toMemory functions from here
                ce->toMemory(&args[wordIndex]);
                wordIndex += (ce->getWidth()+63)/64;
            } else {
                return false;
            }
        }
    }

    state.addressSpace.copyOutConcretes();

    // END: copy from callExternalFunction()
    return args;
}

uint64_t SpecialFunctionHandler::getMutexArg(ExecutionState &state,
                                          std::vector<ref<Expr> > &arguments)
{
    uint64_t *args = getRealArgs(state, arguments);
    uint64_t mutex = args[2];
    delete[]args;
    return mutex;
}

uint64_t SpecialFunctionHandler::getCondArg(ExecutionState &state,
                                          std::vector<ref<Expr> > &arguments)
{
    uint64_t *args = getRealArgs(state, arguments);
    uint64_t cond = args[2];
    delete[]args;
    return cond;
}

void SpecialFunctionHandler::getCondArgWithMutex(ExecutionState &state,
                                          std::vector<ref<Expr> > &arguments,
                                          uint64_t& cond,
                                          uint64_t& mutex)
{
    uint64_t *args = getRealArgs(state, arguments);
    cond = args[2];
    mutex = args[4];
    delete[]args;
}

void SpecialFunctionHandler::handlePthreadMutexInit(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t mutex = getMutexArg(state, arguments);
    if (state.mutexes.find(mutex) != state.mutexes.end())
    {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EBUSY, Expr::Int32));
    } else {
        state.mutexes.insert(std::pair<uint64_t, MutexInfo>(mutex, MutexInfo()));
        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    }
}
void SpecialFunctionHandler::handlePthreadMutexDestroy(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t mutex = getMutexArg(state, arguments);
    if (state.mutexes.find(mutex) != state.mutexes.end())
    {
        if (state.mutexes[mutex].acquired)
        {
            executor.bindLocal(target, thr,
                    ConstantExpr::create(EBUSY, Expr::Int32));
        } else {
            state.mutexes.erase(mutex);
            executor.bindLocal(target, thr,
                    ConstantExpr::create(0, Expr::Int32));
        }
    } else {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }

}

void SpecialFunctionHandler::doMutexLock(ExecutionState &state,
                                          int tid,
                                          MutexInfo &m_info)
{
    if (m_info.acquired)
    {
        m_info.waiters.push(tid);
        state.blockedThreads.insert(tid);
    } else {
        m_info.acquired = true;
        m_info.owner = tid;
    }
}

void SpecialFunctionHandler::doMutexUnlock(ExecutionState &state,
                                          MutexInfo &m_info)
{
    if (!m_info.waiters.empty())
    {
        // all or one?
        int tid = m_info.waiters.front();
        m_info.waiters.pop();

        state.blockedThreads.erase(tid);

        m_info.owner = tid;
    } else {
        m_info.acquired = false;
    }
}

void SpecialFunctionHandler::handlePthreadMutexLock(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t mutex = getMutexArg(state, arguments);
    if (state.mutexes.find(mutex) != state.mutexes.end())
    {
        MutexInfo &m_info = state.mutexes[mutex];
        if (m_info.acquired)
        {
            if (m_info.owner == thr.id)
            {
                executor.bindLocal(target, thr,
                        ConstantExpr::create(EDEADLK, Expr::Int32));
                return;
            }
        }
        doMutexLock(state, thr.id, m_info);
        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    } else {
        std::cerr << "Error! Locking uninited mutex!" << std::endl;
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }
}

void SpecialFunctionHandler::handlePthreadMutexUnlock(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t mutex = getMutexArg(state, arguments);
    if (state.mutexes.find(mutex) != state.mutexes.end())
    {
        MutexInfo &m_info = state.mutexes[mutex];
        if (m_info.acquired)
        {
            if (m_info.owner != thr.id)
            {
                executor.bindLocal(target, thr,
                        ConstantExpr::create(EPERM, Expr::Int32));
                return;
            }
            doMutexUnlock(state, m_info);
            executor.bindLocal(target, thr,
                     ConstantExpr::create(0, Expr::Int32));
        } else {
            std::cerr << "Error! Unlocking an unlocked mutex!" << std::endl;
            executor.bindLocal(target, thr,
                    ConstantExpr::create(EPERM, Expr::Int32));
        }
    } else {
        std::cerr << "Error! Locking uninited mutex!" << std::endl;
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }
}
void SpecialFunctionHandler::handlePthreadMutexTrylock(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t mutex = getMutexArg(state, arguments);
    if (state.mutexes.find(mutex) != state.mutexes.end())
    {
        MutexInfo &m_info = state.mutexes[mutex];
        if (m_info.acquired)
        {
            // return fail
            executor.bindLocal(target, thr,
                     ConstantExpr::create(EBUSY, Expr::Int32));
        } else {
            doMutexLock(state, thr.id, m_info);
            // return succ
            executor.bindLocal(target, thr,
                     ConstantExpr::create(0, Expr::Int32));

        }
    } else {
        std::cerr << "Error! Trylocking uninited mutex!" << std::endl;
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }

}
void SpecialFunctionHandler::handlePthreadCondInit(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t cond = getCondArg(state, arguments);
    if (state.conds.find(cond) != state.conds.end())
    {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EBUSY, Expr::Int32));
    } else {
        state.conds.insert(std::pair<uint64_t, CondInfo>(cond, CondInfo()));
        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    }

}
void SpecialFunctionHandler::handlePthreadCondDestroy(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t cond = getCondArg(state, arguments);
    if (state.conds.find(cond) != state.conds.end())
    {
        if (!state.conds[cond].waiters.empty())
        {
            executor.bindLocal(target, thr,
                    ConstantExpr::create(EBUSY, Expr::Int32));
        } else {
            state.conds.erase(cond);
            executor.bindLocal(target, thr,
                    ConstantExpr::create(0, Expr::Int32));
        }
    } else {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }
}

void SpecialFunctionHandler::handlePthreadCondWait(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t cond, mutex;
    getCondArgWithMutex(state, arguments, cond, mutex);
    if (state.conds.find(cond) != state.conds.end()
            && state.mutexes.find(mutex) != state.mutexes.end())
    {
        MutexInfo &mi = state.mutexes[mutex];
        CondInfo &ci = state.conds[cond];
        if (!mi.acquired || !mi.owner == thr.id)
        {
            executor.bindLocal(target, thr,
                    ConstantExpr::create(EPERM, Expr::Int32));
            return;
        }
        if (ci.binded != 0 && ci.binded != mutex)
        {
            executor.bindLocal(target, thr,
                    ConstantExpr::create(EINVAL, Expr::Int32));
            return;
        }
        ci.binded = mutex;
        // Should be atomic!
        doMutexUnlock(state, mi);
        ci.waiters.push(thr.id);

        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    } else {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }

}
void SpecialFunctionHandler::handlePthreadCondSignal(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t cond = getCondArg(state, arguments);
    if (state.conds.find(cond) != state.conds.end())
    {
        CondInfo &ci = state.conds[cond];
        if (!ci.waiters.empty())
        {
            int tid = ci.waiters.front();
            ci.waiters.pop();
            MutexInfo &mi = state.mutexes[ci.binded];
            doMutexLock(state, tid, mi);
        }
        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    } else {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }
}
void SpecialFunctionHandler::handlePthreadCondBroadcast(ExecutionState &state,
                                          ThreadState &thr,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
    uint64_t cond = getCondArg(state, arguments);
    if (state.conds.find(cond) != state.conds.end())
    {
        CondInfo &ci = state.conds[cond];
        while (!ci.waiters.empty())
        {
            int tid = ci.waiters.front();
            ci.waiters.pop();
            MutexInfo &mi = state.mutexes[ci.binded];
            doMutexLock(state, tid, mi);
        }
        executor.bindLocal(target, thr,
                ConstantExpr::create(0, Expr::Int32));
    } else {
        executor.bindLocal(target, thr,
                ConstantExpr::create(EINVAL, Expr::Int32));
    }
}

void SpecialFunctionHandler::handleNotifyCounter(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  ref<Expr> counter = arguments[0];
  if (dyn_cast<ConstantExpr>(counter)) {
      std::cerr << "warning: klee_notify_counter() with constant" << std::endl;
  }
  std::string name = readStringAtAddress(state, arguments[1]);
  std::cerr << "Got counter notification: " << name << std::endl;
  if (name == "malloc_fail") {
      state.mallocFailCounter = counter;
      state.failMalloc = true;
      state.mallocNum = 0;
  }
}

void SpecialFunctionHandler::handleSysOpen(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  std::string path = readStringAtAddress(state, arguments[0]);
  int flags = readInt(state, arguments[1]);
  mode_t mode = readInt(state, arguments[2]);

  int fd = state.resMgr.openFile(path.c_str(), flags, mode);
  executor.bindLocal(target, thr,
          ConstantExpr::create((uint32_t)fd, Expr::Int32));

}

void SpecialFunctionHandler::handleSysClose(ExecutionState &state,
                            ThreadState &thr,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  int fd = readInt(state, arguments[0]);
  int ret = state.resMgr.closeFile(fd);
  executor.bindLocal(target, thr,
          ConstantExpr::create((uint32_t)ret, Expr::Int32));
}

uint64_t SpecialFunctionHandler::readInt(ExecutionState &state, ref<Expr> argument) {
  ObjectPair op;
  argument = executor.toUnique(state, argument);
  ref<ConstantExpr> expr = cast<ConstantExpr>(argument);
  return expr->getZExtValue();
}
