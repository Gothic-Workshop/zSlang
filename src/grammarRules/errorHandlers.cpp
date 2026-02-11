#include<Grammar.h>

void scriptGrammar::errorHandlers() {
    //recursive
    ERROR_HANDLER(expression);
    ERROR_HANDLER(selCalc);
    ERROR_HANDLER(testCalc);
    ERROR_HANDLER(addCalc);
    ERROR_HANDLER(mulCalc);
    ERROR_HANDLER(unaryCalc);
    ERROR_HANDLER(unaryCalcSub);
    ERROR_HANDLER(nested);
    ERROR_HANDLER(assignmentCalc);

    //start rules
    ERROR_HANDLER(global);
    ERROR_HANDLER(funcDecl);

    //terminals
    ERROR_HANDLER(quotedString);
    ERROR_HANDLER(varDecl);
    ERROR_HANDLER(simpleExp);

    //statement
    ERROR_HANDLER(statementBlock);
    ERROR_HANDLER(statement);
    ERROR_HANDLER(ifStatement);
    ERROR_HANDLER(elseBlock);
    ERROR_HANDLER(forStatement);
    ERROR_HANDLER(whileStatement);
    ERROR_HANDLER(paramList);
}

