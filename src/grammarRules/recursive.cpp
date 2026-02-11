#include<Grammar.h>
#include<cLogger.h>

zOps opsDot(OPS_DOT),
     opsUnary(OPS_UNARY),
     opsMul(OPS_MUL),
     opsAdd(OPS_ADD),
     opsTest(OPS_TEST),
     opsSel(OPS_SEL),
     opsLogic(OPS_LOGIC),
     opsAssign(OPS_ASSIGN),
     opsEvil(OPS_EVIL);

zOps::zOps(TOpGroup group) {
    this->name("operator");

    switch (group) {
        case OPS_DOT:
            add(".", OP_DOT); break;
        case OPS_UNARY:
            add("!", OP_NOT)("-", OP_UNARYMINUS)("+", OP_UNARYPLUS); break;
        case OPS_MUL:
            add("*", OP_MUL)("/", OP_DIV)("%", OP_MOD); break;
        case OPS_ADD:
            add("+", OP_ADD)("-", OP_SUB); break;
        case OPS_TEST:
            add("==", OP_EQ)("!=", OP_NEQ)
               ("<=", OP_LEQ)(">=",OP_GEQ)(">", OP_GT)("<",OP_LT)
               ("~=", OP_MATCH); break;
        case OPS_SEL:
            add("|", OP_FILTER); break;
        case OPS_LOGIC:
            add("&&", OP_AND)
               ("||", OP_OR); break;
        case OPS_ASSIGN:
            add ("=", OP_ASSIGN)
                ("+=", OP_ADDEQ)("-=", OP_SUBEQ)("*=", OP_MULEQ)("/=", OP_DIVEQ)
                ("|=", OP_FILTEREQ)("&&=", OP_ANDEQ)("||=", OP_OREQ)
            ; break;
        case OPS_EVIL:
            //for guys like me who tend to write ++i in for loops and
            //fail to remember that this is interpreted as two unary plus operators.
            add("++", OP_NONE)("--", OP_NONE); break;
        default:
            FAULT("zOps: Invalid Operator Group.");
    }
}

const char *opNames[OP_MAX] = {
    "???",
    ".",
    "[]",
    "!", "-", "+",
    "*", "/", "%",
    "+", "-",
    "!=", "~=", "==", "<", "<=", ">", ">=",
    "|",
    "&&", "||",
    "=", "+=", "-=", "*=", "/=", "|=", "&&=", "||="
};

void scriptGrammar::operatorRules() {
    #define MAKETREE_LEFT(next, ops) \
        next[_val = _1] >>\
        *((ops > next)[_val = phoenix::construct<TBinaryCalc>(_val, _1, _2)])
    #define MAKETREE_RIGHT(this, next, ops) \
        next[_val = _1] >>\
        -(ops > this)[_val = phoenix::construct<TBinaryCalc>(_val, _1, _2)]

    expression    = assignmentCalc.alias();
    assignmentCalc= MAKETREE_RIGHT(expression, logicCalc, (opsAssign - opsTest - opsEvil));
    logicCalc     = MAKETREE_LEFT(selCalc  , opsLogic - opsAssign);
    selCalc       = MAKETREE_LEFT(testCalc , (opsSel  - opsAssign - opsLogic) );
    testCalc      = MAKETREE_LEFT(addCalc  , opsTest);
    addCalc       = MAKETREE_LEFT(mulCalc  , (opsAdd - opsAssign - opsEvil) );
    mulCalc       = MAKETREE_LEFT(unaryCalc, (opsMul - opsAssign - opsEvil) );
    unaryCalc    %= unaryCalcSub | nested;
    unaryCalcSub %= opsUnary > unaryCalc;
    nested = simpleExp[_val = _1] > *(
                        ('[' > expression > ']')
                        [_val = phoenix::construct<TBinaryCalc>(_val, OP_INDEX, _1)]
                    |
                        (opsDot > identifier)
                        [_val = phoenix::construct<TNested>(_val, _2)]
                    |
                        (paramList)
                        [_val = phoenix::construct<TFuncCall>(_val, _1)]
                    );

    expression.name("expression");
    assignmentCalc.name("assignment");
    selCalc.name("filter expression");
    testCalc.name("operand");
    addCalc.name("addent");
    mulCalc.name("factor");
    unaryCalc.name("operand");
    unaryCalcSub.name("operand");
    nested.name("nested identifier");

    #undef MAKETREE_LEFT
    #undef MAKETREE_RIGHT
}
