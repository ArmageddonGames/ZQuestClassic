#include "parser/Types.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ctype.h>

#include "zasm/table.h"
#include "zc/ffscript.h"
#include "zq/ffasm.h"

#include "zq/zquest.h"
#include "base/headers.h"
#include "base/zsys.h"
#include "base/util.h"
#include "dialog/info.h"
using namespace util;

#include <sstream>
#include <fstream>

using std::string;
using std::ostringstream;

extern char *datapath, *temppath;

int32_t ffparse(char const* string)
{
	double negcheck = atof(string);
	
	//if no decimal point, just atoi
	char const* ptr1=strchr(string, '.');
	if(!ptr1)
	{
		return atoi(string)*10000;
	}
	
	int32_t ret=0;
	char *tempstring1;
	tempstring1=(char *)malloc(strlen(string)+5);
	strcpy(tempstring1, string);
	
	for(int32_t i=0; i<4; ++i)
	{
		tempstring1[strlen(string)+i]='0';
	}
	
	char* ptr=strchr(tempstring1, '.');
	*ptr=0;
	ret=atoi(tempstring1)*10000;
	
	++ptr;
	char *ptr2=ptr;
	ptr2+=4;
	*ptr2=0;
	
	if(negcheck<0)
		ret-=atoi(ptr);
	else ret+=atoi(ptr);
	
	if(tempstring1) //may be safer
		free(tempstring1);
	return ret;
}
bool ffcheck(char const* arg)
{

	for(int32_t i=0; i<0x100; i++)
	{
		if(arg[i]!='\0')
		{
			if(i==0)
			{
				if(arg[i]!='-' && arg[i]!='.' && !(arg[i] >= '0' && arg[i] <='9'))
					return false;
			}
			else
			{
				if(arg[i]!='.' && !(arg[i] >= '0' && arg[i] <='9'))
					return false;
			}
		}
		else
		{
			i=0x100;
		}
	}
	
	return true;
}

std::map<std::string, int32_t> labels;

#define SUBBUFSZ         0x200
#define ERR_INSTRUCTION  0
#define ERR_PARAM1       1
#define ERR_PARAM2       2
#define ERR_STR          3
#define ERR_VEC          4
static const char* errstrbuf[] =
{
	"invalid instruction!",
	"parameter 1 invalid!",
	"parameter 2 invalid!",
	"parameter 3 invalid!",
	"string parameter invalid!",
	"vector parameter invalid!"
};
int32_t parse_script_string(vector<ffscript>& zasm, string const& scriptstr, bool report_success)
{
	saved=false;
	string buffer;
	char combuf[SUBBUFSZ] = {0};
	char arg1buf[SUBBUFSZ] = {0};
	char arg2buf[SUBBUFSZ] = {0};
	char arg3buf[SUBBUFSZ] = {0};
	char *argbufs[] = {arg1buf, arg2buf, arg3buf};
	vector<int> arr_vec;
	string arr_str;
	bool has_vec = false;
	bool has_str = false;
	bool stop=false;
	bool success=true;
	int num_commands;
	char const* scrptr = scriptstr.c_str();
	
	for(int i=0;; i++)
	{
		buffer.clear();
		
		if(stop)
		{
			num_commands=i+1;
			break;
		}
		
		bool meta = false;
		bool running = true;
		while(running)
		{
			char c = *scrptr++;
			
			if(!c) //null terminator
			{
				stop = true;
				--scrptr;
				break;
			}
			else
			{
				buffer += c;
				if(buffer[0] == '#') //Metadata line
				{
					while(true)
					{
						char c2 = *scrptr++;
						if(c2 == '\n') break;
						if(!c2)
						{
							stop=true;
							break;
						}
					}
					--i;
					meta = true;
					break;
				}
				else if(c == ';' || c == '\n' || c == 13)
				{
					if(c != '\n')
					{
						while(true)
						{
							char c2 = *scrptr++;
							if(c2 == '\n') break;
							if(!c2)
							{
								stop=true;
								break;
							}
						}
					}
					buffer.pop_back();
					break;
				}
				else if (c == '"' || c == '{')
				{
					bool vec = c == '{';
					auto j = buffer.size()-1;
					bool bslash = false;
					while(true)
					{
						char c2 = *scrptr++;
						bslash = c2 == '\\';
						if(!c2) //null terminator
						{
							stop=true;
							--scrptr;
							running = false;
							break;
						}
						buffer += vec ? toupper(c2) : c2;
						if(vec)
						{
							if(c2 == '}')
							{
								size_t sz = vecstr_size(buffer.c_str()+j);
								if(sz)
								{
									j += sz;
									break;
								}
							}
						}
						else
						{
							if(!bslash && c2 == '"')
							{
								size_t sz = escstr_size(buffer.c_str()+j);
								if(sz)
								{
									j += sz;
									break;
								}
							}
						}
					}
				}
			}
		}
		if(meta) continue;
		
		int32_t k=0;
		
		while(buffer[k] == ' ' || buffer[k] == '\t') k++;
		
		if(buffer[k] == '\0')
		{
			--i;
			continue;
		}
		
		k=0;
		
		if(buffer[k] != ' ' && buffer[k] !='\t' && buffer[k] != '\0')
		{
			char lbuf[80] = {0};
			while(buffer[k] != ' ' && buffer[k] !='\t' && buffer[k] != '\0')
			{
				lbuf[k] = buffer[k];
				k++;
			}
			string lbl(lbuf);
			map<string,int32_t>::iterator it = labels.find(lbl);
			if(it != labels.end())
			{
				char buf[120],buf2[120],buf3[120];
				sprintf(buf,"Unable to parse instruction %d",i+1);
				sprintf(buf2,"The error was: Duplicate Label");
				sprintf(buf3,"The duplicate label was: \"%s\"",lbuf);
				jwin_alert("Error",buf,buf2,buf3,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				stop=true;
				success=false;
				goto zasmfile_fail_str;
			}
			labels[lbl] = i;
			while(buffer[k] == ' ' || buffer[k] == '\t')
			{
				++k;
			}
			if(buffer[k] == '\0')
			{
				--i; //No command on this line
				continue;
			}
		}
	}
	
	scrptr = scriptstr.c_str(); //reset to start
	stop = false;
	
	zasm.clear();
	zasm.reserve(num_commands);
	
	for(int32_t i=0; i<num_commands && !stop; ++i)
	{
		buffer.clear();
		combuf[0]=0;
		for(char* buf : argbufs)
			buf[0] = 0;
		bool meta_mode = false;
		arr_vec.clear();
		arr_str.clear();
		has_vec = false;
		has_str = false;
		
		bool running = true;
		while(running)
		{
			char c = *scrptr++;
			
			if(!c) //null terminator
			{
				stop=true;
				--scrptr;
				break;
			}
			else
			{
				c = toupper(c);
				buffer += c;
				if (buffer[0] == '#') //Metadata line
				{
					meta_mode = true;
					char temp;
					bool buffering = true;
					while ((temp = (*scrptr++)) != '\n')
					{
						if (!temp) //null terminator
						{
							stop = true;
							break;
						}
						if (temp == '\r')
							buffering = false;
						if(buffering)
							buffer += temp;
					}
					break;
				}
				else if (c == ';' || c == '\n' || c == '\r')
				{
					if (c != '\n')
					{
						while(true)
						{
							char c2 = *scrptr++;
							if(c2 == '\n') break;
							if(!c2) //null terminator
							{
								stop=true;
								break;
							}
						}
					}
					buffer.pop_back();
					break;
				}
				else if (c == '"' || c == '{')
				{
					bool vec = c == '{';
					auto j = buffer.size()-1;
					bool bslash = false;
					while(true)
					{
						char c2 = *scrptr++;
						bslash = c2 == '\\';
						if(!c2) //null terminator
						{
							stop=true;
							--scrptr;
							running = false;
							break;
						}
						buffer += vec ? toupper(c2) : c2;
						if(vec)
						{
							if(c2 == '}')
							{
								size_t sz = vecstr_size(buffer.c_str()+j);
								if(sz)
								{
									j += sz;
									break;
								}
							}
						}
						else
						{
							if(!bslash && c2 == '"')
							{
								size_t sz = escstr_size(buffer.c_str()+j);
								if(sz)
								{
									j += sz;
									break;
								}
							}
						}
					}
				}
			}
		}
		
		if(meta_mode)
		{
			//! TODO ZASM MERGE 'buffer' contains metadata. We don't have metadata yet here.
			--i; continue;
		}
		
		int32_t k=0, l=0;
		
		while(buffer[k] == ' ' || buffer[k] == '\t') k++;
		
		if(buffer[k] == '\0')
		{
			--i;
			continue;
		}
		
		k=0;
		
		while(buffer[k] != ' ' && buffer[k] != '\t' && buffer[k] != '\0') k++; //label
		
		while((buffer[k] == ' ' || buffer[k] == '\t') && buffer[k] != '\0')  k++; //ws
		
		while(buffer[k] != ' ' && buffer[k] != '\t' && buffer[k] != '\0') //command
		{
			combuf[l] = buffer[k];
			k++;
			l++;
		}
		
		if(l == 0) //No command
		{
			--i;
			continue;
		}
		
		combuf[l] = '\0';
		l=0;
		
		while((buffer[k] == ' ' || buffer[k] == '\t') && buffer[k] != '\0') k++; //ws
		
		bool bad_dstr = false;
		bool bad_dvec = false;
		for(int arg = 0; arg <= NUM_ZS_ARGS; ++arg)
		{
			if(buffer[k] == '"') //string
			{
				if(has_str) bad_dstr = true;
				else
				{
					arr_str = unescape_string(buffer.c_str()+k);
					has_str = true;
				}
				k += escstr_size(buffer.c_str()+k);
			}
			else if(buffer[k] == '{') //array
			{
				if(has_vec) bad_dvec = true;
				else
				{
					unstringify_vector(arr_vec, buffer.c_str()+k, true);
					has_vec = true;
				}
				k += vecstr_size(buffer.c_str()+k);
			}
			else
			{
				if(arg == NUM_ZS_ARGS)
					break;
				while(buffer[k] != ',' && buffer[k] != ' ' && buffer[k] != '\t' && buffer[k] != '\0')
					argbufs[arg][l++] = buffer[k++];
				argbufs[arg][l] = '\0';
				l = 0;
			}
			while((buffer[k] == ' ' || buffer[k] == '\t' || buffer[k] == ',') && buffer[k] != '\0') k++; //ws/comma
		}
		
		int32_t parse_err;
		ffscript& zas = zasm.emplace_back();
		if(bad_dstr || bad_dvec ||
			!(parse_script_section(combuf, argbufs, zas, parse_err, has_vec ? &arr_vec : nullptr, has_str ? &arr_str : nullptr)))
		{
			if(bad_dstr)
				parse_err = ERR_STR;
			if(bad_dvec)
				parse_err = ERR_VEC;
			char buf[512], name[13];
			extract_name(temppath,name,FILENAME8_3);
			char vstrbuf[64] = {0};
			if(has_str || has_vec)
				sprintf(vstrbuf," (%s%s%s)",has_str ? "str" : "", has_str&&has_vec ? "," : "", has_vec ? "vec" : "");
			sprintf(buf,"Unable to parse instruction %d"
				"\nThe error was: %s"
				"\nThe command was (%s) (%s,%s)%s"
				,i+1
				,errstrbuf[parse_err]
				,combuf,arg1buf,arg2buf,vstrbuf);
			InfoDialog("Error",buf).show();
			stop=true;
			success=false;
			zasm.pop_back();
		}
	}

	if(report_success && success)
	{
		jwin_alert("Success",NULL,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
zasmfile_fail_str:
	return success?D_O_K:D_CLOSE;
}

int32_t set_argument(char const* argbuf, int32_t& arg)
{
	if (auto var = get_script_variable(argbuf))
	{
		arg = *var;
		return 1;
	}
	
	return 0;
}

optional<int> check_comparestr(char const* buf)
{
	int cmp = 0;
	if(buf[0] == 'B')
	{
		cmp |= CMP_BOOL;
		++buf;
	}
	if(buf[0] == 'I')
	{
		cmp |= CMP_SETI;
		++buf;
	}
	if(!strcmp(buf,"<"))
		cmp |= CMP_LT;
	else if(!strcmp(buf,">"))
		cmp |= CMP_GT;
	else if(!strcmp(buf,"<="))
		cmp |= CMP_LE;
	else if(!strcmp(buf,">="))
		cmp |= CMP_GE;
	else if(!strcmp(buf,"=="))
		cmp |= CMP_EQ;
	else if(!strcmp(buf,"!="))
		cmp |= CMP_NE;
	else if(!strcmp(buf,"Never"))
		;
	else if(!strcmp(buf,"Always"))
		cmp |= CMP_FLAGS;
	else return nullopt; //nonmatching
	return cmp;
}

bool handle_arg(ARGTY ty, char const* buf, int& arg)
{
	switch(ty)
	{
		case ARGTY::READ_REG:
		case ARGTY::WRITE_REG:
		case ARGTY::READWRITE_REG:
		{
			if(!set_argument(buf, arg))
				return false;
			return true;
		}
		case ARGTY::LITERAL:
		{
			if(!ffcheck(buf))
				return false;
			arg = ffparse(buf);
			return true;
		}
		case ARGTY::COMPARE_OP:
		{
			if(auto cmpval = check_comparestr(buf))
			{
				arg = *cmpval;
				return true;
			}
			return false;
		}
	}
	return false;
}

int32_t parse_script_section(char const* combuf, char const* const* argbufs, ffscript& zas, int32_t &retcode, std::vector<int32_t> *vptr, std::string *sptr)
{
	zas.arg1 = 0;
	zas.arg2 = 0;
	zas.vecptr = nullptr;
	zas.strptr = nullptr;
	if(vptr)
	{
		zas.vecptr = new std::vector<int32_t>(*vptr);
	}
	if(sptr)
	{
		zas.strptr = new std::string(*sptr);
	}
	bool found_command=false;	
	
	// TODO: should use get_script_command(string)
	for(int32_t i=0; i<NUMCOMMANDS&&!found_command; ++i)
	{
		auto& sc = get_script_command(i);
		if(strcmp(combuf,sc.name)==0)
		{
			found_command=true;
			zas.command = i;
			bool is_goto = false;
			
			switch(i)
			{
				case GOTO: case LOOP: //Hardcodes for some control flow opcodes
				case GOTOTRUE: case GOTOFALSE: case GOTOLESS: case GOTOMORE: case GOTOCMP:
					is_goto = true;
					break;
			}
			if(is_goto) //hardcoded arg1
			{
				string lbl(argbufs[0]);
				auto it = labels.find(lbl);
				if(it != labels.end())
					zas.arg1 = (*it).second;
				else
					zas.arg1 = atoi(argbufs[0])-1;
			}
			int *args[] = {&zas.arg1, &zas.arg2, &zas.arg3};
			for(int q = (is_goto ? 1 : 0); q < sc.args; ++q)
			{
				if(!handle_arg(sc.arg_type[q], argbufs[q], *(args[q])))
				{
					retcode = ERR_PARAM1+q;
					return 0;
				}
			}
			if(byte b = sc.arr_type)
			{
				if(!(b&0x1) != !sptr) //string
				{
					retcode = ERR_STR;
					return 0;
				}
				if(!(b&0x2) != !vptr) //vector
				{
					retcode = ERR_VEC;
					return 0;
				}
			}
			switch(i)
			{
				case CALLFUNC: //Hardcoded, based on GOTO above
					zas.arg1 -= 1;
					break;
			}
		}
	}
	
	if(found_command)
	{
		return 1;
	}
	
	retcode=ERR_INSTRUCTION;
	return 0;
}

