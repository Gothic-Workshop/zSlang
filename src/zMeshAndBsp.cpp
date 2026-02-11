#include "zMeshAndBsp.h"
#include<cLogger.h>
#include <zUnarchiver.h>
#include <zArchiver.h>
#include <sstream>

void zMeshAndBsp::readMembers(zUnarchiver &arc) {
    std::istream &stream = arc.getStream();

    stream.read((char*)&version, 4);
    stream.read((char*)&size,    4);

    stringstream ss;
    ss << "ZEN is compiled. Skipping MeshAndBsp (size: " << size << ").";
    INFO(ss.str());
    cLogger::getLogger()->commit();

    data = malloc ( size );
    stream.read((char*)data, size);

    //normal mode.
    readChunkEnd(arc);
}

void zMeshAndBsp::archiveMembers(zArchiver &arc) {
    INFO("Writing MeshAndBsp Chunk...");
    cLogger::getLogger()->commit();
    {
        //arc.setMode(ios_base::binary);
        arc.getStream().write((char*)&version,    4);
        arc.getStream().write((char*)&size,       4);
        arc.getStream().write((char*)data,     size);
        //arc.setMode(ios_base::out);
    }
    INFO("Done.");
    cLogger::getLogger()->commit();
}

zMeshAndBsp::~zMeshAndBsp() {
    free(data);
}
