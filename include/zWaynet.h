#ifndef ZWAYNET_H
#define ZWAYNET_H

#include <zChunk.h>
#include <cSelection.h>

class zChunkWP;
class zUnarchiver;
class zArchiver;
class cSelection;

class zWaynet : public zChunk
{
    public:
        const cSelection & getWPs() { return wps; };
        virtual void archiveMembers(zArchiver &arc);
        void merge(zWaynet *other);
        void removeWP(zChunkWP* wp);
        void movedToWorld(zWorld * newWorld);
    protected:
        void readMembers(zUnarchiver &arc);
    private:
        cSelection wps;
        zWaynet(zChunkHeaderInfo info) : zChunk(info), world(0) {};
        virtual ~zWaynet();
        friend zChunk* zChunk::readChunk(zUnarchiver &arc); //may construct me.

        zWorld *world;
};

#endif // ZWAYNET_H
