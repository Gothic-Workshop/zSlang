#include "cVarStack.h"
#include<cLogger.h>

#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH

#define INSERT(s, v) insert(std::pair<std::string, TReferencable>(s, v))

cVarStack::cVarStack() {
    local = 0;
}

//***************************
//  Declaration
//***************************

void cVarStack::declareGlobal(const std::string &name, const TValue &val) {
    global.declare(name, val);
}

void cVarStack::declareScoped(const std::string &name, const TValue &val) {
    if (!local) {
        FAULT("There is no local context to declare variables in!");
    }
    local->declare(name, val);
}

void cVarStack::declareScopedRef   (const std::string &name, const TValueRef &ref) {
    if (!local) {
        FAULT("There is no local context to declare variables in!");
    }
    local->declareRef(name, ref);
}

//***************************
//  Context Pushing / Poping
//***************************

void cVarStack::enterFunction(const std::string &funcName, cVarContext *context) {
    localContextStack.push(local);
    local = context;
    callStack.push(funcName);
}
void cVarStack::exitFunction() {
    ASSERT(!localContextStack.empty(), "Trying to pop local context but stack is empty!?");
    local = localContextStack.top();
    localContextStack.pop();
    callStack.pop();
}

void cVarStack::enterScope() {
    ASSERT(local, "Cannot enter scope because there is no local context!");
    local->enterScope();
}
void cVarStack::exitScope() {
    ASSERT(local, "Cannot enter scope because there is no local context!");
    local->exitScope();
}

//***************************
//  Getting Vars
//***************************

TValueRef cVarStack::getRetVarRef() {
    return global.getRef(callStack.top() + RET_VAR_SUFFIX);
}

bool cVarStack::hasRetVar() {
    return global.hasRef(callStack.top() + RET_VAR_SUFFIX);
}

void cVarStack::declareRetVar(const TValue &defaultVal) {
    declareGlobal(callStack.top() + RET_VAR_SUFFIX, defaultVal);
}

void cVarStack::resetRetVarToTemplate() {
    global.resetToTemplate(callStack.top() + RET_VAR_SUFFIX);
}

TValueRef cVarStack::getRef(const std::string &name) const {//throws
    TMaybeRef res;
    if (local) {
        res = local->hasRef(name);
    }

    return res ? *res : global.getRef(name);
}

TMaybeRef cVarStack::hasRef(const std::string &name) const {//throws
    TMaybeRef res;
    return (local && (res = local->hasRef(name))) ? res : global.hasRef(name);
}

cVarStack::~cVarStack() {
    /* pop local contexts (should not be nessessary, though...?) */
    while (!localContextStack.empty()) {
        exitFunction();
    }
}
