/***************************************************************
 * Name:      zTransformApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Sekti ()
 * Created:   2011-03-11
 * Copyright: Sekti (http://forum.worldofplayers.de/forum/member.php?u=11242)
 * License:
 **************************************************************/

#include "zTransformApp.h"
#include <sstream>
#include <cProperty.h>
#include <iostream>
#include <Windows.h>
#include <cMain.h>
#include <cLogger.h>

zTransformApp* zTransformApp::theAPP;

zTransformApp::zTransformApp(int argc, char** argv):argc(argc), argv(argv)
{
	theAPP = this;

    /* Get Interpreter Directory: */
    int size = 10;
    while(true) {
        char buf[size];
        int retSize = GetModuleFileName(NULL, buf, size);
        if (retSize < size) {
            interpreterPath = string(buf);
            interpreterDir  = FilePathToFileDir(interpreterPath);
            break;
        }
        size *= 2;
    }

    std::cout << interpreterPath << interpreterDir << std::endl;
    cLogger::replaceLogger(new czSpyLogger);

    //startup Logger
    bool ld    = getOption("LOGGING", "logDebug", "0")       == "0" ? false : true,
         li    = getOption("LOGGING", "logInfos", "1")       == "0" ? false : true,
         lw    = getOption("LOGGING", "logWarnings", "1")    == "0" ? false : true,
         le    = getOption("LOGGING", "logErrors", "1")      == "0" ? false : true;
         //clear = getOption("LOGGING", "clearBeforeRun", "1") == "0" ? false : true;

    cLogger::setLogDebug(ld);
    cLogger::setLogInfo (li);
    cLogger::setLogWarn (lw);
    cLogger::setLogError(le);

    main = new cMain();
    main->init();

    /* get script directory */
    if (argc >= 2) { do {
        char buf[size];
        int nsize = GetFullPathName(argv[1], size, buf, NULL);
        if (nsize > size) {
            size = nsize;
            continue;
        }
        if (nsize == 0) {
            LOG_ERROR(string("The file \"") + argv[1] + "\" could not be found.");
            break;
        }
        scriptPath = buf;
        scriptDir  = FilePathToFileDir(scriptPath);
        break;
    } while(true); } else {
        LOG_ERROR("No input file specified on the command line!");
    }
}

void zTransformApp::run() {
    if (scriptPath != "") {
        main->runScript(scriptPath);
    }
}

int main(int argc, char ** argv) {
    zTransformApp app(argc, argv);
    app.run();
    app.exit();
    return 0;
}

//***************************
//  INI File Handling
//***************************

string zTransformApp::FilePathToFileDir(const string &fullPath) {
    std::string::const_iterator lastBackslash = fullPath.begin(), pos = lastBackslash;
    while(pos != fullPath.end()) {
        if(*pos == '\\') {
            lastBackslash = pos;
        }
        pos++;
    }
    return string(fullPath.begin(), lastBackslash);
}

string zTransformApp::getIniPath() const {
    string iniPath = interpreterDir + "\\zSlang.ini";

    if (GetFileAttributes(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        LOG_ERROR("Ini File missing (expecting it here: \"" + iniPath + "\"!");
        iniPath = string("");
    }
    return iniPath;
}

string zTransformApp::processOption(string str) const {
    string patterns[] = { "$(ZSLANG_DIR)", "$(SCRIPT_DIR)"};
    string replacement[]  = { interpreterDir, scriptDir };

    for(int i = 0; i < 2; ++i) {
        size_t pos;
        while((pos=str.find(patterns[i])) != string::npos) {
            str.replace(pos, patterns[i].length(), replacement[i]);
        }
    }

    return str;
}

string zTransformApp::getOption(const string &section, const string &key, const string &def) const {
    static char buf[500];
    GetPrivateProfileString(section.c_str(), key.c_str(), def.c_str(),
                            buf, 500,
                            getIniPath().c_str());
    return processOption(buf);
}

zTransformApp::TOptions zTransformApp::getOptions(const string &section) const {
    static char buf[10000];
    GetPrivateProfileString(section.c_str(), 0, 0, buf, 10000, getIniPath().c_str());

    char *curr = buf;
    TOptions result;
    while(true) {
        int len = strlen(curr);
        if (!len) { break; }
        string key(curr);
        result.push_back(TOption(key, getOption(section, key, "")));
        curr += len + 1;
    }
    return result;
}

void zTransformApp::setOption(const string &section, const string &key, const string &val) const {
    WritePrivateProfileString(section.c_str(), key.c_str(), val.c_str(),
                              getIniPath().c_str());
}

void zTransformApp::exit() {
    delete main; //calls quit
}
