#ifndef CMAIN_H
#define CMAIN_H

#include<string>

using std::string;

class cMain {
    public:
        cMain();
        ~cMain();
        void init();
        void killItWithFire(const string &flames);
        static string getApplicationName();
        static string getVersionString();
        void runScript(const string &fPath);
    private:
        void printHello() const;
};

#endif // CMAIN_H
