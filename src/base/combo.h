#ifndef COMBO_H_
#define COMBO_H_

#include "base/ints.h"
#include "base/zfix.h"
#include "base/headers.h"
#include "base/containers.h"
#include "base/flags.h"
#include <functional>

namespace flags {
enum lift_flags : uint8_t
{
	LF_LIFTABLE             = F(0),
	LF_DROPSET              = F(1),
	LF_DROPONLIFT           = F(2),
	LF_SPECIALITEM          = F(3),
	LF_NOUCSET              = F(4),
	LF_NOWPNCMBCSET         = F(5),
	LF_BREAKONSOLID         = F(6),
};
} // ends namespace

#define NUM_COMBO_ATTRIBUTES            4
#define NUM_COMBO_ATTRIBYTES            8
#define NUM_COMBO_ATTRISHORTS           8

///user flags
#define cflag1                          0x00001
#define cflag2                          0x00002
#define cflag3                          0x00004
#define cflag4                          0x00008
#define cflag5                          0x00010
#define cflag6                          0x00020
#define cflag7                          0x00040
#define cflag8                          0x00080
#define cflag9                          0x00100
#define cflag10                         0x00200
#define cflag11                         0x00400
#define cflag12                         0x00800
#define cflag13                         0x01000
#define cflag14                         0x02000
#define cflag15                         0x04000
#define cflag16                         0x08000

enum
{
	CMB_ADVP_TILE,
	CMB_ADVP_CSET2,
	CMB_ADVP_SOLIDITY,
	CMB_ADVP_ANIM,
	CMB_ADVP_TYPE,
	CMB_ADVP_INHFLAG,
	CMB_ADVP_ATTRIBYTE,
	CMB_ADVP_ATTRISHORT,
	CMB_ADVP_ATTRIBUTE,
	CMB_ADVP_FLAGS,
	CMB_ADVP_GENFLAGS,
	CMB_ADVP_LABEL,
	CMB_ADVP_SCRIPT,
	CMB_ADVP_EFFECT,
	CMB_ADVP_TRIGGERS,
	CMB_ADVP_LIFTING,
	CMB_ADVP_GEN_MOVESPEED,
	CMB_ADVP_GEN_SFX,
	CMB_ADVP_GEN_SPRITES,
	CMB_ADVP_SZ
};

struct newcombo
{
	int32_t tile;
	byte flip;
	byte walk = 0xF0;
	byte type;
	byte csets;
	byte frames;
	byte speed;
	word nextcombo;
	byte nextcset;
	byte flag;
	byte skipanim;
	word nexttimer;
	byte skipanimy;
	byte animflags;
	byte expansion[6];
	int32_t attributes[NUM_COMBO_ATTRIBUTES]; // combodata->Attributes[] and Screen->GetComboAttribute(pos, indx) / SetComboAttribute(pos, indx)
	int32_t usrflags; // combodata->Flags
	int16_t genflags; // general flags
	int32_t triggerflags[6];
	int32_t triggerlevel;
	byte triggerbtn;
	byte triggeritem;
	byte trigtimer;
	byte trigsfx;
	int32_t trigchange;
	word trigprox;
	byte trigctr;
	int32_t trigctramnt;
	byte triglbeam;
	int8_t trigcschange;
	int16_t spawnitem;
	int16_t spawnenemy;
	int8_t exstate = -1, exdoor_dir = -1, exdoor_ind;
	int32_t spawnip;
	byte trigcopycat;
	byte trigcooldown;
	byte trig_lstate, trig_gstate;
	int32_t trig_statetime;
	word trig_genscr;
	byte trig_group;
	word trig_group_val;
	byte trig_levelitems;
	int16_t trigdmlevel = -1;
	int16_t trigtint[3]; //r,g,b range [-255,255]
	int16_t triglvlpalette = -1, trigbosspalette = -1;
	int16_t trigquaketime = -1, trigwavytime = -1;
	int16_t trig_swjinxtime = -2, trig_itmjinxtime = -2, trig_shieldjinxtime = -2, trig_stuntime = -2, trig_bunnytime = -2;
	byte trig_pushtime = 8;
	lift_flags liftflags;
	byte liftlvl;
	byte liftsfx;
	byte liftitm;
	byte liftgfx;
	word liftcmb, liftundercmb;
	byte liftcs, liftundercs;
	byte liftsprite;
	byte liftdmg;
	int16_t liftbreaksprite = -1;
	byte liftbreaksfx;
	byte lifthei = 8;
	byte lifttime = 16;
	byte lift_parent_item;
	word prompt_cid;
	byte prompt_cs;
	int16_t prompt_x = 12;
	int16_t prompt_y = -8;
	std::string label;
	byte attribytes[NUM_COMBO_ATTRIBYTES];
	int16_t attrishorts[NUM_COMBO_ATTRISHORTS];
	word script;
	int32_t initd[8];
	int32_t o_tile;
	byte cur_frame;
	byte aclk;
	byte speed_mult = 1;
	byte speed_div = 1;
	zfix speed_add;
	byte sfx_appear, sfx_disappear, sfx_loop, sfx_walking, sfx_standing, sfx_tap, sfx_landing, sfx_falling, sfx_drowning, sfx_lava_drowning;
	byte spr_appear, spr_disappear, spr_walking, spr_standing, spr_falling, spr_drowning, spr_lava_drowning;
	
	void set_tile(int32_t newtile);
	void clear();
	bool is_blank(bool ignoreEff = false) const;
	bool can_cycle() const;
	
	int each_tile(std::function<bool(int32_t)> proc) const;
	
	void advpaste(newcombo const& other, bitstring const& pasteflags);
};

#define AF_FRESH                        0x01
#define AF_CYCLE                        0x02
#define AF_CYCLENOCSET                  0x04
#define AF_TRANSPARENT                  0x08
#define AF_CYCLEUNDERCOMBO              0x10

//triggerflags[0]
#define combotriggerSWORD               0x00000001
#define combotriggerSWORDBEAM           0x00000002
#define combotriggerBRANG               0x00000004
#define combotriggerBOMB                0x00000008
#define combotriggerSBOMB               0x00000010
#define combotriggerLITBOMB             0x00000020
#define combotriggerLITSBOMB            0x00000040
#define combotriggerARROW               0x00000080
#define combotriggerFIRE                0x00000100
#define combotriggerWHISTLE             0x00000200
#define combotriggerBAIT                0x00000400
#define combotriggerWAND                0x00000800
#define combotriggerMAGIC               0x00001000
#define combotriggerWIND                0x00002000
#define combotriggerREFMAGIC            0x00004000
#define combotriggerREFFIREBALL         0x00008000
#define combotriggerREFROCK             0x00010000
#define combotriggerHAMMER              0x00020000
#define combotriggerRESETANIM           0x00040000
#define combotriggerINVERTPROX          0x00080000
#define combotriggerBTN_TOP             0x00100000
#define combotriggerBTN_BOTTOM          0x00200000
#define combotriggerBTN_LEFT            0x00400000
#define combotriggerBTN_RIGHT           0x00800000
#define combotriggerINVERTMINMAX        0x01000000
#define combotriggerSTEP                0x02000000
#define combotriggerSTEPSENS            0x04000000
#define combotriggerSHUTTER             0x08000000
#define combotriggerCMBTYPEFX           0x10000000
#define combotriggerONLYGENTRIG         0x20000000
#define combotriggerKILLWPN             0x40000000
#define combotriggerEWFIREBALL          0x80000000

//triggerflags[1]
#define combotriggerHOOKSHOT            0x00000001
#define combotriggerSPARKLE             0x00000002
#define combotriggerBYRNA               0x00000004
#define combotriggerREFBEAM             0x00000008
#define combotriggerSTOMP               0x00000010
#define combotriggerSCRIPT01            0x00000020
#define combotriggerSCRIPT02            0x00000040
#define combotriggerSCRIPT03            0x00000080
#define combotriggerSCRIPT04            0x00000100
#define combotriggerSCRIPT05            0x00000200
#define combotriggerSCRIPT06            0x00000400
#define combotriggerSCRIPT07            0x00000800
#define combotriggerSCRIPT08            0x00001000
#define combotriggerSCRIPT09            0x00002000
#define combotriggerSCRIPT10            0x00004000
#define combotriggerAUTOMATIC           0x00008000
#define combotriggerSECRETS             0x00010000
#define combotriggerINVERTITEM          0x00020000
#define combotriggerCONSUMEITEM         0x00040000
#define combotriggerCOUNTERGE           0x00080000
#define combotriggerCOUNTERLT           0x00100000
#define combotriggerCOUNTEREAT          0x00200000
#define combotriggerCTRNONLYTRIG        0x00400000
#define combotriggerLIGHTON             0x00800000
#define combotriggerLIGHTOFF            0x01000000
#define combotriggerPUSH                0x02000000
#define combotriggerLENSON              0x04000000
#define combotriggerLENSOFF             0x08000000
#define combotriggerEWARROW             0x10000000
#define combotriggerEWBRANG             0x20000000
#define combotriggerEWSWORD             0x40000000
#define combotriggerEWROCK              0x80000000

//triggerflags[2]
#define combotriggerEWSCRIPT01          0x00000001
#define combotriggerEWSCRIPT02          0x00000002
#define combotriggerEWSCRIPT03          0x00000004
#define combotriggerEWSCRIPT04          0x00000008
#define combotriggerEWSCRIPT05          0x00000010
#define combotriggerEWSCRIPT06          0x00000020
#define combotriggerEWSCRIPT07          0x00000040
#define combotriggerEWSCRIPT08          0x00000080
#define combotriggerEWSCRIPT09          0x00000100
#define combotriggerEWSCRIPT10          0x00000200
#define combotriggerEWMAGIC             0x00000400
#define combotriggerEWBBLAST            0x00000800
#define combotriggerEWSBBLAST           0x00001000
#define combotriggerEWLITBOMB           0x00002000
#define combotriggerEWLITSBOMB          0x00004000
#define combotriggerEWFIRETRAIL         0x00008000
#define combotriggerEWFLAME             0x00010000
#define combotriggerEWWIND              0x00020000
#define combotriggerEWFLAME2            0x00040000
#define combotriggerSPCITEM             0x00080000
#define combotriggerEXSTITEM            0x00100000
#define combotriggerEXSTENEMY           0x00200000
#define combotriggerAUTOGRABITEM        0x00400000
#define combotriggerENEMIESKILLED       0x00800000
#define combotriggerSECRETSTR           0x01000000
#define combotriggerTHROWN              0x02000000
#define combotriggerQUAKESTUN           0x04000000
#define combotriggerSQUAKESTUN          0x08000000
#define combotriggerANYFIRE             0x10000000
#define combotriggerSTRONGFIRE          0x20000000
#define combotriggerMAGICFIRE           0x40000000
#define combotriggerDIVINEFIRE          0x80000000

//triggerflags[3]
#define combotriggerTRIGLEVELSTATE      0x00000001
#define combotriggerLEVELSTATE          0x00000002
#define combotriggerTRIGGLOBALSTATE     0x00000004
#define combotriggerGLOBALSTATE         0x00000008
#define combotriggerKILLENEMIES         0x00000010
#define combotriggerCLEARENEMIES        0x00000020
#define combotriggerCLEARLWEAPONS       0x00000040
#define combotriggerCLEAREWEAPONS       0x00000080
#define combotriggerIGNITE_ANYFIRE      0x00000100
#define combotriggerIGNITE_STRONGFIRE   0x00000200
#define combotriggerIGNITE_MAGICFIRE    0x00000400
#define combotriggerIGNITE_DIVINEFIRE   0x00000800
#define combotriggerSEPARATEWEAPON      0x00001000
#define combotriggerTGROUP_CONTRIB      0x00002000
#define combotriggerTGROUP_LESS         0x00004000
#define combotriggerTGROUP_GREATER      0x00008000
#define combotriggerPUSHEDTRIG          0x00010000
#define combotriggerDIVETRIG            0x00020000
#define combotriggerDIVESENSTRIG        0x00040000
#define combotriggerLWREFARROW          0x00080000
#define combotriggerLWREFFIRE           0x00100000
#define combotriggerLWREFFIRE2          0x00200000
#define combotriggerTOGGLEDARK          0x00400000
#define combotriggerCOND_DARK           0x00800000
#define combotriggerCOND_NODARK         0x01000000
#define combotriggerLITEM_COND          0x02000000
#define combotriggerLITEM_REVCOND       0x04000000
#define combotriggerLITEM_SET           0x08000000
#define combotriggerLITEM_UNSET         0x10000000
#define combotriggerTINT_CLEAR          0x20000000
#define combotriggerONLY_GROUND_WPN     0x40000000
//#define combotrigger                    0x80000000

#define ctrigNONE                       0x00
#define ctrigIGNORE_SIGN                0x01
#define ctrigSECRETS                    0x02
#define ctrigSWITCHSTATE                0x04

enum //cCRUMBLE types
{
	CMBTY_CRUMBLE_RESET,
	CMBTY_CRUMBLE_CUMULATIVE,
	CMBTY_CRUMBLE_INEVITABLE,
};

extern std::vector<newcombo> combobuf;

bool is_push_flag(int flag, optional<int> dir = nullopt);

#endif

