#include<hScriptTypes.h>
#include<cLogger.h>

#include<sstream>
#include<boost/variant/get.hpp>

#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH
#include<boost/variant/apply_visitor.hpp>

//***************************
// Getting the type of a value
//***************************

struct getTypeVisitor : boost::static_visitor<TType> {
    TType operator()(const double &d) const {
        return TYPE_DOUBLE;
    }
    TType operator()(const TVecVal &vec) const {
        return TYPE_ARRAY;
    }
    TType operator()(const std::string &s) const {
        return TYPE_STRING;
    }
    TType operator()(const int &i) const {
        return TYPE_INT;
    }
    TType operator()(const _void &v) const {
        return TYPE_VOID;
    }
    TType operator()(const TObjPtr &c) const {
        return TYPE_OBJECT;
    }
    TType operator()(const cSelection &s) const {
        return TYPE_SELECTION;
    }
    TType operator()(const TStructVal &s) const {
        return TYPE_STRUCT;
    }
    TType operator()(const _template &t) const {
        return TYPE_TEMPLATE;
    }
    TType operator()(const TFuncPtr &decl) const {
        return TYPE_FUNCTION;
    }
} typeVisitor;

TType getType(const TValue &val) {
    return boost::apply_visitor(typeVisitor, val);
}

//***************************
// resetting
//***************************

void resetToDefault(TValue &val) {
    TType t = getType(val);

    if (t == TYPE_STRUCT) {
        TStructVal &s = boost::get<TStructVal>(val);
        TStructVal::TStructMembers::iterator it = s.members.begin();
        for(; it != s.members.end(); ++it) {
            resetToDefault(it->second);
        }
    } else if (t == TYPE_ARRAY) {
        boost::get<TVecVal>(val).resetToDefault();
    } else switch (t) {
        case TYPE_INT :      val = int(0);          break;
        case TYPE_DOUBLE:    val = double(0);       break;
        case TYPE_STRING:    val = std::string(""); break;
        case TYPE_SELECTION: val = cSelection();    break;
        case TYPE_OBJECT:    val = TObjPtr(0);
        case TYPE_FUNCTION:  val = (TFuncDeclaration*)(0); break;
        case TYPE_TEMPLATE:         //nothing to be done
        case TYPE_VOID:      break; //nothing to be done
        case TYPE_ARRAY: case TYPE_STRUCT: case TYPE_MAX:
        default: FAULT("Unimplemented case in resetToDefault!");
    }
}

//***************************
//  to String
//***************************

struct ValueToStringVisitor : boost::static_visitor<> {
    mutable std::stringstream ss;
    ValueToStringVisitor() {};

    /* special function for double (other formatting) */
    void operator()(const double &d) const {
        char buf[30];
        sprintf(buf, "%.9g", d);
        ss << buf;
    }

    void operator()(const TValue &val) const {
        boost::apply_visitor(*this, val);
    }

    void operator()(const TVecVal &vec) const {
        //ss << '{'; //<- toString is used for printing zen files, use exact formatting.
        for(unsigned int i = 0; i < vec.size(); ++i) {
            if (i) ss << " ";
            (*this)(vec[i]);
        }
        //ss << '}';
    }

    void operator()(const TStructVal &s) const {
        ss << "struct " << s.structName << " {";
        TStructVal::TStructMembers::const_iterator it = s.members.begin();
        for(; it != s.members.end(); ) {
            (*this)(it->second);
            ++it;
            if (it != s.members.end()) {
                ss << ", ";
            }
        }
        ss << "}";
    }

    void operator()(const _void &v) const {
        ss << "[void]";
    }

    /* generic */
    template<typename T>
    void operator()(const T &v) const {
        ss << v;
    }

    void operator()(const TObjPtr &c) const {
        FAULT("Trying to convert TObjPtr to std::string.");
    }
    void operator()(const cSelection &s) const {
        FAULT("Trying to convert cSelection to std::string.");
    }
    void operator()(const _template &t) const {
        ss << "[uninitialised template]";
    }
    void operator()(const TFuncPtr &decl) const {
        ss << "&" << decl->identifier;
    }
};

std::string toString(const TValue &val) {
    ValueToStringVisitor v;
    boost::apply_visitor(v, val);
    return v.ss.str();
}

//***************************
//  Vector assignment
//***************************

void assignValue(TValue &dest, const TValue &source) {
    using boost::get;
    /* perform type check */
    TType tl = getType(dest);
    TType tr = getType(source);

    if (tr == TYPE_TEMPLATE) {
        RUNTIME_ERROR("Trying to assign an uninitialised template to something.");
    }

    if (tl != tr) {
        if (tl == TYPE_TEMPLATE) {
            dest = source; //whatever it is
        } else if (tl == TYPE_INT && tr == TYPE_DOUBLE) {
            dest = int(get<double>(source));
        } else if (tl == TYPE_DOUBLE && tr == TYPE_INT) {
            dest = double(get<int>(source));
        } else if (tl == TYPE_STRING &&
            (tr == TYPE_INT || tr == TYPE_DOUBLE)) {
            dest = toString(source);
        } else if (tl == TYPE_SELECTION && tr == TYPE_OBJECT) {
            cSelection sel; sel.insert(get<TObjPtr>(source));
            dest = sel;
        } else if ((tl == TYPE_OBJECT || tl == TYPE_FUNCTION) && tr == TYPE_INT && boost::get<int>(source) == 0) {
            if (tl == TYPE_OBJECT) {
                dest = TObjPtr(0);
            } else {
                dest = TFuncPtr(0);
            }
        } else {
            RUNTIME_ERROR(string("Value of type ")
                          + typeNames[tr]
                          + " cannot be assigned to value of type "
                          + typeNames[tl] + ".");
        }
    } else if (tl == TYPE_STRUCT) {
              TStructVal &s1 = get<TStructVal>(dest);
        const TStructVal &s2 = get<TStructVal>(source);

        if (s1.structName != s2.structName) {
            RUNTIME_ERROR(string("Cannot assign struct of type \"")
                                + s1.structName + "\" to struct of type \""
                                + s2.structName + "\".");
        }
        s1 = s2;
    } else if (tl == TYPE_ARRAY) {
        TVecVal &vec = get<TVecVal>(dest);
        vec.assign(get<TVecVal>(source));
    } else {
        dest = source;
    }
}

void TVecVal::assign(const TVecVal &other) {
    if(this->fixedSize && other.size() != this->size()) {
        std::stringstream ss;
        ss << "Trying to assign an array with the size of "
           << other.size()
           << " to an array with FIXED size of "
           <<  this->size()
           <<  ".";
        RUNTIME_ERROR(ss.str());
    }

    TType tl = getType(this->defaultValue.get());
    TType tr = getType(other.defaultValue.get());

    ASSERT(tl != TYPE_TEMPLATE && tr != TYPE_TEMPLATE,
           string("How did you manage to get an array of templates?!")
           + " This is currently not supported!");


    if (this->size() != other.size()) {
        this->resize(other.size());
    }

    for(uint i = 0; i < vec.size(); ++i) {
        assignValue((*this)[i], other[i]);
    }

    /*  nah... better allow this. This makes some things easier.
     *  such as returning {}
    if (this->size() == 0) {
        //dont allow assignment of empty string array to int arrays
        //or similar bullshit:
        TValue defCpy = defaultValue.get();
        assignValue(defCpy, other.defaultValue.get());
    } */
}

//***************************
//  TVecVal basics
//***************************

/* Remember for the future: If you got classes with pointer members,
 * ALWAYS define the copy constructor and the assignment operator or
 * explicitly disable them. They WILL be called when you expect it least
 * and you will not see the crash until one of the object goes out of
 * scope. */

TVecVal::~TVecVal() {
    this->resize(0); //delete all values
}

void TVecVal::resize(uint newSize) {
    if (newSize < size()) {
        for(uint i = newSize; i < vec.size(); ++i) {
            delete vec[i];
        }
        vec.resize(newSize);
    } else {
        for(uint i = vec.size(); i < newSize; ++i) {
            vec.push_back(new TValue(defaultValue.get())); //copy construct default value
        }
    }
}

void TVecVal::resetToDefault() {
    if (!fixedSize) {
        resize(0);
    } else {
        for (uint i = 0; i < size(); ++i) {
            (*this)[i] = defaultValue.get();
        }
    }
}

TVecVal::TVecVal(const std::vector<TValue> &valVec, const TValue &defaultValue, bool fixedSize)
                : defaultValue(defaultValue), fixedSize(fixedSize) {
    for(uint i = 0; i < valVec.size(); ++i) {
        vec.push_back(new TValue(valVec[i]));
    }
}

TVecVal::TVecVal(const TVecVal &other) : defaultValue(other.defaultValue), fixedSize(other.fixedSize) {
    resize(0); //evtl vorhandene Werte freigeben? Oder werden mit Kopierkonstruktoren immer *neuer* Objekte erzeugt?
    vec.resize(other.size());
    for(uint i = 0; i < other.size(); ++i) {
        vec[i] = new TValue(other[i]);
    }
}

TVecVal::TVecVal(const TValue &defaultValue, uint size, bool fixedSize): defaultValue(defaultValue), fixedSize(fixedSize) {
    resize(size);
}

TVecVal& TVecVal::operator=(const TVecVal &other) {
    resize(0);
    defaultValue = other.defaultValue.get();
    fixedSize = other.fixedSize;
    vec.resize(other.size());

    for(uint i = 0; i < other.size(); ++i) {
        vec[i] = new TValue(other[i]);
    }
    return *this;
}

//***************************
//  Adding / Appending
//***************************

TVecVal& TVecVal::push_back(const TValue &val) {
    if(fixedSize) {
        RUNTIME_ERROR("Cannot append element to array of fixed size.");
    }

    resize(size() + 1);
    assignValue((*this)[size()-1], val);
    return *this;
}

TVecVal& TVecVal::concat(const TVecVal &other) {
    if(fixedSize) {
        RUNTIME_ERROR("Cannot append element to array of fixed size.");
    }

    /* thats possibly complicated or invalid because some dimensions may
     * or may not be fixed in the different vectors:
     * var int someColors[][3];
     * var int someAlphaColours[][4];
     * ...
     * someColors += someAlphaColours; //must not work!
     *
     * The following code however:
     * var int someColours[][];
     * var int myColour[4];
     * someColours |= myColour; //o.k.
     * must work, but the dimension fixation must be dropped! */

    uint oldSize = size();
    resize(oldSize + other.size());

    for(uint i = 0; i < other.size(); ++i) {
        assignValue((*this)[i+oldSize], other[i]);
    }
    return *this;
}

//***************************
// comparison
//***************************

bool TVecVal::operator!=(const TVecVal &other) const {
    return !(*this == other);
}
bool TVecVal::operator==(const TVecVal &other) const {
    if (size() != other.size()) {
        return false;
    }

    for (unsigned int i = 0; i < size(); ++i) {
        if ((*this)[i] != other[i]) {
            return false;
        }
    }

    return true;
}

bool TStructVal::operator==(const TStructVal &other) const {
    if (structName != other.structName) {
        return false;
    }

    return members == other.members;
}

bool operator!=(const TValue &v1, const TValue &v2) {
    return !(v1 == v2);
}

//***************************
// Fits into Decl?
//***************************

void checkRightMatchesLeft(const TValue &left, const TValue &right) {
    /* this is for checking whether a reference is initialised correctly */

    if(getType(left) == TYPE_TEMPLATE) {
        return;
    }

    const TValue *pLeft = &left, *pRight = &right;
    while(true) {
        using boost::get;

        TType tl = getType(*pLeft), tr = getType(*pRight);

        if (tl != tr) {
            RUNTIME_ERROR(string("Type missmatch: ") + typeNames[tl] + " != " + typeNames[tr] + ".");
        }

        if (tl == TYPE_ARRAY) {
            const TVecVal *vecL = get<TVecVal>(pLeft);
            const TVecVal *vecR = get<TVecVal>(pRight);

            if(vecL->fixedSize && vecR->size() != vecL->size()) {
                std::stringstream ss;
                ss << "Vector of size " << vecR->size()
                   << " cannot be assigned to vector of FIXED size "
                   << vecL->size() << ".";
                RUNTIME_ERROR(ss.str());
            }
            pLeft  = &vecL->defaultValue.get();
            pRight = &vecR->defaultValue.get();
        } else if (tl == TYPE_STRUCT) {
            const TStructVal &s1 = get<TStructVal>(*pLeft);
            const TStructVal &s2 = get<TStructVal>(*pRight);

            if (s1.structName != s2.structName) {
                RUNTIME_ERROR(string("Cannot assign struct of type \"")
                                + s1.structName + "\" to struct of type \""
                                + s2.structName + "\".");
            }
            return;
        } else {
            return; //ok
        }
    }
}

//***************************
//  Dimensions of vector
//***************************

int TVecVal::dim() const {
    int dim = 1;

    const TValue *val = &defaultValue.get();
    const TVecVal *vecVal;
    while ((vecVal = boost::get<TVecVal>(val))) {
        val = &vecVal->defaultValue.get();
        dim++;
    }
    return dim;
}

