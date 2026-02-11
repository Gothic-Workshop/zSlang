#ifndef CLOGGER_H
#define CLOGGER_H
#include<string>
#include<windows.h>

using std::string;

class cLogger {
    public:
        virtual ~cLogger() { };
        virtual void debuginfo(const string &str) {}
        virtual void info(const string &str)      {}
        virtual void warn(const string &str)      {}
        virtual void error(const string &str)     {}
        virtual void fault(const string &str)     {}
        virtual void indent();
        virtual void unindent();
        virtual void commit() {}; /* assure that this was all that was to log for the moment
                                   * can be used for flushing buffers, writing logs to memory
                                   * or scrolling in the GUI-View */

        static cLogger* getLogger();
        static void replaceLogger(cLogger *newLogger);

        static bool logDebug;
        static bool logInfo;
        static bool logWarn;
        static bool logError;
        static void setLogDebug(bool b) { logDebug = b; }
        static void setLogInfo (bool b) { logInfo  = b; }
        static void setLogWarn (bool b) { logWarn  = b; }
        static void setLogError(bool b) { logError = b; }
    private:
        static cLogger* logger;
        static cLogger nullLogger;
    protected:
        int indentation;
        cLogger() { indentation = 0; }
};
/*
class cGUILogger : public cLogger {
    public:
        cGUILogger(wxTextCtrl *logctrl);
        virtual ~cGUILogger() { };

        virtual void debuginfo(const string &str);
        virtual void info(const string &str);
        virtual void warn(const string &str);
        virtual void error(const string &str);
        virtual void fault(const string &str);
        virtual void commit(); //scroll

        virtual void addLine(wxColour col, const string &str, const string &header = "");
        virtual void add    (wxColour col, const string &str, const string &header = "");
    protected:
    private:
        wxTextCtrl * logctrl;
        bool isStartOfLine;
};     */

class czSpyLogger : public cLogger {
    public:
        czSpyLogger();
        virtual ~czSpyLogger() {};
        virtual void debuginfo(const string &str);
        virtual void info(const string &str);
        virtual void warn(const string &str);
        virtual void error(const string &str);
        virtual void fault(const string &str);
    private:
        void log(const string &str, const string &header = "");
        HWND SpyHandle;
};

class cConsoleLogger : public cLogger {
    public:
        cConsoleLogger() {};
        virtual ~cConsoleLogger() { };
        virtual void debuginfo(const string &str);
        virtual void info(const string &str);
        virtual void warn(const string &str);
        virtual void error(const string &str);
        virtual void fault(const string &str);
    private:
        void log(const string &str, const string &header = "");
};

class cLoggerIndentation {
    public:
        cLoggerIndentation();
        ~cLoggerIndentation();
};

using std::exception;

class cParseError: public exception {
    virtual const char* what() const throw() {
        return "Error during parsing.";
    }
};
extern cParseError parseError;

class cSektisFault: public exception {
    virtual const char* what() const throw() {
        return "Internal Error. It's Sekti's fault.";
    }
};
extern cSektisFault sektisFault;

class cGeneralFault: public exception {
    virtual const char* what() const throw() {
        return "General Error occured.";
    }
};
extern cGeneralFault generalFault;

class cRuntimeError: public exception {
    virtual const char* what() const throw() {
        return "Error while executing the script.";
    }
};
extern cRuntimeError runtimeError;

class cAssertionFailed: public exception {
    virtual const char* what() const throw() {
        return "Assertion failed. This is Sekti's fault.";
    }
};
extern cAssertionFailed assertionFailed;

class cGeneralError: public exception {
    virtual const char* what() const throw() {
        return "General Error occured. ";
    }
};
extern cGeneralError generalError;

#include <sstream>

inline string FloatToStr(double f) {
    std::stringstream ss;
    ss << f;
    return ss.str();
}
inline string IntToStr(int i) {
    std::stringstream ss;
    ss << i;
    return ss.str();
}

inline void nop(void) {};
/* ASSERTIONS */
#if 1
    #define ASSERT(cond, err)\
                if (!(cond)) {\
                    cLogger::getLogger()->error(err);\
                    throw assertionFailed; \
                }
#else
    #define ASSERT(cond, err) nop()
#endif

#ifdef __DEBUG__
    #define ONDEBUG(d) d
#else
    #define ONDEBUG(d)
#endif

/* LOGGING */
#define INDENTLOG cLoggerIndentation _logIndent

#ifdef __DEBUG__
    #define DEBUGINFO(s) cLogger::getLogger()->debuginfo(s)
#else
    #define DEBUGINFO(s) nop()
#endif

#define INFO(s) cLogger::getLogger()->info(s)
#define WARN(s) cLogger::getLogger()->warn(s)
#define LOG_ERROR(s) cLogger::getLogger()->error(s)
#define GENERAL_ERROR(s) cLogger::getLogger()->error(s);\
                         throw generalError
#define PARSE_ERROR(s) cLogger::getLogger()->error(s);\
                       throw parseError
#define RUNTIME_ERROR(s) cLogger::getLogger()->error(s);\
                       throw runtimeError
#define FAULT(s) cLogger::getLogger()->fault(s); \
                 throw sektisFault
#define FAULT_ONCE(s) \
    static bool failed_before = false;\
    if (!failed_before) { \
        cLogger::getLogger()->fault(s);\
    }\
    throw sektisFault

#endif // CLOGGER_H
