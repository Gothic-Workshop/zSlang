#include "zArchiver.h"
#include<cLogger.h>
#include <zChunk.h>
#include <cSelection.h>
#include <zChunkSelectable.h>
#include <zTransformApp.h>

const string zArchiver::tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
string zArchiver::indentation() {
    return tabs.substr(0, this->currIndentation);
}

zArchiver::zArchiver(const string &file, cSelection *sel)
        : sel(sel), path(file), nextFreeID(0), currIndentation(0) {

    string outputDir = zTransformApp::getAPP()->getOption("DIRECTORIES", "worldOutputDir");
    stream.open((outputDir + '\\' + path).c_str(), ios_base::out | ios_base::binary);

    if (!stream) {
        stream.close();
        stream.clear();
        stream.open(path.c_str(), ios_base::out | ios_base::binary);

        if (!stream) {
            RUNTIME_ERROR(string("Could not open output stream. I tried:\n\"")
                        + (outputDir + '\\' + path) + "\n"
                        + path);
        }

    } else {
        path = outputDir + path;
    }
}

bool zArchiver::isSelected(zChunkSelectable *chunk) {
    return !this->sel
        ||  this->sel->contains(chunk);
}

void zArchiver::ENDL() {
    stream.put(0x0A);
}

void zArchiver::setMode(ios_base::openmode mode) {
    std::streampos pos = stream.tellp();
    stream.close();
    stream.open(path.c_str(), ios_base::out | ios_base::binary | ios_base::app | mode);
    stream.seekp(pos);
}

bool zArchiver::getID(zChunk* chunk, int &ID) {
    if (chunk->getClassName() == "%") {
        /* this is a null-pointer chunk. They are not referenceable
         * and have ID == 0. */
        ID = 0;
        return false; //needs to be archived anyway
    }

    map<zChunk*,int>::iterator it = chunkIDs.find(chunk);

    if (it != chunkIDs.end()) {
        /* chunk has already been archived. */
        ID = it->second;
        return true; //no need to archive again
    } else {
        ID = nextFreeID++;
        chunkIDs.insert(std::pair<zChunk*,int>(chunk,ID));
        return false; //needs to be archived.
    }
}

zArchiver::~zArchiver() {
    stream.close();
}


