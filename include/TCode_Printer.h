
///////////////////////////////////////////////////////////////////////////////
//  Printing for Debug purposes:
///////////////////////////////////////////////////////////////////////////////

#include<hScriptTypes.h>

#include <sstream>
#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH
#include <boost/variant/apply_visitor.hpp>

#include<cLogger.h>

//forward declaring this and implementing it elsewhere would suck.
struct TCode_Printer : boost::static_visitor<TCode_Printer&> {
    private:
        bool printFunctions;
        std::stringstream ss;

    public:
    TCode_Printer(bool printFunctions = false) : printFunctions(printFunctions) {};
    std::string getString() { return ss.str(); }

    TCode_Printer& operator()(const TStringConst & str) {
        ss << /*"string:\""*/ "\"" << str.str << "\"";
        return *this;
    }
    TCode_Printer& operator()(const double &dbl) {
        ss << /*"double:" << */ dbl;
        return *this;
    }
    TCode_Printer& operator()(const int &i) {
        ss << /* "int:" << */ i;
        return *this;
    }
    TCode_Printer& operator()(const TConstant &cst) {
        return boost::apply_visitor(*this, cst);
    }
    TCode_Printer& operator()(const TIdentifier &id) {
        ss << /*"symbol:" << */id;
        return *this;
    }
    #define SEPERATED(type,list,seperator) \
        std::vector<type>::const_iterator begin, end;\
        begin = list.begin(); \
        end   = list.end(); \
        if (begin != end) {\
            (*this)(*begin++);\
            while(begin != end) {\
                ss << seperator;\
                (*this)(*begin++);\
            }\
        }
    TCode_Printer& operator()(const TExpressionList &list) {
        ss << '{';
        SEPERATED(TExpression, list.list, ", ");
        ss << '}';
        return *this;
    }
    TCode_Printer& operator()(const TFuncCall &call) {
        (*this)(call.funcExp);
        ss << '(';
        SEPERATED(TExpression, call.params, ", ");
        ss << ')';
        return *this;
    }
    TCode_Printer& operator()(const TExpression &exp) {
        return boost::apply_visitor(*this, exp);
    }
    TCode_Printer& operator()(const TBinaryCalc &calc) {
        if (calc.op == OP_INDEX) {
            (*this)(calc.left);
            ss << "[";
            (*this)(calc.right);
            ss << "]";
        } else {
            ss << '(';
            (*this)(calc.left);
            ss << " " << opNames[calc.op] << " ";
            (*this)(calc.right);
            ss << ')';
        }
        return *this;
    }
    TCode_Printer& operator()(const TUnaryCalc &calc) {
        ss << opNames[calc.op];
        return (*this)(calc.exp);
    }
    TCode_Printer& operator()(const TNested &nested) {
        ss << '(';
        (*this)(nested.exp);
        ss << '.';
        (*this)(nested.id);
        ss << ')';
        return *this;
    }
    TCode_Printer& operator()(const TVarDeclaration &decl) {
        ss << (decl.isRef ? "ref " : "var ") << (decl.type == TYPE_STRUCT ? decl.structName : typeNames[decl.type]) << " " << decl.identifier;
        (*this)(decl.dimensions);
        if (decl.initialiser) {
            ss << " = "; (*this)(*decl.initialiser);
        }
        return *this;
    }
    TCode_Printer& operator()(const TStructDecl &decl) {
        ss << "struct " << decl.structName << " {\n";
        SEPERATED(TVarDeclaration, decl.decls, ";\n")
        ss << (decl.decls.size() > 0 ? ";\n" : "") << "};";
        return *this;
    }
    TCode_Printer& operator()(const TArrayDimensions &dims) {
        for(unsigned int i = 0; i < dims.size(); ++i) {
            ss << '[';
            if (dims[i]) {
                (*this)(*dims[i]);
            }
            ss << ']';
        }
        return *this;
    }
    TCode_Printer& operator()(const TReturnStatement &stmt) {
        ss << "return ";
        return (*this)(stmt.exp);
    }
    TCode_Printer& operator()(const TStatements &stms) {
        ss << "{" << std::endl;
        foreach(const TStatement &stm, stms) {
            boost::apply_visitor(*this, stm);
            ss << ";" << std::endl;
        }
        ss << "}";
        return *this;
    }
    TCode_Printer& operator()(const TBreakStatement &stmt) {
        ss << "break";
        return *this;
    }
    TCode_Printer& operator()(const TContinueStatement &stmt) {
        ss << "continue";
        return *this;
    }
    TCode_Printer& operator()(const TDanglingBlock &block) {
        return(*this)(block.statements);
    }
    TCode_Printer& operator()(const TForStatement &forStmt) {
        ss << "for(";
        (*this)(forStmt.initialiser); ss << "; ";
        (*this)(forStmt.checker)    ; ss << "; ";
        (*this)(forStmt.stepper)    ; ss << ") ";
        return (*this)(forStmt.block);
    }
    TCode_Printer& operator()(const TOptExp &optExp) {
        if (optExp) {
            (*this)(*optExp);
        }
        return *this;
    }
    TCode_Printer& operator()(const TOptDecl &optDecl) {
        if (optDecl) {
            (*this)(*optDecl);
        }
        return *this;
    }
    TCode_Printer& operator()(const TWhileStatement &whileStmt) {
        ss << "while ";
        return (*this)(whileStmt.condition)(whileStmt.block);
    }
    TCode_Printer& operator()(const TForeachStatement &foreachStmt) {
        ss << "foreach ";
        (*this)(foreachStmt.identifier);
        ss << " in ";
        return (*this)(foreachStmt.exp)(foreachStmt.block);
    }
    TCode_Printer& operator()(const TIfStatement &ifStmt) {
        ss << "if ";
        (*this)(ifStmt.condition)(ifStmt.ifBlock);

        if(!boost::get<boost::blank>(&ifStmt.elseBlock)) {
            ss << " else ";
        }

        return boost::apply_visitor(*this, ifStmt.elseBlock);
    }
    TCode_Printer& operator()(const boost::blank &b) {
        return *this;
    }
    TCode_Printer& operator()(const TFuncDeclaration &decl) {
        ss << typeNames[decl.returnType];
        (*this)(decl.dimensions);
        ss << ' ' << decl.identifier << '(';
        SEPERATED(TVarDeclaration, decl.params, ", ");
        ss << ')';

        if (printFunctions) {
            boost::apply_visitor(*this, decl.statements);
        }
        ss << ";";
        return *this;
    }
    TCode_Printer& operator()(const TExternal &external) {
        FAULT("External in user code?!");
        return *this;
    }
    TCode_Printer& operator()(const TGlobal &glob) {
        return boost::apply_visitor(*this, glob);
    }
    TCode_Printer& operator()(const TProgram &prog) {
        SEPERATED(TGlobal, prog, "\n");
        return *this;
    }
    #undef SEPERATED
    #define CODE_TO_STRING(x) TCode_Printer()(x).getString()
};
