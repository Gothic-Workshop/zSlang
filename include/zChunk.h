#ifndef ZCHUNK_H
#define ZCHUNK_H

#include<map>
#include<vector>

using std::map;
using std::vector;

#include<zChunkMember.h>

class zUnarchiver;
class zChunkVob;
class cProperty;
class zWorld;
class zArchiver;

typedef struct {
    string chunkName, className;
    unsigned int classVersion, objectIndex;
} zChunkHeaderInfo;

class zChunk : public zChunkMember
{
    public:
        virtual ~zChunk();
        cProperty* getProperty(const string &name) const;
        zChunk   * getSubchunk(const string &name) const;
        virtual string getName() const { return info.chunkName; }
        void   setName(const string &name) { info.chunkName = name; }
        string getClassName() { return info.className; }
        static zChunk* readChunk(zUnarchiver &arc);

        virtual void readMembers(zUnarchiver &arc);
        virtual void archiveMembers(zArchiver &arc);
        void archive(zArchiver &arc);

        bool operator==(const zChunk &other) const;
        bool operator!=(const zChunk &other) const {
            return !(operator==(other));
        }
    protected:
        zChunk(zChunkHeaderInfo info);
        static zChunkHeaderInfo readHeader(zUnarchiver &arc);
        zChunkHeaderInfo info;
        vector<zChunkMember*> members;
        map<string,cProperty*> properties;
        static void readChunkEnd(zUnarchiver &arc);
};

#endif // ZCHUNK_H
