#ifndef CVARCONTEXT_H
#define CVARCONTEXT_H

#include<string>
#include<map>
#include<stack>
#include<hScriptTypes.h>
#include<cValueOrRef.h>

typedef boost::optional<TValueRef> TMaybeRef;

class cVarContext
{
        public:
        cVarContext();
        ~cVarContext();

        void declare   (const std::string &name, const TValue &val);
        void declareRef(const std::string &name, const TValueRef &ref);

        void enterScope();
        void exitScope();

        void resetToTemplate(const std::string &name); //makes a fresh uninitialised template
        TValueRef getRef (const std::string &name) const; //throws
        TMaybeRef hasRef (const std::string &name) const;
    private:
        typedef boost::variant<cVariable*, TValueRef> TReferencable;
        typedef std::map<std::string,TReferencable> TSymbolTable;

        bool declare   (const std::string &name, TReferencable ref);
        void unDeclare (const std::string &name);

        TSymbolTable symbols;
        std::stack<std::string> scopeDeclStack;
};

#endif // CVARCONTEXT_H
