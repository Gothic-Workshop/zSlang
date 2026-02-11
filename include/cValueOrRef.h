#ifndef CVALUEORREF
#define CVALUEORREF

#include<hScriptTypes.h>
#include<boost/variant/get.hpp>

//***************************
//  A value
//***************************

class cVariable;

struct TValueRef {
    public:
        TValueRef(TValue *value, cVariable *container = 0)
                    :value(value), container(container) { }
        void assign(const TValue &val) const;
        const TValue& getValue() const { return *value; }
    private:
        TValue *value;
        cVariable* container;
    friend class cValueOrRef;
};

class cValueOrRef {
    public:
        typedef boost::variant<TValue, TValueRef> TValueOrRef;

        cValueOrRef(const TValueRef &value) : value(value) { }
        cValueOrRef(const TValue    &value) : value(value) { }

        cValueOrRef& assign(const TValue &val);
        bool assignable() {
            return boost::get<TValueRef>(&value) != 0;
        }
        const TValueRef& getAsTValueRef() const;
        bool isFragile() const; //returns whether or not the contets is
                                //a reference that needs notification
        TValue &getRef();
        const TValue& getVal() const;

        //follow [] or . access (changes this TValueOrRef!)
        //needed, because the container that wants to be
        //notified should not be forgotten.
        cValueOrRef& followIndex(int index);
        cValueOrRef& followMember(const std::string &name);
    private:
        TValueOrRef value;
        TValue & getNonConstVal();
};

#endif // CVALUEORREF
