#include "base/zdefs.h"
#include "base/hotkey.h"
#include "zq/render_hotkeys.h"
#include "zq/zq_files.h"
#include "zq/zquest.h"
#include "zq/zq_misc.h"
#include "zq/zq_hotkey.h"
#include "zq/zq_class.h"
#include "dialog/zqhotkeys.h"
#include "dialog/info.h"
#include "dialog/externs.h"
#include <fmt/format.h>

extern int32_t prv_mode;
extern bool placing_flags;
extern int32_t alias_origin;

int onSetNewLayer(int newlayer);
int onScreenLPal(int lpal);
int32_t do_OpenQuest();
int32_t onSelectFFCombo();
int onReloadPreview();
int onScrollScreen(int dir, bool warp);
int32_t onComboColLeft();
int32_t onComboColRight();
int32_t onComboColUp();
int32_t onComboColDown();
int32_t onCustomEnemies();
int32_t onCmdExit();
int32_t onIntegrityCheckAll();
int32_t onIntegrityCheckRooms();
int32_t onIntegrityCheckWarps();
int32_t do_NewQuest();
int32_t onEditSubscreens();
int32_t onRulesDlg();
int32_t onComboLocationReport();
int32_t onComboTypeLocationReport();
int32_t onEnemyLocationReport();
int32_t onItemLocationReport();
int32_t onScriptLocationReport();
int32_t onWhatWarpsReport();
int32_t onBuggedNextComboLocationReport();
int32_t onLayer2BG();
int32_t onLayer3BG();
int32_t onRulesSearch();
int32_t onQuickCompile();
int32_t onSmartCompile();
int32_t onStatusEffects();
int toggleConsole();
int showHotkeys();
void cycle_compact_sqr(bool down);
void call_testqst_dialog();
extern uint8_t ViewLayer2BG, ViewLayer3BG;
extern int32_t draw_mode;

char const* get_hotkey_name(uint hkey)
{
	switch(hkey)
	{
		case ZQKEY_NULL_KEY: return "(None)";
		case ZQKEY_UNDO: return "Undo";
		case ZQKEY_REDO: return "Redo";
		case ZQKEY_MINUS_FLAG: return "Flag -";
		case ZQKEY_PLUS_FLAG: return "Flag +";
		case ZQKEY_SAVE: return "Save";
		case ZQKEY_SAVEAS: return "Save As";
		case ZQKEY_OPEN: return "Open Quest";
		case ZQKEY_SCREEN_PAL: return "Screen Palette";
		case ZQKEY_SECRET_COMBO: return "Secret Combos";
		case ZQKEY_DOORS: return "Doors";
		case ZQKEY_FFCS: return "Freeform Combos";
		case ZQKEY_FLAGS: return "Combo Flags";
		case ZQKEY_SCRDATA: return "Screen Data";
		case ZQKEY_TILEWARP: return "Tile Warps";
		case ZQKEY_SIDEWARP: return "Side Warps";
		case ZQKEY_LAYERS: return "Layers";
		case ZQKEY_RESET_TRANSP: return "Reset Transparency";
		case ZQKEY_COPY: return "Copy Screen";
		case ZQKEY_TOGGLE_DARK: return "Toggle Screen Darkness";
		case ZQKEY_ENEMIES: return "Screen Enemies";
		case ZQKEY_SHOW_FLAGS: return "Show Flags";
		case ZQKEY_SHOW_FFCS: return "Show FFCs";
		case ZQKEY_ITEMS: return "Screen Item";
		case ZQKEY_COMBOS: return "Combo Pages";
		case ZQKEY_DARK_PREVIEW: return "Preview Darkness";
		case ZQKEY_SHOW_INFO: return "Show Screen Info";
		case ZQKEY_DRAWING_MODE: return "Cycle Drawing Mode";
		case ZQKEY_GOTO_PAGE: return "Goto Combo Page";
		case ZQKEY_SHOW_CMB_CS_INFO: return "Cycle Combo Info";
		case ZQKEY_STRINGS: return "String Editor";
		case ZQKEY_ROOM: return "Room Data";
		case ZQKEY_TILES: return "Tile Pages";
		case ZQKEY_PASTE: return "Paste Screen";
		case ZQKEY_PASTEALL: return "Paste All";
		case ZQKEY_PASTETOALL: return "Paste To All";
		case ZQKEY_PASTEALLTOALL: return "Paste All To All";
		case ZQKEY_SHOW_SOLID: return "Show Walkability";
		case ZQKEY_PREV_MODE: return "Preview Mode";
		case ZQKEY_COMPILE_ZSCRIPT: return "Compile ZScript";
		case ZQKEY_SCREENSHOT: return "Snapshot";
		case ZQKEY_ABOUT: return "About ZQ";
		case ZQKEY_MINUS_MAP: return "Map -1";
		case ZQKEY_PLUS_MAP: return "Map +1";
		case ZQKEY_MINUS_COLOR: return "CSet -1";
		case ZQKEY_PLUS_COLOR: return "CSet +1";
		case ZQKEY_MINUS_SCR_PAL: return "Screen Palette -1 (Preview)";
		case ZQKEY_PLUS_SCR_PAL: return "Screen Palette +1 (Preview)";
		case ZQKEY_MINUS_16_SCR_PAL: return "Screen Palette -16 (Preview)";
		case ZQKEY_PLUS_16_SCR_PAL: return "Screen Palette +16 (Preview)";
		case ZQKEY_SCREEN_ZOOM_IN: return "Zoom In";
		case ZQKEY_SCREEN_ZOOM_OUT: return "Zoom Out";
		case ZQKEY_GRID: return "Show Grid";
		case ZQKEY_GRID_COLOR: return "Cycle Grid Color";
		case ZQKEY_CHANGE_ALIAS_ORIGIN: return "Change Combo Alias Origin";
		case ZQKEY_DELETE: return "Delete Screen";
		case ZQKEY_FULLSCREEN: return "Toggle Fullscreen";
		case ZQKEY_LYR_0: return "Edit Layer 0";
		case ZQKEY_LYR_1: return "Edit Layer 1";
		case ZQKEY_LYR_2: return "Edit Layer 2";
		case ZQKEY_LYR_3: return "Edit Layer 3";
		case ZQKEY_LYR_4: return "Edit Layer 4";
		case ZQKEY_LYR_5: return "Edit Layer 5";
		case ZQKEY_LYR_6: return "Edit Layer 6";
		case ZQKEY_SCR_LPAL_0: return "Set Screen Palette: 0";
		case ZQKEY_SCR_LPAL_1: return "Set Screen Palette: 1";
		case ZQKEY_SCR_LPAL_2: return "Set Screen Palette: 2";
		case ZQKEY_SCR_LPAL_3: return "Set Screen Palette: 3";
		case ZQKEY_SCR_LPAL_4: return "Set Screen Palette: 4";
		case ZQKEY_SCR_LPAL_5: return "Set Screen Palette: 5";
		case ZQKEY_SCR_LPAL_6: return "Set Screen Palette: 6";
		case ZQKEY_SCR_LPAL_7: return "Set Screen Palette: 7";
		case ZQKEY_SCR_LPAL_8: return "Set Screen Palette: 8";
		case ZQKEY_SCR_LPAL_9: return "Set Screen Palette: 9";
		case ZQKEY_SCR_LPAL_10: return "Set Screen Palette: A";
		case ZQKEY_SCR_LPAL_11: return "Set Screen Palette: B";
		case ZQKEY_SCR_LPAL_12: return "Set Screen Palette: C";
		case ZQKEY_SCR_LPAL_13: return "Set Screen Palette: D";
		case ZQKEY_SCR_LPAL_14: return "Set Screen Palette: E";
		case ZQKEY_SCR_LPAL_15: return "Set Screen Palette: F";
		case ZQKEY_SCROLL_SCREEN_UP: return "Screen Up";
		case ZQKEY_SCROLL_SCREEN_DOWN: return "Screen Down";
		case ZQKEY_SCROLL_SCREEN_LEFT: return "Screen Left";
		case ZQKEY_SCROLL_SCREEN_RIGHT: return "Screen Right";
		case ZQKEY_WARP_SCREEN_UP: return "Screen Sidewarp Up";
		case ZQKEY_WARP_SCREEN_DOWN: return "Screen Sidewarp Down";
		case ZQKEY_WARP_SCREEN_LEFT: return "Screen Sidewarp Left";
		case ZQKEY_WARP_SCREEN_RIGHT: return "Screen Sidewarp Right";
		case ZQKEY_SCROLL_COMBO_UP: return "Scroll Combopane Up";
		case ZQKEY_SCROLL_COMBO_DOWN: return "Scroll Combopane Down";
		case ZQKEY_SCROLL_COMBO_LEFT: return "Scroll Combopane Left";
		case ZQKEY_SCROLL_COMBO_RIGHT: return "Scroll Combopane Right";
		case ZQKEY_COMBO_PAGEUP: return "Combopane Page Up";
		case ZQKEY_COMBO_PAGEDN: return "Combopane Page Down";
		case ZQKEY_SQUAREPANEL_UP: return "Compact Squarepanel Up";
		case ZQKEY_SQUAREPANEL_DOWN: return "Compact Squarepanel Down";
		case ZQKEY_TESTMODE: return "Test Quest";
		case ZQKEY_CAUTO_HEIGHTMINUS: return "Autocombo Height -";
		case ZQKEY_CAUTO_HEIGHTPLUS: return "Autocombo Height +";
		case ZQKEY_CURR_LAYER_HL: return "Highlight Current Layer";
		case ZQKEY_VIEW_MAP: return "View Map";
		case ZQKEY_DRAWMODE_NORMAL: return "Drawing Mode (Normal)";
		case ZQKEY_DRAWMODE_ALIAS: return "Drawing Mode (Alias)";
		case ZQKEY_DRAWMODE_POOL: return "Drawing Mode (Pool)";
		case ZQKEY_DRAWMODE_AUTO: return "Drawing Mode (Auto Combo)";
		case ZQKEY_IMPORT_COMBOS: return "Import Combos";
		case ZQKEY_EXPORT_COMBOS: return "Export Combos";
		case ZQKEY_IMPORT_DMAPS: return "Import DMaps";
		case ZQKEY_EXPORT_DMAPS: return "Export DMaps";
		case ZQKEY_IMPORT_ZGP: return "Import Graphics Pack";
		case ZQKEY_EXPORT_ZGP: return "Export Graphics Pack";
		case ZQKEY_IMPORT_MAP: return "Import Map";
		case ZQKEY_EXPORT_MAP: return "Export Map";
		case ZQKEY_IMPORT_PALS: return "Import Palettes";
		case ZQKEY_EXPORT_PALS: return "Export Palettes";
		case ZQKEY_IMPORT_STRINGS: return "Import Strings";
		case ZQKEY_EXPORT_STRINGS: return "Export Strings";
		case ZQKEY_IMPORT_TILES: return "Import Tiles";
		case ZQKEY_EXPORT_TILES: return "Export Tiles";
		case ZQKEY_CHANGE_TRACK: return "Change track";
		case ZQKEY_CHEATS: return "Cheats";
		case ZQKEY_CSET_FIX: return "Color Set Fix";
		case ZQKEY_ALIASES: return "Combo Aliases";
		case ZQKEY_CMBPOOLS: return "Combo Pools";
		case ZQKEY_AUTOCMB: return "Auto Combos";
		case ZQKEY_DEFAULT_COMBOS: return "Default Combos";
		case ZQKEY_DELETE_MAP: return "Delete Map";
		case ZQKEY_DMAPS: return "DMaps";
		case ZQKEY_DOOR_COMBO_SETS: return "Door Combo Sets";
		case ZQKEY_PASTE_DOORS: return "Paste Doors";
		case ZQKEY_ENDSTRING: return "End String";
		case ZQKEY_EDIT_ENEMIES: return "Enemy Editor";
		case ZQKEY_DEFAULT_ENEMIES: return "Default Enemies";
		case ZQKEY_PASTE_ENEMIES: return "Paste Enemies";
		case ZQKEY_EXIT: return "Exit";
		case ZQKEY_PASTE_FFCS: return "Paste Freeform Combos";
		case ZQKEY_GAME_ICONS: return "Game icons";
		case ZQKEY_GOTO_MAP: return "Goto Map";
		case ZQKEY_PASTE_GUY_STR: return "Paste Guy/String";
		case ZQKEY_HEADER: return "Header";
		case ZQKEY_INFO_TYPES: return "Info Types";
		case ZQKEY_INIT_DATA: return "Init Data";
		case ZQKEY_INTEG_CHECK_ALL: return "Integ. Check (All)";
		case ZQKEY_INTEG_CHECK_SCREENS: return "Integ. Check (Screens)";
		case ZQKEY_INTEG_CHECK_WARPS: return "Integ. Check (Warps)";
		case ZQKEY_EDIT_ITEMS: return "Item Editor";
		case ZQKEY_PASTE_LAYERS: return "Paste Layers";
		case ZQKEY_PALETTES_LEVEL: return "Palettes - Levels";
		case ZQKEY_HERO_SPRITE: return "Hero Sprite";
		case ZQKEY_USED_COMBOS: return "List Combos Used";
		case ZQKEY_PALETTES_MAIN: return "Palettes - Main";
		case ZQKEY_DEFAULT_MAP_STYLES: return "Default Map Styles";
		case ZQKEY_MAP_STYLES: return "Map Styles";
		case ZQKEY_MIDIS: return "MIDIs";
		case ZQKEY_MISC_COLORS: return "Misc Colors";
		case ZQKEY_NEW: return "New";
		case ZQKEY_OPTIONS: return "Options";
		case ZQKEY_DEFAULT_PALETTES: return "Default Palettes";
		case ZQKEY_MAZE_PATH: return "Maze Path";
		case ZQKEY_PLAY_MUSIC: return "Play Music";
		case ZQKEY_APPLY_TEMPLATE_ALL: return "Apply Template to All";
		case ZQKEY_REVERT: return "Revert";
		case ZQKEY_PASTE_ROOMTYPE: return "Paste Room Type Data";
		case ZQKEY_PASTE_SCREEN_DATA: return "Paste Screen Data";
		case ZQKEY_PASTE_SECRET_COMBOS: return "Paste Secret Combos";
		case ZQKEY_SFX: return "SFX Data";
		case ZQKEY_SHOP_TYPES: return "Shop Types";
		case ZQKEY_PALETTES_SPRITES: return "Palettes - Sprites";
		case ZQKEY_DEFAULT_SPRITE_DATA: return "Default Sprite Data";
		case ZQKEY_STOP_TUNES: return "Stop Tunes";
		case ZQKEY_EDIT_SUBSCREENS: return "Subscreens";
		case ZQKEY_AMBIENT_MUSIC: return "Ambient Music";
		case ZQKEY_NES_DUNGEON_TEMPL: return "NES Dungeon Template";
		case ZQKEY_DEFAULT_TILES: return "Default Tiles";
		case ZQKEY_MCGUFFIN_PIECES: return "McGuffin Pieces";
		case ZQKEY_UNDERCOMBO: return "Under Combo";
		case ZQKEY_PASTE_UNDERCOMBO: return "Paste Undercombo";
		case ZQKEY_VIDEO_MODE: return "Video Mode";
		case ZQKEY_VIEW_PALETTE: return "View Palette";
		case ZQKEY_VIEW_PIC: return "View Pic";
		case ZQKEY_PASTE_WARP_RET: return "Paste Warp Return";
		case ZQKEY_WARP_RINGS: return "Warp Rings";
		case ZQKEY_PASTE_WARPS: return "Paste Warps";
		case ZQKEY_EDIT_SPRITE_DATA: return "Sprite Data";
		case ZQKEY_SHOW_CSETS: return "Toggle CSets";
		case ZQKEY_SHOW_TYPES: return "Toggle Types";
		case ZQKEY_DEFAULT_ITEMS: return "Default Items";
		case ZQKEY_EDIT_DROPSETS: return "Edit Dropsets";
		case ZQKEY_PASTE_PALETTE: return "Paste Palette";
		case ZQKEY_QUEST_RULES: return "Quest Options";
		case ZQKEY_REPORT_COMBO_LOC: return "Report: Combo Locations";
		case ZQKEY_REPORT_TYPE_LOC: return "Report: Combo Type Locs.";
		case ZQKEY_REPORT_ENEMY_LOC: return "Report: Enemy Locations";
		case ZQKEY_REPORT_ITEM_LOC: return "Report: Item Locations";
		case ZQKEY_REPORT_SCRIPT_LOC: return "Report: Script Locations";
		case ZQKEY_REPORT_LINKS_HERE: return "Report: What Links Here";
		case ZQKEY_CLEAR_QST_PATH: return "Clear Quest Filepath";
		case ZQKEY_BUGGY_NEXT: return "Find Buggy Next->";
		case ZQKEY_RULES_ZSCRIPT: return "ZScript Rules";
		case ZQKEY_RULES_COMPILER: return "Compiler";
		case ZQKEY_EDIT_SCREEN_SCRIPT: return "Screen Script";
		case ZQKEY_SCREEN_SCREENSHOT: return "Take Screen Snapshot";
		case ZQKEY_VIEW_L2_BG: return "View L2 as BG";
		case ZQKEY_VIEW_L3_BG: return "View L3 as BG";
		case ZQKEY_BOTTLE_TYPES: return "Bottle Types";
		case ZQKEY_BOTTLE_SHOP_TYPES: return "Bottle Shop Types";
		case ZQKEY_FIX_WATER_SOLID: return "Water Solidity Fix";
		case ZQKEY_FIX_EFFECT_SQUARE: return "Effect Square Fix";
		case ZQKEY_SEARCH_QRS: return "Quest Rules Search";
		case ZQKEY_COMPILE_QUICK: return "Quick Compile ZScript";
		case ZQKEY_RULESETS: return "Rulesets";
		case ZQKEY_RULETMPLS: return "Rule Templates";
		case ZQKEY_COMPILE_SMART: return "Smart Compile ZScript";
		case ZQKEY_DEBUG_CONSOLE: return "ZQ Debug Console";
		case ZQKEY_SHOW_HOTKEYS: return "Show Hotkeys";
		case ZQKEY_BIND_HOTKEYS: return "Rebind Hotkeys";
		case ZQKEY_SCREEN_NOTES: return "Screen Notes";
		case ZQKEY_BROWSE_SCREEN_NOTES: return "Browse Notes";
		case ZQKEY_STATUS_EFFECTS: return "Status Effects";
	}
	return "ZQ_NIL_KEY";
}

char const* get_hotkey_cfg_name(uint hkey)
{
	switch(hkey)
	{
		case ZQKEY_NULL_KEY: return "None";
		case ZQKEY_UNDO: return "ZQKEY_UNDO";
		case ZQKEY_REDO: return "ZQKEY_REDO";
		case ZQKEY_MINUS_FLAG: return "ZQKEY_MINUS_FLAG";
		case ZQKEY_PLUS_FLAG: return "ZQKEY_PLUS_FLAG";
		case ZQKEY_SAVE: return "ZQKEY_SAVE";
		case ZQKEY_SAVEAS: return "ZQKEY_SAVEAS";
		case ZQKEY_OPEN: return "ZQKEY_OPEN";
		case ZQKEY_SCREEN_PAL: return "ZQKEY_SCREEN_PAL";
		case ZQKEY_SECRET_COMBO: return "ZQKEY_SECRET_COMBO";
		case ZQKEY_DOORS: return "ZQKEY_DOORS";
		case ZQKEY_FFCS: return "ZQKEY_FFCS";
		case ZQKEY_FLAGS: return "ZQKEY_FLAGS";
		case ZQKEY_SCRDATA: return "ZQKEY_SCRDATA";
		case ZQKEY_TILEWARP: return "ZQKEY_TILEWARP";
		case ZQKEY_SIDEWARP: return "ZQKEY_SIDEWARP";
		case ZQKEY_LAYERS: return "ZQKEY_LAYERS";
		case ZQKEY_RESET_TRANSP: return "ZQKEY_RESET_TRANSP";
		case ZQKEY_COPY: return "ZQKEY_COPY";
		case ZQKEY_TOGGLE_DARK: return "ZQKEY_TOGGLE_DARK";
		case ZQKEY_ENEMIES: return "ZQKEY_ENEMIES";
		case ZQKEY_SHOW_FLAGS: return "ZQKEY_SHOW_FLAGS";
		case ZQKEY_SHOW_FFCS: return "ZQKEY_SHOW_FFCS";
		case ZQKEY_ITEMS: return "ZQKEY_ITEMS";
		case ZQKEY_COMBOS: return "ZQKEY_COMBOS";
		case ZQKEY_DARK_PREVIEW: return "ZQKEY_DARK_PREVIEW";
		case ZQKEY_SHOW_INFO: return "ZQKEY_SHOW_INFO";
		case ZQKEY_DRAWING_MODE: return "ZQKEY_DRAWING_MODE";
		case ZQKEY_GOTO_PAGE: return "ZQKEY_GOTO_PAGE";
		case ZQKEY_SHOW_CMB_CS_INFO: return "ZQKEY_SHOW_CMB_CS_INFO";
		case ZQKEY_STRINGS: return "ZQKEY_STRINGS";
		case ZQKEY_ROOM: return "ZQKEY_ROOM";
		case ZQKEY_TILES: return "ZQKEY_TILES";
		case ZQKEY_PASTE: return "ZQKEY_PASTE";
		case ZQKEY_PASTEALL: return "ZQKEY_PASTEALL";
		case ZQKEY_PASTETOALL: return "ZQKEY_PASTETOALL";
		case ZQKEY_PASTEALLTOALL: return "ZQKEY_PASTEALLTOALL";
		case ZQKEY_SHOW_SOLID: return "ZQKEY_SHOW_SOLID";
		case ZQKEY_PREV_MODE: return "ZQKEY_PREV_MODE";
		case ZQKEY_COMPILE_ZSCRIPT: return "ZQKEY_COMPILE_ZSCRIPT";
		case ZQKEY_SCREENSHOT: return "ZQKEY_SCREENSHOT";
		case ZQKEY_ABOUT: return "ZQKEY_ABOUT";
		case ZQKEY_MINUS_MAP: return "ZQKEY_MINUS_MAP";
		case ZQKEY_PLUS_MAP: return "ZQKEY_PLUS_MAP";
		case ZQKEY_MINUS_COLOR: return "ZQKEY_MINUS_COLOR";
		case ZQKEY_PLUS_COLOR: return "ZQKEY_PLUS_COLOR";
		case ZQKEY_MINUS_SCR_PAL: return "ZQKEY_MINUS_SCR_PAL";
		case ZQKEY_PLUS_SCR_PAL: return "ZQKEY_PLUS_SCR_PAL";
		case ZQKEY_MINUS_16_SCR_PAL: return "ZQKEY_MINUS_16_SCR_PAL";
		case ZQKEY_PLUS_16_SCR_PAL: return "ZQKEY_PLUS_16_SCR_PAL";
		case ZQKEY_SCREEN_ZOOM_IN: return "ZQKEY_SCREEN_ZOOM_IN";
		case ZQKEY_SCREEN_ZOOM_OUT: return "ZQKEY_SCREEN_ZOOM_OUT";
		case ZQKEY_GRID: return "ZQKEY_GRID";
		case ZQKEY_GRID_COLOR: return "ZQKEY_GRID_COLOR";
		case ZQKEY_CHANGE_ALIAS_ORIGIN: return "ZQKEY_CHANGE_ALIAS_ORIGIN";
		case ZQKEY_DELETE: return "ZQKEY_DELETE";
		case ZQKEY_FULLSCREEN: return "ZQKEY_FULLSCREEN";
		case ZQKEY_LYR_0: return "ZQKEY_LYR_0";
		case ZQKEY_LYR_1: return "ZQKEY_LYR_1";
		case ZQKEY_LYR_2: return "ZQKEY_LYR_2";
		case ZQKEY_LYR_3: return "ZQKEY_LYR_3";
		case ZQKEY_LYR_4: return "ZQKEY_LYR_4";
		case ZQKEY_LYR_5: return "ZQKEY_LYR_5";
		case ZQKEY_LYR_6: return "ZQKEY_LYR_6";
		case ZQKEY_SCR_LPAL_0: return "ZQKEY_SCR_LPAL_0";
		case ZQKEY_SCR_LPAL_1: return "ZQKEY_SCR_LPAL_1";
		case ZQKEY_SCR_LPAL_2: return "ZQKEY_SCR_LPAL_2";
		case ZQKEY_SCR_LPAL_3: return "ZQKEY_SCR_LPAL_3";
		case ZQKEY_SCR_LPAL_4: return "ZQKEY_SCR_LPAL_4";
		case ZQKEY_SCR_LPAL_5: return "ZQKEY_SCR_LPAL_5";
		case ZQKEY_SCR_LPAL_6: return "ZQKEY_SCR_LPAL_6";
		case ZQKEY_SCR_LPAL_7: return "ZQKEY_SCR_LPAL_7";
		case ZQKEY_SCR_LPAL_8: return "ZQKEY_SCR_LPAL_8";
		case ZQKEY_SCR_LPAL_9: return "ZQKEY_SCR_LPAL_9";
		case ZQKEY_SCR_LPAL_10: return "ZQKEY_SCR_LPAL_10";
		case ZQKEY_SCR_LPAL_11: return "ZQKEY_SCR_LPAL_11";
		case ZQKEY_SCR_LPAL_12: return "ZQKEY_SCR_LPAL_12";
		case ZQKEY_SCR_LPAL_13: return "ZQKEY_SCR_LPAL_13";
		case ZQKEY_SCR_LPAL_14: return "ZQKEY_SCR_LPAL_14";
		case ZQKEY_SCR_LPAL_15: return "ZQKEY_SCR_LPAL_15";
		case ZQKEY_SCROLL_SCREEN_UP: return "ZQKEY_SCROLL_SCREEN_UP";
		case ZQKEY_SCROLL_SCREEN_DOWN: return "ZQKEY_SCROLL_SCREEN_DOWN";
		case ZQKEY_SCROLL_SCREEN_LEFT: return "ZQKEY_SCROLL_SCREEN_LEFT";
		case ZQKEY_SCROLL_SCREEN_RIGHT: return "ZQKEY_SCROLL_SCREEN_RIGHT";
		case ZQKEY_WARP_SCREEN_UP: return "ZQKEY_WARP_SCREEN_UP";
		case ZQKEY_WARP_SCREEN_DOWN: return "ZQKEY_WARP_SCREEN_DOWN";
		case ZQKEY_WARP_SCREEN_LEFT: return "ZQKEY_WARP_SCREEN_LEFT";
		case ZQKEY_WARP_SCREEN_RIGHT: return "ZQKEY_WARP_SCREEN_RIGHT";
		case ZQKEY_SCROLL_COMBO_UP: return "ZQKEY_SCROLL_COMBO_UP";
		case ZQKEY_SCROLL_COMBO_DOWN: return "ZQKEY_SCROLL_COMBO_DOWN";
		case ZQKEY_SCROLL_COMBO_LEFT: return "ZQKEY_SCROLL_COMBO_LEFT";
		case ZQKEY_SCROLL_COMBO_RIGHT: return "ZQKEY_SCROLL_COMBO_RIGHT";
		case ZQKEY_COMBO_PAGEUP: return "ZQKEY_COMBO_PAGEUP";
		case ZQKEY_COMBO_PAGEDN: return "ZQKEY_COMBO_PAGEDN";
		case ZQKEY_SQUAREPANEL_UP: return "ZQKEY_SQUAREPANEL_UP";
		case ZQKEY_SQUAREPANEL_DOWN: return "ZQKEY_SQUAREPANEL_DOWN";
		case ZQKEY_TESTMODE: return "ZQKEY_TESTMODE";
		case ZQKEY_CAUTO_HEIGHTMINUS: return "ZQKEY_CAUTO_HEIGHTMINUS";
		case ZQKEY_CAUTO_HEIGHTPLUS: return "ZQKEY_CAUTO_HEIGHTPLUS";
		case ZQKEY_CURR_LAYER_HL: return "ZQKEY_CURR_LAYER_HL";
		case ZQKEY_VIEW_MAP: return "ZQKEY_VIEW_MAP";
		case ZQKEY_DRAWMODE_NORMAL: return "ZQKEY_DRAWMODE_NORMAL";
		case ZQKEY_DRAWMODE_ALIAS: return "ZQKEY_DRAWMODE_ALIAS";
		case ZQKEY_DRAWMODE_POOL: return "ZQKEY_DRAWMODE_POOL";
		case ZQKEY_DRAWMODE_AUTO: return "ZQKEY_DRAWMODE_AUTO";
		case ZQKEY_IMPORT_COMBOS: return "ZQKEY_IMPORT_COMBOS";
		case ZQKEY_EXPORT_COMBOS: return "ZQKEY_EXPORT_COMBOS";
		case ZQKEY_IMPORT_DMAPS: return "ZQKEY_IMPORT_DMAPS";
		case ZQKEY_EXPORT_DMAPS: return "ZQKEY_EXPORT_DMAPS";
		case ZQKEY_IMPORT_ZGP: return "ZQKEY_IMPORT_ZGP";
		case ZQKEY_EXPORT_ZGP: return "ZQKEY_EXPORT_ZGP";
		case ZQKEY_IMPORT_MAP: return "ZQKEY_IMPORT_MAP";
		case ZQKEY_EXPORT_MAP: return "ZQKEY_EXPORT_MAP";
		case ZQKEY_IMPORT_PALS: return "ZQKEY_IMPORT_PALS";
		case ZQKEY_EXPORT_PALS: return "ZQKEY_EXPORT_PALS";
		case ZQKEY_IMPORT_STRINGS: return "ZQKEY_IMPORT_STRINGS";
		case ZQKEY_EXPORT_STRINGS: return "ZQKEY_EXPORT_STRINGS";
		case ZQKEY_IMPORT_TILES: return "ZQKEY_IMPORT_TILES";
		case ZQKEY_EXPORT_TILES: return "ZQKEY_EXPORT_TILES";
		case ZQKEY_CHANGE_TRACK: return "ZQKEY_CHANGE_TRACK";
		case ZQKEY_CHEATS: return "ZQKEY_CHEATS";
		case ZQKEY_CSET_FIX: return "ZQKEY_CSET_FIX";
		case ZQKEY_ALIASES: return "ZQKEY_ALIASES";
		case ZQKEY_CMBPOOLS: return "ZQKEY_CMBPOOLS";
		case ZQKEY_AUTOCMB: return "ZQKEY_AUTOCMB";
		case ZQKEY_DEFAULT_COMBOS: return "ZQKEY_DEFAULT_COMBOS";
		case ZQKEY_DELETE_MAP: return "ZQKEY_DELETE_MAP";
		case ZQKEY_DMAPS: return "ZQKEY_DMAPS";
		case ZQKEY_DOOR_COMBO_SETS: return "ZQKEY_DOOR_COMBO_SETS";
		case ZQKEY_PASTE_DOORS: return "ZQKEY_PASTE_DOORS";
		case ZQKEY_ENDSTRING: return "ZQKEY_ENDSTRING";
		case ZQKEY_EDIT_ENEMIES: return "ZQKEY_EDIT_ENEMIES";
		case ZQKEY_DEFAULT_ENEMIES: return "ZQKEY_DEFAULT_ENEMIES";
		case ZQKEY_PASTE_ENEMIES: return "ZQKEY_PASTE_ENEMIES";
		case ZQKEY_EXIT: return "ZQKEY_EXIT";
		case ZQKEY_PASTE_FFCS: return "ZQKEY_PASTE_FFCS";
		case ZQKEY_GAME_ICONS: return "ZQKEY_GAME_ICONS";
		case ZQKEY_GOTO_MAP: return "ZQKEY_GOTO_MAP";
		case ZQKEY_PASTE_GUY_STR: return "ZQKEY_PASTE_GUY_STR";
		case ZQKEY_HEADER: return "ZQKEY_HEADER";
		case ZQKEY_INFO_TYPES: return "ZQKEY_INFO_TYPES";
		case ZQKEY_INIT_DATA: return "ZQKEY_INIT_DATA";
		case ZQKEY_INTEG_CHECK_ALL: return "ZQKEY_INTEG_CHECK_ALL";
		case ZQKEY_INTEG_CHECK_SCREENS: return "ZQKEY_INTEG_CHECK_SCREENS";
		case ZQKEY_INTEG_CHECK_WARPS: return "ZQKEY_INTEG_CHECK_WARPS";
		case ZQKEY_EDIT_ITEMS: return "ZQKEY_EDIT_ITEMS";
		case ZQKEY_PASTE_LAYERS: return "ZQKEY_PASTE_LAYERS";
		case ZQKEY_PALETTES_LEVEL: return "ZQKEY_PALETTES_LEVEL";
		case ZQKEY_HERO_SPRITE: return "ZQKEY_PLAYER_SPRITE";
		case ZQKEY_USED_COMBOS: return "ZQKEY_USED_COMBOS";
		case ZQKEY_PALETTES_MAIN: return "ZQKEY_PALETTES_MAIN";
		case ZQKEY_DEFAULT_MAP_STYLES: return "ZQKEY_DEFAULT_MAP_STYLES";
		case ZQKEY_MAP_STYLES: return "ZQKEY_MAP_STYLES";
		case ZQKEY_MIDIS: return "ZQKEY_MIDIS";
		case ZQKEY_MISC_COLORS: return "ZQKEY_MISC_COLORS";
		case ZQKEY_NEW: return "ZQKEY_NEW";
		case ZQKEY_OPTIONS: return "ZQKEY_OPTIONS";
		case ZQKEY_DEFAULT_PALETTES: return "ZQKEY_DEFAULT_PALETTES";
		case ZQKEY_MAZE_PATH: return "ZQKEY_MAZE_PATH";
		case ZQKEY_PLAY_MUSIC: return "ZQKEY_PLAY_MUSIC";
		case ZQKEY_APPLY_TEMPLATE_ALL: return "ZQKEY_APPLY_TEMPLATE_ALL";
		case ZQKEY_REVERT: return "ZQKEY_REVERT";
		case ZQKEY_PASTE_ROOMTYPE: return "ZQKEY_PASTE_ROOMTYPE";
		case ZQKEY_PASTE_SCREEN_DATA: return "ZQKEY_PASTE_SCREEN_DATA";
		case ZQKEY_PASTE_SECRET_COMBOS: return "ZQKEY_PASTE_SECRET_COMBOS";
		case ZQKEY_SFX: return "ZQKEY_SFX";
		case ZQKEY_SHOP_TYPES: return "ZQKEY_SHOP_TYPES";
		case ZQKEY_PALETTES_SPRITES: return "ZQKEY_PALETTES_SPRITES";
		case ZQKEY_DEFAULT_SPRITE_DATA: return "ZQKEY_DEFAULT_SPRITE_DATA";
		case ZQKEY_STOP_TUNES: return "ZQKEY_STOP_TUNES";
		case ZQKEY_EDIT_SUBSCREENS: return "ZQKEY_EDIT_SUBSCREENS";
		case ZQKEY_AMBIENT_MUSIC: return "ZQKEY_AMBIENT_MUSIC";
		case ZQKEY_NES_DUNGEON_TEMPL: return "ZQKEY_NES_DUNGEON_TEMPL";
		case ZQKEY_DEFAULT_TILES: return "ZQKEY_DEFAULT_TILES";
		case ZQKEY_MCGUFFIN_PIECES: return "ZQKEY_MCGUFFIN_PIECES";
		case ZQKEY_UNDERCOMBO: return "ZQKEY_UNDERCOMBO";
		case ZQKEY_PASTE_UNDERCOMBO: return "ZQKEY_PASTE_UNDERCOMBO";
		case ZQKEY_VIDEO_MODE: return "ZQKEY_VIDEO_MODE";
		case ZQKEY_VIEW_PALETTE: return "ZQKEY_VIEW_PALETTE";
		case ZQKEY_VIEW_PIC: return "ZQKEY_VIEW_PIC";
		case ZQKEY_PASTE_WARP_RET: return "ZQKEY_PASTE_WARP_RET";
		case ZQKEY_WARP_RINGS: return "ZQKEY_WARP_RINGS";
		case ZQKEY_PASTE_WARPS: return "ZQKEY_PASTE_WARPS";
		case ZQKEY_EDIT_SPRITE_DATA: return "ZQKEY_EDIT_SPRITE_DATA";
		case ZQKEY_SHOW_CSETS: return "ZQKEY_SHOW_CSETS";
		case ZQKEY_SHOW_TYPES: return "ZQKEY_SHOW_TYPES";
		case ZQKEY_DEFAULT_ITEMS: return "ZQKEY_DEFAULT_ITEMS";
		case ZQKEY_EDIT_DROPSETS: return "ZQKEY_EDIT_DROPSETS";
		case ZQKEY_PASTE_PALETTE: return "ZQKEY_PASTE_PALETTE";
		case ZQKEY_QUEST_RULES: return "ZQKEY_QUEST_RULES";
		case ZQKEY_REPORT_COMBO_LOC: return "ZQKEY_REPORT_COMBO_LOC";
		case ZQKEY_REPORT_TYPE_LOC: return "ZQKEY_REPORT_TYPE_LOC";
		case ZQKEY_REPORT_ENEMY_LOC: return "ZQKEY_REPORT_ENEMY_LOC";
		case ZQKEY_REPORT_ITEM_LOC: return "ZQKEY_REPORT_ITEM_LOC";
		case ZQKEY_REPORT_SCRIPT_LOC: return "ZQKEY_REPORT_SCRIPT_LOC";
		case ZQKEY_REPORT_LINKS_HERE: return "ZQKEY_REPORT_LINKS_HERE";
		case ZQKEY_CLEAR_QST_PATH: return "ZQKEY_CLEAR_QST_PATH";
		case ZQKEY_BUGGY_NEXT: return "ZQKEY_BUGGY_NEXT";
		case ZQKEY_RULES_ZSCRIPT: return "ZQKEY_RULES_ZSCRIPT";
		case ZQKEY_RULES_COMPILER: return "ZQKEY_RULES_COMPILER";
		case ZQKEY_EDIT_SCREEN_SCRIPT: return "ZQKEY_EDIT_SCREEN_SCRIPT";
		case ZQKEY_SCREEN_SCREENSHOT: return "ZQKEY_SCREEN_SCREENSHOT";
		case ZQKEY_VIEW_L2_BG: return "ZQKEY_VIEW_L2_BG";
		case ZQKEY_VIEW_L3_BG: return "ZQKEY_VIEW_L3_BG";
		case ZQKEY_BOTTLE_TYPES: return "ZQKEY_BOTTLE_TYPES";
		case ZQKEY_BOTTLE_SHOP_TYPES: return "ZQKEY_BOTTLE_SHOP_TYPES";
		case ZQKEY_FIX_WATER_SOLID: return "ZQKEY_FIX_WATER_SOLID";
		case ZQKEY_FIX_EFFECT_SQUARE: return "ZQKEY_FIX_EFFECT_SQUARE";
		case ZQKEY_SEARCH_QRS: return "ZQKEY_SEARCH_QRS";
		case ZQKEY_COMPILE_QUICK: return "ZQKEY_COMPILE_QUICK";
		case ZQKEY_RULESETS: return "ZQKEY_RULESETS";
		case ZQKEY_RULETMPLS: return "ZQKEY_RULETMPLS";
		case ZQKEY_COMPILE_SMART: return "ZQKEY_COMPILE_SMART";
		case ZQKEY_DEBUG_CONSOLE: return "ZQKEY_DEBUG_CONSOLE";
		case ZQKEY_SHOW_HOTKEYS: return "ZQKEY_SHOW_HOTKEYS";
		case ZQKEY_BIND_HOTKEYS: return "ZQKEY_BIND_HOTKEYS";
		case ZQKEY_SCREEN_NOTES: return "ZQKEY_SCREEN_NOTES";
		case ZQKEY_BROWSE_SCREEN_NOTES: return "ZQKEY_BROWSE_SCREEN_NOTES";
		case ZQKEY_STATUS_EFFECTS: return "ZQKEY_STATUS_EFFECTS";
	}
	return "ZQ_NIL_KEY";
}

char const* get_hotkey_helptext(uint hkey)
{
	switch(hkey)
	{
		case ZQKEY_NULL_KEY:
			break;
		case ZQKEY_UNDO:
			return "Undo the last edit";
		case ZQKEY_REDO:
			return "Redo the last undone change";
		case ZQKEY_MINUS_FLAG:
			break;
		case ZQKEY_PLUS_FLAG:
			break;
		case ZQKEY_SAVE:
			return "Save the quest";
		case ZQKEY_SAVEAS:
			return "Save the quest as a new filename";
		case ZQKEY_OPEN:
			return "Open a .qst file";
		case ZQKEY_SCREEN_PAL:
			return "Change the current screen's Palette, as previewed in ZQ";
		case ZQKEY_SECRET_COMBO:
			return "Edit the current screen's Secret Combos";
		case ZQKEY_DOORS:
			return "Edit the screen's NES Doors";
		case ZQKEY_FFCS:
			return "Edit this screen's FFCs";
		case ZQKEY_FLAGS:
			return "Place mapflags";
		case ZQKEY_SCRDATA:
			return "Edit the current screen's Screen Data";
		case ZQKEY_TILEWARP:
			return "Edit the current screen's Tile Warps";
		case ZQKEY_SIDEWARP:
			return "Edit the current screen's Side Warps";
		case ZQKEY_LAYERS:
			return "Edit the screen's Layers";
		case ZQKEY_RESET_TRANSP:
			break;
		case ZQKEY_COPY:
			return "Copies the current screen";
		case ZQKEY_TOGGLE_DARK:
			break;
		case ZQKEY_ENEMIES:
			return "Set the screen's Enemies";
		case ZQKEY_SHOW_FLAGS:
			return "Show the mapflags placed on screen";
		case ZQKEY_SHOW_FFCS:
			return "Outline the FFCs placed on screen";
		case ZQKEY_ITEMS:
			return "Set the screen item";
		case ZQKEY_COMBOS:
			return "Opens the combo pages";
		case ZQKEY_DARK_PREVIEW:
			return "Toggle previewing dark rooms";
		case ZQKEY_SHOW_INFO:
			break;
		case ZQKEY_DRAWING_MODE:
			break;
		case ZQKEY_GOTO_PAGE:
			break;
		case ZQKEY_SHOW_CMB_CS_INFO:
			break;
		case ZQKEY_STRINGS:
			return "Open the String Editor";
		case ZQKEY_ROOM:
			return "Edit the screen's Room Data";
		case ZQKEY_TILES:
			return "Opens the Tile Pages";
		case ZQKEY_PASTE:
			return "Paste most of the copied screen";
		case ZQKEY_PASTEALL:
			return "Paste everything from the copied screen";
		case ZQKEY_PASTETOALL:
			return "Paste most of the copied screen, to every screen on this map";
		case ZQKEY_PASTEALLTOALL:
			return "Paste everything from the copied screen, to every screen on this map";
		case ZQKEY_SHOW_SOLID:
			return "Draw solidity masks on the screen";
		case ZQKEY_PREV_MODE:
			return "Enable Preview Mode";
		case ZQKEY_COMPILE_ZSCRIPT:
			return "Opens the ZScript Compile dialog";
		case ZQKEY_SCREENSHOT:
			return "Take a screenshot";
		case ZQKEY_ABOUT:
			return "Shows the program information";
		case ZQKEY_MINUS_MAP:
			break;
		case ZQKEY_PLUS_MAP:
			break;
		case ZQKEY_MINUS_COLOR:
			break;
		case ZQKEY_PLUS_COLOR:
			break;
		case ZQKEY_MINUS_SCR_PAL:
			break;
		case ZQKEY_PLUS_SCR_PAL:
			break;
		case ZQKEY_MINUS_16_SCR_PAL:
			break;
		case ZQKEY_PLUS_16_SCR_PAL:
			break;
		case ZQKEY_SCREEN_ZOOM_IN:
			break;
		case ZQKEY_SCREEN_ZOOM_OUT:
			break;
		case ZQKEY_GRID:
			return "Toggle the Grid Lines over the screen area";
		case ZQKEY_GRID_COLOR:
			break;
		case ZQKEY_CHANGE_ALIAS_ORIGIN:
			break;
		case ZQKEY_DELETE:
			return "Clear the current Screen";
		case ZQKEY_FULLSCREEN:
			return "Toggle Fullscreen";
		case ZQKEY_LYR_0:
			break;
		case ZQKEY_LYR_1:
			break;
		case ZQKEY_LYR_2:
			break;
		case ZQKEY_LYR_3:
			break;
		case ZQKEY_LYR_4:
			break;
		case ZQKEY_LYR_5:
			break;
		case ZQKEY_LYR_6:
			break;
		case ZQKEY_SCR_LPAL_0:
			break;
		case ZQKEY_SCR_LPAL_1:
			break;
		case ZQKEY_SCR_LPAL_2:
			break;
		case ZQKEY_SCR_LPAL_3:
			break;
		case ZQKEY_SCR_LPAL_4:
			break;
		case ZQKEY_SCR_LPAL_5:
			break;
		case ZQKEY_SCR_LPAL_6:
			break;
		case ZQKEY_SCR_LPAL_7:
			break;
		case ZQKEY_SCR_LPAL_8:
			break;
		case ZQKEY_SCR_LPAL_9:
			break;
		case ZQKEY_SCR_LPAL_10:
			break;
		case ZQKEY_SCR_LPAL_11:
			break;
		case ZQKEY_SCR_LPAL_12:
			break;
		case ZQKEY_SCR_LPAL_13:
			break;
		case ZQKEY_SCR_LPAL_14:
			break;
		case ZQKEY_SCR_LPAL_15:
			break;
		case ZQKEY_SCROLL_SCREEN_UP:
			break;
		case ZQKEY_SCROLL_SCREEN_DOWN:
			break;
		case ZQKEY_SCROLL_SCREEN_LEFT:
			break;
		case ZQKEY_SCROLL_SCREEN_RIGHT:
			break;
		case ZQKEY_WARP_SCREEN_UP:
			break;
		case ZQKEY_WARP_SCREEN_DOWN:
			break;
		case ZQKEY_WARP_SCREEN_LEFT:
			break;
		case ZQKEY_WARP_SCREEN_RIGHT:
			break;
		case ZQKEY_SCROLL_COMBO_UP:
			break;
		case ZQKEY_SCROLL_COMBO_DOWN:
			break;
		case ZQKEY_SCROLL_COMBO_LEFT:
			break;
		case ZQKEY_SCROLL_COMBO_RIGHT:
			break;
		case ZQKEY_COMBO_PAGEUP:
			break;
		case ZQKEY_COMBO_PAGEDN:
			break;
		case ZQKEY_SQUAREPANEL_UP:
			break;
		case ZQKEY_SQUAREPANEL_DOWN:
			break;
		case ZQKEY_TESTMODE:
			return "Test your quest";
		case ZQKEY_CAUTO_HEIGHTPLUS:
			break;
		case ZQKEY_CAUTO_HEIGHTMINUS:
			break;
		case ZQKEY_CURR_LAYER_HL:
			return "Highlight the current layer by dithering other layers";
		case ZQKEY_VIEW_MAP:
			return "View the full map";
		case ZQKEY_DRAWMODE_NORMAL:
			return "Switches back to normal drawing mode from any other mode";
		case ZQKEY_DRAWMODE_ALIAS:
			return "Switches to alias drawing mode (or back to normal if already in alias mode)";
		case ZQKEY_DRAWMODE_POOL:
			return "Switches to pool drawing mode (or back to normal if already in pool mode)";
		case ZQKEY_DRAWMODE_AUTO:
			return "Switches to autocombo drawing mode (or back to normal if already in autocombo mode)";
		
		case ZQKEY_IMPORT_COMBOS:
		case ZQKEY_EXPORT_COMBOS:
		case ZQKEY_IMPORT_DMAPS:
		case ZQKEY_EXPORT_DMAPS:
		case ZQKEY_IMPORT_ZGP:
		case ZQKEY_EXPORT_ZGP:
		case ZQKEY_IMPORT_MAP:
		case ZQKEY_EXPORT_MAP:
		case ZQKEY_IMPORT_PALS:
		case ZQKEY_EXPORT_PALS:
		case ZQKEY_IMPORT_STRINGS:
		case ZQKEY_EXPORT_STRINGS:
		case ZQKEY_IMPORT_TILES:
		case ZQKEY_EXPORT_TILES:
			break; //no info
		case ZQKEY_CHANGE_TRACK:
			return "Change the track of the currently playing music";
		case ZQKEY_CHEATS:
			return "Allows changing the quest's Cheat Codes";
		case ZQKEY_CSET_FIX:
			return "Opens the CSet Fix tool";
		case ZQKEY_ALIASES:
			return "Opens the Combo Alias Pages";
		case ZQKEY_CMBPOOLS:
			return "Opens the Combo Pool Pages";;
		case ZQKEY_AUTOCMB:
			return "Opens the Auto Combo Pages";
		case ZQKEY_DEFAULT_COMBOS:
			return "Reset the quest's Combos to default";
		case ZQKEY_DELETE_MAP:
			return "Clear the current Map";
		case ZQKEY_DMAPS:
			return "Open the DMap Editor";
		case ZQKEY_DOOR_COMBO_SETS:
			return "Edit the quest's NES Door Combo Sets";
		case ZQKEY_PASTE_DOORS:
			return "Paste the NES Doors from the copied screen";
		case ZQKEY_ENDSTRING:
			return "Allows selecting the game win string";
		case ZQKEY_EDIT_ENEMIES:
			return "Open the Enemy Editor";
		case ZQKEY_DEFAULT_ENEMIES:
			return "Reset Enemy Editor data to the default quest's data";
		case ZQKEY_PASTE_ENEMIES:
			return "Paste the Enemies from the copied screen";
		case ZQKEY_EXIT:
			return "Exit ZQuest";
		case ZQKEY_PASTE_FFCS:
			return "Paste FFCs from the copied screen";
		case ZQKEY_GAME_ICONS:
			return "Edit the quest's Game Icons";
		case ZQKEY_GOTO_MAP:
			return "Go to a specified Map";
		case ZQKEY_PASTE_GUY_STR:
			return "Paste the room's Guy and String from the copied screen";
		case ZQKEY_HEADER:
			return "Edit the quest's Header information";
		case ZQKEY_INFO_TYPES:
			return "Edit Info Shop info";
		case ZQKEY_INIT_DATA:
			return "Edit quest Init Data";
		case ZQKEY_INTEG_CHECK_ALL:
			return "Check for misc possible quest issues";
		case ZQKEY_INTEG_CHECK_SCREENS:
			return "Check for misc possible quest issues, relating to rooms";
		case ZQKEY_INTEG_CHECK_WARPS:
			return "Check for misc possible quest issues, relating to warps";
		case ZQKEY_EDIT_ITEMS:
			return "Open the Item Editor";
		case ZQKEY_PASTE_LAYERS:
			return "Paste the Layers from the copied screen";
		case ZQKEY_PALETTES_LEVEL:
			return "Open the Level Palettes";
		case ZQKEY_HERO_SPRITE:
			return "Edit the Hero's sprites";
		case ZQKEY_USED_COMBOS:
			return "List the combos used on the current screen";
		case ZQKEY_PALETTES_MAIN:
			return "Open the Main Palette";
		case ZQKEY_DEFAULT_MAP_STYLES:
			return "Reset the Map Styles to default";
		case ZQKEY_MAP_STYLES:
			return "Edit the Map Styles";
		case ZQKEY_MIDIS:
			return "Edit Subscreens";
		case ZQKEY_MISC_COLORS:
			return "Edit MIDIs";
		case ZQKEY_NEW:
			return "Edit the quest's Misc Colors";
		case ZQKEY_OPTIONS:
			return "Create a new .qst";
		case ZQKEY_DEFAULT_PALETTES:
			return "Open ZQ's Options dialog";
		case ZQKEY_MAZE_PATH:
			return "Default the quest's Palettes";
		case ZQKEY_PLAY_MUSIC:
			return "Edit the screen's Maze Path";
		case ZQKEY_APPLY_TEMPLATE_ALL:
			return "Play a music file in ZQ while you edit";
		case ZQKEY_REVERT:
			return "Reverts changes made to the quest file, re-loading its last save";
		case ZQKEY_PASTE_ROOMTYPE:
			return "Paste the Room Data from the copied screen";
		case ZQKEY_PASTE_SCREEN_DATA:
			return "Paste the Screen Data from the copied screen";
		case ZQKEY_PASTE_SECRET_COMBOS:
			return "Paste the Secret Combos from the copied screen";
		case ZQKEY_SFX:
			return "Edit the quest's SFX Data";
		case ZQKEY_SHOP_TYPES:
			return "Edit the Shop Types";
		case ZQKEY_PALETTES_SPRITES:
			return "Open the Sprite Palettes";
		case ZQKEY_DEFAULT_SPRITE_DATA:
			return "Default the quest's Sprite Data";
		case ZQKEY_STOP_TUNES:
			return "Stop playing music";
		case ZQKEY_EDIT_SUBSCREENS:
			return "Edit the quest's Subscreens";
		case ZQKEY_AMBIENT_MUSIC:
			return "Play built-in ambient music";
		case ZQKEY_NES_DUNGEON_TEMPL:
			return "Create the current screen from Screen 0x83";
		case ZQKEY_DEFAULT_TILES:
			return "Reset the quest's Tiles to default";
		case ZQKEY_MCGUFFIN_PIECES:
			return "Arrange the MCGuffins on your subscreen";
		case ZQKEY_UNDERCOMBO:
			return "Edit the screen's Undercombo";
		case ZQKEY_PASTE_UNDERCOMBO:
			return "Paste the Undercombo from the copied screen";
		case ZQKEY_VIDEO_MODE:
			return "Adjust the resolution of the ZQ window";
		case ZQKEY_VIEW_PALETTE:
			return "View the quest's Palette";
		case ZQKEY_VIEW_PIC:
			return "View image files";
		case ZQKEY_PASTE_WARP_RET:
			return "Paste warp return squares from the copied screen";
		case ZQKEY_WARP_RINGS:
			return "Edit the quest's Warp Rings";
		case ZQKEY_PASTE_WARPS:
			return "Paste the Tile and Side warps from the copied screen";
		case ZQKEY_EDIT_SPRITE_DATA:
			return "Edit Sprite Data";
		case ZQKEY_SHOW_CSETS:
			return "Show CSet numbers placed on the screen";
		case ZQKEY_SHOW_TYPES:
			return "Show combo type numbers placed on the screen";
		case ZQKEY_DEFAULT_ITEMS:
			return "Reset the quest's Items to default";
		case ZQKEY_EDIT_DROPSETS:
			return "Edit Dropsets";
		case ZQKEY_PASTE_PALETTE:
			return "Paste the ZQ Screen Palette from the copied screen";
		case ZQKEY_QUEST_RULES:
			return "Edit Quest Rules";
		case ZQKEY_REPORT_COMBO_LOC:
			return "List what locations the currently selected combo appears in";
		case ZQKEY_REPORT_TYPE_LOC:
			return "List what locations various combo types appear in";
		case ZQKEY_REPORT_ENEMY_LOC:
			return "List what screens enemies are placed on";
		case ZQKEY_REPORT_ITEM_LOC:
			return "List what items are placed on screens";
		case ZQKEY_REPORT_SCRIPT_LOC:
			return "List what FFC Scripts are placed on screens";
		case ZQKEY_REPORT_LINKS_HERE:
			return "List what other screens warp to the current screen, or use the current screen as a layer";
		case ZQKEY_CLEAR_QST_PATH:
			return "Reset the filepath that file browsers use";
		case ZQKEY_BUGGY_NEXT:
			return "Find instances of ->Next combos using buggy secrets behavior";
		case ZQKEY_RULES_ZSCRIPT:
			return "Open ZScript Quest Rules";
		case ZQKEY_RULES_COMPILER:
			return "Edit Compiler Settings";
		case ZQKEY_EDIT_SCREEN_SCRIPT:
			return "Edit the current screen's Script";
		case ZQKEY_SCREEN_SCREENSHOT:
			return "Take a screenshot of just the screen area of the screen";
		case ZQKEY_VIEW_L2_BG:
			return "Toggle viewing Layer 2 as background";
		case ZQKEY_VIEW_L3_BG:
			return "Toggle viewing Layer 3 as background";
		case ZQKEY_BOTTLE_TYPES:
			return "Edit the types of things that can be put in Bottles";
		case ZQKEY_BOTTLE_SHOP_TYPES:
			return "Edit the types of bottle shops";
		case ZQKEY_FIX_WATER_SOLID:
			return "Fix solid water";
		case ZQKEY_FIX_EFFECT_SQUARE:
			return "Fix effect squares";
		case ZQKEY_SEARCH_QRS:
			return "Search all Quest Rules";
		case ZQKEY_COMPILE_QUICK:
			return "Compile the ZScript buffer and skip slot assignment";
		case ZQKEY_RULESETS:
			return "Apply Rulesets (setting EVERY quest rule)";
		case ZQKEY_RULETMPLS:
			return "Apply Rule Templates (setting SOME quest rules)";
		case ZQKEY_COMPILE_SMART:
			return "Compile the ZScript buffer and smartly auto-assign slots";
		case ZQKEY_DEBUG_CONSOLE:
			return "Toggle the ZQuest Debug Console";
		case ZQKEY_SHOW_HOTKEYS:
			return "Show Hotkeys";
		case ZQKEY_BIND_HOTKEYS:
			return "Opens the hotkey binding menu";
		case ZQKEY_SCREEN_NOTES:
			return "Opens the notes for the current screen.";
		case ZQKEY_BROWSE_SCREEN_NOTES:
			return "Opens the notes browsing menu.";
		case ZQKEY_STATUS_EFFECTS:
			return "Opens the status effect data list.";
	}
	return "";
}

void show_hotkey_info(uint hkey)
{
	char const* txt = get_hotkey_helptext(hkey);
	if(!txt || !txt[0]) return;
	InfoDialog(get_hotkey_name(hkey), txt).show();
}

Hotkey zq_hotkeys[ZQKEY_MAX];
uint favorite_commands[MAXFAVORITECOMMANDS] = {ZQKEY_NULL_KEY};
bool fav_cmd_disabled[ZQKEY_MAX] = {false};

bool is_reserved_key(int c)
{
	switch(c)
	{
		case KEY_ESC: case KEY_F1:
			return true;
	}
	return false;
}
bool is_reserved_keycombo(int c, int modflag)
{
	if(c==KEY_F4 && (modflag&KB_ALT_FLAG))
		return true;
	return false;
}

void default_hotkeys()
{
	zq_hotkeys[ZQKEY_UNDO].setval(KEY_Z,KB_CTRL_CMD_FLAG, KEY_U,0);
	zq_hotkeys[ZQKEY_REDO].setval(KEY_Z,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG, KEY_Y,KB_CTRL_CMD_FLAG);
	zq_hotkeys[ZQKEY_MINUS_FLAG].setval(KEY_ASTERISK,0,KEY_OPENBRACE,0);
	zq_hotkeys[ZQKEY_PLUS_FLAG].setval(KEY_SLASH_PAD,0,KEY_CLOSEBRACE,0);
	zq_hotkeys[ZQKEY_SAVE].setval(KEY_F2,0,KEY_S,KB_CTRL_CMD_FLAG);
	zq_hotkeys[ZQKEY_SAVEAS].setval(0,0,0,0);
	zq_hotkeys[ZQKEY_OPEN].setval(KEY_F3,0,KEY_O,KB_CTRL_CMD_FLAG);
	zq_hotkeys[ZQKEY_SCREEN_PAL].setval(KEY_F4,0,0,0);
	zq_hotkeys[ZQKEY_SECRET_COMBO].setval(KEY_F5,0,0,0);
	zq_hotkeys[ZQKEY_DOORS].setval(KEY_F6,0,0,0);
	zq_hotkeys[ZQKEY_FFCS].setval(KEY_F7,0,0,0);
	zq_hotkeys[ZQKEY_FLAGS].setval(KEY_F8,0,0,0);
	zq_hotkeys[ZQKEY_SCRDATA].setval(KEY_F9,0,0,0);
	zq_hotkeys[ZQKEY_TILEWARP].setval(KEY_F10,0,0,0);
	zq_hotkeys[ZQKEY_SIDEWARP].setval(KEY_F11,0,0,0);
	zq_hotkeys[ZQKEY_LAYERS].setval(KEY_F12,0,0,0);
	zq_hotkeys[ZQKEY_RESET_TRANSP].setval(0,0,0,0);
	zq_hotkeys[ZQKEY_COPY].setval(KEY_C,0,0,0);
	zq_hotkeys[ZQKEY_TOGGLE_DARK].setval(KEY_D,0,0,0);
	zq_hotkeys[ZQKEY_ENEMIES].setval(KEY_E,0,0,0);
	zq_hotkeys[ZQKEY_SHOW_FLAGS].setval(KEY_F,0,0,0);
	zq_hotkeys[ZQKEY_SHOW_FFCS].setval(0,0,0,0);
	zq_hotkeys[ZQKEY_ITEMS].setval(KEY_I,0,0,0);
	zq_hotkeys[ZQKEY_COMBOS].setval(KEY_K,0,0,0);
	zq_hotkeys[ZQKEY_DARK_PREVIEW].setval(KEY_L,0,0,0);
	zq_hotkeys[ZQKEY_SHOW_INFO].setval(KEY_N,0,0,0);
	zq_hotkeys[ZQKEY_DRAWING_MODE].setval(KEY_O,0,0,0);
	zq_hotkeys[ZQKEY_GOTO_PAGE].setval(KEY_P,0,0,0);
	zq_hotkeys[ZQKEY_SHOW_CMB_CS_INFO].setval(KEY_Q,0,0,0);
	zq_hotkeys[ZQKEY_ROOM].setval(KEY_R,0,0,0);
	zq_hotkeys[ZQKEY_STRINGS].setval(KEY_S,0,0,0);
	zq_hotkeys[ZQKEY_TILES].setval(KEY_T,0,0,0);
	zq_hotkeys[ZQKEY_PASTE].setval(KEY_V,0,0,0);
	zq_hotkeys[ZQKEY_PASTEALL].setval(KEY_V,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_PASTETOALL].setval(KEY_V,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_PASTEALLTOALL].setval(KEY_V,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_SHOW_SOLID].setval(KEY_W,0,0,0);
	zq_hotkeys[ZQKEY_PREV_MODE].setval(KEY_X,0,0,0);
	zq_hotkeys[ZQKEY_COMPILE_ZSCRIPT].setval(KEY_Y,0,0,0);
	zq_hotkeys[ZQKEY_SCREENSHOT].setval(KEY_Z,0,0,0);
	zq_hotkeys[ZQKEY_ABOUT].setval(KEY_SLASH,0,0,0);
	zq_hotkeys[ZQKEY_MINUS_MAP].setval(KEY_COMMA,0,0,0);
	zq_hotkeys[ZQKEY_PLUS_MAP].setval(KEY_STOP,0,0,0);
	zq_hotkeys[ZQKEY_MINUS_COLOR].setval(KEY_MINUS,0,KEY_MINUS_PAD,0);
	zq_hotkeys[ZQKEY_PLUS_COLOR].setval(KEY_EQUALS,0,KEY_PLUS_PAD,0);
	zq_hotkeys[ZQKEY_MINUS_SCR_PAL].setval(KEY_MINUS,KB_SHIFT_FLAG,KEY_MINUS_PAD,KB_SHIFT_FLAG);
	zq_hotkeys[ZQKEY_PLUS_SCR_PAL].setval(KEY_EQUALS,KB_SHIFT_FLAG,KEY_PLUS_PAD,KB_SHIFT_FLAG);
	zq_hotkeys[ZQKEY_MINUS_16_SCR_PAL].setval(KEY_MINUS,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG,KEY_MINUS_PAD,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG);
	zq_hotkeys[ZQKEY_PLUS_16_SCR_PAL].setval(KEY_EQUALS,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG,KEY_PLUS_PAD,KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG);
	zq_hotkeys[ZQKEY_SCREEN_ZOOM_IN].setval(KEY_EQUALS,KB_CTRL_CMD_FLAG,KEY_PLUS_PAD,KB_CTRL_CMD_FLAG);
	zq_hotkeys[ZQKEY_SCREEN_ZOOM_OUT].setval(KEY_MINUS,KB_CTRL_CMD_FLAG,KEY_MINUS_PAD,KB_CTRL_CMD_FLAG);
	zq_hotkeys[ZQKEY_GRID].setval(KEY_TILDE,0,0,0);
	zq_hotkeys[ZQKEY_GRID_COLOR].setval(KEY_TILDE,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_CHANGE_ALIAS_ORIGIN].setval(KEY_SPACE,0,0,0);
	zq_hotkeys[ZQKEY_DELETE].setval(KEY_DEL,0,KEY_DEL_PAD,0);
	zq_hotkeys[ZQKEY_FULLSCREEN].setval(KEY_ENTER,KB_ALT_FLAG,KEY_ENTER_PAD,KB_ALT_FLAG);
	for(int q = 0; q <= 6; ++q) //0-6, +pad
		zq_hotkeys[ZQKEY_LYR_0+q].setval(KEY_0+q,0,KEY_0_PAD+q,0);
	for(int q = 0; q <= 15; ++q)
	{
		if(q < 10) //Ctrl+(0-9 + pad), Ctrl+Shift+(0-5 + pad)
			zq_hotkeys[ZQKEY_SCR_LPAL_0+q].setval(KEY_0+q,KB_CTRL_CMD_FLAG,KEY_0_PAD+q,KB_CTRL_CMD_FLAG);
		else zq_hotkeys[ZQKEY_SCR_LPAL_0+q].setval(KEY_0+(q-10),KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG,
			KEY_0_PAD+(q-10),KB_CTRL_CMD_FLAG|KB_SHIFT_FLAG);
	}
	zq_hotkeys[ZQKEY_SCROLL_SCREEN_UP].setval(KEY_UP,0,0,0);
	zq_hotkeys[ZQKEY_SCROLL_SCREEN_DOWN].setval(KEY_DOWN,0,0,0);
	zq_hotkeys[ZQKEY_SCROLL_SCREEN_LEFT].setval(KEY_LEFT,0,0,0);
	zq_hotkeys[ZQKEY_SCROLL_SCREEN_RIGHT].setval(KEY_RIGHT,0,0,0);
	zq_hotkeys[ZQKEY_WARP_SCREEN_UP].setval(KEY_UP,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_WARP_SCREEN_DOWN].setval(KEY_DOWN,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_WARP_SCREEN_LEFT].setval(KEY_LEFT,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_WARP_SCREEN_RIGHT].setval(KEY_RIGHT,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_SCROLL_COMBO_UP].setval(KEY_UP,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_SCROLL_COMBO_DOWN].setval(KEY_DOWN,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_SCROLL_COMBO_LEFT].setval(KEY_LEFT,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_SCROLL_COMBO_RIGHT].setval(KEY_RIGHT,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_COMBO_PAGEUP].setval(KEY_PGUP,0,0,0);
	zq_hotkeys[ZQKEY_COMBO_PAGEDN].setval(KEY_PGDN,0,0,0);
	zq_hotkeys[ZQKEY_SQUAREPANEL_UP].setval(KEY_PGUP,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_SQUAREPANEL_DOWN].setval(KEY_PGDN,KB_SHIFT_FLAG,0,0);
	zq_hotkeys[ZQKEY_TESTMODE].setval(KEY_T,KB_CTRL_CMD_FLAG,0,0);
	zq_hotkeys[ZQKEY_CAUTO_HEIGHTPLUS].setval(KEY_CLOSEBRACE, KB_SHIFT_FLAG, 0, 0);
	zq_hotkeys[ZQKEY_CAUTO_HEIGHTMINUS].setval(KEY_OPENBRACE, KB_SHIFT_FLAG, 0, 0);
	zq_hotkeys[ZQKEY_CURR_LAYER_HL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIEW_MAP].setval(KEY_M, KB_CTRL_CMD_FLAG, 0, 0);
	
	zq_hotkeys[ZQKEY_DRAWMODE_NORMAL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DRAWMODE_ALIAS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DRAWMODE_POOL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DRAWMODE_AUTO].setval(0, 0, 0, 0);
	
	zq_hotkeys[ZQKEY_IMPORT_COMBOS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_COMBOS].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_DMAPS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_DMAPS].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_ZGP].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_ZGP].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_MAP].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_MAP].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_PALS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_PALS].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_STRINGS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_STRINGS].setval(0, 0, 0, 0);
    zq_hotkeys[ZQKEY_IMPORT_TILES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXPORT_TILES].setval(0, 0, 0, 0);
	
	zq_hotkeys[ZQKEY_CHANGE_TRACK].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_CHEATS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_CSET_FIX].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_ALIASES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_CMBPOOLS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_AUTOCMB].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_COMBOS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DELETE_MAP].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DMAPS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DOOR_COMBO_SETS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_DOORS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_ENDSTRING].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_ENEMIES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_ENEMIES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_ENEMIES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EXIT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_FFCS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_GAME_ICONS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_GOTO_MAP].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_GUY_STR].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_HEADER].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_INFO_TYPES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_INIT_DATA].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_INTEG_CHECK_ALL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_INTEG_CHECK_SCREENS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_INTEG_CHECK_WARPS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_ITEMS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_LAYERS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PALETTES_LEVEL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_HERO_SPRITE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_USED_COMBOS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PALETTES_MAIN].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_MAP_STYLES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_MAP_STYLES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_MIDIS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_MISC_COLORS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_NEW].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_OPTIONS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_PALETTES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_MAZE_PATH].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PLAY_MUSIC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_APPLY_TEMPLATE_ALL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REVERT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_ROOMTYPE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_SCREEN_DATA].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_SECRET_COMBOS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SFX].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SHOP_TYPES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PALETTES_SPRITES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_SPRITE_DATA].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_STOP_TUNES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_SUBSCREENS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_AMBIENT_MUSIC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_NES_DUNGEON_TEMPL].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_TILES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_MCGUFFIN_PIECES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_UNDERCOMBO].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_UNDERCOMBO].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIDEO_MODE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIEW_PALETTE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIEW_PIC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_WARP_RET].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_WARP_RINGS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_WARPS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_SPRITE_DATA].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SHOW_CSETS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SHOW_TYPES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEFAULT_ITEMS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_DROPSETS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_PASTE_PALETTE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_QUEST_RULES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_COMBO_LOC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_TYPE_LOC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_ENEMY_LOC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_ITEM_LOC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_SCRIPT_LOC].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_REPORT_LINKS_HERE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_CLEAR_QST_PATH].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_BUGGY_NEXT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_RULES_ZSCRIPT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_RULES_COMPILER].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_EDIT_SCREEN_SCRIPT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SCREEN_SCREENSHOT].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIEW_L2_BG].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_VIEW_L3_BG].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_BOTTLE_TYPES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_BOTTLE_SHOP_TYPES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_FIX_WATER_SOLID].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_FIX_EFFECT_SQUARE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SEARCH_QRS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_COMPILE_QUICK].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_RULESETS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_RULETMPLS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_COMPILE_SMART].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_DEBUG_CONSOLE].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SHOW_HOTKEYS].setval(KEY_SLASH, KB_SHIFT_FLAG, 0, 0);
	zq_hotkeys[ZQKEY_BIND_HOTKEYS].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_SCREEN_NOTES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_BROWSE_SCREEN_NOTES].setval(0, 0, 0, 0);
	zq_hotkeys[ZQKEY_STATUS_EFFECTS].setval(0, 0, 0, 0);
}

void load_hotkeys()
{
	default_hotkeys();
	map<string, uint> keymap;
	for(uint q = 1; q < ZQKEY_MAX; ++q)
	{
		int v = zq_hotkeys[q].getval();
		char const* cfgname = get_hotkey_cfg_name(q);
		keymap[cfgname] = q;
		int nv = zc_get_config("ZQ_HOTKEY",cfgname,v);
		if(v != nv)
			zq_hotkeys[q].setval(nv);
	}
	for(int q = 0; q < MAXFAVORITECOMMANDS; ++q)
	{
		string str(zc_get_config("ZQ_HOTKEY",fmt::format("command{:02}",q+1).c_str(),""));
		if(!str.empty())
		{
			auto it = keymap.find(str);
			if(it != keymap.end())
			{
				favorite_commands[q] = it->second;
				continue;
			}
		}
		favorite_commands[q] = 0;
	}
}

void write_fav_command(uint ind, uint hkey)
{
	favorite_commands[ind] = hkey;
	zc_set_config("ZQ_HOTKEY", fmt::format("command{:02}",ind+1).c_str(),
		get_hotkey_cfg_name(hkey));
}

void disable_hotkey(uint hkey, bool dis)
{
	if(hkey >= ZQKEY_MAX)
		return;
	fav_cmd_disabled[hkey] = dis;
}
bool disabled_hotkey(uint hkey)
{
	if(hkey >= ZQKEY_MAX)
		return true;
	if(fav_cmd_disabled[hkey])
		return true;
	switch(hkey)
	{
		case ZQKEY_SAVE: case ZQKEY_REVERT:
			return saved | disable_saving | OverwriteProtection;
		case ZQKEY_SAVEAS:
			return disable_saving;
		case ZQKEY_PASTE: case ZQKEY_PASTEALL: case ZQKEY_PASTETOALL:
		case ZQKEY_PASTEALLTOALL: case ZQKEY_PASTE_DOORS:
		case ZQKEY_PASTE_ENEMIES: case ZQKEY_PASTE_LAYERS:
		case ZQKEY_PASTE_FFCS: case ZQKEY_PASTE_GUY_STR:
		case ZQKEY_PASTE_ROOMTYPE: case ZQKEY_PASTE_SCREEN_DATA:
		case ZQKEY_PASTE_SECRET_COMBOS: case ZQKEY_PASTE_UNDERCOMBO:
		case ZQKEY_PASTE_WARP_RET: case ZQKEY_PASTE_WARPS:
		case ZQKEY_PASTE_PALETTE:
			return !Map.CanPaste();
		case ZQKEY_COPY: case ZQKEY_DELETE:
			return !(Map.CurrScr()->valid&mVALID);
		case ZQKEY_NES_DUNGEON_TEMPL:
			return Map.getCurrScr() >= TEMPLATE;
		case ZQKEY_UNDO:
			return !Map.CanUndo();
		case ZQKEY_REDO:
			return !Map.CanRedo();
		case ZQKEY_DOORS:
			return Map.getCurrScr() >= MAPSCRS;
	}
	return false;
}

bool selected_hotkey(uint hkey)
{
	switch(hkey)
	{
		case ZQKEY_VIEW_L2_BG:
			return ViewLayer2BG;
		case ZQKEY_VIEW_L3_BG:
			return ViewLayer3BG;
		case ZQKEY_DRAWMODE_AUTO:
			return draw_mode == dm_auto;
		case ZQKEY_DRAWMODE_POOL:
			return draw_mode==dm_cpool;
		case ZQKEY_DRAWMODE_ALIAS:
			return draw_mode==dm_alias;
		case ZQKEY_DRAWMODE_NORMAL:
			return draw_mode==dm_normal;
		case ZQKEY_DARK_PREVIEW:
			return (get_qr(qr_NEW_DARKROOM) && (Flags&cNEWDARK));
		case ZQKEY_SCREEN_NOTES:
			return !Map.CurrScr()->usr_notes.empty();
	}
	return false;
}

int run_hotkey(uint hkey)
{
	if(disabled_hotkey(hkey))
		return D_O_K;

	if (hotkeys_is_active() && hkey != ZQKEY_SHOW_HOTKEYS)
		hotkeys_toggle_display(false);

	switch(hkey)
	{
		case ZQKEY_NULL_KEY: break;
		case ZQKEY_UNDO:
			return onUndo();
		case ZQKEY_REDO:
			return onRedo();
		case ZQKEY_MINUS_FLAG:
			return onDecreaseFlag();
		case ZQKEY_PLUS_FLAG:
			return onIncreaseFlag();
		case ZQKEY_SAVE:
			return onSave();
		case ZQKEY_SAVEAS:
			return onSaveAs();
		case ZQKEY_OPEN:
			return do_OpenQuest();
		case ZQKEY_SCREEN_PAL:
			return onScreenPalette();
		case ZQKEY_SECRET_COMBO:
			return onSecretCombo();
		case ZQKEY_DOORS:
			return onDoors();
		case ZQKEY_FFCS:
			return onSelectFFCombo();
		case ZQKEY_FLAGS:
			return onFlags();
		case ZQKEY_SCRDATA:
			return onScrData();
		case ZQKEY_TILEWARP:
			return onTileWarp();
		case ZQKEY_SIDEWARP:
			return onSideWarp();
		case ZQKEY_LAYERS:
			return onLayers();
		case ZQKEY_RESET_TRANSP:
			return onResetTransparency();
		case ZQKEY_COPY:
			return onCopy();
		case ZQKEY_TOGGLE_DARK:
			return onToggleDarkness();
		case ZQKEY_ENEMIES:
			return onEnemies();
		case ZQKEY_SHOW_FLAGS:
			return onShowFlags();
		case ZQKEY_SHOW_FFCS:
			return onToggleShowFFCs();
		case ZQKEY_ITEMS:
			return onItem();
		case ZQKEY_COMBOS:
			return onCombos();
		case ZQKEY_DARK_PREVIEW:
			return onShowDarkness();
		case ZQKEY_SHOW_INFO:
			return onToggleShowInfo();
		case ZQKEY_DRAWING_MODE:
			return onDrawingMode();
		case ZQKEY_GOTO_PAGE:
			return onGotoPage();
		case ZQKEY_SHOW_CMB_CS_INFO:
			return onShowComboInfoCSet();
		case ZQKEY_ROOM:
			return onRoom();
		case ZQKEY_STRINGS:
			return onStrings();
		case ZQKEY_TILES:
			return onTiles();
		case ZQKEY_PASTE:
			return onPaste();
		case ZQKEY_PASTEALL:
			return onPasteAll();
		case ZQKEY_PASTETOALL:
			return onPasteToAll();
		case ZQKEY_PASTEALLTOALL:
			return onPasteAllToAll();
		case ZQKEY_SHOW_SOLID:
			return onShowWalkability();
		case ZQKEY_PREV_MODE:
			return onPreviewMode();
		case ZQKEY_COMPILE_ZSCRIPT:
			return onCompileScript();
		case ZQKEY_SCREENSHOT:
			return onSnapshot();
		case ZQKEY_ABOUT:
			return onAbout();
		case ZQKEY_MINUS_MAP:
			return onDecMap();
		case ZQKEY_PLUS_MAP:
			return onIncMap();
		case ZQKEY_MINUS_COLOR:
			return onDecreaseCSet();
		case ZQKEY_PLUS_COLOR:
			return onIncreaseCSet();
		case ZQKEY_MINUS_SCR_PAL:
			return onDecScrPal();
		case ZQKEY_PLUS_SCR_PAL:
			return onIncScrPal();
		case ZQKEY_MINUS_16_SCR_PAL:
			return onDecScrPal16();
		case ZQKEY_PLUS_16_SCR_PAL:
			return onIncScrPal16();
		case ZQKEY_SCREEN_ZOOM_IN:
			return onZoomIn();
		case ZQKEY_SCREEN_ZOOM_OUT:
			return onZoomOut();
		case ZQKEY_GRID:
			return onToggleGrid(false);
		case ZQKEY_GRID_COLOR:
			return onToggleGrid(true);
			
		case ZQKEY_CHANGE_ALIAS_ORIGIN:
			alias_origin = (alias_origin+1)%4;;
			break;
		case ZQKEY_DELETE:
			return onDelete();
		case ZQKEY_FULLSCREEN:
			return onFullScreen();
		case ZQKEY_LYR_0: case ZQKEY_LYR_1: case ZQKEY_LYR_2: case ZQKEY_LYR_3:
		case ZQKEY_LYR_4: case ZQKEY_LYR_5: case ZQKEY_LYR_6:
			return onSetNewLayer(hkey-ZQKEY_LYR_0);
		case ZQKEY_SCR_LPAL_0: case ZQKEY_SCR_LPAL_1: case ZQKEY_SCR_LPAL_2: case ZQKEY_SCR_LPAL_3:
		case ZQKEY_SCR_LPAL_4: case ZQKEY_SCR_LPAL_5: case ZQKEY_SCR_LPAL_6: case ZQKEY_SCR_LPAL_7:
		case ZQKEY_SCR_LPAL_8: case ZQKEY_SCR_LPAL_9: case ZQKEY_SCR_LPAL_10: case ZQKEY_SCR_LPAL_11:
		case ZQKEY_SCR_LPAL_12: case ZQKEY_SCR_LPAL_13: case ZQKEY_SCR_LPAL_14: case ZQKEY_SCR_LPAL_15:
			return onScreenLPal(hkey-ZQKEY_SCR_LPAL_0);
		case ZQKEY_SCROLL_SCREEN_UP: case ZQKEY_SCROLL_SCREEN_DOWN:
		case ZQKEY_SCROLL_SCREEN_LEFT: case ZQKEY_SCROLL_SCREEN_RIGHT:
			return onScrollScreen(hkey-ZQKEY_SCROLL_SCREEN_UP,false);
		case ZQKEY_WARP_SCREEN_UP: case ZQKEY_WARP_SCREEN_DOWN:
		case ZQKEY_WARP_SCREEN_LEFT: case ZQKEY_WARP_SCREEN_RIGHT:
			return onScrollScreen(hkey-ZQKEY_WARP_SCREEN_UP,true);
		case ZQKEY_SCROLL_COMBO_UP:
			return onComboColUp();
		case ZQKEY_SCROLL_COMBO_DOWN:
			return onComboColDown();
		case ZQKEY_SCROLL_COMBO_LEFT:
			return onComboColLeft();
		case ZQKEY_SCROLL_COMBO_RIGHT:
			return onComboColRight();
		case ZQKEY_COMBO_PAGEUP:
			return onPgUp();
		case ZQKEY_COMBO_PAGEDN:
			return onPgDn();
		case ZQKEY_SQUAREPANEL_UP:
			cycle_compact_sqr(false);
			break;
		case ZQKEY_SQUAREPANEL_DOWN:
			cycle_compact_sqr(true);
			break;
		case ZQKEY_TESTMODE:
			call_testqst_dialog();
			break;
		case ZQKEY_CAUTO_HEIGHTMINUS:
			change_autocombo_height(-1);
			break;
		case ZQKEY_CAUTO_HEIGHTPLUS:
			change_autocombo_height(1);
			break;
		case ZQKEY_CURR_LAYER_HL:
			onToggleHighlightLayer();
			break;
		case ZQKEY_VIEW_MAP:
			onViewMapEx(false);
			break;
		case ZQKEY_DRAWMODE_NORMAL:
			onDrawingModeNormal();
			break;
		case ZQKEY_DRAWMODE_ALIAS:
			onDrawingModeAlias();
			break;
		case ZQKEY_DRAWMODE_POOL:
			onDrawingModePool();
			break;
		case ZQKEY_DRAWMODE_AUTO:
			onDrawingModeAuto();
			break;
		case ZQKEY_IMPORT_COMBOS:
			onImport_Combos();
			break;
		case ZQKEY_EXPORT_COMBOS:
			onExport_Combos();
			break;
		case ZQKEY_IMPORT_DMAPS:
			onImport_DMaps();
			break;
		case ZQKEY_EXPORT_DMAPS:
			onExport_DMaps();
			break;
		case ZQKEY_IMPORT_ZGP:
			onImport_ZGP();
			break;
		case ZQKEY_EXPORT_ZGP:
			onExport_ZGP();
			break;
		case ZQKEY_IMPORT_MAP:
			onImport_Map();
			break;
		case ZQKEY_EXPORT_MAP:
			onExport_Map();
			break;
		case ZQKEY_IMPORT_PALS:
			onImport_Pals();
			break;
		case ZQKEY_EXPORT_PALS:
			onExport_Pals();
			break;
		case ZQKEY_IMPORT_STRINGS:
			onImport_StringsTSV();
			break;
		case ZQKEY_EXPORT_STRINGS:
			onExport_StringsTSV();
			break;
		case ZQKEY_IMPORT_TILES:
			onImport_Tiles();
			break;
		case ZQKEY_EXPORT_TILES:
			onExport_Tiles();
			break;
			
		case ZQKEY_CHANGE_TRACK:
			changeTrack();
			break;
		case ZQKEY_CHEATS:
			onCheats();
			break;
		case ZQKEY_CSET_FIX:
			onCSetFix();
			break;
		case ZQKEY_ALIASES:
			call_alias_pages();
			break;
		case ZQKEY_CMBPOOLS:
			call_cpool_pages();
			break;
		case ZQKEY_AUTOCMB:
			call_autoc_pages();
			break;
		case ZQKEY_DEFAULT_COMBOS:
			onDefault_Combos();
			break;
		case ZQKEY_DELETE_MAP:
			onDeleteMap();
			break;
		case ZQKEY_DMAPS:
			onDmaps();
			break;
		case ZQKEY_DOOR_COMBO_SETS:
			onDoorCombos();
			break;
		case ZQKEY_PASTE_DOORS:
			onPasteDoors();
			break;
		case ZQKEY_ENDSTRING:
			onEndString();
			break;
		case ZQKEY_EDIT_ENEMIES:
			onCustomEnemies();
			break;
		case ZQKEY_DEFAULT_ENEMIES:
			onDefault_Guys();
			break;
		case ZQKEY_PASTE_ENEMIES:
			onPasteEnemies();
			break;
		case ZQKEY_EXIT:
			onCmdExit();
			break;
		case ZQKEY_PASTE_FFCS:
			onPasteFFCombos();
			break;
		case ZQKEY_GAME_ICONS:
			onIcons();
			break;
		case ZQKEY_GOTO_MAP:
			onGotoMap();
			break;
		case ZQKEY_PASTE_GUY_STR:
			onPasteGuy();
			break;
		case ZQKEY_HEADER:
			onHeader();
			break;
		case ZQKEY_INFO_TYPES:
			onInfoTypes();
			break;
		case ZQKEY_INIT_DATA:
			onInit();
			break;
		case ZQKEY_INTEG_CHECK_ALL:
			onIntegrityCheckAll();
			break;
		case ZQKEY_INTEG_CHECK_SCREENS:
			onIntegrityCheckRooms();
			break;
		case ZQKEY_INTEG_CHECK_WARPS:
			onIntegrityCheckWarps();
			break;
		case ZQKEY_EDIT_ITEMS:
			onCustomItems();
			break;
		case ZQKEY_PASTE_LAYERS:
			onPasteLayers();
			break;
		case ZQKEY_PALETTES_LEVEL:
			onColors_Levels();
			break;
		case ZQKEY_HERO_SPRITE:
			onCustomHero();
			break;
		case ZQKEY_USED_COMBOS:
			onUsedCombos();
			break;
		case ZQKEY_PALETTES_MAIN:
			onColors_Main();
			break;
		case ZQKEY_DEFAULT_MAP_STYLES:
			onDefault_MapStyles();
			break;
		case ZQKEY_MAP_STYLES:
			onMapStyles();
			break;
		case ZQKEY_MIDIS:
			onMidis();
			break;
		case ZQKEY_MISC_COLORS:
			onMiscColors();
			break;
		case ZQKEY_NEW:
			do_NewQuest();
			break;
		case ZQKEY_OPTIONS:
			onOptions();
			break;
		case ZQKEY_DEFAULT_PALETTES:
			onDefault_Pals();
			break;
		case ZQKEY_MAZE_PATH:
			onPath();
			break;
		case ZQKEY_PLAY_MUSIC:
			playMusic();
			break;
		case ZQKEY_APPLY_TEMPLATE_ALL:
			onReTemplate();
			break;
		case ZQKEY_REVERT:
			onRevert();
			break;
		case ZQKEY_PASTE_ROOMTYPE:
			onPasteRoom();
			break;
		case ZQKEY_PASTE_SCREEN_DATA:
			onPasteScreenData();
			break;
		case ZQKEY_PASTE_SECRET_COMBOS:
			onPasteSecretCombos();
			break;
		case ZQKEY_SFX:
			onSelectSFX();
			break;
		case ZQKEY_SHOP_TYPES:
			onShopTypes();
			break;
		case ZQKEY_PALETTES_SPRITES:
			onColors_Sprites();
			break;
		case ZQKEY_DEFAULT_SPRITE_DATA:
			onDefault_Weapons();
			break;
		case ZQKEY_STOP_TUNES:
			stopMusic();
			break;
		case ZQKEY_EDIT_SUBSCREENS:
			onEditSubscreens();
			break;
		case ZQKEY_AMBIENT_MUSIC:
			playZCForever();
			break;
		case ZQKEY_NES_DUNGEON_TEMPL:
			onTemplate();
			break;
		case ZQKEY_DEFAULT_TILES:
			onDefault_Tiles();
			break;
		case ZQKEY_MCGUFFIN_PIECES:
			onTriPieces();
			break;
		case ZQKEY_UNDERCOMBO:
			onUnderCombo();
			break;
		case ZQKEY_PASTE_UNDERCOMBO:
			onPasteUnderCombo();
			break;
		case ZQKEY_VIDEO_MODE:
			onZQVidMode();
			break;
		case ZQKEY_VIEW_PALETTE:
			onShowPal();
			break;
		case ZQKEY_VIEW_PIC:
			onViewPic();
			break;
		case ZQKEY_PASTE_WARP_RET:
			onPasteWarpLocations();
			break;
		case ZQKEY_WARP_RINGS:
			onWarpRings();
			break;
		case ZQKEY_PASTE_WARPS:
			onPasteWarps();
			break;
		case ZQKEY_EDIT_SPRITE_DATA:
			onCustomWpns();
			break;
		case ZQKEY_SHOW_CSETS:
			onShowCSet();
			break;
		case ZQKEY_SHOW_TYPES:
			onShowCType();
			break;
		case ZQKEY_DEFAULT_ITEMS:
			onDefault_Items();
			break;
		case ZQKEY_EDIT_DROPSETS:
			onItemDropSets();
			break;
		case ZQKEY_PASTE_PALETTE:
			onPastePalette();
			break;
		case ZQKEY_QUEST_RULES:
			onRulesDlg();
			break;
		case ZQKEY_REPORT_COMBO_LOC:
			onComboLocationReport();
			break;
		case ZQKEY_REPORT_TYPE_LOC:
			onComboTypeLocationReport();
			break;
		case ZQKEY_REPORT_ENEMY_LOC:
			onEnemyLocationReport();
			break;
		case ZQKEY_REPORT_ITEM_LOC:
			onItemLocationReport();
			break;
		case ZQKEY_REPORT_SCRIPT_LOC:
			onScriptLocationReport();
			break;
		case ZQKEY_REPORT_LINKS_HERE:
			onWhatWarpsReport();
			break;
		case ZQKEY_CLEAR_QST_PATH:
			onClearQuestFilepath();
			break;
		case ZQKEY_BUGGY_NEXT:
			onBuggedNextComboLocationReport();
			break;
		case ZQKEY_RULES_ZSCRIPT:
			onZScriptSettings();
			break;
		case ZQKEY_RULES_COMPILER:
			onZScriptCompilerSettings();
			break;
		case ZQKEY_EDIT_SCREEN_SCRIPT:
			onScreenScript();
			break;
		case ZQKEY_SCREEN_SCREENSHOT:
			onMapscrSnapshot();
			break;
		case ZQKEY_VIEW_L2_BG:
			onLayer2BG();
			break;
		case ZQKEY_VIEW_L3_BG:
			onLayer3BG();
			break;
		case ZQKEY_BOTTLE_TYPES:
			onBottleTypes();
			break;
		case ZQKEY_BOTTLE_SHOP_TYPES:
			onBottleShopTypes();
			break;
		case ZQKEY_FIX_WATER_SOLID:
			onWaterSolidity();
			break;
		case ZQKEY_FIX_EFFECT_SQUARE:
			onEffectFix();
			break;
		case ZQKEY_SEARCH_QRS:
			onRulesSearch();
			break;
		case ZQKEY_COMPILE_QUICK:
			onQuickCompile();
			break;
		case ZQKEY_RULESETS:
			PickRuleset();
			break;
		case ZQKEY_RULETMPLS:
			PickRuleTemplate();
			break;
		case ZQKEY_COMPILE_SMART:
			onSmartCompile();
			break;
		case ZQKEY_DEBUG_CONSOLE:
			toggleConsole();
			break;
		case ZQKEY_SHOW_HOTKEYS:
			showHotkeys();
			break;
		case ZQKEY_BIND_HOTKEYS:
			do_zq_hotkey_dialog();
			break;
		case ZQKEY_SCREEN_NOTES:
			edit_screen_notes(Map.CurrScr(), Map.getCurrMap(), Map.getCurrScr());
			break;
		case ZQKEY_BROWSE_SCREEN_NOTES:
			browse_screen_notes();
			break;
		case ZQKEY_STATUS_EFFECTS:
			onStatusEffects();
			break;
	}
	return D_O_K;
}

int run_fav_cmd(uint favcmd)
{
	if(favcmd >= MAXFAVORITECOMMANDS)
		return D_O_K;
	return run_hotkey(favorite_commands[favcmd]);
}

int d_zq_hotkey_proc(int msg, DIALOG* d, int c)
{
	int ret = D_O_K;
	switch(msg)
	{
		case MSG_START:
			d->w = d->h = 0;
			break;
		case MSG_XCHAR:
			int key = c>>8;
			if(is_modkey(key) || !(key))
				break;
			int shifts = get_mods();
			if(key==KEY_F4 && (shifts&KB_ALT_FLAG))
			{
				close_button_quit = true;
				return D_USED_CHAR;
			}
			for(int pass = 0; pass <= 1; ++pass)
				for(int q = 0; q < ZQKEY_MAX; ++q)
				{
					if(zq_hotkeys[q].check(c>>8,shifts,!pass))
						return run_hotkey(q) | D_USED_CHAR;
				}
			break;
	}
	return ret;
}

int do_zq_hotkey_dialog()
{
	Hotkey tmp_hotkeys[ZQKEY_MAX];
	memcpy(tmp_hotkeys, zq_hotkeys, sizeof(tmp_hotkeys));
	
	bool confirm = false;
	ZQHotkeyDialog(confirm).show();
	if(confirm)
	{
		for(int q = 0; q < ZQKEY_MAX; ++q)
		{
			if(tmp_hotkeys[q] != zq_hotkeys[q])
				zc_set_config("ZQ_HOTKEY",get_hotkey_cfg_name(q),zq_hotkeys[q].getval());
		}
		hotkeys_invalidate();
	}
	else
		memcpy(zq_hotkeys, tmp_hotkeys, sizeof(tmp_hotkeys));
	return D_O_K;
}

int do_zq_list_hotkeys_dialog()
{
	hotkeys_toggle_display(true);
	return D_O_K;
}

optional<uint> select_fav_command()
{
	optional<uint> ret;
	ZQHotkeyDialog(ret).show();
	return ret;
}

