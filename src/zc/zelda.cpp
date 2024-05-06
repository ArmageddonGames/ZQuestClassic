#include <_types/_uint32_t.h>
#include <memory>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ctype.h>
#include <string>
#include <map>
#include <vector>
#include <sstream>

#include "zalleg/zalleg.h"
#include "base/qrs.h"
#include "base/dmap.h"
#include "base/cpool.h"
#include "base/packfile.h"
#include "base/msgstr.h"
#include "base/render.h"
#include "base/version.h"
#include "base/zc_alleg.h"
#include "base/misctypes.h"

#include <stdlib.h>

#include "zc/zasm_optimize.h"
#include "zc/zasm_utils.h"
#include "zscriptversion.h"
#include "sound/zcmusic.h"
#include "base/zdefs.h"
#include "zc/zelda.h"
#include "tiles.h"
#include "base/colors.h"
#include "pal.h"
#include "base/zsys.h"
#include "base/zapp.h"
#include "play_midi.h"
#include "qst.h"
#include "zc/matrix.h"
#include "gui/jwin.h"
#include "base/jwinfsel.h"
#include "fontsdat.h"
#include "particles.h"
#include "gamedata.h"
#include "zc/ffscript.h"
#include "zc/jit.h"
#include "zc/script_debug.h"
#include "zc/combos.h"
#include "qst.h"
#include "base/util.h"
#include "drawing.h"
#include "dialog/alert.h"
#include "dialog/info.h"
#include "zc/replay.h"
#include "zc/cheats.h"
#include "zc/saves.h"
#include "base/zc_math.h"
#include <fmt/format.h>
#include <fmt/std.h>
#include "zc/render.h"
#include "zinfo.h"
#include "music_playback.h"

using namespace util;
extern FFScript FFCore; //the core script engine.
extern bool epilepsyFlashReduction;
#include "zconsole/ConsoleLogger.h"
#ifndef _WIN32 //Unix
	#include <fcntl.h>
	#include <unistd.h>
	#include <iostream>
	#include <sstream>
	int32_t pt = 0;
	char* ptname = NULL;
	std::ostringstream lxconsole_oss;
#endif
extern ZModule zcm; //modules
extern zcmodule moduledata;
extern char runningItemScripts[256];
extern char modulepath[2048];

extern char zc_builddate[80];
extern char zc_aboutstr[80];

int32_t DMapEditorLastMaptileUsed = 0;
int32_t switch_type = 0; //Init here to avoid Linux building error in g++.
bool saved = true;
bool zqtesting_mode = false;
static std::string testingqst_name;
bool use_testingst_start = false;
static uint16_t testingqst_dmap = 0;
static uint8_t testingqst_screen = 0;
static uint8_t testingqst_retsqr = 0;
static std::string testingqst_init_data;
static bool replay_debug = false;

extern CConsoleLoggerEx zscript_coloured_console;

static zc_randgen drunk_rng;

#include "init.h"
#include <assert.h>
#include "zc/rendertarget.h"
#include "zconsole.h"
#include "base/win32.h"

#define LOGGAMELOOP 0

#ifdef _MSC_VER
#include <crtdbg.h>
#define stricmp _stricmp
#define getcwd _getcwd
#endif

#ifdef __EMSCRIPTEN__
#include "base/emscripten_utils.h"
#endif

// MSVC fix
#if _MSC_VER >= 1900
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif

#if DEVLEVEL > 0
bool dev_logging = false;
bool dev_debug = false; //UNUSED
bool dev_timestmp = false;
#endif

ZCMUSIC *zcmusic = NULL;
ZCMIXER *zcmixer = NULL;
int32_t colordepth;
int32_t db=0;
int32_t detail_int[10];                                         //temporary holder for things you want to detail
int32_t lens_hint_item[MAXITEMS][2]= {{0,0},{0,0}};                            //aclk, aframe
int32_t lens_hint_weapon[MAXWPNS][5] = {{0,0},{0,0}};                           //aclk, aframe, dir, x, y
int32_t cheat_modifier_keys[4]; //two options each, default either control and either shift
int32_t strike_hint_counter=0;
uint8_t __isZQuest = 0; //shared functions can use this. -
int32_t strike_hint_timer=0;
int32_t strike_hint = 0;
int32_t slot_arg = 0, slot_arg2 = 0;
int32_t previous_DMap = -1;
CScriptDrawingCommands script_drawing_commands;

using std::string;
using std::pair;

int32_t zq_screen_w = 0, zq_screen_h = 0;
int32_t passive_subscreen_height=56;
int32_t original_playing_field_offset=56;
int32_t playing_field_offset=original_playing_field_offset;
int32_t passive_subscreen_offset=0;
extern int32_t directItemA;
extern int32_t directItemB;
extern int32_t directItemY;
extern int32_t directItemX;
extern int32_t hangcount;

extern bool kb_typing_mode;

bool is_compact = false;

bool standalone_mode=false;
fs::path standalone_quest;
std::string standalone_save_path;
bool disable_save_to_disk=false;

int32_t favorite_combos[MAXFAVORITECOMBOS] = {0};
byte favorite_combo_modes[MAXFAVORITECOMBOS] = {0};

void playLevelMusic();

//Prevent restarting during ending sequence from creating a rect clip
int32_t draw_screen_clip_rect_x1=0;
int32_t draw_screen_clip_rect_x2=255;
int32_t draw_screen_clip_rect_y1=0;
int32_t draw_screen_clip_rect_y2=223;

extern int32_t script_hero_sprite;
extern int32_t script_hero_cset;
extern int32_t script_hero_flip;

bool trip=false;
extern byte midi_suspended;
extern int32_t paused_midi_pos;

bool doThrottle()
{
#ifdef ALLEGRO_MACOSX
	int toggle_key = KEY_BACKQUOTE;
#else
	int toggle_key = KEY_TILDE;
#endif
	return (Throttlefps ^ (zc_get_system_key(toggle_key)!=0))
		|| (get_qr(qr_NOFASTMODE) && !replay_is_replaying());
}

void zc_throttle_fps()
{
	int32_t target = Maxfps;
	if (doThrottle() || Paused)
		target = 60;
	throttleFPS(target);
}

int32_t onHelp()
{
    //  restore_mouse();
    //  doHelp(vc(15),vc(0));
    return D_O_K;
}
int32_t d_dropdmaplist_proc(int32_t ,DIALOG *,int32_t)
{
    return D_O_K;
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

int32_t startdmapxy[6] = {0,0,0,0,0,0};

/**********************************/
/******** Global Variables ********/
/**********************************/

int32_t curr_tb_page=0;

RGB_MAP rgb_table;
COLOR_MAP trans_table, trans_table2;

BITMAP     *framebuf, *menu_bmp, *gui_bmp, *scrollbuf, *tmp_bmp, *tmp_scr, *screen2,
           *msg_portrait_display_buf, *msg_txt_display_buf, *msg_bg_display_buf,
		   *pricesdisplaybuf, *tb_page[3], *prim_bmp,
		   *script_menu_buf, *f6_menu_buf;
BITMAP     *zcmouse[NUM_ZCMOUSE];
PALETTE    RAMpal;
PALETTE    pal_gui;
byte       *colordata, *trashbuf;
//byte       *tilebuf;
itemdata   *itemsbuf;
wpndata    *wpnsbuf;
comboclass *combo_class_buf;
guydata    *guysbuf;
item_drop_object    item_drop_sets[MAXITEMDROPSETS];
ZCHEATS    zcheats;
byte       use_cheats;
byte       use_tiles;
char       palnames[MAXLEVELS][PALNAMESIZE];

bool ewind_restart=false;

word     msgclk = 0, msgstr = 0, enqueued_str = 0,
         msgpos = 0,	// screen position of the next character.
         msgptr = 0,	// position within the string of the next character.
         msgcolour = 0,	// colour to use for the displayed text.
         msgspeed = 0,	// delay between each character (5 = default).
         msg_w = 0,
         msg_h = 0,
         msg_count = 0,
         cursor_x = 0,
         cursor_y = 0,
         msg_xpos=0,
         msg_ypos=0,
         msgorig=0;
int16_t msg_margins[4] = {0};
byte msgstr_layer = 6;
int32_t prt_tile=0;
byte prt_cset=0, prt_x=0, prt_y=0, prt_tw=0, prt_th=0, msg_shdtype=0, msg_shdcol=0;
bool msg_onscreen = false, msg_active = false, msgspace = false;
BITMAP   *msg_txt_bmp_buf = NULL, *msg_bg_bmp_buf = NULL, *msg_portrait_bmp_buf = NULL, *msg_menu_bmp_buf = NULL;
BITMAP   *darkscr_bmp_curscr = NULL, *darkscr_bmp_scrollscr = NULL,
         *darkscr_bmp_curscr_trans = NULL, *darkscr_bmp_scrollscr_trans = NULL;
BITMAP *lightbeam_bmp = NULL;
bool lightbeam_present;
FONT	 *msgfont;
word     door_combo_set_count;
word     introclk  = 0, intropos = 0, dmapmsgclk = 0, linkedmsgclk = 0;
int16_t    lensclk = 0;
int32_t     lensid = 0; // Lens's item id. -1 if lens is off.
byte screengrid[22]={0};
byte screengrid_layer[2][22]={0};
byte ffcgrid[MAXFFCS/8]={0};
bool halt=false;
bool screenscrolling=false;
PALETTE tempbombpal;
bool usebombpal = false;

int32_t readsize = 0, writesize = 0;
bool fake_pack_writing=false;

SAMPLE customsfxdata[WAV_COUNT] = {0};
uint8_t customsfxflag[WAV_COUNT>>3]  = {0};
int32_t sfxdat=1;

extern int32_t jwin_pal[jcMAX];
int32_t gui_colorset=99;
int32_t fullscreen = 0;
byte forceExit=0,zc_vsync=0;
byte use_win32_proc=1, console_enabled = 0;
int32_t homescr,currscr,frame=0,currmap=0,dlevel,warpscr,worldscr,scrolling_scr=0,scrolling_map=0;
int32_t newscr_clk=0,opendoors=0,currdmap=0,fadeclk=-1,listpos=0;
int32_t lastentrance=0,lastentrance_dmap=0,prices[3]= {0},loadside = 0, Bwpn = -1, Awpn = -1, Xwpn = -1, Ywpn = -1;
int32_t digi_volume = 0,midi_volume = 0,sfx_volume = 0,emusic_volume = 0,currmidi = -1,hasitem = 0,whistleclk = 0,pan_style = 0;
bool analog_movement=true;
int32_t joystick_index=0,Akey = 0,Bkey = 0,Skey = 0,Lkey = 0,Rkey = 0,Pkey = 0,Exkey1 = 0,Exkey2 = 0,Exkey3 = 0,Exkey4 = 0,Abtn = 0,Bbtn = 0,Sbtn = 0,Mbtn = 0,Lbtn = 0,Rbtn = 0,Pbtn = 0,Exbtn1 = 0,Exbtn2 = 0,Exbtn3 = 0,Exbtn4 = 0,Quit=0;
uint32_t GameFlags=0;
int32_t js_stick_1_x_stick = 0, js_stick_1_x_axis = 0, js_stick_1_x_offset = 0;
int32_t js_stick_1_y_stick = 0, js_stick_1_y_axis = 0, js_stick_1_y_offset = 0;
int32_t js_stick_2_x_stick = 0, js_stick_2_x_axis = 0, js_stick_2_x_offset = 0;
int32_t js_stick_2_y_stick = 0, js_stick_2_y_axis = 0, js_stick_2_y_offset = 0;
int32_t DUkey = 0, DDkey = 0, DLkey = 0, DRkey = 0, DUbtn = 0, DDbtn = 0, DLbtn = 0, DRbtn = 0, ss_after = 0, ss_speed = 0, ss_density = 0, ss_enable = 0;
int32_t hs_startx = 0, hs_starty = 0, hs_xdist = 0, hs_ydist = 0, clockclk = 0, clock_zoras[eMAXGUYS]={0};
int32_t cheat_goto_dmap=0, cheat_goto_screen=0, currcset = 0, currspal6 = -1, currspal14 = -1;
int32_t gfc = 0, gfc2 = 0, pitx = 0, pity = 0, refill_what = 0, refill_why = 0, heart_beep_timer=0, new_enemy_tile_start=1580;
int32_t nets=1580, magicitem=-1,div_prot_item=-1, magiccastclk = 0, quakeclk=0, wavy=0, castx = 0, casty = 0, df_x = 0, df_y = 0, nl1_x = 0, nl1_y = 0, nl2_x = 0, nl2_y = 0;
int32_t magicdrainclk=0, conveyclk=3;
byte newconveyorclk = 0;
bool cheats_execute_goto=false, cheats_execute_light=false;
int32_t checkx = 0, checky = 0;
int32_t loadlast=0;
int32_t skipcont=0;

bool monochrome = false; //GFX are monochrome.
bool palette_user_tinted = false;

bool show_layer_0=true, show_layer_1=true, show_layer_2=true, show_layer_3=true, show_layer_4=true, show_layer_5=true, show_layer_6=true,
//oveheard combos     //pushblocks
     show_layer_over=true, show_layer_push=true, show_sprites=true, show_ffcs=true, show_hitboxes=false, show_walkflags=false, show_ff_scripts=false, show_effectflags = false;


bool Throttlefps = true, MenuOpen = false, ClickToFreeze=false, Paused=false, Saving=false,
	Advance=false, ShowFPS = true, Showpal=false, disableClickToFreeze=false, SaveDragResize=false,
	DragAspect=false, SaveWinPos=false, scaleForceInteger=false, stretchGame=false;
int ShowGameTime=0;
bool SkipTitle = false;
int32_t Maxfps = 0;
double aspect_ratio = 0.75;
int window_min_width = 320, window_min_height = 240;
bool Playing, FrameSkip=false, TransLayers = true,clearConsoleOnLoad = true,clearConsoleOnReload = true;
bool GameLoaded = false;
bool __debug=false,debug_enabled = false;
bool refreshpal,blockpath = false,loaded_guys= false,freeze_guys= false,
     loaded_enemies= false,drawguys= false,watch= false;
bool room_is_dark=false, darkroom=false,naturaldark=false,BSZ= false;                         //,NEWSUBSCR;

bool down_control_states[controls::btnLast] = {false};
bool F12= false,F11= false, F5= false,keyI= false, keyQ= false,
     SystemKeys=true,NESquit= false,volkeys= false,useCD=false,boughtsomething=false,
     fixed_door=false, hookshot_used=false, hookshot_frozen=false,
     pull_hero=false, hs_fix=false, hs_switcher=false,
     cheat_superman=false, gofast=false, checkhero=true, didpit=false, heart_beep=true,
     pausenow=false, castnext=false, add_df1asparkle= false, add_df1bsparkle= false, add_nl1asparkle= false, add_nl1bsparkle= false, add_nl2asparkle= false, add_nl2bsparkle= false,
     is_on_conveyor= false, activated_timed_warp=false;
int32_t hooked_combopos = -1, switchhook_cost_item = -1;
int32_t is_conveyor_stunned = 0;
uint16_t hooked_layerbits = 0;
int32_t hooked_undercombos[14] = {0};
solid_object* switching_object = NULL;

byte COOLSCROLL = 0;

int32_t  add_asparkle=0, add_bsparkle=0;
int32_t SnapshotFormat, NameEntryMode=0;
byte SnapshotScale;

char   zeldadat_sig[52]={0};
char   fontsdat_sig[52]={0};
char   cheat_goto_dmap_str[4]={0};
char   cheat_goto_screen_str[3]={0};
int16_t  visited[6]={0};
byte   guygrid[176]={0};
byte   guygridffc[MAXFFCS]={0};
mapscr tmpscr[2];
mapscr tmpscr2[6];
mapscr tmpscr3[6];
std::vector<std::shared_ptr<zasm_script>> zasm_scripts;
script_data *ffscripts[NUMSCRIPTFFC];
script_data *itemscripts[NUMSCRIPTITEM];
script_data *globalscripts[NUMSCRIPTGLOBAL];
script_data *genericscripts[NUMSCRIPTSGENERIC];
script_data *guyscripts[NUMSCRIPTGUYS];
script_data *lwpnscripts[NUMSCRIPTWEAPONS];
script_data *ewpnscripts[NUMSCRIPTWEAPONS];
script_data *playerscripts[NUMSCRIPTPLAYER];
script_data *screenscripts[NUMSCRIPTSCREEN];
script_data *dmapscripts[NUMSCRIPTSDMAP];
script_data *itemspritescripts[NUMSCRIPTSITEMSPRITE];
script_data *comboscripts[NUMSCRIPTSCOMBODATA];
script_data *subscreenscripts[NUMSCRIPTSSUBSCREEN];

//ZScript array storage
std::vector<ZScriptArray> globalRAM;
ZScriptArray localRAM[NUM_ZSCRIPT_ARRAYS];
std::map<int32_t,ZScriptArray> objectRAM;
ArrayOwner arrayOwner[NUM_ZSCRIPT_ARRAYS];

//script bitmap drawing
ZScriptDrawingRenderTarget* zscriptDrawingRenderTarget;

DebugConsole DebugConsole::singleton = DebugConsole();
ZASMSTackTrace ZASMSTackTrace::singleton = ZASMSTackTrace();


void setZScriptVersion(int32_t s_version)
{
    ZScriptVersion::setVersion(s_version);
}

void initZScriptArrayRAM(bool firstplay)
{
    for(word i = 0; i < NUM_ZSCRIPT_ARRAYS; i++)
    {
        localRAM[i].Clear();
        arrayOwner[i].clear();
    }

	if (!firstplay)
		return;

	//leave to global script ~Init to allocate global memory first time round
	game->globalRAM.clear();
	game->globalRAM.resize(getNumGlobalArrays());
}

void initZScriptGlobalRAM()
{
	for (int32_t q = 0; q < NUMSCRIPTGLOBAL; ++q)
	{
		FFCore.reset_script_engine_data(ScriptType::Global, q);
	}
}

void initZScriptGlobalScript(int32_t ID)
{
	FFCore.reset_script_engine_data(ScriptType::Global, ID);
}

dword getNumGlobalArrays()
{
    word scommand, ret = 0;
	auto& init_script = *globalscripts[GLOBAL_SCRIPT_INIT];
	if (!init_script.valid())
		return 0;

	auto& zasm = init_script.zasm_script->zasm;
	uint32_t start_pc = init_script.pc, end_pc = init_script.end_pc;

	for (auto pc = start_pc; pc < end_pc; pc++)
    {
        scommand = zasm[pc].command;
        
        if(scommand == ALLOCATEGMEMV || scommand == ALLOCATEGMEMR)
            ret++;
    }

    return ret;
}

//movingblock mblock2; //mblock[4]?
//HeroClass   Hero;

int32_t resx= 0,resy= 0;
// the number of horizontal or vertical pixels between the framebuffer (320x240) and the window.
// aka, the letterbox size.
int32_t scrx= 0,scry= 0;
int32_t window_width = 0, window_height = 0;
extern byte pause_in_background;
extern signed char pause_in_background_menu_init;
bool toogam=false;
bool ignoreSideview=false;
int32_t script_mouse_x;
int32_t script_mouse_y;
int32_t script_mouse_z;
int32_t script_mouse_b;

int32_t cheat = (DEVLEVEL > 1) ? 4 : 0;                         // 0 = none; 1,2,3,4 = cheat level
int32_t maxcheat = (DEVLEVEL > 1) ? 4 : 0;

int32_t mouse_down=0;                                             // used to hold the last reading of 'gui_mouse_b()' status
int32_t idle_count=0, active_count=0;


// quest file data
zquestheader QHeader;
byte                midi_flags[MIDIFLAGS_SIZE];
byte                music_flags[MUSICFLAGS_SIZE];
int32_t					msg_strings_size=0;
byte                *quest_file;
dword               quest_map_pos[MAPSCRS*MAXMAPS]={0};

char     *qstpath=NULL;
char     *qstdir=NULL;

// if set, the titlescreen will automatically create a new save with this quest.
std::string load_qstpath;
// if set, no file select is possible for the title screen, and only this quest apply to new saves.
std::string only_qstpath;
char header_version_nul_term[17];

volatile int32_t lastfps=0;
volatile int32_t framecnt=0;
volatile int32_t myvsync=0;

bool update_hw_pal = false;
PALETTE* hw_palette = NULL;
void update_hw_screen(bool force)
{
	if (is_headless())
		return;

#ifdef __EMSCRIPTEN__
	force = true;
#endif

	if(force || (!is_sys_pal && !doThrottle()) || myvsync)
	{
		zc_process_display_events();
		resx = al_get_display_width(all_get_display());
		resy = al_get_display_height(all_get_display());
		if(update_hw_pal && hw_palette)
		{
			zc_set_palette(*hw_palette);
			update_hw_pal = false;
		}
		framecnt++;
		if (myvsync||force)
			render_zc();
		myvsync=0;
	}
}

void myvsync_callback()
{
    ++myvsync;
}
END_OF_FUNCTION(myvsync_callback)

/*
enum { 	SAVESC_BACKGROUND, 		SAVESC_TEXT, 			SAVESC_USETILE, 	
	SAVESC_CURSOR_CSET, 		SAVESC_CUR_SOUND,  		SAVESC_TEXT_CONTINUE_COLOUR, 
	SAVESC_TEXT_SAVE_COLOUR, 	SAVESC_TEXT_RETRY_COLOUR, 	SAVESC_TEXT_CONTINUE_FLASH, 
	SAVESC_TEXT_SAVE_FLASH, 	SAVESC_TEXT_RETRY_FLASH,	SAVESC_MIDI,
	SAVESC_CUR_FLIP, 		    SAVESC_TEXT_DONTSAVE_COLOUR, 	SAVESC_TEXT_SAVEQUIT_COLOUR, 
	SAVESC_TEXT_SAVE2_COLOUR, 	SAVESC_TEXT_QUIT_COLOUR, 	SAVESC_TEXT_DONTSAVE_FLASH,
	SAVESC_TEXT_SAVEQUIT_FLASH,	SAVESC_TEXT_SAVE2_FLASH, 	SAVESC_TEXT_QUIT_FLASH,
	SAVESC_EXTRA1, 			SAVESC_EXTRA2,			SAVESC_EXTRA3,			
	SAVESC_LAST	}; */
#define SAVESC_DEF_TILE 2
int32_t SaveScreenSettings[24] = {
	BLACK, 			WHITE, 			SAVESC_DEF_TILE, 
	1, 			WAV_CHINK,	 	QMisc.colors.msgtext,
	QMisc.colors.msgtext,	QMisc.colors.msgtext,	QMisc.colors.caption,
	QMisc.colors.caption,	QMisc.colors.caption,	-4,
	0, 			QMisc.colors.msgtext, 	QMisc.colors.msgtext, 
	QMisc.colors.msgtext, 	QMisc.colors.msgtext,  	QMisc.colors.caption,
	QMisc.colors.caption,	QMisc.colors.caption,	QMisc.colors.caption,
	0, 			0, 			0 }; //BG, Text, Cursor CSet, Sound, UseTile, Misc
char SaveScreenText[7][32]={"CONTINUE", "SAVE AND QUIT", "QUIT WITHOUT SAVING", "DON'T SAVE", "SAVE AND QUIT", "SAVE", "QUIT" };

void ResetSaveScreenSettings()
{
	SaveScreenSettings[SAVESC_TEXT_CONTINUE_COLOUR] = QMisc.colors.msgtext; 
	SaveScreenSettings[SAVESC_TEXT_SAVE_COLOUR] = QMisc.colors.msgtext; 
	SaveScreenSettings[SAVESC_TEXT_RETRY_COLOUR] = QMisc.colors.msgtext; 
	SaveScreenSettings[SAVESC_TEXT_CONTINUE_FLASH] = QMisc.colors.caption; 
	SaveScreenSettings[SAVESC_TEXT_SAVE_FLASH] = QMisc.colors.caption; 
	SaveScreenSettings[SAVESC_TEXT_RETRY_FLASH] = QMisc.colors.caption;
	SaveScreenSettings[SAVESC_MIDI] = -4;
	//SaveScreenSettings[SAVESC_BACKGROUND] = BLACK;
	SaveScreenSettings[SAVESC_BACKGROUND] = 0; //Isle of Rebirth changed the game over background by changing color 0 of the palette; this needs to be respected!
	SaveScreenSettings[SAVESC_TEXT] = QMisc.colors.msgtext;
	SaveScreenSettings[SAVESC_USETILE] = SAVESC_DEF_TILE;
	SaveScreenSettings[SAVESC_CURSOR_CSET] = 1;
	SaveScreenSettings[SAVESC_CUR_SOUND] =  WAV_CHINK;
	SaveScreenSettings[SAVESC_CUR_FLIP] = 0;
	SaveScreenSettings[SAVESC_TEXT_DONTSAVE_COLOUR] = QMisc.colors.msgtext;
	SaveScreenSettings[SAVESC_TEXT_SAVEQUIT_COLOUR] = QMisc.colors.msgtext;
	SaveScreenSettings[SAVESC_TEXT_SAVE2_COLOUR] = QMisc.colors.msgtext;
	SaveScreenSettings[SAVESC_TEXT_QUIT_COLOUR] = QMisc.colors.msgtext;
	SaveScreenSettings[SAVESC_TEXT_DONTSAVE_FLASH] = QMisc.colors.caption;
	SaveScreenSettings[SAVESC_TEXT_SAVEQUIT_FLASH] = QMisc.colors.caption;
	SaveScreenSettings[SAVESC_TEXT_SAVE2_FLASH] = QMisc.colors.caption;
	SaveScreenSettings[SAVESC_TEXT_QUIT_FLASH] = QMisc.colors.caption;
	SaveScreenSettings[SAVESC_EXTRA1] = 0;
	SaveScreenSettings[SAVESC_EXTRA2] = 0;
	SaveScreenSettings[SAVESC_EXTRA3] = 0;
}

void SetSaveScreenSetting(int32_t indx, int32_t value)
{
	switch(indx)
	{
		case SAVESC_BACKGROUND:
			SaveScreenSettings[SAVESC_BACKGROUND] = vbound(value,0,255);
			break;
		case SAVESC_TEXT:
			SaveScreenSettings[SAVESC_TEXT] = vbound(value,0,255);
			break;
		case SAVESC_USETILE:
			SaveScreenSettings[SAVESC_USETILE] = vbound(value,0,NEWMAXTILES);
			break; 
		case SAVESC_CURSOR_CSET:
			SaveScreenSettings[SAVESC_CURSOR_CSET] = vbound(value,0,14);
			break;
		case SAVESC_CUR_SOUND:
			SaveScreenSettings[SAVESC_CUR_SOUND] = vbound(value,0,255); //MAX_SOUNDS
			break;
		case SAVESC_TEXT_CONTINUE_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_CONTINUE_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVE_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_SAVE_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_RETRY_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_RETRY_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_CONTINUE_FLASH:
			SaveScreenSettings[SAVESC_TEXT_CONTINUE_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVE_FLASH:
			SaveScreenSettings[SAVESC_TEXT_SAVE_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_RETRY_FLASH:
			SaveScreenSettings[SAVESC_TEXT_RETRY_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_MIDI:
			SaveScreenSettings[SAVESC_MIDI] = vbound(value,0,255);
			break;
		case SAVESC_CUR_FLIP:
			SaveScreenSettings[SAVESC_CUR_FLIP] = vbound(value,0,3);
			break;
		case SAVESC_TEXT_DONTSAVE_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_DONTSAVE_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVEQUIT_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_SAVEQUIT_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVE2_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_SAVE2_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_QUIT_COLOUR:
			SaveScreenSettings[SAVESC_TEXT_QUIT_COLOUR] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_DONTSAVE_FLASH:
			SaveScreenSettings[SAVESC_TEXT_DONTSAVE_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVEQUIT_FLASH:
			SaveScreenSettings[SAVESC_TEXT_SAVEQUIT_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_SAVE2_FLASH:
			SaveScreenSettings[SAVESC_TEXT_SAVE2_FLASH] = vbound(value,0,255);
			break;
		case SAVESC_TEXT_QUIT_FLASH:
			SaveScreenSettings[SAVESC_TEXT_QUIT_FLASH] = vbound(value,0,255);
			break;
		default: break;
	}
}


void ChangeSubscreenText(int32_t index, const char *f)
{
	index = vbound(index, 0, SAVESC_END-1);
	strncpy(SaveScreenText[index], f, 31);
	SaveScreenText[index][31]='\0';
}



/**********************************/
/*********** Misc Data ************/
/**********************************/

const char startguy[8] = {-13,-13,-13,-14,-15,-18,-21,-40};
const char gambledat[12*6] =
{
    20,-10,-10, 20,-10,-10, 20,-40,-10, 20,-10,-40,
    50,-10,-10, 50,-10,-10, 50,-40,-10, 50,-10,-40,
    -10,20,-10, -10,20,-10, -40,20,-10, -10,20,-40,
    -10,50,-10, -10,50,-10, -40,50,-10, -10,50,-40,
    -10,-10,20, -10,-10,20, -10,-40,20, -40,-10,20,
    -10,-10,50, -10,-10,50, -10,-40,50, -40,-10,50
};
const byte stx[4][9] =
{
    { 48, 80, 80, 96,112,144,160,160,192},
    { 48, 80, 80, 96,128,144,160,160,192},
    { 80, 80,128,128,160,160,192,192,208},
    { 32, 48, 48, 80, 80,112,112,160,160}
};
const byte sty[4][9] =
{
    {112, 64,128, 96, 80, 96, 64,128,112},
    { 48, 32, 96, 64, 80, 64, 32, 96, 48},
    { 32,128, 64, 96, 64, 96, 48,112, 80},
    { 80, 48,112, 64, 96, 64, 96, 32,128}
};

const byte ten_rupies_x[10] = {120,112,128,96,112,128,144,112,128,120};
const byte ten_rupies_y[10] = {49,65,65,81,81,81,81,97,97,113};

zctune tunes[MAXMIDIS] =
{
    // (title)                              (s) (ls) (le) (l) (vol) (midi)(fmt)
    zctune((char *)"Zelda - Dungeon",     0,  -1,  -1,  1,  176,  NULL, 0),
    zctune((char *)"Zelda - Ending",      0, 129, 225,  1,  160,  NULL, 0),
    zctune((char *)"Zelda - Game Over",   0,  -1,  -1,  1,  224,  NULL, 0),
    zctune((char *)"Zelda - Level 9",     0,  -1,  -1,  1,  255,  NULL, 0),
    zctune((char *)"Zelda - Overworld",   0,  17,  -1,  1,  208,  NULL, 0),
    zctune((char *)"Zelda - Title",       0,  -1,  -1,  0,  168,  NULL, 0),
    zctune((char *)"Zelda - Triforce",    0,  -1,  -1,  0,  168,  NULL, 0)
};

// emusic enhancedMusic[MAXMUSIC];

FONT *setmsgfont()
{
	return get_zc_font(MsgStrings[msgstr].font);
}

void zc_trans_blit(BITMAP* dest, BITMAP* src, int32_t sx, int32_t sy, int32_t dx, int32_t dy, int32_t w, int32_t h)
{
	for(int32_t tx = 0; tx < w; ++tx)
		for(int32_t ty = 0; ty < h; ++ty)
		{
			int32_t c1 = src->line[sy+ty][sx+tx];
			int32_t c2 = dest->line[dy+ty][dx+tx];
			if(c1)
			{
				dest->line[dy+ty][dx+tx] = trans_table.data[c1][c2];
			}
		}
}
void msg_bg(MsgStr const& msg)
{
	if(msg.tile == 0) return;
	if(msg.stringflags & STRINGFLAG_FULLTILE)
	{
		draw_block_flip(msg_bg_bmp_buf,0,0,msg.tile,msg.cset,
			(int32_t)ceil(msg.w/16.0),(int32_t)ceil(msg.h/16.0),0,true,false);
	}
	else
	{
		int32_t add = (get_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT)!=0 ? 2 : 0);
		frame2x2(msg_bg_bmp_buf,0,0,msg.tile,msg.cset,
                 (msg.w>>3)+add,(msg.h>>3)+add,0,true,0);
	}
}
void msg_prt()
{
	clear_bitmap(msg_portrait_bmp_buf);
	if(prt_tile > 0 && prt_th > 0 && prt_tw > 0)
	{
		draw_block_flip(msg_portrait_bmp_buf,0,0,prt_tile,prt_cset,
			prt_tw, prt_th, 0, true, false);
	}
}
void blit_msgstr_bg(BITMAP* dest, int32_t sx, int32_t sy, int32_t dx, int32_t dy, int32_t w, int32_t h)
{
	if(MsgStrings[msgstr].stringflags & STRINGFLAG_TRANS_BG)
	{
		BITMAP* subbmp = create_bitmap_ex(8,w,h);
		if(subbmp)
		{
			color_map = &trans_table2;
			clear_bitmap(subbmp);
			masked_blit(msg_bg_display_buf, subbmp, sx, sy, 0, 0, w, h);
			draw_trans_sprite(dest, subbmp, dx, dy);
			destroy_bitmap(subbmp);
			color_map = &trans_table;
		}
		//zc_trans_blit(dest, msg_bg_display_buf, sx, sy, dx, dy, w, h);
	}
	else
	{
		masked_blit(msg_bg_display_buf, dest, sx, sy, dx, dy, w, h);
	}
}
void blit_msgstr_fg(BITMAP* dest, int32_t sx, int32_t sy, int32_t dx, int32_t dy, int32_t w, int32_t h)
{
	if(MsgStrings[msgstr].stringflags & STRINGFLAG_TRANS_FG)
	{
		BITMAP* subbmp = create_bitmap_ex(8,w,h);
		if(subbmp)
		{
			color_map = &trans_table2;
			clear_bitmap(subbmp);
			masked_blit(msg_txt_display_buf, subbmp, sx, sy, 0, 0, w, h);
			draw_trans_sprite(dest, subbmp, dx, dy);
			destroy_bitmap(subbmp);
			color_map = &trans_table;
		}
		//zc_trans_blit(dest, msg_txt_display_buf, sx, sy, dx, dy, w, h);
	}
	else
	{
		masked_blit(msg_txt_display_buf, dest, sx, sy, dx, dy, w, h);
	}
}
void blit_msgstr_prt(BITMAP* dest, int32_t sx, int32_t sy, int32_t dx, int32_t dy, int32_t w, int32_t h)
{
	masked_blit(msg_portrait_display_buf, dest, sx, sy, dx, dy, w, h);
}

void clearmsgnext(int32_t str)
{
	MsgStrings[str].nextstring = 0;
}

void clr_msg_data();
void donewmsg(int32_t str)
{
	if(msg_onscreen || msg_active)
		dismissmsg();
	clr_msg_data();
    //al_trace("donewmsg %d\n",str);
    
        
    linkedmsgclk=0;
    msg_active = true;
    // Don't set msg_onscreen - not onscreen just yet
    msgstr = str;
    msgorig = msgstr;
    msgfont = setmsgfont();
    msgcolour=QMisc.colors.msgtext;
    msgspeed=zinit.msg_speed;
	msgstr_layer=MsgStrings[msgstr].drawlayer;
    
    if(introclk==0 || (introclk>=72 && dmapmsgclk==0))
	{
        clear_bitmap(msg_bg_display_buf);
        clear_bitmap(msg_txt_display_buf);
	}
        
    clear_bitmap(msg_bg_display_buf);
    set_clip_state(msg_bg_display_buf, 1);
	clear_bitmap(msg_portrait_display_buf);
    set_clip_state(msg_portrait_display_buf, 1);
    clear_bitmap(msg_txt_display_buf);
    set_clip_state(msg_txt_display_buf, 1);
    clear_bitmap(msg_txt_bmp_buf);
    clear_bitmap(msg_menu_bmp_buf);
    clear_bitmap(msg_bg_bmp_buf);
    clear_bitmap(msg_portrait_bmp_buf);
    msgclk=msgpos=msgptr=0;
    msgspace=true;
    msg_w=MsgStrings[msgstr].w;
    msg_h=MsgStrings[msgstr].h;
    msg_xpos=MsgStrings[msgstr].x;
    msg_ypos=MsgStrings[msgstr].y;
	prt_tile=MsgStrings[msgstr].portrait_tile;
	prt_cset=MsgStrings[msgstr].portrait_cset;
	prt_x=MsgStrings[msgstr].portrait_x;
	prt_y=MsgStrings[msgstr].portrait_y;
	prt_tw=MsgStrings[msgstr].portrait_tw;
	prt_th=MsgStrings[msgstr].portrait_th;
	msg_shdtype=MsgStrings[msgstr].shadow_type;
	msg_shdcol=MsgStrings[msgstr].shadow_color;
    
    msg_bg(MsgStrings[msgstr]);
    msg_prt();
    
	int16_t old_margins[4] = {8,0,8,-8};
	int16_t const* copy_from = get_qr(qr_OLD_STRING_EDITOR_MARGINS) ? old_margins : MsgStrings[msgstr].margins;
	for(auto q = 0; q < 4; ++q)
		msg_margins[q] = copy_from[q];
    cursor_x=msg_margins[left];
    cursor_y=msg_margins[up];
}

// Called to make a message disappear
void dismissmsg()
{
    linkedmsgclk=0;
    msgstr = msgclk=msgpos=msgptr=0;
    cursor_x=0;
    cursor_y=0;
	prt_tile=0;
    msg_onscreen = msg_active = false;
    //Hero.finishedmsg(); //Not possible?
    clear_bitmap(msg_bg_display_buf);
    set_clip_state(msg_bg_display_buf, 1);
    clear_bitmap(msg_txt_display_buf);
    set_clip_state(msg_txt_display_buf, 1);
    clear_bitmap(msg_portrait_display_buf);
    set_clip_state(msg_portrait_display_buf, 1);
	clr_msg_data();
}

void dointro()
{
    if(game->visited[currdmap]!=1 || (DMaps[currdmap].flags&dmfALWAYSMSG)!=0)
    {
		if(get_qr(qr_OLD_DMAP_INTRO_STRINGS))
		{
			dmapmsgclk = 0;
			game->visited[currdmap] = 1;
			introclk = intropos = 0;
		}
		else
		{
			if(DMaps[currdmap].intro_string_id)
				donewmsg(DMaps[currdmap].intro_string_id);
			game->visited[currdmap] = 1;
		}
    }
    else
    {
        introclk=intropos=72;
    }
}

bool bad_version(int32_t version)
{
    // minimum zquest version allowed for any quest file
    if(version < 0x183)
        return true;
        
    return false;
}

extern char *weapon_string[];
extern char *item_string[];
extern char *sfx_string[];
extern char *guy_string[];


/**********************************/
/******* Other Source Files *******/
/**********************************/

bool get_debug()
{
    //return false; //remove later
    return __debug;
}

void set_debug(bool d)
{
    __debug=d;
    return;
}

void hit_close_button()
{
    close_button_quit=true;
    return;
}



// Yay, more extern globals.
extern ScriptType curScriptType;
extern word curScriptNum;

void Z_eventlog(const char *format,...)
{
    if(get_qr(qr_LOG) || DEVLEVEL > 0)
    {
        char buf[2048];
        
        va_list ap;
        va_start(ap, format);
        vsnprintf(buf, 2048, format, ap);
        va_end(ap);
        al_trace("%s",buf);
        
		if ( console_enabled ) {zscript_coloured_console.cprintf((CConsoleLoggerEx::COLOR_GREEN | CConsoleLoggerEx::COLOR_RED | CConsoleLoggerEx::COLOR_INTENSITY | 
			CConsoleLoggerEx::COLOR_BACKGROUND_BLACK),"%s",buf); }
    }
}

void Z_scripterrlog(const char * const format,...)
{
    if(get_qr(qr_SCRIPTERRLOG) || DEVLEVEL > 0)
    {
        FFCore.TraceScriptIDs();
		
        char buf[2048];
        
        va_list ap;
        va_start(ap, format);
        vsnprintf(buf, 2048, format, ap);
        va_end(ap);
        al_trace("%s",buf);
        
		if ( console_enabled ) 
		{
			zscript_coloured_console.cprintf((CConsoleLoggerEx::COLOR_RED | CConsoleLoggerEx::COLOR_INTENSITY | 
				CConsoleLoggerEx::COLOR_BACKGROUND_BLACK),"%s",buf);
		}
    }
}

#include "sprite.h"
movingblock mblock2;                                        //mblock[4]?
portal mirror_portal;

sprite_list  guys, items, Ewpns, Lwpns, chainlinks, decorations, portals;
particle_list particles;

#include "zc/zc_custom.h"
#include "zc/hero.h"
HeroClass   Hero;

#include "zc/maps.h"
#include "subscr.h"
#include "zc/guys.h"

#include "zc/title.h"
#include "zc/ending.h"

#include "zc/zc_sys.h"

// Wait... this is only used by ffscript.cpp!?
void addLwpn(int32_t x,int32_t y,int32_t z,int32_t id,int32_t type,int32_t power,int32_t dir, int32_t parentid)
{
    Lwpns.add(new weapon((zfix)x,(zfix)y,(zfix)z,id,type,power,dir,-1,parentid));
}


void addLwpnEx(int32_t x,int32_t y,int32_t z,int32_t id,int32_t type,int32_t power,int32_t dir, int32_t parentitem, int32_t parentid, byte script_gen)
{
	//weapon::weapon(zfix X,zfix Y,zfix Z,int32_t Id,int32_t Type,int32_t pow,int32_t Dir, int32_t Parentitem, int32_t prntid, byte script_gen, bool isDummy)
    Lwpns.add(new weapon((zfix)x,(zfix)y,(zfix)z,id,type,power,dir,parentitem,parentid,false,1));
	
}

void ALLOFF(bool messagesToo, bool decorationsToo, bool force)
{
    if(messagesToo)
    {
        clear_bitmap(msg_bg_display_buf);
        set_clip_state(msg_bg_display_buf, 1);
        clear_bitmap(msg_txt_display_buf);
        set_clip_state(msg_txt_display_buf, 1);
        clear_bitmap(msg_portrait_display_buf);
        set_clip_state(msg_portrait_display_buf, 1);
    }
    
    clear_bitmap(pricesdisplaybuf);
    set_clip_state(pricesdisplaybuf, 1);
    
    if(items.idCount(iPile))
    {
        loadlvlpal(DMaps[currdmap].color);
    }
    
    items.clear(force);
    guys.clear(force);
    Lwpns.clear(force);
    Ewpns.clear(force);
    chainlinks.clear(force);
	mirror_portal.clear();
	portals.clear(force);
    clearScriptHelperData();
    
    if(decorationsToo)
        decorations.clear(force);
        
    particles.clear();
    
    if(Hero.getDivineProtectionShieldClk())
    {
        Hero.setDivineProtectionShieldClk(Hero.getDivineProtectionShieldClk());
    }
    
    Hero.resetflags(false);
    Hero.reset_hookshot();
    linkedmsgclk=0;
    add_asparkle=0;
    add_bsparkle=0;
    add_df1asparkle=false;
    add_df1bsparkle=false;
    add_nl1asparkle=false;
    add_nl1bsparkle=false;
    add_nl2asparkle=false;
    add_nl2bsparkle=false;
    //  for(int32_t i=0; i<1; i++)
    mblock2.clk=0;
    dismissmsg();
    fadeclk=-1;
    introclk=intropos=72;
    
    lensclk = 0;
    lensid=-1;
    drawguys=true;
    
    if(watch && !cheat_superman)
    {
        Hero.setClock(false);
    }
    
    //  if(watch)
    //    Hero.setClock(false);
    watch=freeze_guys=loaded_guys=loaded_enemies=blockpath=false;
    stop_sfx(WAV_BRANG);
    
    for(int32_t i=0; i<176; i++)
        guygrid[i]=0;

    for(int32_t i=0; i<MAXFFCS; i++)
        guygridffc[i]=0;
        
    sle_clk=0;
	mblock2.clear();
    fairy_cnt=0;
    
    if(usebombpal)
    {
        memcpy(RAMpal, tempbombpal, PAL_SIZE*sizeof(RGB));
        refreshpal=true;
        usebombpal=false;
    }
}
void centerHero()
{
    Hero.setX(120);
    Hero.setY(80);
}
zfix  HeroX()
{
    return Hero.getX();
}
zfix  HeroY()
{
    return Hero.getY();
}
zfix  HeroZ()
{
    return Hero.getZ();
}
zfix  HeroFakeZ()
{
    return Hero.getFakeZ();
}
int32_t  HeroHClk()
{
    return Hero.getHClk();
}
int32_t  HeroAction()
{
    return Hero.getAction();
}
int32_t  HeroCharged()
{
    return Hero.isCharged();
}
int32_t  HeroDivineProtectionShieldClk()
{
    return Hero.getDivineProtectionShieldClk();
}
int32_t  HeroHoverClk()
{
    return Hero.getHoverClk();
}
int32_t  HeroSwordClk()
{
    return Hero.getSwordClk();
}
int32_t  HeroItemClk()
{
    return Hero.getItemClk();
}
void setSwordClk(int32_t newclk)
{
    Hero.setSwordClk(newclk);
}
void setItemClk(int32_t newclk)
{
    Hero.setItemClk(newclk);
}
int32_t  HeroLStep()
{
    return Hero.getLStep();
}
void HeroCheckItems(int32_t index)
{
    Hero.checkitems(index);
}
byte HeroGetDontDraw()
{
    return Hero.getDontDraw();
}
zfix  GuyX(int32_t j)
{
    return guys.getX(j);
}
zfix  GuyY(int32_t j)
{
    return guys.getY(j);
}
int32_t  GuyID(int32_t j)
{
    return guys.getID(j);
}
int32_t  GuyMisc(int32_t j)
{
    return guys.getMisc(j);
}
bool  GuySuperman(int32_t j)
{
    if((j>=guys.Count())||(j<0))
    {
        return true;
    }
    
    return ((enemy*)guys.spr(j))->superman !=0;
}

int32_t  GuyCount()
{
    return guys.Count();
}
void StunGuy(int32_t j,int32_t stun)
{
    if(stun<=0) return;
    
    if(((enemy*)guys.spr(j))->z==0 && ((enemy*)guys.spr(j))->fakez==0 && canfall(((enemy*)guys.spr(j))->id))
    {
        ((enemy*)guys.spr(j))->stunclk=zc_min(360,stun*4);
        ((enemy*)guys.spr(j))->fall=-zc_min(FEATHERJUMP,(stun*8)+zc_oldrand()%5);
    }
    else if(((enemy*)guys.spr(j))->z==0 && ((enemy*)guys.spr(j))->fakez==0 && ((enemy*)guys.spr(j))->family == eeLEV && get_qr(qr_QUAKE_STUNS_LEEVERS))
    {
        ((enemy*)guys.spr(j))->stunclk=zc_min(360,stun*4);
    }
}

zfix HeroModifiedX()
{
    return Hero.getModifiedX();
}
zfix HeroModifiedY()
{
    return Hero.getModifiedY();
}
int32_t HeroDir()
{
    return Hero.getDir();
}
void add_grenade(int32_t wx, int32_t wy, int32_t wz, int32_t size, int32_t parentid)
{
    if(size)
    {
        Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wSBomb,0,16*game->get_hero_dmgmult(),HeroDir(),
                             -1, parentid));
        Lwpns.spr(Lwpns.Count()-1)->id=wSBomb;
    }
    else
    {
        Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wBomb,0,4*game->get_hero_dmgmult(),HeroDir(),
                             -1, parentid));
        Lwpns.spr(Lwpns.Count()-1)->id=wBomb;
    }
    
    Lwpns.spr(Lwpns.Count()-1)->clk=48;
    Lwpns.spr(Lwpns.Count()-1)->misc=50;
}

zfix distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2)

{
    return (zfix)sqrt(pow((double)abs(x1-x2),2)+pow((double)abs(y1-y2),2));
}

bool getClock()
{
    return Hero.getClock();
}
void setClock(bool state)
{
    Hero.setClock(state);
}
void CatchBrang()
{
    Hero.Catch();
}

/**************************/
/***** Main Game Code *****/
/**************************/

int8_t smart_vercmp(char const* a, char const* b)
{
	// TODO: Attempting to make this not overflow given version
	// string data may not be null terminated.
	// I did my best here with the first two loops, but I can't understand the
	// rest of this function to know if there is still a problem.
	// Should probably rewrite this to use regular std::string splitting.
	int VERSION_LEN = 9;

	for(int q = 0; q < VERSION_LEN && a[q]; ++q)
	{
		if((a[q] >= '0' && a[q] <= '9') || a[q] == '.')
			continue;
		return strcmp(a, b);
	}
	for(int q = 0; q < VERSION_LEN && b[q]; ++q)
	{
		if((b[q] >= '0' && b[q] <= '9') || b[q] == '.')
			continue;
		return strcmp(a, b);
	}
	char *cpya = (char*)malloc(strlen(a)+1), *cpyb = (char*)malloc(strlen(b)+1);
	strcpy(cpya, a); strcpy(cpyb, b);
	char *ptra = cpya, *ptrb = cpyb, *tmpa = cpya, *tmpb = cpyb;
	std::vector<int32_t> avec, bvec;
	while(*(ptra++))
	{
		if(*ptra == '.')
		{
			*ptra = 0;
			avec.push_back(atoi(tmpa));
			tmpa = ++ptra;
			if(!*ptra) break;
		}
		if(!*ptra)
		{
			avec.push_back(atoi(tmpa));
			break;
		}
	}
	while(*(ptrb++))
	{
		if(*ptrb == '.')
		{
			*ptrb = 0;
			bvec.push_back(atoi(tmpb));
			tmpb = ++ptrb;
			if(!*ptrb) break;
		}
		if(!*ptrb)
		{
			avec.push_back(atoi(tmpb));
			break;
		}
	}
	free(cpya); free(cpyb);
	while(avec.size() < bvec.size())
		avec.push_back(0);
	while(bvec.size() < avec.size())
		bvec.push_back(0);
	for(uint32_t q = 0; q < avec.size(); ++q)
	{
		if(avec.at(q) < bvec.at(q))
			return -1;
		if(avec.at(q) > bvec.at(q))
			return 1;
	}
	return 0;
}

int32_t load_quest(gamedata *g, bool report, byte printmetadata)
{
	chop_path(qstpath);
	int32_t ret = 0;
	int32_t qst_num = g->get_quest();

	if (g->header.qstpath.empty() && !qst_num)
	{
		// We will open the file select dialog to show the quest directory.
		return 0;
	}

	if (g->header.qstpath.empty() && qst_num)
	{
		char* cwd = al_get_current_directory();
		fs::path path;
		if      (qst_num == 1) path = fs::path(cwd) / "quests/Z1 Recreations/classic_1st.qst";
		else if (qst_num == 2) path = fs::path(cwd) / "quests/Z1 Recreations/classic_2nd.qst";
		else if (qst_num == 3) path = fs::path(cwd) / "quests/Old Contest Winners/classic_3rd.qst";
		else if (qst_num == 4) path = fs::path(cwd) / "quests/Z1 Recreations/classic_4th.qst";
		else if (qst_num == 5) path = fs::path(cwd) / "quests/Old Contest Winners/classic_5th.qst";
		else return qe_no_qst;
		al_free(cwd);
		sprintf(qstpath, "%s", path.string().c_str());
		g->header.qstpath = qstpath;
	}

	if (!g->header.qstpath.empty())
	{
		if(is_relative_filename(g->get_qstpath()))
		{
			auto qstpath_fs = fs::path(qstdir) / fs::path(g->get_qstpath());
			sprintf(qstpath, "%s", qstpath_fs.string().c_str());
		}
		else
		{
			sprintf(qstpath,"%s", g->get_qstpath());
		}

		// ZC paths are retarded.
		// This is just an awful hack, and generally some of the worst code ever written, but it only ever gets run
		// when there would be a "file not found error"... (it's easier than dealing with everything else. Sue me.)
		//
		// *side note*
		// It does have a few perks though: You can now move around entire folders and sub-folders from one ZC directory to the next,
		// and we can find them! You can even put all the ZC quests into different sub-directories and share save files, etc..
		// ~Gleeok
		if(!exists(qstpath))
		{
			Z_error("File not found \"%s\". Searching...\n", qstpath);

			if(exists(g->get_qstpath())) //not found? -try this place first:
			{
				sprintf(qstpath,"%s", g->get_qstpath());
				Z_error("Set quest path to \"%s\".\n", qstpath);
			}
			else // Howsabout in here?
			{
				std::string gQstPath = g->get_qstpath();
				size_t bs1 = 0;
				size_t bs2 = std::string::npos;

				// Keep searching through every parent directory as if it was the current one.
				while(bs1 != std::string::npos || bs2 != std::string::npos)
				{
					bs1 = gQstPath.find_first_of('/');
					if(bs1 != std::string::npos)
					{
						gQstPath = gQstPath.substr(bs1 + 1, std::string::npos);
					}
					else
					{
						bs2 = gQstPath.find_first_of('\\');
						if(bs2 != std::string::npos)
							gQstPath = gQstPath.substr(bs2 + 1, std::string::npos);
					}

					if(exists(gQstPath.c_str())) //Quick! Try it now!
					{
						sprintf(qstpath,"%s", gQstPath.c_str());
						Z_error("Set quest path to \"%s\".\n", qstpath);
						break;
					}
					else //Still no dice eh?
					{
						char cwdbuf[260];
						memset(cwdbuf,0,260*sizeof(char));
						getcwd(cwdbuf, 260);

						std::string path = cwdbuf;
						std::string fn;

						if(path.size() != 0 && 
							!(*(path.end()-1)=='/' || *(path.end()-1)=='\\')
							)
						{
							path += '/';
						}

						fn = path + gQstPath;

						if(exists(fn.c_str())) //Last chance for hookers and blackjack truck stop
						{
							sprintf(qstpath,"%s", fn.c_str());
							Z_error("Set quest path to \"%s\".\n", qstpath);
							break;
						}
					}
				} //while
			}
		}//end hack
	}

	if (replay_is_active() && !testingqst_name.empty())
	{
		sprintf(qstpath, "%s", testingqst_name.c_str());
	}
	
	if(!exists(qstpath)) ret = qe_notfound;
	
	if(!ret)
	{
		//setPackfilePassword(datapwd);
		byte skip_flags[4];
		
		for(int32_t i=0; i<4; ++i)
		{
			skip_flags[i]=0;
		}
		
		ret = loadquest(qstpath,&QHeader,&QMisc,tunes+ZC_MIDI_COUNT,false,skip_flags,printmetadata,report,qst_num);
		
		const char* title = g->header.title.c_str();
		if(!title[0] || g->get_hasplayed() == 0)
		{
			g->header.title = QHeader.title;
			strncpy(g->version,QHeader.version,sizeof(g->version));
			// Put the fixed-length header version field into a safer string.
			strncpy(header_version_nul_term,QHeader.version,sizeof(QHeader.version));
		}
		else
		{
			if(!ret && strncmp(title,QHeader.title,sizeof(QHeader.title)))
			{
				ret = qe_match;
			}
		}
		
		if(QHeader.minver[0])
		{
			if(smart_vercmp(g->version,QHeader.minver) < 0)
				ret = qe_minver;
		}
	}
	if(ret && report)
	{
		if (is_ci())
		{
			abort();
		}

		enter_sys_pal();
		std::ostringstream oss;
		if(ret == qe_no_qst)
		{
			oss << qst_error[ret] << ". Press the 'A' button twice to select a custom quest,"
				"\nor load a module that has a default " << ordinal(qst_num+1) << " quest.";
		}
		else
		{
			oss << "Error loading " << get_filename(qstpath)
				<< ":\n" << qst_error[ret] << "\n" << qstpath;
		}
		InfoDialog("File Error",oss.str()).show();
		
		if(standalone_mode)
		{
			abort();
		}
		
		exit_sys_pal();
	}

	return ret;
}

std::string create_replay_path_for_save(const gamedata_header& header)
{
	std::filesystem::path replay_file_dir = zc_get_config("zeldadx", "replay_file_dir", "replays/");
	std::filesystem::create_directory(replay_file_dir);
	std::string filename_prefix = fmt::format("{}-{}", header.title, header.name);
	sanitize(filename_prefix);
	return create_new_file_path(replay_file_dir, filename_prefix, REPLAY_EXTENSION).string();
}

void init_dmap()
{
    // readjust disabled items; could also do dmap-specific scripts here
    for(int32_t i=0; i<MAXITEMS; i++)
    {
        game->items_off[i] &= (~1); // disable last bit - this one is set by dmap
        game->items_off[i] |= DMaps[currdmap].disableditems[i]; // and reset if required
    }
    
    flushItemCache();
    // also update subscreens
    update_subscreens();
    verifyBothWeapons();
    
    return;
}

// Sets globals to their default values.
void init_game_vars(bool is_cont_game = false)
{
	new_subscreen_active = new_subscreen_passive = new_subscreen_overlay = nullptr;

	// There are many replay tests that were made when these globals were not being reset
	// on continue. Let's not break them.
	if (is_cont_game && !(replay_version_check(32) || replay_get_meta_str("qst") == "grassland_attack.qst"))
		return;

	// Don't reset cheats on continue.
	if (!is_cont_game)
	{
		cheat=0;
		show_layer_0=show_layer_1=show_layer_2=show_layer_3=show_layer_4=show_layer_5=show_layer_6=true;
		show_layer_over=show_layer_push=show_sprites=show_ffcs=true;
		cheat_superman=cheats_execute_light=cheats_execute_goto=show_walkflags=show_effectflags=show_ff_scripts=show_hitboxes=false;
	}

	// Various things use the frame counter to do random stuff (ex: runDrunkRNG).
	// We only bother setting it to 0 here so that recordings will play back the
	// same way, even if manually started in the ZC UI.
	frame = 0;

	new_sub_indexes[sstACTIVE] = -1;
	loadside = 0;
	view_map_show_mode = 3;
	for ( int32_t q = 0; q < 256; q++ ) runningItemScripts[q] = 0; //Clear scripts that were running before. 
	draw_screen_clip_rect_x1=0; //Prevent the ending sequence from carrying over through 'Reset System' -Z
	draw_screen_clip_rect_x2=255;
	draw_screen_clip_rect_y1=0;
	draw_screen_clip_rect_y2=223;	
	didpit=false;
	Hero.unfreeze();
	Hero.reset_hookshot();
	Hero.reset_ladder();
	linkedmsgclk=0;
	mblock2.clear();
	add_asparkle=0;
	add_bsparkle=0;
	add_df1asparkle=false;
	add_df1bsparkle=false;
	add_nl1asparkle=false;
	add_nl1bsparkle=false;
	add_nl2asparkle=false;
	add_nl2bsparkle=false;
	gofast=false;
	wavy=quakeclk=0;
	if (zscriptDrawingRenderTarget) zscriptDrawingRenderTarget->SetCurrentRenderTarget(-1);
	new_sub_indexes[sstACTIVE] = new_sub_indexes[sstPASSIVE] = new_sub_indexes[sstOVERLAY] = -1;
	script_hero_sprite = 0; 
	script_hero_flip = -1; 
	script_hero_cset = -1;
	room_is_dark=darkroom=naturaldark=false;
	sle_x=sle_y=newscr_clk=opendoors=0;
	Bwpn=Awpn=Xwpn=Ywpn=-1;
	FFCore.kb_typing_mode = false;
	activated_timed_warp=false;
	clockclk=0;
	fadeclk=-1;
	whistleclk=-1;
	newscr_clk=0;
	subscr_item_clk=0;
	wavy=quakeclk=0;
	show_subscreen_dmap_dots=true;
	show_subscreen_items=true;
	show_subscreen_numbers=true;
	show_subscreen_life=true;
	for(int32_t x = 0; x < MAXITEMS; x++)
	{
		lens_hint_item[x][0]=0;
		lens_hint_item[x][1]=0;
	}
	for(int32_t x = 0; x < MAXWPNS; x++)
	{
		lens_hint_weapon[x][0]=0;
		lens_hint_weapon[x][1]=0;
	}
	for(int32_t i=0; i<6; i++)
	{
		visited[i]=-1;
	}
}

int32_t init_game()
{
	if(clearConsoleOnLoad)
		clearConsole();
	GameLoaded = true;
	init_game_vars();

	//Copy saved data to RAM data (but not global arrays)
	// TODO: this is called twice (copying lots of data) when selecting a save slot. See title.cpp
	if (!saves_select(saves_current_selection()))
	{
		Z_error_fatal("Failed to load save file\n");
	}
	bool firstplay = (game->get_hasplayed() == 0);

	// The following code is the setup for recording a save file, enabled via "replay_new_saves" config.
	// It allows users to start a recording on a new quest, or to continue a recording on an existing quest.
	// This block runs _only_ for recordings associated with a save file. See `-replay` above for how recordings
	// are handled for `-test` mode.
	bool replay_new_saves = zc_get_config("zeldadx", "replay_new_saves", false);
	if (!zqtesting_mode && (replay_new_saves || !firstplay) && !replay_is_active())
	{
		if (firstplay && replay_new_saves)
		{
			std::string replay_path = create_replay_path_for_save(game->header);
			enter_sys_pal();
			if (jwin_alert("Recording",
				"You are about to create a new recording at:",
				relativize_path(replay_path).c_str(),
				"Do you wish to record this save file?",
				"Yes","No",13,27,get_zc_font(font_lfont))==1)
			{
				game->header.replay_file = replay_path;
				replay_start(ReplayMode::Record, replay_path, -1);
				replay_set_debug(replay_debug);
				replay_set_sync_rng(true);
				replay_set_meta("qst", relativize_path(game->header.qstpath));
				replay_set_meta("name", game->get_name());
				replay_save();
			}
			exit_sys_pal();
		}
		else if (!firstplay && !game->header.replay_file.empty())
		{
			if (!std::filesystem::exists(game->header.replay_file))
			{
				std::string msg = fmt::format("Replay file {} does not exist. Cannot continue recording.",
					game->header.replay_file);
				enter_sys_pal();
				jwin_alert("Recording",msg.c_str(),NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
				exit_sys_pal();
			}
			else
			{
				replay_continue(game->header.replay_file);
			}
		}
	}

	replay_step_comment("init_game");
	replay_forget_input();

	rti_game.remove_children();

	int initial_seed = time(0);
	replay_register_rng(zc_get_default_rand());
	replay_register_rng(&drunk_rng);
	zc_game_srand(initial_seed);
	zc_game_srand(initial_seed, &drunk_rng);

	onload_gswitch_timers();
	flushItemCache();
	ResetSaveScreenSettings();
	
	int32_t ret = load_quest(game);
	if(ret != qe_OK)
	{
		saves_unselect();
		Quit = qERROR;
		GameLoaded = false;
		return 1;
	}

	if (zasm_optimize_enabled() && (get_flag_bool("-test-bisect").has_value() || is_ci()))
		zasm_optimize();

	FFCore.init();
	FFCore.user_bitmaps_init();
	FFCore.user_files_init();
	FFCore.user_dirs_init();
	FFCore.user_stacks_init();
	FFCore.user_paldata_init();
	FFCore.user_websockets_init();
	script_init_name_to_slot_index_maps();

	if (testingqst_init_data.size())
	{
		std::string error;
		zinitdata* new_init = apply_init_data_delta(&zinit, testingqst_init_data, error);
		if (new_init)
		{
			zinit = *new_init;
			resetItems(game, new_init, false);
			ringcolor(false);
			delete new_init;
		}
		else
		{
			Z_error_fatal("error applying init data delta: %s\n", error.c_str());
		}
	}

	jit_startup();

	if (!firstplay) load_genscript(*game);
	genscript_timing = SCR_TIMING_START_FRAME;
	timeExitAllGenscript(GENSCR_ST_RELOAD);
	if(firstplay) load_genscript(zinit);
	countGenScripts();
	
	ResetSaveScreenSettings();
	
	maxcheat = game->get_cheat();
	
	//setPackfilePassword(NULL);
	
	bool setcheat=false;
	if(use_testingst_start)
	{
		setcheat=true;
		zprint2("Cheats enabled: Test Mode active\n");
	}
	else if(devpwd())
	{
		setcheat=true;
		zprint2("Cheats enabled: Dev Override active\n");
	}
	else if(check_keyfiles(qstpath, {KEYFILE_MASTER,KEYFILE_CHEAT}, &QHeader))
		setcheat = true;
	
	if(setcheat)
	{
		cheat = 4;
		maxcheat = 4;
	}
	else
	{
		cheat = 0;
	}
	
	//Calculate the quest's script-file-storage path -Em
	{
		memset(qst_files_path, 0, sizeof(qst_files_path));
		string str(qstpath);
		size_t pos = str.find_last_of("/\\");
		if(pos==string::npos) pos=0;
		else ++pos;
		size_t dotpos = str.find_last_of(".");
		sprintf(qst_files_path,"./Files/%s",str.substr(pos, dotpos-pos).c_str());
		regulate_path(qst_files_path);
	}
	
	BSZ = get_qr(qr_BSZELDA)!=0;
	
	COOLSCROLL = (get_qr(qr_COOLSCROLL)!=0 ? 1 : 0) |
				 (get_qr(qr_OVALWIPE)!=0 ? 2 : 0) |
				 (get_qr(qr_TRIANGLEWIPE)!=0 ? 4 : 0) |
				 (get_qr(qr_SMASWIPE)!=0 ? 8 : 0) |
				 (get_qr(qr_FADEBLACKWIPE)!=0 ? 16 : 0);
	identifyCFEnemies();

	if(firstplay)
	{
		// Without this, during replay assert there is a difference (for rings only?) on playback.
		// Seems like the `game` field should be in a known, zero'd out state, but on the original
		// playthrough it isn't. I guess it's related to whatever happened to be in saves[currgame] which
		// was copied above.
		for (int i = 0; i < MAXITEMS; i++)
			game->item[i] = false;

		if (!use_testingst_start)
			game->set_continue_dmap(zinit.start_dmap);
		resetItems(game,&zinit,true);
		if ( FFCore.getQuestHeaderInfo(vZelda) < 0x190 )
		{
			game->set_maxbombs(8);
		}
	}
	
	timeExitAllGenscript(GENSCR_ST_CHANGE_DMAP);
	timeExitAllGenscript(GENSCR_ST_CHANGE_LEVEL);
	previous_DMap = currdmap = warpscr = worldscr=game->get_continue_dmap();
	init_dmap();
	
	if(game->get_continue_scrn() >= 0x80)
	{
		//if ((DMaps[currdmap].type&dmfTYPE)==dmOVERW || QHeader.zelda_version <= 0x190)
		if((DMaps[currdmap].type&dmfTYPE)==dmOVERW)
		{
			homescr = currscr = DMaps[currdmap].cont;
		}
		else
		{
			homescr = currscr = DMaps[currdmap].cont + DMaps[currdmap].xoff;
		}
	}
	else
	{
		homescr = currscr = game->get_continue_scrn();
	}
	
	lastentrance = currscr;
	game->set_continue_scrn(lastentrance);
	lastentrance_dmap = currdmap;
	currmap = DMaps[currdmap].map;
	dlevel = DMaps[currdmap].level;
	
	if(currscr < 0x80 && (DMaps[currdmap].flags&dmfVIEWMAP))
	{
		game->maps[(currmap*MAPSCRSNORMAL)+currscr] |= mVISITED;			  // mark as visited
	}
	
	game->lvlitems[9] &= ~liBOSS;
	
	ALLOFF(true,true,true);
	
	currcset=DMaps[currdmap].color;
	
	tmpscr[0].zero_memory();
	tmpscr[1].zero_memory();
	//clear initialise dmap script
	FFCore.reset_script_engine_data(ScriptType::DMap);
	//Script-related nonsense
	script_drawing_commands.Clear();

	init_script_objects();
	initZScriptArrayRAM(firstplay);
	initZScriptGlobalRAM();
	FFCore.initZScriptHeroScripts();
	FFCore.initZScriptDMapScripts();
	FFCore.initZScriptScriptedActiveSubscreen();
	FFCore.initZScriptItemScripts();

	if (!get_qr(qr_OLD_SCRIPT_VOLUME))
	{
		FFCore.usr_sfx_volume = 10000 * 100;
		FFCore.usr_music_volume = 10000 * 100;
	}
	
	//show quest metadata when loading it
	print_quest_metadata(QHeader, qstpath, byte(game->get_quest()-1));
	
	//FFCore.init(); ///Initialise new ffscript engine core. 
	if(!firstplay && !get_qr(qr_OLD_INIT_SCRIPT_TIMING))
	{
		ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_ONSAVELOAD, GLOBAL_SCRIPT_ONSAVELOAD); //Do this after global arrays have been loaded
		FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_ONSAVELOAD);
	}
	//loadscr(0,currscr,up);
	loadscr(0,currdmap,currscr,-1,false);
	
	putscr(scrollbuf,0,0,&tmpscr[0]);
	putscrdoors(scrollbuf,0,0,&tmpscr[0]);
	
	//preloaded freeform combos
	//ffscript_engine(true); Can't do this here! Global arrays haven't been allocated yet... ~Joe
	
	Hero.init();
	if (use_testingst_start
		&& currscr == testingqst_screen
		&& currdmap == testingqst_dmap)
	{
		if (tmpscr->warpreturnx[testingqst_retsqr] != 0 || tmpscr->warpreturny[testingqst_retsqr] != 0)
		{
			Hero.setX(tmpscr->warpreturnx[testingqst_retsqr]);
			Hero.setY(tmpscr->warpreturny[testingqst_retsqr]);
		}
		else
		{
			Hero.setX(16 * 8);
			Hero.setY(16 * 5);
		}
	}
	if(DMaps[currdmap].flags&dmfBUNNYIFNOPEARL)
	{
		int32_t itemid = current_item_id(itype_pearl);
		if(itemid < 0)
		{
			Hero.setBunnyClock(-1);
		}
	}
	
	if(firstplay) //Move up here, so that arrays are initialised before we run Hero's Init script.
	{
		game->screen_d.clear();
		if(!get_qr(qr_OLD_INIT_SCRIPT_TIMING))
		{
			ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_INIT, GLOBAL_SCRIPT_INIT);
			FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_INIT); //Deallocate LOCAL arrays declared in the init script. This function does NOT deallocate global arrays.
		}
		if ( FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(ScriptType::Player, SCRIPT_PLAYER_INIT); //We run this here so that the user can set up custom
									//positional data, sprites, tiles, csets, invisibility states, and the like.
			FFCore.deallocateAllScriptOwned(ScriptType::Player, SCRIPT_PLAYER_INIT);
		}
		FFCore.initZScriptHeroScripts(); //Clear the stack and the refinfo data to be ready for Hero's active script.
		Hero.set_respawn_point(); //This should be after the init script, so that Hero->X and Hero->Y set by the script
						//are properly set by the engine.
	}
	Hero.resetflags(true); //This should probably occur after running Hero's init script. 

	BITMAP* gui_pal_bitmap = load_bitmap("assets/gui_pal.bmp", pal_gui);
	if (!gui_pal_bitmap)
		Z_error_fatal("Missing required file %s\n", "assets/gui_pal.bmp");
	copy_pal(pal_gui,RAMpal);
	destroy_bitmap(gui_pal_bitmap);

	loadfullpal();
	ringcolor(false);
	loadlvlpal(DMaps[currdmap].color);
	lighting(false,true);
	
	if(firstplay)
	{
		game->set_life(zinit.counter[crLIFE]);
	}
	else
	{
		if(game->get_cont_percent())
		{
			if(game->get_maxlife()%game->get_hp_per_heart()==0)
			{
				auto life = (game->get_maxlife()*game->get_cont_hearts()/100);
				life -= life % game->get_hp_per_heart();
				game->set_life(life);
			}
			else
				game->set_life(game->get_maxlife()*game->get_cont_hearts()/100);
		}
		else
		{
			game->set_life(game->get_cont_hearts());
		}
	}
	
	if(firstplay)
		game->set_hasplayed(1);
	
	if(firstplay)
	{
		if(!get_qr(qr_OLD_SUBSCR) && new_subscreen_active)
		{
			game->awpn = new_subscreen_active->def_btns[0];
			game->bwpn = new_subscreen_active->def_btns[1];
			game->xwpn = new_subscreen_active->def_btns[2];
			game->ywpn = new_subscreen_active->def_btns[3];
			
			if((game->awpn&0xFF) >= new_subscreen_active->pages.size())
				game->awpn = 0xFF;
			if((game->bwpn&0xFF) >= new_subscreen_active->pages.size())
				game->bwpn = 0xFF;
			if((game->xwpn&0xFF) >= new_subscreen_active->pages.size())
				game->xwpn = 0xFF;
			if((game->ywpn&0xFF) >= new_subscreen_active->pages.size())
				game->ywpn = 0xFF;
		}
		else
		{
			game->awpn=game->bwpn=game->ywpn=game->xwpn=255;
		}
		game->forced_awpn = game->forced_bwpn =
			game->forced_xwpn = game->forced_ywpn = -1;
	}
		
	update_subscreens();
	
	refresh_subscr_items();
	
	//load the previous weapons -DD	
	
	bool usesaved = (game->get_quest() == 0xFF); //What was wrong with firstplay?
	int32_t apos = 0xFF, bpos = 0xFF, xpos = 0xFF, ypos = 0xFF;
	
	//Setup button items
	{
		bool use_x = get_qr(qr_SET_XBUTTON_ITEMS), use_y = get_qr(qr_SET_YBUTTON_ITEMS);
		if (new_subscreen_active)
		{
			if (get_qr(qr_OLD_SUBSCR))
			{
				SubscrPage& pg = new_subscreen_active->cur_page();
				if (use_x || use_y)
				{
					if (!get_qr(qr_SELECTAWPN))
					{
						Awpn = selectSword();
						bpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->bwpn : 0xFF);
						if (use_x)
							xpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->xwpn : 0xFF, bpos);
						if (use_y)
							ypos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->ywpn : 0xFF, bpos, xpos);
						directItemA = -1;
					}
					else
					{
						apos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->awpn : 0xFF);
						bpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->bwpn : 0xFF, apos);
						if (use_x)
							xpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->xwpn : 0xFF, apos, bpos);
						if (use_y)
							ypos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->ywpn : 0xFF, apos, bpos, xpos);

						Awpn = pg.get_item_pos(apos >> 8);
						directItemA = NEG_OR_MASK(Awpn, 0xFF);
					}

					game->awpn = apos;

					game->bwpn = bpos;
					Bwpn = pg.get_item_pos(bpos >> 8);
					directItemB = NEG_OR_MASK(Bwpn, 0xFF);

					game->xwpn = xpos;
					Xwpn = pg.get_item_pos(xpos >> 8);
					directItemX = NEG_OR_MASK(Xwpn, 0xFF);

					game->ywpn = ypos;
					Ywpn = pg.get_item_pos(ypos >> 8);
					directItemY = NEG_OR_MASK(Ywpn, 0xFF);

					animate_subscr_buttonitems();

					refresh_subscr_buttonitems();
				}
				else
				{
					if (!get_qr(qr_SELECTAWPN))
					{
						Awpn = selectSword();
						apos = 0xFF;
						bpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->bwpn : 0xFF);
						directItemA = -1;
					}
					else
					{
						apos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->awpn : 0xFF);
						bpos = pg.movepos_legacy(SEL_VERIFY_RIGHT, usesaved ? game->bwpn : 0xFF, apos);

						if (bpos == 0xFF)
						{
							bpos = apos;
							apos = 0xFF;
						}

						Awpn = pg.get_item_pos(apos >> 8);
						directItemA = NEG_OR_MASK(Awpn, 0xFF);
					}

					game->awpn = apos;
					game->bwpn = bpos;
					Bwpn = pg.get_item_pos(bpos >> 8);
					directItemB = NEG_OR_MASK(Bwpn, 0xFF);
					animate_subscr_buttonitems();

					refresh_subscr_buttonitems();
				}
			}
			else
			{
				if(get_qr(qr_SELECTAWPN))
				{
					Awpn = new_subscreen_active->get_item_pos(game->awpn);
					directItemA = NEG_OR_MASK(Awpn, 0xFF);
				}
				else
				{
					selectSword();
					directItemA = -1;
				}
				Bwpn = new_subscreen_active->get_item_pos(game->bwpn);
				directItemB = NEG_OR_MASK(Bwpn, 0xFF);
				Xwpn = new_subscreen_active->get_item_pos(game->xwpn);
				directItemX = NEG_OR_MASK(Xwpn, 0xFF);
				Ywpn = new_subscreen_active->get_item_pos(game->ywpn);
				directItemY = NEG_OR_MASK(Ywpn, 0xFF);
			}
		}
	}
	
	Playing=true;
	
	map_bkgsfx(true);
	
	//Run after Init/onSaveLoad, regardless of firstplay -V
	FFCore.runOnLaunchEngine();
	FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_ONLAUNCH);
	
	FFCore.runGenericPassiveEngine(SCR_TIMING_INIT);
	throwGenScriptEvent(GENSCR_EVENT_INIT);
	
	if(!get_qr(qr_FFCPRELOAD_BUGGED_LOAD)) ffscript_engine(true);
	
	
	if ( Hero.getDontDraw() < 2 ) { Hero.setDontDraw(0); }
	initZScriptGlobalScript(GLOBAL_SCRIPT_GAME); // before 'openscreen' incase FFCore.warpScriptCheck()
	openscreen();
	dointro();
	if(!(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE)))
	{
		loadguys();
	}

	newscr_clk = frame;
	
	if(isdungeon() && currdmap>0) // currdmap>0 is weird, but at least one quest (Mario's Insane Rampage) depends on it
	{
		Hero.stepforward(get_qr(qr_LTTPWALK) ? 11: 12, false);
	}
	
	if(!Quit)
	{
		if(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE))
		{
			Hero.ganon_intro();
		}
		else playLevelMusic();
	}

	 
	//2.53 timing
	if(get_qr(qr_OLD_INIT_SCRIPT_TIMING))
	{
		if(firstplay)
		{
			game->screen_d.clear();
			ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_INIT, GLOBAL_SCRIPT_INIT);
			if(!get_qr(qr_DO_NOT_DEALLOCATE_INIT_AND_SAVELOAD_ARRAYS) ) FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_INIT); //Deallocate LOCAL arrays declared in the init script. This function does NOT deallocate global arrays.
		}
		else
		{
			ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_ONSAVELOAD, GLOBAL_SCRIPT_ONSAVELOAD); //Do this after global arrays have been loaded
			if(!get_qr(qr_DO_NOT_DEALLOCATE_INIT_AND_SAVELOAD_ARRAYS) ) FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_ONSAVELOAD);
		}	
	}
	
	FFCore.initZScriptHeroScripts(); //Call again so we're set up for GLOBAL_SCRIPT_GAME
	FFCore.initZScriptDMapScripts(); //Call again so we're set up for GLOBAL_SCRIPT_GAME
	FFCore.initZScriptItemScripts(); //Call again so we're set up for GLOBAL_SCRIPT_GAME
	FFCore.initZScriptScriptedActiveSubscreen();
	if(get_qr(qr_FFCPRELOAD_BUGGED_LOAD)) ffscript_engine(true);  //Here is a much safer place...
	return 0;
}

int32_t cont_game()
{
	init_game_vars(true);
	replay_step_comment("cont_game");
	GameLoaded = true;
	timeExitAllGenscript(GENSCR_ST_CONTINUE);
	throwGenScriptEvent(GENSCR_EVENT_CONTINUE);
	//  introclk=intropos=msgclk=msgpos=dmapmsgclk=0;
	FFCore.init();
	onload_gswitch_timers();
	didpit=false;
	Hero.unfreeze();
	Hero.reset_hookshot();
	Hero.reset_ladder();
	linkedmsgclk=0;
	mblock2.clear();
	add_asparkle=0;
	add_bsparkle=0;
	add_df1asparkle=false;
	add_df1bsparkle=false;
	add_nl1asparkle=false;
	add_nl1bsparkle=false;
	add_nl2asparkle=false;
	add_nl2bsparkle=false;
	/*
	  if(DMaps[currdmap].cont >= 0x80)
	  {
	  homescr = currscr = DMaps[0].cont;
	  currdmap = warpscr = worldscr=0;
	  currmap = DMaps[0].map;
	  dlevel = DMaps[0].level;
	  }
	  */
	bool changedlevel = false;
	bool changeddmap = false;
	if(currdmap != lastentrance_dmap)
	{
		timeExitAllGenscript(GENSCR_ST_CHANGE_DMAP);
		changeddmap = true;
	}
	if(dlevel != DMaps[lastentrance_dmap].level)
	{
		timeExitAllGenscript(GENSCR_ST_CHANGE_LEVEL);
		changedlevel = true;
	}
	dlevel = DMaps[lastentrance_dmap].level;
	currdmap = lastentrance_dmap;
	if(changeddmap)
	{
		throwGenScriptEvent(GENSCR_EVENT_CHANGE_DMAP);
	}
	if(changedlevel)
	{
		throwGenScriptEvent(GENSCR_EVENT_CHANGE_LEVEL);
	}
	homescr = currscr = lastentrance;
	currmap = DMaps[currdmap].map;
	init_dmap();
	
	for(int32_t i=0; i<6; i++)
	{
		visited[i]=-1;
	}
	
	if(dlevel==0)
	{
		game->lvlitems[9]&=~liBOSS;
	}
	
	ALLOFF();
	whistleclk=-1;
	currcset=DMaps[currdmap].color;
	room_is_dark=darkroom=naturaldark=false;
	tmpscr[0].zero_memory();
	tmpscr[1].zero_memory();
	
//loadscr(0,currscr,up);
	loadscr(0,currdmap,currscr,-1,false);
	
	putscr(scrollbuf,0,0,&tmpscr[0]);
	putscrdoors(scrollbuf,0,0,&tmpscr[0]);
	script_drawing_commands.Clear();
	
	//preloaded freeform combos
	ffscript_engine(true);
	
	loadfullpal();
	ringcolor(false);
	loadlvlpal(DMaps[currdmap].color);
	lighting(false,true);
	Hero.init();
	if (use_testingst_start
		&& currscr == testingqst_screen
		&& currdmap == testingqst_dmap)
	{
		Hero.setX(tmpscr->warpreturnx[testingqst_retsqr]);
		Hero.setY(tmpscr->warpreturny[testingqst_retsqr]);
	}
	if(DMaps[currdmap].flags&dmfBUNNYIFNOPEARL)
	{
		int32_t itemid = current_item_id(itype_pearl);
		if(itemid < 0)
		{
			Hero.setBunnyClock(-1);
		}
	}
	
	wavy=quakeclk=0;
	
	if(game->get_cont_percent())
	{
		if(game->get_maxlife()%game->get_hp_per_heart()==0)
		{
			auto life = (game->get_maxlife()*game->get_cont_hearts()/100);
			life -= life % game->get_hp_per_heart();
			game->set_life(life);
		}
		else
			game->set_life(game->get_maxlife()*game->get_cont_hearts()/100);
	}
	else
	{
		game->set_life(game->get_cont_hearts());
	}
	
	initZScriptGlobalScript(GLOBAL_SCRIPT_ONCONTGAME);
	ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_ONCONTGAME, GLOBAL_SCRIPT_ONCONTGAME);	
	FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_ONCONTGAME);
	
	initZScriptGlobalScript(GLOBAL_SCRIPT_GAME);
	FFCore.initZScriptHeroScripts();
	FFCore.initZScriptDMapScripts();
	FFCore.initZScriptScriptedActiveSubscreen();
	FFCore.initZScriptItemScripts();

	if (!get_qr(qr_OLD_SCRIPT_VOLUME))
	{
		FFCore.usr_sfx_volume = 10000 * 100;
		FFCore.usr_music_volume = 10000 * 100;
	}

	update_subscreens();
	Playing=true;
	map_bkgsfx(true);
	openscreen();
	show_subscreen_numbers=true;
	show_subscreen_life=true;
	dointro();
	if(!(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE)))
	{
		loadguys();
	}

	
	if(!Quit)
	{
		//play_DmapMusic();
		if(!(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE))) playLevelMusic();
		
		if(isdungeon())
			Hero.stepforward(get_qr(qr_LTTPWALK)?11:12, false);
			
		newscr_clk=frame;
		activated_timed_warp=false;
		if(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE))
		{
			Hero.ganon_intro();
		}
	}
	return 0;
}

void restart_level()
{
	blackscr(16,true);
	map_bkgsfx(false);
	
	if(dlevel && !get_qr(qr_LEVEL_RESTART_CONT_POINT))
	{
		bool changedlevel = false;
		bool changeddmap = false;
		if(currdmap != lastentrance_dmap)
		{
			timeExitAllGenscript(GENSCR_ST_CHANGE_DMAP);
			changeddmap = true;
		}
		if(dlevel != DMaps[lastentrance_dmap].level)
		{
			timeExitAllGenscript(GENSCR_ST_CHANGE_LEVEL);
			changedlevel = true;
		}
		dlevel = DMaps[lastentrance_dmap].level;
		currdmap = lastentrance_dmap;
		if(changeddmap)
		{
			throwGenScriptEvent(GENSCR_EVENT_CHANGE_DMAP);
		}
		if(changedlevel)
		{
			throwGenScriptEvent(GENSCR_EVENT_CHANGE_LEVEL);
		}
		homescr = currscr = lastentrance;
		init_dmap();
	}
	else
	{
		if((DMaps[currdmap].type&dmfTYPE)==dmOVERW)
		{
			homescr = currscr = DMaps[currdmap].cont;
		}
		else
		{
			homescr = currscr = DMaps[currdmap].cont + DMaps[currdmap].xoff;
		}
	}
	
	currmap = DMaps[currdmap].map;
	dlevel = DMaps[currdmap].level;
	
	for(int32_t i=0; i<6; i++)
		visited[i]=-1;
		
	ALLOFF();
	whistleclk=-1;
	room_is_dark=darkroom=naturaldark=false;
	tmpscr[0].zero_memory();
	tmpscr[1].zero_memory();
	
	loadscr(0,currdmap,currscr,-1,false);
	putscr(scrollbuf,0,0,&tmpscr[0]);
	putscrdoors(scrollbuf,0,0,&tmpscr[0]);
	//preloaded freeform combos
	ffscript_engine(true);
	
	loadfullpal();
	ringcolor(false);
	loadlvlpal(DMaps[currdmap].color);
	lighting(false,true);
	Hero.init();
	if(DMaps[currdmap].flags&dmfBUNNYIFNOPEARL)
	{
		int32_t itemid = current_item_id(itype_pearl);
		if(itemid < 0)
		{
			Hero.setBunnyClock(-1);
		}
	}
	
	currcset=DMaps[currdmap].color;
	openscreen();
	map_bkgsfx(true);
	Hero.set_respawn_point();
	Hero.trySideviewLadder();
	show_subscreen_numbers=true;
	show_subscreen_life=true;
	if(!(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE)))
	{
		loadguys();
	}
	
	if(!Quit)
	{
		//play_DmapMusic();
		if(!(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE))) playLevelMusic();
		
		if(isdungeon())
			Hero.stepforward(get_qr(qr_LTTPWALK)?11:12, false);
			
		newscr_clk=frame;
		activated_timed_warp=false;
		if(tmpscr->room==rGANON && !get_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE))
		{
			Hero.ganon_intro();
		}
	}
}


void putintro()
{
	if (!get_qr(qr_OLD_DMAP_INTRO_STRINGS))
		return;

    if(!stricmp("                                                                        ", DMaps[currdmap].intro))
    {
        introclk=intropos=72;
        return;
    }
    
    if((cBbtn())&&(get_qr(qr_ALLOWMSGBYPASS)))
    {
        //finish writing out the string
        for(; intropos<72; ++intropos)
        {
            textprintf_ex(msg_txt_display_buf,get_zc_font(font_zfont),((intropos%24)<<3)+32,((intropos/24)<<3)+40,QMisc.colors.msgtext,-1,
                          "%c",DMaps[currdmap].intro[intropos]);
        }
    }
    
    if(intropos>=72)
    {
        if(dmapmsgclk >= 51)
            dmapmsgclk=50;
            
        return;
    }
    
    if(((introclk++)%6<5)&&((!cAbtn())||(!get_qr(qr_ALLOWFASTMSG))))
        return;
        
    dmapmsgclk=51;
    
    if(intropos == 0)
    {
        while(DMaps[currdmap].intro[intropos]==' ')
            ++intropos;
    }
    
    sfx(WAV_MSG); //actual message display
    
    
    //using the clip value to indicate the bitmap is "dirty"
    //rather than add yet another global variable
    set_clip_state(msg_txt_display_buf, 0);
    textprintf_ex(msg_txt_display_buf,get_zc_font(font_zfont),((intropos%24)<<3)+32,((intropos/24)<<3)+40,QMisc.colors.msgtext,-1,
                  "%c",DMaps[currdmap].intro[intropos]);
                  
    ++intropos;
    
    if(DMaps[currdmap].intro[intropos]==' ' && DMaps[currdmap].intro[intropos+1]==' ')
        while(DMaps[currdmap].intro[intropos]==' ')
            ++intropos;
}

void show_ffscript_names()
{
	int32_t ypos = 8;
	
	word c = tmpscr->numFFC();
	for(word i=0; i< c; i++)
	{
		if(ypos > 224) break;
		if(tmpscr->ffcs[i].script)
		{
			textout_shadowed_ex(framebuf,font, ffcmap[tmpscr->ffcs[i].script-1].scriptname.c_str(),2,ypos,WHITE,BLACK,-1);
			ypos+=12;
		}
	}
}

void do_magic_casting()
{
    static int32_t tempx, tempy;
    static byte herotilebuf[256];
    int32_t ltile=0;
    int32_t lflip=0;
    bool shieldModify=true;
    
    if(magicitem==-1)
    {
        return;
    }
    
    switch(itemsbuf[magicitem].family)
    {
    case itype_divinefire:
    {
        if(magiccastclk==0)
        {
            Lwpns.add(new weapon(HeroX(),HeroY(),HeroZ(),wPhantom,pDIVINEFIREROCKET,0,up, magicitem, Hero.getUID()));
            weapon *w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
	    w1->fakez = HeroFakeZ();
            w1->step=4;
            //          Hero.tile=(BSZ)?32:29;
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sidewaterhold2:ls_landhold2, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            casty=Hero.getY();
        }
        
        if(magiccastclk==64)
        {
            Lwpns.add(new weapon((zfix)HeroX(),(zfix)(-32),(zfix)HeroZ(),wPhantom,pDIVINEFIREROCKETRETURN,0,down, magicitem, Hero.getUID()));
            weapon *w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
	    w1->fakez = HeroFakeZ();
            w1->step=4;
            //          Hero.tile=29;
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sidewaterhold2:ls_landhold2, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            castnext=false;
        }
        
        if(castnext)
        {
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sideswimcast:ls_cast, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            if(get_qr(qr_MORESOUNDS))
                sfx(itemsbuf[magicitem].usesound,pan(int32_t(Hero.getX())));
                
            int32_t flamemax=itemsbuf[magicitem].misc1;
            
            for(int32_t flamecounter=((-1)*(flamemax/2))+1; flamecounter<=((flamemax/2)+1); flamecounter++)
            {
		    //divine fire level fix to go here
                //Lwpns.add(new weapon((zfix)HeroX(),(zfix)HeroY(),(zfix)HeroZ(),wFire,3,itemsbuf[magicitem].power*game->get_hero_dmgmult(),
                Lwpns.add(new weapon((zfix)HeroX(),(zfix)HeroY(),(zfix)HeroZ(),wFire,itemsbuf[magicitem].fam_type,itemsbuf[magicitem].power*game->get_hero_dmgmult(),
                                     isSideViewGravity() ? (flamecounter<flamemax ? left : right) : 0, magicitem, Hero.getUID(), false, 0, 0, 0));
                weapon *w = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
		w->fakez = HeroFakeZ();
                w->step=(itemsbuf[magicitem].misc2/100.0);
                w->angular=true;
                w->angle=(flamecounter*PI/(flamemax/2.0));
            }
            
            castnext=false;
            magiccastclk=128;
        }
        
        if((magiccastclk++)>=226)
        {
            magicitem=-1;
            magiccastclk=0;
        }
    }
    break;
    
    case itype_divineescape:
    {
        if(magiccastclk==0)
        {
            herotile(&ltile, &lflip, Hero.IsSideSwim()?ls_sideswimstab:ls_stab, down, zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                ltile+=Hero.getTileModifier();
            }
            
            unpack_tile(newtilebuf, ltile, lflip, true);
            memcpy(herotilebuf, unpackbuf, 256);
            tempx=Hero.getX();
            tempy=Hero.getY();
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sideswimpound:ls_pound, down, zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
        }
        
        if(magiccastclk>=0&&magiccastclk<64)
        {
            Hero.setX(tempx+((zc_oldrand()%3)-1));
            Hero.setY(tempy+((zc_oldrand()%3)-1));
        }
        
        if(magiccastclk==64)
        {
            Hero.setX(tempx);
            Hero.setY(tempy);
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sideswimstab:ls_stab, down, zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
        }
        
        if(magiccastclk==96)
        {
            if(get_qr(qr_MORESOUNDS))
                sfx(itemsbuf[magicitem].usesound,pan(int32_t(Hero.getX())));
                
            if ( Hero.getDontDraw() < 2 ) { Hero.setDontDraw(1); }
            
            for(int32_t i=0; i<16; ++i)
            {
                for(int32_t j=0; j<16; ++j)
                {
                    if(herotilebuf[i*16+j])
                    {
                        if(itemsbuf[magicitem].misc1==1)  // Twilight
                        {
                            particles.add(new pTwilight(Hero.getX()+j, Hero.getY()-Hero.getZ()+i, 5, 0, 0, (zc_oldrand()%8)+i*4));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=3;
                        }
                        else if(itemsbuf[magicitem].misc1==2)  // Sands of Hours
                        {
                            particles.add(new pTwilight(Hero.getX()+j, Hero.getY()-Hero.getZ()+i, 5, 1, 2, (zc_oldrand()%16)+i*2));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=4;
                            
                            if(zc_oldrand()%10 < 2)
                            {
                                p->color=1;
                                p->cset=0;
                            }
                        }
                        else
                        {
                            particles.add(new pDivineEscapeDust(Hero.getX()+j, Hero.getY()-Hero.getZ()+i, 5, 6, herotilebuf[i*16+j], zc_oldrand()%96));
                            
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->angular=true;
                            p->angle=zc_oldrand();
                            p->step=(((double)j)/8);
                            p->yofs=Hero.getYOfs();
                        }
                    }
                }
            }
        }
        
        if((magiccastclk++)>=226)
        {
			if(itemsbuf[magicitem].flags & ITEM_FLAG1) //Act as F6->Continue
			{
				Quit = qCONT;
				skipcont = 1;
			}
			else
			{
				//attackclk=0;
				int32_t div_prot_temp=div_prot_item;
				restart_level();
				div_prot_item=div_prot_temp;
				//xofs=0;
				//action=none;
				magicitem=-1;
				magiccastclk=0;
				if ( Hero.getDontDraw() < 2 ) { Hero.setDontDraw(0); }
			}
        }
    }
    break;
    
    case itype_divineprotection:
    {
        // See also hero.cpp, HeroClass::checkhit().
        if(magiccastclk==0)
        {
            Lwpns.add(new weapon(HeroX(),HeroY(),(zfix)0,wPhantom,pDIVINEPROTECTIONROCKET1,0,left, magicitem, Hero.getUID()));
            weapon *w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
            w1->step=4;
            Lwpns.add(new weapon(HeroX(),HeroY(),(zfix)0,wPhantom,pDIVINEPROTECTIONROCKET2,0,right, magicitem, Hero.getUID()));
            w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
            w1->step=4;
            //          Hero.tile=(BSZ)?32:29;
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sideswimcast:ls_cast, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            castx=Hero.getX();
        }
        
        if(magiccastclk==64)
        {
            for(int32_t i=0; i<Lwpns.Count(); i++)
            {
                weapon* w=static_cast<weapon*>(Lwpns.spr(i));
                if(w->id==wPhantom &&
                  w->type>=pDIVINEPROTECTIONROCKET1 && w->type<=pDIVINEPROTECTIONROCKETTRAILRETURN2)
                    Lwpns.del(i);
            }
            
            int32_t d=zc_max(HeroX(),256-HeroX())+32;
            Lwpns.add(new weapon((zfix)(HeroX()-d),(zfix)HeroY(),(zfix)HeroZ(),wPhantom,pDIVINEPROTECTIONROCKETRETURN1,0,right, magicitem,Hero.getUID()));
            weapon *w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
	    w1->fakez = HeroFakeZ();
            w1->step=4;
            Lwpns.add(new weapon((zfix)(HeroX()+d),(zfix)HeroY(),(zfix)HeroZ(),wPhantom,pDIVINEPROTECTIONROCKETRETURN2,0,left, magicitem,Hero.getUID()));
            w1 = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
	    w1->fakez = HeroFakeZ();
            w1->step=4;
            //          Hero.tile=29;
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sideswimcast:ls_cast, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            castnext=false;
        }
        
        if(castnext)
        {
            //          Hero.tile=4;
            herotile(&Hero.tile, &Hero.flip, &Hero.extend, Hero.IsSideSwim()?ls_sidewaterhold2:ls_landhold2, Hero.getDir(), zinit.heroAnimationStyle);
            
            if(get_qr(qr_EXPANDEDLTM))
            {
                Hero.tile+=Hero.getTileModifier();
            }
            
            Hero.setDivineProtectionShieldClk(itemsbuf[magicitem].misc1);
            
            if(get_qr(qr_MORESOUNDS))
            {
                if(div_prot_item != -1)
                {
                    stop_sfx(itemsbuf[div_prot_item].usesound+1);
                    stop_sfx(itemsbuf[div_prot_item].usesound);
                }
                
                cont_sfx(itemsbuf[magicitem].usesound);
            }
            
            castnext=false;
            magiccastclk=128;
            div_prot_item = magicitem;
        }
        
        // Finish the final spell pose
        if((magiccastclk++)>=160)
        {
            for(int32_t i=0; i<Lwpns.Count(); i++)
            {
                weapon* w=static_cast<weapon*>(Lwpns.spr(i));
                if(w->id==wPhantom &&
                  w->type>=pDIVINEPROTECTIONROCKET1 && w->type<=pDIVINEPROTECTIONROCKETTRAILRETURN2)
                    Lwpns.del(i);
            }
            
            magicitem=-1;
            magiccastclk=0;
        }
    }
    break;
    
    default:
        magiccastclk=0;
        break;
    }
}

void update_hookshot()
{
    int32_t hs_x, hs_y, hs_z,hs_dx, hs_dy;
    bool check_hs=false;
    int32_t dist_bx, dist_by, hs_w;
    chainlinks.animate();
    //  char tempbuf[80];
    //  char tempbuf2[80];
    
    //find out where the head is and make it
    //easy to reference
    if(Lwpns.idFirst(wHookshot)>-1)
    {
        check_hs=true;
    }
    
    if(check_hs)
    {
		weapon* hookweap = (weapon*)Lwpns.spr(Lwpns.idFirst(wHookshot));
        int32_t parentitem = hookweap->parentitem;
        hs_x=hookweap->x;
        hs_y=hookweap->y;
        hs_z=hookweap->z;
        hs_dx=hs_x-hs_startx;
        hs_dy=hs_y-hs_starty;
        
        //extending
        if(hookweap->misc==0)
        {
            int32_t maxchainlinks=itemsbuf[parentitem].misc2;
            
            if(chainlinks.Count()<maxchainlinks)           //extending chain
            {
                if(abs(hs_dx)>=hs_xdist+8)
                {
                    hs_xdist=abs(hs_x-hs_startx);
                    chainlinks.add(new weapon((zfix)hs_x, (zfix)hs_y, (zfix)hs_z,wHSChain, 0,0,Hero.getDir(), parentitem,Hero.getUID()));
                }
                else if(abs(hs_dy)>=hs_ydist+8)
                {
                    hs_ydist=abs(hs_y-hs_starty);
                    chainlinks.add(new weapon((zfix)hs_x, (zfix)hs_y, (zfix)hs_z,wHSChain, 0,0,Hero.getDir(), parentitem,Hero.getUID()));
                }
            }                                                     //stretching chain
            else
            {
                dist_bx=(abs(hs_dx)-(8*chainlinks.Count()))/(chainlinks.Count()+1);
                dist_by=(abs(hs_dy)-(8*chainlinks.Count()))/(chainlinks.Count()+1);
                hs_w=8;
                
                if(hs_dx<0)
                {
                    dist_bx=0-dist_bx;
                    hs_w=-8;
                }
                
                if(hs_dy<0)
                {
                    dist_by=0-dist_by;
                    hs_w=-8;
                }
                
                for(int32_t counter=0; counter<chainlinks.Count(); counter++)
                {
                    if(Hero.getDir()>down)                            //chain is moving horizontally
                    {
                        chainlinks.spr(counter)->x=hs_startx+hs_w+dist_bx+(counter*(hs_w+dist_bx));
                    }
                    else
                    {
                        chainlinks.spr(counter)->y=hs_starty+hs_w+dist_by+(counter*(hs_w+dist_by));
                    }
                }
            }
        }                                                       //retracting
        else if(hookweap->misc==1)
        {
            dist_bx=(abs(hs_dx)-(8*chainlinks.Count()))/(chainlinks.Count()+1);
            dist_by=(abs(hs_dy)-(8*chainlinks.Count()))/(chainlinks.Count()+1);
            hs_w=8;
            
            if(hs_dx<0)
            {
                dist_bx=0-dist_bx;
                hs_w=-8;
            }
            
            if(hs_dy<0)
            {
                dist_by=0-dist_by;
                hs_w=-8;
            }
            
            /* With ZScript modification, chains can conceivably move diagonally.*/
            //if (Hero.getDir()>down)                               //chain is moving horizontally
            {
                if(abs(hs_dx)-(8*chainlinks.Count())>0)             //chain is stretched
                {
                    for(int32_t counter=0; counter<chainlinks.Count(); counter++)
                    {
                        chainlinks.spr(counter)->x=hs_startx+hs_w+dist_bx+(counter*(hs_w+dist_bx));
                    }
                }
                else
                {
                    if(abs(hs_x-hs_startx)<=hs_xdist-8)
                    {
                        hs_xdist=abs(hs_x-hs_startx);
                        
                        if(pull_hero==false)
                        {
                            chainlinks.del(chainlinks.idLast(wHSChain));
                        }
                        else
                        {
                            chainlinks.del(chainlinks.idFirst(wHSChain));
                        }
                    }
                }
            }                                                     //chain is moving vertically
            //else
            {
                if(abs(hs_dy)-(8*chainlinks.Count())>0)             //chain is stretched
                {
                    for(int32_t counter=0; counter<chainlinks.Count(); counter++)
                    {
                        chainlinks.spr(counter)->y=hs_starty+hs_w+dist_by+(counter*(hs_w+dist_by));
                    }
                }
                else
                {
                    if(abs(hs_y-hs_starty)<=hs_ydist-8)
                    {
                        hs_ydist=abs(hs_y-hs_starty);
                        
                        if(pull_hero==false)
                        {
                            chainlinks.del(chainlinks.idLast(wHSChain));
                        }
                        else
                        {
                            chainlinks.del(chainlinks.idFirst(wHSChain));
                        }
                    }
                }
            }
        }
    }
}

void do_dcounters()
{
    for(int32_t i=0; i<32; i++)
    {
        if(game->get_dcounter(i)==0)
        {
            continue;
        }
        
		byte sfx_to_use = 0;
        if(frame&1)
        {
            if(game->get_dcounter(i)>0)
            {
				sfx_to_use = QMisc.miscsfx[sfxREFILL];
                int32_t drain = (i==4 ? game->get_mp_per_block()/4 : 1);
				if(get_qr(qr_FASTCOUNTERDRAIN)) drain *= 4;
                drain = zc_min(game->get_dcounter(i),drain);
                
                if(game->get_counter(i) < game->get_maxcounter(i))
                {
                    game->change_counter(drain, i);
                    game->change_dcounter(-drain, i);
                    
                    if(game->get_dcounter(i) < 0)  // Better safe
                        game->set_dcounter(0, i);
                }
                else
                {
                    game->set_dcounter(0, i);
                    game->set_counter(game->get_maxcounter(i), i);
                }
            }
            else
            {
                if(i==1)   // Only rupee drain is sounded
                    sfx_to_use = QMisc.miscsfx[sfxDRAIN];;
                    
                int32_t drain = (i==4 ? 2*game->get_magicdrainrate() : 1);
				if(get_qr(qr_FASTCOUNTERDRAIN)) drain *= 4;
                drain = zc_min(-game->get_dcounter(i),drain);
                
                if(game->get_counter(i)>0)
                {
                    game->change_counter(-drain, i);
                    game->change_dcounter(drain, i);
                    
                    if(game->get_dcounter(i) > 0)  // Better safe
                        game->set_dcounter(0, i);
                }
                else
                {
                    game->set_dcounter(0, i);
                    game->set_counter(0, i);
                }
            }
        }
        
        if((sfx_to_use) && !lensclk && (i<2 || i==4)) // Life, Rupees and Magic
            sfx(sfx_to_use);
    }
}

void update_msgstr()
{
	if(!msgstr) return;
	
	set_clip_state(msg_bg_display_buf, 0);
	blit(msg_bg_bmp_buf, msg_bg_display_buf, 0, 0, msg_xpos, msg_ypos, msg_w+16, msg_h+16);
	set_clip_state(msg_txt_display_buf, 0);
	if(get_qr(qr_OLD_STRING_EDITOR_MARGINS)!=0)
	{
		blit(msg_txt_bmp_buf, msg_txt_display_buf, 0, 0, msg_xpos, msg_ypos, msg_w+16, msg_h+16);
		masked_blit(msg_menu_bmp_buf, msg_txt_display_buf, 0, 0, msg_xpos, msg_ypos, msg_w+16, msg_h+16);
	}
	else
	{
		blit(msg_txt_bmp_buf, msg_txt_display_buf, msg_margins[left], msg_margins[up], msg_xpos+msg_margins[left], msg_ypos+msg_margins[up], msg_w-msg_margins[left]-msg_margins[right], msg_h-msg_margins[up]-msg_margins[down]);
		masked_blit(msg_menu_bmp_buf, msg_txt_display_buf, msg_margins[left], msg_margins[up], msg_xpos+msg_margins[left], msg_ypos+msg_margins[up], msg_w-msg_margins[left]-msg_margins[right], msg_h-msg_margins[up]-msg_margins[down]);
	}
	set_clip_state(msg_portrait_display_buf, 0);
	blit(msg_portrait_bmp_buf, msg_portrait_display_buf, 0, 0, prt_x, prt_y, prt_tw*16, prt_th*16);
}

extern bool do_end_str;
#define F7 46+7
//bool zasmstacktrace = false;
void game_loop()
{
	while(true)
	{
		GameFlags &= ~GAMEFLAG_RESET_GAME_LOOP;
		genscript_timing = SCR_TIMING_START_FRAME;
		if (midi_suspended==midissuspRESUME )
		{
			if ( currmidi >= 0 )
			{
				int32_t digi_vol, midi_vol;
			
				get_volume(&digi_vol, &midi_vol);
				zc_stop_midi();
				jukebox(currmidi);
				zc_set_volume(digi_vol, midi_vol);
				zc_midi_seek(paused_midi_pos);
			}
			midi_suspended = midissuspNONE;
		}
		
		//  walkflagx=0; walkflagy=0;
		runDrunkRNG();
		clear_darkroom_bitmaps();
		Hero.check_platform_ffc();
		
		
		// Three kinds of freezes: freeze, freezemsg, freezeff
		
		// freezemsg if message is being printed && qr_MSGFREEZE is on,
		// or if a message is being prepared && qr_MSGDISAPPEAR is on.
		bool freezemsg = ((msg_active || (intropos && intropos<72) || (linkedmsgclk && get_qr(qr_MSGDISAPPEAR)))
			&& (get_qr(qr_MSGFREEZE)));
		if (!get_qr(qr_SCRIPTDRAWSFROZENMSG))
			FFCore.skipscriptdraws = freezemsg;
		if(!freezemsg || get_qr(qr_SCRIPTDRAWSFROZENMSG))
		{
			if ( !FFCore.system_suspend[susptSCRIPDRAWCLEAR] ) script_drawing_commands.Clear();
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_START_FRAME);
		
		if(fadeclk>=0 && !freezemsg)
		{
			if(fadeclk==0 && currscr<128)
				blockpath=false;
				
			--fadeclk;
		}
		
		// Messages also freeze FF combos.
		bool freezeff = freezemsg;
		
		bool freeze = false;
		
		word c = tmpscr->numFFC();
		for(word i=0; i<c; i++)
		{
			if(combobuf[tmpscr->ffcs[i].data].type==cSCREENFREEZE) freeze=true;
			
			if(combobuf[tmpscr->ffcs[i].data].type==cSCREENFREEZEFF) freezeff=true;
		}
		
		for(int32_t i=0; i<176; i++)
		{
			if(combobuf[tmpscr->data[i]].type == cSCREENFREEZE) freeze=true;
			
			if(combobuf[tmpscr->data[i]].type == cSCREENFREEZEFF) freezeff=true;
		}
		
		if(!freeze_guys && !freeze && !freezemsg && !FFCore.system_suspend[susptGUYS])
		{
			for(int32_t i=0; i<176; i++)
			{
				if(guygrid[i]>0)
				{
					--guygrid[i];
				}
			}
			for(int32_t i=0; i<MAXFFCS; i++)
			{
				if(guygridffc[i]>0)
				{
					--guygridffc[i];
				}
			}
		}
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "animate_combos()\n");
		#endif
		if ( !FFCore.system_suspend[susptCOMBOANIM] )
		{
			animate_combos();
			cpos_update();
		}
		run_gswitch_timers();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_COMBO_ANIM);
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "load_control_state()\n");
		#endif
		if ( !FFCore.system_suspend[susptCONTROLSTATE] ) load_control_state();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_POLL_INPUT);

		update_slopes();
		if(!freezeff)
		{
			update_freeform_combos();
		}
		
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_FFCS);
		// Arbitrary Rule 637: neither 'freeze' nor 'freezeff' freeze the global script.
		if (!FFCore.system_suspend[susptGLOBALGAME] && !freezemsg && FFCore.doscript(ScriptType::Global, GLOBAL_SCRIPT_GAME))
		{
			ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_GLOBAL_ACTIVE);
		if(!FFCore.system_suspend[susptHEROACTIVE] && !freezemsg && FFCore.doscript(ScriptType::Player) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255)
		{
			ZScriptVersion::RunScript(ScriptType::Player, SCRIPT_PLAYER_ACTIVE);
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_PLAYER_ACTIVE);
		if(!FFCore.system_suspend[susptDMAPSCRIPT] && !freezemsg && FFCore.doscript(ScriptType::DMap) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255)
		{
			ZScriptVersion::RunScript(ScriptType::DMap, DMaps[currdmap].script,currdmap);
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DMAPDATA_ACTIVE);
		if(!FFCore.system_suspend[susptDMAPSCRIPT] && !freezemsg && FFCore.doscript(ScriptType::ScriptedPassiveSubscreen) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255)
		{
			ZScriptVersion::RunScript(ScriptType::ScriptedPassiveSubscreen, DMaps[currdmap].passive_sub_script,currdmap);
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DMAPDATA_PASSIVESUBSCREEN);
		if ( !FFCore.system_suspend[susptCOMBOSCRIPTS] && !freezemsg && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			FFCore.combo_script_engine(false);    
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_COMBOSCRIPT);
		
		
		if(!freeze && !freezemsg)
		{
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "mblock2.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptMOVINGBLOCKS] )  mblock2.animate(0);
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_PUSHBLOCK);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "items.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptITEMSPRITESCRIPTS] )  FFCore.itemSpriteScriptEngine();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_ITEMSPRITE_SCRIPT);
			if ( !FFCore.system_suspend[susptITEMS] ) items.animate();
		
			//Can't be called in items.animate(), as ZQuest also uses this function.
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "items.check_conveyor()\n");
			#endif
			if ( !FFCore.system_suspend[susptCONVEYORSITEMS] ) items.check_conveyor();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_ITEMSPRITE_ANIMATE);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "guys.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptGUYS] ) guys.animate();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_NPC_ANIMATE);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "roaming_item()\n");
			#endif
			if ( !FFCore.system_suspend[susptROAMINGITEM] ) roaming_item();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "dragging_item()\n");
			#endif
			if ( !FFCore.system_suspend[susptDRAGGINGITEM] ) dragging_item();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "Ewpns.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptEWEAPONS] ) Ewpns.animate();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_EWPN_ANIMATE);
			if ( !FFCore.system_suspend[susptEWEAPONSCRIPTS] ) FFCore.eweaponScriptEngine();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_EWPN_SCRIPT);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is setting: %s\n", "checkhero=true()\n");
			#endif
			
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "clear_script_one_frame_conditions()\n");
			#endif
			
			if ( !FFCore.system_suspend[susptONEFRAMECONDS] )  clear_script_one_frame_conditions(); //clears npc->HitBy[] for this frame: the timing on this may need adjustment. 
			
			if ( get_qr(qr_OLD_ITEMDATA_SCRIPT_TIMING) && !FFCore.system_suspend[susptITEMSCRIPTENGINE] )
				FFCore.itemScriptEngine(); //run before lweapon scripts
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_OLD_ITEMDATA_SCRIPT);
			if ( !FFCore.system_suspend[susptHERO] )
			{
				for(int32_t i = 0; i < (gofast ? 8 : 1); i++)
				{
					#if LOGGAMELOOP > 0
					al_trace("game_loop is at: %s\n", "if(Hero.animate(0)\n");
					#endif
					if(Hero.animate(0))
					{
						if(!Quit)
						{
							replay_step_comment("hero died");
							Quit = qGAMEOVER;
						}
						
						return;
					}
					else Hero.post_animate();
					if(GameFlags & GAMEFLAG_RESET_GAME_LOOP) break; //break the for()
				}
				if(GameFlags & GAMEFLAG_RESET_GAME_LOOP) continue; //continue the game_loop while(true)
			}
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_PLAYER_ANIMATE);
			if ( !get_qr(qr_OLD_ITEMDATA_SCRIPT_TIMING) && !FFCore.system_suspend[susptITEMSCRIPTENGINE] )
				FFCore.itemScriptEngine(); //run before lweapon scripts
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_NEW_ITEMDATA_SCRIPT);
			
			//FFCore.itemScriptEngine(); //run before lweapon scripts
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "do_magic_casting()\n");
			#endif
			Hero.cleanupByrna(); //Prevent sfx glitches with Cane of Byrna if it fails to initialise; ported from 2.53. -Z
			if ( !FFCore.system_suspend[susptMAGICCAST] ) do_magic_casting();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_CASTING);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "Lwpns.animate()\n");
			#endif
			//perhaps add sprite.waitdraw, and call sprite script here too?
			//FFCore.lweaponScriptEngine();
			if ( !FFCore.system_suspend[susptLWEAPONS] ) Lwpns.animate();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_LWPN_ANIMATE);
			
			//FFCore.lweaponScriptEngine();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "FFCore.itemScriptEngine())\n");
			#endif
			
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "decorations.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptDECORATIONS] ) decorations.animate();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "particles.animate()\n");
			#endif
			if ( !FFCore.system_suspend[susptPARTICLES] ) particles.animate();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DECOPARTICLE_ANIMATE);
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "update_hookshot()\n");
			#endif
			if ( !FFCore.system_suspend[susptHOOKSHOT] ) update_hookshot();
			
			if(conveyclk<=0)
			{
				conveyclk=3;
			}
			
			--conveyclk;
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "check_collisions()\n");
			#endif
			if ( !FFCore.system_suspend[susptCOLLISIONS] ) check_collisions();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "dryuplake()\n");
			#endif
			if ( !FFCore.system_suspend[susptLAKES] ) dryuplake();
			#if LOGGAMELOOP > 0
			al_trace("game_loop is calling: %s\n", "cycle_palette()\n");
			#endif
			if ( !FFCore.system_suspend[susptPALCYCLE] ) cycle_palette();
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_COLLISIONS_PALETTECYCLE);
		}
		else if(freezemsg)
		{
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_ITEMSPRITE_ANIMATE);
			for(int32_t i=0; i<guys.Count(); i++)
			{
				if(((enemy*)guys.spr(i))->ignore_msg_freeze())
				{
					if ( !FFCore.system_suspend[susptGUYS] ) guys.spr(i)->animate(i);
				}
			}
			FFCore.runGenericPassiveEngine(SCR_TIMING_POST_NPC_ANIMATE);
		}

		FFCore.runGenericPassiveEngine(SCR_TIMING_WAITDRAW);
		if (!FFCore.system_suspend[susptGLOBALGAME] && FFCore.waitdraw(ScriptType::Global, GLOBAL_SCRIPT_GAME))
		{
			ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
			FFCore.waitdraw(ScriptType::Global, GLOBAL_SCRIPT_GAME) = false;
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_GLOBAL_WAITDRAW);
		if ( !FFCore.system_suspend[susptHEROACTIVE] && FFCore.waitdraw(ScriptType::Player) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(ScriptType::Player, SCRIPT_PLAYER_ACTIVE);
			FFCore.waitdraw(ScriptType::Player) = false;
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_PLAYER_WAITDRAW);
		if ( !FFCore.system_suspend[susptDMAPSCRIPT] && FFCore.waitdraw(ScriptType::DMap) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(ScriptType::DMap, DMaps[currdmap].script,currdmap);
			FFCore.waitdraw(ScriptType::DMap) = false;
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DMAPDATA_ACTIVE_WAITDRAW);
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && FFCore.waitdraw(ScriptType::ScriptedPassiveSubscreen) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(ScriptType::ScriptedPassiveSubscreen, DMaps[currdmap].passive_sub_script,currdmap);
			FFCore.waitdraw(ScriptType::ScriptedPassiveSubscreen) = false;
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DMAPDATA_PASSIVESUBSCREEN_WAITDRAW);
		
		
		if ( !FFCore.system_suspend[susptSCREENSCRIPTS] && tmpscr->script != 0 && FFCore.doscript(ScriptType::Screen) && FFCore.waitdraw(ScriptType::Screen) && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(ScriptType::Screen, tmpscr->script);  
			FFCore.waitdraw(ScriptType::Screen) = 0;	    
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_SCREEN_WAITDRAW);
		
		c = tmpscr->numFFC();
		for ( word q = 0; q < c; ++q )
		{
			if (FFCore.waitdraw(ScriptType::FFC, q))
			{
				if(tmpscr->ffcs[q].script != 0 && !FFCore.system_suspend[susptFFCSCRIPTS] )
				{
					ZScriptVersion::RunScript(ScriptType::FFC, tmpscr->ffcs[q].script, q);
					FFCore.waitdraw(ScriptType::FFC, q) = false;
				}
			}
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_FFC_WAITDRAW);
		
		if ( !FFCore.system_suspend[susptCOMBOSCRIPTS] && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			FFCore.combo_script_engine(false, true);    
		}
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_COMBO_WAITDRAW);
		
		//Waitdraw for item scripts. 
		if ( !FFCore.system_suspend[susptITEMSCRIPTENGINE] ) FFCore.itemScriptEngineOnWaitdraw();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_ITEM_WAITDRAW);
		
		//Sprite scripts on Waitdraw in order of : npc, ewpn, lwpn, itemsprite
		if ( !FFCore.system_suspend[susptNPCSCRIPTS] ) FFCore.npcScriptEngineOnWaitdraw();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_NPC_WAITDRAW);
		if ( !FFCore.system_suspend[susptEWEAPONSCRIPTS] ) FFCore.eweaponScriptEngineOnWaitdraw();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_EWPN_WAITDRAW);
		if ( !FFCore.system_suspend[susptLWEAPONSCRIPTS] ) FFCore.lweaponScriptEngineOnWaitdraw();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_LWPN_WAITDRAW);
		if ( !FFCore.system_suspend[susptITEMSPRITESCRIPTS] ) FFCore.itemSpriteScriptEngineOnWaitdraw();
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_ITEMSPRITE_WAITDRAW);
		
		
		
		
		
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "draw_screen()\n");
		#endif
		if ( !FFCore.system_suspend[susptSCREENDRAW] ) draw_screen(tmpscr,true,true);
		else FFCore.runGenericPassiveEngine(SCR_TIMING_POST_DRAW);
		
		//clear Hero's last hits 
		//for ( int32_t q = 0; q < 4; q++ ) Hero.sethitHeroUID(q, 0); //Clearing these here makes checking them fail both before and after waitdraw. 
		#if LOGGAMELOOP > 0
		al_trace("game_loop is at: %s\n", "if(linkedmsgclk)\n");
		#endif
		if(linkedmsgclk==1 && !do_end_str)
		{
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(wpnsbuf[iwMore].tile!=0)\n");
		#endif
			if(wpnsbuf[iwMore].tile!=0)
			{
				putweapon(framebuf,zinit.msg_more_x, message_more_y(), wPhantom, 4, up, lens_hint_weapon[wPhantom][0], lens_hint_weapon[wPhantom][1],-1);
			}
		}
		
		if(!freeze)
		{
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "putintro()\n");
		#endif
			putintro();
		}
		
		if(dmapmsgclk>0)
		{
			Hero.Freeze();
			
			if(dmapmsgclk<=50)
			{
				--dmapmsgclk;
			}
		}
		
		if(dmapmsgclk==1)
		{
			Hero.finishedmsg();
			dmapmsgclk=0;
			introclk=72;
			clear_bitmap(msg_bg_display_buf);
			set_clip_state(msg_bg_display_buf, 1);
			clear_bitmap(msg_txt_display_buf);
			set_clip_state(msg_txt_display_buf, 1);
			clear_bitmap(msg_portrait_display_buf);
			set_clip_state(msg_portrait_display_buf, 1);
			//    clear_bitmap(pricesdisplaybuf);
		}
		
		FFCore.runGenericPassiveEngine(SCR_TIMING_POST_STRINGS);
		
		if(!freeze)
		{
			if(introclk==0 || (introclk>=72 && dmapmsgclk==0))
			{
				putmsg();
				
				update_msgstr();
				if(GameFlags & GAMEFLAG_RESET_GAME_LOOP) continue; //continue the game_loop while(true)
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "do_dcounters()\n");
		#endif
			do_dcounters();
			
		#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(!freezemsg && current_item(itype_heartring))\n");
		#endif
			if(!freezemsg && current_item(itype_heartring))
			{
				int32_t itemid = current_item_id(itype_heartring);
				int32_t fskip = itemsbuf[itemid].misc2;
				
				if(fskip == 0 || frame % fskip == 0)
					game->set_life(zc_min(game->get_life() + itemsbuf[itemid].misc1, game->get_maxlife()));
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(!freezemsg && current_item(itype_magicring))\n");
		#endif
			if(!freezemsg && current_item(itype_magicring))
			{
				int32_t itemid = current_item_id(itype_magicring);
				int32_t fskip = itemsbuf[itemid].misc2;
				
				if(fskip == 0 || frame % fskip == 0)
				{
					game->set_magic(zc_min(game->get_magic() + itemsbuf[itemid].misc1, game->get_maxmagic()));
				}
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(!freezemsg && current_item(itype_wallet))\n");
		#endif
			if(!freezemsg && current_item(itype_wallet))
			{
				int32_t itemid = current_item_id(itype_wallet);
				int32_t fskip = itemsbuf[itemid].misc2;
				
				if(fskip == 0 || frame % fskip == 0)
				{
					game->set_rupies(zc_min(game->get_rupies() + itemsbuf[itemid].misc1, game->get_maxcounter(1)));
				}
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(!freezemsg && current_item(itype_bombbag))\n");
		#endif
			if(!freezemsg && current_item(itype_bombbag))
			{
				int32_t itemid = current_item_id(itype_bombbag);
				
				if(itemsbuf[itemid].misc1)
				{
					int32_t fskip = itemsbuf[itemid].misc2;
					
					if(fskip == 0 || frame % fskip == 0)
					{
						game->set_bombs(zc_min(game->get_bombs() + itemsbuf[itemid].misc1, game->get_maxbombs()));
					}
					
					if((itemsbuf[itemid].flags & ITEM_FLAG1) && zinit.bomb_ratio)
					{
						int32_t ratio = zinit.bomb_ratio;
						
						fskip = itemsbuf[itemid].misc2 * ratio;
						
						if(fskip == 0 || frame % fskip == 0)
						{
							game->set_sbombs(zc_min(game->get_sbombs() + zc_max(itemsbuf[itemid].misc1 / ratio, 1), game->get_maxbombs() / ratio));
						}
					}
				}
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(!freezemsg && current_item(itype_quiver))\n");
		#endif
			if(!freezemsg && current_item(itype_quiver) && game->get_arrows() != game->get_maxarrows())
			{
				int32_t itemid = current_item_id(itype_quiver);
				int32_t fskip = itemsbuf[itemid].misc2;
				
				if(fskip == 0 || frame % fskip == 0)
				{
					game->set_arrows(zc_min(game->get_arrows() + itemsbuf[itemid].misc1, game->get_maxarrows()));
				}
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(lensclk)\n");
		#endif
			if(lensclk && !FFCore.system_suspend[susptLENS])
			{
				draw_lens_over();
				--lensclk;
			}
			#if LOGGAMELOOP > 0
		al_trace("game_loop is calling: %s\n", "if(quakeclk)\n");
		#endif
			// Earthquake!
			if(quakeclk>0 && !FFCore.system_suspend[susptQUAKE] )
			{
				playing_field_offset=56+((int32_t)(zc::math::Sin((double)(--quakeclk*2-frame))*4));
			}
			else
			{
				playing_field_offset=56;
			}
			
		if ( previous_DMap != currdmap )
		{
			FFCore.initZScriptDMapScripts();
			FFCore.initZScriptScriptedActiveSubscreen();
			previous_DMap = currdmap;
		}
			// Other effects in zc_sys.cpp
		}
		
		FFCore.runGenericPassiveEngine(SCR_TIMING_END_FRAME);
		//  putpixel(framebuf, walkflagx, walkflagy+playing_field_offset, vc(int32_t(zc_oldrand()%16)));
		break;
	}
}

void runDrunkRNG(){
	//Runs the RNG for drunk for each control which makes use of drunk toggling. 
	//Index 0-10 refer to control_state[0]-[9], while index 11 is used for `DrunkrMbtn()`/`DrunkcMbtn()`, which do not use control_states[]
	for(int32_t i = 0; i<sizeof(drunk_toggle_state); i++){
		if((!(frame%((zc_oldrand(&drunk_rng)%100)+1)))&&(zc_oldrand(&drunk_rng)%MAXDRUNKCLOCK<Hero.DrunkClock())){
			drunk_toggle_state[i] = (zc_oldrand(&drunk_rng)%2)?true:false;
		} else {
			drunk_toggle_state[i] = false;
		}
	}
}

int32_t get_currdmap()
{
    return currdmap;
}

int32_t get_dlevel()
{
    return dlevel;
}

int32_t get_currscr()
{
    return currscr;
}

int32_t get_currmap()
{
    return currmap;
}

int32_t get_homescr()
{
    return homescr;
}

int32_t get_bmaps(int32_t si)
{
    return game->bmaps[si];
}

bool no_subscreen()
{
    return (tmpscr->flags3&fNOSUBSCR)!=0;
}

bool isMonochrome(){
	return monochrome;
}

bool isUserTinted()
{
	return palette_user_tinted;
}
void isUserTinted(bool state)
{
	palette_user_tinted = state;
}

void setMonochrome(bool v){
	if ( v && (!monochrome || lastMonoPreset) ) { //lastMonoPreset check to allow overwriting a mono preset with setMonochrome's greyscale
		if(isUserTinted()){ // If a user tint is active, disable it and restore RAMpal
			memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
			isUserTinted(false);
		} else if(lastMonoPreset){ // If a monochrome preset was loaded, restore RAMpal, and disable it
			memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
			lastMonoPreset = 0;
		} else { // else back up RAMpal to tempgreypal
			memcpy(tempgreypal, RAMpal, PAL_SIZE*sizeof(RGB));
		}
		if(get_qr(qr_FADE)) {
		for(int32_t i=CSET(0); i < CSET(15); i++)
		{
			int32_t g = zc_min((RAMpal[i].r*42 + RAMpal[i].g*75 + RAMpal[i].b*14) >> 7, 63);
			g = (g >> 1) + 32;
			RAMpal[i] = _RGB(g,g,g);
		}
    
		}
		else
		{
			// this is awkward. NES Z1 converts colors based on the global
			// NES palette. Something like RAMpal[i] = NESpal( reverse_NESpal(RAMpal[i]) & 0x30 );
			for(int32_t i=CSET(0); i < CSET(15); i++)
			{
				RAMpal[i] = NESpal(reverse_NESpal(RAMpal[i]) & 0x30);
			}
		} 
		refreshpal = true;
		monochrome = true; 
	}
	
	else if ( !v && monochrome && !lastMonoPreset ) {
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		refreshpal = true;
		monochrome = false;
	}
}

enum { colourNONE, colourGREY, colourRED, colourGREEN, colourBLUE, colourVIOLET, colourTEAL, colourAMBER, colourCYAN };
enum { baseUNIFORM, baseDISTRIBUTED = 1000 };
enum { tint_r, tint_g, tint_b, tint_bool_dist};

int16_t lastMonoPreset = 0; // The current Monochrome preset loaded
int16_t lastCustomTint[4] = {0,0,0,0}; // The current custom tint information. 0/1/2: R/G/B, 3: Base

void shiftColour(int32_t rshift, int32_t gshift, int32_t bshift, int32_t base)
{
	for(int32_t i=0; i <= 0xEF; i++)
	{
		if(base==baseUNIFORM){//Recolor the palette to uniform greyscale before tinting
			int32_t grey = (RAMpal[i].r+RAMpal[i].g+RAMpal[i].b)/3;
			RAMpal[i] = _RGB(grey,grey,grey);
		} else if(base==baseDISTRIBUTED){//Recolor the palette to distributed greyscale before tinting
			int32_t grey = 0.299*RAMpal[i].r + 0.587*RAMpal[i].g + 0.114*RAMpal[i].b;
			RAMpal[i] = _RGB(grey,grey,grey);
		}
		//Bit-shifting negatives throws errors. If negative, shift in the other direction.
		if(rshift>=0){
			RAMpal[i].r = zc_min(RAMpal[i].r >> rshift,63);
		} else {
			RAMpal[i].r = zc_min(RAMpal[i].r << -rshift,63);
		}
		if(gshift>=0){
			RAMpal[i].g = zc_min(RAMpal[i].g >> gshift,63);
		} else {
			RAMpal[i].g = zc_min(RAMpal[i].g << -gshift,63);
		}
		if(bshift>=0){
			RAMpal[i].b = zc_min(RAMpal[i].b >> bshift,63);
		} else {
			RAMpal[i].b = zc_min(RAMpal[i].b << -bshift,63);
		}
	}
}

void setMonochromatic(int32_t mode)
{
	int32_t base = mode < baseDISTRIBUTED ? baseUNIFORM : baseDISTRIBUTED; //distributed is an additive flag adding 10
	int32_t colour_mode = mode - base;
	if(isUserTinted()){
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		isUserTinted(false); //Disable custom tint, override with monochrome
	}
	lastMonoPreset = mode;
	if (colour_mode <= 0 && monochrome ) //restore
	{
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		refreshpal = true;
		monochrome = false; 
	}
	else if ( colour_mode ) 
	{ 
		//If a preset was already active, restore RAMpal before continuing; else, back up RAMpal.
		if ( monochrome ) {
			memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		} else {
			memcpy(tempgreypal, RAMpal, PAL_SIZE*sizeof(RGB));
		}
		switch(colour_mode)
		{
			case colourGREY:
				shiftColour(0,0,0,base);
				break;
			case colourRED:
				shiftColour(0,4,4,base);
				break;
			case colourGREEN:
				shiftColour(4,0,4,base);
				break;
			case colourBLUE:
				shiftColour(4,4,0,base);
				break;
			case colourVIOLET:
				shiftColour(1,4,0,base);
				break;
			case colourTEAL:
				shiftColour(4,1,0,base);
				break;
			case colourAMBER:
				shiftColour(0,1,4,base);
				break;
			case colourCYAN:
				addColour(-63,-6,-2,base);
				break;
			default: shiftColour(0,0,0,base);
				break;
		}

		refreshpal = true;
		monochrome = true; 
	}
}

void addColour(int32_t radd, int32_t gadd, int32_t badd, int32_t base)
{
	for(int32_t i=0; i <= 0xEF; i++)
	{
		if(base==baseUNIFORM){//Recolor the palette to uniform greyscale before tinting
			int32_t grey = (RAMpal[i].r+RAMpal[i].g+RAMpal[i].b)/3;
			RAMpal[i] = _RGB(grey,grey,grey);
		} else if(base==baseDISTRIBUTED){//Recolor the palette to distributed greyscale before tinting
			int32_t grey = 0.299*RAMpal[i].r + 0.587*RAMpal[i].g + 0.114*RAMpal[i].b;
			RAMpal[i] = _RGB(grey,grey,grey);
		}
		//Add the r/g/b adds to the r/g/b values, clamping between 0 and 63.
		RAMpal[i].r = vbound(RAMpal[i].r + radd,0,63);
		RAMpal[i].g = vbound(RAMpal[i].g + gadd,0,63);
		RAMpal[i].b = vbound(RAMpal[i].b + badd,0,63);
	}
}

void doGFXMonohue(int32_t _r, int32_t _g, int32_t _b, bool m)
{
	if(monochrome) { // If a mono preset or greyscale was active, disable it and restore RAMpal before continuing.
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		monochrome = false;
		lastMonoPreset = 0;
	}
	if(isUserTinted()) { // A tint already is active. Tint should then cascade.
		//Restore RAMpal
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		//Add the current tint parameters to the previous ones
		lastCustomTint[0] = vbound(lastCustomTint[0]+_r,-32768,32767);
		lastCustomTint[1] = vbound(lastCustomTint[1]+_g,-32768,32767);
		lastCustomTint[2] = vbound(lastCustomTint[2]+_b,-32768,32767);
		//Force the base to the new base; bases do NOT cascade, they override.
		lastCustomTint[3] = m?baseDISTRIBUTED:baseUNIFORM;
		//Color with the new args
		addColour(lastCustomTint[0],lastCustomTint[1],lastCustomTint[2],lastCustomTint[3]);
	} else { // No tint is active.
		memcpy(tempgreypal, RAMpal, PAL_SIZE*sizeof(RGB));//Back up RAMpal into tempgreypal
		lastCustomTint[0] = _r;
		lastCustomTint[1] = _g;
		lastCustomTint[2] = _b;
		lastCustomTint[3] = m?baseDISTRIBUTED:baseUNIFORM;
		addColour(_r,_g,_b,m?baseDISTRIBUTED:baseUNIFORM);
	}
	lastMonoPreset = 0; // Clear mono preset to use tint instead
	isUserTinted(true);
	refreshpal = true;
}

void doTint(int32_t _r, int32_t _g, int32_t _b)
{
	if(monochrome) { // If a mono preset or greyscale was active, disable it and restore RAMpal before continuing.
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		monochrome = false;
		lastMonoPreset = 0;
	}
	if(isUserTinted()) { // A tint already is active. Tint should then cascade.
		//Restore RAMpal
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		//Add the current tint parameters to the previous ones
		lastCustomTint[0] = vbound(lastCustomTint[0]+_r,-32768,32767);
		lastCustomTint[1] = vbound(lastCustomTint[1]+_g,-32768,32767);
		lastCustomTint[2] = vbound(lastCustomTint[2]+_b,-32768,32767);
		//Force the base to the new base; bases do NOT cascade, they override.
		lastCustomTint[3] = -1;
		//Color with the new args
		addColour(lastCustomTint[0],lastCustomTint[1],lastCustomTint[2],lastCustomTint[3]);
	} else { // No tint is active
		memcpy(tempgreypal, RAMpal, PAL_SIZE*sizeof(RGB));//Back up RAMpal into tempgreypal
		lastCustomTint[0] = _r;
		lastCustomTint[1] = _g;
		lastCustomTint[2] = _b;
		lastCustomTint[3] = -1;
		addColour(_r,_g,_b,-1);
	}
	lastMonoPreset = 0; // Clear mono preset to use tint instead
	isUserTinted(true);
	refreshpal = true;
}

void doClearTint()
{
	//If a color mode was active, restore RAMpal from the backup
	if(monochrome || isUserTinted()){
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		//Disable the booleans
		monochrome = false;
		isUserTinted(false);
		//Clear the storage
		lastMonoPreset = 0;
		refreshpal = true;
	}
}

void restoreTint(){
	if(isUserTinted()){
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		addColour(lastCustomTint[0],lastCustomTint[1],lastCustomTint[2],lastCustomTint[3]);
		refreshpal = true;
	}
}

void restoreMonoPreset(){
	if(monochrome && lastMonoPreset){
		memcpy(RAMpal, tempgreypal, PAL_SIZE*sizeof(RGB));
		setMonochromatic(lastMonoPreset);
		refreshpal = true;
	}
}

void refreshTints()
{
	if(isMonochrome() && !lastMonoPreset)
	{
		setMonochrome(false);
		setMonochrome(true);
	}
	restoreTint();
	restoreMonoPreset();
}

int32_t getTint(int32_t color)
{
	return lastCustomTint[color];
}

/**************************/
/********** Main **********/
/**************************/

bool is_editor()
{
    return false;
}

bool screenIsScrolling()
{
    return screenscrolling;
}

int32_t isFullScreen()
{
    return !is_windowed_mode();
}

static bool load_replay_file_deffered_called = false;
static std::string load_replay_file_filename;
static ReplayMode load_replay_file_mode;
// Because using "Load Replay" GUI menu can happen while another replay is
// within an inner game-loop (like scrollscr), which can bleed the old replay
// into the new one if `replay_start` is called from the GUI control code.
// Instead, save the information needed to call load_replay_file later.
void load_replay_file_deferred(ReplayMode mode, std::string replay_file)
{
	load_replay_file_deffered_called = true;
	load_replay_file_mode = mode;
	load_replay_file_filename = replay_file;
}

static bool current_session_is_replay = false;
static void load_replay_file(ReplayMode mode, std::string replay_file, int frame)
{
	ASSERT(mode == ReplayMode::Replay || mode == ReplayMode::Assert || mode == ReplayMode::Update);
	replay_start(mode, replay_file, frame);

	std::string qst_meta = replay_get_meta_str("qst");
	testingqst_name = qst_meta;
	// If the path resolved relative to the .zplay file exists, use that instead.
	if (!std::filesystem::path(qst_meta).is_absolute())
	{
		auto resolved_qst = std::filesystem::path(replay_file).parent_path() / qst_meta;
		if (std::filesystem::is_regular_file(resolved_qst))
		{
			testingqst_name = resolved_qst.string();
		}
	}

	if (!std::filesystem::is_regular_file(testingqst_name))
	{
		// TODO: not showing...
		// InfoDialog("File Error", fmt::format("File not found: {}", testingqst_name)).show();
		Z_error("File not found: %s\n", testingqst_name.c_str());
		if (!load_replay_file_deffered_called)
		{
			// This was called from the CLI, so abort.
			abort();
		}
	}

	if (replay_get_meta_bool("test_mode"))
	{
		testingqst_dmap = replay_get_meta_int("starting_dmap");
		testingqst_screen = replay_get_meta_int("starting_scr");
		testingqst_retsqr = replay_get_meta_int("starting_retsqr");
		testingqst_init_data = "";
		use_testingst_start = true;
	}
	else
	{
		use_testingst_start = false;
	}

	if (strlen(zc_get_config("zeldadx", "replay_snapshot", "")) > 0)
		replay_add_snapshot_frame(zc_get_config("zeldadx", "replay_snapshot", ""));

	// Older quests don't have a misc section, and QMisc isn't set by default except via loading the default quest.
	// So do that here, since not all code paths to this will have done it already.
	init_NES_mode();
}

void zc_game_srand(int seed, zc_randgen* rng)
{
	if (rng == nullptr)
		rng = zc_get_default_rand();

	if (replay_is_active())
		replay_set_rng_seed(rng, seed);
	else
		zc_srand(seed, rng);
}


static void allocate_crap()
{	
	for(int32_t i=0; i<WAV_COUNT; i++)
	{
		customsfxdata[i].data=NULL;
		sfx_string[i] = new char[36];
	}
	
	for(int32_t i=0; i<WAV_COUNT>>3; i++)
	{
		customsfxflag[i] = 0;
	}
	
	for(int32_t i=0; i<MAXWPNS; i++)
	{
		weapon_string[i] = new char[64];
	}
	
	for(int32_t i=0; i<MAXITEMS; i++)
	{
		item_string[i] = new char[64];
	}
	
	for(int32_t i=0; i<eMAXGUYS; i++)
	{
		guy_string[i] = new char[64];
	}
	
	for(int32_t i=0; i<NUMSCRIPTFFC; i++)
	{
		ffscripts[i] = new script_data(ScriptType::FFC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTITEM; i++)
	{
		itemscripts[i] = new script_data(ScriptType::Item, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
	{
		guyscripts[i] = new script_data(ScriptType::NPC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
	{
		screenscripts[i] = new script_data(ScriptType::Screen, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGLOBAL; i++)
	{
		globalscripts[i] = new script_data(ScriptType::Global, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTPLAYER; i++)
	{
		playerscripts[i] = new script_data(ScriptType::Player, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		lwpnscripts[i] = new script_data(ScriptType::Lwpn, i);
	}
	 for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		ewpnscripts[i] = new script_data(ScriptType::Ewpn, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
	{
		dmapscripts[i] = new script_data(ScriptType::DMap, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
	{
		itemspritescripts[i] = new script_data(ScriptType::ItemSprite, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
	{
		comboscripts[i] = new script_data(ScriptType::Combo, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSGENERIC; i++)
	{
		genericscripts[i] = new script_data(ScriptType::Generic, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
	{
		subscreenscripts[i] = new script_data(ScriptType::EngineSubscreen, i);
	}
}

void do_load_and_quit_command(const char* quest_path)
{
	// We need to init some stuff before loading a quest file will work.
	int fake_errno = 0;
	allegro_errno = &fake_errno;
	get_qst_buffers();
	allocate_crap();

	byte skip_flags[] = {0, 0, 0, 0};
	int ret = loadquest(quest_path,&QHeader,&QMisc,tunes+ZC_MIDI_COUNT,false,skip_flags,false,false,0xFF);
	exit(ret);
}

void do_extract_zasm_command(const char* quest_path)
{
	// We need to init some stuff before loading a quest file will work.
	int fake_errno = 0;
	allegro_errno = &fake_errno;
	get_qst_buffers();
	allocate_crap();

	byte skip_flags[] = {0, 0, 0, 0};
	int ret = loadquest(quest_path,&QHeader,&QMisc,tunes+ZC_MIDI_COUNT,false,skip_flags,false,false,0xFF);
	if (ret)
		exit(ret);

	DEBUG_PRINT_TO_FILE = true;
	strcpy(qstpath, quest_path);
	bool top_functions = true;
	bool generate_yielder = get_flag_bool("-extract-zasm-yielder").value_or(false);
	if (zasm_optimize_enabled())
		zasm_optimize();
	zasm_for_every_script(true, [&](zasm_script* script){
		ScriptDebugHandle h(script, ScriptDebugHandle::OutputSplit::ByScript, script->name);
		h.print(zasm_to_string(script, top_functions, generate_yielder).c_str());
	});

	exit(0);
}

void do_analyze_zasm_duplication_command(const char* quest_path)
{
	// We need to init some stuff before loading a quest file will work.
	int fake_errno = 0;
	allegro_errno = &fake_errno;
	get_qst_buffers();
	allocate_crap();

	byte skip_flags[] = {0, 0, 0, 0};
	int ret = loadquest(quest_path,&QHeader,&QMisc,tunes+ZC_MIDI_COUNT,false,skip_flags,false,false,0xFF);
	if (ret)
		exit(ret);

	DEBUG_PRINT_TO_FILE = true;
	strcpy(qstpath, quest_path);

	printf("%s\n", zasm_analyze_duplication().c_str());

	exit(0);
}

int main(int argc, char **argv)
{
	int test_zc_arg = used_switch(argc, argv, "-test-zc");
	if (test_zc_arg > 0)
		set_headless_mode();

	qstdir = (char*)malloc(2048);
	qstpath = (char*)malloc(2048);
	qstdir[0] = 0;
	qstpath[0] = 0;

#ifdef _WIN32
	// For purposes of packaging a standalone app.
	if (used_switch(argc, argv, "-package"))
	{
		if (!std::filesystem::exists("zc_args.txt"))
		{
			Z_error_fatal("zc_args.txt does not exist");
		}

		std::vector<std::string> args = util::split_args(read_text_file("zc_args.txt"));
		args.insert(args.begin(), argv[0]);

		argv = new char*[args.size()];
		for (int index = 0; index < args.size(); index++)
		{
			argv[index] = strdup(args[index].c_str());
		}
		argc = args.size();
	}
#endif

	zalleg_setup_allegro(App::zelda, argc, argv);
	allocate_crap();
	set_should_zprint_cb([]() {
		return get_qr(qr_SCRIPTERRLOG) || DEVLEVEL > 0;
	});

	int only_arg = used_switch(argc, argv, "-only");
	if (only_arg)
	{
		fs::path only_path = (fs::current_path() / argv[only_arg+1]);
		only_qstpath = only_path.string();
		if (only_path.extension() != ".qst")
			Z_error_fatal("-only value must be a qst file, but got: %s\n", only_qstpath.c_str());
		if (!fs::exists(only_path))
			Z_error_fatal("Could not find file: %s\n", only_qstpath.c_str());
	}

	int test_opt_zasm_arg = used_switch(argc, argv, "-test-optimize-zasm");
	if (test_opt_zasm_arg)
	{
		zasm_optimize_run_for_file(argv[test_opt_zasm_arg+1]);
		exit(0);
	}

	if (test_zc_arg)
	{
		bool success = true;
		if (!saves_test())
		{
			success = false;
			printf("saves_test failed\n");
		}
		if (!zasm_optimize_test())
		{
			success = false;
			printf("zasm_optimize_test failed\n");
		}
		if (success)
			printf("all tests passed\n");
		exit(success ? 0 : 1);
	}

	int load_and_quit_arg = used_switch(argc, argv, "-load-and-quit");
	if (load_and_quit_arg > 0)
	{
		do_load_and_quit_command(argv[load_and_quit_arg+1]);
	}

	int extract_zasm_arg = used_switch(argc, argv, "-extract-zasm");
	if (extract_zasm_arg > 0)
	{
		do_extract_zasm_command(argv[extract_zasm_arg+1]);
	}

	int analyze_zasm_duplication_arg = used_switch(argc, argv, "-analyze-zasm-duplication");
	if (analyze_zasm_duplication_arg > 0)
	{
		do_analyze_zasm_duplication_command(argv[analyze_zasm_duplication_arg+1]);
	}

	int create_save_arg = used_switch(argc,argv,"-create-save");
	if (create_save_arg)
	{
		set_headless_mode();

		// We need to init some stuff before loading a quest file will work.
		int fake_errno = 0;
		allegro_errno = &fake_errno;
		get_qst_buffers();

		gamedata* new_game = new gamedata();
		new_game->header.name = "newsave";
		new_game->header.qstpath = argv[create_save_arg + 1];
		saves_create_slot(new_game);
		int ret = saves_do_first_time_stuff(saves_count() - 1);
		if (ret)
			printf("failed to save: %d\n", ret);
		exit(ret ? 1 : 0);
	}

	bool onlyInstance=true;
//	refresh_select_screen = 0;
	memset(modulepath, 0, sizeof(modulepath));
	FFCore.clear_combo_scripts();

	memset(zc_builddate,0,80);
	memset(zc_aboutstr,0,80);

	sprintf(zc_builddate,"Build Date: %s %s, %d at @ %s %s", dayextension(BUILDTM_DAY).c_str(), (char*)months[BUILDTM_MONTH], BUILDTM_YEAR, __TIME__, __TIMEZONE__);
	sprintf(zc_aboutstr,"%s, Version %s", ZC_PLAYER_NAME, getVersionString());
	

	Z_title("ZC Launched: %s, %s",ZC_PLAYER_NAME, getVersionString());
	
	if(!get_qst_buffers())
	{
		Z_error_fatal("Error");
	}

	three_finger_flag=false;

	// TODO: digi_volume has been removed as a user controllable setting. It is a global volume control for
	// a4 music streams, but we use emusic_volume for all enhanced music, so it was a weird double volume control.
	// Should remove all usages of digi_volume (maybe replacing with emusic_volume where appropriate). For now,
	// just set to 255.
	digi_volume = 255;
	
	load_game_configs();

	int standalone_arg = used_switch(argc, argv, "-standalone");
	if (standalone_arg)
	{
		standalone_mode=true;
		
		if (standalone_arg == argc-1)
		{
			Z_error_fatal("-standalone requires a quest file, e.g.\n" \
					"  -standalone MyQuest.qst\n" \
					"  -standalone \"Name with spaces.qst\"\n" \
					"Paths may be absolute, or relative to the quest directory.\n" \
					"An optional second argument can be used to specify a sav file");
		}

		// TODO make qstdir a fs::path and absoulute
		standalone_quest = (fs::current_path() / fs::path(qstdir) / argv[standalone_arg + 1]).lexically_normal();
		if (standalone_arg + 2 < argc && argv[standalone_arg + 2][0] != '-')
		{
			standalone_save_path = argv[standalone_arg + 2];
		}
		else
		{
			standalone_save_path = "standalone-" + standalone_quest.stem().string() + ".sav";
		}
	}

	if(used_switch(argc, argv, "-no_console"))
		console_enabled = false;
	
	if ( console_enabled )
	{
		FFCore.ZScriptConsole(true);
	}

	LOCK_FUNCTION(update_throttle_counter);
	if (install_int_ex(update_throttle_counter, BPS_TO_TIMER(60)) < 0)
	{
		Z_error_fatal("Could not install timer.\n");
	}
	
	LOCK_VARIABLE(myvsync);
	LOCK_FUNCTION(myvsync_callback);
	
	bool timerfail = false;
	if(install_int_ex(myvsync_callback,BPS_TO_TIMER(60)))
		timerfail = true;
	
	if(!timerfail && !Z_init_timers())
		timerfail = true;
	
	if(timerfail)
	{
		Z_error_fatal("Couldn't Allocate Timers");
	}
	
	Z_message("OK\n");
	
	// allocate bitmap buffers
	set_color_depth(8);
	framebuf  = create_bitmap_ex(8,256,224);
	menu_bmp  = create_bitmap_ex(8,640,480);
	scrollbuf = create_bitmap_ex(8,512,406);
	screen2   = create_bitmap_ex(8,320,240);
	tmp_scr   = create_bitmap_ex(8,320,240);
	tmp_bmp   = create_bitmap_ex(8,32,32);
	prim_bmp  = create_bitmap_ex(8,512,512);
	msg_bg_display_buf = create_bitmap_ex(8,256, 176);
	msg_txt_display_buf = create_bitmap_ex(8,256, 176);
	msg_bg_bmp_buf = create_bitmap_ex(8, 512+16, 512+16);
	msg_txt_bmp_buf = create_bitmap_ex(8, 512+16, 512+16);
	msg_menu_bmp_buf = create_bitmap_ex(8, 512+16, 512+16);
	msg_portrait_bmp_buf = create_bitmap_ex(8, 256, 256);
	msg_portrait_display_buf = create_bitmap_ex(8, 256, 256);
	pricesdisplaybuf = create_bitmap_ex(8,256, 176);
	script_menu_buf = create_bitmap_ex(8,256,224);
	f6_menu_buf = create_bitmap_ex(8,256,224);
	darkscr_bmp_curscr = create_bitmap_ex(8, 256, 176);
	darkscr_bmp_curscr_trans = create_bitmap_ex(8, 256, 176);
	darkscr_bmp_scrollscr = create_bitmap_ex(8, 256, 176);
	darkscr_bmp_scrollscr_trans = create_bitmap_ex(8, 256, 176);
	lightbeam_bmp = create_bitmap_ex(8, 256, 176);
	
	if(!framebuf || !scrollbuf || !tmp_bmp || !tmp_scr
			|| !screen2 || !msg_txt_display_buf || !msg_bg_display_buf || !pricesdisplaybuf
			|| !script_menu_buf || !f6_menu_buf)
	{
		Z_error_fatal("Error");
	}
	
	clear_bitmap(lightbeam_bmp);
	clear_bitmap(scrollbuf);
	clear_bitmap(framebuf);
	clear_bitmap(msg_bg_display_buf);
	set_clip_state(msg_bg_display_buf, 1);
	clear_bitmap(msg_txt_display_buf);
	set_clip_state(msg_txt_display_buf, 1);
	clear_bitmap(msg_portrait_display_buf);
	set_clip_state(msg_portrait_display_buf, 1);
	clear_bitmap(pricesdisplaybuf);
	set_clip_state(pricesdisplaybuf, 1);
	
	Z_message("Initializing music... ");
	zcmusic_init();
	zcmixer = zcmixer_create();
	Z_message("OK\n");
	
	//  int32_t mode = VidMode;                                       // from config file
	int32_t tempmode=GFX_AUTODETECT;
	int32_t res_arg = used_switch(argc,argv,"-res");
	
	if(used_switch(argc,argv,"-v0")) Throttlefps=false;
	
	if(used_switch(argc,argv,"-v1")) Throttlefps=true;
	if(used_switch(argc,argv,"-show-fps")) ShowFPS=true;
	if(used_switch(argc,argv,"-show-time")) ShowGameTime=true;
	
	debug_enabled = used_switch(argc,argv,"-d") && !strcmp(zc_get_config("zeldadx","debug",""),zeldapwd);
	set_debug(debug_enabled);

	int32_t load_save=0;
	
	load_save = used_switch(argc,argv,"-load");
	load_save = load_save?(argc>load_save+1)?atoi(argv[load_save+1]):0:0;
	if(!load_save)
	{
		load_save = zc_get_config("zeldadx","quickload_slot",0);
	}
	if(!load_save)
	{
		if(used_switch(argc,argv,"-loadlast") || zc_get_config("zeldadx","quickload_last",0))
			load_save = loadlast;
	}
			
	slot_arg = used_switch(argc,argv,"-slot");
	
	if(argc <= (slot_arg+1))
	{
		slot_arg = 0;
	}
	else
	{
		slot_arg2 = vbound(atoi(argv[slot_arg+1]), 1, 100000);
	}
	
	if(standalone_mode)
	{
		load_save=1;
		slot_arg=1;
		slot_arg2=1;
	}
	
	int32_t fast_start = debug_enabled || used_switch(argc,argv,"-fast") || (!standalone_mode && (load_save || (slot_arg && (argc>(slot_arg+1)))));
	
	int32_t checked_epilepsy = zc_get_config("zeldadx","checked_epilepsy",0);
	
	set_color_conversion(COLORCONV_NONE);
	
	//script drawing bitmap allocation
	zscriptDrawingRenderTarget = new ZScriptDrawingRenderTarget();
	
	Z_init_sound();
	
	const int32_t wait_ms_on_set_graphics = 20; //formerly 250. -Gleeok

	// quick quit
	if(used_switch(argc,argv,"-q"))
	{
		printf("-q switch used, quitting program.\n");
		if (get_qr(qr_OLD_SCRIPT_VOLUME))
		{
			//restore user volume settings
			if (FFCore.coreflags & FFCORE_SCRIPTED_MIDI_VOLUME)
			{
				master_volume(-1, ((int32_t)FFCore.usr_midi_volume));
			}
			if (FFCore.coreflags & FFCORE_SCRIPTED_DIGI_VOLUME)
			{
				master_volume((int32_t)(FFCore.usr_digi_volume), 1);
			}
			if (FFCore.coreflags & FFCORE_SCRIPTED_MUSIC_VOLUME)
			{
				emusic_volume = (int32_t)FFCore.usr_music_volume;
			}
			if (FFCore.coreflags & FFCORE_SCRIPTED_SFX_VOLUME)
			{
				sfx_volume = (int32_t)FFCore.usr_sfx_volume;
			}
		}
		if ( FFCore.coreflags&FFCORE_SCRIPTED_PANSTYLE )
		{
			pan_style = (int32_t)FFCore.usr_panstyle;
		}
		save_game_configs();
		set_gfx_mode(GFX_TEXT,80,25,0,0);
		//rest(250); // ???
		//  if(useCD)
		//    cd_exit();
		quit_game();
		Z_message("ZQuest Classic web site: https://zquestclassic.com.com\n");
		Z_message("ZQuest Classic old wiki: https://web.archive.org/web/20210910193102/https://zeldaclassic.com/wiki\n");
		Z_message("ZQuest Classic new wiki: https://github.com/ZQuestClassic/ZQuestClassic/wiki\n");
		
		skipcont = 0;
		if(forceExit) //fix for the allegro at_exit() hang.
			exit(0);
		
		jit_shutdown();
		allegro_exit();
		return 0;
	}
	
	// set video mode
	
	if(res_arg && (argc>(res_arg+2)))
	{
		resx = atoi(argv[res_arg+1]);
		resy = atoi(argv[res_arg+2]);
		bool old_sbig = (argc>(res_arg+3))? stricmp(argv[res_arg+3],"big")==0 : 0;
		bool old_sbig2 = (argc>(res_arg+3))? stricmp(argv[res_arg+3],"big2")==0 : 0;
	}

	//request_refresh_rate(60);al_trace("Used switch: -fullscreen\n");
	
	//is the config file wrong (not zc.cfg!) here? -Z
	if(used_switch(argc,argv,"-fullscreen") ||
			(!used_switch(argc, argv, "-windowed") && zc_get_config("zeldadx","fullscreen",0)==1))
	{
		al_trace("Used switch: -fullscreen\n");
		tempmode = GFX_AUTODETECT_FULLSCREEN;
	}
	else if(used_switch(argc,argv,"-windowed") || zc_get_config("zeldadx","fullscreen",0)==0)
	{
		al_trace("Used switch: -windowed\n");
		tempmode=GFX_AUTODETECT_WINDOWED;
	}

	if (resx != -1)
	{
		if(resx < 256) resx = 256;
		if(resy < 240) resy = 240;
	}

	zq_screen_w = 640;
	zq_screen_h = 480;

	auto [w, h] = zc_get_default_display_size(zq_screen_w, zq_screen_h, resx, resy);
	resx = w;
	resy = h;
	// TODO: consolidate "resx" and "resy" variables with window_width,height.
	window_width = resx;
	window_height = resy;
	
	if(zc_get_config("gui","disable_window_resizing",0))
		all_set_resize_flag(false);
	
	if(!game_vid_mode(tempmode, wait_ms_on_set_graphics))
	{
		al_trace("Fatal Error: could not create a window for ZQuest Classic.\n");
		Z_error_fatal(allegro_error);
	}
	else
	{
		Z_message("set gfx mode succsessful at -%d %dbpp %d x %d \n", tempmode, get_color_depth(), resx, resy);
	}

	const char* window_title = "ZQuest Classic";
	int window_title_arg = used_switch(argc, argv, "-window-title");
	if (window_title_arg > 0)
		window_title = argv[window_title_arg + 1];
	set_window_title(window_title);
	zalleg_create_window();

#ifndef __EMSCRIPTEN__
	if (!all_get_fullscreen_flag() && !is_headless()) {
		// Just in case.
		while (!all_get_display()) {
			al_rest(1);
		}

		int window_w = al_get_display_width(all_get_display());
		int window_h = al_get_display_height(all_get_display());
		if(window_w < 320 || window_h < 240)
		{
			if(window_w < 320) window_w = 320;
			if(window_h < 240) window_h = 240;
			al_resize_display(all_get_display(),window_w,window_h);
		}
		
		int new_x = zc_get_config("zeldadx","window_x",0);
		int new_y = zc_get_config("zeldadx","window_y",0);
#ifdef ALLEGRO_MACOSX
		if (new_x && new_y)
			al_set_window_position(all_get_display(), new_x, new_y);
#else
		if (new_x == 0 && new_y == 0)
		{
			ALLEGRO_MONITOR_INFO info;
			al_get_monitor_info(0, &info);

			new_x = (info.x2 - info.x1) / 2 - window_w / 2;
			new_y = (info.y2 - info.y1) / 2 - window_h / 2;
		}
		al_set_window_position(all_get_display(), new_x, new_y);
#endif
	}
#endif
	switch_type = pause_in_background ? SWITCH_PAUSE : SWITCH_BACKGROUND;

	if (!is_headless())
	{
		set_display_switch_mode(is_windowed_mode()?SWITCH_PAUSE:switch_type);
		set_display_switch_callback(SWITCH_OUT, switch_out_callback);
		set_display_switch_callback(SWITCH_IN, switch_in_callback);
	}

	hw_palette = &RAMpal;
	if (is_headless())
		screen = create_bitmap_ex(8, 256, 240);
	clear_to_color(screen, BLACK);

	// Initialize render tree.
	render_zc();
	
	set_close_button_callback((void (*)()) hit_close_button);

	fix_dialogs();
	gui_mouse_focus = FALSE;
	position_mouse(resx-16,resy-16);
	
	if(!onlyInstance)
	{
		clear_to_color(screen,BLACK);
		enter_sys_pal();
		int32_t ret=jwin_alert3("Multiple Instances",
							"Another instance of ZC is already running.",
							"Running multiple instances may cause your",
							"save file to be deleted. Continue anyway?",
							"&No","&Yes", 0, 'n', 'y', 0, get_zc_font(font_lfont));
		exit_sys_pal();
		if(ret!=2)
		{
			if(forceExit)
				exit(0);
				
			allegro_exit();
			return 0;
		}
	}

#ifdef __EMSCRIPTEN__
	checked_epilepsy = true;
#endif
	
	// TODO: we are repeating this code (See few lines above) but different switch mode ...
	if (!is_headless())
	{
		set_display_switch_mode(is_windowed_mode()?(pause_in_background ? SWITCH_PAUSE : SWITCH_BACKGROUND):SWITCH_BACKAMNESIA);
		set_display_switch_callback(SWITCH_OUT, switch_out_callback);
		set_display_switch_callback(SWITCH_IN, switch_in_callback);
	}

#ifdef ZC_WIN_32
	{
		enter_sys_pal();
		info_dsa("Windows 32-bit is deprecated!",
			"There are known issues with the 32 bit version of ZC. Use 64-bit instead, if you can.",
			"dsa_32bit");
		exit_sys_pal();
	}
#endif

	int32_t test_init_data_arg = used_switch(argc,argv,"-test-init-data");
	if (test_init_data_arg > 0)
		testingqst_init_data = argv[test_init_data_arg + 1];

	int32_t test_arg = used_switch(argc,argv,"-test");
	zqtesting_mode = test_arg > 0;
	if(zqtesting_mode)
	{
		clearConsole();
		Z_message("Initializing test mode...\n");
		if(test_arg+3 >= argc)
		{
			Z_error_fatal( "-test missing parameters:\n"
				"-test \"quest_file_path\" test_dmap test_screen\n" );
		}
		bool error = false;
		testingqst_name = argv[test_arg+1];
		int32_t dm = atoi(argv[test_arg+2]);
		int32_t scr = atoi(argv[test_arg+3]);
		int32_t retsqr = (test_arg+4 >= argc) ? 0 : atoi(argv[test_arg+4]);
		if(!fileexists(testingqst_name.c_str()))
		{
			Z_error_fatal( "-test invalid parameter: 'quest_file_path' was '%s',"
				" but that file does not exist!\n", testingqst_name.c_str());
		}
		if(unsigned(dm) >= MAXDMAPS)
		{
			Z_error_fatal( "-test invalid parameter: 'test_dmap' was '%d'."
				" Must be '0 <= test_dmap < %d'\n", dm, MAXDMAPS);
		}
		if(unsigned(scr) >= 0x80)
		{
			Z_error_fatal( "-test invalid parameter: 'test_screen' was '%d'."
				" Must be '0 <= test_screen < 128'\n", scr);
		}
		if(unsigned(retsqr) > 3) retsqr = 0;
		
		if(error)
		{
			Z_error_fatal("Failed '-test \"%s\" %d %d'\n", testingqst_name.c_str(), dm, scr);
		}
		use_testingst_start = true;
		testingqst_dmap = (uint16_t)dm;
		testingqst_screen = (uint8_t)scr;
		testingqst_retsqr = (uint8_t)retsqr;
	}

	if (used_switch(argc, argv, "-replay-exit-when-done") > 0)
		replay_enable_exit_when_done();

	if (used_switch(argc, argv, "-replay-save-games") > 0)
		saves_enable_save_current_replay();

	int replay_arg = used_switch(argc, argv, "-replay");
	int snapshot_arg = used_switch(argc, argv, "-snapshot");
	int record_arg = used_switch(argc, argv, "-record");
	int assert_arg = used_switch(argc, argv, "-assert");
	int update_arg = used_switch(argc, argv, "-update");
	int frame_arg = used_switch(argc, argv, "-frame");

	int frame = -1;
	if (frame_arg > 0)
		frame = std::stoi(argv[frame_arg + 1]);

	replay_debug = zc_get_config("zeldadx","replay_debug",0) == 1 || used_switch(argc, argv, "-replay-debug") > 0;

	int replay_output_dir_arg = used_switch(argc, argv, "-replay-output-dir");
	if (replay_output_dir_arg > 0)
		replay_set_output_dir(argv[replay_output_dir_arg + 1]);

	if (replay_arg > 0)
	{
		load_replay_file(ReplayMode::Replay, argv[replay_arg + 1], frame);
	}
	else if (assert_arg > 0)
	{
		load_replay_file(ReplayMode::Assert, argv[assert_arg + 1], frame);
	}
	else if (update_arg > 0)
	{
		load_replay_file(ReplayMode::Update, argv[update_arg + 1], frame);
	}
	else if (record_arg > 0)
	{
		ASSERT(zqtesting_mode);
		int replay_name_arg = used_switch(argc, argv, "-replay-name");

		replay_start(ReplayMode::Record, argv[record_arg + 1], frame);
		replay_set_debug(replay_debug);
		replay_set_sync_rng(true);
		replay_set_meta("qst", testingqst_name);
		replay_set_meta_bool("test_mode", true);
		replay_set_meta("starting_dmap", testingqst_dmap);
		replay_set_meta("starting_scr", testingqst_screen);
		replay_set_meta("starting_retsqr", testingqst_retsqr);
		if (used_switch(argc, argv, "-replay-name") > 0)
			replay_set_meta("name", argv[replay_name_arg + 1]);
		use_testingst_start = true;
	}
	if (snapshot_arg > 0)
		replay_add_snapshot_frame(argv[snapshot_arg + 1]);
	
	if(!zqtesting_mode && !replay_is_active())
	{
		if (!checked_epilepsy)
		{
			clear_to_color(screen,BLACK);
			enter_sys_pal();
			if(jwin_alert("EPILEPSY Options",
				"Do you desire epilepsy protection?",
				"This will reduce the intensity of flashing effects",
				"and reduce the amplitude of wavy screen effects.",
				"No","Yes",13,27,get_zc_font(font_lfont))!=1)
			{
				epilepsyFlashReduction = 1;
			}
			exit_sys_pal();
			zc_set_config("zeldadx","checked_epilepsy",1);
			zc_set_config("zeldadx","epilepsy_flash_reduction",epilepsyFlashReduction);
			checked_epilepsy = 1;
		}
	}

	saves_init();

	// This will either quick load the first save file for this quest,
	// or if that doesn't exist prompt the player for a save file name
	// and then load the quest.
	int web_open_arg = used_switch(argc, argv, "-web-open");
	if (web_open_arg)
	{
		std::string qstpath_to_load = (fs::current_path() / argv[web_open_arg + 1]).string();

		std::string rel_qstpath = qstpath_to_load;
		char temppath[2048];
		memset(temppath, 0, 2048);
		// TODO: zc_make_relative_filename really shouldn't require trailing slash, but it does.
		std::string rel_dir = fmt::format("{}/", (fs::current_path() / fs::path(qstdir)).string());
		zc_make_relative_filename(temppath, rel_dir.c_str(), qstpath_to_load.c_str(), 2047);
		if (temppath[0] != 0)
			rel_qstpath = temppath;

		int ret = saves_load();
		if (ret)
		{
			Z_error_fatal("Failed to load saves. error: %d", ret);
		}

		int save_index = -1;
		int savecnt = saves_count();
		for (int i = 0; i < savecnt; i++)
		{
			auto save = saves_get_slot(i);
			if (!save->header->quest) continue;

			if (rel_qstpath == save->header->qstpath)
			{
				if (save_index == -1)
					save_index = i;
				else
				{
					save_index = -1;
					break;
				}
			}
		}

		if (save_index == -1)
		{
			load_qstpath = rel_qstpath;
		}
		else
		{
			load_save = save_index + 1;
		}
		fast_start = true;
	}

	set_display_switch_callback(SWITCH_IN,switch_in_callback);
	set_display_switch_callback(SWITCH_OUT,switch_out_callback);
	
	// play the game
	fix_menu();
	reset_items(true, &QHeader);
	
	clear_to_color(screen,BLACK);
	Quit = qQUIT;
	
	rgb_map = &rgb_table;
	
	DEBUG_PRINT_TO_FILE = zc_get_config("ZSCRIPT", "print_zasm_to_file", true);
	DEBUG_PRINT_TO_CONSOLE = zc_get_config("ZSCRIPT", "print_zasm_to_console", false);
	DEBUG_JIT_PRINT_ASM = zc_get_config("ZSCRIPT", "jit_print_asm", false) || used_switch(argc, argv, "-jit-print-asm");
	DEBUG_JIT_EXIT_ON_COMPILE_FAIL = zc_get_config("ZSCRIPT", "jit_fatal_compile_errors", false) || used_switch(argc, argv, "-jit-fatal-compile-errors");
	hangcount = zc_get_config("ZSCRIPT","ZASM_Hangcount",1000);
	jit_set_enabled(zc_get_config("ZSCRIPT", "jit", false) || used_switch(argc, argv, "-jit") > 0);
	
#ifdef _WIN32
	
	if(use_win32_proc != FALSE)
	{
		Z_message("Config file warning: \"zc_win_proc_fix\" enabled switch found. This can cause crashes on some computers.\n");

		if(win32data.zcSetCustomCallbackProc(al_get_win_window_handle(all_get_display())) != 0)
		{
			use_win32_proc = FALSE;
		}
	}
	
#endif

#ifdef __EMSCRIPTEN__
	em_mark_ready_status();
#endif
	
reload_for_replay_file:
	if (load_replay_file_deffered_called)
	{
		load_replay_file(load_replay_file_mode, load_replay_file_filename, -1);
		load_replay_file_deffered_called = false;
		saves_init();
	}

	current_session_is_replay = replay_is_active();
	disable_save_to_disk = zqtesting_mode || replay_is_active();

	if (!disable_save_to_disk)
	{
		// load saved games
		zprint2("Loading Saved Games\n");
		int ret = saves_load();
		if (ret)
		{
			Z_error_fatal("Failed to load saves. error: %d", ret);
		}
		zprint2("Finished Loading Saved Games\n");
	}

	if (zqtesting_mode || replay_is_active())
	{
		if (replay_is_active() && replay_get_meta_str("sav").size())
		{
			auto save_path = replay_get_replay_path().parent_path() / replay_get_meta_str("sav");
			bool success = saves_create_slot(save_path, false);
			if (!success)
				Z_error_fatal("Failed to load replay's save file");
		}
		else
		{
			gamedata* new_game = new gamedata();
			if (use_testingst_start)
			{
				new_game->set_continue_dmap(testingqst_dmap);
				new_game->set_continue_scrn(testingqst_screen);
			}
			else
			{
				new_game->set_continue_scrn(0xFF);
			}
			new_game->header.qstpath = testingqst_name;
			new_game->set_quest(0xFF);
			if (replay_is_active())
			{
				std::string replay_name = replay_get_meta_str("name", "Hero");
				new_game->set_name(replay_name.c_str());
			}
			else
			{
				new_game->set_name("Hero");
			}
			new_game->set_timevalid(1);
			saves_create_slot(new_game, false);
		}
		saves_select(0);
		if (use_testingst_start)
			Z_message("Test mode: \"%s\", %d, %d\n", testingqst_name.c_str(), testingqst_dmap, testingqst_screen);
		if (replay_is_active())
			printf("Replay is active\n");
	}
	
	init_ffpos();
	
	while(Quit!=qEXIT)
	{
		// this is here to continually fix the keyboard repeat
		set_keyboard_rate(250,33);
		toogam = false;
		ignoreSideview=false;
		clear_bitmap(lightbeam_bmp);
		if(Quit!=qCONT)
		{
			game_mouse_index = ZCM_BLANK; //Force game mouse to blank
			game_mouse();
			MouseSprite::clear(ZCM_CUSTOM); //Delete any custom cursor between quests
		}
		if (zqtesting_mode || replay_is_replaying())
		{
			int32_t q = Quit;
			Quit = 0;
			if (q == qCONT)
			{
				cont_game();
			}
			else if(init_game())
			{
				//Failed initializing? Keep trying.
				while (Quit != qEXIT)
				{
					handle_close_btn_quit();
					if (Quit == qEXIT) break;
					
					Quit = 0;
					init_game();
				}
			}
			game_pal();
		}
		else 
		{
			if(Quit != qCONT)
				init_game_vars();
			titlescreen(load_save);
		}
		if(clearConsoleOnReload)
			clearConsole();
		load_save=0;
		load_qstpath="";
		setup_combo_animations();
		setup_combo_animations2();
		active_cutscene.clear();
		game_mouse();
		
		while(Quit<=0)
		{
#ifdef _WIN32
			
			if(use_win32_proc != FALSE)
			{
				win32data.Update(0);
			}
#endif
			game_loop();
			advanceframe(true);
			FFCore.runF6Engine();
		
			//clear Hero's last hits 
			//for ( int32_t q = 0; q < 4; q++ ) Hero.sethitHeroUID(q, 0);
			//clearing this here makes it impossible 
			//to read before or after waitdraw in scripts. 
		}
		clear_info_bmp();

		if (load_replay_file_deffered_called)
		{
			Quit = 0;
			goto reload_for_replay_file;
		}
		
		tmpscr->flags3=0;
		Playing=Paused=false;
		//Clear active script array ownership
		FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_GAME);
		FFCore.deallocateAllScriptOwned(ScriptType::Player, SCRIPT_PLAYER_ACTIVE);
		switch(Quit)
		{
			case qSAVE:
			case qQUIT:
			case qGAMEOVER:
			case qRELOAD:
			case qCONT:
			case qSAVECONT:
			{
				playing_field_offset=56; // Fixes an issue with Hero being drawn wrong when quakeclk>0
				show_subscreen_dmap_dots=true;
				show_subscreen_numbers=true;
				show_subscreen_items=true;
				show_subscreen_life=true;
				show_ff_scripts=false;
				introclk=intropos=0;
				for ( int32_t q = 0; q < 256; q++ ) runningItemScripts[q] = 0; //Clear scripts that were running before. 

				initZScriptGlobalScript(GLOBAL_SCRIPT_END);
				FFCore.initZScriptHeroScripts(); //Should we not be calling this AFTER running the exit script!!
				FFCore.initZScriptDMapScripts(); //Should we not be calling this AFTER running the exit script!!
				FFCore.initZScriptItemScripts(); //Should we not be calling this AFTER running the exit script!!
				FFCore.initZScriptScriptedActiveSubscreen();
				FFCore.clear_combo_scripts(); //clear running combo script data
				//Run Global script OnExit
				ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_END, GLOBAL_SCRIPT_END);

				if(!skipcont&&!get_qr(qr_NOCONTINUE)) game_over(get_qr(qr_NOSAVE));
				
				if(Quit==qSAVE)
				{
					save_game(false);
				}
				else if(Quit==qSAVECONT)
				{
					save_game(false);
					Quit = qCONT;
				}
				
				skipcont = 0;
				
				if (get_qr(qr_OLD_SCRIPT_VOLUME))
				{
					//restore user volume settings
					if (FFCore.coreflags & FFCORE_SCRIPTED_MIDI_VOLUME)
					{
						master_volume(-1, ((int32_t)FFCore.usr_midi_volume));
					}
					if (FFCore.coreflags & FFCORE_SCRIPTED_DIGI_VOLUME)
					{
						master_volume((int32_t)(FFCore.usr_digi_volume), 1);
					}
					if (FFCore.coreflags & FFCORE_SCRIPTED_MUSIC_VOLUME)
					{
						emusic_volume = (int32_t)FFCore.usr_music_volume;
					}
					if (FFCore.coreflags & FFCORE_SCRIPTED_SFX_VOLUME)
					{
						sfx_volume = (int32_t)FFCore.usr_sfx_volume;
					}
				}
				if ( FFCore.coreflags&FFCORE_SCRIPTED_PANSTYLE )
				{
					pan_style = (int32_t)FFCore.usr_panstyle;
				}
			}
			break;
			
			case qWON:
			{
				show_subscreen_dmap_dots=true;
				show_subscreen_numbers=true;
				show_subscreen_items=true;
				show_subscreen_life=true;
				for ( int32_t q = 0; q < 256; q++ ) runningItemScripts[q] = 0; //Clear scripts that were running before. 

				initZScriptGlobalScript(GLOBAL_SCRIPT_END);
				FFCore.initZScriptHeroScripts(); //get ready for the onWin script
				FFCore.initZScriptDMapScripts();
				FFCore.initZScriptItemScripts();
				FFCore.initZScriptScriptedActiveSubscreen();
				FFCore.clear_combo_scripts();
				//Run global script OnExit
				//ZScriptVersion::RunScript(ScriptType::Player, SCRIPT_PLAYER_WIN); //runs in ending()
				//while(player_doscript) advanceframe(true); //Not safe. The script can run for only one frame. 
				//We need a special routine for win and death player scripts. Otherwise, they work. 
				ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_END, GLOBAL_SCRIPT_END);
			
			
				ending();
				set_uformat(U_UTF8);
			}
			break;
			
			case qINCQST:
			{
				Hero.setDontDraw(true);
				//Hero.setCharging(0);//don't have the sword out during the ending. 
				//Hero.setSwordClk(0);
				show_subscreen_dmap_dots=true;
				show_subscreen_numbers=true;
				show_subscreen_items=true;
				show_subscreen_life=true;
		
				initZScriptGlobalRAM();
				ZScriptVersion::RunScript(ScriptType::Global, GLOBAL_SCRIPT_END, GLOBAL_SCRIPT_END);
				ending_scripted();
			}
			break;
		}
		FFCore.deallocateAllScriptOwned(ScriptType::Global, GLOBAL_SCRIPT_END);
		//Restore original palette before exiting for any reason!
		setMonochrome(false);
		doClearTint();
		Hero.setDontDraw(0);
		if(Quit != qCONT)
		{
			memset(disabledKeys, 0, sizeof(disabledKeys));
			memset(disable_control, 0, sizeof(disable_control));
			FFCore.user_files_init();
			FFCore.user_dirs_init();
			FFCore.user_bitmaps_init();
			FFCore.user_paldata_init();
			FFCore.user_stacks_init();
			FFCore.user_objects_init();
			objectRAM.clear();
			FFCore.deallocateAllScriptOwned();
		}
		else
		{
			FFCore.deallocateAllScriptOwnedCont();
		}
		
		GameFlags = 0; //Clear game flags on ANY exit
		kill_sfx();
		music_stop();
		clear_to_color(screen,BLACK);

		if (replay_is_active())
		{
			if (replay_is_assert_done())
			{
				Quit = qQUIT;
				replay_stop();
			}
			else if (replay_get_mode() != ReplayMode::Record && Quit == qEXIT)
			{
				replay_poll();
				Quit = qQUIT;
			}
			else if (Quit == qQUIT)
			{
				replay_poll();
			}
		}

		if (current_session_is_replay && !replay_is_active() && Quit != qEXIT && Quit != qCONT)
		{
			// Replay is over, so jump up to load the real saves.
			Quit = 0;
			use_testingst_start = false;
			saves_unselect();
			goto reload_for_replay_file;
		}
	}
	
	// clean up
	
	if (replay_get_mode() == ReplayMode::Record) replay_save();
	replay_stop();
	music_stop();
	kill_sfx();
	
	if (get_qr(qr_OLD_SCRIPT_VOLUME))
	{
		//restore user volume settings
		if (FFCore.coreflags & FFCORE_SCRIPTED_MIDI_VOLUME)
		{
			master_volume(-1, ((int32_t)FFCore.usr_midi_volume));
		}
		if (FFCore.coreflags & FFCORE_SCRIPTED_DIGI_VOLUME)
		{
			master_volume((int32_t)(FFCore.usr_digi_volume), 1);
		}
		if (FFCore.coreflags & FFCORE_SCRIPTED_MUSIC_VOLUME)
		{
			emusic_volume = (int32_t)FFCore.usr_music_volume;
		}
		if (FFCore.coreflags & FFCORE_SCRIPTED_SFX_VOLUME)
		{
			sfx_volume = (int32_t)FFCore.usr_sfx_volume;
		}
	}
	if ( FFCore.coreflags&FFCORE_SCRIPTED_PANSTYLE )
	{
		pan_style = (int32_t)FFCore.usr_panstyle;
	}
	if (replay_get_mode() == ReplayMode::Record) replay_save();
	save_game_configs();
	set_gfx_mode(GFX_TEXT,80,25,0,0);
	//rest(250); // ???
	//  if(useCD)
	//    cd_exit();
	quit_game();
	Z_message("ZQuest Classic web site: https://zquestclassic.com\n");
	Z_message("ZQuest Classic old wiki: https://web.archive.org/web/20210910193102/https://zeldaclassic.com/wiki\n");
	Z_message("ZQuest Classic new wiki: https://github.com/ZQuestClassic/ZQuestClassic/wiki\n");
	
	skipcont = 0;
	
	zscript_coloured_console.kill();
	if(forceExit) //fix for the allegro at_exit() hang.
		exit(0);
		
	return 0;
}
END_OF_MAIN()


void remove_installed_timers()
{
    al_trace("Removing timers. \n");
    remove_int(update_throttle_counter);
    Z_remove_timers();
}


void delete_everything_else() //blarg.
{
    refresh_subscr_buttonitems();
	kill_subscr_items();
}

void quit_game()
{
	if (replay_get_mode() == ReplayMode::Record) replay_save();
	replay_stop();

	script_drawing_commands.Dispose(); //for allegro bitmaps
	
	remove_installed_timers();
	delete_everything_else();
	
	al_trace("Freeing Data: \n");
	
	free(game);
	game = NULL;
	
	al_trace("Bitmaps... \n");
	destroy_bitmap(framebuf);
	destroy_bitmap(scrollbuf);
	destroy_bitmap(tmp_scr);
	destroy_bitmap(screen2);
	destroy_bitmap(tmp_bmp);
	destroy_bitmap(prim_bmp);
	set_clip_state(msg_bg_display_buf, 1);
	destroy_bitmap(msg_bg_display_buf);
	set_clip_state(msg_txt_display_buf, 1);
	destroy_bitmap(msg_txt_display_buf);
	set_clip_state(msg_portrait_display_buf, 1);
	destroy_bitmap(msg_portrait_display_buf);
	destroy_bitmap(msg_txt_bmp_buf);
	destroy_bitmap(msg_menu_bmp_buf);
	destroy_bitmap(msg_bg_bmp_buf);
	destroy_bitmap(msg_portrait_bmp_buf);
	set_clip_state(pricesdisplaybuf, 1);
	destroy_bitmap(pricesdisplaybuf);
	destroy_bitmap(zcmouse[0]);
	destroy_bitmap(script_menu_buf);
	destroy_bitmap(f6_menu_buf);
	destroy_bitmap(darkscr_bmp_curscr);
	destroy_bitmap(darkscr_bmp_curscr_trans);
	destroy_bitmap(darkscr_bmp_scrollscr);
	destroy_bitmap(darkscr_bmp_scrollscr_trans);
	destroy_bitmap(lightbeam_bmp);
		
	al_trace("SFX... \n");
	zcmusic_exit();
	zcmixer_exit(zcmixer);
	
	for(int32_t i=0; i<WAV_COUNT; i++)
	{
		delete [] sfx_string[i];
		
		if(customsfxdata[i].data!=NULL)
		{
//      delete [] customsfxdata[i].data;
			free(customsfxdata[i].data);
		}
	}
	
	al_trace("Misc... \n");
	
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
	
	al_trace("Script buffers... \n");
	
	for(int32_t i=0; i<NUMSCRIPTFFC; i++)
	{
		if(ffscripts[i]!=NULL) delete ffscripts[i];
		ffscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTITEM; i++)
	{
		if(itemscripts[i]!=NULL) delete itemscripts[i];
		itemscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
	{
		if(guyscripts[i]!=NULL) delete guyscripts[i];
		guyscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		if(ewpnscripts[i]!=NULL) delete ewpnscripts[i];
		ewpnscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		if(lwpnscripts[i]!=NULL) delete lwpnscripts[i];
		lwpnscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
	{
		if(screenscripts[i]!=NULL) delete screenscripts[i];
		screenscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTGLOBAL; i++)
	{
		if(globalscripts[i]!=NULL) delete globalscripts[i];
		globalscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTPLAYER; i++)
	{
		if(playerscripts[i]!=NULL) delete playerscripts[i];
		playerscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
	{
		if(dmapscripts[i]!=NULL) delete dmapscripts[i];
		dmapscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
	{
		if(itemspritescripts[i]!=NULL) delete itemspritescripts[i];
		itemspritescripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
	{
		if(comboscripts[i]!=NULL) delete comboscripts[i];
		comboscripts[i] = NULL;
	}
	for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
	{
		if(subscreenscripts[i]!=NULL) delete subscreenscripts[i];
		subscreenscripts[i] = NULL;
	}
	
	delete zscriptDrawingRenderTarget;
	
	al_trace("Deleting quest buffers... \n");
	del_qst_buffers();
	
	if(qstdir) free(qstdir);
	
	if(qstpath) free(qstpath);

	FFCore.shutdown();
}

bool isSideViewGravity(int32_t t)
{
	return (((tmpscr[t].flags7 & fSIDEVIEW)!=0) != (DMaps[currdmap].sideview));
}

bool isSideViewHero(int32_t t)
{
	return !ignoreSideview && isSideViewGravity(t);
}


int32_t d_timer_proc(int32_t, DIALOG *, int32_t)
{
    return D_O_K;
}

#ifdef __EMSCRIPTEN__
extern "C" void get_shareable_url()
{
	EM_ASM({
		ZC.setShareableUrl({test: UTF8ToString($0), dmap: $1, screen: $2});
	}, qstpath, currdmap, currscr);
}
#endif

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
				&& itemsbuf[current_item_id(itype_bombbag)].flags & ITEM_FLAG1)
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
	bool ret = true;
	if(!checkTime || !id.magiccosttimer[0] || !(frame%id.magiccosttimer[0]))
		ret = checkCost(id.cost_counter[0], id.cost_amount[0]);
	if(!ret) return false;
	if(!checkTime || !id.magiccosttimer[1] || !(frame%id.magiccosttimer[1]))
		ret = checkCost(id.cost_counter[1], id.cost_amount[1]);
	return ret;
}

void payCost(int32_t ctr, int32_t amnt, int32_t tmr, bool ignoreTimer)
{
	if(!game) return;
	if(!amnt) return;
	if(tmr > 0 && !ignoreTimer && (frame%tmr))
		return;
	bool cost = amnt > 0;
	switch(ctr)
	{
		case crMAGIC:
		{
			if(!get_qr(qr_ENABLEMAGIC))
				return;
			if(cost && current_item_power(itype_magicring) > 0)
				return;
			if(cost)
				amnt *= game->get_magicdrainrate();
			break;
		}
		case crMONEY:
		{
			if(!cost) break;
			if ( current_item_power(itype_wallet) )
				return;
			if(get_qr(qr_OLDINFMAGIC) && current_item_power(itype_magicring) > 0)
				return;
			break;
		}
		case crARROWS:
		{
			if(cost && current_item_power(itype_quiver))
				return;
			if(!get_qr(qr_TRUEARROWS))
				return payCost(crMONEY, amnt, tmr, ignoreTimer);
			break;
		}
		case crBOMBS:
		{
			if(cost && current_item_power(itype_bombbag))
				return;
			break;
		}
		case crSBOMBS:
		{
			if(cost && current_item_power(itype_bombbag)
				&& itemsbuf[current_item_id(itype_bombbag)].flags & ITEM_FLAG1)
				return;
			break;
		}
	}
	game->change_counter(-amnt, ctr);
}
void paymagiccost(int32_t itemid, bool ignoreTimer, bool onlyTimer)
{
	if(itemid < 0)
	{
		return;
	}
	itemdata const& id = itemsbuf[itemid];
	if(!(id.flags&ITEM_VALIDATEONLY) && (!onlyTimer || id.magiccosttimer[0]))
		payCost(id.cost_counter[0], id.cost_amount[0], id.magiccosttimer[0], ignoreTimer);
	if(!(id.flags&ITEM_VALIDATEONLY2) && (!onlyTimer || id.magiccosttimer[1]))
		payCost(id.cost_counter[1], id.cost_amount[1], id.magiccosttimer[1], ignoreTimer);
}

std::string getComboTypeHelpText(int32_t id) { return ""; }
std::string getMapFlagHelpText(int32_t id) { return ""; }

string get_box_cfg_hdr(int num)
{
	return "misc";
}

