#ifndef ZSCRIPT_AST_H_
#define ZSCRIPT_AST_H_

// Declare for flex and bison.
namespace ZScript
{
	class AST;
}
#define YYSTYPE ZScript::AST*

#include <list>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "base/ints.h"
#include "base/headers.h"
#include "y.tab.hpp"
#include "Compiler.h"
#include "CompileOption.h"
#include "Scope.h"
#include "owning_ptr.h"
#include "owning_vector.h"

////////////////////////////////////////////////////////////////

extern std::string curfilename;

namespace ZScript
{
	////////////////////////////////////////////////////////////////
	// Forward Declarations
	
	// ASTVisitors.h
	class ASTVisitor;
	// CompileError.h
	class CompileError;
	class CompileErrorHandler;
	// Scope.h
	class Scope;
	class ZClass;
	// ZScript.h
	class Datum;
	class Literal;
	class Function;
	class UserClass;

	// Local
	class AST; // virtual
	class ASTFile;
	class ASTFloat;
	class ASTString;
	class ASTStringList;
	class ASTAnnotation;
	class ASTAnnotationList;
	class ASTSetOption;
	// Statements
	class ASTStmt; // virtual
	class ASTBlock;
	class ASTStmtIf;
	class ASTStmtIfElse;
	class ASTStmtSwitch;
	class ASTSwitchCases;
	class ASTRange;
	class ASTStmtFor;
	class ASTStmtWhile;
	class ASTStmtDo;
	class ASTStmtRepeat;
	class ASTStmtReturn;
	class ASTStmtReturnVal;
	class ASTStmtBreak;
	class ASTStmtContinue;
	class ASTAssert;
	class ASTStmtEmpty;
	// Declarations
	class ASTDecl; // virtual
	class ASTScript;
	class ASTClass;
	class ASTNamespace;
	class ASTImportDecl;
	class ASTImportCondDecl;
	class ASTFuncDecl;
	class ASTDataDeclList;
	class ASTDataDecl;
	class ASTDataDeclExtraArray;
	class ASTDataTypeDef;
	class ASTScriptTypeDef;
	class ASTUsingDecl;
	// Expressions
	class ASTExpr; // virtual
	class ASTExprConst;
	class ASTExprVarInitializer;
	class ASTExprAssign;
	class ASTExprIdentifier;
	class ASTExprArrow;
	class ASTExprIndex;
	class ASTExprCall;
	class ASTUnaryExpr; // virtual
	class ASTExprNegate;
	class ASTExprNot;
	class ASTExprBitNot;
	class ASTExprIncrement;
	class ASTExprDecrement;
	class ASTExprCast;
	class ASTBinaryExpr; // virtual
	class ASTLogExpr; // virtual
	class ASTExprAnd;
	class ASTExprOr;
	class ASTRelExpr; // virtual
	class ASTExprGT;
	class ASTExprGE;
	class ASTExprLT;
	class ASTExprLE;
	class ASTExprEQ;
	class ASTExprNE;
	class ASTExprXOR;
	class ASTAddExpr; // virtual
	class ASTExprPlus;
	class ASTExprMinus;
	class ASTMultExpr; // virtual
	class ASTExprTimes;
	class ASTExprDivide;
	class ASTExprModulo;
	class ASTBitExpr; // virtual
	class ASTExprBitAnd;
	class ASTExprBitOr;
	class ASTExprBitXor;
	class ASTShiftExpr; // virtual
	class ASTExprLShift;
	class ASTExprRShift;
	class ASTTernaryExpr;
	// Literals
	class ASTLiteral; // virtual
	class ASTNumberLiteral;
	class ASTLongNumberLiteral;
	class ASTBoolLiteral;
	class ASTStringLiteral;
	class ASTArrayLiteral;
	class ASTOptionValue;
	// Types
	class ASTScriptType;
	class ASTDataType;
	
	//This macro defines a "comment UID" for to be separate per-class.
	#define DEF_COMMENT_UID() \
	public: \
		uint get_comment_id() \
		{ \
			if(!comment_id) \
				comment_id = ++next_comment_id; \
			return *comment_id; \
		} \
	private: \
		static uint next_comment_id; \
		optional<uint> comment_id

	//////////////////////////////////////////////////////////////////////////////
	class LocationData
	{
	public:
		int32_t first_line; // 1-indexed
		int32_t last_line; // 1-indexed
		int32_t first_column; // 1-indexed
		int32_t last_column; // 1-indexed
		std::string fname;

		LocationData()
			: first_line(1), last_line(1),
			  first_column(1), last_column(1),
			  fname(curfilename)
		{}

		LocationData(YYLTYPE loc)
			: first_line(loc.first_line), last_line(loc.last_line),
			  first_column(loc.first_column), last_column(loc.last_column),
			  fname(curfilename)
		{}

		std::string asString() const;
	};

	LocationData const LOC_NONE;

	////////////////////////////////////////////////////////////////

	class AST
	{
	public:
		// Clone a single node pointer.
		template <class Node>
		static Node* clone(Node* node) {return node ? node->clone() : NULL;}
		// Clone a single node auto pointer.
		template <class Node>
		static std::unique_ptr<Node> clone(std::unique_ptr<Node> const& node) {
			return node.get()
				? std::unique_ptr<Node>(node->clone())
				: std::unique_ptr<Node>();}
		// Clone a vector of AST nodes.
		template <class Node>
		static std::vector<Node*> clone(std::vector<Node*> const& nodes)
		{
			std::vector<Node*> clones;
			for (typename std::vector<Node*>::const_iterator it = nodes.begin();
			     it != nodes.end(); ++it)
				clones.push_back((*it)->clone());
			return clones;
		}	
		// Clone a list of AST nodes.
		template <class Node> static std::list<Node*> clone(std::list<Node*> const& nodes)
		{
			std::list<Node*> clones;
			for (typename std::list<Node*>::const_iterator it = nodes.begin();
			     it != nodes.end(); ++it)
				clones.push_back((*it)->clone());
			return clones;
		}
		// Execute a single node pointer.
		template <class Node>
		static void execute(Node* node, ASTVisitor& visitor, void* param = NULL)
		{
			if (node) node->execute(visitor, param);
		}
		// Execute a vector of AST nodes.
		template <class Node>
		static void execute(
				std::vector<Node*> const& nodes, ASTVisitor& visitor,
				void* param = NULL)
		{
			for (typename std::vector<Node*>::const_iterator it = nodes.begin();
			     it != nodes.end(); ++it)
				(*it)->execute(visitor, param);
		}
		// Execute a list of AST nodes.
		template <class Node>
		static void execute(
				std::list<Node*> const& nodes, ASTVisitor& visitor,
				void* param = NULL)
		{
			for (typename std::list<Node*>::const_iterator it = nodes.begin();
			     it != nodes.end(); ++it)
				(*it)->execute(visitor, param);
		}

		AST(LocationData const& location = LOC_NONE);
		virtual ~AST() {}
		// Calls subclass's copy constructor on self.
		virtual AST* clone() const = 0;

		virtual void execute(ASTVisitor& visitor, void* param = NULL) = 0;
		virtual std::string asString() const {return "unknown";}

		// Filename and linenumber.
		LocationData location;

		// Documentation string associated with this node.
		string doc_comment;

		// List of expected compile error ids for this node. They are
		// removed as they are encountered.
		owning_vector<ASTExprConst> compileErrorCatches;

		// If this node has been disabled due to an error.
		bool errorDisabled;
		
		bool isDisabled() const {return disabled_;}
		void disable() {disabled_ = true;}
		void undisable() {disabled_ = false;}
		bool registered() const {return isRegistered;}
		void mark_registered() {isRegistered = true;}
		bool reachable() const {return isReachable;}
		void mark_reachable(bool b = true) {isReachable = b;}

		std::map<std::string, std::string> getParsedDocComment() const;
	
		// Subclass Predicates (replacing typeof and such).
		virtual bool isTypeArrow() const {return false;}
		virtual bool isTypeArrowUsrClass() const {return false;}
		virtual bool isTypeArrowNonUsrClass() const {return false;}
		virtual bool isTypeIndex() const {return false;}
		virtual bool isTypeIdentifier() const {return false;}
		virtual bool isTypeVarDecl() const {return false;}
		virtual bool isTypeArrayDecl() const {return false;}
		virtual bool isStringLiteral() const {return false;}
		virtual bool isArrayLiteral() const {return false;}

		virtual std::optional<LocationData> getIdentifierLocation() const {return std::nullopt;}
		virtual Scope* getScope() const {return nullptr;}

	private:
		//If this node has been disabled, for some reason or other. This will prevent any visitor from visiting the node (instant return, without error)
		bool disabled_;
		//If this node has been registered by RegistrationVisitor
		bool isRegistered;
		//If this node has been reached by ReturnVisitor
		bool isReachable;
	};


	////////////////////////////////////////////////////////////////
	
	template<class NodeType>
	class ASTNodeList : public AST
	{
	public:
		ASTNodeList(LocationData const& location = LOC_NONE) : data() {}
		ASTNodeList* clone() const {return new ASTNodeList(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL){}
		
		owning_vector<NodeType> data;
		
		void push(NodeType* node) {data.push_back(node);}
		owning_vector<NodeType>&& take() {return std::move(data);}
	};
	
	class ASTFile : public AST
	{
	public:
		ASTFile(LocationData const& location = LOC_NONE);
		virtual ASTFile* clone() const {return new ASTFile(*this);}
    
		virtual void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;

		// Add a declaration to the proper list based on its type.
		void addDeclaration(ASTDecl* declaration);
		bool hasDeclarations() const;

		owning_vector<ASTSetOption> options;
		owning_vector<ASTImportDecl> imports;
		owning_vector<ASTImportCondDecl> condimports;
		owning_vector<ASTIncludePath> inclpaths;
		owning_vector<ASTDataDeclList> variables;
		owning_vector<ASTFuncDecl> functions;
		owning_vector<ASTDataTypeDef> dataTypes;
		owning_vector<ASTScriptTypeDef> scriptTypes;
		owning_vector<ASTScript> scripts;
		owning_vector<ASTClass> classes;
		owning_vector<ASTNamespace> namespaces;
		owning_vector<ASTUsingDecl> use;
		owning_vector<ASTAssert> asserts;
		
		FileScope* scope;
	};

	class ASTFloat : public AST
	{
	public:
		enum Type {TYPE_DECIMAL, TYPE_BINARY, TYPE_HEX,
			TYPE_L_DECIMAL, TYPE_L_BINARY, TYPE_L_HEX,
			TYPE_OCTAL, TYPE_L_OCTAL,
			TYPE_BINARY_2, TYPE_L_BINARY_2,
			TYPE_OCTAL_2, TYPE_L_OCTAL_2};
	
		ASTFloat(char* value, Type type,
		         LocationData const& location = LOC_NONE);
		ASTFloat(char const* value, Type type,
		         LocationData const& location = LOC_NONE);
		ASTFloat(std::string const& value, Type type,
		         LocationData const& location = LOC_NONE);
		ASTFloat(zfix value, LocationData const& location = LOC_NONE);
		ASTFloat(int32_t ipart, int32_t dpart,
		         LocationData const& location = LOC_NONE);
		ASTFloat* clone() const {return new ASTFloat(*this);}
	
		void execute(ASTVisitor& visitor, void* param = NULL);
    	
		std::pair<std::string,std::string> parseValue(Scope* scope) const;
		int32_t getValue(Scope* scope);
		void initNeg();
		Type type;
		std::string value;
		bool negative;
	};

	class ASTString : public AST
	{
	public:
		ASTString(const char* str,
		          LocationData const& location = LOC_NONE);
		ASTString(const char* str,
				  std::string comment,
		          LocationData const& location);
		ASTString(std::string const& str,
		          LocationData const& location = LOC_NONE);
		ASTString* clone() const {return new ASTString(*this);}
	
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		void append(std::string const& newstr) {str += newstr;}
		const std::string& getValue() const {return str;}
		void setValue(std::string v) {str = v;}
	private:
		std::string str;
	};
	
	class ASTStringList : public AST
	{
	public:
		ASTStringList(LocationData const& location = LOC_NONE);
		ASTStringList(std::string comment,
		          LocationData const& location);
		ASTStringList* clone() const {return new ASTStringList(*this);}
	
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_vector<ASTString> strings;
	};

	class ASTAnnotation : public AST
	{
	public:
		ASTAnnotation(ASTString* key, ASTString* strval,
		          LocationData const& location = LOC_NONE);
		ASTAnnotation(ASTString* key, ASTFloat* intval,
		          LocationData const& location = LOC_NONE);
		ASTAnnotation(ASTString* key, LocationData const& location = LOC_NONE);
		ASTAnnotation* clone() const {return new ASTAnnotation(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_ptr<ASTString> key, strval;
		owning_ptr<ASTFloat> intval;
	};
	
	class ASTAnnotationList : public AST
	{
	public:
		ASTAnnotationList(LocationData const& location = LOC_NONE);
		ASTAnnotationList* clone() const {return new ASTAnnotationList(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_vector<ASTAnnotation> set;
	};
	
	class ASTSetOption : public AST
	{
	public:
		ASTSetOption(std::string const& name, ASTExprConst* expr,
		             LocationData const& location = LOC_NONE);
		ASTSetOption(std::string const& name,
		             CompileOptionSetting setting,
		             LocationData const& location = LOC_NONE);
		virtual ASTSetOption* clone() const {return new ASTSetOption(*this);}

		virtual void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;

		CompileOptionSetting getSetting(
				CompileErrorHandler* = NULL, Scope* scope = NULL);
	
		std::string name;
		CompileOption option;
		owning_ptr<ASTExprConst> expr;
		CompileOptionSetting setting;
	};

	////////////////////////////////////////////////////////////////
	// Statements

	class ASTStmt : public AST
	{
	public:
		ASTStmt(LocationData const& location = LOC_NONE);
		virtual ASTStmt* clone() const = 0;
	};
    
	class ASTBlock : public ASTStmt
	{
	public:
		ASTBlock(LocationData const& location = LOC_NONE);
		ASTBlock* clone() const {return new ASTBlock(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		owning_vector<ASTSetOption> options;
		owning_vector<ASTStmt> statements;
		
		Scope* getScope() {return scope;}
		void setScope(Scope* scp) {scope = scp;}
	private:
		Scope* scope;
	};
    
	class ASTStmtIf : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtIf(ASTExpr* condition, ASTBlock* thenStatement,
		          LocationData const& location = LOC_NONE);
		ASTStmtIf(ASTDataDecl* declaration, ASTBlock* thenStatement,
		          LocationData const& location = LOC_NONE);
		ASTStmtIf* clone() const {return new ASTStmtIf(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		void invert() {inverted = !inverted;}
		bool isInverted() const {return inverted;}
		bool isDecl() const {return _isDecl;}
		
		Scope* getScope() {return scope;}
		void setScope(Scope* scp) {scope = scp;}

		owning_ptr<ASTExpr> condition;
		owning_ptr<ASTDataDecl> declaration;
		owning_ptr<ASTBlock> thenStatement;
	private:
		bool inverted;
		bool _isDecl;
		
		Scope* scope;
	};

	class ASTStmtIfElse : public ASTStmtIf
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtIfElse(
				ASTExpr* condition, ASTBlock* thenStatement, ASTBlock* elseStatement,
				LocationData const& location = LOC_NONE);
		ASTStmtIfElse(
				ASTDataDecl* declaration, ASTBlock* thenStatement, ASTBlock* elseStatement,
				LocationData const& location = LOC_NONE);
		ASTStmtIfElse* clone() const {return new ASTStmtIfElse(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		owning_ptr<ASTBlock> elseStatement;
	};

	// A switch statement.
	class ASTStmtSwitch : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtSwitch(LocationData const& location = LOC_NONE);
		ASTStmtSwitch* clone() const {return new ASTStmtSwitch(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		// The key expression used to switch.
		owning_ptr<ASTExpr> key;
		// A vector of case groupings.
		owning_vector<ASTSwitchCases> cases;
		
		bool isString;
		
		optional<vector<ASTSwitchCases*>> getCompileTimeCases(
			CompileErrorHandler* errorHandler, Scope* scope);
	private:
	};

	// A grouping of switch statement labels, and the code for the group.
	class ASTSwitchCases : public AST
	{
	public:
		ASTSwitchCases(LocationData const& location = LOC_NONE);
		ASTSwitchCases* clone() const {return new ASTSwitchCases(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		// The list of case labels.
		owning_vector<ASTExprConst> cases;
		// The list of string labels
		owning_vector<ASTStringLiteral> str_cases;
		//The list of cases with ranges
		owning_vector<ASTRange> ranges;
		// If the default case is included in this grouping.
		bool isDefault;
		// The block to run.
		owning_ptr<ASTBlock> block;
	};
	
	class ASTRange : public AST
	{
	public:
		ASTRange(ASTExpr* start, ASTExpr* end, uint type = RANGE_LR, LocationData const& location = LOC_NONE);
		ASTRange* clone() const {return new ASTRange(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_ptr<ASTExpr> start;
		owning_ptr<ASTExpr> end;
		
		optional<int32_t> getStartVal(bool inclusive, CompileErrorHandler* errorHandler, Scope* scope);
		optional<int32_t> getEndVal(bool inclusive, CompileErrorHandler* errorHandler, Scope* scope);
		uint type;
		static const uint RANGE_N = 0x0;
		static const uint RANGE_L = 0x1;
		static const uint RANGE_R  = 0x2;
		static const uint RANGE_LR = 0x3;
	};


	class ASTStmtFor : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtFor(ASTStmt* setup, ASTExpr* test,
		           ASTNodeList<ASTStmt>* increments, ASTStmt* body,
				   ASTStmt* elseBlock,
		           LocationData const& location = LOC_NONE);
		ASTStmtFor* clone() const {return new ASTStmtFor(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		owning_ptr<ASTStmt> setup;
		owning_ptr<ASTExpr> test;
		owning_vector<ASTStmt> increments;
		owning_ptr<ASTStmt> body;
		owning_ptr<ASTStmt> elseBlock;
		
		bool ends_loop, ends_else;
		
		bool hasElse() const {return elseBlock;}
		Scope* getScope() {return scope;}
		void setScope(Scope* scp) {scope = scp;}
	private:
		Scope* scope;
	};
	class ASTStmtForEach : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtForEach(ASTString* identifier, ASTExpr* expr,
		           ASTStmt* body, ASTStmt* elseBlock,
		           LocationData const& location = LOC_NONE);
		ASTStmtForEach* clone() const {return new ASTStmtForEach(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_ptr<ASTString> identifier;
		owning_ptr<ASTDataDecl> indxdecl;
		owning_ptr<ASTDataDecl> arrdecl;
		owning_ptr<ASTDataDecl> decl;
		owning_ptr<ASTExpr> arrExpr;
		owning_ptr<ASTStmt> body;
		owning_ptr<ASTStmt> elseBlock;
		
		bool ends_loop, ends_else;
		
		bool hasElse() const {return elseBlock;}
		Scope* getScope() {return scope;}
		void setScope(Scope* scp) {scope = scp;}
	private:
		Scope* scope;
	};

	class ASTStmtRangeLoop : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtRangeLoop(ASTDataType* type, ASTString* iden, ASTRange* range,
			ASTExpr* increment, ASTStmt* body, LocationData const& location = LOC_NONE);
		ASTStmtRangeLoop* clone() const {return new ASTStmtRangeLoop(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_ptr<ASTString> iden;
		uint overflow;
		owning_ptr<ASTDataType> type;
		owning_ptr<ASTDataDecl> decl;
		owning_ptr<ASTRange> range;
		owning_ptr<ASTExpr> increment;
		owning_ptr<ASTStmt> body;
		owning_ptr<ASTStmt> elseBlock;
		
		bool ends_loop, ends_else;
		
		bool hasElse() const {return elseBlock;}
		Scope* getScope() {return scope;}
		void setScope(Scope* scp) {scope = scp;}
		
		static const uint OVERFLOW_ALLOW = 0;
		static const uint OVERFLOW_INT = 1;
		static const uint OVERFLOW_LONG = 2;
	private:
		Scope* scope;
	};

	class ASTStmtWhile : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtWhile(ASTExpr* test, ASTStmt* body, ASTStmt* elseBlock,
		             LocationData const& location = LOC_NONE);
		ASTStmtWhile* clone() const {return new ASTStmtWhile(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		void invert() {inverted = !inverted;}
		bool isInverted() const {return inverted;}

		owning_ptr<ASTExpr> test;
		owning_ptr<ASTStmt> body;
		owning_ptr<ASTStmt> elseBlock;
		
		bool ends_loop, ends_else;
		
		bool hasElse() const {return elseBlock;}
	private:
		bool inverted;
	};

	class ASTStmtDo : public ASTStmt
	{
		DEF_COMMENT_UID();
	public:
		ASTStmtDo(ASTExpr* test,ASTStmt* body, ASTStmt* elseBlock,
		          LocationData const& location = LOC_NONE);
		ASTStmtDo* clone() const {return new ASTStmtDo(*this);}
	
		void execute(ASTVisitor& visitor, void* param = NULL);
		void invert() {inverted = true;}
		bool isInverted() const {return inverted;}

		owning_ptr<ASTExpr> test;
		owning_ptr<ASTStmt> body;
		owning_ptr<ASTStmt> elseBlock;
		
		bool ends_loop, ends_else;
		
		bool hasElse() const {return elseBlock;}
	private:
		bool inverted;
	};
	
	class ASTStmtRepeat : public ASTStmt
	{
	public:
		ASTStmtRepeat(ASTExprConst* iter, ASTStmt* body, LocationData const& location = LOC_NONE);
		ASTStmtRepeat* clone() const {return new ASTStmtRepeat(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		owning_ptr<ASTExprConst> iter;
		owning_vector<ASTStmt> bodies;
		owning_ptr<ASTStmt> body;
	};

	class ASTStmtReturn : public ASTStmt
	{
	public:
		ASTStmtReturn(LocationData const& location = LOC_NONE);
		ASTStmtReturn* clone() const {return new ASTStmtReturn(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
	};

	class ASTStmtReturnVal : public ASTStmtReturn
	{
	public:
		ASTStmtReturnVal(ASTExpr* value = NULL,
		                 LocationData const& location = LOC_NONE);
		ASTStmtReturnVal* clone() const {return new ASTStmtReturnVal(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		owning_ptr<ASTExpr> value;
	};

	class ASTStmtBreak : public ASTStmt
	{
	public:
		ASTStmtBreak(ASTNumberLiteral* val = NULL, LocationData const& location = LOC_NONE);
		ASTStmtBreak* clone() const {return new ASTStmtBreak(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		size_t breakCount;
		owning_ptr<ASTNumberLiteral> count;
	};

	class ASTStmtContinue : public ASTStmt
	{
	public:
		ASTStmtContinue(ASTNumberLiteral* val = NULL, LocationData const& location = LOC_NONE);
		ASTStmtContinue* clone() const {return new ASTStmtContinue(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		size_t contCount;
		owning_ptr<ASTNumberLiteral> count;
	};

	class ASTStmtEmpty : public ASTStmt
	{
	public:
		ASTStmtEmpty(LocationData const& location = LOC_NONE);
		ASTStmtEmpty* clone() const {return new ASTStmtEmpty(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
	};

	////////////////////////////////////////////////////////////////
	// Declarations

	// virtual
	class ASTDecl : public ASTStmt
	{
	public:
		// Different declaration types.
		enum Type
		{
			TYPE_VOID,
			TYPE_SCRIPT,
			TYPE_IMPORT,
			TYPE_CONSTANT,
			TYPE_FUNCTION,
			TYPE_DATALIST,
			TYPE_DATA,
			TYPE_DATATYPE,
			TYPE_SCRIPTTYPE,
			TYPE_NAMESPACE,
			TYPE_USING,
			TYPE_ASSERT,
			TYPE_IMPORT_COND,
			TYPE_INCLUDE_PATH,
			TYPE_CLASS
		};

		ASTDecl(LocationData const& location = LOC_NONE);
		ASTDecl* clone() const /*override*/ = 0;

		// Return the subclass id.
		virtual Type getDeclarationType() const = 0;
	};

	// Declares a script.
	class ASTScript : public ASTDecl
	{
	public:
		ASTScript(LocationData const& location = LOC_NONE);
		ASTScript* clone() const /*override*/ {return new ASTScript(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_SCRIPT;}
    
		// Adds a declaration to the proper vector.
		void addDeclaration(ASTDecl& declaration);

		const std::string& getName() const {return identifier->getValue();}
		std::optional<LocationData> getIdentifierLocation() const {return identifier->location;}

		owning_ptr<ASTString> identifier;
		owning_ptr<ASTScriptType> type;
		zasm_meta metadata;
		owning_vector<ASTSetOption> options;
		owning_vector<ASTDataDeclList> variables;
		owning_vector<ASTFuncDecl> functions;
		owning_vector<ASTDataTypeDef> types;
		owning_vector<ASTUsingDecl> use;
		owning_vector<ASTAssert> asserts;
		
		optional<int32_t> init_weight;
		
		Script* script;
	};
	class ASTClass : public ASTDecl
	{
	public:
		ASTClass(LocationData const& location = LOC_NONE);
		ASTClass* clone() const /*override*/ {return new ASTClass(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_CLASS;}
    
		// Adds a declaration to the proper vector.
		void addDeclaration(ASTDecl& declaration);

		const std::string& getName() const {return identifier->getValue();}
		std::optional<LocationData> getIdentifierLocation() const {return identifier->location;}
		Scope* getScope() const;

		owning_ptr<ASTString> identifier;
		owning_vector<ASTSetOption> options;
		owning_vector<ASTDataDeclList> variables;
		owning_vector<ASTFuncDecl> functions;
		owning_vector<ASTDataTypeDef> types;
		owning_vector<ASTUsingDecl> use;
		owning_vector<ASTAssert> asserts;
		
		owning_vector<ASTFuncDecl> constructors;
		owning_ptr<ASTFuncDecl> destructor;
		
		owning_ptr<ASTDataType> type;
		
		UserClass* user_class;
	};

	class ASTNamespace : public ASTDecl
	{
	public:
		ASTNamespace(LocationData const& location = LOC_NONE);
		ASTNamespace* clone() const {return new ASTNamespace(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		Type getDeclarationType() const /*override*/ {return TYPE_NAMESPACE;}
    
		// Adds a declaration to the proper vector.
		void addDeclaration(ASTDecl& declaration);

		const std::string& getName() const {return identifier->getValue();}
		std::optional<LocationData> getIdentifierLocation() const {return identifier->location;}

		owning_ptr<ASTString> identifier;
		owning_vector<ASTSetOption> options;
		owning_vector<ASTDataDeclList> variables;
		owning_vector<ASTFuncDecl> functions;
		owning_vector<ASTDataTypeDef> dataTypes;
		owning_vector<ASTScriptTypeDef> scriptTypes;
		owning_vector<ASTScript> scripts;
		owning_vector<ASTClass> classes;
		owning_vector<ASTNamespace> namespaces;
		owning_vector<ASTUsingDecl> use;
		owning_vector<ASTAssert> asserts;
		
		Namespace* namesp;
	};

	class ASTImportDecl : public ASTDecl
	{
	public:
		ASTImportDecl(ASTString* import_str,
		              LocationData const& location = LOC_NONE,
					  bool isInclude = false);
		ASTImportDecl* clone() /*override*/ const {
			return new ASTImportDecl(import_str_->clone(), location, include_);}
    
		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_IMPORT;}

		void setFilename(std::string name) {import_str_->setValue(name);}
		std::string getFilename() const {return import_str_->getValue();}
		const ASTString* getImportString() const {return import_str_.get();}
		ASTFile* getTree() {return tree_.get();}
		ASTFile const* getTree() const {return tree_.get();}
		void giveTree(ASTFile* tree) {tree_ = tree;}
		bool isInclude() const {return include_;}
		bool wasChecked() const {return checked;}
		void check() {checked = true;}
		bool wasValidated() const {return validated;}
		void validate() {validated = true;}
	
	private:
		bool checked;
		bool validated;
		bool include_;
		owning_ptr<ASTFile> tree_;
		owning_ptr<ASTString> import_str_;
	};
	
	class ASTImportCondDecl : public ASTDecl
	{
	public:
		ASTImportCondDecl(ASTExprConst* cond, ASTImportDecl* import, LocationData const& location = LOC_NONE);
		ASTImportCondDecl* clone() const /*override*/ {
			return new ASTImportCondDecl(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_IMPORT_COND;}
		
		owning_ptr<ASTExprConst> cond;
		owning_ptr<ASTImportDecl> import;
		
		bool preprocessed;
	};
	
	class ASTIncludePath : public ASTDecl
	{
	public:
		ASTIncludePath(std::string const& filename, LocationData const& location = LOC_NONE);
		ASTIncludePath* clone() const /*override*/ {
			return new ASTIncludePath(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_INCLUDE_PATH;}
		
		std::string path;
	};
	
	class ASTFuncDecl : public ASTDecl
	{
	public:
		ASTFuncDecl(LocationData const& location = LOC_NONE);
		ASTFuncDecl* clone() const /*override*/ {
			return new ASTFuncDecl(*this);}
    
		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_FUNCTION;}
		
		bool getFlag(int32_t flag) const {return (flags & flag) != 0;}
		void setFlag(int32_t flag, bool state = true);
		int32_t getFlags() const {return flags;}
		bool isRun() const;

		const std::string& getName() const;
		std::optional<LocationData> getIdentifierLocation() const;
		Scope* getScope() const {return parentScope;}

		owning_ptr<ASTExprIdentifier> identifier;
		owning_ptr<ASTDataType> returnType;
		owning_vector<ASTDataDecl> parameters;
		owning_vector<ASTDataDecl> param_template;
		owning_vector<ASTExprConst> optparams;
		owning_vector<ASTString> templates;
		std::vector<int32_t> optvals;
		owning_ptr<ASTBlock> block;
		std::vector<std::shared_ptr<DataTypeTemplate>> template_types;
		std::string invalidMsg;
		Function* func;
		Scope* parentScope;
		bool prototype;
		owning_ptr<ASTExprConst> defaultReturn;
	private:
		int32_t flags;
		friend class Function;
	};

	// A line of variable/constant declarations:
	// int32_t a, b, c[];
	class ASTDataDeclList : public ASTDecl
	{
	public:
		ASTDataDeclList(LocationData const& location = LOC_NONE);
		ASTDataDeclList(ASTDataDeclList const&);
		ASTDataDeclList& operator=(ASTDataDeclList const& rhs);
		ASTDataDeclList* clone() const /*override*/ {
			return new ASTDataDeclList(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;
		Type getDeclarationType() const /*override*/ {return TYPE_DATALIST;}

		std::vector<ASTDataDecl*> const& getDeclarations() const {
			return declarations_.data();}
		virtual void addDeclaration(ASTDataDecl* declaration);
		virtual bool isEnum() const {return false;}
		Scope* getScope() const;

		// The base type at the start of the line shared by all the declarations.
		owning_ptr<ASTDataType> baseType;

		bool readonly;
		bool internal;

	private:
		// The list of individual data declarations.
		owning_vector<ASTDataDecl> declarations_;
	};
	
	class ASTDataEnum : public ASTDataDeclList
	{
	public:
		enum BitMode
		{
			BIT_NONE, BIT_INT, BIT_LONG
		};
		
		ASTDataEnum(LocationData const& location = LOC_NONE);
		ASTDataEnum(ASTDataEnum const&);
		ASTDataEnum* clone() const {return new ASTDataEnum(*this);}

		std::optional<LocationData> getIdentifierLocation() const;
		std::string getName() const;
		void execute(ASTVisitor& visitor, void* param = NULL);
		virtual bool isEnum() const {return true;}
		
		void setBitMode(BitMode m){bitmode = m;}
		BitMode getBitMode() const {return bitmode;}
		
		optional<zfix> increment_val;
	private:
		BitMode bitmode = BIT_NONE;
		int32_t nextVal;
	};

	// Declares a single variable or constant. May or may not be inside an
	// ASTDataDeclList.
	class ASTDataDecl : public ASTDecl
	{
	public:
		ASTDataDecl(LocationData const& location = LOC_NONE);
		ASTDataDecl(ASTDataDecl const&);
		ASTDataDecl& operator=(ASTDataDecl const& rhs);
		ASTDataDecl* clone() const /*override*/ {return new ASTDataDecl(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_DATA;}

		ASTExpr* getInitializer() {return initializer_.get();}
		ASTExpr const* getInitializer() const {return initializer_.get();}
		void setInitializer(ASTExpr* initializer);

		// Resolves the type, using either the list's or this node's own base type
		// as appropriate.
		DataType const& resolveType(Scope* scope, CompileErrorHandler* errorHandler);
		DataType const* resolve_ornull(Scope* scope, CompileErrorHandler* errorHandler);

		void setResolvedType(DataType const& newty);

		const std::string& getName() const {return identifier->getValue();}
		std::optional<LocationData> getIdentifierLocation() const {return identifier->location;}

		// The symbol this declaration is binding.
		owning_ptr<ASTString> identifier;
		
		// The list containing this declaration. Should be set by that list when
		// this is added.
		ASTDataDeclList* list;

		// Reference back to the datum manager for this node. Should be set by
		// that Datum when it is created.
		Datum* manager;

		// This type of this data (minus the extra arrays). This should only be
		// set if this declaration is not part of a list, as the list's base type
		// should be used instead in that case.
		owning_ptr<ASTDataType> baseType;
		
		DataType const* resolvedType;

		// Extra array type for this specific declaration. The final type is the
		// list's base type combined with these.
		owning_vector<ASTDataDeclExtraArray> extraArrays;
		
		uint flags;
		bool getFlag(uint flg) const {return flags&flg;}
		void setFlag(uint flg, bool state = true) {SETFLAG(flags,flg,state);}
		static const uint FL_FORCE_VAR = 0x01;
		static const uint FL_SKIP_EMPTY_INIT = 0x02;

		Scope* getScope() const;

	private:
		// The initialization expression. Optional.
		owning_ptr<ASTExpr> initializer_;
	};

	bool hasSize(ASTDataDecl const&);

	// The extra array parameters appended to a data declaration name.
	class ASTDataDeclExtraArray : public AST
	{
	public:
		ASTDataDeclExtraArray(LocationData const& location = LOC_NONE);
		ASTDataDeclExtraArray* clone() const /*override*/ {
			return new ASTDataDeclExtraArray(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		// The vector of array dimensions. Empty means unspecified.
		owning_vector<ASTExpr> dimensions;

		// If this declares an a sized array.
		bool hasSize() const {return (dimensions.size()>0);}

		// Get the total size of this array at compile time.
		optional<int32_t> getCompileTimeSize(
				CompileErrorHandler* errorHandler, Scope* scope)
				const;
	};

	class ASTDataTypeDef : public ASTDecl
	{
	public:
		ASTDataTypeDef(ASTDataType* type = NULL,
		           std::string const& name = "",
		           LocationData const& location = LOC_NONE);
		ASTDataTypeDef* clone() const /*override*/ {
			return new ASTDataTypeDef(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {return TYPE_DATATYPE;}
		bool isCustomDataType() const {return false;}

		owning_ptr<ASTDataType> type;
		std::string name;
	};
	
	class ASTCustomDataTypeDef : public ASTDataTypeDef
	{
	public:
		ASTCustomDataTypeDef(ASTDataType* type,
					ASTString* identifier,
					ASTDataEnum* defn,
					LocationData const& location = LOC_NONE);
		ASTCustomDataTypeDef* clone() const {return new ASTCustomDataTypeDef(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		bool isCustomDataType() const {return true;}

		std::optional<LocationData> getIdentifierLocation() const {return identifier->location;}

		virtual Scope* getScope() const {return definition->getScope();}

		owning_ptr<ASTString> identifier;
		owning_ptr<ASTDataEnum> definition;
	};

	class ASTScriptTypeDef : public ASTDecl
	{
	public:
		ASTScriptTypeDef(
			ASTScriptType* oldType,
			std::string const& newName,
			LocationData const& location = LOC_NONE);
		ASTScriptTypeDef* clone() const /*override*/ {
			return new ASTScriptTypeDef(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;

		Type getDeclarationType() const /*override*/ {
			return TYPE_SCRIPTTYPE;}

		owning_ptr<ASTScriptType> oldType;
		std::string newName;
	};
	
	class ASTUsingDecl : public ASTDecl
	{
	public:
		ASTUsingDecl(ASTExprIdentifier* iden, LocationData const& location = LOC_NONE, bool always = false);
		virtual ASTUsingDecl* clone() const {return new ASTUsingDecl(*this);}

		virtual void execute(ASTVisitor& visitor, void* param = NULL);
		
		Type getDeclarationType() const {return TYPE_USING;}
		
		ASTExprIdentifier* getIdentifier() const {return identifier;}
		
		bool always;
		
	private:
		ASTExprIdentifier* identifier;
	};
	
	class ASTAssert : public ASTDecl
	{
	public:
		ASTAssert(ASTExprConst* expr, ASTString* msg = NULL, LocationData const& location = LOC_NONE);
		ASTAssert* clone() const {return new ASTAssert(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		
		Type getDeclarationType() const {return TYPE_ASSERT;}
		
		owning_ptr<ASTExprConst> expr;
		owning_ptr<ASTString> msg;
	};
	////////////////////////////////////////////////////////////////
	// Expressions

	class ASTExpr : public ASTStmt
	{
	public:
		ASTExpr(LocationData const& location = LOC_NONE);
		virtual ASTExpr* clone() const = 0;

		virtual bool isConstant() const = 0;
		virtual bool isLiteral() const = 0;
		virtual bool isTempVal() const = 0;

		// Return this expression's value if it has already been resolved at
		// compile time.
		virtual optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope)
		{return std::nullopt;}

		// Returns the read or write type for this expression. Null for either
		// means that it can't be read from/written to.
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		bool isLong(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(DataType const* type = getReadType(scope, errorHandler))
			{
				return type->isLong();
			}
			return false;
		}
	};

	// Wrap around an expression to type it as constant.
	class ASTExprConst : public ASTExpr
	{
	public:
		ASTExprConst(ASTExpr* content = NULL,
		             LocationData const& location = LOC_NONE);
		virtual ASTExprConst* clone() const {return new ASTExprConst(*this);}

		virtual void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return true;}
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		virtual optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {
			return content ? content->getReadType(scope, errorHandler) : NULL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	
		owning_ptr<ASTExpr> content;
	};
	
	struct BoolTreeNode
	{
		enum
		{
			MODE_LEAF, MODE_AND, MODE_OR
		};
		owning_ptr<ASTExpr> leaf;
		vector<BoolTreeNode> branch;
		int mode;
		
		optional<int32_t> getCompileTimeValue(CompileErrorHandler* errorHandler, Scope* scope);
		bool isConstant() const;
	};
	class ASTExprBoolTree : public ASTExpr
	{
	public:
		ASTExprBoolTree(LocationData const& location = LOC_NONE);
		virtual ASTExprBoolTree* clone() const {return new ASTExprBoolTree(*this);}

		virtual void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return root.isConstant();};
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		virtual optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::BOOL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		BoolTreeNode root;
	};
	
	class ASTExprVarInitializer : public ASTExprConst
	{
	public:
		ASTExprVarInitializer(ASTExpr* content = NULL,
		                LocationData const& location = LOC_NONE);
		ASTExprVarInitializer* clone() const {return new ASTExprVarInitializer(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		
		bool valueIsArray(Scope* scope, CompileErrorHandler* errorHandler);
		optional<int32_t> value;
	};

	class ASTExprAssign : public ASTExpr
	{
	public:
		ASTExprAssign(ASTExpr* left = NULL,
		              ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprAssign* clone() const {return new ASTExprAssign(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return right && right->isConstant();}
		bool isLiteral() const {return right && right->isLiteral();}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {
			return right ? right->getReadType(scope, errorHandler) : NULL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {
			return right ? right->getWriteType(scope, errorHandler) : NULL;}
	
		owning_ptr<ASTExpr> left;
		owning_ptr<ASTExpr> right;
	};

	class ASTExprIdentifier : public ASTExpr
	{
	public:
		ASTExprIdentifier(std::string const& name = "",
		                  LocationData const& location = LOC_NONE);
		ASTExprIdentifier(std::shared_ptr<ASTString> identifier,
			LocationData const& location = LOC_NONE);
		ASTExprIdentifier* clone() const {return new ASTExprIdentifier(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		std::string asString() const;
		bool isTypeIdentifier() const {return true;}

		bool isConstant() const {return constant_;}
		void markConstant() {constant_ = true;}
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler);
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler);
	
		// The identifier components separated by '.' or '::'.
		std::vector<std::string> components;
		std::vector<std::shared_ptr<ASTString>> componentNodes;
		//Which symbol was used to delimit each?
		std::vector<std::string> delimiters;

		// What this identifier refers to.
		Datum* binding;
		
		bool noUsing;
		
	private:
		bool constant_;
	};

	class ASTExprArrow : public ASTExpr
	{
	public:
		ASTExprArrow(ASTExpr* left,
		             ASTString* right,
		             LocationData const& location = LOC_NONE);
		ASTExprArrow* clone() const {return new ASTExprArrow(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		std::string asString() const;
		bool isTypeArrow() const {return true;}
		bool isTypeArrowUsrClass() const;
		bool isTypeArrowNonUsrClass() const;

		bool isConstant() const {return false;}
		bool isLiteral() const {return false;}
		bool isUsrClass() const {return u_datum;}
		virtual bool isTempVal() const {return false;}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler);
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler);
	
		owning_ptr<ASTExpr> left;
		owning_ptr<ASTString> right;
		owning_ptr<ASTExpr> index;

		UserClass* leftClass;
		Function* arrayFunction;
		Function* readFunction;
		Function* writeFunction;
		DataType const* rtype;
		DataType const* wtype;
		UserClassVar* u_datum;
		bool iscall;
	};

	class ASTExprIndex : public ASTExpr
	{
	public:
		ASTExprIndex(ASTExpr* array = NULL,
		             ASTExpr* index = NULL,
		             LocationData const& location = LOC_NONE);
		ASTExprIndex* clone() const /*override*/ {
			return new ASTExprIndex(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL) /*override*/;
		virtual std::string asString() const;
		bool isTypeIndex() const /*override*/ {return true;}
    
		bool isConstant() const /*override*/;
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) /*override*/;
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) /*override*/;
	
		owning_ptr<ASTExpr> array;
		owning_ptr<ASTExpr> index;
	};

	class ASTExprCall : public ASTExpr
	{
	public:
		ASTExprCall(LocationData const& location = LOC_NONE);
		ASTExprCall* clone() const {return new ASTExprCall(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return false;}
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler);
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler);
		void setConstructor(bool _c) {_constructor = _c;}
		bool isConstructor() const {return _constructor;}
		std::optional<LocationData> getIdentifierLocation() const {return left->location;}
		owning_ptr<ASTExpr> left;
		owning_vector<ASTExpr> parameters;
		owning_vector<ASTDataDecl> inlineParams;
		bool _constructor;
		
		Function* binding;
	};

	// virtual
	class ASTUnaryExpr : public ASTExpr
	{
	public:
		ASTUnaryExpr(LocationData const& location = LOC_NONE);
		virtual ASTUnaryExpr* clone() const = 0;

		virtual bool isConstant() const {return operand->isConstant();}
		virtual bool isLiteral() const {return operand->isLiteral();}
		virtual bool isTempVal() const {return (operand && operand->isTempVal());}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		owning_ptr<ASTExpr> operand;
	};
	
	class ASTExprDelete : public ASTUnaryExpr
	{
	public:
		ASTExprDelete(LocationData const& location = LOC_NONE);
		ASTExprDelete* clone() const {return new ASTExprDelete(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(CompileErrorHandler* errorHandler,
			Scope* scope) {return std::nullopt;}
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			return &DataType::UNTYPED;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};
	
	class ASTExprNegate : public ASTUnaryExpr
	{
	public:
		ASTExprNegate(ASTExpr* op = nullptr, LocationData const& location = LOC_NONE);
		ASTExprNegate* clone() const {return new ASTExprNegate(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(operand->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		bool done;
	};

	class ASTExprNot : public ASTUnaryExpr
	{
	public:
		ASTExprNot(ASTExpr* op = nullptr, LocationData const& location = LOC_NONE);
		ASTExprNot* clone() const {return new ASTExprNot(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::BOOL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		void invert() {inverted = !inverted;}
		bool isInverted() const {return inverted;}
	private:
		bool inverted;
	};

	class ASTExprBitNot : public ASTUnaryExpr
	{
	public:
		ASTExprBitNot(ASTExpr* op = nullptr, LocationData const& location = LOC_NONE);
		ASTExprBitNot* clone() const {return new ASTExprBitNot(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(operand->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprIncrement : public ASTUnaryExpr
	{
	public:
		ASTExprIncrement(bool pre, ASTExpr* op = nullptr, LocationData const& location = LOC_NONE);
		ASTExprIncrement* clone() const {return new ASTExprIncrement(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return false;}
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(operand->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		bool is_pre;
	};

	class ASTExprDecrement : public ASTUnaryExpr
	{
	public:
		ASTExprDecrement(bool pre, ASTExpr* op = nullptr, LocationData const& location = LOC_NONE);
		ASTExprDecrement* clone() const {return new ASTExprDecrement(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		bool isConstant() const {return false;}
		bool isLiteral() const {return false;}
		virtual bool isTempVal() const {return false;}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(operand->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		bool is_pre;
	};
	
	class ASTExprCast : public ASTUnaryExpr
	{
	public:
		ASTExprCast(ASTDataType* type, ASTExpr* expr, LocationData const& location = LOC_NONE);
		ASTExprCast* clone() const {return new ASTExprCast(*this);}
		
		void execute(ASTVisitor& visitor, void* param = NULL);
		
		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler);
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
		
		owning_ptr<ASTDataType> type;
	};

	// virtual
	class ASTBinaryExpr : public ASTExpr
	{
	public:
		ASTBinaryExpr(ASTExpr* left = NULL,
		              ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		virtual ASTBinaryExpr* clone() const = 0;

		bool isConstant() const;
		bool isLiteral() const {return left && left->isLiteral() && right && right->isLiteral();}
		virtual bool isTempVal() const {return (left && left->isTempVal()) || (right && right->isTempVal());}

		owning_ptr<ASTExpr> left;
		owning_ptr<ASTExpr> right;

	protected:
		ASTBinaryExpr& operator=(ASTBinaryExpr const& rhs);
	};

	// virtual
	class ASTLogExpr : public ASTBinaryExpr
	{
	public:
		ASTLogExpr(ASTExpr* left = NULL,
		           ASTExpr* right = NULL,
		           LocationData const& location = LOC_NONE);
		virtual ASTLogExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::BOOL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprAnd : public ASTLogExpr
	{
	public:
		ASTExprAnd(ASTExpr* left = NULL,
		           ASTExpr* right = NULL,
		           LocationData const& location = LOC_NONE);
		ASTExprAnd* clone() const {return new ASTExprAnd(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		
		owning_ptr<ASTExprBoolTree> tree;
	};

	class ASTExprOr : public ASTLogExpr
	{
	public:
		ASTExprOr(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprOr* clone() const {return new ASTExprOr(*this);}
    
		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		owning_ptr<ASTExprBoolTree> tree;
	};

	// virtual
	class ASTRelExpr : public ASTBinaryExpr
	{
	public:
		ASTRelExpr(ASTExpr* left = NULL,
		           ASTExpr* right = NULL,
		           LocationData const& location = LOC_NONE);
		virtual ASTRelExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::BOOL;}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprGT : public ASTRelExpr
	{
	public:
		ASTExprGT(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprGT* clone() const {return new ASTExprGT(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprGE : public ASTRelExpr
	{
	public:
		ASTExprGE(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprGE* clone() const {return new ASTExprGE(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprLT : public ASTRelExpr
	{
	public:
		ASTExprLT(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprLT* clone() const {return new ASTExprLT(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprLE : public ASTRelExpr
	{
	public:
		ASTExprLE(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprLE* clone() const {return new ASTExprLE(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprEQ : public ASTRelExpr
	{
	public:
		ASTExprEQ(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprEQ* clone() const {return new ASTExprEQ(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprNE : public ASTRelExpr
	{
	public:
		ASTExprNE(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprNE* clone() const {return new ASTExprNE(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};
	
	class ASTExprAppxEQ : public ASTRelExpr
	{
	public:
		ASTExprAppxEQ(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprAppxEQ* clone() const {return new ASTExprAppxEQ(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprXOR : public ASTRelExpr
	{
	public:
		ASTExprXOR(ASTExpr* left = NULL,
		          ASTExpr* right = NULL,
		          LocationData const& location = LOC_NONE);
		ASTExprXOR* clone() const {return new ASTExprXOR(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	// virtual
	class ASTAddExpr : public ASTBinaryExpr
	{
	public:
		ASTAddExpr(ASTExpr* left = NULL,
		           ASTExpr* right = NULL,
		           LocationData const& location = LOC_NONE);
		virtual ASTAddExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(left->isLong(scope, errorHandler) || right->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprPlus : public ASTAddExpr
	{
	public:
		ASTExprPlus(ASTExpr* left = NULL,
		            ASTExpr* right = NULL,
		            LocationData const& location = LOC_NONE);
		ASTExprPlus* clone() const {return new ASTExprPlus(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprMinus : public ASTAddExpr
	{
	public:
		ASTExprMinus(ASTExpr* left = NULL,
		             ASTExpr* right = NULL,
		             LocationData const& location = LOC_NONE);
		ASTExprMinus* clone() const {return new ASTExprMinus(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	// virtual
	class ASTMultExpr : public ASTBinaryExpr
	{
	public:
		ASTMultExpr(ASTExpr* left = NULL,
		            ASTExpr* right = NULL,
		            LocationData const& location = LOC_NONE);
		virtual ASTMultExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(left->isLong(scope, errorHandler) || right->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprTimes : public ASTMultExpr
	{
	public:
		ASTExprTimes(ASTExpr* left = NULL,
		             ASTExpr* right = NULL,
		             LocationData const& location = LOC_NONE);
		ASTExprTimes* clone() const {return new ASTExprTimes(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprExpn : public ASTMultExpr
	{
	public:
		ASTExprExpn(ASTExpr* left = NULL,
		             ASTExpr* right = NULL,
		             LocationData const& location = LOC_NONE);
		ASTExprExpn* clone() const {return new ASTExprExpn(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprDivide : public ASTMultExpr
	{
	public:
		ASTExprDivide(ASTExpr* left = NULL,
		              ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprDivide* clone() const {return new ASTExprDivide(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprModulo : public ASTMultExpr
	{
	public:
		ASTExprModulo(ASTExpr* left = NULL,
		              ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprModulo* clone() const {return new ASTExprModulo(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	// virtual
	class ASTBitExpr : public ASTBinaryExpr
	{
	public:
		ASTBitExpr(ASTExpr* left = NULL, ASTExpr* right = NULL,
		           LocationData const& location = LOC_NONE);
		virtual ASTBitExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(left->isLong(scope, errorHandler) || right->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprBitAnd : public ASTBitExpr
	{
	public:
		ASTExprBitAnd(ASTExpr* left = NULL, ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprBitAnd* clone() const {return new ASTExprBitAnd(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprBitOr : public ASTBitExpr
	{
	public:
		ASTExprBitOr(ASTExpr* left = NULL, ASTExpr* right = NULL,
		             LocationData const& location = LOC_NONE);
		ASTExprBitOr* clone() const {return new ASTExprBitOr(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprBitXor : public ASTBitExpr
	{
	public:
		ASTExprBitXor(ASTExpr* left = NULL, ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprBitXor* clone() const {return new ASTExprBitXor(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	// virtual
	class ASTShiftExpr : public ASTBinaryExpr
	{
	public:
		ASTShiftExpr(
				ASTExpr* left = NULL, ASTExpr* right = NULL,
				LocationData const& location = LOC_NONE);
		virtual ASTShiftExpr* clone() const = 0;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler)
		{
			if(left->isLong(scope, errorHandler) || right->isLong(scope, errorHandler))
				return &DataType::LONG;
			return &DataType::FLOAT;
		}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	class ASTExprLShift : public ASTShiftExpr
	{
	public:
		ASTExprLShift(ASTExpr* left = NULL, ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprLShift* clone() const {return new ASTExprLShift(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};

	class ASTExprRShift : public ASTShiftExpr
	{
	public:
		ASTExprRShift(ASTExpr* left = NULL, ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTExprRShift* clone() const {return new ASTExprRShift(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
	};
	
	class ASTTernaryExpr : public ASTExpr
	{
	public:
		ASTTernaryExpr(ASTExpr* left = NULL,
		              ASTExpr* middle = NULL,
		              ASTExpr* right = NULL,
		              LocationData const& location = LOC_NONE);
		ASTTernaryExpr* clone() const {return new ASTTernaryExpr(*this);};

		bool isConstant() const;
		bool isLiteral() const {return middle && middle->isLiteral() && right && right->isLiteral();}
		virtual bool isTempVal() const {return (middle && middle->isTempVal()) || (right && right->isTempVal());}

		void execute(ASTVisitor& visitor, void* param = NULL);

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);

		owning_ptr<ASTExpr> left;
		owning_ptr<ASTExpr> middle;
		owning_ptr<ASTExpr> right;

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return middle->getReadType(scope, errorHandler);}
		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}
	};

	// Literals

	// virtual
	class ASTLiteral : public ASTExpr
	{
	public:
		ASTLiteral(LocationData const& location = LOC_NONE);
		virtual ASTLiteral* clone() const = 0;

		virtual DataType const* getWriteType(Scope* scope, CompileErrorHandler* errorHandler) {return NULL;}

		Literal* manager;
	};

	class ASTNumberLiteral : public ASTLiteral
	{
	public:
		ASTNumberLiteral(
				ASTFloat* value = NULL,
				LocationData const& location = LOC_NONE);
		ASTNumberLiteral* clone() const {return new ASTNumberLiteral(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;

		bool isConstant() const {return true;}
		bool isLiteral() const {return true;}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::FLOAT;}
		
		void negate();
	
		owning_ptr<ASTFloat> value;
	};
	
	class ASTLongNumberLiteral : public ASTNumberLiteral
	{
	public:
		ASTLongNumberLiteral(
				ASTFloat* value = NULL,
				LocationData const& location = LOC_NONE);
		ASTLongNumberLiteral* clone() const {return new ASTLongNumberLiteral(*this);}

		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::LONG;}
	};
	
	class ASTCharLiteral : public ASTLiteral
	{
	public:
		ASTCharLiteral(
				ASTFloat* value = NULL,
				LocationData const& location = LOC_NONE);
		ASTCharLiteral* clone() const {return new ASTCharLiteral(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;
		
		bool isConstant() const {return true;}
		bool isLiteral() const {return true;}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::CHAR;}
	
		owning_ptr<ASTFloat> value;
	};

	class ASTBoolLiteral : public ASTLiteral
	{
	public:
		ASTBoolLiteral(
				bool value = false,
				LocationData const& location = LOC_NONE);
		ASTBoolLiteral* clone() const {return new ASTBoolLiteral(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;
		
		bool isConstant() const {return true;}
		bool isLiteral() const {return true;}
		virtual bool isTempVal() const {return false;}

		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope)
				{return value ? (*ZScript::lookupOption(*scope, CompileOption::OPT_BOOL_TRUE_RETURN_DECIMAL) ? 1L : 10000L) : 0L;}
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return &DataType::BOOL;}
	
		bool value;
	};

	class ASTStringLiteral : public ASTLiteral
	{
	public:
		ASTStringLiteral(
				char const* str = "",
				LocationData const& location = LOC_NONE);
		ASTStringLiteral(
				std::string const& str,
				LocationData const& location = LOC_NONE);
		ASTStringLiteral(ASTString const& raw);
		ASTStringLiteral(ASTStringLiteral const& base);
		ASTStringLiteral& operator=(ASTStringLiteral const& rhs);
		ASTStringLiteral* clone() const /*override*/ {
			return new ASTStringLiteral(*this);}

		void execute (ASTVisitor& visitor, void* param = NULL) /*override*/;
		virtual std::string asString() const;
		
		bool isStringLiteral() const /*override*/ {return true;}

		bool isConstant() const /*override*/ {return true;}
		bool isLiteral() const {return true;}
		virtual bool isTempVal() const {return !declaration;}

		DataTypeArray const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) /*override*/;
		
		// The data declaration that this literal may be part of. If NULL that
		// means this is not part of a data declaration. This should be managed by
		// that declaration and not modified by this object at all.
		ASTDataDecl* declaration;

		std::string value;
	};

	class ASTArrayLiteral : public ASTLiteral
	{
	public:
		ASTArrayLiteral(LocationData const& location = LOC_NONE);
		ASTArrayLiteral(ASTArrayLiteral const& base);
		ASTArrayLiteral& operator=(ASTArrayLiteral const& rhs);
		ASTArrayLiteral* clone() const {return new ASTArrayLiteral(*this);}

		void execute (ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;
		
		bool isArrayLiteral() const {return true;}

		bool isConstant() const {return true;}
		bool isLiteral() const {return true;}
		virtual bool isTempVal() const {return !declaration;}

		DataTypeArray const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {return readType_;}
		void setReadType(DataTypeArray const* type) {readType_ = type;}

		// The data declaration that this literal may be part of. If NULL that
		// means this is not part of a data declaration. This should be managed by
		// that declaration and not modified by this object at all.
		ASTDataDecl* declaration;

		// Optional type specification.
		owning_ptr<ASTDataType> type;
		// Optional size specification.
		owning_ptr<ASTExpr> size;
		// The array elements.
		owning_vector<ASTExpr> elements;

	private:
		// Cached read type.
		DataTypeArray const* readType_;
	};

	class ASTOptionValue : public ASTLiteral
	{
	public:
		ASTOptionValue(std::string const& name = "",
		               LocationData const& location = LOC_NONE);
		ASTOptionValue* clone() const {return new ASTOptionValue(*this);}

		virtual void execute(ASTVisitor& visitor, void* param = NULL);
		virtual std::string asString() const;

		virtual bool isConstant() const {return true;}
		virtual bool isTempVal() const {return false;}
		bool isLiteral() const {return false;} //Not actually a literal, despite being under 'ASTLiteral'

		
		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {
			return &DataType::FLOAT;}

		std::string name;
		CompileOption option;
		optional<int32_t> value;
	};
	
	class ASTIsIncluded : public ASTLiteral
	{
	public:
		ASTIsIncluded(std::string const& name = "",
		               LocationData const& location = LOC_NONE);
		ASTIsIncluded* clone() const {return new ASTIsIncluded(*this);}
		
		virtual void execute(ASTVisitor& visitor, void* param = NULL);
		
		virtual bool isConstant() const {return true;}
		virtual bool isTempVal() const {return false;}
		bool isLiteral() const {return false;} //Not actually a literal, despite being under 'ASTLiteral'
		
		optional<int32_t> getCompileTimeValue(
				CompileErrorHandler* errorHandler, Scope* scope);
		virtual DataType const* getReadType(Scope* scope, CompileErrorHandler* errorHandler) {
			return &DataType::BOOL;}
		
		std::string name;
	};
	// Types

	class ASTScriptType : public AST
	{
	public:
		ASTScriptType(std::string const& name, LocationData const& location);
		ASTScriptType* clone() const {return new ASTScriptType(*this);}

		void execute(ASTVisitor& visitor, void* param = NULL);

		// If type is invalid, try to resolve using name.
		ParserScriptType type;
		std::string name;
	};

	ParserScriptType resolveScriptType(ASTScriptType const&, Scope const&);

	class ASTDataType : public AST
	{
	public:
		// Takes ownership of type.
		ASTDataType(
				DataType* type = NULL,
				LocationData const& location = LOC_NONE);
		// Clones type.
		ASTDataType(
				DataType const& type,
				LocationData const& location = LOC_NONE);
		ASTDataType* clone() const {return new ASTDataType(*this);}
	
		void execute(ASTVisitor& visitor, void* param = NULL);
		DataType const& resolve(Scope& scope, CompileErrorHandler* errorHandler);
		DataType const* resolve_ornull(Scope& scope, CompileErrorHandler* errorHandler);
		void replace(DataType const& newty);
		inline bool wasResolved() const { return wasResolved_; }

		owning_ptr<DataType> type;
		int32_t constant_;
		uint becomeArray;
	private:
		bool wasResolved_;
	};
}

#endif

