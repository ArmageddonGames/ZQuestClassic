#ifndef BUILDVISITORS_H //2.53 Updated to 16th Jan, 2017
#define BUILDVISITORS_H

#include "ASTVisitors.h"
#include "ByteCode.h"
#include "ZScript.h"
#include "Scope.h"
#include "base/headers.h"
#include "parser/DataStructs.h"
#include <algorithm>
#include <vector>
#include <set>

void addOpcode2(std::vector<std::shared_ptr<ZScript::Opcode>>& v, ZScript::Opcode* code);

namespace ZScript
{
	class LValBOHelper;
	class BuildOpcodes : public RecursiveVisitor
	{
	public:
		BuildOpcodes(Scope* curScope);
		BuildOpcodes(LValBOHelper* helper);

		using RecursiveVisitor::visit;
		void visit(AST& node, void* param = NULL);
		void literal_visit(AST& node, void* param = NULL);
		void literal_visit(AST* node, void* param = NULL);
		template <class Container>
		void literal_visit_vec(Container const& nodes, void* param);
		
		void sidefx_visit(AST& node, void* param = NULL);
		void sidefx_visit(AST* node, void* param = NULL);
		template <class Container>
		void sidefx_visit_vec(Container const& nodes, void* param);
	
		virtual void caseDefault(AST& host, void* param);
		virtual void caseSetOption(ASTSetOption& host, void* param);
		virtual void caseUsing(ASTUsingDecl& host, void* param = NULL);
		// Statements
		virtual void caseBlock(ASTBlock &host, void *param);
		virtual void caseStmtIf(ASTStmtIf &host, void *param);
		virtual void caseStmtIfElse(ASTStmtIfElse &host, void *param);
		virtual void caseStmtSwitch(ASTStmtSwitch &host, void* param);
		void caseStmtStrSwitch(ASTStmtSwitch &host, void* param);
		virtual void caseStmtFor(ASTStmtFor &host, void *param);
		virtual void caseStmtForEach(ASTStmtForEach &host, void *param);
		virtual void caseStmtRangeLoop(ASTStmtRangeLoop &host, void *param);
		virtual void caseStmtWhile(ASTStmtWhile &host, void *param);
		virtual void caseStmtDo(ASTStmtDo &host, void *param);
		virtual void caseStmtReturn(ASTStmtReturn &host, void *param);
		virtual void caseStmtReturnVal(ASTStmtReturnVal &host, void *param);
		virtual void caseStmtBreak(ASTStmtBreak &host, void *param);
		virtual void caseStmtContinue(ASTStmtContinue &host, void *param);
		virtual void caseStmtEmpty(ASTStmtEmpty &host, void *param);
		// Declarations
		virtual void caseFuncDecl(ASTFuncDecl &host, void *param);
		virtual void caseDataDecl(ASTDataDecl& host, void* param);
		virtual void caseDataTypeDef(ASTDataTypeDef& host, void* param);
		virtual void caseCustomDataTypeDef(ASTDataTypeDef& host, void* param);
		// Expressions
		virtual void caseExprAssign(ASTExprAssign &host, void *param);
		virtual void caseExprIdentifier(ASTExprIdentifier &host, void *param);
		virtual void caseExprArrow(ASTExprArrow &host, void *param);
		virtual void caseExprIndex(ASTExprIndex &host, void *param);
		virtual void caseExprCall(ASTExprCall &host, void *param);
		virtual void caseExprNegate(ASTExprNegate &host, void *param);
		virtual void caseExprDelete(ASTExprDelete &host, void *param);
		virtual void caseExprNot(ASTExprNot &host, void *param);
		virtual void caseExprBitNot(ASTExprBitNot &host, void *param);
		virtual void caseExprIncrement(ASTExprIncrement &host, void *param);
		virtual void caseExprDecrement(ASTExprDecrement &host, void *param);
		virtual void caseExprBoolTree(ASTExprBoolTree &host, void *param);
		virtual void caseExprAnd(ASTExprAnd &host, void *param);
		virtual void caseExprOr(ASTExprOr &host, void *param);
		virtual void caseExprGT(ASTExprGT &host, void *param);
		virtual void caseExprGE(ASTExprGE &host, void *param);
		virtual void caseExprLT(ASTExprLT &host, void *param);
		virtual void caseExprLE(ASTExprLE &host, void *param);
		virtual void caseExprEQ(ASTExprEQ &host, void *param);
		virtual void caseExprNE(ASTExprNE &host, void *param);
		virtual void caseExprAppxEQ(ASTExprAppxEQ &host, void *param);
		virtual void caseExprXOR(ASTExprXOR &host, void *param);
		virtual void caseExprPlus(ASTExprPlus &host, void *param);
		virtual void caseExprMinus(ASTExprMinus &host, void *param);
		virtual void caseExprTimes(ASTExprTimes &host, void *param);
		virtual void caseExprExpn(ASTExprExpn &host, void *param);
		virtual void caseExprDivide(ASTExprDivide &host, void *param);
		virtual void caseExprModulo(ASTExprModulo &host, void *param);
		virtual void caseExprBitAnd(ASTExprBitAnd &host, void *param);
		virtual void caseExprBitOr(ASTExprBitOr &host, void *param);
		virtual void caseExprBitXor(ASTExprBitXor &host, void *param);
		virtual void caseExprLShift(ASTExprLShift &host, void *param);
		virtual void caseExprRShift(ASTExprRShift &host, void *param);
		virtual void caseExprTernary(ASTTernaryExpr &host, void *param);
		// Literals
		virtual void caseNumberLiteral(ASTNumberLiteral& host, void* param);
		virtual void caseCharLiteral(ASTCharLiteral& host, void* param);
		virtual void caseBoolLiteral(ASTBoolLiteral& host, void* param);
		virtual void caseStringLiteral(ASTStringLiteral& host, void* param);
		virtual void caseArrayLiteral(ASTArrayLiteral& host, void* param);
		virtual void caseOptionValue(ASTOptionValue& host, void* param);
		virtual void caseIsIncluded(ASTIsIncluded& host, void* param);
		// Types
		void caseDataType(ASTDataType& host, void* param) {}

		std::vector<std::shared_ptr<Opcode>> const& getResult() const {return result;}
		int32_t getReturnLabelID() const {return returnlabelid;}
		std::list<int32_t> *getArrayRefs() {return &arrayRefs;}
		std::list<int32_t> const *getArrayRefs() const {return &arrayRefs;}

	private:
		BuildOpcodes();
		void addOpcode(Opcode* code);
		void addOpcode(std::shared_ptr<Opcode> &code);
		Opcode* backOpcode();
		std::vector<std::shared_ptr<Opcode>>& backTarget();
		size_t commentTarget() const;
		
		void commentAt(size_t indx, std::string const& comment);
		void commentBack(std::string const& comment);
		void commentStartEnd(size_t indx, string const& comment);

		template <class Container>
		void addOpcodes(Container const& container);
	
		void deallocateArrayRef(int32_t arrayRef);
		void deallocateArrayRef(int32_t arrayRef, std::vector<std::shared_ptr<Opcode>>& code);
		void deallocateRefsUntilCount(int32_t count);
		void deallocateRefsUntilCount(int32_t count, std::vector<std::shared_ptr<Opcode>>& code);
		
		std::vector<std::shared_ptr<Opcode>> result;
		int32_t returnlabelid;
		int32_t returnRefCount;
		std::vector<int32_t> continuelabelids;
		std::vector<int32_t> continueRefCounts;
		std::vector<int32_t> breaklabelids;
		std::vector<int32_t> breakRefCounts;
		std::vector<uint> break_past_counts;
		std::vector<uint> break_to_counts;
		std::vector<uint> continue_past_counts;
		std::vector<uint> continue_to_counts;
		std::vector<uint> scope_allocations;
		std::list<int32_t> arrayRefs;
		// Stack of opcode targets. Only the latest is used.
		std::vector<std::vector<std::shared_ptr<Opcode>>*> opcodeTargets;
		
		uint break_depth, continue_depth;
		void inc_break(uint count = 1)
		{
			if(!break_depth || !count)
				return;
			for(int q = break_depth-1; count && q >= 0; --q, --count)
			{
				++break_past_counts[q];
				if(count == 1)
					++break_to_counts[q];
			}
		}
		void inc_cont(uint count = 1)
		{
			if(!continue_depth || !count)
				return;
			for(int q = continue_depth-1; count && q >= 0; --q, --count)
			{
				++continue_past_counts[q];
				if(count == 1)
					++continue_to_counts[q];
			}
		}
		void push_break(int32_t id, int32_t count, uint scope_pops = 0)
		{
			++break_depth;
			breaklabelids.push_back(id);
			breakRefCounts.push_back(count);
			break_past_counts.push_back(0);
			break_to_counts.push_back(0);
			scope_allocations.emplace_back(scope_pops);
		}
		void push_cont(int32_t id, int32_t count)
		{
			++continue_depth;
			continuelabelids.push_back(id);
			continueRefCounts.push_back(count);
			continue_past_counts.push_back(0);
			continue_to_counts.push_back(0);
		}
		void pop_break()
		{
			--break_depth;
			breaklabelids.pop_back();
			breakRefCounts.pop_back();
			break_past_counts.pop_back();
			break_to_counts.pop_back();
			scope_allocations.pop_back();
		}
		void pop_cont()
		{
			--continue_depth;
			continuelabelids.pop_back();
			continueRefCounts.pop_back();
			continue_past_counts.pop_back();
			continue_to_counts.pop_back();
		}
		uint scope_pops_back(uint break_count)
		{
			uint ret = 0;
			for(auto it = scope_allocations.rbegin(); break_count && it != scope_allocations.rend(); ++it)
			{
				ret += *it;
				--break_count;
			}
			return ret;
		}
		uint scope_pops_count()
		{
			uint ret = 0;
			for(uint v : scope_allocations)
				ret += v;
			return ret;
		}
		
		// Helper Functions.
		void pop_params(uint count);
		// For when ASTDataDecl is for a single variable.
		void buildVariable(ASTDataDecl& host, OpcodeContext& context);
		// For when ASTDataDecl is an initialized array.
		void buildArrayInit(ASTDataDecl& host, OpcodeContext& context);
		// For when ASTDataDecl is an uninitialized array.
		void buildArrayUninit(ASTDataDecl& host, OpcodeContext& context);
		// For when ASTStringLiteral is a declaration initializer.
		void stringLiteralDeclaration(
				ASTStringLiteral& host, OpcodeContext& context);
		// For when ASTStringLiteral is not a declaration initializer.
		void stringLiteralFree(ASTStringLiteral& host, OpcodeContext& context);
		// For when ASTArrayLiteral is a declaration initializer.
		void arrayLiteralDeclaration(
				ASTArrayLiteral& host, OpcodeContext& context);
		// For when ASTArrayLiteral is not a declaration initializer.
		void arrayLiteralFree(ASTArrayLiteral& host, OpcodeContext& context);
		
		void parseExprs(ASTExpr* left, ASTExpr* right, void* param, bool orderMatters = false);
		void compareExprs(ASTExpr* left, ASTExpr* right, void* param);
		
		void buildPreOp(ASTExpr* operand, void* param, vector<std::shared_ptr<Opcode>> const& ops);
		void buildPostOp(ASTExpr* operand, void* param, vector<std::shared_ptr<Opcode>> const& ops);
		
		void push_param(bool varg = false);
		optional<int> eatSetCompare();
		optional<bool> rec_booltree_shortcircuit(BoolTreeNode& node, int parentMode, int truelbl, int falselbl, void* param);
		void rec_booltree_noshort(BoolTreeNode& node, int parentMode, void* param);
	};

	class LValBOHelper : public ASTVisitor
	{
	public:
		LValBOHelper(Scope* scope);
		LValBOHelper(BuildOpcodes* bo);
		virtual void caseDefault(void *param);
		//virtual void caseDataDecl(ASTDataDecl& host, void* param);
		virtual void caseExprIdentifier(ASTExprIdentifier &host, void *param);
		virtual void caseExprArrow(ASTExprArrow &host, void *param);
		virtual void caseExprIndex(ASTExprIndex &host, void *param);
		std::vector<std::shared_ptr<Opcode>> const& getResult() {return result;}
	private:
		void addOpcode(Opcode* code);
		void addOpcode(std::shared_ptr<Opcode>& code);

		template <class Container>
		void addOpcodes(Container const& container);
	
		std::vector<std::shared_ptr<Opcode>> result;
	};
	
	class CleanupVisitor : public RecursiveVisitor
	{
	public:
		using RecursiveVisitor::visit;
		void visit(AST& node, void* param = NULL);
		ASTExprBoolTree* booltree;
		BoolTreeNode* active_node;
		CleanupVisitor(Scope* curScope) : CleanupVisitor() {scope = curScope;}
	private:
		CleanupVisitor() = default;
	};


	class GetLabels : public ArgumentVisitor
	{
	public:
		GetLabels(std::set<int32_t>& usedLabels) : usedLabels(usedLabels) {}
		
		std::set<int32_t>& usedLabels;
		std::set<int32_t> newLabels;
		
		void caseLabel(LabelArgument& host, void*)
		{
			int32_t id = host.getID();
			if (find<int32_t>(usedLabels, id)) return;
			usedLabels.insert(id);
			newLabels.insert(id);
		}
	};

	class SetLabels : public ArgumentVisitor
	{
	public:
		bool err = false;
		void caseLabel(LabelArgument &host, void *param)
		{
			host.setLineNo(check(host.getID(), *(map<int32_t, int32_t> *)param));
		}
		static int check(int lbl, map<int32_t, int32_t> const& labels, bool* error = nullptr)
		{
			auto it = labels.find(lbl);

			if (it == labels.end())
			{
				zconsole_error("Internal error: couldn't find label %d", lbl);
				if (error)
					*error = true;
				return 0;
			}
			
			return it->second;
		}
	};
	class MergeLabels : public ArgumentVisitor
	{
		vector<int> labels;
		int targ_label;
		MergeLabels(int targ_label, vector<int> const& labels)
			: labels(labels),targ_label(targ_label) {}
	public:
		void caseLabel(LabelArgument &host, void *param)
		{
			for(int lbl : labels)
				if(lbl == host.getID())
				{
					host.setID(targ_label);
					return;
				}
		}
		static void merge(int targ_label, vector<int> const& labels, vector<std::shared_ptr<Opcode>> const& vec, void* param, vector<int32_t*> const* lblvec)
		{
			MergeLabels ml(targ_label, labels);
			ml.execute(vec, param);
			if(lblvec)
				for(int32_t* ptr : *lblvec)
				{
					for(int lbl : labels)
						if(lbl == *ptr)
						{
							*ptr = targ_label;
							break;
						}
				}
		}
	};
}
#endif

