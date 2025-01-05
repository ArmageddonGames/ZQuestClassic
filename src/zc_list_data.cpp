#include "zc_list_data.h"
#include "base/dmap.h"
#include "base/qst.h"
#include "subscr.h"
#include "zc/zelda.h"
#include "zq/zquest.h"
#include "zq/zq_misc.h"
#include "zinfo.h"
#include "base/misctypes.h"
#include "base/autocombo.h"
#include <fmt/format.h>

extern zcmodule moduledata;
extern char *weapon_string[];
extern char *sfx_string[];
extern char *item_string[];
extern const char* old_guy_string[OLDMAXGUYS];
extern char *guy_string[eMAXGUYS];
extern item_drop_object item_drop_sets[MAXITEMDROPSETS];

#ifdef IS_PARSER
#elif defined(IS_PLAYER)
#define customtunes tunes
extern zctune tunes[MAXMIDIS];
#elif defined(IS_EDITOR)
extern zctune *customtunes;
const char *msgslist(int32_t index, int32_t *list_size);
char *MsgString(int32_t index, bool show_number, bool pad_number);
int32_t addtomsglist(int32_t index, bool allow_numerical_sort = true);
#endif

extern std::string msgfont_str[font_max];
const char *ssfont2_str[] =
{
	"Zelda NES", "SS 1", "SS 2", "SS 3", "SS 4", "BS Time", "Small", "Small Prop.", "LttP Small", "Link's Awakening", "Link to the Past",
	"Goron", "Zoran", "Hylian 1", "Hylian 2", "Hylian 3", "Hylian 4", "Proportional", "Oracle", "Oracle Proportional", "Phantom", "Phantom Proportional",
	"Atari 800", "Acorn", "ADOS", "Allegro", "Apple II", "Apple II 80 Column", "Apple IIgs", "Aquarius",
	"Atari 400", "C64", "C64 HiRes", "IBM CGA", "COCO Mode I", "COCO Mode II", "Coupe", "Amstrad CPC",
	"Fantasy Letters", "FDS Katakana", "FDSesque", "FDS Roman", "FF", "Elder Futhark", "Gaia", "Hira",
	"JP Unsorted", "Kong", "Mana", "Mario", "Mot CPU", "MSX Mode 0", "MSX Mode 1", "PET", "Homebrew",
	"Mr. Saturn", "Sci-Fi", "Sherwood", "Sinclair QL", "Spectrum", "Spectrum Large", "TI99", "TRS",
	"Zelda 2", "ZX", "Lisa", ""
};
const char *icounter_str2[-(sscMIN+1)]=
{
    "Gen. Keys w/Magic", "Gen. Keys w/o Magic", "Level Keys w/Magic",
    "Level Keys w/o Magic", "Any Keys w/Magic", "Any Keys w/o Magic",
	"Max Life", "Max Magic"
};


const char *shadowstyle_str[sstsMAX] =
{
    "None", "Shadow", "Shadow (U)", "Shadow (O)", "Shadow (+)", "Shadow (X)", "Shadowed", "Shadowed (U)", "Shadowed (O)", "Shadowed (+)", "Shadowed (X)"
};

static bool skipchar(char c)
{
	return c == 0 || c == '-';
}

GUI::ListData GUI::ZCListData::fonts(bool ss_fonts, bool numbered, bool sorted)
{
	map<std::string, int32_t> ids;
	std::set<std::string> names;
	std::vector<std::string> strings;
	if(ss_fonts)
	{
		for(auto q = 0; ssfont2_str[q][0]; ++q)
		{
			char const* fontname = ssfont2_str[q];
			std::string name;
			if(numbered)
				name = fmt::format("{} ({:03})", fontname, q);
			else name = fontname;
			
			if(sorted)
			{
				ids[name] = q;
				names.insert(name);
			}
			else strings.push_back(name);
		}
	}
	else for(auto q = 0; q < font_max; ++q)
	{
		char const* fontname = msgfont_str[q].c_str();
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", fontname, q);
		else name = fontname;
		
		if(sorted)
		{
			ids[name] = q;
			names.insert(name);
		}
		else strings.push_back(name);
	}
	if(!sorted)
		return GUI::ListData(strings);
	
	GUI::ListData ls;
	for(auto it = names.begin(); it != names.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

static const GUI::ListData combostrs
{
	{ "-17: Screen->D[7]", -17 },
	{ "-16: Screen->D[6]", -16 },
	{ "-15: Screen->D[5]", -15 },
	{ "-14: Screen->D[4]", -14 },
	{ "-13: Screen->D[3]", -13 },
	{ "-12: Screen->D[2]", -12 },
	{ "-11: Screen->D[1]", -11 },
	{ "-10: Screen->D[0]", -10 },
	{ " -2: Screen Catchall", -2 },
	{ " -1: Screen Message String", -1 }
};
GUI::ListData GUI::ZCListData::strings(bool combostr, bool respect_order, bool numbered)
{
	GUI::ListData ls;
	#ifdef IS_EDITOR
	if(combostr)
		ls = combostrs;
	
	std::map<int,int> poses;
	for(int q = 0; q < msg_count; ++q)
	{
		if(respect_order)
		{
			int listpos = addtomsglist(q, false);
			poses[q] = listpos;
		}
		else poses[q] = q;
	}
	for(auto it = poses.begin(); it != poses.end(); ++it)
	{
		int val = it->second;
		std::string name(MsgString(val, numbered, numbered));
		ls.add(name,val);
	}
	#endif
	return ls;
}

GUI::ListData GUI::ZCListData::ss_counters(bool numbered, bool skipNone)
{
	GUI::ListData ls;
	
	if(!skipNone) ls.add("(None)", -1);
	auto sscstr_sz = -(sscMIN+1);
	for(int32_t q = sscMIN+1; q < MAX_COUNTERS; ++q)
	{
		if(q == crNONE) continue;
		char const* str;
		if(q > crNONE)
		{
			if(!ZI.isUsableCtr(q))
				continue; //Hidden
			str = ZI.getCtrName(q);
		}
		else str = icounter_str2[sscstr_sz + q];
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", str, q);
		else name = str;
		ls.add(name, q);
	}
	
	return ls;
}

GUI::ListData GUI::ZCListData::shadow_types()
{
	std::vector<std::string> strings;

	for(auto q = 0; q < sstsMAX; ++q)
	{
		strings.push_back(shadowstyle_str[q]);
	}

	return GUI::ListData(strings);
}

GUI::ListData GUI::ZCListData::enemies(bool numbered, bool defaultFilter)
{
	map<std::string, int32_t> ids;
	std::set<std::string> names;
	
	for(int32_t q=1; q < eMAXGUYS; ++q)
	{
		if(defaultFilter)
		{
			if(q >= 0 && q <= 19) //Room Guys
				continue;
			if(q < OLDMAXGUYS && old_guy_string[q][strlen(old_guy_string[q])-1]==' ')
				continue; //'Hidden' enemies
		}
		char const* npcname = guy_string[q];
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", npcname, q);
		else name = npcname;
		
		ids[name] = q;
		names.insert(name);
	}
	
	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(auto it = names.begin(); it != names.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::efamilies(bool numbered)
{
	std::map<std::string, int32_t> vals;

	GUI::ListData ls;
	for (int32_t q = 1; q < eeMAX; ++q)
	{
		if (!ZI.isUsableEnemyType(q))
			continue; //Hidden
		char const* module_str = ZI.getEnemyTypeName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}

	return ls;
}

GUI::ListData GUI::ZCListData::items(bool numbered, bool none)
{
	map<std::string, int32_t> ids;
	std::set<std::string> names;
	
	for(int32_t q=0; q < MAXITEMS; ++q)
	{
		char const* itname = item_string[q];
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", itname, q);
		else name = itname;
		
		ids[name] = q;
		names.insert(name);
	}
	
	GUI::ListData ls;
	if(none)
		ls.add("(None)", -1);
	for(auto it = names.begin(); it != names.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::dropsets(bool numbered, bool none)
{
	map<std::string, int32_t> ids;
	std::set<std::string> names;
	
	for(int32_t q=0; q < MAXITEMDROPSETS; ++q)
	{
		std::string dropname = item_drop_sets[q].name;
		std::string name;
		if (dropname[0] == 0)
		{
			name = fmt::format("zz{:03}", q);
		}
		else if(numbered)
			name = fmt::format("{} ({:03})", dropname, q);
		else name = dropname;
		
		ids[name] = q;
		names.insert(name);
	}
	
	GUI::ListData ls;
	if(none)
		ls.add("(None)", -1);
	for(auto it = names.begin(); it != names.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::itemclass(bool numbered, bool zero_none)
{
	map<std::string, int32_t> fams;
	std::set<std::string> famnames;
	
	for(int32_t i=zero_none?1:0; i<itype_max; ++i)
	{
		if(!ZI.isUsableItemclass(i))
			continue; //Hidden
		char const* itname = ZI.getItemClassName(i);
        if(i < itype_last || itname[0])
		{
			std::string name;
			if(numbered)
				name = fmt::format("{} ({:03})", itname, i);
			else name = itname;
			
			fams[name] = i;
			famnames.insert(name);
		}
		else 
		{
			std::string name;
			if(numbered)
				name = fmt::format("zz{:03} ({:03})", i, i);
			else name = fmt::format("zz{:03}",i);
			
			fams[name] = i;
			famnames.insert(name);
		}
	}
	
	GUI::ListData ls;
	if(zero_none)
		ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(auto it = famnames.begin(); it != famnames.end(); ++it)
	{
		ls.add(*it, fams[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::combotype(bool numbered, bool skipNone)
{
	GUI::ListData ls;
	map<std::string, int32_t> types;
	std::set<std::string> typenames;

	if(!skipNone) ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(int32_t i=1; i<cMAX; ++i)
	{
		if(!ZI.isUsableComboType(i))
			continue; //Hidden
		char const* module_str = ZI.getComboTypeName(i);
		if(module_str[0])
		{
			std::string name;
			if(numbered)
				name = fmt::format("{} ({:03})", module_str, i);
			else name = module_str;
			
			types[name] = i;
			typenames.insert(name);
		}
		else 
		{
			std::string name;
			if(numbered)
				name = fmt::format("zz{:03} ({:03})", i, i);
			else name = fmt::format("zz{:03}",i);
			
			types[name] = i;
			typenames.insert(name);
		}
	}

	for(auto it = typenames.begin(); it != typenames.end(); ++it)
	{
		ls.add(*it, types[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::mapflag(int32_t numericalFlags, bool numbered, bool skipNone)
{
	GUI::ListData ls;
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	if(!skipNone) ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(int32_t q = 1; q < mfMAX; ++q)
	{
		if(!ZI.isUsableMapFlag(q))
			continue; //Hidden
		char const* module_str = ZI.getMapFlagName(q);
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", module_str, q);
		else name = module_str;
		
		if (numericalFlags)
			ls.add(name, q);
		else
		{
			vals[name] = q;
			names.insert(name);
		}
	}
	if (!numericalFlags)
	{
		for(auto it = names.begin(); it != names.end(); ++it)
		{
			ls.add(*it, vals[*it]);
		}
	}
	
	return ls;
}

GUI::ListData GUI::ZCListData::dmaps(bool numbered)
{
	GUI::ListData ls;
	
	for(int32_t q = 0; q < MAXDMAPS; ++q)
	{
		char const* dm_str = DMaps[q].name;
		if(numbered)
			ls.add(fmt::format("{:3}-{}",q,dm_str), q);
		else ls.add(dm_str, q);
	}
	
	return ls;
}

GUI::ListData GUI::ZCListData::counters(bool numbered, bool skipNone)
{
	GUI::ListData ls;
	// std::map<std::string, int32_t> vals;
	// std::set<std::string> names;
	
	if(!skipNone) ls.add("(None)", crNONE);
	for(int32_t q = 0; q < MAX_COUNTERS; ++q)
	{
		if(!ZI.isUsableCtr(q))
			continue; //Hidden
		char const* module_str = ZI.getCtrName(q);
		std::string sname;
		if(numbered)
			sname = fmt::format("{} ({:03})", module_str, q);
		else sname = module_str;
		
		// vals[sname] = q;
		// names.insert(sname);
		ls.add(sname, q);
	}
	
	// for(auto it = names.begin(); it != names.end(); ++it)
	// {
		// ls.add(*it, vals[*it]);
	// }
	
	return ls;
}

GUI::ListData GUI::ZCListData::miscsprites(bool skipNone, bool inclNegSpecialVals, bool numbered)
{
	std::map<std::string, int32_t> ids;
	std::set<std::string> sprnames;
	
	for(int32_t i=0; i<MAXWPNS; ++i)
	{
		std::string name;
		if(numbered)
			name = fmt::format("{} ({:03})", weapon_string[i], i);
		else name = weapon_string[i];
		
		ids[name] = i;
		sprnames.insert(name);
	}
	
	GUI::ListData ls;
	if(inclNegSpecialVals)
	{
		if(numbered)
		{
			ls.add("Grass Clippings (-004)", -4);
			ls.add("Flower Clippings (-003)", -3);
			ls.add("Bush Leaves (-002)", -2);
		}
		else
		{
			ls.add("Grass Clippings", -4);
			ls.add("Flower Clippings", -3);
			ls.add("Bush Leaves", -2);
		}
	}
	if(!skipNone)
		ls.add("(None)", -1);
	for(auto it = sprnames.begin(); it != sprnames.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::statusnames(bool skipNone, bool numbered)
{
	std::map<std::string, int32_t> ids;
	std::set<std::string> statnames;
	
	for(int q = 0; q < NUM_STATUSES; ++q)
	{
		std::string name = QMisc.status_names[q];
		if(name.empty())
			name = fmt::format("zz{:03}", q);
		
		if(numbered)
			name = fmt::format("{} ({:03})", name, q);
		
		ids[name] = q;
		statnames.insert(name);
	}
	
	GUI::ListData ls;
	if(!skipNone)
		ls.add("(None)", -1);
	for(auto it = statnames.begin(); it != statnames.end(); ++it)
	{
		ls.add(*it, ids[*it]);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::bottletype()
{
	GUI::ListData ls;
	
	ls.add("(None)", 0);
	for(int32_t q = 0; q < 64; ++q)
	{
		if(QMisc.bottle_types[q].name[0])
			ls.add(QMisc.bottle_types[q].name,q+1);
		else
			ls.add(fmt::format("{:2}", q+1), q+1);
	}
	
	return ls;
}

GUI::ListData GUI::ZCListData::lweaptypes(bool numbered)
{
	std::map<std::string, int32_t> vals;

	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for (int32_t q = 1; q < lwMax; ++q)
	{
		if (!ZI.isUsableWeap(q))
			continue; //Hidden
		char const* module_str = ZI.getWeapName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	for (int32_t q = wIce; q < wEnemyWeapons; ++q)
	{
		if (!ZI.isUsableWeap(q))
			continue; //Hidden
		char const* module_str = ZI.getWeapName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	ls.alphabetize();
	//should these always be at the end?
	for (int32_t q = wScript1; q < wScript1+10; ++q)
	{
		if (!ZI.isUsableWeap(q))
			continue; //Hidden
		char const* module_str = ZI.getWeapName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::eweaptypes(bool numbered)
{
	std::map<std::string, int32_t> vals;

	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for (int32_t q = wEnemyWeapons+1; q < wMax; ++q)
	{
		if (!ZI.isUsableWeap(q))
			continue; //Hidden
		char const* module_str = ZI.getWeapName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	ls.alphabetize();
	//should these always be at the end?
	for (int32_t q = wScript1; q < wScript1 + 10; ++q)
	{
		if (!ZI.isUsableWeap(q))
			continue; //Hidden
		char const* module_str = ZI.getWeapName(q);
		if (numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::weaptypes(bool numbered)
{
	std::map<std::string, int32_t> vals;
	
	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(int32_t q=1; q<wMax; ++q)
	{
		if(!ZI.isUsableWeap(q))
			continue; //Hidden
		string module_str = ZI.getWeapName(q);
		if(q >= wEnemyWeapons)
			module_str = fmt::format("E {}", module_str);
		if(numbered)
			ls.add(fmt::format("{} ({:03})", module_str, q), q);
		else ls.add(module_str, q);
	}
	
	return ls;
}

//problably not the best name
static string DefenseToWeaponName(byte def)
{
	switch (def)
	{
	case edefBRANG: return ZI.getWeapName(wBrang); break;
	case edefBOMB: return ZI.getWeapName(wLitBomb); break;
	case edefSBOMB: return ZI.getWeapName(wLitSBomb); break;
	case edefARROW: return ZI.getWeapName(wArrow); break;
	case edefFIRE: return ZI.getWeapName(wFire); break;
	case edefWAND: return "Wand"; break;
	case edefMAGIC: return ZI.getWeapName(wMagic); break;
	case edefHOOKSHOT: return ZI.getWeapName(wHookshot); break;
	case edefHAMMER: return ZI.getWeapName(wHammer); break;
	case edefSWORD: return ZI.getWeapName(wSword); break;
	case edefBEAM: return ZI.getWeapName(wBeam); break;
	case edefREFBEAM: return ZI.getWeapName(wRefBeam); break;
	case edefREFMAGIC: return ZI.getWeapName(wRefMagic); break;
	case edefREFBALL: return ZI.getWeapName(wRefFireball); break;
	case edefREFROCK: return ZI.getWeapName(wRefRock); break;
	case edefSTOMP: return ZI.getWeapName(wStomp); break;
	case edefBYRNA: return ZI.getWeapName(wCByrna); break;
	case edefSCRIPT01: return ZI.getWeapName(wScript1); break;
	case edefSCRIPT02: return ZI.getWeapName(wScript2); break;
	case edefSCRIPT03: return ZI.getWeapName(wScript3); break;
	case edefSCRIPT04: return ZI.getWeapName(wScript4); break;
	case edefSCRIPT05: return ZI.getWeapName(wScript5); break;
	case edefSCRIPT06: return ZI.getWeapName(wScript6); break;
	case edefSCRIPT07: return ZI.getWeapName(wScript7); break;
	case edefSCRIPT08: return ZI.getWeapName(wScript8); break;
	case edefSCRIPT09: return ZI.getWeapName(wScript9); break;
	case edefSCRIPT10: return ZI.getWeapName(wScript10); break;
	case edefWhistle: return ZI.getWeapName(wWhistle); break;
	case edefSwitchHook: return "SwitchHook"; break;
	case edefTHROWN: return ZI.getWeapName(wThrown); break;
	case edefREFARROW: return ZI.getWeapName(wRefArrow); break;
	case edefREFFIRE: return ZI.getWeapName(wRefFire); break;
	case edefREFFIRE2: return ZI.getWeapName(wRefFire2); break;
	//???
	case edefICE: return ZI.getWeapName(wIce); break;
	case edefBAIT: return ZI.getWeapName(wBait); break;
	case edefWIND: return ZI.getWeapName(wWind); break;
	case edefSPARKLE: return ZI.getWeapName(wFSparkle); break;
	case edefSONIC: return ZI.getWeapName(wSound); break;
	default: return "";
	}
}

GUI::ListData GUI::ZCListData::defenses(byte first, byte last, bool enemy)
{
	map<std::string, int32_t> vals;

	GUI::ListData ls;
	for (int32_t q = first; q < last; ++q)
	{
		std::string name = DefenseToWeaponName(q);
		ls.add(name, q);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::sfxnames(bool numbered)
{
	std::map<std::string, int32_t> vals;
	
	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	for(int32_t i=1; i<WAV_COUNT; ++i)
	{
		char const* sfx_name = sfx_string[i];
		if(numbered)
			ls.add(fmt::format("{} ({:03})", sfx_name, i), i);
		else ls.add(sfx_name, i);
	}
	
	return ls;
}
GUI::ListData GUI::ZCListData::midinames(bool numbered, bool incl_engine)
{
	std::map<std::string, int32_t> vals;
	
	GUI::ListData ls;
	ls.add(numbered ? "(None) (000)" : "(None)", 0);
	auto ofs = 1;
	if (incl_engine)
	{
		ls.add("Overworld", 1);
		ls.add("Dungeon", 2);
		ls.add("Level 9", 3);
		ofs = 4;
	}
	for(int32_t i=0; i<MAXCUSTOMTUNES; ++i)
	{
		char const* midi_name = customtunes[i].title;
		if(midi_name[0] == ' ' || !midi_name[0])
			midi_name = "zzUntitled";
		if(numbered)
			ls.add(fmt::format("{} ({:03})", midi_name, i + 1), i+ofs);
		else ls.add(midi_name, i+ofs);
	}
	
	return ls;
}



GUI::ListData GUI::ZCListData::lpals()
{
	GUI::ListData ls;
	char buf[50];
	for (int q = 0; q < 0x1FF; ++q)
	{
		sprintf(buf, "%.3X - %s", q, palnames[q]);
		ls.add(buf, q + 1);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::subscreens(byte type, bool numbered)
{
	std::vector<ZCSubscreen>& vec =
		(type == sstACTIVE ? subscreens_active
		: (type == sstPASSIVE ? subscreens_passive
		: subscreens_overlay));
	GUI::ListData ls;
	for(int q = 0; q < vec.size(); ++q)
	{
		if(numbered)
			ls.add(fmt::format("{} ({:03})",vec[q].name,q), q);
		else ls.add(vec[q].name,q);
	}
	if(!ls.size())
	{
		ls.add("[None Available]",0);
		ls.setInvalid(true);
	}
	return ls;
}

GUI::ListData GUI::ZCListData::disableditems(byte* disabledarray)
{
	GUI::ListData ls;
	for (int q = 0; q < MAXITEMS; ++q)
	{
		if (disabledarray[q] & 1)
		{
			char const* itname = item_string[q];
			ls.add(itname, q);
		}
	}
	if (ls.size() == 0)
	{
		ls.add("", -1);
	}
	ls.alphabetize();
	return ls;
}

GUI::ListData GUI::ZCListData::dirs(int count, bool none)
{
	static const string dirstrs[] = {"Up","Down","Left","Right","Up-Left","Up-Right","Down-Left","Down-Right"};
	GUI::ListData ls;
	if(none)
		ls.add("(None)", -1);
	if(count < 0) count = 4;
	if(count > 8) count = 8;
	for(int q = 0; q < count; ++q)
		ls.add(dirstrs[q],q);
	return ls;
}
//SCRIPTS
static void load_scriptnames(std::set<std::string> &names, std::map<std::string, int32_t> &vals,
	std::map<int32_t, script_slot_data> scrmap, int32_t count)
{
	for(int32_t i = 0; i < count; ++i)
	{
		if(!scrmap[i].scriptname[0])
			continue;
		std::string sname(scrmap[i].scriptname);
		sname += " (" + std::to_string(i+1) + ")";
		
		vals[sname] = i+1;
		names.insert(sname);
	}
}

// Global and player slot names only used by the editor
#ifdef IS_EDITOR
extern std::string global_slotnames[NUMSCRIPTGLOBAL];
extern std::string player_slotnames[NUMSCRIPTHERO - 1];

static GUI::ListData load_scriptnames_slots(std::map<int32_t, script_slot_data> scrmap, int32_t count, std::string* slotnames, bool alphabetize, bool skipempty)
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	vector<std::pair<string, int>> empties;

	GUI::ListData ls;
	for (int32_t i = 0; i < count; ++i)
	{
		std::string sname, slotname;
		if (scrmap[i].scriptname.empty())
		{
			if (skipempty)
				continue;
			sname = alphabetize ? "<Empty>" : "";
		}
		else
			sname = scrmap[i].scriptname;

		slotname = slotnames ? slotnames[i] : (alphabetize ? fmt::format("{:03}", i + 1) : fmt::format("Slot {}", i + 1));
		if(alphabetize)
			sname = fmt::format("{1} ({0})", slotname, sname);
		else
			sname = fmt::format("{}: {}", slotname, sname);

		if (alphabetize)
		{
			if(scrmap[i].scriptname.empty())
				empties.emplace_back( sname,i + 1 );
			else
			{
				vals[sname] = i + 1;
				names.insert(sname);
			}
		}
		else
			ls.add(sname, i+1);
	}
	if (alphabetize)
	{
		ls.add(names, vals);
		for (auto [name, index] : empties)
		{
			ls.add(name, index);
		}
	}
	if (ls.empty())
		ls.add("[No Scripts Found]", 0);
	return ls;
}
#endif

GUI::ListData GUI::ZCListData::itemdata_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,itemmap,NUMSCRIPTITEM-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::itemsprite_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,itemspritemap,NUMSCRIPTSITEMSPRITE-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::ffc_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,ffcmap,NUMSCRIPTFFC-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::npc_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;

	load_scriptnames(names, vals, npcmap, NUMSCRIPTGUYS - 1);

	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names, vals);
	return ls;
}

GUI::ListData GUI::ZCListData::dmap_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;

	load_scriptnames(names, vals, dmapmap, NUMSCRIPTSDMAP - 1);

	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names, vals);
	return ls;
}

GUI::ListData GUI::ZCListData::screen_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,screenmap,NUMSCRIPTSCREEN-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::lweapon_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,lwpnmap,NUMSCRIPTWEAPONS-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::eweapon_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,ewpnmap,NUMSCRIPTWEAPONS-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::combodata_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,comboscriptmap,NUMSCRIPTSCOMBODATA-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::generic_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,genericmap,NUMSCRIPTSGENERIC-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

GUI::ListData GUI::ZCListData::subscreen_script()
{
	std::map<std::string, int32_t> vals;
	std::set<std::string> names;
	
	load_scriptnames(names,vals,subscreenmap,NUMSCRIPTSSUBSCREEN-1);
	
	GUI::ListData ls;
	ls.add("(None)", 0);
	ls.add(names,vals);
	return ls;
}

// Script slot lists only used by the editor
#ifdef IS_EDITOR
GUI::ListData GUI::ZCListData::slots_ffc_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(ffcmap, NUMSCRIPTFFC - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_global_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(globalmap, NUMSCRIPTGLOBAL, global_slotnames, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_itemdata_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(itemmap, NUMSCRIPTITEM - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_npc_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots( npcmap, NUMSCRIPTGUYS - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_lweapon_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(lwpnmap, NUMSCRIPTWEAPONS - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_eweapon_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(ewpnmap, NUMSCRIPTWEAPONS - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_hero_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(playermap, NUMSCRIPTHERO - 1, player_slotnames, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_dmap_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(dmapmap, NUMSCRIPTSDMAP - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_screen_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(screenmap, NUMSCRIPTSCREEN - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_itemsprite_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(itemspritemap, NUMSCRIPTSITEMSPRITE - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_combo_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(comboscriptmap, NUMSCRIPTSCOMBODATA - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_generic_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(genericmap, NUMSCRIPTSGENERIC - 1, nullptr, alphabetize, skipempty);
}

GUI::ListData GUI::ZCListData::slots_subscreen_script(bool alphabetize, bool skipempty)
{
	return load_scriptnames_slots(subscreenmap, NUMSCRIPTSSUBSCREEN - 1, nullptr, alphabetize, skipempty);
}
#endif

//CONST& RETURNS

static const GUI::ListData eanimation_types
{
	{"(None)",0},
	{"Flip",1},
	{"-Unused",2},
	{"2-Frame",3},
	{"-Unused",4},
	{"Octorok (NES)",5},
	{"Tektite (NES)",6},
	{"Leever (NES)",7},
	{"Walker",8},
	{"Zora (NES)",9},
	{"Zora (4-Frame)",10},
	{"Ghini",11},
	{"Armos (NES)",12},
	{"Rope",13},
	{"Wall Master (NES)",14},
	{"Wall Master (4-Frame)",15},
	{"Darknut (NES)",16},
	{"Vire",17},
	{"3-Frame",18},
	{"Wizzrobe (NES)",19},
	{"Aquamentus",20},
	{"Dodongo (NES)",21},
	{"Manhandla",22},
	{"Gleeok",23},
	{"Digdogger",24},
	{"Gohma",26},
	{"Lanmola",26},
	{"2-Frame Flying",27},
	{"4-Frame 4-Dir + Tracking",28},
	{"4-Frame 8-Dir + Tracking",29},
	{"4-Frame 4-Dir + Firing",30},
	{"4-Frame 4-Dir",31},
	{"4-Frame 8-Dir + Firing",32},
	{"Armos (4-Frame)",33},
	{"4-Frame Flying 4-Dir",34},
	{"4-Frame Flying 8-Dir",35},
	{"-Unused",36},
	{"4-Frame 8-Dir Big",37},
	{"Tektite (4-Frame)",38},
	{"3-Frame 4-Dir",39},
	{"2-Frame 4-Dir",40},
	{"Leever (4-Frame)",41},
	{"2-Frame 4-Dir + Tracking",42},
	{"Wizzrobe (4-Frame)",43},
	{"Dodongo (4-Frame)",44},
	{"Dodongo BS (4-Frame)",45},
	{"4-Frame Flying 8-Dir + Firing",46},
	{"4-Frame Flying 4-Dir + Firing",47},
	{"4-Frame",48},
	{"Ganon",49},
	{"2-Frame Big",50},
	{"4-Frame 8-Dir Big + Tracking",51},
	{"4-Frame 4-Dir Big + Tracking",52},
	{"4-Frame 8-Dir Big + Firing",53},
	{"4-Frame 4-Dir Big",54},
	{"4-Frame 4-Dir Big + Firing",55},
};

GUI::ListData const& GUI::ZCListData::eanimations()
{
	return eanimation_types;
}

static const GUI::ListData defense_types
{
	{ "(None)", 0 },
	{ "1/2 Damage", 1 },
	{ "1/4 Damage", 2 },
	{ "Stun", 3 },
	{ "Stun Or Block", 4 },
	{ "Stun Or Ignore", 5 },
	{ "Block If < 1", 6 },
	{ "Block If < 2", 7 },
	{ "Block If < 4", 8 },
	{ "Block If < 6", 9 },
	{ "Block If < 8", 10 },
	{ "Block", 11 },
	{ "Ignore If < 1", 12 },
	{ "Ignore", 13 },
	{ "One-Hit-Kill", 14 },
	{ "Block if Power < 10", 15 },
	{ "Double Damage", 16 },
	{ "Triple Damage", 17 },
	{ "Quadruple Damage", 18 },
	{ "Enemy Gains HP = Damage", 19 },
	{ "Trigger Screen Secrets", 20 },
	// { "-freeze", 21 },
	// { "-msgnotenabled", 22 },
	// { "-msgline", 23 }, 
	// { "-lvldamage", 24 },
	// { "-lvlreduction", 25 },
	{ "Split", 26 },
	{ "Transform", 27 },
	// { "-lvlblock2", 28 },
	// { "-lvlblock3", 29 },
	// { "-lvlblock4", 30 }, 
	// { "-lvlblock5", 31 },
	// { "-shock", 32 },
	{ "Bomb Explosion", 33 },
	{ "Superbomb Explosion", 34 },
	// { "-deadrock", 35 },
	// { "-breakshoeld", 36 },
	// { "-restoreshield", 37 },
	// { "-specialdrop", 38 },
	// { "-incrcounter", 39 },
	// { "-reducecounter", 40 },
	{ "Harmless Explosion", 41 },
	// { "-killnosplit", 42 },
	// { "-tribble", 43 },
	// { "-fireball", 44 },
	// { "-fireballlarge", 45 },
	{ "Summon", 46 }// ,
	// { "-wingame", 47 },
	// { "-jump", 48 },
	// { "-eat", 49 },
	// { "-showmessage", 50 }
};

GUI::ListData const& GUI::ZCListData::deftypes()
{
	return defense_types;
}

static const GUI::ListData warp_effects
{
	{ "None", 0 },
	{ "Zap", 1 },
	{ "Wave", 2 },
	{ "Instant", 3 },
	{ "Open", 4 }
};

GUI::ListData const& GUI::ZCListData::warpeffects()
{
	return warp_effects;
}

static const GUI::ListData screen_state
{
	{ "Door Up", 0 },
	{ "Door Down", 1 },
	{ "Door Left", 2 },
	{ "Door Right", 3 },
	{ "Screen Item", 4 },
	{ "Special Item", 5 },
	{ "Enemies Never Return", 6 },
	{ "Enemies Temp No Return", 7 },
	{ "Lockblock", 8 },
	{ "Boss Lockblock", 9 },
	{ "Chest", 10 },
	{ "Locked Chest", 11 },
	{ "Boss Chest", 12 },
	{ "Secrets", 13 },
	{ "Visited", 14 },
	{ "Light Triggers", 15 }
};

GUI::ListData const& GUI::ZCListData::screenstate()
{
	return screen_state;
}

static const GUI::ListData subscrWidgets =
{
	{ "(None)", widgNULL, "No Widget" },
	{ "2x2 Frame", widgFRAME, "Draws a frame made up of 8x8 sections of a 2x2 of tiles" },
	{ "Text", widgTEXT, "Displays some fixed text on the screen" },
	{ "Line", widgLINE, "Draws a line" },
	{ "Rectangle", widgRECT, "Draws a rectangle" },
	{ "Playtime", widgTIME, "Draws the current playtime" },
	{ "Magic Meter", widgMMETER, "Draws the old, hardcoded magic meter" },
	{ "Life Meter", widgLMETER, "Draws the old, hardcoded life meter" },
	{ "Button Item", widgBTNITM, "Draws the item currently equipped to a given button" },
	{ "Counter", widgCOUNTER, "Draws a counter value (or sum of up to 3 counter values)" },
	{ "Counter Block", widgOLDCTR, "Draws the classic counter block" },
	{ "Minimap Title", widgMMAPTITLE, "Displays the title of the minimap" },
	{ "Minimap", widgMMAP, "Draws the minimap, showing your location and surroundings" },
	{ "Large Map", widgLMAP, "Draws the big map, showing more detailed location information" },
	{ "Background Color", widgBGCOLOR, "Draws a color that covers the whole screen" },
	{ "Item Slot", widgITEMSLOT, "A slot for an item, whether equippable or not." },
	{ "McGuffin Frame", widgMCGUFF_FRAME, "A frame for showing off the McGuffins" },
	{ "McGuffin Piece", widgMCGUFF, "A single McGuffin piece on display" },
	{ "Tile Block", widgTILEBLOCK, "Draws a block of tiles" },
	{ "Minitile", widgMINITILE, "Draws a quarter-tile" },
	{ "Selector", widgSELECTOR, "Draws the selector cursor on the currently selected widget" },
	{ "Gauge Piece: Life", widgLGAUGE, "Allows building highly customizable life gauges" },
	{ "Gauge Piece: Magic", widgMGAUGE, "Allows building highly customizable magic gauges" },
	{ "Text Box", widgTEXTBOX, "Displays some fixed text in a formatted box" },
	{ "Selection Text", widgSELECTEDTEXT, "Displays text that changes based on the widget"
		" currently selected by the cursor" },
	{ "Gauge Piece: Counter", widgMISCGAUGE, "Allows building highly customizable gauges"
		" for any counter" },
	{ "Button Counter", widgBTNCOUNTER, "Shows a counter used by a specified button as a cost" },
};

GUI::ListData const& GUI::ZCListData::subscr_widgets()
{
	return subscrWidgets;
}

static const GUI::ListData dmap_types
{
	{ "NES Dungeon", dmDNGN },
	{ "Overworld", dmOVERW },
	{ "Interior", dmCAVE },
	{ "BS Overworld", dmBSOVERW }
};

GUI::ListData const& GUI::ZCListData::dmaptypes()
{
	return dmap_types;
}


static const GUI::ListData aligns
{
	{ "Left", 0 },
	{ "Center", 1 },
	{ "Right", 2 }
};

GUI::ListData const& GUI::ZCListData::alignments()
{
	return aligns;
}

static const GUI::ListData button
{
	{ "A", 0 },
	{ "B", 1 },
	{ "X", 2 },
	{ "Y", 3 }
};

GUI::ListData const& GUI::ZCListData::buttons()
{
	return button;
}

static const GUI::ListData autocombo_types
{
	{ "(None)", AUTOCOMBO_NONE },
	{ "Basic Relational", AUTOCOMBO_BASIC },
	{ "Relational", AUTOCOMBO_RELATIONAL },
	{ "Flat Mountain", AUTOCOMBO_Z1 },
	{ "Pancake Mountain", AUTOCOMBO_Z4 },
	{ "Fence", AUTOCOMBO_FENCE },
	{ "Dungeon Carving", AUTOCOMBO_DGNCARVE },
	{ "Complex Mountain", AUTOCOMBO_DOR },
	{ "Positional Tiling", AUTOCOMBO_TILING },
	{ "Replace", AUTOCOMBO_REPLACE},
	{ "Dense Forest", AUTOCOMBO_DENSEFOREST},
	{ "Extension", AUTOCOMBO_EXTEND}
};

GUI::ListData const& GUI::ZCListData::autocombotypes()
{
	return autocombo_types;
};

static const GUI::ListData script_types
{
	{ "FFC", int(ScriptType::FFC) },
	{ "Global", int(ScriptType::Global) },
	{ "Item", int(ScriptType::Item) },
	{ "NPC", int(ScriptType::NPC) },
	{ "LWeapon", int(ScriptType::Lwpn) },
	{ "EWeapon", int(ScriptType::Ewpn) },
	{ "Hero", int(ScriptType::Hero) },
	{ "DMap", int(ScriptType::DMap) },
	{ "Screen", int(ScriptType::Screen) },
	{ "Item Sprite", int(ScriptType::ItemSprite) },
	{ "Combo", int(ScriptType::Combo) },
	{ "Generic", int(ScriptType::Generic) },
	{ "Subscreen", int(ScriptType::EngineSubscreen) }
};

GUI::ListData const& GUI::ZCListData::scripttypes()
{
	return script_types;
};

static const GUI::ListData pattern_types
{
	{ "Spawn (Classic)", 0},
	{ "Enter from Sides (Consecutive)", 1},
	{ "Enter from Sides (Random)", 2},
	{ "Fall From Ceiling (Classic)", 3},
	{ "Fall From Ceiling (Random)", 4},
	{ "Spawn (Random)", 5},
	{ "Spawn No Enemies", 6}
};

GUI::ListData const& GUI::ZCListData::patterntypes()
{
	return pattern_types;
};