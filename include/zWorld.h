#ifndef ZWORLD_H
#define ZWORLD_H

#include<string>
#include<stack>
#include<cSelection.h>

using std::string;
using std::stack;

class zWaynet;
class zVobtree;
class zChunk;
class TChunkFilter;
class zChunkSelectable;

class zWorld
{
    public:
        zWorld(const string &fileName);
        virtual ~zWorld();
        zVobtree * getVobtree() { return vobtree; }
        zWaynet  * getWaynet()  { return waynet;  }
        void merge(const string &fileName);

        /* archiving */
        void save         (const string &filename, cSelection* sel = 0);

        class iterator {
            public:
                iterator& operator ++();
                zChunkSelectable * operator *();
                bool operator ==(const iterator& b) const;
                bool operator !=(const iterator& b) const;

                enum TFlags {
                    fNone = 0,
                    fInWaynet = 1,
                    fEnd   = 2,
                };
            private:
                iterator(zWorld *world, TFlags flags);
                stack<cSelection::iterator> iteratorStack, endIteratorStack;

                void enterVobtree();
                void enterWaynet();

                TFlags flags;
                zWorld *world;
                zChunkSelectable * current;

            friend class zWorld;
        };

        iterator end() {
            return iterator(this, iterator::fEnd);
        }
        iterator beginWaynet() {
            return iterator(this, iterator::fInWaynet);
        }
        iterator begin() {
            return iterator(this, iterator::fNone);
        }


    protected:
    private:
        zChunk * worldData;
        zVobtree * vobtree;
        zWaynet  * waynet;

};

#endif // ZWORLD_H
