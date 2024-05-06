#ifndef _PARSERDEFS_H_
#define _PARSERDEFS_H_

#include "base/zdefs.h"
#include "base/headers.h"
using std::shared_ptr;
#undef TEXT
#undef VOID

#include "base/util.h"
using namespace util;

//FUNCFLAG values, for `Function` and `ASTFuncDecl` objects.
#define FUNCFLAG_INLINE             0x00000001 //Can be heavily optimized
#define FUNCFLAG_INVALID            0x00000002 //Needs to throw an error, later in compile
#define FUNCFLAG_STATIC             0x00000004 //Is static (not per-object in classes)
#define FUNCFLAG_CONSTRUCTOR        0x00000008 //Is a user class constructor
#define FUNCFLAG_DESTRUCTOR         0x00000010 //Is a user class destructor
#define FUNCFLAG_CLASSFUNC          0x00000020 //Belongs to a user class
#define FUNCFLAG_VARARGS            0x00000040 //Uses variadic arguments
#define FUNCFLAG_DEPRECATED         0x00000080 //Gives a deprecated warning/error on use
#define FUNCFLAG_NOCAST             0x00000100 //Affects function's overloading
#define FUNCFLAG_INTARRAY           0x00000200 //Function represents a special internal array
#define FUNCFLAG_NIL                0x00000400 //Function does 'nothing' (optimizable)
#define FUNCFLAG_EXITS              0x00000800 //Function exits the current script (or game)
#define FUNCFLAG_NEVER_RETURN       0x00001000 //Function never returns
#define FUNCFLAG_READ_ONLY          0x00002000 //Function is read-only
#define FUNCFLAG_INTERNAL           0x00004000 //Function is internal
#define FUNCFLAG_CONSTEXPR          0x00008000 //Function is constexpr

#define IFUNCFLAG_SKIPPOINTER	      0x01
#define IFUNCFLAG_REASSIGNPTR	      0x02

//Option values. NOTE: Result of "lookupOption" must be '/10000.0' -V
#define OPT_OFF                     0
#define OPT_ON                      1
#define OPT_ERROR                   2
#define OPT_WARN                    3

#define OPTTYPE_QR                  0
#define OPTTYPE_CONFIG              1
#define OPTTYPE_CONFIG_FLOAT        2

#define MAX_INCLUDE_PATH_CHARS      65535

#define REGISTRATION_REC_LIMIT		50

// Base flags, describing the relation
#define CMP_GT    0x01
#define CMP_LT    0x02
#define CMP_EQ    0x04
// Extra behavior flags
#define CMP_SETI  0x08 // In 'SETCMP', multiplies the output by 10000.
#define CMP_BOOL  0x10 // Does a "boolean comparison" (ex. 2 CMP_EQ 3 is true, since !2 == !3)

#define CMP_FLAGS 0x07
#define CMP_NE    (CMP_GT|CMP_LT)
#define CMP_GE    (CMP_GT|CMP_EQ)
#define CMP_LE    (CMP_LT|CMP_EQ)

#define INVERT_CMP(cmp) ((cmp&(~CMP_FLAGS))|((~cmp)&CMP_FLAGS))
inline string CMP_STR(uint cmpval)
{
	string pref;
	if(cmpval & CMP_BOOL)
		pref += "B";
	if(cmpval & CMP_SETI)
		pref += "I";
	switch(cmpval&CMP_FLAGS)
	{
		default:
			return pref+"??";
		case 0:
			return pref+"Never";
		case CMP_GT:
			return pref+">";
		case CMP_GE:
			return pref+">=";
		case CMP_LT:
			return pref+"<";
		case CMP_LE:
			return pref+"<=";
		case CMP_EQ:
			return pref+"==";
		case CMP_NE:
			return pref+"!=";
		case CMP_FLAGS:
			return pref+"Always";
	}
}

void zconsole_db(const char *format,...);
void zconsole_warn(const char *format,...);
void zconsole_error(const char *format,...);
void zconsole_info(const char *format,...);
void zconsole_idle(dword seconds = 2);

void zconsole_db(std::string const& str);
void zconsole_warn(std::string const& str);
void zconsole_error(std::string const& str);
void zconsole_info(std::string const& str);

bool zparser_errored_out();
void zparser_error_out();

#endif

