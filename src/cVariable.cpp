#include<sstream>
#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH
#include<boost/variant/apply_visitor.hpp>

#include<cVariable.h>
#include<cLogger.h>

std::string cVariable::toString() const {
    return ::toString(this->value);
}

TValueRef cVariable::getValueRef() {
    return TValueRef(&this->value, fragile ? this : 0);
}

//***************************
// Comparison:
//***************************

bool cVariable::operator==(const cVariable &other) const {
    return value == other.value;
}

int numvars;
cVariable::cVariable(const TValue& v, const std::string &name)
            : value(v), name(name), fragile(false) {
}

cVariable::~cVariable() {
}
