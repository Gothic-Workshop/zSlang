#ifndef ZMESHANDBSP_H
#define ZMESHANDBSP_H

#include <zChunk.h>


class zMeshAndBsp : public zChunk
{
    public:
    protected:
        void readMembers(zUnarchiver &stream);
        void archiveMembers(zArchiver &arc);
    private:
        void *data;
        int version, size;
        zMeshAndBsp(zChunkHeaderInfo info) : zChunk(info) {};
        virtual ~zMeshAndBsp();
        friend zChunk* zChunk::readChunk(zUnarchiver &arc);; //may construct me
};

#endif // ZMESHANDBSP_H
