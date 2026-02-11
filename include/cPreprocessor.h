#ifndef CPREPROCESSOR_H
#define CPREPROCESSOR_H

#include<string>
using std::string;

class cPreprocessor
{
    public:
        static string preprocess(const string &fileName);
    protected:
    private:
        cPreprocessor()  {};
        ~cPreprocessor() {};
};

#endif // CPREPROCESSOR_H
