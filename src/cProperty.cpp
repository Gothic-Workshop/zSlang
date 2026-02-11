#include "cProperty.h"
#include <sstream>
#include<cLogger.h>
#include <zChunkSelectable.h>
#include <zArchiver.h>
#include <zUnarchiver.h>

#include<boost/variant/get.hpp>

//***************************
//  Property Hasher
//***************************

cProperty::cPropertyHasher cProperty::hasher;

cProperty::cPropertyHasher::cPropertyHasher() {
    #define newMap(s) maps.insert(pair<string,propHashMap*>(s, new propHashMap))
        newMap("vobName");
        newMap("visual");
        newMap("wpName");
    #undef newMap
}

cProperty::cPropertyHasher::~cPropertyHasher() {
    map<string,propHashMap*>::iterator it = maps.begin();

    for (; it != maps.end(); ++it) {
        delete it->second;
    }
}

bool cProperty::cPropertyHasher::wantToHash(const string &name) {
    return maps.find(name) != maps.end();
}

cProperty::cProperty(const string &name, const string &typeName,
                     zChunk *owner, const TValue value)
                     : cVariable(value, name){
    this->typeName = typeName;
    this->owner = owner;
    this->hasherData.hashed = false;
    this->fragile |= this->hasherData.shouldBeHashed =
           cProperty::hasher.wantToHash(name)
        && dynamic_cast<zChunkSelectable*>(owner);
    this->notifyModified(); //initialisieren ist auch updaten.
}

void cProperty::notifyModified() {
    this->hasher.update(this);
}

cProperty::~cProperty() {
    hasher.remove(this);
}

#ifdef PROP_HASH
    #define GETHIGHLOW(n,v) \
        propHashMap *propMap = maps.find(n)->second;\
        std::pair<propHashMapIt,propHashMapIt> lowhigh = propMap->equal_range(v);\
        propHashMapIt &low  = lowhigh.first;\
        propHashMapIt &high = lowhigh.second;
#else
    #define GETHIGHLOW(n,v) \
        propHashMap *propMap = maps.find(n)->second;\
        propHashMapIt low, high;\
        low = propMap->lower_bound(v);\
        high = propMap->upper_bound(v)
#endif

bool cProperty::cPropertyHasher::getChunks(const string &name,
                                           const string &value,
                                           zWorld *world,
                                           set<zChunkSelectable*> &result) {
    if (!wantToHash(name)
    ||  value == "") {
        return false; //cant help in these cases
    }

    GETHIGHLOW(name, value);

    for (; low != high; ++low) {
        if (low->second->getWorld() == world) {
            result.insert(low->second);
        } else {
            WARN(string("Somebodies searching for vobs in a certain world ")
                 + "but vobs of a different world still linger in the hashtable. "
                 + " Why? Leaks? Incoming shitstorm? Sekti will be "
                 + " interested in this. It's probably not your fault.");
        }
    }
    return true;
}

void cProperty::cPropertyHasher::remove(cProperty *prop) {
    if (prop->hasherData.hashed) {
        GETHIGHLOW(prop->getName(), prop->hasherData.hashedWith);

        for (; low != high; ++low) {
            if(low->second == prop->getOwningChunk()) {
                prop->hasherData.hashed = false;
                propMap->erase(low);
                return;
            }
        }

        FAULT("Trying to delete a property from a hash map that isn't there.");
    }
}

#undef GETHIGHLOW

void cProperty::cPropertyHasher::insert(cProperty *prop) {
    ASSERT(!prop->hasherData.hashed, "Hashing already hashed Property!?");

    string newValue = prop->toString();
    if (newValue == "") {
        return;
    }

    DEBUGINFO(string("Hashing property '")
         +prop->getName()+"' with value '"
         +newValue+"'.");

    zChunkSelectable *chunk;
    chunk = dynamic_cast<zChunkSelectable*>(prop->getOwningChunk());
    ASSERT(chunk, "Hashing a property of a non selectable chunk?!");

    propHashMap *propMap = maps.find(prop->getName())->second;
    propMap->insert(pair<string,zChunkSelectable*>(newValue, chunk));

    prop->hasherData.hashed = true;
    prop->hasherData.hashedWith = newValue;
}

void cProperty::cPropertyHasher::update(cProperty *prop) {
    if (!prop->hasherData.shouldBeHashed) {
        return;
    }

    remove(prop);
    insert(prop);
}

//***************************
// Generic
//***************************

void cProperty::archive(zArchiver &arc) {
    ostream &stream = arc.getStream();
    stream  << arc.indentation()
            <<getName()<<"="<<getTypeName()<<":"<<toString()<<endl;
}

void cProperty::checkfullyread(stringstream &stream) {
    /* check whether the stream died or is incompletely read */
    if (stream.fail()) {
        string str = "Input stream failed while reading property of type '";
               str += typeName;
               str += "'. Already read: '" + this->toString() + "'.";
        PARSE_ERROR(str);
    }

    /* check whether the stream is empty */
    if (!stream.eof()) {
        char c; stream >> c;

        if (!stream.fail()) {
            stream.putback(c);

            char rest[stream.str().length()+1];
            stream.getline(&rest[0], stream.str().length());
            WARN(string("Property read but line goes on. Reading type: '")
                 +this->getTypeName() + "', already  read: '"
                 +this->toString() + "', left: '"
                 +rest+"'.");
        }
    }
}

template<typename T>
TVecVal readVector(stringstream &ss) {
    T val = T();
    TVecVal v(val);
    ss >> val;
    while(!ss.fail()) {
        v.push_back(val);
        ss >> val;
    }
    ss.clear(); //alles gut
    return v;
}

template<typename T>
T readSingleValue(stringstream &ss) {
    T v;
    ss >> v;
    return v;
}

cProperty* cProperty::read(zUnarchiver &arc, zChunk *owner,
                           const string &exptype /* = ""*/,
                           const string &expname /* = ""*/) {
    /* skip whitespaces (theres got to be a better way to do it...) */
    char c;
    (arc.getStream()) >> c; //skip whitespaces
    arc.getStream().putback(c); //put the character back.

    /* get the rest of the line*/
    string line;
    std::getline(arc.getStream(), line);

    /* cut it */
    string name; string type;
    stringstream valSS(line);
    std::getline(valSS, name, '=');
    std::getline(valSS, type, ':');

    if ((exptype != "" && exptype != type)
    ||  (expname != "" && expname != name)) {
        PARSE_ERROR(string("Expected property of type '")+exptype
                    +"' and name '"+expname
                    +"' but found property of type '"+type
                    +"' and name '"+ name+"'.");
    }

    TValue val;

    #define TRYTYPE(typeName, T, f)\
                       if (type == typeName) { \
                           val = f<T>(valSS);\
                       } else

    if((type == "string") || (type == "raw")) {
        string str;
        getline (valSS, str);
        valSS.clear(); //there is no end of line, but this is no fail
        val = str;
    } else
    TRYTYPE("float",   double, readSingleValue)
    TRYTYPE("int",     int,    readSingleValue)
    TRYTYPE("bool",    int,    readSingleValue)
    TRYTYPE("enum",    int,    readSingleValue)
    TRYTYPE("vec3",    double, readVector)
    TRYTYPE("rawFloat",double, readVector)
    TRYTYPE("color",   int,    readVector)
    {
        //if everything fails.
        PARSE_ERROR("Unknown type identifier: " + type);
    }
    #undef TRYTYPE

    cProperty *res = new cProperty(name, type, owner, val);
    res->checkfullyread(valSS);

    DEBUGINFO("Property read. name:'"+name+"', type: '"+res->getTypeName()+"'.");

    return res;
}

//***************************
//  Read ints (static)
//***************************

int cProperty::readInt(zUnarchiver &arc, const string &expname /* = ""*/) {
    cProperty *prop = cProperty::read(arc, 0, "int", expname);

    //if cProperty::read does not throw, it has to deliver,
    //I can safely "get" here.
    int res = boost::get<int>(prop->getValue());

    prop->release();
    return res;
}

void cProperty::writeInt(zArchiver &arc, const string &name, int value) {
    /* fake Property, gebraucht vom Waynet und Vobtree */
    ostream &stream = arc.getStream();
    stream << arc.indentation()
           << name <<"=int:"<<value<<endl;
}
