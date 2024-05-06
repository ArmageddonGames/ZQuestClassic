#ifndef ZSCR_DATA_H
#define ZSCR_DATA_H

#include "base/zdefs.h"
#include "zconsole/ConsoleLogger.h"
#include "parser/Compiler.h"
#include <sstream>

using ZScript::disassembled_script_data;

#ifndef IS_PARSER
#include "zq/zquest.h"
#endif //!IS_PARSER

#define ZC_CONSOLE_INFO_CODE -9998
#define ZC_CONSOLE_ERROR_CODE -9997
#define ZC_CONSOLE_WARN_CODE -9996
#define ZC_CONSOLE_DB_CODE -9995
#define ZC_CONSOLE_TERM_CODE -9994
#define ZC_CONSOLE_IDLE_CODE -9993

using std::map;
using std::string;
using std::vector;

namespace ZScript
{
	enum ScriptTypeID
	{
		scrTypeIdInvalid = ZScript::ParserScriptType::idInvalid,
		scrTypeIdStart,
		scrTypeIdGlobal = scrTypeIdStart,
		scrTypeIdFfc,
		scrTypeIdItem,
		scrTypeIdNPC,
		scrTypeIdEWeapon,
		scrTypeIdLWeapon,
		scrTypeIdPlayer,
		scrTypeIdScreen,
		scrTypeIdDMap,
		scrTypeIdItemSprite,
		scrTypeIdUntyped,
		scrTypeIdComboData,
		scrTypeIdSusbcrData,
		scrTypeIdGeneric,
		
		scrTypeIdEnd
	};
}

void write_str(std::string const& str, FILE* f)
{
	size_t sz = str.size();
	fwrite(&sz, sizeof(size_t), 1, f);
	for(size_t q = 0; q < sz; ++q)
	{
		fputc(str.at(q), f);
	}
}
void read_str(std::string& str, FILE* f)
{
	size_t sz;
	fread(&sz, sizeof(size_t), 1, f);
	str.clear();
	for(size_t q = 0; q < sz; ++q)
	{
		str.push_back(fgetc(f));
	}
}
void write_w(word val, FILE* f)
{
	fwrite(&val, sizeof(word), 1, f);
}
void read_w(word &val, FILE* f)
{
	fread(&val, sizeof(word), 1, f);
}
void write_b(byte val, FILE* f)
{
	fwrite(&val, sizeof(byte), 1, f);
}
void read_b(byte &val, FILE* f)
{
	fread(&val, sizeof(byte), 1, f);
}

void write_meta(zasm_meta const& meta, FILE* f)
{
	write_w(meta.zasm_v, f);
	write_w(meta.meta_v, f);
	write_w(meta.ffscript_v, f);
	write_b((byte)meta.script_type, f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.run_idens[q], f);
	for(auto q = 0; q < 8; ++q)
		write_b(meta.run_types[q], f);
	write_b(meta.flags, f);
	write_w(meta.compiler_v1, f);
	write_w(meta.compiler_v2, f);
	write_w(meta.compiler_v3, f);
	write_w(meta.compiler_v4, f);
	write_str(meta.script_name, f);
	write_str(meta.author, f);
	for(auto q = 0; q < 10; ++q)
		write_str(meta.attributes[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.attribytes[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.attrishorts[q], f);
	for(auto q = 0; q < 16; ++q)
		write_str(meta.usrflags[q], f);
	for(auto q = 0; q < 10; ++q)
		write_str(meta.attributes_help[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.attribytes_help[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.attrishorts_help[q], f);
	for(auto q = 0; q < 16; ++q)
		write_str(meta.usrflags_help[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.initd[q], f);
	for(auto q = 0; q < 8; ++q)
		write_str(meta.initd_help[q], f);
	for(auto q = 0; q < 8; ++q)
		write_b(meta.initd_type[q], f);
}

void read_meta(zasm_meta& meta, FILE* f)
{
	read_w(meta.zasm_v, f);
	read_w(meta.meta_v, f);
	read_w(meta.ffscript_v, f);
	{
		byte st = (byte)meta.script_type;
		read_b(st, f);
		meta.script_type = (ScriptType)st;
	}
	for(auto q = 0; q < 8; ++q)
		read_str(meta.run_idens[q], f);
	for(auto q = 0; q < 8; ++q)
		read_b(meta.run_types[q], f);
	read_b(meta.flags, f);
	read_w(meta.compiler_v1, f);
	read_w(meta.compiler_v2, f);
	read_w(meta.compiler_v3, f);
	read_w(meta.compiler_v4, f);
	read_str(meta.script_name, f);
	read_str(meta.author, f);
	for(auto q = 0; q < 10; ++q)
		read_str(meta.attributes[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.attribytes[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.attrishorts[q], f);
	for(auto q = 0; q < 16; ++q)
		read_str(meta.usrflags[q], f);
	for(auto q = 0; q < 10; ++q)
		read_str(meta.attributes_help[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.attribytes_help[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.attrishorts_help[q], f);
	for(auto q = 0; q < 16; ++q)
		read_str(meta.usrflags_help[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.initd[q], f);
	for(auto q = 0; q < 8; ++q)
		read_str(meta.initd_help[q], f);
	for(auto q = 0; q < 8; ++q)
		read_b((byte&)meta.initd_type[q], f);
}

void read_compile_data(vector<shared_ptr<ZScript::Opcode>>& zasm, map<string, ZScript::ScriptTypeID>& stypes, map<string, disassembled_script_data>& scripts)
{
	stypes.clear();
	scripts.clear();
	size_t stypes_sz, scripts_sz;
	size_t dummy;
	ZScript::ScriptTypeID _id;
	char buf[512] = {0};
	char* buf2 = nullptr;
	size_t buf2sz = 0;
	char* buf3 = nullptr;
	size_t buf3sz = 0;
	
	FILE *tempfile = fopen("tmp2","rb");
			
	if(!tempfile)
	{
		//jwin_alert("Error","Unable to open the temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
		return;
	}
	
	fread(&stypes_sz, sizeof(size_t), 1, tempfile);
	for(size_t ind = 0; ind < stypes_sz; ++ind)
	{
		fread(&dummy, sizeof(size_t), 1, tempfile);
		dummy = fread(buf, sizeof(char), dummy, tempfile);
		buf[dummy] = 0;
		fread(&_id, sizeof(ZScript::ScriptTypeID), 1, tempfile);
		stypes[buf] = _id;
	}
	
	fread(&scripts_sz, sizeof(size_t), 1, tempfile);
	for(size_t ind = 0; ind < scripts_sz; ++ind)
	{
		fread(&dummy, sizeof(size_t), 1, tempfile);

		dummy = fread(buf, sizeof(char), dummy, tempfile);
		buf[dummy] = 0;
		
		disassembled_script_data dsd;
		
		read_meta(dsd.meta, tempfile);
		
		fread(&(dsd.format), sizeof(byte), 1, tempfile);
		
		fread(&(dsd.pc), sizeof(int32_t), 1, tempfile);
		fread(&(dsd.end_pc), sizeof(int32_t), 1, tempfile);
		
		scripts[buf] = dsd;
	}
	
	size_t zasm_sz;
	fread(&zasm_sz, sizeof(size_t), 1, tempfile);
	for(size_t ind2 = 0; ind2 < zasm_sz; ++ind2)
	{
		//read opcode into buf2
		fread(&dummy, sizeof(size_t), 1, tempfile);
		if (buf2sz < dummy + 1)
		{
			if (buf2) free(buf2);
			buf2sz = zc_max(dummy + 1, 1024);
			buf2 = (char*)malloc(buf2sz);
			if (!buf2)
			{
				buf2sz = 0;
				goto read_compile_error;
			}
		}
		dummy = fread(buf2, sizeof(char), dummy, tempfile);
		if (dummy >= buf2sz)
			dummy = buf2sz - 1; //This indicates an error, and shouldn't be reached...
		buf2[dummy] = 0;
		
		//read comment into buf3
		fread(&dummy, sizeof(size_t), 1, tempfile);
		if (buf3sz < dummy + 1)
		{
			if (buf3) free(buf3);
			buf3sz = zc_max(dummy + 1, 1024);
			buf3 = (char*)malloc(buf3sz);
			if (!buf3)
			{
				buf3sz = 0;
				goto read_compile_error;
			}
		}
		dummy = fread(buf3, sizeof(char), dummy, tempfile);
		if (dummy >= buf3sz)
			dummy = buf3sz - 1; //This indicates an error, and shouldn't be reached...
		buf3[dummy] = 0;
		
		int32_t lbl;
		fread(&lbl, sizeof(int32_t), 1, tempfile);
		std::shared_ptr<ZScript::Opcode> oc = std::make_shared<ZScript::ArbitraryOpcode>(buf2);
		oc->setLabel(lbl);
		oc->setComment(buf3);
		zasm.push_back(oc);
	}

read_compile_error:
	fclose(tempfile);
	
	if (buf2) free(buf2);
}

void write_compile_data(vector<shared_ptr<ZScript::Opcode>>& zasm, map<string, ZScript::ScriptTypeID>& stypes, map<string, disassembled_script_data>& scripts)
{
	size_t dummy = stypes.size();
	FILE *tempfile = fopen("tmp2","wb");
			
	if(!tempfile)
	{
		//jwin_alert("Error","Unable to create a temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
		return;
	}
	
	fwrite(&dummy, sizeof(size_t), 1, tempfile);
	for(auto it = stypes.begin(); it != stypes.end(); ++it)
	{
		string const& str = it->first;
		ZScript::ScriptTypeID v = it->second;
		dummy = str.size();
		fwrite(&dummy, sizeof(size_t), 1, tempfile);
		fwrite((void*)str.c_str(), sizeof(char), dummy, tempfile);
		fwrite(&v, sizeof(ZScript::ScriptTypeID), 1, tempfile);
	}
	
	dummy = scripts.size();
	fwrite(&dummy, sizeof(size_t), 1, tempfile);
	for(auto it = scripts.begin(); it != scripts.end(); ++it)
	{
		string const& str = it->first;
		disassembled_script_data& v = it->second;
		dummy = str.size();
		fwrite(&dummy, sizeof(size_t), 1, tempfile);
		fwrite((void*)str.c_str(), sizeof(char), dummy, tempfile);
		
		write_meta(v.meta, tempfile);
		
		fwrite(&(v.format), sizeof(byte), 1, tempfile);
		
		fwrite(&(v.pc), sizeof(int32_t), 1, tempfile);
		fwrite(&(v.end_pc), sizeof(int32_t), 1, tempfile);
	}
	
	dummy = zasm.size();
	fwrite(&dummy, sizeof(size_t), 1, tempfile);
	for(auto it = zasm.begin(); it != zasm.end(); ++it)
	{
		string opstr = (*it)->toString();
		string const& commentstr = (*it)->getComment();
		int32_t lbl = (*it)->getLabel();
		
		dummy = opstr.size();
		fwrite(&dummy, sizeof(size_t), 1, tempfile);
		fwrite((void*)opstr.c_str(), sizeof(char), dummy, tempfile);
		
		dummy = commentstr.size();
		fwrite(&dummy, sizeof(size_t), 1, tempfile);
		fwrite((void*)commentstr.c_str(), sizeof(char), dummy, tempfile);
		
		fwrite(&lbl, sizeof(int32_t), 1, tempfile);
	}
	
	fclose(tempfile);
}

void write_script(vector<shared_ptr<ZScript::Opcode>> const& zasm, string& dest,
	bool commented, map<string,disassembled_script_data>* scr_meta_map)
{
	std::ostringstream output;
	string str;
	map<uint,string> meta_map;
	if(scr_meta_map)
	{
		for(auto& pair : *scr_meta_map)
			meta_map[pair.second.pc] = pair.second.meta.get_meta();
	}
	for(uint q = 0; q < zasm.size(); ++q)
	{
		auto it = meta_map.find(q);
		if(it != meta_map.end())
			output << it->second;
		ZScript::Opcode* line = zasm[q].get();
		str = line->printLine(false, commented);
		output << str;
	}
	dest += output.str();
}

#ifndef IS_PARSER

CConsoleLoggerEx parser_console;

static const int32_t DB_COLOR = CConsoleLoggerEx::COLOR_RED | CConsoleLoggerEx::COLOR_BLUE | CConsoleLoggerEx::COLOR_INTENSITY;
static const int32_t WARN_COLOR = CConsoleLoggerEx::COLOR_RED | CConsoleLoggerEx::COLOR_GREEN;
static const int32_t ERR_COLOR = CConsoleLoggerEx::COLOR_RED;
static const int32_t INFO_COLOR = CConsoleLoggerEx::COLOR_WHITE;

void _print_zconsole(const char *buf, char const* header, int32_t color)
{
	if (!DisableCompileConsole)
	{
		int32_t v = parser_console.safeprint( color,header);
		if(v < 0) return; //Failed to print
	}
	
	safe_al_trace(buf);
	safe_al_trace("\n");
	if (!DisableCompileConsole)
	{
		parser_console.safeprint( color, buf);
		parser_console.safeprint( color, "\n");
	}
	else
	{
		box_out(buf);
		box_eol();
	}
}

void ReadConsole(char buf[], int code)
{
	switch(code)
	{
		case ZC_CONSOLE_IDLE_CODE: break;
		case ZC_CONSOLE_DB_CODE: _print_zconsole(buf,"[Debug] ",DB_COLOR); break;
		case ZC_CONSOLE_WARN_CODE: _print_zconsole(buf,"[Warn] ",WARN_COLOR); break;
		case ZC_CONSOLE_ERROR_CODE: _print_zconsole(buf,"[Error] ",ERR_COLOR); break;
		default: _print_zconsole(buf,"[Info] ",INFO_COLOR); break;
	}
}
#endif //!IS_PARSER

#ifdef IS_PARSER
#include "parser/Compiler.h"
void write_compile_data(vector<shared_ptr<ZScript::Opcode>>& zasm, map<string, ZScript::ParserScriptType>& stypes, map<string, disassembled_script_data>& scripts)
{
	map<string, ZScript::ScriptTypeID> sid_types;
	for(auto it = stypes.begin(); it != stypes.end(); ++it)
	{
		sid_types[it->first] = (ZScript::ScriptTypeID)(it->second.getId());
	}
	write_compile_data(zasm, sid_types, scripts);
}

#endif //IS_PARSER


ScriptType get_script_type(string const& name)
{
	if(name=="GLOBAL")
		return ScriptType::Global;
	else if(name=="FFC")
		return ScriptType::FFC;
	else if(name=="SCREEN")
		return ScriptType::Screen;
	else if(name=="HERO" || name=="PLAYER" || name=="LINK")
		return ScriptType::Player;
	else if(name=="ITEMDATA" || name=="ITEM")
		return ScriptType::Item;
	else if(name=="LWEAPON" || name=="LWPN")
		return ScriptType::Lwpn;
	else if(name=="NPC")
		return ScriptType::NPC;
	else if(name=="EWEAPON" || name=="EWPN")
		return ScriptType::Ewpn;
	else if(name=="DMAP")
		return ScriptType::DMap;
	else if(name=="ITEMSPRITE")
		return ScriptType::ItemSprite;
	else if(name=="COMBO" || name=="COMBODATA")
		return ScriptType::Combo;
	
	return ScriptType::None;
}

string get_script_name(ScriptType type)
{
	switch(type)
	{
		case ScriptType::Global:
			return "GLOBAL";
		case ScriptType::FFC:
			return "FFC";
		case ScriptType::Screen:
			return "SCREEN";
		case ScriptType::Player:
			return "HERO";
		case ScriptType::Item:
			return "ITEMDATA";
		case ScriptType::Lwpn:
			return "LWEAPON";
		case ScriptType::NPC:
			return "NPC";
		case ScriptType::Ewpn:
			return "EWEAPON";
		case ScriptType::DMap:
			return "DMAP";
		case ScriptType::ItemSprite:
			return "ITEMSPRITE";
		case ScriptType::Combo:
			return "COMBODATA";
		case ScriptType::Generic: case ScriptType::GenericFrozen:
			return "GENERIC";
		case ScriptType::EngineSubscreen:
			return "SUBSCREEN";
		case ScriptType::None:
		default:
			return "UNKNOWN";
	}
}

//Output metadata as a single string
string zasm_meta::get_meta() const
{
	std::ostringstream oss;
	oss << "#ZASM_VERSION = " << zasm_v
		<< "\n#METADATA_VERSION = " << meta_v
		<< "\n#FFSCRIPT_VERSION = " << ffscript_v
		<< "\n#SCRIPT_NAME = " << script_name;
	if(author.size())
		oss << "\n#AUTHOR = " << author;
	oss << "\n#SCRIPT_TYPE = " << get_script_name(script_type).c_str()
		<< "\n#AUTO_GEN = " << ((flags & ZMETA_AUTOGEN) ? "TRUE" : "FALSE")
		<< "\n#COMPILER_V1 = " << compiler_v1
		<< "\n#COMPILER_V2 = " << compiler_v2
		<< "\n#COMPILER_V3 = " << compiler_v3
		<< "\n#COMPILER_V4 = " << compiler_v4;
	for(auto q = 0; q < 8; ++q)
	{
		if(!run_idens[q].size())
			continue;
		oss << "\n#PARAM_TYPE_" << q << " = " << ZScript::getDataTypeName(run_types[q])
			<< "\n#PARAM_NAME_" << q << " = " << run_idens[q];
	}
	for(auto q = 0; q < 10; ++q)
	{
		if(attributes[q].size())
			oss << "\n#ATTRIBUTE_" << q << " = " << attributes[q];
		if(attributes_help[q].size())
			oss << "\n#ATTRIBUTE_HELP_" << q << " = "
				<< util::escape_characters(attributes_help[q]);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(attribytes[q].size())
			oss << "\n#ATTRIBYTE_" << q << " = " << attribytes[q];
		if(attribytes_help[q].size())
			oss << "\n#ATTRIBYTE_HELP_" << q << " = "
				<< util::escape_characters(attribytes_help[q]);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(attrishorts[q].size())
			oss << "\n#ATTRISHORT_" << q << " = " << attrishorts[q];
		if(attrishorts_help[q].size())
			oss << "\n#ATTRISHORT_HELP_" << q << " = "
				<< util::escape_characters(attrishorts_help[q]);
	}
	for(auto q = 0; q < 16; ++q)
	{
		if(usrflags[q].size())
			oss << "\n#FLAG_" << q << " = " << usrflags[q];
		if(usrflags_help[q].size())
			oss << "\n#FLAG_HELP_" << q << " = "
				<< util::escape_characters(usrflags_help[q]);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(initd[q].size())
			oss << "\n#INITD_" << q << " = " << initd[q];
		if(initd_help[q].size())
			oss << "\n#INITD_HELP_" << q << " = "
				<< util::escape_characters(initd_help[q]);
		if(initd_type[q] > -1)
			oss << "\n#INITD_TYPE_" << q << " = "
				<< to_string(int32_t(initd_type[q]));
	}
	oss << "\n";
	return oss.str();
}

//Parse a single line of metadata
bool zasm_meta::parse_meta(const char *buffer)
{
	string line(buffer);
	size_t space_pos = line.find_first_of(" \t=");
	if(space_pos == string::npos) return false;
	string cmd = line.substr(0, space_pos); //The command portion
	size_t end_space_pos = line.find_first_not_of(" \t=", space_pos);
	if(end_space_pos == string::npos) return false;
	string val = line.substr(end_space_pos); //The value portion
	size_t endpos = val.find_last_not_of(" \t\r\n\0");
	if(endpos != string::npos) ++endpos;
	val = val.substr(0, endpos); //trim trailing whitespace
	
	if(cmd == "#ZASM_VERSION")
	{
		zasm_v = atoi(val.c_str());
	}
	else if(cmd == "#METADATA_VERSION")
	{
		meta_v = atoi(val.c_str());
	}
	else if(cmd == "#FFSCRIPT_VERSION")
	{
		ffscript_v = atoi(val.c_str());
	}
	else if(cmd == "#SCRIPT_TYPE" || cmd == "#TYPE")
	{
		upperstr(val);
		script_type = get_script_type(val);
	}
	else if(cmd == "#SCRIPT_NAME")
	{
		replchar(val, ' ', '_');
		script_name = val;
	}
	else if(cmd == "#AUTHOR")
	{
		author = val;
	}
	else if(cmd == "#AUTO_GEN")
	{
		upperstr(val);
		if(val=="TRUE")
			flags |= ZMETA_AUTOGEN;
		else if(val=="FALSE")
			flags &= ~ZMETA_AUTOGEN;
		else
		{
			if(atoi(val.c_str())!=0)
			{
				flags |= ZMETA_AUTOGEN;
			}
			else flags &= ~ZMETA_AUTOGEN;
		}
	}
	else if(cmd == "#COMPILER_V1")
	{
		compiler_v1 = atoi(val.c_str());
	}
	else if(cmd == "#COMPILER_V2")
	{
		compiler_v2 = atoi(val.c_str());
	}
	else if(cmd == "#COMPILER_V3")
	{
		compiler_v3 = atoi(val.c_str());
	}
	else if(cmd == "#COMPILER_V4")
	{
		compiler_v4 = atoi(val.c_str());
	}
	else if(cmd.size() == 13 && !cmd.compare(0,12,"#PARAM_NAME_"))
	{
		byte ind = cmd.at(12) - '1';
		if (ind < 8)
		{
			replchar(val, ' ', '_');
			run_idens[ind] = val;
		}
		else return false;
	}
	else if(cmd.size() == 13 && !cmd.compare(0,12,"#PARAM_TYPE_"))
	{
		byte ind = cmd.at(12) - '1';
		if (ind < 8)
		{
			replchar(val, ' ', '_');
			run_types[ind] = ZScript::getTypeId(val);
		}
		else return false;
	}
	else if (cmd.size() == 12 && !cmd.compare(0, 11, "#ATTRIBUTE_"))
	{
		byte ind = cmd.at(11) - '0';
		if (ind < 10)
		{
			attributes[ind] = val;
		}
		else return false;
	}
	else if (cmd.size() == 17 && !cmd.compare(0, 16, "#ATTRIBUTE_HELP_"))
	{
		byte ind = cmd.at(16) - '0';
		if (ind < 10)
		{
			attributes_help[ind] = util::unescape_characters(val);
		}
		else return false;
	}
	else if (cmd.size() == 12 && !cmd.compare(0, 11, "#ATTRIBYTE_"))
	{
		byte ind = cmd.at(11) - '0';
		if (ind < 8)
		{
			attribytes[ind] = val;
		}
		else return false;
	}
	else if (cmd.size() == 17 && !cmd.compare(0, 16, "#ATTRIBYTE_HELP_"))
	{
		byte ind = cmd.at(16) - '0';
		if (ind < 8)
		{
			attribytes_help[ind] = util::unescape_characters(val);
		}
		else return false;
	}
	else if (cmd.size() == 13 && !cmd.compare(0, 12, "#ATTRISHORT_"))
	{
		byte ind = cmd.at(12) - '0';
		if (ind < 8)
		{
			attrishorts[ind] = val;
		}
		else return false;
	}
	else if (cmd.size() == 18 && !cmd.compare(0, 17, "#ATTRISHORT_HELP_"))
	{
		byte ind = cmd.at(17) - '0';
		if (ind < 8)
		{
			attrishorts_help[ind] = util::unescape_characters(val);
		}
		else return false;
	}
	else if ((cmd.size() == 7 || cmd.size() == 8) && !cmd.compare(0, 6, "#FLAG_"))
	{
		byte ind = cmd.at(6) - '0';
		if (cmd.size() == 8)
			ind = (ind * 10) + cmd.at(7) - '0';
		if (ind < 16)
		{
			usrflags[ind] = val;
		}
		else return false;
	}
	else if ((cmd.size() == 12 || cmd.size() == 13) && !cmd.compare(0, 11, "#FLAG_HELP_"))
	{
		byte ind = cmd.at(11) - '0';
		if (cmd.size() == 13)
			ind = (ind * 10) + cmd.at(12) - '0';
		if (ind < 16)
		{
			usrflags_help[ind] = util::unescape_characters(val);
		}
		else return false;
	}
	else if (cmd.size() == 8 && !cmd.compare(0, 7, "#INITD_"))
	{
		byte ind = cmd.at(7) - '0';
		if (ind < 8)
		{
			initd[ind] = val;
		}
		else return false;
	}
	else if (cmd.size() == 13 && !cmd.compare(0, 12, "#INITD_HELP_"))
	{
		byte ind = cmd.at(12) - '0';
		if (ind < 8)
		{
			initd_help[ind] = util::unescape_characters(val);
		}
		else return false;
	}
	else if (cmd.size() == 13 && !cmd.compare(0, 12, "#INITD_TYPE_"))
	{
		byte ind = cmd.at(12) - '0';
		if (ind < 8)
		{
			initd_type[ind] = atoi(val.c_str());
		}
		else return false;
	}
	else return false;
	
	return true;
}

#endif

