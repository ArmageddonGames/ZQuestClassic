#ifndef _ZASM_TABLE_H
#define _ZASM_TABLE_H

#include "base/headers.h"

// The version of the ZASM engine a script was compiled for
// NOT the same as V_FFSCRIPT, which is the version of the packfile format
// where the scripts are serialized
#define ZASM_VERSION        3

const char* ScriptTypeToString(ScriptType type);

#define ZMETA_AUTOGEN		0x01
#define ZMETA_DISASSEMBLED	0x02
#define ZMETA_IMPORTED		0x04

#define SCRIPT_FORMAT_DEFAULT		0
#define SCRIPT_FORMAT_INVALID		1
#define SCRIPT_FORMAT_DISASSEMBLED	2
#define SCRIPT_FORMAT_ZASM			3

#define METADATA_V			6
#define V_COMPILER_FIRST	BUILDTM_YEAR
#define V_COMPILER_SECOND	BUILDTM_MONTH
#define V_COMPILER_THIRD	BUILDTM_DAY
#define V_COMPILER_FOURTH	BUILDTM_HOUR
#define ZMETA_NULL_TYPE		1
struct zasm_meta
{
	word zasm_v;
	word meta_v;
	word ffscript_v;
	dword global_count;
	ScriptType script_type;
	std::string run_idens[8];
	byte run_types[8];
	byte flags;
	word compiler_v1, compiler_v2, compiler_v3, compiler_v4;
	std::string script_name;
	std::string author;
	std::string attributes[10];
	std::string attribytes[8];
	std::string attrishorts[8];
	std::string usrflags[16];
	std::string attributes_help[10];
	std::string attribytes_help[8];
	std::string attrishorts_help[8];
	std::string usrflags_help[16];
	std::string initd[8];
	std::string initd_help[8];
	int8_t initd_type[8];
	
	void setFlag(byte flag)
	{
		switch(flag)
		{
			case ZMETA_DISASSEMBLED:
				flags &= ~ZMETA_IMPORTED;
				flags |= ZMETA_DISASSEMBLED;
				break;
			case ZMETA_IMPORTED:
				flags &= ~ZMETA_DISASSEMBLED;
				flags |= ZMETA_IMPORTED;
				break;
			default:
				flags |= flag;
		}
	}
	bool valid() const
	{
		return zasm_v >= 2 && meta_v >= 1 && ffscript_v >= 16;
	}
	void zero()
	{
		zasm_v = 0;
		meta_v = 0;
		ffscript_v = 0;
		global_count = 1024;
		script_type = ScriptType::None;
		flags = 0;
		compiler_v1 = 0;
		compiler_v2 = 0;
		compiler_v3 = 0;
		compiler_v4 = 0;
		for(int32_t q = 0; q < 16; ++q)
		{
			usrflags[q].clear();
			usrflags_help[q].clear();
			if(q > 9) continue;
			attributes[q].clear();
			attributes_help[q].clear();
			if(q > 7) continue;
			initd[q].clear();
			initd_help[q].clear();
			initd_type[q] = -1;
			run_idens[q].clear();
			run_types[q] = ZMETA_NULL_TYPE;
			attribytes[q].clear();
			attribytes_help[q].clear();
			attrishorts[q].clear();
			attrishorts_help[q].clear();
		}
		script_name.clear();
		author.clear();
	}
	void autogen(bool clears = true)
	{
		if(clears) zero();
		zasm_v = ZASM_VERSION;
		meta_v = METADATA_V;
		ffscript_v = V_FFSCRIPT;
		flags = ZMETA_AUTOGEN;
		compiler_v1 = V_COMPILER_FIRST;
		compiler_v2 = V_COMPILER_SECOND;
		compiler_v3 = V_COMPILER_THIRD;
		compiler_v4 = V_COMPILER_FOURTH;
	}
	zasm_meta()
	{
		zero();
	}
	~zasm_meta()
	{
		
	}
	zasm_meta& operator=(zasm_meta const& other)
	{
		zasm_v = other.zasm_v;
		meta_v = other.meta_v;
		ffscript_v = other.ffscript_v;
		script_type = other.script_type;
		for(auto q = 0; q < 16; ++q)
		{
			usrflags[q] = other.usrflags[q];
			usrflags_help[q] = other.usrflags_help[q];
			if(q > 9) continue;
			attributes[q] = other.attributes[q];
			attributes_help[q] = other.attributes_help[q];
			if(q > 7) continue;
			initd[q] = other.initd[q];
			initd_help[q] = other.initd_help[q];
			initd_type[q] = other.initd_type[q];
			run_idens[q] = other.run_idens[q];
			run_types[q] = other.run_types[q];
			attribytes[q] = other.attribytes[q];
			attribytes_help[q] = other.attribytes_help[q];
			attrishorts[q] = other.attrishorts[q];
			attrishorts_help[q] = other.attrishorts_help[q];
			if(q > 3) continue;
		}
		flags = other.flags;
		compiler_v1 = other.compiler_v1;
		compiler_v2 = other.compiler_v2;
		compiler_v3 = other.compiler_v3;
		compiler_v4 = other.compiler_v4;
		script_name = other.script_name;
		author = other.author;
		return *this;
	}
	bool operator==(zasm_meta const& other) const
	{
		if(zasm_v != other.zasm_v) return false;
		if(meta_v != other.meta_v) return false;
		if(ffscript_v != other.ffscript_v) return false;
		if(script_type != other.script_type) return false;
		if(flags != other.flags) return false;
		if(compiler_v1 != other.compiler_v1) return false;
		if(compiler_v2 != other.compiler_v2) return false;
		if(compiler_v3 != other.compiler_v3) return false;
		if(compiler_v4 != other.compiler_v4) return false;
		for(auto q = 0; q < 16; ++q)
		{
			if(usrflags[q].compare(other.usrflags[q]))
				return false;
			if(usrflags_help[q].compare(other.usrflags_help[q]))
				return false;
			if(q > 9) continue;
			if(attributes[q].compare(other.attributes[q]))
				return false;
			if(attributes_help[q].compare(other.attributes_help[q]))
				return false;
			if(q > 7) continue;
			if(initd[q].compare(other.initd[q]))
				return false;
			if(initd_help[q].compare(other.initd_help[q]))
				return false;
			if(initd_type[q] != other.initd_type[q])
				return false;
			if(run_idens[q].compare(other.run_idens[q]))
				return false;
			if(run_types[q] != other.run_types[q])
				return false;
			if(attribytes[q].compare(other.attribytes[q]))
				return false;
			if(attribytes_help[q].compare(other.attribytes_help[q]))
				return false;
			if(attrishorts[q].compare(other.attrishorts[q]))
				return false;
			if(attrishorts_help[q].compare(other.attrishorts_help[q]))
				return false;
		}
		if(script_name.compare(other.script_name))
			return false;
		if(author.compare(other.author))
			return false;
		return true;
	}
	bool operator!=(zasm_meta const& other) const
	{
		return !(*this == other);
	}
	
	bool parse_meta(const char *buffer);
	std::string get_meta() const;
};
ScriptType get_script_type(std::string const& name);
std::string get_script_name(ScriptType type);
#define D(n)               ((0x0000)+(n))
#define A(n)               ((0x0008)+(n))
#define GD(n)              ((0x02F5)+(n))

#define NUM_ZS_ARGS 3
struct ffscript
{
	word command;
	int32_t arg1, arg2, arg3;
	std::vector<int32_t> *vecptr;
	std::string *strptr;
	ffscript()
	{
		command = 0xFFFF;
		arg1 = 0;
		arg2 = 0;
		arg3 = 0;
		vecptr = nullptr;
		strptr = nullptr;
	}
	ffscript(word command, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0): command(command), arg1(arg1), arg2(arg2), arg3(arg3)
	{
		vecptr = nullptr;
		strptr = nullptr;
	}
	~ffscript()
	{
		if(vecptr)
		{
			delete vecptr;
			vecptr = nullptr;
		}
		if(strptr)
		{
			delete strptr;
			strptr = nullptr;
		}
	}
	void give(ffscript& other)
	{
		other.command = command;
		other.arg1 = arg1;
		other.arg2 = arg2;
		other.arg3 = arg3;
		other.vecptr = vecptr;
		other.strptr = strptr;
		vecptr = nullptr;
		strptr = nullptr;
		clear();
	}
	void clear()
	{
		command = 0xFFFF;
		arg1 = 0;
		arg2 = 0;
		arg3 = 0;
		if(vecptr)
		{
			delete vecptr;
			vecptr = nullptr;
		}
		if(strptr)
		{
			delete strptr;
			strptr = nullptr;
		}
	}
	void copy(ffscript& other)
	{
		other.clear();
		other.command = command;
		other.arg1 = arg1;
		other.arg2 = arg2;
		other.arg3 = arg3;
		if(vecptr)
		{
			other.vecptr = new std::vector<int32_t>();
			for(int32_t val : *vecptr)
				other.vecptr->push_back(val);
		}
		if(strptr)
		{
			other.strptr = new std::string();
			for(char c : *strptr)
				other.strptr->push_back(c);
		}
	}
	
	bool operator==(ffscript const& other) const
	{
		//Compare primitive members
		if(command != other.command) return false;
		if(arg1 != other.arg1) return false;
		if(arg2 != other.arg2) return false;
		if(arg3 != other.arg3) return false;
		//Check for pointer existence differences
		if((vecptr==nullptr)!=(other.vecptr==nullptr)) return false;
		if((strptr==nullptr)!=(other.strptr==nullptr)) return false;
		//If both have a pointer, compare pointer size/contents
		if(vecptr)
		{
			if(vecptr->size() != other.vecptr->size())
				return false;
			if((*vecptr) != (*other.vecptr))
				return false;
		}
		if(strptr)
		{
			if(strptr->size() != other.strptr->size())
				return false;
			if(strptr->compare(*other.strptr))
				return false;
		}
		return true;
	}
	
	optional<dword> max_gvar() const;
	
	void set_arg(int ind, int val)
	{
		switch(ind)
		{
			case 0:
				arg1 = val;
				break;
			case 1:
				arg2 = val;
				break;
			case 2:
				arg3 = val;
				break;
		}
	}
	int get_arg(int ind)
	{
		switch(ind)
		{
			case 0:
				return arg1;
			case 1:
				return arg2;
			case 2:
				return arg3;
		}
		return 0;
	}
};

struct script_id {
	auto operator<=>(const script_id&) const = default;

	ScriptType type;
	int index;
};

struct script_data
{
	ffscript* zasm;
	zasm_meta meta;
	script_id id;
	size_t size;
	bool optimized;
	
	void null_script(size_t newSize = 1)
	{
		if (newSize < 1)
			newSize = 1;
		if(zasm)
			delete[] zasm;
		zasm = new ffscript[newSize];
		zasm[0].clear();
		meta.zero();
		size = newSize;
		optimized = false;
	}
	
	bool valid() const
	{
		return (zasm && zasm[0].command != 0xFFFF);
	}
	
	void disable()
	{
		if(zasm)
		{
			zasm[0].clear();
			size = 1;
			optimized = false;
		}
	}
	
	void recalc_size()
	{
		if(zasm)
		{
			for(uint32_t q = 0;;++q)
			{
				if(zasm[q].command == 0xFFFF)
				{
					size = q+1;
					return;
				}
			}
		}

		size = 0;
	}
	
	void set(script_data const& other)
	{
		if(zasm)
			delete[] zasm;
		if(other.size)
		{
			zasm = new ffscript[other.size];
			for(size_t q = 0; q < other.size; ++q)
			{
				other.zasm[q].copy(zasm[q]);
			}
			size = other.size;
			optimized = other.optimized;
		}
		else
		{
			zasm = NULL;
			null_script();
		}
		meta = other.meta;
	}

	script_data(ScriptType type, int index) : zasm(NULL)
	{
		id = {type, index};
		null_script();
	}
	
	script_data(script_data const& other) : zasm(NULL)
	{
		set(other);
	}
	
	~script_data()
	{
		if(zasm)
			delete[] zasm;
	}
	
	void transfer(script_data& other)
	{
		other.meta = meta;
		if(other.zasm)
			delete[] other.zasm;
		other.zasm = zasm;
		other.size = size;
		other.optimized = optimized;
		zasm = NULL;
		null_script();
	}
	
	script_data& operator=(script_data const& other)
	{
		set(other);
		return *this;
	}
	
	bool equal_zasm(script_data const& other) const
	{
		if(size != other.size) return false;
		if(valid() != other.valid()) return false;
		for(auto q = 0; q < size; ++q)
		{
			if(zasm[q] != other.zasm[q]) return false;
		}
		return true;
	}
	
	bool operator==(script_data const& other) const
	{
		if(meta != other.meta) return false;
		return equal_zasm(other);
	}
};

enum
{
    ARGTY_UNUSED_REG,
    ARGTY_READ_REG,
    ARGTY_WRITE_REG,
    ARGTY_READWRITE_REG,
    ARGTY_LITERAL,
    ARGTY_COMPARE_OP,
    ARGTY_READ_GVAR,
    ARGTY_WRITE_GVAR,
};
#define ARGFL_COMPARE_USED 0x01
#define ARGFL_COMPARE_SET  0x02
#define ARGFL_UNIMPL       0x04
struct script_command
{
	string name;
	byte args;
	byte arg_type[3]; //ARGTY_
	byte arr_type; //0x1 = string, 0x2 = array
	byte flags; //ARGFL_

	bool is_register(int arg) const
	{
		return arg_type[arg] == ARGTY_READ_REG || arg_type[arg] == ARGTY_WRITE_REG || arg_type[arg] == ARGTY_READWRITE_REG;
	}

	bool writes_to_register(int arg) const
	{
		return arg_type[arg] == ARGTY_WRITE_REG || arg_type[arg] == ARGTY_READWRITE_REG;
	}
};

struct script_variable
{
    string name;
    int32_t id;
};

// Defines for script flags
#define TRUEFLAG          0x0001
#define MOREFLAG          0x0002
#define TRIGGERED         0x0004

enum ASM_DEFINE
{
	SETV,                 //0x0000
	SETR,                 //0x0001
	ADDR,                 //0x0002
	ADDV,                 //0x0003
	SUBR,                 //0x0004
	SUBV,                 //0x0005
	MULTR,                //0x0006
	MULTV,                //0x0007
	DIVR,                 //0x0008
	DIVV,                 //0x0009
	WAITFRAME,            //0x000A
	GOTO,                 //0x000B
	CHECKTRIG,            //0x000C //NOT IMPLEMENTED
	WARP,                 //0x000D
	COMPARER,             //0x000E
	COMPAREV,             //0x000F
	GOTOTRUE,             //0x0010
	GOTOFALSE,            //0x0011
	GOTOLESS,             //0x0012
	GOTOMORE,             //0x0013
	LOAD1,                //0x0014
	LOAD2,                //0x0015
	SETA1,                //0x0016
	SETA2,                //0x0017
	QUIT,                 //0x0018
	SINR,                 //0x0019
	SINV,                 //0x001A
	COSR,                 //0x001B
	COSV,                 //0x001C
	TANR,                 //0x001D
	TANV,                 //0x001E
	MODR,                 //0x001F
	MODV,                 //0x0020
	ABS,                 //0x0021
	MINR,                 //0x0022
	MINV,                 //0x0023
	MAXR,                 //0x0024
	MAXV,                 //0x0025
	RNDR,                 //0x0026
	RNDV,                 //0x0027
	FACTORIAL,            //0x0028
	POWERR,               //0x0029
	POWERV,               //0x002A
	IPOWERR,              //0x002B
	IPOWERV,              //0x002C
	ANDR,                 //0x002D
	ANDV,                 //0x002E
	ORR,                  //0x002F
	ORV,                  //0x0030
	XORR,                 //0x0031
	XORV,                 //0x0032
	NANDR,                //0x0033
	NANDV,                //0x0034
	NORR,                 //0x0035
	NORV,                 //0x0036
	XNORR,                //0x0037
	XNORV,                //0x0038
	NOT,                  //0x0039
	LSHIFTR,              //0x003A
	LSHIFTV,              //0x003B
	RSHIFTR,              //0x003C
	RSHIFTV,              //0x003D
	TRACER,               //0x003E
	TRACEV,               //0x003F
	TRACE3,               //0x0040
	LOOP,                 //0x0041
	PUSHR,                //0x0042
	PUSHV,                //0x0043
	POP,                  //0x0044
	ENQUEUER,             //0x0045 //NOT IMPLEMENTED
	ENQUEUEV,             //0x0046 //NOT IMPLEMENTED
	DEQUEUE,              //0x0047 //NOT IMPLEMENTED
	PLAYSOUNDR,           //0x0048
	PLAYSOUNDV,           //0x0049
	LOADLWEAPONR,          //0x004A
	LOADLWEAPONV,          //0x004B
	LOADITEMR,             //0x004C
	LOADITEMV,             //0x004D
	LOADNPCR,              //0x004E
	LOADNPCV,              //0x004F
	CREATELWEAPONR,        //0x0050
	CREATELWEAPONV,        //0x0051
	CREATEITEMR,           //0x0052
	CREATEITEMV,           //0x0053
	CREATENPCR,            //0x0054
	CREATENPCV,            //0x0055
	LOADI,                 //0x0056
	STOREI,                //0x0057
	GOTOR,                 //0x0058
	SQROOTV,               //0x0059
	SQROOTR,               //0x005A
	CREATEEWEAPONR,        //0x005B
	CREATEEWEAPONV,        //0x005C
	PITWARP,               //0x005D
	WARPR,                 //0x005E
	PITWARPR,              //0x005F
	CLEARSPRITESR,         //0x0060
	CLEARSPRITESV,         //0x0061
	RECTR,                 //0x0062
	CIRCLER,               //0x0063
	ARCR,                  //0x0064
	ELLIPSER,              //0x0065
	LINER,                 //0x0066
	PUTPIXELR,             //0x0067
	DRAWTILER,             //0x0068
	DRAWCOMBOR,            //0x0069
	ELLIPSE2,              //0x006A
	SPLINER,               //0x006B
	FLOODFILL,             //0x006C
	COMPOUNDR,             //0x006D
	COMPOUNDV,             //0x006E
	MSGSTRR,               //0x006F
	MSGSTRV,               //0x0070
	ISVALIDITEM,           //0x0071
	ISVALIDNPC,            //0x0072
	PLAYMIDIR,             //0x0073
	PLAYMIDIV,             //0x0074
	COPYTILEVV,            //0x0075
	COPYTILEVR,            //0x0076
	COPYTILERV,            //0x0077
	COPYTILERR,            //0x0078
	SWAPTILEVV,            //0x0079
	SWAPTILEVR,            //0x007A
	SWAPTILERV,            //0x007B
	SWAPTILERR,            //0x007C
	CLEARTILEV,            //0x007D
	CLEARTILER,            //0x007E
	OVERLAYTILEVV,         //0x007F
	OVERLAYTILEVR,         //0x0080
	OVERLAYTILERV,         //0x0081
	OVERLAYTILERR,         //0x0082
	FLIPROTTILEVV,         //0x0083
	FLIPROTTILEVR,         //0x0084
	FLIPROTTILERV,         //0x0085
	FLIPROTTILERR,         //0x0086
	GETTILEPIXEL,         //0x0087
	NPCISFLICKERFRAME,         //0x0088
	SETTILEPIXEL,         //0x0089
	HEROISFLICKERFRAME,         //0x008A
	SHIFTTILEVV,           //0x008B
	SHIFTTILEVR,           //0x008C
	SHIFTTILERV,           //0x008D
	SHIFTTILERR,           //0x008E
	ISVALIDLWPN,           //0x008F
	ISVALIDEWPN,           //0x0090
	LOADEWEAPONR,          //0x0091
	LOADEWEAPONV,          //0x0092
	ALLOCATEMEMR,          //0x0093
	ALLOCATEMEMV,          //0x0094
	ALLOCATEGMEMV,         //0x0095
	DEALLOCATEMEMR,		 //0x0096
	DEALLOCATEMEMV,		 //0x0097 //Pointless, can't deallocate a value
	WAITDRAW,				 //0x0098
	ARCTANR,				 //0x0099
	LWPNUSESPRITER,		 //0x09A
	LWPNUSESPRITEV,		 //0x09B
	EWPNUSESPRITER,		 //0x09C
	EWPNUSESPRITEV,		 //0x09D
	LOADITEMDATAR,		 //0x09E
	LOADITEMDATAV,		 //0x09F
	BITNOT,				 //0x00A0
	LOG10,				 //0x00A1
	LOGE,                  //0x00A2
	ISSOLID,               //0x00A3
	LAYERSCREEN,           //0x00A4
	LAYERMAP,              //0x00A5
	TRACE2R,		    //0x00A6
	TRACE2V,		    //0x00A7
	TRACE4,			    //0x00A8
	TRACE5,			    //0x00A9
	SECRETS,		    //0x00AA
	DRAWCHARR,		    //0x00AB
	GETSCREENFLAGS,	    //0x00AC
	QUADR,              //0X00AD
	TRIANGLER,          //0X00AE
	ARCSINR,              //0x00AF
	ARCSINV,              //0x00B0
	ARCCOSR,              //0x00B1
	ARCCOSV,              //0x00B2
	GAMEEND,              //0x00B3
	DRAWINTR,             //0x00B4
	SETTRUE,              //0x00B5
	SETFALSE,             //0x00B6
	SETMORE,              //0x00B7
	SETLESS,              //0x00B8
	FASTTILER,            //0x00B9
	FASTCOMBOR,           //0x00BA
	DRAWSTRINGR,          //0x00BB
	SETSIDEWARP,			//0x00BC
	SAVE,				    //0x00BD
	TRACE6,				//0x00BE
	WHATNO0x00BF, //PTROFF,//0x00BF
	QUAD3DR,              //0x00C0
	TRIANGLE3DR,          //0x00C1
	SETCOLORB,        	//0x00C2 for 2.6
	SETDEPTHB,        	//0x00C3 '
	GETCOLORB,        	//0x00C4 '
	GETDEPTHB,        	//0x00C5 '
	COMBOTILE,			//0x00C6
	SETTILEWARP,			//0x00C7
	GETSCREENEFLAGS,		//0x00C8
	GETSAVENAME,          //0x00C9
	ARRAYSIZE,            //0x00CA
	ITEMNAME,             //0x00CB
	SETSAVENAME,          //0x00CC
	NPCNAME,         		//0x00CD
	GETMESSAGE,			//0x00CE
	GETDMAPNAME,			//0x00CF
	GETDMAPTITLE,			//0x00D0
	GETDMAPINTRO,			//0x00D1
	ALLOCATEGMEMR,        //0x00D2
	BITMAPR,         		//0x00D3
	SETRENDERTARGET,      //0x00D4
	PLAYENHMUSIC,         //0x00D5
	GETMUSICFILE,         //0x00D6
	GETMUSICTRACK,        //0x00D7
	SETDMAPENHMUSIC,      //0x00D8
	DRAWLAYERR,           //0x00D9
	DRAWSCREENR,          //0x00DA
	BREAKSHIELD,          //0x00DB
	SAVESCREEN,           //0x00DC
	SAVEQUITSCREEN,       //0x00DD
	SELECTAWPNR,          //0x00DE
	SELECTAWPNV,          //0x00DF
	SELECTBWPNR,          //0x00E0
	SELECTBWPNV,          //0x00E1
	GETSIDEWARPDMAP,      //0x00E2
	GETSIDEWARPSCR,       //0x00E3
	GETSIDEWARPTYPE,      //0x00E4
	GETTILEWARPDMAP,      //0x00E5
	GETTILEWARPSCR,       //0x00E6
	GETTILEWARPTYPE,      //0x00E7
	GETFFCSCRIPT,         //0x00E8
	BITMAPEXR,	//0x00E9
	__RESERVED_FOR_QUAD2R, //0x00EA
	WAVYIN, //0x00EB
	WAVYOUT, //0x00EC
	ZAPIN, //0x00ED
	ZAPOUT, //0x00EF
	OPENWIPE, //0x00F0
	FREE0x00F1, //0x00F1 was SETLINKTILE
	FREE0x00F2, //0x00F2 was SETLINKEXTEND
	FREE0x00F3, //0x00F3 was GETLINKEXTEND
	SETMESSAGE,			//0x00F4
	SETDMAPNAME,			//0x00F5
	SETDMAPTITLE,			//0x00F5
	SETDMAPINTRO,			//0x00F7
	GREYSCALEON,			//0x00F8
	GREYSCALEOFF,			//0x00F9
	ENDSOUNDR,           //0x00FA
	ENDSOUNDV,           //0x00FB
	PAUSESOUNDR, 	//0x00FC
	PAUSESOUNDV,	//0x00FD
	RESUMESOUNDR,	//0x00FE
	RESUMESOUNDV,	//0x00FF
	PAUSEMUSIC,		//0x0100
	RESUMEMUSIC,	//0x0101
	LWPNARRPTR,	//0x0102
	EWPNARRPTR,	//0x0103
	ITEMARRPTR,	//0x0104
	IDATAARRPTR,	//0x0105
	FFCARRPTR,	//0x0106
	BOOLARRPTR,	//0x0107
	NPCARRPTR,	//0x0108
	LWPNARRPTR2,	//0x0109
	EWPNARRPTR2,	//0x0110
	ITEMARRPTR2,	//0x0111
	IDATAARRPTR2,	//0x0112
	FFCARRPTR2,	//0x0113
	BOOLARRPTR2,	//0x0114
	NPCARRPTR2,	//0x0115
	ARRAYSIZEB,            //0x0116
	ARRAYSIZEF,            //0x0117 
	ARRAYSIZEN,            //0x0118
	ARRAYSIZEL,            //0x0119
	ARRAYSIZEE,            //0x011A
	ARRAYSIZEI,            //0x011B
	ARRAYSIZEID,            //0x011C
	POLYGONR,		//0x011D
	__RESERVED_FOR_POLYGON3DR,		//0x011E
	__RESERVED_FOR_SETRENDERSOURCE,	//0x011F
	LINESARRAY,	//0x0120
	PIXELARRAYR,	//0x0121
	TILEARRAYR,		//0x0122
	COMBOARRAYR,	//0x0123
	RES0000,		//0x0124
	RES0001,		//0x0125
	RES0002,		//0x0126
	RES0003,		//0x0127
	RES0004,		//0x0128
	RES0005,		//0x0129
	RES0006,		//0x012A
	RES0007,		//0x012B
	RES0008,		//0x012C
	RES0009,		//0x012D
	RES000A,		//0x012E
	RES000B,		//0x012F
	RES000C,		//0x0130
	RES000D,		//0x0131
	RES000E,		//0x0132
	RES000F,		//0x0133
	__RESERVED_FOR_CREATELWPN2VV,            //0x0134
	__RESERVED_FOR_CREATELWPN2VR,            //0x0135
	__RESERVED_FOR_CREATELWPN2RV,            //0x0136
	__RESERVED_FOR_CREATELWPN2RR,            //0x0137
	GETSCREENDOOR, //0x0138
	GETSCREENENEMY, //0x0139
	PAUSESFX,
	RESUMESFX,
	CONTINUESFX,
	ADJUSTSFX,
	//__RESERVED_FOR_GETSCREENFLAG, //0x013A
	GETITEMSCRIPT,
	GETSCREENLAYOP,
	GETSCREENSECCMB,
	GETSCREENSECCST,
	GETSCREENSECFLG,
	GETSCREENLAYMAP,
	GETSCREENLAYSCR,
	GETSCREENPATH,
	GETSCREENWARPRX,
	GETSCREENWARPRY,
	TRIGGERSECRETR,
	TRIGGERSECRETV,
	NIL_UNUSED_1,
	NIL_UNUSED_2,
	
	//NPCData
	GETNPCDATAFLAGS,
	GETNPCDATAFLAGS2,
	GETNPCDATAWIDTH,
	GETNPCDATAHEIGHT,
	GETNPCDATASTILE,
	GETNPCDATASWIDTH,
	GETNPCDATASHEIGHT,
	GETNPCDATAETILE,
	GETNPCDATAEWIDTH,
	GETNPCDATAHP,
	GETNPCDATAFAMILY,
	GETNPCDATACSET,
	GETNPCDATAANIM,
	GETNPCDATAEANIM,
	GETNPCDATAFRAMERATE,
	GETNPCDATAEFRAMERATE,
	GETNPCDATATOUCHDMG,
	GETNPCDATAWPNDAMAGE,
	GETNPCDATAWEAPON,
	GETNPCDATARANDOM,
	GETNPCDATAHALT,
	GETNPCDATASTEP,
	GETNPCDATAHOMING,
	GETNPCDATAHUNGER,
	GETNPCDATADROPSET,
	GETNPCDATABGSFX,
	GETNPCDATADEATHSFX,
	GETNPCDATAXOFS,
	GETNPCDATAYOFS,
	GETNPCDATAZOFS,
	GETNPCDATAHXOFS,
	GETNPCDATAHYOFS,
	GETNPCDATAHITWIDTH,
	GETNPCDATAHITHEIGHT,
	GETNPCDATAHITZ,
	GETNPCDATATILEWIDTH,
	GETNPCDATATILEHEIGHT,
	GETNPCDATAWPNSPRITE,
	//TWO INPUTS, ONE RETURN
	GETNPCDATASCRIPTDEF,
	GETNPCDATADEFENSE,
	GETNPCDATASIZEFLAG,
	GETNPCDATAATTRIBUTE,

	//TWO INPUTS, ONE RETURN
	SETNPCDATAFLAGS,
	SETNPCDATAFLAGS2,
	SETNPCDATAWIDTH,
	SETNPCDATAHEIGHT,
	SETNPCDATASTILE,
	SETNPCDATASWIDTH,
	SETNPCDATASHEIGHT,
	SETNPCDATAETILE,
	SETNPCDATAEWIDTH,
	SETNPCDATAHP,
	SETNPCDATAFAMILY,
	SETNPCDATACSET,
	SETNPCDATAANIM,
	SETNPCDATAEANIM,
	SETNPCDATAFRAMERATE,
	SETNPCDATAEFRAMERATE,
	SETNPCDATATOUCHDMG,
	SETNPCDATAWPNDAMAGE,
	SETNPCDATAWEAPON,
	SETNPCDATARANDOM,
	SETNPCDATAHALT,
	SETNPCDATASTEP,
	SETNPCDATAHOMING,
	SETNPCDATAHUNGER,
	SETNPCDATADROPSET,
	SETNPCDATABGSFX,
	SETNPCDATADEATHSFX,
	SETNPCDATAXOFS,
	SETNPCDATAYOFS,
	SETNPCDATAZOFS,
	SETNPCDATAHXOFS,
	SETNPCDATAHYOFS,
	SETNPCDATAHITWIDTH,
	SETNPCDATAHITHEIGHT,
	SETNPCDATAHITZ,
	SETNPCDATATILEWIDTH,
	SETNPCDATATILEHEIGHT,
	SETNPCDATAWPNSPRITE,
	SETNPCDATAHITSFX,
	GETNPCDATAHITSFX,
	//ComboData
	GCDBLOCKENEM,
	GCDBLOCKHOLE,
	GCDBLOCKTRIG,
	GCDCONVEYSPDX,
	GCDCONVEYSPDY,
	GCDCREATEENEM,
	GCDCREATEENEMWH,
	GCDCREATEENEMCH,
	GCDDIRCHTYPE,
	GCDDISTCHTILES,
	GCDDIVEITEM,
	GCDDOCK,
	GCDFAIRY,
	GCDFFCOMBOATTRIB,
	GCDFOOTDECOTILE,
	GCDFOOTDECOTYPE,
	GCDHOOKSHOTGRAB,
	GCDLADDERPASS,
	GCDLOCKBLOCKTYPE,
	GCDLOCKBLOCKCHANGE,
	GCDMAGICMIRRORTYPE,
	GCDMODIFYHPAMOUNT,
	GCDMODIFYHPDELAY,
	GCDMODIFYHPTYPE,
	GCDMODIFYMPAMOUNT,
	GCDMODIFYMPDELAY,
	GCDMODIFYMPTYPE,
	GCDNOPUSHBLOCKS,
	GCDOVERHEAD,
	GCDPLACEENEMY,
	GCDPUSHDIR,
	GCDPUSHWEIGHT,
	GCDPUSHWAIT,
	GCDPUSHED,
	GCDRAFT,
	GCDRESETROOM,
	GCDSAVEPOINT,
	GCDSCREENFREEZE,
	GCDSECRETCOMBO,
	GCDSINGULAR,
	GCDSLOWMOVE,
	GCDSTATUE,
	GCDSTEPTYPE,
	GCDSTEPCHANGETO,
	GCDSTRIKEREMNANTS,
	GCDSTRIKEREMNANTSTYPE,
	GCDSTRIKECHANGE,
	GCDSTRIKECHANGEITEM,
	GCDTOUCHITEM,
	GCDTOUCHSTAIRS,
	GCDTRIGGERTYPE,
	GCDTRIGGERSENS,
	GCDWARPTYPE,
	GCDWARPSENS,
	GCDWARPDIRECT,
	GCDWARPLOCATION,
	GCDWATER,
	GCDWHISTLE,
	GCDWINGAME,
	GCDBLOCKWEAPLVL,
	GCDTILE,
	GCDFLIP,
	GCDWALK,
	GCDTYPE,
	GCDCSETS,
	GCDFOO,
	GCDFRAMES,
	GCDSPEED,
	GCDNEXTCOMBO,
	GCDNEXTCSET,
	GCDFLAG,
	GCDSKIPANIM,
	GCDNEXTTIMER,
	GCDSKIPANIMY,
	GCDANIMFLAGS,
	GCDBLOCKWEAPON,
	GCDEXPANSION,
	GCDSTRIKEWEAPONS,
	SCDBLOCKENEM,
	SCDBLOCKHOLE,
	SCDBLOCKTRIG,
	SCDCONVEYSPDX,
	SCDCONVEYSPDY,
	SCDCREATEENEM,
	SCDCREATEENEMWH,
	SCDCREATEENEMCH,
	SCDDIRCHTYPE,
	SCDDISTCHTILES,
	SCDDIVEITEM,
	SCDDOCK,
	SCDFAIRY,
	SCDFFCOMBOATTRIB,
	SCDFOOTDECOTILE,
	SCDFOOTDECOTYPE,
	SCDHOOKSHOTGRAB,
	SCDLADDERPASS,
	SCDLOCKBLOCKTYPE,
	SCDLOCKBLOCKCHANGE,
	SCDMAGICMIRRORTYPE,
	SCDMODIFYHPAMOUNT,
	SCDMODIFYHPDELAY,
	SCDMODIFYHPTYPE,
	SCDMODIFYMPAMOUNT,
	SCDMODIFYMPDELAY,
	SCDMODIFYMPTYPE,
	SCDNOPUSHBLOCKS,
	SCDOVERHEAD,
	SCDPLACEENEMY,
	SCDPUSHDIR,
	SCDPUSHWEIGHT,
	SCDPUSHWAIT,
	SCDPUSHED,
	SCDRAFT,
	SCDRESETROOM,
	SCDSAVEPOINT,
	SCDSCREENFREEZE,
	SCDSECRETCOMBO,
	SCDSINGULAR,
	SCDSLOWMOVE,
	SCDSTATUE,
	SCDSTEPTYPE,
	SCDSTEPCHANGETO,
	SCDSTRIKEREMNANTS,
	SCDSTRIKEREMNANTSTYPE,
	SCDSTRIKECHANGE,
	SCDSTRIKECHANGEITEM,
	SCDTOUCHITEM,
	SCDTOUCHSTAIRS,
	SCDTRIGGERTYPE,
	SCDTRIGGERSENS,
	SCDWARPTYPE,
	SCDWARPSENS,
	SCDWARPDIRECT,
	SCDWARPLOCATION,
	SCDWATER,
	SCDWHISTLE,
	SCDWINGAME,
	SCDBLOCKWEAPLVL,
	SCDTILE,
	SCDFLIP,
	SCDWALK,
	SCDTYPE,
	SCDCSETS,
	SCDFOO,
	SCDFRAMES,
	SCDSPEED,
	SCDNEXTCOMBO,
	SCDNEXTCSET,
	SCDFLAG,
	SCDSKIPANIM,
	SCDNEXTTIMER,
	SCDSKIPANIMY,
	SCDANIMFLAGS,
	GETNPCDATATILE,
	GETNPCDATAEHEIGHT,
	SETNPCDATATILE,
	SETNPCDATAEHEIGHT,
	//SpriteData
	GETSPRITEDATASTRING,
	GETSPRITEDATATILE,
	GETSPRITEDATAMISC,
	GETSPRITEDATACGETS,
	GETSPRITEDATAFRAMES,
	GETSPRITEDATASPEED,
	GETSPRITEDATATYPE,
	SETSPRITEDATASTRING,
	SETSPRITEDATATILE,
	SETSPRITEDATAMISC,
	SETSPRITEDATACSETS,
	SETSPRITEDATAFRAMES,
	SETSPRITEDATASPEED,
	SETSPRITEDATATYPE,
	SETCONTINUESCREEN,
	SETCONTINUESTRING,
	LOADNPCDATAR,
	LOADNPCDATAV,
	LOADCOMBODATAR,
	LOADCOMBODATAV,
	LOADMAPDATAR,
	LOADMAPDATAV,
	LOADSPRITEDATAR,
	LOADSPRITEDATAV,
	LOADSCREENDATAR,
	LOADSCREENDATAV,
	LOADBITMAPDATAR,
	LOADBITMAPDATAV,
	LOADSHOPR,
	LOADSHOPV,
	LOADINFOSHOPR,
	LOADINFOSHOPV,
	
	LOADMESSAGEDATAR, //COMMAND
	LOADMESSAGEDATAV,//COMMAND

	MESSAGEDATASETSTRINGR, //command
	MESSAGEDATASETSTRINGV, //command
	MESSAGEDATAGETSTRINGR, //command
	MESSAGEDATAGETSTRINGV, //command
	
	LOADDMAPDATAR,
	LOADDMAPDATAV,
	DMAPDATAGETNAMER,
	DMAPDATAGETNAMEV,
	DMAPDATASETNAMER,
	DMAPDATASETNAMEV,
	DMAPDATAGETTITLER,
	DMAPDATAGETTITLEV,
	DMAPDATASETTITLER,
	DMAPDATASETTITLEV,
	DMAPDATAGETINTROR,
	DMAPDATAGETINTROV,
	DMAPDATANSETITROR,
	DMAPDATASETINTROV,
	DMAPDATAGETMUSICR,
	DMAPDATAGETMUSICV,
	DMAPDATASETMUSICR,
	DMAPDATASETMUSICV,
	
	ADJUSTSFXVOLUMER,		
	ADJUSTSFXVOLUMEV,		
	ADJUSTVOLUMER,		
	ADJUSTVOLUMEV,	
	
	FXWAVYR,
	FXWAVYV,
	FXZAPR,
	FXZAPV,
	GREYSCALER,
	GREYSCALEV,

	RETURN,
	MONOCHROMER,
	MONOCHROMEV,
	CLEARTINT,
	TINT,
	MONOHUE,
	
	BMPRECTR,
	BMPCIRCLER,
	BMPARCR,
	BMPELLIPSER,
	BMPLINER,
	BMPSPLINER,
	BMPPUTPIXELR,
	BMPDRAWTILER,
	BMPDRAWCOMBOR,
	BMPFASTTILER,
	BMPFASTCOMBOR,
	BMPDRAWCHARR,
	BMPDRAWINTR,
	BMPDRAWSTRINGR,
	BMPQUADR,
	BMPQUAD3DR,
	BMPTRIANGLER,
	BMPTRIANGLE3DR,
	BMPPOLYGONR,
	BMPDRAWLAYERR,
	BMPDRAWSCREENR,
	BMPBLIT,
	LINKWARPEXR,
	LINKWARPEXV,
	LINKEXPLODER,
	LINKEXPLODEV,
	NPCEXPLODER,
	NPCEXPLODEV,
	ITEMEXPLODER,
	ITEMEXPLODEV,
	LWEAPONEXPLODER,
	LWEAPONEXPLODEV,
	EWEAPONEXPLODER,
	EWEAPONEXPLODEV,
	RUNITEMSCRIPT,
	GETRTCTIMER,
	GETRTCTIMEV,
	
	//new npc functions for npc scripts
	NPCDEAD,
	NPCKICKBUCKET,
	NPCSTOPBGSFX,
	NPCCANMOVE,
	NPCNEWDIR8,
	NPCNEWDIR,
	NPCCONSTWALK,
	NPCCONSTWALK8,
	NPCVARWALK,
	NPCVARWALK8,
	NPCHALTWALK,
	NPCHALTWALK8,
	NPCFLOATWALK,
	// moved to a var: NPCLINEDUP,
	NPCLINKINRANGE,
	NPCATTACK,
	NPCPLACEONAXIS,
	NPCADD,
	NPCFIREBREATH,
	NPCCANSLIDE,
	NPCSLIDE,
	NPCHITWITH,
	// moved to a var: NPCCOLLISION 
	NPCGETINITDLABEL,
	GAMECONTINUE,
	MAPDATAISSOLID,
	SHOWF6SCREEN,
	NPCDATAGETNAME,
	PLAYENHMUSICEX,
	GETENHMUSICPOS,
	SETENHMUSICPOS,
	SETENHMUSICSPEED,
	ISVALIDBITMAP,
	READBITMAP,
	WRITEBITMAP,
	ALLOCATEBITMAP,
	CLEARBITMAP,
	REGENERATEBITMAP,
	BMPBLITTO,
	BMPDRAWSCREENSOLIDR,
	BMPDRAWSCREENCOMBOFR,
	BMPDRAWSCREENCOMBOIR,
	BMPDRAWSCREENCOMBOTR,
	BMPDRAWSCREENSOLID2R,
	GRAPHICSGETPIXEL,
	
	BMPDRAWLAYERSOLIDR,
	BMPDRAWLAYERCFLAGR,
	BMPDRAWLAYERCTYPER,
	BMPDRAWLAYERCIFLAGR,
	BMPDRAWLAYERSOLIDITYR,
	BMPMODE7,
	BITMAPGETPIXEL,
	NOP,
	STRINGCOMPARE,
	STRINGNCOMPARE,
	STRINGLENGTH,
	STRINGCOPY,
	CASTBOOLI,
	CASTBOOLF,
	SETTRUEI,
	SETFALSEI,
	SETMOREI,
	SETLESSI,
		//2 INPUT 0 RETURN 
	ARRAYCOPY,
	ARRAYNCOPY,
	    //1 INPUT, NO RETURN 
	REMCHR,
	STRINGUPPERLOWER,
	STRINGLOWERUPPER,
	STRINGCONVERTCASE,
	    //1 input, 1 ret
	XLEN,
	XTOI,
	ILEN,
	ATOI,
	    //2 INPUT, 1 RET
	STRCSPN,
	STRSTR,
	XTOA,
	ITOA,
	STRCAT,
	STRSPN,
	STRCHR,
	STRRCHR,
	    //2 INP, 1 RET OVERLOADS
	XLEN2,
	XTOI2,
	ILEN2,
	ATOI2,
	REMCHR2,    
	    //3 INPUT 1 RET 
	XTOA3,
	STRCATF,
	ITOA3,
	STRSTR3,
	REMNCHR3,
	STRCAT3,
	STRNCAT3,
	STRCHR3,
	STRRCHR3,
	STRSPN3,
	STRCSPN3,
	UPPERTOLOWER,
	LOWERTOUPPER,
	CONVERTCASE,
	//Game->Get
	GETNPCSCRIPT,
	GETLWEAPONSCRIPT,
	GETEWEAPONSCRIPT,
	GETHEROSCRIPT,
	GETGLOBALSCRIPT,
	GETDMAPSCRIPT,
	GETSCREENSCRIPT,
	GETSPRITESCRIPT,
	GETUNTYPEDSCRIPT,
	GETSUBSCREENSCRIPT,
	GETNPCBYNAME,
	GETITEMBYNAME,
	GETCOMBOBYNAME,
	GETDMAPBYNAME,
	
	SRNDR,
	SRNDV,
	SRNDRND,
	SAVEGAMESTRUCTS,
	READGAMESTRUCTS,
	
	ANDR32,
	ANDV32,
	ORR32,
	ORV32,
	XORR32,
	XORV32,
	BITNOT32,
	LSHIFTR32,
	LSHIFTV32,
	RSHIFTR32,
	RSHIFTV32,
	ISALLOCATEDBITMAP,

	FONTHEIGHTR,
	STRINGWIDTHR,
	CHARWIDTHR,
	MESSAGEWIDTHR,
	MESSAGEHEIGHTR,
	ISVALIDARRAY,
	DIREXISTS,
	GAMESAVEQUIT,
	GAMESAVECONTINUE,
	DRAWTILECLOAKEDR,
	BMPDRAWTILECLOAKEDR,
	DRAWCOMBOCLOAKEDR,
	BMPDRAWCOMBOCLOAKEDR,
	NPCKNOCKBACK,
	CLOSEWIPE,
	OPENWIPESHAPE,
	CLOSEWIPESHAPE,
	FILEEXISTS,
	BITMAPCLEARTOCOLOR,
	LOADNPCBYSUID,
	LOADLWEAPONBYSUID,
	LOADWEAPONCBYSUID,
	LOADDROPSETR,
    LOADTMPSCR,
    LOADSCROLLSCR,
    MAPDATAISSOLIDLYR,
    ISSOLIDLAYER,
	BREAKPOINT,
	TOBYTE,
	TOWORD,
	TOSHORT,
	TOSIGNEDBYTE,
	TOINTEGER,
	FLOOR,
	CEILING,
	
	FILECLOSE,
	FILEFREE,
	FILEISALLOCATED,
	FILEISVALID,
	FILEALLOCATE,
	FILEFLUSH,
	FILEGETCHAR,
	FILEREWIND,
	FILECLEARERR,
	
	FILEOPEN,
	FILECREATE,
	FILEREADSTR,
	FILEWRITESTR,
	FILEPUTCHAR,
	FILEUNGETCHAR,
	
	FILEREADCHARS,
	FILEREADINTS,
	FILEWRITECHARS,
	FILEWRITEINTS,
	FILESEEK,
	FILEOPENMODE,
	FILEGETERROR,
	
	BITMAPFREE,
	
	POPARGS,
	GAMERELOAD,
	
	READPODARRAYR,
	READPODARRAYV,
	WRITEPODARRAYRR,
	WRITEPODARRAYRV,
	WRITEPODARRAYVR,
	WRITEPODARRAYVV,
	
	PRINTFV,
	SPRINTFV,
	
	STRCMPR,
	STRICMPR,
	STRINGICOMPARE,
	STRINGNICOMPARE,
	
	FILEREMOVE,
	FILESYSREMOVE,
	
	DRAWSTRINGR2,
	BMPDRAWSTRINGR2,
	
	MODULEGETIC,
	ITOACAT,
	
	FRAMER,
	BMPFRAMER,
	
	LOADDIRECTORYR,
	DIRECTORYGET,
	DIRECTORYRELOAD,
	DIRECTORYFREE,
	FILEWRITEBYTES,
	GETCOMBOSCRIPT,  
	FILEREADBYTES,
	
	LOADRNG,
	RNGRAND1,
	RNGRAND2,
	RNGRAND3,
	RNGLRAND1,
	RNGLRAND2,
	RNGLRAND3,
	RNGSEED,
	RNGRSEED,
	RNGFREE,
	LWPNDEL,
	EWPNDEL,
	ITEMDEL,
	
	BMPWRITETILE,
	BMPDITHER,
	BMPREPLCOLOR,
	BMPSHIFTCOLOR,
	BMPMASKDRAW,
	RESIZEARRAYR,
	
	BSHOPNAMEGET,
	BSHOPNAMESET,
	BOTTLENAMEGET,
	BOTTLENAMESET,
	LOADBOTTLETYPE,
	LOADBSHOPDATA,
	
	SWITCHNPC,
	SWITCHCMB,
	SWITCHITM,
	SWITCHLW,
	SWITCHEW,
	SCREENDOSPAWN,
	NPCMOVEPAUSED,
	NPCMOVE,
	NPCMOVEANGLE,
	NPCMOVEXY,
	NPCCANMOVEDIR,
	NPCCANMOVEANGLE,
	NPCCANMOVEXY,
	
	SELECTXWPNR,
	SELECTYWPNR,
	BITMAPOWN,
	FILEOWN,
	DIRECTORYOWN,
	RNGOWN,
	LOADGENERICDATA,
	RUNGENFRZSCR,
	WAITTO,
	GETGENERICSCRIPT,
	KILLPLAYER,
	DEGTORAD,
	RADTODEG,
	LWPNMAKEANGULAR,
	EWPNMAKEANGULAR,
	LWPNMAKEDIRECTIONAL,
	EWPNMAKEDIRECTIONAL,
	BMPMASKDRAW2,
	BMPMASKDRAW3,
	BMPMASKBLIT,
	BMPMASKBLIT2,
	BMPMASKBLIT3,
	SUBV2,
	DIVV2,
	COMPAREV2,
	MODV2,
	
	STACKFREE,
	STACKOWN,
	STACKGET,
	STACKSET,
	STACKPOPBACK,
	STACKPOPFRONT,
	STACKPEEKBACK,
	STACKPEEKFRONT,
	STACKPUSHBACK,
	STACKPUSHFRONT,
	LOADSTACK,
	STACKCLEAR,
	POWERV2,
	LPOWERR,
	LPOWERV,
	LPOWERV2,
	SCRTRIGGERCOMBO,
	WAITEVENT,
	OWNARRAYR,
	DESTROYARRAYR,
	GRAPHICSCOUNTCOLOR,
	WRITEPODSTRING,
	WRITEPODARRAY,
	ZCLASS_CONSTRUCT,
	ZCLASS_READ,
	ZCLASS_WRITE,
	ZCLASS_FREE,
	ZCLASS_OWN,
	STARTDESTRUCTOR,
	ZCLASS_GLOBALIZE,
	LOADD,
	STORED,
	OBJ_OWN_BITMAP,
	OBJ_OWN_PALDATA,
	OBJ_OWN_FILE,
	OBJ_OWN_DIR,
	OBJ_OWN_STACK,
	OBJ_OWN_RNG,
	OBJ_OWN_CLASS,
	OBJ_OWN_ARRAY,
	QUIT_NO_DEALLOC,
	GAMESETCUSTOMCURSOR,
	NPCCANPLACE,
	ITEMGETDISPLAYNAME,
	ITEMSETDISPLAYNAME,
	ITEMGETSHOWNNAME,
	HEROMOVEXY,
	HEROCANMOVEXY,
	HEROLIFTRELEASE,
	HEROLIFTGRAB,
	LOADPORTAL,
	CREATEPORTAL,
	LOADSAVPORTAL,
	CREATESAVPORTAL,
	
	
	CREATEPALDATA,
	CREATEPALDATACLR,
	MIXCLR,
	CREATERGBHEX,
	CREATERGB,
	PALDATALOADLEVEL,
	PALDATALOADSPRITE,
	PALDATALOADMAIN,
	PALDATALOADCYCLE,
	PALDATALOADBITMAP,
	PALDATAWRITELEVEL,
	PALDATAWRITELEVELCS,
	PALDATAWRITESPRITE,
	PALDATAWRITESPRITECS,
	PALDATAWRITEMAIN,
	PALDATAWRITEMAINCS,
	PALDATAWRITECYCLE,
	PALDATAWRITECYCLECS,
	PALDATAVALIDCLR,
	PALDATACLEARCLR,
	PALDATACLEARCSET,
	PALDATAMIX,
	PALDATAMIXCS,
	PALDATACOPY,
	PALDATACOPYCSET,
	PALDATAFREE,
	PALDATAOWN,
	MAXVARG,
	MINVARG,
	CHOOSEVARG,
	PUSHVARGV,
	PUSHVARGR,
	PRINTFVARG,
	SPRINTFVARG,
	TRACELR,
	WAITFRAMESR,
	RESRVD_OP_Z3_01,
	RESRVD_OP_Z3_02,
	RESRVD_OP_Z3_03,
	RESRVD_OP_Z3_04,
	RESRVD_OP_Z3_05,
	RESRVD_OP_Z3_06,
	RESRVD_OP_Z3_07,
	RESRVD_OP_Z3_08,
	RESRVD_OP_Z3_09,
	RESRVD_OP_Z3_10,
	PORTALREMOVE,
	SAVEDPORTALREMOVE,
	SAVEDPORTALGENERATE,
	PORTALUSESPRITE,
	HEROMOVEATANGLE,
	HEROCANMOVEATANGLE,
	HEROMOVE,
	HEROCANMOVE,
	DRAWLIGHT_CIRCLE,
	DRAWLIGHT_SQUARE,
	DRAWLIGHT_CONE,
	PEEK,
	PEEKATV,
	MAKEVARGARRAY,
	PRINTFA,
	SPRINTFA,
	CURRENTITEMID,
	ARRAYPUSH,
	ARRAYPOP,
	LOADSUBDATARV,
	SWAPSUBSCREENV,
	SUBDATA_GET_NAME,
	SUBDATA_SET_NAME,

	CONVERTFROMRGB,
	CONVERTTORGB,
	GETENHMUSICLEN,
	SETENHMUSICLOOP,
	PLAYSOUNDEX,
	GETSFXCOMPLETION,
	ENHCROSSFADE,
	RESRVD_OP_MOOSH_08,
	RESRVD_OP_MOOSH_09,
	RESRVD_OP_MOOSH_10,
	
	
	SUBDATA_SWAP_PAGES,
	SUBPAGE_FIND_WIDGET,
	SUBPAGE_MOVE_SEL,
	SUBPAGE_SWAP_WIDG,
	SUBPAGE_NEW_WIDG,
	SUBPAGE_DELETE,
	SUBWIDG_GET_SELTEXT_OVERRIDE,
	SUBWIDG_SET_SELTEXT_OVERRIDE,
	SUBWIDG_TY_GETTEXT,
	SUBWIDG_TY_SETTEXT,

	
	SUBPAGE_FIND_WIDGET_BY_LABEL,

	
	SUBWIDG_GET_LABEL,
	SUBWIDG_SET_LABEL,
	PUSHARGSR,
	PUSHARGSV,
	PUSHVARGSR,
	PUSHVARGSV,

	
	WRAPRADIANS,
	WRAPDEGREES,

	
	CALLFUNC,
	RETURNFUNC,

	
	SETCMP,
	GOTOCMP,
	STACKWRITEATRV,
	STACKWRITEATVV,

	
	TRUNCATE,
	ROUND,
	ROUNDAWAY,
	STOREDV,

	
	STACKWRITEATVV_IF,

	
	SETGVARR,
	SETGVARV,
	GETGVAR,

	NUMCOMMANDS  //0x045F
};


//{ ZASM registers
#define DATA                                                             0x000A
#define FCSET                                                            0x000B
#define DELAY                                                            0x000C
#define FX                                                               0x000D
#define FY                                                               0x000E
#define XD                                                               0x000F
#define YD                                                               0x0010
#define XD2                                                              0x0011
#define YD2                                                              0x0012
#define FFFLAG                                                           0x0013
#define WIDTH                                                            0x0014
#define HEIGHT                                                           0x0015
#define LINK                                                             0x0016
#define INPUTSTART                                                       0x0227
#define INPUTUP                                                          0x0228
#define INPUTDOWN                                                        0x0229
#define INPUTLEFT                                                        0x022A
#define INPUTRIGHT                                                       0x022B
#define INPUTA                                                           0x022C
#define INPUTB                                                           0x022D
#define INPUTL                                                           0x022E
#define INPUTR                                                           0x022F
#define LINKX                                                            0x0230
#define LINKY                                                            0x0231
#define LINKDIR                                                          0x0232
#define LINKHP                                                           0x0233
#define LINKMP                                                           0x0234
#define LINKMAXHP                                                        0x0235
#define LINKMAXMP                                                        0x0236
#define LINKACTION                                                       0x0237
#define LINKITEMD                                                        0x0238
#define LINKZ                                                            0x0239
#define LINKJUMP                                                         0x023A
#define LINKSWORDJINX                                                    0x023B
#define LINKITEMJINX                                                     0x023C
#define LINKDRUNK                                                        0x023D
#define LINKHELD                                                         0x023E
#define INPUTMAP                                                         0x023F
#define LINKEQUIP                                                        0x0240
#define LINKINVIS                                                        0x0241
#define LINKINVINC                                                       0x0242
#define LINKLADDERX                                                      0x0243
#define LINKLADDERY                                                      0x0244
#define LINKHITDIR                                                       0x0245
#define LINKTILE                                                         0x0246
#define LINKFLIP                                                         0x0247
#define INPUTPRESSMAP                                                    0x0248
#define LINKDIAG                                                         0x0249
#define LINKBIGHITBOX                                                    0x024A
#define __RESERVED_FOR_LINKRETSQUARE                                     0x024B
#define __RESERVED_FOR_LINKWARPSOUND                                     0x024C
#define LINKUSINGITEM                                                    0x024D
#define LINKUSINGITEMA                                                   0x024E
#define LINKUSINGITEMB                                                   0x024F
#define __RESERVED_FOR_PLAYWARPSOUND                                     0x0250
#define __RESERVED_FOR_WARPEFFECT                                        0x0251
#define __RESERVED_FOR_PLAYPITWARPSFX                                    0x0252
#define LINKEATEN                                                        0x0253
#define __RESERVED_FOR_LINKEXTEND                                        0x0254
#define UNUSED37                                                         0x0255
#define UNUSED38                                                         0x0256
#define UNUSED39                                                         0x0257
#define UNUSED40                                                         0x0258
#define LWPNX                                                            0x0259
#define LWPNY                                                            0x025A
#define LWPNDIR                                                          0x025B
#define LWPNSTEP                                                         0x025C
#define LWPNANGULAR                                                      0x025D
#define LWPNANGLE                                                        0x025E
#define LWPNDRAWTYPE                                                     0x025F
#define LWPNPOWER                                                        0x0260
#define LWPNDEAD                                                         0x0261
#define LWPNID                                                           0x0262
#define LWPNTILE                                                         0x0263
#define LWPNCSET                                                         0x0264
#define LWPNFLASHCSET                                                    0x0265
#define LWPNFRAMES                                                       0x0266
#define LWPNFRAME                                                        0x0267
#define LWPNASPEED                                                       0x0268
#define LWPNFLASH                                                        0x0269
#define LWPNFLIP                                                         0x026A
#define LWPNCOUNT                                                        0x026B
#define LWPNEXTEND                                                       0x026C
#define LWPNOTILE                                                        0x026D
#define LWPNOCSET                                                        0x026E
#define LWPNZ                                                            0x026F
#define LWPNJUMP                                                         0x0270
#define LWPNCOLLDET                                                      0x0271
#define LWPNRANGE                                                        0x0272
#define LWPNPARENT                                                       0x0273
#define LWPNLEVEL                                                        0x0274
#define GETRENDERTARGET                                                  0x0275
#define UNUSED51                                                         0x0276
#define UNUSED52                                                         0x0277
#define UNUSED53                                                         0x0278
#define UNUSED54                                                         0x0279
#define UNUSED55                                                         0x027A
#define UNUSED56                                                         0x027B
#define UNUSED57                                                         0x027C
#define UNUSED58                                                         0x027D
#define UNUSED59                                                         0x027E
#define UNUSED60                                                         0x027F
#define UNUSED61                                                         0x0280
#define UNUSED62                                                         0x0281
#define UNUSED63                                                         0x0282
#define UNUSED64                                                         0x0283
#define UNUSED65                                                         0x0284
#define UNUSED66                                                         0x0285
#define UNUSED67                                                         0x0286
#define UNUSED68                                                         0x0287
#define UNUSED69                                                         0x0288
#define UNUSED70                                                         0x0289
#define NPCPARENTUID                                                     0x028A
#define EWPNPARENT                                                       0x028B
#define ITEMX                                                            0x028C
#define ITEMY                                                            0x028D
#define ITEMDRAWTYPE                                                     0x028E
#define ITEMID                                                           0x028F
#define ITEMTILE                                                         0x0290
#define ITEMCSET                                                         0x0291
#define ITEMFLASHCSET                                                    0x0292
#define ITEMFRAMES                                                       0x0293
#define ITEMFRAME                                                        0x0294
#define ITEMASPEED                                                       0x0295
#define ITEMDELAY                                                        0x0296
#define ITEMFLASH                                                        0x0297
#define ITEMFLIP                                                         0x0298
#define ITEMCOUNT                                                        0x0299
#define IDATAFAMILY                                                      0x029A
#define IDATALEVEL                                                       0x029B
#define IDATAKEEP                                                        0x029C
#define IDATAAMOUNT                                                      0x029D
#define IDATASETMAX                                                      0x029E
#define IDATAMAX                                                         0x029F
#define IDATACOUNTER                                                     0x02A0
#define ITEMEXTEND                                                       0x02A1
#define ITEMZ                                                            0x02A2
#define ITEMJUMP                                                         0x02A3
#define ITEMOTILE                                                        0x02A4
#define IDATAUSESOUND                                                    0x02A5
#define IDATAPOWER                                                       0x02A6
#define IDATAID                                                          0x02A7
#define IDATALTM                                                         0x02A8
#define IDATAPSCRIPT                                                     0x02A9
#define IDATASCRIPT                                                      0x02AA
#define IDATAMAGCOST                                                     0x02AB
#define IDATAMINHEARTS                                                   0x02AC
#define IDATATILE                                                        0x02AD
#define IDATAMISC                                                        0x02AE
#define IDATACSET                                                        0x02AF
#define IDATAFRAMES                                                      0x02B0
#define IDATAASPEED                                                      0x02B1
#define IDATADELAY                                                       0x02B2
#define IDATAWPN                                                         0x02B3
#define IDATAFRAME                                                       0x02B4
#define ITEMACLK                                                         0x02B5
#define IDATAFLAGS                                                       0x02B6
#define IDATASPRITE                                                      0x02B7
#define IDATAATTRIB                                                      0x02B8
#define ZELDAVERSION                                                     0x02B9
#define NPCX                                                             0x02BA
#define NPCY                                                             0x02BB
#define NPCDIR                                                           0x02BC
#define NPCRATE                                                          0x02BD
#define NPCFRAMERATE                                                     0x02BE
#define NPCHALTRATE                                                      0x02BF
#define NPCDRAWTYPE                                                      0x02C0
#define NPCHP                                                            0x02C1
#define NPCID                                                            0x02C2
#define NPCDP                                                            0x02C3
#define NPCWDP                                                           0x02C4
#define NPCOTILE                                                         0x02C5
#define NPCENEMY                                                         0x02C6
#define NPCWEAPON                                                        0x02C7
#define NPCITEMSET                                                       0x02C8
#define NPCCSET                                                          0x02C9
#define NPCBOSSPAL                                                       0x02CA
#define NPCBGSFX                                                         0x02CB
#define NPCCOUNT                                                         0x02CC
#define NPCEXTEND                                                        0x02CD
#define NPCZ                                                             0x02CE
#define NPCJUMP                                                          0x02CF
#define NPCSTEP                                                          0x02D0
#define NPCTILE                                                          0x02D1
#define NPCHOMING                                                        0x02D2
#define NPCDD                                                            0x02D3
#define NPCTYPE                                                          0x02DE
#define NPCMFLAGS                                                        0x02DF
#define NPCCOLLDET                                                       0x02E0
#define NPCSTUN                                                          0x02E1
#define NPCHUNGER                                                        0x02E2
#define NPCSUPERMAN                                                      0x02E3
#define NPCHASITEM                                                       0x02E4
#define NPCRINGLEAD                                                      0x02E5
#define NPCINVINC                                                        0x02E6
#define NPCSHIELD                                                        0x02E7
#define NPCFROZEN                                                        0x02E8
#define NPCFROZENTILE                                                    0x02E9
#define NPCFROZENCSET                                                    0x02EA
#define UNUSED135                                                        0x02EB
#define SP                                                               0x02EC
#define GAMEDEATHS                                                       0x0855
#define GAMECHEAT                                                        0x0856
#define GAMETIME                                                         0x0857
#define GAMEHASPLAYED                                                    0x0858
#define GAMETIMEVALID                                                    0x0859
#define GAMEGUYCOUNTD                                                    0x085A
#define GAMEGUYCOUNT                                                     0x085B
#define GAMECONTSCR                                                      0x085C
#define GAMECONTDMAP                                                     0x085D
#define GAMECOUNTERD                                                     0x085E
#define GAMEMCOUNTERD                                                    0x085F
#define GAMEDCOUNTERD                                                    0x0860
#define GAMEGENERICD                                                     0x0861
#define GAMEITEMSD                                                       0x0862
#define GAMELITEMSD                                                      0x0863
#define GAMELKEYSD                                                       0x0864
#define ROOMTYPE                                                         0x0865
#define ROOMDATA                                                         0x0866
#define SCREENSTATED                                                     0x0885
#define CURSCR                                                           0x0886
#define CURMAP                                                           0x0887
#define CURDMAP                                                          0x0888
#define COMBODD                                                          0x0889
#define COMBOCD                                                          0x088A
#define COMBOFD                                                          0x088B
#define COMBOTD                                                          0x088C
#define COMBOID                                                          0x088D

#define REFITEMCLASS                                                     0x088E
#define REFITEM                                                          0x088F
#define REFFFC                                                           0x0890
#define REFLWPN                                                          0x0891
#define REFNPC                                                           0x0892
#define REFLWPNCLASS                                                     0x0893
#define REFNPCCLASS                                                      0x0894
#define REFSUBSCREEN                                                     0x0895
#define REFEWPN                                                          0x0896
#define REFEWPNCLASS                                                     0x0897

#define EWPNX                                                            0x0898
#define EWPNY                                                            0x0899
#define EWPNDIR                                                          0x089A
#define EWPNSTEP                                                         0x089B
#define EWPNANGULAR                                                      0x089C
#define EWPNANGLE                                                        0x089D
#define EWPNDRAWTYPE                                                     0x089E
#define EWPNPOWER                                                        0x089F
#define EWPNDEAD                                                         0x08A0
#define EWPNID                                                           0x08A1
#define EWPNTILE                                                         0x08A2
#define EWPNCSET                                                         0x08A3
#define EWPNFLASHCSET                                                    0x08A4
#define EWPNFRAMES                                                       0x08A5
#define EWPNFRAME                                                        0x08A6
#define EWPNASPEED                                                       0x08A7
#define EWPNFLASH                                                        0x08A8
#define EWPNFLIP                                                         0x08A9
#define EWPNCOUNT                                                        0x08AA
#define EWPNEXTEND                                                       0x08AB
#define EWPNOTILE                                                        0x08AC
#define EWPNOCSET                                                        0x08AD
#define EWPNZ                                                            0x08AE
#define EWPNJUMP                                                         0x08AF

#define SCRDOORD                                                         0x08B4
#define CURDSCR                                                          0x08B5
#define INCQST                                                           0x08B6

#define COMBOSD                                                          0x08B7
#define SCREENSTATEDD                                                    0x08B8
#define SDD                                                              0x08B9
#define SDDD                                                             0x08BA
#define GDD                                                              0x08BB

#define FFFLAGSD                                                         0x08BC
#define FFCWIDTH                                                         0x08BD
#define FFCHEIGHT                                                        0x08BE
#define FFTWIDTH                                                         0x08BF
#define FFTHEIGHT                                                        0x08C0
#define FFLINK                                                           0x08C1

#define WAVY                                                             0x09C2
#define QUAKE                                                            0x09C3
#define INPUTMOUSEX                                                      0x09C4
#define INPUTMOUSEY                                                      0x09C5
#define INPUTMOUSEZ                                                      0x09C6
#define INPUTMOUSEB                                                      0x09C7

#define COMBODDM                                                         0x09C8
#define COMBOCDM                                                         0x09C9
#define COMBOFDM                                                         0x09CA
#define COMBOTDM                                                         0x09CB
#define COMBOIDM                                                         0x09CC
#define COMBOSDM                                                         0x09CD

#define SCRIPTRAM                                                        0x09CE
#define GLOBALRAM                                                        0x09CF
#define SCRIPTRAMD                                                       0x09D0
#define GLOBALRAMD                                                       0x09D1

#define LWPNHXOFS                                                        0x09D3
#define LWPNHYOFS                                                        0x09D4
#define LWPNXOFS                                                         0x09D5
#define LWPNYOFS                                                         0x09D6
#define LWPNZOFS                                                         0x09D7
#define LWPNHXSZ                                                         0x09D8
#define LWPNHYSZ                                                         0x09D9
#define LWPNHZSZ                                                         0x09DA

#define EWPNHXOFS                                                        0x09DB
#define EWPNHYOFS                                                        0x09DC
#define EWPNXOFS                                                         0x09DD
#define EWPNYOFS                                                         0x09DE
#define EWPNZOFS                                                         0x09DF
#define EWPNHXSZ                                                         0x09E0
#define EWPNHYSZ                                                         0x09E1
#define EWPNHZSZ                                                         0x09E2

#define NPCHXOFS                                                         0x09E3
#define NPCHYOFS                                                         0x09E4
#define NPCXOFS                                                          0x09E5
#define NPCYOFS                                                          0x09E6
#define NPCZOFS                                                          0x09E7
#define NPCHXSZ                                                          0x09E8
#define NPCHYSZ                                                          0x09E9
#define NPCHZSZ                                                          0x09EA

#define ITEMHXOFS                                                        0x09EB
#define ITEMHYOFS                                                        0x09EC
#define ITEMXOFS                                                         0x09ED
#define ITEMYOFS                                                         0x09EE
#define ITEMZOFS                                                         0x09EF
#define ITEMHXSZ                                                         0x09F0
#define ITEMHYSZ                                                         0x09F1
#define ITEMHZSZ                                                         0x09F2

#define LWPNTXSZ                                                         0x09F3
#define LWPNTYSZ                                                         0x09F4

#define EWPNTXSZ                                                         0x09F5
#define EWPNTYSZ                                                         0x09F6

#define NPCTXSZ                                                          0x09F7
#define NPCTYSZ                                                          0x09F8

#define ITEMTXSZ                                                         0x09F9
#define ITEMTYSZ                                                         0x09FA

#define LINKHXOFS                                                        0x09FB
#define LINKHYOFS                                                        0x09FC
#define LINKXOFS                                                         0x09FD
#define LINKYOFS                                                         0x09FE
#define LINKZOFS                                                         0x09FF
#define LINKHXSZ                                                         0x1000
#define LINKHYSZ                                                         0x1001
#define LINKHZSZ                                                         0x1002
#define LINKTXSZ                                                         0x1003
#define LINKTYSZ                                                         0x1004

#define LWPNBEHIND                                                       0x1005
#define EWPNBEHIND                                                       0x1006

#define SDDDD                                                            0x1007
#define CURLEVEL                                                         0x1008
#define ITEMPICKUP                                                       0x1009
#define LIT                                                              0x100A
#define INPUTEX1                                                         0x100B
#define INPUTEX2                                                         0x100C
#define INPUTEX3                                                         0x100D
#define INPUTEX4                                                         0x100E
#define INPUTPRESSSTART                                                  0x100F
#define INPUTPRESSUP                                                     0x1010
#define INPUTPRESSDOWN                                                   0x1011
#define INPUTPRESSLEFT                                                   0x1012
#define INPUTPRESSRIGHT                                                  0x1013
#define INPUTPRESSA                                                      0x1014
#define INPUTPRESSB                                                      0x1015
#define INPUTPRESSL                                                      0x1016
#define INPUTPRESSR                                                      0x1017
#define INPUTPRESSEX1                                                    0x1018
#define INPUTPRESSEX2                                                    0x1019
#define INPUTPRESSEX3                                                    0x101A
#define INPUTPRESSEX4                                                    0x101B
#define LWPNMISCD                                                        0x101C
#define EWPNMISCD                                                        0x102D
#define NPCMISCD                                                         0x103E
#define ITEMMISCD                                                        0x104F
#define FFMISCD                                                          0x1060
#define GETMIDI                                                          0x1071
#define INPUTAXISUP                                                      0x1072
#define INPUTAXISDOWN                                                    0x1073
#define INPUTAXISLEFT                                                    0x1074
#define INPUTAXISRIGHT                                                   0x1075
#define INPUTPRESSAXISUP                                                 0x1076
#define INPUTPRESSAXISDOWN                                               0x1077
#define INPUTPRESSAXISLEFT                                               0x1078
#define INPUTPRESSAXISRIGHT                                              0x1079

#define FFSCRIPT                                                         0x107A
#define SCREENFLAGSD                                                     0x107B
#define SCREENEFLAGSD                                                    0x1086
#define FFINITDD                                                         0x108B
#define LINKMISCD                                                        0x1094
#define DMAPFLAGSD                                                       0x10A5
#define EWPNCOLLDET                                                      0x10A6

#define NPCDEFENSED                                                      0x10A7
#define DMAPLEVELD                                                       0x10A8
#define DMAPCOMPASSD                                                     0x10A9
#define DMAPCONTINUED                                                    0x10AA
#define DMAPMIDID                                                        0x10AB
#define IDATAINITDD                                                      0x10AC //8

#define GAMESTANDALONE                                                   0x10AD

#define GAMEENTRSCR                                                      0x10AE
#define GAMEENTRDMAP                                                     0x10AF

#define GAMECLICKFREEZE                                                  0x10B0

#define PUSHBLOCKX                                                       0x10B1
#define PUSHBLOCKY                                                       0x10B2
#define PUSHBLOCKCOMBO                                                   0x10B3
#define PUSHBLOCKCSET                                                    0x10B4
#define UNDERCOMBO                                                       0x10B5
#define UNDERCSET                                                        0x10B6
#define DMAPOFFSET                                                       0x10B7
#define DMAPMAP                                                          0x10B8

#define __RESERVED_FOR_GAMETHROTTLE                                      0x10B9

#define REFMAPDATA                                                       0x10BA
#define REFSCREENDATA                                                    0x10BB
#define REFCOMBODATA                                                     0x10BC
#define REFSPRITEDATA                                                    0x10BD
#define REFBITMAP                                                        0x10BE
#define REFDMAPDATA                                                      0x10BF
#define REFSHOPDATA                                                      0x10C0
#define REFMSGDATA                                                       0x10C1
#define REFUNTYPED                                                       0x10C2

#define REFDROPS                                                         0x10C3
#define REFPONDS                                                         0x10C4
#define REFWARPRINGS                                                     0x10C5
#define REFDOORS                                                         0x10C6
#define REFUICOLOURS                                                     0x10C7
#define REFRGB                                                           0x10C8
#define REFPALETTE                                                       0x10C9
#define REFTUNES                                                         0x10CA
#define REFPALCYCLE                                                      0x10CB
#define REFGAMEDATA                                                      0x10CC
#define REFCHEATS                                                        0x10CD

//Itemdata Flags
#define IDATACOMBINE                                                     0x10CE
#define IDATADOWNGRADE                                                   0x10CF
#define IDATAMAGICTIMER                                                  0x10D0
#define IDATAPSTRING                                                     0x10D1
#define IDATAKEEPOLD                                                     0x10D2
#define IDATARUPEECOST                                                   0x10D3
#define IDATAEDIBLE                                                      0x10D4
#define IDATAFLAGUNUSED                                                  0x10D5
#define IDATAGAINLOWER                                                   0x10D6
#define IDATAPFLAGS                                                      0x10D7
#define RESVD024                                                         0x10D8
#define RESVD025                                                         0x10D9

#define NPCSCRDEFENSED                                                   0x10DA

#define __RESERVED_FOR_SETLINKEXTEND                                     0x10DB
#define __RESERVED_FOR_SETLINKTILE                                       0x10DC
#define __RESERVED_FOR_SIDEWARPSFX                                       0x10DD
#define __RESERVED_FOR_PITWARPSFX                                        0x10DE
#define __RESERVED_FOR_SIDEWARPVISUAL                                    0x10DF
#define __RESERVED_FOR_PITWARPVISUAL                                     0x10F0
#define GAMESETA                                                         0x10F1
#define GAMESETB                                                         0x10F2
#define SETITEMSLOT                                                      0x10F3
#define BUTTONPRESS                                                      0x10F4
#define BUTTONINPUT                                                      0x10F5
#define BUTTONHELD                                                       0x10F6
#define RAWKEY                                                           0x10F7
#define READKEY                                                          0x10F8
#define JOYPADPRESS                                                      0x10F9
#define DISABLEDITEM                                                     0x10FA
#define RESVD026                                                         0x10FB
#define UNUSED10FC                                                       0x10FC
#define UNUSED10FD                                                       0x10FD
#define UNUSED10FE                                                       0x10FE
#define UNUSED10FF                                                       0x10FF
#define UNUSED1100                                                       0x1100
#define UNUSED1101                                                       0x1101
#define UNUSED1102                                                       0x1102
#define UNUSED1103                                                       0x1103
#define LINKITEMB                                                        0x1104
#define LINKITEMA                                                        0x1105
#define __RESERVED_FOR_LINKWALKTILE                                      0x1106
#define __RESERVED_FOR_LINKFLOATTILE                                     0x1107
#define __RESERVED_FOR_LINKSWIMTILE                                      0x1108
#define __RESERVED_FOR_LINKDIVETILE                                      0x1109
#define __RESERVED_FOR_LINKSLASHTILE                                     0x110A
#define __RESERVED_FOR_LINKJUMPTILE                                      0x110B
#define __RESERVED_FOR_LINKCHARGETILE                                    0x110C
#define __RESERVED_FOR_LINKSTABTILE                                      0x110D
#define __RESERVED_FOR_LINKCASTTILE                                      0x110E
#define __RESERVED_FOR_LINKHOLD1LTILE                                    0x110F
#define __RESERVED_FOR_LINKHOLD2LTILE                                    0x1110
#define __RESERVED_FOR_LINKHOLD1WTILE                                    0x1111
#define __RESERVED_FOR_LINKHOLD2WTILE                                    0x1112
#define __RESERVED_FOR_LINKPOUNDTILE                                     0x1113
#define __RESERVED_FOR_LINKSWIMSPD                                       0x1114
#define __RESERVED_FOR_LINKWALKANMSPD                                    0x1115
#define __RESERVED_FOR_LINKANIMTYPE                                      0x1116
#define LINKINVFRAME                                                     0x1117
#define LINKCANFLICKER                                                   0x1118
#define LINKHURTSFX                                                      0x1119
#define NOACTIVESUBSC                                                    0x111A
#define UNUSED111B                                                       0x111B
#define ZELDABUILD                                                       0x111C
#define ZELDABETA                                                        0x111D
#define FFCID                                                            0x111E
#define DMAPLEVELPAL                                                     0x111F
#define __RESERVED_FOR_ITEMINDEX                                         0x1120
#define __RESERVED_FOR_LWPNINDEX                                         0x1121
#define __RESERVED_FOR_EWPNINDEX                                         0x1122
#define __RESERVED_FOR_NPCINDEX                                          0x1123
#define __RESERVED_FOR_ITEMPTR                                           0x1124
#define __RESERVED_FOR_NPCPTR                                            0x1125
#define __RESERVED_FOR_LWPNPTR                                           0x1126
#define __RESERVED_FOR_EWPNPTR                                           0x1127


#define SETSCREENDOOR                                                    0x1128
#define SETSCREENENEMY                                                   0x1129
#define GAMEMAXMAPS                                                      0x112A
#define CREATELWPNDX                                                     0x112B
#define __RESERVED_FOR_SCREENFLAG                                        0x112C
#define RESVD112D                                                        0x112D
#define RESVD112E                                                        0x112E

#define RESVD112F                                                        0x112F
#define RESVD1130                                                        0x1130
#define RESVD1131                                                        0x1131
#define RESVD1132                                                        0x1132
#define RESVD1133                                                        0x1133
#define RESVD1134                                                        0x1134
#define RESVD1135                                                        0x1135
#define RESVD1136                                                        0x1136
#define RESVD1137                                                        0x1137
#define RESVD1138                                                        0x1138
#define RESVD1139                                                        0x1139
#define IDATACOSTCOUNTER                                                 0x113A

#define IDATAOVERRIDEFLWEAP                                              0x113B
#define IDATATILEHWEAP                                                   0x113C
#define IDATATILEWWEAP                                                   0x113D
#define IDATAHZSZWEAP                                                    0x113E
#define IDATAHYSZWEAP                                                    0x113F
#define IDATAHXSZWEAP                                                    0x1140
#define IDATADYOFSWEAP                                                   0x1141
#define IDATADXOFSWEAP                                                   0x1142
#define IDATAHYOFSWEAP                                                   0x1143
#define IDATAHXOFSWEAP                                                   0x1144
#define IDATAOVERRIDEFL                                                  0x1145
#define IDATAPICKUP                                                      0x1146
#define IDATATILEH                                                       0x1147
#define IDATATILEW                                                       0x1148
#define IDATAHZSZ                                                        0x1149
#define IDATAHYSZ                                                        0x114A
#define IDATAHXSZ                                                        0x114B
#define IDATADYOFS                                                       0x114C
#define IDATADXOFS                                                       0x114D
#define IDATAHYOFS                                                       0x114E
#define IDATAHXOFS                                                       0x114F

#define IDATAUSEWPN                                                      0x1150
#define IDATAUSEDEF                                                      0x1151
#define IDATAWRANGE                                                      0x1152
#define IDATAUSEMVT                                                      0x1153
#define IDATADURATION                                                    0x1154

#define IDATADUPLICATES                                                  0x1155
#define IDATADRAWLAYER                                                   0x1156
#define IDATACOLLECTFLAGS                                                0x1157
#define IDATAWEAPONSCRIPT                                                0x1158
#define IDATAMISCD                                                       0x1159
#define IDATAWEAPHXOFS                                                   0x115A
#define IDATAWEAPHYOFS                                                   0x115B
#define IDATAWEAPHYSZ                                                    0x115C
#define IDATAWEAPHXSZ                                                    0x115D
#define IDATAWEAPHZSZ                                                    0x115E
#define IDATAWEAPXOFS                                                    0x115F
#define IDATAWEAPYOFS                                                    0x1160
#define IDATAWEAPZOFS                                                    0x1161
#define NPCWEAPSPRITE                                                    0x1162
#define IDATAWPNINITD                                                    0x1163
#define DEBUGREFFFC                                                      0x1164
#define DEBUGREFITEM                                                     0x1165
#define DEBUGREFNPC                                                      0x1166
#define DEBUGREFITEMDATA                                                 0x1167
#define DEBUGREFLWEAPON                                                  0x1168
#define DEBUGREFEWEAPON                                                  0x1169
#define DEBUGSP                                                          0x116A
#define DEBUGGDR                                                         0x116B
#define SCREENWIDTH                                                      0x116C
#define SCREENHEIGHT                                                     0x116D
#define SCREENVIEWX                                                      0x116E
#define SCREENVIEWY                                                      0x116F
#define SCREENGUY                                                        0x1170
#define SCREENSTRING                                                     0x1171
#define SCREENROOM                                                       0x1172
#define SCREENENTX                                                       0x1173
#define SCREENENTY                                                       0x1174
#define SCREENITEM                                                       0x1175
#define SCREENUNDCMB                                                     0x1176
#define SCREENUNDCST                                                     0x1177
#define SCREENCATCH                                                      0x1178
#define SETSCREENLAYOP                                                   0x1179
#define SETSCREENSECCMB                                                  0x117A
#define SETSCREENSECCST                                                  0x117B
#define SETSCREENSECFLG                                                  0x117C
#define SETSCREENLAYMAP                                                  0x117D
#define SETSCREENLAYSCR                                                  0x117E
#define SETSCREENPATH                                                    0x117F
#define SETSCREENWARPRX                                                  0x1180
#define SETSCREENWARPRY                                                  0x1181
#define GAMENUMMESSAGES                                                  0x1182
#define GAMESUBSCHEIGHT                                                  0x1183
#define GAMEPLAYFIELDOFS                                                 0x1184
#define PASSSUBOFS                                                       0x1185

//NPCData
#define SETNPCDATASCRIPTDEF                                              0x1186
#define SETNPCDATADEFENSE                                                0x1187
#define SETNPCDATASIZEFLAG 0x118 
#define SETNPCDATAATTRIBUTE                                              0x1189

#define SCDBLOCKWEAPON                                                   0x118A
#define SCDSTRIKEWEAPONS                                                 0x118B
#define SCDEXPANSION                                                     0x118C
#define SETGAMEOVERELEMENT                                               0x118D
#define SETGAMEOVERSTRING                                                0x118E
#define MOUSEARR                                                         0x118F

#define SPRITEDATATILE                                                   0x1190
#define SPRITEDATAMISC                                                   0x1191
#define SPRITEDATACSETS                                                  0x1192
#define SPRITEDATAFRAMES                                                 0x1193
#define SPRITEDATASPEED                                                  0x1194
#define SPRITEDATATYPE                                                   0x1195

#define NPCDATATILE                                                      0x1196
#define NPCDATAHEIGHT                                                    0x1197
#define NPCDATAFLAGS                                                     0x1198
#define NPCDATAFLAGS2                                                    0x1199
#define NPCDATAWIDTH                                                     0x119A
#define NPCDATAHITSFX                                                    0x119B
#define NPCDATASTILE                                                     0x119C
#define NPCDATASWIDTH                                                    0x119D
#define NPCDATASHEIGHT                                                   0x119E
#define NPCDATAETILE                                                     0x119F
#define NPCDATAEWIDTH                                                    0x11A0
#define NPCDATAEHEIGHT                                                   0x11A1
#define NPCDATAHP                                                        0x11A2
#define NPCDATAFAMILY                                                    0x11A3
#define NPCDATACSET                                                      0x11A4
#define NPCDATAANIM                                                      0x11A5
#define NPCDATAEANIM                                                     0x11A6
#define NPCDATAFRAMERATE                                                 0x11A7
#define NPCDATAEFRAMERATE                                                0x11A8
#define NPCDATATOUCHDAMAGE                                               0x11A9
#define NPCDATAWEAPONDAMAGE                                              0x11AA
#define NPCDATAWEAPON                                                    0x11AB
#define NPCDATARANDOM                                                    0x11AC
#define NPCDATAHALT                                                      0x11AD
#define NPCDATASTEP                                                      0x11AE
#define NPCDATAHOMING                                                    0x11AF
#define NPCDATAHUNGER                                                    0x11B0
#define NPCDATADROPSET                                                   0x11B1
#define NPCDATABGSFX                                                     0x11B2
#define NPCDATADEATHSFX                                                  0x11B3
#define NPCDATAXOFS                                                      0x11B4
#define NPCDATAYOFS                                                      0x11B5
#define NPCDATAZOFS                                                      0x11B6
#define NPCDATAHXOFS                                                     0x11B7
#define NPCDATAHYOFS                                                     0x11B8
#define NPCDATAHITWIDTH                                                  0x11B9
#define NPCDATAHITHEIGHT                                                 0x11BA
#define NPCDATAHITZ                                                      0x11BB
#define NPCDATATILEWIDTH                                                 0x11BC
#define NPCDATATILEHEIGHT                                                0x11BD
#define NPCDATAWPNSPRITE                                                 0x11BE
#define NPCDATADEFENSE                                                   0x11BF
#define NPCDATASIZEFLAG                                                  0x11C0
#define NPCDATAATTRIBUTE                                                 0x11C1
#define NPCDATASHIELD                                                    0x11C2
#define NPCDATAFROZENTILE                                                0x11C3
#define NPCDATAFROZENCSET                                                0x11C4
#define NPCDATARESERVED005                                               0x11C5
#define NPCDATARESERVED006                                               0x11C6
#define NPCDATARESERVED007                                               0x11C7
#define NPCDATARESERVED008                                               0x11C8
#define NPCDATARESERVED009                                               0x11C9
#define NPCDATARESERVED010                                               0x11CA
#define NPCDATARESERVED011                                               0x11CB
#define NPCDATARESERVED012                                               0x11CC
#define NPCDATARESERVED013                                               0x11CD
#define NPCDATARESERVED014                                               0x11CE
#define NPCDATARESERVED015                                               0x11CF
#define NPCDATARESERVED016                                               0x11D0
#define NPCDATARESERVED017                                               0x11D1
#define NPCDATARESERVED018                                               0x11D2
#define NPCDATARESERVED019                                               0x11D3

#define MAPDATAVALID                                                     0x11D4 //b
#define MAPDATAGUY                                                       0x11D5 //b
#define MAPDATASTRING                                                    0x11D6 //w
#define MAPDATAROOM                                                      0x11D7 //b
#define MAPDATAITEM                                                      0x11D8 //b
#define MAPDATAHASITEM                                                   0x11D9 //b
#define MAPDATATILEWARPTYPE                                              0x11DA //b, 4 of these
#define MAPDATATILEWARPOVFLAGS                                           0x11DB //b, tilewarpoverlayflags
#define MAPDATADOORCOMBOSET                                              0x11DC //w
#define MAPDATAWARPRETX                                                  0x11DD //b, 4 of these
#define MAPDATAWARPRETY                                                  0x11DE //b, 4 of these
#define MAPDATAWARPRETURNC                                               0x11DF //w
#define MAPDATASTAIRX                                                    0x11E0 //b
#define MAPDATASTAIRY                                                    0x11E1 //b
#define MAPDATACOLOUR                                                    0x11E2 //w
#define MAPDATAENEMYFLAGS                                                0x11E3 //b
#define MAPDATADOOR                                                      0x11E4 //b, 4 of these
#define MAPDATATILEWARPDMAP                                              0x11E5 //w, 4 of these
#define MAPDATATILEWARPSCREEN                                            0x11E6 //b, 4 of these
#define MAPDATAEXITDIR                                                   0x11E7 //b
#define MAPDATAENEMY                                                     0x11E8 //w, 10 of these
#define MAPDATAPATTERN                                                   0x11E9 //b
#define MAPDATASIDEWARPTYPE                                              0x11EA //b, 4 of these
#define MAPDATASIDEWARPOVFLAGS                                           0x11EB //b
#define MAPDATAWARPARRIVALX                                              0x11EC //b
#define MAPDATAWARPARRIVALY                                              0x11ED //b
#define MAPDATAPATH                                                      0x11EE //b, 4 of these
#define MAPDATASIDEWARPSC                                                0x11EF //b, 4 of these
#define MAPDATASIDEWARPDMAP                                              0x11F0 //w, 4 of these
#define MAPDATASIDEWARPINDEX                                             0x11F1 //b
#define MAPDATAUNDERCOMBO                                                0x11F2 //w
#define MAPDATAUNDERCSET                                                 0x11F3 //b
#define MAPDATACATCHALL                                                  0x11F4 //W
#define MAPDATAFLAGS                                                     0x11F5 //B, 11 OF THESE, flags, flags2-flags10
#define MAPDATACSENSITIVE                                                0x11F6 //B
#define MAPDATANORESET                                                   0x11F7 //W
#define MAPDATANOCARRY                                                   0x11F8 //W
#define MAPDATALAYERMAP                                                  0x11F9 //B, 6 OF THESE
#define MAPDATALAYERSCREEN                                               0x11FA //B, 6 OF THESE
#define MAPDATALAYEROPACITY                                              0x11FB //B, 6 OF THESE
#define MAPDATATIMEDWARPTICS                                             0x11FC //W
#define MAPDATANEXTMAP                                                   0x11FD //B
#define MAPDATANEXTSCREEN                                                0x11FE //B
#define MAPDATASECRETCOMBO                                               0x11FF //W, 128 OF THESE
#define MAPDATASECRETCSET                                                0x1200 //B, 128 OF THESE
#define MAPDATASECRETFLAG                                                0x1201 //B, 128 OF THESE
#define MAPDATAVIEWX                                                     0x1202 //W
#define MAPDATAVIEWY                                                     0x1203 //W
#define MAPDATASCREENWIDTH                                               0x1204 //B
#define MAPDATASCREENHEIGHT                                              0x1205 //B
#define MAPDATAENTRYX                                                    0x1206 //B
#define MAPDATAENTRYY                                                    0x1207 //B
#define MAPDATANUMFF                                                     0x1208 //INT16
#define MAPDATAFFDATA                                                    0x1209 //W, 32 OF THESE
#define MAPDATAFFCSET                                                    0x120A //B, 32
#define MAPDATAFFDELAY                                                   0x120B //W, 32
#define MAPDATAFFX                                                       0x120C //INT32, 32 OF THESE
#define MAPDATAFFY                                                       0x120D //..
#define MAPDATAFFXDELTA                                                  0x120E //..
#define MAPDATAFFYDELTA                                                  0x120F //..
#define MAPDATAFFXDELTA2                                                 0x1210 //..
#define MAPDATAFFYDELTA2                                                 0x1211 //..
#define MAPDATAFFFLAGS                                                   0x1212 //INT16, 23 OF THESE
#define MAPDATAFFWIDTH                                                   0x1213 //B, 32 OF THESE
#define MAPDATAFFHEIGHT                                                  0x1214 //B, 32 OF THESE
#define MAPDATAFFLINK                                                    0x1215 //B, 32 OF THESE
#define MAPDATAFFSCRIPT                                                  0x1216 //W, 32 OF THESE
#define MAPDATAINTID                                                     0x1217 //INT32 , 32 OF THESE, EACH WITH 10 INDICES. 
#define MAPDATAINITA                                                     0x1218 //INT32, 32 OF THESE, EACH WITH 2
#define MAPDATAFFINITIALISED                                             0x1219 //BOOL, 32 OF THESE
#define MAPDATASCRIPTENTRY                                               0x121A //W
#define MAPDATASCRIPTOCCUPANCY                                           0x121B //W
#define MAPDATASCRIPTEXIT                                                0x121C //W
#define MAPDATAOCEANSFX                                                  0x121D //B
#define MAPDATABOSSSFX                                                   0x121E //B
#define MAPDATASECRETSFX                                                 0x121F //B
#define MAPDATAHOLDUPSFX                                                 0x1220 //B
#define MAPDATASCREENMIDI                                                0x1221 //SHORT, OLD QUESTS ONLY?
#define MAPDATALENSLAYER                                                 0x1222 //B, OLD QUESTS ONLY?

#define SCREENDATAVALID                                                  0x1223 //b
#define SCREENDATAGUY                                                    0x1224 //b
#define SCREENDATASTRING                                                 0x1225 //w
#define SCREENDATAROOM                                                   0x1226 //b
#define SCREENDATAITEM                                                   0x1227 //b
#define SCREENDATAHASITEM                                                0x1228 //b
#define SCREENDATATILEWARPTYPE                                           0x1229 //b, 4 of these
#define SCREENDATATILEWARPOVFLAGS                                        0x122A //b, tilewarpoverlayflags
#define SCREENDATADOORCOMBOSET                                           0x122B //w
#define SCREENDATAWARPRETX                                               0x122C //b, 4 of these
#define SCREENDATAWARPRETY                                               0x122D //b, 4 of these
#define SCREENDATAWARPRETURNC                                            0x122E //w
#define SCREENDATASTAIRX                                                 0x122F //b
#define SCREENDATASTAIRY                                                 0x1230 //b
#define SCREENDATACOLOUR                                                 0x1231 //w
#define SCREENDATAENEMYFLAGS                                             0x1232 //b
#define SCREENDATADOOR                                                   0x1233 //b, 4 of these
#define SCREENDATATILEWARPDMAP                                           0x1234 //w, 4 of these
#define SCREENDATATILEWARPSCREEN                                         0x1235 //b, 4 of these
#define SCREENDATAEXITDIR                                                0x1236 //b
#define SCREENDATAENEMY                                                  0x1237 //w, 10 of these
#define SCREENDATAPATTERN                                                0x1238 //b
#define SCREENDATASIDEWARPTYPE                                           0x1239 //b, 4 of these
#define SCREENDATASIDEWARPOVFLAGS                                        0x123A //b
#define SCREENDATAWARPARRIVALX                                           0x123B //b
#define SCREENDATAWARPARRIVALY                                           0x123C //b
#define SCREENDATAPATH                                                   0x123D //b, 4 of these
#define SCREENDATASIDEWARPSC                                             0x123E //b, 4 of these
#define SCREENDATASIDEWARPDMAP                                           0x123F //w, 4 of these
#define SCREENDATASIDEWARPINDEX                                          0x1240 //b
#define SCREENDATAUNDERCOMBO                                             0x1241 //w
#define SCREENDATAUNDERCSET                                              0x1242 //b
#define SCREENDATACATCHALL                                               0x1243 //W
#define SCREENDATAFLAGS                                                  0x1244 //B, 11 OF THESE, flags, flags2-flags10
#define SCREENDATACSENSITIVE                                             0x1245 //B
#define SCREENDATANORESET                                                0x1246 //W
#define SCREENDATANOCARRY                                                0x1247 //W
#define SCREENDATALAYERMAP                                               0x1248 //B, 6 OF THESE
#define SCREENDATALAYERSCREEN                                            0x1249 //B, 6 OF THESE
#define SCREENDATALAYEROPACITY                                           0x124A //B, 6 OF THESE
#define SCREENDATATIMEDWARPTICS                                          0x124B //W
#define SCREENDATANEXTMAP                                                0x124C //B
#define SCREENDATANEXTSCREEN                                             0x124D //B
#define SCREENDATASECRETCOMBO                                            0x124E //W, 128 OF THESE
#define SCREENDATASECRETCSET                                             0x124F //B, 128 OF THESE
#define SCREENDATASECRETFLAG                                             0x1250 //B, 128 OF THESE
#define SCREENDATAVIEWX                                                  0x1251 //W
#define SCREENDATAVIEWY                                                  0x1252 //W
#define SCREENDATASCREENWIDTH                                            0x1253 //B
#define SCREENDATASCREENHEIGHT                                           0x1254 //B
#define SCREENDATAENTRYX                                                 0x1255 //B
#define SCREENDATAENTRYY                                                 0x1256 //B
#define SCREENDATANUMFF                                                  0x1257 //INT16
#define SCREENDATAFFDATA                                                 0x1258 //W, 32 OF THESE
#define SCREENDATAFFCSET                                                 0x1259 //B, 32
#define SCREENDATAFFDELAY                                                0x125A //W, 32
#define SCREENDATAFFX                                                    0x125B //INT32, 32 OF THESE
#define SCREENDATAFFY                                                    0x125C //..
#define SCREENDATAFFXDELTA                                               0x125D //..
#define SCREENDATAFFYDELTA                                               0x125E //..
#define SCREENDATAFFXDELTA2                                              0x125F //..
#define SCREENDATAFFYDELTA2                                              0x1260 //..
#define SCREENDATAFFFLAGS                                                0x1261 //INT16, 23 OF THESE
#define SCREENDATAFFWIDTH                                                0x1262 //B, 32 OF THESE
#define SCREENDATAFFHEIGHT                                               0x1263 //B, 32 OF THESE
#define SCREENDATAFFLINK                                                 0x1264 //B, 32 OF THESE
#define SCREENDATAFFSCRIPT                                               0x1265 //W, 32 OF THESE
#define SCREENDATAINTID                                                  0x1266 //INT32 , 32 OF THESE, EACH WITH 10 INDICES. 
#define SCREENDATAINITA                                                  0x1267 //INT32, 32 OF THESE, EACH WITH 2
#define SCREENDATAFFINITIALISED                                          0x1268 //BOOL, 32 OF THESE
#define SCREENDATASCRIPTENTRY                                            0x1269 //W
#define SCREENDATASCRIPTOCCUPANCY                                        0x126A //W
#define SCREENDATASCRIPTEXIT                                             0x126B //W
#define SCREENDATAOCEANSFX                                               0x126C //B
#define SCREENDATABOSSSFX                                                0x126D //B
#define SCREENDATASECRETSFX                                              0x126E //B
#define SCREENDATAHOLDUPSFX                                              0x126F //B
#define SCREENDATASCREENMIDI                                             0x1270 //SHORT, OLD QUESTS ONLY?
#define SCREENDATALENSLAYER                                              0x1271 //B, OLD QUESTS ONLY?

#define LINKSCRIPTTILE                                                   0x1272

#define MAPDATAITEMX                                                     0x1273
#define MAPDATAITEMY                                                     0x1274
#define SCREENDATAITEMX                                                  0x1275
#define SCREENDATAITEMY                                                  0x1276

#define MAPDATAFFEFFECTWIDTH                                             0x1277
#define MAPDATAFFEFFECTHEIGHT                                            0x1278
#define SCREENDATAFFEFFECTWIDTH                                          0x1279
#define SCREENDATAFFEFFECTHEIGHT                                         0x127A

#define MAPDATAMISCD                                                     0x127B

#define MAPDATACOMBODD                                                   0x127C
#define MAPDATACOMBOCD                                                   0x127D
#define MAPDATACOMBOFD                                                   0x127E
#define MAPDATACOMBOTD                                                   0x127F
#define MAPDATACOMBOID                                                   0x1280
#define MAPDATACOMBOSD                                                   0x1281

#define MAPDATASCREENSTATED                                              0x1282
#define MAPDATASCREENFLAGSD                                              0x1283
#define MAPDATASCREENEFLAGSD                                             0x1284
#define LINKSCRIPFLIP                                                    0x1285

#define LOADMAPDATA                                                      0x1286

#define SHOPDATANAME                                                     0x1287
#define SHOPDATAITEM                                                     0x1288
#define SHOPDATAHASITEM                                                  0x1289
#define SHOPDATAPRICE                                                    0x128A
//Pay for info
#define SHOPDATASTRING                                                   0x128B

#define AUDIOVOLUME                                                      0x128C
#define AUDIOPAN                                                         0x128D

#define MESSAGEDATANEXT                                                  0x128E //W
#define MESSAGEDATATILE                                                  0x128F //W
#define MESSAGEDATACSET                                                  0x1290 //b
#define MESSAGEDATATRANS                                                 0x1291 //BOOL
#define MESSAGEDATAFONT                                                  0x1292 //B
#define MESSAGEDATAX                                                     0x1293 //SHORT
#define MESSAGEDATAY                                                     0x1294 //SHORT
#define MESSAGEDATAW                                                     0x1295 //UNSIGNED SHORT
#define MESSAGEDATAH                                                     0x1296 //UNSIGNED SHORT
#define MESSAGEDATASFX                                                   0x1297 //BYTE
#define MESSAGEDATALISTPOS                                               0x1298 //WORD
#define MESSAGEDATAVSPACE                                                0x1299 //BYTE
#define MESSAGEDATAHSPACE                                                0x129A //BYTE
#define MESSAGEDATAFLAGS                                                 0x129B //BYTE


#define DMAPDATAMAP                                                      0x129C //byte
#define DMAPDATALEVEL                                                    0x129D //word
#define DMAPDATAOFFSET                                                   0x129E //char
#define DMAPDATACOMPASS                                                  0x129F //byte
#define DMAPDATAPALETTE                                                  0x12A0 //word
#define DMAPDATAMIDI                                                     0x12A1 //byte
#define DMAPDATACONTINUE                                                 0x12A2 //byte
#define DMAPDATATYPE                                                     0x12A3 //byte
#define DMAPDATAGRID                                                     0x12A4 //byte[8] --array
#define DMAPDATAMINIMAPTILE                                              0x12A5 //word - two of these, so let's do MinimapTile[2]
#define DMAPDATAMINIMAPCSET                                              0x12A6 //byte - two of these, so let's do MinimapCSet[2]
#define DMAPDATALARGEMAPTILE                                             0x12A7 //word -- two of these, so let's to LargemapTile[2]
#define DMAPDATALARGEMAPCSET                                             0x12A8 //word -- two of these, so let's to LargemaCSet[2]
#define DMAPDATAMUISCTRACK                                               0x12A9 //byte
#define DMAPDATASUBSCRA                                                  0x12AA //byte, active subscreen
#define DMAPDATASUBSCRP                                                  0x12AB //byte, passive subscreen
#define DMAPDATADISABLEDITEMS                                            0x12AC //byte[MAXITEMS]
#define DMAPDATAFLAGS                                                    0x12AD //int32_t
//Audio->Volume()
#define AUUDIOUNUSED0001                                                 0x12AE
#define AUUDIOUNUSED0002                                                 0x12AF

#define ITEMPSTRING                                                      0x12B0
#define ITEMPSTRINGFLAGS                                                 0x12B1
#define ITEMOVERRIDEFLAGS                                                0x12B2
#define LINKPUSH                                                         0x12B3
#define GAMEMISC                                                         0x12B4 //int32_t
#define LINKSTUN                                                         0x12B5 //int32_t (32b)
#define TYPINGMODE                                                       0x12B6 //int32_t (32b)

//NEWCOMBO STRUCT
#define COMBODTILE                                                       0x12B7 //word
#define COMBODFLIP                                                       0x12B8 //char
#define COMBODWALK                                                       0x12B9 //char
#define COMBODTYPE                                                       0x12BA //char
#define COMBODCSET                                                       0x12BC //C
#define COMBODFOO                                                        0x12BD //W
#define COMBODFRAMES                                                     0x12BE //C
#define COMBODNEXTD                                                      0x12BF //W
#define COMBODNEXTC                                                      0x12C0 //C
#define COMBODFLAG                                                       0x12C1 //C
#define COMBODSKIPANIM                                                   0x12C2 //C
#define COMBODNEXTTIMER                                                  0x12C3 //W
#define COMBODAKIMANIMY                                                  0x12C4 //C
#define COMBODANIMFLAGS                                                  0x12C5 //C
#define COMBODEXPANSION                                                  0x12C6 //C , 6 INDICES
#define COMBODATTRIBUTES                                                 0x12C7 //LONG, 4 INDICES, INDIVIDUAL VALUES
#define COMBODUSRFLAGS                                                   0x12C8 //LONG
#define COMBODTRIGGERFLAGS                                               0x12C9 //LONG 3 INDICES AS FLAGSETS
#define COMBODTRIGGERLEVEL                                               0x12CA //LONG

//COMBOCLASS STRUCT
#define COMBODNAME                                                       0x12CB //CHAR[64], STRING
#define COMBODBLOCKNPC                                                   0x12CC //C
#define COMBODBLOCKHOLE                                                  0x12CD //C
#define COMBODBLOCKTRIG                                                  0x12CE //C
#define COMBODBLOCKWEAPON                                                0x12CF //C, 32 INDICES
#define COMBODCONVXSPEED                                                 0x12D0 //SHORT
#define COMBODCONVYSPEED                                                 0x12D1 //SHORT
#define COMBODSPAWNNPC                                                   0x12D2 //W
#define COMBODSPAWNNPCWHEN                                               0x12D3 //C
#define COMBODSPAWNNPCCHANGE                                             0x12D4 //LONG
#define COMBODDIRCHANGETYPE                                              0x12D5 //C
#define COMBODDISTANCECHANGETILES                                        0x12D6 //LONG
#define COMBODDIVEITEM                                                   0x12D7 //SHORT
#define COMBODDOCK                                                       0x12D8 //C
#define COMBODFAIRY                                                      0x12D9 //C
#define COMBODFFATTRCHANGE                                               0x12DA //C
#define COMBODFOORDECOTILE                                               0x12DB //LONG
#define COMBODFOORDECOTYPE                                               0x12DC //C
#define COMBODHOOKSHOTPOINT                                              0x12DD //C
#define COMBODLADDERPASS                                                 0x12DE //C
#define COMBODLOCKBLOCK                                                  0x12DF //C
#define COMBODLOCKBLOCKCHANGE                                            0x12E0 //LONG
#define COMBODMAGICMIRROR                                                0x12E1 //C
#define COMBODMODHPAMOUNT                                                0x12E2 //SHORT
#define COMBODMODHPDELAY                                                 0x12E3 //C
#define COMBODMODHPTYPE                                                  0x12E4 //C
#define COMBODNMODMPAMOUNT                                               0x12E5 //SHORT
#define COMBODMODMPDELAY                                                 0x12E6 //C
#define COMBODMODMPTYPE                                                  0x12E7 //C
#define COMBODNOPUSHBLOCK                                                0x12E8 //C
#define COMBODOVERHEAD                                                   0x12E9 //C
#define COMBODPLACENPC                                                   0x12EA //C
#define COMBODPUSHDIR                                                    0x12EB //C
#define COMBODPUSHWAIT                                                   0x12EC //C
#define COMBODPUSHHEAVY                                                  0x12ED //C
#define COMBODPUSHED                                                     0x12EE //C
#define COMBODRAFT                                                       0x12EF //C
#define COMBODRESETROOM                                                  0x12F0 //C
#define COMBODSAVEPOINTTYPE                                              0x12F1 //C
#define COMBODSCREENFREEZETYPE                                           0x12F2 //C
#define COMBODSECRETCOMBO                                                0x12F3 //C
#define COMBODSINGULAR                                                   0x12F4 //C
#define COMBODSLOWWALK                                                   0x12F5 //C
#define COMBODSTATUETYPE                                                 0x12F6 //C
#define COMBODSTEPTYPE                                                   0x12F7 //C
#define COMBODSTEPCHANGEINTO                                             0x12F8 //LONG
#define COMBODSTRIKEWEAPONS                                              0x12F9 //BYTE, 32 INDICES. 
#define COMBODSTRIKEREMNANTS                                             0x12FA //LONG
#define COMBODSTRIKEREMNANTSTYPE                                         0x12FB //C
#define COMBODSTRIKECHANGE                                               0x12FC //LONG
#define COMBODSTRIKEITEM                                                 0x12FD //SHORT
#define COMBODTOUCHITEM                                                  0x12FE //SHORT
#define COMBODTOUCHSTAIRS                                                0x12FF //C
#define COMBODTRIGGERTYPE                                                0x1300 //C
#define COMBODTRIGGERSENS                                                0x1301 //C
#define COMBODWARPTYPE                                                   0x1302 //C
#define COMBODWARPSENS                                                   0x1303 //C
#define COMBODWARPDIRECT                                                 0x1304 //C
#define COMBODWARPLOCATION                                               0x1305 //C
#define COMBODWATER                                                      0x1306 //C
#define COMBODWHISTLE                                                    0x1307 //C
#define COMBODWINGAME                                                    0x1308 //C
#define COMBODBLOCKWPNLEVEL                                              0x1309 //C
#define LINKHITBY                                                        0x130A //byte[4]
#define LINKDEFENCE                                                      0x130B //byte[256]
#define NPCHITBY                                                         0x130C //byte[4]
#define NPCISCORE                                                        0x130D //byte[4]
#define NPCSCRIPTUID                                                     0x130E //byte[4]
#define LWEAPONSCRIPTUID                                                 0x130F //byte[4]
#define EWEAPONSCRIPTUID                                                 0x1310 //byte[4]
#define ITEMSCRIPTUID                                                    0x1311 //byte[4]
#define DMAPDATASIDEVIEW                                                 0x1312 //byte[4]

#define DONULL                                                           0x1313
#define DEBUGD                                                           0x1314
#define GETPIXEL                                                         0x1315
#define DOUNTYPE                                                         0x1316
#define NPCBEHAVIOUR                                                     0x1317
#define NPCDATABEHAVIOUR                                                 0x1318
#define GRAPHICSMONO                                                     0x1319
#define GRAPHICSTINT                                                     0x131A
#define CREATEBITMAP                                                     0x131B
#define LINKTILEMOD                                                      0x131C
#define NPCINITD                                                         0x131D
#define NPCCOLLISION                                                     0x131E
#define NPCLINEDUP                                                       0x131F
#define NPCDATAINITD                                                     0x1320
#define NPCDATASCRIPT                                                    0x1321
#define NPCMATCHINITDLABEL                                               0x1322
#define LWPNSCRIPT                                                       0x1323
#define LWPNINITD                                                        0x1324
#define ITEMFAMILY                                                       0x1325
#define ITEMLEVEL                                                        0x1326

#define EWPNSCRIPT                                                       0x1327
#define EWPNINITD                                                        0x1328
#define NPCSCRIPT                                                        0x1329
#define DMAPSCRIPT                                                       0x132A
#define DMAPINITD                                                        0x132B
#define SCREENSCRIPT                                                     0x132C
#define SCREENINITD                                                      0x132D
#define LINKINITD                                                        0x132E

#define NPCDATAWEAPONINITD                                               0x132F
#define NPCDATAWEAPONSCRIPT                                              0x1330

#define NPCSCRIPTTILE                                                    0x1331
#define NPCSCRIPTFLIP                                                    0x1332
#define LWPNSCRIPTTILE                                                   0x1333
#define LWPNSCRIPTFLIP                                                   0x1334
#define EWPNSCRIPTTILE                                                   0x1335
#define EWPNSCRIPTFLIP                                                   0x1336

#define LINKENGINEANIMATE                                                0x1337
#define NPCENGINEANIMATE                                                 0x1338
#define LWPNENGINEANIMATE                                                0x1339
#define EWPNENGINEANIMATE                                                0x133A

#define SKIPCREDITS                                                      0x133B
#define SKIPF6                                                           0x133C
#define LWPNUSEWEAPON                                                    0x133D
#define LWPNUSEDEFENCE                                                   0x133E

//Sprite Rotation
#define LWPNROTATION                                                     0x133F
#define EWPNROTATION                                                     0x1340
#define NPCROTATION                                                      0x1341
#define ITEMROTATION                                                     0x1342
#define LINKROTATION                                                     0x1343

//Sprite Scaling
#define LWPNSCALE                                                        0x1344
#define EWPNSCALE                                                        0x1345
#define NPCSCALE                                                         0x1346
#define ITEMSCALE                                                        0x1347
#define LINKSCALE                                                        0x1348
#define ITEMSPRITESCRIPT                                                 0x1349
#define FFRULE                                                           0x134A
#define NUMDRAWS                                                         0x134B
#define MAXDRAWS                                                         0x134C
#define BITMAPWIDTH                                                      0x134D
#define BITMAPHEIGHT                                                     0x134E
#define ALLOCATEBITMAPR                                                  0x134F
#define KEYMODIFIERS                                                     0x1350
#define SIMULATEKEYPRESS                                                 0x1351
#define KEYBINDINGS                                                      0x1352

#define MAPDATASCRIPT                                                    0x1353
#define MAPDATAINITD                                                     0x1354
#define MAPDATAINITDARRAY                                                0x1355
#define COMBODATAID                                                      0x1356

#define LWPNGRAVITY                                                      0x1357
#define EWPNGRAVITY                                                      0x1358
#define NPCGRAVITY                                                       0x1359
#define ITEMGRAVITY                                                      0x135A

#define MAPDATASIDEWARPID                                                0x135B
#define SCREENSIDEWARPID                                                 0x135C
#define LINKGRAVITY                                                      0x135D

#define SCREENDATALAYERINVIS                                             0x135E
#define SCREENDATASCRIPTDRAWS                                            0x135F
#define MAPDATALAYERINVIS                                                0x1360
#define MAPDATASCRIPTDRAWS                                               0x1361
//bytecode

#define ITEMSCRIPTTILE                                                   0x1362
#define ITEMSCRIPTFLIP                                                   0x1363
#define MAPDATAMAP                                                       0x1364
#define MAPDATASCREEN                                                    0x1365
#define IDATAVALIDATE                                                    0x1366
#define DISABLEKEY                                                       0x1367
#define DISABLEBUTTON                                                    0x1368
#define GAMESUSPEND                                                      0x1369
#define LINKOTILE                                                        0x136A
#define LINKOFLIP                                                        0x136B
#define ITEMSPRITEINITD                                                  0x136C
#define ZSCRIPTVERSION                                                   0x136D
#define REFFILE                                                          0x136E
#define LINKCLIMBING                                                     0x136F
#define NPCIMMORTAL                                                      0x1370
#define NPCNOSLIDE                                                       0x1371
#define NPCKNOCKBACKSPEED                                                0x1372
#define NPCNOSCRIPTKB                                                    0x1373
#define HERONOSTEPFORWARD                                                0x1374
#define SCREENDATATWARPRETSQR                                            0x1375
#define SCREENDATASWARPRETSQR                                            0x1376
#define MAPDATATWARPRETSQR                                               0x1377
#define MAPDATASWARPRETSQR                                               0x1378
#define DMAPDATAID                                                       0x1379
#define NPCSUBMERGED                                                     0x137A
#define EWPNPARENTUID                                                    0x137B
#define GAMEGRAVITY                                                      0x137C
#define COMBODASPEED                                                     0x137D
#define DROPSETITEMS                                                     0x137E
#define DROPSETCHANCES                                                   0x137F
#define DROPSETNULLCHANCE                                                0x1380
#define DROPSETCHOOSE                                                    0x1381
#define KEYPRESS                                                         0x1382
#define KEYINPUT                                                         0x1383

#define SPRITEMAXNPC                                                     0x1384
#define SPRITEMAXLWPN                                                    0x1385
#define SPRITEMAXEWPN                                                    0x1386
#define SPRITEMAXITEM                                                    0x1387
#define SPRITEMAXPARTICLE                                                0x1388
#define SPRITEMAXDECO                                                    0x1389
#define EWPNLEVEL                                                        0x138A
#define HEROHEALTHBEEP                                                   0x138B
#define COMBODATTRIBYTES                                                 0x138C
#define NPCRANDOM                                                        0x138D
#define COMBOXR                                                          0x138E
#define COMBOYR                                                          0x138F
#define COMBOPOSR                                                        0x1390
#define COMBODATASCRIPT                                                  0x1391
#define COMBODATAINITD                                                   0x1392
#define HEROSCRIPTCSET                                                   0x1393
#define SHOPDATATYPE                                                     0x1394
#define HEROSTEPS                                                        0x1395
#define HEROSTEPRATE                                                     0x1396
#define COMBODOTILE                                                      0x1397
#define COMBODFRAME                                                      0x1398
#define COMBODACLK                                                       0x1399
#define PC                                                               0x139A
#define GAMESCROLLING                                                    0x139B
#define MESSAGEDATAMARGINS                                               0x139C
#define MESSAGEDATAPORTTILE                                              0x139D
#define MESSAGEDATAPORTCSET                                              0x139E
#define MESSAGEDATAPORTX                                                 0x139F
#define MESSAGEDATAPORTY                                                 0x13A0
#define MESSAGEDATAPORTWID                                               0x13A1
#define MESSAGEDATAPORTHEI                                               0x13A2
#define MESSAGEDATAFLAGSARR                                              0x13A3
#define FILEPOS                                                          0x13A4
#define FILEEOF                                                          0x13A5
#define FILEERR                                                          0x13A6
#define MESSAGEDATATEXTWID                                               0x13A7
#define MESSAGEDATATEXTHEI                                               0x13A8
#define SWITCHKEY                                                        0x13A9
#define HEROJUMPCOUNT                                                    0x13AA

#define HEROPULLDIR                                                      0x13AB
#define HEROPULLCLK                                                      0x13AC
#define HEROFALLCLK                                                      0x13AD
#define HEROFALLCMB                                                      0x13AE
#define HEROMOVEFLAGS                                                    0x13AF
#define ITEMFALLCLK                                                      0x13B0
#define ITEMFALLCMB                                                      0x13B1
#define ITEMMOVEFLAGS                                                    0x13B2
#define LWPNFALLCLK                                                      0x13B3
#define LWPNFALLCMB                                                      0x13B4
#define LWPNMOVEFLAGS                                                    0x13B5
#define EWPNFALLCLK                                                      0x13B6
#define EWPNFALLCMB                                                      0x13B7
#define EWPNMOVEFLAGS                                                    0x13B8
#define NPCFALLCLK                                                       0x13B9
#define NPCFALLCMB                                                       0x13BA
#define NPCMOVEFLAGS                                                     0x13BB
#define ISBLANKTILE                                                      0x13BC
#define LWPNSPECIAL                                                      0x13BD

#define DMAPDATAASUBSCRIPT                                               0x13BE
#define DMAPDATAPSUBSCRIPT                                               0x13BF
#define DMAPDATASUBINITD                                                 0x13C0
#define MODULEGETINT                                                     0x13C1
#define MODULEGETSTR                                                     0x13C2
#define NPCORIGINALHP                                                    0x13C3
#define DMAPDATAMAPSCRIPT                                                0x13C4
#define DMAPDATAMAPINITD                                                 0x13C5

#define CLOCKCLK                                                         0x13C6
#define CLOCKACTIVE                                                      0x13C7
#define NPCHITDIR                                                        0x13C8
#define DMAPDATAFLAGARR                                                  0x13C9

#define LINKCSET                                                         0x13CA
#define NPCSLIDECLK                                                      0x13CB
#define NPCFADING                                                        0x13CC
#define DISTANCE                                                         0x13CD
#define STDARR                                                           0x13CE
#define GHOSTARR                                                         0x13CF
#define TANGOARR                                                         0x13D0
#define NPCHALTCLK                                                       0x13D1
#define NPCMOVESTATUS                                                    0x13D2
#define DISTANCESCALE                                                    0x13D3

#define DMAPDATACHARTED                                                  0x13D4
#define REFDIRECTORY                                                     0x13D5
#define DIRECTORYSIZE                                                    0x13D6
#define LONGDISTANCE                                                     0x13D7
#define LONGDISTANCESCALE                                                0x13D8
#define COMBOED                                                          0x13D9
#define MAPDATACOMBOED                                                   0x13DA
#define COMBODEFFECT                                                     0x13DB
#define SCREENSECRETSTRIGGERED                                           0x13DC
#define ITEMDIR                                                          0x13DD

#define NPCFRAME                                                         0x13DE
#define LINKITEMX                                                        0x13DF
#define LINKITEMY                                                        0x13E0
#define ACTIVESSSPEED                                                    0x13E1
#define HEROISWARPING                                                    0x13E2

#define ITEMGLOWRAD                                                      0x13E3
#define NPCGLOWRAD                                                       0x13E4
#define LWPNGLOWRAD                                                      0x13E5
#define EWPNGLOWRAD                                                      0x13E6

#define ITEMGLOWSHP                                                      0x13E7
#define NPCGLOWSHP                                                       0x13E8
#define LWPNGLOWSHP                                                      0x13E9
#define EWPNGLOWSHP                                                      0x13EA

#define ITEMENGINEANIMATE                                                0x13EB
#define REFRNG                                                           0x13EC
#define LWPNUNBL                                                         0x13ED
#define EWPNUNBL                                                         0x13EE
#define NPCSHADOWSPR                                                     0x13EF
#define LWPNSHADOWSPR                                                    0x13F0
#define EWPNSHADOWSPR                                                    0x13F1
#define ITEMSHADOWSPR                                                    0x13F2
#define NPCSPAWNSPR                                                      0x13F3
#define NPCDEATHSPR                                                      0x13F4
#define NPCDSHADOWSPR                                                    0x13F5
#define NPCDSPAWNSPR                                                     0x13F6
#define NPCDDEATHSPR                                                     0x13F7

#define COMBOLAYERR                                                      0x13F8
#define COMBODATTRISHORTS                                                0x13F9

#define PUSHBLOCKLAYER                                                   0x13FA
#define LINKGRABBED                                                      0x13FB
#define HEROBUNNY                                                        0x13FC
#define GAMELSWITCH                                                      0x13FD
#define GAMEBOTTLEST                                                     0x13FE

#define REFBOTTLETYPE                                                    0x13FF
#define REFBOTTLESHOP                                                    0x1400
#define BOTTLECOUNTER                                                    0x1401
#define BOTTLEAMOUNT                                                     0x1402
#define BOTTLEPERCENT                                                    0x1403
#define BOTTLEFLAGS                                                      0x1404
#define BOTTLENEXT                                                       0x1405
#define BSHOPFILL                                                        0x1406
#define BSHOPCOMBO                                                       0x1407
#define BSHOPCSET                                                        0x1408
#define BSHOPPRICE                                                       0x1409
#define BSHOPSTR                                                         0x140A
#define COMBODUSRFLAGARR                                                 0x140B
#define COMBODGENFLAGARR                                                 0x140C
#define HERORESPAWNX                                                     0x140D
#define HERORESPAWNY                                                     0x140E
#define HERORESPAWNDMAP                                                  0x140F
#define HERORESPAWNSCR                                                   0x1410
#define IDATAUSESOUND2                                                   0x1411
#define HEROSWITCHTIMER                                                  0x1412
#define HEROSWITCHMAXTIMER                                               0x1413
#define NPCSWHOOKED                                                      0x1414
#define GAMEMISCSPR                                                      0x1415
#define GAMEMISCSFX                                                      0x1416
#define HEROTOTALDYOFFS                                                  0x1417
#define NPCTOTALDYOFFS                                                   0x1418
#define LWPNTOTALDYOFFS                                                  0x1419
#define EWPNTOTALDYOFFS                                                  0x141A
#define LWSWHOOKED                                                       0x141B
#define EWSWHOOKED                                                       0x141C
#define ITMSWHOOKED                                                      0x141D
#define DEBUGTESTING                                                     0x141E
#define GAMEMAXCHEAT                                                     0x141F
#define SHOWNMSG                                                         0x1420
#define COMBODTRIGGERFLAGS2                                              0x1421
#define COMBODTRIGGERBUTTON                                              0x1422
#define REFGENERICDATA                                                   0x1423
#define GENDATARUNNING                                                   0x1424
#define GENDATASIZE                                                      0x1425
#define GENDATAEXITSTATE                                                 0x1426
#define GENDATADATA                                                      0x1427
#define GENDATAINITD                                                     0x1428
#define GENDATARELOADSTATE                                               0x1429
#define COMBODCSET2FLAGS                                                 0x142A
#define HEROIMMORTAL                                                     0x142B
#define NPCCANFLICKER                                                    0x142C

#define NPCDROWNCLK                                                      0x142D
#define NPCDROWNCMB                                                      0x142E
#define ITEMDROWNCLK                                                     0x142F
#define ITEMDROWNCMB                                                     0x1430
#define LWPNDROWNCLK                                                     0x1431
#define LWPNDROWNCMB                                                     0x1432
#define EWPNDROWNCLK                                                     0x1433
#define EWPNDROWNCMB                                                     0x1434
#define HERODROWNCLK                                                     0x1435
#define HERODROWNCMB                                                     0x1436
#define NPCFAKEZ                                                         0x1437
#define ITEMFAKEZ                                                        0x1438
#define LWPNFAKEZ                                                        0x1439
#define EWPNFAKEZ                                                        0x143A
#define HEROFAKEZ                                                        0x143B
#define NPCFAKEJUMP                                                      0x143C
#define ITEMFAKEJUMP                                                     0x143D
#define LWPNFAKEJUMP                                                     0x143E
#define EWPNFAKEJUMP                                                     0x143F
#define HEROFAKEJUMP                                                     0x1440
#define HEROSHADOWXOFS                                                   0x1441
#define HEROSHADOWYOFS                                                   0x1442
#define NPCSHADOWXOFS                                                    0x1443
#define NPCSHADOWYOFS                                                    0x1444
#define ITEMSHADOWXOFS                                                   0x1445
#define ITEMSHADOWYOFS                                                   0x1446
#define LWPNSHADOWXOFS                                                   0x1447
#define LWPNSHADOWYOFS                                                   0x1448
#define EWPNSHADOWXOFS                                                   0x1449
#define EWPNSHADOWYOFS                                                   0x144A
#define LWPNDEGANGLE                                                     0x144B
#define EWPNDEGANGLE                                                     0x144C
#define LWPNVX                                                           0x144D
#define LWPNVY                                                           0x144E
#define EWPNVX                                                           0x144F
#define EWPNVY                                                           0x1450
#define LWPNAUTOROTATE                                                   0x1451
#define EWPNAUTOROTATE                                                   0x1452
#define IDATACOSTCOUNTER2                                                0x1453
#define IDATAMAGICTIMER2                                                 0x1454
#define IDATACOST2                                                       0x1455
#define IDATAVALIDATE2                                                   0x1456
#define MESSAGEDATATEXTLEN                                               0x1457
#define LWPNFLAGS                                                        0x1458
#define EWPNFLAGS                                                        0x1459
#define REFSTACK                                                         0x145A
#define STACKSIZE                                                        0x145B
#define STACKFULL                                                        0x145C
#define ITEMFORCEGRAB                                                    0x145D
#define COMBODTRIGGERITEM                                                0x145E
#define COMBODTRIGGERTIMER                                               0x145F
#define COMBODTRIGGERSFX                                                 0x1460
#define COMBODTRIGGERCHANGECMB                                           0x1461
#define SCREENEXSTATED                                                   0x1462
#define MAPDATAEXSTATED                                                  0x1463
#define HEROSTANDING                                                     0x1464
#define COMBODTRIGGERPROX                                                0x1465
#define COMBODTRIGGERLIGHTBEAM                                           0x1466
#define COMBODTRIGGERCTR                                                 0x1467
#define COMBODTRIGGERCTRAMNT                                             0x1468
#define GENDATAEVENTSTATE                                                0x1469
#define GAMEEVENTDATA                                                    0x146A
#define ITEMDROPPEDBY                                                    0x146B
#define GAMEGSWITCH                                                      0x146C
#define COMBODTRIGGERCOOLDOWN                                            0x146D
#define COMBODTRIGGERCOPYCAT                                             0x146E
#define COMBODTRIGITEMPICKUP                                             0x146F
#define COMBODTRIGEXSTATE                                                0x1470
#define COMBODTRIGSPAWNENEMY                                             0x1471
#define COMBODTRIGSPAWNITEM                                              0x1472
#define COMBODTRIGCSETCHANGE                                             0x1473
#define COMBODLIFTGFXCOMBO                                               0x1474
#define COMBODLIFTGFXCCSET                                               0x1475
#define COMBODLIFTUNDERCMB                                               0x1476
#define COMBODLIFTUNDERCS                                                0x1477
#define COMBODLIFTDAMAGE                                                 0x1478
#define COMBODLIFTLEVEL                                                  0x1479
#define COMBODLIFTITEM                                                   0x147A
#define COMBODLIFTFLAGS                                                  0x147B
#define COMBODLIFTGFXTYPE                                                0x147C
#define COMBODLIFTGFXSPRITE                                              0x147D
#define COMBODLIFTSFX                                                    0x147E
#define COMBODLIFTBREAKSPRITE                                            0x147F
#define COMBODLIFTBREAKSFX                                               0x1480
#define COMBODLIFTHEIGHT                                                 0x1481
#define COMBODLIFTTIME                                                   0x1482
#define CLASS_THISKEY                                                    0x1483
#define ZELDABETATYPE                                                    0x1484
#define HEROCOYOTETIME                                                   0x1485
#define FFCLASTCHANGERX                                                  0x1486
#define FFCLASTCHANGERY                                                  0x1487
#define LWPNTIMEOUT                                                      0x1488
#define EWPNTIMEOUT                                                      0x1489
#define COMBODTRIGGERLSTATE                                              0x148A
#define COMBODTRIGGERGSTATE                                              0x148B
#define COMBODTRIGGERGTIMER                                              0x148C
#define GAMEMOUSECURSOR                                                  0x148D
#define COMBODTRIGGERGENSCRIPT                                           0x148E
#define COMBODTRIGGERGROUP                                               0x148F
#define COMBODTRIGGERGROUPVAL                                            0x1490
#define HEROLIFTEDWPN                                                    0x1491
#define HEROLIFTTIMER                                                    0x1492
#define HEROLIFTMAXTIMER                                                 0x1493
#define HEROLIFTHEIGHT                                                   0x1494
#define HEROHAMMERSTATE                                                  0x1495
#define HEROLIFTFLAGS                                                    0x1496
#define COMBODLIFTWEAPONITEM                                             0x1497
#define LWPNDEATHITEM                                                    0x1498
#define LWPNDEATHDROPSET                                                 0x1499
#define LWPNDEATHIPICKUP                                                 0x149A
#define LWPNDEATHSPRITE                                                  0x149B
#define LWPNDEATHSFX                                                     0x149C
#define EWPNDEATHITEM                                                    0x149D
#define EWPNDEATHDROPSET                                                 0x149E
#define EWPNDEATHIPICKUP                                                 0x149F
#define EWPNDEATHSPRITE                                                  0x14A0
#define EWPNDEATHSFX                                                     0x14A1

#define REFPALDATA                                                       0x14A2

#define PALDATACOLOR                                                     0x14A3
#define PALDATAR                                                         0x14A4
#define PALDATAG                                                         0x14A5
#define PALDATAB                                                         0x14A6

#define DMAPDATALOOPSTART                                                0x14A7
#define DMAPDATALOOPEND                                                  0x14A8
#define DMAPDATAXFADEIN                                                  0x14A9
#define DMAPDATAXFADEOUT                                                 0x14AA
#define MUSICUPDATECOND                                                  0x14AB
#define MUSICUPDATEFLAGS                                                 0x14AC
#define DMAPDATAINTROSTRINGID                                            0x14AD
#define IS8BITTILE                                                       0x14AE
#define NPCFLICKERCOLOR                                                  0x14AF
#define HEROFLICKERCOLOR                                                 0x14B0
#define NPCFLASHINGCSET                                                  0x14B1
#define HEROFLASHINGCSET                                                 0x14B2
#define NPCFLICKERTRANSP                                                 0x14B3
#define HEROFLICKERTRANSP                                                0x14B4
#define RESRVD_VAR_MOOSH15                                               0x14B5
#define RESRVD_VAR_MOOSH16                                               0x14B6
#define RESRVD_VAR_MOOSH17                                               0x14B7
#define RESRVD_VAR_MOOSH18                                               0x14B8
#define RESRVD_VAR_MOOSH19                                               0x14B9
#define RESRVD_VAR_MOOSH20                                               0x14BA
#define RESRVD_VAR_MOOSH21                                               0x14BB
#define RESRVD_VAR_MOOSH22                                               0x14BC
#define RESRVD_VAR_MOOSH23                                               0x14BD
#define RESRVD_VAR_MOOSH24                                               0x14BE
#define RESRVD_VAR_MOOSH25                                               0x14BF
#define RESRVD_VAR_MOOSH26                                               0x14C0
#define RESRVD_VAR_MOOSH27                                               0x14C1
#define RESRVD_VAR_MOOSH28                                               0x14C2
#define RESRVD_VAR_MOOSH29                                               0x14C3
#define RESRVD_VAR_MOOSH30                                               0x14C4
#define DMAPDATAMIRRDMAP                                                 0x14C5
#define IDATAGRADUAL                                                     0x14C6
#define IDATASPRSCRIPT                                                   0x14C7
#define IDATAPSOUND                                                      0x14C8
#define IDATACONSTSCRIPT                                                 0x14C9
#define IDATASSWIMDISABLED                                               0x14CA
#define IDATABUNNYABLE                                                   0x14CB
#define IDATAJINXIMMUNE                                                  0x14CC
#define IDATAJINXSWAP                                                    0x14CD
#define SPRITEDATAFLCSET                                                 0x14CE
#define SPRITEDATAFLAGS                                                  0x14CF
#define SPRITEDATAID                                                     0x14D0
#define CLASS_THISKEY2                                                   0x14D1
#define RESRVD_VAR_Z3_01                                                 0x14D2
#define RESRVD_VAR_Z3_02                                                 0x14D3
#define RESRVD_VAR_Z3_03                                                 0x14D4
#define RESRVD_VAR_Z3_04                                                 0x14D5
#define RESRVD_VAR_Z3_05                                                 0x14D6
#define RESRVD_VAR_Z3_06                                                 0x14D7
#define RESRVD_VAR_Z3_07                                                 0x14D8
#define RESRVD_VAR_Z3_08                                                 0x14D9
#define RESRVD_VAR_Z3_09                                                 0x14DA
#define RESRVD_VAR_Z3_10                                                 0x14DB
#define RESRVD_VAR_Z3_11                                                 0x14DC
#define RESRVD_VAR_Z3_12                                                 0x14DD
#define RESRVD_VAR_Z3_13                                                 0x14DE
#define RESRVD_VAR_Z3_14                                                 0x14DF
#define RESRVD_VAR_Z3_15                                                 0x14E0
#define RESRVD_VAR_Z3_16                                                 0x14E1
#define LWPNLIFTLEVEL                                                    0x14E2
#define LWPNLIFTTIME                                                     0x14E3
#define LWPNLIFTHEIGHT                                                   0x14E4
#define EWPNLIFTLEVEL                                                    0x14E5
#define EWPNLIFTTIME                                                     0x14E6
#define EWPNLIFTHEIGHT                                                   0x14E7
#define HEROSHIELDJINX                                                   0x14E8
#define MAPDATALENSSHOWS                                                 0x14E9
#define MAPDATALENSHIDES                                                 0x14EA
#define SCREENLENSSHOWS                                                  0x14EB
#define SCREENLENSHIDES                                                  0x14EC
#define GAMETRIGGROUPS                                                   0x14ED
#define GAMEOVERRIDEITEMS                                                0x14EE
#define DMAPDATASUBSCRO                                                  0x14EF
#define REFSUBSCREENPAGE                                                 0x14F0
#define REFSUBSCREENWIDG                                                 0x14F1
#define SUBDATACURPG                                                     0x14F2
#define SUBDATANUMPG                                                     0x14F3
#define SUBDATAPAGES                                                     0x14F4
#define SUBDATATYPE                                                      0x14F5
#define SUBDATAFLAGS                                                     0x14F6
#define SUBDATACURSORPOS                                                 0x14F7
#define SUBDATASCRIPT                                                    0x14F8
#define SUBDATAINITD                                                     0x14F9
#define SUBDATABTNLEFT                                                   0x14FA
#define SUBDATABTNRIGHT                                                  0x14FB
#define SUBDATATRANSLEFTTY                                               0x14FC
#define SUBDATATRANSLEFTSFX                                              0x14FD
#define SUBDATATRANSLEFTFLAGS                                            0x14FE
#define SUBDATATRANSLEFTARGS                                             0x14FF
#define PORTALX                                                          0x1500
#define PORTALY                                                          0x1501
#define PORTALDMAP                                                       0x1502
#define PORTALSCREEN                                                     0x1503
#define PORTALACLK                                                       0x1504
#define PORTALAFRM                                                       0x1505
#define PORTALOTILE                                                      0x1506
#define PORTALASPD                                                       0x1507
#define PORTALFRAMES                                                     0x1508
#define PORTALSAVED                                                      0x1509
#define PORTALCLOSEDIS                                                   0x150A
#define REFPORTAL                                                        0x150B
#define REFSAVPORTAL                                                     0x150C
#define PORTALWARPSFX                                                    0x150D
#define PORTALWARPVFX                                                    0x150E
#define SAVEDPORTALX                                                     0x150F
#define SAVEDPORTALY                                                     0x1510
#define SAVEDPORTALSRCDMAP                                               0x1511
#define SAVEDPORTALDESTDMAP                                              0x1512
#define SAVEDPORTALSRCSCREEN                                             0x1513
#define SAVEDPORTALWARPSFX                                               0x1514
#define SAVEDPORTALWARPVFX                                               0x1515
#define SAVEDPORTALSPRITE                                                0x1516
#define SAVEDPORTALPORTAL                                                0x1517
#define PORTALCOUNT                                                      0x1518
#define SAVEDPORTALCOUNT                                                 0x1519
#define SAVEDPORTALDSTSCREEN                                             0x151A

#define SUBDATATRANSRIGHTTY                                              0x151B
#define SUBDATATRANSRIGHTSFX                                             0x151C
#define SUBDATATRANSRIGHTFLAGS                                           0x151D
#define SUBDATATRANSRIGHTARGS                                            0x151E
#define SUBDATASELECTORDSTX                                              0x151F
#define SUBDATASELECTORDSTY                                              0x1520
#define SUBDATASELECTORDSTW                                              0x1521
#define SUBDATASELECTORDSTH                                              0x1522
#define SUBDATASELECTORWID                                               0x1523
#define SUBDATASELECTORHEI                                               0x1524
#define SUBDATASELECTORTILE                                              0x1525
#define SUBDATASELECTORCSET                                              0x1526
#define SUBDATASELECTORFRM                                               0x1527
#define SUBDATASELECTORASPD                                              0x1528
#define SUBDATASELECTORDELAY                                             0x1529
#define SUBDATATRANSCLK                                                  0x152A
#define SUBDATATRANSTY                                                   0x152B
#define SUBDATATRANSFLAGS                                                0x152C
#define SUBDATATRANSARGS                                                 0x152D
#define SUBDATATRANSFROMPG                                               0x152E
#define SUBDATATRANSTOPG                                                 0x152F
#define SUBDATASELECTORFLASHCSET                                         0x1530
#define GAMEASUBOPEN                                                     0x1531
#define GAMENUMASUB                                                      0x1532
#define GAMENUMPSUB                                                      0x1533
#define GAMENUMOSUB                                                      0x1534
#define SUBPGINDEX                                                       0x1535
#define SUBPGNUMWIDG                                                     0x1536
#define SUBPGWIDGETS                                                     0x1537
#define SUBPGSUBDATA                                                     0x1538
#define SUBPGCURSORPOS                                                   0x1539
#define SUBWIDGTYPE                                                      0x153A
#define SUBWIDGINDEX                                                     0x153B
#define SUBWIDGPAGE                                                      0x153C
#define SUBWIDGPOS                                                       0x153D
#define SUBWIDGPOSES                                                     0x153E
#define SUBWIDGPOSFLAG                                                   0x153F
#define SUBWIDGX                                                         0x1540
#define SUBWIDGY                                                         0x1541
#define SUBWIDGW                                                         0x1542
#define SUBWIDGH                                                         0x1543
#define SUBWIDGGENFLAG                                                   0x1544
#define SUBWIDGFLAG                                                      0x1545
#define SUBWIDGSELECTORDSTX                                              0x1546
#define SUBWIDGSELECTORDSTY                                              0x1547
#define SUBWIDGSELECTORDSTW                                              0x1548
#define SUBWIDGSELECTORDSTH                                              0x1549
#define SUBWIDGSELECTORWID                                               0x154A
#define SUBWIDGSELECTORHEI                                               0x154B
#define SUBWIDGSELECTORTILE                                              0x154C

#define SUBWIDGSELECTORCSET                                              0x154D
#define SUBWIDGSELECTORFLASHCSET                                         0x154E
#define SUBWIDGSELECTORFRM                                               0x154F
#define SUBWIDGSELECTORASPD                                              0x1550
#define SUBWIDGSELECTORDELAY                                             0x1551
#define SUBWIDGPRESSSCRIPT                                               0x1552
#define SUBWIDGPRESSINITD                                                0x1553
#define SUBWIDGBTNPRESS                                                  0x1554
#define SUBWIDGBTNPG                                                     0x1555
#define SUBWIDGPGMODE                                                    0x1556
#define SUBWIDGPGTARG                                                    0x1557
#define SUBWIDGTRANSPGTY                                                 0x1558
#define SUBWIDGTRANSPGSFX                                                0x1559
#define SUBWIDGTRANSPGFLAGS                                              0x155A
#define SUBWIDGTRANSPGARGS                                               0x155B

#define SUBWIDGTY_CSET                                                   0x155C
#define SUBWIDGTY_TILE                                                   0x155D

#define SUBWIDGTY_FONT                                                   0x155E
#define SUBWIDGTY_ALIGN                                                  0x155F
#define SUBWIDGTY_SHADOWTY                                               0x1560
#define SUBWIDGTY_COLOR_TXT                                              0x1561
#define SUBWIDGTY_COLOR_SHD                                              0x1562
#define SUBWIDGTY_COLOR_BG                                               0x1563

#define SUBWIDGTY_COLOR_OLINE                                            0x1564
#define SUBWIDGTY_COLOR_FILL                                             0x1565

#define SUBWIDGTY_BUTTON                                                 0x1566
#define SUBWIDGTY_COUNTERS                                               0x1567
#define SUBWIDGTY_MINDIG                                                 0x1568
#define SUBWIDGTY_MAXDIG                                                 0x1569
#define SUBWIDGTY_INFITM                                                 0x156A
#define SUBWIDGTY_INFCHAR                                                0x156B
#define SUBWIDGTY_COSTIND                                                0x156C

#define SUBWIDGTY_COLOR_PLAYER                                           0x156D
#define SUBWIDGTY_COLOR_CMPBLNK                                          0x156E
#define SUBWIDGTY_COLOR_CMPOFF                                           0x156F
#define SUBWIDGTY_COLOR_ROOM                                             0x1570
#define SUBWIDGTY_ITEMCLASS                                              0x1571
#define SUBWIDGTY_ITEMID                                                 0x1572
#define SUBWIDGTY_FRAMETILE                                              0x1573
#define SUBWIDGTY_FRAMECSET                                              0x1574
#define SUBWIDGTY_PIECETILE                                              0x1575
#define SUBWIDGTY_PIECECSET                                              0x1576
#define SUBWIDGTY_FLIP                                                   0x1577
#define SUBWIDGTY_NUMBER                                                 0x1578
#define SUBWIDGTY_CORNER                                                 0x1579

#define SUBWIDGTY_FRAMES                                                 0x157A
#define SUBWIDGTY_SPEED                                                  0x157B
#define SUBWIDGTY_DELAY                                                  0x157C
#define SUBWIDGTY_CONTAINER                                              0x157D
#define SUBWIDGTY_GAUGE_WID                                              0x157E
#define SUBWIDGTY_GAUGE_HEI                                              0x157F
#define SUBWIDGTY_UNITS                                                  0x1580
#define SUBWIDGTY_HSPACE                                                 0x1581
#define SUBWIDGTY_VSPACE                                                 0x1582
#define SUBWIDGTY_GRIDX                                                  0x1583
#define SUBWIDGTY_GRIDY                                                  0x1584
#define SUBWIDGTY_ANIMVAL                                                0x1585
#define SUBWIDGTY_SHOWDRAIN                                              0x1586
#define SUBWIDGTY_PERCONTAINER                                           0x1587
#define SUBWIDGTY_TABSIZE                                                0x1588

#define GAMEASUBYOFF                                                     0x1589

#define SUBWIDGDISPITM                                                   0x158A
#define SUBWIDGEQPITM                                                    0x158B

#define SUBWIDG_DISPX                                                    0x158C
#define SUBWIDG_DISPY                                                    0x158D
#define SUBWIDG_DISPW                                                    0x158E
#define SUBWIDG_DISPH                                                    0x158F

#define SCREENSCRDATASIZE                                                0x1590
#define SCREENSCRDATA                                                    0x1591
#define MAPDATASCRDATASIZE                                               0x1592
#define MAPDATASCRDATA                                                   0x1593

#define HEROSHOVEOFFSET                                                  0x1594

#define SCREENDATAGUYCOUNT                                               0x1595
#define MAPDATAGUYCOUNT                                                  0x1596
#define ITEMNOSOUND                                                      0x1597
#define ITEMNOHOLDSOUND                                                  0x1598

#define IDATAUSEBURNSPR                                                  0x1599
#define IDATABURNINGSPR                                                  0x159A
#define LWPNSPRITES                                                      0x159B
#define EWPNSPRITES                                                      0x159C

#define SCREENDATAEXDOOR                                                 0x159D
#define MAPDATAEXDOOR                                                    0x159E
#define COMBODTRIGEXDOORDIR                                              0x159F
#define COMBODTRIGEXDOORIND                                              0x15A0

#define IDATABURNINGLIGHTRAD                                             0x15A1

#define LWPNBURNLIGHTRADIUS                                              0x15A2
#define EWPNBURNLIGHTRADIUS                                              0x15A3

#define IDATAATTRIB_L                                                    0x15A4

#define HEROSLIDING                                                      0x15A5
#define HEROICECMB                                                       0x15A6
#define HEROSCRICECMB                                                    0x15A7
#define HEROICEVX                                                        0x15A8
#define HEROICEVY                                                        0x15A9
#define HEROICEENTRYFRAMES                                               0x15AA
#define HEROICEENTRYMAXFRAMES                                            0x15AB

#define NUMVARIABLES                                                     0x15AC
//} End variables

//{ Start internal arrays

#define INTARR_OFFS 65536
#define INTARR_SCREEN_NPC       (65536+0)
#define INTARR_SCREEN_ITEMSPR   (65536+1)
#define INTARR_SCREEN_LWPN      (65536+2)
#define INTARR_SCREEN_EWPN      (65536+3)
#define INTARR_SCREEN_FFC       (65536+4)
#define INTARR_SCREEN_PORTALS   (65536+5)
#define INTARR_SAVPRTL          (65536+6)

//} End internal arrays

extern script_command command_list[NUMCOMMANDS+1];
extern script_variable variable_list[];

string ZASMVarToString(int32_t arg);
string ZASMArgToString(int32_t arg, int32_t arg_ty);
#endif

