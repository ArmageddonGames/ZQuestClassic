#include "zc/zc_sys.h"

#include "allegro/gfx.h"
#include "allegro/gui.h"
#include "allegro5/joystick.h"
#include "base/files.h"
#include "base/render.h"
#include "zalleg/zalleg.h"
#include "base/qrs.h"
#include "base/dmap.h"
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <math.h>
#include <map>
#include <filesystem>
#include <ctype.h>
#include <sstream>
#include "base/version.h"
#include "base/zc_alleg.h"
#include "gamedata.h"
#include "zc/replay_upload.h"
#include "zc/zc_init.h"
#include "init.h"
#include "zc/replay.h"
#include "zc/cheats.h"
#include "zc/render.h"
#include "base/zc_math.h"
#include "base/zapp.h"
#include "dialog/cheatkeys.h"
#include "metadata/metadata.h"
#include "zc/zelda.h"
#include "zc/saves.h"
#include "tiles.h"
#include "base/colors.h"
#include "pal.h"
#include "base/zsys.h"
#include "base/qst.h"
#include "zc/zc_sys.h"
#include "play_midi.h"
#include "gui/jwin_a5.h"
#include "base/jwinfsel.h"
#include "base/gui.h"
#include "midi.h"
#include "subscr.h"
#include "zc/maps.h"
#include "sprite.h"
#include "zc/guys.h"
#include "zc/hero.h"
#include "zc/title.h"
#include "particles.h"
#include "sound/zcmusic.h"
#include "zconsole.h"
#include "zc/ffscript.h"
#include "dialog/info.h"
#include "dialog/alert.h"
#include "zc/combos.h"
#include "zc/jit.h"
#include "zc/zc_subscr.h"
#include <fmt/format.h>
#include "zconsole/ConsoleLogger.h"
#include "zinfo.h"
#include "base/misctypes.h"
#include "music_playback.h"
#include <base/new_menu.h>
#include <base/files.h>

#ifdef __EMSCRIPTEN__
#include "base/emscripten_utils.h"
#endif

using namespace std::chrono_literals;

extern bool Playing;
int32_t sfx_voice[WAV_COUNT];
int32_t d_stringloader(int32_t msg,DIALOG *d,int32_t c);
int32_t d_midilist_proc(int32_t msg,DIALOG *d,int32_t c);

static ALLEGRO_JOYSTICK* gamepad_dlg_cur_joystick;
static int32_t d_joylist_proc(int32_t msg,DIALOG *d,int32_t c);

extern byte monochrome_console;

extern HeroClass Hero;
extern zcmodule moduledata;
extern sprite_list  guys, items, Ewpns, Lwpns, chainlinks, decorations;
extern particle_list particles;
extern int32_t loadlast;
extern char *sfx_string[WAV_COUNT];
byte use_dwm_flush;
byte use_save_indicator;
int32_t paused_midi_pos = 0;
byte midi_suspended = 0;
byte zc_192b163_warp_compatibility;
char modulepath[2048];
bool epilepsyFlashReduction;
signed char pause_in_background_menu_init = 0;
byte pause_in_background = 0;
bool is_sys_pal = false;
static bool load_control_called_this_frame;
extern PALETTE* hw_palette;
extern bool update_hw_pal;
extern const char* dmaplist(int32_t index, int32_t* list_size);
int32_t getnumber(const char *prompt,int32_t initialval);

extern bool kb_typing_mode; //script only, for disbaling key presses affecting Hero, etc. 
extern int32_t cheat_modifier_keys[4]; //two options each, default either control and either shift

static  const char *qst_module_name = "current_module";
#ifdef ALLEGRO_LINUX
static  const char *samplepath = "samplesoundset/patches.dat";
#endif
char qst_files_path[2048];

extern TopMenu the_player_menu;
#ifdef _MSC_VER
#define getcwd _getcwd
#endif

bool rF11();
bool rI();
bool rQ();
bool zc_key_pressed();

#ifdef _WIN32

// This should only be necessary for MinGW, since it doesn't have a dwmapi.h. Add another #ifdef if you like.
extern "C"
{
	typedef HRESULT(WINAPI *t_DwmFlush)();
	typedef HRESULT(WINAPI *t_DwmIsCompositionEnabled)(BOOL *pfEnabled);
}

void do_DwmFlush()
{
	static HMODULE shell = LoadLibrary("dwmapi.dll");
	
	if(!shell)
		return;
		
	static t_DwmFlush flush=reinterpret_cast<t_DwmFlush>(GetProcAddress(shell, "DwmFlush"));
	static t_DwmIsCompositionEnabled isEnabled=reinterpret_cast<t_DwmIsCompositionEnabled>(GetProcAddress(shell, "DwmIsCompositionEnabled"));
	
	BOOL enabled;
	isEnabled(&enabled);
	
	if(isEnabled)
		flush();
}

#endif // _WIN32

void zc_exit(int code)
{
	extern CConsoleLoggerEx zscript_coloured_console;

	set_is_exiting();

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
	save_game_configs();

	zscript_coloured_console.kill();
	jit_shutdown();
	quit_game();

	Z_message("ZQuest Classic web site: https://zquestclassic.com\n");
	Z_message("ZQuest Classic old wiki: https://web.archive.org/web/20210910193102/https://zeldaclassic.com/wiki\n");
	Z_message("ZQuest Classic new wiki: https://github.com/ZQuestClassic/ZQuestClassic/wiki\n");

	allegro_exit();
	exit(code);
}

bool flash_reduction_enabled(bool check_qr)
{
	return (check_qr && get_qr(qr_EPILEPSY)) || epilepsyFlashReduction || replay_is_debug();
}

// Dialogue largening
void large_dialog(DIALOG *d)
{
	large_dialog(d, 1.5);
}

void large_dialog(DIALOG *d, float RESIZE_AMT)
{
	if(!d[0].d1)
	{
		d[0].d1 = 1;
		int32_t oldwidth = d[0].w;
		int32_t oldheight = d[0].h;
		int32_t oldx = d[0].x;
		int32_t oldy = d[0].y;
		d[0].x -= int32_t(d[0].w/RESIZE_AMT);
		d[0].y -= int32_t(d[0].h/RESIZE_AMT);
		d[0].w = int32_t(d[0].w*RESIZE_AMT);
		d[0].h = int32_t(d[0].h*RESIZE_AMT);
		
		for(int32_t i=1; d[i].proc !=NULL; i++)
		{
			// Place elements horizontally
			double xpc = ((double)(d[i].x - oldx) / (double)oldwidth);
			d[i].x = int32_t(d[0].x + (xpc*d[0].w));
			
			if(d[i].proc != d_stringloader)
			{
				if(d[i].proc==d_bitmap_proc)
				{
					d[i].w *= 2;
				}
				else d[i].w = int32_t(d[i].w*RESIZE_AMT);
			}
			
			// Place elements vertically
			double ypc = ((double)(d[i].y - oldy) / (double)oldheight);
			d[i].y = int32_t(d[0].y + (ypc*d[0].h));
			
			// Vertically resize elements
			if(d[i].proc == jwin_edit_proc || d[i].proc == jwin_check_proc || d[i].proc == jwin_checkfont_proc)
			{
				d[i].h = int32_t((double)d[i].h*1.5);
			}
			else if(d[i].proc == jwin_droplist_proc || d[i].proc == d_joylist_proc)
			{
				d[i].y += int32_t((double)d[i].h*0.25);
				d[i].h = int32_t((double)d[i].h*1.25);
			}
			else if(d[i].proc==d_bitmap_proc)
			{
				d[i].h *= 2;
			}
			else d[i].h = int32_t(d[i].h*RESIZE_AMT);
			
			// Fix frames
			if(d[i].proc == jwin_frame_proc)
			{
				d[i].x++;
				d[i].y++;
				d[i].w-=4;
				d[i].h-=4;
			}
		}
	}
	
	for(int32_t i=1; d[i].proc!=NULL; i++)
	{
		if(d[i].proc==jwin_slider_proc)
			continue;
			
		// Bigger font
		bool bigfontproc = (d[i].proc != d_midilist_proc && d[i].proc != jwin_droplist_proc && d[i].proc != jwin_abclist_proc && d[i].proc != jwin_list_proc && d[i].proc != d_joylist_proc);
		
		if(!d[i].dp2 && bigfontproc)
		{
			d[i].dp2 = get_zc_font(font_lfont_l);
		}
		else if(!bigfontproc)
		{
			((ListData *)d[i].dp)->font = &a4fonts[font_lfont_l];
		}
		
		// Make checkboxes work
		if(d[i].proc == jwin_check_proc)
			d[i].proc = jwin_checkfont_proc;
		else if(d[i].proc == jwin_radio_proc)
			d[i].proc = jwin_radiofont_proc;
	}
	
	jwin_center_dialog(d);
}

static char cfg_sect[] = "zeldadx"; //We need to rename this.
static char ctrl_sect[] = "Controls";
static char sfx_sect[] = "Volume";

int32_t d_dummy_proc(int32_t,DIALOG *,int32_t)
{
	return D_O_K;
}

bool is_reserved_key(int c)
{
	switch(c)
	{
		case KEY_ESC:
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
bool checkcheat(Cheat cheat)
{
	if(cheatkeys[cheat][0] && zc_readkey(cheatkeys[cheat][0]))
		return true; //Main key pressed
	if(cheatkeys[cheat][1] && zc_readkey(cheatkeys[cheat][1]))
		return true; //Alt key pressed
	return false;
}
void load_default_cheatkeys()
{
	memset(cheatkeys, 0, sizeof(cheatkeys));
	cheatkeys[Cheat::Life][0] = KEY_H;
	cheatkeys[Cheat::Life][1] = KEY_ASTERISK;
	cheatkeys[Cheat::Magic][0] = KEY_M;
	cheatkeys[Cheat::Magic][1] = KEY_SLASH_PAD;
	cheatkeys[Cheat::Rupies][0] = KEY_R;
	cheatkeys[Cheat::Bombs][0] = KEY_B;
	cheatkeys[Cheat::Arrows][0] = KEY_A;
	cheatkeys[Cheat::Clock][0] = KEY_I;
	cheatkeys[Cheat::Walls][0] = KEY_F11;
	cheatkeys[Cheat::Fast][0] = KEY_Q;
	cheatkeys[Cheat::Light][0] = KEY_L;
	cheatkeys[Cheat::IgnoreSideView][0] = KEY_V;
	cheatkeys[Cheat::Kill][0] = KEY_K;
	cheatkeys[Cheat::GoTo][0] = KEY_G;
	cheatkeys[Cheat::TrigSecrets][0] = KEY_S;
	cheatkeys[Cheat::ShowL0][0] = KEY_0;
	cheatkeys[Cheat::ShowL1][0] = KEY_1;
	cheatkeys[Cheat::ShowL2][0] = KEY_2;
	cheatkeys[Cheat::ShowL3][0] = KEY_3;
	cheatkeys[Cheat::ShowL4][0] = KEY_4;
	cheatkeys[Cheat::ShowL5][0] = KEY_5;
	cheatkeys[Cheat::ShowL6][0] = KEY_6;
	cheatkeys[Cheat::ShowFFC][0] = KEY_7;
	cheatkeys[Cheat::ShowSprites][0] = KEY_8;
	cheatkeys[Cheat::ShowWalkability][0] = KEY_W;
	cheatkeys[Cheat::ShowEffects][0] = KEY_E;
	cheatkeys[Cheat::ShowOverhead][0] = KEY_O;
	cheatkeys[Cheat::ShowPushblock][0] = KEY_P;
	cheatkeys[Cheat::ShowHitbox][0] = KEY_C;
	cheatkeys[Cheat::ShowFFCScripts][0] = KEY_F;
}

static bool loaded_game_configs;

void load_game_configs()
{
	loaded_game_configs = true;
	strcpy(moduledata.module_name,zc_get_config("ZCMODULE",qst_module_name,"modules/classic.zmod"));
	joystick_index = zc_get_config(ctrl_sect,"joystick_index",0);
	js_stick_1_x_stick = zc_get_config(ctrl_sect,"js_stick_1_x_stick",0);
	js_stick_1_x_axis = zc_get_config(ctrl_sect,"js_stick_1_x_axis",0);
	js_stick_1_x_offset = zc_get_config(ctrl_sect,"js_stick_1_x_offset",0) ? 128 : 0;
	js_stick_1_y_stick = zc_get_config(ctrl_sect,"js_stick_1_y_stick",0);
	js_stick_1_y_axis = zc_get_config(ctrl_sect,"js_stick_1_y_axis",1);
	js_stick_1_y_offset = zc_get_config(ctrl_sect,"js_stick_1_y_offset",0) ? 128 : 0;
	js_stick_2_x_stick = zc_get_config(ctrl_sect,"js_stick_2_x_stick",1);
	js_stick_2_x_axis = zc_get_config(ctrl_sect,"js_stick_2_x_axis",0);
	js_stick_2_x_offset = zc_get_config(ctrl_sect,"js_stick_2_x_offset",0) ? 128 : 0;
	js_stick_2_y_stick = zc_get_config(ctrl_sect,"js_stick_2_y_stick",1);
	js_stick_2_y_axis = zc_get_config(ctrl_sect,"js_stick_2_y_axis",1);
	js_stick_2_y_offset = zc_get_config(ctrl_sect,"js_stick_2_y_offset",0) ? 128 : 0;
	analog_movement = (zc_get_config(ctrl_sect,"analog_movement",1));
   
	//cheat modifier keya
	cheat_modifier_keys[0] = zc_get_config(ctrl_sect,"key_cheatmod_a1",KEY_ZC_LCONTROL);
	cheat_modifier_keys[1] = zc_get_config(ctrl_sect,"key_cheatmod_a2",0);
	cheat_modifier_keys[2] = zc_get_config(ctrl_sect,"key_cheatmod_b1",KEY_ZC_RCONTROL);
	cheat_modifier_keys[3] = zc_get_config(ctrl_sect,"key_cheatmod_b2",0);
	
	//cheat keys
	load_default_cheatkeys();
	char buf[256];
	for(size_t q = 1; q < Cheat::Last; ++q)
	{
		if(!bindable_cheat((Cheat)q)) continue;
		std::string cheatname = cheat_to_string((Cheat)q);
		util::lowerstr(cheatname);
		sprintf(buf, "key_cheat_%s_main", cheatname.c_str());
		cheatkeys[q][0] = zc_get_config(ctrl_sect,buf,cheatkeys[q][0]);
		sprintf(buf, "key_cheat_%s_alt", cheatname.c_str());
		cheatkeys[q][1] = zc_get_config(ctrl_sect,buf,cheatkeys[q][1]);
	}
   
	if((uint32_t)joystick_index >= MAX_JOYSTICKS)
		joystick_index = 0;
	   
	Akey = zc_get_config(ctrl_sect,"key_a",KEY_Z);
	Bkey = zc_get_config(ctrl_sect,"key_b",KEY_X);
	Skey = zc_get_config(ctrl_sect,"key_s",KEY_ENTER);
	Lkey = zc_get_config(ctrl_sect,"key_l",KEY_Q);
	Rkey = zc_get_config(ctrl_sect,"key_r",KEY_W);
	Pkey = zc_get_config(ctrl_sect,"key_p",KEY_SPACE);
	Exkey1 = zc_get_config(ctrl_sect,"key_ex1",KEY_A);
	Exkey2 = zc_get_config(ctrl_sect,"key_ex2",KEY_S);
	Exkey3 = zc_get_config(ctrl_sect,"key_ex3",KEY_D);
	Exkey4 = zc_get_config(ctrl_sect,"key_ex4",KEY_C);
   
	DUkey = zc_get_config(ctrl_sect,"key_up",   KEY_UP);
	DDkey = zc_get_config(ctrl_sect,"key_down", KEY_DOWN);
	DLkey = zc_get_config(ctrl_sect,"key_left", KEY_LEFT);
	DRkey = zc_get_config(ctrl_sect,"key_right",KEY_RIGHT);
   
	Abtn = zc_get_config(ctrl_sect,"btn_a",2);
	Bbtn = zc_get_config(ctrl_sect,"btn_b",1);
	Sbtn = zc_get_config(ctrl_sect,"btn_s",10);
	Mbtn = zc_get_config(ctrl_sect,"btn_m",9);
	Lbtn = zc_get_config(ctrl_sect,"btn_l",5);
	Rbtn = zc_get_config(ctrl_sect,"btn_r",6);
	Pbtn = zc_get_config(ctrl_sect,"btn_p",12);
	Exbtn1 = zc_get_config(ctrl_sect,"btn_ex1",7);
	Exbtn2 = zc_get_config(ctrl_sect,"btn_ex2",8);
	Exbtn3 = zc_get_config(ctrl_sect,"btn_ex3",4);
	Exbtn4 = zc_get_config(ctrl_sect,"btn_ex4",3);
   
	DUbtn = zc_get_config(ctrl_sect,"btn_up",13);
	DDbtn = zc_get_config(ctrl_sect,"btn_down",14);
	DLbtn = zc_get_config(ctrl_sect,"btn_left",15);
	DRbtn = zc_get_config(ctrl_sect,"btn_right",16);
	
	epilepsyFlashReduction = zc_get_config(cfg_sect,"epilepsy_flash_reduction",0);
	
	midi_volume = zc_get_config(sfx_sect,"midi",255);
	sfx_volume = zc_get_config(sfx_sect,"sfx",255);
	emusic_volume = zc_get_config(sfx_sect,"emusic",255);
	pan_style = zc_get_config(sfx_sect,"pan",1);
	volkeys = zc_get_config(sfx_sect,"volkeys",0)!=0;
	zc_vsync = zc_get_config(cfg_sect,"vsync",0);
	Throttlefps = zc_get_config(cfg_sect,"throttlefps",1)!=0;
	Maxfps = zc_get_config(cfg_sect,"maxfps",0);
	TransLayers = zc_get_config(cfg_sect,"translayers",1)!=0;
	SnapshotFormat = zc_get_config(cfg_sect,"snapshot_format",3);
	SnapshotScale = zc_get_config(cfg_sect,"snapshot_scale",2);
	NameEntryMode = zc_get_config(cfg_sect,"name_entry_mode",0);
#ifdef __EMSCRIPTEN__
	if (em_is_mobile()) NameEntryMode = 2;
#endif
	ShowFPS = zc_get_config(cfg_sect,"showfps",0)!=0;
	ShowGameTime = zc_get_config(cfg_sect,"showtime",0);
	NESquit = zc_get_config(cfg_sect,"fastquit",0)!=0;
	ClickToFreeze = zc_get_config(cfg_sect,"clicktofreeze",1)!=0;
	abc_patternmatch = zc_get_config(cfg_sect, "lister_pattern_matching", 1);
	pause_in_background = zc_get_config(cfg_sect, "pause_in_background", 0);
	
	window_width = resx = zc_get_config(cfg_sect,"window_width",-1);
	window_height = resy = zc_get_config(cfg_sect,"window_height",-1);
	SaveDragResize = zc_get_config(cfg_sect,"save_drag_resize",0)!=0;
	DragAspect = zc_get_config(cfg_sect,"drag_aspect",0)!=0;
	SaveWinPos = zc_get_config(cfg_sect,"save_window_position",0)!=0;
	scaleForceInteger = zc_get_config("zeldadx","scaling_force_integer",1)!=0;
	stretchGame = zc_get_config("zeldadx","stretch_game_area",0)!=0;
	
	loadlast = zc_get_config(cfg_sect,"load_last",0);
	
	fullscreen = zc_get_config(cfg_sect,"fullscreen",0);
	
	info_opacity = zc_get_config("zc","debug_info_opacity",255);
#ifdef _WIN32
	console_enabled = (byte) zc_get_config("CONSOLE", "enabled", 0);
	//use_win7_keyboard_fix = (byte) zc_get_config(cfg_sect,"use_win7_key_fix",0);
	use_win32_proc = (byte) zc_get_config(cfg_sect,"zc_win_proc_fix",0); //buggy
   
	// This one's for Aero
	use_dwm_flush = (byte) zc_get_config("zeldadx","use_dwm_flush",0);
   
	monochrome_console = (byte) zc_get_config("CONSOLE","monochrome_debuggers",0);
#else //UNIX
	console_enabled = (byte) zc_get_config("CONSOLE", "enabled", 0);
	monochrome_console = (byte) zc_get_config("CONSOLE","monochrome_debuggers",0);
#endif
	clearConsoleOnLoad = zc_get_config("CONSOLE","clear_console_on_load",1)!=0;
	clearConsoleOnReload = zc_get_config("CONSOLE","clear_console_on_reload",0)!=0;

	strcpy(qstdir,zc_get_config(cfg_sect,"quest_dir","quests"));
	strcpy(qstpath,qstdir); //qstpath is the local (for this run of ZC) quest path, qstdir is the universal quest dir.
	ss_enable = zc_get_config(cfg_sect,"ss_enable",1) ? 1 : 0;
	ss_after = vbound(zc_get_config(cfg_sect,"ss_after",14), 0, 14);
	ss_speed = vbound(zc_get_config(cfg_sect,"ss_speed",2), 0, 6);
	ss_density = vbound(zc_get_config(cfg_sect,"ss_density",3), 0, 6);
	heart_beep = zc_get_config(cfg_sect,"heart_beep",0)!=0;
	//gui_colorset = zc_get_config(cfg_sect,"gui_colorset",0);
	sfxdat = zc_get_config(cfg_sect,"use_sfx_dat",1); // TODO: sfxdat is always set to 1 for titlescreen... and 0 in readsfx... so this cfg is pointless, remove?
	fullscreen = zc_get_config(cfg_sect,"fullscreen",0);
	use_save_indicator = zc_get_config(cfg_sect,"save_indicator",0);
	zc_192b163_warp_compatibility = zc_get_config(cfg_sect,"zc_192b163_warp_compatibility",0);
	SkipTitle = zc_get_config(cfg_sect,"skip_title",0);
}

void save_control_configs(bool kb)
{
	if(kb)
	{
		zc_set_config(ctrl_sect,"key_cheatmod_a1",cheat_modifier_keys[0]);
		zc_set_config(ctrl_sect,"key_cheatmod_a2",cheat_modifier_keys[1]);
		zc_set_config(ctrl_sect,"key_cheatmod_b1",cheat_modifier_keys[2]);
		zc_set_config(ctrl_sect,"key_cheatmod_b2",cheat_modifier_keys[3]);
		
		if (!replay_is_replaying())
		{
			zc_set_config(ctrl_sect,"key_a",Akey);
			zc_set_config(ctrl_sect,"key_b",Bkey);
			zc_set_config(ctrl_sect,"key_s",Skey);
			zc_set_config(ctrl_sect,"key_l",Lkey);
			zc_set_config(ctrl_sect,"key_r",Rkey);
			zc_set_config(ctrl_sect,"key_p",Pkey);
			zc_set_config(ctrl_sect,"key_ex1",Exkey1);
			zc_set_config(ctrl_sect,"key_ex2",Exkey2);
			zc_set_config(ctrl_sect,"key_ex3",Exkey3);
			zc_set_config(ctrl_sect,"key_ex4",Exkey4);
			zc_set_config(ctrl_sect,"key_up",   DUkey);
			zc_set_config(ctrl_sect,"key_down", DDkey);
			zc_set_config(ctrl_sect,"key_left", DLkey);
			zc_set_config(ctrl_sect,"key_right",DRkey);
		}
	}
	else
	{
		zc_set_config(ctrl_sect,"joystick_index",joystick_index);
		zc_set_config(ctrl_sect,"js_stick_1_x_stick",js_stick_1_x_stick);
		zc_set_config(ctrl_sect,"js_stick_1_x_axis",js_stick_1_x_axis);
		zc_set_config(ctrl_sect,"js_stick_1_x_offset",js_stick_1_x_offset ? 1 : 0);
		zc_set_config(ctrl_sect,"js_stick_1_y_stick",js_stick_1_y_stick);
		zc_set_config(ctrl_sect,"js_stick_1_y_axis",js_stick_1_y_axis);
		zc_set_config(ctrl_sect,"js_stick_1_y_offset",js_stick_1_y_offset ? 1 : 0);
		zc_set_config(ctrl_sect,"js_stick_2_x_stick",js_stick_2_x_stick);
		zc_set_config(ctrl_sect,"js_stick_2_x_axis",js_stick_2_x_axis);
		zc_set_config(ctrl_sect,"js_stick_2_x_offset",js_stick_2_x_offset ? 1 : 0);
		zc_set_config(ctrl_sect,"js_stick_2_y_stick",js_stick_2_y_stick);
		zc_set_config(ctrl_sect,"js_stick_2_y_axis",js_stick_2_y_axis);
		zc_set_config(ctrl_sect,"js_stick_2_y_offset",js_stick_2_y_offset ? 1 : 0);
		zc_set_config(ctrl_sect,"analog_movement",analog_movement);
		
		zc_set_config(ctrl_sect,"btn_a",Abtn);
		zc_set_config(ctrl_sect,"btn_b",Bbtn);
		zc_set_config(ctrl_sect,"btn_s",Sbtn);
		zc_set_config(ctrl_sect,"btn_m",Mbtn);
		zc_set_config(ctrl_sect,"btn_l",Lbtn);
		zc_set_config(ctrl_sect,"btn_r",Rbtn);
		zc_set_config(ctrl_sect,"btn_p",Pbtn);
		zc_set_config(ctrl_sect,"btn_ex1",Exbtn1);
		zc_set_config(ctrl_sect,"btn_ex2",Exbtn2);
		zc_set_config(ctrl_sect,"btn_ex3",Exbtn3);
		zc_set_config(ctrl_sect,"btn_ex4",Exbtn4);
		
		zc_set_config(ctrl_sect,"btn_up",DUbtn);
		zc_set_config(ctrl_sect,"btn_down",DDbtn);
		zc_set_config(ctrl_sect,"btn_left",DLbtn);
		zc_set_config(ctrl_sect,"btn_right",DRbtn);
	}
}

void save_cheatkeys()
{
	char buf[256];
	for(size_t q = 1; q < Cheat::Last; ++q)
	{
		if(!bindable_cheat((Cheat)q)) continue;
		std::string cheatname = cheat_to_string((Cheat)q);
		util::lowerstr(cheatname);
		sprintf(buf, "key_cheat_%s_main", cheatname.c_str());
		zc_set_config(ctrl_sect,buf,cheatkeys[q][0]);
		sprintf(buf, "key_cheat_%s_alt", cheatname.c_str());
		if(cheatkeys[q][1])
			zc_set_config(ctrl_sect,buf,cheatkeys[q][1]);
		else zc_set_config(ctrl_sect,buf,(char*)nullptr);
	}
}

void save_game_configs()
{
	if (!loaded_game_configs) return;

	packfile_password("");
	
	zc_set_config("ZCMODULE",qst_module_name,moduledata.module_name);
	
	if (all_get_display() && !all_get_fullscreen_flag()&& SaveWinPos)
	{
		int o_window_x, o_window_y;
		al_get_window_position(all_get_display(), &o_window_x, &o_window_y);
		zc_set_config(cfg_sect,"window_x",o_window_x);
		zc_set_config(cfg_sect,"window_y",o_window_y);
	}
	
	if (all_get_display() && !all_get_fullscreen_flag() && SaveDragResize)
	{
		window_width = al_get_display_width(all_get_display());
		window_height = al_get_display_height(all_get_display());
		zc_set_config(cfg_sect,"window_width",window_width);
		zc_set_config(cfg_sect,"window_height",window_height);
	}
	
	zc_set_config(cfg_sect,"load_last",loadlast);
	zc_set_config(cfg_sect,"use_sfx_dat",sfxdat);
	
	flush_config_file();
#ifdef __EMSCRIPTEN__
	em_sync_fs();
#endif
}

//----------------------------------------------------------------

// Timers

void fps_callback()
{
	lastfps=framecnt;
	framecnt=0;
}

END_OF_FUNCTION(fps_callback)

int32_t Z_init_timers()
{
	static bool didit = false;
	const static char *err_str = "Couldn't allocate timer";
	err_str = err_str; //Unused variable warning
	
	if(didit)
		return 1;
		
	didit = true;
	
	LOCK_VARIABLE(lastfps);
	LOCK_VARIABLE(framecnt);
	LOCK_FUNCTION(fps_callback);
	
	if(install_int_ex(fps_callback,SECS_TO_TIMER(1)))
		return 0;
		
	return 1;
}

void Z_remove_timers()
{
	remove_int(fps_callback);
}

//----------------------------------------------------------------

void go()
{
	blit(screen,tmp_scr,scrx,scry,0,0,screen->w,screen->h);
}

void comeback()
{
	blit(tmp_scr,screen,0,0,scrx,scry,screen->w,screen->h);
}

void dump_pal(BITMAP *dest)
{
	for(int32_t i=0; i<256; i++)
		rectfill(dest,(i&63)<<2,(i&0xFC0)>>4,((i&63)<<2)+3,((i&0xFC0)>>4)+3,i);
}

//----------------------------------------------------------------

int game_mouse_index = ZCM_BLANK;
static bool system_mouse = false;
bool sys_mouse()
{
	system_mouse = true;
	return MouseSprite::set(ZCM_NORMAL);
}
bool game_mouse()
{
	system_mouse = false;
	return MouseSprite::set(game_mouse_index);
}
void custom_mouse(BITMAP* bmp, int fx, int fy, bool sys_recolor, bool user_scale)
{
	if(!bmp)
		return;
	float scale = vbound(zc_get_config("zeldadx","cursor_scale_large",1.5),1.0,5.0);
	int scaledw = bmp->w*scale, scaledh = bmp->h*scale;
	if(bmp->w == scaledw && bmp->h == scaledh)
		user_scale = false;
	if(user_scale || sys_recolor)
	{
		if(!user_scale) scale = 1;
		BITMAP* tmpbmp = create_bitmap_ex(8,bmp->w*scale,bmp->h*scale);
		if(user_scale)
			stretch_blit(bmp, tmpbmp, 0, 0, bmp->w, bmp->h, 0, 0, tmpbmp->w, tmpbmp->h);
		else
			blit(bmp, tmpbmp, 0, 0, 0, 0, bmp->w, bmp->h);
		if(sys_recolor)
			recolor_mouse(tmpbmp);
		MouseSprite::assign(ZCM_CUSTOM, tmpbmp, fx*scale, fy*scale);
		destroy_bitmap(tmpbmp);
	}
	else
	{
		MouseSprite::assign(ZCM_CUSTOM, bmp, fx, fy);
	}
}

//Handles converting the mouse sprite from the .dat file
void recolor_mouse(BITMAP* bmp)
{
	for(int32_t x = 0; x < bmp->w; ++x)
	{
		for(int32_t y = 0; y < bmp->h; ++y)
		{
			int32_t color = getpixel(bmp, x, y);
			switch(color)
			{
				case dvc(1):
					color = jwin_pal[jcCURSORMISC];
					break;
				case dvc(2):
					color = jwin_pal[jcCURSOROUTLINE];
					break;
				case dvc(3):
					color = jwin_pal[jcCURSORLIGHT];
					break;
				case dvc(5):
					color = jwin_pal[jcCURSORDARK];
					break;
				default:
					continue;
			}
			putpixel(bmp, x, y, color);
		}
	}
}
void load_mouse()
{
	PALETTE pal;
	BITMAP* cursor_bitmap = load_bitmap("assets/cursor.bmp", pal);
	if (!cursor_bitmap)
		Z_error_fatal("Missing required file %s\n", "assets/cursor.bmp");

	enter_sys_pal();
	MouseSprite::set(-1);
	float scale = vbound(zc_get_config("zeldadx","cursor_scale_large",1.5),1.0,5.0);
	int32_t sz = 16*scale;
	for(int32_t j = 0; j < 1; ++j)
	{
		BITMAP* tmpbmp = create_bitmap_ex(8,16,16);
		if(zcmouse[j])
			destroy_bitmap(zcmouse[j]);
		zcmouse[j] = create_bitmap_ex(8,sz,sz);
		clear_bitmap(zcmouse[j]);
		clear_bitmap(tmpbmp);
		blit(cursor_bitmap,tmpbmp,1,j*17+1,0,0,16,16);
		recolor_mouse(tmpbmp);
		if(sz!=16)
			stretch_blit(tmpbmp, zcmouse[j], 0, 0, 16, 16, 0, 0, sz, sz);
		else
			blit(tmpbmp, zcmouse[j], 0, 0, 0, 0, 16, 16);
		destroy_bitmap(tmpbmp);
	}
	if(!hw_palette) hw_palette = &RAMpal;
	zc_set_palette(*hw_palette);
	
	BITMAP* blankmouse = create_bitmap_ex(8,16,16);
	clear_bitmap(blankmouse);
	
	MouseSprite::assign(ZCM_NORMAL, zcmouse[0], 1*scale, 1*scale);
	MouseSprite::assign(ZCM_BLANK, blankmouse);
	//Don't assign ZCM_CUSTOM. That'll be handled by scripts.
	
	//Reload the mouse
	if(system_mouse)
		sys_mouse();
	else game_mouse();
	
	destroy_bitmap(blankmouse);
	destroy_bitmap(cursor_bitmap);
	exit_sys_pal();
}

// sets the video mode and initializes the palette and mouse sprite
bool game_vid_mode(int32_t mode,int32_t wait)
{
	if (is_headless())
		return true;

	extern int zq_screen_w, zq_screen_h;
	if(set_gfx_mode(mode,resx,resy,zq_screen_w,zq_screen_h)!=0)
	{
		return false;
	}
	
	scrx = (resx-320)>>1;
	scry = (resy-240)>>1;
	for(int32_t q = 0; q < NUM_ZCMOUSE; ++q)
		zcmouse[q] = NULL;
	load_mouse();
	
	for(int32_t i=240; i<256; i++)
		RAMpal[i]=pal_gui[i];
		
	zc_set_palette(RAMpal);
	clear_to_color(screen,BLACK);
	
	rest(wait);
	return true;
}

void null_quest()
{
	std::string title_assets_path = "modules/classic/title_gfx.dat";
	if (get_last_loaded_qstpath() == title_assets_path)
		return;

	byte skip_flags[4];
	for (int i = 0; i < skip_max; i++)
		set_bit(skip_flags, i, 1);
	set_bit(skip_flags, skip_tiles, 0);
	set_bit(skip_flags, skip_csets, 0);
	set_bit(skip_flags, skip_misc, 0); // needed for miscsfx (skip this and `tests/replays/demons_inferno/demons_inferno_1_of_2.zplay` fails).
	loadquest(title_assets_path.c_str(), &QHeader, &QMisc, tunes+ZC_MIDI_COUNT, false, skip_flags, 0, false);
	sfxdat = 1;
	// TODO: sfx.dat is ~1.2 MB. Could be better to break that up into individual files and load on demand / not at startup.
	// TODO: can we cache the tiles/colordata so we don't have to read title_gfx.dat more than once?
	//       colordata is tiny, but tilebuf is huge, so limit that to just what the title screen needs.
	//       Another option: embed this data into the binary (`xxd -i resources/modules/classic/title_gfx.dat > title_gfx.h`)
}

void init_NES_mode()
{
	null_quest();
}

//----------------------------------------------------------------

qword trianglelines[16]=
{
	0x0000000000000000ULL,
	0xFD00000000000000ULL,
	0xFDFD000000000000ULL,
	0xFDFDFD0000000000ULL,
	0xFDFDFDFD00000000ULL,
	0xFDFDFDFDFD000000ULL,
	0xFDFDFDFDFDFD0000ULL,
	0xFDFDFDFDFDFDFD00ULL,
	0xFDFDFDFDFDFDFDFDULL,
	0x00FDFDFDFDFDFDFDULL,
	0x0000FDFDFDFDFDFDULL,
	0x000000FDFDFDFDFDULL,
	0x00000000FDFDFDFDULL,
	0x0000000000FDFDFDULL,
	0x000000000000FDFDULL,
	0x00000000000000FDULL,
};

word screen_triangles[28][32];

// the ULL suffixes are to prevent this warning:
// warning: integer constant is too large for "int32_t" type

qword triangles[4][16][8]= //[direction][value][line]
{
	{
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL,
			0xFD00000000000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFD000000000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFD0000000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFD00000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFD000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFD0000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFD00ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		}
	},
	{
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL,
			0x0000000000000000ULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL,
			0x00000000000000FDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL,
			0x000000000000FDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x0000000000FDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x00000000FDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x000000FDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		}
	},
	{
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL
		},
		{
			0x0000000000000000ULL,
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL
		},
		{
			0xFD00000000000000ULL,
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFD000000000000ULL,
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFD0000000000ULL,
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFD00000000ULL,
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFD000000ULL,
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFD0000ULL,
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFD00ULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		}
	},
	{
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL
		},
		{
			0x0000000000000000ULL,
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL
		},
		{
			0x0000000000000000ULL,
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL
		},
		{
			0x00000000000000FDULL,
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x000000000000FDFDULL,
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x0000000000FDFDFDULL,
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x00000000FDFDFDFDULL,
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x000000FDFDFDFDFDULL,
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x0000FDFDFDFDFDFDULL,
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0x00FDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		},
		{
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL,
			0xFDFDFDFDFDFDFDFDULL
		}
	}
};

int32_t black_opening_count=0;
int32_t black_opening_x,black_opening_y;
int32_t black_opening_shape;

int32_t choose_opening_shape()
{
	// First, count how many bits are set
	int32_t numBits=0;
	int32_t bitCounter;
	
	for(int32_t i=0; i<bosMAX; i++)
	{
		if(COOLSCROLL&(1<<i))
			numBits++;
	}
	
	// Shouldn't happen...
	if(numBits==0)
		return bosCIRCLE;
		
	// Pick a bit
	bitCounter=zc_rand()%numBits+1;
	
	for(int32_t i=0; i<bosMAX; i++)
	{
		// If this bit is set, decrement the bit counter
		if(COOLSCROLL&(1<<i))
			bitCounter--;
			
		// When the counter hits 0, return a value based on
		// which bit it stopped on.
		// Reminder: enum {bosCIRCLE=0, bosOVAL, bosTRIANGLE, bosSMAS, bosFADEBLACK, bosMAX};
		if(bitCounter==0)
			return i;
	}
	
	// Shouldn't be necessary, but the compiler might complain, at least
	return bosCIRCLE;
}

void close_black_opening(int32_t x, int32_t y, bool wait, int32_t shape)
{
	black_opening_shape= (shape>-1 ? shape : choose_opening_shape());
	
	int32_t w=256, h=224;
	int32_t blockrows=28, blockcolumns=32;
	int32_t xoffset=(x-(w/2))/8, yoffset=(y-(h/2))/8;
	
	for(int32_t blockrow=0; blockrow<blockrows; ++blockrow)  //30
	{
		for(int32_t blockcolumn=0; blockcolumn<blockcolumns; ++blockcolumn)  //40
		{
			screen_triangles[blockrow][blockcolumn]=zc_max(abs(int32_t(double(blockcolumns-1)/2-blockcolumn+xoffset)),abs(int32_t(double(blockrows-1)/2-blockrow+yoffset)))|0x0100|((blockrow-yoffset<blockrows/2)?0:0x8000)|((blockcolumn-xoffset<blockcolumns/2)?0x4000:0);
		}
	}
	
	black_opening_count = 66;
	black_opening_x = x;
	black_opening_y = y;
	lensclk = 0;
	//black_opening_shape=(black_opening_shape+1)%bosMAX;
	
	
	if(black_opening_shape == bosFADEBLACK)
	{
		refreshTints();
		memcpy(tempblackpal, RAMpal, sizeof(RAMpal)); //Store palette in temp palette for fade effect
	}
	if(wait)
	{
		FFCore.warpScriptCheck();
		for(int32_t i=0; i<66; i++)
		{
			draw_screen(tmpscr);
			//put_passive_subscr(framebuf,0,passive_subscreen_offset,false,sspUP);
			advanceframe(true);
			
			if(Quit)
			{
				break;
			}
		}
	}
}

void open_black_opening(int32_t x, int32_t y, bool wait, int32_t shape)
{
	black_opening_shape= (shape>-1 ? shape : choose_opening_shape());
	
	int32_t w=256, h=224;
	int32_t blockrows=28, blockcolumns=32;
	int32_t xoffset=(x-(w/2))/8, yoffset=(y-(h/2))/8;
	
	for(int32_t blockrow=0; blockrow<blockrows; ++blockrow)  //30
	{
		for(int32_t blockcolumn=0; blockcolumn<blockcolumns; ++blockcolumn)  //40
		{
			screen_triangles[blockrow][blockcolumn]=zc_max(abs(int32_t(double(blockcolumns-1)/2-blockcolumn+xoffset)),abs(int32_t(double(blockrows-1)/2-blockrow+yoffset)))|0x0100|((blockrow-yoffset<blockrows/2)?0:0x8000)|((blockcolumn-xoffset<blockcolumns/2)?0x4000:0);
		}
	}
	
	black_opening_count = -66;
	black_opening_x = x;
	black_opening_y = y;
	lensclk = 0;
	if(black_opening_shape == bosFADEBLACK)
	{
		refreshTints();
		memcpy(tempblackpal, RAMpal, sizeof(RAMpal)); //Store palette in temp palette for fade effect
	}
	if(wait)
	{
		FFCore.warpScriptCheck();
		for(int32_t i=0; i<66; i++)
		{
			draw_screen(tmpscr);
			//put_passive_subscr(framebuf,0,passive_subscreen_offset,false,sspUP);
			advanceframe(true);
			
			if(Quit)
			{
				break;
			}
		}
	}
}

void black_opening(BITMAP *dest,int32_t x,int32_t y,int32_t a,int32_t max_a)
{
	clear_to_color(tmp_scr,BLACK);
	int32_t w=256, h=224;
	
	switch(black_opening_shape)
	{
	case bosOVAL:
	{
		double new_w=(w/2)+abs(w/2-x);
		double new_h=(h/2)+abs(h/2-y);
		double b=sqrt(((new_w*new_w)/4)+(new_h*new_h));
		ellipsefill(tmp_scr,x,y,int32_t(2*a*b/max_a)/8*8,int32_t(a*b/max_a)/8*8,0);
		break;
	}
	
	case bosTRIANGLE:
	{
		double new_w=(w/2)+abs(w/2-x);
		double new_h=(h/2)+abs(h/2-y);
		double r=a*(new_w*sqrt((double)3)+new_h)/max_a;
		double P2= (PI/2);
		double P23=(2*PI/3);
		double P43=(4*PI/3);
		double Pa= (-4*PI*a/(3*max_a));
		double angle=P2+Pa;
		double a0=angle;
		double a2=angle+P23;
		double a4=angle+P43;
		triangle(tmp_scr, x+int32_t(zc::math::Cos(a0)*r), y-int32_t(zc::math::Sin(a0)*r),
				 x+int32_t(zc::math::Cos(a2)*r), y-int32_t(zc::math::Sin(a2)*r),
				 x+int32_t(zc::math::Cos(a4)*r), y-int32_t(zc::math::Sin(a4)*r),
				 0);
		break;
	}
	
	case bosSMAS:
	{
		int32_t distance=zc_max(abs(w/2-x),abs(h/2-y))/8;
		
		for(int32_t blockrow=0; blockrow<28; ++blockrow)  //30
		{
			for(int32_t linerow=0; linerow<8; ++linerow)
			{
				qword *triangleline=(qword*)(tmp_scr->line[(blockrow*8+linerow)]);
				
				for(int32_t blockcolumn=0; blockcolumn<32; ++blockcolumn)  //40
				{
					*triangleline=triangles[(screen_triangles[blockrow][blockcolumn]&0xC000)>>14]
								  [zc_min(zc_max((((31+distance)*(max_a-a)/max_a)+((screen_triangles[blockrow][blockcolumn]&0x0FFF)-0x0100)-(15+distance)),0),15)]
								  [linerow];
					++triangleline;
					
					if(linerow==0)
					{
					}
				}
			}
		}
		
		break;
	}
	
	case bosFADEBLACK:
	{
		if(black_opening_count<0)
		{
			black_fade(zc_min(-black_opening_count,63));
		}
		else if(black_opening_count>0)
		{
			black_fade(63-zc_max(black_opening_count-3,0));
		}
		else black_fade(0);
		return; //no blitting from tmp_scr!
	}
	
	case bosCIRCLE:
	default:
	{
		double new_w=(w/2)+abs(w/2-x);
		double new_h=(h/2)+abs(h/2-y);
		int32_t r=int32_t(sqrt((new_w*new_w)+(new_h*new_h))*a/max_a);
		//circlefill(tmp_scr,x,y,a<<3,0);
		circlefill(tmp_scr,x,y,r,0);
		break;
	}
	}
	
	masked_blit(tmp_scr,dest,0,0,0,0,320,240);
}

// fadeamnt is 0-63
void black_fade(int32_t fadeamnt)
{
	fadeamnt = _rgb_scale_6[fadeamnt];
	for(int32_t i=0; i < 0xEF; i++)
	{
		RAMpal[i].r = vbound(tempblackpal[i].r-fadeamnt,0,255);
		RAMpal[i].g = vbound(tempblackpal[i].g-fadeamnt,0,255);
		RAMpal[i].b = vbound(tempblackpal[i].b-fadeamnt,0,255);
	}
	
	refreshpal = true;
}

//----------------------------------------------------------------

bool item_disabled(int32_t item)				 //is this item disabled?
{
	return (unsigned(item) < MAXITEMS && game->items_off[item] != 0);
}

bool can_use_item(int32_t item_type, int32_t item)				  //can Hero use this item?
{
	if(current_item(item_type, true) >=item)
	{
		return true;
	}
	
	return false;
}

bool has_item(int32_t item_type, int32_t it)						//does Hero possess this item?
{
	switch(item_type)
	{
		case itype_bomb:
		case itype_sbomb:
		{
			int32_t itemid = getItemID(itemsbuf, item_type, it);
			
			if(itemid == -1)
				return false;
				
			return (game->get_item(itemid));
		}
		
		case itype_clock:
		{
			int32_t itemid = getItemID(itemsbuf, item_type, it);
			
			if(itemid != -1 && (itemsbuf[itemid].flags & item_flag1)) //Active clock
				return (game->get_item(itemid));
			return Hero.getClock()?1:0;
		}
			
		case itype_key:
			return (game->get_keys()>0);
			
		case itype_magiccontainer:
			return (game->get_maxmagic()>=game->get_mp_per_block());
			
		case itype_triforcepiece:							   //it: -2=any, -1=current level, other=that level
		{
			switch(it)
			{
				case -2:
				{
					for(int32_t i=0; i<MAXLEVELS; i++)
					{
						if(game->lvlitems[i]&liTRIFORCE)
						{
							return true;
						}
					}
					
					return false;
				}
				
				case -1:
					return (game->lvlitems[dlevel]&liTRIFORCE);
					
				default:
					if(it>=0&&it<MAXLEVELS)
					{
						return (game->lvlitems[it]&liTRIFORCE);
					}
					
					break;
			}
			
			return 0;
		}
		
		case itype_map:										 //it: -2=any, -1=current level, other=that level
		{
			switch(it)
			{
				case -2:
				{
					for(int32_t i=0; i<MAXLEVELS; i++)
					{
						if(game->lvlitems[i]&liMAP)
						{
							return true;
						}
					}
					
					return false;
				}
				
				case -1:
					return (game->lvlitems[dlevel]&liMAP)!=0;
					
				default:
					if(it>=0&&it<MAXLEVELS)
					{
						return (game->lvlitems[it]&liMAP)!=0;
					}
					
					break;
			}
			
			return 0;
		}
		
		case itype_compass:									 //it: -2=any, -1=current level, other=that level
		{
			switch(it)
			{
				case -2:
				{
					for(int32_t i=0; i<MAXLEVELS; i++)
					{
						if(game->lvlitems[i]&liCOMPASS)
						{
							return true;
						}
					}
					
					return false;
				}
				
				case -1:
					return (game->lvlitems[dlevel]&liCOMPASS)!=0;
					
				default:
					if(it>=0&&it<MAXLEVELS)
					{
						return (game->lvlitems[it]&liCOMPASS)!=0;
					}
					
					break;
			}
			return 0;
		}
		
		case itype_bosskey:									 //it: -2=any, -1=current level, other=that level
		{
			switch(it)
			{
				case -2:
				{
					for(int32_t i=0; i<MAXLEVELS; i++)
					{
						if(game->lvlitems[i]&liBOSSKEY)
						{
							return true;
						}
					}
					
					return false;
				}
				
				case -1:
					return (game->lvlitems[dlevel]&liBOSSKEY)?1:0;
					
				default:
					if(it>=0&&it<MAXLEVELS)
					{
						return (game->lvlitems[it]&liBOSSKEY)?1:0;
					}
					break;
			}
			return 0;
		}
		
		default:
			int32_t itemid = getItemID(itemsbuf, item_type, it);
			
			if(itemid == -1)
				return false;
				
			return game->get_item(itemid);
	}
}

int current_item(int item_type, bool checkmagic, bool jinx_check, bool check_bunny)
{
	switch(item_type)
	{
		case itype_clock:
		{
			int maxid = current_item_id(item_type, checkmagic, jinx_check, check_bunny);
			
			if(maxid != -1 && (itemsbuf[maxid].flags & item_flag1)) //Active clock
				return itemsbuf[maxid].fam_type;
			
			return has_item(itype_clock,1) ? 1 : 0;
		}
			
		case itype_key:
			return game->get_keys();
			
		case itype_lkey:
			return game->lvlkeys[get_dlevel()];
			
		case itype_magiccontainer:
			return game->get_maxmagic()/game->get_mp_per_block();
			
		case itype_triforcepiece:
		{
			int count=0;
			
			for(int i=0; i<MAXLEVELS; i++)
			{
				count+=(game->lvlitems[i]&liTRIFORCE)?1:0;
			}
			
			return count;
		}
		
		case itype_map:
		{
			int count=0;
			
			for(int i=0; i<MAXLEVELS; i++)
			{
				count+=(game->lvlitems[i]&liMAP)?1:0;
			}
			
			return count;
		}
		
		case itype_compass:
		{
			int count=0;
			
			for(int i=0; i<MAXLEVELS; i++)
			{
				count+=(game->lvlitems[i]&liCOMPASS)?1:0;
			}
			
			return count;
		}
		
		case itype_bosskey:
		{
			int count=0;
			
			for(int i=0; i<MAXLEVELS; i++)
			{
				count+=(game->lvlitems[i]&liBOSSKEY)?1:0;
			}
			
			return count;
		}
		
		default:
			int maxid = current_item_id(item_type, checkmagic, jinx_check, check_bunny);
			
			if(maxid == -1)
				return 0;
				
			return itemsbuf[maxid].fam_type;
	}
}

std::map<int32_t, int32_t> itemcache;
std::map<int32_t, int32_t> itemcache_cost;

void removeFromItemCache(int32_t itemclass)
{
	itemcache.erase(itemclass);
	itemcache_cost.erase(itemclass);
	cache_tile_mod_clear();
}

void flushItemCache(bool justcost)
{
	itemcache_cost.clear();
	if(!justcost)
		itemcache.clear();
	else if(replay_version_check(0,19))
		return;

	cache_tile_mod_clear();
	
	//also fix the active subscreen if items were deleted -DD
	if(game != NULL)
	{
		verifyBothWeapons();
		refresh_subscr_items();
	}
}

// This is used often, so it should be as direct as possible.
int _c_item_id_internal(int itemtype, bool checkmagic, bool jinx_check, bool check_bunny)
{
	bool use_cost_cache = replay_version_check(19);
	if(jinx_check)
	{
		//special case for shields...
		if (itemtype == itype_shield && !Hero.status.jinx_shield)
			jinx_check = false;
		else if(!(Hero.status.jinx_melee|| Hero.status.jinx_item))
			jinx_check = false; //not jinxed
	}
	if(!Hero.status.bunny || itemtype == itype_pearl) // bunny_check does not apply
		check_bunny = false;
	if(itemtype == itype_ring) checkmagic = true;
	if (!jinx_check && !check_bunny
		&& (use_cost_cache || itemtype != itype_ring))
	{
		auto& cache = checkmagic && use_cost_cache ? itemcache_cost : itemcache;
		auto res = cache.find(itemtype);
		
		if(res != cache.end())
			return res->second;
	}
	
	int result = -1;
	int highestlevel = -1;
	
	for(int i=0; i<MAXITEMS; i++)
	{
		if(game->get_item(i) && itemsbuf[i].family==itemtype && !item_disabled(i))
		{
			if(checkmagic && itemtype != itype_magicring)
				if(!checkmagiccost(i))
					continue;
			if(jinx_check && (usesSwordJinx(i) ? Hero.status.jinx_melee : Hero.status.jinx_item))
				if(!(itemsbuf[i].flags & item_jinx_immune))
					continue;
			if(check_bunny && !checkbunny(i))
				continue;
			
			if(itemsbuf[i].fam_type >= highestlevel)
			{
				highestlevel = itemsbuf[i].fam_type;
				result=i;
			}
		}
	}
	
	if(!(jinx_check || check_bunny)) //Can't cache jinx_check/check_bunny
	{
		if (use_cost_cache)
		{
			if (!checkmagic)
				itemcache[itemtype] = result;
			if (checkmagic || result < 0 || checkmagiccost(result))
				itemcache_cost[itemtype] = result;
		}
		else
		{
			itemcache[itemtype] = result;
		}
	}
	return result;
}

// 'jinx_check' indicates that the highest level item *immune to jinxes* should be returned.
int current_item_id(int itype, bool checkmagic, bool jinx_check, bool check_bunny)
{
	if(itype < 0 || itype >= itype_max) return -1;
	if(game->OverrideItems[itype] > -2)
	{
		auto ovid = game->OverrideItems[itype];
		if(ovid < 0 || ovid >= MAXITEMS)
			return -1;
		if(itemsbuf[ovid].family == itype)
		{
			if(itype == itype_magicring)
				checkmagic = false;
			else if(itype == itype_ring)
				checkmagic = true;
			
			if(checkmagic && !checkmagiccost(ovid))
				return -1;

			if (jinx_check && !checkitem_jinx(ovid))
			{
				return -1;
			}
			return ovid;
		}
	}
	auto ret = _c_item_id_internal(itype,checkmagic,jinx_check,check_bunny);
	if(!jinx_check) //If not already a jinx-immune-only check...
	{
		//And the player IS jinxed...
		if(Hero.status.is_jinxed())
		{
			//Then do a jinx-immune-only check here
			auto ret2 = _c_item_id_internal(itype,checkmagic,true,check_bunny);
			//And *IF IT FINDS A VALID ITEM*, return that one instead! -Em
			//Should NOT need a compat rule, as this should always return -1 in old quests.
			if(ret2 > -1) return ret2;
		}
	}
	return ret;
}

int current_item_power(int itemtype, bool checkmagic, bool jinx_check, bool check_bunny)
{
	int result = current_item_id(itemtype, checkmagic, jinx_check, check_bunny);
	return (result<0) ? 0 : itemsbuf[result].power;
}

int32_t heart_container_id()
{
	for(int32_t i=0; i<MAXITEMS; i++)
	{
		if(itemsbuf[i].family == itype_heartcontainer)
		{
			return i;
		}
	}
	return -1;
}

struct tilemod_cache_state_t
{
	bool operator==(const tilemod_cache_state_t&) const = default;

	bool valid;
	bool bunny_clock;
	bool superman;
	int shield;
};
tilemod_cache_state_t tilemod_cache_state;
int32_t tilemod_cache_value;

void cache_tile_mod_clear()
{
	tilemod_cache_state = {false};
}

int32_t item_tile_mod()
{
	tilemod_cache_state_t state = {
		.valid = true,
		.bunny_clock = Hero.status.bunny,
		.superman = Hero.superman,
		.shield = Hero.active_shield_id,
	};
	if (tilemod_cache_state == state)
		return tilemod_cache_value;

	int32_t tile=0;
	bool check_bombcost = !get_qr(qr_BROKEN_BOMB_AMMO_COSTS);
	if(check_bombcost || game->get_bombs())
	{
		int32_t itemid = current_item_id(itype_bomb,check_bombcost);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(check_bombcost || game->get_sbombs())
	{
		int32_t itemid = current_item_id(itype_sbomb,check_bombcost);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_clock))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iClock
				: getHighestLevelEvenUnowned(itemsbuf, itype_clock);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_key))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iKey
				: getHighestLevelEvenUnowned(itemsbuf, itype_key);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_lkey))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iLevelKey
				: getHighestLevelEvenUnowned(itemsbuf, itype_lkey);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_map))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iMap
				: getHighestLevelEvenUnowned(itemsbuf, itype_map);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_compass))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iCompass
				: getHighestLevelEvenUnowned(itemsbuf, itype_compass);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_bosskey))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iBossKey
				: getHighestLevelEvenUnowned(itemsbuf, itype_bosskey);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_magiccontainer))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iMagicC
				: getHighestLevelEvenUnowned(itemsbuf, itype_magiccontainer);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	if(current_item(itype_triforcepiece))
	{
		int32_t itemid =
			get_qr(qr_HARDCODED_LITEM_LTMS)
				? iTriforce
				: getHighestLevelEvenUnowned(itemsbuf, itype_triforcepiece);
		if(itemid > -1 && checkbunny(itemid))
			tile+=itemsbuf[itemid].ltm;
	}
	
	for(int32_t i=0; i<itype_max; i++)
	{
		if(!get_qr(qr_HARDCODED_LITEM_LTMS))
		{
			switch(i)
			{
				case itype_bomb:
				case itype_sbomb:
				case itype_clock:
				case itype_key:
				case itype_lkey:
				case itype_map:
				case itype_compass:
				case itype_bosskey:
				case itype_magiccontainer:
				case itype_triforcepiece:
					continue; //already handled
			}
		}
		int32_t itemid = current_item_id(i,false);
		if(i == itype_shield)
			itemid = getCurrentShield(false);
		
		if(itemid < 0 || !checkbunny(itemid))
			continue;
		
		itemdata const& itm = itemsbuf[itemid];
		
		switch(itm.family)
		{
			case itype_shield:
				if(itm.flags & item_flag9) //active shield
				{
					if(!usingActiveShield(itemid))
					{
						tile+=itm.misc6; //'Inactive PTM'
						continue;
					}
				}
				break;
		}
		
		tile+=itm.ltm;
	}
	
	tilemod_cache_value = tile;
	tilemod_cache_state = state;
	return tile;
}

int32_t bunny_tile_mod()
{
	if(Hero.status.bunny)
	{
		return game->get_bunny_ltm();
	}
	return 0;
}

// Hints are drawn on a separate layer to combo reveals.
void draw_lens_under(BITMAP *dest, bool layer)
{
	//Lens flag 1: Replacement for qr_LENSHINTS; if set, lens will show hints. Does nothing if flag 2 is set.
	//Lens flag 2: Disable "hints", prevent rendering of Secret Combos
	//Lens flag 3: Don't show armos/chest/dive items
	//Lens flag 4: Show Raft Paths
	//Lens flag 5: Show Invisible Enemies
	bool hints = (itemsbuf[Hero.getLastLensID()].flags & item_flag2) ? false : (layer && (itemsbuf[Hero.getLastLensID()].flags & item_flag1));
	
	int32_t strike_hint_table[11]=
	{
		mfARROW, mfBOMB, mfBRANG, mfWANDMAGIC,
		mfSWORD, mfREFMAGIC, mfHOOKSHOT,
		mfREFFIREBALL, mfHAMMER, mfSWORDBEAM, mfWAND
	};
	
	{
		int32_t blink_rate=flash_reduction_enabled()?6:1;
		int32_t tempitem, tempweapon=0;
		strike_hint=strike_hint_table[strike_hint_counter];
		
		if(strike_hint_timer>32)
		{
			strike_hint_timer=0;
			strike_hint_counter=((strike_hint_counter+1)%11);
		}
		
		++strike_hint_timer;
		
		for(int32_t i=0; i<176; i++)
		{
			int32_t x = (i & 15) << 4;
			int32_t y = (i & 0xF0) + playing_field_offset;
			int32_t tempitemx=-16, tempitemy=-16;
			int32_t tempweaponx=-16, tempweapony=-16;
			
			for(int32_t iter=0; iter<2; ++iter)
			{
				int32_t checkflag=0;
				
				if(iter==0)
				{
					checkflag=combobuf[tmpscr->data[i]].flag;
				}
				else
				{
					checkflag=tmpscr->sflag[i];
				}
				
				if(checkflag==mfSTRIKE)
				{
					if(!hints)
					{
						if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSTRIKE],tmpscr->secretcset[sSTRIKE]);
					}
					else
					{
						checkflag = strike_hint;
					}
				}
				
				switch(checkflag)
				{
					case 0:
					case mfZELDA:
					case mfPUSHED:
					case mfENEMY0:
					case mfENEMY1:
					case mfENEMY2:
					case mfENEMY3:
					case mfENEMY4:
					case mfENEMY5:
					case mfENEMY6:
					case mfENEMY7:
					case mfENEMY8:
					case mfENEMY9:
					case mfSINGLE:
					case mfSINGLE16:
					case mfNOENEMY:
					case mfTRAP_H:
					case mfTRAP_V:
					case mfTRAP_4:
					case mfTRAP_LR:
					case mfTRAP_UD:
					case mfNOGROUNDENEMY:
					case mfNOBLOCKS:
					case mfSCRIPT1:
					case mfSCRIPT2:
					case mfSCRIPT3:
					case mfSCRIPT4:
					case mfSCRIPT5:
					case mfSCRIPT6:
					case mfSCRIPT7:
					case mfSCRIPT8:
					case mfSCRIPT9:
					case mfSCRIPT10:
					case mfSCRIPT11:
					case mfSCRIPT12:
					case mfSCRIPT13:
					case mfSCRIPT14:
					case mfSCRIPT15:
					case mfSCRIPT16:
					case mfSCRIPT17:
					case mfSCRIPT18:
					case mfSCRIPT19:
					case mfSCRIPT20:
					case mfPITHOLE:
					case mfPITFALLFLOOR:
					case mfLAVA:
					case mfICE:
					case mfICEDAMAGE:
					case mfDAMAGE1:
					case mfDAMAGE2:
					case mfDAMAGE4:
					case mfDAMAGE8:
					case mfDAMAGE16:
					case mfDAMAGE32:
					case mfFREEZEALL:
					case mfFREZEALLANSFFCS:
					case mfFREEZEFFCSOLY:
					case mfSCRITPTW1TRIG:
					case mfSCRITPTW2TRIG:
					case mfSCRITPTW3TRIG:
					case mfSCRITPTW4TRIG:
					case mfSCRITPTW5TRIG:
					case mfSCRITPTW6TRIG:
					case mfSCRITPTW7TRIG:
					case mfSCRITPTW8TRIG:
					case mfSCRITPTW9TRIG:
					case mfSCRITPTW10TRIG:
					case mfTROWEL:
					case mfTROWELNEXT:
					case mfTROWELSPECIALITEM:
					case mfSLASHPOT:
					case mfLIFTPOT:
					case mfLIFTORSLASH:
					case mfLIFTROCK:
					case mfLIFTROCKHEAVY:
					case mfDROPITEM:
					case mfSPECIALITEM:
					case mfDROPKEY:
					case mfDROPLKEY:
					case mfDROPCOMPASS:
					case mfDROPMAP:
					case mfDROPBOSSKEY:
					case mfSPAWNNPC:
					case mfSWITCHHOOK:
					case mfSIDEVIEWLADDER:
					case mfSIDEVIEWPLATFORM:
					case mfNOENEMYSPAWN:
					case mfENEMYALL:
					case mfNOMIRROR:
					case mfUNSAFEGROUND:
					case mf168:
					case mf169:
					case mf170:
					case mf171:
					case mf172:
					case mf173:
					case mf174:
					case mf175:
					case mf176:
					case mf177:
					case mf178:
					case mf179:
					case mf180:
					case mf181:
					case mf182:
					case mf183:
					case mf184:
					case mf185:
					case mf186:
					case mf187:
					case mf188:
					case mf189:
					case mf190:
					case mf191:
					case mf192:
					case mf193:
					case mf194:
					case mf195:
					case mf196:
					case mf197:
					case mf198:
					case mf199:
					case mf200:
					case mf201:
					case mf202:
					case mf203:
					case mf204:
					case mf205:
					case mf206:
					case mf207:
					case mf208:
					case mf209:
					case mf210:
					case mf211:
					case mf212:
					case mf213:
					case mf214:
					case mf215:
					case mf216:
					case mf217:
					case mf218:
					case mf219:
					case mf220:
					case mf221:
					case mf222:
					case mf223:
					case mf224:
					case mf225:
					case mf226:
					case mf227:
					case mf228:
					case mf229:
					case mf230:
					case mf231:
					case mf232:
					case mf233:
					case mf234:
					case mf235:
					case mf236:
					case mf237:
					case mf238:
					case mf239:
					case mf240:
					case mf241:
					case mf242:
					case mf243:
					case mf244:
					case mf245:
					case mf246:
					case mf247:
					case mf248:
					case mf249:
					case mf250:
					case mf251:
					case mf252:
					case mf253:
					case mf254:
					case mfEXTENDED:
						break;
						
					case mfPUSHUD:
					case mfPUSHLR:
					case mfPUSH4:
					case mfPUSHU:
					case mfPUSHD:
					case mfPUSHL:
					case mfPUSHR:
					case mfPUSHUDNS:
					case mfPUSHLRNS:
					case mfPUSH4NS:
					case mfPUSHUNS:
					case mfPUSHDNS:
					case mfPUSHLNS:
					case mfPUSHRNS:
					case mfPUSHUDINS:
					case mfPUSHLRINS:
					case mfPUSH4INS:
					case mfPUSHUINS:
					case mfPUSHDINS:
					case mfPUSHLINS:
					case mfPUSHRINS:
						if(!hints && ((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&16))
									  || ((get_debug() && zc_getkey(KEY_N)) && (frame&16))))
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->undercombo,tmpscr->undercset);
						}
						
						if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
								|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
						{
							if(hints)
							{
								switch(combobuf[tmpscr->data[i]].type)
								{
								case cPUSH_HEAVY:
								case cPUSH_HW:
									tempitem=getItemIDPower(itemsbuf,itype_bracelet,1);
									tempitemx=x, tempitemy=y;
									
									if(tempitem>-1)
										putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
										
									break;
									
								case cPUSH_HEAVY2:
								case cPUSH_HW2:
									tempitem=getItemIDPower(itemsbuf,itype_bracelet,2);
									tempitemx=x, tempitemy=y;
									
									if(tempitem>-1)
										putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
										
									break;
								}
							}
						}
						
						break;
						
					case mfWHISTLE:
						if(hints)
						{
							tempitem=getItemID(itemsbuf,itype_whistle,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
						//Why is this here?
					case mfFAIRY:
					case mfMAGICFAIRY:
					case mfALLFAIRY:
						if(hints)
						{
							tempitem=getItemID(itemsbuf, itype_fairy,1);//iFairyMoving;
							
							if(tempitem < 0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfANYFIRE:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sBCANDLE],tmpscr->secretcset[sBCANDLE]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_candle,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfSTRONGFIRE:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sRCANDLE],tmpscr->secretcset[sRCANDLE]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_candle,2);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfMAGICFIRE:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sWANDFIRE],tmpscr->secretcset[sWANDFIRE]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_wand,1);
							
							if(tempitem<0) break;
							
							tempweapon=wFire;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							else
							{
								tempweaponx=x;
								tempweapony=y;
							}
							
							putweapon(dest,tempweaponx,tempweapony,tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfDIVINEFIRE:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sDIVINEFIRE],tmpscr->secretcset[sDIVINEFIRE]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_divinefire,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfARROW:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sARROW],tmpscr->secretcset[sARROW]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_arrow,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfSARROW:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSARROW],tmpscr->secretcset[sSARROW]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_arrow,2);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfGARROW:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sGARROW],tmpscr->secretcset[sGARROW]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_arrow,3);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfBOMB:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sBOMB],tmpscr->secretcset[sBOMB]);
						}
						else
						{
							//tempitem=getItemID(itemsbuf,itype_bomb,1);
							tempweapon = wLitBomb;
							
							//if (tempitem<0) break;
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempweaponx=x;
								tempweapony=y;
							}
							
							putweapon(dest,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
						}
						
						break;
						
					case mfSBOMB:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSBOMB],tmpscr->secretcset[sSBOMB]);
						}
						else
						{
							//tempitem=getItemID(itemsbuf,itype_sbomb,1);
							//if (tempitem<0) break;
							tempweapon = wLitSBomb;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempweaponx=x;
								tempweapony=y;
							}
							
							putweapon(dest,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
						}
						
						break;
						
					case mfARMOS_SECRET:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSTAIRS],tmpscr->secretcset[sSTAIRS]);
						}	
						break;
						
					case mfBRANG:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sBRANG],tmpscr->secretcset[sBRANG]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_brang,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfMBRANG:
						if(!hints)
				{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sMBRANG],tmpscr->secretcset[sMBRANG]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_brang,2);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfFBRANG:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sFBRANG],tmpscr->secretcset[sFBRANG]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_brang,3);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfWANDMAGIC:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sWANDMAGIC],tmpscr->secretcset[sWANDMAGIC]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_wand,1);
							
							if(tempitem<0) break;
							
							tempweapon=itemsbuf[tempitem].wpn3;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							else
							{
								tempweaponx=x;
								tempweapony=y;
								--lens_hint_weapon[wMagic][4];
								
								if(lens_hint_weapon[wMagic][4]<-8)
								{
									lens_hint_weapon[wMagic][4]=8;
								}
							}
							
							putweapon(dest,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfREFMAGIC:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sREFMAGIC],tmpscr->secretcset[sREFMAGIC]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_shield,3);
							
							if(tempitem<0) break;
							
							tempweapon=ewMagic;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							else
							{
								tempweaponx=x;
								tempweapony=y;
								
								if(lens_hint_weapon[ewMagic][2]==up)
								{
									--lens_hint_weapon[ewMagic][4];
								}
								else
								{
									++lens_hint_weapon[ewMagic][4];
								}
								
								if(lens_hint_weapon[ewMagic][4]>8)
								{
									lens_hint_weapon[ewMagic][2]=up;
								}
								
								if(lens_hint_weapon[ewMagic][4]<=0)
								{
									lens_hint_weapon[ewMagic][2]=down;
								}
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
							putweapon(dest,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, lens_hint_weapon[ewMagic][2], lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
						}
						
						break;
						
					case mfREFFIREBALL:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sREFFIREBALL],tmpscr->secretcset[sREFFIREBALL]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_shield,3);
							
							if(tempitem<0) break;
							
							tempweapon=ewFireball;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
								tempweaponx=x;
								tempweapony=y;
								++lens_hint_weapon[ewFireball][3];
								
								if(lens_hint_weapon[ewFireball][3]>8)
								{
									lens_hint_weapon[ewFireball][3]=-8;
									lens_hint_weapon[ewFireball][4]=8;
								}
								
								if(lens_hint_weapon[ewFireball][3]>0)
								{
									++lens_hint_weapon[ewFireball][4];
								}
								else
								{
									--lens_hint_weapon[ewFireball][4];
								}
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
							putweapon(dest,tempweaponx+lens_hint_weapon[tempweapon][3],tempweapony+lens_hint_weapon[ewFireball][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1],-1);
						}
						
						break;
						
					case mfSWORD:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSWORD],tmpscr->secretcset[sSWORD]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfWSWORD:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sWSWORD],tmpscr->secretcset[sWSWORD]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,2);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfMSWORD:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sMSWORD],tmpscr->secretcset[sMSWORD]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,3);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfXSWORD:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sXSWORD],tmpscr->secretcset[sXSWORD]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,4);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfSWORDBEAM:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sSWORDBEAM],tmpscr->secretcset[sSWORDBEAM]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 1);
						}
						
						break;
						
					case mfWSWORDBEAM:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sWSWORDBEAM],tmpscr->secretcset[sWSWORDBEAM]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,2);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 2);
						}
						
						break;
						
					case mfMSWORDBEAM:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sMSWORDBEAM],tmpscr->secretcset[sMSWORDBEAM]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,3);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 3);
						}
						
						break;
						
					case mfXSWORDBEAM:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sXSWORDBEAM],tmpscr->secretcset[sXSWORDBEAM]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_sword,4);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 4);
						}
						
						break;
						
					case mfHOOKSHOT:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sHOOKSHOT],tmpscr->secretcset[sHOOKSHOT]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_hookshot,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfWAND:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sWAND],tmpscr->secretcset[sWAND]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_wand,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfHAMMER:
						if(!hints)
						{
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))putcombo(dest,x,y,tmpscr->secretcombo[sHAMMER],tmpscr->secretcset[sHAMMER]);
						}
						else
						{
							tempitem=getItemID(itemsbuf,itype_hammer,1);
							
							if(tempitem<0) break;
							
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate))
									|| ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								tempitemx=x;
								tempitemy=y;
							}
							
							putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
						}
						
						break;
						
					case mfARMOS_ITEM:
					case mfDIVE_ITEM:
						if((!getmapflag() || (tmpscr->flags9&fBELOWRETURN)) && !(itemsbuf[Hero.getLastLensID()].flags & item_flag3))
						{
							putitem2(dest,x,y,tmpscr->catchall, lens_hint_item[tmpscr->catchall][0], lens_hint_item[tmpscr->catchall][1], 0);
						}
						break;
						
					case 16:
					case 17:
					case 18:
					case 19:
					case 20:
					case 21:
					case 22:
					case 23:
					case 24:
					case 25:
					case 26:
					case 27:
					case 28:
					case 29:
					case 30:
					case 31:
						if(!hints)
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))
								putcombo(dest,x,y,tmpscr->secretcombo[checkflag-16+4],tmpscr->secretcset[checkflag-16+4]);
									 
						break;
					case mfSECRETSNEXT:
						if(!hints)
							if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))
								putcombo(dest,x,y,tmpscr->data[i]+1,tmpscr->cset[i]);
									 
						break;
					
					case mfSTRIKE:
						if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))
						{
							goto special;
						}
						else
						{
							break;
						}
						
					default: goto special;
					
					special:
						if(layer && ((checkflag!=mfRAFT && checkflag!=mfRAFT_BRANCH&& checkflag!=mfRAFT_BOUNCE) ||(itemsbuf[Hero.getLastLensID()].flags & item_flag4)))
						{
							if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&blink_rate)) || ((get_debug() && zc_getkey(KEY_N)) && (frame&blink_rate)))
							{
								rectfill(dest,x,y,x+15,y+15,WHITE);
							}
						}
						
						break;
				}
			}
		}
		
		if(layer)
		{
			if(tmpscr->door[0]==dWALK)
				rectfill(dest, 120, 16+playing_field_offset, 135, 31+playing_field_offset, WHITE);
				
			if(tmpscr->door[1]==dWALK)
				rectfill(dest, 120, 144+playing_field_offset, 135, 159+playing_field_offset, WHITE);
				
			if(tmpscr->door[2]==dWALK)
				rectfill(dest, 16, 80+playing_field_offset, 31, 95+playing_field_offset, WHITE);
				
			if(tmpscr->door[3]==dWALK)
				rectfill(dest, 224, 80+playing_field_offset, 239, 95+playing_field_offset, WHITE);
				
			if(tmpscr->door[0]==dBOMB)
			{
				showbombeddoor(dest, 0);
			}
			
			if(tmpscr->door[1]==dBOMB)
			{
				showbombeddoor(dest, 1);
			}
			
			if(tmpscr->door[2]==dBOMB)
			{
				showbombeddoor(dest, 2);
			}
			
			if(tmpscr->door[3]==dBOMB)
			{
				showbombeddoor(dest, 3);
			}
		}
		
		if(tmpscr->stairx + tmpscr->stairy)
		{
			if(!hints)
			{
				if(!(itemsbuf[Hero.getLastLensID()].flags & item_flag2))
					putcombo(dest,tmpscr->stairx,tmpscr->stairy+playing_field_offset,tmpscr->secretcombo[sSTAIRS],tmpscr->secretcset[sSTAIRS]);
			}
			else
			{
				if(tmpscr->flags&fWHISTLE)
				{
					tempitem=getItemID(itemsbuf,itype_whistle,1);
					int32_t tempitemx=-16;
					int32_t tempitemy=-16;
					
					if((!(get_debug() && zc_getkey(KEY_N)) && (lensclk&(blink_rate/4)))
							|| ((get_debug() && zc_getkey(KEY_N)) && (frame&(blink_rate/4))))
					{
						tempitemx=tmpscr->stairx;
						tempitemy=tmpscr->stairy+playing_field_offset;
					}
					
					putitem2(dest,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
				}
			}
		}
	}
}

BITMAP *lens_scr_d; // The "d" is for "destructible"!

void draw_lens_over()
{
	// Oh, what the heck.
	static BITMAP *lens_scr = NULL;
	static int32_t last_width = -1;
	int32_t width = itemsbuf[current_item_id(itype_lens,true)].misc1;
	
	// Only redraw the circle if the size has changed
	if(width != last_width)
	{
		if(lens_scr == NULL)
		{
			lens_scr = create_bitmap_ex(8,2*288,2*(240-playing_field_offset));
		}
		
		clear_to_color(lens_scr, BLACK);
		circlefill(lens_scr, 288, 240-playing_field_offset, width, 0);
		circle(lens_scr, 288, 240-playing_field_offset, width+2, 0);
		circle(lens_scr, 288, 240-playing_field_offset, width+5, 0);
		last_width=width;
	}
	
	masked_blit(lens_scr, framebuf, 288-(HeroX()+8), 240-playing_field_offset-(HeroY()+8), 0, playing_field_offset, 256, 168);
	do_primitives(framebuf, SPLAYER_LENS_OVER, tmpscr, 0, playing_field_offset);
}

//----------------------------------------------------------------

void draw_wavy(BITMAP *source, BITMAP *target, int32_t amplitude, bool interpol)
{
	//recreating a big bitmap every frame is highly sluggish.
	static BITMAP *wavebuf = create_bitmap_ex(8,288,240-original_playing_field_offset);
	clear_to_color(wavebuf, BLACK);
	blit(source,wavebuf,0,original_playing_field_offset,16,0,256,224-original_playing_field_offset);
	
	int32_t ofs;
	amplitude = zc_min(2048,amplitude); // some arbitrary limit to prevent crashing
	if(flash_reduction_enabled() && !get_qr(qr_WAVY_NO_EPILEPSY)) amplitude = zc_min(16,amplitude);
	int32_t amp2=168;
	if(flash_reduction_enabled() && !get_qr(qr_WAVY_NO_EPILEPSY_2)) amp2*=2;
	int32_t i=frame%amp2;
	
	for(int32_t j=0; j<168; j++)
	{
		if(j&1 && interpol)
		{
			// Add 288*2048 to ensure it's never negative. It'll get modded out.
			ofs=288*2048+int32_t(zc::math::Sin((double(i+j)*2*PI/amp2))*amplitude);
		}
		else
		{
			ofs=288*2048-int32_t(zc::math::Sin((double(i+j)*2*PI/amp2))*amplitude);
		}
		
		if(ofs)
		{
			for(int32_t k=0; k<256; k++)
			{
				target->line[j+original_playing_field_offset][k]=wavebuf->line[j][(k+ofs+16)%288];
			}
		}
	}
}

void draw_fuzzy(int32_t fuzz)
// draws from right half of scrollbuf to framebuf
{
	int32_t firstx, firsty, xstep, ystep, i, y, dx, dy;
	byte *start, *si, *di;
	
	if(fuzz<1)
		fuzz = 1;
		
	xstep = 128%fuzz;
	
	if(xstep > 0)
		xstep = fuzz-xstep;
		
	ystep = 112%fuzz;
	
	if(ystep > 0)
		ystep = fuzz-ystep;
		
	firsty = 1;
	
	for(y=0; y<224;)
	{
		start = &(scrollbuf->line[y][256]);
		
		for(dy=0; dy<ystep && dy+y<224; dy++)
		{
			si = start;
			di = &(framebuf->line[y+dy][0]);
			i = xstep;
			firstx = 1;
			
			for(dx=0; dx<256; dx++)
			{
				*(di++) = *si;
				
				if(++i >= fuzz)
				{
					if(!firstx)
						si += fuzz;
					else
					{
						si += fuzz-xstep;
						firstx = 0;
					}
					
					i = 0;
				}
			}
		}
		
		if(!firsty)
			y += fuzz;
		else
		{
			y += ystep;
			ystep = fuzz;
			firsty = 0;
		}
	}
}

void updatescr(bool allowwavy)
{
	static BITMAP *wavybuf = create_bitmap_ex(8,256,224);
	static BITMAP *panorama = create_bitmap_ex(8,256,224);
		
	if(toogam)
	{
		textout_ex(framebuf,font,"no walls",8,216,1,-1);
	}
	
	if(Showpal)
		dump_pal(framebuf);
		
	if(!Playing)
		black_opening_count=0;
		
	if(black_opening_count<0) //shape is opening up
	{
		black_opening(framebuf,black_opening_x,black_opening_y,(66+black_opening_count),66);
		
		if(Advance||(!Paused))
		{
			++black_opening_count;
		}
	}
	else if(black_opening_count>0) //shape is closing
	{
		black_opening(framebuf,black_opening_x,black_opening_y,black_opening_count,66);
		
		if(Advance||(!Paused))
		{
			--black_opening_count;
		}
	}

	if(black_opening_count==0&&black_opening_shape==bosFADEBLACK)
	{
		black_opening_shape = bosCIRCLE;
		memcpy(RAMpal, tempblackpal, PAL_SIZE*sizeof(RGB));
		refreshTints();
		refreshpal=true;
	}
	
	if(refreshpal)
	{
		refreshpal=false;
		RAMpal[253] = _RGB(0,0,0);
		RAMpal[254] = _RGB(255,255,255);
		hw_palette = &RAMpal;
		update_hw_pal = true;

		// Creating rgb_table and trans_table is pretty expensive, so try not to redo the same work
		// within a short period of time by using a cache.
		typedef std::array<uint32_t, PAL_SIZE> pal_table_cache_key;
		struct pal_table_cache_entry {
			RGB_MAP rgb_table;
			COLOR_MAP trans_table;
		};
		static std::map<pal_table_cache_key, pal_table_cache_entry> pal_table_cache;

		static constexpr int pal_table_cache_max_memory_mb = 10;
		static constexpr int pal_table_cache_max_size = pal_table_cache_max_memory_mb / ((double)sizeof(pal_table_cache_entry) / 1024 / 1024);
		if (pal_table_cache.size() > pal_table_cache_max_size)
			pal_table_cache.clear();

		pal_table_cache_key key;
		for (int i = 0; i < PAL_SIZE; i++)
			key[i] = RAMpal[i].r + (RAMpal[i].g << 8) + (RAMpal[i].b << 16);
		auto cache_it = pal_table_cache.find(key);
		if (cache_it == pal_table_cache.end())
		{
			create_rgb_table(&rgb_table, RAMpal, NULL);
			create_zc_trans_table(&trans_table, RAMpal, 128, 128, 128);
			pal_table_cache[key] = {rgb_table, trans_table};
			trans_table2 = trans_table;
		}
		else
		{
			rgb_table = cache_it->second.rgb_table;
			trans_table = cache_it->second.trans_table;
			trans_table2 = cache_it->second.trans_table;
		}
		
		for(int32_t q=0; q<PAL_SIZE; q++)
		{
			trans_table2.data[0][q] = q;
			trans_table2.data[q][q] = q;
		}
	}
	
	bool clearwavy = (wavy <= 0);
	
	if(wavy <= 0)
	{
		// So far one thing can alter wavy apart from scripts: Wavy DMaps.
		wavy = (DMaps[currdmap].flags&dmfWAVY ? 4 : 0);
	}
	
	blit(framebuf, wavybuf, 0, 0, 0, 0, 256, 224);
	
	if(wavy && Playing && allowwavy)
	{
		draw_wavy(framebuf, wavybuf, wavy,false);
	}
	
	if(clearwavy)
		wavy = 0; // Wavy was set by a DMap flag. Clear it.
	else if(Playing && !Paused)
		wavy--; // Wavy was set by a script. Decrement it.
		
	if(Playing && msgpos && !screenscrolling)
	{
		if(!(msg_bg_display_buf->clip))
			blit_msgstr_bg(framebuf,0,0,0,playing_field_offset,256,168);
		if(!(msg_portrait_display_buf->clip))
			blit_msgstr_prt(framebuf,0,0,0,playing_field_offset,256,168);
		if(!(msg_txt_display_buf->clip))
			blit_msgstr_fg(framebuf,0,0,0,playing_field_offset,256,168);
	}
	
	bool nosubscr = (tmpscr->flags3&fNOSUBSCR && !(tmpscr->flags3&fNOSUBSCROFFSET));
	
	if(nosubscr)
	{
		rectfill(panorama,0,0,255,passive_subscreen_height/2,0);
		rectfill(panorama,0,168+passive_subscreen_height/2,255,168+passive_subscreen_height-1,0);
		blit(wavybuf,panorama,0,playing_field_offset,0,passive_subscreen_height/2,256,224-passive_subscreen_height);
	}
	
	//TODO: Optimize blit 'overcalls' -Gleeok
	BITMAP *source = nosubscr ? panorama : wavybuf;
	blit(source,framebuf,0,0,0,0,256,224);
	
	update_hw_screen();
}

//----------------------------------------------------------------

static PALETTE syspal;
int32_t onGUISnapshot()
{
	char buf[200];
	int32_t num=0;
	do
	{
		sprintf(buf, "%szc_screen%05d.%s", get_snap_str(), ++num, snapshotformat_str[SnapshotFormat][1]);
	}
	while(num<99999 && exists(buf));
	
	if (!al_save_bitmap(buf, al_get_backbuffer(all_get_display())))
		InfoDialog("Error", "Failed to save snapshot").show();
	
	return D_O_K;
}

int32_t onNonGUISnapshot()
{
	PALETTE temppal;
	get_palette(temppal);
	bool realpal=(zc_getkey(KEY_ZC_LCONTROL, true) || zc_getkey(KEY_ZC_RCONTROL, true));
	
	char buf[200];
	int32_t num=0;
	
	do
	{
		sprintf(buf, "%szc_screen%05d.%s", get_snap_str(), ++num, snapshotformat_str[SnapshotFormat][1]);
	}
	while(num<99999 && exists(buf));

	if ((tmpscr->flags3&fNOSUBSCR && !(tmpscr->flags3&fNOSUBSCROFFSET)) && !(key[KEY_ALT]))
	{
		BITMAP *b = create_bitmap_ex(8,256,168);
		clear_to_color(b,0);
		blit(framebuf,b,0,passive_subscreen_height/2,0,0,256,168);
		alleg4_save_bitmap(b, SnapshotScale, buf, realpal ? temppal : RAMpal);
		destroy_bitmap(b);
	}
	else
	{
		alleg4_save_bitmap(framebuf, SnapshotScale, buf, realpal?temppal:RAMpal);
	}
	
	return D_O_K;
}

int32_t onSnapshot()
{
	if(zc_getkey(KEY_LSHIFT, true)||zc_getkey(KEY_RSHIFT, true))
	{
		onGUISnapshot();
	}
	else
	{
		onNonGUISnapshot();
	}
	
	return D_O_K;
}

int32_t onSaveMapPic()
{
	int32_t mapres2 = 0;
	char buf[200];
	int32_t num=0;
	mapscr tmpscr_b[2];
	mapscr tmpscr_c[6];
	BITMAP* _screen_draw_buffer = NULL;
	_screen_draw_buffer = create_bitmap_ex(8,256,224);
	set_clip_state(_screen_draw_buffer,1);
	
	for(int32_t i=0; i<6; ++i)
	{
		tmpscr_c[i] = tmpscr2[i];
		tmpscr2[i].zero_memory();
		
		if(i>=2)
		{
			continue;
		}
		
		tmpscr_b[i] = tmpscr[i];
		tmpscr[i].zero_memory();
	}
	
	do
	{
		sprintf(buf, "%szc_screen%05d.png", get_snap_str(), ++num);
	}
	while(num<99999 && exists(buf));
	
	BITMAP* mappic = NULL;
	
	
	bool done=false, redraw=true;
	
	mappic = create_bitmap_ex(8,(256*16)>>mapres,(176*8)>>mapres);
	
	if(!mappic)
	{
		enter_sys_pal();
		jwin_alert("View Map","Not enough memory.",NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
		exit_sys_pal();
		return D_O_K;;
	}
	
	// draw the map
	set_clip_rect(_screen_draw_buffer, 0, 0, _screen_draw_buffer->w, _screen_draw_buffer->h);
	
	for(int32_t y=0; y<8; y++)
	{
		for(int32_t x=0; x<16; x++)
		{
			if(!displayOnMap(x, y))
			{
				rectfill(_screen_draw_buffer, 0, 0, 255, 223, WHITE);
			}
			else
			{
				int32_t s = (y<<4) + x;
				loadscr2(1,s,-1);
				
				for(int32_t i=0; i<6; i++)
				{
					if(tmpscr[1].layermap[i]<=0)
						continue;
					
					tmpscr2[i]=TheMaps[(tmpscr[1].layermap[i]-1)*MAPSCRS+tmpscr[1].layerscreen[i]];
				}
				
				if(XOR((tmpscr+1)->flags7&fLAYER2BG, DMaps[currdmap].flags&dmfLAYER2BG)) do_layer(_screen_draw_buffer, 0, 2, tmpscr+1, -256, playing_field_offset, 2);
				
				if(XOR((tmpscr+1)->flags7&fLAYER3BG, DMaps[currdmap].flags&dmfLAYER3BG)) do_layer(_screen_draw_buffer, 0, 3, tmpscr+1, -256, playing_field_offset, 2);
				
				if(lenscheck(tmpscr+1,0)) putscr(_screen_draw_buffer,256,0,tmpscr+1);
				do_layer(_screen_draw_buffer, 0, 1, tmpscr+1, -256, playing_field_offset, 2);
				
				if(!XOR((tmpscr+1)->flags7&fLAYER2BG, DMaps[currdmap].flags&dmfLAYER2BG)) do_layer(_screen_draw_buffer, 0, 2, tmpscr+1, -256, playing_field_offset, 2);
				
				putscrdoors(_screen_draw_buffer,256,0,tmpscr+1);
				if(get_qr(qr_PUSHBLOCK_SPRITE_LAYER))
				{
					do_layer(_screen_draw_buffer, -2, 0, tmpscr+1, -256, playing_field_offset, 2);
					if(get_qr(qr_PUSHBLOCK_LAYER_1_2))
					{
						do_layer(_screen_draw_buffer, -2, 1, tmpscr+1, -256, playing_field_offset, 2);
						do_layer(_screen_draw_buffer, -2, 2, tmpscr+1, -256, playing_field_offset, 2);
					}
				}
				do_layer(_screen_draw_buffer, -3, 0, tmpscr+1, -256, playing_field_offset, 2); // Freeform combos!
				
				if(!XOR((tmpscr+1)->flags7&fLAYER3BG, DMaps[currdmap].flags&dmfLAYER3BG)) do_layer(_screen_draw_buffer, 0, 3, tmpscr+1, -256, playing_field_offset, 2);
				
				do_layer(_screen_draw_buffer, 0, 4, tmpscr+1, -256, playing_field_offset, 2);
				do_layer(_screen_draw_buffer, -1, 0, tmpscr+1, -256, playing_field_offset, 2);
				if(get_qr(qr_OVERHEAD_COMBOS_L1_L2))
				{
					do_layer(_screen_draw_buffer, -1, 1, tmpscr+1, -256, playing_field_offset, 2);
					do_layer(_screen_draw_buffer, -1, 2, tmpscr+1, -256, playing_field_offset, 2);
				}
				do_layer(_screen_draw_buffer, 0, 5, tmpscr+1, -256, playing_field_offset, 2);
				do_layer(_screen_draw_buffer, 0, 6, tmpscr+1, -256, playing_field_offset, 2);
				
			}
			
			stretch_blit(_screen_draw_buffer, mappic, 256, 0, 256, 176, x<<(8-mapres), (y*176)>>mapres, 256>>mapres, 176>>mapres);
		}
	}
	
	for(int32_t i=0; i<6; ++i)
	{
		tmpscr2[i]=tmpscr_c[i];
		
		if(i>=2)
		{
			continue;
		}
		
		tmpscr[i]=tmpscr_b[i];
	}
	
	save_bitmap(buf,mappic,RAMpal);
	destroy_bitmap(mappic);
	destroy_bitmap(_screen_draw_buffer);
	return D_O_K;
}

void f_Quit(int32_t type)
{
	if(type==qQUIT && !Playing)
		return;
	
	bool from_menu = is_sys_pal;
	
	if(!from_menu)
	{
		music_pause();
		pause_all_sfx();
		sys_mouse();
	}
	enter_sys_pal();
	clear_keybuf();
	
	if (replay_version_check(0, 10))
		replay_poll();
	if (replay_is_replaying())
		replay_peek_quit();

	if (!replay_is_replaying())
		switch(type)
		{
		case qQUIT:
			onQuit();
			break;
			
		case qRESET:
			onReset();
			break;
			
		case qEXIT:
			onExit();
			break;
		}
	
	if(Quit)
	{
		kill_sfx();
		music_stop();
		exit_sys_pal();
		update_hw_screen();
	}
	else
	{
		exit_sys_pal();
		if(!from_menu)
		{
			music_resume();
			resume_all_sfx();
		}
	}
	
	if(!from_menu)
		game_mouse();
	eat_buttons();
	
	zc_readrawkey(KEY_ESC);
		
	zc_readrawkey(KEY_ENTER);
}

//----------------------------------------------------------------

int32_t onNoWalls()
{
	cheats_enqueue(Cheat::Walls);
	return D_O_K;
}

int32_t onIgnoreSideview()
{
	cheats_enqueue(Cheat::IgnoreSideView);
	return D_O_K;
}

int32_t input_idle(bool checkmouse)
{
	static int32_t mx, my, mz, mb;
	
	if(keypressed() || zc_key_pressed() ||
	   (checkmouse && (mx != mouse_x || my != mouse_y || mz != mouse_z || mb != mouse_b)))
	{
		idle_count = 0;
		
		if(active_count < MAX_ACTIVE)
		{
			++active_count;
		}
	}
	else if(idle_count < MAX_IDLE)
	{
		++idle_count;
		active_count = 0;
	}
	
	mx = mouse_x;
	my = mouse_y;
	mz = mouse_z;
	mb = mouse_b;
	
	return idle_count;
}

int32_t onGoFast()
{
	cheats_enqueue(Cheat::Fast);
	return D_O_K;
}

int32_t onKillCheat()
{
	cheats_enqueue(Cheat::Kill);
	return D_O_K;
}

int32_t onSecretsCheat()
{
	cheats_enqueue(Cheat::TrigSecrets);
	return D_O_K;
}
int32_t onSecretsCheatPerm()
{
	cheats_enqueue(Cheat::TrigSecretsPerm);
	return D_O_K;
}

int32_t onShowLayer0()
{
	show_layer_0 = !show_layer_0;
	return D_O_K;
}
int32_t onShowLayer1()
{
	show_layer_1 = !show_layer_1;
	return D_O_K;
}
int32_t onShowLayer2()
{
	show_layer_2 = !show_layer_2;
	return D_O_K;
}
int32_t onShowLayer3()
{
	show_layer_3 = !show_layer_3;
	return D_O_K;
}
int32_t onShowLayer4()
{
	show_layer_4 = !show_layer_4;
	return D_O_K;
}
int32_t onShowLayer5()
{
	show_layer_5 = !show_layer_5;
	return D_O_K;
}
int32_t onShowLayer6()
{
	show_layer_6 = !show_layer_6;
	return D_O_K;
}
int32_t onShowLayerO()
{
	show_layer_over=!show_layer_over;
	return D_O_K;
}
int32_t onShowLayerP()
{
	show_layer_push=!show_layer_push;
	return D_O_K;
}
int32_t onShowLayerS()
{
	show_sprites=!show_sprites;
	return D_O_K;
}
int32_t onShowLayerF()
{
	show_ffcs=!show_ffcs;
	return D_O_K;
}
int32_t onShowLayerW()
{
	show_walkflags=!show_walkflags;
	if(show_walkflags)
		show_effectflags = false;
	return D_O_K;
}
int32_t onShowLayerE()
{
	show_effectflags=!show_effectflags;
	if(show_effectflags)
		show_walkflags = false;
	return D_O_K;
}
int32_t onShowFFScripts()
{
	show_ff_scripts=!show_ff_scripts;
	return D_O_K;
}
int32_t onShowHitboxes()
{
	show_hitboxes=!show_hitboxes;
	return D_O_K;
}
int32_t onShowInfoOpacity()
{
	info_opacity = vbound(getnumber("Debug Info Opacity",info_opacity),0,255);
	zc_set_config("zc","debug_info_opacity",info_opacity);
	return D_O_K;
}

int32_t onLightSwitch()
{
	cheats_enqueue(Cheat::Light);
	return D_O_K;
}

int32_t onGoTo();
int32_t onGoToComplete();

bool handle_close_btn_quit()
{
	if(close_button_quit)
	{
		close_button_quit=false;
		f_Quit(qEXIT);
	}
	return (exiting_program = Quit==qEXIT);
}

void syskeys()
{
	update_system_keys();
	
	int32_t oldtitle_version;
	
	poll_joystick();
	
	handle_close_btn_quit();
	if(Quit == qEXIT) return;
	
	if(rMbtn() || (gui_mouse_b() && !mouse_down && ClickToFreeze &&!disableClickToFreeze))
	{
		System();
	}
	
	mouse_down=gui_mouse_b();
	
	if(zc_read_system_key(KEY_F1))
	{
		if(zc_get_system_key(KEY_ZC_LCONTROL) || zc_get_system_key(KEY_ZC_RCONTROL))
		{
			halt=!halt;
			//zinit.subscreen=(zinit.subscreen+1)%ssdtMAX;
		}
		else
		{
			Throttlefps=!Throttlefps;
			zc_set_config(cfg_sect,"throttlefps", (int32_t)Throttlefps);
		}
	}
	
	if(zc_read_system_key(KEY_F2))
	{
		ShowFPS=!ShowFPS;
		zc_set_config(cfg_sect,"showfps",(int32_t)ShowFPS);
	}
	
	if(zc_read_system_key(KEY_F3) && Playing)	Paused=!Paused;
	
	if(zc_read_system_key(KEY_F4) && Playing)
	{
		Paused=true;
		Advance=true;
	}
	
	if(zc_read_system_key(KEY_F6)) onTryQuit();
	
#ifndef ALLEGRO_MACOSX
	if(zc_read_system_key(KEY_F9))	f_Quit(qRESET);
	
	if(zc_read_system_key(KEY_F10))   f_Quit(qEXIT);
#else
	if(zc_read_system_key(KEY_F7))	f_Quit(qRESET);
	
	if(zc_read_system_key(KEY_F8))   f_Quit(qEXIT);
#endif
	if(zc_read_system_key(KEY_F5)&&(Playing && currscr<128 && DMaps[currdmap].flags&dmfVIEWMAP))	onSaveMapPic();
	
	if (zc_read_system_key(KEY_F12))
	{
		onSnapshot();
	}
	
	if(debug_enabled && zc_read_system_key(KEY_TAB))
		set_debug(!get_debug());
	
	if(CheatModifierKeys())
	{
		for(Cheat c = (Cheat)1; c < Cheat::Last; c = (Cheat)(c+1))
		{
			if(!bindable_cheat(c))
				continue;
			if(get_debug() || cheat >= cheat_lvl(c))
			{
				if(checkcheat(c))
					cheats_hit_bind(c);
			}
		}
	}
	
	if(volkeys)
	{
		if(zc_read_system_key(KEY_PGUP)) master_volume(-1,midi_volume+8);
		
		if(zc_read_system_key(KEY_PGDN)) master_volume(-1,midi_volume==255?248:midi_volume-8);
		
		if(zc_read_system_key(KEY_HOME)) master_volume(digi_volume+8,-1);
		
		if(zc_read_system_key(KEY_END))  master_volume(digi_volume==255?248:digi_volume-8,-1);
	}
	
	if(!get_debug() || !SystemKeys || replay_is_replaying())
		goto bottom;
	
	if(zc_readkey(KEY_P))   Paused=!Paused;
	
	if(zc_readkey(KEY_A))
	{
		Paused=true;
		Advance=true;
	}
	
	if(zc_readkey(KEY_G))   db=(db==999)?0:999;
#ifndef ALLEGRO_MACOSX
	if(zc_readkey(KEY_F8))  Showpal=!Showpal;
	
	if(zc_readkey(KEY_F7))
	{
		Matrix(ss_speed, ss_density, 0);
		game_pal();
	}
#else
	// The reason these are different on Mac in the first place is that
	// the OS doesn't let us use F9 and F10...
	if(zc_readkey(KEY_F10))  Showpal=!Showpal;
	
	if(zc_readkey(KEY_F9))
	{
		Matrix(ss_speed, ss_density, 0);
		game_pal();
	}
#endif
	if(zc_readkey(KEY_PLUS_PAD) || zc_readkey(KEY_EQUALS))
	{
		//change containers
		if(zc_getkey(KEY_ZC_LCONTROL) || zc_getkey(KEY_ZC_RCONTROL))
		{
			//magic containers
			if(zc_getkey(KEY_LSHIFT) || zc_getkey(KEY_RSHIFT))
			{
				game->set_maxmagic(zc_min(game->get_maxmagic()+game->get_mp_per_block(),game->get_mp_per_block()*8));
			}
			else
			{
				game->set_maxlife(zc_min(game->get_maxlife()+game->get_hp_per_heart(),game->get_hp_per_heart()*24));
			}
		}
		else
		{
			if(zc_getkey(KEY_LSHIFT) || zc_getkey(KEY_RSHIFT))
			{
				game->set_magic(zc_min(game->get_magic()+1,game->get_maxmagic()));
			}
			else
			{
				game->set_life(zc_min(game->get_life()+1,game->get_maxlife()));
			}
		}
	}
	
	if(zc_readkey(KEY_MINUS_PAD) || zc_readkey(KEY_MINUS))
	{
		//change containers
		if(zc_getkey(KEY_ZC_LCONTROL) || zc_getkey(KEY_ZC_RCONTROL))
		{
			//magic containers
			if(zc_getkey(KEY_LSHIFT) || zc_getkey(KEY_RSHIFT))
			{
				game->set_maxmagic(zc_max(game->get_maxmagic()-game->get_mp_per_block(),0));
				game->set_magic(zc_min(game->get_maxmagic(), game->get_magic()));
				//heart containers
			}
			else
			{
				game->set_maxlife(zc_max(game->get_maxlife()-game->get_hp_per_heart(),game->get_hp_per_heart()));
				game->set_life(zc_min(game->get_maxlife(), game->get_life()));
			}
		}
		else
		{
			if(zc_getkey(KEY_LSHIFT) || zc_getkey(KEY_RSHIFT))
			{
				game->set_magic(zc_max(game->get_magic()-1,0));
			}
			else
			{
				game->set_life(zc_max(game->get_life()-1,0));
			}
		}
	}
	
	if(zc_readkey(KEY_COMMA))  jukebox(currmidi-1);
	
	if(zc_readkey(KEY_STOP))   jukebox(currmidi+1);
	
	verifyBothWeapons();
	
bottom:

	if(input_idle(true) > after_time())
	{
		Matrix(ss_speed, ss_density, 0);
		game_pal();
	}
}

void checkQuitKeys()
{
#ifndef ALLEGRO_MACOSX
	if(key[KEY_F9])	f_Quit(qRESET);
	
	if(key[KEY_F10])   f_Quit(qEXIT);
#else
	if(key[KEY_F7])	f_Quit(qRESET);
	
	if(key[KEY_F8])   f_Quit(qEXIT);
#endif
}

bool CheatModifierKeys()
{
	// Cheats are replayed via the X cheat step, no need to check for keyboard input
	// to trigger cheats.
	if (replay_is_replaying())
		return false;

	if ( ( cheat_modifier_keys[0] > 0 && key[cheat_modifier_keys[0]] ) ||
		( cheat_modifier_keys[1] > 0 && key[cheat_modifier_keys[1]] ) ||
		(cheat_modifier_keys[0] <= 0 && cheat_modifier_keys[1] <= 0))
	{
		if ( ( cheat_modifier_keys[2] <= 0 || key[cheat_modifier_keys[2]] ) ||
			( cheat_modifier_keys[3] > 0 && key[cheat_modifier_keys[3]] )  ||
			(cheat_modifier_keys[2] <= 0 && cheat_modifier_keys[3] <= 0))
		{
			return true;
		}
	}
	return false;
}

//99:05:54, for some reason?
#define OLDMAXTIME  21405240
//9000:00:00, the highest even-thousand hour fitting within 32b signed. This is 375 *DAYS*.
#define MAXTIME	 1944000000

void advanceframe(bool allowwavy, bool sfxcleanup, bool allowF6Script)
{
	if(zcmusic!=NULL)
	{
		zcmusic_poll();
	}
	zcmixer_update(zcmixer, emusic_volume, FFCore.usr_music_volume, get_qr(qr_OLD_SCRIPT_VOLUME));
	
	updatescr(allowwavy);

	Advance=false;
	while(Paused && !Advance && !Quit)
	{
		// have to call this, otherwise we'll get an infinite loop
		syskeys();
		if(allowF6Script)
		{
			FFCore.runF6Engine();
		}
		
#ifdef _WIN32
		
		if(use_dwm_flush)
		{
			do_DwmFlush();
		}
		
#endif
		
		// to keep music playing
		if(zcmusic!=NULL)
		{
			zcmusic_poll();
		}

		update_hw_screen();
	}
	
	if(Quit)
		return;
	
	if(Playing && game->get_time()<unsigned(get_qr(qr_GREATER_MAX_TIME) ? MAXTIME : OLDMAXTIME))
		game->change_time(1);
	
	// Many mistakes have been make re: inputs, and we are stuck with many replays relying on those mistakes.

	bool should_reset_down_state = !get_qr(qr_BROKEN_INPUT_DOWN_STATE);
	if (replay_version_check(0, 16))
		should_reset_down_state = replay_version_check(11, 16);
	if (should_reset_down_state)
	{
		for (int i = 0; i < ZC_CONTROL_STATES; i++)
			down_control_states[i] = raw_control_state[i];
	}
	
	if (replay_is_active())
	{
		if (replay_version_check(3))
			replay_poll();
		
		if (replay_version_check(11) || replay_version_check(6, 8))
			replay_peek_input();
	}
	
	load_control_called_this_frame = false;
	
	poll_keyboard();
	update_keys();
	
	++frame;
	
	if (replay_is_replaying())
		replay_do_cheats();
	syskeys();
	
	// The mouse variables can change from the mouse thread at anytime during a frame,
	// so save the result at the start so that replaying is consistent.
	script_mouse_x = gui_mouse_x();
	script_mouse_y = gui_mouse_y();
	script_mouse_z = mouse_z;
	script_mouse_b = mouse_b;
	
	// Cheats used via the System menu (called by syskeys) will call cheats_enqueue. syskeys
	// is called just above, and in the paused loop above, so the queue-and-defer-slightly
	// approach here means it doesn't matter which call adds the cheat.
	cheats_execute_queued();
	
	if (replay_is_replaying())
		replay_peek_quit();
	if (GameFlags & GAMEFLAG_TRYQUIT)
		replay_step_quit(0);
	if(allowF6Script)
		FFCore.runF6Engine();
	if (Quit)
		replay_step_quit(Quit);
	
#ifdef _WIN32
	
	if(use_dwm_flush)
	{
		do_DwmFlush();
	}
	
#endif
	
	if(sfxcleanup)
		sfx_cleanup();
	
	jit_poll();

#ifdef __EMSCRIPTEN__
	// Yield the main thread back to the browser occasionally.
	if (is_headless())
	{
		static int rate = 10000;
		static int force_yield = rate;
		if (force_yield++ >= rate)
		{
			force_yield = 0;
			emscripten_sleep(0);
		}
	}
#endif

	if (zqtesting_mode && test_mode_auto_restart)
	{
		static auto last_write_time = fs::last_write_time(qstpath);
		static auto last_check = std::chrono::system_clock::now();

		if (std::chrono::system_clock::now() - last_check > 200ms)
		{
			last_check = std::chrono::system_clock::now();
			auto write_time = fs::last_write_time(qstpath);
			if (last_write_time != write_time)
			{
				last_write_time = write_time;
				disableClickToFreeze = false;
				Quit = qRESET;
				replay_quit();
			}
		}
	}
}

void zapout()
{
	set_clip_rect(scrollbuf, 0, 0, scrollbuf->w, scrollbuf->h);
	blit(framebuf,scrollbuf,0,0,256,0,256,224);
	
	FFCore.runGenericPassiveEngine(SCR_TIMING_END_FRAME);
	script_drawing_commands.Clear();
	
	// zap out
	for(int32_t i=1; i<=24; i++)
	{
		draw_fuzzy(i);
		advanceframe(true);
		
		if(Quit)
		{
			break;
		}
	}
}

void zapin()
{
	FFCore.warpScriptCheck();
	draw_screen(tmpscr);
	set_clip_rect(scrollbuf, 0, 0, scrollbuf->w, scrollbuf->h);
	blit(framebuf,scrollbuf,0,0,256,0,256,224);
	
	// zap out
	FFCore.runGenericPassiveEngine(SCR_TIMING_END_FRAME);
	for(int32_t i=24; i>=1; i--)
	{
		draw_fuzzy(i);
		advanceframe(true);
		
		if(Quit)
		{
			break;
		}
	}
}


void wavyout(bool showhero)
{
	draw_screen(tmpscr, showhero);
	
	BITMAP *wavebuf = create_bitmap_ex(8,288,224);
	clear_to_color(wavebuf,0);
	blit(framebuf,wavebuf,0,0,16,0,256,224);
	
	static PALETTE wavepal;
	
	int32_t ofs;
	int32_t amplitude=8;
	
	int32_t wavelength=4;
	double palpos=0, palstep=4, palstop=126;
	
	FFCore.runGenericPassiveEngine(SCR_TIMING_END_FRAME);
	for(int32_t i=0; i<168; i+=wavelength)
	{
		for(int32_t l=0; l<256; l++)
		{
			wavepal[l].r=vbound(int32_t(RAMpal[l].r+((palpos/palstop)*(255-RAMpal[l].r))),0,255);
			wavepal[l].g=vbound(int32_t(RAMpal[l].g+((palpos/palstop)*(255-RAMpal[l].g))),0,255);
			wavepal[l].b=vbound(int32_t(RAMpal[l].b+((palpos/palstop)*(255-RAMpal[l].b))),0,255);
		}
		
		palpos+=palstep;
		
		if(palpos>=0)
		{
			hw_palette = &wavepal;
			update_hw_pal = true;
		}
		else
		{
			hw_palette = &RAMpal;
			update_hw_pal = true;
		}
		
		for(int32_t j=0; j+playing_field_offset<224; j++)
		{
			for(int32_t k=0; k<256; k++)
			{
				ofs=0;
				
				if((j<i)&&(j&1))
				{
					ofs=int32_t(zc::math::Sin((double(i+j)*2*PI/168.0))*amplitude);
				}
				
				framebuf->line[j+playing_field_offset][k]=wavebuf->line[j+playing_field_offset][k+ofs+16];
			}
		}
		
		advanceframe(true);

		if(Quit)
			break;
	}
	
	destroy_bitmap(wavebuf);

	hw_palette = &RAMpal;
	update_hw_pal = true;
}

void wavyin()
{
	draw_screen(tmpscr);
	
	BITMAP *wavebuf = create_bitmap_ex(8,288,224);
	clear_to_color(wavebuf,0);
	blit(framebuf,wavebuf,0,0,16,0,256,224);
	
	static PALETTE wavepal;

	refreshpal=false;
	int32_t ofs;
	int32_t amplitude=8;
	int32_t wavelength=4;
	double palpos=168, palstep=4, palstop=126;
	
	FFCore.runGenericPassiveEngine(SCR_TIMING_END_FRAME);
	for(int32_t i=0; i<168; i+=wavelength)
	{
		for(int32_t l=0; l<256; l++)
		{
			wavepal[l].r=vbound(int32_t(RAMpal[l].r+((palpos/palstop)*(255-RAMpal[l].r))),0,255);
			wavepal[l].g=vbound(int32_t(RAMpal[l].g+((palpos/palstop)*(255-RAMpal[l].g))),0,255);
			wavepal[l].b=vbound(int32_t(RAMpal[l].b+((palpos/palstop)*(255-RAMpal[l].b))),0,255);
		}
		
		palpos-=palstep;
		
		if(palpos>=0)
		{
			hw_palette = &wavepal;
			update_hw_pal = true;
		}
		else
		{
			hw_palette = &RAMpal;
			update_hw_pal = true;
		}
		
		for(int32_t j=0; j+playing_field_offset<224; j++)
		{
			for(int32_t k=0; k<256; k++)
			{
				ofs=0;
				
				if((j<(167-i))&&(j&1))
				{
					ofs=int32_t(zc::math::Sin((double(i+j)*2*PI/168.0))*amplitude);
				}
				
				framebuf->line[j+playing_field_offset][k]=wavebuf->line[j+playing_field_offset][k+ofs+16];
			}
		}
		
		advanceframe(true);
		
		if(Quit)
			break;
	}
	
	destroy_bitmap(wavebuf);

	hw_palette = &RAMpal;
	update_hw_pal = true;
}

void blackscr(int32_t fcnt,bool showsubscr)
{
	reset_pal_cycling();
	script_drawing_commands.Clear();
	
	FFCore.warpScriptCheck();
	bool showtime = game->should_show_time();
	while(fcnt>0)
	{
		clear_bitmap(framebuf);
		
		if(showsubscr)
		{
			put_passive_subscr(framebuf,0,passive_subscreen_offset,showtime,sspUP);
			if(get_qr(qr_SCRIPTDRAWSINWARPS) || (get_qr(qr_PASSIVE_SUBSCRIPT_RUNS_WHEN_GAME_IS_FROZEN)))
			{
				do_script_draws(framebuf, tmpscr, 0, playing_field_offset);
			}
		}
		
		advanceframe(true);
		
		if(Quit)
			break;
			
		--fcnt;
	}
}

void openscreen(int32_t shape)
{
	reset_pal_cycling();
	black_opening_count=0;
	
	if(COOLSCROLL || shape>-1)
	{
		open_black_opening(HeroX()+8, (HeroY()-HeroZ()-HeroFakeZ())+8+playing_field_offset, true, shape);
		return;
	}
	else
	{
		Hero.setDontDraw(true);
		show_subscreen_dmap_dots=false;
		show_subscreen_numbers=false;
		show_subscreen_life=false;
	}
	
	int32_t x=128;
	
	FFCore.warpScriptCheck();
	for(int32_t i=0; i<80; i++)
	{
		draw_screen(tmpscr);
		x=128-(((i*128/80)/8)*8);
		
		if(x>0)
		{
			rectfill(framebuf,0,playing_field_offset,x,167+playing_field_offset,0);
			rectfill(framebuf,256-x,playing_field_offset,255,167+playing_field_offset,0);
		}
		
		advanceframe(true);
		
		if(Quit)
		{
			break;
		}
	}
	
	Hero.setDontDraw(false);
	show_subscreen_items=true;
	show_subscreen_dmap_dots=true;
	show_subscreen_numbers=true;
	show_subscreen_life=true;
}

void closescreen(int32_t shape)
{
	reset_pal_cycling();
	black_opening_count=0;
	
	if(COOLSCROLL || shape>-1)
	{
		close_black_opening(HeroX()+8, (HeroY()-HeroZ()-HeroFakeZ())+8+playing_field_offset, true, shape);
		return;
	}
	else
	{
		Hero.setDontDraw(true);
		show_subscreen_dmap_dots=false;
		show_subscreen_numbers=false;
		//	show_subscreen_items=false;
		show_subscreen_life=false;
	}
	
	int32_t x=128;
	
	FFCore.warpScriptCheck();
	for(int32_t i=79; i>=0; --i)
	{
		draw_screen(tmpscr);
		x=128-(((i*128/80)/8)*8);
		
		if(x>0)
		{
			rectfill(framebuf,0,playing_field_offset,x,167+playing_field_offset,0);
			rectfill(framebuf,256-x,playing_field_offset,255,167+playing_field_offset,0);
		}
		
		advanceframe(true);
		
		if(Quit)
		{
			break;
		}
	}
	
	Hero.setDontDraw(false);
	show_subscreen_items=true;
	show_subscreen_dmap_dots=true;
}

int32_t TriforceCount()
{
	int32_t c=0;
	
	for(int32_t i=1; i<=8; i++)
		if(game->lvlitems[i]&liTRIFORCE)
			++c;
			
	return c;
}

int32_t onCustomGame()
{
	auto save = get_unset_save_slot();
	if (!save)
		return D_CLOSE;

	if (prompt_for_quest_path(save->header->qstpath))
	{
		save->header->qstpath = qstpath;
		return D_O_K;
	}

	return D_CLOSE;
}

int32_t onContinue()
{
	return D_CLOSE;
}

int32_t onThrottleFPS()
{
	Throttlefps = !Throttlefps;
	zc_set_config(cfg_sect,"throttlefps", (int32_t)Throttlefps);
	return D_O_K;
}

int32_t onWinPosSave()
{
	SaveWinPos = !SaveWinPos;
	zc_set_config(cfg_sect,"save_window_position",(int32_t)SaveWinPos);
	return D_O_K;
}
int32_t onIntegerScaling()
{
	scaleForceInteger = !scaleForceInteger;
	zc_set_config("zeldadx","scaling_force_integer",(int)scaleForceInteger);
	return D_O_K;
}
int32_t onStretchGame()
{
	stretchGame = !stretchGame;
	zc_set_config("zeldadx","stretch_game_area",stretchGame?1:0);
	return D_O_K;
}

int32_t onClickToFreeze()
{
	ClickToFreeze = !ClickToFreeze;
	zc_set_config(cfg_sect,"clicktofreeze", (int32_t)ClickToFreeze);
	return D_O_K;
}

int32_t OnSaveZCConfig()
{
	if(jwin_alert3(
			"Save Configuration", 
			"Are you sure that you wish to save your present configuration settings?", 
			"This will overwrite your prior settings!",
			NULL,
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)	
	{
		save_game_configs();
		return D_O_K;
	}
	else return D_O_K;
}

int32_t OnnClearQuestDir()
{
	auto current_path = fs::current_path() / "quests";
	if(jwin_alert3(
			"Clear Current Directory Cache", 
			"Are you sure that you wish to reset where ZC Player looks for quests?", 
			fmt::format("The new directory will be: {}", current_path.string()).c_str(),
			NULL,
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)	
	{
		zc_set_config("zeldadx","quest_dir","quests");
		flush_config_file();
		strcpy(qstdir,"quests");
#ifdef __EMSCRIPTEN__
		em_sync_fs();
#endif
		return D_O_K;
	}
	else return D_O_K;
}

int32_t onConsole()
{
	if ( !console_enabled )
	{
		AlertDialog("ZC Console",
			"Open the ZC Console?" 
			"\nThis will display any messages logged by scripts,"
			" including errors.",
			[&](bool ret,bool)
			{
				if(ret)
				{
					FFCore.ZScriptConsole(true);
				}
			}).show();
		return D_O_K;
	}
	else
	{
		FFCore.ZScriptConsole(false);
		return D_O_K;
	}
}

int32_t onClrConsoleOnReload()
{
	clearConsoleOnReload = !clearConsoleOnReload;
	zc_set_config("CONSOLE","clear_console_on_reload",clearConsoleOnReload?1:0);
	return D_O_K;
}
int32_t onClrConsoleOnLoad()
{
	clearConsoleOnLoad = !clearConsoleOnLoad;
	zc_set_config("CONSOLE","clear_console_on_load",clearConsoleOnLoad?1:0);
	return D_O_K;
}


int32_t onFrameSkip()
{
	FrameSkip = !FrameSkip;
	return D_O_K;
}

int32_t onSaveDragResize()
{
	SaveDragResize = !SaveDragResize;
	zc_set_config(cfg_sect,"save_drag_resize",(int32_t)SaveDragResize);
	return D_O_K;
}

int32_t onDragAspect()
{
	DragAspect = !DragAspect;
	zc_set_config(cfg_sect,"drag_aspect",(int32_t)DragAspect);
	return D_O_K;
}

int32_t onTransLayers()
{
	TransLayers = !TransLayers;
	zc_set_config(cfg_sect,"translayers",(int32_t)TransLayers);
	return D_O_K;
}

int32_t onNESquit()
{
	NESquit = !NESquit;
	zc_set_config(cfg_sect,"fastquit",(int32_t)NESquit);
	return D_O_K;
}

int32_t onVolKeys()
{
	volkeys = !volkeys;
	zc_set_config(sfx_sect,"volkeys",(int32_t)volkeys);
	return D_O_K;
}

int32_t onShowFPS()
{
	ShowFPS = !ShowFPS;
	zc_set_config(cfg_sect,"showfps",(int32_t)ShowFPS);
	return D_O_K;
}

int32_t onShowTime()
{
	ShowGameTime = !ShowGameTime;
	zc_set_config(cfg_sect,"showtime",ShowGameTime);
	return D_O_K;
}

bool is_Fkey(int32_t k)
{
	switch(k)
	{
	case KEY_F1:
	case KEY_F2:
	case KEY_F3:
	case KEY_F4:
	case KEY_F5:
	case KEY_F6:
	case KEY_F7:
	case KEY_F8:
	case KEY_F9:
	case KEY_F10:
	case KEY_F11:
	case KEY_F12:
		return true;
	}
	
	return false;
}

void kb_getkey(DIALOG *d);

//Used by all keyboard key settings dialogues.
void kb_clearjoystick(DIALOG *d)
{
	d->flags|=D_SELECTED;
	
	jwin_button_proc(MSG_DRAW,d,0);
	jwin_draw_win(screen, (screen->w-160)/2, (screen->h-48)/2, 160, 48, FR_WIN);
	//  text_mode(vc(11));
	textout_centre_ex(screen, font, "Press any key to clear", gui_bmp->w/2, gui_bmp->h/2 - 8, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	textout_centre_ex(screen, font, "ESC to cancel", gui_bmp->w/2, gui_bmp->h/2, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	
	update_hw_screen();
	
	clear_keybuf();
	int32_t k = next_press_key();
	clear_keybuf();
	
	//shnarf
	//47=f1
	//59=esc
//	if(k>0 && k<123 && !((k>46)&&(k<60)))
//		*((int32_t*)d->dp3) = k;
	if ( k != 59 ) *((int32_t*)d->dp3) = 0;
		
		
	d->flags&=~D_SELECTED;
}

//Clears key to 0. 
//Used by all keyboard key settings dialogues.
void kb_clearkey(DIALOG *d);

int32_t d_j_clearbutton_proc(int32_t msg,DIALOG *d,int32_t c)
{
	switch(msg)
	{
	case MSG_KEY:
	case MSG_CLICK:
	
		kb_clearjoystick(d);
		
		while(gui_mouse_b())
		{
			clear_keybuf();
			rest(1);
		}
			
		return D_REDRAW;
	}
	
	return jwin_button_proc(msg,d,c);
}

int32_t d_kbutton_proc(int32_t msg,DIALOG *d,int32_t c);
//Only used in keyboard settings dialogues to clear keys. 
int32_t d_k_clearbutton_proc(int32_t msg,DIALOG *d,int32_t c);

int32_t j_getbtn(DIALOG *d)
{
	d->flags|=D_SELECTED;
	jwin_button_proc(MSG_DRAW,d,0);
	jwin_draw_win(screen, (screen->w-160)/2, (screen->h-48)/2, 160, 48, FR_WIN);
	//  text_mode(vc(11));
	int32_t y = screen->h/2 - 12;
	textout_centre_ex(screen, font, "Press a button", screen->w/2, y, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	textout_centre_ex(screen, font, "ESC to cancel", screen->w/2, y+8, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	textout_centre_ex(screen, font, "SPACE to disable", screen->w/2, y+16, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	
	update_hw_screen();
	
	int32_t b = next_joy_input(true);
	if (b == -2)
		return D_CLOSE;
	
	if(b>=0)
		*((int32_t*)d->dp3) = b;
		
	d->flags&=~D_SELECTED;
	
	return D_O_K;
}

void j_getstick(DIALOG *d)
{
	d->flags|=D_SELECTED;
	jwin_button_proc(MSG_DRAW,d,0);
	jwin_draw_win(screen, (screen->w-160)/2, (screen->h-48)/2, 160, 48, FR_WIN);
	//  text_mode(vc(11));
	int32_t y = screen->h/2 - 12;
	textout_centre_ex(screen, font, "Move a stick (or DPAD)", screen->w/2, y, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	textout_centre_ex(screen, font, "ESC to cancel", screen->w/2, y+8, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	textout_centre_ex(screen, font, "SPACE to disable", screen->w/2, y+16, jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	
	update_hw_screen();
	
	int32_t b = next_joy_input(false);
	
	if(b>=0)
		*((int32_t*)d->dp3) = b;
		
	d->flags&=~D_SELECTED;
}

int32_t d_jbutton_proc(int32_t msg,DIALOG *d,int32_t c)
{
	switch(msg)
	{
	case MSG_KEY:
	case MSG_CLICK:

		int ret = j_getbtn(d);
		if (ret != D_O_K)
			return ret;
		
		while(gui_mouse_b()) {
			rest(1);
			clear_keybuf();
		}
			
		return D_REDRAW;
	}

	return jwin_button_proc(msg,d,c);
}

int32_t d_jstick_proc(int32_t msg,DIALOG *d,int32_t c)
{
	switch(msg)
	{
	case MSG_KEY:
	case MSG_CLICK:

		j_getstick(d);
		
		while(gui_mouse_b()) {
			rest(1);
			clear_keybuf();
		}
			
		return D_REDRAW;
	}

	return jwin_button_proc(msg,d,c);
}

//shnarf
extern const char *key_str[];
std::string get_keystr(int key);

const char *pan_str[4] = { "  MONO", "   1/2", "   3/4", "  FULL" };

static char str_a[80],str_b[80],str_s[80],str_m[80],str_l[80],str_r[80],str_p[80],str_ex1[80],str_ex2[80],str_ex3[80],str_ex4[80],
	str_leftmod1[80],str_leftmod2[80],str_rightmod1[80],str_rightmod2[80], str_left[80], str_right[80], str_up[80], str_down[80],
	str_primary_stick[80], str_secondary_stick[80];

int32_t d_stringloader(int32_t msg,DIALOG *d,int32_t c)
{
	//these are here to bypass compiler warnings about unused arguments
	c=c;

	if (d->w == 1)
	{
		if (!gamepad_dlg_cur_joystick || !al_get_joystick_active(gamepad_dlg_cur_joystick))
		{
			InfoDialog("ZC", "Invalid gamepad. Did it disconnect?").show();
			return D_CLOSE;
		}
	}

	if(msg==MSG_DRAW)
	{
		switch(d->w)
		{
			case 0:
				sprintf(str_a,"%03d\n%s",Akey,key_str[Akey]);
				sprintf(str_b,"%03d\n%s",Bkey,key_str[Bkey]);
				sprintf(str_s,"%03d\n%s",Skey,key_str[Skey]);
				sprintf(str_l,"%03d\n%s",Lkey,key_str[Lkey]);
				sprintf(str_r,"%03d\n%s",Rkey,key_str[Rkey]);
				sprintf(str_p,"%03d\n%s",Pkey,key_str[Pkey]);
				sprintf(str_ex1,"%03d\n%s",Exkey1,key_str[Exkey1]);
				sprintf(str_ex2,"%03d\n%s",Exkey2,key_str[Exkey2]);
				sprintf(str_ex3,"%03d\n%s",Exkey3,key_str[Exkey3]);
				sprintf(str_ex4,"%03d\n%s",Exkey4,key_str[Exkey4]);
				sprintf(str_up,"%03d\n%s",DUkey,key_str[DUkey]);
				sprintf(str_down,"%03d\n%s",DDkey,key_str[DDkey]);
				sprintf(str_left,"%03d\n%s",DLkey,key_str[DLkey]);
				sprintf(str_right,"%03d\n%s",DRkey,key_str[DRkey]);
				sprintf(str_leftmod1,"%03d\n%s",cheat_modifier_keys[0],key_str[cheat_modifier_keys[0]]);
				sprintf(str_leftmod2,"%03d\n%s",cheat_modifier_keys[1],key_str[cheat_modifier_keys[1]]);
				sprintf(str_rightmod1,"%03d\n%s",cheat_modifier_keys[2],key_str[cheat_modifier_keys[2]]);
				sprintf(str_rightmod2,"%03d\n%s",cheat_modifier_keys[3],key_str[cheat_modifier_keys[3]]);
				break;
				
			case 1:
				sprintf(str_a,"%03d\n%s",Abtn,joybtn_name(Abtn));
				sprintf(str_b,"%03d\n%s",Bbtn,joybtn_name(Bbtn));
				sprintf(str_s,"%03d\n%s",Sbtn,joybtn_name(Sbtn));
				sprintf(str_l,"%03d\n%s",Lbtn,joybtn_name(Lbtn));
				sprintf(str_r,"%03d\n%s",Rbtn,joybtn_name(Rbtn));
				sprintf(str_m,"%03d\n%s",Mbtn,joybtn_name(Mbtn));
				sprintf(str_p,"%03d\n%s",Pbtn,joybtn_name(Pbtn));
				sprintf(str_ex1,"%03d\n%s",Exbtn1,joybtn_name(Exbtn1));
				sprintf(str_ex2,"%03d\n%s",Exbtn2,joybtn_name(Exbtn2));
				sprintf(str_ex3,"%03d\n%s",Exbtn3,joybtn_name(Exbtn3));
				sprintf(str_ex4,"%03d\n%s",Exbtn4,joybtn_name(Exbtn4));
				sprintf(str_up,"%03d\n%s",DUbtn,joybtn_name(DUbtn));
				sprintf(str_down,"%03d\n%s",DDbtn,joybtn_name(DDbtn));
				sprintf(str_left,"%03d\n%s",DLbtn,joybtn_name(DLbtn));
				sprintf(str_right,"%03d\n%s",DRbtn,joybtn_name(DRbtn));
				sprintf(str_primary_stick,"%03d\n%s",js_stick_1_x_stick,joystick_name(js_stick_1_x_stick));
				sprintf(str_secondary_stick,"%03d\n%s",js_stick_2_x_stick,joystick_name(js_stick_2_x_stick));
				sprintf(str_leftmod1,"%03d\n%s",cheat_modifier_keys[0],key_str[cheat_modifier_keys[0]]);
				sprintf(str_leftmod2,"%03d\n%s",cheat_modifier_keys[1],key_str[cheat_modifier_keys[1]]);
				sprintf(str_rightmod1,"%03d\n%s",cheat_modifier_keys[2],key_str[cheat_modifier_keys[2]]);
				sprintf(str_rightmod2,"%03d\n%s",cheat_modifier_keys[3],key_str[cheat_modifier_keys[3]]);
				break;
				
			case 2:
				sprintf(str_a,"   %3d",midi_volume);
				sprintf(str_l,"   %3d",emusic_volume);
				sprintf(str_r,"   %3d",sfx_volume);
				strcpy(str_s,pan_str[pan_style]);
				sprintf(str_leftmod1,"%3d\n%s",cheat_modifier_keys[0],key_str[cheat_modifier_keys[0]]);
				sprintf(str_leftmod2,"%3d\n%s",cheat_modifier_keys[1],key_str[cheat_modifier_keys[1]]);
				sprintf(str_rightmod1,"%3d\n%s",cheat_modifier_keys[2],key_str[cheat_modifier_keys[2]]);
				sprintf(str_rightmod2,"%3d\n%s",cheat_modifier_keys[3],key_str[cheat_modifier_keys[3]]);
				break;
		}
	}
	
	return D_O_K;
}

int32_t set_vol(void *dp3, int32_t d2)
{
	switch(((int32_t*)dp3)[0])
	{
	case 0:
		midi_volume   = zc_min(d2<<3,255);
		break;
		
	case 1:
		digi_volume   = zc_min(d2<<3,255);
		break;
		
	case 2:
		emusic_volume = zc_min(d2<<3,255);
		break;
		
	case 3:
		sfx_volume	= zc_min(d2<<3,255);
		break;
	}
	
	// text_mode(vc(11));
	textprintf_right_ex(screen,get_zc_font(font_lfont_l), ((int32_t*)dp3)[1],((int32_t*)dp3)[2],jwin_pal[jcBOXFG],jwin_pal[jcBOX],"   %3d",zc_min(d2<<3,255));
	return D_O_K;
}

int32_t set_pan(void *dp3, int32_t d2)
{
	pan_style = vbound(d2,0,3);
	// text_mode(vc(11));
	textout_right_ex(screen,get_zc_font(font_lfont_l), pan_str[pan_style],((int32_t*)dp3)[1],((int32_t*)dp3)[2],jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
	return D_O_K;
}

static int32_t gamepad_joys_list[] =
{
	61,
	-1
};

static int32_t gamepad_btn_list[] =
{
	6,
	7,8,9,10,11,12,13,14,15,16,17,
	18,19,20,21,22,23,24,25,26,27,28,
	29,30,31,32,33,34,35,36,37,38,39,
	-1
};

static int32_t gamepad_dirs_list[] =
{
	40,41,42,43,
	44,45,46,47,
	48,49,50,51,
	52,53,54,55,
	56,57,58,59,
	60,
	-1
};

static TABPANEL gamepad_tabs[] =
{
	// (text)
	{ (char *)"Controllers",		D_SELECTED,  gamepad_joys_list, 0, NULL },
	{ (char *)"Buttons",		0,  gamepad_btn_list, 0, NULL },
	{ (char *)"Directions",  0,		   gamepad_dirs_list, 0, NULL },
	{ NULL,				  0,		   NULL,			   0, NULL }
};

const char *joy_list(int32_t index, int32_t *list_size)
{
	if (index == -1)
	{
		*list_size = al_get_num_joysticks();
		return NULL;
	}

	ALLEGRO_JOYSTICK* joy = al_get_joystick(index);
	if (!joy)
	{
		return "?";
	}

	return al_get_joystick_name(joy);
}

static ListData joy__list(joy_list, &font);

static int32_t d_joylist_proc(int32_t msg,DIALOG *d,int32_t c)
{
	int32_t d2 = d->d2;
	int32_t ret = jwin_droplist_proc(msg,d,c);
	
	if(d2!=d->d2)
	{
		joystick_index = d->d2;
		ret |= D_REDRAW_ALL;
	}
	
	return ret;
}

static DIALOG gamepad_dlg[] =
{
	// (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3)
	{ jwin_win_proc,	   8,	24,   304,  256,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Gamepad Controls", NULL,  NULL },
	{ jwin_tab_proc,	   8+4, 24+23,304-8,256-52,vc(0),   vc(15),  0,	   0,		 0,		0, (void *) gamepad_tabs, NULL, (void *)gamepad_dlg },
	{ d_stringloader,	  0,	0,	1,	0,	0,	   0,	   0,	   0,		 0,		0,	   NULL, NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ jwin_button_proc,	90,   254,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ jwin_button_proc,	170,  254,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Cancel", NULL,  NULL },
	// 6
	{ d_dummy_proc,	 14,   61,   294,  192,  0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	// 7
	{ jwin_ctext_proc,	  92,   92-20,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_a, NULL,  NULL },
	{ jwin_ctext_proc,	  92,   120-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_b, NULL,  NULL },
	{ jwin_ctext_proc,	  92,   148-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_s, NULL,  NULL },
	{ jwin_ctext_proc,	  92,   180-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex1, NULL,  NULL },
	{ jwin_ctext_proc,	  92,   212-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex3, NULL,  NULL },
	{ jwin_ctext_proc,	  237,  92-20,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_l, NULL,  NULL },
	{ jwin_ctext_proc,	  237,  120-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_r, NULL,  NULL },
	{ jwin_ctext_proc,	  237,  148-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_p, NULL,  NULL },
	{ jwin_ctext_proc,	  237,  180-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex2, NULL,  NULL },
	{ jwin_ctext_proc,	  237,  212-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex4, NULL,  NULL },
	{ jwin_ctext_proc,	  92,   244-20,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_m, NULL,  NULL },
	// 18
	{ d_jbutton_proc,	  22,   90-20,   61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "A",	 NULL, &Abtn},
	{ d_jbutton_proc,	  22,   118-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "B",	 NULL, &Bbtn},
	{ d_jbutton_proc,	  22,   146-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Start", NULL, &Sbtn},
	{ d_jbutton_proc,	  22,   178-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "X (EX1)",	 NULL, &Exbtn1},
	{ d_jbutton_proc,	  22,   210-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "EX3", NULL, &Exbtn3},
	{ d_jbutton_proc,	  167,  90-20,   61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "L",	 NULL, &Lbtn},
	{ d_jbutton_proc,	  167,  118-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "R",	 NULL, &Rbtn},
	{ d_jbutton_proc,	  167,  146-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Map",   NULL, &Pbtn},
	{ d_jbutton_proc,	  167,  178-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Y (EX2)",	 NULL, &Exbtn2},
	{ d_jbutton_proc,	  167,  210-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "EX4",   NULL, &Exbtn4},
	{ d_jbutton_proc,	  22,   242-20,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Menu",  NULL, &Mbtn},
	// 29
	{ d_j_clearbutton_proc,	  22+91,  90-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Abtn},
	{ d_j_clearbutton_proc,	  22+91,  118-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Bbtn},
	{ d_j_clearbutton_proc,	  22+91,  146-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Sbtn},
	{ d_j_clearbutton_proc,	  22+91,  178-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exbtn1},
	{ d_j_clearbutton_proc,	  22+91,  210-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exbtn3},
	{ d_j_clearbutton_proc,	  167+91,  90-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Lbtn},
	{ d_j_clearbutton_proc,	  167+91,  118-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Rbtn},
	{ d_j_clearbutton_proc,	  167+91,  146-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Pbtn},
	{ d_j_clearbutton_proc,	  167+91,  178-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exbtn2},
	{ d_j_clearbutton_proc,	  167+91,  210-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exbtn4},
	{ d_j_clearbutton_proc,	  22+91,  242-20,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Mbtn},
	// 40
	{ jwin_frame_proc,	 14,   62,   147,  80,  0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_frame_proc,	 159,  62,   147,  80,  0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,		 30,   68,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Vertical", NULL,  NULL },
	{ jwin_text_proc,		 175,  68,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Horizontal", NULL,  NULL },
	// 44
	{ jwin_text_proc,	  92,   84,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_up, NULL,  NULL },
	{ jwin_text_proc,	  92,   112,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_down, NULL,  NULL },
	{ jwin_text_proc,	  237,  84,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_left, NULL,  NULL },
	{ jwin_text_proc,	  237,  112,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_right, NULL,  NULL },
	// 48
	{ d_jbutton_proc,	  22,   82,   61,   21,   vc(14),  vc(11),  0,	   0,		 0,		0, (void *) "Up",	 NULL, &DUbtn },
	{ d_jbutton_proc,	  22,   110,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Down",   NULL, &DDbtn },
	{ d_jbutton_proc,	  167,  82,   61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Left",   NULL, &DLbtn },
	{ d_jbutton_proc,	  167,  110,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Right",  NULL, &DRbtn },
	// 52
	{ d_j_clearbutton_proc,	  22+91,  82,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DUbtn},
	{ d_j_clearbutton_proc,	  22+91,  110,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DDbtn},
	{ d_j_clearbutton_proc,	  167+91, 82,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",   NULL, &DLbtn},
	{ d_j_clearbutton_proc,	  167+91, 110,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DRbtn},
	// 56
	{ jwin_check_proc,	 22,   150,  147,  8,	vc(14),  vc(1),   0,	   0,		 1,		0, (void *) "Primary: Use Analog Stick (Ignore above and use below instead)", NULL, NULL },
	{ d_jstick_proc,	  22,   165,   61,   21,   vc(14),  vc(11),  0,	   0,		 0,		0, (void *) "Primary",	 NULL, &js_stick_1_x_stick },
	{ d_jstick_proc,	  22,   195,   61,   21,   vc(14),  vc(11),  0,	   0,		 0,		0, (void *) "Secondary",	 NULL, &js_stick_2_x_stick },
	{ jwin_text_proc,	  90,   165,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_primary_stick, NULL,  NULL },
	{ jwin_text_proc,	  90,   195,   60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_secondary_stick, NULL,  NULL },

	// 61
	{ d_joylist_proc,  22,   62,  150,  16,   0,	   0,	   0,	   0,		 0,		0, (void *) &joy__list, NULL,  NULL },

	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

static int32_t keyboard_keys_list[] =
{
	6,7,8,9,10,
	11,12,13,14,15,16,17,18,19,20,
	21,22,23,24,25,26,27,28,29,30,
	31,32,33,34,35,36,37,38,39,40,
	-1
};

static int32_t keyboard_dirs_list[] =
{
	41,42,43,44,
	45,46,47,48,
	49,50,51,52,
	53,54,55,56,
	-1
};

static int32_t keyboard_mods_list[] =
{
	57,58,59,60,
	61,62,63,64,
	65,66,67,68,
	69,70,71,72,
	-1
};

static TABPANEL keyboard_control_tabs[] =
{
	// (text)
	{ (char *)"Keys",		D_SELECTED,  keyboard_keys_list, 0, NULL },
	{ (char *)"Directions",  0,		   keyboard_dirs_list, 0, NULL },
	{ (char *)"Cheat Mods",  0,		   keyboard_mods_list, 0, NULL },
	{ NULL,				  0,		   NULL,			   0, NULL }
};

static DIALOG keyboard_control_dlg[] =
{
	// (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3)
	{ jwin_win_proc,	   8,	39,   304,  240,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Keyboard Controls", NULL,  NULL },
	{ jwin_tab_proc,	   8+4, 39+23,304-8,240-56,vc(0),   vc(15),  0,	   0,		 0,		0, (void *) keyboard_control_tabs, NULL, (void *)keyboard_control_dlg },
	{ d_stringloader,	  0,	0,	0,	0,	0,	   0,	   0,	   0,		 0,		0, NULL, NULL,  NULL },
	{ jwin_button_proc,	90,   254,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ jwin_button_proc,	170,  254,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Cancel", NULL,  NULL },
	{ d_timer_proc,		0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	// Keys
	// 6
	{ jwin_frame_proc,	 14,   80,   148,  105,  0,	   0,	   0,	   0,		 FR_ETCHED,0, NULL, NULL,  NULL },
	{ jwin_frame_proc,	 158,  80,   148,  105,  0,	   0,	   0,	   0,		 FR_ETCHED,0, NULL, NULL,  NULL },
	{ jwin_frame_proc,	 14,   181,  292,  67,   0,	   0,	   0,	   0,		 FR_ETCHED,0, NULL, NULL,  NULL },
	{ jwin_text_proc,	  30,   86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Standard", NULL,  NULL },
	{ jwin_text_proc,	  175,  86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Extended", NULL,  NULL },
	// 11
	{ jwin_text_proc,	  92-4,   102,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_a, NULL,  NULL },
	{ jwin_text_proc,	  92-4,   130,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_b, NULL,  NULL },
	{ jwin_text_proc,	  92-4,   158,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_s, NULL,  NULL },
	{ jwin_text_proc,	  92-4,   190,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex1, NULL,  NULL },
	{ jwin_text_proc,	  92-4,   222,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex3, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  102,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_l, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  130,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_r, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  158,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_p, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  190,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex2, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  222,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_ex4, NULL,  NULL },
	// 21
	{ d_kbutton_proc,	  22,   100,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "A",	 NULL, &Akey},
	{ d_kbutton_proc,	  22,   128,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "B",	 NULL, &Bkey},
	{ d_kbutton_proc,	  22,   156,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Start", NULL, &Skey},
	{ d_kbutton_proc,	  22,   188,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "X (EX1)",	 NULL, &Exkey1},
	{ d_kbutton_proc,	  22,   220,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "EX3", NULL, &Exkey3},
	{ d_kbutton_proc,	  167,  100,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "L",	 NULL, &Lkey},
	{ d_kbutton_proc,	  167,  128,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "R",	 NULL, &Rkey},
	{ d_kbutton_proc,	  167,  156,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Map",   NULL, &Pkey},
	{ d_kbutton_proc,	  167,  188,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Y (EX2)",	 NULL, &Exkey2},
	{ d_kbutton_proc,	  167,  220,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "EX4",   NULL, &Exkey4},
	// 31
	{ d_k_clearbutton_proc,	  22+91,  100,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Akey},
	{ d_k_clearbutton_proc,	  22+91,  128,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Bkey},
	{ d_k_clearbutton_proc,	  22+91,  156,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Skey},
	{ d_k_clearbutton_proc,	  22+91,  188,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exkey1},
	{ d_k_clearbutton_proc,	  22+91,  220,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exkey3},
	{ d_k_clearbutton_proc,	  167+91, 100,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Lkey},
	{ d_k_clearbutton_proc,	  167+91, 128,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Rkey},
	{ d_k_clearbutton_proc,	  167+91, 156,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Pkey},
	{ d_k_clearbutton_proc,	  167+91, 188,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exkey2},
	{ d_k_clearbutton_proc,	  167+91, 220,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &Exkey4},
	// Dirs
	// 41
	{ jwin_frame_proc,	 14,   80,   147,  80,   0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_frame_proc,	 159,  80,   147,  80,   0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,		 30,   86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Vertical", NULL,  NULL },
	{ jwin_text_proc,		 175,  86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Horizontal", NULL,  NULL },
	// 45
	{ jwin_text_proc,	  92-4,   102,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_up, NULL,  NULL },
	{ jwin_text_proc,	  92-4,   130,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_down, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  102,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_left, NULL,  NULL },
	{ jwin_text_proc,	  237-4,  130,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_right, NULL,  NULL },
	// 49
	{ d_kbutton_proc,	  22,   100,  61,   21,   vc(14),  vc(11),  0,	   0,		 0,		0, (void *) "Up",	 NULL, &DUkey},
	{ d_kbutton_proc,	  22,   128,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Down",   NULL, &DDkey},
	{ d_kbutton_proc,	  167,  100,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Left",   NULL, &DLkey},
	{ d_kbutton_proc,	  167,  128,  61,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Right",  NULL, &DRkey},
	// 53
	{ d_k_clearbutton_proc,	  22+91,  100,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DUkey},
	{ d_k_clearbutton_proc,	  22+91,  128,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DDkey},
	{ d_k_clearbutton_proc,	  167+91, 100,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DLkey},
	{ d_k_clearbutton_proc,	  167+91, 128,   40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &DRkey},
	// Mods
	// 57
	{ jwin_frame_proc,	 14,   80,   148,  140-58,  0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_frame_proc,	 158,  80,   148,  140-58,  0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,	  30,   86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Left", NULL,  NULL },
	{ jwin_text_proc,	  175,  86,   160,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Right", NULL,  NULL },
	// 61
	{ jwin_text_proc,	  92-26,   101,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_leftmod1, NULL,  NULL },
	{ jwin_text_proc,	  92-26,   129,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_rightmod1, NULL,  NULL },
	{ jwin_text_proc,	  237-4-22,101,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_leftmod2, NULL,  NULL },
	{ jwin_text_proc,	  237-4-22,129,  60,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_rightmod2, NULL,  NULL },
	// 65
	{ d_kbutton_proc,	  22,   100,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Main",	 NULL, &cheat_modifier_keys[0]},
	{ d_kbutton_proc,	  22,   128,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Second",	 NULL, &cheat_modifier_keys[2]},
	{ d_kbutton_proc,	  167,  100,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Main", NULL, &cheat_modifier_keys[1]},
	{ d_kbutton_proc,	  167,  128,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Second",	 NULL, &cheat_modifier_keys[3]},
	// 69
	{ d_k_clearbutton_proc,	  22+91,  100,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &cheat_modifier_keys[0]},
	{ d_k_clearbutton_proc,	  22+91,  128,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &cheat_modifier_keys[2]},
	{ d_k_clearbutton_proc,	  167+91, 100,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",   NULL, &cheat_modifier_keys[1]},
	{ d_k_clearbutton_proc,	  167+91, 128,  40,   21,   vc(14),  vc(1),   0,	   0,		 0,		0, (void *) "Clear",	 NULL, &cheat_modifier_keys[3]},
	// 73
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

int32_t midi_dp[3] = {0,0,0};
int32_t emus_dp[3] = {2,0,0};
int32_t sfx_dp[3]  = {3,0,0};
int32_t pan_dp[3]  = {0,0,0};

static DIALOG sound_dlg[] =
{
	//(dialog proc)		  (x)	 (y)	(w)	 (h)	(fg)	   (bg)				 (key) (flags)	   (d1)		   (d2) (dp)						   (dp2)			   (dp3)
	{ jwin_win_proc,		  0,	  0,	320,	178,	0,		 0,				   0,	D_EXIT,	   0,			 0, (void *) "Sound Settings",	  NULL,			   NULL	 },
	{ d_stringloader,		 0,	  0,	  2,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_button_proc,	  58,	148,	 61,	 21,	0,		 0,				   0,	D_EXIT,	   0,			 0, (void *) "OK",				  NULL,			   NULL	 },
	{ jwin_button_proc,	 138,	148,	 61,	 21,	0,		 0,				   0,	D_EXIT,	   0,			 0, (void *) "Cancel",			  NULL,			   NULL	 },
	{ d_timer_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_frame_proc,	   10,	 28,	300,	112,	0,		 0,				   0,	0,			FR_ETCHED,	 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_rtext_proc,	  190,	 40,	 40,	  8,	vc(7),	 vc(11),			  0,	0,			0,			 0, (void *) str_a,				 NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_rtext_proc,	  190,	 56,	 40,	  8,	vc(7),	 vc(11),			  0,	0,			0,			 0, (void *) str_l,				 NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	// 10
	{ jwin_rtext_proc,	  190,	 72,	 40,	  8,	vc(7),	 vc(11),			  0,	0,			0,			 0, (void *) str_r,				 NULL,			   NULL	 },
	{ jwin_rtext_proc,	  190,	 88,	 40,	  8,	vc(7),	 vc(11),			  0,	0,			0,			 0, (void *) str_s,				 NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_slider_proc,	 196,	 40,	 96,	  8,	vc(0),	 jwin_pal[jcBOX],	 0,	0,		   32,			 0,  NULL, (void *) set_vol,   midi_dp  },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_slider_proc,	 196,	 56,	 96,	  8,	vc(0),	 jwin_pal[jcBOX],	 0,	0,		   32,			 0,  NULL, (void *) set_vol,   emus_dp  },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_slider_proc,	 196,	 72,	 96,	  8,	vc(0),	 jwin_pal[jcBOX],	 0,	0,		   32,			 0,  NULL, (void *) set_vol,   sfx_dp   },
	//20
	{ jwin_slider_proc,	 196,	88,	 96,	  8,	vc(0),	 jwin_pal[jcBOX],	 0,	0,			3,			 0,  NULL, (void *) set_pan,   pan_dp   },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_text_proc,		17,	 40,	 48,	  8,	vc(0),	 vc(11),			  0,	0,			0,			 0, (void *) "MIDI Volume",  NULL,			   NULL	 },
	{ jwin_text_proc,		17,	 56,	 48,	  8,	vc(0),	 vc(11),			  0,	0,			0,			 0, (void *) "Music Volume (Enhanced Music)",	 NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ jwin_text_proc,		17,	 72,	 48,	  8,	vc(0),	 vc(11),			  0,	0,			0,			 0, (void *) "SFX Volume",		  NULL,			   NULL	 },
	{ jwin_text_proc,		17,	 88,	 48,	  8,	vc(0),	 vc(11),			  0,	0,			0,			 0, (void *) "SFX Pan",			 NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	//30
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ d_dummy_proc,		   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
	{ NULL,				   0,	  0,	  0,	  0,	0,		 0,				   0,	0,			0,			 0,  NULL,						   NULL,			   NULL	 },
};

char zc_builddate[80];
char zc_aboutstr[80];

static DIALOG about_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3) */
	{ jwin_win_proc,	   68,   52,   184,  154,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "About", NULL,  NULL },
	{ jwin_button_proc,	140,  176,  41,   21,   vc(14),  0,	   0,	   D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ jwin_ctext_proc,		160,  84,   0,	8,	vc(0),   vc(11),  0,	   0,		 0,		0, zc_aboutstr, NULL,  NULL },
	{ jwin_ctext_proc,		160,  92,   0,	8,	vc(0) ,  vc(11),  0,	   0,		 0,		0, str_s, NULL,  NULL },
	{ jwin_ctext_proc,		160,  100,  0,	8,	vc(0) ,  vc(11),  0,	   0,		 0,		0, zc_builddate, NULL,  NULL },
	{ jwin_text_proc,		 88,   124,  140,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Coded by:", NULL,  NULL },
	{ jwin_text_proc,		 88,   132,  140,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "  Phantom Menace", NULL,  NULL },
	{ jwin_text_proc,		 88,   144,  140,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Produced by:", NULL,  NULL },
	{ jwin_text_proc,		 88,   152,  140,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "  Armageddon Games", NULL,  NULL },
	{ jwin_frame_proc,	 80,   117,  160,  50,   0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};


static DIALOG quest_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3) */
	{ jwin_win_proc,	   68,   25,   184,  190,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Quest Info", NULL,  NULL },
	{ jwin_edit_proc,	  84,   54,   152,  16,   0,	   0,	   0,	   D_READONLY, 100,	 0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,		 89,   84,   141,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Number:", NULL,  NULL },
	{ jwin_text_proc,		 152,  84,   24,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_a, NULL,  NULL },
	{ jwin_text_proc,		 89,   94,   141,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Version:", NULL,  NULL },
	{ jwin_text_proc,		 160,  94,   64,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   header_version_nul_term, NULL,  NULL },
	{ jwin_text_proc,		 89,   104,  141,  8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "ZQ Version:", NULL,  NULL },
	{ jwin_text_proc,		 130,  104,  64,   8,	vc(7),   vc(11),  0,	   0,		 0,		0,	   str_s, NULL,  NULL },
	{ jwin_text_proc,		 84,   126,  80,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Title:", NULL,  NULL },
	{ jwin_textbox_proc,   84,   136,  152,  24,   0,	   0,	   0,	   0,		 0,		0,	   QHeader.title, NULL,  NULL },
	{ jwin_text_proc,		 84,   168,  80,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Author:", NULL,  NULL },
	{ jwin_textbox_proc,   84,   178,  152,  24,   0,	   0,	   0,	   0,		 0,		0,	   QHeader.author, NULL,  NULL },
	{ jwin_frame_proc,	 84,   79,   152,  38,   0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

static DIALOG triforce_dlg[] =
{
	/* (dialog proc)	 (x)   (y)   (w)   (h)   (fg)	 (bg)	(key)	(flags)	 (d1)		   (d2)	 (dp) */
	{ jwin_win_proc,	  72,	64,  177,  105,  vc(14),   vc(1),	0,  D_EXIT,		   0,		0, (void *) "Triforce Pieces", NULL,  NULL },
	// 1
	{ jwin_check_proc,   129,	94,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "1", NULL,  NULL },
	{ jwin_check_proc,   129,   104,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "2", NULL,  NULL },
	{ jwin_check_proc,   129,   114,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "3", NULL,  NULL },
	{ jwin_check_proc,   129,   124,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "4", NULL,  NULL },
	{ jwin_check_proc,   172,	94,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "5", NULL,  NULL },
	{ jwin_check_proc,   172,   104,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "6", NULL,  NULL },
	{ jwin_check_proc,   172,   114,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "7", NULL,  NULL },
	{ jwin_check_proc,   172,   124,   24,	9,   vc(0),  vc(11),	0,	   0,		   1,		0, (void *) "8", NULL,  NULL },
	// 9
	{ jwin_button_proc,  90,	144,   61,   21,   vc(0),  vc(11),  'k',  D_EXIT,		   0,		0, (void *) "O&K", NULL,  NULL },
	{ jwin_button_proc,  170,   144,   61,   21,   vc(0),  vc(11),   27,  D_EXIT,		   0,		0, (void *) "Cancel", NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

int32_t onToggleRecordingNewSaves()
{
	if (zc_get_config("zeldadx", "replay_new_saves", false))
	{
		zc_set_config("zeldadx", "replay_new_saves", false);
	}
	else
	{
		zc_set_config("zeldadx", "replay_new_saves", true);
		jwin_alert("Recording", "Newly created saves will be recorded and written to a replay file.",
			NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
	}
	return D_O_K;
}

#ifdef HAS_CURL
int32_t onToggleAutoUploadReplays()
{
	if (zc_get_config("zeldadx", "replay_upload", false))
	{
		zc_set_config("zeldadx", "replay_upload", false);
	}
	else
	{
		zc_set_config("zeldadx", "replay_upload", true);
		jwin_alert("Replays", "Replays will be automatically uploaded. This helps development by",
			" preventing bugs and simplifying bug reports.",
			"Upload will happen no more than once a week when closing ZC",
			"OK",NULL,13,27,get_zc_font(font_lfont));

		if (!zc_get_config("zeldadx", "replay_new_saves", false))
			onToggleRecordingNewSaves();
	}
	return D_O_K;
}

int32_t onUploadReplays()
{
	if(jwin_alert3(
			"Upload replays", 
			"Upload your replays now to assist in development?",
			NULL,
			NULL,
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)	
	{
		int num_uploaded = replay_upload();
		jwin_alert("Upload replays", fmt::format("Uploaded {} replays", num_uploaded).c_str(),
			NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
	}
	return D_O_K;
}

int32_t onClearUploadCache()
{
	if(jwin_alert3(
			"Upload replays", 
			"Clear the upload cache?",
			"This simply deletes replays/state.json. There's no harm in doing this, but",
			"likely is not necessary.",
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)	
	{
		replay_upload_clear_cache();
	}
	return D_O_K;
}
#endif

int32_t onToggleSnapshotAllFrames()
{
	replay_set_snapshot_all_frames(!replay_is_snapshot_all_frames());
	return D_O_K;
}

int32_t onStopReplayOrRecord()
{
	if (replay_is_replaying())
	{
		replay_quit();
	}
	else if (replay_get_mode() == ReplayMode::Record)
	{
		if (!replay_get_meta_bool("test_mode"))
		{
			jwin_alert("Recording", "You cannot stop recording a save file.",
				NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
			return D_CLOSE;
		}

		if (jwin_alert("Stop Recording",
			"Save replay to disk and stop recording?",
			"This will stop the recording.",
			NULL,
			"Yes","No",13,27,get_zc_font(font_lfont)) != 1)
		return D_CLOSE;

		replay_save();
		replay_stop();
	}
	return D_O_K;
}

static int32_t handle_on_load_replay(ReplayMode mode)
{
	bool ctrl = CHECK_CTRL_CMD;
	if (Playing)
	{
		if (jwin_alert("Replay - Warning!",
			"Loading a replay will exit the current game.",
			"All unsaved progress will be lost.",
			"Do you wish to continue?",
			"Yes","No",13,27,get_zc_font(font_lfont)) != 1)
		return D_CLOSE;
	}

	std::string mode_string = replay_mode_to_string(mode);
	mode_string[0] = std::toupper(mode_string[0]);

	std::string line_1 = "Select a replay file to play back.";
	std::string line_2 = "You won't be able to save, and it won't effect existing saves.";
	std::string line_3 = "You can stop the replay and take over manually any time.";
	if (mode == ReplayMode::Update)
	{
		line_1 = "Select a replay file to update.";
		line_2 = "WARNING: be sure to back up the zplay file";
		line_3 = "and verify that the updated replay works as expected!";
	}

	if (jwin_alert(mode_string.c_str(),
		line_1.c_str(),
		line_2.c_str(),
		line_3.c_str(),
		"OK","Nevermind",13,27,get_zc_font(font_lfont)) == 1)
	{
		std::string replay_path = "replays/";
		if(ctrl && devpwd())
			replay_path = "../../tests/replays/";
		std::string prompt = fmt::format("Load Replay ({})", REPLAY_EXTENSION);
		if (auto result = prompt_for_existing_file(prompt, REPLAY_EXTENSION, nullptr, replay_path))
			replay_path = *result;
		else
			return D_CLOSE;

		replay_quit();
		load_replay_file_deferred(mode, replay_path);
		Quit = qRESET;
		return D_CLOSE;
	}
	return D_O_K;
}

int32_t onLoadReplay()
{
	return handle_on_load_replay(ReplayMode::Replay);
}

int32_t onLoadReplayAssert()
{
	return handle_on_load_replay(ReplayMode::Assert);
}

int32_t onLoadReplayUpdate()
{
	return handle_on_load_replay(ReplayMode::Update);
}

int32_t onSaveReplay()
{
	if (replay_get_mode() == ReplayMode::Record)
	{
		if (!replay_get_meta_bool("test_mode"))
		{
			if (jwin_alert("Save Replay",
				"This will save a copy of the replay up to this point.",
				"The official replay file will be untouched.",
				"Do you wish to continue?",
				"Yes","No",13,27,get_zc_font(font_lfont)) != 1)
			{
				return D_CLOSE;
			}

			std::string replay_path = replay_get_replay_path().string();
			std::string prompt = fmt::format("Save Replay ({})", REPLAY_EXTENSION);
			if (auto result = prompt_for_new_file(prompt, REPLAY_EXTENSION, nullptr, replay_path))
				replay_path = *result;
			else
				return D_CLOSE;

			if (fileexists(replay_path.c_str()))
			{
				jwin_alert("Save Replay", "You cannot overwrite an existing file.",
					NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
				return D_CLOSE;
			}

			replay_save(replay_path);
		}
		else
		{
			replay_save();
		}
	}
	return D_O_K;
}

enum
{
	MENUID_REPLAY_RECORDNEW,
	MENUID_REPLAY_STOP,
	MENUID_REPLAY_SAVE,
	MENUID_REPLAY_SNAP_ALL,
	MENUID_REPLAY_AUTOUPLOAD,
	MENUID_REPLAY_UPLOAD,
	MENUID_REPLAY_CLEARUPLOADCACHE,
};
static NewMenu replay_menu
{
	{ "Record new saves", onToggleRecordingNewSaves, MENUID_REPLAY_RECORDNEW },
	{},
	{ "Stop replay", onStopReplayOrRecord, MENUID_REPLAY_STOP },
	{ "Load replay", onLoadReplay },
	{ "Load replay (assert)", onLoadReplayAssert },
	{ "Load replay (update)", onLoadReplayUpdate },
	{ "Save replay", onSaveReplay, MENUID_REPLAY_SAVE },
	{ "Snapshot all frames", onToggleSnapshotAllFrames, MENUID_REPLAY_SNAP_ALL },
#ifdef HAS_CURL
	{},
	{ "Auto upload replays", onToggleAutoUploadReplays, MENUID_REPLAY_AUTOUPLOAD },
	{ "Upload replays", onUploadReplays, MENUID_REPLAY_UPLOAD },
	{ "Clear upload cache", onClearUploadCache, MENUID_REPLAY_CLEARUPLOADCACHE },
#endif
};

static DIALOG credits_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3) */
	{ jwin_win_proc,	   40,   38,   241,  173,  vc(14),  vc(1),   0,	   D_EXIT,	0,		0, (void *) "ZQuest Classic Credits", NULL,  NULL },
	{ jwin_frame_proc,	 47,   65,   227,  115,  vc(15),  vc(1),   0,	   0,		 FR_DEEP,  0,	   NULL, NULL,  NULL },
	{ d_bitmap_proc,	   49,   67,   222,  110,  vc(15),  vc(1),   0,	   0,		 0,		0,	   NULL, NULL,  NULL },
	{ jwin_button_proc,	140,  184,  41,   21,   vc(14),  vc(1),   0,	   D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

static ListData dmap_list(dmaplist, &font);

static DIALOG goto_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)					 (dp2) (dp3) */
	{ jwin_win_proc,	   48,   25,   205,  100,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Goto Location", NULL,  NULL },
	{ jwin_button_proc,	90,   176-78,  61,   21,   vc(14),  0,	   13,	 D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ jwin_button_proc,	170,  176-78,  61,   21,   vc(14),  0,	   27,	 D_EXIT,	0,		0, (void *) "Cancel", NULL,  NULL },
	{ jwin_text_proc,	  55,   129-75,  80,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "DMap:", NULL,  NULL },
	{ jwin_droplist_proc,	  88,  126-75,  160,  16,   0,	   0,	   0,	   0,		 0,		0, (void *) &dmap_list, NULL,  NULL },
	{ jwin_text_proc,	  55,   149-75,  80,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Screen:", NULL,  NULL },
	{ jwin_edit_proc,	  132,  146-75,  91,   16,   0,	   0,	   0,	   0,		 2,		0,	   NULL, NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

int32_t onGoTo()
{
	bool music = false;
	music = music;
	sprintf(cheat_goto_screen_str,"%X",cheat_goto_screen);
	
	goto_dlg[0].dp2=get_zc_font(font_lfont);
	goto_dlg[4].d2=cheat_goto_dmap;
	goto_dlg[6].dp=cheat_goto_screen_str;
	
	clear_keybuf();
	
	large_dialog(goto_dlg);
		
	if(do_zqdialog(goto_dlg,4)==1)
	{
		int dmap = goto_dlg[4].d2;
		int screen = zc_xtoi(cheat_goto_screen_str);
		int adjusted_screen = screen + DMaps[dmap].xoff;
		if (adjusted_screen < 0 || adjusted_screen >= 128)
		{
			InfoDialog("Invalid screen", fmt::format("The screen {:02X} is out of bounds.", adjusted_screen)).show();
		}
		else
		{
			cheats_enqueue(Cheat::GoTo, dmap, screen);
		}
	};
	
	return D_O_K;
}

int32_t onGoToComplete()
{
	if(!Playing)
	{
		return D_O_K;
	}
	
	enter_sys_pal();
	music_pause();
	pause_all_sfx();
	onGoTo();
	eat_buttons();
	
	zc_readrawkey(KEY_ESC);
	
	exit_sys_pal();
	music_resume();
	resume_all_sfx();
	return D_O_K;
}

int32_t onCredits()
{
	return D_O_K;
}

const char *midilist(int32_t index, int32_t *list_size)
{
	if(index<0)
	{
		*list_size=0;
		
		for(int32_t i=0; i<MAXMIDIS; i++)
			if(tunes[i].data)
				++(*list_size);
				
		return NULL;
	}
	
	int32_t i=0,m=0;
	
	while(m<=index && i<=MAXMIDIS)
	{
		if(tunes[i].data)
			++m;
			
		++i;
	}
	
	--i;
	
	if(i==MAXMIDIS && m<index)
		return "(null)";
		
	return tunes[i].title;
}

/*  ------- MIDI info stuff -------- */

char *text;
midi_info *zmi;
bool dialog_running;
bool listening;

void get_info(int32_t index);

int32_t d_midilist_proc(int32_t msg,DIALOG *d,int32_t c)
{
	int32_t d2 = d->d2;
	int32_t ret = jwin_droplist_proc(msg,d,c);
	
	if(d2!=d->d2)
	{
		get_info(d->d2);
	}
	
	return ret;
}

int32_t d_listen_proc(int32_t msg,DIALOG *d,int32_t c)
{
	/* 'd->d1' is offset from 'd' in DIALOG array to midilist proc */
	
	int32_t ret = jwin_button_proc(msg,d,c);
	
	if(ret == D_CLOSE)
	{
		// get current midi index
		int32_t index = (d+(d->d1))->d2;
		int32_t i=0, m=0;
		
		while(m<=index && i<=MAXMIDIS)
		{
			if(tunes[i].data)
				++m;
				
			++i;
		}
		
		--i;
		jukebox(i);
		listening = true;
		ret = D_O_K;
	}
	
	return ret;
}

int32_t d_savemidi_proc(int32_t msg,DIALOG *d,int32_t c)
{
	/* 'd->d1' is offset from 'd' in DIALOG array to midilist proc */
	
	int32_t ret = jwin_button_proc(msg,d,c);
	
	if(ret == D_CLOSE)
	{
		// get current midi index
		int32_t index = (d+(d->d1))->d2;
		int32_t i=0, m=0;
		
		while(m<=index && i<=MAXMIDIS)
		{
			if(tunes[i].data)
				++m;
				
			++i;
		}
		
		--i;

		char title[40] = "Save MIDI: ";
		static EXT_LIST list[] =
		{
			{ (char *)"MIDI files (*.mid)", (char *)"mid" },
			{ NULL,								  NULL }
		};
		
		strcpy(title+11, tunes[i].title);
	title[39] = '\0';

		std::string fname;
		if (auto result = prompt_for_new_file(title, "", list, "tune.mid"))
			fname = *result;
		else
			goto done;
			
		if(exists(fname.c_str()))
		{
			if(jwin_alert(title, fname.c_str(), "already exists.", "Overwrite it?", "&Yes","&No",'y','n',get_zc_font(font_lfont))==2)
				goto done;
		}
		
		// save midi i
		
		if (save_midi(fname.c_str(), tunes[i].data) != 0)
			jwin_alert(title, "Error saving MIDI to", fname.c_str(), NULL, "Darn", NULL,13,27,get_zc_font(font_lfont));
			
done:
		chop_path(fname.data());
		ret = D_REDRAW;
	}
	
	return ret;
}

static ListData midi_list(midilist, &font);

static DIALOG midi_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)	 (dp2) (dp3) */
	{ jwin_win_proc,	   8,	28,   304,  184,  0,	   0,		0,	   D_EXIT,	0,		0, (void *) "MIDI Info", NULL,  NULL },
	{ jwin_text_proc,		 32,   60,   40,   8,	vc(0),   vc(11),   0,	   0,		 0,		0, (void *) "Tune:", NULL,  NULL },
	{ d_midilist_proc,	 80,   56,   192,  16,   0,	   0,		0,	   0,		 0,		0, (void *) &midi_list, NULL,  NULL },
	{ jwin_textbox_proc,   15,   80,   290,  96,   0,	   0,		0,	   0,		 0,		0,	   NULL, NULL,  NULL },
	{ d_listen_proc,	   24,   183,  72,   21,   0,	   0,		'l',	 D_EXIT,	-2,	   0, (void *) "&Listen", NULL,  NULL },
	{ d_savemidi_proc,	 108,  183,  72,   21,   0,	   0,		's',	 D_EXIT,	-3,	   0, (void *) "&Save", NULL,  NULL },
	{ jwin_button_proc,	236,  183,  61,   21,   0,	   0,		'k',	 D_EXIT,	0,		0, (void *) "O&K", NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

void get_info(int32_t index)
{
	int32_t i=0, m=0;
	
	while(m<=index && i<=MAXMIDIS)
	{
		if(tunes[i].data)
			++m;
			
		++i;
	}
	
	--i;
	
	if(i==MAXMIDIS && m<index)
		strcpy(text,"(null)");
	else
	{
		get_midi_info(tunes[i].data,zmi);
		get_midi_text(tunes[i].data,zmi,text);
	}
	
	midi_dlg[0].dp2=get_zc_font(font_lfont);
	midi_dlg[3].dp = text;
	midi_dlg[3].d1 = midi_dlg[3].d2 = 0;
	midi_dlg[5].flags = (tunes[i].flags&tfDISABLESAVE) ? D_DISABLED : D_EXIT;
	
	if(dialog_running)
	{
		jwin_textbox_proc(MSG_DRAW,midi_dlg+3,0);
		d_savemidi_proc(MSG_DRAW,midi_dlg+5,0);
	}
}

int32_t onMIDICredits()
{
	text = (char*)malloc(4096);
	zmi = (midi_info*)malloc(sizeof(midi_info));
	
	if(!text || !zmi)
	{
		jwin_alert(NULL,"Not enough memory",NULL,NULL,"OK",NULL,13,27,get_zc_font(font_lfont));
		return D_O_K;
	}
	
	bool do_pause_midi = midi_pos >= 0 && currmidi;
	auto restore_midi = currmidi;
	if(do_pause_midi)
	{
		paused_midi_pos = midi_pos;
		stop_midi();
		midi_suspended = midissuspHALTED;
	}
	
	midi_dlg[0].dp2=get_zc_font(font_lfont);
	midi_dlg[2].d1 = 0;
	midi_dlg[2].d2 = 0;
	midi_dlg[4].flags = D_EXIT;
	midi_dlg[5].flags = (tunes[midi_dlg[2].d1].flags&tfDISABLESAVE) ? D_DISABLED : D_EXIT;
	
	listening = false;
	dialog_running=false;
	get_info(0);
	
	dialog_running=true;
	
	large_dialog(midi_dlg);
		
	do_zqdialog(midi_dlg,0);
	dialog_running=false;
	
	if(listening)
		music_stop();
	
	if(do_pause_midi)
	{
		// TODO: this probably doesn't resume midis nicely when scrolling (or in some other inner-gameloop).
		midi_suspended = midissuspRESUME;
		currmidi = restore_midi;
		midi_pos = paused_midi_pos;
	}
		
	if(text) free(text);
	if(zmi) free(zmi);
	return D_O_K;
}

int32_t onAbout()
{
	char buf1[80]={0};
	std::ostringstream oss;
	sprintf(buf1,ZC_PLAYER_NAME);
	oss << buf1 << '\n';
	sprintf(buf1,"Version: %s", getVersionString());
	oss << buf1 << '\n';
	sprintf(buf1,"Build Date: %s %s, %d at @ %s %s", dayextension(BUILDTM_DAY).c_str(), (char*)months[BUILDTM_MONTH], BUILDTM_YEAR, __TIME__, __TIMEZONE__);
	oss << buf1 << '\n';
	sprintf(buf1, "Built By: %s", DEV_SIGNOFF);
	oss << buf1 << '\n';
	
	InfoDialog("About ZC", oss.str()).show();
	return D_O_K;
}

int32_t onQuest()
{
	char fname[100];
	strcpy(fname, get_filename(qstpath));
	quest_dlg[0].dp2=get_zc_font(font_lfont);
	quest_dlg[1].dp = fname;
	
	if(QHeader.quest_number==0)
		sprintf(str_a,"Custom");
	else
		sprintf(str_a,"%d",QHeader.quest_number);
		
	sprintf(str_s,"%s",QHeader.getVerStr());
	
	quest_dlg[11].d1 = quest_dlg[9].d1 = 0;
	quest_dlg[11].d2 = quest_dlg[9].d2 = 0;
	
	large_dialog(quest_dlg);
		
	do_zqdialog(quest_dlg, 0);
	return D_O_K;
}

void call_vidmode_dlg();
int32_t onVidMode()
{
	call_vidmode_dlg();
	return D_O_K;
}

#define addToHash(c,b,h) if(h->find(c ## key) == h->end()) \
{(*h)[c ## key]=true;} else { if ( c ## key != 0 ) b = false;}
//Added an extra statement, so that if the key is cleared to 0, the cleared
//keybinding status need not be unique. -Z ( 1st April, 2019 )

void load_ukeys(int32_t* arr)
{
	arr[ukey_a] = Akey;
	arr[ukey_b] = Bkey;
	arr[ukey_s] = Skey;
	arr[ukey_l] = Lkey;
	arr[ukey_r] = Rkey;
	arr[ukey_p] = Pkey;
	arr[ukey_ex1] = Exkey1;
	arr[ukey_ex2] = Exkey2;
	arr[ukey_ex3] = Exkey3;
	arr[ukey_ex4] = Exkey4;
	arr[ukey_du] = DUkey;
	arr[ukey_dd] = DDkey;
	arr[ukey_dl] = DLkey;
	arr[ukey_dr] = DRkey;
	arr[ukey_mod1a] = cheat_modifier_keys[0];
	arr[ukey_mod1b] = cheat_modifier_keys[1];
	arr[ukey_mod2a] = cheat_modifier_keys[2];
	arr[ukey_mod2b] = cheat_modifier_keys[3];
};

static const char* ukey_names[] = {
	"A", "B", "Start", "L", "R", "Map",
	"Ex1", "Ex2", "Ex3", "Ex4", "Up", "Down",
	"Left", "Right", "Cheat Mod L1", "Cheat Mod L2",
	"Cheat Mod R1", "Cheat Mod R2",
};
std::string get_ukey_name(int32_t k)
{
	if (k < num_ukey) return ukey_names[k];
	return "";
}

int32_t onKeyboard()
{
	int32_t a = Akey;
	int32_t b = Bkey;
	int32_t s = Skey;
	int32_t l = Lkey;
	int32_t r = Rkey;
	int32_t p = Pkey;
	int32_t ex1 = Exkey1;
	int32_t ex2 = Exkey2;
	int32_t ex3 = Exkey3;
	int32_t ex4 = Exkey4;
	int32_t du = DUkey;
	int32_t dd = DDkey;
	int32_t dl = DLkey;
	int32_t dr = DRkey;
	int32_t mod1a = cheat_modifier_keys[0];
	int32_t mod1b = cheat_modifier_keys[1];
	int32_t mod2a = cheat_modifier_keys[2];
	int32_t mod2b = cheat_modifier_keys[3];
	bool done=false;
	int32_t ret;
	
	keyboard_control_dlg[0].dp2=get_zc_font(font_lfont);
	
	large_dialog(keyboard_control_dlg);
		
	while(!done)
	{
		ret = do_zqdialog(keyboard_control_dlg,3);
		
		if(ret==3) // OK
		{
			int32_t ukeys[num_ukey];
			load_ukeys(ukeys);
			std::vector<std::string> uniqueError;
			for(int32_t q = 0; q < num_ukey; ++q)
			{
				for(int32_t p = q+1; p < num_ukey; ++p)
				{
					if(ukeys[q] == ukeys[p] && ukeys[q] != 0)
					{
						char buf[64];
						sprintf(buf, "'%s' conflicts with '%s'", get_ukey_name(q).c_str(), get_ukey_name(p).c_str());
						std::string str(buf);
						uniqueError.push_back(str);
					}
				}
			}
			if(uniqueError.size() == 0)
			{
				done = true;
				save_control_configs(true);
			}
			else
			{
				box_start(1, "Duplicate Keys", get_zc_font(font_lfont), get_zc_font(font_sfont), false, keyboard_control_dlg[0].w,keyboard_control_dlg[0].h, 2);
				box_out("Cannot have duplicate keybinds!"); box_eol();
				for(std::vector<std::string>::iterator it = uniqueError.begin();
					it != uniqueError.end(); ++it)
				{
					box_out((*it).c_str()); box_eol();
				}
				box_end(true);
			}
		}
		else // Cancel
		{
			Akey = a;
			Bkey = b;
			Skey = s;
			Lkey = l;
			Rkey = r;
			Pkey = p;
			Exkey1 = ex1;
			Exkey2 = ex2;
			Exkey3 = ex3;
			Exkey4 = ex4;
			DUkey = du;
			DDkey = dd;
			DLkey = dl;
			DRkey = dr;
			cheat_modifier_keys[0] = mod1a;
			cheat_modifier_keys[1] = mod1b;
			cheat_modifier_keys[2] = mod2a;
			cheat_modifier_keys[3] = mod2b;

			done=true;
		}

        rest(1);
	}
	
	return D_O_K;
}

int32_t onGamepad()
{
	if (al_get_num_joysticks() == 0)
	{
		InfoDialog("ZC", "No gamepads detected.").show();
		return D_O_K;
	}

	int32_t a = Abtn;
	int32_t b = Bbtn;
	int32_t s = Sbtn;
	int32_t l = Lbtn;
	int32_t r = Rbtn;
	int32_t m = Mbtn;
	int32_t p = Pbtn;
	int32_t ex1 = Exbtn1;
	int32_t ex2 = Exbtn2;
	int32_t ex3 = Exbtn3;
	int32_t ex4 = Exbtn4;
	int32_t up = DUbtn;
	int32_t down = DDbtn;
	int32_t left = DLbtn;
	int32_t right = DRbtn;
	int32_t joy = joystick_index;
	int32_t stick_1 = js_stick_1_x_stick;
	int32_t stick_2 = js_stick_2_x_stick;

	gamepad_dlg[0].dp2=get_zc_font(font_lfont);
	if(analog_movement)
		gamepad_dlg[56].flags|=D_SELECTED;
	else
		gamepad_dlg[56].flags&=~D_SELECTED;

	// TODO: should use controller device GUID or name instead of index, otherwise this value is not
	// consistent unless exact same number of joysticks is always connected. Name is problematic b/c
	// xinput driver doesn't actually get a name (at least, doesn't for my Xbox controller).
	// TODO: should store gamepad control mappings per-controller, otherwise switching joystick
	// requires remapping every time.
	if (joystick_index >= al_get_num_joysticks())
		joystick_index = 0;
	gamepad_dlg[61].d2 = joystick_index;

	gamepad_dlg_cur_joystick = al_get_joystick(joystick_index);
	if (!gamepad_dlg_cur_joystick)
	{
		InfoDialog("ZC", "Invalid gamepad. Did it disconnect?").show();
		return D_CLOSE;
	}

	large_dialog(gamepad_dlg);
		
	int32_t ret = do_zqdialog(gamepad_dlg,4);
	
	if(ret == 4) //OK
	{
		analog_movement = gamepad_dlg[56].flags&D_SELECTED;
		joystick_index = gamepad_dlg[61].d2;
		gamepad_dlg_cur_joystick = al_get_joystick(joystick_index);
		if (!gamepad_dlg_cur_joystick)
		{
			InfoDialog("ZC", "Invalid gamepad. Did it disconnect?").show();
			return D_CLOSE;
		}
		js_stick_1_y_stick = js_stick_1_x_stick;
		js_stick_2_y_stick = js_stick_2_x_stick;
		save_control_configs(false);
	}
	else //Cancel
	{
		Abtn = a;
		Bbtn = b;
		Sbtn = s;
		Lbtn = l;
		Rbtn = r;
		Mbtn = m;
		Pbtn = p;
		Exbtn1 = ex1;
		Exbtn2 = ex2;
		Exbtn3 = ex3;
		Exbtn4 = ex4;
		DUbtn = up;
		DDbtn = down;
		DLbtn = left;
		DRbtn = right;
		joystick_index = joy;
		js_stick_1_x_stick = stick_1;
		js_stick_2_x_stick = stick_2;
	}

	return D_O_K;
}

int32_t onCheatKeys()
{
	int32_t oldcheats[Cheat::Last][2];
	memcpy(oldcheats, cheatkeys, sizeof(cheatkeys));
	
	bool done=false;
	
	while(!done)
	{
		bool confirm = false;
		CheatKeysDialog(&confirm).show();
		if(confirm) // OK
		{
			std::vector<std::string> uniqueError;
			char buf[512];
			for(size_t q = 1; q < Cheat::Last; ++q)
			{
				if(cheatkeys[q][1] && !cheatkeys[q][0])
				{
					cheatkeys[q][0] = cheatkeys[q][1];
					cheatkeys[q][1] = 0;
				}
			}
			for(size_t q = 1; q < Cheat::Last; ++q)
			{
				if(!bindable_cheat((Cheat)q)) continue;
				for(size_t p = q+1; p < Cheat::Last; ++p)
				{
					if(!bindable_cheat((Cheat)p)) continue;
					for(size_t q2 = 0; q2 <= 1; ++q2)
						for(size_t p2 = 0; p2 <= 1; ++p2)
						{
							if(cheatkeys[q][q2] == cheatkeys[p][p2] && cheatkeys[q][q2] != 0)
							{
								uniqueError.push_back(fmt::format("'{}' ({}) conflicts with '{}' ({}) - both '{}'",
									cheat_to_string((Cheat)q), q2?"Alt":"Main",
									cheat_to_string((Cheat)p), p2?"Alt":"Main",
									get_keystr(cheatkeys[q][q2])));
							}
						}
				}
			}
			if(uniqueError.size() == 0)
			{
				done = true;
				save_cheatkeys();
			}
			else
			{
				box_start(1, "Duplicate Keys", get_zc_font(font_lfont), get_zc_font(font_sfont), false, 500,400, 2);
				box_out("Cannot have duplicate keybinds!"); box_eol();
				for(std::vector<std::string>::iterator it = uniqueError.begin();
					it != uniqueError.end(); ++it)
				{
					box_out((*it).c_str()); box_eol();
				}
				box_end(true);
			}
		}
		else // Cancel
		{
			memcpy(cheatkeys, oldcheats, sizeof(cheatkeys));
			done=true;
		}
        rest(1);
	}
	
	return D_O_K;
}

int32_t onSound()
{
	if (get_qr(qr_OLD_SCRIPT_VOLUME))
	{
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

	int32_t m = midi_volume;
	int32_t e = emusic_volume;
	int32_t s = sfx_volume;
	int32_t p = pan_style;
	pan_style = vbound(pan_style,0,3);
	
	sound_dlg[0].dp2=get_zc_font(font_lfont);
	
	large_dialog(sound_dlg);
		
	midi_dp[1] = sound_dlg[6].x;
	midi_dp[2] = sound_dlg[6].y;
	emus_dp[1] = sound_dlg[8].x;
	emus_dp[2] = sound_dlg[8].y;
	sfx_dp[1]  = sound_dlg[10].x;
	sfx_dp[2]  = sound_dlg[10].y;
	pan_dp[1]  = sound_dlg[11].x;
	pan_dp[2]  = sound_dlg[11].y;
	sound_dlg[15].d2 = (midi_volume==255) ? 32 : midi_volume>>3;
	sound_dlg[17].d2 = (emusic_volume==255) ? 32 : emusic_volume>>3;
	sound_dlg[19].d2 = (sfx_volume==255) ? 32 : sfx_volume>>3;
	sound_dlg[20].d2 = pan_style;
	
	int32_t ret = do_zqdialog(sound_dlg,1);
	
	if(ret==2)
	{
		master_volume(digi_volume,midi_volume);
		if (zcmusic)
			zcmusic_set_volume(zcmusic, emusic_volume);
		
		int32_t temp_volume = sfx_volume;
		if (GameLoaded && !get_qr(qr_OLD_SCRIPT_VOLUME))
			temp_volume = (sfx_volume * FFCore.usr_sfx_volume) / 10000 / 100;
		for(int32_t i=0; i<WAV_COUNT; ++i)
		{
			if(sfx_voice[i] >= 0)
				voice_set_volume(sfx_voice[i], temp_volume);
		}
		zc_set_config(sfx_sect,"midi",midi_volume);
		zc_set_config(sfx_sect,"sfx",sfx_volume);
		zc_set_config(sfx_sect,"emusic",emusic_volume);
		zc_set_config(sfx_sect,"pan",pan_style);
	}
	else
	{
		midi_volume   = m;
		emusic_volume = e;
		sfx_volume	= s;
		pan_style	 = p;
	}
	
	return D_O_K;
}

int32_t queding(char const* s1, char const* s2, char const* s3)
{
	return jwin_alert("ZQuest Classic",s1,s2,s3,"&Yes","&No",'y','n',get_zc_font(font_lfont));
}

int32_t onQuit()
{
	if(Playing)
	{
		int32_t ret=0;
		
		if(get_qr(qr_NOCONTINUE))
		{
			if(standalone_mode)
			{
				ret=queding("End current game?",
							"The continue screen is disabled; the game",
							"will be reloaded from the last save.");
			}
			else
			{
				ret=queding("End current game?",
							"The continue screen is disabled. You will",
							"be returned to the file select screen.");
			}
		}
		else
			ret=queding("End current game?",NULL,NULL);
			
		if(ret==1)
		{
			disableClickToFreeze=false;
			Quit=qQUIT;
			
			// Trying to evade a door repair charge?
			if(repaircharge)
			{
				game->change_drupy(-repaircharge);
				repaircharge=0;
			}
			
			return D_CLOSE;
		}
	}
	
	return D_O_K;
}

int32_t onTryQuitMenu()
{
	return onTryQuit(true);
}

int32_t onTryQuit(bool inMenu)
{
	if(Playing && !(GameFlags & GAMEFLAG_NO_F6))
	{
		if(active_cutscene.can_f6())
		{
			if(get_qr(qr_OLD_F6))
			{
				if(inMenu) onQuit();
				else /*if(!get_qr(qr_NOCONTINUE))*/ f_Quit(qQUIT);
			}
			else
			{
				disableClickToFreeze=false;
				GameFlags |= GAMEFLAG_TRYQUIT;
			}
			return D_CLOSE;
		}
		else active_cutscene.error();
	}
	
	return D_O_K;
}

int32_t onReset()
{
	if(queding("  Reset system?  ",NULL,NULL)==1)
	{
		disableClickToFreeze=false;
		Quit=qRESET;
		replay_quit();
		return D_CLOSE;
	}
	
	return D_O_K;
}

int32_t onExit()
{
	if(queding(" Quit ZQuest Classic? ",NULL,NULL)==1)
	{
		Quit=qEXIT;
		return D_CLOSE;
	}
	
	return D_O_K;
}

int32_t onDebug()
{
	if(debug_enabled)
		set_debug(!get_debug());
	return D_O_K;
}

int32_t onHeartBeep()
{
	heart_beep=!heart_beep;
	zc_set_config(cfg_sect,"heart_beep",heart_beep);
	return D_O_K;
}

int32_t onSaveIndicator()
{
	use_save_indicator = use_save_indicator ? 0 : 1;
	zc_set_config(cfg_sect,"save_indicator",use_save_indicator);
	return D_O_K;
}

int32_t onEpilepsy()
{
	if(jwin_alert3(
			"Epilepsy Flash Reduction", 
			"Enabling this will reduce the intensity of flashing and screen wave effects.",
			"Disabling this will restore standard flash and wavy behaviour.",
			"Proceed?",
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)
	{
		epilepsyFlashReduction = epilepsyFlashReduction ? 0 : 1;
		zc_set_config(cfg_sect,"epilepsy_flash_reduction",epilepsyFlashReduction);
	}
	return D_O_K;
}

bool rc = false;

static DIALOG getnum_dlg[] =
{
	// (dialog proc)	   (x)   (y)	(w)	 (h)   (fg)	 (bg)	(key)	(flags)	 (d1)		   (d2)	 (dp)
	{ jwin_win_proc,		80,   80,	 160,	72,   vc(0),  vc(11),  0,	   D_EXIT,	 0,			 0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,		  104,  104+4,  48,	 8,	vc(0),  vc(11),  0,	   0,		  0,			 0, (void *) "Number:", NULL,  NULL },
	{ jwin_edit_proc,	   168,  104,	48,	 16,	0,	 0,	   0,	   0,		  6,			 0,	   NULL, NULL,  NULL },
	{ jwin_button_proc,	 90,   126,	61,	 21,   vc(0),  vc(11),  13,	  D_EXIT,	 0,			 0, (void *) "OK", NULL,  NULL },
	{ jwin_button_proc,	 170,  126,	61,	 21,   vc(0),  vc(11),  27,	  D_EXIT,	 0,			 0, (void *) "Cancel", NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

int32_t getnumber(const char *prompt,int32_t initialval)
{
	char buf[20];
	sprintf(buf,"%d",initialval);
	getnum_dlg[0].dp=(void *)prompt;
	getnum_dlg[0].dp2=get_zc_font(font_lfont);
	getnum_dlg[2].dp=buf;
	
	large_dialog(getnum_dlg);
		
	if(do_zqdialog(getnum_dlg,2)==3)
		return atoi(buf);
		
	return initialval;
}

int32_t onLife()
{
	int value = vbound(getnumber("Life",game->get_life()),1,game->get_maxlife());
	cheats_enqueue(Cheat::Life, value);
	return D_O_K;
}

int32_t onHeartC()
{
	int max_life = vbound(getnumber("Heart Containers",game->get_maxlife()/game->get_hp_per_heart()),1,4095) * game->get_hp_per_heart();
	int life = vbound(getnumber("Life",game->get_life()/game->get_hp_per_heart()),1,max_life/game->get_hp_per_heart())*game->get_hp_per_heart();
	cheats_enqueue(Cheat::MaxLife, max_life);
	cheats_enqueue(Cheat::Life, life);
	return D_O_K;
}

int32_t onMagicC()
{
	int max_magic = vbound(getnumber("Magic Containers",game->get_maxmagic()/game->get_mp_per_block()),0,2047) * game->get_mp_per_block();
	int magic = vbound(getnumber("Magic",game->get_magic()/game->get_mp_per_block()),0,max_magic/game->get_mp_per_block())*game->get_mp_per_block();
	cheats_enqueue(Cheat::MaxMagic, max_magic);
	cheats_enqueue(Cheat::Magic, magic);
	return D_O_K;
}

int32_t onRupies()
{
	int value = vbound(getnumber("Rupees",game->get_rupies()),0,game->get_maxcounter(1));
	cheats_enqueue(Cheat::Rupies, value);
	return D_O_K;
}

int32_t onMaxBombs()
{
	int value = vbound(getnumber("Max Bombs",game->get_maxbombs()),0,0xFFFF);
	cheats_enqueue(Cheat::MaxBombs, value);
	cheats_enqueue(Cheat::Bombs, value);
	return D_O_K;
}

int32_t onRefillLife()
{
	cheats_enqueue(Cheat::Life, game->get_maxlife());
	return D_O_K;
}
int32_t onRefillMagic()
{
	cheats_enqueue(Cheat::Magic, game->get_maxmagic());
	return D_O_K;
}
int32_t onClock()
{
	cheats_enqueue(Cheat::Clock);
	return D_O_K;
}

int32_t onQstPath()
{
	char initial_path[2048];
	chop_path(qstdir);
	strcpy(initial_path, qstdir);
	
	if (auto result = prompt_for_existing_folder("Quest File Directory", initial_path, "qst"))
	{
		char* path = result->data();
		chop_path(path);
		fix_filename_case(path);
		fix_filename_slashes(path);
		strcpy(qstdir,path);
		strcpy(qstpath,qstdir);
		zc_set_config("zeldadx","quest_dir",qstdir);
		flush_config_file();
	}

	return D_O_K;
}

#include "dialog/cheat_dialog.h"
int32_t onCheat()
{
	call_setcheat_dialog();
	game->set_cheat(maxcheat);
	if(cheat) game->did_cheat(true);
	return D_O_K;
}

int32_t onCheatRupies()
{
	cheats_enqueue(Cheat::Rupies, game->get_maxcounter(1));
	return D_O_K;
}

int32_t onCheatArrows()
{
	cheats_enqueue(Cheat::Arrows, game->get_maxarrows());
	return D_O_K;
}

int32_t onCheatBombs()
{
	cheats_enqueue(Cheat::Bombs, game->get_maxbombs(), game->get_maxcounter(6));
	return D_O_K;
}

// *** screen saver

int32_t after_time()
{
	if(ss_enable == 0)
		return INT_MAX;

	if(ss_after <= 0)
		return 5 * 60;
		
	if(ss_after <= 3)
		return ss_after * 15 * 60;
		
	if(ss_after <= 13)
		return (ss_after - 3) * 60 * 60;
		
	return MAX_IDLE + 1;
}

static const char *after_str[15] =
{
	" 5 sec", "15 sec", "30 sec", "45 sec", " 1 min", " 2 min", " 3 min",
	" 4 min", " 5 min", " 6 min", " 7 min", " 8 min", " 9 min", "10 min",
	"Never"
};

const char *after_list(int32_t index, int32_t *list_size)
{
	if(index < 0)
	{
		*list_size = 15;
		return NULL;
	}
	
	return after_str[index];
}

static ListData after__list(after_list, &font);

static DIALOG scrsaver_dlg[] =
{
	/* (dialog proc)	   (x)   (y)   (w)   (h)   (fg)	 (bg)	 (key)	(flags)	(d1)	  (d2)	 (dp)  (dp2)	(dp3) */
	{ jwin_win_proc,	   32,   64,   256,  136,  0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Screen Saver Settings", NULL,  NULL },
	{ jwin_frame_proc,	 42,   92,   236,  70,   0,	   0,	   0,	   0,		 FR_ETCHED,0,	   NULL, NULL,  NULL },
	{ jwin_text_proc,		 60,   104,  48,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Run After", NULL,  NULL },
	{ jwin_text_proc,		 60,   128,  48,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Speed", NULL,  NULL },
	{ jwin_text_proc,		 60,   144,  48,   8,	vc(0),   vc(11),  0,	   0,		 0,		0, (void *) "Density", NULL,  NULL },
	{ jwin_droplist_proc,  144,  100,  96,   16,   0,	   0,	   0,	   0,		 0,		0, (void *) &after__list, NULL,  NULL },
	{ jwin_slider_proc,	144,  128,  116,  8,	vc(0),   jwin_pal[jcBOX],  0,	   0,		 6,		0,	   NULL, NULL,  NULL },
	{ jwin_slider_proc,	144,  144,  116,  8,	vc(0),   jwin_pal[jcBOX],  0,	   0,		 6,		0,	   NULL, NULL,  NULL },
	{ jwin_button_proc,	42,   170,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "OK", NULL,  NULL },
	{ jwin_button_proc,	124,  170,  72,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Preview", NULL,  NULL },
	{ jwin_button_proc,	218,  170,  61,   21,   0,	   0,	   0,	   D_EXIT,	0,		0, (void *) "Cancel", NULL,  NULL },
	{ d_timer_proc,		 0,	0,	 0,	0,	0,	   0,	   0,	   0,		  0,		  0,		 NULL, NULL, NULL },
	{ NULL,				 0,	0,	0,	0,   0,	   0,	   0,	   0,		  0,			 0,	   NULL,						   NULL,  NULL }
};

int32_t onScreenSaver()
{
	scrsaver_dlg[0].dp2=get_zc_font(font_lfont);
	int32_t oldcfgs[3];
	scrsaver_dlg[5].d1 = scrsaver_dlg[5].d2 = oldcfgs[0] = ss_after;
	scrsaver_dlg[6].d2 = oldcfgs[1] = ss_speed;
	scrsaver_dlg[7].d2 = oldcfgs[2] = ss_density;
	
	large_dialog(scrsaver_dlg);
		
	int32_t ret = do_zqdialog(scrsaver_dlg,-1);
	
	if(ret == 8 || ret == 9)
	{
		ss_after   = scrsaver_dlg[5].d1;
		ss_speed   = scrsaver_dlg[6].d2;
		ss_density = scrsaver_dlg[7].d2;
		if(oldcfgs[0] != ss_after)
			zc_set_config(cfg_sect,"ss_after",ss_after);
		if(oldcfgs[1] != ss_speed)
			zc_set_config(cfg_sect,"ss_speed",ss_speed);
		if(oldcfgs[2] != ss_density)
			zc_set_config(cfg_sect,"ss_density",ss_density);
	}
	
	if(ret == 9)
		// preview Screen Saver
	{
		clear_keybuf();
		Matrix(ss_speed, ss_density, 30);
		system_pal(true);
		sys_mouse();
	}
	
	return D_O_K;
}

/*****  Menus  *****/

enum
{
	MENUID_GAME_LOADQUEST,
	MENUID_GAME_ENDGAME,
};
static NewMenu game_menu
{
	{ "&Continue","ESC", onContinue },
	{},
	{ "L&oad Quest...", onCustomGame, MENUID_GAME_LOADQUEST },
	{ "&End Game","F6", onTryQuitMenu, MENUID_GAME_ENDGAME },
	{},
#ifdef __EMSCRIPTEN__
	{ "&Reset","F7", onReset },
#elif defined(ALLEGRO_MACOSX)
	{ "&Reset","F7", onReset },
	{ "&Quit","F8", onExit },
#else
	{ "&Reset","F9", onReset },
	{ "&Quit","F10", onExit },
#endif
};

static NewMenu snapshot_format_menu
{
	{ "&BMP", std::bind(onSetSnapshotFormat, ssfmtBMP) },
	{ "&GIF", std::bind(onSetSnapshotFormat, ssfmtGIF) },
	{ "&JPG", std::bind(onSetSnapshotFormat, ssfmtJPG) },
	{ "&PNG", std::bind(onSetSnapshotFormat, ssfmtPNG) },
	{ "PC&X", std::bind(onSetSnapshotFormat, ssfmtPCX) },
	{ "&TGA", std::bind(onSetSnapshotFormat, ssfmtTGA) },
};

static NewMenu controls_menu
{
	{ "Key&board...", onKeyboard },
	{ "&Gamepad...", onGamepad },
	{ "&Cheat Keys...", onCheatKeys },
};

static NewMenu name_entry_mode_menu
{
	{ "&Keyboard", onKeyboardEntry },
	{ "&Letter Grid", onLetterGridEntry },
	{ "&Extended Letter Grid", onExtLetterGridEntry },
};

static void set_controls_menu_active()
{
	
}

enum
{
	MENUID_WINDOW_LOCK_ASPECT,
	MENUID_WINDOW_LOCK_INTSCALE,
	MENUID_WINDOW_SAVE_SIZE,
	MENUID_WINDOW_SAVE_POS,
	MENUID_WINDOW_STRETCH,
};
static NewMenu window_menu
{
	{ "Lock Aspect Ratio", onDragAspect, MENUID_WINDOW_LOCK_ASPECT },
	{ "Lock Integer Scale", onIntegerScaling, MENUID_WINDOW_LOCK_INTSCALE },
	{ "Save Size Changes", onSaveDragResize, MENUID_WINDOW_SAVE_SIZE },
	{ "Save Position Changes", onWinPosSave, MENUID_WINDOW_SAVE_POS },
	{ "Stretch Game Area", onStretchGame, MENUID_WINDOW_STRETCH },
};
void call_zc_options_dlg();
enum
{
	MENUID_OPTIONS_PAUSE_BG,
	MENUID_OPTIONS_EPILEPSYPROT,
};
static NewMenu options_menu
{
	{ "Name &Entry Mode", &name_entry_mode_menu },
	{ "S&napshot Format", &snapshot_format_menu },
	{ "&Window Settings", &window_menu },
	{ "Epilepsy Flash Reduction", onEpilepsy, MENUID_OPTIONS_EPILEPSYPROT },
	{ "Pause In Background", onPauseInBackground, MENUID_OPTIONS_PAUSE_BG },
	{ "More Options", call_zc_options_dlg },
};
enum
{
	MENUID_SETTINGS_CONTROLS,
	MENUID_SETTINGS_CAPFPS,
	MENUID_SETTINGS_SHOWFPS,
	MENUID_SETTINGS_SHOWTIME,
	MENUID_SETTINGS_CLICK_FREEZE,
	MENUID_SETTINGS_TRANSLAYERS,
	MENUID_SETTINGS_NESQUIT,
	MENUID_SETTINGS_VOLKEYS,
	MENUID_SETTINGS_HEARTBEEP,
	MENUID_SETTINGS_SAVEINDICATOR,
	MENUID_SETTINGS_DEBUG,
};
static NewMenu settings_menu
{
	{ "&Sound...", onSound },
	{ "C&ontrols", &controls_menu, MENUID_SETTINGS_CONTROLS },
	{},
	{ "Options", &options_menu },
	{},
	{ "&Cap FPS","F1", onThrottleFPS, MENUID_SETTINGS_CAPFPS },
	{ "Show &FPS","F2", onShowFPS, MENUID_SETTINGS_SHOWFPS },
	{ "Show &Time", onShowTime, MENUID_SETTINGS_SHOWTIME },
	{ "Click to Freeze", onClickToFreeze, MENUID_SETTINGS_CLICK_FREEZE },
	{ "Cont. &Heart Beep", onHeartBeep, MENUID_SETTINGS_HEARTBEEP },
	{ "Show Trans. &Layers", onTransLayers, MENUID_SETTINGS_TRANSLAYERS },
	{ "Up+A+B To &Quit", onNESquit, MENUID_SETTINGS_NESQUIT },
	{ "Volume &Keys", onVolKeys, MENUID_SETTINGS_VOLKEYS },
	{ "Sa&ve Indicator", onSaveIndicator, MENUID_SETTINGS_SAVEINDICATOR },
	{},
	{ "Debu&g", onDebug, MENUID_SETTINGS_DEBUG },
};

enum
{
	MENUID_MISC_FULLSCREEN,
	MENUID_MISC_VIDMODE,
	MENUID_MISC_QUEST_INFO,
	MENUID_MISC_QUEST_DIR,
	MENUID_MISC_CONSOLE,
	MENUID_MISC_CLEAR_CONSOLE_ON_LOAD,
};
static NewMenu misc_menu
{
	{ "&About...", onAbout },
	// TODO: re-enable, but: 1) do not use a bitmap thing that is hard to update 2) update names and 3) don't use the Z-word.
	// { "&Credits...", onCredits },
	{ "&Fullscreen", onFullscreenMenu, MENUID_MISC_FULLSCREEN },
	{ "&Video Mode...", onVidMode, MENUID_MISC_VIDMODE },
	{},
	{ "&Quest Info...", onQuest, MENUID_MISC_QUEST_INFO },
	{ "Quest &MIDI Info...", onMIDICredits },
	{ "Quest &Directory...", onQstPath, MENUID_MISC_QUEST_DIR },
	{},
	{ "Take &Snapshot F12", onSnapshot },
	{ "Sc&reen Saver...", onScreenSaver },
	{ "Save ZC Configuration", OnSaveZCConfig },
	{ "Show Console", onConsole, MENUID_MISC_CONSOLE },
	{ "Clear Console on Qst Load", onClrConsoleOnLoad, MENUID_MISC_CLEAR_CONSOLE_ON_LOAD },
	{ "Clear Directory Cache", OnnClearQuestDir },
};

enum
{
	MENUID_REFILL_ARROWS,
};
static NewMenu refill_menu
{
	{ "&Life", onRefillLife },
	{ "&Magic", onRefillMagic },
	{ "&Bombs", onCheatBombs },
	{ "&Rupees", onCheatRupies },
	{ "&Arrows", onCheatArrows, MENUID_REFILL_ARROWS },
};

enum
{
	MENUID_SHOW_L0,
	MENUID_SHOW_L1,
	MENUID_SHOW_L2,
	MENUID_SHOW_L3,
	MENUID_SHOW_L4,
	MENUID_SHOW_L5,
	MENUID_SHOW_L6,
	MENUID_SHOW_OVER,
	MENUID_SHOW_PUSH,
	MENUID_SHOW_FFC,
	MENUID_SHOW_SPR,
	MENUID_SHOW_SCRIPTNAME,
	MENUID_SHOW_SOLIDITY,
	MENUID_SHOW_HITBOX,
	MENUID_SHOW_EFFECT,
};
static NewMenu show_menu
{
	{ "Combos", onShowLayer0, MENUID_SHOW_L0 },
	{ "Layer 1", onShowLayer1, MENUID_SHOW_L1 },
	{ "Layer 2", onShowLayer2, MENUID_SHOW_L2 },
	{ "Layer 3", onShowLayer3, MENUID_SHOW_L3 },
	{ "Layer 4", onShowLayer4, MENUID_SHOW_L4 },
	{ "Layer 5", onShowLayer5, MENUID_SHOW_L5 },
	{ "Layer 6", onShowLayer6, MENUID_SHOW_L6 },
	{ "Overhead Combos", onShowLayerO, MENUID_SHOW_OVER },
	{ "Push Blocks", onShowLayerP, MENUID_SHOW_PUSH },
	{ "Freeform Combos", onShowLayerF, MENUID_SHOW_FFC },
	{ "Sprites", onShowLayerS, MENUID_SHOW_SPR },
	{},
	{ "Current FFC Scripts", onShowFFScripts, MENUID_SHOW_SCRIPTNAME },
	{},
	{ "Walkability", onShowLayerW, MENUID_SHOW_SOLIDITY },
	{ "Hitboxes", onShowHitboxes, MENUID_SHOW_HITBOX },
	{ "Effects", onShowLayerE, MENUID_SHOW_EFFECT },
	{ "Info Opacity", onShowInfoOpacity },
};

enum
{
	MENUID_CHEAT_CHOP_L1,
	MENUID_CHEAT_CHOP_L2,
	MENUID_CHEAT_CHOP_L3,
	MENUID_CHEAT_CHOP_L4,
	MENUID_CHEAT_INVULN,
	MENUID_CHEAT_NOCLIP,
	MENUID_CHEAT_IGNORESV,
	MENUID_CHEAT_GOFAST,
};
static NewMenu cheat_menu
{
	{ "Set &Cheat", onCheat },
	{ MENUID_CHEAT_CHOP_L1 },
	{ "Re&fill", &refill_menu },
	{ MENUID_CHEAT_CHOP_L2 },
	{ "&Invincible", onClock, MENUID_CHEAT_INVULN },
	{ "Ma&x Bombs...", onMaxBombs },
	{ "&Heart Containers...", onHeartC },
	{ "&Magic Containers...", onMagicC },
	{ MENUID_CHEAT_CHOP_L3 },
	{ "&Hero Data...", onCheatConsole },
	{ MENUID_CHEAT_CHOP_L4 },
	{ "Walk Through &Walls", onNoWalls, MENUID_CHEAT_NOCLIP },
	{ "Hero Ignores Side&view", onIgnoreSideview, MENUID_CHEAT_IGNORESV },
	{ "&Quick Movement", onGoFast, MENUID_CHEAT_GOFAST },
	{ "&Kill All Enemies", onKillCheat },
	{ "Trigger &Secrets", onSecretsCheat },
	{ "Trigger Secrets Perm", onSecretsCheatPerm },
	{ "Show/Hide Layer", &show_menu },
	{ "Toggle &Light", onLightSwitch },
	{ "&Goto Location...", onGoTo },
};

#if DEVLEVEL > 0
int32_t devLogging();
int32_t devDebug();
int32_t devTimestmp();
#if DEVLEVEL > 1
int32_t setCheat();
#endif //DEVLEVEL > 1
enum
{
	MENUID_DEV_LOGGING,
	MENUID_DEV_DEBUG,
	MENUID_DEV_TIMESTAMP,
	MENUID_DEV_SETCHEAT,
};
static NewMenu dev_menu
{
	{ "&Force Error Log", devLogging, MENUID_DEV_LOGGING },
	// { "&Extra Debug Log", devDebug, MENUID_DEV_DEBUG },
	{ "&Timestamp Log", devTimestmp, MENUID_DEV_TIMESTAMP },
	#if DEVLEVEL > 1
	{},
	{ "Set &Cheat", setCheat, MENUID_DEV_SETCHEAT },
	#endif //DEVLEVEL > 1
};
int32_t devLogging()
{
	dev_logging = !dev_logging;
	dev_menu.select_uid(MENUID_DEV_LOGGING, dev_logging);
	return D_O_K;
}
// int32_t devDebug()
// {
	// dev_debug = !dev_debug;
	// dev_menu.select_uid(MENUID_DEV_DEBUG, dev_debug);
	// dev_menu[dv_dbg].flags = dev_debug ? D_SELECTED : 0;
	// return D_O_K;
// }
int32_t devTimestmp()
{
	dev_timestmp = !dev_timestmp;
	dev_menu.select_uid(MENUID_DEV_TIMESTAMP, dev_timestmp);
	return D_O_K;
}
#if DEVLEVEL > 1
int32_t setCheat()
{
	cheat = (vbound(getnumber("Cheat Level",cheat), 0, 4));
	return D_O_K;
}
#endif //DEVLEVEL > 1
#endif //DEVLEVEL > 0

enum
{
	MENUID_PLAYER_CHEAT,
};
TopMenu the_player_menu
{
	{ "&Game", &game_menu },
	{ "&Settings", &settings_menu },
	{ "&Cheat", &cheat_menu, MENUID_PLAYER_CHEAT, true },
	{ "&Replay", &replay_menu },
	{ "&ZC", &misc_menu },
	#if DEVLEVEL > 0
	{ "&Dev", &dev_menu },
	#endif
};

int32_t onPauseInBackground()
{
	if(jwin_alert3(
			"Toggle Pause In Background", 
			"This action will change whether ZC Player pauses when the window loses focus.",
			"",
			"Proceed?",
		 "&Yes", 
		"&No", 
		NULL, 
		'y', 
		'n', 
		0, 
		get_zc_font(font_lfont)) == 1)
	{
		pause_in_background = pause_in_background ? 0 : 1;
		zc_set_config("zeldadx","pause_in_background", pause_in_background);
		int switch_type = pause_in_background ? SWITCH_PAUSE : SWITCH_BACKGROUND;
		set_display_switch_mode(fullscreen?SWITCH_BACKAMNESIA:switch_type);
		set_display_switch_callback(SWITCH_OUT, switch_out_callback);
		set_display_switch_callback(SWITCH_IN, switch_in_callback);
	}
	options_menu.select_uid(MENUID_OPTIONS_PAUSE_BG, pause_in_background);
	return D_O_K;
}

int32_t onKeyboardEntry()
{
	NameEntryMode=0;
	zc_set_config(cfg_sect,"name_entry_mode",NameEntryMode);
	return D_O_K;
}

int32_t onLetterGridEntry()
{
	NameEntryMode=1;
	zc_set_config(cfg_sect,"name_entry_mode",NameEntryMode);
	return D_O_K;
}

int32_t onExtLetterGridEntry()
{
	NameEntryMode=2;
	zc_set_config(cfg_sect,"name_entry_mode",NameEntryMode);
	return D_O_K;
}

static BITMAP* oldscreen;
int32_t onFullscreenMenu()
{
	PALETTE oldpal;
	get_palette(oldpal);

	fullscreen = !fullscreen;
	all_toggle_fullscreen(fullscreen);
	zc_set_config("zeldadx","fullscreen",fullscreen);

	zc_set_palette(oldpal);
	gui_mouse_focus=0;
	extern int32_t switch_type;
	switch_type = pause_in_background ? SWITCH_PAUSE : SWITCH_BACKGROUND;
	set_display_switch_mode(fullscreen?SWITCH_BACKAMNESIA:switch_type);
	set_display_switch_callback(SWITCH_OUT, switch_out_callback);
	set_display_switch_callback(SWITCH_IN, switch_in_callback);
	misc_menu.select_uid(MENUID_MISC_FULLSCREEN, isFullScreen());
	misc_menu.select_uid(MENUID_MISC_VIDMODE, isFullScreen());

	return D_O_K;
}

void fix_menu()
{
	if(!debug_enabled)
		settings_menu.chop_index = 13;
}

int32_t onSetSnapshotFormat(SnapshotType format)
{
	SnapshotFormat = format;
	zc_set_config("zeldadx", "snapshot_format", format);
	snapshot_format_menu.select_only_index(format);
	return D_O_K;
}


void color_layer(RGB *src,RGB *dest,char r,char g,char b,char pos,int32_t from,int32_t to)
{
	PALETTE tmp;
	
	for(int32_t i=0; i<256; i++)
	{
		tmp[i].r=r;
		tmp[i].g=g;
		tmp[i].b=b;
	}
	
	fade_interpolate(src,tmp,dest,pos,from,to);
}

void system_pal(bool force)
{
	if(is_sys_pal && !force) return;
	is_sys_pal = true;
	load_colorset(gui_colorset, syspal, jwin_a5_colors);
	hw_palette = &syspal;
	update_hw_pal = true;
}

static uint32_t entered_sys_pal = 0;
void enter_sys_pal()
{
	if(is_sys_pal)
	{
		if(entered_sys_pal)
			++entered_sys_pal;
		return;
	}
	sys_mouse();
	system_pal(true);
	++entered_sys_pal;
}
void exit_sys_pal()
{
	if(entered_sys_pal)
	{
		if(!--entered_sys_pal)
		{
			game_pal();
			game_mouse();
		}
	}
}

void switch_out_callback()
{
	if (pause_in_background && !MenuOpen)
	{
		System();
	}
}

void switch_in_callback()
{
}

void game_pal()
{
	is_sys_pal = false;
	entered_sys_pal = 0;
	hw_palette = &RAMpal;
	update_hw_pal = true;
}

static char bar_str[] = "";

void music_pause()
{
	//al_pause_duh(tmplayer);
	zcmusic_pause(zcmusic, ZCM_PAUSE);
	if(zcmixer->oldtrack)
		zcmusic_pause(zcmixer->oldtrack, ZCM_PAUSE);
	zc_midi_pause();
}

void music_resume()
{
	//al_resume_duh(tmplayer);
	zcmusic_pause(zcmusic, ZCM_RESUME);
	if (zcmixer->oldtrack)
		zcmusic_pause(zcmixer->oldtrack, ZCM_RESUME);
	zc_midi_resume();
}

void music_stop()
{
	//al_stop_duh(tmplayer);
	//unload_duh(tmusic);
	//tmusic=NULL;
	//tmplayer=NULL;
	zcmusic_stop(zcmusic);
	zcmusic_unload_file(zcmusic);
	if (zcmixer->oldtrack)
	{
		zcmusic_stop(zcmixer->oldtrack);
		zcmusic_unload_file(zcmixer->oldtrack);
	}
	zcmixer->newtrack = NULL;
	zc_stop_midi();
	currmidi=-1;
}

bool reload_fonts = false;
void System()
{
	mouse_down = gui_mouse_b();
	music_pause();
	pause_all_sfx();
	MenuOpen = true;
	enter_sys_pal();
	//  FONT *oldfont=font;
	//  font=tfont;
	
	misc_menu.select_uid(MENUID_MISC_FULLSCREEN, isFullScreen());
	misc_menu.disable_uid(MENUID_MISC_VIDMODE, isFullScreen());
	
	#if DEVLEVEL > 1
	dev_menu.disable_uid(MENUID_DEV_SETCHEAT, !Playing);
	#endif
	game_menu.disable_uid(MENUID_GAME_LOADQUEST, get_unset_save_slot());
	game_menu.disable_uid(MENUID_GAME_ENDGAME, !Playing);
	misc_menu.disable_uid(MENUID_MISC_QUEST_INFO, !Playing);
	misc_menu.disable_uid(MENUID_MISC_QUEST_DIR, Playing);
	clear_keybuf();

	clear_bitmap(menu_bmp);
	oldscreen = screen;
	screen = menu_bmp;
	
	the_player_menu.reset_state();
	the_player_menu.position(0, 0);
	
	bool running = true;
	bool esc = key[KEY_ESC] || cMbtn();
	bool autopop = esc;
	do
	{
		if(reload_fonts)
		{
			init_custom_fonts();
			clear_bitmap(menu_bmp);
			broadcast_dialog_message(MSG_DRAW, 0);
			reload_fonts = false;
		}
		if(handle_close_btn_quit())
			break;
		
		//update submenus
		{
			settings_menu.disable_uid(MENUID_SETTINGS_CONTROLS, replay_is_replaying());
			settings_menu.select_uid(MENUID_SETTINGS_CAPFPS, Throttlefps);
			settings_menu.select_uid(MENUID_SETTINGS_SHOWFPS, ShowFPS);
			settings_menu.select_uid(MENUID_SETTINGS_SHOWTIME, ShowGameTime);
			settings_menu.select_uid(MENUID_SETTINGS_CLICK_FREEZE, ClickToFreeze);
			settings_menu.select_uid(MENUID_SETTINGS_TRANSLAYERS, TransLayers);
			settings_menu.select_uid(MENUID_SETTINGS_NESQUIT, NESquit);
			settings_menu.select_uid(MENUID_SETTINGS_VOLKEYS, volkeys);

			window_menu.select_uid(MENUID_WINDOW_LOCK_ASPECT, DragAspect);
			window_menu.select_uid(MENUID_WINDOW_LOCK_INTSCALE, scaleForceInteger);
			window_menu.select_uid(MENUID_WINDOW_SAVE_SIZE, SaveDragResize);
			window_menu.select_uid(MENUID_WINDOW_SAVE_POS, SaveWinPos);
			window_menu.select_uid(MENUID_WINDOW_STRETCH, stretchGame);
			
			options_menu.select_uid(MENUID_OPTIONS_EPILEPSYPROT, epilepsyFlashReduction);
			options_menu.select_uid(MENUID_OPTIONS_PAUSE_BG, pause_in_background);
			
			name_entry_mode_menu.select_only_index(NameEntryMode);
			
			misc_menu.select_uid(MENUID_MISC_CONSOLE, console_enabled);
			misc_menu.select_uid(MENUID_MISC_CLEAR_CONSOLE_ON_LOAD, clearConsoleOnLoad);
			
			bool nocheat = (replay_is_replaying() || !Playing
				|| (!maxcheat && !zcheats.flags && !get_debug() && DEVLEVEL < 2 && !zqtesting_mode && !devpwd()));
			the_player_menu.disable_uid(MENUID_PLAYER_CHEAT, nocheat);
			refill_menu.disable_uid(MENUID_REFILL_ARROWS, !get_qr(qr_TRUEARROWS));
			cheat_menu.chop_index.reset();
			if(cheat < 4)
				cheat_menu.chop_index = cheat_menu.ind_at(MENUID_CHEAT_CHOP_L1+cheat);
			cheat_menu.select_uid(MENUID_CHEAT_INVULN, getClock());
			cheat_menu.select_uid(MENUID_CHEAT_NOCLIP, toogam);
			cheat_menu.select_uid(MENUID_CHEAT_IGNORESV, ignoreSideview);
			cheat_menu.select_uid(MENUID_CHEAT_GOFAST, gofast);
			
			show_menu.select_uid(MENUID_SHOW_L0, show_layer_0);
			show_menu.select_uid(MENUID_SHOW_L1, show_layer_1);
			show_menu.select_uid(MENUID_SHOW_L2, show_layer_2);
			show_menu.select_uid(MENUID_SHOW_L3, show_layer_3);
			show_menu.select_uid(MENUID_SHOW_L4, show_layer_4);
			show_menu.select_uid(MENUID_SHOW_L5, show_layer_5);
			show_menu.select_uid(MENUID_SHOW_L6, show_layer_6);
			show_menu.select_uid(MENUID_SHOW_OVER, show_layer_over);
			show_menu.select_uid(MENUID_SHOW_PUSH, show_layer_push);
			show_menu.select_uid(MENUID_SHOW_SPR, show_sprites);
			show_menu.select_uid(MENUID_SHOW_FFC, show_ffcs);
			show_menu.select_uid(MENUID_SHOW_SOLIDITY, show_walkflags);
			show_menu.select_uid(MENUID_SHOW_SCRIPTNAME, show_ff_scripts);
			show_menu.select_uid(MENUID_SHOW_HITBOX, show_hitboxes);
			show_menu.select_uid(MENUID_SHOW_EFFECT, show_effectflags);
			
			settings_menu.select_uid(MENUID_SETTINGS_HEARTBEEP, heart_beep);
			settings_menu.select_uid(MENUID_SETTINGS_SAVEINDICATOR, use_save_indicator);
			
			replay_menu.by_uid(MENUID_REPLAY_STOP)->text = fmt::format("Stop {}",
				replay_get_mode() == ReplayMode::Record ? "recording" : "replaying");
			
			replay_menu.select_uid(MENUID_REPLAY_RECORDNEW, zc_get_config("zeldadx", "replay_new_saves", false));
#ifdef HAS_CURL
			replay_menu.select_uid(MENUID_REPLAY_AUTOUPLOAD, replay_upload_auto_enabled());
#endif
			replay_menu.disable_uid(MENUID_REPLAY_STOP, !replay_is_active());
			replay_menu.disable_uid(MENUID_REPLAY_SAVE, replay_get_mode() != ReplayMode::Record);
			replay_menu.select_uid(MENUID_REPLAY_SNAP_ALL, replay_is_snapshot_all_frames());
			
			snapshot_format_menu.select_only_index(SnapshotFormat);
		}
		
		if(debug_enabled)
			settings_menu.select_uid(MENUID_SETTINGS_DEBUG, get_debug());
		
		if(autopop)
			clear_keybuf();
		the_player_menu.run(true);
		if(autopop)
		{
			the_player_menu.pop_sub(0, &the_player_menu);
			the_player_menu.draw(screen, the_player_menu.hovered_ind());
			autopop = false;
			update_hw_screen();
		}
		
		update_hw_screen();
		
		auto mb = gui_mouse_b();
		if(XOR(mb, mouse_down))
		{
			if(!the_player_menu.has_mouse())
				if(mb)
					break;
			mouse_down = mb;
		}
		
		if(input_idle(true) > after_time())
			// run Screeen Saver
		{
			// Screen saver enabled for now.
			clear_keybuf();
			Matrix(ss_speed, ss_density, 0);
			system_pal(true);
			sys_mouse();
		}
		
		poll_keyboard();
		if(esc)
		{
			if(!key[KEY_ESC])
				esc = false;
		}
		
		if(keypressed() && !CHECK_ALT) //System hotkeys
		{
			auto c = peekkey();
			bool eatkey = true;
			switch(c>>8)
			{
				//Spare keys used by the menu
				case KEY_UP:
				case KEY_DOWN:
				case KEY_LEFT:
				case KEY_RIGHT:
					eatkey = false;
					break;
				case KEY_F1:
					onThrottleFPS();
					break;
				case KEY_F2:
					onShowFPS();
					break;
				case KEY_F6:
					onTryQuitMenu();
					break;
				#ifndef ALLEGRO_MACOSX
				case KEY_F9:
					onReset();
					break;
				case KEY_F10:
					onExit();
					break;
				#else
				case KEY_F7:
					onReset();
					break;
				case KEY_F8:
					onExit();
					break;
				#endif
				case KEY_F12:
					onSnapshot();
					break;
				case KEY_TAB:
					onDebug();
					break;
				case KEY_ESC:
					if(!esc)
						running = false;
					break;
			}
			if(eatkey)
				readkey();
		}
		if(Quit || (GameFlags & GAMEFLAG_TRYQUIT))
			break;
	}
	while(running);

	screen = oldscreen;
	
	mouse_down=gui_mouse_b();
	MenuOpen = false;
	if(Quit)
	{
		kill_sfx();
		music_stop();
		update_hw_screen();
	}
	else
	{
		music_resume();
		resume_all_sfx();
		
		if(rc)
			ringcolor(false);
	}
	exit_sys_pal();
	
	eat_buttons();
	
	rc=false;
	clear_keybuf();

	zc_init_apply_cheat_delta();
}

void fix_dialogs()
{
	jwin_center_dialog(about_dlg);
	jwin_center_dialog(gamepad_dlg);
	jwin_center_dialog(credits_dlg);
	jwin_center_dialog(gamemode_dlg);
	jwin_center_dialog(getnum_dlg);
	jwin_center_dialog(goto_dlg);
	jwin_center_dialog(keyboard_control_dlg);
	jwin_center_dialog(midi_dlg);
	jwin_center_dialog(quest_dlg);
	jwin_center_dialog(scrsaver_dlg);
	jwin_center_dialog(sound_dlg);
	jwin_center_dialog(triforce_dlg);
}

INLINE int32_t mixvol(int32_t v1,int32_t v2)
{
	return (zc_min(v1,255)*zc_min(v2,255)) >> 8;
}

int32_t get_emusic_volume()
{
	int32_t temp_volume = emusic_volume;
	if (GameLoaded && !get_qr(qr_OLD_SCRIPT_VOLUME))
		temp_volume = (emusic_volume * FFCore.usr_music_volume) / 10000 / 100;
	if (!zcmusic)
		return temp_volume;
	return (temp_volume * zcmusic->fadevolume) / 10000;
}

int32_t get_zcmusicpos()
{
	int32_t debugtracething = zcmusic_get_curpos(zcmusic);
	return debugtracething;
	return 0;
}

void set_zcmusicpos(int32_t position)
{
	zcmusic_set_curpos(zcmusic, position);
}

void set_zcmusicspeed(int32_t speed)
{
	zcmusic_set_speed(zcmusic, speed);
}

int32_t get_zcmusiclen()
{
	return zcmusic_get_length(zcmusic);
}

void set_zcmusicloop(double start, double end)
{
	zcmusic_set_loop(zcmusic, start, end);
}

void jukebox(int32_t index,int32_t loop)
{
	if (is_headless())
		return;

	music_stop();
	
	if(index<0)		 index=MAXMIDIS-1;
	
	if(index>=MAXMIDIS) index=0;
	
	music_stop();
	
	// Allegro's DIGMID driver (the one normally used on on Linux) gets
	// stuck notes when a song stops. This fixes it.
	if(strcmp(midi_driver->name, "DIGMID")==0)
		zc_set_volume(0, 0);
	
	zc_set_volume(-1, mixvol(tunes[index].volume, midi_volume >>1));
	zc_play_midi(tunes[index].data,loop);
	
	if(tunes[index].start>0)
		zc_midi_seek(tunes[index].start);
		
	midi_loop_start = tunes[index].loop_start;
	midi_loop_end = tunes[index].loop_end;
	
	currmidi=index;
	master_volume(digi_volume, midi_volume);
	//midi_paused=false;
}

void jukebox(int32_t index)
{
	if(index<0)		 index=MAXMIDIS-1;
	
	if(index>=MAXMIDIS) index=0;
	
	// do nothing if it's already playing
	if(index==currmidi && midi_pos>=0)
	{
		return;
	}
	
	jukebox(index,tunes[index].loop);
}

void play_DmapMusic()
{
	if (is_headless())
		return;

	static char tfile[2048];
	static int32_t ttrack=0;
	bool domidi=false;
	
	int32_t fadeoutframes = 0;
	if (zcmusic != NULL)
		fadeoutframes = zcmusic->fadeoutframes;

	if(DMaps[currdmap].tmusic[0]!=0)
	{
		if(zcmusic==NULL ||
		   strcmp(zcmusic->filename,DMaps[currdmap].tmusic)!=0 ||
		   (zcmusic->type==ZCMF_GME && zcmusic->track != DMaps[currdmap].tmusictrack))
		{
			if (DMaps[currdmap].tmusic_xfade_in > 0 || fadeoutframes > 0)
			{
				if (play_enh_music_crossfade(DMaps[currdmap].tmusic, qstpath, DMaps[currdmap].tmusictrack, get_emusic_volume(), DMaps[currdmap].tmusic_xfade_in, fadeoutframes))
				{
					if (zcmusic != NULL)
					{
						zcmusic->fadeoutframes = DMaps[currdmap].tmusic_xfade_out;
						zcmusic_set_loop(zcmusic, double(DMaps[currdmap].tmusic_loop_start / 10000.0), double(DMaps[currdmap].tmusic_loop_end / 10000.0));
					}
				}
			}
			else
			{
				if (zcmusic != NULL)
				{
					zcmusic_stop(zcmusic);
					zcmusic_unload_file(zcmusic);
					zcmusic = NULL;
					zcmixer->newtrack = NULL;
				}

				zcmusic = zcmusic_load_for_quest(DMaps[currdmap].tmusic, qstpath).first;
				zcmixer->newtrack = zcmusic;

				if (zcmusic != NULL)
				{
					zc_stop_midi();
					strcpy(tfile, DMaps[currdmap].tmusic);
					zcmusic_play(zcmusic, emusic_volume);
					int32_t temptracks = 0;
					temptracks = zcmusic_get_tracks(zcmusic);
					temptracks = (temptracks < 2) ? 1 : temptracks;
					ttrack = vbound(DMaps[currdmap].tmusictrack, 0, temptracks - 1);
					zcmusic_change_track(zcmusic, ttrack);
					zcmusic_set_loop(zcmusic, double(DMaps[currdmap].tmusic_loop_start / 10000.0), double(DMaps[currdmap].tmusic_loop_end / 10000.0));
				}
				else
				{
					tfile[0] = 0;
					domidi = true;
				}
			}
		}
	}
	else
	{
		if (DMaps[currdmap].midi == 0 && fadeoutframes > 0 && zcmusic != NULL && strcmp(zcmusic->filename, DMaps[currdmap].tmusic) != 0)
		{
			play_enh_music_crossfade(NULL, qstpath, DMaps[currdmap].tmusictrack, get_emusic_volume(), DMaps[currdmap].tmusic_xfade_in, fadeoutframes);
		}
		else
		{
			domidi = true;
		}
	}
	
	if(domidi)
	{
		int32_t m=DMaps[currdmap].midi;
		
		switch(m)
		{
		case 1:
			jukebox(ZC_MIDI_OVERWORLD);
			break;
			
		case 2:
			jukebox(ZC_MIDI_DUNGEON);
			break;
			
		case 3:
			jukebox(ZC_MIDI_LEVEL9);
			break;
			
		default:
			if(m>=4 && m<4+MAXCUSTOMMIDIS)
				jukebox(m+MIDIOFFSET_DMAP);
			else
				music_stop();
		}
	}
}

void playLevelMusic()
{
	if (is_headless())
		return;

	int32_t m=tmpscr->screen_midi;
	
	switch(m)
	{
	case -2:
		music_stop();
		break;
		
	case -1:
		play_DmapMusic();
		break;
		
	case 1:
		jukebox(ZC_MIDI_OVERWORLD);
		break;
		
	case 2:
		jukebox(ZC_MIDI_DUNGEON);
		break;
		
	case 3:
		jukebox(ZC_MIDI_LEVEL9);
		break;
		
	default:
		if(m>=4 && m<4+MAXCUSTOMMIDIS)
			jukebox(m+MIDIOFFSET_MAPSCR);
		else
			music_stop();
	}
}

void master_volume(int32_t dv,int32_t mv)
{
	if(dv>=0) digi_volume=zc_max(zc_min(dv,255),0);
	
	if(mv>=0) midi_volume=zc_max(zc_min(mv,255),0);
	
	int32_t i = zc_min(zc_max(currmidi,0),MAXMIDIS-1);
	int32_t temp_vol = midi_volume;
	if (!get_qr(qr_OLD_SCRIPT_VOLUME))
		temp_vol = (midi_volume * FFCore.usr_music_volume) / 10000 / 100;
	zc_set_volume(digi_volume,mixvol(tunes[i].volume, temp_vol));
}

// array of voices, one for each sfx sample in the data file
// 0+ = voice #
// -1 = voice not allocated
void Z_init_sound()
{
	for(int32_t i=0; i<WAV_COUNT; i++)
		sfx_voice[i]=-1;

	const char* midis[ZC_MIDI_COUNT] = {
		"assets/dungeon.mid",
		"assets/ending.mid",
		"assets/gameover.mid",
		"assets/level9.mid",
		"assets/overworld.mid",
		"assets/title.mid",
		"assets/triforce.mid",
	};
	for(int32_t i=0; i<ZC_MIDI_COUNT; i++)
	{
		tunes[i].data = load_midi(midis[i]);
		if (!tunes[i].data)
			Z_error_fatal("Missing required file %s\n", midis[i]);
	}

	for(int32_t j=0; j<MAXCUSTOMMIDIS; j++)
		tunes[ZC_MIDI_COUNT+j].data=NULL;
		
	master_volume(digi_volume,midi_volume);
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

SAMPLE* sfx_get_sample(int32_t index)
{
	// check index
	if (index<=0 || index>=WAV_COUNT)
		return nullptr;
		
	if (sfxdat)
	{
		if (index<Z35)
		{
			return (SAMPLE*)sfxdata[index].dat;
		}
		else
		{
			return (SAMPLE*)sfxdata[Z35].dat;
		}
	}
	else
	{
		return &customsfxdata[index];
	}

	return nullptr;
}

// allocates a voice for the sample "wav_index" (index into zelda.dat)
// if a voice is already allocated (and/or playing), then it just returns true
// Returns true:  voice is allocated
//		 false: unsuccessful
bool sfx_init(int32_t index)
{
	// check index
	if(index<=0 || index>=WAV_COUNT)
		return false;

	if (sfx_voice[index] == -1)
	{
		SAMPLE* sample = sfx_get_sample(index);
		if (!sample)
			return false;

		sfx_voice[index] = allocate_voice(sample);
	}

	return sfx_voice[index] != -1;
}

int32_t sfx_get_default_freq(int32_t index)
{
	if (sfxdat)
	{
		if (index < Z35)
		{
			return ((SAMPLE*)sfxdata[index].dat)->freq;
		}
		else
		{
			return ((SAMPLE*)sfxdata[Z35].dat)->freq;
		}
	}
	else
	{
		return customsfxdata[index].freq;
	}
}

int32_t sfx_get_length(int32_t index)
{
	if (sfxdat)
	{
		if (index < Z35)
		{
			return int32_t(((SAMPLE*)sfxdata[index].dat)->len);
		}
		else
		{
			return int32_t(((SAMPLE*)sfxdata[Z35].dat)->len);
		}
	}
	else
	{
		return int32_t(customsfxdata[index].len);
	}
}

// plays an sfx sample
void sfx(int32_t index,int32_t pan,bool loop, bool restart, int32_t vol, int32_t freq)
{
	if(!sfx_init(index))
		return;
	if (!is_headless())
	{
		voice_set_playmode(sfx_voice[index], loop ? PLAYMODE_LOOP : PLAYMODE_PLAY);
		voice_set_pan(sfx_voice[index], pan);

		// Only used by ZScript currently
		if (freq <= -1)
		{
			freq = sfx_get_default_freq(index);
		}
		voice_set_frequency(sfx_voice[index], freq);

		// Only used by ZScript currently
		int32_t temp_volume = (sfx_volume * vol) / 10000 / 100;
		if (GameLoaded && !get_qr(qr_OLD_SCRIPT_VOLUME))
			temp_volume = (temp_volume * FFCore.usr_sfx_volume) / 10000 / 100;
		voice_set_volume(sfx_voice[index], temp_volume);

		int32_t pos = voice_get_position(sfx_voice[index]);

		if (restart) voice_set_position(sfx_voice[index], 0);

		if (pos <= 0)
			voice_start(sfx_voice[index]);
	}

	if (restart && replay_is_debug())
	{
		// TODO: get rid of this bandaid next time replays are mass-updated.
		const char* sfx_name = sfx_string[index];
		if (strcmp(sfx_name, "Hero is hit") == 0)
			sfx_name = "Player is hit";
		else if (strcmp(sfx_name, "Hero dies") == 0)
			sfx_name = "Player dies";
		replay_step_comment(fmt::format("sfx {}", sfx_name));
	}
}

// true if sfx is allocated
bool sfx_allocated(int32_t index)
{
	return (index>0 && index<WAV_COUNT && sfx_voice[index]!=-1);
}

// start it (in loop mode) if it's not already playing,
// otherwise adjust it to play in loop mode -DD
void cont_sfx(int32_t index)
{
	if (is_headless())
		return;

	if(!sfx_init(index))
	{
		return;
	}
	
	if(voice_get_position(sfx_voice[index])<=0)
	{
		voice_set_position(sfx_voice[index],0);
		voice_set_playmode(sfx_voice[index],PLAYMODE_LOOP);
		voice_set_volume(sfx_voice[index], sfx_volume);
		voice_start(sfx_voice[index]);
	}
	else
	{
		adjust_sfx(index, 128, true);
	}
}

// adjust parameters while playing
void adjust_sfx(int32_t index,int32_t pan,bool loop)
{
	if(index<=0 || index>=WAV_COUNT || sfx_voice[index]==-1)
		return;
		
	voice_set_playmode(sfx_voice[index],loop?PLAYMODE_LOOP:PLAYMODE_PLAY);
	voice_set_pan(sfx_voice[index],pan);
}

// pauses a voice
void pause_sfx(int32_t index)
{
	if(index>0 && index<WAV_COUNT && sfx_voice[index]!=-1)
		voice_stop(sfx_voice[index]);
}

// resumes a voice
void resume_sfx(int32_t index)
{
	if (is_headless())
		return;

	if(index>0 && index<WAV_COUNT && sfx_voice[index]!=-1)
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
	if(index<=0 || index>=WAV_COUNT)
		return;
		
	if(sfx_voice[index]!=-1)
	{
		deallocate_voice(sfx_voice[index]);
		sfx_voice[index]=-1;
	}
}

// Stops SFX played by Hero's item of the given family
void stop_item_sfx(int32_t family)
{
	int32_t id=current_item_id(family);
	
	if(id<0)
		return;
		
	stop_sfx(itemsbuf[id].usesound);
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
	switch(pan_style)
	{
	case 0:
		return 128;
		
	case 1:
		return vbound((x>>1)+68,0,255);
		
	case 2:
		return vbound(((x*3)>>2)+36,0,255);
	}
	
	return vbound(x,0,255);
}

bool joybtn(int32_t b)
{
	if(b == 0)
		return false;
	if (b-1 >= joy[joystick_index].num_buttons)
		return false;
		
	return joy[joystick_index].button[b-1].b !=0;
}

bool joystick(int32_t s)
{
	if(s < 0)
		return false;
	if (s >= joy[joystick_index].num_sticks)
		return false;
	
	for (int i = 0; i < joy[joystick_index].stick[s].num_axis; i++)
	{
		if (joy[joystick_index].stick[s].axis[i].d1 || joy[joystick_index].stick[s].axis[i].d2)
			return true;
	}
	return false;
}

const char* joybtn_name(int32_t b)
{
	if (b <= 0 || b > joy[joystick_index].num_buttons)
		return "";

	return joy[joystick_index].button[b-1].name;
}

const char* joystick_name(int32_t s)
{
	if (s < 0 || s >= joy[joystick_index].num_sticks)
		return "";

	return joy[joystick_index].stick[s].name;
}

int32_t button_pressed()
{
	if (joystick_index >= MAX_JOYSTICKS)
		return 0;

	for(int32_t i=1; i<=joy[joystick_index].num_buttons; i++)
	{
		if(joybtn(i))
			return i;
	}

	return 0;
}

int32_t next_press_key();

int32_t next_joy_input(bool buttons)
{
	clear_keybuf();

	//first, we need to wait until they're pressing no buttons
	for(;;)
	{
		if(keypressed())
		{
			switch(readkey()>>8)
			{
			case KEY_ESC:
				return -1;
				
			case KEY_SPACE:
				return 0;
			}
		}
		
		poll_joystick();
		bool done = true;
		
		if (buttons)
		{
			for(int32_t i=1; i<=joy[joystick_index].num_buttons; i++)
			{
				if(joybtn(i)) done = false;
			}
		}
		else
		{
			if (!gamepad_dlg_cur_joystick || !al_get_joystick_active(gamepad_dlg_cur_joystick))
			{
				InfoDialog("ZC", "Invalid gamepad. Did it disconnect?").show();
				return -2;
			}
			for(int32_t i=0; i<joy[joystick_index].num_sticks; i++)
			{
				if(joystick(i)) done = false;
			}
		}
		
		if(done) break;
		rest(1);
	}

	//now, we need to wait for them to press any button
	for(;;)
	{
		if(keypressed())
		{
			switch(readkey()>>8)
			{
			case KEY_ESC:
				return -1;
				
			case KEY_SPACE:
				return 0;
			}
		}
		
		poll_joystick();
		
		if (buttons)
		{
			if (!gamepad_dlg_cur_joystick || !al_get_joystick_active(gamepad_dlg_cur_joystick))
			{
				InfoDialog("ZC", "Invalid gamepad. Did it disconnect?").show();
				return -2;
			}
			for(int32_t i=1; i<=joy[joystick_index].num_buttons; i++)
			{
				if(joybtn(i))
					return i;
			}
		}
		else
		{
			for(int32_t i=0; i<joy[joystick_index].num_sticks; i++)
			{
				if(joystick(i))
					return i;
			}
		}
		rest(1);
	}
}

static bool rButton(bool &btn, bool &flag, bool rawbtn)
{
	bool ret = btn && !flag;
	flag = rawbtn;
	
	return ret;
}
static bool rButton(bool &btn, bool &flag)
{
	bool ret = btn && !flag;
	flag = btn;
	
	return ret;
}
static bool rButtonPeek(bool btn, bool flag)
{
	if(!btn)
	{
		return false;
	}
	else if(!flag)
	{
		return true;
	}
	
	return false;
}

// Updated only by keyboard/gamepad.
// If in replay mode, this is set directly by the replay system.
// This should never be read from directly - use control_state instead.
bool raw_control_state[ZC_CONTROL_STATES];

// Every call to load_control_state (pretty much every frame) resets this to be equal to raw_control_state.
// This state can drift from raw_control_state if button states are "eaten" or overriden by a script. But that only
// lasts until the next call to load_control_state.
bool control_state[ZC_CONTROL_STATES];
bool disable_control[ZC_CONTROL_STATES];
bool drunk_toggle_state[11];
bool disabledKeys[127];
bool KeyInput[127];
bool KeyPress[127];

bool key_current_frame[127];
bool key_previous_frame[127];

static bool key_system[127];
static bool key_system_previous[127];
static bool key_system_press[127];

bool button_press[ZC_CONTROL_STATES];
bool button_hold[ZC_CONTROL_STATES];

#define STICK_1_X joy[joystick_index].stick[js_stick_1_x_stick].axis[js_stick_1_x_axis]
#define STICK_1_Y joy[joystick_index].stick[js_stick_1_y_stick].axis[js_stick_1_y_axis]
#define STICK_2_X joy[joystick_index].stick[js_stick_2_x_stick].axis[js_stick_2_x_axis]
#define STICK_2_Y joy[joystick_index].stick[js_stick_2_y_stick].axis[js_stick_2_y_axis]
#define STICK_PRECISION   56 //define your own sensitivity

void load_control_state()
{
	load_control_called_this_frame = true;

	if (replay_version_check(8, 11))
	{
		for (int i = 0; i < ZC_CONTROL_STATES; i++)
			down_control_states[i] = raw_control_state[i];
	}

	if (!replay_is_replaying())
	{
		raw_control_state[0]=zc_getrawkey(DUkey, true)||(analog_movement ? STICK_1_Y.d1 || STICK_1_Y.pos - js_stick_1_y_offset < -STICK_PRECISION : joybtn(DUbtn));
		raw_control_state[1]=zc_getrawkey(DDkey, true)||(analog_movement ? STICK_1_Y.d2 || STICK_1_Y.pos - js_stick_1_y_offset > STICK_PRECISION : joybtn(DDbtn));
		raw_control_state[2]=zc_getrawkey(DLkey, true)||(analog_movement ? STICK_1_X.d1 || STICK_1_X.pos - js_stick_1_x_offset < -STICK_PRECISION : joybtn(DLbtn));
		raw_control_state[3]=zc_getrawkey(DRkey, true)||(analog_movement ? STICK_1_X.d2 || STICK_1_X.pos - js_stick_1_x_offset > STICK_PRECISION : joybtn(DRbtn));
		raw_control_state[4]=zc_getrawkey(Akey, true)||joybtn(Abtn);
		raw_control_state[5]=zc_getrawkey(Bkey, true)||joybtn(Bbtn);
		raw_control_state[6]=zc_getrawkey(Skey, true)||joybtn(Sbtn);
		raw_control_state[7]=zc_getrawkey(Lkey, true)||joybtn(Lbtn);
		raw_control_state[8]=zc_getrawkey(Rkey, true)||joybtn(Rbtn);
		raw_control_state[9]=zc_getrawkey(Pkey, true)||joybtn(Pbtn);
		raw_control_state[10]=zc_getrawkey(Exkey1, true)||joybtn(Exbtn1);
		raw_control_state[11]=zc_getrawkey(Exkey2, true)||joybtn(Exbtn2);
		raw_control_state[12]=zc_getrawkey(Exkey3, true)||joybtn(Exbtn3);
		raw_control_state[13]=zc_getrawkey(Exkey4, true)||joybtn(Exbtn4);
		
		if(num_joysticks != 0)
		{
			raw_control_state[14] = STICK_2_Y.pos - js_stick_2_y_offset < -STICK_PRECISION;
			raw_control_state[15] = STICK_2_Y.pos - js_stick_2_y_offset > STICK_PRECISION;
			raw_control_state[16] = STICK_2_X.pos - js_stick_2_x_offset < -STICK_PRECISION;
			raw_control_state[17] = STICK_2_X.pos - js_stick_2_x_offset > STICK_PRECISION;
		}
		else
		{
			raw_control_state[14] = false;
			raw_control_state[15] = false;
			raw_control_state[16] = false;
			raw_control_state[17] = false;
		}
	}
	if (replay_is_active())
	{
		if (replay_get_version() < 3)
			replay_poll();
		else if (replay_is_replaying() && replay_get_version() < 6)
			replay_peek_input();
		else if (replay_is_replaying() && replay_version_check(8, 11))
			replay_peek_input();
		if (replay_get_version() == 8)
			update_keys();
	}

	// Some test replay files were made before a serious input bug was fixed, so instead
	// of re-doing them or tossing them out, just check for that zplay version.
	bool botched_input = replay_is_active() && replay_get_version() != 1 && replay_get_version() < 8;
	for (int i = 0; i < ZC_CONTROL_STATES; i++)
	{
		control_state[i] = raw_control_state[i];
		if (botched_input && !control_state[i])
			down_control_states[i] = false;
	}
	bool did_bad_cutscene_btn = false;
	for(int q = 0; q < 18; ++q)
		if(control_state[q] && !active_cutscene.can_button(q))
		{
			control_state[q] = false;
			did_bad_cutscene_btn = true;
		}
	if(did_bad_cutscene_btn)
		active_cutscene.error();
	
	button_press[0]=rButton(control_state[0],button_hold[0]);
	button_press[1]=rButton(control_state[1],button_hold[1]);
	button_press[2]=rButton(control_state[2],button_hold[2]);
	button_press[3]=rButton(control_state[3],button_hold[3]);
	button_press[4]=rButton(control_state[4],button_hold[4]);
	button_press[5]=rButton(control_state[5],button_hold[5]);
	button_press[6]=rButton(control_state[6],button_hold[6]);
	button_press[7]=rButton(control_state[7],button_hold[7]);
	button_press[8]=rButton(control_state[8],button_hold[8]);
	button_press[9]=rButton(control_state[9],button_hold[9]);
	button_press[10]=rButton(control_state[10],button_hold[10]);
	button_press[11]=rButton(control_state[11],button_hold[11]);
	button_press[12]=rButton(control_state[12],button_hold[12]);
	button_press[13]=rButton(control_state[13],button_hold[13]);
	button_press[14]=rButton(control_state[14],button_hold[14]);
	button_press[15]=rButton(control_state[15],button_hold[15]);
	button_press[16]=rButton(control_state[16],button_hold[16]);
	button_press[17]=rButton(control_state[17],button_hold[17]);
}

// Returns true if any game key is pressed. This is needed because keypressed()
// doesn't detect modifier keys and control_state[] can be modified by scripts.
bool zc_key_pressed()
//may also need to use zc_getrawkey
{
	if((zc_getrawkey(DUkey, true)||(analog_movement ? STICK_1_Y.d1 || STICK_1_Y.pos - js_stick_1_y_offset< -STICK_PRECISION : joybtn(DUbtn))) ||
	   (zc_getrawkey(DDkey, true)||(analog_movement ? STICK_1_Y.d2 || STICK_1_Y.pos - js_stick_1_y_offset > STICK_PRECISION : joybtn(DDbtn))) ||
	   (zc_getrawkey(DLkey, true)||(analog_movement ? STICK_1_X.d1 || STICK_1_X.pos - js_stick_1_x_offset < -STICK_PRECISION : joybtn(DLbtn))) ||
	   (zc_getrawkey(DRkey, true)||(analog_movement ? STICK_1_X.d2 || STICK_1_X.pos - js_stick_1_x_offset > STICK_PRECISION : joybtn(DRbtn))) ||
	   (zc_getrawkey(Akey, true)||joybtn(Abtn)) ||
	   (zc_getrawkey(Bkey, true)||joybtn(Bbtn)) ||
	   (zc_getrawkey(Skey, true)||joybtn(Sbtn)) ||
	   (zc_getrawkey(Lkey, true)||joybtn(Lbtn)) ||
	   (zc_getrawkey(Rkey, true)||joybtn(Rbtn)) ||
	   (zc_getrawkey(Pkey, true)||joybtn(Pbtn)) ||
	   (zc_getrawkey(Exkey1, true)||joybtn(Exbtn1)) ||
	   (zc_getrawkey(Exkey2, true)||joybtn(Exbtn2)) ||
	   (zc_getrawkey(Exkey3, true)||joybtn(Exbtn3)) ||
	   (zc_getrawkey(Exkey4, true)||joybtn(Exbtn4))) // Skipping joystick axes
		return true;
	
	return false;
}

bool getInput(int32_t btn, bool press, bool drunk, bool ignoreDisable, bool eatEntirely, bool peek)
{
	bool ret = false, drunkstate = false, rawret = false;;
	bool* flag = &down_control_states[btn];
	switch(btn)
	{
		case btnF12:
			ret = zc_getkey(KEY_F12, ignoreDisable);
			rawret = zc_getrawkey(KEY_F12, ignoreDisable);
			eatEntirely = false;
			break;
		case btnF11:
			ret = zc_getkey(KEY_F11, ignoreDisable);
			rawret = zc_getrawkey(KEY_F11, ignoreDisable);
			eatEntirely = false;
			break;
		case btnF5:
			ret = zc_getkey(KEY_F5, ignoreDisable);
			rawret = zc_getrawkey(KEY_F5, ignoreDisable);
			eatEntirely = false;
			break;
		case btnQ:
			ret = zc_getkey(KEY_Q, ignoreDisable);
			rawret = zc_getrawkey(KEY_Q, ignoreDisable);
			eatEntirely = false;
			break;
		case btnI:
			ret = zc_getkey(KEY_I, ignoreDisable);
			rawret = zc_getrawkey(KEY_I, ignoreDisable);
			eatEntirely = false;
			break;
		case btnM:
			if(FFCore.kb_typing_mode) return false;
			rawret = ret = zc_getrawkey(KEY_ESC, ignoreDisable);
			eatEntirely = false;
			break;
		default: //control_state[] index
			if(FFCore.kb_typing_mode) return false;
			if(!ignoreDisable && get_qr(qr_FIXDRUNKINPUTS) && disable_control[btn]) drunk = false;
			else if(btn<11) drunkstate = drunk_toggle_state[btn];
			ret = control_state[btn] && (ignoreDisable || !disable_control[btn]);
			rawret = raw_control_state[btn];
	}
	assert(flag);
	if(press)
	{
		if(peek)
			ret = rButtonPeek(ret, *flag);
		else if(get_qr(qr_BROKEN_INPUT_DOWN_STATE)) ret = rButton(ret, *flag);
		else ret = rButton(ret, *flag, rawret);
	}
	if(eatEntirely && ret) control_state[btn] = false;
	if(drunk && drunkstate) ret = !ret;
	return ret;
}

byte getIntBtnInput(byte intbtn, bool press, bool drunk, bool ignoreDisable, bool eatEntirely, bool peek)
{
	byte ret = 0;
	if(intbtn & INT_BTN_A) ret |= getInput(btnA, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_A : 0;
	if(intbtn & INT_BTN_B) ret |= getInput(btnB, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_B : 0;
	if(intbtn & INT_BTN_L) ret |= getInput(btnL, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_L : 0;
	if(intbtn & INT_BTN_R) ret |= getInput(btnR, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_R : 0;
	if(intbtn & INT_BTN_EX1) ret |= getInput(btnEx1, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_EX1 : 0;
	if(intbtn & INT_BTN_EX2) ret |= getInput(btnEx2, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_EX2 : 0;
	if(intbtn & INT_BTN_EX3) ret |= getInput(btnEx3, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_EX3 : 0;
	if(intbtn & INT_BTN_EX4) ret |= getInput(btnEx4, press, drunk, ignoreDisable, eatEntirely, peek) ? INT_BTN_EX4 : 0;
	return ret; //No early return, to make sure all button presses are eaten that should be! -Em
}

byte checkIntBtnVal(byte intbtn, byte vals)
{
	return intbtn&vals;
}

bool Up()
{
	return getInput(btnUp);
}
bool Down()
{
	return getInput(btnDown);
}
bool Left()
{
	return getInput(btnLeft);
}
bool Right()
{
	return getInput(btnRight);
}
bool cAbtn()
{
	return getInput(btnA);
}
bool cBbtn()
{
	return getInput(btnB);
}
bool cSbtn()
{
	return getInput(btnS);
}
bool cLbtn()
{
	return getInput(btnL);
}
bool cRbtn()
{
	return getInput(btnR);
}
bool cPbtn()
{
	return getInput(btnP);
}
bool cEx1btn()
{
	return getInput(btnEx1);
}
bool cEx2btn()
{
	return getInput(btnEx2);
}
bool cEx3btn()
{
	return getInput(btnEx3);
}
bool cEx4btn()
{
	return getInput(btnEx4);
}
bool AxisUp()
{
	return getInput(btnAxisUp);
}
bool AxisDown()
{
	return getInput(btnAxisDown);
}
bool AxisLeft()
{
	return getInput(btnAxisLeft);
}
bool AxisRight()
{
	return getInput(btnAxisRight);
}

bool cMbtn()
{
	return getInput(btnM);
}
bool cF12()
{
	return getInput(btnF12);
}
bool cF11()
{
	return getInput(btnF11);
}
bool cF5()
{
	return getInput(btnF5);
}
bool cQ()
{
	return getInput(btnQ);
}
bool cI()
{
	return getInput(btnI);
}

bool rUp()
{
	return getInput(btnUp, true);
}
bool rDown()
{
	return getInput(btnDown, true);
}
bool rLeft()
{
	return getInput(btnLeft, true);
}
bool rRight()
{
	return getInput(btnRight, true);
}
bool rAbtn()
{
	return getInput(btnA, true);
}
bool rBbtn()
{
	return getInput(btnB, true);
}
bool rSbtn()
{
	return getInput(btnS, true);
}
bool rMbtn()
{
	return getInput(btnM, true);
}
bool rLbtn()
{
	return getInput(btnL, true);
}
bool rRbtn()
{
	return getInput(btnR, true);
}
bool rPbtn()
{
	return getInput(btnP, true);
}
bool rEx1btn()
{
	return getInput(btnEx1, true);
}
bool rEx2btn()
{
	return getInput(btnEx2, true);
}
bool rEx3btn()
{
	return getInput(btnEx3, true);
}
bool rEx4btn()
{
	return getInput(btnEx4, true);
}
bool rAxisUp()
{
	return getInput(btnAxisUp, true);
}
bool rAxisDown()
{
	return getInput(btnAxisDown, true);
}
bool rAxisLeft()
{
	return getInput(btnAxisLeft, true);
}
bool rAxisRight()
{
	return getInput(btnAxisRight, true);
}

bool rF11()
{
	return getInput(btnF11, true);
}
bool rQ()
{
	return getInput(btnQ, true);
}
bool rI()
{
	return getInput(btnI, true);
}

bool DrunkUp()
{
	return getInput(btnUp, false, true);
}
bool DrunkDown()
{
	return getInput(btnDown, false, true);
}
bool DrunkLeft()
{
	return getInput(btnLeft, false, true);
}
bool DrunkRight()
{
	return getInput(btnRight, false, true);
}
bool DrunkcAbtn()
{
	return getInput(btnA, false, true);
}
bool DrunkcBbtn()
{
	return getInput(btnB, false, true);
}
bool DrunkcEx1btn()
{
	return getInput(btnEx1, false, true);
}
bool DrunkcEx2btn()
{
	return getInput(btnEx2, false, true);
}
bool DrunkcSbtn()
{
	return getInput(btnS, false, true);
}
bool DrunkcMbtn()
{
	return getInput(btnM, false, true);
}
bool DrunkcLbtn()
{
	return getInput(btnL, false, true);
}
bool DrunkcRbtn()
{
	return getInput(btnR, false, true);
}
bool DrunkcPbtn()
{
	return getInput(btnP, false, true);
}

bool DrunkrUp()
{
	return getInput(btnUp, true, true);
}
bool DrunkrDown()
{
	return getInput(btnDown, true, true);
}
bool DrunkrLeft()
{
	return getInput(btnLeft, true, true);
}
bool DrunkrRight()
{
	return getInput(btnRight, true, true);
}
bool DrunkrAbtn()
{
	return getInput(btnA, true, true);
}
bool DrunkrBbtn()
{
	return getInput(btnB, true, true);
}
bool DrunkrEx1btn()
{
	return getInput(btnEx1, true, true);
}
bool DrunkrEx2btn()
{
	return getInput(btnEx2, true, true);
}
bool DrunkrEx3btn()
{
	return getInput(btnEx3, true, true);
}
bool DrunkrEx4btn()
{
	return getInput(btnEx4, true, true);
}
bool DrunkrSbtn()
{
	return getInput(btnS, true, true);
}
bool DrunkrMbtn()
{
	return getInput(btnM, true, true);
}
bool DrunkrLbtn()
{
	return getInput(btnL, true, true);
}
bool DrunkrRbtn()
{
	return getInput(btnR, true, true);
}
bool DrunkrPbtn()
{
	return getInput(btnP, true, true);
}

void eat_buttons()
{
	getInput(btnA, true, false, true);
	getInput(btnB, true, false, true);
	getInput(btnS, true, false, true);
	getInput(btnM, true, false, true);
	getInput(btnL, true, false, true);
	getInput(btnR, true, false, true);
	getInput(btnP, true, false, true);
	getInput(btnEx1, true, false, true);
	getInput(btnEx2, true, false, true);
	getInput(btnEx3, true, false, true);
	getInput(btnEx4, true, false, true);
}

// Is true for the _first frame_ of a key press.
// But! it is possible that a script manually sets the value of KeyPress,
// in which case it will be restored to the "true" value based on `key_current_frame`
// and `key_previous_frame` on the next frame.
bool zc_readkey(int32_t k, bool ignoreDisable)
{
	if(ignoreDisable) return KeyPress[k];
	switch(k)
	{
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
			return KeyPress[k];
			
		default:
			return KeyPress[k] && !disabledKeys[k];
	}
}

// Is true for _every frame_ a key is held down.
// But! it is possible that a script manually sets the value of KeyInput,
// in which case it will be restored to the "true" value based on `key_current_frame`
// on the next frame.
bool zc_getkey(int32_t k, bool ignoreDisable)
{
	if(ignoreDisable) return KeyInput[k];
	switch(k)
	{
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
			return KeyInput[k];
			
		default:
			return KeyInput[k] && !disabledKeys[k];
	}
}

// Reads (and then clears) the current frame key state directly.
// Scripts can also modify `key_current_frame`.
bool zc_readrawkey(int32_t k, bool ignoreDisable)
{
	if(zc_getrawkey(k, ignoreDisable))
	{
		_key[k]=key[k]=key_current_frame[k]=0;
		return true;
	}
	_key[k]=key[k]=key_current_frame[k]=0;
	return false;
}

// Reads the current frame key state directly.
// Scripts can also modify `key_current_frame`.
bool zc_getrawkey(int32_t k, bool ignoreDisable)
{
	if(ignoreDisable) return key_current_frame[k];
	switch(k)
	{
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
			return key_current_frame[k];
			
		default:
			return key_current_frame[k] && !disabledKeys[k];
	}
}

// Only used for a handful of keys, like tilde and Function keys.
// This state is never read within the game.
// It exists so that all keyboard input still functions during replay,
// without inadvertently doing things like toggling throttling if the player
// presses ~
bool zc_get_system_key(int32_t k)
{
	return key_system[k];
}

// True for the _first_ frame of a key press.
bool zc_read_system_key(int32_t k)
{
	return key_system_press[k];
}

bool is_system_key(int32_t k)
{
	switch (k)
	{
		case KEY_BACKQUOTE:
		case KEY_CLOSEBRACE:
		case KEY_END:
		case KEY_HOME:
		case KEY_OPENBRACE:
		case KEY_PGDN:
		case KEY_PGUP:
		case KEY_TAB:
		case KEY_TILDE:
			return true;
	}
	return is_Fkey(k);
}

void update_system_keys()
{
	for (int32_t q = 0; q < 127; ++q)
	{
		if (!is_system_key(q))
			continue;

		key_system[q] = key[q];
		key_system_press[q] = key_system[q] && !key_system_previous[q];
		key_system_previous[q] = key_system[q];
	}
}

void update_keys()
{
	for (int32_t q = 0; q < 127; ++q)
	{
		// When replaying, replay.cpp takes care of updating `key_current_frame`.
		if (!replay_is_replaying())
			key_current_frame[q] = key[q];

		KeyPress[q] = key_current_frame[q] && !key_previous_frame[q];
		KeyInput[q] = key_current_frame[q];
		key_previous_frame[q] = key_current_frame[q];
	}
}

bool zc_disablekey(int32_t k, bool val)
{
	switch(k)
	{
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
			return false;
			
		default:
			disabledKeys[k] = val;
			return true;
	}
}

void zc_putpixel(int32_t layer, int32_t x, int32_t y, int32_t cset, int32_t color, int32_t timer)
{
	timer=timer;
	particles.add(new particle(zfix(x), zfix(y), layer, cset, color));
}
