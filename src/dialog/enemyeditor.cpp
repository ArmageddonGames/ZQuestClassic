#include "enemyeditor.h"
#include "gui/key.h"
#include "info.h"
#include "alert.h"
#include "base/zsys.h"
#include "tiles.h"
#include "gui/builder.h"
#include "zc_list_data.h"
#include "items.h"
#include "base/qrs.h"
#include <fmt/format.h>
#include "zinfo.h"
#include "defdata.h"

extern bool saved;
extern zcmodule moduledata;
extern guydata* guysbuf;
extern itemdata* itemsbuf;
extern int32_t CSet;
extern char* guy_string[eMAXGUYS];
extern const char* old_guy_string[OLDMAXGUYS];
extern script_data* guyscripts[NUMSCRIPTGUYS];
extern script_data* ewpnscripts[NUMSCRIPTWEAPONS];
char* ordinal(int32_t num);
using std::string;
using std::to_string;

static size_t guy_tabs[15] = { 0 };
static bool guy_use_script_data = true;

static bool edited = false;

bool call_enemy_editor(int32_t index)
{
	guy_use_script_data = zc_get_config("zquest", "show_enemyscript_meta_attribs", 1) ? true : false;
	edited = false;
	EnemyEditorDialog(index).show();
	return edited;
}

EnemyEditorDialog::EnemyEditorDialog(guydata const& ref, int32_t index) :
	//pointer crap

	index(index), local_guyref(ref), enemy_name(guy_string[index]),
	spawn_type(ref.flags& guy_fade_instant ? 2 : ref.flags & guy_fade_flicker ? 1 : 0),
	death_type(0), //unimplemented for now
	list_enemies(GUI::ZCListData::enemies(false, true)),
	list_families(GUI::ZCListData::efamilies()),
	list_animations(GUI::ZCListData::eanimations()),
	list_guyscripts(GUI::ZCListData::npc_script()),
	list_ewpnscripts(GUI::ZCListData::eweapon_script()),
	list_sfx(GUI::ZCListData::sfxnames(true)),
	list_counters_nn(GUI::ZCListData::counters(true, true)),
	list_dropsets(GUI::ZCListData::dropsets(false)),
	list_sprites(GUI::ZCListData::miscsprites()),
	list_eweaptype(GUI::ZCListData::eweaptypes()),
	list_defenses(GUI::ZCListData::defenses()),
	list_deftypes(GUI::ZCListData::deftypes())
{}

EnemyEditorDialog::EnemyEditorDialog(int32_t index) :
	EnemyEditorDialog(guysbuf[index], index) 
{}

static const GUI::ListData list_walkmisc1
{
	{"1 Shot",0},
	{"1 (End-Halt)",1},
	{"Rapid-Fire",2},
	{"1 (Fast)",3},
	{"1 (Slanted)",4},
	{"3 Shots",5},
	{"4 Shots",6},
	{"5 Shots",7},
	{"3 (Fast)",8},
	{"Breath",9},
	{"8 Shots",10},
	{"Summon",11},
	{"Summon (Layer)",12},
};

static const GUI::ListData list_walkmisc2
{
	{"Normal",0},
	{"Split On Hit",1},
	{"Split On Death",2},
	{"8 Shots",3},
	{"Explode",4},
	{"Tribble",5},
};

static const GUI::ListData list_walkmisc7
{
	{"None",0},
	{"Temp. Jinx",1},
	{"Perm. Jinx",2},
	{"Cure Jinx",3},
	{"Lose Magic",4},
	{"Lose Rupees",5},
	{"Drunk",6},
	{"Eat (Items)",7},
	{"Eat (Magic)",8},
	{"Eat (Rupees)",9},
	{"Eat (Damage)",10},
};

static const GUI::ListData list_walkmisc9
{
	{"Normal",0},
	{"Rope",1},
	{"Vire",2},
	{"Pols Voice",3},
	{"Armos",4},
};

static const GUI::ListData list_gleeokmisc3
{
	{"1 Shot",0},
	{ "Breath",1}
};

static const GUI::ListData list_gohmamisc1
{
	{"1 Shot",0},
	{"3 Shots",0},
	{"Breath",2}
};

static const GUI::ListData list_manhandlamisc2
{
	{"1x1 (4 Heads)",0},
	{"2x2 (8 Heads)",1},
};

static const GUI::ListData list_aquamisc1
{
	{"Right (Facing Left)", 0},
	{"Left (Facing Right)", 1},
};

static const GUI::ListData list_patramisc4
{
	{"Big Circle",0},
	{"Oval",1},
};

static const GUI::ListData list_patramisc5
{
	{"None",0},
	{"Center Eye",1},
	{"Inner Eyes",2},
	{"Inner + Center",3},
};

static const GUI::ListData list_patramisc10
{
	{"1x1 Patra", 0},
	{"2x2 Patra", 1},
};

static const GUI::ListData list_patramisc20
{
	{"Random (Any)",0},
	{"Random (Single)",1},
	{"Barrage",2},
	{"Ring",3},
	{"Stream",4},
};

static const GUI::ListData list_patramisc22
{
	{"Never",0},
	{"When Expanding",1},
	{"Expand + Warning",2},
};

static const GUI::ListData list_patramisc25
{
	{"Always",0},
	{"When Vulnerable",1},
	{"When Invulnerable",2},
	{"When Behind 1 Layer",3},
};

static const GUI::ListData list_patramisc26
{
	{"Always",0},
	{"With Outer Ring",1},
	{"Without Outer Ring",2},
};

static const GUI::ListData list_patramisc28
{
	{"1 Shot",0},
	{"1 (Fast)",1},
	{"3 Shots",2},
	{"3 (Fast)",3},
	{"5 Shots",4},
	{"5 (Fast)",5},
	{"4 (Cardinal)",6},
	{"4 (Diagonal)",7},
	{"4 (Card/Diag Random)",8},
	{"8 Shots",9},
	{"Breath",10},
	{"Stream",11},
};

static const GUI::ListData list_dodongomisc10
{
	{"NES",0},
	{"BS-Zelda",1},
};

static const GUI::ListData list_digdoggermisc10
{
	{"Digdogger",0},
	{"Kid",1},
};

static const GUI::ListData list_wizzrobemisc1
{
	{"Teleport",0},
	{ "Phase",1 }
};

static const GUI::ListData list_wizzrobemisc2
{
	{"1 shot", 0},
	{"8 shot", 1},
	{"Summon", 2},
	{"Summon (Layer)", 3}
};

static const GUI::ListData list_keesemisc1
{
	{"Keese",0},
	{"Bat",1},
	{"Keese(Fast)",2},
};

static const GUI::ListData list_keesemisc2
{
	{"Normal",0},
	{"Tribble",1},
};

static const GUI::ListData list_trapmisc1
{
	{"4-Way",0},
	{"Horizontal",1},
	{"Vertical",2},
};

static const GUI::ListData list_trapmisc2
{
	{"Line Of Sight",0},
	{"Constant",1}
};

static const GUI::ListData list_leevermisc1
{
	{"Hero's path",0},
	{"In place",1},
	{"Hero's path + Random",2}
};

static const GUI::ListData list_rockmisc10
{
	{"1x1",0},
	{"2x2",1},
};

// 0: no, 1: yes
static const GUI::ListData list_yesnomisc
{
	{"No",0},
	{"Yes",1},
};

// 0: yes, 1: no
static const GUI::ListData list_noyesmisc
{
	{"Yes",0},
	{"No",1},
};

static const GUI::ListData list_null
{
	{"NOPE",0},
};

static const GUI::ListData list_spawntype
{
	{"Puff",0},
	{"Flicker",1},
	{"Instant",2},
};

static const GUI::ListData list_deathtype
{
	{"Normal",0},
	{"Explode",1},
	{"Eight Shards",2}, //Ganon
	{"Robot Master",3}, //Megaman
	{"Disintegrate",4}, //BS GANON
};

void EnemyEditorDialog::refreshScript()
{
	loadEnemyType();
	//active
	int32_t sw_initd[8];
	for (auto q = 0; q < 8; ++q)
	{
		l_initd[q] = "InitD[" + to_string(q) + "]:";
		h_initd[q].clear();
		sw_initd[q] = -1;
	}
	if (local_guyref.script)
	{
		zasm_meta const& meta = guyscripts[local_guyref.script]->meta;
		for (auto q = 0; q < 8; ++q)
		{
			if (unsigned(meta.initd_type[q]) < nswapMAX)
				sw_initd[q] = meta.initd_type[q];
			if (meta.initd[q].size())
				l_initd[q] = meta.initd[q];
			if (meta.initd_help[q].size())
				h_initd[q] = meta.initd_help[q];
		}
	}
	else
	{
		for (auto q = 0; q < 8; ++q)
			sw_initd[q] = nswapDEC;
	}
	//weapon
	int32_t sw_wpninitd[8];
	for (auto q = 0; q < 8; ++q)
	{
		l_wpninitd[q] = "InitD[" + to_string(q) + "]:";
		h_wpninitd[q].clear();
		sw_wpninitd[q] = -1;
	}
	if (local_guyref.script)
	{
		zasm_meta const& meta = ewpnscripts[local_guyref.weaponscript]->meta;
		for (auto q = 0; q < 8; ++q)
		{
			if (unsigned(meta.initd_type[q]) < nswapMAX)
				sw_wpninitd[q] = meta.initd_type[q];
			if (meta.initd[q].size())
				l_wpninitd[q] = meta.initd[q];
			if (meta.initd_help[q].size())
				h_wpninitd[q] = meta.initd_help[q];
		}
	}
	else
	{
		for (auto q = 0; q < 8; ++q)
			sw_wpninitd[q] = nswapDEC;
	}

	for (auto q = 0; q < 8; ++q)
	{
		//active
		ib_initds[q]->setDisabled(h_initd[q].empty());
		l_initds[q]->setText(l_initd[q]);
		if (sw_initd[q] > -1)
			tf_initd[q]->setSwapType(sw_initd[q]);
		//weapon
		ib_wpninitds[q]->setDisabled(h_wpninitd[q].empty());
		l_wpninitds[q]->setText(l_wpninitd[q]);
		if (sw_wpninitd[q] > -1)
			tf_wpninitd[q]->setSwapType(sw_wpninitd[q]);
	}
}

void EnemyEditorDialog::loadEnemyType()
{
	//so let me explain whats going on here 
	//1: set the default labels
	//2: overrride those labels base off the enemy type
	//3: initialize the attribute labels and switchers (do note the switchers are initialized at the position storing the textfields)
	//4: override the switchers base off the enemy type that use DDLs and set those up.
	//5: call attribute update function, since attributes updated
	// all of this must be done in this order... -Jambu
	for (size_t q = 0; q < 32; ++q)
	{
		plist_attributes[q] = nullptr;
	}
	for (int q = 0; q < 32; ++q)
		l_attribute[q] = fmt::format("Misc Attr. {}:", q + 1);
	l_bflag[0] = "Enemy is Completely Invisible";
	l_bflag[1] = "Item Specified in Attributes 13 Dispels Invisibility";
	l_bflag[2] = "Doesn't always return";
	l_bflag[3] = "Draw Invisible as Cloaked";
	l_bflag[4] = "BFlags[4]";
	l_bflag[5] = "BFlags[5]";
	l_bflag[6] = "BFlags[6]";
	l_bflag[7] = "BFlags[7]";
	l_bflag[8] = "BFlags[8]";
	l_bflag[9] = "BFlags[9]";
	l_bflag[10] = "BFlags[10]";
	l_bflag[11] = "Toggle Move Offscreen";
	l_bflag[12] = "Fast Drawing";
	l_bflag[13] = "Ignore Sideview Ladders/Platforms";
	l_bflag[14] = "Move Off-Grid (WIP)";
	l_bflag[15] = "Render Cloaked Instead of VISIBLE";

	//These need to be moved else where to be honest
	l_attribute[12] = "This Item Dispels Invisibility:";
	l_attribute[14] = "Transform Effect:";
	l_attribute[15] = "Transform Into Enemy:";
	l_attribute[16] = "Transform Inv Time:";
	
	switch(local_guyref.family)
	{
		case eeWALK:
		{
			l_attribute[0] = "Shot Type:";
			l_attribute[1] = "Death Type:";
			l_attribute[2] = "Death Attr. 1:";
			l_attribute[3] = "Death Attr. 2:";
			l_attribute[4] = "Death Attr. 3:";
			l_attribute[5] = "Extra Shots:";
			l_attribute[6] = "Touch Effects:";
			l_attribute[7] = "Effect Strength:";
			l_attribute[8] = "Walk Style:";
			l_attribute[9] = "Walk Attribute:";
			l_bflag[4] = "Split in Place";
			break;
		}
		case eeGLEEOK:
		{
			l_attribute[0] = "Heads:";
			l_attribute[1] = "Head HP:";
			l_attribute[2] = "Shot Type:";
			l_attribute[3] = "Unused";
			l_attribute[4] = "Neck Segments:";
			l_attribute[5] = "Neck Offset 1:";
			l_attribute[6] = "Neck Offset 2:";
			l_attribute[7] = "Head Offset";
			l_attribute[8] = "Flying Head Offset";
			l_attribute[9] = "Unused";
			l_bflag[2] = "BFlags[2]";
			l_bflag[4] = "Obeys Spawn Points";
			l_bflag[5] = "Center Spawn X Point";
			l_bflag[6] = "Heads Block Projectiles";
			break;
		}
		case eeDIG:
		{
			l_attribute[0] = "Enemy 1 ID:";
			l_attribute[1] = "Enemy 2 ID:";
			l_attribute[2] = "Enemy 3 ID:";
			l_attribute[3] = "Enemy 4 ID:";
			l_attribute[4] = "Enemy 1 Qty:";
			l_attribute[5] = "Enemy 2 Qty:";
			l_attribute[6] = "Enemy 3 Qty:";
			l_attribute[7] = "Enemy 4 Qty:";
			l_attribute[8] = "Unused";
			l_attribute[9] = "Type";
			l_bflag[2] = "BFlags[2]";
			break;
		}
		case eePATRA:
		{
			l_attribute[0] = "Outer Eyes:";
			l_attribute[1] = "Inner Eyes:";
			l_attribute[2] = "Eyes' HP:";
			l_attribute[3] = "Eye Movement:";
			l_attribute[4] = "Shooters:";
			l_attribute[5] = "Pattern Odds:";
			l_attribute[6] = "Pattern Cycles:";
			l_attribute[7] = "Eye Offset:";
			l_attribute[8] = "Eye CSet:";
			l_attribute[9] = "Type:";
			l_attribute[17] = "Shot chance (1/N per frame):";
			l_attribute[18] = "Firing cooldown:";
			l_attribute[19] = "Inner Eyes Firing Pattern:";
			l_attribute[20] = "Warning Spins:";
			l_attribute[21] = "Stays Still:";
			l_attribute[22] = "Outer Ring Loss Speed Boost:";
			l_attribute[23] = "Inner Ring Loss Speed Boost:";
			l_attribute[24] = "Can Fire:";
			l_attribute[25] = "Can Expand:";
			l_attribute[26] = "Inner Eye HP:";
			l_attribute[27] = "Center Eye Firing Pattern:";
			l_attribute[28] = "Outer Eye Radius:";
			l_attribute[29] = "Inner Eye Radius:";
			l_attribute[30] = "Outer Eye Expanded Radius:";
			l_attribute[31] = "Inner Eye Expanded Radius:";
			l_bflag[2] = "Has Firing Animation";
			l_bflag[4] = "Obeys Spawn Points";
			l_bflag[5] = "Slow down when Firing";
			l_bflag[6] = "Don't attack when expanding";
			l_bflag[7] = "Don't expand when spawned";
			l_bflag[8] = "Pattern Odds reset when Expanding";
			l_bflag[9] = "Don't 'expand' without orbiters";
			break;
		}
		case eePROJECTILE:
		{
			l_attribute[0] = "Shot Type:";
			l_attribute[2] = "Shot Attribute 1:";
			l_attribute[3] = "Shot Attribute 2:";
			break;
		}
		case eeGHOMA:
		{
			l_attribute[0] = "Shot Type:";
			l_bflag[2] = "BFlags[2]";
			l_bflag[4] = "Obeys Spawn Points";
			break;
		}
		case eeAQUA:
		{
			l_attribute[0] = "Side:";
			l_bflag[2] = "BFlags[2]";
			l_bflag[4] = "Obeys Spawn Points";
			break;
		}
		case eeMANHAN:
		{
			l_attribute[0] = "Frame Rate:";
			l_attribute[1] = "Size";
			l_bflag[2] = "BFlags[2]";
			break;
		}
		case eeLANM:
		{
			l_attribute[0] = "Segments:";
			l_attribute[1] = "Segment Lag:";
			l_attribute[2] = "Item per segment:";
			l_bflag[2] = "BFlags[2]";
			l_bflag[4] = "Obeys Spawn Points";
			break;
		}
		case eeMOLD:
		{
			l_attribute[0] = "Segments:";
			l_attribute[1] = "Item per segment:";
			l_bflag[2] = "BFlags[2]";
			l_bflag[4] = "Obeys Spawn Points";
			break;
		}
		case eeWIZZ:
		{
			l_attribute[0] = "Walk Style:";
			l_attribute[1] = "Shot Type:";
			l_attribute[2] = "Shot Attr. 1:";
			l_attribute[3] = "Solid Combos OK:";
			l_attribute[4] = "Teleport Delay:";
			l_bflag[4] = "Old Windrobe Teleport";
			break;
		}
		case eeDONGO:
		{
			l_attribute[9] = "Type:";
			l_bflag[2] = "BFlags[2]";
			break;
		}
		case eeKEESE:
		{
			l_attribute[0] = "Walk Style:";
			l_attribute[1] = "Death Type:";
			l_attribute[2] = "Enemy Id:";
			l_attribute[13] = "Landing Chance (1/N):",
			l_attribute[14] = "Landing Cooldown:",
			l_attribute[15] = "Halt Duration:";
			l_attribute[16] = "Acceleration Frame Interval";
			l_attribute[17] = "Acceleration Step Modifier:";
			l_attribute[18] = "Spawn Step:";
			l_attribute[19] = "Tribble Timer";
			break;
		}
		case eeTEK:
		{
			l_attribute[0] = "1/N Jump Start:";
			l_attribute[1] = "1/N Jump Cont.:";
			l_attribute[2] = "Jump Z Velocity:";
			break;
		}
		case eeLEV:
		{
			l_attribute[0] = "Emerge style:";
			l_attribute[1] = "Submerged CSet:";
			l_attribute[2] = "Emerge step:";
			break;
		}
		case eeWALLM:
		{
			l_attribute[0] = "Fixed Distance";
			break;
		}
		case eeTRAP:
		{
			l_attribute[0] = "Direction:";
			l_attribute[1] = "Move Style:";
			break;
		}
		case eeROCK:
		{
			l_attribute[9] = "Size:";
			
			break;
		}
		case eeNONE:
		{
			l_attribute[9] = "Boss Death Trigger:";

			break;
		}
		case eeGHINI: case eePEAHAT: //TODO DEPRECIATE ME
		{
			l_attribute[0] = "Stop On Pits";
			l_attribute[13] = "Landing Chance (1/N):",
			l_attribute[14] = "Landing Cooldown:",
			l_attribute[15] = "Halt Duration:";
			l_attribute[16] = "Acceleration Factor:";
			break;
		}
		case eeZORA:
		{
			l_bflag[4] = "Lava Only";
			l_bflag[5] = "Lava and Water";
			l_bflag[6] = "Shallow Water";
			l_bflag[7] = "Doesn't regenerate health";
			break;
		}
		case eeSPINTILE:
		{
			//no attributes or flags either (yet)
			break;
		}
		case eeGANON:
		{
			l_attribute[13] = "Death Shot Item ID"; //TODO MOVE ME TO MISC ZERO AND ADD ME TO DEFAULT DATA FOR OLD QUEST
			break;
		}
		default: //script and friendly families
		{
			break;
		}
	}

#define SW_TEXTFIELD 0
#define SW_DROPDOWN 1
	
	for (size_t q = 0; q < 32; ++q)
	{
		l_attributes[q]->setText(l_attribute[q]);
		ib_attributes[q]->setDisabled(h_attribute[q].empty());
		pbtn_attributes[q]->setDisabled(!plist_attributes[q]);
		tf_attributes[q]->setLowBound(-999999);
		tf_attributes[q]->setHighBound(999999);
		sw_attributes[q]->switchTo(SW_TEXTFIELD);

		if (q > 15)
			continue;

		l_bflags[q]->setText(l_bflag[q]);
		//ib_bflags[q]->setDisabled(h_bflag[q].empty());
	}

	switch (local_guyref.family)
	{
	case eeWALK:
		ddl_attributes[0]->setListData(list_walkmisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		ddl_attributes[1]->setListData(list_walkmisc2);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		ddl_attributes[6]->setListData(list_walkmisc7);
		sw_attributes[6]->switchTo(SW_DROPDOWN);
		ddl_attributes[8]->setListData(list_walkmisc9);
		sw_attributes[8]->switchTo(SW_DROPDOWN);
		break;
	case eeGLEEOK:
		ddl_attributes[2]->setListData(list_gleeokmisc3);
		sw_attributes[2]->switchTo(SW_DROPDOWN);
		break;
	case eeDIG:
		ddl_attributes[0]->setListData(list_enemies);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		ddl_attributes[1]->setListData(list_enemies);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		ddl_attributes[2]->setListData(list_enemies);
		sw_attributes[2]->switchTo(SW_DROPDOWN);
		ddl_attributes[3]->setListData(list_enemies);
		sw_attributes[3]->switchTo(SW_DROPDOWN);
		ddl_attributes[9]->setListData(list_digdoggermisc10);
		sw_attributes[9]->switchTo(SW_DROPDOWN);
		break;
	case eePATRA:
		ddl_attributes[3]->setListData(list_patramisc4);
		sw_attributes[3]->switchTo(SW_DROPDOWN);
		ddl_attributes[4]->setListData(list_patramisc5);
		sw_attributes[4]->switchTo(SW_DROPDOWN);
		ddl_attributes[9]->setListData(list_patramisc10);
		sw_attributes[9]->switchTo(SW_DROPDOWN);
		ddl_attributes[19]->setListData(list_patramisc20);
		sw_attributes[19]->switchTo(SW_DROPDOWN);
		ddl_attributes[21]->setListData(list_patramisc22);
		sw_attributes[21]->switchTo(SW_DROPDOWN);
		ddl_attributes[24]->setListData(list_patramisc25);
		sw_attributes[24]->switchTo(SW_DROPDOWN);
		ddl_attributes[25]->setListData(list_patramisc26);
		sw_attributes[25]->switchTo(SW_DROPDOWN);
		ddl_attributes[27]->setListData(list_patramisc28);
		sw_attributes[27]->switchTo(SW_DROPDOWN);
		break;
	case eePROJECTILE:
		ddl_attributes[0]->setListData(list_walkmisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		break;
	case eeGHOMA:
		ddl_attributes[0]->setListData(list_gohmamisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		break;
	case eeAQUA:
		ddl_attributes[0]->setListData(list_aquamisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		break;
	case eeMANHAN:
		ddl_attributes[1]->setListData(list_manhandlamisc2);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		break;
	case eeLANM:
		ddl_attributes[2]->setListData(list_yesnomisc);
		sw_attributes[2]->switchTo(SW_DROPDOWN);
		break;
	case eeMOLD:
		ddl_attributes[1]->setListData(list_yesnomisc);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		break;
	case eeWIZZ:
		ddl_attributes[0]->setListData(list_wizzrobemisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		ddl_attributes[1]->setListData(list_wizzrobemisc2);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		ddl_attributes[3]->setListData(list_yesnomisc);
		sw_attributes[3]->switchTo(SW_DROPDOWN);
		break;
	case eeDONGO:
		ddl_attributes[9]->setListData(list_dodongomisc10);
		sw_attributes[9]->switchTo(SW_DROPDOWN);
		break;
	case eeKEESE: //FLOATER
		ddl_attributes[0]->setListData(list_keesemisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		ddl_attributes[1]->setListData(list_keesemisc2);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		ddl_attributes[2]->setListData(list_enemies);
		sw_attributes[2]->switchTo(SW_DROPDOWN);
		break;
	case eeLEV:
		ddl_attributes[0]->setListData(list_leevermisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		break;
	case eeWALLM:
		ddl_attributes[0]->setListData(list_noyesmisc);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		break;
	case eeTRAP:
		ddl_attributes[0]->setListData(list_trapmisc1);
		sw_attributes[0]->switchTo(SW_DROPDOWN);
		ddl_attributes[1]->setListData(list_trapmisc2);
		sw_attributes[1]->switchTo(SW_DROPDOWN);
		break;
	case eeROCK:
		ddl_attributes[9]->setListData(list_rockmisc10);
		sw_attributes[9]->switchTo(SW_DROPDOWN);
		break;
	case eeNONE: //(Boss Death Triggers)
		ddl_attributes[9]->setListData(list_yesnomisc);
		sw_attributes[9]->switchTo(SW_DROPDOWN);
		break;
	default:
		break;
	}
	
	loadAttributes();
}

void EnemyEditorDialog::loadAttributes()
{
	switch(local_guyref.family)
	{
		case eeWALK:
		{
			static const int dependent_attribs[] = {2,3,4,7,9};
			for(auto q : dependent_attribs)
			{
				l_attribute[q] = "Unused:";
				h_attribute[q] = "";
				plist_attributes[q] = nullptr;
			}
			GUI::ListData const* lists[12] = { nullptr };
			
			if(local_guyref.attributes[0] == e1tSUMMON)
			{
				l_attribute[2] = "Enemy ID:";
				h_attribute[2] = "Enemy to Summon";
				l_attribute[3] = "Enemy Count:";
				h_attribute[3] = "Max number of the specified enemy that can be on-screen before summoner stops";
				lists[2] = &list_enemies;
			}
			else
			{
				switch(local_guyref.attributes[1])
				{
					case e2tTRIBBLE:
						lists[2] = &list_enemies;
						l_attribute[2] = "Enemy ID:";
						h_attribute[2] = "Enemy to Tribble into (transforming if not killed within a set time).";
						l_attribute[3] = "Enemy Count:";
						h_attribute[3] = "Tribble into this many copies of the specified enemy.";
						l_attribute[4] = "Tribble Time:";
						h_attribute[4] = "Tribbles into specified enemy after this many frames. If 0, defaults to 256 frames.";
						break;
					case e2tSPLITHIT:
					case e2tSPLIT:
						lists[2] = &list_enemies;
						l_attribute[2] = "Enemy ID:";
						h_attribute[2] = "Enemy to Split into.";
						l_attribute[3] = "Enemy Count:";
						h_attribute[3] = "Split into this many copies of the specified enemy.";
						break;
					case e2tFIREOCTO:
					case e2tBOMBCHU:
						l_attribute[2] = "Weapon Offset:";
						h_attribute[2] = "This number is added to the enemy's 'Weapon' dropdown value to get the weapon used for the death effect. Yes this is confusing.";
						l_attribute[3] = "Weapon Dmg:";
						h_attribute[3] = "Damage of the weapon(s) spawned by the death effect";
						break;
				}
			}
			
			switch(local_guyref.attributes[6])
			{
				case e7tTEMPJINX:
				case e7tPERMJINX:
				case e7tUNJINX:
				{
					l_attribute[7] = "Jinx Type(s):";
					static const vector<CheckListInfo> jinxtype_checklist =
					{
						{ "Sword Jinx" }, { "Item Jinx" }, { "Shield Jinx" }
					};
					plist_attributes[7] = &jinxtype_checklist;
					break;
				}
				case e7tTAKEMAGIC:
				case e7tTAKERUPEES:
				{
					l_attribute[7] = "Amount:";
					break;
				}
				case e7tDRUNK:
				{
					l_attribute[7] = "Drunk Time:";
					h_attribute[7] = "Time, in frames, the effect lasts";
					break;
				}
				case e7tEATITEMS:
				case e7tEATMAGIC:
				case e7tEATRUPEES:
				{
					l_attribute[7] = "Delay:";
					h_attribute[7] = "Time, in frames, between each eating";
					break;
				}
				case e7tEATHURT:
				{
					l_attribute[7] = "Reserved:";
					break;
				}
			}
			
			switch(local_guyref.attributes[8])
			{
				case e9tROPE:
					l_attribute[9] = "Charge Step:";
					h_attribute[9] = "Step speed used when the enemy charges forward.";
					break;
				case e9tVIRE:
				case e9tPOLSVOICE:
					l_attribute[9] = "Reserved:";
					break;
				case e9tARMOS:
					l_attribute[9] = "Other Step:";
					h_attribute[9] = fmt::format("Has a 50% chance to use this step speed instead of the enemy's normal step speed."
						" If the animation style is set to '{}', adds 20 to the tile of the enemy when this step is used.",
						list_animations.findText(aARMOS));
					break;
			}
			
			for(auto q : dependent_attribs)
			{
				if(lists[q])
				{
					ddl_attributes[q]->setListData(*lists[q]);
					ddl_attributes[q]->setSelectedValue(local_guyref.attributes[q]);
					sw_attributes[q]->switchTo(SW_DROPDOWN);
				}
				else sw_attributes[q]->switchTo(SW_TEXTFIELD);
				
				l_attributes[q]->setText(l_attribute[q]);
				ib_attributes[q]->setDisabled(h_attribute[q].empty());
				
				pbtn_attributes[q]->setDisabled(!plist_attributes[q]);
			}
			break;
		}
	}
	updateWarnings();
	pendDraw();
}

void EnemyEditorDialog::updateWarnings()
{
	warnings.clear();
	switch(local_guyref.family)
	{
		case eeWALK:
			if(local_guyref.attributes[0] == e1tSUMMON)
			{
				switch(local_guyref.attributes[1])
				{
					case e2tTRIBBLE:
					case e2tSPLITHIT:
					case e2tSPLIT:
					case e2tFIREOCTO:
					case e2tBOMBCHU:
						warnings.emplace_back("'Shot Type: Summon' is incompatible with selected 'Death Type'!");
						break;
				}
			}
			break;
	}
	
	warnbtn->setDisabled(warnings.empty());
}

//{ Widgets
#define ATTR_WID 6_em
#define ATTR_LAB_WID 12_em
#define SPR_LAB_WID 10_em
#define ACTION_LAB_WID 6_em
#define ACTION_FIELD_WID 6_em
#define FLAGS_WID 24_em

std::shared_ptr<GUI::Widget> EnemyEditorDialog::NumberField(auto* data, int32_t _min, int32_t _max, int _length, bool _disabled)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return TextField(
		type = GUI::TextField::type::INT_DECIMAL,
		maxLength = _length,
		disabled = _disabled,
		hAlign = 1.0, fitParent = true,
		val = *data,
		low = _min,
		high = _max,
		onValChangedFunc = [data](GUI::TextField::type, std::string_view, int32_t val)
		{
			*data = val;
		}
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::NameField(string const& str)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Row(
		Label(text=str, hAlign=1.0, rightPadding=0_px),
		TextField(
			type = GUI::TextField::type::TEXT,
			maxLength = 63,
			fitParent = true,
			text = enemy_name,
			onValChangedFunc = [&](GUI::TextField::type, std::string_view text, int32_t)
			{
				enemy_name = text;
			}
		)
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::DropDownField(auto* field, GUI::ListData const& ls, bool _disabled)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return DropDownList(
		data = ls,
		disabled = _disabled,
		hAlign = 0.0,
		selectedValue = *field,
		fitParent = true,
		onSelectFunc = [field](int32_t val)
		{
			*field = val;
		}
	);
}

inline bool EnemyEditorDialog::NoDefenses()
{
	return local_guyref.family == eeROCK || local_guyref.family == eeTRAP || local_guyref.family == eeDONGO || local_guyref.family == eeGANON;
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::DefenseField(auto* indexs, bool _dobutton)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	std::shared_ptr<GUI::Grid> grid = Rows<3>();
	for (int32_t q = 0; q < sizeof(indexs)+1; q++)
	{
		int32_t index = indexs[q];
		if (index == -1)
		{
			grid->add(_d);
			grid->add(_d);
			grid->add(_d);
			return grid;
		}
		GUI::ListItem& li = list_defenses.accessItem(index);
		ddl_defenses[index] = DropDownList(
			disabled = NoDefenses(),
			data = list_deftypes,
			hAlign = 0.0,
			selectedValue = local_guyref.defense[index],
			fitParent = true,
			onSelectFunc = [&, index](int32_t val)
			{
				local_guyref.defense[index] = val;
			});
		grid->add(Label(
			text = li.text,
			hAlign = 1.0,
			rightPadding = 0_px,
			disabled = NoDefenses()));
		grid->add(ddl_defenses[index]);
		if (q == 0 && _dobutton)
			grid->add(Button(
				fitParent = true,
				disabled = NoDefenses(),
				text = "Set All",
				minwidth = 40_px,
				height = 1.5_em,
				onClick = message::SETALLDEFENSE
			));
		else grid->add(_d);
	}
	return grid;
}

//Flags Tab

std::shared_ptr<GUI::Widget> EnemyEditorDialog::SizeFlag(int32_t index)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = "Enabled",
		checked = local_guyref.SIZEflags & index,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.SIZEflags, index, state);
		}
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::GuyFlag(guy_flags index, string const& str, bool _disabled)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = str,
		checked = local_guyref.flags & index,
		disabled = _disabled,
		fitParent = true,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.flags, index, state);
		}
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::EditorFlag(int32_t index)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return l_bflags[index] = Checkbox(
		minwidth = FLAGS_WID, hAlign = 0.0,
		checked = local_guyref.editorflags & (1 << index),
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.editorflags, (1 << index), state);
	});
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::MoveFlag(move_flags index, string const& str)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = str,
		checked = local_guyref.moveflags & index,
		fitParent = true,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.moveflags, index, state);
		}
	);
}

//Attack Tab

std::shared_ptr<GUI::Widget> EnemyEditorDialog::WeaponSizeFlag(int32_t index)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = "Enabled",
		checked = local_guyref.weapoverrideFLAGS & index,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.weapoverrideFLAGS, index, state);
		}
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::WeaponBlockFlag(byte index, string const& str)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = str,
		checked = local_guyref.wunblockable & index,
		fitParent = true,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.wunblockable, index, state);
		}
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::WeaponMoveFlag(move_flags index, string const& str)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Checkbox(
		text = str,
		checked = local_guyref.wmoveflags & index,
		fitParent = true,
		onToggleFunc = [&, index](bool state)
		{
			SETFLAG(local_guyref.wmoveflags, index, state);
		}
	);
}

//Effects Tab

//Script Tab

std::shared_ptr<GUI::Widget> EnemyEditorDialog::ScriptField(int index)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Row(padding = 0_px,
		l_initds[index] = Label(minwidth = ATTR_LAB_WID, hAlign = 1.0),
		ib_initds[index] = Button(forceFitH = true, text = "?",
			disabled = true,
			onPressFunc = [&, index]()
			{
				InfoDialog("InitD Info", h_initd[index]).show();
			}),
		tf_initd[index] = TextField(
			fitParent = true, minwidth = 8_em,
			type = GUI::TextField::type::SWAP_ZSINT2,
			val = local_guyref.initD[index],
			onValChangedFunc = [&, index](GUI::TextField::type, std::string_view, int32_t val)
			{
				local_guyref.initD[index] = val;
			})
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::WeaponScriptField(int index)
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Row(padding = 0_px,
		l_wpninitds[index] = Label(minwidth = ATTR_LAB_WID, hAlign = 1.0),
		ib_wpninitds[index] = Button(forceFitH = true, text = "?",
			disabled = true,
			onPressFunc = [&, index]()
			{
				InfoDialog("InitD Info", h_wpninitd[index]).show();
			}),
		tf_wpninitd[index] = TextField(
			fitParent = true, minwidth = 8_em,
			type = GUI::TextField::type::SWAP_ZSINT2,
			val = local_guyref.weap_initiald[index],
			onValChangedFunc = [&, index](GUI::TextField::type, std::string_view, int32_t val)
			{
				local_guyref.weap_initiald[index] = val;
			})
	);
}

std::shared_ptr<GUI::Widget> EnemyEditorDialog::view()
{
	using namespace GUI::Builder;
	using namespace GUI::Props;
	using namespace GUI::Key;

#define MAXHALT (local_guyref.family==eeFIRE||local_guyref.family==eeOTHER||(local_guyref.family>=eeSCRIPT01&&local_guyref.family<=eeFFRIENDLY10))?999:16
#define MAXRATE (local_guyref.family==eeFIRE||local_guyref.family==eeOTHER||(local_guyref.family>=eeSCRIPT01&&local_guyref.family<=eeFFRIENDLY10))?999:16
#define MAXHOMING (local_guyref.family==eeFIRE||local_guyref.family==eeOTHER||(local_guyref.family>=eeSCRIPT01&&local_guyref.family<=eeFFRIENDLY10))?9999:256
#define MAXSTEP (local_guyref.family==eeFIRE||local_guyref.family==eeOTHER||(local_guyref.family>=eeSCRIPT01&&local_guyref.family<=eeFFRIENDLY10))?9999:1000
#define HAS_SHIELD (local_guyref.family==eeWALK||local_guyref.family==eeFIRE||local_guyref.family==eeOTHER)
#define	TURNFREQHALTRATE local_guyref.family == eeKEESE || local_guyref.family == eeGHINI || local_guyref.family == eePEAHAT || local_guyref.family == eeMANHAN \
	|| local_guyref.family == eeGLEEOK || local_guyref.family == eePATRA || local_guyref.family == eeDIG ? "Turn Freq:" : "Halt Rate:"
#define	TURNFREQHALTRATEHINT local_guyref.family == eeKEESE || local_guyref.family == eeGHINI || local_guyref.family == eePEAHAT || local_guyref.family == eeMANHAN \
	|| local_guyref.family == eeGLEEOK || local_guyref.family == eePATRA || local_guyref.family == eeDIG ? \
	"Turn Freq: How often this enemy considers turning after moving to a new combo\nRanges from 0 (never) to 16 (always)." : \
	"Halt Rate: How often this enemy considers stopping after moving to a new combo\nRanges from 0 (never) to 16 (always)."

	char titlebuf[256];
	sprintf(titlebuf, "Enemy %d: %s", index, guy_string[index]);

	// ATTRIBUTE SWITCHERS
	std::shared_ptr<GUI::Grid> attributes1_tab = Rows<4>();
	std::shared_ptr<GUI::Grid> attributes2_tab = Rows<4>();
	std::shared_ptr<GUI::Grid> attributes3_tab = Rows<4>();

	for (int q = 0; q < 32; ++q)
	{
		auto& tab = q < 12 ? attributes1_tab : q < 22 ? attributes2_tab : attributes3_tab;

		tab->add(l_attributes[q] = Label(minwidth = ATTR_LAB_WID, textAlign = 2));
		tab->add(sw_attributes[q] = Switcher(
			tf_attributes[q] = TextField(
				width = 300_px,
				hAlign = 0.0,
				type = GUI::TextField::type::INT_DECIMAL,
				maxLength = 7,
				low = -999999,
				high = 999999,
				val = local_guyref.attributes[q],
				fitParent = true,
				onValChangedFunc = [&, q](GUI::TextField::type, std::string_view, int32_t val)
				{
					local_guyref.attributes[q] = val;
					ddl_attributes[q]->setSelectedValue(val);
					loadAttributes();
				}),
			ddl_attributes[q] = DropDownList(
				width = 300_px,
				hAlign = 0.0,
				data = list_noyesmisc,
				vPadding = 0_px,
				fitParent = true, selectedValue = local_guyref.attributes[q],
				onSelectFunc = [&, q](int32_t val)
				{
					local_guyref.attributes[q] = val;
					tf_attributes[q]->setVal(val);
					loadAttributes();
				})
		));
		tab->add(pbtn_attributes[q] = Button(forceFitH = true, text = "P",
			disabled = true,
			onPressFunc = [&, q]()
			{
				if(plist_attributes[q] && call_checklist_dialog(fmt::format("Select '{}'", l_attribute[q]), *plist_attributes[q], local_guyref.attributes[q]))
				{
					tf_attributes[q]->setVal(local_guyref.attributes[q]);
					ddl_attributes[q]->setSelectedValue(local_guyref.attributes[q]);
					loadAttributes();
				}
			}));
		tab->add(ib_attributes[q] = Button(forceFitH = true, text = "?",
			disabled = true,
			onPressFunc = [&, q]()
			{
				InfoDialog("Attribute Info", h_attribute[q]).show();
			}));
	}

	auto basics_tab = TabPanel(
		ptr = &guy_tabs[1],
		TabRef(name = "Basics", TabPanel(
			ptr = &guy_tabs[2],
			TabRef(name = "Data", Row(
				Column(
					Row(
						NameField("Name:")
					),
					Row(
						Frame(title = "Graphics", hAlign = 1.0, fitParent = true,
							Columns<3>(
								//
								Label(text = "Old"),
								tswatch[0] = SelTileSwatch(
									tile = local_guyref.tile,
									cset = local_guyref.cset,
									showFlip = false,
									showvals = false,
									onSelectFunc = [&](int32_t t, int32_t c, int32_t, int32_t)
									{
										local_guyref.tile = t;
										local_guyref.cset = local_guyref.cset == 14 ? 14 : c;
										for (int q = 0; q < 3; ++q)
											tswatch[q]->setCSet(local_guyref.cset);
									}
								),
								Rows<2>(hAlign = 1.0,
									Label(text = "W:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.width, 0, 20, 2),
									Label(text = "H:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.height, 0, 20, 2)
								),
								//
								Label(text = "Special"),
								tswatch[1] = SelTileSwatch(
									tile = local_guyref.s_tile,
									cset = local_guyref.cset,
									showFlip = false,
									showvals = false,
									onSelectFunc = [&](int32_t t, int32_t c, int32_t, int32_t)
									{
										local_guyref.s_tile = t;
										local_guyref.cset = local_guyref.cset == 14 ? 14 : c;
										for (int q = 0; q < 3; ++q)
											tswatch[q]->setCSet(local_guyref.cset);
									}
								),
								Rows<2>(hAlign = 1.0,
									Label(text = "W:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.s_width, 0, 20, 2),
									Label(text = "H:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.s_height, 0, 20, 2)
								),
								//
								Label(text = "New"),
								tswatch[2] = SelTileSwatch(
									tile = local_guyref.e_tile,
									cset = local_guyref.cset,
									showFlip = false,
									showvals = false,
									onSelectFunc = [&](int32_t t, int32_t c, int32_t, int32_t)
									{
										local_guyref.e_tile = t;
										local_guyref.cset = local_guyref.cset == 14 ? 14 : c;
										for (int q = 0; q < 3; ++q)
											tswatch[q]->setCSet(local_guyref.cset);
									}
								),
								Rows<2>(hAlign = 1.0,
									Label(text = "W:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.e_width, 0, 20, 2),
									Label(text = "H:", hAlign = 1.0, rightPadding = 0_px),
									NumberField(&local_guyref.e_height, 0, 20, 2)
								)
								//
							)
						),
						Frame(title = "Stats",
							Rows_Columns<3, 5>(hAlign = 1.0,
								Label(text = "HP:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("How many hit points this enemy has."),
								NumberField(&local_guyref.hp, 0, 32767, 5),
								Label(text = "Damage:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("How much HP the Hero loses if this enemy touches him."),
								NumberField(&local_guyref.dp, 0, 32767, 5),
								Label(text = "W. Damage:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("How much HP the Hero loses if this enemy's weapon hits him"),
								NumberField(&local_guyref.wdp, 0, 32767, 5),
								Label(text = "Hunger:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("Determines how attracted this enemy is to the Bait weapon."
									"\nThe range of values is 0 (no response) to 4 (extremely attracted)."),
								NumberField(&local_guyref.grumble, 0, 4, 1),
								Checkbox(
									text = "Use Boss CSet",
									boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
									checked = local_guyref.cset == 14,
									colSpan = 3,
									onToggleFunc = [&](bool state)
									{
										local_guyref.cset = state ? 14 : 8;
									}
								),
								Label(text = TURNFREQHALTRATE, hAlign = 1.0, rightPadding = 0_px),
								INFOBTN(TURNFREQHALTRATEHINT),
								NumberField(&local_guyref.hrate, 0, MAXHALT, 4),
								Label(text = "Random Rate:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("How often this enemy considers changing direction after stepping upon a new combo."
										"\nRanges from 0 (never)to 16 (always)."),
								NumberField(&local_guyref.rate, 0, MAXRATE, 4),
								Label(text = "Homing Factor:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("How often this enemy changes to a direction that points toward the Hero."
										"\nRanges from 0 (never)to 255 (always)."),
								NumberField(&local_guyref.homing, 0, MAXHOMING, 4),
								Label(text = "Step Speed:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("Movement speed. 100 step speed is 1 pixel per frame."
										"\nNote that only certain enemy types use this."),
								NumberField(&local_guyref.step, 0, MAXSTEP, 4),
								Label(text = "Boss CSet:", hAlign = 1.0, rightPadding = 0_px),
								INFOBTN("If enabled the enemy will use CSet 14, and load the specified ESP to CSet 14"
										"\nNote that this has no effect if Use Boss CSet to the left is unchecked."),
								NumberField(&local_guyref.bosspal, -1, 29, 2)
							)
						)
					),
					Row(
						Frame(title = "Misc", hAlign = 1.0, fitParent = true,
							Column(
								Rows<4>(rowSpacing = 0.25_em, vAlign = 1.0,
									Label(text = "Type:", hAlign = 1.0, rightPadding = 0_px),
									DropDownList(
										data = list_families,
										hAlign = 0.0,
										fitParent = true,
										selectedValue = local_guyref.family,
										onSelectionChanged = message::ENEMYTYPE
									),
									Label(text = "Item Set:", hAlign = 1.0, rightPadding = 0_px),
									DropDownField(&local_guyref.item_set, list_dropsets),
									//
									Label(text = "Old Anim:", hAlign = 1.0, rightPadding = 0_px, disabled=get_qr(qr_NEWENEMYTILES)),
									DropDownField(&local_guyref.anim, list_animations, get_qr(qr_NEWENEMYTILES)),
									Label(text = "ASpeed:", hAlign = 1.0, rightPadding = 0_px, disabled=get_qr(qr_NEWENEMYTILES)),
									NumberField(&local_guyref.frate, 0, 256, 3, get_qr(qr_NEWENEMYTILES)),
									//
									Label(text = "New Anim:", hAlign = 1.0, rightPadding = 0_px, disabled = !get_qr(qr_NEWENEMYTILES)),
									DropDownField(&local_guyref.e_anim, list_animations, !get_qr(qr_NEWENEMYTILES)),
									Label(text = "ASpeed:", hAlign = 1.0, rightPadding = 0_px, disabled = !get_qr(qr_NEWENEMYTILES)),
									NumberField(&local_guyref.e_frate, 0, 256, 3, !get_qr(qr_NEWENEMYTILES))
									//
								)
							)
						)
					)
				)
			))
		))
	);
	auto attributes_tab = TabPanel(
		ptr = &guy_tabs[3],
		TabRef(name = "Attributes", TabPanel(
			ptr = &guy_tabs[4],
			TabRef(name = "1", Row(
				attributes1_tab
			)),
			TabRef(name = "2", Row(
				attributes2_tab
			)),
			TabRef(name = "3", Row(
				attributes3_tab
			))
		))
	);
	int32_t defensearray1[10] = { edefBRANG,edefBOMB,edefSBOMB,edefARROW,edefFIRE,edefWAND,edefMAGIC,edefHOOKSHOT,edefHAMMER,edefSWORD };
	int32_t defensearray2[10] = { edefBEAM,edefREFBEAM,edefREFMAGIC,edefREFBALL,edefREFROCK,edefSTOMP,edefBYRNA,edefWhistle,edefSwitchHook,-1};
	int32_t defensearray3[10] = { edefTHROWN,edefREFARROW,edefREFFIRE,edefREFFIRE2,-1,-1,-1,-1,-1,-1 };
	int32_t defensearray4[10] = { edefSCRIPT01, edefSCRIPT02, edefSCRIPT03, edefSCRIPT04, edefSCRIPT05, edefSCRIPT06, edefSCRIPT07, edefSCRIPT08, edefSCRIPT09, edefSCRIPT10 };
	auto defenses_tab = TabPanel(
		ptr = &guy_tabs[5],
		TabRef(name = "Defenses", TabPanel(
			ptr = &guy_tabs[6],
			TabRef(name = "1", Column(
				DefenseField(defensearray1, true)
			)),
			TabRef(name = "2", Column(
				DefenseField(defensearray2)
			)),
			TabRef(name = "3", Column(
				DefenseField(defensearray3)
			)),
			TabRef(name = "Custom", Column(
				DefenseField(defensearray4)
			))
		))
	);
	auto flags_tab = TabPanel(
		ptr = &guy_tabs[7],
		TabRef(name = "Flags", TabPanel(
			ptr = &guy_tabs[8],
			TabRef(name = "Basic", Row(
				Column(hAlign = 1.0, fitParent = true,
					GuyFlag(guy_bhit, "Damaged By Power 0 weapons"),
					GuyFlag(guy_invisible, "Does not draw"),
					GuyFlag(guy_never_return, "Never Returns After Death"),
					GuyFlag(guy_doesnt_count, "Doesn't Count as Beatable Enemy"),
					GuyFlag(guy_ignore_kill_all, "Ignores 'Kill All Enemies' effects"),
					GuyFlag(guy_lens_only, "Can Only Be Seen By Lens of Truth"),
					GuyFlag(guy_flashing, "Is Flashing"),
					GuyFlag(guy_blinking, "Is Flickering"),
					GuyFlag(guy_transparent, "Is Translucent"),
					GuyFlag(guy_shield_front, "Shield In Front", !HAS_SHIELD),
					GuyFlag(guy_shield_left, "Shield On Left", !HAS_SHIELD),
					GuyFlag(guy_shield_right, "Shield On Right", !HAS_SHIELD),
					GuyFlag(guy_shield_back, "Shield In Back", !HAS_SHIELD),
					GuyFlag(guy_bkshield, "Hammer Can Break Shield", !HAS_SHIELD)
				)
			)),
			TabRef(name = "Behavior", Row(
				Column(
					EditorFlag(0),
					EditorFlag(1),
					EditorFlag(2),
					EditorFlag(3),
					EditorFlag(4),
					EditorFlag(5),
					EditorFlag(6),
					EditorFlag(7),
					EditorFlag(8),
					EditorFlag(9),
					EditorFlag(10),
					EditorFlag(11),
					EditorFlag(12),
					EditorFlag(13),
					EditorFlag(14),
					EditorFlag(15)
				)
			)),
			TabRef(name = "Spawning", Row(
				Column(hAlign = 1.0, fitParent = true,
					GuyFlag(guy_zora, "Spawned By 'Fish' Screen Flag"),
					GuyFlag(guy_rock, "Spawned By 'Falling Rocks' Screen Flag"),
					GuyFlag(guy_trap, "Spawned By 'Corner Traps' Screen Flag"),
					GuyFlag(guy_trph, "Spawned By 'Horz Trap' Combo Type/Flag"),
					GuyFlag(guy_trpv, "Spawned By 'Vert Trap' Combo Type/Flag"),
					GuyFlag(guy_trp4, "Spawned By '4-Way Trap' Combo Type/Flag"),
					GuyFlag(guy_trplr, "Spawned By 'LF Trap' Combo Type/Flag"),
					GuyFlag(guy_trpud, "Spawned By 'UD Trap' Combo Type/Flag"),
					GuyFlag(guy_trp2, "Spawned By 'Middle Traps' Screen Flag"),
					GuyFlag(guy_fire, "Spawned By 'Shooting Statues' Screen Flag"),
					GuyFlag(guy_armos, "Spawned By 'Armos' Combo Type"),
					GuyFlag(guy_ghini, "Spawned By 'Grave' Combo Type"),
					GuyFlag(guy_ganon, "Spawned By 'Ganon' Room Type"),
					GuyFlag(guy_ignoretmpnr, "Ignores 'Temp No Return'")
				)
			)),
			TabRef(name = "Movement", Column(
				Frame(title = "Basic Movement", hAlign = 1.0, fitParent = true,
					Column(hAlign = 1.0, fitParent = true,
						MoveFlag(move_obeys_grav, "Obeys Gravity"),
						MoveFlag(move_can_pitfall, "Can Fall Into Pitfalls"),
						MoveFlag(move_can_pitwalk, "Can Walk Over Pitfalls"),
						MoveFlag(move_can_waterdrown, "Can Drown In Liquid"),
						MoveFlag(move_can_waterwalk, "Can Walk On Liquid"),
						MoveFlag(move_new_movement, "Use 'Scripted Movement' for engine movement"),
						MoveFlag(move_not_pushable, "Cannot be pushed by moving solid objects")
					)
				),
				Frame(title = "Scripted Movement", hAlign = 1.0, fitParent = true,
					Column(hAlign = 1.0, fitParent = true,
						MoveFlag(move_only_waterwalk, "Can ONLY walk on Liquid"),
						MoveFlag(move_only_shallow_waterwalk, "Can ONLY walk on Shallow Liquid"),
						MoveFlag(move_only_pitwalk, "Can ONLY walk on Pitfalls"),
						MoveFlag(move_ignore_solidity, "Can walk through Solidity"),
						MoveFlag(move_ignore_blockflags, "Can walk through No Enemies Flags"),
						MoveFlag(move_ignore_screenedge, "Can walk through Screen Edge")
					)
				)
			)),
			TabRef(name = "Size", Row(
				Columns_Rows<10, 3>(hAlign = 1.0, vAlign = 1.0,
					Label(text = "TileWidth:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "TileHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitWidth:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitZHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitXOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitYOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "DrawZOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "DrawXOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "DrawYOffset:", hAlign = 1.0, rightPadding = 0_px),
					NumberField(&local_guyref.txsz, 0, 4, 1),
					NumberField(&local_guyref.tysz, 0, 4, 1),
					NumberField(&local_guyref.hxsz, 0, 64, 3),
					NumberField(&local_guyref.hysz, 0, 64, 3),
					NumberField(&local_guyref.hzsz, 0, 64, 3),
					NumberField(&local_guyref.hxofs, -64, 64, 3),
					NumberField(&local_guyref.hyofs, -64, 64, 3),
					NumberField(&local_guyref.zofs, -1000, 1000, 5),
					NumberField(&local_guyref.xofs, -1000, 1000, 5),
					NumberField(&local_guyref.yofs, -1000, 1000, 5),
					SizeFlag(OVERRIDE_TILE_WIDTH),
					SizeFlag(OVERRIDE_TILE_HEIGHT),
					SizeFlag(OVERRIDE_HIT_WIDTH),
					SizeFlag(OVERRIDE_HIT_HEIGHT),
					SizeFlag(OVERRIDE_HIT_Z_HEIGHT),
					SizeFlag(OVERRIDE_HIT_X_OFFSET),
					SizeFlag(OVERRIDE_HIT_Y_OFFSET),
					SizeFlag(OVERRIDE_DRAW_Z_OFFSET),
					SizeFlag(OVERRIDE_DRAW_X_OFFSET),
					SizeFlag(OVERRIDE_DRAW_Y_OFFSET)
				)
			))
		))
	);
	auto attack_tab = TabPanel(
		ptr = &guy_tabs[9],
		TabRef(name = "Attack", TabPanel(
			ptr = &guy_tabs[10],
			TabRef(name = "Weapon", Column(
				Row(
					Frame(title = "Projectile", hAlign = 1.0, fitParent = true,
						Rows<3>(hAlign = 1.0, rowSpacing = 0.5_em,
							//
							Label(text = "Type:", hAlign = 1.0, rightPadding = 0_px),
							DropDownField(&local_guyref.weapon, list_eweaptype),
							INFOBTN("The weapon type that is fired."),
							//
							Label(text = "Sprite:", hAlign = 1.0, rightPadding = 0_px),
							DropDownField(&local_guyref.wpnsprite, list_sprites),
							INFOBTN("Sprite used for this weapon."),
							//
							Label(text = "Fire Sound:", hAlign = 1.0, rightPadding = 0_px),
							DropDownField(&local_guyref.firesfx, list_sfx),
							INFOBTN("Plays when this weapon is fired."
							"\nNote: that explosions and boomerangs use the SpecialSFX"),
							//
							Label(text = "Special Sound:", hAlign = 1.0, rightPadding = 0_px),
							DropDownField(&local_guyref.specialsfx, list_sfx),
							INFOBTN("Special Sound used for only specific weapons.")
						)
					),
					Frame(title = "Stats", hAlign = 1.0, fitParent = true,
						Rows<3>(hAlign = 1.0, fitParent = true,
							//
							Label(text = "W. Damage:", hAlign = 1.0, rightPadding = 0_px),
							INFOBTN("How much HP the Hero loses if this enemy's weapon hits him"),
							NumberField(&local_guyref.wdp, 0, 32767, 5),
							//
							Label(text = "W. Step Speed:", hAlign = 1.0, rightPadding = 0_px),
							INFOBTN("Movement speed. 100 step speed is 1 pixel per frame."
								"\nNote that bomb blast and firetrails don't use this."),
							NumberField(&local_guyref.wstep, 0, 1000, 5)
							//
						)
					)
				),
				Row(
					Frame(title = "Ignore Flags", hAlign = 1.0, fitParent = true, 
						Column(hAlign = 1.0, fitParent = true,
							WeaponBlockFlag(WPNUNB_BLOCK,"Block Defense Types"),
							WeaponBlockFlag(WPNUNB_IGNR, "Ignore Defense Types"),
							WeaponBlockFlag(WPNUNB_SHLD, "Shield Block flags"),
							WeaponBlockFlag(WPNUNB_REFL, "Shield Reflect flags")
						)
					),
					Frame(title = "Movement Flags", hAlign = 1.0, fitParent = true,
						Column(hAlign = 1.0, fitParent = true,
							WeaponMoveFlag(move_obeys_grav, "Obeys Gravity"),
							WeaponMoveFlag(move_can_pitfall, "Can Fall Into Pitfalls"),
							WeaponMoveFlag(move_can_waterdrown, "Can Drown In Liquid")
						)
					)
				)
			)),
			TabRef(name = "Weapon Size", Row(
				Columns_Rows<9, 3>(hAlign = 1.0, vAlign = 1.0,
					Label(text = "TileWidth:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "TileHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitWidth:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitZHeight:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitXOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "HitYOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "DrawXOffset:", hAlign = 1.0, rightPadding = 0_px),
					Label(text = "DrawYOffset:", hAlign = 1.0, rightPadding = 0_px),
					NumberField(&local_guyref.weap_tilew, 0, 4, 1),
					NumberField(&local_guyref.weap_tileh, 0, 4, 1),
					NumberField(&local_guyref.weap_hxsz, 0, 64, 3),
					NumberField(&local_guyref.weap_hysz, 0, 64, 3),
					NumberField(&local_guyref.weap_hzsz, 0, 64, 3),
					NumberField(&local_guyref.weap_hxofs, -64, 64, 3),
					NumberField(&local_guyref.weap_hyofs, -64, 64, 3),
					NumberField(&local_guyref.weap_xofs, -1000, 1000, 5),
					NumberField(&local_guyref.weap_yofs, -1000, 1000, 5),
					WeaponSizeFlag(OVERRIDE_TILE_WIDTH),
					WeaponSizeFlag(OVERRIDE_TILE_HEIGHT),
					WeaponSizeFlag(OVERRIDE_HIT_WIDTH),
					WeaponSizeFlag(OVERRIDE_HIT_HEIGHT),
					WeaponSizeFlag(OVERRIDE_HIT_Z_HEIGHT),
					WeaponSizeFlag(OVERRIDE_HIT_X_OFFSET),
					WeaponSizeFlag(OVERRIDE_HIT_Y_OFFSET),
					WeaponSizeFlag(OVERRIDE_DRAW_X_OFFSET),
					WeaponSizeFlag(OVERRIDE_DRAW_Y_OFFSET)
				)
			)),
			TabRef(name = "Burning", Column(
				Row(
					INFOBTN("With this checked, the created weapon will use the appropriate"
						" burning sprite INSTEAD of its' normal sprite."
						"\nAdditionally, the weapon will use the specified light radius."),
					Checkbox(
						width = FLAGS_WID,
						checked = (local_guyref.flags & guy_burning_sprites),
						text = "Use Burning Sprites",
						onToggleFunc = [&](bool state)
						{
							SETFLAG(local_guyref.flags, guy_burning_sprites, state);
						}
					)
				),
				Rows<4>(
					_d, Label(text = "Sprite"), Label(text = "Light Radius"), _d,
					//
					Label(text = "No Fire:", hAlign = 1.0),
					DropDownList(
						data = list_sprites,
						selectedValue = local_guyref.burnsprs[WPNSPR_BASE],
						onSelectFunc = [&](int32_t val)
						{
							local_guyref.burnsprs[WPNSPR_BASE] = val;
						}),
					NumberField(&local_guyref.light_rads[WPNSPR_BASE], 0, 255, 3),
					INFOBTN("Settings used for the weapon when not on fire"),
					//
					Label(text = "Normal Fire:", hAlign = 1.0),
					DropDownList(
						data = list_sprites,
						selectedValue = local_guyref.burnsprs[WPNSPR_IGNITE_ANY],
						onSelectFunc = [&](int32_t val)
						{
							local_guyref.burnsprs[WPNSPR_IGNITE_ANY] = val;
						}),
					NumberField(&local_guyref.light_rads[WPNSPR_IGNITE_ANY], 0, 255, 3),
					INFOBTN("Settings used for the weapon when on 'Normal' fire"),
					//
					Label(text = "Strong Fire:", hAlign = 1.0),
					DropDownList(
						data = list_sprites,
						selectedValue = local_guyref.burnsprs[WPNSPR_IGNITE_STRONG],
						onSelectFunc = [&](int32_t val)
						{
							local_guyref.burnsprs[WPNSPR_IGNITE_STRONG] = val;
						}),
					NumberField(&local_guyref.light_rads[WPNSPR_IGNITE_STRONG], 0, 255, 3),
					INFOBTN("Settings used for the weapon when on 'Strong' fire"),
					//
					Label(text = "Magic Fire:", hAlign = 1.0),
					DropDownList(
						data = list_sprites,
						selectedValue = local_guyref.burnsprs[WPNSPR_IGNITE_MAGIC],
						onSelectFunc = [&](int32_t val)
						{
							local_guyref.burnsprs[WPNSPR_IGNITE_MAGIC] = val;
						}),
					NumberField(&local_guyref.light_rads[WPNSPR_IGNITE_MAGIC], 0, 255, 3),
					INFOBTN("Settings used for the weapon when on 'Magic' fire"),
					//
					Label(text = "Divine Fire:", hAlign = 1.0),
					DropDownList(
						data = list_sprites,
						selectedValue = local_guyref.burnsprs[WPNSPR_IGNITE_DIVINE],
						onSelectFunc = [&](int32_t val)
						{
							local_guyref.burnsprs[WPNSPR_IGNITE_DIVINE] = val;
						}),
					NumberField(&local_guyref.light_rads[WPNSPR_IGNITE_DIVINE], 0, 255, 3),
					INFOBTN("Settings used for the weapon when on 'Divine' fire")
				)
			))
		))
	);
	auto effects_tab = TabPanel(
		ptr = &guy_tabs[11],
		TabRef(name = "Effects", TabPanel(
			ptr = &guy_tabs[12],
			TabRef(name = "Basic", Column(
				Frame(title = "Sprites", hAlign = 1.0, fitParent = true,
					Rows<2>(hAlign = 1.0, rowSpacing=0.5_em,
						Label(text = "Shadow Sprite:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.spr_shadow, list_sprites),
						//
						Label(text = "Spawn Sprite:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.spr_spawn, list_sprites),
						//
						Label(text = "Death Sprite:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.spr_death, list_sprites)
					)
				),
				Frame(title = "SFX", hAlign = 1.0, fitParent = true,
					Rows<2>(hAlign = 1.0, rowSpacing = 0.5_em,
						Label(text = "BG Sound:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.bgsfx, list_sfx),
						//
						Label(text = "Hit Sound:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.hitsfx, list_sfx),
						//
						Label(text = "Death Sound:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&local_guyref.deadsfx, list_sfx)
					)
				),
				Frame(title = "Animation", hAlign = 1.0, fitParent = true,
					Rows<2>(
						Label(text = "Spawn Animation:", hAlign = 1.0, rightPadding = 0_px),
						DropDownField(&spawn_type, list_spawntype)
					)
				)
			))
		))
	);
	auto scripts_tab = TabPanel(
		ptr = &guy_tabs[13],
		TabRef(name = "Scripts", TabPanel(
			ptr = &guy_tabs[14],
			TabRef(name = "Action Script", Row(
				Column(
					ScriptField(0),
					ScriptField(1),
					ScriptField(2),
					ScriptField(3),
					ScriptField(4),
					ScriptField(5),
					ScriptField(6),
					ScriptField(7)
				),
				Column(vAlign = 0.0,
					Row(
						padding = 0_px,
						SCRIPT_LIST_PROC("NPC Action Script:", list_guyscripts, local_guyref.script, refreshScript)
					)
				)
			)),
			TabRef(name = "Weapon Script", Row(
				Column(
					WeaponScriptField(0),
					WeaponScriptField(1),
					WeaponScriptField(2),
					WeaponScriptField(3),
					WeaponScriptField(4),
					WeaponScriptField(5),
					WeaponScriptField(6),
					WeaponScriptField(7)
				),
				Column(vAlign = 0.0,
					Row(
						padding = 0_px,
						SCRIPT_LIST_PROC("Weapon Script:", list_ewpnscripts, local_guyref.weaponscript, refreshScript)
					)
				)
			))
		))
	);

	window = Window(
		use_vsync = true,
		title = titlebuf,
		info = "Edit enemies, setting up their graphics, effects, and attributes.",
		onClose = message::CANCEL,
		//shortcuts = {},
		Column(
			TabPanel(
				ptr = &guy_tabs[0],
				TabRef(name = "Basics", basics_tab),
				TabRef(name = "Attributes", attributes_tab),
				TabRef(name = "Defenses",defenses_tab),
				TabRef(name = "Flags",flags_tab),
				TabRef(name = "Attack",attack_tab),
				TabRef(name = "Effects",effects_tab),
				TabRef(name = "Scripts",scripts_tab)
			),
			Row(
				vAlign = 1.0,
				spacing = 2_em,
				Button(
					focused = true,
					text = "OK",
					minwidth = 90_px,
					onClick = message::OK),
				warnbtn = Button(
					text = "Warnings",
					minwidth = 90_px,
					onClick = message::WARNINGS),
				Button(
					text = "Clear",
					minwidth = 90_px,
					onClick = message::CLEAR),
				Button(
					text = "Default",
					minwidth = 90_px,
					onClick = message::DEFAULT),
				Button(
					text = "Cancel",
					minwidth = 90_px,
					onClick = message::CANCEL)
			)
		)
	);
	refreshScript();
	updateWarnings();
	return window;
}

void EnemyEditorDialog::apply_enemy()
{
	
	if (spawn_type==0)
		local_guyref.flags = local_guyref.flags & ~(guy_fade_flicker | guy_fade_instant);
	else if (spawn_type==1)
		local_guyref.flags = (local_guyref.flags & ~(guy_fade_instant)) | guy_fade_flicker;
	else if (spawn_type==2)
		local_guyref.flags = (local_guyref.flags & ~(guy_fade_flicker)) | guy_fade_instant;

	guysbuf[index] = local_guyref;
	strncpy(guy_string[index], enemy_name.c_str(), 63);

	saved = false;
	edited = true;
}

bool EnemyEditorDialog::handleMessage(const GUI::DialogMessage<message>& msg)
{
	switch (msg.message)
	{
	case message::ENEMYTYPE:
	{
		local_guyref.family = int32_t(msg.argument);
		loadEnemyType();
		return false;
	}
	case message::SETALLDEFENSE:
	{
		for (int q = 0; q < edefLAST255; ++q)
		{
			//unimplemented defense protection
			if ((q >= edefSCRIPT && q <= edefQUAKE) || (q >= edefICE && q <= edefSONIC) || !ddl_defenses[q])
				continue;
			local_guyref.defense[q] = local_guyref.defense[0];
			ddl_defenses[q]->setSelectedValue(local_guyref.defense[0]);
		}
		return false;
	}
	case message::CLEAR:
	{
		bool doclear = false;
		AlertDialog("Are you sure?",
			"Clearing the enemy will set all values to null!",
			[&](bool ret, bool)
			{
				doclear = ret;
			}).show();
		if (doclear)
		{
			local_guyref = guysbuf[0];
			enemy_name = fmt::format("e{:03}", index);
			rerun_dlg = true;
			return true;
		}
		return false;
	}
	case message::DEFAULT:
	{
		bool doclear = false;
		AlertDialog("Are you sure?",
			"This will reset all values back to their defaults!",
			[&](bool ret, bool)
			{
				doclear = ret;
			}).show();
		if (doclear)
		{
			local_guyref = default_guys[0];
			if (index < OLDMAXGUYS)
			{
				local_guyref = default_guys[index];
				string name = old_guy_string[index];
				enemy_name = name;
				// sprites
				local_guyref.spr_shadow = (local_guyref.family == eeROCK && local_guyref.attributes[9] == 1) ? iwLargeShadow : iwShadow;
				local_guyref.spr_death = iwDeath;
				local_guyref.spr_spawn = iwSpawn;
				// darknuts
				if (index == eDKNUT1 || index == eDKNUT2 || index == eDKNUT3 || index == eDKNUT4 || index == eDKNUT5)
				{
					if (get_qr(qr_NEWENEMYTILES))
					{
						local_guyref.s_tile = local_guyref.e_tile + 120;
						local_guyref.s_width = local_guyref.e_width;
						local_guyref.s_height = local_guyref.e_height;
					}
					else local_guyref.s_tile = 860;
				}
			}
			else
			{
				enemy_name = fmt::format("e{:03}", index);
			}
			rerun_dlg = true;
			return true;
		}
		return false;
	}

	case message::WARNINGS:
		if (warnings.size())
			displayinfo("Warnings", warnings,
				"The following issues were found with this enemy:");
		return false;
	case message::OK:
	{
		if (warnings.size())
		{
			bool cancel = false;
			AlertDialog alert("Warnings", warnings, [&](bool ret, bool)
				{
					if (!ret) cancel = true;
				});
			alert.setSubtext("The following issues were found with this enemy:");
			alert.show();
			if (cancel)
				return false;
		}
		apply_enemy();
		return true;
	}

	case message::CANCEL:
		return true;
	}
	return false;
}

