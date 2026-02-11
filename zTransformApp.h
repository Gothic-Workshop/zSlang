/***************************************************************
 * Name:      zTransformApp.h
 * Purpose:   Defines Application Class
 * Author:    Sekti ()
 * Created:   2011-03-11
 * Copyright: Sekti (http://forum.worldofplayers.de/forum/member.php?u=11242)
 * License:
 **************************************************************/

#ifndef ZTRANSFORMAPP_H
#define ZTRANSFORMAPP_H

#include<string>
#include<vector>
using std::string;
using std::wstring;

class cMain;
class zTransformFrame;

class zTransformApp {
    public:
        zTransformApp(int argc, char** argv);
        void run();
        void exit();

        typedef std::pair<string,string> TOption;
        typedef std::vector<TOption> TOptions;
        string getOption(const string &section, const string &name, const string &def = "") const;
        TOptions getOptions(const string &section) const;
        void   setOption(const string &section, const string &key,  const string &val)      const;

        const string& getScriptDir()       const { return scriptDir;      }
        const string& getScriptPath()      const { return scriptPath;     }
        const string& getInterpreterPath() const { return interpreterDir; }
        const string& getInterpreterDir()  const { return interpreterDir; }

        static string FilePathToFileDir(const string &fullPath);

        static zTransformApp* getAPP() { return theAPP; }
    private:
        string scriptDir;
        string scriptPath;
        string interpreterDir;
        string interpreterPath;

        string getIniPath() const;
        string processOption(string str) const;
        cMain * main;
        static zTransformApp* theAPP;
        int argc;
        char **argv;
};

#endif // ZTRANSFORMAPP_H
