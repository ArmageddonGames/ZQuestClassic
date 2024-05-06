#ifndef ZSCRIPT_ZSCRIPT_H
#define ZSCRIPT_ZSCRIPT_H

#include <vector>
#include <map>
#include "AST.h"
#include "base/containers.h"
#include "CompilerUtils.h"
#include "Types.h"
#include "base/general.h"
#include "parser/parserDefs.h"

namespace ZScript
{
	class CompileErrorHandler;

	class TypeStore;
	class Program;
	class Script;
	class UserClass;
	class Namespace;
	class Variable;
	class BuiltinVariable;
	class Function;
	class Scope;
	class RootScope;
	class FileScope;
	class ScriptScope;
	class NamespaceScope;
	class FunctionScope;
	
	////////////////////////////////////////////////////////////////
	// Program
	
	class Program : private NoCopy
	{
	public:
		Program(ASTFile&, CompileErrorHandler*);
		~Program();

		ASTFile& getRoot() {return root_;}
		TypeStore const& getTypeStore() const {return typeStore_;}
		TypeStore& getTypeStore() {return typeStore_;}
		RootScope& getScope() const {return *rootScope_;}

		std::vector<Script*> scripts;
		std::vector<UserClass*> classes;
		std::vector<Namespace*> namespaces;
		Script* getScript(std::string const& name) const;
		Script* getScript(ASTScript* node) const;
		Script* addScript(ASTScript& node, Scope& parentScope, CompileErrorHandler* handler);
		UserClass* getClass(std::string const& name) const;
		UserClass* getClass(ASTClass* node) const;
		UserClass* addClass(ASTClass& node, Scope& parentScope, CompileErrorHandler* handler);
		Namespace* addNamespace(
			ASTNamespace& node, Scope& parentScope, CompileErrorHandler* handler);

		// Gets the non-internal (user-defined) global scope functions.
		std::vector<Function*> getUserGlobalFunctions() const;

		// Gets all user-defined functions.
		std::vector<Function*> getUserFunctions() const;
		std::vector<Function*> getUserClassConstructors() const;
		std::vector<Function*> getUserClassDestructors() const;

		// Return a list of all errors in the script declaration.
		std::vector<CompileError const*> getErrors() const;
		// Does this script have a declaration error?
		bool hasError() const {return (getErrors().size()>0);}

	private:
		std::map<std::string, Script*> scriptsByName_;
		std::map<ASTScript*, Script*> scriptsByNode_;
		std::map<std::string, UserClass*> classesByName_;
		std::map<ASTClass*, UserClass*> classesByNode_;
		
		TypeStore typeStore_;
		RootScope* rootScope_;
		ASTFile& root_;
	};

	// Gets all defined functions.
	std::vector<Function*> getFunctions(Program const&);

	////////////////////////////////////////////////////////////////
	// Script

	class UserScript;
	class BuiltinScript;
	
	class Script
	{
	public:
		virtual ~Script();

		virtual ParserScriptType getType() const = 0;
		virtual void setName(std::string const& newname) = 0;
		virtual std::string const& getName() const = 0;
		virtual std::string const& getAuthor() const = 0;
		virtual zasm_meta const& getMetadata() const = 0;
		virtual ASTScript* getNode() const = 0;
		virtual ScriptScope& getScope() = 0;
		virtual ScriptScope const& getScope() const = 0;
		virtual std::optional<int32_t> getInitWeight() const {return std::nullopt;}
		
		void setRun(Function* func) {runFunc = func;}
		Function* getRun() const {return runFunc;}
		
		bool isPrototypeRun() const;

	protected:
		Script(Program& program);

	private:
		Function* runFunc;
		Program& program;
	};

	class UserScript : public Script
	{
		friend UserScript* createScript(
				Program&, Scope&, ASTScript&, CompileErrorHandler*);

	public:
		ParserScriptType getType() const /*override*/;
		void setName(std::string const& newname) /*override*/ {node.metadata.script_name = newname;};
		std::string const& getName() const /*override*/ {return node.metadata.script_name;};
		std::string const& getAuthor() const /*override*/ {return node.metadata.author;};
		zasm_meta const& getMetadata() const /*override*/ {return node.metadata;};
		ASTScript* getNode() const /*override*/ {return &node;};
		ScriptScope& getScope() /*override*/ {return *scope;}
		ScriptScope const& getScope() const /*override*/ {return *scope;}
		std::optional<int32_t> getInitWeight() const /*override*/ {return node.init_weight;}

	private:
		UserScript(Program&, ASTScript&);

		ASTScript& node;
		ScriptScope* scope;
	};

	class BuiltinScript : public Script
	{
		friend BuiltinScript* createScript(
				Program&, Scope&, ParserScriptType, std::string const& name,
				CompileErrorHandler*);
	public:
		ParserScriptType getType() const /*override*/ {return type;}
		void setName(std::string const& newname) /*override*/ {name = newname;}
		std::string const& getName() const /*override*/ {return name;};
		std::string const& getAuthor() const /*override*/ {return builtin_author;};
		zasm_meta const& getMetadata() const /*override*/
		{
			static zasm_meta builtin_meta;
			builtin_meta.autogen();
			return builtin_meta;
		};
		ASTScript* getNode() const /*override*/ {return NULL;};
		ScriptScope& getScope() /*override*/ {return *scope;}
		ScriptScope const& getScope() const /*override*/ {return *scope;}
		
	private:
		BuiltinScript(Program&, ParserScriptType, std::string const& name);
		static const std::string builtin_author;
		
		ParserScriptType type;
		std::string name;
		ScriptScope* scope;
	};

	UserScript* createScript(
			Program&, Scope&, ASTScript&, CompileErrorHandler* = NULL);
	BuiltinScript* createScript(
			Program&, Scope&, ParserScriptType, std::string const& name,
			CompileErrorHandler* = NULL);
	
	std::optional<int32_t> getLabel(Script const&);

	////////////////////////////////////////////////////////////////
	// UserClass

	class UserClass
	{
		friend UserClass* createClass(
			Program&, Scope&, ASTClass&, CompileErrorHandler*);
	public:
		~UserClass();

		std::string const& getName() const {return node.getName();}
		ASTClass* getNode() const {return &node;}
		ClassScope& getScope() {return *scope;}
		ClassScope const& getScope() const {return *scope;}
		DataType* getType() {return classType;}
		void setType(DataType* t) {classType = t;}
		
		std::string internalRefVar;
		std::vector<int32_t> members;
	protected:
		UserClass(Program& program, ASTClass& user_class);
		DataType* classType;

	private:
		Program& program;
		ASTClass& node;
		ClassScope* scope;
	};

	UserClass* createClass(Program&, Scope&, ASTClass&, CompileErrorHandler* = NULL);

	
	////////////////////////////////////////////////////////////////
	// Namespace
	
	class Namespace
	{
		friend Namespace* createNamespace(Program& program, Scope& parentScope, ASTNamespace& node, CompileErrorHandler* errorHandler);
		
	public:
		Namespace(ASTNamespace& namesp);
		std::string const& getName() const {return name;}
		NamespaceScope& getScope() {return *scope;}
		NamespaceScope const& getScope() const {return *scope;}
		void setScope(NamespaceScope* newscope) {scope = newscope;}
		
	private:
		
		NamespaceScope* scope;
		std::string name;
	};
	
	Namespace* createNamespace(Program& program, Scope& parentScope, ASTNamespace& node, CompileErrorHandler* errorHandler = NULL);
	
	////////////////////////////////////////////////////////////////
	// Datum
	
	// Something that can be resolved to a data value.
	class Datum
	{
	public:
		// The containing scope.
		Scope& scope;

		// The type of this data.
		DataType const& type;

		// Id for lookup tables.
		int32_t const id;

		// Get the data's name.
		virtual std::optional<std::string> getName() const {return std::nullopt;}

		// Get a doc comment associated with this data.
		virtual std::optional<std::string> getDocComment() const {return std::nullopt;}
		
		// Get the value at compile time.
		virtual std::optional<int32_t> getCompileTimeValue(bool getinitvalue = false) const {return std::nullopt;}

		// Get the declaring node.
		virtual AST* getNode() const {return NULL;}
		
		// Get the global register this uses.
		virtual std::optional<int32_t> getGlobalId() const {return std::nullopt;}
		
		virtual bool isBuiltIn() const {return false;}
		
		virtual ~Datum() = default;
		
		int32_t getStackOffset(bool i10k = true) const;
		
		void mark_erased() {erased = true;}
		bool is_erased() const {return erased;}

	protected:
		Datum(Scope& scope, DataType const& type);
		
		bool erased;
		
		// Call in static creation function to register with scope.
		bool tryAddToScope(CompileErrorHandler* = NULL);
	};

	// Is this datum a global value?
	bool isGlobal(Datum const& data);

	// Return the stack offset of the value.
	std::optional<int32_t> getStackOffset(Datum const&);

	// A literal value that requires memory management.
	class Literal : public Datum
	{
	public:
		static Literal* create(
				Scope&, ASTLiteral&, DataType const&,
				CompileErrorHandler* = NULL);
		
		ASTLiteral* getNode() const {return &node;}

	private:
		Literal(Scope& scope, ASTLiteral& node, DataType const& type);

		ASTLiteral& node;
	};

	// A variable.
	class Variable : public Datum
	{
	public:
		static Variable* create(
				Scope&, ASTDataDecl&, DataType const&,
				CompileErrorHandler* = NULL);

		std::optional<std::string> getName() const {return node.getName();}
		std::optional<std::string> getDocComment() const {return node.doc_comment;}
		ASTDataDecl* getNode() const {return &node;}
		std::optional<int32_t> getGlobalId() const {return globalId;}
		virtual std::optional<int32_t> getCompileTimeValue(bool getinitvalue = false) const;
	private:
		Variable(Scope& scope, ASTDataDecl& node, DataType const& type);

		ASTDataDecl& node;
		std::optional<int32_t> globalId;
	};
	
	//A UserClass variable
	class UserClassVar : public Datum
	{
	public:
		static UserClassVar* create(
				Scope&, ASTDataDecl&, DataType const&,
				CompileErrorHandler* = NULL);

		std::optional<std::string> getName() const {return node.getName();}
		std::optional<std::string> getDocComment() const {return node.doc_comment;}
		ASTDataDecl* getNode() const {return &node;}
		UserClass* getClass() const {return &(scope.getClass()->user_class);}
		int32_t getIndex() const {return _index;}
		void setIndex(int32_t ind) {_index = ind;}
		int32_t getOrder() const {return _order_ind;}
		void setOrder(int32_t ind) {_order_ind = ind;}
		
		bool is_arr;
		bool is_internal;
		bool is_readonly;
	private:
		UserClassVar(Scope& scope, ASTDataDecl& node, DataType const& type);
		
		int32_t _index;
		int32_t _order_ind;
		ASTDataDecl& node;
	};

	// A compiler generated variable.
	class BuiltinVariable : public Datum
	{
	public:
		static BuiltinVariable* create(
				Scope&, DataType const&, std::string const& name,
				CompileErrorHandler* = NULL);

		std::optional<std::string> getName() const {return name;}
		std::optional<int32_t> getGlobalId() const {return globalId;}

		virtual bool isBuiltIn() const {return true;}
		
	private:
		BuiltinVariable(Scope&, DataType const&, std::string const& name);

		std::string const name;
		std::optional<int32_t> globalId;
	};

	// An inlined constant.
	class Constant : public Datum
	{
	public:
		static Constant* create(
				Scope&, ASTDataDecl&, DataType const&, int32_t value,
				CompileErrorHandler* = NULL);

		std::optional<std::string> getName() const;

		virtual std::optional<int32_t> getCompileTimeValue(bool getinitvalue = false) const {return value;}

		ASTDataDecl* getNode() const {return &node;}
	
	private:
		Constant(Scope&, ASTDataDecl&, DataType const&, int32_t value);

		ASTDataDecl& node;
		int32_t value;
	};

	////////////////////////////////////////////////////////////////
	// FunctionSignature

	// Comparable signature structure.
	class FunctionSignature
	{
	public:
		FunctionSignature(
				std::string const& name,
				std::vector<DataType const*> const& parameterTypes,
				DataType const* returnType = nullptr);
		FunctionSignature(Function const& function, bool useret = false);

		int32_t compare(FunctionSignature const& other) const;
		bool operator==(FunctionSignature const& other) const;
		bool operator<(FunctionSignature const& other) const;
		std::string asString() const;
		operator std::string() const {return asString();}

		std::string name;
		bool prefix, destructor;
		DataType const* returnType;
		std::vector<DataType const*> parameterTypes;
	};
	
	
	////////////////////////////////////////////////////////////////
	// Function
	
	class Function
	{
	public:
		Function(DataType const* returnType, std::string const& name,
		         std::vector<DataType const*> paramTypes, std::vector<std::shared_ptr<const std::string>> paramNames,
		         int32_t id, int32_t flags = 0, int32_t internal_flags = 0, bool prototype = false, optional<int32_t> defaultReturn = nullopt);
		Function() = default;
		
		DataType const* returnType;
		string name;
		bool hasPrefixType;
		bool isFromTypeTemplate;
		byte extra_vargs;
		
		std::vector<DataType const*> paramTypes;
		std::vector<std::shared_ptr<const std::string>> paramNames;
		std::vector<Datum*> paramDatum;
		std::vector<DataType const*> templ_bound_ts;
		
		std::vector<int32_t> opt_vals;
		int32_t id;

		ASTFuncDecl* node;
		Datum* thisVar;
		Function* aliased_func; //the function this is an alias for, if any

		// Get the opcodes.
		std::vector<std::shared_ptr<Opcode>> const& getCode() const
		{
			if(aliased_func)
				return aliased_func->getCode();
			return ownedCode;
		}
		// Get and remove the code for this function.
		std::vector<std::shared_ptr<Opcode>> takeCode();
		// Add code for this function, transferring ownership.
		// Clears the input vector.
		void giveCode(std::vector<std::shared_ptr<Opcode>>& code);
		
		FunctionSignature getSignature(bool useret = false) const
		{
			if(aliased_func)
				return aliased_func->getSignature(useret);
			return FunctionSignature(*this, useret);
		}
		FunctionSignature getUnaliasedSignature(bool useret = false) const
		{
			return FunctionSignature(*this, useret);
		}
		
		// If this is a script level function, return that script.
		Script* getScript() const;
		UserClass* getClass() const;

		int32_t numParams() const
		{
			return paramTypes.size();
		}
		int32_t getLabel() const;
		int32_t getAltLabel() const;
		void setFlag(int32_t flag, bool state = true)
		{
			if(node) state ? node->flags |= flag : node->flags &= ~flag;
			state ? flags |= flag : flags &= ~flag;
		}
		bool getFlag(int32_t flag) const
		{
			if(aliased_func)
			{
				if(aliased_func->getFlag(flag))
					return true;
				flag &= FUNCFLAG_DEPRECATED;
			}
			
			return (flags & flag) != 0;
		}
		void setIntFlag(int32_t flag, bool state = true)
		{
			state ? internal_flags |= flag : internal_flags &= ~flag;
		}
		bool getIntFlag(int32_t flag) const
		{
			if(aliased_func)
				return aliased_func->getIntFlag(flag);
			
			return (internal_flags & flag) != 0;
		}
		void setInfo(string newinfo) {info = newinfo;}
		string const& getInfo() const
		{
			if(info.size())
				return info;
			if(aliased_func)
				return aliased_func->getInfo();
			return info;
		}
		
		bool isInternal() const {return !node;}
		bool isNil() const {
			if (prototype || getFlag(FUNCFLAG_NIL|FUNCFLAG_READ_ONLY))
				return true;

			if (getFlag(FUNCFLAG_INTERNAL))
			{
				assert(!getCode().empty());
				return false;
			}

			return false;
		}
		
		// If this is a tracing function (disabled by `#option LOGGING false`)
		bool isTracing() const;
		bool prototype;
		
		optional<int32_t> defaultReturn;
		
		bool shouldShowDepr(bool err) const;
		
		void alias(Function* func, bool force = false);
		bool is_aliased() const {return bool(aliased_func);}
		
		#define CONSTEXPR_CBACK_TY std::function<optional<int32_t>(vector<optional<int32_t>> const&, \
			AST&, CompileErrorHandler*, Scope*)>
		#define CONSTEXPR_CBACK_HEADER(...) [__VA_ARGS__](vector<optional<int32_t>> const& args, \
			AST& node, CompileErrorHandler* handler, Scope* scope) -> optional<int32_t>
		/** constexpr system:
		 * This callback lambda uses the header macro 'CONSTEXPR_CBACK_HEADER()' above.
		 * The 'vector<optional<int>> const& args' parameter contains the compile-time value of each function
		 *     parameter, or 'nullopt' if the value is not compile-time constant.
		 * The lambda returns an 'optional<int>', which should be 'nullopt' if not enough of the function
		 *     parameters were constant to determine the return value, and otherwise should be the
		 *     return value of this Function for those constant parameters.
		 * -Em
		 */
		void set_constexpr(CONSTEXPR_CBACK_TY callback) {constexpr_callback = callback;}
		CONSTEXPR_CBACK_TY const& get_constexpr() const
		{
			if(aliased_func)
				return aliased_func->get_constexpr();
			return constexpr_callback;
		}
		
		FunctionScope* getInternalScope()
		{
			if(aliased_func)
				return aliased_func->getInternalScope();
			return internalScope;
		}
		FunctionScope const* getInternalScope() const
		{
			if(aliased_func)
				return aliased_func->getInternalScope();
			return internalScope;
		}
		void setInternalScope(FunctionScope* scope)
		{
			if(aliased_func)
				return aliased_func->setInternalScope(scope);
			internalScope = scope;
		}
		Scope* getExternalScope()
		{
			if(aliased_func)
				return aliased_func->getExternalScope();
			return externalScope;
		}
		Scope const* getExternalScope() const
		{
			if(aliased_func)
				return aliased_func->getExternalScope();
			return externalScope;
		}
		void setExternalScope(Scope* scope)
		{
			if(aliased_func)
				return aliased_func->setExternalScope(scope);
			externalScope = scope;
		}

		ASTFuncDecl* getNode() const
		{
			return node;
		}
		
		bool isTemplate() const
		{
			if(returnType->isTemplate())
				return true;
			for(DataType const* ty : paramTypes)
				if(ty->isTemplate())
					return true;
			return false;
		}
		
		bool isTemplateSkip() const
		{
			return !getFlag(FUNCFLAG_INTERNAL) && isTemplate();
		}
		
		Function* apply_templ_func(std::vector<DataType const*> const& bound_ts);
		
		std::vector<std::shared_ptr<Function>>& get_applied_funcs() {return applied_funcs;}
		std::vector<std::shared_ptr<Function>> const& get_applied_funcs() const {return applied_funcs;}
		
	private:
		CONSTEXPR_CBACK_TY constexpr_callback;
		
		mutable std::optional<int32_t> label;
		mutable std::optional<int32_t> altlabel;
		int32_t flags, internal_flags;
		FunctionScope* internalScope;
		Scope* externalScope; //used by template functions
		string info;
		
		std::vector<std::shared_ptr<Function>> applied_funcs;

		// Code implementing this function.
		std::vector<std::shared_ptr<Opcode>> ownedCode;
		friend class ASTFuncDecl;
	};

	// Is this function a "run" function?
	bool isRun(Function const&);

	// Get the size of the function stack.
	int32_t getStackSize(Function const&);

	// Get the function's parameter count, including "this" if present.
	int32_t getParameterCount(Function const&);
}

#endif
