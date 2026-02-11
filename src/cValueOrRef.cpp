#include<cValueOrRef.h>
#include<cLogger.h>
#include<sstream>
#include<zChunkSelectable.h>
#include<cSelection.h>
#include<cVariable.h>
#include<cProperty.h>

#include<boost/variant/get.hpp>
#include<boost/variant/apply_visitor.hpp>

/* needed for dynamic casting */
#include<zChunkWP.h>
#include<zChunkVob.h>

TValue& cValueOrRef::getNonConstVal() {
    using boost::get;

    TValue* v     = get<TValue>   (&this->value);
    TValueRef* vr = get<TValueRef>(&this->value);

    ASSERT(v || vr, "cValueOrRef is neither value nor ref?!");

    return v ? *v : *vr->value;
}

const TValue& cValueOrRef::getVal() const {
    using boost::get;

    const TValue* v     = get<TValue>   (&this->value);
    const TValueRef* vr = get<TValueRef>(&this->value);

    ASSERT(v || vr, "cValueOrRef is neither value nor ref?!");

    return v ? *v : *vr->value;
}

bool cValueOrRef::isFragile() const {
    using boost::get;

    const TValueRef* vr = get<TValueRef>(&this->value);
    return vr && vr->container;
}

TValue& cValueOrRef::getRef() {
    if (isFragile()) {
        FAULT("Cannot publish non-const reference to fragile value.");
    }

    using boost::get;
    TValue* v     = get<TValue>   (&this->value);
    TValueRef* vr = get<TValueRef>(&this->value);

    return v ? *v : *vr->value;
}

cValueOrRef& cValueOrRef::followIndex(int index) {
    using boost::get;

    TValue   * v  = get<TValue>   (&this->value);
    TValueRef* vr = get<TValueRef>(&this->value);

    ASSERT(v || vr, "cValueOrRef is neither value nor ref?!");

    TVecVal* vec = v ? get<TVecVal>(v) : get<TVecVal>(vr->value);

    if(!vec) {
        RUNTIME_ERROR("Index operator [] applied to value that is not an array.");
    }
    if (index < 0 || ((int)vec->size()) <= index) {
        std::stringstream ss;
        ss << "Array index out of bounds! Size: "
           << vec->size()
           << ", index: "
           << index << '.';
        RUNTIME_ERROR(ss.str());
    }

    if(v) {
        /* discard the vector, remember only one entry */
        this->value = (*vec)[index];
        return *this;
    } else {
        /* update reference */
        vr->value = &(*vec)[index];
        return *this;
    }
}

struct TMemberFollowVisitor : boost::static_visitor <cValueOrRef::TValueOrRef>{
    typedef cValueOrRef::TValueOrRef TValueOrRef;

    private:
        std::string member;

        template<typename T>
        void fail(const T &val) {
            std::stringstream ss;
            ss << "Value of type "
               << typeNames[SCRIPT_TYPE_OF(val)]
               << " does not have a member with name \""
               << member << "\".";
            RUNTIME_ERROR(ss.str());
        }
        #define FAIL fail(v); return _void()
    public:
        TMemberFollowVisitor(const std::string &member) : member(member) {}

        TValueOrRef operator()(std::string &v) {
            if(member == "length") {
                return TValue(int(v.length()));
            }
            FAIL;
        }
        TValueOrRef operator()(TVecVal &v) {
            if(member == "size") {
                return TValue(int(v.size()));
            }
            FAIL;
        }
        TValueOrRef operator()(cSelection &v) {
            if(member == "size") {
                return TValue(int(v.size()));
            }
            FAIL;
        }
        TValueOrRef operator()(TStructVal &s) {
            TStructVal::TStructMembers::iterator it = s.members.find(member);
            if (it == s.members.end()) {
                std::stringstream ss;
                ss << "The struct with the name \""
                   << s.structName
                   << "\" does not have a member with name \""
                   << member << "\".";
                RUNTIME_ERROR(ss.str());
            }
            return TValueRef(&it->second);
        }
        TValueOrRef operator()(TObjPtr &v) {
            if (!v) {
                RUNTIME_ERROR("Requesting member \"" + member + "\" of uninitialized object variable.");
            } else if (!zChunkSelectable::isLiving(v)) {
                RUNTIME_ERROR("Trying to access member \"" + member + "\" of object that was destroyed previously.");
            }

            if(member == "name") {
                if (dynamic_cast<zChunkWP*>(v)) {
                    member = "wpName";
                } else {
                    member = "vobName";
                }
            } else if (member == "parent") {
                TObjPtr parent = v->getParent();
                if (parent && parent->getParent()) {
                    return TValue(parent);
                } else {
                    // !parent ==> wp
                    // !parent->getParent() ==> vob which has vobtree as parent (no real parent)
                    return TValue(TObjPtr(0));
                }
            } else if (member == "childs") {
                return TValue(v->getChilds());
            } /*else if (member == "childsArr") { //sinnlos
                TVecVal childVec(TObjPtr(0));

                const cSelection& sel = v->getChilds();
                for(cSelection::iterator it = sel.begin(); it != sel.end(); ++it) {
                    childVec.push_back(*it);
                }
                return TValue(childVec);
            } */else if (member == "pos" || member == "pos2D" /*|| member == "pos3D"*/) {
                bool is2D = member == "pos2D";

                if (dynamic_cast<zChunkWP*>(v)) {
                    member = "position";
                } else {
                    member = "trafoOSToWSPos";
                }

                if (is2D) {
                    //gibt es nicht wirklich.
                    cVariable *var = v->getProperty(member);
                    if (var) {
                        if (getType(var->getValue()) == TYPE_ARRAY) {
                            TVecVal val = boost::get<TVecVal>(var->getValue());

                            if (val.size() == 3) {
                                val[1] = val[2];
                                val.fixedSize = false;
                                val.resize(2);
                                return TValueOrRef(val);
                            }
                        }
                    }
                }
            } else if (member == "className") {
                std::stringstream ss(v->getClassName());
                string name;
                getline(ss, name, ':');
                return TValue(name);
            } else if (member == "classHierarchy") {
                return TValue(v->getClassName());
            }

            cVariable* var = v->getProperty(member);

            if(!var) {
                RUNTIME_ERROR("Requested property \"" + member
                              + "\" does not exist.");
            }

            return var->getValueRef();
        }

        TValueOrRef operator()(TFuncPtr &v) {
            if(member == "numParams") {
                if (v == 0) {
                    RUNTIME_ERROR("Cannot access the \"numParams\" of a function pointer pointing to nothing.");
                }
                return TValue((int)(*v).params.size());
            }
            FAIL;
        }

        template<typename T>
        TValueOrRef operator()(const T &v) {
            FAIL;
        }
        #undef FAIL
};

cValueOrRef& cValueOrRef::followMember(const std::string &name) {
    if (name == "nullVal") {
        if (boost::get<TValue>(&this->value) == 0) {
            this->value = this->getVal(); //Cpy
        }
        resetToDefault(boost::get<TValue>(this->value));
        return *this;
    }

    TMemberFollowVisitor v(name);
    this->value = boost::apply_visitor(v, this->getNonConstVal());
    return *this;
}

const TValueRef& cValueOrRef::getAsTValueRef() const {
    const TValueRef *res = boost::get<TValueRef>(&this->value);
    ASSERT(res, "cValueOrRef::getAsTValueRef was called on r-value!");
    return *res;
}

cValueOrRef& cValueOrRef::assign(const TValue &val) {
    ASSERT(assignable(), "Trying to assign value to non-assignable lvalue.");
    boost::get<TValueRef>(this->value).assign(val);
    return *this;
}

void TValueRef::assign(const TValue &val) const {
    using boost::get;

    ::assignValue(*this->value, val);

    if (this->container) {
        this->container->notifyModified();
    }
}
