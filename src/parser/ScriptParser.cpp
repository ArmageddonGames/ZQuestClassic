//2.53 Updated to 16th Jan, 2017
#include "parser/MetadataVisitor.h"
#include "zsyssimple.h"
#include "ByteCode.h"
#include "CompileError.h"
#include "CompileOption.h"
#include "LibrarySymbols.h"
#include "y.tab.hpp"
#include <iostream>
#include <assert.h>
#include <cstdlib>
#include <string>
#include <memory>

#include "ASTVisitors.h"
#include "DataStructs.h"
#include "Scope.h"
#include "SemanticAnalyzer.h"
#include "BuildVisitors.h"
#include "RegistrationVisitor.h"
#include "ReturnVisitor.h"
#include "ZScript.h"
#include <fmt/format.h>

#ifdef HAS_SENTRY
#define SENTRY_BUILD_STATIC 1
#include "sentry.h"
#endif

using std::unique_ptr;
using std::shared_ptr;
using namespace ZScript;

#include <allegro/alcompat.h>
#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
#define HAVE_DIR_LIST
#endif
static void get_root_path(char* path, int32_t size)
{
#ifdef HAVE_DIR_LIST
	int32_t drive = _al_getdrive();
#else
	int32_t drive = 0;
#endif

    _al_getdcwd(drive, path, size - ucwidth(OTHER_PATH_SEPARATOR));
	fix_filename_case(path);
	fix_filename_slashes(path);
	put_backslash(path);
}

static std::filesystem::path relativize_path(std::string src_path)
{
	char rootpath[PATH_MAX] = {0};
	get_root_path(rootpath, PATH_MAX);
    return std::filesystem::relative(src_path, rootpath);
}

static std::filesystem::path derelativize_path(std::string src_path)
{
	char rootpath[PATH_MAX] = {0};
	get_root_path(rootpath, PATH_MAX);
	char buf[PATH_MAX*2] = {0};
	return (std::filesystem::path(rootpath) / src_path).lexically_normal();
}


extern std::vector<string> ZQincludePaths;

void ScriptParser::initialize(bool has_qrs)
{
	vid = 0;
	fid = 0;
	gid = 1;
	lid = 0;
	CompileError::initialize();
	CompileOption::initialize(has_qrs);
	includePaths.clear();
	includePaths.resize(0);
}
extern uint32_t zscript_failcode;
extern bool zscript_error_out;
extern bool delay_asserts, ignore_asserts;
vector<ZScript::BasicCompileError> casserts;
static unique_ptr<ScriptsData> _compile_helper(string const& filename, bool include_metadata)
{
	using namespace ZScript;
	zscript_failcode = 0;
	zscript_error_out = false;
	if(ignore_asserts) delay_asserts = true;
	casserts.clear();
	try
	{
		zconsole_info("%s", "Pass 1: Parsing");
		zconsole_idle();

		unique_ptr<ASTFile> root(parseFile(filename, true));
		if(zscript_error_out) return nullptr;
		if (!root.get())
		{
			log_error(CompileError::CantOpenSource(NULL));
			return nullptr;
		}

		zconsole_info("%s", "Pass 2: Preprocessing");
		zconsole_idle();

		root->imports.insert(root->imports.begin(), new ASTImportDecl("bindings.zh"));
		if (!ScriptParser::preprocess(root.get(), ScriptParser::recursionLimit))
			return nullptr;
		if(zscript_error_out) return nullptr;

		SimpleCompileErrorHandler handler;
		Program program(*root, &handler);
		if (handler.hasError())
			return nullptr;
		if(zscript_error_out) return nullptr;

		zconsole_info("%s", "Pass 3: Registration");
		zconsole_idle();

		RegistrationVisitor regVisitor(program);
		if(regVisitor.hasFailed()) return nullptr;
		if(zscript_error_out) return nullptr;

		zconsole_info("%s", "Pass 4: Analyzing Code");
		zconsole_idle();

		SemanticAnalyzer semanticAnalyzer(program);
		if (semanticAnalyzer.hasFailed() || regVisitor.hasFailed())
			return nullptr;
		if(zscript_error_out) return nullptr;

		FunctionData fd(program);
		if(zscript_error_out) return nullptr;
		if (fd.globalVariables.size() > MAX_SCRIPT_REGISTERS)
		{
			log_error(CompileError::TooManyGlobal(NULL));
			return nullptr;
		}

		zconsole_info("%s", "Pass 5: Checking code paths");
		zconsole_idle();
		
		ReturnVisitor rv(program);
		if(zscript_error_out) return nullptr;
		if(rv.hasFailed()) return nullptr;
		
		zconsole_info("%s", "Pass 6: Generating object code");
		zconsole_idle();

		unique_ptr<IntermediateData> id(ScriptParser::generateOCode(fd));
		if (!id.get())
			return nullptr;
		if(zscript_error_out) return nullptr;
		
		zconsole_info("%s", "Pass 7: Assembling");
		zconsole_idle();
		
		ScriptAssembler sa(*id.get());
		sa.assemble();
		if (sa.assemble_err) return nullptr;

		unique_ptr<ScriptsData> result(new ScriptsData(sa));
		
		if(!ignore_asserts && casserts.size()) return nullptr;
		if(zscript_error_out) return nullptr;

		if (include_metadata)
		{
			MetadataVisitor md(program);
			if(zscript_error_out) return nullptr;
			if(rv.hasFailed()) return nullptr;
			result->metadata = md.getOutput();
		}

		zconsole_info("%s", "Success!");

		return result;
	}
	catch (compile_exception &e)
	{
		zconsole_error(fmt::format("An unexpected compile error has occurred:\n{}",e.what()));
		zscript_error_out = true;
		return nullptr;
	}
#ifndef _DEBUG
	catch (std::exception &e)
	{
#ifdef HAS_SENTRY
		sentry_value_t event = sentry_value_new_event();
		sentry_value_t exc = sentry_value_new_exception("Parser Runtime Error", e.what());
		sentry_value_set_stacktrace(exc, NULL, 0);
		sentry_event_add_exception(event, exc);
		sentry_capture_event(event);
#endif

		zconsole_error(fmt::format("An unexpected runtime error has occurred:\n{}",e.what()));
		zscript_error_out = true;
		return nullptr;
	}
#endif
}
unique_ptr<ScriptsData> ZScript::compile(string const& filename, bool include_metadata)
{
	auto ret = _compile_helper(filename, include_metadata);
	if(!ignore_asserts)
		for(BasicCompileError const& error : casserts)
			error.handle();
	return ret;
}

int32_t ScriptParser::vid = 0;
int32_t ScriptParser::fid = 0;
int32_t ScriptParser::gid = 1;
int32_t ScriptParser::lid = 0;

string ScriptParser::prepareFilename(string const& filename)
{
	string retval = filename;

	regulate_path(retval);
	return retval;
}

vector<string> ScriptParser::includePaths;

string& cleanInclude(string& includePath)
{
	//Add a `/` to the end of the include path, if it is missing
	int32_t lastnot = includePath.find_last_not_of("/\\");
	int32_t last = includePath.find_last_of("/\\");
	if(lastnot != string::npos)
	{
		if(last == string::npos || last < lastnot)
			includePath += "/";
	}
	regulate_path(includePath);
	return includePath;
}

string* ScriptParser::checkIncludes(string& includePath, string const& importname, vector<string> includes)
{
	for (size_t q = 0; q < includes.size(); ++q ) //Loop through all include paths, or until valid file is found
	{
		includePath = includes.at(q);
		cleanInclude(includePath);
		includePath = prepareFilename(includePath + importname);
		FILE* f = fopen(includePath.c_str(), "r");
		if(!f) continue;
		fclose(f);
		return &includePath;
	}
	return NULL;
}

extern std::vector<std::filesystem::path> force_ignores;
bool ScriptParser::valid_include(ASTImportDecl& decl, string& ret_fname)
{
	if(decl.wasValidated())
	{
		ret_fname = decl.getFilename();
		return true;
	}
	string* fname = NULL;
	string includePath;
	string importname = prepareFilename(decl.getFilename());
	if(!decl.isInclude()) //Check root dir first for imports
	{
		FILE* f = fopen(importname.c_str(), "r");
		if(f)
		{
			fclose(f);
			fname = &importname;
		}
	}
	if(!fname)
	{
		// Scan include paths
		auto ss = std::filesystem::path(importname);
		if (std::filesystem::path(importname).is_absolute())
		{
			fname = &importname;
		}
		else
		{
			int32_t importfound = importname.find_first_not_of("/\\");
			if(importfound != string::npos) //If the import is not just `/`'s and `\`'s...
			{
				if(importfound != 0)
					importname = importname.substr(importfound); //Remove leading `/` and `\`
				//Convert the include string to a proper import path
				fname = checkIncludes(includePath, importname, ZQincludePaths);
				if(!fname)
				{
					fname = checkIncludes(includePath, importname, includePaths);
				}
			}
		}
	}
	string filename = fname ? *fname : prepareFilename(importname); //Check root dir last, if nothing has been found yet.
	ret_fname = filename;
	//Note: If the user gives an absolute path, `relpath` will be that absolute path!
	std::filesystem::path relpath = std::filesystem::path(filename).lexically_normal();
	std::filesystem::path abspath = derelativize_path(filename);
	FILE* f = fopen(abspath.string().c_str(), "r");
	if(f)
	{
		fclose(f);
		if(std::find(force_ignores.begin(), force_ignores.end(), abspath) != force_ignores.end())
		{
			decl.disable();
			return true;
		}
	}
	f = fopen(filename.c_str(), "r");
	if(f)
	{
		fclose(f);
		if(std::find(force_ignores.begin(), force_ignores.end(), relpath) != force_ignores.end())
		{
			decl.disable();
			return true;
		}
		//zconsole_db("Importing filename '%s' successfully", filename.c_str());
		decl.setFilename(filename);
		decl.validate();
		return true;
	}
	else return false;
}

bool ScriptParser::preprocess_one(ASTImportDecl& importDecl, int32_t reclimit)
{
	string filename;
	if(!valid_include(importDecl, filename))
	{
		log_error(CompileError::CantOpenImport(&importDecl, filename));
		return false;
	}
	if(importDecl.isDisabled()) return true;
	unique_ptr<ASTFile> imported(parseFile(filename));
	if (!imported.get())
	{
		log_error(CompileError::CantParseImport(&importDecl, filename));
		return false;
	}

	// Save the AST in the import declaration.
	importDecl.giveTree(imported.release());

	// Recurse on imports.
	if (!preprocess(importDecl.getTree(), reclimit - 1))
		return false;

	return true;
}

bool ScriptParser::preprocess(ASTFile* root, int32_t reclimit)
{
	assert(root);
	
	if (reclimit == 0)
	{
		auto error = CompileError::ImportRecursion(NULL, recursionLimit);
		log_error(error);
		throw compile_exception(error.toString());
	}
	for(auto it = root->inclpaths.begin();
	     it != root->inclpaths.end(); ++it)
	{
		bool dupe = false;
		string& path = cleanInclude((*it)->path);
		for(auto it2 = includePaths.begin();
			it2 != includePaths.end(); ++it2)
		{
			if(!strcmp((*it2).c_str(), path.c_str()))
			{
				dupe = true;
				break;
			}
		}
		if(!dupe)
			includePaths.push_back(path);
	}
	// Repeat parsing process for each of import files
	bool ret = true;
	for (auto it = root->imports.begin();
	     it != root->imports.end(); ++it)
	{
		if(!preprocess_one(**it, reclimit)) ret = false;
	}

	return ret;
}

unique_ptr<IntermediateData> ScriptParser::generateOCode(FunctionData& fdata)
{
	Program& program = fdata.program;
	Scope* scope = &program.getScope();
	TypeStore* typeStore = &program.getTypeStore();
	vector<Datum*>& globalVariables = fdata.globalVariables;

	// Z_message("yes");
	bool failure = false;

	//we now have labels for the functions and ids for the global variables.
	//we can now generate the code to intialize the globals
	unique_ptr<IntermediateData> rval(new IntermediateData(fdata));

	// Push 0s for init stack space.
	/* Why? The stack should already be init'd to all 0, anyway?
	rval->globalsInit.push_back(
			new OSetImmediate(new VarArgument(EXP1),
			                  new LiteralArgument(0)));
	int32_t globalStackSize = *program.getScope().getRootStackSize();
	for (int32_t i = 0; i < globalStackSize; ++i)
		rval->globalsInit.push_back(
				new OPushRegister(new VarArgument(EXP1)));*/

	// Generate variable init code.
	for (vector<Datum*>::iterator it = globalVariables.begin();
	     it != globalVariables.end(); ++it)
	{
		Datum& variable = **it;
		AST& node = *variable.getNode();
		
		CleanupVisitor cv(scope);
		node.execute(cv);

		OpcodeContext oc(typeStore);

		BuildOpcodes bo(scope);
		node.execute(bo, &oc);
		if (bo.hasError()) failure = true;
		appendElements(rval->globalsInit, oc.initCode);
		appendElements(rval->globalsInit, bo.getResult());
		appendElements(rval->globalsInit, oc.deallocCode);
	}

	// Pop off everything.
	/* See above; why push this in the first place?
	for (int32_t i = 0; i < globalStackSize; ++i)
		rval->globalsInit.push_back(
				new OPopRegister(new VarArgument(EXP2)));*/
	
	//Parse the indexes for class variables
	for(UserClass* user_class : program.classes)
	{
		user_class->getScope().parse_ucv();
	}
	
	//globals have been initialized, now we repeat for the functions
	vector<Function*> funs = program.getUserFunctions();
	appendElements(funs, program.getUserClassConstructors());
	appendElements(funs, program.getUserClassDestructors());
	for (vector<Function*>::iterator it = funs.begin(); it != funs.end(); ++it)
	{
		Function& function = **it;
		if(function.is_aliased())
			continue;
		if(function.isTemplateSkip())
		{
			for(auto& fun : function.get_applied_funcs())
				funs.push_back(fun.get());
			continue;
		}
		bool classfunc = function.getFlag(FUNCFLAG_CLASSFUNC) && !function.getFlag(FUNCFLAG_STATIC);
		int puc = 0;
		if(classfunc)
		{
			if(function.getFlag(FUNCFLAG_CONSTRUCTOR))
				puc = puc_construct;
			else if(function.getFlag(FUNCFLAG_DESTRUCTOR))
				puc = puc_destruct;
			else puc = puc_funcs;
		}
		if(function.getFlag(FUNCFLAG_INLINE)) continue; //Skip inline func decls, they are handled at call location -V
		if(puc != puc_construct && function.prototype) continue; //Skip prototype func decls, they are ALSO handled at the call location -V
		ASTFuncDecl& node = *function.node;

		bool isRun = ZScript::isRun(function);
		string scriptname;
		Script* functionScript = function.getScript();
		if (functionScript)
		{
			scriptname = functionScript->getName();
		}
		scope = function.getInternalScope();

		if (classfunc)
		{
			UserClass& user_class = scope->getClass()->user_class;
			
			vector<std::shared_ptr<Opcode>> funccode;
			
			int32_t stackSize = getStackSize(function);
			// Start of the function.
			if (puc == puc_construct)
			{
				vector<Function*> destr = user_class.getScope().getDestructor();
				std::shared_ptr<Opcode> first;
				Function* destructor = destr.size() == 1 ? destr.at(0) : nullptr;
				if(destructor && !destructor->isNil())
				{
					Function* destructor = destr[0];
					first.reset(new OSetImmediateLabel(new VarArgument(EXP1),
						new LabelArgument(destructor->getLabel(), true)));
				}
				else first.reset(new OSetImmediate(new VarArgument(EXP1),
					new LiteralArgument(0)));
				first->setLabel(function.getLabel());
				funccode.push_back(std::move(first));
				addOpcode2(funccode, new OSetRegister(new VarArgument(CLASS_THISKEY2),new VarArgument(CLASS_THISKEY)));
				addOpcode2(funccode, new OConstructClass(new VarArgument(CLASS_THISKEY),
					new VectorArgument(user_class.members)));
				std::vector<int> object_indices;
				for (auto&& member : user_class.getScope().getClassData())
				{
					auto& type = member.second->getNode()->resolveType(scope, nullptr);
					if (type.canHoldObject())
					{
						object_indices.push_back(member.second->getIndex());
						object_indices.push_back((int)type.getScriptObjectTypeId());
					}
				}
				if (!object_indices.empty())
					addOpcode2(funccode, new OMarkTypeClass(new VectorArgument(object_indices)));
				funccode.push_back(std::shared_ptr<Opcode>(new ONoOp(function.getAltLabel())));
			}
			else if(puc == puc_destruct)
			{
				std::shared_ptr<Opcode> first(new ODestructor(new StringArgument(user_class.getName())));
				first->setLabel(function.getLabel());
				funccode.push_back(std::move(first));
			}
			else
			{
				std::shared_ptr<Opcode> first(new ONoOp(function.getLabel()));
				funccode.push_back(std::move(first));
			}
			// Push 0s for the local variables.
			for (int32_t i = stackSize - getParameterCount(function); i > 0; --i)
				addOpcode2(funccode, new OPushImmediate(new LiteralArgument(0)));
			
			// Set up the stack frame register
			addOpcode2(funccode, new OSetRegister(new VarArgument(SFRAME),
												new VarArgument(SP2)));
			if (puc == puc_construct)
				addOpcode2(funccode, new OPushRegister(new VarArgument(CLASS_THISKEY2)));
			CleanupVisitor cv(scope);
			node.execute(cv);
			OpcodeContext oc(typeStore);
			if (puc != puc_construct)
			{
				auto returnType = function.returnType;
				oc.returns_object = returnType && returnType->isObject();
			}
			BuildOpcodes bo(scope);
			bo.parsing_user_class = puc;
			node.execute(bo, &oc);
			
			if (bo.hasError()) failure = true;
			
			appendElements(funccode, bo.getResult());
			
			if(function.getFlag(FUNCFLAG_NEVER_RETURN))
			{
				if(funccode.size())
					funccode.back()->mergeComment("[Opt:NeverRet]");
			}
			else
			{
				// Pop off everything
				Opcode* next;
				if(stackSize)
					next = new OPopArgsRegister(new VarArgument(NUL),
						new LiteralArgument(stackSize));
				else next = new ONoOp();
				next->setLabel(bo.getReturnLabelID());
				addOpcode2(funccode, next);
				
				if (puc == puc_construct) //return val
				{
					addOpcode2(funccode, new OSetRegister(new VarArgument(EXP1), new VarArgument(CLASS_THISKEY)));
					addOpcode2(funccode, new OPopRegister(new VarArgument(CLASS_THISKEY)));
				}
				addOpcode2(funccode, new OReturnFunc());
			}
			function.giveCode(funccode);
		}
		else
		{
			vector<std::shared_ptr<Opcode>> funccode;
			
			int32_t stackSize = getStackSize(function);
			
			// Start of the function.
			funccode.push_back(std::shared_ptr<Opcode>(new ONoOp(function.getLabel())));
			
			// Push on the this, if a script
			if (isRun)
			{
				ParserScriptType type = program.getScript(scriptname)->getType();

				if (type == ParserScriptType::ffc )
				{
					addOpcode2(funccode, 
						new OPushRegister(new VarArgument(REFFFC)));
				}
				else if (type == ParserScriptType::item )
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFITEMCLASS)));
				}
				else if (type == ParserScriptType::npc )
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFNPC)));
				}
				else if (type == ParserScriptType::lweapon )
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFLWPN)));
				}
				else if (type == ParserScriptType::eweapon )
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFEWPN)));
				}
				else if (type == ParserScriptType::dmapdata )
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFDMAPDATA)));
				}
				else if (type == ParserScriptType::itemsprite)
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFITEM)));
				}
				else if (type == ParserScriptType::subscreendata)
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFSUBSCREEN)));
				}
				else if (type == ParserScriptType::combodata)
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFCOMBODATA)));
				}
				else if (type == ParserScriptType::genericscr)
				{
					addOpcode2(funccode,
						new OPushRegister(new VarArgument(REFGENERICDATA)));
				}
				else addOpcode2(funccode, new OPushImmediate(new LiteralArgument(0)));
			}
			
			// Push 0s for the local variables.
			for (int32_t i = stackSize - getParameterCount(function); i > 0; --i)
				addOpcode2(funccode, new OPushImmediate(new LiteralArgument(0)));
			
			// Set up the stack frame register
			addOpcode2(funccode, new OSetRegister(new VarArgument(SFRAME),
												new VarArgument(SP2)));

			// Retain references from parameters that are objects.
			for (auto&& datum : function.getInternalScope()->getLocalData())
			{
				auto position = lookupStackPosition(*scope, *datum);
				assert(position);
				if (!position)
					continue;

				if (datum->type.isObject() && datum->getName() != "this")
				{
					addOpcode2(funccode, new OMarkTypeStack(new LiteralArgument(1), new LiteralArgument(*position)));
					addOpcode2(funccode, new ORefInc(new LiteralArgument(*position)));
				}
			}

			CleanupVisitor cv(scope);
			node.execute(cv);
			OpcodeContext oc(typeStore);
			auto returnType = function.returnType;
			oc.returns_object = returnType && returnType->isObject();
			BuildOpcodes bo(scope);
			node.execute(bo, &oc);

			if (bo.hasError()) failure = true;
			
			appendElements(funccode, bo.getResult());
			
			if(function.getFlag(FUNCFLAG_NEVER_RETURN))
			{
				if(funccode.size())
					funccode.back()->mergeComment("[Opt:NeverRet]");
			}
			else
			{
				// Add appendix code.
				funccode.push_back(std::shared_ptr<Opcode>(new ONoOp(bo.getReturnLabelID())));

				// Release references from parameters that are objects.
				for (auto&& datum : function.getInternalScope()->getLocalData())
				{
					auto position = lookupStackPosition(*scope, *datum);
					assert(position);
					if (datum->type.isObject() && position && datum->getName() != "this")
						addOpcode2(funccode, new ORefRemove(new LiteralArgument(*position)));
				}
				
				// Pop off everything.
				if(stackSize)
					addOpcode2(funccode, new OPopArgsRegister(new VarArgument(NUL),
						new LiteralArgument(stackSize)));
				
				//if it's a main script, quit.
				if (isRun)
					addOpcode2(funccode, new OQuit()); //exit the script
				else addOpcode2(funccode, new OReturnFunc());
			}
			
			function.giveCode(funccode);
		}
	}

	if (failure)
	{
		rval.reset();
		return unique_ptr<IntermediateData>(rval.release());;
	}

	//Z_message("yes");
	return unique_ptr<IntermediateData>(rval.release());
}

ScriptAssembler::ScriptAssembler(IntermediateData& id) : program(id.program),
	rval(), runlabels(), runlbl_ptrs(), ginit(id.globalsInit), assemble_err(false)
{}

void ScriptAssembler::assemble()
{
	assemble_init();
	assemble_scripts();
	gather_labels();
	link_functions();
	optimize_output();
	finalize_labels();
}

void ScriptAssembler::assemble_init()
{
	// Do the global inits
	// If there's a global script called "Init", append it to ~Init:
	Script* userInit = program.getScript("Init");
	if (userInit && (userInit->getType() != ParserScriptType::global || userInit->isPrototypeRun()))
		userInit = nullptr;
	
	map<int32_t,vector<Script*>> initScripts;
	
	if(userInit)
	{
		std::optional<int32_t> weight = userInit->getInitWeight();
		auto& vec = initScripts[weight ? *weight : 0];
		vec.push_back(userInit);
	}
	for (vector<Script*>::const_iterator it = program.scripts.begin();
	     it != program.scripts.end(); ++it)
	{
		Script& script = **it;
		if(script.getType() != ParserScriptType::global) continue;
		if(script.isPrototypeRun()) continue; //skippable
		if(std::optional<int32_t> weight = script.getInitWeight())
		{
			auto& vec = initScripts[*weight];
			vec.push_back(&script);
		}
	}
	vector<shared_ptr<Opcode>> ginit_mergefuncs;
	for(auto it = initScripts.begin(); it != initScripts.end(); ++it)
	{
		auto& vec = it->second;
		for(auto it = vec.begin(); it != vec.end(); ++it)
		{
			Script& script = **it;
			Function* run = script.getRun();
			vector<shared_ptr<Opcode>> const& runCode = run->getCode();
			
			//Function call the run function
			//push the stack frame pointer
			addOpcode2(ginit, new OPushRegister(new VarArgument(SFRAME)));
			
			int32_t funcaddr = ScriptParser::getUniqueLabelID();
			addOpcode2(ginit, new OCallFunc(new LabelArgument(funcaddr, true)));
			
			addOpcode2(ginit,new OPopRegister(new VarArgument(SFRAME)));
			
			//Add the function to the end of the script, as a special copy
			bool didlabel = false;
			size_t index = 0;
			for(auto it = runCode.begin(); it != runCode.end(); ++it, ++index)
			{
				Opcode* op = it->get();
				if(dynamic_cast<OQuit*>(op))
				{
					op = new OReturnFunc(); //Replace 'Quit();' with 'return;'
				}
				else
					op = op->makeClone(true);
				if(!didlabel)
				{
					op->setLabel(funcaddr);
					didlabel = true;
				}
				addOpcode2(ginit_mergefuncs, op);
			}
			Opcode* last = ginit_mergefuncs.back().get();
			if(OReturnFunc* opcode = dynamic_cast<OReturnFunc*>(last))
				; //function ends in a return already
			else
				addOpcode2(ginit_mergefuncs, new OReturnFunc());
		}
	}
	addOpcode2(ginit, new OQuit());
	ginit.insert(ginit.end(), ginit_mergefuncs.begin(), ginit_mergefuncs.end());
	Script* init = program.getScript("~Init");
	assemble_script(init, ginit, 0, "void run()");
}

void ScriptAssembler::assemble_script(Script* scr,
	vector<shared_ptr<Opcode>> runCode, int numparams, string const& runsig)
{
	// Push on the params to the run.
	auto script_start_indx = rval.size();
	int i = 0;
	for (; i < numparams && i < 9; ++i)
		addOpcode2(rval, new OPushRegister(new VarArgument(i)));
	for (; i < numparams; ++i)
		addOpcode2(rval, new OPushRegister(new VarArgument(EXP1)));
	if(rval.size() > script_start_indx)
		rval[script_start_indx]->setComment(fmt::format("{} Params",runsig));

	// Make the rval
	auto rv_sz = rval.size();
	for (vector<shared_ptr<Opcode>>::iterator it = runCode.begin();
	     it != runCode.end(); ++it)
		addOpcode2(rval, (*it)->makeClone());
	if(rval.size() == rv_sz+1)
		rval.back()->mergeComment(fmt::format("{} Body",runsig));
	else if(rval.size() > rv_sz)
	{
		rval[rv_sz]->mergeComment(fmt::format("{} Body Start",runsig));
		rval.back()->mergeComment(fmt::format("{} Body End",runsig));
	}
	
	Opcode* firstop = rval[script_start_indx].get();
	Opcode* lastop = rval.back().get();
	int startlbl = firstop->getLabel();
	if(startlbl < 0)
	{
		startlbl = ScriptParser::getUniqueLabelID();
		firstop->setLabel(startlbl);
	}
	int endlbl = lastop->getLabel();
	if(endlbl < 0)
	{
		endlbl = ScriptParser::getUniqueLabelID();
		lastop->setLabel(endlbl);
	}
	runlabels[scr] = {startlbl,endlbl};
}

void ScriptAssembler::assemble_scripts()
{
	for (vector<Script*>::const_iterator it = program.scripts.begin();
	     it != program.scripts.end(); ++it)
	{
		Script& script = **it;
		if(script.getName() == "~Init") continue; //init script
		if(script.getType() == ParserScriptType::global && (script.getName() == "Init" || script.getInitWeight()))
			continue; //init script
		if(script.getType() == ParserScriptType::untyped)
			continue; //untyped script has no body
		Function& run = *script.getRun();
		if(run.prototype)
			continue; //Skip if run is prototype
		int32_t numparams = script.getRun()->paramTypes.size();
		assemble_script(&script, run.getCode(), numparams, run.getUnaliasedSignature(true).asString());
	}
}

void ScriptAssembler::gather_labels()
{
	runlbl_ptrs.clear();
	for(auto& p : runlabels)
	{
		auto& lbls = p.second;
		runlbl_ptrs.push_back(&lbls.first);
		runlbl_ptrs.push_back(&lbls.second);
	}
}
void ScriptAssembler::link_functions()
{
	// Generate a map of labels to functions.
	vector<Function*> allFunctions = getFunctions(program);
	appendElements(allFunctions, program.getUserClassConstructors());
	appendElements(allFunctions, program.getUserClassDestructors());
	map<int32_t, Function*> functionsByLabel;
	for (vector<Function*>::iterator it = allFunctions.begin();
	     it != allFunctions.end(); ++it)
	{
		Function& function = **it;
		if(function.is_aliased())
			continue;
		if(function.isTemplateSkip())
		{
			for(auto& applied : function.get_applied_funcs())
				allFunctions.push_back(applied.get());
			continue;
		}
		functionsByLabel[function.getLabel()] = &function;
		if(function.getFlag(FUNCFLAG_CONSTRUCTOR))
			functionsByLabel[function.getAltLabel()] = &function;
	}

	// Grab all labels directly jumped to.
	std::set<int32_t> usedLabels;
	GetLabels getlabel(usedLabels);
	getlabel.execute(rval, nullptr);
	std::set<int32_t> unprocessedLabels(usedLabels);

	// Grab labels used by each function until we run out of functions.
	while (!unprocessedLabels.empty())
	{
		int32_t label = *unprocessedLabels.begin();
		Function* function = find<Function*>(functionsByLabel, label).value_or(nullptr);
		if (function)
		{
			vector<shared_ptr<Opcode>> const& functionCode = function->getCode();
			GetLabels temp(usedLabels);
			temp.execute(functionCode, nullptr);
			insertElements(unprocessedLabels, temp.newLabels);
		}

		unprocessedLabels.erase(label);
	}

	for (std::set<int32_t>::iterator it = usedLabels.begin();
	     it != usedLabels.end(); ++it)
	{
		int32_t label = *it;
		Function* function =
			find<Function*>(functionsByLabel, label).value_or(nullptr);
		if (!function) continue;

		vector<shared_ptr<Opcode>> functionCode = function->getCode();
		auto rv_sz = rval.size();
		for (vector<shared_ptr<Opcode>>::iterator it = functionCode.begin();
		     it != functionCode.end(); ++it)
			addOpcode2(rval, (*it)->makeClone());
		if(rval.size() == rv_sz+1)
			rval.back()->mergeComment(fmt::format("Func[{}] Body",function->getUnaliasedSignature(true).asString()));
		else if(rval.size() > rv_sz)
		{
			rval[rv_sz]->mergeComment(fmt::format("Func[{}] Body Start",function->getUnaliasedSignature(true).asString()));
			rval.back()->mergeComment(fmt::format("Func[{}] Body End",function->getUnaliasedSignature(true).asString()));
		}
	}
}

void ScriptAssembler::optimize_output()
{
	// Run automatic optimizations
	{
		{ //macros
			#define START_OPT_PASS() \
			zconsole_idle(); \
			for(auto it = rval.begin(); it != rval.end();) \
			{ \
				Opcode* ocode = it->get(); \
				auto lbl = ocode->getLabel(); \
				string comment = ocode->getComment();
			#define END_OPT_PASS() \
				++it; \
			}
			
			#define TRASH_OP(ty, is_nop, condfunc) \
			if(ty* op = dynamic_cast<ty*>(ocode)) \
			{ \
				std::function<bool()> proc = condfunc; \
				if(proc && !proc()) {++it; continue;} \
				auto it2 = it; \
				++it2; \
				Opcode* nextcode = it2 == rval.end() ? nullptr : it2->get(); \
				if(nextcode) \
					nextcode->mergeComment(comment, true); \
				if(lbl == -1) /*no label, just trash it*/ \
				{ \
					it = rval.erase(it); \
					continue; \
				} \
				if(!nextcode) \
				{ \
					if(!is_nop) \
					{ \
						ONoOp* nop = new ONoOp(lbl); \
						nop->setComment(comment); \
						it = rval.erase(it); \
						it = rval.insert(it,std::shared_ptr<Opcode>(nop)); \
					} \
					break; /*can't merge with something that doesn't exist*/ \
				} \
				auto lbl2 = nextcode->getLabel(); \
				if(lbl2 == -1) /*next code has no label, pass the label*/ \
				{ \
					nextcode->setLabel(lbl); \
					it = rval.erase(it); \
					continue; \
				} \
				/*Else merge the two labels!*/ \
				it = rval.erase(it); \
				MergeLabels::merge(lbl2, {lbl}, rval, nullptr, &runlbl_ptrs); \
				continue; \
			}
			#define MERGE_CONSEC_1(ty) \
			if(ty* op = dynamic_cast<ty*>(ocode)) \
			{ \
				auto it2 = it; \
				++it2; \
				if(it2 == rval.end()) \
					break; \
				if(ty* op2 = dynamic_cast<ty*>(it2->get())) \
				{ \
					if(!op->getArgument()->toString().compare( \
						op2->getArgument()->toString())) \
					{ \
						auto lbl2 = op2->getLabel(); \
						op2->mergeComment(comment, true); \
						if(lbl2 == -1 && lbl > -1) \
						{ \
							op2->setLabel(lbl); \
							it = rval.erase(it); \
							continue; \
						} \
						it = rval.erase(it); \
						if(lbl > -1) \
						{ \
							MergeLabels::merge(lbl2, {lbl}, rval, nullptr, &runlbl_ptrs); \
						} \
						continue; \
					} \
				} \
				++it; \
				continue; \
			}
			#define CONV_GOTO_CMP(ty, cmp) \
			if(ty* op = dynamic_cast<ty*>(ocode)) \
			{ \
				LabelArgument* label_arg = static_cast<LabelArgument*>(op->takeArgument()); \
				it = rval.erase(it); \
				OGotoCompare* newop = new OGotoCompare(label_arg,new CompareArgument(cmp)); \
				newop->setLabel(lbl); \
				newop->setComment(comment); \
				it = rval.insert(it,std::shared_ptr<Opcode>(newop)); \
				continue; \
			}
			#define MERGE_GOTO_NEXT(ty) \
			if(ty* op = dynamic_cast<ty*>(ocode)) \
			{ \
				auto it2 = it; \
				++it2; \
				if(it2 == rval.end()) \
					break; \
				LabelArgument* label_arg = (LabelArgument*)op->getArgument(); \
				Opcode* nextcode = it2->get(); \
				auto lbl2 = nextcode->getLabel(); \
				if(lbl2 > -1 && label_arg->getID() == lbl2) \
				{ \
					nextcode->mergeComment(comment, true); \
					it = rval.erase(it); \
					if(lbl > -1) \
					{ \
						MergeLabels::merge(lbl2, {lbl}, rval, nullptr, &runlbl_ptrs); \
					} \
					continue; \
				} \
				++it; \
				continue; \
			}
			#define MERGE_GOTO_NEXT2(ty) \
			if(ty* op = dynamic_cast<ty*>(ocode)) \
			{ \
				auto it2 = it; \
				++it2; \
				if(it2 == rval.end()) \
					break; \
				LabelArgument* label_arg = (LabelArgument*)op->getFirstArgument(); \
				Opcode* nextcode = it2->get(); \
				auto lbl2 = nextcode->getLabel(); \
				if(lbl2 > -1 && label_arg->getID() == lbl2) \
				{ \
					nextcode->mergeComment(comment, true); \
					it = rval.erase(it); \
					if(lbl > -1) \
					{ \
						MergeLabels::merge(lbl2, {lbl}, rval, nullptr, &runlbl_ptrs); \
					} \
					continue; \
				} \
				++it; \
				continue; \
			}
			#define MERGE_CONSEC_REPCOUNT_START(ty1,ty2) \
			{ \
				ty1* single_op = dynamic_cast<ty1*>(ocode); \
				ty2* multi_op = dynamic_cast<ty2*>(ocode); \
				if(single_op || multi_op) \
				{ \
					auto it2 = it; \
					++it2; \
					if(it2 == rval.end()) \
						break; \
					auto const target_arg = single_op \
						? (single_op->getArgument()) \
						: (multi_op->getFirstArgument()); \
					size_t addcount = 0; \
					while(it2 != rval.end()) \
					{ \
						Opcode* nextcode = it2->get(); \
						if(nextcode->getLabel() != -1) \
							break; /*can't combine*/ \
						ty1* single_next = dynamic_cast<ty1*>(nextcode); \
						ty2* multi_next = dynamic_cast<ty2*>(nextcode); \
						if(!(single_next || multi_next)) \
							break; /*can't combine*/ \
						if(*(Argument*)target_arg != *(Argument*)(single_next \
							? (single_next->getArgument()) \
							: (multi_next->getFirstArgument()))) \
							break; /*Different registers, can't combine*/ \
						if(multi_next) \
						{ \
							LiteralArgument const* larg = multi_next->getSecondArgument(); \
							addcount += larg->value; \
						} \
						else /*if single_next*/ \
							++addcount; \
						Opcode::mergeComment(comment, nextcode->getComment()); \
						it2 = rval.erase(it2); \
					}
			#define MERGE_CONSEC_REPCOUNT_END(ty1,ty2) \
					if(addcount) \
					{ \
						if(single_op) \
						{ \
							auto arg = single_op->takeArgument(); \
							it = rval.erase(it); \
							it = rval.insert(it,std::shared_ptr<Opcode>(new ty2(arg,new LiteralArgument(addcount+1)))); \
							(*it)->setLabel(lbl); \
							(*it)->setComment(comment); \
						} \
						else /*if multi_op*/ \
						{ \
							LiteralArgument* litarg = multi_op->getSecondArgument(); \
							litarg->value += addcount; \
							multi_op->setComment(comment); \
						} \
					} \
					else if(multi_op) \
					{ \
						LiteralArgument* litarg = multi_op->getSecondArgument(); \
						if(*litarg == 1) \
						{ \
							auto arg = multi_op->takeFirstArgument(); \
							it = rval.erase(it); \
							it = rval.insert(it,std::shared_ptr<Opcode>(new ty1(arg))); \
							(*it)->setLabel(lbl); \
							(*it)->setComment(comment); \
						} \
					} \
					++it; \
					continue; \
				} \
			}
			#define MERGE_CONSEC_REPCOUNT(ty1,ty2) \
			MERGE_CONSEC_REPCOUNT_START(ty1,ty2) \
			MERGE_CONSEC_REPCOUNT_END(ty1,ty2)
			
		} //macros
		START_OPT_PASS() //Trim NoOps
			TRASH_OP(ONoOp, true, nullptr)
		END_OPT_PASS()
		START_OPT_PASS()
			// Change [PEEKAT reg,0] to [PEEK reg]
			if(OPeekAtImmediate* peekop = dynamic_cast<OPeekAtImmediate*>(ocode))
			{
				LiteralArgument* litarg = peekop->getSecondArgument();
				if(!litarg->value)
				{
					VarArgument* arg = peekop->takeFirstArgument();
					it = rval.erase(it);
					it = rval.insert(it, std::shared_ptr<Opcode>(new OPeek(arg)));
					(*it)->setLabel(lbl);
					(*it)->setComment(comment);
				}
				++it;
				continue;
			}
			// If [STORE reg,lit] is followed by [LOAD reg,lit], the LOAD
			// can be deleted, as 'reg' already will contain the value to be loaded.
			if(OStoreDirect* stored = dynamic_cast<OStoreDirect*>(ocode))
			{
				Argument const* regarg = stored->getFirstArgument();
				Argument const* litarg = stored->getSecondArgument();
				auto it2 = it;
				++it2;
				if(OLoad* loadd = dynamic_cast<OLoad*>(it2->get()))
				{
					if(*regarg == *loadd->getFirstArgument()
						&& *litarg == *loadd->getSecondArgument()
						&& loadd->getLabel() < 0)
					{
						stored->mergeComment(loadd->getComment());
						rval.erase(it2);
						continue;
					}
				}
				++it;
				continue;
			}
		END_OPT_PASS()
		START_OPT_PASS()
			//Merge multiple consecutive identical pops/pushes
			MERGE_CONSEC_REPCOUNT_START(OPopRegister,OPopArgsRegister)
			{ // turn single-pop followed by single-push into peek
				size_t startcount = 1;
				if(multi_op)
				{
					LiteralArgument* litarg = multi_op->getSecondArgument();
					startcount = litarg->value;
				}
				if(addcount+startcount == 1)
				{
					Opcode* nextcode = it2->get();
					if(nextcode->getLabel() == -1)
					{
						if(OPushRegister* pusharg = dynamic_cast<OPushRegister*>(nextcode))
						{
							if(*target_arg == *pusharg->getArgument())
							{
								auto arg = pusharg->takeArgument();
								it2 = rval.erase(it2);
								it = rval.erase(it);
								it = rval.insert(it,std::shared_ptr<Opcode>(new OPeek(arg)));
								(*it)->setLabel(lbl);
								(*it)->setComment(comment);
								++it;
								continue;
							}
						}
					}
				}
			}
			MERGE_CONSEC_REPCOUNT_END(OPopRegister,OPopArgsRegister)
			MERGE_CONSEC_REPCOUNT(OPushRegister,OPushArgsRegister)
			MERGE_CONSEC_REPCOUNT(OPushImmediate,OPushArgsImmediate)
			MERGE_CONSEC_REPCOUNT(OPushVargR,OPushVargsR)
			MERGE_CONSEC_REPCOUNT(OPushVargV,OPushVargsV)
			// goto if never, can be trashed
			TRASH_OP(OGotoCompare, false, [&]()
				{
					auto cmp = op->getSecondArgument()->value;
					return !(cmp&CMP_FLAGS);
				})
			//Convert gotos to OGotoCompare
			CONV_GOTO_CMP(OGotoTrueImmediate, CMP_EQ)
			CONV_GOTO_CMP(OGotoFalseImmediate, CMP_NE)
			CONV_GOTO_CMP(OGotoMoreImmediate, CMP_GE)
			CONV_GOTO_CMP(OGotoLessImmediate, CMP_LE)
			//Merge consecutive identical gotos
			MERGE_CONSEC_1(OGotoImmediate)
			MERGE_CONSEC_1(OGotoRegister)
		END_OPT_PASS()
		START_OPT_PASS()
			if(OGotoCompare* op = dynamic_cast<OGotoCompare*>(ocode))
			{
				auto it2 = it;
				++it2;
				if(it2 == rval.end())
					break;
				CompareArgument* cmparg = static_cast<CompareArgument*>(op->getSecondArgument());
				cmparg->value &= ~CMP_SETI;
				auto cmp = cmparg->value;
				if(OGotoCompare* op2 = dynamic_cast<OGotoCompare*>(it2->get()))
				{
					if(!op->getFirstArgument()->toString().compare(
						op2->getFirstArgument()->toString()))
					{
						CompareArgument* cmparg2 = static_cast<CompareArgument*>(op2->getSecondArgument());
						if((cmparg2->value & CMP_BOOL) != (cmp & CMP_BOOL)) //differing bool-states are weird...
						{
							++it;
							continue;
						}
						cmparg2->value &= ~CMP_SETI;
						cmparg2->value |= cmp; //merge compare types
						auto lbl2 = op2->getLabel();
						op2->mergeComment(comment, true);
						if(lbl2 == -1 && lbl > -1)
						{
							op2->setLabel(lbl);
							it = rval.erase(it);
							continue;
						}
						it = rval.erase(it);
						if(lbl > -1)
						{
							MergeLabels::merge(lbl2, {lbl}, rval, nullptr, &runlbl_ptrs);
						}
						continue;
					}
				}
				if((cmp&CMP_FLAGS) == CMP_FLAGS)
				{
					LabelArgument* label_arg = static_cast<LabelArgument*>(op->takeFirstArgument());
					it = rval.erase(it);
					OGotoImmediate* newop = new OGotoImmediate(label_arg);
					newop->setLabel(lbl);
					newop->setComment(comment);
					it = rval.insert(it,std::shared_ptr<Opcode>(newop));
					continue;
				}
				++it;
				continue;
			}
		END_OPT_PASS()
		START_OPT_PASS()
			//Trim GOTOs that go to the line directly after them
			MERGE_GOTO_NEXT(OGotoImmediate)
			MERGE_GOTO_NEXT2(OGotoCompare)
			MERGE_GOTO_NEXT(OCallFunc)
		END_OPT_PASS()
		map<int,std::pair<int,int>> gotocmp_map;
		START_OPT_PASS()
			if(OGotoImmediate* op = dynamic_cast<OGotoImmediate*>(ocode))
			{
				if(lbl > -1) //redirect labels that jump to GOTOs
				{
					auto targ_lbl = static_cast<LabelArgument*>(op->getArgument())->getID();
					MergeLabels::merge(targ_lbl, {lbl}, rval, nullptr, &runlbl_ptrs);
					op->setLabel(-1);
				}
				++it;
				continue;
			}
			if(OGotoCompare* op = dynamic_cast<OGotoCompare*>(ocode))
			{
				if(lbl > -1) //store labels in map, for optimization in next pass
				{
					int targ_lbl = static_cast<LabelArgument*>(op->getFirstArgument())->getID();
					int cmp = static_cast<CompareArgument*>(op->getSecondArgument())->value & ~CMP_SETI;
					gotocmp_map[lbl] = {targ_lbl,cmp};
				}
				++it;
				continue;
			}
		END_OPT_PASS()
		START_OPT_PASS()
			//[X: GOTO N] [N: GOTOCMP x,cmp] -> [X: GOTOCMP x,cmp] [N: GOTOCMP x,cmp]
			if(OGotoImmediate* op = dynamic_cast<OGotoImmediate*>(ocode))
			{
				LabelArgument* lbl_arg = static_cast<LabelArgument*>(op->getArgument());
				auto it2 = gotocmp_map.find(lbl_arg->getID());
				if(it2 != gotocmp_map.end())
				{
					lbl_arg = static_cast<LabelArgument*>(op->takeArgument());
					lbl_arg->setID(it2->second.first);
					it = rval.erase(it);
					OGotoCompare* newop = new OGotoCompare(lbl_arg, new CompareArgument(it2->second.second));
					newop->setComment(comment);
					//lbl == -1 is guaranteed
					it = rval.insert(it,std::shared_ptr<Opcode>(newop));
					continue;
				}
				++it;
				continue;
			}
		END_OPT_PASS()
		START_OPT_PASS()
			//[N: GOTOCMP N+2,c] [N+1: GOTO x] -> [N: GOTOCMP x,INV(c)]
			if(OGotoCompare* op = dynamic_cast<OGotoCompare*>(ocode))
			{
				auto it2 = it;
				++it2;
				if(it2 == rval.end())
					break;
				auto it3 = it2;
				++it3;
				if(it3 == rval.end())
					break;
				CompareArgument* cmparg = static_cast<CompareArgument*>(op->getSecondArgument());
				if(OGotoImmediate* op2 = dynamic_cast<OGotoImmediate*>(it2->get()))
				{
					LabelArgument* mid_lbl_arg = static_cast<LabelArgument*>(op2->getArgument());
					auto it3_lbl = it3->get()->getLabel();
					LabelArgument* lblarg = static_cast<LabelArgument*>(op->getFirstArgument());
					if(it3_lbl == lblarg->getID())
					{
						lblarg->setID(mid_lbl_arg->getID());
						cmparg->value = INVERT_CMP(cmparg->value);
						op->mergeComment(op2->getComment());
						rval.erase(it2);
						++it;
						continue;
					}
				}
				++it;
				continue;
			}
			MERGE_CONSEC_1(OGotoImmediate) //Redo this here due to timing stuff -Em
		END_OPT_PASS()
		START_OPT_PASS()
			MERGE_GOTO_NEXT(OGotoImmediate) //Redo this here due to timing stuff -Em
			MERGE_GOTO_NEXT2(OGotoCompare) //Redo this here due to timing stuff -Em
			//OSetImmediate -> OTraceRegister ('Trace()' optimization)
			if(OSetImmediate* setop = dynamic_cast<OSetImmediate*>(ocode))
			{
				Argument const* regarg = setop->getFirstArgument();
				auto it2 = it;
				++it2;
				Opcode* nextcode = it2->get();
				if(OTraceRegister* traceop = dynamic_cast<OTraceRegister*>(nextcode))
				{
					if(traceop->getLabel() == -1 && *regarg == *traceop->getArgument())
					{
						auto arg = setop->takeSecondArgument();
						Opcode::mergeComment(comment, traceop->getComment());
						it2 = rval.erase(it2);
						it = rval.erase(it);
						it = rval.insert(it, std::shared_ptr<Opcode>(new OTraceImmediate(arg)));
						(*it)->setLabel(lbl);
						(*it)->setComment(comment);
						++it;
						continue;
					}
				}
			}
		END_OPT_PASS()
	}
}

void ScriptAssembler::finalize_labels()
{
	// Set the label line numbers.
	map<int32_t, int32_t> linenos;
	int32_t lineno = 1;

	for (auto it = rval.begin(); it != rval.end(); ++it)
	{
		if ((*it)->getLabel() != -1)
			linenos[(*it)->getLabel()] = lineno;
		lineno++;
	}

	// Now fill in those labels
	SetLabels setlabel;
	setlabel.execute(rval, &linenos);
	if (setlabel.err)
		assemble_err = true;
	
	// ...and for tracking the run functions
	for(auto& pair : runlabels)
	{
		auto& pcs = pair.second;
		pcs.first = SetLabels::check(pcs.first, linenos);
		if(pcs.first)
			--pcs.first; //stupid 1-indexing...
		pcs.second = SetLabels::check(pcs.second, linenos);
		if(pcs.second)
			--pcs.second; //stupid 1-indexing...
	}
}

std::pair<int32_t,bool> ScriptParser::parseLong(std::pair<string, string> parts, Scope* scope)
{
	// Not sure if this should really check for negative numbers;
	// in most contexts, that's checked beforehand. parts only
	// includes the minus if this is a constant. - Saf
	bool negative=false;
	std::pair<int32_t, bool> rval;
	rval.second=true;
	bool intOneLarger = *lookupOption(*scope, CompileOption::OPT_TRUE_INT_SIZE) != 0;

	if(parts.first.data()[0]=='-')
	{
		negative=true;
		parts.first = parts.first.substr(1);
	}

	if(parts.second.size() > 4)
	{
		rval.second = false;
		parts.second = parts.second.substr(0,4);
	}

	if(parts.first.size() > 6)
	{
		rval.second = false;
		parts.first = parts.first.substr(0,6);
	}

	int32_t firstpart = atoi(parts.first.c_str());
	if(intOneLarger) //MAX_INT should be 214748, but if that is the value, there should be no float component. -V
	{
		if(firstpart > 214748)
		{
			firstpart = 214748;
			rval.second = false;
		}
	}
	else if(firstpart > 214747)
	{
		firstpart = 214747;
		rval.second = false;
	}

	int32_t intval = ((int32_t)(firstpart))*10000;
	//add fractional part; tricky!
	int32_t fpart = 0;


	while(parts.second.length() < 4)
		parts.second += "0";

	for(uint32_t i = 0; i < 4; i++)
	{
		fpart *= 10;
		fpart += parts.second[i] - '0';
	}

	/*for(uint32_t i=0; i<4; i++)
	  {
	  fpart*=10;
	  char tmp[2];
	  tmp[0] = parts.second.at(i);
	  tmp[1] = 0;
	  fpart += atoi(tmp);
	  }*/

	if(intOneLarger && firstpart == 214748 && (negative ? fpart > 3648 : fpart > 3647))
	{
		fpart = negative ? 3648 : 3647;
		rval.second = false;
	}


	rval.first = intval + fpart;
	if(negative)
		rval.first = -rval.first;
	return rval;
}

ScriptsData::ScriptsData(ScriptAssembler& assembler)
{
	for(auto& pair : assembler.getLabelMap())
	{
		Script* script = pair.first;
		auto& pcs = pair.second;
		string const& name = script->getName();
		
		zasm_meta& meta = theScripts[name].meta;
		theScripts[name].pc = pcs.first;
		theScripts[name].end_pc = pcs.second;
		scriptTypes[name] = script->getType();
		
		meta = script->getMetadata();
		meta.script_type = script->getType().getTrueId();
		meta.script_name = name;
		meta.author = script->getAuthor();
		if(Function* run = script->getRun())
		{
			int32_t ind = 0;
			for(vector<shared_ptr<const string>>::const_iterator it = run->paramNames.begin();
				it != run->paramNames.end(); ++it)
			{
				meta.run_idens[ind] = (**it);
				if(!meta.initd[ind].size())
					meta.initd[ind] = meta.run_idens[ind];
				if (++ind > 7) break; //sanity check
			}
			ind = 0;
			for(vector<DataType const*>::const_iterator it = run->paramTypes.begin();
				it != run->paramTypes.end(); ++it)
			{
				std::optional<DataTypeId> id = assembler.program.getTypeStore().getTypeId(**it);
				meta.run_types[ind] = id ? *id : ZTID_VOID;
				int8_t ty = -1;
				if(id) switch(*id)
				{
					case ZTID_BOOL:
						ty = nswapBOOL;
						break;
					case ZTID_LONG:
						ty = nswapLDEC;
						break;
					case ZTID_FLOAT:
					case ZTID_UNTYPED:
						ty = nswapDEC;
						break;
				}
				if(meta.initd_type[ind] < 0)
					meta.initd_type[ind] = ty;
				if (++ind > 7) break; //sanity check
			}
		}
	}
	zasm = std::move(assembler.getCode());
}

