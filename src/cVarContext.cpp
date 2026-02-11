#include "cVarContext.h"
#include<cVariable.h>

cVarContext::cVarContext()
{
    enterScope();
}

cVarContext::~cVarContext()
{
    //cVarContext may be destructed on runtime errors before exiting all blocks
    while(!scopeDeclStack.empty()) {
        exitScope();
    }
}

//***************************
//  Declare / Undeclare
//***************************

void cVarContext::declare(const std::string &name, const TValue &val) {
    cVariable *var = new cVariable(val, name);

    if (!declare(name, TReferencable(var))) {
        delete var;
        throw runtimeError;
    }
}

void cVarContext::declareRef(const std::string &name, const TValueRef &ref) {
    if (!declare(name, TReferencable(ref))) {
        throw runtimeError;
    }
}

bool cVarContext::declare(const std::string &name, TReferencable ref) {
    if(hasRef(name)) {
        LOG_ERROR("Variable \"" + name + "\" already defined.");
        return false;
    }

    symbols.insert(std::pair<string, TReferencable>(name, ref));
    scopeDeclStack.push(name);
    return true;
}

//***************************
// enter / exit
//***************************

const char SCOPE_START_TOK[] = "%SCOPE_START%";

void cVarContext::enterScope() {
    this->scopeDeclStack.push(SCOPE_START_TOK);
}

void cVarContext::exitScope() {
    if(scopeDeclStack.empty()) {
        FAULT("Exiting block that was not entered.");
    }

    std::string currDecl = scopeDeclStack.top();
    while(currDecl != SCOPE_START_TOK) {
        unDeclare(currDecl);
        scopeDeclStack.pop();

        ASSERT(!scopeDeclStack.empty(), "Missing SCOPE_START_TOK?!");

        currDecl = scopeDeclStack.top();
    }

    scopeDeclStack.pop();
}

void cVarContext::unDeclare(const std::string &name) {
    TSymbolTable::iterator it = symbols.find(name);

    ASSERT(it != symbols.end(), "Undeclaring variable that was not declared: \"" + name + "\"?!");

    cVariable **var;
    if((var = boost::get<cVariable*>(&it->second))) {
        delete *var;
    }

    symbols.erase(it);
}

//***************************
// Reset to template
//***************************

void cVarContext::resetToTemplate(const std::string &name) {
    TSymbolTable::iterator it = symbols.find(name);
    ASSERT(it != symbols.end(), "Trying to reset non-existing variable to template state!");
    cVariable ** var = boost::get<cVariable*>(&it->second);
    ASSERT(var, "Trying to reset something to template that is not a variable!");
    delete *var;
    *var = new cVariable(_template(), name);
}

TValueRef cVarContext::getRef(const std::string &name) const {//throws
    TMaybeRef res = hasRef(name);
    if (!res) {
        RUNTIME_ERROR("Undefined variable or field: \"" + name + "\".");
    }
    return *res;
}

TMaybeRef cVarContext::hasRef(const std::string &name) const { //returns null
    TSymbolTable::const_iterator it = symbols.find(name);

    if (it != symbols.end()) {
        const TValueRef *ref = boost::get<TValueRef>(&it->second);

        if(ref) {
            return *ref;
        } else {
            return boost::get<cVariable*>(it->second)->getValueRef();
        }
    }
    return TMaybeRef(); //nothing
}
