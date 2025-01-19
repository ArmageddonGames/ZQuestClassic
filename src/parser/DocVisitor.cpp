#include "base/util.h"
#include "parser/AST.h"
#include "parser/CommentUtils.h"
#include "parserDefs.h"
#include "DocVisitor.h"
#include <cassert>
#include "Scope.h"
#include "CompileError.h"
#include <nlohmann/json.hpp>
#include <fmt/format.h>

using namespace ZScript;
using json = nlohmann::ordered_json;

static json root;
static json* active;

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#symbolKind
enum class SymbolKind
{
	File = 1,
	Module,
	Namespace,
	Package,
	Class,
	Method,
	Property,
	Field,
	Constructor,
	Enum,
	Interface,
	Function,
	Variable,
	Constant,
	String,
	Number,
	Boolean,
	Array,
	Object,
	Key,
	Null,
	EnumMember,
	Struct,
	Event,
	Operator,
	TypeParameter,
};

template <typename T>
static std::string getName(const T& node)
{
	return node.getName();
}
static std::string getName(const ASTDataEnum& node)
{
	if (!node.getIdentifierLocation())
		return "";

	return node.getName();
}

static void linkifyString(std::string& string, const AST* node)
{
	bool check_params = false;
	auto matches = parseForSymbolLinks(string, node, check_params);
	for (auto it = matches.rbegin(); it != matches.rend(); it++)
	{
		if (it->symbol_name.starts_with("#")) // external docs link
		{
			if (it->link_text.empty())
				string.replace(it->pos, it->len, fmt::format("[#{}#]", it->symbol_name.substr(1)));
			else
				string.replace(it->pos, it->len, fmt::format("[#{}|{}#]", it->symbol_name.substr(1), it->link_text));
			continue;
		}
		if (!it->symbol_node)
		{
			string.replace(it->pos, it->len, fmt::format("`{}`", it->link_text));
			continue;
		}

		if (it->link_text.empty())
			string.replace(it->pos, it->len, fmt::format("[@{}@]", getSymbolId(it->symbol_node)));
		else
			string.replace(it->pos, it->len, fmt::format("[@{}|{}@]", getSymbolId(it->symbol_node), it->link_text));
	}
}

static json getCommentJson(const AST* node)
{
	// TODO: the doc_comment isn't always in a consistent place. For example see `utils::hmm // ...` in `metadata.zh` 
	if (auto n = dynamic_cast<const ASTDataDecl*>(node); n && n->list && node->doc_comment.empty())
		node = n->list;
	else if (auto n = dynamic_cast<const ASTDataEnum*>(node); n)
		if (auto custom_type = dynamic_cast<const DataTypeCustom*>(n->baseType->type.get()); custom_type && custom_type->getSource())
			if (auto n = dynamic_cast<const ASTCustomDataTypeDef*>(custom_type->getSource()); n)
				node = n;

	if (node->doc_comment.empty())
		return nullptr;

	auto parsed_comment = node->getParsedDocComment();
	json result = json::object();

	result["tags"] = json::object();
	util::trimstr(parsed_comment[""]);
	for (auto [k, v] : parsed_comment)
	{
		if (k == "")
			continue;

		if (v.empty())
			result["tags"][k] = true;
		else
		{
			linkifyString(v, node);
			result["tags"][k] = v;
		}
	}

	std::string comment = parsed_comment[""];
	linkifyString(comment, node);
	result["text"] = comment;

	return result;
}

template <typename T>
static json* appendSymbol(SymbolKind kind, const T& node)
{
	std::optional<LocationData> loc;
	if (auto n = dynamic_cast<const ASTDataEnum*>(&node))
		loc = n->location;
	else
		loc = node.getIdentifierLocation();

	(*active).push_back({
		{"name", getName(node)},
		{"id", getSymbolId(&node)},
		{"kind", kind},
		{"comment", getCommentJson(&node)},
		{"children", json::array()},
	});
	json* result = &active->back();
	if (loc)
	{
		std::string fname = loc->fname;
#ifdef _WIN32
		util::replstr(fname, "\\", "/");
#endif
		(*result)["location"] = {
			{"file", fname},
			{"line", loc->first_line},
		};
	}
	return result;
}

DocVisitor::DocVisitor(Program& program) : RecursiveVisitor(program)
{
	root = {
		{"files", json::array()},
	};
	visit(program.getRoot());
}

void DocVisitor::visit(AST& node, void* param)
{
	RecursiveVisitor::visit(node, param);
}

void DocVisitor::caseFile(ASTFile& host, void* param)
{
	root["files"].push_back({
		{"name", host.location.fname},
		{"symbols", json::array()},
	});

	auto prev_active = active;
	active = &root["files"].back()["symbols"];
	RecursiveVisitor::caseFile(host, param);
	active = prev_active;
}

void DocVisitor::caseClass(ASTClass& host, void* param)
{
	auto symbol = appendSymbol(SymbolKind::Class, host);

	if (auto parent = host.user_class->getParentClass())
		(*symbol)["parent"] = parent->getName();

	auto prev_active = active;
	active = &(*symbol)["children"];
	RecursiveVisitor::caseClass(host, param);
	active = prev_active;
}

void DocVisitor::caseDataDecl(ASTDataDecl& host, void* param)
{
	auto symbol = appendSymbol(SymbolKind::Variable, host);
	(*symbol)["type"] = host.resolvedType->getName();
	if (auto init = host.getInitializer())
		if (auto value = init->getCompileTimeValue(nullptr, host.getScope()))
			(*symbol)["value"] = *value;
	RecursiveVisitor::caseDataDecl(host, param);
}

void DocVisitor::caseDataEnum(ASTDataEnum& host, void* param)
{
	auto symbol = appendSymbol(SymbolKind::Enum, host);
	DataType const* baseType = host.baseType->resolve_ornull(*scope, this);
	(*symbol)["type"] = baseType->isLong() ? "long" : "int";

	auto prev_active = active;
	active = &(*symbol)["children"];
	for (auto* decl : host.getDeclarations())
	{
		auto value = decl->getInitializer()->getCompileTimeValue(nullptr, decl->getScope());
		auto symbol = appendSymbol(SymbolKind::EnumMember, *decl);
		(*symbol)["type"] = decl->getInitializer()->getReadType(decl->getScope(), nullptr)->isLong() ? "long" : "int";
		(*symbol)["value"] = *value;
	}
	active = prev_active;
}

void DocVisitor::caseFuncDecl(ASTFuncDecl& host, void* param)
{
	if (host.prototype || host.getFlag(FUNCFLAG_NIL))
		return;

	auto kind = host.getFlag(FUNCFLAG_CONSTRUCTOR) ? SymbolKind::Constructor : SymbolKind::Function;
	auto symbol = appendSymbol(kind, host);
	(*symbol)["returnType"] = host.returnType->type->getName();
	(*symbol)["parameters"] = json::array();
	for (auto param : host.parameters)
	{
		json j_param = {
			{"name", param->getName()},
			{"type", param->resolvedType->getName()},
		};
		(*symbol)["parameters"].push_back(std::move(j_param));
	}
	for (int i = 0; i < host.optvals.size(); i++)
	{
		std::string snippet = getSourceCodeSnippet(host.optparams[i]->location);
		(*symbol)["parameters"][host.parameters.size() - host.optvals.size() + i]["default"] = (*symbol)["value"] = snippet;
	}
	if (host.getFlag(FUNCFLAG_VARARGS))
		(*symbol)["varargs"] = host.func->extra_vargs;
}

std::string DocVisitor::getOutput()
{
	return root.dump(2);
}
