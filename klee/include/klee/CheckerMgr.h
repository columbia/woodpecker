#ifndef KLEE_CHECKERMGR_H
#define KLEE_CHECKERMGR_H

#include <vector>

#include "klee/Checker.h"

namespace klee {
    class CheckerMgr {
private:
        std::vector<Checker*> checkers;

public:
        // macro defined in Checker.h
        DECLARE_EVENT_HANDLER(onBranch, ONBRANCH);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        DECLARE_EVENT_HANDLER(onRet, ONRET);

        Checker::Result report(ExecutionState *state);
        void addChecker(Checker* checker);

        void clear() { checkers.clear(); }

        ~CheckerMgr();
        CheckerMgr(const CheckerMgr& other);
        CheckerMgr();

    };

}

#endif // KLEE_CHECKERMGR_H

