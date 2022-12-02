#include "SymbolDefs.h"

DirectorySymbols DirectorySymbols::singleton = DirectorySymbols();

static AccessorTable DirectoryTable[] =
{
//	  name,                     rettype,                  setorget,     var,              numindex,      funcFlags,                            numParams,   params
	{ "getSize",                ZTID_FLOAT,         GETTER,       DIRECTORYSIZE,    1,             0,                                    1,           { ZTID_DIRECTORY, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },
	{ "GetFilename",            ZTID_BOOL,          FUNCTION,     0,                1,             0,                                    3,           { ZTID_DIRECTORY, ZTID_FLOAT, ZTID_CHAR, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },
	{ "Reload",                 ZTID_VOID,          FUNCTION,     0,                1,             0,                                    1,           { ZTID_DIRECTORY, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },
	{ "Free",                   ZTID_VOID,          FUNCTION,     0,                1,             0,                                    1,           { ZTID_DIRECTORY, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },
	{ "Own",                    ZTID_VOID,          FUNCTION,     0,                1,             0,                                    1,           { ZTID_DIRECTORY, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },
	
	{ "",                       -1,                       -1,           -1,               -1,            0,                                    0,           { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } }
};

DirectorySymbols::DirectorySymbols()
{
	table = DirectoryTable;
	refVar = REFDIRECTORY;
}

void DirectorySymbols::generateCode()
{
	//bool GetFilename(directory, int32_t, char32)
	{
		Function* function = getFunction("GetFilename", 3);
		int32_t label = function->getLabel();
		vector<shared_ptr<Opcode>> code;
		addOpcode2 (code, new OPopRegister(new VarArgument(EXP2)));
		LABELBACK(label);
		addOpcode2 (code, new OPopRegister(new VarArgument(EXP1)));
		//pop pointer
		POPREF();
		LABELBACK(label);
		addOpcode2 (code, new ODirectoryGet(new VarArgument(EXP1), new VarArgument(EXP2)));
		RETURN();
		function->giveCode(code);
	}
	//void Reload(directory)
	{
		Function* function = getFunction("Reload", 1);
		int32_t label = function->getLabel();
		vector<shared_ptr<Opcode>> code;
		//pop pointer
		ASSERT_NON_NUL();
		POPREF();
		LABELBACK(label);
		addOpcode2 (code, new ODirectoryReload());
		RETURN();
		function->giveCode(code);
	}
	//void Free(directory)
	{
		Function* function = getFunction("Free", 1);
		int32_t label = function->getLabel();
		vector<shared_ptr<Opcode>> code;
		//pop pointer
		ASSERT_NON_NUL();
		POPREF();
		LABELBACK(label);
		addOpcode2 (code, new ODirectoryFree());
		RETURN();
		function->giveCode(code);
	}
	//void Own(directory)
	{
		Function* function = getFunction("Own", 1);
		int32_t label = function->getLabel();
		vector<shared_ptr<Opcode>> code;
		//pop pointer
		ASSERT_NON_NUL();
		POPREF();
		LABELBACK(label);
		addOpcode2 (code, new ODirectoryOwn());
		RETURN();
		function->giveCode(code);
	}
}

