#ifndef ZCHUNKSELECTABLE_H
#define ZCHUNKSELECTABLE_H

#include <zChunk.h>
#include <set>

class zChunkVob;
class cSelection;

class zChunkSelectable : public zChunk {
    public:
        /* dummy implementation of some stuff you may want to request.
         * The vob will overwrite them, the Waypoint won't.
         * but its good to be able to ask any Selectable
         * whether it has children without casting around */
        virtual zChunkVob* getParent() { return 0; }

        virtual const cSelection& getChilds() const;

        static bool isLiving(zChunkSelectable* chunk);

        zWorld * getWorld() { return world; }
        void movedToWorld(zWorld * world);
    protected:
        zChunkSelectable(zChunkHeaderInfo info);
        virtual ~zChunkSelectable();
    private:
        static std::set<zChunkSelectable*> livingChunks;
        zWorld * world;
};

#endif // ZCHUNKSELECTABLE_H
