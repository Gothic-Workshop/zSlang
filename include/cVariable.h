#ifndef CVARIABLE_H
#define CVARIABLE_H

#include <hScriptTypes.h>
#include <cValueOrRef.h>

class cVariable {
    public:
        /* output */
        std::string toString() const;
        const TValue&   getValue()    const { return value; }
              TValueRef getValueRef();
        /* whenever a TValueRef changes the value in this variable
         * it has to call this function: */
        virtual void notifyModified() {};

        TType getType() {
            return ::getType(this->value);
        }

        cVariable(const TValue& v, const std::string &name);
        virtual ~cVariable();

        virtual std::string getName(void) const { return name; }

        bool operator==(const cVariable &other) const;
        bool operator!=(const cVariable &other) const {
            return !(operator==(other));
        }
    protected:
        TValue value;
        std::string name;
        bool fragile; //needs notification
};

#endif // CVARIABLE_H
