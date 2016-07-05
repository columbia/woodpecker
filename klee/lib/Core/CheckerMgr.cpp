#include "klee/CheckerMgr.h"
#include <iostream>

using namespace klee;

#define CALL_CHECKERS_VOID(x) \
    Checker::Result final_ret = Checker::OK; \
    for (unsigned int i=0; i<checkers.size(); i++) \
    { \
        Checker::Result ret = checkers[i]->x(state); \
        final_ret |= ret; \
    } \
    \
    return final_ret;


#define CALL_CHECKERS(name, capname) \
    Checker::Result final_ret = Checker::OK; \
    for (unsigned int i=0; i<checkers.size(); i++) \
    { \
        Checker::Result ret = checkers[i]->name(state, thr, CHECKER_##capname##_ARGUMENTS); \
        final_ret |= ret; \
    } \
    \
    return final_ret;

#define GET_BOOL_PROP(name, capname) \
    bool final_ret = false; \
    for (unsigned int i=0; i<checkers.size(); i++) \
    { \
        bool ret = checkers[i]->name(state, thr, CHECKER_##capname##_ARGUMENTS); \
        final_ret |= ret; \
    } \
    \
    return final_ret;

Checker::Result CheckerMgr::onBranch(ExecutionState &state,
                ThreadState &thr,
                CHECKER_ONBRANCH_ARGUMENTS_WITH_TYPE)
{
    CALL_CHECKERS(onBranch, ONBRANCH)
}

Checker::Result CheckerMgr::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE)
{
    CALL_CHECKERS(onCall, ONCALL)
}

Checker::Result CheckerMgr::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE)
{
    CALL_CHECKERS(onRet, ONRET)
}

bool CheckerMgr::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    GET_BOOL_PROP(wantConcrete, WANTCONCRETE);
}

Checker::Result CheckerMgr::report(ExecutionState *state)
{
    CALL_CHECKERS_VOID(report);
}

void CheckerMgr::addChecker(Checker* checker)
{
    checkers.push_back(checker);
}

CheckerMgr::~CheckerMgr()
{
    for (unsigned int i=0; i<checkers.size(); i++)
    {
        delete checkers[i];
    }
}

CheckerMgr::CheckerMgr(const CheckerMgr& other)
{
    checkers.resize(other.checkers.size());

    for (unsigned int i=0; i<checkers.size(); i++)
    {
        checkers[i] = other.checkers[i]->clone();
    }
    
}

CheckerMgr::CheckerMgr()
{
}
