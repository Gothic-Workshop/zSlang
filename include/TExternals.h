#ifndef TEXTERNALS_H
#define TEXTERNALS_H

#include<vector>
#include<hScriptTypes.h>

class cInterpreter;

struct TExternals
{
    public:
        static void addExternal(const TFuncDeclaration &decl) {
            externals.push_back(decl);
        }
        static void registerExternals(cInterpreter *interpreter);
    protected:
    private:
        TExternals() {};
        static std::vector<TFuncDeclaration> externals;
};

#endif // TEXTERNALS_H
