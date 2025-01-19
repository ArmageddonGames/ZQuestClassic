#include "parser/CommentUtils.h"
#include "parser/AST.h"
#include "parser/Scope.h"
#include "parser/ZScript.h"
#include <regex>
#include <string>
#include <vector>

using namespace ZScript;

static std::map<const AST*, uint16_t> symbolMap;
static uint16_t nextSymbolId = 1;

uint16_t getSymbolId(const AST* node)
{
	if (auto n = dynamic_cast<const ASTCustomDataTypeDef*>(node))
		return getSymbolId(n->definition.get());

	auto it = symbolMap.find(node);
	if (it != symbolMap.end())
		return it->second;

	return symbolMap[node] = nextSymbolId++;
}

static ASTExprIdentifier parseExprIdentifier(const std::string& str)
{
	ASTExprIdentifier ident{};
	int i = 0;
	int j = 0;
	while (j < str.size())
	{
		if (j + 1 < str.size())
		{
			bool matches = false;
			matches |= str[j] == ':' && str[j + 1] == ':';
			matches |= str[j] == '-' && str[j + 1] == '>';
			if (matches)
			{
				ident.components.push_back(str.substr(i, j));
				ident.delimiters.push_back(str.substr(j, 2));
				j += 2;
				i = j;
				continue;
			}
		}

		j++;
		if (j == str.size())
			ident.components.push_back(str.substr(i));
	}

	return ident;
}

std::vector<ParseCommentResult> parseForSymbolLinks(std::string comment, const AST* node, bool check_params)
{
	Scope* scope = scope = node->getScope();
	if (!scope)
		return {};

	// identifier, followed by an optional and non-captured "[]" or "()"
	static std::string p_ident = "(#?[a-zA-Z_][->:a-zA-Z0-9_]*)(?:\\[\\]|\\(\\))?";
	static std::string p_link = fmt::format("\\{{@link {}(?:\\|([^}}]+))?\\}}", p_ident);
	static std::string p_shorthand = fmt::format("\\[{}(?:\\|([^\\]]+))?\\]", p_ident);
	static std::string p_regex = fmt::format("{}|{}", p_link, p_shorthand);
	// Supports:
	// @link {symbol}
	// @link {symbol|text}
	// [symbol]
	// [symbol|text]
	// 'symbol' is either an internal symbol, or an external docs target starting with '#'
	static const std::regex r(p_regex);
	std::sregex_iterator it(comment.begin(), comment.end(), r);
	std::sregex_iterator end;

	std::vector<ParseCommentResult> matches;
	while (it != end)
	{
		auto pos = (*it).position(0);
		auto len = (*it).length(0);
		std::string symbol_name, link_text;
		if ((*it)[3].matched)
		{
			symbol_name = (*it)[3].str();
			if((*it)[4].matched)
				link_text = (*it)[4].str();
		}
		else
		{
			symbol_name = (*it)[1].str();
			if((*it)[2].matched)
				link_text = (*it)[2].str();
		}
		matches.emplace_back(symbol_name, (int)pos, (int)len, link_text, nullptr);
		it++;
	}

	for (auto it = matches.begin(); it != matches.end(); it++)
	{
		auto& [symbol_name, pos, len, link_text, symbol_node] = *it;
		
		if(symbol_name.starts_with("#"))
			continue; // link to external docs, not a symbol

		bool had_link_text = !link_text.empty();
		if (!had_link_text)
			link_text = symbol_name;

		ASTExprIdentifier ident;
		bool is_array = false;
		bool is_fn = false;
		auto fn = lookupGetter(*scope, symbol_name);
		if (!fn)
			fn = lookupSetter(*scope, symbol_name);
		if (!fn)
		{
			// TODO: possibly support param types for overloaded functions: `CenterX(npc, bool)`
			ident = parseExprIdentifier(symbol_name);
			auto fns = lookupFunctions(*scope, ident.components, ident.delimiters, {}, false, false, true);
			if (!fns.empty())
				fn = fns[0];
		}
		if (fn && fn->node)
		{
			symbol_node = fn->node;
			is_fn = true;
		}
		else
		{
			ident = parseExprIdentifier(symbol_name);
		}

		// Parameter?
		bool was_param = false;
		if (!symbol_node)
		{
			if (auto fn_node = dynamic_cast<const ASTFuncDecl*>(node))
			{
				for (auto param_node : fn_node->parameters.data())
				{
					if (param_node->getName() == symbol_name)
					{
						symbol_node = param_node;
						was_param = true;
						break;
					}
				}
			}
		}
		if (!check_params && was_param)
		{
			// Linkifying parameters is not useful for docs generation, and breaks stuff. So ignore.
			symbol_node = nullptr;
			continue;
		}

		if (!symbol_node)
		{
			if (auto datum = lookupDatum(*scope, ident, nullptr))
			{
				symbol_node = datum->getNode();
				is_array = datum->type.isArray();
			}
		}
		if (!symbol_node)
		{
			if (auto datum = lookupClassVars(*scope, ident, nullptr))
			{
				symbol_node = datum->getNode();
				is_array = datum->type.isArray();
			}
		}
		if (!symbol_node)
		{
			if (auto type = lookupDataType(*scope, ident, nullptr))
			{
				if (auto custom_type = dynamic_cast<const DataTypeCustom*>(type); custom_type)
					symbol_node = custom_type->getSource();
			}
		}
		// Resolve class fields, like [itemdata::Counter]
		if (!symbol_node && ident.components.size()	== 2 && ident.delimiters[0] == "::")
		{
			std::string prop = ident.components[1];
			ident.delimiters.pop_back();
			ident.components.pop_back();
			if (auto type = lookupDataType(*scope, ident, nullptr))
			{
				ident.components[0] = prop;
				if (auto datum = lookupClassVars(type->getUsrClass()->getScope(), ident, nullptr))
				{
					symbol_node = datum->getNode();
					is_array = datum->type.isArray();
				}
			}
		}
		// Resolve variable fields, like [Game->Counter]
		if (!symbol_node && ident.components.size()	== 2 && ident.delimiters[0] == "->")
		{
			std::string prop = ident.components[1];
			ident.delimiters.pop_back();
			ident.components.pop_back();
			if (auto datum = lookupDatum(*scope, ident, nullptr))
			{
				ident.components[0] = prop;
				if (auto field = lookupClassVars(datum->type.getUsrClass()->getScope(), ident, nullptr))
				{
					symbol_node = field->getNode();
					is_array = field->type.isArray();
				}
				if (!symbol_node)
				{
					auto fns = lookupClassFuncs(*datum->type.getUsrClass(), ident.components[0], {}, &datum->type.getUsrClass()->getScope(), true);
					if (!fns.empty())
					{
						symbol_node = fns[0]->getNode();
						is_fn = true;
					}
				}
			}
		}
		// TODO: obviously, there's a lot of code smell above. Can we make a better `lookupSymbol` method?

		if (!had_link_text)
		{
			if (is_array)
				link_text += "[]";
			else if (is_fn)
				link_text += "()";
		}

		if (!symbol_node)
			logDebugMessage(fmt::format("could not resolve symbol \"{}\"", symbol_name).c_str());
	}

	return matches;
}
