#include "zWorld.h"
#include <zArchiver.h>
#include <zUnarchiver.h>
#include<cLogger.h>
#include <zChunk.h>
#include <zVobtree.h>
#include <zWaynet.h>
#include <zChunkWP.h>
#include <cMain.h>

#include <Windows.h>

zWorld::zWorld(const string &fileName) {
    zUnarchiver arc(fileName, this);

    if (!arc.getStream().good()) {
        RUNTIME_ERROR(string("Trying to open a non existing zen file: \"")
                      + fileName + '"' );
        return;
    }

    INFO("Unarchiving zen file...");
    {
        INDENTLOG;
        INFO("Unarchiving header...");
        //UNINDENTLOG;
        try {
            INDENTLOG;
            arc.skip("ZenGin"); arc.skip("Archive");
            arc.skip("ver"); arc.skip("1");
            arc.skip("zCArchiverGeneric");
            arc.skip("ASCII");
            arc.skip("saveGame"); arc.skip("0");
            arc.skip("date"); arc.skip(); arc.skip();
            arc.skip("user"); arc.skipLine(); //may contain spaces
            arc.skip("END");
            arc.skip("objects"); arc.skip();
            arc.skip("END");
        } catch (cParseError &e) {
            LOG_ERROR("This does not seem to be a zen-file saved in ascii mode.");
            throw; //rethrow
        }
        INFO("Done. Reading world data...");
        cLogger::getLogger()->commit();
        try {
            INDENTLOG;
            worldData = zChunk::readChunk(arc);
        } catch (cParseError &e) {
            LOG_ERROR("Error while reading the zenfile.");
            throw; //rethrow
        }
        INFO("Done.");
        cLogger::getLogger()->commit();

        this->vobtree = dynamic_cast<zVobtree*>(worldData->getSubchunk("VobTree"));

        zChunk * waynetWrapper = worldData->getSubchunk("WayNet");
        if (waynetWrapper) {
            this->waynet = dynamic_cast<zWaynet*>
                           (waynetWrapper->getSubchunk("zCWayNet"));
        } else {
            this->waynet = 0;
        }

        if (!vobtree){
            INFO("No vobtree found.");
        } else {
            INFO("Vobtree found.");
        }
        if (!waynet) {
            INFO("No waynet found.");
        } else {
            INFO("Waynet found.");
        }

    }

    stringstream ss;
    ss << "World loaded. " << arc.getNumChunks() << " numbered chunks read.";
    INFO(ss.str());
}

void zWorld::merge(const string &fileName) {
    zWorld other(fileName);

    if (!vobtree) {
        vobtree = other.vobtree;
        other.vobtree = 0; //steel the vobtree

        if (vobtree) {
            vobtree->movedToWorld(this);
        }
    } else {
        vobtree->merge(other.vobtree);
    }

    if (!waynet) {
        waynet = other.waynet; //steel the waynet
        other.waynet = 0;

        if (waynet) {
            waynet->movedToWorld(this);
        }
    } else {
        waynet->merge(other.waynet);
    }
}

void zWorld::save(const string &filename, cSelection* sel) {
    zArchiver arc(filename, sel);

    INFO("Archiving zen file...");
    {
        INDENTLOG;
        INFO("Archiving Header...");
        {
            INDENTLOG;

            ostream &stream = arc.getStream();
            stream << "ZenGin Archive"; arc.ENDL();
            stream << "ver 1"; arc.ENDL();
            stream << "zCArchiverGeneric"; arc.ENDL();
            stream << "ASCII"; arc.ENDL();
            stream << "saveGame 0"; arc.ENDL();

            SYSTEMTIME	st;
            GetLocalTime (&st);
            stream <<"date "<<st.wDay<<"."<<st.wMonth<<"."<<st.wYear<<" "
                   <<st.wHour<<":"<<st.wMinute<<":"<<st.wSecond; arc.ENDL();

            stream << "user "
                    << cMain::getApplicationName() << "."
                    << cMain::getVersionString(); arc.ENDL();

            stream << "END"; arc.ENDL();
            stream << "objects 0"; arc.ENDL(); // i don't count them.
            stream << "END"; arc.ENDL();
            arc.ENDL();
        }
        INFO("Done. Writing world data...");
        {
            INDENTLOG;
            worldData->archive(arc);
        }
        INFO("Done.");
    }
}

zWorld::~zWorld() {
    worldData -> release();
}


//***************************
//  Iterator:
//***************************

zWorld::iterator::iterator(zWorld *world, TFlags flags) {
    this->world = world;

    /* this can only construct begin and end. */
    this->flags = flags;
    this->current = 0;
    if (flags == fNone) {
        enterVobtree();
    } else if (flags == fInWaynet) {
        enterWaynet();
    }
}

void zWorld::iterator::enterVobtree() {
    current = world->getVobtree();

    if (!current){
        enterWaynet();
        return;
    }

    ++*this; //the Vobtree is no valid vob
}

void zWorld::iterator::enterWaynet() {
    flags = fInWaynet;

    if (!world->getWaynet()) {
        flags = fEnd;
        return;
    } else {
        iteratorStack.push(world->getWaynet()->getWPs().begin());
        endIteratorStack.push(world->getWaynet()->getWPs().end());

        if (iteratorStack.top() == endIteratorStack.top()) {
            flags = fEnd;
        } else {
            current = *iteratorStack.top();
        }
    }
}

zWorld::iterator& zWorld::iterator::operator ++() {
    switch (flags) {
        case fEnd:
            FAULT("Cannot increment Iterator beyond the end of the data.");
        case fInWaynet:
            iteratorStack.top()++;

            if (iteratorStack.top() == endIteratorStack.top()) {
                flags = fEnd;
            } else {
                current = *iteratorStack.top();
            }
            return *this;
        case fNone:
            {
                /* start exploring the childs of current: */
                if (current->getChilds().size()) {
                    iteratorStack.push(current->getChilds().begin());
                    endIteratorStack.push(current->getChilds().end());
                    current = *iteratorStack.top();
                    return *this;
                }

                /* if current does not have any childs backtrack: */
                while(true) {
                    if (iteratorStack.empty()) {
                        //no branch left
                        enterWaynet();
                        return *this;
                    } else {
                        iteratorStack.top()++; //go on in the tree

                        if (iteratorStack.top() != endIteratorStack.top()) {
                            current = *iteratorStack.top();
                            return *this;
                        } else {
                            iteratorStack.pop();
                            endIteratorStack.pop();
                        }
                    }
                }
            }
        default:
            FAULT("zWorld::iterator::operator ++(): Should never be here!");
    }
}

zChunkSelectable * zWorld::iterator::operator *() {
    if (flags == fEnd) {
        FAULT("Trying to get object in end() iterator.");
    }
    return current;
}

bool zWorld::iterator::operator !=(const iterator& b) const {
    return !(*this == b);
}

bool zWorld::iterator::operator ==(const iterator& b) const {
    if (flags != b.flags) {
        return false;
    }

    if (flags == fEnd) {
        /* both are pointing at the end */
        return true;
    }

    return current == b.current;
}
