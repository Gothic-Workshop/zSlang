#include "cInterpreter.h"
#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH
#include<boost/variant/apply_visitor.hpp>
#include<boost/variant/get.hpp>
#include<cLogger.h>
#include<cSelection.h>
#include<zChunkSelectable.h>
#include<TExternals.h>
#include<zWorld.h>

#include<TCode_Printer.h>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/home/phoenix/operator.hpp>
#include <boost/spirit/home/phoenix/statement.hpp>
#include <boost/spirit/home/phoenix/object.hpp>
#include <boost/spirit/home/phoenix/function.hpp>

#include<boost\regex.hpp>
#include<sstream>

//***************************
//  THE REAL INTERPRETATION
//***************************

struct cInterpreter::TToBoolVisitor: boost::static_visitor<bool> {
    bool operator()(const TValue &v) {
        return boost::apply_visitor(*this, v);
    }

    #define FAIL(type)


    bool operator()(const _void &v) {
        RUNTIME_ERROR(string("Conversion of type ")
                      + typeNames[SCRIPT_TYPE_OF(v)]
                      + " to bool requested but not implemented.");
    }
    bool operator()(const _template &v) {
        RUNTIME_ERROR(string("Conversion of type ")
                      + typeNames[SCRIPT_TYPE_OF(v)]
                      + " to bool requested but not implemented.");
    }

    bool operator()(const TVecVal &v) {
        RUNTIME_ERROR(string("Conversion of type ")
                      + typeNames[SCRIPT_TYPE_OF(v)]
                      + " to bool requested but not implemented.");
    }
    bool operator()(const TStructVal &v) {
        RUNTIME_ERROR(string("Conversion of type ")
                      + typeNames[SCRIPT_TYPE_OF(v)]
                      + " to bool requested but not implemented.");
    }
    bool operator()(const std::string &s) {
        return !s.empty();
    }
    bool operator()(const cSelection &s) {
        return s.size();
    }
    bool operator()(const TObjPtr chunk) {
        return chunk && zChunkSelectable::isLiving(chunk);
    }
    template<typename T>
    bool operator()(const T& t) {
        return t;
    }
} toBool;

//Compute a Value:
struct cInterpreter::TExpressionVisitor : boost::static_visitor<cValueOrRef> {
    cInterpreter *interp;

    TExpressionVisitor(cInterpreter *interp) : interp(interp) {};

    cValueOrRef operator()(const TConstant &c) const {
        return boost::apply_visitor(*this, c);
    }
    cValueOrRef operator()(const int &i) const {
        return cValueOrRef(TValue(i));
    }
    cValueOrRef operator()(const double &d) const {
        return cValueOrRef(TValue(d));
    }
    cValueOrRef operator()(const TStringConst &s) const {
        return cValueOrRef(TValue(s.str));
    }

    cValueOrRef operator()(const TIdentifier &id) const {
        TFuncTable::iterator it;
        if((interp->hasVar(id))) {
            return interp->varStack.getRef(id);
        } else if ((it = interp->functions.find(id)) != interp->functions.end()) {
            return TValue(it->second);
        } else {
            /* let the varStack print the error message (unknown identifier) */
            return interp->varStack.getRef(id);
        }
    }

    bool unify(TValue &v1, TValue &v2) const {
        //requirement: v1 and v2 are valid (if they are vectors the default value is valid)
        //             unify must be symmetric
        //             the "is more general than" relation must be transitive
        //             and antisymmetric
        //             unify must not make v1 and v2 less general.

        using boost::get;

        TType t1 = getType(v1);
        TType t2 = getType(v2);

        if (t1 != t2) {
            if (t1 == TYPE_INT && t2 == TYPE_DOUBLE) {
                v1 = double(boost::get<int>(v1));
                return true;
            } else if (t1 == TYPE_DOUBLE && t2 == TYPE_INT) {
                v2 = double(boost::get<int>(v2));
                return true;
            }
            //other conversions? I guess not.
            RUNTIME_ERROR(string("Error constructing array. Could not unify the types ")
                          + typeNames[t1]
                          + " and "
                          + typeNames[t2] + ".");
            return true;
        } else if (t1 == TYPE_ARRAY) {
            TVecVal &vec1 = get<TVecVal>(v1);
            TVecVal &vec2 = get<TVecVal>(v2);

            bool didSth;
            if ((didSth = unify(vec1.defaultValue.get(), vec2.defaultValue.get()))) {
                //There was need to unify them.
                //Unify every element with the default values:
                for(uint i = 0; i < vec1.size(); ++i) {
                    unify(vec1[i], vec1.defaultValue.get());
                }
                for(uint i = 0; i < vec2.size(); ++i) {
                    unify(vec2[i], vec2.defaultValue.get());
                }
            }

            bool fixedSize = vec1.size() == vec2.size() && vec1.fixedSize && vec2.fixedSize;
            didSth |= (vec1.fixedSize != fixedSize || vec2.fixedSize != fixedSize);
            vec1.fixedSize = vec2.fixedSize = fixedSize;
            return didSth;
        } else {
            //The same primitive types? Its ok.
            return false;
        }
    }
    cValueOrRef operator()(const TExpressionList &list) const {
        if(list.list.size() == 0) {
            /* RUNTIME_ERROR(string("The empty array constructor '{ }'")
                + " is an illegal expression because it cannot be typed (sorry)."); */
            //default-type it as an int array:
            return cValueOrRef(TValue(TVecVal(std::vector<TValue>(), TValue(0))));
        }

        std::vector<TValue> vec;
        for(uint i = 0; i < list.list.size(); ++i) {
            vec.push_back((*this)(list.list[i]).getVal());
        }

        //first run assures that vec[0] is most general
        bool unified = false;
        for(uint i = 1; i < list.list.size(); ++i) {
            unified |= unify(vec[0], vec[i]);
        }
        //second run assures that everything is as general as vec[0]
        if (unified) {
            for(uint i = 1; i < list.list.size(); ++i) {
                unify(vec[0], vec[i]);
            }
        }

        //should not be fixed size, or assigning this to a template will make it have fixed size as well.
        return cValueOrRef(TVecVal(vec, interp->createDefaultValue(vec[0]), false));
    }
    cValueOrRef operator()(const TExpression &exp) const {
        return boost::apply_visitor(*this, exp);
    }
    cValueOrRef operator()(const TFuncCall &call) const {
        TValue funcExp = (*this)(call.funcExp).getVal();
        TType t = getType(funcExp);

        if (t != TYPE_FUNCTION) {
            RUNTIME_ERROR(string("Cannot call value of type ") + typeNames[t] + " (only functions can be called).");
        }

        TFuncDeclaration* fun = boost::get<TFuncDeclaration*>(funcExp);
        if (!fun) {
            RUNTIME_ERROR(string("Cannot call uninitialised function (cannot call null)."));
        }

        return interp->callFunction(fun, call.params);
    }

    bool applyUnaryMinus(TValue &val) const {
        TType t = getType(val);

        using boost::get;
        switch(t) {
            case TYPE_INT:      get<int>(val) *= -1;    return true;
            case TYPE_DOUBLE:   get<double>(val) *= -1; return true;
            case TYPE_ARRAY: {
                TVecVal &v = get<TVecVal>(val);
                for(uint i = 0; i < v.size(); ++i) {
                    if (!applyUnaryMinus(v[i])) {
                        return false;
                    }
                }
                return true;
            } default:
                return false;
        }
    }

    bool applyUnaryPlus(TValue &val) const {
        TType t = getType(val);

        if (t == TYPE_INT || t == TYPE_DOUBLE) {
            return true;
        }
        using boost::get;
        if (t == TYPE_ARRAY) {
            TVecVal &v = get<TVecVal>(val);
            for(uint i = 0; i < v.size(); ++i) {
                if (!applyUnaryPlus(v[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    cValueOrRef operator()(const TUnaryCalc &calc) const {
        cValueOrRef val = (*this)(calc.exp);

        try {
            if (calc.op == OP_NOT) {
                return TValue(!toBool(val.getVal()));
            } else if (calc.op == OP_UNARYMINUS || calc.op == OP_UNARYPLUS) {
                TValue res = val.getVal();
                bool s = calc.op == OP_UNARYMINUS ? applyUnaryMinus(res) : applyUnaryPlus(res);

                if (!s) { goto nope; }

                return res;
            } else {
                /* my fault */
                FAULT(string("Unimplemented unary operator: ")
                       + opNames[calc.op] + ".");
            }

            nope:
            throw runtimeError;
        } catch (cRuntimeError &e) {
            RUNTIME_ERROR(string("Cannot apply unary operator ")
                                      + opNames[calc.op]
                                      + " to the expression <"
                                      + CODE_TO_STRING(calc.exp)
                                      + "> which is of type "
                                      + typeNames[getType(val.getVal())]
                                      + '.');
            throw;
        }
    }
    cValueOrRef operator()(const TNested &calc) const {
        return (*this)(calc.exp).followMember(calc.id);
    }

    private:
        void fold(TValue &leftVal, const TValue &rightVal, const TOperator op, bool typeMayChange) const {
            TType tl = getType(leftVal);
            TType tr = getType(rightVal);

            #define TYPECHECK_0(leftType,rightType)
            #define TYPECHECK_1(leftType,rightType) && leftType == tl && rightType == tr
            #define BINOP(check, leftType,rightType,OP,action)\
                if(OP == op TYPECHECK_##check(leftType,rightType)) {\
                    CTYPE(leftType)        & l = boost::get<CTYPE(leftType)> (leftVal);\
                    CTYPE(rightType) const & r = boost::get<CTYPE(rightType)>(rightVal);\
                    action;\
                    return;\
                }

            #define CONSTRUCT(value) leftVal = TValue(value)
            #define COMPARABLE(check, type)\
                BINOP(check, type, type, OP_LT,  CONSTRUCT(l <  r))\
                BINOP(check, type, type, OP_LEQ, CONSTRUCT(l <= r))\
                BINOP(check, type, type, OP_GT,  CONSTRUCT(l >  r))\
                BINOP(check, type, type, OP_GEQ, CONSTRUCT(l >= r))
            #define COMPARABLE2(check, type1, type2)\
                BINOP(check, type1, type2, OP_EQ,  CONSTRUCT(l == r))\
                BINOP(check, type1, type2, OP_NEQ, CONSTRUCT(l != r))\
                BINOP(check, type1, type2, OP_LT,  CONSTRUCT(l <  r))\
                BINOP(check, type1, type2, OP_LEQ, CONSTRUCT(l <= r))\
                BINOP(check, type1, type2, OP_GT,  CONSTRUCT(l >  r))\
                BINOP(check, type1, type2, OP_GEQ, CONSTRUCT(l >= r))\
                \
                BINOP(check, type2, type1, OP_EQ,  CONSTRUCT(l == r))\
                BINOP(check, type2, type1, OP_NEQ, CONSTRUCT(l != r))\
                BINOP(check, type2, type1, OP_LT,  CONSTRUCT(l <  r))\
                BINOP(check, type2, type1, OP_LEQ, CONSTRUCT(l <= r))\
                BINOP(check, type2, type1, OP_GT,  CONSTRUCT(l >  r))\
                BINOP(check, type2, type1, OP_GEQ, CONSTRUCT(l >= r))

            //comparison:
            if (op == OP_EQ && tl == tr) {
                leftVal = TValue((int)(leftVal == rightVal));
                return;
            }
            if (op == OP_NEQ && tl == tr) {
                leftVal = TValue((int)(leftVal != rightVal));
                return;
            }
            //basic arithmetic (integer and double)
            if (tl == TYPE_INT && tr == TYPE_INT) {
                BINOP(0, TYPE_INT,    TYPE_INT,    OP_ADD, l += r)
                BINOP(0, TYPE_INT,    TYPE_INT,    OP_MUL, l *= r)
                BINOP(0, TYPE_INT,    TYPE_INT,    OP_DIV, if (r) l /= r; else { RUNTIME_ERROR("Division by zero."); })
                BINOP(0, TYPE_INT,    TYPE_INT,    OP_SUB, l -= r)

                BINOP(0, TYPE_INT,    TYPE_INT,    OP_MOD, l %= r)

                COMPARABLE(0, TYPE_INT)
            } else if (tl == TYPE_DOUBLE && tr == TYPE_DOUBLE) {
                BINOP(0, TYPE_DOUBLE, TYPE_DOUBLE, OP_ADD, l += r)
                BINOP(0, TYPE_DOUBLE, TYPE_DOUBLE, OP_MUL, l *= r)
                BINOP(0, TYPE_DOUBLE, TYPE_DOUBLE, OP_DIV, l /= r)
                BINOP(0, TYPE_DOUBLE, TYPE_DOUBLE, OP_SUB, l -= r)

                COMPARABLE(0, TYPE_DOUBLE)
            } else if (tl == TYPE_DOUBLE && tr == TYPE_INT) {
                BINOP(0, TYPE_DOUBLE, TYPE_INT, OP_ADD, l += r)
                BINOP(0, TYPE_DOUBLE, TYPE_INT, OP_MUL, l *= r)
                BINOP(0, TYPE_DOUBLE, TYPE_INT, OP_DIV, l /= r)
                BINOP(0, TYPE_DOUBLE, TYPE_INT, OP_SUB, l -= r)
            } else if (tl == TYPE_INT && tr == TYPE_DOUBLE) {
                BINOP(0, TYPE_INT, TYPE_DOUBLE, OP_ADD, if (typeMayChange) { CONSTRUCT(l + r); } else { l += r; }) //careful: make an int out of a double
                BINOP(0, TYPE_INT, TYPE_DOUBLE, OP_MUL, if (typeMayChange) { CONSTRUCT(l * r); } else { l *= r; })
                BINOP(0, TYPE_INT, TYPE_DOUBLE, OP_DIV, if (typeMayChange) { CONSTRUCT(l / r); } else { l /= r; })
                BINOP(0, TYPE_INT, TYPE_DOUBLE, OP_SUB, if (typeMayChange) { CONSTRUCT(l - r); } else { l -= r; })
            } else if (tl == TYPE_STRING && tr == TYPE_STRING) {
                BINOP(0, TYPE_STRING, TYPE_STRING, OP_ADD, l += r)
                BINOP(0, TYPE_STRING, TYPE_STRING, OP_MATCH,
                      CONSTRUCT(boost::regex_search(l, boost::regex(r)))); //regex!

                COMPARABLE(0, TYPE_STRING)
            } else if (tl == TYPE_SELECTION && tr == TYPE_SELECTION) {
                BINOP(0, TYPE_SELECTION, TYPE_SELECTION, OP_ADD,   l.add(r))
                BINOP(0, TYPE_SELECTION, TYPE_SELECTION, OP_SUB,   l.subtract(r))
                BINOP(0, TYPE_SELECTION, TYPE_SELECTION, OP_MUL,   l.intersect(r))

                COMPARABLE(0, TYPE_SELECTION)
            } else if (tl == TYPE_SELECTION && tr == TYPE_OBJECT) {
                BINOP(0, TYPE_SELECTION, TYPE_OBJECT, OP_ADD,   l.insert(r))
                BINOP(0, TYPE_SELECTION, TYPE_OBJECT, OP_SUB,   l.remove(r))

                //element of
                BINOP(0, TYPE_SELECTION, TYPE_OBJECT, OP_GT, CONSTRUCT(l.contains(r)))
            } else if (tl == TYPE_OBJECT && tr == TYPE_SELECTION) {
                //element of
                BINOP(0, TYPE_OBJECT, TYPE_SELECTION, OP_LT,  CONSTRUCT(r.contains(l)))
                BINOP(0, TYPE_OBJECT, TYPE_SELECTION, OP_ADD, CONSTRUCT(cSelection(r).insert(l)))

            } else if (tl == TYPE_OBJECT && tr == TYPE_OBJECT) {
                //BINOP(0, TYPE_OBJECT, TYPE_OBJECT, OP_EQ,    CONSTRUCT(l ==  r))
                //BINOP(0, TYPE_OBJECT, TYPE_OBJECT, OP_MATCH, CONSTRUCT(*l == *r)) //unnessessary

                if (op == OP_ADD) {
                    cSelection sel;
                    BINOP(0, TYPE_OBJECT, TYPE_OBJECT, OP_ADD, CONSTRUCT(sel.insert(l).insert(r)))
                }
            } else if (tl == TYPE_ARRAY) {
                /* append */
                if (op == OP_FILTER) {
                    TVecVal &vec = boost::get<TVecVal>(leftVal);

                    if(typeMayChange) {
                        vec.fixedSize = false;
                    }

                    vec.push_back(rightVal);
                    return;
                }

                if (tr == TYPE_ARRAY && op == OP_OR) {
                    //concatenation:
                    if (typeMayChange) {
                        boost::get<TVecVal>(leftVal).fixedSize = false;
                    }

                    BINOP(0, TYPE_ARRAY, TYPE_ARRAY, OP_OR, l.concat(r))
                }

                if (tr == TYPE_ARRAY
                &&  (op == OP_ADD || op == OP_SUB)) {
                    /* perform elementwise folding: */
                    TVecVal& vl = boost::get<TVecVal>(leftVal);
                    const TVecVal& vr = boost::get<TVecVal>(rightVal);

                    if (vl.size() == vr.size()) {
                        for(uint i = 0; i < vl.size(); ++i) {
                            fold(vl[i], vr[i], op, typeMayChange);
                        }
                        return;
                    }
                }
            }

            /* matrix multiplication */
            if ((tl == TYPE_ARRAY ||  tr == TYPE_ARRAY) && (op == OP_MUL || op == OP_DIV)) {
                if (tl == tr && op == OP_MUL) {
                    //two arrays
                          TVecVal &vl = boost::get<TVecVal>(leftVal);
                    const TVecVal &vr = boost::get<TVecVal>(rightVal);

                    int dl = vl.dim();
                    int dr = vr.dim();

                    uint n = vl.size();
                    uint m = vr.size();

                    if(dl == 1 && dr == 1) {
                        //scalar product
                        if (!typeMayChange) {
                            RUNTIME_ERROR("Cannot perform x *= y for two one dimensional arrays x, y since the result is a scalar which is not assingable to the vector x.");
                        }

                        if (n != m) {
                            RUNTIME_ERROR("Cannot compute scalar product: size mismatch!");
                        }

                        if (n == 0) {
                            /* scalar product of two empty vectors
                               would normally be of undefined type. However, since i know
                               the default value of the two vectors i can fold them.
                            */
                            leftVal = vl.defaultValue.get();
                            fold(leftVal, vr.defaultValue.get(), OP_MUL, true);
                            return;
                        }

                        for(uint i = 0; i < n; ++i) {
                            fold(vl[i], vr[i], OP_MUL, true);
                        }
                        for(uint i = 1; i < n; ++i) {
                            fold(vl[0], vl[i], OP_ADD, true);
                        }

                        leftVal = TValue(vl[0]);
                        return;
                    } else if (dl == 1 && dr == 2) {
                        //may sound unsual, but is used by the matrix multiplication
                        //meaning: "v * M" = v^T * M
                        if (n != m) {
                            RUNTIME_ERROR("Cannot compute matrix product: size missmatch!");
                            return;
                        }

                        if (n == 0) {
                            /* empty vector times empty matrix...? */
                            return; //done
                        }

                        const TVecVal &vr0 = boost::get<TVecVal>(vr[0]);
                        if (!typeMayChange && vl.fixedSize && n != vr0.size()) {
                            RUNTIME_ERROR("Cannot compute v *= M for a one dimensional vector v of FIXED SIZE if M is not square.");
                            return;
                        }
                        if (vr0.size() == 0) {
                            /* vector times empty matrix */
                            vl.resize(0); //done
                            return;
                        }

                        /* copy of the left hand side because the vector
                         * might be a row of the matrix! */
                        TValue res = leftVal;
                        TVecVal& resVec = boost::get<TVecVal>(res);
                        for(uint i = 0; i < n; ++i){
                            fold(resVec[i], vr[i], op, true); //scalar times vector
                        }

                        for (uint i = 1; i < n; ++i) {
                            fold(resVec[0], resVec[i], OP_ADD, true); //add the vectors
                        }

                        if (typeMayChange) {
                            leftVal = resVec[0];
                        } else {
                            resetToDefault(leftVal);
                            assignValue(leftVal, resVec[0]);
                        }
                        return;
                    } else if (dl == 2 && dr == 1) {
                        if (!typeMayChange) {
                            RUNTIME_ERROR("Cannot compute M *= v for a vector v and a matrix M because the resulting vector is not assignable to a matrix.");
                        }

                        if (n == 0 || m == 0) {
                            RUNTIME_ERROR("Cannot compute matrix product: matrix is empty!");
                            return;
                        }

                        /* fold the lines of the matrix with the vector */
                        /* make a copy because the right side might be a row
                         * of the left side */
                        TValue rightValCopy = rightVal;
                        for(uint i = 0; i < n; ++i) {
                            fold(vl[i], rightValCopy, OP_MUL, true);
                        }
                        /* repair the default value: */
                        vl.defaultValue = interp->createDefaultValue(getType(vl[0]));
                        return;
                    } else if (dl == 2 && dr == 2) {
                        //two matrices, but they may be the same,
                        //therefore copy the right one.
                        TValue rightValCopy = rightVal;
                        for(uint i = 0; i < n; ++i) {
                            fold(vl[i], rightValCopy, OP_MUL, typeMayChange);
                        }
                        return;
                    }
                } else if (tl == TYPE_ARRAY) {
                    /* left array, right scalar */
                    /* => perform scalation on every element */
                    /* => OP_MUL and OP_DIV both allowed */

                    /* The right hand side might be a value in the left hand side.
                     * I need to copy */
                    TValue rightValCopy = rightVal;
                    TVecVal &v = boost::get<TVecVal>(leftVal);
                    for(uint i = 0; i < v.size(); ++i) {
                        fold(v[i], rightValCopy, op, typeMayChange);
                    }

                    //type may have changed   (<--? What does that do?)
                    if(typeMayChange && v.size()) {
                        v.defaultValue = interp->createDefaultValue(v[0]);
                    }
                    return;
                } else if (op == OP_MUL) {
                    /* right array, left scalar */
                    if (typeMayChange) {
                        const TVecVal &vr = boost::get<TVecVal>(rightVal);
                        leftVal = TVecVal(std::vector<TValue>(vr.size(), leftVal),
                                          interp->createDefaultValue(leftVal));
                        TVecVal &vl = boost::get<TVecVal>(leftVal);


                        for(uint i = 0; i < vl.size(); ++i) {
                            fold(vl[i], vr[i], op, typeMayChange);
                        }

                        //type may now be different:
                        if (vl.size()) {
                            vl.defaultValue = interp->createDefaultValue(vl[0]);
                        }
                        return;
                    }
                }
            }

            COMPARABLE2(1, TYPE_INT, TYPE_DOUBLE);

            //implicit to string conversion in addition:
            BINOP(1, TYPE_STRING, TYPE_INT,    OP_ADD, l += toString(r))
            BINOP(1, TYPE_INT, TYPE_STRING,    OP_ADD, CONSTRUCT(toString(l) + r))
            BINOP(1, TYPE_STRING, TYPE_DOUBLE, OP_ADD, l += toString(r))
            BINOP(1, TYPE_DOUBLE, TYPE_STRING, OP_ADD, CONSTRUCT(toString(l) + r))

            if (tl == TYPE_VOID || tr == TYPE_VOID) {
                RUNTIME_ERROR("Void return value cannot be used in an expression.");
            }


            RUNTIME_ERROR(string("Cannot apply operator '")
                   + opNames[op] + "' to arguments of type "
                   + typeNames[tl] + " and " + typeNames[tr] + ".");

            #undef COMPARABLE
            #undef COMPARABLE2
            #undef TYPECHECK_0
            #undef TYPECHECK_1
            #undef BINOP
            #undef CONSTRUCT
        }

    struct TChunkTester {
        const TExpressionVisitor &visitor;
        const TValueRef &var;
        const TExpression &exp;
        TChunkTester(const TExpressionVisitor &visitor,
                     const TValueRef &var,
                     const TExpression &exp)
                    :visitor(visitor), var(var), exp(exp) { }
        bool operator()(TObjPtr chunk) const {
            var.assign(TValue(chunk));
            return toBool(visitor(exp).getVal());
        }
    };
    void filterSelection(cSelection &sel, const TExpression &filter) const {
        interp->varStack.enterScope();
        interp->varStack.declareScoped("_", (TObjPtr)(0));

        TChunkTester tester(*this, interp->varStack.getRef("_"), filter);
        sel.filter(TPredicateFilter<TChunkTester>(tester));

        interp->varStack.exitScope();
    }
    public:

    cValueOrRef operator()(const TBinaryCalc &calc) const {
        using boost::get;

        cValueOrRef left = (*this)(calc.left);
        TOperator opEq = OP_NONE;

        switch (calc.op) {
            /*##################*/
            case OP_INDEX: {
                /* container can stay a ref, because evaluating the index
                   cannot compromise the validity of a reference */
                const TValue& index   = (*this)(calc.right).getVal();
                TType indexType       = getType(index);

                if(indexType != TYPE_INT) {
                    RUNTIME_ERROR(string("The Arrayindex in the expression << ")
                                  + CODE_TO_STRING(calc)
                                  + " >> is of type "
                                  + typeNames[indexType]
                                  + " (but should be an integer).");
                }
                return left.followIndex(get<int>(index));
            }
            /*##################*/
            case OP_ASSIGN: {
                if (!left.assignable()) {
                    RUNTIME_ERROR(string("The left hand side of the assigning expression << ")
                                  + CODE_TO_STRING(calc)
                                  + " >> is a constant expression and cannot be assigned a value.");
                }

                /* evaluating the right hand side cannot have a side effect
                 * on a reference on the left hand side. Just go ahead: */
                return left.assign((*this)(calc.right).getVal());
            }
            /*##################*/
            case OP_ADDEQ: opEq = OP_ADD; goto handleOpEq;
            case OP_SUBEQ: opEq = OP_SUB; goto handleOpEq;
            case OP_DIVEQ: opEq = OP_DIV; goto handleOpEq;
            case OP_MULEQ: opEq = OP_MUL; goto handleOpEq;
            case OP_FILTEREQ: {
                opEq = OP_FILTER;
                if (getType(left.getVal()) == TYPE_SELECTION) {
                    RUNTIME_ERROR("Filter operator deprecated.");

                    if(!left.assignable()) {
                        RUNTIME_ERROR(string("The left hand side of the assigning expression << ")
                                  + CODE_TO_STRING(calc)
                                  + " >> is a constant expression and cannot be assigned a value.");
                    }

                    //copy nessessary, because code in the filtering expression might modify
                    //the selection (though it should not do that)
                    cSelection cpy = boost::get<cSelection>(left.getVal());
                    filterSelection(cpy, calc.right);
                    return left.assign(cpy);
                }
                goto handleOpEq;
            }
            {
                handleOpEq:
                if (!left.assignable()) {
                    RUNTIME_ERROR(string("The left hand side of the assigning expression << ")
                                  + CODE_TO_STRING(calc)
                                  + " >> is a constant expression and cannot be assigned a value.");
                }

                if(left.isFragile()) {
                    cValueOrRef right = (*this)(calc.right);
                    TValue leftVal = left.getVal(); //thats a copy, may have changed!
                    fold(leftVal, right.getVal(), opEq, false);
                    left.assign(leftVal); //does the notification
                    return left;
                } else {
                    //no notification nessessary:
                    TValue &leftVal = left.getRef();
                    cValueOrRef right = (*this)(calc.right);
                    fold(leftVal, right.getVal(), opEq, false /* type may not change */); //changes directly left
                    return left;
                }
            }
            /*##################*/
            case OP_AND: {
                            bool b1 = toBool(left.getVal());
                            return TValue(b1 && toBool((*this)(calc.right).getVal()));
            }
            case OP_OR:  {
                            if (getType(left.getVal()) == TYPE_ARRAY) {
                                goto stdop;
                            }
                            bool b1 = toBool(left.getVal());
                            return TValue(b1 || toBool((*this)(calc.right).getVal()));
            }
            case OP_OREQ: {
                            opEq = OP_OR;
                            if (getType(left.getVal()) == TYPE_ARRAY || !left.assignable()) {
                                goto handleOpEq;
                            }
                            left.assign(toBool(left.getVal()) || toBool((*this)(calc.right).getVal()));
                            return left;
            }
            case OP_ANDEQ: {
                            if (!left.assignable()){ //use the error message there
                                goto handleOpEq;
                            }
                            left.assign(toBool(left.getVal()) && toBool((*this)(calc.right).getVal()));
                            return left;
            }
            /*##################*/
            case OP_FILTER: {
                if (getType(left.getVal()) == TYPE_SELECTION) {
                    RUNTIME_ERROR("Filter operator deprecated.");

                    cSelection cpy = boost::get<cSelection>(left.getVal());
                    filterSelection(cpy, calc.right);
                    return TValue(cpy);
                }
            }
            case OP_FOREACH:

            case OP_MUL:
            case OP_DIV:
            case OP_ADD:
            case OP_SUB:

            case OP_LEQ:
            case OP_GEQ:
            case OP_GT:
            case OP_LT:

            case OP_NEQ:
            case OP_MATCH:
            case OP_EQ: stdop:{
                /* evaluating the right hand side can change the already
                   calculated value if I do merely keep it as a ref. For example:

                   a = 1;
                   b = a + (a = 2)

                   b should be set to 1 + 2 = 3, but if I keep the left hand
                   side of the addition as a reference and do not copy the value
                   the old value of 1 will be lost when evaluating the right
                   hand side.

                   This has some performance implications, if a is a complex
                   data type, for example:

                   selection a;
                   selection b;
                   ...
                   c = a + (a = b);   //1
                   c = a + b;         //2

                   I will create a copy of a in both cases even though it
                   could be done faster in the second case.
                */
                TValue leftVal = left.getVal(); //thats a copy
                cValueOrRef right = (*this)(calc.right);
                fold(leftVal, right.getVal(), calc.op, true /*type may change*/);
                return leftVal;
            }
            default:
                /* my fault */
                FAULT(string("Unimplemented binary operator: ")
                       + opNames[calc.op] + ".");

        }
    }

    /*
    OP_MUL, OP_DIV,
    OP_ADD, OP_MINUS,
    OP_NEQ, OP_MATCH, OP_EQ, OP_LT, OP_LEQ, OP_GT, OP_GEQ,
    OP_FILTER, OP_FOREACH,
    }

      ,
                       boost::recursive_wrapper<TFuncCall>,
                       boost::recursive_wrapper<TUnaryCalc>,
                       boost::recursive_wrapper<TBinaryCalc>
                          > TExpression;*/
};

struct cInterpreter::TStatementVisitor : boost::static_visitor<> {
    cInterpreter::TExpressionVisitor expVisitor;
    cInterpreter::TToBoolVisitor toBoolVisitor;
    cInterpreter *interp;
    TStatementVisitor(cInterpreter *interp)
            : expVisitor(interp), interp(interp),
              brk(false), ret(false), cont(false) {}

    bool brk, ret, cont;

    void operator()(const TExternal &ext) {
        interp->cleanRetVal();
        interp->varStack.getRetVarRef().assign(ext(interp));
        ret = true;
    }
    void operator()(const TStatements &statements) {
        interp->varStack.enterScope();
        for (uint s = 0;
             s < statements.size() && !ret && !brk &&!cont;
             ++s) {
            (*this)(statements[s]);
        }
        interp->varStack.exitScope();
    }
    void operator()(const TDanglingBlock &block) {
        (*this)(block.statements);
    }
    void operator()(const TStatement &statement) {
        boost::apply_visitor(*this, statement);
    }
    void operator()(const TIfStatement &stmt) {
        if (toBoolVisitor(expVisitor(stmt.condition).getVal())) {
            (*this)(stmt.ifBlock);
        } else {
            boost::apply_visitor(*this,stmt.elseBlock);
        }
    }
    void operator()(const boost::blank &b) {}
    void operator()(const TVarDeclaration &decl) {
        TValue initialiserValue;
        if (decl.initialiser) {
            /* evalualte the initialiser value *before* declaring the variable.
             * var int j = 5;
               {
                   var int j = j + 1;
                   Info(j); //should be 6
               }
               Info(j); //should be 5
            */
            initialiserValue = expVisitor(*decl.initialiser).getVal();
        }

        interp->varStack.declareScoped(decl.identifier,
                                 interp->createDefaultValue(decl));
        if(decl.initialiser) {
            interp->varStack.getRef(decl.identifier).assign(initialiserValue);
        }
    }
    void operator()(const TReturnStatement &stmt) {
        /* subtle problem: Don't get the retVarRef before evaluating
         * the expression! If the return value has template type
         * it may be destroyed and rebuilt while evaluating the expression! */

        interp->cleanRetVal();
        TValueRef retVarRef = interp->varStack.getRetVarRef();
        if (stmt.exp) {
            if (getType(retVarRef.getValue()) == TYPE_VOID) {
                RUNTIME_ERROR("Return statement with expression in function returning void.");
            }

            retVarRef.assign(expVisitor(*stmt.exp).getVal());
        } else {
            if (getType(retVarRef.getValue()) != TYPE_VOID) {
                RUNTIME_ERROR("Return statement without expression in function returning non-void.");
            }
            retVarRef.assign(_void());
        }

        ret = true;
    }
    void operator()(const TBreakStatement &breakStatement) {
        brk = true;
    }
    void operator()(const TContinueStatement &continueStatement) {
        cont = true;
    }
    void operator()(const TForStatement &forStmt) {
        interp->varStack.enterScope();
        if(forStmt.initialiser) {
            (*this)(forStmt.initialiser.get());
        }
        while(!forStmt.checker ||
              toBoolVisitor(expVisitor(*forStmt.checker).getVal())) {
            (*this)(forStmt.block);

            if (ret || brk) break;
            cont = false;

            if (forStmt.stepper) {
                expVisitor(*forStmt.stepper);
            }
        }
        brk = false;
        interp->varStack.exitScope();
    }
    void operator()(const TForeachStatement &foreachStmt) {
        cValueOrRef set = expVisitor(foreachStmt.exp);

        TType t = getType(set.getVal());
        const string &name = foreachStmt.identifier;

        if (t == TYPE_ARRAY) {
            /* iterate over an array. In case the expression over which
             * to iterate evaluates to something assignable, the
             * variable that takes the array elements will be a reference */
            const TVecVal &vec = boost::get<TVecVal>(set.getVal());

            for(uint i = 0; !ret && !brk && i < vec.size(); ++i) {
                interp->varStack.enterScope();

                if (set.assignable()) {
                    cValueOrRef setCpy = set;
                    setCpy.followIndex(i);
                    interp->varStack.declareScopedRef(name, setCpy.getAsTValueRef());
                } else {
                    interp->varStack.declareScoped(name, vec[i]);
                }

                (*this)(foreachStmt.block);

                interp->varStack.exitScope();
                cont = false;
            }

            brk = false;
        } else if (t == TYPE_SELECTION) {
            cSelection sel = boost::get<cSelection>(set.getVal()); //copy!

            cSelection::iterator it = sel.begin();
            for(; !ret && !brk && it != sel.end(); ++it) {
                interp->varStack.enterScope();
                interp->varStack.declareScoped(name, *it);
                (*this)(foreachStmt.block);
                interp->varStack.exitScope();
                cont = false;
            }
            brk = false;
        } else {
            RUNTIME_ERROR(string("The foreach-loop can only iterate over arrays and selections,")
                          + " not over values of type " + typeNames[t] + ".");
        }
    }
    void operator()(const TWhileStatement &whileStmt) {
        while(!brk && !ret && toBoolVisitor(expVisitor(whileStmt.condition).getVal())) {
            (*this)(whileStmt.block);
            cont = false;
        }
        brk = false;
    }
    void operator()(const TExpression &exp) {
        expVisitor(exp);
    }
};

//***************************
//  Construction
//***************************

struct cInterpreter::TGlobalVisitor : boost::static_visitor<> {
    cInterpreter *interpreter;
    TGlobalVisitor(cInterpreter *interpreter):interpreter(interpreter) {};
    void operator()(TFuncDeclaration &decl) {
        interpreter->registerFunc(&decl);
    }
    void operator()(TVarDeclaration &decl) {
        interpreter->varStack.declareGlobal(
                        decl.identifier,
                        interpreter->createDefaultValue(decl));

        if (decl.initialiser) {
            try {
                interpreter->varStack.getRef(decl.identifier).assign(
                     TExpressionVisitor(interpreter)(decl.initialiser.get()).getVal());
            } catch (cRuntimeError &e) {
                LOG_ERROR("While default initialising the variable \"" + decl.identifier + "\".");
                throw;
            }
        }
    }
    void operator()(TStructDecl &decl) {
        cInterpreter::TStructTable::iterator it;
        if ((it = interpreter->structs.find(decl.structName)) != interpreter->structs.end()) {
            RUNTIME_ERROR(string("Redefined struct: ") + decl.structName);
        }
        interpreter->structs[decl.structName] = &decl;

        /* the structs I am referring to may not refer back to me. */
        std::stack<TStructDecl*> criticalStructs;
        criticalStructs.push(&decl);

        while(!criticalStructs.empty()) {
            TStructDecl *curr = criticalStructs.top();
            criticalStructs.pop();

            foreach(TVarDeclaration &var, curr->decls) {
                if ((var.type == TYPE_STRUCT)
                && ((it = interpreter->structs.find(var.structName)) != interpreter->structs.end())) {
                    if (it->second == &decl) {
                        RUNTIME_ERROR("The struct \"" + decl.structName + "\" uses itself (directly or indirectly). This is not allowed (would need 'infinite memory').");
                    }
                    criticalStructs.push(it->second);
                }
            }
        }
    }
};

cInterpreter::cInterpreter(TProgram &program) : program(program), world(0) {
    currentFunctionHasTemplateReturnValue = false;
    introduceDefines();
    TExternals::registerExternals(this);

    //this may already do some calculations:
    try {
        TGlobalVisitor gv(this);
        foreach(TGlobal &global, program) {
            boost::apply_visitor(gv, global);
        }
    } catch (cRuntimeError &e) {
        LOG_ERROR("While constructing global variables and registering function.");
        throw;
    }
}

void cInterpreter::introduceDefines() {
    define("endl", "\n");
    define("backslash", "\\");
    define("quotes", "\"");
}

#include<sstream>
void cInterpreter::loadWorld(const std::string &path) {
    destroyWorld();
    world = new zWorld(path);
}

void cInterpreter::destroyWorld() {
    delete world; world = 0;
}

//***************************
//  Default Values
//***************************

TValue cInterpreter::createDefaultValue(const TVarDeclaration &decl, bool ignoreRefFlag) {
    if(decl.isRef && !ignoreRefFlag) {
        RUNTIME_ERROR(string("References may only be declared as parameters of functions")
                      + " (partly because i want to spare me the effort of checking"
                      + " whether the reference goes out of scope before the"
                      + " referenced value goes out of scope).");
    }

    return createDefaultValue(decl.type, decl.structName, decl.dimensions);
}

TValue cInterpreter::createDefaultValue(const TFuncDeclaration &decl) {
    return createDefaultValue(decl.returnType, decl.structName, decl.dimensions);
}

TValue cInterpreter::createDefaultValue(TType type, const string &structName, const TArrayDimensions &dim) {
    TValue val;
    switch(type) {
        case TYPE_VOID:      val = _void();         break;
        case TYPE_INT :      val = int(0);          break;
        case TYPE_DOUBLE:    val = double(0);       break;
        case TYPE_STRING:    val = std::string(""); break;
        case TYPE_SELECTION: val = cSelection();    break;
        case TYPE_OBJECT:    val = TObjPtr(0);      break;
        case TYPE_FUNCTION:  val = (TFuncDeclaration*)(0); break;
        case TYPE_TEMPLATE:  {
                val = _template();
                if (dim.size() != 0) {
                    RUNTIME_ERROR(string("An array of templates would be")
                                + " a heterogenous collection of data"
                                + " and could lead for example to an array"
                                + " containing strings and ints simultaneously."
                                + " Since this would be shitty programming style"
                                + " it is not supported (and will not be supported)."
                                + " If you want an array of an unknown (but constant)"
                                + " type, simply assign it to a template"
                                + " (a template is already flexible enough"
                                + " to take the form of an array).");
                }
                return val;
            }
        case TYPE_STRUCT: {
                TStructTable::iterator declIt = structs.find (structName);
                if (declIt == structs.end()) {
                    RUNTIME_ERROR(string("Undefined struct or misspelled typename: ")
                                  + structName);
                }
                TStructDecl &decl = *declIt->second;

                val = TStructVal();
                TStructVal &s = boost::get<TStructVal>(val);
                s.structName = structName;
                TVarDeclarations::iterator it = decl.decls.begin();

                for(; it != decl.decls.end(); ++it) {
                    TValue & newVal = (s.members[it->identifier] = createDefaultValue(*it));
                    if (it->initialiser) {
                        assignValue(newVal, TExpressionVisitor(this)(it->initialiser.get()).getVal());
                    }
                }
                break;
            }
        case TYPE_ARRAY:
        case TYPE_MAX:
                FAULT(string("Trying to createDefaultValue for ") + typeNames[type]);

        /* use -Wswitch to warn of type not handled */
    }

    for(int i = dim.size() - 1; i >= 0; --i) {
        if(dim[i]) {
            TValue size = TExpressionVisitor(this)(*dim[i]).getVal();
            const int *d = boost::get<int>(&size);
            if (!d) {
                RUNTIME_ERROR("Array Dimension is not an integer.");
            } else if (*d <= 0) {
                RUNTIME_ERROR("Array Dimension must be positive or omitted.");
            }

            TVecVal tmpVec(val, uint(*d), true);//std::vector<TValue>(*d, val), val, true);
            val = TValue(tmpVec);
        } else {
            /* unspecified size */
            val = TVecVal(val);
        }
    }
    return val;
}

TValue cInterpreter::createDefaultValue(TValue val) {
    resetToDefault(val);
    return val;
}

//***************************
//  Defines:
//***************************
void cInterpreter::define(const std::string &str, const TValue &val)  {
    varStack.declareGlobal(str, val);
}

//externals register via this function
void cInterpreter::registerFunc(TFuncDeclaration *decl) {
    if (functions.find(decl->identifier) != functions.end()) {
        RUNTIME_ERROR("Redefined function: \"" + decl->identifier + "\".");
    }

    functions.insert(
        std::pair<std::string, TFuncDeclaration*>(
            decl->identifier,
            decl
        )
    );
}

const TFuncDeclaration& cInterpreter::findFunction(const std::string &name) const {
    TFuncTable::const_iterator it = this->functions.find(name);

    TFuncDeclaration* func;
    TMaybeRef ref;
    if (it != functions.end()) {
        func = it->second;
    } else if((ref = this->varStack.hasRef(name))) {
        TFuncDeclaration* const * f = boost::get<TFuncDeclaration*>(&(*ref).getValue());
        if (!f) {
            RUNTIME_ERROR("\"" + name + "\" is not callable!");
        } else if (*f == 0) {
            RUNTIME_ERROR("\"" + name + "\" cannot be called because it is not initialised (it is null)!");
        }
        func = *f;
    } else {
        RUNTIME_ERROR("Undefined function: \"" + name + "\".");
    }
    return *func;
}

cValueOrRef cInterpreter:: callFunction(const std::string &name, const TParamList &params) {
    TFuncTable::iterator it = this->functions.find(name);

    TFuncDeclaration* func;
    TMaybeRef var;
    if (it != functions.end()) {
        func = it->second;
    } else if((var = this->varStack.hasRef(name))) {
        if (getType((*var).getValue()) != TYPE_FUNCTION) {
            RUNTIME_ERROR("\"" + name + "\" is not callable!");
        }
        func = boost::get<TFuncDeclaration*>(var->getValue());

        if (!func) {
            RUNTIME_ERROR("\"" + name + "\" is null / uninitialised and cannot be called!");
        }
    } else {
        RUNTIME_ERROR("Undefined function: \"" + name + "\".");
    }

    return callFunction(func, params);
}

cValueOrRef cInterpreter::callFunction(TFuncDeclaration *func, const TParamList &params) {
    //*******************************
    // ENTER CONTEXT
    //*******************************

    cVarContext context;
    const string &funcName = func->identifier;

    /* Step1 : Evaluate the parameters that are provided (while still in caller scope)
               and put them into the context (default construction in callee scope) */

    if(params.size() > func->params.size()) {
        std::stringstream ss;
        ss << "Too may parameters provided to function \"" << funcName << "\""
           << "(takes " << func->params.size() << " but " << params.size()
           << " given).";
        RUNTIME_ERROR(ss.str());
    }

    TExpressionVisitor expV(this);
    try { for(uint i = 0; i < params.size(); ++i) {
        cValueOrRef p = expV(params[i]);
        const string &paramName = func->params[i].identifier;

        if(func->params[i].isRef && p.assignable()) {
            try {
                /* default initialise dummy (in callee scope)
                 * (not the reference, they are not default initialisable)
                 * and check, whether they have the same type */
                varStack.enterFunction(funcName, &context);
                TValue dummy = createDefaultValue(func->params[i], true /* ignore ref flag */);
                varStack.exitFunction();

                /* the reason for this being so complicated is that non-trivial
                 * things are allowed to happen in the array dimensions:
                 * void(ref arr1[], ref arr2[arr1.size])
                 * is allowed! It would be faster but even more complicated to
                 * not default construct arr2. */
                checkRightMatchesLeft(dummy, p.getVal());
            } catch (cRuntimeError &e) {
                LOG_ERROR("Reference paramter cannot be initialised.");
                throw;
            }
            context.declareRef(paramName, p.getAsTValueRef());
        } else {
            /* default initialise the variable (in calle scope)
             * and assign it */

            varStack.enterFunction(funcName, &context);
            context.declare(paramName, createDefaultValue(func->params[i], true));
            varStack.exitFunction();

            context.getRef(paramName).assign(p.getVal());
        }
    }} catch (cRuntimeError &e) {
        LOG_ERROR(string("While evaluating parameters for call to ") + funcName + ".");
        throw;
    }

    /* Step2 : Default initialise the variables that are not provided
               (while already in callee scope) */
    varStack.enterFunction(funcName, &context);

    bool oldTemplState = currentFunctionHasTemplateReturnValue;
    currentFunctionHasTemplateReturnValue = func->returnType == TYPE_TEMPLATE;

    uint p = params.size();
    try { for(; p < func->params.size(); ++p) {
        /* create the variable with default value: */
        if (!func->params[p].initialiser) {
            std::stringstream ss;
            ss << "The function \"" << funcName
               << "\" takes " << func->params.size()
               << " parameters but only the first " << p
               << " are provided or default initialised.";
            RUNTIME_ERROR(ss.str());
        }

        std::string & name = func->params[p].identifier;
        cValueOrRef defParam = expV(*func->params[p].initialiser);

        if (!func->params[p].isRef || !defParam.assignable()) {
            context.declare(name, createDefaultValue(func->params[p], true));
            context.getRef(name).assign(defParam.getVal());
        } else {
            /* default initialise dummy
             * (not the reference, they are not default initialisable)
             * and check, whether they have the same type */
            TValue dummy = createDefaultValue(func->params[p], true);
            checkRightMatchesLeft(dummy, defParam.getVal());
            context.declareRef(name, defParam.getAsTValueRef());
        }
    }} catch (cRuntimeError &e) {
        LOG_ERROR(string("While trying to default initialise unprovided parameters for call to " + funcName + "."));
        throw;
    }

    /* check for stack overflow */
    /* if anyone reads this: If your disgusted by this code and know
     * a better way to achieve my aim, please, please tell me! */
    int i;
    static long long stackBottom  = (long long)&i;
           long long currStackPos = (long long)&i;
    static int STACKSIZE    = 0x1000000; //as in the parameter for ld
    if (stackBottom - currStackPos > STACKSIZE - (STACKSIZE / 10)) {
        cLogger::getLogger()->fault("Imminent stack overflow averted. Check for infinite recursion!");
        throw runtimeError;
    }

    //create variable to hold the return value, if not already created.
    //I am not creating them earlier (when finding the function in the file)
    //because they may refer to structs that are not default constructible yet.
    if (!this->varStack.hasRetVar()) {
        this->varStack.declareRetVar(createDefaultValue(*func));
    }

    try {
        TStatementVisitor sv(this);
        boost::apply_visitor(sv, func->statements);

        if (sv.brk) {
            RUNTIME_ERROR("There is no loop here to break!");
        }
        if (sv.cont) {
            RUNTIME_ERROR("There is no loop here to continue!");
        }
        if(!sv.ret && func->returnType != TYPE_VOID) {
            RUNTIME_ERROR("Function \"" + func->identifier
                          + "\" did not return a value.");
        }
    } catch (cRuntimeError &e) {
        LOG_ERROR("While executing " + funcName + ".");
        throw;
    }

    //*******************************
    // EXIT CONTEXT
    //*******************************

    cValueOrRef res = this->varStack.getRetVarRef();
    this->varStack.exitFunction();
    currentFunctionHasTemplateReturnValue = oldTemplState;

    return res;
}

//***************************
// misc
//***************************

cInterpreter::~cInterpreter() {
    destroyWorld();
}
