#ifndef ZCHUNKWP_H
#define ZCHUNKWP_H

#include <zChunkSelectable.h>
#include <cSelection.h>

class zChunkWP : public zChunkSelectable
{
    public:
        const cSelection& getConnected() { return connected; }
        void connect(zChunkWP *other);
        void disconnect(zChunkWP *other);
        void disconnectAll();
        static zChunkWP * readChunkWP(zUnarchiver &arc);
        string getName() const;
    protected:
    private:
        cSelection connected;
        zChunkWP(zChunkHeaderInfo info) : zChunkSelectable (info) {};
        virtual ~zChunkWP();
        friend zChunk* zChunk::readChunk(zUnarchiver &arc);; //may construct me
};

#endif // ZCHUNKWP_H
