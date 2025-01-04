#include "allegro/gui.h"
#include "base/files.h"
#define MIDI_TRACK_BUFFER_SIZE 50

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <vector>
#include <filesystem>
#include <base/new_menu.h>

#include "dialog/info_lister.h"
#ifdef __APPLE__
// malloc.h is deprecated, but malloc also lives in stdlib
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "zalleg/zalleg.h"
#include "base/qrs.h"
#include "base/dmap.h"
#include "base/msgstr.h"
#include "base/packfile.h"
#include "base/cpool.h"
#include "base/autocombo.h"
#include "base/render.h"
#include "base/version.h"
#include "zq/autocombo/autopattern_base.h"
#include "zq/autocombo/pattern_basic.h"
#include "zq/autocombo/pattern_flatmtn.h"
#include "zq/autocombo/pattern_fence.h"
#include "zq/autocombo/pattern_cakemtn.h"
#include "zq/autocombo/pattern_relational.h"
#include "zq/autocombo/pattern_dungeoncarve.h"
#include "zq/autocombo/pattern_dormtn.h"
#include "zq/autocombo/pattern_tiling.h"
#include "zq/autocombo/pattern_replace.h"
#include "zq/autocombo/pattern_denseforest.h"
#include "zq/autocombo/pattern_extend.h"
#include "zq/render_hotkeys.h"
#include "zq/render_minimap.h"
#include "zq/render_tooltip.h"
#include "base/misctypes.h"
#include "parser/Compiler.h"
#include "base/zc_alleg.h"
#include "particles.h"
#include "dialog/combopool.h"
#include "dialog/alert.h"
#include "dialog/alertfunc.h"
#include "zq/gui/edit_autocombo.h"

particle_list particles;
void setZScriptVersion(int32_t) { } //bleh...

#include <al5_img.h>
#include <loadpng.h>
#include <fmt/format.h>

#include "dialog/cheat_codes.h"
#include "dialog/set_password.h"
#include "dialog/foodlg.h"
#include "dialog/quest_rules.h"
#include "dialog/script_rules.h"
#include "dialog/headerdlg.h"
#include "dialog/ffc_editor.h"
#include "dialog/screen_data.h"
#include "dialog/edit_dmap.h"
#include "dialog/compilezscript.h"
#include "dialog/screen_enemies.h"
#include "dialog/enemypattern.h"
#include "dialog/sfxdata.h"
#include "dialog/mapstyles.h"
#include "dialog/externs.h"

#include "base/gui.h"
#include "gui/jwin_a5.h"
#include "gui/jwin.h"
#include "zc_list_data.h"
#include "gui/editbox.h"
#include "zq/zq_misc.h"
#include "zq/zq_tiles.h"                                       // tile and combo code

#include "zq/zquest.h"
#include "zq/ffasm.h"
#include "zq/render.h"

// the following are used by both zelda.cc and zquest.cc
#include "base/zdefs.h"
#include "base/qrs.h"
#include "tiles.h"
#include "base/colors.h"
#include "base/qst.h"
#include "base/zsys.h"
#include "base/zapp.h"
#include "play_midi.h"
#include "sound/zcmusic.h"

#include "midi.h"
#include "sprite.h"
#include "fontsdat.h"
#include "base/jwinfsel.h"
#include "zq/zq_class.h"
#include "subscr.h"
#include "zq/zq_subscr.h"
#include "zc/ffscript.h"
#include "gui/EditboxNew.h"
#include "sfx.h"
#include "zq/zq_custom.h" // custom items and guys
#include "zq/zq_strings.h"
#include "zq/questReport.h"
#include "zq/ffasmexport.h"
#include <fstream>
#include "base/module.h"
#include "drawing.h"
#include "zconsole/ConsoleLogger.h"
#include "colorname.h"
#include "zq/zq_hotkey.h"
#include "zq/package.h"
#include "zq/zq_files.h"
#include "music_playback.h"

extern CConsoleLoggerEx parser_console;

namespace fs = std::filesystem;

//Windows mmemory tools
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib") // Needed to avoid linker issues. -Z
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

//SDL_Surface *sdl_screen;

#if defined(ALLEGRO_WINDOWS)
static const char *data_path_name   = "win_data_path";
static const char *midi_path_name   = "win_midi_path";
static const char *image_path_name  = "win_image_path";
static const char *tmusic_path_name = "win_tmusic_path";
static const char *last_quest_name  = "win_last_quest";
static const char *qtname_name      = "win_qtname%d";
static const char *qtpath_name      = "win_qtpath%d";
#elif defined(ALLEGRO_LINUX)
static const char *data_path_name   = "linux_data_path";
static const char *midi_path_name   = "linux_midi_path";
static const char *image_path_name  = "linux_image_path";
static const char *tmusic_path_name = "linux_tmusic_path";
static const char *last_quest_name  = "linux_last_quest";
static const char *qtname_name      = "linux_qtname%d";
static const char *qtpath_name      = "linux_qtpath%d";
#elif defined(__APPLE__)
static const char *data_path_name   = "macosx_data_path";
static const char *midi_path_name   = "macosx_midi_path";
static const char *image_path_name  = "macosx_image_path";
static const char *tmusic_path_name = "macosx_tmusic_path";
static const char *last_quest_name  = "macosx_last_quest";
static const char *qtname_name      = "macosx_qtname%d";
static const char *qtpath_name      = "macosx_qtpath%d";
#endif

#include "base/win32.h"

#include "zq/zq_init.h"
#include "zq/zq_doors.h"
#include "zq/zq_cset.h"
#include "zinfo.h"

#ifdef _MSC_VER
#include <crtdbg.h>

#endif

// MSVC fix
#if _MSC_VER >= 1900
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif

extern byte monochrome_console;

#include "zconsole/ConsoleLogger.h"

extern CConsoleLoggerEx zscript_coloured_console;

uint8_t console_is_open = 0;
uint8_t __isZQuest = 1; //Shared functionscan reference this. -Z
bool is_zq_replay_test = false;

#include "base/util.h"

#ifdef __EMSCRIPTEN__
#include "base/emscripten_utils.h"
#endif

using namespace util;

using std::vector;
using std::map;
using std::stringstream;

FFScript FFCore;

void load_size_poses();
void do_previewtext();
bool do_slots(map<string, disassembled_script_data> &scripts, int assign_mode);
void do_script_disassembly(map<string, disassembled_script_data>& scripts, bool fromCompile);

int32_t startdmapxy[6] = {-1000, -1000, -1000, -1000, -1000, -1000};
bool cancelgetnum=false;

int32_t tooltip_timer=0, tooltip_maxtimer=30, tooltip_current_ffc=0;
int32_t combobrushoverride=-1;
ComboPosition mouse_combo_pos;

int32_t original_playing_field_offset=0;
int32_t playing_field_offset=original_playing_field_offset;
int32_t passive_subscreen_height=56;
int32_t passive_subscreen_offset=0;

bool disable_saving=false, OverwriteProtection;
bool halt=false;
bool show_sprites=true;
bool show_hitboxes = false;
bool zq_ignore_item_ownership = true;

// Used to find FFC script names
vector<string> asffcscripts;
vector<string> asglobalscripts;
vector<string> asitemscripts;
vector<string> asnpcscripts;
vector<string> aseweaponscripts;
vector<string> aslweaponscripts;
vector<string> asplayerscripts;
vector<string> asdmapscripts;
vector<string> asscreenscripts;
vector<string> asitemspritescripts;
vector<string> ascomboscripts;
vector<string> asgenericscripts;
vector<string> assubscreenscripts;

vector<string> ZQincludePaths;

int32_t CSET_SIZE = 16;
int32_t CSET_SHFT = 4;
//editbox_data temp_eb_data;
/*
  #define CSET(x)         ((x)<<CSET_SHFT)
  #define csBOSS          14
  */

/*
  enum { m_block, m_coords, m_flags, m_guy, m_warp, m_misc, m_layers,
  m_menucount };
  */
void update_combo_cycling();
void update_freeform_combos();

/*
  #define MAXMICE 14
  #define MAXARROWS 8
  #define SHADOW_DEPTH 2
  */
int32_t coord_timer=0, coord_frame=0;
int32_t blackout_color, zq_screen_w, zq_screen_h;
int32_t draw_mode=0;

size_and_pos minimap;
size_and_pos real_minimap;

size_and_pos minimap_zoomed;
size_and_pos real_minimap_zoomed;

size_and_pos map_page_bar[9];
int32_t mappage_count = 9;

size_and_pos combolist[MAX_COMBO_COLS];
size_and_pos combolistscrollers[MAX_COMBO_COLS];
int32_t num_combo_cols = MAX_COMBO_COLS;

static bool zoom_in_btn_disabled;
static bool zoom_out_btn_disabled;
size_and_pos zoominbtn;
size_and_pos zoomoutbtn;
size_and_pos compactbtn;
size_and_pos mainbar;

size_and_pos screrrorpos;

size_and_pos comboaliaslist[MAX_COMBO_COLS];
size_and_pos comboalias_preview;
size_and_pos combopool_preview;
size_and_pos combopool_prevbtn;

size_and_pos combo_merge_btn;

size_and_pos combo_preview;
size_and_pos combo_preview2;
size_and_pos combo_preview_text1;
size_and_pos combo_preview_text2;
size_and_pos combolist_window;
size_and_pos drawmode_btn;
size_and_pos main_panel;
size_and_pos squares_panel;
size_and_pos preview_panel;
size_and_pos layer_panel;
size_and_pos preview_text;

size_and_pos favorites_window;
size_and_pos favorites_list;
size_and_pos favorites_x;
size_and_pos favorites_infobtn;
size_and_pos favorites_zoombtn;
size_and_pos favorites_pgleft;
size_and_pos favorites_pgright;

size_and_pos commands_window;
size_and_pos commands_list;
size_and_pos commands_x;
size_and_pos commands_infobtn;
size_and_pos commands_zoombtn;
size_and_pos commands_txt;

size_and_pos squarepanel_swap_btn;
size_and_pos squarepanel_up_btn;
size_and_pos squarepanel_down_btn;
size_and_pos itemsqr_pos;
size_and_pos flagsqr_pos;
size_and_pos stairsqr_pos;
size_and_pos warparrival_pos;
size_and_pos warpret_pos[4];
size_and_pos enemy_prev_pos;

size_and_pos txtoffs_single;
size_and_pos txtoffs_double_1;
size_and_pos txtoffs_double_2;
int32_t panel_align = 1;

int32_t command_buttonwidth = 88;
int32_t command_buttonheight = 19;

int32_t layerpanel_buttonwidth = 58;
int32_t layerpanel_buttonheight = 16;

int32_t layerpanel_checkbox_hei = 13;
int32_t layerpanel_checkbox_wid = 13;

int32_t favorite_combos[MAXFAVORITECOMBOS];
byte favorite_combo_modes[MAXFAVORITECOMBOS];
bool ShowFavoriteComboModes;
byte FavoriteComboPage;

char comboprev_buf[512] = {0};
char comboprev_buf2[512] = {0};
FONT* txfont;

const char *roomtype_string[MAXROOMTYPES] =
{
	"(None)","Special Item","Pay for Info","Secret Money","Gamble",
	"Door Repair","Red Potion or Heart Container","Feed the Goriya","Triforce Check",
	"Potion Shop","Shop","More Bombs","Leave Money or Life","10 Rupees",
	"3-Stair Warp","Ganon","Zelda", "-<item pond>", "1/2 Magic Upgrade", "Learn Slash", "More Arrows","Take One Item"
};

const char *catchall_string[MAXROOMTYPES] =
{
	"Generic Catchall","Special Item","Info Type","Amount","Generic Catchall","Repair Fee","Generic Catchall","Generic Catchall","Generic Catchall","Shop Type",
	"Shop Type","Price","Price","Generic Catchall","Warp Ring","Generic Catchall","Generic Catchall", "Generic Catchall", "Generic Catchall",
	"Generic Catchall", "Price","Shop Type","Bottle Shop Type"
};

#define MAXPOOLCOMBOS MAXFAVORITECOMBOS

struct cmbdat_pair
{
    int32_t data;
    byte cset;
    cmbdat_pair() { clear(); }
    void clear()
    {
        data = -1;
        cset = 0;
    }
    bool valid() const
    {
        return data > -1;
    }
};
bool pool_dirty=true;
cmbdat_pair pool_combos[MAXPOOLCOMBOS];
static std::vector<byte> pool;

bool pool_valid()
{
	if(pool_dirty)
	{
		pool.clear();
		for(auto q = 0; q < MAXPOOLCOMBOS; ++q)
		{
			if(pool_combos[q].valid())
				pool.push_back(q);
		}
		pool_dirty = false;
	}
	return pool.size() > 0;
}
cmbdat_pair const& get_pool_combo()
{
	if(!pool_valid()) return pool_combos[0];
	auto ind = zc_rand(pool.size()-1);
	return pool_combos[pool.at(ind)];
}

int32_t mouse_scroll_h;

// 'mapscreen' refers to the area of the editor where the screen is drawn.
int32_t mapscreen_x, mapscreen_y, showedges, showallpanels;
// The scale of the entire mapscreen area. This varies based on compact/extended mode.
static int mapscreen_screenunit_scale;
// Would love to have no limit, but our screen bitmap has too low a resolution at the moment.
// The scale of an individual screen being drawn. This is `mapscreen_screenunit_scale / Map.getViewSize()`.
static double mapscreen_single_scale;
// 4 is roughly the largest value where things render okay. Beyond that, our low bitmap resolution results in tons
// of downsampling. Let users go to 16 anyway.
static int mapscreen_num_screens_to_draw_max = 16;
// The valid layers for the current screen(s).
static bool mapscreen_valid_layers[6];

struct VisibleScreen
{
	int dx, dy;
	int xoff, yoff;
	mapscr* scr;
	int screen;
};
static std::vector<VisibleScreen> visible_screens;
static VisibleScreen* active_visible_screen = nullptr;

static void set_active_visible_screen(mapscr* scr)
{
	active_visible_screen = nullptr;
	for (auto& visible_screen : visible_screens)
	{
		if (visible_screen.scr == scr)
		{
			active_visible_screen = &visible_screen;
			break;
		}
	}
}

static ComboPosition get_mapscreen_mouse_combo_pos()
{
	int startxint = mapscreen_x+(showedges?int(16*mapscreen_single_scale):0);
	int startyint = mapscreen_y+(showedges?int(16*mapscreen_single_scale):0);
	int cx = (gui_mouse_x()-startxint)/(16*mapscreen_single_scale);
	int cy = (gui_mouse_y()-startyint)/(16*mapscreen_single_scale);
	return ComboPosition{cx, cy};
}

static void refresh_visible_screens()
{
	int num_screens = Map.getViewSize();
	int screen_width = mapscreenbmp->w * mapscreen_single_scale;
	int screen_height = mapscreenbmp->h * mapscreen_single_scale;

	visible_screens.clear();
	for (int dx = 0; dx < num_screens; dx++)
	{
		for (int dy = 0; dy < num_screens; dy++)
		{
			int mx = Map.getViewScr()%16 + dx;
			int my = Map.getViewScr()/16 + dy;
			if (mx < 0 || mx >= 16 || my < 0 || my >= 9)
				continue;

			int screen = Map.getViewScr() + dx + dy * 16;
			if (screen >= MAPSCRS)
				continue;

			mapscr* scr = Map.Scr(screen);
			int offx = dx * screen_width;
			int offy = dy * screen_height;
			visible_screens.emplace_back(VisibleScreen{dx, dy, offx, offy, scr, screen});
		}
	}

	for (int i = 0; i < 6; i++)
	{
		mapscreen_valid_layers[i] = false;
		for (auto& vis_screen : visible_screens)
		{
			mapscreen_valid_layers[i] |= vis_screen.scr->layermap[i] > 0;
		}
	}
}

int32_t readsize, writesize;
bool fake_pack_writing=false;

int32_t showxypos_x;
int32_t showxypos_y;
int32_t showxypos_w;
int32_t showxypos_h;
int32_t showxypos_color;
int32_t showxypos_ffc=-1000;
bool showxypos_icon=false;

int32_t showxypos_cursor_x;
int32_t showxypos_cursor_y;
bool showxypos_cursor_icon=false;
int32_t showxypos_cursor_color;
bool showxypos_dummy = false;

bool canfill=true;                                          //to prevent double-filling (which stops undos)
int32_t lens_hint_item[MAXITEMS][2];                            //aclk, aframe
int32_t lens_hint_weapon[MAXWPNS][5];                           //aclk, aframe, dir, x, y
//int32_t mode, switch_mode, orig_mode;
int32_t tempmode=GFX_AUTODETECT;
RGB_MAP zq_rgb_table;
COLOR_MAP trans_table, trans_table2;
MIDI *song=NULL;
BITMAP *menu1, *menu3, *mapscreenbmp, *tmp_scr, *screen2, *mouse_bmp[MOUSE_BMP_MAX][4], *mouse_bmp_1x[MOUSE_BMP_MAX][4], *icon_bmp[ICON_BMP_MAX][4], *flag_bmp[16][4], *select_bmp[2], *dmapbmp_small, *dmapbmp_large;
BITMAP *arrow_bmp[MAXARROWS],*brushbmp, *brushscreen; //*brushshadowbmp;
byte *colordata=NULL, *trashbuf=NULL;
itemdata *itemsbuf;
wpndata  *wpnsbuf;
comboclass *combo_class_buf;
guydata  *guysbuf;
item_drop_object    item_drop_sets[MAXITEMDROPSETS];
newcombo curr_combo;
PALETTE RAMpal;
midi_info Midi_Info;
bool zq_showpal=false;
bool is_compact = false;

int pixeldb = 1;
int infobg = 1;
bool large_merged_combopane = false;
bool compact_merged_combopane = true;
bool large_zoomed_fav = false;
bool compact_zoomed_fav = true;
bool large_zoomed_cmd = false;
bool compact_zoomed_cmd = true;

bool compact_square_panels = false;
int compact_active_panel = 0;

int combo_col_scale = 1;

std::vector<std::shared_ptr<zasm_script>> zasm_scripts;
script_data *ffscripts[NUMSCRIPTFFC];
script_data *itemscripts[NUMSCRIPTITEM];
script_data *guyscripts[NUMSCRIPTGUYS];
script_data *lwpnscripts[NUMSCRIPTWEAPONS];
script_data *ewpnscripts[NUMSCRIPTWEAPONS];
script_data *globalscripts[NUMSCRIPTGLOBAL];
script_data *genericscripts[NUMSCRIPTSGENERIC];
script_data *playerscripts[NUMSCRIPTHERO];
script_data *screenscripts[NUMSCRIPTSCREEN];
script_data *dmapscripts[NUMSCRIPTSDMAP];
script_data *itemspritescripts[NUMSCRIPTSITEMSPRITE];
script_data *comboscripts[NUMSCRIPTSCOMBODATA];
script_data *subscreenscripts[NUMSCRIPTSSUBSCREEN];

extern string zScript;
char zScriptBytes[512];
char zLastVer[512] = { 0 };
SAMPLE customsfxdata[WAV_COUNT];
uint8_t customsfxflag[WAV_COUNT>>3];
int32_t sfxdat=1;

int32_t onImport_ComboAlias();
int32_t onExport_ComboAlias();

void set_console_state();

void clearConsole()
{
	if(!console_is_open) return;
	zscript_coloured_console.cls(CConsoleLoggerEx::COLOR_BACKGROUND_BLACK);
	zscript_coloured_console.gotoxy(0,0);
	zscript_coloured_console.cprintf( CConsoleLoggerEx::COLOR_BLUE | CConsoleLoggerEx::COLOR_INTENSITY |
		CConsoleLoggerEx::COLOR_BACKGROUND_BLACK,"ZQuest Classic Logging Console\n");
}

void initConsole()
{
	if(console_is_open) return;
	console_is_open = 1;
	set_console_state();
	zscript_coloured_console.Create("ZQuest Classic Logging Console", 600, 200);
	clearConsole();
}

void killConsole()
{
	if(!console_is_open) return;
	console_is_open = 0;
	set_console_state();
	zscript_coloured_console.kill();
}

int toggleConsole()
{
	console_is_open ? killConsole() : initConsole();
	zc_set_config("zquest","open_debug_console",console_is_open?1:0);
	return D_O_K;
}

int showHotkeys()
{
	hotkeys_toggle_display(!hotkeys_is_active());
	return D_O_K;
}

typedef int32_t (*intF)();
typedef struct command_pair
{
    char name[80];
    int32_t flags;
    intF command;
} command_pair;

extern map_and_screen map_page[MAX_MAPPAGE_BTNS];

int32_t do_OpenQuest()
{
	return onOpen();
}

int32_t do_NewQuest()
{
	//clear the panel recent screen buttons to prevent crashes from invalid maps
	for ( int32_t q = 0; q < 9; q++ )
	{
		map_page[q].map = 0;
		map_page[q].screen = 0;
	}
	Map.setCurrMap(0);
	Map.setCurrScr(0);
	return onNew();
}

extern int CheckerCol1, CheckerCol2;
int32_t alignment_arrow_timer=0;
int32_t  Flip=0,Combo=0,CSet=2,current_combolist=0,current_comboalist=0,current_cpoollist=0,current_cautolist=0,current_mappage=0;
int32_t  Flags=0,Flag=0,menutype=(m_block);
int MouseScroll = 0, SavePaths = 0, CycleOn = 0, ShowGrid = 0, ShowScreenGrid = 0, GridColor = 15, ShowCurScreenOutline = 1,
	CmbCursorCol = 15, TilePgCursorCol = 15, CmbPgCursorCol = 15, TTipHLCol = 13,
	TileProtection = 0, ComboProtection = 0, NoScreenPreview = 0, MMapCursorStyle = 0,
	LayerDitherBG = -1, LayerDitherSz = 2, RulesetDialog = 0,
	EnableTooltips = 0, TooltipsHighlight = 0, ShowFFScripts = 0, ShowSquares = 0,
	ShowFFCs = 0, ShowInfo = 0, skipLayerWarning = 0, WarnOnInitChanged = 0,
	DisableLPalShortcuts = 1, DisableCompileConsole = 0, numericalFlags = 0,
	ActiveLayerHighlight = 0, DragCenterOfSquares = 0;
uint8_t InvalidBG = 0;
bool NoHighlightLayer0 = false;
int32_t FlashWarpSquare = -1, FlashWarpClk = 0; // flash the destination warp return when ShowSquares is active
uint8_t ViewLayer3BG = 0, ViewLayer2BG = 0;
int32_t window_width, window_height;
bool ShowFPS = false, SaveDragResize = false, DragAspect = false, SaveWinPos=false;
bool allowHideMouse = false;
double aspect_ratio = LARGE_H / double(LARGE_W);
int window_min_width = 0, window_min_height = 0;
int32_t ComboBrush = 0;                                             //show the brush instead of the normal mouse
int32_t ComboBrushPause = 0;                                        //temporarily disable the combo brush
int32_t FloatBrush = 0;                                             //makes the combo brush float a few pixels up and left
int AutoBrush = 0; //Drag to size the brush on the combo panes
bool AutoBrushRevert = false; //Revert after placing
int LinkedScroll = 0;
//complete with shadow
int32_t OpenLastQuest = 0;                                          //makes the program reopen the quest that was
//open at the time you quit
int32_t ShowMisalignments = 0;                                      //makes the program display arrows over combos that are
//not aligned with the next screen.
int32_t AnimationOn = 0;                                            //animate the combos in zquest?
int32_t AutoBackupRetention = 0;                                    //use auto-backup feature?  if so, how many backups (1-10) to keep
int32_t AutoSaveInterval = 0;                                       //how often a timed autosave is made (not overwriting the current file)
int32_t UncompressedAutoSaves = 0;                                  //should timed saves be uncompressed/encrypted?
int32_t KeyboardRepeatDelay = 0;                                    //the time in milliseconds after holding down a key that the key starts to repeat
int32_t KeyboardRepeatRate = 0;                                     //the time in milliseconds between each repetition of a repeated key

time_t auto_save_time_start, auto_save_time_current;
double auto_save_time_diff = 0;
int32_t AutoSaveRetention = 0;                                      //how many autosaves of a quest to keep
int32_t ImportMapBias = 0;                                          //tells what has precedence on map importing
int32_t BrushWidth=1, BrushHeight=1;
bool saved=true;
bool __debug=false;
int32_t LayerMaskInt[7]={0};
int32_t CurrentLayer=0;
int32_t DuplicateAction[4]={0};
int32_t OnlyCheckNewTilesForDuplicates = 0;
int32_t try_recovering_missing_scripts = 0;

uint8_t PreFillTileEditorPage = 0, PreFillComboEditorPage = 0;
int32_t DMapEditorLastMaptileUsed = 0;

/*
  , HorizontalDuplicateAction;
  int32_t VerticalDuplicateAction, BothDuplicateAction;
  */
word msg_count = 0;
int32_t LeechUpdate = 0;
int32_t LeechUpdateTiles = 0;
int32_t SnapshotFormat = 0;
byte SnapshotScale = 0;

byte Color = 0;
extern int32_t jwin_pal[jcMAX];
int32_t gui_colorset=99;

static int32_t combo_apos=0; //currently selected combo alias
int32_t alias_origin=0;
int32_t alias_cset_mod=0;

static int32_t combo_pool_pos=0; //currently selected combo pool
bool weighted_cpool = true;
bool cpool_prev_visible = false;

static int32_t combo_auto_pos=0; //currently selected autocombo
byte cauto_height = 1;

bool trip=false;

int32_t fill_type=1;

bool first_save=false;
char *filepath,*midipath,*datapath,*imagepath,*tmusicpath,*last_timed_save;
string helpstr, zstringshelpstr;

ZCMUSIC *zcmusic = NULL;
ZCMIXER *zcmixer = NULL;
int32_t midi_volume = 255;
extern int32_t prv_mode;
int32_t prv_warp = 0;
int32_t prv_twon = 0;
int32_t ff_combo = 0;

int32_t Frameskip = 0, RequestedFPS = 60, zqUseWin32Proc = 1;
int32_t zqColorDepth = 8;
int32_t joystick_index=0;

void set_last_timed_save(char const* buf)
{
	if(buf && buf[0])
	{
		if(buf != last_timed_save)
			strcpy(last_timed_save, buf);
    }
	else
	{
		last_timed_save[0] = 0;
		buf = nullptr;
	}
	zc_set_config("zquest","last_timed_save",buf);
}

void loadlvlpal(int32_t level);
bool get_debug()
{
    return __debug;
    //return true;
}

void set_debug(bool d)
{
    __debug=d;
    return;
}

bool handle_quit()
{
	if(onExit()==D_CLOSE)
		return (exiting_program = true);
	return false;
}
bool handle_close_btn_quit()
{
	if(close_button_quit)
	{
		close_button_quit=false;
		return handle_quit();
	}
	return false;
}
// **** Timers ****

volatile int32_t lastfps=0;
volatile int32_t framecnt=0;
size_t cpoolbrush_index = 0;

// quest data
zquestheader header;
byte                midi_flags[MIDIFLAGS_SIZE];
byte                music_flags[MUSICFLAGS_SIZE];
byte                *quest_file;
int32_t					msg_strings_size;
zctune              *customtunes;
//emusic            *enhancedMusic;
ZCHEATS             zcheats;
byte                use_cheats;
byte                use_tiles;
// Note: may not be null-terminated (must refactor writecolordata to fix).
char                palnames[MAXLEVELS][17];
char                zquestdat_sig[52];
char		    qstdat_str[2048];

int32_t gme_track=0;

int32_t dlevel; // just here until gamedata is properly done

bool bad_version(int32_t ver)
{
    if(ver < 0x170)
        return true;
        
    return false;
}

// These are for drawing eyeballs correctly in combo_tile.
zfix HeroModifiedX()
{
	return gui_mouse_x() - 7;
}
zfix HeroModifiedY()
{
	return gui_mouse_y() - 7;
}

static NewMenu import_250_menu
{
	{ "&DMaps", onImport_DMaps },
	{ "&Combo Table", onImport_Combos },
	{ "&Combo Alias", onImport_ComboAlias },
};

static NewMenu import_graphics
{
	{ "&Palettes", onImport_Pals },
	{},
	{ "Tileset (&Full)", onImport_Tiles },
	{ "&Tile Pack", onImport_Tilepack },
	{ "T&ile Pack to...", onImport_Tilepack_To },
	{},
	{ "&Combo Set (Range)", onImport_Combos },
	{ "Combo Pack (Full, 1:1)", onImport_Combopack },
	{ "Combo Pack to... (Dest)", onImport_Combopack_To },
	{},
	{ "Combo &Alias Pack", onImport_Comboaliaspack },
	{ "Combo A&lias Pack to...", onImport_Comboaliaspack_To },
	{},
	{ "&Doorsets", onImport_Doorset },
};

static NewMenu import_menu
{
	{ "&Enemies", onImport_Guys },
	{ "&Map", onImport_Map },
	{ "&DMaps", onImport_DMaps },
	{ "&Strings (.tsv)", onImport_StringsTSV },
	{ "String Table (deprecated)", onImport_Msgs },
	{},
	{ "&Graphics", &import_graphics },
	{},
	{ "2.50 (Broken)", &import_250_menu },
};

static NewMenu export_250_menu
{
	{ "&DMaps", onExport_DMaps },
	{ "&Combo Table", onExport_Combos },
	{ "&Combo Alias", onExport_ComboAlias },
	{ "&Graphics Pack", onExport_ZGP },
};

static NewMenu zq_help_menu
{
	{ "&Editor Help", onHelp },
	{ "&Strings Help", onZstringshelp },
};

static NewMenu export_graphics
{
	{ "&Palettes", onExport_Pals },
	{},
	{ "Tileset (&Full)", onExport_Tiles },
	{ "&Tile Pack", onExport_Tilepack },
	{},
	{ "&Combo Set", onExport_Combos },
	{ "Combo Pack", onExport_Combopack },
	{},
	{ "Combo &Alias Pack", onExport_Comboaliaspack },
	{},
	{ "&Doorsets", onExport_Doorset },
};

static NewMenu export_menu
{
#ifdef _WIN32
	{ "&Package", onExport_Package },
#endif
	{ "&Enemies", onExport_Guys },
	{ "&Map", onExport_Map },
	{ "&DMaps", onExport_DMaps },
	{},
	{ "&Strings (.tsv)", onExport_StringsTSV },
	{ "String Table (deprecated)", onExport_Msgs },
	{},
	{ "&Graphics", &export_graphics },
	{},
	{ "2.50 (Broken)", &export_250_menu },
};


static NewMenu recent_menu
{
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
};
static char rec_menu_fullpaths[10][512];
static char rec_menu_strs[10][64];

int32_t customOpen(char const* path);
void do_recent_quest(uint32_t ind)
{
	if(ind > 9) return;
	strcpy(temppath, rec_menu_fullpaths[ind]);
	customOpen(temppath);
}
int32_t do_RecentQuest_0() { do_recent_quest(0); return D_O_K; }
int32_t do_RecentQuest_1() { do_recent_quest(1); return D_O_K; }
int32_t do_RecentQuest_2() { do_recent_quest(2); return D_O_K; }
int32_t do_RecentQuest_3() { do_recent_quest(3); return D_O_K; }
int32_t do_RecentQuest_4() { do_recent_quest(4); return D_O_K; }
int32_t do_RecentQuest_5() { do_recent_quest(5); return D_O_K; }
int32_t do_RecentQuest_6() { do_recent_quest(6); return D_O_K; }
int32_t do_RecentQuest_7() { do_recent_quest(7); return D_O_K; }
int32_t do_RecentQuest_8() { do_recent_quest(8); return D_O_K; }
int32_t do_RecentQuest_9() { do_recent_quest(9); return D_O_K; }

void refresh_recent_menu()
{
	int32_t (*procs[10])(void) = { 
		do_RecentQuest_0, do_RecentQuest_1, do_RecentQuest_2, do_RecentQuest_3,
		do_RecentQuest_4, do_RecentQuest_5, do_RecentQuest_6,
		do_RecentQuest_7, do_RecentQuest_8, do_RecentQuest_9
	};
	static MenuItem nilitem("---",nullptr,nullopt,true);
	for(auto q = 0; q < 10; ++q)
	{
		MenuItem& mit = *recent_menu.at(q);
        bool valid = rec_menu_fullpaths[q][0] != '-';
		if(valid)
			mit = MenuItem(rec_menu_strs[q],procs[q]);
		else mit = nilitem;
	}
}

void load_recent_quests()
{
	char configname[64] = "rec_qst_";
	char* ptr = &configname[strlen(configname)];
	char buf[512] = {0};
	for(auto q = 0; q < 10; ++q)
	{
		sprintf(ptr, "%d", q); //increment the configname value
		char const* qst_str = zc_get_config("recent",configname,nullptr);
		if(qst_str[0])
		{
			strncpy(rec_menu_fullpaths[q], qst_str, 511);
			relativize_path(buf, rec_menu_fullpaths[q]);
			if(strlen(buf) > 62)
			{
				buf[60] = buf[61] = buf[62] = '.'; //add "..." as the last 3 characters
			}
			strncpy(rec_menu_strs[q], buf, 63);
		}
		else
		{
			strcpy(rec_menu_fullpaths[q], "---");
			strcpy(rec_menu_strs[q], "---");
		}
		rec_menu_fullpaths[q][511] = 0;
		rec_menu_strs[q][63] = 0;
	}
	refresh_recent_menu();
}

void write_recent_quests()
{
	char configname[64] = "rec_qst_";
	char* ptr = &configname[strlen(configname)];
	for(auto q = 0; q < 10; ++q)
	{
		sprintf(ptr, "%d", q); //increment the configname value
		zc_set_config("recent",configname,(rec_menu_fullpaths[q][0]!='-') ? rec_menu_fullpaths[q] : nullptr);
	}
}

void update_recent_quest(char const* path)
{
	int32_t ind = -1;
	for(auto q = 0; q < 10; ++q)
	{
		if(!strcmp(path, rec_menu_fullpaths[q]))
		{
			ind = q;
			break;
		}
	}
	if(ind > -1)
	{
		for(auto q = ind; q > 0; --q)
		{
			strcpy(rec_menu_fullpaths[q], rec_menu_fullpaths[q-1]);
			strcpy(rec_menu_strs[q], rec_menu_strs[q-1]);
		}
	}
	else
	{
		int32_t free_ind = 9; //if none found, override the last index
		for(auto q = 0; q < 9; ++q)
		{
			if(rec_menu_fullpaths[q][0] == '-')
			{
				free_ind = q;
				break;
			}
		}
		
		for(auto q = free_ind; q > 0; --q)
		{
			strcpy(rec_menu_fullpaths[q], rec_menu_fullpaths[q-1]);
			strcpy(rec_menu_strs[q], rec_menu_strs[q-1]);
		}
	}
	char buf[512] = {0};
	strcpy(rec_menu_fullpaths[0], path);
	relativize_path(buf, rec_menu_fullpaths[0]);
	if(strlen(buf) > 62)
	{
		buf[60] = buf[61] = buf[62] = '.'; //add "..." as the last 3 characters
	}
	strncpy(rec_menu_strs[0], buf, 63);
	refresh_recent_menu();
	zc_set_config("zquest",last_quest_name,path);
	write_recent_quests();
}

void reload_zq_gui()
{
	init_custom_fonts();
	load_size_poses();
	refresh_visible_screens();
	update_combobrush();
	refresh(rCLEAR|rALL);
}
void change_mapscr_zoom(int delta)
{
	int num_screens = Map.getViewSize();
	num_screens = std::clamp(num_screens + delta, 1, mapscreen_num_screens_to_draw_max);
	Map.setViewSize(num_screens);
	std::string qst_cfg_header = qst_cfg_header_from_path(filepath);
	zc_set_config(qst_cfg_header.c_str(), "zoom_num_screens", Map.getViewSize());
	reload_zq_gui();
}
void toggle_is_compact()
{
	is_compact = !is_compact;
	zc_set_config("ZQ_GUI","compact_mode",is_compact?1:0);
	reload_zq_gui();
}
void toggle_merged_mode()
{
	if(is_compact)
	{
		compact_merged_combopane = !compact_merged_combopane;
		zc_set_config("ZQ_GUI","merge_cpane_compact",compact_merged_combopane?1:0);
	}
	else
	{
		large_merged_combopane = !large_merged_combopane;
		zc_set_config("ZQ_GUI","merge_cpane_large",large_merged_combopane?1:0);
	}
	reload_zq_gui();
}
void toggle_compact_sqr_mode()
{
	compact_square_panels = !compact_square_panels;
	zc_set_config("ZQ_GUI","square_panels_compact",compact_square_panels?1:0);
	reload_zq_gui();
}
void cycle_compact_sqr(bool down)
{
	if(!(is_compact && compact_square_panels))
		return;
	static const int num_panels = 3;
	if(down)
		compact_active_panel = (compact_active_panel+1)%num_panels;
	else
		compact_active_panel = (compact_active_panel-1+num_panels)%num_panels;
	reload_zq_gui();
}
void toggle_favzoom_mode()
{
	if(is_compact)
	{
		compact_zoomed_fav = !compact_zoomed_fav;
		zc_set_config("ZQ_GUI","zoom_fav_compact",compact_zoomed_fav?1:0);
	}
	else
	{
		large_zoomed_fav = !large_zoomed_fav;
		zc_set_config("ZQ_GUI","zoom_fav_large",large_zoomed_fav?1:0);
	}
	reload_zq_gui();
}
void toggle_cmdzoom_mode()
{
	if(is_compact)
	{
		compact_zoomed_cmd = !compact_zoomed_cmd; 
		zc_set_config("ZQ_GUI","zoom_cmd_compact",compact_zoomed_cmd?1:0);
	}
	else
	{
		large_zoomed_cmd = !large_zoomed_cmd;
		zc_set_config("ZQ_GUI","zoom_cmd_large",large_zoomed_cmd?1:0);
	}
	reload_zq_gui();
}

enum
{
	MENUID_FILE_SAVE,
	MENUID_FILE_SAVEAS,
	MENUID_FILE_REVERT,
};

static NewMenu file_menu
{
	{ "&New", do_NewQuest },
	{ "&Open", do_OpenQuest },
	{ "&Load Tileset", onTileset },
	{ "Recent", &recent_menu },
	{},
	{ "&Save", onSave, MENUID_FILE_SAVE },
	{ "Save &as...", onSaveAs, MENUID_FILE_SAVEAS },
	{ "&Revert", onRevert, MENUID_FILE_REVERT },
	{},
	{ "&Import", &import_menu },
	{ "&Export", &export_menu },
#ifndef __EMSCRIPTEN__
	{},
	{ "E&xit", handle_quit },
#endif
};

enum
{
	MENUID_MAPS_NEXT,
	MENUID_MAPS_PREV,
};
static NewMenu maps_menu
{
	{ "&Goto Map...", onGotoMap },
	{ "Next Map", onIncMap, MENUID_MAPS_NEXT },
	{ "Previous Map", onDecMap, MENUID_MAPS_PREV },
	{},
	{ "D&elete Map", onDeleteMap },
};

static NewMenu misc_menu
{
	{ "S&ubscreens", onEditSubscreens },
	{ "&Shop Types", onShopTypes },
	{ "Status &Effects", onStatusEffects },
	{ "&Bottle Types", onBottleTypes },
	{ "Bottle S&hop Types", onBottleShopTypes },
	{ "&Info Types", onInfoTypes },
	{ "&Warp Rings", onWarpRings },
	{ "&Triforce Pieces", onTriPieces },
	{ "E&nd String", onEndString },
	{ "Item &Drop Sets", onItemDropSets },
};

static NewMenu spr_menu
{
	{ "&Sprite Data", onCustomWpns },
	{ "&Hero", onCustomHero },
	{ "&Misc Sprites", onMiscSprites },
};

NewMenu colors_menu
{
	{ "&Main ", onColors_Main },
	{ "&Levels ", onColors_Levels },
	{ "&Sprites ", onColors_Sprites },
};

static NewMenu defs_menu
{
	{ "&Palettes", onDefault_Pals },
	{ "&Tiles", onDefault_Tiles },
	{ "&Combos", onDefault_Combos },
	{ "&Items", onDefault_Items },
	{ "&Enemies", onDefault_Guys },
	{ "&Sprite Data", onDefault_Weapons },
	{ "&Map Styles", onDefault_MapStyles },
	{ "SF&X Data", onDefault_SFX },
};

int32_t onEditComboAlias();
int32_t onEditComboPool();
int32_t onEditAutoCombo();

static NewMenu graphics_menu
{
	{ "&Palettes ", &colors_menu },
	{ "&Sprites ", &spr_menu },
	{ "&Combos", onCombos },
	{ "&Tiles", onTiles },
	{ "&Game icons", onIcons },
	{ "Misc co&lors", onMiscColors },
	{ "&Map styles", onMapStyles },
	{ "&Door Combo Sets", onDoorCombos },
	{ "Combo &Aliases", onEditComboAlias },
};

static NewMenu audio_menu
{
	{ "SF&X Data", onSelectSFX },
	{ "&MIDIs", onMidis },
	{ "Mis&c SFX", onMiscSFX },
};

void set_rules(byte* newrules);

void call_testqst_dialog();
int32_t onTestQst()
{
	call_testqst_dialog();
	return D_O_K;
}

int32_t onRulesDlg()
{
	call_qr_dialog(21, set_rules);
	return D_O_K;
}

int32_t onRulesSearch()
{
	call_qrsearch_dialog(set_rules);
	return D_O_K;
}

int32_t onZScriptSettings()
{
	ScriptRulesDialog(quest_rules, 17, [](byte* newrules)
	{
		saved = false;
		memcpy(quest_rules, newrules, QR_SZ);
		unpack_qrs();
	}).show();
	return D_O_K;
}

void call_zinf_dlg();
int32_t onZInfo()
{
	call_zinf_dlg();
	return D_O_K;
}

static NewMenu quest_menu
{
	{ "&Options ", onRulesDlg },
	{ "&Test", onTestQst },
	{ "&Items", onCustomItems },
	{ "Ene&mies", onCustomEnemies },
	{ "&Hero", onCustomHero },
	{ "&Strings", onStrings },
	{ "&DMaps", onDmaps },
	{ "I&nit Data", onInit },
	{ "Misc D&ata ", &misc_menu },
	{ "&ZInfo", onZInfo },
	{},
	{ "&Graphics ", &graphics_menu },
	{ "A&udio ", &audio_menu },
	{},
	{ "De&faults ", &defs_menu },
};

static NewMenu paste_menu
{
	{ "Paste &To All", onPasteToAll },
	{ "Paste &All To All", onPasteAllToAll },
};

static NewMenu paste_item_menu
{
	{ "&Undercombo", onPasteUnderCombo },
	{ "&Secret Combos", onPasteSecretCombos },
	{ "&Freeform Combos", onPasteFFCombos },
	{ "Screen &Data", onPasteScreenData },
	{ "&Warps", onPasteWarps },
	{ "Warp &Return", onPasteWarpLocations },
	{ "&Enemies", onPasteEnemies },
	{ "Room &Type Data", onPasteRoom },
	{ "&Guy/String", onPasteGuy },
	{ "Doo&rs", onPasteDoors },
	{ "&Layers", onPasteLayers },
	{ "&Palette", onPastePalette },
};

enum
{
	MENUID_EDIT_UNDO,
	MENUID_EDIT_REDO,
	MENUID_EDIT_COPY,
	MENUID_EDIT_PASTE,
	MENUID_EDIT_PASTEALL,
	MENUID_EDIT_ADVPASTE,
	MENUID_EDIT_SPECPASTE,
	MENUID_EDIT_DELETE,
};
static NewMenu edit_menu
{
	{ "&Undo", onUndo, MENUID_EDIT_UNDO },
	{ "&Redo", onRedo, MENUID_EDIT_REDO },
	{ "&Copy", onCopy, MENUID_EDIT_COPY },
	{ "&Paste", onPaste, MENUID_EDIT_PASTE },
	{ "Paste A&ll", onPasteAll, MENUID_EDIT_PASTEALL },
	{ "&Adv. Paste ", &paste_menu, MENUID_EDIT_ADVPASTE },
	{ "Paste &Spec. ", &paste_item_menu, MENUID_EDIT_SPECPASTE },
	{ "&Delete", onDelete, MENUID_EDIT_DELETE },
	{},
	{ "&Maps ", &maps_menu },
};

static NewMenu drawing_mode_menu
{
	{ "&Normal", onDrawingModeNormal, dm_normal },
	{ "&Combo Alias", onDrawingModeAlias, dm_alias },
	{ "&Pool", onDrawingModePool, dm_cpool },
	{ "&Auto Combo", onDrawingModeAuto, dm_auto },
};

static NewMenu integrity_check_menu
{
	{ "&All ", onIntegrityCheckAll },
	{ "&Screens ", onIntegrityCheckRooms },
	{ "&Warps ", onIntegrityCheckWarps },
};

static NewMenu quest_reports_menu
{
	{ "&Bugged Next-> Combo Locations", onBuggedNextComboLocationReport },
	{ "&Combo Locations", onComboLocationReport },
	{ "&Combo Type Locations", onComboTypeLocationReport },
	{ "&Enemy Locations", onEnemyLocationReport },
	{ "&Item Locations", onItemLocationReport },
	{ "&Script Locations", onScriptLocationReport },
	{ "&What Links Here", onWhatWarpsReport },
	{ "In&tegrity Check ", &integrity_check_menu },
};

int32_t onPalFix();
int32_t onPitFix();
int32_t onStrFix()
{
	if(get_qr(qr_OLD_STRING_EDITOR_MARGINS))
	{
		AlertDialog("Fix: Old Margins",
			"Fixing margins may cause strings that used to spill outside the textbox"
			" to instead be cut off. Are you sure?",
			[&](bool ret,bool)
			{
				if(ret)
				{
					set_qr(qr_OLD_STRING_EDITOR_MARGINS, 0);
					saved = false;
				}
			}).show();
	}
	if(get_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT))
	{
		AlertDialog("Fix: Old Frame Size",
			"This will fix the frame size of all strings. No visual changes should occur,"
			" as the string width/height will be fixed, but the compat QR will also be unchecked.",
			[&](bool ret,bool)
			{
				if(ret)
				{
					for(auto q = 0; q < msg_count; ++q)
					{
						MsgStrings[q].w += 16;
						MsgStrings[q].h += 16;
					}
					set_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT, 0);
					saved = false;
				}
			}).show();
	}
	return D_O_K;
}

int32_t onRemoveOldArrivalSquare();
enum
{
	MENUID_FIXTOOL_OLDSTRING,
};
static NewMenu fixtools_menu
{
	{ "&Color Set Fix", onCSetFix },
	{ "&Liquid Solidity Fix", onWaterSolidity },
	{ "&Effect Square Fix", onEffectFix },
	{ "&Level Palette Fix", onPalFix },
	{ "&Pit and Liquid Damage Fix", onPitFix },
	{ "&Old Strings Fix", onStrFix, MENUID_FIXTOOL_OLDSTRING },
	{ "&Green Arrival Square Fix", onRemoveOldArrivalSquare },
};

static NewMenu tool_menu
{
	{ "Combo &Flags", onFlags, nullopt, MFL_EXIT_PRE_PROC },
	{ "Fix &Tools ", &fixtools_menu },
	{ "&NES Dungeon Template", onTemplate },
	{ "&Apply Template to All", onReTemplate },
	{},
	{ "&Preview Mode", onPreviewMode },
	{ "Drawing &Mode ", &drawing_mode_menu },
	{},
	{ "&List Combos Used", onUsedCombos },
	{ "&Quest Reports ", &quest_reports_menu },
};

int32_t onLayer3BG()
{
	ViewLayer3BG = ViewLayer3BG ? 0 : 1;
	zc_set_config("zquest","layer3_bg",ViewLayer3BG);
	return D_O_K;
}
int32_t onLayer2BG()
{
	ViewLayer2BG = ViewLayer2BG ? 0 : 1;
	zc_set_config("zquest","layer2_bg",ViewLayer2BG);
	return D_O_K;
}
int onGridToggle();
enum
{
	MENUID_VIEW_WALKABILITY,
	MENUID_VIEW_FLAGS,
	MENUID_VIEW_CSET,
	MENUID_VIEW_TYPES,
	MENUID_VIEW_INFO,
	MENUID_VIEW_SQUARES,
	MENUID_VIEW_FFCS,
	MENUID_VIEW_SCRIPTNAMES,
	MENUID_VIEW_GRID,
	MENUID_VIEW_SCREENGRID,
	MENUID_VIEW_CURSCROUTLINE,
	MENUID_VIEW_DARKNESS,
	MENUID_VIEW_L2BG,
	MENUID_VIEW_L3BG,
	MENUID_VIEW_LAYERHIGHLIGHT,
};
NewMenu view_menu
{
	{ "View &Map...", onViewMap },
	{ "View &Palette", onShowPal },
	{},
	{ "Show &Walkability", onShowWalkability, MENUID_VIEW_WALKABILITY },
	{ "Show &Flags", onShowFlags, MENUID_VIEW_FLAGS },
	{ "Show &CSets", onShowCSet, MENUID_VIEW_CSET },
	{ "Show &Types", onShowCType, MENUID_VIEW_TYPES },
	{},
	{ "Show Screen &Info", onToggleShowInfo, MENUID_VIEW_INFO },
	{ "Show &Squares", onToggleShowSquares, MENUID_VIEW_SQUARES },
	{ "Show FFCs", onToggleShowFFCs, MENUID_VIEW_FFCS },
	{ "Show Script &Names", onToggleShowScripts, MENUID_VIEW_SCRIPTNAMES },
	{ "Show &Grid", onGridToggle, MENUID_VIEW_GRID },
	{ "Show Screen G&rid", onToggleScreenGrid, MENUID_VIEW_SCREENGRID },
	{ "Show Current Screen Outline", onToggleCurrentScreenOutline, MENUID_VIEW_CURSCROUTLINE },
	{ "Show &Darkness", onShowDarkness, MENUID_VIEW_DARKNESS },
	{ "Layer 2 is Background", onLayer2BG, MENUID_VIEW_L2BG },
	{ "Layer 3 is Background", onLayer3BG, MENUID_VIEW_L3BG },
	{ "Highlight Current Layer", onToggleHighlightLayer, MENUID_VIEW_LAYERHIGHLIGHT },
};

void set_rules(byte* newrules)
{
	saved = false;
	if(newrules != quest_rules)
		memcpy(quest_rules, newrules, QR_SZ);
	unpack_qrs();
	if(!get_qr(qr_ALLOW_EDITING_COMBO_0))
	{
		combobuf[0].walk = 0xF0;
		combobuf[0].type = 0;
		combobuf[0].flag = 0;
	}
	
	// For 2.50.0 and 2.50.1
	if(get_qr(qr_VERYFASTSCROLLING))
		set_qr(qr_FASTDNGN, 1);
}

int32_t onSelectFFCombo();

void onScreenNotes()
{
	edit_screen_notes(Map.CurrScr(), Map.getCurrMap(), Map.getCurrScr());
}
static NewMenu data_menu
{
	{ "&Screen Data", onScrData },
	{ "&Freeform Combos", onSelectFFCombo },
	{ "La&yers", onLayers },
	{ "&Tile Warp", onTileWarp },
	{ "Side &Warp", onSideWarp },
	{ "Secret &Combos", onSecretCombo },
	{ "&Under Combo", onUnderCombo },
	{ "&Doors", onDoors },
	{ "&Maze Path", onPath },
	{},
	{ "&Item", onItem },
	{ "&Enemies", onEnemies },
	{ "&Palette", onScreenPalette },
	{},
	{ "&Room Data", onRoom },
	{ "&Notes", onScreenNotes },
	{ "&Browse Notes", browse_screen_notes },
};

static NewMenu tunes_menu
{
	{ "ZC Forever", playZCForever },
	{ "Wind Fish", playTune1 },
	{ "Overworld", playTune2 },
	{ "Hyrule Castle", playTune3 },
	{ "Lost Woods", playTune4 },
	{ "Great Sea", playTune5 },
	{ "East Hyrule", playTune6 },
	{ "Dancing Dragon", playTune7 },
	{ "Stone Tower", playTune8 },
	{ "Villages", playTune9 },
	{ "Swamp + Desert", playTune10 },
	{ "Outset Island", playTune11 },
	{ "Kakariko Village", playTune12 },
	{ "Clock Town", playTune13 },
	{ "Temple", playTune14 },
	{ "Dark World", playTune15 },
	{ "Dragon Roost", playTune16 },
	{ "Horse Race", playTune17 },
	{ "Credits", playTune18 },
	{ "Zelda's Lullaby", playTune19 },
};

enum
{
	MENUID_MEDIA_TUNES,
	MENUID_MEDIA_PLAYMUSIC,
	MENUID_MEDIA_CHANGETRACK,
};
static NewMenu media_menu
{
	{ "Ambient Music ", &tunes_menu, MENUID_MEDIA_TUNES },
	{ "&Play music", playMusic, MENUID_MEDIA_PLAYMUSIC },
	{ "&Change track", changeTrack, MENUID_MEDIA_CHANGETRACK },
	{ "&Stop tunes", stopMusic },
};

enum
{
	MENUID_ETC_VIDMODE,
	MENUID_ETC_FULLSCREEN,
	MENUID_ETC_DEBUG_CONSOLE,
};
static NewMenu etc_menu
{
	{ "Help", &zq_help_menu },
	{ "&About", onAbout },
	{ "&Video Mode", onZQVidMode, MENUID_ETC_VIDMODE },
	{ "&Options...", onOptions },
	{ "&Hotkeys...", do_zq_hotkey_dialog },
	{ "&List Hotkeys...", do_zq_list_hotkeys_dialog },
	{ "&Fullscreen", onFullScreen, MENUID_ETC_FULLSCREEN },
	{},
	{ "&View Pic...", onViewPic },
	{ "Media", &media_menu },
	{},
	{ "&Debug Console", toggleConsole, MENUID_ETC_DEBUG_CONSOLE },
	{ "Clear Quest Filepath", onClearQuestFilepath },
	{ "&Take ZQ Snapshot", onSnapshot },
	{ "Take &Screen Snapshot", onMapscrSnapshot },
};

static NewMenu zscript_menu
{
	{ "Compile &ZScript...", onCompileScript },
	{},
	{ "&Compiler Settings", onZScriptCompilerSettings },
	{ "&Quest Script Settings", onZScriptSettings },
	{},
	{ "&Import ZASM Script", onImportZASM },
};

void set_console_state()
{
	etc_menu.select_uid(MENUID_ETC_DEBUG_CONSOLE, console_is_open);
}

NewMenu foo_menu{{"FOO",[](){InfoDialog("PLACEHOLDER","THIS IS A PLACEHOLDER").show();}}};

TopMenu the_menu
{
    { "&File", &file_menu },
    { "&Quest", &quest_menu },
    { "&Edit", &edit_menu },
    { "&View", &view_menu },
    { "&Tools", &tool_menu },
    { "&Screen", &data_menu },
    { "&ZScript", &zscript_menu },
    { "Et&C", &etc_menu },
};

void rebuild_trans_table();
int32_t launchPicViewer(BITMAP **pictoview, PALETTE pal,
                    int32_t *px2, int32_t *py2, double *scale, bool isviewingmap, bool skipmenu = false);

int32_t onResetTransparency()
{
    restore_mouse();
    rebuild_trans_table();
    jwin_alert("Notice","Translucency Table Rebuilt",NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
    
    refresh(rALL);
    return D_O_K;
}

int32_t onFullScreen()
{
	get_palette(RAMpal);
	bool windowed=is_windowed_mode()!=0;
	all_toggle_fullscreen(windowed);

	gui_mouse_focus=0;
	gui_bg_color=jwin_pal[jcBOX];
	gui_fg_color=jwin_pal[jcBOXFG];
	MouseSprite::set(ZQM_NORMAL);
	zc_set_palette(RAMpal);
	position_mouse(zq_screen_w/2,zq_screen_h/2);
	set_display_switch_mode(SWITCH_BACKGROUND);
	set_display_switch_callback(SWITCH_OUT, switch_out);
	set_display_switch_callback(SWITCH_IN, switch_in);
	zc_set_config("zquest","fullscreen", is_windowed_mode() ? 0 : 1);
	return D_REDRAW;
}

int32_t onEnter()
{
    if(key[KEY_ALT]||key[KEY_ALTGR])
    {
        return onFullScreen();
    }

    return D_O_K;
}

//PROC, x, y, w, h, fg, bg, key, flags, d1, d2, *dp, *dp2, *dp3

//*text, (*proc), *child, flags, *dp

void run_zq_frame();
int32_t d_nbmenu_proc(int32_t msg,DIALOG *d,int32_t c);


/*int32_t onY()
{
  return D_O_K;
}*/

int32_t onToggleGrid(bool color)
{
    if(color)
    {
        GridColor=(GridColor+8)%16;
		zc_set_config("zquest", "grid_color", GridColor);
    }
    else
    {
        ShowGrid=!ShowGrid;
		zc_set_config("zquest","show_grid",ShowGrid);
    }
    
    return D_O_K;
}

int onGridToggle()
{
	return onToggleGrid(CHECK_CTRL_CMD);
}

int32_t onToggleScreenGrid()
{
	ShowScreenGrid=!ShowScreenGrid;
	zc_set_config("zquest","show_screen_grid",ShowScreenGrid);
	return D_O_K;
}

int32_t onToggleCurrentScreenOutline()
{
	ShowCurScreenOutline=!ShowCurScreenOutline;
	zc_set_config("zquest","show_current_screen_outline",ShowCurScreenOutline);
	return D_O_K;
}

int32_t onToggleShowScripts()
{
    ShowFFScripts=!ShowFFScripts;
    zc_set_config("zquest","showffscripts",ShowFFScripts);
    return D_O_K;
}

int32_t onToggleShowFFCs()
{
    ShowFFCs=!ShowFFCs;
    zc_set_config("zquest","showffcs",ShowFFCs);
    return D_O_K;
}

int32_t onToggleShowSquares()
{
    ShowSquares=!ShowSquares;
    zc_set_config("zquest","showsquares",ShowSquares);
    return D_O_K;
}

int32_t onToggleShowInfo()
{
    ShowInfo=!ShowInfo;
	zc_set_config("zquest","showinfo",ShowInfo);
    return D_O_K;
}

int32_t onToggleHighlightLayer()
{
	ActiveLayerHighlight = ActiveLayerHighlight ? 0 : 1;
	zc_set_config("zquest","hl_active_lyr",ActiveLayerHighlight);
	return D_O_K;
}

int onKeySlash()
{
	if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
	{
		onAbout();
	}
	return D_O_K;
}

int onAKey()
{
	if(prv_mode)
		Map.set_prvadvance(1);
	return D_O_K;
}

int onReloadPreview()
{
	if(prv_mode)
	{
		Map.set_prvscr(Map.get_prv_map(), Map.get_prv_scr());
		Map.set_prvcmb(0);
	}
	return D_O_K;
}
int onSecretsPreview()
{
	if(prv_mode)
	{
		Map.prv_secrets(false);
		refresh(rALL);
	}
	return D_O_K;
}

int onSKey()
{
	if(CHECK_CTRL_CMD)
	{
		if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
		{
			onSaveAs();
		}
		else
		{
			if(!saved)
				onSave();
		}
	}
	else if(prv_mode)
	{
		Map.prv_secrets(false);
		refresh(rALL);
	}
	else onStrings();
	return D_O_K;
}
int onSetNewLayer(int newlayer)
{
	CurrentLayer = newlayer;
	refresh(rALL);
	return D_O_K;
}
void lpal_dsa()
{
	info_dsa("Level Palette Shortcuts",
		"You currently have level palette shortcuts disabled."
		" These can be re-enabled in 'Etc->Options', on the toggle 'Disable Level Palette Shortcuts'.",
		"dsa_lpal");
}
int onScreenLPal(int lpal)
{
	if(DisableLPalShortcuts)
	{
		lpal_dsa();
		return D_O_K;
	}
	saved=false;
	Map.setcolor(lpal);
	refresh(rSCRMAP);
	return D_O_K;
}

int32_t onPressEsc()
{
	if(zoomed_minimap)
		mmap_set_zoom(false);
	else return onExit();
	return D_O_K;
}

static DIALOG dialogs[] =
{
	/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)  (d1)         (d2)     (dp) */
	{ d_nbmenu_proc,     0,    0,    0,    13,    0,    0,    0,       D_USER,  0,            0, (void *) &the_menu, NULL, NULL },
	{ d_zq_hotkey_proc,  0,    0,    0,    0,     0,    0,    0,       0,       0,            0, NULL, NULL, NULL },
	
	{ d_keyboard_proc,   0,    0,    0,    0,     0,    0,    0,       0,       KEY_F1,       0, (void *) onHelp, NULL, NULL },
	{ d_keyboard_proc,   0,    0,    0,    0,     0,    0,    0,       0,       KEY_ESC,      0, (void *) onPressEsc, NULL, NULL },
	{ d_keyboard_proc,   0,    0,    0,    0,     0,    0,    39,      0,       0,            0, (void *) onUsedCombos, NULL, NULL },
	{ d_vsync_proc,      0,    0,    0,    0,     0,    0,    0,       0,       0,            0,       NULL, NULL, NULL },
	{ NULL,              0,    0,    0,    0,     0,    0,    0,       0,       0,            0,       NULL, NULL, NULL }
};


int32_t onDecColour()
{
	if ( CHECK_CTRL_CMD )
	{
		return onDecScrPal16();
	}
	
	else if ( key[KEY_LSHIFT] || key[KEY_RSHIFT] )
	{
		return onDecScrPal();
	}
	
	else
	{
		return onDecreaseCSet();
	}
}

int32_t onIncColour()
{
	
	if ( CHECK_CTRL_CMD )
	{
		return onIncScrPal16();
	}
	
	else if ( key[KEY_LSHIFT] || key[KEY_RSHIFT] )
	{
		return onIncScrPal();
	}
	
	else
	{
		return onIncreaseCSet();
	}
 }

static DIALOG getnum_dlg[] =
{
    // (dialog proc)       (x)   (y)    (w)     (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,        80,   80,     160,    72,   vc(0),              vc(11),           0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_rtext_proc,      114,  104+4,  48,     8,    jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,          0,             0, (void *) "Value:", NULL, NULL },
    { jwin_edit_proc,       168,  104,    48,     16,    0,                 0,                0,       0,          6,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   126,    61,     21,   vc(0),              vc(11),           13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  126,    61,     21,   vc(0),              vc(11),           27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t getnumber(const char *prompt,int32_t initialval)
{
    cancelgetnum=true;
    char buf[20];
    sprintf(buf,"%d",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=get_zc_font(font_lfont);
    getnum_dlg[2].dp=(void *)buf;
    
    large_dialog(getnum_dlg);
        
    int32_t ret=do_zqdialog(getnum_dlg,2);
    
    if(ret!=0&&ret!=4)
    {
        cancelgetnum=false;
    }
    
    if(ret==3)
        return atoi(buf);
        
    return initialval;
}

static DIALOG save_tiles_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Save Tile Pack", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void savesometiles(const char *prompt,int32_t initialval)
{
	
	char firsttile[8], tilecount[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	//int32_t ret;
	
	
	
	save_tiles_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	
	save_tiles_dlg[5].dp = firsttile;
	save_tiles_dlg[7].dp = tilecount;
	
	large_dialog(save_tiles_dlg);
	
	int32_t ret = do_zqdialog(save_tiles_dlg,-1);
	jwin_center_dialog(save_tiles_dlg);
	
	if(ret == 8)
	{
		first_tile_id = vbound(atoi(firsttile), 0, NEWMAXTILES);
		the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_new_file_compat("Save ZTILE(.ztile)", "ztile", NULL,datapath,false))
		{  
			char name[PATH_MAX];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
			if(f)
			{
				writetilefile(f,first_tile_id,the_tile_count);
				pack_fclose(f);
				char tmpbuf[PATH_MAX+20]={0};
				sprintf(tmpbuf,"Saved %s",name);
				jwin_alert("Success!",tmpbuf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
		}
	}
}

static DIALOG read_tiles_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Load Tilepack To:", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Starting at:",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  d_dummy_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { d_dummy_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void writesometiles_to(const char *prompt,int32_t initialval)
{
	
	char firsttile[8];;
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
		//int32_t ret;
	
	
	
	read_tiles_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	read_tiles_dlg[5].dp = firsttile;
	
	large_dialog(read_tiles_dlg);
	
	int32_t ret = do_zqdialog(read_tiles_dlg,-1);
	jwin_center_dialog(read_tiles_dlg);
	
	if(ret == 8)
	{
		first_tile_id = vbound(atoi(firsttile), 0, NEWMAXTILES);
		//the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_existing_file_compat("Load ZTILE(.ztile)", "ztile", NULL,datapath,false))
		{  
			
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				
				if (!readtilefile_to_location(f,first_tile_id))
				{
					al_trace("Could not read from .ztile packfile %s\n", name);
					jwin_alert("ZTILE File: Error","Could not load the specified Tile.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else
				{
					jwin_alert("ZTILE File: Success!","Loaded the source tiles to your tile sheets!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				pack_fclose(f);
			}
		}
	}
}


static DIALOG save_combofiles_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Save Combo Pack", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void savesomecombos(const char *prompt,int32_t initialval)
{
	
	char firsttile[8], tilecount[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	//int32_t ret;
	
	
	
	save_combofiles_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	
	save_combofiles_dlg[5].dp = firsttile;
	save_combofiles_dlg[7].dp = tilecount;
	
	large_dialog(save_combofiles_dlg);
	
	int32_t ret = do_zqdialog(save_combofiles_dlg,-1);
	jwin_center_dialog(save_combofiles_dlg);
	
	if(ret == 8)
	{
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOS-1));
		the_tile_count = vbound(atoi(tilecount), 1, (MAXCOMBOS-1)-first_tile_id);
		if(prompt_for_new_file_compat("Save ZCOMBO(.zcombo)", "zcombo", NULL,datapath,false))
		{  
			char name[PATH_MAX];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
			if(f)
			{
				writecombofile(f,first_tile_id,the_tile_count);
				pack_fclose(f);
				char tmpbuf[PATH_MAX+20]={0};
				sprintf(tmpbuf,"Saved %s",name);
				jwin_alert("Success!",tmpbuf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
		}
	}
}


static DIALOG load_comboset_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Combo Set (Range)", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First:",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  d_dummy_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { d_dummy_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_check_proc,        10,     46,     95,      9,    vc(14),                 vc(1),                   0,       0,           1,    0, (void *) "Don't Overwrite",                      NULL,   NULL                  },
    
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void writesomecombos(const char *prompt,int32_t initialval)
{
	
	char firsttile[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
		//int32_t ret;
	
	
	
	load_comboset_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	load_comboset_dlg[5].dp = firsttile;
	
	byte nooverwrite = 0;
	
	
	large_dialog(load_comboset_dlg);
	
	int32_t ret = do_zqdialog(load_comboset_dlg,-1);
	jwin_center_dialog(load_comboset_dlg);
	
	if(ret == 8)
	{
		if (load_comboset_dlg[10].flags & D_SELECTED) nooverwrite = 1;
	
		al_trace("Nooverwrite is: %d\n", nooverwrite);
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOS-1));
		//the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_existing_file_compat("Load ZCOMBO(.zcombo)", "zcombo", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				
				if (!readcombofile(f,first_tile_id,nooverwrite))
				{
					al_trace("Could not read from .zcombo packfile %s\n", name);
					jwin_alert("ZCOMBO File: Error","Could not load the specified combos.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else
				{
					jwin_alert("ZCOMBO File: Success!","Loaded the source combos to your combo pages!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
				pack_fclose(f);
			}
			
		}
	}
}

static DIALOG load_combopack_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Import Full Combo Package 1:1", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  d_dummy_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Starting at:",               NULL,   NULL  },
    { d_dummy_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  d_dummy_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { d_dummy_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_check_proc,        10,     42,     95,      9,    vc(14),                 vc(1),                   0,       0,           1,    0, (void *) "Don't Overwrite",                      NULL,   NULL                  },
    
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void loadcombopack(const char *prompt,int32_t initialval)
{
	
	char firsttile[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
		//int32_t ret;
	
	
	
	load_combopack_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	load_combopack_dlg[5].dp = firsttile;
	
	byte nooverwrite = 0;
	
	
	large_dialog(load_combopack_dlg);
	
	int32_t ret = do_zqdialog(load_combopack_dlg,-1);
	jwin_center_dialog(load_combopack_dlg);
	
	if(ret == 8)
	{
		if (load_combopack_dlg[10].flags & D_SELECTED) nooverwrite = 1;
	
		al_trace("Nooverwrite is: %d\n", nooverwrite);
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOS-1));
		//the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_existing_file_compat("Load ZCOMBO(.zcombo)", "zcombo", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				//need dialogue here
				if (!readcombofile(f,0,nooverwrite))
				{
					al_trace("Could not read from .zcombo packfile %s\n", name);
					jwin_alert("ZCOMBO File: Error","Could not load the specified Tile.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else
				{
					jwin_alert("ZCOMBO File: Success!","Loaded the source combos to your combo pages!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
			}
	
			pack_fclose(f);
		}
	}
}


static DIALOG read_combopack_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Load Combos (Specific Dest)", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    24,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Starting at:",               NULL,   NULL  },
    { jwin_edit_proc,          55,     22,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  d_dummy_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { d_dummy_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    //10
    { jwin_check_proc,        10,     58,     95,      9,    vc(14),                 vc(1),                   0,       0,           1,    0, (void *) "Don't Overwrite",                      NULL,   NULL                  },
    //11
    {  jwin_text_proc,        10,    42,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Skip:",               NULL,   NULL  },
    //12
    { jwin_edit_proc,          55,     40,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};



void writesomecombos_to(const char *prompt,int32_t initialval)
{
	
	char firsttile[8];
	char skiptile[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
		//int32_t ret;
	
	
	
	read_combopack_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(skiptile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	read_combopack_dlg[5].dp = firsttile;
	
	byte nooverwrite = 0;
	int32_t skipover = 0;
	
	sprintf(skiptile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	read_combopack_dlg[12].dp = skiptile;
	
	large_dialog(read_combopack_dlg);
	
	int32_t ret = do_zqdialog(read_combopack_dlg,-1);
	jwin_center_dialog(read_combopack_dlg);
	
	if(ret == 8)
	{
		if (read_combopack_dlg[10].flags & D_SELECTED) nooverwrite = 1;
		
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOS-1));
		skipover = vbound(atoi(skiptile), 0, (MAXCOMBOS-1));
		al_trace("skipover is: %d\n", skipover);
		//skipover = vbound(skipover, 0, (MAXCOMBOS-1-skipover));
		//the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_existing_file_compat("Load ZCOMBO(.zcombo)", "zcombo", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				
				if (!readcombofile_to_location(f,first_tile_id,nooverwrite, skipover))
				{
					al_trace("Could not read from .zcombo packfile %s\n", name);
					jwin_alert("ZCOMBO File: Error","Could not load the specified combos.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else
				{
					jwin_alert("ZCOMBO File: Success!","Loaded the source combos to your combo pages!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
				pack_fclose(f);
			}
			
		}
	}
}



static DIALOG save_dmaps_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Save DMaps (.zdmap)", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Last",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void savesomedmaps(const char *prompt,int32_t initialval)
{
	
	char firstdmap[8], lastdmap[8];
	int32_t first_dmap_id = 0; int32_t last_dmap_id = 0;
	sprintf(firstdmap,"%d",0);
	sprintf(lastdmap,"%d",1);
	//int32_t ret;
	
	
	
	save_dmaps_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firstdmap,"%d",0);
	sprintf(lastdmap,"%d",0);
	
	save_dmaps_dlg[5].dp = firstdmap;
	save_dmaps_dlg[7].dp = lastdmap;
	
	large_dialog(save_dmaps_dlg);
	
	int32_t ret = do_zqdialog(save_dmaps_dlg,-1);
	jwin_center_dialog(save_dmaps_dlg);
	
	if(ret == 8)
	{
		first_dmap_id = vbound(atoi(firstdmap), 0, MAXDMAPS-1);
		last_dmap_id = vbound(atoi(lastdmap), 0,  MAXDMAPS-1);
		
		if ( last_dmap_id < first_dmap_id )
		{
			int32_t swap = last_dmap_id;
			last_dmap_id = first_dmap_id;
			first_dmap_id = swap;			
		}
		if(!prompt_for_new_file_compat("Export DMaps (.zdmap)","zdmap",NULL,datapath,false))
		
		
		saved=false;
	    
		PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
		if(f)
		{
			if(!writesomedmaps(f,first_dmap_id,last_dmap_id,MAXDMAPS))
			{
				char buf[PATH_MAX+20],name[PATH_MAX];
				extract_name(temppath,name,FILENAMEALL);
				sprintf(buf,"Unable to load %s",name);
				jwin_alert("Error",buf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
			else
			{
				char name[PATH_MAX];
				extract_name(temppath,name,FILENAMEALL);
				char tmpbuf[PATH_MAX+20]={0};
				sprintf(tmpbuf,"Saved %s",name);
				jwin_alert("Success!",tmpbuf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
		}
		pack_fclose(f);
	}
}

static DIALOG save_comboaliasfiles_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Save Combo Alias Pack", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void savesomecomboaliases(const char *prompt,int32_t initialval)
{
	
	char firsttile[8], tilecount[8];
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	//int32_t ret;
	
	
	
	save_comboaliasfiles_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	sprintf(tilecount,"%d",1);
	
	save_comboaliasfiles_dlg[5].dp = firsttile;
	save_comboaliasfiles_dlg[7].dp = tilecount;
	
	large_dialog(save_comboaliasfiles_dlg);
	
	int32_t ret = do_zqdialog(save_comboaliasfiles_dlg,-1);
	jwin_center_dialog(save_comboaliasfiles_dlg);
	
	if(ret == 8)
	{
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOALIASES-1));
		the_tile_count = vbound(atoi(tilecount), 1, (MAXCOMBOALIASES-1)-first_tile_id);
		if(prompt_for_new_file_compat("Save ZALIAS(.zalias)", "zalias", NULL,datapath,false))
		{  
			char name[PATH_MAX];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
			if(f)
			{
				writecomboaliasfile(f,first_tile_id,the_tile_count);
				pack_fclose(f);
				char tmpbuf[PATH_MAX+20]={0};
				sprintf(tmpbuf,"Saved %s",name);
				jwin_alert("Success!",tmpbuf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
		}
	}
}


static DIALOG read_comboaliaspack_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)


	{ jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Load Combo Pack To:", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Starting at:",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  d_dummy_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { d_dummy_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void writesomecomboaliases_to(const char *prompt,int32_t initialval)
{
	
	char firsttile[8];;
	int32_t first_tile_id = 0; int32_t the_tile_count = 1;
	sprintf(firsttile,"%d",0);
		//int32_t ret;
	
	
	
	read_comboaliaspack_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firsttile,"%d",0);
	//sprintf(tilecount,"%d",1);
	
	read_comboaliaspack_dlg[5].dp = firsttile;
	
	large_dialog(read_comboaliaspack_dlg);
	
	int32_t ret = do_zqdialog(read_comboaliaspack_dlg,-1);
	jwin_center_dialog(read_comboaliaspack_dlg);
	
	if(ret == 8)
	{
		first_tile_id = vbound(atoi(firsttile), 0, (MAXCOMBOALIASES-1));
		//the_tile_count = vbound(atoi(tilecount), 1, NEWMAXTILES-first_tile_id);
		if(prompt_for_existing_file_compat("Load ZALIAS(.zalias)", "zalias", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				
				if (!readcomboaliasfile_to_location(f,first_tile_id))
				{
					al_trace("Could not read from .zcombo packfile %s\n", name);
					jwin_alert("ZALIAS File: Error","Could not load the specified combo aliases.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else
				{
					jwin_alert("ZALIAS File: Success!","Loaded the source combos to your combo alias table!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
				pack_fclose(f);
			}
		}
	}
}




//Doorsets

static DIALOG save_doorset_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      0,   0,   120,  100,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Save Doorset", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   72,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { jwin_button_proc,   69,  72,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void do_exportdoorset(const char *prompt,int32_t initialval)
{
	char firstdoor[8], doorct[8];
	int32_t first_doorset_id = 0; int32_t the_doorset_count = 1;
	sprintf(firstdoor,"%d",0);
	sprintf(doorct,"%d",1);
	//int32_t ret;
	save_doorset_dlg[0].dp2 = get_zc_font(font_lfont);
	
	sprintf(firstdoor,"%d",0);
	sprintf(doorct,"%d",1);
	
	save_doorset_dlg[5].dp = firstdoor;
	save_doorset_dlg[7].dp = doorct;
	
	large_dialog(save_doorset_dlg);
	
	int32_t ret = do_zqdialog(save_doorset_dlg,-1);
	jwin_center_dialog(save_doorset_dlg);
	
	if(ret == 8) //OK
	{
		/* sanity bounds
		first_doorset_id = vbound(atoi(firstdoor), 0, (MAXCOMBOS-1));
		the_doorset_count = vbound(atoi(doorct), 1, (MAXCOMBOS-1)-first_doorset_id);
		*/
		if(prompt_for_new_file_compat("Save ZDOORS(.zdoors)", "zdoors", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
			if(f)
			{
				writezdoorsets(f,first_doorset_id,the_doorset_count);
				pack_fclose(f);
				char tmpbuf[512]={0};
				sprintf(tmpbuf,"Saved %s",name);
				jwin_alert("Success!",tmpbuf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			}
		}
	}
}

static DIALOG load_doorset_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      0,   0,   120,  124,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Door Set (Range)", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //for future tabs
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    //4
    {  jwin_text_proc,        10,    28,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "First:",               NULL,   NULL  },
    { jwin_edit_proc,          55,     26,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //6
    {  jwin_text_proc,        10,    46,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Count",               NULL,   NULL  },
    { jwin_edit_proc,          55,     44,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    { jwin_button_proc,   15,   92,  36,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,   69,  92,  36,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    //10
    {  jwin_text_proc,        10,    64,     20,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Dest",               NULL,   NULL  },
    { jwin_edit_proc,          55,     63,    40,     16,    vc(12),                 vc(1),                   0,       0,          63,    0,  NULL,                                           NULL,   NULL                  },
    //8
    
    // { jwin_check_proc,        10,     46,     95,      9,    vc(14),                 vc(1),                   0,       0,           1,    0, (void *) "Don't Overwrite",                      NULL,   NULL                  },
    
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void do_importdoorset(const char *prompt,int32_t initialval)
{
	
	char firstdoor[8], doorct[8], destid[8];
	int32_t first_doorset_id = 0; int32_t the_doorset_count = 1;
	int32_t the_dest_id = 0;
	sprintf(firstdoor,"%d",0);
	sprintf(doorct,"%d",1);
	sprintf(destid,"%d",0);
		//int32_t ret;
	
	save_doorset_dlg[0].dp2 = get_zc_font(font_lfont);
	
	load_doorset_dlg[5].dp = firstdoor;
	load_doorset_dlg[7].dp = doorct;
	load_doorset_dlg[11].dp = destid;
	
	byte nooverwrite = 0;
	
	large_dialog(load_doorset_dlg);
	
	int32_t ret = do_zqdialog(load_doorset_dlg,-1);
	jwin_center_dialog(load_doorset_dlg);
	
	if(ret == 8) //OK
	{
		//if (load_doorset_dlg[10].flags & D_SELECTED) nooverwrite = 1;
	
		//sanity bound
		first_doorset_id = vbound(atoi(firstdoor), 0, door_combo_set_count);
		the_doorset_count = vbound(atoi(doorct), 1, door_combo_set_count);
		the_dest_id = vbound(atoi(destid), 0, door_combo_set_count);
		if(prompt_for_existing_file_compat("Load ZDOORS(.zdoors)", "zdoors", NULL,datapath,false))
		{  
			char name[256];
			extract_name(temppath,name,FILENAMEALL);
			PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
			if(f)
			{
				int32_t ret = readzdoorsets(f,first_doorset_id,the_doorset_count, the_dest_id);
				
				if (!ret)
				{
					al_trace("Could not read from .zdoors packfile %s\n", name);
					jwin_alert("ZDOORS File: Error","Could not load the specified doorsets.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				else if ( ret == 1 )
				{
					jwin_alert("ZDOORS File: Success!","Loaded the source doorsets!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
				else if ( ret == 2 )
				{
					jwin_alert("ZDOORS File: Issue:","Targets exceed doorset count!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					saved=false;
				}
				pack_fclose(f);
			}
		}
	}
}

int32_t gettilepagenumber(const char *prompt, int32_t initialval)
{
    char buf[20];
    sprintf(buf,"%d",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=get_zc_font(font_lfont);
    getnum_dlg[2].dp=buf;
    
    large_dialog(getnum_dlg);
        
    int32_t ret = do_zqdialog(getnum_dlg,2);
    
    if(ret==3)
        return atoi(buf);
        
    return -1;
}

int32_t gethexnumber(const char *prompt,int32_t initialval)
{
    cancelgetnum=true;
    char buf[20];
    sprintf(buf,"%X",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=get_zc_font(font_lfont);
    getnum_dlg[2].dp=(void *)buf;
    
    large_dialog(getnum_dlg);
        
    int32_t ret=do_zqdialog(getnum_dlg,2);
    
    if(ret!=0&&ret!=4)
    {
        cancelgetnum=false;
    }
    
    if(ret==3)
        return zc_xtoi(buf);
        
    return initialval;
}

void update_combo_cycling()
{
    Map.update_combo_cycling();
}

void update_freeform_combos()
{
    Map.update_freeform_combos();
}

bool layers_valid(mapscr *tempscr)
{
    for(int32_t i=0; i<6; i++)
    {
        if(tempscr->layermap[i]>map_count)
        {
            return false;
        }
    }
    
    return true;
}

void fix_layers(mapscr *tempscr, bool showwarning)
{
    char buf[80]="layers have been changed: ";
    
    for(int32_t i=0; i<6; i++)
    {
        if(tempscr->layermap[i]>map_count)
        {
            strcat(buf, "%d ");
            sprintf(buf, buf, i+1);
            tempscr->layermap[i]=0;
        }
    }
    
    if(showwarning)
    {
        jwin_alert("Invalid layers detected",
                   "One or more layers on this screen used",
                   "maps that do not exist. The settings of these",
                   buf, "O&K", NULL, 'o', 0, get_zc_font(font_lfont));
    }
}

extern const char *colorlist(int32_t index, int32_t *list_size);

static char autobackup_str_buf[32];
const char *autobackuplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,10);
        
        if(index==0)
        {
            sprintf(autobackup_str_buf,"Disabled");
        }
        else
        {
            sprintf(autobackup_str_buf,"%2d",index);
        }
        
        return autobackup_str_buf;
    }
    
    *list_size=11;
    return NULL;
}

static char autosave_str_buf[32];
const char *autosavelist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,10);
        
        if(index==0)
        {
            sprintf(autosave_str_buf,"Disabled");
        }
        else
        {
            sprintf(autosave_str_buf,"%2d Minute%c",index,index>1?'s':0);
        }
        
        return autosave_str_buf;
    }
    
    *list_size=11;
    return NULL;
}

const char *autosavelist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,9);
        sprintf(autosave_str_buf,"%2d",index+1);
        return autosave_str_buf;
    }
    
    *list_size=10;
    return NULL;
}


static int32_t options_1_list[] =
{
    // dialog control number
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, -1
};

static int32_t options_2_list[] =
{
    // dialog control number
	50, 51, -1
};

static int32_t options_3_list[] =
{
    // dialog control number
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, -1
};
static int32_t options_4_list[] =
{
	57, 58, 59, 60,
	-1
};

static TABPANEL options_tabs[] =
{
    // (text)
    { (char *)" 1 ",       D_SELECTED,   options_1_list,  0, NULL },
    { (char *)" 2 ",       0,            options_2_list,  0, NULL },
    { (char *)" 3 ",       0,            options_3_list,  0, NULL },
    { (char *)" 4 ",       0,            options_4_list,  0, NULL },
    { NULL,                0,            NULL, 0, NULL }
};

static ListData autobackup_list(autobackuplist, &font);
static ListData autosave_list(autosavelist, &font);
static ListData autosave_list2(autosavelist2, &font);
static ListData color_list(colorlist, &font);
static ListData snapshotformat_list(snapshotformatlist, &font);
void init_ffpos();

const char *dm_names[dm_max]=
{
    "Normal",
    "Relational", // Removed.
    "Dungeon",    // Removed.
    "Alias",
	"Pool",
	"Auto"
};

void fix_drawing_mode_menu()
{
	drawing_mode_menu.select_only_uid(draw_mode);
}

int32_t onDrawingMode()
{
    draw_mode=(draw_mode+1)%dm_max;
	int dm_relational = 1;
	if ((int)draw_mode == dm_relational)
		draw_mode += 2;
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int32_t onDrawingModeNormal()
{
    draw_mode=dm_normal;
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int32_t onDrawingModeAlias()
{
    if(draw_mode==dm_alias)
    {
        return onDrawingModeNormal();
    }
    
    draw_mode=dm_alias;
    alias_cset_mod=0;
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int32_t onDrawingModePool()
{
    if(draw_mode==dm_cpool)
    {
        return onDrawingModeNormal();
    }
    
    draw_mode=dm_cpool;
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int32_t onDrawingModeAuto()
{
	if (draw_mode == dm_auto)
	{
		return onDrawingModeNormal();
	}

	draw_mode = dm_auto;
	fix_drawing_mode_menu();
	restore_mouse();
	return D_O_K;
}

int32_t onReTemplate()
{
    if(jwin_alert("Confirm Overwrite","Apply NES Dungeon template to","all screens on this map?",NULL,"&Yes","&No",'y','n',get_zc_font(font_lfont))==1)
    {
        Map.TemplateAll();
        refresh(rALL);
    }
    
    return D_O_K;
}

int32_t onUndo()
{
    Map.UndoCommand();
    refresh(rALL);
    return D_O_K;
}

int32_t onRedo()
{
    Map.RedoCommand();
    refresh(rALL);
    return D_O_K;
}

int32_t onCopy()
{
    if(prv_mode)
    {
        Map.set_prvcmb(Map.get_prvcmb()==0?1:0);
        
        init_ffpos();
        
        return D_O_K;
    }
    
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.Copy(screen);
    return D_O_K;
}

int32_t onPaste()
{
	if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
	{
		if(CHECK_CTRL_CMD)
			return onPasteAllToAll();
		else return onPasteAll();
	}
	else if(CHECK_CTRL_CMD)
		return onPasteToAll();
	else
	{
		int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
		Map.DoPasteScreenCommand(PasteCommandType::ScreenPartial, screen);
	}
	return D_O_K;
}

int32_t onPasteAll()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
	Map.DoPasteScreenCommand(PasteCommandType::ScreenAll, screen);
	return D_O_K;
}

int32_t onPasteToAll()
{
	if(confirmBox("You are about to paste to all screens on the current map."))
	{
		Map.DoPasteScreenCommand(PasteCommandType::ScreenPartialToEveryScreen);
	}
	return D_O_K;
}

int32_t onPasteAllToAll()
{
	if(confirmBox("You are about to paste to all screens on the current map."))
	{
		Map.DoPasteScreenCommand(PasteCommandType::ScreenAllToEveryScreen);
	}
	return D_O_K;
}

int32_t onPasteUnderCombo()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenUnderCombo, screen);
    return D_O_K;
}

int32_t onPasteSecretCombos()
{
    Map.DoPasteScreenCommand(PasteCommandType::ScreenSecretCombos);
    return D_O_K;
}

int32_t onPasteFFCombos()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenFFCombos, screen);
    return D_O_K;
}

int32_t onPasteWarps()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenWarps, screen);
    return D_O_K;
}

int32_t onPasteScreenData()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenData, screen);
    return D_O_K;
}

int32_t onPasteWarpLocations()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenWarpLocations, screen);
    return D_O_K;
}

int32_t onPasteDoors()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenDoors, screen);
    return D_O_K;
}

int32_t onPasteLayers()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenLayers, screen);
    return D_O_K;
}

int32_t onPastePalette()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenPalette, screen);
    return D_O_K;
}

int32_t onPasteRoom()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenRoom, screen);
    return D_O_K;
}

int32_t onPasteGuy()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenGuy, screen);
    return D_O_K;
}

int32_t onPasteEnemies()
{
	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
    Map.DoPasteScreenCommand(PasteCommandType::ScreenEnemies, screen);
    return D_O_K;
}

int32_t onDelete()
{
    restore_mouse();

	int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
	mapscr* scr = active_visible_screen ? active_visible_screen->scr : Map.CurrScr();
	if(!(scr->valid&mVALID) || jwin_alert("Confirm Delete","Delete this screen?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
	{
		Map.DoClearScreenCommand(screen);
	}
    
    saved=false;
    return D_O_K;
}

int32_t onDeleteMap()
{
    if(jwin_alert("Confirm Delete","Clear this entire map?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        Map.clearmap(false);
        refresh(rALL);
        saved=false;
    }
    
    return D_O_K;
}

int32_t onToggleDarkness()
{
    Map.CurrScr()->flags^=4;
    refresh(rMAP+rMENU);
    saved=false;
    return D_O_K;
}

int32_t onIncMap()
{
    int32_t m=Map.getCurrMap();
    int32_t oldcolor=Map.getcolor();
    Map.setCurrMap(m+1>=map_count?0:m+1);
    // Map.setCurrScr(Map.getCurrScr()); //Needed to refresh the screen info. -Z ( 26th March, 2019 )
    Map.setlayertarget(); //Needed to refresh the screen info. -Z ( 26th March, 2019 )
    
    int32_t newcolor=Map.getcolor();
    
    if(newcolor!=oldcolor)
    {
        rebuild_trans_table();
    }
    
    refresh(rALL);
    return D_O_K;
}

int32_t onDecMap()
{
    int32_t m=Map.getCurrMap();
    int32_t oldcolor=Map.getcolor();
    Map.setCurrMap((m-1<0)?map_count-1:zc_min(m-1,map_count-1));
    // Map.setCurrScr(Map.getCurrScr()); //Needed to refresh the screen info. -Z ( 26th March, 2019 )
    Map.setlayertarget(); //Needed to refresh the screen info. -Z ( 26th March, 2019 )
    
    int32_t newcolor=Map.getcolor();
    
    if(newcolor!=oldcolor)
    {
        rebuild_trans_table();
    }
    
    refresh(rALL);
    return D_O_K;
}


int32_t onDefault_Pals()
{
    if(jwin_alert("Confirm Reset","Reset all palette data?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        
        if(!init_colordata(true, &header, &QMisc))
        {
            jwin_alert("Error","Palette reset failed.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
        }
        
        refresh_pal();
    }
    
    return D_O_K;
}

int32_t onDefault_Combos()
{
    if(jwin_alert("Confirm Reset","Reset combo data?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        
        if(!init_combos(true, &header))
        {
            jwin_alert("Error","Combo reset failed.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}

int32_t onDefault_Items()
{
    if(jwin_alert("Confirm Reset","Reset all items?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        reset_items(true, &header);
    }
    
    return D_O_K;
}

int32_t onDefault_Weapons()
{
    if(jwin_alert("Confirm Reset","Reset weapon/misc. sprite data?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        reset_wpns(true, &header);
    }
    
    return D_O_K;
}

int32_t onDefault_Guys()
{
    if(jwin_alert("Confirm Reset","Reset all enemy/NPC data?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        reset_guys();
    }
    
    return D_O_K;
}


int32_t onDefault_Tiles()
{
    if(jwin_alert("Confirm Reset","Reset all tiles?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        
        if(!init_tiles(true, &header))
        {
            jwin_alert("Error","Tile reset failed.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}

void change_sfx(SAMPLE *sfx1, SAMPLE *sfx2);

int32_t onDefault_SFX()
{
	if(jwin_alert("Confirm Reset","Reset all sound effects?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
	{
		saved=false;
		SAMPLE *temp_sample;
		
		for(int32_t i=1; i<WAV_COUNT; i++)
		{
			temp_sample = (SAMPLE *)sfxdata[zc_min(i,Z35)].dat;
			change_sfx(&customsfxdata[i], temp_sample);
			sprintf(sfx_string[i],"s%03d",i);
			
			if(i<Z35)
				strcpy(sfx_string[i], old_sfx_string[i-1]);
			set_bit(customsfxflag, i<Z35?1:0, i-1);
		}
	}
	
	return D_O_K;
}


int32_t onDefault_MapStyles()
{
    if(jwin_alert("Confirm Reset","Reset all map styles?", NULL, NULL, "Yes", "Cancel", 'y', 27,get_zc_font(font_lfont)) == 1)
    {
        saved=false;
        reset_mapstyles(true, &QMisc);
    }
    
    return D_O_K;
}

int onScrollScreen(int dir, bool warp)
{
	Map.scroll(dir,warp);
	return D_O_K;
}

int32_t onComboColLeft()
{
	if(draw_mode==dm_cpool||draw_mode==dm_auto)
		;
	else if((First[current_combolist]>0)&&(draw_mode!=dm_alias))
	{
		First[current_combolist]-=1;
		clear_tooltip();
		refresh(rCOMBOS);
	}
	else if((combo_alistpos[current_comboalist]>0)&&(draw_mode==dm_alias))
	{
		combo_alistpos[current_comboalist]-=1;
		clear_tooltip();
		refresh(rCOMBOS);
	}
	
	clear_keybuf();
	return D_O_K;
}

int32_t onComboColRight()
{
	auto& sqr = (draw_mode == dm_alias ? comboaliaslist[current_comboalist] : combolist[current_combolist]);
	if(draw_mode==dm_cpool||draw_mode==dm_auto)
		;
	else if((First[current_combolist]<(MAXCOMBOS-(sqr.w*sqr.h)))&&(draw_mode!=dm_alias))
	{
		First[current_combolist]+=1;
		clear_tooltip();
		refresh(rCOMBOS);
	}
	else if((combo_alistpos[current_comboalist]<(MAXCOMBOALIASES-(sqr.w*sqr.h)))&&(draw_mode==dm_alias))
	{
		combo_alistpos[current_comboalist]+=1;
		clear_tooltip();
		refresh(rCOMBOS);
	}
	
	clear_keybuf();
	return D_O_K;
}

int32_t onComboColUp()
{
	auto& sqr = (draw_mode == dm_alias ? comboaliaslist[current_comboalist] : combolist[current_combolist]);
	if(draw_mode==dm_cpool||draw_mode==dm_auto)
		;
	else if((First[current_combolist]>0)&&(draw_mode!=dm_alias))
	{
		First[current_combolist]-=zc_min(First[current_combolist],sqr.w);
		clear_tooltip();
		
		refresh(rCOMBOS);
	}
	else if((combo_alistpos[current_comboalist]>0)&&(draw_mode==dm_alias))
	{
		combo_alistpos[current_comboalist]-=zc_min(combo_alistpos[current_comboalist],sqr.w);
		clear_tooltip();
		refresh(rCOMBOS);
	}
	
	clear_keybuf();
	return D_O_K;
}

int32_t onComboColDown()
{
	auto& sqr = (draw_mode == dm_alias ? comboaliaslist[current_comboalist] : combolist[current_combolist]);
	
	if(draw_mode==dm_cpool||draw_mode==dm_auto)
		;
	else if((First[current_combolist]<(MAXCOMBOS-(sqr.w*sqr.h)))&&(draw_mode!=dm_alias))
	{
		First[current_combolist]+=zc_min((MAXCOMBOS-sqr.w)-First[current_combolist],sqr.w);
		clear_tooltip();
		refresh(rCOMBOS);
	}
	else if((combo_alistpos[current_comboalist]<(MAXCOMBOALIASES-(comboaliaslist[0].w*comboaliaslist[0].h)))&&(draw_mode==dm_alias))
	{
		combo_alistpos[current_comboalist]+=zc_min((MAXCOMBOALIASES-sqr.w)-combo_alistpos[current_comboalist],sqr.w);
		clear_tooltip();
		refresh(rCOMBOS);
	}
	
	clear_keybuf();
	return D_O_K;
}

void scrollup(int j)
{
    switch(draw_mode)
	{
		case dm_alias:
		{
			auto& sqr = comboaliaslist[j];
			if(combo_alistpos[j]>0)
			{
				if(CHECK_CTRL_CMD)
				{
					combo_alistpos[j]=0;
					clear_tooltip();
				}
				else
				{
					combo_alistpos[j]-=zc_min(combo_alistpos[j],(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
		case dm_cpool:
		{
			auto& sqr = comboaliaslist[j];
			if(combo_pool_listpos[j]>0)
			{
				if(CHECK_CTRL_CMD)
				{
					combo_pool_listpos[j]=0;
					clear_tooltip();
				}
				else
				{
					combo_pool_listpos[j]-=zc_min(combo_pool_listpos[j],(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
		case dm_auto:
		{
			auto& sqr = comboaliaslist[j];
			if (combo_auto_listpos[j] > 0)
			{
				if (CHECK_CTRL_CMD)
				{
					combo_auto_listpos[j] = 0;
					clear_tooltip();
				}
				else
				{
					combo_auto_listpos[j] -= zc_min(combo_auto_listpos[j], (sqr.w * sqr.h));
					clear_tooltip();
				}

				refresh(rCOMBOS);
			}
			break;
		}
		default:
		{
			auto& sqr = combolist[j];
			if(First[j]>0)
			{
				if(CHECK_CTRL_CMD)
				{
					First[j]-=zc_min(First[j],256);
					clear_tooltip();
				}
				else
				{
					First[j]-=zc_min(First[j],(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
	}
}
void scrolldown(int j)
{
	switch(draw_mode)
	{
		case dm_alias:
		{
			auto& sqr = comboaliaslist[j];
			if(combo_alistpos[j]<(MAXCOMBOALIASES-(sqr.w*sqr.h)))
			{
				if(CHECK_CTRL_CMD)
				{
					combo_alistpos[j]=MAXCOMBOALIASES-(sqr.w*sqr.h);
					clear_tooltip();
				}
				else
				{
					combo_alistpos[j]=zc_min((MAXCOMBOALIASES-(sqr.w*sqr.h)),combo_alistpos[j]+(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
		case dm_cpool:
		{
			auto& sqr = comboaliaslist[j];
			if(combo_pool_listpos[j]<(MAXCOMBOALIASES-(sqr.w*sqr.h)))
			{
				if(CHECK_CTRL_CMD)
				{
					combo_pool_listpos[j]=MAXCOMBOALIASES-(sqr.w*sqr.h);
					clear_tooltip();
				}
				else
				{
					combo_pool_listpos[j]=zc_min((MAXCOMBOALIASES-(sqr.w*sqr.h)),combo_pool_listpos[j]+(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
		case dm_auto:
		{
			auto& sqr = comboaliaslist[j];
			if (combo_auto_listpos[j] < (MAXCOMBOALIASES - (sqr.w * sqr.h)))
			{
				if (CHECK_CTRL_CMD)
				{
					combo_auto_listpos[j] = MAXCOMBOALIASES - (sqr.w * sqr.h);
					clear_tooltip();
				}
				else
				{
					combo_auto_listpos[j] = zc_min((MAXCOMBOALIASES - (sqr.w * sqr.h)), combo_pool_listpos[j] + (sqr.w * sqr.h));
					clear_tooltip();
				}

				refresh(rCOMBOS);
			}
			break;
		}
		default:
		{
			auto& sqr = combolist[j];
			if(First[j]<(MAXCOMBOS-(sqr.w*sqr.h)))
			{
				if(CHECK_CTRL_CMD)
				{
					First[j]=zc_min((MAXCOMBOS-sqr.w*sqr.h),First[j]+256);
					clear_tooltip();
				}
				else
				{
					First[j]=zc_min((MAXCOMBOS-(sqr.w*sqr.h)),First[j]+(sqr.w*sqr.h));
					clear_tooltip();
				}
				
				refresh(rCOMBOS);
			}
			break;
		}
	}
}

int32_t onPgUp()
{
    switch(draw_mode)
	{
		case dm_alias:
			scrollup(current_comboalist);
			break;
		case dm_cpool:
			scrollup(current_cpoollist);
			break;
		case dm_auto:
			scrollup(current_cautolist);
			break;
		default:
			scrollup(current_combolist);
			break;
	}
    return D_O_K;
}

int32_t onPgDn()
{
    switch(draw_mode)
	{
		case dm_alias:
			scrolldown(current_comboalist);
			break;
		case dm_cpool:
			scrolldown(current_cpoollist);
			break;
		case dm_auto:
			scrolldown(current_cautolist);
			break;
		default:
			scrolldown(current_combolist);
			break;
	}
    return D_O_K;
}

int32_t onIncreaseCSet()
{
	if(draw_mode!=dm_alias)
	{
		CSet=wrap(CSet+1,0,13);
		refresh(rCOMBOS+rMENU+rCOMBO);
	}
	else
	{
		alias_cset_mod=wrap(alias_cset_mod+1,0,13);
	}
    return D_O_K;
}

int32_t onDecreaseCSet()
{
	if(draw_mode!=dm_alias)
	{
		CSet=wrap(CSet-1,0,13);
		refresh(rCOMBOS+rMENU+rCOMBO);
	}
	else
	{
		alias_cset_mod=wrap(alias_cset_mod-1,0,13);
	}
    return D_O_K;
}

int32_t onGotoPage()
{
	if (draw_mode==dm_alias)
	{
		static const int PER_PAGE = 260;
		if(optional<int> v = call_get_num("Scroll to Alias Page", 0, MAXCOMBOALIASES/PER_PAGE-1, 0))
			combo_alistpos[current_comboalist] = *v*PER_PAGE;
	}
	else if (draw_mode==dm_cpool)
	{
		static const int PER_PAGE = 260;
		if(optional<int> v = call_get_num("Scroll to Combo Pool Page", 0, MAXCOMBOPOOLS/PER_PAGE-1, 0))
			combo_pool_listpos[current_cpoollist] = *v*PER_PAGE;
	}
	else if (draw_mode == dm_auto)
	{
		static const int PER_PAGE = 260;
		if(optional<int> v = call_get_num("Scroll to Auto Combo Page", 0, MAXAUTOCOMBOS/PER_PAGE-1, 0))
			combo_auto_listpos[current_cautolist] = *v*PER_PAGE;
	}
	else
	{
		static const int PER_PAGE = 256;
		if(optional<int> v = call_get_num("Scroll to Combo Page", 0, MAXCOMBOS/PER_PAGE-1, 0))
			First[current_combolist] = *v*PER_PAGE;
	}
    
    return D_O_K;
}

static char track_number_str_buf[MIDI_TRACK_BUFFER_SIZE] = {0};
const char *tracknumlist(int32_t index, int32_t *list_size)
{
    //memset(track_number_str_buf,0,50);
    if(index>=0)
    {
        bound(index,0,255);
        std::string name = zcmusic_get_track_name(zcmusic, index);
        sprintf(track_number_str_buf,"%02d %s",index+1, name.c_str());
        return track_number_str_buf;
    }
    
    *list_size=zcmusic_get_tracks(zcmusic);
    return NULL;
}

static ListData tracknum_list(tracknumlist, &font);

static DIALOG change_track_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      60-12,   40,   200-16,  72,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Track", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_droplist_proc, 72-12,   60+4,   161,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,     0,             0, (void *) &tracknum_list, NULL, NULL },
    { jwin_button_proc,   70,   87,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   150,  87,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};
//  return list_dlg[2].d1;

int32_t changeTrack()
{
    restore_mouse();
    change_track_dlg[0].dp2=get_zc_font(font_lfont);
    change_track_dlg[2].d1=gme_track;
    
    large_dialog(change_track_dlg);
    
    if(do_zqdialog(change_track_dlg,2)==3)
    {
        gme_track=change_track_dlg[2].d1;
        zcmusic_change_track(zcmusic, gme_track);
    }
    
    return D_O_K;
}

void set_media_tunes()
{
	media_menu.select_uid(MENUID_MEDIA_TUNES, true);
	media_menu.select_uid(MENUID_MEDIA_PLAYMUSIC, false);
	disable_hotkey(ZQKEY_AMBIENT_MUSIC, false);
	disable_hotkey(ZQKEY_PLAY_MUSIC, false);
		
	media_menu.disable_uid(MENUID_MEDIA_CHANGETRACK, true);
	disable_hotkey(ZQKEY_CHANGE_TRACK, true);
}

int32_t playMusic()
{
	char *ext;
	bool ismidi=false;
	char allmusic_types[256];
	sprintf(allmusic_types, "%s;mid", zcmusic_types);
	
	if(prompt_for_existing_file_compat("Load Music",(char*)allmusic_types,NULL,midipath,false))
	{
		strcpy(midipath,temppath);
		
		ext=get_extension(midipath);
		
		if(
			(stricmp(ext,"ogg")==0)||
			(stricmp(ext,"mp3")==0)||
			(stricmp(ext,"it")==0)||
			(stricmp(ext,"xm")==0)||
			(stricmp(ext,"s3m")==0)||
			(stricmp(ext,"mod")==0)||
			(stricmp(ext,"spc")==0)||
			(stricmp(ext,"gym")==0)||
			(stricmp(ext,"nsf")==0)||
			(stricmp(ext,"gbs")==0)||
			(stricmp(ext,"vgm")==0)
		)
		{
			ismidi=false;
		}
		else if((stricmp(ext,"mid")==0))
		{
			ismidi=true;
		}
		else
		{
			return D_O_K;
		}
		
		zc_stop_midi();
		
		if(zcmusic != NULL)
		{
			zcmusic_stop(zcmusic);
			zcmusic_unload_file(zcmusic);
			zcmusic = NULL;
			zcmixer->newtrack = NULL;
		}
		
		if(ismidi)
		{
			packfile_password("");
			if((song=load_midi(midipath))!=NULL)
			{
				if(zc_play_midi(song,true)==0)
				{
					media_menu.select_uid(MENUID_MEDIA_TUNES, false);
					media_menu.select_uid(MENUID_MEDIA_PLAYMUSIC, true);
					disable_hotkey(ZQKEY_AMBIENT_MUSIC, false);
					disable_hotkey(ZQKEY_PLAY_MUSIC, false);
					
					media_menu.disable_uid(MENUID_MEDIA_CHANGETRACK, true);
					disable_hotkey(ZQKEY_CHANGE_TRACK, true);
				}
			}
		}
		else
		{
			gme_track=0;
			zcmusic = (ZCMUSIC*)zcmusic_load_file(midipath);
			
			if(zcmusic!=NULL)
			{
				media_menu.select_uid(MENUID_MEDIA_TUNES, false);
				media_menu.select_uid(MENUID_MEDIA_PLAYMUSIC, true);
				disable_hotkey(ZQKEY_AMBIENT_MUSIC, false);
				disable_hotkey(ZQKEY_PLAY_MUSIC, false);
				
				bool distrack = zcmusic_get_tracks(zcmusic)<2;
				media_menu.disable_uid(MENUID_MEDIA_CHANGETRACK, distrack);
				disable_hotkey(ZQKEY_CHANGE_TRACK, distrack);
					
				zcmusic_play(zcmusic, midi_volume);
			}
		}
	}
	
	return D_O_K;
}

int32_t playZCForever()
{
	stopMusic();

	zcmusic = zcmusic_load_file("assets/zc/ZC_Forever_HD.mp3");
	if (zcmusic)
	{
		zcmusic_play(zcmusic, midi_volume);
		set_media_tunes();
	}
	return D_O_K;
}

// It took awhile to get these values right, so no meddlin'!
int32_t playTune1()
{
    return playTune(0);
}
int32_t playTune2()
{
    return playTune(81);
}
int32_t playTune3()
{
    return playTune(233);
}
int32_t playTune4()
{
    return playTune(553);
}
int32_t playTune5()
{
    return playTune(814);
}
int32_t playTune6()
{
    return playTune(985);
}
int32_t playTune7()
{
    return playTune(1153);
}
int32_t playTune8()
{
    return playTune(1333);
}
int32_t playTune9()
{
    return playTune(1556);
}
int32_t playTune10()
{
    return playTune(1801);
}
int32_t playTune11()
{
    return playTune(2069);
}
int32_t playTune12()
{
    return playTune(2189);
}
int32_t playTune13()
{
    return playTune(2569);
}
int32_t playTune14()
{
    return playTune(2753);
}
int32_t playTune15()
{
    return playTune(2856);
}
int32_t playTune16()
{
    return playTune(3042);
}
int32_t playTune17()
{
    return playTune(3125);
}
int32_t playTune18()
{
    return playTune(3217);
}
int32_t playTune19()
{
    return playTune(3296);
}

int32_t playTune(int32_t pos)
{
    zc_stop_midi();
    
    if(zcmusic != NULL)
    {
        zcmusic_stop(zcmusic);
        zcmusic_unload_file(zcmusic);
        zcmusic = NULL;
		zcmixer->newtrack = NULL;
    }
    
    if(zc_play_midi(asset_tunes_midi,true)==0)
    {
        zc_midi_seek(pos);
        set_media_tunes();
    }
    
    return D_O_K;
}

int32_t stopMusic()
{
    zc_stop_midi();
    
    if(zcmusic != NULL)
    {
        zcmusic_stop(zcmusic);
        zcmusic_unload_file(zcmusic);
        zcmusic = NULL;
		zcmixer->newtrack = NULL;
    }
    
	media_menu.select_uid(MENUID_MEDIA_TUNES, false);
	media_menu.select_uid(MENUID_MEDIA_PLAYMUSIC, false);
	disable_hotkey(ZQKEY_AMBIENT_MUSIC, false);
	disable_hotkey(ZQKEY_PLAY_MUSIC, false);
	
    media_menu.disable_uid(MENUID_MEDIA_CHANGETRACK, true);
	disable_hotkey(ZQKEY_CHANGE_TRACK, true);
    return D_O_K;
}

static int32_t gamemisc1_list[] =
{
	5,6,7,8,
	9,10,11,12,
	
	37,38,39,40,
	41,42,43,44,
	
	71,72,73,74,
	75,76,77,78,
	
	-1
};

static int32_t gamemisc2_list[] =
{
	13,14,15,16,
	17,18,19,20,
	
	45,46,47,48,
	49,50,51,52,
	
	79,80,81,82,
	83,84,85,86,
	
	-1
};

static int32_t gamemisc3_list[] =
{
    21,22,23,24,
	25,26,27,28,
	
	53,54,55,56,
	57,58,59,60,
	
	87,88,89,90,
	91,92,93,94,
	
	-1
};

static int32_t gamemisc4_list[] =
{
	29,30,31,32,
	33,34,35,36,
	
	61,62,63,64,
	65,66,67,68,
	
	95,96,97,98,
	99,100,101,102,
	
	-1
};

static TABPANEL gamemisc_tabs[] =
{
    // (text)
    { (char *)" Misc[0-7] ",     D_SELECTED, gamemisc1_list, 0, NULL },
    { (char *)" Misc[8-15] ",     0,          gamemisc2_list, 0, NULL },
    { (char *)" Misc[16-23] ",     0,          gamemisc3_list, 0, NULL },
    { (char *)" Misc[24-31] ",     0,          gamemisc4_list, 0, NULL },
    { NULL,              0,          NULL,            0, NULL }
};

//to do: Make string boxes larger, and split into two tabs. 
static DIALOG gamemiscarray_dlg[] =
{
	// (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
	
	{ jwin_win_proc,       0,   10,  310,  224,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       (void *) "Game->Misc[]", NULL, NULL },
	{ d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
	{ jwin_tab_proc,         3,   26,   304,  174,    vc(14),   vc(1),      0,      0,          1,             0, (void *) gamemisc_tabs,	NULL, (void *)gamemiscarray_dlg },
	{  d_dummy_proc,           240,    144,     40,      8,    vc(14),                 vc(1),                   0,    0,           0,    0,  NULL,													       NULL,   NULL                 },
	{  d_dummy_proc,           240,    144,     40,      8,    vc(14),                 vc(1),                   0,    0,           0,    0,  NULL,													       NULL,   NULL                 },
	
	//5
	{ jwin_edit_proc,      8,   42,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+20,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+40,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//8
	{ jwin_edit_proc,      8,   42+60,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+80,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+100,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+120,   100-12,    16, vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+140,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//13
	{ jwin_edit_proc,     8,   42,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+20,   100-12,    16, vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+40,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+60,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+80,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//18
	{ jwin_edit_proc,     8,   42+100,   100-12,    16, vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+120,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+140,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42,   100-12,    16, vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+20,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//23
	{ jwin_edit_proc,      8,   42+40,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+60,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+80,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+100,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,      8,   42+120,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//28
	{ jwin_edit_proc,      8,   42+140,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+20,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+40,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+60,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//33
	{ jwin_edit_proc,     8,   42+80,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+100,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+120,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	{ jwin_edit_proc,     8,   42+140,   100-12,    16,  vc(12),   vc(1),   0,       0,          64,             0,       NULL, NULL, NULL },
	//37
	{ jwin_numedit_swap_zsint_proc,     96,  42,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+20,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//39
	{ jwin_numedit_swap_zsint_proc,     96,  42+40,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+60,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+80,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+100,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+120,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//44
	{ jwin_numedit_swap_zsint_proc,     96,  42+140,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+20,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	
	{ jwin_numedit_swap_zsint_proc,     96,  42+40,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+60,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//49
	{ jwin_numedit_swap_zsint_proc,     96,  42+80,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+100,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+120,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+140,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//54
	{ jwin_numedit_swap_zsint_proc,     96,  42+20,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+40,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+60,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+80,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+100,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//59
	{ jwin_numedit_swap_zsint_proc,     96,  42+120,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+140,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+20,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+40,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//64
	{ jwin_numedit_swap_zsint_proc,     96,  42+60,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+80,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+100,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+120,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	{ jwin_numedit_swap_zsint_proc,     96,  42+140,   60,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
	//69
	{ jwin_button_proc,       70,    204,     61,     21,    vc(14),                 vc(1),                  13,       D_EXIT,      0,    0, (void *) "OK",                                  NULL,   NULL                  },
	{ jwin_button_proc,      170,    204,     61,     21,    vc(14),                 vc(1),                  27,       D_EXIT,      0,    0, (void *) "Cancel",                              NULL,   NULL                  },
	
	//71
	{ jwin_swapbtn_proc,    156,    42,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    62,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    82,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   102,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   122,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   142,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   162,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   182,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	//79
	{ jwin_swapbtn_proc,    156,    42,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    62,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    82,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   102,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   122,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   142,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   162,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   182,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	//87
	{ jwin_swapbtn_proc,    156,    42,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    62,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    82,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   102,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   122,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   142,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   162,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   182,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	//95
	{ jwin_swapbtn_proc,    156,    42,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    62,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,    82,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   102,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   122,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   142,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   162,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	{ jwin_swapbtn_proc,    156,   182,    16,    16,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
	
	{ NULL,                0,    0,    0,    0,  0,                   0,                      0,      0,          0,             0,       NULL,                           NULL,  NULL }
};

//  +----------+
//  |          |
//  | View Pic |
//  |          |
//  |          |
//  |          |
//  +----------+

BITMAP *pic=NULL;
BITMAP *bmap=NULL;
PALETTE picpal;
PALETTE mappal;
int32_t  picx=0,picy=0,mapx=0,mapy=0,pblack,pwhite;

double picscale=1.0,mapscale=1.0;
bool vp_showpal=true, vp_showsize=true, vp_center=true;

//INLINE int32_t pal_sum(RGB p) { return p.r + p.g + p.b; }

void get_bw(RGB *pal,int32_t &black,int32_t &white)
{
    black=white=1;
    
    for(int32_t i=1; i<256; i++)
    {
        if(pal_sum(pal[i])<pal_sum(pal[black]))
            black=i;
            
        if(pal_sum(pal[i])>pal_sum(pal[white]))
            white=i;
    }
}

void draw_bw_mouse(int32_t white, int32_t old_mouse, int32_t new_mouse)
{
    blit(mouse_bmp[old_mouse][0],mouse_bmp[new_mouse][0],0,0,0,0,16,16);
    
    for(int32_t y=0; y<16; y++)
    {
        for(int32_t x=0; x<16; x++)
        {
            if(getpixel(mouse_bmp[new_mouse][0],x,y)!=0)
            {
                putpixel(mouse_bmp[new_mouse][0],x,y,white);
            }
        }
    }
}

int32_t load_the_pic(BITMAP **dst, PALETTE dstpal)
{
    PALETTE temppal;
    
    for(int32_t i=0; i<256; i++)
    {
        temppal[i]=dstpal[i];
        dstpal[i]=RAMpal[i];
    }
    
    // set up the new palette
    for(int32_t i=0; i<64; i++)
    {
        dstpal[i].r = i;
        dstpal[i].g = i;
        dstpal[i].b = i;
    }
    
    zc_set_palette(dstpal);
    
    BITMAP *graypic = create_bitmap_ex(8,screen->w,screen->h);
    int32_t _w = screen->w-1;
    int32_t _h = screen->h-1;
    
    // gray scale the current frame
    for(int32_t y=0; y<_h; y++)
    {
        for(int32_t x=0; x<_w; x++)
        {
            int32_t c = screen->line[y][x];
            int32_t gray = zc_min((temppal[c].r*42 + temppal[c].g*75 + temppal[c].b*14) >> 7, 63);
            graypic->line[y][x] = gray;
        }
    }
    
    blit(graypic,screen,0,0,0,0,screen->w,screen->h);
    destroy_bitmap(graypic);
#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wformat-overflow"
#endif
    char extbuf[2][80];
    memset(extbuf[0],0,80);
    memset(extbuf[1],0,80);
    sprintf(extbuf[0], "View Image (%s", snapshotformat_str[0][1]);
    strcpy(extbuf[1], snapshotformat_str[0][1]);
    
    for(int32_t i=1; i<ssfmtMAX; ++i)
    {
        sprintf(extbuf[0], "%s, %s", extbuf[0], snapshotformat_str[i][1]);
        sprintf(extbuf[1], "%s;%s", extbuf[1], snapshotformat_str[i][1]);
    }
    
    sprintf(extbuf[0], "%s)", extbuf[0]);
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif
    
    int32_t gotit = prompt_for_existing_file_compat(extbuf[0],extbuf[1],NULL,imagepath,true);
    
    if(!gotit)
    {
        zc_set_palette(temppal);
        get_palette(dstpal);
        return 1;
    }
    
    strcpy(imagepath,temppath);
    
    if(*dst)
    {
        destroy_bitmap(*dst);
    }
    
    for(int32_t i=0; i<256; i++)
    {
        dstpal[i].r = 0;
        dstpal[i].g = 0;
        dstpal[i].b = 0;
    }
    
    *dst = load_bitmap(imagepath,picpal);
    
    if(!*dst)
    {
        jwin_alert("Error","Error loading image:",imagepath,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
        return 2;
    }
    
    //  get_bw(picpal,pblack,pwhite);
    //  draw_bw_mouse(pwhite);
    //  gui_bg_color = pblack;
    //  gui_fg_color = pwhite;
    
    if(vp_center)
    {
        picx=picy=0;
    }
    else
    {
        picx=(*dst)->w-zq_screen_w;
        picy=(*dst)->h-zq_screen_h;
    }
    
    return 0;
}
int load_the_pic_new(BITMAP **dst, PALETTE dstpal)
{
#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wformat-overflow"
#endif
    char extbuf[2][80];
    memset(extbuf[0],0,80);
    memset(extbuf[1],0,80);
    sprintf(extbuf[0], "View Image (%s", snapshotformat_str[0][1]);
    strcpy(extbuf[1], snapshotformat_str[0][1]);
    
    for(int32_t i=1; i<ssfmtMAX; ++i)
    {
        sprintf(extbuf[0], "%s, %s", extbuf[0], snapshotformat_str[i][1]);
        sprintf(extbuf[1], "%s;%s", extbuf[1], snapshotformat_str[i][1]);
    }
    
    sprintf(extbuf[0], "%s)", extbuf[0]);
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif
    
    int32_t gotit = prompt_for_existing_file_compat(extbuf[0],extbuf[1],NULL,imagepath,true);
    
    if(!gotit)
        return 1;
    
    strcpy(imagepath,temppath);
    
    if(*dst)
        destroy_bitmap(*dst);
    
    for(int32_t i=0; i<256; i++)
    {
        dstpal[i].r = 0;
        dstpal[i].g = 0;
        dstpal[i].b = 0;
    }
    
    *dst = load_bitmap(imagepath,dstpal);
    
    if(!*dst)
    {
        jwin_alert("Error","Error loading image:",imagepath,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
        return 2;
    }
    
    if(vp_center)
    {
        picx=picy=0;
    }
    else
    {
        picx=(*dst)->w-zq_screen_w;
        picy=(*dst)->h-zq_screen_h;
    }
    
    return 0;
}

int32_t saveMapAsImage(ALLEGRO_BITMAP* bitmap)
{
    char buf[200];
    int32_t num=0;
    
    do
    {
        snprintf(buf, 200, "%szquest_map%05d.%s", get_snap_str(), ++num, snapshotformat_str[SnapshotFormat][1]);
        buf[199]='\0';
    }
    while(num<99999 && exists(buf));
    
	if (!al_save_bitmap(buf, bitmap))
		InfoDialog("Error", "Failed to save map image").show();
    
    return D_O_K;
}

int32_t onViewPic()
{
    return launchPicViewer(&pic,picpal,&picx,&picy,&picscale,false);
}


class MapViewRTI : public RenderTreeItem
{
public:
	MapViewRTI(): RenderTreeItem("map_view")
	{
	}

	int bw, bh, sw, sh, flags;

private:
	void render(bool bitmap_resized)
	{
		MapCursor previous_cursor = Map.getCursor();
		Map.setViewSize(1);

		BITMAP* bmap4_single = create_bitmap_ex(8,256,176);
		set_bitmap_create_flags(true);
		ALLEGRO_BITMAP* bmap5_single = al_create_bitmap(256,176);
		for(int32_t y=0; y<8; y++)
		{
			for(int32_t x=0; x<16; x++)
			{
				clear_bitmap(bmap4_single);
				Map.setCurrScr(y*16+x);
				Map.draw(bmap4_single, 0, 0, flags, -1, y*16+x, -1);
				stretch_blit(bmap4_single, bmap4_single, 0, 0, 0, 0, 256, 176, 256, 176);
				all_render_a5_bitmap(bmap4_single, bmap5_single);
				al_draw_scaled_bitmap(bmap5_single, 0, 0, 256, 176, sw * x, sh * y, sw, sh, 0);
			}
		}

		Map.setCursor(previous_cursor);
		destroy_bitmap(bmap4_single);
		al_destroy_bitmap(bmap5_single);
	}
};
static MapViewRTI rti_map_view;

int32_t launchPicViewer(BITMAP **pictoview, PALETTE pal, int32_t *px2, int32_t *py2, double *scale2, bool isviewingmap, bool skipmenu)
{
	restore_mouse();
	BITMAP *buf;
	bool done=false, redraw=true;
	
	popup_zqdialog_start();
	
	// Always call load_the_map() when viewing the map.
	if((!*pictoview || isviewingmap) && (isviewingmap ? load_the_map(skipmenu) : load_the_pic(pictoview,pal)))
	{
		zc_set_palette(RAMpal);
		popup_zqdialog_end();
		close_the_map();
		return D_O_K;
	}
	
	get_bw(pal,pblack,pwhite);
	
	int32_t oldfgcolor = gui_fg_color;
	int32_t oldbgcolor = gui_bg_color;
	
	buf = create_bitmap_ex(8,zq_screen_w,zq_screen_h);
	
	if(!buf)
	{
		jwin_alert("Error","Error creating temp bitmap",NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
		popup_zqdialog_end();
		close_the_map();
		return D_O_K;
	}

	static LegacyBitmapRTI viewer_overlay_rti("viewer_overlay");
	viewer_overlay_rti.set_size(buf->w, buf->h);
	viewer_overlay_rti.a4_bitmap = buf;
	viewer_overlay_rti.transparency_index = 15;
	get_root_rti()->add_child(&viewer_overlay_rti);
	
	zc_set_palette(pal);

	if(isviewingmap)
	{
		int sw = rti_map_view.width / 16;
		int sh = rti_map_view.height / 8;
		int scr = Map.getCurrScr();
		if (scr >= 0x00 && scr <= 0x7F)
		{
			int dw = al_get_display_width(all_get_display()) / get_root_rti()->get_transform().xscale;
			int dh = al_get_display_height(all_get_display()) / get_root_rti()->get_transform().yscale;
			mapx = (-(scr % 16) * sw - sw/2 + dw/2);
			mapy = (-(scr / 16) * sh - sh/2 + dh/2);
		}
	}

	int w, h;
	if (isviewingmap)
	{
		w = rti_map_view.width;
		h = rti_map_view.height;
	}
	else
	{
		w = (*pictoview)->w;
		h = (*pictoview)->h;
	}

	do
	{
		if (isviewingmap)
		{
			float scale = *scale2;
			int dw = al_get_display_width(all_get_display()) / get_root_rti()->get_transform().xscale;
			int dh = al_get_display_height(all_get_display()) / get_root_rti()->get_transform().yscale;
			mapx = std::max(mapx, (int)(-w*scale + dw));
			mapy = std::max(mapy, (int)(-h*scale + dh));
			mapx = std::min(mapx, 0);
			mapy = std::min(mapy, 0);
			rti_map_view.set_transform({mapx, mapy, scale, scale});
		}

		if(redraw)
		{
			clear_to_color(buf,15);

			if (!isviewingmap)
				stretch_blit(*pictoview, buf, 0, 0, w, h,
					int32_t(zq_screen_w + (*px2 - w) * *scale2) / 2, int32_t(zq_screen_h + (*py2 - h) * *scale2) / 2,
					int32_t(w * *scale2), int32_t(h * *scale2));
						 
			if(vp_showpal)
				for(int32_t i=0; i<256; i++)
					rectfill(buf,((i&15)<<2)+zq_screen_w-64,((i>>4)<<2)+zq_screen_h-64,((i&15)<<2)+zq_screen_w-64+3,((i>>4)<<2)+zq_screen_h-64+3,i);
					
			if(vp_showsize)
			{
				textprintf_ex(buf,font,0,zq_screen_h-8,pwhite,pblack,"%dx%d %.2f%%",w,h,*scale2*100.0);
			}
			
			if (!isviewingmap)
				blit(buf,screen,0,0,0,0,zq_screen_w,zq_screen_h);
			redraw=false;
		}
		
		custom_vsync();
		
		int32_t step = 16;
		
		if(*scale2 < 1.0)
			step = int32_t(4.0/ *scale2);
			
		if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
			step <<= 2;
			
		if(CHECK_CTRL_CMD)
			step >>= 1;
			
		if(key[KEY_UP])
		{
			*py2+=step;
			redraw=true;
		}
		
		if(key[KEY_DOWN])
		{
			*py2-=step;
			redraw=true;
		}
		
		if(key[KEY_LEFT])
		{
			*px2+=step;
			redraw=true;
		}
		
		if(key[KEY_RIGHT])
		{
			*px2-=step;
			redraw=true;
		}
		
		if(keypressed() && !redraw)
			switch(readkey()>>8)
			{
			case KEY_PGUP:
				*scale2*=0.95;
				
				if(*scale2<0.1) *scale2=0.1;
				
				redraw=true;
				break;
				
			case KEY_PGDN:
				*scale2/=0.95;
				
				if(*scale2>5.0) *scale2=5.0;
				
				redraw=true;
				break;
				
			case KEY_HOME:
				*scale2/=2.0;
				
				if(*scale2<0.1) *scale2=0.1;
				
				redraw=true;
				break;
				
			case KEY_END:
				*scale2*=2.0;
				
				if(*scale2>5.0) *scale2=5.0;
				
				redraw=true;
				break;
				
			case KEY_TILDE:
				*scale2=0.5;
				redraw=true;
				break;
				
			case KEY_Z:
				*px2=w-zq_screen_w;
				*py2=h-zq_screen_h;
				vp_center=false;
				redraw=true;
				break;
				
			case KEY_1:
				*scale2=1.0;
				redraw=true;
				break;
				
			case KEY_2:
				*scale2=2.0;
				redraw=true;
				break;
				
			case KEY_3:
				*scale2=3.0;
				redraw=true;
				break;
				
			case KEY_4:
				*scale2=4.0;
				redraw=true;
				break;
				
			case KEY_5:
				*scale2=5.0;
				redraw=true;
				break;
				
			case KEY_C:
				*px2=*py2=0;
				redraw=vp_center=true;
				break;
				
			case KEY_S:
				vp_showsize = !vp_showsize;
				redraw=true;
				break;
				
			case KEY_D:
				vp_showpal = !vp_showpal;
				redraw=true;
				break;
				
			case KEY_P:
				if(isviewingmap) break;
				
			case KEY_ESC:
				done=true;
				break;
				
			case KEY_SPACE:
				close_the_map();
				// TODO: why is `load_the_map` rendering a black dialog?
				if(isviewingmap ? load_the_map(skipmenu) : load_the_pic(pictoview,pal)==2)
				{
					done=true;
				}
				else
				{
					redraw=true;
					gui_bg_color = pblack;
					gui_fg_color = pwhite;
					*scale2=1.0;
					zc_set_palette(pal);
				}
				
				get_bw(pal,pblack,pwhite);
				break;
			}
	}
	while(!done);
	
	destroy_bitmap(buf);
	zc_set_palette(RAMpal);
	gui_fg_color = oldfgcolor;
	gui_bg_color = oldbgcolor;
	
	popup_zqdialog_end();
	position_mouse_z(0);
	viewer_overlay_rti.remove();
	close_the_map();
	return D_O_K;
}

static DIALOG loadmap_dlg[] =
{
    // (dialog proc)         (x)    (y)     (w)     (h)     (fg)        (bg)    (key)     (flags)  (d1)  (d2)   (dp)                                 (dp2)   (dp3)
    {  jwin_win_proc,          0,     0,    225,    143,    vc(14),     vc(1),      0,    D_EXIT,     0,    0, (void *) "View Map",                 NULL,   NULL  },
    {  d_timer_proc,           0,     0,      0,      0,    0,          0,          0,    0,          0,    0,  NULL,                                NULL,   NULL  },
    {  jwin_text_proc,        32,    26,     96,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Resolution",               NULL,   NULL  },
    // 3
    {  jwin_radio_proc,       16,    36,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "1/4 - 1024x352",		   NULL,   NULL  },
    {  jwin_radio_proc,       16,    46,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "1/2 - 2048x704",		   NULL,   NULL  },
    {  jwin_radio_proc,       16,    56,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "Full - 4096x1408",		   NULL,   NULL  },
    {  jwin_text_proc,       144,    26,     97,      9,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Options",                  NULL,   NULL  },
    // 7
    {  jwin_check_proc,      144,    36,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Solidity",                     NULL,   NULL  },
    {  jwin_check_proc,      144,    46,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Flags",                    NULL,   NULL  },
    {  jwin_check_proc,      144,    56,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Dark",                     NULL,   NULL  },
    {  jwin_check_proc,      144,    66,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Items",                    NULL,   NULL  },
    // 11
    {  jwin_button_proc,      42,    110,     61,     21,    vc(14),     vc(1),     13,    D_EXIT,     0,    0, (void *) "OK",                       NULL,   NULL  },
    {  jwin_button_proc,     122,    110,     61,     21,    vc(14),     vc(1),     27,    D_EXIT,     0,    0, (void *) "Cancel",                   NULL,   NULL  },
    {  jwin_check_proc,       16,    88,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Save to Image",  NULL,   NULL  },
	// 14
	{  jwin_radio_proc,       16,    66,     97,      9,    vc(14),     vc(1),       0,    0,          0,    0, (void*)"2x  - 8192x2816",		   NULL,   NULL  },
	{  jwin_radio_proc,       16,    76,     97,      9,    vc(14),     vc(1),       0,    0,          0,    0, (void*)"4x  - 16384x5632",		   NULL,   NULL  },
	 {  NULL,                   0,     0,      0,      0,    0,          0,          0,    0,          0,    0,  NULL,                                NULL,   NULL  }
};

int32_t load_the_map(bool skipmenu)
{
    static int32_t res = 0;
    static int32_t flags = cDEBUG;
    
    loadmap_dlg[0].dp2    = get_zc_font(font_lfont);
    loadmap_dlg[3].flags  = (res==2) ? D_SELECTED : 0;
    loadmap_dlg[4].flags  = (res==1) ? D_SELECTED : 0;
    loadmap_dlg[5].flags  = (res==0) ? D_SELECTED : 0;
    loadmap_dlg[7].flags  = (flags&cWALK)   ? D_SELECTED : 0;
    loadmap_dlg[8].flags  = (flags&cFLAGS)  ? D_SELECTED : 0;
    loadmap_dlg[9].flags  = (flags&cNODARK) ? 0 : D_SELECTED;
    loadmap_dlg[10].flags = (flags&cNOITEM) ? 0 : D_SELECTED;
    loadmap_dlg[13].flags = 0;
	loadmap_dlg[14].flags = (res==3) ? D_SELECTED : 0;
	loadmap_dlg[15].flags = (res==4) ? D_SELECTED : 0;
    
	if(!skipmenu)
	{
		large_dialog(loadmap_dlg);

		if (do_zqdialog(loadmap_dlg, 11) != 11)
		{
			return 1;
		}
    
		flags = cDEBUG;
    
		if(loadmap_dlg[3].flags&D_SELECTED)  res=2;
    
		if(loadmap_dlg[4].flags&D_SELECTED)  res=1;
    
		if(loadmap_dlg[5].flags&D_SELECTED)  res=0;
    
		if(loadmap_dlg[7].flags&D_SELECTED)  flags|=cWALK;
    
		if(loadmap_dlg[8].flags&D_SELECTED)  flags|=cFLAGS;
    
		if(!(loadmap_dlg[9].flags&D_SELECTED))  flags|=cNODARK;
    
		if(!(loadmap_dlg[10].flags&D_SELECTED)) flags|=cNOITEM;

		if(loadmap_dlg[14].flags&D_SELECTED) res=3;

		if(loadmap_dlg[15].flags&D_SELECTED) res=4;
    }
    
    int32_t bw = (256*16)>>res;
	int32_t bh = (176*8)>>res;
	int32_t sw = 256>>res;
	int32_t sh = 176>>res;
	if(res>2)
	{
		bw = (256*16)<<(res-2);
		bh = (176*8)<<(res-2);
		sw = 256<<(res-2);
		sh = 176<<(res-2);
	}

	rti_map_view.flags = flags;
	rti_map_view.bw = bw;
	rti_map_view.bh = bh;
	rti_map_view.sw = sw;
	rti_map_view.sh = sh;
	rti_map_view.set_size(bw, bh);
	rti_map_view.dirty = true;
	get_root_rti()->add_child(&rti_map_view);
	render_zq();

    vp_showpal = false;
    get_bw(picpal,pblack,pwhite);
    mapx = mapy = 0;
    mapscale = 1;
    imagepath[0] = 0;

    if(loadmap_dlg[13].flags & D_SELECTED) saveMapAsImage(rti_map_view.bitmap);

	memcpy(mappal,RAMpal,sizeof(RAMpal));

    return 0;
}

void close_the_map()
{
	rti_map_view.remove();
}

int32_t onViewMap()
{
	return onViewMapEx(false);
}
int32_t onViewMapEx(bool skipmenu)
{
    int32_t temp_aligns=ShowMisalignments;
    ShowMisalignments=0;
    //if(load_the_map()==0)
    //{
    launchPicViewer(&bmap,mappal,&mapx, &mapy, &mapscale,true,skipmenu);
    //}
    ShowMisalignments=temp_aligns;
    return D_O_K;
}

static const char *mazedirstr[4] = {"North","South","West","East"};
char _pathstr[40]="North,North,North,North";

char *pathstr(byte path[])
{
    sprintf(_pathstr,"%s,%s,%s,%s",mazedirstr[path[0]],mazedirstr[path[1]],
            mazedirstr[path[2]],mazedirstr[path[3]]);
    return _pathstr;
}

char _ticksstr[32]="99.99 seconds";

char *ticksstr(int32_t tics)
{
    int32_t mins=tics/(60*60);
    tics=tics-(mins*60*60);
    int32_t secs=tics/60;
    tics=tics-(secs*60);
    tics=tics*100/60;
    
    if(mins>0)
    {
        sprintf(_ticksstr,"%d:%02d.%02d",mins, secs, tics);
    }
    else
    {
        sprintf(_ticksstr,"%d.%02d seconds",secs, tics);
    }
    
    return _ticksstr;
}
void textprintf_disabled(BITMAP *bmp, AL_CONST FONT *f, int32_t x, int32_t y, int32_t color_hl, int32_t color_sh, AL_CONST char *format, ...)
{
    char buf[512];
    va_list ap;
    ASSERT(bmp);
    ASSERT(f);
    ASSERT(format);
    
    va_start(ap, format);
    uvszprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    
    
    textout_ex(bmp, f, buf, x+1, y+1, color_hl, -1);
    
    textout_ex(bmp, f, buf, x, y, color_sh, -1);
}

void textprintf_centre_disabled(BITMAP *bmp, AL_CONST FONT *f, int32_t x, int32_t y, int32_t color_hl, int32_t color_sh, AL_CONST char *format, ...)
{
    char buf[512];
    va_list ap;
    ASSERT(bmp);
    ASSERT(f);
    ASSERT(format);
    
    va_start(ap, format);
    uvszprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    
    textout_centre_ex(bmp, f, buf, x+1, y+1, color_hl, -1);
    textout_centre_ex(bmp, f, buf, x, y, color_sh, -1);
}

void draw_sqr_frame(size_and_pos const& sqr)
{
	jwin_draw_frame(menu1,sqr.x,sqr.y,sqr.tw(),sqr.th(),FR_DEEP);
}
void draw_sqr_icon(size_and_pos const& sqr, BITMAP* icon)
{
	stretch_blit(icon, menu1, 0, 0, 16, 16, sqr.x+2, sqr.y+2, sqr.tw()-4, sqr.th()-4);
}
void draw_sqr_nums(size_and_pos const& sqr, FONT* f, bool center, int num)
{
	if(center)
		textprintf_centre_ex(menu1,f,sqr.x+txtoffs_single.x,sqr.y+txtoffs_single.y,jwin_pal[jcBOXFG],-1,"%d",num);
	else
		textprintf_ex(menu1,f,sqr.x+txtoffs_single.x,sqr.y+txtoffs_single.y,jwin_pal[jcBOXFG],-1,"%d",num);
}
void draw_sqr_nums(size_and_pos const& sqr, FONT* f, bool center, int num1, int num2)
{
	if(center)
	{
		textprintf_centre_ex(menu1,f,sqr.x+txtoffs_double_1.x,sqr.y+txtoffs_double_1.y,jwin_pal[jcBOXFG],-1,"%d",num1);
		textprintf_centre_ex(menu1,f,sqr.x+txtoffs_double_2.x,sqr.y+txtoffs_double_2.y,jwin_pal[jcBOXFG],-1,"%d",num2);
	}
	else
	{
		textprintf_ex(menu1,f,sqr.x+txtoffs_double_1.x,sqr.y+txtoffs_double_1.y,jwin_pal[jcBOXFG],-1,"%d",num1);
		textprintf_ex(menu1,f,sqr.x+txtoffs_double_2.x,sqr.y+txtoffs_double_2.y,jwin_pal[jcBOXFG],-1,"%d",num2);
	}
}
void draw_sqr_btn(size_and_pos const& sqr, const char* txt, int flags, FONT* f = nullptr)
{
	if(sqr.x < 0) return;
	FONT* tfont = font;
	if(f)
		font = f;
	draw_text_button(menu1, sqr.x, sqr.y, sqr.tw(), sqr.th(), txt, 0, 0, flags, true);
	font = tfont;
}
void draw_sqr_btn(size_and_pos const& sqr, int icon, int flags, FONT* f = nullptr)
{
	if(sqr.x < 0) return;
	FONT* tfont = font;
	if(f)
		font = f;
	draw_icon_button(menu1, sqr.x, sqr.y, sqr.tw(), sqr.th(), icon, 0, 0, flags, true);
	font = tfont;
}

void drawpanel()
{
	mapscr *scr=Map.CurrScr();
	int32_t NextCombo = combobuf[Combo].nextcombo;
	int32_t NextCSet = combobuf[Combo].nextcset;
	if(combobuf[Combo].animflags & AF_CYCLEUNDERCOMBO)
	{
		NextCombo = scr->undercombo;
		NextCSet = scr->undercset;
	}
	if(combobuf[Combo].animflags & AF_CYCLENOCSET)
		NextCSet = CSet;
	
	FONT* tfont = font;
	if(prv_mode)
	{
		jwin_draw_frame(menu1,0,preview_panel.y,preview_panel.x+preview_panel.w, preview_panel.h, FR_WIN);
		rectfill(menu1,preview_panel.x,preview_panel.y+2,preview_panel.x+preview_panel.w-3,preview_panel.y+preview_panel.h-3,jwin_pal[jcBOX]);
	}
	else
	{
		auto& sqr = main_panel;
		rectfill(menu1,sqr.x,sqr.y,sqr.x+sqr.w-1,sqr.y+sqr.h-1, jwin_pal[jcBOX]);
		refresh(rSCRMAP);
		jwin_draw_frame(menu1,sqr.x,sqr.y,sqr.w,sqr.h, FR_WIN);
		
		font = get_custom_font(CFONT_GUI);
		draw_sqr_btn(squarepanel_swap_btn, "SWP", 0);
		if(compact_square_panels)
		{
			textprintf_centre_ex(menu1,font,squarepanel_up_btn.cx(),squarepanel_up_btn.y-text_height(font)-2,jwin_pal[jcBOXFG],-1,"%d",compact_active_panel);
			draw_sqr_btn(squarepanel_up_btn, BTNICON_ARROW_UP, 0);
			draw_sqr_btn(squarepanel_down_btn, BTNICON_ARROW_DOWN, 0);
		}
		font = tfont;
		
		FONT* sqr_text_font = (is_compact && compact_square_panels) ? get_custom_font(CFONT_GUI) : font;
		//Item:
		if(itemsqr_pos.x > -1)
		{
			draw_sqr_frame(itemsqr_pos);
			if(scr->hasitem)
			{
				rectfill(menu1,itemsqr_pos.x+2,itemsqr_pos.y+2,itemsqr_pos.x+itemsqr_pos.tw()-3,itemsqr_pos.y+itemsqr_pos.th()-3,0);
				overtile16_scale(menu1, itemsbuf[scr->item].tile,itemsqr_pos.x+2,itemsqr_pos.y+2,itemsbuf[scr->item].csets&15,0,itemsqr_pos.tw()-4,itemsqr_pos.th()-4);
			}
			else draw_sqr_icon(itemsqr_pos, icon_bmp[0][coord_frame]);
			draw_sqr_nums(itemsqr_pos, sqr_text_font, panel_align == 1, scr->itemx, scr->itemy);
		}
		//Flag:
		if(flagsqr_pos.x > -1)
		{
			draw_sqr_frame(flagsqr_pos);
			draw_sqr_icon(flagsqr_pos,flag_bmp[Flag%16][coord_frame]);
			draw_sqr_nums(flagsqr_pos, sqr_text_font, panel_align == 1, Flag);
		}
		
		//Stairs:
		if(stairsqr_pos.x > -1)
		{
			draw_sqr_frame(stairsqr_pos);
			draw_sqr_icon(stairsqr_pos,icon_bmp[1][coord_frame]);
			draw_sqr_nums(stairsqr_pos, sqr_text_font, panel_align == 1, scr->stairx, scr->stairy);
		}
		
		//Green arrival square:
		bool disabled_arrival = get_qr(qr_NOARRIVALPOINT);
		if(warparrival_pos.x > -1)
		{
			draw_sqr_frame(warparrival_pos);
			BITMAP* icon = icon_bmp[2][coord_frame];
			if(disabled_arrival)
			{
				icon = create_bitmap_ex(8,16,16);
				blit(icon_bmp[2][0], icon, 0, 0, 0, 0, 16, 16);
				replColor(icon, 0xE7, 0xEA, 0xEA, false);
				replColor(icon, 0xE8, 0xE2, 0xE2, false);
			}
			
			draw_sqr_icon(warparrival_pos, icon);
			draw_sqr_nums(warparrival_pos, sqr_text_font, panel_align == 1, scr->warparrivalx, scr->warparrivaly);
			
			if(disabled_arrival)
				destroy_bitmap(icon);
		}
		
		//Blue return squares:
		for(int32_t i=0; i<4; i++)
		{
			if(warpret_pos[i].x < 0) continue;
			draw_sqr_frame(warpret_pos[i]);
			draw_sqr_icon(warpret_pos[i], icon_bmp[ICON_BMP_RETURN_A+i][coord_frame]);
			draw_sqr_nums(warpret_pos[i], sqr_text_font, panel_align == 1, scr->warpreturnx[i], scr->warpreturny[i]);
		}
		
		// Enemies
		auto& ep = enemy_prev_pos;
		if(ep.x > -1)
		{
			if(ep.fw > -1)
			{
				rectfill(menu1, ep.x, ep.y, ep.x+ep.tw()-1,ep.y+ep.th()-1,vc(0));
				rectfill(menu1, ep.x+ep.fw, ep.y+ep.fh, ep.x+ep.tw()-1, ep.y+ep.th()-1, jwin_pal[jcBOX]);
				jwin_draw_frag_frame(menu1, ep.x, ep.y, ep.tw(), ep.th(), ep.fw, ep.fh, FR_DEEP);
			}
			else
			{
				rectfill(menu1, ep.x, ep.y, ep.x+ep.tw()-1,ep.y+ep.th()-1,vc(0));
				draw_sqr_frame(ep);
			}
			for(int32_t i=0; i< 10 && Map.CurrScr()->enemy[i]!=0; i++)
			{
				int32_t id = Map.CurrScr()->enemy[i];
				int32_t tile = get_qr(qr_NEWENEMYTILES) ? guysbuf[id].e_tile : guysbuf[id].tile;
				int32_t cset = guysbuf[id].cset;
				auto& sqr = ep.subsquare(i);
				if(tile)
					overtile16_scale(menu1, tile+efrontfacingtile(id),sqr.x,sqr.y,cset,0,sqr.tw(),sqr.th());
			}
		}
	}
	font = tfont;
}

void show_screen_error(const char *str, int32_t i, int32_t c)
{
    rectfill(menu1, screrrorpos.x-text_length(get_zc_font(font_lfont_l),str),screrrorpos.y-(i*16),screrrorpos.x,screrrorpos.y-((i-1)*16)-4,vc(0));
    textout_shadowed_ex(menu1,get_zc_font(font_lfont_l), str,screrrorpos.x-text_length(get_zc_font(font_lfont_l),str),screrrorpos.y-(i*16),c,vc(0),-1);
}

void tile_warp_notification(int32_t which, char *buf)
{
    char letter = 'A'+which;
    
    switch(Map.CurrScr()->tilewarptype[which])
    {
    case wtCAVE:
        sprintf(buf,"Tile Warp %c: Cave/Item Cellar",letter);
        break;
        
    default:
    {
        char buf2[30];
        
        if(strlen(DMaps[Map.CurrScr()->tilewarpdmap[which]].name)==0)
        {
            sprintf(buf2,"%d",Map.CurrScr()->tilewarpdmap[which]);
        }
        else
            sprintf(buf2,"%d-%s",Map.CurrScr()->tilewarpdmap[which],DMaps[Map.CurrScr()->tilewarpdmap[which]].name);
            
        sprintf(buf,"Tile Warp %c: %s, %02X", letter, buf2, Map.CurrScr()->tilewarpscr[which]);
        break;
    }
    
    case wtNOWARP:
        sprintf(buf,"Tile Warp %c: Cancel Warp", letter);
        break;
    }
}

void side_warp_notification(int32_t which, int32_t dir, char *buf)
{
    char letter = 'A'+which;
    char buf3[16];
    
    if(dir==0 && Map.CurrScr()->timedwarptics)
        sprintf(buf3,"%s, Timed",mazedirstr[dir]);
    else if(dir==4)
        sprintf(buf3,"Timed");
    else
        strcpy(buf3, mazedirstr[dir]);
        
    switch(Map.CurrScr()->sidewarptype[which])
    {
    case wtCAVE:
        sprintf(buf,"Side Warp %c (%s): Cave/Item Cellar",letter, buf3);
        break;
        
    default:
    {
        // Destination DMap name
        if(strlen(DMaps[Map.CurrScr()->sidewarpdmap[which]].name)==0)
        {
            sprintf(buf,"Side Warp %c (%s): %d, %02X", letter, buf3, Map.CurrScr()->sidewarpdmap[which], Map.CurrScr()->sidewarpscr[which]);
        }
        else
            sprintf(buf,"Side Warp %c (%s): %d-%s, %02X", letter, buf3, Map.CurrScr()->sidewarpdmap[which],DMaps[Map.CurrScr()->sidewarpdmap[which]].name, Map.CurrScr()->sidewarpscr[which]);
            
        break;
    }
    
    case wtNOWARP:
        sprintf(buf,"Side Warp %c (%s): Cancel Warp", letter, buf3);
        break;
    }
}

static bool arrowcursor = true; // Used by combo aliases and Combo Brush cursors. -L

void xout(BITMAP* dest, int x, int y, int x2, int y2, int c, int bgc = -1)
{
	//BG Fill
	if(bgc > -1)
		rectfill(dest, x, y, x2, y2, bgc);
	++x; ++y; --x2; --y2;
	//Border
	safe_rect(dest, x, y, x2, y2, c);
	//line(dest, x, y, x2, y, c);
	//line(dest, x, y, x, y2, c);
	//X
	line(dest, x, y, x2, y2, c);
	line(dest, x, y2, x2, y, c);
}

void put_autocombo_engravings(BITMAP* dest, combo_auto const& ca, bool selected, int32_t x, int32_t y, int32_t scale)
{
	if (!ca.valid())
	{
		if (ca.getDisplay() > 0)
			put_engraving(dest, x, y, 15, scale);
	}
	else
	{
		if (ca.getType() == AUTOCOMBO_Z4 || ca.getType() == AUTOCOMBO_DOR)
		{
			byte hei = vbound(ca.getArg() + 1, 1, 9);
			if (selected)
				hei = vbound(cauto_height, 1, 9);
			put_engraving(dest, x, y, 15 - hei, scale);
		}
	}
}

void draw_screenunit_map_screen(VisibleScreen visible_screen)
{
	int num_screens_to_draw = Map.getViewSize();
	int screen = visible_screen.screen;
	int xoff = visible_screen.xoff;
	int yoff = visible_screen.yoff;

	mapscr* scr = visible_screen.scr;
	if (!layers_valid(scr))
		fix_layers(scr, true);

	clear_to_color(mapscreenbmp, jwin_pal[jcBOX]);
	if (LayerDitherBG > -1)
	{
		if (LayerDitherSz > 0)
			ditherblit(mapscreenbmp, nullptr, vc(LayerDitherBG), dithChecker, LayerDitherSz);
		else
			clear_to_color(mapscreenbmp, vc(LayerDitherBG));
	}

	int view_scr_x = Map.getViewScr() % 16;
	int view_scr_y = Map.getViewScr() / 16;
	int scr_x = screen % 16;
	int scr_y = screen / 16;
	int edge_xoff = 0, edge_yoff = 0;
	if(showedges)
	{
		if (scr_x == view_scr_x)
			edge_xoff = 16;
		else
			xoff -= 16;
		
		if (scr_y == view_scr_y)
			edge_yoff = 16;
		else
			yoff -= 16;
	}

	combotile_add_x = mapscreen_x + xoff;
	combotile_add_y = mapscreen_y + yoff;
	combotile_mul_x = mapscreen_single_scale;
	combotile_mul_y = mapscreen_single_scale;
	Map.draw(mapscreenbmp, scr_x == view_scr_x && showedges ? 16 : 0, scr_y == view_scr_y && showedges ? 16 : 0, Flags, Map.getCurrMap(), screen, ActiveLayerHighlight ? CurrentLayer : -1);
	combotile_add_x = 0;
	combotile_add_y = 0;
	combotile_mul_x = 1;
	combotile_mul_y = 1;

	// TODO: should be better to move this out of draw_screenunit_map_screen.
	if (showedges && screen < 128)
	{
		bool peek_above = scr_y == view_scr_y;
		bool peek_below = scr_y == view_scr_y + num_screens_to_draw - 1;
		bool peek_left = scr_x == view_scr_x;
		bool peek_right = scr_x == view_scr_x + num_screens_to_draw - 1;

		int right_col = 272 - (num_screens_to_draw > 1 ? 16 : 0);
		int bottom_row = 192 - (num_screens_to_draw > 1 ? 16 : 0);

		//not the first row of screens
		if (peek_above)
		{
			if(screen>15 && !NoScreenPreview)
			{
				Map.drawrow(mapscreenbmp, edge_xoff, 0, Flags, 160, -1, screen-16);
			}
			else
			{
				Map.drawstaticrow(mapscreenbmp, edge_xoff, 0);
			}
		}
		
		//not the last row of screens
		if (peek_below)
		{
			if(screen + 16 < 0x80 && !NoScreenPreview)
			{
				Map.drawrow(mapscreenbmp, edge_xoff, bottom_row, Flags, 0, -1, screen+16);
			}
			else
			{
				Map.drawstaticrow(mapscreenbmp, edge_xoff, bottom_row);
			}
		}
		
		//not the first column of screens
		if (peek_left)
		{
			if(screen&0x0F && !NoScreenPreview)
			{
				Map.drawcolumn(mapscreenbmp, 0, edge_yoff, Flags, 15, -1, screen-1);
			}
			else
			{
				Map.drawstaticcolumn(mapscreenbmp, 0, edge_yoff);
			}
		}
		
		//not the last column of screens
		if (peek_right)
		{
			if((screen&0x0F)<15 && !NoScreenPreview)
			{
				Map.drawcolumn(mapscreenbmp, right_col, edge_yoff, Flags, 0, -1, screen+1);
			}
			else
			{
				Map.drawstaticcolumn(mapscreenbmp, right_col, edge_yoff);
			}
		}
		
		//not the first row or first column of screens
		if (peek_above && peek_left)
		{
			if((screen>15)&&(screen&0x0F) && !NoScreenPreview)
			{
				Map.drawblock(mapscreenbmp, 0, 0, Flags, 175, -1, screen-17);
			}
			else
			{
				Map.drawstaticblock(mapscreenbmp, 0, 0);
			}
		}
		
		//not the first row or last column of screens
		if (peek_above && peek_right)
		{
			if((screen>15)&&((screen&0x0F)<15) && !NoScreenPreview)
			{
				Map.drawblock(mapscreenbmp, right_col, 0, Flags, 160, -1, screen-15);
			}
			else
			{
				Map.drawstaticblock(mapscreenbmp, right_col, 0);
			}
		}
		
		//not the last row or first column of screens
		if (peek_below && peek_left)
		{
			if((screen<112)&&(screen&0x0F) && !NoScreenPreview)
			{
				Map.drawblock(mapscreenbmp, 0, bottom_row, Flags, 15, -1, screen+15);
			}
			else
			{
				Map.drawstaticblock(mapscreenbmp, 0, bottom_row);
			}
		}
		
		//not the last row or last column of screens
		if (peek_below && peek_right)
		{
			if((screen<112)&&((screen&0x0F)<15) && !NoScreenPreview)
			{
				Map.drawblock(mapscreenbmp, right_col, bottom_row, Flags, 0, -1, screen+17);
			}
			else
			{
				Map.drawstaticblock(mapscreenbmp, right_col, bottom_row);
			}
		}
	}
	
	if (ShowSquares && Map.getViewSize() < 4)
	{
		if(scr->stairx || scr->stairy)
		{
			int32_t x1 = scr->stairx+edge_xoff;
			int32_t y1 = scr->stairy+edge_yoff;
			safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,vc(14));
		}
		
		if(scr->warparrivalx || scr->warparrivaly)
		{
			int32_t x1 = scr->warparrivalx +edge_xoff;
			int32_t y1 = scr->warparrivaly +edge_yoff;
			safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,vc(10));
		}
		
		for(int32_t i=0; i<4; i++) if(scr->warpreturnx[i] || scr->warpreturny[i])
			{
				int32_t x1 = scr->warpreturnx[i]+edge_xoff;
				int32_t y1 = scr->warpreturny[i]+edge_yoff;
				int32_t clr = vc(9);
				
				if(FlashWarpSquare==i)
				{
					if(!FlashWarpClk)
						FlashWarpSquare=-1;
					else if(!(--FlashWarpClk%3))
						clr = vc(15);
				}
				
				safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,clr);
			}
	}
	
	if(ShowFFCs)
	{
		mapscr* ffscr = prv_mode ? Map.get_prvscr() : scr;
		int num_ffcs = ffscr->numFFC();
		for(int32_t i=num_ffcs-1; i>=0; i--)
		{
			ffcdata& ff = ffscr->ffcs[i];
			if(ff.data !=0 && (CurrentLayer<2 || (ff.flags&ffc_overlay)))
			{
				auto x = ff.x+edge_xoff;
				auto y = ff.y+edge_yoff;
				safe_rect(mapscreenbmp, x+0, y+0, x+ff.txsz*16-1, y+ff.tysz*16-1, vc(12));
			}
		}
	}
	
	if(!(Flags&cDEBUG) && pixeldb==1)
	{
		for(int32_t j=168; j<176; j++)
		{
			for(int32_t i=0; i<256; i++)
			{
				if(((i^j)&1)==0)
				{
					putpixel(mapscreenbmp,edge_xoff+i,
						edge_yoff+j,vc(blackout_color));
				}
			}
		}
	}

	int w = mapscreenbmp->w * mapscreen_single_scale;
	int h = mapscreenbmp->h * mapscreen_single_scale;
	stretch_blit(mapscreenbmp, menu1, 0, 0, mapscreenbmp->w, mapscreenbmp->h, mapscreen_x + xoff, mapscreen_y + yoff, w, h);
}

void draw_screenunit(int32_t unit, int32_t flags)
{
	FONT* tfont = font;
	switch(unit)
	{
		case rSCRMAP:
		{
			size_and_pos *mini_sqr = &minimap;
			size_and_pos *real_mini_sqr = &real_minimap;
			
			if(zoomed_minimap)
			{
				mini_sqr = &minimap_zoomed;
				real_mini_sqr = &real_minimap_zoomed;
			}
			
			auto txt_x = real_mini_sqr->x+2+8*real_mini_sqr->xscale;
			auto txt_y = real_mini_sqr->y+2+8*real_mini_sqr->yscale;
			
			rectfill(menu1, mini_sqr->x-1, mini_sqr->y-2,mini_sqr->x+mini_sqr->w-1,mini_sqr->y+mini_sqr->h-1,jwin_pal[jcBOX]);
			if(zoomed_minimap)
				jwin_draw_frame(menu1, mini_sqr->x-1, mini_sqr->y-2,mini_sqr->w,mini_sqr->h,FR_WIN);
			jwin_draw_minimap_frame(menu1,real_mini_sqr->x,real_mini_sqr->y,real_mini_sqr->tw(), real_mini_sqr->th(), real_mini_sqr->xscale, FR_DEEP);
			
			if(Map.getCurrMap()<Map.getMapCount())
			{
				for(int32_t i=0; i<MAPSCRS; i++)
				{
					auto& sqr = real_mini_sqr->subsquare(i);
					
					if(Map.Scr(i)->valid&mVALID)
					{
						// Handled by mmap_draw.
					}
					else
					{
						if (InvalidBG == 2)
						{
							draw_checkerboard(menu1, sqr.x, sqr.y, sqr.w);
						}
						else if (InvalidBG == 1)
						{
							for(int32_t dy=0; dy<sqr.h; dy++)
							{
								for(int32_t dx=0; dx<sqr.w; dx++)
								{
									menu1->line[dy+sqr.y][dx+sqr.x]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
								}
							}
						}
						else
						{
							int32_t offs = 2*(sqr.w/9);
							draw_x(menu1, sqr.x+offs, sqr.y+offs, sqr.x+sqr.w-1-offs, sqr.y+sqr.h-1-offs, vc(15));
						}
					}
				}
				
				int32_t s=Map.getCurrScr();
				
				BITMAP* txtbmp = create_bitmap_ex(8,256,64);
				clear_bitmap(txtbmp);
				int txtscale = zoomed_minimap ? (is_compact ? 2 : 3) : 1;
				font = get_zc_font(font_lfont_l);
				
				int32_t space = text_length(font, "255")+2, spc_s = text_length(font, "S")+2, spc_m = text_length(font, "M")+2;
				textprintf_disabled(txtbmp,font,0,0,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"M");
				static int map_shortcut_tooltip_id = ttip_register_id();
				ttip_install(map_shortcut_tooltip_id, "Prev map: ,\nNext map: .", txt_x, txt_y, 30, 20, txt_x, txt_y - 60);
				
				textprintf_ex(txtbmp,font,spc_m,0,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-3d",Map.getCurrMap()+1);
				
				textprintf_disabled(txtbmp,font,spc_m+space,0,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"S");
				textprintf_ex(txtbmp,font,spc_m+space+spc_s,0,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"0x%02X (%d)",s, s);
				masked_stretch_blit(txtbmp, menu1, 0, 0, 256, 64, txt_x, txt_y, 256*txtscale, 64*txtscale);
				destroy_bitmap(txtbmp);
			}
		}
		break;
		case rMAP:
		{
			refresh_visible_screens();
			mapscreen_single_scale = (double)mapscreen_screenunit_scale / Map.getViewSize();

			int num_combos_width = 16 * Map.getViewSize();
			int num_combos_height = 11 * Map.getViewSize();
			
			if(CurrentLayer > 0 && !mapscreen_valid_layers[CurrentLayer-1])
				CurrentLayer = 0;
			
			for (auto& vis_screen : visible_screens)
			{
				draw_screenunit_map_screen(vis_screen);
			}

			if (showxypos_icon)
			{
				int x0 = showxypos_x + (showedges?16:0);
				int y0 = showxypos_y + (showedges?16:0);
				int x1 = x0 + showxypos_w - 1;
				int y1 = y0 + showxypos_h - 1;
				x0 *= mapscreen_single_scale;
				y0 *= mapscreen_single_scale;
				x1 *= mapscreen_single_scale;
				y1 *= mapscreen_single_scale;
				x0 += mapscreen_x;
				y0 += mapscreen_y;
				x1 += mapscreen_x;
				y1 += mapscreen_y;

				if (showxypos_color == vc(15))
					safe_rect(menu1, x0, y0, x1, y1, showxypos_color);
				else
					rectfill(menu1, x0, y0, x1, y1, showxypos_color);
			}
			
			if(showxypos_cursor_icon)
			{
				int x0 = showxypos_cursor_x + (showedges?16:0);
				int y0 = showxypos_cursor_y + (showedges?16:0);
				int x1 = x0 + showxypos_w - 1;
				int y1 = y0 + showxypos_h - 1;
				x0 *= mapscreen_single_scale;
				y0 *= mapscreen_single_scale;
				x1 *= mapscreen_single_scale;
				y1 *= mapscreen_single_scale;
				x0 += mapscreen_x;
				y0 += mapscreen_y;
				x1 += mapscreen_x;
				y1 += mapscreen_y;
				safe_rect(menu1, x0, y0, x1, y1, showxypos_cursor_color);
			}

			// Draw dithering over the edge/preview combos.
			if(showedges)
			{
				int tile_size = 16 * mapscreen_single_scale;
				int tiles_across = (16 * Map.getViewSize()) + 2;
				int bottom_row_y = (Map.getViewSize()*11 + 1) * tile_size;
				int right_col_x = (Map.getViewSize()*16 + 1) * tile_size;

				//top preview
				for(int32_t j=0; j<tile_size; j++)
				{
					for(int32_t i=0; i<tiles_across * tile_size; i++)
					{
						if(((i^j)&1)==0)
						{
							putpixel(menu1,mapscreen_x+i,mapscreen_y+j,vc(0));
						}
					}
				}
				
				//bottom preview
				for(int32_t j = bottom_row_y; j < bottom_row_y + tile_size; j++)
				{
					for(int32_t i=0; i<tiles_across * tile_size; i++)
					{
						if(((i^j)&1)==0)
						{
							putpixel(menu1,mapscreen_x+i,mapscreen_y+j,vc(0));
						}
					}
				}
				
				//left preview
				for(int32_t j=tile_size; j<int32_t(192*mapscreen_screenunit_scale); j++)
				{
					for(int32_t i=0; i<16*mapscreen_single_scale; i++)
					{
						if(((i^j)&1)==0)
						{
							putpixel(menu1,mapscreen_x+i,mapscreen_y+j,vc(0));
						}
					}
				}
				
				//right preview
				for(int32_t j=tile_size; j<int32_t(192*mapscreen_screenunit_scale); j++)
				{
					for(int32_t i = right_col_x; i < right_col_x + tile_size; i++)
					{
						if(((i^j)&1)==0)
						{
							putpixel(menu1,mapscreen_x+i,mapscreen_y+j,vc(0));
						}
					}
				}
			}

			if(!(Flags&cDEBUG) && pixeldb==2)
			{
				for(int32_t j=int32_t(168*mapscreen_single_scale); j<int32_t(176*mapscreen_single_scale); j++)
				{
					for(int32_t i=0; i<int32_t(256*mapscreen_single_scale); i++)
					{
					
						if(((i^j)&1)==0)
						{
							putpixel(menu1,int32_t(mapscreen_x+(showedges?(16*mapscreen_single_scale):0)+i),
									 int32_t(mapscreen_y+(showedges?(16*mapscreen_single_scale):0)+j),vc(blackout_color));
						}
					}
				}
			}

			// TODO: This should move to `zmap::draw` (and delete the current code there doing a similar thing).
			if (Map.isDark(Map.getCurrScr()) && Map.getViewSize() == 1)
			{
				if((Flags&cNEWDARK) && get_qr(qr_NEW_DARKROOM))
				{
					BITMAP* tmpDark = create_bitmap_ex(8,16*16,16*11);
					BITMAP* tmpDarkTrans = create_bitmap_ex(8,16*16,16*11);
					BITMAP* tmpbuf = create_bitmap_ex(8,
						mapscreen_single_scale*(256+(showedges?32:0)),
						mapscreen_single_scale*(176+(showedges?32:0)));
					BITMAP* tmpbuf2 = create_bitmap_ex(8,
						mapscreen_single_scale*(256+(showedges?32:0)),
						mapscreen_single_scale*(176+(showedges?32:0)));
					int32_t darkCol = zinit.darkcol;
					switch(darkCol) //special cases
					{
						case BLACK:
							darkCol = vc(0);
							break;
						case WHITE:
							darkCol = vc(15);
							break;
					}
					clear_to_color(tmpDark, darkCol);
					clear_to_color(tmpDarkTrans, darkCol);
					clear_bitmap(tmpbuf);
					clear_bitmap(tmpbuf2);
					//Handle torch combos
					color_map = &trans_table2;
					Map.draw_darkness(tmpDark, tmpDarkTrans);
					//
					mapscr* tmp = Map.CurrScr();
					if(tmp->flags9 & fDARK_DITHER)
					{
						ditherblit(tmpDark, tmpDark, 0, zinit.dither_type, zinit.dither_arg);
						ditherblit(tmpDarkTrans, tmpDarkTrans, 0, zinit.dither_type, zinit.dither_arg);
					}
					
					if(mapscreen_single_scale == 1)
					{
						blit(tmpDark, tmpbuf, 0, 0, (showedges?16:0), (showedges?16:0), 16*16, 16*11);
						blit(tmpDarkTrans, tmpbuf2, 0, 0, (showedges?16:0), (showedges?16:0), 16*16, 16*11);
					}
					else
					{
						stretch_blit(tmpDark, tmpbuf, 0, 0, 16*16, 16*11,
							(showedges?16:0)*mapscreen_single_scale, (showedges?16:0)*mapscreen_single_scale,
							(16*16)*mapscreen_single_scale, (16*11)*mapscreen_single_scale);
						stretch_blit(tmpDarkTrans, tmpbuf2, 0, 0, 16*16, 16*11,
							(showedges?16:0)*mapscreen_single_scale, (showedges?16:0)*mapscreen_single_scale,
							(16*16)*mapscreen_single_scale, (16*11)*mapscreen_single_scale);
					}
					
					if(tmp->flags9 & fDARK_TRANS)
					{
						draw_trans_sprite(menu1, tmpbuf, mapscreen_x, mapscreen_y);
					}
					else
					{
						masked_blit(tmpbuf,menu1,0,0,mapscreen_x,mapscreen_y,tmpbuf->w,tmpbuf->h);
					}
					draw_trans_sprite(menu1, tmpbuf2, mapscreen_x, mapscreen_y);
					color_map = &trans_table;
					//
					destroy_bitmap(tmpDark);
					destroy_bitmap(tmpDarkTrans);
					destroy_bitmap(tmpbuf);
					destroy_bitmap(tmpbuf2);
				}
				else if(!(Flags&cNODARK))
				{
					for(int32_t j=0; j<80*mapscreen_single_scale; j++)
					{
						for(int32_t i=0; i<(80*mapscreen_single_scale)-j; i++)
						{
							if(((i^j)&1)==0)
							{
								putpixel(menu1,int32_t(mapscreen_x+(showedges?(16*mapscreen_single_scale):0))+i,
										 int32_t(mapscreen_y+(showedges?(16*mapscreen_single_scale):0)+j),vc(blackout_color));
							}
						}
					}
				}
			}
			
			double startx=mapscreen_x+(showedges?(16*mapscreen_single_scale):0);
			double starty=mapscreen_y+(showedges?(16*mapscreen_single_scale):0);
			bool inrect = isinRect(gui_mouse_x(),gui_mouse_y(),startx,starty,(startx+(256*mapscreen_screenunit_scale)-1),(starty+(176*mapscreen_screenunit_scale)-1));
			
			if(!(flags&rNOCURSOR) && ((ComboBrush && !ComboBrushPause)||draw_mode==dm_alias) && inrect)
			{
				int mgridscale=16*mapscreen_single_scale;
				if(allowHideMouse)
				{
					if(arrowcursor)
					{
						arrowcursor = false;
						MouseSprite::set(ZQM_BLANK);
					}
				}
				else if(!arrowcursor)
				{
					arrowcursor = true;
					MouseSprite::set(ZQM_NORMAL);
				}
				ComboPosition pos = get_mapscreen_mouse_combo_pos();
				int32_t mx = pos.x * 16 * mapscreen_single_scale;
				int32_t my = pos.y * 16 * mapscreen_single_scale;

				clear_bitmap(brushscreen);
				int32_t tempbw=BrushWidth;
				int32_t tempbh=BrushHeight;
				
				if(draw_mode==dm_alias)
				{
					BrushWidth = combo_aliases[combo_apos].width+1;
					BrushHeight = combo_aliases[combo_apos].height+1;
				}
				else if(draw_mode == dm_cpool)
				{
					BrushWidth = BrushHeight = 1;
					combo_pool const& pool = combo_pools[combo_pool_pos];
					if(pool.valid())
					{
						int32_t cid = Combo;
						int8_t cset = CSet;
						pool.get_w_wrap(cid,cset,cpoolbrush_index/16); //divide to reduce speed
						put_combo(brushbmp,0,0,cid,cset,Flags&(cFLAGS|cWALK),0);
					}
					else clear_bitmap(brushbmp);
				}
				else if (draw_mode == dm_auto)
				{
					BrushWidth = BrushHeight = 1;
				}

				stretch_blit(brushbmp, brushscreen, 0, 0, BrushWidth*16, BrushHeight*16, 0, 0, BrushWidth*mgridscale, BrushHeight*mgridscale);
				int float_offx = 0;
				int float_offy = 0;
				
				if(FloatBrush)
				{
					float_offx = -SHADOW_DEPTH*mapscreen_single_scale;
					float_offy = -SHADOW_DEPTH*mapscreen_single_scale;
					
					//shadow
					for(int x = 0; x < SHADOW_DEPTH*mapscreen_single_scale; ++x)
						for(int y = 0; y < (BrushHeight*mgridscale) + (SHADOW_DEPTH*mapscreen_single_scale); ++y)
						{
							if((((x^y)&1)==1) && y < 12*mgridscale)
								putpixel(brushscreen,x+(BrushWidth*mgridscale),y,vc(0));
						}
					
					for(int x = 0; x < BrushWidth*mgridscale; ++x)
						for(int y = 0; y < SHADOW_DEPTH*mapscreen_single_scale; ++y)
						{
							if((((x^y)&1)==1) && x<16*mgridscale)
								putpixel(brushscreen,x,y+(BrushHeight*mgridscale),vc(0));
						}
				}
				
				if(draw_mode==dm_alias)
				{
					combo_alias *combo = &combo_aliases[combo_apos];
					
					if(BrushWidth > 1 && (alias_origin & 1)) //right-align
						float_offx -= (BrushWidth - 1) * mgridscale;
					
					if(BrushHeight > 1 && (alias_origin & 2)) //bottom-align
						float_offy -= (BrushHeight - 1) * mgridscale;
				}

				int bx = mapscreen_x + mx + float_offx + (showedges?(16*mapscreen_single_scale):0);
				int by = mapscreen_y + my + float_offy + (showedges?(16*mapscreen_single_scale):0);
				masked_blit(brushscreen, menu1, 0, 0, bx, by, 16*mgridscale, 11*mgridscale);
				BrushWidth=tempbw;
				BrushHeight=tempbh;
			}
			else
			{
				if(!arrowcursor)
				{
					MouseSprite::set(ZQM_NORMAL);
					arrowcursor = true;
				}
			}

			if(ShowGrid)
			{
				int w = num_combos_width;
				int h = num_combos_height;
				double tile_size = 16.0 / Map.getViewSize() * mapscreen_screenunit_scale;
				
				if(showedges)
				{
					w += 2;
					h += 2;
				}

				for (int x = 1; x < w; x++)
				{
					vline(menu1, mapscreen_x + x*tile_size, mapscreen_y, mapscreen_y + (h*tile_size)-1, vc(GridColor));
				}

				for (int y = 1; y < h; y++)
				{
					hline(menu1, mapscreen_x, mapscreen_y + y*tile_size, mapscreen_x + (w*tile_size)-1, vc(GridColor));
				}
			}

			if(ShowScreenGrid)
			{
				int w = num_combos_width;
				int h = num_combos_height;
				double tile_size = 16.0 / Map.getViewSize() * mapscreen_screenunit_scale;
				int startx = mapscreen_x + (showedges ? (16 * mapscreen_single_scale) : 0);
				int starty = mapscreen_y + (showedges ? (16 * mapscreen_single_scale) : 0);
				
				if(showedges)
				{
					w += 1;
					h += 1;
				}

				int color = (GridColor+8)%16;

				for (int x = 16; x < w; x+=16)
				{
					vline(menu1, startx + x*tile_size, mapscreen_y, starty + (h*tile_size)-1, vc(color));
				}

				for (int y = 11; y < h; y+=11)
				{
					hline(menu1, startx, starty + y*tile_size, startx + (w*tile_size)-1, vc(color));
				}
			}

			// Draw a black-yellow-black rect around the currently selected screen.
			if (ShowCurScreenOutline && Map.getViewSize() > 1)
			{
				int mw = 256 * mapscreen_single_scale;
				int mh = 176 * mapscreen_single_scale;
				int mx = (Map.getCurrScr() % 16) - (Map.getViewScr() % 16);
				int my = (Map.getCurrScr() / 16) - (Map.getViewScr() / 16);
				int x0 = mapscreen_x + (showedges ? (16 * mapscreen_single_scale) : 0) + mx * mw;
				int y0 = mapscreen_y + (showedges ? (16 * mapscreen_single_scale) : 0) + my * mh;
				dotted_rect(menu1, x0+1, y0+1, x0 + mw - 1, y0 + mh - 1, vc(1), vc(0));
				rect(menu1, x0, y0, x0 + mw, y0 + mh, vc(14));
				dotted_rect(menu1, x0-1, y0-1, x0 + mw + 1, y0 + mh + 1, vc(1), vc(0));
			}
			
			// Map tabs
			font = get_custom_font(CFONT_GUI);
			
			map_page[current_mappage].map=Map.getCurrMap();
			map_page[current_mappage].screen=Map.getCurrScr();
			
			for(int32_t btn=0; btn<mappage_count; ++btn)
			{
				char tbuf[15];
				sprintf(tbuf, "%d:%02X", map_page[btn].map+1, map_page[btn].screen);
				draw_layer_button(menu1,map_page_bar[btn].x, map_page_bar[btn].y, map_page_bar[btn].w, map_page_bar[btn].h,tbuf,(btn==current_mappage?D_SELECTED:0));
			}
		}
		break;
		case rCOMBOS:
		{
			auto real_h = combolist_window.h;
			jwin_draw_frame(menu1,combolist_window.x,combolist_window.y,combolist_window.w,real_h, FR_WIN);
			rectfill(menu1,combolist_window.x+2,combolist_window.y+2,combolist_window.x+combolist_window.w-3,combolist_window.y+real_h-3,jwin_pal[jcBOX]);
			
			//Scrollers
			for(int32_t c = 0; c < num_combo_cols; ++c)
			{
				auto& pos = combolistscrollers[c];
				
				{ //Scroll up
					auto& p = pos.subsquare(0);
					jwin_draw_frame(menu1,p.x,p.y,p.w,p.h,FR_ETCHED);
					
					for(int32_t i=0; i<3; i++)
					{
						hline(menu1, p.x+5-i, p.y+4+i, p.x+5+i, jwin_pal[jcBOXFG]);
					}
				}
				
				{ //Scroll down
					auto& p = pos.subsquare(1);
					jwin_draw_frame(menu1,p.x,p.y,p.w,p.h,FR_ETCHED);
					
					for(int32_t i=0; i<3; i++)
					{
						hline(menu1,p.x+5-i,p.y+6-i, p.x+5+i, jwin_pal[jcBOXFG]);
					}
				}
			}
			
			if(draw_mode==dm_alias)
			{
				if(LinkedScroll)
				{
					int tmp = current_comboalist;
					for(int q = tmp-1; q >= 0; --q)
					{
						combo_alistpos[q] = combo_alistpos[q+1]-(comboaliaslist[q].w*comboaliaslist[q].h);
						if(combo_alistpos[q] < 0)
						{
							tmp = 0;
							combo_alistpos[0] = 0;
							break;
						}
					}
					for(int q = tmp+1; q < num_combo_cols; ++q)
						combo_alistpos[q] = combo_alistpos[q-1]+(comboaliaslist[q-1].w*comboaliaslist[q-1].h);
					for(int q = 0; q < num_combo_cols; ++q)
						if(combo_apos >= combo_alistpos[q] && combo_apos < combo_alistpos[q] + (comboaliaslist[q].w*comboaliaslist[q].h))
						{
							current_comboalist = q;
							break;
						}
				}
				for(int32_t c = 0; c < num_combo_cols; ++c)
				{
					auto& pos = comboaliaslist[c];
					rectfill(menu1,pos.x,pos.y,pos.x+(pos.w*pos.xscale)-1,pos.y+(pos.h*pos.yscale)-1,0);
					jwin_draw_frame(menu1,pos.x-2,pos.y-2,(pos.w*pos.xscale)+4,(pos.h*pos.yscale)+4,FR_DEEP);
				}
				
				auto& prev = comboalias_preview;
				jwin_draw_frame(menu1, prev.x-2, prev.y-2, prev.w+4, prev.h+4,FR_DEEP);
				
				BITMAP *prv = create_bitmap_ex(8,64,64);
				clear_bitmap(prv);
				int32_t scalefactor = 1;
				
				for(int32_t j=0; j<num_combo_cols; ++j)
				{
					auto per_page = (comboaliaslist[j].w * comboaliaslist[j].h);
					if(combo_alistpos[j] + per_page >= MAXCOMBOALIASES)
						combo_alistpos[j] = MAXCOMBOALIASES-per_page;
					auto& col = comboaliaslist[j];
					for(int32_t i=0; i<(comboaliaslist[j].w*comboaliaslist[j].h); i++)
					{
						draw_combo_alias_thumbnail(menu1, &combo_aliases[combo_alistpos[j]+i],
							(i%col.w)*col.xscale+col.x, (i/col.w)*col.yscale+col.y, col.xscale/16);
					}
					
					if((combo_aliases[combo_apos].width>7)||(combo_aliases[combo_apos].height>7))
					{
						scalefactor=4;
					}
					else if((combo_aliases[combo_apos].width>3)||(combo_aliases[combo_apos].height>3))
					{
						scalefactor=2;
					}
					
					
					if(j==current_comboalist)
					{
						stretch_blit(brushbmp, prv, 0,0,scalefactor*64,zc_min(scalefactor*64,176),0,0,64,scalefactor==4?44:64);
						blit(prv,menu1,0,0,comboalias_preview.x,comboalias_preview.y,comboalias_preview.w,comboalias_preview.h);
						
						int32_t rect_pos=combo_apos-combo_alistpos[current_comboalist];
						
						if((rect_pos>=0)&&(rect_pos<(combo_alistpos[current_comboalist]+(col.w*col.h))))
						{
							int selw = col.xscale;
							int selh = col.yscale;
							int x1 = (rect_pos&(col.w-1))*col.xscale+col.x;
							int y1 = (rect_pos/col.w)*col.yscale+col.y;
							safe_rect(menu1,x1,y1,x1+selw-1,y1+selh-1,vc(CmbCursorCol),2);
						}
					}
				}
				
				destroy_bitmap(prv);
			}
			else if(draw_mode==dm_cpool)
			{
				if(LinkedScroll)
				{
					int tmp = current_cpoollist;
					for(int q = tmp-1; q >= 0; --q)
					{
						combo_pool_listpos[q] = combo_pool_listpos[q+1]-(comboaliaslist[q].w*comboaliaslist[q].h);
						if(combo_pool_listpos[q] < 0)
						{
							tmp = 0;
							combo_pool_listpos[0] = 0;
							break;
						}
					}
					for(int q = tmp+1; q < num_combo_cols; ++q)
						combo_pool_listpos[q] = combo_pool_listpos[q-1]+(comboaliaslist[q-1].w*comboaliaslist[q-1].h);
					for(int q = 0; q < num_combo_cols; ++q)
						if(combo_pool_pos >= combo_pool_listpos[q] && combo_pool_pos < combo_pool_listpos[q] + (comboaliaslist[q].w*comboaliaslist[q].h))
						{
							current_cpoollist = q;
							break;
						}
				}
				for(int32_t c = 0; c < num_combo_cols; ++c)
				{
					auto& pos = comboaliaslist[c];
					rectfill(menu1,pos.x,pos.y,pos.x+(pos.w*pos.xscale)-1,pos.y+(pos.h*pos.yscale)-1,0);
					jwin_draw_frame(menu1,pos.x-2,pos.y-2,(pos.w*comboaliaslist[c].xscale)+4,(pos.h*comboaliaslist[c].yscale)+4,FR_DEEP);
				}
				
				for (int32_t j = 0; j < num_combo_cols; ++j) //the actual panes
				{
					auto per_page = (comboaliaslist[j].w * comboaliaslist[j].h);
					if(combo_pool_listpos[j] + per_page >= MAXCOMBOPOOLS)
						combo_pool_listpos[j] = MAXCOMBOPOOLS-per_page;
					for(int32_t i=0; i<(comboaliaslist[j].w*comboaliaslist[j].h); i++)
					{
						int32_t cid=-1; int8_t cs=CSet;
						combo_pool const& cp = combo_pools[combo_pool_listpos[j]+i];
						
						auto& list = comboaliaslist[j];
						if(cp.get_w(cid,cs,0) && !combobuf[cid].tile)
						{
							cid = -1; //no tile to draw
						}
						auto cx = (i%list.w)*list.xscale+list.x;
						auto cy = (i/list.w)*list.yscale+list.y;
						put_combo(menu1,cx,cy,cid,cs,Flags&(cFLAGS|cWALK),0,list.xscale/16);
					}
				}
				int32_t rect_pos=combo_pool_pos-combo_pool_listpos[current_cpoollist];
				
				if((rect_pos>=0)&&(rect_pos<(combo_pool_listpos[current_cpoollist]+(comboaliaslist[current_cpoollist].w*comboaliaslist[current_cpoollist].h))))
				{
					int selw = comboaliaslist[current_cpoollist].xscale;
					int selh = comboaliaslist[current_cpoollist].yscale;
					int x1 = (rect_pos&(comboaliaslist[current_cpoollist].w-1))*comboaliaslist[current_cpoollist].xscale+comboaliaslist[current_cpoollist].x;
					int y1 = (rect_pos/comboaliaslist[current_cpoollist].w)*comboaliaslist[current_cpoollist].yscale+comboaliaslist[current_cpoollist].y;
					safe_rect(menu1,x1,y1,x1+selw-1,y1+selh-1,vc(CmbCursorCol),2);
				}
				
				//Handle Preview
				combo_pool const& cpool = combo_pools[combo_pool_pos];
				
				int32_t cid; int8_t cs;
				size_t total = weighted_cpool ? cpool.getTotalWeight() : cpool.combos.size();
				size_t ind = 0;
				size_t indw = combopool_preview.w/16;
				size_t indh = combopool_preview.h/16;
				size_t rows = total ? vbound(total/indw,1,indh) : 0;
				if (is_compact)
					rows = vbound(rows, 1, 3);
				else
					rows = vbound(rows, 1, 4);
				size_t real_height = rows*16;
				
				cpool_prev_visible = rows > 0;
				if(rows)
				{
					jwin_draw_frame(menu1,combopool_preview.x-2,combopool_preview.y-2,
						combopool_preview.w+4,real_height+4,FR_DEEP);
					rectfill(menu1,combopool_preview.x,combopool_preview.y,
						combopool_preview.x+combopool_preview.w-1,
						combopool_preview.y+real_height-1,vc(0));
					draw_text_button(menu1,combopool_prevbtn.x,combopool_prevbtn.y,
						combopool_prevbtn.w,combopool_prevbtn.h,
						weighted_cpool ? "Weighted" : "Unweighted",vc(1),vc(14),0,true);
					if(!is_compact)
						textprintf_ex(menu1,font,combopool_prevbtn.x+combopool_prevbtn.w+5,
							combopool_prevbtn.y,jwin_pal[jcBOXFG],-1,"Preview");
					for(auto y = 0; y < real_height; y += 16)
					{
						for(auto x = 0; x < combopool_preview.w; x += 16, ++ind)
						{
							auto nx = combopool_preview.x+x, ny = combopool_preview.y+y;
							if(ind < total)
							{
								cs = CSet;
								if(weighted_cpool
									? cpool.get_w(cid,cs,ind)
									: cpool.get_ind(cid,cs,ind))
								{
									put_combo(menu1,nx,ny,cid,cs,Flags&(cFLAGS|cWALK),0);
									continue;
								}
							}
							//No combo to display
							xout(menu1, nx, ny, nx+15, ny+15, vc(15));
						}
					}
				}
			}
			else if (draw_mode == dm_auto)
			{
				if (LinkedScroll)
				{
					int tmp = current_cautolist;
					for (int q = tmp - 1; q >= 0; --q)
					{
						combo_auto_listpos[q] = combo_auto_listpos[q + 1] - (comboaliaslist[q].w * comboaliaslist[q].h);
						if (combo_auto_listpos[q] < 0)
						{
							tmp = 0;
							combo_auto_listpos[0] = 0;
							break;
						}
					}
					for (int q = tmp + 1; q < num_combo_cols; ++q)
						combo_auto_listpos[q] = combo_auto_listpos[q - 1] + (comboaliaslist[q - 1].w * comboaliaslist[q - 1].h);
					for (int q = 0; q < num_combo_cols; ++q)
						if (combo_auto_pos >= combo_auto_listpos[q] && combo_auto_pos < combo_auto_listpos[q] + (comboaliaslist[q].w * comboaliaslist[q].h))
						{
							current_cautolist = q;
							break;
						}
				}
				for (int32_t c = 0; c < num_combo_cols; ++c)
				{
					auto& pos = comboaliaslist[c];
					rectfill(menu1, pos.x, pos.y, pos.x + (pos.w * pos.xscale) - 1, pos.y + (pos.h * pos.yscale) - 1, 0);
					jwin_draw_frame(menu1, pos.x - 2, pos.y - 2, (pos.w * comboaliaslist[c].xscale) + 4, (pos.h * comboaliaslist[c].yscale) + 4, FR_DEEP);
				}

				for (int32_t j = 0; j < num_combo_cols; ++j) //the actual panes
				{
					auto per_page = (comboaliaslist[j].w * comboaliaslist[j].h);
					if(combo_auto_listpos[j] + per_page >= MAXAUTOCOMBOS)
						combo_auto_listpos[j] = MAXAUTOCOMBOS-per_page;
					for (int32_t i = 0; i < (comboaliaslist[j].w * comboaliaslist[j].h); i++)
					{
						int32_t cid = -1; int8_t cs = CSet;
						combo_auto const& ca = combo_autos[combo_auto_listpos[j] + i];

						auto& list = comboaliaslist[j];
						cid = ca.getDisplay();
						if (cid == 0)
							cid = -1;
						auto cx = (i % list.w) * list.xscale + list.x;
						auto cy = (i / list.w) * list.yscale + list.y;
						put_combo(menu1, cx, cy, cid, cs, Flags & (cFLAGS | cWALK), 0, list.xscale / 16);
						put_autocombo_engravings(menu1, ca, combo_auto_listpos[j] + i == combo_auto_pos, cx, cy, list.xscale / 16);
					}
				}
				int32_t rect_pos = combo_auto_pos - combo_auto_listpos[current_cautolist];

				if ((rect_pos >= 0) && (rect_pos < (combo_auto_listpos[current_cautolist] + (comboaliaslist[current_cautolist].w * comboaliaslist[current_cautolist].h))))
				{
					int selw = comboaliaslist[current_cautolist].xscale;
					int selh = comboaliaslist[current_cautolist].yscale;
					int x1 = (rect_pos & (comboaliaslist[current_cautolist].w - 1)) * comboaliaslist[current_cautolist].xscale + comboaliaslist[current_cautolist].x;
					int y1 = (rect_pos / comboaliaslist[current_cautolist].w) * comboaliaslist[current_cautolist].yscale + comboaliaslist[current_cautolist].y;
					safe_rect(menu1, x1, y1, x1 + selw - 1, y1 + selh - 1, vc(CmbCursorCol), 2);

					combo_auto const& ca = combo_autos[combo_auto_pos];
					put_autocombo_engravings(menu1, ca, true, x1, y1, selw / 16);
				}
			}
			else
			{
				if(LinkedScroll)
				{
					int tmp = current_combolist;
					for(int q = tmp-1; q >= 0; --q)
					{
						First[q] = First[q+1]-(combolist[q].w*combolist[q].h);
						if(First[q] < 0)
						{
							tmp = 0;
							First[0] = 0;
							break;
						}
					}
					for(int q = tmp+1; q < num_combo_cols; ++q)
						First[q] = First[q-1]+(combolist[q-1].w*combolist[q-1].h);
					for(int q = 0; q < num_combo_cols; ++q)
						if(Combo >= First[q] && Combo < First[q] + (combolist[q].w*combolist[q].h))
						{
							current_combolist = q;
							break;
						}
				}
				for(int32_t c = 0; c < num_combo_cols; ++c)
				{
					auto& pos = combolist[c];
					rectfill(menu1,pos.x,pos.y,pos.x+(pos.w*pos.xscale)-1,pos.y+(pos.h*pos.yscale)-1,0);
					jwin_draw_frame(menu1,pos.x-2,pos.y-2,(pos.w*pos.xscale)+4,(pos.h*pos.yscale)+4,FR_DEEP);
				}
				
				int32_t drawmap, drawscr;
				drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
				drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
				
				for(int32_t j=0; j<num_combo_cols; ++j)
				{
					auto per_page = (combolist[j].w * combolist[j].h);
					if(First[j] + per_page >= MAXCOMBOS)
						First[j] = MAXCOMBOS-per_page;
					for(int32_t i=0; i<(combolist[j].w*combolist[j].h); i++)
					{
						put_combo(menu1,(i%combolist[j].w)*combolist[j].xscale+combolist[j].x,
							(i/combolist[j].w)*combolist[j].yscale+combolist[j].y,
							i+First[j],CSet,Flags&(cFLAGS|cWALK),0,combolist[j].xscale/16);
					}
				}
				
				int32_t rect_pos=Combo-First[current_combolist];
				
				if((rect_pos>=0)&&(rect_pos<(combo_pool_listpos[current_combolist]+(combolist[current_combolist].w*combolist[current_combolist].h))))
				{
					int selw = (AutoBrush?BrushWidth:1)*combolist[current_combolist].xscale;
					int selh = (AutoBrush?BrushHeight:1)*combolist[current_combolist].yscale;
					int x1 = (rect_pos&(combolist[current_combolist].w-1))*combolist[current_combolist].xscale+combolist[current_combolist].x;
					int y1 = (rect_pos/combolist[current_combolist].w)*combolist[current_combolist].yscale+combolist[current_combolist].y;
					safe_rect(menu1,x1,y1,x1+selw-1,y1+selh-1,vc(CmbCursorCol),2);
				}
			}
		}
		break;
		case rCOMBO:
		{
			int32_t drawmap, drawscr;
			drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
			drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
			
			// Combo preview
			int32_t cid = Combo; int8_t cs = CSet;
			if(draw_mode == dm_alias)
			{
				cid = combo_aliases[combo_apos].combos[0];
				cs = wrap(combo_aliases[combo_apos].csets[0]+alias_cset_mod, 0, 13);
			}
			else if(draw_mode == dm_cpool)
			{
				combo_pool const& cpool = combo_pools[combo_pool_pos];
				cid = 0;
				cpool.get_w(cid,cs,0);
			}
			else if (draw_mode == dm_auto)
			{
				combo_auto const& cauto = combo_autos[combo_auto_pos];
				cid = cauto.getDisplay();
			}
			static BITMAP *combo_preview_bmp=create_bitmap_ex(8,32,32);
			static BITMAP *cycle_preview_bmp=create_bitmap_ex(8,32,32);
			// Combo
			put_combo(combo_preview_bmp,0,0,cid,cs,Flags&(cFLAGS|cWALK),0);
			jwin_draw_frame(menu1,combo_preview.x-2,combo_preview.y-2,combo_preview.w+4,combo_preview.h+4, FR_DEEP);
			stretch_blit(combo_preview_bmp, menu1, 0, 0, 16, 16, combo_preview.x, combo_preview.y, combo_preview.w, combo_preview.h);
			
			comboprev_buf[0] = 0;
			comboprev_buf2[0] = 0;
			if(draw_mode == dm_cpool)
			{
				sprintf(comboprev_buf,"Pool: %d",combo_pool_pos);
				int x = combo_preview_text1.x+(combo_preview_text1.w*combo_preview_text1.xscale);
				textbox_out(menu1,txfont,x,combo_preview_text1.y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],comboprev_buf,2,&combo_preview_text1);
			}
			else if (draw_mode == dm_auto)
			{
				GUI::ListData ac_types = GUI::ZCListData::autocombotypes();
				std::string type_name = ac_types.findText(combo_autos[combo_auto_pos].getType());
				if (is_compact)
					sprintf(comboprev_buf, "AC: %d CS: %d\n%s", combo_auto_pos, CSet, type_name.c_str());
				else
					sprintf(comboprev_buf, "Auto: %d CSet: %d\n%s\nEntries: %d", combo_auto_pos, CSet, type_name.c_str(), int32_t(combo_autos[combo_auto_pos].combos.size()));
				int x = combo_preview_text1.x + (combo_preview_text1.w * combo_preview_text1.xscale);
				textbox_out(menu1, txfont, x, combo_preview_text1.y, jwin_pal[jcBOXFG], jwin_pal[jcBOX], comboprev_buf, 2, &combo_preview_text1);
			}
			else if(draw_mode != dm_alias)
			{
				int x = combo_preview_text1.x+(combo_preview_text1.w*combo_preview_text1.xscale);
				
				char shortbuf[512];
				char buf[256];
				strcpy(buf,combo_class_buf[combobuf[Combo].type].name);
				sprintf(comboprev_buf,"Combo: %d\nCSet: %d\n%s",Combo,CSet,buf);
				int ind = strlen(buf)-1;
				int x2 = x;
				if(x2 - text_length(txfont, buf) <= combolist_window.x)
				{
					auto dotlen = text_length(txfont, "..");
					x2 -= dotlen;
					while(x2 - text_length(txfont, buf) <= combolist_window.x)
					{
						if(ind < 0) break;
						buf[ind--] = '\0';
					}
					while(ind >= 0 && buf[ind] == ' ')
						buf[ind--] = 0; //trim spaces
					strcat(buf, "..");
				}
				
				if(is_compact)
				{
					char b2[256];
					sprintf(b2, "Combo %d CS %d", Combo, CSet);
					if(x-text_length(txfont, b2) <= combolist_window.x)
						sprintf(b2, "Cmb %d CS %d", Combo, CSet);
					sprintf(shortbuf,"%s\n%s",b2,buf);
				}
				else sprintf(shortbuf,"Combo: %d\nCSet: %d\n%s",Combo,CSet,buf);
				textbox_out(menu1,txfont,x,combo_preview_text1.y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],shortbuf,2,&combo_preview_text1);
			}
			
			// Cycle
			if(!is_compact)
			{
				int32_t NextCombo = combobuf[Combo].nextcombo;
				int32_t NextCSet = combobuf[Combo].nextcset;
				if(combobuf[Combo].animflags & AF_CYCLEUNDERCOMBO)
				{
					mapscr* scr = Map.CurrScr();
					NextCombo = scr->undercombo;
					NextCSet = scr->undercset;
				}
				if(combobuf[Combo].animflags & AF_CYCLENOCSET)
					NextCSet = CSet;
				bool normal_dm = draw_mode != dm_alias && draw_mode != dm_cpool && draw_mode != dm_auto;
				jwin_draw_frame(menu1,combo_preview2.x-2,combo_preview2.y-2,combo_preview2.w+4,combo_preview2.h+4, FR_DEEP);
				if(NextCombo>0 && normal_dm)
				{
					put_combo(cycle_preview_bmp,0,0,NextCombo,NextCSet,Flags&(cFLAGS|cWALK),0);
					
					if(Flags&cWALK) put_walkflags(cycle_preview_bmp,0,0,NextCombo,0);
					
					if(Flags&cFLAGS) put_flags(cycle_preview_bmp,0,0,NextCombo,0,cFLAGS,0);
					
					stretch_blit(cycle_preview_bmp, menu1, 0, 0, 16, 16, combo_preview2.x, combo_preview2.y, combo_preview2.w, combo_preview2.h);
				}
				else
				{
					if (InvalidBG == 2)
					{
						draw_checkerboard(menu1, combo_preview2.x, combo_preview2.y, 32);
					}
					else if(InvalidBG == 1)
					{
						for(int32_t dy=0; dy<combo_preview2.w; dy++)
						{
							for(int32_t dx=0; dx<combo_preview2.w; dx++)
							{
								menu1->line[dy+combo_preview2.y][dx+combo_preview2.x]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
							}
						}
					}
					else
					{
						rectfill(menu1, combo_preview2.x,combo_preview2.y, combo_preview2.x+32,combo_preview2.y+combo_preview2.h,vc(0));
						safe_rect(menu1, combo_preview2.x,combo_preview2.y, combo_preview2.x+32,combo_preview2.y+combo_preview2.h,vc(15));
						line(menu1, combo_preview2.x,combo_preview2.y, combo_preview2.x+32,combo_preview2.y+combo_preview2.h,vc(15));
						line(menu1, combo_preview2.x,combo_preview2.y+combo_preview2.h, combo_preview2.x+32,combo_preview2.y,vc(15));
					}
				}
				
				if(normal_dm)
				{
					char shortbuf[512];
					char buf[256];
					strcpy(buf,combo_class_buf[combobuf[NextCombo].type].name);
					sprintf(comboprev_buf2, "Cycle: %d\nCSet: %d\n%s", NextCombo, NextCSet, buf);
					int ind = strlen(buf)-1;
					int x2 = combo_preview_text2.x;
					if(x2 + text_length(txfont, buf) > zq_screen_w-2)
					{
						auto dotlen = text_length(txfont, "..");
						x2 += dotlen;
						while(x2 + text_length(txfont, buf) > zq_screen_w-2)
						{
							if(ind < 0) break;
							buf[ind--] = '\0';
						}
						while(ind >= 0 && buf[ind] == ' ')
							buf[ind--] = 0; //trim spaces
						strcat(buf, "..");
					}
					
					sprintf(shortbuf, "Cycle: %d\nCSet: %d\n%s", NextCombo, NextCSet, buf);
					textbox_out(menu1,txfont,combo_preview_text2.x,combo_preview_text2.y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],shortbuf,0,&combo_preview_text2);
				}
			}
			
			font = get_zc_font(font_lfont_l);
			bool merged = is_compact ? compact_merged_combopane : large_merged_combopane;
			draw_text_button(menu1,combo_merge_btn.x,combo_merge_btn.y,combo_merge_btn.w,combo_merge_btn.h,merged ? "<|>" : ">|<",vc(1),vc(14),0,true);
		}
		break;
		case rFAVORITES:
		{
			font = get_zc_font(font_lfont_l);
				
			jwin_draw_frame(menu1,favorites_window.x,favorites_window.y,favorites_window.w,favorites_window.h, FR_WIN);
			rectfill(menu1,favorites_window.x+2,favorites_window.y+2,favorites_window.x+favorites_window.w-3,favorites_window.y+favorites_window.h-3,jwin_pal[jcBOX]);
			jwin_draw_frame(menu1,favorites_list.x-2,favorites_list.y-2,(favorites_list.w*favorites_list.xscale)+4,(favorites_list.h*favorites_list.yscale)+4, FR_DEEP);
			rectfill(menu1,favorites_list.x,favorites_list.y,favorites_list.x+(favorites_list.w*favorites_list.xscale)-1,favorites_list.y+(favorites_list.h*favorites_list.yscale)-1,jwin_pal[jcBOXFG]);
				
			textprintf_ex(menu1,get_zc_font(font_lfont_l),favorites_list.x-2,favorites_list.y-15,jwin_pal[jcBOXFG],-1,is_compact ? "Favorites" : "Favorite Combos");
			BITMAP* subb = create_bitmap_ex(8,16,16);

			for(int32_t col=0; col<favorites_list.w; ++col)
			{
				for(int32_t row=0; row<favorites_list.h; ++row)
				{
					auto i = (row*FAVORITECOMBO_PER_ROW)+col+FAVORITECOMBO_PER_PAGE*FavoriteComboPage;
					auto& sqr = favorites_list.subsquare(col,row);
					if(i >= MAXFAVORITECOMBOS || favorite_combos[i]==-1)
					{
						if (InvalidBG == 2)
						{
							draw_checkerboard(menu1, sqr.x, sqr.y, sqr.w);
						}
						else if(InvalidBG == 1)
						{
							for(int32_t dy=0; dy<sqr.h; dy++)
							{
								for(int32_t dx=0; dx<sqr.w; dx++)
								{
									menu1->line[sqr.y+dy][sqr.x+dx]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
								}
							}
						}
						else
						{
							xout(menu1, sqr.x, sqr.y, sqr.x+sqr.w-1, sqr.y+sqr.h-1, vc(15), vc(0));
						}
					}
					else
					{
						clear_bitmap(subb);
						bool repos = combotile_override_x < 0 && combotile_override_y < 0;

						switch(favorite_combo_modes[i])
						{
							case dm_alias:
								draw_combo_alias_thumbnail(subb, &combo_aliases[favorite_combos[i]], 0, 0, 1);
								if (ShowFavoriteComboModes)
									put_engraving(subb, 0, 0, 0x3E, 1);
								break;
							case dm_cpool:
							{
								int32_t cid = -1; int8_t cs = CSet;
								combo_pool const& cp = combo_pools[favorite_combos[i]];

								if (cp.get_w(cid, cs, 0) && !combobuf[cid].tile)
									cid = -1; //no tile to draw
								put_combo(subb, 0, 0, cid, cs, 0, 0);
								if (ShowFavoriteComboModes)
									put_engraving(subb, 0, 0, 0x3D, 1);
								break;
							}
							case dm_auto:
							{
								int32_t cid = -1; int8_t cs = CSet;
								combo_auto const& ca = combo_autos[favorite_combos[i]];

								cid = ca.getDisplay();
								if (cid == 0)
									cid = -1;
								put_combo(subb, 0, 0, cid, cs, 0, 0);
								if (ShowFavoriteComboModes)
									put_engraving(subb, 0, 0, 0x3C, 1);
								break;
							}
							default:
								if (repos)
								{
									combotile_override_x = sqr.x + (sqr.w - 16) / 2;
									combotile_override_y = sqr.y + (sqr.h - 16) / 2;
								}
								put_combo(subb, 0, 0, favorite_combos[i], CSet, Flags & (cFLAGS | cWALK), 0);
								if (repos) combotile_override_x = combotile_override_y = -1;
						}
						stretch_blit(subb, menu1, 0, 0, 16, 16, sqr.x, sqr.y, sqr.w, sqr.h);
					}
				}
			}

			destroy_bitmap(subb);

			bool zoomed = is_compact ? compact_zoomed_fav : large_zoomed_fav;
			if(!is_compact)
				textprintf_right_ex(menu1, get_zc_font(font_lfont_l), favorites_pgleft.x - 2, favorites_pgleft.y, jwin_pal[jcBOXFG], -1, "%d/9", FavoriteComboPage + 1);

			draw_text_button(menu1, favorites_pgleft.x, favorites_pgleft.y, favorites_pgleft.w, favorites_pgleft.h, is_compact ? "<" : "<-", vc(1), vc(14), 0, true);
			draw_text_button(menu1, favorites_pgright.x, favorites_pgright.y, favorites_pgright.w, favorites_pgright.h, is_compact ? ">" : "->", vc(1), vc(14), 0, true);
			draw_text_button(menu1,favorites_zoombtn.x,favorites_zoombtn.y,favorites_zoombtn.w,favorites_zoombtn.h,zoomed ? "-" : "+",vc(1),vc(14),0,true);
			draw_text_button(menu1,favorites_x.x,favorites_x.y,favorites_x.w,favorites_x.h,"X",vc(1),vc(14),0,true);
			draw_text_button(menu1,favorites_infobtn.x,favorites_infobtn.y,favorites_infobtn.w,favorites_infobtn.h,"?",vc(1),vc(14),0,true);
		}
		break;
		case rCOMMANDS:
		{
			jwin_draw_frame(menu1,commands_window.x,commands_window.y,commands_window.w,commands_window.h, FR_WIN);
			rectfill(menu1,commands_window.x+2,commands_window.y+2,commands_window.x+commands_window.w-3,commands_window.y+commands_window.h-3,jwin_pal[jcBOX]);
			jwin_draw_frame(menu1,commands_list.x-2,commands_list.y-2,(commands_list.w*commands_list.xscale)+4,(commands_list.h*commands_list.yscale)+4, FR_DEEP);
			rectfill(menu1,commands_list.x,commands_list.y,commands_list.x+(commands_list.w*commands_list.xscale)-1,commands_list.y+(commands_list.h*commands_list.yscale)-1,jwin_pal[jcBOXFG]);
			font=get_custom_font(CFONT_FAVCMD);
			
			for(int32_t cmd=0; cmd<(commands_list.w*commands_list.h); ++cmd)
			{
				uint hkey = favorite_commands[cmd];
				draw_layer_button(menu1,
					(cmd%commands_list.w)*commands_list.xscale+commands_list.x,
					(cmd/commands_list.w)*commands_list.yscale+commands_list.y,
					commands_list.xscale,
					commands_list.yscale,
					get_hotkey_name(hkey),
					(selected_hotkey(hkey)?D_SELECTED:0) | (disabled_hotkey(hkey)?D_DISABLED:0));
			}
			
			font = get_zc_font(font_lfont_l);
			if(commands_txt.x > 0)
			{
				gui_textout_ln(menu1, get_zc_font(font_lfont_l), (ucc*)"Favorite Commands", commands_txt.x, commands_txt.y, jwin_pal[jcBOXFG], -1, 0);
			}
			
			bool zoomed = is_compact ? compact_zoomed_cmd : large_zoomed_cmd;
			draw_text_button(menu1,commands_zoombtn.x,commands_zoombtn.y,commands_zoombtn.w,commands_zoombtn.h,zoomed ? "-" : "+",vc(1),vc(14),0,true);
			draw_text_button(menu1,commands_x.x,commands_x.y,commands_x.w,commands_x.h,"X",vc(1),vc(14),0,true);
			draw_text_button(menu1,commands_infobtn.x,commands_infobtn.y,commands_infobtn.w,commands_infobtn.h,"?",vc(1),vc(14),0,true);
		}
		break;
	}
	font = tfont;
}

bool pause_refresh = true;
bool is_refreshing = false;
void refresh(int32_t flags, bool update)
{
	if(pause_refresh) return;
	static bool refreshing = false;

	int num_screens_to_draw = Map.getViewSize();

	bool earlyret = refreshing;
	is_refreshing = refreshing = true;
	//^ These prevent recursive calls from updating the screen early
	
	bool zoom_delay = (zoomed_minimap && flags != rSCRMAP);
	if(zoom_delay)
		flags &= ~rSCRMAP;
	
	if(flags&rCLEAR)
	{
		//magic pink = 0xED
		//system black = vc(0)
		//Clear a4 menu
		clear_to_color(menu1,jwin_pal[jcBOX]);
		
		//Clears should refresh everything!
		flags |= rALL;
	}
	
	if(flags&rSCRMAP)
		draw_screenunit(rSCRMAP,flags);
	
	if(flags&rMAP)
		draw_screenunit(rMAP,flags);
	
	if((flags&rCOMBOS) || (draw_mode == dm_cpool && (flags&rFAVORITES)))
		draw_screenunit(rCOMBOS,flags);
	
	if(flags&(rCOMBO|rCOMBOS))
		draw_screenunit(rCOMBO,flags);
	
	if(flags&rMENU)
		drawpanel();
	
	if(flags&rFAVORITES)
		draw_screenunit(rFAVORITES,flags);
	
	if(flags&rCOMMANDS)
		draw_screenunit(rCOMMANDS,flags);
	
	FONT* tfont = font;
	font = get_custom_font(CFONT_GUI);
	jwin_draw_frame(menu1,layer_panel.x,layer_panel.y,layer_panel.w,layer_panel.h,FR_DEEP);
	rectfill(menu1,layer_panel.x,layer_panel.y,layer_panel.x+layer_panel.w-1,layer_panel.y+layer_panel.h-1,jwin_pal[jcBOX]);
	
	for(int32_t i=0; i<=6; ++i)
	{
		char tbuf[15];

		if (i>0 && mapscreen_valid_layers[i - 1] && num_screens_to_draw == 1)
		{
			if(is_compact)
				sprintf(tbuf, "%s%d %d:%02X", (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"", i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
			else sprintf(tbuf, "%s%d (%d:%02X)", (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"", i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
		}
		else
		{
			sprintf(tbuf, "%s%d", (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"", i);
		}
		
		int32_t spacing_offs = is_compact ? 2 : 10;
		int32_t rx = (i * (layerpanel_buttonwidth+spacing_offs+layerpanel_checkbox_wid)) + layer_panel.x+(is_compact?2:6);
		int32_t ry = layer_panel.y;
		auto cbyofs = (layerpanel_buttonheight-layerpanel_checkbox_hei)/2;
		draw_layer_button(menu1, rx, ry, layerpanel_buttonwidth, layerpanel_buttonheight, tbuf, CurrentLayer==i? D_SELECTED : ( i > 0 && !mapscreen_valid_layers[i-1]) ? D_DISABLED : 0);
		draw_checkbox(menu1,rx+layerpanel_buttonwidth+1,ry+cbyofs,layerpanel_checkbox_wid,layerpanel_checkbox_hei,LayerMaskInt[i]!=0);
	}
	
	font=tfont;
	
	// } //if(true)
	if(zq_showpal)
	{
		for(int32_t i=0; i<256; i++)
		{
			rectfill(menu1,((i&15)<<2)+256,((i>>4)<<2)+176,((i&15)<<2)+259,((i>>4)<<2)+179,i);
		}
	}
	{ //Show top-left info
		size_t maxwid = (mapscreen_screenunit_scale*mapscreenbmp->w)-1;
		size_t maxhei = (mapscreen_screenunit_scale*mapscreenbmp->w);
		set_clip_rect(menu1,mapscreen_x,mapscreen_y,mapscreen_x+maxwid-1,mapscreen_y+maxhei-1);
		FONT* showfont = get_custom_font(CFONT_INFO);
		int showfont_h = text_height(showfont);
		int32_t ypos = mapscreen_y;
		if(prv_mode)
		{
			textout_shadowed_ex(menu1,showfont,"Preview Mode",0,ypos,vc(15),vc(0),infobg?vc(0):-1);
			ypos += showfont_h+1;
			if(prv_twon)
			{
				textprintf_shadowed_ex(menu1,showfont,0,ypos,vc(15),vc(0),infobg?vc(0):-1,"T Warp=%d tics", Map.get_prvtime());
				ypos += showfont_h+1;
			}
		}
		if(ShowFPS)
		{
			textprintf_shadowed_ex(menu1,showfont,0,ypos,vc(15),vc(0),infobg?vc(0):-1,"FPS: %3d",lastfps);
			ypos += showfont_h+1;
		}
		
		if(ShowFFScripts && !prv_mode)
		{
			word num_ffcs = Map.CurrScr()->numFFC();
			for(word i=0; i< num_ffcs; i++)
			{
				if(ypos+showfont_h-1 > map_page_bar[0].y)
					break;
				if(Map.CurrScr()->ffcs[i].script && Map.CurrScr()->ffcs[i].data)
				{
					textout_shadowed_ex(menu1, showfont, ffcmap[Map.CurrScr()->ffcs[i].script-1].scriptname.substr(0,300).c_str(),0,ypos,vc(showxypos_ffc==i ? 14 : 15),vc(0),infobg?vc(0):-1);
					ypos+=showfont_h+1;
				}
			}
		}
		clear_clip_rect(menu1);
		if(prv_mode)
			do_previewtext();
	}
	// Show Errors & Details
	//This includes the presence of: Screen State Carryover, Timed Warp, Maze Path, the 'Sideview Gravity', 'Invisible Hero',
	//'Save Screen', 'Continue Here' and 'Treat As..' Screen Flags,
	// the String, every Room Type and Catch All, and all four Tile and Side Warps.
	if(!prv_mode && ShowInfo)
	{
		int32_t i=0;
		char buf[2048];
		
		// Start with general information
		if(Map.CurrScr()->flags3&fINVISHERO)
		{
			sprintf(buf,"Invisible Hero");
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.getLayerTargetMap() > 0)
		{
			Map.setlayertarget(); //Now the text does not carry over when changing maps, but shifting back, it does not **re-appear** until you change screens.
			//It was also required to set some updates in onDecMap and onIncMap. #
			//This fixes Screen Info not displaying properly when changing maps. -Z 
			//Needed to refresh the screen info. -Z ( 26th March, 2019 )
			int32_t m = Map.getLayerTargetMultiple();
			sprintf(buf,"Used as a layer by screen %d:%02X",Map.getLayerTargetMap(),Map.getLayerTargetScr());
			char buf2[24];
			
			if(m>0)
			{
				sprintf(buf2," and %d other%s",m,m>1?"s":"");
				strcat(buf,buf2);
			}
			
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->nextmap)
		{
			sprintf(buf,"Screen State carries over to %d:%02X",Map.CurrScr()->nextmap,Map.CurrScr()->nextscr);
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->timedwarptics)
		{
			sprintf(buf,"%s%sTimed Warp: %s",(Map.CurrScr()->flags4&fTIMEDDIRECT)?"Direct ":"",(Map.CurrScr()->flags5&fRANDOMTIMEDWARP)?"Random ":"",ticksstr(Map.CurrScr()->timedwarptics));
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->flags&fMAZE)
		{
			sprintf(buf,"Maze Path: %s (Exit %s)",pathstr(Map.CurrScr()->path),mazedirstr[Map.CurrScr()->exitdir]);
			show_screen_error(buf,i++,vc(15));
		}
		
		bool continuescreen = false, savecombo = false;
		
		if(Map.CurrScr()->flags4&fAUTOSAVE)
		{
			sprintf(buf,"Automatic Save%s Screen", (Map.CurrScr()->flags6&fCONTINUEHERE) ? "-Continue":"");
			show_screen_error(buf,i++,vc(15));
			continuescreen = ((Map.CurrScr()->flags6&fCONTINUEHERE)!=0);
			savecombo = true;
		}
		else if(Map.CurrScr()->flags6&fCONTINUEHERE)
		{
			sprintf(buf,"Continue Screen");
			show_screen_error(buf,i++,vc(15));
			continuescreen = true;
		}
		
		if(isSideViewGravity())
		{
			sprintf(buf,"Sideview Gravity");
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->flags6 & (fCAVEROOM|fDUNGEONROOM))
		{
			sprintf(buf,"Treat As %s%s Screen", (Map.CurrScr()->flags6&fCAVEROOM) ? "Interior":"NES Dungeon",
					(Map.CurrScr()->flags6 & (fCAVEROOM|fDUNGEONROOM)) == (fCAVEROOM|fDUNGEONROOM) ? " or NES Dungeon":"");
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->oceansfx != 0)
		{
			sprintf(buf,"Ambient Sound: %s",sfx_string[Map.CurrScr()->oceansfx]);
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->bosssfx != 0)
		{
			sprintf(buf,"Boss Roar Sound: %s",sfx_string[Map.CurrScr()->bosssfx]);
			show_screen_error(buf,i++,vc(15));
		}
		
		if(Map.CurrScr()->str)
		{
			strncpy(buf,MsgString(Map.CurrScr()->str, true, false),72);
			buf[72] = '\0';
			char shortbuf[72];
			strip_extra_spaces(buf);
			shorten_string(shortbuf, buf, get_zc_font(font_lfont_l), 72, 280);
			sprintf(buf,"String %s",shortbuf);
			show_screen_error(buf,i++,vc(15));
		}
		
		if((Map.CurrScr()->flags&fWHISTLE) || (Map.CurrScr()->flags7&fWHISTLEWATER))
		{
			sprintf(buf,"Whistle ->%s%s%s",(Map.CurrScr()->flags&fWHISTLE)?" Stairs":"",
				    (Map.CurrScr()->flags&fWHISTLE && Map.CurrScr()->flags7&fWHISTLEWATER)?", ":"",
			        (Map.CurrScr()->flags7&fWHISTLEWATER)?"Dry Lake":"");
			show_screen_error(buf,i++,vc(15));
		}
		
		switch(Map.CurrScr()->room)
		{
			case rSP_ITEM:
				sprintf(buf,"Special Item is %s",item_string[Map.CurrScr()->catchall]);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rINFO:
			{
				int32_t shop = Map.CurrScr()->catchall;
				sprintf(buf,"Pay For Info: -%d, -%d, -%d",
						QMisc.info[shop].price[0],QMisc.info[shop].price[1],QMisc.info[shop].price[2]);
				show_screen_error(buf,i++, vc(15));
			}
			break;
			
			case rMONEY:
				sprintf(buf,"Secret Money: %d Rupees",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rGAMBLE:
				show_screen_error("Gamble Room",i++, vc(15));
				break;
				
			case rREPAIR:
				sprintf(buf,"Door Repair: -%d Rupees",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rRP_HC:
				sprintf(buf,"Take %s or %s", item_string[iRPotion], item_string[iHeartC]);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rGRUMBLE:
				show_screen_error("Feed the Goriya",i++, vc(15));
				break;
				
			case rTRIFORCE:
				show_screen_error("Triforce Check",i++, vc(15));
				break;
				
			case rP_SHOP:
			case rSHOP:
			{
				int32_t shop = Map.CurrScr()->catchall;
				sprintf(buf,"%sShop: ",
						Map.CurrScr()->room==rP_SHOP ? "Potion ":"");
						
				for(int32_t j=0; j<3; j++) if(QMisc.shop[shop].item[j]>0)  // Print the 3 items and prices
				{
					strcat(buf,item_string[QMisc.shop[shop].item[j]]);
					strcat(buf,":");
					char pricebuf[8];
					sprintf(pricebuf,"%d",QMisc.shop[shop].price[j]);
					strcat(buf,pricebuf);
					
					if(j<2 && QMisc.shop[shop].item[j+1]>0) strcat(buf,", ");
				}
					
				show_screen_error(buf,i++, vc(15));
			}
			break;
			
			case rBOTTLESHOP:
			{
				int32_t shop = Map.CurrScr()->catchall;
				sprintf(buf,"Bottle Shop: ");
						
				for(int32_t j=0; j<3; j++) if(QMisc.bottle_shop_types[shop].fill[j]>0)  // Print the 3 fills and prices
				{
					strcat(buf,QMisc.bottle_types[QMisc.bottle_shop_types[shop].fill[j]-1].name);
					strcat(buf,":");
					char pricebuf[8];
					sprintf(pricebuf,"%d",QMisc.bottle_shop_types[shop].price[j]);
					strcat(buf,pricebuf);
					
					if(j<2 && QMisc.bottle_shop_types[shop].fill[j+1]>0) strcat(buf,", ");
				}
					
				show_screen_error(buf,i++, vc(15));
			}
			break;
			
			case rTAKEONE:
			{
				int32_t shop = Map.CurrScr()->catchall;
				sprintf(buf,"Take Only One: %s%s%s%s%s",
						QMisc.shop[shop].item[0]<1?"":item_string[QMisc.shop[shop].item[0]],QMisc.shop[shop].item[0]>0?", ":"",
						QMisc.shop[shop].item[1]<1?"":item_string[QMisc.shop[shop].item[1]],(QMisc.shop[shop].item[1]>0&&QMisc.shop[shop].item[2]>0)?", ":"",
						QMisc.shop[shop].item[2]<1?"":item_string[QMisc.shop[shop].item[2]]);
				show_screen_error(buf,i++, vc(15));
			}
			break;
			
			case rBOMBS:
				sprintf(buf,"More Bombs: -%d Rupees",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rARROWS:
				sprintf(buf,"More Arrows: -%d Rupees",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case rSWINDLE:
				sprintf(buf,"Leave Life or %d Rupees",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
				
			case r10RUPIES:
				show_screen_error("10 Rupees",i++, vc(15));
				break;
				
			case rGANON:
				show_screen_error("Ganon Room",i++, vc(15));
				break;
				
			case rZELDA:
				show_screen_error("Zelda Room",i++, vc(15));
				break;
				
			case rMUPGRADE:
				show_screen_error("1/2 Magic Upgrade",i++, vc(15));
				break;
				
			case rLEARNSLASH:
				show_screen_error("Learn Slash",i++, vc(15));
				break;
				
			case rWARP:
				sprintf(buf,"3-Stair Warp: Warp Ring %d",Map.CurrScr()->catchall);
				show_screen_error(buf,i++, vc(15));
				break;
		}
		
		bool undercombo = false, warpa = false, warpb = false, warpc = false, warpd = false, warpr = false;
		
		word num_ffcs = Map.CurrScr()->numFFC();
		for(int32_t c=0; c<176+128+1+num_ffcs; ++c)
		{
			// Checks both combos, secret combos, undercombos and FFCs
			//Fixme:
			int32_t ctype =
				combobuf[vbound(
					(c>=305 ? Map.CurrScr()->ffcs[c-305].data :
					 c>=304 ? Map.CurrScr()->undercombo :
					 c>=176 ? Map.CurrScr()->secretcombo[c-176] :
					 !Map.CurrScr()->valid ? 0 : // Sanity check: does room combo data exist?
					 Map.CurrScr()->data[c]
					), 0, MAXCOMBOS-1)].type;
							 
			if(!undercombo && integrityBoolUnderCombo(Map.CurrScr(),ctype))
			{
				undercombo = true;
				show_screen_error("Under Combo is combo 0",i++, vc(7));
			}
			
			// Tile Warp types
			switch(ctype)
			{
				case cSAVE:
				case cSAVE2:
					if(!savecombo)
					{
						savecombo = true;
						
						if(integrityBoolSaveCombo(Map.CurrScr(),ctype))
							show_screen_error("Save Screen",i++, vc(15));
						else
							show_screen_error("Save-Continue Screen",i++, vc(15));
					}
					
					break;
					
				case cSTAIRR:
				case cPITR:
				case cSWARPR:
					if(!warpr && (Map.CurrScr()->tilewarptype[0]==wtCAVE || Map.CurrScr()->tilewarptype[1]==wtCAVE ||
								  Map.CurrScr()->tilewarptype[2]==wtCAVE || Map.CurrScr()->tilewarptype[3]==wtCAVE))
					{
						warpr = true;
						show_screen_error("Random Tile Warp contains Cave/Item Cellar",i++, vc(7));
					}
					
					break;
					
				case cCAVED:
				case cPITD:
				case cSTAIRD:
				case cCAVE2D:
				case cSWIMWARPD:
				case cDIVEWARPD:
				case cSWARPD:
					if(!warpd)
					{
						warpd = true;
						tile_warp_notification(3,buf);
						show_screen_error(buf,i++, vc(15));
					}
					
					break;
					
				case cCAVEC:
				case cPITC:
				case cSTAIRC:
				case cCAVE2C:
				case cSWIMWARPC:
				case cDIVEWARPC:
				case cSWARPC:
					if(!warpc)
					{
						warpc = true;
						tile_warp_notification(2,buf);
						show_screen_error(buf,i++, vc(15));
					}
					
					break;
					
				case cCAVEB:
				case cPITB:
				case cSTAIRB:
				case cCAVE2B:
				case cSWIMWARPB:
				case cDIVEWARPB:
				case cSWARPB:
					if(!warpb)
					{
						warpb = true;
						tile_warp_notification(1,buf);
						show_screen_error(buf,i++, vc(15));
					}
					
					break;
					
				case cCAVE:
				case cPIT:
				case cSTAIR:
				case cCAVE2:
				case cSWIMWARP:
				case cDIVEWARP:
				case cSWARPA:
					if(!warpa)
					{
						warpa = true;
						tile_warp_notification(0,buf);
						show_screen_error(buf,i++, vc(15));
					}
					
					break;
			}
		}
		
		int32_t sidewarpnotify = 0;
		
		if(Map.CurrScr()->flags2&wfUP)
		{
			side_warp_notification(Map.CurrScr()->sidewarpindex&3,0,buf);
			show_screen_error(buf,i++, vc(15));
			sidewarpnotify|=(1<<(Map.CurrScr()->sidewarpindex&3));
		}
		
		if(Map.CurrScr()->flags2&wfDOWN)
		{
			side_warp_notification((Map.CurrScr()->sidewarpindex>>2)&3,1,buf);
			show_screen_error(buf,i++, vc(15));
			sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>2)&3));
		}
		
		if(Map.CurrScr()->flags2&wfLEFT)
		{
			side_warp_notification((Map.CurrScr()->sidewarpindex>>4)&3,2,buf);
			show_screen_error(buf,i++, vc(15));
			sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>4)&3));
		}
		
		if(Map.CurrScr()->flags2&wfRIGHT)
		{
			side_warp_notification((Map.CurrScr()->sidewarpindex>>6)&3,3,buf);
			show_screen_error(buf,i++, vc(15));
			sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>6)&3));
		}
		
		if(!(sidewarpnotify&1) && Map.CurrScr()->timedwarptics)
		{
			side_warp_notification(0,4,buf); // Timed Warp
			show_screen_error(buf,i++, vc(15));
		}
		
		// Now for errors
		if((Map.CurrScr()->flags4&fSAVEROOM) && !savecombo) show_screen_error("Save Point->Continue Here, but no Save Point combo?",i++, vc(14));
		
		if(integrityBoolEnemiesItem(Map.CurrScr())) show_screen_error("Enemies->Item, but no enemies",i++, vc(7));
		
		if(integrityBoolEnemiesSecret(Map.CurrScr())) show_screen_error("Enemies->Secret, but no enemies",i++, vc(7));
		
		if(integrityBoolGuyNoString(Map.CurrScr())) show_screen_error("Non-Fairy Guy, but String is (none)",i++, vc(14));
		
		if(integrityBoolRoomNoGuy(Map.CurrScr())) show_screen_error("Guy is (none)",i++, vc(14));
		
		if(integrityBoolRoomNoString(Map.CurrScr())) show_screen_error("String is (none)",i++, vc(14));
		
		if(integrityBoolRoomNoGuyNoString(Map.CurrScr())) show_screen_error("Guy and String are (none)",i++, vc(14));
	}
	
	if(zoom_delay)
		draw_screenunit(rSCRMAP,flags);
	
	
	if(flags&rCLEAR)
	{
		//Draw the whole gui
		blit(menu1,screen,0,0,0,0,zq_screen_w,zq_screen_h);
	}
	else
	{
		blit(menu1,screen,0,16,0,16,zq_screen_w,zq_screen_h-16);
		blit(menu1,screen,combolist_window.x-64,0,combolist_window.x-64,0,combolist_window.w+64,16);
		
		if(flags&rCOMBO)
			blit(menu1,screen,combo_preview.x,combo_preview.y,combo_preview.x,combo_preview.y,combo_preview.w,combo_preview.h);
	}
		
	if(earlyret)
		return;
	
	//Draw the Main Menu
	rectfill(screen,mainbar.x,mainbar.y,mainbar.x+mainbar.w-1,mainbar.y+mainbar.h-1,jwin_pal[jcBOX]);
	jwin_draw_frame(screen,mainbar.x,mainbar.y,mainbar.w,mainbar.h,FR_WIN);
	
	FONT* oldfont = font;
	font = get_custom_font(CFONT_GUI);
	
	//Drawmode button
	draw_text_button(screen,drawmode_btn.x,drawmode_btn.y,drawmode_btn.w,drawmode_btn.h,dm_names[draw_mode],vc(1),vc(14),0,true);
	//Compact button
	draw_text_button(screen,compactbtn.x, compactbtn.y, compactbtn.w, compactbtn.h, is_compact ? "< Expand" : "> Compact", vc(1),vc(14),0,true);
	//Zoom buttons
	zoom_in_btn_disabled = num_screens_to_draw == 1;
	zoom_out_btn_disabled = num_screens_to_draw == mapscreen_num_screens_to_draw_max;
	draw_text_button(screen,zoominbtn.x, zoominbtn.y, zoominbtn.w, zoominbtn.h, "+", vc(1),vc(14),zoom_in_btn_disabled ? D_DISABLED : 0,true);
	draw_text_button(screen,zoomoutbtn.x, zoomoutbtn.y, zoomoutbtn.w, zoomoutbtn.h, "-", vc(1),vc(14),zoom_out_btn_disabled ? D_DISABLED : 0,true);

	font = oldfont;
	
	d_nbmenu_proc(MSG_DRAW, &dialogs[0], 0);
	
	ComboBrushPause=0;
	
	SCRFIX();
	if(update)
		custom_vsync();
	is_refreshing = refreshing = false;
}

static int minimap_tooltip_id = ttip_register_id();

void select_scr()
{
	if(Map.getCurrMap()>=Map.getMapCount())
		return;
		
	int32_t tempcb=ComboBrush;
	ComboBrush=0;
	
	size_and_pos const& real_mini = zoomed_minimap ? real_minimap_zoomed : real_minimap;
	
	//scooby
	while(gui_mouse_b())
	{
		int32_t x=gui_mouse_x();
		int32_t y=gui_mouse_y();
		
		int32_t ind = real_mini.rectind(x,y);

		if(ind>=MAPSCRS)
			ind-=16;
			
		if(ind > -1 && ind != Map.getCurrScr())
		{
			Map.setCurrScr(ind);
		}
		
		custom_vsync();
		refresh(rALL);
	}
	
	ComboBrush=tempcb;
}

void clear_cpool()
{
	for(int32_t i=0; i<MAXFAVORITECOMBOS; ++i)
	{
		pool_combos[i].clear();
	}
	pool_dirty = true;
}

bool select_favorite()
{
    int32_t tempcb=ComboBrush;
    ComboBrush=0;
    bool valid=false;
    
    while(gui_mouse_b())
    {
        valid=false;
        int32_t x=gui_mouse_x();
        
        if(x<favorites_list.x)
			x=favorites_list.x;
        
        if(x>favorites_list.x+(favorites_list.w*favorites_list.xscale)-1)
			x=favorites_list.x+(favorites_list.w*favorites_list.xscale)-1;
        
        int32_t y=gui_mouse_y();
        
        if(y<favorites_list.y)
			y=favorites_list.y;
        
        if(y>favorites_list.y+(favorites_list.h*favorites_list.yscale)-1)
			y=favorites_list.y+(favorites_list.h*favorites_list.yscale)-1;
        
        int32_t tempc=(((y-favorites_list.y)/favorites_list.yscale)*FAVORITECOMBO_PER_ROW)+((x-favorites_list.x)/favorites_list.xscale) + FAVORITECOMBO_PER_PAGE * FavoriteComboPage;

		if(tempc >=  MAXFAVORITECOMBOS)
		{
			//Nothing, invalid
		}
        else
        {
			if(favorite_combos[tempc]!=-1)
            {
				switch(favorite_combo_modes[tempc])
				{
					case dm_alias:
						draw_mode = dm_alias;
						combo_apos = favorite_combos[tempc];
						break;
					case dm_cpool:
						draw_mode = dm_cpool;
						combo_pool_pos = favorite_combos[tempc];
						break;
					case dm_auto:
						draw_mode = dm_auto;
						combo_auto_pos = favorite_combos[tempc];
						break;
					default:
						draw_mode = dm_normal;
						Combo = favorite_combos[tempc];
				}
				if(AutoBrush)
					BrushWidth = BrushHeight = 1;
                valid=true;
				fix_drawing_mode_menu();
            }
        }
        
        custom_vsync();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
    return valid;
}

void select_combo(int32_t clist)
{
    current_combolist=clist;
    int32_t tempcb=ComboBrush;
    ComboBrush=0;
    
	int autobrush_cx = -1, autobrush_cy = -1;
	int autobrush_first = First[current_combolist];
	auto& curlist = combolist[current_combolist];
	AutoBrushRevert = (key[KEY_ALT]||key[KEY_ALTGR]);
    while(gui_mouse_b())
    {
        int32_t x=gui_mouse_x();
        
        if(x<curlist.x)
			x=curlist.x;
        
        if(x>curlist.x+(curlist.w*curlist.xscale)-1)
			x=curlist.x+(curlist.w*curlist.xscale)-1;
        
        int32_t y=gui_mouse_y();
        
        if(y<curlist.y)
			y=curlist.y;
        
        if(y>curlist.y+(curlist.h*curlist.yscale)-1)
			y=curlist.y+(curlist.h*curlist.yscale)-1;
        
		int cx = ((x-curlist.x)/curlist.xscale), cy = ((y-curlist.y)/curlist.yscale);
        if(AutoBrush)
		{
			if(autobrush_cx < 0)
			{
				autobrush_cx = cx;
				autobrush_cy = cy;
			}
			BrushWidth = vbound(abs(autobrush_cx-cx)+1,1,16);
			BrushHeight = vbound(abs(autobrush_cy-cy)+1,1,11);
			cx = std::min(autobrush_cx,cx);
			cy = std::min(autobrush_cy,cy);
		}
		Combo=(cy*curlist.w)+cx+First[current_combolist];
		custom_vsync();
        refresh(rALL);
		if(AutoBrush) //Prevent any scrolling
			First[current_combolist] = autobrush_first;
    }
    if(key[KEY_ALT]||key[KEY_ALTGR])
		AutoBrushRevert = true;
	position_mouse_z(0);
    ComboBrush=tempcb;
}

void select_comboa(int32_t clist)
{
    current_comboalist=clist;
    int32_t tempcb=ComboBrush;
    ComboBrush=0;
    alias_cset_mod=0;
    
	auto& curlist = comboaliaslist[current_comboalist];
    while(gui_mouse_b())
    {
        int32_t x=gui_mouse_x();
        
        if(x<curlist.x)
			x=curlist.x;
        
        if(x>curlist.x+(curlist.w*curlist.xscale)-1)
			x=curlist.x+(curlist.w*curlist.xscale)-1;
        
        int32_t y=gui_mouse_y();
        
        if(y<curlist.y)
			y=curlist.y;
        
        if(y>curlist.y+(curlist.h*curlist.yscale)-1)
			y=curlist.y+(curlist.h*curlist.yscale)-1;
        
        combo_apos=(((y-curlist.y)/curlist.yscale)*curlist.w)+((x-curlist.x)/curlist.xscale)+combo_alistpos[current_comboalist];
        custom_vsync();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
}

void select_combop(int32_t clist)
{
    current_cpoollist=clist;
    int32_t tempcb=ComboBrush;
    ComboBrush=0;
    
	auto& curlist = comboaliaslist[current_cpoollist];
    while(gui_mouse_b())
    {
        int32_t x=gui_mouse_x();
        
        if(x<curlist.x) x=curlist.x;
        
        if(x>curlist.x+(curlist.w*curlist.xscale)-1)
			x=curlist.x+(curlist.w*curlist.xscale)-1;
        
        int32_t y=gui_mouse_y();
        
        if(y<curlist.y) y=curlist.y;
        
        if(y>curlist.y+(curlist.h*curlist.yscale)-1)
			y=curlist.y+(curlist.h*curlist.yscale)-1;
        
        combo_pool_pos=(((y-curlist.y)/curlist.yscale)*curlist.w)+((x-curlist.x)/curlist.xscale)+combo_pool_listpos[current_cpoollist];
        custom_vsync();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
}

void select_autocombo(int32_t clist)
{
	current_cautolist = clist;
	int32_t tempcb = ComboBrush;
	ComboBrush = 0;

	auto& curlist = comboaliaslist[current_cautolist];
	while (gui_mouse_b())
	{
		int32_t x = gui_mouse_x();

		if (x < curlist.x) x = curlist.x;

		if (x > curlist.x + (curlist.w * curlist.xscale) - 1)
			x = curlist.x + (curlist.w * curlist.xscale) - 1;

		int32_t y = gui_mouse_y();

		if (y < curlist.y) y = curlist.y;

		if (y > curlist.y + (curlist.h * curlist.yscale) - 1)
			y = curlist.y + (curlist.h * curlist.yscale) - 1;

		combo_auto_pos = (((y - curlist.y) / curlist.yscale) * curlist.w) + ((x - curlist.x) / curlist.xscale) + combo_auto_listpos[current_cautolist];
		cauto_height = combo_autos[combo_auto_pos].getArg() + 1;
		custom_vsync();
		refresh(rALL);
	}

	ComboBrush = tempcb;
}

void update_combobrush()
{
    clear_bitmap(brushbmp);
    
    if(draw_mode==dm_alias)
    {
        //int32_t count=(combo_aliases[combo_apos].width+1)*(combo_aliases[combo_apos].height+1)*(comboa_lmasktotal(combo_aliases[combo_apos].layermask));
        for(int32_t z=0; z<=comboa_lmasktotal(combo_aliases[combo_apos].layermask); z++)
        {
            for(int32_t y=0; y<=combo_aliases[combo_apos].height; y++)
            {
                for(int32_t x=0; x<=combo_aliases[combo_apos].width; x++)
                {
                    int32_t position = ((y*(combo_aliases[combo_apos].width+1))+x)+((combo_aliases[combo_apos].width+1)*(combo_aliases[combo_apos].height+1)*z);
                    
                    if(combo_aliases[combo_apos].combos[position])
                    {
                        if(z==0)
                        {
                            putcombo(brushbmp,x<<4,y<<4,combo_aliases[combo_apos].combos[position],wrap(combo_aliases[combo_apos].csets[position]+alias_cset_mod, 0, 13));
                        }
                        else
                        {
                            overcombo(brushbmp,x<<4,y<<4,combo_aliases[combo_apos].combos[position],wrap(combo_aliases[combo_apos].csets[position]+alias_cset_mod, 0, 13));
                        }
                    }
                }
            }
        }
        
		int xoff = 6, yoff = 6;
		if(FloatBrush) // Offset the floating pixels, so the 'x' appears centered on the combo still -Em
		{
			xoff += 2;
			yoff += 2;
		}
		if(alias_origin & 1) // Right-align
			xoff += combo_aliases[combo_apos].width*16;
		if(alias_origin & 2) // Bottom-align
			yoff += combo_aliases[combo_apos].height*16;
		
		textprintf_shadowed_ex(brushbmp, get_zc_font(font_sfont), xoff, yoff, vc(15), vc(0), -1, "x");
    }
    else if(draw_mode != dm_cpool)
    {
		int32_t cid = combobrushoverride > -1 ? combobrushoverride : Combo;
		int32_t c = 0;
		
		for(int32_t i=0; i<256; i++)
		{
			if(unsigned(cid+c) >= MAXCOMBOS) break;
			if(((i%COMBOS_PER_ROW)<BrushWidth)&&((i/COMBOS_PER_ROW)<BrushHeight))
			{
				put_combo(brushbmp,(i%COMBOS_PER_ROW)<<4,(i/COMBOS_PER_ROW)<<4,cid+c,CSet,Flags&(cFLAGS|cWALK),0);
			}
			
			if(((cid+c)&3)==3)
				c+=48;
			
			++c;
				
			if((i%COMBOS_PER_ROW)==(COMBOS_PER_ROW-1))
				c-=256;
		}
    }
}

byte relational_source_grid[256]=
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 16, 17, 17, 18, 18, 19, 19, 16, 16, 17, 17, 18, 18, 19, 19,
    20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23,
    24, 24, 24, 24, 25, 25, 25, 25, 24, 24, 24, 24, 25, 25, 25, 25,
    26, 27, 26, 27, 26, 27, 26, 27, 28, 29, 28, 29, 28, 29, 28, 29,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    31, 31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32,
    33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
    34, 35, 36, 37, 34, 35, 36, 37, 34, 35, 36, 37, 34, 35, 36, 37,
    38, 38, 39, 39, 38, 38, 39, 39, 38, 38, 39, 39, 38, 38, 39, 39,
    40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
    41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
    42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
    46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46
};

static void draw_autocombo(ComboPosition combo_pos, bool rclick, bool pressframe = false)
{
	combo_auto &ca = combo_autos[combo_auto_pos];
	int scr = Map.getScreenForPosition(combo_pos);
	int pos = combo_pos.truncate();

	if (ca.valid())
	{
		switch (ca.getType())
		{
			case AUTOCOMBO_BASIC:
			{
				AutoPattern::autopattern_basic ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_Z1:
			{
				AutoPattern::autopattern_flatmtn ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_FENCE:
			{
				AutoPattern::autopattern_fence ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_Z4:
			{
				AutoPattern::autopattern_cakemtn ap(ca.getType(), CurrentLayer, scr, pos, &ca, cauto_height);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_RELATIONAL:
			{
				AutoPattern::autopattern_relational ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_DGNCARVE:
			{
				AutoPattern::autopattern_dungeoncarve ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_DOR:
			{
				AutoPattern::autopattern_dormtn ap(ca.getType(), CurrentLayer, scr, pos, &ca, cauto_height);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_TILING:
			{
				if (pressframe && (key[KEY_LSHIFT] || key[KEY_RSHIFT]))
				{
					int32_t x = (scr % 16) * 16 + (pos % 16);
					int32_t y = (scr / 16) * 11 + (pos / 16);
					byte w = (ca.getArg() & 0xF) + 1;
					byte h = ((ca.getArg() >> 4) & 0xF) + 1;
					ca.setOffsets(x % w, y % h);
				}
				AutoPattern::autopattern_tiling ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_REPLACE:
			{
				AutoPattern::autopattern_replace ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_DENSEFOREST:
			{
				if (pressframe && (key[KEY_LSHIFT] || key[KEY_RSHIFT]))
				{
					int32_t x = (scr % 16) * 16 + (pos % 16);
					int32_t y = (scr / 16) * 11 + (pos / 16);
					ca.setOffsets(x % 2, y % 2);
				}
				AutoPattern::autopattern_denseforest ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
			case AUTOCOMBO_EXTEND:
			{
				if (CHECK_CTRL_CMD)
					break;
				AutoPattern::autopattern_extend ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				if (rclick)
					ap.erase(scr, pos);
				else
					ap.execute(scr, pos);
				break;
			}
		}
	}
	else
	{
		ca.updateValid();
		if(!ca.valid())
			InfoDialog("Notice", "The autocombo you're trying to use is invalid. Reason:"
				+ ca.getInvalidReason()).show();
	}
}

static void draw_autocombo_command(ComboPosition combo_pos, int32_t cmd = 0, int32_t arg = 0)
{
	combo_auto ca = combo_autos[combo_auto_pos];
	int scr = Map.getScreenForPosition(combo_pos);
	int pos = combo_pos.truncate();

	if (ca.valid())
	{
		switch (ca.getType())
		{
			case AUTOCOMBO_FENCE:
			{
				AutoPattern::autopattern_fence ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				ap.flip_all_connected(scr, pos, 2048);
				break;
			}
			case AUTOCOMBO_Z4:
			{
				AutoPattern::autopattern_cakemtn ap(ca.getType(), CurrentLayer, scr, pos, &ca, cauto_height);
				switch (cmd)
				{
					case 0: // Flip
						ap.flip_all_connected(scr, pos, 2048);
						break;
					case 1: // Grow
						ap.resize_connected(scr, pos, 2048, vbound(arg, 1, 9));
						break;
				}
			}
		}
	}
}

static int32_t get_autocombo_floating_cid(ComboPosition combo_pos, bool clicked)
{
	combo_auto& ca = combo_autos[combo_auto_pos];
	int scr = Map.getScreenForPosition(combo_pos);
	int pos = combo_pos.truncate();
	int cid = 0;

	if (ca.valid() && Map.isValidPosition(mouse_combo_pos))
	{
		switch (ca.getType())
		{
			case AUTOCOMBO_BASIC:
			{
				AutoPattern::autopattern_basic ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}

			case AUTOCOMBO_Z1:
			{
				AutoPattern::autopattern_flatmtn ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_FENCE:
			{
				AutoPattern::autopattern_fence ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_Z4:
			{
				AutoPattern::autopattern_cakemtn ap(ca.getType(), CurrentLayer, scr, pos, &ca, cauto_height);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_RELATIONAL:
			{
				AutoPattern::autopattern_relational ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_DGNCARVE:
			{
				AutoPattern::autopattern_dungeoncarve ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_DOR:
			{
				AutoPattern::autopattern_dormtn ap(ca.getType(), CurrentLayer, scr, pos, &ca, cauto_height);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_TILING:
			{
				std::pair<byte, byte> offs = ca.getOffsets();
				if (!clicked && (key[KEY_LSHIFT] || key[KEY_RSHIFT]))
				{
					int32_t x = (scr % 16) * 16 + (pos % 16);
					int32_t y = (scr / 16) * 11 + (pos / 16);
					byte w = (ca.getArg() & 0xF) + 1;
					byte h = ((ca.getArg() >> 4) & 0xF) + 1;
					offs.first = (x % w);
					offs.second = (y % h);
				}
				AutoPattern::autopattern_tiling ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_REPLACE:
			{
				AutoPattern::autopattern_replace ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_DENSEFOREST:
			{
				AutoPattern::autopattern_denseforest ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
			case AUTOCOMBO_EXTEND:
			{
				AutoPattern::autopattern_extend ap(ca.getType(), CurrentLayer, scr, pos, &ca);
				cid = ap.get_floating_cid(scr, pos);
				break;
			}
		}
	}
	return cid;
}

void change_autocombo_height(int32_t change)
{
	bool can_change = false;
	if (draw_mode == dm_auto)
	{
		combo_auto ca = combo_autos[combo_auto_pos];
		switch (ca.getType())
		{
			case AUTOCOMBO_Z4:
				can_change = true;
				[[fallthrough]];
			case AUTOCOMBO_DOR:
				break;
			default:
				return;
		}
	}
	else
		return;

	int32_t x = gui_mouse_x();
	int32_t y = gui_mouse_y();
	double startx = mapscreen_x + (showedges ? (16 * mapscreen_single_scale) : 0);
	double starty = mapscreen_y + (showedges ? (16 * mapscreen_single_scale) : 0);
	int32_t startxint = mapscreen_x + (showedges ? int32_t(16 * mapscreen_single_scale) : 0);
	int32_t startyint = mapscreen_y + (showedges ? int32_t(16 * mapscreen_single_scale) : 0);
	ComboPosition pos = get_mapscreen_mouse_combo_pos();

	if (can_change && isinRect(x, y, startxint, startyint, int32_t(startx + (256 * mapscreen_single_scale) - 1), int32_t(starty + (176 * mapscreen_single_scale) - 1)))
	{
		Map.StartListCommand();
		draw_autocombo_command(pos, 1, cauto_height + change);
		Map.FinishListCommand();
	}
	cauto_height = vbound(cauto_height + change, 1, 9);
}

void draw(bool justcset)
{
	combo_pool const& pool = combo_pools[combo_pool_pos];
	if(draw_mode == dm_cpool && !pool.valid())
		return;
    saved=false;
    
    refresh(rMAP+rSCRMAP);
	ComboPosition last_pos = {-1, -1};
	
    Map.StartListCommand();
	bool pressframe = true;
	while(gui_mouse_b())
    {
        int32_t x=gui_mouse_x();
        int32_t y=gui_mouse_y();
        double startx=mapscreen_x+(showedges?(16*mapscreen_single_scale):0);
        double starty=mapscreen_y+(showedges?(16*mapscreen_single_scale):0);
        int32_t startxint=mapscreen_x+(showedges?int32_t(16*mapscreen_single_scale):0);
        int32_t startyint=mapscreen_y+(showedges?int32_t(16*mapscreen_single_scale):0);
		int num_combos_width = 16 * Map.getViewSize();
		int num_combos_height = 11 * Map.getViewSize();

        if(isinRect(x,y,startxint,startyint,int32_t(startx+(256*mapscreen_screenunit_scale)-1),int32_t(starty+(176*mapscreen_screenunit_scale)-1)))
        {
            int32_t cxstart=(x-startx)/(16*mapscreen_single_scale);
            int32_t cystart=(y-starty)/(16*mapscreen_single_scale);
			ComboPosition combo_start = {cxstart, cystart};
			if (pressframe)
			{
				last_pos = combo_start;
			}
			else if (combo_start == last_pos)
			{
				custom_vsync();
				refresh(rALL);
				continue;
			}
			else if(draw_mode == dm_auto)
			{
				// TODO: support when zoomed out.
				if (combo_autos[combo_auto_pos].getType() == AUTOCOMBO_FENCE || combo_autos[combo_auto_pos].getType() == AUTOCOMBO_Z4)
				{
					// Don't allow moving the brush at anything but cardinal directions while in these modes
					bool did_diag = std::abs(combo_start.x - last_pos.x) == 1 && std::abs(combo_start.y - last_pos.y) == 1;

					if (did_diag)
					{
						int32_t oldx = last_pos.x;
						int32_t oldy = last_pos.y;
						int32_t cx = (oldx * 16 * mapscreen_single_scale) + 8;
						int32_t cy = (oldy * 16 * mapscreen_single_scale) + 8;
						int32_t nx = x - startxint;
						int32_t ny = y - startyint;
						if (std::abs(nx - cx) < std::abs(ny - cy))
						{
							oldy = vbound(oldy + ((ny - cy) < 0 ? -1 : 1), 0, 11);
						}
						else
						{
							oldx = vbound(oldx + ((nx - cx) < 0 ? -1 : 1), 0, 15);
						}
						combo_start = {oldx, oldy};
					}
				}
			}
			last_pos = combo_start;
            
            switch(draw_mode)
            {
				case dm_normal:
				{
					int32_t cc=Combo;
					
					for(int32_t cy=0; cy+cystart<num_combos_height&&cy<BrushHeight; cy++)
					{
						for(int32_t cx=0; cx+cxstart<num_combos_width&&cx<BrushWidth; cx++)
						{
							auto pos = combo_start + ComboPosition{cx, cy};
							cc=Combo + cx + cy*4;
							Map.DoSetComboCommand(pos, justcset ? -1 : cc, CSet);
						}
					}
					
					update_combobrush();
				}
				break;
				case dm_cpool:
				{
					int32_t cid = Combo;
                    int8_t cs = CSet;
					pool.pick(cid,cs);
					Map.DoSetComboCommand(combo_start, justcset ? -1 : cid, cs);
					
					update_combobrush();
				}
				break;
				
				case dm_alias:
				{
					combo_alias *combo = &combo_aliases[combo_apos];
					if(!combo->layermask)
					{
						int32_t ox=0, oy=0;
						
						switch(alias_origin)
						{
						case 0:
							ox=0;
							oy=0;
							break;
							
						case 1:
							ox=(combo->width);
							oy=0;
							break;
							
						case 2:
							ox=0;
							oy=(combo->height);
							break;
							
						case 3:
							ox=(combo->width);
							oy=(combo->height);
							break;
						}
						
						for(int32_t cy=0; cy-oy+cystart<num_combos_height&&cy<=combo->height; cy++)
						{
							for(int32_t cx=0; cx-ox+cxstart<num_combos_width&&cx<=combo->width; cx++)
							{
								if((cx+cxstart-ox>=0)&&(cy+cystart-oy>=0))
								{
									int32_t p=(cy*(combo->width+1))+cx;
									
									if(combo->combos[p])
									{
										auto pos = combo_start + ComboPosition{cx - ox, cy - oy};
										Map.DoSetComboCommand(pos, combo->combos[p], wrap(combo->csets[p]+alias_cset_mod, 0, 13));
									}
								}
							}
						}
					}
					else
					{
						int32_t laypos = 0;
						int32_t ox=0, oy=0;
						
						switch(alias_origin)
						{
						case 0:
							ox=0;
							oy=0;
							break;
							
						case 1:
							ox=(combo->width);
							oy=0;
							break;
							
						case 2:
							ox=0;
							oy=(combo->height);
							break;
							
						case 3:
							ox=(combo->width);
							oy=(combo->height);
							break;
						}
						
						for(int32_t cz=0; cz<7; cz++)
						{
							if (cz > 0 && !(combo->layermask & (1<<(cz-1))))
								continue;

							if (cz > 0)
								laypos++;

							for(int32_t cy=0; cy-oy+cystart<num_combos_height&&cy<=combo->height; cy++)
							{
								for(int32_t cx=0; cx-ox+cxstart<num_combos_width&&cx<=combo->width; cx++)
								{
									if((cx+cxstart-ox>=0)&&(cy+cystart-oy>=0))
									{
										int32_t p=((cy*(combo->width+1))+cx)+((combo->width+1)*(combo->height+1)*laypos);
										
										if (combo->combos[p])
										{
											auto pos = combo_start + ComboPosition{cx - ox, cy - oy};
											if(cz > 0 && Map.Scr(pos)->layermap[cz - 1] == 0)
												continue;
											int prev = CurrentLayer;
											CurrentLayer = cz;
											Map.DoSetComboCommand(pos, combo->combos[p], wrap(combo->csets[p]+alias_cset_mod, 0, 13));
											CurrentLayer = prev;
										}
									}
								}
							}
						}
					}
					
					break;
				}
            
				case dm_auto:
				{
					draw_autocombo(combo_start, gui_mouse_b() & 2, pressframe);

					combobrushoverride = get_autocombo_floating_cid(combo_start, true);
					update_combobrush();
				}
			}
		}
		pressframe = false;

		custom_vsync();
		refresh(rALL);
    }

    Map.FinishListCommand();
	if(AutoBrushRevert)
	{
		AutoBrushRevert = false;
		BrushWidth = 1;
		BrushHeight = 1;
	}
}

static void replace(ComboPosition start)
{
	int32_t cid = Combo;
	int8_t cs = CSet;
	combo_pool const& pool = combo_pools[combo_pool_pos];
	if(draw_mode == dm_cpool && !pool.valid())
		return;

	int c = start.truncate();
	mapscr* scr = Map.Scr(start, CurrentLayer);
	if (!scr) return;

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();
	int targetcombo = scr->data[c];
	int targetcset  = scr->cset[c];

	saved = false;
	Map.StartListCommand();
	if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
	{
		for (int x = 0; x < num_combos_width; x++)
		{
			for (int y = 0; y < num_combos_height; y++)
			{
				ComboPosition pos = {x, y};
				int c = pos.truncate();
				mapscr* scr = Map.Scr(pos, CurrentLayer);
				if (!scr)
					continue;

				if ((scr->cset[c]) == targetcset)
				{
					if(draw_mode == dm_cpool)
						pool.pick(cid,cs);
					Map.DoSetComboCommand(pos, -1, cs);
				}
			}
		}
	}
	else
	{
		for (int x = 0; x < num_combos_width; x++)
		{
			for (int y = 0; y < num_combos_height; y++)
			{
				ComboPosition pos = {x, y};
				int c = pos.truncate();
				mapscr* scr = Map.Scr(pos, CurrentLayer);
				if (!scr)
					continue;

				if(((scr->data[c])==targetcombo) &&
					((scr->cset[c])==targetcset))
				{
					if(draw_mode == dm_cpool)
						pool.pick(cid,cs);
					Map.DoSetComboCommand(pos, cid, cs);
				}
			}
		}
	}
	Map.FinishListCommand();

	refresh(rMAP);
}

static void draw_block(ComboPosition start, int32_t w, int32_t h)
{
	int32_t cid = Combo;
    int8_t cs = CSet;
	if(draw_mode == dm_cpool)
	{
		combo_pool const& pool = combo_pools[combo_pool_pos];
		if(!pool.pick(cid,cs)) return;
	}

	mapscr* scr = Map.Scr(start, CurrentLayer);
	if (!scr) return;

    saved = false;
    Map.StartListCommand();
    for (int32_t y=0; y < h && y < 11*Map.getViewSize(); y++)
        for (int32_t x=0; x < w && x < 16*Map.getViewSize(); x++)
        {
            Map.DoSetComboCommand(start + ComboPosition{x, y}, cid+(y*4)+x, cs);
        }
    
    Map.FinishListCommand();
    refresh(rMAP+rSCRMAP);
}

static std::vector<ComboPosition> flood_filler(ComboPosition start_pos, bool allow_diagonal, std::function<bool(ComboPosition)> check)
{
	std::vector<ComboPosition> seen, queue;

	queue.push_back(start_pos);
	while (!queue.empty())
	{
		ComboPosition pos = queue.back();
		queue.pop_back();
		seen.push_back(pos);

		ComboPosition pos2;
		#define FLOOD_FILLER_CHECK(dx, dy)\
			pos2 = pos + ComboPosition{dx, dy};\
			if (std::find(seen.begin(), seen.end(), pos2) == seen.end() && check(pos2))\
				queue.push_back(pos2);

		FLOOD_FILLER_CHECK(0, 1);
		FLOOD_FILLER_CHECK(0, -1);
		FLOOD_FILLER_CHECK(1, 0);
		FLOOD_FILLER_CHECK(-1, 0);

		if (allow_diagonal)
		{
			FLOOD_FILLER_CHECK(1, 1);
			FLOOD_FILLER_CHECK(1, -1);
			FLOOD_FILLER_CHECK(-1, 1);
			FLOOD_FILLER_CHECK(-1, -1);
		}

		#undef FLOOD_FILLER_CHECK
	}

	return seen;
}

static void fill(int32_t targetcombo, int32_t targetcset, ComboPosition start_pos, bool allow_diagonal, bool only_cset)
{
	bool rclick = gui_mouse_b() & 2;
	bool ignored_combo = false;

	mapscr* scr = Map.ScrMakeValid(start_pos, CurrentLayer);
	if (!scr)
		return;

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();

	auto combo_positions = flood_filler(start_pos, allow_diagonal, [&](ComboPosition pos){
		if (pos.x < 0 || pos.y < 0 || pos.x >= num_combos_width || pos.y >= num_combos_height)
			return false;

		mapscr* scr = Map.Scr(pos, CurrentLayer);
		if (!scr || !scr->is_valid())
			return false;

		int cid = scr->data[pos.truncate()];
		int cset = scr->cset[pos.truncate()];

		if (draw_mode == dm_auto)
		{
			combo_auto const& cauto = combo_autos[combo_auto_pos];
			
			ignored_combo = cauto.isIgnoredCombo(cid);
			if (rclick)
			{
				if (cauto.containsCombo(targetcombo))
				{
					if (!cauto.containsCombo(cid))
						return false;
					if (cauto.getType() == AUTOCOMBO_REPLACE && ignored_combo)
						return false;
				}
				else
					return false;
			}
			else
			{
				if (cid != targetcombo && !ignored_combo)
					return false;
				if (cauto.getType() == AUTOCOMBO_REPLACE && !ignored_combo)
					return false;
			}

			if (cset != targetcset && !ignored_combo)
				return false;
		}
		else
		{
			if(!only_cset)
			{
				if (cid != targetcombo)
					return false;
			}

			if (cset != targetcset)
				return false;
		}

		return true;
	});

	for (auto& pos : combo_positions)
	{
		int32_t cid = Combo;
		int8_t cs = CSet;

		if (draw_mode == dm_cpool)
		{
			combo_pool const& pool = combo_pools[combo_pool_pos];
			if (!pool.pick(cid, cs)) continue;
		}
		else if (draw_mode == dm_auto)
		{
			combo_auto const& cauto = combo_autos[combo_auto_pos];
			if (!cauto.valid())
				continue;
			if (!rclick && (cauto.containsCombo(targetcombo) && !ignored_combo))
				continue;
			if (rclick && cauto.getEraseCombo() == targetcombo)
				continue;
		}

		if (draw_mode == dm_auto)
			draw_autocombo(pos, rclick);
		else
			Map.DoSetComboCommand(pos, only_cset ? -1 : cid, cs);
	}
}

static void fill_flag(int32_t targetflag, ComboPosition start_pos, bool allow_diagonal)
{
	mapscr* scr = Map.ScrMakeValid(start_pos, CurrentLayer);
	if (!scr)
		return;

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();

	auto combo_positions = flood_filler(start_pos, allow_diagonal, [&](ComboPosition pos){
		if (pos.x < 0 || pos.y < 0 || pos.x >= num_combos_width || pos.y >= num_combos_height)
			return false;

		mapscr* scr = Map.Scr(pos, CurrentLayer);
		if (!scr || !scr->is_valid())
			return false;

		if (scr->sflag[pos.truncate()] != targetflag)
			return false;

		return true;
	});

	for (auto& pos : combo_positions)
    	Map.DoSetFlagCommand(pos, Flag);
}

static void fill2(int32_t targetcombo, int32_t targetcset, ComboPosition pos, int32_t dir, int32_t diagonal, bool only_cset)
{
	mapscr* scr = Map.Scr(pos, CurrentLayer);
	if (!scr || !scr->is_valid())
		return;

	int cid = scr->data[pos.truncate()];
	int cset = scr->cset[pos.truncate()];

    if (!only_cset)
    {
        if (cid == targetcombo)
            return;
    }
    
    if (cset == targetcset)
        return;

	cid = Combo;
    int8_t cs = CSet;
	if(draw_mode == dm_cpool)
	{
		combo_pool const& pool = combo_pools[combo_pool_pos];
		if(!pool.pick(cid,cs)) return;
	}
	
    Map.DoSetComboCommand(pos, only_cset ? -1 : cid, cs);

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();

	if (pos.y > 0 && dir != down)
        fill2(targetcombo, targetcset, pos + ComboPosition{0, -1}, up, diagonal, only_cset);
    if (pos.y < num_combos_height-1 && dir != up)
        fill2(targetcombo, targetcset, pos + ComboPosition{0, 1}, down, diagonal, only_cset);
    if (pos.x > 0 && dir != right)
        fill2(targetcombo, targetcset, pos + ComboPosition{-1, 0}, left, diagonal, only_cset);
    if (pos.x < num_combos_width-1 && dir != left)
        fill2(targetcombo, targetcset, pos + ComboPosition{1, 0}, right, diagonal, only_cset);

    if (diagonal == 1)
    {
        if (pos.y > 0 && pos.x > 0 && dir != r_down)
            fill2(targetcombo, targetcset, pos + ComboPosition{-1, -1}, l_up, diagonal, only_cset);
        if (pos.y < num_combos_height-1 && pos.x < num_combos_width-1 && dir != l_up)
            fill2(targetcombo, targetcset, pos + ComboPosition{1, 1}, r_down, diagonal, only_cset);
        if (pos.x > 0 && pos.y < num_combos_height-1 && dir != r_up)
            fill2(targetcombo, targetcset, pos + ComboPosition{-1, 1}, l_down, diagonal, only_cset);
        if (pos.x < num_combos_width-1 && pos.y > 0 && dir != l_down)
            fill2(targetcombo, targetcset, pos + ComboPosition{1, -1}, r_up, diagonal, only_cset);
    }
}


enum SnapMode
{
	SNAP_NONE, SNAP_HALF, SNAP_WHOLE
};
static void snap_xy(int& x, int& y, SnapMode mode, roundType rounding, optional<int> max_x = nullopt, optional<int> max_y = nullopt)
{
	if(mode == SNAP_NONE)
	{
		if(max_x) x = vbound(x,*max_x,0);
		if(max_y) y = vbound(y,*max_y,0);
		return;
	}
	int xoff = 0, yoff = 0;
	switch(rounding)
	{
		case ROUND_TO_0:
			rounding = ROUND_DOWN;
			break;
		case ROUND_AWAY_0:
			rounding = ROUND_UP;
			break;
	}
	int r = 0;
	switch(mode)
	{
		case SNAP_HALF:
			r = 8;
			break;
		case SNAP_WHOLE:
			r = 16;
			break;
	}
	assert(r > 0);
	// r must be a power of 2, for bitwise reasons
	switch(rounding)
	{
		case ROUND_DOWN:
			break;
		case ROUND_UP:
			xoff = ((x & (r-1)) ? r : 0);
			yoff = ((y & (r-1)) ? r : 0);
			break;
		case ROUND_NEAREST:
			xoff = ((x & (r-1)) >= (r/2) ? r : 0);
			yoff = ((y & (r-1)) >= (r/2) ? r : 0);
			break;
	}
	x = (x & ~(r-1)) + xoff;
	y = (y & ~(r-1)) + yoff;
	if(max_x && x >= *max_x) x = *max_x-r;
	else if(max_x && x < 0) x = 0;
	if(max_y && y >= *max_y) y = *max_y-r;
	else if(max_y && y < 0) y = 0;
}

static void doxypos(byte &px2, byte &py2, int32_t color, SnapMode snap_mode,
	SnapMode shift_mode, bool immediately, int32_t cursoroffx,
	int32_t cursoroffy, int32_t iconw, int32_t iconh)
{
    int32_t tempcb=ComboBrush;
    ComboBrush=0;
    MouseSprite::set(ZQM_POINT_BOX);
    
	int viz_off_x = (active_visible_screen ? active_visible_screen->dx * 256 : 0);
	int viz_off_y = (active_visible_screen ? active_visible_screen->dy * 176 : 0);

    int32_t oldpx=px2, oldpy=py2;
    int32_t startxint=mapscreen_x+(showedges?int32_t(16*mapscreen_single_scale):0);
    int32_t startyint=mapscreen_y+(showedges?int32_t(16*mapscreen_single_scale):0);
    showxypos_x=px2 + viz_off_x;
    showxypos_y=py2 + viz_off_y;
    showxypos_w=iconw;
    showxypos_h=iconh;
    showxypos_color=vc(color);
    showxypos_icon=!showxypos_dummy;
    bool canedit=false;
    bool done=false;
	
	clear_tooltip();
    
    while(!done && (!(gui_mouse_b()&2) || immediately))
    {
        if(!gui_mouse_b() || immediately)
        {
            canedit=true;
        }
        
		// TODO: would be nice if these bounds were based on the individual screen.
        if(canedit && gui_mouse_b()==1 && isinRect(gui_mouse_x(),gui_mouse_y(),startxint,startyint,(startxint+(256*mapscreen_screenunit_scale)-1),(startyint+(176*mapscreen_screenunit_scale)-1)))
        {
            set_mouse_range(startxint,startyint,int32_t(startxint+(256*mapscreen_screenunit_scale)-1),int32_t(startyint+(176*mapscreen_screenunit_scale)-1));
            
			double offx = 0, offy = 0;
			roundType rounding = ROUND_DOWN;
			if(DragCenterOfSquares)
			{
				offx -= iconw*mapscreen_single_scale/2;
				offy -= iconh*mapscreen_single_scale/2;
				rounding = ROUND_NEAREST;
			}
			int32_t x, y;
            do
            {
                x=int32_t((gui_mouse_x()-startxint+offx)/mapscreen_single_scale)-cursoroffx;
                y=int32_t((gui_mouse_y()-startyint+offy)/mapscreen_single_scale)-cursoroffy;
                showxypos_cursor_icon=true;
				showxypos_cursor_color = showxypos_color;
				auto _mode = (key[KEY_LSHIFT] || key[KEY_RSHIFT]) ? shift_mode : snap_mode;
                showxypos_cursor_x = x-viz_off_x;
                showxypos_cursor_y = y-viz_off_y;
				snap_xy(showxypos_cursor_x, showxypos_cursor_y, _mode, rounding, 256, 176);
				showxypos_cursor_x += viz_off_x;
				showxypos_cursor_y += viz_off_y;
                custom_vsync();
                refresh(rALL | rNOCURSOR);
                int32_t xpos[2], ypos[2];
				int32_t x1,y1,x2,y2;
                
				char b1[200] = {0};
				char b2[200] = {0};
				if(showxypos_dummy)
					strcpy(b1, "DUMMY MEASURING");
				else sprintf(b1, "%d %d",oldpx,oldpy);
				sprintf(b2, "%d %d (%d %d)",x-viz_off_x,y-viz_off_y,showxypos_cursor_x-viz_off_x,showxypos_cursor_y-viz_off_y);
				
				int len[2] = {text_length(font,b1),text_length(font,b2)};
				
                if(is_compact)
				{
					xpos[0] = 4;
					ypos[0] = layer_panel.y - 21;
					xpos[1] = xpos[0];
					ypos[1] = ypos[0]+10;
				}
				else
                {
                    xpos[0] = 450;
                    ypos[0] = 405;
					xpos[1] = xpos[0];
					ypos[1] = ypos[0]+10;
                }
				
				x1 = xpos[0];
				y1 = ypos[0];
				x2 = xpos[0];
				y2 = ypos[0];
				for(auto q = 0; q < 2; ++q)
				{
					if(xpos[q] < x1)
						x1 = xpos[q];
					if(ypos[q] < y1)
						y1 = ypos[q];
					if(ypos[q] > y2)
						y2 = ypos[q];
					if(xpos[q] + len[q] > x2)
						x2 = xpos[q] + len[q];
				}
				x1 -= 4;
				y1 -= 2;
				y2 += text_height(font)+2;
				
				auto minx = zc_min(xpos[0],xpos[1]);
				auto miny = zc_min(ypos[0],ypos[1]);
				rectfill(screen,x1,y1,x2,y2,vc(0));
                textprintf_ex(screen,font,xpos[0],ypos[0],vc(15),vc(0),"%s",b1);
                textprintf_ex(screen,font,xpos[1],ypos[1],vc(15),vc(0),"%s",b2);
				update_hw_screen();
            }
            while(gui_mouse_b()==1);
			
            if(gui_mouse_b()==0)
            {
				auto _mode = (key[KEY_LSHIFT] || key[KEY_RSHIFT]) ? shift_mode : snap_mode;
				int x2 = vbound(x-viz_off_x,0,255);
				int y2 = vbound(y-viz_off_y,0,175);
				snap_xy(x2, y2, _mode, rounding, 256, 176);
                px2=byte(x2);
                py2=byte(y2);
            }
            
            set_mouse_range(0,0,zq_screen_w-1,zq_screen_h-1);
            done=true;
        }
        
        if(keypressed())
        {
            switch(readkey()>>8)
            {
            case KEY_ESC:
            case KEY_ENTER:
                goto finished;
            }
        }
        
        custom_vsync();
        refresh(rALL | rNOCURSOR);
    }
    
finished:
    MouseSprite::set(ZQM_NORMAL);
    refresh(rMAP+rMENU);
    
    while(gui_mouse_b())
    {
        /* do nothing */
        rest(1);
    }
    
    showxypos_x=-1000;
    showxypos_y=-1000;
    showxypos_color=-1000;
    showxypos_ffc=-1000;
    showxypos_icon=false;
    showxypos_cursor_x=-1000;
    showxypos_cursor_y=-1000;
    showxypos_cursor_icon=false;
	showxypos_cursor_color=-1000;
	showxypos_dummy=false;
    
    if(px2!=oldpx||py2!=oldpy)
    {
        saved=false;
    }
    
    ComboBrush=tempcb;
}
static void doxypos(byte &px2,byte &py2,int32_t color,SnapMode snap_mode, optional<SnapMode> shift_mode = nullopt)
{
    doxypos(px2,py2,color,snap_mode,shift_mode ? *shift_mode : snap_mode,false,0,0,16,16);
}

bool placing_flags = false;
void doflags()
{
	placing_flags = true;
	int of=Flags;
	Flags=cFLAGS;
	refresh(rMAP | rNOCURSOR);
	
	bool canedit=false;
	bool didShift = false;
	int tFlag = Flag;
	while(!(gui_mouse_b()&2) && !handle_close_btn_quit())
	{
		int x=gui_mouse_x();
		int y=gui_mouse_y();
		double startx=mapscreen_x+(showedges?(16*mapscreen_single_scale):0);
		double starty=mapscreen_y+(showedges?(16*mapscreen_single_scale):0);
		int startxint=mapscreen_x+(showedges?int(16*mapscreen_single_scale):0);
		int startyint=mapscreen_y+(showedges?int(16*mapscreen_single_scale):0);
		int cx=(x-startxint)/int(16*mapscreen_single_scale);
		int cy=(y-startyint)/int(16*mapscreen_single_scale);
		ComboPosition combo_pos = {cx, cy};
		int c = combo_pos.truncate();

		if(!gui_mouse_b())
			canedit=true;
        bool shift = key[KEY_LSHIFT] || key[KEY_RSHIFT];

		if(canedit && gui_mouse_b()==1 && isinRect(x,y,startxint,startyint,int(startx+(256*mapscreen_screenunit_scale)-1),int(starty+(176*mapscreen_screenunit_scale)-1)))
		{
			mapscr* cur_scr = Map.Scr(combo_pos, CurrentLayer);
			if (!cur_scr) continue;

			Map.setCurrScr(Map.getScreenForPosition(combo_pos));

			if(key[KEY_ALT]||key[KEY_ALTGR])
				Flag = cur_scr->sflag[c];
			else
			{
				saved=false;
				int tflag = Flag;
				if(shift)
					Flag = mfNONE;
				if(CurrentLayer!=0)
				{
					// Notify if they are using a flag that doesn't work on this layer.
					if(!skipLayerWarning && ((Flag >= mfTRAP_H && Flag < mfPUSHD) || (Flag == mfFAIRY) || (Flag == mfMAGICFAIRY)
							|| (Flag == mfALLFAIRY) || (Flag == mfRAFT) || (Flag == mfRAFT_BRANCH)
							|| (Flag == mfDIVE_ITEM) || (Flag == mfARMOS_SECRET) || (Flag == mfNOENEMY)
							|| (Flag == mfZELDA)))
					{
						InfoDialog("Notice","You are currently working on layer "
							+to_string(CurrentLayer)
							+". This combo flag does not function on layers above '0'.").show();
					}
					if(!skipLayerWarning && CurrentLayer > 2 &&
						((Flag == mfBLOCKHOLE) || (Flag >= mfPUSHD && Flag < mfNOBLOCKS)
						|| (Flag == mfPUSHUD) || (Flag == mfPUSH4)))
					{
						InfoDialog("Notice","You are currently working on layer "
							+to_string(CurrentLayer)
							+". This combo flag does not function on layers above '2'.").show();
					}
				}
				if(CHECK_CTRL_CMD)
				{
					switch(fill_type)
					{
						case 0:
							flood_flag();
							break;
							
						case 1:
						case 3:
							fill_4_flag();
							break;
							
						case 2:
						case 4:
							fill_8_flag();
							break;
					}
				}
				else
				{
					Map.DoSetFlagCommand(combo_pos, Flag);
				}
				Flag = tflag;
			}
		}
		
		if(mouse_z)
		{
			for(int i=0; i<abs(mouse_z); ++i)
			{
				if(mouse_z>0)
					onIncreaseFlag();
				else
					onDecreaseFlag();
			}
			
			position_mouse_z(0);
		}
		
		if(keypressed())
		{
			int k = readkey();
			switch(k>>8)
			{
				case KEY_ESC:
				case KEY_ENTER:
					goto finished;
			}
			object_message(dialogs+1, MSG_XCHAR, k);
			Flags=cFLAGS;
		}
		
		MouseSprite::set(ZQM_FLAG_0+(shift?0:Flag%16));
		
		refresh(rALL | rCLEAR | rNOCURSOR);
		custom_vsync();
	}
	
finished:
	Flags=of;
	placing_flags = false;
	MouseSprite::set(ZQM_NORMAL);
	refresh(rMAP+rMENU);
	
	while(gui_mouse_b())
	{
		/* do nothing */
        rest(1);
	}
}

// Drag FFCs around
static void moveffc(int i, int cx, int cy)
{
	mapscr* scr = active_visible_screen->scr;
	int screen = active_visible_screen->screen;

    int32_t ffx = vbound(scr->ffcs[i].x.getFloor(),0,240);
    int32_t ffy = vbound(scr->ffcs[i].y.getFloor(),0,160);
	int32_t offx = ffx, offy = ffy;
    showxypos_ffc = i;
    doxypos((byte&)ffx,(byte&)ffy,15,SNAP_HALF,SNAP_NONE,true,0,0,(scr->ffTileWidth(i)*16),(scr->ffTileHeight(i)*16));
    if(ffx > 240) ffx = 240;
    if(ffy > 160) ffy = 160;
    if((ffx != offx) || (ffy != offy))
    {
        auto set_ffc_data = set_ffc_command::create_data(scr->ffcs[i]);
        set_ffc_data.x = ffx;
        set_ffc_data.y = ffy;
        Map.DoSetFFCCommand(Map.getCurrMap(), screen, i, set_ffc_data);
        saved = false;
    }
}

void set_brush_width(int32_t width);
void set_brush_height(int32_t height);

int32_t set_brush_width_1()
{
    set_brush_width(1);
    return D_O_K;
}
int32_t set_brush_width_2()
{
    set_brush_width(2);
    return D_O_K;
}
int32_t set_brush_width_3()
{
    set_brush_width(3);
    return D_O_K;
}
int32_t set_brush_width_4()
{
    set_brush_width(4);
    return D_O_K;
}
int32_t set_brush_width_5()
{
    set_brush_width(5);
    return D_O_K;
}
int32_t set_brush_width_6()
{
    set_brush_width(6);
    return D_O_K;
}
int32_t set_brush_width_7()
{
    set_brush_width(7);
    return D_O_K;
}
int32_t set_brush_width_8()
{
    set_brush_width(8);
    return D_O_K;
}
int32_t set_brush_width_9()
{
    set_brush_width(9);
    return D_O_K;
}
int32_t set_brush_width_10()
{
    set_brush_width(10);
    return D_O_K;
}
int32_t set_brush_width_11()
{
    set_brush_width(11);
    return D_O_K;
}
int32_t set_brush_width_12()
{
    set_brush_width(12);
    return D_O_K;
}
int32_t set_brush_width_13()
{
    set_brush_width(13);
    return D_O_K;
}
int32_t set_brush_width_14()
{
    set_brush_width(14);
    return D_O_K;
}
int32_t set_brush_width_15()
{
    set_brush_width(15);
    return D_O_K;
}
int32_t set_brush_width_16()
{
    set_brush_width(16);
    return D_O_K;
}

int32_t set_brush_height_1()
{
    set_brush_height(1);
    return D_O_K;
}
int32_t set_brush_height_2()
{
    set_brush_height(2);
    return D_O_K;
}
int32_t set_brush_height_3()
{
    set_brush_height(3);
    return D_O_K;
}
int32_t set_brush_height_4()
{
    set_brush_height(4);
    return D_O_K;
}
int32_t set_brush_height_5()
{
    set_brush_height(5);
    return D_O_K;
}
int32_t set_brush_height_6()
{
    set_brush_height(6);
    return D_O_K;
}
int32_t set_brush_height_7()
{
    set_brush_height(7);
    return D_O_K;
}
int32_t set_brush_height_8()
{
    set_brush_height(8);
    return D_O_K;
}
int32_t set_brush_height_9()
{
    set_brush_height(9);
    return D_O_K;
}
int32_t set_brush_height_10()
{
    set_brush_height(10);
    return D_O_K;
}
int32_t set_brush_height_11()
{
    set_brush_height(11);
    return D_O_K;
}

static NewMenu brush_width_menu
{
	{ "1", set_brush_width_1 },
	{ "2", set_brush_width_2 },
	{ "3", set_brush_width_3 },
	{ "4", set_brush_width_4 },
	{ "5", set_brush_width_5 },
	{ "6", set_brush_width_6 },
	{ "7", set_brush_width_7 },
	{ "8", set_brush_width_8 },
	{ "9", set_brush_width_9 },
	{ "10", set_brush_width_10 },
	{ "11", set_brush_width_11 },
	{ "12", set_brush_width_12 },
	{ "13", set_brush_width_13 },
	{ "14", set_brush_width_14 },
	{ "15", set_brush_width_15 },
	{ "16", set_brush_width_16 },
};
static NewMenu brush_height_menu
{
	{ "1", set_brush_height_1 },
	{ "2", set_brush_height_2 },
	{ "3", set_brush_height_3 },
	{ "4", set_brush_height_4 },
	{ "5", set_brush_height_5 },
	{ "6", set_brush_height_6 },
	{ "7", set_brush_height_7 },
	{ "8", set_brush_height_8 },
	{ "9", set_brush_height_9 },
	{ "10", set_brush_height_10 },
	{ "11", set_brush_height_11 },
};

int toggle_autobrush();
int toggle_combobrush();
int toggle_floatbrush();
enum
{
	MENUID_BRUSH_AUTOBRUSH,
	MENUID_BRUSH_WIDTH,
	MENUID_BRUSH_HEIGHT,
	MENUID_BRUSH_COMBOBRUSH,
	MENUID_BRUSH_FLOATBRUSH,
};
static NewMenu brush_menu
{
	{ "AutoBrush", toggle_autobrush, MENUID_BRUSH_AUTOBRUSH },
	{ "Brush Width ", &brush_width_menu, MENUID_BRUSH_WIDTH },
	{ "Brush Height ", &brush_height_menu, MENUID_BRUSH_HEIGHT },
	{ "ComboBrush", toggle_combobrush, MENUID_BRUSH_COMBOBRUSH },
	{ "FloatBrush", toggle_floatbrush, MENUID_BRUSH_FLOATBRUSH },
};
int toggle_autobrush()
{
	AutoBrush = AutoBrush ? 0 : 1;
	BrushWidth = BrushHeight = 1;
	brush_menu.select_uid(MENUID_BRUSH_AUTOBRUSH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_WIDTH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_HEIGHT, AutoBrush);
	zc_set_config("zquest","autobrush",AutoBrush);
	return D_O_K;
}
int toggle_combobrush()
{
	ComboBrush = ComboBrush ? 0 : 1;
	brush_menu.select_uid(MENUID_BRUSH_COMBOBRUSH, ComboBrush);
	zc_set_config("zquest","combo_brush",ComboBrush);
	return D_O_K;
}
int toggle_floatbrush()
{
	FloatBrush = FloatBrush ? 0 : 1;
	brush_menu.select_uid(MENUID_BRUSH_FLOATBRUSH, FloatBrush);
	zc_set_config("zquest","float_brush",FloatBrush);
	return D_O_K;
}

int32_t set_flood();
int32_t set_fill_4();
int32_t set_fill_8();
int32_t set_fill2_4();
int32_t set_fill2_8();

// Sets every combo.
void flood()
{
	ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr || !scr->is_valid())
		return;

	saved=false;

	bool include_combos = !(key[KEY_LSHIFT]||key[KEY_RSHIFT]);

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();

	Map.StartListCommand();
	for (int x = 0; x < num_combos_width; x++)
	{
		for (int y = 0; y < num_combos_height; y++)
		{
			ComboPosition pos = {x, y};
			mapscr* scr = Map.Scr(pos, CurrentLayer);
			if (!scr || !scr->is_valid())
				continue;

			if (draw_mode == dm_auto)
				draw_autocombo(pos, gui_mouse_b() & 2);
			else
				Map.DoSetComboCommand(pos, include_combos ? Combo : -1, CSet);
		}
	}
	Map.FinishListCommand();

	refresh(rMAP+rSCRMAP);
}
void flood_flag()
{
	ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr || !scr->is_valid())
		return;

	saved=false;

	int num_combos_width = 16 * Map.getViewSize();
	int num_combos_height = 11 * Map.getViewSize();

	Map.StartListCommand();
	for (int x = 0; x < num_combos_width; x++)
	{
		for (int y = 0; y < num_combos_height; y++)
		{
			ComboPosition pos = {x, y};
			mapscr* scr = Map.Scr(pos, CurrentLayer);
			if (!scr || !scr->is_valid())
				continue;

			Map.DoSetFlagCommand(pos, Flag);
		}
	}
	Map.FinishListCommand();

	refresh(rMAP+rSCRMAP);
}

void fill_4()
{
	ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;

	int c = pos.truncate();
	if (draw_mode == dm_cpool || draw_mode == dm_auto
		|| (scr->cset[c]!=CSet || (scr->data[c]!=Combo && !(key[KEY_LSHIFT]||key[KEY_RSHIFT]))))
	{
		saved=false;

		Map.StartListCommand();
		if (draw_mode == dm_auto && (combo_autos[combo_auto_pos].getType() == AUTOCOMBO_FENCE ||
			combo_autos[combo_auto_pos].getType() == AUTOCOMBO_Z4))
		{
			draw_autocombo_command(pos);
		}
		else
		{
			bool allow_diagonal = false;
			fill(scr->data[c], scr->cset[c], pos, allow_diagonal, (key[KEY_LSHIFT] || key[KEY_RSHIFT]));
		}
		Map.FinishListCommand();
		refresh(rMAP+rSCRMAP);
	}
}
void fill_4_flag()
{
	ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;

	int flag = scr->sflag[pos.truncate()];
    if (flag != Flag)
    {
        saved=false;

        Map.StartListCommand();
		bool allow_diagonal = false;
		fill_flag(flag, pos, allow_diagonal);
        Map.FinishListCommand();
        refresh(rMAP+rSCRMAP);
    }
}
void fill_8()
{
    ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;

	int c = pos.truncate();
    if (draw_mode == dm_cpool || draw_mode == dm_auto
		|| (scr->cset[c] != CSet ||
            (scr->data[c] != Combo &&
             !(key[KEY_LSHIFT]||key[KEY_RSHIFT]))))
    {
        saved=false;

        Map.StartListCommand();
		bool allow_diagonal = true;
        fill(scr->data[c], scr->cset[c], pos, allow_diagonal, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
        Map.FinishListCommand();

        refresh(rMAP+rSCRMAP);
    }
}
void fill_8_flag()
{
    ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;
    
	int flag = scr->sflag[pos.truncate()];
    if (flag != Flag)
    {
        saved=false;

        Map.StartListCommand();
		bool allow_diagonal = true;
        fill_flag(flag, pos, allow_diagonal);
        Map.FinishListCommand();
        refresh(rMAP+rSCRMAP);
    }
}

void fill2_4()
{
    ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;
    
    saved=false;

    Map.StartListCommand();
    fill2(Combo, CSet, pos, 255, 0, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
    Map.FinishListCommand();
    refresh(rMAP+rSCRMAP);
}

void fill2_8()
{
    ComboPosition pos = get_mapscreen_mouse_combo_pos();
	mapscr* scr = Map.ScrMakeValid(pos, CurrentLayer);
	if (!scr)
		return;
    
    saved=false;

    Map.StartListCommand();
    fill2(Combo, CSet, pos, 255, 1, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
    Map.FinishListCommand();

    refresh(rMAP+rSCRMAP);
}

static NewMenu fill_menu
{
	{ "Flood", set_flood, 0 },
	{ "Fill (4-way)", set_fill_4, 1 },
	{ "Fill (8-way)", set_fill_8, 2 },
	{ "Fill2 (4-way)", set_fill2_4, 3 },
	{ "Fill2 (8-way)", set_fill2_8, 4 },
};
void set_filltype(int ty)
{
	fill_type = ty;
	fill_menu.select_only_uid(ty);
}

int32_t set_flood()
{
	set_filltype(0);
    return D_O_K;
}

int32_t set_fill_4()
{
    set_filltype(1);
    return D_O_K;
}

int32_t set_fill_8()
{
    set_filltype(2);
    return D_O_K;
}

int32_t set_fill2_4()
{
    set_filltype(3);
    return D_O_K;
}

int32_t set_fill2_8()
{
    set_filltype(4);
    return D_O_K;
}

int32_t draw_block_1_2()
{
    draw_block(mouse_combo_pos,1,2);
    return D_O_K;
}

int32_t draw_block_2_1()
{
    draw_block(mouse_combo_pos,2,1);
    return D_O_K;
}

int32_t draw_block_2_2()
{
    draw_block(mouse_combo_pos,2,2);
    return D_O_K;
}

int32_t draw_block_2_3()
{
    draw_block(mouse_combo_pos,2,3);
    return D_O_K;
}

int32_t draw_block_3_2()
{
    draw_block(mouse_combo_pos,3,2);
    return D_O_K;
}

int32_t draw_block_3_3()
{
    draw_block(mouse_combo_pos,3,3);
    return D_O_K;
}

int32_t draw_block_4_2()
{
    draw_block(mouse_combo_pos,4,2);
    return D_O_K;
}

int32_t draw_block_4_4()
{
    draw_block(mouse_combo_pos,4,4);
    return D_O_K;
}

static NewMenu draw_block_menu
{
	{ "1x2", draw_block_1_2 },
	{ "2x1", draw_block_2_1 },
	{ "2x2", draw_block_2_2 },
	{ "2x3", draw_block_2_3 },
	{ "3x2", draw_block_3_2 },
	{ "3x3", draw_block_3_3 },
	{ "4x2", draw_block_4_2 },
	{ "4x4", draw_block_4_4 },
};

static NewMenu paste_screen_menu
{
	{ "Paste", onPaste },
	{ "Paste All", onPasteAll },
	{ "Paste to All", onPasteToAll },
	{ "Paste All to All", onPasteAllToAll },
};

int32_t scrollto_cmb(int32_t cid)
{
	auto& sqr = combolist[current_combolist];
	int32_t res = vbound(cid-(sqr.w*sqr.h/2),0,MAXCOMBOS-(sqr.w*sqr.h));
	res -= res%sqr.w;
	return res;
}
int32_t scrollto_alias(int32_t alid)
{
	auto& sqr = comboaliaslist[current_comboalist];
	int32_t res = vbound(alid-(sqr.w*sqr.h/2),0,MAXCOMBOALIASES-(sqr.w*sqr.h));
	res -= res%sqr.w;
	return res;
}

int32_t scrollto_cpool(int32_t cpid)
{
	auto& sqr = comboaliaslist[current_cpoollist];
	int32_t res = vbound(cpid-(sqr.w*sqr.h/2),0,MAXCOMBOPOOLS-(sqr.w*sqr.h));
	res -= res%sqr.w;
	return res;
}

int32_t scrollto_cauto(int32_t caid)
{
	auto& sqr = comboaliaslist[current_cautolist];
	int32_t res = vbound(caid - (sqr.w * sqr.h / 2), 0, MAXCOMBOPOOLS - (sqr.w * sqr.h));
	res -= res % sqr.w;
	return res;
}

void add_favorite_combo_block(int32_t favind, int32_t cid, bool force)
{
	int32_t w = vbound(BrushWidth, 1, 4);
	int32_t h = vbound(BrushHeight, 1, 7);
	for (int32_t xi = 0; xi < w; ++xi)
	{
		for (int32_t yi = 0; yi < h; ++yi)
		{
			int32_t cx = cid % 4;
			int32_t cy = cid / 4;
			int32_t cc = (cy + yi) * 4 + cx + xi;
			int32_t fx = favind % FAVORITECOMBO_PER_ROW;
			int32_t fy = favind / FAVORITECOMBO_PER_ROW;
			int32_t fc = (fy + yi) * FAVORITECOMBO_PER_ROW + fx + xi + FAVORITECOMBO_PER_PAGE * FavoriteComboPage;

			if (cx + xi < 4 && cc < MAXCOMBOS && fx + xi < FAVORITECOMBO_PER_ROW && fy + yi < FAVORITECOMBO_PER_COLUMN)
			{
				if (favorite_combos[fc] < 0 || force)
				{
					favorite_combo_modes[fc] = dm_normal;
					favorite_combos[fc] = cc;
				}
			}
		}
	}
}

void onRCSelectCombo(int32_t c)
{
	int32_t drawmap, drawscr;

	if(CurrentLayer==0)
	{
		drawmap=Map.getCurrMap();
		drawscr=Map.getCurrScr();
	}
	else
	{
		drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
		drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
	}
	mapscr* draw_mapscr = Map.AbsoluteScr(drawmap, drawscr);
	if(!draw_mapscr) return;

	Combo=draw_mapscr->data[c];
	if(AutoBrush)
		BrushWidth = BrushHeight = 1;
}

void onRCScrollToombo(int32_t c)
{
	int32_t drawmap, drawscr;
	
	if(CurrentLayer==0)
	{
		drawmap=Map.getCurrMap();
		drawscr=Map.getCurrScr();
	}
	else
	{
		drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
		drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
	}
	mapscr* draw_mapscr = Map.AbsoluteScr(drawmap, drawscr);
	if(!draw_mapscr) return;
	
	auto& sqr = combolist[current_combolist];
	First[current_combolist]=scrollto_cmb(draw_mapscr->data[c]);
}

enum
{
	MENUID_RCSCREEN_PASTE,
	MENUID_RCSCREEN_ADVPASTE,
	MENUID_RCSCREEN_SPECPASTE,
};
static NewMenu rc_menu_screen
{
	{ "Copy Screen", onCopy },
	{ "Paste Screen", &paste_screen_menu, MENUID_RCSCREEN_PASTE },
	{ "...Advanced Paste", &paste_menu, MENUID_RCSCREEN_ADVPASTE },
	{ "...Special Paste", &paste_item_menu, MENUID_RCSCREEN_SPECPASTE },
};

void call_options_dlg();
int32_t onOptions()
{
	call_options_dlg();
	brush_menu.select_uid(MENUID_BRUSH_AUTOBRUSH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_WIDTH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_HEIGHT, AutoBrush);
	brush_menu.select_uid(MENUID_BRUSH_FLOATBRUSH, FloatBrush);
	brush_menu.select_uid(MENUID_BRUSH_COMBOBRUSH, ComboBrush);
	return D_O_K;
}

void follow_twarp(int warpindex)
{
	if(warpindex >= 4)
	{
		InfoDialog("Random Tile Warp",
			"This is a random tile warp combo, so it chooses"
			" randomly between the screen's four Tile Warps.").show();
		warpindex=zc_oldrand()&3;
	}
	
	int32_t tm = Map.getCurrMap();
	int32_t ts = Map.getCurrScr();
	int32_t wt = Map.CurrScr()->tilewarptype[warpindex];
	
	if(wt==wtCAVE || wt==wtNOWARP)
	{
		char buf[56];
		InfoDialog(warptype_string[wt],fmt::format("This screen's Tile Warp {} is set to {}, so it doesn't lead to another screen.",'A'+warpindex,warptype_string[wt]));
		return;
	}
	
	Map.dowarp(0,warpindex);
	
	if(ts!=Map.getCurrScr() || tm!=Map.getCurrMap())
	{
		FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(warpindex*2))&3;
		FlashWarpClk = 32;
	}
}
void edit_twarp(int warpindex)
{
	if(warpindex>=4)
	{
		InfoDialog("Random Tile Warp",
			"This is a random tile warp combo, so it chooses"
			" randomly between the screen's four Tile Warps.").show();
		warpindex=zc_oldrand()&3;
	}
	
	if(warpindex > -1 && warpindex < 4)
		onTileWarpIndex(warpindex);
}

int toggle_linked_scrolling()
{
	LinkedScroll = LinkedScroll ? 0 : 1;
	zc_set_config("zquest","linked_comboscroll",LinkedScroll);
	return D_O_K;
}
void on_scroll_cpane()
{
	switch (draw_mode)
	{
		case dm_alias:
			combo_alistpos[current_comboalist] = scrollto_alias(combo_apos);
			break;
		case dm_cpool:
			combo_pool_listpos[current_cpoollist] = scrollto_cpool(combo_pool_pos);
			break;
		case dm_auto:
			combo_auto_listpos[current_cautolist] = scrollto_cauto(combo_auto_pos);
			break;
		default:
			First[current_combolist] = scrollto_cmb(Combo);
			break;
	}
}
void on_edit_cpane()
{
	switch (draw_mode)
	{
		case dm_alias:
			onEditComboAlias();
			break;
		case dm_cpool:
			onEditComboPool();
			break;
		case dm_auto:
			onEditAutoCombo();
			break;
		default:
			reset_combo_animations();
			reset_combo_animations2();
			edit_combo(Combo, true, CSet);
			setup_combo_animations();
			setup_combo_animations2();
			break;
	}
}
void on_cpane_page()
{
	switch(draw_mode)
	{
		case dm_normal:
			combo_screen(Combo>>8,Combo);
			break;
		case dm_alias:
			call_alias_pages(combo_apos);
			break;
		case dm_auto:
			call_autoc_pages(combo_auto_pos);
			break;
		case dm_cpool:
			call_cpool_pages(combo_pool_pos);
			break;
	}
}
void open_cpane_tilepage()
{
	onGotoTiles(combobuf[Combo].o_tile);
}
static int _clicked_fav = 0;
void fav_rc_remove()
{
	favorite_combo_modes[_clicked_fav] = dm_normal;
	favorite_combos[_clicked_fav] = -1;
	saved = false;
}
void popup_favorites_rc(int f, int x, int y)
{
	_clicked_fav = f;
	string type;
	switch (draw_mode)
	{
		case dm_alias:
			type = "Alias";
			break;
		case dm_cpool:
			type = "Pool";
			break;
		case dm_auto:
			type = "Autocombo";
			break;
		case dm_normal:
			type = "Combo";
			break;
		default: return;
	}
	NewMenu rcmenu
	{
		{ fmt::format("Scroll to {}", type), on_scroll_cpane },
		{ fmt::format("Edit {}", type), on_edit_cpane },
		{ fmt::format("Open {} Page", type), on_cpane_page },
		{ fmt::format("Remove Fav {}", type), fav_rc_remove },
	};
	switch (draw_mode)
	{
		case dm_normal:
			rcmenu.add({
				{},
				{ "Open Tile Page", open_cpane_tilepage },
				});
			break;
	}
	rcmenu.pop(x, y);
}
void popup_cpane_rc(int x, int y)
{
	string type;
	switch (draw_mode)
	{
		case dm_alias:
			type = "Alias";
			break;
		case dm_cpool:
			type = "Pool";
			break;
		case dm_auto:
			type = "Autocombo";
			break;
		case dm_normal:
			type = "Combo";
			break;
		default: return;
	}
	NewMenu rcmenu;
	switch(draw_mode)
	{
		case dm_normal:
			rcmenu.add({
					{ fmt::format("Edit {}", type), on_edit_cpane },
					{ fmt::format("Open {} Page", type), on_cpane_page },
					{ "Open Tile Page", open_cpane_tilepage },
					{ "Combo Locations", onComboLocationReport },
					{},
					{ "Scroll to Page...", onGotoPage },
					{ "Linked Scrolling", toggle_linked_scrolling, nullopt, LinkedScroll ? MFL_SEL : 0 },
				});
			break;
		case dm_alias:
		case dm_cpool:
		case dm_auto:
			rcmenu.add({
					{ fmt::format("Edit {}", type), on_edit_cpane },
					{ fmt::format("Open {} Page", type), on_cpane_page },
					{},
					{ "Scroll to Page...", onGotoPage },
					{ "Linked Scrolling", toggle_linked_scrolling, nullopt, LinkedScroll ? MFL_SEL : 0 },
				});
			break;
	}
	rcmenu.pop(x, y);
}

void set_brush_width(int32_t width)
{
    BrushWidth = width;
    for(int q = 0; q < brush_width_menu.size(); ++q)
		brush_width_menu.at(q)->select(q==BrushWidth-1);
    refresh(rALL);
}

void set_brush_height(int32_t height)
{
    BrushHeight = height;
    for(int q = 0; q < brush_height_menu.size(); ++q)
		brush_height_menu.at(q)->select(q==BrushHeight-1);
    refresh(rALL);
}

void restore_mouse()
{
    ComboBrushPause=1;
    MouseSprite::set(ZQM_NORMAL);
}

static int32_t comboa_cnt=0;
static int32_t combop_cnt=0;
static int32_t layer_cnt=0;

static char paste_ffc_menu_text[21];
static char paste_ffc_menu_text2[21];
static char follow_warp_menu_text[21];
static char follow_warp_menu_text2[21];

static int fake_mouse_b(){return 0;}
static int (*mouseb_proc)();
static bool killed_mouse = false;
void zq_killmouse()
{
	if(killed_mouse) return;
	mouseb_proc = gui_mouse_b;
	gui_mouse_b = fake_mouse_b;
	killed_mouse = true;
}
void zq_restoremouse()
{
	if(!killed_mouse) return;
	gui_mouse_b = mouseb_proc;
	killed_mouse = false;
}


void domouse()
{
	static int mouse_down = 0;
	static int32_t scrolldelay = 0;
	auto mousexy = zc_get_mouse();
	auto x = mousexy.first;
	auto y = mousexy.second;
	double startx=mapscreen_x+(showedges?(16*mapscreen_single_scale):0);
    double starty=mapscreen_y+(showedges?(16*mapscreen_single_scale):0);
	int32_t startxint=mapscreen_x+(showedges?int32_t(16*mapscreen_single_scale):0);
	int32_t startyint=mapscreen_y+(showedges?int32_t(16*mapscreen_single_scale):0);
	int32_t cx=(x-startx)/(16*mapscreen_single_scale);
	int32_t cy=(y-starty)/(16*mapscreen_single_scale);
	ComboPosition combo_pos = {cx, cy};

	if (draw_mode == dm_auto)
	{
		if (combo_pos != mouse_combo_pos)
			combobrushoverride = get_autocombo_floating_cid(combo_pos, false);
	}
	else
		combobrushoverride = -1;

	mouse_combo_pos = combo_pos;
	update_combobrush();
	
	++scrolldelay;
	
	bool x_on_list = false;
	for(auto q = 0; q < num_combo_cols; ++q)
	{
		if((x>=combolist[q].x) && (x<combolist[q].x+(combolist[q].xscale*combolist[q].w)))
		{
			x_on_list = true;
			break;
		}
	}
	if(MouseScroll && x_on_list && (key[KEY_LSHIFT] || key[KEY_RSHIFT] || (scrolldelay&3)==0))
	{
		int32_t test_list=0;
		
		for(test_list=0; test_list<num_combo_cols; ++test_list)
		{
			if((x>=combolist[test_list].x) && (x<combolist[test_list].x+(combolist[test_list].xscale*combolist[test_list].w)))
			{
				break;
			}
		}
		
		if(test_list<num_combo_cols)
		{
			if(y>=combolist[test_list].y-mouse_scroll_h && y<=combolist[test_list].y && First[test_list])
			{
				if((CHECK_CTRL_CMD)&&(key[KEY_ALT] || key[KEY_ALTGR]))
				{
					First[test_list]=0;
				}
				else if(CHECK_CTRL_CMD)
				{
					First[test_list]-=zc_min(First[test_list],256);
				}
				else if(key[KEY_ALT] || key[KEY_ALTGR])
				{
					First[test_list]-=zc_min(First[test_list],(combolist[test_list].w*combolist[test_list].h));
				}
				else
				{
					First[test_list]-=zc_min(First[test_list],combolist[test_list].w);
				}
			}
			
			if(y>=combolist[test_list].y+(combolist[test_list].h*combolist[test_list].yscale)-1 && y<combolist[test_list].y+(combolist[test_list].h*combolist[test_list].yscale)+mouse_scroll_h-1 && First[test_list]<(MAXCOMBOS-(combolist[test_list].w*combolist[test_list].h)))
			{
				int32_t offset = combolist[test_list].w*combolist[test_list].h;
				
				if((CHECK_CTRL_CMD)&&(key[KEY_ALT] || key[KEY_ALTGR]))
				{
					First[test_list]=MAXCOMBOS-offset;
				}
				else if(CHECK_CTRL_CMD)
				{
					First[test_list] = zc_min(MAXCOMBOS-offset, First[test_list]+256);
				}
				else if(key[KEY_ALT] || key[KEY_ALTGR])
				{
					First[test_list] = zc_min(MAXCOMBOS-offset, First[test_list]+ offset);
				}
				else
				{
					First[test_list] = zc_min(MAXCOMBOS - offset, First[test_list] + combolist[test_list].w);
				}
			}
		}
	}

	// The screen for this combo_pos, layer 0. Used to access ffcs.
	mapscr* scr = Map.Scr(combo_pos);
	// The screen for this combo_pos at the CurrentLayer. Could be same as scr.
	mapscr* draw_mapscr = scr && CurrentLayer ? Map.Scr(combo_pos, CurrentLayer) : scr;
	int c = combo_pos.truncate();
	set_active_visible_screen(scr);

//-------------
//tooltip stuff
//-------------
	if (active_visible_screen && isinRect(x,y,startxint,startyint,startxint+(256*mapscreen_screenunit_scale)-1,startyint+(176*mapscreen_screenunit_scale)-1))
	{
		static int mapscr_tooltip_id = ttip_register_id();
		bool did_ffttip = false;
		int num_ffcs = scr->numFFC();
		for(int32_t i=num_ffcs-1; i>=0; i--)
			if(scr->ffcs[i].data !=0 && (CurrentLayer<2 || (scr->ffcs[i].flags&ffc_overlay)))
			{
				int32_t ffx = scr->ffcs[i].x.getFloor() + active_visible_screen->dx * 256;
				int32_t ffy = scr->ffcs[i].y.getFloor() + active_visible_screen->dy * 176;
				int32_t ffw = scr->ffTileWidth(i)*16;
				int32_t ffh = scr->ffTileHeight(i)*16;
				int32_t cx2 = (x-startxint)/mapscreen_single_scale;
				int32_t cy2 = (y-startyint)/mapscreen_single_scale;
				
				if(cx2 >= ffx && cx2 < ffx+ffw && cy2 >= ffy && cy2 < ffy+ffh)
				{
					// FFC tooltip
					if(tooltip_current_ffc != i)
					{
						clear_tooltip();
					}
					
					tooltip_current_ffc = i;
					char msg[1024] = {0};
					auto& ff = scr->ffcs[i];
					sprintf(msg,"FFC: %d Combo: %d\nCSet: %d Type: %s\nScript: %s",
							i+1, ff.data,ff.data,
							combo_class_buf[combobuf[ff.data].type].name,
							(ff.script<=0 ? "(None)" : ffcmap[ff.script-1].scriptname.substr(0,400).c_str()));
					ttip_install(mapscr_tooltip_id, msg, startxint+(ffx*mapscreen_single_scale), startyint+(ffy*mapscreen_single_scale), ffw*mapscreen_single_scale, ffh*mapscreen_single_scale, x, y);
					did_ffttip = true;
					break;
				}
			}
		if(!did_ffttip)
		{
			if(unsigned(c) < 176 && draw_mapscr && !gui_mouse_b())
			{
				int cid = draw_mapscr->data[c];
				newcombo const& cmb = combobuf[cid];
				std::ostringstream oss;
				int cs = draw_mapscr->cset[c];
				int sflag = draw_mapscr->sflag[c];
				oss << "Pos: " << c
					<< "\nCombo: " << cid
					<< "\nCSet: " << cs;
				if(sflag || cmb.flag)
					oss << "\nFlags: " << sflag << ", " << (int)cmb.flag;
				if(cmb.type)
					oss << "\nCombo type: " << combo_class_buf[cmb.type].name;
				if(cmb.label[0])
					oss << "\nLabel: " << cmb.label;
				ttip_install(mapscr_tooltip_id, oss.str().c_str(), startxint+(cx*16*mapscreen_single_scale), startyint+(cy*16*mapscreen_single_scale), 16*mapscreen_single_scale, 16*mapscreen_single_scale, x, y);
			}
		}
	}
	
	{
		size_and_pos* squares[4] = {&itemsqr_pos,&stairsqr_pos,&warparrival_pos,&flagsqr_pos};
		for(int32_t j=0; j<4; j++)
		{
			auto& square = *squares[j];
			if(square.rect(x,y))
			{
				char msg[160];
				sprintf(msg,
						j==0 ? "Item Location" :
						j==1 ? "Stairs Secret\nTriggered when a Trigger Push Block is pushed." :
						j==2 ? "Arrival Square\nPlayer's location when they begin/resume the game." :
						"Combo Flags");
				update_tooltip(x,y,square,msg);
			}
		}
		
		// Warp Returns
		for(int32_t j=0; j<4; j++)
		{
			size_and_pos& wret = warpret_pos[j];
			if(wret.rect(x,y))
			{
				char msg[160];
				sprintf(msg,"Warp Return Square %c\nPlayer's destination after warping to this screen.",(char)('A'+j));
				update_tooltip(x,y,wret,msg);
			}
		}
		
		// Enemies
		if(enemy_prev_pos.rect(x,y))
		{
			char msg[160];
			sprintf(msg,"Enemies that appear on this screen.");
			update_tooltip(x,y,enemy_prev_pos,msg);
		}
		
		int32_t cmd = commands_list.rectind(x,y);
		if(cmd > -1)
		{
			update_tooltip(x,y,commands_list.subsquare(cmd),
				fmt::format("Fav Command {}: {}\n{}", cmd,
					get_hotkey_name(favorite_commands[cmd]),
					get_hotkey_helptext(favorite_commands[cmd])).c_str());
		}
	}
	
	if(draw_mode==dm_alias)
	{
		for(int32_t j=0; j<num_combo_cols; ++j)
		{
			auto& sqr = comboaliaslist[j];
			auto ind = sqr.rectind(x,y);
			if(ind > -1)
			{
				auto c2=ind+combo_alistpos[j];
				char msg[80];
				sprintf(msg, "Combo alias %d", c2);
				update_tooltip(x,y,sqr.subsquare(ind), msg);
			}
		}
	}
	else if(draw_mode==dm_cpool)
	{
		for(int32_t j=0; j<num_combo_cols; ++j)
		{
			auto& sqr = comboaliaslist[j];
			auto ind = sqr.rectind(x,y);
			if(ind > -1)
			{
				auto c2=ind+combo_pool_listpos[j];
				char msg[80];
				sprintf(msg, "Combo Pool %d", c2);
				update_tooltip(x,y,sqr.subsquare(ind), msg);
			}
		}
		if(cpool_prev_visible && combopool_prevbtn.rect(x,y))
		{
			if(do_layer_button_reset(combopool_prevbtn.x,combopool_prevbtn.y,
				combopool_prevbtn.w,combopool_prevbtn.h,
				weighted_cpool ? "Weighted" : "Unweighted",0,true))
			{
				weighted_cpool = !weighted_cpool;
			}
		}
	}
	else if (draw_mode == dm_auto)
	{
		for (int32_t j = 0; j < num_combo_cols; ++j)
		{
			auto& sqr = comboaliaslist[j];
			auto ind = sqr.rectind(x, y);
			if (ind > -1)
			{
				auto c2 = ind + combo_auto_listpos[j];
				char msg[80];
				sprintf(msg, "Auto Combo %d", c2);
				update_tooltip(x, y, sqr.subsquare(ind), msg);
			}
		}
	}
	else
	{
		if(combo_preview.rect(x,y))
		{
			auto str = "Combo Colors:\n"+get_combo_colornames(Combo,CSet);
			update_tooltip(x,y,combo_preview,str.c_str());
		}
		else if(comboprev_buf[0] && combo_preview_text1.rect(x,y))
		{
			update_tooltip(x,y,combo_preview_text1,comboprev_buf);
		}
		else if(comboprev_buf2[0] && combo_preview_text2.rect(x,y))
		{
			update_tooltip(x,y,combo_preview_text2,comboprev_buf2);
		}
		else for(int32_t j=0; j<num_combo_cols; ++j)
		{
			auto& sqr = combolist[j];
			auto ind = sqr.rectind(x,y);
			if(ind > -1)
			{
				int32_t c2=ind+First[j];
				std::ostringstream oss;
				newcombo const& cmb = combobuf[c2];
				oss << "Combo " << c2 << ": " << combo_class_buf[cmb.type].name;
				if(cmb.flag != 0)
					oss << "\nInherent flag: " << ZI.getMapFlagName(cmb.flag);
				if(!cmb.label.empty())
					oss << "\nLabel: " << cmb.label;
					
				update_tooltip(x,y,sqr.subsquare(ind), oss.str().c_str());
			}
		}
	}
	
	if (favorites_list.rect(x, y))
	{
		int32_t f = favorites_list.rectind(x, y);
		int32_t row = f / favorites_list.w;
		int32_t col = f % favorites_list.w;
		f = (row * FAVORITECOMBO_PER_ROW) + col;

		auto& sqr = favorites_list.subsquare(col, row);

		char buf[180];
		if (favorite_combos[f] == -1)
			sprintf(buf, "Fav Combo %d\nEmpty", f);
		else
		{
			switch (favorite_combo_modes[f])
			{
			case dm_alias:
				sprintf(buf, "Fav Combo %d\nAlias %d", f, favorite_combos[f]);
				break;
			case dm_cpool:
				sprintf(buf, "Fav Combo %d\nPool %d", f, favorite_combos[f]);
				break;
			case dm_auto:
				sprintf(buf, "Fav Combo %d\nAutocombo %d", f, favorite_combos[f]);
				break;
			default:
				sprintf(buf, "Fav Combo %d\nCombo %d", f, favorite_combos[f]);
			}
		}
		update_tooltip(x, y, sqr, buf);
	}

	size_and_pos const& real_mini = zoomed_minimap ? real_minimap_zoomed : real_minimap;
	auto ind = real_mini.rectind(x,y);
	if(ind > -1)
	{
		char buf[80];
		sprintf(buf,"0x%02X (%d)", ind, ind);
		ttip_install(minimap_tooltip_id, buf, real_mini.subsquare(ind), real_mini.x+real_mini.tw(), real_mini.y-16);
		ttip_set_highlight_thickness(minimap_tooltip_id, zoomed_minimap ? 2 : 1);
		// Make sure always above the other tooltip items to the right of the map (even in big map mode).
		ttip_set_z_index(minimap_tooltip_id, 100);
		ttip_clear_timer();
	}
	else
	{
		ttip_uninstall(minimap_tooltip_id);
	}
	
	// Mouse clicking stuff
	int real_mb = gui_mouse_b();
	int mb = real_mb & ~mouse_down; //Only handle clicks that have not been handled already
	auto mz = mouse_z;
	bool lclick = mb&1;
	bool rclick = mb&2;

	if (mb && hotkeys_is_active())
	{
		hotkeys_toggle_display(false);
		while (gui_mouse_b())
		{
			custom_vsync();
		}
		return;
	}
	
	FONT* tfont = font;
	if(zoomed_minimap)
	{
		if((lclick||rclick) && !minimap_zoomed.rect(x,y))
		{
			// 'Clicked off'
			mmap_set_zoom(false);
			goto domouse_doneclick;
		}
	}
	
	if(real_mb==0)
	{
		mouse_down = 0;
		canfill=true;
	}
	else if(lclick || rclick)
	{
		//on the minimap
		if(real_mini.rect(x,y))
		{
			if(lclick)
				select_scr();
			else if(rclick && !(mouse_down&2))
			{
				mmap_set_zoom(!zoomed_minimap);
			}
			goto domouse_doneclick;
		}
		
		if(zoomed_minimap && minimap_zoomed.rect(x,y))
			goto domouse_doneclick; //Eat clicks
		
		//on the map tabs
		font = get_custom_font(CFONT_GUI);
		for(int32_t btn=0; btn<mappage_count; ++btn)
		{
			char tbuf[15];
			sprintf(tbuf, "%d:%02X", map_page[btn].map+1, map_page[btn].screen);
			auto& sqr = map_page_bar[btn];
			if(sqr.rect(x,y))
			{
				if(do_layer_button_reset(sqr.x,sqr.y,sqr.w,sqr.h,tbuf,(btn==current_mappage?D_SELECTED:0)))
				{
					draw_layer_button(screen, sqr.x,sqr.y,sqr.w,sqr.h,tbuf,D_SELECTED);
					map_page[current_mappage].map=Map.getCurrMap();
					map_page[current_mappage].screen=Map.getCurrScr();
					current_mappage=btn;
					Map.setCurrMap(map_page[current_mappage].map);
					Map.setCurrScr(map_page[current_mappage].screen);
					rebuild_trans_table(); //Woo
				}
				goto domouse_doneclick;
			}
		}
		
		if(compactbtn.rect(x,y))
		{
			if(do_text_button(compactbtn.x, compactbtn.y, compactbtn.w, compactbtn.h, is_compact ? "< Expand" : "> Compact"));
				toggle_is_compact();
			goto domouse_doneclick;
		}

		if(!zoom_in_btn_disabled && zoominbtn.rect(x,y))
		{
			if(do_text_button(zoominbtn.x, zoominbtn.y, zoominbtn.w, zoominbtn.h, "+"))
				change_mapscr_zoom(-1);
			goto domouse_doneclick;
		}

		if(!zoom_out_btn_disabled && zoomoutbtn.rect(x,y))
		{
			if(do_text_button(zoomoutbtn.x, zoomoutbtn.y, zoomoutbtn.w, zoomoutbtn.h, "-"))
				change_mapscr_zoom(1);
			goto domouse_doneclick;
		}
		
		font = get_zc_font(font_lfont_l);
		if(combo_merge_btn.rect(x,y))
		{
			bool merged = is_compact ? compact_merged_combopane : large_merged_combopane;
			if(do_text_button(combo_merge_btn.x,combo_merge_btn.y,combo_merge_btn.w,combo_merge_btn.h,merged ? "<|>" : ">|<"))
			{
				toggle_merged_mode();
			}
			goto domouse_doneclick;
		}

		if(favorites_zoombtn.rect(x,y))
		{
			bool zoomed = is_compact ? compact_zoomed_fav : large_zoomed_fav;
			if(do_text_button(favorites_zoombtn.x,favorites_zoombtn.y,favorites_zoombtn.w,favorites_zoombtn.h,zoomed ? "-" : "+"))
			{
				toggle_favzoom_mode();
			}
			goto domouse_doneclick;
		}
		else if(favorites_x.rect(x,y))
		{
			if(do_text_button(favorites_x.x,favorites_x.y,favorites_x.w,favorites_x.h,"X"))
			{
				AlertDialog("Clear Favorite Combos",
					"Are you sure you want to clear all favorite combos?",
					[&](bool ret,bool)
					{
						if(ret)
						{
							for(auto q = 0; q < MAXFAVORITECOMBOS; ++q)
							{
								favorite_combos[q] = -1;
								favorite_combo_modes[q] = dm_normal;
							}
							saved = false;
							refresh(rFAVORITES);
						}
					}).show();
			}
			goto domouse_doneclick;
		}
		else if(favorites_infobtn.rect(x,y))
		{
			if(do_text_button(favorites_infobtn.x,favorites_infobtn.y,favorites_infobtn.w,favorites_infobtn.h,"?"))
			{
				InfoDialog("Favorite Combos",
					"On LClick (empty): Sets clicked favorite to the current combo."
					"\nOn LClick: Sets current combo to clicked favorite."
					"\nShift+LClick: Sets clicked favorite to current combo."
					"\nCtrl+LClick: Clears clicked favorite."
					"\nAlt+LClick: Scrolls to clicked favorite."
					"\nRClick: Opens context menu."
					"\n\nClick the Page buttons (<-/->) to cycle between pages (RClick to jump to a page)"
					"\nClick the Zoom button (+/-) to toggle zoom level."
					"\nClick the X button to clear all favorite combos.").show();
			}
			goto domouse_doneclick;
		}
		else if(favorites_pgleft.rect(x,y))
		{
			if (do_text_button(favorites_pgleft.x, favorites_pgleft.y, favorites_pgleft.w, favorites_pgleft.h, is_compact ? "<" : "<-"))
			{
				if (rclick)
				{
					if(auto val = popup_num_menu(x, y, 1, 9, FavoriteComboPage, [](int p){return fmt::format("Page {}",p);}))
						FavoriteComboPage = vbound(*val-1, 0, 8);
				}
				else
					FavoriteComboPage = FavoriteComboPage == 0 ? 8 : --FavoriteComboPage;
				reload_zq_gui();
			}
			goto domouse_doneclick;
		}
		else if(favorites_pgright.rect(x,y))
		{
			if (do_text_button(favorites_pgright.x, favorites_pgright.y, favorites_pgright.w, favorites_pgright.h, is_compact ? ">" : "->"))
			{
				if (rclick)
				{
					if(auto val = popup_num_menu(x, y, 1, 9, FavoriteComboPage, [](int p){return fmt::format("Page {}",p);}))
						FavoriteComboPage = vbound(*val-1, 0, 8);
				}
				else
					FavoriteComboPage = FavoriteComboPage == 8 ? 0 : ++FavoriteComboPage;
				reload_zq_gui();
			}
			goto domouse_doneclick;
		}

		if(commands_zoombtn.rect(x,y))
		{
			bool zoomed = is_compact ? compact_zoomed_cmd : large_zoomed_cmd;
			if(do_text_button(commands_zoombtn.x,commands_zoombtn.y,commands_zoombtn.w,commands_zoombtn.h,zoomed ? "-" : "+"))
			{
				toggle_cmdzoom_mode();
			}
			goto domouse_doneclick;
		}
		else if(commands_x.rect(x,y))
		{
			if(do_text_button(commands_x.x,commands_x.y,commands_x.w,commands_x.h,"X"))
			{
				AlertDialog("Clear Favorite Commands",
					"Are you sure you want to clear all favorite commands?",
					[&](bool ret,bool)
					{
						if(ret)
						{
							char buf[20];
							for(auto q = 0; q < MAXFAVORITECOMMANDS; ++q)
							{
								write_fav_command(q,0);
							}
							refresh(rFAVORITES);
						}
					}).show();
			}
			goto domouse_doneclick;
		}
		else if(commands_infobtn.rect(x,y))
		{
			if(do_text_button(commands_infobtn.x,commands_infobtn.y,commands_infobtn.w,commands_infobtn.h,"?"))
			{
				InfoDialog("Favorite Commands",
					"On LClick (empty): Choose a favorite command"
					"\nOn LClick: Runs the favorite command"
					"\nShift+Click: Choose a favorite command"
					"\nRClick: Choose a favorite command"
					"\nCtrl+Click: Clears clicked command"
					"\nAlt+Click: Shows info on the favorite command"
					"\n\nClick the Zoom button (+/-) to toggle zoom level"
					"\nClick the X button to clear all favorite commands").show();
			}
			goto domouse_doneclick;
		}
		font=tfont;
		
		// On the layer panel
		font = get_custom_font(CFONT_GUI);
		for(int32_t i=0; i<=6; ++i)
		{
			int32_t spacing_offs = is_compact ? 2 : 10;
			int32_t rx = (i * (layerpanel_buttonwidth+spacing_offs+layerpanel_checkbox_wid)) + layer_panel.x+(is_compact?2:6);
			int32_t ry = layer_panel.y;
			
			if ((i == 0 || mapscreen_valid_layers[i - 1]) && isinRect(x,y,rx,ry,rx+layerpanel_buttonwidth-1,ry+layerpanel_buttonheight-1))
			{
				char tbuf[15];

				if (Map.getViewSize() > 1)
				{
					sprintf(tbuf, "%d", i);
				}
				else if (i != 0 && mapscreen_valid_layers[i - 1])
				{
					if (is_compact)
					{
						sprintf(tbuf, "%s%d %d:%02X",
							(i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"",
							i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
					}
					else
					{
						sprintf(tbuf, "%s%d (%d:%02X)",
							(i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"",
							i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
					}
				}
				else
				{
					sprintf(tbuf, "%d", i);
				}
				
				if(do_text_button(rx, ry, layerpanel_buttonwidth, layerpanel_buttonheight, tbuf))
				{
					CurrentLayer = i;
					goto domouse_doneclick;
				}
			}
			
			auto cbyofs = (layerpanel_buttonheight-layerpanel_checkbox_hei)/2;
			if(isinRect(x,y,rx+layerpanel_buttonwidth+1,ry+cbyofs,rx+layerpanel_buttonwidth+1+layerpanel_checkbox_wid-1,ry+2+layerpanel_checkbox_hei-1))
			{
				do_checkbox(menu1,rx+layerpanel_buttonwidth+1,ry+cbyofs,layerpanel_checkbox_wid,layerpanel_checkbox_hei,LayerMaskInt[i]);
				goto domouse_doneclick;
			}
		}
		font=tfont;
		
		//Uses lclick/rclick separately
		
		//on the map screen
		if(isinRect(x,y,startxint,startyint,startxint+(256*mapscreen_screenunit_scale)-1,startyint+(176*mapscreen_screenunit_scale)-1))
		{
			if (lclick)
			{
				Map.setCurrScr(Map.getScreenForPosition(combo_pos));
			}

			if (draw_mode == dm_auto)
			{
				if (CHECK_CTRL_CMD)
				{
					if (canfill)
					{
						switch (fill_type)
						{
						case 0:
							flood();
							break;

						case 1:
							fill_4();
							break;

						case 2:
							fill_8();
							break;

						case 3:
							fill2_4();
							break;

						case 4:
							fill2_8();
							break;
						}

						canfill = false;
					}
				}
				else
					draw(key[KEY_LSHIFT] || key[KEY_RSHIFT]);
			}
			else if (scr && lclick)
			{
				int32_t cx2 = (x-startxint)/mapscreen_single_scale;
				int32_t cy2 = (y-startyint)/mapscreen_single_scale;
				
				// Move items
				if (scr->hasitem && active_visible_screen)
				{
					int32_t ix = scr->itemx + active_visible_screen->dx * 256;
					int32_t iy = scr->itemy + active_visible_screen->dy * 176;
					
					if(cx2 >= ix && cx2 < ix+16 && cy2 >= iy && cy2 < iy+16)
						doxypos(scr->itemx, scr->itemy, 11, SNAP_HALF, SNAP_NONE, true, 0, 0, 16, 16);
				}
				
				// Move FFCs
				int num_ffcs = scr->numFFC();
				for(int32_t i=num_ffcs-1; i>=0; i--)
					if(scr->ffcs[i].data !=0 && (CurrentLayer<2 || (scr->ffcs[i].flags&ffc_overlay)))
					{
						int32_t ffx = scr->ffcs[i].x.getFloor() + active_visible_screen->dx * 256;
						int32_t ffy = scr->ffcs[i].y.getFloor() + active_visible_screen->dy * 176;
						
						if(cx2 >= ffx && cx2 < ffx+(scr->ffTileWidth(i)*16) && cy2 >= ffy && cy2 < ffy+(scr->ffTileHeight(i)*16))
						{
							moveffc(i, cx2, cy2);
							break;
						}
					}
				
				if(key[KEY_ALT]||key[KEY_ALTGR])
				{
					if (!draw_mapscr) return;

					Combo=draw_mapscr->data[c];
					if(AutoBrush)
						BrushWidth = BrushHeight = 1;
					if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
						CSet=draw_mapscr->cset[c];
					if(CHECK_CTRL_CMD)
						First[current_combolist]=scrollto_cmb(draw_mapscr->data[c]);
				}
				else if(CHECK_CTRL_CMD)
				{
					if(canfill)
					{
						switch(fill_type)
						{
						case 0:
							flood();
							break;
							
						case 1:
							fill_4();
							break;
							
						case 2:
							fill_8();
							break;
							
						case 3:
							fill2_4();
							break;
							
						case 4:
							fill2_8();
							break;
						}
						
						canfill=false;
					}
				}
				else draw(key[KEY_LSHIFT] || key[KEY_RSHIFT]);
			}
			else if (scr && rclick)
			{
				ComboBrushPause=1;
				refresh(rMAP);
				restore_mouse();
				ComboBrushPause=0;
				
				bool clickedffc = false;
				
				// FFC right-click menu
				// This loop also serves to find the free ffc with the smallest slot number.
				int num_ffcs = scr->numFFC();
				uint32_t earliestfreeffc = num_ffcs;
				for(int32_t i=num_ffcs-1; i>=0; i--)
				{
					auto data = scr->ffcs[i].data;
					if(data==0)
					{
						if(i < earliestfreeffc)
							earliestfreeffc = i;
						continue;
					}
					
					if(clickedffc || !(scr->valid&mVALID))
						continue;
						
					if(data!=0 && (CurrentLayer<2 || (scr->ffcs[i].flags&ffc_overlay)))
					{
						int32_t ffx = scr->ffcs[i].x.getFloor() + active_visible_screen->dx * 256;
						int32_t ffy = scr->ffcs[i].y.getFloor() + active_visible_screen->dy * 176;
						int32_t cx2 = (x-startxint)/mapscreen_single_scale;
						int32_t cy2 = (y-startyint)/mapscreen_single_scale;
						
						if(cx2 >= ffx && cx2 < ffx+(scr->ffTileWidth(i)*16) && cy2 >= ffy && cy2 < ffy+(scr->ffTileHeight(i)*16))
						{
							NewMenu rcmenu
							{
								{ "Copy FFC", [&](){Map.CopyFFC(active_visible_screen->screen, i);} },
								{ "Paste FFC data", [&]()
									{
										bool didconfirm = false;
										AlertDialog("Confirm Paste",
											"Really replace the FFC with the data of the copied FFC?",
											[&](bool ret,bool)
											{
												if(ret)
													didconfirm = true;
											}).show();
										if(didconfirm)
										{
											auto set_ffc_data = Map.getCopyFFCData();
											set_ffc_data.x = scr->ffcs[i].x;
											set_ffc_data.y = scr->ffcs[i].y;
											Map.DoSetFFCCommand(Map.getCurrMap(), active_visible_screen->screen, i, set_ffc_data);
										}
									}, nullopt, Map.getCopyFFC() < 0 },
								{ "Edit FFC", [&](){call_ffc_dialog(i, active_visible_screen->scr, active_visible_screen->screen);} },
								{ "Clear FFC", [&]()
									{
										bool didconfirm = false;
										AlertDialog("Confirm Clear",
											"Really clear this Freeform Combo?",
											[&](bool ret,bool)
											{
												if(ret)
													didconfirm = true;
											}).show();
										if(didconfirm)
										{
											Map.DoSetFFCCommand(Map.getCurrMap(), active_visible_screen->screen, i, {
												.x = 0,
												.y = 0,
												.vx = 0,
												.vy = 0,
												.ax = 0,
												.ay = 0,
												.data = 0,
												.cset = 0,
												.delay = 0,
												.link = 0,
												.script = 0,
												.tw = 1,
												.th = 1,
												.ew = 16,
												.eh = 16,
												.flags = ffc_none,
												.initd = 0,
											});
											saved = false;
										}
									} },
								{ "Snap to Grid", [&]()
									{
										int oldffx = scr->ffcs[i].x.getInt();
										int oldffy = scr->ffcs[i].y.getInt();
										int pos = COMBOPOS(oldffx,oldffy);
										int newffy = COMBOY(pos);
										int newffx = COMBOX(pos);

										auto set_ffc_data = set_ffc_command::create_data(scr->ffcs[i]);
										set_ffc_data.x = newffx;
										set_ffc_data.y = newffy;
										Map.DoSetFFCCommand(Map.getCurrMap(), active_visible_screen->screen, i, set_ffc_data);

										saved = false;
									} },
							};
							rcmenu.pop(x, y);
							clickedffc = true;
							break;
						}
					}
				}
				
				// Combo right-click menu
				if(!clickedffc)
				{
					int warpindex = Map.warpindex(scr->data[c]);
					string txt_twarp_follow, txt_twarp_edit, txt_ffc_edit, txt_ffc_paste;
					bool show_ffcs = earliestfreeffc < MAXFFCS;
					bool dis_paste_ffc = Map.getCopyFFC() < 0;
					bool show_warps = warpindex > -1;
					bool show_warpback = Map.has_warpback();
					// FFC-specific options
					if(earliestfreeffc < MAXFFCS)
					{
						txt_ffc_edit = fmt::format("Edit New FFC {}",earliestfreeffc+1);
						if(Map.getCopyFFC()>-1)
							txt_ffc_paste = fmt::format("Paste FFC as FFC {}",earliestfreeffc+1);
						else
							txt_ffc_paste = "Paste FFC";
					}
					
					if(warpindex > -1)
					{
						char letter = warpindex==4 ? 'R' : 'A'+warpindex;
						txt_twarp_follow = fmt::format("Follow Tile Warp {}",letter);
						txt_twarp_edit = fmt::format("Edit Tile Warp {}",letter);
					}

					NewMenu draw_rc_menu
					{
						{ "Select Combo", [&]()
							{
								Combo = draw_mapscr->data[c];
								if(AutoBrush)
									BrushWidth = BrushHeight = 1;
							}, nullopt, !draw_mapscr },
						{ "Scroll to Combo", [&]()
							{
								First[current_combolist] = scrollto_cmb(draw_mapscr->data[c]);
							}, nullopt, !draw_mapscr },
						{ "Edit Combo", [&]()
							{
								edit_combo(draw_mapscr->data[c],true,draw_mapscr->cset[c]);
							}, nullopt, !draw_mapscr },
						{},
						{ "Replace All", [&](){replace(combo_pos);} },
						{ "Draw Block", &draw_block_menu },
						{ "Brush Settings ", &brush_menu },
						{ "Set Fill Type ", &fill_menu },
					};
					if(show_warps || show_warpback)
					{
						draw_rc_menu.add_sep();
						if(show_warpback)
							draw_rc_menu.add({ "Warp Back", [&](){Map.warpback();} });
						if(show_warps)
						{
							draw_rc_menu.add({ txt_twarp_follow, [&](){follow_twarp(warpindex);} });
							draw_rc_menu.add({ txt_twarp_edit, [&](){edit_twarp(warpindex);} });
						}
					}
					if(show_ffcs)
					{
						draw_rc_menu.add_sep();
						draw_rc_menu.add({ txt_ffc_edit, [&]()
							{
								ffdata tempdat;
								// x, y are ints on ffdata (but ffc x, y are zfix), so *10000
								tempdat.x = ((int((x-startxint)/mapscreen_single_scale)&(~0x0007)) % 256) * 10000;
								tempdat.y = ((int((y-startyint)/mapscreen_single_scale)&(~0x0007)) % 176) * 10000;
								tempdat.data = Combo;
								tempdat.cset = CSet;
								call_ffc_dialog(earliestfreeffc, tempdat, active_visible_screen->scr, active_visible_screen->screen);
							} });
						draw_rc_menu.add({ txt_ffc_paste, [&]()
							{
								auto set_ffc_data = Map.getCopyFFCData();
								set_ffc_data.x = ((int((x-startxint)/mapscreen_single_scale)&(~0x0007)) % 256);
								set_ffc_data.y = ((int((y-startyint)/mapscreen_single_scale)&(~0x0007)) % 176);
								Map.DoSetFFCCommand(Map.getCurrMap(), active_visible_screen->screen, earliestfreeffc, set_ffc_data);
							}, nullopt, dis_paste_ffc });
					}
					draw_rc_menu.add_sep();
					draw_rc_menu.add({ "Screen", &rc_menu_screen });
					draw_rc_menu.pop(x,y);
				}
			}
			goto domouse_doneclick;
		}
		
		//on the drawing mode button
		font = get_custom_font(CFONT_GUI);
		if(drawmode_btn.rect(x,y))
		{
			if(lclick)
			{
				if(do_text_button(drawmode_btn.x,drawmode_btn.y,drawmode_btn.w,drawmode_btn.h,dm_names[draw_mode]))
					onDrawingMode();
			}
			else if(rclick)
				drawing_mode_menu.pop(x,y);
			goto domouse_doneclick;
		}
		font=tfont;
		
		//Squares
		//
		set_active_visible_screen(Map.CurrScr());
		{
			if(squarepanel_swap_btn.rect(x,y))
			{
				toggle_compact_sqr_mode();
				goto domouse_doneclick;
			}
			if(squarepanel_up_btn.rect(x,y))
			{
				cycle_compact_sqr(false);
				goto domouse_doneclick;
			}
			if(squarepanel_down_btn.rect(x,y))
			{
				cycle_compact_sqr(true);
				goto domouse_doneclick;
			}
			
			bool do_dummyxy = false;
			bool dummymode = key[KEY_LSHIFT] || key[KEY_RSHIFT];
			
			if(itemsqr_pos.rect(x,y))
			{
				if(dummymode) do_dummyxy = true;
				else
				{
					onItem();
					
					if(!rclick && Map.CurrScr()->hasitem)
						doxypos(Map.CurrScr()->itemx,Map.CurrScr()->itemy,11,SNAP_HALF,SNAP_NONE);
					goto domouse_doneclick;
				}
			}
			
			if(stairsqr_pos.rect(x,y))
			{
				if(dummymode) do_dummyxy = true;
				else
				{
					doxypos(Map.CurrScr()->stairx,Map.CurrScr()->stairy,14,SNAP_WHOLE);
					goto domouse_doneclick;
				}
			}
			
			if(warparrival_pos.rect(x,y))
			{
				if(dummymode) do_dummyxy = true;
				else
				{
					if(get_qr(qr_NOARRIVALPOINT))
					{
						info_dsa("Arrival Square",
								"The arrival square cannot be used unless the QR 'Use Warp Return "
								"Points Only' under 'Quest->Options->Combos' is disabled."
								"\nGenerally, this square only exists for compatibility purposes, and is not used"
								" in creating new quests.",
								"dsa_warparrival");
					}
					else doxypos(Map.CurrScr()->warparrivalx,Map.CurrScr()->warparrivaly,10,SNAP_HALF,SNAP_NONE);
					goto domouse_doneclick;
				}
			}
			
			if(flagsqr_pos.rect(x,y))
			{
				if(dummymode) do_dummyxy = true;
				else
				{
					onFlags();
					goto domouse_doneclick;
				}
			}
			
			for(auto q = 0; q < 4; ++q)
			{
				if(warpret_pos[q].rect(x,y))
				{
					if(dummymode) do_dummyxy = true;
					else
					{
						doxypos(Map.CurrScr()->warpreturnx[q],Map.CurrScr()->warpreturny[q],9,SNAP_HALF,SNAP_NONE);
						goto domouse_doneclick;
					}
				}
			}
			
			if(enemy_prev_pos.rect(x,y))
			{
				if(dummymode) do_dummyxy = true;
				else
				{
					onEnemies();
					goto domouse_doneclick;
				}
			}
			
			if(do_dummyxy)
			{
				byte x = 0, y = 0;
				showxypos_dummy = true;
				doxypos(x,y,13,SNAP_HALF,SNAP_NONE);
				goto domouse_doneclick;
			}
		}
		
		if(draw_mode==dm_alias)
		{
			for(int32_t j=0; j<num_combo_cols; ++j)
			{
				if(combolistscrollers[j].rectind(x,y)==0 && !mouse_down)
				{
					scrollup(j);
					goto domouse_doneclick;
				}
				else if(combolistscrollers[j].rectind(x,y)==1 && !mouse_down)
				{
					scrolldown(j);
					goto domouse_doneclick;
				}
				else if(comboaliaslist[j].rect(x,y))
				{
					select_comboa(j);
					
					if(rclick && comboaliaslist[j].rect(gui_mouse_x(),gui_mouse_y()))
						popup_cpane_rc(x, y);
					goto domouse_doneclick;
				}
			}
		}
		else if(draw_mode==dm_cpool)
		{
			for(int32_t j=0; j<num_combo_cols; ++j)
			{
				if(combolistscrollers[j].rectind(x,y)==0 && !mouse_down)
				{
					scrollup(j);
					goto domouse_doneclick;
				}
				else if(combolistscrollers[j].rectind(x,y)==1 && !mouse_down)
				{
					scrolldown(j);
					goto domouse_doneclick;
				}
				else if(comboaliaslist[j].rect(x,y))
				{
					select_combop(j);
					
					if(rclick && comboaliaslist[j].rect(gui_mouse_x(),gui_mouse_y()))
						popup_cpane_rc(x, y);
					goto domouse_doneclick;
				}
			}
		}
		else if (draw_mode == dm_auto)
		{
			for (int32_t j = 0; j < num_combo_cols; ++j)
			{
				if (combolistscrollers[j].rectind(x, y) == 0 && !mouse_down)
				{
					scrollup(j);
					goto domouse_doneclick;
				}
				else if (combolistscrollers[j].rectind(x, y) == 1 && !mouse_down)
				{
					scrolldown(j);
					goto domouse_doneclick;
				}
				else if (comboaliaslist[j].rect(x, y))
				{
					select_autocombo(j);

					if(rclick && comboaliaslist[j].rect(gui_mouse_x(),gui_mouse_y()))
						popup_cpane_rc(x, y);
					goto domouse_doneclick;
				}
			}
		}
		else
		{
			for(int32_t j=0; j<num_combo_cols; ++j)
			{
				if(combolistscrollers[j].rectind(x,y)==0 && !mouse_down)
				{
					scrollup(j);
					goto domouse_doneclick;
				}
				else if(combolistscrollers[j].rectind(x,y)==1 && !mouse_down)
				{
					scrolldown(j);
					goto domouse_doneclick;
				}
				else if(combolist[j].rect(x,y))
				{
					select_combo(j);
					
					if(rclick && combolist[j].rect(gui_mouse_x(),gui_mouse_y()))
						popup_cpane_rc(x, y);
					goto domouse_doneclick;
				}
			}
		}
		
		//on the favorites list
		if(favorites_list.rect(x,y))
		{
			if(lclick)
			{
				int32_t f=favorites_list.rectind(x,y);
				int32_t row=f/favorites_list.w;
				int32_t col=f%favorites_list.w;
				f = (row*FAVORITECOMBO_PER_ROW)+col;
				int32_t fp = f + FAVORITECOMBO_PER_PAGE * FavoriteComboPage;
				
				bool dmcond = favorite_combos[fp] < 0;
				if((key[KEY_LSHIFT] || key[KEY_RSHIFT] || dmcond) && !(CHECK_CTRL_CMD))
				{
					int32_t tempcb=ComboBrush;
					ComboBrush=0;
					
					while(gui_mouse_b())
					{
						x=gui_mouse_x();
						y=gui_mouse_y();
						
						switch(draw_mode)
						{
							case dm_alias:
								if (favorite_combos[fp] != combo_apos || favorite_combo_modes[fp] != dm_alias)
								{
									favorite_combo_modes[fp] = dm_alias;
									favorite_combos[fp] = combo_apos;
									saved = false;
								}
								break;
							case dm_cpool:
								if (favorite_combos[fp] != combo_pool_pos || favorite_combo_modes[fp] != dm_cpool)
								{
									favorite_combo_modes[fp] = dm_cpool;
									favorite_combos[fp] = combo_pool_pos;
									saved = false;
								}
								break;
							case dm_auto:
								if (favorite_combos[fp] != combo_auto_pos || favorite_combo_modes[fp] != dm_auto)
								{
									favorite_combo_modes[fp] = dm_auto;
									favorite_combos[fp] = combo_auto_pos;
									saved = false;
								}
								break;
							default:
								if (favorite_combos[fp] != Combo || favorite_combo_modes[fp] != dm_normal)
								{
									if (BrushWidth > 1 || BrushHeight > 1)
									{
										add_favorite_combo_block(f, Combo, key[KEY_LSHIFT] || key[KEY_RSHIFT]);
										break;
									}
									favorite_combo_modes[fp] = dm_normal;
									favorite_combos[fp] = Combo;
									saved = false;
								}
						}
						
						custom_vsync();
						refresh(rALL | rFAVORITES);
					}
					
					ComboBrush=tempcb;
				}
				else if(CHECK_CTRL_CMD)
				{
					int32_t tempcb=ComboBrush;
					ComboBrush=0;
					
					while(gui_mouse_b())
					{
						x=gui_mouse_x();
						y=gui_mouse_y();
						
						if(favorite_combos[fp]!=-1)
						{
							favorite_combo_modes[fp] = dm_normal;
							favorite_combos[fp]=-1;
							saved=false;
						}
						
						custom_vsync();
						refresh(rALL | rFAVORITES);
					}
					
					ComboBrush=tempcb;
				}
				else if(key[KEY_ALT] || key[KEY_ALTGR])
				{
					if(select_favorite())
					{
						switch(favorite_combo_modes[fp])
						{
							case dm_alias:
								combo_alistpos[current_comboalist]=scrollto_alias(combo_apos);
								break;
							case dm_cpool:
								combo_pool_listpos[current_cpoollist] = scrollto_cpool(combo_pool_pos);
								break;
							case dm_auto:
								combo_auto_listpos[current_cautolist] = scrollto_cauto(combo_auto_pos);
								break;
							default:
								First[current_combolist]=scrollto_cmb(Combo);
						}
					}
				}
				else
				{
					select_favorite();
				}
			}
			else if(rclick)
			{
				bool valid=select_favorite();
				
				if(valid)
				{
					int f = favorites_list.rectind(x,y);
					int row = f/favorites_list.w;
					int col = f%favorites_list.w;
					f = (row*FAVORITECOMBO_PER_ROW) + col + (FAVORITECOMBO_PER_PAGE * FavoriteComboPage);
					popup_favorites_rc(f, x, y);
				}
			}
			goto domouse_doneclick;
		}
		
		//on the commands buttons
		int32_t cmd = commands_list.rectind(x,y);
		if(cmd > -1)
		{
			uint hkey = favorite_commands[cmd];
			bool shift=(key[KEY_LSHIFT] || key[KEY_RSHIFT]);
			bool ctrl=(CHECK_CTRL_CMD);
			bool alt=(key[KEY_ALT] || key[KEY_ALTGR]);
			bool dis = disabled_hotkey(hkey);
			auto& btn = commands_list.subsquare(cmd);
			if(!dis||rclick||shift||ctrl||alt)
			{
				FONT *tfont=font;
				font=get_custom_font(CFONT_FAVCMD);
				if(do_layer_button_reset(btn.x,btn.y,btn.w,btn.h,
					get_hotkey_name(hkey),
					selected_hotkey(hkey)?D_SELECTED:0,
					true))
				{
					font=tfont;
					if(alt)
					{
						show_hotkey_info(hkey);
					}
					else if(ctrl)
					{
						write_fav_command(cmd,0);
					}
					else if(rclick || shift || hkey==ZQKEY_NULL_KEY)
					{
						if(auto newkey = select_fav_command())
							write_fav_command(cmd,*newkey);
					}
					else
					{
						run_hotkey(hkey);
					}
				}
				
				font=tfont;
			}
			goto domouse_doneclick;
		}
	}

domouse_doneclick:
	mouse_down |= mb&3;
	
	if(mouse_z!=0)
	{
		int32_t z=0;
		
		for(int32_t j=0; j<num_combo_cols; ++j)
		{
			z=abs(mouse_z);
			
			if(key[KEY_ALT]||key[KEY_ALTGR])
			{
				z*=combolist[j].h;
			}
			
			
			if(draw_mode == dm_alias)
			{
				if(comboaliaslist[j].rect(x,y))
				{
					if(mouse_z<0)  //scroll down
					{
						combo_alistpos[current_comboalist] = zc_min(MAXCOMBOALIASES - comboaliaslist[j].w*comboaliaslist[j].h,
															 combo_alistpos[current_comboalist]+comboaliaslist[j].w*z);
					}
					else //scroll up
					{
						if(combo_alistpos[current_comboalist]>0)
						{
							combo_alistpos[current_comboalist]-=zc_min(combo_alistpos[current_comboalist],comboaliaslist[j].w*z);
						}
					}
					goto domouse_donez;
				}
			}
			else if(draw_mode == dm_cpool)
			{
				if(comboaliaslist[j].rect(x,y))
				{
					if(mouse_z<0)  //scroll down
					{
						combo_pool_listpos[current_cpoollist] = zc_min(MAXCOMBOPOOLS - comboaliaslist[j].w*comboaliaslist[j].h,
															 combo_pool_listpos[current_cpoollist]+comboaliaslist[j].w*z);
					}
					else //scroll up
					{
						if(combo_pool_listpos[current_cpoollist]>0)
						{
							combo_pool_listpos[current_cpoollist]-=zc_min(combo_pool_listpos[current_cpoollist],comboaliaslist[j].w*z);
						}
					}
					goto domouse_donez;
				}
			}
			else if (draw_mode == dm_auto)
			{
				if (comboaliaslist[j].rect(x, y))
				{
					if (mouse_z < 0)  //scroll down
					{
						combo_auto_listpos[current_cautolist] = zc_min(MAXAUTOCOMBOS - comboaliaslist[j].w * comboaliaslist[j].h,
							combo_auto_listpos[current_cautolist] + comboaliaslist[j].w * z);
					}
					else //scroll up
					{
						if (combo_auto_listpos[current_cautolist] > 0)
						{
							combo_auto_listpos[current_cautolist] -= zc_min(combo_auto_listpos[current_cautolist], comboaliaslist[j].w * z);
						}
					}
					goto domouse_donez;
				}
			}
			else
			{
				if(combolist[j].rect(x,y))
				{
					if(mouse_z<0)  //scroll down
					{
						First[current_combolist] = zc_min(MAXCOMBOS-combolist[j].w*combolist[j].h,
							First[current_combolist] + combolist[j].w*z);
					}
					else //scroll up
					{
						if(First[current_combolist]>0)
						{
							First[current_combolist]-=zc_min(First[current_combolist],combolist[j].w*z);
						}
					}
					goto domouse_donez;
				}
			}
		}
		
		z=abs(mouse_z);
		
		if(real_mini.rect(x,y))
		{
			for(int32_t i=0; i<z; ++i)
			{
				if(mouse_z>0) onIncMap();
				else onDecMap();
			}
			goto domouse_donez;
		}
		
		if(is_compact && compact_square_panels
			&& squares_panel.rect(x,y))
		{
			cycle_compact_sqr(mouse_z < 0);
			goto domouse_donez;
		}
domouse_donez:
		position_mouse_z(0);
	}
	font = tfont;
	active_visible_screen = nullptr;
}

int32_t d_viewpal_proc(int32_t msg, DIALOG *d, int32_t c)
{
	int32_t ret = d_bitmap_proc(msg, d, c);
	char* buf = (char*)d->dp2; //buffer to store the color code in
	DIALOG* d2 = (DIALOG*)d->dp3; //DIALOG* to update the text proc
	if(!buf)
		return ret;
	switch(msg)
	{
		case MSG_IDLE:
		case MSG_GOTMOUSE:
		case MSG_LOSTMOUSE:
			break;
		default:
			return ret;
	}
	char t[16];
	memcpy(t, buf, 16);
	int32_t x = gui_mouse_x() - d->x;
	int32_t y = gui_mouse_y() - d->y;
	if(msg != MSG_LOSTMOUSE && isinRect(x, y, 0, 0, d->w-1, d->h-1))
	{
		float palscale = 1.5;
		for(int32_t i = 0; i<256; ++i)
			if(isinRect(x,y,(int32_t)(((i&31)<<3)*palscale),(int32_t)(((i&0xE0)>>2)*palscale), (int32_t)((((i&31)<<3)+7)*palscale),(int32_t)((((i&0xE0)>>2)+7)*palscale)))
			{
				sprintf(buf, "0x%02X (%03d)     ", i, i); //Extra spaces to increase drawn width, so it draws the blank area
				break;
			}
	}
	else memset(buf, ' ', 15);
	if(strcmp(buf, t) && d2 && d2->proc == jwin_text_proc && d2->dp == d->dp2)
		object_message(d2, MSG_DRAW, 0);
	return ret;
}

static DIALOG showpal_dlg[] =
{
	/* (dialog proc)     (x)     (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
	{ jwin_win_proc,     24,      68,  272,  119,  vc(14),  vc(1),      0,    D_EXIT,        0,             0, (void *) "View Palette",        NULL,  NULL },
	{ jwin_frame_proc,   30,   76+16,  260,   68,       0,      0,      0,         0,  FR_DEEP,             0, NULL,                           NULL,  NULL },
	{ d_viewpal_proc,    32,   76+18,  256,   64,       0,      0,      0,         0,        0,             0, NULL,                           NULL,  NULL },
	{ jwin_text_proc,  32+8,76+18+66,   20,    8,  vc(11),  vc(1),      0,         0,        0,             0, NULL,                           NULL,  NULL },
	{ jwin_button_proc, 130,  144+18,   61,   21,  vc(14),  vc(1),     27,    D_EXIT,        0,             0, (void *) "OK",                  NULL,  NULL },
	{ d_timer_proc,       0,       0,    0,    0,       0,      0,      0,         0,        0,             0, NULL,                           NULL,  NULL },
	{ NULL,               0,       0,    0,    0,       0,      0,      0,         0,        0,             0, NULL,                           NULL,  NULL }
};

int32_t onShowPal()
{
	float palscale = 1.5;
	
	BITMAP *palbmp = create_bitmap_ex(8,(int32_t)(256*palscale),(int32_t)(64*palscale));
	
	if(!palbmp)
		return D_O_K;
	clear_to_color(palbmp,jwin_pal[jcBOX]); //If not cleared, random static appears between swatches! -E
	showpal_dlg[0].dp2=get_zc_font(font_lfont);
	
	for(int32_t i=0; i<256; i++)
		rectfill(palbmp,(int32_t)(((i&31)<<3)*palscale),(int32_t)(((i&0xE0)>>2)*palscale), (int32_t)((((i&31)<<3)+7)*palscale),(int32_t)((((i&0xE0)>>2)+7)*palscale),i);
	showpal_dlg[2].dp=(void *)palbmp;
	char buf[16] = {0};
	showpal_dlg[2].dp2=(void *)buf;
	showpal_dlg[2].dp3=(void *)&(showpal_dlg[3]);
	showpal_dlg[3].dp=(void *)buf;
	showpal_dlg[3].dp2=(void *)get_zc_font(font_deffont);
	
	large_dialog(showpal_dlg);
	do_zqdialog(showpal_dlg,2);
	destroy_bitmap(palbmp);
	return D_O_K;
}

static DIALOG csetfix_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,        72,   80,   176+1,  96+1,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "CSet Fix", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_radio_proc,      104+22,  108,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Full Screen", NULL, NULL },
    { jwin_radio_proc,      104+22,  118+2,  80+1,   8+1,    vc(14),  vc(1),  0,       D_SELECTED, 0,             0, (void *) "Dungeon Floor", NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      104+22,  128+4,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "All Layers", NULL, NULL },
    { jwin_button_proc,     90,   152,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  152,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t onCSetFix()
{
    restore_mouse();
    csetfix_dlg[0].dp2=get_zc_font(font_lfont);
    int32_t s=2,x2=14,y2=9;
    
    large_dialog(csetfix_dlg);
        
    if(do_zqdialog(csetfix_dlg,-1)==6)
    {
        if(csetfix_dlg[2].flags&D_SELECTED)
        {
            s=0;
            x2=16;
            y2=11;
        }
        
        if(csetfix_dlg[5].flags&D_SELECTED)
        {
            /*
			int32_t drawmap, drawscr;
			if (CurrentLayer==0)
			{
				drawmap=Map.getCurrMap();
				drawscr=Map.getCurrScr();
			}
			else
			{
				drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
				drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
			}
			mapscr* draw_mapscr = Map.AbsoluteScr(drawmap, drawscr);
			if(!draw_mapscr) return;
			saved=false;
			Map.Ugo();

			if(!(draw_mapscr->valid&mVALID))
			{
				Map.CurrScr()->valid|=mVALID;
				draw_mapscr->valid|=mVALID;
				Map.setcolor(Color);
			}
			for(int32_t i=0; i<176; i++)
			{
				draw_mapscr->data[i]=Combo;
				draw_mapscr->cset[i]=CSet;
			}
			refresh(rMAP+rSCRMAP);
            */
        }
        
        Map.StartListCommand();
        for(int32_t y=s; y<y2; y++)
        {
            for(int32_t x=s; x<x2; x++)
            {
                Map.DoSetComboCommand(Map.getCurrMap(), Map.getCurrScr(), (y<<4)+x, -1, CSet);
            }
        }
        Map.FinishListCommand();
        
        refresh(rMAP);
        saved = false;
    }
    
    return D_O_K;
}
static bool doAllSolidWater()
{
	for(int32_t i=0; i < MAXCOMBOS; ++i)
	{
		if(combo_class_buf[combobuf[i].type].water!=0)
		{
			combobuf[i].walk |= 0x0F; //Solid
		}
	}
	return true;
}
static bool doNoSolidWater()
{
	for(int32_t i=0; i < MAXCOMBOS; ++i)
	{
		if(combo_class_buf[combobuf[i].type].water!=0)
		{
			combobuf[i].walk &= ~0x0F; //Non-solid
		}
	}
	return true;
}
int32_t onWaterSolidity()
{
	AlertFuncDialog("Water Conversion",
		"Forcibly set the solidity of all 'Liquid' combos in the quest?",
		""
	).add_buttons(2,
		{ "Solid", "Non-Solid", "Cancel" },
		{ doAllSolidWater, doNoSolidWater, nullptr }
	).show();
    return D_O_K;
}

static bool doAllEffectSquare()
{
	for(int32_t i=0; i < MAXCOMBOS; ++i)
	{
		combobuf[i].walk |= 0xF0; //Effect
	}
	return true;
}
static bool doBlankEffectSquare()
{
	for(int32_t i=0; i < MAXCOMBOS; ++i)
	{
		if(combobuf[i].is_blank(true))
		{
			combobuf[i].walk |= 0xF0; //Effect
		}
	}
	return true;
}

int32_t onEffectFix()
{
	AlertFuncDialog("Effect Square Conversion",
		"Forcibly fill the green effect square of all combos in the quest?",
		""
	).add_buttons(2,
		{ "All", "Blank Only", "Cancel" },
		{ doAllEffectSquare, doBlankEffectSquare, nullptr }
	).show();
    return D_O_K;
}

static bool clear_green_arrival_squares()
{
	for(mapscr& scr : TheMaps)
	{
		if(!scr.valid) continue;
		scr.warparrivalx = 0;
		scr.warparrivaly = 0;
	}
	set_qr(qr_NOARRIVALPOINT, true);
	return true;
}

static bool replace_green_arrival_squares()
{
	// Check for conflicts first
	bool has_conflicts = false;
	
	for(mapscr& scr : TheMaps)
	{
		if(!scr.valid) continue;
		if(!(scr.warparrivalx || scr.warparrivaly)) continue;
		if(scr.warpreturnx[0] || scr.warpreturny[0])
		{
			has_conflicts = true;
			break;
		}
	}
	
	enum
	{
		NOT_ASKED = -1,
		MODE_FORCE, MODE_IGNORE, MODE_FIND_IGNORE, MODE_FIND_FORCE, MODE_CANCEL
	};
	int mode = NOT_ASKED;
	
	if(has_conflicts)
	{
		AlertFuncDialog("Handle Conflicts",
			"Warp Square A is not available for all screens that have arrival squares."
			" How should this be handled? (See '?' for more info)",
			"Overwrite A: Replace the existing warp return square A with the position of the green arrival square"
			"\nIgnore: Do nothing if warp return square A exists"
			"\nFind Space or Ignore: Choose another, unused, square to set to the position of the green arrival square."
			" If none are unused, 'Ignore' instead."
			"\nFind Space or Overwrite: Choose another, unused square to set to the position of the green arrival square."
			" If none are unused, 'Overwrite A' instead."
			"\nCancel: Don't do anything"
		).add_buttons(1,
			{ "Overwrite A", "Ignore", "Find Space or Ignore", "Find Space or Overwrite A", "Cancel" },
			mode
		).show();
		if(mode == NOT_ASKED || mode == MODE_CANCEL)
			return false;
	}
	for(mapscr& scr : TheMaps)
	{
		if(!scr.valid) continue;
		if(!(scr.warparrivalx || scr.warparrivaly)) continue;
		int indx = 0;
		if(scr.warpreturnx[0] || scr.warpreturny[0])
		{
			if(mode == MODE_IGNORE) continue; // Warp A not free, so ignore
			if(mode != MODE_FORCE)
			{
				for(int q = 1; q < 4; ++q)
				{
					if(scr.warpreturnx[q] || scr.warpreturny[q])
						continue;
					indx = q; // Use this warp, since it's free
					break;
				}
				if(indx == 0 && mode == MODE_FIND_IGNORE)
					continue; // Nothing free, so ignore
			}
		}
		scr.warpreturnx[indx] = scr.warparrivalx;
		scr.warpreturny[indx] = scr.warparrivaly;
		scr.warparrivalx = 0;
		scr.warparrivaly = 0;
	}
	set_qr(qr_NOARRIVALPOINT, true);
	return true;
}

int32_t onRemoveOldArrivalSquare()
{
	AlertFuncDialog("Arrival Square Removal",
		"Clear the old green 'Arrival' squares for the whole quest?"
		"\n(There will be no further confirmation, and this operation cannot be undone)",
		""
	).add_buttons(2,
		{ "Replace With Blue Return Square", "Clear Completely", "Cancel" },
		{ replace_green_arrival_squares, clear_green_arrival_squares, nullptr }
	).show();
    return D_O_K;
}

byte* getPalPointer(int32_t pal, int32_t cset)
{
	if (pal < 0) return colordata + CSET(cset)*3;
	byte* ret = colordata + CSET(pal*pdLEVEL+poLEVEL)*3;
	switch(cset)
	{
		case 2: case 3: case 4:
			return ret + CSET(cset-2)*3;
		case 9:
			return ret + CSET(3)*3;
		case 1:
			return ret + CSET(13)*3;
		case 5:
			return ret + CSET(14)*3;
		case 7:
			return ret + CSET(15)*3;
		case 8:
			return ret + CSET(16)*3;
	}
	return NULL;
}

void copyCSet(int32_t destpal, int32_t destcset, int32_t srcpal, int32_t srccset)
{
	byte* dest = getPalPointer(destpal, destcset);
	byte* src = getPalPointer(srcpal, srccset);
	if (dest && src)
	{
		memcpy(dest, src, 16*3);
	}
}

void setColorPalette(int32_t flags, int32_t lowpal, int32_t highpal)
{
	for (auto q = lowpal; q <= highpal; ++q)
	{
		for (auto c = 0; c < 12; ++c)
		{
			if (!(flags&(1<<c))) continue;
			copyCSet(q, c, -1, c);
		}
	}
}

void setPitDamage(int32_t flags, int32_t lowcombo, int32_t highcombo, int32_t damage)
{
	for(int32_t i=lowcombo; i < highcombo; ++i)
	{
		if((combobuf[i].type == cPITFALL && (flags & (1<<0)))
		|| (combobuf[i].type == cWATER && !(combobuf[i].usrflags & (1<<0)) && (flags & (1<<1)))
		|| (combobuf[i].type == cWATER && (combobuf[i].usrflags & (1<<0)) && (flags & (1<<2))))
		{
			if ((combobuf[i].type != cPITFALL || (flags & (1<<9)) || !(combobuf[i].usrflags & (1<<0)))
			&& ((flags & (1<<8)) || combobuf[i].attributes[0] == 0))
				combobuf[i].attributes[0] = damage*10000; 
		}
	}
}

static DIALOG template_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 72,   80,   176+1,  116+1,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "NES Dungeon Template", NULL, NULL },
    { d_comboframe_proc,   178,  122+3,  20,  20,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      180,  124+3,  16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    //  { d_bitmap_proc,     180,  104,  16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_radio_proc,      104+33,  128+3,  64+1,   8+1,    vc(14),  vc(1),  0,       D_SELECTED, 0,             0, (void *) "Floor:", NULL, NULL },
    { jwin_radio_proc,      104+33,  148+3,  64+1,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "No Floor", NULL, NULL },
    { jwin_button_proc,     90,   172,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  172,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       104,  102,    16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "This copies the contents of",        NULL,   NULL },
    { jwin_text_proc,       104,  112,    16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "screen 83 of the current map.",    NULL,   NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t onTemplate()
{
    static bool donethis=false;
    
    if(!donethis||!(key[KEY_LSHIFT]||key[KEY_RSHIFT]))
    {
        template_dlg[2].d1=Combo;
        template_dlg[2].fg=CSet;
        donethis=true;
    }
    
    restore_mouse();
    
    if(Map.getCurrScr()==TEMPLATE)
        return D_O_K;
        
    //  BITMAP *floor_bmp = create_bitmap_ex(8,16,16);
    //  if(!floor_bmp) return D_O_K;
    template_dlg[0].dp2=get_zc_font(font_lfont);
    //  put_combo(floor_bmp,0,0,Combo,CSet,0,0);
    //  template_dlg[2].dp=floor_bmp;
    
    large_dialog(template_dlg);
        
    if(do_zqdialog(template_dlg,-1)==5)
    {
        saved=false;
		int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
        Map.DoTemplateCommand((template_dlg[3].flags==D_SELECTED) ? template_dlg[2].d1 : -1, template_dlg[2].fg, screen);
        refresh(rMAP+rSCRMAP);
    }
    
    //  destroy_bitmap(floor_bmp);
    return D_O_K;
}

int32_t d_sel_scombo_proc(int32_t msg, DIALOG *d, int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    switch(msg)
    {
    case MSG_CLICK:
        while(gui_mouse_b())
        {
            int32_t x = zc_min(zc_max(gui_mouse_x() - d->x,0)>>4, 15);
            int32_t y = zc_min(zc_max(gui_mouse_y() - d->y,0)&0xF0, 160);
            
            if(x+y != d->d1)
            {
                d->d1 = x+y;
                custom_vsync();
                d_sel_scombo_proc(MSG_DRAW,d,0);
            }
        }
        
        break;
        
    case MSG_DRAW:
    {
        blit((BITMAP*)(d->dp),screen,0,0,d->x,d->y,d->w,d->h);
        int32_t x = d->x + (((d->d1)&15)<<4);
        int32_t y = d->y + ((d->d1)&0xF0);
        rect(screen,x,y,x+15,y+15,vc(15));
    }
    break;
    }
    
    return D_O_K;
}

static DIALOG cflag_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     70,   163,  51,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     190,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     130,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Help", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void questrev_help()
{
	jwin_alert("Help","The revision number of your quest.",NULL,NULL,"O&K",NULL,'k',16,get_zc_font(font_lfont));
}

void questminrev_help()
{
	jwin_alert("Help","If a player's saved game was from a revision less than the minimum", "revision, they have to restart from the beginning.", "This is useful if you make major changes to your quest.","O&K",NULL,'k',16,get_zc_font(font_lfont));
}

int32_t select_cflag(const char *prompt,int32_t flag)
{
    cflag_dlg[0].dp=(void *)prompt;
    cflag_dlg[0].dp2=get_zc_font(font_lfont);
    GUI::ListData ld = GUI::ZCListData::mapflag(numericalFlags, true);
	ListData select_cflag_list = ld.getJWin(&font);
    int32_t index = ld.findIndex(flag);
	cflag_dlg[2].d1=index;
	cflag_dlg[2].dp=(void *) &select_cflag_list;
    
    large_dialog(cflag_dlg);
        
    int32_t ret;
    
    do
    {
        ret=do_zqdialog(cflag_dlg,2);
        
        if(ret==5)
        {
            cflag_help(ld.getValue(cflag_dlg[2].d1));
        }
    }
    while(ret==5);
    
    if(ret==0||ret==4)
    {
        position_mouse_z(0);
        return -1;
    }
    
    return ld.getValue(cflag_dlg[2].d1);
}

int32_t select_flag(int32_t &f)
{
    int32_t ret=select_cflag("Flag Type",f);
    
    if(ret>=0)
    {
        f=ret;
        return true;
    }
    
    return false;
}

int32_t d_scombo_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    switch(msg)
    {
    case MSG_CLICK:
    {
        int32_t c2=d->d1;
        int32_t cs=d->fg;
        int32_t f=d->d2;
        
        if(d->bg==1 || (CHECK_CTRL_CMD))
        {
            while(gui_mouse_b())
            {
                /* do nothing */
                rest(1);
            }
            
            if(select_flag(f))
            {
                d->d2=f;
                
            }
        }
        else if(key[KEY_LSHIFT])
        {
            if(gui_mouse_b()&1)
            {
                d->d1++;
                
                if(d->d1>=MAXCOMBOS) d->d1=0;
            }
            else if(gui_mouse_b()&2)
            {
                d->d1--;
                
                if(d->d1<0) d->d1=MAXCOMBOS-1;
            }
        }
        else if(key[KEY_RSHIFT])
        {
            if(gui_mouse_b()&1)
            {
                d->fg++;
                
                if(d->fg>11) d->fg=0;
            }
            else if(gui_mouse_b()&2)
            {
                d->fg--;
                
                if(d->fg<0) d->fg=11;
            }
        }
        else if(key[KEY_ALT])
        {
            if(gui_mouse_b()&1)
            {
                d->d1 = Combo;
                d->fg = CSet;
            }
        }
        else
        {
            if(select_combo_2(c2, cs))
            {
                d->d1=c2;
                d->fg=cs;
            }
        }
        
        return D_REDRAW;
    }
    break;
    
    case MSG_DRAW:
		d->w = 32;
		d->h = 32;
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        BITMAP *bigbmp = create_bitmap_ex(8,d->w,d->h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
			if(d->bg) //flags only
			{
				put_flag(buf,0,0,d->d2);
			}
            else if(d->d1)
            {
                putcombo(buf,0,0,d->d1,d->fg);
                
                if(Flags&cFLAGS)
                    put_flags(buf,0,0,d->d1,d->fg,cFLAGS,d->d2);
            }
            
            stretch_blit(buf, bigbmp, 0,0, 16, 16, 0, 0, d->w, d->h);
            destroy_bitmap(buf);
            blit(bigbmp,screen,0,0,d->x-1,d->y-1,d->w,d->h);
            destroy_bitmap(bigbmp);
        }
        
        
        /*BITMAP *buf = create_bitmap_ex(8,16,16);
        if(buf)
        {
          clear_bitmap(buf);
          if(d->d1)
            putcombo(buf,0,0,d->d1,d->fg);
        
          blit(buf,screen,0,0,d->x,d->y,d->w,d->h);
          destroy_bitmap(buf);
        }*/
        break;
    }
    
    return D_O_K;
}

/*int32_t d_scombo2_proc(int32_t msg, DIALOG *d, int32_t c)
{
  //these are here to bypass compiler warnings about unused arguments
  c=c;

  switch(msg)
  {
    case MSG_CLICK:
    if (CHECK_CTRL_CMD)
    {
      select_scombo(d->d1);
    }
    else
    {
      select_scombo(d->d1);
    }
    d_scombo_proc(MSG_DRAW,d,0);
    break;


    case MSG_DRAW:
    BITMAP *buf = create_bitmap_ex(8,16,16);
    if(buf)
    {
      clear_bitmap(buf);
      Map.draw_secret2(buf,d->d1);
      blit(buf,screen,0,0,d->x,d->y,16,16);
      destroy_bitmap(buf);
    }
    break;
  }

  return D_O_K;
}*/

int32_t onSecretF();

static int32_t secret_burn_list[] =
{
    // dialog control number
    4, 5, 6, 7, 48, 49, 50, 51, 92, 93, 94, 95, -1
};

static int32_t secret_arrow_list[] =
{
    // dialog control number
    8, 9, 10, 52, 53, 54, 96, 97, 98, -1
};

static int32_t secret_bomb_list[] =
{
    // dialog control number
    11, 12, 55, 56, 99, 100, -1
};

static int32_t secret_boomerang_list[] =
{
    // dialog control number
    13, 14, 15, 57, 58, 59, 101, 102, 103, -1
};

static int32_t secret_magic_list[] =
{
    // dialog control number
    16, 17, 60, 61, 104, 105, -1
};

static int32_t secret_sword_list[] =
{
    // dialog control number
    18, 19, 20, 21, 22, 23, 24, 25, 62, 63, 64, 65, 66, 67, 68, 69, 106, 107, 108, 109, 110, 111, 112, 113, -1
};

static int32_t secret_misc_list[] =
{
    // dialog control number
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, -1
};

static TABPANEL secret_tabs[] =
{
    // (text)
    { (char *)"Burn",       D_SELECTED,   secret_burn_list, 0, NULL },
    { (char *)"Arrow",       0,           secret_arrow_list, 0, NULL },
    { (char *)"Bomb",        0,           secret_bomb_list, 0, NULL },
    { (char *)"Boomerang",   0,           secret_boomerang_list, 0, NULL },
    { (char *)"Magic",       0,           secret_magic_list, 0, NULL },
    { (char *)"Sword",       0,           secret_sword_list, 0, NULL },
    { (char *)"Misc",        0,           secret_misc_list, 0, NULL },
    { NULL,                  0,           NULL, 0, NULL }
};

static DIALOG secret_dlg[] =
{
    // (dialog proc)            (x)     (y)     (w)     (h)     (fg)        (bg)    (key)        (flags)  (d1)         (d2)   (dp)                             (dp2)   (dp3)
    {  jwin_win_proc,             0,      0,    301,    212,    vc(14),     vc(1),      0,       D_EXIT,     0,           0,  NULL,        NULL,   NULL                },
    {  jwin_tab_proc,             6,     25,    289,    156,    0,          0,          0,       0,          0,           0, (void *) secret_tabs,            NULL, (void *)secret_dlg  },
    {  jwin_button_proc,         80,    187,     61,     21,    vc(14),     vc(1),     13,       D_EXIT,     0,           0, (void *) "OK",                   NULL,   NULL                },
    {  jwin_button_proc,        160,    187,     61,     21,    vc(14),     vc(1),     27,       D_EXIT,     0,           0, (void *) "Cancel",               NULL,   NULL                },
    // 4
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Any Fire",             NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Strong Fire",          NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Fire",           NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Divine Fire",          NULL,   NULL                },
    //8
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wooden Arrow",         NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Silver Arrow",         NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Golden Arrow",         NULL,   NULL                },
    //11
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Bomb",                 NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Super Bomb",           NULL,   NULL                },
    //13
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wooden Boomerang",     NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Boomerang",      NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Fire Boomerang",       NULL,   NULL                },
    //16
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wand Magic",           NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Reflected Magic",      NULL,   NULL                },
    //18
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Sword",                NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "White Sword",          NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Sword",          NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Master Sword",         NULL,   NULL                },
    {  jwin_text_proc,          160,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Sword Beam",           NULL,   NULL                },
    {  jwin_text_proc,          160,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "White Sword Beam",     NULL,   NULL                },
    {  jwin_text_proc,          160,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Sword Beam",     NULL,   NULL                },
    {  jwin_text_proc,          160,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Master Sword Beam",    NULL,   NULL                },
    //26
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Stairs",               NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Reflected Fireball",   NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Hookshot",             NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wand",                 NULL,   NULL                },
    {  jwin_text_proc,           12,    141,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Hammer",               NULL,   NULL                },
    {  jwin_text_proc,           12,    163,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Any Weapon",           NULL,   NULL                },
    //32
    {  jwin_ctext_proc,         235,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Flags 16-31",          NULL,   NULL                },
    {  jwin_text_proc,           87,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Secrets->Next (Flag only)",           NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 03",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 04",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 05",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 06",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 07",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 08",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 09",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 10",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 11",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 12",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 13",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 14",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 15",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 16",      NULL,   NULL                },
    //48 (burn)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //52 (arrow)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //55 (bomb)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //57 (boomerang)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //60 (magic)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //62 (sword)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //70 (misc)
    {  jwin_frame_proc,          63,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,          63,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,          63,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,          63,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,          63,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,          63,    157,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //76 (16-32)
    {  jwin_frame_proc,         192,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         214,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         236,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         258,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         192,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         214,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         236,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         258,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         192,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         214,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         236,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         258,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         192,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         214,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         236,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         258,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    
    //92 (burn)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //96 (arrow)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //99 (bomb)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //101 (boomerang)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //104 (magic)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //106 (sword)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //114 (misc)
    {  d_scombo_proc,            65,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,            65,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,            65,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,            65,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,            65,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,            65,    159,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //120 (16-32)
    {  d_scombo_proc,           194,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           216,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           238,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           260,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           194,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           216,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           238,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           260,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           194,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           216,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           238,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           260,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           194,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           216,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           238,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           260,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //136 Secrets->Next
    {  jwin_frame_proc,         158,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
	{  d_scombo_proc,           160,     49,     16,     16,    0,          1,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //138
    {  d_keyboard_proc,           0,      0,      0,      0,    0,          0,          0,       0,          KEY_F1,      0, (void *) onHelp,                 NULL,   NULL                },
    {  d_keyboard_proc,           0,      0,      0,      0,    0,          0,          'f',     0,          0,           0, (void *) onSecretF,              NULL,   NULL                },
    {  d_timer_proc,              0,      0,      0,      0,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  NULL,                      0,      0,      0,      0,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                }
};

int32_t onSecretF()
{
    Flags^=cFLAGS;
    object_message(secret_dlg+1, MSG_DRAW, 0);
    return D_O_K;
}


int32_t onSecretCombo()
{
    secret_dlg[0].dp2=get_zc_font(font_lfont);
    
    
    mapscr *s;
    
    if(CurrentLayer==0)
    {
        s=Map.CurrScr();
    }
    else
    {
        //   s=TheMaps[(Map.CurrScr()->layermap[CurrentLayer-1]-1)*MAPSCRS+(Map.CurrScr()->layerscreen[CurrentLayer-1])];
        s=Map.AbsoluteScr((Map.CurrScr()->layermap[CurrentLayer-1]-1), (Map.CurrScr()->layerscreen[CurrentLayer-1]));
    }
	if(!s) return D_O_K;
    
    char secretcombonumstr[27];
    sprintf(secretcombonumstr,"Secret Combos for Layer %d", CurrentLayer);
    secret_dlg[0].dp = secretcombonumstr;
    
    secret_dlg[92].d1 = s->secretcombo[sBCANDLE];
    secret_dlg[92].fg = s->secretcset[sBCANDLE];
    secret_dlg[92].d2 = s->secretflag[sBCANDLE];
    
    secret_dlg[93].d1 = s->secretcombo[sRCANDLE];
    secret_dlg[93].fg = s->secretcset[sRCANDLE];
    secret_dlg[93].d2 = s->secretflag[sRCANDLE];
    
    secret_dlg[94].d1 = s->secretcombo[sWANDFIRE];
    secret_dlg[94].fg = s->secretcset[sWANDFIRE];
    secret_dlg[94].d2 = s->secretflag[sWANDFIRE];
    
    secret_dlg[95].d1 = s->secretcombo[sDIVINEFIRE];
    secret_dlg[95].fg = s->secretcset[sDIVINEFIRE];
    secret_dlg[95].d2 = s->secretflag[sDIVINEFIRE];
    
    secret_dlg[96].d1 = s->secretcombo[sARROW];
    secret_dlg[96].fg = s->secretcset[sARROW];
    secret_dlg[96].d2 = s->secretflag[sARROW];
    
    secret_dlg[97].d1 = s->secretcombo[sSARROW];
    secret_dlg[97].fg = s->secretcset[sSARROW];
    secret_dlg[97].d2 = s->secretflag[sSARROW];
    
    secret_dlg[98].d1 = s->secretcombo[sGARROW];
    secret_dlg[98].fg = s->secretcset[sGARROW];
    secret_dlg[98].d2 = s->secretflag[sGARROW];
    
    secret_dlg[99].d1 = s->secretcombo[sBOMB];
    secret_dlg[99].fg = s->secretcset[sBOMB];
    secret_dlg[99].d2 = s->secretflag[sBOMB];
    
    secret_dlg[100].d1 = s->secretcombo[sSBOMB];
    secret_dlg[100].fg = s->secretcset[sSBOMB];
    secret_dlg[100].d2 = s->secretflag[sSBOMB];
    
    for(int32_t i=0; i<3; i++)
    {
        secret_dlg[101+i].d1 = s->secretcombo[sBRANG+i];
        secret_dlg[101+i].fg = s->secretcset[sBRANG+i];
        secret_dlg[101+i].d2 = s->secretflag[sBRANG+i];
    }
    
    for(int32_t i=0; i<2; i++)
    {
        secret_dlg[104+i].d1 = s->secretcombo[sWANDMAGIC+i];
        secret_dlg[104+i].fg = s->secretcset[sWANDMAGIC+i];
        secret_dlg[104+i].d2 = s->secretflag[sWANDMAGIC+i];
    }
    
    for(int32_t i=0; i<8; i++)
    {
        secret_dlg[106+i].d1 = s->secretcombo[sSWORD+i];
        secret_dlg[106+i].fg = s->secretcset[sSWORD+i];
        secret_dlg[106+i].d2 = s->secretflag[sSWORD+i];
    }
    
    secret_dlg[114].d1 = s->secretcombo[sSTAIRS];
    secret_dlg[114].fg = s->secretcset[sSTAIRS];
    secret_dlg[114].d2 = s->secretflag[sSTAIRS];
    
    secret_dlg[115].d1 = s->secretcombo[sREFFIREBALL];
    secret_dlg[115].fg = s->secretcset[sREFFIREBALL];
    secret_dlg[115].d2 = s->secretflag[sREFFIREBALL];
    
    for(int32_t i=0; i<4; i++)
    {
        secret_dlg[116+i].d1 = s->secretcombo[sHOOKSHOT+i];
        secret_dlg[116+i].fg = s->secretcset[sHOOKSHOT+i];
        secret_dlg[116+i].d2 = s->secretflag[sHOOKSHOT+i];
    }
    
    for(int32_t i=0; i<16; i++)
    {
        secret_dlg[120+i].d1 = s->secretcombo[sSECRET01+i];
        secret_dlg[120+i].fg = s->secretcset[sSECRET01+i];
        secret_dlg[120+i].d2 = s->secretflag[sSECRET01+i];
    }
	
	//Sec->Next doesn't have a combo/cset value associated
	secret_dlg[137].d1 = 0;
	secret_dlg[137].fg = 0;
	secret_dlg[137].d2 = s->secretflag[sSECNEXT];
    
	large_dialog(secret_dlg,1.75);
	
	for(int32_t q = 0; secret_dlg[q].proc != NULL; ++q)
	{
		if(secret_dlg[q].proc == jwin_frame_proc)
			secret_dlg[q].w = secret_dlg[q].h = 36;
	}
    
    go();
    
    if(do_zqdialog(secret_dlg,3) == 2)
    {
        saved = false;
        s->secretcombo[sBCANDLE] = secret_dlg[92].d1;
        s->secretcset[sBCANDLE] = secret_dlg[92].fg;
        s->secretflag[sBCANDLE] = secret_dlg[92].d2;
        
        s->secretcombo[sRCANDLE] = secret_dlg[93].d1;
        s->secretcset[sRCANDLE] = secret_dlg[93].fg;
        s->secretflag[sRCANDLE] = secret_dlg[93].d2;
        
        s->secretcombo[sWANDFIRE] = secret_dlg[94].d1;
        s->secretcset[sWANDFIRE] = secret_dlg[94].fg;
        s->secretflag[sWANDFIRE] = secret_dlg[94].d2;
        
        s->secretcombo[sDIVINEFIRE] = secret_dlg[95].d1;
        s->secretcset[sDIVINEFIRE] = secret_dlg[95].fg;
        s->secretflag[sDIVINEFIRE] = secret_dlg[95].d2;
        
        s->secretcombo[sARROW] = secret_dlg[96].d1;
        s->secretcset[sARROW] = secret_dlg[96].fg;
        s->secretflag[sARROW] = secret_dlg[96].d2;
        
        s->secretcombo[sSARROW] = secret_dlg[97].d1;
        s->secretcset[sSARROW] = secret_dlg[97].fg;
        s->secretflag[sSARROW] = secret_dlg[97].d2;
        
        s->secretcombo[sGARROW] = secret_dlg[98].d1;
        s->secretcset[sGARROW] = secret_dlg[98].fg;
        s->secretflag[sGARROW] = secret_dlg[98].d2;
        
        s->secretcombo[sBOMB] = secret_dlg[99].d1;
        s->secretcset[sBOMB] = secret_dlg[99].fg;
        s->secretflag[sBOMB] = secret_dlg[99].d2;
        
        s->secretcombo[sSBOMB] = secret_dlg[100].d1;
        s->secretcset[sSBOMB] = secret_dlg[100].fg;
        s->secretflag[sSBOMB] = secret_dlg[100].d2;
        
        for(int32_t i=0; i<3; i++)
        {
            s->secretcombo[sBRANG+i] = secret_dlg[101+i].d1;
            s->secretcset[sBRANG+i] = secret_dlg[101+i].fg;
            s->secretflag[sBRANG+i] = secret_dlg[101+i].d2;
        }
        
        for(int32_t i=0; i<2; i++)
        {
            s->secretcombo[sWANDMAGIC+i] = secret_dlg[104+i].d1;
            s->secretcset[sWANDMAGIC+i] = secret_dlg[104+i].fg;
            s->secretflag[sWANDMAGIC+i] = secret_dlg[104+i].d2;
        }
        
        for(int32_t i=0; i<8; i++)
        {
            s->secretcombo[sSWORD+i] = secret_dlg[106+i].d1;
            s->secretcset[sSWORD+i] = secret_dlg[106+i].fg;
            s->secretflag[sSWORD+i] = secret_dlg[106+i].d2;
        }
        
        s->secretcombo[sSTAIRS] = secret_dlg[114].d1;
        s->secretcset[sSTAIRS] = secret_dlg[114].fg;
        s->secretflag[sSTAIRS] = secret_dlg[114].d2;
        
        s->secretcombo[sREFFIREBALL] = secret_dlg[115].d1;
        s->secretcset[sREFFIREBALL] = secret_dlg[115].fg;
        s->secretflag[sREFFIREBALL] = secret_dlg[115].d2;
        
        for(int32_t i=0; i<4; i++)
        {
            s->secretcombo[sHOOKSHOT+i] = secret_dlg[116+i].d1;
            s->secretcset[sHOOKSHOT+i] = secret_dlg[116+i].fg;
            s->secretflag[sHOOKSHOT+i] = secret_dlg[116+i].d2;
        }
        
        for(int32_t i=0; i<16; i++)
        {
            s->secretcombo[sSECRET01+i] = secret_dlg[120+i].d1;
            s->secretcset[sSECRET01+i] = secret_dlg[120+i].fg;
            s->secretflag[sSECRET01+i] = secret_dlg[120+i].d2;
        }
		s->secretflag[sSECNEXT] = secret_dlg[137].d2;
        
    }
    
    comeback();
    return D_O_K;
}

static DIALOG under_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     72,   60,   176+1,120+1,vc(14),  vc(1),  0,       D_EXIT,     0,             0, NULL, NULL, NULL },
    { jwin_text_proc,    115,  83,   20,   20,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "Current", NULL, NULL },
    { d_comboframe_proc, 122,  92,   20,   20,   0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      124,  94,   16,   16,   0,       0,      0,       D_NOCLICK,  0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,    184,  83,   20,   20,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "New", NULL, NULL },
    { d_comboframe_proc, 182,  92,   20,   20,   0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      184,  94,   16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,  90,   124,  61,   21,   vc(14),  vc(1),  's',     D_EXIT,     0,             0, (void *) "&Set", NULL, NULL },
    { jwin_button_proc,  170,  124,  61,   21,   vc(14),  vc(1),  'c',     D_EXIT,     0,             0, (void *) "&Cancel", NULL, NULL },
    { jwin_button_proc,  90,   152,  61,   21,   vc(14),  vc(1),  'a',     D_EXIT,     0,             0, (void *) "Set &All", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          KEY_F1,        0, (void *) onHelp, NULL, NULL },
    { d_timer_proc,      0,    0,    0,    0,    0,       0,      0,       0,          0,             0,         NULL, NULL, NULL },
    { NULL,              0,    0,    0,    0,    0,       0,      0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t onUnderCombo()
{
	char titlebuf[64];
	sprintf(titlebuf, "Under Combo (Layer %d)", CurrentLayer);
	under_dlg[0].dp = titlebuf;
	under_dlg[0].dp2 = get_zc_font(font_lfont);
	mapscr* scr;
	if(CurrentLayer==0)
	{
		scr=Map.CurrScr();
	}
	else
	{
		auto map=Map.CurrScr()->layermap[CurrentLayer-1]-1;
		auto screen=Map.CurrScr()->layerscreen[CurrentLayer-1];
		scr = Map.AbsoluteScr(map,screen);
	}
	if(!scr) return D_O_K;
	
	under_dlg[3].d1=scr->undercombo;
	under_dlg[3].fg=scr->undercset;
	
	under_dlg[6].d1=Combo;
	under_dlg[6].fg=CSet;
	
	large_dialog(under_dlg);
	// Doesn't place "New" and "Current" text too well
	under_dlg[1].x=342;
	under_dlg[4].x=438;
	
	int32_t ret = do_zqdialog(under_dlg,-1);
	
	if(ret==7)
	{
		saved=false;
		scr->undercombo = under_dlg[6].d1;
		scr->undercset = under_dlg[6].fg;
	}
	
	if(ret==9 && jwin_alert("Confirm Overwrite","Set all Under Combos","on this map?",NULL,"&Yes","&No",'y','n',get_zc_font(font_lfont))==1)
	{
		saved=false;
		
		for(int32_t i=0; i<128; i++)
		{
			Map.Scr(i)->undercombo = under_dlg[6].d1;
			Map.Scr(i)->undercset = under_dlg[6].fg;
		}
	}
	
	return D_O_K;
}

static DIALOG list_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_list_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  163,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

/*
  typedef struct item_struct {
  char *s;
  int32_t i;
  } item_struct;
  */
item_struct bii[MAXITEMS+1];
int32_t bii_cnt=-1;

void build_bii_list(bool usenone)
{
	int32_t start=bii_cnt=0;
    
    if(usenone)
    {
        bii[0].s = (char *)"(None)";
        bii[0].i = -2;
        bii_cnt=start=1;
    }
    
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        bii[bii_cnt].s = item_string[i];
        bii[bii_cnt].i = i;
        ++bii_cnt;
    }
    
    for(int32_t i=start; i<bii_cnt-1; i++)
    {
        for(int32_t j=i+1; j<bii_cnt; j++)
        {
            if(stricmp(bii[i].s,bii[j].s)>0 && strcmp(bii[j].s,""))
            {
                zc_swap(bii[i],bii[j]);
            }
        }
    }
}


const char *itemlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = bii_cnt;
        return NULL;
    }
    
    return bii[index].s;
}
const char *itemlist_num(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = bii_cnt;
        return NULL;
    }
	static char biin_buf[64+6];
	if(bii[index].i < 0)
		return bii[index].s;
    sprintf(biin_buf, "%s (%03d)", bii[index].s, bii[index].i);
    return biin_buf;
}

// disable items on dmaps stuff
int32_t DI[MAXITEMS];
int32_t nDI;

void initDI(int32_t index)
{
    int32_t j=0;
    
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        int32_t index1=bii[i].i; // true index of item in dmap's DI list
        
        if(DMaps[index].disableditems[index1])
        {
            DI[j]=i;
            j++;
        }
    }
    
    nDI=j;
    
    for(int32_t i=j; i<MAXITEMS; i++) DI[j]=0;
    
    return;
}

void insertDI(int32_t id, int32_t index)
{
    int32_t trueid=bii[id].i;
    DMaps[index].disableditems[trueid] |= 1; //bit set
    initDI(index);
    return;
}

void deleteDI(int32_t id, int32_t index)
{
    int32_t i=DI[id];
    int32_t trueid=bii[i].i;
    DMaps[index].disableditems[trueid] &= (~1); // bit clear
    initDI(index);
    return;
}

const char *DIlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = nDI;
        return NULL;
    }
    
    int32_t i=DI[index];
    return bii[i].s;
    
}

weapon_struct biw[MAXWPNS];
int32_t biw_cnt=-1;

void build_biw_list()
{
    int32_t start=biw_cnt=0;
    
    for(int32_t i=start; i<MAXWPNS; i++)
    {
        biw[biw_cnt].s = (char *)weapon_string[i];
        biw[biw_cnt].i = i;
        ++biw_cnt;
    }
    
    for(int32_t i=start; i<biw_cnt-1; i++)
    {
        for(int32_t j=i+1; j<biw_cnt; j++)
            if(stricmp(biw[i].s,biw[j].s)>0 && strcmp(biw[j].s,""))
                zc_swap(biw[i],biw[j]);
    }
}

const char *weaponlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = biw_cnt;
        return NULL;
    }
    
    return biw[index].s;
}
const char *weaponlist_num(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = biw_cnt;
        return NULL;
    }
	static char biwn_buf[64+6];
	if(biw[index].i < 0)
		return biw[index].s;
    sprintf(biwn_buf, "%s (%03d)", biw[index].s, biw[index].i);
    return biwn_buf;
}
int32_t writeoneweapon(PACKFILE *f, int32_t index)
{
    dword section_version=V_WEAPONS;
    dword section_cversion=CV_WEAPONS;
    int32_t zversion = ZELDA_VERSION;
    int32_t zbuild = VERSION_BUILD;
    int32_t iid = biw[index].i;
    al_trace("Writing Weapon Sprite .zwpnspr file for weapon id: %d\n", iid);
  
    //section version info
    if(!p_iputl(zversion,f))
    {
	    return 0;
    }
    if(!p_iputl(zbuild,f))
    {
	    return 0;
    }
    if(!p_iputw(section_version,f))
    {
	    return 0;
    }
    
    if(!p_iputw(section_cversion,f))
    {
	    return 0;
    }
    
    //weapon string
	
    if(!pfwrite((char *)weapon_string[iid], 64, f))
    {
        return 0;
    }
            
    if(!p_putc(wpnsbuf[iid].misc,f))
    {
        return 0;
    }
            
    if(!p_putc(wpnsbuf[iid].csets,f))
    {
        return 0;
    }
            
    if(!p_putc(wpnsbuf[iid].frames,f))
    {
        return 0;
    }
            
    if(!p_putc(wpnsbuf[iid].speed,f))
    {
        return 0;
    }
            
    if(!p_putc(wpnsbuf[iid].type,f))
    {
        return 0;
    }
	    
    if(!p_iputw(wpnsbuf[iid].script,f))
    {
        return 0;
    }
	    
    //2.55 starts here
    if(!p_iputl(wpnsbuf[iid].tile,f))
    {
        return 0;
    }

    return 1;
}


int32_t readoneweapon(PACKFILE *f, int32_t index)
{
	dword section_version = 0;
	dword section_cversion = 0;
	int32_t zversion = 0;
	int32_t zbuild = 0;
	wpndata tempwpnspr;
	memset(&tempwpnspr, 0, sizeof(wpndata));
     
   
	//char dmapstring[64]={0};
	//section version info
	if(!p_igetl(&zversion,f))
	{
		return 0;
	}
	if(!p_igetl(&zbuild,f))
	{
		return 0;
	}
	if(!p_igetw(&section_version,f))
	{
		return 0;
	}
	if(!p_igetw(&section_cversion,f))
	{
		return 0;
	}
	al_trace("readoneweapon section_version: %d\n", section_version);
	al_trace("readoneweapon section_cversion: %d\n", section_cversion);

	if ( zversion > ZELDA_VERSION )
	{
		al_trace("Cannot read .zwpnspr packfile made in ZC version (%x) in this version of ZC (%x)\n", zversion, ZELDA_VERSION);
		return 0;
	}
	
	else if ( ( section_version > V_WEAPONS ) || ( section_version == V_WEAPONS && section_cversion < CV_WEAPONS ) )
	{
		al_trace("Cannot read .zwpnspr packfile made using V_WEAPONS (%d) subversion (%d)\n", section_version, section_cversion);
		return 0;
		
	}
	else
	{
		al_trace("Reading a .zwpnspr packfile made in ZC Version: %x, Build: %d\n", zversion, zbuild);
	}
	
	char tmp_wpn_name[64];
	memset(tmp_wpn_name,0,64);
	if(!pfread(&tmp_wpn_name, 64, f))
	{
		return 0;
	}
	
    word oldtile = 0;
    if(section_version < 8)
	    if(!p_igetw(&oldtile,f))
            return 0;
            
    if(!p_getc(&tempwpnspr.misc,f))
    {
        return 0;
    }
            
    if(!p_getc(&tempwpnspr.csets,f))
    {
        return 0;
    }
            
    if(!p_getc(&tempwpnspr.frames,f))
    {
        return 0;
    }
            
    if(!p_getc(&tempwpnspr.speed,f))
    {
        return 0;
    }
    
    if(!p_getc(&tempwpnspr.type,f))
    {
        return 0;
    }
	
	if(!p_igetw(&tempwpnspr.script,f))
    {
        return 0;
    }

	//2.55 starts here
	if ( zversion >= 0x255 )
	{
		if  ( section_version >= 7 )
		{
			if(!p_igetl(&tempwpnspr.tile,f))
			{
				return 0;
			}
		}
	}
	if ( zversion < 0x255 ) 
	{
		tempwpnspr.tile = oldtile;
	}
	::memcpy( &(wpnsbuf[biw[index].i]),&tempwpnspr, sizeof(wpndata));
	::memcpy(weapon_string[biw[index].i], tmp_wpn_name, 64);
       
	return 1;
}

static int32_t seldata_copy;
static void (*seldata_paste_func)(int32_t, int32_t);

void seldata_rclick_func(int32_t index, int32_t x, int32_t y)
{
	NewMenu rcmenu {
		{ "&Copy", [&](){seldata_copy = index;} },
		{ "Paste", "&v", [&]()
			{
				seldata_paste_func(seldata_copy, index);
				saved = false;
			}, 0, seldata_copy < 0 },
	};
	rcmenu.pop(x, y);
}

int32_t select_data(const char *prompt,int32_t index,const char *(proc)(int32_t,int32_t*), FONT *title_font, void (*copyFunc)(int32_t, int32_t))
{
    if(proc==NULL)
        return -1;
    
    list_dlg[0].dp=(void *)prompt;
    list_dlg[0].dp2=title_font;
    list_dlg[2].d1=index;
    ListData select_list(proc, &font);
    list_dlg[2].dp=(void *) &select_list;
    
    large_dialog(list_dlg);
    
    seldata_copy=-1;
    seldata_paste_func=copyFunc;
    if(copyFunc)
    {
        list_dlg[2].flags|=D_USER<<1;
        list_dlg[2].dp3=(void*)seldata_rclick_func;
    }
    else
    {
        list_dlg[2].flags&=~(D_USER<<1);
        list_dlg[2].dp3=0;
    }
    
    int32_t ret=do_zqdialog(list_dlg,2);
    
    if(ret==0||ret==4)
    {
        position_mouse_z(0);
        return -1;
    }
    
    return list_dlg[2].d1;
}

int32_t select_data(const char *prompt,int32_t index,const char *(proc)(int32_t,int32_t*), const char *b1, const char *b2, FONT *title_font, void (*copyFunc)(int32_t, int32_t))
{
    if(proc==NULL)
        return -1;
        
    list_dlg[0].dp=(void *)prompt;
    list_dlg[0].dp2=title_font;
    list_dlg[2].d1=index;
    ListData select_data_list(proc, &font);
    list_dlg[2].dp=(void *) &select_data_list;
    list_dlg[3].dp=(void *)b1;
    list_dlg[4].dp=(void *)b2;
    
    large_dialog(list_dlg);
    
    seldata_copy=-1;
    seldata_paste_func=copyFunc;
    if(copyFunc)
    {
        list_dlg[2].flags|=D_USER<<1;
        list_dlg[2].dp3=(void*)seldata_rclick_func;
    }
    else
    {
        list_dlg[2].flags&=~(D_USER<<1);
        list_dlg[2].dp3=0;
    }
    
    int32_t ret = do_zqdialog(list_dlg,2);
    list_dlg[3].dp=(void *) "OK";
    list_dlg[4].dp=(void *) "Cancel";
    
    if(ret==0||ret==4)
    {
        position_mouse_z(0);
        return -1;
    }
    
    position_mouse_z(0);
    return list_dlg[2].d1;
}

static char sfx_str_buf[42];

const char *sfxlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,WAV_COUNT-1);
        sprintf(sfx_str_buf,"%d: %s",index, index ? sfx_string[index] : "(None)");
        return sfx_str_buf;
    }
    
    *list_size=WAV_COUNT;
    return NULL;
}

static ListData sfx_list(sfxlist, &font);

const char *screenscriptdroplist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = biscreens_cnt;
        return NULL;
    }
    
    return biscreens[index].first.c_str();
}

//droplist like the dialog proc, naming scheme for this stuff is awful...
static ListData screenscript_list(screenscriptdroplist, &a4fonts[font_pfont]);

int32_t onScreenScript()
{
    call_screendata_dialog(7);
    return D_O_K;
}

int32_t onScrData()
{
	restore_mouse();
	call_screendata_dialog();
	return D_O_K;
}

const char *roomslist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        if(index>=MAXROOMTYPES)
            index=MAXROOMTYPES-1;
            
        return roomtype_string[index];
    }
    
    *list_size=MAXROOMTYPES;
    return NULL;
}

static char number_str_buf[MIDI_TRACK_BUFFER_SIZE];
int32_t number_list_size=1;
bool number_list_zero=false;

const char *numberlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,number_list_size-1);
        sprintf(number_str_buf,"%d",index+(number_list_zero?0:1));
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

static char dmap_str_buf[37];
int32_t dmap_list_size=MAXDMAPS;
bool dmap_list_zero=true;

const char *dmaplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,dmap_list_size-1);
        sprintf(dmap_str_buf,"%3d-%s",index+(dmap_list_zero?0:1), DMaps[index].name);
        return dmap_str_buf;
    }
    
    *list_size=dmap_list_size;
    return NULL;
}

char *hexnumlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,number_list_size-1);
        sprintf(number_str_buf,"%X",index+(number_list_zero?0:1));
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

const char *maplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXMAPS-1);
        sprintf(number_str_buf,"%d",index+1);
        return number_str_buf;
    }
    
    *list_size=MAXMAPS;
    return NULL;
}

const char *gotomaplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,map_count-1);
        sprintf(number_str_buf,"%d",index+1);
        return number_str_buf;
    }
    
    *list_size = map_count;
    return NULL;
}

const char *midilist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    
    {
        bound(index,0,MAXCUSTOMMIDIS_ZQ-1);
        return midi_string[index];
    }
    
    *list_size=MAXCUSTOMMIDIS_ZQ;
    return NULL;
}

const char *custommidilist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXCUSTOMMIDIS_ZQ-1);
        sprintf(number_str_buf,"%3d - %s",index+(number_list_zero?0:1),customtunes[index].data?customtunes[index].title:"(Empty)");
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

const char *enhancedmusiclist(int32_t index, int32_t *list_size)
{
    index=index; //this is here to prevent unused parameter warnings
    list_size=list_size; //this is here to prevent unused parameter warnings
    /*if(index>=0)
    {
      bound(index,0,MAXMUSIC-1);
      sprintf(number_str_buf,"%3d - %s",index+(number_list_zero?0:1),enhancedMusic[index].filename[0]?enhancedMusic[index].title:"(Empty)" );
      return number_str_buf;
    }
    *list_size=number_list_size;*/
    return NULL;
}


const char *levelnumlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,0xFFF);
        sprintf(number_str_buf,"%.3X - %s",index,palnames[index]);
        return number_str_buf;
    }
    
    *list_size=MAXLEVELS;
    return NULL;
}

static char shop_str_buf[40];
int32_t shop_list_size=1;

const char *shoplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,shop_list_size-1);
        sprintf(shop_str_buf,"%3d:  %s",index,QMisc.shop[index].name);
        return shop_str_buf;
    }
    
    *list_size=shop_list_size;
    return NULL;
}

static char bottle_str_buf[40];
int32_t bottle_list_size=1;

const char *bottlelist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,bottle_list_size-1);
		sprintf(bottle_str_buf,"%2d:  %s",index+1,QMisc.bottle_types[index].name);
        return bottle_str_buf;
    }
    
    *list_size=bottle_list_size;
    return NULL;
}

static char bottleshop_str_buf[40];
int32_t bottleshop_list_size=1;

const char *bottleshoplist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,bottleshop_list_size-1);
		sprintf(bottleshop_str_buf,"%3d:  %s",index,QMisc.bottle_shop_types[index].name);
        return bottleshop_str_buf;
    }
    
    *list_size=bottleshop_list_size;
    return NULL;
}

static char info_str_buf[40];
int32_t info_list_size=1;

const char *infolist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,info_list_size-1);
        sprintf(info_str_buf,"%3d:  %s",index,QMisc.info[index].name);
        return info_str_buf;
    }
    
    *list_size=info_list_size;
    return NULL;
}

bool mapcount_will_affect_layers(word newmapcount)
{
	for(int32_t i=0; i<(newmapcount)*MAPSCRS; i++)
	{
		mapscr *layerchecker=&TheMaps[i];
		
		for(int32_t j=0; j<6; j++)
		{
			if(layerchecker->layermap[j]>(newmapcount))
			{
				return true;
			}
		}
	}
	return false;
}

void update_map_count(word newmapcount)
{
	if(map_count == newmapcount) return;
	saved = false;
	setMapCount2(newmapcount);
	//Prevent the nine 'last mapscreen' buttons from pointing to invlid locations
	//if the user reduces the mapcount. -Z ( 23rd September, 2019 )
	for ( int32_t q = 0; q < 9; q++ )
	{
		map_page[q].map = ( map_page[q].map > newmapcount-1 ) ? newmapcount-1 : map_page[q].map;
	}
	for(int32_t i=0; i<(newmapcount)*MAPSCRS; i++)
	{
		fix_layers(&TheMaps[i], false);
	}

    refresh(rMAP+rSCRMAP+rMENU);
}

int32_t onGotoMap()
{
    int32_t ret = select_data("Goto Map",Map.getCurrMap(),gotomaplist,get_zc_font(font_lfont));
    
    if(ret >= 0)
    {
        int32_t m=Map.getCurrMap();
        Map.setCurrMap(ret);
    }
    
    refresh(rALL);
    return D_O_K;
}

int32_t onFlags()
{
    restore_mouse();
    int32_t ret=select_cflag("Select Combo Flag",Flag);
    position_mouse_z(0);
    
    if(ret>=0)
    {
        Flag=ret;
        refresh(rMENU);
        doflags();
    }
    
    return D_O_K;
}

static DIALOG usedcombo_list_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Combos Used", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_textbox_proc,       72-12,   60+4,   176+24+1,  92+4,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,      NULL, NULL, NULL },
    { jwin_button_proc,     130,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};



int32_t onUsedCombos()
{
    restore_mouse();
    usedcombo_list_dlg[0].dp2=get_zc_font(font_lfont);
    
    int32_t usedcombos[7][300][2];
    char combolist_text[65536];
    char temptext[80];
    
    int32_t drawmap=Map.getCurrMap();
    int32_t drawscr=Map.getCurrScr();
    int32_t counter[7];
    
    for(int32_t layer=0; layer<7; ++layer)
    {
        counter[layer]=0;
        
        if(layer==0)
        {
            drawmap=Map.getCurrMap();
            drawscr=Map.getCurrScr();
        }
        else
        {
			drawmap=Map.CurrScr()->layermap[layer-1]-1;
			drawscr=Map.CurrScr()->layerscreen[layer-1];
        }
		mapscr* draw_mapscr = Map.AbsoluteScr(drawmap, drawscr);
		if(!draw_mapscr) continue;
        
        usedcombos[layer][0][0]=draw_mapscr->data[0];
        usedcombos[layer][0][1]=1;
        counter[layer]=1;
        
        for(int32_t i=1; i<176; ++i)
        {
            bool used=false;
            
            for(int32_t j=0; j<counter[layer]; ++j)
            {
                if(usedcombos[layer][j][0]==draw_mapscr->data[i])
                {
                    ++usedcombos[layer][j][1];
                    used=true;
                    break;
                }
            }
            
            if(!used)
            {
                usedcombos[layer][counter[layer]][0]=draw_mapscr->data[i];
                usedcombos[layer][counter[layer]][1]=1;
                ++counter[layer];
            }
        }
        
        for(int32_t i=0; i<counter[layer]-1; i++)
        {
            for(int32_t j=i+1; j<counter[layer]; j++)
            {
                if(usedcombos[layer][i][0]>usedcombos[layer][j][0])
                {
                    zc_swap(usedcombos[layer][i][0],usedcombos[layer][j][0]);
                    zc_swap(usedcombos[layer][i][1],usedcombos[layer][j][1]);
                }
            }
        }
    }
    
    sprintf(combolist_text, " ");
    
    for(int32_t layer=0; layer<7; ++layer)
    {
        if(counter[layer]>0)
        {
            if(layer>0)
            {
                strcat(combolist_text, "\n");
            }
            
            sprintf(temptext, "Combos on layer %d\n-----------------\n", layer);
            strcat(combolist_text, temptext);
            
            for(int32_t i=0; i<counter[layer]; i++)
            {
                if((i<counter[layer]-1) && (((usedcombos[layer][i][1]==usedcombos[layer][i+1][1]&&(usedcombos[layer][i][0]+1==usedcombos[layer][i+1][0])) && ((i==0) || ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0])))))))
                {
                    sprintf(temptext, "%5d ", usedcombos[layer][i][0]);
                    strcat(combolist_text, temptext);
                }
                else if(((i>0) && (((usedcombos[layer][i][1]==usedcombos[layer][i-1][1])&&((usedcombos[layer][i][0]-1==usedcombos[layer][i-1][0]))) && ((i==counter[layer]-1) || ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0])))))))
                {
                    sprintf(temptext, "- %5d (%3d)\n", usedcombos[layer][i][0],usedcombos[layer][i][1]);
                    strcat(combolist_text, temptext);
                }
                else if(((i==0) && ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0]))))||
                        ((i==counter[layer]-1) && ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0]))))||
                        ((i>0) && (i<counter[layer]-1) && ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0]))) && ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0])))))
                {
                    sprintf(temptext, "    %5d     (%3d)\n", usedcombos[layer][i][0],usedcombos[layer][i][1]);
                    strcat(combolist_text, temptext);
                }
            }
        }
    }
    
    strcat(combolist_text, "\n");
    usedcombo_list_dlg[2].dp=combolist_text;
    usedcombo_list_dlg[2].d2=0;
    
    large_dialog(usedcombo_list_dlg);
        
    do_zqdialog(usedcombo_list_dlg,2);
    position_mouse_z(0);
    return D_O_K;
}

int32_t onItem()
{
    restore_mouse();
    int32_t exit_status;
    int32_t current_item=Map.CurrScr()->hasitem != 0 ? Map.CurrScr()->item : -1;
    
	ItemListerDialog(current_item,true).show();
	if(current_item != lister_sel_val)
	{
		if(lister_sel_val>=0)
		{
			saved = false;
			Map.CurrScr()->item = lister_sel_val;
			Map.CurrScr()->hasitem = true;
		}
		else
		{
			saved = false;
			Map.CurrScr()->hasitem = false;
		}
	}

    refresh(rMAP+rMENU);
    return D_O_K;
}

void call_room_dlg(mapscr* scr);
int32_t onRoom()
{
	restore_mouse();
	auto* scr = Map.CurrScr();
	call_room_dlg(scr);
	
	refresh(rMAP+rMENU);
	return D_O_K;
}

int32_t onEndString()
{
    int32_t ret=select_data("Select Ending String",QMisc.endstring,msgslist,get_zc_font(font_lfont));
    
    if(ret>=0)
    {
        saved=false;
        QMisc.endstring=msglistcache[ret];
    }

    refresh(rMENU);
    return D_O_K;
}

static ListData levelnum_list(levelnumlist, &font);

static DIALOG screen_pal_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      60-12,   40,   200-16,  96,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Palette", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_droplist_proc, 72-12,   84+4,   161,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &levelnum_list, NULL, NULL },
    { jwin_button_proc,   70,   111,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   150,  111,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,       72-12,   60+4,  168,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Note: This does not affect how the", NULL, NULL },
    { jwin_text_proc,       72-12,   72+4,  168,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "room will be displayed in-game!", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};
//  return list_dlg[2].d1;

int32_t onScreenPalette()
{
	restore_mouse();
	screen_pal_dlg[0].dp2=get_zc_font(font_lfont);
	auto oldcol = screen_pal_dlg[2].d1 = Map.getcolor();
	
	large_dialog(screen_pal_dlg);
	auto old_valid = Map.CurrScr()->valid;
	pause_dlg_tint(true);
	zq_set_screen_never_freeze(true);
	while(true)
	{
		auto ret = do_zqdialog(screen_pal_dlg,2);
		if(ret == 2)
		{
			Map.setcolor(screen_pal_dlg[2].d1);
			refresh(rALL);
		}
		else
		{
			if(ret == 3)
			{
				if(screen_pal_dlg[2].d1 != oldcol)
					saved=false;
				Map.setcolor(screen_pal_dlg[2].d1);
			}
			else
			{
				Map.setcolor(oldcol);
				Map.CurrScr()->valid = old_valid;
			}
			refresh(rALL);
			break;
		}
	}
	pause_dlg_tint(false);
	zq_set_screen_never_freeze(false);
	
	rebuild_trans_table();
	
	return D_O_K;
}

int32_t onDecScrPal()
{
	if(DisableLPalShortcuts)
	{
		lpal_dsa();
		return D_O_K;
	}
    restore_mouse();
    int32_t c=Map.getcolor();
    c+=511;
    c=c%512;
    Map.setcolor(c);
    refresh(rALL);
	saved = false;
    return D_O_K;
}

int32_t onIncScrPal()
{
	if(DisableLPalShortcuts)
	{
		lpal_dsa();
		return D_O_K;
	}
    restore_mouse();
    int32_t c=Map.getcolor();
    c+=1;
    c=c%512;
    Map.setcolor(c);
    refresh(rALL);
	saved = false;
    return D_O_K;
}

int32_t PalWrap(int32_t kX, int32_t const kLowerBound, int32_t const kUpperBound)
{
    int32_t range_size = kUpperBound - kLowerBound + 1;

    if (kX < kLowerBound)
        kX += range_size * ((kLowerBound - kX) / range_size + 1);

    return kLowerBound + (kX - kLowerBound) % range_size;
}

int32_t onDecScrPal16()
{
	if(DisableLPalShortcuts)
	{
		lpal_dsa();
		return D_O_K;
	}
    restore_mouse(); 
    int32_t c=Map.getcolor();
      
    c = PalWrap( ( c-0x10 ), 0, 511 );
     
    Map.setcolor(c);
    refresh(rALL);
	saved = false;
    return D_O_K;
}

int32_t onIncScrPal16()
{
	if(DisableLPalShortcuts)
	{
		lpal_dsa();
		return D_O_K;
	}
    restore_mouse();
    int32_t c=Map.getcolor();
      	    
    c = PalWrap( ( c+0x10 ), 0, 511 );
    Map.setcolor(c);
    refresh(rALL);
	saved = false;
    return D_O_K;
}

int32_t onZoomIn()
{
	change_mapscr_zoom(-1);
    return D_O_K;
}

int32_t onZoomOut()
{
	change_mapscr_zoom(1);
    return D_O_K;
}

int32_t d_ndroplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_droplist_proc(msg,d,c);
    
    // The only place this proc is used is in the info type editor.
    // If it's ever used anywhere else, this will probably need to be changed.
    // Maybe add a flag for it or something.
    int32_t msgID=msg_at_pos(d->d1);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        textprintf_ex(screen,font,d->x - 48,d->y + 4,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%5d",msgID);
    }
    
    return ret;
}

int32_t d_idroplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_droplist_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        int32_t tile = bii[d->d1].i >=0 ? itemsbuf[bii[d->d1].i].tile : 0;
        int32_t cset = bii[d->d1].i >=0 ? itemsbuf[bii[d->d1].i].csets&15 : 0;
        int32_t x = d->x + d->w + 4;
        int32_t y = d->y - 8;
        int32_t w = 32;
        int32_t h = 32;
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        BITMAP *bigbmp = create_bitmap_ex(8,w,h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
            if(tile)
                overtile16(buf, tile,0,0,cset,0);
                
            stretch_blit(buf, bigbmp, 0,0, 16, 16, 0, 0, w, h);
            destroy_bitmap(buf);
            jwin_draw_frame(screen,x,y,w+4,h+4,FR_DEEP);
            blit(bigbmp,screen,0,0,x+2,y+2,w,h);
            destroy_bitmap(bigbmp);
        }
        
    }
    
    return ret;
}

int32_t d_nidroplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = d_idroplist_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        textprintf_ex(screen,font,d->x - 48,d->y + 4,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%5d",bii[d->d1].i);
    }
    
    return ret;
}

// Triforce pieces
static byte triframe_points[9*4] =
{
    0,2,2,0,  2,0,4,2,  0,2,4,2,  1,1,3,1,  2,0,2,2,
    1,1,1,2,  1,1,2,2,  3,1,3,2,  3,1,2,2
};

int32_t d_tri_frame_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    if(msg==MSG_DRAW)
    {
        int32_t x[5],y[3];
        
        x[0]=d->x;
        x[1]=d->x+(d->w>>2);
        x[2]=d->x+(d->w>>1);
        x[3]=d->x+(d->w>>1)+(d->w>>2);
        x[4]=d->x+d->w;
        y[0]=d->y;
        y[1]=d->y+(d->h>>1);
        y[2]=d->y+d->h;
        
        byte *p = triframe_points;
        
        for(int32_t i=0; i<9; i++)
        {
            line(screen,x[*p],y[*(p+1)],x[*(p+2)],y[*(p+3)],d->fg);
            p+=4;
        }
    }
    
    return D_O_K;
}

int32_t d_tri_edit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    jwin_button_proc(msg,d,c);
    
    if(msg==MSG_CLICK)
    {
        int32_t v = getnumber("Piece Number",d->d1);
        
        if(v>=0)
        {
            bound(v,1,8);
            
            if(v!=d->d1)
            {
                DIALOG *tp = d - d->d2;
                
                for(int32_t i=0; i<8; i++)
                {
                    if(tp->d1==v)
                    {
                        tp->d1 = d->d1;
                        ((char*)(tp->dp))[0] = d->d1+'0';
                        jwin_button_proc(MSG_DRAW,tp,0);
                    }
                    
                    ++tp;
                }
                
                d->d1 = v;
                ((char*)(d->dp))[0] = v+'0';
            }
        }
        
        d->flags = 0;
        jwin_button_proc(MSG_DRAW,d,0);
    }
    
    return D_O_K;
}

static DIALOG tp_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     56,   32,   208,  160,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Triforce Pieces", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_tri_frame_proc,  64,   56,   192,    96,   jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,          0,             0,       NULL, NULL, NULL },
    // 3
    { d_tri_edit_proc,   138,  82,   17,   17,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { d_tri_edit_proc,   166,  82,   17,   17,   vc(14),  vc(1),  0,       0,          0,             1, (void *) "2", NULL, NULL },
    { d_tri_edit_proc,   90,   130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             2, (void *) "3", NULL, NULL },
    { d_tri_edit_proc,   214,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             3, (void *) "4", NULL, NULL },
    // 7
    { d_tri_edit_proc,   138,  110,  17,   17,   vc(14),  vc(1),  0,       0,          0,             4, (void *) "5", NULL, NULL },
    { d_tri_edit_proc,   118,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             5, (void *) "6", NULL, NULL },
    { d_tri_edit_proc,   166,  110,  17,   17,   vc(14),  vc(1),  0,       0,          0,             6, (void *) "7", NULL, NULL },
    { d_tri_edit_proc,   186,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             7, (void *) "8", NULL, NULL },
    // 11
    { jwin_button_proc,     90,   166,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  166,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t onTriPieces()
{
    tp_dlg[0].dp2=get_zc_font(font_lfont);
    char temptext[8][2];
    
    for(int32_t i=0; i<8; i++)
    {
        tp_dlg[i+3].d1 = QMisc.triforce[i];
        //    ((char*)(tp_dlg[i+3].dp))[0] = QMisc.triforce[i]+'0';
        sprintf(temptext[i], "%d", QMisc.triforce[i]);
        tp_dlg[i+3].dp=temptext[i];
    }
    
    large_dialog(tp_dlg);
        
    if(do_zqdialog(tp_dlg,-1) == 11)
    {
        saved=false;
        
        for(int32_t i=0; i<8; i++)
            QMisc.triforce[i] = tp_dlg[i+3].d1;
    }
    
    return D_O_K;
}

int32_t d_maptile_proc(int32_t msg,DIALOG *d,int32_t c);
bool small_dmap=false;

static DIALOG dmapmaps_dlg[] =
{

    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 4,    18,   313,  217,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Custom DMap Map Styles", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_button_proc,     93,   208,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     168,  208,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_ctext2_proc,      160,  38,    0,   8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimaps", NULL, NULL },
    { d_ctext2_proc,      112,  46,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Without Map", NULL, NULL },
    { d_ctext2_proc,      208,  46,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "With Map", NULL, NULL },
    
    { d_ctext2_proc,      162,  110,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Large Maps", NULL, NULL },
    { d_ctext2_proc,      80,   118,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Without Map", NULL, NULL },
    { d_ctext2_proc,      240,  118,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "With Map", NULL, NULL },
    // 5
    { d_maptile_proc,    72,   54,   80,   48,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    168,  54,   80,   48,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    8,    126,  144,  80,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    168,  126,  144,  80,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    // 11
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t d_hexedit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    return jwin_hexedit_proc(msg,d,c);
}

void drawgrid(BITMAP *dest,int32_t x,int32_t y,int32_t grid,int32_t fg,int32_t bg,int32_t div)
{
    if(div!=-1)
        rectfill(dest,x-1,y-1,x+63,y+3,div);
        
    for(int32_t dx=0; dx<64; dx+=8)
    {
        if(grid&0x80)
            rectfill(dest,x+dx,y,x+dx+6,y+2,fg);
        else if(bg!=-1)
            rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
            
        grid<<=1;
    }
}

void drawovergrid(BITMAP *dest,int32_t x,int32_t y,int32_t grid,int32_t color,int32_t div)
{
    if(div!=-1)
        rectfill(dest,x-1,y-1,x+63,y+3,div);
        
    for(int32_t dx=0; dx<64; dx+=4)
    {
        rectfill(dest,x+dx,y,x+dx+2,y+2,color);
        grid<<=1;
    }
}

void drawgrid(BITMAP *dest,int32_t x,int32_t y,int32_t w, int32_t h, int32_t tw, int32_t th, int32_t *grid,int32_t fg,int32_t bg,int32_t div)
{
    //these are here to bypass compiler warnings about unused arguments
    w=w;
    tw=tw;
    th=th;
    
    rectfill(dest,x,y,x+(8*8),y+(1*4),div);
    
    for(int32_t dy=0; dy<h; dy++)
    {
        for(int32_t dx=0; dx<64; dx+=8)
        {
            if(grid[0]&0x80)
                rectfill(dest,x+dx,y,x+dx+6,y+2,fg);
            else
                rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
                
            grid[0]<<=1;
        }
    }
}

void drawgrid_s(BITMAP *dest,int32_t x,int32_t y,int32_t grid,int32_t fg,int32_t bg,int32_t div)
{
    rectfill(dest,x-1,y-1,x+63,y+3,div);
    
    for(int32_t dx=0; dx<64; dx+=8)
    {
        rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
        
        if(grid&0x80)
            rectfill(dest,x+dx+2,y,x+dx+4,y+2,fg);
            
        grid<<=1;
    }
}

void drawdmap(int32_t dmap)
{
    int32_t c;
    zcolors mc=QMisc.colors;
    
    switch((DMaps[dmap].type&dmfTYPE))
    {
    case dmDNGN:
    case dmCAVE:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        // overworld_map_tile overrides the NES minimap. dungeon_map_tile does not.
        else for(int32_t y=1; y<33; y+=4)
                drawgrid(dmapbmp_small,0,y,DMaps[dmap].grid[y>>2], DMaps[dmap].flags&dmfMINIMAPCOLORFIX ? mc.cave_fg : mc.dngn_fg, -1, -1);
                
        c=DMaps[dmap].compass;
        //  rectfill(dmapbmp,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,dvc(2*4));
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(4));
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(10));
        break;
        
    case dmOVERW:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        else if(!mc.overworld_map_tile)
            for(int32_t y=1; y<33; y+=4)
                drawovergrid(dmapbmp_small,1,y,DMaps[dmap].grid[y>>2],mc.overw_bg,vc(0));
                
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*4+1,(c>>4)*4+1,(c&15)*4+3,(c>>4)*4+3,vc(10));
        break;
        
    case dmBSOVERW:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        else if(!mc.overworld_map_tile)
            for(int32_t y=1; y<33; y+=4)
                //    drawgrid_s(dmapbmp,1,y,DMaps[dmap].grid[y>>2],dvc(2*4),dvc(2*3),dvc(3+4));
                drawgrid_s(dmapbmp_small,0,y,DMaps[dmap].grid[y>>2],mc.bs_goal,mc.bs_dk,vc(14));
                
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(10));
        break;
    }
}

void drawdmap_screen(int32_t x, int32_t y, int32_t w, int32_t h, int32_t dmap)
{
    BITMAP *tempbmp = create_bitmap_ex(8,w,h);
    clear_to_color(tempbmp, vc(0));
    zcolors mc=QMisc.colors;
    
//  rectfill(tempbmp,x,y,x+w-1,y+h-1,vc(0));

    if(DMaps[dmap].minimap_2_tile)
    {
        draw_block(tempbmp,0,0,DMaps[dmap].minimap_2_tile,DMaps[dmap].minimap_2_cset,5,3);
    }
    else if(((DMaps[dmap].type&dmfTYPE)==dmDNGN || (DMaps[dmap].type&dmfTYPE)==dmCAVE) && mc.dungeon_map_tile)
    {
        draw_block(tempbmp,0,0,mc.dungeon_map_tile,mc.dungeon_map_cset,5,3);
    }
    else if(((DMaps[dmap].type&dmfTYPE)==dmOVERW || (DMaps[dmap].type&dmfTYPE)==dmBSOVERW) && mc.overworld_map_tile)
    {
        draw_block(tempbmp,0,0,mc.overworld_map_tile,mc.overworld_map_cset,5,3);
    }
    
    masked_blit(dmapbmp_small,tempbmp,0,0,8,7,65,33);
    
    blit(tempbmp,screen,0,0,x,y,w,h);
    destroy_bitmap(tempbmp);
    
}

int32_t d_dmaplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    if(msg==MSG_DRAW)
    {
        int32_t dmap = d->d1;
        int32_t xy[6] = {44,92,128,100,128,110};
        //int32_t *xy = (int32_t*)(d->dp3);
        float temp_scale = 1.5;
        
        drawdmap(dmap);
        
        if(xy[0]>-1000&&xy[1]>-1000)
        {
            int32_t x = d->x+int32_t((xy[0]-2)*temp_scale);
            int32_t y = d->y+int32_t((xy[1]-2)*temp_scale);
            int32_t w = 84;
            int32_t h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]>-1000&&xy[3]>-1000)
        {
            textprintf_ex(screen,get_zc_font(font_lfont_l),d->x+int32_t((xy[2])*temp_scale),d->y+int32_t((xy[3])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %-3d",DMaps[d->d1].map+1);
        }
        
        if(xy[4]>-1000&&xy[5]>-1000)
        {
            textprintf_ex(screen,get_zc_font(font_lfont_l),d->x+int32_t((xy[4])*temp_scale),d->y+int32_t((xy[5])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level: %-3d",DMaps[d->d1].level);
        }
    }
    
    return jwin_list_proc(msg,d,c);
}

int32_t d_dropdmaplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    if(msg==MSG_DRAW)
    {
        int32_t dmap = d->d1;
        int32_t *xy = (int32_t*)(d->dp3);
        float temp_scale = 1.5;
        
        drawdmap(dmap);
        
        if(xy[0]>-1000&&xy[1]>-1000)
        {
            int32_t x = d->x+int32_t((xy[0]-2)*temp_scale);
            int32_t y = d->y+int32_t((xy[1]-2)*temp_scale);
            int32_t w = 84;
            int32_t h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]>-1000&&xy[3]>-1000)
        {
            textprintf_ex(screen,get_zc_font(font_lfont_l),d->x+int32_t((xy[2])*temp_scale),d->y+int32_t((xy[3])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %-3d",DMaps[d->d1].map+1);
        }
        
        if(xy[4]>-1000&&xy[5]>-1000)
        {
            textprintf_ex(screen,get_zc_font(font_lfont_l),d->x+int32_t((xy[4])*temp_scale),d->y+int32_t((xy[5])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level: %-3d",DMaps[d->d1].level);
        }
    }
    
    return jwin_droplist_proc(msg,d,c);
}

void drawxmap(ALLEGRO_BITMAP* dest, int32_t themap, int32_t xoff, bool large, int dx, int dy)
{
	ALLEGRO_STATE old_state;
	al_store_state(&old_state, ALLEGRO_STATE_TARGET_BITMAP);

	al_set_target_bitmap(dest);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));

	int32_t cols = (large ? 8 : 16);
	int32_t col_width = large ? 22 : 11;
	int32_t dot_width = (large ? 6 : 4);
	int32_t dot_offset = (large ? 7 : 3);
	int32_t l = 10;

	for (int32_t y = 0; y < 8; y++)
	{
		// Users might have set the dmap to a map that has since been deleted.
		if (themap >= Map.getMapCount())
			break;

		for (int32_t x = 0; x < cols; x++)
		{
			if (x + xoff < 0 || x + xoff > 15)
				continue;
			mapscr* scr = &TheMaps[themap * MAPSCRS + y * 16 + x + (large ? xoff : 0)];
			if (!(scr->valid & mVALID))
				continue;
			al_draw_filled_rectangle(dx + (x * col_width), dy + (y * l), dx + (x * col_width + col_width), dy + ((y * l) + l), real_lc1(scr->color));

			al_draw_filled_rectangle(dx + (x * col_width + dot_offset), dy + (y * l + 3), dx + (x * col_width + dot_offset + dot_width), dy + (y * l + l - 3), real_lc2(scr->color));
		}
	}

	al_restore_state(&old_state);
}

const char *dmapscriptdroplist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = bidmaps_cnt;
        return NULL;
    }
    
    return bidmaps[index].first.c_str();
}


//droplist like the dialog proc, naming scheme for this stuff is awful...
static ListData dmapscript_list(dmapscriptdroplist, &a4fonts[font_pfont]);

//int32_t selectdmapxy[6] = {90,142,164,150,164,160};
int32_t selectdmapxy[6] = {44,92,128,100,128,110};

static ListData dmap_list(dmaplist, &font);

static DIALOG selectdmap_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,   72-44,   56-30,   176+88+1,  120+74+1,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select DMap", NULL, NULL },
    { d_timer_proc,        0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_dmaplist_proc,    46,   50,   64+72+88+1,   60+24+1+2,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &dmap_list, NULL, selectdmapxy },
    { jwin_button_proc,   90,   152+44,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,  170,  152+44,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_DEL, (void *) close_dlg, NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_C, (void*)close_dlg, NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_V, (void*)close_dlg, NULL, NULL },
    { NULL,                0,    0,    0,    0,    0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static dmap copiedDMap;
static byte dmapcopied = 0;

int32_t writesomedmaps(PACKFILE *f, int32_t first, int32_t last, int32_t max)
{
    
    dword section_version=V_DMAPS;
    dword section_cversion=CV_DMAPS;
	int32_t zversion = ZELDA_VERSION;
	int32_t zbuild = VERSION_BUILD;
	
	if(!p_iputl(V_ZDMAP,f))
	{
		return 0;
	}
  
    //section version info
	if(!p_iputl(zversion,f))
	{
		return 0;
	}
	if(!p_iputl(zbuild,f))
	{
		return 0;
	}
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
    
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	//max possible at this time
	if(!p_iputl(max,f))
	{
		new_return(4);
	}
	//first id written
	if(!p_iputl(first,f))
	{
		new_return(5);
	}
	//last id written
	if(!p_iputl(last,f))
	{
		new_return(6);
	}
	int32_t count = last-first;
	//number written
	if(!p_iputl(count,f))
	{
		new_return(7);
	}
	
   
        for ( int32_t i = first; i <= last; ++i )
	{
		if ( i > max ) break;
	
            if(!p_putc(DMaps[i].map,f))
            {
                new_return(8);
            }
            
            if(!p_iputw(DMaps[i].level,f))
            {
                new_return(9);
            }
            
            if(!p_putc(DMaps[i].xoff,f))
            {
                new_return(10);
            }
            
            if(!p_putc(DMaps[i].compass,f))
            {
                new_return(11);
            }
            
            if(!p_iputw(DMaps[i].color,f))
            {
                new_return(12);
            }
            
            if(!p_putc(DMaps[i].midi,f))
            {
                new_return(13);
            }
            
            if(!p_putc(DMaps[i].cont,f))
            {
                new_return(14);
            }
            
            if(!p_putc(DMaps[i].type,f))
            {
                new_return(15);
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_putc(DMaps[i].grid[j],f))
                {
                    new_return(16);
                }
            }
            
            //16
            if(!pfwrite(&DMaps[i].name,sizeof(DMaps[0].name),f))
            {
                new_return(17);
            }

			if(!p_putwstr(DMaps[i].title,f))
            {
                new_return(18);
            }
            
            if(!pfwrite(&DMaps[i].intro,sizeof(DMaps[0].intro),f))
            {
                new_return(19);
            }
            
            if(!p_iputl(DMaps[i].minimap_1_tile,f))
            {
                new_return(20);
            }
            
            if(!p_putc(DMaps[i].minimap_1_cset,f))
            {
                new_return(21);
            }
            
            if(!p_iputl(DMaps[i].minimap_2_tile,f))
            {
                new_return(22);
            }
            
            if(!p_putc(DMaps[i].minimap_2_cset,f))
            {
                new_return(23);
            }
            
            if(!p_iputl(DMaps[i].largemap_1_tile,f))
            {
                new_return(24);
            }
            
            if(!p_putc(DMaps[i].largemap_1_cset,f))
            {
                new_return(25);
            }
            
            if(!p_iputl(DMaps[i].largemap_2_tile,f))
            {
                new_return(26);
            }
            
            if(!p_putc(DMaps[i].largemap_2_cset,f))
            {
                new_return(27);
            }
            
            if(!pfwrite(&DMaps[i].tmusic,sizeof(DMaps[0].tmusic),f))
            {
                new_return(28);
            }
            
            if(!p_putc(DMaps[i].tmusictrack,f))
            {
                new_return(29);
            }
            
            if(!p_putc(DMaps[i].active_subscreen,f))
            {
                new_return(30);
            }
            
            if(!p_putc(DMaps[i].passive_subscreen,f))
            {
                new_return(31);
            }
            
            byte disabled[32];
            memset(disabled,0,32);
            
            for(int32_t j=0; j<MAXITEMS; j++)
            {
                if(DMaps[i].disableditems[j])
                {
                    disabled[j/8] |= (1 << (j%8));
                }
            }
            
            if(!pfwrite(disabled,32,f))
            {
                new_return(32);
            }
            
            if(!p_iputl(DMaps[i].flags,f))
            {
                new_return(33);
            }
	    if(!p_putc(DMaps[i].sideview,f))
            {
                new_return(30);
            }
	    if(!p_iputw(DMaps[i].script,f))
            {
                new_return(31);
            }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		if(!p_iputl(DMaps[i].initD[q],f))
	        {
			new_return(32);
		}
		    
	    }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		    for ( int32_t w = 0; w < 65; w++ )
		    {
			if (!p_putc(DMaps[i].initD_label[q][w],f))
			{
				new_return(33);
			}
		}
	    }
		if(!p_iputw(DMaps[i].active_sub_script,f))
		{
			new_return(34);
		}
		if(!p_iputw(DMaps[i].passive_sub_script,f))
		{
			new_return(35);
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			if(!p_iputl(DMaps[i].sub_initD[q],f))
			{
				new_return(36);
			}
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			for(int32_t w = 0; w < 65; ++w)
			{
				if(!p_putc(DMaps[i].sub_initD_label[q][w],f))
				{
					new_return(37);
				}
			}
		}
		if(!p_iputw(DMaps[i].onmap_script,f))
		{
			new_return(35);
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			if(!p_iputl(DMaps[i].onmap_initD[q],f))
			{
				new_return(36);
			}
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			for(int32_t w = 0; w < 65; ++w)
			{
				if(!p_putc(DMaps[i].onmap_initD_label[q][w],f))
				{
					new_return(37);
				}
			}
		}
		if (!p_iputw(DMaps[i].mirrorDMap, f))
		{
			new_return(38);
		}
		if (!p_iputl(DMaps[i].tmusic_loop_start, f))
		{
			new_return(39);
		}
		if (!p_iputl(DMaps[i].tmusic_loop_end, f))
		{
			new_return(40);
		}
		if (!p_iputl(DMaps[i].tmusic_xfade_in, f))
		{
			new_return(41);
		}
		if (!p_iputl(DMaps[i].tmusic_xfade_out, f))
		{
			new_return(42);
		}
	}

	return 1;
}


int32_t readsomedmaps(PACKFILE *f)
{
	dword section_version = 0;
	dword section_cversion = 0;
	int32_t zversion = 0;
	int32_t zbuild = 0;
	dmap tempdmap{};
	
	int32_t first = 0, last = 0, max = 0, count = 0;
	int32_t datatype_version = 0;
   
	//char dmapstring[64]={0};
	//section version info
	if(!p_igetl(&datatype_version,f))
	{
		return 0;
	}
	if ( datatype_version < 0 )
	{
		if(!p_igetl(&zversion,f))
		{
			return 0;
		}
	}
	else
	{
		zversion = datatype_version;
	}
	if(!p_igetl(&zbuild,f))
	{
		return 0;
	}
	
	if(!p_igetw(&section_version,f))
	{
		return 0;
	}
    
	if(!p_igetw(&section_cversion,f))
	{
		return 0;
	}
	if ( datatype_version < 0 )
	{
		if(!p_igetl(&max,f))
		{
			return 0;
		}
		if(!p_igetl(&first,f))
		{
			return 0;
		}
		if(!p_igetl(&last,f))
		{
			return 0;
		}
		if(!p_igetl(&count,f))
		{
			return 0;
		} 
	}
	else
	{
		first = 0;
		last = 0;
		count = 1;
		max = 255;
	}
	
	
	
	
	al_trace("readsomedmaps section_version: %d\n", section_version);
	al_trace("readsomedmaps section_cversion: %d\n", section_cversion);
    
	if ( zversion > ZELDA_VERSION )
	{
		al_trace("Cannot read .zdmap packfile made in ZC version (%x) in this version of ZC (%x)\n", zversion, ZELDA_VERSION);
		return 0;
	}
	else if (( section_version > V_DMAPS ) || ( section_version == V_DMAPS && section_cversion > CV_DMAPS ) ) 
	{
		al_trace("Cannot read .zdmap packfile made using V_DMAPS (%d) subversion (%d)\n", section_version, section_cversion);
		return 0;
	}
	else
	{
		al_trace("Reading a .zdmap packfile made in ZC Version: %x, Build: %d\n", zversion, zbuild);
	}
	//if(!pfread(&dmapstring, 64, f))
	//{
	//	return 0;
	//}
    
    
   
		for ( int32_t i = first; i <= last; ++i )
		{
		    if(!p_getc(&tempdmap.map,f))
		    {
			return 0;
		    }
		    
		    if(!p_igetw(&tempdmap.level,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.xoff,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.compass,f))
		    {
			return 0;
		    }
		    
		    if(!p_igetw(&tempdmap.color,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.midi,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.cont,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.type,f))
		    {
			return 0;
		    }
		    
		    for(int32_t j=0; j<8; j++)
		    {
			if(!p_getc(&tempdmap.grid[j],f))
			{
			    return 0;
			}
		    }
		    
		    //16
		    if(!pfread(&tempdmap.name,sizeof(DMaps[0].name),f))
		    {
			return 0;
		    }

			if (section_version<20)
			{
				char title[22];
				if (!p_getstr(title, sizeof(title) - 1, f))
				{
					return 0;
				}
				tempdmap.title.assign(title);
			}
			else
			{
				if (!p_getwstr(&tempdmap.title, f))
				{
					return 0;
				}
			}
		    
		    if(!pfread(&tempdmap.intro,sizeof(DMaps[0].intro),f))
		    {
			return 0;
		    }
		    
		    if(!p_igetl(&tempdmap.minimap_1_tile,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.minimap_1_cset,f))
		    {
			return 0;
		    }
		    
		    if(!p_igetl(&tempdmap.minimap_2_tile,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.minimap_2_cset,f))
		    {
			return 0;
		    }
		    
		    if(!p_igetl(&tempdmap.largemap_1_tile,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.largemap_1_cset,f))
		    {
			return 0;
		    }
		    
		    if(!p_igetl(&tempdmap.largemap_2_tile,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.largemap_2_cset,f))
		    {
			return 0;
		    }
		    
		    if(!pfread(&tempdmap.tmusic,sizeof(DMaps[0].tmusic),f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.tmusictrack,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.active_subscreen,f))
		    {
			return 0;
		    }
		    
		    if(!p_getc(&tempdmap.passive_subscreen,f))
		    {
			return 0;
		    }
		    
		    byte disabled[32];
		    memset(disabled,0,32);
		    
		    if(!pfread(&disabled, 32, f)) return 0;
		    
		    for(int32_t j=0; j<MAXITEMS; j++)
		    {
			if(disabled[j/8] & (1 << (j%8))) tempdmap.disableditems[j]=1;
			else tempdmap.disableditems[j]=0;
		    }
		    
		    
		    if(!p_igetl(&tempdmap.flags,f))
		    {
			return 0;
		    }
			if ( zversion >= 0x255 )
			{
				if  ( section_version >= 14 )
				{
				    //2.55 starts here
				    if(!p_getc(&tempdmap.sideview,f))
				    {
					return 0;
				    }
				    if(!p_igetw(&tempdmap.script,f))
				    {
					return 0;
				    }
				    for ( int32_t q = 0; q < 8; q++ )
				    {
					if(!p_igetl(&tempdmap.initD[q],f))
					{
						return 0;
				    }
					    
				    }
				    for ( int32_t q = 0; q < 8; q++ )
				    {
					    for ( int32_t w = 0; w < 65; w++ )
					    {
						if (!p_getc(&tempdmap.initD_label[q][w],f))
						{
							return 0;
						}
					}
				    }
					if(!p_igetw(&tempdmap.active_sub_script,f))
					{
						return 0;
					}
					if(!p_igetw(&tempdmap.passive_sub_script,f))
					{
						return 0;
					}
					for(int32_t q = 0; q < 8; ++q)
					{
						if(!p_igetl(&tempdmap.sub_initD[q],f))
						{
							return 0;
						}
					}	
					for(int32_t q = 0; q < 8; ++q)
					{
						for(int32_t w = 0; w < 65; ++w)
						{
							if(!p_getc(&tempdmap.sub_initD_label[q][w],f))
							{
								return 0;
							}
						}
					}
					if(!p_igetw(&tempdmap.onmap_script,f))
					{
						return 0;
					}
					for(int32_t q = 0; q < 8; ++q)
					{
						if(!p_igetl(&tempdmap.onmap_initD[q],f))
						{
							return 0;
						}
					}	
					for(int32_t q = 0; q < 8; ++q)
					{
						for(int32_t w = 0; w < 65; ++w)
						{
							if(!p_getc(&tempdmap.onmap_initD_label[q][w],f))
							{
								return 0;
							}
						}
					}
					if (!p_igetw(&tempdmap.mirrorDMap, f))
					{
						return 0;
					}
					if (!p_igetl(&tempdmap.tmusic_loop_start, f))
					{
						return 0;
					}
					if (!p_igetl(&tempdmap.tmusic_loop_end, f))
					{
						return 0;
					}
					if (!p_igetl(&tempdmap.tmusic_xfade_in, f))
					{
						return 0;
					}
					if (!p_igetl(&tempdmap.tmusic_xfade_out, f))
					{
						return 0;
					}
				}
			}
			DMaps[i].clear();
			DMaps[i] = tempdmap;
	    }
       
	return 1;
}



int32_t writeonedmap(PACKFILE *f, int32_t i)
{
    
    dword section_version=V_DMAPS;
    dword section_cversion=CV_DMAPS;
	int32_t zversion = ZELDA_VERSION;
	int32_t zbuild = VERSION_BUILD;
	
  
    //section version info
	if(!p_iputl(V_ZDMAP,f))
	{
		return 0;
	}
	if(!p_iputl(zversion,f))
	{
		return 0;
	}
	if(!p_iputl(zbuild,f))
	{
		return 0;
	}
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
    
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
    
   
        
            if(!p_putc(DMaps[i].map,f))
            {
                new_return(6);
            }
            
            if(!p_iputw(DMaps[i].level,f))
            {
                new_return(7);
            }
            
            if(!p_putc(DMaps[i].xoff,f))
            {
                new_return(8);
            }
            
            if(!p_putc(DMaps[i].compass,f))
            {
                new_return(9);
            }
            
            if(!p_iputw(DMaps[i].color,f))
            {
                new_return(10);
            }
            
            if(!p_putc(DMaps[i].midi,f))
            {
                new_return(11);
            }
            
            if(!p_putc(DMaps[i].cont,f))
            {
                new_return(12);
            }
            
            if(!p_putc(DMaps[i].type,f))
            {
                new_return(13);
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_putc(DMaps[i].grid[j],f))
                {
                    new_return(14);
                }
            }
            
            //16
            if(!pfwrite(&DMaps[i].name,sizeof(DMaps[0].name),f))
            {
                new_return(15);
            }
            
            if(!pfwrite(&DMaps[i].title,sizeof(DMaps[0].title),f))
            {
                new_return(16);
            }
            
            if(!pfwrite(&DMaps[i].intro,sizeof(DMaps[0].intro),f))
            {
                new_return(17);
            }
            
            if(!p_iputl(DMaps[i].minimap_1_tile,f))
            {
                new_return(18);
            }
            
            if(!p_putc(DMaps[i].minimap_1_cset,f))
            {
                new_return(19);
            }
            
            if(!p_iputl(DMaps[i].minimap_2_tile,f))
            {
                new_return(20);
            }
            
            if(!p_putc(DMaps[i].minimap_2_cset,f))
            {
                new_return(21);
            }
            
            if(!p_iputl(DMaps[i].largemap_1_tile,f))
            {
                new_return(22);
            }
            
            if(!p_putc(DMaps[i].largemap_1_cset,f))
            {
                new_return(23);
            }
            
            if(!p_iputl(DMaps[i].largemap_2_tile,f))
            {
                new_return(24);
            }
            
            if(!p_putc(DMaps[i].largemap_2_cset,f))
            {
                new_return(25);
            }
            
            if(!pfwrite(&DMaps[i].tmusic,sizeof(DMaps[0].tmusic),f))
            {
                new_return(26);
            }
            
            if(!p_putc(DMaps[i].tmusictrack,f))
            {
                new_return(25);
            }
            
            if(!p_putc(DMaps[i].active_subscreen,f))
            {
                new_return(26);
            }
            
            if(!p_putc(DMaps[i].passive_subscreen,f))
            {
                new_return(27);
            }
            
            byte disabled[32];
            memset(disabled,0,32);
            
            for(int32_t j=0; j<MAXITEMS; j++)
            {
                if(DMaps[i].disableditems[j])
                {
                    disabled[j/8] |= (1 << (j%8));
                }
            }
            
            if(!pfwrite(disabled,32,f))
            {
                new_return(28);
            }
            
            if(!p_iputl(DMaps[i].flags,f))
            {
                new_return(29);
            }
	    if(!p_putc(DMaps[i].sideview,f))
            {
                new_return(30);
            }
	    if(!p_iputw(DMaps[i].script,f))
            {
                new_return(31);
            }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		if(!p_iputl(DMaps[i].initD[q],f))
	        {
			new_return(32);
		}
		    
	    }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		    for ( int32_t w = 0; w < 65; w++ )
		    {
			if (!p_putc(DMaps[i].initD_label[q][w],f))
			{
				new_return(33);
			}
		}
	    }
		if(!p_iputw(DMaps[i].active_sub_script,f))
		{
			new_return(34);
		}
		if(!p_iputw(DMaps[i].passive_sub_script,f))
		{
			new_return(35);
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			if(!p_iputl(DMaps[i].sub_initD[q],f))
			{
				new_return(36);
			}
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			for(int32_t w = 0; w < 65; ++w)
			{
				if(!p_putc(DMaps[i].sub_initD_label[q][w],f))
				{
					new_return(37);
				}
			}
		}
		if(!p_iputw(DMaps[i].onmap_script,f))
		{
			new_return(35);
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			if(!p_iputl(DMaps[i].onmap_initD[q],f))
			{
				new_return(36);
			}
		}
		for(int32_t q = 0; q < 8; ++q)
		{
			for(int32_t w = 0; w < 65; ++w)
			{
				if(!p_putc(DMaps[i].onmap_initD_label[q][w],f))
				{
					new_return(37);
				}
			}
		}
		if (!p_iputw(DMaps[i].mirrorDMap, f))
		{
			new_return(38);
		}
		if (!p_iputl(DMaps[i].tmusic_loop_start, f))
		{
			new_return(39);
		}
		if (!p_iputl(DMaps[i].tmusic_loop_end, f))
		{
			new_return(40);
		}
		if (!p_iputl(DMaps[i].tmusic_xfade_in, f))
		{
			new_return(41);
		}
		if (!p_iputl(DMaps[i].tmusic_xfade_out, f))
		{
			new_return(42);
		}

	return 1;
}


int32_t readonedmap(PACKFILE *f, int32_t index)
{
	dword section_version = 0;
	dword section_cversion = 0;
	int32_t zversion = 0;
	int32_t zbuild = 0;
	dmap tempdmap{};
	int32_t datatype_version = 0;
	int32_t first = 0;
	int32_t last = 0;
	int32_t max = 0;
	int32_t count = 0;
   
	//char dmapstring[64]={0};
	//section version info
	if(!p_igetl(&datatype_version,f))
	{
		return 0;
	}
	if ( datatype_version < 0 )
	{
		if(!p_igetl(&zversion,f))
		{
			return 0;
		}
	}
	else
	{
		zversion = datatype_version;
	}
	if(!p_igetl(&zbuild,f))
	{
		return 0;
	}
	
	if(!p_igetw(&section_version,f))
	{
		return 0;
	}
    
	if(!p_igetw(&section_cversion,f))
	{
		return 0;
	}
	al_trace("readonedmap section_version: %d\n", section_version);
	al_trace("readonedmap section_cversion: %d\n", section_cversion);
    
	
	if ( datatype_version < 0 )
	{
		if(!p_igetl(&max,f))
		{
			return 0;
		}
		if(!p_igetl(&first,f))
		{
			return 0;
		}
		if(!p_igetl(&last,f))
		{
			return 0;
		}
		if(!p_igetl(&count,f))
		{
			return 0;
		}
	}
	if ( zversion > ZELDA_VERSION )
	{
		al_trace("Cannot read .zdmap packfile made in ZC version (%x) in this version of ZC (%x)\n", zversion, ZELDA_VERSION);
		return 0;
	}
	else if (( section_version > V_DMAPS ) || ( section_version == V_DMAPS && section_cversion > CV_DMAPS ) ) 
	{
		al_trace("Cannot read .zdmap packfile made using V_DMAPS (%d) subversion (%d)\n", section_version, section_cversion);
		return 0;
	}
	else
	{
		al_trace("Reading a .zdmap packfile made in ZC Version: %x, Build: %d\n", zversion, zbuild);
	}
	//if(!pfread(&dmapstring, 64, f))
	//{
	//	return 0;
	//}
    
    
   
        
            if(!p_getc(&tempdmap.map,f))
            {
                return 0;
            }
            
            if(!p_igetw(&tempdmap.level,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.xoff,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.compass,f))
            {
                return 0;
            }
            
            if(!p_igetw(&tempdmap.color,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.midi,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.cont,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.type,f))
            {
                return 0;
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_getc(&tempdmap.grid[j],f))
                {
                    return 0;
		}
            }
            
            //16
            if(!pfread(&tempdmap.name,sizeof(DMaps[0].name),f))
            {
                return 0;
            }

			if (section_version<20)
			{
				char title[22];
				if (!p_getstr(title, sizeof(title) - 1, f))
				{
					return 0;
				}
				tempdmap.title.assign(title);
			}
			else
			{
				if (!p_getwstr(&tempdmap.title, f))
				{
					return 0;
				}
			}

            if(!pfread(&tempdmap.title,sizeof(DMaps[0].title),f))
            {
                return 0;
            }
            
            if(!pfread(&tempdmap.intro,sizeof(DMaps[0].intro),f))
            {
                return 0;
            }
            
            if(!p_igetl(&tempdmap.minimap_1_tile,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.minimap_1_cset,f))
            {
                return 0;
            }
            
            if(!p_igetl(&tempdmap.minimap_2_tile,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.minimap_2_cset,f))
            {
                return 0;
            }
            
            if(!p_igetl(&tempdmap.largemap_1_tile,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.largemap_1_cset,f))
            {
                return 0;
            }
            
            if(!p_igetl(&tempdmap.largemap_2_tile,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.largemap_2_cset,f))
            {
                return 0;
            }
            
            if(!pfread(&tempdmap.tmusic,sizeof(DMaps[0].tmusic),f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.tmusictrack,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.active_subscreen,f))
            {
                return 0;
            }
            
            if(!p_getc(&tempdmap.passive_subscreen,f))
            {
                return 0;
            }
            
            byte disabled[32];
	    memset(disabled,0,32);
            
            if(!pfread(&disabled, 32, f)) return 0;
            
            for(int32_t j=0; j<MAXITEMS; j++)
            {
                if(disabled[j/8] & (1 << (j%8))) tempdmap.disableditems[j]=1;
                else tempdmap.disableditems[j]=0;
            }
	    
            
            if(!p_igetl(&tempdmap.flags,f))
            {
                return 0;
            }
		if ( zversion >= 0x255 )
		{
			if  ( section_version >= 14 )
			{
			    //2.55 starts here
			    if(!p_getc(&tempdmap.sideview,f))
			    {
				return 0;
			    }
			    if(!p_igetw(&tempdmap.script,f))
			    {
				return 0;
			    }
			    for ( int32_t q = 0; q < 8; q++ )
			    {
				if(!p_igetl(&tempdmap.initD[q],f))
				{
					return 0;
			    }
				    
			    }
			    for ( int32_t q = 0; q < 8; q++ )
			    {
				    for ( int32_t w = 0; w < 65; w++ )
				    {
					if (!p_getc(&tempdmap.initD_label[q][w],f))
					{
						return 0;
					}
				}
			    }
				if(!p_igetw(&tempdmap.active_sub_script,f))
				{
					return 0;
				}
				if(!p_igetw(&tempdmap.passive_sub_script,f))
				{
					return 0;
				}
				for(int32_t q = 0; q < 8; ++q)
				{
					if(!p_igetl(&tempdmap.sub_initD[q],f))
					{
						return 0;
					}
				}	
				for(int32_t q = 0; q < 8; ++q)
				{
					for(int32_t w = 0; w < 65; ++w)
					{
						if(!p_getc(&tempdmap.sub_initD_label[q][w],f))
						{
							return 0;
						}
					}
				}
				if(!p_igetw(&tempdmap.onmap_script,f))
				{
					return 0;
				}
				for(int32_t q = 0; q < 8; ++q)
				{
					if(!p_igetl(&tempdmap.onmap_initD[q],f))
					{
						return 0;
					}
				}	
				for(int32_t q = 0; q < 8; ++q)
				{
					for(int32_t w = 0; w < 65; ++w)
					{
						if(!p_getc(&tempdmap.onmap_initD_label[q][w],f))
						{
							return 0;
						}
					}
				}
				if (!p_igetw(&tempdmap.mirrorDMap, f))
				{
					return 0;
				}
				if (!p_igetl(&tempdmap.tmusic_loop_start, f))
				{
					return 0;
				}
				if (!p_igetl(&tempdmap.tmusic_loop_end, f))
				{
					return 0;
				}
				if (!p_igetl(&tempdmap.tmusic_xfade_in, f))
				{
					return 0;
				}
				if (!p_igetl(&tempdmap.tmusic_xfade_out, f))
				{
					return 0;
				}
			}
		}
		DMaps[index] = tempdmap;
       
	return 1;
}

void dmap_rclick_func(int32_t index, int32_t x, int32_t y)
{
    if(((unsigned)index)>MAXDMAPS)
        return;
    
	NewMenu rcmenu {
		{ "&Copy", [&]()
			{
				copiedDMap = DMaps[index];
				dmapcopied = 1;
			} },
		{ "Paste", "&v", [&]()
			{
				DMaps[index] = copiedDMap;
				selectdmap_dlg[2].flags |= D_DIRTY;
				saved = false;
			}, 0, !dmapcopied },
		{ "&Save", [&]()
			{
				if(!prompt_for_new_file_compat("Save DMAP(.zdmap)", "zdmap", NULL,datapath,false))
					return;
				PACKFILE *f=pack_fopen_password(temppath,F_WRITE, "");
				if(!f) return;
				writesomedmaps(f,index, index, MAXDMAPS);
				pack_fclose(f);
			} },
		{ "&Load", [&]()
			{
				if(!prompt_for_existing_file_compat("Load DMAP(.zdmap)", "zdmap", NULL,datapath,false))
					return;
				PACKFILE *f=pack_fopen_password(temppath,F_READ, "");
				if(!f) return;
				
				if (!readonedmap(f,index))
				{
					al_trace("Could not read from .zdmap packfile %s\n", temppath);
					jwin_alert("ZDMAP File: Error","Could not load the specified DMap.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				}
				
				pack_fclose(f);
				selectdmap_dlg[2].flags |= D_DIRTY; //Causes the dialogie list to refresh, updating the item name.
				saved = false;
			} },
	};
	rcmenu.pop(x, y);
}


int32_t onDmaps()
{
    int32_t ret;
    char buf[40];
    dmapcopied = 0;
    dmap_list_size=MAXDMAPS;
    number_list_zero=true;
    selectdmap_dlg[0].dp2=get_zc_font(font_lfont);
    selectdmap_dlg[2].dp3 = (void *)&dmap_rclick_func;
    selectdmap_dlg[2].flags|=(D_USER<<1);
    
    large_dialog(selectdmap_dlg);
    
    
    
    ret=do_zqdialog(selectdmap_dlg,2);
    dmap* pSelectedDmap = 0;
    
    
    
    while(ret!=4&&ret!=0)
    {
        int32_t d=selectdmap_dlg[2].d1;
        
        if(ret==6) //copy
		{
			pSelectedDmap = &DMaps[d];
		}
		else if(ret==7 && pSelectedDmap != 0 ) //paste
		{
			if( pSelectedDmap != &DMaps[d] )
			{
				DMaps[d] = *pSelectedDmap;
				saved=false;
			}
		}
        else if(ret==5)
        {
            sprintf(buf,"Delete DMap %d?",d);
            
            if(jwin_alert("Confirm Delete",buf,NULL,NULL,"&Yes","&No",'y','n',get_zc_font(font_lfont))==1)
            {
                reset_dmap(d);
                saved=false;
            }
        }
        else
        {
			call_editdmap_dialog(d);
        }
        
        ret=do_zqdialog(selectdmap_dlg,2);
    }
    
    return D_O_K;
}

int32_t onMidis()
{
    stopMusic();
	MidiListerDialog().show();
    return D_O_K;
}

static DIALOG editmusic_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Music Specs", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    // 2
    { jwin_text_proc,       56,   94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Music:", NULL, NULL },
    { jwin_text_proc,       104,  94-16,   48,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   114,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       104,  114-4,  160,  16,   vc(12),  vc(1),  0,       0,          19,            0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   124-4+12,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Volume:", NULL, NULL },
    { jwin_edit_proc,       120,  124-4+12-4,  32,   16,   vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    // 8
    { jwin_check_proc,      176,  124+12-4,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Loop", NULL, NULL },
    // 9
    { jwin_button_proc,     50,   72-24,   57,   21,   vc(14),  vc(1),  'l',     D_EXIT,     0,             0, (void *) "&Load", NULL, NULL },
    { jwin_iconbutton_proc, 116,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     BTNICON_STOPSQUARE,    0, NULL, NULL, NULL },
    { jwin_iconbutton_proc, 156,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     BTNICON_ARROW_RIGHT,   0, NULL, NULL, NULL },
    { jwin_iconbutton_proc, 196,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     BTNICON_ARROW_RIGHT2,  0, NULL, NULL, NULL },
    { jwin_iconbutton_proc, 236,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     BTNICON_ARROW_RIGHT3,  0, NULL, NULL, NULL },
    // 14
    { jwin_text_proc,       56,   134+4+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Start:", NULL, NULL },
    { jwin_edit_proc,       112,  134+12,  32,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  134+12+4,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop Start:", NULL, NULL },
    { jwin_edit_proc,       240,  134+12,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  144+12+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop End:", NULL, NULL },
    { jwin_edit_proc,       240,  144+12+12-4,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    // 20
    { jwin_text_proc,       176,  94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Position:", NULL, NULL },
    { jwin_text_proc,       217,  94-16,   32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Length:", NULL, NULL },
    { jwin_text_proc,       216,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Time:", NULL, NULL },
    { jwin_text_proc,       104,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    // 26
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t d_musiclist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    return jwin_list_proc(msg,d,c);
}

static ListData enhancedmusic_list(enhancedmusiclist, &font);

static DIALOG selectmusic_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Enhanced Music", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_musiclist_proc,   31,   44,   164, (1+16)*8,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &enhancedmusic_list, NULL, NULL },
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  13,     D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_DEL, (void *) close_dlg, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

const char *warptypelist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        if(index>=MAXWARPTYPES)
            index=MAXWARPTYPES-1;
            
        return warptype_string[index];
    }
    
    *list_size=MAXWARPTYPES;
    //  *list_size=6;
    return NULL;
}

const char *warpeffectlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        if(index>=MAXWARPEFFECTS)
            index=MAXWARPEFFECTS-1;
            
        return warpeffect_string[index];
    }
    
    *list_size=MAXWARPEFFECTS;
    return NULL;
}

static int32_t warp1_list[] =
{
    2,3,4,5,6,7,8,9,10,11,12,13,53,54,63,67,-1
};

static int32_t warp2_list[] =
{
    17,18,19,20,21,22,23,24,25,26,27,28,55,56,64,68,-1
};

static int32_t warp3_list[] =
{
    29,30,31,32,33,34,35,36,37,38,39,40,57,58,65,69,-1
};

static int32_t warp4_list[] =
{
    41,42,43,44,45,46,47,48,49,50,51,52,59,60,66,70,-1
};

static TABPANEL warp_tabs[] =
{
    // (text)
    { (char *)"A",     D_SELECTED, warp1_list, 0, NULL },
    { (char *)"B",     0,          warp2_list, 0, NULL },
    { (char *)"C",     0,          warp3_list, 0, NULL },
    { (char *)"D",     0,          warp4_list, 0, NULL },
    { NULL,            0,          NULL,       0, NULL }
};

int32_t onTileWarpIndex(int32_t index)
{
    int32_t i=-1;
    
    while(warp_tabs[++i].text != NULL)
        warp_tabs[i].flags = (i==index ? D_SELECTED : 0);
        
    onTileWarp();
    return D_O_K;
}

static char warpr_buf[10];
const char *warprlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,3);
        sprintf(warpr_buf,"%c",index+0x41);
        return warpr_buf;
    }
    
    *list_size=4;
    return NULL;
}

int32_t d_wflag_proc(int32_t msg,DIALOG *d,int32_t c);

static ListData warp_dlg_list(warptypelist, &font);
static ListData warp_ret_list(warprlist, &font);

int32_t d_warpdestscrsel_proc(int32_t msg,DIALOG *d,int32_t c)
{
	if(msg == MSG_START)
	{
		d->d1 = -1; //cached val
		d->d2 = -1; //cached dmap
		d->fg = 0; //cached 'force_16'
	}
	char* buf = (char*)d->dp;
	vector<DIALOG*>* dlgs = (vector<DIALOG*>*)d->dp2;
	int* dmap_ptr = (int*) d->dp3;
	if(!(buf && dmap_ptr))
		return D_O_K;
	bool is_overworld = ((DMaps[*dmap_ptr].type&dmfTYPE)==dmOVERW);
	int scrw = is_overworld ? 16 : 8, scrh = 9;
	const int max = 0x87;
	int bufval = zc_xtoi(buf);
	int val = vbound(bufval,0,max);
	auto& dm = DMaps[*dmap_ptr];
	auto val_offset = dm.xoff < 0 ? -dm.xoff : 0;
	bool force_16 = d->fg;
	if(!is_overworld)
	{
		if((val&0xF) >= 0x8)
			force_16 = true;
		else if((val&0xF) < val_offset && (val&0xF0) < 0x80)
			force_16 = true;
	}
	if(force_16) //can't bound, some quests need to warp out of bounds... -Em
	{
		scrw = 16; //just force show the larger grid instead
		val_offset = 0;
	}
	
	int xscl = d->w/scrw;
	int yscl = d->h/scrh;
	
	int ret = D_O_K;
	bool redraw = false;
	if(d->d1 != val)
	{
		redraw = true;
		d->d1 = val;
	}
	if(bufval != val)
	{
		redraw = true;
		sprintf(buf, "%X", val);
	}
	if(d->d2 != *dmap_ptr)
	{
		redraw = true;
		d->d2 = *dmap_ptr;
	}
	switch(msg)
	{
		case MSG_WANTFOCUS:
			ret = D_WANTFOCUS;
			break;
		case MSG_CLICK:
		{
			d->fg = force_16 ? 1 : 0;
			bool redraw2 = false;
			while(gui_mouse_b())
			{
				if(redraw2)
				{
					broadcast_dialog_message(MSG_DRAW, 0);
					redraw2 = false;
				}
				if(!d->fg && (gui_mouse_b()&2))
				{
					scrw = 16;
					xscl = d->w/scrw;
					yscl = d->h/scrh;
					val_offset = 0;
					d->fg = 1;
					redraw2 = true;
				}
				custom_vsync();
				if(!mouse_in_rect(d->x,d->y,d->w,d->h))
					continue;
				int mx = gui_mouse_x()-d->x, my = gui_mouse_y()-d->y;
				int y = vbound(my/yscl,0,scrh-1);
				auto offs = y==8 ? 0 : val_offset;
				int x = vbound(mx/xscl,offs,scrw-1);
				auto val2 = (y*16)+x;
				if(val2 > max) //out of bounds in the bottom-right
					continue;
				val = val2;
				if(d->d1 != val)
				{
					d->d1 = val;
					sprintf(buf, "%02X", val);
					redraw2 = true;
				}
			}
			redraw = true;
			d->fg = 0;
			break;
		}
		case MSG_DRAW:
		{
			rectfill(screen,d->x,d->y,d->x+d->w-1,d->y+d->h-1,jwin_pal[jcBOX]);
			jwin_draw_frame(screen, d->x-2, d->y-2, d->w+4, d->h+4, FR_MENU);
			for(int yind = 0; yind < scrh; ++yind)
			{
				auto gr = (yind < 8 ? dm.grid[yind] : 0);
				for(int xind = (yind == 8 ? 0 : val_offset); xind < scrw; ++xind)
				{
					int scr = xind+(yind*16);
					if(scr > max)
						continue;
					int fr = FR_MENU;
					if(scr == d->d1)
						fr = FR_GREEN;
					else if(!is_overworld && xind < 8 && (gr&(1<<(8-xind-1))))
						fr = FR_MENU_INV;
					jwin_draw_frame(screen, d->x+(xind*xscl), d->y+(yind*yscl), xscl, yscl, fr);
				}
			}
			break;
		}
		case MSG_XCHAR:
		{
			bool on_80 = (val&0xF0) == 0x80;
			switch(c>>8)
			{
				case KEY_UP:
					if((val&0xF0) && !(val_offset && on_80 && (val&0xF) < val_offset))
					{
						val -= 0x10;
						redraw = true;
					}
					ret |= D_USED_CHAR;
					break;
				case KEY_DOWN:
					if((val&0xF0) < ((val&0xF) < 0x8 ? 0x80 : 0x70))
					{
						val += 0x10;
						redraw = true;
					}
					ret |= D_USED_CHAR;
					break;
				case KEY_LEFT:
					if((val&0xF) > (on_80 ? 0 : val_offset))
					{
						--val;
						redraw = true;
					}
					ret |= D_USED_CHAR;
					break;
				case KEY_RIGHT:
					if((val&0xF) < scrw-1 && val < 0x87)
					{
						++val;
						redraw = true;
					}
					ret |= D_USED_CHAR;
					break;
			}
			if(redraw)
				sprintf(buf, "%02X", val);
			break;
		}
	}
	if(redraw)
	{
		if(msg == MSG_IDLE)
			broadcast_dialog_message(MSG_DRAW,0);
		else
		{
			d->d1 = d->d2 = -1;
			object_message(d,MSG_IDLE,0);
		}
	}
	
	return ret;
}

int32_t tilewarpdmapxy[6] = {170,38,170,18,170,27};
static DIALOG tilewarp_dlg[] =
{
	/* (dialog proc)           (x)     (y)     (w)     (h)     (fg)                    (bg)                  (key) (flags)     (d1) (d2)  (dp) */
	{ jwin_win_proc,             0,      0,    302,    178,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0,  NULL, NULL, NULL },
	{ jwin_rtext_proc,          89,     43,     40,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Type:", NULL, NULL },
	{ jwin_rtext_proc,          57,     62,     40,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "DMap:", NULL, NULL },
	{ jwin_rtext_proc,          57,     80,     64,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Screen: 0x", NULL, NULL },
	{ jwin_droplist_proc,       91,     38,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &warp_dlg_list, NULL, NULL },
	//5
	{ d_dropdmaplist_proc,      59,     57,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &dmap_list, NULL, tilewarpdmapxy },
	{ jwin_hexedit_proc,        59,     76,     24,     16,    vc(12),                 vc(1),                   0, 0,          2,     0,  NULL, NULL, NULL },
	{ jwin_button_proc,         61,    152,     41,     21,    vc(14),                 vc(1),                 'k', D_EXIT,     0,     0, (void *) "O&K", NULL, NULL },
	{ jwin_button_proc,        121,    152,     41,     21,    vc(14),                 vc(1),                 'g', D_EXIT,     0,     0, (void *) "&Go", NULL, NULL },
	{ jwin_button_proc,        181,    152,     61,     21,    vc(14),                 vc(1),                  27, D_EXIT,     0,     0, (void *) "Cancel", NULL, NULL },
	//10
	{ jwin_rtext_proc,          82,     95,    100,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Use Warp Return:", NULL, NULL },
	{ jwin_droplist_proc,       10,    105,     72,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &warp_ret_list, NULL, NULL },
	{ jwin_check_proc,          10,    125,    129,      9,    vc(14),                 vc(1),                   0, 0,          1,     0, (void *) "Combos Carry Over", NULL, NULL },
	{ d_warpdestscrsel_proc,    90,     76,   8*16,    8*9,    0,                      0,                       0, 0,          0,     0,  NULL, NULL, NULL },
	{ jwin_button_proc,         59,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "A", NULL, NULL },
	//15
	{ jwin_button_proc,        109,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "B", NULL, NULL },
	{ jwin_button_proc,        159,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "C", NULL, NULL },
	{ jwin_button_proc,        209,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "D", NULL, NULL },
	{ NULL,                      0,      0,      0,      0,    0,                      0,                       0, 0,          0,     0,  NULL, NULL, NULL }
};

int32_t sidewarpdmapxy[6] = {170,38,170,18,170,27};
static DIALOG sidewarp_dlg[] =
{
	/* (dialog proc)           (x)     (y)     (w)     (h)     (fg)                    (bg)                  (key) (flags)     (d1) (d2)  (dp) */
	{ jwin_win_proc,             0,      0,    302,    178,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0,  NULL, NULL, NULL },
	{ jwin_rtext_proc,          89,     43,     40,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Type:", NULL, NULL },
	{ jwin_rtext_proc,          57,     62,     40,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "DMap:", NULL, NULL },
	{ jwin_rtext_proc,          57,     80,     64,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Screen: 0x", NULL, NULL },
	{ jwin_droplist_proc,       91,     38,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &warp_dlg_list, NULL, NULL },
	//5
	{ d_dropdmaplist_proc,      59,     57,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &dmap_list, NULL, tilewarpdmapxy },
	{ jwin_hexedit_proc,        59,     76,     24,     16,    vc(12),                 vc(1),                   0, 0,          2,     0,  NULL, NULL, NULL },
	{ jwin_button_proc,         61,    152,     41,     21,    vc(14),                 vc(1),                 'k', D_EXIT,     0,     0, (void *) "O&K", NULL, NULL },
	{ jwin_button_proc,        121,    152,     41,     21,    vc(14),                 vc(1),                 'g', D_EXIT,     0,     0, (void *) "&Go", NULL, NULL },
	{ jwin_button_proc,        181,    152,     61,     21,    vc(14),                 vc(1),                  27, D_EXIT,     0,     0, (void *) "Cancel", NULL, NULL },
	//10
	{ jwin_rtext_proc,          82,     95,    100,      8,    vc(14),                 vc(1),                   0, 0,          0,     0, (void *) "Use Warp Return:", NULL, NULL },
	{ jwin_droplist_proc,       10,    105,     72,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,     0, (void *) &warp_ret_list, NULL, NULL },
	{ jwin_check_proc,          10,    125,    129,      9,    vc(14),                 vc(1),                   0, 0,          1,     0, (void *) "Combos Carry Over", NULL, NULL },
	{ d_warpdestscrsel_proc,    90,     76,   8*16,    8*9,    0,                      0,                       0, 0,          0,     0,  NULL, NULL, NULL },
	{ jwin_button_proc,         59,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "A", NULL, NULL },
	//15
	{ jwin_button_proc,        109,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "B", NULL, NULL },
	{ jwin_button_proc,        159,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "C", NULL, NULL },
	{ jwin_button_proc,        209,     21,     50,     14,    vc(14),                 vc(1),                   0, D_EXIT,     0,     0, (void *) "D", NULL, NULL },
	// 18
	{ d_wflag_proc,             18,     17,     15,      8,    vc(4),                  vc(0),                   0, 0,          1,             0,  NULL, NULL, NULL },
	{ d_wflag_proc,             18,     47,     15,      8,    vc(4),                  vc(0),                   0, 0,          1,             0,  NULL, NULL, NULL },
	// 20
	{ d_wflag_proc,              8,     27,      8,     15,    vc(4),                  vc(0),                   0, 0,          1,             0,  NULL, NULL, NULL },
	{ d_wflag_proc,            	37,     27,      8,     15,    vc(4),                  vc(0),                   0, 0,          1,             0,  NULL, NULL, NULL },
	
	{ NULL,                      0,      0,      0,      0,    0,                      0,                       0, 0,          0,             0,  NULL, NULL, NULL }
};

int32_t warpringxy[6] = {170,38,170,18,170,27};
static DIALOG warpring_warp_dlg[] =
{
	/* (dialog proc)           (x)     (y)      (w)     (h)      (fg)                  (bg)                 (key)  (flags)     (d1) (d2)   (dp) */
	{ jwin_win_proc,             0,      0,    302,    145,    vc(14),                 vc(1),                   0, D_EXIT,     0,      0,  NULL, NULL, NULL },
	{ jwin_rtext_proc,          57,     25,     40,      8,    vc(14),                 vc(1),                   0, 0,          0,      0, (void *) "DMap:", NULL, NULL },
	{ jwin_rtext_proc,          57,     46,     64,      8,    vc(14),                 vc(1),                   0, 0,          0,      0, (void *) "Screen: 0x", NULL, NULL },
	{ d_dropdmaplist_proc,      59,     19,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0, 0,          0,      0, (void *) &dmap_list, NULL, warpringxy },
	{ jwin_hexedit_proc,        59,     39,     24,     16,    vc(12),                 vc(1),                   0, 0,          2,      0,  NULL, NULL, NULL },
	// 5
	{ jwin_button_proc,         61,    115,     41,     21,    vc(14),                 vc(1),                 'k', D_EXIT,     0,      0, (void *) "O&K", NULL, NULL },
	{ jwin_button_proc,        121,    115,     41,     21,    vc(14),                 vc(1),                 'g', D_EXIT,     0,      0, (void *) "&Go", NULL, NULL },
	{ jwin_button_proc,        181,    115,     61,     21,    vc(14),                 vc(1),                  27, D_EXIT,     0,      0, (void *) "Cancel", NULL, NULL },
	{ d_warpdestscrsel_proc,    90,     39,   8*16,    8*9,    0,                      0,                       0, 0,          0,      0,  NULL, NULL, NULL },
	
	{ NULL,                      0,      0,      0,      0,    0,                      0,                       0, 0,          0,      0,  NULL, NULL, NULL }
};

// Side warp flag procedure
int32_t d_wflag_proc(int32_t msg,DIALOG *d,int32_t)
{
	int32_t ret = D_O_K;
    switch(msg)
    {
		case MSG_DRAW:
		{
			int32_t c2=(d->flags&D_SELECTED)?d->fg:d->bg;
			
			if(d->d1==1)
			{
				jwin_draw_frame(screen,d->x,d->y,d->w,d->h, FR_DEEP);
				rectfill(screen,d->x+2, d->y+2, d->x+d->w-3, d->y+d->h-3,c2);
				
				if(d->flags&D_SELECTED)
				{
					int32_t e=d->d2&3;
					
					if(d->w>d->h)
						textprintf_centre_ex(screen,get_zc_font(font_lfont_l), d->x+(d->w/2),d->y,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
					else
						textprintf_centre_ex(screen,get_zc_font(font_lfont_l), d->x+(d->w/2),d->y+(d->h/2)-4,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
				}
				
			}
			else
			{
				rectfill(screen,d->x, d->y, d->x+d->w-1, d->y+d->h-1,c2);
			}
		}
		break;
		
		case MSG_CLICK:
		{
			if(d->flags & D_DISABLED)
				return D_O_K;
			bool rclick = gui_mouse_b() & 2;
			if(d->d1==1)
			{
				if(!(d->flags&D_SELECTED))
				{
					d->flags |= D_SELECTED;
					d->d2 &= 0x80;
					if (rclick)
						d->d2 |= 3;
				}
				else
				{
					if((d->d2&3) == (rclick?0:3))
					{
						d->flags ^= D_SELECTED;
						d->d2 &= 0x80;
					}
					else
					{
						int32_t f = d->d2&3;
						d->d2 &= 0x80;
						f += rclick ? -1 : 1;
						d->d2 |= f;
					}
				}
			}
			else
			{
				d->flags^=D_SELECTED;
			}
			
			int32_t c2=(d->flags&D_SELECTED)?d->fg:d->bg;
			
			if(d->d1==1)
			{
				jwin_draw_frame(screen,d->x,d->y,d->w,d->h, FR_DEEP);
				rectfill(screen,d->x+2, d->y+2, d->x+d->w-3, d->y+d->h-3,c2);
				
				if(d->flags&D_SELECTED)
				{
					int32_t e=d->d2&3;
					
					if(d->w>d->h)
						textprintf_centre_ex(screen,get_zc_font(font_lfont_l),d->x+(d->w/2),d->y,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
					else
						textprintf_centre_ex(screen,get_zc_font(font_lfont_l),d->x+(d->w/2),d->y+(d->h/2)-4,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
				}
			}
			else
			{
				rectfill(screen,d->x, d->y, d->x+d->w-1, d->y+d->h-1,c2);
			}
			
			
			while(gui_mouse_b())
			{
				/* do nothing */
				rest(1);
			}
			ret = D_REDRAWME;
		}
		break;
    }
    
    return ret;
}

int32_t d_dmapscrsel_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int32_t ret = D_O_K;
    
    switch(msg)
    {
    case MSG_CLICK:
        sprintf((char*)((d+2)->dp),"%X%X",vbound((gui_mouse_y()-d->y)/4,0,7),vbound((gui_mouse_x()-d->x)/(((DMaps[(d-1)->d1].type&dmfTYPE)==1)?4:8),0,(((DMaps[(d-1)->d1].type&dmfTYPE)==1)?15:7)));
        object_message(d+2, MSG_DRAW, 0);
        break;
    }
    
    return ret;
}

int32_t warpdestsel_x=-1;
int32_t warpdestsel_y=-1;
int32_t warpdestmap=-1;
int32_t warpdestscr=-1;

int32_t jwin_minibutton_proc(int32_t msg,DIALOG *d,int32_t c)
{
    switch(msg)
    {
    case MSG_DRAW:
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, false);
        return D_O_K;
        break;
    }
    
    return jwin_button_proc(msg,d,c);
}

int32_t d_triggerbutton_proc(int32_t msg,DIALOG *d,int32_t c)
{
    static BITMAP *dummy=create_bitmap_ex(8, 1, 1);
    
    switch(msg)
    {
    case MSG_START:
        d->w=gui_textout_ln(dummy, font, (uint8_t *)d->dp, 0, 0, jwin_pal[jcMEDDARK], -1, 0)+4;
        d->h=text_height(font)+5;
        break;
        
    case MSG_GOTFOCUS:
        d->flags&=~D_GOTFOCUS;
        break;
        
    }
    
    return jwin_minibutton_proc(msg,d,c);
}

int32_t d_alltriggerbutton_proc(int32_t msg,DIALOG *d,int32_t c)
{
    DIALOG *temp_d;
    int32_t ret=d_triggerbutton_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_CLICK:
        temp_d=d-1;
        
        while(temp_d->proc==d_triggerbutton_proc)
        {
            temp_d->flags&=~D_SELECTED;
            temp_d->flags|=D_DIRTY;
            
            if(d->flags&D_SELECTED)
            {
                temp_d->flags|=D_SELECTED;
            }
            
            --temp_d;
        }
        
        break;
    }
    
    return ret;
}

int32_t d_ticsedit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_edit_proc(msg,d,c);
    
    if(msg==MSG_DRAW)
    {
        int32_t tics=vbound(atoi((char*)d->dp),0,65535);
        sprintf((char*)(d+1)->dp,"%s %s",ticksstr(tics),tics==0?"(No Timed Warp)":"               ");
        object_message(d+1,MSG_DRAW,c);
    }
    
    return ret;
}

static ListData warp_effect_list(warpeffectlist,&font);

struct tw_data
{
	int twscr[4], twtype[4], twdmap[4], wret[4];
	byte oflags;
	optional<uint> loaded;
	
	tw_data(mapscr* scr) {load_scr(scr);}
	void load_scr(mapscr* scr)
	{
		oflags = scr->tilewarpoverlayflags;
		for(int q = 0; q < 4; ++q)
		{
			twscr[q] = scr->tilewarpscr[q];
			twtype[q] = scr->tilewarptype[q];
			twdmap[q] = scr->tilewarpdmap[q];
			wret[q] = (scr->warpreturnc >> (2*q))&3;
		}
		loaded.reset();
	}
	void save_scr(mapscr* scr)
	{
		saved=false;
		scr->tilewarpoverlayflags = oflags;
		scr->warpreturnc = scr->warpreturnc & 0xFF00;
		for(int q = 0; q < 4; ++q)
		{
			scr->tilewarpscr[q] = twscr[q];
			scr->tilewarptype[q] = twtype[q];
			scr->tilewarpdmap[q] = twdmap[q];
			scr->warpreturnc |= wret[q] << (2*q);
		}
	}
	
	void load(uint ind)
	{
		if(ind >= 4) return;
		loaded = ind;
		tilewarp_dlg[4].d1 = twtype[ind];
		tilewarp_dlg[5].d1 = twdmap[ind];
		char* buf = (char*)tilewarp_dlg[6].dp;
		sprintf(buf,"%02X",twscr[ind]);
		tilewarp_dlg[11].d1 = wret[ind];
		SETFLAG(tilewarp_dlg[12].flags, D_SELECTED, get_bit(&oflags,ind));
		for(int q = 0; q < 4; ++q)
			SETFLAG(tilewarp_dlg[14+q].flags,(D_SELECTED|D_DISABLED),q==ind);
	}
	void save(uint ind)
	{
		if(ind >= 4) return;
		twtype[ind] = tilewarp_dlg[4].d1;
		twdmap[ind] = tilewarp_dlg[5].d1;
		char* buf = (char*)tilewarp_dlg[6].dp;
		twscr[ind] = vbound(zc_xtoi(buf),0x00,0x87);
		wret[ind] = tilewarp_dlg[11].d1;
		set_bit(&oflags, ind, tilewarp_dlg[12].flags & D_SELECTED);
	}
	void save()
	{
		if(loaded)
			save(*loaded);
	}
	void swap(uint ind)
	{
		if(ind >= 4) return;
		if(loaded)
		{
			save(*loaded);
			if(*loaded == ind)
				return;
		}
		load(ind);
	}
};
int32_t onTileWarp()
{
    restore_mouse();
    tilewarp_dlg[0].dp=(void *) "Tile Warp";
    tilewarp_dlg[0].dp2=get_zc_font(font_lfont);
    
	mapscr* mptr = Map.CurrScr();
    char buf[10];
    tilewarp_dlg[6].dp=buf;
	tilewarp_dlg[13].dp = buf;
	tilewarp_dlg[13].dp3 = &tilewarp_dlg[5].d1;
	
	vector<DIALOG*> dlgs;
	dlgs.push_back(&tilewarp_dlg[5]);
	dlgs.push_back(&tilewarp_dlg[6]);
	tilewarp_dlg[13].dp2 = &dlgs;
	
	tw_data data(mptr);
	data.load(0);
    
    dmap_list_size=MAXDMAPS;
    dmap_list_zero=true;
    
	large_dialog(tilewarp_dlg);
    
	bool running = true;
	int ret;
	do
    {
		ret = do_zqdialog(tilewarp_dlg,-1);
		switch(ret)
		{
			// OK, GO
			case 7: case 8:
				running = false;
				data.save();
				data.save_scr(mptr);
				refresh(rMENU);
				break;
			//Cancel
			case 9:
				running = false;
				break;
			//A,B,C,D
			case 14: case 15: case 16: case 17:
				data.swap(ret-14);
				break;
		}
	}
	while(running);
    
    if(ret==8) //GO
    {
        int32_t index = *data.loaded;
        
        FlashWarpSquare = -1;
        int32_t tm = Map.getCurrMap();
        int32_t ts = Map.getCurrScr();
        int32_t thistype = mptr->tilewarptype[index];
        Map.dowarp(0,index);
        
        if((ts!=Map.getCurrScr() || tm!=Map.getCurrMap()) && thistype != wtCAVE && thistype != wtSCROLL)
        {
            FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(index*2))&3;
            FlashWarpClk = 32;
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}

struct sw_data
{
	int swscr[4], swtype[4], swdmap[4], wret[4];
	byte oflags;
	optional<uint> loaded;
	
	sw_data(mapscr* scr) {load_scr(scr);}
	void load_scr(mapscr* scr)
	{
		oflags = scr->sidewarpoverlayflags;
		for(int q = 0; q < 4; ++q)
		{
			swscr[q] = scr->sidewarpscr[q];
			swtype[q] = scr->sidewarptype[q];
			swdmap[q] = scr->sidewarpdmap[q];
			wret[q] = (scr->warpreturnc >> (2*(q+4)))&3;
		}
		loaded.reset();
		
		for(int32_t i=0; i<4; i++)
		{
			sidewarp_dlg[18+i].d2 = 0x80;
			if(scr->flags2&(1<<i))
			{
				sidewarp_dlg[18+i].flags = D_SELECTED ;
				sidewarp_dlg[18+i].d2 |= (scr->sidewarpindex>>(2*i))&3;
			}
			else
			{
				sidewarp_dlg[18+i].flags = 0;
			}
		}
	}
	void save_scr(mapscr* scr)
	{
		saved=false;
		scr->sidewarpoverlayflags = oflags;
		scr->warpreturnc = scr->warpreturnc & 0x00FF;
		for(int q = 0; q < 4; ++q)
		{
			scr->sidewarpscr[q] = swscr[q];
			scr->sidewarptype[q] = swtype[q];
			scr->sidewarpdmap[q] = swdmap[q];
			scr->warpreturnc |= wret[q] << (2*(q+4));
		}
		
		scr->flags2 &= ~0xF;
		scr->sidewarpindex = 0;
		for(int32_t i=0; i<4; i++)
		{
			if(sidewarp_dlg[18+i].flags & D_SELECTED)
				scr->flags2 |= 1<<i;
			scr->sidewarpindex |= (sidewarp_dlg[18+i].d2&3) << (i*2);
		}
	}
	
	void load(uint ind)
	{
		if(ind >= 4) return;
		loaded = ind;
		sidewarp_dlg[4].d1 = swtype[ind];
		sidewarp_dlg[5].d1 = swdmap[ind];
		char* buf = (char*)sidewarp_dlg[6].dp;
		sprintf(buf,"%02X",swscr[ind]);
		sidewarp_dlg[11].d1 = wret[ind];
		SETFLAG(sidewarp_dlg[12].flags, D_SELECTED, get_bit(&oflags,ind));
		for(int q = 0; q < 4; ++q)
			SETFLAG(sidewarp_dlg[14+q].flags,(D_SELECTED|D_DISABLED),q==ind);
	}
	void save(uint ind)
	{
		if(ind >= 4) return;
		swtype[ind] = sidewarp_dlg[4].d1;
		swdmap[ind] = sidewarp_dlg[5].d1;
		char* buf = (char*)sidewarp_dlg[6].dp;
		swscr[ind] = vbound(zc_xtoi(buf),0x00,0x87);
		wret[ind] = sidewarp_dlg[11].d1;
		set_bit(&oflags, ind, sidewarp_dlg[12].flags & D_SELECTED);
	}
	void save()
	{
		if(loaded)
			save(*loaded);
	}
	void swap(uint ind)
	{
		if(ind >= 4) return;
		if(loaded)
		{
			save(*loaded);
			if(*loaded == ind)
				return;
		}
		load(ind);
	}
};
int32_t onSideWarp()
{
    restore_mouse();
    sidewarp_dlg[0].dp=(void *) "Side Warp";
    sidewarp_dlg[0].dp2=get_zc_font(font_lfont);
    
	mapscr* mptr = Map.CurrScr();
    char buf[10];
    sidewarp_dlg[6].dp=buf;
	sidewarp_dlg[13].dp = buf;
	sidewarp_dlg[13].dp3 = &sidewarp_dlg[5].d1;
	
	vector<DIALOG*> dlgs;
	dlgs.push_back(&sidewarp_dlg[5]);
	dlgs.push_back(&sidewarp_dlg[6]);
	sidewarp_dlg[13].dp2 = &dlgs;
	
	sw_data data(mptr);
	data.load(0);
    
    dmap_list_size=MAXDMAPS;
    dmap_list_zero=true;
    
	large_dialog(sidewarp_dlg);
    
	bool running = true;
	int ret;
	do
    {
		ret = do_zqdialog(sidewarp_dlg,-1);
		switch(ret)
		{
			// OK, GO
			case 7: case 8:
				running = false;
				data.save();
				data.save_scr(mptr);
				refresh(rMENU);
				break;
			//Cancel
			case 9:
				running = false;
				break;
			//A,B,C,D
			case 14: case 15: case 16: case 17:
				data.swap(ret-14);
				break;
		}
	}
	while(running);
    
    if(ret==8) //GO
    {
        int32_t index = *data.loaded;
		
        FlashWarpSquare = -1;
        int32_t tm = Map.getCurrMap();
        int32_t ts = Map.getCurrScr();
		
        int32_t thistype = mptr->sidewarptype[index];
        Map.dowarp(1,index);
        
        if((ts!=Map.getCurrScr() || tm!=Map.getCurrMap()) && thistype != wtSCROLL)
        {
            FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(8+index*2))&3;
            FlashWarpClk = 0x20;
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}



const char *dirlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        if(index>3)
            index=3;
            
        return mazedirstr[index];
    }
    
    *list_size=4;
    return NULL;
}

static ListData path_dlg_list(dirlist, &font);

static DIALOG path_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,      80,   57,   161,  164,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Maze Path", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       94,   106,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,       94,   124,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,       94,   142,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,       94,   160,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "4th", NULL, NULL },
    { jwin_text_proc,       94,   178,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Exit", NULL, NULL },
    { jwin_droplist_proc,   140,  102,   80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  120,   80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  138,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  156,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  174,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_button_proc,     90,   194,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  194,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { jwin_text_proc,       87,   82,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "A Lost Woods-style maze screen", NULL, NULL },
    { jwin_text_proc,       87,   92,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "with a normal and secret exit.", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t onPath()
{
    restore_mouse();
    path_dlg[0].dp2=get_zc_font(font_lfont);
    
    for(int32_t i=0; i<4; i++)
        path_dlg[i+7].d1 = Map.CurrScr()->path[i];
        
    path_dlg[11].d1 = Map.CurrScr()->exitdir;
    
    large_dialog(path_dlg);
        
    int32_t ret;
    
    do
    {
        ret=do_zqdialog(path_dlg,7);
        
        if(ret==12) for(int32_t i=0; i<4; i++)
            {
                if(path_dlg[i+7].d1 == path_dlg[11].d1)
                {
                    if(jwin_alert("Exit Problem","One of the path's directions is","also the normal Exit direction! Continue?",NULL,"Yes","No",'y','n',get_zc_font(font_lfont))==2)
                        ret = -1;
                        
                    break;
                }
            }
    }
    while(ret == -1);
    
    if(ret==12)
    {
        saved=false;
        
        for(int32_t i=0; i<4; i++)
            Map.CurrScr()->path[i] = path_dlg[i+7].d1;
            
        Map.CurrScr()->exitdir = path_dlg[11].d1;
        
        if(!(Map.CurrScr()->flags&fMAZE))
            if(jwin_alert("Screen Flag","Turn on the 'Use Maze Path' Screen Flag?","(Go to 'Screen Data' to turn it off.)",NULL,"Yes","No",'y','n',get_zc_font(font_lfont))==1)
                Map.CurrScr()->flags |= fMAZE;
    }
    
    refresh(rMAP+rMENU);
    return D_O_K;
}



static DIALOG editinfo_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,   10,  208,  204,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   60,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,     24,  106,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,     24,  152,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,     56,   60,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  106,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  152,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    // 8
    { jwin_edit_proc,     86,   56,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,   74,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  102,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,  120,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  148,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,  166,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   42,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     56,   38,  137,   16,  vc(12),              vc(1),                  0,           0,    31,             0,       NULL, NULL, NULL },
    // 16
    { jwin_button_proc,   34,  188,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  114,  188,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL,                           NULL,  NULL }
};

void EditInfoType(int32_t index)
{
    char ps1[6],ps2[6],ps3[6];
    char infoname[33];
    char caption[40];
    
    int32_t str1, str2, str3;
    
    sprintf(caption,"Info Data %d",index);
    editinfo_dlg[0].dp = caption;
    editinfo_dlg[0].dp2 = get_zc_font(font_lfont);
    
    sprintf(ps1,"%d",QMisc.info[index].price[0]);
    sprintf(ps2,"%d",QMisc.info[index].price[1]);
    sprintf(ps3,"%d",QMisc.info[index].price[2]);
    strncpy(infoname,QMisc.info[index].name,32);
    infoname[32] = 0;
    editinfo_dlg[8].dp  = ps1;
    editinfo_dlg[10].dp = ps2;
    editinfo_dlg[12].dp = ps3;
    editinfo_dlg[15].dp = infoname;
    str1 = QMisc.info[index].str[0];
    str2 = QMisc.info[index].str[1];
    str3 = QMisc.info[index].str[2];
    editinfo_dlg[9].d1  = MsgStrings[str1].listpos;
    editinfo_dlg[11].d1 = MsgStrings[str2].listpos;
    editinfo_dlg[13].d1 = MsgStrings[str3].listpos;
    ListData msgs_list(msgslist2, &a4fonts[font_lfont_l]);
    editinfo_dlg[9].dp  =
        editinfo_dlg[11].dp =
            editinfo_dlg[13].dp = (void *) &msgs_list;
            
    large_dialog(editinfo_dlg);
        
    int32_t ret = do_zqdialog(editinfo_dlg,-1);
    
    if(ret==16)
    {
        saved=false;
        QMisc.info[index].price[0] = vbound(atoi(ps1), 0, 65535);
        QMisc.info[index].price[1] = vbound(atoi(ps2), 0, 65535);
        QMisc.info[index].price[2] = vbound(atoi(ps3), 0, 65535);
        strncpy(QMisc.info[index].name,infoname,32);
        str1 = editinfo_dlg[9].d1;
        str2 = editinfo_dlg[11].d1;
        str3 = editinfo_dlg[13].d1;
        QMisc.info[index].str[0] = msg_at_pos(str1);
        QMisc.info[index].str[1] = msg_at_pos(str2);
        QMisc.info[index].str[2] = msg_at_pos(str3);
        
        //move 0s to the end
        word swaptmp;
        
        if(QMisc.info[index].str[0] == 0)
        {
            //possibly permute the infos
            if(QMisc.info[index].str[1] != 0)
            {
                //swap
                swaptmp = QMisc.info[index].str[0];
                QMisc.info[index].str[0] = QMisc.info[index].str[1];
                QMisc.info[index].str[1] = swaptmp;
                swaptmp = QMisc.info[index].price[0];
                QMisc.info[index].price[0] = QMisc.info[index].price[1];
                QMisc.info[index].price[1] = swaptmp;
            }
            else if(QMisc.info[index].str[2] != 0)
            {
                //move info 0 to 1, 1 to 2, and 2 to 0
                swaptmp = QMisc.info[index].str[0];
                QMisc.info[index].str[0] = QMisc.info[index].str[2];
                QMisc.info[index].str[2] = QMisc.info[index].str[1];
                QMisc.info[index].str[1] = swaptmp;
                swaptmp = QMisc.info[index].price[0];
                QMisc.info[index].price[0] = QMisc.info[index].price[2];
                QMisc.info[index].price[2] = QMisc.info[index].price[1];
                QMisc.info[index].price[1] = swaptmp;
            }
        }
        
        if(QMisc.info[index].str[1] == 0 && QMisc.info[index].str[2] != 0)
            //swap
        {
            swaptmp = QMisc.info[index].str[1];
            QMisc.info[index].str[1] = QMisc.info[index].str[2];
            QMisc.info[index].str[2] = swaptmp;
            swaptmp = QMisc.info[index].price[1];
            QMisc.info[index].price[1] = QMisc.info[index].price[2];
            QMisc.info[index].price[2] = swaptmp;
        }
    }
}

int32_t onInfoTypes()
{
    info_list_size = 256;
    
    int32_t index = select_data("Info Types",0,infolist,"Edit","Done",get_zc_font(font_lfont));
    
    while(index!=-1)
    {
        EditInfoType(index);
        
        index = select_data("Info Types",index,infolist,"Edit","Done",get_zc_font(font_lfont));
    }
    
    return D_O_K;
}



//This dialogie is self-contained, and does not use dialogue control numbers in a separate array to generate its fields.
static DIALOG editshop_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,   10,  221,  204,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   60,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,     24,  106,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,     24,  152,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,     56,   60,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  106,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  152,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    // 8
    { jwin_edit_proc,     86,   56,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,   74,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  102,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,  120,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  148,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,  166,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   42,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     56,   38,  137,   16,  vc(12),              vc(1),                  0,           0,    31,             0,       NULL, NULL, NULL },
    
    // 16
    { jwin_button_proc,   40,  188,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  121,  188,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    //18
    { jwin_text_proc,    130,   60,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Info:", NULL, NULL },
    { jwin_text_proc,    130,  106,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Info:", NULL, NULL },
    { jwin_text_proc,    130,  152,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Info:", NULL, NULL },
    // 21
    { jwin_edit_proc,     155,   56,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     155,   102,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     155,   148,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,      0,          0,             0,       NULL,                           NULL,  NULL }
};

void EditShopType(int32_t index)
{

    build_bii_list(true);
    char ps1[6],ps2[6],ps3[6];
	char info1[6],info2[6],info3[6];
    char shopname[32];
    char caption[40];
    
    sprintf(caption,"Shop Data %d",index);
    editshop_dlg[0].dp = caption;
    editshop_dlg[0].dp2=get_zc_font(font_lfont);
    
    sprintf(ps1,"%d",QMisc.shop[index].price[0]);
    sprintf(ps2,"%d",QMisc.shop[index].price[1]);
    sprintf(ps3,"%d",QMisc.shop[index].price[2]);
	
    sprintf(info1,"%d",QMisc.shop[index].str[0]);
    sprintf(info2,"%d",QMisc.shop[index].str[1]);
    sprintf(info3,"%d",QMisc.shop[index].str[2]);
	
    sprintf(shopname,"%s",QMisc.shop[index].name);
    editshop_dlg[8].dp  = ps1;
    editshop_dlg[10].dp = ps2;
    editshop_dlg[12].dp = ps3;
    editshop_dlg[15].dp = shopname;
    
    editshop_dlg[21].dp  = info1;
    editshop_dlg[22].dp = info2;
    editshop_dlg[23].dp = info3;
    
    ListData item_list(itemlist_num, &a4fonts[font_lfont_l]);
    
    editshop_dlg[9].dp  = (void *) &item_list;
    editshop_dlg[11].dp  = (void *) &item_list;
    editshop_dlg[13].dp  = (void *) &item_list;
    
    for(int32_t i=0; i<3; ++i)
    {
        if(QMisc.shop[index].hasitem[i])
        {
            for(int32_t j=0; j<bii_cnt; j++)
            {
                if(bii[j].i == QMisc.shop[index].item[i])
                {
                    editshop_dlg[9+(i<<1)].d1  = j;
                }
            }
        }
        else
        {
            editshop_dlg[9+(i<<1)].d1 = -2;
        }
    }
    
    large_dialog(editshop_dlg);
        
    int32_t ret = do_zqdialog(editshop_dlg,-1);
    
    if(ret==16)
    {
        saved=false;
        QMisc.shop[index].price[0] = vbound(atoi(ps1), 0, 65535);
        QMisc.shop[index].price[1] = vbound(atoi(ps2), 0, 65535);
        QMisc.shop[index].price[2] = vbound(atoi(ps3), 0, 65535);
	    
	QMisc.shop[index].str[0] = vbound(atoi(info1), 0, 65535);
        QMisc.shop[index].str[1] = vbound(atoi(info2), 0, 65535);
        QMisc.shop[index].str[2] = vbound(atoi(info3), 0, 65535);
	    
        snprintf(QMisc.shop[index].name, 32, "%s",shopname);
        
        for(int32_t i=0; i<3; ++i)
        {
            if(bii[editshop_dlg[9+(i<<1)].d1].i == -2)
            {
                QMisc.shop[index].hasitem[i] = 0;
                QMisc.shop[index].item[i] = 0;
                QMisc.shop[index].price[i] = 0;
            }
            else
            {
                QMisc.shop[index].hasitem[i] = 1;
                QMisc.shop[index].item[i] = bii[editshop_dlg[9+(i<<1)].d1].i;
            }
        }
        
        //filter all the 0 items to the end (yeah, bubble sort; sue me)
        word swaptmp;
        
        for(int32_t j=0; j<3-1; j++)
        {
            for(int32_t k=0; k<2-j; k++)
            {
                if(QMisc.shop[index].hasitem[k]==0)
                {
                    swaptmp = QMisc.shop[index].item[k];
                    QMisc.shop[index].item[k] = QMisc.shop[index].item[k+1];
                    QMisc.shop[index].item[k+1] = swaptmp;
                    swaptmp = QMisc.shop[index].price[k];
                    QMisc.shop[index].price[k] = QMisc.shop[index].price[k+1];
                    QMisc.shop[index].price[k+1] = swaptmp;
                    swaptmp = QMisc.shop[index].hasitem[k];
                    QMisc.shop[index].hasitem[k] = QMisc.shop[index].item[k+1];
                    QMisc.shop[index].hasitem[k+1] = swaptmp;
                }
            }
        }
    }
}

int32_t onShopTypes()
{
    shop_list_size = 256;
    
    int32_t index = select_data("Shop Types",0,shoplist,"Edit","Done",get_zc_font(font_lfont));
    
    while(index!=-1)
    {
        EditShopType(index);
        index = select_data("Shop Types",index,shoplist,"Edit","Done",get_zc_font(font_lfont));
    }
    
    return D_O_K;
}

int32_t onStatusEffects()
{
	//!TODO_STATUS info_lister for statuses, with edit buttons
	// Edit button for default and hero separately?
	return D_O_K;
}

void call_bottle_dlg(int32_t index);
int32_t onBottleTypes()
{
	bottle_list_size = 64;
	int32_t index = 0;
	
	while(index > -1)
	{
		index = select_data("Bottle Types", index, bottlelist, "Edit", "Done", get_zc_font(font_lfont));
		if(index > -1)
			call_bottle_dlg(index);
	}
	
	return D_O_K;
}

void call_bottleshop_dlg(int32_t index);
int32_t onBottleShopTypes()
{
	bottleshop_list_size = 256;
	int32_t index = 0;
	
	while(index > -1)
	{
		index = select_data("Bottle Shop Types", index, bottleshoplist, "Edit", "Done", get_zc_font(font_lfont));
		if(index > -1)
			call_bottleshop_dlg(index);
	}
	
	return D_O_K;
}


static char item_drop_set_str_buf[70];
int32_t item_drop_set_list_size=MAXITEMDROPSETS;

const char *itemdropsetlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,item_drop_set_list_size-1);
        sprintf(item_drop_set_str_buf,"%3d:  %s",index,item_drop_sets[index].name);
        return item_drop_set_str_buf;
    }
    
    *list_size=item_drop_set_list_size;
    return NULL;
}

int32_t d_itemdropedit_proc(int32_t msg,DIALOG *d,int32_t c);

static int32_t edititemdropset_1_list[] =
{
    // dialog control number
    10, 11, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,24,25,26,27,28, -1
};

static int32_t edititemdropset_2_list[] =
{
    // dialog control number
    12, 13, 29, 30, 31, 32, 33,34,35,36,37,38,39,40,41,42,43, -1
};

static TABPANEL edititemdropset_tabs[] =
{
    // (text)
    { (char *)" Page 1 ",       D_SELECTED,   edititemdropset_1_list,  0, NULL },
    { (char *)" Page 2 ",       0,            edititemdropset_2_list,  0, NULL },
    { NULL,                     0,            NULL,                    0, NULL }
};

static DIALOG edititemdropset_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,    0,  320,  240,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    
    // 2
    { jwin_button_proc,   89,  213,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  169,  213,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    
    // 4
    { jwin_text_proc,      9,   29,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     39,   25,  275,   16,  vc(12),              vc(1),                  0,           0,    32,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      9,   47,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Nothing Chance:", NULL, NULL },
    { d_itemdropedit_proc,     84,   43,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    
    { jwin_tab_proc,       4,   65,  312,   143, vc(0),               vc(15),                 0,           0,     0,             0, (void *) edititemdropset_tabs,                    NULL, (void *)edititemdropset_dlg },
    { jwin_text_proc,     114,   43+4,   26,   16,   vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    // 10
    { jwin_text_proc,     10,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Chance:", NULL, NULL },
    { jwin_text_proc,     56,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Item:", NULL, NULL },
    { jwin_text_proc,     10,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Chance:", NULL, NULL },
    { jwin_text_proc,     56,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Item:", NULL, NULL },
    
    // 14
    { d_itemdropedit_proc,      9,   96,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,   96,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,   96+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  118,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  118,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,  118+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  140,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  140,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,  140+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  162,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  162,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       37,  162+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  184,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  184,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       37,  184+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
// 29
    { d_itemdropedit_proc,      9,   96,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,   96,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,   96+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  118,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  118,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  118+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  140,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  140,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  140+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  162,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  162,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  162+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  184,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_idroplist_proc,   55,  184,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      39,  184+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,      0,          0,             0,       NULL, NULL,  NULL }
};

int32_t d_itemdropedit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_edit_proc(msg,d,c);
    
    if(msg==MSG_DRAW)
    {
        int32_t t = atoi((char*)edititemdropset_dlg[7].dp);
        
        for(int32_t i=0; i<10; ++i)
        {
            t += atoi((char*)edititemdropset_dlg[14+(i*3)].dp);
        }
        
        {
            int32_t t2 = (int32_t)(100*atoi((char*)edititemdropset_dlg[7].dp) / zc_max(t,1));
            sprintf((char*)edititemdropset_dlg[9].dp,"%d%%%s",t2, t2 <= 11 ? " ":"");
            object_message(&edititemdropset_dlg[9],MSG_DRAW,c);
        }
        
        for(int32_t i=0; i<10; ++i)
        {
            int32_t t2 = (int32_t)(100*atoi((char*)edititemdropset_dlg[14+(i*3)].dp) / zc_max(t,1));
            sprintf((char*)edititemdropset_dlg[16+(i*3)].dp,"%d%%%s",t2, t2 <= 11 ? " ":"");
            object_message(&edititemdropset_dlg[16+(i*3)],MSG_DRAW,c);
        }
        
    }
    
    return ret;
}

void EditItemDropSet(int32_t index)
{
    build_bii_list(true);
    char chance[11][10];
    char itemdropsetname[64];
    char caption[40];
    char percent_str[11][5];
    
    sprintf(caption,"Item Drop Set Data %d",index);
    edititemdropset_dlg[0].dp = caption;
    edititemdropset_dlg[0].dp2=get_zc_font(font_lfont);
    
    sprintf(itemdropsetname,"%s",item_drop_sets[index].name);
    edititemdropset_dlg[5].dp = itemdropsetname;
    
    sprintf(chance[0],"%d",item_drop_sets[index].chance[0]);
    edititemdropset_dlg[7].dp = chance[0];
    
    ListData item_list(itemlist_num, &a4fonts[font_lfont_l]);
    sprintf(percent_str[0],"    ");
    edititemdropset_dlg[9].dp  = percent_str[0];
    
    for(int32_t i=0; i<10; ++i)
    {
        sprintf(chance[i+1],"%d",item_drop_sets[index].chance[i+1]);
        edititemdropset_dlg[14+(i*3)].dp  = chance[i+1];
        edititemdropset_dlg[15+(i*3)].dp  = (void *) &item_list;
        sprintf(percent_str[i+1],"    ");
        edititemdropset_dlg[16+(i*3)].dp  = percent_str[i+1];
        
        if(item_drop_sets[index].chance[i+1]==0)
        {
            edititemdropset_dlg[15+(i*3)].d1  = -2;
        }
        else
        {
            for(int32_t j=0; j<bii_cnt; j++)
            {
                if(bii[j].i == item_drop_sets[index].item[i])
                {
                    edititemdropset_dlg[15+(i*3)].d1  = j;
                }
            }
        }
    }
    
    large_dialog(edititemdropset_dlg);
        
    int32_t ret = do_zqdialog(edititemdropset_dlg,-1);
    
    if(ret==2)
    {
        saved=false;
        
        sprintf(item_drop_sets[index].name,"%s",itemdropsetname);
        
        item_drop_sets[index].chance[0]=atoi(chance[0]);
        
        for(int32_t i=0; i<10; ++i)
        {
            item_drop_sets[index].chance[i+1]=atoi(chance[i+1]);
            
            if(bii[edititemdropset_dlg[15+(i*3)].d1].i == -2)
            {
                item_drop_sets[index].chance[i+1]=0;
            }
            else
            {
                item_drop_sets[index].item[i] = bii[edititemdropset_dlg[15+(i*3)].d1].i;
            }
            
            if(item_drop_sets[index].chance[i+1]==0)
            {
                item_drop_sets[index].item[i] = 0;
            }
        }
    }
}

int32_t count_item_drop_sets()
{
    int32_t count=0;
    bool found=false;
    
    for(count=255; (count>0); --count)
    {
        for(int32_t i=0; (i<11); ++i)
        {
            if(item_drop_sets[count].chance[i]!=0)
            {
                found=true;
                break;
            }
        }
        
        if(found)
        {
            break;
        }
    }
    
    return count+1;
}

int32_t onItemDropSets()
{
    item_drop_set_list_size = MAXITEMDROPSETS;
    
    int32_t index = select_data("Item Drop Sets",0,itemdropsetlist,"Edit","Done",get_zc_font(font_lfont));
    
    while(index!=-1)
    {
        EditItemDropSet(index);
        index = select_data("Item Drop Sets",index,itemdropsetlist,"Edit","Done",get_zc_font(font_lfont));
    }
    
    return D_O_K;
}

int32_t curr_ring = 0;

void EditWarpRingScr(int32_t ring,int32_t index)
{
	char caption[40],buf[10];
	restore_mouse();
	
	sprintf(caption,"Ring %d  Warp %d",ring,index+1);
	warpring_warp_dlg[0].dp = (void *)caption;
	warpring_warp_dlg[0].dp2=get_zc_font(font_lfont);
	
	sprintf(buf,"%02X",QMisc.warp[ring].scr[index]);
	warpring_warp_dlg[3].d1 = QMisc.warp[ring].dmap[index];
	warpring_warp_dlg[4].dp = buf;
	warpring_warp_dlg[8].dp = buf;
	warpring_warp_dlg[8].dp3 = &warpring_warp_dlg[3].d1;
	
	vector<DIALOG*> dlgs;
	dlgs.push_back(&warpring_warp_dlg[3]);
	dlgs.push_back(&warpring_warp_dlg[4]);
	warpring_warp_dlg[8].dp2 = &dlgs;
	
	dmap_list_size=MAXDMAPS;
	dmap_list_zero=true;
	
	large_dialog(warpring_warp_dlg);
	
	int32_t ret=do_zqdialog(warpring_warp_dlg,-1);
	
	if(ret==5 || ret==6)
	{
		saved=false;
		QMisc.warp[ring].dmap[index] = warpring_warp_dlg[3].d1;
		QMisc.warp[ring].scr[index] = zc_xtoi(buf);
	}
	
	if(ret==6)
	{
		Map.dowarp2(ring,index);
		refresh(rALL);
	}
}

int32_t d_warplist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    if(msg==MSG_DRAW)
    {
        int32_t *xy = (int32_t*)(d->dp3);
        int32_t ring = curr_ring;
        int32_t dmap = QMisc.warp[ring].dmap[d->d1];
        float temp_scale = 1.5;
        
        drawdmap(dmap);
        
        if(xy[0]||xy[1])
        {
            int32_t x = d->x+int32_t((xy[0]-2)*temp_scale);
            int32_t y = d->y+int32_t((xy[1]-2)*temp_scale);
            int32_t w = 84;
            int32_t h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]||xy[3])
        {
            textprintf_ex(screen,font,d->x+int32_t(xy[2]*temp_scale),d->y+int32_t(xy[3]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %d ",DMaps[dmap].map+1);
        }
        
        if(xy[4]||xy[5])
        {
            textprintf_ex(screen,font,d->x+int32_t(xy[4]*temp_scale),d->y+int32_t(xy[5]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level:%2d ",DMaps[dmap].level);
        }
        
        if(xy[6]||xy[7])
        {
            textprintf_ex(screen,font,d->x+int32_t(xy[6]*temp_scale),d->y+int32_t(xy[7]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Scr: 0x%02X ",QMisc.warp[ring].scr[d->d1]);
        }
    }
    
    return jwin_list_proc(msg,d,c);
}

int32_t d_wclist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t d1 = d->d1;
    int32_t ret = jwin_droplist_proc(msg,d,c);
    QMisc.warp[curr_ring].size=d->d1+3;
    
    if(d->d1 != d1)
        return D_CLOSE;
        
    return ret;
}

const char *wclist(int32_t index, int32_t *list_size)
{
    static char buf[2];
    
    if(index>=0)
    {
        if(index>6)
            index=6;
            
        sprintf(buf,"%d",index+3);
        return buf;
    }
    
    *list_size=7;
    return NULL;
}

//int32_t warpringdmapxy[8] = {160,116,160,90,160,102,160,154};
int32_t warpringdmapxy[8] = {80,26,80,0,80,12,80,78};

static ListData number_list(numberlist, &font);
static ListData wc_list(wclist, &font);

static DIALOG warpring_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,          0,      0,    193,    166,    vc(14),                 vc(1),                   0,    D_EXIT,     0,          0,  NULL,                    NULL,   NULL            },
    { d_timer_proc,           0,      0,      0,      0,    0,                      0,                       0,    0,          0,          0,  NULL,                    NULL,   NULL            },
    { jwin_text_proc,        16,     33,     48,      8,    vc(14),                 vc(1),                   0,    0,          0,          0, (void *) "Count:",       NULL,   NULL            },
    { d_wclist_proc,         72,     29,     48,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,          1,          0, (void *) &wc_list,       NULL,   NULL            },
    // 4
    { d_warplist_proc,       16,     50,     65,     71,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    D_EXIT,     0,          0, (void *) &number_list,   NULL,   warpringdmapxy  },
    { jwin_button_proc,      26,    140,     61,     21,    vc(14),                 vc(1),                  13,    D_EXIT,     0,          0, (void *) "Edit",         NULL,   NULL            },
    { jwin_button_proc,     106,    140,     61,     21,    vc(14),                 vc(1),                  27,    D_EXIT,     0,          0, (void *) "Done",         NULL,   NULL            },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       0,    0,          KEY_F1,     0, (void *) onHelp,         NULL,   NULL            },
    { NULL,                   0,      0,      0,      0,    0,                      0,                       0,    0,          0,          0,  NULL,                    NULL,   NULL            }
};

int32_t select_warp()
{
    QMisc.warp[curr_ring].size = vbound(QMisc.warp[curr_ring].size,3,9);
    number_list_zero = false;
    
    int32_t ret=4;
    
    large_dialog(warpring_dlg);
        
    do
    {
        number_list_size = QMisc.warp[curr_ring].size;
        warpring_dlg[3].d1 = QMisc.warp[curr_ring].size-3;
        ret = do_zqdialog(warpring_dlg,ret);
    }
    while(ret==3);
    
    if(ret==6 || ret==0)
    {
        return -1;
    }
    
    return warpring_dlg[4].d1;
}

void EditWarpRing(int32_t ring)
{
    char buf[40];
    sprintf(buf,"Ring %d Warps",ring);
    warpring_dlg[0].dp = buf;
    warpring_dlg[0].dp2 = get_zc_font(font_lfont);
    curr_ring = ring;
    
    int32_t index = select_warp();
    
    while(index!=-1)
    {
        EditWarpRingScr(ring,index);
        index = select_warp();
    }
}

int32_t onWarpRings()
{
    number_list_size = 9;
    number_list_zero = true;
    
    int32_t index = select_data("Warp Rings",0,numberlist,"Edit","Done",get_zc_font(font_lfont));
    
    while(index!=-1)
    {
        EditWarpRing(index);
        number_list_size = 9;
        number_list_zero = true;
        index = select_data("Warp Rings",index,numberlist,"Edit","Done",get_zc_font(font_lfont));
    }
    
    return D_O_K;
}

const char *enemy_viewer(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size=10;
        
        return NULL;
    }
    
    int32_t guy=Map.CurrScr()->enemy[index];

    if (guy == 0) return "(None)";
    return guy>=eSTART ? guy_string[guy] : (char *) "(Guy - Do Not Use!)";
}

enemy_struct bie[eMAXGUYS];
enemy_struct ce[100];
int32_t enemy_type=0,bie_cnt=-1,ce_cnt;

enemy_struct big[zqMAXGUYS];
enemy_struct cg[100];
int32_t guy_type=0,big_cnt=-1,cg_cnt;

//Uses old_max_guys[] in zq_misc.cpp to define what are visible if bool hide is set true. -Z
void build_bie_list(bool hide)
{
    bie[0].s = (char *)"(None)";
    bie[0].i = 0;
    bie_cnt=1;
    
    for(int32_t i=1; i<eMAXGUYS; i++)
    {
        if (i < eSTART) continue; // ignore guys - enemies only!

        if(i >= OLDMAXGUYS || old_guy_string[i][strlen(old_guy_string[i])-1]!=' ' || !hide)
        {
            bie[bie_cnt].s = (char *)guy_string[i];
            bie[bie_cnt].i = i;
            ++bie_cnt;
        }
    }
    
    for(int32_t i=1; i<bie_cnt-1; i++) //Start at 1 so '(None)' isn't alphabetized!
    {
        for(int32_t j=i+1; j<bie_cnt; j++)
        {
            if(strcmp(bie[i].s,bie[j].s)>0)
            {
                zc_swap(bie[i],bie[j]);
            }
        }
    }
}

int32_t efrontfacingtile(int32_t id)
{
    int32_t anim = get_qr(qr_NEWENEMYTILES)?guysbuf[id].e_anim:guysbuf[id].anim;
    int32_t usetile = 0;
    
    switch(anim)
    {
	    
    case aNONE: break;
    case aAQUA:
        if(!(get_qr(qr_NEWENEMYTILES) && guysbuf[id].attributes[0]))
            break;
            
    case aWALLM:
    case aGHOMA:
        usetile=1;
        break;
        
        //Fallthrough
    case a2FRM4DIR:
    case aWALK:
        usetile=2;
        break;
        
    case aLEV:
    case a3FRM4DIR:
        usetile=3;
        break;
        
    case aLANM:
        usetile = !(get_qr(qr_NEWENEMYTILES))?0:4;
        break;
        
    case aNEWDONGO:
    case a4FRM8EYE:
    case aNEWWIZZ:
    case aARMOS4:
    case aNEWTEK:
    case aNEWWALLM:
    case a4FRM4DIRF:
    case a4FRM4DIR:
    case a4FRM8DIRF:
    case a4FRMPOS8DIR:
    case a4FRMPOS8DIRF:
    case a4FRMPOS4DIR:
    case a4FRMPOS4DIRF:
        usetile=4;
        break;
        
    case aDONGO:
        usetile=6;
        break;
        
    case aDONGOBS:
        usetile=24;
        break;
        
    case aNEWLEV:
        usetile=40;
        break;
        
    case aNEWZORA:
        if(guysbuf[id].family==eeZORA)
            usetile=44;
            
        break;
        
    case aGLEEOK:
        if(!get_qr(qr_NEWENEMYTILES))
            usetile = (guysbuf[id].s_tile - guysbuf[id].tile)+1;
        else
            usetile = (guysbuf[id].attributes[7]);
            
        break;
    }
    
    return zc_max(get_qr(qr_NEWENEMYTILES) ? -guysbuf[id].e_tile
                  : -guysbuf[id].tile, usetile);
}

int32_t onEnemies()
{
	call_screenenemies_dialog();
	refresh(rALL);
	return D_O_K;
}

char author[65],title[65],password[32];

int32_t d_showedit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_edit_proc(msg,d,c);
    
    if(msg==MSG_DRAW)
    {
        (d+1)->proc(MSG_DRAW,d+1,0);
    }
    
    return ret;
}

int32_t onHeader()
{
	call_header_dlg();
    return D_O_K;
}

void call_cheats_dlg();
int32_t onCheats()
{
	call_cheats_dlg();
	return D_O_K;
}

bool do_x_button(BITMAP *dest, int32_t x, int32_t y)
{
    bool over=false;
    
    while(gui_mouse_b())
    {
        custom_vsync();
        
        if(isinRect(gui_mouse_x(),gui_mouse_y(),x,y,x+15,y+13))
        {
            if(!over)
            {
                draw_x_button(dest, x, y, D_SELECTED);
                over=true;
            }
        }
        else
        {
            if(over)
            {
                draw_x_button(dest, x, y, 0);
                over=false;
            }
        }
    }
    
    return over;
}

bool do_question_button(BITMAP *dest, int32_t x, int32_t y)
{
    bool over=false;
    
    while(gui_mouse_b())
    {
        custom_vsync();
        
        if(isinRect(gui_mouse_x(),gui_mouse_y(),x,y,x+15,y+13))
        {
            if(!over)
            {
                draw_question_button(dest, x, y, D_SELECTED);
                over=true;
            }
        }
        else
        {
            if(over)
            {
                draw_question_button(dest, x, y, 0);
                over=false;
            }
        }
    }
    
    return over;
}


int32_t d_dummy_proc(int32_t,DIALOG *,int32_t)
{
	return D_O_K;
}

int32_t d_maptile_proc(int32_t msg, DIALOG *d, int32_t)
{
    switch(msg)
    {
    case MSG_CLICK:
        if(select_tile(d->d1,d->d2,1,d->fg,true, 0, true))
		return D_REDRAW;
	
    case MSG_DRAW:
    {
        int32_t dw = d->w;
        int32_t dh = d->h;
        
        if(d->dp2==(void*)1)
        {
            dw /= 2;
            dh /= 2;
        }
        
        BITMAP *buf = create_bitmap_ex(8,dw,dh);
        
        if(buf)
        {
            clear_bitmap(buf);
            
            for(int32_t y=0; y<dh; y+=16)
                for(int32_t x=0; x<dw; x+=16)
                {
                    if(d->d1)
                        puttile16(buf,d->d1+(y>>4)*20+(x>>4),x,y,d->fg,0);
                }
                
            if(d->dp2==(void*)1)
                stretch_blit(buf,screen,0,0,dw,dh,d->x-1,d->y-1,dw*2,dh*2);
            else
                blit(buf,screen,0,0,d->x,d->y,dw,dh);
                
            destroy_bitmap(buf);
        }
    }
    }
    
    return D_O_K;
}

static int32_t last_combo=0;
static int32_t last_cset=0;
static combo_alias temp_aliases[MAXCOMBOALIASES];

static char comboa_str_buf[32];

int32_t d_comboalist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t d1 = d->d1;
    int32_t ret = jwin_droplist_proc(msg,d,c);
    comboa_cnt = d->d1;
    
    if(d1!=d->d1)
    {
        set_comboaradio(temp_aliases[comboa_cnt].layermask);
        return D_REDRAW;
    }
    
    return ret;
}

const char *comboalist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXCOMBOALIASES-1);
        sprintf(comboa_str_buf,"%d",index);
        return comboa_str_buf;
    }
    
    *list_size=MAXCOMBOALIASES;
    return NULL;
}

extern int32_t scheme[jcMAX];

int32_t d_comboa_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    combo_alias *combo;
    combo = &temp_aliases[comboa_cnt];
    int32_t position;
    int32_t cur_layer, temp_layer;
    int32_t lay_count=0;
    int32_t size = 2;
    
    int32_t cx1=(gui_mouse_x()-d->x-(120-(combo->width*8)));
    int32_t cy1=(gui_mouse_y()-d->y-(80-(combo->height*8)));
    int32_t cx=cx1/(16*size);
    int32_t cy=cy1/(16*size);
    
    int32_t co,cs;
    
    
    switch(msg)
    {
    case MSG_CLICK:
        if((cx>combo->width)||(cx1<0))
            return D_O_K;
            
        if((cy>combo->height)||(cy1<0))
            return D_O_K;
            
        for(int32_t j=0; j<layer_cnt; j++)
        {
            if(combo->layermask&(1<<j))
                lay_count++;
        }
        
        position=(lay_count)*(combo->width+1)*(combo->height+1);
        position+=(cy*(combo->width+1))+cx;
        
        if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
        {
            combo->combos[position] = 0;
            combo->csets[position] = 0;
            
            while(gui_mouse_b())
            {
                /* do nothing */
                rest(1);
            }
            
            return D_REDRAW;
        }
        
        co=combo->combos[position];
        cs=combo->csets[position];
        
        if((co==0)||(key[KEY_ZC_LCONTROL]))
        {
            co=last_combo;
            cs=last_cset;
        }
        
        if((select_combo_2(co,cs)))
        {
            last_combo = co;
            last_cset = cs;
            
            combo->combos[position]=co;
            combo->csets[position]=cs;
        }
        
        return D_REDRAW;
        break;
        
    case MSG_DRAW:
        BITMAP *buf = create_bitmap_ex(8,d->w,d->h);
        
        if(buf)
        {
            clear_bitmap(buf);
            
            for(int32_t z=0; z<=comboa_lmasktotal(combo->layermask); z++)
            {
                int32_t k=0;
                cur_layer=0;
                temp_layer=combo->layermask;
                
                while((temp_layer!=0)&&(k<z))
                {
                    if(temp_layer&1)
                    {
                        k++;
                    }
                    
                    cur_layer++;
                    temp_layer = temp_layer>>1;
                }
                
                for(int32_t y=0; (y<d->h)&&((y/16)<=combo->height); y+=16)
                {
                    for(int32_t x=0; (x<d->w)&&((x/16)<=combo->width); x+=16)
                    {
                        int32_t cpos = (z*(combo->width+1)*(combo->height+1))+(((y/16)*(combo->width+1))+(x/16));
                        
                        if(combo->combos[cpos])
                        {
                            if(!((d-1)->flags&D_SELECTED)||(cur_layer==layer_cnt))
                            {
                                if(z==0)
                                {
                                    puttile16(buf,combobuf[combo->combos[cpos]].tile,x,y,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                                }
                                else
                                {
                                    overtile16(buf,combobuf[combo->combos[cpos]].tile,x,y,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                                }
                            }
                        }
                    }
                }
            }
            
            rectfill(screen, d->x-2,d->y-2,d->x+256+2,d->y+176+2,jwin_pal[jcBOX]);
            int32_t dx = 120-(combo->width*8)+d->x;
            int32_t dy = 80-(combo->height*8)+d->y;
            stretch_blit(buf,screen,0,0,(combo->width+1)*16,(combo->height+1)*16,dx,dy,(combo->width+1)*16*size,(combo->height+1)*16*size);
            //blit(buf,screen,0,0,120-(combo->width*8)+d->x,80-(combo->height*8)+d->y,(combo->width+1)*16,(combo->height+1)*16);
            (d-11)->w = (combo->width+1)*16*size+2;
            (d-11)->h = (combo->height+1)*16*size+2;
            (d-11)->x = 120-(combo->width*8)+4*size+2+(d-14)->x;
            (d-11)->y = 80-(combo->height*8)+25*size+2+(d-14)->y;
            object_message((d-11),MSG_DRAW,0);
            
            destroy_bitmap(buf);
        }
        
        break;
    }
    
    return D_O_K;
}

void draw_combo_alias_thumbnail(BITMAP *dest, combo_alias const* combo, int32_t x, int32_t y, int32_t size)
{
    if(!combo->combo)
    {
        int32_t cur_layer, temp_layer;
        
        int32_t cw=combo->width+1;
        int32_t ch=combo->height+1;
        int32_t dw=cw<<4;
        int32_t dh=ch<<4;
        int32_t sw=16, sh=16, sx=0, sy=0;
        
        if(cw<ch)
        {
            sw=((cw<<4)/ch);
            sx=((16-sw)>>1);
        }
        else
        {
            sh=((ch<<4)/cw);
            sy=((16-sh)>>1);
        }
        
        BITMAP *buf = create_bitmap_ex(8,dw,dh);
        BITMAP *buf2 = create_bitmap_ex(8, 16*size, 16*size);
        clear_bitmap(buf);
        clear_bitmap(buf2);
        
        if(buf&&(combo->width>0||combo->height>0||combo->combos[0]>0))
        {
            clear_bitmap(buf);
            
            for(int32_t z=0; z<=comboa_lmasktotal(combo->layermask); z++)
            {
                int32_t k=0;
                cur_layer=0;
                temp_layer=combo->layermask;
                
                while((temp_layer!=0)&&(k<z))
                {
                    if(temp_layer&1)
                    {
                        k++;
                    }
                    
                    cur_layer++;
                    temp_layer = temp_layer>>1;
                }
                
                for(int32_t y2=0; (y2<dh)&&((y2>>4)<=combo->height); y2+=16)
                {
                    for(int32_t x2=0; (x2<dw)&&((x2>>4)<=combo->width); x2+=16)
                    {
                        int32_t cpos = (z*(combo->width+1)*(combo->height+1))+(((y2/16)*(combo->width+1))+(x2/16));
                        
                        if(combo->combos[cpos])
                        {
                            if(z==0)
                            {
                                puttile16(buf,combobuf[combo->combos[cpos]].tile,x2,y2,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                            }
                            else
                            {
                                overtile16(buf,combobuf[combo->combos[cpos]].tile,x2,y2,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                            }
                        }
                    }
                }
            }
            
            stretch_blit(buf, buf2, 0, 0, (cw*16), (ch*16), sx*size, sy*size, sw*size, sh*size);
            blit(buf2, dest, 0, 0, x, y, 16*size, 16*size);
        }
        else
        {
            rectfill(dest,x,y,x+16*size-1,y+16*size-1,0);
            rectfill(dest,x+3*size,y+3*size,x+12*size,y+12*size,vc(4));
        }
        
        if(buf)
            destroy_bitmap(buf);
            
        if(buf2)
            destroy_bitmap(buf2);
    }
    else
    {
        if(combobuf[combo->combo].tile>0)
        {
            rectfill(dest,x,y,x+16*size-1,y+16*size-1,0);
            put_combo(dest, x, y, combo->combo, combo->cset, 0, 0, size);
        }
        else
        {
            rectfill(dest,x,y,x+16*size-1,y+16*size-1,0);
            rectfill(dest,x+3*size,y+3*size,x+12*size,y+12*size,vc(4));
        }
    }
}

int32_t d_comboat_proc(int32_t msg,DIALOG *d,int32_t)
{
    switch(msg)
    {
    case MSG_CLICK:
    {
        int32_t c2;
        int32_t cs;
        c2=temp_aliases[comboa_cnt].combo;
        cs=temp_aliases[comboa_cnt].cset;
        
        if(gui_mouse_b()&2)  //right mouse button
        {
            if(c2==0&&cs==0&&!(gui_mouse_b()&1))
            {
                return D_O_K;
            }
            
            temp_aliases[comboa_cnt].combo=0;
            temp_aliases[comboa_cnt].cset=0;
        }
        
        if(gui_mouse_b()&1)  //left mouse button
        {
            if(select_combo_2(c2, cs))
            {
                temp_aliases[comboa_cnt].combo=c2;
                temp_aliases[comboa_cnt].cset=cs;
            }
            
            return D_REDRAW;
        }
        else
        {
            return D_REDRAWME;
        }
    }
    break;
    
    case MSG_DRAW:
        draw_combo_alias_thumbnail(screen, &temp_aliases[comboa_cnt], d->x-1, d->y-1,2);
        break;
        
    default:
        break;
    }
    
    return D_O_K;
}

int32_t d_comboa_radio_proc(int32_t msg,DIALOG *d,int32_t c);

static DIALOG orgcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)    (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,         0,    0,   200,  161,   vc(14),   vc(1),       0,     D_EXIT,       0,             0, (void *) "Organize Combo Aliases", NULL, NULL },
    { jwin_button_proc,     27,   130,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     112,  130,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    
    { jwin_radio_proc,		10,	   40,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Copy", NULL, NULL },
    { jwin_text_proc,     10,   50,   33,		9,       0,       0,      0,       0,          0,             0, (void *) "", NULL, NULL },
    // { jwin_radio_proc,		10,	   50,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,			(void*) "Move", NULL, NULL },
    { jwin_radio_proc,		10,	   60,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Swap", NULL, NULL },
    /* 6 */  { jwin_edit_proc,      110,   35,   32,   16,    vc(12),  vc(1),  0,       0,          4,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      110,   55,   32,   16,    vc(12),  vc(1),  0,       0,          4,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     60,   40,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Source", NULL, NULL },
    { jwin_text_proc,     60,   60,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Dest", NULL, NULL},
    { jwin_radio_proc,		10,	   80,		60,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Insert new (before source)", NULL, NULL },
    { jwin_radio_proc,		10,	   100,		60,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Delete source", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static DIALOG newcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)    (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,         0,    0,   200,  161,   vc(14),   vc(1),       0,     D_EXIT,       0,             0, (void *) "Combo Alias Properties", NULL, NULL },
    { jwin_button_proc,     27,   130,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     112,  130,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,     24,   34,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Alias Width", NULL, NULL },
    { jwin_text_proc,     24,   52,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Alias Height", NULL, NULL },
    { jwin_text_proc,     24,   70,   100,   8,       0,       0,      0,       0,          0,             0, (void *) "Layers to Draw On:", NULL, NULL },
    { jwin_edit_proc,      104,   30,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      122,   48,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     24,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "1", NULL, NULL },
    { jwin_check_proc,     50,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "2", NULL, NULL },
    { jwin_check_proc,     76,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "3", NULL, NULL },
    { jwin_check_proc,     102,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "4", NULL, NULL },
    { jwin_check_proc,     128,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "5", NULL, NULL },
    { jwin_check_proc,     154,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "6", NULL, NULL },
    
    
    // { jwin_text_proc,     24,   106,   80,   8,       0,       0,      0,       0,          0,             0,       (void *) "Copy to :", NULL, NULL },
    //15
    // { jwin_edit_proc,      100,   100,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    // { jwin_check_proc,     84,   106,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0,       (void *) "", NULL, NULL },
    
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

bool swapComboAlias(int32_t source, int32_t dest)
{
    if(source==dest)
		return false;
	zc_swap(temp_aliases[source],temp_aliases[dest]);
    return true;
}


bool copyComboAlias(int32_t source, int32_t dest)
{
    if(source == dest)
		return false;
    temp_aliases[dest] = temp_aliases[source];
    return true;
}

int32_t getcurrentcomboalias();

int32_t onOrgComboAliases()
{
	char cSrc[8];
	char cDest[8];
	sprintf(cSrc,"%d", getcurrentcomboalias());
	strcpy(cDest,cSrc);
	int32_t iSrc = 0;
	int32_t iDest = 0;
	
	//sprintf(cSrc,"0");
	//sprintf(cDest,"0");
	orgcomboa_dlg[0].dp2=get_zc_font(font_lfont);
	orgcomboa_dlg[6].dp= cSrc;
	orgcomboa_dlg[7].dp= cDest;
	int32_t ret = 1;
	large_dialog(orgcomboa_dlg);
	do
	{
		iSrc = atoi((char*)orgcomboa_dlg[6].dp);
		iDest = atoi((char*)orgcomboa_dlg[7].dp);
		ret = do_zqdialog(orgcomboa_dlg,-1);
		
		if(ret!=1) return ret;
		
		if((atoi((char*) orgcomboa_dlg[6].dp))<0 || (atoi((char*) orgcomboa_dlg[6].dp)) > MAXCOMBOALIASES-1)
		{
			char buf[100];
			snprintf(buf, 100, "Invalid source (range 0-%d)", MAXCOMBOALIASES-1);
			buf[99]='\0';
			jwin_alert("Error",buf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			ret = 1;
		}
		
		// 10,11=ins, del
		if(orgcomboa_dlg[10].flags & D_SELECTED)  //insert
		{
			for(int32_t j=MAXCOMBOALIASES-1; j>(atoi((char*) orgcomboa_dlg[6].dp)); --j)
				copyComboAlias(j-1,j);
			ret = -1;
		}
		
		if(orgcomboa_dlg[11].flags & D_SELECTED)  //delete
		{
			for(int32_t j=(atoi((char*) orgcomboa_dlg[6].dp)); j<MAXCOMBOALIASES-1; ++j)
				copyComboAlias(j+1,j);
			ret = -1;
		}
		
		if((atoi((char*) orgcomboa_dlg[6].dp)) == (atoi((char*) orgcomboa_dlg[7].dp)))
		{
			jwin_alert("Error","Source and dest can't be the same.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			ret = 1;
		}
		
		if((atoi((char*) orgcomboa_dlg[7].dp)) < 0 || (atoi((char*) orgcomboa_dlg[7].dp)) > MAXCOMBOALIASES-1)
		{
			char buf[100];
			snprintf(buf, 100, "Invalid dest (range 0-%d)", MAXCOMBOALIASES-1);
			buf[99]='\0';
			
			jwin_alert("Error",buf,NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
			ret = 1;
		}
		
		if(orgcomboa_dlg[3].flags & D_SELECTED)  //copy
		{
			copyComboAlias((atoi((char*) orgcomboa_dlg[6].dp)),(atoi((char*) orgcomboa_dlg[7].dp)));
			ret = -1;
		}
		
		if(orgcomboa_dlg[5].flags & D_SELECTED)  //swap
		{
			swapComboAlias((atoi((char*) orgcomboa_dlg[6].dp)),(atoi((char*) orgcomboa_dlg[7].dp)));
			ret = -1;
		}
	}
	while(ret==1);
	return ret;
}

int32_t onNewComboAlias()
{
    combo_alias *combo;
    combo = &temp_aliases[comboa_cnt];
    
    char cwidth[5];
    char cheight[5];
    // char cp[3];
    
    word temp_combos[16*11*7];
    byte temp_csets[16*11*7];
    sprintf(cwidth, "%d", combo->width+1);
    sprintf(cheight, "%d", combo->height+1);
    int32_t old_count = (comboa_lmasktotal(combo->layermask)+1)*(combo->width+1)*(combo->height+1);
    int32_t old_width=combo->width;
    int32_t old_height=combo->height;
    int32_t oldlayer=combo->layermask;
    
    for(int32_t i=0; i<old_count; i++)
    {
        temp_csets[i] = combo->csets[i];
        temp_combos[i] = combo->combos[i];
    }
    
    newcomboa_dlg[0].dp2 = get_zc_font(font_lfont);
    newcomboa_dlg[6].dp = cwidth;
    newcomboa_dlg[7].dp = cheight;
    newcomboa_dlg[8].flags = (combo->layermask&1)? D_SELECTED : 0;
    newcomboa_dlg[9].flags = (combo->layermask&2)? D_SELECTED : 0;
    newcomboa_dlg[10].flags = (combo->layermask&4)? D_SELECTED : 0;
    newcomboa_dlg[11].flags = (combo->layermask&8)? D_SELECTED : 0;
    newcomboa_dlg[12].flags = (combo->layermask&16)? D_SELECTED : 0;
    newcomboa_dlg[13].flags = (combo->layermask&32)? D_SELECTED : 0;
    
    large_dialog(newcomboa_dlg);
    
    int32_t ret = do_zqdialog(newcomboa_dlg,-1);
    
    if(ret==1)
    {
        combo->width = ((atoi(cwidth)-1)<16)?zc_max(0,(atoi(cwidth)-1)):15;
        combo->height = ((atoi(cheight)-1)<11)?zc_max(0,(atoi(cheight)-1)):10;
        combo->layermask=0;
        combo->layermask |= (newcomboa_dlg[8].flags&D_SELECTED)?1:0;
        combo->layermask |= (newcomboa_dlg[9].flags&D_SELECTED)?2:0;
        combo->layermask |= (newcomboa_dlg[10].flags&D_SELECTED)?4:0;
        combo->layermask |= (newcomboa_dlg[11].flags&D_SELECTED)?8:0;
        combo->layermask |= (newcomboa_dlg[12].flags&D_SELECTED)?16:0;
        combo->layermask |= (newcomboa_dlg[13].flags&D_SELECTED)?32:0;
        
        int32_t new_count = (comboa_lmasktotal(combo->layermask)+1)*(combo->width+1)*(combo->height+1);
        
        combo->combos.clear();
        combo->csets.clear();
        
        int32_t j=1;
        int32_t old_size=(old_width+1)*(old_height+1);
        int32_t new_start[7] =
        {
            0,
            ((combo->width+1)*(combo->height+1)*(1)),
            ((combo->width+1)*(combo->height+1)*(2)),
            ((combo->width+1)*(combo->height+1)*(3)),
            ((combo->width+1)*(combo->height+1)*(4)),
            ((combo->width+1)*(combo->height+1)*(5)),
            ((combo->width+1)*(combo->height+1)*(6))
        };
        int32_t new_layers[6] = {0,0,0,0,0,0};
        int32_t temp_layer = combo->layermask;
        int32_t temp_old = oldlayer;
        int32_t old_layers[6] = {0,0,0,0,0,0};
        int32_t k=1;
        
        for(int32_t i=0; (i<6)&&(temp_layer!=0); j++,temp_layer>>=1,temp_old>>=1)
        {
            if(temp_layer&1)
            {
                new_layers[i] = j;
                //if(oldlayer&(1<<(j-1))) old_layers[i] = k++;
                i++;
            }
            
            if(temp_old&1)
            {
                if(temp_layer&1)
                {
                    old_layers[i-1] = k;
                }
                
                k++;
            }
        }
        
        for(int32_t i=0; i<new_count; i++)
        {
            if(i>=new_start[6])
            {
                //oldl=oldlayer>>(new_layers[5]-1);
                j=i-new_start[6];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[5]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[5])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[5])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[5])
            {
                //oldl=oldlayer>>(new_layers[4]-1);
                j=i-new_start[5];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[4]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[4])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[4])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[4])
            {
                //oldl=oldlayer>>(new_layers[3]-1);
                j=i-new_start[4];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[3]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[3])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[3])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[3])
            {
                //oldl=oldlayer>>(new_layers[2]-1);
                j=i-new_start[3];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[2]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[2])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[2])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[2])
            {
                //oldl=oldlayer>>(new_layers[1]-1);
                j=i-new_start[2];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[1]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[1])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[1])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[1])
            {
                //oldl=oldlayer>>(new_layers[0]-1);
                j=i-new_start[1];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[0]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[0])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[0])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[0])
            {
                if(((i/(combo->width+1))<=old_height)&&((i%(combo->width+1))<=old_width))
                {
                    combo->combos[i] = temp_combos[(i%(combo->width+1))+((old_width+1)*(i/(combo->width+1)))];
                    combo->csets[i] = temp_csets[(i%(combo->width+1))+((old_width+1)*(i/(combo->width+1)))];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
        }
        
        set_comboaradio(combo->layermask);
    }
    
    return ret;
}

int32_t d_orgcomboa_proc(int32_t msg, DIALOG *d, int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int32_t down=0;
    int32_t selected=(d->flags&D_SELECTED)?1:0;
    int32_t last_draw;
    
    switch(msg)
    {
    
    case MSG_DRAW:
    {
        FONT *tfont=font;
        font=get_zc_font(font_lfont_l);
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, true);
        font=tfont;
    }
    break;
    
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_KEY:
        /* close dialog? */
        onOrgComboAliases();
        return D_REDRAW;
        
        /* or just toggle */
        /*d->flags ^= D_SELECTED;
        object_message(d, MSG_DRAW, 0);
        break;*/
        
    case MSG_CLICK:
        last_draw = 0;
        
        /* track the mouse until it is released */
        while(gui_mouse_b())
        {
            down = mouse_in_rect(d->x, d->y, d->w, d->h);
            
            /* redraw? */
            if(last_draw != down)
            {
                if(down != selected)
                    d->flags |= D_SELECTED;
                else
                    d->flags &= ~D_SELECTED;
                    
                object_message(d, MSG_DRAW, 0);
                last_draw = down;
            }
            
            /* let other objects continue to animate */
            broadcast_dialog_message(MSG_IDLE, 0);
        }
        
        /* redraw in normal state */
        if(down)
        {
            if(d->flags&D_EXIT)
            {
                d->flags &= ~D_SELECTED;
                object_message(d, MSG_DRAW, 0);
            }
        }
        
        /* should we close the dialog? */
        if(down)
        {
            onOrgComboAliases();
            return D_REDRAW;
        }
        
        break;
    }
    
    return D_O_K;
}

int32_t d_comboabutton_proc(int32_t msg, DIALOG *d, int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int32_t down=0;
    int32_t selected=(d->flags&D_SELECTED)?1:0;
    int32_t last_draw;
    
    switch(msg)
    {
    
    case MSG_DRAW:
    {
        FONT *tfont=font;
        font=get_zc_font(font_lfont_l);
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, true);
        font=tfont;
    }
    break;
    
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_KEY:
        /* close dialog? */
        onNewComboAlias();
        return D_REDRAW;
        
        /* or just toggle */
        /*d->flags ^= D_SELECTED;
        object_message(d, MSG_DRAW, 0);
        break;*/
        
    case MSG_CLICK:
        last_draw = 0;
        
        /* track the mouse until it is released */
        while(gui_mouse_b())
        {
            down = mouse_in_rect(d->x, d->y, d->w, d->h);
            
            /* redraw? */
            if(last_draw != down)
            {
                if(down != selected)
                    d->flags |= D_SELECTED;
                else
                    d->flags &= ~D_SELECTED;
                    
                object_message(d, MSG_DRAW, 0);
                last_draw = down;
            }
            
            /* let other objects continue to animate */
            broadcast_dialog_message(MSG_IDLE, 0);
        }
        
        /* redraw in normal state */
        if(down)
        {
            if(d->flags&D_EXIT)
            {
                d->flags &= ~D_SELECTED;
                object_message(d, MSG_DRAW, 0);
            }
        }
        
        /* should we close the dialog? */
        if(down)
        {
            onNewComboAlias();
            return D_REDRAW;
        }
        
        break;
    }
    
    return D_O_K;
}

int32_t d_comboacheck_proc(int32_t msg, DIALOG *d, int32_t c)
{
    int32_t temp = d->flags&D_SELECTED;
    int32_t ret=jwin_checkfont_proc(msg,d,c);
    
    if(temp != (d->flags&D_SELECTED))
    {
        return D_REDRAW;
    }
    
    return ret;
}

static ListData comboa_list(comboalist, &font);

static DIALOG editcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)   (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,        0,    0,  320,  240,  vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Combo Alias Edit", NULL, NULL },
    { jwin_button_proc,     148,  212,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     232,  212,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_frame_proc,      4+121,   28+81,   1,   1,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_comboabutton_proc,   25,  212,  81,   21,   vc(14),  vc(1),  'p',     D_EXIT,     0,             0, (void *) "&Properties", NULL, NULL },
    { d_dummy_proc,         0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL },
    { d_comboa_radio_proc,  285,   44,  30,   8+1,    vc(14),  vc(1),  0,       D_SELECTED,          0,             0, (void *) "0", NULL, NULL },
    { d_comboa_radio_proc,  285,   54,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { d_comboa_radio_proc,  285,   64,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "2", NULL, NULL },
    { d_comboa_radio_proc,  285,   74,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "3", NULL, NULL },
    { d_comboa_radio_proc,  285,   84,   30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "4", NULL, NULL },
    
    { d_comboa_radio_proc,  285,   94,   30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "5", NULL, NULL },
    { d_comboa_radio_proc,  285,   104,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "6", NULL, NULL },
    { d_comboacheck_proc,     285,   164,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,      NULL, NULL, NULL },
    { d_comboa_proc,         6,    27,  256,  176,  0,   0,      0,       0,     0,             0,       NULL, NULL, NULL },
    { jwin_ctext_proc,     290,   176,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Only Show", NULL, NULL },
    { jwin_ctext_proc,     290,   186,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Current", NULL, NULL },
    { jwin_ctext_proc,     290,   196,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Layer", NULL, NULL },
    { jwin_ctext_proc,     290,   122,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Thumbnail", NULL, NULL },
    { jwin_frame_proc,     280,   132,   20,   20,  0,        0,      0,       0,         FR_DEEP,       0,       NULL, NULL, NULL },
    { d_comboat_proc,      282,   134,   16,   16,  0,   0,      0,       0,     0,             0,       NULL, NULL, NULL },
    
    //21
    { d_orgcomboa_proc,   106,  212,  21,   21,   vc(14),  vc(1),  'p',     D_EXIT,     0,             0, (void *) "&Org", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t getcurrentcomboalias()
{
    return editcomboa_dlg[5].d1;
}

int32_t d_comboa_radio_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t temp = layer_cnt;
    int32_t ret = jwin_radiofont_proc(msg,d,c);
    
    if(editcomboa_dlg[6].flags&D_SELECTED) layer_cnt=0;
    else if(editcomboa_dlg[7].flags&D_SELECTED) layer_cnt=1;
    else if(editcomboa_dlg[8].flags&D_SELECTED) layer_cnt=2;
    else if(editcomboa_dlg[9].flags&D_SELECTED) layer_cnt=3;
    else if(editcomboa_dlg[10].flags&D_SELECTED) layer_cnt=4;
    else if(editcomboa_dlg[11].flags&D_SELECTED) layer_cnt=5;
    else if(editcomboa_dlg[12].flags&D_SELECTED) layer_cnt=6;
    
    if(temp != layer_cnt)
    {
        return D_REDRAW;
    }
    
    return ret;
}

int32_t set_comboaradio(byte layermask)
{
    if(editcomboa_dlg[7].flags&D_SELECTED) editcomboa_dlg[7].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[8].flags&D_SELECTED) editcomboa_dlg[8].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[9].flags&D_SELECTED) editcomboa_dlg[9].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[10].flags&D_SELECTED) editcomboa_dlg[10].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[11].flags&D_SELECTED) editcomboa_dlg[11].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[12].flags&D_SELECTED) editcomboa_dlg[12].flags &= ~D_SELECTED;
    
    if(!(layermask&1)) editcomboa_dlg[7].flags |= D_DISABLED;
    else editcomboa_dlg[7].flags &= ~D_DISABLED;
    
    if(!(layermask&2)) editcomboa_dlg[8].flags |= D_DISABLED;
    else editcomboa_dlg[8].flags &= ~D_DISABLED;
    
    if(!(layermask&4)) editcomboa_dlg[9].flags |= D_DISABLED;
    else editcomboa_dlg[9].flags &= ~D_DISABLED;
    
    if(!(layermask&8)) editcomboa_dlg[10].flags |= D_DISABLED;
    else editcomboa_dlg[10].flags &= ~D_DISABLED;
    
    if(!(layermask&16)) editcomboa_dlg[11].flags |= D_DISABLED;
    else editcomboa_dlg[11].flags &= ~D_DISABLED;
    
    if(!(layermask&32)) editcomboa_dlg[12].flags |= D_DISABLED;
    else editcomboa_dlg[12].flags &= ~D_DISABLED;
    
    editcomboa_dlg[6].flags |= D_SELECTED;
    layer_cnt=0;
    return 1;
}

int32_t onEditComboPool()
{
	call_cpool_dlg(combo_pool_pos);
	return D_O_K;
}
int32_t onEditAutoCombo()
{
	call_autocombo_dlg(combo_auto_pos);
	return D_O_K;
}
int32_t onEditComboAlias()
{
	comboa_cnt = combo_apos;
	reset_combo_animations();
	reset_combo_animations2();
	
	for(int32_t i=0; i<MAXCOMBOALIASES; i++)
		temp_aliases[i] = combo_aliases[i];
	
	editcomboa_dlg[0].dp2 = get_zc_font(font_lfont);
	set_comboaradio(temp_aliases[comboa_cnt].layermask);
	editcomboa_dlg[5].d1 = comboa_cnt;
	
	bool small_d1 = editcomboa_dlg[0].d1==0;
	large_dialog(editcomboa_dlg,2);
	
	if(small_d1)
	{
		for(int32_t i=6; i<=12; i++)
		{
			editcomboa_dlg[i].w=30*1.5;
			editcomboa_dlg[i].h=9*1.5;
		}
		
		editcomboa_dlg[13].w=17*1.5;
		editcomboa_dlg[13].h=9*1.5;
		editcomboa_dlg[4].w=81*1.5;
		editcomboa_dlg[4].h=21*1.5;
		editcomboa_dlg[4].dp2=get_zc_font(font_lfont_l);
		editcomboa_dlg[21].w=21*1.5;
		editcomboa_dlg[21].h=21*1.5;
		editcomboa_dlg[21].dp2=get_zc_font(font_lfont_l);
	}
	
	int32_t ret=do_zqdialog(editcomboa_dlg,-1);
	
	if(ret==1)
	{
		saved=false;
		
		for(int32_t i=0; i<MAXCOMBOALIASES; i++)
			combo_aliases[i] = temp_aliases[i];
	}
	
	setup_combo_animations();
	setup_combo_animations2();
	return D_O_K;
}
void call_calias_dlg(int index)
{
	combo_apos = comboa_cnt = index;
	onEditComboAlias();
}

static char ffcombo_str_buf[MAXFFCS];
static char fflink_str_buf[MAXFFCS];

BITMAP* ffcur;

const char *ffcombolist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXFFCS-1);
        sprintf(ffcombo_str_buf,"%d",index+1);
        return ffcombo_str_buf;
    }
    
    *list_size=MAXFFCS;
    return NULL;
}

static ListData ffcombo_list(ffcombolist, &font);

static DIALOG ffcombo_sel_dlg[] =
{
    { jwin_win_proc,        0,    0,  200,   179,  vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Choose Freeform Combo", NULL, NULL },
    { jwin_button_proc,     35,   152,   61,   21,  vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,    104,   152,   61,   21,  vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_ffcombolist_proc,  11,   24,   49,   16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &ffcombo_list, NULL, NULL },
    { d_comboframe_proc,   68,  23,   20,   20,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_bitmap_proc,     70,  25,   16,   16,   0,       0,      0,       0,             0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t d_ffcombolist_proc(int32_t msg,DIALOG *d,int32_t c)
{
    int32_t ret = jwin_droplist_proc(msg,d,c);
    int32_t d1 = d->d1;
    int32_t x=ffcombo_sel_dlg[0].x;
    int32_t y=ffcombo_sel_dlg[0].y;
    FONT *tempfont=(font);
    int32_t x2=text_length(tempfont, "Move Delay:")+4;
    
    switch(msg)
    {
    case MSG_DRAW:
        if(!ffcur) return D_O_K;
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        
        if(buf)
        {
            clear_bitmap(buf);
            putcombo(buf,0,0,Map.CurrScr()->ffcs[d1].data,Map.CurrScr()->ffcs[d1].cset);
            stretch_blit(buf, ffcur, 0,0, 16, 16, 0, 0, ffcur->w, ffcur->h);
            destroy_bitmap(buf);
        }
        
        object_message(&ffcombo_sel_dlg[5],MSG_DRAW,0);
        
        int32_t xd = x+int32_t(68*1.5);
        int32_t y2 = y+int32_t(55*1.5);
        int32_t yd = 9;
        
        rectfill(screen,xd,y2,xd+x2+100,y2+yd*14,jwin_pal[jcBOX]);
        
        textprintf_ex(screen,tempfont,xd,y2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo #:");
        textprintf_ex(screen,tempfont,xd+x2,y2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffcs[d1].data);
        
        textprintf_ex(screen,tempfont,xd,y2+yd,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"CSet #:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffcs[d1].cset);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Pos:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].x.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*3,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Pos:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*3,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].y.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*4,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Speed:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*4,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].vx.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*5,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Speed:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*5,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].vy.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*6,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Accel:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*6,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].ax.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*7,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Accel:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*7,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffcs[d1].ay.getFloat());
        
        textprintf_ex(screen,tempfont,xd,y2+yd*8,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Linked To:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*8,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffcs[d1].link);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*9,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Move Delay:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*9,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffcs[d1].delay);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*10,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo W:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*10,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffEffectWidth(d1)));
        
        textprintf_ex(screen,tempfont,xd,y2+yd*11,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo H:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*11,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffEffectHeight(d1)));
        
        textprintf_ex(screen,tempfont,xd,y2+yd*12,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Tile W:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*12,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffTileWidth(d1)));
        
        textprintf_ex(screen,tempfont,xd,y2+yd*13,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Tile H:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*13,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffTileHeight(d1)));
        
        break;
    }
    
    return ret;
}
int32_t onSelectFFCombo()
{
    ffcombo_sel_dlg[0].dp2 = get_zc_font(font_lfont);
    ffcombo_sel_dlg[3].d1 = ff_combo;
    ffcur = create_bitmap_ex(8,32,32);
    
    if(!ffcur) return D_O_K;
    
	Map.CurrScr()->ensureFFC(MAXFFCS - 1);
    putcombo(ffcur,0,0,Map.CurrScr()->ffcs[ff_combo].data,Map.CurrScr()->ffcs[ff_combo].cset);
    ffcombo_sel_dlg[5].dp = ffcur;
    
	bool resize = !(ffcombo_sel_dlg[0].d1);
	large_dialog(ffcombo_sel_dlg);
	
	if(resize)
	{
		ffcombo_sel_dlg[5].x--;
		ffcombo_sel_dlg[5].y--;
	}
    
    int32_t ret=do_zqdialog(ffcombo_sel_dlg,0);
    
    while(ret==1)
    {
        ff_combo = ffcombo_sel_dlg[3].d1;
        mapscr* scr = active_visible_screen ? active_visible_screen->scr : Map.CurrScr();
        int screen = active_visible_screen ? active_visible_screen->screen : Map.getCurrScr();
        call_ffc_dialog(ff_combo, scr, screen);
        ret=do_zqdialog(ffcombo_sel_dlg,0);
    }
    
    destroy_bitmap(ffcur);
    return D_O_K;
}

const char *globalscriptlist(int32_t index, int32_t *list_size);
static ListData globalscript_list(globalscriptlist, &font);
const char *playerscriptlist(int32_t index, int32_t *list_size);
static ListData playerscript_list(playerscriptlist, &font);

const char *ffscriptlist(int32_t index, int32_t *list_size);

static ListData ffscript_list(ffscriptlist, &font);

char *strip_decimals(char *string)
{
    int32_t len=(int32_t)strlen(string);
    char *src=(char *)malloc(len+1);
    char *tmpsrc=src;
    memcpy(src,string,len+1);
    memset(src,0,len+1);
    
    for(size_t i=0; string[i]&&i<=strlen(string); i++)
    {
        *tmpsrc=string[i];
        
        if(*tmpsrc=='.')
        {
            while(string[i+1]=='.'&&i<=strlen(string))
            {
                i++;
            }
        }
        
        tmpsrc++;
    }
    
    memcpy(string,src,len);
    free(src);
    return string;
}

// Unused??? -L 6/6/11
char *clean_numeric_string(char *string)
{
    bool found_sign=false;
    bool found_decimal=false;
    int32_t len=(int32_t)strlen(string);
    char *src=(char *)malloc(len+1);
    char *tmpsrc=src;
    memcpy(src,string,len+1);
    memset(src,0,len+1);
    
    // strip out non-numerical characters
    for(size_t i=0; string[i]&&i<=strlen(string); i++)
    {
        *tmpsrc=string[i];
        
        if(*tmpsrc!='.'&&*tmpsrc!='-'&&*tmpsrc!='+'&&!isdigit(*tmpsrc))
        {
            while(*tmpsrc!='.'&&*tmpsrc!='-'&&*tmpsrc!='+'&&!isdigit(*tmpsrc))
            {
                i++;
            }
        }
        
        tmpsrc++;
    }
    
    len=(int32_t)strlen(src);
    char *src2=(char *)malloc(len+1);
    tmpsrc=src2;
    memcpy(src,src2,len+1);
    memset(src2,0,len+1);
    
    // second purge
    for(size_t i=0; src[i]&&i<=strlen(src); i++)
    {
        *tmpsrc=src[i];
        
        if(*tmpsrc=='-'||*tmpsrc=='+')
        {
            if(found_sign||found_decimal)
            {
                while(*tmpsrc=='-'||*tmpsrc=='+')
                {
                    i++;
                }
            }
            
            found_sign=true;
        }
        
        if(*tmpsrc=='.')
        {
            if(found_decimal)
            {
                while(*tmpsrc=='.')
                {
                    i++;
                }
            }
            
            found_decimal=true;
        }
        
        tmpsrc++;
    }
    
    sprintf(string, "%s", src2);
    free(src);
    free(src2);
    return string;
}

script_struct biglobal[NUMSCRIPTGLOBAL+1]; //global script
int32_t biglobal_cnt = -1;
script_struct biffs[NUMSCRIPTFFC]; //ff script
int32_t biffs_cnt = -1;
script_struct biitems[NUMSCRIPTITEM]; //item script
int32_t biitems_cnt = -1;
script_struct binpcs[NUMSCRIPTGUYS]; //npc script
int32_t binpcs_cnt = -1;

script_struct bilweapons[NUMSCRIPTWEAPONS]; //lweapon script
int32_t bilweapons_cnt = -1;

script_struct bieweapons[NUMSCRIPTWEAPONS]; //eweapon script
int32_t bieweapons_cnt = -1;

script_struct bihero[NUMSCRIPTHERO]; //link script
int32_t bihero_cnt = -1;

script_struct biscreens[NUMSCRIPTSCREEN]; //screen (screendata) script
int32_t biscreens_cnt = -1;

script_struct bidmaps[NUMSCRIPTSDMAP]; //dmap (dmapdata) script
int32_t bidmaps_cnt = -1;

script_struct biditemsprites[NUMSCRIPTSITEMSPRITE]; //dmap (dmapdata) script
int32_t biitemsprites_cnt = -1;

script_struct bidcomboscripts[NUMSCRIPTSCOMBODATA]; //dmap (dmapdata) script
int32_t bidcomboscripts_cnt = -1;
//static char ffscript_str_buf[32];

void build_biglobal_list()
{
    biglobal[0].first = "(None)";
    biglobal[0].second = -1;
    biglobal_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTGLOBAL; ++i)
    {
        if(globalmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << globalmap[i].scriptname << " (" << i << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        biglobal[biglobal_cnt].first = ss.str();
        biglobal[biglobal_cnt].second = i;
        ++biglobal_cnt;
    }
    
    // Blank out the rest of the list
    for(int32_t i=biglobal_cnt; i<NUMSCRIPTGLOBAL; ++i)
    {
        biglobal[i].first="";
        biglobal[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < biglobal_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < biglobal_cnt; j++)
        {
            if(stricmp(biglobal[i].first.c_str(),biglobal[j].first.c_str()) > 0 && strcmp(biglobal[j].first.c_str(),""))
                zc_swap(biglobal[i],biglobal[j]);
        }
    }
    
    biglobal_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTGLOBAL+1; ++i)
        if(biglobal[i].first.length() > 0)
            biglobal_cnt = i+1;
}

void build_biffs_list()
{
    biffs[0].first = "(None)";
    biffs[0].second = -1;
    biffs_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTFFC - 1; i++)
    {
        if(ffcmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << ffcmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        biffs[biffs_cnt].first = ss.str();
        biffs[biffs_cnt].second = i;
        biffs_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=biffs_cnt; i<NUMSCRIPTFFC; i++)
    {
        biffs[i].first="";
        biffs[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < biffs_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < biffs_cnt; j++)
        {
            if(stricmp(biffs[i].first.c_str(),biffs[j].first.c_str()) > 0 && strcmp(biffs[j].first.c_str(),""))
                zc_swap(biffs[i],biffs[j]);
        }
    }
    
    biffs_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTFFC; i++)
        if(biffs[i].first.length() > 0)
            biffs_cnt = i+1;
}

//npc scripts
void build_binpcs_list()
{
    binpcs[0].first = "(None)";
    binpcs[0].second = -1;
    binpcs_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTGUYS - 1; i++)
    {
        if(npcmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << npcmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        binpcs[binpcs_cnt].first = ss.str();
        binpcs[binpcs_cnt].second = i;
        binpcs_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=binpcs_cnt; i<NUMSCRIPTGUYS; i++)
    {
        binpcs[i].first="";
        binpcs[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < binpcs_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < binpcs_cnt; j++)
        {
            if(stricmp(binpcs[i].first.c_str(),binpcs[j].first.c_str()) > 0 && strcmp(binpcs[j].first.c_str(),""))
                zc_swap(binpcs[i],binpcs[j]);
        }
    }
    
    binpcs_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTGUYS; i++)
        if(binpcs[i].first.length() > 0)
            binpcs_cnt = i+1;
}


//lweapon scripts
void build_bilweapons_list()
{
    bilweapons[0].first = "(None)";
    bilweapons[0].second = -1;
    bilweapons_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTWEAPONS - 1; i++)
    {
        if(lwpnmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << lwpnmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        bilweapons[bilweapons_cnt].first = ss.str();
        bilweapons[bilweapons_cnt].second = i;
        bilweapons_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=bilweapons_cnt; i<NUMSCRIPTWEAPONS; i++)
    {
        bilweapons[i].first="";
        bilweapons[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < bilweapons_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < bilweapons_cnt; j++)
        {
            if(stricmp(bilweapons[i].first.c_str(),bilweapons[j].first.c_str()) > 0 && strcmp(bilweapons[j].first.c_str(),""))
                zc_swap(bilweapons[i],bilweapons[j]);
        }
    }
    
    bilweapons_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
        if(bilweapons[i].first.length() > 0)
            bilweapons_cnt = i+1;
}

//eweapon scripts
void build_bieweapons_list()
{
    bieweapons[0].first = "(None)";
    bieweapons[0].second = -1;
    bieweapons_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTWEAPONS - 1; i++)
    {
        if(ewpnmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << ewpnmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        bieweapons[bieweapons_cnt].first = ss.str();
        bieweapons[bieweapons_cnt].second = i;
        bieweapons_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=bieweapons_cnt; i<NUMSCRIPTWEAPONS; i++)
    {
        bieweapons[i].first="";
        bieweapons[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < bieweapons_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < bieweapons_cnt; j++)
        {
            if(stricmp(bieweapons[i].first.c_str(),bieweapons[j].first.c_str()) > 0 && strcmp(bieweapons[j].first.c_str(),""))
                zc_swap(bieweapons[i],bieweapons[j]);
        }
    }
    
    bieweapons_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
        if(bieweapons[i].first.length() > 0)
            bieweapons_cnt = i+1;
}

//player scripts
void build_bihero_list()
{
    bihero[0].first = "(None)";
    bihero[0].second = -1;
    bihero_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTHERO - 1; i++)
    {
        if(playermap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << playermap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        bihero[bihero_cnt].first = ss.str();
        bihero[bihero_cnt].second = i;
        bihero_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=bihero_cnt; i<NUMSCRIPTHERO; i++)
    {
        bihero[i].first="";
        bihero[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < bihero_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < bihero_cnt; j++)
        {
            if(stricmp(bihero[i].first.c_str(),bihero[j].first.c_str()) > 0 && strcmp(bihero[j].first.c_str(),""))
                zc_swap(bihero[i],bihero[j]);
        }
    }
    
    bihero_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTHERO; i++)
        if(bihero[i].first.length() > 0)
            bihero_cnt = i+1;
}

//dmap scripts
void build_bidmaps_list()
{
    bidmaps[0].first = "(None)";
    bidmaps[0].second = -1;
    bidmaps_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTSDMAP - 1; i++)
    {
        if(dmapmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << dmapmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        bidmaps[bidmaps_cnt].first = ss.str();
        bidmaps[bidmaps_cnt].second = i;
        bidmaps_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=bidmaps_cnt; i<NUMSCRIPTSDMAP; i++)
    {
        bidmaps[i].first="";
        bidmaps[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < bidmaps_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < bidmaps_cnt; j++)
        {
            if(stricmp(bidmaps[i].first.c_str(),bidmaps[j].first.c_str()) > 0 && strcmp(bidmaps[j].first.c_str(),""))
                zc_swap(bidmaps[i],bidmaps[j]);
        }
    }
    
    bidmaps_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTSDMAP; i++)
        if(bidmaps[i].first.length() > 0)
            bidmaps_cnt = i+1;
}

//screen scripts
void build_biscreens_list()
{
    biscreens[0].first = "(None)";
    biscreens[0].second = -1;
    biscreens_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTSCREEN - 1; i++)
    {
        if(screenmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << screenmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        biscreens[biscreens_cnt].first = ss.str();
        biscreens[biscreens_cnt].second = i;
        biscreens_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=biscreens_cnt; i<NUMSCRIPTSCREEN; i++)
    {
        biscreens[i].first="";
        biscreens[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < biscreens_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < biscreens_cnt; j++)
        {
            if(stricmp(biscreens[i].first.c_str(),biscreens[j].first.c_str()) > 0 && strcmp(biscreens[j].first.c_str(),""))
                zc_swap(biscreens[i],biscreens[j]);
        }
    }
    
    biscreens_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTSCREEN; i++)
        if(biscreens[i].first.length() > 0)
            biscreens_cnt = i+1;
}

//screen scripts
void build_biitemsprites_list()
{
    biditemsprites[0].first = "(None)";
    biditemsprites[0].second = -1;
    biitemsprites_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTSITEMSPRITE - 1; i++)
    {
        if(itemspritemap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << itemspritemap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        biditemsprites[biitemsprites_cnt].first = ss.str();
        biditemsprites[biitemsprites_cnt].second = i;
        biitemsprites_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=biitemsprites_cnt; i<NUMSCRIPTSITEMSPRITE; i++)
    {
        biditemsprites[i].first="";
        biditemsprites[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < biitemsprites_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < biitemsprites_cnt; j++)
        {
            if(stricmp(biditemsprites[i].first.c_str(),biditemsprites[j].first.c_str()) > 0 && strcmp(biditemsprites[j].first.c_str(),""))
                zc_swap(biditemsprites[i],biditemsprites[j]);
        }
    }
    
    biitemsprites_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTSITEMSPRITE; i++)
        if(biditemsprites[i].first.length() > 0)
            biitemsprites_cnt = i+1;
}

void build_biitems_list()
{
    biitems[0].first = "(None)";
    biitems[0].second = -1;
    biitems_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTITEM - 1; i++, biitems_cnt++)
    {
        stringstream ss;
        
        if(!itemmap[i].isEmpty())
            ss << itemmap[i].scriptname << " (" << i+1 << ")";
            
        biitems[biitems_cnt].first = ss.str();
        biitems[biitems_cnt].second = i;
    }
    
    for(int32_t i = 0; i < biitems_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < biitems_cnt; j++)
        {
            if(stricmp(biitems[i].first.c_str(), biitems[j].first.c_str()) > 0 && strcmp(biitems[j].first.c_str(),""))
                zc_swap(biitems[i], biitems[j]);
        }
    }
    
    biitems_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTITEM; i++)
        if(biitems[i].first.length() > 0)
            biitems_cnt = i+1;
}


//dmap scripts
void build_bidcomboscripts_list()
{
    bidcomboscripts[0].first = "(None)";
    bidcomboscripts[0].second = -1;
    bidcomboscripts_cnt = 1;
    
    for(int32_t i = 0; i < NUMSCRIPTSCOMBODATA - 1; i++)
    {
        if(comboscriptmap[i].scriptname.length()==0)
            continue;
            
        stringstream ss;
        ss << comboscriptmap[i].scriptname << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        bidcomboscripts[bidcomboscripts_cnt].first = ss.str();
        bidcomboscripts[bidcomboscripts_cnt].second = i;
        bidcomboscripts_cnt++;
    }
    
    // Blank out the rest of the list
    for(int32_t i=bidcomboscripts_cnt; i<NUMSCRIPTSCOMBODATA; i++)
    {
        bidcomboscripts[i].first="";
        bidcomboscripts[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int32_t i = 0; i < bidcomboscripts_cnt - 1; i++)
    {
        for(int32_t j = i + 1; j < bidcomboscripts_cnt; j++)
        {
            if(stricmp(bidcomboscripts[i].first.c_str(),bidcomboscripts[j].first.c_str()) > 0 && strcmp(bidcomboscripts[j].first.c_str(),""))
                zc_swap(bidcomboscripts[i],bidcomboscripts[j]);
        }
    }
    
    bidcomboscripts_cnt = 0;
    
    for(int32_t i = 0; i < NUMSCRIPTSCOMBODATA; i++)
        if(bidcomboscripts[i].first.length() > 0)
            bidcomboscripts_cnt = i+1;
}


const char *globalscriptlist(int32_t index, int32_t *list_size)
{
    if(index < 0)
    {
        *list_size = biglobal_cnt;
        return NULL;
    }
    
    return biglobal[index].first.c_str();
}

const char *ffscriptlist(int32_t index, int32_t *list_size)
{
    if(index < 0)
    {
        *list_size = biffs_cnt;
        return NULL;
    }
    
    return biffs[index].first.c_str();
}

const char *playerscriptlist(int32_t index, int32_t *list_size)
{
    if(index < 0)
    {
        *list_size = bihero_cnt;
        return NULL;
    }
    
    return bihero[index].first.c_str();
}

const char *lweaponscriptlist(int32_t index, int32_t *list_size)
{
    if(index < 0)
    {
        *list_size = bilweapons_cnt;
        return NULL;
    }
    
    return bilweapons[index].first.c_str();
}

const char *npcscriptlist(int32_t index, int32_t *list_size)
{
    if(index < 0)
    {
        *list_size = binpcs_cnt;
        return NULL;
    }
    
    return binpcs[index].first.c_str();
}

static char itemscript_str_buf[32];

char *itemscriptlist(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        bound(index,0,255);
        sprintf(itemscript_str_buf,"%d: %s",index, ffcmap[index-1].scriptname.c_str());
        return itemscript_str_buf;
    }
    
    *list_size=256;
    return NULL;
}

static char ffscript_str_buf2[32];

const char *ffscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,510);
        
        if(ffcmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, ffcmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(ffscript_str_buf2,"%d: %s",index+1, buf);
        return ffscript_str_buf2;
    }
    
    *list_size=511;
    return NULL;
}

static char itemscript_str_buf2[32];

const char *itemscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(itemmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, itemmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(itemscript_str_buf2,"%d: %s",index+1, buf);
        return itemscript_str_buf2;
    }
    
    *list_size=255;
    return NULL;
}


static char comboscript_str_buf2[32];
const char *comboscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(comboscriptmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, comboscriptmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(comboscript_str_buf2,"%d: %s",index+1, buf);
        return comboscript_str_buf2;
    }
    
    *list_size=255;
    return NULL;
}

static char gscript_str_buf2[40];

const char *gscriptlist2(int32_t index, int32_t *list_size)
{
    if(index >= 0)
    {
        bound(index,0,3);
        
        char buf[20];
        
        if(globalmap[index].scriptname == "")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, globalmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        switch(index)
        {
            case GLOBAL_SCRIPT_INIT:
                sprintf(gscript_str_buf2,"Initialization: %s", buf); break;
            case GLOBAL_SCRIPT_GAME:
                sprintf(gscript_str_buf2,"Active: %s", buf); break;
            case GLOBAL_SCRIPT_END:
                sprintf(gscript_str_buf2,"onExit: %s", buf); break;
            case GLOBAL_SCRIPT_ONSAVELOAD:
                sprintf(gscript_str_buf2,"onSaveLoad: %s", buf); break;
            case GLOBAL_SCRIPT_ONLAUNCH:
                sprintf(gscript_str_buf2,"onLaunch: %s", buf); break;
            case GLOBAL_SCRIPT_ONCONTGAME:
                sprintf(gscript_str_buf2,"onContGame: %s", buf); break;
            case GLOBAL_SCRIPT_F6:
                sprintf(gscript_str_buf2,"onF6Menu: %s", buf); break;
            case GLOBAL_SCRIPT_ONSAVE:
                sprintf(gscript_str_buf2,"onSave: %s", buf); break;
        }
            
        return gscript_str_buf2;
    }
    
    if(list_size != NULL)
        *list_size=4;
        
    return NULL;
}

static int32_t as_ffc_list[] = { 4, 5, 6, -1};
static int32_t as_global_list[] = { 7, 8, 9, -1}; //Why does putting 15 in here not place my message only on the global tab? ~Joe
static int32_t as_item_list[] = { 10, 11, 12, -1};
static int32_t as_npc_list[] = { 18, 19, 20, -1}; //npc scripts TAB
static int32_t as_lweapon_list[] = { 21, 22, 23, -1}; //lweapon scripts TAB
static int32_t as_eweapon_list[] = { 24, 25, 26, -1}; //eweapon scripts TAB
static int32_t as_hero_list[] = { 27, 28, 29, -1}; //hero scripts TAB
static int32_t as_screen_list[] = { 30, 31, 32, -1}; //screendata scripts TAB
static int32_t as_dmap_list[] = { 33, 34, 35, -1}; //dmapdata scripts TAB
static int32_t as_itemsprite_list[] = { 36, 37, 38, -1}; //dmapdata scripts TAB
static int32_t as_comboscript_list[] = { 39, 40, 41, -1}; //combodata scripts TAB
static int32_t as_genericscript_list[] = { 45, 46, 47, -1}; //generic scripts TAB
static int32_t as_subscreenscript_list[] = { 48, 49, 50, -1}; //generic scripts TAB

static TABPANEL assignscript_tabs[] =
{
    // (text)
    { (char *)"FFC",     D_SELECTED,  as_ffc_list,    0, NULL },
    { (char *)"Global",	 0,         as_global_list, 0, NULL },
    { (char *)"Item",		 0,         as_item_list,   0, NULL },
    { (char *)"NPC",		 0,         as_npc_list,   0, NULL },
    { (char *)"LWeapon",		 0,         as_lweapon_list,   0, NULL },
    { (char *)"EWeapon",		 0,         as_eweapon_list,   0, NULL },
    { (char *)"Hero",		 0,         as_hero_list,   0, NULL },
    { (char *)"DMap",		 0,         as_dmap_list,   0, NULL },
    { (char *)"Screen",		 0,         as_screen_list,   0, NULL },
    { (char *)"Item Sprite",		 0,         as_itemsprite_list,   0, NULL },
    { (char *)"Combo",		 0,         as_comboscript_list,   0, NULL },
    { (char *)"Generic",		 0,         as_genericscript_list,   0, NULL },
    { (char *)"Subscreen",		 0,         as_subscreenscript_list,   0, NULL },
    { NULL,                0,           NULL,         0, NULL }
};

const char *assignffclist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)ffcmap.size();
        return NULL;
    }
    
    return ffcmap[index].output.c_str();
}

const char *assigngloballist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)globalmap.size();
        return NULL;
    }
    
    return globalmap[index].output.c_str();
}

const char *assigncombolist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)comboscriptmap.size();
        return NULL;
    }
    
    return comboscriptmap[index].output.c_str();
}

const char *assigngenericlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = ((int32_t)genericmap.size());
        return NULL;
    }
    
    return genericmap[index].output.c_str();
}

const char *assignsubscreenlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = ((int32_t)subscreenmap.size());
        return NULL;
    }
    
    return subscreenmap[index].output.c_str();
}

const char *assignitemlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)itemmap.size();
        return NULL;
    }
    
    return itemmap[index].output.c_str();
}
const char *assignnpclist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)npcmap.size();
        return NULL;
    }
    
    return npcmap[index].output.c_str();
}

const char *assignlweaponlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)lwpnmap.size();
        return NULL;
    }
    
    return lwpnmap[index].output.c_str();
}

const char *assigneweaponlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)ewpnmap.size();
        return NULL;
    }
    
    return ewpnmap[index].output.c_str();
}

const char *assignplayerlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)playermap.size();
        return NULL;
    }
    
    return playermap[index].output.c_str();
}

const char *assigndmaplist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)dmapmap.size();
        return NULL;
    }
    
    return dmapmap[index].output.c_str();
}

const char *assignscreenlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)screenmap.size();
        return NULL;
    }
    
    return screenmap[index].output.c_str();
}

const char *assignitemspritelist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)itemspritemap.size();
        return NULL;
    }
    
    return itemspritemap[index].output.c_str();
}

const char *assignffcscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asffcscripts.size();
        return NULL;
    }
    
    return asffcscripts[index].c_str();
}

const char *assignglobalscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asglobalscripts.size();
        return NULL;
    }
    
    return asglobalscripts[index].c_str();
}

const char *assignitemscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asitemscripts.size();
        return NULL;
    }
    
    return asitemscripts[index].c_str();
}

const char *assignnpcscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asnpcscripts.size();
        return NULL;
    }
    
    return asnpcscripts[index].c_str();
}

const char *assignlweaponscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)aslweaponscripts.size();
        return NULL;
    }
    
    return aslweaponscripts[index].c_str();
}

const char *assigneweaponscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)aseweaponscripts.size();
        return NULL;
    }
    
    return aseweaponscripts[index].c_str();
}

const char *assignplayerscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asplayerscripts.size();
        return NULL;
    }
    
    return asplayerscripts[index].c_str();
}

const char *assigndmapscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asdmapscripts.size();
        return NULL;
    }
    
    return asdmapscripts[index].c_str();
}

const char *assignscreenscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asscreenscripts.size();
        return NULL;
    }
    
    return asscreenscripts[index].c_str();
}

const char *assignitemspritescriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asitemspritescripts.size();
        return NULL;
    }
    
    return asitemspritescripts[index].c_str();
}

const char *assigncomboscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)ascomboscripts.size();
        return NULL;
    }
    
    return ascomboscripts[index].c_str();
}

const char *assigngenericscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)asgenericscripts.size();
        return NULL;
    }
    
    return asgenericscripts[index].c_str();
}

const char *assignsubscreenscriptlist(int32_t index, int32_t *list_size)
{
    if(index<0)
    {
        *list_size = (int32_t)assubscreenscripts.size();
        return NULL;
    }
    
    return assubscreenscripts[index].c_str();
}

static ListData assignffc_list(assignffclist, &font);
static ListData assignffcscript_list(assignffcscriptlist, &font);
static ListData assignglobal_list(assigngloballist, &font);
static ListData assignglobalscript_list(assignglobalscriptlist, &font);
static ListData assignitem_list(assignitemlist, &font);
static ListData assignitemscript_list(assignitemscriptlist, &font);
static ListData assignnpc_list(assignnpclist, &font);
static ListData assignnpcscript_list(assignnpcscriptlist, &font);
static ListData assignlweapon_list(assignlweaponlist, &font);
static ListData assignlweaponscript_list(assignlweaponscriptlist, &font);
static ListData assigneweapon_list(assigneweaponlist, &font);
static ListData assigneweaponscript_list(assigneweaponscriptlist, &font);

static ListData assignplayer_list(assignplayerlist, &font);
static ListData assignplayerscript_list(assignplayerscriptlist, &font);

static ListData assigndmap_list(assigndmaplist, &font);
static ListData assigndmapscript_list(assigndmapscriptlist, &font);

static ListData assignscreen_list(assignscreenlist, &font);
static ListData assignscreenscript_list(assignscreenscriptlist, &font);

static ListData assignitemsprite_list(assignitemspritelist, &font);
static ListData assignitemspritescript_list(assignitemspritescriptlist, &font);

static ListData assigncombo_list(assigncombolist, &font);
static ListData assigncomboscript_list(assigncomboscriptlist, &font);

static ListData assigngeneric_list(assigngenericlist, &font);
static ListData assigngenericscript_list(assigngenericscriptlist, &font);

static ListData assignsubscreen_list(assignsubscreenlist, &font);
static ListData assignsubscreenscript_list(assignsubscreenscriptlist, &font);

static DIALOG assignscript_dlg[] =
{
    //						x		y		w		h		fg		bg		key	flags	d1	d2	dp
    { jwin_win_proc,		  0,	0,		330,	236,	vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "Assign Compiled Script", NULL, NULL },
    { jwin_tab_proc,		  6,	25,		330-12,	130,	0,		0,		0,	0,		0,  0,  assignscript_tabs, NULL, (void*)assignscript_dlg },
    { jwin_button_proc,	  251,	207,	61,		21,		vc(14),	vc(1),	27,	D_EXIT,	0,	0,	(void *) "Cancel", NULL, NULL },
    { jwin_button_proc,	  182,	207,	61,		21,		vc(14), vc(1),	0,	    D_EXIT,	0,	0,	(void *) "OK", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignffc_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignffcscript_list, NULL, NULL },
    //6
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignglobal_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignglobalscript_list, NULL, NULL },
    //9
    { jwin_button_proc,	  154+5,	93,	15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitem_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitemscript_list, NULL, NULL },
    //12
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //13
    { jwin_check_proc,      22,  211,   90,   8,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Output ZASM code to allegro.log", NULL, NULL },
    { jwin_text_proc,       22,  178,   90,   24,    vc(14),  vc(1),  0,       0,          0,             0, NULL, NULL, NULL },
    { d_dummy_proc,       0,  0,  0,   0,    vc(14),  vc(1),  0,       0,          0,             0, NULL, NULL, NULL },
    //16
    { d_dummy_proc,       0,  0,  0,   0,    vc(14),  vc(1),  0,       0,          0,             0, NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    //npc scripts
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignnpc_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignnpcscript_list, NULL, NULL },
    //20
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //21
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignlweapon_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignlweaponscript_list, NULL, NULL },
    //23
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //24
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigneweapon_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigneweaponscript_list, NULL, NULL },
    //26
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //27
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignplayer_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignplayerscript_list, NULL, NULL },
    //29
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //30
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignscreen_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignscreenscript_list, NULL, NULL },
    //32
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //33
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigndmap_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigndmapscript_list, NULL, NULL },
    //35
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //36
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitemsprite_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitemspritescript_list, NULL, NULL },
    //38
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    
    //39
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigncombo_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigncomboscript_list, NULL, NULL },
    //41
    { jwin_button_proc,	     154+5,   93,     15,     10,     vc(14), vc(1),  0, D_EXIT, 0,  0,  (void *) "<<", NULL, NULL },
    { jwin_button_proc,      78-24,  158,     48,     16,     vc(14), vc(1),  0, D_EXIT, 0,  0,  (void *) "Script Info",  NULL, NULL },
    { jwin_button_proc,  174+78-24,  158,     48,     16,     vc(14), vc(1),  0, D_EXIT, 0,  0,  (void *) "Script Info",  NULL, NULL },
    { jwin_button_proc,   87+78-24,  158,     48,     16,     vc(14), vc(1),  0, D_EXIT, 0,  0,  (void *) "Clear",  NULL, NULL },
    //45
	{ jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigngeneric_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assigngenericscript_list, NULL, NULL },
    //47
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    //48
	{ jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignsubscreen_list, NULL, NULL },
    { jwin_abclist_proc,    174+10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignsubscreenscript_list, NULL, NULL },
    //50
    { jwin_button_proc,	  154+5,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    { jwin_check_proc,      22,  221,   90,   8,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "...And output ZASM comments", NULL, NULL },
    
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,        NULL, NULL, NULL }
    
};

int32_t txtout(BITMAP* dest, const char* txt, int32_t x, int32_t y, bool disabled)
{
	if(disabled)
	{
		gui_textout_ln(dest, font, (uint8_t*)txt, x+1, y+1, scheme[jcLIGHT], scheme[jcBOX], 0);
		return gui_textout_ln(dest, font, (uint8_t*)txt, x, y, scheme[jcMEDDARK], -1, 0);
	}
	else
	{
		return gui_textout_ln(dest, font, (uint8_t*)txt, x, y, scheme[jcBOXFG], scheme[jcBOX], 0);
	}
}

int32_t jwin_zmeta_proc(int32_t msg, DIALOG *d, int32_t )
{
	int32_t ret = D_O_K;
    ASSERT(d);
    
	BITMAP* target = (msg==MSG_START ? NULL : screen);
    switch(msg)
    {
		case MSG_START:
		case MSG_DRAW:
		{
			FONT *oldfont = font;
			
			if(d->dp2)
			{
				font = (FONT*)d->dp2;
			}
			
			bool disabled = (d->flags & D_DISABLED) != 0;
			if(d->dp)
			{
				zasm_meta const& meta = *((zasm_meta*)d->dp);
				int32_t ind = -1;
				d->w = 0;
				if(!meta.valid())
				{
					d->w = txtout(target, "Invalid ZASM metadata found!", d->x, d->y, disabled);
					++ind;
				}
				
				int32_t t_w = 0;
				char buf[1024];
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "ZASM Version: %d", meta.zasm_v);
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Metadata Version: %d", meta.meta_v);
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "FFScript Version: %d", meta.ffscript_v);
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Script Name: %s", meta.script_name.c_str());
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Author: %s", meta.author.c_str());
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Script Type: %s", get_script_name(meta.script_type).c_str());
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				for(auto q = 0; q < 4; ++q)
				{
					if(!meta.attributes[q].size())
						continue;
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "Attributes[%d]: %s", q, meta.attributes[q].c_str());
					t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
					d->w = zc_max(d->w, t_w);
				}
				for(auto q = 0; q < 8; ++q)
				{
					if(!meta.attribytes[q].size())
						continue;
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "Attribytes[%d]: %s", q, meta.attribytes[q].c_str());
					t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
					d->w = zc_max(d->w, t_w);
				}
				for(auto q = 0; q < 8; ++q)
				{
					if(!meta.attrishorts[q].size())
						continue;
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "Attrishorts[%d]: %s", q, meta.attrishorts[q].c_str());
					t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
					d->w = zc_max(d->w, t_w);
				}
				bool indentrun = false;
				int32_t run_indent = txtout(NULL, "void run(", 0, 0, false);
				std::ostringstream oss;
				oss << "void run(";
				for(int32_t q = 0; q < 8; ++q)
				{
					if(!meta.run_idens[q].size() || meta.run_types[q] == ZMETA_NULL_TYPE) continue;
					if(q > 0)
						oss << ", ";
					string type_name = ZScript::getDataTypeName(meta.run_types[q]);
					lowerstr(type_name); //all lowercase for this output
					if(oss.str().size() > unsigned(indentrun ? 41 : 50))
					{
						memset(buf, 0, sizeof(buf));
						sprintf(buf, "%s", oss.str().c_str());
						t_w = txtout(target, buf, d->x + (indentrun ? run_indent : 0), d->y + ((++ind)*(text_height(font) + 3)), disabled) + (indentrun ? run_indent : 0);
						d->w = zc_max(d->w, t_w);
						oss.str("");
						indentrun = true;
					}
					oss << type_name.c_str() << " " << meta.run_idens[q];
				}
				oss << ");";
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "%s", oss.str().c_str());
				t_w = txtout(target, buf, d->x + (indentrun ? run_indent : 0), d->y + ((++ind)*(text_height(font) + 3)), disabled) + (indentrun ? run_indent : 0);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Compiler Version: %d.%d.%d.%d", meta.compiler_v1, meta.compiler_v2, meta.compiler_v3, meta.compiler_v4);
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "Parser-generated: %s", (meta.flags & ZMETA_AUTOGEN)!=0 ? "TRUE" : "FALSE");
				t_w = txtout(target, buf, d->x, d->y + ((++ind)*(text_height(font) + 3)), disabled);
				d->w = zc_max(d->w, t_w);
				d->h = (++ind) * (text_height(font) + 3) -3;
			}
			else
			{
				d->w = txtout(target, "No ZASM metadata found!", d->x, d->y, disabled);
				d->h = text_height(font);
			}
			
			if(d->dp3) //function trigger
			{
				typedef void (*funcType)(void);
				funcType func=reinterpret_cast<funcType>(d->dp3);
				func();
			}
			
			font = oldfont;
			break;
		}
    }
    
    return ret;
}

void resize_scriptinfo_dlg();

static DIALOG scriptinfo_dlg[] =
{
	//                    x     y       w       h        fg       bg   key    flags    d1    d2    dp
	{ jwin_win_proc,      0,    0,    200,    150,    vc(14),  vc(1),    0,  D_EXIT,    0,    0,   (void *) "Script Metadata", NULL, NULL },
	{ d_dummy_proc,       6,   25, 330-12,    130,         0,      0,    0,       0,    0,    0,   assignscript_tabs, NULL, NULL },
	{ jwin_button_proc,  70,  120,     60,     20,    vc(14),  vc(1),  'k',  D_EXIT,    0,    0,   (void *) "Done", NULL, NULL },
	{ jwin_zmeta_proc,   50,   30,    100,    100,    vc(14),  vc(1),    0,       0,    0,    0,   NULL, NULL, (void*)resize_scriptinfo_dlg },
	
	{ NULL,               0,    0,      0,      0,         0,      0,    0,       0,    0,    0,   NULL, NULL, NULL }
};

void resize_scriptinfo_dlg()
{
	DIALOG *meta_proc = &scriptinfo_dlg[3], *window = &scriptinfo_dlg[0], *ok_button = &scriptinfo_dlg[2];
	int32_t bmargin = 15, hmargins = 30;
	jwin_ulalign_dialog(scriptinfo_dlg);
	window->w = hmargins*2 + meta_proc->w;
	meta_proc->x = hmargins;
	window->h = meta_proc->y + meta_proc->h + ok_button->h + bmargin*2;
	ok_button->x = (window->w/2)-(ok_button->w/2);
	ok_button->y = meta_proc->y + meta_proc->h + bmargin;
	jwin_center_dialog(scriptinfo_dlg);
}

void showScriptInfo(zasm_meta const* meta)
{
	scriptinfo_dlg[3].dp = (void*)meta;
	scriptinfo_dlg[0].dp2 = get_zc_font(font_lfont);
	large_dialog(scriptinfo_dlg);
	jwin_zmeta_proc(MSG_START,&scriptinfo_dlg[3],0); //Calculate size before calling dialog
	jwin_center_dialog(scriptinfo_dlg);
	do_zqdialog(scriptinfo_dlg,2);
}

void write_includepaths();
void call_compile_settings();
int32_t onZScriptCompilerSettings()
{
	call_compile_settings();
	return D_O_K;
}

void doEditZScript()
{
	if(do_box_edit(zScript, "ZScript Buffer", false, false))
        saved=false;
}

std::string qst_cfg_header_from_path(std::string path);
extern char *filepath;
string get_box_cfg_hdr(int num)
{
	if(num)
		return "misc";
	return qst_cfg_header_from_path(filepath);
}

//{ Start type-specific import dlgs
static ListData ffscript_sel_dlg_list(ffscriptlist2, &font);
static ListData itemscript_sel_dlg_list(itemscriptlist2, &font);
static ListData comboscript_sel_dlg_list(comboscriptlist2, &font);
static ListData gscript_sel_dlg_list(gscriptlist2, &font);
static char npcscript_str_buf2[32];
const char *npcscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(npcmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, npcmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(npcscript_str_buf2,"%d: %s",index+1, buf);
        return npcscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTGUYS-1);
    return NULL;
}
static ListData npcscript_sel_dlg_list(npcscriptlist2, &font);
static char lweaponscript_str_buf2[32];
const char *lweaponscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(lwpnmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, lwpnmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(lweaponscript_str_buf2,"%d: %s",index+1, buf);
        return lweaponscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTWEAPONS-1);
    return NULL;
}
static ListData lweaponscript_sel_dlg_list(lweaponscriptlist2, &font);
static char eweaponscript_str_buf2[32];
const char *eweaponscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(ewpnmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, ewpnmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(eweaponscript_str_buf2,"%d: %s",index+1, buf);
        return eweaponscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTWEAPONS-1);
    return NULL;
}
static ListData eweaponscript_sel_dlg_list(eweaponscriptlist2, &font);
static char playerscript_str_buf2[32];
const char *playerscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,3);
        
        if(playermap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, playermap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
	
	if(index==0)
            sprintf(playerscript_str_buf2,"Init: %s", buf);
            
        if(index==1)
            sprintf(playerscript_str_buf2,"Active: %s", buf);
	
	if(index==2)
            sprintf(playerscript_str_buf2,"Death: %s", buf);
            
        
        //sprintf(playerscript_str_buf2,"%d: %s",index+1, buf);
        return playerscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTHERO-1);
    return NULL;
}
static char itemspritescript_str_buf2[32];
const char *itemspritescriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(itemspritemap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, itemspritemap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(itemspritescript_str_buf2,"%d: %s",index+1, buf);
        return itemspritescript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTSITEMSPRITE-1);
    return NULL;
}
static ListData playerscript_sel_dlg_list(playerscriptlist2, &font);
static char dmapscript_str_buf2[32];
const char *dmapscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(dmapmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, dmapmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(dmapscript_str_buf2,"%d: %s",index+1, buf);
        return dmapscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTSDMAP-1);
    return NULL;
}
static ListData dmapscript_sel_dlg_list(dmapscriptlist2, &font);
static ListData itemspritescript_sel_dlg_list(itemspritescriptlist2, &font);
static char screenscript_str_buf2[32];
const char *screenscriptlist2(int32_t index, int32_t *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(screenmap[index].scriptname=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, screenmap[index].scriptname.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(screenscript_str_buf2,"%d: %s",index+1, buf);
        return screenscript_str_buf2;
    }
    
    *list_size=(NUMSCRIPTSCREEN-1);
    return NULL;
}
static ListData screenscript_sel_dlg_list(screenscriptlist2, &font);
//} End type-specific import dlgs

void clear_map_states()
{
	for(map<int32_t, script_slot_data>::iterator it = ffcmap.begin();
		it != ffcmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = globalmap.begin();
		it != globalmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = itemmap.begin();
		it != itemmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = npcmap.begin();
		it != npcmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = ewpnmap.begin();
		it != ewpnmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = lwpnmap.begin();
		it != lwpnmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = playermap.begin();
		it != playermap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = dmapmap.begin();
		it != dmapmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = screenmap.begin();
		it != screenmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = itemspritemap.begin();
		it != itemspritemap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
	for(map<int32_t, script_slot_data>::iterator it = comboscriptmap.begin();
		it != comboscriptmap.end(); ++it)
	{
		(*it).second.format = SCRIPT_FORMAT_DEFAULT;
	}
}

void clearAssignSlotDlg()
{
	assignscript_dlg[0].dp2 = get_zc_font(font_lfont);
	assignscript_dlg[4].d1 = -1;
	assignscript_dlg[5].d1 = -1;
	assignscript_dlg[7].d1 = -1;
	assignscript_dlg[8].d1 = -1;
	assignscript_dlg[10].d1 = -1;
	assignscript_dlg[11].d1 = -1;
	assignscript_dlg[13].flags = 0;
}

int32_t onSlotAssign()
{
	clearAssignSlotDlg();
	//Clear right-hand side of names
	asffcscripts.clear();
	asffcscripts.push_back("<none>");
	asglobalscripts.clear();
	asglobalscripts.push_back("<none>");
	asitemscripts.clear();
	asitemscripts.push_back("<none>");
	asnpcscripts.clear();
	asnpcscripts.push_back("<none>");
	aseweaponscripts.clear();
	aseweaponscripts.push_back("<none>");
	aslweaponscripts.clear();
	aslweaponscripts.push_back("<none>");
	asplayerscripts.clear();
	asplayerscripts.push_back("<none>");
	asdmapscripts.clear();
	asdmapscripts.push_back("<none>");
	asscreenscripts.clear();
	asscreenscripts.push_back("<none>");
	asitemspritescripts.clear();
	asitemspritescripts.push_back("<none>");
	
	ascomboscripts.clear();
	ascomboscripts.push_back("<none>");
	asgenericscripts.clear();
	asgenericscripts.push_back("<none>");
	assubscreenscripts.clear();
	assubscreenscripts.push_back("<none>");
	//Declare new script vector
	map<string, disassembled_script_data> scripts;
	
	do_script_disassembly(scripts, false);
	
	do_slots(scripts, false);
	return D_O_K;
}

void inc_script_name(string& name)
{
	size_t pos = name.find_last_not_of("0123456789");
	pos = name.find_first_of("0123456789",pos);
	std::ostringstream oss;
	if(pos == string::npos)
	{
		oss << name << 2;
	}
	else
	{
		int32_t val = atoi(name.substr(pos).c_str());
		oss << name.substr(0,pos) << val+1;
	}
	name = oss.str();
}

void do_script_disassembly(map<string, disassembled_script_data>& scripts, bool fromCompile)
{
	clearAssignSlotDlg();
	bool skipDisassembled = fromCompile && try_recovering_missing_scripts == 0;
	for(int32_t i = 0; i < NUMSCRIPTGLOBAL; ++i)
	{
		if(scripts.find(globalmap[i].scriptname) != scripts.end())
		{
			if(scripts[globalmap[i].scriptname].first.script_type != ScriptType::Global)
			{
				while(scripts.find(globalmap[i].scriptname) != scripts.end())
					inc_script_name(globalmap[i].scriptname);
			}
			else continue;
		}
		switch(i)
		{
			case GLOBAL_SCRIPT_INIT:
			{
				break;
			}
			default:
				if(!globalmap[i].isEmpty())
				{
					globalmap[i].format = SCRIPT_FORMAT_INVALID;
				}
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTFFC-1; ++i)
	{
		if(scripts.find(ffcmap[i].scriptname) != scripts.end())
		{
			if(scripts[ffcmap[i].scriptname].first.script_type != ScriptType::FFC)
			{
				while(scripts.find(ffcmap[i].scriptname) != scripts.end())
					inc_script_name(ffcmap[i].scriptname);
			}
			else continue;
		}
		if(!ffcmap[i].isEmpty())
		{
			ffcmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTITEM-1; ++i)
	{
		if(scripts.find(itemmap[i].scriptname) != scripts.end())
		{
			if(scripts[itemmap[i].scriptname].first.script_type != ScriptType::Item)
			{
				while(scripts.find(itemmap[i].scriptname) != scripts.end())
					inc_script_name(itemmap[i].scriptname);
			}
			else continue;
		}
		if(!itemmap[i].isEmpty())
		{
			itemmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTGUYS-1; ++i)
	{
		if(scripts.find(npcmap[i].scriptname) != scripts.end())
		{
			if(scripts[npcmap[i].scriptname].first.script_type != ScriptType::NPC)
			{
				while(scripts.find(npcmap[i].scriptname) != scripts.end())
					inc_script_name(npcmap[i].scriptname);
			}
			else continue;
		}
		if(!npcmap[i].isEmpty())
		{
			npcmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTWEAPONS-1; ++i)
	{
		if(scripts.find(lwpnmap[i].scriptname) != scripts.end())
		{
			if(scripts[lwpnmap[i].scriptname].first.script_type != ScriptType::Lwpn)
			{
				while(scripts.find(lwpnmap[i].scriptname) != scripts.end())
					inc_script_name(lwpnmap[i].scriptname);
			}
			else continue;
		}
		if(!lwpnmap[i].isEmpty())
		{
			lwpnmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTWEAPONS-1; ++i)
	{
		if(scripts.find(ewpnmap[i].scriptname) != scripts.end())
		{
			if(scripts[ewpnmap[i].scriptname].first.script_type != ScriptType::Ewpn)
			{
				while(scripts.find(ewpnmap[i].scriptname) != scripts.end())
					inc_script_name(ewpnmap[i].scriptname);
			}
			else continue;
		}
		if(!ewpnmap[i].isEmpty())
		{
			ewpnmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTHERO-1; ++i)
	{
		if(scripts.find(playermap[i].scriptname) != scripts.end())
		{
			if(scripts[playermap[i].scriptname].first.script_type != ScriptType::Hero)
			{
				while(scripts.find(playermap[i].scriptname) != scripts.end())
					inc_script_name(playermap[i].scriptname);
			}
			else continue;
		}
		if(!playermap[i].isEmpty())
		{
			playermap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSDMAP-1; ++i)
	{
		if(scripts.find(dmapmap[i].scriptname) != scripts.end())
		{
			if(scripts[dmapmap[i].scriptname].first.script_type != ScriptType::DMap)
			{
				while(scripts.find(dmapmap[i].scriptname) != scripts.end())
					inc_script_name(dmapmap[i].scriptname);
			}
			else continue;
		}
		if(!dmapmap[i].isEmpty())
		{
			dmapmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSCREEN-1; ++i)
	{
		if(scripts.find(screenmap[i].scriptname) != scripts.end())
		{
			if(scripts[screenmap[i].scriptname].first.script_type != ScriptType::Screen)
			{
				while(scripts.find(screenmap[i].scriptname) != scripts.end())
					inc_script_name(screenmap[i].scriptname);
			}
			else continue;
		}
		if(!screenmap[i].isEmpty())
		{
			screenmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSITEMSPRITE-1; ++i)
	{
		if(scripts.find(itemspritemap[i].scriptname) != scripts.end())
		{
			if(scripts[itemspritemap[i].scriptname].first.script_type != ScriptType::ItemSprite)
			{
				while(scripts.find(itemspritemap[i].scriptname) != scripts.end())
					inc_script_name(itemspritemap[i].scriptname);
			}
			else continue;
		}
		if(!itemspritemap[i].isEmpty())
		{
			itemspritemap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSCOMBODATA-1; ++i)
	{
		if(scripts.find(comboscriptmap[i].scriptname) != scripts.end())
		{
			if(scripts[comboscriptmap[i].scriptname].first.script_type != ScriptType::Combo)
			{
				while(scripts.find(comboscriptmap[i].scriptname) != scripts.end())
					inc_script_name(comboscriptmap[i].scriptname);
			}
			else continue;
		}
		if(!comboscriptmap[i].isEmpty())
		{
			comboscriptmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSGENERIC-1; ++i)
	{
		if(scripts.find(genericmap[i].scriptname) != scripts.end())
		{
			if(scripts[genericmap[i].scriptname].first.script_type != ScriptType::Generic)
			{
				while(scripts.find(genericmap[i].scriptname) != scripts.end())
					inc_script_name(genericmap[i].scriptname);
			}
			else continue;
		}
		if(!genericmap[i].isEmpty())
		{
			genericmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
	for(int32_t i = 0; i < NUMSCRIPTSSUBSCREEN-1; ++i)
	{
		if(scripts.find(subscreenmap[i].scriptname) != scripts.end())
		{
			if(scripts[subscreenmap[i].scriptname].first.script_type != ScriptType::EngineSubscreen)
			{
				while(scripts.find(subscreenmap[i].scriptname) != scripts.end())
					inc_script_name(subscreenmap[i].scriptname);
			}
			else continue;
		}
		if(!subscreenmap[i].isEmpty())
		{
			subscreenmap[i].format = SCRIPT_FORMAT_INVALID;
		}
	}
}

enum script_slot_type
{
	type_ffc, type_global, type_itemdata, type_npc, type_lweapon, type_eweapon,
	type_hero, type_dmap, type_screen, type_itemsprite, type_combo, type_generic,
	type_subscreen, num_types
};
script_slot_type getType(ScriptType type)
{
	switch(type)
	{
		case ScriptType::FFC: return type_ffc;
		case ScriptType::Global: return type_global;
		case ScriptType::Item: return type_itemdata;
		case ScriptType::NPC: return type_npc;
		case ScriptType::Lwpn: return type_lweapon;
		case ScriptType::Ewpn: return type_eweapon;
		case ScriptType::Hero: return type_hero;
		case ScriptType::DMap:
		case ScriptType::ScriptedActiveSubscreen:
		case ScriptType::ScriptedPassiveSubscreen:
		case ScriptType::OnMap:
			return type_dmap;
		case ScriptType::Generic: case ScriptType::GenericFrozen:
			return type_generic;
		case ScriptType::Screen: return type_screen;
		case ScriptType::ItemSprite: return type_itemsprite;
		case ScriptType::Combo: return type_combo;
		case ScriptType::EngineSubscreen: return type_subscreen;
		default: return type_ffc; //Default
	}
}
#define SLOTMSGFLAG_MISSING		0x01
#define SLOTMSGFLAG_PRESERVED	0x02
#define SLOTMSGFLAG_IMPORTED	0x04
#define SLOTMSG_SIZE			512
bool checkSkip(int32_t format, byte flags)
{
	switch(format)
	{
		case SCRIPT_FORMAT_DEFAULT:
			return (flags != 0);
		case SCRIPT_FORMAT_INVALID:
			return ((flags & SLOTMSGFLAG_MISSING)==0);
		case SCRIPT_FORMAT_DISASSEMBLED:
			return ((flags & SLOTMSGFLAG_PRESERVED)==0);
		case SCRIPT_FORMAT_ZASM:
			return ((flags & SLOTMSGFLAG_IMPORTED)==0);
		default: return true;
	}
}
void clearAllSlots(int32_t type, byte flags = 0)
{
	bound(type,0,num_types-1);
	switch(type)
	{
		case type_ffc:
		{
			for(int32_t q = 0; q < NUMSCRIPTFFC-1; ++q)
			{
				if(checkSkip(ffcmap[q].format, flags)) continue;
				ffcmap[q].scriptname = "";
				ffcmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_global:
		{
			//Start at 1 to not clear Init
			for(int32_t q = 1; q < NUMSCRIPTGLOBAL; ++q)
			{
				if(checkSkip(globalmap[q].format, flags)) continue;
				globalmap[q].scriptname = "";
				globalmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_itemdata:
		{
			for(int32_t q = 0; q < NUMSCRIPTITEM-1; ++q)
			{
				if(checkSkip(itemmap[q].format, flags)) continue;
				itemmap[q].scriptname = "";
				itemmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_npc:
		{
			for(int32_t q = 0; q < NUMSCRIPTGUYS-1; ++q)
			{
				if(checkSkip(npcmap[q].format, flags)) continue;
				npcmap[q].scriptname = "";
				npcmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_lweapon:
		{
			for(int32_t q = 0; q < NUMSCRIPTWEAPONS-1; ++q)
			{
				if(checkSkip(lwpnmap[q].format, flags)) continue;
				lwpnmap[q].scriptname = "";
				lwpnmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_eweapon:
		{
			for(int32_t q = 0; q < NUMSCRIPTWEAPONS-1; ++q)
			{
				if(checkSkip(ewpnmap[q].format, flags)) continue;
				ewpnmap[q].scriptname = "";
				ewpnmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_hero:
		{
			for(int32_t q = 0; q < NUMSCRIPTHERO-1; ++q)
			{
				if(checkSkip(playermap[q].format, flags)) continue;
				playermap[q].scriptname = "";
				playermap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_dmap:
		{
			for(int32_t q = 0; q < NUMSCRIPTSDMAP-1; ++q)
			{
				if(checkSkip(dmapmap[q].format, flags)) continue;
				dmapmap[q].scriptname = "";
				dmapmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_screen:
		{
			for(int32_t q = 0; q < NUMSCRIPTSCREEN-1; ++q)
			{
				if(checkSkip(screenmap[q].format, flags)) continue;
				screenmap[q].scriptname = "";
				screenmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_itemsprite:
		{
			for(int32_t q = 0; q < NUMSCRIPTSITEMSPRITE-1; ++q)
			{
				if(checkSkip(itemspritemap[q].format, flags)) continue;
				itemspritemap[q].scriptname = "";
				itemspritemap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_combo:
		{
			for(int32_t q = 0; q < NUMSCRIPTSCOMBODATA-1; ++q)
			{
				if(checkSkip(comboscriptmap[q].format, flags)) continue;
				comboscriptmap[q].scriptname = "";
				comboscriptmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_generic:
		{
			for(int32_t q = 0; q < NUMSCRIPTSGENERIC-1; ++q)
			{
				if(checkSkip(genericmap[q].format, flags)) continue;
				genericmap[q].scriptname = "";
				genericmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
		case type_subscreen:
		{
			for(int32_t q = 0; q < NUMSCRIPTSSUBSCREEN-1; ++q)
			{
				if(checkSkip(subscreenmap[q].format, flags)) continue;
				subscreenmap[q].scriptname = "";
				subscreenmap[q].format = SCRIPT_FORMAT_DEFAULT;
			}
			break;
		}
	}
}

static bool doslots_log_output = false, doslots_comment_output = true;
void setup_scriptslot_dlg(char* buf, byte flags)
{
	//{ Set up the textbox at the bottom, and auto-resize height based on it
	int32_t prev_height = assignscript_dlg[14].h;
	memset(buf, 0, SLOTMSG_SIZE);
	//
	strcpy(buf, "Slots with matching names have been updated.\n");
	if(flags & SLOTMSGFLAG_MISSING)
		strcat(buf,"Scripts prefixed with '--' were not found, and will not function.\n");
	if(flags & SLOTMSGFLAG_PRESERVED)
		strcat(buf,	"Scripts prefixed with '++' were not found, but have been preserved.\n"
		            "    These scripts may not function correctly if they use global variables.\n");
	if(flags & SLOTMSGFLAG_IMPORTED)
		strcat(buf,"Scripts prefixed with '==' are imported ZASM scripts.\n");
	strcat(buf,"Global scripts named 'Init' will be appended to '~Init'");
	//
	SETFLAG(assignscript_dlg[13].flags, D_SELECTED, doslots_log_output);
	SETFLAG(assignscript_dlg[51].flags, D_SELECTED, doslots_comment_output);
	assignscript_dlg[14].dp = buf;
	object_message(&assignscript_dlg[14], MSG_START, 0); //Set the width/height
	if(int32_t diff = assignscript_dlg[14].h - prev_height) //resize dlg
	{
		int32_t prev_bottom = assignscript_dlg[14].y + prev_height;
		for(int32_t q = 1; assignscript_dlg[q].proc; ++q)
		{
			if(q==14) continue; //Don't change self
			if(assignscript_dlg[q].y < prev_bottom) continue; //above proc
			assignscript_dlg[q].y += diff;
		}
		assignscript_dlg[0].h += diff;
		jwin_center_dialog(assignscript_dlg);
	}
	//}
}

std::string global_slotnames[NUMSCRIPTGLOBAL] = {
	"Init",
	"Active",
	"onExit",
	"onSaveLoad",
	"onLaunch",
	"onContGame",
	"onF6Menu",
	"onSave",
};
std::string player_slotnames[NUMSCRIPTHERO-1] = {
	"Init",
	"Active",
	"onDeath",
	"onWin",
};
byte reload_scripts(map<string, disassembled_script_data> &scripts)
{
	byte slotflags = 0;
	char temp[100];
	for(int32_t i = 0; i < NUMSCRIPTFFC-1; i++)
	{
		if(ffcmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(ffcmap[i].isZASM())
			{
				if(ffcmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(ffcmap[i].scriptname) != scripts.end())
				ffcmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				ffcmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		ffcmap[i].slotname = temp;
		ffcmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTGLOBAL; i++)
	{
		globalmap[i].slotname=fmt::format("{}:",global_slotnames[i]);
		if(!globalmap[i].isEmpty())
		{
			if(globalmap[i].isZASM())
			{
				if(globalmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(globalmap[i].scriptname) != scripts.end() || globalmap[i].scriptname == "~Init")
				globalmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Unloaded
			{
				globalmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		globalmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTITEM-1; i++)
	{
		if(itemmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(itemmap[i].isZASM())
			{
				if(itemmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(itemmap[i].scriptname) != scripts.end())
				itemmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				itemmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		itemmap[i].slotname = temp;
		itemmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTGUYS-1; i++)
	{
		if(npcmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(npcmap[i].isZASM())
			{
				if(npcmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(npcmap[i].scriptname) != scripts.end())
				npcmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				npcmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		npcmap[i].slotname = temp;
		npcmap[i].update();
	} 
	for(int32_t i = 0; i < NUMSCRIPTWEAPONS-1; i++)
	{
		if(ewpnmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(ewpnmap[i].isZASM())
			{
				if(ewpnmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(ewpnmap[i].scriptname) != scripts.end())
				ewpnmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				ewpnmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		ewpnmap[i].slotname = temp;
		ewpnmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTWEAPONS-1; i++)
	{
		if(lwpnmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(lwpnmap[i].isZASM())
			{
				if(lwpnmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(lwpnmap[i].scriptname) != scripts.end())
				lwpnmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				lwpnmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		lwpnmap[i].slotname = temp;
		lwpnmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTHERO-1; i++)
	{
		playermap[i].slotname=fmt::format("{}:",player_slotnames[i]);
		if(!playermap[i].isEmpty())
		{
			if(playermap[i].isZASM())
			{
				if(playermap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(playermap[i].scriptname) != scripts.end())
				playermap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Unloaded
			{
				playermap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		playermap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSCREEN-1; i++)
	{
		if(screenmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(screenmap[i].isZASM())
			{
				if(screenmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(screenmap[i].scriptname) != scripts.end())
				screenmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				screenmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		screenmap[i].slotname = temp;
		screenmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSDMAP-1; i++)
	{
		if(dmapmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(dmapmap[i].isZASM())
			{
				if(dmapmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(dmapmap[i].scriptname) != scripts.end())
				dmapmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				dmapmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		dmapmap[i].slotname = temp;
		dmapmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSITEMSPRITE-1; i++)
	{
		if(itemspritemap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(itemspritemap[i].isZASM())
			{
				if(itemspritemap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(itemspritemap[i].scriptname) != scripts.end())
				itemspritemap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				itemspritemap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		itemspritemap[i].slotname = temp;
		itemspritemap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSCOMBODATA-1; i++)
	{
		if(comboscriptmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(comboscriptmap[i].isZASM())
			{
				if(comboscriptmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(comboscriptmap[i].scriptname) != scripts.end())
				comboscriptmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				comboscriptmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		comboscriptmap[i].slotname = temp;
		comboscriptmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSGENERIC-1; i++)
	{
		if(genericmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(genericmap[i].isZASM())
			{
				if(genericmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(genericmap[i].scriptname) != scripts.end())
				genericmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				genericmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		genericmap[i].slotname = temp;
		genericmap[i].update();
	}
	for(int32_t i = 0; i < NUMSCRIPTSSUBSCREEN-1; i++)
	{
		if(subscreenmap[i].isEmpty())
			sprintf(temp, "Slot %d:", i+1);
		else
		{
			sprintf(temp, "Slot %d:", i+1);
			if(subscreenmap[i].isZASM())
			{
				if(subscreenmap[i].isImportedZASM()) slotflags |= SLOTMSGFLAG_IMPORTED;
				else slotflags |= SLOTMSGFLAG_PRESERVED;
			}
			else if(scripts.find(subscreenmap[i].scriptname) != scripts.end())
				subscreenmap[i].format = SCRIPT_FORMAT_DEFAULT;
			else // Previously loaded script not found
			{
				subscreenmap[i].format = SCRIPT_FORMAT_INVALID;
				slotflags |= SLOTMSGFLAG_MISSING;
			}
		}
		subscreenmap[i].slotname = temp;
		subscreenmap[i].update();
	}
	return slotflags;
}

void doClearSlots(byte* flags);

extern byte compile_success_sample, compile_error_sample,
	compile_finish_sample, compile_audio_volume;
static map<string, disassembled_script_data> *doslot_scripts = nullptr;
bool handle_slot(script_slot_data& slotdata, int indx, script_data** scriptdata)
{
	if(slotdata.hasScriptData())
	{
		string scriptstr;
		(*doslot_scripts)[slotdata.scriptname].write(scriptstr, doslots_log_output, false, doslots_comment_output);
		if (parse_script_string(scriptdata[indx],scriptstr,false) != D_O_K)
			return false;
		
		if(slotdata.isDisassembled()) scriptdata[indx]->meta.setFlag(ZMETA_DISASSEMBLED);
		else if(slotdata.isImportedZASM()) scriptdata[indx]->meta.setFlag(ZMETA_IMPORTED);
	}
	else if(scriptdata[indx])
	{
		delete scriptdata[indx];
		// script_data::id is not used in editor, so should be fine for now.
		scriptdata[indx] = new script_data(ScriptType::None, 0);
	}
	return true;
}
bool handle_slot_map(map<int32_t, script_slot_data>& mp, int offs, script_data** scriptdata)
{
	for(auto it = mp.begin(); it != mp.end(); it++)
	{
		if(!handle_slot(it->second, it->first+offs, scriptdata))
			return false;
	}
	return true;
}

void smart_slot_named(map<string, disassembled_script_data> &scripts,
	vector<string> const& scriptnames, map<int32_t, script_slot_data>& mp,
	std::string* slotnames, int slotstart, int slotend)
{
	for(int q = slotstart; q < slotend; ++q)
	{
		auto& lval = mp[q];
		if(!lval.isEmpty())
			continue; //occupied, leave alone
		bool done = false;
		if(!done) //Check case-sensitive
			for(size_t rind = 0; rind < scriptnames.size(); ++rind)
			{
				auto const& rval = scriptnames[rind];
				if(rval == "<none>") continue;
				if(rval == slotnames[q])
				{ //Perfect match
					lval.updateName(rval);
					lval.format = scripts[lval.scriptname].format;
					done = true;
					break;
				}
			}
		if(!done) //Check case-insensitive
			for(size_t rind = 0; rind < scriptnames.size(); ++rind)
			{
				auto const& rval = scriptnames[rind];
				if(rval == "<none>") continue;
				string lc_rv = rval, lc_slot = slotnames[q];
				lowerstr(lc_rv);
				lowerstr(lc_slot);
				if(lc_rv == lc_slot)
				{ //Insensitive match
					lval.updateName(rval);
					lval.format = scripts[lval.scriptname].format;
					break;
				}
			}
	}
}
void smart_slot_type(map<string, disassembled_script_data> &scripts,
	vector<string> const& scriptnames, map<int32_t, script_slot_data>& mp,
	int slotcount)
{
	for(size_t rind = 0; rind < scriptnames.size(); ++rind)
	{
		auto const& rval = scriptnames[rind];
		if(rval == "<none>") continue;
		script_slot_data* first_open_slot = nullptr;
		bool done = false;
		for(int q = 0; q < slotcount; ++q)
		{
			auto& lval = mp[q];
			if(lval.isEmpty())
			{
				if(!first_open_slot)
					first_open_slot = &lval;
			}
			else if(lval.scriptname == rval)
			{
				done = true;
				break;
			}
		}
		if(!done)
		{
			if(!first_open_slot)
				break; //no slots left to assign to!
			first_open_slot->updateName(rval);
			first_open_slot->format = scripts[first_open_slot->scriptname].format;
		}
	}
}

bool do_slots(map<string, disassembled_script_data> &scripts, int assign_mode)
{
	large_dialog(assignscript_dlg);
	int32_t ret = 3;
	char slots_msg[SLOTMSG_SIZE] = {0};
	byte slotflags = reload_scripts(scripts);
	setup_scriptslot_dlg(slots_msg, slotflags);
	bool retval = false;
	
    popup_zqdialog_start();
	while(!assign_mode)
	{
		slotflags = reload_scripts(scripts);
        ret = do_zqdialog(assignscript_dlg, ret);
		
		switch(ret)
		{
			case 0:
			case 2:
				//Cancel
				goto exit_do_slots;
				
			case 3: goto auto_do_slots;
			
			case 6:
				//<<, FFC
			{
				int32_t lind = assignscript_dlg[4].d1;
				int32_t rind = assignscript_dlg[5].d1;
				
				if(lind < 0 || rind < 0)
					break;
					
				if(asffcscripts[rind] == "<none>")
				{
					ffcmap[lind].scriptname = "";
					ffcmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					ffcmap[lind].updateName(asffcscripts[rind]);
					ffcmap[lind].format = scripts[ffcmap[lind].scriptname].format;
				}
				
				break;
			}
			case 9:
				//<<, Global
			{
				int32_t lind = assignscript_dlg[7].d1;
				int32_t rind = assignscript_dlg[8].d1;
				
				if(lind < 0 || rind < 0)
					break;
					
				if(lind == 0)
				{
					jwin_alert("Error","ZScript reserves this slot.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
					break;
				}
				
				if(asglobalscripts[rind] == "<none>")
				{
					globalmap[lind].scriptname = "";
					globalmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					globalmap[lind].updateName(asglobalscripts[rind]);
					globalmap[lind].format = scripts[globalmap[lind].scriptname].format;
				}
				
				break;
			}				
			case 12:
				//<<, ITEM
			{
				int32_t lind = assignscript_dlg[10].d1;
				int32_t rind = assignscript_dlg[11].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(asitemscripts[rind] == "<none>")
				{
					itemmap[lind].scriptname = "";
					itemmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					itemmap[lind].updateName(asitemscripts[rind]);
					itemmap[lind].format = scripts[itemmap[lind].scriptname].format;
				}
				
				break;
			}
			case 20:
				//<<, NPC
			{
				int32_t lind = assignscript_dlg[18].d1;
				int32_t rind = assignscript_dlg[19].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(asnpcscripts[rind] == "<none>")
				{
					npcmap[lind].scriptname = "";
					npcmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					npcmap[lind].updateName(asnpcscripts[rind]);
					npcmap[lind].format = scripts[npcmap[lind].scriptname].format;
				}
				
				break;
			}
			case 23:
				//<<, LWeapon
			{
				int32_t lind = assignscript_dlg[21].d1;
				int32_t rind = assignscript_dlg[22].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(aslweaponscripts[rind] == "<none>")
				{
					lwpnmap[lind].scriptname = "";
					lwpnmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					lwpnmap[lind].updateName(aslweaponscripts[rind]);
					lwpnmap[lind].format = scripts[lwpnmap[lind].scriptname].format;
				}
				
				break;
			}
			case 26:
				//<<, EWeapon
			{
				int32_t lind = assignscript_dlg[24].d1;
				int32_t rind = assignscript_dlg[25].d1;
				
				if(lind < 0 || rind < 0)
					break;
					
				if(aseweaponscripts[rind] == "<none>")
				{
					ewpnmap[lind].scriptname = "";
					ewpnmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					ewpnmap[lind].updateName(aseweaponscripts[rind]);
					ewpnmap[lind].format = scripts[ewpnmap[lind].scriptname].format;
				}
				
				break;
			}
			case 29:
				//<<, Hero
			{
				int32_t lind = assignscript_dlg[27].d1;
				int32_t rind = assignscript_dlg[28].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(asplayerscripts[rind] == "<none>")
				{
					playermap[lind].scriptname = "";
					playermap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					playermap[lind].updateName(asplayerscripts[rind]);
					playermap[lind].format = scripts[playermap[lind].scriptname].format;
				}
				
				break;
			}
			case 32:
				//<<, Screendata
			{
				int32_t lind = assignscript_dlg[30].d1;
				int32_t rind = assignscript_dlg[31].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(asscreenscripts[rind] == "<none>")
				{
					screenmap[lind].scriptname = "";
					screenmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					screenmap[lind].updateName(asscreenscripts[rind]);
					screenmap[lind].format = scripts[screenmap[lind].scriptname].format;
				}
				
				break;
			}
			case 35:
				//<<, dmapdata
			{
				int32_t lind = assignscript_dlg[33].d1;
				int32_t rind = assignscript_dlg[34].d1;
				
				if(lind < 0 || rind < 0)
					break;
					
				if(asdmapscripts[rind] == "<none>")
				{
					dmapmap[lind].scriptname = "";
					dmapmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					dmapmap[lind].updateName(asdmapscripts[rind]);
					dmapmap[lind].format = scripts[dmapmap[lind].scriptname].format;
				}
				
				break;
			}
			case 38:
				//<<, itemsprite
			{
				int32_t lind = assignscript_dlg[36].d1;
				int32_t rind = assignscript_dlg[37].d1;
				
				if(lind < 0 || rind < 0)
					break;
					
				if(asitemspritescripts[rind] == "<none>")
				{
					itemspritemap[lind].scriptname = "";
					itemspritemap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					itemspritemap[lind].updateName(asitemspritescripts[rind]);
					itemspritemap[lind].format = scripts[itemspritemap[lind].scriptname].format;
				}
				
				break;
			}
			case 41:
				//<<, comboscript
			{
				int32_t lind = assignscript_dlg[39].d1;
				int32_t rind = assignscript_dlg[40].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(ascomboscripts[rind] == "<none>")
				{
					comboscriptmap[lind].scriptname = "";
					comboscriptmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					comboscriptmap[lind].updateName(ascomboscripts[rind]);
					comboscriptmap[lind].format = scripts[comboscriptmap[lind].scriptname].format;
				}
				
				break;
			}
			case 47:
				//<<, generic script
			{
				int32_t lind = assignscript_dlg[45].d1;
				int32_t rind = assignscript_dlg[46].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(asgenericscripts[rind] == "<none>")
				{
					genericmap[lind].scriptname = "";
					genericmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					genericmap[lind].updateName(asgenericscripts[rind]);
					genericmap[lind].format = scripts[genericmap[lind].scriptname].format;
				}
				
				break;
			}
			case 50:
				//<<, subscreen script
			{
				int32_t lind = assignscript_dlg[48].d1;
				int32_t rind = assignscript_dlg[49].d1;
				
				if(lind < 0 || rind < 0)
					break;
				
				if(assubscreenscripts[rind] == "<none>")
				{
					subscreenmap[lind].scriptname = "";
					subscreenmap[lind].format = SCRIPT_FORMAT_DEFAULT;
				}
				else
				{
					subscreenmap[lind].updateName(assubscreenscripts[rind]);
					subscreenmap[lind].format = scripts[subscreenmap[lind].scriptname].format;
				}
				
				break;
			}
			
			case 42:
				//Script Info, information
			{
				disassembled_script_data* target = nullptr;
				switch(get_selected_tab((TABPANEL*)assignscript_dlg[1].dp))
				{
					default:
					case 0: //FFC
					{
						int32_t id = assignscript_dlg[4].d1;
						if(id > -1 && ffcmap[id].hasScriptData())
						{
							target = &(scripts[ffcmap[id].scriptname]);
						}
						break;
					}
					case 1: //Global
					{
						int32_t id = assignscript_dlg[7].d1;
						if(id > -1 && globalmap[id].hasScriptData())
						{
							target = &(scripts[globalmap[id].scriptname]);
						}
						break;
					}
					case 2: //Item
					{
						int32_t id = assignscript_dlg[10].d1;
						if(id > -1 && itemmap[id].hasScriptData())
						{
							target = &(scripts[itemmap[id].scriptname]);
						}
						break;
					}
					case 3: //npc
					{
						int32_t id = assignscript_dlg[19].d1;
						if(id > -1 && npcmap[id].hasScriptData())
						{
							target = &(scripts[npcmap[id].scriptname]);
						}
						break;
					}
					case 4: //lweapon
					{
						int32_t id = assignscript_dlg[21].d1;
						if(id > -1 && lwpnmap[id].hasScriptData())
						{
							target = &(scripts[lwpnmap[id].scriptname]);
						}
						break;
					}
					case 5: //eweapon
					{
						int32_t id = assignscript_dlg[24].d1;
						if(id > -1 && ewpnmap[id].hasScriptData())
						{
							target = &(scripts[ewpnmap[id].scriptname]);
						}
						break;
					}
					case 6: //hero
					{
						int32_t id = assignscript_dlg[27].d1;
						if(id > -1 && playermap[id].hasScriptData())
						{
							target = &(scripts[playermap[id].scriptname]);
						}
						break;
					}
					case 7: //dmap
					{
						int32_t id = assignscript_dlg[33].d1;
						if(id > -1 && dmapmap[id].hasScriptData())
						{
							target = &(scripts[dmapmap[id].scriptname]);
						}
						break;
					}
					case 8: //screen
					{
						int32_t id = assignscript_dlg[30].d1;
						if(id > -1 && screenmap[id].hasScriptData())
						{
							target = &(scripts[screenmap[id].scriptname]);
						}
						break;
					}
					case 9: //itemsprite
					{
						int32_t id = assignscript_dlg[36].d1;
						if(id > -1 && itemspritemap[id].hasScriptData())
						{
							target = &(scripts[itemspritemap[id].scriptname]);
						}
						break;
					}
					case 10: //combo
					{
						int32_t id = assignscript_dlg[39].d1;
						if(id > -1 && comboscriptmap[id].hasScriptData())
						{
							target = &(scripts[comboscriptmap[id].scriptname]);
						}
						break;
					}
					case 11: //Generic
					{
						int32_t id = assignscript_dlg[45].d1;
						if(id > -1 && genericmap[id].hasScriptData())
						{
							target = &(scripts[genericmap[id].scriptname]);
						}
						break;
					}
					case 12: //Subscreen
					{
						int32_t id = assignscript_dlg[48].d1;
						if(id > -1 && subscreenmap[id].hasScriptData())
						{
							target = &(scripts[subscreenmap[id].scriptname]);
						}
						break;
					}
				}
				zasm_meta* meta = target ? &target->first : nullptr;
				if(devpwd() && CHECK_CTRL_CMD)
				{
					string str;
					target->write(str, false, false, (assignscript_dlg[51].flags == D_SELECTED), true);
					set_al_clipboard(str);
					displayinfo("Clipboard Copy","Copied ZASM of selected script to clipboard");
				}
				else if(meta)
					showScriptInfo(meta);
				break;
			}
		
			case 43:
				//Script Info, information
			{
				disassembled_script_data* target = NULL;
				switch(get_selected_tab((TABPANEL*)assignscript_dlg[1].dp))
				{
					default:
					case 0: //FFC
					{
						int32_t id = assignscript_dlg[5].d1;
						if(id < 0 || asffcscripts[id] == "<none>" || asffcscripts[id].at(0) == '-') break;
						target = &(scripts[asffcscripts[id]]);
						break;
					}
					case 1: //Global
					{
						int32_t id = assignscript_dlg[8].d1;
						if(id < 0 || asglobalscripts[id] == "<none>" || asglobalscripts[id].at(0) == '-') break;
						target = &(scripts[asglobalscripts[id]]);
						break;
					}
					case 2: //Item
					{
						int32_t id = assignscript_dlg[11].d1;
						if(id < 0 || asitemscripts[id] == "<none>" || asitemscripts[id].at(0) == '-') break;
						target = &(scripts[asitemscripts[id]]);
						break;
					}
					case 3: //npc
					{
						int32_t id = assignscript_dlg[20].d1;
						if(id < 0 || asnpcscripts[id] == "<none>" || asnpcscripts[id].at(0) == '-') break;
						target = &(scripts[asnpcscripts[id]]);
						break;
					}
					case 4: //lweapon
					{
						int32_t id = assignscript_dlg[22].d1;
						if(id < 0 || aslweaponscripts[id] == "<none>" || aslweaponscripts[id].at(0) == '-') break;
						target = &(scripts[aslweaponscripts[id]]);
						break;
					}
					case 5: //eweapon
					{
						int32_t id = assignscript_dlg[25].d1;
						if(id < 0 || aseweaponscripts[id] == "<none>" || aseweaponscripts[id].at(0) == '-') break;
						target = &(scripts[aseweaponscripts[id]]);
						break;
					}
					case 6: //hero
					{
						int32_t id = assignscript_dlg[28].d1;
						if(id < 0 || asplayerscripts[id] == "<none>" || asplayerscripts[id].at(0) == '-') break;
						target = &(scripts[asplayerscripts[id]]);
						break;
					}
					case 7: //dmap
					{
						int32_t id = assignscript_dlg[34].d1;
						if(id < 0 || asdmapscripts[id] == "<none>" || asdmapscripts[id].at(0) == '-') break;
						target = &(scripts[asdmapscripts[id]]);
						break;
					}
					case 8: //screen
					{
						int32_t id = assignscript_dlg[31].d1;
						if(id < 0 || asscreenscripts[id] == "<none>" || asscreenscripts[id].at(0) == '-') break;
						target = &(scripts[asscreenscripts[id]]);
						break;
					}
					case 9: //itemsprite
					{
						int32_t id = assignscript_dlg[37].d1;
						if(id < 0 || asitemspritescripts[id] == "<none>" || asitemspritescripts[id].at(0) == '-') break;
						target = &(scripts[asitemspritescripts[id]]);
						break;
					}
					case 10: //combo
					{
						int32_t id = assignscript_dlg[40].d1;
						if(id < 0 || ascomboscripts[id] == "<none>" || ascomboscripts[id].at(0) == '-') break;
						target = &(scripts[ascomboscripts[id]]);
						break;
					}
					case 11: //generic
					{
						int32_t id = assignscript_dlg[46].d1;
						if(id < 0 || asgenericscripts[id] == "<none>" || asgenericscripts[id].at(0) == '-') break;
						target = &(scripts[asgenericscripts[id]]);
						break;
					}
					case 12: //subscreen
					{
						int32_t id = assignscript_dlg[49].d1;
						if(id < 0 || assubscreenscripts[id] == "<none>" || assubscreenscripts[id].at(0) == '-') break;
						target = &(scripts[assubscreenscripts[id]]);
						break;
					}
				}
				zasm_meta* meta = target ? &target->first : nullptr;
				if(devpwd() && CHECK_CTRL_CMD)
				{
					string str;
					target->write(str, false, false, (assignscript_dlg[51].flags == D_SELECTED), true);
					set_al_clipboard(str);
					displayinfo("Clipboard Copy","Copied ZASM of selected script to clipboard");
				}
				else if(meta)
					showScriptInfo(meta);
				break;
			}
			
			case 44:
				//Clear, clear slots of current type- after a confirmation.
			{
				doClearSlots(&slotflags);
				break;
			}
		}
	}
	if(assign_mode == 2) //Smart Assign
	{
		//For global/hero scripts, match slot names if unoccupied
		smart_slot_named(scripts, asglobalscripts, globalmap, global_slotnames, 1, NUMSCRIPTGLOBAL);
		smart_slot_named(scripts, asplayerscripts, playermap, player_slotnames, 0, NUMSCRIPTHERO-1);
		//For other scripts, assign all un-assigned scripts
		smart_slot_type(scripts, asffcscripts, ffcmap, NUMSCRIPTFFC-1);
		smart_slot_type(scripts, asitemscripts, itemmap, NUMSCRIPTITEM-1);
		smart_slot_type(scripts, asnpcscripts, npcmap, NUMSCRIPTGUYS-1);
		smart_slot_type(scripts, aslweaponscripts, lwpnmap, NUMSCRIPTWEAPONS-1);
		smart_slot_type(scripts, aseweaponscripts, ewpnmap, NUMSCRIPTWEAPONS-1);
		smart_slot_type(scripts, asscreenscripts, screenmap, NUMSCRIPTSCREEN-1);
		smart_slot_type(scripts, asdmapscripts, dmapmap, NUMSCRIPTSDMAP-1);
		smart_slot_type(scripts, asitemspritescripts, itemspritemap, NUMSCRIPTSITEMSPRITE-1);
		smart_slot_type(scripts, ascomboscripts, comboscriptmap, NUMSCRIPTSCOMBODATA-1);
		smart_slot_type(scripts, asgenericscripts, genericmap, NUMSCRIPTSGENERIC-1);
		smart_slot_type(scripts, assubscreenscripts, subscreenmap, NUMSCRIPTSSUBSCREEN-1);
	}
auto_do_slots:
	doslots_log_output = (assignscript_dlg[13].flags == D_SELECTED);
	doslots_comment_output = (assignscript_dlg[51].flags == D_SELECTED);
	doslot_scripts = &scripts;
	//OK
	{
		auto start_assign_time = std::chrono::steady_clock::now();
		if(!handle_slot_map(ffcmap, 1, ffscripts))
			goto exit_do_slots;
		if(!handle_slot_map(globalmap, 0, globalscripts))
			goto exit_do_slots;
		if(!handle_slot_map(itemmap, 1, itemscripts))
			goto exit_do_slots;
		if(!handle_slot_map(npcmap, 1, guyscripts))
			goto exit_do_slots;
		if(!handle_slot_map(lwpnmap, 1, lwpnscripts))
			goto exit_do_slots;
		if(!handle_slot_map(ewpnmap, 1, ewpnscripts))
			goto exit_do_slots;
		if(!handle_slot_map(playermap, 1, playerscripts))
			goto exit_do_slots;
		if(!handle_slot_map(dmapmap, 1, dmapscripts))
			goto exit_do_slots;
		if(!handle_slot_map(screenmap, 1, screenscripts))
			goto exit_do_slots;
		if(!handle_slot_map(itemspritemap, 1, itemspritescripts))
			goto exit_do_slots;
		if(!handle_slot_map(comboscriptmap, 1, comboscripts))
			goto exit_do_slots;
		if(!handle_slot_map(genericmap, 1, genericscripts))
			goto exit_do_slots;
		if(!handle_slot_map(subscreenmap, 1, subscreenscripts))
			goto exit_do_slots;

		auto end_assign_time = std::chrono::steady_clock::now();
		int compile_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_assign_time - start_assign_time).count();
		al_trace("Assign Slots took %d ms\n", compile_time_ms);
		char buf[256] = {0};
		sprintf(buf, "ZScripts successfully loaded into script slots"
			"\nAssign Slots took %d ms", compile_time_ms);
		compile_finish_sample = vbound(zc_get_config("Compiler","compile_finish_sample",20),0,255);
		compile_audio_volume = vbound(zc_get_config("Compiler","compile_audio_volume",200),0,255);
		if ( compile_finish_sample > 0 )
		{
			if(sfxdat)
				sfx_voice[compile_finish_sample]=allocate_voice((SAMPLE*)sfxdata[compile_finish_sample].dat);
			else sfx_voice[compile_finish_sample]=allocate_voice(&customsfxdata[compile_finish_sample]);
			voice_set_volume(sfx_voice[compile_finish_sample], compile_audio_volume);
			voice_start(sfx_voice[compile_finish_sample]);
		}
		if(!assign_mode)
			InfoDialog("Slots Assigned",buf).show();
		if ( compile_finish_sample > 0 )
		{
			if(sfx_voice[compile_finish_sample]!=-1)
			{
				deallocate_voice(sfx_voice[compile_finish_sample]);
				sfx_voice[compile_finish_sample]=-1;
			}
		}
		build_biffs_list();
		build_biitems_list();
		retval = true;
		goto exit_do_slots;
	}
exit_do_slots:
	doslot_scripts = nullptr;
    popup_zqdialog_end();
	return retval;
}

static char slottype_str_buf[32];

const char *slottype_list(int32_t index, int32_t *list_size)
{
	if(index >= 0)
	{
		bound(index,0,num_types-1);
        
		switch(index)
		{
			case type_ffc:
				strcpy(slottype_str_buf, "FFC");
				break;
			case type_global:
				strcpy(slottype_str_buf, "Global");
				break;
			case type_itemdata:
				strcpy(slottype_str_buf, "Item");
				break;
			case type_npc:
				strcpy(slottype_str_buf, "NPC");
				break;
			case type_lweapon:
				strcpy(slottype_str_buf, "LWeapon");
				break;
			case type_eweapon:
				strcpy(slottype_str_buf, "EWeapon");
				break;
			case type_hero:
				strcpy(slottype_str_buf, "Hero");
				break;
			case type_dmap:
				strcpy(slottype_str_buf, "DMap");
				break;
			case type_screen:
				strcpy(slottype_str_buf, "Screen");
				break;
			case type_itemsprite:
				strcpy(slottype_str_buf, "ItemSprite");
				break;
			case type_combo:
				strcpy(slottype_str_buf, "Combo");
				break;
			case type_generic:
				strcpy(slottype_str_buf, "Generic");
				break;
			case type_subscreen:
				strcpy(slottype_str_buf, "Subscreen");
				break;
		}
        
        return slottype_str_buf;
	}
	*list_size = 11;
	return NULL;
}
static ListData slottype_sel_list(slottype_list, &font);

static DIALOG clearslots_dlg[] =
{
    { jwin_win_proc,        0,       0,       200,  159,    vc(14),             vc(1),              0,   D_EXIT,     0,  0, (void *) "Clear Slots", NULL, NULL },
    { jwin_button_proc,     35,      132,     61,   21,     vc(14),             vc(1),              13,  D_EXIT,     0,  0, (void *) "Confirm", NULL, NULL },
    { jwin_button_proc,     104,     132,     61,   21,     vc(14),             vc(1),              27,  D_EXIT,     0,  0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   50,      28+16,   70,   16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   0,          0,  0, (void *) &slottype_sel_list, NULL, NULL },
	{ jwin_radio_proc,      40,      34+00,   81,   9,      vc(14),             vc(1),              0,   D_SELECTED, 0,  0, (void *) "Clear Script Type:", NULL, NULL },
	{ jwin_radio_proc,      40,      34+32,   81,   9,      vc(14),             vc(1),              0,   0,          0,  0, (void *) "Clear Missing (--) Slots", NULL, NULL },
	{ jwin_radio_proc,      40,      34+48,   81,   9,      vc(14),             vc(1),              0,   0,          0,  0, (void *) "Clear Preserved (++) Slots", NULL, NULL },
	{ jwin_radio_proc,      40,      34+64,   81,   9,      vc(14),             vc(1),              0,   0,          0,  0, (void *) "Clear Imported (==) Slots", NULL, NULL },
	{ jwin_radio_proc,      40,      34+80,   81,   9,      vc(14),             vc(1),              0,   0,          0,  0, (void *) "Clear All", NULL, NULL },
    { d_timer_proc,         0,       0,       0,    0,      0,                  0,                  0,   0,          0,  0, NULL, NULL, NULL },
    { NULL,                 0,       0,       0,    0,      0,                  0,                  0,   0,          0,  0, NULL, NULL, NULL }
};

void doClearSlots(byte* flags)
{
	//{ Setup
	clearslots_dlg[0].dp2=get_zc_font(font_lfont);
	clearslots_dlg[3].d1 = get_selected_tab((TABPANEL*)assignscript_dlg[1].dp); //Default to current tab's type
	clearslots_dlg[4].flags |= D_SELECTED;
	clearslots_dlg[5].flags &= ~D_SELECTED;
	clearslots_dlg[6].flags &= ~D_SELECTED;
	clearslots_dlg[7].flags &= ~D_SELECTED;
	clearslots_dlg[8].flags &= ~D_SELECTED;
	if(((*flags) & SLOTMSGFLAG_MISSING) == 0)
		clearslots_dlg[5].flags |= D_DISABLED;
	else
		clearslots_dlg[5].flags &= ~D_DISABLED;
	if(((*flags) & SLOTMSGFLAG_PRESERVED) == 0)
		clearslots_dlg[6].flags |= D_DISABLED;
	else
		clearslots_dlg[6].flags &= ~D_DISABLED;
	if(((*flags) & SLOTMSGFLAG_IMPORTED) == 0)
		clearslots_dlg[7].flags |= D_DISABLED;
	else
		clearslots_dlg[7].flags &= ~D_DISABLED;
	//}
	
	large_dialog(clearslots_dlg);
	
	if(do_zqdialog(clearslots_dlg,2)==1)
	{
		int32_t q = 3;
		while((clearslots_dlg[++q].flags & D_SELECTED) == 0);
		switch(q)
		{
			case 4: //Clear type
			{
				clearAllSlots(clearslots_dlg[3].d1);
				break;
			}
			case 5: //Clear Missing
			{
				for(int32_t q = 0; q <= 10; ++q)
					clearAllSlots(q,SLOTMSGFLAG_MISSING);
				break;
			}
			case 6: //Clear Preserved
			{
				for(int32_t q = 0; q <= 10; ++q)
					clearAllSlots(q,SLOTMSGFLAG_PRESERVED);
				break;
			}
			case 7: //Clear Imported ZASM
			{
				for(int32_t q = 0; q <= 10; ++q)
					clearAllSlots(q,SLOTMSGFLAG_IMPORTED);
				break;
			}
			case 8: //Clear ALL
			{
				for(int32_t q = 0; q <= 10; ++q)
					clearAllSlots(q);
				break;
			}
		}
	}
}

static DIALOG exportzasm_dlg[] =
{
    { jwin_win_proc,        0,       0,       200,  159,    vc(14),             vc(1),              0,   D_EXIT,     0,  0, (void *) "Export ZASM", NULL, NULL },
    { jwin_button_proc,     35,      132,     61,   21,     vc(14),             vc(1),              13,  D_EXIT,     0,  0, (void *) "Confirm", NULL, NULL },
    { jwin_button_proc,     104,     132,     61,   21,     vc(14),             vc(1),              27,  D_EXIT,     0,  0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   50,      28+16,   100,  16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   D_EXIT,     0,  0, (void *) &slottype_sel_list, NULL, NULL },
    { jwin_droplist_proc,   50,      28+48,   100,  16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   0,          0,  0, NULL, NULL, NULL },
    { jwin_text_proc,       50,      28+8,    16,   8,      vc(11),             vc(1),              0,   0,          0,  0, (void *) "Script Type:", NULL, NULL },
    { jwin_text_proc,       50,      28+40,   16,   8,      vc(11),             vc(1),              0,   0,          0,  0, (void *) "Script Slot:", NULL, NULL },
    { NULL,                 0,       0,       0,    0,      0,                  0,                  0,   0,          0,  0, NULL, NULL, NULL }
};

static DIALOG importzasm_dlg[] =
{
    { jwin_win_proc,        0,       0,       200,  159,    vc(14),             vc(1),              0,   D_EXIT,     0,  0, (void *) "Import ZASM", NULL, NULL },
    { jwin_button_proc,     35,      132,     61,   21,     vc(14),             vc(1),              13,  D_EXIT,     0,  0, (void *) "Confirm", NULL, NULL },
    { jwin_button_proc,     104,     132,     61,   21,     vc(14),             vc(1),              27,  D_EXIT,     0,  0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   50,      28+16,   100,  16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   D_EXIT,     0,  0, (void *) &slottype_sel_list, NULL, NULL },
    { jwin_droplist_proc,   50,      28+48,   100,  16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   0,          0,  0, NULL, NULL, NULL },
    // 5
	{ jwin_text_proc,       50,      28+8,    16,   8,      vc(11),             vc(1),              0,   0,          0,  0, (void *) "Script Type:", NULL, NULL },
    { jwin_text_proc,       50,      28+40,   16,   8,      vc(11),             vc(1),              0,   0,          0,  0, (void *) "Script Slot:", NULL, NULL },
    { jwin_text_proc,       50,      28+72,   16,   8,      vc(11),             vc(1),              0,   0,          0,  0, (void *) "Script Name:", NULL, NULL },
	{ jwin_edit_proc,       50,      28+80,   100,  16,     jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,   0,          19, 0, NULL, NULL, NULL },
	
    { NULL,                 0,       0,       0,    0,      0,                  0,                  0,   0,          0,  0, NULL, NULL, NULL }
};
extern ListData itemscript_list;
extern ListData itemspritescript_list;
extern ListData lweaponscript_list;
extern ListData npcscript_list;
extern ListData eweaponscript_list;
extern ListData comboscript_list;

static EXT_LIST zasm_extlist[] =
{
	{ (char *)"ZASM Files (*.zasm)",                        (char *)"zasm"                                     },
	{ NULL,                                                  NULL                                              }
};

int32_t onImportZASM()
{
	importzasm_dlg[0].dp2 = get_zc_font(font_lfont);
	importzasm_dlg[4].dp = (void*)&ffscript_list;
	if(!prompt_for_existing_file_compat("Import Script (.zasm)","zasm",zasm_extlist,datapath,false))
	{
		return D_O_K;
	}
	script_data *temp_slot = new script_data(ScriptType::None, 0);
	if(parse_script_file(temp_slot, temppath, false) == D_CLOSE)
	{
		jwin_alert("Error","Failed to parse specified file!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
		delete temp_slot;
		return D_O_K;
	}

    std::string namebuf;
	if(temp_slot->meta.valid()) //Found metadata
	{
		importzasm_dlg[3].d1 = getType(temp_slot->meta.script_type);
		namebuf = temp_slot->meta.script_name;
		switch(importzasm_dlg[3].d1)
		{
			default: //Shouldn't occur, but to be safe
			case type_ffc:
				importzasm_dlg[4].dp = (void*)&ffscript_sel_dlg_list;
				break;
			case type_global:
				importzasm_dlg[4].dp = (void*)&gscript_sel_dlg_list;
				break;
			case type_itemdata:
				importzasm_dlg[4].dp = (void*)&itemscript_sel_dlg_list;
				break;
			case type_npc:
				importzasm_dlg[4].dp = (void*)&npcscript_sel_dlg_list;
				break;
			case type_lweapon:
				importzasm_dlg[4].dp = (void*)&lweaponscript_sel_dlg_list;
				break;
			case type_eweapon:
				importzasm_dlg[4].dp = (void*)&eweaponscript_sel_dlg_list;
				break;
			case type_hero:
				importzasm_dlg[4].dp = (void*)&playerscript_sel_dlg_list;
				break;
			case type_dmap:
				importzasm_dlg[4].dp = (void*)&dmapscript_sel_dlg_list;
				break;
			case type_screen:
				importzasm_dlg[4].dp = (void*)&screenscript_sel_dlg_list;
				break;
			case type_itemsprite:
				importzasm_dlg[4].dp = (void*)&itemspritescript_sel_dlg_list;
				break;
			case type_combo:
				importzasm_dlg[4].dp = (void*)&comboscript_sel_dlg_list;
				break;
		}
		importzasm_dlg[4].d1 = 0;
	}
	else
	{
		importzasm_dlg[3].d1 = 0;
		importzasm_dlg[4].dp = (void*)&ffscript_list;
		importzasm_dlg[4].d1 = 0;
	}
	importzasm_dlg[8].dp = (void*)namebuf.c_str();
	bool confirmed = false;
	int32_t indx = 1;
	while(!confirmed)
	{
		large_dialog(importzasm_dlg);
		indx = do_zqdialog(importzasm_dlg, indx);
		switch(indx)
		{
			case 1: //confirm; exit dlg
			{
				if(!namebuf[0]) break; //No name?
				script_data **slot = NULL;
				script_slot_data *map = NULL;
				//{ Find script choice
				int32_t scriptInd = importzasm_dlg[4].d1;
				switch(importzasm_dlg[3].d1)
				{
					case type_ffc:
						slot = &ffscripts[scriptInd];
						map = &ffcmap[scriptInd];
						break;
					case type_global:
						slot = &globalscripts[scriptInd];
						map = &globalmap[scriptInd];
						break;
					case type_itemdata:
						slot = &itemscripts[scriptInd];
						map = &itemmap[scriptInd];
						break;
					case type_npc:
						slot = &guyscripts[scriptInd];
						map = &npcmap[scriptInd];
						break;
					case type_lweapon:
						slot = &lwpnscripts[scriptInd];
						map = &lwpnmap[scriptInd];
						break;
					case type_eweapon:
						slot = &ewpnscripts[scriptInd];
						map = &ewpnmap[scriptInd];
						break;
					case type_hero:
						slot = &playerscripts[scriptInd];
						map = &playermap[scriptInd];
						break;
					case type_dmap:
						slot = &dmapscripts[scriptInd];
						map = &dmapmap[scriptInd];
						break;
					case type_screen:
						slot = &screenscripts[scriptInd];
						map = &screenmap[scriptInd];
						break;
					case type_itemsprite:
						slot = &itemspritescripts[scriptInd];
						map = &itemspritemap[scriptInd];
						break;
					case type_combo:
						slot = &comboscripts[scriptInd];
						map = &comboscriptmap[scriptInd];
						break;
					case type_generic:
						slot = &genericscripts[scriptInd];
						map = &genericmap[scriptInd];
						break;
					case type_subscreen:
						slot = &subscreenscripts[scriptInd];
						map = &subscreenmap[scriptInd];
						break;
				}
				//}
				if(!slot) break; //Not found?
				*slot = std::move(temp_slot);
				map->format = SCRIPT_FORMAT_ZASM;
				map->updateName(namebuf);
				confirmed = true;
				break;
			}
			case 0: case 2: //Close dlg
			{
				delete temp_slot;
				return D_O_K;
			}
			case 3: //Type select
			{
				switch(importzasm_dlg[3].d1)
				{
					default: //Shouldn't occur, but to be safe
					case type_ffc:
						importzasm_dlg[4].dp = (void*)&ffscript_sel_dlg_list;
						break;
					case type_global:
						importzasm_dlg[4].dp = (void*)&gscript_sel_dlg_list;
						break;
					case type_itemdata:
						importzasm_dlg[4].dp = (void*)&itemscript_sel_dlg_list;
						break;
					case type_npc:
						importzasm_dlg[4].dp = (void*)&npcscript_sel_dlg_list;
						break;
					case type_lweapon:
						importzasm_dlg[4].dp = (void*)&lweaponscript_sel_dlg_list;
						break;
					case type_eweapon:
						importzasm_dlg[4].dp = (void*)&eweaponscript_sel_dlg_list;
						break;
					case type_hero:
						importzasm_dlg[4].dp = (void*)&playerscript_sel_dlg_list;
						break;
					case type_dmap:
						importzasm_dlg[4].dp = (void*)&dmapscript_sel_dlg_list;
						break;
					case type_screen:
						importzasm_dlg[4].dp = (void*)&screenscript_sel_dlg_list;
						break;
					case type_itemsprite:
						importzasm_dlg[4].dp = (void*)&itemspritescript_sel_dlg_list;
						break;
					case type_combo:
						importzasm_dlg[4].dp = (void*)&comboscript_sel_dlg_list;
						break;
				}
				importzasm_dlg[4].d1 = 0;
				break;
			}
		}
	}
	delete temp_slot;
	return D_O_K;
}

void center_zscript_dialogs()
{
    jwin_center_dialog(exportzasm_dlg);
    jwin_center_dialog(importzasm_dlg);
    jwin_center_dialog(clearslots_dlg);
}

// array of voices, one for each sfx sample in the data file
// 0+ = voice #
// -1 = voice not allocated
int32_t sfx_voice[WAV_COUNT];

void Z_init_sound()
{
    for(int32_t i=0; i<WAV_COUNT; i++)
        sfx_voice[i]=-1;
        
//  master_volume(digi_volume,midi_volume);
}

// returns number of voices currently allocated
int32_t sfx_count()
{
    int32_t c=0;
    
    for(int32_t i=0; i<WAV_COUNT; i++)
        if(sfx_voice[i]!=-1)
            ++c;
            
    return c;
}

// clean up finished samples
void sfx_cleanup()
{
    for(int32_t i=0; i<WAV_COUNT; i++)
        if(sfx_voice[i]!=-1 && voice_get_position(sfx_voice[i])<0)
        {
            deallocate_voice(sfx_voice[i]);
            sfx_voice[i]=-1;
        }
}

// allocates a voice for the sample "wav_index" (index into zelda.dat)
// if a voice is already allocated (and/or playing), then it just returns true
// Returns true:  voice is allocated
//         false: unsuccessful
SAMPLE templist[WAV_COUNT];

bool sfx_init(int32_t index)
{
    // check index
    if(index<1 || index>=WAV_COUNT)
        return false;
        
    if(sfx_voice[index]==-1)
    {
        sfx_voice[index]=allocate_voice(&templist[index]);
    }
    
    return sfx_voice[index] != -1;
}

// plays an sfx sample
void sfx(int32_t index,int32_t pan,bool loop,bool restart,int32_t vol,int32_t freq)
{
    if(!sfx_init(index))
        return;
        
    voice_set_playmode(sfx_voice[index],loop?PLAYMODE_LOOP:PLAYMODE_PLAY);
    voice_set_pan(sfx_voice[index],pan);
    
    int32_t pos = voice_get_position(sfx_voice[index]);
    
    if(restart) voice_set_position(sfx_voice[index],0);
    
    if(pos<=0)
        voice_start(sfx_voice[index]);
}

// start it (in loop mode) if it's not already playing,
// otherwise just leave it in its current position
void cont_sfx(int32_t index)
{
    if(!sfx_init(index))
        return;
        
    if(voice_get_position(sfx_voice[index])<=0)
    {
        voice_set_position(sfx_voice[index],0);
        voice_set_playmode(sfx_voice[index],PLAYMODE_LOOP);
        voice_start(sfx_voice[index]);
    }
}

// adjust parameters while playing
void adjust_sfx(int32_t index,int32_t pan,bool loop)
{
    if(index<0 || index>=WAV_COUNT || sfx_voice[index]==-1)
        return;
        
    voice_set_playmode(sfx_voice[index],loop?PLAYMODE_LOOP:PLAYMODE_PLAY);
    voice_set_pan(sfx_voice[index],pan);
}

// pauses a voice
void pause_sfx(int32_t index)
{
    if(index>=0 && index<WAV_COUNT && sfx_voice[index]!=-1)
        voice_stop(sfx_voice[index]);
}

// resumes a voice
void resume_sfx(int32_t index)
{
    if(index>=0 && index<WAV_COUNT && sfx_voice[index]!=-1)
        voice_start(sfx_voice[index]);
}

// pauses all active voices
void pause_all_sfx()
{
    for(int32_t i=0; i<WAV_COUNT; i++)
        if(sfx_voice[i]!=-1)
            voice_stop(sfx_voice[i]);
}

// resumes all paused voices
void resume_all_sfx()
{
    for(int32_t i=0; i<WAV_COUNT; i++)
        if(sfx_voice[i]!=-1)
            voice_start(sfx_voice[i]);
}

// stops an sfx and deallocates the voice
void stop_sfx(int32_t index)
{
    if(index<0 || index>=WAV_COUNT)
        return;
        
    if(sfx_voice[index]!=-1)
    {
        deallocate_voice(sfx_voice[index]);
        sfx_voice[index]=-1;
    }
}

void kill_sfx()
{
    for(int32_t i=0; i<WAV_COUNT; i++)
        if(sfx_voice[i]!=-1)
        {
            deallocate_voice(sfx_voice[i]);
            sfx_voice[i]=-1;
        }
}

int32_t pan(int32_t x)
{
	return 128;
	/*switch(pan_style)
	{
		case 0: return 128;
		case 1: return vbound((x>>1)+68,0,255);
		case 2: return vbound(((x*3)>>2)+36,0,255);
	}
	return vbound(x,0,255);*/
}


void change_sfx(SAMPLE *sfx1, SAMPLE *sfx2)
{
    sfx1->bits = sfx2->bits;
    sfx1->stereo = sfx2->stereo;
    sfx1->freq = sfx2->freq;
    sfx1->priority = sfx2->priority;
    sfx1->len = sfx2->len;
    sfx1->loop_start = sfx2->loop_start;
    sfx1->loop_end = sfx2->loop_end;
    sfx1->param = sfx2->param;
    
    if(sfx1->data != NULL)
    {
        free(sfx1->data);
    }
    
    if(sfx2->data == NULL)
        sfx1->data = NULL;
    else
    {
        // When quests are saved and loaded, data is written in words.
        // If the last byte is dropped, it'll cause the sound to end with
        // a click. It could simply be extended and padded with 0, but
        // that causes compatibility issues... So we'll cut off
        // the last byte and decrease the length.
        
        int32_t len = (sfx1->bits==8?1:2)*(sfx1->stereo == 0 ? 1 : 2)*sfx1->len;
        
        while(len%sizeof(word))
        {
            // sizeof(word) should be 2, so this doesn't really need
            // to be a loop, but what the heck.
            sfx1->len--;
            len = (sfx1->bits==8?1:2)*(sfx1->stereo == 0 ? 1 : 2)*sfx1->len;
        }
        
        sfx1->data = malloc(len);
        memcpy(sfx1->data, sfx2->data, len);
    }
}

bool confirmBox(const char *m1, const char *m2, const char *m3)
{
	if(!m3)
	{
		if(!m2) m2 = "Are you sure?";
		else m3 = "Are you sure?";
	}
	return jwin_alert("Confirmation", m1, m2, m3, "Yes", "No", 'y', 'n', get_zc_font(font_lfont)) == 1;
}

int32_t onSelectSFX()
{
    SFXListerDialog(0).show();
    refresh(rMAP+rCOMBOS);
    return D_O_K;
}

bool saveWAV(int32_t slot, const char *filename)
{
    if (slot < 1 || slot >= 511 )
        return false;

    if (customsfxdata[slot].data == NULL)
	return false;
    
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
        return false;
    ofs.write("RIFF",4);
    uint32_t samplerate = customsfxdata[slot].freq;
    uint16_t channels = customsfxdata[slot].stereo ? 2 : 1;
    uint32_t datalen = customsfxdata[slot].len*channels*customsfxdata[slot].bits / 8;
    uint32_t size = 36 + datalen;
    ofs.write((char *)&size, 4);
    ofs.write("WAVE", 4);
    ofs.write("fmt ", 4);
    uint32_t fmtlen = 16;
    ofs.write((char *)&fmtlen, 4);
    uint16_t type = 1;
    ofs.write((char *)&type, 2);
    ofs.write((char *)&channels, 2);
    ofs.write((char *)&samplerate, 4);
    uint32_t bytespersec = samplerate*channels*customsfxdata[slot].bits / 8; 
    ofs.write((char *)&bytespersec, 4);
    uint16_t blockalign = channels*customsfxdata[slot].bits / 8;
    ofs.write((char *)&blockalign, 2);
    uint16_t bitspersample = customsfxdata[slot].bits;
    ofs.write((char *)&bitspersample, 2);
    ofs.write("data", 4);
    ofs.write((char *)&datalen, 4);
    if (bitspersample == 8)
    {
        for (int32_t i = 0; i < (int32_t)customsfxdata[slot].len*channels; i++)
        {
            char data = ((char *)customsfxdata[slot].data)[i];
            data ^= 0x80;
            ofs.write(&data, 1);
        }
    }
    else if (bitspersample == 16)
    {
        for (int32_t i = 0; i < (int32_t)customsfxdata[slot].len*channels; i++)
        {
            uint16_t data = ((uint16_t *)customsfxdata[slot].data)[i];
            data ^= 0x8000;
            ofs.write((char *)&data, 2);
        }
    }
    else
        return false;
    return !!ofs;
} 

int32_t onEditSFX(int32_t index)
{
	call_sfxdata_dialog(index);
	return D_O_K;
}

int32_t onMapStyles()
{
	call_mapstyles_dialog();
	return D_O_K;
}

int32_t d_misccolors_old_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    if(msg==MSG_DRAW)
    {
        textout_ex(screen,font,"0123456789ABCDEF",d->x+8,d->y,d->fg,d->bg);
        textout_ex(screen,font,"0",d->x,d->y+8,d->fg,d->bg);
        textout_ex(screen,font,"1",d->x,d->y+16,d->fg,d->bg);
        textout_ex(screen,font,"5",d->x,d->y+24,d->fg,d->bg);
        
        for(int32_t i=0; i<32; i++)
        {
            int32_t px2 = d->x+((i&15)<<3)+8;
            int32_t py2 = d->y+((i>>4)<<3)+8;
            rectfill(screen,px2,py2,px2+7,py2+7,i);
        }
        
        for(int32_t i=0; i<16; i++)
        {
            int32_t px2 = d->x+(i<<3)+8;
            rectfill(screen,px2,d->y+24,px2+7,d->y+31,i+80);
        }
    }
    
    return D_O_K;
}

int32_t hexclicked=-1;

int32_t d_misccolors_hexedit_proc(int32_t msg,DIALOG *d,int32_t c)
{
    switch(msg)
    {
    case MSG_GOTFOCUS:
        hexclicked=((int32_t)(size_t)(d->dp3))+20;
        break;
        
    case MSG_LOSTFOCUS:
        hexclicked=-1;
        break;
    }
    
    return d_hexedit_proc(msg,d,c);
}


int32_t d_misccolors_proc(int32_t msg,DIALOG *d,int32_t c);

static int32_t misccolor1_list[] =
{
    // dialog control number
    4, 5, 6, 7, 8, 20, 21, 22, 23, 24, 36, 37, 38, 39, 40, -1
};

static int32_t misccolor2_list[] =
{
    // dialog control number
    9, 10, 11, 12, 13, 25, 26, 27, 28, 29, 41, 42, 43, 44, 45, -1
};

static int32_t misccolor3_list[] =
{
    // dialog control number
    14, 15, 16, 17, 18, 30, 31, 32, 33, 34, 46, 47, 48, 49, 50, -1
};

static int32_t misccolor4_list[] =
{
    19, 35, 51, 54, 55, 56, -1
};

static TABPANEL misccolor_tabs[] =
{
    // (text)
    { (char *)"1",  D_SELECTED,  misccolor1_list, 0, NULL },
    { (char *)"2",  0,           misccolor2_list, 0, NULL },
    { (char *)"3",  0,           misccolor3_list, 0, NULL },
    { (char *)"4",  0,           misccolor4_list, 0, NULL },
    { NULL,         0,           NULL, 0, NULL }
};

int32_t d_misccolors_tab_proc(int32_t msg,DIALOG *d,int32_t c);

static DIALOG misccolors_dlg[] =
{
    // (dialog proc)        (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,        2,   21,    316,  197-23,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Misc Colors", NULL, NULL },
    //  { jwin_frame_proc,      98-84+1+2,   52+8-6+4,   132,  100,  vc(15),  vc(1),  0,       0,          FR_DEEP,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_misccolors_proc,    92-84+1+2,   44+8-6+4,   128+8,  96+8,   vc(9),   vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //3
    { d_misccolors_tab_proc,  150+14-2+10-15,   60-14,  150-10+15,  144-20-10,    jwin_pal[jcBOXFG],  jwin_pal[jcBOX],      0,      0,          0,             0, (void *) misccolor_tabs, NULL, (void *)misccolors_dlg },
    //4
    { jwin_text_proc,       215-25-12-15,    76-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Text:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    94-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Caption:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    112-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Overworld Minmap:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    130-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    148-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Foreground 1:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Foreground 2:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "BS Minimap Dark:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   112-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "BS Minimap Goal:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   130-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Compass Mark (Light):", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   148-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Compass Mark (Dark):", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Subscreen Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Subscreen Shadow:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   112-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Triforce Frame:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   130-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Big Map Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   148-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Big Map Foreground:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Hero's Position:", NULL, NULL },
    
    //20
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)0, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)1, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)2, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)3, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)4, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)5, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)6, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)7, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)8, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)9, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)10, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)11, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)12, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)13, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)14, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)15, },
    
    //36
    { jwin_text_proc,       283-25+14+2,    76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    
    //52
    { jwin_button_proc,     90,   190-20,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  190-20,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Message Text:", NULL, NULL },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)35, },
    { jwin_text_proc,		  283-25+14+2,   94-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int32_t d_misccolors_tab_proc(int32_t msg,DIALOG *d,int32_t c)
{

    switch(msg)
    {
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        break;
    }
    
    return jwin_tab_proc(msg,d,c);
}


int32_t d_misccolors_proc(int32_t msg,DIALOG *d,int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    int32_t mul=12;
        
    switch(msg)
    {
    case MSG_CLICK:
        if(hexclicked!=-1)
        {
            int32_t color_col=vbound(((gui_mouse_x()-d->x-8)/mul),0,15);
            int32_t color_row=vbound(((gui_mouse_y()-d->y-10)/mul),0,11);
            sprintf((char*)misccolors_dlg[hexclicked].dp,"%X%X",color_row,color_col);
            object_message(misccolors_dlg+hexclicked,MSG_DRAW,0);
        }
        
        break;
        
    case MSG_DRAW:
        for(int32_t i=0; i<10; i++)
        {
            textprintf_centre_ex(screen,font,d->x+8+4+(i*mul),d->y,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%d", i);
        }
        
        for(int32_t i=0; i<6; i++)
        {
            textprintf_centre_ex(screen,font,d->x+8+4+((10+i)*mul),d->y,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%c", i+'A');
        }
        
        for(int32_t i=0; i<10; i++)
        {
            textprintf_right_ex(screen,font,d->x+6,d->y+(i*mul)+10,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%d", i);
        }
        
        for(int32_t i=0; i<2; i++)
        {
            textprintf_right_ex(screen,font,d->x+6,d->y+((i+10)*mul)+10,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%c", i+'A');
        }
        
        jwin_draw_frame(screen,d->x+6,d->y+8,int32_t(132*1.5)-2,int32_t(100*1.5)-2,FR_DEEP);
        
        for(int32_t i=0; i<192; i++)
        {
            int32_t px2 = d->x+int32_t(((i&15)<<3)*1.5)+8;
            int32_t py2 = d->y+int32_t(((i>>4)<<3)*1.5)+8+2;
            rectfill(screen,px2,py2,px2+(mul-1),py2+(mul-1),i);
        }
        
        break;
    }
    
    return D_O_K;
}


int32_t onMiscColors()
{
    char buf[17][3];
    byte *si = &(QMisc.colors.text);
    misccolors_dlg[0].dp2=get_zc_font(font_lfont);
    
    for(int32_t i=0; i<16; i++)
    {
        sprintf(buf[i],"%02X",*(si++));
        sprintf(buf[16], "%02X", QMisc.colors.msgtext);
        misccolors_dlg[i+20].dp = buf[i];
        misccolors_dlg[55].dp = buf[16];
    }
    
    large_dialog(misccolors_dlg);
        
    if(do_zqdialog(misccolors_dlg,0)==52)
    {
        saved=false;
        si = &(QMisc.colors.text);
        
        for(int32_t i=0; i<16; i++)
        {
            *si = zc_xtoi(buf[i]);
            ++si;
        }
        
        QMisc.colors.msgtext = zc_xtoi(buf[16]);
    }
    
    return D_O_K;
}

// ****  Palette cycling  ****

static int32_t palclk[3];
static int32_t palpos[3];

void reset_pal_cycling()
{
    for(int32_t i=0; i<3; i++)
        palclk[i]=palpos[i]=0;
}

void cycle_palette()
{
    if(!get_qr(qr_FADE))
        return;
        
    int32_t level = Map.CurrScr()->color;
    bool refreshpal = false;
    
    for(int32_t i=0; i<3; i++)
    {
        palcycle c = QMisc.cycles[level][i];
        
        if(c.count&0xF0)
        {
            if(++palclk[i] >= c.speed)
            {
                palclk[i]=0;
                
                if(++palpos[i] >= (c.count>>4))
                    palpos[i]=0;
                    
                byte *si = colordata + CSET(level*pdLEVEL+poFADE1+1+palpos[i])*3;
                
                si += (c.first&15)*3;
                
                for(int32_t col=c.first&15; col<=(c.count&15); col++)
                {
                    RAMpal[CSET(c.first>>4)+col] = _RGB(si);
                    si+=3;
                }
                
                refreshpal = true;
            }
        }
    }
    
    if(refreshpal)
    {
        rebuild_trans_table();
        zc_set_palette_range(RAMpal,0,192,false);
    }
}


void doHelp()
{
	do_box_edit(helpstr, "ZQuest Help", true, true);
}

int32_t onHelp()
{
    restore_mouse();
    doHelp();
    return D_O_K;
}

void doZstringshelp()
{
	do_box_edit(zstringshelpstr, "ZStrings Help", true, true);
}

int32_t onZstringshelp()
{
    restore_mouse();
    doZstringshelp();
    return D_O_K;
}

static DIALOG layerdata_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     16-12,   20+32,   288+1+24,  200+1-32-16,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Layer Data", NULL, NULL },
    { jwin_button_proc,     170,  180,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     90,   180,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    // 3
    { jwin_rtext_proc,       72,   88,    40,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Map:", NULL, NULL },
    { jwin_rtext_proc,       72,   88+18,    48,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Screen:", NULL, NULL },
    { jwin_rtext_proc,       72,   88+36,    56,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Transparent:", NULL, NULL },
    { jwin_ctext_proc,       89,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { jwin_ctext_proc,       89+40,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "2", NULL, NULL },
    { jwin_ctext_proc,       89+80,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "3", NULL, NULL },
    { jwin_ctext_proc,       89+120,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "4", NULL, NULL },
    { jwin_ctext_proc,       89+160,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "5", NULL, NULL },
    { jwin_ctext_proc,       89+200,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "6", NULL, NULL },
    
    //12
    { jwin_edit_proc,      76,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76,   76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+40,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+40,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+40,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+80,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+80,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+80,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+120,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+120,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+120,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+160,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+160,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+160,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+200,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+200,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+200,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    //30
    { jwin_button_proc,     76,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+40,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+80,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+120,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+160,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+200,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
    
};

int32_t edit_layers(mapscr* tempscr)
{
    char buf[6][2][8];
    layerdata_dlg[0].dp2 = get_zc_font(font_lfont);
    
    for(int32_t x=0; x<6; x++)
    {
        sprintf(buf[x][0],"%d",tempscr->layermap[x]);
        sprintf(buf[x][1],"%02X",tempscr->layerscreen[x]);
    }
    
    for(int32_t x=0; x<6; x++)
    {
        for(int32_t y=0; y<2; y++)
        {
            layerdata_dlg[(x*3)+y+12].dp = buf[x][y];
        }
    }
    
    for(int32_t x=0; x<6; x++)
    {
        layerdata_dlg[(x*3)+2+12].flags = (tempscr->layeropacity[x]<255) ? D_SELECTED : 0;
    }
    
    large_dialog(layerdata_dlg);
        
    int32_t ret=do_zqdialog(layerdata_dlg,0);
    
    if(ret>=2)
    {
        for(int32_t x=0; x<6; x++)
        {
        
            tempscr->layermap[x]=atoi(buf[x][0]);
            
            if(tempscr->layermap[x]>map_count)
            {
                tempscr->layermap[x]=0;
            }
            
            tempscr->layerscreen[x]=zc_xtoi(buf[x][1]);
            
            if(zc_xtoi(buf[x][1])>=MAPSCRS)
            {
                tempscr->layerscreen[x]=0;
            }
            
            //      tempscr->layeropacity[x]=layerdata_dlg[(x*9)+8+19].flags & D_SELECTED ? 128:255;
            tempscr->layeropacity[x]=layerdata_dlg[(x*3)+2+12].flags & D_SELECTED ? 128:255;
        }
        
        //  } else if (ret>72&&ret<79) {
        //    return (ret-72);
    }
    
    return ret;
}

static DIALOG autolayer_dlg[] =
{
	/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
	{ jwin_win_proc,        64,   32+48,   192+1,  184+1-64,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Autolayer Setup", NULL, NULL },
	{ jwin_text_proc,       76,   56+48,   136,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Map for layer ?: ", NULL, NULL },
	{ jwin_edit_proc,       212,  56+48,   32,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
	{ jwin_check_proc,      76,   56+18+48,   153,   8,    vc(14),  vc(1),  0,       D_EXIT,          1,             0, (void *) "Only Blank Screens", NULL, NULL },
	{ jwin_button_proc,     90,   188-12,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
	
	//5
	{ jwin_button_proc,     170,  188-12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
	{ d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
	{ d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
	{ jwin_check_proc,      76,   56+28+48,   153,   8,    vc(14),  vc(1),  0,       D_EXIT,          1,             0, (void *) "Only Blank Layers", NULL, NULL },
	{ jwin_check_proc,      76,   56+38+48,   153,   8,    vc(14),  vc(1),  0,       D_EXIT,          1,             0, (void *) "Overwrite Layers", NULL, NULL },
	
	//10
	{ NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};
enum
{
	autolyr_blankscreens,
	autolyr_blanklayers,
	autolyr_any
};
void autolayer(mapscr* tempscr, int32_t layer, int32_t al[6][3])
{
	char tbuf[80],mlayer[80];
	autolayer_dlg[0].dp2=get_zc_font(font_lfont);
	sprintf(tbuf, "Map for layer %d: ", layer+1);
	autolayer_dlg[1].dp=tbuf;
	sprintf(mlayer, "%d", tempscr->layermap[layer]);
	autolayer_dlg[2].dp=mlayer;
	
	large_dialog(autolayer_dlg);
	int ret, sel = 8, fl = autolyr_blanklayers;
	bool running = true;
	do
	{
		SETFLAG(autolayer_dlg[3].flags, D_SELECTED, sel==3);
		SETFLAG(autolayer_dlg[8].flags, D_SELECTED, sel==8);
		SETFLAG(autolayer_dlg[9].flags, D_SELECTED, sel==9);
		switch(ret=do_zqdialog(autolayer_dlg,0))
		{
			case 4: //OK
			{
				int32_t lmap=vbound(atoi(mlayer),0,Map.getMapCount());
				al[layer][0]=lmap;
				tempscr->layermap[layer]=lmap;
				tempscr->layerscreen[layer]=Map.getCurrScr();
				al[layer][1]=fl;
				al[layer][2]=1;
				running = false;
				break;
			}
			case 0: case 5: //cancel
				running = false;
				break;
			case 3:
				sel = ret;
				fl = autolyr_blankscreens;
				break;
			case 8:
				sel = ret;
				fl = autolyr_blanklayers;
				break;
			case 9:
				sel = ret;
				fl = autolyr_any;
				break;
		}
	}
	while(ret != 0 && ret != 4 && ret != 5);
}

int32_t onLayers()
{
	mapscr tempscr=*Map.CurrScr();
	int32_t al[6][3]; //autolayer[layer][0=map, 1=autolyr_ type, 2=bool go]
	
	for(int32_t i=0; i<6; i++)
	{
		al[i][0]=tempscr.layermap[i];
		al[i][1]=0;
		al[i][2]=0;
	}
	
	int32_t ret;
	
	do
	{
		ret=edit_layers(&tempscr);
		
		if(ret>2)                                               //autolayer button
		{
			autolayer(&tempscr, ret-30, al);
		}
	}
	while(ret>2);                                             //autolayer button
	
	if(ret==2)                                                //OK
	{
		saved=false;
		TheMaps[Map.getCurrMap()*MAPSCRS+Map.getCurrScr()]=tempscr;
		
		for(int32_t i=0; i<6; i++)
		{
			int32_t tm=tempscr.layermap[i]-1;
			int32_t ts=tempscr.layerscreen[i];
			
			if(al[i][2])
			{
				map_autolayers[Map.getCurrMap()*6+i] = al[i][0];
				for(int32_t j=0; j<128; j++)
				{
					auto& curmapscr = TheMaps[Map.getCurrMap()*MAPSCRS+j];
					if(al[i][1] == autolyr_blankscreens && (curmapscr.valid&mVALID))
						continue;
					else if(al[i][1] == autolyr_blanklayers && curmapscr.layermap[i])
						continue;
					
					curmapscr.layermap[i]=al[i][0];
					curmapscr.layerscreen[i]=al[i][0]?j:0;
				}
			}
		}
	}
	
	// Check that the working layer wasn't just disabled
	if(CurrentLayer>0 && tempscr.layermap[CurrentLayer-1]==0)
		CurrentLayer=0;
	
	return D_O_K;
}


char *itoa(int32_t i)
{
    static char itoaret[500];
    sprintf(itoaret, "%d", i);
    return itoaret;
}

void fps_callback()
{
    lastfps=framecnt;
    framecnt=0;
}

END_OF_FUNCTION(fps_callback)

//uint32_t col_diff[3*128];
/*
  void bestfit_init(void)
  {
  int32_t i;

  for (i=1; i<64; i++)

  {
  int32_t k = i * i;
  col_diff[0  +i] = col_diff[0  +128-i] = k * (59 * 59);
  col_diff[128+i] = col_diff[128+128-i] = k * (30 * 30);
  col_diff[256+i] = col_diff[256+128-i] = k * (11 * 11);
  }
  }
  */
void create_rgb_table2(RGB_MAP *table, AL_CONST PALETTE pal_8bit, void (*callback)(int32_t pos))
{
#define UNUSED 65535
#define LAST 65532

   // Allegro has been modified to use an 8 bit palette, but this method and RGB_MAP still use 6 bit.
   PALETTE pal;
   for (int i = 0; i < 256; i++)
   {
      pal[i] = pal_8bit[i];
      pal[i].r /= 4;
      pal[i].g /= 4;
      pal[i].b /= 4;
   }

    /* macro add adds to single linked list */
#define add(i)    (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                                          (first != LAST ? (next[last] = (i)) : (first = (i))), \
                                          (last = (i))) : 0)
    
    /* same but w/o checking for first element */
#define add1(i)   (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                                          next[last] = (i), \
                                          (last = (i))) : 0)
    /* calculates distance between two colors */
#define dist(a1, a2, a3, b1, b2, b3) \
          (col_diff[ ((a2) - (b2)) & 0x7F] + \
           (col_diff + 128)[((a1) - (b1)) & 0x7F] + \
           (col_diff + 256)[((a3) - (b3)) & 0x7F])
    
    /* converts r,g,b to position in array and back */
#define pos(r, g, b) \
          (((r) / 2) * 32 * 32 + ((g) / 2) * 32 + ((b) / 2))
    
#define depos(pal, r, g, b) \
          ((b) = ((pal) & 31) * 2, \
           (g) = (((pal) >> 5) & 31) * 2, \
           (r) = (((pal) >> 10) & 31) * 2)
    
    /* is current color better than pal1? */
#define better(r1, g1, b1, pal1) \
          (((int32_t)dist((r1), (g1), (b1), \
                      (pal1).r, (pal1).g, (pal1).b)) > (int32_t)dist2)
    
    /* checking of position */
#define dopos(rp, gp, bp, ts) \
          if ((rp > -1 || r > 0) && (rp < 1 || r < 61) && \
              (gp > -1 || g > 0) && (gp < 1 || g < 61) && \
              (bp > -1 || b > 0) && (bp < 1 || b < 61)) \
        {                     \
          i = first + rp * 32 * 32 + gp * 32 + bp; \
          if (!data[i])       \
          {                   \
            data[i] = val;    \
            add1(i);          \
          }                   \
          else if ((ts) && (data[i] != val)) \
            {                 \
              dist2 = (rp ? (col_diff+128)[(r+2*rp-pal[val].r) & 0x7F] : r2) + \
                (gp ? (col_diff    )[(g+2*gp-pal[val].g) & 0x7F] : g2) + \
                (bp ? (col_diff+256)[(b+2*bp-pal[val].b) & 0x7F] : b2); \
              if (better((r+2*rp), (g+2*gp), (b+2*bp), pal[data[i]])) \
              {               \
                data[i] = val; \
                add1(i);      \
              }               \
            }                 \
        }
    
    int32_t i, curr, r, g, b, val, dist2;
    uint32_t r2, g2, b2;
    uint16_t next[32*32*32];
    uint8_t *data;
    int32_t first = LAST;
    int32_t last = LAST;
    int32_t count = 0;
    int32_t cbcount = 0;
    
#define AVERAGE_COUNT   18000
    
    if(col_diff[1] == 0)
        bestfit_init();
        
    memset(next, 255, sizeof(next));
    memset(table->data, 0, sizeof(char)*32*32*32);
    
    
    data = (uint8_t *)table->data;
    
    /* add starting seeds for floodfill */
    for(i=1; i<PAL_SIZE; i++)
    {
        curr = pos(pal[i].r, pal[i].g, pal[i].b);
        
        if(next[curr] == UNUSED)
        {
            data[curr] = i;
            add(curr);
        }
    }
    
    /* main floodfill: two versions of loop for faster growing in blue axis */
    //   while (first != LAST) {
    while(first < LAST)
    {
        depos(first, r, g, b);
        
        /* calculate distance of current color */
        val = data[first];
        r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
        g2 = (col_diff)[((pal[val].g)-(g)) & 0x7F];
        b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
        
        /* try to grow to all directions */
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
        dopos(0, 0, 1, 1);
        dopos(0, 0,-1, 1);
        dopos(1, 0, 0, 1);
        dopos(-1, 0, 0, 1);
        dopos(0, 1, 0, 1);
        dopos(0,-1, 0, 1);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
        
        /* faster growing of blue direction */
        if((b > 0) && (data[first-1] == val))
        {
            b -= 2;
            first--;
            b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
            
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
            dopos(-1, 0, 0, 0);
            dopos(1, 0, 0, 0);
            dopos(0,-1, 0, 0);
            dopos(0, 1, 0, 0);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
            
            first++;
        }
        
        /* get next from list */
        i = first;
        first = next[first];
        next[i] = UNUSED;
        
        /* second version of loop */
        //      if (first != LAST) {
        if(first < LAST)
        {
        
            depos(first, r, g, b);
            
            val = data[first];
            r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
            g2 = (col_diff)[((pal[val].g)-(g)) & 0x7F];
            b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
            
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
            dopos(0, 0, 1, 1);
            dopos(0, 0,-1, 1);
            dopos(1, 0, 0, 1);
            dopos(-1, 0, 0, 1);
            dopos(0, 1, 0, 1);
            dopos(0,-1, 0, 1);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
            
            if((b < 61) && (data[first + 1] == val))
            {
                b += 2;
                first++;
                b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7f];
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
                dopos(-1, 0, 0, 0);
                dopos(1, 0, 0, 0);
                dopos(0,-1, 0, 0);
                dopos(0, 1, 0, 0);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
                
                first--;
            }
            
            i = first;
            first = next[first];
            next[i] = UNUSED;
        }
        
        count++;
        
        if(count == (cbcount+1)*AVERAGE_COUNT/256)
        {
            if(cbcount < 256)
            {
                if(callback)
                    callback(cbcount);
                    
                cbcount++;
            }
        }
        
    }
    
    /* only the transparent (pink) color can be mapped to index 0 */
    if((pal[0].r == 63) && (pal[0].g == 0) && (pal[0].b == 63))
        table->data[31][0][31] = 0;
        
    if(callback)
        while(cbcount < 256)
            callback(cbcount++);
}

void rebuild_trans_table()
{
    create_rgb_table2(&zq_rgb_table, RAMpal, NULL);
    create_zc_trans_table(&trans_table, RAMpal, 128, 128, 128);
    memcpy(&trans_table2, &trans_table, sizeof(COLOR_MAP));
    
    for(int32_t q=0; q<PAL_SIZE; q++)
    {
        trans_table2.data[0][q] = q;
        trans_table2.data[q][q] = q;
    }
}

int32_t isFullScreen()
{
    return !is_windowed_mode();
}

void hit_close_button()
{
    close_button_quit=true;
    return;
}

extern bool dirty_screen;

void anim_hw_screen(bool force)
{
	// if (force || myvsync)
	{
		++cpoolbrush_index;
		
		if(prv_mode)
		{
			if(Map.get_prvtime())
			{
				Map.set_prvtime(Map.get_prvtime()-1);
				
				if(!Map.get_prvtime())
				{
					prv_warp=1;
				}
			}
		}
		if(AnimationOn)
		{
			animate_combos();
			update_freeform_combos();
		}
		
		if(CycleOn)
			cycle_palette();
	
		animate_coords();
		update_hw_screen();
	}
}

void custom_vsync()
{
	anim_hw_screen(true);
}

void switch_out()
{
	zcmusic_pause(zcmusic, ZCM_PAUSE);
	zc_midi_pause();
}

void switch_in()
{
	if(exiting_program)
		return;
	zcmusic_pause(zcmusic, ZCM_RESUME);
	zc_midi_resume();
}

void Z_eventlog(const char *format,...)
{
    format=format; //to prevent a compiler warning
}

int32_t get_currdmap()
{
    return zinit.start_dmap;
}

int32_t get_dlevel()
{
    return DMaps[zinit.start_dmap].level;
}

int32_t get_currscr()
{
    return Map.getCurrScr();
}

int32_t get_currmap()
{
    return Map.getCurrMap();
}

int32_t get_homescr()
{
    return DMaps[zinit.start_dmap].cont;
}

int current_item(int item_type, bool checkmagic, bool jinx_check, bool check_bunny)
{
    //TODO remove as special case?? -DD
    if(item_type==itype_shield)
    {
        return 2;
    }
    
	int id = current_item_id(item_type, checkmagic, jinx_check, check_bunny);
	return id > -1 ? itemsbuf[id].fam_type : 0;
}

int current_item_power(int itemtype, bool checkmagic, bool jinx_check, bool check_bunny)
{
	if (game)
	{
		int result = current_item_id(itemtype, checkmagic, jinx_check, check_bunny);
		return (result<0) ? 0 : itemsbuf[result].power;
	}
    return 1;
}

int32_t current_item_id(int32_t itemtype, bool, bool, bool)
{
	if (game)
	{
		int32_t result = -1;
		int32_t highestlevel = -1;

		for (int32_t i = 0; i < MAXITEMS; i++)
		{
			if ((zq_ignore_item_ownership || game->get_item(i)) && itemsbuf[i].family == itemtype)
			{
				if (itemsbuf[i].fam_type >= highestlevel)
				{
					highestlevel = itemsbuf[i].fam_type;
					result = i;
				}
			}
		}
		return result;
	}
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        if(itemsbuf[i].family==itemtype)
            return i;
    }
    
    return -1;
}


bool can_use_item(int32_t item_type, int32_t item)
{
    //these are here to bypass compiler warnings about unused arguments
    item_type=item_type;
    item=item;
    
    return true;
}

bool has_item(int32_t item_type, int32_t it)
{
    //these are here to bypass compiler warnings about unused arguments
    item_type=item_type;
    it=it;
    
    return true;
}

int32_t get_bmaps(int32_t si)
{
    //these are here to bypass compiler warnings about unused arguments
    si=si;
    
    return 255;
}

bool no_subscreen()
{
    return false;
}

static void allocate_crap()
{
	filepath=(char*)malloc(2048);
	datapath=(char*)malloc(2048);
	midipath=(char*)malloc(2048);
	imagepath=(char*)malloc(2048);
	tmusicpath=(char*)malloc(2048);
	last_timed_save=(char*)malloc(2048);
	
	if(!filepath || !datapath || !imagepath || !midipath || !tmusicpath || !last_timed_save)
	{
		Z_error_fatal("Error: no memory for file paths!");
	}
	

	customtunes = (zctune*)malloc(sizeof(zctune)*MAXCUSTOMMIDIS_ZQ);
	memset(customtunes, 0, sizeof(zctune)*MAXCUSTOMMIDIS_ZQ);

	for(int32_t i=0; i<MAXCUSTOMMIDIS_ZQ; ++i)
	{
		customtunes[i].data=NULL;
	}

	for(int32_t i=0; i<MAXCUSTOMTUNES; i++)
	{
		midi_string[i+4]=customtunes[i].title;
		screen_midi_string[i+5]=customtunes[i].title;
	}
	
	for(int32_t i=0; i<WAV_COUNT; i++)
	{
		if(sfx_string[i]!=NULL) delete sfx_string[i];
		customsfxdata[i].data=NULL;
		sfx_string[i] = new char[36];
		memset(sfx_string[i], 0, 36);
	}
	
	for(int32_t i=0; i<MAXWPNS; i++)
	{
		if(weapon_string[i]!=NULL) delete weapon_string[i];
		weapon_string[i] = new char[64];
		memset(weapon_string[i], 0, 64);
	}
	
	for(int32_t i=0; i<MAXITEMS; i++)
	{
		if(item_string[i]!=NULL) delete item_string[i];
		item_string[i] = new char[64];
		memset(item_string[i], 0, 64);
	}
	
	for(int32_t i=0; i<eMAXGUYS; i++)
	{
		if(guy_string[i]!=NULL) delete guy_string[i];
		guy_string[i] = new char[64];
		memset(guy_string[i], 0, 64);
	}
	
	for(int32_t i=0; i<NUMSCRIPTFFC; i++)
	{
		delete ffscripts[i];
		ffscripts[i] = new script_data(ScriptType::FFC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTITEM; i++)
	{
		delete itemscripts[i];
		itemscripts[i] = new script_data(ScriptType::Item, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
	{
		delete guyscripts[i];
		guyscripts[i] = new script_data(ScriptType::NPC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
	{
		delete screenscripts[i];
		screenscripts[i] = new script_data(ScriptType::Screen, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGLOBAL; i++)
	{
		delete globalscripts[i];
		globalscripts[i] = new script_data(ScriptType::Global, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTHERO; i++)
	{
		delete playerscripts[i];
		playerscripts[i] = new script_data(ScriptType::Hero, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		delete lwpnscripts[i];
		lwpnscripts[i] = new script_data(ScriptType::Lwpn, i);
	}
	 for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		delete ewpnscripts[i];
		ewpnscripts[i] = new script_data(ScriptType::Ewpn, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
	{
		delete dmapscripts[i];
		dmapscripts[i] = new script_data(ScriptType::DMap, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
	{
		delete itemspritescripts[i];
		itemspritescripts[i] = new script_data(ScriptType::ItemSprite, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
	{
		delete comboscripts[i];
		comboscripts[i] = new script_data(ScriptType::Combo, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSGENERIC; i++)
	{
		delete genericscripts[i];
		genericscripts[i] = new script_data(ScriptType::Generic, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
	{
		delete subscreenscripts[i];
		subscreenscripts[i] = new script_data(ScriptType::EngineSubscreen, i);
	}
}

// Removes the top layer encoding from a quest file. See open_quest_file.
// This has zero impact on the contents of the quest file. There should be no way for this to
// break anything.
static void do_unencrypt_qst_command(const char* input_filename, const char* output_filename)
{
	// If the file is already an unencrypted packfile, there's nothing to do.
	PACKFILE* pf_check = pack_fopen_password(input_filename, F_READ_PACKED, "");
	pack_fclose(pf_check);
	if (pf_check) return;

	int32_t error;
	PACKFILE* pf = open_quest_file(&error, input_filename, false);
	PACKFILE* pf2 = pack_fopen_password(output_filename, F_WRITE_PACKED, "");
	int c;
	while ((c = pack_getc(pf)) != EOF)
	{
		pack_putc(c, pf2);
	}
	pack_fclose(pf);
	pack_fclose(pf2);
	clear_quest_tmpfile();
}

// This will remove the PACKFILE compression. Incidentally, it also removes the top encoding layer.
static void do_uncompress_qst_command(const char* input_filename, const char* output_filename)
{
	auto unencrypted_result = try_open_maybe_legacy_encoded_file(input_filename, ENC_STR, nullptr, QH_NEWIDSTR, QH_IDSTR);
	if (unencrypted_result.not_found)
	{
		printf("qst not found\n");
		zq_exit(1);
	}
	if (!unencrypted_result.compressed && !unencrypted_result.encrypted)
	{
		// If the file is already an uncompressed file, there's nothing to do but copy it.
		fs::copy(input_filename, output_filename);
		return;
	}

	pack_fclose(unencrypted_result.decoded_pf);

	int32_t error;
	PACKFILE* pf = open_quest_file(&error, input_filename, false);
	PACKFILE* pf2 = pack_fopen_password(output_filename, F_WRITE, "");
	int c;
	while ((c = pack_getc(pf)) != EOF)
	{
		pack_putc(c, pf2);
	}
	pack_fclose(pf);
	pack_fclose(pf2);
	clear_quest_tmpfile();
}

// Copy a quest file by loading and resaving, exactly like if the user did it in the UI.
// Note there could be changes introduced in the loading or saving functions. These are
// typically for compatability, but could possibly be a source of bugs.
static void do_copy_qst_command(const char* input_filename, const char* output_filename)
{
	set_headless_mode();

	// We need to init some stuff before loading a quest file will work.
	int fake_errno = 0;
	allegro_errno = &fake_errno;
	get_qst_buffers();

	int ret = load_quest(input_filename, false);
	if (ret)
	{
		zq_exit(ret);
	}

	ret = save_quest(output_filename, false);
	zq_exit(ret);
}

int32_t Awpn=-1, Bwpn=-1, Xwpn = -1, Ywpn = -1;
sprite_list  guys, items, Ewpns, Lwpns, chainlinks, decorations, portals;
int32_t exittimer = 10000, exittimer2 = 100;

static bool partial_load_test(const char* test_dir)
{
	int ret = load_quest("quests/Z1 Recreations/classic_1st.qst", false);
	if (ret)
	{
		printf("failed to load classic_1st.qst: ret == %d\n", ret);
		return false;
	}

	int cont = DMaps[0].cont;

	// Skip same stuff as used in zq_tiles.cpp for grabbing tiles from a qst.
	byte skip_flags[4];
	for (int i=0; i<skip_max; ++i)
		set_bit(skip_flags,i,1);
	set_bit(skip_flags,skip_tiles,0);
	set_bit(skip_flags,skip_header,0);
	zquestheader tempheader;
	memset(&tempheader, 0, sizeof(zquestheader));
	auto ptux_path = fs::path(test_dir) / "quests/PTUX.qst";
	ret = loadquest(ptux_path.string().c_str(), &tempheader, &QMisc, customtunes, false, skip_flags);

	if (ret)
	{
		printf("failed to load PTUX.qst: ret == %d\n", ret);
		return false;
	}

	if (DMaps[0].cont != cont)
	{
		printf("unexpected modification: DMaps[0].cont == %d, should be %d\n", DMaps[0].cont, cont);
		return false;
	}

	// TODO should run replay. Currently, resaving classic_1st.qst fails its replay (see test_save in test_zeditor.py)

	return true;
}

template <typename ...Params>
[[noreturn]] void FatalConsole(const char *format, Params&&... params)
{
	FFCore.ZScriptConsole(CConsoleLoggerEx::COLOR_RED|CConsoleLoggerEx::COLOR_INTENSITY|CConsoleLoggerEx::COLOR_BACKGROUND_BLACK,"");
	Z_error_fatal(format, std::forward<Params>(params)...);
}

static BITMAP* load_asset_bmp(const char* path)
{
	BITMAP* bmp = load_bmp(path, nullptr);
	if (!bmp)
		Z_error_fatal("Failed to load required asset: %s\n", path);
	return bmp;
}

static void load_asset_pal(PALETTE pal, const char* path)
{
	BITMAP* bmp = load_bmp(path, pal);
	if (!bmp)
		Z_error_fatal("Failed to load required asset: %s\n", path);
}

static MIDI* load_asset_midi(const char* path)
{
	MIDI* midi = load_midi(path);
	if (!midi)
		Z_error_fatal("Failed to load required asset: %s\n", path);
	return midi;
}

BITMAP* asset_icons_bmp;
BITMAP* asset_engravings_bmp;
BITMAP* asset_mouse_bmp;
BITMAP* asset_select_bmp;
BITMAP* asset_arrows_bmp;
MIDI* asset_tunes_midi;
PALETTE asset_pal;

static void load_assets()
{
	asset_icons_bmp = load_asset_bmp("assets/editor/icons.bmp");
	asset_engravings_bmp = load_asset_bmp("assets/editor/engravings.bmp");
	asset_mouse_bmp = load_asset_bmp("assets/editor/mouse.bmp");
	asset_select_bmp = load_asset_bmp("assets/editor/select.bmp");
	asset_arrows_bmp = load_asset_bmp("assets/editor/arrows.bmp");
	asset_tunes_midi = load_asset_midi("assets/editor/tunes.mid");
	load_asset_pal(asset_pal, "assets/editor/pal.bmp");
}

static bool application_has_loaded;

int32_t main(int32_t argc,char **argv)
{
	int test_zc_arg = used_switch(argc, argv, "-test-zc");
	if (test_zc_arg > 0)
		set_headless_mode();

	zalleg_setup_allegro(App::zquest, argc, argv);
	allocate_crap();
	set_should_zprint_cb([]() {
		return get_qr(qr_SCRIPTERRLOG) || DEVLEVEL > 0;
	});

	int package_arg = used_switch(argc, argv, "-package");
	if (package_arg > 0)
	{
		if (package_arg + 3 > argc)
		{
			printf("%d\n", argc);
			printf("expected -package <game.qst> <package name>\n");
			zq_exit(1);
		}

		const char* input_filename = argv[package_arg + 1];
		const char* package_name = argv[package_arg + 2];
		if (auto error = package_create(input_filename, package_name))
			Z_error_fatal("%s\n", error->c_str());
		zq_exit(0);
	}

	int copy_qst_arg = used_switch(argc, argv, "-copy-qst");
	if (copy_qst_arg > 0)
	{
		if (copy_qst_arg + 3 > argc)
		{
			printf("%d\n", argc);
			printf("expected -copy-qst <input> <output>\n");
			zq_exit(1);
		}

		const char* input_filename = argv[copy_qst_arg + 1];
		const char* output_filename = argv[copy_qst_arg + 2];
		do_copy_qst_command(input_filename, output_filename);
	}

	Z_title("%s, %s",ZQ_EDITOR_NAME, getVersionString());

	if(!get_qst_buffers())
	{
		Z_error_fatal("Error");
	}
	
	undocombobuf.clear();
	undocombobuf.resize(MAXCOMBOS);
	
	if((newundotilebuf=(tiledata*)malloc(NEWMAXTILES*sizeof(tiledata)))==NULL)
	{
		Z_error_fatal("Error: no memory for tile undo buffer!");
	}
	
	memset(newundotilebuf, 0, NEWMAXTILES*sizeof(tiledata));
	
	Z_message("Resetting new tile buffer...");
	newtilebuf = (tiledata*)malloc(NEWMAXTILES*sizeof(tiledata));
	
	for(int32_t j=0; j<NEWMAXTILES; j++)
		newtilebuf[j].data=NULL;
		
	Z_message("OK\n");
	
	zc_srand(time(0));

	if (test_zc_arg > 0)
	{
		set_headless_mode();

		if (test_zc_arg + 1 > argc)
		{
			printf("%d\n", argc);
			printf("expected -test-zc <path to test dir>\n");
			zq_exit(1);
		}

		const char* test_dir = argv[test_zc_arg + 1];

		// We need to init some stuff before loading a quest file will work.
		int fake_errno = 0;
		allegro_errno = &fake_errno;
		get_qst_buffers();

		bool success = true;
		if (!partial_load_test(test_dir))
		{
			success = false;
			printf("partial_load_test failed\n");
		}
		if (success)
			printf("all tests passed\n");
		zq_exit(success ? 0 : 1);
	}

	int unencrypt_qst_arg = used_switch(argc, argv, "-unencrypt-qst");
	if (unencrypt_qst_arg > 0)
	{
		if (unencrypt_qst_arg + 3 > argc)
		{
			printf("%d\n", argc);
			printf("expected -unencrypt-qst <input> <output>\n");
			zq_exit(1);
		}

		const char* input_filename = argv[unencrypt_qst_arg + 1];
		const char* output_filename = argv[unencrypt_qst_arg + 2];
		do_unencrypt_qst_command(input_filename, output_filename);
		zq_exit(0);
	}

	int uncompress_qst_arg = used_switch(argc, argv, "-uncompress-qst");
	if (uncompress_qst_arg > 0)
	{
		if (uncompress_qst_arg + 3 > argc)
		{
			printf("%d\n", argc);
			printf("expected -uncompress-qst <input> <output>\n");
			zq_exit(1);
		}

		const char* input_filename = argv[uncompress_qst_arg + 1];
		const char* output_filename = argv[uncompress_qst_arg + 2];
		do_uncompress_qst_command(input_filename, output_filename);
		zq_exit(0);
	}

	three_finger_flag=false;

#ifndef __EMSCRIPTEN__
	if(zc_get_config("zquest","open_debug_console",0))
		initConsole();
#endif
	
	LOCK_VARIABLE(lastfps);
	
	LOCK_VARIABLE(framecnt);
	LOCK_FUNCTION(fps_callback);
	
	if(install_int_ex(fps_callback,SECS_TO_TIMER(1)))
	{
		Z_error_fatal("couldn't allocate timer");
	}
	
	
	LOCK_VARIABLE(dclick_status);
	LOCK_VARIABLE(dclick_time);
	lock_dclick_function();
	install_int(dclick_check, 20);
	
	set_gfx_mode(GFX_TEXT,80,50,0,0);

	load_assets();

	Z_message("OK\n");

	helpstr = util::read_text_file("docs/zquest.txt");
	zstringshelpstr = util::read_text_file("docs/zstrings.txt");
	
	// loading data files...
	
	filepath[0]=temppath[0]=0;
	
	const char *default_path="";
	
	strcpy(datapath,zc_get_config("zquest",data_path_name,default_path));
	strcpy(midipath,zc_get_config("zquest",midi_path_name,default_path));
	strcpy(imagepath,zc_get_config("zquest",image_path_name,default_path));
	strcpy(tmusicpath,zc_get_config("zquest",tmusic_path_name,default_path));
	chop_path(datapath);
	chop_path(midipath);
	chop_path(imagepath);
	chop_path(tmusicpath);
	
	DisableLPalShortcuts        = zc_get_config("zquest","dis_lpal_shortcut",1);
	DisableCompileConsole        = zc_get_config("zquest","internal_compile_console",0);
	MouseScroll					= zc_get_config("zquest","mouse_scroll",0);
	WarnOnInitChanged			  = zc_get_config("zquest","warn_initscript_changes",1);
	MMapCursorStyle				= zc_get_config("zquest","cursorblink_style",1);
	LayerDitherBG				= zc_get_config("zquest", "layer_dither_bg", -1);
	LayerDitherSz				= zc_get_config("zquest", "layer_dither_sz", 3);
	InvalidBG					= zc_get_config("zquest", "invalid_bg", 0);
	TileProtection				 = zc_get_config("zquest","tile_protection",1);
	ComboProtection				 = zc_get_config("zquest","combo_protection",TileProtection);
	ShowGrid					   = zc_get_config("zquest","show_grid",0);
	ShowCurScreenOutline			= zc_get_config("zquest","show_current_screen_outline",1);
	ShowScreenGrid				   = zc_get_config("zquest","show_screen_grid",0);
	GridColor					  = zc_get_config("zquest","grid_color",15);
	CmbCursorCol					  = zc_get_config("zquest","combo_cursor_color",15);
	TilePgCursorCol					  = zc_get_config("zquest","tpage_cursor_color",15);
	CmbPgCursorCol					  = zc_get_config("zquest","cpage_cursor_color",15);
	TTipHLCol					  = zc_get_config("zquest","ttip_hl_color",13);
	CheckerCol1					  = zc_get_config("zquest","checker_color_1",7);
	CheckerCol2					  = zc_get_config("zquest","checker_color_2",8);
	SnapshotFormat				 = zc_get_config("zquest","snapshot_format",3);
	SnapshotScale				 = zc_get_config("zquest","snapshot_scale",2);
	SavePaths					  = zc_get_config("zquest","save_paths",1);
	CycleOn						= zc_get_config("zquest","cycle_on",1);
	ShowFPS						= zc_get_config("zquest","showfps",0)!=0;
	SaveDragResize						= zc_get_config("zquest","save_drag_resize",0)!=0;
	DragAspect						= zc_get_config("zquest","drag_aspect",0)!=0;
	SaveWinPos						= zc_get_config("zquest","save_window_position",0)!=0;
	ComboBrush					 = zc_get_config("zquest","combo_brush",0);
	FloatBrush					 = zc_get_config("zquest","float_brush",0);
	AutoBrush = zc_get_config("zquest","autobrush",1);
	LinkedScroll = zc_get_config("zquest","linked_comboscroll",0);
	allowHideMouse = zc_get_config("ZQ_GUI","allowHideMouse",0);
	ShowFavoriteComboModes = zc_get_config("ZQ_GUI","show_fav_combo_modes",1);
	NoHighlightLayer0 = zc_get_config("zquest","no_highlight_layer0",0);
	RulesetDialog				  = zc_get_config("zquest","rulesetdialog",1);
	EnableTooltips				 = zc_get_config("zquest","enable_tooltips",1);
	TooltipsHighlight				 = zc_get_config("zquest","ttip_highlight",1);
	tooltip_maxtimer				 = vbound(zc_get_config("zquest","ttip_timer",30),0,60*60);
	ShowFFScripts				  = zc_get_config("zquest","showffscripts",1);
	ShowSquares					= zc_get_config("zquest","showsquares",1);
	ShowFFCs					= zc_get_config("zquest","showffcs",0);
	ShowInfo					   = zc_get_config("zquest","showinfo",1);
	skipLayerWarning			   = zc_get_config("zquest","skip_layer_warning",0);
	numericalFlags			  	 = zc_get_config("zquest","numerical_flags",0);
	ViewLayer2BG = zc_get_config("zquest","layer2_bg",0);
	ViewLayer3BG = zc_get_config("zquest","layer3_bg",0);
	ActiveLayerHighlight = zc_get_config("zquest","hl_active_lyr",0);
	DragCenterOfSquares = zc_get_config("zquest","drag_squares_from_center",0);
	
	OpenLastQuest				  = zc_get_config("zquest","open_last_quest",0);
	ShowMisalignments			  = zc_get_config("zquest","show_misalignments",0);
	AnimationOn					= zc_get_config("zquest","animation_on",1);
	AutoBackupRetention			= zc_get_config("zquest","auto_backup_retention",2);
	AutoSaveInterval			   = zc_get_config("zquest","auto_save_interval",6);
	AutoSaveRetention			  = zc_get_config("zquest","auto_save_retention",2);
	UncompressedAutoSaves		  = zc_get_config("zquest","uncompressed_auto_saves",1);
	OverwriteProtection			= zc_get_config("zquest","overwrite_prevention",0)!=0;
	ImportMapBias				  = zc_get_config("zquest","import_map_bias",0);
	
	KeyboardRepeatDelay		   = zc_get_config("zquest","keyboard_repeat_delay",300);
	KeyboardRepeatRate			= zc_get_config("zquest","keyboard_repeat_rate",80);
	
//  Frameskip					 = zc_get_config("zquest","frameskip",0); //todo: this is not actually supported yet.
	RequestedFPS				  = zc_get_config("zquest","fps",60);
	
	// Autofill for Combo Page, Tile Page
	PreFillTileEditorPage	  = zc_get_config("zquest","PreFillTileEditorPage",0);
	PreFillComboEditorPage	  = zc_get_config("zquest","PreFillComboEditorPage",0);
	
	pixeldb = zc_get_config("ZQ_GUI","bottom_8_pixels",0);
	infobg = zc_get_config("ZQ_GUI","info_text_bg",0);
	
	large_merged_combopane = zc_get_config("ZQ_GUI","merge_cpane_large",0);
	compact_merged_combopane = zc_get_config("ZQ_GUI","merge_cpane_compact",1);
	
	compact_square_panels = zc_get_config("ZQ_GUI","square_panels_compact",0);
	
	large_zoomed_fav = zc_get_config("ZQ_GUI","zoom_fav_large",0);
	compact_zoomed_fav = zc_get_config("ZQ_GUI","zoom_fav_compact",1);
	large_zoomed_cmd = zc_get_config("ZQ_GUI","zoom_cmd_large",1);
	compact_zoomed_cmd = zc_get_config("ZQ_GUI","zoom_cmd_compact",1);
	
	if(zc_get_config("gui","disable_window_resizing",0))
		all_set_resize_flag(false);
	
	load_hotkeys();
	
#ifdef _WIN32
	zqUseWin32Proc				 = zc_get_config("zquest","zq_win_proc_fix",0);
	
#endif

	if (!render_timer_start())
	{
		Z_error_fatal("couldn't allocate timer");
	}
	
	byte layermask = zc_get_config("zquest","layer_mask",0x7F);
	int32_t usefullscreen = zc_get_config("zquest","fullscreen",0);
	tempmode = (usefullscreen == 0 ? GFX_AUTODETECT_WINDOWED : GFX_AUTODETECT_FULLSCREEN);
	
	for(int32_t x=0; x<7; x++)
	{
		LayerMaskInt[x]=get_bit(&layermask,x);
	}
	
	DuplicateAction[0]			 = zc_get_config("zquest","normal_duplicate_action",2);
	DuplicateAction[1]			 = zc_get_config("zquest","horizontal_duplicate_action",0);
	DuplicateAction[2]			 = zc_get_config("zquest","vertical_duplicate_action",0);
	DuplicateAction[3]			 = zc_get_config("zquest","both_duplicate_action",0);
	LeechUpdate					= zc_get_config("zquest","leech_update",500);
	LeechUpdateTiles			   = zc_get_config("zquest","leech_update_tiles",1);
	OnlyCheckNewTilesForDuplicates = zc_get_config("zquest","only_check_new_tiles_for_duplicates",0);
	//gui_colorset				   = zc_get_config("zquest","gui_colorset",0);
	
	strcpy(last_timed_save,zc_get_config("zquest","last_timed_save",""));
	
	midi_volume					= zc_get_config("zquest", "midi", 255);
	
	abc_patternmatch			   = zc_get_config("zquest", "lister_pattern_matching", 1);
	NoScreenPreview			   = zc_get_config("zquest", "no_preview", 0);
	
	monochrome_console = zc_get_config("CONSOLE","monochrome_debuggers",0)?1:0;
	
	try_recovering_missing_scripts = 0;//zc_get_config("Compiler", "try_recovering_missing_scripts",0);
	//We need to remove all of the zeldadx refs to the config file for zquest. 
	
	set_keyboard_rate(KeyboardRepeatDelay,KeyboardRepeatRate);
	
	is_compact = zc_get_config("ZQ_GUI","compact_mode",1);
	mapscreenbmp = nullptr;
	brushbmp = nullptr;
	brushscreen = nullptr;
	screen2 = nullptr;
	tmp_scr = nullptr;
	menu1 = nullptr;
	menu3 = nullptr;
	
	for(int32_t i=0; i<MAXFAVORITECOMBOS; ++i)
	{
		favorite_combos[i]=-1;
		pool_combos[i].clear();
	}
	FavoriteComboPage = 0;
	pool_dirty = true;
	
	if(used_switch(argc,argv,"-d"))
	{
		set_debug(!strcmp(zquestpwd,zc_get_config("zquest","debug_this","")));
	}
	
	zcmusic_init();
	zcmixer = zcmixer_create();
	install_int_ex([](){ zcmusic_poll(); }, MSEC_TO_TIMER(25));

	set_color_depth(8);
	
	set_close_button_callback((void (*)()) hit_close_button);
	
	if(used_switch(argc,argv,"-fullscreen"))
	{
		tempmode = GFX_AUTODETECT_FULLSCREEN;
	}
	else if(used_switch(argc,argv,"-windowed"))
	{
		tempmode=GFX_AUTODETECT_WINDOWED;
	}

	zq_screen_w = LARGE_W;
	zq_screen_h = LARGE_H;
	window_width = zc_get_config("zquest","window_width",-1);
	window_height = zc_get_config("zquest","window_height",-1);
	auto [w, h] = zc_get_default_display_size(LARGE_W, LARGE_H, window_width, window_height);
	int32_t videofail = is_headless() ? 0 : (set_gfx_mode(tempmode,w,h,zq_screen_w,zq_screen_h));

	//extra block here is intentional
	if(videofail!=0)
	{
		quit_game();
		allegro_exit();
	}

	zalleg_create_window();
	Z_message("gfx mode set at -%d %dbpp %d x %d \n",
				tempmode, get_color_depth(), zq_screen_w, zq_screen_h);

	set_window_title("ZC Editor");

	load_size_poses();

	if (!is_headless())
	{
		// Just in case.
		while (!all_get_display()) {
			al_rest(1);
		}

		al_resize_display(all_get_display(), w, h);
	}


#ifndef __EMSCRIPTEN__
	if (!all_get_fullscreen_flag() && !is_headless()) {
		al_resize_display(all_get_display(), w, h);

		int window_w = al_get_display_width(all_get_display());
		int window_h = al_get_display_height(all_get_display());

		int new_x = zc_get_config("zquest","window_x",0);
		int new_y = zc_get_config("zquest","window_y",0);
		if(zc_get_config("zquest","save_window_position",0) && (new_x || new_y))
		{
			//load saved position
			//already stored in new_x/new_y
		}
		else
		{
			//Get default position
			ALLEGRO_MONITOR_INFO info;
			al_get_monitor_info(0, &info);
			
			int mw = (info.x2 - info.x1);
			int mh = (info.y2 - info.y1);
			new_x = mw / 2 - window_w / 2;
			new_y = mh / 2 - window_h / 2;
			//Don't spawn the window too far down (taskbar?)
			if(new_y + window_h > mh - 72)
				new_y = mh-72-window_h;
		}
#ifdef ALLEGRO_MACOSX
		if (zc_get_config("zquest","save_window_position",0))
			al_set_window_position(all_get_display(), new_x, new_y);
#else
		al_set_window_position(all_get_display(), new_x, new_y);
#endif
	}
#endif
	
	position_mouse(zq_screen_w/2,zq_screen_h/2);
	
	dmapbmp_small = create_bitmap_ex(8,65,33);
	dmapbmp_large = create_bitmap_ex(8,177,81);
	
	if(!screen2 || !tmp_scr || !menu1 || !menu3 || !dmapbmp_large || !dmapbmp_large || !brushbmp || !brushscreen)// || !brushshadowbmp )
	{
		Z_error_fatal("Failed to create system bitmaps!\n");
		return 1;
	}

	int quick_assign_arg = used_switch(argc, argv, "-quick-assign");
	if (quick_assign_arg > 0)
	{
		is_zq_replay_test = true;
		set_headless_mode();

		int load_ret = load_quest(argv[quick_assign_arg + 1], false);
		bool success = load_ret == qe_OK;
		if (!success)
		{
			printf("Failed to load quest: %d\n", load_ret);
			zq_exit(1);
		}

		success = do_compile_and_slots(1, false);
		if (!success)
		{
			printf("Failed to compile\n");
			zq_exit(1);
		}

		success = save_quest(argv[quick_assign_arg + 1], false) == 0;
		if (!success)
		{
			printf("Failed to save quest\n");
			zq_exit(1);
		}

		zq_exit(0);
	}

	int smart_assign_arg = used_switch(argc, argv, "-smart-assign");
	if (smart_assign_arg > 0)
	{
		is_zq_replay_test = true;
		set_headless_mode();

		int load_ret = load_quest(argv[smart_assign_arg + 1], false);
		bool success = load_ret == qe_OK;
		if (!success)
		{
			printf("Failed to load quest: %d\n", load_ret);
			zq_exit(1);
		}

		success = do_compile_and_slots(2, false);
		if (!success)
		{
			printf("Failed to compile\n");
			zq_exit(1);
		}

		success = save_quest(argv[smart_assign_arg + 1], false) == 0;
		if (!success)
		{
			printf("Failed to save quest\n");
			zq_exit(1);
		}

		zq_exit(0);
	}

	int export_strings_arg = used_switch(argc, argv, "-export-strings");
	if (export_strings_arg > 0)
	{
		if (export_strings_arg + 3 > argc)
		{
			printf("%d\n", argc);
			printf("expected -export-strings input.qst output.tsv\n");
			zq_exit(1);
		}

		is_zq_replay_test = true;
		set_headless_mode();

		int load_ret = load_quest(argv[export_strings_arg + 1], false);
		bool success = load_ret == qe_OK;
		if (!success)
		{
			printf("Failed to load quest: %d\n", load_ret);
			zq_exit(1);
		}

		success = save_strings_tsv(argv[export_strings_arg + 2]);
		if (!success)
		{
			printf("Failed to export strings\n");
			zq_exit(1);
		}

		zq_exit(0);
	}

	if (!is_headless())
	{
		zc_set_palette(asset_pal);
		get_palette(RAMpal);
		load_colorset(gui_colorset);
		zc_set_palette(RAMpal);
		clear_to_color(screen,vc(0));
	}

	zScript = string();
	strcpy(zScriptBytes, "0 Bytes in Buffer");
	for(int32_t i=0; i<MOUSE_BMP_MAX; i++)
	{
		for(int32_t j=0; j<4; j++)
		{
			mouse_bmp[i][j] = NULL;
			mouse_bmp_1x[i][j] = NULL;
		}
	}
	load_mice();
	gui_mouse_focus=0;
	MouseSprite::set(ZQM_NORMAL);
	render_zq(); // Ensure the rendering bitmaps are setup.

#ifdef __EMSCRIPTEN__
	em_mark_ready_status();
#endif
	
	load_icons();
	
	bool load_last_timed_save=false;
	
	load_recent_quests();
	refresh_recent_menu();
	//clearConsole();
	if((last_timed_save[0]!=0)&&(exists(last_timed_save)))
	{
		if(jwin_alert("ZQuest","It appears that ZQuest crashed last time.","Would you like to load the last timed save?",NULL,"&Yes","&No",'y','n',get_zc_font(font_lfont))==1)
		{
			int32_t ret = load_quest(last_timed_save);
			
			if(ret == qe_OK)
			{
				strcpy(filepath,last_timed_save);
				load_last_timed_save=true;
				saved=false;
			}
			else
			{
				jwin_alert("Error","Unable to reload the last timed save.",NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
			}
		}
	}
	
	if(!load_last_timed_save)
	{
		strcpy(filepath,zc_get_config("zquest",last_quest_name,""));
		
		if(argc>1 && argv[1][0]!='-')
		{
			int32_t ret = load_quest(argv[1]);
			
			if(ret == qe_OK)
			{
				first_save=true;
				strcpy(filepath,argv[1]);
				refresh(rALL);
			}
		}
		else if(OpenLastQuest&&filepath[0]&&exists(filepath)&&!used_switch(argc,argv,"-new"))
		{
			int32_t ret = load_quest(filepath);
			
			if(ret == qe_OK)
			{
				first_save=true;
				refresh(rALL);
			}
			else
			{
				filepath[0]=temppath[0]=0;
				first_save=false;
			}
		}
		else
		{
			if (onNew() == D_CLOSE)
				exit(0);

			//otherwise the blank quest gets the name of the last loaded quest... not good! -DD
			filepath[0]=temppath[0]=0;
			first_save=false;
		}
	}

	if(used_switch(argc,argv,"-q"))
	{
		Z_message("-q switch used, quitting program.\n");
		zq_exit(0);
	}
	
	for(int32_t x=0; x<MAXITEMS; x++)
	{
		lens_hint_item[x][0]=0;
		lens_hint_item[x][1]=0;
	}
	
	for(int32_t x=0; x<MAXWPNS; x++)
	{
		lens_hint_weapon[x][0]=0;
		lens_hint_weapon[x][1]=0;
	}
	
	load_selections();
	load_arrows();
	clear_to_color(menu1,vc(0));
	DIALOG_PLAYER *player2=init_dialog(dialogs,-1);
	
	get_palette(RAMpal);
	
	rgb_map = &zq_rgb_table;

#ifdef __EMSCRIPTEN__
	{
		int qs_map = EM_ASM_INT({
			return new URL(location.href).searchParams.get('map') ?? -1;
		});
		int qs_screen = EM_ASM_INT({
			return new URL(location.href).searchParams.get('screen') ?? -1;
		});
		if (qs_map != -1 && qs_screen != -1) {
			Map.setCurrMap(qs_map);
			Map.setCurrScr(qs_screen);
		}
	}
#endif

	//  setup_combo_animations();
	pause_refresh = false;
	refresh_pal();
	refresh(rALL);
    for(int q = 0; q < brush_width_menu.size(); ++q)
		brush_width_menu.at(q)->select(q==0);
    for(int q = 0; q < brush_height_menu.size(); ++q)
		brush_height_menu.at(q)->select(q==0);
	set_filltype(1);
	
	rebuild_trans_table();

	if (!is_headless())
	{
		set_display_switch_mode(SWITCH_BACKGROUND);
		set_display_switch_callback(SWITCH_OUT, switch_out);
		set_display_switch_callback(SWITCH_IN, switch_in);
	}
	
	if(!update_dialog(player2))
		exiting_program = true;
	//clear_keybuf();
	media_menu.disable_uid(MENUID_MEDIA_CHANGETRACK, true);
	disable_hotkey(ZQKEY_CHANGE_TRACK, true);
	
	fix_drawing_mode_menu();
	
	
#ifdef _WIN32
	
	if(zqUseWin32Proc != FALSE)
	{
		al_trace("Config file warning: \"zq_win_proc_fix\" enabled switch found. This can cause crashes on some computers.\n");
		win32data.zqSetDefaultThreadPriority(0);
		win32data.zqSetCustomCallbackProc(al_get_win_window_handle(all_get_display()));
	}
	
#endif
	
	time(&auto_save_time_start);
	
	FFCore.init();
	ZQincludePaths = FFCore.includePaths;
	
	Map.setCopyFFC(-1); //Do not have an initial ffc on the clipboard. 
	brush_menu.select_uid(MENUID_BRUSH_AUTOBRUSH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_WIDTH, AutoBrush);
	brush_menu.disable_uid(MENUID_BRUSH_HEIGHT, AutoBrush);
	brush_menu.select_uid(MENUID_BRUSH_COMBOBRUSH, ComboBrush);
	brush_menu.select_uid(MENUID_BRUSH_FLOATBRUSH, FloatBrush);
	
	init_ffpos();
	
	call_foo_dlg();

	application_has_loaded = true;

	while(!exiting_program)
	{
#ifdef _WIN32
		if(zqUseWin32Proc != FALSE)
			win32data.Update(Frameskip); //experimental win32 fixes
#endif
		check_autosave();
		++alignment_arrow_timer;
		
		if(alignment_arrow_timer>63)
		{
			alignment_arrow_timer=0;
		}
		++frame;
		
		file_menu.disable_uid(MENUID_FILE_SAVE, saved||disable_saving||OverwriteProtection);
		file_menu.disable_uid(MENUID_FILE_REVERT, saved||disable_saving||OverwriteProtection);
		file_menu.disable_uid(MENUID_FILE_SAVEAS, disable_saving);
		
		fixtools_menu.disable_uid(MENUID_FIXTOOL_OLDSTRING,
			!(get_qr(qr_OLD_STRING_EDITOR_MARGINS)
				||get_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT)));
		
		edit_menu.disable_uid(MENUID_EDIT_UNDO, !Map.CanUndo());
		edit_menu.disable_uid(MENUID_EDIT_REDO, !Map.CanRedo());
		
		bool canpaste = Map.CanPaste();
		edit_menu.disable_uid(MENUID_EDIT_PASTE, !canpaste);
		edit_menu.disable_uid(MENUID_EDIT_PASTEALL, !canpaste);
		edit_menu.disable_uid(MENUID_EDIT_ADVPASTE, !canpaste);
		edit_menu.disable_uid(MENUID_EDIT_SPECPASTE, !canpaste);
		rc_menu_screen.disable_uid(MENUID_RCSCREEN_PASTE, !canpaste);
		rc_menu_screen.disable_uid(MENUID_RCSCREEN_ADVPASTE, !canpaste);
		rc_menu_screen.disable_uid(MENUID_RCSCREEN_SPECPASTE, !canpaste);
		for(MenuItem& mit : paste_menu.inner())
			mit.disable(!canpaste);
		for(MenuItem& mit : paste_item_menu.inner())
			mit.disable(!canpaste);
		
		edit_menu.disable_uid(MENUID_EDIT_COPY, !(Map.CurrScr()->valid&mVALID));
		edit_menu.disable_uid(MENUID_EDIT_DELETE, !(Map.CurrScr()->valid&mVALID));
		
		// Are some things selected?
		view_menu.select_uid(MENUID_VIEW_WALKABILITY, Flags&cWALK);
		view_menu.select_uid(MENUID_VIEW_FLAGS, Flags&cFLAGS);
		view_menu.select_uid(MENUID_VIEW_CSET, Flags&cCSET);
		view_menu.select_uid(MENUID_VIEW_TYPES, Flags&cCTYPE);
		view_menu.select_uid(MENUID_VIEW_INFO, ShowInfo);
		view_menu.select_uid(MENUID_VIEW_SQUARES, ShowSquares);
		view_menu.select_uid(MENUID_VIEW_FFCS, ShowFFCs);
		view_menu.select_uid(MENUID_VIEW_SCRIPTNAMES, ShowFFScripts);
		view_menu.select_uid(MENUID_VIEW_GRID, ShowGrid);
		view_menu.select_uid(MENUID_VIEW_SCREENGRID, ShowScreenGrid);
		view_menu.select_uid(MENUID_VIEW_CURSCROUTLINE, ShowCurScreenOutline);
		view_menu.select_uid(MENUID_VIEW_DARKNESS, get_qr(qr_NEW_DARKROOM) && (Flags&cNEWDARK));
		view_menu.select_uid(MENUID_VIEW_L2BG, ViewLayer2BG);
		view_menu.select_uid(MENUID_VIEW_L3BG, ViewLayer3BG);
		view_menu.select_uid(MENUID_VIEW_LAYERHIGHLIGHT, ActiveLayerHighlight);
		
		maps_menu.disable_uid(MENUID_MAPS_NEXT, !map_count || Map.getCurrMap() >= map_count);
		maps_menu.disable_uid(MENUID_MAPS_PREV, Map.getCurrMap()<=0);
		
		etc_menu.disable_uid(MENUID_ETC_VIDMODE, isFullScreen()==1);
		etc_menu.select_uid(MENUID_ETC_FULLSCREEN, isFullScreen()==1);
		
		if(!update_dialog(player2))
			exiting_program = true;
		
		//clear_keybuf();
		handle_close_btn_quit();
	}

	zq_exit(0);
	return 0;
}
END_OF_MAIN()

void zq_exit(int code)
{
	set_is_exiting();
	parser_console.kill();
	killConsole();
	
	quit_game();
	allegro_exit();
	exit(code);
}

void init_bitmap(BITMAP** bmp, int32_t w, int32_t h)
{
	if(*bmp)
		destroy_bitmap(*bmp);
	*bmp = create_bitmap_ex(8,w,h);
	clear_bitmap(*bmp);
}
void load_size_poses()
{
	ttip_uninstall_all();
	
	FONT* favcmdfont = get_custom_font(CFONT_FAVCMD);
	FONT* guifont = get_custom_font(CFONT_GUI);
	
	d_nbmenu_proc(MSG_START, &dialogs[0], 0);
	
	commands_list.xscale = command_buttonwidth;
	commands_list.yscale = 10+text_height(favcmdfont);
	
	auto drawmode_wid = 64;
	for(auto q = 0; q < dm_max; ++q)
	{
		auto wid = text_length(guifont, dm_names[q]);
		if(wid > drawmode_wid)
			drawmode_wid = wid;
	}

	//Main GUI objects
	if(is_compact)
	{
		num_combo_cols = 2;
		combo_col_scale = 16;
		if(compact_merged_combopane)
		{
			num_combo_cols = 1;
			combo_col_scale = 32;
		}
		
		mapscreen_x=0;
		mapscreen_y=dialogs[0].h;
		mapscreen_screenunit_scale=3;
		mapscreen_single_scale = (double)mapscreen_screenunit_scale / Map.getViewSize();
		showedges=0;
		showallpanels=0;
		
		blackout_color=8;
		
		auto mapscr_wid = (((showedges?2:0)+16)*16*mapscreen_screenunit_scale);
		combolist_window.w=zq_screen_w-mapscr_wid;
		combolist_window.x=zq_screen_w-combolist_window.w;
		
		favorites_window.x=combolist_window.x;
		favorites_window.w=combolist_window.w;
		favorites_window.h=136;
		favorites_window.y=zq_screen_h-favorites_window.h;
		
		combolist_window.y=0;
		combolist_window.h=favorites_window.y-combolist_window.y;
		
		combo_preview.x=zq_screen_w-32-8;
		combo_preview.y=combolist_window.y+6;
		combo_preview.w=32;
		combo_preview.h=32;
		combo_preview2.clear();
		
		auto col_wid = 4*combo_col_scale;
		auto cols_wid = col_wid * num_combo_cols;
		auto cols_spacing = (combolist_window.w-cols_wid)/(num_combo_cols+1);
		for(auto q = 0; q < num_combo_cols; ++q)
		{
			combolist[q].x=combolist_window.x+(cols_spacing*(q+1))+(col_wid*q);
			combolist[q].y=combolist_window.y+54;
			combolist[q].w=4;
			combolist[q].h=compact_merged_combopane ? 15 : 30;
			combolist[q].xscale = combo_col_scale;
			combolist[q].yscale = combo_col_scale;
			
			comboaliaslist[q].x = combolist[q].x;
			comboaliaslist[q].y = combolist[q].y;
			comboaliaslist[q].w = 4;
			comboaliaslist[q].h = compact_merged_combopane ? 13 : 26;
			comboaliaslist[q].xscale = combo_col_scale;
			comboaliaslist[q].yscale = combo_col_scale;
			
			combolistscrollers[q].w=2;
			combolistscrollers[q].h=1;
			combolistscrollers[q].xscale=11;
			combolistscrollers[q].yscale=11;
			combolistscrollers[q].x=combolist[q].x+(combolist[q].w*combolist[q].xscale/2)-11;
			combolistscrollers[q].y=combolist[q].y-combolistscrollers[q].th()-3;
		}
		
		comboalias_preview.x=zq_screen_w-((combolist_window.w+64)/2);
		comboalias_preview.h=64;
		comboalias_preview.y=favorites_window.y-comboalias_preview.h-8;
		comboalias_preview.w=64;
		
		combo_merge_btn.w = 20;
		combo_merge_btn.h = 20;
		combo_merge_btn.x = zq_screen_w-(combolist_window.w+combo_merge_btn.w)/2;
		combo_merge_btn.y = combolist[0].y-combo_merge_btn.h;
		
		if(num_combo_cols == 1)
		{
			combolistscrollers[0].x += 34;
		}
		drawmode_btn.x = combolist_window.x-drawmode_wid;
		drawmode_btn.y = 0;
		drawmode_btn.w = drawmode_wid;
		drawmode_btn.h = mapscreen_y;
		
		compactbtn.w = text_length(guifont,"> Compact")+10;
		compactbtn.x = drawmode_btn.x-compactbtn.w;
		compactbtn.y = drawmode_btn.y;
		compactbtn.h = drawmode_btn.h;

		zoominbtn.w = text_length(guifont,"+")+10;
		zoominbtn.x = compactbtn.x-zoominbtn.w;
		zoominbtn.y = compactbtn.y;
		zoominbtn.h = compactbtn.h;

		zoomoutbtn.w = text_length(guifont,"-")+10;
		zoomoutbtn.x = zoominbtn.x-zoomoutbtn.w;
		zoomoutbtn.y = compactbtn.y;
		zoomoutbtn.h = compactbtn.h;

		for(int32_t i=0; i<=8; i++)
		{
			map_page_bar[i].w = 48;
			map_page_bar[i].x = mapscreen_x+(i*48);
			map_page_bar[i].y = mapscreen_y+(11*16*mapscreen_screenunit_scale);
			map_page_bar[i].h = text_height(guifont)+12;
		}
		
		minimap.w=7+48*3;
		minimap.h=16+27*3;
		
		layer_panel.x=map_page_bar[6].x;
		layer_panel.y=map_page_bar[0].y;
		layer_panel.w=combolist_window.x - layer_panel.x;
		layer_panel.h=map_page_bar[0].h;
		layerpanel_buttonwidth = 51;
		layerpanel_buttonheight = layer_panel.h;
		layerpanel_checkbox_hei = layerpanel_buttonheight-4;
		layerpanel_checkbox_wid = 15;
		
		minimap.x=3;
		minimap.y=layer_panel.y+layer_panel.h+4;

		real_minimap.x = minimap.x+3;
		real_minimap.y = minimap.y+5;
		real_minimap.w = 16;
		real_minimap.h = 9;
		real_minimap.xscale = 9;
		real_minimap.yscale = 9;
		real_minimap.fw = real_minimap.xscale*8;
		real_minimap.fh = real_minimap.yscale*8;
		
		int upscale_mm = 3;
		int xwid = real_minimap.tw()*(upscale_mm-1);
		int xhei = real_minimap.th()*(upscale_mm-1);
		minimap_zoomed.set(minimap.x, minimap.y-xhei, minimap.w+xwid, minimap.h+xhei+4);
		real_minimap_zoomed.set(minimap_zoomed.x+3, minimap_zoomed.y+5, real_minimap.w, real_minimap.h, real_minimap.xscale*upscale_mm, real_minimap.yscale*upscale_mm);
		real_minimap_zoomed.fw = real_minimap_zoomed.xscale*8;
		real_minimap_zoomed.fh = real_minimap_zoomed.yscale*8;
		
		screrrorpos.x = combolist_window.x - 3;
		screrrorpos.y = layer_panel.y - 16;
		
		mouse_scroll_h=10;
		
		favorites_list.x=favorites_window.x+8;
		favorites_list.y=favorites_window.y+16;
		favorites_list.xscale = 16;
		favorites_list.yscale = 16;
		favorites_list.w=(favorites_window.w-16)/favorites_list.xscale;
		favorites_list.h=(favorites_window.h-24)/favorites_list.yscale;
		
		commands_list.w=4;
		
		int bh = commands_list.yscale;
		int bw = 26;
		commands_window.w=commands_list.w*commands_list.xscale+10+bw;
		commands_window.x=combolist_window.x-commands_window.w;
		commands_window.y=layer_panel.y+layer_panel.h;
		commands_window.h=zq_screen_h-commands_window.y;
		int bx = commands_window.x+2;
		
		commands_list.y=commands_window.y+4;
		commands_list.h=(zq_screen_h - commands_list.y) / commands_list.yscale;
		commands_list.x=bx+bw;
		
		commands_zoombtn.w = bw;
		commands_zoombtn.h = bh;
		commands_zoombtn.x = bx;
		commands_zoombtn.y = commands_list.y;
		
		commands_infobtn.w = bw;
		commands_infobtn.h = bh;
		commands_infobtn.x = bx;
		commands_infobtn.y = commands_zoombtn.y + commands_infobtn.h;
		
		commands_x.w = bw;
		commands_x.h = bh;
		commands_x.x = bx;
		commands_x.y = commands_infobtn.y + commands_x.h;
		
		commands_txt.clear();
		
		main_panel.x = 0;
		main_panel.y = layer_panel.y+layer_panel.h;
		main_panel.w = commands_window.x - main_panel.x;
		main_panel.h = 76+32;
		preview_panel = main_panel;
		preview_panel.x = 0;
		preview_panel.w = commands_window.x - preview_panel.x;
		
		preview_text.x = preview_panel.x+3;
		preview_text.y = preview_panel.y+3;
		preview_text.w = 2;
		preview_text.h = 6;
		preview_text.xscale = 10;
		preview_text.yscale = text_height(get_zc_font(font_lfont_l));
		
		panel_align = 1;
		int swapbtnw = 32, swapbtnh = 20;
		int swapbtnx = main_panel.x+main_panel.tw()-swapbtnw;
		squarepanel_swap_btn.set(swapbtnx, zq_screen_h-swapbtnh, swapbtnw, swapbtnh);
		
		int sqx = minimap.x+minimap.tw();
		squares_panel.set(sqx,main_panel.y,main_panel.tw()-sqx,main_panel.th());
		if(compact_square_panels)
		{
			int cmpy = main_panel.y+(main_panel.th()/2);
			squarepanel_up_btn.set(swapbtnx, cmpy-swapbtnh, swapbtnw, swapbtnh);
			squarepanel_down_btn.set(swapbtnx, cmpy, swapbtnw, swapbtnh);
			
			txtoffs_single.x = 18;
			txtoffs_single.y = 36;
			txtoffs_double_1.x = 18;
			txtoffs_double_1.y = 36;
			txtoffs_double_2.x = 18;
			txtoffs_double_2.y = 36 + text_height(get_custom_font(CFONT_GUI));
			
			//Clear them all- if they stay cleared, they are invisible.
			itemsqr_pos.clear();
			stairsqr_pos.clear();
			warparrival_pos.clear();
			flagsqr_pos.clear();
			enemy_prev_pos.clear();
			for(int q = 0; q < 4; ++q)
				warpret_pos[q].clear();
			
			int sqr_x1 = sqx+12;
			int sqr_y1 = main_panel.y+12;
			int sqr_xoffs = (16*2)+4 + 12;
			switch(compact_active_panel)
			{
				case 0: //Warp Squares
				{
					int x = sqr_x1;
					for(int q = 0; q < 4; ++q)
					{
						warpret_pos[q].set(x,sqr_y1,(16*2)+4,(16*2)+4);
						x += sqr_xoffs; 
					}
					break;
				}
				case 1: //Other Squares
				{
					itemsqr_pos.set(sqr_x1+(sqr_xoffs*0), sqr_y1, (16*2)+4,(16*2)+4);
					stairsqr_pos.set(sqr_x1+(sqr_xoffs*1), sqr_y1, (16*2)+4,(16*2)+4);
					warparrival_pos.set(sqr_x1+(sqr_xoffs*2), sqr_y1, (16*2)+4,(16*2)+4);
					flagsqr_pos.set(sqr_x1+(sqr_xoffs*3), sqr_y1, (16*2)+4,(16*2)+4);
					break;
				}
				case 2: //Enemy Preview
				{
					enemy_prev_pos.set(sqr_x1, sqr_y1, 5, 2, 32, 32);
					break;
				}
			}
		}
		else
		{
			squarepanel_up_btn.clear();
			squarepanel_down_btn.clear();
			txtoffs_single.x = 10;
			txtoffs_single.y = 22;
			txtoffs_double_1.x = 10;
			txtoffs_double_1.y = 22;
			txtoffs_double_2.x = 10;
			txtoffs_double_2.y = 30;
			
			
			int sqr_x1 = sqx+24;
			int sqr_y1 = main_panel.y+12;
			int sqr_y2 = sqr_y1+42;
			int sqr_xdist = 32;
			itemsqr_pos.set(sqr_x1+(sqr_xdist*0),sqr_y1,20,20);
			stairsqr_pos.set(sqr_x1+(sqr_xdist*1),sqr_y1,20,20);
			warparrival_pos.set(sqr_x1+(sqr_xdist*2),sqr_y1,20,20);
			flagsqr_pos.set(sqr_x1+(sqr_xdist*3),sqr_y1,20,20);
			for(auto q = 0; q < 4; ++q)
			{
				warpret_pos[q].set(sqr_x1+(sqr_xdist*q),sqr_y2,20,20);
			}
			enemy_prev_pos.set(sqr_x1+(sqr_xdist*4), sqr_y1, 4, 3, 16, 16);
			enemy_prev_pos.fw = enemy_prev_pos.xscale*2;
			enemy_prev_pos.fh = enemy_prev_pos.yscale*2;
		}
		
		auto& last_alias_list = comboaliaslist[num_combo_cols-1];
		combopool_preview.x=comboaliaslist[0].x;
		combopool_preview.y=last_alias_list.y+(last_alias_list.h*last_alias_list.yscale)+16;
		combopool_preview.w=(last_alias_list.x+(last_alias_list.w*last_alias_list.xscale))-comboaliaslist[0].x;
		combopool_preview.h=zq_screen_h-8-combopool_preview.y;
		combopool_preview.w -= combopool_preview.w%16;
		combopool_preview.h -= combopool_preview.h%16;
		
		FONT* tfont = get_zc_font(font_lfont_l);
		combopool_prevbtn.w = text_length(tfont, "Unweighted")+10;
		combopool_prevbtn.h = 11;
		combopool_prevbtn.x = combopool_preview.x;
		combopool_prevbtn.y = combopool_preview.y-combopool_prevbtn.h;
		
		mappage_count = 6;
		
		txfont = get_zc_font(font_lfont_l);
		combo_preview_text1.set(combo_preview.x-5,combo_preview.y,1,3,1,text_height(txfont));
		combo_preview_text2.clear();
		
		favorites_x.w = 17;
		favorites_infobtn.w = 17;
		favorites_zoombtn.w = 17;
		favorites_pgleft.w = 17;
		favorites_pgright.w = 17;
	}
	else
	{
		num_combo_cols = 4;
		combo_col_scale = 16;
		if(large_merged_combopane)
		{
			num_combo_cols = 2;
			combo_col_scale = 32;
		}
		
		mapscreen_x=0;
		mapscreen_y=dialogs[0].h;
		mapscreen_screenunit_scale=2;
		mapscreen_single_scale = (double)mapscreen_screenunit_scale / Map.getViewSize();
		showedges=Map.getViewSize() <= 2 ? 1 : 0;
		showallpanels=0;
		
		blackout_color=8;
		
		favorites_window.h=136;
		favorites_window.y=zq_screen_h-favorites_window.h;
		
		auto mapscr_wid = (((2)+16)*16*mapscreen_screenunit_scale);
		combolist_window.w=zq_screen_w-mapscr_wid;
		combolist_window.x=zq_screen_w-combolist_window.w;
		combolist_window.y=0;
		combolist_window.h=favorites_window.y-combolist_window.y;
		
		favorites_window.x=combolist_window.x;
		favorites_window.w=combolist_window.w;
		
		combo_preview.x=(zq_screen_w-(combolist_window.w/2))-40;
		combo_preview.y=combolist_window.y+6;
		combo_preview.w=32;
		combo_preview.h=32;
		combo_preview2 = combo_preview;
		combo_preview2.x += 48;
		
		auto col_wid = 4*combo_col_scale;
		auto cols_wid = col_wid * num_combo_cols;
		auto cols_spacing = (combolist_window.w-cols_wid)/(num_combo_cols+1);
		for(auto q = 0; q < num_combo_cols; ++q)
		{
			combolist[q].x=combolist_window.x+(cols_spacing*(q+1))+(col_wid*q);
			combolist[q].y=combolist_window.y+60;
			combolist[q].w=4;
			combolist[q].h=large_merged_combopane ? 15 : 30;
			combolist[q].xscale = combo_col_scale;
			combolist[q].yscale = combo_col_scale;
			
			comboaliaslist[q].x=combolist[q].x;
			comboaliaslist[q].y=combolist[q].y;
			comboaliaslist[q].w=4;
			comboaliaslist[q].h=large_merged_combopane ? 12 : 25;
			comboaliaslist[q].xscale = combo_col_scale;
			comboaliaslist[q].yscale = combo_col_scale;
			
			combolistscrollers[q].w=2;
			combolistscrollers[q].h=1;
			combolistscrollers[q].xscale=11;
			combolistscrollers[q].yscale=11;
			combolistscrollers[q].x=combolist[q].x+(combolist[q].w*combolist[q].xscale/2)-11;
			combolistscrollers[q].y=combolist[q].y-combolistscrollers[q].th()-2;
		}
		
		comboalias_preview.x=zq_screen_w-((combolist_window.w+64)/2);
		comboalias_preview.h=64;
		comboalias_preview.w=64;
		comboalias_preview.y=favorites_window.y-comboalias_preview.h-8;
		
		combo_merge_btn.w = 20;
		combo_merge_btn.h = 20;
		combo_merge_btn.x = zq_screen_w-(combolist_window.w+combo_merge_btn.w)/2;
		combo_merge_btn.y = combolist[0].y-combo_merge_btn.h;
		squarepanel_swap_btn.clear();
		squarepanel_up_btn.clear();
		squarepanel_down_btn.clear();
		
		drawmode_btn.x = combolist_window.x-drawmode_wid;
		drawmode_btn.y = 0;
		drawmode_btn.w = drawmode_wid;
		drawmode_btn.h = mapscreen_y;
		
		compactbtn.w = text_length(guifont,"> Compact")+10;
		compactbtn.x = drawmode_btn.x-compactbtn.w;
		compactbtn.y = drawmode_btn.y;
		compactbtn.h = drawmode_btn.h;

		zoominbtn.w = text_length(guifont,"+")+10;
		zoominbtn.x = compactbtn.x-zoominbtn.w;
		zoominbtn.y = compactbtn.y;
		zoominbtn.h = compactbtn.h;

		zoomoutbtn.w = text_length(guifont,"-")+10;
		zoomoutbtn.x = zoominbtn.x-zoomoutbtn.w;
		zoomoutbtn.y = compactbtn.y;
		zoomoutbtn.h = compactbtn.h;

		for(int32_t i=0; i<=8; i++)
		{
			map_page_bar[i].x = mapscreen_x+(i*16*2*mapscreen_screenunit_scale);
			map_page_bar[i].y = mapscreen_y+((13)*16*mapscreen_screenunit_scale);
			map_page_bar[i].w = 64;
			map_page_bar[i].h = text_height(guifont)+12;
		}
		
		minimap.w=7+48*3;
		minimap.h=16+27*3;
		
		layer_panel.x=map_page_bar[0].x;
		layer_panel.y=map_page_bar[0].y+map_page_bar[0].h;
		layer_panel.w=map_page_bar[8].x+map_page_bar[8].w;
		layer_panel.h=text_height(guifont)+8;
		layerpanel_buttonwidth = 58;
		layerpanel_buttonheight = layer_panel.h;
		layerpanel_checkbox_hei = layerpanel_buttonheight-4;
		layerpanel_checkbox_wid = 14;
		
		commands_list.w=4;
		commands_window.w=commands_list.w*commands_list.xscale+16;
		commands_window.x=combolist_window.x-commands_window.w;
		commands_window.y=layer_panel.y+layer_panel.h;
		commands_window.h=zq_screen_h-commands_window.y;
		
		//buttons panel
		main_panel.x = 0;
		main_panel.y = layer_panel.y+layer_panel.h;
		main_panel.w = commands_window.x - main_panel.x;
		main_panel.h = zq_screen_h - main_panel.y;
		preview_panel = main_panel;
		
		preview_text.x = preview_panel.x+3;
		preview_text.y = preview_panel.y+3;
		preview_text.w = 1;
		preview_text.h = 12;
		preview_text.xscale = 10;
		preview_text.yscale = text_height(get_zc_font(font_lfont_l));
		
		minimap.x=3;
		minimap.y=main_panel.y+4;
		
		real_minimap.x = minimap.x+3;
		real_minimap.y = minimap.y+5;
		real_minimap.w = 16;
		real_minimap.h = 9;
		real_minimap.xscale = 9;
		real_minimap.yscale = 9;
		real_minimap.fw = real_minimap.xscale*8;
		real_minimap.fh = real_minimap.yscale*8;
		
		int upscale_mm = 4;
		int xwid = real_minimap.tw()*(upscale_mm-1);
		int xhei = real_minimap.th()*(upscale_mm-1);
		int zh = minimap.h+xhei+4;
		minimap_zoomed.set(minimap.x, zq_screen_h-zh, minimap.w+xwid, zh);
		real_minimap_zoomed.set(minimap_zoomed.x+3, minimap_zoomed.y+5, real_minimap.w, real_minimap.h, real_minimap.xscale*upscale_mm, real_minimap.yscale*upscale_mm);
		real_minimap_zoomed.fw = real_minimap_zoomed.xscale*8;
		real_minimap_zoomed.fh = real_minimap_zoomed.yscale*8;
		
		screrrorpos.x = 575;
		screrrorpos.y = 388;
		
		mouse_scroll_h=10;
		
		favorites_list.x=favorites_window.x+8;
		favorites_list.y=favorites_window.y+16;
		favorites_list.xscale = 16;
		favorites_list.yscale = 16;
		favorites_list.w=(favorites_window.w-16)/favorites_list.xscale;
		favorites_list.h=(favorites_window.h-24)/favorites_list.yscale;
		
		int bh = 16;
		int by = commands_window.y+4;
		commands_list.y=by+bh;
		commands_list.h=(zq_screen_h - commands_list.y) / commands_list.yscale;
		commands_list.x=commands_window.x+8;
		
		commands_x.w = 20;
		commands_x.h = bh;
		commands_x.x = commands_list.x + commands_list.tw() - commands_x.w;
		commands_x.y = by;
		
		commands_infobtn.w = 20;
		commands_infobtn.h = bh;
		commands_infobtn.x = commands_x.x - commands_infobtn.w;
		commands_infobtn.y = by;
		
		commands_zoombtn.w = 20;
		commands_zoombtn.h = bh;
		commands_zoombtn.x = commands_infobtn.x - commands_zoombtn.w;
		commands_zoombtn.y = by;
		
		commands_txt.x = commands_list.x;
		commands_txt.y = by+(bh-text_height(get_zc_font(font_lfont_l)))/2;
		
		favorites_x.x = favorites_window.x + favorites_window.w - favorites_x.w - 2;
		favorites_x.y = favorites_list.y-15;
		
		favorites_infobtn.x = favorites_x.x - favorites_infobtn.w;
		favorites_infobtn.y = favorites_x.y;
		
		favorites_zoombtn.x = favorites_infobtn.x - favorites_zoombtn.w;
		favorites_zoombtn.y = favorites_infobtn.y;

		favorites_pgright.x = favorites_zoombtn.x - favorites_pgright.w;
		favorites_pgright.y = favorites_zoombtn.y;

		favorites_pgleft.x = favorites_pgright.x - favorites_pgleft.w;
		favorites_pgleft.y = favorites_pgleft.y;
		
		txtoffs_single.x = 22;
		txtoffs_single.y = 6;
		txtoffs_double_1.x = 22;
		txtoffs_double_1.y = 2;
		txtoffs_double_2.x = 22;
		txtoffs_double_2.y = 10;
		panel_align = 0;
		
		int sqx = minimap.x+minimap.tw();
		squares_panel.set(sqx,main_panel.y,main_panel.tw()-sqx,main_panel.th());
		int x2 = sqx+4;
		int x1 = x2 - (20+(8*3)+2);
		int y1 = main_panel.y+10;
		int sw = 20, sh = 20;
		int offs = sh+4;
		
		itemsqr_pos.set(x2,y1+(0*offs),sw,sh);
		flagsqr_pos.set(x2,y1+(1*offs),sw,sh);
		stairsqr_pos.set(x2,y1+(2*offs),sw,sh);
		warparrival_pos.set(x2,y1+(6*offs),sw,sh);
		
		enemy_prev_pos.x = main_panel.x+14;
		enemy_prev_pos.y = main_panel.y+12 + minimap.h;
		enemy_prev_pos.w = 4;
		enemy_prev_pos.h = 3;
		enemy_prev_pos.xscale = 16;
		enemy_prev_pos.yscale = 16;
		enemy_prev_pos.fw = enemy_prev_pos.xscale*2;
		enemy_prev_pos.fh = enemy_prev_pos.yscale*2;
		
		warpret_pos[0].set(x1,y1+(4*offs),sw,sh);
		warpret_pos[1].set(x1,y1+(5*offs),sw,sh);
		warpret_pos[2].set(x2,y1+(4*offs),sw,sh);
		warpret_pos[3].set(x2,y1+(5*offs),sw,sh);
		
		auto& last_alias_list = comboaliaslist[num_combo_cols-1];
		combopool_preview.x=comboaliaslist[0].x;
		combopool_preview.y=last_alias_list.y+(last_alias_list.h*last_alias_list.yscale)+16;
		combopool_preview.w=(last_alias_list.x+(last_alias_list.w*last_alias_list.xscale))-comboaliaslist[0].x;
		combopool_preview.h=(favorites_window.y-combopool_preview.y);//+favorites_window.h-10;
		combopool_preview.w -= combopool_preview.w%16;
		combopool_preview.h -= combopool_preview.h%16;
		
		FONT* tfont = get_zc_font(font_lfont_l);
		combopool_prevbtn.w = text_length(tfont, "Unweighted")+10;
		combopool_prevbtn.h = 11;
		combopool_prevbtn.x = combopool_preview.x;
		combopool_prevbtn.y = combopool_preview.y-combopool_prevbtn.h;
		
		mappage_count = 9;
		
		txfont = get_zc_font(font_lfont_l);
		combo_preview_text1.set(combo_preview.x-9,combo_preview.y,1,3,1,text_height(txfont));
		combo_preview_text2.set(combo_preview2.x+combo_preview2.w+8,combo_preview2.y,1,3,1,text_height(txfont));
		
		favorites_x.w = 30;
		favorites_infobtn.w = 30;
		favorites_zoombtn.w = 30;
		favorites_pgleft.w = 30;
		favorites_pgright.w = 30;
	}
	//Same in all modes
	{
		if(is_compact ? compact_zoomed_cmd : large_zoomed_cmd)
		{
			commands_list.w /= 2;
			commands_list.xscale *= 2;
		}
		if(is_compact ? compact_zoomed_fav : large_zoomed_fav)
		{
			if(favorites_list.w%2)
				favorites_list.x += (favorites_list.xscale / 2);
			favorites_list.xscale *= 2;
			favorites_list.yscale *= 2;
			favorites_list.w /= 2;
			favorites_list.h /= 2;
		}
		
		favorites_x.h = 14;
		favorites_x.x = favorites_window.x + favorites_window.w - favorites_x.w - 2;
		favorites_x.y = favorites_list.y-15;
		
		favorites_infobtn.h = favorites_x.h;
		favorites_infobtn.x = favorites_x.x - favorites_infobtn.w;
		favorites_infobtn.y = favorites_x.y;
		
		favorites_zoombtn.h = favorites_infobtn.h;
		favorites_zoombtn.x = favorites_infobtn.x - favorites_zoombtn.w;
		favorites_zoombtn.y = favorites_infobtn.y;

		favorites_pgright.h = favorites_zoombtn.h;
		favorites_pgright.x = favorites_zoombtn.x - favorites_pgright.w;
		favorites_pgright.y = favorites_zoombtn.y;

		favorites_pgleft.h = favorites_pgright.h;
		favorites_pgleft.x = favorites_pgright.x - favorites_pgleft.w;
		favorites_pgleft.y = favorites_pgright.y;
		
		mainbar.x = dialogs[0].x+dialogs[0].w;
		mainbar.y = 0;
		mainbar.w = zoomoutbtn.x-mainbar.x;
		mainbar.h = drawmode_btn.h;
	}
	
	//Ensure current combo list selected is valid
	current_combolist=vbound(current_combolist,0,num_combo_cols-1);
	current_comboalist=vbound(current_comboalist,0,num_combo_cols-1);
	current_cpoollist=vbound(current_cpoollist,0,num_combo_cols-1);
	current_cautolist = vbound(current_cautolist, 0, num_combo_cols - 1);
	
	//Generate bitmaps
	init_bitmap(&mapscreenbmp,16*(showedges?18:16),16*(showedges?13:11));
	init_bitmap(&brushbmp,256*mapscreen_screenunit_scale,176*mapscreen_screenunit_scale);
	init_bitmap(&brushscreen,(256+(showedges?16:0))*mapscreen_screenunit_scale,(176+(showedges?16:0))*mapscreen_screenunit_scale);
	
	init_bitmap(&screen2,zq_screen_w,zq_screen_h);
	init_bitmap(&tmp_scr,zq_screen_w,zq_screen_h);
	init_bitmap(&menu1,zq_screen_w,zq_screen_h);
	init_bitmap(&menu3,zq_screen_w,zq_screen_h);
	
	center_zq_class_dialogs();
	center_zq_custom_dialogs();
	center_zq_files_dialogs();
	center_zq_subscreen_dialogs();
	center_zq_tiles_dialogs();
	center_zquest_dialogs();
	
	aspect_ratio = zq_screen_h / double(zq_screen_w);

	mmap_init();
}

void remove_locked_params_on_exit()
{
    al_trace("Removing timers. \n");
    remove_int(fps_callback);
    remove_int(dclick_check);
}

void destroy_bitmaps_on_exit()
{
    al_trace("Cleaning bitmaps...");
    destroy_bitmap(screen2);
    destroy_bitmap(tmp_scr);
    destroy_bitmap(menu1);
    destroy_bitmap(menu3);
    destroy_bitmap(mapscreenbmp);
    destroy_bitmap(dmapbmp_small);
    destroy_bitmap(dmapbmp_large);
    destroy_bitmap(brushbmp);
    destroy_bitmap(brushscreen);
    al_trace("...");
    
    for(int32_t i=0; i<MOUSE_BMP_MAX*4; i++)
	{
        destroy_bitmap(mouse_bmp[i/4][i%4]);
        destroy_bitmap(mouse_bmp_1x[i/4][i%4]);
	}
        
    for(int32_t i=0; i<ICON_BMP_MAX*4; i++)
        destroy_bitmap(icon_bmp[i/4][i%4]);
        
    for(int32_t i=0; i<16*4; i++)
		destroy_bitmap(flag_bmp[i/4][i%4]);
    for(int32_t i=0; i<2; i++)
        destroy_bitmap(select_bmp[i]);
        
    for(int32_t i=0; i<MAXARROWS; i++)
        destroy_bitmap(arrow_bmp[i]);
        
    al_trace(" OK. \n");
}


void quit_game()
{
    set_last_timed_save(nullptr);
    save_config_file();
    zc_set_palette(black_palette);
    zc_stop_midi();
    
    remove_locked_params_on_exit();
    
    al_trace("Cleaning sfx. \n");
    
    for(int32_t i=0; i<WAV_COUNT; i++)
    {
        if(customsfxdata[i].data!=NULL)
        {
//      delete [] customsfxdata[i].data;
            free(customsfxdata[i].data);
        }
        
        delete [] sfx_string[i];
    }
    
    for(int32_t i=0; i<MAXWPNS; i++)
    {
        delete [] weapon_string[i];
    }
    
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        delete [] item_string[i];
    }
    
    for(int32_t i=0; i<eMAXGUYS; i++)
    {
        delete [] guy_string[i];
    }
    
    al_trace("Cleaning script buffer. \n");
    
    for(int32_t i=0; i<NUMSCRIPTFFC; i++)
    {
        if(ffscripts[i]!=NULL) delete ffscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTITEM; i++)
    {
        if(itemscripts[i]!=NULL) delete itemscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
    {
        if(guyscripts[i]!=NULL) delete guyscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
    {
        if(lwpnscripts[i]!=NULL) delete lwpnscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
    {
        if(ewpnscripts[i]!=NULL) delete ewpnscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
    {
        if(screenscripts[i]!=NULL) delete screenscripts[i];
    }
    
    for(int32_t i=0; i<3; i++) //should this be NUMSCRIPTGLOBAL or NUMSCRIPTGLOBALOLD? -Z
    {
        if(globalscripts[i]!=NULL) delete globalscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTHERO; i++)
    {
        if(playerscripts[i]!=NULL) delete playerscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
    {
        if(dmapscripts[i]!=NULL) delete dmapscripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
    {
        if(itemspritescripts[i]!=NULL) delete itemspritescripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
    {
        if(comboscripts[i]!=NULL) delete comboscripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
    {
        if(subscreenscripts[i]!=NULL) delete subscreenscripts[i];
    }
    
    al_trace("Cleaning qst buffers. \n");
    del_qst_buffers();
    
    
    al_trace("Cleaning midis. \n");
    
    if(customtunes)
    {
        for(int32_t i=0; i<MAXCUSTOMMIDIS_ZQ; i++)
            customtunes[i].reset();
            
        free(customtunes);
    }
    
    al_trace("Cleaning undotilebuf. \n");
    
    undocombobuf.clear();
    
    if(newundotilebuf)
    {
        for(int32_t i=0; i<NEWMAXTILES; i++)
            if(newundotilebuf[i].data) free(newundotilebuf[i].data);
            
        free(newundotilebuf);
    }
    
    if(filepath) free(filepath);
    
    if(datapath) free(datapath);
    
    if(midipath) free(midipath);
    
    if(imagepath) free(imagepath);
    
    if(tmusicpath) free(tmusicpath);
    
    if(last_timed_save) free(last_timed_save);
    
    destroy_bitmaps_on_exit();
}

void quit_game2()
{
    set_last_timed_save(nullptr);
    save_config_file();
    zc_set_palette(black_palette);
    zc_stop_midi();
    
    remove_locked_params_on_exit();
    
    al_trace("Cleaning sfx. \n");
    
    for(int32_t i=0; i<WAV_COUNT; i++)
    {
        if(customsfxdata[i].data!=NULL)
        {
//      delete [] customsfxdata[i].data;
            free(customsfxdata[i].data);
        }
        
        delete [] sfx_string[i];
    }
    
    for(int32_t i=0; i<MAXWPNS; i++)
    {
        delete [] weapon_string[i];
    }
    
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        delete [] item_string[i];
    }
    
    for(int32_t i=0; i<eMAXGUYS; i++)
    {
        delete [] guy_string[i];
    }
    
    al_trace("Cleaning script buffer. \n");
    
    for(int32_t i=0; i<NUMSCRIPTFFC; i++)
    {
        if(ffscripts[i]!=NULL) delete ffscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTITEM; i++)
    {
        if(itemscripts[i]!=NULL) delete itemscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
    {
        if(guyscripts[i]!=NULL) delete guyscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
    {
        if(lwpnscripts[i]!=NULL) delete lwpnscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
    {
        if(ewpnscripts[i]!=NULL) delete ewpnscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
    {
        if(screenscripts[i]!=NULL) delete screenscripts[i];
    }
    
    for(int32_t i=0; i<3; i++) //should this be NUMSCRIPTGLOBAL or NUMSCRIPTGLOBALOLD? -Z
    {
        if(globalscripts[i]!=NULL) delete globalscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTHERO; i++)
    {
        if(playerscripts[i]!=NULL) delete playerscripts[i];
    }
    
    for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
    {
        if(dmapscripts[i]!=NULL) delete dmapscripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
    {
        if(itemspritescripts[i]!=NULL) delete itemspritescripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
    {
        if(comboscripts[i]!=NULL) delete comboscripts[i];
    }
    for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
    {
        if(subscreenscripts[i]!=NULL) delete subscreenscripts[i];
    }
    
    al_trace("Cleaning qst buffers. \n");
    del_qst_buffers();
    
    
    al_trace("Cleaning midis. \n");
    
    if(customtunes)
    {
        for(int32_t i=0; i<MAXCUSTOMMIDIS_ZQ; i++)
            customtunes[i].reset();
            
        free(customtunes);
    }
    
    al_trace("Cleaning undotilebuf. \n");
    
    undocombobuf.clear();
    
    if(newundotilebuf)
    {
        for(int32_t i=0; i<NEWMAXTILES; i++)
            if(newundotilebuf[i].data) free(newundotilebuf[i].data);
            
        free(newundotilebuf);
    }
    
    if(filepath) free(filepath);
    
    if(datapath) free(datapath);
    
    if(midipath) free(midipath);
    
    if(imagepath) free(imagepath);
    
    if(tmusicpath) free(tmusicpath);
    
    if(last_timed_save) free(last_timed_save);
}

void center_zquest_dialogs()
{
    jwin_center_dialog(assignscript_dlg);
    jwin_center_dialog(autolayer_dlg);
    center_zq_cset_dialogs();
    jwin_center_dialog(change_track_dlg);
    jwin_center_dialog(csetfix_dlg);
    jwin_center_dialog(dmapmaps_dlg);
    center_zq_door_dialogs();
    jwin_center_dialog(editcomboa_dlg);
    jwin_center_dialog(editinfo_dlg);
    jwin_center_dialog(editmusic_dlg);
    jwin_center_dialog(editshop_dlg);
    jwin_center_dialog(ffcombo_sel_dlg);
    jwin_center_dialog(getnum_dlg);
    jwin_center_dialog(layerdata_dlg);
    jwin_center_dialog(list_dlg);
    jwin_center_dialog(loadmap_dlg);
    jwin_center_dialog(misccolors_dlg);
    jwin_center_dialog(newcomboa_dlg);
    jwin_center_dialog(orgcomboa_dlg);
    jwin_center_dialog(path_dlg);
    jwin_center_dialog(screen_pal_dlg);
    jwin_center_dialog(secret_dlg);
    jwin_center_dialog(selectdmap_dlg);
    jwin_center_dialog(selectmusic_dlg);
    jwin_center_dialog(showpal_dlg);
    jwin_center_dialog(strlist_dlg);
    jwin_center_dialog(template_dlg);
    jwin_center_dialog(tp_dlg);
    jwin_center_dialog(under_dlg);
    jwin_center_dialog(tilewarp_dlg);
    jwin_center_dialog(sidewarp_dlg);
    jwin_center_dialog(warpring_dlg);
    center_zscript_dialogs();
}


void animate_coords()
{
    coord_frame=(coord_timer>>3)&3;
    
    if(++coord_timer>=(1<<5))
    {
        coord_timer=0;
    }
}

static const char *help_list[] =
{
    "PREVIEW MODE",
    "PgUp/PgDn - Scroll through hotkey list",
    "Esc/Enter - Exit Preview Mode",
    "R - Restore screen to original state",
    "C - Toggle combo cycling On/Off",
    "S - Trigger screen secrets",
    "Q/W/F - These still work",
    "P - Pause everything",
    "A - Advance frame-by-frame",
    "1-4 - Trigger tile warp A-D",
    "5-8 - Trigger side warp A-D",
    "9 - Enable timed warps",
    "",
    "",
};

void do_previewtext()
{
	FONT* oldfont = font;
	font = get_zc_font(font_lfont_l);
	
	//Put in help areas
	auto& sqr = preview_text;
	int ind = 0, len = 0;
	for(int q = 0; q < 12; ++q)
	{
		int l = text_length(font, help_list[q]);
		if(len < l) len = l;
	}
	sqr.xscale = len+2;
	sqr.yscale = text_height(font);
	for(int col = 0; col < sqr.w; ++col)
	{
		for(int row = 0; row < sqr.h; ++row)
		{
			auto& line = sqr.subsquare(col,row);
			textprintf_ex(menu1,font,line.x,line.y,jwin_pal[jcTEXTFG],-1,"%s",help_list[ind++]);
		}
	}
	
	font = oldfont;
}


bool reload_fonts = false;
void run_zq_frame()
{
	if(reload_fonts)
	{
		init_custom_fonts();
		load_size_poses();
		reload_fonts = false;
	}

	domouse();
	custom_vsync();
	refresh(rCLEAR|rALL);
}
int32_t d_nbmenu_proc(int32_t msg,DIALOG *d,int32_t c)
{
	switch(msg)
	{
		case MSG_VSYNC:
			run_zq_frame();
			break;
		case MSG_GOTMOUSE:
		case MSG_XCHAR:
			ComboBrushPause=1;
			refresh(rMAP);
			ComboBrushPause=0;
			clear_tooltip();
			break;
	}
	
	return GuiMenu::proc(msg,d,c);
}

bool prv_press=false;

void dopreview()
{
	refresh(rMAP);
	
	while(!(gui_mouse_b()))
	{
		if(keypressed())
		{
			if(!prv_press)
			{
				prv_press=true;
				
				switch(readkey()>>8)
				{
					case KEY_ESC:
					case KEY_ENTER:
					case KEY_ENTER_PAD:
						goto finished;
						break;
						
					case KEY_F:
						Flags^=cFLAGS;
						refresh(rMAP);
						break;
						
					case KEY_R:
						onReloadPreview();
						break;
						
					case KEY_S:
						onSecretsPreview();
						break;
						
					case KEY_C:
						onCopy();
						break;
						
					case KEY_A:
						onAKey();
						break;
						
					case KEY_P:
						onP();
						break;
					
					case KEY_L:
						onShowDarkness();
						break;
						
					case KEY_1:
						Map.prv_dowarp(0,0);
						prv_warp=0;
						break;
						
					case KEY_2:
						Map.prv_dowarp(0,1);
						prv_warp=0;
						break;
						
					case KEY_3:
						Map.prv_dowarp(0,2);
						prv_warp=0;
						break;
						
					case KEY_4:
						Map.prv_dowarp(0,3);
						prv_warp=0;
						break;
						
					case KEY_5:
						Map.prv_dowarp(1,0);
						prv_warp=0;
						break;
						
					case KEY_6:
						Map.prv_dowarp(1,1);
						prv_warp=0;
						break;
						
					case KEY_7:
						Map.prv_dowarp(1,2);
						prv_warp=0;
						break;
						
					case KEY_8:
						Map.prv_dowarp(1,3);
						prv_warp=0;
						break;
						
					case KEY_9:
						if(prv_twon)
						{
							prv_twon=0;
							Map.set_prvtime(0);
							prv_warp=0;
						}
						else
						{
							Map.set_prvtime(Map.get_prvscr()->timedwarptics);
							prv_twon=1;
						}
						
						break;
						
					case KEY_W:
						onShowWalkability();
						break;
						
					case KEY_Q:
						onShowComboInfoCSet();
						break;
				}
			}
			else
			{
				readkey();
			}
		}
		else
		{
			prv_press=false;
		}
		
		if(prv_warp)
		{
			Map.prv_dowarp(1,0);
			prv_warp=0;
		}
		
		if(Map.get_prvfreeze())
		{
			if(Map.get_prvadvance())
			{
				custom_vsync();
				Map.set_prvadvance(0);
			}
		}
		else
		{
			custom_vsync();
			Map.set_prvadvance(0);
		}
		
		refresh(rALL);
	}
	
finished:
	//Flags=of;
	reset_combo_animations();
	reset_combo_animations2();
	MouseSprite::set(ZQM_NORMAL);
	prv_mode=0;
	Map.set_prvcmb(0);
	Map.set_prvadvance(0);
	Map.set_prvfreeze(0);
	Map.set_prvtime(0);
	prv_warp=0;
	loadlvlpal(Map.getcolor());
	rebuild_trans_table();
	refresh(rMAP+rMENU);
	
	while(gui_mouse_b())
	{
		/* do nothing */
		rest(1);
	}
}

void call_vidmode_dlg();
int32_t onZQVidMode()
{
    call_vidmode_dlg();
    return D_O_K;
}

bool screenIsScrolling()
{
    return false;
}

void write_includepaths()
{
	FILE* f = fopen("includepaths.txt", "w");
	if(f)
	{
		fwrite(FFCore.includePathString,1,strlen(FFCore.includePathString),f);
		fclose(f);
	}
}

int32_t save_config_file()
{
	if (!application_has_loaded) return 0;

    char qtnametitle[20];
    char qtpathtitle[20];
    char *datapath2=(char *)malloc(2048);
    char *midipath2=(char *)malloc(2048);
    char *imagepath2=(char *)malloc(2048);
    char *tmusicpath2=(char *)malloc(2048);
    strcpy(datapath2, datapath);
    strcpy(midipath2, midipath);
    strcpy(imagepath2, imagepath);
    strcpy(tmusicpath2, tmusicpath);
    chop_path(datapath2);
    chop_path(midipath2);
    chop_path(imagepath2);
    chop_path(tmusicpath2);
    
	zc_set_config("ZCMODULE","current_module",moduledata.module_name);
	//
	write_includepaths();
	
    zc_set_config("zquest",data_path_name,datapath2);
    zc_set_config("zquest",midi_path_name,midipath2);
    zc_set_config("zquest",image_path_name,imagepath2);
    zc_set_config("zquest",tmusic_path_name,tmusicpath2);
	
    if (all_get_display() && !all_get_fullscreen_flag() && SaveDragResize) 
    {
		window_width = al_get_display_width(all_get_display());
		window_height = al_get_display_height(all_get_display());
		zc_set_config("zquest","window_width",window_width);
		zc_set_config("zquest","window_height",window_height);
    }
    if (all_get_display() && !all_get_fullscreen_flag() && SaveWinPos)
    {
		int o_window_x, o_window_y;
		al_get_window_position(all_get_display(), &o_window_x, &o_window_y);
		zc_set_config("zquest", "window_x", o_window_x);
		zc_set_config("zquest", "window_y", o_window_y);
    }
    
	byte b = 0;
    for(int32_t x=0; x<7; x++)
    {
        set_bit(&b,x,LayerMaskInt[x]);
    }
    
    zc_set_config("zquest","layer_mask",b);
    
    flush_config_file();
#ifdef __EMSCRIPTEN__
    em_sync_fs();
#endif
    free(datapath2);
    free(midipath2);
    free(imagepath2);
    free(tmusicpath2);
    return 0;
}

int32_t d_timer_proc(int32_t msg, DIALOG *d, int32_t c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    d=d;
    
    switch(msg)
    {
    case MSG_IDLE:
#ifdef _WIN32
        if(zqUseWin32Proc != FALSE)
            win32data.Update(Frameskip); //experimental win32 fixes
            
#endif
            
        // This has been crashing on Windows, and it saves plenty without it
        //check_autosave();
        break;
    }
    
    return D_O_K;
}

void check_autosave()
{
    if (!first_save)
        return;

    if(AutoSaveInterval>0)
    {
        time(&auto_save_time_current);
        auto_save_time_diff = difftime(auto_save_time_current,auto_save_time_start);
        
        if(auto_save_time_diff>AutoSaveInterval*60)
        {
            MouseSprite::set(ZQM_NORMAL);
			replace_extension(last_timed_save, filepath, "qt0", 2047);
			set_last_timed_save(last_timed_save);
            go();
            
            if((header.zelda_version != ZELDA_VERSION || header.build != VERSION_BUILD))
            {
                jwin_alert("Auto Save","This quest was saved in an older version of ZQuest.","If you wish to use the autosave feature, you must manually","save the files in this version first.","OK",NULL,13,27,get_zc_font(font_lfont));
                time(&auto_save_time_start);
                comeback();
                return;
            }
            
            int32_t ret = save_quest(last_timed_save, true);
            
            if(ret)
            {
                jwin_alert("Error","Timed save did not complete successfully.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
                set_last_timed_save(nullptr);
            }
            
            save_config_file();
            time(&auto_save_time_start);
            comeback();
        }
    }
}

void flushItemCache(bool) {}
void ringcolor(bool forceDefault)
{
    forceDefault=forceDefault;
}

bool item_disabled(int32_t)
{
    return false;
}

int32_t onCmdExit()
{
    // replaces onExit for the -large button command "Exit"
    close_button_quit = true;
    return 0;
}

int32_t onQuickCompile()
{
	if(do_compile_and_slots(1,false))
	{
		saved = false;
		InfoDialog("Quick Compile","Success!").show();
	}
	else
	{
		InfoDialog("Quick Compile","Failure!").show();
	}
	return 0;
}
int32_t onSmartCompile()
{
	if(do_compile_and_slots(2,false))
	{
		saved = false;
		InfoDialog("Smart Compile","Success!").show();
	}
	else
	{
		InfoDialog("Smart Compile","Failure!").show();
	}
	return 0;
}

int32_t strchrnum(char const* str, char c)
{
	for(int32_t i=0; str[i]; ++i)
	{
		if(str[i]==c)
		{
			return i;
		}
	}
	
	return -1;
}

int32_t get_longest_line_length(FONT *f, char* str)
{
	int32_t maxlen=0;
	char* tmpstr = str;
	char temp=0;
	int32_t t=0;
	int32_t new_t=-1;
	while(tmpstr[0])
	{
		t=strchrnum(tmpstr, '\n');
		
		if(t==-1)
		{
			t=(int32_t)strlen(tmpstr);
		}
		
		if((uint32_t)t!=strlen(tmpstr))
		{
			new_t=t+1;
		}
		else
		{
			new_t=-1;
		}
		
		temp = tmpstr[t];
		tmpstr[t]=0;
		maxlen=zc_max(maxlen,text_length(f, tmpstr));
		tmpstr[t]=temp;
		
		if(new_t!=-1)
		{
			tmpstr+=new_t;
		}
		else break;
	}
	return maxlen;
}

int32_t count_lines(char const* str)
{
	int32_t count=1;
	
	for(word i=0; i<strlen(str); ++i)
	{
		if(str[i]=='\n')
		{
			++count;
		}
	}
	
	return count;
}

void debug_pos(size_and_pos const& pos, int color)
{
	if(pos.w < 1 || pos.h < 1)
		return;
	if(pos.xscale > 1 || pos.yscale > 1)
	{
		auto maxind = pos.w*pos.h;
		for(auto q = 0; q < maxind; ++q)
		{
			auto& sub = pos.subsquare(q);
			if(sub.x < 0) break;
			highlight_sqr(screen, color, sub, 1);
		}
	}
	else
	{
		if(pos.fw > -1 && pos.fh > -1)
			highlight_frag(screen, color, pos, 1);
		else highlight_sqr(screen, color, pos, 1);
	}
}

void textbox_out(BITMAP* dest, FONT* font, int x, int y, int fg, int bg, char const* str, int align, size_and_pos* dims)
{
	static size_and_pos nilsz;
	size_and_pos& txbox = dims ? *dims : nilsz;
	
	char* kill = (char*)malloc(strlen(str)+1);
	char *tmpstr = kill;
	strcpy(tmpstr,str);
	
	while(tmpstr[0] == '\n')
		++tmpstr;
	int len = strlen(tmpstr);
	while(tmpstr[len-1] == '\n')
		tmpstr[--len] = 0;
	
	txbox.x=x;
	txbox.y=y;
	int32_t lines=count_lines(tmpstr);
	txbox.w = 1;
	txbox.h = lines;
	txbox.xscale = get_longest_line_length(font, tmpstr);
	txbox.yscale = text_height(font);
	
	int ax = 0; //Aligned x
	switch(align)
	{
		case 0: //left
			break;
		case 1: //center
			txbox.x -= txbox.xscale/2;
			ax = txbox.xscale/2;
			break;
		case 2: //right
			txbox.x -= txbox.xscale;
			ax = txbox.xscale;
			break;
	}
	
	int bw = txbox.w*txbox.xscale;
	int bh = txbox.h*txbox.yscale;
	BITMAP* outbmp = create_bitmap_ex(8, bw, bh);
	clear_to_color(outbmp, bg);
	
	char temp = 0;
	int32_t t=0;
	int32_t new_t=-1;
	int32_t line=0;
	
	while(tmpstr[t])
	{
		t=strchrnum(tmpstr, '\n');
		
		if(t==-1)
			t=(int32_t)strlen(tmpstr);
		
		if((uint32_t)t!=strlen(tmpstr))
			new_t=t+1;
		else
			new_t=-1;
		
		temp = tmpstr[t];
		tmpstr[t]=0;
		gui_textout_ln(outbmp, font, (ucc*)tmpstr, ax, (line*txbox.yscale), fg, -1, align);
		tmpstr[t]=temp;
		++line;
		
		if(new_t!=-1)
		{
			tmpstr+=new_t;
			t=0;
		}
	}
	
	blit(outbmp, dest, 0, 0, txbox.x, txbox.y, bw, bh);
	destroy_bitmap(outbmp);
	free(kill);
}

void highlight_sqr(BITMAP* dest, int color, int x, int y, int w, int h, int thick)
{
	for(int q = 0; q < thick; ++q)
	{
		safe_rect(dest, x+q, y+q, x+w-1-q, y+h-1-q, color);
	}
}
void highlight_sqr(BITMAP* dest, int color, size_and_pos const& rec, int thick)
{
	highlight_sqr(dest, color, rec.x, rec.y, rec.tw(), rec.th(), thick);
}
void highlight_frag(BITMAP* dest, int color, int x1, int y1, int w, int h, int fw, int fh, int thick)
{
	int xc = x1+fw-1;
	int yc = y1+fh-1;
	int x2 = x1+w-1;
	int y2 = y1+h-1;
	
    hline(dest, x1, y1, x2, color);
    vline(dest, x1, y1, y2, color);
    
    hline(dest, x1, y2, xc, color);
    vline(dest, x2, y1, yc, color);
    hline(dest, xc, yc, x2, color);
    vline(dest, xc, yc, y2, color);
}
void highlight_frag(BITMAP* dest, int color, size_and_pos const& rec, int thick)
{
	highlight_frag(dest, color, rec.x, rec.y, rec.tw(), rec.th(), rec.fw, rec.fh, thick);
}

void highlight(BITMAP* dest, size_and_pos& hl)
{
	if(hl.fw > -1 && hl.fh > -1)
	{
		highlight_frag(dest, hl.data[1], hl, hl.data[0]);
	}
	else highlight_sqr(dest, hl.data[1], hl, hl.data[0]);
}

std::pair<int, int> get_box_text_size(char const* tipmsg, double txscale)
{
	if(txscale < 1) txscale = 1;
	char* kill = (char*)malloc(strlen(tipmsg)+1);
	char *tmpstr = kill;
	strcpy(tmpstr,tipmsg);
	
	while(tmpstr[0] == '\n')
		++tmpstr;
	int len = strlen(tmpstr);
	while(tmpstr[len-1] == '\n')
		tmpstr[--len] = 0;
	
	int32_t lines = count_lines(tmpstr);
	int txlen = get_longest_line_length(font, tmpstr);
	int txhei = lines*text_height(font);
	int tx_sclen = (txlen * txscale);
	int tx_schei = (txhei * txscale);
	int w = tx_sclen + 8 + 1;
	int h = tx_schei + 8 + 1;
	if (w > zq_screen_w)
		w = zq_screen_w;
	if (h > zq_screen_h)
		h = zq_screen_h;
	return {w, h};
}

void draw_box(BITMAP* destbmp, size_and_pos* pos, char const* tipmsg, double txscale)
{
	if(txscale < 1) txscale = 1;
	char* kill = (char*)malloc(strlen(tipmsg)+1);
	char *tmpstr = kill;
	strcpy(tmpstr,tipmsg);
	
	while(tmpstr[0] == '\n')
		++tmpstr;
	int len = strlen(tmpstr);
	while(tmpstr[len-1] == '\n')
		tmpstr[--len] = 0;
	
	auto& box = *pos;
	clear_bitmap(destbmp);
	
	int32_t lines=count_lines(tmpstr);
	int txlen = get_longest_line_length(font, tmpstr);
	int txhei = lines*text_height(font);
	int tx_sclen = (txlen * txscale);
	int tx_schei = (txhei * txscale);
	box.w = tx_sclen + 8 + 1;
	box.h = tx_schei + 8 + 1;
	if (box.w > zq_screen_w)
		box.w = zq_screen_w;
	if (box.h > zq_screen_h)
		box.h = zq_screen_h;
	
	if(box.x+box.w>=zq_screen_w)
	{
		box.x=(zq_screen_w - box.w);
	}
	
	if(box.y+box.h>=zq_screen_h)
	{
		box.y=(zq_screen_h - box.h);
	}
	
	rectfill(destbmp, 1, 1, box.w-3, box.h-3, jwin_pal[jcTEXTBG]);
	rect(destbmp, 0, 0, box.w-2, box.h-2, jwin_pal[jcTEXTFG]);
	vline(destbmp, box.w-1, 0, box.h-1, jwin_pal[jcTEXTFG]);
	hline(destbmp, 1, box.h-1, box.w-2, jwin_pal[jcTEXTFG]);
	destbmp->line[box.h-1][0]=0;
	destbmp->line[0][box.w-1]=0;
	
	char temp = 0;
	int32_t t=0;
	int32_t new_t=-1;
	int32_t line=0;
	
	BITMAP* txbmp = create_bitmap_ex(8,box.w,box.h);
	clear_bitmap(txbmp);
	while(tmpstr[t])
	{
		t=strchrnum(tmpstr, '\n');
		
		if(t==-1)
		{
			t=(int32_t)strlen(tmpstr);
		}
		
		if((uint32_t)t!=strlen(tmpstr))
		{
			new_t=t+1;
		}
		else
		{
			new_t=-1;
		}
		
		temp = tmpstr[t];
		tmpstr[t]=0;
		textprintf_ex(txbmp, font, 0, (line*text_height(font)), jwin_pal[jcTEXTFG], -1, "%s", tmpstr);
		tmpstr[t]=temp;
		++line;
		
		if(new_t!=-1)
		{
			tmpstr+=new_t;
			t=0;
		}
	}
	masked_stretch_blit(txbmp,destbmp,0,0,txlen,txhei,4,4,tx_sclen,tx_schei);
	destroy_bitmap(txbmp);
	free(kill);
}

void update_tooltip(int32_t x, int32_t y, size_and_pos const& sqr, char const* tipmsg, double scale)
{
	update_tooltip(x,y,sqr.x,sqr.y,sqr.w*sqr.xscale,sqr.h*sqr.yscale,tipmsg,sqr.fw,sqr.fh,scale);
}
void update_tooltip(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t tw, int32_t th, char const* tipmsg, int fw, int fh, double scale)
{
	if(!EnableTooltips)
	{
		return;
	}

	ttip_install(ttip_global_id, tipmsg, tx, ty, tw, th, x, y, fw, fh);
}

void ZQ_ClearQuestPath()
{
	zc_set_config("zquest","win_last_quest",(char const*)nullptr);
	strcpy(filepath,"");
}

//FFCore

int32_t FFScript::GetScriptObjectUID(int32_t type)
{
	++script_UIDs[type];
	return script_UIDs[type];
}

void FFScript::init()
{
	for ( int32_t q = 0; q < wexLast; q++ ) warpex[q] = 0;
	numscriptdraws = 0;
	max_ff_rules = qr_MAX;
	temp_no_stepforward = 0;
	nostepforward = 0;
	
	coreflags = 0;
	skip_ending_credits = 0;
	music_update_cond = 0;
	music_update_flags = 0;
	for ( int32_t q = 0; q < susptLAST; q++ ) { system_suspend[q] = 0; }
	
	for ( int32_t q = 0; q < UID_TYPES; ++q ) { script_UIDs[q] = 0; }
	//for ( int32_t q = 0; q < 512; q++ ) FF_rules[q] = 0;
	int32_t usr_midi_volume = usr_digi_volume = usr_sfx_volume = usr_music_volume = usr_panstyle = 0;
	FF_hero_action = 0;
	enemy_removal_point[spriteremovalY1] = -32767;
	enemy_removal_point[spriteremovalY2] = 32767;
	enemy_removal_point[spriteremovalX1] = -32767;
	enemy_removal_point[spriteremovalX2] = 32767;
	enemy_removal_point[spriteremovalZ1] = -32767;
	enemy_removal_point[spriteremovalZ2] = 32767;
	
	for ( int32_t q = 0; q < 4; q++ ) 
	{
		FF_screenbounds[q] = 0;
		FF_screen_dimensions[q] = 0;
		FF_subscreen_dimensions[q] = 0;
		FF_eweapon_removal_bounds[q] = 0; 
		FF_lweapon_removal_bounds[q] = 0;
	}
	for ( int32_t q = 0; q < FFSCRIPTCLASS_CLOCKS; q++ )
	{
		FF_clocks[q] = 0;
	}
	for ( int32_t q = 0; q < SCRIPT_DRAWING_RULES; q++ )
	{
		ScriptDrawingRules[q] = 0;
	}
	for ( int32_t q = 0; q < NUM_USER_MIDI_OVERRIDES; q++ ) 
	{
		FF_UserMidis[q] = 0;
	}
	subscreen_scroll_speed = 0; //make a define for a default and read quest override! -Z
	kb_typing_mode = false;
	initIncludePaths();
	for(int32_t q = 0; q < 7; ++q)
	{
		tempScreens[q] = NULL;
		ScrollingScreens[q] = NULL;
	}
}

void FFScript::updateIncludePaths()
{
	includePaths.clear();
	int32_t pos = 0; int32_t pathnumber = 0;
	for ( int32_t q = 0; includePathString[pos]; ++q )
	{
		int32_t dest = 0;
		char buf[2048] = {0};
		while(includePathString[pos] != ';' && includePathString[pos])
		{
			buf[dest] = includePathString[pos];
			++pos;
			++dest;
		}
		++pos;
		string str(buf);
		includePaths.push_back(str);
	}
}

void FFScript::initIncludePaths()
{
	memset(includePathString,0,sizeof(includePathString));
	FILE* f = fopen("includepaths.txt", "r");
	if(f)
	{
		int32_t pos = 0;
		int32_t c;
		do
		{
			c = fgetc(f);
			if(c!=EOF) 
				includePathString[pos++] = c;
		}
		while(c!=EOF && pos<MAX_INCLUDE_PATH_CHARS);
		if(pos<MAX_INCLUDE_PATH_CHARS)
			includePathString[pos] = '\0';
		includePathString[MAX_INCLUDE_PATH_CHARS-1] = '\0';
		fclose(f);
	}
	else strcpy(includePathString, "include/;headers/;scripts/;");
	al_trace("Full path string is: ");
	safe_al_trace(includePathString);
	al_trace("\n");
	updateIncludePaths();

	for ( size_t q = 0; q < includePaths.size(); ++q )
	{
		al_trace("Include path %zu: ",q);
		safe_al_trace(includePaths.at(q).c_str());
		al_trace("\n");
	}
}

int32_t FFScript::getQRBit(int32_t rule)
{
	return ( get_qr(rule) ? 1 : 0 );
}

int32_t FFScript::getTime(int32_t type)
{
	//struct tm *tm_struct = localtime(time(NULL));
	struct tm * tm_struct;
	time_t rawtime;
	time (&rawtime);
	tm_struct = localtime (&rawtime);
	
	switch(type)
	{
		case curyear:
		{
			int32_t year = tm_struct->tm_year + 1900;        /* year */
			//year format starts at 1900, so we add it to the return
			return year;
			
		}
		case curmonth:
		{
			int32_t month = tm_struct->tm_mon +1;         /* month */
			//Months start at 0, but we want 1->12
			return month;
		}
		case curday_month:
		{
			int32_t day_month = tm_struct->tm_mday;        /* day of the month */
			return day_month;
		}
		case curday_week: 
		{
			int32_t day_week = tm_struct->tm_wday;        /* day of the week */
			return day_week;
		}
		case curhour:
		{
			int32_t hour = tm_struct->tm_hour;        /* hours */
			return hour;
		}
		case curminute: 
		{
			int32_t minutes = tm_struct->tm_min;         /* minutes */
			return minutes;
		}
		case cursecond:
		{
			int32_t secs = tm_struct->tm_sec;         /* seconds */
			return secs;
		}
		case curdayyear:
		{
			int32_t day_year = tm_struct->tm_yday;        /* day in the year */
			return day_year;
		}
		case curDST:
		{
			int32_t isDST = tm_struct->tm_isdst;       /* daylight saving time */
			return isDST;
		}
		default: return -1;
		
	}
}

extern const char *itemclass_help_string_defaults[itype_max];
//ZModule Functions



/* end */

int32_t FFScript::getQuestHeaderInfo(int32_t type)
{
    return quest_format[type];
}

bool isSideViewGravity(int32_t t)
{
    return (Map.CurrScr()->flags7&fSIDEVIEW) != 0;
}




void FFScript::ZScriptConsole(bool open)
{


	#ifdef _WIN32
	if ( console_is_open )
	{
		zscript_coloured_console.Create("ZQuest Classic Logging Console", 600, 200);
		zscript_coloured_console.cls(CConsoleLoggerEx::COLOR_BACKGROUND_BLACK);
		zscript_coloured_console.gotoxy(0,0);
		zscript_coloured_console.cprintf( CConsoleLoggerEx::COLOR_BLUE | CConsoleLoggerEx::COLOR_INTENSITY |
		CConsoleLoggerEx::COLOR_BACKGROUND_BLACK,"ZQuest Classic Logging Console\n");
	}
	else
	{
		//close
		zscript_coloured_console.Close();
	}
	#endif	
}

template <typename ...Params>
void FFScript::ZScriptConsole(int32_t attributes,const char *format, Params&&... params)
{
	#ifdef _WIN32
	initConsole();
	zscript_coloured_console.cprintf( attributes, format, std::forward<Params>(params)... );
	#endif	
}

int32_t getpitfall(int32_t x, int32_t y){return 0;}

int32_t iswaterexzq(int32_t combo, int32_t map, int32_t screen, int32_t layer, int32_t x, int32_t y, bool secrets, bool fullcheck, bool LayerCheck){return 0;}

int32_t MAPCOMBOzq(int32_t x, int32_t y){return 0;}

bool update_hw_pal = false;
void update_hw_screen()
{
	if (is_headless())
		return;

	framecnt++;

	zc_process_display_events();
	if (update_hw_pal)
	{
		zc_set_palette(RAMpal);
		update_hw_pal = false;
	}

	render_timer_wait();
	render_zq();
}

bool checkCost(int32_t ctr, int32_t amnt)
{
	if(!game) return true;
	if(amnt <= 0) return true;
	switch (ctr)
	{
		case crMONEY: //rupees
		{
			if ( current_item_power(itype_wallet) ) return true;
			break;
		}
		case crMAGIC: //magic
		{
			if (get_qr(qr_ENABLEMAGIC))
			{
				return (((current_item_power(itype_magicring) > 0)
					 ? game->get_maxmagic()
					 : game->get_magic()+game->get_dmagic())>=amnt*game->get_magicdrainrate());
			}
			return true;
		}
		case crARROWS:
		{
			if(current_item_power(itype_quiver))
				return true;
			if(!get_qr(qr_TRUEARROWS))
				return checkCost(crMONEY, amnt);
			break;
		}
		case crBOMBS:
		{
			if(current_item_power(itype_bombbag))
				return true;
			break;
		}
		case crSBOMBS:
		{
			if(current_item_power(itype_bombbag)
				&& itemsbuf[current_item_id(itype_bombbag)].flags & item_flag1)
				return true;
			break;
		}
	}
	return (game->get_counter(ctr)+game->get_dcounter(ctr)>=amnt);
}
bool checkmagiccost(int32_t itemid, bool checkTime)
{
	if(itemid < 0)
	{
		return false;
	}
	itemdata const& id = itemsbuf[itemid];
	return checkCost(id.cost_counter[0], id.cost_amount[0])
		&& checkCost(id.cost_counter[1], id.cost_amount[1]);
}

void payCost(int32_t ctr, int32_t amnt, int32_t tmr, bool ignoreTimer)
{
	return;
}
void paymagiccost(int32_t itemid, bool ignoreTimer, bool onlyTimer)
{
	return;
}

void enter_sys_pal(){}
void exit_sys_pal(){}

void replay_step_comment(std::string comment) {}
bool replay_is_active() {return false;}
bool replay_is_replaying() {return false;}
bool replay_version_check(int min, int max) {return false;}
bool replay_is_debug() {return false;}
int32_t item::run_script(int32_t mode){return 0;};
ffcdata* slopes_getFFC(int index)
{
	return nullptr;
}

#ifdef __EMSCRIPTEN__
extern "C" void open_test_mode()
{
	int dmap = -1;
	int32_t pal = Map.getcolor();
	for(auto q = 0; q < MAXDMAPS; ++q)
	{
		if(DMaps[q].map == Map.getCurrMap())
		{
			if(pal == DMaps[q].color)
			{
				dmap = q;
				break;
			}
			if(dmap < 0)
				dmap = q;
		}
	}
	if(dmap < 0) dmap = 0;

	em_open_test_mode(filepath, dmap, Map.getCurrScr(), -1);
}

extern "C" void get_shareable_url()
{
	EM_ASM({
        ZC.setShareableUrl({open: UTF8ToString($0), map: $1, screen: $2});
	}, filepath, Map.getCurrMap(), Map.getCurrScr());
}
#endif
