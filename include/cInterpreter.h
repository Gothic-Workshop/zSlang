#ifndef CINTERPRETER_H
#define CINTERPRETER_H

#include<hScriptTypes.h>
#include<cVariable.h>
#include<cVarStack.h>
#include<cVarContext.h>
#include<map>
#include<string>

class zWorld;

class cInterpreter
{
    public:
        cInterpreter(TProgram &program);
        ~cInterpreter();
        void callMain() { callFunction("main"); };
        cValueOrRef callFunction(
                    const std::string &name,
                    const TParamList &params = TParamList());
        cValueOrRef callFunction(
                    TFuncDeclaration *decl,
                    const TParamList &params = TParamList());
        void define(const std::string &str, const TValue &val);
        void registerFunc(TFuncDeclaration *decl);

        TMaybeRef hasVar(const std::string &name) {
            return varStack.hasRef(name);
        }
        const TValue& getVarValue(const std::string &name) {
            return varStack.getRef(name).getValue();
        }
        const TValueRef getVarRef(const std::string &name) {
            return varStack.getRef(name);
        }
        void cleanRetVal() {
            if (currentFunctionHasTemplateReturnValue) {
                varStack.resetRetVarToTemplate();
            }
        }

        zWorld * getWorld() { return world; }
        void destroyWorld();
        void loadWorld(const std::string &path);
        TValue createDefaultValue(TValue val);
    protected:
    private:
        void introduceDefines();

        TValue createDefaultValue(const TVarDeclaration &decl, bool ignoreRefFlag = false);
        TValue createDefaultValue(const TFuncDeclaration &decl);
        TValue createDefaultValue(TType type, const string &structName, const TArrayDimensions &dim = TArrayDimensions());

        typedef std::map<std::string, TFuncDeclaration*> TFuncTable;
        TFuncTable functions;
        typedef std::map<std::string, TStructDecl*> TStructTable;
        TStructTable structs;

        cVarStack varStack;
        TProgram &program;

        zWorld *world;

        struct TGlobalVisitor;
        struct TToBoolVisitor;
        struct TExpressionVisitor;
        struct TStatementVisitor;

        bool currentFunctionHasTemplateReturnValue;

        const TFuncDeclaration& findFunction(const std::string &name) const;

        friend struct TStatementVisitor;
        friend struct TGlobalVisitor;

/*      Feste Werte von außen?
        Return Statement?
        Wie aus einer Kaskade von Blöcken returnen?

        default value for vectors!
        */
};


#endif // CINTERPRETER_H
