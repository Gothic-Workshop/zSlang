#include "TExternals.h"
#include<cInterpreter.h>
#include<hScriptTypes.h>
#include<cLogger.h>
#include<zWorld.h>
#include<zChunkWP.h>
#include<zChunkVob.h>
#include<zWaynet.h>
#include<zVobtree.h>

#include<sstream>

#include <boost/foreach.hpp>
#define foreach         BOOST_FOREACH
#include <boost/variant/get.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

std::vector<TFuncDeclaration> TExternals::externals;

void TExternals::registerExternals(cInterpreter *interpreter) {
    foreach(TFuncDeclaration &decl, externals) {
        interpreter->registerFunc(&decl);
    }
}

//***************************
//  The Macros
//***************************

#define ADD_DIMENSION(z,n,data) decl.dimensions.push_back(false);
//Add a parameter to the TFuncDeclaration
#define ADD_PARAM_SIMPLE(r, data, elem) ADD_PARAM_SIMPLE2 elem
#define ADD_PARAM_SIMPLE2(parType, parName) ADD_PARAM_DIM(parType, parName, 0)
#define ADD_PARAM_DIM(parType, parName, dim)          \
        {                                             \
            TVarDeclaration varDecl;                  \
            varDecl.isRef = 0;                        \
            varDecl.type = SCRIPT_TYPE(parType);      \
            varDecl.identifier = #parName;       \
                                                      \
            for (int i = 0; i < dim; ++i) {           \
                varDecl.dimensions.push_back(false);  \
            }                                         \
                                                      \
            decl.params.push_back(varDecl);           \
        }
#define ADD_PARAM_ARRAY(r, data, elem) ADD_PARAM_DIM elem

//Get a parameter from the varStack:
#define GET_PARAM_SIMPLE(r, data, elem) GET_PARAM_SIMPLE2 elem
#define GET_PARAM_SIMPLE2(type, name)              \
        type name = boost::get<type>(interpreter->getVarValue(#name));
#define GET_PARAM_ARRAY(r, data, elem) GET_PARAM_ARRAY2 elem
#define GET_PARAM_ARRAY2(type, name, dim)   \
        BOOST_PP_IF(dim, \
            TVecVal name = boost::get<TVecVal>(interpreter->getVarValue(#name));,\
            GET_PARAM_SIMPLE2(type, name)\
        )

//General implementation of DEFINE_EXTERNAL (more special cases follow)
#define DEFINE_EXTERNAL_EX(extName, smplOrCmplx, retType, retDim, params, paramSeq, code)      \
struct External_##extName {                                                    \
    External_##extName() {                                                     \
        TFuncDeclaration decl;                                                 \
        decl.returnType = SCRIPT_TYPE(retType);                                \
        decl.identifier = #extName;                                       \
                                                                               \
        BOOST_PP_REPEAT(retDim, ADD_DIMENSION, _)                              \
        BOOST_PP_IIF(params,                                                   \
            BOOST_PP_SEQ_FOR_EACH(ADD_PARAM_##smplOrCmplx, ___, paramSeq),     \
            BOOST_PP_EMPTY()                                                   \
        )                                                                      \
                                                                               \
        decl.statements = &execute;                                            \
        TExternals::addExternal(decl);                                         \
    }                                                                          \
    static TValue execute(cInterpreter *interpreter) {                         \
        BOOST_PP_IIF(params,                                                   \
            BOOST_PP_SEQ_FOR_EACH(GET_PARAM_##smplOrCmplx, _, paramSeq),       \
            BOOST_PP_EMPTY()                                                   \
        )                                                                      \
                                                                               \
        code                                                                   \
    }                                                                          \
} external_##extName;

#define DEFINE_EXTERNAL(extName, retType, paramSeq, code)\
        DEFINE_EXTERNAL_EX(extName, SIMPLE, retType, 0, 1, paramSeq, code)

#define DEFINE_EXTERNAL_NOPARAMS(extName, retType, code)\
        DEFINE_EXTERNAL_EX(extName, SIMPLE, retType, 0, 0, _, code)

#define DEFINE_EXTERNAL_ARRAYS(extName, retType, retDim, paramSeq, code)\
        DEFINE_EXTERNAL_EX(extName, ARRAY, retType, retDim, 1, paramSeq, code)

#define GET_PARAM_TEMPLATE(r, data, name) GET_PARAM_TEMPLATE2(name)
#define GET_PARAM_TEMPLATE2(name)\
        TValue name = interpreter->getVarValue(#name);
#define ADD_PARAM_TEMPLATE(r, data, name) ADD_PARAM_DIM(_template, name, 0)
#define DEFINE_EXTERNAL_TEMPLATES(extName, retType, retDim, paramSeq, code)\
        DEFINE_EXTERNAL_EX(extName, TEMPLATE, retType, retDim, 1, paramSeq, code)

//***************************
// Error Printing
//***************************

DEFINE_EXTERNAL(Debug, _void,
    ((string, s)),
    DEBUGINFO(s);
    return _void();
)

DEFINE_EXTERNAL(Info, _void,
    ((string, s)),
    INFO(s);
    return _void();
)

DEFINE_EXTERNAL(Warn, _void,
    ((string, s)),
    WARN(s);
    return _void();
)

DEFINE_EXTERNAL(Error, _void,
    ((string, s)),
    LOG_ERROR(s);
    return _void();
)

//***************************
// Failing
//***************************

DEFINE_EXTERNAL(Fatal, _void,
    ((string, s)),
    RUNTIME_ERROR("Fatal Error was called: " + s);
    return _void();
)

//***************************
// Misc Util
//***************************

DEFINE_EXTERNAL_ARRAYS(CVT_StrToVec, int, 1,
    ((string, s, 0)),
    std::vector<TValue> vec(s.length(), 0);
    for(unsigned int i = 0; i < s.length(); ++i) {
        vec[i] = s[i];
    }
    return TVecVal(vec, TValue(int(0)), true);
)

DEFINE_EXTERNAL_ARRAYS(CVT_VecToStr, string, 0,
    ((int, vec, 1)),

    string str(vec.size(), ' ');

    for(unsigned int i = 0; i < vec.size(); ++i) {
        int chr = boost::get<int>(vec[i]);

        if (chr < 0 || chr > 255) {
            WARN("vecToStr: Character not in range [0,255].");
        }

        str[i] = chr;
    }
    return str;
)

/*
    Das geht Skriptseitig
DEFINE_EXTERNAL_ARRAYS(CVT_SelToArr, TObjPtr, 1,
    ((cSelection, sel, 0)),

    std::vector<TValue> vec;
    for (cSelection::iterator it = sel.begin(); it != sel.end(); ++it) {
        vec.push_back(*it);
    }
    return TVecVal(vec, TObjPtr(0), true);
)

DEFINE_EXTERNAL_ARRAYS(CVT_ArrToSel, cSelection, 0,
    ((TObjPtr, arr, 1)),
    cSelection res;
    for (unsigned int i = 0; i < arr.size(); ++i) {
        res.insert(boost::get<TObjPtr>(arr[i]));
    }
    return res;
) */

#include<sstream>
DEFINE_EXTERNAL_ARRAYS(CVT_RawToFloats, double, 1,
    ((string, raw, 0)),
    if (raw.length() % 8) {
          RUNTIME_ERROR("CVT_RawToFloats: The size of raw is not dividable by 8.");
    }
    uint numFloats = raw.length()/8;

    float fbuf[numFloats];
    char *buf = (char*)&fbuf;
    for(uint i = 0; i < numFloats * 4; i += 1) {
        int byte;
        sscanf(raw.c_str() + 2*i, "%2x", &byte);
        buf[i] = byte;
    }

    TVecVal res(double(0.0));
    for(uint i = 0; i < numFloats; ++i) {
        res.push_back(fbuf[i]);
    }

    return res;
)

DEFINE_EXTERNAL_ARRAYS(CVT_FloatsToRaw, string, 0,
    ((double, floats, 1)),
    float fbuf[floats.size()];
    unsigned char *cbuf = (unsigned char*)&fbuf;
    for(uint i = 0; i < floats.size(); ++i) {
        fbuf[i] = boost::get<double>(floats[i]);
    }

    char buf[8*floats.size()];
    for(uint i = 0; i < 4*floats.size(); i += 1) {
        sprintf(buf + 2*i, "%02x", cbuf[i]);
    }
    return string(buf);
)

DEFINE_EXTERNAL_TEMPLATES(TPL_TypeOf, string, 0,
    (value),
    return typeNames[getType(value)];
)

DEFINE_EXTERNAL_TEMPLATES(TPL_StructName, string, 0,
    (value),
    if (getType(value) != TYPE_STRUCT) {
        return "";
    }
    return boost::get<TStructVal>(value).structName;
)

DEFINE_EXTERNAL_TEMPLATES(TPL_BaseTypeOf, string, 0,
    (value),
    TValue *val = &value;
    TVecVal *vecVal;
    while ((vecVal = boost::get<TVecVal>(val))) {
        val = &vecVal->defaultValue.get();
    }
    return typeNames[getType(*val)];
)

DEFINE_EXTERNAL_TEMPLATES(TPL_DimOf, int, 0,
    (value),
    const TVecVal *vecVal = boost::get<TVecVal>(&value);
    if(vecVal) {
        return vecVal->dim();
    } else {
        return 0;
    }
)

/* Jetzt als Member

DEFINE_EXTERNAL_TEMPLATES(TPL_NullVal, _template, 0,
    (value),
    return interpreter->createDefaultValue(value);
)                   */

struct External_ARR_Resize {
    External_ARR_Resize() {
        TFuncDeclaration decl;
        decl.returnType = TYPE_TEMPLATE;
        decl.identifier = "ARR_Resize";
        decl.statements = &execute;

        TVarDeclaration arrParam, sizeParam;
        arrParam.identifier = "arr";
        arrParam.isRef = true;
        arrParam.type  = TYPE_TEMPLATE;

        sizeParam.identifier = "size";
        sizeParam.isRef = false;
        sizeParam.type = TYPE_INT;

        decl.params.push_back(arrParam);
        decl.params.push_back(sizeParam);
        TExternals::addExternal(decl);
    }
    static TValue execute(cInterpreter *interpreter) {
        cValueOrRef ref = interpreter->getVarRef("arr");
        int size = boost::get<int>(interpreter->getVarValue("size"));
        if (ref.isFragile()) {
            /* that better never happens for big arrays */
            TVecVal vec = boost::get<TVecVal>(ref.getVal());
            vec.resize(size);
            ref.assign(vec);
        } else {
            TVecVal &vec = boost::get<TVecVal>(ref.getRef());
            if (vec.fixedSize) {
                RUNTIME_ERROR(string("ARR_Resize called with vector of fixed size."));
            }
            vec.resize(size);
        }
        return _void();
    }
} external_ARR_Resize;

/* zu halbherzig
DEFINE_EXTERNAL(FUNC_getType, string,
    ((TFuncPtr, ptr)),
    if(!ptr) {
        RUNTIME_ERROR("FUNC_getType: The provided function is null.");
    }
    string res = typeNames[ptr->returnType];
    for(unsigned int d = 0; d < ptr->dimensions.size(); ++d) {
        res += "[]";
    }
    res += "(";
    for(unsigned int p = 0; p < ptr->params.size(); ++p) {
        res += typeNames[ptr->params[p].type];
        for (unsigned int d = 0; d < ptr->params[p].dimensions.size(); ++d) {
            res += "[]";
        }
        if (p < ptr->params.size() - 1) {
            res += ",";
        }
    }
    res += ")";
    return res;
) */

/* schon ein member
DEFINE_EXTERNAL(FUNC_ParamCount, int,
    ((TFuncPtr, ptr)),
    if (!ptr) {
        RUNTIME_ERROR("FUNC_getType: The provided function is null.");
    }
    return (int)ptr->params.size();
)                  */

/* zu halbherzig
DEFINE_EXTERNAL(FUNC_getParamType, string,
    ((TFuncPtr, ptr))
    ((int,      index)),
    if (!ptr) {
        RUNTIME_ERROR("FUNC_getType: The provided function is null.");
    }
    if (index < 0 || index >= (int)ptr->params.size()) {
        std::stringstream ss;
        ss << "FUNC_getType: Index out of range: " << index << ".";
        RUNTIME_ERROR(ss.str());
    }
    string res = typeNames[ptr->params[index].type];
    for(unsigned int d = 0; d < ptr->params[index].dimensions.size(); ++d) {
        res += "[]";
    }
    return res;
)*/

//***************************
//  World loading
//***************************

DEFINE_EXTERNAL(WLD_Load, _void,
    ((string, path)),
    INFO("Loading World from \"" + path + "\".");
    INDENTLOG;
    interpreter->loadWorld(path);
    return _void();
)

DEFINE_EXTERNAL(WLD_Merge, cSelection,
    ((string, path)),
    INFO("Merging World \"" + path + "\".");
    INDENTLOG;
    if(zWorld* world = interpreter->getWorld()) {
        cSelection oldObj = cSelection::all(world);
        world->merge(path);
        return cSelection::all(world).subtract(oldObj);
    } else {
        WARN("No world loaded, cannot merge Worlds. Loading world instead.");
        interpreter->loadWorld(path);
        return cSelection::all(interpreter->getWorld());
    }
)

DEFINE_EXTERNAL(WLD_Save, _void,
    ((string, path)),
    INFO("Saving World to \"" + path + "\".");
    if(zWorld *world = interpreter->getWorld()) {
        world->save(path);
    } else {
        RUNTIME_ERROR("Cannot save the world because non is loaded!");
    }
    return _void();
)

DEFINE_EXTERNAL(WLD_SaveSelection, _void,
    ((string, path))
    ((cSelection, sel)),
    INFO("Saving Selection to \"" + path + "\".");
    if(zWorld *world = interpreter->getWorld()) {
        world->save(path, &sel);
    } else {
        RUNTIME_ERROR("Cannot save the world because non is loaded!");
    }
    return _void();
)

DEFINE_EXTERNAL_NOPARAMS(WLD_Destroy, _void,
    INFO("Destroying world.");
    interpreter->destroyWorld();
    return _void();
)

//***************************
// Vob handling
//***************************

void EXT_checkValidObject(TObjPtr obj) {
    if (!obj) {
        RUNTIME_ERROR("Trying to access an object, but the object pointer is null.");
    }

    if(!zChunkSelectable::isLiving(obj)) {
        RUNTIME_ERROR("Trying to access non existing object. Fix this bug: The zSlang Interpreter has partially undefined behaviour when accessing deleted objects!");
    }
}

DEFINE_EXTERNAL(WLD_DeleteObject, _void,
    ((TObjPtr, obj)),
    //test whether this points to a valid object

    EXT_checkValidObject(obj);

    zChunkWP* wp;
    if ((wp = dynamic_cast<zChunkWP*>(obj))) {
        ASSERT(interpreter->getWorld(), "Valid WP but no World?");
        ASSERT(interpreter->getWorld()->getWaynet(), "Valid WP but no Waynet?");
        interpreter->getWorld()->getWaynet()->removeWP(wp);
    } else {
        zChunkVob *vob = dynamic_cast<zChunkVob*>(obj);
        ASSERT(vob, "Valid object that is neither WP nor vob?!");
        ASSERT(vob->getParent(), "Vob without parent?!");
        vob->getParent()->removeChild(vob);
    }
    return _void();
)

DEFINE_EXTERNAL(WLD_MoveToParent, _void,
    ((TObjPtr, obj))
    ((TObjPtr, newParent)),

    EXT_checkValidObject(obj);
    zChunkWP* wp = dynamic_cast<zChunkWP*>(obj);

    if (wp) {
        if (!newParent) {
            return _void(); //ok.
        }
        RUNTIME_ERROR("WLD_MoveToParent: Only vobs can be moved to a new parent. The given Vob however is a WP.");
    }

    zChunkVob *vob = dynamic_cast<zChunkVob*>(obj);

    ASSERT(vob, "Object that is neither wp nor vob?!");

    if (newParent) {
        EXT_checkValidObject(newParent);
        zChunkVob *parentVob = dynamic_cast<zChunkVob*>(newParent);

        if (!parentVob) {
            RUNTIME_ERROR("WLD_MoveToParent: The given parent is neither a valid vob nor null.");
        }

        vob->moveTo(parentVob);
    } else {
        ASSERT(interpreter->getWorld() && interpreter->getWorld()->getVobtree(), "Valid vob but no vobtree?!");
        vob->moveTo(interpreter->getWorld()->getVobtree());
    }
    return _void();
)

//***************************
// Waypoints
//***************************

DEFINE_EXTERNAL(WLD_ConnectWPs, _void,
    ((TObjPtr, o1))
    ((TObjPtr, o2)),
    EXT_checkValidObject(o1);
    EXT_checkValidObject(o2);
    zChunkWP * wp1 = dynamic_cast<zChunkWP*>(o1);
    zChunkWP * wp2 = dynamic_cast<zChunkWP*>(o2);

    if(!wp1 || !wp2) {
        RUNTIME_ERROR("WLD_ConnectWPs: At least one of the given objects is null or not a waypoint.");
    }

    wp1->connect(wp2);
    return _void();
)

DEFINE_EXTERNAL(WLD_DisconnectWPs, _void,
    ((TObjPtr, o1))
    ((TObjPtr, o2)),
    EXT_checkValidObject(o1);
    EXT_checkValidObject(o2);
    zChunkWP * wp1 = dynamic_cast<zChunkWP*>(o1);
    zChunkWP * wp2 = dynamic_cast<zChunkWP*>(o2);

    if(!wp1 || !wp2) {
        RUNTIME_ERROR("WLD_DisconnectWPs: At least one of the given objects is null or not a waypoint.");
    }

    wp1->disconnect(wp2);
    return _void();
)

DEFINE_EXTERNAL(WLD_GetConnectedWPs, cSelection,
    ((TObjPtr, o)),
    EXT_checkValidObject(o);
    zChunkWP * wp = dynamic_cast<zChunkWP*>(o);

    if (!wp) {
        RUNTIME_ERROR("WLD_GetConnectedWPs: The object is null or not a waypoint.");
    }

    return TValue(wp->getConnected());
)

DEFINE_EXTERNAL(WLD_IsWP, int,
    ((TObjPtr, o)),
    EXT_checkValidObject(o);
    return (int)(dynamic_cast<zChunkWP*>(o) != 0);
)

DEFINE_EXTERNAL(WLD_IsVob, int,
    ((TObjPtr, o)),
    EXT_checkValidObject(o);
    return (int)(dynamic_cast<zChunkVob*>(o) != 0);
)

//***************************
// Basic Vob selection
//***************************

DEFINE_EXTERNAL_NOPARAMS(WLD_GetAll, cSelection,
    return cSelection::all(interpreter->getWorld());
)

DEFINE_EXTERNAL_NOPARAMS(WLD_GetVobs, cSelection,
    return cSelection::vobs(interpreter->getWorld());
)

DEFINE_EXTERNAL_NOPARAMS(WLD_GetWPs, cSelection,
    return cSelection::wps(interpreter->getWorld());
)

DEFINE_EXTERNAL(WLD_GetByName, cSelection,
    ((string, name)),
    return cSelection::byProperty(interpreter->getWorld(),
                                  "vobName", name)
           .add(cSelection::byProperty(interpreter->getWorld(), "wpName", name));
)

DEFINE_EXTERNAL(WLD_GetVobsByName, cSelection,
    ((string, name)),
    return cSelection::byProperty(interpreter->getWorld(),
                                  "vobName", name);
)

DEFINE_EXTERNAL(WLD_GetWPsByName, cSelection,
    ((string, name)),
    return cSelection::byProperty(interpreter->getWorld(),
                                  "wpName", name);
)

DEFINE_EXTERNAL(WLD_GetVobsByVisual, cSelection,
    ((string, name)),
    return cSelection::byProperty(interpreter->getWorld(),
                                  "visual", name);
)

//***************************
// Arithmetik
//***************************

#include<math.h>

#define ADAPT_MATH_FUN(fun)       \
    DEFINE_EXTERNAL(fun, double,  \
        ((double, d)),            \
        return fun(d);            \
    )

//misc
ADAPT_MATH_FUN(sqrt)
ADAPT_MATH_FUN(exp)
ADAPT_MATH_FUN(log)

DEFINE_EXTERNAL(pow, double,
    ((double, base))
    ((double, exp)),
    return pow(base,exp);
)

//trigonometry
ADAPT_MATH_FUN(sin)
ADAPT_MATH_FUN(cos)
ADAPT_MATH_FUN(tan)
ADAPT_MATH_FUN(sinh)
ADAPT_MATH_FUN(cosh)
ADAPT_MATH_FUN(tanh)
ADAPT_MATH_FUN(asin)
ADAPT_MATH_FUN(acos)
ADAPT_MATH_FUN(atan)
ADAPT_MATH_FUN(abs)
ADAPT_MATH_FUN(fabs)

DEFINE_EXTERNAL(atan2, double,
    ((double, y))
    ((double, x)),
    return atan2(y,x);
)

//random
#include <cstdlib>
DEFINE_EXTERNAL_NOPARAMS(rand, int,
    return rand();
)

//time
#include<ctime>
DEFINE_EXTERNAL_NOPARAMS(timeMS, int,
    long long res = clock();
    res *= 1000;
    res /= CLOCKS_PER_SEC;
    return (int)res;
)

#undef ADAPT_MATH_FUN


#undef ADD_DIMENSION
#undef ADD_PARAM_SIMPLE
#undef ADD_PARAM_SIMPLE2
#undef ADD_PARAM_DIM
#undef ADD_PARAM_ARRAY
#undef GET_PARAM_SIMPLE
#undef GET_PARAM_SIMPLE2
#undef GET_PARAM_ARRAY
#undef GET_PARAM_ARRAY2
#undef DEFINE_EXTERNAL_EX
#undef DEFINE_EXTERNAL
#undef DEFINE_EXTERNAL_ARRAYS
#undef GET_PARAM_TEMPLATE
#undef GET_PARAM_TEMPLATE2
#undef ADD_PARAM_TEMPLATE
