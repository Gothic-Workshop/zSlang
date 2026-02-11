#ifndef CHUNKMEMBER_H
#define CHUNKMEMBER_H

#include<string>
using std::string;

class zArchiver;

class zChunkMember
{
    public:
        zChunkMember() { refsCounter = 1; };
        virtual ~zChunkMember() {};
        virtual string getName() const = 0;
        zChunkMember *addRef() {
            refsCounter++;
            return this;
        }
        void release(void) {
            refsCounter--;
            if (!refsCounter) {
                delete this;
            }
        }

        virtual void archive(zArchiver &arc) = 0;
    protected:
    private:
        int refsCounter;
};

#endif // CHUNKMEMBER_H
