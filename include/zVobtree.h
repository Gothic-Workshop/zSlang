#ifndef ZVOBTREE_H
#define ZVOBTREE_H

#include <zChunkVob.h>
#include <map>

using std::multimap;

/* strictly speeking this is no vob.
 * this is not even a selectable Chunk.
 * but I can make good use of the zChunkVob properties
 * and it is nice for the vobtree to actually be a tree.
 * so i don't want to make the root something entirely differnt. */
class zVobtree : public zChunkVob {
    public:
        void archiveMembers(zArchiver &arc);
        void merge(zVobtree *other);
    protected:
        void readMembers(zUnarchiver &arc);
        zVobtree(zChunkHeaderInfo info) : zChunkVob(info) {};
        virtual ~zVobtree() {};
        friend zChunk* zChunk::readChunk(zUnarchiver &arc);; //may construct me
    private:
        void archiveChildrenOf (
            zArchiver &arc, zChunkSelectable* chunk,
            multimap<zChunkSelectable*,zChunkSelectable*> &childMap,
            int &childProps);
};

#endif // ZVOBTREE_H
