#ifndef ZDEFS_H_
#define ZDEFS_H_

#define DEVLEVEL 0
#define COLLECT_SCRIPT_ITEM_ZERO -32767

//DEVLEVEL 1 = extra debug tools
//DEVLEVEL 2 = force enable cheats
//DEVLEVEL 3 ?? (Seems to do nothing special)
//DEVLEVEL 4 = ignore quest passwords

#ifdef _DEBUG
#if DEVLEVEL < 4
#undef DEVLEVEL
#define DEVLEVEL 4
#endif
#endif

#if DEVLEVEL > 0
#define DEVLOGGING	dev_logging
#define DEVDEBUG	dev_debug
#define DEVTIMESTAMP dev_timestmp
#else
#define DEVLOGGING	 false
#define DEVDEBUG	 false
#define DEVTIMESTAMP false
#endif

#include <cstdio>
#include <math.h>
#include <cstring>
#include <set>
#include <assert.h>
#include <algorithm>

#include "base/headers.h"
#include "metadata/metadata.h"
#include "base/about.h"
#include "base/zc_alleg.h"
#include "gamedata.h"
#include "base/random.h"
#include "base/util.h"
#include "base/render.h"
#include "fontsdat.h"
#include "zconfig.h"
#include "flags.h"

struct mapscr;
class solid_object;
class ffcdata;
struct cpos_info;


// These version fields are deprecated, and no longer update. Replaced by base/version.h
#define ZELDA_VERSION       0x0255                         //version of the program
#define ZC_VERSION_ID 25500 //Version ID for ZScript Game->Version
#define VERSION_BUILD       61                             //V_BUILD build number of this version. Deprecated.

#define COPYRIGHT_YEAR      "2019"                          //shown on title screen and in ending

#define MIN_VERSION         0x0184

#define ZELDADAT_VERSION      0x0211                        //version of zelda.dat
#define ZELDADAT_BUILD        17                            //build of zelda.dat
#define SFXDAT_VERSION        0x0211                        //version of sfx.dat
#define SFXDAT_BUILD          15                            //build of sfx.dat
#define FONTSDAT_VERSION      0x0253                        //version of fonts.dat
#define FONTSDAT_BUILD        30                            //build of fonts.dat
#define QSTDAT_VERSION        0x0253                        //version of qst.dat
#define QSTDAT_BUILD          30                            //build of qst.dat

enum {ENC_METHOD_192B104=0, ENC_METHOD_192B105, ENC_METHOD_192B185, ENC_METHOD_211B9, ENC_METHOD_211B18, ENC_METHOD_MAX};

//Moved these OS-dependent defs from 'ffasm.cpp', to be global.
#ifdef ALLEGRO_MACOSX
#define strnicmp strncasecmp
#endif

#ifdef ALLEGRO_MACOSX
#define strnicmp strncasecmp
#endif

#ifdef ALLEGRO_LINUX
#define strnicmp strncasecmp
#endif

#ifdef _MSC_VER
#define stricmp _stricmp
#define strnicmp _strnicmp
#define unlink _unlink
#endif

#ifdef _WIN32
#define PATH_SLASH '\\'
#define WRONG_PATH_SLASH '/'
#else
#define PATH_SLASH '/'
#define WRONG_PATH_SLASH '\\'
#endif

#define PI 3.14159265358979323846
#define PI2 (2*PI)

#define ZC_ID(a,b,c,d)  (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

//ID values of the different section types
#define ID_HEADER         ZC_ID('H','D','R',' ')              //quest header
#define ID_RULES          ZC_ID('R','U','L','E')              //quest rules
#define ID_STRINGS        ZC_ID('S','T','R',' ')              //message strings
#define ID_MISC           ZC_ID('M','I','S','C')              //misc data
#define ID_TILES          ZC_ID('T','I','L','E')              //tiles
#define ID_COMBOS         ZC_ID('C','M','B','O')              //combos
#define ID_CSETS          ZC_ID('C','S','E','T')              //csets (and pal names?)
#define ID_MAPS           ZC_ID('M','A','P',' ')              //maps
#define ID_DMAPS          ZC_ID('D','M','A','P')              //dmaps
#define ID_DOORS          ZC_ID('D','O','O','R')              //door combo sets
#define ID_ITEMS          ZC_ID('I','T','E','M')              //items
#define ID_WEAPONS        ZC_ID('W','P','N',' ')              //weapons
#define ID_COLORS         ZC_ID('M','C','L','R')              //misc. colors
#define ID_ICONS          ZC_ID('I','C','O','N')              //save game icons
#define ID_GRAPHICSPACK   ZC_ID('G','P','A','K')              //graphics pack header
#define ID_INITDATA       ZC_ID('I','N','I','T')              //initialization data
#define ID_GUYS           ZC_ID('G','U','Y',' ')              //guys
#define ID_MIDIS          ZC_ID('M','I','D','I')              //midis
#define ID_CHEATS         ZC_ID('C','H','T',' ')              //cheats
#define ID_SAVEGAME       ZC_ID('S','V','G','M')              //save game data (used in the save game file)
#define ID_COMBOALIASES   ZC_ID('C','M','B','A')              //combo alias
#define ID_HEROSPRITES    ZC_ID('L','I','N','K')              //Hero sprites
#define ID_SUBSCREEN      ZC_ID('S','U','B','S')              //subscreen data
#define ID_ITEMDROPSETS   ZC_ID('D','R','O','P')              //item drop set tables
#define ID_FAVORITES      ZC_ID('F','A','V','S')              //favorite combos and combo aliases
#define ID_FFSCRIPT       ZC_ID('F','F','S','C')              //ff scripts data
#define ID_SFX            ZC_ID('S','F','X',' ')              //sfx data
#define ID_ZINFO          ZC_ID('Z','I','N','F')              //ZInfo data

//Version number of the different section types
#define V_HEADER           9
#define V_RULES           17
#define V_STRINGS         10
#define V_MISC            16
#define V_TILES            3 //2 is a int32_t, max 214500 tiles (ZScript upper limit)
#define V_COMBOS          50
#define V_CSETS            6 //palette data
#define V_MAPS            30
#define V_DMAPS           21
#define V_DOORS            1
#define V_ITEMS           59
#define V_WEAPONS          8
#define V_COLORS           4 //Misc Colours
#define V_ICONS            10 //Game Icons
#define V_GRAPHICSPACK     1
#define V_INITDATA        39
#define V_GUYS            53
#define V_MIDIS            4
#define V_CHEATS           1
#define V_SAVEGAME        42
#define V_COMBOALIASES     5
#define V_HEROSPRITES      16
#define V_SUBSCREEN        12
#define V_ITEMDROPSETS     2
#define V_FFSCRIPT         26
#define V_SFX              8
#define V_FAVORITES        4

#define V_COMPATRULE       73
#define V_ZINFO            4

//= V_SHOPS is under V_MISC

/*
  * Compatible version number of the different section types
  * Basically, the last version number that this section type
  * is just an extension of (ie. new variables are stuck on the end)
  * instead of a complete rewrite (or variables added to the middle).
  * If this and the version number are the same, then this is
  * a total reworking of the section and probably won't be able
  * to be read by anything that was written for a previous version.
  */
#define CV_HEADER          3
#define CV_RULES           1
#define CV_STRINGS         2
#define CV_MISC            7
#define CV_TILES           1
#define CV_COMBOS          1
#define CV_CSETS           1
#define CV_MAPS            9
#define CV_DMAPS           1
#define CV_DOORS           1
#define CV_ITEMS          15
#define CV_WEAPONS         1
#define CV_COLORS          1
#define CV_ICONS           1
#define CV_GRAPHICSPACK    1
#define CV_INITDATA       15
#define CV_GUYS            4
#define CV_MIDIS           3
#define CV_CHEATS          1
#define CV_SAVEGAME        5
#define CV_COMBOALIASES    1
#define CV_HEROSPRITES     1
#define CV_SUBSCREEN       3
#define CV_ITEMDROPSETS    1
#define CV_FFSCRIPT        1
#define CV_SFX             5
#define CV_FAVORITES       1
#define CV_ZINFO           0


// Loose Object Version Metadata
// If the version is 0, it is ther original.
// 'Higher Versions' use negative numbers, so -1 is newer than 0, -2 is newer than -1.
// Sorry for the ugly hack, but it maintains full compatibility through
// Packfile magic. -Z
#define V_ZCOMBO -1
#define V_ZMOD -1
#define V_ZTHEME -1
#define V_ZASM -1
#define V_ZTILE -1
#define V_ZTILESET -1
#define V_ZDMAP -1
#define V_ZALIAS -1
#define V_ZNPC -1
#define V_ZITEM -1
#define V_ZWPNSPR -1


void zprint(const char * const format,...);
void zprint2(const char * const format,...);

extern int32_t curr_tb_page;
extern int32_t original_playing_field_offset;
extern int32_t playing_field_offset;
extern int32_t passive_subscreen_height;
extern int32_t passive_subscreen_offset;

extern int32_t CSET_SIZE;
extern int32_t CSET_SHFT;

extern volatile bool close_button_quit, exiting_program, dialog_open_quit;
bool handle_close_btn_quit();
#define HANDLE_CLOSE_ZQDLG() \
if(close_button_quit) \
{ \
	dialog_open_quit = true; \
	handle_close_btn_quit(); \
	dialog_open_quit = false; \
}

// system colors
#define lc1(x) ((x)+192)                                    // offset to 'level bg color' x (row 12)
#define lc2(x) ((x)+208)                                    // offset to 'level fg color' x (row 13)
#define vc(x)  ((x)+224)                                    // offset to 'VGA color' x (row 14)
#define dvc(x) ((x)+240)                                    // offset to dark 'VGA color' x (row 15)
#define r_lc1(x) ((x)-192)
#define r_lc2(x) ((x)-208)
#define r_vc(x)  ((x)-224)
#define r_dvc(x) ((x)-240)
#define BLACK         253
#define WHITE         254

#define LARGE_W       912
#define LARGE_H       684

#define BYTE_FILTER 0xFF
#define DIAG_TO_SIDE		0.7071
#define SIDE_TO_DIAG		1.4142
#define STEP_DIAGONAL(s)	(s*DIAG_TO_SIDE)

#define WRAP_CS(cs) ((cs+14)%14)
#define WRAP_CS2(cs,cs2) (get_qr(qr_OLDCS2)?((cs+cs2+16)%16):((cs+cs2+14)%14))

#define XOR(a,b) (!(a) != !(b))

// quest stuff
#define ZQ_TILES        0
#define ZQ_MIDIS2       1                                   //4 bytes
#define ZQ_CHEATS2       5
#define ZQ_MAXDATA      20

// lvlitems flags
#define liTRIFORCE      0x01
#define liMAP           0x02
#define liCOMPASS       0x04
#define liBOSS          0x08
#define liBOSSKEY       0x10

#define liALLUSED       0x1F
#define liALL           0xFF

// sprite drawing flag bits
#define sprdrawflagALWAYSOLDDRAWS 1

// item "pick up" flags
#define ipBIGRANGE      1                                   // Collision rectangle is large
#define ipHOLDUP        2                                   // Hero holds up item when he gets it
#define ipONETIME       4                                   // Getting this item sets mITEM
#define ipDUMMY         8                                   // Dummy item.  Can't get this.
#define ipCHECK         16                                  // Check restrictions (money in a shop, etc.)
#define ipMONEY         32                                  // This item causes money loss (bomb upgrade, swindle room, etc.)
#define ipFADE          64                                  // Blinks if fadeclk>0
#define ipENEMY         128                                 // Enemy is carrying it around
#define ipTIMER         256                                 // Disappears after a while
#define ipBIGTRI        512                                 // Large collision rectangle (used for large triforce)
#define ipNODRAW        1024                                // Don't draw this (for underwater items)
#define ipONETIME2      2048                                // Getting this item sets mSPECIALITEM
#define ipSECRETS       4096                                // Trigger Secrets when picked up
#define ipCANGRAB       8192                                // Always grabbable to hookshot/arrows/brang

//We have definitions for this below, but this is for scripted stuff in 2,54. 
enum { warpfxNONE, warpfxBLACKOUT, warpfxWIPE, warpfxSCROLL, warpfxZAP, warpfxZAPIN, warpfxZAPOUT, warpfxWAVY, 
	warpfxWAVYIN, warpfxWAVYOUT, warpfxWAVYNOHERO, warpfxWAVYINNOHERO, warpfxWAVYOUTNOHERO, 
		warpfxLAST};
	
//wipe types - mosaic should be one of them. 
enum { wipeNONE, wipeCOOL, wipeBLACK, wipeBSZELDA, wipeTRIANGLE, wipeCIRCLE, wipeALLSTARS, wipeLAST };

enum
{
    wtCAVE, wtPASS, wtEXIT, wtSCROLL, wtIWARP, wtIWARPBLK, wtIWARPOPEN,
    wtIWARPZAP, wtIWARPWAVE, wtNOWARP, wtWHISTLE, wtMAX
};

enum
{
    weINSTANT, weCIRCLE, weOVAL, weTRIANGLE, weSMAS, weBLINDS_SMOOTH, weBLINDS_STEPPED, weMOSAIC, weWAVE_WHITE, weWAVE_BLACK, weFADE_WHITE, weFADE_BLACK, weDEFAULT_OC, weDEST_DEFAULT, weMAX
};
// sprite palettes
enum
{
    spAQUA, spGLEEOK, spDIG, spGANON, spBROWN, spPILE, spBLUE, spRED,
    spGOLD, spICON1, spICON2, spICON3, spICON4, spGLEEOKF, spFROZEN
};

// map flags
enum
{	//These are the COMBO flags. -Z
	//0
	mfNONE, 	mfPUSHUD, 	mfPUSH4, 	mfWHISTLE, 	mfANYFIRE, 
	//5
	mfARROW, 	mfBOMB, 	mfFAIRY, 	mfRAFT, 	mfARMOS_SECRET, 
	//10
	mfARMOS_ITEM, 	mfSBOMB, 	mfRAFT_BRANCH, 	mfDIVE_ITEM,	mfLENSMARKER, 
	//15
	mfZELDA,	mfSECRETS01, 	mfSECRETS02, 	mfSECRETS03, 	mfSECRETS04,       
	//20
	mfSECRETS05, 	mfSECRETS06, 	mfSECRETS07, 	mfSECRETS08,	mfSECRETS09, 
	//25
	mfSECRETS10, 	mfSECRETS11, 	mfSECRETS12,	mfSECRETS13, 	mfSECRETS14, 
	//30
	mfSECRETS15, 	mfSECRETS16,	mfTRAP_H, 	mfTRAP_V, 	mfTRAP_4, 
	//35
	mfTRAP_LR, 	mfTRAP_UD,	mfENEMY0, 	mfENEMY1, 	mfENEMY2, 
	//40
	mfENEMY3, 	mfENEMY4,	mfENEMY5, 	mfENEMY6, 	mfENEMY7, 
	//45
	mfENEMY8, 	mfENEMY9,	mfPUSHLR, 	mfPUSHU, 	mfPUSHD, 
	//50
	mfPUSHL, 	mfPUSHR,	mfPUSHUDNS, 	mfPUSHLRNS, 	mfPUSH4NS, 
	//55
	mfPUSHUNS,	mfPUSHDNS, 	mfPUSHLNS, 	mfPUSHRNS, 	mfPUSHUDINS,
	//60
	mfPUSHLRINS, 	mfPUSH4INS, 	mfPUSHUINS,	mfPUSHDINS, 	mfPUSHLINS, 
	//65
	mfPUSHRINS,	mfBLOCKTRIGGER,	mfNOBLOCKS, 	mfBRANG, 	mfMBRANG,
	//70
	mfFBRANG, 	mfSARROW, 	mfGARROW, 	mfSTRONGFIRE, 	mfMAGICFIRE, 
	//75
	mfDIVINEFIRE,	mfWANDMAGIC, 	mfREFMAGIC, 	mfREFFIREBALL, 	mfSWORD, 
	//80
	mfWSWORD,	mfMSWORD, 	mfXSWORD, 	mfSWORDBEAM, 	mfWSWORDBEAM, 
	//85
	mfMSWORDBEAM,	mfXSWORDBEAM, 	mfHOOKSHOT, 	mfWAND, 	mfHAMMER, 
	//90
	mfSTRIKE, 	mfBLOCKHOLE,	mfMAGICFAIRY, 	mfALLFAIRY, 	mfSINGLE, 
	//95
	mfSINGLE16,	mfNOENEMY, 	mfNOGROUNDENEMY, mfSCRIPT1, 	mfSCRIPT2, 
	//100
	mfSCRIPT3, 	mfSCRIPT4, 	mfSCRIPT5,	mfRAFT_BOUNCE, 	mfPUSHED,
	//105
	mfSCRIPT6, 	mfSCRIPT7, 	mfSCRIPT8, 	mfSCRIPT9, 	mfSCRIPT10, 
	//110
	mfSCRIPT11, 	mfSCRIPT12, 	mfSCRIPT13, 	mfSCRIPT14, 	mfSCRIPT15,
	//115
	mfSCRIPT16, 	mfSCRIPT17, 	mfSCRIPT18, 	mfSCRIPT19, 	mfSCRIPT20,
	//120
	mfPITHOLE, 	mfPITFALLFLOOR,	mfLAVA, 	mfICE, 		mfICEDAMAGE, 
	//125
	mfDAMAGE1, 	mfDAMAGE2, 	mfDAMAGE4, 	mfDAMAGE8, 	mfDAMAGE16, 
	//130
	mfDAMAGE32,	mfFREEZEALL, 	mfFREZEALLANSFFCS, mfFREEZEFFCSOLY, mfSCRITPTW1TRIG, 
	//135
	mfSCRITPTW2TRIG, mfSCRITPTW3TRIG, mfSCRITPTW4TRIG, mfSCRITPTW5TRIG, mfSCRITPTW6TRIG, 
	//140
	mfSCRITPTW7TRIG, mfSCRITPTW8TRIG, mfSCRITPTW9TRIG, mfSCRITPTW10TRIG, mfTROWEL, 
	//145
	mfTROWELNEXT, mfTROWELSPECIALITEM,mfSLASHPOT, 	mfLIFTPOT,	mfLIFTORSLASH, 
	//150
	mfLIFTROCK, 	mfLIFTROCKHEAVY, mfDROPITEM, 	mfSPECIALITEM, 	mfDROPKEY, 
	//155
	mfDROPLKEY, 	mfDROPCOMPASS, 	mfDROPMAP, 	mfDROPBOSSKEY, mfSPAWNNPC, 
	//160
	mfSWITCHHOOK, 	mfSIDEVIEWLADDER, mfSIDEVIEWPLATFORM, mfNOENEMYSPAWN, mfENEMYALL,
	//165
	mfSECRETSNEXT, mfNOMIRROR, mfUNSAFEGROUND, mf168, mf169, mf170, mf171, mf172, mf173, mf174,
    mf175, mf176, mf177, mf178, mf179, mf180, mf181, mf182, mf183, mf184, mf185, mf186, mf187, 
    mf188, mf189, mf190, mf191, mf192, mf193, mf194, mf195, mf196, mf197, mf198, mf199, mf200,
    mf201, mf202, mf203, mf204, mf205, mf206, mf207, mf208, mf209, mf210, mf211, mf212, mf213,
    mf214, mf215, mf216, mf217, mf218, mf219, mf220, mf221, mf222, mf223, mf224, mf225, mf226, 
    mf227, mf228, mf229, mf230, mf231, mf232, mf233, mf234, mf235, mf236, mf237, mf238, mf239,
    mf240, mf241, mf242, mf243, mf244, mf245, mf246, mf247, mf248, mf249, mf250, mf251, mf252, mf253, mf254, 
    mfEXTENDED, mfMAX //256 for array sizing
    
    //mfSLASHPOTSPECIALITEM, //stack flags to make a drop or special item
     //mfLIFTPOTSPECIAL, 
};

// combo types
//combo name strings come from defdata
enum
{
    cNONE, cSTAIR, cCAVE, cWATER, cARMOS, 
	//5
	cGRAVE, cDOCK, cUNDEF, cPUSH_WAIT, cPUSH_HEAVY, 
	//10
	cPUSH_HW, cL_STATUE, cR_STATUE, cWALKSLOW, cCVUP, 
	//15
	cCVDOWN, cCVLEFT, cCVRIGHT, cSWIMWARP, cDIVEWARP,
	//20
	cLADDERHOOKSHOT, cTRIGNOFLAG, cTRIGFLAG, cZELDA, cSLASH, 
	//25
	cSLASHITEM, cPUSH_HEAVY2, cPUSH_HW2, cPOUND, cHSGRAB, 
	//30
	cHSBRIDGE, cDAMAGE1, cDAMAGE2, cDAMAGE3, cDAMAGE4, 
	//35
	cC_STATUE, cTRAP_H, cTRAP_V, cTRAP_4, cTRAP_LR, 
	//40
	cTRAP_UD, cPIT, cHOOKSHOTONLY, cOVERHEAD, cNOFLYZONE, 
	//45
	cMIRROR, cMIRRORSLASH, cMIRRORBACKSLASH, cMAGICPRISM, cMAGICPRISM4,
	//50
	cMAGICSPONGE, cCAVE2, cEYEBALL_A, cEYEBALL_B, cNOJUMPZONE, 
	//55
	cBUSH, cFLOWERS, cTALLGRASS, cSHALLOWWATER, cLOCKBLOCK, 
	//60
	cLOCKBLOCK2, cBOSSLOCKBLOCK, cBOSSLOCKBLOCK2, cLADDERONLY, cBSGRAVE,
	//65
	cCHEST, cCHEST2, cLOCKEDCHEST, cLOCKEDCHEST2, cBOSSCHEST, 
	//70
	cBOSSCHEST2, cRESET, cSAVE, cSAVE2, /*cVERTICAL,*/ cCAVEB, 
	//75
	cCAVEC, cCAVED, cSTAIRB, cSTAIRC, cSTAIRD, 
	//80
	cPITB, cPITC, cPITD, cCAVE2B, cCAVE2C, 
	//85
	cCAVE2D, cSWIMWARPB, cSWIMWARPC, cSWIMWARPD, cDIVEWARPB, 
	//90
	cDIVEWARPC, cDIVEWARPD, cSTAIRR, cPITR, cAWARPA, 
	//95
	cAWARPB, cAWARPC, cAWARPD, cAWARPR, cSWARPA, 
	//100
	cSWARPB, cSWARPC, cSWARPD, cSWARPR, cSTRIGNOFLAG, 
	//105
	cSTRIGFLAG, cSTEP, cSTEPSAME, cSTEPALL, cSTEPCOPY, 
	//110
	cNOENEMY, cBLOCKARROW1, cBLOCKARROW2, cBLOCKARROW3, cBLOCKBRANG1, 
	//115
	cBLOCKBRANG2, cBLOCKBRANG3, cBLOCKSBEAM, cBLOCKALL, cBLOCKFIREBALL, 
	//120
	cDAMAGE5, cDAMAGE6, cDAMAGE7, cCHANGE/**DEPRECATED**/, cSPINTILE1, 
	//125
	cSPINTILE2, cSCREENFREEZE, cSCREENFREEZEFF, cNOGROUNDENEMY, cSLASHNEXT, 
	//130
	cSLASHNEXTITEM, cBUSHNEXT, cSLASHTOUCHY, cSLASHITEMTOUCHY, cBUSHTOUCHY, 
	//135
	cFLOWERSTOUCHY, cTALLGRASSTOUCHY, cSLASHNEXTTOUCHY, cSLASHNEXTITEMTOUCHY, cBUSHNEXTTOUCHY, 
	//140
	cEYEBALL_4, cTALLGRASSNEXT, cSCRIPT1, cSCRIPT2, cSCRIPT3, 
	//145
	cSCRIPT4, cSCRIPT5, cSCRIPT6, cSCRIPT7, cSCRIPT8, 
	//150
	cSCRIPT9, cSCRIPT10, cSCRIPT11, cSCRIPT12, cSCRIPT13, 
	//155
	cSCRIPT14, cSCRIPT15, cSCRIPT16, cSCRIPT17, cSCRIPT18, 
	//160
	cSCRIPT19, cSCRIPT20, cTRIGGERGENERIC, cPITFALL, cSTEPSFX,
	//165
	cBRIDGE, cSIGNPOST, cCSWITCH, cCSWITCHBLOCK, cTORCH,
	//170
	cSPOTLIGHT, cGLASS, cLIGHTTARGET, cSWITCHHOOK, cBUTTONPROMPT,
	//175
	cCUSTOMBLOCK, cSHOOTER, cSLOPE, cCUTSCENETRIG, cPUSHBLOCK,
	//180
	cICY, cMIRRORNEW, cCRUMBLE,
    cMAX,
	// ! potential new stuff that I might decide it is worth adding. 
    //Five additional user script types, 
    
    //165
    cDIG, cDIGNEXT, cDIGITEM, //Dig a hole, dig a hole.
    
      /* pot, or rock:
		lift and slash combos need a 'break' sprite set in the combo editor. 
		The lifted object also needs a 'damage enemies' value, so that when thrown
		it makes an lweapon. The leweapon type, and damage should be inputs
		in the combo editor. -1 to damage means enemies ignore it. 
		-1 to weapon means that itdoesn't generate a weapon.
	
		The default type should be LW_POT, which we need to add, or LW_ROCK. 
		We should add both of these. 
		
		These should also include a break sound, a throw sound, and possibly other
		attributes such as throw distance. Sideview behaviour must be very carefully 
		defined. -Z
		
	*/
    //168
    //normal (use undercombo), lift and generate drop item, lift and generate special item (screen item)
    cLIFT, cLIFTITEM, cLIFTSPECITER, 
    //171
    //lift and show next combo, same plus item drop, same with room item
    cLIFTNEXT, cLIFTNEXTITEM, cLIFTNEXTSPECITEM,
    //174
    //lift or slash (do not change combo; use undercombo), ...and make drop item, ...and make room item
    cLIFTSLASH, cLIFTSLAHITEM, cLIFTSLASHSPECITEM, 
    //177
    //lift or shash, then change the combo at its position to the 'next combo', ...
	//...and make drop, ...and make screen room item
    cLIFTSLASHNEXT, cLIFTSLASHNEXTITEM, cLIFTSLASHNEXTSPECITEM, 
    //180

    cBREAKAWAYFLOOR, //as combo cycling to pit, but it makes a sound? Might be useful? 
    //181
    cFREEZEFFCONLY, //freezes only scripts
    //182
    // cSYS183, cSYS184, cSYS185, cSYS186, cSYS187, cSYS188, cSYS189, cSYS190,
    // cSYS191, cSYS192, cSYS193, cSYS194, cSYS195, cSYS196, cSYS197, cSYS198, cSYS199, cSYS200, 
    // cSYS201, cSYS202, cSYS203, cSYS204, cSYS205, cSYS206, cSYS207, cSYS208, cSYS209, cSYS210,
    // cSYS211, cSYS212, cSYS213, cSYS214, cSYS215, cSYS216, cSYS217, cSYS218, cSYS219, cSYS220,
    // cSYS221, cSYS222, cSYS223, cSYS224, cSYS225, cSYS226, cSYS227, cSYS228, cSYS229, cSYS230, 
    // cSYS231, cSYS232, cSYS233, cSYS234, cSYS235, cSYS236, cSYS237, cSYS238, cSYS239, cSYS240, 
    // cSYS241, cSYS242, cSYS243, cSYS244, cSYS245, cSYS246, cSYS247, cSYS248, cSYS249, cSYS250,
    // cSYS251, cSYS252, cSYS253, cSYS254,
    //Should be 255
    cEXPANDED=255, //Set to this, and then select an expansion[0] type for the 'oombo editor'.
    
    //2.54, these would need filepack changes. Needs V_COMBOS.
    cMAX250 //Old max, to make filepack handling easier. 
    
    //These need to be in a new, index of expansion[]. 
    //Let's use expansion[0] for now. 
    
    //cMAX
};

#define PITFALL_FALL_FRAMES 70
#define WATER_DROWN_FRAMES 64

//Combo editor and additional system combos for combo type 'expanded'.

//We could also change .type from a byte to a word, and typecast (byte)combo.type for old quests. 
//That would make 2.50 exporting harder, though. 
enum 
{
	cUSER000, cUSER001, cUSER002, cUSER003, cUSER004, cUSER005, cUSER006, cUSER007, cUSER008, cUSER009,
    cUSER010, cUSER011, cUSER012, cUSER013, cUSER014, cUSER015, cUSER016, cUSER017, cUSER018, cUSER019,
    cUSER020, cUSER021, cUSER022, cUSER023, cUSER024, cUSER025, cUSER026, cUSER027, cUSER028, cUSER029,
    cUSER030, cUSER031, cUSER032, cUSER033, cUSER034, cUSER035, cUSER036, cUSER037, cUSER038, cUSER039,
    cUSER040, cUSER041, cUSER042, cUSER043, cUSER044, cUSER045, cUSER046, cUSER047, cUSER048, cUSER049,
    cUSER050, cUSER051, cUSER052, cUSER053, cUSER054, cUSER055, cUSER056, cUSER057, cUSER058, cUSER059,
    cUSER060, cUSER061, cUSER062, cUSER063, cUSER064, cUSER065, cUSER066, cUSER067, cUSER068, cUSER069,
    cUSER070, cUSER071, cUSER072, cUSER073, cUSER074, cUSER075, cUSER076, cUSER077, cUSER078, cUSER079,
    cUSER080, cUSER081, cUSER082, cUSER083, cUSER084, cUSER085, cUSER086, cUSER087, cUSER088, cUSER089,
    cUSER090, cUSER091, cUSER092, cUSER093, cUSER094, cUSER095, cUSER096, cUSER097, cUSER098, cUSER099,
    cUSER100, cUSER101, cUSER102, cUSER103, cUSER104, cUSER105, cUSER106, cUSER107, cUSER108, cUSER109,
    cUSER110, cUSER111, cUSER112, cUSER113, cUSER114, cUSER115, cUSER116, cUSER117, cUSER118, cUSER119,
    cUSER120, cUSER121, cUSER122, cUSER123, cUSER124, cUSER125, cUSER126, cUSER127,
    
    cSYSTEM000, cSYSTEM001, cSYSTEM002, cSYSTEM003, cSYSTEM004, cSYSTEM005, cSYSTEM006, cSYSTEM007, cSYSTEM008, cSYSTEM009,
    cSYSTEM010, cSYSTEM011, cSYSTEM012, cSYSTEM013, cSYSTEM014, cSYSTEM015, cSYSTEM016, cSYSTEM017, cSYSTEM018, cSYSTEM019,
    cSYSTEM020, cSYSTEM021, cSYSTEM022, cSYSTEM023, cSYSTEM024, cSYSTEM025, cSYSTEM026, cSYSTEM027, cSYSTEM028, cSYSTEM029,
    cSYSTEM030, cSYSTEM031, cSYSTEM032, cSYSTEM033, cSYSTEM034, cSYSTEM035, cSYSTEM036, cSYSTEM037, cSYSTEM038, cSYSTEM039,
    cSYSTEM040, cSYSTEM041, cSYSTEM042, cSYSTEM043, cSYSTEM044, cSYSTEM045, cSYSTEM046, cSYSTEM047, cSYSTEM048, cSYSTEM049,
    cSYSTEM050, cSYSTEM051, cSYSTEM052, cSYSTEM053, cSYSTEM054, cSYSTEM055, cSYSTEM056, cSYSTEM057, cSYSTEM058, cSYSTEM059,
    cSYSTEM060, cSYSTEM061, cSYSTEM062, cSYSTEM063, cSYSTEM064, cSYSTEM065, cSYSTEM066, cSYSTEM067, cSYSTEM068, cSYSTEM069,
    cSYSTEM070, cSYSTEM071, cSYSTEM072, cSYSTEM073, cSYSTEM074, cSYSTEM075, cSYSTEM076, cSYSTEM077, cSYSTEM078, cSYSTEM079,
    cSYSTEM080, cSYSTEM081, cSYSTEM082, cSYSTEM083, cSYSTEM084, cSYSTEM085, cSYSTEM086, cSYSTEM087, cSYSTEM088, cSYSTEM089,
    cSYSTEM090, cSYSTEM091, cSYSTEM092, cSYSTEM093, cSYSTEM094, cSYSTEM095, cSYSTEM096, cSYSTEM097, cSYSTEM098, cSYSTEM099,
    cSYSTEM100, cSYSTEM101, cSYSTEM102, cSYSTEM103, cSYSTEM104, cSYSTEM105, cSYSTEM106, cSYSTEM107, cSYSTEM108, cSYSTEM109,
    cSYSTEM110, cSYSTEM111, cSYSTEM112, cSYSTEM113, cSYSTEM114, cSYSTEM115, cSYSTEM116, cSYSTEM117, cSYSTEM118, cSYSTEM119,
    cSYSTEM120, cSYSTEM121, cSYSTEM122, cSYSTEM123, cSYSTEM124, cSYSTEM125, cSYSTEM126, cSYSTEM127,
    cEXPANDED_MAX	
};

#define lensflagSHOWHINTS = 0x0010;
#define lensflagHIDESECRETS = 0x0020;
#define lensflagNOXRAY = 0x0400;
#define lensflagSHOWRAFTPATHS = 0x0800;
#define lensflagSHOWINVISENEMIES = 0x1000;


enum 
{
	USR_MIDI_DEATH, USR_MIDI_GAMEOVER, USR_MIDI_OVERWORLD, USR_MIDI_DUNGEON, USR_MIDI_LEVELNINE, USR_MIDI_MAX 
	
}; //Obviously, the overworld, dungeon and L9 midis don;t need to be here, but we can define some user space for o
//	otherwise hardcoded sounds, and use these instead of hardcoded SFX if they are set somehow. 

// refill stuff
enum { REFILL_NONE, REFILL_FAIRYDONE, REFILL_LIFE, REFILL_MAGIC, REFILL_ALL};
#define REFILL_FAIRY -1

// 2.55 lighting stuff
enum { pal_litDEFAULT, pal_litOVERRIDE, pal_litRESET, pal_litSET, pal_litRESETONLY };

//Z-axis related
#define FEATHERJUMP 80

//other
#define MAXDRUNKCLOCK          500

enum
{
	dBUSHLEAVES, dFLOWERCLIPPINGS, dGRASSCLIPPINGS, dHAMMERSMACK,
	dTALLGRASS, dRIPPLES, dDIVINEPROTECTIONSHIELD, dHOVER, dCOMBOSPRITE,
	dCUSTOMWALK, dSTATUSSPRITE,
	dMAXDECORATIONS
};

// items
enum                                                        // value matters because of current quest file data
{
    iRupy, i5Rupies, iHeart, iBombs, iClock,
    iSword, iWSword, iMSword, iShield, iKey,
    // 10
    iBCandle, iRCandle, iLetter, iArrow, iSArrow,
    iBow, iBait, iBRing, iRRing, iBracelet,
    // 20
    iTriforce, iMap, iCompass, iBrang, iMBrang,
    iWand, iRaft,iLadder,iHeartC, iBPotion,
    // 30
    iRPotion, iWhistle,iBook, iMKey, iFairyMoving,
    iFBrang, iXSword, iMShield, i20Rupies, i50Rupies,
    // 40
    i200Rupies, iWallet500, iWallet999, iPile, iBigTri,
    iSelectA, iMisc1, iMisc2, iSBomb, iHCPiece,
    // 50
    iAmulet, iFlippers, iHookshot, iLens, iHammer,
    iBoots, iL2Bracelet, iGArrow, iMagicC, iSMagic,
    // 60
    iLMagic, iGRing, iKillAll, iL2Amulet, iDivineFire,
    iDivineEscape, iDivineProtection, iBossKey, iBow2, iFairyStill,
    // 70
    i1ArrowAmmo, i5ArrowAmmo, i10ArrowAmmo, i30ArrowAmmo, iQuiver,
    iQuiverL2, iQuiverL3, i1BombAmmo, i4BombAmmo, i8BombAmmo,
    // 80
    i30BombAmmo, iBombBag, iBombBagL2, iBombBagL3, iLevelKey,
    iSelectB, i10Rupies, i100Rupies, iCByrna, iLongshot,
    // 90
    iLetterUsed,iRocsFeather,iHoverBoots,iSShield,iSpinScroll,
    iCrossScroll,iQuakeScroll,iL2QuakeScroll,iL2SpinScroll,iWhispRing,
    // 100
    iL2WhispRing, iChargeRing, iL2ChargeRing, iPerilScroll, iWalletL3,
    iQuiverL4, iBombBagL4, iOldGlove, iL2Ladder,iWealthMedal,
    // 110
    iL2WealthMedal, iL3WealthMedal,iHeartRing, iL2HeartRing, iL3HeartRing,
    iMagicRing, iL2MagicRing, iL3MagicRing, iL4MagicRing, iStoneAgony,
    // 120
    iStompBoots, iPerilRing, iWhimsicalRing,
    iCustom1, iCustom2, iCustom3, iCustom4, iCustom5,
    iCustom6, iCustom7, iCustom8, iCustom9, iCustom10,
    iCustom11, iCustom12, iCustom13, iCustom14, iCustom15,
    iCustom16, iCustom17, iCustom18, iCustom19, iCustom20,
    iLast
};

// item sets
enum
{
    isNONE, isDEFAULT, isBOMBS, isMONEY, isLIFE, isBOMB100, isSBOMB100,
    isMAGIC, isMAGICBOMBS, isMAGICMONEY, isMAGICLIFE, isMAGIC2, isTALLGRASS, isMAX
};

// weapons (in qst data)
enum
{
    wSWORD, wWSWORD, wMSWORD, wXSWORD, wBRANG,
    wMBRANG, wFBRANG, wBOMB, wSBOMB, wBOOM,
// 10
    wARROW, wSARROW, wFIRE, wWIND, wBAIT,
    wWAND, wMAGIC, ewFIREBALL, ewROCK, ewARROW,
// 20
    ewSWORD, ewMAGIC, iwSpawn, iwDeath, iwSwim,
    wHAMMER, wHSHEAD, wHSCHAIN_H, wHSHANDLE, wSSPARKLE,
// 30
    wGSPARKLE, wMSPARKLE, wFSPARKLE, iwHammerSmack, wGARROW,
    ewFLAME, ewWIND, iwMMeter, wDIVINEFIRE1A, wDIVINEFIRE1B,
// 40
    wDIVINEFIRES1A, wDIVINEFIRES1B, wHSCHAIN_V, iwMore, iwBossMarker,
    iwHeroSlash, wSWORDSLASH, wWSWORDSLASH, wMSWORDSLASH, wXSWORDSLASH,
// 50
    iwShadow, iwLargeShadow, iwBushLeaves, iwFlowerClippings, iwGrassClippings,
    iwTallGrass, iwRipples, iwNPCs, wDIVINEPROTECTION1A, wDIVINEPROTECTION1B,
// 60
    wDIVINEPROTECTIONS1A, wDIVINEPROTECTIONS1B, wDIVINEPROTECTION2A, wDIVINEPROTECTION2B, wDIVINEPROTECTIONS2A,
    wDIVINEPROTECTIONS2B, iwDivineProtectionShieldFront, iwDivineProtectionShieldBack, iwSubscreenVine, wCBYRNA,
// 70
    wCBYRNASLASH, wLSHEAD, wLSCHAIN_H, wLSHANDLE, wLSCHAIN_V,
    wSBOOM, ewBOMB, ewSBOMB, ewBOOM, ewSBOOM,
// 80
    ewFIRETRAIL, ewFLAME2, ewFLAME2TRAIL, ewICE, iwHover,
    wFIREMAGIC, iwQuarterHearts, wCBYRNAORB, //iwSideLadder, iwSideRaft,
    
    wLast, wSCRIPT, wMAX=256
};

//Types of hit detection
#define HIT_BY_NPC 0
#define HIT_BY_EWEAPON 1
#define HIT_BY_LWEAPON 2
#define HIT_BY_FFC 3

#define HIT_BY_NPC_UID 4
#define HIT_BY_EWEAPON_UID 5
#define HIT_BY_LWEAPON_UID 6
#define HIT_BY_FFC_UID 7

#define HIT_BY_LWEAPON_PARENT_ID 8
#define HIT_BY_LWEAPON_PARENT_FAMILY 9

#define HIT_BY_NPC_TYPE 10
#define HIT_BY_EWEAPON_TYPE 11
#define HIT_BY_LWEAPON_TYPE 12
#define HIT_BY_NPC_ENGINE_UID 13
#define HIT_BY_EWEAPON_ENGINE_UID 14
#define HIT_BY_LWEAPON_ENGINE_UID 15
#define HIT_BY_NPC_ID 16


#define NUM_HIT_TYPES_USED 17

// weapon types in game engine
enum
{
    // 0
    wNone,wSword,wBeam,wBrang,
    wBomb,wSBomb,wLitBomb,wLitSBomb,
    // 8
    wArrow,wFire,wWhistle,wBait,
    wWand,wMagic,wCatching,wWind,
    // 16
    wRefMagic,wRefFireball,wRefRock, wHammer,
    wHookshot, wHSHandle, wHSChain, wSSparkle,
    // 24
    wFSparkle, wSmack, wPhantom, wCByrna,
	//28
    wRefBeam, wStomp,
	//30
    lwMax,
    // Dummy weapons - must be between lwMax and wEnemyWeapons!
	//31
    wScript1, wScript2, wScript3, wScript4,
	//35
    wScript5, wScript6, wScript7, wScript8,
	//39
    wScript9, wScript10, wIce, wFlame, //ice rod, fire rod
    wSound, // -Z: sound + defence split == digdogger, sound + one hit kill == pols voice -Z
	wThrown, wPot, //Thrown pot or rock -Z //Just gonna use a single 'wThrown' -Em
	wLit, //Lightning or Electric -Z
	wBombos, wEther, wQuake,// -Z
	wSword180, wSwordLA,
	//52
	wBugNet, wRefArrow, wRefFire, wRefFire2,
    // Enemy weapons
    wEnemyWeapons=128,
    //129
    ewFireball,ewArrow,ewBrang,ewSword,
    ewRock,ewMagic,ewBomb,ewSBomb,
    //137
    ewLitBomb,ewLitSBomb,ewFireTrail,ewFlame,
    ewWind,ewFlame2,ewFlame2Trail,
    //145
    ewIce,ewFireball2,
    wMax
};

enum defWpnSprite
{
	//0
	ws_0,
	wsBeam = 0,
	wsRefBeam = 1,
	wsSword2 = 1,
	wsSword3,
	wsSword4,
	wsBrang,
	wsBrang2,
	wsBrang3,
	wsBomb,
	wsSBomb,
	wsBombblast,
	//10
	wsArrow,
	wsArrow2,
	wsFire,
	wsWind,
	wsBait,
	wsWandHandle,
	wsMagic,
	wsRefFireball,
	wsFireball = 17,
	wsFireball2 = 17,
	wsRock,
	wsEArrow,
	//20
	wsEBeam,
	wsRefMagic,
	wsEMagic = 21,
	wsSpawn,
	wsDeath,
	wsUnused24,
	wsHammer,
	wsHookshotHead,
	wsHookshotChainH, 
	wsHookshotHandle,
	wsFireSparkle, //silver sparkle
	wsSilverSparkle = 29, //silver sparkle
	//30
	wsGoldSparkle,
	wsBrang2Sparkle,
	wsBrang3Sparkle,
	wsHammerSmack,
	wArrow3,
	wsEFire,
	wsEWind,
	wsMagicGauge,
	wsDFireFalling,
	wsDFireRising,
	//40
	wsDFireTrailRising,
	wsDFireTrailFalling,
	wsHookshotChainV,
	wsMore,
	wsUnused44,
	wsUnused45,
	wsSword1Slash,
	wsSword2Slash,
	wsSword3Slash,
	wsSword4Slash,
	//50
	wsShadow,
	wsShadowLarge,
	wsBushLeaves,
	wsFlowerClippings,
	wsGrassGlippings,
	wsTallGrass,
	wsRipples,
	wsUnused57,
	wsNayLoveLeft,
	wsNayLoveLeftReturn,
	//60
	wsNayLoveTrailLeft,
	wsNayLoveTrailLeftReturn,
	wsNayLoveRight,
	wsNayLoveRightReturn,
	wsNayLoveTrailRight,
	wsNayLoveTrailRightReturn,
	wsNayLoveShieldFront,
	wsNayLoveShieldBack,
	wsSubscreenVine,
	wsByrnaCane,
	//70
	wsByrnaSlash,
	wsLongshotHead,
	wsLongshotChainH,
	wsLongshotHandle,
	wsLongshotChainV,
	wsSbombblast,
	wsEBomb,
	wsESbomb,
	wsEBombblast,
	wsESbombblast,
	//80
	wsEFiretrail,
	wsEFire2,
	wsEFiretrail2,
	wsIce,
	wsHoverboots,
	wsMagicFire,
	wsQuarterHeartPieces,
	wsByrnaBeam,
	wsFirework,
	wsLast
};

// phantom weapon types
enum
{
    pDIVINEFIREROCKET, pDIVINEFIREROCKETRETURN, pDIVINEFIREROCKETTRAIL, pDIVINEFIREROCKETTRAILRETURN, pMESSAGEMORE,
    pDIVINEPROTECTIONROCKET1, pDIVINEPROTECTIONROCKETRETURN1, pDIVINEPROTECTIONROCKETTRAIL1, pDIVINEPROTECTIONROCKETTRAILRETURN1,
    pDIVINEPROTECTIONROCKET2, pDIVINEPROTECTIONROCKETRETURN2, pDIVINEPROTECTIONROCKETTRAIL2, pDIVINEPROTECTIONROCKETTRAILRETURN2
};

enum
{
    eNONE=0,
    gNONE=0, gABEI, gAMA, gDUDE, gMOBLIN,
    gFIRE, gFAIRY, gGORIYA, gZELDA, gABEI2,
    //10
    gEMPTY, gDUMMY1, gDUMMY2, gDUMMY3, gDUMMY4,
    gDUMMY5, gDUMMY6, gDUMMY7, gDUMMY8, gDUMMY9,
    //20
    eSTART=20,
    eOCTO1S=20, eOCTO2S, eOCTO1F, eOCTO2F, eTEK1,
    eTEK2, eLEV1, eLEV2, eMOBLIN1, eMOBLIN2,
    //30
    eLYNEL1, eLYNEL2, ePEAHAT, eZORA, eROCK,
    eGHINI1, eGHINI2, eARMOS, eKEESE1, eKEESE2,
    //40
    eKEESE3, eSTALFOS, eGEL, eZOL, eROPE,
    eGORIYA1, eGORIYA2, eTRAP, eWALLM, eDKNUT1,
    //50
    eDKNUT2, eBUBBLEST, eVIRE, eLIKE, eGIBDO,
    ePOLSV, eWIZ1, eWIZ2, eRAQUAM, eMOLDORM,
    //60
    eDODONGO, eMANHAN, eGLEEOK1, eGLEEOK2, eGLEEOK3,
    eGLEEOK4, eDIG1, eDIG3, eDIGPUP1, eDIGPUP2,
    //70
    eDIGPUP3, eDIGPUP4, eGOHMA1, eGOHMA2, eCENT1,
    eCENT2, ePATRA1, ePATRA2, eGANON, eSTALFOS2,
    //80
    eROPE2, eBUBBLESP, eBUBBLESR, eSHOOTFBALL, eITEMFAIRY,
    eFIRE, eOCTO5, eDKNUT5, eGELTRIB, eZOLTRIB,
    //90
    eKEESETRIB, eVIRETRIB, eDKNUT3, eLAQUAM, eMANHAN2,
    eTRAP_H, eTRAP_V, eTRAP_LR, eTRAP_UD, eFWIZ,
    //100
    eWWIZ, eCEILINGM, eFLOORM, ePATRABS, ePATRAL2,
    ePATRAL3, eBAT, eBATROBE, eBATROBEKING, eGLEEOK1F,
    //110
    eGLEEOK2F, eGLEEOK3F, eGLEEOK4F, eMWIZ, eDODONGOBS,
    eDODONGOF, eTRIGGER, eBUBBLEIT, eBUBBLEIP, eBUBBLEIR,
    //120
    eSTALFOS3, eGOHMA3, eGOHMA4, eNPCSTAND1, eNPCSTAND2,
    eNPCSTAND3, eNPCSTAND4, eNPCSTAND5, eNPCSTAND6, eNPCWALK1,
    //130
    eNPCWALK2, eNPCWALK3, eNPCWALK4, eNPCWALK5, eNPCWALK6,
    eBOULDER, eGORIYA3, eLEV3, eOCTO3S, eOCTO3F,
    //140
    eOCTO4S, eOCTO4F, eTRAP_8WAY, eTRAP_DIAGONAL, eTRAP_SLASH_C,
    eTRAP_SLASH_LOS, eTRAP_BACKSLASH_C, eTRAP_BACKSLASH_LOS, eTRAP_CW_C, eTRAP_CW_LOS,
    //150
    eTRAP_CCW_C, eTRAP_CCW_LOS, eSUMMONER, eIWIZ, eSHOOTMAGIC,
    eSHOOTROCK, eSHOOTSPEAR, eSHOOTSWORD, eSHOOTFLAME, eSHOOTFLAME2,
    //160
    eBOMBCHU, eFGEL, eFZOL, eFGELTRIB, eFZOLTRIB,
    eTEK3, eSPINTILE1, eSPINTILE2, eLYNEL3, eFPEAHAT,
    //170
    eMPOLSV, eWPOLSV, eDKNUT4, eFGHINI, eMGHINI,
    eGRAPBUGHP, eGRAPBUGMP, e177,
    
    eMAXGUYS = MAXGUYS
};

#define OLDMAXGUYS	e177

// enemy families
enum
{
    eeGUY=0, eeWALK,
    eeSHOOT/*DEPRECATED*/,
    eeTEK, eeLEV, eePEAHAT, eeZORA, eeROCK,
	//8
    eeGHINI, eeARMOS/*DEPRECATED*/, eeKEESE, eeGEL/*DEPRECATED*/, eeZOL/*DEPRECATED*/, eeROPE/*DEPRECATED*/, eeGORIYA/*DEPRECATED*/, eeTRAP,
	//16
    eeWALLM, eeBUBBLE/*DEPRECATED*/, eeVIRE/*DEPRECATED*/, eeLIKE/*DEPRECATED*/, eePOLSV/*DEPRECATED*/, eeWIZZ, eeAQUA, eeMOLD,
	//24
    eeDONGO, eeMANHAN, eeGLEEOK, eeDIG, eeGHOMA, eeLANM, eePATRA, eeGANON,
	//32
    eePROJECTILE, eeGELTRIB/*DEPRECATED*/, eeZOLTRIB/*DEPRECATED*/, eeVIRETRIB/*DEPRECATED*/, eeKEESETRIB/*DEPRECATED*/, eeSPINTILE, eeNONE,
	//39
    eeFAIRY, eeFIRE, eeOTHER, eeMAX250, //eeFire is Other (Floating), eeOther is Other in the Editor.
    eeSCRIPT01, eeSCRIPT02, eeSCRIPT03, eeSCRIPT04, eeSCRIPT05, eeSCRIPT06, eeSCRIPT07, eeSCRIPT08, eeSCRIPT09, eeSCRIPT10,
    eeSCRIPT11, eeSCRIPT12, eeSCRIPT13, eeSCRIPT14, eeSCRIPT15, eeSCRIPT16, eeSCRIPT17, eeSCRIPT18, eeSCRIPT19, eeSCRIPT20,
    eeFFRIENDLY01, eeFFRIENDLY02, eeFFRIENDLY03, eeFFRIENDLY04, eeFFRIENDLY05, eeFFRIENDLY06, eeFFRIENDLY07, eeFFRIENDLY08,
    eeFFRIENDLY09, eeFFRIENDLY10,
    eeMAX
};

// enemy animation styles
enum
{
    aNONE, aFLIP, aUNUSED1, a2FRM, aUNUSED2,
    aOCTO, aTEK, aLEV, aWALK, aZORA,
    aNEWZORA, aGHINI, aARMOS, aROPE, aWALLM,
    aNEWWALLM, aDWALK, aVIRE, a3FRM, aWIZZ,
    aAQUA, aDONGO, aMANHAN, aGLEEOK, aDIG,
    aGHOMA, aLANM, a2FRMPOS, a4FRM4EYE,a4FRM8EYE,
    a4FRM4DIRF, a4FRM4DIR, a4FRM8DIRF, aARMOS4, a4FRMPOS4DIR,
    a4FRMPOS8DIR, aUNUSED3, a4FRM8DIRB, aNEWTEK, a3FRM4DIR,
    a2FRM4DIR, aNEWLEV, a2FRM4EYE, aNEWWIZZ, aNEWDONGO,
    aDONGOBS, a4FRMPOS8DIRF, a4FRMPOS4DIRF, a4FRMNODIR, aGANON, a2FRMB, 
    a4FRM8EYEB, a4FRM4EYEB, a4FRM8DIRFB, a4FRM4DIRB, a4FRM4DIRFB, aMAX
};
// deprecated styles
#define aFLIPSLOW 1
#define a2FRMSLOW 3
#define aNEWDWALK 42
#define aNEWPOLV 39
#define a4FRM3TRAP 36

// Enemy misc1 types
enum { e1tNORMAL, e1tEACHTILE, e1tCONSTANT, e1tHOMINGBRANG=2, e1tFAST, e1tSLANT, e1t3SHOTS, e1t4SHOTS, e1t5SHOTS, e1t3SHOTSFAST, e1tFIREOCTO, e1t8SHOTS, e1tSUMMON, e1tSUMMONLAYER, e1tLAST };
// Enemy misc2 types
enum { e2tNORMAL, e2tSPLITHIT, e2tSPLIT, e2tFIREOCTO, e2tBOMBCHU, e2tTRIBBLE, e2tLAST };
#define e2tKEESETRIB 1

// Enemy misc7 types
enum { e7tNORMAL, e7tTEMPJINX, e7tPERMJINX, e7tUNJINX, e7tTAKEMAGIC, e7tTAKERUPEES, e7tDRUNK,
// all from this point involve engulfing
       e7tEATITEMS, e7tEATMAGIC, e7tEATRUPEES, e7tEATHURT,
// all from this point involve dragging
       e7tWALLMASTER, e7tLAST
     };

// Enemy misc8 types
enum { e8tSWORD=0x1, e8tITEM=0x2, e8tSHIELD=0x4, e8tLAST=0x8};

// Enemy misc9 types
// Walker
enum { e9tNORMAL, e9tROPE, e9tVIRE, e9tPOLSVOICE, e9tARMOS,
// remainder unimplemented
       e9tLEEVER, e9tZ3LEEVER, e9tZ3WALK, e9tZ3STALFOS, e9tLAST
     };

enum { patrat1SHOT, patrat1SHOTFAST, patrat3SHOT, patrat3SHOTFAST, patrat5SHOT, patrat5SHOTFAST, patrat4SHOTCARD, patrat4SHOTDIAG, patrat4SHOTRAND, patrat8SHOT, patratBREATH, patratSTREAM, patratLAST };

/*
// Floater
enum { e9tPEAHAT=1, e9tGHINI };
// Teleporter
enum { e9tBATROBE=1, e9tZ3WIZZROBE };
// Traps
enum { e9tAUTOTRAP=1 };
// Moldorm
enum {  e9tMOLDORM=1, e9tVLANMOLA, e9tVMOLDORM, e9tZ3MOLDORM, //restricted to walkable combos
	e9tTAIL, //only tail is vulnerable
	e9tMINIMOLD, //doesn't segment
	e9tBIGTAIL //doesn't segment, only tail is vulnerable
     };
*/

// New defence outcomes. 
enum
{
    edNORMAL, // : IMPLEMENTED : Take damage (or stun)
    edHALFDAMAGE, // : IMPLEMENTED : Take half damage
    edQUARTDAMAGE, // : IMPLEMENTED : Take 0.25 damage
    edSTUNONLY, // : IMPLEMENTED : Stun instead of taking damage.
    edSTUNORCHINK, // : IMPLEMENTED : If damage > 0, stun instead. Else, bounce off.
    edSTUNORIGNORE, // : IMPLEMENTED : If damage > 0, stun instead. Else, ignore.
    edCHINKL1, // : IMPLEMENTED : Bounces off, plays SFX_CHINK
    edCHINKL2, // : IMPLEMENTED : Bounce off unless damage >= 2
    edCHINKL4, //: IMPLEMENTED : Bounce off unless damage >= 4
    edCHINKL6, // : IMPLEMENTED : Bounce off unless damage >= 6
    edCHINKL8, // : IMPLEMENTED : Bounce off unless damage >= 8
    edCHINK, // : IMPLEMENTED : Bounces off, plays SFX_CHINK
    edIGNOREL1, // : IMPLEMENTED : Ignore unless damage > 1.
    edIGNORE, // : IMPLEMENTED : Do Nothing
    ed1HKO, // : IMPLEMENTED : One-hit knock-out
	edCHINKL10, //: IMPLEMENTED : If damage is less than 10
	ed2x, // : IMPLEMENTED : Double damage.
	ed3x, // : IMPLEMENTED : Triple Damage.
	ed4x, // : IMPLEMENTED : 4x damage.
	edHEAL, // : IMPLEMENTED : Gain the weapon damage in HP.
	edTRIGGERSECRETS, // : IMPLEMENTED : Triggers screen secrets.
	edFREEZE, //Freeze solid
	edMSG_NOT_ENABLED, //A message for 'The following are not yet enabled.
	edMSG_LINE, //An entry for the hiriz line in THE zq PULLDOWN
	edLEVELDAMAGE, //Damage * item level
	edLEVELREDUCTION, //Damage / item level
	
	edSPLIT, //: IMPLEMENTED : causes the enemy to split if it has a split attribute
	edREPLACE, //replaced by next in list?
	edLEVELCHINK2, //If item level is < 2: This needs a weapon variable that is set by 
	edLEVELCHINK3, //If item level is < 3: the item that generates it (itemdata::level stored to
	edLEVELCHINK4, //If item level is < 4: weapon::level, or something similar; then a check to
	edLEVELCHINK5, //If item level is < 5: read weapon::level in hit detection. 
	edSHOCK, //buzz blob
	edEXPLODESMALL, //: IMPLEMENTED : ew bombblast
	edEXPLODELARGE, //: IMPLEMENTED : super bomb blast
	edSTONE, //deadrock
	
	edBREAKSHIELD, //break the enemy shield
	edRESTORESHIELD, //if the enemy had a shield, reset it
	edSPECIALDROP, //but where to define it?
	edINCREASECOUNTER, //but where to define the counter
	edREDUCECOUNTER, //same problem
	edEXPLODEHARMLESS, //: IMPLEMENTED : boss death explosion; needs different sprites?
	edKILLNOSPLIT, //If sufficient damage to kill it, a splitting enemy just dies.
	edTRIBBLE, //Tribbles on hit. 
	edFIREBALL, //Makes a 1x1 fireball; Z3 Gibdo
	edFIREBALLLARGE, //Makes a 3x3  Z3 Gibdo for large npcs. 
	edSUMMON, //: IMPLEMENTED : Summons a number of enemies as defined by the summon properties of the npc. 
	//edSAVE, edRETRY, edCRASHZC // Sanity Check Required. -Z
	edWINGAME, //Wand of Gamelon. 
	edJUMP, //Z3 stalfos
	edEATHERO, //-G //Is this practical? We need specisal npc mvoement for it. -Z
	edSHOWMESSAGE, //Shows a ZString when hit. e.g., Z3 Ganon
	edSWITCH, //Switch places with the Hero, as a switchhook does
	
    edLAST
};
#define edX edIGNORE // Shorthand
// Remaining 16 reserved for future use.


// enemy spawn patterns
enum { pRANDOM, pSIDES, pSIDESR, pCEILING, pCEILINGR, pRANDOMR, pNOSPAWN };

// We only use t4Bit and tf8Bit.
// t4Bit takes up 128 bytes in the file format, tf8bit takes up 256 bit.
// They both take 256 bit in memory. t4Bit used to take 128, but this was changed
// to greatly speed up the tile drawing code.
enum { tfInvalid=0, tf4Bit, tf8Bit, tf16Bit, tf24Bit, tf32Bit, tfMax };

struct size_and_pos
{
	bool operator==(const size_and_pos&) const = default;

	int x = -1, y = -1;
	int w = -1, h = -1;
	int xscale = 1, yscale = 1;
	int fw = -1, fh = -1;
	
	int data[8] = {0};
	
	//Get virtual values
	int tw() const;
	int th() const;
	int cx() const;
	int cy() const;
	
	void clear(); //Clear to default vals
	
	bool rect(int mx, int my) const; //Check rect collision
	int rectind(int mx, int my) const; //Check scaled collision
	
	//Set coord values
	void set(int nx, int ny, int nw, int nh);
	void set(int nx, int ny, int nw, int nh, int xs, int ys);
	
	size_and_pos const& subsquare(int ind) const;
	size_and_pos const& subsquare(int col, int row) const;
	size_and_pos const& rel_subsquare(int x, int y, int ind) const;
	size_and_pos const& rel_subsquare(int x, int y, int col, int row) const;
	size_and_pos(int nx = -1, int ny = -1, int nw = -1, int nh = -1, int xsc = 1, int ysc = 1, int fw = -1, int fh = -1);
};

struct tiledata
{
    byte format;
    byte *data;
};

//Weapon editor

//Weapon clocks
enum
{ 
	wpnclkFRAMECOUNT, wpnclkMAGICUSAGE = 9 
};

//Weapon Types
enum
{
	weaptypeNONE, weaptypeSWORD, weaptypeSWORDBEAM, weaptypeBRANG, weaptypeBOMBBLAST,
	weaptypeSBOMBBLAST, weaptypeBOMB, weaptypeSBOMB, weaptypeARROW, weaptypeFIRE,
	weaptypeWHISTLE, weaptypeBAIT, weaptypeWAND, weaptypeMAGIC, weaptypeCANDLE,
	weaptypeWIND, weaptypeREFMAGIC, weaptypeREFFIREBALL, weaptypeREFROCK, weaptypeHAMMER,
	weaptypeHOOKSHOT, weaptype21, weaptype22, weaptypeSPARKLE, weaptype24,
	weaptype25, weaptypeBYRNA, weaptypeREFBEAM, weaptype28, weaptype29,
	weaptypeSCRIPT1, weaptypeSCRIPT2, weaptypeSCRIPT3, weaptypeSCRIPT4, weaptypeSCRIPT5,
	weaptypeSCRIPT6, weaptypeSCRIPT7, weaptypeSCRIPT8, weaptypeSCRIPT9, weaptypeSCRIPT10
};

//Defence types
enum
{
	weapdefNONE, weapdefSWORD, weapdefSWORDBEAM, weapdefBRANG, weapdefBOMBBLAST,
	weapdefSBOMBBLAST, weapdefBOMB, weapdefSBOMB, weapdefARROW, weapdefFIRE,
	weapdefWHISTLE, weapdefBAIT, weapdefWAND, weapdefMAGIC, weapdefCANDLE,
	weapdefWIND, weapdefREFMAGIC, weapdefREFFIREBALL, weapdefREFROCK, weapdefHAMMER,
	weapdefHOOKSHOT, weapdef21, weapdef22, weapdefSPARKLE, weapdef24,
	weapdef25, weapdefBYRNA, weapdefREFBEAM, weapdef28, weapdef29,
	weapdefSCRIPT1, weapdefSCRIPT2, weapdefSCRIPT3, weapdefSCRIPT4, weapdefSCRIPT5,
	weapdefSCRIPT6, weapdefSCRIPT7, weapdefSCRIPT8, weapdefSCRIPT9, weapdefSCRIPT10 
};
	
#define ITEM_MOVEMENT_PATTERNS 10
//Move pattern array indices
enum{
	wpnmovePATTERN, wmoveARG1, wmoveARG2, wmoveARG3, wmoveARG4, wmoveDUPLICATES, wmoveOTHER1
};

//Movement patterns
enum
{
	wmovepatternNONE, wmovepatternLINE, wmovepatternSINE, wmovepatternCOSINE, wmovepatternCIRCLE, wmovepatternARC, wmovepatternPATTERN_A, 
	wmovepatternPATTERN_B, wmovepatternPATTERN_C, wmovepatternPATTERN_D, wmovepatternPATTERN_E, wmovepatternPATTERN_F
};

struct wpndata
{
	int32_t tile;
	byte misc;                                                // 000bvhff (vh:flipping, f:flash (1:NES, 2:BSZ))
	byte csets;                                               // ffffcccc (f:flash cset, c:cset)
	byte frames;                                              // animation frame count
	byte speed;                                               // animation speed
	byte type;                                                // used by certain weapons
	word script;
	
	byte flip() const
	{
		return (misc >> 2) & 0b11;
	}
	byte cs() const
	{
		return csets & 0xF;
	}
};

#define WF_AUTOFLASH  0x01
#define WF_2PFLASH    0x02
#define WF_HFLIP      0x04
#define WF_VFLIP      0x08
#define WF_BEHIND     0x10 //Weapon renders behind other sprites

struct item_drop_object
{
    char name[65];
    word item[10];
    word chance[11]; //0=none
};

//Size Flags
#define OVERRIDE_TILE_WIDTH	0x00000001
#define OVERRIDE_TILE_HEIGHT	0x00000002
#define OVERRIDE_HIT_WIDTH	0x00000004
#define OVERRIDE_HIT_HEIGHT	0x00000008
#define OVERRIDE_HIT_Z_HEIGHT	0x00000010
#define OVERRIDE_HIT_X_OFFSET	0x00000020
#define OVERRIDE_HIT_Y_OFFSET	0x00000040
#define OVERRIDE_DRAW_X_OFFSET	0x00000080
#define OVERRIDE_DRAW_Y_OFFSET	0x00000100
#define OVERRIDE_DRAW_Z_OFFSET	0x00000200

#define MAX_NPC_ATTRIBUTES 32
#define MAX_NPC_FLAGS 64

struct guydata
{
    guy_flags flags;
    int32_t  tile;
    byte  width;
    byte  height; //0=striped, 1+=rectangular
    int32_t  s_tile; //secondary (additional) tile(s)
    byte  s_width;
    byte  s_height;  //0=striped, 1+=rectangular
    int32_t  e_tile;
    byte  e_width;
    byte  e_height;
    
    int16_t hp;
    
    int16_t  family, cset, anim, e_anim, frate, e_frate;
    int16_t  dp, wdp, weapon;
    
    int16_t  rate, hrate, step, homing, grumble, item_set;
    
	int32_t attributes[32]; //refactored this into an array, no more madness.
    int16_t  bgsfx, bosspal, extend;
    byte defense[edefLAST255];
    byte  hitsfx, deadsfx;
    //Add all new guydata variables after this point, if you do not want to edit defdata to fit.
    //Adding earlier will offset defdata arrays. -Z
    
    //2.6 enemy editor tile and hit sizes. -Z
    int32_t xofs,yofs,zofs;
    // no hzofs - it's always equal to zofs.
    int32_t hxofs,hyofs,hxsz,hysz,hzsz;
    int32_t txsz,tysz;
    byte scriptdefense[scriptDEFLAST]; //old 2.future quest file crossover support. 
    int32_t wpnsprite; //wpnsprite is new for 2.6 -Z
    int32_t SIZEflags;; //Flags for size panel offsets. The user must enable these to override defaults. 
    int32_t frozentile, frozencset, frozenclock;
    int16_t frozenmisc[10];
    //v 34
    int16_t firesfx; //weapon fire (attack) sound
    int32_t movement[32]; //Reserved for npc movement types and args. 
    int32_t new_weapon[32]; //Reserved for weapon patterns and args.
    int32_t initD[8];
    
    word script; //For future npc action scripts. 
    //int16_t parentCore; //Probably not needed here. -Z
    int32_t editorflags;
	move_flags moveflags;
    
    char initD_label[8][65];
    char weapon_initD_label[8][65];
    
    word weaponscript;
    int32_t weap_initiald[INITIAL_D];
    
	byte spr_shadow, spr_death, spr_spawn;

	// attack tab
	byte wunblockable;
	move_flags wmoveflags;
	int32_t weapoverrideFLAGS;
	int32_t weap_hxofs, weap_hyofs, weap_hxsz, weap_hysz, weap_hzsz, weap_xofs, weap_yofs, weap_tilew, weap_tileh;
	int32_t wstep;
	byte burnsprs[WPNSPR_MAX];
	byte light_rads[WPNSPR_MAX];
	byte specialsfx;

#define ENEMY_FLAG1   0x01
#define ENEMY_FLAG2   0x02
#define ENEMY_FLAG3     0x04
#define ENEMY_FLAG4     0x08
#define ENEMY_FLAG5     0x10
#define ENEMY_FLAG6     0x20
#define ENEMY_FLAG7     0x40
#define ENEMY_FLAG8     0x80
#define ENEMY_FLAG9     0x100
#define ENEMY_FLAG10     0x200
#define ENEMY_FLAG11     0x400
#define ENEMY_FLAG12     0x800
#define ENEMY_FLAG13     0x1000
#define ENEMY_FLAG14     0x2000
#define ENEMY_FLAG15     0x4000
#define ENEMY_FLAG16     0x8000
    
};

#define LIFTFL_DIS_SHIELD             0x00000001
#define LIFTFL_DIS_ITEMS              0x00000002
#define LIFTFL_DIS_SWIMMING           0x00000004
#define LIFTFL_DROP_ON_HIT            0x00000008
#define NUM_LIFTFL 4

class refInfo
{
public:
	//word script; //script number
	dword pc; //current command offset
	
	int32_t d[8]; //d registers
	uint32_t sp; //stack pointer for current script
	dword wait_index; // nth WaitX instruction (0 being pc 0) last execution stopped at. for jit only
	uint32_t retsp; //stack pointer for the return stack
	
	dword ffcref;
	int32_t idata;
	dword itemref, guyref, lwpn, ewpn;
	dword screenref, npcdataref, bitmapref, spriteref, spritedataref, dmapsref, zmsgref, shopsref, untypedref;
	int32_t mapsref;
	//to implement
	dword dropsetref, pondref, warpringref, doorsref, zcoloursref, rgbref, paletteref, palcycleref, tunesref;
	dword gamedataref, cheatsref; 
	dword fileref, comboidref, directoryref, rngref, stackref, paldataref;
	dword bottletyperef, bottleshopref, genericdataref;
	int32_t combosref, comboposref;
	int32_t portalref, saveportalref;
	int32_t websocketref;
	dword subdataref, subpageref, subwidgref;
	
	int32_t switchkey; //used for switch statements
	dword thiskey, thiskey2; //used for user class 'this' pointers
	dword waitframes; //wait multiple frames in a row
	
	int32_t cmp_op1, cmp_op2; //cached compare operands
	optional<int32_t> cmp_strcache;
	std::set<uint32_t> stack_pos_is_object;
	
	void Clear()
	{
		*this = refInfo();
	}
};


//Build date info
extern const int BUILDTM_YEAR;
extern const int BUILDTM_MONTH;
extern const int BUILDTM_DAY;
extern const int BUILDTM_HOUR;
extern const int BUILDTM_MINUTE;
extern const int BUILDTM_SECOND;

// The version of the ZASM engine a script was compiled for
// NOT the same as V_FFSCRIPT, which is the version of the packfile format
// where the scripts are serialized
#define ZASM_VERSION        3

const char* ScriptTypeToString(ScriptType type);

#define ZMETA_AUTOGEN		0x01
#define ZMETA_DISASSEMBLED	0x02
#define ZMETA_IMPORTED		0x04

#define SCRIPT_FORMAT_DEFAULT		0
#define SCRIPT_FORMAT_INVALID		1
#define SCRIPT_FORMAT_DISASSEMBLED	2
#define SCRIPT_FORMAT_ZASM			3

#define METADATA_V			5
#define V_COMPILER_FIRST	BUILDTM_YEAR
#define V_COMPILER_SECOND	BUILDTM_MONTH
#define V_COMPILER_THIRD	BUILDTM_DAY
#define V_COMPILER_FOURTH	BUILDTM_HOUR
#define ZMETA_NULL_TYPE		1
struct zasm_meta
{
	word zasm_v;
	word meta_v;
	word ffscript_v;
	ScriptType script_type;
	std::string run_idens[8];
	byte run_types[8];
	byte flags;
	word compiler_v1, compiler_v2, compiler_v3, compiler_v4;
	std::string script_name;
	std::string author;
	std::string attributes[10];
	std::string attribytes[8];
	std::string attrishorts[8];
	std::string usrflags[16];
	std::string attributes_help[10];
	std::string attribytes_help[8];
	std::string attrishorts_help[8];
	std::string usrflags_help[16];
	std::string initd[8];
	std::string initd_help[8];
	int8_t initd_type[8];
	
	void setFlag(byte flag)
	{
		switch(flag)
		{
			case ZMETA_DISASSEMBLED:
				flags &= ~ZMETA_IMPORTED;
				flags |= ZMETA_DISASSEMBLED;
				break;
			case ZMETA_IMPORTED:
				flags &= ~ZMETA_DISASSEMBLED;
				flags |= ZMETA_IMPORTED;
				break;
			default:
				flags |= flag;
		}
	}
	bool valid() const
	{
		return zasm_v >= 2 && meta_v >= 1 && ffscript_v >= 16;
	}
	void zero()
	{
		zasm_v = 0;
		meta_v = 0;
		ffscript_v = 0;
		script_type = ScriptType::None;
		flags = 0;
		compiler_v1 = 0;
		compiler_v2 = 0;
		compiler_v3 = 0;
		compiler_v4 = 0;
		for(int32_t q = 0; q < 16; ++q)
		{
			usrflags[q].clear();
			usrflags_help[q].clear();
			if(q > 9) continue;
			attributes[q].clear();
			attributes_help[q].clear();
			if(q > 7) continue;
			initd[q].clear();
			initd_help[q].clear();
			initd_type[q] = -1;
			run_idens[q].clear();
			run_types[q] = ZMETA_NULL_TYPE;
			attribytes[q].clear();
			attribytes_help[q].clear();
			attrishorts[q].clear();
			attrishorts_help[q].clear();
		}
		script_name.clear();
		author.clear();
	}
	void autogen(bool clears = true)
	{
		if(clears) zero();
		zasm_v = ZASM_VERSION;
		meta_v = METADATA_V;
		ffscript_v = V_FFSCRIPT;
		flags = ZMETA_AUTOGEN;
		compiler_v1 = V_COMPILER_FIRST;
		compiler_v2 = V_COMPILER_SECOND;
		compiler_v3 = V_COMPILER_THIRD;
		compiler_v4 = V_COMPILER_FOURTH;
	}
	zasm_meta()
	{
		zero();
	}
	~zasm_meta()
	{
		
	}
	zasm_meta& operator=(zasm_meta const& other)
	{
		zasm_v = other.zasm_v;
		meta_v = other.meta_v;
		ffscript_v = other.ffscript_v;
		script_type = other.script_type;
		for(auto q = 0; q < 16; ++q)
		{
			usrflags[q] = other.usrflags[q];
			usrflags_help[q] = other.usrflags_help[q];
			if(q > 9) continue;
			attributes[q] = other.attributes[q];
			attributes_help[q] = other.attributes_help[q];
			if(q > 7) continue;
			initd[q] = other.initd[q];
			initd_help[q] = other.initd_help[q];
			initd_type[q] = other.initd_type[q];
			run_idens[q] = other.run_idens[q];
			run_types[q] = other.run_types[q];
			attribytes[q] = other.attribytes[q];
			attribytes_help[q] = other.attribytes_help[q];
			attrishorts[q] = other.attrishorts[q];
			attrishorts_help[q] = other.attrishorts_help[q];
			if(q > 3) continue;
		}
		flags = other.flags;
		compiler_v1 = other.compiler_v1;
		compiler_v2 = other.compiler_v2;
		compiler_v3 = other.compiler_v3;
		compiler_v4 = other.compiler_v4;
		script_name = other.script_name;
		author = other.author;
		return *this;
	}
	bool operator==(zasm_meta const& other) const
	{
		if(zasm_v != other.zasm_v) return false;
		if(meta_v != other.meta_v) return false;
		if(ffscript_v != other.ffscript_v) return false;
		if(script_type != other.script_type) return false;
		if(flags != other.flags) return false;
		if(compiler_v1 != other.compiler_v1) return false;
		if(compiler_v2 != other.compiler_v2) return false;
		if(compiler_v3 != other.compiler_v3) return false;
		if(compiler_v4 != other.compiler_v4) return false;
		for(auto q = 0; q < 16; ++q)
		{
			if(usrflags[q].compare(other.usrflags[q]))
				return false;
			if(usrflags_help[q].compare(other.usrflags_help[q]))
				return false;
			if(q > 9) continue;
			if(attributes[q].compare(other.attributes[q]))
				return false;
			if(attributes_help[q].compare(other.attributes_help[q]))
				return false;
			if(q > 7) continue;
			if(initd[q].compare(other.initd[q]))
				return false;
			if(initd_help[q].compare(other.initd_help[q]))
				return false;
			if(initd_type[q] != other.initd_type[q])
				return false;
			if(run_idens[q].compare(other.run_idens[q]))
				return false;
			if(run_types[q] != other.run_types[q])
				return false;
			if(attribytes[q].compare(other.attribytes[q]))
				return false;
			if(attribytes_help[q].compare(other.attribytes_help[q]))
				return false;
			if(attrishorts[q].compare(other.attrishorts[q]))
				return false;
			if(attrishorts_help[q].compare(other.attrishorts_help[q]))
				return false;
		}
		if(script_name.compare(other.script_name))
			return false;
		if(author.compare(other.author))
			return false;
		return true;
	}
	bool operator!=(zasm_meta const& other) const
	{
		return !(*this == other);
	}
	
	bool parse_meta(const char *buffer);
	std::string get_meta() const;
};
ScriptType get_script_type(std::string const& name);
std::string get_script_name(ScriptType type);

#define NUM_ZS_ARGS 3
struct ffscript
{
	word command;
	int32_t arg1, arg2, arg3;
	std::vector<int32_t> *vecptr;
	std::string *strptr;
	ffscript()
	{
		command = 0xFFFF;
		arg1 = 0;
		arg2 = 0;
		arg3 = 0;
		vecptr = nullptr;
		strptr = nullptr;
	}
	ffscript(word command, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0): command(command), arg1(arg1), arg2(arg2), arg3(arg3)
	{
		vecptr = nullptr;
		strptr = nullptr;
	}
	~ffscript()
	{
		if(vecptr)
		{
			delete vecptr;
			vecptr = nullptr;
		}
		if(strptr)
		{
			delete strptr;
			strptr = nullptr;
		}
	}
	void give(ffscript& other)
	{
		other.command = command;
		other.arg1 = arg1;
		other.arg2 = arg2;
		other.arg3 = arg3;
		other.vecptr = vecptr;
		other.strptr = strptr;
		vecptr = nullptr;
		strptr = nullptr;
		clear();
	}
	void clear()
	{
		command = 0xFFFF;
		arg1 = 0;
		arg2 = 0;
		arg3 = 0;
		if(vecptr)
		{
			delete vecptr;
			vecptr = nullptr;
		}
		if(strptr)
		{
			delete strptr;
			strptr = nullptr;
		}
	}
	void copy(ffscript& other)
	{
		other.clear();
		other.command = command;
		other.arg1 = arg1;
		other.arg2 = arg2;
		other.arg3 = arg3;
		if(vecptr)
		{
			other.vecptr = new std::vector<int32_t>();
			for(int32_t val : *vecptr)
				other.vecptr->push_back(val);
		}
		if(strptr)
		{
			other.strptr = new std::string();
			for(char c : *strptr)
				other.strptr->push_back(c);
		}
	}
	
	bool operator==(ffscript const& other) const
	{
		//Compare primitive members
		if(command != other.command) return false;
		if(arg1 != other.arg1) return false;
		if(arg2 != other.arg2) return false;
		if(arg3 != other.arg3) return false;
		//Check for pointer existence differences
		if((vecptr==nullptr)!=(other.vecptr==nullptr)) return false;
		if((strptr==nullptr)!=(other.strptr==nullptr)) return false;
		//If both have a pointer, compare pointer size/contents
		if(vecptr)
		{
			if(vecptr->size() != other.vecptr->size())
				return false;
			if((*vecptr) != (*other.vecptr))
				return false;
		}
		if(strptr)
		{
			if(strptr->size() != other.strptr->size())
				return false;
			if(strptr->compare(*other.strptr))
				return false;
		}
		return true;
	}
};

struct script_id {
	auto operator<=>(const script_id&) const = default;

	ScriptType type;
	int index;
};

typedef uint16_t zasm_script_id;

struct zasm_script
{
	zasm_script() = default;
	zasm_script(zasm_script_id id, std::string name, std::vector<ffscript>&& zasm) : id(id), optimized(false), name(name), size(zasm.size()), zasm(std::exchange(zasm, {})) {}
	zasm_script(std::vector<ffscript>&& zasm) : id(0), optimized(false), name(""), size(zasm.size()), zasm(std::exchange(zasm, {})) {}

	zasm_script_id id;
	bool optimized;
	std::string name;
	size_t size;
	std::vector<ffscript> zasm;

	// TODO: remove the necessity of this terminal command being here.
	bool valid() const
	{
		return !zasm.empty() && zasm[0] != 0xFFFF;
	}
};

struct script_data
{
	// The zasm instructions used by this script.
	// In quests before 3.0, each script had its own chunk of zasm.
	// Since 3.0 all scripts share the same chunk.
	// TODO: The previous comment is not true _yet_, but will be when the "mergeslots3" branch is merged.
	std::shared_ptr<::zasm_script> zasm_script = nullptr;
	zasm_meta meta;
	script_id id;

	script_data(ScriptType type, int index) : id({type, index}) {}

	std::string name() const
	{
		if (meta.script_name.empty())
			return fmt::format("{}-{}", ScriptTypeToString(id.type), id.index);
		else
			return fmt::format("{}-{}-{}", ScriptTypeToString(id.type), id.index, meta.script_name);
	}
	
	bool valid() const
	{
		return zasm_script && zasm_script->valid();
	}
	
	void disable()
	{
		zasm_script = nullptr;
	}
};

//Sprite boundary array indices
enum
{
	spriteremovalY1, spriteremovalY2, spriteremovalX1, spriteremovalX2, spriteremovalZ1, spriteremovalZ2
};


enum
{
    sBCANDLE, sARROW, sBOMB, sSTAIRS, sSECRET01, sSECRET02, sSECRET03,
    sSECRET04, sSECRET05, sSECRET06, sSECRET07, sSECRET08, sSECRET09,
    sSECRET10, sSECRET11, sSECRET12, sSECRET13, sSECRET14, sSECRET15,
    sSECRET16, sRCANDLE, sWANDFIRE, sDIVINEFIRE, sSARROW, sGARROW,
    sSBOMB, sBRANG, sMBRANG, sFBRANG, sWANDMAGIC, sREFMAGIC, sREFFIREBALL,
    sSWORD, sWSWORD, sMSWORD, sXSWORD, sSWORDBEAM, sWSWORDBEAM,
    sMSWORDBEAM, sXSWORDBEAM, sHOOKSHOT, sWAND, sHAMMER, sSTRIKE, sSECNEXT
};

struct comboclass
{
    char  name[64];                       //  name
    byte  block_enemies;                  //  a (bit flags, 1=ground, 2=jump, 4=fly)
    byte  block_hole;                     //  b
    byte  block_trigger;                  //  c
    byte  block_weapon[32];               //  d (ID of lweapons/eweapons to block)
    int16_t conveyor_x_speed;               //  e
    int16_t conveyor_y_speed;               //  f
    word  create_enemy;                   //  g
    byte  create_enemy_when;              //  h
    int32_t  create_enemy_change;            //  i
    byte  directional_change_type;        //  j
    int32_t  distance_change_tiles;          //  k
    int16_t dive_item;                      //  l
    byte  dock;                           //  m
    byte  fairy;                          //  n
    byte  ff_combo_attr_change;           //  o
    int32_t  foot_decorations_tile;          //  p
    byte  foot_decorations_type;          //  q
    byte  hookshot_grab_point;            //  r
    byte  ladder_pass;                    //  s
    byte  lock_block_type;                //  t
    int32_t  lock_block_change;              //  u
    byte  magic_mirror_type;              //  v
    int16_t modify_hp_amount;               //  w
    byte  modify_hp_delay;                //  x
    byte  modify_hp_type;                 //  y
    int16_t modify_mp_amount;               //  z
    byte  modify_mp_delay;                // aa
    byte  modify_mp_type;                 // ab
    byte  no_push_blocks;                 // ac
    byte  overhead;                       // ad
    byte  place_enemy;                    // ae
    byte  push_direction;                 // af
    byte  push_weight;                    // ag
    byte  push_wait;                      // ah
    byte  pushed;                         // ai
    byte  raft;                           // aj
    byte  reset_room;                     // ak
    byte  save_point_type;                // al
    byte  screen_freeze_type;             // am
    byte  secret_combo;                   // an
    byte  singular;                       // ao
    byte  slow_movement;                  // ap
    byte  statue_type;                    // aq
    byte  step_type;                      // ar
    int32_t  step_change_to;                 // as
    byte  strike_weapons[32];             // at
    int32_t  strike_remnants;                // au
    byte  strike_remnants_type;           // av
    int32_t  strike_change;                  // aw
    int16_t strike_item;                    // ax
    int16_t touch_item;                     // ay
    byte  touch_stairs;                   // az
    byte  trigger_type;                   // ba
    byte  trigger_sensitive;              // bb
    byte  warp_type;                      // bc
    byte  warp_sensitive;                 // bd
    byte  warp_direct;                    // be
    byte  warp_location;                  // bf
    byte  water;                          // bg
    byte  whistle;                        // bh
    byte  win_game;                       // bi
    byte  block_weapon_lvl;               // bj (maximum item level to block)
	byte pit;                             // bk
};

struct tiletype
{
    byte bitplanes;
    byte *data;
};

struct ZCHEATS
{
    dword flags;
    char  codes[4][41];
    //168
};

#define QH_IDSTR    "AG Zelda Classic Quest File\n "
#define QH_NEWIDSTR "AG ZC Enhanced Quest File\n   "
#define ENC_STR     "Zelda Classic Quest File"

struct zquestheader
{
    char  id_str[31];
    int16_t zelda_version; // Deprecated
    char zelda_version_string[40];
    word  internal;
    byte  quest_number;
    byte  old_rules[2];
    byte  old_map_count;
    char  old_str_count;
    byte  data_flags[ZQ_MAXDATA];
    byte  old_rules2[2];
    char  old_options;
    char  version[17]; // The quest version (not ZC). Could be an empty string.
    char  title[65];
    char  author[65];
    bool  dirty_password;
    char  password[256];
    uint8_t pwd_hash[16];
    char  minver[17];
    byte  build;
    byte  use_keyfile;
    byte  old_foo[9];
    byte  old_rules3[2];
    byte  old_rules4[2];
    byte  old_rules5[2];
    byte  old_rules6[2];
    byte  old_rules7[2];
    byte  old_rules8[2];
    byte  old_rules9[2];
    byte  old_rules10[2];
    byte  old_midi_flags[MIDIFLAGS_SIZE];
    byte  old_foo2[18];
    // No one used custom quest templates, so we stopped supporting it.
    char  templatepath[2048];
    int32_t version_major;
    int32_t version_minor;
    int32_t version_patch;

	// Deprecated.
    int32_t new_version_id_fourth;
    int32_t new_version_id_alpha;
    int32_t new_version_id_beta;
    int32_t new_version_id_gamma;
    int32_t new_version_id_release;

	bool new_version_is_nightly;
    word new_version_id_date_year;
    byte new_version_id_date_month;
    byte new_version_id_date_day;
    byte new_version_id_date_hour;
    byte new_version_id_date_minute;
    char new_version_devsig[256];
    char new_version_compilername[256];
    char new_version_compilerversion[256];
    char product_name[1024];
    byte compilerid;
    int32_t compilerversionnumber_first;
    int32_t compilerversionnumber_second;
    int32_t compilerversionnumber_third;
    int32_t compilerversionnumber_fourth;
    word developerid;
    char made_in_module_name[1024];
    char build_datestamp[256];
    char build_timestamp[256];
    char build_timezone[6];
    //made in module_name
    
	bool external_zinfo;
	
	
	bool is_legacy() const;
	int8_t getAlphaState() const;
	char const* getAlphaStr(bool ignoreNightly = false) const;
	int32_t getAlphaVer() const;
	char const* getAlphaVerStr() const;
	char const* getVerStr() const;
	char const* getVerCmpStr() const;
	int32_t compareDate() const;
	int32_t compareVer() const;
};

#define MFORMAT_MIDI 0

//tune flags
#define tfDISABLESAVE    1

struct zctune
{
    MIDI *data;
    int32_t start;
    int32_t loop_start = -1;
    int32_t loop_end = -1;
    int16_t loop = 1;
    int16_t volume = 144;
    byte flags;
    char title[37];

    zctune()
    {
        data = NULL;
        reset();
    }
    
    zctune(char _title[36], int32_t _start, int32_t _loop_start, int32_t _loop_end, int16_t _loop,int16_t _volume)
        : data(nullptr), start(_start), loop_start(_loop_start), loop_end(_loop_end), loop(_loop), volume(_volume), flags(0)
    {
        strncpy(title, _title, 36);
    }
    
    void clear()
    {
        memset(title,0,36);
        start = loop_start = loop_end = 0;
        loop = volume = flags = 0;
        data = NULL;
    }
    
    void copyfrom(zctune z)
    {
        start = z.start;
        loop_start = z.loop_start;
        loop_end = z.loop_end;
        loop = z.loop;
        flags = z.flags;
        volume = z.volume;
        strncpy(title, z.title, 36);
        data = z.data;
		z.data = nullptr;
    }
    
    void reset()
    {
        title[0]=0;
        loop=1;
        volume=144;
        start=0;
        loop_start=-1;
        loop_end=-1;
        flags=0;
        destroy_midi(data);
        data = NULL;
    }
};

#define itype_max_zc250 255 //Last in the 2.50.x lists. 

enum {i_sword=1, i_wsword, i_msword, i_xsword, imax_sword};
enum {i_wbrang=1, i_mbrang, i_fbrang, imax_brang};
enum {i_warrow=1, i_sarrow, i_garrow, imax_arrow};
enum {i_bcandle=1, i_rcandle, imax_candle};
enum {i_recorder=1, imax_whistle};
enum {i_bait=1, imax_bait};
enum {i_letter=1, i_letter_used, imax_letter};
enum {i_bpotion=1, i_rpotion, imax_potion};
enum {i_wand=1, imax_wand};
enum {i_bring=2, i_rring, i_gring, imax_ring};
enum {i_swallet=1, i_lwallet, i_mwallet, imax_wallet};
enum {i_amulet1=1, i_amulet2, imax_amulet};
enum {i_smallshield=1, i_largeshield, i_mirrorshield, imax_shield};
enum {i_shortbow=1, i_longbow, imax_bow};
enum {i_raft=1, imax_raft};
enum {i_ladder=1, i_ladder2, imax_ladder};
enum {i_book=1, imax_book};
enum {i_magickey=1, imax_magickey};
enum {i_oldglove=1, i_bracelet1, i_bracelet2, imax_bracelet};
enum {i_flippers=1, imax_flippers};
enum {i_boots=1, imax_boots};
enum {i_hookshot=1, i_longshot, imax_hookshot};
enum {i_lens=1, imax_lens};
enum {i_hammer=1, imax_hammer};
enum {i_divinefire=1, imax_divinefire};
enum {i_divineescape=1, imax_divineescape};
enum {i_divineprotection=1, imax_divineprotection};
enum {i_quiver=1, i_quiverl2, i_quiverl3, i_quiverl4, imax_quiver};
enum {i_cbyrna=1, imax_cbyrna};
enum {i_rocs=1, imax_rocs};
enum {i_hoverboots=1, imax_hoverboots};
enum {i_spinscroll1=1, i_spinscroll2, imax_spinscroll};
enum {i_crossscroll=1, imax_crossscroll};
enum {i_quakescroll1=1, i_quakescroll2, imax_quakescroll};
enum {i_whispring1=1, i_whispring2, imax_whispring};
enum {i_chargering1=1, i_chargering2, imax_chargering};
enum {i_perilscroll=1, imax_perilscroll};
enum {i_shieldscroll=1, imax_shieldscroll};
enum {i_wealthmedal=1, i_wealthmedal2, i_wealthmedal3, imax_wealthmedal};
enum {i_heartring=1, i_heartring2, i_heartring3, imax_heartring};
enum {i_magicring=1, i_magicring2, i_magicring3, i_magicring4, imax_magicring};
enum {i_rupee=1, i_5rupee, i_10rupee, i_20rupee, i_50rupee, i_100rupee, i_200rupee, imax_rupee};
enum {i_arrowa=1, i_5arrowa, i_10arrowa, i_30arrowa, imax_arrowa};
enum {i_bomba=1, i_4bomba, i_8bomba, i_30bomba, imax_bomba};
enum {i_bomb = 1, imax_bomb};
enum {i_sbomb = 1, imax_sbomb};
enum {i_bombbag1=1, i_bombbag2, i_bombbag3, i_bombbag4, imax_bombbag};

//enum {i_clock=1, imax_clock};

enum glow_shape
{
	glshapeCIRC, glshapeCONE,
	glshapeMAX
};

// "initialization data" flags (bit numbers in bit string)
// These are DEFUNCT, it seems.
enum
{
    idE_RAFT, idE_LADDER, idE_BOOK, idE_KEY,
    idE_FLIPPERS, idE_BOOTS, idE_HOVERBOOTS, idE_MAX
};
enum
{
    idI_WAND, idI_LETTER, idI_LENS, idI_HOOKSHOT,
    idI_BAIT, idI_HAMMER, idI_CBYRNA, idI_ROCS, idI_MAX
};
enum { idI_DFIRE, idI_FWIND, idI_NLOVE, idI_MAX2 };
enum
{
    idBP_WOODENPERCENT, idBP_WHITEPERCENT,
    idBP_MAGICPERCENT, idBP_MASTERPERCENT, idBP_MAX
};

///////////////
/// MODULES ///
///////////////

// TODO: we would like to remove the module system / loading from datafiles eventually.

enum { zelda_dat, fonts_dat, sfx_dat };

enum {
    sels_tile_frame, sels_tile_questicon_1A, sels_tile_questicon_1B, sels_tile_questicon_2A,
    sels_tile_questicon_2B, sels_tile_questicon_3A, sels_tile_questicon_3B, sels_tile_questicon_4A, 
    sels_tile_questicon_4B, sels_tile_questicon_5A, sels_tile_questicon_5B, sels_tile_questicon_6A, 
    sels_tile_questicon_6B, sels_tile_questicon_7A, sels_tile_questicon_7B, sels_tile_questicon_8A, 
    sels_tile_questicon_8B, sels_tile_questicon_9A, sels_tile_questicon_9B, sels_tile_questicon_10A, 
    sels_tile_questicon_10B, 
    //x positions
    sels_tile_questicon_1A_X, sels_tile_questicon_1B_X, sels_tile_questicon_2A_X, sels_tile_questicon_2B_X,
    sels_tile_questicon_3A_X, sels_tile_questicon_3B_X, sels_tile_questicon_4A_X, sels_tile_questicon_4B_X, 
    sels_tile_questicon_5A_X, sels_tile_questicon_5B_X, sels_tile_questicon_6A_X, sels_tile_questicon_6B_X, 
    sels_tile_questicon_7A_X, sels_tile_questicon_7B_X, sels_tile_questicon_8A_X, sels_tile_questicon_8B_X, 
    sels_tile_questicon_9A_X, sels_tile_questicon_9B_X, sels_tile_questicon_10A_X, sels_tile_questicon_10B_X,
	
	
    sels_cursor_tile, sels_heart_tile, sels_herotile, draw_hero_first,
    sels_tile_LAST
};

enum {
    sels_tile_frame_cset, sels_tile_questicon_1A_cset, sels_tile_questicon_1B_cset, sels_tile_questicon_2A_cset,
    sels_tile_questicon_2B_cset, sels_tile_questicon_3A_cset, sels_tile_questicon_3B_cset, sels_tile_questicon_4A_cset, 
    sels_tile_questicon_4B_cset, sels_tile_questicon_5A_cset, sels_tile_questicon_5B_cset, sels_tile_questicon_6A_cset, 
    sels_tile_questicon_6B_cset, sels_tile_questicon_7A_cset, sels_tile_questicon_7B_cset, sels_tile_questicon_8A_cset, 
    sels_tile_questicon_8B_cset, sels_tile_questicon_9A_cset, sels_tile_questicon_9B_cset, sels_tile_questicon_10A_cset, 
    sels_tile_questicon_10B_cset, change_cset_on_quest_3, 
	sels_cusror_cset, sels_heart_tilettile_cset, sels_hero_cset,
	
	sels_tile_cset_LAST
	
};

#define titleScreen250 0
#define titleScreen210 10
#define titleScreenMAIN 20

INLINE bool isinRect(int32_t x,int32_t y,int32_t rx1,int32_t ry1,int32_t rx2,int32_t ry2)
{
    return x>=rx1 && x<=rx2 && y>=ry1 && y<=ry2;
}

INLINE void SCRFIX()
{
    putpixel(screen,0,0,getpixel(screen,0,0));
}

extern void flushItemCache(bool justcost = false);
extern void removeFromItemCache(int32_t itemclass);

#define GLOBAL_SCRIPT_INIT 			0
#define GLOBAL_SCRIPT_GAME			1
#define GLOBAL_SCRIPT_END			2
#define GLOBAL_SCRIPT_ONSAVELOAD	3
#define GLOBAL_SCRIPT_ONLAUNCH		4
#define GLOBAL_SCRIPT_ONCONTGAME	5
#define GLOBAL_SCRIPT_F6			6
#define GLOBAL_SCRIPT_ONSAVE		7

#define SCRIPT_HERO_INIT 1
#define SCRIPT_HERO_ACTIVE 2
#define SCRIPT_HERO_DEATH 3
#define SCRIPT_HERO_WIN 4

//Hero Internal Flags
#define LF_PAID_SWORD_COST		0x01
#define LF_PAID_WAND_COST		0x02
#define LF_PAID_CBYRNA_COST		0x04

#define RUNSCRIPT_OK			0
#define RUNSCRIPT_ERROR			1
#define RUNSCRIPT_SELFDELETE	2
#define RUNSCRIPT_STOPPED		3
#define RUNSCRIPT_SELFREMOVE	4

bool runscript_do_earlyret(int runscript_val);

enum //Mapscr hardcodes for temp mapscrs
{
	MAPSCR_SCROLL6 = -14,
	MAPSCR_SCROLL5,
	MAPSCR_SCROLL4,
	MAPSCR_SCROLL3,
	MAPSCR_SCROLL2,
	MAPSCR_SCROLL1,
	MAPSCR_SCROLL0,
	MAPSCR_TEMP6,
	MAPSCR_TEMP5,
	MAPSCR_TEMP4,
	MAPSCR_TEMP3,
	MAPSCR_TEMP2,
	MAPSCR_TEMP1,
	MAPSCR_TEMP0
};

//MIDI States
enum { midissuspNONE, midissuspHALTED, midissuspRESUME };

enum swStyle //Switchhook animation styles
{
	swPOOF, swFLICKER, swRISE
};

enum //Special hardcoded draw layers
{
	SPLAYER_PLAYER_DRAW = -200000, //The Hero, when this occurs changes based on various conditions.
	SPLAYER_EWEAP_BEHIND_DRAW, //Eweapons with 'Behind = true'
	SPLAYER_EWEAP_FRONT_DRAW, //Eweapons with 'Behind = false'
	SPLAYER_LWEAP_BEHIND_DRAW, //Lweapons with 'Behind = true'
	SPLAYER_LWEAP_FRONT_DRAW, //Lweapons with 'Behind = false'
	SPLAYER_LWEAP_ABOVE_DRAW, //Lweapons with (Z+FakeZ) > Init Data 'Jump Layer Height', IF the Hero is also above this height.
	SPLAYER_CHAINLINK_DRAW, //Hookshot chain links
	SPLAYER_NPC_DRAW, //Enemies
	SPLAYER_NPC_ABOVEPLAYER_DRAW, //Enemies 'grabbing' the Hero, or with a higher Z than the Hero
	SPLAYER_NPC_AIRBORNE_DRAW, //Flying enemies, or enemies in the Z axis (amount required QR dependant)
	SPLAYER_ITEMSPRITE_DRAW, //Itemsprites
	SPLAYER_FAIRYITEM_DRAW, //Moving fairies
	SPLAYER_PUSHBLOCK, //Pushable Blocks
	SPLAYER_MOVINGBLOCK, //Moving Pushable Blocks
	SPLAYER_OVERHEAD_CMB, //Overhead combos
	SPLAYER_OVERHEAD_FFC, //Overhead ffcs
	SPLAYER_DARKROOM_UNDER, //Under dark room darkness
	SPLAYER_DARKROOM_OVER, //Over dark room darkness
	SPLAYER_FFC_DRAW, //Non-Overhead ffcs
	SPLAYER_LENS_UNDER_1, //Lens drawing secrets/revealing things
	SPLAYER_LENS_UNDER_2, //Lens drawing secrets/revealing things
	SPLAYER_LENS_OVER, //Lens drawing blackness to cover most of the screen
};

//

extern const char months[13][13];
char *VerStrFromHex(int32_t version);

RGB _RGB(byte *si);
RGB _RGB(int32_t r,int32_t g,int32_t b);
// Converts 6-bit RGB to 8-bit.
void convertRGB(RGB& c);
RGB invRGB(RGB s);
RGB mixRGB(int32_t r1,int32_t g1,int32_t b1,int32_t r2,int32_t g2,int32_t b2,int32_t ratio);

extern char tmp_themefile[2048];
char const* get_themefile();
void set_theme(char const* fpath);
void reset_theme();
void load_themefile(char const* fpath, PALETTE pal, ALLEGRO_COLOR* colors);
void load_themefile(char const* fpath);
void save_themefile(char const* fpath, PALETTE pal, ALLEGRO_COLOR* colors);
void save_themefile(char const* fpath);
void load_udef_colorset(App a, PALETTE pal, ALLEGRO_COLOR* colors);
void load_udef_colorset(App a);
void load_colorset(int32_t colorset, PALETTE pal, ALLEGRO_COLOR* colors);
void load_colorset(int32_t colorset);

void update_hw_screen();

bool valid_str(char const* ptr, char cancel = 0);

std::string get_dbreport_string();
std::string get_qr_hexstr(byte* qrs = NULL, bool hash = false, bool disctags = true);
bool clipboard_has_text();
bool get_al_clipboard(std::string& clipboard);
void set_al_clipboard(std::string const& clipboard);
bool load_qr_hexstr(std::string hexstr);
bool load_qr_hexstr_clipboard();
bool load_dev_info(std::string const& devstr);
bool load_dev_info_clipboard();
std::string generate_zq_about();

void enter_sys_pal();
void exit_sys_pal();

enum {nswapDEC, nswapHEX, nswapLDEC, nswapLHEX, nswapBOOL, nswapMAX};


double WrapAngle(double radians);
double WrapDegrees(double degrees);
double DegreesToRadians(double deg);
double RadiansToDegrees(double rad);
double DirToRadians(int dir);
double DirToDegrees(int dir);
int32_t AngleToDir(double radians);
int32_t AngleToDir4(double degrees);
int32_t AngleToDir4Rad(double radians);

bool isNextType(int32_t type);
bool isWarpType(int32_t type);
int32_t getWarpLetter(int32_t type);
int32_t simplifyWarpType(int32_t type);
bool isStepType(int32_t type);
bool isDamageType(int32_t type);
bool isConveyorType(int32_t type);
bool isChestType(int32_t type);

DATAFILE* load_datafile_count(const char* path, size_t& sz);

enum
{
	ZSD_NONE = -1,
	ZSD_NPC, ZSD_LWPN, ZSD_EWPN, ZSD_ITSPR, ZSD_COMBODATA,
	ZSD_COMBOPOS, ZSD_FFC,
	NUM_ZSD
};

#define FONTSDAT_CNT (FONT_ZX+1)

#define SMART_WRAP(x, mod) ((x) < 0 ? (((mod)-(-(x)%(mod)))%(mod)) : ((x)%(mod)))
#define MEMCPY_ARR(dest,src) memcpy(dest,src,sizeof(dest))

#undef cmb1
#undef cmb2
#undef cmb3
#undef cmb4
#undef cmb5
#undef cmb6
#undef cmb7
#undef cmb8
#undef cmb9
#undef cmb10
#undef cmb11
#undef cmb12
#undef cmb13
#undef cmb14
#undef cmb15
#undef cmb16

#undef rad1
#undef rad2
#undef rad3
#undef rad4
#undef rad5
#undef rad6
#undef rad7
#undef rad8
#undef rad9
#undef rad10
#undef rad11
#undef rad12
#undef rad13
#undef rad14
#undef rad15
#undef rad16

#endif                                                      //_ZDEFS_H_
