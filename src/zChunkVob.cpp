#include "zChunkVob.h"
#include<cLogger.h>

zChunkVob::~zChunkVob() {
    /* copy needed because childs changes all the time */
    cSelection childCpy = childs;
    for(cSelection::iterator it = childCpy.begin(); it != childCpy.end(); ++it) {
        this->removeChild(static_cast<zChunkVob*>(*it));
    }

    if(parent) {
        parent->removeChild(this);
    }
}

void zChunkVob::insertChild(zChunkVob *vob) {
    vob->addRef();
    childs.insert(vob);
    vob->parent = this;
}

void zChunkVob::moveTo(zChunkVob *newParent) {
    ASSERT(newParent, "zChunkVob::moveTo: newParent is null.");

    /* consistency check: I cannot move myself to a child of me */
    zChunkVob *parentWalker = newParent;
    while(parentWalker) {
        if (parentWalker == this) {
            RUNTIME_ERROR("Cannot make objekt the child of one of its childs.");
        } else {
            parentWalker = parentWalker->getParent();
        }
    }

    this->addRef(); //dont kill myself with the next step

    if(this->parent) {
        this->parent->removeChild(this);
    }
    newParent->insertChild(this);
    this->release(); //release again
    this->movedToWorld(newParent->getWorld());
}

void zChunkVob::removeChild(zChunkVob *vob) {
    if (childs.contains(vob)) {
        childs.remove(vob);
        vob->parent = 0;
        vob->release();
    } else {
        FAULT("Trying to destroy a non existing parent-child relationship.");
    }
}
