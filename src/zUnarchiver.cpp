#include "zUnarchiver.h"
#include<cLogger.h>
#include <fstream>
#include <sstream>
#include <zChunk.h>
#include <zTransformApp.h>

zUnarchiver::zUnarchiver(const string &file, zWorld * world) : stream(file.c_str(), std::ios_base::in | std::ios_base::binary ), path(file), world(world) {
    if (!stream) {
        stringstream error;
        error << "Could not open the specified file. I tried the string itself and all directory prefixes specified in the ini file (DIRECTORIES.worldIncludePath). The following could not be found:";
        error << std::endl << file;

        string searchDirsRaw = zTransformApp::getAPP()->getOption("DIRECTORIES", "worldIncludePath");
        stringstream ss(searchDirsRaw);

        for(std::getline(ss, path, ';'); path != ""; getline(ss, path, ';')) {
            if(!path.length() || path[path.length() - 1] != '\\') path.push_back('\\');

            path += file;
            stream.close();
            stream.clear();
            stream.open(path.c_str(), std::ios_base::in | std::ios_base::binary);

            if (stream) {
                break;
            }
            error << std::endl << path;
            path = "";
        }

        if (!stream) {
            RUNTIME_ERROR(error.str());
        }
    }
}

void zUnarchiver::newChunk(int ID, zChunk* chunk) {
    /* unreferencable chunk, no need to remember. */
    if (!ID) {
        return;
    }

    if (chunks.find(ID) != chunks.end()) {
        PARSE_ERROR("Unarchiving an already unarchived chunk?!");
        return;
    }
    chunks.insert(pair<int,zChunk*>(ID, chunk));
};

zChunk* zUnarchiver::getChunk(int ID) {
    map<int,zChunk*>::iterator it = chunks.find(ID);
    if (it != chunks.end()) {
        return it->second;
    } else {
        LOG_ERROR("Requesting a non existing chunk.");
        return 0;
    }
}

void zUnarchiver::skip(const string &exptoken) {
    string token;
    stream >> token;

    if (token != exptoken) {
        PARSE_ERROR(string("'")+exptoken+"' expected but '"+token+"' found.");
    }
}

void zUnarchiver::skip(void) {
    string token;
    stream >> token;
}

void zUnarchiver::skipLine(void) {
    string token;
    getline(stream, token);
}

zUnarchiver::~zUnarchiver() {
    stream.close();
}

/* The stl gotta be fucking kidding me:
    Somehow the get pointer was wrong so I tried to debug.
    The more I observed the get pointer via tellg, the more it went wrong.
    Turns out tellg has some weird side effects that are not documented
    and probably due to some bug somewhere. I'm not the only one experiencing this
    problem, there are some other guys on the internet, most of which
    posted in some forum and were told that there's gotta be some bug in their code.

    And some other people claimed to fix it somewhere ten years ago...
    Seems like it somehow got through to me.

    May be related to querying the position where the next character is something
    ifstream does not like to see in non-binary mode...
    Solution is to always work in binary mode (no problem so far...) and
    not having to need to use tellg in the first place.
*/

    /*
void zUnarchiver::setMode(std::ios_base::openmode mode) {
    return;
    std::streampos pos = stream.tellg();

    stream.close();
    stream.open(path.c_str(), std::ios_base::in | mode);
    stream.seekg(pos);
}     */
