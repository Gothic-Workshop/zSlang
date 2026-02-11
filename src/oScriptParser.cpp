#include<oScriptParser.h>
#include<cLogger.h>
#include<TCode_Printer.h>

#include <fstream>
#include <iostream>

//***************************
// include the Grammar here
//***************************

#include<Grammar.h>

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////

void oScriptParser::parseFile(const std::string &fname)
{
    INFO(std::string("ScriptParser compiled on ") + __DATE__ + ", " + __TIME__".");

    std::string storage; // We will read the contents here.
    INFO("Reading file: \"" + fname + "\"...");
    {
        // ****************************
        //reading the file to memory
        // ****************************
        std::ifstream in(&fname[0], std::ios_base::in);

        if (!in) {
            PARSE_ERROR("Failed to open file: \""+ fname + '"');
        }

        in.unsetf(std::ios::skipws); // No white space skipping!
        std::copy(
            std::istream_iterator<char>(in),
            std::istream_iterator<char>(),
            std::back_inserter(storage));

        if(!storage.length()) {
            PARSE_ERROR("File \"" + fname + "\" is empty.");
        }
    } INFO("DONE. Parsing...");
    cLogger::getLogger()->commit();
    {
        INDENTLOG;
        scriptGrammar grammar; // Our grammar

        using ascii::space;
        strIt iter = storage.begin();
        strIt end  = storage.end();
        bool r = phrase_parse(iter, end, grammar, space, ast);

        if (!r || iter != end) {
            if (!grammar.hadErrors) {
                grammar.errorPos = iter; //if it did not really fail
            }

            strIt contextStart = storage.begin(), contextEnd = storage.end();

            /* print the context of the error.
             * try to use beginning and ending of lines and
             * stick to a certain context size */
            const int contextSize = 20; /* TODO: Make this configurable */

            if (grammar.errorPos - contextStart > contextSize) {
                contextStart = grammar.errorPos - contextSize;

                while ((contextStart != storage.begin())
                && (*contextStart != '\n')) {
                    --contextStart;
                }

                if (*contextStart == '\n') {
                    ++contextStart;
                }
            }
            if (contextEnd - grammar.errorPos > contextSize) {
                contextEnd = grammar.errorPos + contextSize;

                while((contextEnd != storage.end())
                && (*contextEnd != '\n')) {
                    ++contextEnd;
                }
            }

            string beforeError(contextStart, grammar.errorPos);
            string afterError (grammar.errorPos, contextEnd);

            if(!grammar.hadErrors) { //actual failure with message.
                LOG_ERROR("Unrecognized input:");
            } else {
                LOG_ERROR(grammar.errorInfoStream.str());
            }

            INDENTLOG;
            //cLogger *logger = cLogger::getLogger();

            /*if (cGUILogger * guiLogger = dynamic_cast<cGUILogger*>(logger)) {
                guiLogger->add    (*wxGREEN, beforeError);
                guiLogger->addLine(*wxRED,   afterError );
            } else {*/
                string::iterator linebreak = find(beforeError.rbegin(), beforeError.rend(), '\n').base();
                WARN(beforeError);
                LOG_ERROR(string(beforeError.end() - linebreak, ' ') + afterError);
            //}

            throw parseError;
        }

        //INFO(TCode_Printer(true)(ast).getString());
    }
    INFO("Parsing succeeded.");
    cLogger::getLogger()->commit();
}

