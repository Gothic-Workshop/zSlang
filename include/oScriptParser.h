#ifndef OSCRIPTPARSER_H
#define OSCRIPTPARSER_H

#include<string>
#include<vector>

#include<hScriptTypes.h>

class oScriptParser
{
    public:
        oScriptParser(const std::string &fname) {
            parseFile(fname);
        };
        ~oScriptParser() {};
        TProgram &getAST() { return ast; }
    protected:
    private:
        void parseFile(const std::string &fname);
        TProgram ast;
};

#endif // OSCRIPTPARSER_H
