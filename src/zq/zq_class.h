#ifndef _ZQ_CLASS_H_
#define _ZQ_CLASS_H_

#include "base/zdefs.h"
#include "base/mapscr.h"
#include "base/containers.h"
#include <stdio.h>
#include <string_view>
#include <deque>
#include <memory>
#include <stack>
#include <array>

struct ZCSubscreen;

int32_t COMBOPOS(int32_t x, int32_t y);
int32_t COMBOPOS_B(int32_t x, int32_t y);
int32_t COMBOX(int32_t pos);
int32_t COMBOY(int32_t pos);

void set_preview_mode(int32_t prv);

/************************/
/****** ZMAP class ******/
/************************/

class user_input_command
{
public:
    // The screen the user was on when command was created.
    int view_map;
    int view_scr;

    virtual void execute() = 0;
    virtual void undo() = 0;
    // Rough estimate of how much memory retained by the command, in terms of sizeof(mapscr).
    int size() { return 0; }
};

class list_command : public user_input_command
{
public:
    std::vector<std::shared_ptr<user_input_command>> commands;

    void execute();
    void undo();
    int size();
};

class set_combo_command : public user_input_command
{
public:
    int map, scr, pos, combo, cset;
    int prev_combo, prev_cset;

    void execute();
    void undo();
};

class set_ffc_command : public user_input_command
{
public:
	struct data_t {
		zfix x, y, vx, vy, ax, ay;
		int data, cset, delay, link, script;
		int tw, th;
		int ew, eh;
		dword flags;
		std::array<int, 2> inita;
		std::array<int, 8> initd;
		bool operator==(data_t const&) const = default;
	};
	static data_t create_data(const ffcdata& ffc);

    int map, scr, i;
	data_t data, prev_data;

    void execute();
    void undo();
};

class set_flag_command : public user_input_command
{
public:
    int map, scr, pos, flag;
    int prev_flag;

    void execute();
    void undo();
};

class set_door_command : public user_input_command
{
public:
    int side, door;
    int prev_door;

    void execute();
    void undo();
};

class set_dcs_command : public user_input_command
{
public:
    int dcs;
    int prev_dcs;

    void execute();
    void undo();
};

enum PasteCommandType {
    ScreenAll,
    ScreenAllToEveryScreen,
    ScreenData,
    ScreenDoors,
    ScreenEnemies,
    ScreenFFCombos,
    ScreenGuy,
    ScreenLayers,
    ScreenPalette,
    ScreenPartial,
    ScreenPartialToEveryScreen,
    ScreenRoom,
    ScreenSecretCombos,
    ScreenUnderCombo,
    ScreenWarpLocations,
    ScreenWarps,
};

class paste_screen_command : public user_input_command
{
public:
    PasteCommandType type;
    int data;
    std::shared_ptr<mapscr> screen;
    std::vector<std::shared_ptr<mapscr>> prev_screens;

    void execute();
    void undo();
    int size();

private:
    void perform(mapscr* to);
};

class set_screen_command : public user_input_command
{
public:
    std::shared_ptr<mapscr> screen;
    std::shared_ptr<mapscr> prev_screen;

    void execute();
    void undo();
    int size();
};

class tile_grid_draw_command : public user_input_command
{
public:
	byte tile_grid[15][20];
	byte prev_tile_grid[15][20];

    void execute();
    void undo();
};

void reset_dmap(int32_t index);
void truncate_dmap_title(std::string& title);
//void mapfix_0x166(mapscr *scr);
bool setMapCount2(int32_t c);
class zmap
{
    mapscr *screens;
    int32_t currmap,copymap;
    int32_t currscr,copyscr;
	optional<int32_t> warpbackmap, warpbackscreen;
    int32_t copyffc;
    int32_t scrpos[MAXMAPS+1];
	
	bounded_map<dword,int32_t> copyscrdata;
	
    mapscr copymapscr;
    mapscr prvscr; //NEW
    mapscr prvlayers[6];
    std::deque<std::shared_ptr<user_input_command>> undo_stack;
    std::stack<std::shared_ptr<user_input_command>> redo_stack;
    //int32_t prv_mode; //NEW
    int32_t prv_cmbcycle, prv_map, prv_scr, prv_freeze, prv_advance, prv_time; //NEW
    bool can_paste;
    // A screen which uses the current screen as a layer
    int32_t layer_target_map, layer_target_scr, layer_target_multiple;

public:

    zmap();
    ~zmap();
	void clear();
	void force_refr_pointer();
    bool CanUndo();
    bool CanRedo();
    bool CanPaste();
    int32_t  CopyScr();
    int32_t  getCopyFFC();
    void setCopyFFC(int32_t n);
    set_ffc_command::data_t getCopyFFCData();

    // Undo/Redo
    void StartListCommand();
    void FinishListCommand();
    void RevokeListCommand();
	bool InListCommand() const;
    void ExecuteCommand(std::shared_ptr<user_input_command> command, bool skip_execute = false);
    void UndoCommand();
    void RedoCommand();
    void ClearCommandHistory();
    void CapCommandHistory();
    void DoSetComboCommand(int map, int scr, int pos, int combo, int cset);
    void DoSetFFCCommand(int map, int scr, int i, set_ffc_command::data_t data);
    void DoSetFlagCommand(int map, int scr, int pos, int flag);
    void DoPutDoorCommand(int side, int door, bool force = false);
    void DoSetDoorCommand(int scr, int side, int door);
    void DoSetDCSCommand(int dcs);
    void DoPasteScreenCommand(PasteCommandType type, int data = 0);
    void DoClearScreenCommand();
    void DoTemplateCommand(int floorcombo, int floorcset, int scr);

    void Copy();
    void CopyFFC(int32_t n);
    void Paste(const mapscr& copymapscr);
    void PasteAll(const mapscr& copymapscr);
    void PasteToAll(const mapscr& copymapscr);
    void PasteAllToAll(const mapscr& copymapscr);
    void PasteUnderCombo(const mapscr& copymapscr);
    void PasteSecretCombos(const mapscr& copymapscr);
    void PasteFFCombos(mapscr& copymapscr);
    void PasteWarps(const mapscr& copymapscr);
    void PasteScreenData(const mapscr& copymapscr);
    void PasteWarpLocations(const mapscr& copymapscr);
    void PasteDoors(const mapscr& copymapscr);
    void PasteLayers(const mapscr& copymapscr);
    void PasteRoom(const mapscr& copymapscr);
    void PasteGuy(const mapscr& copymapscr);
    void PastePalette(const mapscr& copymapscr);
    void PasteEnemies(const mapscr& copymapscr);
    void update_combo_cycling();
    void update_freeform_combos();
    int32_t getMapCount();
    bool isDungeon(int32_t scr);
    bool isstepable(int32_t combo);
    bool ishookshottable(int32_t bx, int32_t by, int32_t i);
    bool ishookshottable(int32_t map, int32_t screen, int32_t bx, int32_t by, int32_t i);
    int32_t warpindex(int32_t combo);
    bool clearall(bool validate);
    bool reset_templates(bool validate);
    bool clearmap(bool newquest);
    void clearscr(int32_t scr);
    int32_t  load(const char *path);
    int32_t  save(const char *path);
    int32_t MAPCOMBO3(int32_t map, int32_t screen, int32_t layer, int32_t x,int32_t y);
    int32_t MAPCOMBO3(int32_t map, int32_t screen, int32_t layer, int32_t pos);
    int32_t MAPCOMBO2(int32_t lyr,int32_t x,int32_t y, int32_t map = -1, int32_t scr = -1);
    int32_t MAPCOMBO(int32_t x,int32_t y, int32_t map = -1, int32_t scr = -1);
    int32_t MAPFLAG3(int32_t map, int32_t screen, int32_t layer, int32_t x,int32_t y);
    int32_t MAPFLAG3(int32_t map, int32_t screen, int32_t layer, int32_t pos);
    int32_t MAPFLAG2(int32_t lyr,int32_t x,int32_t y, int32_t map = -1, int32_t scr = -1);
    int32_t MAPFLAG(int32_t x,int32_t y, int32_t map = -1, int32_t scr = -1);
    void put_walkflags_layered(BITMAP *dest,int32_t x,int32_t y,int32_t pos,int32_t layer);
    void put_walkflags_layered_external(BITMAP *dest,int32_t x,int32_t y,int32_t pos,int32_t layer, int32_t map, int32_t screen);
    bool misaligned(int32_t map, int32_t scr, int32_t i, int32_t dir);
    void check_alignments(BITMAP* dest,int32_t x,int32_t y,int32_t scr=-1);
	void draw_darkness(BITMAP* dest, BITMAP* transdest);
    void draw(BITMAP *dest,int32_t x,int32_t y,int32_t flags,int32_t map,int32_t scr,int32_t hl_layer);
    void drawrow(BITMAP *dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr);
    void drawcolumn(BITMAP *dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr);
    void drawblock(BITMAP* dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr);
    void drawstaticblock(BITMAP* dest,int32_t x,int32_t y);
    void drawstaticrow(BITMAP* dest,int32_t x,int32_t y);
    void drawstaticcolumn(BITMAP* dest,int32_t x,int32_t y);
    void draw_template(BITMAP *dest,int32_t x,int32_t y);
    void draw_template2(BITMAP *dest,int32_t x,int32_t y);
    void draw_secret(BITMAP *dest, int32_t pos);
    void draw_secret2(BITMAP *dest, int32_t pos);
    void scroll(int32_t dir, bool warp);
    mapscr *CurrScr();
    mapscr *Scr(int32_t scr);
    mapscr *AbsoluteScr(int32_t scr);
    mapscr *AbsoluteScr(int32_t map, int32_t scr);
    int32_t  getCurrMap();
    bool isDark();
    void setCurrentView(int32_t map, int32_t scr);
    void setCurrMap(int32_t index);
    int32_t  getCurrScr();
    void setCurrScr(int32_t scr);
    void setlayertarget();
    void setcolor(int32_t c);
    int32_t getcolor();
    void resetflags();
    word tcmbdat(int32_t pos);
    word tcmbcset(int32_t pos);
    int32_t tcmbflag(int32_t pos);
    word tcmbdat2(int32_t pos);
    word tcmbcset2(int32_t pos);
    int32_t tcmbflag2(int32_t pos);
    void put_door(BITMAP *dest,int32_t pos,int32_t side,int32_t type,int32_t xofs,int32_t yofs,bool ignorepos, int32_t scr);
    void over_door(BITMAP *dest,int32_t pos,int32_t side,int32_t xofs,int32_t yofs,bool ignorepos, int32_t scr);
    void TemplateAll();
    void Template(int32_t floorcombo, int32_t floorcset, int32_t scr);
    void putdoor(int32_t scr,int32_t side,int32_t door);
    
	void goto_dmapscr(int dmap, int scr);
	void goto_mapscr(int map, int scr);
	
	void dowarp(int32_t type, int32_t index);
    void prv_dowarp(int32_t type, int32_t index);
    void dowarp2(int32_t ring,int32_t index);
	
	void set_warpback();
	bool has_warpback();
	void warpback();
	
    void set_prvscr(int32_t map, int32_t scr);
    mapscr* get_prvscr();
    void prv_secrets(bool);
    int32_t get_prv_map()
    {
        return prv_map;
    }
    int32_t get_prv_scr()
    {
        return prv_scr;
    }
    void set_prvcmb(int32_t set)
    {
        prv_cmbcycle=set;
    }
    int32_t get_prvcmb()
    {
        return prv_cmbcycle;
    }
    void set_prvfreeze(int32_t set)
    {
        prv_freeze=set;
    }
    int32_t get_prvfreeze()
    {
        return prv_freeze;
    }
    void set_prvadvance(int32_t set)
    {
        prv_advance=set;
    }
    int32_t get_prvadvance()
    {
        return prv_advance;
    }
    void set_prvtime(int32_t set)
    {
        prv_time=set;
    }
    int32_t get_prvtime()
    {
        return prv_time;
    }
    int32_t getLayerTargetMap();
    int32_t getLayerTargetScr();
    int32_t getLayerTargetMultiple();
};

extern zmap Map;

//bool read_old_mapfile(mapscr *scr, FILE *f);
extern const char *loaderror[];

void put_walkflags(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t layer);
void put_flags(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag);
void put_flag(BITMAP* dest, int32_t x, int32_t y, int32_t flag);
void put_combo(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag,int32_t scale);
void put_combo(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag);
void put_engraving(BITMAP* dest, int32_t x, int32_t y, int32_t slot, int32_t scale);
void copy_mapscr(mapscr *dest, const mapscr *src);
void delete_mapscr(mapscr *dest);

/******************************/
/******** ZQuest stuff ********/
/******************************/

//word msg_count=0;
//const char zqsheader[30];

bool setMapCount2(int32_t c);
int32_t init_quest();
void set_questpwd(std::string_view pwd, bool use_keyfile);
int32_t quest_access(const char *filename, zquestheader *hdr);
bool write_midi(MIDI *m,PACKFILE *f);
int32_t load_quest(const char *filename, bool show_progress = true);
int32_t load_tileset(const char *filename, dword tsetflags);
int32_t save_unencoded_quest(const char *filename, bool compressed, const char* afname = NULL);
int32_t save_quest(const char *filename, bool timed_save);

int32_t writemapscreen(PACKFILE *f, int32_t i, int32_t j);

bool load_msgstrs(const char *path, int32_t startstring);
bool save_msgstrs(const char *path);
bool save_msgstrs_text(const char *path);
int32_t writestrings(PACKFILE *f, word version, word build, word start_msgstr, word max_msgstrs);
int32_t writestrings_text(PACKFILE *f);

bool save_strings_tsv(const char *path);
bool load_strings_tsv(const char *path);
void parse_strings_tsv(std::string tsv);
int32_t writestrings_tsv(PACKFILE *f);

bool load_pals(const char *path, int32_t startcset);
bool save_pals(const char *path);
int32_t writecolordata(PACKFILE *f, word version, word build, word start_cset, word max_csets);

bool load_combos(const char *path, int32_t startcombo);
bool save_combos(const char *path);
int32_t writecombos(PACKFILE *f, word version, word build, word start_combo, word max_combos);
int32_t writecomboaliases(PACKFILE *f, word version, word build);

bool load_tiles(const char *path, int32_t starttile);
bool save_tiles(const char *path);
int32_t writetiles(PACKFILE *f, word version, word build, int32_t start_tile, int32_t max_tiles);

bool load_dmaps(const char *path, int32_t startdmap);
bool save_dmaps(const char *path);
int32_t writedmaps(PACKFILE *f, word version, word build, word start_dmap, word max_dmaps);

bool save_guys(const char *path);
bool load_guys(const char *path);

bool load_zgp(const char *path);
bool save_zgp(const char *path);

bool save_subscreen(const char *path, ZCSubscreen const& savefrom);
bool load_subscreen(const char *path, ZCSubscreen& loadto);

bool load_zqt(const char *path);
bool save_zqt(const char *path);

void center_zq_class_dialogs();

int32_t writeitems(PACKFILE *f, zquestheader *Header);
int32_t writeweapons(PACKFILE *f, zquestheader *Header);
int32_t writemisccolors(PACKFILE *f, zquestheader *Header);
int32_t writegameicons(PACKFILE *f, zquestheader *Header);
int32_t writedoorcombosets(PACKFILE *f, zquestheader *Header);
int32_t write_one_subscreen(PACKFILE *f, zquestheader *Header, int32_t i);
int32_t writeffscript(PACKFILE *f, zquestheader *Header);
int32_t write_quest_zasm(PACKFILE *f);
int32_t write_one_ffscript(PACKFILE *f, zquestheader *Header, int32_t i, script_data **script);
int32_t writeitemdropsets(PACKFILE *f, zquestheader *Header);
int32_t writefavorites(PACKFILE *f, zquestheader *Header);
#endif
