#include<Grammar.h>

#include <boost/spirit/home/phoenix/container.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_eps.hpp>
#include <boost/spirit/include/qi_attr.hpp>

zTypes::zTypes() {
    this->name("typename");

    for(int t = TYPE_VOID; t < TYPE_ARRAY; ++t) {
        add(typeNames[t], TType(t));
    }
}

zTypes typeSymbol;

const char *typeNames[TYPE_MAX] = {
    "void",
    "int",
    "float",
    "string",
    "selection",
    "object",
    "template",
    "function",
    "struct",
    "array",
};

void scriptGrammar::terminalRules() {
    using boost::spirit::ascii::alnum;
    using boost::spirit::ascii::alpha;
    using boost::spirit::eps;
    using boost::spirit::qi::attr;

    using phoenix::construct;

    keyword       = lexeme[lit(_r1) >> !(alnum | '_')];

    constant     %= (int_ >> !char_('.')) | double_ | quotedString;
    quotedString %= lexeme['"' > *(char_ - '"') > '"'];

    idString     %= lexeme[(alpha | char_("_")) >> *(alnum | char_("_"))];
    identifier   %= idString;

    varOrRef      = keyword(string("var"))[_val = false] | keyword(string("ref"))[_val = true];

    typeOrStruct  = typeSymbol[_val = _1] | eps[_val = TYPE_STRUCT];
    structName    = eps(_r1 != TYPE_STRUCT) | identifier;
    varDecl      %= varOrRef > typeOrStruct[_a = _1] > structName(_a)
                    > identifier > arrayDimensions > -('=' > expression);
    arrayDimension   = '[' > (-expression)[_val = _1] > ']';
    arrayDimensions %= *(arrayDimension);
    simpleExp    %= ('(' > expression > ')') | expressionList | identifier | constant; //constants after identifier because double_ matches identifiers like "inf" as well.
    expressionList %= '{' > -(expression % ',') > '}';

    keyword.name("keyword");
    constant.name("constant");
    quotedString.name("string constant");
    identifier.name("identifier");
    idString.name("identifier");
    varDecl.name("variable declaration");
    simpleExp.name("simple expression");
    expressionList.name("expression list");
}
