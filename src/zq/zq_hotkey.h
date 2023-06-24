#ifndef ZQ_HOTKEY_H
#define ZQ_HOTKEY_H

enum
{
	ZQKEY_UNDO,
	ZQKEY_REDO,
	ZQKEY_PLUS_FLAG,
	ZQKEY_MINUS_FLAG,
	ZQKEY_SAVE,
	ZQKEY_SAVEAS,
	ZQKEY_OPEN,
	ZQKEY_SCREEN_PAL,
	ZQKEY_SECRET_COMBO,
	ZQKEY_DOORS,
	ZQKEY_FFCS,
	ZQKEY_FLAGS,
	ZQKEY_SCRDATA,
	ZQKEY_TILEWARP,
	ZQKEY_SIDEWARP,
	ZQKEY_LAYERS,
	ZQKEY_RESET_TRANSP,
	ZQKEY_COPY,
	ZQKEY_TOGGLE_DARK,
	ZQKEY_ENEMIES,
	ZQKEY_SHOW_FLAGS,
	ZQKEY_ITEMS,
	ZQKEY_COMBOS,
	ZQKEY_DARK_PREVIEW,
	ZQKEY_SHOW_INFO,
	ZQKEY_DRAWING_MODE,
	ZQKEY_GOTO_PAGE,
	ZQKEY_SHOW_CMB_CS_INFO,
	ZQKEY_STRINGS,
	ZQKEY_ROOM,
	ZQKEY_TILES,
	ZQKEY_PASTE,
	ZQKEY_PASTEALL,
	ZQKEY_PASTETOALL,
	ZQKEY_PASTEALLTOALL,
	ZQKEY_SHOW_SOLID,
	ZQKEY_PREV_MODE,
	ZQKEY_COMPILE_ZSCRIPT,
	ZQKEY_SCREENSHOT,
	ZQKEY_ABOUT,
	ZQKEY_MINUS_MAP,
	ZQKEY_PLUS_MAP,
	ZQKEY_MINUS_COLOR,
	ZQKEY_PLUS_COLOR,
	ZQKEY_MINUS_SCR_PAL,
	ZQKEY_PLUS_SCR_PAL,
	ZQKEY_MINUS_16_SCR_PAL,
	ZQKEY_PLUS_16_SCR_PAL,
	ZQKEY_GRID,
	ZQKEY_GRID_COLOR,
	ZQKEY_COMBO_COL_MODE,
	ZQKEY_DELETE,
	ZQKEY_FULLSCREEN,
	ZQKEY_LYR_0,ZQKEY_LYR_1,ZQKEY_LYR_2,ZQKEY_LYR_3,
	ZQKEY_LYR_4,ZQKEY_LYR_5,ZQKEY_LYR_6,
	ZQKEY_SCR_LPAL_0,ZQKEY_SCR_LPAL_1,ZQKEY_SCR_LPAL_2,ZQKEY_SCR_LPAL_3,
	ZQKEY_SCR_LPAL_4,ZQKEY_SCR_LPAL_5,ZQKEY_SCR_LPAL_6,ZQKEY_SCR_LPAL_7,
	ZQKEY_SCR_LPAL_8,ZQKEY_SCR_LPAL_9,ZQKEY_SCR_LPAL_10,ZQKEY_SCR_LPAL_11,
	ZQKEY_SCR_LPAL_12,ZQKEY_SCR_LPAL_13,ZQKEY_SCR_LPAL_14,ZQKEY_SCR_LPAL_15,
	ZQKEY_SCROLL_SCREEN_UP,ZQKEY_SCROLL_SCREEN_DOWN,ZQKEY_SCROLL_SCREEN_LEFT,ZQKEY_SCROLL_SCREEN_RIGHT,
	ZQKEY_WARP_SCREEN_UP,ZQKEY_WARP_SCREEN_DOWN,ZQKEY_WARP_SCREEN_LEFT,ZQKEY_WARP_SCREEN_RIGHT,
	ZQKEY_SCROLL_COMBO_UP,ZQKEY_SCROLL_COMBO_DOWN,ZQKEY_SCROLL_COMBO_LEFT,ZQKEY_SCROLL_COMBO_RIGHT,
	ZQKEY_COMBO_PAGEUP,ZQKEY_COMBO_PAGEDN,
	ZQKEY_SQUAREPANEL_UP,ZQKEY_SQUAREPANEL_DOWN,
	ZQKEY_TESTMODE,
	ZQKEY_MAX
};
extern Hotkey zq_hotkeys[ZQKEY_MAX];
char const* get_hotkey_name(int hkey);
bool is_reserved_key(int c);
bool is_reserved_keycombo(int c, int modflag);
void default_hotkeys();
void load_hotkeys();
int d_zq_hotkey_proc(int msg, DIALOG* d, int c);
int do_zq_hotkey_dialog();

#endif
