#include<Grammar.h>
#include <boost/spirit/include/qi_attr.hpp>

void scriptGrammar::startRules() {
    using boost::spirit::qi::attr;

    program      %= *global;
    global       %= (funcDecl | varDecl | structDecl) > -lit(';');
    funcDecl     %= keyword(string("func")) > typeOrStruct[_a = _1]
                    > structName(_a) > arrayDimensions > identifier
                    > '(' > -(varDecl % ',') > ')' > statementBlock;
    structDecl   %= keyword(string("struct")) > identifier > '{'
                    > *(varDecl > ';') > '}';

    program.name("script");
    global.name("global declaration");
    funcDecl.name("function declaration");
}
