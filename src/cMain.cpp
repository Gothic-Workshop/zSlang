#include <cMain.h>
#include <zWorld.h>
#include <cSelection.h>
#include <cProperty.h>
#include <oScriptParser.h>
#include <cInterpreter.h>
#include<zTransformApp.h>
#include <cLogger.h>
#include <cPreprocessor.h>

cMain::cMain() {
}

cMain::~cMain() {
}

void cMain::killItWithFire(const string &flames) {
    LOG_ERROR(flames + "Serious Fault");
    std::terminate();
}

string cMain::getApplicationName() {
    return "zSlangInterpreter";
}

string cMain::getVersionString() {
    return "1.0";
}

void cMain::printHello() const {
    INFO(string("Hello, this is ")
                + this->getApplicationName() + " "
                + this->getVersionString()
                + ", compiled on " + __DATE__ + ", " + __TIME__".");
}

void cMain::init() {
    printHello();
}

void cMain::runScript(const string &fPath) {
    INFO("Running script " + fPath + "...");
    try {
        {
            INDENTLOG;
            string preprocessInput = cPreprocessor::preprocess(fPath);
            oScriptParser parser(preprocessInput);
            cInterpreter  interp(parser.getAST());
            interp.callFunction("main");
        }
        INFO("Execution finished normally.");
    } catch (cSektisFault &f) {
        LOG_ERROR ("This was not supposed to happen. Tell Sekti about it.");
    } catch (cParseError &e) {
        LOG_ERROR ("There have been parsing errors.");
    } catch (cGeneralFault &e) {
        LOG_ERROR ("Unexpected error.");
    } catch (cRuntimeError &e) {
        LOG_ERROR("Runtime Error encountered.");
    } catch (cGeneralError &e) {
        LOG_ERROR("General Error occured.");
    } catch (std::exception &e) {
        LOG_ERROR(string("Unhandled exception encountered: ") + e.what());
    } catch (...) {
        LOG_ERROR ("Oh, dear.");
    }
}
