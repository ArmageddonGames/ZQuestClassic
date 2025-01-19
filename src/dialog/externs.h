#ifndef ZC_DIALOG_EXTERNS_H_
#define ZC_DIALOG_EXTERNS_H_

#include "base/headers.h"
#include "base/containers.h"
struct mapscr;

void call_cmb_pages(optional<int> val = nullopt);
void call_cpool_pages(optional<int> val = nullopt);
void call_autoc_pages(optional<int> val = nullopt);
void call_alias_pages(optional<int> val = nullopt);

optional<int32_t> call_get_num(string const& lbl, int32_t dv, int32_t max = 0, int32_t min = 0);
optional<int32_t> call_get_num(string const& lbl, string const& inf, int32_t dv, int32_t max = 0, int32_t min = 0);
optional<zfix> call_get_zfix(string const& lbl, zfix dv, zfix max = 0_zf, zfix min = 0_zf);
optional<zfix> call_get_zfix(string const& lbl, string const& inf, zfix dv, zfix max = 0_zf, zfix min = 0_zf);

void displayinfo(string const& title, string const& text, optional<string> subtext = nullopt);
void displayinfo(string const& title, vector<string> const& lines, optional<string> subtext = nullopt);

bool call_checklist_dialog(string const& title, vector<CheckListInfo> const& flagnames, bitstring& flags);
bool call_checklist_dialog(string const& title, vector<CheckListInfo> const& flagnames, int32_t& flags);

void edit_screen_notes(mapscr* scr, int map, int screen);
void browse_screen_notes();

#endif

