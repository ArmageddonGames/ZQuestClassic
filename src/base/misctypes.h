#ifndef MISCTYPES_H_
#define MISCTYPES_H_

#include <array>

#include "base/ints.h"
#include "base/general.h"
#include "base/containers.h"
struct PACKFILE;

enum {dt_pass=0, dt_lock, dt_shut, dt_boss, dt_olck, dt_osht, dt_obos, dt_wall, dt_bomb, dt_walk, dt_max};
enum {df_walktrans=0};

struct DoorComboSet
{
	bool operator==(const DoorComboSet& o) const = default;

	word doorcombo_u[9][4];                                   //[door type][door combo]
	byte doorcset_u[9][4];                                    //[door type][door combo]
	word doorcombo_d[9][4];                                   //[door type][door combo]
	byte doorcset_d[9][4];                                    //[door type][door combo]
	word doorcombo_l[9][6];                                   //[door type][door combo]
	byte doorcset_l[9][6];                                    //[door type][door combo]
	word doorcombo_r[9][6];                                   //[door type][door combo]
	byte doorcset_r[9][6];                                    //[door type][door combo]
	word bombdoorcombo_u[2];                                  //rubble
	byte bombdoorcset_u[2];                                   //rubble
	word bombdoorcombo_d[2];                                  //rubble
	byte bombdoorcset_d[2];                                   //rubble
	word bombdoorcombo_l[3];                                  //rubble
	byte bombdoorcset_l[3];                                   //rubble
	word bombdoorcombo_r[3];                                  //rubble
	byte bombdoorcset_r[3];                                   //rubble
	word walkthroughcombo[4];                                 //[n, s, e, w]
	byte walkthroughcset[4];                                  //[n, s, e, w]
	byte flags[2];
	byte expansion[30];
};

struct shoptype
{
	char name[33];
	byte item[3];
	byte hasitem[3];
	word price[3];
	word str[3];  //item info string
	//10
};

struct bottleshoptype
{
	char name[33];
	byte fill[3];
	word comb[3];
	byte cset[3];
	word price[3];
	word str[3];
	void clear()
	{
		memset(name, 0, sizeof(name));
		memset(fill, 0, sizeof(fill));
		memset(comb, 0, sizeof(comb));
		memset(cset, 0, sizeof(cset));
		memset(price, 0, sizeof(price));
		memset(str, 0, sizeof(str));
	}
};

struct bottletype
{
	char name[33];
	char counter[3];
	word amount[3];
	byte flags;
	byte next_type;
#define BTFLAG_PERC0       0x01
#define BTFLAG_PERC1       0x02
#define BTFLAG_PERC2       0x04
#define BTFLAG_CURESHJINX  0x08
#define BTFLAG_CURESWJINX  0x10
#define BTFLAG_CUREITJINX  0x20
#define BTFLAG_ALLOWIFFULL 0x40
#define BTFLAG_AUTOONDEATH 0x80
	void clear()
	{
		memset(name, 0, sizeof(name));
		memset(counter, -1, sizeof(counter));
		memset(amount, 0, sizeof(amount));
		flags = 0;
		next_type = 0;
	}
	bool is_blank() const
	{
		if(name[0])
			return false;
		for(int q = 0; q < 3; ++q)
		{
			if(counter[q] > -1 && amount[q] > 0)
				return false;
			if(next_type)
				return false;
		}
		return true;
	}
};

struct pondtype
{
	byte olditem[3];
	byte newitem[3];
	byte fairytile;
	byte aframes;
	byte aspeed;
	word msg[15];
	byte foo[32];
};

struct infotype
{
	char name[33];
	word str[3];
	word price[3];
};

struct warpring
{
	word dmap[9];
	byte scr[9];
	byte size;
};

struct zcolors
{
	byte text, caption;
	byte overw_bg, dngn_bg;
	byte dngn_fg, cave_fg;
	byte bs_dk, bs_goal;
	byte compass_lt, compass_dk;
	byte subscr_bg, subscr_shadow, triframe_color;
	byte bmap_bg,bmap_fg;
	byte hero_dot;
	byte triforce_cset;
	byte triframe_cset;
	byte overworld_map_cset;
	byte dungeon_map_cset;
	byte blueframe_cset;
	int32_t triforce_tile;
	int32_t triframe_tile;
	int32_t overworld_map_tile;
	int32_t dungeon_map_tile;
	int32_t blueframe_tile;
	int32_t HCpieces_tile;
	byte HCpieces_cset;
	byte msgtext;
	byte foo[6];
	byte foo2[256];
};

struct palcycle
{
	byte first,count,speed;
};

enum
{
	STATUS_JINX_MELEE,
	STATUS_JINX_ITEM,
	STATUS_JINX_SHIELD,
	STATUS_STUN,
	STATUS_BUNNY,
	NUM_ENGINE_STATUSES,
	NUM_STATUSES = 256
};
struct EntityStatus
{
	// Damage or Healing over time
	int32_t damage;
	word damage_rate = 59;
	bool damage_iframes = false;
	bool ignore_iframes = true;
	
	// A sprite OR tile to overlay (or 'under'lay)
	// EX: Static effect for electrified, flame effect for burning
	uint8_t visual_sprite;
	int32_t visual_tile;
	zfix visual_x, visual_y;
	byte visual_tilewidth = 1, visual_tileheight = 1;
	bool visual_relative = true;
	bool visual_under;
	bool visual_hide_sprite; // hide the enemy/hero sprite
	
	// A tile modifier to the sprite's tile
	int32_t sprite_tile_mod;
	// If non-zero, mask out the entire sprite with this color
	byte sprite_mask_color;
	
	// Which status effects are cured by this effect
	bool cures[NUM_STATUSES];
	
	// Changes to defenses of the affected enemy/hero
	bounded_map<word, int16_t> defenses {edefLAST255, -1};
	
	// Basic engine effects
	bool jinx_melee, jinx_item, jinx_shield;
	bool stun, bunny;
	
	bool is_empty() const;
	
	void clear();
	
	int32_t read(PACKFILE *f, word s_version);
	int32_t write(PACKFILE *f);
	
	EntityStatus& operator=(EntityStatus const& other) = default;
	bool operator==(EntityStatus const& other) const = default;
};

enum miscsprite
{
	sprFALL,
	sprDROWN,
	sprLAVADROWN,
	sprSWITCHPOOF,
	sprMAX = 256
};
enum miscsfx
{
	sfxBUSHGRASS,
	sfxSWITCHED,
	sfxLOWHEART,
	sfxERROR,
	sfxHURTPLAYER,
	sfxHAMMERPOUND,
	sfxSUBSCR_ITEM_ASSIGN,
	sfxSUBSCR_CURSOR_MOVE,
	sfxREFILL,
	sfxDRAIN,
	sfxTAP,
	sfxTAP_HOLLOW,
	sfxHERO_LANDS,
	sfxMAX = 256
};

struct miscQdata
{
	shoptype shop[NUM_SHOPS];
	infotype info[NUM_INFOS];
	warpring warp[NUM_WARP_RINGS];
	palcycle cycles[NUM_PAL_CYCLES][3];
	byte     triforce[8];                                     // positions of triforce pieces on subscreen
	zcolors  colors;
	int32_t     icons[4];
	word endstring;
	// This has been deprecated, but kept around to keep a funny quest working (alarm clocks).
	int32_t questmisc[32]; //Misc init values for the user. Used by scripts.
	int32_t zscript_last_compiled_version;
	byte sprites[sprMAX];
	
	bottletype bottle_types[NUM_BOTTLE_TYPES];
	bottleshoptype bottle_shop_types[NUM_BOTTLE_SHOPS];
	
	byte miscsfx[sfxMAX];
	
	std::string status_names[NUM_STATUSES];
	EntityStatus status_effects[NUM_STATUSES];
	
	miscQdata& operator=(miscQdata const& other) = default;
};

extern std::array<DoorComboSet, MAXDOORCOMBOSETS> DoorComboSets;
extern std::array<std::string, MAXDOORCOMBOSETS> DoorComboSetNames;
extern miscQdata QMisc;

#endif

