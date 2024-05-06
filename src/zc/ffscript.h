#ifndef _FFSCRIPT_H_
#define _FFSCRIPT_H_

#include "base/zdefs.h"
#include "base/initdata.h"
#include "parser/parserDefs.h"
#include <utility>
#include <string>
#include <list>
#include <deque>
#include <bitset>
#include "zasm/defines.h"
#include "zc/zelda.h"
#include "zc/replay.h"

#define ZS_BYTE 255
#define ZS_CHAR 255
#define ZS_WORD 65535
#define ZS_SHORT 32767
#define ZS_LONG 214747
#define ZS_INT 214747
#define ZS_FIX 214748
#define ZS_DWORD 65535

#define MAX_LWPN_SPRITES 1024
#define MAX_EWPN_SPRITES 1024
#define MAX_NPC_SPRITES 1024
#define MAX_ITEM_SPRITES 1024

#define ZSCRIPT_MAX_STRING_CHARS 214748

#define MAX_ZQ_LAYER 6
#define MAX_DRAW_LAYER 7
#define MIN_ZQ_LAYER 0
#define MIN_DRAW_LAYER 0
#define MAX_FLAGS 512

#define SRAM_VERSION 2

#define MIN_INTERNAL_BITMAP -1 //RT_SCREEN
#define MAX_INTERNAL_BITMAP 6 //RT_BITMAP6
#define FFRULES_SIZE 1024

//Hero->WarpEx Flags
#define warpFlagDONTKILLSCRIPTDRAWS   0x001
#define warpFlagDONTKILLSOUNDS        0x002
#define warpFlagFORCERESETMUSIC       0x004
#define warpFlagSETENTRANCESCREEN     0x008
#define warpFlagSETENTRANCEDMAP       0x010
#define warpFlagSETCONTINUESCREEN     0x020
#define warpFlagSETCONTINUEDMAP       0x040
#define warpFlagDONTRESTARTDMAPSCRIPT 0x080
#define warpFlagDONTCLEARSPRITES      0x100
#define warpFlagFORCECONTINUEMUSIC    0x200

#define warpFlagCLEARITEMS 0x200
#define warpFlagCLEARGUYS 0x400
#define warpFlagCLEARLWEAPONS 0x800
#define warpFlagCLEAREWEAPONS 0x1000
#define warpFlagCLEARHOOKSHOT 0x2000
#define warpFlagCLEARDECORATIONS 0x4000
#define warpFlagCLEARPARTICLES 0x8000
#define warpFlagNOSTEPFORWARD 0x10000



//Visual Warp Effect Types for Hero->WarpEx()
enum { warpEffectNONE, warpEffectZap, warpEffectWave, warpEffectInstant, warpEffectOpen, warpEffectMozaic }; 
void doWarpEffect(int32_t warpEffect, bool out);

#define svALL 		0
#define svGUYS 		0x01
#define svITEMS 	0x02
#define svWEAPONS 	0x04
#define svCOMBOS 	0x08
#define svDMAPS 	0x10
#define svMAPSCR 	0x20

enum herospritetype
{
	LSprwalkspr, LSprstabspr, LSprslashspr, LSprfloatspr,
	LSprswimspr, LSprdivespr, LSprpoundspr, LSprjumpspr,
	LSprchargespr, LSprcastingspr, LSprholdspr1, LSprholdspr2,
	LSprholdsprw1, LSprholdsprw2, LSprdrownspr, LSprlavadrownspr, 
	LSprsideswimspr, LSprsideswimslashspr, LSprsideswimstabspr, 
	LSprsideswimpoundspr, LSprsideswimchargespr, LSprholdsprSw1, 
	LSprholdsprSw2, LSprsideswimcastingspr, LSprsidedrownspr, LSprrevslashspr, LSprlast
};

//suspend types
enum { 
	//Typical processes that we want to pause, similar to ALLOFF()
	
	//0 : Combo animation
	susptCOMBOANIM,
	
	//1->5 Main Sprite animation by type
	susptGUYS, susptLWEAPONS, susptEWEAPONS, susptITEMS, susptHERO, 
	
	//6 : FFC (e.g. movement, changers, but not scripts)
	susptUPDATEFFC, //ffcs
	
	//7->8 : Sprite subclasses
	susptDECORATIONS, susptPARTICLES, //sprite subclasses
	
	//9->10: Palette events
	susptPALCYCLE, susptLAKES, //lake dries up
	
	//11->15 : game system events
	susptCOLLISIONS, susptCONTROLSTATE, susptONEFRAMECONDS, susptSCRIPDRAWCLEAR, susptQUAKE,

	//16->27 Script Types
	susptGLOBALGAME, susptNPCSCRIPTS, susptLWEAPONSCRIPTS, susptEWEAPONSCRIPTS, susptITEMSPRITESCRIPTS,
	//21
	susptFFCSCRIPTS, susptHEROACTIVE, susptITEMSCRIPTENGINE, susptDMAPSCRIPT, susptSCREENSCRIPTS,
	susptSUBSCREENSCRIPTS, susptCOMBOSCRIPTS, //27
	
	//28->59: Reserved padding
	
	//60->42 : Moving items
	susptCONVEYORSITEMS = 60, susptDRAGGINGITEM, susptROAMINGITEM,
	//63->67 : Misc
	susptLENS, susptHOOKSHOT, susptMOVINGBLOCKS, susptMAGICCAST, susptSCREENDRAW,
	//68
	susptLAST };

//npc function enums
// npc_collision()
	//bool npc->Collision
enum { obj_type_lweapon, obj_type_eweapon, obj_type_npc, obj_type_player, 
	obj_type_ffc, obj_type_combo_pos, obj_type_item, obj_type_LAST };
	
//do_npc_simulate_hit
	//npc->SimulateHit()
enum { simulate_hit_type_weapon, simulate_hit_type_sprite };	

//Flags for SetVolume() and GetVolume() that are set in FFCoreFlags[] 
//these determine what is under script control and are used for automatic restoration
//of user volume settings on quest exit.
enum {
	FFCORE_SCRIPTED_MIDI_VOLUME 	= 0x0001,
	FFCORE_SCRIPTED_DIGI_VOLUME 	= 0x0002,
	FFCORE_SCRIPTED_MUSIC_VOLUME 	= 0x0004,
	FFCORE_SCRIPTED_SFX_VOLUME 	= 0x0008,
	FFCORE_SCRIPTED_PANSTYLE 	= 0x0010
};

//Music Update States
//Used for determining when music should update on warps 
enum {
	MUSIC_UPDATE_SCREEN,
	MUSIC_UPDATE_DMAP,
	MUSIC_UPDATE_LEVEL,
	MUSIC_UPDATE_NEVER
};
enum {
	MUSIC_UPDATE_FLAG_NOCUT  = 0x1, //Music persists through things that would normally cut to silence (such as entrance exit warps)
	MUSIC_UPDATE_FLAG_REVERT = 0x2 //State reverts to screen on music switch
};

//SYstem Date and Time Categories for GetSystemTime()
enum { curyear, curmonth, curday_month, curday_week, curhour, 
	curminute, cursecond, curdayyear, curDST, curTimeLAST };

	
enum { wexType, wexDMap, wexScreen, wexX, wexY, wexEffect, wexSound, wexFlags, wexDir, wexActive, wexLast };


//Script drawing: (t/b/a)
//Allow or forbid drawing during specific game events. 
enum{
	scdrDRAW_WHILE_SCROLLING, scdrDRAW_DURING_SCREEN_TRANSITION, scdrDRAW_DURING_WARP,
	scdrDRAW_DURING_WIPES, scdrLAST
};

//UID types for ->Script_UID
enum
{
	UID_TYPE_NPC, UID_TYPE_WEAPON, UID_TYPE_ITEM, UID_TYPES
};

//Quest Version Information Categories
//These reflect the version details from when the quest was last saved.
enum 
{
	vZelda, qQuestNumber, vBuild, vBeta, vHeader, vRules, vStrings, vMisc,
	vTiles, vCombos, vCSets, vMaps, vDMaps, vDoors, vItems, vWeaponSprites,
	vColours, vIcons, vGfxPack, vInitData, vGuys, vMIDIs, vCheats, vSaveformat,
	vComboAliases, vHeroSprites, vSubscreen, vItemDropsets, vFFScript, vSFX, vFavourites,
	qMapCount, vLastCompile, vCompatRule,
	versiontypesLAST
	
};

//Quest Header 'User Set Version' categories. Not in use at present.
enum
{
	 qQuestVersion, qMinQuestVersion, qvLAST
};

enum //ScrollingData indexes
{
	SCROLLDATA_DIR, SCROLLDATA_NX, SCROLLDATA_NY, SCROLLDATA_OX, SCROLLDATA_OY, SZ_SCROLLDATA
};

///----------------------------------------------//
//           New Mapscreen Flags Tools           //
///----------------------------------------------//

enum mapflagtype
{
	// Room Types
	MSF_INTERIOR, MSF_DUNGEON, MSF_SIDEVIEW,
	
	// View
	MSF_INVISHERO, MSF_NOHEROMARKER, MSF_NOSUBSCREEN, MSF_NOOFFSET,
	MSF_LAYER2BG, MSF_LAYER3BG, MSF_DARKROOM,
	
	// Secrets
	MSF_BLOCKSHUT, MSF_TEMPSECRETS, MSF_TRIGPERM, MSF_ALLTRIGFLAGS,
	
	// Warp
	MSF_AUTODIRECT, MSF_SENDSIRECT, MSF_MAZEPATHS, MSF_MAZEOVERRIDE,
	MSF_SPRITECARRY, MSF_DIRECTTIMEDWARPS, MSF_SECRETSISABLETIMEWRP, MSF_RANDOMTIMEDWARP,
	
	// Item
	MSF_HOLDUP, MSF_FALLS,
	
	// Combo
	MSF_MIDAIR, MSF_CYCLEINIT, MSF_IGNOREBOOTS, MSF_TOGGLERINGS,
	
	// Save
	MSF_SAVECONTHERE, MSF_SAVEONENTRY, MSF_CONTHERE, MSF_NOCONTINUEWARP,
	
	// FFC
	MSF_WRAPFFC, MSF_NOCARRYOVERFFC, 
	
	// Whistle
	MSF_STAIRS, MSF_PALCHANGE, MSF_DRYLAKE, 
	
	// Enemies
	MSF_INVISIBLEENEMIES, MSF_TRAPS_IGNORE_SOLID, MSF_EMELIESALWAYSRETURN, MSF_ENEMIES_ITEM, MSF_ENEMEIS_SECRET,
	MSF_ENEMIES_SECRET_PERM,  
	
		//->enemyflags
		MSF_SPAWN_ZORA, MSF_SPAWN_CORNERTRAP, MSF_SPAWN_MIDDLETRAP, MSF_SPAWN_ROCK, MSF_SPAWN_SHOOTER,
		MSF_RINGLEADER, MSF_ENEMYHASITEM, MSF_ENEMYISBOSS, 
	
	// Misc
	MSF_ALLOW_LADDER, MSF_NO_DIVING, MSF_SFXONENTRY, MSF_LENSEFFECT,
		 
	//Custom / Script 
	MSF_SCRIPT1,
	MSF_CUSTOM1 = MSF_SCRIPT1,
	MSF_SCRIPT2,
	MSF_CUSTOM2 = MSF_SCRIPT2,
	MSF_SCRIPT3,
	MSF_CUSTOM3 = MSF_SCRIPT3,
	MSF_SCRIPT4,
	MSF_CUSTOM4 = MSF_SCRIPT4,
	MSF_SCRIPT5,
	MSF_CUSTOM5 = MSF_SCRIPT5,
	
	MSF_DUMMY_8, 
	MSF_LAST
	
};

//User-generated / Script-Generated bitmap object
#define UBMPFLAG_FREEING               0x01
#define UBMPFLAG_CAN_DELETE            0x02
struct user_bitmap : public user_abstract_obj
{
	BITMAP* u_bmp;
	int32_t width;
	int32_t height;
	byte flags;

	user_bitmap() = default;
	user_bitmap(const user_bitmap&) = delete;

	~user_bitmap()
	{
		destroy_bitmap(u_bmp);
	}

	void destroy()
	{
		destroy_bitmap(u_bmp);
		width = 0;
		height = 0;
		flags = 0;
		u_bmp = NULL;
	}

	void free_obj()
	{
		flags |= UBMPFLAG_FREEING;
	}

	void mark_can_del()
	{
		flags |= UBMPFLAG_CAN_DELETE;
	}

	bool is_freeing()
	{
		return flags & UBMPFLAG_FREEING;
	}

	bool can_del()
	{
		return flags & UBMPFLAG_CAN_DELETE;
	}
};



//Old, 2.50 bitmap IDs
enum { rtSCREEN = -1, rtBMP0 = 0, rtBMP1, 
	rtBMP2, rtBMP3, rtBMP4, rtBMP5, rtBMP6, firstUserGeneratedBitmap };
//bitmap constants
#define MAX_USER_BITMAPS 256
#define MIN_OLD_RENDERTARGETS -1 //old script drawing
#define MAX_OLD_RENDERTARGETS 6
	
//Holds all of the user-generated / script-generated bitmaps and their information.
	//User bitmap lowest viable ID is 'rtBMP6+1' (firstUserGeneratedBitmap)
struct script_bitmaps
{
	void update();
	user_bitmap& get(int32_t id);
};

#define MAX_USER_FILES 256
struct user_file : public user_abstract_obj
{
	FILE* file;
	std::string filepath;

	~user_file()
	{
		if (file)
			fclose(file);
	}
	
	void close()
	{
		if(file) fclose(file);
		file = NULL;
		filepath = "";
	}
	
	int32_t do_remove()
	{
		if(file) fclose(file);
		file = NULL;
		int32_t r = remove(filepath.c_str());
		filepath = "";
		return r;
	}
	
	void setPath(char* buf)
	{
		if(buf)
			filepath = buf;
		else filepath = "";
	}
};

#define MAX_USER_DIRS 256
struct user_dir : public user_abstract_obj
{
	FLIST* list;
	std::string filepath;

	~user_dir()
	{
		if (list)
		{
			list->clear();
			free(list);
			list = NULL;
		}
	}
	
	void setPath(const char* buf);
	void refresh()
	{
		if(list)
			list->load(filepath.c_str());
		else setPath(filepath.c_str());
	}
	int32_t size()
	{
		return list->size;
	}
	bool get(int32_t index, char* buf)
	{
		return list->get(index, buf);
	}
};


#define MAX_USER_STACKS 256
#define USERSTACK_MAX_SIZE 2147483647
struct user_stack : public user_abstract_obj
{
	std::deque<int32_t> theStack;
	
	int32_t size()
	{
		return theStack.size();
	}
	bool full()
	{
		return theStack.size() == USERSTACK_MAX_SIZE;
	}
	int32_t get(int32_t index)
	{
		if(unsigned(index) >= theStack.size()) return 0;
		return theStack[index];
	}
	void set(int32_t index, int32_t val)
	{
		if(unsigned(index) >= theStack.size()) return;
		theStack[index] = val;
	}
	void push_back(int32_t val)
	{
		if(full()) return;
		theStack.push_back(val);
	}
	void push_front(int32_t val)
	{
		if(full()) return;
		theStack.push_front(val);
	}
	int32_t pop_back()
	{
		if(theStack.empty()) return 0;
		int32_t val = theStack.back();
		theStack.pop_back();
		return val;
	}
	int32_t pop_front()
	{
		if(theStack.empty()) return 0;
		int32_t val = theStack.front();
		theStack.pop_front();
		return val;
	}
	int32_t peek_back()
	{
		if(theStack.empty()) return 0;
		return theStack.back();
	}
	int32_t peek_front()
	{
		if(theStack.empty()) return 0;
		return theStack.front();
	}
	void clearStack()
	{
		theStack.clear();
		theStack.shrink_to_fit();
	}
};

#define MAX_USER_RNGS 256
struct user_rng : public user_abstract_obj
{
	zc_randgen* gen;
	
	int32_t rand()
	{
		return zc_rand(gen);
	}
	int32_t rand(int32_t upper, int32_t lower=0)
	{
		return zc_rand(upper, lower, gen);
	}
	void srand(int32_t seed)
	{
		zc_game_srand(seed, gen);
	}
	int32_t srand()
	{
		int32_t seed = time(0) + ((int64_t(rand()) * int64_t(rand())) * ((rand() % 2) ? 1 : -1));
		srand(seed);
		return seed;
	}
	void set_gen(zc_randgen* newgen)
	{
		gen = newgen;
		if(newgen) srand();
	}
};

#define MAX_USER_PALDATAS 256
#define PALDATA_NUM_COLORS 256
#define PALDATA_BITSTREAM_SIZE 32
struct user_paldata : public user_abstract_obj
{
	RGB colors[PALDATA_NUM_COLORS];
	byte colors_used[PALDATA_BITSTREAM_SIZE]; //A set of 256 bitflags

	enum { CSPACE_RGB, CSPACE_CMYK, CSPACE_HSV, CSPACE_HSV_CW, CSPACE_HSV_CCW, CSPACE_HSL, CSPACE_HSL_CW, CSPACE_HSL_CCW, CSPACE_LAB, CSPACE_LCH, CSPACE_LCH_CW, CSPACE_LCH_CCW };
	
	//Sets a color index on the paldata
	void set_color(int32_t ind, RGB c)
	{
		c.r = vbound(c.r, 0, 63);
		c.g = vbound(c.g, 0, 63);
		c.b = vbound(c.b, 0, 63);
		colors[ind] = c;
		set_bit(colors_used, ind, true);
	}

	void load_cset(int32_t cset, int32_t dataset);
	void write_cset(int32_t cset, int32_t dataset);
	bool check_cset(int32_t cset, int32_t dataset);
	void load_cset_main(int32_t cset);
	void write_cset_main(int32_t cset);
	bool check_cset_main(int32_t cset);
	static RGB mix_color(RGB start, RGB end, double percent, int32_t color_space = CSPACE_RGB);
	static void RGBTo(RGB c, double arr[], int32_t color_space);
	static RGB RGBFrom(double arr[], int32_t color_space);
	static double HueToRGB(double v1, double v2, double vH);
	static double WrapLerp(double a, double b, double t, double min, double max, int32_t direction);
	void mix(user_paldata *pal_start, user_paldata *pal_end, double percent, int32_t color_space = CSPACE_RGB, int32_t start_color = 0, int32_t end_color = 240);
};

//Module System.
//Putting this here for now.
#include "base/module.h"

typedef struct ZSCRIPT_CONFIG_ENTRY
{
   char *name;                      /* variable name (NULL if comment) */
   char *data;                      /* variable value */
   struct ZSCRIPT_CONFIG_ENTRY *next;       /* linked list */
} ZSCRIPT_CONFIG_ENTRY;


typedef struct ZSCRIPT_CONFIG
{
   ZSCRIPT_CONFIG_ENTRY *head;              /* linked list of config entries */
   char *filename;                  /* where were we loaded from? */
   int32_t dirty;                       /* has our data changed? */
} ZSCRIPT_CONFIG;


typedef struct ZSCRIPT_CONFIG_HOOK
{
   char *section;                   /* hooked config section info */
   int32_t (*intgetter)(AL_CONST char *name, int32_t def);
   AL_CONST char *(*stringgetter)(AL_CONST char *name, AL_CONST char *def);
   void (*stringsetter)(AL_CONST char *name, AL_CONST char *value);
   struct ZSCRIPT_CONFIG_HOOK *next; 
} ZSCRIPT_CONFIG_HOOK;


#define MAX_CONFIGS     4

static ZSCRIPT_CONFIG *config[MAX_CONFIGS] = { NULL, NULL, NULL, NULL };
static ZSCRIPT_CONFIG *config_override = NULL;
static ZSCRIPT_CONFIG *config_language = NULL;
static ZSCRIPT_CONFIG *system_config = NULL;

static ZSCRIPT_CONFIG_HOOK *config_hook = NULL;

static int32_t config_installed = FALSE;

static char **config_argv = NULL;
static char *argv_buf = NULL;
static int32_t argv_buf_size = 0;

int32_t run_script_int(bool is_jitted);

//Config files
void zscript_flush_config(ZSCRIPT_CONFIG *cfg);
void zscript_flush_config_file(void);
void zscript_destroy_config(ZSCRIPT_CONFIG *cfg);
void zscript_config_cleanup(void);
void zscript_init_config(int32_t loaddata);
int32_t zscript_get_config_line(const char *data, int32_t length, char **name, char **val);
void zscript_set_config(ZSCRIPT_CONFIG **config, const char *data, int32_t length, const char *filename);
void zscript_load_config_file(ZSCRIPT_CONFIG **config, const char *filename, const char *savefile);
void zscript_set_config_file(const char *filename);
void zscript_set_config_data(const char *data, int32_t length);
void zscript_override_config_file(const char *filename);
void zscript_override_config_data(const char *data, int32_t length);
void zscript_push_config_state(void);
void zscript_pop_config_state(void);
void zscript_prettify_config_section_name(const char *in, char *out, int32_t out_size);
void zscript_hook_config_section(const char *section, int32_t (*intgetter)(const char *, int32_t), const char *(*stringgetter)(const char *, const char *), void (*stringsetter)(const char *, const char *));
int32_t zscript_config_is_hooked(const char *section);
ZSCRIPT_CONFIG_ENTRY *zscript_find_config_string(ZSCRIPT_CONFIG *config, const char *section, const char *name, ZSCRIPT_CONFIG_ENTRY **prev);
const char *zscript_get_config_string(const char *section, const char *name, const char *def);
int32_t zscript_get_config_int(const char *section, const char *name, int32_t def);
int32_t zscript_get_config_hex(const char *section, const char *name, int32_t def);
float zscript_get_config_float(const char *section, const char *name, float def);
int32_t zscript_get_config_id(const char *section, const char *name, int32_t def);
char **zscript_get_config_argv(const char *section, const char *name, int32_t *argc);
ZSCRIPT_CONFIG_ENTRY *zscript_insert_config_variable(ZSCRIPT_CONFIG *the_config, ZSCRIPT_CONFIG_ENTRY *p, const char *name, const char *data);
void zscript_set_config_string(const char *section, const char *name, const char *val);
void zscript_set_config_int(const char *section, const char *name, int32_t val);
void zscript_set_config_hex(const char *section, const char *name, int32_t val);
void zscript_set_config_float(const char *section, const char *name, float val);
void zscript_set_config_id(const char *section, const char *name, int32_t val);
void _zscript_reload_config(void);
void zscript_reload_config_texts(const char *new_language);
const char *zscript_get_config_text(const char *msg);
int32_t zscript_add_unique_config_name(const char ***names, int32_t n, char const *name);
int32_t zscript_attach_config_entries(ZSCRIPT_CONFIG *conf, const char *section,int32_t n, const char ***names, int32_t list_sections);
int32_t zscript_list_config_entries(const char *section, const char ***names);
int32_t zscript_list_config_sections(const char ***names);
void zscript_free_config_entries(const char ***names);

void clearConsole();


enum scr_timing
{
	SCR_TIMING_INIT = -1,
	//0
	SCR_TIMING_START_FRAME, SCR_TIMING_POST_COMBO_ANIM, SCR_TIMING_POST_POLL_INPUT,
	SCR_TIMING_POST_FFCS, SCR_TIMING_POST_GLOBAL_ACTIVE,
	//5
	SCR_TIMING_POST_PLAYER_ACTIVE, SCR_TIMING_POST_DMAPDATA_ACTIVE,
	SCR_TIMING_POST_DMAPDATA_PASSIVESUBSCREEN, SCR_TIMING_POST_COMBOSCRIPT,
	SCR_TIMING_POST_PUSHBLOCK,
	//10
	SCR_TIMING_POST_ITEMSPRITE_SCRIPT, SCR_TIMING_POST_ITEMSPRITE_ANIMATE,
	SCR_TIMING_POST_NPC_ANIMATE, SCR_TIMING_POST_EWPN_ANIMATE, SCR_TIMING_POST_EWPN_SCRIPT,
	//15
	SCR_TIMING_POST_OLD_ITEMDATA_SCRIPT, SCR_TIMING_POST_PLAYER_ANIMATE,
	SCR_TIMING_POST_NEW_ITEMDATA_SCRIPT, SCR_TIMING_POST_CASTING,
	SCR_TIMING_POST_LWPN_ANIMATE,
	//20
	SCR_TIMING_POST_DECOPARTICLE_ANIMATE, SCR_TIMING_POST_COLLISIONS_PALETTECYCLE,
	SCR_TIMING_WAITDRAW, SCR_TIMING_POST_GLOBAL_WAITDRAW, SCR_TIMING_POST_PLAYER_WAITDRAW,
	//25
	SCR_TIMING_POST_DMAPDATA_ACTIVE_WAITDRAW, SCR_TIMING_POST_DMAPDATA_PASSIVESUBSCREEN_WAITDRAW,
	SCR_TIMING_POST_SCREEN_WAITDRAW, SCR_TIMING_POST_FFC_WAITDRAW, SCR_TIMING_POST_COMBO_WAITDRAW,
	//30
	SCR_TIMING_POST_ITEM_WAITDRAW, SCR_TIMING_POST_NPC_WAITDRAW, SCR_TIMING_POST_EWPN_WAITDRAW,
	SCR_TIMING_POST_LWPN_WAITDRAW, SCR_TIMING_POST_ITEMSPRITE_WAITDRAW,
	//35
	SCR_TIMING_PRE_DRAW, SCR_TIMING_POST_DRAW, SCR_TIMING_POST_STRINGS, SCR_TIMING_END_FRAME,
	SCR_NUM_TIMINGS
};
enum
{
	GENSCR_ST_RELOAD,
	GENSCR_ST_CONTINUE,
	GENSCR_ST_CHANGE_SCREEN,
	GENSCR_ST_CHANGE_DMAP,
	GENSCR_ST_CHANGE_LEVEL,
	GENSCR_NUMST
};
enum
{
	GENSCR_EVENT_NIL = -1,
	GENSCR_EVENT_INIT,
	GENSCR_EVENT_CONTINUE,
	GENSCR_EVENT_FFC_PRELOAD,
	GENSCR_EVENT_CHANGE_SCREEN,
	GENSCR_EVENT_CHANGE_DMAP,
	GENSCR_EVENT_CHANGE_LEVEL,
	GENSCR_EVENT_HERO_HIT_1,
	GENSCR_EVENT_HERO_HIT_2,
	GENSCR_EVENT_COLLECT_ITEM,
	GENSCR_EVENT_ENEMY_DROP_ITEM_1,
	GENSCR_EVENT_ENEMY_DROP_ITEM_2,
	GENSCR_EVENT_ENEMY_DEATH,
	GENSCR_EVENT_ENEMY_HIT1,
	GENSCR_EVENT_ENEMY_HIT2,
	GENSCR_EVENT_POST_COLLECT_ITEM,
	GENSCR_EVENT_PLAYER_FALL,
	GENSCR_EVENT_PLAYER_DROWN,
	GENSCR_NUMEVENT
};
enum
{
	GENEVT_ICTYPE_COLLECT, //Hero collected the item
	GENEVT_ICTYPE_MELEE, //Melee lweapon collected the item
	GENEVT_ICTYPE_MELEE_EW, //Melee eweapon collected the item
	GENEVT_ICTYPE_RANGED_DRAG //Ranged weapon collected, and will drag
};

struct user_genscript
{
	//Saved vars
	byte _doscript;
	bounded_map<dword,int32_t> data;
	word exitState;
	word reloadState;
	uint32_t eventstate;
	bounded_vec<byte,int32_t> initd;
	
	//Temp Vars
	bool wait_atleast = true;
	bool waitevent;
	scr_timing waituntil = SCR_TIMING_START_FRAME;
	
	void clear();
	void launch();
	void quit();
	size_t dataSize() const
	{
		return data.size();
	}
	void dataResize(int32_t sz)
	{
		data.resize(vbound(sz, 0, 214748));
	}
	void timeExit(byte exState)
	{
		if(!doscript()) return;
		if(exitState & (1<<exState))
			quit();
		else if(reloadState & (1<<exState))
			launch();
	}
	byte& doscript();
	byte const& doscript() const;
	static user_genscript& get(int indx);
private:
	static user_genscript user_scripts[NUMSCRIPTSGENERIC];
	
	int32_t indx;
	user_genscript() = default;
};
extern int32_t genscript_timing;
void countGenScripts();
void timeExitAllGenscript(byte exState);
void throwGenScriptEvent(int32_t event);
void load_genscript(const gamedata& gd);
void load_genscript(const zinitdata& gd);
void save_genscript(gamedata& gd);

class FFScript
{
	
    
public:
//FFScript();
void init();
void shutdown();


int32_t max_ff_rules;
mapscr* tempScreens[7];
mapscr* ScrollingScreens[7];
int32_t ScrollingData[SZ_SCROLLDATA];
std::vector<int32_t> eventData;

int32_t getQRBit(int32_t rule);
void setHeroAction(int32_t a);
int32_t getHeroAction();
void Play_Level_Music();
int32_t getTime(int32_t type); //get system RTC Information.
void getRTC(const bool v);
int32_t getQuestHeaderInfo(int32_t type);
void do_graphics_getpixel();

void set_mapscreenflag_state(mapscr *m, int32_t flagid, bool state);
int32_t get_mapscreenflag_state(mapscr *m, int32_t flagid);

void clearRunningItemScripts();
bool itemScriptEngine();
void npcScriptEngineOnWaitdraw();
bool itemScriptEngineOnWaitdraw();
void lweaponScriptEngineOnWaitdraw();
void eweaponScriptEngine();
void eweaponScriptEngineOnWaitdraw();
void itemSpriteScriptEngine();
void itemSpriteScriptEngineOnWaitdraw();
void warpScriptCheck();
void runWarpScripts(bool waitdraw);
void runF6Engine();
void runOnDeathEngine();
void runOnLaunchEngine();
void runGenericPassiveEngine(int32_t scrtm);
bool runGenericFrozenEngine(const word script, const int32_t* init_data = nullptr);
bool runScriptedActiveSusbcreen();
bool runOnMapScriptEngine();
void doScriptMenuDraws();
void runOnSaveEngine();
void initIncludePaths();
void initRunString();
void updateRunString();
void updateIncludePaths();
bool checkExtension(std::string &filename, const std::string &extension);
//String.h functions for ffscript - 2.55 Alpha 23
void do_strcmp();
void do_strncmp();
void do_strcpy(const bool a, const bool b);
void do_strlen(const bool v);
//More string.h functions, 19th May, 2019 
void do_arraycpy(const bool a, const bool b);
void AlloffLimited(int32_t flagset);
void do_xlen(const bool v);
double ln(double temp);
double Log2( double n );
int32_t numDigits(int32_t number);
int32_t Log10(double temp);
double LogToBase(double x, double base);
void do_xtoi(const bool v);
void do_ilen(const bool v);
void do_atoi(const bool v);
void do_atol(const bool v);
bool isNumber(char chr);
int32_t UpperToLower(std::string *s);
int32_t LowerToUpper(std::string *s);
int32_t ConvertCase(std::string *s);
int32_t ilen(char *p);
int32_t zc_strlen(char *p);
int32_t atox(char *ip_str);
void do_LowerToUpper(const bool v);
void do_UpperToLower(const bool v);
void do_ConvertCase(const bool v);
void do_stricmp();
void do_strnicmp();

void do_getgenericscript();
void do_getcomboscript();
void do_getnpcscript();
void do_getlweaponscript();
void do_geteweaponscript();
void do_getheroscript();
void do_getglobalscript();
void do_getdmapscript();
void do_getscreenscript();
void do_getitemspritescript();
void do_getuntypedscript();
void do_getsubscreenscript();
void do_getnpcbyname();
void do_getitembyname();
void do_getcombobyname();
void do_getdmapbyname();

int32_t getHeroOTile(int32_t index1, int32_t index2);
int32_t getHeroOFlip(int32_t index1, int32_t index2);

int32_t IsBlankTile(int32_t i);
int32_t Is8BitTile(int32_t i);

defWpnSprite getDefWeaponSprite(int32_t wpnid);
//defWpnSprite getDefWeaponSprite(weapon *wp);

//ZC SRAM
//void read_maps(PACKFILE *f, int32_t vers_id);
//void write_maps(PACKFILE *f, int32_t vers_id);
void read_dmaps(PACKFILE *f, int32_t vers_id);
void write_dmaps(PACKFILE *f, int32_t vers_id);
void read_combos(PACKFILE *f, int32_t vers_id);
void write_combos(PACKFILE *f, int32_t vers_id);
void write_items(PACKFILE *f, int32_t vers_id);
void read_items(PACKFILE *f, int32_t vers_id);
void write_enemies(PACKFILE *f, int32_t vers_id);
void read_enemies(PACKFILE *f, int32_t vers_id);
void write_weaponsprtites(PACKFILE *f, int32_t vers_id);
void read_weaponsprtites(PACKFILE *f, int32_t vers_id);
void write_mapscreens(PACKFILE *f,int32_t vers_id);
void read_mapscreens(PACKFILE *f, int32_t vers_id);
void do_savegamestructs(const bool v, const bool v2);
void do_loadgamestructs(const bool v, const bool v2);
int32_t Distance(double x1, double y1, double x2, double y2);
int32_t Distance(double x1, double y1, double x2, double y2, int32_t scale);
int32_t LongDistance(double x1, double y1, double x2, double y2);
int32_t LongDistance(double x1, double y1, double x2, double y2, int32_t scale);
void do_distance();

int32_t combo_script_engine(const bool preload, const bool waitdraw = false);

void do_strstr();
void do_strcat();
void do_strspn();
void do_strcspn();
void do_strchr();
void do_strrchr();
void do_xtoi2();
void do_remchr2();
void do_atoi2();
void do_ilen2();
void do_xlen2();
void do_itoa();
void do_itoacat();
void do_xtoa();

void do_tracebool(const bool v);
void do_tracestring();
void do_printf(const bool v, const bool varg);
void do_sprintf(const bool v, const bool varg);
void do_printfarr();
void do_sprintfarr();
void do_varg_max();
void do_varg_min();
void do_varg_choose();
void do_varg_makearray(ScriptType type, const uint32_t UID);
void do_breakpoint();
void do_trace(bool v);
void do_tracel(bool v);
void do_tracenl();
void do_cleartrace();
void do_tracetobase();
void ZScriptConsole(bool open);
template <typename ...Params>
void ZScriptConsole(int32_t attributes,const char *format, Params&&... params);
void TraceScriptIDs();
/*
int32_t getQuestHeaderInfo(int32_t type)
{
    return quest_format[type];
}


*/

//Script-only Warp, Hero->WarpEx(int32_t type, int32_t dmap, int32_t screen, int32_t x, int32_t y, int32_t effect, int32_t sound, int32_t flags, int32_t dir)
//Script-only Warp, Hero->WarpEx(int32_t array[])
//{int32_t type, int32_t dmap, int32_t screen, int32_t x, int32_t y, int32_t effect, int32_t sound, int32_t flags, int32_t dir}
void queueWarp(int32_t wtype, int32_t tdm, int32_t tscr, int32_t wx, int32_t wy,
	int32_t weff, int32_t wsfx, int32_t wflag, int32_t wdir);
bool warp_player(int32_t warpType, int32_t dmapID, int32_t scrID, int32_t warpDestX, int32_t warpDestY, int32_t warpEffect, int32_t warpSound, int32_t warpFlags, int32_t heroFacesDir);

void user_files_init();
void user_dirs_init();
void user_objects_init();
void user_stacks_init();
void user_rng_init();
void user_paldata_init();
void user_websockets_init();

bool get_scriptfile_path(char* buf, const char* path);

void do_fopen(const bool v, const char* f_mode);
void do_fremove();
void do_fclose();
void do_allocate_file();
void do_deallocate_file();
void do_file_isallocated();
void do_file_isvalid();
void do_fflush();
void do_file_readchars();
void do_file_readbytes();
void do_file_readstring();
void do_file_readints();
void do_file_writechars();
void do_file_writebytes();
void do_file_writestring();
void do_file_writeints();
void do_file_getchar();
void do_file_putchar();
void do_file_ungetchar();
void do_file_clearerr();
void do_file_rewind();
void do_file_seek();
void do_file_geterr();

void do_loaddirectory();
void do_loadstack();
void do_loadrng();
void do_create_paldata();
void do_create_paldata_clr();
void do_mix_clr();
void do_create_rgb_hex();
void do_create_rgb();
void do_convert_from_rgb();
void do_convert_to_rgb();
void do_paldata_load_level();
void do_paldata_load_sprite();
void do_paldata_load_main();
void do_paldata_load_cycle();
void do_paldata_load_bitmap();
void do_paldata_write_level();
void do_paldata_write_levelcset();
void do_paldata_write_sprite();
void do_paldata_write_spritecset();
void do_paldata_write_main();
void do_paldata_write_maincset();
void do_paldata_write_cycle();
void do_paldata_write_cyclecset();
void do_paldata_colorvalid();
void do_paldata_getcolor();
void do_paldata_setcolor();
void do_paldata_clearcolor();
void do_paldata_clearcset();
int32_t do_paldata_getrgb(int32_t v);
void do_paldata_setrgb(int32_t v, int32_t val);
void do_paldata_mix();
void do_paldata_mixcset();
void do_paldata_copy();
void do_paldata_copycset();
void do_directory_get();
void do_directory_reload();
void do_directory_free();

void user_bitmaps_init();

uint32_t get_free_bitmap(bool skipError = false);
void do_deallocate_bitmap();
bool isSystemBitref(int32_t ref);

uint32_t create_user_bitmap_ex(int32_t w, int32_t h);
void do_isvalidbitmap();
void do_isallocatedbitmap();

bool doing_dmap_enh_music(int32_t dm);
bool can_dmap_change_music(int32_t dm);
void do_set_music_position(const bool v);
void do_get_music_position();
void do_set_music_speed(const bool v);
void do_get_music_length();
void do_set_music_loop();

BITMAP* GetScriptBitmap(int32_t id, bool skipError = false);

int32_t highest_valid_user_bitmap();
int32_t do_create_bitmap();

void do_adjustsfxvolume(const bool v);
void do_adjustvolume(const bool v);
void do_warp_ex(const bool v);
//FFScript();
//static void init();

int32_t quest_format[versiontypesLAST];
byte skip_ending_credits; //checked in ending.cpp. If > 0, then we skip the game credits, but not Hero's Win script. -Z

byte system_suspend[susptLAST];

int32_t coreflags;
int32_t script_UIDs[UID_TYPES];
int32_t usr_midi_volume, usr_digi_volume, usr_sfx_volume, usr_music_volume, usr_panstyle;
byte music_update_cond, music_update_flags;

byte FF_hero_action; //This way, we can make safe replicas of internal Hero actions to be set by script. 
bool kb_typing_mode; //script only, for disbaling key presses affecting Hero, etc. 
int32_t FF_screenbounds[4]; //edges of the screen, left, right, top, bottom used for where to scroll. 
int32_t FF_screen_dimensions[4]; //height, width, displaywidth, displayheight
int32_t FF_subscreen_dimensions[4];
int32_t numscriptdraws;
bool skipscriptdraws;
int32_t FF_eweapon_removal_bounds[4]; //left, right, top, bottom coordinates for automatic eweapon removal. 
int32_t FF_lweapon_removal_bounds[4]; //left, right, top, bottom coordinates for automatic lweapon removal. 

std::vector<std::string> includePaths;
char includePathString[MAX_INCLUDE_PATH_CHARS];
char scriptRunString[21];
int32_t warpex[wexLast];
int32_t StdArray[256];
int32_t GhostArray[256];
int32_t TangoArray[256];

#define FFSCRIPTCLASS_CLOCKS 10
int32_t FF_clocks[FFSCRIPTCLASS_CLOCKS]; //Will be used for Heroaction, anims, and so forth 

#define SCRIPT_DRAWING_RULES 20
byte ScriptDrawingRules[SCRIPT_DRAWING_RULES];

#define NUM_USER_MIDI_OVERRIDES 6
int32_t FF_UserMidis[NUM_USER_MIDI_OVERRIDES]; //MIDIs to use for Game Over, and similar to override system defaults. 

//Enemy removal bounds
int32_t enemy_removal_point[6];

//Disable stepforward on dungeon dmaps
byte nostepforward;
byte temp_no_stepforward; 

byte subscreen_scroll_speed;

void set_sarg1(int32_t v);
void setSubscreenScrollSpeed(byte n);
int32_t getSubscreenScrollSpeed();
void do_fx_zap(const bool v);
void do_fx_wavy(const bool v);
void do_greyscale(const bool v);
void do_monochromatic(const bool v);
void gfxmonohue();
void Tint();
void clearTint();
//Advances the game frame without checking 'Quit' variable status.
//Used for making scripts such as Hero's onWin and onDeath scripts
//run for multiple frames.
void Waitframe(bool allowwavy = true, bool sfxcleanup = true);

void initZScriptDMapScripts();
void initZScriptOnMapScript();
void initZScriptScriptedActiveSubscreen();
void initZScriptSubscreenScript();
void initZScriptHeroScripts();
void initZScriptItemScripts();

void clear_script_engine_data();
void reset_script_engine_data(ScriptType type, int index = 0);
void clear_script_engine_data(ScriptType type, int index = 0);
void clear_script_engine_data_of_type(ScriptType type);
refInfo& ref(ScriptType type, int index);
byte& doscript(ScriptType type, int index = 0);
bool& waitdraw(ScriptType type, int index = 0);

//Combo Scripts
void clear_combo_scripts();
void clear_combo_script(int32_t lyr, int32_t pos);
void ClearComboScripts();
int32_t getComboDataLayer(int32_t c, ScriptType scripttype);
int32_t getCombodataPos(int32_t c, ScriptType scripttype);
int32_t getCombodataY(int32_t c, ScriptType scripttype);
int32_t getCombodataX(int32_t c, ScriptType scripttype);

//tba
//void deallocateComboArrays();

int32_t GetScriptObjectUID(int32_t type);
    
//byte item_messages_played[MAXITEMS]; //Each field is set when an item pickup message plays the first time per session
				//so that they do not play every time an item is collected, unless one of the flags is set for it.

void SetFFEngineFlag(int32_t flag, bool v);
void SetItemMessagePlayed(int32_t itm);
bool GetItemMessagePlayed(int32_t itm);
	
int32_t do_getpixel();
void do_bmpcollision();
int32_t GetDefaultWeaponSprite(int32_t w);

int32_t GetQuestVersion();
int32_t GetQuestBuild();
int32_t GetQuestSectionVersion(int32_t section);

int32_t do_get_internal_uid_npc(int32_t i);
int32_t do_get_internal_uid_item(int32_t i);
int32_t do_get_internal_uid_lweapon(int32_t i);
int32_t do_get_internal_uid_eweapon(int32_t i);

void do_loadnpc_by_script_uid(const bool v);
void do_loaditem_by_script_uid(const bool v);
void do_loadlweapon_by_script_uid(const bool v);
void do_loadeweapon_by_script_uid(const bool v);

int32_t getEnemyByScriptUID(int32_t sUID);
int32_t getLWeaponByScriptUID(int32_t sUID);
int32_t getEWeaponByScriptUID(int32_t sUID);

//new npc functions for npc scripts
void do_isdeadnpc();
void do_canslidenpc();
void do_slidenpc();
void do_npc_stopbgsfx();
void do_npcattack();
void do_npc_newdir();
void do_npc_constwalk();
void do_npc_varwalk();
void do_npc_varwalk8();
void do_npc_constwalk8();
void do_npc_haltwalk();
void do_npc_haltwalk8();
void do_npc_floatwalk();
void do_npc_breathefire();
void do_npc_newdir8();
int32_t npc_collision();
int32_t npc_linedup();
void do_npc_hero_in_range(const bool v);
void do_npc_simulate_hit(const bool v);
void do_npc_knockback(const bool v);
void do_npc_add(const bool v);
void do_npc_canmove(const bool v);
void get_npcdata_initd_label(const bool v);
void do_getnpcdata_getname();

//Deletion functions
void do_npc_delete();
void do_lweapon_delete();
void do_eweapon_delete();
bool do_itemsprite_delete();

//int32_t do_get_internal_uid

    //virtual ~FFScript();
    
    /*
    static item *temp_ff_item
    static enemy *temp_ff_enemy;
    static weapon *temp_ff_lweapon;
    static weapon *temp_ff_eweapon;

    static int32_t load_enemy(const int32_t eid, const char * const funcvar)
    {
        temp_ff_enemy = (enemy *) guys.getByUID(eid);
        
        if(temp_ff_enemy == NULL)
        {
            Z_scripterrlog("Invalid NPC with UID %ld passed to %s\nNPCs on screen have UIDs ", eid, funcvar);
            
            for(word i = 0; i < guys.Count(); i++)
                Z_scripterrlog("%ld ", guys.spr(i)->getUID());
                
            Z_scripterrlog("\n");
            return _InvalidSpriteUID;
        }
        
        return _NoError;
    }
    
    static INLINE enemy *get_enemy()
    {
        return temp_ff_enemy;
    }
    
    // Currently only used in a context where the enemy is known to be valid,
    // so there's no need to print an error
    static int32_t get_enemy_Index(const int32_t eid)
    {
        for(word i = 0; i < guys.Count(); i++)
        {
            if(guys.spr(i)->getUID() == eid)
                return i;
        }
        
        return -1;
    }
    
    static int32_t get_enemy_Misc(const byte a)
    {
        switch(a)
        {
        case 0: return temp_ff_enemy->dmisc1;
        case 1: return temp_ff_enemy->dmisc2;
        case 2: return temp_ff_enemy->dmisc3;
        case 3: return temp_ff_enemy->dmisc4;
        case 4: return temp_ff_enemy->dmisc5;
        case 5: return temp_ff_enemy->dmisc6;
        case 6: return temp_ff_enemy->dmisc7;
        case 7: return temp_ff_enemy->dmisc8;
        case 8: return temp_ff_enemy->dmisc9;
        case 9: return temp_ff_enemy->dmisc10;
        case 10: return temp_ff_enemy->dmisc11;
        case 11: return temp_ff_enemy->dmisc12;
        case 12: return temp_ff_enemy->dmisc13;
        case 13: return temp_ff_enemy->dmisc14;
        case 14: return temp_ff_enemy->dmisc15;
        case 15: return temp_ff_enemy->dmisc16;
        case 16: return temp_ff_enemy->dmisc17;
        case 17: return temp_ff_enemy->dmisc18;
        case 18: return temp_ff_enemy->dmisc19;
        case 19: return temp_ff_enemy->dmisc20;
        case 20: return temp_ff_enemy->dmisc21;
        case 21: return temp_ff_enemy->dmisc22;
        case 22: return temp_ff_enemy->dmisc23;
        case 23: return temp_ff_enemy->dmisc24;
        case 24: return temp_ff_enemy->dmisc25;
        case 25: return temp_ff_enemy->dmisc26;
        case 26: return temp_ff_enemy->dmisc27;
        case 27: return temp_ff_enemy->dmisc28;
        case 28: return temp_ff_enemy->dmisc29;
        case 29: return temp_ff_enemy->dmisc30;
        case 30: return temp_ff_enemy->dmisc31;
        case 31: return temp_ff_enemy->dmisc32;
        }
        
        return 0;
    }
    
    static bool enemy_hasHero()
    {
        if(temp_ff_enemy->family == eeWALLM)
            return ((eWallM *) temp_ff_enemy)->hashero;
            
        if(temp_ff_enemy->family == eeWALK)
            return ((eStalfos *) temp_ff_enemy)->hashero;
            
        return false;
    }
    
    static int32_t get_enemy_Misc_Flags()
    {
        flagpos = 5;
        // Must be in the same order as in the Enemy Editor pane
        return (temp_ff_enemy->flags&0x1F)
               | ornextflag(temp_ff_enemy->flags&(lens_only))
               | ornextflag(temp_ff_enemy->flags2&(guy_flashing))
               | ornextflag(temp_ff_enemy->flags2&(guy_blinking))
               | ornextflag(temp_ff_enemy->flags2&(guy_transparent))
               | ornextflag(temp_ff_enemy->flags&(inv_front))
               | ornextflag(temp_ff_enemy->flags&(inv_left))
               | ornextflag(temp_ff_enemy->flags&(inv_right))
               | ornextflag(temp_ff_enemy->flags&(inv_back))
               | ornextflag(temp_ff_enemy->flags&(guy_bkshield));
    }
    
    static INLINE void cleartemp_ff_enemy()
    {
        temp_ff_enemy = NULL;
    }
    



    static int32_t load_Item(const int32_t iid, const char * const funcvar)
    {
        temp_ff_item = (item *) items.getByUID(iid);
        
        if(temp_ff_item == NULL)
        {
            Z_scripterrlog("Invalid item with UID %ld passed to %s\nItems on screen have UIDs ", iid, funcvar);
            
            for(word i = 0; i < items.Count(); i++)
                Z_scripterrlog("%ld ", items.spr(i)->getUID());
                
            Z_scripterrlog("\n");
            return _InvalidSpriteUID;
        }
        
        return _NoError;
    }
    
    static int32_t getItemIndex(const int32_t iid)
    {
        for(word i = 0; i < items.Count(); i++)
        {
            if(items.spr(i)->getUID() == iid)
                return i;
        }
        
        return -1;
    }
    
    static INLINE item* get_Item()
    {
        return tempitem;
    }
    
    static INLINE void clearTempItem()
    {
        temp_ff_item = NULL;
    }
    



    static int32_t load_LWeapon(const int32_t wid, const char * const funcvar)
    {
        temp_ff_lweapon = (weapon *) Lwpns.getByUID(wid);
        
        if(temp_ff_lweapon == NULL)
        {
            Z_scripterrlog("Invalid lweapon with UID %ld passed to %s\nLWeapons on screen have UIDs ", wid, funcvar);
            
            for(word i = 0; i < Lwpns.Count(); i++)
                Z_scripterrlog("%ld ", Lwpns.spr(i)->getUID());
                
            Z_scripterrlog("\n");
            return _InvalidSpriteUID;
        }
        
        return _NoError;
    }
    
    static int32_t getLWeaponIndex(const int32_t lwid)
    {
        for(word i = 0; i < Lwpns.Count(); i++)
        {
            if(Lwpns.spr(i)->getUID() == lwid)
                return i;
        }
        
        return -1;
    }
    
    static INLINE weapon *get_LWeapon()
    {
        return temp_ff_lweapon;
    }
    
    //eweapon
    static INLINE void clearTempLWeapon()
    {
        temp_ff_lweapon = NULL;
    }
    static int32_t load_EWeapon(const int32_t wid, const char * const funcvar)
    {
        temp_ff_eweapon = (weapon *) Ewpns.getByUID(wid);
        
        if(temp_ff_eweapon == NULL)
        {
            Z_scripterrlog("Invalid eweapon with UID %ld passed to %s\nEWeapons on screen have UIDs ", wid, funcvar);
            
            for(word i = 0; i < Ewpns.Count(); i++)
                Z_scripterrlog("%ld ", Ewpns.spr(i)->getUID());
                
            Z_scripterrlog("\n");
            return _InvalidSpriteUID;
        }
        
        return _NoError;
    }
    
    static int32_t getEWeaponIndex(const int32_t lwid)
    {
        for(word i = 0; i < Ewpns.Count(); i++)
        {
            if(Ewpns.spr(i)->getUID() == lwid)
                return i;
        }
        
        return -1;
    }
    
    static INLINE weapon *get_EWeapon()
    {
        return temp_ff_eweapon;
    }
    
    static INLINE void clearTempEWeapon()
    {
        temp_ff_eweapon = NULL;
    }	
    */


	static INLINE int32_t ZSbound_byte(int32_t val)
	{
		return vbound(val,0,ZS_BYTE);
	}
	static INLINE int32_t ZSbound_char(int32_t val)
	{
		return vbound(val,0,ZS_CHAR);
	}
	static INLINE int32_t ZSbound_word(int32_t val)
	{
		return vbound(val,0,ZS_WORD);
	}
	static INLINE int32_t ZSbound_short(int32_t val)
	{
		return vbound(val,0,ZS_SHORT);
	}
	static INLINE int32_t ZSbound_long(int32_t val)
	{
		return vbound(val,0,ZS_LONG);
	}
	static INLINE int32_t ZSbound_fix(int32_t val)
	{
		return vbound(val,0,ZS_FIX);
	}
	
static void set_screenwarpReturnY(mapscr *m, int32_t d, int32_t value);
static void set_screendoor(mapscr *m, int32_t d, int32_t value);
static void set_screenenemy(mapscr *m, int32_t index, int32_t value);
static void set_screenlayeropacity(mapscr *m, int32_t d, int32_t value);
static void set_screensecretcombo(mapscr *m, int32_t d, int32_t value);
static void set_screensecretcset(mapscr *m, int32_t d, int32_t value);
static void set_screensecretflag(mapscr *m, int32_t d, int32_t value);
static void set_screenlayermap(mapscr *m, int32_t d, int32_t value);
static void set_screenlayerscreen(mapscr *m, int32_t d, int32_t value);
static void set_screenpath(mapscr *m, int32_t d, int32_t value);
static void set_screenwarpReturnX(mapscr *m, int32_t d, int32_t value);
static void set_screenGuy(mapscr *m, int32_t value);
static void set_screenString(mapscr *m, int32_t value);
static void set_screenRoomtype(mapscr *m, int32_t value);
static void set_screenEntryX(mapscr *m, int32_t value);
static void set_screenEntryY(mapscr *m, int32_t value);
static void set_screenitem(mapscr *m, int32_t value);
static void set_screenundercombo(mapscr *m, int32_t value);
static void set_screenundercset(mapscr *m, int32_t value);
static void set_screenatchall(mapscr *m, int32_t value);
static void deallocateZScriptArray(const int32_t ptrval);
static int32_t get_screen_d(int32_t index1, int32_t index2);
static void set_screen_d(int32_t index1, int32_t index2, int32_t val);
static int32_t whichlayer(int32_t scr);

static void do_zapout();
static void do_zapin();
static void do_openscreen();
static void do_closescreen();
static void do_openscreenshape();
static void do_closescreenshape();
static void do_wavyin();
static void do_wavyout();
static void do_triggersecret(const bool v);
static void do_changeffcscript(const bool v);

static void setHeroDiagonal(bool v);
static bool getHeroDiagonal();
static bool getHeroBigHitbox();
static void setHeroBigHitbox(bool v);




//NPCData getters One Input, One Return
	static void getNPCData_flags(); //word
	static void getNPCData_flags2();
	static void getNPCData_width();
	static void getNPCData_height();
	static void getNPCData_s_tile();
	static void getNPCData_s_width();
	static void getNPCData_s_height();
	static void getNPCData_e_tile();
	static void getNPCData_e_width();
	static void getNPCData_hp();
	static void getNPCData_family();
	static void getNPCData_cset();
	static void getNPCData_anim();
	static void getNPCData_e_anim();
	static void getNPCData_frate();
	static void getNPCData_e_frate();
	static void getNPCData_dp();
	static void getNPCData_wdp();
	static void getNPCData_weapon();
	static void getNPCData_rate();
	static void getNPCData_hrate();
	static void getNPCData_step();
	static void getNPCData_homing();
	static void getNPCData_grumble();
	static void getNPCData_item_set();
	static void getNPCData_bgsfx();
	static void getNPCData_hitsfx();
	static void getNPCData_deadsfx();
	static void getNPCData_xofs();
	static void getNPCData_yofs();
	static void getNPCData_zofs();
	static void getNPCData_hxofs();
	static void getNPCData_hyofs();
	static void getNPCData_hxsz();
	static void getNPCData_hysz();
	static void getNPCData_hzsz();
	static void getNPCData_txsz();
	static void getNPCData_tysz();
	static void getNPCData_wpnsprite();

	//NPCData Getters, two inouts, one return



	//static void getNPCData_scriptdefence();
	static void getNPCData_misc();//switch-case
	static void getNPCData_defense(); //extra arg
	static void getNPCData_SIZEflags();


	//NPCData Setters, two inputs, no return.

	static void setNPCData_flags(); //word
	static void setNPCData_flags2();
	static void setNPCData_width();
	static void setNPCData_height();
	static void setNPCData_s_tile();
	static void setNPCData_s_width();
	static void setNPCData_s_height();
	static void setNPCData_e_tile();
	static void setNPCData_e_width();
	static void setNPCData_hp();
	static void setNPCData_family();
	static void setNPCData_cset();
	static void setNPCData_anim();
	static void setNPCData_e_anim();
	static void setNPCData_frate();
	static void setNPCData_e_frate();
	static void setNPCData_dp();
	static void setNPCData_wdp();
	static void setNPCData_weapon();
	static void setNPCData_rate();
	static void setNPCData_hrate();
	static void setNPCData_step();
	static void setNPCData_homing();
	static void setNPCData_grumble();
	static void setNPCData_item_set();
	static void setNPCData_bgsfx();
	static void setNPCData_hitsfx();
	static void setNPCData_deadsfx();
	static void setNPCData_xofs();
	static void setNPCData_yofs();
	static void setNPCData_zofs();
	static void setNPCData_hxofs();
	static void setNPCData_hyofs();
	static void setNPCData_hxsz();
	static void setNPCData_hysz();
	static void setNPCData_hzsz();
	static void setNPCData_txsz();
	static void setNPCData_tysz();
	static void setNPCData_wpnsprite();

	//NPCData Setters, three inputs, no return.
	//static void setNPCData_scriptdefence();
	static void setNPCData_defense(int32_t v); //extra arg
	static void setNPCData_SIZEflags(int32_t v);
	static void setNPCData_misc(int32_t val);
	
	static void setNPCData_tile();
	static void setNPCData_e_height();
	static void getNPCData_tile();
	static void getNPCData_e_height();
	
	//one input, one return
	static void getComboData_block_enemies();
	static void getComboData_block_hole();
	static void getComboData_block_trigger();
	static void getComboData_conveyor_x_speed();
	static void getComboData_conveyor_y_speed();
	static void getComboData_create_enemy();
	static void getComboData_create_enemy_when();
	static void getComboData_create_enemy_change();
	static void getComboData_directional_change_type();
	static void getComboData_distance_change_tiles();
	static void getComboData_dive_item();
	static void getComboData_dock();
	static void getComboData_fairy();
	static void getComboData_ff_combo_attr_change();
	static void getComboData_foot_decorations_tile();
	static void getComboData_foot_decorations_type();
	static void getComboData_hookshot_grab_point();
	static void getComboData_ladder_pass();
	static void getComboData_lock_block_type();
	static void getComboData_lock_block_change();
	static void getComboData_magic_mirror_type();
	static void getComboData_modify_hp_amount();
	static void getComboData_modify_hp_delay();
	static void getComboData_modify_hp_type();
	static void getComboData_modify_mp_amount();
	static void getComboData_modify_mp_delay();
	static void getComboData_modify_mp_type();
	static void getComboData_no_push_blocks();
	static void getComboData_overhead();
	static void getComboData_place_enemy();
	static void getComboData_push_direction();
	static void getComboData_push_weight();
	static void getComboData_push_wait();
	static void getComboData_pushed();
	static void getComboData_raft();
	static void getComboData_reset_room();
	static void getComboData_save_point_type();
	static void getComboData_screen_freeze_type();

	static void getComboData_secret_combo();
	static void getComboData_singular();
	static void getComboData_slow_movement();
	static void getComboData_statue_type();
	static void getComboData_step_type();
	static void getComboData_step_change_to();
	static void getComboData_strike_remnants();
	static void getComboData_strike_remnants_type();
	static void getComboData_strike_change();
	static void getComboData_strike_item();
	static void getComboData_touch_item();
	static void getComboData_touch_stairs();
	static void getComboData_trigger_type();
	static void getComboData_trigger_sensitive();
	static void getComboData_warp_type();
	static void getComboData_warp_sensitive();
	static void getComboData_warp_direct();
	static void getComboData_warp_location();
	static void getComboData_water();
	static void getComboData_whistle();
	static void getComboData_win_game();
	static void getComboData_block_weapon_lvl();

	static void getComboData_tile();
	static void getComboData_flip();

	static void getComboData_walk();
	static void getComboData_type();
	static void getComboData_csets();
	static void getComboData_frames();
	static void getComboData_speed();
	static void getComboData_nextcombo();
	static void getComboData_nextcset();
	static void getComboData_flag();
	static void getComboData_skipanim();
	static void getComboData_nexttimer();
	static void getComboData_skipanimy();
	static void getComboData_animflags();

	//two inputs, one return
	static void getComboData_block_weapon();
	static void getComboData_expansion();
	static void getComboData_strike_weapons();

	//two inputs, no return
	static void setComboData_block_enemies();
	static void setComboData_block_hole();
	static void setComboData_block_trigger();
	static void setComboData_conveyor_x_speed();
	static void setComboData_conveyor_y_speed();
	static void setComboData_create_enemy();
	static void setComboData_create_enemy_when();
	static void setComboData_create_enemy_change();
	static void setComboData_directional_change_type();
	static void setComboData_distance_change_tiles();
	static void setComboData_dive_item();
	static void setComboData_dock();
	static void setComboData_fairy();
	static void setComboData_ff_combo_attr_change();
	static void setComboData_foot_decorations_tile();
	static void setComboData_foot_decorations_type();
	static void setComboData_hookshot_grab_point();
	static void setComboData_ladder_pass();
	static void setComboData_lock_block_type();
	static void setComboData_lock_block_change();
	static void setComboData_magic_mirror_type();
	static void setComboData_modify_hp_amount();
	static void setComboData_modify_hp_delay();
	static void setComboData_modify_hp_type();
	static void setComboData_modify_mp_amount();
	static void setComboData_modify_mp_delay();
	static void setComboData_modify_mp_type();
	static void setComboData_no_push_blocks();
	static void setComboData_overhead();
	static void setComboData_place_enemy();
	static void setComboData_push_direction();
	static void setComboData_push_weight();
	static void setComboData_push_wait();
	static void setComboData_pushed();
	static void setComboData_raft();
	static void setComboData_reset_room();
	static void setComboData_save_point_type();
	static void setComboData_screen_freeze_type();

	static void setComboData_secret_combo();
	static void setComboData_singular();
	static void setComboData_slow_movement();
	static void setComboData_statue_type();
	static void setComboData_step_type();
	static void setComboData_step_change_to();
	static void setComboData_strike_remnants();
	static void setComboData_strike_remnants_type();
	static void setComboData_strike_change();
	static void setComboData_strike_item();
	static void setComboData_touch_item();
	static void setComboData_touch_stairs();
	static void setComboData_trigger_type();
	static void setComboData_trigger_sensitive();
	static void setComboData_warp_type();
	static void setComboData_warp_sensitive();
	static void setComboData_warp_direct();
	static void setComboData_warp_location();
	static void setComboData_water();
	static void setComboData_whistle();
	static void setComboData_win_game();
	static void setComboData_block_weapon_lvl();

	static void setComboData_tile();
	static void setComboData_flip();

	static void setComboData_walk();
	static void setComboData_type();
	static void setComboData_csets();
	static void setComboData_frames();
	static void setComboData_speed();
	static void setComboData_nextcombo();
	static void setComboData_nextcset();
	static void setComboData_flag();
	static void setComboData_skipanim();
	static void setComboData_nexttimer();
	static void setComboData_skipanimy();
	static void setComboData_animflags();

	//three inputs, no return
	static void setComboData_block_weapon(int32_t v);
	static void setComboData_expansion(int32_t v);
	static void setComboData_strike_weapons(int32_t v);
	
	//SpriteData
	static void getSpriteDataTile();
	static void getSpriteDataMisc();
	static void getSpriteDataCSets();
	static void getSpriteDataFrames();
	static void getSpriteDataSpeed();
	static void getSpriteDataType();
	static void getSpriteDataString();
	static void setSpriteDataTile();
	static void setSpriteDataMisc();
	static void setSpriteDataCSets();
	static void setSpriteDataFrames();
	static void setSpriteDataSpeed();
	static void setSpriteDataType();
	static void setSpriteDataString();

	//Change Save/Continue and Retry screen settings:
	static void FFSetSaveScreenSetting();
	static void FFChangeSubscreenText();
	
	static void do_typedpointer_typecast(const bool v);
	
	//New Datatypes
	static void do_loadnpcdata(const bool v);
	static void do_loadcombodata(const bool v);
	static void do_loadmapdata(const bool v);
	static void do_loadmapdata_tempscr(const bool v);
	static void do_loadmapdata_scrollscr(const bool v);
	static int32_t loadMapData();
	static void do_loadspritedata(const bool v);
	static void do_loadscreendata(const bool v);
	static void do_loadbitmapid(const bool v);
	static void do_loadshopdata(const bool v);
	static void do_loadinfoshopdata(const bool v);
	static void do_setMIDI_volume(int32_t m);
	static void do_setMusic_volume(int32_t m);
	static void do_setDIGI_volume(int32_t m);
	static void do_setSFX_volume(int32_t m);
	static void do_setSFX_pan(int32_t m);
	
	static int32_t do_getMIDI_volume();
	static int32_t do_getMusic_volume();
	static int32_t do_getDIGI_volume();
	static int32_t do_getSFX_volume();
	static int32_t do_getSFX_pan();
	
	static void do_loadmessagedata(const bool v);
	static void do_messagedata_setstring(const bool v);
	static void do_messagedata_getstring(const bool v);
	
	static void do_loaddmapdata(const bool v);
	static void do_loaddropset(const bool v);
	static void do_loadbottle(const bool v);
	static void do_loadbottleshop(const bool v);
	static void do_loadgenericdata(const bool v);
	static void do_getDMapData_dmapname(const bool v);
	static void do_setDMapData_dmapname(const bool v);
	static void do_getDMapData_dmaptitle(const bool v);
	static void do_setDMapData_dmaptitle(const bool v);
	static void do_getDMapData_dmapintro(const bool v);
	static void do_setDMapData_dmapintro(const bool v);
	static void do_getDMapData_music(const bool v);
	static void do_setDMapData_music(const bool v);
	
	static void do_load_active_subscreendata(const bool v);
	static void do_load_passive_subscreendata(const bool v);
	static void do_load_overlay_subscreendata(const bool v);
	static void do_load_subscreendata(const bool v, const bool v2);
	
	static void do_checkdir(const bool is_dir);
	static void do_fs_remove();

#define INVALIDARRAY localRAM[0]  //localRAM[0] is never used

enum __Error
    {
        _NoError, //OK!
        _Overflow, //script array too small
        _InvalidPointer, //passed NULL pointer or similar
        _OutOfBounds, //library array out of bounds
        _InvalidSpriteUID //bad npc, ffc, etc.
    };
    
	static void deallocateAllScriptOwned(ScriptType scriptType, const int32_t UID, bool requireAlways = true);
	static void deallocateAllScriptOwned();
	static void deallocateAllScriptOwnedCont();

	user_object& create_user_object(uint32_t id);
	std::vector<user_object*> get_user_objects();
	user_object* get_user_object(uint32_t id);
	
    private:
    int32_t sid;
};

extern int32_t ffmisc[MAXFFCS][16];
extern PALETTE tempgreypal; //script greyscale
extern PALETTE userPALETTE[256];
extern PALETTE tempblackpal;

void script_init_name_to_slot_index_maps();

int32_t get_register(int32_t arg);
void set_register(int32_t arg, int32_t value);
int32_t run_script(ScriptType type, const word script, const int32_t i = -1); //Global scripts don't need 'i'
int32_t ffscript_engine(const bool preload);

void deallocateArray(const int32_t ptrval);
void clearScriptHelperData();

void do_getscreenflags();
void do_getscreeneflags();
int32_t get_screendoor(mapscr *m, int32_t d);
int32_t get_screenlayeropacity(mapscr *m, int32_t d);
int32_t get_screensecretcombo(mapscr *m, int32_t d);
int32_t get_screensecretcset(mapscr *m, int32_t d);
int32_t get_screensecretflag(mapscr *m, int32_t d);
int32_t get_screenlayermap(mapscr *m, int32_t d);
int32_t get_screenlayerscreen(mapscr *m, int32_t d);
int32_t get_screenpath(mapscr *m, int32_t d);
int32_t get_screenwarpReturnX(mapscr *m, int32_t d);
int32_t get_screenwarpReturnY(mapscr *m, int32_t d);

int32_t get_screenGuy(mapscr *m);
int32_t get_screenString(mapscr *m);
int32_t get_screenRoomtype(mapscr *m);
int32_t get_screenViewY(mapscr *m);
int32_t get_screenEntryX(mapscr *m);
int32_t get_screenEntryY(mapscr *m);
int32_t get_screenitem(mapscr *m);
int32_t get_screenundercombo(mapscr *m);
int32_t get_screenundercset(mapscr *m);
int32_t get_screenatchall(mapscr *m);
void do_getscreenLayerOpacity();
void do_getscreenSecretCombo();
void do_getscreenSecretCSet();
void do_getscreenSecretFlag();
void do_getscreenLayerMap();
void do_getscreenLayerscreen();
void do_getscreenPath();
void do_getscreenWarpReturnX();
void do_getscreenWarpReturnY();
void do_getscreenatchall();
void do_getscreenUndercombo();
void do_getscreenUnderCSet();
void do_getscreenWidth();
void do_getscreenHeight();
void do_getscreenGuy();
void do_getscreenString();
void do_getscreenRoomType();
void do_getscreenEntryX();
void do_getscreenEntryY();
void do_getscreenItem();
void do_getscreendoor();
int32_t get_screennpc(mapscr *m, int32_t index);
void do_getscreennpc();

// Defines for script flags
#define TRUEFLAG          0x0001
#define MOREFLAG          0x0002
#define TRIGGERED         0x0004

struct quad3Dstruct
{
    int32_t index;
    float pos[12], uv[8];
    int32_t size[2], color[4];
};

struct triangle3Dstruct
{
    int32_t index;
    float pos[9], uv[6];
    int32_t size[2], color[3];
};

bool command_is_wait(int command);
bool command_is_goto(int command);
bool command_uses_comparison_result(int command);
bool command_writes_comparison_result(int command);
int command_to_cmp(int command, int arg);
bool command_could_return_not_ok(int command);
// Returns true if the command has no side effects other than
// potentially writing some value to a register pointed at by
// on of its args.
// The optimizer uses this to know if it is safe to remove a
// command, given its register output is not needed.
bool command_is_pure(int command);

int32_t get_combopos_ref(int32_t pos, int32_t layer);
int32_t combopos_ref_to_pos(int32_t combopos_ref);
int32_t combopos_ref_to_layer(int32_t combopos_ref);

void init_script_objects();
bool pc_overflow(dword pc, bool print_err = true);

#endif

