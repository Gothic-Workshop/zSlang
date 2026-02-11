#include<Grammar.h>
#include <boost/spirit/include/qi_eps.hpp>

void scriptGrammar::miscRules() {
    using boost::spirit::eps;

    statements      %= *statement;
    statementBlock  %= '{' > statements > '}';
    danglingBlock   %= statementBlock;  //needed for recursive wrapping
    /* semicolon is optional in most cases */
    delimitedStatement %= (
                                returnStatement |
                                varDecl         | //because it can have in initialiser
                                expression
                          ) > lit(';');
    undelimitedStatement = (
                                danglingBlock     |
                                breakStatement    |
                                continueStatement |
                                ifStatement       |
                                forStatement      |
                                foreachStatement  |
                                whileStatement
                        )[_val = _1] > -lit(';');
    statement       %=  undelimitedStatement | delimitedStatement;

    returnStatement    %= keyword(string("return")) > -expression;
    breakStatement     %= keyword(string("break"));
    continueStatement  %= keyword(string("continue"));

    ifStatement     %= keyword(string("if")) > expression > statementBlock > elseBlock;
    elseBlock        = (keyword(string("else")) > (ifStatement[_val = _1] | statementBlock[_val = _1]) ) | eps/*keep the default constructed boost::blank*/;

    forStatement    %= keyword(string("for")) > '(' > -varDecl > ';' > -expression > ';' > -expression > ')' > statementBlock;
    foreachStatement%= keyword(string("foreach")) > identifier > keyword(string("in")) > expression > statementBlock;
    whileStatement  %= keyword(string("while")) > expression > statementBlock;

    paramList    %= '(' > (-(expression % ',')) > ')';

    statement.name("statement");
    statementBlock.name("statement-block");
    statements.name("statements");

    returnStatement.name("return statement");
    ifStatement.name("if-Statement");
    elseBlock.name("else-branch");

    forStatement.name("for-loop");
    whileStatement.name("while-loop");
    paramList.name("parameter list");
}


