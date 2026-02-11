#include <zChunk.h>
#include<cLogger.h>
#include <zMeshAndBsp.h>
#include <zVobtree.h>
#include <zWaynet.h>
#include <zChunkWP.h>
#include <zUnarchiver.h>
#include <zArchiver.h>
#include <sstream>
#include <cProperty.h>

zChunk::zChunk(zChunkHeaderInfo info) {
    this->info = info;
}

zChunkHeaderInfo zChunk::readHeader(zUnarchiver &arc) {
    std::istream &stream = arc.getStream();

    char c;
    stream >> c;

    if (c != '[') {
        PARSE_ERROR(string("Chunk Start expected but '")+ c + "' found.");
    }

    zChunkHeaderInfo info;
    stream >> info.chunkName >> info.className
            >> info.classVersion >> info.objectIndex;
    stream >> c;

    if (c != ']') {
        PARSE_ERROR(string("Chunk Start broken. Expected ']' but got '")+c+"'.");
    }

    /* the MeshAndBsp reader needs to be exactly at the start of the binary data! */
    stream.get(c);
    if (c != '\n') {
        PARSE_ERROR("Expected linebreak not found.");
    }

    return info;
}

zChunk* zChunk::readChunk(zUnarchiver &arc) {
    zChunkHeaderInfo info = zChunk::readHeader(arc); //throws parse error

    #ifdef __DEBUG__
        stringstream ss;
        ss << "Reading chunk. name: '" << info.chunkName
           << "', class: " << info.className
           << ", index: " << info.objectIndex << ".";
        DEBUGINFO(ss.str());
        INDENTLOG;
    #endif

    if(info.className == "§") {
        zChunk *oldChunk = arc.getChunk(info.objectIndex);

        if (!oldChunk) {
            PARSE_ERROR("Referenced chunk could not be found.");
        }
        zChunk::readChunkEnd(arc); //dont forget to read "[]"

        /* this object is now child of (at least) two objects.*/
        oldChunk->addRef();
        return oldChunk;
    }

    zChunk * newChunk;
    if (info.chunkName == "MeshAndBsp") {
        newChunk = new zMeshAndBsp(info);
    } else if (info.chunkName == "VobTree") {
        newChunk = new zVobtree(info);
    } else if (info.className == "zCWayNet") {
        newChunk = new zWaynet(info);
    } else if (info.className == "zCWaypoint") {
        newChunk = new zChunkWP(info);
    } else if ((info.className.length() >= 5)
    && (info.className.substr(info.className.length() - 5, 5) == "zCVob")) {
        newChunk = new zChunkVob(info);
    } else {
        newChunk = new zChunk(info);
    }

    arc.newChunk(info.objectIndex, newChunk);

    try {
        newChunk->readMembers(arc);
    } catch (cParseError &e) {
        //at least this should hold, shouldnt it?
        ASSERT(newChunk, "Chunk creation failed unexpectedly.");
        newChunk->release(); //speicher freigeben.
        throw; //rethrow:
    }
    return newChunk;
}

void zChunk::readMembers(zUnarchiver &arc) {
    std::istream &stream = arc.getStream();
    char c;

    while (stream.good()) {
        stream >> c;

        if (c != '[') {
            stream.unget();
            cProperty *newProp = cProperty::read(arc, this);
            members.push_back(newProp);
            properties.insert(pair<string, cProperty*> (newProp->getName(), newProp));
        } else {
            stream >> c;

            if (c == ']') {
                return; //end of input
            };

            //hier beginnt ein Unterchunk, zwei Character zurück:
            stream.unget();
            stream.unget();

            zChunk *subchunk = zChunk::readChunk(arc);
            members.push_back(subchunk);
        }
    }

    PARSE_ERROR("Stream died unexpectedly. Broken Zenfile?");
}

void zChunk::readChunkEnd(zUnarchiver &arc) {
    char c,d;
    arc.getStream() >> c >> d;

    if (c != '[' || d != ']') {
        PARSE_ERROR(string("Expected '[]' after Chunk but found '")+c+d+"'.");
    }
}

zChunk* zChunk::getSubchunk(const string &name) const {
    for (unsigned int i = 0; i < members.size(); ++i) {
        if (dynamic_cast<zChunk*>(members[i])) {
            zChunk * chunk = static_cast<zChunk*>(members[i]);

            if ((chunk->getName() == name)
            ||  (chunk->info.className == name)) {
                return chunk;
            }
        }
    }
    return 0;
}

cProperty* zChunk::getProperty(const string &name) const {
    map<string,cProperty*>::const_iterator it = properties.find(name);

    if (it == properties.end()) {
        return 0;
    }

    return it->second;
}

//***************************
//  archiving
//***************************

void zChunk::archive(zArchiver &arc) {
    bool alreadyInArchive;
    int ID;
    alreadyInArchive = arc.getID(this, ID);

    arc.getStream() << arc.indentation()
      << "[" << info.chunkName << " "
      << (alreadyInArchive ? "§" : info.className)    << " "
      << (alreadyInArchive ?  0  : info.classVersion) << " "
      << ID << "]"; arc.ENDL();

    if (!alreadyInArchive)
    {
        INDENTARCHIVE;
        this->archiveMembers(arc);
    }

    arc.getStream() << arc.indentation() << "[]"; arc.ENDL();
}

void zChunk::archiveMembers(zArchiver &arc) {
    /* standard behaviour (overwritten in subclasses) */
    vector<zChunkMember*>::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        (*it)->archive(arc);
    }
}

//***************************
// Comparison
//***************************

bool zChunk::operator==(const zChunk &other) const {
    /* performance */
    if(this == &other) {
        return true;
    }
    if(this->members.size() != other.members.size()) {
        return false;
    }

    for (unsigned int i = 0; i < members.size(); ++i) {
        cProperty* myProp    = dynamic_cast<cProperty*>(this->members[i]);
        cProperty* otherProp = dynamic_cast<cProperty*>(other.members[i]);

        if(myProp || otherProp) {
            //one of them is a property
            if (!myProp || !otherProp
            ||  *myProp != *otherProp) {
                //properties differ or one of them is not a property
                return false;
            }
            //equal
            continue;
        }

        zChunk* myChunk    = dynamic_cast<zChunk*>(this->members[i]);
        zChunk* otherChunk = dynamic_cast<zChunk*>(other.members[i]);

        if(myChunk || otherChunk) {
            //one of them is a Chunk
            if (!myChunk || !otherProp
            ||  *myChunk != *otherChunk) {
                //Chunks differ or one of them is not a Chunk
                return false;
            }
            //equal
            continue;
        }

        FAULT("In operator== of zChunk: Unhandled chunk type?!");
    }

    return true;
}

//***************************
//  dtor
//***************************

zChunk::~zChunk() {
    for (unsigned int i = 0; i < members.size(); ++i) {
        members[i]->release();
    }
}
