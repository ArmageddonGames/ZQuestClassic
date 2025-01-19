/**
 * Registration handler, to ensure that anything global can be referenced in any order
 * Begun: 26th June, 2019
 * Author: Emily
 */

#include "parser/ASTVisitors.h"
#include "parser/LibrarySymbols.h"
#include "parser/Types.h"
#include "parser/ZScript.h"
#include "parserDefs.h"
#include "RegistrationVisitor.h"
#include <cassert>
#include <sstream>
#include "Scope.h"
#include "CompileError.h"

#include "zasm/table.h"
#include "zc/ffscript.h"

using std::ostringstream;
using namespace ZScript;
using std::shared_ptr;
using std::unique_ptr;

int32_t StringToVar(std::string var);

////////////////////////////////////////////////////////////////
// RegistrationVisitor

RegistrationVisitor::RegistrationVisitor(Program& program)
	: RecursiveVisitor(program), hasChanged(false)
{
	scope = &program.getScope();
	caseRoot(program.getRoot());
	assert(dynamic_cast<RootScope*>(scope)); //Ensure no scope corruption
}

void RegistrationVisitor::visit(AST& node, void* param)
{
	if(node.isDisabled()) return; //Don't visit disabled nodes.
	if(registered(node)) return; //Don't double-register
	RecursiveVisitor::visit(node, param);
}

template <class Container>
void RegistrationVisitor::block_regvisit_vec(Container const& nodes, void* param)
{
	for (auto it = nodes.cbegin();
		 it != nodes.cend(); ++it)
	{
		failure_temp = false;
		visit(**it, param);
		if(failure_halt) return;
	}
}

void RegistrationVisitor::caseDefault(AST& host, void* param)
{
	doRegister(host);
}

//Handle the root file specially!
void RegistrationVisitor::caseRoot(ASTFile& host, void* param)
{
	int32_t recursionLimit = REGISTRATION_REC_LIMIT;
	while(--recursionLimit)
	{
		caseFile(host, param);
		if(registered(host)) return;
		if(!hasChanged) return; //Nothing new was registered on this pass. Only errors remain.
		hasChanged = false;
	}
	//Failed recursionLimit
	handleError(CompileError::RegistrationRecursion(&host, REGISTRATION_REC_LIMIT));
}

void RegistrationVisitor::caseFile(ASTFile& host, void* param)
{
	ScopeReverter sr(&scope);
	if(host.scope)
		scope = host.scope;
	else
		scope = host.scope = scope->makeFileChild(host.asString());
	block_regvisit_vec(host.options, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.use, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.dataTypes, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.scriptTypes, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.imports, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.condimports, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.variables, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.functions, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.namespaces, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.scripts, param);
	if (breakRecursion(host, param)) return;
	block_regvisit_vec(host.classes, param);
	if(registered_vec(host.options) && registered_vec(host.use) && registered_vec(host.dataTypes)
		&& registered_vec(host.scriptTypes) && registered_vec(host.imports) && registered_vec(host.variables)
		&& registered_vec(host.functions) && registered_vec(host.namespaces) && registered_vec(host.scripts)
		&& registered_vec(host.condimports) && registered_vec(host.classes))
	{
		doRegister(host);
	}
}

void RegistrationVisitor::caseSetOption(ASTSetOption& host, void* param)
{
	visit(host.expr.get(), param);
	if(!registered(host.expr.get())) return; //Non-initialized constant
	
	// If the option name is "default", set the default option instead.
	if (host.name == "default")
	{
		CompileOptionSetting setting = host.getSetting(this, scope);
		if (!setting) return; // error
		scope->setDefaultOption(setting);
		return;
	}
	
	// Make sure the option is valid.
	if (!host.option.isValid())
	{
		handleError(CompileError::UnknownOption(&host, host.name));
		doRegister(host);
		return;
	}

	// Set the option to the provided value.
	CompileOptionSetting setting = host.getSetting(this, scope);
	if (!setting) return; // error
	scope->setOption(host.option, setting);
	doRegister(host);
}

// Declarations
void RegistrationVisitor::caseScript(ASTScript& host, void* param)
{
	visit(host.type.get());
	if(!registered(host.type.get())) return;
	
	Script& script = host.script ? *host.script : *(host.script = program.addScript(host, *scope, this));
	if (breakRecursion(host)) return;

	// Recurse on script elements with its scope.
	{
		ScopeReverter sr(&scope);
		scope = &script.getScope();

		block_regvisit_vec(host.options, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.use, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.types, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.variables, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.functions, param);
		if (breakRecursion(host)) return;
	}

	if(!(registered_vec(host.options) && registered_vec(host.use) && registered_vec(host.types)
		&& registered_vec(host.variables) && registered_vec(host.functions)))
	{
		return;
	}
	
	if(script.getType() == ParserScriptType::untyped)
	{
		doRegister(host);
		return;
	}
	//
	// Check for a valid run function.
	vector<Function*> possibleRuns = script.getScope().getLocalFunctions("run");
	if (possibleRuns.size() == 0)
	{
		return; //Don't register
	}
	doRegister(host);
	string name = script.getName();
	if (possibleRuns.size() > 1)
	{
		handleError(CompileError::TooManyRun(&host, name, "run"));
		if (breakRecursion(host)) return;
	}
	if (*possibleRuns[0]->returnType != DataType::ZVOID)
	{
		handleError(CompileError::ScriptRunNotVoid(&host, name, "run"));
		if (breakRecursion(host)) return;
	}
	script.setRun(possibleRuns[0]);
}

void RegistrationVisitor::caseClass(ASTClass& host, void* param)
{
	auto parsed_comment = host.getParsedDocComment();
	UserClass* parent_class = nullptr;
	if (parsed_comment.contains("extends"))
	{
		std::string parent_class_name = parsed_comment["extends"];
		ASTExprIdentifier ident{};
		ident.components.push_back(parent_class_name);
		if (auto type = lookupDataType(*scope, ident, nullptr))
		{
			if (auto custom_type = dynamic_cast<const DataTypeCustom*>(type); custom_type)
				parent_class = custom_type->getUsrClass();
		}
		else
		{
			handleError(CompileError::BadInternal(&host, fmt::format("Unknown class \"{}\" in @extends", parent_class_name)));
		}
	}

	UserClass& user_class = host.user_class ? *host.user_class : *(host.user_class = program.addClass(host, parent_class ? parent_class->getScope() : *scope, this));
	// UserClass& user_class = host.user_class ? *host.user_class : *(host.user_class = program.addClass(host, *scope, this));
	if (parsed_comment.contains("zasm_ref"))
		user_class.internalRefVar = parsed_comment["zasm_ref"];
	if (parent_class)
		user_class.setParentClass(parent_class);
	if (breakRecursion(host))
	{
		doRegister(host);
		return;
	}
	string name = user_class.getName();
	
	if(!host.type)
	{
		//Don't allow use of a name that already exists
		unique_ptr<ASTExprIdentifier> temp(new ASTExprIdentifier(name, host.location));
		if(DataType const* existingType = lookupDataType(*scope, *temp, this, true))
		{
			handleError(
				CompileError::RedefDataType(&host, host.getName()));
			doRegister(host);
			return;
		}
		
		//Construct a new constant type
		DataTypeCustomConst* newConstType = new DataTypeCustomConst("const " + host.getName(), &user_class);
		//Construct the base type
		DataTypeCustom* newBaseType = new DataTypeCustom(host.getName(), newConstType, &user_class, newConstType->getCustomId());
		newBaseType->setSource(&host);
		newConstType->setSource(&host);
		
		//Set the type to the base type
		host.type.reset(new ASTDataType(newBaseType, host.location));
		
		DataType::addCustom(newBaseType);
		user_class.setType(newBaseType);
		
		//This call should never fail, because of the error check above.
		scope->addDataType(host.getName(), newBaseType, &host);
		if (breakRecursion(*host.type.get())) return;
		
		for (auto it = host.constructors.begin();
			 it != host.constructors.end(); ++it)
		{
			ASTFuncDecl* func = *it;
			func->returnType.reset(new ASTDataType(newBaseType, func->location));
		}
	}

	// Recurse on user_class elements with its scope.
	{
		ScopeReverter sr(&scope);
		scope = &user_class.getScope();

		block_regvisit_vec(host.options, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.use, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.types, param);
		if (breakRecursion(host, param)) return;
		parsing_user_class = puc_vars;
		block_regvisit_vec(host.variables, param);
		parsing_user_class = puc_none;
		if (breakRecursion(host, param)) return;
		parsing_user_class = puc_funcs;
		block_regvisit_vec(host.functions, param);
		parsing_user_class = puc_none;
		if (breakRecursion(host, param)) return;
		parsing_user_class = puc_construct;
		block_regvisit_vec(host.constructors, param);
		parsing_user_class = puc_none;
		if (breakRecursion(host, param)) return;

		parsing_user_class = puc_destruct;
		visit(host.destructor.get(), param);
		parsing_user_class = puc_none;
	}

	if (breakRecursion(host)) return;
	//
	if(!(registered_vec(host.options) && registered_vec(host.use) && registered_vec(host.types)
		&& registered_vec(host.variables) && registered_vec(host.functions)
		&& registered_vec(host.constructors) && registered(host.destructor.get())))
	{
		return;
	}

	auto scope = &user_class.getScope();

	// 5 is NUL
	int refvar = user_class.internalRefVar.empty() ? 5 : StringToVar(user_class.internalRefVar);

	for (auto var : host.variables)
	{
		if (!var->internal)
			continue;

		for (auto decl : var->getDeclarations())
		{
			auto parsed_comment = var->getParsedDocComment();

			if (parsed_comment.contains("zasm_var") && parsed_comment.contains("zasm_internal_array"))
			{
				handleError(CompileError::BadInternal(decl, "Only one of @zasm_var or @zasm_internal_array is allowed"));
				continue;
			}

			if (!parsed_comment.contains("zasm_var") && !parsed_comment.contains("zasm_internal_array"))
			{
				handleError(CompileError::BadInternal(decl, "Expected one of @zasm_var or @zasm_internal_array"));
				continue;
			}

			int fn_value;
			if (parsed_comment.contains("zasm_var"))
			{
				if (auto sv = get_script_variable(parsed_comment["zasm_var"]))
				{
					fn_value = *sv;
				}
				else
				{
					handleError(CompileError::BadInternal(decl, fmt::format("Invalid ZASM register: {}", parsed_comment["zasm_var"])));
					continue;
				}
			}
			else
			{
				try {
					fn_value = std::stoi(parsed_comment["zasm_internal_array"]);
				} catch (std::exception ex) {
					handleError(CompileError::BadInternal(decl, fmt::format("Invalid internal array: {} (must be an integer)", parsed_comment["zasm_internal_array"])));
					continue;
				}

				fn_value = (INTARR_OFFS + fn_value) * 10000;
			}

			auto& ty = decl->manager->type;
			bool is_internal_arr = parsed_comment.contains("zasm_internal_array");
			bool is_arr = ty.isArray();
			auto var_type = is_internal_arr ? &ty : ty.baseType(*scope, nullptr);
			bool deprecated = parsed_comment.contains("deprecated");

			// Add a getter.
			{
				std::vector<const DataType*> params = {user_class.getType()};
				if (is_arr && !is_internal_arr)
					params.push_back(&DataType::FLOAT);

				Function* fn = scope->addGetter(var_type, decl->getName(), params, {}, 0);
				if (deprecated)
				{
					fn->setFlag(FUNCFLAG_DEPRECATED);
					fn->setInfo(parsed_comment["deprecated"]);
				}
				if (is_internal_arr)
					fn->setFlag(FUNCFLAG_INTARRAY);

				if (is_internal_arr)
					getConstant(refvar, fn, fn_value);
				else if (is_arr)
					getIndexedVariable(refvar, fn, fn_value);
				else
					getVariable(refvar, fn, fn_value);
			}

			if (is_internal_arr)
				continue;

			// Add a setter.
			{
				std::vector<const DataType*> params = {user_class.getType()};
				if (is_arr)
					params.push_back(&DataType::FLOAT);
				params.push_back(var_type);

				Function* fn = scope->addSetter(&DataType::ZVOID, decl->getName(), params, {}, 0);
				if (deprecated)
				{
					fn->setFlag(FUNCFLAG_DEPRECATED);
					fn->setInfo(parsed_comment["deprecated"]);
				}
				if (var->readonly)
					fn->setFlag(FUNCFLAG_READ_ONLY);

				if (is_arr)
					setIndexedVariable(refvar, fn, fn_value);
				else if (params.size() > 1 && params[1] == &DataType::BOOL)
					setBoolVariable(refvar, fn, fn_value);
				else
					setVariable(refvar, fn, fn_value);
			}
		}
	}

	for (auto fn_decl : host.functions)
	{
		if (!fn_decl->getFlag(FUNCFLAG_INTERNAL))
			continue;

		scope->initFunctionBinding(fn_decl->func, this);
		if (user_class.internalRefVar.empty())
			fn_decl->func->setIntFlag(IFUNCFLAG_SKIPPOINTER);
	}

	for (auto fn_decl : host.constructors)
	{
		if (!fn_decl->getFlag(FUNCFLAG_INTERNAL))
			continue;

		scope->initFunctionBinding(fn_decl->func, this);

		if (fn_decl->func->getFlag(FUNCFLAG_NIL))
		{
			scope->removeFunction(fn_decl->func);
			delete fn_decl->func;
			fn_decl->func = nullptr;
		}
	}

	doRegister(host);
}

void RegistrationVisitor::caseNamespace(ASTNamespace& host, void* param)
{
	Namespace* namesp_ = host.namesp ? host.namesp : ((host.namesp = program.addNamespace(host, *scope, this)));
	if (!namesp_) return;
	Namespace& namesp = *namesp_;
	if (breakRecursion(host)) return;

	// Recurse on script elements with its scope.
	{
		ScopeReverter sr(&scope);
		scope = &namesp.getScope();

		block_regvisit_vec(host.options, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.dataTypes, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.scriptTypes, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.use, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.variables, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.functions, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.namespaces, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.scripts, param);
		if (breakRecursion(host, param)) return;
		block_regvisit_vec(host.classes, param);
	}

	if(registered_vec(host.options) && registered_vec(host.use) && registered_vec(host.dataTypes)
		&& registered_vec(host.scriptTypes) && registered_vec(host.variables) && registered_vec(host.functions)
		&& registered_vec(host.namespaces) && registered_vec(host.scripts))
	{
		doRegister(host);
	}
}

void RegistrationVisitor::caseImportDecl(ASTImportDecl& host, void* param)
{
	//Check if the import is valid, or to be stopped by header guard. -V
	if(getRoot(*scope)->checkImport(&host, *lookupOption(*scope, CompileOption::OPT_HEADER_GUARD) / 10000.0, this))
	{
		visit(host.getTree(), param);
		if(registered(host.getTree())) doRegister(host);
	}
}
void RegistrationVisitor::caseIncludePath(ASTIncludePath& host, void* param)
{}

void RegistrationVisitor::caseImportCondDecl(ASTImportCondDecl& host, void* param)
{
	visit(*host.cond, param);
	if(!registered(*host.cond)) return; //Not registered yet
	std::optional<int32_t> val = host.cond->getCompileTimeValue(this, scope);
	if(val && (*val != 0))
	{
		if(!host.preprocessed)
		{
			ScriptParser::preprocess_one(*host.import, ScriptParser::recursionLimit);
			host.preprocessed = true;
		}
		visit(*host.import, param);
		if(!registered(*host.import)) return;
	}
	doRegister(host);
}

void RegistrationVisitor::caseUsing(ASTUsingDecl& host, void* param)
{
	//Handle adding scope
	ASTExprIdentifier* iden = host.getIdentifier();
	Scope* temp = host.always ? getRoot(*scope) : scope;
	int32_t numMatches = temp->useNamespace(iden->components, iden->delimiters, iden->noUsing);
	if(numMatches == 0) //Quit before registering; check again later
		return;
	doRegister(host);
	if(numMatches > 1)
		handleError(CompileError::TooManyUsing(&host, iden->asString()));
	else if(numMatches == -1)
		handleError(CompileError::DuplicateUsing(&host, iden->asString()));
}

void RegistrationVisitor::caseDataTypeDef(ASTDataTypeDef& host, void* param)
{
	visit(host.type.get());
	if (breakRecursion(*host.type.get())) return;
	if(!registered(host.type.get())) return;
	doRegister(host);
	// Add type to the current scope under its new name.
	DataType const& type = host.type->resolve(*scope, this);
	if(!scope->addDataType(host.name, &type, &host))
	{
		unique_ptr<ASTExprIdentifier> temp(new ASTExprIdentifier(host.name, host.location));
		DataType const* originalType = lookupDataType(*scope, *temp, this, true);
		if (breakRecursion(host) || !originalType || (*originalType != type))
			handleError(
				CompileError::RedefDataType(
					&host, host.name));
		temp.reset();
	}
}

void RegistrationVisitor::caseCustomDataTypeDef(ASTCustomDataTypeDef& host, void* param)
{
	if(!host.type)
	{
		//Don't allow use of a name that already exists
		unique_ptr<ASTExprIdentifier> temp(new ASTExprIdentifier(host.name, host.location));
		if(DataType const* existingType = lookupDataType(*scope, *temp, this, true))
		{
			handleError(
				CompileError::RedefDataType(
					&host, host.name));
			temp.reset();
			doRegister(host);
			return;
		}
		temp.reset();
		
		//Construct a new constant type
		DataTypeCustomConst* newConstType = new DataTypeCustomConst("const " + host.name);
		//Construct the base type
		DataTypeCustom* newBaseType = new DataTypeCustom(host.name, newConstType, nullptr, newConstType->getCustomId());
		newBaseType->setSource(&host);
		newConstType->setSource(&host);
		
		//Set the type to the base type
		host.type.reset(new ASTDataType(newBaseType, host.location));
		//Set the enum type to the const type
		host.definition->baseType.reset(new ASTDataType(newConstType, host.location));
		
		DataType::addCustom(newBaseType);
		
		//This call should never fail, because of the error check above.
		scope->addDataType(host.name, newBaseType, &host);
		if (breakRecursion(*host.type.get())) return;
	}
	visit(host.definition.get());
	if(registered(host.definition.get())) doRegister(host);
}

void RegistrationVisitor::caseScriptTypeDef(ASTScriptTypeDef& host, void* param)
{
	// Resolve the base type under current scope.
	ParserScriptType type = resolveScriptType(*host.oldType, *scope);
	if (!type.isValid()) return;

	doRegister(host);
	// Add type to the current scope under its new name.
	if (!scope->addScriptType(host.newName, type, &host))
	{
		ParserScriptType originalType = lookupScriptType(*scope, host.newName);
		if (originalType != type)
			handleError(
				CompileError::RedefScriptType(
					&host, host.newName, originalType.getName()));
		return;
	}
}

void RegistrationVisitor::caseDataDeclList(ASTDataDeclList& host, void* param)
{
	// Resolve the base type.
	DataType const* baseType = host.baseType->resolve_ornull(*scope, this);
    if (breakRecursion(*host.baseType.get())) return;
	if (!host.baseType->wasResolved() || !baseType) return;

	// Don't allow void type.
	if (baseType->isVoid())
	{
		handleError(CompileError::BadVarType(&host, host.asString(), baseType->getName()));
		doRegister(host);
		return;
	}
	
	if (baseType->isAuto() && host.getDeclarations().size() > 1)
	{
		handleError(CompileError::GroupAuto(&host));
		return;
	}
	
	// Check for disallowed global types.
	if (scope->isGlobal() && !baseType->canBeGlobal())
	{
		handleError(CompileError::RefVar(&host, baseType->getName()));
		doRegister(host);
		return;
	}

	// Recurse on list contents.
	visit_vec(host.getDeclarations());
	if (breakRecursion(host)) return;
	if(registered_vec(host.getDeclarations())) doRegister(host);
}

void RegistrationVisitor::caseDataEnum(ASTDataEnum& host, void* param)
{
	// Resolve the base type.
	DataType const* baseType = host.baseType->resolve_ornull(*scope, this);
    if (breakRecursion(*host.baseType.get())) return;
	if (!baseType) return;

	// Don't allow void/auto types.
	if (baseType->isVoid() || baseType->isAuto())
	{
		handleError(CompileError::BadVarType(&host, host.asString(), baseType->getName()));
		doRegister(host);
		return;
	}

	// Check for disallowed global types.
	if (scope->isGlobal() && !baseType->canBeGlobal())
	{
		handleError(CompileError::RefVar(&host, baseType->getName()));
		doRegister(host);
		return;
	}

	//Handle initializer assignment
	zfix value = 0;
	auto bitmode = host.getBitMode();
	switch(bitmode)
	{
		case ASTDataEnum::BIT_INT:
			value = 1;
			break;
		case ASTDataEnum::BIT_LONG:
			value = 0.0001_zf;
			break;
	}
	bool is_first = true;
	std::vector<ASTDataDecl*> decls = host.getDeclarations();
	for(vector<ASTDataDecl*>::iterator it = decls.begin();
		it != decls.end(); ++it)
	{
		ASTDataDecl* declaration = *it;
		if(ASTExpr* init = declaration->getInitializer())
		{
			visit(init);
			if(!registered(init)) return;
			if(std::optional<int32_t> v = init->getCompileTimeValue(this, scope))
			{
				value = zslongToFix(*v);
				// Should we WARN here if 'bitmode' is on? This could break the doubling increment....
				// Could maybe warn only if not assigned to an exact power of 2 (based on mode)?
			}
			else return;
		}
		else
		{
			if(!is_first)
			{
				if(host.increment_val)
					value += *host.increment_val;
				else if(bitmode)
					value *= 2;
				else if(baseType->isLong())
					value += 0.0001_zf;
				else value += 1;
			}
			ASTNumberLiteral* lit = new ASTNumberLiteral(new ASTFloat(value.getTrunc(), value.getZLongDPart(), host.location), host.location);
			declaration->setInitializer(lit);
		}
		is_first = false;
		visit(declaration, param);
		if(breakRecursion(host, param))
		{
			if(registered(declaration)) doRegister(host); //Decl errored, but registered; fatal error
			return;
		}
	}
	if(registered_vec(host.getDeclarations())) doRegister(host);
}

void RegistrationVisitor::caseDataDecl(ASTDataDecl& host, void* param)
{
	// First do standard recursing.
	RecursiveVisitor::caseDataDecl(host, paramRead);
	if (breakRecursion(host)) return;
	if(!(registered_vec(host.extraArrays) && (!host.getInitializer() || registered(host.getInitializer())))) return;
	// Then resolve the type.
	DataType const* type = host.resolve_ornull(scope, this);
	if (breakRecursion(host)) return;
	if (!type) return;
	
	doRegister(host);

	// Don't allow void type.
	if (type->isVoid())
	{
		handleError(CompileError::BadVarType(&host, host.getName(), type->getName()));
		return;
	}
	
	if (type->isAuto())
	{
		bool good = false;
		auto arr_depth = type->getArrayDepth();
		auto init = host.getInitializer();
		if(init)
		{
			auto readty = init->getReadType(scope, this);
			if(readty && readty->isResolved() && !readty->isVoid() && !readty->isAuto())
			{
				if(readty->getArrayDepth() < arr_depth)
				{
					handleError(CompileError::BadAutoType(&host, type->getName(), fmt::format("must have an initializer with type that is at least {}-depth array", arr_depth)));
					return;
				}
				type = type->isConstant() ? readty->getConstType() : readty;
				host.setResolvedType(*type);
				good = true;
			}
		}
		if(!good)
		{
			handleError(CompileError::BadAutoType(&host, type->getName(), "must have an initializer with valid type to mimic."));
			return;
		}
	}

	// Check for disallowed global types.
	if (scope->isGlobal() && !type->canBeGlobal())
	{
		handleError(CompileError::RefVar(
				            &host, type->getName() + " " + host.getName()));
		return;
	}

	// Is it a constant?
	bool isConstant = false;
	if (type->isConstant() && !host.list->internal)
	{
		// A constant without an initializer doesn't make sense (unless it is internal).
		if (!host.getInitializer())
		{
			handleError(CompileError::ConstUninitialized(&host));
			return;
		}

		// Inline the constant if possible.
		isConstant = host.getInitializer()->getCompileTimeValue(this, scope).has_value();
		//The dataType is constant, but the initializer is not. This is not allowed in Global or Script scopes, as it causes crashes. -V
		if(!isConstant && (scope->isGlobal() || scope->isScript() || scope->isClass()))
		{
			handleError(CompileError::ConstNotConstant(&host, host.getName()));
			return;
		}
	}
	else if(parsing_user_class == puc_vars) //class variables
	{
		if(host.getInitializer())
		{
			handleError(CompileError::ClassNoInits(&host, host.getName()));
			return;
		}
	}

	if (isConstant)
	{
		if (scope->getLocalDatum(host.getName()))
		{
			handleError(CompileError::VarRedef(&host, host.getName()));
			return;
		}
		
		int32_t value = *host.getInitializer()->getCompileTimeValue(this, scope);
		Constant::create(*scope, host, *type, value, this);
	}
	else
	{
		if(parsing_user_class == puc_vars)
		{
			UserClassVar::create(*scope, host, *type, this);
		}
		else
		{
			if (scope->getLocalDatum(host.getName()))
			{
				handleError(CompileError::VarRedef(&host, host.getName()));
				return;
			}

			if (host.list->internal)
			{
				// Every variable in bindings files should be read-only, so this is fine.
				Constant::create(*scope, host, *type, 0, this);
				return;
			}

			Variable::create(*scope, host, *type, this);
		}
	}
}

void RegistrationVisitor::caseDataDeclExtraArray(ASTDataDeclExtraArray& host, void* param)
{
	// Type Check size expressions.
	RecursiveVisitor::caseDataDeclExtraArray(host);
	if (breakRecursion(host)) return;
	if(!registered_vec(host.dimensions)) return;
	doRegister(host);
	
	// Iterate over sizes.
	for (vector<ASTExpr*>::const_iterator it = host.dimensions.begin();
		 it != host.dimensions.end(); ++it)
	{
		ASTExpr& size = **it;

		// Make sure each size can cast to float.
		if (!size.getReadType(scope, this)->canCastTo(DataType::FLOAT, scope))
		{
			handleError(CompileError::NonIntegerArraySize(&host));
			return;
		}

		// Make sure that the size is constant.
		if (!size.getCompileTimeValue(this, scope))
		{
			handleError(CompileError::ExprNotConstant(&host));
			return;
		}
		
		if(std::optional<int32_t> theSize = size.getCompileTimeValue(this, scope))
		{
			if(*theSize % 10000)
			{
				handleError(CompileError::ArrayDecimal(&host));
			}
			theSize = (*theSize / 10000);
			if(*theSize < 0 || *theSize > 214748)
			{
				handleError(CompileError::ArrayInvalidSize(&host));
				return;
			}
		}
	}
}

void RegistrationVisitor::caseFuncDecl(ASTFuncDecl& host, void* param)
{
	ScopeReverter sr(&scope);
	
	bool templated = !host.templates.empty();
	if(host.parentScope)
		scope = host.parentScope;
	else
	{
		if(host.identifier->components.size() > 1)
		{
			ASTExprIdentifier const& id = *(host.identifier);
			
			vector<string> scopeNames(id.components.begin(), --id.components.end());
			vector<string> scopeDelimiters(id.delimiters.begin(), id.delimiters.end());
			host.parentScope = lookupScope(*scope, scopeNames, scopeDelimiters, id.noUsing, host, this);
			if(!host.parentScope)
			{
				return;
			}
			scope = host.parentScope;
		}
		else host.parentScope = scope;
		// Add an extra anonymous scope, used by templates
		host.parentScope = scope = scope->makeChild();
	}
	Scope* extern_scope = scope;
	Scope* func_lives_in = scope->getParent();
	
	if(host.getFlag(FUNCFLAG_INVALID))
	{
		handleError(CompileError::BadFuncModifiers(&host, host.invalidMsg));
		return;
	}

	if (host.getFlag(FUNCFLAG_CONSTEXPR) && !host.getFlag(FUNCFLAG_INTERNAL))
	{
		host.invalidMsg += " `constexpr` is currently only allowed for internal function bindings.";
		handleError(CompileError::BadFuncModifiers(&host, host.invalidMsg));
		return;
	}

	/* This option is being disabled for now, as inlining of user functions is being disabled -V
	if(*lookupOption(*scope, CompileOption::OPT_FORCE_INLINE)
		&& !host.isRun())
	{
		host.setFlag(FUNCFLAG_INLINE);
	}*/
	
	if(templated && host.template_types.empty())
	{
		for(auto& ptr : host.templates)
		{
			string const& name = ptr->getValue();
			host.template_types.emplace_back(DataTypeTemplate::create(name));
			scope->addDataType(name, host.template_types.back().get(), nullptr);
		}
		host.param_template = host.parameters; //copy the pre-initialized params
	}
	
	// Resolve the return type under current scope.
	DataType const& returnType = host.returnType->resolve(*scope, this);
	if (breakRecursion(*host.returnType.get())) return;
	if (!returnType.isResolved()) return;
	if(returnType.isAuto())
	{
		handleError(CompileError::BadReturnType(&host, returnType.getName()));
		return;
	}

	// Gather the parameter types.
	vector<DataType const*> paramTypes;
	vector<ASTDataDecl*> const& params = host.parameters.data();
	vector<shared_ptr<const string>> paramNames;
	for (vector<ASTDataDecl*>::const_iterator it = params.begin();
		 it != params.end(); ++it)
	{
		ASTDataDecl& decl = **it;

		// Resolve the parameter type under current scope.
		DataType const* type = decl.resolve_ornull(scope, this);
		if (breakRecursion(decl)) return;
		if (!type) return;

		// Don't allow void/auto params.
		if (type->isVoid() || type->isAuto())
		{
			handleError(CompileError::FunctionBadParamType(&decl, decl.getName(), type->getName()));
			doRegister(host);
			return;
		}
		paramNames.emplace_back(new string(decl.getName()));
		paramTypes.push_back(type);
	}
	if(host.getFlag(FUNCFLAG_VARARGS) && !paramTypes.back()->isArray())
	{
		handleError(CompileError::BadVArgType(&host, paramTypes.back()->getName()));
		return;
	}
	if(host.prototype)
	{
		//Check the default return
		visit(host.defaultReturn.get(), param);
		if(breakRecursion(host.defaultReturn.get())) return;
		if(!(registered(host.defaultReturn.get()))) return;
		
		DataType const& defValType = *host.defaultReturn->getReadType(scope, this);
		if(!defValType.isResolved()) return;
		//Check type validity of default return
		if((*(host.defaultReturn->getCompileTimeValue(this, scope)) == 0) &&
			(defValType == DataType::CUNTYPED || defValType == DataType::UNTYPED))
		{
			//Default is null; don't check casting, as null needs to be valid even for things
			//that untyped does not normally cast to, such as VOID! -V
		}
		else checkCast(defValType, returnType, &host);
	}
	
	if(breakRecursion(host)) return;
	visit_vec(host.optparams, param);
	if(breakRecursion(host)) return;
	
	auto parcnt = paramTypes.size() - host.optparams.size();
	for(auto it = host.optparams.begin(); it != host.optparams.end() && parcnt < paramTypes.size(); ++it, ++parcnt)
	{
		DataType const* getType = (*it)->getReadType(scope, this);
		if(!getType) return;
		checkCast(*getType, *paramTypes[parcnt], &host);
		if(breakRecursion(host)) return;
		std::optional<int32_t> optVal = (*it)->getCompileTimeValue(this, scope);
		if (!optVal)
		{
			handleError(CompileError::Error(*it, fmt::format("Function '{}' has an optional parameter whose default value is not a constant expression", host.getName())));
			doRegister(host);
			return;
		}

		host.optvals.push_back(*optVal);
	}
	if(breakRecursion(host)) return;
	
	doRegister(host);
	
	// Add the function to the scope.
	Function* function = func_lives_in->addFunction(
			&returnType, host.getName(), paramTypes, paramNames, host.getFlags(), &host, this, extern_scope);
	host.func = function;
	
	if(breakRecursion(host)) return;
	// If adding it failed, it means this scope already has a function with
	// that name.
	if (function == NULL)
	{
		if(host.prototype) return; //Skip this error for prototype functions; error is handled inside 'addFunction()' above
		handleError(CompileError::FunctionRedef(&host, host.getName()));
		return;
	}

	function->node = &host;
}

// Expressions -- Needed for constant evaluation
void RegistrationVisitor::caseExprConst(ASTExprConst& host, void* param)
{
	RecursiveVisitor::caseExprConst(host, param);
	if (host.getCompileTimeValue(this, scope)) doRegister(host);
}

void RegistrationVisitor::caseVarInitializer(ASTExprVarInitializer& host, void* param)
{
	RecursiveVisitor::caseVarInitializer(host, param);
	if(registered(host.content.get()))
	{
		if(host.valueIsArray(scope, this)) doRegister(host);
		else
		{
			host.value = host.content->getCompileTimeValue(this, scope);
			if(host.value) doRegister(host);
		}
	}
}

void RegistrationVisitor::caseExprAssign(ASTExprAssign& host, void* param)
{
	visit(host.left.get(), paramWrite);
	if (breakRecursion(host)) return;
	visit(host.right.get(), paramRead);
	if (breakRecursion(host)) return;	
	if(!(registered(host.left.get()) && registered(host.right.get()))) return;
	doRegister(host);
	DataType const* ltype = host.left->getWriteType(scope, this);
	if (!ltype)
	{
		handleError(
			CompileError::NoWriteType(
				host.left.get(), host.left->asString()));
		return;
	}
	if (ltype->isConstant())
		handleError(CompileError::LValConst(&host, host.left->asString()));
}

void RegistrationVisitor::caseExprIdentifier(ASTExprIdentifier& host, void* param)
{
	// Bind to named variable.
	host.binding = lookupDatum(*scope, host, this);
	if (!host.binding) return;
	doRegister(host);
	if(host.binding->type.isArray())
	{
		handleError(CompileError::NoArrayGlobalVar(&host));
		return;
	}
}

void RegistrationVisitor::caseExprArrow(ASTExprArrow& host, void* param)
{
	//Doesn't get hit at registration time?
	doRegister(host);
}

void RegistrationVisitor::caseExprIndex(ASTExprIndex& host, void* param)
{
	visit(host.array.get());
	if (breakRecursion(host)) return;
	visit(host.index.get());
	if (breakRecursion(host)) return;
	if(registered(host.array.get()) && registered(host.index.get())) doRegister(host);
}

void RegistrationVisitor::caseExprCall(ASTExprCall& host, void* param)
{
	// Cast left.
	ASTExprArrow* arrow = NULL;
	if (host.left->isTypeArrow())
	{
		arrow = static_cast<ASTExprArrow*>(host.left.get());
		arrow->iscall = true;
	}
	ASTExprIdentifier* identifier = NULL;
	if (host.left->isTypeIdentifier())
		identifier = static_cast<ASTExprIdentifier*>(host.left.get());
	
	bool r = true;
	// Don't visit left for identifier, since we don't want to bind to a
	// variable.
	if (!identifier)
	{
		visit(host.left.get(), paramNone);
		if (breakRecursion(host)) return;
		r = registered(host.left.get());
	}

	visit_vec(host.parameters);
	if (breakRecursion(host)) return;
	if(!(r && registered_vec(host.parameters)))
		return; //can't resolve yet

	UserClass* user_class = nullptr;
	// Gather parameter types.
	vector<DataType const*> parameterTypes;
	if (arrow)
	{
		DataType const* arrtype = arrow->left->getReadType(scope, this);
		if(!arrtype)
			return;
		if((user_class = arrtype->getUsrClass()))
			;
		else parameterTypes.push_back(arrtype);
	}
	for (vector<ASTExpr*>::const_iterator it = host.parameters.begin();
		 it != host.parameters.end(); ++it)
		parameterTypes.push_back((*it)->getReadType(scope, this));

	// Grab functions with the proper name, and matching parameter types
	vector<Function*> functions;
	if(identifier)
	{
		if(host.isConstructor())
		{
			user_class = lookupClass(*scope, identifier->components, identifier->delimiters, identifier->noUsing);
			if(!user_class)
			{
				handleError(CompileError::NoClass(&host, identifier->asString()));
				return;
			}
			functions = lookupConstructors(*user_class, parameterTypes, scope);
		}
		else
		{
			if(identifier->components.size() == 1 && parsing_user_class > puc_vars)
			{
				user_class = &scope->getClass()->user_class;
				if(parsing_user_class == puc_construct && identifier->components[0] == user_class->getName())
					functions = lookupConstructors(*user_class, parameterTypes, scope);
				if(!functions.size())
					functions = lookupFunctions(*scope, identifier->components[0], parameterTypes, identifier->noUsing, true);
			}
			if(!functions.size())
				functions = lookupFunctions(*scope, identifier->components, identifier->delimiters, parameterTypes, identifier->noUsing);
		}
	}
	else if(user_class)
	{
		functions = lookupClassFuncs(*user_class, arrow->right->getValue(), parameterTypes, scope);
	}
	else functions = lookupFunctions(arrow->leftClass->getScope(), arrow->right->getValue(), parameterTypes, true, false, false, scope); //Never `using` arrow functions

	// Find function with least number of casts.
	vector<Function*> bestFunctions;
	int32_t bestCastCount = parameterTypes.size() + 1;
	for (vector<Function*>::iterator it = functions.begin();
		 it != functions.end(); ++it)
	{
		// Count number of casts.
		Function& function = **it;
		int32_t castCount = 0;
		size_t lowsize = zc_min(parameterTypes.size(), function.paramTypes.size());
		for(size_t i = 0; i < lowsize; ++i)
		{
			DataType const& from = getNaiveType(*parameterTypes[i], scope);
			DataType const& to = getNaiveType(*function.paramTypes[i], scope);
			if (from == to) continue;
			++castCount;
		}

		// If this beats the record, clear results and keep it.
		if (castCount < bestCastCount)
		{
			bestFunctions.clear();
			bestFunctions.push_back(&function);
			bestCastCount = castCount;
		}

		// If this just matches the record, append it.
		else if (castCount == bestCastCount)
			bestFunctions.push_back(&function);
	}
	// We may have failed, but let's check optional parameters first...
	if(bestFunctions.size() > 1)
	{
		auto targSize = parameterTypes.size();
		int32_t bestDiff = -1;
		//Find the best (minimum) difference between the passed param count and function max param count
		for(auto it = bestFunctions.begin(); it != bestFunctions.end(); ++it)
		{
			int32_t maxSize = (*it)->paramTypes.size();
			int32_t diff = maxSize - targSize;
			if(bestDiff < 0 || diff < bestDiff) bestDiff = diff;
		}
		//Remove any functions that don't share the minimum difference.
		for(auto it = bestFunctions.begin(); it != bestFunctions.end();)
		{
			int32_t maxSize = (*it)->paramTypes.size();
			int32_t diff = maxSize - targSize;
			if(diff > bestDiff)
				it = bestFunctions.erase(it);
			else ++it;
		}
	}
	// We may have failed, though namespaces may resolve the issue. Check for namespace closeness.
	if(bestFunctions.size() > 1)
	{
		std::map<Function*, Scope*> bestNSs;
		std::map<Function*, Scope*> bestScripts;
		for (vector<Function*>::const_iterator it = bestFunctions.begin();
		     it != bestFunctions.end(); ++it)
		{
			Scope* ns = NULL;
			Scope* scr = NULL;
			for(Scope* current = (*it)->getInternalScope(); current; current = current->getParent())
			{
				if(!scr && current->isScript())
				{
					scr = current;
				}
				if(current->isNamespace())
				{
					ns = current;
					break;
				}
			}
			bestNSs[*it] = ns;
			bestScripts[*it] = scr;
		}
		Function* bestFound = NULL;
		for(Scope* current = scope; current; current = current->getParent())
		{
			if(current->isScript())
			{
				for (vector<Function*>::const_iterator it = bestFunctions.begin();
				     it != bestFunctions.end(); ++it)
				{
					if(current == bestScripts[*it])
					{
						if(bestFound)
						{
							bestFound = NULL;
							current = NULL;
							break;
						}
						else bestFound = *it;
					}
				}
			}
			else if(current->isNamespace())
			{
				for (vector<Function*>::const_iterator it = bestFunctions.begin();
				     it != bestFunctions.end(); ++it)
				{
					if(current == bestNSs[*it])
					{
						if(bestFound)
						{
							bestFound = NULL;
							current = NULL;
							break;
						}
						else bestFound = *it;
					}
				}
			}
			if(!current) break;
		}
		if(bestFound) //Found a singular best; override the prior calculations, and salvage the call! -V
		{
			bestFunctions.clear();
			bestFunctions.push_back(bestFound);
		}
	}
	// We may have failed, but give higher priority to 'untyped' first...
	if(bestFunctions.size() > 1)
	{
		vector<Function*> newBestFunctions = bestFunctions;
		size_t maxsize = 0;
		for(auto it = newBestFunctions.begin(); it != newBestFunctions.end(); ++it)
		{
			if(maxsize < (*it)->paramTypes.size())
				maxsize = (*it)->paramTypes.size();
		}
		//Remove any strictly-less-specific functions
		for(size_t p = 0; p < maxsize; ++p)
		{
			int flag = 0;
			for(auto it = bestFunctions.begin(); flag != 3 && it != bestFunctions.end();++it)
			{
				auto& pty = (*it)->paramTypes;
				if (pty.size() <= p)
					continue;
				bool ut = pty.at(p)->isUntyped();
				if(ut) flag |= 1;
				else flag |= 2;
			}
			if(flag != 3) continue;
			for(auto it = newBestFunctions.begin(); it != newBestFunctions.end();)
			{
				auto& pty = (*it)->paramTypes;
				if (pty.size() <= p)
				{
					++it;
					continue;
				}
				bool ut = pty.at(p)->isUntyped();
				if (ut) //untyped, keep
				{
					++it;
					continue;
				}
				else if(parameterTypes.size() > p)
				{
					DataType const& from = getNaiveType(*parameterTypes[p], scope);
					DataType const& to = getNaiveType(*pty[p], scope);
					if(from == to) //Exact match, keep
					{
						++it;
						continue;
					}
				}
				//Not exact match, not untyped; junk it.
				it = newBestFunctions.erase(it);
				continue;
			}
			if(newBestFunctions.size() == 0)
				break; //Nothing left to loop on
		}
		if(newBestFunctions.size() > 0) //Don't overwrite if eliminated all
			bestFunctions = newBestFunctions;
	}
	// We failed.
	if (bestFunctions.size() != 1)
	{
		FunctionSignature signature(host.left->asString(), parameterTypes);
		if (bestFunctions.size() == 0)
		{
			handleError(
					CompileError::NoFuncMatch(&host, signature.asString()));
		}
		else
		{
			// Sort to keep order same across platforms.
			std::sort(bestFunctions.begin(), bestFunctions.end(), [](Function* a, Function* b) {
				return a->id < b->id;
			});

			// Build list of function signatures.
			ostringstream oss;
			for (vector<Function*>::const_iterator it = bestFunctions.begin();
			     it != bestFunctions.end(); ++it)
			{
				oss << "        ";
				string namespacenames = "";
				for(Scope* current = (*it)->getInternalScope(); current; current = current->getParent())
				{
					if(!current->isNamespace()) continue;
					NamespaceScope* ns = static_cast<NamespaceScope*>(current);
					namespacenames = ns->namesp->getName() + "::" + namespacenames;
				}
				oss << namespacenames << (*it)->getSignature().asString() << "\n";
			}
			
			handleError(
					CompileError::TooFuncMatch(
							&host,
							signature.asString(),
							oss.str()));
		}
		return;
	}
	
	// Is this a call to a disabled tracing function?
	if (*lookupOption(*scope, CompileOption::OPT_NO_LOGGING)
	    && bestFunctions.front()->isTracing())
	{
		host.disable();
		return;
	}
	
	host.binding = bestFunctions.front();
	deprecWarn(host.binding, &host, "Function", host.binding->getUnaliasedSignature().asString());
	if(host.binding->getFlag(FUNCFLAG_READ_ONLY))
		handleError(CompileError::ReadOnly(&host, host.binding->getUnaliasedSignature().asString()));
	
	if(!host.binding->get_constexpr())
		handleError(CompileError::GlobalVarFuncCall(&host));
	doRegister(host);
}

void RegistrationVisitor::caseExprNegate(ASTExprNegate& host, void* param)
{
	if(!host.done)
	{
		if(ASTNumberLiteral* lit = dynamic_cast<ASTNumberLiteral*>(host.operand.get()))
		{
			if(lit->value.get())
			{
				lit->negate();
				host.done = true;
			}
		}
	}
	analyzeUnaryExpr(host);
}
void RegistrationVisitor::caseExprDelete(ASTExprDelete& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprNot(ASTExprNot& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprBitNot(ASTExprBitNot& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprIncrement(ASTExprIncrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprDecrement(ASTExprDecrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprCast(ASTExprCast& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprAnd(ASTExprAnd& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprOr(ASTExprOr& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprGT(ASTExprGT& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprGE(ASTExprGE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLT(ASTExprLT& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLE(ASTExprLE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprEQ(ASTExprEQ& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprNE(ASTExprNE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprAppxEQ(ASTExprAppxEQ& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprPlus(ASTExprPlus& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprMinus(ASTExprMinus& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprTimes(ASTExprTimes& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprExpn(ASTExprExpn& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprDivide(ASTExprDivide& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprModulo(ASTExprModulo& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitAnd(ASTExprBitAnd& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitOr(ASTExprBitOr& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitXor(ASTExprBitXor& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLShift(ASTExprLShift& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprRShift(ASTExprRShift& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprTernary(ASTTernaryExpr& host, void* param)
{
	visit(host.left.get());
	if (breakRecursion(host)) return;
	visit(host.middle.get());
	if (breakRecursion(host)) return;
	visit(host.right.get());
	if (breakRecursion(host)) return;
	if(registered(host.left.get()) && registered(host.middle.get()) && registered(host.right.get())) doRegister(host);
}

//Types
void RegistrationVisitor::caseScriptType(ASTScriptType& host, void* param)
{
	ParserScriptType const& type = resolveScriptType(host, *scope);
	if(type.isValid()) doRegister(host);
}

void RegistrationVisitor::caseDataType(ASTDataType& host, void* param)
{
	DataType const& type = host.resolve(*scope, this);
	if(type.isResolved()) doRegister(host);
}

//Literals
void RegistrationVisitor::caseArrayLiteral(ASTArrayLiteral& host, void* param)
{
	RecursiveVisitor::caseArrayLiteral(host, param);
	if(registered(host.type.get()) && registered(host.size.get()) && registered_vec(host.elements))
		doRegister(host);
}

void RegistrationVisitor::caseStringLiteral(ASTStringLiteral& host, void* param)
{
	// Add to scope as a managed literal.
	Literal::create(*scope, host, *host.getReadType(scope, this), this);
	doRegister(host);
}

//Helper Functions
void RegistrationVisitor::analyzeUnaryExpr(ASTUnaryExpr& host)
{
	visit(host.operand.get());
	if (breakRecursion(host)) return;
	if(registered(host.operand.get()))doRegister(host);
}

void RegistrationVisitor::analyzeBinaryExpr(ASTBinaryExpr& host)
{
	visit(host.left.get());
	if (breakRecursion(host)) return;
	visit(host.right.get());
	if (breakRecursion(host)) return;
	if((registered(host.left.get()) && registered(host.right.get()))) doRegister(host);
}

bool RegistrationVisitor::registered(AST& node) const
{
	return node.registered();
}

bool RegistrationVisitor::registered(AST* node) const
{
	if(node) return registered(*node);
	return true;
}

template <class Container>
bool RegistrationVisitor::registered_vec(Container const& nodes) const
{
	for(auto it = nodes.cbegin(); it != nodes.cend(); ++it)
	{
		if(!registered(*it)) return false;
	}
	return true;
}


