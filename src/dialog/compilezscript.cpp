#include "compilezscript.h"
#include <gui/builder.h>
#include "zalleg/zalleg.h"
#include "zq/zquest.h"
#include "alert.h"
#include "alertfunc.h"
#include "headerdlg.h"
#include "zc/ffscript.h"
#include "qst.h"
#include "zscrdata.h"
#include "info.h"
#include <fmt/format.h>
#include "base/misctypes.h"
#include "dialog/view_script_slots.h"
#include <chrono>
using std::string;

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include "base/emscripten_utils.h"
#endif

void doEditZScript();
void clear_map_states();
bool do_slots(vector<shared_ptr<ZScript::Opcode>> const& zasm,
	map<string, disassembled_script_data> &scripts, int assign_mode);
int32_t onZScriptCompilerSettings();

extern string zScript;
extern FFScript FFCore;
extern std::vector<string> asffcscripts, asglobalscripts, asitemscripts,
	asnpcscripts, aseweaponscripts, aslweaponscripts,
	asplayerscripts, asdmapscripts, asscreenscripts,
	asitemspritescripts, ascomboscripts, asgenericscripts,
	assubscreenscripts;
extern std::map<int32_t, script_slot_data > globalmap;

byte compile_success_sample = 0;
byte compile_error_sample = 0;
byte compile_finish_sample = 0;
byte compile_audio_volume = 0;

static void compile_sfx(bool success)
{
	if ( success )
	{
		compile_error_sample = 0;
		compile_success_sample = vbound(zc_get_config("Compiler","compile_success_sample",20),0,255);
		if ( compile_success_sample > 0 )
		{
			compile_audio_volume = vbound(zc_get_config("Compiler","compile_audio_volume",100),0,255);
			if(sfxdat)
				sfx_voice[compile_success_sample]=allocate_voice((SAMPLE*)sfxdata[compile_success_sample].dat);
			else sfx_voice[compile_success_sample]=allocate_voice(&customsfxdata[compile_success_sample]);
			voice_set_volume(sfx_voice[compile_success_sample], compile_audio_volume);
			voice_start(sfx_voice[compile_success_sample]);
		}
	}
	else
	{
		compile_success_sample = 0;
		compile_error_sample = vbound(zc_get_config("Compiler","compile_error_sample",28),0,255);
		if ( compile_error_sample > 0 )
		{
			compile_audio_volume = vbound(zc_get_config("Compiler","compile_audio_volume",100),0,255);
			if(sfxdat)
				sfx_voice[compile_error_sample]=allocate_voice((SAMPLE*)sfxdata[compile_error_sample].dat);
			else sfx_voice[compile_error_sample]=allocate_voice(&customsfxdata[compile_error_sample]);
			voice_set_volume(sfx_voice[compile_error_sample], compile_audio_volume);
			voice_start(sfx_voice[compile_error_sample]);
		}
	}
}

int32_t onCompileScript()
{
	CompileZScriptDialog().show();
	return D_O_K;
}

bool doCompileOpenHeaderDlg()
{
    call_header_dlg();
    return false;
}

static bool compile_cancel;
static int g_assign_mode;
bool quickassign()
{
	g_assign_mode = 1;
	return true;
}
bool smartassign()
{
	g_assign_mode = 2;
	return true;
}
bool docancel()
{
	compile_cancel = true;
	return true;
}

// If assign_mode is 0, will prompt user for assign_mode.
bool do_compile_and_slots(int assign_mode, bool delay)
{
	bool noquick_compile = !assign_mode;
	char tmpfilename[L_tmpnam];
	std::tmpnam(tmpfilename);
	FILE *tempfile = fopen(tmpfilename,"w");

	char consolefilename[L_tmpnam];
	std::tmpnam(consolefilename);

	if(!tempfile)
	{
		jwin_alert("Error","Unable to create a temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
		return true;
	}
	
	fwrite(zScript.c_str(), sizeof(char), zScript.size(), tempfile);
	fclose(tempfile);
	
	map<string, ZScript::ScriptTypeID> stypes;
	map<string, disassembled_script_data> scripts;
	vector<shared_ptr<ZScript::Opcode>> zasm;
	
	std::string quest_rules_hex = get_qr_hexstr();
	auto start_compile_time = std::chrono::steady_clock::now();
	bool hasWarnErr = false;
#ifdef __EMSCRIPTEN__
	int32_t code = em_compile_zscript(tmpfilename, consolefilename, quest_rules_hex.c_str());
#else
	int32_t code = -9999;
	if(!fileexists(ZSCRIPT_FILE))
	{
		InfoDialog("Parser", ZSCRIPT_FILE " was not found!").show();
		return false;
	}
	parser_console.kill();

	std::vector<std::string> args = {
		"-input", tmpfilename,
		"-console", consolefilename,
		"-qr", quest_rules_hex.c_str(),
		"-linked",
	};
	if(noquick_compile && zc_get_config("Compiler","noclose_compile_console",0))
		args.push_back("-noclose");
	if(delay)
		args.push_back("-delay");
	process_manager* pm = launch_piped_process(ZSCRIPT_FILE, "zq_parser_pipe", args);
	dword compile_timeout = zc_get_config("Compiler","compiler_timeout",30,App::zscript);
	if(!pm)
	{
		InfoDialog("Parser","Failed to launch " ZSCRIPT_FILE).show();
		return false;
	}
	
	if (!DisableCompileConsole)
	{
		parser_console.Create("ZScript Parser Output", 600, 200, NULL, "zconsole.exe");
		parser_console.cls(CConsoleLoggerEx::COLOR_BACKGROUND_BLACK);
		parser_console.gotoxy(0,0);
		_print_zconsole("External ZScript Parser\n","[INFO] ",INFO_COLOR);
	}
	else
	{
		box_start(1, "Compile Progress", get_zc_font(font_lfont), get_zc_font(font_sfont),true, 512, 280);
	}
	
	int current = 0, last = 0;
	int syncthing = 0;
	FILE *console = nullptr;
	try
	{
		pm->read(&syncthing, sizeof(int32_t));
		console = fopen(consolefilename, "r");
		bool running = true;
		if (console) 
		{
			char buf4[4096];
			const dword def_idle = delay ? 60*60 : 0;
			dword idle_seconds = def_idle;
			while(running)
			{
				pm->timeout_seconds = compile_timeout ? compile_timeout+idle_seconds : 0;
				idle_seconds = def_idle;
				pm->read(&code, sizeof(int32_t));
				switch(code)
				{
					case ZC_CONSOLE_IDLE_CODE:
					{
						pm->read(&idle_seconds, sizeof(dword));
						idle_seconds += def_idle;
						pm->write(&code, sizeof(int32_t));
						break;
					}
					case ZC_CONSOLE_DB_CODE:
					case ZC_CONSOLE_ERROR_CODE:
					case ZC_CONSOLE_WARN_CODE:
						hasWarnErr = true;
						[[fallthrough]];
					case ZC_CONSOLE_INFO_CODE:
						fseek(console, 0, SEEK_END);
						current = ftell(console);
						if (current != last)
						{
							int amount = (current-last);
							fseek(console, last, SEEK_SET);
							last = current;
							int end = fread(&buf4, sizeof(char), amount, console);
							buf4[end] = 0;
							ReadConsole(buf4, code);
						}
						pm->write(&code, sizeof(int32_t));
						break;
					default:
						running = false;
						break;
				}
			}
		}
		pm->read(&code, sizeof(int32_t));
	}
	catch(zc_io_exception& e)
	{
		zprint2("CAUGHT ZC_IO_EXCEPTION!\n");
		if (DisableCompileConsole) box_end(true);
		switch(e.getType())
		{
			case zc_io_exception::IO_TIMEOUT:
				InfoDialog("Timeout",fmt::format("IO Timeout error: the parser timed out. {}",
					pm->is_alive() ? "Unknown cause." : "Parser process died or crashed.")).show();
				break;
			case zc_io_exception::IO_DEAD:
				InfoDialog("Dead Parser","The parser process died unexpectedly.").show();
				break;
			default:
				InfoDialog("Error","An unknown error occurred while compiling").show();
				break;
		}
		delete pm;
		if(console) fclose(console);
		return false;
	}
	delete pm;
	if(console) fclose(console);
#endif
	auto end_compile_time = std::chrono::steady_clock::now();
	int compile_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_compile_time - start_compile_time).count();
	
	char buf[1024] = {0};
	sprintf(buf, "ZScript compilation: Returned code '%d' (%s)\n"
		"Compile took %d ms%s",
		code, code ? "failure" : "success",
		compile_time_ms,
		code ? (!DisableCompileConsole?"\nCompilation failed. See console for details.":"\nCompilation failed.") : "");
	
	if(!code)
	{
		read_compile_data(zasm, stypes, scripts);
		if (!(DisableCompileConsole || hasWarnErr)) 
		{
			parser_console.kill();
		}
	}
	else if (DisableCompileConsole)
	{
		char buf3[256] = {0};
		sprintf(buf3, "Compile took %d ms", compile_time_ms);
		box_out(buf3);
		box_eol();
		box_out("Compilation failed.");
		box_eol();
	}
	compile_sfx(!code);
	if (DisableCompileConsole) box_end(true);
	
	compile_cancel = code;

	if (!DisableCompileConsole)
	{
		if(code)
			InfoDialog("ZScript Parser", buf).show();
		else if(!assign_mode)
		{
			g_assign_mode = 0;
			AlertFuncDialog("ZScript Parser",
				buf,
				"Quick Assign:"
				"\nUpdates all already-assigned scripts"
				"\nSmart Assign:"
				"\nUpdates all already-assigned scripts"
				"\nAssigns global/hero scripts to empty slots of the same name"
				"\nAssigns all other unassigned scripts to slots",
				4, 2, //4 where [2] is focused
				{ "Quick Assign", "Smart Assign", "OK", "Cancel" },
				{ quickassign, smartassign, nullptr, docancel }
			).show();
			assign_mode = g_assign_mode;
		}
	}
	if ( compile_success_sample > 0 )
	{
		if(sfx_voice[compile_success_sample]!=-1)
		{
			deallocate_voice(sfx_voice[compile_success_sample]);
			sfx_voice[compile_success_sample]=-1;
		}
	}
	if ( compile_error_sample > 0 )
	{
		if(sfx_voice[compile_error_sample]!=-1)
		{
			deallocate_voice(sfx_voice[compile_error_sample]);
			sfx_voice[compile_error_sample]=-1;
		}
	}
	//refresh(rALL);
	
	if(compile_cancel)
		return false;
	
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
	clear_map_states();
	globalmap[0].updateName("~Init"); //force name to ~Init
	
	using namespace ZScript;
	for (auto it = stypes.begin(); it != stypes.end(); ++it)
	{
		string const& name = it->first;
		if(name.size() && name[0] == '~')
			continue; //hidden script
		switch(it->second)
		{
			case scrTypeIdFfc:
				asffcscripts.push_back(name);
				break;
			case scrTypeIdItem:
				asitemscripts.push_back(name);
				break;
			case scrTypeIdNPC:
				asnpcscripts.push_back(name);
				break;
			case scrTypeIdEWeapon:
				aseweaponscripts.push_back(name);
				break;
			case scrTypeIdLWeapon:
				aslweaponscripts.push_back(name);
				break;
			case scrTypeIdPlayer:
				asplayerscripts.push_back(name);
				break;
			case scrTypeIdDMap:
				asdmapscripts.push_back(name);
				break;
			case scrTypeIdScreen:
				asscreenscripts.push_back(name);
				break;
			case scrTypeIdItemSprite:
				asitemspritescripts.push_back(name);
				break;
			case scrTypeIdComboData:
				ascomboscripts.push_back(name);
				break;
			case scrTypeIdGeneric:
				asgenericscripts.push_back(name);
				break;
			case scrTypeIdGlobal:
				asglobalscripts.push_back(name);
				break;
			case scrTypeIdSusbcrData:
				assubscreenscripts.push_back(name);
				break;
		}
	}
	
	//scripts are compiled without error, so store the zscript version here: -Z, 25th July 2019, A29
	QMisc.zscript_last_compiled_version = V_FFSCRIPT;
	FFCore.quest_format[vLastCompile] = V_FFSCRIPT;
	zprint2("Compiled scripts in version: %d\n", QMisc.zscript_last_compiled_version);
	
	//assign scripts to slots
	do_slots(zasm, scripts, assign_mode);

	return true;
}

CompileZScriptDialog::CompileZScriptDialog()
{}

static int32_t Type_Checked = 0, lowcombo = 0, highcombo = 65279, damage = 4;

void CompileZScriptDialog::updateLabels()
{
	char buf[64];
	sprintf(buf, "Last Compiled Using ZScript: v.%d",(FFCore.quest_format[vLastCompile]));
	labels[0]->setText(buf);
	sprintf(buf, "%d Bytes in Buffer", (int32_t)(zScript.size()));
	labels[1]->setText(buf);
}

#define BTN(txt, msg, w) \
Button( \
	fitParent = true, \
	padding = 0_px, \
	text = (txt), \
	width = (w), \
	height = 3_em, \
	onClick = message::msg \
)
#define BTNF(txt, msg, w) \
Button( \
	focused = true, \
	fitParent = true, \
	padding = 0_px, \
	text = (txt), \
	width = (w), \
	height = 3_em, \
	onClick = message::msg \
)

std::shared_ptr<GUI::Widget> CompileZScriptDialog::view()
{
	using namespace GUI::Builder;
	using namespace GUI::Props;
	const GUI::Size panel_width = 200_px;
	const GUI::Size col_spacing = 4_px;
	const GUI::Size row_spacing = 4_px;
	
	window = Window(
		title = "Compile ZScript",
		onClose = message::CANCEL,
		Column(
			labels[0] = Label(hAlign = 0.0),
			labels[1] = Label(hAlign = 0.0),
			Column(hPadding = 0_px, vPadding = DEFAULT_PADDING*2,
				rowSpacing = row_spacing,
				//
				Row(padding = 0_px,
					columnSpacing = col_spacing,
					BTN("&Load",LOAD,panel_width/3+col_spacing/2),
					BTN("E&xport",EXPORT,panel_width/3+col_spacing/2),
					BTN("&Edit",EDIT,panel_width/3+col_spacing/2)
				),
				//
				Row(padding = 0_px,
					columnSpacing = col_spacing,
					BTN("&Docs",DOCS,panel_width/3+col_spacing/2),
					BTN("&std_zh",STD_ZH,panel_width/3+col_spacing/2),
					BTN("Settings",SETTINGS,panel_width/3+col_spacing/2)
				),
				//
				Row(padding = 0_px,
					columnSpacing = col_spacing,
					BTNF("&Compile",COMPILE,panel_width/3+col_spacing/2),
					BTN("Slots",SLOTS,panel_width/3+col_spacing/2),
					BTN("Cancel",CANCEL,panel_width/3+col_spacing/2)
				)
			)
		)
	);
	updateLabels();
	return window;
}

bool CompileZScriptDialog::handleMessage(const GUI::DialogMessage<message>& msg)
{
	bool ctrl = key_shifts & KB_CTRL_FLAG;
	switch(msg.message)
	{
		case message::LOAD:
		{
			//Load from File
			if(zScript.size() > 0)
			{
				if(jwin_alert("Confirm Overwrite","Loading will erase the current buffer.","Proceed anyway?",NULL,"Yes","No",'y','n',get_zc_font(font_lfont))==2)
					return false;
					
				zScript.clear();
				updateLabels();
				saved = false;
			}
			
			if(!getname("Load ZScript (.z, .zh, .zs, .zlib, etc.)", (char *)"z,zh,zs,zlib,zasm,zscript,squid" ,NULL,datapath,false))
				return false;
				
			FILE *zscript = fopen(temppath,"r");
			
			if(zscript == NULL)
			{
				jwin_alert("Error","Cannot open specified file!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				return false;
			}
			
			char c = fgetc(zscript);
			
			while(!feof(zscript))
			{
				zScript += c;
				c = fgetc(zscript);
			}
			
			fclose(zscript);
			saved = false;
			updateLabels();
			return false;
		}
		
		case message::EXPORT:
		{
			if(!getname("Save ZScript (.zs)", "zs", NULL,datapath,false))
				break;
				
			if(exists(temppath))
			{
				if(jwin_alert("Confirm Overwrite","File already exists.","Overwrite?",NULL,"Yes","No",'y','n',get_zc_font(font_lfont))==2)
					break;
			}
			
			FILE *zscript = fopen(temppath,"w");
			
			if(!zscript)
			{
				jwin_alert("Error","Unable to open file for writing!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				break;
			}
			
			int32_t written = (int32_t)fwrite(zScript.c_str(), sizeof(char), zScript.size(), zscript);
			
			if(written != (int32_t)zScript.size())
				jwin_alert("Error","IO error while writing script to file!",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
				
			fclose(zscript);
			return false;
		}
		
		case message::EDIT:
			doEditZScript();
			updateLabels();
			return false;
		
		case message::DOCS:
			if(fileexists("docs/ZScript_Docs.html"))
				launch_file("docs/ZScript_Docs.html");
			return false;
		
		case message::STD_ZH:
			if(util::checkPath("include/std_zh",true))
				launch_file("include/std_zh");
			return false;
		
		case message::SETTINGS:
			onZScriptCompilerSettings();
			return false;

		case message::SLOTS:
			ViewScriptSlotsDialog().show();
			return false;
		
		case message::COMPILE:
		{
			bool delay = ctrl && devpwd();
			return do_compile_and_slots(0, delay);
		}
		
		case message::CANCEL:
			return true;
	}
	return false;
}
