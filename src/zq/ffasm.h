#ifndef _FFASM_H_
#define _FFASM_H_

#include "base/zdefs.h"
#include <utility>
#include <string>
#include <list>

int32_t set_argument(char const* argbuf, int32_t& argument);
int32_t parse_script_section(char const* combuf, char const* const* argbufs, ffscript& zas, int32_t &retcode, std::vector<int32_t> *vptr = nullptr, std::string *sptr = nullptr);
int32_t parse_script_string(vector<ffscript>& zasm, std::string const& scriptstr, bool report_success);
int32_t ffparse(char *string);

#endif
