#ifndef ZC_LIST_DATA_H_
#define ZC_LIST_DATA_H_

#include "gui/list_data.h"

namespace GUI::ZCListData
{
	GUI::ListData fonts(bool ss_fonts, bool numbered = true, bool sorted = true);
	GUI::ListData shadow_types();
	GUI::ListData strings(bool combostr = false, bool respect_order = true, bool numbered = true);
	GUI::ListData ss_counters(bool numbered = false, bool skipNone = false);
	GUI::ListData enemies(bool numbered = false, bool defaultFilter = true);
	GUI::ListData efamilies(bool numbered = false);
	GUI::ListData items(bool numbered = false, bool none = true);
	GUI::ListData dropsets(bool numbered = false, bool none = false);
	GUI::ListData itemclass(bool numbered = false, bool zero_none = false);
	GUI::ListData combotype(bool numbered = false, bool skipNone = false);
	GUI::ListData mapflag(int32_t numericalFlags, bool numbered = false, bool skipNone = false);
	GUI::ListData counters(bool numbered = false, bool skipNone = false);
	GUI::ListData miscsprites(bool skipNone = true, bool inclNegSpecialVals = false, bool numbered = true);
	GUI::ListData statusnames(bool skipNone = true, bool numbered = true);
	GUI::ListData bottletype();
	GUI::ListData dmaps(bool numbered = false);
	GUI::ListData lweaptypes(bool numbered = false);
	GUI::ListData eweaptypes(bool numbered = false);
	GUI::ListData weaptypes(bool numbered = false);
	GUI::ListData defenses(byte first, byte last, bool enemy);
	GUI::ListData sfxnames(bool numbered = false);
	GUI::ListData midinames(bool numbered = false, bool incl_engine = false);
	GUI::ListData lpals();
	GUI::ListData subscreens(byte type, bool numbered = false);
	GUI::ListData disableditems(byte* disabledarray);
	GUI::ListData dirs(int count, bool none);
	//Scripts
	GUI::ListData itemdata_script();
	GUI::ListData itemsprite_script();
	GUI::ListData ffc_script();
	GUI::ListData npc_script();
	GUI::ListData dmap_script();
	GUI::ListData screen_script();
	GUI::ListData lweapon_script();
	GUI::ListData eweapon_script();
	GUI::ListData combodata_script();
	GUI::ListData generic_script();
	GUI::ListData subscreen_script();
	GUI::ListData slots_ffc_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_global_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_itemdata_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_npc_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_lweapon_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_eweapon_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_hero_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_dmap_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_screen_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_itemsprite_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_combo_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_generic_script(bool alphabetize = false, bool skipempty = false);
	GUI::ListData slots_subscreen_script(bool alphabetize = false, bool skipempty = false);
	//const& returns
	GUI::ListData const& eanimations();
	GUI::ListData const& deftypes();
	GUI::ListData const& warpeffects();
	GUI::ListData const& screenstate();
	GUI::ListData const& subscr_widgets();
	GUI::ListData const& dmaptypes();
	GUI::ListData const& alignments();
	GUI::ListData const& buttons();
	GUI::ListData const& autocombotypes();
	GUI::ListData const& scripttypes();
	GUI::ListData const& patterntypes();
}

#endif
