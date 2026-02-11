#include "cLogger.h"
#include "zTransformApp.h"

#include<iostream>
#include<sstream>

/* global Exceptions for everyone free to throw */
cParseError parseError;
cSektisFault sektisFault;
cGeneralFault generalFault;
cRuntimeError runtimeError;
cAssertionFailed assertionFailed;
cGeneralError generalError;

//***************************
//  Null Logger / Generic Logger
//***************************

bool cLogger::logDebug = true,
     cLogger::logInfo  = true,
     cLogger::logWarn  = true,
     cLogger::logError = true;

cLogger* cLogger::logger;
cLogger  cLogger::nullLogger;

cLogger* cLogger::getLogger() {
    if(logger) {
        return logger;
    } else {
        return &nullLogger;
    }
}

void cLogger::replaceLogger(cLogger* newLogger) {
    if (logger != &nullLogger) {
        delete logger;
    }
    logger = newLogger;
    if (logger == 0) {
        logger = &nullLogger;
    }
}


//***************************
// GUI Logger
//***************************

//this also specifies the maximum indentation
/*
string cLogger::spaces = "                              ";
#define MAX_LINELENGTH 500

cGUILogger::cGUILogger(wxTextCtrl * logctrl) {
    this->logctrl = logctrl;
    indentation = 0;
    isStartOfLine = true;
}

void cGUILogger::debuginfo(const string &str) {
    if (logDebug)
        addLine(*wxBLACK, str, "DEBUGINFO: ");
}

void cGUILogger::info(const string &str) {
    if (logInfo)
        addLine(*wxBLACK, str, "INFO: ");
}

void cGUILogger::warn(const string &str) {
    if (logWarn)
        addLine(*wxGREEN, str, "WARNING: ");
}

void cGUILogger::error(const string &str) {
    if (logError)
        addLine(*wxRED, str, "ERROR: ");
}

void cGUILogger::fault(const string &str) {
    wxMessageBox(str, string("Serious Fault"));
    error(str);
}

void cGUILogger::add(wxColour col, const string &str, const string &header) {
    wxTextAttr style = logctrl->GetDefaultStyle();
    style.SetTextColour(col);
    logctrl->SetDefaultStyle(style);

    std::string indent = spaces.substr(0, 2*indentation);
    std::stringstream ss(str);

    bool firstLine = true;

    char buf[MAX_LINELENGTH];
    while(ss.good()) {
        ss.getline(buf,MAX_LINELENGTH);

        std::string line = "";

        if (isStartOfLine) {
            line = indent;
            if (firstLine) {
                line += header;
                firstLine = false;
            } else {
                line += string(header.length(), ' ');
            }
        }

        line += buf;
        if(ss.good()) { // good and going on (\n could be extracted)
            line += '\n';
            isStartOfLine = true;
        } else if (strlen(buf)) {
            // the string ends before a newline is reached
            isStartOfLine = false;
        } else {
            break; // nothing left to print
        }

        logctrl->AppendText(line);
    }
}

void cGUILogger::addLine(wxColour col, const string &str, const string &header) {
    add(col, str + '\n', header);
}

void cGUILogger::commit() {
    //scroll:
    logctrl->ShowPosition(logctrl->GetLastPosition()); //all the way down
    logctrl->PageUp(); //the last page should be visible
    logctrl->Thaw();    //repaint
}      */

//***************************
//  Console Logger
//***************************

void cConsoleLogger::debuginfo(const string &str) {
    if (logDebug)
        log(str, "DEBUGINFO: ");
}
void cConsoleLogger::info(const string &str) {
    if (logInfo)
        log(str, "INFO: ");
}
void cConsoleLogger::warn(const string &str) {
    if (logWarn)
        log(str, "WARN: ");
}
void cConsoleLogger::error(const string &str) {
    if (logError)
        log(str, "ERROR: ");
}
void cConsoleLogger::fault(const string &str) {
    log("FAULT: " + str);
}
void cConsoleLogger::log(const string &str, const string &header) {
    using namespace std;
    std::string indent(2*indentation, ' ');

    bool firstLine = true;

    stringstream ss(str);
    string buf;
    while(getline(ss, buf)) {
        cout << indent << (firstLine ? header : string(header.length(), ' ')) << buf << endl;
        firstLine = false;
    }
}

//***************************
//  zSpy Logger
//***************************

czSpyLogger::czSpyLogger() {
    SpyHandle = FindWindow(0,"[zSpy]");

    if (!SpyHandle) {
        HINSTANCE ret =
        ShellExecute(0, "open", (zTransformApp::getAPP()->getInterpreterDir() + "\\_intern\\zSpy.exe").c_str(),
                    0,
                    0,
                    SW_SHOW);

        if (ret <= (HINSTANCE)32) {
            MessageBox(0,
                    "Could not start zSpy for logging! Make sure the zSpy is placed in \\intern\\zSpy.exe!",
                    "Error starting zSpy",
                    MB_OK
            );
        } else while (0 == (SpyHandle = FindWindow(0,"[zSpy]")));
    }

    unsigned WM_LOGCOMMAND	= RegisterWindowMessage("WM_LOGCOMMAND");
    string str("START");
	ATOM atomStart = GlobalAddAtom(str.c_str());
	str = "SHOW";
	ATOM atomShow  = GlobalAddAtom(str.c_str());
	SendMessage(SpyHandle,WM_LOGCOMMAND,0,(LPARAM)atomStart );
	SendMessage(SpyHandle,WM_LOGCOMMAND,0,(LPARAM)atomShow );
	GlobalDeleteAtom(atomStart);
	GlobalDeleteAtom(atomShow);
}

void czSpyLogger::debuginfo(const string &str) {
    if (logDebug)
        log(str, "DEBUGINFO: ");
}
void czSpyLogger::info(const string &str) {
    if (logInfo)
        log(str, "Info:  ");
}
void czSpyLogger::warn(const string &str) {
    if (logWarn)
        log(str, "Warn:  ");
}
void czSpyLogger::error(const string &str) {
    if (logError)
        log(str, "Fault: ");
}
void czSpyLogger::fault(const string &str) {
    log("Fatal:  " + str);
}
void czSpyLogger::log(const string &str, const string &header) {
    using namespace std;
    std::string indent(2*indentation, ' ');

    stringstream ss(str);
    string buf;
    while(getline(ss, buf)) {
        stringstream out;
        out << indent << header << buf << endl;

        string message = out.str();
        COPYDATASTRUCT data;
        data.lpData = (void*)message.c_str();
        data.cbData = message.length() + 1;

        SendMessage(SpyHandle,WM_COPYDATA,(WPARAM)0,(LPARAM)&data);
    }
}

//***************************
//  Indentation
//***************************

void cLogger::indent() {
    indentation ++;
}

void cLogger::unindent() {
    indentation --;
    ASSERT(indentation >= 0, "Indentation < 0");
}

cLoggerIndentation::cLoggerIndentation() {
    cLogger::getLogger()->indent();
}

cLoggerIndentation::~cLoggerIndentation() {
    cLogger::getLogger()->unindent();
}
