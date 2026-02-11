#include "zChunkSelectable.h"
#include<cLogger.h>
#include <zChunkVob.h>

const cSelection& zChunkSelectable::getChilds() const {
    static const cSelection sel;
    return sel;
}

//***************************
//  my World
//***************************

void zChunkSelectable::movedToWorld(zWorld * world) {
    ASSERT(!getParent() || getParent()->getWorld() == world, "Chunk moved to different world than it's parent?");

    if (this->world != world) {
        this->world = world;
    }

    cSelection childs = getChilds();
    for (cSelection::iterator it = childs.begin(); it != childs.end(); ++it) {
        (*it)->movedToWorld(world);
    }
}

//***************************
//  Living chunks
//***************************

std::set<zChunkSelectable*> zChunkSelectable::livingChunks;

bool zChunkSelectable::isLiving(zChunkSelectable* chunk) {
    return livingChunks.count(chunk);
}

zChunkSelectable::zChunkSelectable(zChunkHeaderInfo info) : zChunk (info) {
    livingChunks.insert(this);
}
zChunkSelectable::~zChunkSelectable() {
    livingChunks.erase(this);
}
