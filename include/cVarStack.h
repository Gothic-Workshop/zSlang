#ifndef CVARSTACK_H
#define CVARSTACK_H

#include<cVariable.h>
#include<string>
#include<stack>
#include<map>
#include<cVarContext.h>

const char RET_VAR_SUFFIX[] = "::result";

class cVarStack
{
    public:
        cVarStack();
        ~cVarStack();

        void declareGlobal(const std::string &name, const TValue &val);
        void declareScoped(const std::string &name, const TValue &val);
        void declareScopedRef(const std::string &name, const TValueRef &ref);
        void enterFunction(const std::string &funcName, cVarContext *context);
        void exitFunction ();

        void enterScope();
        void exitScope();

        void resetRetVarToTemplate(); //makes a fresh uninitialised template
        bool hasRetVar(); //may not be created yet
        void declareRetVar(const TValue &defaultVal);

        TValueRef getRetVarRef();
        TValueRef getRef (const std::string &name) const; //throws

        TMaybeRef hasRef(const std::string &name) const; //returns null
    private:
        typedef std::stack<std::string> TCallStack;

        cVarContext global, *local;
        TCallStack callStack;

        std::stack<cVarContext*>  localContextStack;
};

#endif // CVARSTACK_H
