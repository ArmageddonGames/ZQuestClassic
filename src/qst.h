#ifndef _ZC_QST_H_
#define _ZC_QST_H_

#include "base/zdefs.h"
#include "subscr.h"
#include "zscriptversion.h"

struct miscQdata;

#include "zc/ffscript.h"
extern FFScript ffengine;

struct script_slot_data
{
	std::string slotname;
	std::string scriptname;
	byte format;
	std::string output;
	
	script_slot_data() : slotname(""), scriptname(""), format(SCRIPT_FORMAT_DEFAULT), output("") {}
	void update()
	{
		char const* formatstr = getFormatStr()->c_str();
		char const* slotstr = slotname.c_str();
		char const* scriptstr = scriptname.c_str();
		size_t len = strlen(formatstr) + strlen(slotstr) + strlen(scriptstr) - 4 + 1;
		char* temp = (char*)malloc(len);
		ASSERT(temp);
		sprintf(temp, formatstr, slotstr, scriptstr);
		temp[len - 1] = 0;
		output = temp;
		free(temp);
	}
	
	void updateName(std::string newname)
	{
		if(newname.at(0) == '+' || newname.at(0) == '=')
		{
			scriptname = newname.substr(2);
		}
		else scriptname = newname;
		update();
	}
	
	void clear()
	{
		slotname = "";
		scriptname = "";
		format = SCRIPT_FORMAT_DEFAULT;
		output = "";
	}
	
	bool hasScriptData()
	{
		return (scriptname != "") && (format != SCRIPT_FORMAT_INVALID);
	}
	
	bool isEmpty()
	{
		return scriptname == "";
	}
	
	std::string const* getFormatStr()
	{
		switch(format)
		{
			case SCRIPT_FORMAT_DEFAULT:
				return &DEFAULT_FORMAT;
			case SCRIPT_FORMAT_INVALID:
				return &INVALID_FORMAT;
		}
		return &DEFAULT_FORMAT;
	}
	
	static const std::string DEFAULT_FORMAT;
	static const std::string INVALID_FORMAT;
};

extern std::map<int32_t, script_slot_data > ffcmap;
extern std::map<int32_t, script_slot_data > globalmap;
extern std::map<int32_t, script_slot_data > genericmap;
extern std::map<int32_t, script_slot_data > itemmap;
extern std::map<int32_t, script_slot_data > npcmap;
extern std::map<int32_t, script_slot_data > ewpnmap;
extern std::map<int32_t, script_slot_data > lwpnmap;
extern std::map<int32_t, script_slot_data > playermap;
extern std::map<int32_t, script_slot_data > dmapmap;
extern std::map<int32_t, script_slot_data > screenmap;
extern std::map<int32_t, script_slot_data > itemspritemap;
extern std::map<int32_t, script_slot_data > comboscriptmap;
extern std::map<int32_t, script_slot_data > subscreenmap;

#define MAX_COMBO_COLS 4
extern int32_t First[MAX_COMBO_COLS],combo_alistpos[MAX_COMBO_COLS],combo_pool_listpos[MAX_COMBO_COLS], combo_auto_listpos[MAX_COMBO_COLS];
typedef struct map_and_screen
{
    int32_t map;
    int32_t screen;
} map_and_screen;
#define MAX_MAPPAGE_BTNS 9
extern map_and_screen map_page[MAX_MAPPAGE_BTNS];
// define these in main code
//extern bool init_tiles(bool validate);
//extern bool init_combos(bool validate);

#define checkstatus(ret)\
          switch (ret)  \
        {               \
          case 0:       \
          break;        \
          case qe_invalid:\
          goto invalid; \
          break;        \
          default:      \
          pack_fclose(f);\
          if(!oldquest) \
            delete_file(tmpfilename);\
          return ret;   \
          break;        \
        }

extern bool bad_version(int32_t ver);

enum
{
    qe_OK, qe_notfound, qe_invalid, qe_version, qe_obsolete,
    qe_missing, qe_internal, qe_pwd, qe_match, qe_minver,
    qe_nomem, qe_debug, qe_cancel, qe_silenterr, qe_no_qst
};

enum
{
    zgp_tiles, zgp_combos, zgp_pals, zgp_items, zgp_wpns, zgp_misc,
    zgp_maps, zgp_doors
};

enum
{
    skip_header, skip_rules, skip_strings, skip_misc,
    skip_tiles, skip_combos, skip_comboaliases, skip_csets,
    skip_maps, skip_dmaps, skip_doors, skip_items,
    skip_weapons, skip_colors, skip_icons, skip_initdata,
    skip_guys, skip_herosprites, skip_subscreens, skip_ffscript,
    skip_sfx, skip_midis, skip_cheats, skip_itemdropsets,
    skip_favorites, skip_zinfo, skip_max
};

extern const char *qst_error[];
extern word msg_count;
extern word door_combo_set_count;
extern int32_t favorite_combos[MAXFAVORITECOMBOS];
extern byte favorite_combo_modes[MAXFAVORITECOMBOS];

char *VerStrFromHex(int32_t version);
char *ordinal(int32_t num);

void clear_quest_tmpfile();
PACKFILE *open_quest_file(int32_t *open_error, const char *filename, bool show_progress);
PACKFILE *open_quest_template(zquestheader *Header, char *deletefilename, bool validate);

void clear_combo(int32_t i);
void clear_combos();
void pack_combos();
void fix_maps(mapscr *buf,int32_t cnt);
//void reset_midi(zcmidi_ *m);
//void reset_midis(zcmidi_ *m);
void reset_scr(int32_t scr);
//bool reset_items();
//bool reset_wpns();

int32_t get_qst_buffers();
void del_qst_buffers();
int32_t count_dmaps();
int32_t count_shops(miscQdata *Misc);
int32_t count_infos(miscQdata *Misc);
int32_t count_warprings(miscQdata *Misc);
int32_t count_palcycles(miscQdata *Misc);

void port250QuestRules();
void portCandleRules();
void portBombRules();

bool is_loading_quest();
int32_t loadquest(const char *filename, zquestheader *Header, miscQdata *Misc,
	zctune *tunes, bool show_progress, byte *skip_flags, byte printmetadata = 1,
	bool report = true, byte qst_num = 0, dword tileset_flags = 0);

char *byte_conversion(int32_t number, int32_t format);
char *byte_conversion2(int32_t number1, int32_t number2, int32_t format1, int32_t format2);

bool valid_zqt(PACKFILE *f);
bool valid_zqt(const char *filename);
bool reset_mapstyles(bool validate, miscQdata *Misc);
bool reset_items(bool validate, zquestheader *Header);
bool reset_guys();
bool reset_wpns(bool validate, zquestheader *Header);
bool init_tiles(bool validate, zquestheader *Header);
bool init_colordata(bool validate, zquestheader *Header, miscQdata *Misc);
bool init_combos(bool validate, zquestheader *Header);

void get_questpwd(char *encrypted_pwd, int16_t pwdkey, char *pwd);
bool devpwd();
bool check_questpwd(zquestheader *Header, char *pwd);

enum
{
	KEYFILE_MASTER,
	KEYFILE_CHEAT,
	KEYFILE_ZPWD,
	KEYFILE_NUMTY
};
bool check_keyfiles(char const* path, vector<uint> types, zquestheader* Header);

void update_guy_1(guydata *tempguy);
void initMsgStr(MsgStr *str);
void init_msgstrings(int32_t start, int32_t end);

int32_t copyquest(PACKFILE *f);
void print_quest_metadata(zquestheader const& tempheader, char const* path = NULL, byte qst_num = 0);
int32_t readheader(PACKFILE *f, zquestheader *Header, byte printmetadata = 0);
int32_t readrules(PACKFILE *f, zquestheader *Header);
int32_t readstrings(PACKFILE *f, zquestheader *Header);
int32_t readdoorcombosets(PACKFILE *f, zquestheader *Header);
int32_t readdmaps(PACKFILE *f, zquestheader *Header, word version, word build, word start_dmap, word max_dmaps);
int32_t readmisccolors(PACKFILE *f, zquestheader *Header, miscQdata *Misc);
int32_t readgameicons(PACKFILE *f, zquestheader *Header, miscQdata *Misc);
int32_t readmisc(PACKFILE *f, zquestheader *Header, miscQdata *Misc);
int32_t readitems(PACKFILE *f, word version, word build);
int32_t readweapons(PACKFILE *f, zquestheader *Header);
int32_t readguys(PACKFILE *f, zquestheader *Header);
int32_t readmapscreen(PACKFILE *f, zquestheader *Header, mapscr *temp_mapscr, word version, int scrind = -1);
int32_t readmaps(PACKFILE *f, zquestheader *Header);
int32_t readcombos(PACKFILE *f, zquestheader *Header, word version, word build, word start_combo, word max_combos);
int32_t readcomboaliases(PACKFILE *f, zquestheader *Header, word version, word build);
int32_t readcolordata(PACKFILE *f, miscQdata *Misc, word version, word build, word start_cset, word max_csets);
int32_t readtiles(PACKFILE *f, tiledata *buf, zquestheader *Header, word version, word build, word start_tile, int32_t max_tiles, bool from_init);
int32_t readtunes(PACKFILE *f, zquestheader *Header, zctune *tunes);
int32_t readcheatcodes(PACKFILE *f, zquestheader *Header);
int32_t readinitdata(PACKFILE *f, zquestheader *Header);
int32_t readffscript(PACKFILE *f, zquestheader *Header);
int32_t read_quest_zasm(PACKFILE *f, word s_version);
int32_t read_one_ffscript(PACKFILE *f, zquestheader *Header, int32_t i, word s_version, word s_cversion, script_data **script, word zmeta_version);
int32_t read_old_ffscript(PACKFILE *f, int32_t script_index, word s_version, script_data **script, word zmeta_version);
int32_t readsfx(PACKFILE *f, zquestheader *Header);
int32_t readitemdropsets(PACKFILE *f, word version, word build);
int32_t readfavorites(PACKFILE *f, int32_t, word);

int32_t readsubscreens(PACKFILE *f);
int32_t read_old_subscreens(PACKFILE *f, word s_version);
int32_t read_one_old_subscreen(PACKFILE* f, subscreen_group* g, word s_version);

void init_msgstr(MsgStr *str);

int32_t get_version_and_build(PACKFILE *f, word *version, word *build);
bool find_section(PACKFILE *f, int32_t section_id_requested);

void reset_subscreen(subscreen_group *tempss);
void reset_subscreens();
int32_t setupsubscreens();
void setupsfx();
void reset_itembuf(itemdata *item, int32_t id);
void reset_itemname(int32_t id);
void reset_weaponname(int32_t i);
void init_guys(int32_t guyversion);
void init_item_drop_sets();
void init_favorites();

#define CHAS_ATTRIB   0x01
#define CHAS_FLAG     0x02
#define CHAS_TRIG     0x04
#define CHAS_ANIM     0x08
#define CHAS_SCRIPT   0x10
#define CHAS_BASIC    0x20
#define CHAS_LIFT     0x40
#define CHAS_GENERAL  0x80

#define SCRHAS_ROOMDATA  0x00000001
#define SCRHAS_ITEM      0x00000002
#define SCRHAS_TWARP     0x00000004
#define SCRHAS_SWARP     0x00000008
#define SCRHAS_WARPRET   0x00000010
#define SCRHAS_LAYERS    0x00000020
#define SCRHAS_MAZE      0x00000040
#define SCRHAS_D_S_U     0x00000080
#define SCRHAS_FLAGS     0x00000100
#define SCRHAS_ENEMY     0x00000200
#define SCRHAS_CARRY     0x00000400
#define SCRHAS_SCRIPT    0x00000800
#define SCRHAS_UNUSED    0x00001000
#define SCRHAS_SECRETS   0x00002000
#define SCRHAS_COMBOFLAG 0x00004000
#define SCRHAS_MISC      0x00008000



#define TILESET_CLEARMAPS      0x01
#define TILESET_CLEARSCRIPTS   0x02
#define TILESET_CLEARHEADER    0x04

#endif                                                      // _ZC_QST_H_
