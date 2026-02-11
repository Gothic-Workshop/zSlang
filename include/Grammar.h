#ifndef GRAMMAR_H
#define GRAMMAR_H

#include<oScriptParser.h>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/home/phoenix/statement/sequence.hpp>
#include <boost/spirit/home/phoenix/statement/if.hpp>

#include <boost/spirit/home/phoenix/bind/bind_function.hpp>
#include <boost/spirit/home/phoenix/object/construct.hpp>

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

using qi::lit;
using qi::lexeme;
using qi::on_error;
using qi::fail;
using namespace qi::labels;

using qi::int_;
using qi::double_;
using ascii::char_;
using qi::lit;

using phoenix::val;
using phoenix::ref;

struct zTypes : qi::symbols<char, TType> {
    zTypes();
};
extern zTypes typeSymbol;

struct zOps  : qi::symbols<char, TOperator> {
    zOps(TOpGroup group);
};
extern zOps opsDot, opsUnary, opsMul, opsAdd, opsTest, opsSel, opsAssign;

///////////////////////////////////////////////////////////////////////////
//  Grammar
///////////////////////////////////////////////////////////////////////////
//[tutorial_xml2_grammar
typedef std::string::const_iterator strIt;
struct scriptGrammar
  : qi::grammar<strIt, TProgram(), ascii::space_type>
{
    /* split into different files for faster compilation time */
    void terminalRules();
    void operatorRules();
    void startRules();
    void miscRules();
    void errorHandlers();

    scriptGrammar()
      : scriptGrammar::base_type(program)
    {
        hadErrors = false;
        terminalRules();
        operatorRules();
        startRules();
        miscRules();

        errorHandlers();
    }

    #define RULE(type) qi::rule<strIt, type, ascii::space_type>
    #define RULE_LOCALS(type, locals) qi::rule<strIt, type, locals, ascii::space_type>
    RULE(void(std::string)) keyword;
    RULE(TConstant()) constant;
    RULE(TStringConst()) quotedString;
    RULE(std::string()) idString;
    RULE(TIdentifier()) identifier;
    RULE(std::string()) idSub;
    RULE(TStatements()) statements;
    RULE(TStatements()) statementBlock;
    RULE(TDanglingBlock()) danglingBlock;
    RULE(TStatement()) delimitedStatement;
    RULE(TStatement()) undelimitedStatement;
    RULE(TStatement()) statement;
    RULE(bool())            varOrRef;
    RULE_LOCALS(TVarDeclaration(), qi::locals<TType>) varDecl;
    RULE(TType())                                     typeOrStruct;
    RULE(string(TType))          structName;
    RULE(TOptExp()) arrayDimension;
    RULE(TArrayDimensions()) arrayDimensions;
    RULE(TProgram()) program;
    RULE(TGlobal()) global;
    RULE_LOCALS(TFuncDeclaration(), qi::locals<TType>) funcDecl;
    RULE(TStructDecl())      structDecl;
    RULE(TReturnStatement()) returnStatement;
    RULE(TIfStatement())     ifStatement;
    RULE(TElseBlock())       elseBlock;
    RULE(TForStatement())    forStatement;
    RULE(TWhileStatement())  whileStatement;
    RULE(TForeachStatement())foreachStatement;
    RULE(TBreakStatement())  breakStatement;
    RULE(TContinueStatement())  continueStatement;
    RULE(TExpressionList())  expressionList;
    RULE(TExpression())  expression,
                         assignmentCalc,
                         logicCalc,
                         selCalc,
                         addCalc,
                         mulCalc,
                         testCalc,
                         dotCalc,
                         unaryCalc,
                         nested,
                         simpleExp;
    RULE(std::vector<TExpression>()) paramList;

    //I got to help boost::qi out here:
    RULE(TUnaryCalc())   unaryCalcSub;

    std::stringstream errorInfoStream;
    std::string::const_iterator errorPos;
    bool hadErrors;

    #undef RULE
    #undef RULE_LOCALS

    #define ERROR_HANDLER(rule)\
    on_error<fail> (rule,\
                        phoenix::if_(!phoenix::ref(hadErrors))[\
                            (errorInfoStream << val("Expecting "),\
                             errorInfoStream << _4,\
                             errorInfoStream << val(" while parsing <"),\
                             errorInfoStream << phoenix::ref(rule.name()),\
                             errorInfoStream << val(">"),\
                             phoenix::ref(errorPos) = _3,\
                             phoenix::ref(hadErrors) = true)\
                        ].else_[\
                            (errorInfoStream << val(", while parsing <"),\
                             errorInfoStream << phoenix::ref(rule.name()),\
                             errorInfoStream << val(">"))\
                        ]\
                    );
};

#endif //GRAMMAR_H
