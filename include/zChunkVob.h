#ifndef ZCHUNKVOB_H
#define ZCHUNKVOB_H

#include <vector>
#include <zChunkSelectable.h>
#include <cSelection.h>

class zChunkVob : public zChunkSelectable {
    public:
        virtual const cSelection& getChilds() const { return childs; }
        void insertChild(zChunkVob *vob);
        void moveTo(zChunkVob *newParent);
        void removeChild(zChunkVob *vob);
        zChunkVob * getParent() { return parent; };
    protected:
        zChunkVob(zChunkHeaderInfo info) : zChunkSelectable (info) {
            parent = 0;
        };
        virtual ~zChunkVob();
        friend zChunk* zChunk::readChunk(zUnarchiver &arc);; //may construct me.
    private:
        cSelection childs;
        zChunkVob *parent;
};

#endif // ZCHUNKVOB_H
