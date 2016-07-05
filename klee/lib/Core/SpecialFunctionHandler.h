//===-- SpecialFunctionHandler.h --------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SPECIALFUNCTIONHANDLER_H
#define KLEE_SPECIALFUNCTIONHANDLER_H

#include <map>
#include <vector>
#include <string>

namespace llvm {
  class Function;
}

namespace klee {
  class Executor;
  class Expr;
  class ExecutionState;
  class ThreadState;
  struct MutexInfo;
  struct KInstruction;
  template<typename T> class ref;
  
  class SpecialFunctionHandler {
  public:
    typedef void (SpecialFunctionHandler::*Handler)(ExecutionState &state,
                                                    ThreadState &thr,
                                                    KInstruction *target, 
                                                    std::vector<ref<Expr> > 
                                                      &arguments);
    typedef std::map<const llvm::Function*, 
                     std::pair<Handler,bool> > handlers_ty;

    handlers_ty handlers;
    class Executor &executor;

  public:
    SpecialFunctionHandler(Executor &_executor);

    /// Perform any modifications on the LLVM module before it is
    /// prepared for execution. At the moment this involves deleting
    /// unused function bodies and marking intrinsics with appropriate
    /// flags for use in optimizations.
    void prepare();

    /// Initialize the internal handler map after the module has been
    /// prepared for execution.
    void bind();

    bool handle(ExecutionState &state, 
                ThreadState &thr,
                llvm::Function *f,
                KInstruction *target,
                std::vector< ref<Expr> > &arguments);

    /* Convenience routines */

    std::string readStringAtAddress(ExecutionState &state, ref<Expr> address);
    uint64_t readInt(ExecutionState &state, ref<Expr> argument);
    uint64_t* getRealArgs(ExecutionState &state, std::vector<ref<Expr> > &arguments);
    uint64_t getMutexArg(ExecutionState &state, std::vector<ref<Expr> > &arguments);
    uint64_t getCondArg(ExecutionState &state, std::vector<ref<Expr> > &arguments);
    void getCondArgWithMutex(ExecutionState &state, std::vector<ref<Expr> > &arguments, 
            uint64_t& cond, uint64_t& mutex);


    void doMutexLock(ExecutionState &state, int tid, MutexInfo &m_info);
    void doMutexUnlock(ExecutionState &state, MutexInfo &m_info);

    /* Handlers */

#define HANDLER(name) void name(ExecutionState &state, \
                                ThreadState &thr, \
                                KInstruction *target, \
                                std::vector< ref<Expr> > &arguments)
    HANDLER(handleAbort);
    HANDLER(handleAssert);
    HANDLER(handleAssertFail);
    HANDLER(handleAssume);
    HANDLER(handleCalloc);
    HANDLER(handleCheckMemoryAccess);
    HANDLER(handleDefineFixedObject);
    HANDLER(handleDelete);    
    HANDLER(handleDeleteArray);
    HANDLER(handleExit);
    HANDLER(handleAliasFunction);
    HANDLER(handleFree);
    HANDLER(handleGetErrno);
    HANDLER(handleGetObjSize);
    HANDLER(handleGetValue);
    HANDLER(handleIsSymbolic);
    HANDLER(handleMakeSymbolic);
    HANDLER(handleMalloc);
    HANDLER(handleMarkGlobal);
    HANDLER(handleMerge);
    HANDLER(handleNew);
    HANDLER(handleNewArray);
    HANDLER(handlePreferCex);
    HANDLER(handlePrintExpr);
    HANDLER(handlePrintRange);
    HANDLER(handleRange);
    HANDLER(handleRealloc);
    HANDLER(handleReportError);
    HANDLER(handleRevirtObjects);
    HANDLER(handleSetForking);
    HANDLER(handleSilentExit);
    HANDLER(handleStackTrace);
    HANDLER(handleUnderConstrained);
    HANDLER(handleWarning);
    HANDLER(handleWarningOnce);

    HANDLER(handleNotifyCounter);
    HANDLER(handleSysOpen);
    HANDLER(handleSysClose);

    HANDLER(handlePthreadCreate);

    HANDLER(handlePthreadMutexInit);
    HANDLER(handlePthreadMutexDestroy);
    HANDLER(handlePthreadMutexLock);
    HANDLER(handlePthreadMutexUnlock);
    HANDLER(handlePthreadMutexTrylock);

    HANDLER(handlePthreadCondInit);
    HANDLER(handlePthreadCondDestroy);
    HANDLER(handlePthreadCondWait);
    HANDLER(handlePthreadCondSignal);
    HANDLER(handlePthreadCondBroadcast);
#undef HANDLER
  };
} // End klee namespace

#endif
