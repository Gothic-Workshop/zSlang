#include "zWaynet.h"
#include <zChunkWP.h>
#include<cLogger.h>
#include <cProperty.h>
#include <vector>
#include <zArchiver.h>
#include <cSelection.h>
#include <zUnarchiver.h>

void zWaynet::readMembers(zUnarchiver &arc) {
    this->world = arc.getWorld();

    INFO("Reading Waynet chunk...");
    cLogger::getLogger()->commit();
    {
        INDENTLOG;

        if (cProperty::readInt(arc,"waynetVersion") != 1) {
            PARSE_ERROR("Wrong Waynet version!");
        }

        int numWaypoints = cProperty::readInt(arc,"numWaypoints");

        for (int i = 0; i < numWaypoints; ++i) {
            wps.insert(zChunkWP::readChunkWP(arc));
        }

        int numWays = cProperty::readInt(arc,"numWays");

        for (int i = 0; i < numWays; ++i) {
            zChunkWP *left  = zChunkWP::readChunkWP(arc);
            zChunkWP *right = zChunkWP::readChunkWP(arc);

            /* In case this Waypoint has already been read:
               Discard the additional reference, I don't need it. */
            if (left->getConnected().size()) {
                ONDEBUG(
                    ASSERT(find(wps.begin(), wps.end(), left) != wps.end(), "Waynet consistency Error.");
                )
                left ->release();
            } else {
                ONDEBUG(
                    ASSERT(find(wps.begin(), wps.end(), left) == wps.end(), "Waynet consistency Error.");
                )
                wps.insert(left);
            }
            if (right->getConnected().size()) {
                ASSERT(find(wps.begin(), wps.end(), right) != wps.end(), "Waynet consistency Error.");
                right->release();
            } else {
                ASSERT(find(wps.begin(), wps.end(), right) == wps.end(), "Waynet consistency Error.");
                wps.insert(right);
            }

            left->connect(right);
        }

        for(cSelection::iterator it = wps.begin(); it != wps.end(); ++it) {
            (*it)->movedToWorld(arc.getWorld());
        }

        readChunkEnd(arc);
    }
    INFO("Done.");
    cLogger::getLogger()->commit();
}

void zWaynet::movedToWorld(zWorld * newWorld) {
    world = newWorld;
    for(cSelection::iterator it = wps.begin(); it != wps.end(); ++it) {
        (*it)->movedToWorld(world);
    }
}

void zWaynet::merge(zWaynet* other) {
    if (!other) { return; }

    other->movedToWorld(world);
    this->wps.add(other->wps);
    other->wps = cSelection(); //stolen
}

void zWaynet::archiveMembers(zArchiver &arc) {
    INFO("Archiving the waynet...");
    {
        cProperty::writeInt(arc, "waynetVersion", 1);

        vector<zChunkWP*> isolated;
        vector<pair<zChunkWP*,zChunkWP*> > ways;

        for(cSelection::iterator it = wps.begin(); it != wps.end(); ++it) {
            zChunkWP *wp = dynamic_cast<zChunkWP*>(*it);
            ASSERT(wp, "Waynet has non-WP members?!");

            if(!arc.isSelected(wp)) {
                continue;
            }

            int numWays = 0;

            cSelection connected = wp->getConnected();
            for (cSelection::iterator conIt = connected.begin();
                conIt != connected.end(); ++conIt) {
                zChunkWP *other = dynamic_cast<zChunkWP*>(*conIt);
                ASSERT(other, "Waypoint has non-WP siblings?!");

                if(!arc.isSelected(other)) {
                    //TODO: Create Connect-WPs!
                    continue;
                }

                numWays++;

                if (wp < other) { //Every way only once, not twice!
                    ways.push_back(pair<zChunkWP*,zChunkWP*> (wp,other));
                }
            }

            if (!numWays) {
                isolated.push_back(wp);
            }
        }

        cProperty::writeInt(arc, "numWaypoints", isolated.size());
        // collected all the information, now write it:
        for (unsigned int i = 0; i < isolated.size(); ++i) {
            // set the name. Don't mind setting it back,
            //it has to be recalculated from scratch anyway
            stringstream ss;
            ss << "waypoint" << i;
            isolated[i]->setName(ss.str());
            isolated[i]->archive(arc);
        }

        cProperty::writeInt(arc, "numWays", ways.size());
        for (unsigned int i = 0; i < ways.size(); ++i) {
            zChunkWP *left, *right;
            left  = ways[i].first;
            right = ways[i].second;

            stringstream ss;
            ss << "wayl" << i;
            left->setName(ss.str());

            ss.str(""); ss << "wayr" << i;
            right->setName(ss.str());

            left->archive(arc);
            right->archive(arc);
        }
    }
    INFO("Done.");
    cLogger::getLogger()->commit();
}

void zWaynet::removeWP(zChunkWP* wp) {
    ASSERT(wps.contains(wp), "Request to remove WP from Waynet, but WP was not registered in Waynet.");
    wps.remove(wp);
    wp->release();
}

zWaynet::~zWaynet() {
    for (cSelection::iterator it = wps.begin(); it != wps.end(); ++it) {
        (*it)->release();
    }
    wps = cSelection();
}
