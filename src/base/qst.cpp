#include "allegro/file.h"
#include "base/flags.h"
#include "base/general.h"
#include "base/util.h"
#include "base/version.h"
#include "base/zapp.h"
#include "base/qrs.h"
#include "base/cpool.h"
#include "base/autocombo.h"
#include "base/packfile.h"
#include "base/dmap.h"
#include "base/combo.h"
#include "base/msgstr.h"
#include "base/flags.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <stdio.h>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <fmt/format.h>


#include "fmt/core.h"
#include "metadata/sigs/devsig.h.sig"
#include "metadata/sigs/compilersig.h.sig"
#include "metadata/versionsig.h"
#include "base/zc_alleg.h"
#include "base/zdefs.h"
#include "base/colors.h"
#include "tiles.h"
#include "base/zsys.h"
#include "base/qst.h"
#include "defdata.h"
#include "subscr.h"
#include "sfx.h"
#include "md5.h"
#include "zc/zelda.h"
#include "zinfo.h"
#include "zc/ffscript.h"
#include "particles.h"
#include "dialog/alert.h"
#include "base/misctypes.h"
#include "base/initdata.h"

extern ZModule zcm;
extern zcmodule moduledata;
extern uint8_t __isZQuest;
extern sprite_list  guys, items, Ewpns, Lwpns, chainlinks, decorations;
extern particle_list particles;
extern void setZScriptVersion(int32_t s_version);

static bool read_ext_zinfo = false, read_zinfo = false;
static bool loadquest_report = false;
static char const* loading_qst_name = NULL;
static std::string last_loaded_qstpath;
static byte loading_qst_num = 0;
static byte subscr_mode = ssdtMAX;
// Very old quests only used a byte for combos, and each screen had a "combo page" to vary
// what combos were used. This vector just lets us convert those old quests on load.
static std::vector<byte> old_combo_pages;

dword loading_tileset_flags = 0;

int32_t First[MAX_COMBO_COLS]={0},combo_alistpos[MAX_COMBO_COLS]={0},combo_pool_listpos[MAX_COMBO_COLS]={0},combo_auto_listpos[MAX_COMBO_COLS]={0};
map_and_screen map_page[MAX_MAPPAGE_BTNS]= {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};

#ifdef _MSC_VER
	#define strncasecmp _strnicmp
#endif

#ifndef _AL_MALLOC
#define _AL_MALLOC(a) _al_malloc(a)
#define _AL_FREE(a) _al_free(a)
#endif

using std::string;
using std::pair;

extern int32_t                 hero_animation_speed; //lower is faster animation
extern byte                *colordata;
extern tiledata            *newtilebuf;
extern byte                *trashbuf;
extern itemdata            *itemsbuf;
extern wpndata             *wpnsbuf;
extern comboclass          *combo_class_buf;
extern guydata             *guysbuf;
extern ZCHEATS             zcheats;
extern char                palnames[MAXLEVELS][17];
extern char                *byte_conversion(int32_t number, int32_t format);
extern char                *byte_conversion2(int32_t number1, int32_t number2, int32_t format1, int32_t format2);
string				             zScript;
std::map<int32_t, script_slot_data > ffcmap;
std::map<int32_t, script_slot_data > globalmap;
std::map<int32_t, script_slot_data > genericmap;
std::map<int32_t, script_slot_data > itemmap;
std::map<int32_t, script_slot_data > npcmap;
std::map<int32_t, script_slot_data > ewpnmap;
std::map<int32_t, script_slot_data > lwpnmap;
std::map<int32_t, script_slot_data > playermap;
std::map<int32_t, script_slot_data > dmapmap;
std::map<int32_t, script_slot_data > screenmap;
std::map<int32_t, script_slot_data > itemspritemap;
std::map<int32_t, script_slot_data > comboscriptmap;
std::map<int32_t, script_slot_data > subscreenmap;
void free_newtilebuf();
bool combosread=false;
bool mapsread=false;
bool fixffcs=false;
bool fixpolsvoice=false;

void script_slot_data::update()
{
	switch (format)
	{
		case SCRIPT_FORMAT_INVALID:
			output = fmt::format("{} --{}", slotname, scriptname);
			break;
		case SCRIPT_FORMAT_DISASSEMBLED:
			output = fmt::format("{} ++{}", slotname, scriptname);
			break;
		case SCRIPT_FORMAT_ZASM:
			output = fmt::format("{} =={}", slotname, scriptname);
			break;
		case SCRIPT_FORMAT_DEFAULT:
		default:
			output = fmt::format("{} {}", slotname, scriptname);
	}
}

char qstdat_string[2048] = { 0 };

static zinfo* load_tmp_zi = NULL;

int32_t memDBGwatch[8]= {0,0,0,0,0,0,0,0}; //So I can monitor memory crap
const byte clavio[9]={97,109,111,110,103,117,115,0};

//enum { qe_OK, qe_notfound, qe_invalid, qe_version, qe_obsolete,
//       qe_missing, qe_internal, qe_pwd, qe_match, qe_minver };

const char *qst_error[] =
{
    "OK","File not found","Invalid quest file",
    "Version not supported","Obsolete version",
    "Missing new data"  ,                                     /* but let it pass in ZQuest */
    "Internal error occurred", "Invalid password",
    "Quest title doesn't match saved game", "Save file is for older version of quest; please start new save",
    "Out of memory", "File Debug Mode", "Canceled", "", "No quest assigned"
};

//for legacy quests -DD
enum { ssiBOMB, ssiSWORD, ssiSHIELD, ssiCANDLE, ssiLETTER, ssiPOTION, ssiLETTERPOTION, ssiBOW, ssiARROW, ssiBOWANDARROW, ssiBAIT, ssiRING, ssiBRACELET, ssiMAP,
       ssiCOMPASS, ssiBOSSKEY, ssiMAGICKEY, ssiBRANG, ssiWAND, ssiRAFT, ssiLADDER, ssiWHISTLE, ssiBOOK, ssiWALLET, ssiSBOMB, ssiHCPIECE, ssiAMULET, ssiFLIPPERS,
       ssiHOOKSHOT, ssiLENS, ssiHAMMER, ssiBOOTS, ssiDIVINEFIRE, ssiDIVINEESCAPE, ssiDIVINEPROTECTION, ssiQUIVER, ssiBOMBBAG, ssiCBYRNA, ssiROCS, ssiHOVERBOOTS,
       ssiSPINSCROLL, ssiCROSSSCROLL, ssiQUAKESCROLL, ssiWHISPRING, ssiCHARGERING, ssiPERILSCROLL, ssiWEALTHMEDAL, ssiHEARTRING, ssiMAGICRING, ssiSPINSCROLL2,
       ssiQUAKESCROLL2, ssiAGONY, ssiSTOMPBOOTS, ssiWHIMSICALRING, ssiPERILRING, ssiMAX
     };

static byte deprecated_rules[QUESTRULES_NEW_SIZE];


char *byte_conversion(int32_t number, int32_t format)
{
    static char num_str[40];
    
    if(format==-1)                                            //auto
    {
        format=1;                                               //bytes
        
        if(number>1024)
        {
            format=2;                                             //kilobytes
        }
        
        if(number>1024*1024)
        {
            format=3;                                             //megabytes
        }
        
        if(number>1024*1024*1024)
        {
            format=4;                                             //gigabytes (dude, what are you doing?)
        }
    }
    
    switch(format)
    {
    case 1:                                                 //bytes
        sprintf(num_str,"%db",number);
        break;
        
    case 2:                                                 //kilobytes
        sprintf(num_str,"%.2fk",float(number)/1024);
        break;
        
    case 3:                                                 //megabytes
        sprintf(num_str,"%.2fM",float(number)/(1024*1024));
        break;
        
    case 4:                                                 //gigabytes
        sprintf(num_str,"%.2fG",float(number)/(1024*1024*1024));
        break;
        
    default:
        abort();
        break;
    }
    
    return num_str;
}

char *byte_conversion2(int32_t number1, int32_t number2, int32_t format1, int32_t format2)
{
    static char num_str1[40];
    static char num_str2[40];
    static char num_str[80];
    
    if(format1==-1)                                           //auto
    {
        format1=1;                                              //bytes
        
        if(number1>1024)
        {
            format1=2;                                            //kilobytes
        }
        
        if(number1>1024*1024)
        {
            format1=3;                                            //megabytes
        }
        
        if(number1>1024*1024*1024)
        {
            format1=4;                                            //gigabytes (dude, what are you doing?)
        }
    }
    
    if(format2==-1)                                           //auto
    {
        format2=1;                                              //bytes
        
        if(number2>1024)
        {
            format2=2;                                            //kilobytes
        }
        
        if(number2>1024*1024)
        {
            format2=3;                                            //megabytes
        }
        
        if(number2>1024*1024*1024)
        {
            format2=4;                                            //gigabytes (dude, what are you doing?)
        }
    }
    
    switch(format1)
    {
    case 1:                                                 //bytes
        sprintf(num_str1,"%db",number1);
        break;
        
    case 2:                                                 //kilobytes
        sprintf(num_str1,"%.2fk",float(number1)/1024);
        break;
        
    case 3:                                                 //megabytes
        sprintf(num_str1,"%.2fM",float(number1)/(1024*1024));
        break;
        
    case 4:                                                 //gigabytes
        sprintf(num_str1,"%.2fG",float(number1)/(1024*1024*1024));
        break;
        
    default:
        abort();
        break;
    }
    
    switch(format2)
    {
    case 1:                                                 //bytes
        sprintf(num_str2,"%db",number2);
        break;
        
    case 2:                                                 //kilobytes
        sprintf(num_str2,"%.2fk",float(number2)/1024);
        break;
        
    case 3:                                                 //megabytes
        sprintf(num_str2,"%.2fM",float(number2)/(1024*1024));
        break;
        
    case 4:                                                 //gigabytes
        sprintf(num_str2,"%.2fG",float(number2)/(1024*1024*1024));
        break;
        
    default:
        abort();
        break;
    }
    
    sprintf(num_str, "%s/%s", num_str1, num_str2);
    return num_str;
}

char *ordinal(int32_t num)
{
    static const char *ending[4] = {"st","nd","rd","th"};
    static char ord_str[8];
    
    char *end;
    int32_t t=(num%100)/10;
    int32_t n=num%10;
    
    if(n>=1 && n<4 && t!=1)
        end = (char *)ending[n-1];
    else
        end = (char *)ending[3];
        
    sprintf(ord_str,"%d%s",num%10000,end);
    return ord_str;
}

int32_t get_version_and_build(PACKFILE *f, word *version, word *build)
{
    int32_t ret;
    *version=0;
    *build=0;
    byte temp_map_count=map_count;
    byte temp_midi_flags[MIDIFLAGS_SIZE];
    memcpy(temp_midi_flags, midi_flags, MIDIFLAGS_SIZE);
    
    zquestheader tempheader;
    
    if(!f)
    {
        return qe_invalid;
    }
    
    ret=readheader(f, &tempheader);
    
    if(ret)
    {
        return ret;
    }
    
    map_count=temp_map_count;
    memcpy(midi_flags, temp_midi_flags, MIDIFLAGS_SIZE);
    *version=tempheader.zelda_version;
    *build=tempheader.build;
    return 0;
}


bool find_section(PACKFILE *f, int32_t section_id_requested)
{

    if(!f)
    {
        return false;
    }
    
    int32_t section_id_read;
    bool catchup=false;
    word dummy;
    byte tempbyte;
    char tempbuf[65536];
    
    
    switch(section_id_requested)
    {
    case ID_RULES:
    case ID_STRINGS:
    case ID_MISC:
    case ID_TILES:
    case ID_COMBOS:
    case ID_CSETS:
    case ID_MAPS:
    case ID_DMAPS:
    case ID_DOORS:
    case ID_ITEMS:
    case ID_WEAPONS:
    case ID_COLORS:
    case ID_ICONS:
    case ID_INITDATA:
    case ID_GUYS:
    case ID_MIDIS:
    case ID_CHEATS:
        break;
        
    default:
        al_trace("Bad section requested!\n");
        return false;
        break;
    }
    
    dword section_size;
    
    //section id
    if(!p_mgetl(&section_id_read,f))
    {
        return false;
    }
    
    while(!pack_feof(f))
    {
        switch(section_id_read)
        {
        case ID_RULES:
        case ID_STRINGS:
        case ID_MISC:
        case ID_TILES:
        case ID_COMBOS:
        case ID_CSETS:
        case ID_MAPS:
        case ID_DMAPS:
        case ID_DOORS:
        case ID_ITEMS:
        case ID_WEAPONS:
        case ID_COLORS:
        case ID_ICONS:
        case ID_INITDATA:
        case ID_GUYS:
        case ID_MIDIS:
        case ID_CHEATS:
            catchup=false;
            break;
            
        default:
            break;
        }
        
        
        while(catchup)
        {
            //section id
            section_id_read=(section_id_read<<8);
            
            if(!p_getc(&tempbyte,f))
            {
                return false;
            }
            
            section_id_read+=tempbyte;
        }
        
        if(section_id_read==section_id_requested)
        {
            return true;
        }
        else
        {
            //section version info
            if(!p_igetw(&dummy,f))
            {
                return false;
            }
            
            if(!p_igetw(&dummy,f))
            {
                return false;
            }
            
            //section size
            if(!p_igetl(&section_size,f))
            {
                return false;
            }
            
            //pack_fseek(f, section_size);
            while(section_size>65535)
            {
                pfread(tempbuf,65535,f);
                tempbuf[65535]=0;
                section_size-=65535;
            }
            
            if(section_size>0)
            {
                pfread(tempbuf,section_size,f);
                tempbuf[section_size]=0;
            }
        }
        
        //section id
        if(!p_mgetl(&section_id_read,f))
        {
            return false;
        }
    }
    
    return false;
}





bool valid_zqt(PACKFILE *f)
{

    //word tiles_used;
    //word combos_used;
    //open the file
    //PACKFILE *f = pack_fopen(path, F_READ_PACKED);
    if(!f)
        return false;
        
    //for now, everything else is valid
    return true;
    
    /*int16_t version;
    byte build;
    
    //read the version and make sure it worked
    if(!p_igetw(&version,f))
    {
      goto error;
    }
    
    //read the build and make sure it worked
    if(!p_getc(&build,f))
      goto error;
    
    //read the tile info and make sure it worked
    if(!p_igetw(&tiles_used,f))
    {
      goto error;
    }
    
    for (int32_t i=0; i<tiles_used; i++)
    {
      if(!pfread(trashbuf,tilesize(tf4Bit),f))
      {
        goto error;
      }
    }
    
    //read the combo info and make sure it worked
    if(!p_igetw(&combos_used,f))
    {
      goto error;
    }
    for (int32_t i=0; i<combos_used; i++)
    {
      if(!pfread(trashbuf,sizeof(newcombo),f))
      {
        goto error;
      }
    }
    
    //read the palette info and make sure it worked
    for (int32_t i=0; i<48; i++)
    {
      if(!pfread(trashbuf,newpdTOTAL,f))
      {
        goto error;
      }
    }
    if(!pfread(trashbuf,sizeof(palcycle)*256*3,f))
    {
      goto error;
    }
    for (int32_t i=0; i<MAXLEVELS; i++)
    {
      if(!pfread(trashbuf,PALNAMESIZE,f))
      {
        goto error;
      }
    }
    
    //read the sprite info and make sure it worked
    for (int32_t i=0; i<MAXITEMS; i++)
    {
      if(!pfread(trashbuf,sizeof(itemdata),f))
      {
        goto error;
      }
    }
    
    for (int32_t i=0; i<MAXWPNS; i++)
    {
      if(!pfread(trashbuf,sizeof(wpndata),f))
      {
        goto error;
      }
    }
    
    //read the triforce pieces info and make sure it worked
    for (int32_t i=0; i<8; ++i)
    {
      if(!p_getc(&trashbuf,f))
      {
        goto error;
      }
    }
    
    
    
    //read the game icons info and make sure it worked
    for (int32_t i=0; i<4; ++i)
    {
      if(!p_igetw(&trashbuf,f))
      {
        goto error;
      }
    }
    
    //read the misc colors info and map styles info and make sure it worked
    if(!pfread(trashbuf,sizeof(zcolors),f))
    {
      goto error;
    }
    
    //read the template screens and make sure it worked
    byte num_maps;
    if(!p_getc(&num_maps,f))
    {
      goto error;
    }
    for (int32_t i=0; i<TEMPLATES; i++)
    {
      if(!pfread(trashbuf,sizeof(mapscr),f))
      {
        goto error;
      }
    }
    if (num_maps>1)                                           //dungeon templates
    {
      for (int32_t i=0; i<TEMPLATES; i++)
      {
        if(!pfread(trashbuf,sizeof(mapscr),f))
        {
          goto error;
        }
      }
    }
    
    //yay!  it worked!  close the file and say everything was ok.
    pack_fclose(f);
    return true;
    
    error:
    pack_fclose(f);
    return false;*/
}

bool valid_zqt(const char *filename)
{
    PACKFILE *f=NULL;
    bool isvalid;
    int32_t error;
    f=open_quest_file(&error, filename, false);
    
    if(!f)
    {
//      setPackfilePassword(NULL);
        return false;
    }
    
    isvalid=valid_zqt(f);
	
	clear_quest_tmpfile();
	pack_fclose(f);
    
//  setPackfilePassword(NULL);
    return isvalid;
}

/*
	.qst file history

	.qst files have always been compressed using allegro's packfiles.

	At some point, an encoding layer was added. The two layers look like this:

		1) The top layer is from us. See decode_file_007.
			[0-24]    Preamble "Zelda Classic Quest File"
			[25-28]   Initial decoding seed value.
			[29-X]    Allegro-compressed payload (AKA "packed" file), but XOR'd based on seed value
			[last 4]  Checksum

		2) The bottom layer is a "compressed packed file" from Allegro 4. The entire payload
			is XOR'd with a password (datapwd). Once that is undone, the first four bytes are "slh!",
			followed by a lzss compressed representation of the payload (from allergo' packfile compression).
			The oldest quests skip the password part.

	Simply, the job of this function is to peel away the top layer.

	With this second layer of encryption, the data isn't any more secure, and adds a significant delay
	in opening and saving files. There is no version field, so they decryption key is
	found via trial-by-error (very slow!)

	There are other file types of interest:
		- .zqt: quest template files, skips top-layer encryption pass
		- .qsu: "unencoded" (and uncompressed) files; skips encryption and compression (also makes the longtan password moot)
		- .qu?: same as above. automated backup files
		- .qb?: same as above. automated backup files
		- .qt?: compressed and encrypted (or not encrypted, as of May 2023)

	May 2023: .qst files are now saved without the top layer encoding, and no allegro packfile password. The first bytes of these
	files are now "slh!.AG ZC Enhanced Quest File".
	The following command will take an existing qst file and upgrade it: `./zquest -unencrypt-qst <input> <output>`
*/
PACKFILE *open_quest_file(int32_t *open_error, const char *filename, bool show_progress)
{
	if (show_progress)
	{
		box_start(1, "Loading Quest", get_zc_font(font_lfont), font, true);
	}

	auto unencrypted_result = try_open_maybe_legacy_encoded_file(filename, ENC_STR, nullptr, QH_NEWIDSTR, QH_IDSTR);
	if (unencrypted_result.decoded_pf)
		return unencrypted_result.decoded_pf;
	if (unencrypted_result.not_found)
	{
		*open_error = qe_notfound;
		return nullptr;
	}

	// Everything below here is legacy code - recently saved quest files will have
	// returned by now.
	// The only replay qst file that is still using this legacy encoding is `link_to_the_zelda.qst`.

	// Note: although this is primarily for loading .qst files, it can also handle all of the
	// file types mentioned in the comment above. No need to be told if the file being loaded
	// is encrypted or compressed, we can do some simple and fast checks to determine how to load it.
	bool top_layer_compressed = unencrypted_result.top_layer_compressed;
	bool compressed = unencrypted_result.compressed;
	bool encrypted = unencrypted_result.encrypted;

	char tmpfilename[L_tmpnam];
	temp_name(tmpfilename);
	char percent_done[30];
	int32_t current_method=0;
    
	PACKFILE *f;
	const char *passwd= encrypted ? datapwd : "";
    
	// oldquest flag is set when an unencrypted qst file is suspected.
	bool oldquest = false;
	int32_t ret;

	if(strcmp(filename, "default.qst")!=0)
	{
		box_out(filename);
	}
	else
	{
		box_out("new quest"); // Or whatever
	}
	box_out("...");
	box_eol();
	box_eol();
    
	if(encrypted)
	{
		box_out("Decrypting...");
		box_save_x();
		ret = decode_file_007(filename, tmpfilename, ENC_STR, ENC_METHOD_MAX-1, top_layer_compressed, passwd);
        
		if(ret)
		{
			switch(ret)
			{
			case 1:
				box_out("error.");
				box_eol();
				box_end(true);
				*open_error=qe_notfound;
				return NULL;
                
			case 2:
				box_out("error.");
				box_eol();
				box_end(true);
				*open_error=qe_internal;
				return NULL;
				// be sure not to delete tmpfilename now...
			}
            
			if(ret==5)                                              //old encryption?
			{
				current_method++;
				sprintf(percent_done, "%d%%", (current_method*100)/ENC_METHOD_MAX);
				box_out(percent_done);
				box_load_x();
				ret = decode_file_007(filename, tmpfilename, ENC_STR, ENC_METHOD_211B9, strstr(filename, ".dat#")!=NULL, passwd);
			}
            
			if(ret==5)                                              //old encryption?
			{
				current_method++;
				sprintf(percent_done, "%d%%", (current_method*100)/ENC_METHOD_MAX);
				box_out(percent_done);
				box_load_x();
				ret = decode_file_007(filename, tmpfilename, ENC_STR, ENC_METHOD_192B185, strstr(filename, ".dat#")!=NULL, passwd);
			}
            
			if(ret==5)                                              //old encryption?
			{
				current_method++;
				sprintf(percent_done, "%d%%", (current_method*100)/ENC_METHOD_MAX);
				box_out(percent_done);
				box_load_x();
				ret = decode_file_007(filename, tmpfilename, ENC_STR, ENC_METHOD_192B105, strstr(filename, ".dat#")!=NULL, passwd);
			}
            
			if(ret==5)                                              //old encryption?
			{
				current_method++;
				sprintf(percent_done, "%d%%", (current_method*100)/ENC_METHOD_MAX);
				box_out(percent_done);
				box_load_x();
				ret = decode_file_007(filename, tmpfilename, ENC_STR, ENC_METHOD_192B104, strstr(filename, ".dat#")!=NULL, passwd);
			}
            
			if(ret)
			{
				oldquest = true;
				passwd="";
			}
		}
        
		box_out("okay.");
		box_eol();
	}
	else
	{
		oldquest = true;
	}
    
	box_out("Opening...");
	f = pack_fopen_password(oldquest ? filename : tmpfilename, compressed ? F_READ_PACKED : F_READ, passwd);
	if(!f)
	{
		if((compressed==1)&&(errno==EDOM))
		{
			f = pack_fopen_password(oldquest ? filename : tmpfilename, F_READ, passwd);
		}
        
		if(!f)
		{
			if(!oldquest)
			{
				delete_file(tmpfilename);
			}
			box_out("error.");
			box_eol();
			box_end(true);
			*open_error=qe_invalid;
			return NULL;
		}
	}
    
	if(!oldquest)
	{
		delete_file(tmpfilename);
	}
    
	box_out("okay.");
	box_eol();
    
	return f;
}

PACKFILE *open_quest_template(zquestheader *Header, const char *filename, bool validate)
{
    PACKFILE *f=NULL;
    int32_t open_error=0;
 
    if (Header->templatepath[0] != 0)
    {
        // TODO: should be safe to remove this, no one seems to use custom quest templates.
        filename=Header->templatepath;
    }
    
    f=open_quest_file(&open_error, filename, false);
    
    if(!f)
    {
        return NULL;
    }
    
    if(validate)
    {
        if(!valid_zqt(f))
        {
            jwin_alert("Error","Invalid Quest Template",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
            pack_fclose(f);
			clear_quest_tmpfile();
            return NULL;
        }
    }
    
    return f;
}

static bool init_section(zquestheader *Header, int32_t section_id, miscQdata *Misc, zctune *tunes, bool validate, const char* filename)
{
	// The only time the player uses this is to init tiles for some quests 1.90 or older. See readtiles.
	if (get_app_id() == App::zelda)
		assert(section_id == ID_TILES);

    combosread=false;
    mapsread=false;
    fixffcs=false;
    
    switch(section_id)
    {
    case ID_RULES:
    case ID_STRINGS:
    case ID_MISC:
    case ID_TILES:
    case ID_COMBOS:
    case ID_CSETS:
    case ID_MAPS:
    case ID_DMAPS:
    case ID_DOORS:
    case ID_ITEMS:
    case ID_WEAPONS:
    case ID_COLORS:
    case ID_ICONS:
    case ID_INITDATA:
    case ID_GUYS:
    case ID_MIDIS:
    case ID_CHEATS:
    case ID_ITEMDROPSETS:
    case ID_FAVORITES:
        break;
        
    default:
        return false;
        break;
    }
    
    int32_t ret;
    word version, build;
    PACKFILE *f=NULL;
    
    //why is this here?
    /*
      if(colordata==NULL)
      return false;
      */
    
    //setPackfilePassword(datapwd);
    f=open_quest_template(Header, filename, validate);
    
    if(!f)  //no file, nothing to delete
    {
//	  setPackfilePassword(NULL);
        return false;
    }
    
    ret=get_version_and_build(f, &version, &build);
    
    if(ret||(version==0))
    {
        pack_fclose(f);
		clear_quest_tmpfile();
        
//	setPackfilePassword(NULL);
        return false;
    }
    
    if(!find_section(f, section_id))
    {
        al_trace("Can't find section!\n");
        pack_fclose(f);
        clear_quest_tmpfile();
        
        //setPackfilePassword(NULL);
        return false;
    }
    
    switch(section_id)
    {
    case ID_RULES:
        //rules
        ret=readrules(f, Header);
        break;
        
    case ID_STRINGS:
        //strings
        ret=readstrings(f, Header);
        break;
        
    case ID_MISC:
        //misc data
        ret=readmisc(f, Header, Misc);
        break;
        
    case ID_TILES:
        //tiles
        ret=readtiles(f, newtilebuf, Header, version, build, 0, NEWMAXTILES, true);
        break;
        
    case ID_COMBOS:
        //combos
        clear_combos();
        ret=readcombos(f, Header, version, build, 0, MAXCOMBOS);
        combosread=true;
        break;
        
    case ID_COMBOALIASES:
        //combos
        ret=readcomboaliases(f, Header, version, build);
        break;
        
    case ID_CSETS:
        //color data
        ret=readcolordata(f, Misc, version, build, 0, newerpdTOTAL);
        break;
        
    case ID_MAPS:
        //maps
        ret=readmaps(f, Header);
        mapsread=true;
        break;
        
    case ID_DMAPS:
        //dmaps
        ret=readdmaps(f, Header, version, build, 0, MAXDMAPS);
        break;
        
    case ID_DOORS:
        //door combo sets
        ret=readdoorcombosets(f, Header);
        break;
        
    case ID_ITEMS:
        //items
        ret=readitems(f, version, build);
        break;
        
    case ID_WEAPONS:
        //weapons
        ret=readweapons(f, Header);
        break;
        
    case ID_COLORS:
        //misc. colors
        ret=readmisccolors(f, Header, Misc);
        break;
        
    case ID_ICONS:
        //game icons
        ret=readgameicons(f, Header, Misc);
        break;
        
    case ID_INITDATA:
        //initialization data
        ret=readinitdata(f, Header);
        break;
        
    case ID_GUYS:
        //guys
        ret=readguys(f, Header);
        break;
        
    case ID_MIDIS:
        //midis
        ret=readtunes(f, Header, tunes);
        break;
        
    case ID_CHEATS:
        //cheat codes
        ret=readcheatcodes(f, Header);
        break;
        
    case ID_ITEMDROPSETS:
        //item drop sets
        // Why is this one commented out?
        //ret=readitemdropsets(f, (int32_t)version, (word)build);
        break;
        
    case ID_FAVORITES:
        // favorite combos and aliases
        ret=readfavorites(f, version, build);
        break;
        
    default:
        ret=-1;
        break;
    }
    
    pack_fclose(f);
    clear_quest_tmpfile();

    //setPackfilePassword(NULL);
    if(!ret)
    {
        return true;
    }
    
    return false;
}

bool init_tiles_for_190(bool validate, zquestheader *Header)
{
    return init_section(Header, ID_TILES, NULL, NULL, validate, "modules/classic/classic_1st.qst");
}

bool init_tiles(bool validate, zquestheader *Header)
{
    return init_section(Header, ID_TILES, NULL, NULL, validate, "modules/classic/default.qst");
}

bool init_combos(bool validate, zquestheader *Header)
{
    return init_section(Header, ID_COMBOS, NULL, NULL, validate, "modules/classic/default.qst");
}

bool init_colordata(bool validate, zquestheader *Header, miscQdata *Misc)
{
    return init_section(Header, ID_CSETS, Misc, NULL, validate, "modules/classic/default.qst");
}

void init_spritelists()
{
	if(FFCore.quest_format[vZelda] < 0x255)
	{
		guys.setMax(255);
		items.setMax(255);
		Ewpns.setMax(255);
		Lwpns.setMax(255);
		chainlinks.setMax(255);
		decorations.setMax(255);
		particles.setMax(255);
	}
	else
	{
		guys.setMax(255);
		items.setMax(255);
		Ewpns.setMax(255);
		Lwpns.setMax(255);
		chainlinks.setMax(255);
		decorations.setMax(255);
		particles.setMax(255*((255*4)+1)); //255 per sprite that can use particles; guys, items, ewpns, lwpns, +HERO
	}
}

bool reset_items(bool validate, zquestheader *Header)
{
    bool ret = true;
    if (get_app_id() == App::zquest)
        ret = init_section(Header, ID_ITEMS, NULL, NULL, validate, "modules/classic/default.qst");
    
    for(int32_t i=0; i<MAXITEMS; i++) reset_itemname(i);
    
    return ret;
}

bool reset_guys()
{
    // The .dat file's guys definitions are always synchronised with defdata.cpp's - even the tile settings.
    init_guys(V_GUYS);
    return true;
}

bool reset_wpns(bool validate, zquestheader *Header)
{
	bool ret = true;
    if (get_app_id() == App::zquest)
        ret = init_section(Header, ID_WEAPONS, NULL, NULL, validate, "modules/classic/default.qst");
    
    for(int32_t i=0; i<MAXWPNS; i++)
        reset_weaponname(i);
        
    return ret;
}

bool reset_mapstyles(bool validate, miscQdata *Misc)
{
    Misc->colors.blueframe_tile = 20044;
    Misc->colors.blueframe_cset = 0;
    Misc->colors.triforce_tile = 23461;
    Misc->colors.triforce_cset = 1;
    Misc->colors.triframe_tile = 18752;
    Misc->colors.triframe_cset = 1;
    Misc->colors.overworld_map_tile = 16990;
    Misc->colors.overworld_map_cset = 2;
    Misc->colors.HCpieces_tile = 21160;
    Misc->colors.HCpieces_cset = 8;
    Misc->colors.dungeon_map_tile = 19651;
    Misc->colors.dungeon_map_cset = 8;
    return true;
}

int32_t get_qst_buffers()
{
    TheMaps.resize(MAPSCRS);
	old_combo_pages.resize(MAPSCRS);
	map_autolayers.resize(6);

    Z_message("OK\n");
    
    // The vast majority of finished quests (and I presume this will be consistent for all time) use < 1000 strings in total.
    // (Shoelace's "Hero of Dreams" uses 1415.)
    // So let's be a bit generous and allow 4096 initially.
    // In the rare event that a quest overshoots this mark, we'll reallocate to the full 65535 later.
    // I tested it and it worked without flaw on 6/6/11. - L.
	// 2022: bumped from 4096 to 8192 to avoid a bug where the Strings menu shows (None) strings when the list passes
	//       this threshold. Possibly some bug related to `msglistcache` to being reset?
	// See https://discord.com/channels/876899628556091432/992984989073416242
    msg_strings_size = 8192;
    
	MsgStrings = new MsgStr[msg_strings_size];
        
    //memset(MsgStrings, 0, sizeof(MsgStr)*msg_strings_size);
	for(auto q = 0; q < msg_strings_size; ++q)
	{
		MsgStrings[q].clear();
	}   
    
    if((DMaps=new dmap[MAXDMAPS])==NULL)
        return 0;

    
    
	combobuf.clear();
	combobuf.resize(MAXCOMBOS);
    
    
    if((colordata=(byte*)malloc(psTOTAL255))==NULL)
        return 0;
    
    free_newtilebuf();
    if((newtilebuf=(tiledata*)malloc(NEWMAXTILES*sizeof(tiledata)))==NULL)
        return 0;
        
    memset(newtilebuf, 0, NEWMAXTILES*sizeof(tiledata));
    //Z_message("Performed memset on tiles\n"); 
    clear_tiles(newtilebuf);
    //Z_message("Performed clear_tiles()\n"); 
    
    if (get_app_id() == App::zquest)
    {
        if((grabtilebuf=(tiledata*)malloc(NEWMAXTILES*sizeof(tiledata)))==NULL)
            return 0;
            
        memset(grabtilebuf, 0, NEWMAXTILES*sizeof(tiledata));
        clear_tiles(grabtilebuf);
    }
    
    if((trashbuf=(byte*)malloc(100000))==NULL)
        return 0;
        
    // Big, ugly band-aid here. Perhaps the most common cause of random crashes
    // has been inadvertently accessing itemsbuf[-1]. All such crashes should be
    // fixed by ensuring there's actually itemdata there.
    // If you change this, be sure to update del_qst_buffers, too.
    
    if((itemsbuf=(itemdata*)malloc(sizeof(itemdata)*(MAXITEMS+1)))==NULL)
        return 0;
        
    memset(itemsbuf,0,sizeof(itemdata)*(MAXITEMS+1));
    itemsbuf++;
    
    if((wpnsbuf=(wpndata*)malloc(sizeof(wpndata)*MAXWPNS))==NULL)
        return 0;
        
    memset(wpnsbuf,0,sizeof(wpndata)*MAXWPNS);
    
    if((guysbuf=(guydata*)malloc(sizeof(guydata)*MAXGUYS))==NULL)
        return 0;
        
    memset(guysbuf,0,sizeof(guydata)*MAXGUYS);
    
    if((combo_class_buf=(comboclass*)malloc(sizeof(comboclass)*cMAX))==NULL)
        return 0;
        
    return 1;
}


void free_newtilebuf()
{
    if(newtilebuf)
    {
        for(int32_t i=0; i<NEWMAXTILES; i++)
            if(newtilebuf[i].data)
                free(newtilebuf[i].data);
                
        free(newtilebuf);
	newtilebuf = 0;
    }
}

void free_grabtilebuf()
{
    if (get_app_id() == App::zquest)
    {
        if(grabtilebuf)
        {
            for(int32_t i=0; i<NEWMAXTILES; i++)
                if(grabtilebuf[i].data) free(grabtilebuf[i].data);
                
            free(grabtilebuf);
	    grabtilebuf = 0;
        }
    }
}

void del_qst_buffers()
{
    if(MsgStrings) delete[] MsgStrings;
    
	if (DMaps) delete[] DMaps;
    
	combobuf.clear();
    
    if(colordata) free(colordata);
	
    free_newtilebuf();
    free_grabtilebuf();
    
    if(trashbuf) free(trashbuf);
    
    // See get_qst_buffers
    if(itemsbuf)
    {
        itemsbuf--;
        free(itemsbuf);
    }
    
    if(wpnsbuf) free(wpnsbuf);
    
    if(guysbuf) free(guysbuf);
    
    if(combo_class_buf) free(combo_class_buf);
}

bool init_palnames()
{
    // if(palnames==NULL)
        // return false;
        
    for(int32_t x=0; x<MAXLEVELS; x++)
    {
        switch(x)
        {
        case 0:
            sprintf(palnames[x],"Overworld");
            break;
            
        case 10:
            sprintf(palnames[x],"Caves");
            break;
            
        case 11:
            sprintf(palnames[x],"Passageways");
            break;
            
        default:
            sprintf(palnames[x],"%c",0);
            break;
        }
    }
    
    return true;
}

static void *read_block(PACKFILE *f, int32_t size, int32_t alloc_size)
{
    void *p;
    
    p = _AL_MALLOC(MAX(size, alloc_size));
    
    if(!p)
    {
        return NULL;
    }
    
    if(!pfread(p,size,f))
    {
        _AL_FREE(p);
        return NULL;
    }
    
    if(pack_ferror(f))
    {
        _AL_FREE(p);
        return NULL;
    }
    
    return p;
}

// Only use for reading parts of older quests (Header->zelda_version <= 0x192)
static const byte* legacy_skip_flags;

/* read_midi:
  *  Reads MIDI data from a datafile (this is not the same thing as the
  *  standard midi file format).
  */

static MIDI *read_midi(PACKFILE *f)
{
    MIDI *m;
    int32_t c;
    int16_t divisions=0;
    int32_t len=0;
    
    m = (MIDI*)_AL_MALLOC(sizeof(MIDI));
    
    if(!m)
    {
        return NULL;
    }
    
    for(c=0; c<MIDI_TRACKS; c++)
    {
        m->track[c].len = 0;
        m->track[c].data = NULL;
    }
    
    p_mgetw(&divisions,f);
    m->divisions=divisions;
    
    for(c=0; c<MIDI_TRACKS; c++)
    {
        p_mgetl(&len,f);
        m->track[c].len=len;
        
        if(m->track[c].len > 0)
        {
            m->track[c].data = (byte*)read_block(f, m->track[c].len, 0);
            
            if(!m->track[c].data)
            {
                destroy_midi(m);
                return NULL;
            }
        }
    }
    
    LOCK_DATA(m, sizeof(MIDI));
    
    for(c=0; c<MIDI_TRACKS; c++)
    {
        if(m->track[c].data)
        {
            LOCK_DATA(m->track[c].data, m->track[c].len);
        }
    }
    
    return m;
}

void clear_combo(int32_t i)
{
	combobuf[i].clear();
}

void clear_combos()
{
    for(int32_t tmpcounter=0; tmpcounter<MAXCOMBOS; tmpcounter++)
        clear_combo(tmpcounter);
}

void pack_combos()
{
    int32_t di = 0;
    
    for(int32_t si=0; si<1024; si+=2)
        combobuf[di++] = combobuf[si];
        
    for(; di<1024; di++)
        clear_combo(di);
}

void reset_tunes(zctune *tune)
{
    for(int32_t i=0; i<MAXCUSTOMTUNES; i++)
    {
        tune[i].reset();
    }
}


/*void reset_midi(zcmidi_ *m)
{
  m->title[0]=0;
  m->loop=1;
  m->volume=144;
  m->start=0;
  m->loop_start=-1;
  m->loop_end=-1;
  if(m->midi)
  {
    destroy_midi(m->midi);
  }
  m->midi=NULL;
}


void reset_midis(zcmidi_ *m)
{
  for(int32_t i=0; i<MAXCUSTOMMIDIS; i++)
  {
      reset_midi(m+i);
  }
}
*/

void reset_scr(int32_t scr)
{
    /*
      byte *di=((byte*)TheMaps)+(scr*sizeof(mapscr));
      for(unsigned i=0; i<sizeof(mapscr); i++)
      *(di++) = 0;
      TheMaps[scr].valid=mVERSION;
      */
    
    TheMaps[scr].zero_memory();
    //byte *di=((byte*)TheMaps)+(scr*sizeof(mapscr));
    
    for(int32_t i=0; i<6; i++)
    {
        //these will be uncommented later
        //TheMaps[scr].layerxsize[i]=16;
        //TheMaps[scr].layerysize[i]=11;
        TheMaps[scr].layeropacity[i]=255;
    }
    
    TheMaps[scr].valid=mVERSION;
    
}

int32_t doortranslations_u[9][4]=
{
    {37,38,53,54},
    {37,38,39,40},
    {37,38,55,56},
    {37,38,39,40},
    {37,38,53,54},
    {37,38,53,54},
    {37,38,53,54},
    {7,8,23,24},
    {7,8,41,42}
};

int32_t doortranslations_d[9][4]=
{
    {117,118,133,134},
    {135,136,133,134},
    {119,120,133,134},
    {135,136,133,134},
    {117,118,133,134},
    {117,118,133,134},
    {117,118,133,134},
    {151,152,167,168},
    {137,138,167,168},
};

//enum {dt_pass=0, dt_lock, dt_shut, dt_boss, dt_olck, dt_osht, dt_obos, dt_wall, dt_bomb, dt_walk, dt_max};
int32_t doortranslations_l[9][6]=
{
    {66,67,82,83,98,99},
    {66,68,82,84,98,100},
    {66,69,82,85,98,101},
    {66,68,82,84,98,100},
    {66,67,82,83,98,99},
    {66,67,82,83,98,99},
    {66,67,82,83,98,99},
    {64,65,80,81,96,97},
    {64,65,80,114,96,97},
};

int32_t doortranslations_r[9][6]=
{

    {76,77,92,93,108,109},
    {75,77,91,93,107,109},
    {74,77,90,93,106,109},
    {75,77,91,93,107,109},
    {76,77,92,93,108,109},
    {76,77,92,93,108,109},
    {76,77,92,93,108,109},
    {78,79,94,95,110,111},
    {78,79,125,95,110,111},
};

int32_t tdcmbdat(int32_t map, int32_t scr, int32_t pos)
{
	byte old_cpage = old_combo_pages[map*MAPSCRS+scr];
    return (TheMaps[map*MAPSCRS+TEMPLATE].data[pos]&0xFF) + (old_cpage<<8);
}

int32_t tdcmbcset(int32_t map, int32_t scr, int32_t pos)
{
    //these are here to bypass compiler warnings about unused arguments
    map=map;
    scr=scr;
    pos=pos;
    
    //what does this function do?
    //  return TheMaps[map*MAPSCRS+TEMPLATE].cset[pos];
    return 2;
}

int32_t MakeDoors(int32_t map, int32_t scr)
{
    if(!(TheMaps[map*MAPSCRS+scr].valid&mVALID))
    {
        return 0;
    }
    
    DoorComboSet tempdcs{};
    
    //up
    for(int32_t i=0; i<9; i++)
    {
        for(int32_t j=0; j<4; j++)
        {
            tempdcs.doorcombo_u[i][j]=tdcmbdat(map,scr,doortranslations_u[i][j]);
            tempdcs.doorcset_u[i][j]=tdcmbcset(map,scr,doortranslations_u[i][j]);
        }
    }
    
    tempdcs.bombdoorcombo_u[0]=tdcmbdat(map,scr,57);
    tempdcs.bombdoorcset_u[0]=tdcmbcset(map,scr,57);
    tempdcs.bombdoorcombo_u[1]=tdcmbdat(map,scr,58);
    tempdcs.bombdoorcset_u[1]=tdcmbcset(map,scr,58);
    tempdcs.walkthroughcombo[0]=tdcmbdat(map,scr,34);
    tempdcs.walkthroughcset[0]=tdcmbdat(map,scr,34);
    
    //down
    for(int32_t i=0; i<9; i++)
    {
        for(int32_t j=0; j<4; j++)
        {
            tempdcs.doorcombo_d[i][j]=tdcmbdat(map,scr,doortranslations_d[i][j]);
            tempdcs.doorcset_d[i][j]=tdcmbcset(map,scr,doortranslations_d[i][j]);
        }
    }
    
    tempdcs.bombdoorcombo_d[0]=tdcmbdat(map,scr,121);
    
    tempdcs.bombdoorcset_d[0]=tdcmbcset(map,scr,121);
    tempdcs.bombdoorcombo_d[1]=tdcmbdat(map,scr,122);
    tempdcs.bombdoorcset_d[1]=tdcmbcset(map,scr,122);
    tempdcs.walkthroughcombo[1]=tdcmbdat(map,scr,34);
    tempdcs.walkthroughcset[1]=tdcmbdat(map,scr,34);
    
    //left
    //        TheMaps[i*MAPSCRS+j].warpdmap=TheOldMap.warpdmap;
    for(int32_t i=0; i<9; i++)
    {
        for(int32_t j=0; j<6; j++)
        {
            tempdcs.doorcombo_l[i][j]=tdcmbdat(map,scr,doortranslations_l[i][j]);
            tempdcs.doorcset_l[i][j]=tdcmbcset(map,scr,doortranslations_l[i][j]);
        }
    }
    
    for(int32_t j=0; j>6; j++)
    {
        if((j!=2)&&(j!=3))
        {
            tempdcs.doorcombo_l[dt_bomb][j]=TheMaps[map*MAPSCRS+scr].data[doortranslations_l[dt_bomb][j]];
            tempdcs.doorcset_l[dt_bomb][j]=TheMaps[map*MAPSCRS+scr].cset[doortranslations_l[dt_bomb][j]];
        }
    }
    
    tempdcs.bombdoorcombo_l[0]=0;
    tempdcs.bombdoorcset_l[0]=tdcmbcset(map,scr,115);
    tempdcs.bombdoorcombo_l[1]=tdcmbdat(map,scr,115);
    tempdcs.bombdoorcset_l[1]=tdcmbcset(map,scr,115);
    tempdcs.bombdoorcombo_l[2]=0;
    tempdcs.bombdoorcset_l[2]=tdcmbcset(map,scr,115);
    tempdcs.walkthroughcombo[2]=tdcmbdat(map,scr,34);
    tempdcs.walkthroughcset[2]=tdcmbdat(map,scr,34);
    
    //right
    for(int32_t i=0; i<9; i++)
    {
        for(int32_t j=0; j<6; j++)
        {
            tempdcs.doorcombo_r[i][j]=tdcmbdat(map,scr,doortranslations_r[i][j]);
            tempdcs.doorcset_r[i][j]=tdcmbcset(map,scr,doortranslations_r[i][j]);
        }
    }
    
    for(int32_t j=0; j>6; j++)
    {
        if((j!=2)&&(j!=3))
        {
            tempdcs.doorcombo_r[dt_bomb][j]=TheMaps[map*MAPSCRS+scr].data[doortranslations_r[dt_bomb][j]];
            tempdcs.doorcset_r[dt_bomb][j]=TheMaps[map*MAPSCRS+scr].cset[doortranslations_r[dt_bomb][j]];
        }
    }
    
    tempdcs.bombdoorcombo_r[0]=0;
    tempdcs.bombdoorcset_r[0]=tdcmbcset(map,scr,124);
    tempdcs.bombdoorcombo_r[1]=tdcmbdat(map,scr,124);
    tempdcs.bombdoorcset_r[1]=tdcmbcset(map,scr,124);
    tempdcs.bombdoorcombo_r[2]=0;
    tempdcs.bombdoorcset_r[2]=tdcmbcset(map,scr,124);
    tempdcs.walkthroughcombo[3]=tdcmbdat(map,scr,34);
    tempdcs.walkthroughcset[3]=tdcmbdat(map,scr,34);
    
    int32_t k;
    
    for(k=0; k<door_combo_set_count; k++)
    {
        if(DoorComboSets[k]==tempdcs)
        {
            break;
        }
    }

    if (k >= DoorComboSets.size())
    {
        return 0;
    }

    if(k==door_combo_set_count)
    {
        DoorComboSets[k]=tempdcs;
        DoorComboSetNames[k] = fmt::format("Door Combo Set {}", k);
        ++door_combo_set_count;
    }
    
    return k;
    /*
      doorcombo_u[9][4];
      doorcset_u[9][4];
      doorcombo_d[9][4];
      doorcset_d[9][4];
      doorcombo_l[9][6];
      doorcset_l[9][6];
      doorcombo_r[9][6];
      doorcset_r[9][6];
      bombdoorcombo_u[2];
      bombdoorcset_u[2];
      bombdoorcombo_d[2];
      bombdoorcset_d[2];
      bombdoorcombo_l[3];
      bombdoorcset_l[3];
      bombdoorcombo_r[3];
      bombdoorcset_r[3];
      walkthroughcombo[4];
      walkthroughcset[4];
      */
}

INLINE int32_t tcmbdat2(int32_t map, int32_t scr, int32_t pos)
{
	byte old_cpage = old_combo_pages[map*MAPSCRS+scr];
    return (TheMaps[map*MAPSCRS+TEMPLATE2].data[pos]&0xFF) + (old_cpage<<8);
}

INLINE int32_t tcmbcset2(int32_t map, int32_t pos)
{

    return TheMaps[map*MAPSCRS+TEMPLATE2].cset[pos];
}

INLINE int32_t tcmbflag2(int32_t map, int32_t pos)
{
    return TheMaps[map*MAPSCRS+TEMPLATE2].sflag[pos];
}


void get_questpwd(char *encrypted_pwd, int16_t pwdkey, char *pwd)
{
    char temp_pwd[30];
    memset(temp_pwd,0,30);
    
    if(pwdkey!=0)
    {
        memcpy(temp_pwd,encrypted_pwd,30);
        temp_pwd[29]=0;
        
        for(int32_t i=0; i<30; i++)
        {
            temp_pwd[i] -= pwdkey;
            int32_t t=pwdkey>>15;
            pwdkey = (pwdkey<<1)+t;
        }
    }
    
    memcpy(pwd,temp_pwd,30);
}


bool devpwd()
{
	#ifdef _DEBUG
	return true;
	#endif
	#if DEVLEVEL > 3
	return true;
	#endif
	return !strcmp(zc_get_config("dev","pwd","",App::zquest), (char*)clavio) || is_ci();
}
bool check_questpwd(zquestheader *Header, char *pwd)
{
	if(devpwd())
		return true;
	if((!strcmp(pwd, (char*)clavio)))
		return true;
	cvs_MD5Context ctx;
	uint8_t md5sum[16];
	
	cvs_MD5Init(&ctx);
	cvs_MD5Update(&ctx, (const uint8_t*)pwd, (unsigned)strlen(pwd));
	cvs_MD5Final(md5sum, &ctx);
	
	return (memcmp(Header->pwd_hash,md5sum,16)==0);
}

static char const* key_exts[KEYFILE_NUMTY] = {"key", "zcheat", "zpwd"};
static bool key_hashed[KEYFILE_NUMTY] = {false, true, false};
static char const* key_namestr[KEYFILE_NUMTY] = {"Master","Cheat","ZPwd"};

static bool check_keyfile(char const* path, char const* ext, bool hashed, char const* typestr, zquestheader* Header)
{
	char keyfilename[2048];
	replace_extension(keyfilename, path, ext, 2047);
	if(!exists(keyfilename))
		return false;
	bool ret = false;
	
	char password[QSTPWD_LEN] = {0}, pwd[32] = {0};
	PACKFILE *fp = pack_fopen_password(keyfilename, F_READ,"");
	char msg[80] = {0};
	pfread(msg, 80, fp);
	if(strcmp(msg,"ZQuest Auto-Generated Quest Password Key File.  DO NOT EDIT!"))
	{
		zprint2("Found %s Key File '%s' (invalid header)\n", typestr, keyfilename);
		pack_fclose(fp);
		return false;
	}
	int16_t ver;
	byte bld;
	p_igetw(&ver, fp);
	p_getc(&bld, fp);
	pfread(password, QSTPWD_LEN, fp, true);
	if(hashed)
	{
		char unhashed_pw[QSTPWD_LEN] = {0};
		
		char hashmap = 'Z';
		hashmap += 'Q';
		hashmap += 'U';
		hashmap += 'E';
		hashmap += 'S';
		hashmap += 'T';
		
		for ( int32_t q = 0; q < QSTPWD_LEN; ++q )
			unhashed_pw[q] = password[q] - hashmap;
		
		ret = check_questpwd(Header, unhashed_pw);
	}
	else ret = check_questpwd(Header, password);
	pack_fclose(fp);
	zprint2("Found %s Key File '%s' (%s access)\n",
		typestr, keyfilename, ret ? "valid" : "invalid");
	return ret;
}

bool check_keyfiles(char const* path, vector<uint> types, zquestheader* Header)
{
	char exedir[PATH_MAX] = {0};
	extract_name(path, exedir, FILENAMEALL);
	char const* paths[] = {path, exedir};
	for(uint keyty : types)
	{
		if(keyty >= KEYFILE_NUMTY)
			continue;
		for(char const* p : paths)
		{
			if(check_keyfile(p, key_exts[keyty], key_hashed[keyty], key_namestr[keyty], Header))
				return true;
		}
	}
	return false;
}

void print_quest_metadata(zquestheader const& tempheader, char const* path, byte qst_num)
{
	zprint2("\n");
	zprint2("[QUEST METADATA]\n");
	if(qst_num < moduledata.max_quest_files)
		zprint2("Loading module quest %d\n", qst_num+1);
	if(path)
		zprint2("Path: %s\n", path);
	zprint2("ZC Version: %s\n", tempheader.getVerStr());
	if(tempheader.new_version_id_date_day)
		zprint2("ZC Build Date: %d-%d-%d %s %s\n", tempheader.new_version_id_date_year, tempheader.new_version_id_date_month, tempheader.new_version_id_date_day, tempheader.build_timestamp, tempheader.build_timezone);
	if(tempheader.version[0])
		zprint2("Qst Version: %s\n", tempheader.version);
	if(tempheader.author[0])
		zprint2("Author: %s\n", tempheader.author);
	zprint2("\n");
}

int32_t readheader(PACKFILE *f, zquestheader *Header, byte printmetadata)
{
	int32_t dummy;
	zquestheader tempheader{};
	char dummybuf[80];
	byte temp_map_count;
	byte temp_midi_flags[MIDIFLAGS_SIZE];
	word version;
	char temp_pwd[30], temp_pwd2[30];
	int16_t temp_pwdkey;
	cvs_MD5Context ctx;
	memset(temp_midi_flags, 0, MIDIFLAGS_SIZE);
	memset(FFCore.quest_format, 0, sizeof(FFCore.quest_format));
	

	
	if(!pfread(tempheader.id_str,sizeof(tempheader.id_str),f))      // first read old header
	{
		Z_message("Unable to read header string\n");
		return qe_invalid;
	}
	
	// check header
	if(strcmp(tempheader.id_str,QH_NEWIDSTR))
	{
		if(strcmp(tempheader.id_str,QH_IDSTR))
		{
			Z_message("Invalid header string:  '%s' (was expecting '%s' or '%s')\n", tempheader.id_str, QH_IDSTR, QH_NEWIDSTR);
			return qe_invalid;
		}
	}
	
	int32_t templatepath_len=0;
	
	tempheader.external_zinfo = false;
	read_zinfo = false;
	if(!strcmp(tempheader.id_str,QH_IDSTR))                      //pre-1.93 version
	{
		byte padding;
		
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&tempheader.zelda_version,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[vZelda] = tempheader.zelda_version;
		
		if(tempheader.zelda_version > ZELDA_VERSION)
		{
			return qe_version;
		}
		
		FFCore.quest_format[vZelda] = tempheader.zelda_version;
		
		if(strcmp(tempheader.id_str,QH_IDSTR))
		{
			return qe_invalid;
		}
		
		if(bad_version(tempheader.zelda_version))
		{
			return qe_obsolete;
		}
		
		if(!p_igetw(&tempheader.internal,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempheader.quest_number,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[qQuestNumber] = tempheader.quest_number;
		
		if(!pfread(&quest_rules[0],2,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_map_count,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[qMapCount] = temp_map_count;
		
		if(!p_getc(&tempheader.old_str_count,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempheader.data_flags[ZQ_TILES],f))
		{
			return qe_invalid;
		}
		
		if(!pfread(temp_midi_flags,4,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempheader.data_flags[ZQ_CHEATS2],f))
		{
			return qe_invalid;
		}
		
		if(!pfread(dummybuf,14,f))
		{
			return qe_invalid;
		}
		
		if(!pfread(&quest_rules[2],2,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&dummybuf,f))
		{
			return qe_invalid;
		}
		
		if(!pfread(tempheader.version,9,f))
		{
			return qe_invalid;
		}
		
		if(!pfread(tempheader.title,sizeof(tempheader.title),f))
		{
			return qe_invalid;
		}
		// These fields are expected to end in null bytes!
		tempheader.title[sizeof(tempheader.title)-1] = 0;
		
		if(!pfread(tempheader.author,sizeof(tempheader.author),f))
		{
			return qe_invalid;
		}
		tempheader.author[sizeof(tempheader.author)-1] = 0;
		
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_pwdkey,f))
		{
			return qe_invalid;
		}
		
		if(!pfread(temp_pwd,30,f))
		{
			return qe_invalid;
		}
	
		get_questpwd(temp_pwd, temp_pwdkey, temp_pwd2);
		cvs_MD5Init(&ctx);
		cvs_MD5Update(&ctx, (const uint8_t*)temp_pwd2, (unsigned)strnlen(temp_pwd2, 30));
		cvs_MD5Final(tempheader.pwd_hash, &ctx);
		
		if(tempheader.zelda_version < 0x177)                       // lacks new header stuff...
		{
			//memset(tempheader.minver,0,20);                          //   char minver[9], byte build, byte foo[10]
			// Not anymore...
			memset(tempheader.minver,0,17);
			tempheader.build=0;
			tempheader.use_keyfile=0;
			memset(tempheader.old_foo, 0, 9);
		}
		else
		{
			if(!pfread(tempheader.minver,9,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempheader.build,f))
			{
				return qe_invalid;
			}
			
			FFCore.quest_format[vBuild] = tempheader.build;
			
			if(!p_getc(&tempheader.use_keyfile,f))
			{
				return qe_invalid;
			}
			
			if(!pfread(dummybuf,9,f))
			{
				return qe_invalid;
			}
		}                                                       // starting at minver
		
		if(tempheader.zelda_version < 0x187)                    // lacks newer header stuff...
		{
			memset(&quest_rules[4],0,16);                          //   word rules3..rules10
		}
		else
		{
			if(!pfread(&quest_rules[4],16,f))                      // read new header additions
			{
				return qe_invalid;                                  // starting at rules3
			}
			
			if(tempheader.zelda_version <= 0x190)
			{
				set_qr(qr_MEANPLACEDTRAPS,0);
			}
		}
		unpack_qrs();
		
		if((tempheader.zelda_version < 0x192)||
				((tempheader.zelda_version == 0x192)&&(tempheader.build<149)))
		{
			set_qr(qr_BRKNSHLDTILES,(get_qr(qr_BRKBLSHLDS_DEP)));
			set_bit(deprecated_rules,qr_BRKBLSHLDS_DEP,(get_qr(qr_BRKBLSHLDS_DEP)));
			set_qr(qr_BRKBLSHLDS_DEP,0);
		}
		
		if(tempheader.zelda_version >= 0x192)                       //  lacks newer header stuff...
		{
			byte *mf=temp_midi_flags;
			
			if((tempheader.zelda_version == 0x192)&&(tempheader.build<178))
			{
				mf=(byte*)dummybuf;
			}
			
			if(!pfread(mf,32,f))                  // read new header additions
			{
				return qe_invalid;                                  // starting at foo2
			}
			
			if(!pfread(dummybuf,18,f))                        // read new header additions
			{
				return qe_invalid;                                  // starting at foo2
			}
		}
		
		if((tempheader.zelda_version < 0x192)||
				((tempheader.zelda_version == 0x192)&&(tempheader.build<145)))
		{
			memset(tempheader.templatepath,0,2048);
		}
		else
		{
			if(!pfread(tempheader.templatepath,280,f))               // read templatepath
			{
				return qe_invalid;
			}
		}
		
		if((tempheader.zelda_version < 0x192)||
				((tempheader.zelda_version == 0x192)&&(tempheader.build<186)))
		{
			tempheader.use_keyfile=0;
		}
	}
	else
	{
		//section id
		if(!p_mgetl(&dummy,f))
		{
			return qe_invalid;
		}
		
		//section version info
		if(!p_igetw(&version,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[vHeader] = version;
		
		if(!p_igetw(&dummy,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&dummy,f))
		{
			return qe_invalid;
		}
		
		//finally...  section data
		if(!p_igetw(&tempheader.zelda_version,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[vZelda] = tempheader.zelda_version;
	
		//do some quick checking...
		if(tempheader.zelda_version > ZELDA_VERSION)
		{
			return qe_version;
		}
		
		if(strcmp(tempheader.id_str,QH_NEWIDSTR))
		{
			return qe_invalid;
		}
		
		if(bad_version(tempheader.zelda_version))
		{
			return qe_obsolete;
		}
		
		if(!p_getc(&tempheader.build,f))
		{
			return qe_invalid;
		}
	
		FFCore.quest_format[vBuild] = tempheader.build;
	
		if(version<3)
		{
			if(!pfread(temp_pwd,30,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_pwdkey,f))
			{
				return qe_invalid;
			}
			
			get_questpwd(temp_pwd, temp_pwdkey, temp_pwd2);
			cvs_MD5Init(&ctx);
			cvs_MD5Update(&ctx, (const uint8_t*)temp_pwd2, (unsigned)strnlen(temp_pwd2, 30));
			cvs_MD5Final(tempheader.pwd_hash, &ctx);
		}
		else
		{
			if(!pfread(tempheader.pwd_hash,sizeof(tempheader.pwd_hash),f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_igetw(&tempheader.internal,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempheader.quest_number,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[qQuestNumber] = tempheader.quest_number;
		
		size_t versz = version < 8 ? 9 : 16;
		if(!pfread(tempheader.version,versz,f))
		{
			return qe_invalid;
		}
	
		//FFCore.quest_format[qQuestVersion] = tempheader.version;
		//needs to be copied as char[9] or stored as a s.str
		if(!pfread(tempheader.minver,versz,f))
		{
			return qe_invalid;
		}
	
		//FFCore.quest_format[qMinQuestVersion] = tempheader.minver;
		if(!pfread(tempheader.title,sizeof(tempheader.title),f))
		{
			return qe_invalid;
		}
		tempheader.title[sizeof(tempheader.title)-1] = 0;
		
		if(!pfread(tempheader.author,sizeof(tempheader.author),f))
		{
			return qe_invalid;
		}
		tempheader.author[sizeof(tempheader.author)-1] = 0;
		
		if(!p_getc(&tempheader.use_keyfile,f))
		{
			return qe_invalid;
		}
		
		/*
		  if(!pfread(tempheader.data_flags,sizeof(tempheader.data_flags),f))
		  {
		  return qe_invalid;
		  }
		  */
		if(!p_getc(&tempheader.data_flags[ZQ_TILES],f))
		{
			return qe_invalid;
		}
		
		if(!pfread(&dummybuf,4,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempheader.data_flags[ZQ_CHEATS2],f))
		{
			return qe_invalid;
		}
		
		if(!pfread(dummybuf,14,f))
		{
			return qe_invalid;
		}
		
		templatepath_len=sizeof(tempheader.templatepath);
		
		if(version==1)
		{
			templatepath_len=280;
		}
		
		if(!pfread(tempheader.templatepath,templatepath_len,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_map_count,f))
		{
			return qe_invalid;
		}
	
		if(version>=4)
		{
			if(!p_igetl(&tempheader.version_major,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.version_minor,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.version_patch,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.new_version_id_fourth,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.new_version_id_alpha,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.new_version_id_beta,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.new_version_id_gamma,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.new_version_id_release,f))
			{
				return qe_invalid;
			}

			// 2.55 alpha 86 incorrectly used 56 as the second version component. Fix that here.
			if (tempheader.version_major == 2 && tempheader.version_minor == 56 && tempheader.new_version_id_alpha == 86)
				tempheader.version_minor = 55;

			if(!p_igetw(&tempheader.new_version_id_date_year,f))
			{
				return qe_invalid;
			}
			if(!p_getc(&tempheader.new_version_id_date_month,f))
			{
				return qe_invalid;
			}
			if(!p_getc(&tempheader.new_version_id_date_day,f))
			{
				return qe_invalid;
			}
			if(!p_getc(&tempheader.new_version_id_date_hour,f))
			{
				return qe_invalid;
			}
			if(!p_getc(&tempheader.new_version_id_date_minute,f))
			{
				return qe_invalid;
			}
					
			if(!pfread(tempheader.new_version_devsig,256,f))
			{
				return qe_invalid;
			}
			if(!strcmp(tempheader.new_version_devsig, "Venrob"))
				strcpy(tempheader.new_version_devsig, "EmilyV99");
			if(!pfread(tempheader.new_version_compilername,256,f))
			{
				return qe_invalid;
			}
			if(!pfread(tempheader.new_version_compilerversion,256,f))
			{
				return qe_invalid;
			}
			if(!pfread(tempheader.product_name,1024,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempheader.compilerid,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.compilerversionnumber_first,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.compilerversionnumber_second,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.compilerversionnumber_third,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempheader.compilerversionnumber_fourth,f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&tempheader.developerid,f))
			{
				return qe_invalid;
			}
			if(!pfread(tempheader.made_in_module_name,1024,f))
			{
				return qe_invalid;
			}
			if(!pfread(tempheader.build_datestamp,256,f))
			{
				return qe_invalid;
			}
			if(!pfread(tempheader.build_timestamp,256,f))
			{
				return qe_invalid;
			}
		}
		else // <4
		{
			tempheader.version_major = 0;
			tempheader.version_minor = 0;
			tempheader.version_patch = 0;
			tempheader.new_version_id_fourth = 0;
			tempheader.new_version_id_alpha = 0;
			tempheader.new_version_id_beta = 0;
			tempheader.new_version_id_gamma = 0;
			tempheader.new_version_id_release = 0;
			tempheader.new_version_id_date_year = 0;
			tempheader.new_version_id_date_month = 0;
			tempheader.new_version_id_date_day = 0;
			tempheader.new_version_id_date_hour = 0;
			tempheader.new_version_id_date_minute = 0;
			
			memset(tempheader.new_version_devsig, 0, 256);
			memset(tempheader.new_version_compilername, 0, 256);
			memset(tempheader.new_version_compilerversion, 0, 256);
			memset(tempheader.product_name, 0, 1024);
			strcpy(tempheader.product_name, "ZQuest Classic");
			
			tempheader.compilerid = 0;
			tempheader.compilerversionnumber_first = 0;
			tempheader.compilerversionnumber_second = 0;
			tempheader.compilerversionnumber_third = 0;
			tempheader.compilerversionnumber_fourth = 0;
			tempheader.developerid = 0;
			
			memset(tempheader.made_in_module_name, 0, 1024);
			memset(tempheader.build_datestamp, 0, 256);
			memset(tempheader.build_timestamp, 0, 256);
		}

		if ( version >= 5 )
		{
			if(!pfread(tempheader.build_timezone,6,f))
			{
				return qe_invalid;
			}
		}
		else // < 5
		{
			memset(tempheader.build_timezone, 0, 6);
		}
		if ( version >= 6 )
		{
			byte b;
			if(!p_getc(&b,f))
			{
				return qe_invalid;
			}
			tempheader.external_zinfo = b?true:false;
			read_zinfo = true;
		}
		
		if(version >= 7)
		{
			if(!p_getc(&(tempheader.new_version_is_nightly),f))
			{
				return qe_invalid;
			}
		}
		else
		{
			tempheader.new_version_is_nightly = false;
			if(tempheader.zelda_version < 0x255)
			{
				switch(tempheader.zelda_version)
				{
					case 0x254:
						tempheader.version_major = 2;
						tempheader.version_minor = 54;
						break;
					case 0x250:
						switch(tempheader.build)
						{
							case 19:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_gamma = 1;
								break;
							case 20:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_gamma = 2;
								break;
							case 21:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_gamma = 3;
								break;
							case 22:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_gamma = 4;
								break;
							case 23:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_gamma = 5;
								break;
							case 24:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.new_version_id_release = -1;
								break;
							case 25:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 1;
								tempheader.new_version_id_gamma = 1;
								break;
							case 26:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 1;
								tempheader.new_version_id_gamma = 2;
								break;
							case 27: 
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 1;
								tempheader.new_version_id_gamma = 3;
								break;
							case 28:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 1;
								tempheader.new_version_id_release = -1;
								break;
							case 29:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 2;
								tempheader.new_version_id_release = -1;
								break;
							case 30:
								tempheader.version_major = 2;
								tempheader.version_minor = 50;
								tempheader.version_patch = 3;
								tempheader.new_version_id_gamma = 1;
								break;
							case 31:
								tempheader.version_major = 2;
								tempheader.version_minor = 53;
								tempheader.new_version_id_gamma = -1;
								break;
							case 32:
								tempheader.version_major = 2;
								tempheader.version_minor = 53;
								tempheader.new_version_id_release = -1;
								break;
							case 33:
								tempheader.version_major = 2;
								tempheader.version_minor = 53;
								tempheader.version_patch = 1;
								break;
						}
						break;
					
					case 0x211:
						tempheader.version_major = 2;
						tempheader.version_minor = 11;
						tempheader.new_version_id_beta = tempheader.build;
						break;
					case 0x210:
						tempheader.version_major = 2;
						tempheader.version_minor = 10;
						tempheader.new_version_id_beta = tempheader.build;
						break;
				}
			}
		}

		if (version>=9)
		{
			std::string version_string;
			if(!p_getcstr(&version_string, f))
			{
				return qe_invalid;
			}

			strncpy(tempheader.zelda_version_string, version_string.c_str(), sizeof(tempheader.zelda_version_string));
			snprintf(tempheader.zelda_version_string, sizeof(tempheader.zelda_version_string), "%s", version_string.c_str());
		}
		else
		{
			snprintf(tempheader.zelda_version_string, sizeof(tempheader.zelda_version_string), "%d.%d.%d", tempheader.version_major, tempheader.version_minor, tempheader.version_patch);
		}
	}

	if(printmetadata || __isZQuest)
	{
		print_quest_metadata(tempheader, loading_qst_name, loading_qst_num);
	}
	
	//{ Version Warning
	int32_t vercmp = tempheader.compareVer();
	int32_t astatecmp = compare(int32_t(tempheader.getAlphaState()), getAlphaState());
	int32_t avercmp = compare(tempheader.getAlphaVer(), 0);
	if(vercmp > 0 || (!vercmp &&
		(astatecmp > 0 || (!astatecmp &&
			avercmp > 0))))
	{
		bool r = true;
		if(loadquest_report)
		{
			enter_sys_pal();
			AlertDialog("Quest saved in newer version",
				"This quest was last saved in a newer version of ZQuest."
				" Attempting to load this quest may not work correctly; to"
				" avoid issues, try loading this quest in at least '" + std::string(tempheader.getVerStr()) + "'"
				"\n\nWould you like to continue loading anyway? (Not recommended)",
				[&](bool ret,bool)
				{
					r = ret;
				}).show();
			exit_sys_pal();
		}
		if(!r)
			return qe_silenterr;
	}
	else if(tempheader.compareDate() > 0)
	{
		bool r = true;
		if(loadquest_report)
		{
			enter_sys_pal();
			AlertDialog("Quest saved in newer build",
				fmt::format("This quest was last saved in a newer build of ZQuest, and may have"
					" issues loading in this build."
					"\n{}"
					"\n\nWould you like to continue loading anyway?",
					tempheader.getVerCmpStr()),
				[&](bool ret,bool)
				{
					r = ret;
				}).show();
			exit_sys_pal();
		}
		if(!r)
			return qe_silenterr;
	}
	//}
	
	read_ext_zinfo = tempheader.external_zinfo;
	
	memcpy(Header, &tempheader, sizeof(tempheader));
	map_count=temp_map_count;
	memcpy(midi_flags, temp_midi_flags, MIDIFLAGS_SIZE);

	unpack_qrs();

	return 0;
}

int32_t readrules(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_rules);
	if (should_skip)
		return 0;

	int32_t dummy;
	zquestheader tempheader;
	word s_version=0;
	dword compatrule_version=0;
	
	memcpy(&tempheader, Header, sizeof(tempheader));
	
	if(tempheader.zelda_version >= 0x193)
	{
		//section version info
		if(!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}
	
		FFCore.quest_format[vRules] = s_version;
		
		if(!p_igetw(&dummy,f))
		{
			return qe_invalid;
		}
		
		if(s_version > 16)
		{
			if(!p_igetl(&compatrule_version,f))
			{
				return qe_invalid;
			}
		}
		FFCore.quest_format[vCompatRule] = compatrule_version;
		
		//section size
		if(!p_igetl(&dummy,f))
		{
			return qe_invalid;
		}
		
		if ( s_version < 15 )
		{
			//finally...  section data
			if(!pfread(quest_rules,QUESTRULES_SIZE,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			
			if(!pfread(quest_rules,QUESTRULES_NEW_SIZE,f))
			{
				return qe_invalid;
			}
			
		}
	}

	//{ bunch of compat stuff
	memcpy(deprecated_rules, quest_rules, QUESTRULES_NEW_SIZE);

	unpack_qrs();
	
	if(s_version<2)
	{
		set_qr(14,0);
		set_qr(27,0);
		set_qr(28,0);
		set_qr(29,0);
		set_qr(30,0);
		set_qr(32,0);
		set_qr(36,0);
		set_qr(49,0);
		set_qr(50,0);
		set_qr(51,0);
		set_qr(68,0);
		set_qr(75,0);
		set_qr(76,0);
		set_qr(98,0);
		set_qr(110,0);
		set_qr(113,0);
		set_qr(116,0);
		set_qr(102,0);
		set_qr(132,0);
	}
	
	//Now, do any updates...
	if((tempheader.zelda_version < 0x211)||((tempheader.zelda_version == 0x211)&&(tempheader.build<18)))
	{
		set_qr(qr_SMOOTHVERTICALSCROLLING,1);
		set_qr(qr_REPLACEOPENDOORS, 1);
		set_qr(qr_OLDLENSORDER, 1);
		set_qr(qr_NOFAIRYGUYFIRES, 1);
		set_qr(qr_TRIGGERSREPEAT, 1);
	}
	
	if((tempheader.zelda_version < 0x193)||((tempheader.zelda_version == 0x193)&&(tempheader.build<3)))
	{
		set_qr(qr_WALLFLIERS,1);
	}
	
	if((tempheader.zelda_version < 0x193)||((tempheader.zelda_version == 0x193)&&(tempheader.build<4)))
	{
		set_qr(qr_NOBOMBPALFLASH,1);
	}
	
	if((tempheader.zelda_version < 0x193)||((tempheader.zelda_version == 0x193)&&(tempheader.build<3)))
	{
		set_qr(qr_NOSCROLLCONTINUE,1);
	}
	
	if(tempheader.zelda_version <= 0x210)
	{
		set_qr(qr_ARROWCLIP,1);
	}
	
	if(tempheader.zelda_version == 0x210)
	{
		set_qr(qr_NOSCROLLCONTINUE, get_qr(qr_CMBCYCLELAYERS));
		set_qr(qr_CMBCYCLELAYERS, 0);
		set_qr(qr_CONT_SWORD_TRIGGERS, 1);
	}
	
	if(tempheader.zelda_version <= 0x210)
	{
		set_qr(qr_OLDSTYLEWARP,1);
		set_qr(qr_210_WARPRETURN,1);
	}
	
	//might not be correct
	if(tempheader.zelda_version < 0x210)
	{
		set_bit(deprecated_rules, qr_OLDTRIBBLES_DEP,1);
		set_qr(qr_OLDTRIBBLES_DEP,1);
		set_qr(qr_OLDHOOKSHOTGRAB,1);
	}
	
	if(tempheader.zelda_version < 0x211)
	{
		set_qr(qr_WRONG_BRANG_TRAIL_DIR,1);
	}
	
	if((tempheader.zelda_version == 0x192 && tempheader.build <= 163) || tempheader.zelda_version < 0x192)
	{
		set_qr(qr_192b163_WARP,1);
	}
	
	if(tempheader.zelda_version == 0x210)
	{
		set_bit(deprecated_rules, qr_OLDTRIBBLES_DEP, get_qr(qr_DMGCOMBOPRI));
		set_qr(qr_OLDTRIBBLES_DEP, get_qr(qr_DMGCOMBOPRI));
		set_qr(qr_DMGCOMBOPRI, 0);
	}
	
	if(tempheader.zelda_version < 0x211 || (tempheader.zelda_version == 0x211 && tempheader.build<15))
	{
		set_qr(qr_OLDPICKUP,1);
	}
	
	if(tempheader.zelda_version < 0x211 || (tempheader.zelda_version == 0x211 && tempheader.build < 18))
	{
		set_qr(qr_NOSOLIDDAMAGECOMBOS, 1);
		set_qr(qr_ITEMPICKUPSETSBELOW, 1); // broke around build 400
	}
	
	if(tempheader.zelda_version < 0x250) // version<0x250 checks for beta 18; build was set to 18 prematurely
	{
		set_qr(qr_HOOKSHOTDOWNBUG, 1);
	}
	
	if(tempheader.zelda_version == 0x250 && tempheader.build == 24) // Annoying...
	{
		set_qr(qr_PEAHATCLOCKVULN, 1);
	}
	
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build < 22)) //22 is 2.50.0 RC4. Gotta set the door repair QR... -Dimi
	{
		set_qr(qr_OLD_DOORREPAIR, 1);
	}
	
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build < 20)) //20 is 2.50.0 RC1 and RC2 (cause it didn't get bumped). Okay I'm gonna be honest I have no idea if any 2.50 build was available before RC1, but gonna try and cover my ass here -Dimi
	{
		set_qr(qr_OLD_SECRETMONEY, 1);
	}
	
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build < 28)) //28 is 2.50.1 final. Potion bug might have been used, I dunno. -Dimi
	{
		set_qr(qr_OLD_POTION_OR_HC, 1);
	}
	
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build<28))
	{
		set_qr(qr_OFFSCREENWEAPONS, 1);
	}
	
	//Bombchu fix.
	if(tempheader.zelda_version == 0x250)
	{
		if ( tempheader.build == 24 ) //2.50.0
		{
			set_qr(qr_BOMBCHUSUPERBOMB, 1);
		}
		if ( tempheader.build == 28 ) //2.50.1
		{
			set_qr(qr_BOMBCHUSUPERBOMB, 1);
		}
		if ( tempheader.build == 29 ) //2.50.2
		{
			set_qr(qr_BOMBCHUSUPERBOMB, 0);
		}
		if ( tempheader.build == 30 ) //2.50.3RC1
		{
			set_qr(qr_BOMBCHUSUPERBOMB, 0);
		}
	}
	
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build<29))
	{
		// qr_OFFSETEWPNCOLLISIONFIX
		// All 'official' quests need this disabled.
		// All 2.10 and lower quests need this enabled to preseve compatability.
		// All 2.11 - 2.5.1 quests should have it set also, due to a bug in about half of all the betas.

		//~Gleeok
		set_qr(qr_OFFSETEWPNCOLLISIONFIX, 1); //This has to be set!!!!
		
		// Broke in build 695
		if(tempheader.zelda_version>=0x211 && tempheader.build>=18)
			set_qr(qr_BROKENSTATUES, 1);
	}
	if (tempheader.zelda_version <= 0x190 || (tempheader.zelda_version == 0x192 && std::string(tempheader.title).starts_with("Zelda 3000\0")))
	{
		set_qr(qr_COPIED_SWIM_SPRITES, 1);
	}
	if ( (tempheader.zelda_version == 0x250 && tempheader.build < 33) || tempheader.zelda_version == 0x254 || tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x255 && tempheader.build < 50) )
	{
		set_qr(qr_OLD_SLASHNEXT_SECRETS, 1);
	}
	
	if ( (tempheader.zelda_version < 0x211) ) //2.10 water and ladder interaction
	{
		set_qr(qr_OLD_210_WATER, 1);
	}
	
	if ( (tempheader.zelda_version < 0x255 ) || (tempheader.zelda_version == 0x255 &&  tempheader.build < 51 ) ) //2.10 water and ladder interaction
	{
		set_qr(qr_STEP_IS_FLOAT,0);
	}
	
	if ( tempheader.zelda_version < 0x250 ) 
	{
		set_qr(qr_8WAY_SHOT_SFX_DEP, 1);		
	}
	
	if(s_version < 3)
	{
		set_qr(qr_HOLDNOSTOPMUSIC, 1);
		set_qr(qr_CAVEEXITNOSTOPMUSIC, 1);
	}
	
	if(s_version<4)
	{
		set_qr(10,0);
	}
	
	if(s_version<5)
	{
		set_qr(27,0);
	}
	
	if(s_version<6)
	{
		set_qr(46,0);
	}
	
	if(s_version<7) // January 2008
	{
		set_qr(qr_HEARTSREQUIREDFIX,0);
		set_qr(qr_PUSHBLOCKCSETFIX,1);
	}
	
	if(s_version<8)
	{
		set_qr(12, 0);
	}
	else
	{
		set_bit(deprecated_rules, 12, 0);
	}
	
	if(s_version<9) // October 2008
	{
		set_qr(qr_NOROPE2FLASH_DEP,0);
		set_qr(qr_NOBUBBLEFLASH_DEP,0);
		set_qr(qr_GHINI2BLINK_DEP,0);
		set_qr(qr_PHANTOMGHINI2_DEP,0);
	}
	
	if(s_version<10) // December 2008
	{
		set_qr(qr_NOCLOCKS_DEP,0);
		set_qr(qr_ALLOW10RUPEEDROPS_DEP,0);
	}
	
	if(s_version<11) // April 2009
	{
		set_qr(qr_SLOWENEMYANIM_DEP,0);
	}
	
	// This served no purpose.
	// if(s_version<12)  // December 2009
	// {
	// 	set_qr(qr_BRKBLSHLDS_DEP,0);
	// 	set_qr(qr_OLDTRIBBLES_DEP,0);
	// }
	
	//if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build < 24))
	if(s_version < 13)
	{
		set_qr(qr_SHOPCHEAT, 1);
	}
	
	// Not entirely sure this is the best place for this...
	//2.50.2 bitmap offset fix
	memset(extra_rules, 0, EXTRARULES_SIZE);
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build<29))
	{
		set_er(er_BITMAPOFFSET, 1);
		set_qr(qr_BITMAPOFFSETFIX, 1);
	}
	//required because quest templates also used this bit, although
	//it never did anything, before. -Z
	if ( tempheader.zelda_version == 0x250 )
	{
		if( tempheader.build == 29 || tempheader.build == 30 || tempheader.build == 31 )
		{
			set_er(er_BITMAPOFFSET, 0);
			set_qr(qr_BITMAPOFFSETFIX, 0);    
		}
	}
	if ( tempheader.zelda_version == 0x254 )
	{
		set_er(er_BITMAPOFFSET, 0);
		set_qr(qr_BITMAPOFFSETFIX, 0);    
	}
	if ( tempheader.zelda_version == 0x255 && tempheader.build < 42 ) //QR was added to 255 in this build.
	{
		set_er(er_BITMAPOFFSET, 0);
		set_qr(qr_BITMAPOFFSETFIX, 0);    
	}
	//optimise fast drawing for older versions.
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 42) )
	{
		set_qr(qr_OLDSPRITEDRAWS, 1);    
	}
	//Old eweapon->Parent (was added in 2.54, Alpha 19)
	//The change was made in build 43, but I'm setting this to < 42, because quests made in 42 would benefit from this change, and
	//older quests can set the rule by hand. We need a new qst.dat again.
	if ( tempheader.zelda_version == 0x254 || (tempheader.zelda_version == 0x255 && tempheader.build < 42) )
	{
		set_qr(qr_OLDEWPNPARENT, 1);    
	}
	if ( tempheader.zelda_version == 0x254 || (tempheader.zelda_version == 0x255 && tempheader.build < 44) )
	{
		set_qr(qr_OLDCREATEBITMAP_ARGS, 1);    
	}
	if ( tempheader.zelda_version == 0x254 || (tempheader.zelda_version == 0x255 && tempheader.build < 45) )
	{
		set_qr(qr_OLDQUESTMISC, 1);    
	}
	if ( tempheader.zelda_version < 0x254 )
	{
		set_qr(qr_OLDCREATEBITMAP_ARGS, 0);  
		set_qr(qr_OLDEWPNPARENT, 0); 	    
		set_qr(qr_OLDQUESTMISC, 0); 	    
	}
	
	//item scripts continue to run
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 44) )
	{
		set_qr(qr_ITEMSCRIPTSKEEPRUNNING, 0);  	    
		set_qr(qr_SCRIPTSRUNINHEROSTEPFORWARD, 0);  	    
		set_qr(qr_FIXSCRIPTSDURINGSCROLLING, 0);
		set_qr(qr_SCRIPTDRAWSINWARPS, 0);  	    
		set_qr(qr_DYINGENEMYESDONTHURTHERO, 0);  	    
		set_qr(qr_OUTOFBOUNDSENEMIES, 0);  
		set_qr(qr_SPRITEXY_IS_FLOAT, 0);
	}
	
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 46) )
	{
		set_qr(qr_CLEARINITDONSCRIPTCHANGE, 1);  	    
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 46) )
	{
		set_qr(qr_TRACESCRIPTIDS, 0);      
		set_qr(qr_SCRIPT_FRIENDLY_ENEMY_TYPES, 1);      
		set_qr(qr_PARSER_BOOL_TRUE_DECIMAL, 1);   
		set_qr(qr_PARSER_250DIVISION,1);
		set_qr(qr_PARSER_BOOL_TRUE_DECIMAL,1);
		set_qr(qr_PARSER_TRUE_INT_SIZE,0);
		set_qr(qr_PARSER_FORCE_INLINE,0);
		set_qr(qr_PARSER_BINARY_32BIT,0);
		if ( get_qr(qr_SELECTAWPN) ) 
		{
			set_qr(qr_NO_L_R_BUTTON_INVENTORY_SWAP,1); 
			//In < 2.55a27, if you had an A+B subscreen, L and R didn't shift through inventory.
			//Now they **do**, unless you disable that behaviour.
			//For the sake of compatibility, old quests with the A+B subscreen rule enabed
			//now enable the disable L/R item swap on load.
		}
			
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 47) )
	{
		//Compatibility: Setting the hero's action to rafting was previously disallowed, though legal for scripts to attempt.
		set_qr(qr_DISALLOW_SETTING_RAFTING, 1);
		//Compatibility: The calculation for when to loop an animation did not factor in ASkipY correctly, resulting in
		//animations ending earlier than they should.
		set_qr(qr_BROKEN_ASKIP_Y_FRAMES, 1);
		//Enemies would ignore solidity on the top half of combos
		set_qr(qr_ENEMY_BROKEN_TOP_HALF_SOLIDITY, 1);
		//Ceiling collison was a bit wonky, including hitting your head before you are near the ceiling or clipping into it slightly.
		set_qr(qr_OLD_SIDEVIEW_CEILING_COLLISON, 1);
		//If an itemdata had a 'frames' of 0, items created of that data would ignore all changes to 'frames'
		set_qr(qr_0AFRAME_ITEMS_IGNORE_AFRAME_CHANGES, 1);
		//Collision used some odd calculations before, and enemies could not be hit back into the top row or left column
		set_qr(qr_OLD_ENEMY_KNOCKBACK_COLLISION, 1);
	}
	if ( tempheader.zelda_version < 0x255 )
	{
		set_qr(qr_NOFFCWAITDRAW, 1);  
		set_qr(qr_NOITEMWAITDRAW, 1);  
		set_qr(qr_SETENEMYWEAPONSPRITESONWPNCHANGE, 1); 
		set_qr(qr_OLD_INIT_SCRIPT_TIMING, 1);
		//set_qr(qr_DO_NOT_DEALLOCATE_INIT_AND_SAVELOAD_ARRAYS, 1);
	}
	if ( tempheader.zelda_version < 0x255 || ( tempheader.zelda_version == 0x255 && tempheader.build < 48 ) )
	{
		set_qr(qr_SETENEMYWEAPONSPRITESONWPNCHANGE, 1);  
	}
	if( tempheader.zelda_version < 0x255 || ( tempheader.zelda_version == 0x255 && tempheader.build < 52 ) )
	{
		set_qr(qr_OLD_PRINTF_ARGS, 1);
	}
	
	
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 54) )
	{
		set_qr(qr_BROKEN_RING_POWER, 1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 56) )
	{
		set_qr(qr_NO_OVERWORLD_MAP_CHARTING, 1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 57) )
	{
		set_qr(qr_DUNGEONS_USE_CLASSIC_CHARTING, 1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 58) )
	{
		//Rule used to be 'qr_SETXYBUTTONITEMS', now split.
		if(get_qr(qr_SET_XBUTTON_ITEMS))
			set_qr(qr_SET_YBUTTON_ITEMS,1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 59) )
	{
		set_qr(qr_ALLOW_EDITING_COMBO_0,1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 60) )
	{
		set_qr(qr_OLD_CHEST_COLLISION,1);
	}
	
	if ( tempheader.zelda_version < 0x254 )
	{
		set_qr(qr_250WRITEEDEFSCRIPT, 1);  
	}
	//Sideview spikes in 2.50.0
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build<27)) //2.50.1RC3
	{
		set_qr(qr_OLDSIDEVIEWSPIKES, 1);
	}
	//more 2.50 fixes -Z
	if(tempheader.zelda_version < 0x250 || (tempheader.zelda_version == 0x250 && tempheader.build<31))
	{
		set_qr(qr_MELEEMAGICCOST, 0);
		set_qr(qr_GANONINTRO, 0); //This will get flipped later on in the compatrule 11 check. That's why it's turning it off.
		set_qr(qr_OLDMIRRORCOMBOS, 1);
		set_qr(qr_BROKENBOOKCOST, 1);
		set_qr(qr_BROKENCHARINTDRAWING, 1);
	
	}
	if(tempheader.zelda_version == 0x254 && tempheader.build<41)
	{
		//set_qr(qr_MELEEMAGICCOST, get_er(er_MAGICCOSTSWORD));
		set_qr(qr_MELEEMAGICCOST, 1);
	}
	
	if(tempheader.zelda_version < 0x193)
	{
		set_qr(qr_SHORTDGNWALK, 1);
	}
	
	if(tempheader.zelda_version < 0x255)
	{
		set_qr(qr_OLDINFMAGIC, 1);
	}
	
	if((tempheader.zelda_version < 0x250)) //2.10 and earlier allowed the triforce to Warp Hero out of Item Cellars in Dungeons. -Z (15th March, 2019 )
	{
		set_qr(qr_SIDEVIEWTRIFORCECELLAR,1);
	}
	
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 47) )
	{
		set_qr(qr_OLD_F6,1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 49) )
	{
		set_qr(qr_NO_OVERWRITING_HOPPING,1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 50) )
	{
		set_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT,1);
	}
	if ( tempheader.zelda_version < 0x255 || (tempheader.zelda_version == 0x255 && tempheader.build < 53) )
	{
		set_qr(qr_BROKEN_OVERWORLD_MINIMAP,1);
	}
	//}
	
	if(compatrule_version < 1) //Enemies->Secret only affects flag 16-31
		set_qr(qr_ENEMIES_SECRET_ONLY_16_31,1);
	
	if(compatrule_version < 2) //Old CSet2 Handling
		set_qr(qr_OLDCS2,1);
	
	if(compatrule_version < 3) //Hardcoded Shadow/Spawn/Death anim frames
		set_qr(qr_HARDCODED_ENEMY_ANIMS,1);
	
	if(compatrule_version < 4) //Hardcoded Shadow/Spawn/Death anim frames
		set_qr(qr_OLD_ITEMDATA_SCRIPT_TIMING,1);
	
	if(compatrule_version < 5 && tempheader.zelda_version >= 0x250) //Hardcoded Shadow/Spawn/Death anim frames
		set_qr(qr_NO_LANMOLA_RINGLEADER,1);
	
	if(compatrule_version < 6) //Step->Secret (Temp) only affects flag 16-31
		set_qr(qr_STEPTEMP_SECRET_ONLY_16_31,1);
	
	if(compatrule_version < 7) //'Hit All Triggers->Perm Secret' doesn't trigger temp secrets
		set_qr(qr_ALLTRIG_PERMSEC_NO_TEMP,1);
	
	if(compatrule_version < 8) //Hardcoded LItem/Bomb/Clock/Magic Tile Mods
		set_qr(qr_HARDCODED_LITEM_LTMS,1);
	
	if(compatrule_version < 9)
	{
		//Hardcoded BS Patras
		set_qr(qr_HARDCODED_BS_PATRA,1);
		//Hardcoded Patra Inner Eye offsets
		set_qr(qr_PATRAS_USE_HARDCODED_OFFSETS,1);
		//Broken 'Big enemy' animation style
		set_qr(qr_BROKEN_BIG_ENEMY_ANIMATION,1);
		//Broken Attribute 31/32
		set_qr(qr_BROKEN_ATTRIBUTE_31_32,1);
	}
	
	if(compatrule_version < 10) //Shared candle use limits
		set_qr(qr_CANDLES_SHARED_LIMIT,1);
	
	if(compatrule_version < 11) //No cross-screen return points
		set_qr(qr_OLD_RESPAWN_POINTS,1);

	if(compatrule_version < 12)
	{
		//Old fire trail duration
		set_qr(qr_OLD_FLAMETRAIL_DURATION,1);
		//Old Intro String in Ganon Room Behavior
		if(get_qr(qr_GANONINTRO)) set_qr(qr_GANONINTRO,0);
		else set_qr(qr_GANONINTRO,1);
	}
	
	if(compatrule_version < 13 && tempheader.zelda_version >= 0x255) //ANone doesn't reset to originaltile
		set_qr(qr_ANONE_NOANIM,1);
	
	if(compatrule_version < 14) //Old Bridge Combo Behavior
		set_qr(qr_OLD_BRIDGE_COMBOS,1);
	
	if(compatrule_version < 15) //Broken Z3 Animation
		set_qr(qr_BROKEN_Z3_ANIMATION,1);
	
	if(compatrule_version < 16) //Old Enemy Tile Behavior with Animation (None) Enemies
		set_qr(qr_OLD_TILE_INITIALIZATION,1);
	
	if(compatrule_version < 17)
	{
		//Old Quake/DrawYOffset behavior
		//set_qr(qr_OLD_DRAWOFFSET,1);
		//I'm leaving this commented cause I doubt it'll break anything and I think the bugfix might be appreciated in older versions.
		//On the offchance that it *does* break old quests, fixing it is as simple as uncommenting the set_bit above.
	}
	
	if(compatrule_version < 18)
	{
		//Broken DrawScreen Derivative Functions
		set_qr(qr_BROKEN_DRAWSCREEN_FUNCTIONS,1);
		//Scrolling Cancels Charge
		set_qr(qr_SCROLLING_KILLS_CHARGE,1);
	}
	
	if(compatrule_version < 19) //Broken Enemy Item Carrying with Large Enemies
		set_qr(qr_BROKEN_ITEM_CARRYING,1);
	
	if(compatrule_version < 20)
		set_qr(qr_CUSTOMWEAPON_IGNORE_COST,1);
	
	if(compatrule_version < 21)
	{
		set_qr(qr_LEEVERS_DONT_OBEY_STUN,1);
		set_qr(qr_GANON_CANT_SPAWN_ON_CONTINUE,1);
		set_qr(qr_WIZZROBES_DONT_OBEY_STUN,1);
		set_qr(qr_OLD_BUG_NET,1);
		set_qr(qr_MANHANDLA_BLOCK_SFX,1);
	}
	
	if(compatrule_version < 22)
		set_qr(qr_BROKEN_KEEPOLD_FLAG,1);
	
	if(compatrule_version < 23)
		set_qr(qr_OLD_HALF_MAGIC,1);
	
	if(compatrule_version < 24)
	{
		set_qr(qr_WARPS_RESTART_DMAPSCRIPT,1);
		set_qr(qr_DMAP_0_CONTINUE_BUG,1);
	}
	
	if(compatrule_version < 25)
	{
		if (get_qr(qr_OLD_FAIRY_LIMIT)) set_qr(qr_OLD_FAIRY_LIMIT,0);
		else set_qr(qr_OLD_FAIRY_LIMIT,1);
		set_qr(qr_OLD_SCRIPTED_KNOCKBACK,1);
	}
	if(compatrule_version < 26)
	{
		set_qr(qr_OLD_KEESE_Z_AXIS,1);
		set_qr(qr_POLVIRE_NO_SHADOW,1);
		set_qr(qr_SUBSCR_OLD_SELECTOR,1);
	}
	if(compatrule_version < 27) //Noticed some junk data in the QR array...
	{
		for(auto q = qr_POLVIRE_NO_SHADOW+1; q < qr_PARSER_250DIVISION; ++q)
			set_qr(q,0);
		for(auto q = qr_COMBODATA_INITD_MULT_TENK+1; q < qr_MAX; ++q)
			set_qr(q,0);
		//This should nuke any remaining junk data... not sure if it affected anything previous. -Em
	}
	if(compatrule_version < 28)
		set_qr(qr_SUBSCR_BACKWARDS_ID_ORDER,1);
	if(compatrule_version < 29)
		set_qr(qr_OLD_LOCKBLOCK_COLLISION,1);
	if(compatrule_version < 30)
	{
		set_qr(qr_DECO_2_YOFFSET,1);
		set_qr(qr_SCREENSTATE_80s_BUG,1);
	}
	if(compatrule_version < 31)
	{
		set_qr(qr_GOHMA_UNDAMAGED_BUG,1);
		set_qr(qr_FFCPRELOAD_BUGGED_LOAD,1);
	}
	if(compatrule_version < 32)
		set_qr(qr_BROKEN_GETPIXEL_VALUE,1);
	if(compatrule_version < 33)
		set_qr(qr_NO_LIFT_SPRITE,1);
	if(compatrule_version < 34)
	{
		set_qr(qr_OLD_SIDEVIEW_LANDING_CODE,1);
		set_qr(qr_OLD_FFC_SPEED_CAP,1);
		set_qr(qr_OLD_FFC_FUNCTIONALITY,1);
		set_qr(qr_OLD_WIZZROBE_SUBMERGING,1);
	}
	if(compatrule_version < 35)
	{
		set_qr(qr_ZS_NO_NEG_ARRAY,1);
		set_qr(qr_BROKEN_INPUT_DOWN_STATE,1);
	}
	if(compatrule_version < 36)
		set_qr(qr_OLD_SHALLOW_SFX,1);
	if(compatrule_version < 37)
		set_qr(qr_SPARKLES_INHERIT_PROPERTIES,1);
	if(compatrule_version < 38)
		set_qr(qr_BUGGED_LAYERED_FLAGS,1);
	if(compatrule_version < 39)
		set_qr(qr_HARDCODED_FFC_BUSH_DROPS,1);
	if(compatrule_version < 40)
		set_qr(qr_MOVINGBLOCK_FAKE_SOLID,1);
	if(compatrule_version < 41)
		set_qr(qr_BROKENHITBY,1);
	if(compatrule_version < 42)
		set_qr(qr_BROKEN_MOVING_BOMBS,1);
	if(compatrule_version < 43)
		set_qr(qr_OLD_BOMB_HITBOXES,1);
	if(compatrule_version < 44)
		set_qr(qr_SCROLLWARP_NO_RESET_FRAME,1);
	if(compatrule_version < 45)
		set_qr(qr_ENEMIES_DONT_SCRIPT_FIRST_FRAME,1);
	if(compatrule_version < 46)
		set_qr(qr_BROKEN_RAFT_SCROLL,1);
	if(compatrule_version < 47)
	{
		set_qr(qr_SENSITIVE_SOLID_DAMAGE,1);
		set_qr(qr_OLD_CONVEYOR_COLLISION,1);
	}
	if(compatrule_version < 48)
		set_qr(qr_OLD_GUY_HANDLING,1);
	if(compatrule_version < 49)
		set_qr(qr_FAIRY_FLAG_COMPAT,1);
	if(compatrule_version < 50)
		set_qr(qr_OLD_LENS_LAYEREFFECT,1);
	if(compatrule_version < 51)
		set_qr(qr_PUSHBLOCK_SPRITE_LAYER,1);
	if (compatrule_version < 52)
		set_qr(qr_OLD_SCRIPT_VOLUME, 1);
	if(compatrule_version < 53)
	{
		set_qr(qr_OLD_SUBSCR,1);
		set_qr(qr_ITM_0_INVIS_ON_BTNS,1);
		set_qr(qr_OLD_GAUGE_TILE_LAYOUT,1);
	}
	if(compatrule_version < 54)
		set_qr(qr_WALKTHROUGHWALL_NO_DOORSTATE,1);
	if(compatrule_version < 55)
		set_qr(qr_SPOTLIGHT_IGNR_SOLIDOBJ,1);
	if(compatrule_version < 56)
		set_qr(qr_BROKEN_LIGHTBEAM_HITBOX,1);
	if(compatrule_version < 57)
		set_qr(qr_BROKEN_SWORD_SPIN_TRIGGERS,1);
	if(compatrule_version < 58)
		set_qr(qr_OLD_DMAP_INTRO_STRINGS,1);
	if(compatrule_version < 59)
		set_qr(qr_SCRIPT_CONTHP_IS_HEARTS,1);
	if(compatrule_version < 60)
		set_qr(qr_SEPARATE_BOMBABLE_TAPPING_SFX,1);
	if(compatrule_version < 61)
		set_qr(qr_BROKEN_BOMB_AMMO_COSTS,1);
	if(compatrule_version < 62)
		set_qr(qr_OLD_BROKEN_WARPEX_MUSIC,1);
	if(compatrule_version < 63)
	{
		set_qr(qr_BROKEN_LIFTSWIM,1);
		set_qr(qr_BROKEN_GENERIC_PUSHBLOCK_LOCKING,1);
	}
	if(compatrule_version < 64)
		set_qr(qr_BROKEN_FLAME_ARROW_REFLECTING,1);
	if(compatrule_version < 65)
		set_qr(qr_BROKEN_SIDEVIEW_SPRITE_JUMP,1);
	if(compatrule_version < 66)
		set_qr(qr_NEWDARK_TRANS_STACKING,1);
	if(compatrule_version < 67)
		set_qr(qr_OLD_HERO_WARP_RETSQUARE,1);
	if(compatrule_version < 68)
		set_qr(qr_SCRIPTS_6_BIT_COLOR,1);
	if(compatrule_version < 69)
		set_qr(qr_SETENEMYWEAPONSOUNDSONWPNCHANGE, 1);
	if (compatrule_version < 70)
		set_qr(qr_BROKEN_CONVEYORS, 1);
	if (compatrule_version < 71)
		set_qr(qr_BROKEN_PUSHBLOCK_TOP_HALF_SOLIDS, 1);
	if (compatrule_version < 72)
		set_qr(qr_BROKEN_PUSHBLOCK_FLAG_CLONING, 1);
	if (compatrule_version < 73)
	{
		set_qr(qr_OLD_LANDING_SFX, 1);
		set_qr(qr_FIRE_LEVEL_TRIGGERS_ARENT_WEAPONS, 1);
	}
	
	set_qr(qr_ANIMATECUSTOMWEAPONS,0);
	if (s_version < 16)
		set_qr(qr_BROKEN_HORIZONTAL_WEAPON_ANIM,1);
	
	memcpy(Header, &tempheader, sizeof(tempheader));
	
	return 0;
}

void init_msgstr(MsgStr *str)
{
	str->s = "";
	str->s.shrink_to_fit();
	str->nextstring=0;
    str->tile=0;
    str->cset=0;
    str->trans=false;
    str->font=font_zfont;
    str->y=32;
    str->sfx=18;
    str->listpos=0;
    str->x=24;
	str->w=get_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT)!=0 ? 24*8 : 26*8;
	str->h=get_qr(qr_STRING_FRAME_OLD_WIDTH_HEIGHT)!=0 ? 3*8 : 5*8;
    str->hspace=0;
    str->vspace=0;
    str->stringflags=0;
	str->margins[up] = 8;
	str->margins[down] = 0;
	str->margins[left] = 8;
	str->margins[right] = 8;
	str->portrait_tile = 0;
	str->portrait_cset = 0;
	str->portrait_x = 0;
	str->portrait_y = 0;
	str->portrait_tw = 1;
	str->portrait_th = 1;
	str->shadow_type = 0;
	str->shadow_color = 0;
	str->drawlayer = 6;
}

void init_msgstrings(int32_t start, int32_t end)
{
    if(end <= start || end-start > msg_strings_size)
        return;
        
    for(int32_t i=start; i<end; i++)
    {
        init_msgstr(&MsgStrings[i]);
        MsgStrings[i].listpos=i;
    }
    
    if(start==0)
    {
		MsgStrings[0].s = "(None)";
        MsgStrings[0].listpos = 0;
    }
}

int32_t readstrings(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_strings);

	MsgStr tempMsgString;
	init_msgstr(&tempMsgString);
	
	word temp_msg_count=0;
	word temp_expansion[16];
	memset(temp_expansion, 0, 16*sizeof(word));
	char buf[8193] = {0};
	if(Header->zelda_version < 0x193)
	{
		byte tempbyte;
		int32_t strings_to_read=0;
		if (!should_skip)
			set_qr(qr_OLD_STRING_EDITOR_MARGINS,true);
		if((Header->zelda_version < 0x192)||
			((Header->zelda_version == 0x192)&&(Header->build<31)))
		{
			strings_to_read=128;
			temp_msg_count=Header->old_str_count;
			
			// Some sort of string count corruption seems to be common in old quests
			if(temp_msg_count>128)
			{
				temp_msg_count=128;
			}
		}
		else if((Header->zelda_version == 0x192)&&(Header->build<140))
		{
			strings_to_read=255;
			temp_msg_count=Header->old_str_count;
		}
		else
		{
			if(!p_igetw(&temp_msg_count,f))
			{
				return qe_invalid;
			}
			
			strings_to_read=temp_msg_count;
			
			if (!should_skip && temp_msg_count >= msg_strings_size)
			{
				Z_message("Reallocating string buffer...\n");
				
				// if((MsgStrings=(MsgStr*)_al_sane_realloc(MsgStrings,sizeof(MsgStr)*MAXMSGS))==NULL)
					// return qe_nomem;
					
				//memset(MsgStrings, 0, sizeof(MsgStr)*MAXMSGS);
				delete[] MsgStrings;
				MsgStrings = new MsgStr[MAXMSGS];
				msg_strings_size = MAXMSGS;
				for(auto q = 0; q < msg_strings_size; ++q)
				{
					MsgStrings[q].clear();
				}
			}
		}
		
		//reset the message strings
		if (!should_skip)
			init_msgstrings(0,msg_strings_size);
		
		for(int32_t x=0; x<strings_to_read; x++)
		{
			init_msgstr(&tempMsgString);
			tempMsgString.listpos = x;
			
			if(!pfread(buf,73,f))
			{
				return qe_invalid;
			}
			
			buf[74] = '\0';
			tempMsgString.s = buf;
				
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			
			if((Header->zelda_version < 0x192)||
				((Header->zelda_version == 0x192)&&(Header->build<148)))
			{
				tempMsgString.nextstring=tempbyte?x+1:0;
				
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tempMsgString.nextstring,f))
				{
					return qe_invalid;
				}
				
				if(!pfread(temp_expansion,32,f))
				{
					return qe_invalid;
				}
			}
			
			if (!should_skip)
				MsgStrings[x] = tempMsgString;
		}
	}
	else
	{
		int32_t dummy_int;
		word s_version;
		word s_cversion;
		
		//section version info
		if(!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}
	
		FFCore.quest_format[vStrings] = s_version;
		
		if(!p_igetw(&s_cversion,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&dummy_int,f))
		{
			return qe_invalid;
		}
		
		//finally...  section data
		if(!p_igetw(&temp_msg_count,f))
		{
			return qe_invalid;
		}
		
		if(temp_msg_count >= msg_strings_size)
		{
			Z_message("Reallocating string buffer...\n");
			
			// if((MsgStrings=(MsgStr*)_al_sane_realloc(MsgStrings,sizeof(MsgStr)*MAXMSGS))==NULL)
				// return qe_nomem;
			delete[] MsgStrings;
			MsgStrings = new MsgStr[MAXMSGS];
			msg_strings_size = MAXMSGS;
			for(auto q = 0; q < msg_strings_size; ++q)
			{
				MsgStrings[q].clear();
			}
			//memset(MsgStrings, 0, sizeof(MsgStr)*MAXMSGS);
		}
		
		//reset the message strings
		if(s_version < 7)
			set_qr(qr_OLD_STRING_EDITOR_MARGINS,true);
		init_msgstrings(0,msg_strings_size);
		
		int32_t string_length=(s_version<2)?73:145;
		
		for(int32_t i=0; i<temp_msg_count; i++)
		{
			init_msgstr(&tempMsgString);
			tempMsgString.listpos = i;
			if(s_version > 8)
			{
				if(!p_igetl(&string_length,f))
				{
					return qe_invalid;
				}
			}

			if (string_length < 0 || string_length > 8193)
			{
				return qe_invalid;
			}

			if (string_length > 0)
			{
				if (!pfread(buf, string_length, f))
				{
					return qe_invalid;
				}
			}
			else
			{
				buf[0] = 0;
			}
			
			if(!p_igetw(&tempMsgString.nextstring,f))
			{
				return qe_invalid;
			}
			
			if(s_version<2)
			{
				buf[72] = '\0';
				tempMsgString.s = buf;
			}
			else
			{
				// June 2008: A bug corrupted the last 4 chars of a string.
				// Discard these.
				if(s_version<3)
				{
					for(int32_t j=140; j<144; j++)
					{
						buf[j] = '\0';
					}
				}
				if(string_length > 8192) string_length = 8192;
				buf[string_length]='\0'; //Force-terminate
				tempMsgString.s = buf;
				
				if ( s_version >= 6 )
				{
					if(!p_igetl(&tempMsgString.tile,f))
					{
						return qe_invalid;
					}
				}
				else
				{
					if(!p_igetw(&tempMsgString.tile,f))
					{
						return qe_invalid;
					}
				}
				
				if(!p_getc(&tempMsgString.cset,f))
				{
					return qe_invalid;
				}
				
				byte dummy_char;
				
				if(!p_getc(&dummy_char,f)) // trans is stored as a char...
				{
					return qe_invalid;
				}
				
				tempMsgString.trans=dummy_char!=0;
				
				if(!p_getc(&tempMsgString.font,f))
				{
					return qe_invalid;
				}
				
				if(s_version < 5)
				{
					if(!p_getc(&tempMsgString.y,f))
					{
						return qe_invalid;
					}
				}
				else
				{
					if(!p_igetw(&tempMsgString.x,f))
					{
						return qe_invalid;
					}
					
					if(!p_igetw(&tempMsgString.y,f))
					{
						return qe_invalid;
					}
					
					if(!p_igetw(&tempMsgString.w,f))
					{
						return qe_invalid;
					}
					
					if(!p_igetw(&tempMsgString.h,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.hspace,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.vspace,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.stringflags,f))
					{
						return qe_invalid;
					}
				}
				
				if(s_version >= 7)
				{
					for(int32_t q = 0; q < 4; ++q)
					{
						if(!p_getc(&tempMsgString.margins[q],f))
						{
							return qe_invalid;
						}
					}
					
					if(!p_igetl(&tempMsgString.portrait_tile,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.portrait_cset,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.portrait_x,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.portrait_y,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.portrait_tw,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.portrait_th,f))
					{
						return qe_invalid;
					}
				}
				
				if(s_version >= 8)
				{
					if(!p_getc(&tempMsgString.shadow_type,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&tempMsgString.shadow_color,f))
					{
						return qe_invalid;
					}
				}
				
				if(s_version >= 10)
				{
					if(!p_getc(&tempMsgString.drawlayer,f))
					{
						return qe_invalid;
					}
				}
				
				if(!p_getc(&tempMsgString.sfx,f))
				{
					return qe_invalid;
				}
				
				if(s_version>3)
				{
					if(!p_igetw(&tempMsgString.listpos,f))
					{
						return qe_invalid;
					}
				}
			}
			
			MsgStrings[i].copyAll(tempMsgString);
		}
	}
	
	if (!should_skip)
		msg_count=temp_msg_count;
	
	return 0;
}

int32_t readdoorcombosets(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_doors);

    if((Header->zelda_version < 0x192)||
            ((Header->zelda_version == 0x192)&&(Header->build<158)))
    {
        return 0;
    }
    
    word temp_door_combo_set_count=0;
    DoorComboSet tempDoorComboSet;
    word dummy_word;
    int32_t dummy_long;
    byte padding;
    int32_t s_version = 0;
    
	if (!should_skip)
	{
		DoorComboSets = {};
		DoorComboSetNames = {};
	}
    
    if(Header->zelda_version > 0x192)
    {
        //section version info
        if(!p_igetw(&s_version,f))
        {
            return qe_invalid;
        }
	
        FFCore.quest_format[vDoors] = s_version;
	
        if(!p_igetw(&dummy_word,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy_long,f))
        {
            return qe_invalid;
        }
    }
    
    //finally...  section data
    if(!p_igetw(&temp_door_combo_set_count,f))
    {
        return qe_invalid;
    }

	if (temp_door_combo_set_count > MAXDOORCOMBOSETS)
	{
		return qe_invalid;
	}
    
    for(int32_t i=0; i<temp_door_combo_set_count; i++)
    {
        memset(&tempDoorComboSet, 0, sizeof(DoorComboSet));
        
        //name
		char name[21];
        if(!pfread(&name,sizeof(name),f))
        {
            return qe_invalid;
        }
		if (!should_skip)
			DoorComboSetNames[i] = name;
        
        if(Header->zelda_version < 0x193)
        {
            if(!p_getc(&padding,f))
            {
                return qe_invalid;
            }
        }
        
        //up door
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<4; k++)
            {
                if(!p_igetw(&tempDoorComboSet.doorcombo_u[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<4; k++)
            {
                if(!p_getc(&tempDoorComboSet.doorcset_u[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        //down door
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<4; k++)
            {
                if(!p_igetw(&tempDoorComboSet.doorcombo_d[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<4; k++)
            {
                if(!p_getc(&tempDoorComboSet.doorcset_d[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        //left door
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<6; k++)
            {
                if(!p_igetw(&tempDoorComboSet.doorcombo_l[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<6; k++)
            {
                if(!p_getc(&tempDoorComboSet.doorcset_l[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        //right door
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<6; k++)
            {
                if(!p_igetw(&tempDoorComboSet.doorcombo_r[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        for(int32_t j=0; j<9; j++)
        {
            for(int32_t k=0; k<6; k++)
            {
                if(!p_getc(&tempDoorComboSet.doorcset_r[j][k],f))
                {
                    return qe_invalid;
                }
            }
        }
        
        //up bomb rubble
        for(int32_t j=0; j<2; j++)
        {
            if(!p_igetw(&tempDoorComboSet.bombdoorcombo_u[j],f))
            {
                return qe_invalid;
            }
        }
        
        for(int32_t j=0; j<2; j++)
        {
            if(!p_getc(&tempDoorComboSet.bombdoorcset_u[j],f))
            {
                return qe_invalid;
            }
        }
        
        //down bomb rubble
        for(int32_t j=0; j<2; j++)
        {
            if(!p_igetw(&tempDoorComboSet.bombdoorcombo_d[j],f))
            {
                return qe_invalid;
            }
        }
        
        for(int32_t j=0; j<2; j++)
        {
            if(!p_getc(&tempDoorComboSet.bombdoorcset_d[j],f))
            {
                return qe_invalid;
            }
        }
        
        //left bomb rubble
        for(int32_t j=0; j<3; j++)
        {
            if(!p_igetw(&tempDoorComboSet.bombdoorcombo_l[j],f))
            {
                return qe_invalid;
            }
        }
        
        for(int32_t j=0; j<3; j++)
        {
            if(!p_getc(&tempDoorComboSet.bombdoorcset_l[j],f))
            {
                return qe_invalid;
            }
        }
        
        if(Header->zelda_version < 0x193)
        {
            if(!p_getc(&padding,f))
            {
                return qe_invalid;
            }
            
        }
        
        //right bomb rubble
        for(int32_t j=0; j<3; j++)
        {
            if(!p_igetw(&tempDoorComboSet.bombdoorcombo_r[j],f))
            {
                return qe_invalid;
            }
        }
        
        for(int32_t j=0; j<3; j++)
        {
            if(!p_getc(&tempDoorComboSet.bombdoorcset_r[j],f))
            {
                return qe_invalid;
            }
        }
        
        if(Header->zelda_version < 0x193)
        {
            if(!p_getc(&padding,f))
            {
                return qe_invalid;
            }
        }
        
        //walkthrough stuff
        for(int32_t j=0; j<4; j++)
        {
            if(!p_igetw(&tempDoorComboSet.walkthroughcombo[j],f))
            {
                return qe_invalid;
            }
        }
        
        for(int32_t j=0; j<4; j++)
        {
            if(!p_getc(&tempDoorComboSet.walkthroughcset[j],f))
            {
                return qe_invalid;
            }
        }
        
        //flags
        for(int32_t j=0; j<2; j++)
        {
            if(!p_getc(&tempDoorComboSet.flags[j],f))
            {
                return qe_invalid;
            }
        }
        
        if(Header->zelda_version < 0x193)
        {
            if(!pfread(&tempDoorComboSet.expansion,sizeof(tempDoorComboSet.expansion),f))
            {
                return qe_invalid;
            }
        }
        
        if (!should_skip)
            memcpy(&DoorComboSets[i], &tempDoorComboSet, sizeof(tempDoorComboSet));
    }
    
	if (!should_skip)
		door_combo_set_count=temp_door_combo_set_count;
    
    return 0;
}

int32_t count_dmaps()
{
    int32_t i=MAXDMAPS-1;
    bool found=false;
    
    while(i>=0 && !found)
    {
        if((DMaps[i].map!=0)||(DMaps[i].level!=0)||(DMaps[i].xoff!=0)||
                (DMaps[i].compass!=0)||(DMaps[i].color!=0)||(DMaps[i].midi!=0)||
                (DMaps[i].cont!=0)||(DMaps[i].type!=0))
            found=true;
            
        for(int32_t j=0; j<8; j++)
        {
            if(DMaps[i].grid[j]!=0)
            
                found=true;
        }
        
        if((DMaps[i].name[0]!=0)||(DMaps[i].title[0]!=0)||
                (DMaps[i].intro[0]!=0)||(DMaps[i].tmusic[0]!=0))
            found=true;
            
        if((DMaps[i].minimap_1_tile!=0)||(DMaps[i].minimap_2_tile!=0)||
                (DMaps[i].largemap_1_tile!=0)||(DMaps[i].largemap_2_tile!=0)||
                (DMaps[i].minimap_1_cset!=0)||(DMaps[i].minimap_2_cset!=0)||
                (DMaps[i].largemap_1_cset!=0)||(DMaps[i].largemap_2_cset!=0))
            found=true;
            
        if(!found)
        {
            i--;
        }
    }
    
    return i+1;
}


int32_t count_shops(miscQdata *Misc)
{
    int32_t i=NUM_SHOPS-1,j;
    bool found=false;
    
    while(i>=0 && !found)
    {
        j=2;
        
        while(j>=0 && !found)
        {
            if((Misc->shop[i].hasitem[j]!=0)||(Misc->shop[i].price[j]!=0))
            {
                found=true;
            }
            else
            {
                j--;
            }
        }
        
        if(Misc->shop[i].name[0]!=0)
        {
            found=true;
        }
        
        if(!found)
        {
            i--;
        }
    }
    
    return i+1;
}

int32_t count_infos(miscQdata *Misc)
{
    int32_t i=255,j;
    bool found=false;
    
    while(i>=0 && !found)
    {
        j=2;
        
        while(j>=0 && !found)
        {
            if((Misc->info[i].str[j]!=0)||(Misc->info[i].price[j]!=0))
            {
                found=true;
            }
            else
            {
                j--;
            }
        }
        
        if(Misc->info[i].name[0]!=0)
        {
            found=true;
        }
        
        if(!found)
        {
            i--;
        }
    }
    
    return i+1;
}

int32_t count_warprings(miscQdata *Misc)
{
    int32_t i=15,j;
    bool found=false;
    
    while(i>=0 && !found)
    {
        j=7;
        
        while(j>=0 && !found)
        {
            if((Misc->warp[i].dmap[j]!=0)||(Misc->warp[i].scr[j]!=0))
            {
                found=true;
            }
            else
            {
                j--;
            }
        }
        
        if(!found)
        {
            i--;
        }
    }
    
    return i+1;
}

int32_t count_palcycles(miscQdata *Misc)
{
    int32_t i=255,j;
    bool found=false;
    
    while(i>=0 && !found)
    {
        j=2;
        
        while(j>=0 && !found)
        {
            if(Misc->cycles[i][j].count!=0)
            {
                found=true;
            }
            else
            {
                j--;
            }
        }
        
        if(!found)
        {
            i--;
        }
    }
    
    return i+1;
}

void clear_screen(mapscr *temp_scr)
{
	temp_scr->zero_memory();
}

// NOTE: when modifying this, you need to also update:
//    readonedmap, readdmaps, and FFScript::read_dmaps
// (and their associated write functions)
int32_t readdmaps(PACKFILE *f, zquestheader *Header, word, word, word start_dmap, word max_dmaps)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_dmaps);

	word dmapstoread=0;
	dmap tempDMap;
	
	int32_t dummy;
	word s_version=0, s_cversion=0;
	byte padding;

	char legacy_title[22];
	
	if (!should_skip)
	for(int32_t i=0; i<max_dmaps; i++)
	{
		DMaps[start_dmap + i].clear();
		sprintf(legacy_title,"                    ");
		sprintf(DMaps[start_dmap+i].intro,"                                                                        ");
		DMaps[start_dmap+i].type |= dmCAVE;
	}
	
	if(!Header || Header->zelda_version > 0x192)
	{
		//section version info
		if(!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[vDMaps] = s_version;
		
		
		if(!p_igetw(&s_cversion,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&dummy,f))
		{
			return qe_invalid;
		}
		
		//finally...  section data
		if(!p_igetw(&dmapstoread,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		if((Header->zelda_version < 0x192)||
				((Header->zelda_version == 0x192)&&(Header->build<5)))
		{
			dmapstoread=32;
		}
		else if(s_version <= 4)
		{
			dmapstoread=OLDMAXDMAPS;
		}
		else
		{
			dmapstoread=MAXDMAPS;
		}
	}
	
	dmapstoread=zc_min(dmapstoread, max_dmaps);
	dmapstoread=zc_min(dmapstoread, MAXDMAPS-start_dmap);
	
	for(int32_t i=start_dmap; i<dmapstoread+start_dmap; i++)
	{
		tempDMap.clear();
		sprintf(legacy_title,"                    ");
		sprintf(tempDMap.intro,"                                                                        ");
		
		if(!p_getc(&tempDMap.map,f))
		{
			return qe_invalid;
		}
		
		if(s_version <= 4)
		{
			byte tempbyte;
			
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			
			tempDMap.level=(word)tempbyte;
		}
		else
		{
			if(!p_igetw(&tempDMap.level,f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_getc(&tempDMap.xoff,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempDMap.compass,f))
		{
			return qe_invalid;
		}
		
		if(s_version > 8) // February 2009
		{
			if(!p_igetw(&tempDMap.color,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			byte tempbyte;
			
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			
			tempDMap.color = (word)tempbyte;
		}
		
		if(!p_getc(&tempDMap.midi,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempDMap.cont,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempDMap.type,f))
		{
			return qe_invalid;
		}
		
		if((tempDMap.type & dmfTYPE) == dmOVERW &&
		   (!Header || Header->zelda_version >= 0x210)) // Not sure exactly when this changed
			tempDMap.xoff = 0;
		
		for(int32_t j=0; j<8; j++)
		{
			if(!p_getc(&tempDMap.grid[j],f))
			{
				return qe_invalid;
			}
		}
		
		if(Header && ((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<41))))
		{
			if(tempDMap.level>0&&tempDMap.level<10)
			{
				sprintf(legacy_title,"LEVEL-%d             ", tempDMap.level);
			}
			tempDMap.title.assign(legacy_title);
			
			if(i==0 && Header->zelda_version <= 0x190)
			{
				tempDMap.cont = std::max((int)tempDMap.cont - tempDMap.xoff, 0);
				tempDMap.compass = std::max((int)tempDMap.compass - tempDMap.xoff, 0);
			}
			
			//forgotten -DD
			if(tempDMap.level==0)
			{
				tempDMap.flags=dmfCAVES|dmf3STAIR|dmfWHIRLWIND|dmfGUYCAVES;
			}
		}
		else
		{
			if(!p_getstr(tempDMap.name,sizeof(DMaps[0].name) - 1,f))
			{
				return qe_invalid;
			}
			
			if(s_version<20)
			{
				if (!p_getstr(legacy_title, sizeof(legacy_title) - 1, f))
				{
					return qe_invalid;
				}
				tempDMap.title.assign(legacy_title);
			}
			else
			{
				if (!p_getwstr(&tempDMap.title, f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getstr(tempDMap.intro,sizeof(DMaps[0].intro)-1,f))
			{
				return qe_invalid;
			}
			
			if(Header && ((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<152))))
			{
				if ((tempDMap.type & dmfTYPE) == dmOVERW) tempDMap.flags = dmfCAVES | dmf3STAIR | dmfWHIRLWIND | dmfGUYCAVES;
				DMaps[i] = tempDMap;
				
				continue;
			}
			
			if(Header && (Header->zelda_version < 0x193))
			{
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 11 )
			{
				if(!p_igetl(&tempDMap.minimap_1_tile,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tempDMap.minimap_1_tile,f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getc(&tempDMap.minimap_1_cset,f))
			{
				return qe_invalid;
			}
			
			if(Header && (Header->zelda_version < 0x193))
			{
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
			
			if ( s_version >= 11 )
			{
				if(!p_igetl(&tempDMap.minimap_2_tile,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tempDMap.minimap_2_tile,f))
				{
					return qe_invalid;
				}
			}
			if(!p_getc(&tempDMap.minimap_2_cset,f))
			{
				return qe_invalid;
			}
			
			if(Header && (Header->zelda_version < 0x193))
			{
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
			
			if ( s_version >= 11 )
			{
				if(!p_igetl(&tempDMap.largemap_1_tile,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tempDMap.largemap_1_tile,f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getc(&tempDMap.largemap_1_cset,f))
			{
				return qe_invalid;
			}
			
			if(Header && (Header->zelda_version < 0x193))
			{
			
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
			
			if ( s_version >= 11 )
			{
				if(!p_igetl(&tempDMap.largemap_2_tile,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tempDMap.largemap_2_tile,f))
				{
					return qe_invalid;
				}
			}
			if(!p_getc(&tempDMap.largemap_2_cset,f))
			{
				return qe_invalid;
			}
			
			if(!p_getstr(tempDMap.tmusic,sizeof(DMaps[0].tmusic)-1,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>1)
		{
			if(!p_getc(&tempDMap.tmusictrack,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempDMap.active_subscreen,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempDMap.passive_subscreen,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>2)
		{
			byte di[32];
			
			if(!pfread(&di, 32, f)) return qe_invalid;
			
			for(int32_t j=0; j<MAXITEMS; j++)
			{
				if(di[j/8] & (1 << (j%8))) tempDMap.disableditems[j]=1;
				else tempDMap.disableditems[j]=0;
			}
		}
		
		if(s_version >= 6)
		{
			if(!p_igetl(&tempDMap.flags,f))
			{
				return qe_invalid;
			}
		}
		else if(s_version>3)
		{
			char temp;
			
			if(!p_getc(&temp,f))
			{
				return qe_invalid;
			}
			
			tempDMap.flags = temp;
		}
		else if(tempDMap.level==0 && ((Header->zelda_version < 0x211) || ((Header->zelda_version == 0x211) && (Header->build<18))))
		{
			tempDMap.flags=dmfCAVES|dmf3STAIR|dmfWHIRLWIND|dmfGUYCAVES;
		}
		else
			tempDMap.flags=0;
			
		if(s_version<7)
		{
			if(tempDMap.level==0 && get_bit(deprecated_rules,14))
				tempDMap.flags|= dmfVIEWMAP;
		}
		
		if(s_version<8)
		{
			if(tempDMap.level==0 && (tempDMap.type&dmfTYPE)==dmDNGN)
			{
				tempDMap.type &= ~dmDNGN;
				tempDMap.type |= dmCAVE;
			}
			else if((tempDMap.type&dmfTYPE)==dmCAVE)
			{
				tempDMap.flags |= dmfMINIMAPCOLORFIX;
			}
		}
		
		if(Header && ((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>=41)))
				&& (Header->zelda_version < 0x193))
		{
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
		}
	
		if(s_version >= 10)
		{
			if(!p_getc(&tempDMap.sideview,f))
			{
				return qe_invalid;
			}
		}
		if(s_version < 10) tempDMap.sideview = 0;
		
		//Dmap Scripts
		if(s_version >= 12)
		{
			if(!p_igetw(&tempDMap.script,f))
			{
				return qe_invalid;
			}
			for ( int32_t q = 0; q < 8; q++ )
			{
				if(!p_igetl(&tempDMap.initD[q],f))
				{
					return qe_invalid;
				}
			}
		}
		if ( s_version < 12 )
		{
			tempDMap.script = 0;
			for ( int32_t q = 0; q < 8; q++ )
			{
				tempDMap.initD[q] = 0;
			}
		}
		
		if(s_version >= 13)
		{
			for ( int32_t q = 0; q < 8; q++ )
			{
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_getc(&tempDMap.initD_label[q][w],f))
					{
						return qe_invalid;
					} 
				}
			}
		}
		if ( s_version < 13 )
		{
			tempDMap.script = 0;
			for ( int32_t q = 0; q < 8; q++ )
			{
				for ( int32_t w = 0; w < 65; w++ )
					tempDMap.initD_label[q][w] = 0;
			}
		}
		if(s_version >= 14)
		{
			if(!p_igetw(&tempDMap.active_sub_script,f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&tempDMap.passive_sub_script,f))
			{
				return qe_invalid;
			}
			for ( int32_t q = 0; q < 8; ++q )
			{
				if(!p_igetl(&tempDMap.sub_initD[q],f))
				{
					return qe_invalid;
				}
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				for ( int32_t w = 0; w < 65; ++w )
				{
					if(!p_getc(&tempDMap.sub_initD_label[q][w],f))
					{
						return qe_invalid;
					} 
				}
			}
		}
		else
		{
			tempDMap.active_sub_script = 0;
			tempDMap.passive_sub_script = 0;
			for(int32_t q = 0; q < 8; ++q)
			{
				tempDMap.sub_initD[q] = 0;
				for(int32_t w = 0; w < 65; ++w)
					tempDMap.sub_initD_label[q][w] = 0;
			}
		}
		if(s_version >= 15)
		{
			if(!p_igetw(&tempDMap.onmap_script,f))
			{
				return qe_invalid;
			}
			for ( int32_t q = 0; q < 8; ++q )
			{
				if(!p_igetl(&tempDMap.onmap_initD[q],f))
				{
					return qe_invalid;
				}
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				for ( int32_t w = 0; w < 65; ++w )
				{
					if(!p_getc(&tempDMap.onmap_initD_label[q][w],f))
					{
						return qe_invalid;
					} 
				}
			}
		}
		else
		{
			tempDMap.onmap_script = 0;
			for(int32_t q = 0; q < 8; ++q)
			{
				tempDMap.onmap_initD[q] = 0;
				for(int32_t w = 0; w < 65; ++w)
				{
					tempDMap.onmap_initD_label[q][w] = 0;
				}
			}
		}
		if(s_version >= 16)
		{
			if(!p_igetw(&tempDMap.mirrorDMap,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			tempDMap.mirrorDMap = -1;
		}

		if (s_version >= 17)
		{
			// Reserved for z3.
		}

		// Enhanced music loop points
		if (s_version >= 18)
		{
			if (!p_igetl(&tempDMap.tmusic_loop_start, f))
			{
				return qe_invalid;
			}
			if (!p_igetl(&tempDMap.tmusic_loop_end, f))
			{
				return qe_invalid;
			}
			if (!p_igetl(&tempDMap.tmusic_xfade_in, f))
			{
				return qe_invalid;
			}
			if (!p_igetl(&tempDMap.tmusic_xfade_out, f))
			{
				return qe_invalid;
			}
		}
		else
		{
			tempDMap.tmusic_loop_start = 0;
			tempDMap.tmusic_loop_end = 0;
			tempDMap.tmusic_xfade_in = 0;
			tempDMap.tmusic_xfade_out = 0;
		}
		
		if(s_version >= 19)
			if(!p_getc(&tempDMap.overlay_subscreen, f))
				return qe_invalid;

		if (s_version >= 20)
		{
			if (!p_igetl(&tempDMap.intro_string_id, f))
				return qe_invalid;
		}
		else
			tempDMap.intro_string_id = 0;

		// Reserved for z3.
		if(s_version >= 21)
		{
			for(int32_t j=0; j<8; j++)
			{
				for(int32_t k=0; k<8; k++)
				{
					char c;
					if(!p_getc(&c,f))
					{
						return qe_invalid;
					}
				}
			}
		}

		if (!should_skip)
		{
			if(loading_tileset_flags & TILESET_CLEARMAPS)
				tempDMap.map = 0;
			if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
			{
				tempDMap.script = 0;
				for(int q = 0; q < 8; ++q)
					tempDMap.initD[q] = 0;
			}
			DMaps[i] = tempDMap;
		}
	}
	
	return 0;
}

int32_t readmisccolors(PACKFILE *f, zquestheader *Header, miscQdata *Misc)
{
	//these are here to bypass compiler warnings about unused arguments
	Header=Header;
	
	miscQdata temp_misc = *Misc;
	word s_version=0, s_cversion=0;
	int32_t tempsize=0;
	word dummyw;
	
	//section version info
	if(!p_igetw(&s_version,f))
	{
		return qe_invalid;
	}
	
	FFCore.quest_format[vColours] = s_version;
	
	al_trace("Misc Colours section version: %d\n", s_version);
	
	if(!p_igetw(&s_cversion,f))
	{
		return qe_invalid;
	}
	
	
	//section size
	if(!p_igetl(&tempsize,f))
	{
		return qe_invalid;
	}
	
	//finally...  section data
	readsize=0;
	
	if(!p_getc(&temp_misc.colors.text,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.caption,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.overw_bg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.dngn_bg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.dngn_fg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.cave_fg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.bs_dk,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.bs_goal,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.compass_lt,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.compass_dk,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.subscr_bg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.triframe_color,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.hero_dot,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.bmap_bg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.bmap_fg,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.triforce_cset,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.triframe_cset,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.overworld_map_cset,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.dungeon_map_cset,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.blueframe_cset,f))
	{
		return qe_invalid;
	}
	if(s_version < 4)
	{
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.triforce_tile = dummyw;
		
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.triframe_tile = dummyw;
		
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.overworld_map_tile = dummyw;
		
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.dungeon_map_tile = dummyw;
		
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.blueframe_tile = dummyw;
		
		if(!p_igetw(&dummyw,f))
			return qe_invalid;
		temp_misc.colors.HCpieces_tile = dummyw;
	}
	
	if(!p_getc(&temp_misc.colors.HCpieces_cset,f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&temp_misc.colors.subscr_shadow,f))
	{
		return qe_invalid;
	}
	
	if(s_version < 2)
	{
		temp_misc.colors.msgtext = 0x01;
	}
	else
	{
		if(!p_getc(&temp_misc.colors.msgtext, f))
		{
			return qe_invalid;
		}
	}
	
	if ( s_version >= 3 ) //expanded tile pages to 825
	{
		if(!p_igetl(&temp_misc.colors.triforce_tile,f))
		{
			 return qe_invalid;
		}
		
		if(!p_igetl(&temp_misc.colors.triframe_tile,f))
		{
			 return qe_invalid;
		}
		
		if(!p_igetl(&temp_misc.colors.overworld_map_tile,f))
		{
			 return qe_invalid;
		}
		
		if(!p_igetl(&temp_misc.colors.dungeon_map_tile,f))
		{
			 return qe_invalid;
		}
		
		if(!p_igetl(&temp_misc.colors.blueframe_tile,f))
		{
			 return qe_invalid;
		}
		
		if(!p_igetl(&temp_misc.colors.HCpieces_tile,f))
		{
			 return qe_invalid;
		}
	}
	
	*Misc = temp_misc;
	
	return 0;
}

int32_t readgameicons(PACKFILE *f, zquestheader *, miscQdata *Misc)
{
    miscQdata temp_misc = *Misc;
    word s_version=0, s_cversion=0;
    byte icons;
    int32_t tempsize=0;
    
    //section version info
    if(!p_igetw(&s_version,f))
    {
        return qe_invalid;
    }
    
    FFCore.quest_format[vIcons] = s_version;
    
    if(!p_igetw(&s_cversion,f))
    {
        return qe_invalid;
    }
    
    
    //section size
    if(!p_igetl(&tempsize,f))
    {
        return qe_invalid;
    }
    
    //finally...  section data
    readsize=0;
    
    icons=4;
    
    if ( s_version >= 10 )
    {
	    for(int32_t i=0; i<icons; i++)
	    {
		if(!p_igetl(&temp_misc.icons[i],f))
		{
		    return qe_invalid;
		}
	    }	   
    }
    else
    {	    
	    for(int32_t i=0; i<icons; i++)
	    {
		if(!p_igetw(&temp_misc.icons[i],f))
		{
		    return qe_invalid;
		}
	    }
    }

	*Misc = temp_misc;
    
    return 0;
}

int32_t readmisc(PACKFILE *f, zquestheader *Header, miscQdata *Misc)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_misc);

	word maxinfos=256;
	word maxshops=256;
	word shops=16, infos=16, warprings=8, palcycles=256, windwarps=9, triforces=8, icons=4;
	word ponds=16, pondsize=72, expansionsize=98*2;
	byte tempbyte, padding;
	miscQdata temp_misc = *Misc;
	word s_version=0, s_cversion=0;
	word swaptmp;
	int32_t tempsize=0;
	
	for(int32_t i=0; i<maxshops; ++i)
	{
		memset(&temp_misc.shop, 0, sizeof(shoptype)*256);
	}
	
	for(int32_t i=0; i<maxinfos; ++i)
	{
		memset(&temp_misc.info, 0, sizeof(infotype)*256);
	}
	
	if(Header->zelda_version > 0x192)
	{
		//section version info
		if(!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}
		
		FFCore.quest_format[vMisc] = s_version;
		
		if(!p_igetw(&s_cversion,f))
		{
			return qe_invalid;
		}
		
		
		//section size
		if(!p_igetl(&tempsize,f))
		{
			return qe_invalid;
		}
	}
	
	//finally...  section data
	readsize=0;
	
	//shops
	if(Header->zelda_version > 0x192)
	{
		if(!p_igetw(&shops,f))
		{
			return qe_invalid;
		}
	}

	if (shops > NUM_SHOPS)
	{
		return qe_invalid;
	}
	
	for(int32_t i=0; i<shops; i++)
	{
		if(s_version > 6)
		{
			if(!p_getstr(temp_misc.shop[i].name,sizeof(temp_misc.shop[i].name)-1,f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t j=0; j<3; j++)
		{
			if(!p_getc(&temp_misc.shop[i].item[j],f))
			{
				return qe_invalid;
			}
			
			if(s_version < 4)
			{
				temp_misc.shop[i].hasitem[j] = (temp_misc.shop[i].item[j] == 0) ? 0 : 1;
			}
		}
		
		if(Header->zelda_version < 0x193)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t j=0; j<3; j++)
		{
			if(!p_igetw(&temp_misc.shop[i].price[j],f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version > 3)
		{
			for(int32_t j=0; j<3; j++)
			{
				if(!p_getc(&temp_misc.shop[i].hasitem[j],f))
					return qe_invalid;
			}
		}
	
	/*
	if(s_version < 8)
		{
			for(int32_t j=0; j<3; j++)
			{
				(&temp_misc.shop[i].str[j])=0; //initialise.
			}
		}
	*/
	}
	
	//filter all the 0 items to the end (yeah, bubble sort; sue me)
	for(int32_t i=0; i<maxshops; ++i)
	{
		for(int32_t j=0; j<3-1; j++)
		{
			for(int32_t k=0; k<2-j; k++)
			{
				if(temp_misc.shop[i].hasitem[k]==0)
				{
					swaptmp = temp_misc.shop[i].item[k];
					temp_misc.shop[i].item[k] = temp_misc.shop[i].item[k+1];
					temp_misc.shop[i].item[k+1] = swaptmp;
					swaptmp = temp_misc.shop[i].price[k];
					temp_misc.shop[i].price[k] = temp_misc.shop[i].price[k+1];
					temp_misc.shop[i].price[k+1] = swaptmp;
					swaptmp = temp_misc.shop[i].hasitem[k];
					temp_misc.shop[i].hasitem[k] = temp_misc.shop[i].hasitem[k+1];
					temp_misc.shop[i].hasitem[k+1] = swaptmp;
				}
			}
		}
	}
	
	//infos
	if(Header->zelda_version > 0x192)
	{
		if(!p_igetw(&infos,f))
		{
			return qe_invalid;
		}
	}

	if (infos > NUM_INFOS)
	{
		return qe_invalid;
	}
	

	for(int32_t i=0; i<infos; i++)
	{
		if(s_version > 6)
		{
			if(!p_getstr(temp_misc.info[i].name,sizeof(temp_misc.info[i].name)-1,f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t j=0; j<3; j++)
		{
			if((Header->zelda_version < 0x192)||
					((Header->zelda_version == 0x192)&&(Header->build<146)))
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				temp_misc.info[i].str[j]=tempbyte;
			}
			else
			{
				if(!p_igetw(&temp_misc.info[i].str[j],f))
				{
					return qe_invalid;
				}
			}
		}
		
		if(Header->zelda_version < 0x193)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
		}
		
		if((Header->zelda_version == 0x192)&&(Header->build>145))
		{
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t j=0; j<3; j++)
		{
			if(!p_igetw(&temp_misc.info[i].price[j],f))
			{
				return qe_invalid;
			}
		}
	}
	
	//filter all the 0 strings to the end (yeah, bubble sort; sue me)
	for(int32_t i=0; i<maxinfos; ++i)
	{
		for(int32_t j=0; j<3-1; j++)
		{
			for(int32_t k=0; k<2-j; k++)
			{
				if(temp_misc.info[i].str[k]==0)
				{
					swaptmp = temp_misc.info[i].str[k];
					temp_misc.info[i].str[k] = temp_misc.info[i].str[k+1];
					temp_misc.info[i].str[k+1] = swaptmp;
					swaptmp = temp_misc.info[i].price[k];
					temp_misc.info[i].price[k] = temp_misc.info[i].price[k+1];
					temp_misc.info[i].price[k+1] = swaptmp;
				}
			}
		}
	}
	
	
	//warp rings
	if(s_version > 5)
		warprings++;
		
	if(Header->zelda_version > 0x192)
	{
		if(!p_igetw(&warprings,f))
		{
			return qe_invalid;
		}

		if (warprings > NUM_WARP_RINGS)
		{
			// return qe_invalid;
			// Note: we can't actually fail here because for some reason, some quest files have more than the max
			// number of possible warp rings. Some examples of this are: demosp253.qst, yuurand.qst
			// So instead below we disable `keepdata` when reading the bad warp ring data, so no memory is corrupted.
		}
	}
	
	for(int32_t i=0; i<warprings; i++)
	{
		// See above comment on the `warprings` range check.
		bool keepdata = i < NUM_WARP_RINGS;

		for(int32_t j=0; j<8+((s_version > 5)?1:0); j++)
		{
			if(s_version <= 3)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}

				if (keepdata)
					temp_misc.warp[i].dmap[j]=(word)tempbyte;
			}
			else
			{
				word tempword;
				if(!p_igetw(&tempword,f))
				{
					return qe_invalid;
				}

				if (keepdata)
					temp_misc.warp[i].dmap[j] = tempword;
			}
		}
		
		for(int32_t j=0; j<8+((s_version > 5)?1:0); j++)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			if (keepdata)
				temp_misc.warp[i].scr[j] = tempbyte;
		}

		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
		if (keepdata)
			temp_misc.warp[i].size = tempbyte;
		
		if(Header->zelda_version < 0x193)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
		}
	}
	
	//palette cycles
	if(Header->zelda_version < 0x193)                         //in 1.93+, palette cycling is saved with the palettes
	{
		for(int32_t i=0; i<256; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				temp_misc.cycles[i][j].first=0;
				temp_misc.cycles[i][j].count=0;
				temp_misc.cycles[i][j].speed=0;
			}
		}
		
		if((Header->zelda_version < 0x192)||
				((Header->zelda_version == 0x192)&&(Header->build<73)))
		{
			palcycles=16;
		}
		
		for(int32_t i=0; i<palcycles; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				if(!p_getc(&temp_misc.cycles[i][j].first,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&temp_misc.cycles[i][j].count,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&temp_misc.cycles[i][j].speed,f))
				{
					return qe_invalid;
				}
			}
		}
	}
	
	//Wind warps are now just another warp ring.
	if(s_version <= 5)
	{
		if(Header->zelda_version > 0x192)
		{
			if(!p_igetw(&windwarps,f))
			{
				return qe_invalid;
			}
		}

		if (windwarps > NUM_WARP_RINGS)
		{
			return qe_invalid;
		}
		
		for(int32_t i=0; i<windwarps; i++)
		{
			if(s_version <= 3)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				temp_misc.warp[8].dmap[i]=tempbyte;
			}
			else
			{
				if(!p_igetw(&temp_misc.warp[8].dmap[i],f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getc(&temp_misc.warp[8].scr[i],f))
			{
				return qe_invalid;
			}
			
			temp_misc.warp[8].size = 9;
			
			if(s_version == 5)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
			}
		}
	}
	
	
	//triforce pieces
	for(int32_t i=0; i<triforces; i++)
	{
		if(!p_getc(&temp_misc.triforce[i],f))
		{
			return qe_invalid;
		}
	}
	
	//misc color data
	if(s_version<3)
	{
		if(!p_getc(&temp_misc.colors.text,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.caption,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.overw_bg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.dngn_bg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.dngn_fg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.cave_fg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.bs_dk,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.bs_goal,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.compass_lt,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.compass_dk,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.subscr_bg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.triframe_color,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.hero_dot,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.bmap_bg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.bmap_fg,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.triforce_cset,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.triframe_cset,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.overworld_map_cset,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.dungeon_map_cset,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.blueframe_cset,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.triforce_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.triframe_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.overworld_map_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.dungeon_map_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.blueframe_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_misc.colors.HCpieces_tile,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_misc.colors.HCpieces_cset,f))
		{
			return qe_invalid;
		}
		
		temp_misc.colors.msgtext = 0x01;
		
		if(Header->zelda_version < 0x193)
		{
			for(int32_t i=0; i<7; i++)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
			}
		}
		
		if((Header->zelda_version == 0x192)&&(Header->build>145))
		{
			for(int32_t i=0; i<256; i++)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
			}
		}
		
		if(s_version>1)
		{
			if(!p_getc(&temp_misc.colors.subscr_shadow,f))
			{
				return qe_invalid;
			}
		}
		
		//save game icons
		if((Header->zelda_version < 0x192)||
				((Header->zelda_version == 0x192)&&(Header->build<73)))
		{
			icons=3;
		}
		
		for(int32_t i=0; i<icons; i++)
		{
			if(!p_igetw(&temp_misc.icons[i],f))
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version < 0x192)||
			((Header->zelda_version == 0x192)&&(Header->build<30)))
	{
		*Misc = temp_misc;
		
		return 0;
	}
	
	//pond information
	if(Header->zelda_version < 0x193)
	{
		if((Header->zelda_version == 0x192)&&(Header->build<146))
		{
			pondsize=25;
		}
		
		for(int32_t i=0; i<ponds; i++)
		{
			for(int32_t j=0; j<pondsize; j++)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
					
				}
			}
		}
	}
	
	//end string
	if((Header->zelda_version < 0x192)||
			((Header->zelda_version == 0x192)&&(Header->build<146)))
	{
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
		
		temp_misc.endstring=tempbyte;
		
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		if(!p_igetw(&temp_misc.endstring,f))
		{
			return qe_invalid;
		}
	}
	
	//expansion
	if(Header->zelda_version < 0x193)
	{
		if((Header->zelda_version == 0x192)&&(Header->build<73))
		{
			expansionsize=99*2;
		}
		
		for(int32_t i=0; i<expansionsize; i++)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
		}
	}
	//shops v8
	
	
	if(s_version >= 8)
	{
		for(int32_t i=0; i<shops; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				if(!p_igetw(&temp_misc.shop[i].str[j],f))
					return qe_invalid;
			}
		}
	}
	
	memset(&temp_misc.questmisc, 0, sizeof(int32_t)*32);
	memset(&temp_misc.zscript_last_compiled_version, 0, sizeof(int32_t));
	
	//v9 includes quest misc[32]
	// ... this has been deprecated (2024)
	if(s_version >= 9)
	{
		for ( int32_t q = 0; q < 32; q++ ) 
		{
			if(!p_igetl(&temp_misc.questmisc[q],f))
						return qe_invalid;
		}
		// this was string labels
		if (pack_fseek(f, 32 * 128))
			return qe_invalid;
	}
	
	if(s_version >= 11 )
	{
		if(!p_igetl(&temp_misc.zscript_last_compiled_version,f))
			return qe_invalid;
	}
	else if(s_version < 11 )
	{
		temp_misc.zscript_last_compiled_version = -1;
	}
	
	FFCore.quest_format[vLastCompile] = temp_misc.zscript_last_compiled_version;
	
	if(s_version >= 12)
	{
		byte spr;
		for(int32_t q = 0; q < sprMAX; ++q)
		{
			if(!p_getc(&spr,f))
				return qe_invalid;
			temp_misc.sprites[q] = spr;
		}
	}
	else
	{
		memset(&(temp_misc.sprites), 0, sizeof(temp_misc.sprites));
		//temp_misc.sprites[sprFALL] = ;
	}
	
	if(s_version >= 13)
	{
		for(size_t q = 0; q < 64; ++q)
		{
			bottletype* bt = &(temp_misc.bottle_types[q]);
            if (!p_getstr(bt->name, sizeof(bt->name)-1, f))
                return qe_invalid;
			for(size_t j = 0; j < 3; ++j)
			{
                if (!p_getc(&(bt->counter[j]), f))
                    return qe_invalid;
                if (!p_igetw(&(bt->amount[j]), f))
                    return qe_invalid;
			}
            if (!p_getc(&(bt->flags), f))
                return qe_invalid;
            if (!p_getc(&(bt->next_type), f))
                return qe_invalid;
		}
		for(size_t q = 0; q < 256; ++q)
		{
			bottleshoptype* bst = &(temp_misc.bottle_shop_types[q]);
            if (!pfread(bst->name, sizeof(bst->name)-1, f))
                return qe_invalid;
			for(size_t j = 0; j < 3; ++j)
			{
                if (!p_getc(&(bst->fill[j]), f))
                    return qe_invalid;
                if (!p_igetw(&(bst->comb[j]), f))
                    return qe_invalid;
                if (!p_getc(&(bst->cset[j]), f))
                    return qe_invalid;
                if (!p_igetw(&(bst->price[j]), f))
                    return qe_invalid;
                if (!p_igetw(&(bst->str[j]), f))
                    return qe_invalid;
			}
		}
	}
	else
	{
		for(size_t q = 0; q < 64; ++q)
			temp_misc.bottle_types[q].clear();
		for(size_t q = 0; q < 256; ++q)
			temp_misc.bottle_shop_types[q].clear();
	}
	
	if(s_version >= 14)
	{
		byte msfx;
		for(int32_t q = 0; q < sfxMAX; ++q)
		{
			if(!p_getc(&msfx,f))
				return qe_invalid;
			temp_misc.miscsfx[q] = msfx;
		}
	}
	else
	{
		memset(&(temp_misc.miscsfx), 0, sizeof(temp_misc.miscsfx));
		temp_misc.miscsfx[sfxBUSHGRASS] = WAV_ZN1GRASSCUT;
		temp_misc.miscsfx[sfxLOWHEART] = WAV_ER;
	}
	if(s_version < 15)
	{
		temp_misc.miscsfx[sfxHURTPLAYER] = WAV_OUCH;
		temp_misc.miscsfx[sfxHAMMERPOUND] = WAV_ZN1HAMMERPOST;
		temp_misc.miscsfx[sfxSUBSCR_ITEM_ASSIGN] = WAV_PLACE;
		temp_misc.miscsfx[sfxSUBSCR_CURSOR_MOVE] = WAV_CHIME;
		temp_misc.miscsfx[sfxREFILL] = WAV_MSG;
		temp_misc.miscsfx[sfxDRAIN] = WAV_MSG;
	}
	if(s_version < 16)
	{
		temp_misc.miscsfx[sfxTAP] = WAV_ZN1TAP;
		temp_misc.miscsfx[sfxTAP_HOLLOW] = WAV_ZN1TAP2;
	}
	
	if(s_version < 17)
	{
		temp_misc.status_names[STATUS_JINX_MELEE] = "Melee Jinx";
		temp_misc.status_effects[STATUS_JINX_MELEE].jinx_melee = true;
		temp_misc.status_names[STATUS_JINX_ITEM] = "Item Jinx";
		temp_misc.status_effects[STATUS_JINX_ITEM].jinx_item = true;
		temp_misc.status_names[STATUS_JINX_SHIELD] = "Shield Jinx";
		temp_misc.status_effects[STATUS_JINX_SHIELD].jinx_shield = true;
		temp_misc.status_names[STATUS_STUN] = "Stun";
		temp_misc.status_effects[STATUS_STUN].stun = true;
		temp_misc.status_names[STATUS_BUNNY] = "Bunny";
		temp_misc.status_effects[STATUS_BUNNY].bunny = true;
	}
	else
	{
		for(int q = 0; q < NUM_STATUSES; ++q)
		{
			if(!p_getcstr(&temp_misc.status_names[q], f))
				return qe_invalid;
			if(auto ret = temp_misc.status_effects[q].read(f,s_version))
				return ret;
		}
	}
	
	if (!should_skip)
		*Misc = temp_misc;
	
	return 0;
}

extern char *item_string[MAXITEMS];
extern const char *old_item_string[iLast];
extern char *weapon_string[MAXWPNS];
extern const char *old_weapon_string[wLast];

int32_t readitems(PACKFILE *f, word version, word build)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_items);

    byte padding;
    int32_t  dummy;
    word items_to_read=MAXITEMS;
    itemdata tempitem;
    word s_version=0, s_cversion=0;
    word dummy_word;
    
    if(version < 0x186)
    {
        items_to_read=64;
    }
    
    if(version > 0x192)
    {
        items_to_read=0;
        
        //section version info
        if(!p_igetw(&s_version,f))
        {
            return qe_invalid;
        }
	
	FFCore.quest_format[vItems] = s_version;
        
        if(!p_igetw(&s_cversion,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy,f))
        {
            return qe_invalid;
        }
        
        //finally...  section data
        if(!p_igetw(&items_to_read,f))
        {
            return qe_invalid;
        }

        if (items_to_read > MAXITEMS)
        {
            return qe_invalid;
        }
    }
    
    if(s_version>1)
    {
        for(int32_t i=0; i<items_to_read; i++)
        {
            char tempname[64];
            
            if(!pfread(tempname, 64, f))
            {
                return qe_invalid;
            }
            
			item_string[i][0] = '\0';
			strncat(item_string[i], tempname, 64 - 1);
        }
    }
    else if (!should_skip)
    {
		for(int32_t i=0; i<MAXITEMS; i++)
		{
			reset_itemname(i);
		}
    }
    
	if (!should_skip)
	for(int32_t i=0; i<MAXITEMS; i++)
	{
		itemdata& id = itemsbuf[i];
		memset(&id, 0, sizeof(itemdata));
		id.count=-1;
		id.playsound=WAV_SCALE;
		reset_itembuf(&id,i);
	}
    
    for(int32_t i=0; i<items_to_read; i++)
    {
        memset(&tempitem, 0, sizeof(itemdata));
        reset_itembuf(&tempitem,i);
        
	    
		if ( s_version > 35 ) //expanded tiles	
		{    
			if(!p_igetl(&tempitem.tile,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			if(!p_igetw(&tempitem.tile,f))
			{
				return qe_invalid;
			}
		}
		
        if(!p_getc(&tempitem.misc_flags,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&tempitem.csets,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&tempitem.frames,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&tempitem.speed,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&tempitem.delay,f))
        {
            return qe_invalid;
        }
        
        if(version < 0x193)
        {
            if(!p_getc(&padding,f))
            {
                return qe_invalid;
            }
            
            if((version < 0x192)||((version == 0x192)&&(build<186)))
            {
				if (should_skip)
					continue;

                switch(i)
                {
                case iShield:
                    tempitem.ltm=get_qr(qr_BSZELDA)?-12:10;
                    break;
                    
                case iMShield:
                    tempitem.ltm=get_qr(qr_BSZELDA)?-6:-10;
                    break;
                    
                default:
                    tempitem.ltm=0;
                    break;
                }
                
                tempitem.count=-1;
				tempitem.flags=item_none;
                tempitem.wpn=tempitem.wpn2=tempitem.wpn3=tempitem.wpn3=tempitem.pickup_hearts=
                                                tempitem.misc1=tempitem.misc2=tempitem.usesound=0;
                tempitem.family=0xFF;
                tempitem.playsound=WAV_SCALE;
                reset_itembuf(&tempitem,i);
                
				memcpy(&itemsbuf[i], &tempitem, sizeof(itemdata));
                
                continue;
            }
        }
        
        if(!p_igetl(&tempitem.ltm,f))
        {
            return qe_invalid;
        }
        
        if(version < 0x193)
        {
            for(int32_t q=0; q<12; q++)
            {
                if(!p_getc(&padding,f))
                {
                    return qe_invalid;
                }
            }
        }
        
        if(s_version>1)
        {
			if ( s_version >= 31 )
			{
				if(!p_igetl(&tempitem.family,f))
				{
					return qe_invalid;
				}    
			}
            else
			{		    
				if(!p_getc(&tempitem.family,f))
				{
					return qe_invalid;
				}
            }
            if(s_version < 16)
                if(tempitem.family == 0xFF)
                    tempitem.family = itype_misc;
                    
            if(!p_getc(&tempitem.fam_type,f))
            {
                return qe_invalid;
            }
            
            if(s_version>5)
            {
				if(s_version>=31)
				{
					if(!p_igetl(&tempitem.power,f))
					{
						return qe_invalid;
					}
				}
				else
				{
					if(!p_getc(&tempitem.power,f))
					{
					return qe_invalid;
					}
				}
						
				//converted flags from 16b to 32b -Z
				if ( s_version < 41 )
				{
					if(!p_igetw(&tempitem.flags,f))
					{
						return qe_invalid;
					}
				}
				else
				{
					if(!p_igetl(&tempitem.flags,f))
					{
						return qe_invalid;
					}
				}
            }
            else
            {
                //tempitem.power = tempitem.fam_type;
                char tempchar;
                
                if(!p_getc(&tempchar,f))
                {
                    return qe_invalid;
                }
                
                if (tempchar) tempitem.flags |= item_gamedata;
            }
            
            if(!p_igetw(&tempitem.script,f))
            {
                return qe_invalid;
            }
            
            if(s_version<=3)
            {
                if(tempitem.script > NUMSCRIPTITEM)
                {
                    tempitem.script = 0;
                }
            }
            
            if(!p_getc(&tempitem.count,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&tempitem.amount,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&tempitem.collect_script,f))
            {
                return qe_invalid;
            }
            
            if(s_version<=3)
            {
                if(tempitem.collect_script > NUMSCRIPTITEM)
                {
                    tempitem.collect_script = 0;
                }
            }
            
            if(!p_igetw(&tempitem.setmax,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&tempitem.max,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&tempitem.playsound,f))
            {
                return qe_invalid;
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_igetl(&tempitem.initiald[j],f))
                {
                    return qe_invalid;
                }
            }
            
            for(int32_t j=0; j<2; j++)
            {
				byte temp;
                if(!p_getc(&temp,f))
                {
                    return qe_invalid;
                }
            }
            
            if(s_version>4)
            {
                if(s_version>5)
                {
                    if(!p_getc(&tempitem.wpn,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(!p_getc(&tempitem.wpn2,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(!p_getc(&tempitem.wpn3,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(!p_getc(&tempitem.wpn4,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(s_version>=15)
                    {
                        if(!p_getc(&tempitem.wpn5,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_getc(&tempitem.wpn6,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_getc(&tempitem.wpn7,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_getc(&tempitem.wpn8,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_getc(&tempitem.wpn9,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_getc(&tempitem.wpn10,f))
                        {
                            return qe_invalid;
                        }
                    }
                    
                    if(!p_getc(&tempitem.pickup_hearts,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(s_version<15)
                    {
                        if(!p_igetw(&dummy_word,f))
                        {
                            return qe_invalid;
                        }
                        
                        tempitem.misc1=dummy_word;
                        
                        if(!p_igetw(&dummy_word,f))
                        {
                            return qe_invalid;
                        }
                        
                        tempitem.misc2=dummy_word;
                    }
                    else
                    {
                        if(!p_igetl(&tempitem.misc1,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc2,f))
                        {
                            return qe_invalid;
                        }
                        
                        // Version 24: sh_ice -> sh_script; previously, all shields could block script weapons
                        if(s_version<24)
                        {
                            if(tempitem.family==itype_shield)
                            {
                                tempitem.misc1|=sh_script;
                            }
                        }
                    }
                    
					if(s_version < 53)
					{
						byte tempbyte;
						if(!p_getc(&tempbyte,f))
						{
							return qe_invalid;
						}
						tempitem.cost_amount[0] = tempbyte;
					}
					else
					{
						for(auto q = 0; q < 2; ++q)
						{
							if(!p_igetw(&tempitem.cost_amount[q],f))
							{
								return qe_invalid;
							}
						}
					}
                }
                else
                {
                    char tempchar;
                    
                    if(!p_getc(&tempchar,f))
                    {
                        return qe_invalid;
                    }
                    
                    if (tempchar) tempitem.flags |= item_edible;
                }
                
                // June 2007: more misc. attributes
                if(s_version>=12)
                {
                    if(s_version<15)
                    {
                        if(!p_igetw(&dummy_word,f))
                        {
                            return qe_invalid;
                        }
                        
                        tempitem.misc3=dummy_word;
                        
                        if(!p_igetw(&dummy_word,f))
                        {
                            return qe_invalid;
                        }
                        
                        tempitem.misc4=dummy_word;
                    }
                    else
                    {
                        if(!p_igetl(&tempitem.misc3,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc4,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc5,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc6,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc7,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc8,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc9,f))
                        {
                            return qe_invalid;
                        }
                        
                        if(!p_igetl(&tempitem.misc10,f))
                        {
                            return qe_invalid;
                        }
                    }
                    
                    if(!p_getc(&tempitem.usesound,f))
                    {
                        return qe_invalid;
                    }
					
					if(s_version >= 49)
					{
						if(!p_getc(&tempitem.usesound2,f))
						{
							return qe_invalid;
						}
					}
					else tempitem.usesound2 = 0;
					
					if(s_version < 50 && tempitem.family == itype_mirror)
					{
						//Split continue/dmap warp effect/sfx, port for old
						tempitem.misc2 = tempitem.misc1;
						tempitem.usesound2 = tempitem.usesound;
					}
                }
            }
	    
			if ( s_version >= 26 )  //! New itemdata vars for weapon editor. -Z
			{			// temp.useweapon, temp.usedefence, temp.weaprange, temp.weap_pattern[ITEM_MOVEMENT_PATTERNS]
				if(!p_getc(&tempitem.useweapon,f))
				{
					return qe_invalid;
				}
				if(!p_getc(&tempitem.usedefence,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weaprange,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weapduration,f))
				{
					return qe_invalid;
				}
				for ( int32_t q = 0; q < ITEM_MOVEMENT_PATTERNS; q++ )
				{
					if(!p_igetl(&tempitem.weap_pattern[q],f))
					{
						return qe_invalid;
					}
				}
			}
			
			if ( s_version >= 27 )  //! New itemdata vars for weapon editor. -Z
			{			// temp.useweapon, temp.usedefence, temp.weaprange, temp.weap_pattern[ITEM_MOVEMENT_PATTERNS]
				if(!p_igetl(&tempitem.duplicates,f))
				{
					return qe_invalid;
				}
				for ( int32_t q = 0; q < INITIAL_D; q++ )
				{
					if(!p_igetl(&tempitem.weap_initiald[q],f))
					{
						return qe_invalid;
					}
				}
				for ( int32_t q = 0; q < 2; q++ )
				{
					byte temp;
					if(!p_getc(&temp,f))
					{
						return qe_invalid;
					}
				}
				
				if(!p_getc(&tempitem.drawlayer,f))
				{
					return qe_invalid;
				}
				
				
				if(!p_igetl(&tempitem.hxofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.hyofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.hxsz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.hysz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.hzsz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.xofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.yofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_hxofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_hyofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_hxsz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_hysz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_hzsz,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_xofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_yofs,f))
				{
					return qe_invalid;
				}
				if(!p_igetw(&tempitem.weaponscript,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.wpnsprite,f))
				{
					return qe_invalid;
				}
				auto num_cost_tmr = (s_version > 52 ? 2 : 1);
				for(auto q = 0; q < num_cost_tmr; ++q)
				{
					if(!p_igetl(&tempitem.magiccosttimer[q],f))
					{
						return qe_invalid;
					}
				}
				for(auto q = num_cost_tmr; q < 2; ++q)
					tempitem.magiccosttimer[q] = 0;
			}
			if ( s_version >= 28 )  //! New itemdata vars for weapon editor. -Z
			{
				//Item Size FLags, TileWidth, TileHeight
				if(!p_igetl(&tempitem.overrideFLAGS,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.tilew,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.tileh,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 29 )  //! More new vars. 
			{
				//Item Size FLags, TileWidth, TileHeight
				if(!p_igetl(&tempitem.weapoverrideFLAGS,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_tilew,f))
				{
					return qe_invalid;
				}
				if(!p_igetl(&tempitem.weap_tileh,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 30 )  //! More new vars. 
			{
				//Pickup Type
				if(!p_igetl(&tempitem.pickup,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 32 )  //! More new vars. 
			{
				//Pickup Type
				if(!p_igetw(&tempitem.pstring,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 33 )  //! More new vars. 
			{
				//Pickup Type
				if(!p_igetw(&tempitem.pickup_string_flags,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 34 )  //! cost counter
			{
				if(s_version < 53)
				{
					if(!p_getc(&tempitem.cost_counter[0],f))
					{
						return qe_invalid;
					}
				}
				else
				{
					for(auto q = 0; q < 2; ++q)
					{
						if(!p_getc(&tempitem.cost_counter[q],f))
						{
							return qe_invalid;
						}
					}
				}
			}
			if ( s_version >= 44 )  //! sprite scripts
			{
				for ( int32_t q = 0; q < 8; q++ )
				{
					for ( int32_t w = 0; w < 65; w++ )
					{
						if(!p_getc(&(tempitem.initD_label[q][w]),f))
						{
							return qe_invalid;
						} 
					}
					for ( int32_t w = 0; w < 65; w++ )
					{
						if(!p_getc(&(tempitem.weapon_initD_label[q][w]),f))
						{
							return qe_invalid;
						} 
					}
					for ( int32_t w = 0; w < 65; w++ )
					{
						if(!p_getc(&(tempitem.sprite_initD_label[q][w]),f))
						{
							return qe_invalid;
						} 
					}
					if(!p_igetl(&(tempitem.sprite_initiald[q]),f))
					{
						return qe_invalid;
					}
					
				}
				for ( int32_t q = 0; q < 2; q++ )
				{
					byte temp;
					if(!p_getc(&temp,f))
					{
						return qe_invalid;
					}
				}
				//Pickup Type
				if(!p_igetw(&tempitem.sprite_script,f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 48 )  //! pickup flags
			{
				if(!p_getc(&(tempitem.pickupflag),f))
				{
					return qe_invalid;
				}
			}
			if ( s_version >= 57 )
			{
				std::string str;
				if(!p_getcstr(&str,f))
					return qe_invalid;
				strncpy(tempitem.display_name,str.c_str(),255);
			}
        }
        else
        {
            tempitem.count=-1;
            tempitem.family=itype_misc;
			tempitem.flags=item_none;
            tempitem.wpn=tempitem.wpn2=tempitem.wpn3=tempitem.wpn3=tempitem.pickup_hearts=tempitem.misc1=tempitem.misc2=tempitem.usesound=0;
            tempitem.playsound=WAV_SCALE;
            reset_itembuf(&tempitem,i);
        }
		
		if(s_version >= 58)
		{
			for(int q = 0; q < WPNSPR_MAX; ++q)
			{
				if(!p_getc(&tempitem.burnsprs[q],f))
					return qe_invalid;
				if(s_version >= 59)
					if(!p_getc(&tempitem.light_rads[q],f))
						return qe_invalid;
			}
		}
        
		if (!should_skip)
		{
			if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
			{
				tempitem.script = 0;
				tempitem.weaponscript = 0;
				for(int q = 0; q < 8; ++q)
				{
					tempitem.initiald[q] = 0;
					tempitem.weap_initiald[q] = 0;
				}
			}
			memcpy(&itemsbuf[i], &tempitem, sizeof(itemdata));
		}
    }

	if (should_skip)
		return 0;
    
    //////////////////////////////////////////////////////
    // Now do any updates because of new item additions
    // (These can't be done above because items_to_read
    // might be too low.)
    //////////////////////////////////////////////////////
	for(int32_t i=0; i<MAXITEMS; i++)
	{
		memcpy(&tempitem, &itemsbuf[i], sizeof(itemdata));
		
		//Account for older quests that didn't have an actual item for the used letter
		if(s_version < 2 && i==iLetterUsed)
		{
			reset_itembuf(&tempitem, iLetterUsed);
			strcpy(item_string[i],old_item_string[i]);
			tempitem.tile = itemsbuf[iLetter].tile;
			tempitem.csets = itemsbuf[iLetter].csets;
			tempitem.misc_flags = itemsbuf[iLetter].misc_flags;
			tempitem.frames = itemsbuf[iLetter].frames;
			tempitem.speed = itemsbuf[iLetter].speed;
			tempitem.ltm = itemsbuf[iLetter].ltm;
		}
		
		if(s_version < 3)
		{
			switch(i)
			{
				case iRocsFeather:
				case iHoverBoots:
				case iSpinScroll:
				case iL2SpinScroll:
				case iCrossScroll:
				case iQuakeScroll:
				case iL2QuakeScroll:
				case iWhispRing:
				case iL2WhispRing:
				case iChargeRing:
				case iL2ChargeRing:
				case iPerilScroll:
				case iWalletL3:
				case iQuiverL4:
				case iBombBagL4:
				case iBracelet:
				case iL2Bracelet:
				case iOldGlove:
				case iL2Ladder:
				case iWealthMedal:
				case iL2WealthMedal:
				case iL3WealthMedal:
					reset_itembuf(&tempitem, i);
					strcpy(item_string[i],old_item_string[i]);
					break;
					
				case iSShield:
					reset_itembuf(&tempitem, i);
					strcpy(item_string[i],old_item_string[i]);
					strcpy(item_string[iShield],old_item_string[iShield]);
					strcpy(item_string[iMShield],old_item_string[iMShield]);
					break;
			}
		}
		
		if(s_version < 5)
		{
			switch(i)
			{
				case iHeartRing:
				case iL2HeartRing:
				case iL3HeartRing:
				case iMagicRing:
				case iL2MagicRing:
				case iL3MagicRing:
				case iL4MagicRing:
					reset_itembuf(&tempitem, i);
					strcpy(item_string[i],old_item_string[i]);
					break;
			}
		}
		
		if(s_version < 6)  // April 2007: Advanced item editing capabilities.
		{
			if(i!=iBPotion && i!=iRPotion)
				if (get_bit(deprecated_rules,32)) tempitem.flags |= item_keep_old;
				
			switch(i)
			{
				case iTriforce:
					tempitem.fam_type=1;
					break;
					
				case iBigTri:
					tempitem.fam_type=0;
					break;
					
				case iBombs:
					tempitem.fam_type=i_bomb;
					tempitem.power=4;
					tempitem.wpn=wBOMB;
					tempitem.wpn2=wBOOM;
					tempitem.misc1 = 50;
					
					if(get_bit(deprecated_rules,qr_SLOWBOMBFUSES_DEP)) tempitem.misc1 = 200;
					
					break;
					
				case iSBomb:
					tempitem.fam_type=i_sbomb;
					tempitem.power=16;
					tempitem.wpn=wSBOMB;
					tempitem.wpn2=wSBOOM;
					tempitem.misc1 = 50;
					
					if(get_bit(deprecated_rules,qr_SLOWBOMBFUSES_DEP)) tempitem.misc1 = 400;
					
					break;
					
				case iBook:
					if(get_bit(deprecated_rules, qr_FIREMAGICSPRITE_DEP))
						tempitem.wpn = wFIREMAGIC;
						
					break;
					
				case iSArrow:
					tempitem.wpn2 = get_bit(deprecated_rules,27) ? wSSPARKLE : 0; //qr_SASPARKLES
					tempitem.power=4;
					tempitem.flags|=item_gamedata;
					tempitem.wpn=wSARROW;
					break;
					
				case iGArrow:
					tempitem.wpn2 = get_bit(deprecated_rules,28) ? wGSPARKLE : 0; //qr_GASPARKLES
					tempitem.power=8;
					tempitem.flags|=(item_gamedata|item_flag1);
					tempitem.wpn=wGARROW;
					break;
					
				case iBrang:
					tempitem.power=0;
					tempitem.wpn=wBRANG;
					tempitem.misc1=36;
					break;
					
				case iMBrang:
					tempitem.wpn2 = get_bit(deprecated_rules,29) ? wMSPARKLE : 0; //qr_MBSPARKLES
					tempitem.power=0;
					tempitem.wpn=wMBRANG;
					break;
					
				case iFBrang:
					tempitem.wpn3 = get_bit(deprecated_rules,30) ? wFSPARKLE : 0; //qr_FBSPARKLES
					tempitem.power=2;
					tempitem.wpn=wFBRANG;
					break;
					
				case iBoots:
					tempitem.cost_amount[0] = get_bit(deprecated_rules,qr_MAGICBOOTS_DEP) ? 1 : 0;
					tempitem.power=7;
					break;
					
				case iWand:
					tempitem.cost_amount[0] = get_bit(deprecated_rules,qr_MAGICWAND_DEP) ? 8 : 0;
					tempitem.power=2;
					tempitem.wpn=wWAND;
					tempitem.wpn3=wMAGIC;
					break;
					
				case iBCandle:
					tempitem.cost_amount[0] = get_bit(deprecated_rules,qr_MAGICCANDLE_DEP) ? 4 : 0;
					tempitem.power=1;
					tempitem.flags|=(item_gamedata|item_flag1);
					tempitem.wpn3=wFIRE;
					break;
					
				case iRCandle:
					tempitem.cost_amount[0] = get_bit(deprecated_rules,qr_MAGICCANDLE_DEP) ? 4 : 0;
					tempitem.power=1;
					tempitem.wpn3=wFIRE;
					break;
					
				case iSword:
					tempitem.power=1;
					tempitem.flags|= item_flag4 |item_flag2;
					tempitem.wpn=tempitem.wpn3=wSWORD;
					tempitem.wpn2=wSWORDSLASH;
					break;
					
				case iWSword:
					tempitem.power=2;
					tempitem.flags|= item_flag4 |item_flag2;
					tempitem.wpn=tempitem.wpn3=wWSWORD;
					tempitem.wpn2=wWSWORDSLASH;
					break;
					
				case iMSword:
					tempitem.power=4;
					tempitem.flags|= item_flag4 |item_flag2;
					tempitem.wpn=tempitem.wpn3=wMSWORD;
					tempitem.wpn2=wMSWORDSLASH;
					break;
					
				case iXSword:
					tempitem.power=8;
					tempitem.flags|= item_flag4 |item_flag2;
					tempitem.wpn=tempitem.wpn3=wXSWORD;
					tempitem.wpn2=wXSWORDSLASH;
					break;
					
				case iDivineProtection:
					tempitem.flags |= get_bit(deprecated_rules,qr_FLICKERINGDIVINEPROTECTIONROCKET_DEP) ? item_flag1 : item_none;
					tempitem.flags |= get_bit(deprecated_rules,qr_TRANSLUCENTDIVINEPROTECTIONROCKET_DEP) ? item_flag2 : item_none;
					tempitem.wpn=wDIVINEPROTECTION1A;
					tempitem.wpn2=wDIVINEPROTECTION1B;
					tempitem.wpn3=wDIVINEPROTECTIONS1A;
					tempitem.wpn4=wDIVINEPROTECTIONS1B;
					tempitem.wpn6=wDIVINEPROTECTION2A;
					tempitem.wpn7=wDIVINEPROTECTION2B;
					tempitem.wpn8=wDIVINEPROTECTIONS2A;
					tempitem.wpn9=wDIVINEPROTECTIONS2B;
					tempitem.wpn5 = iwDivineProtectionShieldFront;
					tempitem.wpn10 = iwDivineProtectionShieldBack;
					tempitem.misc1=512;
					tempitem.cost_amount[0]=64;
					break;
					
				case iLens:
					tempitem.misc1=60;
					tempitem.flags |= get_qr(qr_ENABLEMAGIC) ? item_none : item_rupee_magic;
					tempitem.cost_amount[0] = get_qr(qr_ENABLEMAGIC) ? 2 : 1;
					break;
					
				case iArrow:
					tempitem.power=2;
					tempitem.wpn=wARROW;
					break;
					
				case iHoverBoots:
					tempitem.misc1=45;
					tempitem.wpn=iwHover;
					break;
					
				case iDivineFire:
					tempitem.power=8;
					tempitem.wpn=wDIVINEFIRE1A;
					tempitem.wpn2=wDIVINEFIRE1B;
					tempitem.wpn3=wDIVINEFIRES1A;
					tempitem.wpn4=wDIVINEFIRES1B;
					tempitem.misc1 = 32;
					tempitem.misc2 = 200;
					tempitem.cost_amount[0]=32;
					break;
					
				case iDivineEscape:
					tempitem.cost_amount[0]=32;
					break;
					
				case iHookshot:
					tempitem.power=0;
					tempitem.flags&=~item_flag1;
					tempitem.wpn=wHSHEAD;
					tempitem.wpn2=wHSCHAIN_H;
					tempitem.wpn4=wHSHANDLE;
					tempitem.wpn3=wHSCHAIN_V;
					tempitem.misc1=50;
					tempitem.misc2=100;
					break;
					
				case iLongshot:
					tempitem.power=0;
					tempitem.flags&=~item_flag1;
					tempitem.wpn=wLSHEAD;
					tempitem.wpn2=wLSCHAIN_H;
					tempitem.wpn4=wLSHANDLE;
					tempitem.wpn3=wLSCHAIN_V;
					tempitem.misc1=99;
					tempitem.misc2=100;
					break;
					
				case iHammer:
					tempitem.power=4;
					tempitem.wpn=wHAMMER;
					tempitem.wpn2=iwHammerSmack;
					break;
					
				case iCByrna:
					tempitem.power=1;
					tempitem.wpn=wCBYRNA;
					tempitem.wpn2=wCBYRNASLASH;
					tempitem.wpn3=wCBYRNAORB;
					tempitem.misc1=4;
					tempitem.misc2=16;
					tempitem.misc3=1;
					tempitem.cost_amount[0]=1;
					break;
					
				case iWhistle:
					tempitem.wpn=wWIND;
					tempitem.misc1=3;
					tempitem.flags|=item_flag1;
					break;
					
				case iBRing:
					tempitem.power=2;
					tempitem.misc1=spBLUE;
					break;
					
				case iRRing:
					tempitem.power=4;
					tempitem.misc1=spRED;
					break;
					
				case iGRing:
					tempitem.power=8;
					tempitem.misc1=spGOLD;
					break;
					
				case iSpinScroll:
					tempitem.power = 2;
					tempitem.misc1 = 1;
					break;
					
				case iL2SpinScroll:
					tempitem.family=itype_spinscroll2;
					tempitem.fam_type=1;
					tempitem.cost_amount[0]=8;
					tempitem.power=2;
					tempitem.misc1 = 20;
					break;
					
				case iQuakeScroll:
					tempitem.misc1=0x10;
					tempitem.misc2=64;
					break;
					
				case iL2QuakeScroll:
					tempitem.family=itype_quakescroll2;
					tempitem.fam_type=1;
					tempitem.power = 2;
					tempitem.misc1=0x20;
					tempitem.misc2=192;
					tempitem.cost_amount[0]=8;
					break;
					
				case iChargeRing:
					tempitem.misc1=64;
					tempitem.misc2=128;
					break;
					
				case iL2ChargeRing:
					tempitem.misc1=32;
					tempitem.misc2=64;
					break;
					
				case iOldGlove:
					tempitem.flags |= item_flag1;
					
					//fallthrough
				case iBombBagL4:
				case iWalletL3:
				case iQuiverL4:
				case iBracelet:
					tempitem.power = 1;
					break;
					
				case iL2Bracelet:
					tempitem.power = 2;
					break;
					
				case iMKey:
					tempitem.power=0xFF;
					tempitem.flags |= item_flag1;
					break;
			}
		}
		
		if(s_version < 7)
		{
			switch(i)
			{
				case iStoneAgony:
				case iStompBoots:
				case iPerilRing:
				case iWhimsicalRing:
				{
					reset_itembuf(&tempitem, i);
					strcpy(item_string[i],old_item_string[i]);
					break;
				}
			}
		}
		
		if(s_version < 8) // May 2007: Some corrections.
		{
			switch(i)
			{
				case iMShield:
					tempitem.misc1|=sh_flame;
					tempitem.misc2|=sh_fireball|sh_magic;
					
					if(get_qr(qr_SWORDMIRROR))
					{
						tempitem.misc2 |= sh_sword;
					}
					
					// fallthrough
				case iShield:
					tempitem.misc1|=sh_fireball|sh_sword|sh_magic;
					
					// fallthrough
				case iSShield:
					tempitem.misc1|=sh_rock|sh_arrow|sh_brang|sh_script;
					
					if(get_bit(deprecated_rules,102))  //qr_REFLECTROCKS
					{
						tempitem.misc2 |= sh_rock;
					}
					
					break;
					
				case iWhispRing:
					tempitem.power=1;
					tempitem.flags|=item_gamedata|item_flag1;
					tempitem.misc1 = 3;
					break;
					
				case iL2WhispRing:
					tempitem.power=0;
					tempitem.flags|=item_gamedata|item_flag1;
					tempitem.misc1 = 3;
					break;
					
				case iL2Ladder:
				case iBow:
				case iCByrna:
					tempitem.power = 1;
					break;
			}
		}
		
		if(s_version < 9 && i==iClock)
		{
			tempitem.misc1 = get_bit(deprecated_rules, qr_TEMPCLOCKS_DEP) ? 256 : 0;
		}
		
		//add the misc flag for bomb
		if(s_version < 10 && tempitem.family == itype_bomb)
		{
			tempitem.flags = (tempitem.flags & ~item_flag1) | (get_qr(qr_LONGBOMBBOOM_DEP) ? item_flag1 : item_none);
		}
		
		if(s_version < 11 && tempitem.family == itype_triforcepiece)
		{
			tempitem.flags = (tempitem.fam_type ? item_gamedata : item_none);
			tempitem.playsound = (tempitem.fam_type ? WAV_SCALE : WAV_CLEARED);
		}
		
		if(s_version < 12) // June 2007: More Misc. attributes.
		{
			switch(i)
			{
			case iFBrang:
				tempitem.misc4 |= sh_fireball|sh_sword|sh_magic;
				
				//fallthrough
			case iMBrang:
				tempitem.misc3 |= sh_sword|sh_magic;
				
				//fallthrough
			case iHookshot:
			case iLongshot:
				//fallthrough
				tempitem.misc3 |= sh_fireball;
				
			case iBrang:
				tempitem.misc3 |= sh_brang|sh_rock|sh_arrow;
				break;
			}
			
			switch(tempitem.family)
			{
			case itype_hoverboots:
				tempitem.usesound = WAV_ZN1HOVER;
				break;
				
			case itype_wand:
				tempitem.usesound = WAV_WAND;
				break;
				
			case itype_book:
				tempitem.usesound = WAV_FIRE;
				break;
				
			case itype_arrow:
				tempitem.usesound = WAV_ARROW;
				break;
				
			case itype_hookshot:
				tempitem.usesound = WAV_HOOKSHOT;
				break;
				
			case itype_brang:
				tempitem.usesound = WAV_BRANG;
				break;
				
			case itype_shield:
				tempitem.usesound = WAV_CHINK;
				break;
				
			case itype_sword:
				tempitem.usesound = WAV_SWORD;
				break;
				
			case itype_whistle:
				tempitem.usesound = WAV_WHISTLE;
				break;
				
			case itype_hammer:
				tempitem.usesound = WAV_HAMMER;
				break;
				
			case itype_divinefire:
				tempitem.usesound = WAV_ZN1DIVINEFIRE;
				break;
				
			case itype_divineescape:
				tempitem.usesound = WAV_ZN1DIVINEESCAPE;
				break;
				
			case itype_divineprotection:
				tempitem.usesound = WAV_ZN1DIVINEPROTECTION1;
				break;
				
			case itype_bomb:
			case itype_sbomb:
			case itype_quakescroll:
			case itype_quakescroll2:
				tempitem.usesound = WAV_BOMB;
				break;
				
			case itype_spinscroll:
			case itype_spinscroll2:
				tempitem.usesound = WAV_ZN1SPINATTACK;
				break;
			}
		}
		
		if(s_version < 13) // July 2007
		{
			if(tempitem.family == itype_whistle)
			{
				tempitem.misc1 = (tempitem.power==2 ? 4 : 3);
				tempitem.power = 1;
				tempitem.flags|=item_flag1;
			}
			else if(tempitem.family == itype_wand)
				tempitem.flags|=item_flag1;
			else if(tempitem.family == itype_book)
			{
				tempitem.flags|=item_flag1;
				tempitem.power = 2;
			}
		}
		
		if(s_version < 14) // August 2007
		{
			if(tempitem.family == itype_fairy)
			{
				tempitem.usesound = WAV_SCALE;
				
				if(tempitem.fam_type)
					tempitem.misc3=50;
			}
			else if(tempitem.family == itype_potion)
			{
				tempitem.flags |= item_gain_old;
			}
		}
		
		if(s_version < 17) // November 2007
		{
			if(tempitem.family == itype_candle && !tempitem.wpn3)
			{
				tempitem.wpn3 = wFIRE;
			}
			else if(tempitem.family == itype_arrow && tempitem.power>4)
			{
				tempitem.flags|=item_flag1;
			}
		}
		
		if(s_version < 18) // New Year's Eve 2007
		{
			if(tempitem.family == itype_whistle)
				tempitem.misc2 = 8; // Use the Whistle warp ring
			else if(tempitem.family == itype_bait)
				tempitem.misc1 = 768; // Frames until it goes
			else if(tempitem.family == itype_triforcepiece)
			{
				if(tempitem.flags & item_gamedata)
				{
					tempitem.misc2 = 1; // Cutscene 1
					tempitem.flags |= item_flag1; // Side Warp Out
				}
			}
		}
		
		if(s_version < 19)  // January 2008
		{
			if(tempitem.family == itype_divineprotection)
			{
				if (get_bit(deprecated_rules,qr_NOBOMBPALFLASH+1)) tempitem.flags |= item_flag3;
				if (get_bit(deprecated_rules,qr_NOBOMBPALFLASH+2)) tempitem.flags |= item_flag4;
			}
		}
		
		if(s_version < 20)  // October 2008
		{
			if(tempitem.family == itype_divineprotection)
			{
				tempitem.wpn6=wDIVINEPROTECTION2A;
				tempitem.wpn7=wDIVINEPROTECTION2B;
				tempitem.wpn8=wDIVINEPROTECTIONS2A;
				tempitem.wpn9=wDIVINEPROTECTIONS2B;
				tempitem.wpn5 = iwDivineProtectionShieldFront;
				tempitem.wpn10 = iwDivineProtectionShieldBack;
			}
		}
		
		if(s_version < 21)  // November 2008
		{
			if(tempitem.flags & 0x0100)  // item_slash
			{
				tempitem.flags &= ~item_unused;
				
				if(tempitem.family == itype_sword ||
						tempitem.family == itype_wand ||
						tempitem.family == itype_candle ||
						tempitem.family == itype_cbyrna)
				{
					tempitem.flags |= item_flag4;
				}
			}
		}
		
		if(s_version < 22)  // September 2009
		{
			if(tempitem.family == itype_sbomb || tempitem.family == itype_bomb)
			{
				tempitem.misc3 = tempitem.power/2;
			}
		}
		
		if(s_version < 23)    // March 2011
		{
			if(tempitem.family == itype_divinefire)
				tempitem.wpn5 = wFIRE;
			else if(tempitem.family == itype_book)
				tempitem.wpn2 = wFIRE;
		}
		
		// Version 25: Bomb bags were acting as though "super bombs also" was checked
		// whether it was or not, and a lot of existing quests depended on the
		// incorrect behavior.
		if(s_version < 25)    // January 2012
		{
			if(tempitem.family == itype_bombbag)
				tempitem.flags |= item_flag1;
				
			if(tempitem.family == itype_divinefire)
				tempitem.flags |= item_flag3; // Sideview gravity flag
		}
		
		if( version < 0x254) //Nuke greyed-out flags/values from <=2.53, in case they are used in 2.54/2.55
		{
			switch(tempitem.family)
			{
				case itype_sword:
				{
					tempitem.flags &= ~(item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_brang:
				{
					tempitem.flags &= ~(item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_arrow:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_candle:
				{
					tempitem.flags &= ~ (item_flag3 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_whistle:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_bait:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_letter:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_potion:
				{
					tempitem.flags &= ~ (item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_wand:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_ring:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_wallet:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_amulet:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_shield:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_bow:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_raft:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_ladder:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_book:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_magickey:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_bracelet:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_flippers:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_boots:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_hookshot:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_lens:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_hammer:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_divinefire:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_divineescape:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_divineprotection:
				{
					tempitem.flags &= ~ (item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					break;
				}
				case itype_bomb:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_sbomb:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_clock:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_key:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_magiccontainer:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_triforcepiece:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_map:	case itype_compass:	case itype_bosskey:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_quiver:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_lkey:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_cbyrna:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag5);
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_rupee: case itype_arrowammo:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_fairy:
				{
					tempitem.flags &= ~ (item_flag3 | item_flag4 | item_flag5);
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_magic: case itype_heart: case itype_heartcontainer: case itype_heartpiece: case itype_killem: case itype_bombammo:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_bombbag:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_rocs:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_hoverboots:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_spinscroll:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_crossscroll:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_quakescroll:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_whispring:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_chargering:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_perilscroll:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_wealthmedal:
				{
					tempitem.flags &= ~ (item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_heartring:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_magicring:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_spinscroll2:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_quakescroll2:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_agony:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_stompboots:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_whimsicalring:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_perilring:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
				case itype_custom1: case itype_custom2: case itype_custom3: case itype_custom4: case itype_custom5:
				case itype_custom6: case itype_custom7: case itype_custom8: case itype_custom9: case itype_custom10:
				case itype_custom11: case itype_custom12: case itype_custom13: case itype_custom14: case itype_custom15:
				case itype_custom16: case itype_custom17: case itype_custom18: case itype_custom19: case itype_custom20:
				case itype_bowandarrow: case itype_letterpotion: case itype_misc:
				{
					tempitem.flags &= ~ (item_flag1 | item_flag2 | item_flag3 | item_flag4 | item_flag5);
					tempitem.misc1 = 0;
					tempitem.misc2 = 0;
					tempitem.misc3 = 0;
					tempitem.misc4 = 0;
					tempitem.misc5 = 0;
					tempitem.misc6 = 0;
					tempitem.misc7 = 0;
					tempitem.misc8 = 0;
					tempitem.misc9 = 0;
					tempitem.misc10 = 0;
					tempitem.wpn = 0;
					tempitem.wpn2 = 0;
					tempitem.wpn3 = 0;
					tempitem.wpn4 = 0;
					tempitem.wpn5 = 0;
					tempitem.wpn6 = 0;
					tempitem.wpn7 = 0;
					tempitem.wpn8 = 0;
					tempitem.wpn9 = 0;
					tempitem.wpn10 = 0;
					break;
				}
			}
		}
		//Port quest rules to items
		if( s_version <= 31) 
		{
			if(tempitem.family == itype_bomb)
			{
				if ( get_qr(qr_OUCHBOMBS) )  tempitem.flags |= item_flag2;
				else tempitem.flags &= ~ item_flag2;
			}
			else if(tempitem.family == itype_sbomb)
			{
				if ( get_qr(qr_OUCHBOMBS) )  tempitem.flags |= item_flag2;
				else tempitem.flags &= ~ item_flag2;
			}
			
			else if(tempitem.family == itype_brang)
			{
				if ( get_qr(qr_BRANGPICKUP) )  tempitem.flags |= item_flag4;
				else tempitem.flags &= ~ item_flag4;
			}	
			else if(tempitem.family == itype_wand)
			{
				if ( get_qr(qr_NOWANDMELEE) )  tempitem.flags |= item_flag3;
				else tempitem.flags &= ~ item_flag3;
			}
		}
		
		//Port quest rules to items
		if( s_version <= 37) 
		{
			if(tempitem.family == itype_flippers)
			{
				if ( (get_qr(qr_NODIVING)) ) tempitem.flags |= item_flag1;
				else tempitem.flags &= ~ item_flag1;
			}
			else if(tempitem.family == itype_sword)
			{
				if ( (get_qr(qr_QUICKSWORD)) ) tempitem.flags |= item_flag5;
				else tempitem.flags &= ~ item_flag5;
			}
			else if(tempitem.family == itype_wand)
			{
				if ( (get_qr(qr_QUICKSWORD)) ) tempitem.flags |= item_flag5;
				else tempitem.flags &= ~ item_flag5;
			}
			else if(tempitem.family == itype_book || tempitem.family == itype_candle)
			{
				//@Emily: What was qrFIREPROOFHERO2 again, and does that also need to enable this?
				if ( (get_qr(qr_FIREPROOFHERO)) ) tempitem.flags |= item_flag3;
				else tempitem.flags &= ~ item_flag3;
			}
		}
		
		if( s_version < 38)
		{
			if(tempitem.family == itype_brang || tempitem.family == itype_hookshot)
			{
				if(get_qr(qr_BRANGPICKUP)) tempitem.flags |= item_flag4;
				else tempitem.flags &= ~item_flag4;
				
				if(get_qr(qr_Z3BRANG_HSHOT)) tempitem.flags |= item_flag5 | item_flag6;
				else tempitem.flags &= ~(item_flag5|item_flag6);
			} 
			else if(tempitem.family == itype_arrow)
			{
				if(get_qr(qr_BRANGPICKUP)) tempitem.flags |= item_flag4;
				else tempitem.flags &= ~item_flag4;
				
				if(get_qr(qr_Z3BRANG_HSHOT)) tempitem.flags &= ~item_flag2;
				else tempitem.flags |= item_flag2;
			}
		}
		
		if( s_version < 39)
		{
			if(tempitem.family == itype_divinefire || tempitem.family == itype_book || tempitem.family == itype_candle)
			{
				if(get_qr(qr_TEMPCANDLELIGHT)) tempitem.flags |= item_flag5;
				else tempitem.flags &= ~item_flag5;
			}
			else if(tempitem.family == itype_potion)
			{
				if(get_qr(qr_NONBUBBLEMEDICINE))
				{
					tempitem.flags &= ~(item_flag3|item_flag4);
				}
				else
				{
					tempitem.flags |= item_flag3;
					if(get_qr(qr_ITEMBUBBLE))tempitem.flags |= item_flag4;
					else tempitem.flags &= ~item_flag4;
				}
			}
			else if(tempitem.family == itype_triforcepiece)
			{
				if(get_qr(qr_NONBUBBLETRIFORCE))
				{
					tempitem.flags |= item_flag3;
					if(get_qr(qr_ITEMBUBBLE))tempitem.flags |= item_flag4;
					else tempitem.flags &= ~item_flag4;
				}
				else
				{
					tempitem.flags &= ~(item_flag3|item_flag4);
				}
			}
		}
		
		if( s_version < 40)
		{
			if(tempitem.family == itype_ring || tempitem.family == itype_perilring)
			{
				if(get_qr(qr_RINGAFFECTDAMAGE))tempitem.flags |= item_flag1;
				else tempitem.flags &= ~item_flag1;
			} 
			else if(tempitem.family == itype_candle || tempitem.family == itype_sword || tempitem.family == itype_wand || tempitem.family == itype_cbyrna)
			{
				if(get_qr(qr_SLASHFLIPFIX))tempitem.flags |= item_flag8;
				else tempitem.flags &= ~item_flag8;
			}
			if(tempitem.family == itype_sword || tempitem.family == itype_wand || tempitem.family == itype_hammer)
			{
				if(get_qr(qr_NOITEMMELEE))tempitem.flags |= item_flag7;
				else tempitem.flags &= ~item_flag7;
			} 
			else if(tempitem.family == itype_cbyrna)
			{
				tempitem.flags |= item_flag7;
			}
		}
		
		if( s_version < 41 )
		{
			if(tempitem.family == itype_sword)
			{
				if(get_qr(qr_SWORDMIRROR))tempitem.flags |= item_flag9;
				else tempitem.flags &= ~item_flag9;
				
				if(get_qr(qr_SLOWCHARGINGWALK))tempitem.flags |= item_flag10;
				else tempitem.flags &= ~item_flag10;
			}
		}
		
		if( s_version < 42 )
		{
			if(tempitem.family == itype_wand)
			{
				if(get_qr(qr_NOWANDMELEE))tempitem.flags |= item_flag3;
				else tempitem.flags &= ~item_flag3;
				
				tempitem.flags &= ~item_flag6;
			} 
			else if(tempitem.family == itype_hammer)
			{
				tempitem.flags &= ~item_flag3;
			} 
			else if(tempitem.family == itype_cbyrna)
			{
				tempitem.flags |= item_flag3;
				
				tempitem.flags &= ~item_flag6;
			} 
			else if(tempitem.family == itype_sword)
			{
				if(get_qr(qr_MELEEMAGICCOST))tempitem.flags |= item_flag6;
				else tempitem.flags &= ~item_flag6;
			}
		}
		
		if( s_version < 43 )
		{
			if(tempitem.family == itype_whistle)
			{
				if(get_qr(qr_WHIRLWINDMIRROR))tempitem.flags |= item_flag3;
				else tempitem.flags &= ~item_flag3;
			}
		}
		
		if( s_version < 45 )
		{
			if(tempitem.family == itype_flippers)
			{
				tempitem.misc1 = 50; //Dive length, default 50 frames -V
				tempitem.misc2 = 30; //Dive cooldown, default 30 frames -V
			}
		}
		
		if( s_version < 46 )
		{
			if(tempitem.family == itype_raft)
			{
				tempitem.misc1 = 1; //Rafting speed modifier; default 1. Negative slows, positive speeds.
			}
		}
		if ( s_version < 34 )  //! set the default counter for older quests. 
		{
			if ( (tempitem.flags & item_rupee_magic) )
			{
				tempitem.cost_counter[0] = 1;
			}
			else 
			{
				if(get_qr(qr_ENABLEMAGIC))
					tempitem.cost_counter[0] = 4;
				else
				{
					tempitem.cost_amount[0] = 0;
					tempitem.cost_counter[0] = -1;
				}
			}
		}
		
		if ( s_version < 35 ) //new Lens of Truth flags		
		{
			if ( tempitem.family == itype_lens )
			{
				if ( get_qr(qr_RAFTLENS) ) 
				{
					tempitem.flags |= item_flag4;
				}
				if ( get_qr(qr_LENSHINTS) ) 
				{
					tempitem.flags |= item_flag1;
				}
				if ( get_qr(qr_LENSSEESENEMIES) ) 
				{
					tempitem.flags |= item_flag5;
				}
			}
		}
		if ( s_version < 44 ) //InitD Labels and Sprite Script Data
		{
			for ( int32_t q = 0; q < 8; q++ )
			{
				sprintf(tempitem.initD_label[q],"InitD[%d]",q);
				sprintf(tempitem.weapon_initD_label[q],"InitD[%d]",q);
				sprintf(tempitem.sprite_initD_label[q],"InitD[%d]",q);
				tempitem.sprite_initiald[q] = 0;
			}
			tempitem.sprite_script = 0;
		}
		if ( s_version < 47 ) //InitD Labels and Sprite Script Data
		{
			tempitem.pickupflag = 0;
		}
		
		if( s_version < 51 )
		{
			if( tempitem.family == itype_candle )
			{
				tempitem.misc4 = 50; //Step speed
			}
		}
		
		if( s_version < 52 )
		{
			if( tempitem.family == itype_shield )
				tempitem.flags |= item_flag1; //'Block Front' flag
		}
		if(s_version < 53)
		{
			switch(tempitem.family)
			{
				case itype_arrow:
					tempitem.cost_counter[1] = crARROWS;
					tempitem.cost_amount[1] = 1;
					break;
				case itype_bomb:
					tempitem.cost_counter[1] = crBOMBS;
					tempitem.cost_amount[1] = 1;
					break;
				case itype_sbomb:
					tempitem.cost_counter[1] = crSBOMBS;
					tempitem.cost_amount[1] = 1;
					break;
				default:
					tempitem.cost_counter[1] = crNONE;
					tempitem.cost_amount[1] = 0;
			}
			tempitem.magiccosttimer[1] = 0;
		}
		if( s_version < 54 )
		{
			if( tempitem.family == itype_flippers )
				tempitem.misc3 = INT_BTN_A; //'Block Front' flag
		}
		if(s_version < 55)
		{
			switch(tempitem.family)
			{
				case itype_spinscroll:
				case itype_quakescroll:
					tempitem.usesound2 = WAV_ZN1CHARGE;
					break;
				case itype_spinscroll2:
				case itype_quakescroll2:
					tempitem.usesound2 = WAV_ZN1CHARGE2;
					break;
			}
		}
		if(s_version < 56)
		{
			switch(tempitem.family)
			{
				case itype_divinefire:
					SETFLAG(tempitem.flags, item_flag9, version < 0x255); //Strong Fire
					SETFLAG(tempitem.flags, item_flag10, version < 0x250); //Magic Fire
					tempitem.flags |= item_flag11; //Divine Fire
					break;
				case itype_candle:
					SETFLAG(tempitem.flags, item_flag9, tempitem.fam_type > 1); //Strong Fire
					tempitem.flags &= ~item_flag10; //Magic Fire
					tempitem.flags &= ~item_flag11; //Divine Fire
					break;
				case itype_book:
					tempitem.flags |= item_flag9; //Strong Fire
					tempitem.flags |= item_flag10; //Magic Fire
					tempitem.flags &= ~item_flag11; //Divine Fire
					break;
			}
		}
		
		if(tempitem.fam_type==0)  // Always do this
			tempitem.fam_type=1;
			
		memcpy(&itemsbuf[i], &tempitem, sizeof(itemdata));
	}

	return 0;
}

static bool did_init_def_items = false;
void init_def_items()
{
	if(did_init_def_items) return;
	did_init_def_items = true;
	default_items[3].cost_counter[1] = crBOMBS;
	default_items[13].cost_counter[1] = crARROWS;
	default_items[14].cost_counter[1] = crARROWS;
	default_items[48].cost_counter[1] = crSBOMBS;
	default_items[57].cost_counter[1] = crARROWS;
}
void reset_itembuf(itemdata *item, int32_t id)
{
	init_def_items();
    if(id<iLast)
    {
        // Copy everything *EXCEPT* the tile, misc, cset, frames, speed, delay and ltm.
        word tile = item->tile;
        byte miscs = item->misc_flags, cset = item->csets, frames = item->frames, speed = item->speed, delay = item->delay;
        int32_t ltm = item->ltm;
        
        memcpy(item,&default_items[id],sizeof(itemdata));
        item->tile = tile;
        item->misc_flags = miscs;
        item->csets = cset;
        item->frames = frames;
        item->speed = speed;
        item->delay = delay;
        item->ltm = ltm;
    }
}

void reset_itemname(int32_t id)
{
    sprintf(item_string[id],"zz%03d",id);
    
    if(id < iLast)
        strcpy(item_string[id],old_item_string[id]);
}

int32_t readweapons(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_weapons);

    word weapons_to_read=MAXWPNS;
    int32_t dummy;
    byte padding;
    wpndata tempweapon;
    word s_version=0, s_cversion=0;
    
    
    if(Header->zelda_version < 0x186)
    {
        weapons_to_read=64;
    }
    
    if(Header->zelda_version < 0x185)
    {
        weapons_to_read=32;
    }
    
    if(Header->zelda_version > 0x192)
    {
        weapons_to_read=0;
        
        //section version info
        if(!p_igetw(&s_version,f))
        {
            return qe_invalid;
        }
	
	FFCore.quest_format[vWeaponSprites] = s_version;
        
        if(!p_igetw(&s_cversion,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy,f))
        {
            return qe_invalid;
        }
        
        //finally...  section data
        if(!p_igetw(&weapons_to_read,f))
        {
            return qe_invalid;
        }

        if (weapons_to_read > MAXWPNS)
        {
            return qe_invalid;
        }
    }
    
    if(s_version>2)
    {
        for(int32_t i=0; i<weapons_to_read; i++)
        {
            char tempname[64];
            
            if(!pfread(tempname, 64, f))
            {
                return qe_invalid;
            }
            
			weapon_string[i][0] = '\0';
			strncat(weapon_string[i], tempname, 64 - 1);
        }
        
        if(s_version<4)
        {
			strcpy(weapon_string[iwHover],old_weapon_string[iwHover]);
			strcpy(weapon_string[wFIREMAGIC],old_weapon_string[wFIREMAGIC]);
        }
        
        if(s_version<5)
        {
            strcpy(weapon_string[iwQuarterHearts],old_weapon_string[iwQuarterHearts]);
        }
        
        /*
            if (s_version<6)
            {
              strcpy(weapon_string[iwSideRaft],old_weapon_string[iwSideRaft]);
              strcpy(weapon_string[iwSideLadder],old_weapon_string[iwSideLadder]);
            }
        */
    }
    else if (!should_skip)
    {
		for(int32_t i=0; i<MAXWPNS; i++)
			reset_weaponname(i);
    }
    
	for(int32_t i=0; i<weapons_to_read; i++)
	{
		word oldtile = 0;
		if (s_version < 8)
		{
			if (!p_igetw(&oldtile, f))
				return qe_invalid;
		}

		if(!p_getc(&tempweapon.misc,f))
		{
			return qe_invalid;
		}
        
		if(!p_getc(&tempweapon.csets,f))
		{
			return qe_invalid;
		}
        
		if(!p_getc(&tempweapon.frames,f))
		{
			return qe_invalid;
		}
        
		if(!p_getc(&tempweapon.speed,f))
		{
			return qe_invalid;
		}
        
		if(!p_getc(&tempweapon.type,f))
		{
			return qe_invalid;
		}
	
		if ( s_version >= 7 )
		{
			if(!p_igetw(&tempweapon.script,f))
			{
				return qe_invalid;
			}
			if(!p_igetl(&tempweapon.tile,f))
			{
				return qe_invalid;
			}	    
		}
		if ( s_version < 7 ) 
		{
			tempweapon.tile = oldtile;
		}
        
		if(Header->zelda_version < 0x193)
		{
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
		}
        
		if(s_version < 6)
		{
			if(i==ewFIRETRAIL)
			{
				tempweapon.misc |= WF_BEHIND;
			}
			else
				tempweapon.misc &= ~WF_BEHIND;
		}
        
		if (!should_skip)
			memcpy(&wpnsbuf[i], &tempweapon, sizeof(tempweapon));
	}

	if (should_skip)
		return 0;
    
	if(s_version<2)
	{
		wpnsbuf[wSBOOM]=wpnsbuf[wBOOM];
	}
	
	if(s_version<5)
	{
		wpnsbuf[iwQuarterHearts].tile=1;
		wpnsbuf[iwQuarterHearts].csets=1;
	}
	
	if(Header->zelda_version < 0x176)
	{
		wpnsbuf[iwSpawn] = *((wpndata*)(itemsbuf + iMisc1));
		wpnsbuf[iwDeath] = *((wpndata*)(itemsbuf + iMisc2));
		memset(&itemsbuf[iMisc1],0,sizeof(itemdata));
		memset(&itemsbuf[iMisc2],0,sizeof(itemdata));
	}
	
	if((Header->zelda_version < 0x192)||
			((Header->zelda_version == 0x192)&&(Header->build<129)))
	{
		wpnsbuf[wHSCHAIN_V] = wpnsbuf[wHSCHAIN_H];
	}
	
	if((Header->zelda_version < 0x210))
	{
		wpnsbuf[wLSHEAD] = wpnsbuf[wHSHEAD];
		wpnsbuf[wLSCHAIN_H] = wpnsbuf[wHSCHAIN_H];
		wpnsbuf[wLSHANDLE] = wpnsbuf[wHSHANDLE];
		wpnsbuf[wLSCHAIN_V] = wpnsbuf[wHSCHAIN_V];
	}
    
    return 0;
}

static void guy_update_firesfx(guydata& tempguy)
{
	tempguy.firesfx = 0;
	if (tempguy.family == eeWIZZ)
	{
		switch (tempguy.attributes[1])
		{
		case 0: // normal weapon
			tempguy.firesfx = WAV_WAND;
			break;
		case 1: // 8 shots
			if (get_qr(qr_8WAY_SHOT_SFX_DEP)) tempguy.firesfx = WAV_FIRE;
			else
			{
				switch (tempguy.weapon)
				{
				case ewFireTrail:
				case ewFlame:
				case ewFlame2Trail:
				case ewFlame2:
					tempguy.firesfx = WAV_FIRE;
					break;
				case ewWind:
				case ewMagic:
					tempguy.firesfx = WAV_WAND;
					break;
				case ewIce:
					tempguy.firesfx = WAV_ZN1ICE;
					break;
				case ewRock:
					tempguy.firesfx = WAV_ZN1ROCK;
					break;
				case ewFireball2:
				case ewFireball:
					tempguy.firesfx = WAV_ZN1FIREBALL;
					break;
				case ewBrang:
					tempguy.firesfx = WAV_BRANG;
					break;
				case ewBomb:case ewSBomb: case ewLitBomb:case ewLitSBomb:
					tempguy.firesfx = WAV_BOMB;
					break;
				default:
					tempguy.firesfx = 0;
					break;
				}
				break;
			}
		case 2: // Summon
			tempguy.firesfx = WAV_FIRE;
			break;
		case 3: // Summon Layer
			tempguy.firesfx = get_qr(qr_MORESOUNDS) ? WAV_ZN1SUMMON : WAV_FIRE;
			break;
		}
	}
	else
	{
		if ((tempguy.family == eeWALK || tempguy.family == eePROJECTILE) && (tempguy.attributes[0] == e1tSUMMON || tempguy.attributes[0] == e1tSUMMONLAYER))
		{
			tempguy.firesfx = get_qr(qr_MORESOUNDS) ? WAV_ZN1SUMMON : WAV_FIRE;
		}
		else
		{
			switch (tempguy.weapon)
			{
			case ewFireTrail:
			case ewFlame:
			case ewFlame2Trail:
			case ewFlame2:
				tempguy.firesfx = WAV_FIRE;
				break;
			case ewWind:
			case ewMagic:
				tempguy.firesfx = WAV_WAND;
				break;
			case ewIce:
				tempguy.firesfx = WAV_ZN1ICE;
				break;
			case ewRock:
				tempguy.firesfx = WAV_ZN1ROCK;
				break;
			case ewFireball2:
			case ewFireball:
				tempguy.firesfx = WAV_ZN1FIREBALL;
				break;
			case ewBrang:
				tempguy.firesfx = WAV_BRANG;
				break;
			case ewBomb:case ewSBomb: case ewLitBomb:case ewLitSBomb:
				tempguy.firesfx = WAV_BOMB;
				break;
			default:
				tempguy.firesfx = 0;
				break;
			}
		}
	}
}

static void guy_update_weaponflags(guydata& tempguy)
{
	tempguy.wunblockable = 0;
	tempguy.wmoveflags = move_none;
	if (tempguy.weapon == ewFlame || tempguy.weapon == ewFireTrail)
		tempguy.wmoveflags |= (move_obeys_grav | move_can_pitfall);
	tempguy.weapoverrideFLAGS = 0;
	tempguy.weap_hxofs = 0;
	tempguy.weap_hyofs = 0;
	tempguy.weap_hxsz = 0;
	tempguy.weap_hysz = 0;
	tempguy.weap_hzsz = 0;
	tempguy.weap_xofs = 0;
	tempguy.weap_yofs = 0;
	tempguy.weap_tilew = 0;
	tempguy.weap_tileh = 0;
	switch (tempguy.weapon)
	{
	case ewFireball: case ewFireball2:
		tempguy.wstep = 175;
		break;
	case ewSword: case ewLitBomb: case ewLitSBomb:
	case ewRock:  case ewMagic: case ewWind:
		tempguy.wstep = 300;
		break;
	case ewArrow:
		tempguy.wstep = 200;
		break;
	case ewFlame: case ewFlame2:
		tempguy.wstep = 100;
		break;
	default:
		tempguy.wstep = 0;
		break;
	}
	for (int q = 0; q < WPNSPR_MAX; ++q)
	{
		tempguy.burnsprs[q] = 0;
		tempguy.light_rads[q] = 0;
	}
}

static void guy_update_weaponspecialsfx(guydata& tempguy)
{
	switch (tempguy.weapon)
	{
	case ewBrang:
		tempguy.specialsfx = WAV_BRANG;
		break;
	case ewBomb: case ewSBomb: case ewLitBomb:case ewLitSBomb:
		tempguy.specialsfx = WAV_BOMB;
		break;
	default:
		tempguy.specialsfx = 0;
		break;
	}
}

void init_guys(int32_t guyversion)
{
    for(int32_t i=0; i<MAXGUYS; i++)
    {
        guysbuf[i] = default_guys[0];
    }
    
    for(int32_t i=0; i<OLDMAXGUYS; i++)
    {
        guysbuf[i] = default_guys[i];
        guysbuf[i].spr_shadow = (guysbuf[i].family==eeROCK && guysbuf[i].attributes[9] == 1) ? iwLargeShadow : iwShadow;
		guysbuf[i].spr_death = iwDeath;
		guysbuf[i].spr_spawn = iwSpawn;
        // Patra fix: 2.10 BSPatras used spDIG. 2.50 Patras use CSet 7.
        if(guyversion<=3 && i==ePATRABS)
        {
            guysbuf[i].bosspal=spDIG;
            guysbuf[i].cset=14;
            guysbuf[i].attributes[8] = 14;
        }
        
        if(guyversion<=3)
        {
            // Rope/Ghini Flash rules
            if(get_bit(deprecated_rules, qr_NOROPE2FLASH_DEP))
            {
                if(i==eROPE2)
                {
                    guysbuf[i].flags &= ~guy_flashing;
                }
            }
            
            if(get_bit(deprecated_rules, qr_NOBUBBLEFLASH_DEP))
            {
                if(i==eBUBBLEST || i==eBUBBLESP || i==eBUBBLESR || i==eBUBBLEIT || i==eBUBBLEIP || i==eBUBBLEIR)
                {
                    guysbuf[i].flags &= ~guy_flashing;
                }
            }
            
            if(i==eGHINI2)
            {
                if(get_bit(deprecated_rules, qr_GHINI2BLINK_DEP))
                {
                    guysbuf[i].flags |= guy_blinking;
                }
                
                if(get_bit(deprecated_rules, qr_PHANTOMGHINI2_DEP))
                {
                    guysbuf[i].flags |= guy_transparent;
                }
            }

			if (i == eDIG1 || i == eDIG3)
			{
				guysbuf[i].flags |= guy_ignore_kill_all;
			}
        }
        
        // Darknut fix
        if(i==eDKNUT1 || i==eDKNUT2 || i==eDKNUT3 || i==eDKNUT4 || i==eDKNUT5)
        {
            if(get_qr(qr_NEWENEMYTILES))
            {
                guysbuf[i].s_tile=guysbuf[i].e_tile+120;
                guysbuf[i].s_width=guysbuf[i].e_width;
                guysbuf[i].s_height=guysbuf[i].e_height;
            }
            else guysbuf[i].s_tile=860;
            
            if(!get_bit(deprecated_rules,qr_BRKBLSHLDS_DEP))
				guysbuf[i].flags &= ~guy_bkshield;
			else
				guysbuf[i].flags |= guy_bkshield;
        }
        
        if((i==eGELTRIB || i==eFGELTRIB) && get_bit(deprecated_rules,qr_OLDTRIBBLES_DEP))
        {
            guysbuf[i].attributes[2] = (i == eFGELTRIB ? eFZOL : eZOL);
        }

        guy_update_firesfx(guysbuf[i]);
		guy_update_weaponflags(guysbuf[i]);
		guy_update_weaponspecialsfx(guysbuf[i]);
    }
}

void reset_weaponname(int32_t i)
{
    if(i<wLast)
    {
        strcpy(weapon_string[i],old_weapon_string[i]);
    }
    else
        sprintf(weapon_string[i],"zz%03d",i);
}

void init_item_drop_sets()
{
    for(int32_t i=0; i<MAXITEMDROPSETS; i++)
    {
//    item_drop_sets[i] = default_item_drop_sets[0];
        memset(&item_drop_sets[i], 0, sizeof(item_drop_object));
    }
    
    for(int32_t i=0; i<isMAX; i++)
    {
        item_drop_sets[i] = default_item_drop_sets[i];
        
        // Deprecated: qr_NOCLOCKS and qr_ALLOW10RUPEEDROPS
        for(int32_t j=0; j<10; ++j)
        {
            int32_t it = item_drop_sets[i].item[j];
            
            if((itemsbuf[it].family == itype_rupee && ((itemsbuf[it].amount)&0xFFF) == 10)
                    && !get_bit(deprecated_rules, qr_ALLOW10RUPEEDROPS_DEP))
            {
                item_drop_sets[i].chance[j+1]=0;
            }
            else if(itemsbuf[it].family == itype_clock && get_bit(deprecated_rules, qr_NOCLOCKS_DEP))
            {
                item_drop_sets[i].chance[j+1]=0;
            }
            
            // From Sept 2007 to Dec 2008, non-gameplay items were prohibited.
            if(itemsbuf[it].family == itype_misc)
            {
                // If a non-gameplay item was selected, then item drop was aborted.
                // Reflect this by increasing the 'Nothing' chance accordingly.
                item_drop_sets[i].chance[0]+=item_drop_sets[i].chance[j+1];
                item_drop_sets[i].chance[j+1]=0;
            }
        }
    }
}

void init_favorites()
{
    for(int32_t i=0; i<MAXFAVORITECOMBOS; i++)
    {
        favorite_combos[i]=-1;
    }
}

const char *ctype_name[cMAX]=
{
    "cNONE", "cSTAIR", "cCAVE", "cWATER", "cARMOS", "cGRAVE", "cDOCK",
    "cUNDEF", "cPUSH_WAIT", "cPUSH_HEAVY", "cPUSH_HW", "cL_STATUE", "cR_STATUE",
    "cWALKSLOW", "cCVUP", "cCVDOWN", "cCVLEFT", "cCVRIGHT", "cSWIMWARP", "cDIVEWARP",
    "cLADDERHOOKSHOT", "cTRIGNOFLAG", "cTRIGFLAG", "cZELDA", "cSLASH", "cSLASHITEM",
    "cPUSH_HEAVY2", "cPUSH_HW2", "cPOUND", "cHSGRAB", "cHSBRIDGE", "cDAMAGE1",
    "cDAMAGE2", "cDAMAGE3", "cDAMAGE4", "cC_STATUE", "cTRAP_H", "cTRAP_V", "cTRAP_4",
    "cTRAP_LR", "cTRAP_UD", "cPIT", "cHOOKSHOTONLY", "cOVERHEAD", "cNOFLYZONE", "cMIRROR",
    "cMIRRORSLASH", "cMIRRORBACKSLASH", "cMAGICPRISM", "cMAGICPRISM4",
    "cMAGICSPONGE", "cCAVE2", "cEYEBALL_A", "cEYEBALL_B", "cNOJUMPZONE", "cBUSH",
    "cFLOWERS", "cTALLGRASS", "cSHALLOWWATER", "cLOCKBLOCK", "cLOCKBLOCK2",
    "cBOSSLOCKBLOCK", "cBOSSLOCKBLOCK2", "cLADDERONLY", "cBSGRAVE",
    "cCHEST", "cCHEST2", "cLOCKEDCHEST", "cLOCKEDCHEST2", "cBOSSCHEST", "cBOSSCHEST2",
    "cRESET", "cSAVE", "cSAVE2", "cCAVEB", "cCAVEC", "cCAVED",
    "cSTAIRB", "cSTAIRC", "cSTAIRD", "cPITB", "cPITC", "cPITD",
    "cCAVE2B", "cCAVE2C", "cCAVE2D", "cSWIMWARPB", "cSWIMWARPC", "cSWIMWARPD",
    "cDIVEWARPB", "cDIVEWARPC", "cDIVEWARPD", "cSTAIRR", "cPITR",
    "cAWARPA", "cAWARPB", "cAWARPC", "cAWARPD", "cAWARPR",
    "cSWARPA", "cSWARPB", "cSWARPC", "cSWARPD", "cSWARPR", "cSTRIGNOFLAG", "cSTRIGFLAG",
    "cSTEP", "cSTEPSAME", "cSTEPALL", "cSTEPCOPY", "cNOENEMY", "cBLOCKARROW1", "cBLOCKARROW2",
    "cBLOCKARROW3", "cBLOCKBRANG1", "cBLOCKBRANG2", "cBLOCKBRANG3", "cBLOCKSBEAM", "cBLOCKALL",
    "cBLOCKFIREBALL", "cDAMAGE5", "cDAMAGE6", "cDAMAGE7", "cCHANGE", "cSPINTILE1", "cSPINTILE2",
    "cSCREENFREEZE", "cSCREENFREEZEFF", "cNOGROUNDENEMY", "cSLASHNEXT", "cSLASHNEXTITEM", "cBUSHNEXT"
    "cSLASHTOUCHY", "cSLASHITEMTOUCHY", "cBUSHTOUCHY", "cFLOWERSTOUCHY", "cTALLGRASSTOUCHY",
    "cSLASHNEXTTOUCHY", "cSLASHNEXTITEMTOUCHY", "cBUSHNEXTTOUCHY", "cEYEBALL_4", "cTALLGRASSNEXT",
    "cSCRIPT1", "cSCRIPT2", "cSCRIPT3", "cSCRIPT4", "cSCRIPT5",
    "cSCRIPT6", "cSCRIPT7", "cSCRIPT8", "cSCRIPT9", "cSCRIPT10",
    "cSCRIPT11", "cSCRIPT12", "cSCRIPT13", "cSCRIPT14", "cSCRIPT15",
    "cSCRIPT16", "cSCRIPT17", "cSCRIPT18", "cSCRIPT19", "cSCRIPT20"
    
};

int32_t init_combo_classes()
{
	zinfo* zi = (load_tmp_zi ? load_tmp_zi : &ZI);
    for(int32_t i=0; i<cMAX; i++)
    {
        combo_class_buf[i] = default_combo_classes[i];
		if ( char const* nm = zi->getComboTypeName(i) )
		{
			size_t len = strlen(nm);
			for ( size_t q = 0; q < 64; q++ )
			{
				combo_class_buf[i].name[q] = (q<len ? nm[q] : 0);
			}
		}
    }
    
    return 0;
}

int32_t readherosprites2(PACKFILE *f, int32_t v_herosprites, int32_t cv_herosprites)
{
    bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_herosprites);
    if (should_skip) return 0;

	assert(v_herosprites < 6);
	//these are here to bypass compiler warnings about unused arguments
	cv_herosprites=cv_herosprites;
	
	zinit.hero_swim_speed=67; //default
	setupherotiles(zinit.heroAnimationStyle);
	setupherodefenses();
	setupherooffsets();
    
    if(v_herosprites>=0)
    {
        word tile, tile2;
        byte flip, extend, dummy_byte;
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			walkspr[i][spr_tile]=(int32_t)tile;
			walkspr[i][spr_flip]=(int32_t)flip;
			walkspr[i][spr_extend]=(int32_t)extend;
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			stabspr[i][spr_tile]=(int32_t)tile;
			stabspr[i][spr_flip]=(int32_t)flip;
			stabspr[i][spr_extend]=(int32_t)extend;
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			slashspr[i][spr_tile]=(int32_t)tile;
			slashspr[i][spr_flip]=(int32_t)flip;
			slashspr[i][spr_extend]=(int32_t)extend;
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			floatspr[i][spr_tile]=(int32_t)tile;
			floatspr[i][spr_flip]=(int32_t)flip;
			floatspr[i][spr_extend]=(int32_t)extend;
        }
        
        if(v_herosprites>1)
        {
            for(int32_t i=0; i<4; i++)
            {
                if(!p_igetw(&tile,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&flip,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&extend,f))
                {
                    return qe_invalid;
                }
                
				swimspr[i][spr_tile]=(int32_t)tile;
				swimspr[i][spr_flip]=(int32_t)flip;
				swimspr[i][spr_extend]=(int32_t)extend;
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			divespr[i][spr_tile]=(int32_t)tile;
			divespr[i][spr_flip]=(int32_t)flip;
			divespr[i][spr_extend]=(int32_t)extend;
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_igetw(&tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&extend,f))
            {
                return qe_invalid;
            }
            
			poundspr[i][spr_tile]=(int32_t)tile;
			poundspr[i][spr_flip]=(int32_t)flip;
			poundspr[i][spr_extend]=(int32_t)extend;
        }
        
        if(!p_igetw(&tile,f))
        {
            return qe_invalid;
        }
        
        flip=0;
        
        if(v_herosprites>0)
        {
            if(!p_getc(&flip,f))
            {
                return qe_invalid;
            }
        }
        
        if(!p_getc(&extend,f))
        {
            return qe_invalid;
        }
        
		castingspr[spr_tile]=(int32_t)tile;
		castingspr[spr_flip]=(int32_t)flip;
		castingspr[spr_extend]=(int32_t)extend;
        
        if(v_herosprites>0)
        {
			int32_t num_holdsprs = (v_herosprites > 6 ? 3 : 2);
            for(int32_t i=0; i<2; i++)
            {
                for(int32_t j=0; j<num_holdsprs; j++)
                {
                    if(!p_igetw(&tile,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(!p_getc(&flip,f))
                    {
                        return qe_invalid;
                    }
                    
                    if(!p_getc(&extend,f))
                    {
                        return qe_invalid;
                    }
                    
					holdspr[i][j][spr_tile]=(int32_t)tile;
					holdspr[i][j][spr_flip]=(int32_t)flip;
					holdspr[i][j][spr_extend]=(int32_t)extend;
                }
            }
        }
        else
        {
            for(int32_t i=0; i<2; i++)
            {
                if(!p_igetw(&tile,f))
                {
                    return qe_invalid;
                }
                
                if(!p_igetw(&tile2,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&extend,f))
                {
                    return qe_invalid;
                }
                
				holdspr[i][spr_hold1][spr_tile]=(int32_t)tile;
				holdspr[i][spr_hold1][spr_flip]=(int32_t)flip;
				holdspr[i][spr_hold1][spr_extend]=(int32_t)extend;
				holdspr[i][spr_hold2][spr_tile]=(int32_t)tile2;
				holdspr[i][spr_hold2][spr_flip]=(int32_t)flip;
				holdspr[i][spr_hold2][spr_extend]=(int32_t)extend;
            }
        }
        
        if(v_herosprites>2)
        {
            for(int32_t i=0; i<4; i++)
            {
                if(!p_igetw(&tile,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&flip,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&extend,f))
                {
                    return qe_invalid;
                }
                
				jumpspr[i][spr_tile]=(int32_t)tile;
				jumpspr[i][spr_flip]=(int32_t)flip;
				jumpspr[i][spr_extend]=(int32_t)extend;
            }
        }
        
        if(v_herosprites>3)
        {
            for(int32_t i=0; i<4; i++)
            {
                if(!p_igetw(&tile,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&flip,f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&extend,f))
                {
                    return qe_invalid;
                }
                
				chargespr[i][spr_tile]=(int32_t)tile;
				chargespr[i][spr_flip]=(int32_t)flip;
				chargespr[i][spr_extend]=(int32_t)extend;
            }
        }
        
        if(v_herosprites>4)
        {
            if(!p_getc(&dummy_byte,f))
            {
                return qe_invalid;
            }
            
			zinit.hero_swim_speed=(byte)dummy_byte;
        }
		
		memset(frozenspr, 0, sizeof(frozenspr));
		memset(frozen_waterspr, 0, sizeof(frozen_waterspr));
		memset(onfirespr, 0, sizeof(onfirespr));
		memset(onfire_waterspr, 0, sizeof(onfire_waterspr));
		memset(diggingspr, 0, sizeof(diggingspr));
		memset(usingrodspr, 0, sizeof(usingrodspr));
		memset(usingcanespr, 0, sizeof(usingcanespr));
		memset(pushingspr, 0, sizeof(pushingspr));
		memset(liftingspr, 0, sizeof(liftingspr));
		memset(liftingwalkspr, 0, sizeof(liftingwalkspr));
		memset(stunnedspr, 0, sizeof(stunnedspr));
		memset(stunned_waterspr, 0, sizeof(stunned_waterspr));
		memset(fallingspr, 0, sizeof(fallingspr));
		memset(shockedspr, 0, sizeof(shockedspr));
		memset(shocked_waterspr, 0, sizeof(shocked_waterspr));
		memset(pullswordspr, 0, sizeof(pullswordspr));
		memset(readingspr, 0, sizeof(readingspr));
		memset(slash180spr, 0, sizeof(slash180spr));
		memset(slashZ4spr, 0, sizeof(slashZ4spr));
		memset(dashspr, 0, sizeof(dashspr));
		memset(bonkspr, 0, sizeof(bonkspr));
		memset(medallionsprs, 0, sizeof(medallionsprs));
		memset(holdspr[0][2], 0, sizeof(holdspr[0][2])); //Sword hold (Land)
		memset(holdspr[1][2], 0, sizeof(holdspr[1][2])); //Sword hold (Water)
		for(int32_t q = 0; q < 4; ++q)
		{
			for(int32_t p = 0; p < 3; ++p)
			{
				drowningspr[q][p] = divespr[q][p];
				drowning_lavaspr[q][p] = divespr[q][p];
			}
		}
		memset(sideswimspr, 0, sizeof(sideswimspr));
		memset(sideswimslashspr, 0, sizeof(sideswimslashspr));
		memset(sideswimstabspr, 0, sizeof(sideswimstabspr));
		memset(sideswimpoundspr, 0, sizeof(sideswimpoundspr));
		memset(sideswimchargespr, 0, sizeof(sideswimchargespr));
		memset(sideswimholdspr, 0, sizeof(sideswimholdspr));
		memset(sidedrowningspr, 0, sizeof(sidedrowningspr));
    }
    
	if(FFCore.quest_format[vInitData] < 34)
	{
		bool fastswim = zinit.hero_swim_speed > 60;
		// '2/3' or '1/2'
		zinit.hero_swim_mult = fastswim ? 2 : 1;
		zinit.hero_swim_div = fastswim ? 3 : 2;
	}
    return 0;
}

void setSprite(int32_t* arr, int32_t tile, int32_t flip, int32_t ext)
{
    arr[spr_tile] = tile;
    arr[spr_flip] = (flip > 3 ? 0 : flip);
    arr[spr_extend] = (ext > 2 ? 0 : ext);
}
//Used to read the player sprites as int32_t, not word. 
int32_t readherosprites3(PACKFILE *f, int32_t v_herosprites, int32_t cv_herosprites)
{
	//these are here to bypass compiler warnings about unused arguments
	cv_herosprites=cv_herosprites;
	
	zinit.hero_swim_speed=67; //default
	setupherotiles(zinit.heroAnimationStyle);
	setupherodefenses();
	setupherooffsets();
	
	int32_t tile, tile2;
	byte flip, extend, dummy_byte;
	
	if(v_herosprites>=0)
	{
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(walkspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(stabspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(slashspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(floatspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		if(v_herosprites>1)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_igetl(&tile,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&flip,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&extend,f))
				{
					return qe_invalid;
				}
				
				setSprite(swimspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(divespr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetl(&tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&extend,f))
			{
				return qe_invalid;
			}
			
			setSprite(poundspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
		}
		
		if(!p_igetl(&tile,f))
		{
			return qe_invalid;
		}
		
		flip=0;
		
		if(v_herosprites>0)
		{
			if(!p_getc(&flip,f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_getc(&extend,f))
		{
			return qe_invalid;
		}
		
		setSprite(castingspr, int32_t(tile), int32_t(flip), int32_t(extend));
		
		if(v_herosprites>0)
		{
			int32_t num_holdsprs = (v_herosprites > 6 ? 3 : 2);
			for(int32_t i=0; i<2; i++)
			{
				for(int32_t j=0; j<num_holdsprs; j++)
				{
					if(!p_igetl(&tile,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&flip,f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&extend,f))
					{
						return qe_invalid;
					}
					
					setSprite(holdspr[i][j], int32_t(tile), int32_t(flip), int32_t(extend));
				}
			}
		}
		else
		{
			for(int32_t i=0; i<2; i++)
			{
				if(!p_igetl(&tile,f))
				{
					return qe_invalid;
				}
				
				if(!p_igetl(&tile2,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&extend,f))
				{
					return qe_invalid;
				}
				
				setSprite(holdspr[i][spr_hold1], int32_t(tile), int32_t(flip), int32_t(extend));
				setSprite(holdspr[i][spr_hold2], int32_t(tile2), int32_t(flip), int32_t(extend));
			}
		}
		
		if(v_herosprites>2)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_igetl(&tile,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&flip,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&extend,f))
				{
					return qe_invalid;
				}
				
				setSprite(jumpspr[i], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		
		if(v_herosprites>3)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_igetl(&tile,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&flip,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&extend,f))
				{
					return qe_invalid;
				}
				
				setSprite(chargespr[i], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		
		if(v_herosprites>4)
		{
			if(!p_getc(&dummy_byte,f))
			{
				return qe_invalid;
			}
			
			zinit.hero_swim_speed=(byte)dummy_byte;
		}
		
		if(v_herosprites>6)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(frozenspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(frozen_waterspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(onfirespr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(onfire_waterspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(diggingspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(usingrodspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(usingcanespr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(pushingspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				byte frames = 0;
				if(v_herosprites > 15)
				{
					if(!p_getc(&frames,f))
						return qe_invalid;
				}
				
				setSprite(liftingspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
				liftingspr[q][spr_frames] = frames;
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(liftingwalkspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(stunnedspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(stunned_waterspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(drowningspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(drowning_lavaspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(fallingspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(shockedspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(shocked_waterspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(pullswordspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(readingspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(slash180spr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(slashZ4spr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(dashspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(bonkspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			
			for(int32_t q = 0; q < 3; ++q) //Not directions; number of medallion sprs
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(medallionsprs[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(frozenspr, 0, sizeof(frozenspr));
			memset(frozen_waterspr, 0, sizeof(frozen_waterspr));
			memset(onfirespr, 0, sizeof(onfirespr));
			memset(onfire_waterspr, 0, sizeof(onfire_waterspr));
			memset(diggingspr, 0, sizeof(diggingspr));
			memset(usingrodspr, 0, sizeof(usingrodspr));
			memset(usingcanespr, 0, sizeof(usingcanespr));
			memset(pushingspr, 0, sizeof(pushingspr));
			memset(liftingspr, 0, sizeof(liftingspr));
			memset(liftingwalkspr, 0, sizeof(liftingwalkspr));
			memset(stunnedspr, 0, sizeof(stunnedspr));
			memset(stunned_waterspr, 0, sizeof(stunned_waterspr));
			memset(fallingspr, 0, sizeof(fallingspr));
			memset(shockedspr, 0, sizeof(shockedspr));
			memset(shocked_waterspr, 0, sizeof(shocked_waterspr));
			memset(pullswordspr, 0, sizeof(pullswordspr));
			memset(readingspr, 0, sizeof(readingspr));
			memset(slash180spr, 0, sizeof(slash180spr));
			memset(slashZ4spr, 0, sizeof(slashZ4spr));
			memset(dashspr, 0, sizeof(dashspr));
			memset(bonkspr, 0, sizeof(bonkspr));
			memset(medallionsprs, 0, sizeof(medallionsprs));
			memset(holdspr[0][2], 0, sizeof(holdspr[0][2])); //Sword hold (Land)
			memset(holdspr[1][2], 0, sizeof(holdspr[1][2])); //Sword hold (Water)
			for(int32_t q = 0; q < 4; ++q)
			{
				for(int32_t p = 0; p < 3; ++p)
				{
					drowningspr[q][p] = divespr[q][p];
					drowning_lavaspr[q][p] = divespr[q][p];
				}
			}
		}
		if (v_herosprites > 8)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(sideswimspr, 0, sizeof(sideswimspr));
		}
		if (v_herosprites > 9)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimslashspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimstabspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimpoundspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimchargespr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(sideswimslashspr, 0, sizeof(sideswimslashspr));
			memset(sideswimstabspr, 0, sizeof(sideswimstabspr));
			memset(sideswimpoundspr, 0, sizeof(sideswimpoundspr));
			memset(sideswimchargespr, 0, sizeof(sideswimchargespr));
		}
		if (v_herosprites > 10)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				int32_t hmr;
				if(!p_igetl(&hmr,f))
					return qe_invalid;
				
				hammeroffsets[q] = hmr;
			}
		}
		else
		{
			for(int32_t q = 0; q < 4; ++q) hammeroffsets[q] = 0;
		}
		if (v_herosprites > 11)
		{
			for(int32_t q = 0; q < 3; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sideswimholdspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(sideswimholdspr, 0, sizeof(sideswimholdspr));
		}
		if (v_herosprites > 12)
		{
			if(!p_igetl(&tile,f))
				return qe_invalid;
			
			if(!p_getc(&flip,f))
				return qe_invalid;
			
			if(!p_getc(&extend,f))
				return qe_invalid;
			setSprite(sideswimcastingspr, int32_t(tile), int32_t(flip), int32_t(extend));
			
		}
		else
		{
			memset(sideswimcastingspr, 0, sizeof(sideswimcastingspr));
		}
		if (v_herosprites > 13)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(sidedrowningspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(sidedrowningspr, 0, sizeof(sidedrowningspr));
		}
		if (v_herosprites > 14)
		{
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_igetl(&tile,f))
					return qe_invalid;
				
				if(!p_getc(&flip,f))
					return qe_invalid;
				
				if(!p_getc(&extend,f))
					return qe_invalid;
				
				setSprite(revslashspr[q], int32_t(tile), int32_t(flip), int32_t(extend));
			}
		}
		else
		{
			memset(revslashspr, 0, sizeof(revslashspr));
		}
		if (v_herosprites > 7)
		{
			int32_t num_defense = wMax;
			byte def = 0;

			//Set num_defense accordingly if changes to enum require version upgrade - Jman
			/*if(v_herosprites > [x])
			* {
			*	 num_defense = 146 //value of wMax on version 8
			* }
			*/

			for (int32_t q = 0; q < num_defense; q++)
			{
				if (!p_getc(&def, f))
					return qe_invalid;

				hero_defence[q] = def;
			}
		}
		else
		{
			int32_t num_defense = wMax;
			for (int32_t q = 0; q < num_defense; q++)
			{
				hero_defence[q] = 0;
			}
		}
	}
	
	if(FFCore.quest_format[vInitData] < 34)
	{
		bool fastswim = zinit.hero_swim_speed > 60;
		// '2/3' or '1/2'
		zinit.hero_swim_mult = fastswim ? 2 : 1;
		zinit.hero_swim_div = fastswim ? 3 : 2;
	}
	return 0;
}


int32_t readherosprites(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword dummy;
    word s_version=0, s_cversion=0;
    
    //section version info
    if(!p_igetw(&s_version,f))
    {
        return qe_invalid;
    }
    
    FFCore.quest_format[vHeroSprites] = s_version;
    
    if(!p_igetw(&s_cversion,f))
    {
        return qe_invalid;
    }
    
    //section size
    if(!p_igetl(&dummy,f))
    {
        return qe_invalid;
    }
    if ( s_version >= 6 ) 
    {
	    return readherosprites3(f, s_version, dummy);
    }
    else return readherosprites2(f, s_version, dummy);
}

int32_t read_old_subscreens(PACKFILE *f, word s_version)
{
	subscreens_active.clear();
	subscreens_passive.clear();
	subscreens_overlay.clear();
	for(int32_t i=0; i<MAXCUSTOMSUBSCREENS; i++)
	{
		subscreen_group g;
		memset(&g,0,sizeof(subscreen_group));
		int32_t ret = read_one_old_subscreen(f, &g, s_version);
		if(ret!=0)
			return ret;
		if(g.objects[0].type == ssoNULL) continue;
		auto& vec = g.ss_type == sstPASSIVE ? subscreens_passive : subscreens_active;
		ZCSubscreen& sub = vec.emplace_back();
		sub.load_old(g);
	}
	
	return 0;
}

int32_t read_one_old_subscreen(PACKFILE *f, subscreen_group* g, word s_version)
{
	int32_t numsub=0;
	byte temp_ss=0;
	subscreen_object temp_sub_stack;
	subscreen_object *temp_sub = &temp_sub_stack;
	
	char tempname[64];

	// FWIW I never saw anything bigger than 20.
	#define MAX_DP1_LEN 1024
	char tempdp1[MAX_DP1_LEN];
	
	if(!pfread(tempname,64,f))
	{
		return qe_invalid;
	}
	
	if(s_version > 1)
	{
		if(!p_getc(&temp_ss,f))
		{
			return qe_invalid;
		}
	}
	
	if(s_version < 4)
	{
		uint8_t tmp=0;
		
		if(!p_getc(&tmp,f))
		{
			return qe_invalid;
		}
		
		numsub = (int32_t)tmp;
	}
	else
	{
		word tmp;
		
		if(!p_igetw(&tmp, f))
		{
			return qe_invalid;
		}
		
		numsub = (int32_t)tmp;
	}
	
	int32_t j;
	
	for(j=0; (j<MAXSUBSCREENITEMS&&j<numsub); j++)
	{
		memset(temp_sub,0,sizeof(subscreen_object));
		
		switch(g->objects[j].type)
		{
			case ssoTEXT:
			case ssoTEXTBOX:
			case ssoCURRENTITEMTEXT:
			case ssoCURRENTITEMCLASSTEXT:
				if(g->objects[j].dp1 != NULL) delete [](char *)g->objects[j].dp1;
				
				//fall through
			default:
				memset(&g->objects[j],0,sizeof(subscreen_object));
				break;
		}
		
		if(!p_getc(&(temp_sub->type),f))
			return qe_invalid;
		
		if(!p_getc(&(temp_sub->pos),f))
			return qe_invalid;
		
		if(s_version < 5)
		{
			switch(temp_sub->pos)
			{
				case 0:
					temp_sub->pos = sspUP | sspDOWN | sspSCROLLING;
					break;
					
				case 1:
					temp_sub->pos = sspUP;
					break;
					
				case 2:
					temp_sub->pos = sspDOWN;
					break;
					
				default:
					temp_sub->pos = 0;
			}
		}
		
		if(!p_igetw(&(temp_sub->x),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->y),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->w),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->h),f))
			return qe_invalid;
		
		if(!p_getc(&(temp_sub->colortype1),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->color1),f))
			return qe_invalid;
		
		if(!p_getc(&(temp_sub->colortype2),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->color2),f))
			return qe_invalid;
		
		if(!p_getc(&(temp_sub->colortype3),f))
			return qe_invalid;
		
		if(!p_igetw(&(temp_sub->color3),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d1),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d2),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d3),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d4),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d5),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d6),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d7),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d8),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d9),f))
			return qe_invalid;
		
		if(!p_igetd(&(temp_sub->d10),f))
			return qe_invalid;
		
		if(s_version < 2)
		{
			if(!p_igetl(&(temp_sub->speed),f))
				return qe_invalid;
			
			if(!p_igetl(&(temp_sub->delay),f))
				return qe_invalid;
			
			if(!p_igetl(&(temp_sub->frame),f))
				return qe_invalid;
		}
		else
		{
			if(!p_getc(&(temp_sub->speed),f))
				return qe_invalid;
			
			if(!p_getc(&(temp_sub->delay),f))
				return qe_invalid;
			
			if(!p_igetw(&(temp_sub->frame),f))
				return qe_invalid;
		}
		
		int32_t temp_size=0;
		
		// bool deletets = false;
		switch(temp_sub->type)
		{
		case ssoTEXT:
		case ssoTEXTBOX:
		case ssoCURRENTITEMTEXT:
		case ssoCURRENTITEMCLASSTEXT:
		{
			word temptempsize;
			
			if(!p_igetw(&temptempsize,f))
			{
				return qe_invalid;
			}
			
			//temptempsize = temp1 + (temp2 << 8);
			temp_size = (int32_t)temptempsize;
			uint32_t char_length = temp_size+2;
			if (char_length > MAX_DP1_LEN)
			{
				return qe_invalid;
			}
			tempdp1[char_length - 1] = '\0';
			
			if(temp_size)
				if(!pfread(tempdp1,temp_size+1,f))
					return qe_invalid;
			break;
		}
			
		case ssoLIFEMETER:
			if(get_bit(deprecated_rules, 12) != 0) // qr_24HC
				temp_sub->d3 = 1;
			
			if(!p_getc(&(temp_sub->dp1),f))
				return qe_invalid;
			
			break;
			
			
		case ssoCURRENTITEM:
		
			if(s_version < 6)
			{
				switch(temp_sub->d1)
				{
				case ssiBOMB:
					temp_sub->d1 = itype_bomb;
					break;
					
				case ssiSWORD:
					temp_sub->d1 = itype_sword;
					break;
					
				case ssiSHIELD:
					temp_sub->d1 = itype_shield;
					break;
					
				case ssiCANDLE:
					temp_sub->d1 = itype_candle;
					break;
					
				case ssiLETTER:
					temp_sub->d1 = itype_letter;
					break;
					
				case ssiPOTION:
					temp_sub->d1 = itype_potion;
					break;
					
				case ssiLETTERPOTION:
					temp_sub->d1 = itype_letterpotion;
					break;
					
				case ssiBOW:
					temp_sub->d1 = itype_bow;
					break;
					
				case ssiARROW:
					temp_sub->d1 = itype_arrow;
					break;
					
				case ssiBOWANDARROW:
					temp_sub->d1 = itype_bowandarrow;
					break;
					
				case ssiBAIT:
					temp_sub->d1 = itype_bait;
					break;
					
				case ssiRING:
					temp_sub->d1 = itype_ring;
					break;
					
				case ssiBRACELET:
					temp_sub->d1 = itype_bracelet;
					break;
					
				case ssiMAP:
					temp_sub->d1 = itype_map;
					break;
					
				case ssiCOMPASS:
					temp_sub->d1 = itype_compass;
					break;
					
				case ssiBOSSKEY:
					temp_sub->d1 = itype_bosskey;
					break;
					
				case ssiMAGICKEY:
					temp_sub->d1 = itype_magickey;
					break;
					
				case ssiBRANG:
					temp_sub->d1 = itype_brang;
					break;
					
				case ssiWAND:
					temp_sub->d1 = itype_wand;
					break;
					
				case ssiRAFT:
					temp_sub->d1 = itype_raft;
					break;
					
				case ssiLADDER:
					temp_sub->d1 = itype_ladder;
					break;
					
				case ssiWHISTLE:
					temp_sub->d1 = itype_whistle;
					break;
					
				case ssiBOOK:
					temp_sub->d1 = itype_book;
					break;
					
				case ssiWALLET:
					temp_sub->d1 = itype_wallet;
					break;
					
				case ssiSBOMB:
					temp_sub->d1 = itype_sbomb;
					break;
					
				case ssiHCPIECE:
					temp_sub->d1 = itype_heartpiece;
					break;
					
				case ssiAMULET:
					temp_sub->d1 = itype_amulet;
					break;
					
				case ssiFLIPPERS:
					temp_sub->d1 = itype_flippers;
					break;
					
				case ssiHOOKSHOT:
					temp_sub->d1 = itype_hookshot;
					break;
					
				case ssiLENS:
					temp_sub->d1 = itype_lens;
					break;
					
				case ssiHAMMER:
					temp_sub->d1 = itype_hammer;
					break;
					
				case ssiBOOTS:
					temp_sub->d1 = itype_boots;
					break;
					
				case ssiDIVINEFIRE:
					temp_sub->d1 = itype_divinefire;
					break;
					
				case ssiDIVINEESCAPE:
					temp_sub->d1 = itype_divineescape;
					break;
					
				case ssiDIVINEPROTECTION:
					temp_sub->d1 = itype_divineprotection;
					break;
					
				case ssiQUIVER:
					temp_sub->d1 = itype_quiver;
					break;
					
				case ssiBOMBBAG:
					temp_sub->d1 = itype_bombbag;
					break;
					
				case ssiCBYRNA:
					temp_sub->d1 = itype_cbyrna;
					break;
					
				case ssiROCS:
					temp_sub->d1 = itype_rocs;
					break;
					
				case ssiHOVERBOOTS:
					temp_sub->d1 = itype_hoverboots;
					break;
					
				case ssiSPINSCROLL:
					temp_sub->d1 = itype_spinscroll;
					break;
					
				case ssiCROSSSCROLL:
					temp_sub->d1 = itype_crossscroll;
					break;
					
				case ssiQUAKESCROLL:
					temp_sub->d1 = itype_quakescroll;
					break;
					
				case ssiWHISPRING:
					temp_sub->d1 = itype_whispring;
					break;
					
				case ssiCHARGERING:
					temp_sub->d1 = itype_chargering;
					break;
					
				case ssiPERILSCROLL:
					temp_sub->d1 = itype_perilscroll;
					break;
					
				case ssiWEALTHMEDAL:
					temp_sub->d1 = itype_wealthmedal;
					break;
					
				case ssiHEARTRING:
					temp_sub->d1 = itype_heartring;
					break;
					
				case ssiMAGICRING:
					temp_sub->d1 = itype_magicring;
					break;
					
				case ssiSPINSCROLL2:
					temp_sub->d1 = itype_spinscroll2;
					break;
					
				case ssiQUAKESCROLL2:
					temp_sub->d1 = itype_quakescroll2;
					break;
					
				case ssiAGONY:
					temp_sub->d1 = itype_agony;
					break;
					
				case ssiSTOMPBOOTS:
					temp_sub->d1 = itype_stompboots;
					break;
					
				case ssiWHIMSICALRING:
					temp_sub->d1 = itype_whimsicalring;
					break;
					
				case ssiPERILRING:
					temp_sub->d1 = itype_perilring;
					break;
					
				default:
					temp_sub->d1 += itype_custom1 - ssiMAX;
				}
			}
			
			//fall-through
		default:
			if(!p_getc(&(temp_sub->dp1),f))
				return qe_invalid;
			
			break;
		}
		
		if(s_version < 7)
		{
			switch(temp_sub->type)
			{
				case ssoMAGICGAUGE:
				{
					if(!temp_sub->d9)
						temp_sub->d9 = -1; //-1 now represents 'always'
					break;
				}
				case ssoLIFEGAUGE:
					temp_sub->d9 = 0; //Unused, doesn't do anything? Clear it...
					break;
			}
		}
		
		switch(temp_sub->type)
		{
		case ssoTEXT:
		case ssoTEXTBOX:
		case ssoCURRENTITEMTEXT:
		case ssoCURRENTITEMCLASSTEXT:
			if(g->objects[j].dp1 != NULL) delete[](char *)g->objects[j].dp1;
			
			memcpy(&g->objects[j],temp_sub,sizeof(subscreen_object));
			g->objects[j].dp1 = new char[temp_size+2];
			strcpy((char*)g->objects[j].dp1,tempdp1);
			break;
			
		case ssoCOUNTER:
			if(s_version<3)
			{
				temp_sub->d6=(temp_sub->d6?1:0)+(temp_sub->d8?2:0);
				temp_sub->d8=0;
			}
			
		default:
			memcpy(&g->objects[j],temp_sub,sizeof(subscreen_object));
			break;
		}
		
		g->name[0] = '\0';
		strncat(g->name, tempname, 64 - 1);
		g->ss_type = temp_ss;
	}
	
	for(j=numsub; j<MAXSUBSCREENITEMS; j++)
	{
		//clear all unused object in this subscreen -DD
		switch(g->objects[j].type)
		{
		case ssoTEXT:
		case ssoTEXTBOX:
		case ssoCURRENTITEMTEXT:
		case ssoCURRENTITEMCLASSTEXT:
			if(g->objects[j].dp1 != NULL) delete [](char *)g->objects[j].dp1;
			
			//fall through
		default:
			memset(&g->objects[j],0,sizeof(subscreen_object));
			break;
		}
	}
	
	return 0;
}

int32_t readsubscreens(PACKFILE *f)
{
	word s_version, s_cversion;
	dword dummy;
	if(!p_igetw(&s_version,f))
		return qe_invalid;
	FFCore.quest_format[vSubscreen] = s_version;
	if(!p_igetw(&s_cversion,f))
		return qe_invalid;
	if(!p_igetl(&dummy,f)) //section size
		return qe_invalid;
	
	if(s_version < 8)
		return read_old_subscreens(f,s_version);
	
	subscreens_active.clear();
	subscreens_passive.clear();
	subscreens_overlay.clear();
	
	byte sz;
	if(!p_getc(&sz,f))
		return qe_invalid;
	for(byte q = 0; q < sz; ++q)
	{
		ZCSubscreen& tmp = subscreens_active.emplace_back();
		if (auto ret = tmp.read(f, s_version))
			return ret;
	}
	if(!p_getc(&sz,f))
		return qe_invalid;
	for(byte q = 0; q < sz; ++q)
	{
		ZCSubscreen& tmp = subscreens_passive.emplace_back();
		if (auto ret = tmp.read(f, s_version))
			return ret;
	}
	if(!p_getc(&sz,f))
		return qe_invalid;
	for(byte q = 0; q < sz; ++q)
	{
		ZCSubscreen& tmp = subscreens_overlay.emplace_back();
		if (auto ret = tmp.read(f, s_version))
			return ret;
	}
	return 0;
}

void reset_subscreen(subscreen_group *tempss)
{
    for(int32_t i=0; i<MAXSUBSCREENITEMS; ++i)
    {
        switch(tempss->objects[i].type)
        {
        case ssoTEXT:
        case ssoTEXTBOX:
        case ssoCURRENTITEMTEXT:
        case ssoCURRENTITEMCLASSTEXT:
            if(tempss->objects[i].dp1 != NULL) delete [](char *)tempss->objects[i].dp1;
            
            //fall through
        default:
            memset(&tempss->objects[i],0,sizeof(subscreen_object));
            break;
        }
    }
}

void reset_subscreens()
{
	subscreens_active.clear();
	subscreens_passive.clear();
	subscreens_overlay.clear();
}

int32_t setupsubscreens()
{
    reset_subscreens();
	//return 0;
	for(int q = 0; q < 2; ++q)
	{
		subscreens_active.emplace_back();
		subscreens_passive.emplace_back();
	}
    int32_t tempsubscreen=subscr_mode;
    
    if(tempsubscreen>=ssdtMAX)
        tempsubscreen=0;
    
    switch(tempsubscreen)
    {
		case ssdtOLD:
		case ssdtNEWSUBSCR:
		case ssdtREV2:
		case ssdtBSZELDA:
		case ssdtBSZELDAMODIFIED:
		case ssdtBSZELDAENHANCED:
		case ssdtBSZELDACOMPLETE:
		{
			subscreens_active[0].load_old(default_subscreen_active[tempsubscreen][0]);
			subscreens_active[0].sub_type=sstACTIVE;
			subscreens_active[0].name = "Active Subscreen (Triforce)";
			subscreens_active[1].load_old(default_subscreen_active[tempsubscreen][1]);
			subscreens_active[1].sub_type=sstACTIVE;
			subscreens_active[1].name = "Active Subscreen (Dungeon Map)";
			subscreens_passive[0].load_old(default_subscreen_passive[tempsubscreen][0]);
			subscreens_passive[0].sub_type=sstPASSIVE;
			subscreens_passive[0].name = "Passive Subscreen (Magic)";
			subscreens_passive[1].load_old(default_subscreen_passive[tempsubscreen][1]);
			subscreens_passive[1].sub_type=sstPASSIVE;
			subscreens_passive[1].name = "Passive Subscreen (No Magic)";
			break;
		}
		
		case ssdtZ3:
		{
			subscreens_active[0].load_old(z3_active_a);
			subscreens_active[0].sub_type=sstACTIVE;
			subscreens_active[1].load_old(z3_active_ab);
			subscreens_active[1].sub_type=sstACTIVE;
			subscreens_passive[0].load_old(z3_passive_a);
			subscreens_passive[0].sub_type=sstPASSIVE;
			subscreens_passive[1].load_old(z3_passive_ab);
			subscreens_passive[1].sub_type=sstPASSIVE;
			break;
		}
    }
    subscr_mode = ssdtMAX;
    return 0;
}

extern std::vector<std::shared_ptr<zasm_script>> zasm_scripts;

extern script_data *ffscripts[NUMSCRIPTFFC];
extern script_data *itemscripts[NUMSCRIPTITEM];
extern script_data *guyscripts[NUMSCRIPTGUYS];
extern script_data *lwpnscripts[NUMSCRIPTWEAPONS];
extern script_data *ewpnscripts[NUMSCRIPTWEAPONS];
extern script_data *globalscripts[NUMSCRIPTGLOBAL];
extern script_data *genericscripts[NUMSCRIPTSGENERIC];
extern script_data *playerscripts[NUMSCRIPTHERO];
extern script_data *screenscripts[NUMSCRIPTSCREEN];
extern script_data *dmapscripts[NUMSCRIPTSDMAP];
extern script_data *itemspritescripts[NUMSCRIPTSITEMSPRITE];
extern script_data *comboscripts[NUMSCRIPTSCOMBODATA];
extern script_data *subscreenscripts[NUMSCRIPTSSUBSCREEN];

static std::vector<const script_data*> read_scripts;

int32_t readffscript(PACKFILE *f, zquestheader *Header)
{
	int32_t dummy;
	word s_version=0, s_cversion=0, zmeta_version=0;
	byte numscripts=0;
	numscripts=numscripts; //to avoid unused variables warnings
	int32_t ret;
	read_scripts.clear();
	zasm_scripts.clear();
	
	//section version info
	if(!p_igetw(&s_version,f))
	{
		return qe_invalid;
	}
	
	FFCore.quest_format[vFFScript] = s_version;
	
	if(!p_igetw(&s_cversion,f))
	{
		return qe_invalid;
	}
	
	if(s_version >= 18)
	{
		if(!p_igetw(&zmeta_version,f))
		{
			return qe_invalid;
		}
	}
	
	//section size
	if(!p_igetl(&dummy,f))
	{
		return qe_invalid;
	}

	if ( FFCore.quest_format[vLastCompile] < 13 ) FFCore.quest_format[vLastCompile] = s_version;
	al_trace("Loaded scripts last compiled in ZScript version: %d\n", (FFCore.quest_format[vLastCompile]));
	
	//finally...  section data
	for(int32_t i = 0; i < ((s_version < 2) ? NUMSCRIPTFFCOLD : NUMSCRIPTFFC); i++)
	{
		ret = read_one_ffscript(f, Header, i, s_version, s_cversion, ffscripts[i], zmeta_version);
		
		if (ret)
		{
			return qe_invalid;
		}
	}
	
	/* HIGHLY UNORTHODOX UPDATING THING, by Deedee
	* This fixes changes to sprite jump values introduced in early 2.55 alphas.
	* Zoria didn't bump up the versions as liberally as he should have, but thankfully
	* there was a version bump a week before a change that broke stuff.
	*/
	if(((Header->zelda_version < 0x253)||((Header->zelda_version == 0x253)&&(Header->build<33))||((Header->zelda_version > 0x253) && s_version < 12)))
	{
		set_qr(qr_SPRITE_JUMP_IS_TRUNCATED,1);
	}
	if(s_version < 19)
	{
		set_qr(qr_FLUCTUATING_ENEMY_JUMP,1);
	}
	
	if(s_version > 1)
	{
		for(int32_t i = 0; i < NUMSCRIPTITEM; i++)
		{
			ret = read_one_ffscript(f, Header, i, s_version, s_cversion, itemscripts[i], zmeta_version);
			
			if (ret)
			{
				return qe_invalid;
			}
		}
		
		for(int32_t i = 0; i < NUMSCRIPTGUYS; i++)
		{
			ret = read_one_ffscript(f, Header, i, s_version, s_cversion, guyscripts[i], zmeta_version);
			
			if (ret)
			{
				return qe_invalid;
			}
		}
		
		script_data *fake = new script_data(ScriptType::None, 0);
		for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
		{
			ret = read_one_ffscript(f, Header, i, s_version, s_cversion, fake, zmeta_version);
			
			if (ret)
			{
				return qe_invalid;
			}
		}
		delete fake;
	
		for(int32_t i = 0; i < NUMSCRIPTSCREEN; i++)
		{
			ret = read_one_ffscript(f, Header, i, s_version, s_cversion, screenscripts[i], zmeta_version);
			
			if (ret)
			{
				return qe_invalid;
			}
		}
	
		if(s_version > 16)
		{
			for(int32_t i = 0; i < NUMSCRIPTGLOBAL; ++i)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, globalscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
		else if(s_version > 13)
		{
			for(int32_t i = 0; i < NUMSCRIPTGLOBAL255OLD; ++i)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, globalscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			
			if(globalscripts[GLOBAL_SCRIPT_ONSAVE] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONSAVE];
				
			globalscripts[GLOBAL_SCRIPT_ONSAVE] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONSAVE);
		}
		else if(s_version > 4)
		{
			for(int32_t i = 0; i < NUMSCRIPTGLOBAL253; ++i)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, globalscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			
			if(globalscripts[GLOBAL_SCRIPT_ONLAUNCH] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONLAUNCH];
				
			globalscripts[GLOBAL_SCRIPT_ONLAUNCH] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONLAUNCH);
			
			if(globalscripts[GLOBAL_SCRIPT_ONCONTGAME] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONCONTGAME];
				
			globalscripts[GLOBAL_SCRIPT_ONCONTGAME] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONCONTGAME);
			
			if(globalscripts[GLOBAL_SCRIPT_F6] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_F6];
				
			globalscripts[GLOBAL_SCRIPT_F6] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_F6);
			
			if(globalscripts[GLOBAL_SCRIPT_ONSAVE] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONSAVE];
				
			globalscripts[GLOBAL_SCRIPT_ONSAVE] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONSAVE);
		}
		else
		{
			for(int32_t i = 0; i < NUMSCRIPTGLOBALOLD; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, globalscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			
			if(globalscripts[GLOBAL_SCRIPT_ONSAVELOAD] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONSAVELOAD];
				
			globalscripts[GLOBAL_SCRIPT_ONSAVELOAD] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONSAVELOAD);
			
			if(globalscripts[GLOBAL_SCRIPT_ONLAUNCH] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONLAUNCH];
				
			globalscripts[GLOBAL_SCRIPT_ONLAUNCH] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONLAUNCH);
			
			if(globalscripts[GLOBAL_SCRIPT_ONCONTGAME] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONCONTGAME];
				
			globalscripts[GLOBAL_SCRIPT_ONCONTGAME] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONCONTGAME);
			
			if(globalscripts[GLOBAL_SCRIPT_F6] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_F6];
				
			globalscripts[GLOBAL_SCRIPT_F6] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_F6);
			
			if(globalscripts[GLOBAL_SCRIPT_ONSAVE] != NULL)
				delete globalscripts[GLOBAL_SCRIPT_ONSAVE];
				
			globalscripts[GLOBAL_SCRIPT_ONSAVE] = new script_data(ScriptType::Global, GLOBAL_SCRIPT_ONSAVE);
		}
		
		if(s_version > 10) //expanded the number of Hero scripts to 5. 
		{
			for(int32_t i = 0; i < NUMSCRIPTHERO; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, playerscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
		else
		{
			for(int32_t i = 0; i < NUMSCRIPTHEROOLD; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, playerscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			if(playerscripts[3] != NULL)
				delete playerscripts[3];
					
			playerscripts[3] = new script_data(ScriptType::Hero, 3);
			
			if(playerscripts[4] != NULL)
				delete playerscripts[4];
					
			playerscripts[4] = new script_data(ScriptType::Hero, 4);
		}
		if(s_version > 8 && s_version < 10)
		{
			
			for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, ewpnscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			for(int32_t i = 0; i < NUMSCRIPTSDMAP; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, dmapscripts[i], zmeta_version);
			
				if (ret)
				{
					return qe_invalid;
				}
			}
			
		}
		if(s_version >= 10)
		{
			for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, lwpnscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			for(int32_t i = 0; i < NUMSCRIPTWEAPONS; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, ewpnscripts[i], zmeta_version);
				
				if (ret)
				{
					return qe_invalid;
				}
			}
			for(int32_t i = 0; i < NUMSCRIPTSDMAP; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, dmapscripts[i], zmeta_version);
			
				if (ret)
				{
					return qe_invalid;
				}
			}
			
		}
		if(s_version >=12)
		{
			for(int32_t i = 0; i < NUMSCRIPTSITEMSPRITE; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, itemspritescripts[i], zmeta_version);
					
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
		if(s_version >=15)
		{
			for(int32_t i = 0; i < NUMSCRIPTSCOMBODATA; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, comboscripts[i], zmeta_version);
					
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
		if(s_version >19)
		{
			word numgenscripts = NUMSCRIPTSGENERIC;
			if(!p_igetw(&numgenscripts,f))
			{
				return qe_invalid;
			}
			for(int32_t i = 0; i < numgenscripts; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, genericscripts[i], zmeta_version);
					
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
		if(s_version >21)
		{
			word numsubscripts = NUMSCRIPTSSUBSCREEN;
			if(!p_igetw(&numsubscripts,f))
			{
				return qe_invalid;
			}
			for(int32_t i = 0; i < numsubscripts; i++)
			{
				ret = read_one_ffscript(f, Header, i, s_version, s_cversion, subscreenscripts[i], zmeta_version);
					
				if (ret)
				{
					return qe_invalid;
				}
			}
		}
	}
	
	if(s_version > 2)
	{
		int32_t bufsize;
		p_igetl(&bufsize, f);
		if (bufsize < 0 || bufsize > 1024*1024*10)
		{
			// God help anyone storing more than 10MB of code in the script buffer.
			return qe_invalid;
		}
		char * buf = new char[bufsize+1];
		pfread(buf, bufsize, f);
		buf[bufsize]=0;
		
		zScript = string(buf);
			
		delete[] buf;
		word numffcbindings;
		p_igetw(&numffcbindings, f);
		
		for(int32_t i=0; i<numffcbindings; i++)
		{
			word id;
			p_igetw(&id, f);
			p_igetl(&bufsize, f);
			if (bufsize < 0 || bufsize > 1024)
				return qe_invalid;
			buf = new char[bufsize+1];
			pfread(buf, bufsize, f);
			buf[bufsize]=0;
			
			//fix for buggy older saved quests -DD
			if(id < NUMSCRIPTFFC-1)
				ffcmap[id].scriptname = buf;
				
			delete[] buf;
		}
		
		word numglobalbindings;
		p_igetw(&numglobalbindings, f);
		
		for(int32_t i=0; i<numglobalbindings; i++)
		{
			word id;
			p_igetw(&id, f);
			p_igetl(&bufsize, f);
			if (bufsize < 0 || bufsize > 1024)
				return qe_invalid;
			buf = new char[bufsize+1];
			pfread(buf, bufsize, f);
			buf[bufsize]=0;
			
			// id in principle should be valid, since slot assignment cannot assign a global script to a bogus slot.
			// However, because of a corruption bug, some 2.50.x quests contain bogus entries in the global bindings table.
			// Ignore these. -DD
			if (id < NUMSCRIPTGLOBAL)
			{
				//Disable old '~Continue's, they'd wreak havoc. Bit messy, apologies ~Joe
				if(strcmp(buf,"~Continue") == 0)
				{
					globalmap[id].scriptname = "";
					
					if(globalscripts[GLOBAL_SCRIPT_ONSAVELOAD] != NULL)
						globalscripts[GLOBAL_SCRIPT_ONSAVELOAD]->disable();
				}
				else
				{
					globalmap[id].scriptname = buf;
				}
			}
			
			delete[] buf;
		}
		
		if(s_version > 3)
		{
			word numitembindings;
			p_igetw(&numitembindings, f);
			
			for(int32_t i=0; i<numitembindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTITEM-1)
					itemmap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
		//(v9+)
		if(s_version > 8)
		{
			//npc scripts
			word numnpcbindings;
			p_igetw(&numnpcbindings, f);
			
			for(int32_t i=0; i<numnpcbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTGUYS-1)
					npcmap[id].scriptname = buf;
					
				delete[] buf;
			}
			//lweapon
			word numlwpnbindings;
			p_igetw(&numlwpnbindings, f);
			
			for(int32_t i=0; i<numlwpnbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTWEAPONS-1)
					lwpnmap[id].scriptname = buf;
					
				delete[] buf;
			}
			//eweapon
			word numewpnbindings;
			p_igetw(&numewpnbindings, f);
			
			for(int32_t i=0; i<numewpnbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTWEAPONS-1)
					ewpnmap[id].scriptname = buf;
					
				delete[] buf;
			}
			//hero
			word numherobindings;
			p_igetw(&numherobindings, f);
			
			for(int32_t i=0; i<numherobindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTHERO-1)
					playermap[id].scriptname = buf;
					
				delete[] buf;
			}
			//dmaps
			word numdmapbindings;
			p_igetw(&numdmapbindings, f);
			
			for(int32_t i=0; i<numdmapbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSDMAP-1)
					dmapmap[id].scriptname = buf;
					
				delete[] buf;
			}
			//screen
			word numscreenbindings;
			p_igetw(&numscreenbindings, f);
			
			for(int32_t i=0; i<numscreenbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSDMAP-1)
					screenmap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
		if(s_version > 11)
		{
			word numspritebindings;
			p_igetw(&numspritebindings, f);
			
			for(int32_t i=0; i<numspritebindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSDMAP-1)
					itemspritemap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
		if(s_version >= 15)
		{
			word numcombobindings;
			p_igetw(&numcombobindings, f);
			
			for(int32_t i=0; i<numcombobindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSCOMBODATA-1)
					comboscriptmap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
		if(s_version > 19)
		{
			word numgenericbindings;
			p_igetw(&numgenericbindings, f);
			
			for(int32_t i=0; i<numgenericbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSGENERIC-1)
					genericmap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
		if(s_version > 21)
		{
			word numsubscreenbindings;
			p_igetw(&numsubscreenbindings, f);
			
			for(int32_t i=0; i<numsubscreenbindings; i++)
			{
				word id;
				p_igetw(&id, f);
				p_igetl(&bufsize, f);
				if (bufsize < 0 || bufsize > 1024)
					return qe_invalid;
				buf = new char[bufsize+1];
				pfread(buf, bufsize, f);
				buf[bufsize]=0;
				
				//fix this too
				if(id <NUMSCRIPTSSUBSCREEN-1)
					subscreenmap[id].scriptname = buf;
					
				delete[] buf;
			}
		}
	}

	// Set ZScriptVersion to the value encoded in the script's meta.ffscript_v.
	// This is only updated when the scripts have been recompiled.
	// They should all match each other.
	std::optional<word> zscript_version;
	if (s_version >= 16)
	{
		for (auto script : read_scripts)
		{
			if (script->meta.ffscript_v == 0 || script->meta.ffscript_v > s_version)
				break;

			if (!zscript_version.has_value())
			{
				zscript_version = script->meta.ffscript_v;
				continue;
			}

			if (zscript_version.value() != script->meta.ffscript_v)
			{
				zscript_version.reset();
				break;
			}
		}

		if (!zscript_version.has_value())
			al_trace("WARNING: Setting zscript version to section version as fallback.\n");
	}
	setZScriptVersion(zscript_version.value_or(s_version));
	read_scripts.clear();

	return 0;
}

void(*reset_scripts_hook)();

void reset_scripts()
{
	// We can't modify the script data while jit threads are possibly compiling them.
	if (reset_scripts_hook)
		reset_scripts_hook();

	for(int32_t i=0; i<NUMSCRIPTSGENERIC; i++)
	{
		if (genericscripts[i]!=NULL) genericscripts[i]->disable();
		else genericscripts[i] = new script_data({ScriptType::Generic, i});
	}

	for(int32_t i=0; i<NUMSCRIPTFFC; i++)
	{
		if (ffscripts[i])
			ffscripts[i]->disable();
		else
			ffscripts[i] = new script_data(ScriptType::FFC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTITEM; i++)
	{
		if (itemscripts[i])
			itemscripts[i]->disable();
		else
			itemscripts[i] = new script_data(ScriptType::Item, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
	{
		if (guyscripts[i])
			guyscripts[i]->disable();
		else
			guyscripts[i] = new script_data(ScriptType::NPC, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
	{
		if (screenscripts[i])
			screenscripts[i]->disable();
		else
			screenscripts[i] = new script_data(ScriptType::Screen, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTGLOBAL; i++)
	{
		if (globalscripts[i])
			globalscripts[i]->disable();
		else
			globalscripts[i] = new script_data(ScriptType::Global, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTHERO; i++)
	{
		if (playerscripts[i])
			playerscripts[i]->disable();
		else
			playerscripts[i] = new script_data(ScriptType::Hero, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		if (lwpnscripts[i])
			lwpnscripts[i]->disable();
		else
			lwpnscripts[i] = new script_data(ScriptType::Lwpn, i);
	}
	 for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
	{
		if (ewpnscripts[i])
			ewpnscripts[i]->disable();
		else
			ewpnscripts[i] = new script_data(ScriptType::Ewpn, i);
	}
	
	for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
	{
		if (dmapscripts[i])
			dmapscripts[i]->disable();
		else
			dmapscripts[i] = new script_data(ScriptType::DMap, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
	{
		if (itemspritescripts[i])
			itemspritescripts[i]->disable();
		else
			itemspritescripts[i] = new script_data(ScriptType::ItemSprite, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
	{
		if (comboscripts[i])
			comboscripts[i]->disable();
		else
			comboscripts[i] = new script_data(ScriptType::Combo, i);
	}
	for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
	{
		if (subscreenscripts[i])
			subscreenscripts[i]->disable();
		else
			subscreenscripts[i] = new script_data(ScriptType::EngineSubscreen, i);
	}
}

int32_t read_one_ffscript(PACKFILE *f, zquestheader *, int32_t script_index, word s_version, word , script_data *script, word zmeta_version)
{
	ASSERT(script);

	//Please also update loadquest() when modifying this method -DD
	char b33[34] = {0};
	b33[33] = 0;
	int32_t num_commands=1000;
	
	if(s_version>=2)
	{
		if(!p_igetl(&num_commands,f))
		{
			return qe_invalid;
		}
	}

#ifdef ZC_FUZZ
	const int32_t command_limit = 300000;
#else
	const int32_t command_limit = 10000000;
#endif
	if (num_commands < 0 || num_commands > command_limit)
	{
		return qe_invalid;
	}

	std::vector<ffscript> zasm;
	zasm.reserve(num_commands);

	if(s_version >= 16)
	{
		zasm_meta temp_meta;
		
		if(!p_igetw(&(temp_meta.zasm_v),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.meta_v),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.ffscript_v),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_meta.script_type),f))
		{
			return qe_invalid;
		}
		
		for(int32_t q = 0; q < 8; ++q)
		{
			if(zmeta_version < 3)
			{
				for(int32_t c = 0; c < 33; ++c)
				{
					if(!p_getc(&(b33[c]),f))
					{
						return qe_invalid;
					}
				}
				temp_meta.run_idens[q].assign(b33);
			}
			else
			{
				if(!p_getcstr(&temp_meta.run_idens[q],f))
				{
					return qe_invalid;
				}
			}
		}
		
		for(int32_t q = 0; q < 8; ++q)
		{
			if(!p_getc(&(temp_meta.run_types[q]),f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_getc(&(temp_meta.flags),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.compiler_v1),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.compiler_v2),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.compiler_v3),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_meta.compiler_v4),f))
		{
			return qe_invalid;
		}
		
		if(zmeta_version == 2)
		{
			for(int32_t c = 0; c < 33; ++c)
			{
				if(!p_getc(&b33[c],f))
				{
					return qe_invalid;
				}
			}
			temp_meta.script_name.assign(b33);
			
			for(int32_t c = 0; c < 33; ++c)
			{
				if(!p_getc(&b33[c],f))
				{
					return qe_invalid;
				}
			}
			temp_meta.author.assign(b33);
		}
		else if(zmeta_version > 2)
		{
			if(!p_getcstr(&temp_meta.script_name,f))
				return qe_invalid;
			if(!p_getcstr(&temp_meta.author,f))
				return qe_invalid;
			auto num_meta_attrib = (zmeta_version < 5 ? 4 : 10);
			for(auto q = 0; q < num_meta_attrib; ++q)
			{
				if(!p_getcstr(&temp_meta.attributes[q],f))
					return qe_invalid;
				if(!p_getwstr(&temp_meta.attributes_help[q],f))
					return qe_invalid;
			}
			for(auto q = 0; q < 8; ++q)
			{
				if(!p_getcstr(&temp_meta.attribytes[q],f))
					return qe_invalid;
				if(!p_getwstr(&temp_meta.attribytes_help[q],f))
					return qe_invalid;
			}
			for(auto q = 0; q < 8; ++q)
			{
				if(!p_getcstr(&temp_meta.attrishorts[q],f))
					return qe_invalid;
				if(!p_getwstr(&temp_meta.attrishorts_help[q],f))
					return qe_invalid;
			}
			for(auto q = 0; q < 16; ++q)
			{
				if(!p_getcstr(&temp_meta.usrflags[q],f))
					return qe_invalid;
				if(!p_getwstr(&temp_meta.usrflags_help[q],f))
					return qe_invalid;
			}
		}
		if(zmeta_version > 3)
		{
			for(auto q = 0; q < 8; ++q)
			{
				if(!p_getcstr(&temp_meta.initd[q],f))
					return qe_invalid;
				if(!p_getwstr(&temp_meta.initd_help[q],f))
					return qe_invalid;
			}
			for(auto q = 0; q < 8; ++q)
			{
				if(!p_getc(&temp_meta.initd_type[q],f))
					return qe_invalid;
			}
		}
		else
		{
			for(auto q = 0; q < 8; ++q)
			{
				temp_meta.initd[q] = temp_meta.run_idens[q];
			}
		}
		
		script->meta = temp_meta;
	} else script->meta = {};
	
	for(int32_t j=0; j<num_commands; j++)
	{
		auto& sc = zasm.emplace_back();
		if(!p_igetw(&sc.command,f))
		{
			return qe_invalid;
		}
		
		if(sc.command == 0xFFFF)
		{
			break;
		}

		if(!p_igetl(&sc.arg1,f))
		{
			return qe_invalid;
		}
		
		if(!p_igetl(&sc.arg2,f))
		{
			return qe_invalid;
		}
		
		if(s_version >= 24)
			if(!p_igetl(&sc.arg3,f))
				return qe_invalid;
		
		if(s_version >= 21)
		{
			uint32_t sz = 0;
			if(!p_igetl(&sz,f))
			{
				return qe_invalid;
			}
			if(sz) //string found
			{
				sc.strptr = new std::string();
				char dummy;
				for(size_t q = 0; q < sz; ++q)
				{
					if(!p_getc(&dummy,f))
					{
						return qe_invalid;
					}
					sc.strptr->push_back(dummy);
				}
			}
			if(!p_igetl(&sz,f))
			{
				return qe_invalid;
			}
			if(sz) //vector found
			{
				sc.vecptr = new std::vector<int32_t>();
				int32_t dummy;
				for(size_t q = 0; q < sz; ++q)
				{
					if(!p_igetl(&dummy,f))
					{
						return qe_invalid;
					}
					sc.vecptr->push_back(dummy);
				}
			}
		}
	}

	// If the first command is unknown, invalidate the whole thing.
	// Saw this for https://www.purezc.net/index.php?page=quests&id=411 hero script 0
	if (!zasm.empty() && zasm[0].command >= NUMCOMMANDS && zasm[0].command != 0xFFFF)
	{
		al_trace("Warning: found script with bad instruction, disabling script: %s %d\n", ScriptTypeToString(script->id.type), script->id.index);
		zasm.clear();
	}

	zasm_script_id id = zasm_scripts.size();
	auto& zs = zasm_scripts.emplace_back(std::make_shared<zasm_script>(id, script->name(), std::move(zasm)));
	script->zasm_script = zs;
	if (script->valid())
		read_scripts.push_back(script);

	return 0;
}

extern SAMPLE customsfxdata[WAV_COUNT];
extern uint8_t customsfxflag[WAV_COUNT>>3];
extern int32_t sfxdat;
extern DATAFILE *sfxdata;
const char *old_sfx_string[Z35] =
{
    "Arrow", "Sword beam", "Bomb blast", "Boomerang",  "Subscreen cursor",
    "Shield is hit", "Item chime",  "Roar (Dodongo, Gohma)", "Shutter", "Enemy dies",
    "Enemy is hit", "Low hearts warning", "Fire", "Ganon's fanfare", "Boss is hit", "Hammer",
    "Hookshot", "Message", "Hero is hit", "Item fanfare", "Bomb placed", "Item pickup",
    "Refill", "Roar (Aquamentus, Gleeok, Ganon)", "Item pickup 2", "Ocean ambience",
    "Secret chime", "Hero dies", "Stairs", "Sword", "Roar (Manhandla, Digdogger, Patra)",
    "Wand magic", "Whistle", "Zelda's fanfare", "Charging weapon", "Charging weapon 2",
    "Divine Fire", "Enemy falls from ceiling", "Divine Escape", "Fireball", "Tall Grass slashed",
    "Pound pounded", "Hover Boots", "Ice magic", "Jump", "Lens of Truth off", "Lens of Truth on",
    "Divine Protection shield", "Divine Protection 2", "Push block", "Rock", "Spell rocket down",
    "Spell rocket up", "Sword spin attack", "Splash", "Summon magic", "Sword tapping",
    "Sword tapping (secret)", "Whistle whirlwind", "Cane of Byrna orbit"
};
char *sfx_string[WAV_COUNT];

int32_t readsfx(PACKFILE *f, zquestheader *Header)
{
	//these are here to bypass compiler warnings about unused arguments
	Header=Header;
	
	int32_t dummy;
	word s_version=0, s_cversion=0;
	//int32_t ret;
	SAMPLE temp_sample = {};
	temp_sample.loop_start=0;
	temp_sample.loop_end=0;
	temp_sample.param=0;
	
	//section version info
	if(!p_igetw(&s_version,f))
	{
		return qe_invalid;
	}
	
	FFCore.quest_format[vSFX] = s_version;
	
	if(!p_igetw(&s_cversion,f))
	{
		return qe_invalid;
	}
	
	//section size
	if(!p_igetl(&dummy,f))
	{
		return qe_invalid;
	}
	
	/* HIGHLY UNORTHODOX UPDATING THING, by L
	 * This fixes quests made before revision 411 (such as the 'Lost Isle Build'),
	 * where the meaning of GOTOLESS changed. It also coincided with V_SFX
	 * changing from 1 to 2.
	 */
	if(s_version < 2)
		set_qr(qr_GOTOLESSNOTEQUAL,1);
		
	/* End highly unorthodox updating thing */
	
	int32_t wavcount = WAV_COUNT;
	
	if(s_version < 6)
		wavcount = 128;
		
	uint8_t tempflag[WAV_COUNT>>3];
	
	if(s_version < 4)
	{
		memset(tempflag, 0xFF, WAV_COUNT>>3);
	}
	else
	{
		if(s_version < 6)
			memset(tempflag, 0, WAV_COUNT>>3);
			
		for(int32_t i=0; i<(wavcount>>3); i++)
		{
			p_getc(&tempflag[i], f);
		}
		
	}
		
	if(s_version>4)
	{
		for(int32_t i=1; i<WAV_COUNT; i++)
		{
			sprintf(sfx_string[i],"s%03d",i);
			
			if((i<Z35))
				strcpy(sfx_string[i], old_sfx_string[i-1]);
			
			if(i>=wavcount)
				continue;
			if(get_bit(tempflag, i-1))
			{
				char tempname[36];
				
				if(!pfread(tempname, 36, f))
				{
					return qe_invalid;
				}
				
				sfx_string[i][0] = '\0';
				strncat(sfx_string[i], tempname, 36 - 1);
			}
			else
			{
				sprintf(sfx_string[i],"s%03d",i);
				
				if(i<Z35)
					strcpy(sfx_string[i], old_sfx_string[i-1]);
				sfx_string[i][35] = 0; //Force NULL Termination
			}
		}
	}
	else
	{
		for(int32_t i=1; i<WAV_COUNT; i++)
		{
			sprintf(sfx_string[i],"s%03d",i);
			
			if(i<Z35)
				strcpy(sfx_string[i], old_sfx_string[i-1]);
		}
	}
	
	//finally...  section data
	for(int32_t i=1; i<wavcount; i++)
	{
		if(get_bit(tempflag, i-1))
		{
			
			if(!p_igetl(&dummy,f))
			{
				return qe_invalid;
			}
			
			(temp_sample.bits) = dummy;
			
			if(!p_igetl(&dummy,f))
			{
				return qe_invalid;
			}
			
			(temp_sample.stereo) = dummy;
			
			if(!p_igetl(&dummy,f))
			{
				return qe_invalid;
			}
			
			(temp_sample.freq) = dummy;
			
			if(!p_igetl(&dummy,f))
			{
				return qe_invalid;
			}
			
			(temp_sample.priority) = dummy;
			
			if(!p_igetl(&(temp_sample.len),f))
			{
				return qe_invalid;
			}
			
			if(!p_igetl(&(temp_sample.loop_start),f))
			{
				return qe_invalid;
			}
			
			if(!p_igetl(&(temp_sample.loop_end),f))
			{
				return qe_invalid;
			}
			
			if(!p_igetl(&(temp_sample.param),f))
			{
				return qe_invalid;
			}

			auto len = (temp_sample.bits==8?1:2)*(temp_sample.stereo==0?1:2)*temp_sample.len;
#ifdef ZC_FUZZ
	const int32_t sfx_limit = 100000;
			if (len < 0 || len > sfx_limit)
			{
				return qe_invalid;
			}
#endif
			temp_sample.data = calloc(len,1);
			
			if(s_version < 3)
				len = (temp_sample.bits==8?1:2)*temp_sample.len;
				
			//old-style, non-portable loading (Bad Allegro! Bad!!) -DD
			if(s_version < 2)
			{
				if(!pfread(temp_sample.data, len,f))
				{
					return qe_invalid;
				}
			}
			else
			{
				//re-endianfy the data
				int32_t wordstoread = len / sizeof(word);
				
				for(int32_t j=0; j<wordstoread; j++)
				{
					word temp;
					
					if(!p_igetw(&temp, f))
					{
						return qe_invalid;
					}
					
					((word *)temp_sample.data)[j] = temp;
				}
			}
		}
		else if(i < Z35)
		{
			SAMPLE* datsamp = (SAMPLE*)(sfxdata[i].dat);
			memcpy(&temp_sample, datsamp, sizeof(SAMPLE));
			set_bit(tempflag, i-1, 1);
			int32_t len = (temp_sample.bits==8?1:2)*(temp_sample.stereo==0?1:2)*temp_sample.len;
			temp_sample.data = calloc(len,1);
			memcpy(temp_sample.data, datsamp->data, len);
		}
		else continue;
		
		if(customsfxdata[i].data!=NULL)
		{
			// delete [] customsfxdata[i].data;
			free(customsfxdata[i].data);
		}
		
		// customsfxdata[i].data = new byte[(temp_sample.bits==8?1:2)*temp_sample.len];
		int32_t len2 = (temp_sample.bits==8?1:2)*(temp_sample.stereo==0?1:2)*temp_sample.len;
		customsfxdata[i].data = calloc(len2,1);
		customsfxdata[i].bits = temp_sample.bits;
		customsfxdata[i].stereo = temp_sample.stereo;
		customsfxdata[i].freq = temp_sample.freq;
		customsfxdata[i].priority = temp_sample.priority;
		customsfxdata[i].len = temp_sample.len;
		customsfxdata[i].loop_start = temp_sample.loop_start;
		customsfxdata[i].loop_end = temp_sample.loop_end;
		customsfxdata[i].param = temp_sample.param;
		int32_t cpylen = len2;
		
		if(s_version<3)
		{
			cpylen = (temp_sample.bits==8?1:2)*temp_sample.len;
			al_trace("WARNING: Quest SFX %d is in stereo, and may be corrupt.\n",i);
		}
		
		memcpy(customsfxdata[i].data,temp_sample.data,cpylen);
		
		free(temp_sample.data);
	}
	
	memcpy(customsfxflag, tempflag, WAV_COUNT>>3);
	
	sfxdat=0;
	return 0;
}

void setupsfx()
{
    for(int32_t i=1; i<WAV_COUNT; i++)
    {
        sprintf(sfx_string[i],"s%03d",i);
        
        if(i<Z35)
        {
            strcpy(sfx_string[i], old_sfx_string[i-1]);
        }
        
        memset(customsfxflag, 0, WAV_COUNT>>3);
        
        int32_t j=i;
        
        if(i>Z35)
        {
            i=Z35;
        }
        
        SAMPLE *temp_sample = (SAMPLE *)sfxdata[i].dat;
        
        if(customsfxdata[j].data!=NULL)
        {
//    delete [] customsfxdata[j].data;
            free(customsfxdata[j].data);
        }
        
//    customsfxdata[j].data = new byte[(temp_sample->bits==8?1:2)*temp_sample->len];
        customsfxdata[j].data = calloc((temp_sample->bits==8?1:2)*(temp_sample->stereo == 0 ? 1 : 2)*temp_sample->len,1);
        customsfxdata[j].bits = temp_sample->bits;
        customsfxdata[j].stereo = temp_sample->stereo;
        customsfxdata[j].freq = temp_sample->freq;
        customsfxdata[j].priority = temp_sample->priority;
        customsfxdata[j].len = temp_sample->len;
        customsfxdata[j].loop_start = temp_sample->loop_start;
        customsfxdata[j].loop_end = temp_sample->loop_end;
        customsfxdata[j].param = temp_sample->param;
        memcpy(customsfxdata[j].data, (temp_sample->data), (temp_sample->bits==8?1:2)*(temp_sample->stereo==0 ? 1 : 2)*temp_sample->len);
        i=j;
    }
}

extern char *guy_string[eMAXGUYS];
extern const char *old_guy_string[OLDMAXGUYS];

int32_t readguys(PACKFILE *f, zquestheader *Header)
{
    bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_guys);
    if (should_skip) return 0;

    dword dummy;
    word guy_cversion;
    word guyversion=0;
    
    if(Header->zelda_version >= 0x193)
    {
        //section version info
        if(!p_igetw(&guyversion,f))
        {
            return qe_invalid;
        }
        
	FFCore.quest_format[vGuys] = guyversion;
	
        if(!p_igetw(&guy_cversion,f))
        {
            return qe_invalid;
        }
        al_trace("Guy CVersion is: %d\n", guy_cversion);
        //section size
        if(!p_igetl(&dummy,f))
        {
            return qe_invalid;
        }
    }
    
    if(guyversion > 3)
    {
        for(int32_t i=0; i<MAXGUYS; i++)
        {
            char tempname[64];
            
            // rev. 1511 : guyversion = 23. upped to 512 editable enemies. -Gleeok
            // if guyversion < 23 then there is only 256 enemies in the packfile, so default the rest.
            if(guyversion < 23 && i >= OLDBETAMAXGUYS)
            {
                memset(tempname, 0, sizeof(char)*64);
                sprintf(tempname, "e%03d", i);
                strcpy(guy_string[i], tempname);
                
                continue;
            }
            
            if(!pfread(tempname, 64, f))
            {
                return qe_invalid;
            }
            
            // Don't retain names of uneditable enemy entries!
			// for version upgrade to 2.5
			if(guyversion < 23 && i >= 177)
			{
				// some of the older builds have names such as 'zz123',
				// (this order gets messed up with some eXXX and some zzXXX)
				// so let's update to the newer naming convection. -Gleeok
				char tmpbuf[64];
				memset(tmpbuf, 0, sizeof(char)*64);
				sprintf(tmpbuf, "zz%03d", i);
				
				if(memcmp(tempname, tmpbuf, size_t(5)) == 0)
				{
					memset(tempname, 0, sizeof(char)*64);
					sprintf(tempname, "e%03d", i);
				}
			}
			
			if(i >= OLDMAXGUYS || strlen(tempname)<1 || tempname[strlen(tempname)-1]!=' ')
			{
				guy_string[i][0] = '\0';
				strncat(guy_string[i], tempname, 64 - 1);
			}
			else
			{
				strcpy(guy_string[i],old_guy_string[i]);
			}
        }
    }
    else
    {
		for(int32_t i=0; i<eMAXGUYS; i++)
		{
			sprintf(guy_string[i],"zz%03d",i);
		}
		
		for(int32_t i=0; i<OLDMAXGUYS; i++)
		{
			strcpy(guy_string[i],old_guy_string[i]);
		}
    }
    
    
    //finally...  section data
	init_guys(guyversion);                            //using default data for now...
	
	// Goriya guy fix
	if((Header->zelda_version < 0x211)||((Header->zelda_version == 0x211)&&(Header->build<7)))
	{
		if(get_qr(qr_NEWENEMYTILES))
		{
			guysbuf[gGORIYA].tile=130;
			guysbuf[gGORIYA].e_tile=130;
		}
	}
    
    if(Header->zelda_version < 0x193)
    {
        if(get_bit(deprecated_rules,46))
        {
            guysbuf[eDODONGO].cset=14;
            guysbuf[eDODONGO].bosspal=spDIG;
        }
    }
 // Not sure when this first changed, but it's necessary for 2.10, at least
    // @TODO: @BUG:1.92 - 1.84? Figure this out exactly for the final 2.50 release.
//2.10 Fixes  
    if((Header->zelda_version < 0x211)||((Header->zelda_version == 0x211)&&(Header->build<18)))
    {
	guysbuf[eWWIZ].editorflags |= ENEMY_FLAG5;
	guysbuf[eMOLDORM].editorflags |= ENEMY_FLAG6;
	guysbuf[eMANHAN].editorflags |= ENEMY_FLAG6;
	guysbuf[eCENT1].attributes[2] = 1;
	guysbuf[eCENT2].attributes[2] = 1;
    }
    if((Header->zelda_version <= 0x255) || (Header->zelda_version == 0x255 && Header->build < 47) )
    {
	guysbuf[eWPOLSV].defense[edefWhistle] = ed1HKO;
    }
    if(Header->zelda_version <= 0x210)
    {
        guysbuf[eGLEEOK1F].attributes[5] = 16;
        guysbuf[eGLEEOK2F].attributes[5] = 16;
        guysbuf[eGLEEOK3F].attributes[5] = 16;
        guysbuf[eGLEEOK4F].attributes[5] = 16;
	    
        guysbuf[eWIZ1].attributes[3] = 1; //only set the enemy that needs backward compat, not all of them.
        guysbuf[eBATROBE].attributes[3] = 1;
        //guysbuf[eSUMMONER].misc4 = 1;
        guysbuf[eWWIZ].attributes[3] = 1;
	    guysbuf[eDODONGO].deadsfx = 15; //In 2.10 and earlier, Dodongos used this as their death sound.
	guysbuf[eDODONGOBS].deadsfx = 15; //In 2.10 and earlier, Dodongos used this as their death sound.
    }
    if(Header->zelda_version == 0x190)
    {
	al_trace("Setting Tribble Properties for Version: %x", Header->zelda_version);
	guysbuf[eKEESETRIB].attributes[2] = eVIRE; //1.90 and earlier, keese and gel tribbles grew up into 
	guysbuf[eGELTRIB].attributes[2] = eZOL; //normal vires, and zols -Z (16th January, 2019 )
    }
    
    // The versions here may not be correct
    // zelda_version>=0x211 handled at guyversion<24
    if(Header->zelda_version <= 0x190)
    {
        guysbuf[eCENT1].attributes[2] = 0;
        guysbuf[eCENT2].attributes[2] = 0;
        guysbuf[eMOLDORM].attributes[1] = 0;
	//guysbuf[eKEESETRIB].misc3 = eVIRE; //1.90 and earlier, keese and gel tribbles grew up into 
	//guysbuf[eGELTRIB].misc3 = eZOL; //normal vires, and zols -Z (16th January, 2019 )
    }
    else if(Header->zelda_version <= 0x210)
    {
        guysbuf[eCENT1].attributes[2] = 1;
        guysbuf[eCENT2].attributes[2] = 1;
        guysbuf[eMOLDORM].attributes[1] = 0;
    }
    
    if ( Header->zelda_version < 0x211 ) //Default rest rates for phantom ghinis, peahats and keese in < 2.50 quests
    {
		guysbuf[eKEESE1].attributes[15] = 120;
		guysbuf[eKEESE2].attributes[15] = 120;
		guysbuf[eKEESE3].attributes[15] = 120;
		guysbuf[eKEESETRIB].attributes[15] = 120;
		guysbuf[eKEESE1].attributes[16] = 16;
		guysbuf[eKEESE2].attributes[16] = 16;
		guysbuf[eKEESE3].attributes[16] = 16;
		guysbuf[eKEESETRIB].attributes[16] = 16;
			
		guysbuf[ePEAHAT].attributes[15] = 80;
		guysbuf[ePEAHAT].attributes[16] = 16;
			
		guysbuf[eGHINI2].attributes[15] = 120;
		guysbuf[eGHINI2].attributes[16] = 10;
		
		if (replay_version_check(20))
		{
			guysbuf[eGHINI2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eMOLDORM].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eKEESETRIB].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eKEESE3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eKEESE2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eKEESE1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTEK1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTEK2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePEAHAT].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eROCK].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRAP].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eWALLM].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePOLSV].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eMANHAN].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK4].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIG1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIG3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIGPUP1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIGPUP2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIGPUP3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eDIGPUP4].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eRAQUAM].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eITEMFAIRY].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eFIRE].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eMANHAN2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRAP_H].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRAP_V].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRAP_LR].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRAP_UD].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePATRA1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePATRA2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePATRABS].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eBAT].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK1F].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK2F].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK3F].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGLEEOK4F].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eTRIGGER].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePATRAL2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[ePATRAL3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGOHMA1].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGOHMA2].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGOHMA3].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eGOHMA4].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eMPOLSV].moveflags |= (move_can_waterwalk|move_can_pitwalk);
			guysbuf[eWPOLSV].moveflags |= (move_can_waterwalk|move_can_pitwalk);
		}
    }
	
    
    if(guyversion<=2)
    {
        return readherosprites2(f, guyversion==2?0:-1, 0);
    }
    
    if(guyversion > 3)
    {
        guydata tempguy;
        
        for(int32_t i=0; i<MAXGUYS; i++)
        {
            if(guyversion < 23)   // May 2012 : 512 max enemies
            {
                if(i >= OLDBETAMAXGUYS)
                {
                    memset(&guysbuf[i], 0, sizeof(guydata));
                    continue;
                }
            }
            
            memset(&tempguy, 0, sizeof(guydata));
            
			uint32_t flags1;
			uint32_t flags2;
            if(!p_igetl(&(flags1),f))
            {
                return qe_invalid;
            }
			if(!p_igetl(&(flags2),f))
			{
				return qe_invalid;
			}
			tempguy.flags = guy_flags(flags1) | guy_flags(uint64_t(flags2)<<32ULL);
            
	    if ( guyversion >= 36 ) //expanded tiles
	    {
		    if(!p_igetl(&(tempguy.tile),f))
		    {
			return qe_invalid;
		    }
	    }
	    else
	    {
		    if(!p_igetw(&(tempguy.tile),f))
		    {
			return qe_invalid;
		    }
	    }    
            if(!p_getc(&(tempguy.width),f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&(tempguy.height),f))
            {
                return qe_invalid;
            }
            
	    if ( guyversion >= 36 ) //expanded tiles
	    {
		    if(!p_igetl(&(tempguy.s_tile),f))
		    {
			return qe_invalid;
		    }
	    }
	    else
	    {
		    if(!p_igetw(&(tempguy.s_tile),f))
		    {
			return qe_invalid;
		    }
	    }
	    
            if(!p_getc(&(tempguy.s_width),f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&(tempguy.s_height),f))
            {
                return qe_invalid;
            }
            
	    if ( guyversion >= 36 ) //expanded tiles
	    {
		    if(!p_igetl(&(tempguy.e_tile),f))
		    {
			return qe_invalid;
		    }
	    }
	    else
	    {
		    if(!p_igetw(&(tempguy.e_tile),f))
		    {
			return qe_invalid;
		    }
	    }
	    
            if(!p_getc(&(tempguy.e_width),f))
            {
                return qe_invalid;
            }
            
            if(!p_getc(&(tempguy.e_height),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.hp),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.family),f))
            {
                return qe_invalid;
            }
            
            if(guyversion < 9 && (i==eDKNUT1 || i==eDKNUT2 || i==eDKNUT3 || i==eDKNUT4 || i==eDKNUT5)) // Whoops, forgot about Darknuts...
            {
                if(get_qr(qr_NEWENEMYTILES))
                {
                    tempguy.s_tile=tempguy.e_tile+120;
                    tempguy.s_width=tempguy.e_width;
                    tempguy.s_height=tempguy.e_height;
                }
                else tempguy.s_tile=860;
            }
            
            if(!p_igetw(&(tempguy.cset),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.anim),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.e_anim),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.frate),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.e_frate),f))
            {
                return qe_invalid;
            }
            
            if(guyversion < 13)  // April 2009
            {
                if(get_bit(deprecated_rules, qr_SLOWENEMYANIM_DEP))
                {
                    tempguy.frate *= 2;
                    tempguy.e_frate *= 2;
                }
            }
            
            if(guyversion < 14)  // May 1 2009
            {
                if(tempguy.anim==a2FRMSLOW)
                {
                    tempguy.anim=a2FRM;
                    tempguy.frate *= 2;
                }
                
                if(tempguy.e_anim==a2FRMSLOW)
                {
                    tempguy.e_anim=a2FRM;
                    tempguy.e_frate *= 2;
                }
                
                if(tempguy.anim==aFLIPSLOW)
                {
                    tempguy.anim=aFLIP;
                    tempguy.frate *= 2;
                }
                
                if(tempguy.e_anim==aFLIPSLOW)
                {
                    tempguy.e_anim=aFLIP;
                    tempguy.e_frate *= 2;
                }
                
                if(tempguy.anim == aNEWDWALK) tempguy.anim = a4FRM4DIR;
                
                if(tempguy.e_anim == aNEWDWALK) tempguy.e_anim = a4FRM4DIR;
                
                if(tempguy.anim == aNEWPOLV || tempguy.anim == a4FRM3TRAP)
                {
                    tempguy.anim=a4FRM4DIR;
                    tempguy.s_tile=(get_qr(qr_NEWENEMYTILES) ? tempguy.e_tile : tempguy.tile)+20;
                }
                
                if(tempguy.e_anim == aNEWPOLV || tempguy.e_anim == a4FRM3TRAP)
                {
                    tempguy.e_anim=a4FRM4DIR;
                    tempguy.s_tile=(get_qr(qr_NEWENEMYTILES) ? tempguy.e_tile : tempguy.tile)+20;
                }
            }
            
            if(!p_igetw(&(tempguy.dp),f))
            {
                return qe_invalid;
            }
            
            //correction for guy fire
            if(guyversion < 6)
            {
                if(i == gFIRE)
                    tempguy.dp = 2;
            }
            
            if(!p_igetw(&(tempguy.wdp),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.weapon),f))
            {
                return qe_invalid;
            }
            
            //correction for bosses using triple, "rising" fireballs
            if(guyversion < 5)
            {
                if(i == eLAQUAM || i == eRAQUAM || i == eGOHMA1 || i == eGOHMA2 ||
                        i == eGOHMA3 || i == eGOHMA4)
                {
                    if(tempguy.weapon == ewFireball)
                        tempguy.weapon = ewFireball2;
                }
            }
            
            if(!p_igetw(&(tempguy.rate),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.hrate),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.step),f))
            {
                return qe_invalid;
            }
            
            // HIGHLY UNORTHODOX UPDATING THING, part 2
            if(fixpolsvoice && tempguy.family==eePOLSV)
            {
                tempguy.step /= 2;
            }
            
            if(!p_igetw(&(tempguy.homing),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.grumble),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.item_set),f))
            {
                return qe_invalid;
            }
            
            if(guyversion>=22) // Version 22: Expand misc attributes to 32 bits
            {
				for (int q = 0; q < 10; ++q)
				{
					if (!p_igetl(&(tempguy.attributes[q]), f))
					{
						return qe_invalid;
					}
				}
            }
            else
            {
                int16_t tempMisc;
                
				for(int q=0;q<10;q++)
				{
					if (!p_igetw(&tempMisc, f))
					{
						return qe_invalid;
					}
					tempguy.attributes[q] = tempMisc;
				}
                
                if(guyversion < 13)  // April 2009 - a tiny Wizzrobe update
                {
                    if(tempguy.family == eeWIZZ && !(tempguy.attributes[0]))
                        tempguy.attributes[4] = 74;
                }

            }
            
            if(!p_igetw(&(tempguy.bgsfx),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.bosspal),f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&(tempguy.extend),f))
            {
                return qe_invalid;
            }
            
	    //! Enemy Defences
	    
	    //If a 2.50 quest, use only the 2.5 defences. 
            if(guyversion >= 16 )  // November 2009 - Super Enemy Editor
            {
                for(int32_t j=0; j<edefLAST; j++)
                {
                    if(!p_getc(&(tempguy.defense[j]),f))
                    {
                        return qe_invalid;
                    }
                }
		//then copy the generic script defence to all the new script defences
		
            }
	    
	    
	    
            
            if(guyversion >= 18)
            {
                if(!p_getc(&(tempguy.hitsfx),f))
                {
                    return qe_invalid;
                }
                
                if(!p_getc(&(tempguy.deadsfx),f))
                {
                    return qe_invalid;
                }
            }
            
            if(guyversion >= 22)
            {
                if(!p_igetl(&(tempguy.attributes[10]), f))
                {
                    return qe_invalid;
                }
                
                if(!p_igetl(&(tempguy.attributes[11]),f))
                {
                    return qe_invalid;
                }
            }
            else if(guyversion >= 19)
            {
                int16_t tempMisc;
                
                if(!p_igetw(&tempMisc,f))
                {
                    return qe_invalid;
                }
                
                tempguy.attributes[10] = tempMisc;
                
                if(!p_igetw(&tempMisc,f))
                {
                    return qe_invalid;
                }
                
                tempguy.attributes[11] = tempMisc;
            }
	    
	    //If a 2.54 or later quest, use all of the defences. 
	    if(guyversion > 24) // Add new guyversion conditional statement 
            {
		for(int32_t j=edefLAST; j<edefLAST255; j++)
                {
                    if(!p_getc(&(tempguy.defense[j]),f))
                    {
                        return qe_invalid;
                    }
                }
            }
	    
	    if(guyversion <= 24) // Port over generic script settings from old quests in the new editor. 
            {
		for(int32_t j=edefSCRIPT01; j<=edefSCRIPT10; j++)
                {
                    tempguy.defense[j] = tempguy.defense[edefSCRIPT] ;
                }
            }
	    
	    //tilewidth, tileheight, hitwidth, hitheight, hitzheight, hitxofs, hityofs, hitzofs
	    if(guyversion > 25)
	    {
		    if(!p_igetl(&(tempguy.txsz),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.tysz),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.hxsz),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.hysz),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.hzsz),f))
                    {
                        return qe_invalid;
                    }
		    /* Is it safe to read a fixed with getl, or do I need to typecast it? -Z
		   
		    */
	    }
	    //More Enemy Editor vars for 2.60
	    if(guyversion > 26)
	    {
		    if(!p_igetl(&(tempguy.hxofs),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.hyofs),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.xofs),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.yofs),f))
                    {
                        return qe_invalid;
                    }
		    if(!p_igetl(&(tempguy.zofs),f))
                    {
                        return qe_invalid;
                    }
	    }
	    
	    if(guyversion <= 27) // Port over generic script settings from old quests in the new editor. 
            {
		tempguy.wpnsprite = 0;
            }
	    
	    if(guyversion > 27)
	    {
	        if(!p_igetl(&(tempguy.wpnsprite),f))
                    {
                        return qe_invalid;
                    }
	    }
	    if(guyversion <= 28) // Port over generic script settings from old quests in the new editor. 
            {
		tempguy.SIZEflags = 0;
            }
	    if(guyversion > 28)
	    {
		if(!p_igetl(&(tempguy.SIZEflags),f))
		    {
			return qe_invalid;
		    }
		
	    }
	    if(guyversion < 30) // Port over generic script settings from old quests in the new editor. 
            {
		tempguy.frozentile = 0;
		tempguy.frozencset = 0;
		tempguy.frozenclock = 0;
		for ( int32_t q = 0; q < 10; q++ ) tempguy.frozenmisc[q] = 0;
            }
	    if(guyversion >= 30)
	    {
		if(!p_igetl(&(tempguy.frozentile),f))
		{
			return qe_invalid;
		}  
		if(!p_igetl(&(tempguy.frozencset),f))
		{
			return qe_invalid;
		}  
		if(!p_igetl(&(tempguy.frozenclock),f))
		{
			return qe_invalid;
		}  
		for ( int32_t q = 0; q < 10; q++ ) {
			if(!p_igetw(&(tempguy.frozenmisc[q]),f))
			{
				return qe_invalid;
			}
		}
		
	    }
	    
	    if(guyversion >= 34)
	    {
		if(!p_igetw(&(tempguy.firesfx),f))
		{
			return qe_invalid;
		}  
		for(int q=15;q<32;++q)
		{
			if(!p_igetl(&(tempguy.attributes[q]),f))
			{
				return qe_invalid;
			} 
		}
		
		for ( int32_t q = 0; q < 32; q++ ) {
			if(!p_igetl(&(tempguy.movement[q]),f))
			{
				return qe_invalid;
			}
		}
		for ( int32_t q = 0; q < 32; q++ ) {
			if(!p_igetl(&(tempguy.new_weapon[q]),f))
			{
				return qe_invalid;
			}
		}
		if(!p_igetw(&(tempguy.script),f))
		{
			return qe_invalid;
		} 
		for ( int32_t q = 0; q < 8; q++ )
		{
			if(!p_igetl(&(tempguy.initD[q]),f))
			{
				return qe_invalid;
			} 			
		}
		for ( int32_t q = 0; q < 2; q++ )
		{
			int32_t temp;
			if(!p_igetl(&temp,f))
			{
				return qe_invalid;
			} 			
		}
		
	    }
	    
	    if(guyversion >= 37)
	    {
		if(!p_igetl(&(tempguy.editorflags),f))
		{
			return qe_invalid;
		}     
	    }
	    if ( guyversion < 37 ) { tempguy.editorflags = 0; }
	    if(guyversion >= 38)
	    {
		if(!p_igetl(&(tempguy.attributes[12]),f))
		{
			return qe_invalid;
		} 
		if(!p_igetl(&(tempguy.attributes[13]),f))
		{
			return qe_invalid;
		} 
		if(!p_igetl(&(tempguy.attributes[14]),f))
		{
			return qe_invalid;
		}  
		   
	    }
	    if ( guyversion < 38 ) 
	    { 
		tempguy.attributes[12] = 0; 
		tempguy.attributes[13] = 0; 
		tempguy.attributes[14] = 0; 
	    }
	    
	    if ( guyversion >= 39 )
	    {
		for ( int32_t q = 0; q < 8; q++ )
		{
			for ( int32_t w = 0; w < 65; w++ )
			{
				if(!p_getc(&(tempguy.initD_label[q][w]),f))
				{
					return qe_invalid;
				} 
			}
			for ( int32_t w = 0; w < 65; w++ )
			{
				if(!p_getc(&(tempguy.weapon_initD_label[q][w]),f))
				{
					return qe_invalid;
				} 
			}
		}
		    
		    
	    }
	    if ( guyversion < 39 ) //apply old InitD strings to both
	    {
		for ( int32_t q = 0; q < 8; q++ )
		{
			sprintf(tempguy.initD_label[q],"InitD[%d]",q);
			sprintf(tempguy.weapon_initD_label[q],"InitD[%d]",q);
		}
	    }
	    if ( guyversion >= 40 )
	    {
		    if(!p_igetw(&(tempguy.weaponscript),f))
		    {
				return qe_invalid;
		    } 
	    }
	    if ( guyversion < 40 ) 
	    {
		    tempguy.weaponscript = 0;
	    }
	    //eweapon script InitD
	    if ( guyversion >= 41 )
	    {
		    for ( int32_t q = 0; q < 8; q++ )
		    {
			    if(!p_igetl(&(tempguy.weap_initiald[q]),f))
			    {
					return qe_invalid;
			    } 
		    }
		    if ( guy_cversion < 4 )
		    {
			if ( tempguy.family == eeKEESE )
			{

				if ( !tempguy.attributes[0] )
				{
					tempguy.attributes[15] = 120;
					tempguy.attributes[16] = 16;
					
				}
			}
			if ( tempguy.family == eePEAHAT )
			{	
				tempguy.attributes[15] = 80;
				tempguy.attributes[16] = 16;
			}

			if ( tempguy.family == eeGHINI )
			{	
				tempguy.attributes[15] = 120;
				tempguy.attributes[16] = 10;
			}			
			    
		    }
	    }
	    
	    
	    
	    //default weapon sprites (quest version < 2.54)
	    //port over old defaults -Z
	    if(guyversion < 32)
	    {
		if ( tempguy.wpnsprite <= 0 )
		{
			switch(tempguy.weapon)
			{
				case wNone:
					tempguy.wpnsprite = 0; break;
				
				case wSword:
				case wBeam:
				case wBrang:
				case wBomb:
				case wSBomb:
				case wLitBomb:
				case wLitSBomb:
				case wArrow:
				case wFire:
				case wWhistle:
				case wBait:
				case wWand:
				case wMagic:
				case wCatching:
				case wWind:
				case wRefMagic:
				case wRefFireball:
				case wRefRock:
				case wHammer:
				case wHookshot:
				case wHSHandle:
				case wHSChain:
				case wSSparkle:
				case wFSparkle:
				case wSmack:
				case wPhantom:
				case wCByrna:
				case wRefBeam:
				case wStomp:
				case lwMax:
				case wScript1:
				case wScript2:
				case wScript3:
				case wScript4:
				case wScript5:
				case wScript6:
				case wScript7:
				case wScript8:
				case wScript9:
				case wScript10:
				case wIce:
					//Cannot use any of these weapons yet. 
					tempguy.wpnsprite = -1;
					break;
				
				case wEnemyWeapons:
				case ewFireball: tempguy.wpnsprite = 17; break;
				
				case ewArrow: tempguy.wpnsprite = 19; break;
				case ewBrang: tempguy.wpnsprite = 4; break;
				case ewSword: tempguy.wpnsprite = 20; break;
				case ewRock: tempguy.wpnsprite = 18; break;
				case ewMagic: tempguy.wpnsprite = 21; break;
				case ewBomb: tempguy.wpnsprite = 78; break;
				case ewSBomb: tempguy.wpnsprite = 79; break;
				case ewLitBomb: tempguy.wpnsprite = 76; break;
				case ewLitSBomb: tempguy.wpnsprite = 77; break;
				case ewFireTrail: tempguy.wpnsprite = 80; break;
				case ewFlame: tempguy.wpnsprite = 35; break;
				case ewWind: tempguy.wpnsprite = 36; break;
				case ewFlame2: tempguy.wpnsprite = 81; break;
				case ewFlame2Trail: tempguy.wpnsprite = 82; break;
				case ewIce: tempguy.wpnsprite = 83; break;
				case ewFireball2: tempguy.wpnsprite = 17; break; //fireball (rising)
				
					
				default: break; //No assign.
			}
		}
	    }
	    
	    //default weapon fire sound (quest version < 2.54)
	    //port over old defaults and zero new data. -Z
	    if(guyversion < 34)
	    {
		for ( int32_t q = 0; q < 32; q++ )
		{
			tempguy.movement[q] = 0;
			tempguy.new_weapon[q] = 0;
			
		}
		
		//NPC Script attributes.
		tempguy.script = 0; //No scripted enemies existed. -Z
		for ( int32_t q = 0; q < 8; q++ ) tempguy.initD[q] = 0; //Script Data
		
		for ( int32_t q = 15; q < 32; q++) tempguy.attributes[q] = 0; //misc 16-32

		//old default sounds
		if ( tempguy.firesfx <= 0 )
		{
			switch(tempguy.weapon)
			{
				case wNone:
					tempguy.firesfx = 0; break;
				
				case wSword:
				case wBeam:
				case wBrang:
				case wBomb:
				case wSBomb:
				case wLitBomb:
				case wLitSBomb:
				case wArrow:
				case wFire:
				case wWhistle:
				case wBait:
				case wWand:
				case wMagic:
				case wCatching:
				case wWind:
				case wRefMagic:
				case wRefFireball:
				case wRefRock:
				case wHammer:
				case wHookshot:
				case wHSHandle:
				case wHSChain:
				case wSSparkle:
				case wFSparkle:
				case wSmack:
				case wPhantom:
				case wCByrna:
				case wRefBeam:
				case wStomp:
				case lwMax:
				case wScript1:
				case wScript2:
				case wScript3:
				case wScript4:
				case wScript5:
				case wScript6:
				case wScript7:
				case wScript8:
				case wScript9:
				case wScript10:
				case wIce:
					//Cannot use any of these weapons yet. 
					tempguy.firesfx = -1;
					break;
				
				case wEnemyWeapons:
				case ewFireball: tempguy.firesfx = 40; break;
				
				case ewArrow: tempguy.firesfx = 1; break; //Ghost.zh has 0?
				case ewBrang: tempguy.firesfx = 4; break; //Ghost.zh has 0?
				case ewSword: tempguy.firesfx = 20; break; //Ghost.zh has 0?
				case ewRock: tempguy.firesfx = 51; break;
				case ewMagic: tempguy.firesfx = 32; break;
				case ewBomb: tempguy.firesfx = 3; break; //Ghost.zh has 0?
				case ewSBomb: tempguy.firesfx = 3; break; //Ghost.zh has 0?
				case ewLitBomb: tempguy.firesfx = 21; break; //Ghost.zh has 0?
				case ewLitSBomb: tempguy.firesfx = 21; break; //Ghost.zh has 0?
				case ewFireTrail: tempguy.firesfx = 13; break;
				case ewFlame: tempguy.firesfx = 13; break;
				case ewWind: tempguy.firesfx = 32; break;
				case ewFlame2: tempguy.firesfx = 13; break;
				case ewFlame2Trail: tempguy.firesfx = 13; break;
				case ewIce: tempguy.firesfx = 44; break;
				case ewFireball2: tempguy.firesfx = 40; break; //fireball (rising)
				
				//what about special attacks (e.g. summoning == 56)
				default: break; //No assign.
			}
		}
	    }
	    
	    //Port hardcoded hit sound to the enemy hitsfx defaults for older quests. 
	    if(Header->zelda_version <= 0x250 || ( Header->zelda_version > 0x250 && guyversion < 35 ))
	    {
		    if ( tempguy.hitsfx == 0 ) tempguy.hitsfx = 11;
	    }
	    //Keese and bat halt rates.
	    if ( guyversion < 42 && guy_cversion < 4  ) 
	    {
		    
			if ( tempguy.family == eeKEESE )
			{

				if ( !tempguy.attributes[0] )
				{
					tempguy.attributes[15] = 120;
					tempguy.attributes[16] = 16;
					
				}
			}
			if ( tempguy.family == eePEAHAT )
			{	
				tempguy.attributes[15] = 80;
				tempguy.attributes[16] = 16;
			}
			if ( tempguy.family == eeGHINI )
			{	
				tempguy.attributes[15] = 120;
				tempguy.attributes[16] = 10;
			}			
			    
		    
	    }
		
	    
            //miscellaneous other corrections
            //fix the mirror wizzrobe -DD
            if(guyversion < 7)
            {
                if(i == eMWIZ)
                {
                    tempguy.attributes[1] = 0;
                    tempguy.attributes[3] = 1;
                }
            }
            
            if(guyversion < 8)
            {
                if(i == eGLEEOK1 || i == eGLEEOK2 || i == eGLEEOK3 || i == eGLEEOK4 || i == eGLEEOK1F || i == eGLEEOK2F || i == eGLEEOK3F || i == eGLEEOK4F)
                {
                    // Some of these are deliberately different to NewDefault/defdata.cpp, by the way. -L
                    tempguy.attributes[4] = 4; //neck length in segments
                    tempguy.attributes[5] = 8; //neck offset from first body tile
                    tempguy.attributes[6] = 40; //offset for each subsequent neck tile from the first neck tile
                    tempguy.attributes[7] = 168; //head offset from first body tile
                    tempguy.attributes[8] = 228; //flying head offset from first body tile
                    
                    if(i == eGLEEOK1F || i == eGLEEOK2F || i == eGLEEOK3F || i == eGLEEOK4F)
                    {
                        tempguy.attributes[5] += 10; //neck offset from first body tile
                        tempguy.attributes[7] -= 12; //head offset from first body tile
                    }
                }
            }
            
            if(guyversion < 10) // December 2007 - Dodongo CSet fix
            {
                if(get_bit(deprecated_rules,46) && tempguy.family==eeDONGO && tempguy.attributes[0]==0)
                    tempguy.bosspal = spDIG;
            }
            
            if(guyversion < 11) // December 2007 - Spinning Tile fix
            {
                if(tempguy.family==eeSPINTILE)
                {
                    tempguy.flags |= guy_superman;
                    tempguy.item_set = 0; // Don't drop items
                    tempguy.step = 300;
                }
            }
            
            if(guyversion < 12) // October 2008 - Flashing Bubble, Rope 2, and Ghini 2 fix
            {
                if(get_bit(deprecated_rules, qr_NOROPE2FLASH_DEP))
                {
                    if(tempguy.family==eeROPE)
                    {
                        tempguy.flags &= ~guy_flashing;
                    }
                }
                
                if(get_bit(deprecated_rules, qr_NOBUBBLEFLASH_DEP))
                {
                    if(tempguy.family==eeBUBBLE)
                    {
                        tempguy.flags &= ~guy_flashing;
                    }
                }
                
                if((tempguy.family==eeGHINI)&&(tempguy.attributes[0]))
                {
                    if(get_bit(deprecated_rules, qr_GHINI2BLINK_DEP))
                    {
                        tempguy.flags |= guy_blinking;
                    }
                    
                    if(get_bit(deprecated_rules, qr_PHANTOMGHINI2_DEP))
                    {
                        tempguy.flags |= guy_transparent;
                    }
                }
            }
            
            if(guyversion < 15) // July 2009 - Guy Fire and Fairy fix
            {
                if(i==gFIRE)
                {
                    tempguy.e_anim = aFLIP;
                    tempguy.e_frate = 24;
                }
                
                if(i==gFAIRY)
                {
                    tempguy.e_anim = a2FRM;
                    tempguy.e_frate = 16;
                }
            }
            
            if(guyversion < 16)  // November 2009 - Super Enemy Editor part 1
            {
                if(i==0) Z_message("Updating guys to version 16...\n");
                
                update_guy_1(&tempguy);
                
                if(i==eMPOLSV)
                {
                    tempguy.defense[edefARROW] = edCHINK;
                    tempguy.defense[edefMAGIC] = ed1HKO;
                    tempguy.defense[edefREFMAGIC] = ed1HKO;
                }
            }
            
            if(guyversion < 17)  // December 2009
            {
                if(tempguy.family==eePROJECTILE)
                {
                    tempguy.attributes[0] = 0;
                }
            }
            
            if(guyversion < 18)  // January 2010
            {
                bool boss = (tempguy.family == eeAQUA || tempguy.family==eeDONGO || tempguy.family == eeMANHAN || tempguy.family == eeGHOMA || tempguy.family==eeDIG
                             || tempguy.family == eeGLEEOK || tempguy.family==eePATRA || tempguy.family == eeGANON || tempguy.family==eeMOLD);
                             
                tempguy.hitsfx = (boss && tempguy.family != eeMOLD && tempguy.family != eeDONGO && tempguy.family != eeDIG) ? WAV_GASP : 0;
                tempguy.deadsfx = (boss && (tempguy.family != eeDIG || tempguy.attributes[9] == 0)) ? WAV_GASP : WAV_EDEAD;
                
                if(tempguy.family == eeAQUA)
                    for(int32_t j=0; j<edefLAST; j++) tempguy.defense[j] = default_guys[eRAQUAM].defense[j];
                else if(tempguy.family == eeMANHAN)
                    for(int32_t j=0; j<edefLAST; j++) tempguy.defense[j] = default_guys[eMANHAN].defense[j];
                else if(tempguy.family==eePATRA)
                    for(int32_t j=0; j<edefLAST; j++) tempguy.defense[j] = default_guys[eGLEEOK1].defense[j];
                else if(tempguy.family==eeGHOMA)
                {
                    for(int32_t j=0; j<edefLAST; j++)
                        tempguy.defense[j] = default_guys[eGOHMA1].defense[j];
                        
                    tempguy.defense[edefARROW] = ((tempguy.attributes[0]==3) ? edCHINKL8 : (tempguy.attributes[0]==2) ? edCHINKL4 : 0);
                    
                    if(tempguy.attributes[0]==3 && !tempguy.weapon) tempguy.weapon = ewFlame;
                    
                    tempguy.attributes[0]--;
                }
                else if(tempguy.family == eeGLEEOK)
                {
                    for(int32_t j=0; j<edefLAST; j++)
                        tempguy.defense[j] = default_guys[eGLEEOK1].defense[j];
                        
                    if(tempguy.attributes[2]==1 && !tempguy.weapon) tempguy.weapon = ewFlame;
                }
                else if(tempguy.family == eeARMOS)
                {
                    tempguy.family=eeWALK;
                    tempguy.hrate = 0;
                    tempguy.attributes[9] = tempguy.attributes[0];
                    tempguy.attributes[0] = tempguy.attributes[1] = tempguy.attributes[2] = tempguy.attributes[3] = tempguy.attributes[4] = tempguy.attributes[5] = tempguy.attributes[6] = tempguy.attributes[7] = 0;
                    tempguy.attributes[0] = tempguy.attributes[1] = tempguy.attributes[2] = tempguy.attributes[3] = tempguy.attributes[4] = tempguy.attributes[5] = tempguy.attributes[6] = tempguy.attributes[7] = 0;
                    tempguy.attributes[8] = e9tARMOS;
                }
                else if(tempguy.family == eeGHINI && !tempguy.attributes[0])
                {
                    tempguy.family=eeWALK;
                    tempguy.hrate = 0;
                    tempguy.attributes[0] = tempguy.attributes[1] = tempguy.attributes[2] = tempguy.attributes[3] = tempguy.attributes[4] = tempguy.attributes[5] =
                                                        tempguy.attributes[6] = tempguy.attributes[7] = tempguy.attributes[8] = tempguy.attributes[9] = 0;
                }
                
                // Spawn animation flags
                if(tempguy.family == eeWALK && (tempguy.flags&guy_armos || tempguy.flags&guy_ghini))
                    tempguy.flags |= guy_fade_flicker;
                else
                    tempguy.flags &= (guy_flags)0x0F00000F; // Get rid of the unused flags!
            }
            
            if(guyversion < 20)  // April 2010
            {
                if(tempguy.family == eeTRAP)
                {
                    tempguy.attributes[1] = tempguy.attributes[9];
                    
                    if(tempguy.attributes[9]>=1)
                    {
                        tempguy.attributes[0]++;
                    }
                    
                    tempguy.attributes[9] = 0;
                }
                
                // Bomb Blast fix
                if(tempguy.weapon==ewBomb && tempguy.family!=eeROPE && (tempguy.family!=eeWALK || tempguy.attributes[1] != e2tBOMBCHU))
                    tempguy.weapon = ewLitBomb;
                else if(tempguy.weapon==ewSBomb && tempguy.family!=eeROPE && (tempguy.family!=eeWALK || tempguy.attributes[1] != e2tBOMBCHU))
                    tempguy.weapon = ewLitSBomb;
            }
            
            if(guyversion < 21)  // September 2011
            {
                if(tempguy.family == eeKEESE || tempguy.family == eeKEESETRIB)
                {
                    if(tempguy.family == eeKEESETRIB)
                    {
                        tempguy.family = eeKEESE;
                        tempguy.attributes[1] = e2tKEESETRIB;
                        tempguy.attributes[0] = 0;
                    }
                    
                    tempguy.rate = 2;
                    tempguy.hrate = 8;
                    tempguy.homing = 0;
                    tempguy.step= (tempguy.family == eeKEESE && tempguy.attributes[0] ? 100:62);
                }
                else if(tempguy.family == eePEAHAT || tempguy.family==eePATRA)
                {
                    if(tempguy.family == eePEAHAT)
                    {
                        tempguy.rate = 4;
                        tempguy.step = 62;
                    }
                    else
                        tempguy.step = 25;
                        
                    tempguy.hrate = 8;
                    tempguy.homing = 0;
                }
                else if(tempguy.family == eeDIG || tempguy.family == eeMANHAN)
                {
                    if(tempguy.family == eeMANHAN)
                        tempguy.step=50;
                        
                    tempguy.hrate = 16;
                    tempguy.homing = 0;
                }
                else if(tempguy.family == eeGLEEOK)
                {
                    tempguy.rate = 2;
                    tempguy.homing = 0;
                    tempguy.hrate = 9;
                    tempguy.step=89;
                }
                else if(tempguy.family == eeGHINI)
                {
                    tempguy.rate = 4;
                    tempguy.hrate = 12;
                    tempguy.step=62;
                    tempguy.homing = 0;
                }
                
                // Bigdig random rate fix
                if(tempguy.family==eeDIG && tempguy.attributes[9]==1)
                {
                    tempguy.rate = 2;
                }
            }
            
            if(guyversion < 24) // November 2012
            {
                if(tempguy.family==eeLANM)
                    tempguy.attributes[2] = 1;
                else if(tempguy.family==eeMOLD)
                    tempguy.attributes[1] = 0;
            }
	    
	    if(guyversion < 33) //Whistle defence did not exist before this version of 2.54. -Z
	    {
		if(tempguy.family!=eeDIG)
		{
			tempguy.defense[edefWhistle] = edIGNORE; //Might need to be ignore, universally. 
		}
			
	    }
	    // does not seem to solve the issue!
	    if ( Header->zelda_version <= 0x210 ) 
	    {
		al_trace("Detected version %d for dodongo patch.\n",Header->zelda_version);
		if ( tempguy.family == eeDONGO ) 
		{
			tempguy.deadsfx = 15; //In 2.10 and earlier, Dodongos used this as their death sound.
		}
	    }
            
			if(guyversion >= 42)
			{
				if(guyversion >= 47)
				{
					if(!p_igetl(&(tempguy.moveflags),f))
					{
						return qe_invalid;
					}
				}
				else
				{
					byte fl;
					if(!p_getc(&fl,f))
					{
						return qe_invalid;
					}
					tempguy.moveflags = (move_flags)fl;
				}
			}
			else
			{
				switch(tempguy.family)
				{
					//No gravity; floats over pits
					case eeTEK: case eePEAHAT: case eeROCK: case eeTRAP:
					case eePROJECTILE: case eeSPINTILE: case eeKEESE: case eeFIRE:
					//Special (bosses, etc)
					case eeFAIRY: case eeGUY: case eeNONE: case eeZORA:
					case eeAQUA: case eeDIG: case eeGHOMA: case eeGANON:
					case eePATRA: case eeGLEEOK: case eeMOLD: case eeMANHAN:
						tempguy.moveflags = move_can_pitwalk;
						break;
					//No gravity, but falls in pits
					case eeLEV:
						tempguy.moveflags = move_can_pitfall;
						break;
					//Bosses that respect pits
					case eeDONGO:
						tempguy.moveflags = move_obeys_grav;
						break;
					case eeLANM:
						tempguy.moveflags = move_none;
						break;
					//Gravity, floats over pits
					case eeWIZZ: case eeWALLM: case eeGHINI:
						tempguy.moveflags = move_obeys_grav | move_can_pitwalk;
						break;
					//Gravity and falls in pits
					case eeWALK:
						if (tempguy.attributes[8]==e9tPOLSVOICE||tempguy.attributes[8]==e9tVIRE)
							break;
						[[fallthrough]];
					case eeOTHER:
					case eeSCRIPT01: case eeSCRIPT02: case eeSCRIPT03: case eeSCRIPT04: case eeSCRIPT05:
					case eeSCRIPT06: case eeSCRIPT07: case eeSCRIPT08: case eeSCRIPT09: case eeSCRIPT10:
					case eeSCRIPT11: case eeSCRIPT12: case eeSCRIPT13: case eeSCRIPT14: case eeSCRIPT15:
					case eeSCRIPT16: case eeSCRIPT17: case eeSCRIPT18: case eeSCRIPT19: case eeSCRIPT20:
					case eeFFRIENDLY01: case eeFFRIENDLY02: case eeFFRIENDLY03: case eeFFRIENDLY04: case eeFFRIENDLY05:
					case eeFFRIENDLY06: case eeFFRIENDLY07: case eeFFRIENDLY08: case eeFFRIENDLY09: case eeFFRIENDLY10:
						tempguy.moveflags = move_obeys_grav | move_can_pitfall;
				}
			}
			if(guyversion < 43)
			{
				switch(tempguy.family)
				{
					//No gravity; floats over pits
					case eeTEK: case eePEAHAT: case eeROCK: case eeTRAP:
					case eePROJECTILE: case eeSPINTILE: case eeKEESE: case eeFIRE:
					//Special (bosses, etc)
					case eeFAIRY: case eeGUY: case eeNONE: case eeZORA:
					case eeAQUA: case eeDIG: case eeGHOMA: case eeGANON:
					case eePATRA: case eeGLEEOK: case eeMOLD: case eeMANHAN:
					case eeWIZZ: case eeWALLM: case eeGHINI:
					//Gravity, floats over pits
						tempguy.moveflags |= move_can_waterwalk;
						tempguy.moveflags |= move_can_pitwalk;
						break;
				}
			}
			if (guyversion < 44)
			{
				if ( tempguy.family == eeGHOMA )
				{
					tempguy.flags |= guy_fade_instant;
				}
			}
			if (guyversion > 44)
			{
				if(!p_getc(&(tempguy.spr_shadow),f))
				{
					return qe_invalid;
				}
				if(!p_getc(&(tempguy.spr_death),f))
				{
					return qe_invalid;
				}
				if(!p_getc(&(tempguy.spr_spawn),f))
				{
					return qe_invalid;
				}
			}
			else
			{
				tempguy.spr_shadow = (tempguy.family==eeROCK && tempguy.attributes[9]==1) ? iwLargeShadow : iwShadow;
				tempguy.spr_death = iwDeath;
				tempguy.spr_spawn = iwSpawn;
			}
			
			if(guyversion < 46)
			{
				if(tempguy.family == eeWALK && tempguy.attributes[8] == e9tPOLSVOICE)
				{
					tempguy.moveflags |= move_can_waterwalk;
				}
			}

			if (guyversion < 47)
			{
				if (tempguy.family == eeDIG && tempguy.attributes[9]!=1)
				{
					tempguy.flags |= guy_ignore_kill_all;
				}
			}

			if (guyversion < 48)
			{
				if (tempguy.family == eeWALK && (tempguy.attributes[6]==e7tPERMJINX || tempguy.attributes[6]==e7tTEMPJINX || tempguy.attributes[6]==e7tUNJINX)) //BUBBLE CHECK
				{
					switch (tempguy.attributes[7]) {
						case 0: //Sword
							tempguy.attributes[7] = e8tSWORD;
							break;
						case 1:	//Item
							tempguy.attributes[7] = e8tITEM;
							break;
						case 2: //Both
							tempguy.attributes[7] = e8tSWORD|e8tITEM;
							break;
						default: //this can actually happen since Misc8 can be set to any number.
							tempguy.attributes[7] = 0;
							break;
					}
				}
			}

			//these could possible be combined but rather be safe...
			if (guyversion < 51) //reimport the firesfx, zoria ducked up.
			{
				guy_update_firesfx(tempguy);
			}
			if (guyversion < 52)
			{
				guy_update_weaponflags(tempguy);
			}
			else
			{
				if (!p_getc(&(tempguy.wunblockable), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.wmoveflags), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weapoverrideFLAGS), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_tilew), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_tileh), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_hxsz), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_hysz), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_hzsz), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_hxofs), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_hyofs), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_xofs), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.weap_yofs), f))
					return qe_invalid;
				if (!p_igetl(&(tempguy.wstep), f))
					return qe_invalid;
				for (int32_t q = 0; q < WPNSPR_MAX; ++q)
				{
					if (!p_igetw(&(tempguy.burnsprs[q]), f))
						return qe_invalid;
					if (!p_igetw(&(tempguy.light_rads[q]), f))
						return qe_invalid;
				}
			}
			if (guyversion < 53)
			{
				guy_update_weaponspecialsfx(tempguy);
			}
			else
			{
				if (!p_getc(&(tempguy.specialsfx), f))
					return qe_invalid;
			}

			if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
			{
				tempguy.script = 0;
				for(int q = 0; q < 8; ++q)
					tempguy.initD[q] = 0;
			}
			guysbuf[i] = tempguy;
        }
    }
    
    return 0;
}

void update_guy_1(guydata *tempguy) // November 2009
{
    bool doesntcount = false;
    tempguy->flags &= ~guy_weak_arrow; // Formerly 'weak to arrow' which wasn't implemented
    
    switch(tempguy->family)
    {
    case 1: //eeWALK
        switch(tempguy->attributes[9])
        {
        case 0: //Stalfos
            if(tempguy->attributes[0]==1)  // Fires four projectiles at once
                tempguy->attributes[0]=4;
                
            break;
            
        case 1: //Darknut
            goto darknuts;
            break;
        }
        
        tempguy->attributes[9] = 0;
        break;
        
    case 2: //eeSHOOT
        tempguy->family = eeWALK;
        
        switch(tempguy->attributes[9])
        {
        case 0: //Octorok
            if(tempguy->attributes[0]==1||tempguy->attributes[0]==2)
            {
                tempguy->attributes[0]=e1tFIREOCTO;
                tempguy->attributes[1]=e2tFIREOCTO;
            }
            else tempguy->attributes[0] = 0;
            
            tempguy->attributes[5]=tempguy->attributes[3];
            tempguy->attributes[3]=tempguy->attributes[2];
            tempguy->attributes[2]=0;
            break;
            
        case 1: // Moblin
            tempguy->attributes[0] = 0;
            break;
            
        case 2: //Lynel
            tempguy->attributes[5]=tempguy->attributes[0]+1;
            tempguy->attributes[0]=0;
            break;
            
        case 3: //Stalfos 2
            if(tempguy->attributes[0]==1)  // Fires four projectiles at once
                tempguy->attributes[0]=e1t4SHOTS;
            else tempguy->attributes[0] = 0;
            
            break;
            
        case 4: //Darknut 5
darknuts:
            tempguy->defense[edefFIRE] = edIGNORE;
            tempguy->defense[edefBRANG] = edSTUNORCHINK;
            tempguy->defense[edefHOOKSHOT] = 0;
            tempguy->defense[edefARROW] = tempguy->defense[edefBYRNA] = tempguy->defense[edefREFROCK] =
                                              tempguy->defense[edefMAGIC] = tempguy->defense[edefSTOMP] = edCHINK;
                                              
            if(tempguy->attributes[0]==1)
                tempguy->attributes[0]=2;
            else if(tempguy->attributes[0]==2)
            {
                tempguy->attributes[3]=tempguy->attributes[2];
                tempguy->attributes[2]=tempguy->attributes[1];
                tempguy->attributes[1]=e2tSPLIT;
                tempguy->attributes[0] = 0;
            }
            else tempguy->attributes[0] = 0;
            
            tempguy->flags |= guy_shield_front;
            
            if(!get_bit(deprecated_rules,qr_BRKBLSHLDS_DEP))
                tempguy->flags &= ~guy_bkshield;
			else
				tempguy->flags |= guy_bkshield;
                
            break;
        }
        
        tempguy->attributes[9] = 0;
        break;
        
        /*
        		case 9: //eeARMOS
        		tempguy->family = eeWALK;
        		break;
        */
    case 11: //eeGEL
    case 33: //eeGELTRIB
        if(tempguy->family==33)
        {
            tempguy->attributes[3] = 1;
            
            if(get_bit(deprecated_rules, qr_OLDTRIBBLES_DEP))  //Old Tribbles
                tempguy->attributes[2] = tempguy->attributes[1];
                
            tempguy->attributes[1] = e2tTRIBBLE;
        }
        else
        {
            tempguy->attributes[3] = 0;
            tempguy->attributes[2] = 0;
            tempguy->attributes[1] = 0;
        }
        
        tempguy->family = eeWALK;
        
        if(tempguy->attributes[0])
        {
            tempguy->attributes[0]=1;
            tempguy->weapon = ewFireTrail;
        }
        
        break;
        
    case 34: //eeZOLTRIB
    case 12: //eeZOL
        tempguy->attributes[3]=tempguy->attributes[2];
        tempguy->attributes[2]=tempguy->attributes[1];
        tempguy->family = eeWALK;
        tempguy->attributes[1]=e2tSPLITHIT;
        
        if(tempguy->attributes[0])
        {
            tempguy->attributes[0]=1;
            tempguy->weapon = ewFireTrail;
        }
        
        break;
        
    case 13: //eeROPE
        tempguy->family = eeWALK;
        tempguy->attributes[8] = e9tROPE;
        
        if(tempguy->attributes[0])
        {
            tempguy->attributes[3] = tempguy->attributes[2];
            tempguy->attributes[2] = tempguy->attributes[1];
            tempguy->attributes[1] = e2tBOMBCHU;
        }
        
        tempguy->attributes[0] = 0;
        break;
        
    case 14: //eeGORIYA
        tempguy->family = eeWALK;
        
        if(tempguy->attributes[0]!=2) tempguy->attributes[0] = 0;
        
        break;
        
    case 17: //eeBUBBLE
        tempguy->family = eeWALK;
        tempguy->attributes[7] = tempguy->attributes[1];
        tempguy->attributes[6] = tempguy->attributes[0] + 1;
        tempguy->attributes[0] = tempguy->attributes[1] = 0;
        
        //fallthrogh
    case eeTRAP:
    case eeROCK:
        doesntcount = true;
        break;
        
    case 35: //eeVIRETRIB
    case 18: //eeVIRE
        tempguy->family = eeWALK;
        tempguy->attributes[3]=tempguy->attributes[2];
        tempguy->attributes[2]=tempguy->attributes[1];
        tempguy->attributes[1]=e2tSPLITHIT;
        tempguy->attributes[8]=e9tVIRE;
        break;
        
    case 19: //eeLIKE
        tempguy->family = eeWALK;
        tempguy->attributes[6] = e7tEATITEMS;
        tempguy->attributes[7]=95;
        break;
        
    case 20: //eePOLSV
        tempguy->defense[edefBRANG] = edSTUNORCHINK;
        tempguy->defense[edefBOMB] = tempguy->defense[edefSBOMB] = tempguy->defense[edefFIRE] = edIGNORE;
        tempguy->defense[edefMAGIC] = tempguy->defense[edefBYRNA] = edCHINK;
        tempguy->defense[edefARROW] = ed1HKO;
        tempguy->defense[edefHOOKSHOT] = edSTUNONLY;
        tempguy->family = eeWALK;
        tempguy->attributes[8] = e9tPOLSVOICE;
        tempguy->rate = 4;
        tempguy->homing = 32;
        tempguy->hrate = 10;
        tempguy->grumble = 0;
        break;
        
    case eeWIZZ:
        if(tempguy->attributes[3])
        {
            for(int32_t i=0; i < edefLAST; i++)
                tempguy->defense[i] = (i != edefREFBEAM && i != edefREFMAGIC && i != edefQUAKE) ? edIGNORE : 0;
        }
        else
        {
            tempguy->defense[edefBRANG] = edSTUNORCHINK;
            tempguy->defense[edefMAGIC] = edCHINK;
            tempguy->defense[edefHOOKSHOT] = edSTUNONLY;
            tempguy->defense[edefARROW] = tempguy->defense[edefFIRE] =
                                              tempguy->defense[edefWAND] = tempguy->defense[edefBYRNA] = edIGNORE;
        }
        
        break;
        
    case eePEAHAT:
        tempguy->flags &= ~(guy_superman|guy_sbombonly);
        
        if(!(tempguy->flags & guy_bhit))
            tempguy->defense[edefBRANG] = edSTUNONLY;
            
        break;
        
    case eeLEV:
        tempguy->defense[edefSTOMP] = edCHINK;
        break;
    }
    
    // Old flags
    if(tempguy->flags & guy_superman)
    {
        for(int32_t i = 0; i < edefLAST; i++)
            if(!(i==edefSBOMB && (tempguy->flags & guy_sbombonly)))
                tempguy->defense[i] = (i==edefBRANG && tempguy->defense[i] != edIGNORE
                                       && tempguy->family != eeROCK && tempguy->family != eeTRAP
                                       && tempguy->family != eePROJECTILE) ? edSTUNORIGNORE : edIGNORE;
    }
    
    tempguy->flags &= ~(guy_superman|guy_sbombonly);
    
    if(doesntcount)
        tempguy->flags |= (guy_doesnt_count);
}

int32_t readmapscreen_old(PACKFILE *f, zquestheader *Header, mapscr *temp_mapscr, word version, int scrind)
{
	byte tempbyte, padding;
	int32_t extras, secretcombos;
	if(!p_getc(&(temp_mapscr->valid),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->guy),f))
		return qe_invalid;
	temp_mapscr->guytile = -1; //signal to use default guy values
	SETFLAG(temp_mapscr->roomflags,RFL_ALWAYS_GUY,temp_mapscr->guy==gFAIRY);
	SETFLAG(temp_mapscr->roomflags,RFL_GUYFIRES,temp_mapscr->guy!=gFAIRY || !get_qr(qr_NOFAIRYGUYFIRES));
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<146)))
	{
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
		
		temp_mapscr->str=tempbyte;
	}
	else
	{
		if(!p_igetw(&(temp_mapscr->str),f))
		{
			return qe_invalid;
		}
	}
	
	if(!p_getc(&(temp_mapscr->room),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->item),f))
	{
		return qe_invalid;
	}
	
	if(Header->zelda_version < 0x211 || (Header->zelda_version == 0x211 && Header->build < 14))
	{
		temp_mapscr->hasitem = (temp_mapscr->item != 0) ? 1 : 0;
	}
	else
	{
		if(!p_getc(&(temp_mapscr->hasitem),f))
			return qe_invalid;
	}
	
	if((Header->zelda_version < 0x192)||
		((Header->zelda_version == 0x192)&&(Header->build<154)))
	{
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
	}
	
	if(!p_getc(&(temp_mapscr->tilewarptype[0]),f))
	{
		return qe_invalid;
	}
	
	if(Header->zelda_version < 0x193)
	{
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->tilewarptype[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	else
	{
		temp_mapscr->tilewarptype[1]=0;
		temp_mapscr->tilewarptype[2]=0;
		temp_mapscr->tilewarptype[3]=0;
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>153)))
	{
		if(!p_igetw(&(temp_mapscr->door_combo_set),f))
		{
			return qe_invalid;
		}
	}
	
	if(!p_getc(&(temp_mapscr->warpreturnx[0]),f))
	{
		return qe_invalid;
	}
	
	temp_mapscr->warpreturnx[1]=0;
	temp_mapscr->warpreturnx[2]=0;
	temp_mapscr->warpreturnx[3]=0;
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->warpreturnx[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if(!p_getc(&(temp_mapscr->warpreturny[0]),f))
	{
		return qe_invalid;
	}
	
	temp_mapscr->warpreturny[1]=0;
	temp_mapscr->warpreturny[2]=0;
	temp_mapscr->warpreturny[3]=0;
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->warpreturny[i]),f))
			{
				return qe_invalid;
			}
		}
		
		if(version>=18)
		{
			if(!p_igetw(&temp_mapscr->warpreturnc,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			byte temp;
			
			if(!p_getc(&temp,f))
			{
				return qe_invalid;
			}
			
			temp_mapscr->warpreturnc=temp<<8|temp;
		}
	}
	
	if(!p_getc(&(temp_mapscr->stairx),f))
	
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->stairy),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->itemx),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->itemy),f))
	{
		return qe_invalid;
	}
	
	if(version > 15) // February 2009
	{
		if(!p_igetw(&(temp_mapscr->color),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		if(!p_getc(& tempbyte,f))
		{
			return qe_invalid;
		}
		
		temp_mapscr->color = (word) tempbyte;
	}
	
	if(!p_getc(&(temp_mapscr->enemyflags),f))
	{
		return qe_invalid;
	}
	
	for(int32_t k=0; k<4; k++)
	{
		if(!p_getc(&(temp_mapscr->door[k]),f))
		{
			return qe_invalid;
			
		}
	}
	
	if(version <= 11)
	{
		if(!p_getc(&(tempbyte),f))
		{
			return qe_invalid;
		}
		
		temp_mapscr->tilewarpdmap[0]=(word)tempbyte;
		
		if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
		{
			for(int32_t i=1; i<4; i++)
			{
				if(!p_getc(&(tempbyte),f))
				{
					return qe_invalid;
				}
				
				temp_mapscr->tilewarpdmap[i]=(word)tempbyte;
			}
		}
		else
		{
			temp_mapscr->tilewarpdmap[1]=0;
			temp_mapscr->tilewarpdmap[2]=0;
			temp_mapscr->tilewarpdmap[3]=0;
		}
	}
	else
	{
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetw(&(temp_mapscr->tilewarpdmap[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if(!p_getc(&(temp_mapscr->tilewarpscr[0]),f))
	{
		return qe_invalid;
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->tilewarpscr[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	else
	{
		temp_mapscr->tilewarpscr[1]=0;
		temp_mapscr->tilewarpscr[2]=0;
		temp_mapscr->tilewarpscr[3]=0;
	}
	
	if(version >= 15)
	{
		if(!p_getc(&(temp_mapscr->tilewarpoverlayflags),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->tilewarpoverlayflags=0;
	}
	
	if(!p_getc(&(temp_mapscr->exitdir),f))
	{
		return qe_invalid;
	}
	
	if(Header->zelda_version < 0x193)
	{
		if(!p_getc(&tempbyte,f))
		{
			return qe_invalid;
		}
		
	}
	
	if((Header->zelda_version == 0x192)&&(Header->build>145)&&(Header->build<154))
	{
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
	}
	
	for(int32_t k=0; k<10; k++)
	{
		/*
			if (!temp_mapscr->enemy[k])
			{
			  continue;
			}
		*/
		if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<10)))
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			
			temp_mapscr->enemy[k]=tempbyte;
		}
		else
		{
			if(!p_igetw(&(temp_mapscr->enemy[k]),f))
			{
				return qe_invalid;
			}
		}
		
		if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<108)))
		{
			//using enumerations here is dangerous
			//very easy to break old quests -DD
			if(temp_mapscr->enemy[k]>=57)  //old eGOHMA1
			{
				temp_mapscr->enemy[k]+=5;
			}
			else if(temp_mapscr->enemy[k]>=52)  //old eGLEEOK2
			{
				temp_mapscr->enemy[k]+=1;
			}
		}
		
		if(version < 9)
		{
			if(temp_mapscr->enemy[k]>0)
			{
				temp_mapscr->enemy[k]+=10;
			}
		}
		//don't read in any invalid data
		if ( ((unsigned)temp_mapscr->enemy[k]) > MAXGUYS )
		{
			al_trace("Tried to read an invalid enemy ID (%d) for tmpscr->enemy[%d]. This has been cleared to 0.\n", temp_mapscr->enemy[k], k);
			temp_mapscr->enemy[k] = 0;
		}
	}
	
	if(!p_getc(&(temp_mapscr->pattern),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->sidewarptype[0]),f))
	{
		return qe_invalid;
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->sidewarptype[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	else
	{
		temp_mapscr->sidewarptype[1]=0;
		temp_mapscr->sidewarptype[2]=0;
		temp_mapscr->sidewarptype[3]=0;
	}
	
	if(version >= 15)
	{
		if(!p_getc(&(temp_mapscr->sidewarpoverlayflags),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->sidewarpoverlayflags=0;
	}
	
	if(!p_getc(&(temp_mapscr->warparrivalx),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->warparrivaly),f))
	{
		return qe_invalid;
	}
	
	for(int32_t k=0; k<4; k++)
	{
		if(!p_getc(&(temp_mapscr->path[k]),f))
		{
			return qe_invalid;
		}
	}
	
	if(!p_getc(&(temp_mapscr->sidewarpscr[0]),f))
	{
		return qe_invalid;
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		for(int32_t i=1; i<4; i++)
		{
			if(!p_getc(&(temp_mapscr->sidewarpscr[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	else
	{
		temp_mapscr->sidewarpscr[1]=0;
		temp_mapscr->sidewarpscr[2]=0;
		temp_mapscr->sidewarpscr[3]=0;
	}
	
	if(version <= 11)
	{
		if(!p_getc(&(tempbyte),f))
		{
			return qe_invalid;
		}
		
		temp_mapscr->sidewarpdmap[0]=(word)tempbyte;
		
		if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
		{
			for(int32_t i=1; i<4; i++)
			{
				if(!p_getc(&(tempbyte),f))
				{
					return qe_invalid;
				}
				
				temp_mapscr->sidewarpdmap[i]=(word)tempbyte;
			}
		}
		else
		{
			temp_mapscr->sidewarpdmap[1]=0;
			temp_mapscr->sidewarpdmap[2]=0;
			temp_mapscr->sidewarpdmap[3]=0;
		}
	}
	else
	{
		for(int32_t i=0; i<4; i++)
		{
			if(!p_igetw(&(temp_mapscr->sidewarpdmap[i]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		if(!p_getc(&(temp_mapscr->sidewarpindex),f))
		{
			return qe_invalid;
		}
	}
	else temp_mapscr->sidewarpindex = 0;
	
	if(!p_igetw(&(temp_mapscr->undercombo),f))
	{
		return qe_invalid;
	}
	
	if(Header->zelda_version < 0x193)
	{
		if (!p_getc(&old_combo_pages[scrind], f))
		{
			return qe_invalid;
		}
	}
	
	if(!p_getc(&(temp_mapscr->undercset),f))                //recalculated for older quests
	{
		return qe_invalid;
	}
	
	if(!p_igetw(&(temp_mapscr->catchall),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->flags),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->flags2),f))
	{
		return qe_invalid;
	}
	
	if(!p_getc(&(temp_mapscr->flags3),f))
	{
		return qe_invalid;
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>1)))
		//if (version>2)
	{
		if(!p_getc(&(temp_mapscr->flags4),f))
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>7)))
	{
		if(!p_getc(&(temp_mapscr->flags5),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_mapscr->noreset),f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&(temp_mapscr->nocarry),f))
		{
			return qe_invalid;
		}
		
		if(temp_mapscr->flags5&32)
		{
			temp_mapscr->flags5 &= ~32;
			temp_mapscr->noreset |= 48;
		}
		
		if(version<8)
		{
			if(temp_mapscr->noreset&1)
			{
				temp_mapscr->noreset|=8192;
			}
			
			if(temp_mapscr->nocarry&1)
			{
				temp_mapscr->nocarry|=8192;
				temp_mapscr->nocarry&=~1;
			}
		}
	}
	else
	{
		temp_mapscr->flags5 = 0;
		temp_mapscr->noreset = 0;
		temp_mapscr->nocarry = 0;
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>9)))
	{
		if(!p_getc(&(temp_mapscr->flags6),f))
		{
			return qe_invalid;
		}
	}
	
	if(version>5)
	{
		if(!p_getc(&(temp_mapscr->flags7),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->flags8),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->flags9),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->flags10),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->csensitive),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->csensitive=1;
	}
	
	if(version<14) // August 2007: screen SFX added
	{
		if(temp_mapscr->flags&8) //fROAR
		{
			temp_mapscr->bosssfx=
				(temp_mapscr->flags3&2) ? WAV_DODONGO :  // fDODONGO
				(temp_mapscr->flags2&32) ? WAV_VADER : // fVADER
				WAV_ROAR;
		}
		
		if(temp_mapscr->flags&128) //fSEA
		{
			temp_mapscr->oceansfx=WAV_SEA;
		}
		
		temp_mapscr->secretsfx = (temp_mapscr->flags3&64) //fNOSECRETSOUND
			? 0 : WAV_SECRET;
		
		temp_mapscr->flags3 &= ~66; //64|2
		temp_mapscr->flags2 &= ~32;
		temp_mapscr->flags &= ~136; // 128|8
	}
	else
	{
		if(!p_getc(&(temp_mapscr->oceansfx),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->bosssfx),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->secretsfx),f))
		{
			return qe_invalid;
		}
	}
	
	if(version<15) // October 2007: another SFX
	{
		temp_mapscr->holdupsfx=WAV_PICKUP;
	}
	else
	{
		if(!p_getc(&(temp_mapscr->holdupsfx),f))
		{
			return qe_invalid;
		}
	}
	
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>97)))
	{
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&(temp_mapscr->layermap[k]),f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&(temp_mapscr->layerscreen[k]),f))
			{
				return qe_invalid;
			}
		}
	}
	else if((Header->zelda_version == 0x192)&&(Header->build>23)&&(Header->build<98))
	{
		if(!p_getc(&(temp_mapscr->layermap[2]),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->layerscreen[2]),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->layermap[4]),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->layerscreen[4]),f))
		
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version == 0x192)&&(Header->build>149))
	{
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layerxsize
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layerxspeed
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layerxdelay
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layerysize
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layeryspeed
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&tempbyte,f))                          //layerydelay
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>149)))
	{
		for(int32_t k=0; k<6; k++)
		{
			if(!p_getc(&(temp_mapscr->layeropacity[k]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>153)))
	{
		if((Header->zelda_version == 0x192)&&(Header->build>153))
		{
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_igetw(&(temp_mapscr->timedwarptics),f))
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<24)))
	{
		extras=15;
	}
	else if(((Header->zelda_version == 0x192)&&(Header->build<98)))
	{
		extras=11;
	}
	else if((Header->zelda_version == 0x192)&&(Header->build<150))
	{
		extras=32;
	}
	else if((Header->zelda_version == 0x192)&&(Header->build<154))
	{
		extras=64;
	}
	else if(Header->zelda_version < 0x193)
	{
		extras=62;
	}
	else
	
	{
		extras=0;
	}
	
	for(int32_t k=0; k<extras; k++)
	{
		if(!p_getc(&tempbyte,f))                            //extra[k]
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version > 0x211)||((Header->zelda_version == 0x211)&&(Header->build>2)))
		//if (version>3)
	{
		if(!p_getc(&(temp_mapscr->nextmap),f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&(temp_mapscr->nextscr),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->nextmap=0;
		temp_mapscr->nextscr=0;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<137)))
	{
		secretcombos=20;
	}
	else if((Header->zelda_version == 0x192)&&(Header->build<154))
	{
		secretcombos=256;
	}
	else
	{
		secretcombos=128;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<154)))
	{
		for(int32_t k=0; k<secretcombos; k++)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			
			if(k<128)
			{
				temp_mapscr->secretcombo[k]=tempbyte;
			}
		}
	}
	else
	{
		for(int32_t k=0; k<128; k++)
		{
			if(!p_igetw(&(temp_mapscr->secretcombo[k]),f))
			{
				return qe_invalid;
			}
			
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>153)))
	{
		for(int32_t k=0; k<128; k++)
		{
			if(!p_getc(&(temp_mapscr->secretcset[k]),f))
			{
				return qe_invalid;
			}
		}
		
		for(int32_t k=0; k<128; k++)
		{
			if(!p_getc(&(temp_mapscr->secretflag[k]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version == 0x192)&&(Header->build>97)&&(Header->build<154))
	{
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
	}
	
	for(int32_t k=0; k<176; k++)
	{
		if(!p_igetw(&(temp_mapscr->data[k]),f))
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version == 0x192)&&(Header->build>20)&&(Header->build<24))
	{
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&padding,f))
		{
			return qe_invalid;
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>20)))
	{
		for(int32_t k=0; k<176; k++)
		{
			if(!p_getc(&(temp_mapscr->sflag[k]),f))
			{
				return qe_invalid;
			}
			
			if((Header->zelda_version == 0x192)&&(Header->build<24))
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
			}
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>97)))
	{
		for(int32_t k=0; k<176; k++)
		{
		
			if(!p_getc(&(temp_mapscr->cset[k]),f))
			{
				return qe_invalid;
			}
		}
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<154)))
	{
		temp_mapscr->undercset=(temp_mapscr->undercombo>>8)&7;
		temp_mapscr->undercombo=(temp_mapscr->undercombo&0xFF)+(old_combo_pages[scrind]<<8);
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<137)))
	{
		temp_mapscr->secretcombo[sSBOMB]=temp_mapscr->secretcombo[sBOMB];
		temp_mapscr->secretcombo[sRCANDLE]=temp_mapscr->secretcombo[sBCANDLE];
		temp_mapscr->secretcombo[sWANDFIRE]=temp_mapscr->secretcombo[sBCANDLE];
		temp_mapscr->secretcombo[sDIVINEFIRE]=temp_mapscr->secretcombo[sBCANDLE];
		temp_mapscr->secretcombo[sSARROW]=temp_mapscr->secretcombo[sARROW];
		temp_mapscr->secretcombo[sGARROW]=temp_mapscr->secretcombo[sARROW];
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<154)))
	{
		for(int32_t k=0; k<176; k++)
		{
			if((Header->zelda_version == 0x192)&&(Header->build>149))
			{
				if((Header->zelda_version == 0x192)&&(Header->build!=153))
				{
					temp_mapscr->cset[k]=((temp_mapscr->data[k]>>8)&7);
				}
			}
			else
			{
				if((Header->zelda_version < 0x192)||
						((Header->zelda_version == 0x192)&&(Header->build<21)))
				{
					temp_mapscr->sflag[k]=(temp_mapscr->data[k]>>11);
				}
				
				temp_mapscr->cset[k]=((temp_mapscr->data[k]>>8)&7);
			}
			
			temp_mapscr->data[k]=(temp_mapscr->data[k]&0xFF)+(old_combo_pages[scrind]<<8);
		}
	}
	
	/*if(version>12)
	{
	  if(!p_getc(&(temp_mapscr->scrWidth),f))
	  {
		return qe_invalid;
	  }
	if(!p_getc(&(temp_mapscr->scrHeight),f))
	  {
		return qe_invalid;
	  }
	}*/
	
	if(version>4)
	{
		if(!p_igetw(&(temp_mapscr->screen_midi),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->screen_midi = -1;
	}
	
	if(version>=17)
	{
		if(!p_getc(&(temp_mapscr->lens_layer),f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_mapscr->lens_layer = llNORMAL;
	}
	
	if(version>6)
	{
		dword bits;
		if(!p_igetl(&bits,f))
		{
			return qe_invalid;
		}
		
		int32_t m;
		float tempfloat;
		word tempw;
		temp_mapscr->ffcCountMarkDirty();
		temp_mapscr->ffcs.clear();
		temp_mapscr->resizeFFC(32);
		for(m=0; m<32; m++)
		{
			ffcdata& tempffc = temp_mapscr->ffcs[m];
			if((bits>>m)&1)
			{
				if(!p_igetw(&tempw,f))
				{
					return qe_invalid;
				}
				tempffc.data = tempw;
				
				if(!p_getc(&(tempffc.cset),f))
				{
					return qe_invalid;
				}
				
				if(!p_igetw(&(tempffc.delay),f))
				{
					return qe_invalid;
				}
				
				if(version < 9)
				{
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.x=zslongToFix(int32_t(tempfloat*10000));
					
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.y=zslongToFix(int32_t(tempfloat*10000));
					
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.vx=zslongToFix(int32_t(tempfloat*10000));
					
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.vy=zslongToFix(int32_t(tempfloat*10000));
					
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.ax=zslongToFix(int32_t(tempfloat*10000));
					
					if(!p_igetf_DO_NOT_USE(&tempfloat,f))
					{
						return qe_invalid;
					}
					
					tempffc.ay=zslongToFix(int32_t(tempfloat*10000));
				}
				else
				{
					if(!p_igetzf(&(tempffc.x),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetzf(&(tempffc.y),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetzf(&(tempffc.vx),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetzf(&(tempffc.vy),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetzf(&(tempffc.ax),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetzf(&(tempffc.ay),f))
					{
						return qe_invalid;
					}
				}
				
				if(!p_getc(&(tempffc.link),f))
				{
					return qe_invalid;
				}
				
				if(version>7)
				{
					if(!p_getc(&tempbyte,f))
					{
						return qe_invalid;
					}
			
					tempffc.hit_width = (tempbyte&0x3F)+1;
					tempffc.txsz = (tempbyte>>6)+1;
					
					if(!p_getc(&tempbyte,f))
					{
						return qe_invalid;
					}
			
					tempffc.hit_height = (tempbyte&0x3F)+1;
					tempffc.tysz = (tempbyte>>6)+1;
					
					if(!p_igetl(&(tempffc.flags),f))
					{
						return qe_invalid;
					}
				}
				else
				{
					tempffc.hit_width=16;
					tempffc.hit_height=16; 
					tempffc.txsz=1;
					tempffc.tysz=1;
					tempffc.flags=ffc_none;
				}
		
				tempffc.updateSolid();
			
				
				if(Header->zelda_version == 0x211 || (Header->zelda_version == 0x250 && Header->build<20))
				{
					tempffc.flags|=ffc_ignoreholdup;
				}
				
				if(version>9)
				{
					if(!p_igetw(&(tempffc.script),f))
					{
						return qe_invalid;
					}
				}
				else
				{
					tempffc.script=0;
				}
				
				if(version>10)
				{
					if(!p_igetl(&(tempffc.initd[0]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[1]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[2]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[3]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[4]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[5]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[6]),f))
					{
						return qe_invalid;
					}
					
					if(!p_igetl(&(tempffc.initd[7]),f))
					{
						return qe_invalid;
					}
					
					if(!p_getc(&(tempbyte),f))
					{
						return qe_invalid;
					}
					if(!p_getc(&(tempbyte),f))
					{
						return qe_invalid;
					}
				}
				
				if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
				{
					tempffc.script = 0;
					for(int q = 0; q < 8; ++q)
						tempffc.initd[q] = 0;
				}
				if(version <= 11)
				{
					fixffcs=true;
				}
			}
		}
	}

	temp_mapscr->ffcCountMarkDirty();
	temp_mapscr->resizeFFC(temp_mapscr->numFFC());
	temp_mapscr->ffcs.shrink_to_fit();
	
	//add in the new whistle flags
	if(version<13)
	{
		if(temp_mapscr->flags & fWHISTLE)
		{
			temp_mapscr->flags7 |= (fWHISTLEPAL | fWHISTLEWATER);
		}
	}
	
	//2.55 starts here
	if ( version >= 19 && Header->zelda_version > 0x253 )
	{
		// mapscr fields that were never used, so are now removed:
		// int32_t npcstrings[10];
		// int16_t new_items[10];
		// int16_t new_item_x[10];
		// int16_t new_item_y[10];
		if (pack_fseek(f, 100))
		{
			return qe_invalid;
		}
	}

	if ( version >= 20 && Header->zelda_version > 0x253 )
	{
	if(!p_igetw(&(temp_mapscr->script),f))
	{
		return qe_invalid;
	} 
	for ( int32_t q = 0; q < 8; q++)
	{
		if(!p_igetl(&(temp_mapscr->screeninitd[q]),f))
		{
			return qe_invalid;
		}
	}		
	}
	if ( version < 20 )
	{
	temp_mapscr->script = 0;
	for ( int32_t q = 0; q < 8; q++) temp_mapscr->screeninitd[q] = 0;
	}
	if ( version >= 21 && Header->zelda_version > 0x253 )
	{
	if(!p_getc(&(temp_mapscr->preloadscript),f))
	{
		return qe_invalid;
	}       
	}
	if ( version < 21 )
	{
	temp_mapscr->preloadscript = 0;    
	}
	//all builds with version > 20 need this. -Z
	
	if ( version >= 22 && Header->zelda_version > 0x253 ) //26th June, 2019; Layer Visibility
	{
	if(!p_getc(&(temp_mapscr->hidelayers ),f))
	{
		return qe_invalid;
	} 
	if(!p_getc(&(temp_mapscr->hidescriptlayers  ),f))
	{
		return qe_invalid;
	}      
	}
	if ( version < 22 )
	{
	temp_mapscr->hidelayers = 0;    
	temp_mapscr->hidescriptlayers = 0;    
	}
	
	//Dodongos in 2.10 used the boss roar, not the dodongo sound. -Z
	//May be any version before 2.11. -Z
	/* --not the roar, the HIT SFX
	if ( Header->zelda_version <= 0x210 ) 
	{
	if ( temp_mapscr->bosssfx == WAV_DODONGO ) 
	{
		temp_mapscr->bosssfx = WAV_ROAR;
	}
	}
	*/
	for(int32_t k=0; k<4; k++)
	{
		if(temp_mapscr->door[k] == dNONE)
			temp_mapscr->door[k] = dWALL;
	}
	
	return 0;
}
int32_t readmapscreen(PACKFILE *f, zquestheader *Header, mapscr *temp_mapscr, word version, int scrind)
{
	if(version < 23)
	{
		auto ret = readmapscreen_old(f,Header,temp_mapscr,version,scrind);
		if(ret) return ret;
	}
	else
	{
		if(!p_getc(&(temp_mapscr->valid),f))
			return qe_invalid;
		if(!(temp_mapscr->valid & mVALID))
		{
			int map = scrind/MAPSCRS;
			int scr = scrind%MAPSCRS;
			if(version > 25 && scrind > -1 && (map*6+5) < map_autolayers.size())
			{
				//Empty screen, apply autolayers
				for(int q = 0; q < 6; ++q)
				{
					auto layermap = map_autolayers[map*6+q];
					temp_mapscr->layermap[q] = layermap;
					if(layermap)
						temp_mapscr->layerscreen[q] = scr;
				}
			}
			return 0;
		}
		uint32_t scr_has_flags;
		if(!p_igetl(&scr_has_flags,f))
			return qe_invalid;
		
		if(scr_has_flags & SCRHAS_ROOMDATA)
		{
			if(!p_getc(&(temp_mapscr->guy),f))
				return qe_invalid;
			if(version > 26)
			{
				if(!p_igetl(&(temp_mapscr->guytile),f))
					return qe_invalid;
				if(!p_getc(&(temp_mapscr->guycs),f))
					return qe_invalid;
				if(!p_igetw(&(temp_mapscr->roomflags),f))
					return qe_invalid;
			}
			else
			{
				temp_mapscr->guytile = -1; //signal to use default guy values
				SETFLAG(temp_mapscr->roomflags,RFL_ALWAYS_GUY,temp_mapscr->guy==gFAIRY);
				SETFLAG(temp_mapscr->roomflags,RFL_GUYFIRES,temp_mapscr->guy!=gFAIRY || !get_qr(qr_NOFAIRYGUYFIRES));
			}
			if(!p_igetw(&(temp_mapscr->str),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->room),f))
				return qe_invalid;
			if(!p_igetw(&(temp_mapscr->catchall),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_ITEM)
		{
			if(!p_getc(&(temp_mapscr->item),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->hasitem),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->itemx),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->itemy),f))
				return qe_invalid;
		}
		if(scr_has_flags & (SCRHAS_SWARP|SCRHAS_TWARP))
		{
			if(!p_igetw(&temp_mapscr->warpreturnc,f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_TWARP)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->tilewarptype[i]),f))
					return qe_invalid;
			}
			for(int32_t i=0; i<4; i++)
			{
				if(!p_igetw(&(temp_mapscr->tilewarpdmap[i]),f))
					return qe_invalid;
			}
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->tilewarpscr[i]),f))
					return qe_invalid;
			}
			if(!p_getc(&(temp_mapscr->tilewarpoverlayflags),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_SWARP)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->sidewarptype[i]),f))
					return qe_invalid;
			}
			for(int32_t i=0; i<4; i++)
			{
				if(!p_igetw(&(temp_mapscr->sidewarpdmap[i]),f))
					return qe_invalid;
			}
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->sidewarpscr[i]),f))
					return qe_invalid;
			}
			if(!p_getc(&(temp_mapscr->sidewarpoverlayflags),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->sidewarpindex),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_WARPRET)
		{
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->warpreturnx[i]),f))
					return qe_invalid;
			}
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&(temp_mapscr->warpreturny[i]),f))
					return qe_invalid;
			}
			if(!p_getc(&(temp_mapscr->warparrivalx),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->warparrivaly),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_LAYERS)
		{
			for(int32_t k=0; k<6; k++)
			{
				if(!p_getc(&(temp_mapscr->layermap[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<6; k++)
			{
				if(!p_getc(&(temp_mapscr->layerscreen[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<6; k++)
			{
				if(!p_getc(&(temp_mapscr->layeropacity[k]),f))
					return qe_invalid;
			}
			if(!p_getc(&(temp_mapscr->hidelayers),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->hidescriptlayers),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_MAZE)
		{
			for(int32_t k=0; k<4; k++)
			{
				if(!p_getc(&(temp_mapscr->path[k]),f))
					return qe_invalid;
			}
			if(!p_getc(&(temp_mapscr->exitdir),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_D_S_U)
		{
			if(!p_igetw(&(temp_mapscr->door_combo_set),f))
				return qe_invalid;
			for(int32_t k=0; k<4; k++)
			{
				if(!p_getc(&(temp_mapscr->door[k]),f))
					return qe_invalid;
				if(version < 29 && temp_mapscr->door[k] == dNONE)
					temp_mapscr->door[k] = dWALL;
			}
			
			if(!p_getc(&(temp_mapscr->stairx),f))
				return qe_invalid;
			
			if(!p_getc(&(temp_mapscr->stairy),f))
				return qe_invalid;
			if(!p_igetw(&(temp_mapscr->undercombo),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->undercset),f))
				return qe_invalid;
		}
		else if(version < 29)
		{
			for(int k = 0; k < 4; ++k)
				temp_mapscr->door[k] = dWALL;
		}
		if(scr_has_flags & SCRHAS_FLAGS)
		{
			if(!p_getc(&(temp_mapscr->flags),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags2),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags3),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags4),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags5),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags6),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags7),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags8),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags9),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->flags10),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->enemyflags),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_ENEMY)
		{
			for(int32_t k=0; k<10; k++)
			{
				if(!p_igetw(&(temp_mapscr->enemy[k]),f))
					return qe_invalid;
				if (unsigned(temp_mapscr->enemy[k]) > MAXGUYS)
					temp_mapscr->enemy[k] = 0;
			}
			if(!p_getc(&(temp_mapscr->pattern),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_CARRY)
		{
			if(!p_igetw(&(temp_mapscr->noreset),f))
				return qe_invalid;
			if(!p_igetw(&(temp_mapscr->nocarry),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->nextmap),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->nextscr),f))
				return qe_invalid;
		}
		if(scr_has_flags & SCRHAS_SCRIPT)
		{
			if(!p_igetw(&(temp_mapscr->script),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->preloadscript),f))
				return qe_invalid;
			for ( int32_t q = 0; q < 8; q++ )
			{
				if(!p_igetl(&(temp_mapscr->screeninitd[q]),f))
					return qe_invalid;
			}
		}
		if(scr_has_flags & SCRHAS_SECRETS)
		{
			for(int32_t k=0; k<128; k++)
			{
				if(!p_igetw(&(temp_mapscr->secretcombo[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<128; k++)
			{
				if(!p_getc(&(temp_mapscr->secretcset[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<128; k++)
			{
				if(!p_getc(&(temp_mapscr->secretflag[k]),f))
					return qe_invalid;
			}
		}
		if(scr_has_flags & SCRHAS_COMBOFLAG)
		{
			for(int32_t k=0; k<176; ++k)
			{
				if(!p_igetw(&(temp_mapscr->data[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<176; ++k)
			{
				if(!p_getc(&(temp_mapscr->sflag[k]),f))
					return qe_invalid;
			}
			for(int32_t k=0; k<176; ++k)
			{
				if(!p_getc(&(temp_mapscr->cset[k]),f))
					return qe_invalid;
			}
		}
		if(scr_has_flags & SCRHAS_MISC)
		{
			if(!p_igetw(&(temp_mapscr->color),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->csensitive),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->oceansfx),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->bosssfx),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->secretsfx),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->holdupsfx),f))
				return qe_invalid;
			if(!p_igetw(&(temp_mapscr->timedwarptics),f))
				return qe_invalid;
			if(!p_igetw(&(temp_mapscr->screen_midi),f))
				return qe_invalid;
			if(!p_getc(&(temp_mapscr->lens_layer),f))
				return qe_invalid;
			if(version > 27)
			{
				if(!p_getc(&(temp_mapscr->lens_show),f))
					return qe_invalid;
				if(!p_getc(&(temp_mapscr->lens_hide),f))
					return qe_invalid;
			}
		}
		else
		{
			temp_mapscr->screen_midi = -1;
			temp_mapscr->csensitive = 1;
		}
		//FFC
		bool old_ff = version < 25;
		dword bits = 0;
		word numffc = 32;
		if(old_ff)
		{
			if(!p_igetl(&bits,f))
				return qe_invalid;
		}
		else
		{
			if(!p_igetw(&numffc,f))
				return qe_invalid;
		}
		byte tempbyte;
		word tempw;
		static ffcdata nil_ffc;
		temp_mapscr->ffcCountMarkDirty();
		temp_mapscr->ffcs.clear();
		temp_mapscr->resizeFFC(std::min(MAXFFCS, (int)numffc));
		for(word m = 0; m < numffc; ++m)
		{
			ffcdata& tempffc = (m < MAXFFCS)
				? temp_mapscr->ffcs[m]
				: nil_ffc; //sanity
			if(old_ff && !(bits & (1<<m))) continue;
			
			if(!p_igetw(&tempw,f))
				return qe_invalid;
			if(!old_ff && !tempw) //empty ffc, nothing more to load
				continue;
			tempffc.data = tempw;
			
			if(!p_getc(&(tempffc.cset),f))
				return qe_invalid;
			if(!p_igetw(&(tempffc.delay),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.x),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.y),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.vx),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.vy),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.ax),f))
				return qe_invalid;
			if(!p_igetzf(&(tempffc.ay),f))
				return qe_invalid;
			if(!p_getc(&(tempffc.link),f))
				return qe_invalid;
			if(version < 24)
			{
				if(!p_getc(&tempbyte,f))
					return qe_invalid;
				tempffc.hit_width = (tempbyte&0x3F)+1;
				tempffc.txsz = (tempbyte>>6)+1;
				if(!p_getc(&tempbyte,f))
					return qe_invalid;
				tempffc.hit_height = (tempbyte&0x3F)+1;
				tempffc.tysz = (tempbyte>>6)+1;
			}
			else
			{
				if(!p_igetl(&(tempffc.hit_width),f))
					return qe_invalid;
				if(!p_igetl(&(tempffc.hit_height),f))
					return qe_invalid;
				if(!p_getc(&(tempffc.txsz),f))
					return qe_invalid;
				if(!p_getc(&(tempffc.tysz),f))
					return qe_invalid;
			}
			if(!p_igetl(&(tempffc.flags),f))
				return qe_invalid;
			tempffc.updateSolid();
			if(!p_igetw(&(tempffc.script),f))
				return qe_invalid;
			for(auto q = 0; q < 8; ++q)
			{
				if(!p_igetl(&(tempffc.initd[q]),f))
					return qe_invalid;
			}
			if(!p_getc(&(tempbyte),f))
				return qe_invalid;
			if(!p_getc(&(tempbyte),f))
				return qe_invalid;
			
			if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
			{
				tempffc.script = 0;
				for(int q = 0; q < 8; ++q)
					tempffc.initd[q] = 0;
			}
		}
		//END FFC
		if(version > 29)
			if(!p_getlstr(&temp_mapscr->usr_notes, f))
				return qe_invalid;
	}

	temp_mapscr->ffcCountMarkDirty();
	temp_mapscr->resizeFFC(temp_mapscr->numFFC());
	temp_mapscr->ffcs.shrink_to_fit();

	return 0;
}

int32_t readmaps(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_maps);
	int32_t scr=0;
	
	word version=0;
	dword dummy;
	int32_t screens_to_read;
	
	mapscr temp_mapscr{};
	word temp_map_count;
	dword section_size;
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<137)))
	{
		screens_to_read=MAPSCRS192b136;
	}
	else
	{
		screens_to_read=MAPSCRS;
	}
	
	if(Header->zelda_version > 0x192)
	{
		//section version info
		if(!p_igetw(&version,f))
		{
			return qe_invalid;
		}
	
		FFCore.quest_format[vMaps] = version;
		
		if(!p_igetw(&dummy,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&section_size,f))
		{
			return qe_invalid;
		}
		
		//finally...  section data
		if(!p_igetw(&temp_map_count,f))
		{
			return 5;
		}
	}
	else
	{
		temp_map_count=map_count;
	}

	if (temp_map_count > MAXMAPS)
	{
		return qe_invalid;
	}

	if (!should_skip)
	{
		const int32_t _mapsSize = MAPSCRS*temp_map_count;
		TheMaps.clear();
		TheMaps.resize(_mapsSize);
		old_combo_pages.clear();
		old_combo_pages.resize(_mapsSize);
		map_autolayers.clear();
		map_autolayers.resize(temp_map_count*6);
	}
	
	for(int32_t i=0; i<temp_map_count && i<MAXMAPS; i++)
	{
		byte valid=1;
		if(version > 22)
		{
			if(!p_getc(&valid,f))
				return qe_invalid;
		}
		if(valid && version > 25)
		{
			for(int q = 0; q < 6; ++q)
			{
				if(!p_igetw(&map_autolayers[i*6+q],f))
					return qe_invalid;
			}
		}
		for(int32_t j=0; j<screens_to_read; j++)
		{
			scr=i*MAPSCRS+j;
			mapscr* screen = should_skip ? &temp_mapscr : &TheMaps[scr];
			if(valid)
				readmapscreen(f, Header, screen, version, scr);
			else if (!should_skip)
				clear_screen(screen);
		}

		if (should_skip)
			continue;
		
		if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<137)))
		{
			int32_t index = (i*MAPSCRS+132);
			
			TheMaps[index]=TheMaps[index-1];
			
			MEMCPY_ARR(TheMaps[i*MAPSCRS+132].data, TheMaps[i*MAPSCRS+131].data);
			MEMCPY_ARR(TheMaps[i*MAPSCRS+132].sflag, TheMaps[i*MAPSCRS+131].sflag);
			MEMCPY_ARR(TheMaps[i*MAPSCRS+132].cset, TheMaps[i*MAPSCRS+131].cset);
			
			for(int32_t j=133; j<MAPSCRS; j++)
			{
				scr=i*MAPSCRS+j;
				
				TheMaps[scr].zero_memory();
				TheMaps[scr].valid = mVERSION;
				TheMaps[scr].screen_midi = -1;
				TheMaps[scr].csensitive = 1;
			}
		}
		
		if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<154)))
		{
			for(int32_t j=0; j<MAPSCRS; j++)
			{
				scr=i*MAPSCRS+j;
				TheMaps[scr].door_combo_set=MakeDoors(i, j);
				
				for(int32_t k=0; k<128; k++)
				{
					TheMaps[scr].secretcset[k]=tcmbcset2(i, TheMaps[scr].secretcombo[k]);
					TheMaps[scr].secretflag[k]=tcmbflag2(i, TheMaps[scr].secretcombo[k]);
					TheMaps[scr].secretcombo[k]=tcmbdat2(i, j, TheMaps[scr].secretcombo[k]);
				}
			}
		}
	}
	map_count = temp_map_count;
	clear_screen(&temp_mapscr);
	return 0;
}


void update_combo(newcombo& cmb, word section_version)
{
	if(section_version < 40)
	{
		switch(cmb.type)
		{
			case cWATER: case cSHALLOWWATER:
				cmb.attribytes[6] = iwRipples;
				break;
			case cTALLGRASS: case cTALLGRASSNEXT: case cTALLGRASSTOUCHY:
				cmb.attribytes[6] = iwTallGrass;
				break;
		}
	}
	if(section_version < 49)
	{
		if(cmb.type == cWATER || cmb.type == cSHALLOWWATER)
			cmb.sfx_landing = WAV_ZN1SPLASH;
	}
}
int32_t readcombos_old(word section_version, PACKFILE *f, zquestheader *, word version, word build, word start_combo, word max_combos)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_combos);

	if (!should_skip)
	{
		reset_combo_animations();
		reset_combo_animations2();
		init_combo_classes();
	}

	// combos
	word combos_used=0;
	int32_t dummy;
	byte padding;
	newcombo temp_combo;
	//word section_cversion=0;

	if (!should_skip)
	for(int32_t q = start_combo; q < start_combo+max_combos; ++q)
		combobuf[q].clear();

	// if(version > 0x192)
	// {
		// //section version info
		// if(!p_igetw(&section_version,f))
		// {
			// return qe_invalid;
		// }
		
		// FFCore.quest_format[vCombos] = section_version;
		
		// if(!p_igetw(&section_cversion,f))
		// {
			// return qe_invalid;
		// }
		
		// //section size
		// if(!p_igetl(&dummy,f))
		// {
			// return qe_invalid;
		// }
	// }

	if(version < 0x174)
	{
		combos_used=1024;
	}
	else if(version < 0x191)
	{
		combos_used=2048;
	}
	else
	{
		if(!p_igetw(&combos_used,f))
		{
			return qe_invalid;
		}
	}

	//finally...  section data
	for(int32_t i=0; i<combos_used; i++)
	{
		temp_combo.clear();
		
		if ( section_version >= 11 )
		{
			if(!p_igetl(&temp_combo.tile,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			if(!p_igetw(&temp_combo.tile,f))
			{
				return qe_invalid;
			}
		}
		temp_combo.o_tile = temp_combo.tile;
		if(!p_getc(&temp_combo.flip,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_combo.walk,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_combo.type,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_combo.csets,f))
		{
			return qe_invalid;
		}
		
		if(version < 0x193)
		{
			if(!p_getc(&padding,f))
				return qe_invalid;
			
			if(!p_getc(&padding,f))
				return qe_invalid;
			
			if(version < 0x192)
			{
				if(version == 0x191)
				{
					for(int32_t tmpcounter=0; tmpcounter<16; tmpcounter++)
					{
						if(!p_getc(&padding,f))
							return qe_invalid;
					}
				}
			}
		}
		if(version >= 0x192)
		{
			if(!p_getc(&temp_combo.frames,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.speed,f))
				return qe_invalid;
			
			if(!p_igetw(&temp_combo.nextcombo,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.nextcset,f))
				return qe_invalid;
			
			//Base flag
			if(section_version>=3)
				if(!p_getc(&temp_combo.flag,f))
					return qe_invalid;
			
			if(section_version>=4)
			{
				if(!p_getc(&temp_combo.skipanim,f))
					return qe_invalid;
				
				if(!p_igetw(&temp_combo.nexttimer,f))
					return qe_invalid;
			}
			
			if(section_version>=5)
				if(!p_getc(&temp_combo.skipanimy,f))
					return qe_invalid;
			
			if(section_version>=6)
			{
				if(!p_getc(&temp_combo.animflags,f))
					return qe_invalid;
				
				if(section_version == 6)
					temp_combo.animflags = temp_combo.animflags ? AF_FRESH : 0;
			}
			
			if(section_version>=8) //combo Attributes[4] and userflags.
			{
				for ( int32_t q = 0; q < NUM_COMBO_ATTRIBUTES; q++ )
					if(!p_igetl(&temp_combo.attributes[q],f))
						return qe_invalid;
				if(!p_igetl(&temp_combo.usrflags,f))
					return qe_invalid;
				if(section_version >= 20)
					if(!p_igetw(&temp_combo.genflags,f))
						return qe_invalid;
			}
			if(section_version>=10) //combo trigger flags
			{
				for ( int32_t q = 0; q < 3; q++ )
					if(!p_igetl(&temp_combo.triggerflags[q],f))
						return qe_invalid;
			}
			else if(section_version==9) //combo trigger flags, V9 only had two indices of triggerflags[]
			{
				for ( int32_t q = 0; q < 2; q++ )
					if(!p_igetl(&temp_combo.triggerflags[q],f))
						return qe_invalid;
			}
			if(section_version >= 9)
				if(!p_igetl(&temp_combo.triggerlevel,f))
					return qe_invalid;
			if(section_version >= 22)
				if(!p_getc(&temp_combo.triggerbtn,f))
					return qe_invalid;
			if(section_version >= 24)
			{
				if(!p_getc(&temp_combo.triggeritem,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.trigtimer,f))
					return qe_invalid;
			}
			if(section_version >= 25)
				if(!p_getc(&temp_combo.trigsfx,f))
					return qe_invalid;
			if(section_version >= 27)
				if(!p_igetl(&temp_combo.trigchange,f))
					return qe_invalid;
			
			if(section_version >= 29)
			{
				if(!p_igetw(&temp_combo.trigprox,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.trigctr,f))
					return qe_invalid;
				if(!p_igetl(&temp_combo.trigctramnt,f))
					return qe_invalid;
			}
			if(section_version >= 30)
				if(!p_getc(&temp_combo.triglbeam,f))
					return qe_invalid;
			if(section_version >= 31)
			{
				if(!p_getc(&temp_combo.trigcschange,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.spawnitem,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.spawnenemy,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.exstate,f))
					return qe_invalid;
				if(!p_igetl(&temp_combo.spawnip,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.trigcopycat,f))
					return qe_invalid;
			}
			if(section_version >= 32)
				if(!p_getc(&temp_combo.trigcooldown,f))
					return qe_invalid;
			
			if(section_version>=12) //combo label
			{
				char label[12];
				label[11] = '\0';
				for ( int32_t q = 0; q < 11; q++ )
					if(!p_getc(&label[q],f))
						return qe_invalid;
				temp_combo.label = label;
			}
			if(section_version>=13) //attribytes[4]
				for ( int32_t q = 0; q < 4; q++ )
					if(!p_getc(&temp_combo.attribytes[q],f))
						return qe_invalid;
			/* HIGHLY UNORTHODOX UPDATING THING, by Deedee
			* This fixes a poor implementation of a ->next flag bug thing.
			* Zoria didn't bump up the versions as liberally as he should have, but thankfully
			* there was a version bump a few weeks before a change that broke stuff.
			*/
			if (section_version >= 13 && section_version < 21)
			{
				set_qr(qr_BUGGY_BUGGY_SLASH_TRIGGERS,1);
			}
			//combo scripts
			if(section_version>=14) 
			{
				if(!p_igetw(&temp_combo.script,f))
					return qe_invalid;
				for ( int32_t q = 0; q < 2; q++ )
					if(!p_igetl(&temp_combo.initd[q],f))
						return qe_invalid;
			}
			if(section_version>=15)
			{
				if(!p_igetl(&temp_combo.o_tile,f)) return qe_invalid;
				if(!temp_combo.o_tile) temp_combo.o_tile = temp_combo.tile;
				if(!p_getc(&temp_combo.cur_frame,f)) return qe_invalid;
				if(!p_getc(&temp_combo.aclk,f)) return qe_invalid;
			}
			if(section_version>=17) //attribytes[4]
			{
				for ( int32_t q = 4; q < 8; q++ ) //bump up attribytes...
					if(!p_getc(&temp_combo.attribytes[q],f))
						return qe_invalid;
				for ( int32_t q = 0; q < 8; q++ ) //...and add attrishorts
					if(!p_igetw(&temp_combo.attrishorts[q],f))
						return qe_invalid;
			}
			
			if(version < 0x193)
				for(int32_t q=0; q<11; q++)
					if(!p_getc(&dummy,f))
						return qe_invalid;
		}
		
		//Goriya tiles were flipped around in 2.11 build 7. Compensate for the flip here. -DD
		if((version < 0x211)||((version == 0x211)&&(build<7)))
		{
			if(!get_qr(qr_NEWENEMYTILES))
			{
				switch(temp_combo.tile)
				{
					case 130:
						temp_combo.tile = 132;
						break;
						
					case 131:
						temp_combo.tile = 133;
						break;
						
					case 132:
						temp_combo.tile = 130;
						break;
						
					case 133:
						temp_combo.tile = 131;
						break;
				}
			}
		}
		
		if(section_version < 15)
			temp_combo.o_tile = temp_combo.tile;
		
		if(section_version<18) //upper bits for .walk
			temp_combo.walk |= 0xF0;
			
		if(section_version < 19)
			for(int32_t q = 0; q < 4; ++q)
				temp_combo.attributes[q] *= 10000L;
		
		if(section_version < 23)
		{
			switch(temp_combo.type) //combotriggerCMBTYPEFX now required for combotype-specific effects
			{
				case cSCRIPT1: case cSCRIPT2: case cSCRIPT3: case cSCRIPT4: case cSCRIPT5:
				case cSCRIPT6: case cSCRIPT7: case cSCRIPT8: case cSCRIPT9: case cSCRIPT10:
				case cTRIGGERGENERIC: case cCSWITCH:
					temp_combo.triggerflags[0] |= combotriggerCMBTYPEFX;
			}
		}
		if(section_version < 25)
		{
			switch(temp_combo.type)
			{
				case cLOCKBLOCK: case cBOSSLOCKBLOCK:
					if(!(temp_combo.usrflags & cflag3))
						temp_combo.attribytes[3] = WAV_DOOR;
					temp_combo.usrflags &= ~cflag3;
					break;
			}
		}
		
		if(section_version < 26)
			if(temp_combo.type == cARMOS)
				if(temp_combo.usrflags & cflag1)
					temp_combo.usrflags |= cflag3;
		
		if(section_version < 27)
		{
			if(temp_combo.triggerflags[0] & 0x00040000) //'next'
				temp_combo.trigchange = 1;
			else if(temp_combo.triggerflags[0] & 0x00080000) //'prev'
				temp_combo.trigchange = -1;
			else temp_combo.trigchange = 0;
			temp_combo.triggerflags[0] &= ~(0x00040000|0x00080000);
		}
		if(section_version < 28)
		{
			switch(temp_combo.type)
			{
				case cLOCKBLOCK: case cLOCKEDCHEST:
					if(temp_combo.usrflags & cflag7)
						temp_combo.usrflags |= cflag8;
					else temp_combo.usrflags &= ~cflag8;
					temp_combo.usrflags &= ~cflag7;
					break;
			}
			switch(temp_combo.type)
			{
				case cCHEST: case cLOCKEDCHEST: case cBOSSCHEST:
					temp_combo.attrishorts[2] = -1;
					temp_combo.usrflags |= cflag7;
					break;
			}
		}
		if(section_version < 20)
		{
			temp_combo.genflags = 0;
			switch(temp_combo.type)
			{
				case cPUSH_WAIT: case cPUSH_HEAVY:
				case cPUSH_HW: case cL_STATUE:
				case cR_STATUE: case cPUSH_HEAVY2:
				case cPUSH_HW2: case cPOUND:
				case cC_STATUE: case cMIRROR:
				case cMIRRORSLASH: case cMIRRORBACKSLASH:
				case cMAGICPRISM: case cMAGICPRISM4:
				case cMAGICSPONGE: case cEYEBALL_A:
				case cEYEBALL_B: case cEYEBALL_4:
				case cBUSH: case cFLOWERS:
				case cLOCKBLOCK: case cLOCKBLOCK2:
				case cBOSSLOCKBLOCK: case cBOSSLOCKBLOCK2:
				case cCHEST: case cCHEST2:
				case cLOCKEDCHEST: case cLOCKEDCHEST2:
				case cBOSSCHEST: case cBOSSCHEST2:
				case cBUSHNEXT: case cBUSHTOUCHY:
				case cFLOWERSTOUCHY: case cBUSHNEXTTOUCHY:
				case cSIGNPOST: case cCSWITCHBLOCK:
				case cTORCH: case cTRIGGERGENERIC:
					if(temp_combo.usrflags & cflag16)
					{
						temp_combo.genflags |= cflag1;
						temp_combo.usrflags &= ~cflag16;
					}
					break;
			}
		}
		
		update_combo(temp_combo, section_version);
		
		if(i>=start_combo && !should_skip)
		{
			if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
			{
				temp_combo.script = 0;
				for(int q = 0; q < 8; ++q)
					temp_combo.initd[q] = 0;
			}
			combobuf[i] = temp_combo;
		}
	}

	if (should_skip)
		return 0;

	if((version < 0x192)|| ((version == 0x192)&&(build<185)))
	{
		for(int32_t tmpcounter=0; tmpcounter<MAXCOMBOS; tmpcounter++)
		{
			if(combobuf[tmpcounter].type==cHOOKSHOTONLY)
			{
				combobuf[tmpcounter].type=cLADDERHOOKSHOT;
			}
		}
	}
	
	//June 3 2012; ladder only is broken in 2.10 and allows the hookshot also. -Gleeok
	if(version == 0x210 && get_app_id() != App::zquest)
	{
		for(int32_t tmpcounter=0; tmpcounter<MAXCOMBOS; tmpcounter++)
			if(combobuf[tmpcounter].type == cLADDERONLY)
				combobuf[tmpcounter].type = cLADDERHOOKSHOT;
	}
	
	if(section_version<7)
	{
		for(int32_t tmpcounter=0; tmpcounter<MAXCOMBOS; tmpcounter++)
		{
			switch(combobuf[tmpcounter].type)
			{
			case cSLASH:
				combobuf[tmpcounter].type=cSLASHTOUCHY;
				break;
				
			case cSLASHITEM:
				combobuf[tmpcounter].type=cSLASHITEMTOUCHY;
				break;
				
			case cBUSH:
				combobuf[tmpcounter].type=cBUSHTOUCHY;
				break;
				
			case cFLOWERS:
				combobuf[tmpcounter].type=cFLOWERSTOUCHY;
				break;
				
			case cTALLGRASS:
				combobuf[tmpcounter].type=cTALLGRASSTOUCHY;
				break;
				
			case cSLASHNEXT:
				combobuf[tmpcounter].type=cSLASHNEXTTOUCHY;
				break;
				
			case cSLASHNEXTITEM:
				combobuf[tmpcounter].type=cSLASHNEXTITEMTOUCHY;
				break;
				
			case cBUSHNEXT:
				combobuf[tmpcounter].type=cBUSHNEXTTOUCHY;
				break;
			}
		}
	}
	if (section_version < 16)
	{
		for(int32_t tmpcounter=0; tmpcounter<MAXCOMBOS; tmpcounter++)
		{
			if (combobuf[tmpcounter].type == cWATER)
			{
				combobuf[tmpcounter].attributes[0] = 40000L;
			}
		}
	}
	if(!get_qr(qr_ALLOW_EDITING_COMBO_0))
	{
		combobuf[0].walk = 0xF0;
		combobuf[0].type = 0;
		combobuf[0].flag = 0;
	}

	//Now for the new combo alias reset
	if(section_version<2)
	{
		for(int32_t j=0; j<MAXCOMBOALIASES; j++)
		{
			combo_aliases[j].width = 0;
			combo_aliases[j].height = 0;
			combo_aliases[j].layermask = 0;
			combo_aliases[j].combos.clear();
			combo_aliases[j].csets.clear();
		}
	}


	setup_combo_animations();
	setup_combo_animations2();
	return 0;
}
int32_t readcombo_loop(PACKFILE* f, word s_version, newcombo& temp_combo)
{
	byte combo_has_flags;
	if(!p_getc(&combo_has_flags,f))
		return qe_invalid;
	
	temp_combo.clear();
	if(combo_has_flags)
	{
		if(combo_has_flags&CHAS_BASIC)
		{
			if(!p_igetl(&temp_combo.tile,f))
				return qe_invalid;
			temp_combo.o_tile = temp_combo.tile;
			
			if(!p_getc(&temp_combo.flip,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.walk,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.type,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.flag,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.csets,f))
				return qe_invalid;
		}
		if(combo_has_flags&CHAS_SCRIPT)
		{
			if (s_version>=41)
			{
				p_getcstr(&temp_combo.label, f);
			}
			else
			{
				char label[12];
				label[11] = '\0';
				for ( int32_t q = 0; q < 11; q++ )
					if(!p_getc(&label[q],f))
						return qe_invalid;
				temp_combo.label = label;
			}

			if(!p_igetw(&temp_combo.script,f)) return qe_invalid;
			auto initd_count = s_version >= 43 ? 8 : 2;
			for ( int32_t q = 0; q < initd_count; q++ )
				if(!p_igetl(&temp_combo.initd[q],f))
					return qe_invalid;
		}
		if(combo_has_flags&CHAS_ANIM)
		{
			if(!p_getc(&temp_combo.frames,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.speed,f))
				return qe_invalid;
			
			if(!p_igetw(&temp_combo.nextcombo,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.nextcset,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.skipanim,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.skipanimy,f))
				return qe_invalid;
			
			if(!p_getc(&temp_combo.animflags,f))
				return qe_invalid;
		}
		if(combo_has_flags&CHAS_ATTRIB)
		{
			for ( int32_t q = 0; q < 4; q++ )
				if(!p_igetl(&temp_combo.attributes[q],f))
					return qe_invalid;
			for ( int32_t q = 0; q < 8; q++ )
				if(!p_getc(&temp_combo.attribytes[q],f))
					return qe_invalid;
			for ( int32_t q = 0; q < 8; q++ )
				if(!p_igetw(&temp_combo.attrishorts[q],f))
					return qe_invalid;
		}
		if(combo_has_flags&CHAS_FLAG)
		{
			if(!p_igetl(&temp_combo.usrflags,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.genflags,f))
				return qe_invalid;
		}
		if(combo_has_flags&CHAS_TRIG)
		{
			int numtrigs = s_version < 36 ? 3 : 6;
			for ( int32_t q = 0; q < numtrigs; q++ )
				if(!p_igetl(&temp_combo.triggerflags[q],f))
					return qe_invalid;
			if(!p_igetl(&temp_combo.triggerlevel,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.triggerbtn,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.triggeritem,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigtimer,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigsfx,f))
				return qe_invalid;
			if(!p_igetl(&temp_combo.trigchange,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.trigprox,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigctr,f))
				return qe_invalid;
			if(!p_igetl(&temp_combo.trigctramnt,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.triglbeam,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigcschange,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.spawnitem,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.spawnenemy,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.exstate,f))
				return qe_invalid;
			if(!p_igetl(&temp_combo.spawnip,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigcopycat,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.trigcooldown,f))
				return qe_invalid;
			if(s_version >= 35)
			{
				if(!p_igetw(&temp_combo.prompt_cid,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.prompt_cs,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.prompt_x,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.prompt_y,f))
					return qe_invalid;
			}
			if(s_version >= 36)
			{
				if(!p_getc(&temp_combo.trig_lstate,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.trig_gstate,f))
					return qe_invalid;
				if(!p_igetl(&temp_combo.trig_statetime,f))
					return qe_invalid;
			}
			if(s_version >= 37)
			{
				if(!p_igetw(&temp_combo.trig_genscr,f))
					return qe_invalid;
			}
			if(s_version >= 38)
			{
				if(!p_getc(&temp_combo.trig_group,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trig_group_val,f))
					return qe_invalid;
			}
			if(s_version >= 45)
			{
				if(!p_getc(&temp_combo.exdoor_dir,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.exdoor_ind,f))
					return qe_invalid;
			}
			if(s_version >= 46)
			{
				if(!p_getc(&temp_combo.trig_levelitems,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trigdmlevel,f))
					return qe_invalid;
				if(s_version >= 48)
				{
					for(int q = 0; q < 3; ++q)
						if(!p_igetw(&temp_combo.trigtint[q],f))
							return qe_invalid;
				}
				else
				{
					for(int q = 0; q < 3; ++q)
						if(!p_getc(&temp_combo.trigtint[q],f))
							return qe_invalid;
					for(int q = 0; q < 3; ++q)
					{
						int v = temp_combo.trigtint[q];
						int va = abs(v);
						temp_combo.trigtint[q] = _rgb_scale_6[va] * sign(v);
					}
				}
				if(!p_igetw(&temp_combo.triglvlpalette,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trigbosspalette,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trigquaketime,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trigwavytime,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trig_swjinxtime,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trig_itmjinxtime,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trig_stuntime,f))
					return qe_invalid;
				if(!p_igetw(&temp_combo.trig_bunnytime,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.trig_pushtime,f))
					return qe_invalid;
			}
			if(s_version >= 47)
			{
				if (!p_igetw(&temp_combo.trig_shieldjinxtime, f))
					return qe_invalid;
			}
		}
		if(combo_has_flags&CHAS_LIFT)
		{
			if(!p_igetw(&temp_combo.liftcmb,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftcs,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.liftundercmb,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftundercs,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftdmg,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftlvl,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftitm,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftflags,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftgfx,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftsprite,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftsfx,f))
				return qe_invalid;
			if(!p_igetw(&temp_combo.liftbreaksprite,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.liftbreaksfx,f))
				return qe_invalid;
			if(s_version >= 34)
			{
				if(!p_getc(&temp_combo.lifthei,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.lifttime,f))
					return qe_invalid;
			}
			if(s_version >= 39)
			{
				if(!p_getc(&temp_combo.lift_parent_item,f))
					return qe_invalid;
			}
		}
		if(combo_has_flags&CHAS_GENERAL)
		{
			if(!p_getc(&temp_combo.speed_mult,f))
				return qe_invalid;
			if(!p_getc(&temp_combo.speed_div,f))
				return qe_invalid;
			if(!p_igetzf(&temp_combo.speed_add,f))
				return qe_invalid;
			if(s_version >= 42)
			{
				if(!p_getc(&temp_combo.sfx_appear,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_disappear,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_loop,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_walking,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_standing,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_appear,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_disappear,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_walking,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_standing,f))
					return qe_invalid;
			}
			if(s_version >= 44)
			{
				if(!p_getc(&temp_combo.sfx_tap,f))
					return qe_invalid;
			}
			if(s_version >= 49)
			{
				if(!p_getc(&temp_combo.sfx_landing,f))
					return qe_invalid;
			}
			if(s_version >= 50)
			{
				if(!p_getc(&temp_combo.spr_falling,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_drowning,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.spr_lava_drowning,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_falling,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_drowning,f))
					return qe_invalid;
				if(!p_getc(&temp_combo.sfx_lava_drowning,f))
					return qe_invalid;
			}
		}
	}
	update_combo(temp_combo, s_version);
	return 0;
}
int32_t readcombos(PACKFILE *f, zquestheader *Header, word version, word build, word start_combo, word max_combos)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_combos);

	word section_version=0;
	word section_cversion=0;
	word combos_used=0;
	int32_t dummy;
	byte padding;
	newcombo temp_combo;
	
	if (!should_skip)
	{
		reset_combo_animations();
		reset_combo_animations2();
		init_combo_classes();

		for(int32_t q = start_combo; q < start_combo+max_combos; ++q)
			combobuf[q].clear();
	}
	
	if(version > 0x192) //Version info
	{
		if(!p_igetw(&section_version,f))
		{
			return qe_invalid;
		}
		FFCore.quest_format[vCombos] = section_version;
		if(!p_igetw(&section_cversion,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&dummy,f))
		{
			return qe_invalid;
		}
	}
	
	if(section_version > 32) //Cleanup time!
	{
		if(!p_igetw(&combos_used,f))
		{
			return qe_invalid;
		}
		for(int32_t i=0; i<combos_used; i++)
		{
			auto ret = readcombo_loop(f,section_version,temp_combo);
			if(ret) return ret;
			if(i>=start_combo)
			{
				if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
				{
					temp_combo.script = 0;
					for(int q = 0; q < 8; ++q)
						temp_combo.initd[q] = 0;
				}
				combobuf[i] = temp_combo;
			}
		}
	}
	else //Call the old function for all old versions
	{
		auto ret = readcombos_old(section_version,f,Header,version,build,start_combo,max_combos);
		if(ret) return ret; //error, end read
	}

	if (should_skip)
		return 0;

	if(!get_qr(qr_ALLOW_EDITING_COMBO_0))
	{
		combobuf[0].walk = 0xF0;
		combobuf[0].type = 0;
		combobuf[0].flag = 0;
	}
	
	setup_combo_animations();
	setup_combo_animations2();
	return 0;
}

int32_t readcomboaliases(PACKFILE *f, zquestheader *Header, word version, word build)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    version=version;
    build=build;
    
    int32_t dummy;
    word sversion=0, c_sversion;
    
    //section version info
    if(!p_igetw(&sversion,f))
    {
        return qe_invalid;
    }
    
    FFCore.quest_format[vComboAliases] = sversion;
    
    if(!p_igetw(&c_sversion,f))
    {
        return qe_invalid;
    }
    
    //section size
    if(!p_igetl(&dummy,f))
    {
        return qe_invalid;
    }
    
    int32_t max_num_combo_aliases = MAXCOMBOALIASES;
    
    if(sversion < 3) // max saved combo alias' upped from 256 to 2048.
    {
        max_num_combo_aliases = MAX250COMBOALIASES;
    }
    if(sversion < 2) // max saved combo alias' upped from 256 to 2048.
    {
        max_num_combo_aliases = OLDMAXCOMBOALIASES;
    }
    
    for(int32_t j=0; j<max_num_combo_aliases; j++)
    {
        byte width,height,mask,tempcset;
        int32_t count;
        word tempword;
        byte tempbyte;
        
        if(!p_igetw(&tempword,f))
        {
            return qe_invalid;
        }
        
		combo_aliases[j].combo = tempword;
        
        if(!p_getc(&tempbyte,f))
        {
            return qe_invalid;
        }
        
		combo_aliases[j].cset = tempbyte;
        
        if(!p_getc(&width,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&height,f))
        {
            return qe_invalid;
        }
        
        if(!p_getc(&mask,f))
        {
            return qe_invalid;
        }
        
        count=(width+1)*(height+1)*(comboa_lmasktotal(mask)+1);
        
		combo_aliases[j].width = width;
		combo_aliases[j].height = height;
		combo_aliases[j].layermask = mask;
		combo_aliases[j].combos.clear();
		combo_aliases[j].csets.clear();
        
        for(int32_t k=0; k<count; k++)
        {
            if(!p_igetw(&tempword,f))
            {
                return qe_invalid;
            }
            
			combo_aliases[j].combos[k] = tempword;
        }
        
        for(int32_t k=0; k<count; k++)
        {
            if(!p_getc(&tempcset,f))
            {
                return qe_invalid;
            }
            
			combo_aliases[j].csets[k] = tempcset;
        }
    }
    
	//Combo pools!
	word num_combo_pools = 0;
	if(sversion >= 4)
	{
		if(!p_igetw(&num_combo_pools,f))
		{
			return qe_invalid;
		}
	}
	
	for(combo_pool& pool : combo_pools)
	{
		pool.clear();
	}
	
	combo_pool temp_cpool;
	for(word cp = 0; cp < num_combo_pools; ++cp)
	{
		int32_t num_combos_in_pool = 0;
		if(!p_igetl(&num_combos_in_pool,f))
		{
			return qe_invalid;
		}
		if(num_combos_in_pool < 1) continue; //nothing to read
		
		temp_cpool.clear();
		
		int32_t cp_cid; int8_t cp_cs; word cp_quant;
		for(auto q = 0; q < num_combos_in_pool; ++q)
		{
			if(!p_igetl(&cp_cid,f))
			{
				return qe_invalid;
			}
			if(!p_getc(&cp_cs,f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&cp_quant,f))
			{
				return qe_invalid;
			}
			temp_cpool.add(cp_cid, cp_cs, cp_quant);
		}
		
		combo_pools[cp] = temp_cpool;
	}

	//Autocombos!
	word num_combo_autos = 0;
	if (sversion >= 5)
	{
		if (!p_igetw(&num_combo_autos, f))
		{
			return qe_invalid;
		}
	}

	for (combo_auto& cauto : combo_autos)
	{
		cauto.clear(true);
	}

	combo_auto temp_cauto;
	for (word ca = 0; ca < num_combo_autos; ++ca)
	{
		byte type;
		int32_t display_cid, erase_cid;
		byte flags, arg;
		if (!p_getc(&type, f))
		{
			return qe_invalid;
		}
		if (!p_igetl(&display_cid, f))
		{
			return qe_invalid;
		}
		if (!p_igetl(&erase_cid, f))
		{
			return qe_invalid;
		}
		if (!p_getc(&flags, f))
		{
			return qe_invalid;
		}
		if (!p_getc(&arg, f))
		{
			return qe_invalid;
		}
		int32_t num_combos_in_cauto = 0;
		if (!p_igetl(&num_combos_in_cauto, f))
		{
			return qe_invalid;
		}
		if (num_combos_in_cauto < 1) continue; //nothing to read

		temp_cauto.clear();

		temp_cauto.setType(type);
		temp_cauto.setDisplay(display_cid);
		temp_cauto.setEraseCombo(erase_cid);
		temp_cauto.setFlags(flags);
		temp_cauto.setArg(arg);

		int32_t ca_cid; byte ca_ctype;  int16_t ca_offset; int16_t ca_engrave_offset;
		for (auto q = 0; q < num_combos_in_cauto; ++q)
		{
			if (!p_getc(&ca_ctype, f))
			{
				return qe_invalid;
			}
			if (!p_igetl(&ca_cid, f))
			{
				return qe_invalid;
			}
			temp_cauto.addEntry(ca_cid, ca_ctype, q, -1);
		}

		combo_autos[ca] = temp_cauto;
	}
	
    return 0;
}

int32_t readcolordata(PACKFILE *f, miscQdata *Misc, word version, word build, word start_cset, word max_csets)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_colors);

	//these are here to bypass compiler warnings about unused arguments
	//THE *48 REFERS TO EACH CSET BEING 16 COLORS with 3 VALUES OF RGB (3*16 is 48)
	//Capitalized cause it'll save you a headache. -Deedee
	start_cset=start_cset;
	max_csets=max_csets;
	word s_version=0;
	
	miscQdata temp_misc = *Misc;
	
	byte temp_colordata[48];
	char temp_palname[PALNAMESIZE+1];
	
	int32_t dummy;
	word palcycles;
	
	if(version > 0x192)
	{
		//section version info
		if(!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}
	
		FFCore.quest_format[vCSets] = s_version;
		
		if(!p_igetw(&dummy,f))
		{
			return qe_invalid;
		}
		
		//section size
		if(!p_igetl(&dummy,f))
		{
			return qe_invalid;
		}
	}
	if (s_version < 5)
	{
		bool RealOldVerion = ((version < 0x192)||((version == 0x192)&&(build<73)));
		
		//finally...  section data
		int32_t q = 0;
		int32_t p = -15;
		for(int32_t i=0; i<oldpdTOTAL; ++i)
		{
			memset(temp_colordata, 0, 48);
			
			if(!pfread(temp_colordata,48,f))
			{
				return qe_invalid;
			}

			if (should_skip)
				continue;

			memcpy(&colordata[q*48], temp_colordata, 48);

			++q;
			if (p > 0 && (p%13)==12 && (i < oldpoSPRITE || !RealOldVerion)) //It's > 0 instead of >= 0 because it should append 
			{
				if (s_version < 5) //Bumping up the size of level palettes
				{
					memcpy(&colordata[(q)*48], &colordata[1*48], 48);
					memcpy(&colordata[(q+1)*48], &colordata[5*48], 48);
					memcpy(&colordata[(q+2)*48], &colordata[7*48], 48);
					memcpy(&colordata[(q+3)*48], &colordata[8*48], 48);
					q+=4;
				}
				else
				{
					for(int m = 0; m < 4; ++m)
					{
						memset(temp_colordata, 0, 48);
						if(!pfread(temp_colordata,48,f))
						{
							return qe_invalid;
						}
						memcpy(&colordata[q*48], temp_colordata, 48);
						++q;
					}
				}
			}
			++p;
		}
		
		if(RealOldVerion)
		{
			if (!should_skip)
			{
				memcpy(colordata+(poSPRITE255*48), colordata+((q-30)*48), 30*16*3);
				memset(colordata+((q-30)*48), 0, ((poSPRITE255-(q-30))*48));
				memcpy(colordata+((poSPRITE255+11)*48), colordata+((poSPRITE255+10)*48), 48);
				memcpy(colordata+((poSPRITE255+10)*48), colordata+((poSPRITE255+9)*48), 48);
				memcpy(colordata+((poSPRITE255+9)*48), colordata+((poSPRITE255+8)*48), 48);
				memset(colordata+((poSPRITE255+8)*48), 0, 48);
			}
		}
		else
		{
			memset(temp_colordata, 0, 48);
			
			for(int32_t i=0; i<newpdTOTAL-oldpdTOTAL; ++i)
			{
				if(!pfread(temp_colordata,48,f))
				{
					return qe_invalid;
				}

				if (should_skip)
					continue;

				memcpy(&colordata[q*48], temp_colordata, 48);

				++q;
				if (p > 0 && (p%13)==12 && (i < (newpoSPRITE-oldpdTOTAL) || (s_version >= 4))) //It's > 0 instead of >= 0 because it should append 
				{
					if (s_version < 5) //Bumping up the size of level palettes
					{
						memcpy(&colordata[(q)*48], &colordata[1*48], 48);
						memcpy(&colordata[(q+1)*48], &colordata[5*48], 48);
						memcpy(&colordata[(q+2)*48], &colordata[7*48], 48);
						memcpy(&colordata[(q+3)*48], &colordata[8*48], 48);
						q+=4;
					}
					else
					{
						for(int m = 0; m < 4; ++m)
						{
							memset(temp_colordata, 0, 48);
							if(!pfread(temp_colordata,48,f))
							{
								return qe_invalid;
							}
							memcpy(&colordata[q*48], temp_colordata, 48);
							++q;
						}
					}
				}
				++p;
			}
			
			if(s_version < 4)
			{
				if (!should_skip)
				{
					memcpy(colordata+(poSPRITE255*48), colordata+((q-30)*48), 30*16*3);
					memset(colordata+((q-30)*48), 0, ((poSPRITE255-(q-30))*48));
				}
			}
			else
			{
				for(int32_t i=0; i<newerpdTOTAL-newpdTOTAL; ++i)
				{
					if(!pfread(temp_colordata,48,f))
					{
						return qe_invalid;
					}

					if (should_skip)
						continue;
					
					memcpy(&colordata[q*48], temp_colordata, 48);
					++q;
					if (p > 0 && (p%13)==12 && i < newerpoSPRITE-newpdTOTAL) //It's > 0 instead of >= 0 because it should append 
					{
						if (s_version < 5) //Bumping up the size of level palettes
						{
							memcpy(&colordata[(q)*48], &colordata[1*48], 48);
							memcpy(&colordata[(q+1)*48], &colordata[5*48], 48);
							memcpy(&colordata[(q+2)*48], &colordata[7*48], 48);
							memcpy(&colordata[(q+3)*48], &colordata[8*48], 48);
							q+=4;
						}
						else
						{
							for(int m = 0; m < 4; ++m)
							{
								memset(temp_colordata, 0, 48);
								if(!pfread(temp_colordata,48,f))
								{
									return qe_invalid;
								}
								memcpy(&colordata[q*48], temp_colordata, 48);
								++q;
							}
						}
					}
					++p;
				}
				
				//By this point, q should be about equal to pdTOTAL255. If it isn't, I've fucked up. -Deedee
			}
		}
	}
	else
	{
		for(int32_t i=0; i<pdTOTAL255; ++i)
		{
			memset(temp_colordata, 0, 48);
			
			if(!pfread(temp_colordata,48,f))
			{
				return qe_invalid;
			}
			
			memcpy(&colordata[i*48], temp_colordata, 48);
		}
	}

	if (!should_skip && s_version < 6)
	{
		for (int i = 0; i < psTOTAL255; i++)
		{
			colordata[i] = _rgb_scale_6[colordata[i]];
		}
	}

	if((version < 0x192)||((version == 0x192)&&(build<76)))
	{
		if (!should_skip)
			init_palnames();
	}
	else
	{
		int32_t palnamestoread = 0;
		
		if(s_version < 3)
			palnamestoread = OLDMAXLEVELS;
		else
			palnamestoread = 512;
			
		for(int32_t i=0; i<palnamestoread; ++i)
		{
			if(!p_getstr(temp_palname,PALNAMESIZE,f))
			{
				return qe_invalid;
			}

			if (!should_skip)
				memcpy(palnames[i], temp_palname, PALNAMESIZE);
		}

		if (should_skip)
			return 0;
		
		for(int32_t i=palnamestoread; i<MAXLEVELS; i++)
		{
			memset(palnames[i], 0, PALNAMESIZE);
		}
	}
	
	if(version > 0x192)
	{
		for(int32_t i=0; i<256; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				temp_misc.cycles[i][j].first=0;
				temp_misc.cycles[i][j].count=0;
				temp_misc.cycles[i][j].speed=0;
			}
		}
		
		if(!p_igetw(&palcycles,f))
		{
			return qe_invalid;
		}

		if (palcycles > NUM_PAL_CYCLES)
		{
			return qe_invalid;
		}
		
		for(int32_t i=0; i<palcycles; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				if(!p_getc(&temp_misc.cycles[i][j].first,f))
				{
					return qe_invalid;
				}
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_getc(&temp_misc.cycles[i][j].count,f))
				{
					return qe_invalid;
				}
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_getc(&temp_misc.cycles[i][j].speed,f))
				{
					return qe_invalid;
				}
			}
		}
		
		*Misc = temp_misc;
	}
	
	return 0;
}

int32_t readtiles(PACKFILE *f, tiledata *buf, zquestheader *Header, word version, word build, word start_tile, int32_t max_tiles, bool from_init)
{
    bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_tiles);

    int32_t tiles_used=0;
	word section_version = 0;
	word section_cversion = 0;
	int32_t section_size= 0;
    byte *temp_tile = new byte[tilesize(tf32Bit)];
	
    //Tile Expansion
    //if ( version >= 0x254 && build >= 41 )
    if (version < 0x254 && build < 41)
    {
	    max_tiles = ZC250MAXTILES;
    }
    
	
    if(Header!=NULL&&(!Header->data_flags[ZQ_TILES]&&!from_init))         //keep for old quests
    {
		if(!init_tiles_for_190(true, Header))
		{
			al_trace("Unable to initialize tiles\n");
		}
        
        delete[] temp_tile;
        temp_tile=NULL;
        return 0;
    }
    else
    {
        if(version > 0x192)
        {
            //section version info
            if(!p_igetw(&section_version,f))
            {
                delete[] temp_tile;
                return qe_invalid;
            }
            
			FFCore.quest_format[vTiles] = section_version;
			
            if(!p_igetw(&section_cversion,f))
            {
                delete[] temp_tile;
                return qe_invalid;
            }
            
            //section size
            if(!p_igetl(&section_size,f))
            {
                delete[] temp_tile;
                return qe_invalid;
            }
        }
        
		//if ( build < 41 ) 
		//{
		//	tiles_used = ZC250MAXTILES;
		//}
		
        if(version < 0x174)
        {
            tiles_used=TILES_PER_PAGE*4;
        }                                                       //no expanded tile space
        else if(version < 0x191)
        {
            tiles_used=OLDMAXTILES;
        }
        else
        {
            //finally...  section data
            if ( version >= 0x254 && build >= 41 ) //read and write the size of tiles_used properly
			{ 
				if(!p_igetl(&tiles_used,f))
				{
					delete[] temp_tile;
					return qe_invalid;
				}
			}
			else
			{
				if(!p_igetw(&tiles_used,f))
				{
					delete[] temp_tile;
					return qe_invalid;
				}
			}
        }
        
        tiles_used=zc_min(tiles_used, max_tiles);
        
		//if ( version < 0x254 || ( version >= 0x254 && build < 41 )) //don't do this, it crashes ZQuest. -Z
		//if ( version < 0x254 && build < 41 )
		if ( version < 0x254 || (version == 0x254 && build < 41) )
		//if ( build < 41 )
		{
			tiles_used=zc_min(tiles_used, ZC250MAXTILES-start_tile);
		}
		else //2.55
		{
			tiles_used = zc_min(tiles_used,NEWMAXTILES-start_tile); 
		}
		
		//if ( section_version > 1 ) tiles_used = NEWMAXTILES;
			
	
        for(int32_t i=0; i<tiles_used; ++i)
        {
            byte format=tf4Bit;
            memset(temp_tile, 0, tilesize(tf32Bit));
            
            if((version>0x211)||((version==0x211)&&(build>4)))
            {
                if(!p_getc(&format,f))
                {
                    delete[] temp_tile;
                    return qe_invalid;
                }
            }
            if(section_version > 2 && !format)
			{
				reset_tile(buf,start_tile+i,tf4Bit);
				continue;
			}
			
			int size = format == tf4Bit ? 128 : tilesize(format);
            if(!pfread(temp_tile,size,f))
            {
                delete[] temp_tile;
                return qe_invalid;
            }

			if (should_skip)
				continue;

			buf[start_tile+i].format=format;
			
			if(buf[start_tile+i].data)
			{
				free(buf[start_tile+i].data);
				buf[start_tile+i].data=NULL;
			}

			buf[start_tile+i].data=(byte *)malloc(tilesize(buf[start_tile+i].format));

			if (format == tf4Bit)
			{
				byte temp[256];
				byte *si = temp_tile + 128;
				byte *di = temp + 256;
				
				for(int i=127; i>=0; --i)
				{
					(*(--di)) = (*(--si)) >> 4;
					(*(--di)) = (*si) & 15;
				}

				memcpy(buf[start_tile+i].data,temp,256);
			}
			else
			{
				memcpy(buf[start_tile+i].data,temp_tile,tilesize(buf[start_tile+i].format));
			}
        }
    }

	if (should_skip)
		return 0;
    
	if ( section_version < 2 ) //write blank tile data --check s_version with this again instead?
	{
		for ( int32_t q = ZC250MAXTILES; q < NEWMAXTILES; ++q )
		{
			
			//memcpy(buf[q].data,temp_tile,tilesize(buf[q].format));
			reset_tile(buf,q,tf4Bit);
			
			
			/*
			
			byte tempbyte;
			for(int32_t i=0; i<tilesize(tf4Bit); i++)
			{
				tempbyte=buf[ZC250MAXTILES-1].data[i];
				buf[q].data[i] = tempbyte;
			}
			//int32_t temp = tempbyte=buf[130].data[i];
			//buf[q].data = buf[ZC250MAXTILES-1].data;
			*/
			//reset_tile(buf,q,tf4Bit);
		}
		
	}
    
	if ( version < 0x254 || ( version >= 0x254 && build < 41 ))
	{
		for(int32_t i=start_tile+tiles_used; i<max_tiles; ++i)
		{
			reset_tile(buf,i,tf4Bit);
		}
	}
	else
	{
		for(int32_t i=start_tile+tiles_used; i<max_tiles; ++i)
		{
			reset_tile(buf,i,tf4Bit);
		}
	}
	
	if((version < 0x192)|| ((version == 0x192)&&(build<186)))
	{
		if(get_qr(qr_BSZELDA))   //
		{
			byte tempbyte;
			int32_t floattile=wpnsbuf[iwSwim].tile;
			
			for(int32_t i=0; i<tilesize(tf4Bit); i++)  //BSZelda tiles are out of order //does this include swim tiles?
			{
				tempbyte=buf[23].data[i];
				buf[23].data[i]=buf[24].data[i];
				buf[24].data[i]=buf[25].data[i];
				buf[25].data[i]=buf[26].data[i];
				buf[26].data[i]=tempbyte;
			}
			//swim tiles are out of order, too, but nobody cared? -Z 
			for(int32_t i=0; i<tilesize(tf4Bit); i++)
			{
				tempbyte=buf[floattile+11].data[i];
				buf[floattile+11].data[i]=buf[floattile+12].data[i];
				buf[floattile+12].data[i]=tempbyte;
			}
		}
	}
	
	if((version < 0x211)||((version == 0x211)&&(build<7)))   //Goriya tiles are out of order
	{
		if(!get_qr(qr_NEWENEMYTILES))
		{
			byte tempbyte;
			
			for(int32_t i=0; i<tilesize(tf4Bit); i++)
			{
				tempbyte=buf[130].data[i];
				buf[130].data[i]=buf[132].data[i];
				buf[132].data[i]=tempbyte;
				
				tempbyte=buf[131].data[i];
				buf[131].data[i]=buf[133].data[i];
				buf[133].data[i]=tempbyte;
			}
		}
	}
	
	al_trace("Registering blank tiles\n");
	register_blank_tiles();
    
    //memset(temp_tile, 0, tilesize(tf32Bit));
    delete[] temp_tile;
    temp_tile=NULL;
    return 0;
}

int32_t readtunes(PACKFILE *f, zquestheader *Header, zctune *tunes /*zcmidi_ *midis*/)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_midis);

	static byte fake_midi_flags[32];

    byte *mf=should_skip ? fake_midi_flags : midi_flags;
    int32_t dummy;
    word dummy2;
    // zcmidi_ temp_midi;
    int32_t tunes_to_read;
    int32_t tune_count=0;
    word section_version=0;
    zctune temp;
    
    if(Header->zelda_version < 0x193)
    {
        //    mf=Header->data_flags+ZQ_MIDIS2;
        if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<178)))
        {
            tunes_to_read=MAXCUSTOMMIDIS192b177;
        }
        else
        {
            tunes_to_read=MAXCUSTOMTUNES;
        }
    }
    else
    {
        //section version info
        if(!p_igetw(&section_version,f))
        {
            return qe_invalid;
        }
	
		if (!should_skip)
			FFCore.quest_format[vMIDIs] = section_version;
        
        if(!p_igetw(&dummy2,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy,f))
        {
            return qe_invalid;
        }
        
        //finally...  section data
        if(!pfread(midi_flags,sizeof(midi_flags),f))
        {
            return qe_invalid;
        }
        
        tunes_to_read=MAXCUSTOMTUNES;
    }
    
    for(int32_t i=0; i<MAXCUSTOMTUNES; ++i)
    {
        if(get_bit(mf, i))
        {
            ++tune_count;
        }
    }
    
	if (!should_skip)
		reset_tunes(tunes); //reset_midis(midis);
    
    for(int32_t i=0; i<tunes_to_read; i++)
    {
        temp.clear(); //memset(&temp_midi,0,sizeof(zcmidi_));
        
		if (!should_skip)
			tunes[i].reset(); // reset_midi(midis+i);
        
        if(get_bit(mf,i))
        {
            if(section_version < 4)
            {
                if(!p_getstr(temp.title,20,f))
                {
                    return qe_invalid;
                }
            }
            else
            {
                if(!p_getstr(temp.title,sizeof(temp.title)-1,f))
                {
                    return qe_invalid;
                }
            }
            
            if(!p_igetl(&temp.start,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetl(&temp.loop_start,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetl(&temp.loop_end,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&temp.loop,f))
            {
                return qe_invalid;
            }
            
            if(!p_igetw(&temp.volume,f))
            {
                return qe_invalid;
            }
            
            if(Header->zelda_version < 0x193)
            {
                if(!p_igetl(&dummy,f))
                {
                    return qe_invalid;
                }
            }
            
            if(section_version >= 3)
            {
                if(!pfread(&temp.flags,sizeof(temp.flags),f))
                {
                    return qe_invalid;
                }
            }
            
			if (!should_skip)
				tunes[i].copyfrom(temp); // memcpy(&midis[i], &temp_midi, sizeof(zcmidi_));
            
            if(section_version < 2) //= 1 || (Header->zelda_version < 0x211) || (Header->zelda_version == 0x211 && Header->build < 18))
            {
				if (should_skip)
				{
					if (read_midi(f)==NULL)
					{
						return qe_invalid;
					}

					continue;
				}

                // old format - a midi is a midi
                if((tunes[i].data=read_midi(f))==NULL)
                {
                    return qe_invalid;
                }
            }
            else
            {
				byte format;
                if(!pfread(&format,sizeof(format),f))
                {
                    return qe_invalid;
                }

				// MIDI is the only format saved here.
				// Never did more than MIDI for a zctune, and no plans to now.
				if (format != MFORMAT_MIDI)
				{
					return qe_invalid;
				}

				tunes[i].data = read_midi(f);
                if (!tunes[i].data)
				{
					return qe_invalid;
				}
            }
        }
    }
    
    return 0;
}

int32_t readcheatcodes(PACKFILE *f, zquestheader *Header)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_cheats);

    int32_t dummy;
    ZCHEATS tempzcheats;
    char temp_use_cheats=1;
    memset(&tempzcheats, 0, sizeof(tempzcheats));
    word s_version = 0;
    
    if(Header->zelda_version > 0x192)
    {
        //section version info
        if(!p_igetw(&s_version,f))
        {
            return qe_invalid;
        }
        
	FFCore.quest_format[vCheats] = s_version;
        if(!p_igetw(&dummy,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy,f))
        {
            return qe_invalid;
        }
        
        //finally...  section data
        if(!p_getc(&temp_use_cheats,f))
        {
            return qe_invalid;
        }
    }
    
    if(Header->data_flags[ZQ_CHEATS2])
    {
        if(!p_igetl(&tempzcheats.flags,f))
        {
            return qe_invalid;
        }
        
        if(!pfread(&tempzcheats.codes, sizeof(tempzcheats.codes),f))
        {
            return qe_invalid;
        }
    }
    
	if (should_skip)
		return 0;

	memcpy(&zcheats, &tempzcheats, sizeof(tempzcheats));
	Header->data_flags[ZQ_CHEATS2]=temp_use_cheats;
    
    return 0;
}

int32_t readinitdata_old(PACKFILE *f, zquestheader *Header, word s_version, word s_cversion, zinitdata& temp_zinit)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_initdata);

	int32_t dummy;
	byte padding, tempbyte;
	
	// Legacy item properties (now integrated into itemdata)
	byte sword_hearts[4];
	byte beam_hearts[4];
	byte beam_percent=0;
	word beam_power[4];
	byte hookshot_length=99;
	byte hookshot_links=100;
	byte longshot_length=99;
	byte longshot_links=100;
	byte moving_fairy_hearts=3;
	byte moving_fairy_heart_percent=0;
	byte stationary_fairy_hearts=3;
	byte stationary_fairy_heart_percent=0;
	byte moving_fairy_magic=0;
	byte moving_fairy_magic_percent=0;
	byte stationary_fairy_magic=0;
	byte stationary_fairy_magic_percent=0;
	byte blue_potion_hearts=100;
	byte blue_potion_heart_percent=1;
	byte red_potion_hearts=100;
	byte red_potion_heart_percent=1;
	byte blue_potion_magic=100;
	byte blue_potion_magic_percent=1;
	byte red_potion_magic=100;
	byte red_potion_magic_percent=1;
	
	byte bomb_ratio = 4;
	
	subscr_mode = 0;
	
	/* HIGHLY UNORTHODOX UPDATING THING, by L
	 * This fixes quests made before revision 277 (such as the 'Lost Isle Build'),
	 * where the speed of Pols Voice changed. It also coincided with V_INITDATA
	 * changing from 13 to 14.
	 */
	if(s_version < 14)
		fixpolsvoice=true;
		
	/* End highly unorthodox updating thing */
	
	if(s_version >= 15 && get_bit(deprecated_rules, 27)) // The int16_t-lived rule, qr_JUMPHEROLAYER3
		temp_zinit.jump_hero_layer_threshold=0;
		
	if(s_version >= 10)
	{
		char temp;
		
		//new-style items
		for(int32_t j=0; j<256; j++)
		{
			if(!p_getc(&temp,f))
				return qe_invalid;
				
			temp_zinit.set_item(j, temp != 0);
		}
	}
	
	if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>26)))
	{
		char temp;
		
		//finally...  section data
		if((Header->zelda_version > 0x192)||
				//new only
				((Header->zelda_version == 0x192)&&(Header->build>173)))
		{
			//OLD-style items... sigh
			if(s_version < 10)
			{
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iRaft, temp != 0);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iLadder, temp != 0);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iBook, temp != 0);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iMKey, temp != 0);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iFlippers, temp != 0);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.set_item(iBoots, temp != 0);
			}
		}
		
		if(s_version < 10)
		{
			char tempring, tempsword, tempshield, tempwallet, tempbracelet, tempamulet, tempbow;
			
			if(!p_getc(&tempring,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempsword,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempshield,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempwallet,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempbracelet,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempamulet,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempbow,f))
			{
				return qe_invalid;
			}
			
			//old only
			if((Header->zelda_version == 0x192)&&(Header->build<174))
			{
				tempring=(tempring)?(1<<(tempring-1)):0;
				tempsword=(tempsword)?(1<<(tempsword-1)):0;
				tempshield=(tempshield)?(1<<(tempshield-1)):0;
				tempwallet=(tempwallet)?(1<<(tempwallet-1)):0;
				tempbracelet=(tempbracelet)?(1<<(tempbracelet-1)):0;
				tempamulet=(tempamulet)?(1<<(tempamulet-1)):0;
				tempbow=(tempbow)?(1<<(tempbow-1)):0;
			}
			
			//rings start at level 2... wtf
			//account for this -DD
			tempring <<= 1;
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_ring, tempring);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_sword, tempsword);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_shield, tempshield);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_wallet, tempwallet);
			//bracelet ALSO starts at level 2 :-( -DD
			tempbracelet<<=1;
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_bracelet, tempbracelet);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_amulet, tempamulet);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_bow, tempbow);
			
			//new only
			if((Header->zelda_version == 0x192)&&(Header->build>173))
			{
				for(int32_t q=0; q<32; q++)
				{
					if(!p_getc(&padding,f))
					{
						return qe_invalid;
					}
				}
			}
			
			char tempcandle, tempboomerang, temparrow, tempwhistle;
			
			if(!p_getc(&tempcandle,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&tempboomerang,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&temparrow,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&temp,f))
			{
				return qe_invalid;
			}
			
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_potion, temp);
			
			if(!p_getc(&tempwhistle,f))
			{
				return qe_invalid;
			}
			
			//old only
			if((Header->zelda_version == 0x192)&&(Header->build<174))
			{
				tempcandle=(tempcandle)?(1<<(tempcandle-1)):0;
				tempboomerang=(tempboomerang)?(1<<(tempboomerang-1)):0;
				temparrow=(temparrow)?(1<<(temparrow-1)):0;
				tempwhistle=(tempwhistle)?(1<<(tempwhistle-1)):0;
			}
			
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_candle, tempcandle);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_brang, tempboomerang);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_arrow, temparrow);
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_whistle, tempwhistle);
			//What about the potion...?
			
		}
		
		if(s_version < 29)
		{
			//Oh sure, stick these IN THE MIDDLE OF THE ITEMS, just to make me want
			//to jab out my eye...
			if(!p_getc(&padding,f))
				return qe_invalid;
			temp_zinit.counter[crBOMBS] = padding;
			
			if(!p_getc(&padding,f))
				return qe_invalid;
			temp_zinit.counter[crSBOMBS] = padding;
		}
		
		//Back to more OLD item code
		if(s_version < 10)
		{
			if((Header->zelda_version > 0x192)||
					//new only
					((Header->zelda_version == 0x192)&&(Header->build>173)))
			{
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_wand, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_letter, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_lens, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_hookshot, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_bait, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_hammer, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_divinefire, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_divineescape, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				addOldStyleFamily(&temp_zinit, itemsbuf, itype_divineprotection, temp);
				
				if(!p_getc(&temp,f))
				{
					return qe_invalid;
				}
				
				if(Header->zelda_version == 0x192)
				{
					for(int32_t q=0; q<32; q++)
					{
						if(!p_getc(&padding,f))
						{
							return qe_invalid;
						}
					}
				}
			}
		}
		
		//old only
		if((Header->zelda_version == 0x192)&&(Header->build<174))
		{
			byte equipment, tmpitm;                                //bit flags
			
			if(!p_getc(&equipment,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.set_item(iRaft, get_bit(&equipment, idE_RAFT)!=0);
			temp_zinit.set_item(iLadder, get_bit(&equipment, idE_LADDER)!=0);
			temp_zinit.set_item(iBook, get_bit(&equipment, idE_BOOK)!=0);
			temp_zinit.set_item(iMKey, get_bit(&equipment, idE_KEY)!=0);
			temp_zinit.set_item(iFlippers, get_bit(&equipment, idE_FLIPPERS)!=0);
			temp_zinit.set_item(iBoots, get_bit(&equipment, idE_BOOTS)!=0);
			
			
			if(!p_getc(&tmpitm,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.set_item(iWand, get_bit(&tmpitm, idI_WAND)!=0);
			temp_zinit.set_item(iLetter, get_bit(&tmpitm, idI_LETTER)!=0);
			temp_zinit.set_item(iLens, get_bit(&tmpitm, idI_LENS)!=0);
			temp_zinit.set_item(iHookshot, get_bit(&tmpitm, idI_HOOKSHOT)!=0);
			temp_zinit.set_item(iBait, get_bit(&tmpitm, idI_BAIT)!=0);
			temp_zinit.set_item(iHammer, get_bit(&tmpitm, idI_HAMMER)!=0);
		}
		
		if(!p_getc(&tempbyte,f))
			return qe_invalid;
		temp_zinit.mcounter[crLIFE] = tempbyte;
		
		
		if(s_version < 14)
		{
			byte temphp;
			
			if(!p_getc(&temphp,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.counter[crLIFE]=temphp;
			
			if(!p_getc(&temphp,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.cont_heart=temphp;
		}
		else
		{
			if(!p_igetw(&temp_zinit.counter[crLIFE],f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.cont_heart,f))
			{
				return qe_invalid;
			}
		}
		
		if(!p_getc(&temp_zinit.hcp,f))
		{
			return qe_invalid;
		}
		
		if(s_version >= 14)
		{
			if(!p_getc(&temp_zinit.hcp_per_hc,f))
			{
				return qe_invalid;
			}
			
			if(s_version<16)  // July 2007
			{
				if(get_qr(qr_BRANGPICKUP+1))
					temp_zinit.hcp_per_hc = 0xFF;
					
				//Dispose of legacy rule
				set_qr(qr_BRANGPICKUP+1, 0);
			}
		}
		
		if(s_version < 29)
		{
			if(!p_getc(&padding,f))
				return qe_invalid;
			temp_zinit.mcounter[crBOMBS] = padding;
		}
		
		if(!p_getc(&temp_zinit.counter[crKEYS],f))
		{
			return qe_invalid;
		}
		
		if(!p_igetw(&temp_zinit.counter[crMONEY],f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&tempbyte,f))
			return qe_invalid;
		for(int q = 0; q < 8; ++q)
			set_bit(temp_zinit.mcguffin, q+1, get_bitl(tempbyte, q));
		
		if(s_version>12 || (Header->zelda_version == 0x211 && Header->build == 18))
		{
			for(int32_t i=0; i<64; i++)
			{
				if(!p_getc(&temp_zinit.map[i],f))
				{
					return qe_invalid;
				}
			}
			
			for(int32_t i=0; i<64; i++)
			{
				if(!p_getc(&temp_zinit.compass[i],f))
				{
					return qe_invalid;
				}
			}
		}
		else
		{
			for(int32_t i=0; i<32; i++)
			{
				if(!p_getc(&temp_zinit.map[i],f))
				{
					return qe_invalid;
				}
			}
			
			for(int32_t i=0; i<32; i++)
			{
				if(!p_getc(&temp_zinit.compass[i],f))
				{
					return qe_invalid;
				}
			}
		}
		
		if((Header->zelda_version > 0x192)||
				//new only
				((Header->zelda_version == 0x192)&&(Header->build>173)))
		{
			if(s_version>12 || (Header->zelda_version == 0x211 && Header->build == 18))
			{
				for(int32_t i=0; i<64; i++)
				{
					if(!p_getc(&temp_zinit.boss_key[i],f))
					{
						return qe_invalid;
					}
				}
			}
			else
			{
				for(int32_t i=0; i<32; i++)
				{
					if(!p_getc(&temp_zinit.boss_key[i],f))
					{
						return qe_invalid;
					}
				}
			}
		}
		
		byte tmpmisc[16];
		for(int32_t i=0; i<16; i++)
			if(!p_getc(&tmpmisc[i],f))
				return qe_invalid;
		temp_zinit.flags.set(INIT_FL_CONTPERCENT,get_bit(tmpmisc,0));
		temp_zinit.magicdrainrate = get_bit(tmpmisc,1) ? 1 : 2; //Double Magic flag
		temp_zinit.flags.set(INIT_FL_CANSLASH,get_bit(tmpmisc,2));
		
		if(s_version < 15) for(int32_t i=0; i<4; i++)
			if(!p_getc(&sword_hearts[i],f))
				return qe_invalid;
			
		if(!p_getc(&temp_zinit.last_map,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_zinit.last_screen,f))
		{
			return qe_invalid;
		}
		
		if(s_version < 14)
		{
			byte tempmp;
			
			if(!p_getc(&tempmp,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.mcounter[crMAGIC]=tempmp;
			
			if(!p_getc(&tempmp,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.counter[crMAGIC]=tempmp;
		}
		else
		{
			if(!p_igetw(&temp_zinit.mcounter[crMAGIC],f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.counter[crMAGIC],f))
			{
				return qe_invalid;
			}
		}
		
		
		if(s_version < 15)
		{
			if(s_version < 12)
			{
				temp_zinit.mcounter[crMAGIC]*=32;
				temp_zinit.counter[crMAGIC]*=32;
			}
			
			for(int32_t i=0; i<4; i++)
			{
				if(!p_getc(&beam_hearts[i],f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getc(&beam_percent,f))
			{
				return qe_invalid;
			}
		}
		else
		{
			if(!p_getc(&temp_zinit.bomb_ratio,f))
				return qe_invalid;
			if(temp_zinit.bomb_ratio < 1)
				temp_zinit.bomb_ratio = 1;
			else bomb_ratio = temp_zinit.bomb_ratio; //jank
		}
		
		if(s_version < 15)
		{
			byte tempbp;
			
			for(int32_t i=0; i<4; i++)
			{
				if(!(s_version < 14 ? p_getc(&tempbp,f) : p_igetw(&tempbp,f)))
				{
					return qe_invalid;
				}
				
				beam_power[i]=tempbp;
			}
			
			if(!p_getc(&hookshot_links,f))
			{
				return qe_invalid;
			}
			
			if(s_version>6)
			{
				if(!p_getc(&hookshot_length,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&longshot_links,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&longshot_length,f))
				{
					return qe_invalid;
				}
			}
		}
		
		if(!p_getc(&temp_zinit.msg_more_x,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&temp_zinit.msg_more_y,f))
		{
			return qe_invalid;
		}
		
		if(!p_getc(&subscr_mode,f))
			return qe_invalid;
		
		//old only
		if((Header->zelda_version == 0x192)&&(Header->build<174))
		{
			for(int32_t i=0; i<32; i++)
			{
				if(!p_getc(&temp_zinit.boss_key[i],f))
				{
					return qe_invalid;
				}
			}
		}
		
		if((Header->zelda_version > 0x192)||((Header->zelda_version == 0x192)&&(Header->build>173)))  //new only
		{
			if(s_version <= 10)
			{
				if(!p_getc(&tempbyte,f))
				{
					return qe_invalid;
				}
				
				temp_zinit.start_dmap = (word)tempbyte;
			}
			else
			{
				if(!p_igetw(&temp_zinit.start_dmap,f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_getc(&temp_zinit.heroAnimationStyle,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>1 && s_version < 29)
		{
			if(!p_getc(&padding,f))
				return qe_invalid;
			temp_zinit.counter[crARROWS] = padding;
			
			if(!p_getc(&padding,f))
				return qe_invalid;
			temp_zinit.mcounter[crARROWS] = padding;
		}
		
		if(s_version>2)
		{
			if(s_version <= 10)
			{
				for(int32_t i=0; i<OLDMAXLEVELS; i++)
				{
					if(!p_getc(&(temp_zinit.level_keys[i]),f))
					{
						return qe_invalid;
					}
				}
			}
			else
			{
				for(int32_t i=0; i<MAXLEVELS; i++)
				{
					if(!p_getc(&(temp_zinit.level_keys[i]),f))
					{
						return qe_invalid;
					}
				}
			}
		}
		
		if(s_version>3)
		{
			if(!p_igetw(&temp_zinit.ss_grid_x,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_grid_y,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_grid_xofs,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_grid_yofs,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_grid_color,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_bbox_1_color,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_bbox_2_color,f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.ss_flags,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.ss_grid_x=zc_max(temp_zinit.ss_grid_x,1);
			temp_zinit.ss_grid_y=zc_max(temp_zinit.ss_grid_y,1);
		}
		
		if(s_version>4 && s_version<15)
		{
			if(!p_getc(&moving_fairy_hearts,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&moving_fairy_heart_percent,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>5 && s_version < 10)
		{
			if(!p_getc(&temp,f))
			{
				return qe_invalid;
			}
			
			addOldStyleFamily(&temp_zinit, itemsbuf, itype_quiver, temp);
		}
		
		if(s_version>6 && s_version<15)
		{
			if(!p_getc(&stationary_fairy_hearts,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&stationary_fairy_heart_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&moving_fairy_magic,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&moving_fairy_magic_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&stationary_fairy_magic,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&stationary_fairy_magic_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&blue_potion_hearts,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&blue_potion_heart_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&red_potion_hearts,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&red_potion_heart_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&blue_potion_magic,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&blue_potion_magic_percent,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&red_potion_magic,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&red_potion_magic_percent,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>6)
			if(!p_getc(&padding,f))
				return qe_invalid;
		
		if(s_version>7)
		{
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>8)
		{
			if(!p_igetw(&temp_zinit.mcounter[crMONEY],f))
			{
				return qe_invalid;
			}
			
			if(!p_igetw(&temp_zinit.mcounter[crKEYS],f))
			{
				return qe_invalid;
			}
		}
		
		if(s_version>16)
		{
			if(!p_getc(&tempbyte,f))
			{
				return qe_invalid;
			}
			temp_zinit.gravity = tempbyte*100;
			if(!p_igetw(&temp_zinit.terminalv,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&temp_zinit.msg_speed,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&padding,f))
			{
				return qe_invalid;
			}
			
			if(!p_getc(&temp_zinit.jump_hero_layer_threshold,f))
			{
				return qe_invalid;
			}
		}
		else if (replay_version_check(0, 13))
			temp_zinit.msg_speed = 0;
		
		if(s_version>17)
		{
			if(!p_getc(&temp_zinit.msg_more_is_offset,f))
			{
				return qe_invalid;
			}
		}
		
		//expaned init data for larger values in 2.55
		if ( s_version >= 19 ) //expand init data bombs, sbombs, and arrows to 0xFFFF
		{
			if(!p_igetw(&temp_zinit.counter[crBOMBS],f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&temp_zinit.counter[crSBOMBS],f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&temp_zinit.mcounter[crBOMBS],f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&temp_zinit.mcounter[crSBOMBS],f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&temp_zinit.counter[crARROWS],f))
			{
				return qe_invalid;
			}
			if(!p_igetw(&temp_zinit.mcounter[crARROWS],f))
			{
				return qe_invalid;
			}
			
		}
		if ( s_version >= 20 )
		{
			if(!p_igetw(&temp_zinit.heroStep,f))
			{
				return qe_invalid;
			}
		}
		else
		{	
			temp_zinit.heroStep = 150; //1.5 pixels per frame
		}
		if ( s_version >= 21 )
		{
			if(!p_igetw(&temp_zinit.subscrSpeed,f))
			{
				return qe_invalid;
			}
		}
		else
		{	
			temp_zinit.subscrSpeed = 1; //3 pixels per frame
		}
		//old only
		if((Header->zelda_version == 0x192)&&(Header->build<174))
		{
			byte items2;
			
			if(!p_getc(&items2,f))
			{
				return qe_invalid;
			}
			
			temp_zinit.set_item(iDivineFire, get_bit(&items2, idI_DFIRE)!=0);
			temp_zinit.set_item(iDivineEscape, get_bit(&items2, idI_FWIND)!=0);
			temp_zinit.set_item(iDivineProtection, get_bit(&items2, idI_NLOVE)!=0);
		}
		
		if(Header->zelda_version < 0x193)
		{
			for(int32_t q=0; q<96; q++)
			{
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
			
			//new only
			if((Header->zelda_version == 0x192)&&(Header->build>173))
			{
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
				
				if(!p_getc(&padding,f))
				{
					return qe_invalid;
				}
			}
		}
	}
	
	if((Header->zelda_version < 0x211)||((Header->zelda_version == 0x211)&&(Header->build<15)))
	{
		//temp_zinit.shield=i_smallshield;
		int32_t sshieldid = getItemID(itemsbuf, itype_shield, i_smallshield);
		
		if(sshieldid != -1)
			temp_zinit.set_item(sshieldid, true);
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<27)))
	{
		temp_zinit.mcounter[crLIFE]=3;
		temp_zinit.counter[crLIFE]=3;
		temp_zinit.cont_heart=3;
		temp_zinit.mcounter[crBOMBS]=8;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<50)))
	{
		sword_hearts[0]=0;
		sword_hearts[1]=5;
		sword_hearts[2]=12;
		sword_hearts[3]=21;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<51)))
	{
		temp_zinit.last_map=0;
		temp_zinit.last_screen=0;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<68)))
	{
		temp_zinit.mcounter[crMAGIC]=0;
		temp_zinit.counter[crMAGIC]=0;
		temp_zinit.magicdrainrate = 2;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<129)))
	{
	
		for(int32_t x=0; x<4; x++)
		{
			beam_hearts[x]=100;
		}
		
		for(int32_t i=0; i<idBP_MAX; i++)
		{
			set_bit(&beam_percent,i,!get_qr(qr_LENSHINTS+i));
			set_qr(qr_LENSHINTS+i,0);
		}
		
		for(int32_t x=0; x<4; x++)
		{
			beam_power[x]=get_qr(qr_HIDECARRIEDITEMS)?50:100;
		}
		
		set_qr(qr_HIDECARRIEDITEMS,0);
		hookshot_links=100;
		temp_zinit.msg_more_x=224;
		temp_zinit.msg_more_y=64;
	}
	
	// Okay,  let's put these legacy values into itemsbuf.
	if(s_version < 15)
		for(int32_t i=0; i<MAXITEMS; i++)
		{
			switch(i)
			{
				case iFairyStill:
					itemsbuf[i].misc1 = stationary_fairy_hearts;
					itemsbuf[i].misc2 = stationary_fairy_magic;
					itemsbuf[i].misc3 = 0;
					if (stationary_fairy_heart_percent) itemsbuf[i].flags |= item_flag1;
					if (stationary_fairy_magic_percent) itemsbuf[i].flags |= item_flag2;
					break;
					
				case iFairyMoving:
					itemsbuf[i].misc1 = moving_fairy_hearts;
					itemsbuf[i].misc2 = moving_fairy_magic;
					itemsbuf[i].misc3 = 50;
					if (moving_fairy_heart_percent) itemsbuf[i].flags |= item_flag1;
					if (moving_fairy_magic_percent) itemsbuf[i].flags |= item_flag2;
					break;
					
				case iRPotion:
					itemsbuf[i].misc1 = red_potion_hearts;
					itemsbuf[i].misc2 = red_potion_magic;
					if (red_potion_heart_percent) itemsbuf[i].flags |= item_flag1;
					if (red_potion_magic_percent) itemsbuf[i].flags |= item_flag2;
					break;
					
				case iBPotion:
					itemsbuf[i].misc1 = blue_potion_hearts;
					itemsbuf[i].misc2 = blue_potion_magic;
					if (blue_potion_heart_percent) itemsbuf[i].flags |= item_flag1;
					if (blue_potion_magic_percent) itemsbuf[i].flags |= item_flag2;
					break;
					
				case iSword:
					itemsbuf[i].pickup_hearts = sword_hearts[0];
					itemsbuf[i].misc1 = beam_hearts[0];
					itemsbuf[i].misc2 = beam_power[0];
					// It seems that item_flag1 was already added by reset_itembuf()...
					itemsbuf[i].flags &= (!get_bit(&beam_percent,0)) ? ~item_flag1 : ~item_none;
					break;
					
				case iWSword:
					itemsbuf[i].pickup_hearts = sword_hearts[1];
					itemsbuf[i].misc1 = beam_hearts[1];
					itemsbuf[i].misc2 = beam_power[1];
					itemsbuf[i].flags &= (!get_bit(&beam_percent,1)) ? ~item_flag1 : ~item_none;
					break;
					
				case iMSword:
					itemsbuf[i].pickup_hearts = sword_hearts[2];
					itemsbuf[i].misc1 = beam_hearts[2];
					itemsbuf[i].misc2 = beam_power[2];
					itemsbuf[i].flags &= (!get_bit(&beam_percent,2)) ? ~item_flag1 : ~item_none;
					break;
					
				case iXSword:
					itemsbuf[i].pickup_hearts = sword_hearts[3];
					itemsbuf[i].misc1 = beam_hearts[3];
					itemsbuf[i].misc2 = beam_power[3];
					itemsbuf[i].flags &= (!get_bit(&beam_percent,3)) ? ~item_flag1 : ~item_none;
					break;
					
				case iHookshot:
					itemsbuf[i].misc1 = hookshot_length;
					itemsbuf[i].misc2 = hookshot_links;
					break;
					
				case iLongshot:
					itemsbuf[i].misc1 = longshot_length;
					itemsbuf[i].misc2 = longshot_links;
					break;
			}
		}
		
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<168)))
	{
		//was new subscreen rule
		subscr_mode=get_qr(qr_FREEFORM)?1:0;
		set_qr(qr_FREEFORM,0);
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<185)))
	{
		temp_zinit.start_dmap=0;
	}
	
	if((Header->zelda_version < 0x192)||((Header->zelda_version == 0x192)&&(Header->build<186)))
	{
		temp_zinit.heroAnimationStyle=get_qr(qr_BSZELDA)?1:0;
	}
	
	if(s_version < 16 && get_bit(deprecated_rules, qr_COOLSCROLL+1))
	{
		//addOldStyleFamily(&temp_zinit, itemsbuf, itype_wallet, 4);   //is this needed?
		temp_zinit.mcounter[crMONEY]=999;
		//temp_zinit.counter[crMONEY]=999; //This rule only gave you an invisible max wallet; it did not give you max rupies.
	}
	if(Header->zelda_version < 0x190) //1.84 bugfix. -Z
	{
		//temp_zinit.items[iBombBag] = true; //No, this is 30 max bombs!
		temp_zinit.mcounter[crBOMBS] = 8;
	}
	// al_trace("About to copy over new init data values for quest made in: %x\n", Header->zelda_version);
	//time to ensure that we port all new values properly:
	if(Header->zelda_version < 0x250)
	{
		temp_zinit.mcounter[crSBOMBS] = bomb_ratio > 0 ? ( temp_zinit.mcounter[crBOMBS]/temp_zinit.bomb_ratio ) : (temp_zinit.mcounter[crBOMBS]/4);
	}
	
	if(s_version > 21)
	{
		if(!p_getc(&temp_zinit.hp_per_heart,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.magic_per_block,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.hero_damage_multiplier,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.ene_damage_multiplier,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.hp_per_heart = 16; //HP_PER_HEART, previously hardcoded
		temp_zinit.magic_per_block = 32; //MAGICPERBLOCK, previously hardcoded
		temp_zinit.hero_damage_multiplier = 2; //DAMAGE_MULTIPLIER, previously hardcoded
		temp_zinit.ene_damage_multiplier = 4; //(HP_PER_HEART/4), previously hardcoded
	}
	
	if(s_version > 22)
	{
		for(int32_t q = crCUSTOM1; q <= crCUSTOM25; ++q)
			if(!p_igetw(&temp_zinit.counter[q],f))
				return qe_invalid;
		for(int32_t q = crCUSTOM1; q <= crCUSTOM25; ++q)
			if(!p_igetw(&temp_zinit.mcounter[q],f))
				return qe_invalid;
	}
	
	
	if(s_version > 23)
	{
		if(!p_getc(&temp_zinit.dither_type,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.dither_arg,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.dither_percent,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.def_lightrad,f))
		{
			return qe_invalid;
		}
		if(!p_getc(&temp_zinit.transdark_percent,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.dither_type = 0;
		temp_zinit.dither_arg = 0;
		temp_zinit.dither_percent = 20;
		temp_zinit.def_lightrad = 24;
		temp_zinit.transdark_percent = 0;
	}
	
	if(s_version > 24)
	{
		if(!p_getc(&temp_zinit.darkcol,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.darkcol = BLACK;
	}
	
	if(s_version > 25)
	{
		if(!p_igetl(&temp_zinit.gravity,f))
		{
			return qe_invalid;
		}
		if(!p_igetl(&temp_zinit.swimgravity,f))
		{
			return qe_invalid;
		}
	}
	
	
	if(s_version > 26)
	{
		if(!p_igetw(&temp_zinit.heroSideswimUpStep,f))
		{
			return qe_invalid;
		}
		if(!p_igetw(&temp_zinit.heroSideswimSideStep,f))
		{
			return qe_invalid;
		}
		if(!p_igetw(&temp_zinit.heroSideswimDownStep,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.heroSideswimUpStep = 150;
		temp_zinit.heroSideswimSideStep = 100;
		temp_zinit.heroSideswimDownStep = 75;
	}
	
	if(s_version > 27)
	{
		if(!p_igetl(&temp_zinit.exitWaterJump,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.exitWaterJump = 0;
	}
	
	if(s_version > 29)
	{
		if(!p_igetl(&temp_zinit.bunny_ltm,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.bunny_ltm = 0;
	}
	
	if(s_version > 30)
	{
		if(!p_getc(&temp_zinit.switchhookstyle,f))
		{
			return qe_invalid;
		}
	}
	else
	{
		temp_zinit.switchhookstyle = 1;
	}
	
	if(s_version > 31)
	{
		if(!p_getc(&temp_zinit.magicdrainrate,f))
		{
			return qe_invalid;
		}
	}
	
	temp_zinit.clear_genscript();
	if(s_version > 32)
	{
		word numgenscript = 0;
		if(!p_igetw(&numgenscript,f))
			return qe_invalid;
		if (numgenscript > NUMSCRIPTSGENERIC)
			return qe_invalid;
		for(auto q = 1; q < numgenscript; ++q)
		{
			if(!p_getc(&tempbyte,f))
				return qe_invalid;
			if(!(tempbyte&2))
				continue;
			temp_zinit.gen_doscript.set(q, tempbyte&1);
			if(!p_igetw(&temp_zinit.gen_exitState[q],f))
				return qe_invalid;
			if(!p_igetw(&temp_zinit.gen_reloadState[q],f))
				return qe_invalid;
			for(auto p = 0; p < 8; ++p)
				if(!p_igetl(&temp_zinit.gen_initd[q][p],f))
					return qe_invalid;
			dword sz;
			if(!p_igetl(&sz,f))
				return qe_invalid;
			temp_zinit.gen_data[q].resize(sz);
			std::vector<int32_t> dummy;
			if(!p_getlvec(&dummy,f))
				return qe_invalid;
			temp_zinit.gen_data[q] = dummy;
			if(!p_igetl(&temp_zinit.gen_eventstate[q],f))
				return qe_invalid;
		}
	}
	if(s_version > 33)
	{
		if(!p_getc(&temp_zinit.hero_swim_mult,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hero_swim_div,f))
			return qe_invalid;
	}
	if(s_version > 34)
	{
		uint32_t num_used_mapscr_data;
		if(!p_igetl(&num_used_mapscr_data,f))
			return qe_invalid;
		for(uint32_t q = 0; q < num_used_mapscr_data; ++q)
		{
			uint32_t sz;
			if(!p_igetl(&sz,f))
				return qe_invalid;
			temp_zinit.screen_data[q].resize(sz);
			if(sz)
			{
				std::vector<int32_t> dummy;
				if(!p_getlvec(&dummy,f))
					return qe_invalid;
				temp_zinit.screen_data[q] = dummy;
			}
		}
	}
	if (s_version > 35)
		if(!p_igetzf(&temp_zinit.shove_offset,f))
			return qe_invalid;
	
	temp_zinit.counter[crLIFE] *= temp_zinit.hp_per_heart;
	temp_zinit.mcounter[crLIFE] *= temp_zinit.hp_per_heart;
	if(!temp_zinit.flags.get(INIT_FL_CONTPERCENT))
		temp_zinit.cont_heart *= temp_zinit.hp_per_heart;
	
	return 0;
}
int32_t readinitdata(PACKFILE *f, zquestheader *Header)
{
	zinitdata temp_zinit = {};
	
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_initdata);
	
	int32_t dummy;
	word s_version=0, s_cversion=0;
	byte padding;
	
	if(Header->zelda_version > 0x192)
	{
		if(!p_igetw(&s_version,f))
			return qe_invalid;
		FFCore.quest_format[vInitData] = s_version;
		
		if(!p_igetw(&s_cversion,f))
			return qe_invalid;
		
		//section size
		if(!p_igetl(&dummy,f))
			return qe_invalid;
	}
	
	if(s_version < 37)
	{
		if(auto ret = readinitdata_old(f,Header,s_version,s_cversion,temp_zinit))
			return ret;
	}
	else
	{
		subscr_mode = ssdtMAX;
		for(int q = 0; q < MAXITEMS/8; ++q)
			if(!p_getc(&temp_zinit.items[q], f))
				return qe_invalid;
		for(int q = 0; q < MAXLEVELS/8; ++q)
		{
			if(!p_getc(&temp_zinit.map[q], f))
				return qe_invalid;
			if(!p_getc(&temp_zinit.compass[q], f))
				return qe_invalid;
			if(!p_getc(&temp_zinit.boss_key[q], f))
				return qe_invalid;
			if(!p_getc(&temp_zinit.mcguffin[q], f))
				return qe_invalid;
		}
		if(!p_getbvec(&temp_zinit.level_keys, f))
			return qe_invalid;
		byte num_counters;
		if(!p_getc(&num_counters,f))
			return qe_invalid;
		for(int q = 0; q < num_counters; ++q)
			if(!p_igetw(&temp_zinit.counter[q],f))
				return qe_invalid;
		for(int q = 0; q < num_counters; ++q)
			if(!p_igetw(&temp_zinit.mcounter[q],f))
				return qe_invalid;
		if(!p_getc(&temp_zinit.bomb_ratio,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hcp,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hcp_per_hc,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.cont_heart,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hp_per_heart,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.magic_per_block,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hero_damage_multiplier,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.ene_damage_multiplier,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.dither_type,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.dither_arg,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.dither_percent,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.def_lightrad,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.transdark_percent,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.darkcol,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_grid_x,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_grid_y,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_grid_xofs,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_grid_yofs,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_grid_color,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_bbox_1_color,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_bbox_2_color,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.ss_flags,f))
			return qe_invalid;
		if(!p_getbitstr(&temp_zinit.flags,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.last_map,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.last_screen,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.msg_more_x,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.msg_more_y,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.msg_more_is_offset,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.msg_speed,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.gravity,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.swimgravity,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.terminalv,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hero_swim_speed,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hero_swim_mult,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.hero_swim_div,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.heroSideswimUpStep,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.heroSideswimSideStep,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.heroSideswimDownStep,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.exitWaterJump,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.heroStep,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.heroAnimationStyle,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.jump_hero_layer_threshold,f))
			return qe_invalid;
		if(!p_igetl(&temp_zinit.bunny_ltm,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.start_dmap,f))
			return qe_invalid;
		if(!p_igetw(&temp_zinit.subscrSpeed,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.switchhookstyle,f))
			return qe_invalid;
		if(!p_getc(&temp_zinit.magicdrainrate,f))
			return qe_invalid;
		if(!p_igetzf(&temp_zinit.shove_offset,f))
			return qe_invalid;
		if(!p_getbitstr(&temp_zinit.gen_doscript, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.gen_exitState, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.gen_reloadState, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.gen_initd, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.gen_eventstate, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.gen_data, f))
			return qe_invalid;
		if(!p_getbmap(&temp_zinit.screen_data, f))
			return qe_invalid;
		if (s_version >= 38)
		{
			if (!p_getc(&temp_zinit.spriteflickerspeed, f))
				return qe_invalid;
			if (!p_getc(&temp_zinit.spriteflickercolor, f))
				return qe_invalid;
			if (!p_getc(&temp_zinit.spriteflickertransp, f))
				return qe_invalid;
		}
		if(s_version >= 39)
			if(!p_igetzf(&temp_zinit.air_drag, f))
				return qe_invalid;
	}
	if (should_skip)
		return 0;

	if(loading_tileset_flags & TILESET_CLEARMAPS)
	{
		temp_zinit.last_map = 0;
		temp_zinit.last_screen = 0;
		temp_zinit.screen_data.clear();
	}
	temp_zinit.normalize();
	zinit = temp_zinit;
	
	if(zinit.heroAnimationStyle==las_zelda3slow)
	{
		hero_animation_speed=2;
	}
	else
	{
		hero_animation_speed=1;
	}
	
	return 0;
}

/*
void setupitemdropsets()
{
  for(int32_t i=0; i<isMAX; i++)
  {
    memcpy(&item_drop_sets[i], &default_item_drop_sets[i], sizeof(item_drop_object));
  }
}
*/

int32_t readitemdropsets(PACKFILE *f, int32_t version, word build)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_itemdropsets);

    build=build; // here to prevent compiler warnings
    dword dummy_dword;
    word item_drop_sets_to_read=0;
    item_drop_object tempitemdrop;
    word s_version=0, s_cversion=0;
    
	if (!should_skip)
	for(int32_t i=0; i<MAXITEMDROPSETS; i++)
	{
		memset(&item_drop_sets[i], 0, sizeof(item_drop_object));
	}
    
    if(version > 0x192)
    {
        item_drop_sets_to_read=0;
        
        //section version info
        if(!p_igetw(&s_version,f))
        {
            return qe_invalid;
        }
	
	FFCore.quest_format[vItemDropsets] = s_version;
        
        if(!p_igetw(&s_cversion,f))
        {
            return qe_invalid;
        }
        
        //section size
        if(!p_igetl(&dummy_dword,f))
        {
            return qe_invalid;
        }
        
        //finally...  section data
        if(!p_igetw(&item_drop_sets_to_read,f))
        {
            return qe_invalid;
        }

        if (item_drop_sets_to_read > MAXITEMDROPSETS)
        {
            return qe_invalid;
        }
    }
    else
    {
		init_item_drop_sets();
    }
    
    if(s_version>=1)
    {
        for(int32_t i=0; i<item_drop_sets_to_read; i++)
        {
            if(!p_getstr(tempitemdrop.name,sizeof(tempitemdrop.name)-1,f))
            {
                return qe_invalid;
            }
            
            for(int32_t j=0; j<10; ++j)
            {
                if(!p_igetw(&tempitemdrop.item[j],f))
                {
                    return qe_invalid;
                }
            }
            
            for(int32_t j=0; j<11; ++j)
            {
                if(!p_igetw(&tempitemdrop.chance[j],f))
                {
                    return qe_invalid;
                }
            }
            
            // Dec 2008: Addition of the 'Tall Grass' set, #12,
            // overrides the quest's set #12.
            if(s_version<2 && i==12)
                continue;
                
            // Deprecated: qr_NOCLOCKS and qr_ALLOW10RUPEEDROPS
            if(s_version<2) for(int32_t j=0; j<10; ++j)
                {
                    int32_t it = tempitemdrop.item[j];
                    
                    if((itemsbuf[it].family == itype_rupee
                            && ((itemsbuf[it].amount)&0xFFF) == 10)
                            && !get_bit(deprecated_rules, qr_ALLOW10RUPEEDROPS_DEP))
                    {
                        tempitemdrop.chance[j+1]=0;
                    }
                    else if(itemsbuf[it].family == itype_clock && get_bit(deprecated_rules, qr_NOCLOCKS_DEP))
                    {
                        tempitemdrop.chance[j+1]=0;
                    }
                    
                    // From Sept 2007 to Dec 2008, non-gameplay items were prohibited.
                    if(itemsbuf[it].family == itype_misc)
                    {
                        // If a non-gameplay item was selected, then item drop was aborted.
                        // Reflect this by increasing the 'Nothing' chance accordingly.
                        tempitemdrop.chance[0]+=tempitemdrop.chance[j+1];
                        tempitemdrop.chance[j+1]=0;
                    }
                }
            
			if (!should_skip)
				memcpy(&item_drop_sets[i], &tempitemdrop, sizeof(item_drop_object));
        }
    }
    
    return 0;
}

int32_t readfavorites(PACKFILE *f, int32_t, word)
{
	bool should_skip = legacy_skip_flags && get_bit(legacy_skip_flags, skip_favorites);

	int32_t temp_num;
	dword dummy_dword;
	word num_favorite_combos;
	word num_favorite_combo_aliases;
	word s_version=0, s_cversion=0;
	
	//section version info
	if(!p_igetw(&s_version,f))
	{
		return qe_invalid;
	}
	
	if (!should_skip)
		FFCore.quest_format[vFavourites] = s_version;
	
	if(!p_igetw(&s_cversion,f))
	{
		return qe_invalid;
	}
	
	//section size
	if(!p_igetl(&dummy_dword,f))
	{
		return qe_invalid;
	}
	
	word per_row = FAVORITECOMBO_PER_ROW;
	word per_page = FAVORITECOMBO_PER_PAGE;
	if(s_version >= 3)
		if(!p_igetw(&per_row,f))
			return qe_invalid;
	if(s_version >= 4)
		if(!p_igetw(&per_page,f))
			return qe_invalid;
	//finally...  section data
	if(!p_igetw(&num_favorite_combos,f))
	{
		return qe_invalid;
	}
	
	//Hack; port old favorite combos
	if(s_version < 3 && num_favorite_combos == 100)
		per_row = 13;

	if (!should_skip)
	for(int q = 0; q < MAXFAVORITECOMBOS; ++q)
		favorite_combos[q] = -1;
	byte favtype = 0;
	for(int32_t i=0; i<num_favorite_combos; i++)
	{
		if (s_version >= 4)
		{
			if (!p_getc(&favtype, f))
			{
				return qe_invalid;
			}
		}
		else
			favtype = 0;
		if(!p_igetl(&temp_num,f))
		{
			return qe_invalid;
		}

		if (should_skip)
			continue;
		
		if(per_row == FAVORITECOMBO_PER_ROW)
		{
			favorite_combos[i] = temp_num;
			favorite_combo_modes[i] = favtype;
		}
		else
		{
			int new_i = (i%per_row) + (i/per_row)*FAVORITECOMBO_PER_ROW;
			favorite_combos[new_i]=temp_num;
			favorite_combo_modes[new_i] = favtype;
		}
	}
	
	// Discard the separate favorite aliases list from previous versions
	if(s_version<4)
	{
		if (!p_igetw(&num_favorite_combo_aliases, f))
		{
			return qe_invalid;
		}

		for (int32_t i = 0; i < num_favorite_combo_aliases; i++)
		{
			if (!p_igetl(&temp_num, f))
			{
				return qe_invalid;
			}
		}
	}
	
	word max_combo_cols = 0;
	word max_mappages = 0;
	if(s_version >= 2)
	{
		if(!p_igetw(&max_combo_cols,f))
			return qe_invalid;
		int32_t tmp = 0, tmp2 = 0, tmp3 = 0;
		for(int q = 0; q < max_combo_cols; ++q)
		{
			if(!p_igetl(&tmp,f))
				return qe_invalid;
			if(!p_igetl(&tmp2,f))
				return qe_invalid;
			if(!p_igetl(&tmp3,f))
				return qe_invalid;
			if(q < MAX_COMBO_COLS)
			{
				First[q] = tmp;
				combo_alistpos[q] = tmp2;
				combo_pool_listpos[q] = tmp3;
			}
		}
		
		if(!p_igetw(&max_mappages,f))
			return qe_invalid;
		for(int q = 0; q < max_mappages; ++q)
		{
			if(!p_igetl(&tmp,f))
				return qe_invalid;
			if(!p_igetl(&tmp2,f))
				return qe_invalid;
			if(q < MAX_MAPPAGE_BTNS)
			{
				map_page[q].map = tmp;
				map_page[q].screen = tmp2;
			}
		}
	}

	if (should_skip)
		return 0;

	for(int q = max_combo_cols; q < MAX_COMBO_COLS; ++q)
	{
		First[q] = 0;
		combo_alistpos[q] = 0;
		combo_pool_listpos[q] = 0;
	}
	for(int q = max_mappages; q < MAX_MAPPAGE_BTNS; ++q)
	{
		map_page[q].map = 0;
		map_page[q].screen = 0;
	}
	
	return 0;
}

/*
  switch (ret) {
  case 0:
  break;

  case qe_invalid:
  goto invalid;
  break;
  default:
  pack_fclose(f);
  if(!oldquest)
  delete_file(tmpfilename);
  return ret;
  break;
  }
  */

const char *skip_text[skip_max]=
{
    "skip_header", "skip_rules", "skip_strings", "skip_misc",
    "skip_tiles", "skip_combos", "skip_comboaliases", "skip_csets",
    "skip_maps", "skip_dmaps", "skip_doors", "skip_items",
    "skip_weapons", "skip_colors", "skip_icons", "skip_initdata",
    "skip_guys", "skip_herosprites", "skip_subscreens", "skip_ffscript",
    "skip_sfx", "skip_midis", "skip_cheats", "skip_itemdropsets",
    "skip_favorites"
};


void port250QuestRules(){
	
	portCandleRules(); //Candle
	portBombRules();

}

void portCandleRules()
{
	bool hurtshero = get_qr(qr_FIREPROOFHERO);
	//itemdata itemsbuf;
	for ( int32_t q = 0; q < MAXITEMS; q++ ) 
	{
		if ( itemsbuf[q].family == itype_candle )
		{
			if ( hurtshero ) itemsbuf[q].flags |= item_flag2;
			else itemsbuf[q].flags &= ~ item_flag2;
		}
	}
}

void portBombRules()
{
	bool hurtshero = get_qr(qr_OUCHBOMBS);
	//itemdata itemsbuf;
	for ( int32_t q = 0; q < MAXITEMS; q++ ) 
	{
		if ( itemsbuf[q].family == itype_bomb )
		{
			if ( hurtshero ) itemsbuf[q].flags |= item_flag2;
			else itemsbuf[q].flags &= ~ item_flag2;
		}
	}
}

static int section_id_to_enum(int id)
{
	switch (id)
	{
		case ID_HEADER: return skip_header;
		case ID_RULES: return skip_rules;
		case ID_STRINGS: return skip_strings;
		case ID_MISC: return skip_misc;
		case ID_TILES: return skip_tiles;
		case ID_COMBOS: return skip_combos;
		case ID_COMBOALIASES: return skip_comboaliases;
		case ID_CSETS: return skip_csets;
		case ID_MAPS: return skip_maps;
		case ID_DMAPS: return skip_dmaps;
		case ID_DOORS: return skip_doors;
		case ID_ITEMS: return skip_items;
		case ID_WEAPONS: return skip_weapons;
		case ID_COLORS: return skip_colors;
		case ID_ICONS: return skip_icons;
		case ID_INITDATA: return skip_initdata;
		case ID_GUYS: return skip_guys;
		case ID_HEROSPRITES: return skip_herosprites;
		case ID_SUBSCREEN: return skip_subscreens;
		case ID_FFSCRIPT: return skip_ffscript;
		case ID_SFX: return skip_sfx;
		case ID_MIDIS: return skip_midis;
		case ID_CHEATS: return skip_cheats;
		case ID_ITEMDROPSETS: return skip_itemdropsets;
		case ID_FAVORITES: return skip_favorites;
		case ID_ZINFO: return skip_zinfo;
	}

	return -1;
}

static int maybe_skip_section(PACKFILE* f, dword& section_id, const byte* skip_flags)
{
	int section_enum = section_id_to_enum(section_id);
	bool skip = section_enum >= 0 && get_bit(skip_flags, section_enum);
	if (skip)
	{
		word s_version;
		if (!p_igetw(&s_version,f))
		{
			return qe_invalid;
		}

		word c_version;
		if (!p_igetw(&c_version,f))
		{
			return qe_invalid;
		}

		if (section_id == ID_RULES && s_version > 16)
		{
			dword dummy;
			if (!p_igetl(&dummy,f))
			{
				return qe_invalid;
			}
		}

		if (section_id == ID_FFSCRIPT && s_version >= 18)
		{
			word dummy;
			if (!p_igetw(&dummy,f))
			{
				return qe_invalid;
			}
		}

		dword section_length;
		if (!p_igetl(&section_length,f))
		{
			return qe_invalid;
		}

		if (pack_fseek(f, section_length))
		{
			return qe_invalid;
		}

		if (!pack_feof(f))
		{
			if (!p_mgetl(&section_id,f))
			{
				return qe_invalid;
			}
		}

		return qe_cancel;
	}

	return qe_OK;
}

//Internal function for loadquest wrapper
static int32_t _lq_int(const char *filename, zquestheader *Header, miscQdata *Misc, zctune *tunes, bool show_progress, byte *skip_flags, byte printmetadata)
{
    DMapEditorLastMaptileUsed = 0;
    combosread=false;
    mapsread=false;
    fixffcs=false;
	
	bool do_clear_scripts = !get_bit(skip_flags,skip_ffscript);
	if(loading_tileset_flags & TILESET_CLEARSCRIPTS)
	{
		set_bit(skip_flags, skip_ffscript, 1);
		setZScriptVersion(V_FFSCRIPT);
		FFCore.quest_format[vFFScript] = V_FFSCRIPT;
		FFCore.quest_format[vLastCompile] = V_FFSCRIPT;
		do_clear_scripts = true;
	}
	if(loading_tileset_flags & TILESET_CLEARMAPS)
	{
		set_bit(skip_flags, skip_maps, 1);
	}
    
    //  show_progress=true;
    char tmpfilename[L_tmpnam];
    temp_name(tmpfilename);
//  char percent_done[30];
    bool catchup=false;
    byte tempbyte;
    word old_map_count=map_count;
    
    byte old_quest_rules[QUESTRULES_NEW_SIZE] = {0};
    byte old_extra_rules[EXTRARULES_SIZE] = {0};
    byte old_midi_flags[MIDIFLAGS_SIZE] = {0};
    
    if(get_bit(skip_flags, skip_rules))
    {
        memcpy(old_quest_rules, quest_rules, QUESTRULES_NEW_SIZE);
        memcpy(old_extra_rules, extra_rules, EXTRARULES_SIZE);
    }
    
    memset(quest_rules, 0, QUESTRULES_NEW_SIZE); //clear here to prevent any kind of carryover -Z
	unpack_qrs();
   // memset(extra_rules, 0, EXTRARULES_SIZE); //clear here to prevent any kind of carryover -Z
   
    if(get_bit(skip_flags, skip_midis))
    {
        memcpy(old_midi_flags, midi_flags, MIDIFLAGS_SIZE);
    }
    
    
	if(do_clear_scripts)
	{
		zScript.clear();
		globalmap.clear();
		genericmap.clear();
		ffcmap.clear();
		itemmap.clear();
		npcmap.clear();
		ewpnmap.clear();
		lwpnmap.clear();
		playermap.clear();
		dmapmap.clear();
		screenmap.clear();
		itemspritemap.clear();
		comboscriptmap.clear();
		subscreenmap.clear();
		
		for(int32_t i=0; i<NUMSCRIPTFFC-1; i++)
		{
			ffcmap[i].clear();
		}
		
		globalmap[0].slotname = "Slot 1:";
		globalmap[0].scriptname = "~Init";
		globalmap[0].update();
		
		for(int32_t i=1; i<NUMSCRIPTGLOBAL; i++)
		{
			globalmap[i].clear();
		}
		
		for(int32_t i=0; i<NUMSCRIPTITEM-1; i++)
		{
			itemmap[i].clear();
		}
		
		//new script types -- prevent carrying over to a quest that you load after reading them
		//e.g., a quest has an npc script, and you make a blank quest, that now believes that it has an npc script, too!
		for(int32_t i=0; i<NUMSCRIPTGUYS-1; i++)
		{
			npcmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTWEAPONS-1; i++)
		{
			lwpnmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTWEAPONS-1; i++)
		{
			ewpnmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTHERO-1; i++)
		{
			playermap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSDMAP-1; i++)
		{
			dmapmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSCREEN-1; i++)
		{
			screenmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE-1; i++)
		{
			itemspritemap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSCOMBODATA-1; i++)
		{
			comboscriptmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSGENERIC-1; i++)
		{
			genericmap[i].clear();
		}
		for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN-1; i++)
		{
			subscreenmap[i].clear();
		}
		
		reset_scripts();
	}
	
    zquestheader tempheader;
    memset(&tempheader, 0, sizeof(zquestheader));
	zinfo tempzi;
	tempzi.clear();
	load_tmp_zi = &tempzi;
    
    // oldquest flag is set when an unencrypted qst file is suspected.
    bool oldquest = false;
    int32_t open_error=0;
    PACKFILE *f=open_quest_file(&open_error, filename, show_progress);
    
    if (!f)
    {
        ASSERT(open_error != 0);
        return open_error;
    }
	char zinfofilename[2048];
	replace_extension(zinfofilename, filename, "zinfo", 2047);
    int32_t ret=0;
    
    //header
    box_out("Reading Header...");
    ret=readheader(f, &tempheader, printmetadata);
    checkstatus(ret);
    box_out("okay.");
    box_eol();
    
	if(read_zinfo)
	{
		box_out("Reading ZInfo - ");
		box_out(read_ext_zinfo ? "External..." : "Internal...");
		if(read_ext_zinfo)
		{
			PACKFILE *inf=pack_fopen_password(zinfofilename, F_READ, "");
			ret=readzinfo(inf, tempzi, tempheader);
			if(inf) pack_fclose(inf);
			checkstatus(ret);
		}
		else
		{
			ret=readzinfo(f, tempzi, tempheader);
			checkstatus(ret);
		}
		box_out("okay.");
		box_eol();
	}
	
    if(tempheader.zelda_version>=0x193)
    {
        dword section_id;
        
        //section id
        if(!p_mgetl(&section_id,f))
        {
            goto invalid;
        }

        std::set<dword> seen_sections;
        
        while(!pack_feof(f))
        {
            if (seen_sections.contains(section_id))
                goto invalid;
            seen_sections.insert(section_id);

			if (int retval = maybe_skip_section(f, section_id, skip_flags); retval != qe_OK)
			{
				if (retval == qe_cancel)
					continue;
				checkstatus(retval);
			}

            switch(section_id)
            {
            case ID_RULES:
            
                //rules
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Rules...");
                ret=readrules(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_STRINGS:
            
                //strings
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Strings...");
                ret=readstrings(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_MISC:
            
                //misc data
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Misc. Data...");
                ret=readmisc(f, &tempheader, Misc);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_TILES:
            
                //tiles
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Tiles...");
                ret=readtiles(f, newtilebuf, &tempheader, tempheader.zelda_version, tempheader.build, 0, NEWMAXTILES, false);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_COMBOS:
            
                //combos
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Combos...");
                ret=readcombos(f, &tempheader, tempheader.zelda_version, tempheader.build, 0, MAXCOMBOS);
                combosread=true;
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_COMBOALIASES:
            
                //combo aliases
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Combo Aliases...");
                ret=readcomboaliases(f, &tempheader, tempheader.zelda_version, tempheader.build);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_CSETS:
            
                //color data
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Color Data...");
                ret=readcolordata(f, Misc, tempheader.zelda_version, tempheader.build, 0, newerpdTOTAL);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_MAPS:
            
                //maps
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Maps...");
                ret=readmaps(f, &tempheader);
                mapsread=true;
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_DMAPS:
            
                //dmaps
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading DMaps...");
                ret=readdmaps(f, &tempheader, tempheader.zelda_version, tempheader.build, 0, MAXDMAPS);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_DOORS:
            
                //door combo sets
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Doors...");
                ret=readdoorcombosets(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_ITEMS:
            
                //items
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Items...");
                ret=readitems(f, tempheader.zelda_version, tempheader.build);
                checkstatus(ret);
                
                box_out("okay.");
                box_eol();
                break;
                
            case ID_WEAPONS:
            
                //weapons
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Weapons...");
                ret=readweapons(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_COLORS:
            
                //misc. colors
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Misc. Colors...");
                ret=readmisccolors(f, &tempheader, Misc);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_ICONS:
            
                //game icons
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Game Icons...");
                ret=readgameicons(f, &tempheader, Misc);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_INITDATA:
            
                //initialization data
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Init. Data...");
                ret=readinitdata(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                
                if(!get_bit(skip_flags, skip_subscreens))
                {
                    if(subscr_mode!=ssdtMAX)  //not using custom subscreens
                    {
                        setupsubscreens();
                        
                        for(int32_t i=0; i<MAXDMAPS; ++i)
                        {
                            int32_t type=DMaps[i].type&dmfTYPE;
                            DMaps[i].active_subscreen=(type == dmOVERW || type == dmBSOVERW)?0:1;
                            DMaps[i].passive_subscreen=(get_qr(qr_ENABLEMAGIC))?0:1;
                        }
                    }
                }
                
                if(!get_bit(skip_flags, skip_sfx))
                {
                    setupsfx();
                }
                
                if(!get_bit(skip_flags, skip_itemdropsets))
                {
                    init_item_drop_sets();
                }
                
                if(!get_bit(skip_flags, skip_favorites))
                {
                    init_favorites();
                }
                
                break;
                
            case ID_GUYS:
            
                //guys
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Custom Guy Data...");
                ret=readguys(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_HEROSPRITES:
            
                //hero sprites
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Custom Hero Sprite Data...");
                ret=readherosprites(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_SUBSCREEN:
            
                //custom subscreens
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Custom Subscreen Data...");
                ret=readsubscreens(f);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_FFSCRIPT:
            
                //Freeform combo scripts
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading FF Script Data...");
                ret=readffscript(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_SFX:
            
                //SFX data
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading SFX Data...");
                ret=readsfx(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_MIDIS:
            
                //midis
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Tunes...");
                ret=readtunes(f, &tempheader, tunes);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_CHEATS:
            
                //cheat codes
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Cheat Codes...");
                ret=readcheatcodes(f, &tempheader);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_ITEMDROPSETS:
            
                //item drop sets
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Item Drop Sets...");
                ret=readitemdropsets(f, tempheader.zelda_version, tempheader.build);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            case ID_FAVORITES:
            
                //favorite combos and combo aliases
                if(catchup)
                {
                    box_out("found.");
                    box_eol();
                    catchup=false;
                }
                
                box_out("Reading Favorite Combos...");
                ret=readfavorites(f, tempheader.zelda_version, tempheader.build);
                checkstatus(ret);
                box_out("okay.");
                box_eol();
                break;
                
            default:
                if(!catchup)
                {
                    box_out("Bad token!  Searching...");
                    box_eol();
                }
                
                catchup=true;
                break;
            }
            
	    
            if(catchup)
            {
                //section id
                section_id=(section_id<<8);
                
                if(!p_getc(&tempbyte,f))
                {
					goto invalid;
                }
                
                section_id+=tempbyte;
            }
            
            else
            {
                //section id
                if(!pack_feof(f))
                {
                    if(!p_mgetl(&section_id,f))
                    {
                        goto invalid;
                    }
                }
            }
        }
    }
    else
    {
		std::vector<std::tuple<std::string, int32_t, std::function<int32_t()>>> hardcoded_sections = {
			{ "Rules", ID_RULES, [&](){ return readrules(f, &tempheader); }},
			{ "Strings", ID_STRINGS, [&](){ return readstrings(f, &tempheader); }},
			{ "Doors", ID_DOORS, [&](){ return readdoorcombosets(f, &tempheader); }},
			{ "DMaps", ID_DMAPS, [&](){ return readdmaps(f, &tempheader, tempheader.zelda_version, tempheader.build, 0, MAXDMAPS); }},
			{ "Misc. Data", ID_MISC, [&](){ return readmisc(f, &tempheader, Misc); }},
			{ "Items", ID_ITEMS, [&](){ return readitems(f, tempheader.zelda_version, tempheader.build); }},
			{ "Weapons", ID_WEAPONS, [&](){ return readweapons(f, &tempheader); }},
			{ "Custom Guy Data", ID_GUYS, [&](){ return readguys(f, &tempheader); }},
			{ "Maps", ID_MAPS, [&](){ return readmaps(f, &tempheader); }},
			{ "Combos", ID_COMBOS, [&](){ return readcombos(f, &tempheader, tempheader.zelda_version, tempheader.build, 0, MAXCOMBOS); }},
			{ "Color Data", ID_CSETS, [&](){ return readcolordata(f, Misc, tempheader.zelda_version, tempheader.build, 0, newerpdTOTAL); }},
			{ "Tiles", ID_TILES, [&](){ return readtiles(f, newtilebuf, &tempheader, tempheader.zelda_version, tempheader.build, 0, NEWMAXTILES, false); }},
			{ "Tunes", ID_MIDIS, [&](){ return readtunes(f, &tempheader, tunes); }},
			{ "Cheat Codes", ID_CHEATS, [&](){ return readcheatcodes(f, &tempheader); }},
			{ "Init. Data", ID_INITDATA, [&](){ return readinitdata(f, &tempheader); }},
			{ "Custom Hero Sprite Data", ID_HEROSPRITES, [&](){ return readherosprites2(f, -1, 0); }},
			{ "Up Default Item Drop Sets", ID_ITEMDROPSETS, [&](){ return readitemdropsets(f, -1, 0); }},
		};

		legacy_skip_flags = skip_flags;
		for (auto& [desc, section_id, fn] : hardcoded_sections)
		{
			int section_enum = section_id_to_enum(section_id);
			bool skip = section_enum >= 0 && get_bit(skip_flags, section_enum);
			if (skip)
			{
				// Nothing to read.
				if (section_id == ID_RULES)
					continue;
				if (section_id == ID_GUYS)
					continue;

				// Haven't looked at how to skip these, because we don't need to currently: the only
				// usage of skip_flags currently is all off except: header and tiles (see `load_imagebuf`).
				if (section_id == ID_MIDIS)
					continue;
				if (section_id == ID_CHEATS)
					continue;
				if (section_id == ID_INITDATA)
					continue;
				if (section_id == ID_HEROSPRITES)
					continue;
				if (section_id == ID_ITEMDROPSETS)
					continue;
			}

			// Would be nice, but old sections mostly did not save section sizes. We could advance by
			// a specific amount, but it'd be a lot of work to get it right. So, for old quests, let's just
			// read all the sections even if requested to skip some.
			// if (int retval = maybe_skip_section(f, section_id, skip_flags); retval != qe_OK)
			// {
			// 	if (retval == qe_cancel)
			// 		continue;
			// 	checkstatus(retval);
			// }

			box_out(fmt::format("Reading {}...", desc).c_str());
			ret = fn();
			checkstatus(ret);
			box_out("okay.");
			box_eol();
		}
		legacy_skip_flags = nullptr;

        if(!get_bit(skip_flags, skip_subscreens))
        {
            setupsubscreens();
            
            for(int32_t i=0; i<MAXDMAPS; ++i)
            {
                int32_t type=DMaps[i].type&dmfTYPE;
                DMaps[i].active_subscreen=(type == dmOVERW || type == dmBSOVERW)?0:1;
                DMaps[i].passive_subscreen=(get_qr(qr_ENABLEMAGIC))?0:1;
            }
        }
        
        box_out("Setting Up Default Sound Effects...");
        
        if(!get_bit(skip_flags, skip_sfx))
            setupsfx();
		
		if(!get_bit(skip_flags, skip_favorites))
			init_favorites();
            
        box_out("okay.");
        box_eol();
    }
    
	init_spritelists();
	
    // check data
    if(f)
    {
        pack_fclose(f);
    }
	clear_quest_tmpfile();
    
    if(!oldquest)
    {
        if(exists(tmpfilename))
        {
            delete_file(tmpfilename);
        }
    }
    
    if(fixffcs && combosread && mapsread)
    {
        for(int32_t i=0; i<map_count; i++)
        {
            for(int32_t j=0; j<MAPSCRS; j++)
            {
                TheMaps[(i*MAPSCRS)+j].resizeFFC(32);
                for(int32_t m=0; m<32; m++)
                {
                    if(combobuf[TheMaps[(i*MAPSCRS)+j].ffcs[m].data].type == cCHANGE)
                        TheMaps[(i*MAPSCRS)+j].ffcs[m].flags|=ffc_changer;
                }
            }
        }
    }
    
    if(get_qr(qr_CONTFULL_DEP) && !get_bit(skip_flags, skip_rules) && !get_bit(skip_flags, skip_initdata))
    {
        set_qr(qr_CONTFULL_DEP, 0);
        zinit.flags.set(INIT_FL_CONTPERCENT,true);
        zinit.cont_heart=100;
        zinit.counter[crLIFE]=zinit.mcounter[crLIFE];
    }
    
    box_out("Done.");
    box_eol();
    box_end(false);
    
    if(!get_bit(skip_flags, skip_header))
    {
        memcpy(Header, &tempheader, sizeof(tempheader));
    }
    if(!get_bit(skip_flags, skip_zinfo))
    {
		ZI.copyFrom(tempzi);
    }
    
    if(get_bit(skip_flags, skip_maps))
    {
        map_count=old_map_count;
    }
    
    if(get_bit(skip_flags, skip_rules))
    {
        memcpy(quest_rules, old_quest_rules, QUESTRULES_NEW_SIZE);
        memcpy(extra_rules, old_extra_rules, EXTRARULES_SIZE);
		unpack_qrs();
    }
    
    if(get_bit(skip_flags, skip_midis))
    {
        memcpy(midi_flags, old_midi_flags, MIDIFLAGS_SIZE);
    }
    
	if(loading_tileset_flags & TILESET_CLEARMAPS)
	{
		TheMaps.clear();
		TheMaps.resize(MAPSCRS*1);
		map_count = 1;
		map_autolayers.clear();
		map_autolayers.resize(6*1);
	}
	if(loading_tileset_flags & TILESET_CLEARHEADER)
	{
		memset(Header->password, 0, sizeof(Header->password));
		memset(Header->minver, 0, sizeof(Header->minver));
		memset(Header->title, 0, sizeof(Header->title));
		memset(Header->author, 0, sizeof(Header->author));
		memset(Header->version, 0, sizeof(Header->version));
		Header->use_keyfile = 0;
		Header->dirty_password = false;
		cvs_MD5Context ctx;
		cvs_MD5Init(&ctx);
		cvs_MD5Update(&ctx, (const uint8_t*)"", 0);
		cvs_MD5Final(Header->pwd_hash, &ctx);
	}

	if (!get_bit(skip_flags, skip_maps))
	{
		// Not needed, so release its memory.
		old_combo_pages = {};
	}

    return qe_OK;
    
invalid:
    // TODO: It's too easy to forget to jump to this label, and accidentally leave the file open.
    // Should wrap PACKFILE in a std::unique_pointer with a custom deallocator.
    box_out("error.");
    box_eol();
    box_end(true);
    
    pack_fclose(f);
    
    if(!oldquest)
    {
        if(exists(tmpfilename))
        {
            delete_file(tmpfilename);
        }
    }
    
    return qe_invalid;
    
}

static bool _is_loading_quest;

bool is_loading_quest()
{
	return _is_loading_quest;
}

std::string get_last_loaded_qstpath()
{
	return last_loaded_qstpath;
}

int32_t loadquest(const char *filename, zquestheader *Header, miscQdata *Misc,
	zctune *tunes, bool show_progress, byte *skip_flags, byte printmetadata,
	bool report, byte qst_num, dword tilesetflags)
{
	loading_tileset_flags = tilesetflags;
	const char* basename = get_filename(filename);
	zapp_reporting_add_breadcrumb("load_quest", basename);
	zapp_reporting_set_tag("qst.filename", basename);

	last_loaded_qstpath = filename;
	loading_qst_name = filename;
	loading_qst_num = qst_num;
	// In CI, builds are cached for replay tests, which can result in their build dates being earlier than what it would be locally.
	// So to avoid a more-recently updated .qst file from hitting the "last saved in a newer version" prompt, we disable in CI.
	if (!is_ci())
		loadquest_report = report;

	_is_loading_quest = true;
	auto start = std::chrono::steady_clock::now();
	zprint2("Loading qst: %s\n", filename);
	int32_t ret = _lq_int(filename, Header, Misc, tunes, show_progress, skip_flags, printmetadata);
	int32_t load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	zprint2("Time to load qst: %d ms\n", load_ms);
	_is_loading_quest = false;
	if (ret)
		zprint2("Error: %s\n", qst_error[ret]);

	if(show_progress)
	{
		if(ret)
		{
			box_out("-- Error loading quest file! --");
			box_out(fmt::format("Error: {}", qst_error[ret]).c_str());
			box_end(true);
		}
		else box_end(false);
	}

	load_tmp_zi = NULL;
	loading_qst_name = NULL;
	loadquest_report = false;
	loading_qst_num = 0;

	zapp_reporting_set_tag("qst.author", Header->author);
	zapp_reporting_set_tag("qst.title", Header->title);
	zapp_reporting_set_tag("qst.zc_version", Header->getVerStr());

	return ret;
}
