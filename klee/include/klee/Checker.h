#ifndef KLEE_CHECKER_H
#define KLEE_CHECKER_H

#include "klee/Expr.h"
#include <string>

namespace llvm {
    class BranchInst;
    class CallInst;
    class Instruction;
    class CallSite;
    class Function;
}

namespace klee {
    class ExecutionState;
    class ThreadState;
    class KInstruction;
    class Executor;

    class Checker {
protected:
        Executor *_executor;

        std::string getStrFromCharPtr(ref<Expr> charptr, ExecutionState &state);
        uint64_t getIntFromExp(ref<Expr> exp);
        uint64_t getPtrFromPPtr(ref<Expr> pptr, ExecutionState &state);
        llvm::Function *getTargetFunctionFromCaller(KInstruction *caller, ExecutionState &state);
        ref<Expr> getCallingArgument(KInstruction *caller, int number, ThreadState &thr);

public:
        typedef uint64_t Result;
        // bitmask:
        // last bit: error=1
        // second bit: important = 1

        enum ResultType {
            OK = 0x0,
            ERROR = 0x1,
            IMPORTANT = 0x2,
            NEED_RET  = 0x4,
            NEED_ARG0 = 0x8,
            NEED_ARG1 = 0x10,
            NEED_ARG2 = 0x20,
            NEED_ARG3 = 0x40,
            NEED_ARG4 = 0x80,
            NEED_OTHER_ARGS = 0xFFFFFFFFFFFFFFFFll
                - ((1 << 8) - 1),
            NEED_ALL_ARGS = 0xFFFFFFFFFFFFFFFFll
                - ERROR - IMPORTANT
        };

        static const int NEED_ARG_BEGIN = 3;
        static const int NEED_ARG_END = 62;

        // on Events
#define DECLARE_EVENT_HANDLER(name, capname) virtual Checker::Result name(ExecutionState &state, \
        ThreadState &thr, CHECKER_##capname##_ARGUMENTS_WITH_TYPE)
#define DECLARE_PROP_HANDLER(name, capname) virtual bool name(ExecutionState &state, \
        ThreadState &thr, CHECKER_##capname##_ARGUMENTS_WITH_TYPE)
#define DECLARE_STATIC_PROP_HANDLER(name, capname) virtual bool name(CHECKER_##capname##_ARGUMENTS_WITH_TYPE)
#define CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE const std::string& funcname
#define CHECKER_MAYBEEVENT_ARGUMENTS funcname
        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT) {
            return false;
        }

#define CHECKER_ONCALL_ARGUMENTS_WITH_TYPE KInstruction *target, llvm::Function *f,  \
        std::vector< ref<Expr> > &arguments, uint64_t *args
#define CHECKER_ONCALL_ARGUMENTS target, f, arguments, args

        DECLARE_EVENT_HANDLER(onCall, ONCALL) {
            return OK;
        }

#define CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE KInstruction *target, llvm::Function *f,  \
        std::vector< ref<Expr> > &arguments
#define CHECKER_WANTCONCRETE_ARGUMENTS target, f, arguments

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE) {
            return false;
        }

#define CHECKER_ONRET_ARGUMENTS_WITH_TYPE KInstruction *caller, ref<Expr> result
#define CHECKER_ONRET_ARGUMENTS caller, result

        DECLARE_EVENT_HANDLER(onRet, ONRET) {
            return OK;
        }

#define CHECKER_ONBRANCH_ARGUMENTS_WITH_TYPE llvm::BranchInst *bi, KInstruction *inst, \
                ref<Expr> cond, ExecutionState *trueBranch, ExecutionState *falseBranch
#define CHECKER_ONBRANCH_ARGUMENTS bi, inst, cond, trueBranch, falseBranch
        DECLARE_EVENT_HANDLER(onBranch, ONBRANCH) {
            return OK;
        }

#define CHECKER_REPORT_ARGUMENTS_WITH_TYPE ExecutionState *state
#define CHECKER_REPORT_ARGUMENTS state
        virtual Result report(ExecutionState *state) { return OK; }

        // Constructor & destructor
        virtual ~Checker() {}

        virtual Checker* clone() = 0;

        Checker(Executor *executor) {
            _executor = executor;
        }

    };

}

#endif // KLEE_CHECKER_H
