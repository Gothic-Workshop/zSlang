#include "zVobtree.h"
#include <stack>
#include<cLogger.h>
#include <cProperty.h>
#include <zArchiver.h>
#include <zWorld.h>
#include <cSelection.h>
#include <zUnarchiver.h>

void zVobtree::readMembers(zUnarchiver &arc) {
    INFO("Reading vobtree chunk...");
    cLogger::getLogger()->commit();
    {
        INDENTLOG;

        zChunkVob *currparent = this;
        int childs = cProperty::readInt(arc,"childs0");
        int propCounter = 1;
        stack<int> childs_stack;

        while(currparent) {
            if (childs > 0) {
                zChunk *newChunk = zChunk::readChunk(arc);
                //readChunk delivers or throws a parse error
                ASSERT(newChunk, "zChunk::readChunk did not deliver.");

                zChunkVob *newVob = dynamic_cast<zChunkVob*>(newChunk);

                if (!newVob) {
                    newChunk->release(); //throw it away and get out
                    PARSE_ERROR("zCVob expected but different object found.");
                }

                currparent->insertChild(newVob);
                newVob->release(); //i dont need this reference any more

                childs--;
                childs_stack.push(childs);

                currparent = newVob;

                char counterStr[10];
                sprintf(counterStr,"%d",propCounter++);
                childs = cProperty::readInt(arc, string("childs")+counterStr);
            } else {
                //this parent has no more children
                currparent = currparent->getParent();

                if (currparent) {
                    childs = childs_stack.top();
                    childs_stack.pop();
                }
            }
        }

        this->movedToWorld(arc.getWorld());
    }
    zChunk::readChunkEnd(arc);
    INFO("Done.");
    cLogger::getLogger()->commit();
}

void zVobtree::merge(zVobtree *other) {
    if (other) {
        cSelection vobs = other->getChilds(); //cpy because it will change

        for(cSelection::iterator it = vobs.begin(); it != vobs.end(); ++it) {
            (static_cast<zChunkVob*>(*it))->moveTo(this);
        }
    }
}

void zVobtree::archiveChildrenOf(
        zArchiver &arc,
        zChunkSelectable* chunk,
        multimap<zChunkSelectable*,zChunkSelectable*> &childMap,
        int &childProps) {
    typedef multimap<zChunkSelectable*,zChunkSelectable*>::iterator mmIt;

    int childs = childMap.count(chunk);
    stringstream ss; ss << "childs" << childProps++;
    cProperty::writeInt(arc, ss.str(), childs);

    mmIt childWalker = childMap.lower_bound(chunk);
    for (; childWalker != childMap.upper_bound(chunk); ++childWalker) {
        childWalker->second->archive(arc);
        archiveChildrenOf(arc, childWalker->second, childMap, childProps);
    }
}
void zVobtree::archiveMembers(zArchiver &arc) {
    INFO("Writing vobtree chunk...");
    {
        INDENTLOG;
        multimap<zChunkSelectable*,zChunkSelectable*> childMap;

        zWorld::iterator it = getWorld()->begin();
        while (it != getWorld()->beginWaynet()) {
            zChunkSelectable *chunk = *it;
            if (arc.isSelected(chunk)) {
                zChunkSelectable *parent = chunk->getParent();

                while(parent != this && !arc.isSelected(parent)) {
                    parent = parent->getParent();
                }

                childMap.insert(pair<zChunkSelectable*,zChunkSelectable*>
                                    (parent,chunk));
            }

            ++it;
        }

        int childProps = 0;
        archiveChildrenOf(arc, this, childMap, childProps);
    }
    INFO("Done.");
    cLogger::getLogger()->commit();
}

