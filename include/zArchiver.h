#ifndef ZARCHIVER_H
#define ZARCHIVER_H

#include<string>
#include<fstream>
#include<map>

using std::string;
using std::ifstream;
using std::map;
using std::ios_base;
using std::ofstream;
using std::ostream;
using std::endl;

class cSelection;
class zChunk;
class zChunkSelectable;

class zArchiver
{
    /* The archiver knows the selection of vobs that should be archived.
     * However, it is upon the objects to use this information.
     * The archiver could not handle selections on his own anyway
     * since waynet and vobtree _properties_ depend on the selection. */
    public:
        zArchiver(const string &path, cSelection *sel = 0);
        void setMode(ios_base::openmode mode);
        bool isSelected(zChunkSelectable *chunk);
        void ENDL();
        /* return value:
         * true if chunk has already been archived, ID set to existing ID
         * false if chunk has not yet been archived. New ID created. */
        bool getID(zChunk* chunk, int &ID);
        ostream &getStream() { return stream; }
        ~zArchiver();

        string indentation();

        class Indenter {
            public:
            Indenter(zArchiver *arc) {
                this->arc = arc;
                arc->indent();
            }
            ~Indenter() {
                arc->unindent();
            }

            private:
                zArchiver *arc;
        };
    protected:
    private:
        ofstream stream;
        cSelection *sel;
        string path;
        map<zChunk*,int> chunkIDs;
        int nextFreeID;
        int currIndentation;

        static const string tabs;
        void indent() { currIndentation++; }
        void unindent() { currIndentation--; }
};

#define INDENTARCHIVE zArchiver::Indenter indenter(&arc)

#endif // ZARCHIVER_H
