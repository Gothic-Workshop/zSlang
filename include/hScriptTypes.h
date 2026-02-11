#ifndef HSCRIPTTYPES_H
#define HSCRIPTTYPES_H

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/optional.hpp>
#include <vector>
#include <cSelection.h>
#include <cLogger.h>
typedef unsigned int uint;

///////////////////////////////////////////////////////////////////////////
//  AST typedefs
///////////////////////////////////////////////////////////////////////////

//types
enum TType {
    TYPE_VOID,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_SELECTION,
    TYPE_OBJECT,
    TYPE_TEMPLATE,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_ARRAY,
    TYPE_MAX
};

struct _void {
    bool operator==(const _void &v) const { return true; }
};
struct _template {
    bool operator==(const _template &v) const {
        RUNTIME_ERROR("Trying to compare uninitialised templates!");
        return false;
    }
};

class zChunkSelectable;
typedef zChunkSelectable* TObjPtr;
class cSelection;
struct TVecVal;
struct TStructVal;
struct TFuncDeclaration;
typedef TFuncDeclaration* TFuncPtr;

inline TType SCRIPT_TYPE_OF(const _void       &a) { return TYPE_VOID;      }
inline TType SCRIPT_TYPE_OF(const int         &a) { return TYPE_INT;       }
inline TType SCRIPT_TYPE_OF(const double      &a) { return TYPE_DOUBLE;    }
inline TType SCRIPT_TYPE_OF(const std::string &a) { return TYPE_STRING;    }
inline TType SCRIPT_TYPE_OF(const cSelection  &s) { return TYPE_SELECTION; }
inline TType SCRIPT_TYPE_OF(const TObjPtr     &o) { return TYPE_OBJECT;    }
inline TType SCRIPT_TYPE_OF(const TStructVal  &s) { return TYPE_STRUCT;    }
inline TType SCRIPT_TYPE_OF(const TVecVal     &v) { return TYPE_ARRAY;     }
inline TType SCRIPT_TYPE_OF(const _template   &v) { return TYPE_TEMPLATE;  }
inline TType SCRIPT_TYPE_OF(const TFuncPtr    &p) { return TYPE_FUNCTION;  }

#define SCRIPT_TYPE(cppType) SCRIPT_TYPE_OF(cppType())

#define CTYPE(type) CTYPE_##type
#define CTYPE_TYPE_VOID _void
#define CTYPE_TYPE_INT int
#define CTYPE_TYPE_DOUBLE double
#define CTYPE_TYPE_STRING string
#define CTYPE_TYPE_SELECTION cSelection
#define CTYPE_TYPE_OBJECT TObjPtr
#define CTYPE_TYPE_STRUCT TStructType
#define CTYPE_TYPE_ARRAY TVecVal
#define CTYPE_TYPE_TEMPLATE _template
#define CTYPE_TYPE_FUNCTION TFuncPtr

enum TOperator {
    OP_NONE = 0,

    /* operators on the same line belong to the same group */
    OP_DOT,
    OP_INDEX,
    OP_NOT, OP_UNARYMINUS, OP_UNARYPLUS,
    OP_MUL, OP_DIV, OP_MOD,
    OP_ADD, OP_SUB,
    OP_NEQ, OP_MATCH, OP_EQ, OP_LT, OP_LEQ, OP_GT, OP_GEQ,
    OP_FILTER,
    OP_AND, OP_OR,
    OP_ASSIGN, OP_ADDEQ, OP_SUBEQ, OP_MULEQ, OP_DIVEQ, OP_FILTEREQ, OP_ANDEQ, OP_OREQ,

    OP_MAX
};
//% used for two different things
#define OP_FOREACH OP_MOD


enum TOpGroup {
    OPS_DOT   = 1,
    OPS_UNARY,
    OPS_MUL,
    OPS_ADD,
    OPS_TEST,
    OPS_SEL,
    OPS_LOGIC,
    OPS_ASSIGN,
    OPS_EVIL
};

extern const char *typeNames[TYPE_MAX];
extern const char *opNames[OP_MAX];

//a value:
struct TVecVal;
struct zObject;

typedef boost::variant<_void,
                       int,
                       double,
                       std::string,
                       TObjPtr,    //object
                       cSelection,           //cSelection
                       TVecVal,
                       TStructVal,
                       _template,
                       TFuncPtr> TValue;
bool operator!=(const TValue &v1, const TValue &v2);
std::string toString(const TValue &val);
void assignValue(TValue &dest, const TValue &source);
void resetToDefault(TValue &val);

struct TStructVal {
    typedef std::map<string, TValue> TStructMembers;

    string structName;
    TStructMembers members;

    bool operator==(const TStructVal &other) const;
};
BOOST_FUSION_ADAPT_STRUCT(
    TStructVal,
    (string, structName)
    (TStructVal::TStructMembers, members)
)

struct TVecVal {
    /* must be a vector of TValue* to assure that the position
     * of a value in the array is static, i.e:
     * TValue &ref = vec[i] must remain valid as long as the
     * vector exists and has at least i+1 elements.
     * If TVecVal was implemented as a vector<TValue>
     * a reference would become invalid when changing the size
     * of the vector (because the stl is free to relocate the
     * elements when the size of the vector changes). */
    private:
        /* dont let anyone fiddle with vec, this is bound
         * to cause memory leaks due to unfreed values */
        std::vector<TValue*> vec;
    public:
        inline       TValue & operator[](uint pos) {
            ASSERT(pos < size(), "TVecVal::operator[]: Index out of bounds!");
            return *vec[pos];
        }
        inline const TValue & operator[](uint pos) const {
            ASSERT(pos < size(), "TVecVal::operator[]: Index out of bounds!");
            return *vec[pos];
        }
        inline uint size() const {
            return vec.size();
        }
        ~TVecVal(); //important: delete all values!

        boost::recursive_wrapper<TValue> defaultValue; //used when resizing and for type checking when vec is empty
        bool fixedSize;

        TVecVal(const std::vector<TValue> &valVec, const TValue &defaultValue, bool fixedSize = true);
        TVecVal(const TValue &defaultValue, uint size = 0, bool fixedSize = false);
        TVecVal(const TVecVal &other);
        TVecVal& operator=(const TVecVal &other);

        /* operators implemented in TValue.cpp */
        bool operator!=(const TVecVal &other) const;
        bool operator==(const TVecVal &other) const;

        TVecVal& push_back(const TValue &val);
        TVecVal& concat(const TVecVal &other);

        void resetToDefault();
        void resize(uint newSize);
        void assign(const TVecVal &other);
        int dim() const; //dimensions
};

/* implemented in TValue.cpp  */
TType getType(const TValue &val);
void checkRightMatchesLeft(const TValue &val1, const TValue &val2); //throws

//Atoms
struct  TStringConst { std::string str; }; //extra struct to not confuse it with identifier
BOOST_FUSION_ADAPT_STRUCT(TStringConst, (std::string, str))
typedef boost::variant<int,double,TStringConst> TConstant;
typedef std::string TIdentifier;

//Expressions (Calculations)
struct TFuncCall;
struct TBinaryCalc;
struct TUnaryCalc;
struct TNested;
struct TExpressionList;

typedef boost::variant<TConstant,
                       TIdentifier,
                       boost::recursive_wrapper<TFuncCall>,
                       boost::recursive_wrapper<TUnaryCalc>,
                       boost::recursive_wrapper<TBinaryCalc>,
                       boost::recursive_wrapper<TNested>,
                       boost::recursive_wrapper<TExpressionList>
                          > TExpression;

struct TExpressionList {
    std::vector<TExpression> list;
};
BOOST_FUSION_ADAPT_STRUCT(
    TExpressionList,
    (std::vector<TExpression>,  list   )
)


struct TBinaryCalc {
    TExpression left;
    TOperator op;
    TExpression right;

    TBinaryCalc(TExpression left,
                TOperator op,
                TExpression right) : left(left), op(op), right(right) {};
};
BOOST_FUSION_ADAPT_STRUCT(
    TBinaryCalc,
    (TExpression,  left   )
    (TOperator,    op     )
    (TExpression,  right  )
)
struct TUnaryCalc {
    TOperator op;
    TExpression exp;
};
BOOST_FUSION_ADAPT_STRUCT(
    TUnaryCalc,
    (TOperator,    op     )
    (TExpression,  exp    )
)

struct TNested {
    TExpression exp;
    TIdentifier id;

    TNested(TExpression exp, TIdentifier id) : exp(exp), id(id) {};
};
BOOST_FUSION_ADAPT_STRUCT(
    TNested,
    (TExpression, exp     )
    (TIdentifier, id      )
)

//function call
typedef std::vector<TExpression> TParamList;

struct TFuncCall {
    TExpression funcExp;
    TParamList params;

    TFuncCall(TExpression e, TParamList p) : funcExp(e), params(p) {};
};
BOOST_FUSION_ADAPT_STRUCT(
    TFuncCall,
    (TExpression, funcExp  )
    (TParamList , params)
)

//declaration
typedef boost::optional<TExpression> TOptExp;
typedef std::vector<TOptExp> TArrayDimensions;
typedef TOptExp TInitialiser;
struct TVarDeclaration {
    bool isRef;
    TType type;
    string structName; //only if type is struct
    TIdentifier identifier;
    TArrayDimensions dimensions;
    TInitialiser initialiser;
};
BOOST_FUSION_ADAPT_STRUCT(
    TVarDeclaration,
    (bool,   isRef)
    (TType,  type  )
    (string, structName)
    (TIdentifier, identifier  )
    (TArrayDimensions, dimensions)
    (TInitialiser, initialiser)
)

//struct declaration
typedef std::vector<TVarDeclaration> TVarDeclarations;
struct TStructDecl {
    TIdentifier structName;
    TVarDeclarations decls;
};
BOOST_FUSION_ADAPT_STRUCT(
    TStructDecl,
    (TIdentifier, structName)
    (TVarDeclarations, decls)
)

//Statement
struct TReturnStatement {
    boost::optional<TExpression> exp;
};
BOOST_FUSION_ADAPT_STRUCT(
    TReturnStatement,
    (boost::optional<TExpression>,  exp  )
)

struct TIfStatement;
struct TForStatement;
struct TForeachStatement;
struct TWhileStatement;
struct TBreakStatement {};
struct TContinueStatement {};
struct TDanglingBlock;

typedef boost::variant<
            TVarDeclaration,
            TExpression,
            TReturnStatement,
            TBreakStatement,
            TContinueStatement,
            boost::recursive_wrapper<TIfStatement>,
            boost::recursive_wrapper<TForStatement>,
            boost::recursive_wrapper<TWhileStatement>,
            boost::recursive_wrapper<TDanglingBlock>,
            boost::recursive_wrapper<TForeachStatement>
        >
        TStatement;

typedef std::vector<TStatement> TStatements;

struct TDanglingBlock {
    TStatements statements;
};
BOOST_FUSION_ADAPT_STRUCT(
    TDanglingBlock,
    (TStatements, statements)
)

// if / else
typedef boost::variant<boost::blank, boost::recursive_wrapper<TIfStatement>, TStatements> TElseBlock;
struct TIfStatement {
    TExpression condition;
    TStatements ifBlock;
    TElseBlock elseBlock;
};

BOOST_FUSION_ADAPT_STRUCT(
    TIfStatement,
    (TExpression, condition  )
    (TStatements, ifBlock    )
    (TElseBlock,  elseBlock  )
)

//while loop
struct TWhileStatement {
    TExpression condition;
    TStatements block;
};
BOOST_FUSION_ADAPT_STRUCT(
    TWhileStatement,
    (TExpression, condition  )
    (TStatements, block      )
)

//for loop
typedef boost::optional<TVarDeclaration> TOptDecl;
struct TForStatement {
    TOptDecl initialiser;
    TOptExp  checker, stepper;
    TStatements block;
};
BOOST_FUSION_ADAPT_STRUCT(
    TForStatement,
    (TOptDecl, initialiser)
    (TOptExp, checker    )
    (TOptExp, stepper    )
    (TStatements   , block      )
)

struct TForeachStatement {
    TIdentifier identifier;
    TExpression exp;
    TStatements block;
};
BOOST_FUSION_ADAPT_STRUCT(
    TForeachStatement,
    (TIdentifier, identifier)
    (TExpression, exp)
    (TStatements, block)
)

//statement or external
class cInterpreter;
typedef TValue(*TExternal)(cInterpreter*);
typedef boost::variant<TStatements, TExternal> TCode;

//Func Decl
typedef std::vector<TVarDeclaration> TParamDeclarations;
struct TFuncDeclaration {
    TType              returnType;
    string             structName;
    TArrayDimensions   dimensions;
    TIdentifier        identifier;
    TParamDeclarations params;
    TCode              statements;
};
BOOST_FUSION_ADAPT_STRUCT(
    TFuncDeclaration,
    (TType,              returnType)
    (string,             structName)
    (TArrayDimensions, dimensions)
    (TIdentifier,        identifier)
    (TParamDeclarations, params)
    (TCode,        statements)
)

//Program
typedef boost::variant<TVarDeclaration,TFuncDeclaration, TStructDecl> TGlobal;
typedef std::vector<TGlobal> TProgram;

#endif //HSCRIPTTYPES_H
