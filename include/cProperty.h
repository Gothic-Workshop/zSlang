#ifndef CPROPERTY_H
#define CPROPERTY_H
#include<string>
#include<sstream>
#include<vector>
#include<set>
#include<map>

#define PROP_HASH
#ifdef PROP_HASH
    #include<boost/unordered_map.hpp>
    using boost::unordered_multimap;
#endif

#include<zChunkMember.h>
#include<cVariable.h>

using std::string;
using std::map;
using std::multimap;
using std::stringstream;
using std::set;
using std::vector;

class zUnarchiver;
class zArchiver;
class zChunk;
class zChunkSelectable;

class cProperty : public zChunkMember, public cVariable {
    public:
        //caution: getName is defined in both base classes!
        virtual std::string getName(void) const {
            return cVariable::getName();
        }

        /* update hashed data */
        virtual void notifyModified();
    public:
        /* output */
        virtual void archive(zArchiver &arc);

        /* read from ZEN File */
        static cProperty* read(zUnarchiver &arc, zChunk *owner,
                               const string &exptype = "",
                               const string &expname = "");
        /* read and write Ints from Zen file, no wrapping */
        static int  readInt  (zUnarchiver &arc, const string &expname = "");
        static void writeInt (zArchiver   &arc, const string &name, int value);

        virtual ~cProperty();

        zChunk * getOwningChunk() const { return owner; }
        std::string getTypeName() const { return typeName; };

    protected:
        cProperty(const string &name, const string &typeName,
                  zChunk *owner, const TValue value);
        void updated(); //call if value changed (hashtable update!)

        void checkfullyread(stringstream &stream);
        zChunk *owner;
        string typeName;
    private:
        struct THasherData {
            bool hashed;
            bool shouldBeHashed;
            string hashedWith;
        } hasherData;

        #ifdef PROP_HASH
            typedef unordered_multimap<string,zChunkSelectable*> propHashMap;
        #else
            typedef multimap<string,zChunkSelectable*> propHashMap;
        #endif
        typedef propHashMap::iterator propHashMapIt;

        class cPropertyHasher {
            public:
                cPropertyHasher();
                ~cPropertyHasher();
                bool getChunks(const string &name,
                               const string &value,
                               zWorld *world,
                               set<zChunkSelectable*> &result);
                bool wantToHash(const string &propertyName);
                void update(cProperty * prop);
                void remove(cProperty * prop);
            private:
                void insert(cProperty * prop);
                map<string,propHashMap*> maps;
        };
        static cPropertyHasher hasher;
    public:
        /* pass down to the hasher: */
        static bool getChunks (const string &name,
                               const string &value,
                               zWorld *world,
                               set<zChunkSelectable*> &result) {
            return hasher.getChunks(name, value, world, result);
        }
};

#endif // CPROPERTY_H
