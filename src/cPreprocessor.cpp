#include "cPreprocessor.h"
#include <zTransformApp.h>
#include <sstream>
#include <cLogger.h>
#include <algorithm>
#include <fstream>

using std::stringstream;

/*
string cPreprocessor::backSlashToSlash(string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
} */

string  cPreprocessor::preprocess(const string &fileName) {
    //***************************
    //  Get include Paths
    //***************************
    string currDir = zTransformApp::getAPP()->getInterpreterDir();
    string includePath;
    {
        /* include paths relative to the program */
        string buf;
        stringstream ss(zTransformApp::getAPP()->
                        getOption("DIRECTORIES", "includePath", ""));
        while (getline(ss, buf, ';')) {
            if(buf != "") {
                if(buf[0] == '"') {
                    includePath += " -I " + buf;
                } else {
                    includePath += " -I \"" + buf + "\"";
                }
            }
        }
    }

    //***************************
    // Collect Defines
    //***************************

    string defines;
    zTransformApp::TOptions options = zTransformApp::getAPP()->getOptions("DEFINES");

    zTransformApp::TOptions::iterator it;
    for(it = options.begin(); it != options.end(); ++it) {
        defines += " -D " + (*it).first + "=" + (*it).second;
    }

    //***************************
    //  Build the command line
    //***************************

    string commandLine;
    {
        commandLine = ".\\_intern\\mcpp.exe -P" + includePath + defines
                           + " \"" + fileName + "\" "
                           + " -Q -o \"" + currDir + "\\_intern\\preprocessorOutput.zsl\"";

        INFO("Calling preprocessor via: " + commandLine);
    }

    //***************************
    // Make the call and wait
    //***************************

    unsigned long exitCode;
    {
        STARTUPINFO si;
        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = si.lpDesktop = si.lpTitle = 0;
        si.cbReserved2 = 0;
        si.lpReserved2 = 0;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi;

        SetCurrentDirectory(currDir.c_str());

        bool bSuccess;
        bSuccess = CreateProcess(
                                  0,                         //__in_opt     LPCTSTR lpApplicationName,
                                  (char*)commandLine.c_str(),//__inout_opt  LPTSTR lpCommandLine,
                                  0,                         //__in_opt     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                  0,                         //__in_opt     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                  0,                         //__in         BOOL bInheritHandles,
                                  CREATE_DEFAULT_ERROR_MODE, //__in         DWORD dwCreationFlags,
                                  0,                         //__in_opt     LPVOID lpEnvironment,
                                  currDir.c_str(),           //__in_opt     LPCTSTR lpCurrentDirectory,
                                  &si,                       //__in         LPSTARTUPINFO lpStartupInfo,
                                  &pi                        //__out        LPPROCESS_INFORMATION lpProcessInformation
        );

        if(!bSuccess) {
            stringstream ss;
            ss << "Could not call the preprocessor! Error Code: " << GetLastError();
            GENERAL_ERROR(ss.str());
        };

        WaitForSingleObject( pi.hProcess, INFINITE );


        if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
            GENERAL_ERROR("Failed to query the exit code of the preprocessor!");
        };

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    //***************************
    // Analysis
    //***************************

    {
        int errorFileExisted = MoveFileEx("mcpp.err", "_intern\\mcpp.err", MOVEFILE_REPLACE_EXISTING);
        bool printedErrors = false;

        std::ifstream ifs("_intern\\mcpp.err");

        if(exitCode) {
            stringstream ss;
            if (exitCode == 2) {
                ss << "Exitcode of mccp: 2 (file not found).";
            } else {
                ss << "There where errors during preprocessing. Exitcode of mcpp: " << exitCode << ". Error mesages:";
                printedErrors = true;
            }

            LOG_ERROR(ss.str());
        } else {
            ifs.peek(); //look for eof
            if (errorFileExisted && ifs.good()) {
                WARN("There where the following warnings during preprocessing:");
                printedErrors = true;
            }
        }

        if (errorFileExisted) {
            INDENTLOG;
            string buf;
            while(getline(ifs, buf) && (buf != "")) {
                WARN(buf);
                printedErrors = true;
            }
        }

        if(printedErrors) {
            WARN("[end of preprocessor messages.]");
        }

        if(exitCode){
            throw generalError;
        } else {
            INFO("Preprocessor finished successfully.");
        }
    }

    return currDir + "\\_intern\\preprocessorOutput.zsl";
}
