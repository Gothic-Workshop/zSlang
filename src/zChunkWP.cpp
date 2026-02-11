#include "zChunkWP.h"
#include<cLogger.h>
#include<boost/variant/get.hpp>
#include<cProperty.h>

void zChunkWP::connect(zChunkWP *other) {
    if (this == other) {
        INFO("Minor warning: Trying to connect wp to itself: " + this->getName());
        return;
    }

    if (connected.contains(other)) {
        INFO("Minor warning: Trying to connect two waypoints that are already connected: "
             + this->getName() + " <--> " + other->getName());
        return;
    }

    connected.insert(other);
    other->connected.insert(this);
}

void zChunkWP::disconnect(zChunkWP *other) {
    if (this == other) {
        INFO("Minor warning: Trying to disconnect wp from itself: " + this->getName());
        return;
    }

    if (!connected.contains(other)) {
        INFO("Minor warning: Trying to disconnect two waypoints that are not connected: "
                 + this->getName() + " <--> " + other->getName());
    }

    connected.remove(other);
    other->connected.remove(this);
}

void zChunkWP::disconnectAll() {
    for (cSelection::iterator it = connected.begin(); it != connected.end(); ++it) {
        zChunkWP *other = dynamic_cast<zChunkWP*>(*it);
        ASSERT(other, "WP connected to non-WP?!");
        other->connected.remove(this);
    }
    connected = cSelection(); //empty
}

string zChunkWP::getName() const {
    cProperty *nameProp = this->getProperty("wpName");

    if (!nameProp) {
        WARN("Waypoint without wpName property!?");
        return "";
    }

    const string *val = boost::get<string>(&nameProp->getValue());

    if (!val) {
        WARN("Waypoint with wpName that is not a string?!");
        return "";
    }

    return *val;
}

zChunkWP * zChunkWP::readChunkWP(zUnarchiver &arc) {
    zChunk * newChunk = zChunk::readChunk(arc);
    zChunkWP * newWP = dynamic_cast<zChunkWP*>(newChunk);

    if (!newWP) {
        string err = string("Expected a zCWaypoint but found a '")
                           +newChunk->getClassName() + "'.";

        newChunk -> release(); //throw it away and throw an error
        PARSE_ERROR(err);
    }

    return newWP;
}

zChunkWP::~zChunkWP() {
    disconnectAll();
}
