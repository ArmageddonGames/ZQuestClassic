#ifndef COMPILER_H
#define COMPILER_H

#include "CompilerUtils.h"
#include "Types.h"
#include "parserDefs.h"
#include "base/headers.h"
#include <fmt/format.h>
#include "zq/ffasm.h"

#include <cstdio>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
using std::unique_ptr;

namespace ZScript
{
	////////////////////////////////////////////////////////////////
	// Forward Declarations

	// AST.h
	class ASTFile;
	class ASTImportDecl;
	class ASTIncludePath;

	// ByteCode.h
	class ArgumentVisitor;

	// ZScript.h
	class Program;
	class FunctionSignature;
	class Script;

	// DataStructs.h
	struct FunctionData;
	struct IntermediateData;
	
	////////////////////////////////////////////////////////////////
	
	class Opcode
	{
	public:
		Opcode() : label(-1) {}
		virtual ~Opcode() {}
		virtual std::string toString() const = 0;
		int getLabel() const
		{
			return label;
		}
		void setLabel(int l)
		{
			label = l;
		}
		string const& getComment() const
		{
			return comment;
		}
		void setComment(string const& str)
		{
			comment = str;
		}
		static void mergeComment(string& basestr, string const& str, bool before = false)
		{
			if(str.empty())
				return;
			if(basestr.empty())
			{
				basestr = str;
				return;
			}
			if(before)
				basestr = fmt::format("{} AND {}",str,basestr);
			else basestr = fmt::format("{} AND {}",basestr,str);
		}
		void mergeComment(string const& str, bool before = false)
		{
			Opcode::mergeComment(comment, str, before);
		}
		string printLine(bool showlabel = false, bool showcomment = true)
		{
			string labelstr = " ";
			if(showlabel && label > -1)
				labelstr = fmt::format("l{}:",label);
            string commentstr;
			if(showcomment && comment.size())
				commentstr = fmt::format("; {}",comment);
			return fmt::format("{}{}{}\n",labelstr,toString(),commentstr);
		}
		Opcode * makeClone(bool copylabel = true, bool copycomment = true)
		{
			Opcode *dup = clone();
			if(copylabel)
				dup->setLabel(label);
			if(copycomment)
				dup->setComment(comment);
			return dup;
		}
		virtual void execute(ArgumentVisitor&, void*) {}
	protected:
		virtual Opcode *clone() const = 0;
	private:
		int label;
		string comment;
	};

	class ArbitraryOpcode : public Opcode
	{
	public:
		ArbitraryOpcode(std::string data) : str(data) {}
		ArbitraryOpcode(char const* data) : str(data) {}
		std::string str;
		std::string toString() const
		{
			return str;
		}
		Opcode *clone() const
		{
			return new ArbitraryOpcode(str);
		}
	};

	struct disassembled_script_data
	{
		zasm_meta meta;
		uint32_t pc, end_pc;
		byte format;
		std::string formatName(std::string name)
		{
			return name;
		}
		disassembled_script_data() : format(SCRIPT_FORMAT_DEFAULT)
		{}
	};

	class ScriptAssembler
	{
	public:
		Program& program;
		
		ScriptAssembler(IntermediateData& id);
		void assemble();
		vector<shared_ptr<Opcode>>& getCode() {return rval;}
		map<Script*,std::pair<int32_t,int32_t>>& getLabelMap() {return runlabels;}
		bool assemble_err;
	private:
		vector<shared_ptr<Opcode>>& ginit;
		
		vector<shared_ptr<Opcode>> rval;
		map<Script*,std::pair<int32_t,int32_t>> runlabels;
		vector<int32_t*> runlbl_ptrs;
		
		void assemble_init();
		void assemble_script(Script* scr, vector<shared_ptr<Opcode>> runCode,
			int numparams, string const& runsig);
		void assemble_scripts();
		void gather_labels();
		void link_functions();
		void optimize_output();
		void finalize_labels();
	};
	
	class ScriptsData
	{
	public:
		ScriptsData(ScriptAssembler&);
		vector<shared_ptr<Opcode>> zasm;
		std::map<std::string, disassembled_script_data> theScripts;
		std::map<std::string, ParserScriptType> scriptTypes;
		std::string metadata;
	};

	unique_ptr<ScriptsData> compile(std::string const& filename, bool include_metadata);

	class ScriptParser
	{
	public:
		static int32_t getUniqueVarID()
		{
			return vid++;
		}
		static int32_t getUniqueFuncID()
		{
			return fid++;
		}
		static int32_t getUniqueLabelID()
		{
			return lid++;
		}
		static int32_t getUniqueGlobalID()
		{
			return gid++;
		}
		static bool preprocess_one(ASTImportDecl& decl, int32_t reclevel);
		static bool preprocess(ASTFile* root, int32_t reclevel);
		static unique_ptr<IntermediateData> generateOCode(FunctionData& fdata);
		static void initialize(bool has_qrs);
		static std::pair<int32_t,bool> parseLong(
				std::pair<std::string,std::string> parts, Scope* scope);

		static int32_t const recursionLimit = 30;
	private:
		static bool valid_include(ASTImportDecl& decl, std::string& ret_fname);
		static std::string prepareFilename(std::string const& filename);
		static std::string* checkIncludes(std::string& includePath, std::string const& importname, std::vector<std::string> includes);
		static std::vector<std::shared_ptr<Opcode>> assembleOne(
				Program& program, std::vector<std::shared_ptr<Opcode>> script,
				int32_t numparams, FunctionSignature const& runsig);
		static int32_t vid;
		static int32_t fid;
		static int32_t gid;
		static int32_t lid;
		static std::vector<std::string> includePaths;
	};
	
	class compile_exception : public std::exception
	{
	public:
		const char * what() const noexcept override {
			return msg.c_str();
		}
		compile_exception(std::string const& msg) : msg(msg)
		{}
	private:
		std::string msg;
	};
}

#endif

