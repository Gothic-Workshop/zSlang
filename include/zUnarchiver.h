#ifndef ZUNARCHIVER_H
#define ZUNARCHIVER_H

#include<fstream>
#include<string>
#include<map>
using std::string;
using std::map;
using std::pair;
using std::ifstream;
using std::stringstream;

class zChunk;
class zWorld;

class zUnarchiver
{
    public:
        zUnarchiver(const string &path, zWorld * world);
        virtual ~zUnarchiver();
        ifstream &getStream(void) { return stream; }
        void newChunk(int ID, zChunk *chunk);
        zChunk* getChunk(int ID);
        void skip(const string &exptoken);
        void skip();
        void skipLine();
        //void setMode(std::ios_base::openmode mode);
        zWorld * getWorld() { return world; };
        int getNumChunks() const {
            return chunks.size();
        }
    protected:
    private:
        ifstream stream;
        map<int,zChunk*> chunks;
        string path;
        zWorld * world;
};

#endif // ZUNARCHIVER_H
