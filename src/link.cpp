//--------------------------------------------------------
//--------------------------------------------------------
//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  link.cc
//
//  Link's class: LinkClass
//  Handles a lot of game play stuff as well as Link's
//  movement, attacking, etc.
//
//--------------------------------------------------------

#ifndef __GTHREAD_HIDE_WIN32API
#define __GTHREAD_HIDE_WIN32API 1
#endif                            //prevent indirectly including windows.h

#include "precompiled.h" //always first

#include <string.h>
#include <set>
#include <stdio.h>

#include "link.h"
#include "guys.h"
#include "subscr.h"
#include "zc_subscr.h"
#include "decorations.h"
#include "gamedata.h"
#include "zc_custom.h"
#include "title.h"
#include "ffscript.h"
#include "drawing.h"
extern FFScript FFCore;
extern word combo_doscript[176];
extern byte itemscriptInitialised[256];
extern LinkClass Link;
extern ZModule zcm;
extern zcmodule moduledata;
extern refInfo linkScriptData;
#include "mem_debug.h"
#include "zscriptversion.h"
#include "particles.h"

extern refInfo itemScriptData[256];
extern refInfo itemCollectScriptData[256];
extern int32_t item_stack[256][MAX_SCRIPT_REGISTERS];
extern int32_t item_collect_stack[256][MAX_SCRIPT_REGISTERS];
extern refInfo *ri; //= NULL;
extern int32_t(*stack)[MAX_SCRIPT_REGISTERS];
extern byte dmapscriptInitialised;
extern word item_doscript[256];
extern word item_collect_doscript[256];
extern portal* mirror_portal;
using std::set;

extern int32_t skipcont;

extern int32_t draw_screen_clip_rect_x1;
extern int32_t draw_screen_clip_rect_x2;
extern int32_t draw_screen_clip_rect_y1;
extern int32_t draw_screen_clip_rect_y2;
//extern bool draw_screen_clip_rect_show_link;
extern word global_wait;
extern bool link_waitdraw;
extern bool dmap_waitdraw;
extern bool passive_subscreen_waitdraw;
extern bool active_subscreen_waitdraw;

int32_t link_count = -1;
int32_t link_animation_speed = 1; //lower is faster animation
int32_t z3step = 2;
static zfix link_newstep(1.5);
static zfix link_newstep_diag(1.5);
bool did_scripta=false;
bool did_scriptb=false;
bool did_scriptl=false;
byte lshift = 0;
int32_t dowpn = -1;
int32_t directItem = -1; //Is set if Link is currently using an item directly
int32_t directItemA = -1;
int32_t directItemB = -1;
int32_t directItemX = -1;
int32_t directItemY = -1;
int32_t directWpn = -1;
int32_t whistleitem=-1;
extern word g_doscript;
extern word link_doscript;
extern word dmap_doscript;
extern word passive_subscreen_doscript;
extern byte epilepsyFlashReduction;
extern int32_t script_link_cset;

void playLevelMusic();

extern particle_list particles;

byte lsteps[8] = { 1, 1, 2, 1, 1, 2, 1, 1 };

#define CANFORCEFACEUP	(get_bit(quest_rules,qr_SIDEVIEWLADDER_FACEUP)!=0 && dir!=up && (action==walking || action==none))
#define NO_GRIDLOCK		(get_bit(quest_rules, qr_DISABLE_4WAY_GRIDLOCK))
#define SWITCHBLOCK_STATE (switchblock_z<0?switchblock_z:(switchblock_z+z < 0 ? zslongToFix(2147483647) : switchblock_z+z))

static inline bool platform_fallthrough()
{
	return (getInput(btnDown, false, get_bit(quest_rules,qr_SIDEVIEW_FALLTHROUGH_USES_DRUNK)!=0) && get_bit(quest_rules,qr_DOWN_FALL_THROUGH_SIDEVIEW_PLATFORMS))
		|| (Link.jumping < 0 && getInput(btnDown, false, get_bit(quest_rules,qr_SIDEVIEW_FALLTHROUGH_USES_DRUNK)!=0) && get_bit(quest_rules,qr_DOWNJUMP_FALL_THROUGH_SIDEVIEW_PLATFORMS));
}

static inline bool on_sideview_solid(int32_t x, int32_t y, bool ignoreFallthrough = false)
{
	return (_walkflag(x+4,y+16,0) || (y>=160 && currscr>=0x70 && !(tmpscr->flags2&wfDOWN)) ||
		(((y%16)==0) && (!platform_fallthrough() || ignoreFallthrough) &&
		(checkSVLadderPlatform(x+4,y+16) || checkSVLadderPlatform(x+12,y+16))));
}

bool LinkClass::isStanding(bool forJump)
{
	bool st = (z==0 && !(isSideViewLink() && !on_sideview_solid(x,y) && !ladderx && !laddery && !getOnSideviewLadder()) && hoverclk==0);
	if(!st) return false;
	int32_t val = check_pitslide();
	if(val == -2) return false;
	if(val == -1) return true;
	return forJump;
}

static int32_t isNextType(int32_t type)
{
	switch(type)
	{
		case cLIFTSLASHNEXT:
		case cLIFTSLASHNEXTSPECITEM:
		case cLIFTSLASHNEXTITEM:
		case cDIGNEXT:
		case cLIFTNEXT:
		case cLIFTNEXTITEM:
		case cLIFTNEXTSPECITEM:
		case cSLASHNEXT:
		case cBUSHNEXT:
		case cTALLGRASSNEXT:
		case cSLASHNEXTITEM:
		case cSLASHNEXTTOUCHY:
		case cSLASHNEXTITEMTOUCHY:
		case cBUSHNEXTTOUCHY:
		{
			return true;
		}
		default: return false;
	}
}

static int32_t MatchComboTrigger(weapon *w, newcombo *c, int32_t comboid)
{
	int32_t wid = (w->useweapon > 0) ? w->useweapon : w->id;
	
		if ( ( wid == wSword && wid == wSword && c[comboid].triggerflags[0]&combotriggerSWORD ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wBeam && c[comboid].triggerflags[0]&combotriggerSWORDBEAM ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wBrang && c[comboid].triggerflags[0]&combotriggerBRANG ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wBomb && c[comboid].triggerflags[0]&combotriggerBOMB ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wSBomb && c[comboid].triggerflags[0]&combotriggerSBOMB ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wLitBomb && c[comboid].triggerflags[0]&combotriggerLITBOMB ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wLitSBomb && c[comboid].triggerflags[0]&combotriggerLITSBOMB ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wArrow && c[comboid].triggerflags[0]&combotriggerARROW ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wFire && c[comboid].triggerflags[0]&combotriggerFIRE ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wWhistle && c[comboid].triggerflags[0]&combotriggerWHISTLE ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wBait && c[comboid].triggerflags[0]&combotriggerBAIT ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wWand && c[comboid].triggerflags[0]&combotriggerWAND ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wMagic && c[comboid].triggerflags[0]&combotriggerMAGIC ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wWind && c[comboid].triggerflags[0]&combotriggerWIND ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wRefMagic && c[comboid].triggerflags[0]&combotriggerREFMAGIC ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wRefFireball && c[comboid].triggerflags[0]&combotriggerREFFIREBALL ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wRefRock && c[comboid].triggerflags[0]&combotriggerREFROCK ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wHammer && c[comboid].triggerflags[0]&combotriggerHAMMER ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		    //ZScript liter support ends here. 
		
		else if ( ( wid == wHookshot && c[comboid].triggerflags[1]&combotriggerHOOKSHOT ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		
		else if ( ( wid == wFSparkle && c[comboid].triggerflags[1]&combotriggerSPARKLE ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		else if ( ( wid == wSSparkle && c[comboid].triggerflags[1]&combotriggerSPARKLE ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wCByrna && c[comboid].triggerflags[1]&combotriggerBYRNA ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wRefBeam && c[comboid].triggerflags[1]&combotriggerREFBEAM ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wStomp && c[comboid].triggerflags[1]&combotriggerSTOMP ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		    
		    //item trigger flags page 2
		
		else if ( ( wid == wScript1 && c[comboid].triggerflags[1]&combotriggerSCRIPT01 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript2 && c[comboid].triggerflags[1]&combotriggerSCRIPT02 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript3 && c[comboid].triggerflags[1]&combotriggerSCRIPT03 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript4 && c[comboid].triggerflags[1]&combotriggerSCRIPT04 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript5 && c[comboid].triggerflags[1]&combotriggerSCRIPT05 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript6 && c[comboid].triggerflags[1]&combotriggerSCRIPT06 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript7 && c[comboid].triggerflags[1]&combotriggerSCRIPT07 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript8 && c[comboid].triggerflags[1]&combotriggerSCRIPT08 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript9 && c[comboid].triggerflags[1]&combotriggerSCRIPT09 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else if ( ( wid == wScript10 && c[comboid].triggerflags[1]&combotriggerSCRIPT10 ) && ( w->type >= c[comboid].triggerlevel ) )  return 1;
		
		else return 0;
}

bool LinkClass::can_pitfall(bool ignore_hover)
{
	return (!(isSideViewGravity()||action==rafting||z>0||fall<0||(hoverclk && !ignore_hover)||inlikelike||inwallm||pull_link||toogam||(ladderx||laddery)||getOnSideviewLadder()||drownclk||!(moveflags & FLAG_CAN_PITFALL)));
}

int32_t LinkClass::DrunkClock()
{
    return drunkclk;
}
void LinkClass::setDrunkClock(int32_t newdrunkclk)
{
    drunkclk=newdrunkclk;
}

int32_t LinkClass::StunClock()
{
    return lstunclock;
}
void LinkClass::setStunClock(int32_t v)
{
    lstunclock=v;
}

int32_t LinkClass::BunnyClock()
{
    return lbunnyclock;
}
void LinkClass::setBunnyClock(int32_t v)
{
    lbunnyclock=v;
}

LinkClass::LinkClass() : sprite()
{
    init();
}

//2.6

//Stop the subscreen from falling. -Z

bool LinkClass::stopSubscreenFalling(){
	return preventsubscreenfalling;
}

void LinkClass::stopSubscreenFalling(bool v){
	preventsubscreenfalling = v;
}


//Set the button items by brute force

void LinkClass::setAButtonItem(int32_t itmslot){
	game->awpn = itmslot;
}

void LinkClass::setBButtonItem(int32_t itmslot){
	game->bwpn = itmslot;
}

bool LinkClass::getCanLinkFlicker(){
	return flickerorflash; //enable or disable flicker or flash
}
void LinkClass::setCanLinkFlicker(bool v){
	flickerorflash = v;
}

void LinkClass::ClearhitLinkUIDs()
{ 		//Why the flidd doesn't this work?! Clearing this to 0 in a way that doesn't demolish script access is impossible. -Z
		//All I want, is to clear it at the end of a frame, or at the start of a frame, so that if it changes to non-0
		//that a script can read it before Waitdraw(). --I want it to go stale at the end of a frame.
		//I suppose I will need to do this inside the script engine, and not the game_loop() ? -Z
		//THis started out as a simple clear to 0 of lastHitBy[n], but that did not work:
		//I added the second element to this, so that I could store the frame on which the hit is recorded, and 
		//clear it on the next frame, but that had the SAME outcome.
		//Where and how can I clear a value at the end of every frame, so that:
		// 1. If set by internal mecanics, it has its value that you can read by script, before waitdraw--this part works at present.
		// 2. FFCs can read it before Waitframe. --same.
		// 3. After Waitframe(), it is wiped by the ZC Engine to 0. --I cannot get this to happen without breaking 1 and 2. 
	for ( int32_t q = 0; q < NUM_HIT_TYPES_USED_LINK; q++ ) 	
	{
		/*
		if ( lastHitBy[q][1] == (frame-1) ) //Verify if this is needed at all now. 
		{
			//Z_scripterrlog("frame is: %d\n", frame);
			//Z_scripterrlog("Link->HitBy frame is: %d\n", lastHitBy[q][1]);
			lastHitBy[q][0] = 0;
		}
		*/
		lastHitBy[q][0] = 0;
	}
}

void LinkClass::sethitLinkUID(int32_t type, int32_t screen_index)
{
	lastHitBy[type][0] = screen_index;
	//lastHitBy[type][1] = frame;
	/* Let's figure out how to clear this...
	if ( global_wait ) lastHitBy[type] = screen_index;
	else lastHitBy[type] = 0;
	//No, we clear it in Zelda.cpp, with this:
	    if(global_wait)
	    {
		ZScriptVersion::RunScript(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
		global_wait=false;
	    }
	    
	    
	    draw_screen(tmpscr);
	    
	    //clear Link's last hits 
	    //for ( int32_t q = 0; q < 4; q++ ) Link.sethitLinkUID(q, 0);
	    
	*/
	
}

int32_t LinkClass::gethitLinkUID(int32_t type)
{
	return lastHitBy[type][0];
}

void LinkClass::set_defence(int32_t type, int32_t v)
{
	defence[type] = v;
}

int32_t LinkClass::get_defence(int32_t type)
{
	return defence[type];
}


//Set Link;s hurt sfx
void LinkClass::setHurtSFX(int32_t sfx){
	hurtsfx = sfx;
}	
int32_t LinkClass::getHurtSFX() {
	return hurtsfx;
}

bool  LinkClass::getDiagMove()
{
    return diagonalMovement;
}
void LinkClass::setDiagMove(bool newdiag)
{
    diagonalMovement=newdiag;
}
bool  LinkClass::getBigHitbox()
{
    return bigHitbox;
}
void LinkClass::setBigHitbox(bool newbigHitbox)
{
    bigHitbox=newbigHitbox;
}
int32_t LinkClass::getStepRate()
{
    return steprate;
}
void LinkClass::setStepRate(int32_t newrate)
{
    steprate = newrate;
}
int32_t LinkClass::getSwimUpRate()
{
    return game->get_sideswim_up();
}
void LinkClass::setSwimUpRate(int32_t newrate)
{
    game->set_sideswim_up(newrate);
}
int32_t LinkClass::getSwimSideRate()
{
    return game->get_sideswim_side();
}
void LinkClass::setSwimSideRate(int32_t newrate)
{
    game->set_sideswim_side(newrate);
}
int32_t LinkClass::getSwimDownRate()
{
    return game->get_sideswim_down();
}
void LinkClass::setSwimDownRate(int32_t newrate)
{
    game->set_sideswim_down(newrate);
}


//void LinkClass::linkstep() { lstep = lstep<(BSZ?27:11) ? lstep+1 : 0; }
void LinkClass::linkstep()
{
    lstep = lstep<((zinit.linkanimationstyle==las_bszelda)?27:11) ? lstep+1 : 0;
	//need to run all global/hero/dmap scripts here?
}

bool is_moving()
{
    return DrunkUp()||DrunkDown()||DrunkLeft()||DrunkRight();
}

// called by ALLOFF()
void LinkClass::resetflags(bool all)
{
    refilling=REFILL_NONE;
    inwallm=false;
    inlikelike=blowcnt=whirlwind=specialcave=hclk=fairyclk=refill_why=didstuff=0;
    
    if(swordclk>0 || all)
        swordclk=0;
        
    if(itemclk>0 || all)
        itemclk=0;
        
    if(all)
    {
        NayrusLoveShieldClk=0;
        
        if(nayruitem != -1)
        {
            stop_sfx(itemsbuf[nayruitem].usesound);
            stop_sfx(itemsbuf[nayruitem].usesound+1);
        }
        
        nayruitem = -1;
        hoverclk=jumping=0;
		hoverflags = 0;
    }
    damageovertimeclk = -1;
    newconveyorclk = 0;
    hopclk=0;
    hopdir=-1;
    attackclk=0;
    stomping=false;
    reset_swordcharge();
    diveclk=drownclk=0;
    action=none; FFCore.setLinkAction(none);
    conveyor_flags=0;
    magiccastclk=0;
    magicitem=-1;
}

//Can use this for Link->Stun. -Z
void LinkClass::Freeze()
{
    if (action != inwind)
    {
        if (IsSideSwim()) {action=sideswimfreeze; FFCore.setLinkAction(sideswimfreeze);} 
	else {action=freeze; FFCore.setLinkAction(freeze);} 
        // also cancel Link's attack
        attackclk = 0;
    }
}
void LinkClass::unfreeze()
{
    if(action==freeze && fairyclk<1) { action=none; FFCore.setLinkAction(none); }
    if(action==sideswimfreeze && fairyclk<1) { action=sideswimming; FFCore.setLinkAction(sideswimming); }
}

void LinkClass::Drown(int32_t state)
{
	// Link should never drown if the ladder is out
	if(ladderx+laddery)
		return;
	
	switch(state)
	{
		case 1:
			action=lavadrowning; FFCore.setLinkAction(lavadrowning);
			attackclk=0;
			attack=wNone;
			attackid=-1;
			reset_swordcharge();
			drownclk=64;
			z=fall=0;
			break;

		
		default:
		{
			if (isSideViewLink() && get_bit(quest_rules,qr_SIDESWIM)){action=sidedrowning; FFCore.setLinkAction(sidedrowning);}
			else {action=drowning; FFCore.setLinkAction(drowning);}
			attackclk=0;
			attack=wNone;
			attackid=-1;
			reset_swordcharge();
			drownclk=64;
			z=fall=0;
			break;
		}
	}
	
}

void LinkClass::finishedmsg()
{
    //these are to cancel out any keys that Link may
    //be pressing so he doesn't attack at the end of
    //a message if he was scrolling through it quickly.
    rAbtn();
    rBbtn();
    unfreeze();
    
    if(action == landhold1 ||
            action == landhold2 ||
            action == waterhold1 ||
            action == waterhold2 ||
            action == sidewaterhold1 ||
            action == sidewaterhold2)
    {
        holdclk = 1;
    }
}
void LinkClass::setEaten(int32_t i)
{
    inlikelike=i;
}
int32_t LinkClass::getEaten()
{
    return inlikelike;
}
zfix  LinkClass::getX()
{
    return x;
}
zfix  LinkClass::getY()
{
    return y;
}
zfix  LinkClass::getZ()
{
    return z;
}
zfix  LinkClass::getFall()
{
    return fall;
}
zfix  LinkClass::getXOfs()
{
    return xofs;
}
zfix  LinkClass::getYOfs()
{
    return yofs;
}
void LinkClass::setXOfs(int32_t newxofs)
{
    xofs=newxofs;
}
void LinkClass::setYOfs(int32_t newyofs)
{
    yofs=newyofs;
}
int32_t  LinkClass::getHXOfs()
{
    return hxofs;
}
int32_t  LinkClass::getHYOfs()
{
    return hyofs;
}
int32_t  LinkClass::getHXSz()
{
    return hxsz;
}
int32_t  LinkClass::getHYSz()
{
    return hysz;
}
zfix  LinkClass::getClimbCoverX()
{
    return climb_cover_x;
}
zfix  LinkClass::getClimbCoverY()
{
    return climb_cover_y;
}
int32_t  LinkClass::getLadderX()
{
    return ladderx;
}
int32_t  LinkClass::getLadderY()
{
    return laddery;
}

void LinkClass::setX(int32_t new_x)
{
    zfix dx=new_x-x;
    if(Lwpns.idFirst(wHookshot)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHookshot))->x+=dx;
	hs_startx+=(int32_t)dx;
    }
    
    if(Lwpns.idFirst(wHSHandle)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHSHandle))->x+=dx;
    }
	
	if(chainlinks.Count()>0)
	{
		for(int32_t j=0; j<chainlinks.Count(); j++)
        {
            chainlinks.spr(j)->x+=dx;
        }
	}
    
    x=new_x;
    
    // A kludge
    if(!diagonalMovement && dir<=down)
        is_on_conveyor=true;
}

void LinkClass::setY(int32_t new_y)
{
    zfix dy=new_y-y;
    if(Lwpns.idFirst(wHookshot)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHookshot))->y+=dy;
	hs_starty+=(int32_t)dy;
    }
    
    if(Lwpns.idFirst(wHSHandle)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHSHandle))->y+=dy;
    }
	
	if(chainlinks.Count()>0)
	{
		for(int32_t j=0; j<chainlinks.Count(); j++)
        {
            chainlinks.spr(j)->y+=dy;
        }
	}
    
    y=new_y;
    
    // A kludge
    if(!diagonalMovement && dir>=left)
        is_on_conveyor=true;
}

void LinkClass::setZ(int32_t new_z)
{
    if(isSideViewLink())
        return;
        
    if(z==0 && new_z > 0)
    {
        switch(action)
        {
        case swimming:
	{
            diveclk=0;
            action=walking; FFCore.setLinkAction(walking);
            break;
	}
            
        case waterhold1:
	{
            action=landhold1; FFCore.setLinkAction(landhold1);
            break;
	}
            
        case waterhold2:
	{
            action=landhold2; FFCore.setLinkAction(landhold2);
            break;
	}
            
        default:
            if(charging) //!DIMITODO: Let Link jump while charging sword
            {
                reset_swordcharge();
                attackclk=0;
            }
            
            break;
        }
    }
    
    z=(new_z>0 ? new_z : 0);
}

void LinkClass::setXfix(zfix new_x)
{
	//Z_scripterrlog("setxdbl: %f\n",new_x);
    zfix dx=new_x-x;
    if(Lwpns.idFirst(wHookshot)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHookshot))->x+=dx;
	hs_startx+=(int32_t)dx;
    }
    
    if(Lwpns.idFirst(wHSHandle)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHSHandle))->x+=dx;
    }
	
	if(chainlinks.Count()>0)
	{
		for(int32_t j=0; j<chainlinks.Count(); j++)
        {
            chainlinks.spr(j)->x+=dx;
        }
	}
    
    x=new_x;
    
    // A kludge
    if(!diagonalMovement && dir<=down)
        is_on_conveyor=true;
}

void LinkClass::setYfix(zfix new_y)
{
    zfix dy=new_y-y;
    if(Lwpns.idFirst(wHookshot)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHookshot))->y+=dy;
	hs_starty+=(int32_t)dy;
    }
    
    if(Lwpns.idFirst(wHSHandle)>-1)
    {
        Lwpns.spr(Lwpns.idFirst(wHSHandle))->y+=dy;
    }
	
	if(chainlinks.Count()>0)
	{
		for(int32_t j=0; j<chainlinks.Count(); j++)
        {
            chainlinks.spr(j)->y+=dy;
        }
	}
    
    y=new_y;
    
    // A kludge
    if(!diagonalMovement && dir>=left)
        is_on_conveyor=true;
}

void LinkClass::setZfix(zfix new_z)
{
    if(isSideViewLink())
        return;
        
    if(z==0 && new_z > 0)
    {
        switch(action)
        {
        case swimming:
	{
            diveclk=0;
            action=walking; FFCore.setLinkAction(walking);
            break;
	}
            
        case waterhold1:
	{
            action=landhold1; FFCore.setLinkAction(landhold1);
            break;
	}
            
        case waterhold2:
	{
            action=landhold2; FFCore.setLinkAction(landhold2);
            break;
	}
            
        default:
            if(charging) //!DIMITODO: Let Link jump while charging sword
            {
                reset_swordcharge();
                attackclk=0;
            }
            
            break;
        }
    }
    
    z=(new_z>0 ? new_z : 0);
}

void LinkClass::setFall(zfix new_fall)
{
    fall=new_fall;
    jumping=-1;
}
void LinkClass::setClimbCoverX(int32_t new_x)
{
    climb_cover_x=new_x;
}
void LinkClass::setClimbCoverY(int32_t new_y)
{
    climb_cover_y=new_y;
}
int32_t  LinkClass::getLStep()
{
    return lstep;
}
int32_t  LinkClass::getCharging()
{
    return charging;
}
bool LinkClass::isCharged()
{
    return spins>0;
}
int32_t  LinkClass::getAttackClk()
{
    return attackclk;
}
void  LinkClass::setAttackClk(int32_t new_clk)
{
    attackclk=new_clk;
}
void LinkClass::setCharging(int32_t new_charging)
{
    charging=new_charging;
}
int32_t  LinkClass::getSwordClk()
{
    return swordclk;
}
int32_t  LinkClass::getItemClk()
{
    return itemclk;
}
void LinkClass::setSwordClk(int32_t newclk)
{
    swordclk=newclk;
}
void LinkClass::setItemClk(int32_t newclk)
{
    itemclk=newclk;
}
zfix  LinkClass::getModifiedX()
{
    zfix tempx=x;
    
    if(screenscrolling&&(dir==left))
    {
        tempx=tempx+256;
    }
    
    return tempx;
}

zfix  LinkClass::getModifiedY()
{
    zfix tempy=y;
    
    if(screenscrolling&&(dir==up))
    {
        tempy=tempy+176;
    }
    
    return tempy;
}

int32_t  LinkClass::getDir()
{
    return dir;
}
void LinkClass::setDir(int32_t newdir)
{
    dir=newdir;
    reset_hookshot();
}
int32_t  LinkClass::getHitDir()
{
    return hitdir;
}
void LinkClass::setHitDir(int32_t newdir)
{
    hitdir = newdir;
}
int32_t  LinkClass::getClk()
{
    return clk;
}
int32_t  LinkClass::getPushing()
{
    return pushing;
}
void LinkClass::Catch()
{
    if(!inwallm && (action==none || action==walking))
    {
        SetAttack();
        attackclk=0;
        attack=wCatching;
    }
}

bool LinkClass::getClock()
{
    return superman;
}
void LinkClass::setClock(bool state)
{
    superman=state;
}
int32_t  LinkClass::getAction() // Used by ZScript
{
    if(spins > 0)
        return isspinning;
    else if(charging > 0)
        return ischarging;
    else if(diveclk > 0)
        return isdiving;
    //else if (pushing > 0) return ispushing; //Needs a QR? -Z or make it an instruction as Link->Pushing? Probably better, as that has a clk?? 
        
    return action;
}

int32_t  LinkClass::getAction2() // Used by ZScript new FFCore.actions
{
    if(spins > 0)
        return isspinning;
    else if(charging > 0)
        return ischarging;
    else if(diveclk > 0)
        return isdiving;
    //else if (pushing > 0) return ispushing; //Needs a QR? -Z or make it an instruction as Link->Pushing? Probably better, as that has a clk?? 
        
    return -1;
}

void LinkClass::setAction(actiontype new_action) // Used by ZScript
{
	if(new_action==dying || new_action==won || new_action==scrolling ||
	   new_action==inwind || new_action==ischarging || new_action==sideswimischarging || 
	   new_action==hopping) //!DIMITODO: allow setting sideswimming stuff
		return; // Can't use these actions.
	
	if (!isSideViewLink() && (new_action>=sideswimming && new_action <= sideswimischarging))
		return;
	
	if(new_action==rafting)
	{
		if(get_bit(quest_rules, qr_DISALLOW_SETTING_RAFTING)) return;
		if(!(isRaftFlag(nextflag(x+8,y+8,dir,false))||isRaftFlag(nextflag(x+8,y+8,dir,true))))
			return;
	}
	
	
	if(magicitem>-1 && itemsbuf[magicitem].family==itype_faroreswind)
	{
		// Using Farore's Wind
		if(magiccastclk<96)
		{
			// Not cast yet; cancel it
			magicitem=-1;
			magiccastclk=0;
		}
		else
			// Already activated; don't do anything
			return;
	}
	
	if(action==inwind) // Remove from whirlwind
	{
		xofs=0;
		whirlwind=0;
		lstep=0;
		if ( dontdraw < 2 ) { dontdraw=0; }
	}
	else if(action==freeze||action==sideswimfreeze) // Might be in enemy wind
	{
		sprite* wind=0;
		bool foundWind=false;
		for(int32_t i=0; i<Ewpns.Count(); i++)
		{
			wind=Ewpns.spr(i);
			if(wind->id==ewWind && wind->misc==999)
			{
				foundWind=true;
				break;
			}
		}
		
		if(foundWind)
		{
			xofs=0;
			if ( dontdraw < 2 ) { dontdraw=false; }
			wind->misc=-1;
			x=wind->x;
			y=wind->y;
		}
	}
	
	//Unless compat rule is on, reset hopping clocks when writing action!
	if(action == hopping && !get_bit(quest_rules,qr_NO_OVERWRITING_HOPPING))
	{
		hopclk = 0;
		hopdir = -1;
	}
	
	if(new_action != attacking && new_action != sideswimattacking)
	{
		attackclk=0;
		
		if(attack==wHookshot)
			reset_hookshot();
	}
	if(new_action != isspinning && new_action != sideswimisspinning)
	{
		charging = 0;
		spins = 0;
	}
	
	if(action == falling && new_action != falling)
	{
		fallclk = 0; //Stop falling;
	}
	
	switch(new_action)
	{
	case isspinning:
	case sideswimisspinning:
		if(attack==wSword)
		{
			attackclk = SWORDCHARGEFRAME+1;
			charging = 0;
			
			if(spins==0)
				spins = 5;
		}
		return;
		
	case isdiving:
		if(action==swimming && diveclk==0)
		{
			int32_t flippers_id = current_item_id(itype_flippers);
			diveclk = (flippers_id < 0 ? 80 : (itemsbuf[flippers_id].misc1 + itemsbuf[flippers_id].misc2)); // Who cares about qr_NODIVING? It's the questmaker's business.
		}
		return;
		
	case drowning:
	case sidedrowning:
	//I would add a sanity check to see if Link is in water, but I *KNOW* that quests have used this 
	// INTENTIONALLY while Link is on Land, as a blink-out effect. :( -Z
		if(!drownclk)
			Drown();
			
		break;
	
	case lavadrowning:
		//Lavadrowning is just drowning but with a different argument. Simplicity! -Dimi
		if(!drownclk)
			Drown(1);
			
		break;
		
	case falling:
		if(!fallclk)
		{
			//If there is a pit under Link, use it's combo.
			if(int32_t c = getpitfall(x+8,y+(bigHitbox?8:12))) fallCombo = c;
			else if(int32_t c = getpitfall(x,y+(bigHitbox?0:8))) fallCombo = c;
			else if(int32_t c = getpitfall(x+15,y+(bigHitbox?0:8))) fallCombo = c;
			else if(int32_t c = getpitfall(x,y+15)) fallCombo = c;
			else if(int32_t c = getpitfall(x+15,y+15)) fallCombo = c;
			//Else, use a null value; triggers default pit values
			else fallCombo = 0;
			fallclk = PITFALL_FALL_FRAMES;
		}
		break;
		
	case gothit:
	case swimhit:
	case sideswimhit:
		if(!hclk)
			hclk=48;
			
		break;
		
	case landhold1:
	case landhold2:
	case waterhold1:
	case waterhold2:
	case sidewaterhold1:
	case sidewaterhold2:
		if(!holdclk)
			holdclk=130;
			
		attack=none;
		break;
		
	case attacking:
	case sideswimattacking:
		attack=none;
		break;
		
	default:
		break;
	}
	
	action=new_action; FFCore.setLinkAction(new_action);
}

void LinkClass::setHeldItem(int32_t newitem)
{
    holditem=newitem;
}
int32_t LinkClass::getHeldItem()
{
    return holditem;
}
bool LinkClass::isDiving()
{
	int32_t flippers_id = current_item_id(itype_flippers);
    return (diveclk > (flippers_id < 0 ? 30 : itemsbuf[flippers_id].misc2));
}
bool LinkClass::isSwimming()
{
    return ((action==swimming)||(action==sideswimming)||IsSideSwim()||
            (action==waterhold1)||(action==waterhold2)||
            (hopclk==0xFF));
}

void LinkClass::setDontDraw(byte new_dontdraw)
{
    dontdraw=new_dontdraw;
}

byte LinkClass::getDontDraw()
{
    return dontdraw;
}

void LinkClass::setHClk(int32_t newhclk)
{
    hclk=newhclk;
}

int32_t LinkClass::getHClk()
{
    return hclk;
}

int32_t LinkClass::getSpecialCave()
{
    return specialcave;    // used only by maps.cpp
}

void LinkClass::init()
{
	scale = 0;
	rotation = 0;
	do_animation = 1;
    if ( dontdraw != 2 ) {  dontdraw = 0; } //scripted dontdraw == 2, normal == 1, draw link == 0
    hookshot_used=false;
    hookshot_frozen=false;
    onpassivedmg=false;
    dir = up;
    damageovertimeclk = -1;
    newconveyorclk = 0;
    shiftdir = -1;
    sideswimdir = right;
    holddir = -1;
    landswim = 0;
    sdir = up;
    ilswim=true;
    walkable=false;
    moveflags = FLAG_OBEYS_GRAV | FLAG_CAN_PITFALL | FLAG_CAN_WATERDROWN;
    warp_sound = 0;
    subscr_speed = zinit.subscrSpeed;
	steprate = zinit.heroStep;
	is_warping = false;
	can_mirror_portal = true;
	
	hammer_swim_up_offset = hammeroffsets[0];
	hammer_swim_down_offset = hammeroffsets[1];
	hammer_swim_left_offset = hammeroffsets[2];
	hammer_swim_right_offset = hammeroffsets[3];
    
    if(get_bit(quest_rules,qr_NOARRIVALPOINT))
    {
        x=tmpscr->warpreturnx[0];
        y=tmpscr->warpreturny[0];
    }
    else
    {
        x=tmpscr->warparrivalx;
        y=tmpscr->warparrivaly;
    }
    
    z=fall=0;
    hzsz = 12; // So that flying peahats can still hit him.
    
    if(x==0)   dir=right;
    
    if(x==240) dir=left;
    
    if(y==0)   dir=down;
    
    if(y==160) dir=up;
    
    lstep=0;
    skipstep=0;
    autostep=false;
    attackclk=holdclk=hoverclk=jumping=raftclk=0;
    attack=wNone;
    attackid=-1;
    action=none; FFCore.setLinkAction(none); tempaction=none;
    xofs=0;
    yofs=playing_field_offset;
    cs=6;
    pushing=fairyclk=0;
    id=0;
    inlikelike=0;
    superman=inwallm=false;
    scriptcoldet=1;
    blowcnt=whirlwind=specialcave=0;
    hopclk=diveclk=fallclk=0;
	fallCombo = -1;
	pit_pulldir = -1;
    hopdir=-1;
    conveyor_flags=0;
    drunkclk=0;
    lstunclock = 0;
    drawstyle=3;
    ffwarp = false;
    stepoutindex=stepoutwr=stepoutdmap=stepoutscr=0;
    stepnext=stepsecret=-1;
    ffpit = false;
    entry_x=x;
    entry_y=y;
    falling_oldy = y;
    magiccastclk=0;
    magicitem = nayruitem = -1;
	last_lens_id = 0; //Should be -1 (-Z)
	misc_internal_link_flags = 0;
	last_cane_of_byrna_item_id = -1;
	on_sideview_ladder = false;
	switchblock_z = 0;
	switchblock_offset = false;
	extra_jump_count = 0;
	hoverflags = 0;
    lbunnyclock = 0;
    
    for(int32_t i=0; i<32; i++) miscellaneous[i] = 0;
    
    bigHitbox=(get_bit(quest_rules, qr_LTTPCOLLISION));
    diagonalMovement=(get_bit(quest_rules,qr_LTTPWALK));
    
    //2.6
	preventsubscreenfalling = false;  //-Z
	hurtsfx = 19; //getHurtSFX(); //Set the default sound. 
	flickerorflash = true; //flicker or flash unless disabled externally.
	walkspeed = 0; //not used, yet. -Z
	for ( int32_t q = 0; q < NUM_HIT_TYPES_USED; q++ ) lastHitBy[q][0] = 0; 
	for ( int32_t q = 0; q < NUM_HIT_TYPES_USED; q++ ) lastHitBy[q][1] = 0; 
	for ( int32_t q = 0; q < wMax; q++ ) 
	{
		defence[q] = link_defence[q]; //we will need to have a Link section in the quest load/save code! -Z Added 3/26/21 - Jman
		//zprint2("defence[%d] is: %d\n", q, defence[q]);
	}
	//Run script!
	if (( FFCore.getQuestHeaderInfo(vZelda) >= 0x255 ) && (game->get_hasplayed()) ) //if (!hasplayed) runs in game_loop()
	{
		ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_INIT, SCRIPT_LINK_INIT); 
		FFCore.deallocateAllArrays(SCRIPT_LINK, SCRIPT_LINK_INIT);
		FFCore.initZScriptLinkScripts(); //Clear the stack and the refinfo data to be ready for Link's active script. 
		setEntryPoints(LinkX(),LinkY()); //screen entry at spawn; //This should be after the init script, so that Link->X and Link->Y set by the script
						//are properly set by the engine.
	}
	FFCore.nostepforward = 0;
}

void LinkClass::draw_under(BITMAP* dest)
{
    int32_t c_raft=current_item_id(itype_raft);
    int32_t c_ladder=current_item_id(itype_ladder);
    
    if(action==rafting && c_raft >-1)
    {
        if(((dir==left) || (dir==right)) && (get_bit(quest_rules,qr_RLFIX)))
        {
            overtile16(dest, itemsbuf[c_raft].tile, x, y+playing_field_offset+4,
                       itemsbuf[c_raft].csets&15, rotate_value((itemsbuf[c_raft].misc>>2)&3)^3);
        }
        else
        {
            overtile16(dest, itemsbuf[c_raft].tile, x, y+playing_field_offset+4,
                       itemsbuf[c_raft].csets&15, (itemsbuf[c_raft].misc>>2)&3);
        }
    }
    
    if(ladderx+laddery && c_ladder >-1)
    {
        if((ladderdir>=left) && (get_bit(quest_rules,qr_RLFIX)))
        {
            overtile16(dest, itemsbuf[c_ladder].tile, ladderx, laddery+playing_field_offset,
                       itemsbuf[c_ladder].csets&15, rotate_value((itemsbuf[iRaft].misc>>2)&3)^3);
        }
        else
        {
            overtile16(dest, itemsbuf[c_ladder].tile, ladderx, laddery+playing_field_offset,
                       itemsbuf[c_ladder].csets&15, (itemsbuf[c_ladder].misc>>2)&3);
        }
    }
}

void LinkClass::drawshadow(BITMAP* dest, bool translucent)
{
    int32_t tempy=yofs;
    yofs+=8;
    shadowtile = wpnsbuf[spr_shadow].newtile;
    sprite::drawshadow(dest,translucent);
    yofs=tempy;
}

// The Stone of Agony reacts to these flags.
bool LinkClass::agonyflag(int32_t flag)
{
    switch(flag)
    {
    case mfWHISTLE:
    case mfBCANDLE:
    case mfARROW:
    case mfBOMB:
    case mfSBOMB:
    case mfBRANG:
    case mfMBRANG:
    case mfFBRANG:
    case mfSARROW:
    case mfGARROW:
    case mfRCANDLE:
    case mfWANDFIRE:
    case mfDINSFIRE:
    case mfWANDMAGIC:
    case mfREFMAGIC:
    case mfREFFIREBALL:
    case mfSWORD:
    case mfWSWORD:
    case mfMSWORD:
    case mfXSWORD:
    case mfSWORDBEAM:
    case mfWSWORDBEAM:
    case mfMSWORDBEAM:
    case mfXSWORDBEAM:
    case mfHOOKSHOT:
    case mfWAND:
    case mfHAMMER:
    case mfSTRIKE:
        return true;
    }
    
    return false;
}


// Find the attack power of the current melee weapon.
// The Whimsical Ring is applied on a target-by-target basis.
int32_t LinkClass::weaponattackpower()
{
    int32_t power = attack==wCByrna ? itemsbuf[directWpn>-1 ? directWpn : current_item_id(itype_cbyrna)].misc4 : directWpn>-1 ? itemsbuf[directWpn].power : (current_item_power(attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword));
    
    // Multiply it by the power of the spin attack/quake hammer, if applicable.
    power *= (spins>0 ? itemsbuf[current_item_id(attack==wHammer ? itype_quakescroll : (spins>5 || current_item_id(itype_spinscroll) < 0) ? itype_spinscroll2 : itype_spinscroll)].power : 1);
    return power;
}

#define NET_CLK_TOTAL 30
#define NET_DIR_INC (NET_CLK_TOTAL/3)
// Must only be called once per frame!
void LinkClass::positionNet(weapon *w, int32_t itemid)
{
	itemid = vbound(itemid, 0, MAXITEMS-1);
	int32_t t = w->o_tile,
		wx = 1, wy = 1;
	
	//Invert positioning clock if right-handed animation
	int32_t clock = (itemsbuf[itemid].flags&ITEM_FLAG2 ? (NET_CLK_TOTAL-1)-attackclk : attackclk);
	if(clock >= NET_CLK_TOTAL)
		w->dead = 0;
	int32_t tiledir = dir;
	switch(dir)
	{
		case up:
		{
			if(clock < NET_DIR_INC) tiledir = l_up;
			else if(clock >= NET_DIR_INC*2) tiledir = r_up;
			break;
		}
		case down:
		{
			if(clock < NET_DIR_INC) tiledir = r_down;
			else if(clock >= NET_DIR_INC*2) tiledir = l_down;
			break;
		}
		case left:
		{
			if(clock < NET_DIR_INC) tiledir = l_down;
			else if(clock >= NET_DIR_INC*2) tiledir = l_up;
			break;
		}
		case right:
		{
			if(clock < NET_DIR_INC) tiledir = r_up;
			else if(clock >= NET_DIR_INC*2) tiledir = r_down;
			break;
		}
	}
	int32_t offs = 0;
	if(tiledir > right)
		offs = ((clock%NET_DIR_INC)<NET_DIR_INC/2) ? 1 : 0;
	else offs = vbound(((clock%NET_DIR_INC)/(NET_DIR_INC/3))-1,-1,1);
	//One of 8 positions
	switch(tiledir)
	{
		case up:
		{
			wx = 6*offs;
			wy = -14;
			break;
		}
		case r_up:
		{
			wx = (offs ? 10 : 14);
			wy = (offs ? -12 : -10);
			break;
		}
		case right:
		{
			wx = 14;
			wy = 6*offs;
			break;
		}
		case r_down:
		{
			wx = (offs ? 14 : 10);
			wy = (offs ? 10 : 12);
			break;
		}
		case down:
		{
			wx = -6*offs;
			wy = 14;
			break;
		}
		case l_down:
		{
			wx = (offs ? -10 : -14);
			wy = (offs ? 12 : 10);
			break;
		}
		case left:
		{
			wx = -14;
			wy = -6*offs;
			break;
		}
		case l_up:
		{
			wx = (offs ? -14 : -10);
			wy = (offs ? -10 : -12);
			break;
		}
	}
	
    w->x = x+wx;
    w->y = y+wy-(54-(yofs));
    w->z = (z+zofs);
    w->tile = t+tiledir;
    w->power = 0;
    w->dir = dir;
}
void LinkClass::positionSword(weapon *w, int32_t itemid)
{
	//if ( w->ScriptGenerated ) return; //t/b/a for script-generated swords.
	//if ( itemsbuf[itemid].ScriptGenerated ) return; //t/b/a for script-generated swords.
    itemid=vbound(itemid, 0, MAXITEMS-1);
    // Place a sword weapon at the right spot.
    int32_t wy=1;
    int32_t wx=1;
    int32_t f=0,t,cs2;
    
    t = w->o_tile;
    cs2 = w->o_cset;
    slashxofs=0;
    slashyofs=0;
    
    switch(dir)
    {
    case up:
        wx=-1;
        wy=-12;
        
        if(game->get_canslash() && w->id==wSword && itemsbuf[itemid].flags & ITEM_FLAG4 && charging==0)
        {
            if(attackclk>10) //extended stab
            {
                slashyofs-=3;
                wy-=2;
            }
            
            if(attackclk>=14) //retracting stab
            {
                slashyofs+=3;
                wy+=2;
            }
        }
        else
        {
            if(attackclk==SWORDCHARGEFRAME)
            {
                wy+=4;
            }
            else if(attackclk==13)
            {
                wy+=4;
            }
            else if(attackclk==14)
            {
                wy+=8;
            }
        }
        
        break;
        
    case down:
        f=get_bit(quest_rules,qr_SWORDWANDFLIPFIX)?3:2;
        wy=11;
        
        if(game->get_canslash() && w->id==wSword && itemsbuf[itemid].flags & ITEM_FLAG4 && charging==0)
        {
            if(attackclk>10) //extended stab
            {
                slashyofs+=3;
                wy+=2;
            }
            
            if(attackclk>=14) //retracting stab
            {
                slashyofs-=3;
                wy-=2;
            }
        }
        else
        {
            if(attackclk==SWORDCHARGEFRAME)
            {
                wy-=2;
            }
            else if(attackclk==13)
            {
                wy-=4;
            }
            else if(attackclk==14)
            {
                wy-=8;
            }
        }
        
        break;
        
    case left:
        f=1;
        wx=-11;
        ++t;
        
        if(game->get_canslash() && w->id==wSword && itemsbuf[itemid].flags & ITEM_FLAG4 && charging==0)
        {
            if(attackclk>10)  //extended stab
            {
                slashxofs-=4;
                wx-=7;
            }
            
            if(attackclk>=14) //retracting stab
            {
                slashxofs+=3;
                wx+=7;
            }
        }
        else
        {
            if(attackclk==SWORDCHARGEFRAME)
            {
                wx+=2;
            }
            else if(attackclk==13)
            {
                wx+=4;
            }
            else if(attackclk==14)
            {
                wx+=8;
            }
        }
        
        break;
        
    case right:
        wx=11;
        ++t;
        
        if(game->get_canslash() && w->id==wSword && itemsbuf[itemid].flags & ITEM_FLAG4 && charging==0)
        {
            if(attackclk>10) //extended stab
            {
                slashxofs+=4;
                wx+=7;
            }
            
            if(attackclk>=14) //retracting stab
            {
                slashxofs-=3;
                wx-=7;
            }
        }
        else
        {
            if(attackclk==SWORDCHARGEFRAME)
            {
                wx-=2;
            }
            else if(attackclk==13)
            {
                wx-=4;
            }
            else if(attackclk==14)
            {
                wx-=8;
            }
        }
        
        break;
    }
    
    if(game->get_canslash() && itemsbuf[itemid].flags & ITEM_FLAG4 && attackclk<11)
    {
        int32_t wpn2=itemsbuf[itemid].wpn2;
        wpn2=vbound(wpn2, 0, MAXWPNS);
        
        //slashing tiles
        switch(dir)
        {
        case up:
            wx=15;
            wy=-3;
            ++t;
            f=0;                                     //starts pointing right
            
            if(attackclk>=7)
            {
                wy-=9;
                wx-=3;
                t = wpnsbuf[wpn2].newtile;
                cs2 = wpnsbuf[wpn2].csets&15;
                f=0;
            }
            
            break;
            
        case down:
            wx=-13;
            wy=-1;
            ++t;
            f=1;                                     //starts pointing left
            
            if(attackclk>=7)
            {
                wy+=15;
                wx+=2;
                t = wpnsbuf[wpn2].newtile;
                cs2 = wpnsbuf[wpn2].csets&15;
                ++t;
                f=0;
            }
            
            break;
            
        case left:
            wx=3;
            wy=-15;
            --t;
            f=0;                                     //starts pointing up
            
            if(attackclk>=7)
            {
                wx-=15;
                wy+=3;
                slashxofs-=1;
                t = wpnsbuf[wpn2].newtile;
                cs2 = wpnsbuf[wpn2].csets&15;
                t+=2;
                f=0;
            }
            
            break;
            
        case right:
            --t;
            
            if(spins>0 || (itemsbuf[itemid].flags & ITEM_FLAG8))
            {
                wx=1;
                wy=13;
                f=2;
            }
            else
            {
                wx=3;
                wy=-15;
                f=0;
            }
            
            if(attackclk>=7)
            {
                wx+=15;
                slashxofs+=1;
                t = wpnsbuf[wpn2].newtile;
                cs2 = wpnsbuf[wpn2].csets&15;
                
                if(spins>0 || (itemsbuf[itemid].flags & ITEM_FLAG8))
                {
                    wx-=1;
                    wy-=2;
                }
                else
                {
                    t+=3;
                    f=0;
                    wy+=3;
                }
            }
            
            break;
        }
    }
    
    int32_t itemid2 = current_item_id(itype_chargering);
    
    if(charging>(itemid2>=0 ? itemsbuf[itemid2].misc1 : 64))
    {
        cs2=(BSZ ? (frame&3)+6 : ((frame>>2)&1)+7);
    }
    
    /*if(BSZ || ((isdungeon() && currscr<128) && !get_bit(quest_rules,qr_LINKDUNGEONPOSFIX)))
    {
      wy+=2;
    }*/
    w->x = x+wx;
    w->y = y+wy-(54-(yofs+slashyofs));
    w->z = (z+zofs);
    w->tile = t;
    w->flip = f;
    w->power = weaponattackpower();
    w->dir = dir;
}

void LinkClass::draw(BITMAP* dest)
{
	/*{
		char buf[36];
		//sprintf(buf,"%d %d %d %d %d %d %d",dir, action, attack, attackclk, charging, spins, tapping);
		textout_shadowed_ex(framebuf,font, buf, 2,72,WHITE,BLACK,-1);
	}*/
	int32_t oxofs, oyofs;
	bool shieldModify = false;
	bool invisible=(dontdraw>0) || (tmpscr->flags3&fINVISLINK);
	
	if(action==dying)
	{
		if(!invisible)
		{
			if ( script_link_cset > -1 ) cs = script_link_cset;
				sprite::draw(dest);
		}
		return;
	}
	
	bool useltm=(get_bit(quest_rules,qr_EXPANDEDLTM) != 0);
	
	oxofs=xofs;
	oyofs=yofs;
	
	if(!invisible)
		yofs = oyofs-((!BSZ && isdungeon() && currscr<128 && !get_bit(quest_rules,qr_LINKDUNGEONPOSFIX)) ? 2 : 0);
		
	// Stone of Agony
	bool agony=false;
	int32_t agonyid = current_item_id(itype_agony);
	
	if(invisible)
		goto attack;
		
	if(agonyid>-1)
	{
		int32_t power=itemsbuf[agonyid].power;
		int32_t left=static_cast<int32_t>(x+8-power)&0xF0; // Check top-left pixel of each tile
		int32_t right=(static_cast<int32_t>(x+8+power)&0xF0)+16;
		int32_t top=static_cast<int32_t>(y+(bigHitbox ? 8 : 12)-power)&0xF0;
		int32_t bottom=(static_cast<int32_t>(y+(bigHitbox ? 8 : 12)+power)&0xF0)+16;
		
		for(int32_t x=left; x<right; x+=16)
		{
			for(int32_t y=top; y<bottom; y+=16)
			{
				if(agonyflag(MAPFLAG(x, y)) || agonyflag(MAPCOMBOFLAG(x, y)))
				{
					agony=true;
					x=right; // Break out of outer loop
					break;
				}
			}
		}
	}
	
	cs = 6;
	if ( script_link_cset > -1 ) cs = script_link_cset;
	if(!get_bit(quest_rules,qr_LINKFLICKER))
	{
		if(superman && getCanLinkFlicker())
		{
			cs += (((~frame)>>1)&3);
		}
		else if(hclk&&(NayrusLoveShieldClk<=0) && getCanLinkFlicker())
		{
			cs += ((hclk>>1)&3);
		}
	}
	
attack:

	if(attackclk || (action==attacking||action==sideswimattacking))
	{
		/* Spaghetti code constants!
		* - Link.attack contains a weapon type...
		* - which must be converted to an itype...
		* - which must be converted to an item ID...
		* - which is used to acquire a wpn ID! Aack!
		*/
		int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : attack==wBugNet ? itype_bugnet : itype_sword);
		int32_t itemid = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
		itemid=vbound(itemid, 0, MAXITEMS-1);
		// if ( itemsbuf[itemid].ScriptGenerated ) return; //t/b/a for script-generated swords.
		if(attackclk>4||attack==wBugNet||(attack==wSword&&game->get_canslash()))
		{
			if(attack == wBugNet)
			{
				weapon *w=NULL;
				bool found = false;
				for(int32_t q = 0; q < Lwpns.Count(); ++q)
				{
					w = (weapon*)Lwpns.spr(q);
					if(w->id == wBugNet)
					{
						found = true;
						break;
					}
				}
				if(!found)
				{
					Lwpns.add(new weapon((zfix)0,(zfix)0,(zfix)0,wBugNet,0,0,dir,itemid,getUID(),false,false,true));
					
					w = (weapon*)Lwpns.spr(Lwpns.Count()-1);
				}
				positionNet(w, itemid);
			}
			else if((attack==wSword || attack==wWand || ((attack==wFire || attack==wCByrna) && itemsbuf[itemid].wpn)) && wpnsbuf[itemsbuf[itemid].wpn].newtile)
			{
				// Create a sword weapon at the right spot.
				weapon *w=NULL;
				bool found = false;
				
				// Look for pre-existing sword
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					w = (weapon*)Lwpns.spr(i);
					
					if(w->id == (attack==wSword ? wSword : wWand))
					{
						found = true;
						break;
					}
				}
				
				if(!found)  // Create one if sword nonexistant
				{
					Lwpns.add(new weapon((zfix)0,(zfix)0,(zfix)0,(attack==wSword ? wSword : wWand),0,0,dir,itemid,getUID(),false,false,true));
					w = (weapon*)Lwpns.spr(Lwpns.Count()-1);
					
					positionSword(w,itemid);
					
					// Stone of Agony
					if(agony)
					{
						w->y-=!(frame%zc_max(60-itemsbuf[agonyid].misc1,2));
					}
				}
				
				// These are set by positionSword(), above or in checkstab()
				yofs += slashyofs;
				xofs += slashxofs;
				slashyofs = slashxofs = 0;
			}
		}
		
		if(attackclk<7
				|| (attack==wSword &&
					(attackclk<(game->get_canslash()?15:13) || (charging>0 && attackclk!=SWORDCHARGEFRAME)))
				|| ((attack==wWand || attack==wFire || attack==wCByrna) && attackclk<13)
				|| (attack==wHammer && attackclk<=30))
		{
			if(!invisible)
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimstab:ls_stab, dir, zinit.linkanimationstyle);
				
				if((game->get_canslash() && (attack==wSword || attack==wWand || attack==wFire || attack==wCByrna)) && itemsbuf[itemid].flags&ITEM_FLAG4 && (attackclk<7))
				{
					linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimslash:ls_slash, dir, zinit.linkanimationstyle);
				}
				
				if((attack==wHammer) && (attackclk<13))
				{
					linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimpound:ls_pound, dir, zinit.linkanimationstyle);
				}
				
				if(useltm)
				{
					if ( script_link_sprite <= 0 ) tile+=getTileModifier();
				}
			
				// Stone of Agony
				if(agony)
				{
					yofs-=!(frame%zc_max(60-itemsbuf[agonyid].misc1,3));
				}

				//Probably what makes Link flicker, except for the QR check. What makes him flicker when that rule is off?! -Z
				
				//I'm pretty sure he doesn't flicker when the rule is off. Also, take note of the parenthesis after the ! in this if statement; I was blind and didn't see it, and thought this code did something completely different. -Deedee
				if (!(get_bit(quest_rules, qr_LINKFLICKER) && ((superman || hclk) && (frame & 1))))
				{
					masked_draw(dest);
				}

				//Prevent flickering -Z
				if (!getCanLinkFlicker()) masked_draw(dest);
			}
			
			if(attack!=wHammer)
			{
				xofs=oxofs;
				yofs=oyofs;
				return;
			}
		}
		
		if(attack==wHammer) // To do: possibly abstract this out to a positionHammer routine?
		{
			int32_t wy=1;
			int32_t wx=1;
			int32_t f=0,t,cs2;
			weapon *w=NULL;
			bool found = false;
			
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				w = (weapon*)Lwpns.spr(i);
				
				if(w->id == wHammer)
				{
					found = true;
					break;
				}
			}
			
			if(!found)
			{
				Lwpns.add(new weapon((zfix)0,(zfix)0,(zfix)0,wHammer,0,0,dir,itemid,getUID(),false,false,true));
				w = (weapon*)Lwpns.spr(Lwpns.Count()-1);
				found = true;
			}
			
			t = w->o_tile;
			cs2 = w->o_cset;
			
			switch(dir)
			{
			case up:
				wx=-1;
				wy=-15;
				if (IsSideSwim())wy+=hammer_swim_up_offset;
				
				if(attackclk>=13)
				{
					wx-=1;
					wy+=1;
					++t;
				}
				
				if(attackclk>=15)
				{
					if (IsSideSwim())wy-=hammer_swim_up_offset;
					++t;
				}
				
				break;
				
			case down:
				wx=3;
				wy=-14;
				if (IsSideSwim())wy+=hammer_swim_down_offset;
				t+=3;
				
				if(attackclk>=13)
				{
					wy+=16;
					++t;
				}
				
				if(attackclk>=15)
				{
					wx-=1;
					wy+=12;
					if (IsSideSwim())wy-=hammer_swim_down_offset;
					++t;
				}
				
				break;
				
			case left:
				wx=0;
				wy=-14;
				if (IsSideSwim())wy+=hammer_swim_left_offset;
				t+=6;
				f=1;
				
				if(attackclk>=13)
				{
					wx-=7;
					wy+=8;
					++t;
				}
				
				if(attackclk>=15)
				{
					wx-=8;
					wy+=8;
					if (IsSideSwim())wy-=hammer_swim_left_offset;
					++t;
				}
				
				break;
				
			case right:
				wx=0;
				wy=-14;
				if (IsSideSwim())wy+=hammer_swim_right_offset;
				t+=6;
				
				if(attackclk>=13)
				{
					wx+=7;
					wy+=8;
					++t;
				}
				
				if(attackclk>=15)
				{
					wx+=8;
					wy+=8;
					if (IsSideSwim())wy-=hammer_swim_right_offset;
					++t;
				}
				
				break;
			}
			
			if(BSZ || ((isdungeon() && currscr<128) && !get_bit(quest_rules,qr_LINKDUNGEONPOSFIX)))
			{
				wy+=2;
			}
			
			// Stone of Agony
			if(agony)
			{
				wy-=!(frame%zc_max(60-itemsbuf[agonyid].misc1,3));
			}
			
			w->x = x+wx;
			w->y = y+wy-(54-yofs);
			w->z = (z+zofs);
			w->tile = t;
			w->flip = f;
			w->hxsz=20;
			w->hysz=20;
			
			if(dir>down)
			{
				w->hysz-=6;
			}
			else
			{
				w->hxsz-=6;
				w->hyofs=4;
			}
			
			w->power = weaponattackpower();
			
			if(attackclk==15 && z==0 && (sideviewhammerpound() || !isSideViewLink()))
			{
				sfx(((iswaterex(MAPCOMBO(x+wx+8,y+wy), currmap, currscr, -1, x+wx+8, y+wy, true) || COMBOTYPE(x+wx+8,y+wy)==cSHALLOWWATER) && get_bit(quest_rules,qr_MORESOUNDS)) ? WAV_ZN1SPLASH : itemsbuf[itemid].usesound,pan(x.getInt()));
			}
			
			xofs=oxofs;
			yofs=oyofs;
			return;
		}
	}
	else if(!charging && !spins)  // remove the sword
	{
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			weapon *w = (weapon*)Lwpns.spr(i);
			
			if(w->id == wSword || w->id == wHammer || w->id==wWand)
				w->dead=1;
		}
	}
	
	if(invisible)
	{
		xofs=oxofs;
		yofs=oyofs;
		return;
	}
	
	if(action != casting && action != sideswimcasting)
	{
		// Keep this consistent with checkspecial2, line 7800-ish...
		bool inwater = iswaterex(MAPCOMBO(x+4,y+9), currmap, currscr, -1, x+4, y+9, true, false)  && iswaterex(MAPCOMBO(x+4,y+15), currmap, currscr, -1, x+4, y+15, true, false) &&  iswaterex(MAPCOMBO(x+11,y+9), currmap, currscr, -1, x+11, y+9, true, false) && iswaterex(MAPCOMBO(x+11,y+15), currmap, currscr, -1, x+11, y+15, true, false);
		
		int32_t jumping2 = int32_t(jumping*((zinit.gravity2 / 100)/16.0));
		
		//if (jumping!=0) al_trace("%d %d %f %d\n",jumping,zinit.gravity,zinit.gravity/16.0,jumping2);
		switch(zinit.linkanimationstyle)
		{
		case las_original:                                               //normal
			if(action==drowning)
			{
				if(inwater)
				{
					linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_drown, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile+=((frame>>3) & 1)*(extend==2?2:1);
				}
				else
				{
					xofs=oxofs;
					yofs=oyofs;
					return;
				}
			}
			else if(action==lavadrowning)
			{
				linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_lavadrown, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((frame>>3) & 1)*(extend==2?2:1);
			}
			else if(action==sidedrowning)
			{
				linktile(&tile, &flip, &extend, ls_sidedrown, down, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((frame>>3) & 1)*(extend==2?2:1);
			}
			else if (action == sideswimming || action == sideswimhit)
			{
				linktile(&tile, &flip, &extend, ls_sideswim, dir, zinit.linkanimationstyle);
				
				if(lstep>=6)
				{
					if(dir==up)
					{
						if ( script_link_sprite <= 0 ) ++flip;
					}
					else
					{
						if ( script_link_sprite <= 0 ) extend==2?tile+=2:++tile;
					}
				}
			}
			else if(action==swimming || action==swimhit || hopclk==0xFF)
			{
				linktile(&tile, &flip, &extend, is_moving()?ls_swim:ls_float, dir, zinit.linkanimationstyle);
				
				if(lstep>=6)
				{
					if(dir==up)
					{
						if ( script_link_sprite <= 0 ) ++flip;
					}
					else
					{
						if ( script_link_sprite <= 0 ) extend==2?tile+=2:++tile;
					}
				}
				
				if(isDiving())
				{
					linktile(&tile, &flip, &extend, ls_dive, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile+=((frame>>3) & 1)*(extend==2?2:1);
				}
			}
			else if(charging > 0 && attack != wHammer)
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimcharge:ls_charge, dir, zinit.linkanimationstyle);
				
				if(lstep>=6)
				{
					if(dir==up)
					{
						if ( script_link_sprite <= 0 ) ++flip;
					}
					else
					{
						if ( script_link_sprite <= 0 ) extend==2?tile+=2:++tile;
					}
				}
			}
			else if((z>0 || isSideViewLink()) && jumping2>0 && jumping2<24 && game->get_life()>0 && action!=rafting)
			{
				linktile(&tile, &flip, &extend, ls_jump, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((int32_t)jumping2/8)*(extend==2?2:1);
			}
			else if(fallclk>0)
			{
				linktile(&tile, &flip, &extend, ls_falling, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((PITFALL_FALL_FRAMES-fallclk)/10)*(extend==2?2:1);
			}
			else
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswim:ls_walk, dir, zinit.linkanimationstyle);
				
				if(dir>up)
				{
					useltm=true;
					shieldModify=true;
				}
				
				if(lstep>=6)
				{
					if(dir==up)
					{
						if ( script_link_sprite <= 0 ) ++flip;
					}
					else
					{
						if ( script_link_sprite <= 0 ) extend==2?tile+=2:++tile;
					}
				}
			}
			
			break;
			
		case las_bszelda:                                               //BS
			if(action==drowning)
			{
				if(inwater)
				{
					linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_drown, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				}
				else
				{
					xofs=oxofs;
					yofs=oyofs;
					return;
				}
			}
			else if (action == sidedrowning)
			{
				linktile(&tile, &flip, &extend, ls_sidedrown, down, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if(action==lavadrowning)
			{
				linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_lavadrown, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if (action == sideswimming || action == sideswimhit)
			{
				linktile(&tile, &flip, &extend, ls_sideswim, dir, zinit.linkanimationstyle);
				
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if(action==swimming || action==swimhit || hopclk==0xFF)
			{
				if (get_bit(quest_rules, qr_COPIED_SWIM_SPRITES)) 
				{
					linktile(&tile, &flip, &extend, ls_walk, dir, zinit.linkanimationstyle);
				}
				else
				{
					linktile(&tile, &flip, &extend, is_moving()?ls_swim:ls_float, dir, zinit.linkanimationstyle);
				}
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				
				if(isDiving())
				{
					if (get_bit(quest_rules, qr_COPIED_SWIM_SPRITES)) 
					{
						linktile(&tile, &flip, &extend, ls_walk, dir, zinit.linkanimationstyle);
					}
					else
					{
						linktile(&tile, &flip, &extend, ls_dive, dir, zinit.linkanimationstyle);
					}
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				}
			}
			else if(charging > 0 && attack != wHammer)
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimcharge:ls_charge, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if((z>0 || isSideViewLink()) && jumping2>0 && jumping2<24 && game->get_life()>0)
			{
				linktile(&tile, &flip, &extend, ls_jump, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((int32_t)jumping2/8)*(extend==2?2:1);
			}
			else if(fallclk>0)
			{
				linktile(&tile, &flip, &extend, ls_falling, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += ((PITFALL_FALL_FRAMES-fallclk)/10)*(extend==2?2:1);
			}
			else
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswim:ls_walk, dir, zinit.linkanimationstyle);
				
				if(dir>up)
				{
					useltm=true;
					shieldModify=true;
				}
				
				/*
				else if (dir==up)
				{
				useltm=true;
				}
				*/
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			
			break;
			
		case las_zelda3slow:                                           //8-frame Zelda 3 (slow)
		case las_zelda3:                                               //8-frame Zelda 3
			if(action == drowning)
			{
				if(inwater)
				{
					linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_drown, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				}
				else
				{
					xofs=oxofs;
					yofs=oyofs;
					return;
				}
			}
			else if(action == lavadrowning)
			{
					linktile(&tile, &flip, &extend, (drownclk > 60) ? ls_float : ls_lavadrown, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			
			}else if(action == sidedrowning)
			{
					linktile(&tile, &flip, &extend, ls_sidedrown, down, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if (action == sideswimming || action == sideswimhit)
			{
				linktile(&tile, &flip, &extend, ls_sideswim, dir, zinit.linkanimationstyle);
				
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
			}
			else if(action == swimming || action==swimhit || hopclk==0xFF)
			{
				linktile(&tile, &flip, &extend, is_moving()?ls_swim:ls_float, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				
				if(isDiving())
				{
					linktile(&tile, &flip, &extend, ls_dive, dir, zinit.linkanimationstyle);
					if ( script_link_sprite <= 0 ) tile += anim_3_4(lstep,7)*(extend==2?2:1);
				}
			}
			else if(charging > 0 && attack != wHammer)
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswimcharge:ls_charge, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=(extend==2?2:1);
				//int32_t l=link_count/link_animation_speed;
				int32_t l=(link_count/link_animation_speed)&15;
				//int32_t l=((p[lt_clock]/link_animation_speed)&15);
				l-=((l>3)?1:0)+((l>12)?1:0);
				if ( script_link_sprite <= 0 ) tile+=(l/2)*(extend==2?2:1);
			}
			else if((z>0 || isSideViewLink()) && jumping2>0 && jumping2<24 && game->get_life()>0)
			{
				linktile(&tile, &flip, &extend, ls_jump, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile+=((int32_t)jumping2/8)*(extend==2?2:1);
			}
			else if(fallclk>0)
			{
				linktile(&tile, &flip, &extend, ls_falling, dir, zinit.linkanimationstyle);
				if ( script_link_sprite <= 0 ) tile += ((PITFALL_FALL_FRAMES-fallclk)/10)*(extend==2?2:1);
			}
			else
			{
				linktile(&tile, &flip, &extend, (IsSideSwim())?ls_sideswim:ls_walk, dir, zinit.linkanimationstyle);
				
				if(action == walking || action == climbcoverbottom || action == climbcovertop)
				{
					if ( script_link_sprite <= 0 ) tile += (extend == 2 ? 2 : 1);
				}
				
				if(dir>up)
				{
					useltm=true;
					shieldModify=true;
				}
				
				if(action == walking || action == hopping || action == climbcoverbottom || action == climbcovertop)
				{
					//tile+=(extend==2?2:1);
					//tile+=(((active_count>>2)%8)*(extend==2?2:1));
					int32_t l = link_count / link_animation_speed;
					l -= ((l > 3) ? 1 : 0) + ((l > 12) ? 1 : 0);
					if ( script_link_sprite <= 0 ) tile += (l / 2) * (extend == 2 ? 2 : 1);
				}
			}
			
			break;
			
		default:
			break;
		}
	}
	
	yofs = oyofs-((!BSZ && isdungeon() && currscr<128 && !get_bit(quest_rules,qr_LINKDUNGEONPOSFIX)) ? 2 : 0);
	
	if(action==won)
	{
		yofs=playing_field_offset - 2;
	}
	
	if(action==landhold1 || action==landhold2)
	{
		useltm=(get_bit(quest_rules,qr_EXPANDEDLTM) != 0);
		yofs = oyofs-((!BSZ && isdungeon() && currscr<128 && !get_bit(quest_rules,qr_LINKDUNGEONPOSFIX)) ? 2 : 0);
		linktile(&tile, &flip, &extend, (action==landhold1)?ls_landhold1:ls_landhold2, dir, zinit.linkanimationstyle);
	}
	else if(action==waterhold1 || action==waterhold2)
	{
		useltm=(get_bit(quest_rules,qr_EXPANDEDLTM) != 0);
		linktile(&tile, &flip, &extend, (action==waterhold1)?ls_waterhold1:ls_waterhold2, dir, zinit.linkanimationstyle);
	}
	else if(action==sidewaterhold1 || action==sidewaterhold2)
	{
		useltm=(get_bit(quest_rules,qr_EXPANDEDLTM) != 0);
		linktile(&tile, &flip, &extend, (action==sidewaterhold1)?ls_sidewaterhold1:ls_sidewaterhold2, dir, zinit.linkanimationstyle);
	}
	
	if(action!=casting && action!=sideswimcasting)
	{
		if(useltm)
		{
			if ( script_link_sprite <= 0 ) tile+=getTileModifier();
		}
	}
	
	// Stone of Agony
	if(agony)
	{
		yofs-=!(frame%zc_max(60-itemsbuf[agonyid].misc1,3));
	}
	
	if(!(get_bit(quest_rules,qr_LINKFLICKER)&&((superman||hclk)&&(frame&1))))
	{
		masked_draw(dest);
	}
	
	//draw held items after Link so they don't go behind his head
	if(action==landhold1 || action==landhold2)
	{
		if(holditem > -1)
		{
			if(get_bit(quest_rules,qr_HOLDITEMANIMATION))
			{
				putitem2(dest,x-((action==landhold1)?4:0),y+yofs-16-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem,lens_hint_item[holditem][0], lens_hint_item[holditem][1], 0);
			}
			else
			{
				putitem(dest,x-((action==landhold1)?4:0),y+yofs-16-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem);
			}
		}
	}
	else if(action==waterhold1 || action==waterhold2)
	{
		if(holditem > -1)
		{
			if(get_bit(quest_rules,qr_HOLDITEMANIMATION))
			{
				putitem2(dest,x-((action==waterhold1)?4:0),y+yofs-12-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem,lens_hint_item[holditem][0], lens_hint_item[holditem][1], 0);
			}
			else
			{
				putitem(dest,x-((action==waterhold1)?4:0),y+yofs-12-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem);
			}
		}
	}
	else if(action==sidewaterhold1 || action==sidewaterhold2) //!DIMITODO: Check to see if this looks right or if it needs waterhold's offset.
	{
		if(holditem > -1)
		{
			if(get_bit(quest_rules,qr_HOLDITEMANIMATION))
			{
				putitem2(dest,x-((action==sidewaterhold1)?4:0),y+yofs-16-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem,lens_hint_item[holditem][0], lens_hint_item[holditem][1], 0);
			}
			else
			{
				putitem(dest,x-((action==sidewaterhold1)?4:0),y+yofs-16-(get_bit(quest_rules, qr_NOITEMOFFSET)),holditem);
			}
		}
	}
	if(fairyclk==0||(get_bit(quest_rules,qr_NOHEARTRING)))
	{
		xofs=oxofs;
		yofs=oyofs;
		return;
	}
	
	double a2 = fairyclk*2*PI/80 + (PI/2);
	int32_t hearts=0;
	//  int32_t htile = QHeader.dat_flags[ZQ_TILES] ? 2 : 0;
	int32_t htile = 2;
	
	do
	{
		int32_t nx=125;
		
		if(get_bit(quest_rules,qr_HEARTRINGFIX))
		{
			nx=x;
		}
		
		int32_t ny=88;
		
		if(get_bit(quest_rules,qr_HEARTRINGFIX))
		{
			ny=y;
		}
		
		double tx = cos(a2)*53  +nx;
		double ty = -sin(a2)*53 +ny+playing_field_offset;
		overtile8(dest,htile,int32_t(tx),int32_t(ty),1,0);
		a2-=PI/4;
		++hearts;
	}
	while(a2>PI/2 && hearts<8);
	
	xofs=oxofs;
	yofs=oyofs;
}

void LinkClass::masked_draw(BITMAP* dest)
{
    if(isdungeon() && currscr<128 && (x<16 || x>224 || y<18 || y>146) && !get_bit(quest_rules,qr_FREEFORM))
    {
        // clip under doorways
        BITMAP *sub=create_sub_bitmap(dest,16,playing_field_offset+16,224,144);
        
        if(sub!=NULL)
        {
            yofs -= (playing_field_offset+16);
            xofs -= 16;
            sprite::draw(sub);
            xofs=0;
            yofs += (playing_field_offset+16);
            destroy_bitmap(sub);
        }
    }
    else
    {
        sprite::draw(dest);
    }
    
    return;
}

// separate case for sword/wand/hammer/slashed weapons only
// the main weapon checking is in the global function check_collisions()
bool LinkClass::checkstab()
{
    if(action!=attacking && action!=sideswimattacking || (attack!=wSword && attack!=wWand && attack!=wHammer && attack!=wCByrna && attack!=wFire && attack != wBugNet)
            || (attackclk<=4))
        return false;
        
    weapon *w=NULL;
    
    int32_t wx=0,wy=0,wz=0,wxsz=0,wysz=0;
    bool found = false;
    int32_t melee_weapon_index = 0;
	int32_t parentitem=-1;
    
    for(int32_t i=0; i<Lwpns.Count(); i++)
    {
        w = (weapon*)Lwpns.spr(i);
        
        if(w->id == (attack==wCByrna || attack==wFire ? wWand : attack))  // Kludge: Byrna and Candle sticks are wWand-type.
        {
            found = true;
            melee_weapon_index = i+1;
            // Position the sword as Link slashes with it.
            if(w->id!=wHammer&&w->id!=wBugNet)
                positionSword(w,w->parentitem);
                
            wx=w->x;
            wy=w->y;
            wz=w->z;
            wxsz = w->hxsz;
            wysz = w->hysz;
			parentitem = w->parentitem;
            break;
        }
    }
	
    if(attack==wSword && attackclk>=14 && charging==0)
        return false;
        
    if(!found)
        return false;
		
	if(attack == wFire)
		return false;
	
	if(attack==wHammer)
	{
		if(attackclk<15)
		{
			switch(w->dir)
			{
			case up:
				wx=x-1;
				wy=y-4;
				break;
				
			case down:
				wx=x+8;
				wy=y+28;
				break; // This is consistent with 2.10
				
			case left:
				wx=x-13;
				wy=y+14;
				break;
				
			case right:
				wx=x+21;
				wy=y+14;
				break;
			}
			
			if(attackclk==12 && z==0 && sideviewhammerpound())
			{
		//decorations.add(new dHammerSmack((zfix)wx, (zfix)wy, dHAMMERSMACK, 0));
				/* The hammer smack sprites weren't being positioned properly if Link changed directions on the same frame that they are created.
		switch(dir)
				{
				case up:
					decorations.add(new dHammerSmack(x-1, y-4, dHAMMERSMACK, 0));
					break;
					
				case down:
					decorations.add(new dHammerSmack(x+8, y+28, dHAMMERSMACK, 0));
					break;
					
				case left:
					decorations.add(new dHammerSmack(x-13, y+14, dHAMMERSMACK, 0));
					break;
					
				case right:
					decorations.add(new dHammerSmack(x+21, y+14, dHAMMERSMACK, 0));
					break;
				}
		*/
			}
			
			return false;
		}
		else if(attackclk==15)
		{
			// Hammer's reach needs adjusted slightly for backward compatibility
			if(w->dir==up)
				w->hyofs-=1;
			else if(w->dir==left)
				w->hxofs-=2;
		}
	}
	
	// The return of Spaghetti Code Constants!
	int32_t itype = (attack==wWand ? itype_wand : attack==wSword ? itype_sword : attack==wCByrna ? itype_cbyrna : attack==wBugNet ? itype_bugnet : itype_hammer);
	int32_t itemid = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
	itemid = vbound(itemid, 0, MAXITEMS-1);
	
	// The sword offsets aren't based on anything other than what felt about right
	// compared to the NES game and what mostly kept it from hitting things that
	// should clearly be out of range. They could probably still use more tweaking.
	// Don't use 2.10 for reference; it's pretty far off.
	// - Saf
	
	if(game->get_canslash() && (attack==wSword || attack==wWand) && itemsbuf[itemid].flags & ITEM_FLAG4)
	{
		switch(w->dir)
		{
		case up:
			if(attackclk<8)
			{
				wy-=4;
			}
			
			break;
			
		case down:
			//if(attackclk<8)
		{
			wy-=2;
		}
		break;
		
		case left:
		
			//if(attackclk<8)
		{
			wx+=2;
		}
		
		break;
		
		case right:
		
			//if(attackclk<8)
		{
			wx-=3;
			//wy+=((spins>0 || get_bit(quest_rules, qr_SLASHFLIPFIX)) ? -4 : 4);
		}
		
		break;
		}
	}
	
	switch(w->dir)
	{
		case up:
			wx+=2;
			break;
			
		case down:
			break;
			
		case left:
			wy-=3;
			break;
			
		case right:
			wy-=3;
			break;
	}
	
	wx+=w->hxofs;
	wy+=w->hyofs;
	
	for(int32_t i=0; i<guys.Count(); i++)
	{
		if(attack==wBugNet) break;
		// So that Link can actually hit peahats while jumping, his weapons' hzsz becomes 16 in midair.
		if((guys.spr(i)->hit(wx,wy,wz,wxsz,wysz,wz>0?16:8) && ((attack!=wWand && attack!=wHammer && attack!=wCByrna) || !(itemsbuf[itemid].flags & ITEM_FLAG3)))
				|| ((attack==wWand || attack==wCByrna) && guys.spr(i)->hit(wx,wy-8,z,16,24,z>8) && !(itemsbuf[itemid].flags & ITEM_FLAG3))
				|| (attack==wHammer && guys.spr(i)->hit(wx,wy-8,z,16,24,z>0?16:8) && !(itemsbuf[itemid].flags & ITEM_FLAG3)))
		{
			// Checking the whimsical ring for every collision check causes
			// an odd bug. It's much more likely to activate on a 0-damage
			// weapon, since a 0-damage hit won't make the enemy invulnerable
			// to damaging hits in the following frames.
			
			int32_t whimsyid = current_item_id(itype_whimsicalring);
			
			int32_t dmg = weaponattackpower();
			if(whimsyid>-1)
			{
				if(!(zc_oldrand()%zc_max(itemsbuf[whimsyid].misc1,1)))
					dmg += current_item_power(itype_whimsicalring);
				else whimsyid = -1;
			}
			int32_t atkringid = current_item_id(itype_atkring);
			if(atkringid>-1)
			{
				dmg *= itemsbuf[atkringid].misc2; //Multiplier
				dmg += itemsbuf[atkringid].misc1; //Additive
			}
			
			int32_t h = hit_enemy(i,attack,dmg*game->get_hero_dmgmult(),wx,wy,dir,directWpn);
			enemy *e = (enemy*)guys.spr(i);
			if (h == -1) { e->hitby[HIT_BY_LWEAPON] = melee_weapon_index; } //temp_hit = true; }
			//melee weapons and non-melee weapons both writing to this index may be a problem. It needs to be cleared by something earlier than this check.
			
			if(h<0 && whimsyid>-1)
			{
				sfx(itemsbuf[whimsyid].usesound);
			}
			
			if(h && charging>0)
			{
				attackclk = SWORDTAPFRAME;
				spins=0;
			}
			
			if(h && hclk==0 && inlikelike != 1 && !get_bit(quest_rules, qr_DYING_ENEMIES_IGNORE_STUN))
			{
				if(GuyHit(i,x+7,y+7,z,2,2,hzsz)!=-1)
				{
					hitlink(i);
				}
			}
			
			if(h==2)
				break;
		}
	}
		
	if(attack == wBugNet
		|| (parentitem==-1&&!get_bit(quest_rules,qr_NOITEMMELEE))
		|| (parentitem>-1&&!(itemsbuf[parentitem].flags & ITEM_FLAG7)))
	{
		int32_t bugnetid = attack != wBugNet ? -1 : (parentitem > -1 ? parentitem : current_item_id(itype_bugnet));
		for(int32_t j=0; j<items.Count(); j++)
		{
			item* ptr = (item*)items.spr(j);
			bool dofairy = (attack==wBugNet && itemsbuf[ptr->id].family == itype_fairy)
				&& (bugnetid > -1 && !(itemsbuf[bugnetid].flags & ITEM_FLAG1));
			
			if((itemsbuf[ptr->id].family == itype_bottlefill || dofairy) && !game->canFillBottle())
				continue; //No picking these up unless you have a bottle to fill!
			if((ptr->pickup & ipCANGRAB) || (ptr->pickup & ipTIMER) || dofairy)
			{
				if(((ptr->pickup & ipCANGRAB) || ptr->clk2 >= 32 || dofairy) && !ptr->fallclk && !ptr->drownclk)
				{
					if(ptr->hit(wx,wy,z,wxsz,wysz,1) || (attack==wWand && ptr->hit(x,y-8,z,wxsz,wysz,1))
							|| (attack==wHammer && ptr->hit(x,y-8,z,wxsz,wysz,1)))
					{
						int32_t pickup = ptr->pickup;
						
						if(pickup&ipONETIME) // set mITEM for one-time-only items
							setmapflag(mITEM);
						else if(pickup&ipONETIME2) // set mBELOW flag for other one-time-only items
							setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
						
						if(pickup&ipSECRETS)								// Trigger secrets if this item has the secret pickup
						{
							if(tmpscr->flags9&fITEMSECRETPERM) setmapflag(mSECRET);
							hidden_entrance(0, true, false, -5);
						}
						//!DIMI
						
						if(dofairy)
						{
							game->fillBottle(itemsbuf[ptr->id].misc4);
						}
						else
						{
							if(itemsbuf[ptr->id].collect_script)
							{
								//clear item script stack. 
								//ri = &(itemScriptData[ptr->id]);
								//ri->Clear();
								//itemCollectScriptData[ptr->id].Clear();
								//for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[ptr->id][q] = 0;
								ri = &(itemCollectScriptData[ptr->id]);
								for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[ptr->id][q] = 0xFFFF;
								ri->Clear();
								//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[ptr->id].collect_script, ((ptr->id & 0xFFF)*-1));
								
								if ( ptr->id > 0 && !item_collect_doscript[ptr->id] ) //No collect script on item 0. 
								{
									item_collect_doscript[ptr->id] = 1;
									itemscriptInitialised[ptr->id] = 0;
									ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[ptr->id].collect_script, ((ptr->id)*-1));
									//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
										FFCore.deallocateAllArrays(SCRIPT_ITEM,-(ptr->id));
								}
								else if (ptr->id == 0 && !item_collect_doscript[ptr->id]) //item 0
								{
									item_collect_doscript[ptr->id] = 1;
									itemscriptInitialised[ptr->id] = 0;
									ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[ptr->id].collect_script, COLLECT_SCRIPT_ITEM_ZERO);
									//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
										FFCore.deallocateAllArrays(SCRIPT_ITEM,COLLECT_SCRIPT_ITEM_ZERO);
								}
					
								//runningItemScripts[ptr->id] = 0;
					
							}
				
							//Passive item scripts on colelction
							if(itemsbuf[ptr->id].script && ( (itemsbuf[ptr->id].flags&ITEM_PASSIVESCRIPT) && (get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING)) ))
							{
								ri = &(itemScriptData[ptr->id]);
								for ( int32_t q = 0; q < 1024; q++ ) item_stack[ptr->id][q] = 0xFFFF;
								ri->Clear();
								//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid & 0xFFF);
								item_doscript[ptr->id] = 1;
								itemscriptInitialised[ptr->id] = 0;
								//Z_scripterrlog("Link.cpp starting a passive item script.\n");
								ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[ptr->id].script, ptr->id);
											
							}
				
							getitem(ptr->id);
						}
						items.del(j);
						
						for(int32_t i=0; i<Lwpns.Count(); i++)
						{
							weapon *w2 = (weapon*)Lwpns.spr(i);
							
							if(w2->dragging==j)
							{
								w2->dragging=-1;
							}
							else if(w2->dragging>j)
							{
								w2->dragging-=1;
							}
						}
						
						--j;
					}
				}
			}
		}
	}
	
	if(attack==wCByrna || attack==wBugNet)
		return false;
	
	if(attack==wSword)
	{
		if(attackclk == 6)
		{
			for(int32_t q=0; q<176; q++)
			{
				set_bit(screengrid,q,0);
				set_bit(screengrid_layer[0],q,0);
				set_bit(screengrid_layer[1],q,0);
			
			}
			
			for(int32_t q=0; q<32; q++)
				set_bit(ffcgrid, q, 0);
		}
		
		if(dir==up && ((x.getInt()&15)==0))
		{
			check_slash_block(wx,wy);
			check_slash_block(wx,wy+8);
		
		//layers
		check_slash_block_layer(wx,wy,1);
		check_slash_block_layer(wx,wy+8,1);
		check_slash_block_layer(wx,wy,1);
		check_slash_block_layer(wx,wy+8,1);
		//2
		check_slash_block_layer(wx,wy,2);
		check_slash_block_layer(wx,wy+8,2);
		check_slash_block_layer(wx,wy,2);
		check_slash_block_layer(wx,wy+8,2);
		
		
		
		}
		else if(dir==up && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_slash_block(wx,wy);
			check_slash_block(wx,wy+8);
			check_slash_block(wx+8,wy);
			check_slash_block(wx+8,wy+8);
		///layer 1
		check_slash_block_layer(wx,wy,1);
			check_slash_block_layer(wx,wy+8,1);
			check_slash_block_layer(wx+8,wy,1);
			check_slash_block_layer(wx+8,wy+8,1);
		///layer 2
		check_slash_block_layer(wx,wy,2);
			check_slash_block_layer(wx,wy+8,2);
			check_slash_block_layer(wx+8,wy,2);
			check_slash_block_layer(wx+8,wy+8,2);
		}
		
		if(dir==down && ((x.getInt()&15)==0))
		{
			check_slash_block(wx,wy+wysz-8);
			check_slash_block(wx,wy+wysz);
		
		//layer 1
		check_slash_block_layer(wx,wy+wysz-8,1);
			check_slash_block_layer(wx,wy+wysz,1);
		//layer 2
		check_slash_block_layer(wx,wy+wysz-8,2);
			check_slash_block_layer(wx,wy+wysz,2);
		}
		else if(dir==down && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_slash_block(wx,wy+wysz-8);
			check_slash_block(wx,wy+wysz);
			check_slash_block(wx+8,wy+wysz-8);
			check_slash_block(wx+8,wy+wysz);
		//layer 1
		check_slash_block_layer(wx,wy+wysz-8,1);
			check_slash_block_layer(wx,wy+wysz,1);
			check_slash_block_layer(wx+8,wy+wysz-8,1);
			check_slash_block_layer(wx+8,wy+wysz,1);
		//layer 2
		check_slash_block_layer(wx,wy+wysz-8,2);
			check_slash_block_layer(wx,wy+wysz,2);
			check_slash_block_layer(wx+8,wy+wysz-8,2);
			check_slash_block_layer(wx+8,wy+wysz,2);
		}
		
		if(dir==left)
		{
			check_slash_block(wx,wy+8);
			check_slash_block(wx+8,wy+8);
		//layer 1
		check_slash_block_layer(wx,wy+8,1);
			check_slash_block_layer(wx+8,wy+8,1);
		//layer 2
		check_slash_block_layer(wx,wy+8,2);
			check_slash_block_layer(wx+8,wy+8,2);
		}
		
		if(dir==right)
		{
			check_slash_block(wx+wxsz,wy+8);
			check_slash_block(wx+wxsz-8,wy+8);
		//layer 1
		check_slash_block_layer(wx+wxsz,wy+8,1);
			check_slash_block_layer(wx+wxsz-8,wy+8,1);
		//layer 2
		check_slash_block_layer(wx+wxsz,wy+8,2);
			check_slash_block_layer(wx+wxsz-8,wy+8,2);
		}
	}
	else if(attack==wWand)
	{
		if(attackclk == 5)
		{
			for(int32_t q=0; q<176; q++)
			{
				set_bit(screengrid,q,0);
				set_bit(screengrid_layer[0],q,0);
				set_bit(screengrid_layer[1],q,0);
			}
			
			for(int32_t q=0; q<32; q++)
				set_bit(ffcgrid,q, 0);
		}
		
		// cutable blocks
		if(dir==up && (x.getInt()&15)==0)
		{
			check_wand_block(wx,wy);
			check_wand_block(wx,wy+8);
		}
		else if(dir==up && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_wand_block(wx,wy);
			check_wand_block(wx,wy+8);
			check_wand_block(wx+8,wy);
			check_wand_block(wx+8,wy+8);
		}
		
		if(dir==down && (x.getInt()&15)==0)
		{
			check_wand_block(wx,wy+wysz-8);
			check_wand_block(wx,wy+wysz);
		}
		else if(dir==down && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_wand_block(wx,wy+wysz-8);
			check_wand_block(wx,wy+wysz);
			check_wand_block(wx+8,wy+wysz-8);
			check_wand_block(wx+8,wy+wysz);
		}
		
		if(dir==left)
		{
			check_wand_block(wx,y+8);
			check_wand_block(wx+8,y+8);
		}
		
		if(dir==right)
		{
			check_wand_block(wx+wxsz,y+8);
			check_wand_block(wx+wxsz-8,y+8);
		}
	}
	else if((attack==wHammer) && ((attackclk==15) || ( spins==1 && attackclk >=15 ))) //quake hammer should be spins == 1
	//else if((attack==wHammer) && (attackclk==15))
	//reverting this, because it breaks multiple-hit pegs
	//else if((attack==wHammer) && (attackclk>=15)) //>= instead of == for time it takes to charge up hammer with quake scrolls.
	{
		// poundable blocks
		for(int32_t q=0; q<176; q++)
		{
			set_bit(screengrid,q,0);
				set_bit(screengrid_layer[0],q,0);
				set_bit(screengrid_layer[1],q,0);
		}
		
		for(int32_t q=0; q<32; q++)
			set_bit(ffcgrid, q, 0);
			
		if(dir==up && (x.getInt()&15)==0)
		{
			check_pound_block(wx,wy);
			check_pound_block(wx,wy+8);
		}
		else if(dir==up && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_pound_block(wx,wy);
			check_pound_block(wx,wy+8);
			check_pound_block(wx+8,wy);
			check_pound_block(wx+8,wy+8);
		}
		
		if(dir==down && (x.getInt()&15)==0)
		{
			check_pound_block(wx,wy+wysz-8);
			check_pound_block(wx,wy+wysz);
		}
		else if(dir==down && ((x.getInt()&15)==8||diagonalMovement||NO_GRIDLOCK))
		{
			check_pound_block(wx,wy+wysz-8);
			check_pound_block(wx,wy+wysz);
			check_pound_block(wx+8,wy+wysz-8);
			check_pound_block(wx+8,wy+wysz);
		}
		
		if(dir==left)
		{
			check_pound_block(wx,y+8);
			check_pound_block(wx+8,y+8);
		}
		
		if(dir==right)
		{
			check_pound_block(wx+wxsz,y+8);
			check_pound_block(wx+wxsz-8,y+8);
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

void LinkClass::check_slash_block_layer(int32_t bx, int32_t by, int32_t layer)
{
	
    if(!(get_bit(quest_rules,qr_BUSHESONLAYERS1AND2))) 
    {
	    //zprint("bit off\n");
	    return;
    }
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    //first things first
    if(attack!=wSword)
        return;
        
    if(z>8 || attackclk==SWORDCHARGEFRAME  // is not charging>0, as tapping a wall reduces attackclk but retains charging
            || (attackclk>SWORDTAPFRAME && tapping))
        return;
        
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
   
    int32_t flag = MAPFLAGL(layer,bx,by);
    int32_t flag2 = MAPCOMBOFLAGL(layer,bx,by);
    int32_t cid = MAPCOMBOL(layer,bx,by);
    int32_t type = combobuf[cid].type;
    //zprint("cid is: %d\n", cid);
     //zprint("type is: %d\n", type);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    
    if((get_bit(screengrid_layer[layer-1], i) != 0) || (!isCuttableType(type)))
    {
	return;
        //ignorescreen = true;
	//zprint("ignoring\n");
    }
    
    int32_t sworditem = (directWpn>-1 && itemsbuf[directWpn].family==itype_sword) ? itemsbuf[directWpn].fam_type : current_item(itype_sword);
    if(!ignorescreen)
    {
	    if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(screengrid_layer[layer-1],i,1);
            if(isCuttableNextType(type) || isCuttableNextType(type))
            {
                FFCore.tempScreens[layer]->data[i]++;
            }
            else
            {
                FFCore.tempScreens[layer]->data[i] = tmpscr->undercombo;
                FFCore.tempScreens[layer]->cset[i] = tmpscr->undercset;
                FFCore.tempScreens[layer]->sflag[i] = 0;
            }
	if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        else if(isCuttableItemType(type))
        {
            int32_t it = -1;
		
		//select_dropitem( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) ? (combobuf[MAPCOMBO(bx,by)-1].attributes[1])/10000L : 12, bx, by);
		if ( (combobuf[cid].usrflags&cflag2) )
		{
		
			it = (combobuf[cid].usrflags&cflag11) ? combobuf[cid].attribytes[1] : select_dropitem(combobuf[cid].attribytes[1]); 
			
		}
            if(it!=-1)
            {
                items.add(new item((zfix)bx, (zfix)by,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
            }
        }
        
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,tmpscr->data[i],tmpscr->cset[i]);
        
        //if(isCuttableType(type) || isGenericType(type))
        //{
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		//if ( isGenericType(type) )
		if (!isBushType(type) && !isFlowersType(type) && !isGrassType(type))
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
		else
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
			else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
		}
            }
            
            if(isBushType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        //}
            
    }
    
}




void LinkClass::check_slash_block(int32_t bx, int32_t by)
{
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    //first things first
    if(attack!=wSword)
        return;
        
    if(z>8 || attackclk==SWORDCHARGEFRAME  // is not charging>0, as tapping a wall reduces attackclk but retains charging
            || (attackclk>SWORDTAPFRAME && tapping))
        return;
        
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    int32_t cid = MAPCOMBO(bx,by);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    
    if(get_bit(screengrid, i) != 0)
    {
        ignorescreen = true;
    }
    
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
    {
        ignoreffc = true;
    }
    
    if(!isCuttableType(type) &&
            (flag<mfSWORD || flag>mfXSWORD) &&  flag!=mfSTRIKE && (flag2<mfSWORD || flag2>mfXSWORD) && flag2!=mfSTRIKE)
    {
        ignorescreen = true;
    }
    
    if(!isCuttableType(type2) &&
            (flag3<mfSWORD || flag3>mfXSWORD) && flag3!=mfSTRIKE)
    {
        ignoreffc = true;
    }
    
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    int32_t sworditem = (directWpn>-1 && itemsbuf[directWpn].family==itype_sword) ? itemsbuf[directWpn].fam_type : current_item(itype_sword);
    byte skipsecrets = 0;
    
    if ( isNextType(type) ) //->Next combos should not trigger secrets. Their child combo, may want to do that! -Z 17th December, 2019
    {
		if (get_bit(quest_rules,qr_IDIOTICSHASHNEXTSECRETBUGSUPPORT)) //Haha wow Zoria you really named it that huh -Dimi
		{
			skipsecrets = 0;
		}
		else skipsecrets = 1; ;
    }
    
    if(!ignorescreen && !skipsecrets)
    {
	
	
        if((flag >= 16)&&(flag <= 31))
        {  
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag>=mfSWORD&&flag<=mfXSWORD)||(flag==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(((flag2 >= 16)&&(flag2 <= 31)))
        { 
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag2>=mfSWORD&&flag2<=mfXSWORD)||(flag2==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else
        {
            if(isCuttableNextType(type))
            {
                s->data[i]++;
            }
            else
            {
                s->data[i] = s->undercombo;
                s->cset[i] = s->undercset;
                s->sflag[i] = 0;
            }
            
            //pausenow=true;
        }
    }
    else if(!ignorescreen && skipsecrets)
    {
	if(isCuttableNextType(type))
            {
                s->data[i]++;
            }
            else
            {
                s->data[i] = s->undercombo;
                s->cset[i] = s->undercset;
                s->sflag[i] = 0;
            }
	    
    }
    
    if(((flag3>=mfSWORD&&flag3<=mfXSWORD)||(flag3==mfSTRIKE)) && !ignoreffc)
    {
        for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
        {
            findentrance(bx,by,mfSWORD+i2,true);
        }
        
        findentrance(fx,fy,mfSTRIKE,true);
    }
    else if(!ignoreffc)
    {
        if(isCuttableNextType(type2))
        {
            s->ffdata[current_ffcombo]++;
        }
        else
        {
            s->ffdata[current_ffcombo] = s->undercombo;
            s->ffcset[current_ffcombo] = s->undercset;
        }
    }
    
    if(!ignorescreen)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(screengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        else if(isCuttableItemType(type))
        {
		int32_t it = -1;
		//zprint("reached iscuttableitem, with cid: %d\n", cid);
		//select_dropitem( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) ? (combobuf[MAPCOMBO(bx,by)-1].attributes[1] / 10000L) : 12, bx, by);
		if ( (combobuf[cid].usrflags&cflag2) ) //specific dropset or item
		{
			//zprint("Custom itemset: %d\n", combobuf[cid].attribytes[1]);
			if ( combobuf[cid].usrflags&cflag11 ) 
			{
				//zprint("specific item %d\n", combobuf[cid].attribytes[1]);
				it = combobuf[cid].attribytes[1];
			}
			else
			{
				//zprint("specific dropset %d\n", combobuf[cid].attribytes[1]);
				it = select_dropitem(combobuf[cid].attribytes[1]); 
				
				
			}
			//it = (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag11) ? combobuf[MAPCOMBO(bx,by)-1].attribytes[1] : select_dropitem(combobuf[MAPCOMBO(bx,by)-1].attribytes[1]); 
			
		}
		//old style slash item and tall grass
		else if ( !(combobuf[cid].usrflags&cflag2) )
		{
			//zprint("Standard tall grass drop.\n");
			it = select_dropitem(12, bx, by);
		}
		if(it!=-1)
		{
			items.add(new item((zfix)bx, (zfix)by,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
		}
        }
        
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
        
        //if(isBushType(type) || isFlowersType(type) || isGrassType(type) || isGenericType(type))
        //{
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if(!(isBushType(type) || isFlowersType(type) || isGrassType(type)))
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
		else
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
			else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
		}
            }
            
            if(isBushType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        //}
    }
    
    if(!ignoreffc)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(ffcgrid, current_ffcombo, 1);
        
        if(isCuttableItemType(type2))
        {
            int32_t it=-1;
		if ( (combobuf[cid].usrflags&cflag2) )
		{
		
			it = (combobuf[cid].usrflags&cflag11) ? combobuf[cid].attribytes[1] : select_dropitem(combobuf[cid].attribytes[1]); 
			
		}
		
		else
		{
			int32_t r=zc_oldrand()%100;
            
			if(r<15)
			{
				it=iHeart;                                // 15%
			}
			else if(r<35)
			{
				it=iRupy;                                 // 20%
			}
		}
            
            if(it!=-1 && itemsbuf[it].family != itype_misc) // Don't drop non-gameplay items
            {
                items.add(new item((zfix)fx, (zfix)fy,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
            }
        }
        
        if(isBushType(type2) || isFlowersType(type2) || isGrassType(type2) || isGenericType(type2))
        {
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if ( isGenericType(type) )
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
                else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
            }
            
            if(isBushType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else if (isGenericType(type2))
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        }
    }
}

void LinkClass::check_wpn_triggers(int32_t bx, int32_t by, weapon *w)
{
	/*
	int32_t par_item = w->parentitem;
	al_trace("check_wpn_triggers(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_wpn_triggers(weapon *w): usewpn is: %d\n", usewpn);
	
	*/
	bx=vbound(bx, 0, 255);
	by=vbound(by, 0, 176);
	int32_t cid = MAPCOMBO(bx,by);
	switch(w->useweapon)
	{
		case wArrow:
			findentrance(bx,by,mfARROW,true);
			findentrance(bx,by,mfSARROW,true);
			findentrance(bx,by,mfGARROW,true);
		case wBeam:
			for(int32_t i = 0; i <4; i++) findentrance(bx,by,mfSWORDBEAM+i,true);
			break;
		case wHookshot:
			findentrance(bx,by,mfHOOKSHOT,true);
			break;
		case wBrang:
			for(int32_t i = 0; i <3; i++) findentrance(bx,by,mfBRANG+i,true);
			break;
		case wMagic:
			findentrance(bx,by,mfWANDMAGIC,true);
			break;
		case wRefMagic:
			findentrance(bx,by,mfWANDMAGIC,true);
			break;
		case wRefBeam:
			for(int32_t i = 0; i <4; i++) findentrance(bx,by,mfSWORDBEAM+i,true);
			break;
		//reflected magic needs to happen in mirrors:
		//
		//findentrance(bx,by,mfREFMAGIC,true)
		case wRefFireball:
			findentrance(bx,by,mfREFFIREBALL,true);
			break;
		case wBomb:
			findentrance(bx+w->txsz,by+tysz+(isSideViewGravity()?2:-3),mfBOMB,true);
			break;
		
		case wSBomb:
			findentrance(bx+w->txsz,by+tysz+(isSideViewGravity()?2:-3),mfSBOMB,true);
			break;
			
		case wFire:
			findentrance(bx,by,mfBCANDLE,true);
			findentrance(bx,by,mfRCANDLE,true);
			findentrance(bx,by,mfWANDFIRE,true);
		/* if we want the weapon to die
		if (findentrance(bx,by,mfBCANDLE,true) ) dead = 1;
			if (findentrance(bx,by,mfRCANDLE,true) ) dead = 1;
			if (findentrance(bx,by,mfWANDFIRE,true)) dead = 1;
		*/
			break;
		
		case wScript1:
			break;
		case wScript2:
			break;
		case wScript3:
			break;
		case wScript4:
			break;
		case wScript5:
			break;
		case wScript6:
			break;
		case wScript7:
			break;
		case wScript8:
			break;
		case wScript9:
			break;
		case wScript10:
			break;
		case wIce:
			break;
		case wCByrna:
			break;
		case wWhistle:
			break;
		case wSSparkle:
		case wFSparkle:
			break;
		case wWind:
			break;
		case wBait:
			break;
		case wFlame:
		case wThrowRock:
		case wBombos:
		case wEther:
		case wQuake:
		case wSwordLA:
		case wSword180:
		case wStomp:
			break;
		case wSword:
		case wWand:
		//case wCandle:
		case wHSHandle:
		case wLitBomb:
		case wLitSBomb:
			break;
		default: break;
		
	}
}

void LinkClass::check_slash_block_layer2(int32_t bx, int32_t by, weapon *w, int32_t layer)
{
	
    if(!(get_bit(quest_rules,qr_BUSHESONLAYERS1AND2))) 
    {
	    //zprint("bit off\n");
	    return;
    }
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    //first things first
    if(w->useweapon != wSword)
        return;
        
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
   
    int32_t flag = MAPFLAGL(layer,bx,by);
    int32_t flag2 = MAPCOMBOFLAGL(layer,bx,by);
    int32_t cid = MAPCOMBOL(layer,bx,by);
    int32_t type = combobuf[cid].type;
    //zprint("cid is: %d\n", cid);
     //zprint("type is: %d\n", type);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
        return;
    
    if((get_bit(w->wscreengrid_layer[layer-1], i) != 0) || (!isCuttableType(type)))
    {
	return; 
        //ignorescreen = true;
	//zprint("ignoring\n");
    }
    
    int32_t sworditem = (directWpn>-1 && itemsbuf[directWpn].family==itype_sword) ? itemsbuf[directWpn].fam_type : current_item(itype_sword);
    
    {
	    if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(w->wscreengrid_layer[layer-1],i,1);
            if(isCuttableNextType(type) || isCuttableNextType(type))
            {
                FFCore.tempScreens[layer]->data[i]++;
            }
            else
            {
                FFCore.tempScreens[layer]->data[i] = tmpscr->undercombo;
                FFCore.tempScreens[layer]->cset[i] = tmpscr->undercset;
                FFCore.tempScreens[layer]->sflag[i] = 0;
            }
	if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        else if(isCuttableItemType(type))
        {
            int32_t it = -1;
		
		//select_dropitem( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) ? (combobuf[MAPCOMBO(bx,by)-1].attributes[1] / 10000L) : 12, bx, by);
		if ( (combobuf[cid].usrflags&cflag2) )
		{
		
			it = (combobuf[cid].usrflags&cflag11) ? combobuf[cid].attribytes[1] : select_dropitem(combobuf[cid].attribytes[1]); 
			
		}
            if(it!=-1)
            {
                items.add(new item((zfix)bx, (zfix)by,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
            }
        }
        
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,tmpscr->data[i],tmpscr->cset[i]);
        
        if(isBushType(type) || isFlowersType(type) || isGrassType(type) || isGenericType(type))
        {
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if ( isGenericType(type) )
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
		else
			sfx(WAV_ZN1GRASSCUT,int32_t(bx));
            }
            
            if(isBushType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else if (isGenericType(type))
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        }
            
    }
    
}






void LinkClass::check_slash_block2(int32_t bx, int32_t by, weapon *w)
{
	/*
	int32_t par_item = w->parentitem;
	al_trace("check_slash_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_slash_block(weapon *w): usewpn is: %d\n", usewpn);
	*/
    
	
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
        
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    byte dontignore = 0;
    byte dontignoreffc = 0;
    
	    if (isCuttableType(type) && MatchComboTrigger(w, combobuf, cid))
	    {
		al_trace("This weapon (%d) can slash the combo: combobuf[%d].\n", w->id, cid);
		dontignore = 1;
	    }
    
	    /*to-do, ffcs
	    if (isCuttableType(type2) && MatchComboTrigger(w, combobuf, cid))
	    {
		al_trace("This weapon (%d) can slash the combo: combobuf[%d].\n", w->id, cid);
		dontignoreffc = 1;
	    }*/
	if(w->useweapon != wSword && !dontignore) return;

    
    int32_t i = (bx>>4) + by;
	    if (get_bit(w->wscreengrid,(((bx>>4) + by))) ) return;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    
    if(get_bit(w->wscreengrid, i) != 0)
    {
        ignorescreen = true; dontignore = 0;
    }
    
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
    {
        ignoreffc = true;
    }
    
    if(!isCuttableType(type) &&
            (flag<mfSWORD || flag>mfXSWORD) &&  flag!=mfSTRIKE && (flag2<mfSWORD || flag2>mfXSWORD) && flag2!=mfSTRIKE)
    {
        ignorescreen = true;
    }
    
    if(!isCuttableType(type2) &&
            (flag3<mfSWORD || flag3>mfXSWORD) && flag3!=mfSTRIKE)
    {
        ignoreffc = true;
    }
    
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    int32_t sworditem = (directWpn>-1 && itemsbuf[directWpn].family==itype_sword) ? itemsbuf[directWpn].fam_type : current_item(itype_sword);
    byte skipsecrets = 0;
    if ( isNextType(type) ) //->Next combos should not trigger secrets. Their child combo, may want to do that! -Z 17th December, 2019
    {
		if (get_bit(quest_rules,qr_IDIOTICSHASHNEXTSECRETBUGSUPPORT))
		{
			skipsecrets = 0;
		}
		else skipsecrets = 1; 
    }
    if(!skipsecrets && (!ignorescreen || dontignore))
    {
        if((flag >= 16)&&(flag <= 31))
        { 
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag>=mfSWORD&&flag<=mfXSWORD)||(flag==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(((flag2 >= 16)&&(flag2 <= 31)))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag2>=mfSWORD&&flag2<=mfXSWORD)||(flag2==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else
        {
            if(isCuttableNextType(type))
            {
                s->data[i]++;
            }
            else
            {
                s->data[i] = s->undercombo;
                s->cset[i] = s->undercset;
                s->sflag[i] = 0;
            }
            
            //pausenow=true;
        }
    }
    else if(skipsecrets && (!ignorescreen || dontignore))
    {
	    if(isCuttableNextType(type))
            {
                s->data[i]++;
            }
            else
            {
                s->data[i] = s->undercombo;
                s->cset[i] = s->undercset;
                s->sflag[i] = 0;
            }
    }
    
    if(((flag3>=mfSWORD&&flag3<=mfXSWORD)||(flag3==mfSTRIKE)) && !ignoreffc)
    {
        for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
        {
            findentrance(bx,by,mfSWORD+i2,true);
        }
        
        findentrance(fx,fy,mfSTRIKE,true);
    }
    else if(!ignoreffc)
    {
        if(isCuttableNextType(type2))
        {
            s->ffdata[current_ffcombo]++;
        }
        else
        {
            s->ffdata[current_ffcombo] = s->undercombo;
            s->ffcset[current_ffcombo] = s->undercset;
        }
    }
    
    if(!ignorescreen || dontignore)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(w->wscreengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
	else if(isCuttableItemType(type))
        {
		int32_t it = -1;
		//zprint("reached iscuttableitem, with cid: %d\n", cid);
		//select_dropitem( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) ? (combobuf[MAPCOMBO(bx,by)-1].attributes[1] / 10000L) : 12, bx, by);
		if ( (combobuf[cid].usrflags&cflag2) ) //specific dropset or item
		{
			//zprint("Custom itemset: %d\n", combobuf[cid].attribytes[1]);
			if ( combobuf[cid].usrflags&cflag11 ) 
			{
				//zprint("specific item %d\n", combobuf[cid].attribytes[1]);
				it = combobuf[cid].attribytes[1];
			}
			else
			{
				//zprint("specific dropset %d\n", combobuf[cid].attribytes[1]);
				it = select_dropitem(combobuf[cid].attribytes[1]); 
				
				
			}
			//it = (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag11) ? combobuf[MAPCOMBO(bx,by)-1].attribytes[1] : select_dropitem(combobuf[MAPCOMBO(bx,by)-1].attribytes[1]); 
			
		}
		//old style slash item and tall grass
		else if ( !(combobuf[cid].usrflags&cflag2) )
		{
			//zprint("Standard tall grass drop.\n");
			it = select_dropitem(12, bx, by);
		}
		if(it!=-1)
		{
			items.add(new item((zfix)bx, (zfix)by,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
		}
        }
        
        
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
        
        if(isBushType(type) || isFlowersType(type) || isGrassType(type) || isGenericType(type))
        {
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if ( isGenericType(type) )
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
                else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
            }
            
            if(isBushType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else if (isGenericType(type))
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        }
    }
    
    if(!ignoreffc)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(ffcgrid, current_ffcombo, 1);
        
        if(isCuttableItemType(type2))
        {
            int32_t it=-1;
		if ( (combobuf[cid].usrflags&cflag2) )
		{
		
			it = (combobuf[cid].usrflags&cflag11) ? combobuf[cid].attribytes[1] : select_dropitem(combobuf[cid].attribytes[1]); 
			
		}
           
		
		else
		{
			int32_t r=zc_oldrand()%100;
            
			if(r<15)
			{
				it=iHeart;                                // 15%
			}
			else if(r<35)
			{
				it=iRupy;                                 // 20%
			}
		}
            
            if(it!=-1 && itemsbuf[it].family != itype_misc) // Don't drop non-gameplay items
            {
                items.add(new item((zfix)fx, (zfix)fy,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
            }
        }
        
        //if(isBushType(type2) || isFlowersType(type2) || isGrassType(type2) || isGenericType(type2))
        //{
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if(!(isBushType(type2) || isFlowersType(type2) || isGrassType(type2)))
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
		}
                else 
		{
			if (combobuf[cid].usrflags&cflag3)
			{
				sfx(combobuf[cid].attribytes[2],int32_t(bx));
			}
			else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
		}
            }
            
            if(isBushType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type2))
            {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else if (isGenericType(type2))
	    {
		if ( combobuf[cid].usrflags&cflag1 )
		{
			if ( combobuf[cid].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[cid].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[cid].attribytes[0]));
		}
            }
        //}
    }
}


void LinkClass::check_wand_block2(int32_t bx, int32_t by, weapon *w)
{
	/*
	int32_t par_item = w->parentitem;
	al_trace("check_wand_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_wand_block(weapon *w): usewpn is: %d\n", usewpn);
	*/
	
	byte dontignore = 0;
	byte dontignoreffc = 0;
    
	
	
    

    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
   
    //Z_scripterrlog("check_wand_block2 MatchComboTrigger() returned: %d\n", );
    if(w->useweapon != wWand && !MatchComboTrigger (w, combobuf, cid)) return;
    if ( MatchComboTrigger (w, combobuf, cid) ) dontignore = 1;
    
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3=0;
    int32_t flag31 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag32 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag33 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag34 = MAPFFCOMBOFLAG(fx,fy);
    
    if(flag31==mfWAND||flag32==mfWAND||flag33==mfWAND||flag34==mfWAND)
        flag3=mfWAND;
        
    if(flag31==mfSTRIKE||flag32==mfSTRIKE||flag33==mfSTRIKE||flag34==mfSTRIKE)
        flag3=mfSTRIKE;
        
    int32_t i = (bx>>4) + by;
    
    if(flag!=mfWAND&&flag2!=mfWAND&&flag3!=mfWAND&&flag!=mfSTRIKE&&flag2!=mfSTRIKE&&flag3!=mfSTRIKE)
        return;
        
    if(i > 175)
        return;
        
    //mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    //findentrance(bx,by,mfWAND,true);
    //findentrance(bx,by,mfSTRIKE,true);
    if((findentrance(bx,by,mfWAND,true)==false)&&(findentrance(bx,by,mfSTRIKE,true)==false))
    {
        if(flag3==mfWAND||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfWAND,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
    }
    
    if(dontignore) { findentrance(bx,by,mfWAND,true); }
    
    //putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
}

void LinkClass::check_pound_block2(int32_t bx, int32_t by, weapon *w)
{
	/*
	int32_t par_item = w->parentitem;
	al_trace("check_pound_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_pound_block(weapon *w): usewpn is: %d\n", usewpn);
	*/
	//keep things inside the screen boundaries
	bx=vbound(bx, 0, 255);
	by=vbound(by, 0, 176);
	int32_t fx=vbound(bx, 0, 255);
	int32_t fy=vbound(by, 0, 176);
	int32_t cid = MAPCOMBO(bx,by);
	byte dontignore = MatchComboTrigger (w, combobuf, cid);
	if(w->useweapon != wHammer && !dontignore ) return;
	

	
    
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    int32_t i = (bx>>4) + by;
    if (get_bit(w->wscreengrid,(((bx>>4) + by))) ) return;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    bool pound=false;
    
    if(type!=cPOUND && flag!=mfHAMMER && flag!=mfSTRIKE && flag2!=mfHAMMER && flag2!=mfSTRIKE)
        ignorescreen = true; // Affect only FFCs
        
    if(get_bit(w->wscreengrid, i) != 0)
    {
        ignorescreen = true; dontignore = 0;
    }
        
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
        ignoreffc = true;
        
    if(type2!=cPOUND && flag3!=mfSTRIKE && flag3!=mfHAMMER)
        ignoreffc = true;
        
    if(ignorescreen && ignoreffc)  // Nothing to do.
        return;
        
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    if(!ignorescreen || dontignore)
    {
        if(flag==mfHAMMER||flag==mfSTRIKE)  // Takes precedence over Secret Tile and Armos->Secret
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(flag2==mfHAMMER||flag2==mfSTRIKE)
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if((flag >= 16)&&(flag <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if((flag2 >= 16)&&(flag2 <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else pound = true;
    }
    
    if(!ignoreffc)
    {
        if(flag3==mfHAMMER||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfHAMMER,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
        else
        {
            s->ffdata[current_ffcombo]+=1;
        }
    }
    
    if(!ignorescreen || dontignore)
    {
        if(pound)
            s->data[i]+=1;
            
        set_bit(w->wscreengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by, (zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        
        if(type==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
            
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
    }
    
    if(!ignoreffc)
    {
        set_bit(ffcgrid,current_ffcombo,1);
        
        if(type2==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
    }
    
    return;
}



void LinkClass::check_slash_block(weapon *w)
{
	//first things 
	
	int32_t par_item = w->parentitem;
	al_trace("check_slash_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_slash_block(weapon *w): usewpn is: %d\n", usewpn);
    if(usewpn != wSword) return;
	
	
    int32_t bx = 0, by = 0;
	bx = ((int32_t)w->x) + (((int32_t)w->hxsz)/2);
	by = ((int32_t)w->y) + (((int32_t)w->hysz)/2);
	al_trace("check_slash_block(weapon *w): bx is: %d\n", bx);
	al_trace("check_slash_block(weapon *w): by is: %d\n", by);
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    
    int32_t cid = MAPCOMBO(bx,by);
        
    //if(z>8 || attackclk==SWORDCHARGEFRAME  // is not charging>0, as tapping a wall reduces attackclk but retains charging
    //        || (attackclk>SWORDTAPFRAME && tapping))
    //    return;
        
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
    {
	    al_trace("check_slash_block(weapon *w): %s\n", "i > 175");
        return;
    }
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    
    if(get_bit(screengrid, i) != 0)
    {
        ignorescreen = true;
    }
    
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
    {
        ignoreffc = true;
    }
    
    if(!isCuttableType(type) &&
            (flag<mfSWORD || flag>mfXSWORD) &&  flag!=mfSTRIKE && (flag2<mfSWORD || flag2>mfXSWORD) && flag2!=mfSTRIKE)
    {
        ignorescreen = true;
    }
    
    if(!isCuttableType(type2) &&
            (flag3<mfSWORD || flag3>mfXSWORD) && flag3!=mfSTRIKE)
    {
        ignoreffc = true;
    }
    
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    int32_t sworditem = (par_item >-1 ? itemsbuf[par_item].fam_type : current_item(itype_sword)); //Get the level of the item, else the highest sword level in inventory.
    
    if(!ignorescreen)
    {
        if((flag >= 16)&&(flag <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag>=mfSWORD&&flag<=mfXSWORD)||(flag==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(((flag2 >= 16)&&(flag2 <= 31)))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if(((flag2>=mfSWORD&&flag2<=mfXSWORD)||(flag2==mfSTRIKE)))
        {
            for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
            {
                findentrance(bx,by,mfSWORD+i2,true);
            }
            
            findentrance(bx,by,mfSTRIKE,true);
        }
        else
        {
            if(isCuttableNextType(type))
            {
                s->data[i]++;
            }
            else
            {
                s->data[i] = s->undercombo;
                s->cset[i] = s->undercset;
                s->sflag[i] = 0;
            }
            
            //pausenow=true;
        }
    }
    
    if(((flag3>=mfSWORD&&flag3<=mfXSWORD)||(flag3==mfSTRIKE)) && !ignoreffc)
    {
        for(int32_t i2=0; i2<=zc_min(sworditem-1,3); i2++)
        {
            findentrance(bx,by,mfSWORD+i2,true);
        }
        
        findentrance(fx,fy,mfSTRIKE,true);
    }
    else if(!ignoreffc)
    {
        if(isCuttableNextType(type2))
        {
            s->ffdata[current_ffcombo]++;
        }
        else
        {
            s->ffdata[current_ffcombo] = s->undercombo;
            s->ffcset[current_ffcombo] = s->undercset;
        }
    }
    
    if(!ignorescreen)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(screengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        else if(isCuttableItemType(type))
        {
		int32_t it = -1;
		//zprint("reached iscuttableitem, with cid: %d\n", cid);
		//select_dropitem( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) ? (combobuf[MAPCOMBO(bx,by)-1].attributes[1] / 10000L) : 12, bx, by);
		if ( (combobuf[cid].usrflags&cflag2) ) //specific dropset or item
		{
			//zprint("Custom itemset: %d\n", combobuf[cid].attribytes[1]);
			if ( combobuf[cid].usrflags&cflag11 ) 
			{
				//zprint("specific item %d\n", combobuf[cid].attribytes[1]);
				it = combobuf[cid].attribytes[1];
			}
			else
			{
				//zprint("specific dropset %d\n", combobuf[cid].attribytes[1]);
				it = select_dropitem(combobuf[cid].attribytes[1]); 
				
				
			}
			//it = (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag11) ? combobuf[MAPCOMBO(bx,by)-1].attribytes[1] : select_dropitem(combobuf[MAPCOMBO(bx,by)-1].attribytes[1]); 
			
		}
		//old style slash item and tall grass
		else if ( !(combobuf[cid].usrflags&cflag2) )
		{
			//zprint("Standard tall grass drop.\n");
			it = select_dropitem(12, bx, by);
		}
		if(it!=-1)
		{
			items.add(new item((zfix)bx, (zfix)by,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
		}
        }
        
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
        
        //if(isBushType(type) || isFlowersType(type) || isGrassType(type) || isGenericType(type))
        //{
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if(!(isBushType(type) || isFlowersType(type) || isGrassType(type)))
		{
			if (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag3)
			{
				sfx(combobuf[MAPCOMBO(bx,by)-1].attribytes[2],int32_t(bx));
			}
		}
                else 
		{
			if (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag3)
			{
				sfx(combobuf[MAPCOMBO(bx,by)-1].attribytes[2],int32_t(bx));
			}
			else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
		}
            }
            
            if(isBushType(type))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else
	    {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
            }
        //}
    }
    
    if(!ignoreffc)
    {
        if(!isTouchyType(type) && !get_bit(quest_rules, qr_CONT_SWORD_TRIGGERS)) set_bit(ffcgrid, current_ffcombo, 1);
        
        if(isCuttableItemType(type2))
        {
            int32_t it=-1;
		if ( (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag2) )
		{
		
			it = (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag11) ? combobuf[MAPCOMBO(bx,by)-1].attribytes[1] : select_dropitem(combobuf[MAPCOMBO(bx,by)-1].attribytes[1]); 
			
		}
		
		
		else
		{
			int32_t r=zc_oldrand()%100;
            
			if(r<15)
			{
				it=iHeart;                                // 15%
			}
			else if(r<35)
			{
				it=iRupy;                                 // 20%
			}
		}
		
		
            
            
            if(it!=-1 && itemsbuf[it].family != itype_misc) // Don't drop non-gameplay items
            {
                items.add(new item((zfix)fx, (zfix)fy,(zfix)0, it, ipBIGRANGE + ipTIMER, 0));
            }
        }
        
        //if(isBushType(type2) || isFlowersType(type2) || isGrassType(type2) || isGenericType(type2))
        //{
            if(get_bit(quest_rules,qr_MORESOUNDS))
            {
		if(!(isBushType(type2) || isFlowersType(type2) || isGrassType(type2)))
		{
			if (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag3)
			{
				sfx(combobuf[MAPCOMBO(bx,by)-1].attribytes[2],int32_t(bx));
			}
		}
                else 
		{
			if (combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag3)
			{
				sfx(combobuf[MAPCOMBO(bx,by)-1].attribytes[2],int32_t(bx));
			}
			else sfx(WAV_ZN1GRASSCUT,int32_t(bx));
		}
            }
            
            if(isBushType(type2))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
		else decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
            }
            else if(isFlowersType(type2))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
		else decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
            }
            else if(isGrassType(type2))
            {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
                else decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
            }
	    else if (isGenericType(type2))
	    {
		if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag1 )
		{
			if ( combobuf[MAPCOMBO(bx,by)-1].usrflags&cflag10 ) //select sys sprite
			{
				switch( combobuf[MAPCOMBO(bx,by)-1].attribytes[0] )
				{
					case 1: decorations.add(new dBushLeaves((zfix)fx, (zfix)fy, dBUSHLEAVES, 0, 0));
					case 2: decorations.add(new dFlowerClippings((zfix)fx, (zfix)fy, dFLOWERCLIPPINGS, 0, 0));
					case 3: decorations.add(new dGrassClippings((zfix)fx, (zfix)fy, dGRASSCLIPPINGS, 0, 0));
					default: decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
				}
			}
			else 
				decorations.add(new comboSprite((zfix)fx, (zfix)fy, 0, 0, combobuf[MAPCOMBO(bx,by)-1].attribytes[0]));
		}
            }
        //}
    }
}

//TODO: Boomerang that cuts bushes. -L
/*void LinkClass::slash_bush()
{

}*/

void LinkClass::check_wand_block(int32_t bx, int32_t by)
{
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
    
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3=0;
    int32_t flag31 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag32 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag33 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag34 = MAPFFCOMBOFLAG(fx,fy);
    
    if(flag31==mfWAND||flag32==mfWAND||flag33==mfWAND||flag34==mfWAND)
        flag3=mfWAND;
        
    if(flag31==mfSTRIKE||flag32==mfSTRIKE||flag33==mfSTRIKE||flag34==mfSTRIKE)
        flag3=mfSTRIKE;
        
    int32_t i = (bx>>4) + by;
    
    if(flag!=mfWAND&&flag2!=mfWAND&&flag3!=mfWAND&&flag!=mfSTRIKE&&flag2!=mfSTRIKE&&flag3!=mfSTRIKE)
        return;
        
    if(i > 175)
        return;
        
    //mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    //findentrance(bx,by,mfWAND,true);
    //findentrance(bx,by,mfSTRIKE,true);
    if((findentrance(bx,by,mfWAND,true)==false)&&(findentrance(bx,by,mfSTRIKE,true)==false))
    {
        if(flag3==mfWAND||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfWAND,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
    }
    
    //putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
}

void LinkClass::check_pound_block(int32_t bx, int32_t by)
{
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
    
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    bool pound=false;
    
    if(type!=cPOUND && flag!=mfHAMMER && flag!=mfSTRIKE && flag2!=mfHAMMER && flag2!=mfSTRIKE)
        ignorescreen = true; // Affect only FFCs
        
    if(get_bit(screengrid, i) != 0)
        ignorescreen = true;
        
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
        ignoreffc = true;
        
    if(type2!=cPOUND && flag3!=mfSTRIKE && flag3!=mfHAMMER)
        ignoreffc = true;
        
    if(ignorescreen && ignoreffc)  // Nothing to do.
        return;
        
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    if(!ignorescreen)
    {
        if(flag==mfHAMMER||flag==mfSTRIKE)  // Takes precedence over Secret Tile and Armos->Secret
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(flag2==mfHAMMER||flag2==mfSTRIKE)
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if((flag >= 16)&&(flag <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if((flag2 >= 16)&&(flag2 <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else pound = true;
    }
    
    if(!ignoreffc)
    {
        if(flag3==mfHAMMER||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfHAMMER,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
        else
        {
            s->ffdata[current_ffcombo]+=1;
        }
    }
    
    if(!ignorescreen)
    {
        if(pound)
            s->data[i]+=1;
            
        set_bit(screengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by, (zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        
        if(type==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
            
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
    }
    
    if(!ignoreffc)
    {
        set_bit(ffcgrid,current_ffcombo,1);
        
        if(type2==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
    }
    
    return;
}


void LinkClass::check_wand_block(weapon *w)
{
	
    int32_t par_item = w->parentitem;
	al_trace("check_wand_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_wand_block(weapon *w): usewpn is: %d\n", usewpn);
    if(usewpn != wWand) return;
	
	
    int32_t bx = 0, by = 0;
	bx = ((int32_t)w->x) + (((int32_t)w->hxsz)/2);
	by = ((int32_t)w->y) + (((int32_t)w->hysz)/2);
	
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3=0;
    int32_t flag31 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag32 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag33 = MAPFFCOMBOFLAG(fx,fy);
    int32_t flag34 = MAPFFCOMBOFLAG(fx,fy);
    
    if(flag31==mfWAND||flag32==mfWAND||flag33==mfWAND||flag34==mfWAND)
        flag3=mfWAND;
        
    if(flag31==mfSTRIKE||flag32==mfSTRIKE||flag33==mfSTRIKE||flag34==mfSTRIKE)
        flag3=mfSTRIKE;
        
    int32_t i = (bx>>4) + by;
    
    if(flag!=mfWAND&&flag2!=mfWAND&&flag3!=mfWAND&&flag!=mfSTRIKE&&flag2!=mfSTRIKE&&flag3!=mfSTRIKE)
        return;
        
    if(i > 175)
        return;
        
    //mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    //findentrance(bx,by,mfWAND,true);
    //findentrance(bx,by,mfSTRIKE,true);
    if((findentrance(bx,by,mfWAND,true)==false)&&(findentrance(bx,by,mfSTRIKE,true)==false))
    {
        if(flag3==mfWAND||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfWAND,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
    }
    
    //putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
}

void LinkClass::check_pound_block(weapon *w)
{
	
	int32_t par_item = w->parentitem;
	al_trace("check_pound_block(weapon *w): par_item is: %d\n", par_item);
	int32_t usewpn = -1;
	if ( par_item > -1 )
	{
		usewpn = itemsbuf[par_item].useweapon;
	}
	else if ( par_item == -1 && w->ScriptGenerated ) 
	{
		usewpn = w->useweapon;
	}
	al_trace("check_pound_block(weapon *w): usewpn is: %d\n", usewpn);
    if(usewpn != wHammer) return;
	
	
    int32_t bx = 0, by = 0;
	bx = ((int32_t)w->x) + (((int32_t)w->hxsz)/2);
	by = ((int32_t)w->y) + (((int32_t)w->hysz)/2);
    //keep things inside the screen boundaries
    bx=vbound(bx, 0, 255);
    by=vbound(by, 0, 176);
    int32_t fx=vbound(bx, 0, 255);
    int32_t fy=vbound(by, 0, 176);
    int32_t cid = MAPCOMBO(bx,by);
    //first things first
    if(z>8) return;
    
    //find out which combo row/column the coordinates are in
    bx &= 0xF0;
    by &= 0xF0;
    
    int32_t type = COMBOTYPE(bx,by);
    int32_t type2 = FFCOMBOTYPE(fx,fy);
    int32_t flag = MAPFLAG(bx,by);
    int32_t flag2 = MAPCOMBOFLAG(bx,by);
    int32_t flag3 = MAPFFCOMBOFLAG(fx,fy);
    int32_t i = (bx>>4) + by;
    
    if(i > 175)
        return;
        
    bool ignorescreen=false;
    bool ignoreffc=false;
    bool pound=false;
    
    if(type!=cPOUND && flag!=mfHAMMER && flag!=mfSTRIKE && flag2!=mfHAMMER && flag2!=mfSTRIKE)
        ignorescreen = true; // Affect only FFCs
        
    if(get_bit(screengrid, i) != 0)
        ignorescreen = true;
        
    int32_t current_ffcombo = getFFCAt(fx,fy);
    
    if(current_ffcombo == -1 || get_bit(ffcgrid, current_ffcombo) != 0)
        ignoreffc = true;
        
    if(type2!=cPOUND && flag3!=mfSTRIKE && flag3!=mfHAMMER)
        ignoreffc = true;
        
    if(ignorescreen && ignoreffc)  // Nothing to do.
        return;
        
    mapscr *s = tmpscr + ((currscr>=128) ? 1 : 0);
    
    if(!ignorescreen)
    {
        if(flag==mfHAMMER||flag==mfSTRIKE)  // Takes precedence over Secret Tile and Armos->Secret
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if(flag2==mfHAMMER||flag2==mfSTRIKE)
        {
            findentrance(bx,by,mfHAMMER,true);
            findentrance(bx,by,mfSTRIKE,true);
        }
        else if((flag >= 16)&&(flag <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else if((flag2 >= 16)&&(flag2 <= 31))
        {
            s->data[i] = s->secretcombo[(s->sflag[i])-16+4];
            s->cset[i] = s->secretcset[(s->sflag[i])-16+4];
            s->sflag[i] = s->secretflag[(s->sflag[i])-16+4];
        }
        else if(flag2 == mfARMOS_SECRET)
        {
            s->data[i] = s->secretcombo[sSTAIRS];
            s->cset[i] = s->secretcset[sSTAIRS];
            s->sflag[i] = s->secretflag[sSTAIRS];
            sfx(tmpscr->secretsfx);
        }
        else pound = true;
    }
    
    if(!ignoreffc)
    {
        if(flag3==mfHAMMER||flag3==mfSTRIKE)
        {
            findentrance(fx,fy,mfHAMMER,true);
            findentrance(fx,fy,mfSTRIKE,true);
        }
        else
        {
            s->ffdata[current_ffcombo]+=1;
        }
    }
    
    if(!ignorescreen)
    {
        if(pound)
            s->data[i]+=1;
            
        set_bit(screengrid,i,1);
        
        if((flag==mfARMOS_ITEM||flag2==mfARMOS_ITEM) && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
        {
            items.add(new item((zfix)bx, (zfix)by, (zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
            sfx(tmpscr->secretsfx);
        }
        
        if(type==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
            
        putcombo(scrollbuf,(i&15)<<4,i&0xF0,s->data[i],s->cset[i]);
    }
    
    if(!ignoreffc)
    {
        set_bit(ffcgrid,current_ffcombo,1);
        
        if(type2==cPOUND && get_bit(quest_rules,qr_MORESOUNDS))
            sfx(WAV_ZN1HAMMERPOST,int32_t(bx));
    }
    
    return;
}

//defend results should match defence types. 
//RETURN VALUES: 
// -1 iGNORE WEAPON
// 0 No effects
// 1 Effects, weapon is not ignored or removed
int32_t LinkClass::defend(weapon *w)
{
	int32_t def = conv_edef_unblockable(defence[w->id], w->unblockable);
	switch(def)
	{
		case edNORMAL: return 1;
		case edHALFDAMAGE: // : IMPLEMENTED : Take half damage
		{
			w->power *= 0.5;
			return 1;
		}
		case edQUARTDAMAGE:
		{
			w->power *= 0.25;
			return 1;
		}
		case edSTUNONLY:
		{
			setStunClock(120);
			return 1;
		}
		case edSTUNORCHINK: // : IMPLEMENTED : If damage > 0, stun instead. Else, bounce off.
		{
			if (w->power > 0) 
			{
				setStunClock(120);
				return 1;
			}
			else 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
		}
		case edSTUNORIGNORE: // : IMPLEMENTED : If damage > 0, stun instead. Else, ignore.
		{
			if (w->power > 0) 
			{
				setStunClock(120);
				return 1;
			}
			else 
			{
				return -1;
			}
		}
		case edCHINKL1: // : IMPLEMENTED : Bounces off, plays SFX_CHINK
		{
			if (w->power < 1) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case edCHINKL2: // : IMPLEMENTED : Bounce off unless damage >= 2
		{
			if (w->power < 2) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case edCHINKL4: //: IMPLEMENTED : Bounce off unless damage >= 4
		{
			if (w->power < 4) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case edCHINKL6: // : IMPLEMENTED : Bounce off unless damage >= 6
		{
			if (w->power < 6) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case edCHINKL8: // : IMPLEMENTED : Bounce off unless damage >= 8
		{
			if (w->power < 8) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case edCHINK: // : IMPLEMENTED : Bounces off, plays SFX_CHINK
		{
			sfx(WAV_CHINK,pan(int32_t(x)));
			w->dead = 0;
			return -1;
		}
		case edIGNOREL1: // : IMPLEMENTED : Ignore unless damage > 1.
		{
			if (w->power < 1) 
			{
				return -1;
			}
			else return 1;
		}
		case edIGNORE: // : IMPLEMENTED : Do Nothing
		{
			return -1;
		}
		case ed1HKO: // : IMPLEMENTED : One-hit knock-out
		{
			game->set_life(0);
			return 1;
		}
		case edCHINKL10: //: IMPLEMENTED : If damage is less than 10
		{
			if (w->power < 10) 
			{
				sfx(WAV_CHINK,pan(int32_t(x)));
				w->dead = 0;
				return -1;
			}
			else 
			{
				return 1;
			}
		}
		case ed2x: // : IMPLEMENTED : Double damage.
		{
			w->power *= 2;
			return 1;
		}
		case ed3x: // : IMPLEMENTED : Triple Damage.
		{
			w->power *= 3;
			return 1;
		}
		case ed4x: // : IMPLEMENTED : 4x damage.
		{
			w->power *= 4;
			return 1;
		}
		case edHEAL: // : IMPLEMENTED : Gain the weapon damage in HP.
		{
			//sfx(WAV_HEAL,pan(int32_t(x)));
			game->set_life(zc_min(game->get_life()+w->power, game->get_maxlife()));
			w->dead = 0;
			return -1;
		}
	
		case edFREEZE: return 1; //Not IMPLEMENTED
	
		case edLEVELDAMAGE: //Damage * item level
		{
			w->power *= w->family_level;
			return 1;
		}
		case edLEVELREDUCTION: //Damage / item level
		{
			if ( w->family_level > 0 ) 
			{
				w->power /= w->family_level;
			}
			else w->power = 0;
			return 1;
		}
	
		//edLEVELCHINK2, //If item level is < 2: This needs a weapon variable that is set by 
		//edLEVELCHINK3, //If item level is < 3: the item that generates it (itemdata::level stored to
		//edLEVELCHINK4, //If item level is < 4: weapon::level, or something similar; then a check to
		//edLEVELCHINK5, //If item level is < 5: read weapon::level in hit detection. 
	
		//edSHOCK, //buzz blob
	
	
		case edBREAKSHIELD: //destroy the player's present shield
		{
			w->power = 0;
			w->dead = 0;
			int32_t itemid = current_item_id(itype_shield);
			//sfx(WAV_BREAKSHIELD,pan(int32_t(x)));
			if(itemsbuf[itemid].flags&ITEM_EDIBLE)
				game->set_item(itemid, false);
			//Remove Link's shield
			return -1; 
		}
	
	
		
		default: return 0;
	}
}

int32_t LinkClass::EwpnHit()
{
	for(int32_t i=0; i<Ewpns.Count(); i++)
	{
		if(Ewpns.spr(i)->hit(x+7,y+7,z,2,2,1))
		{
			weapon *ew = (weapon*)(Ewpns.spr(i));
			bool hitshield=false;
			
			if((ew->ignoreLink)==true || ew->fallclk|| ew->drownclk)
				break;
		
			int32_t defresult = defend(ew);
			if ( defresult == -1 ) return -1; //The weapon did something special, but it is otherwise ignored, possibly killed by defend(). 
				
			if(ew->id==ewWind)
			{
				xofs=1000;
				action=freeze; FFCore.setLinkAction(freeze);
				ew->misc=999;                                         // in enemy wind
				attackclk=0;
				return -1;
			}
			
			switch(dir)
			{
				case up:
					if(ew->dir==down || ew->dir==l_down || ew->dir==r_down)
						hitshield=true;
						
					break;
					
				case down:
					if(ew->dir==up || ew->dir==l_up || ew->dir==r_up)
						hitshield=true;
						
					break;
					
				case left:
					if(ew->dir==right || ew->dir==r_up || ew->dir==r_down)
						hitshield=true;
						
					break;
					
				case right:
					if(ew->dir==left || ew->dir==l_up || ew->dir==l_down)
						hitshield=true;
						
					break;
			}
			
			switch(ew->id)
			{
				case ewLitBomb:
				case ewBomb:
				case ewLitSBomb:
				case ewSBomb:
					return i;
			}
			
			if(!hitshield || (action==attacking||action==sideswimattacking) || action==swimming || action == sideswimming || action == sideswimattacking || charging > 0 || spins > 0 || hopclk==0xFF)
			{
				return i;
			}
			
			int32_t itemid = current_item_id(itype_shield);
			
			if(itemid<0 || !(checkbunny(itemid) && checkmagiccost(itemid))) return i;
			
			paymagiccost(itemid);
			
			bool reflect = false;
			
			switch(ew->id)
			{
				case ewFireball2:
				case ewFireball:
					if(ew->type & 1) //Boss fireball
					{
						if(!(itemsbuf[itemid].misc1 & (shFIREBALL2)))
							return i;
							
						reflect = ((itemsbuf[itemid].misc2 & shFIREBALL2) != 0);
					}
					else
					{
						if(!(itemsbuf[itemid].misc1 & (shFIREBALL)))
							return i;
							
						reflect = ((itemsbuf[itemid].misc2 & shFIREBALL) != 0);
					}
					
					break;
					
				case ewMagic:
					if(!(itemsbuf[itemid].misc1 & shMAGIC))
						return i;
						
					reflect = ((itemsbuf[itemid].misc2 & shMAGIC) != 0);
					break;
					
				case ewSword:
					if(!(itemsbuf[itemid].misc1 & shSWORD))
						return i;
						
					reflect = ((itemsbuf[itemid].misc2 & shSWORD) != 0);
					break;
					
				case ewFlame:
					if(!(itemsbuf[itemid].misc1 & shFLAME))
						return i;
						
					reflect = ((itemsbuf[itemid].misc2 & shFLAME) != 0); // Actually isn't reflected.
					break;
					
				case ewRock:
					if(!(itemsbuf[itemid].misc1 & shROCK))
						return i;
						
					reflect = (itemsbuf[itemid].misc2 & shROCK);
					break;
					
				case ewArrow:
					if(!(itemsbuf[itemid].misc1 & shARROW))
						return i;
						
					reflect = ((itemsbuf[itemid].misc2 & shARROW) != 0); // Actually isn't reflected.
					break;
					
				case ewBrang:
					if(!(itemsbuf[itemid].misc1 & shBRANG))
						return i;
						
					break;
					
				default: // Just throw the script weapons in here...
					if(ew->id>=wScript1 && ew->id<=wScript10)
					{
						if(!(itemsbuf[itemid].misc1 & shSCRIPT))
							return i;
							
						reflect = ((itemsbuf[itemid].misc2 & shSCRIPT) != 0);
					}
					
					break;
			}
			
			if(reflect && (ew->unblockable&WPNUNB_REFL))
				reflect = false;
			if(!reflect && (ew->unblockable&WPNUNB_SHLD))
				return i;
			
			int32_t oldid = ew->id;
			ew->onhit(false, reflect ? 2 : 1, dir);
			
			if(ew->id != oldid)                                     // changed type from ewX to wX
			{
				//        ew->power*=game->get_hero_dmgmult();
				Lwpns.add(ew);
				Ewpns.remove(ew);
				ew->isLWeapon = true; //Make sure this gets set everywhere!
			}
			
			if(ew->id==wRefMagic)
			{
				ew->ignoreLink=true;
				ew->ignorecombo=-1;
			}
			
			sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
		}
	}
	
	return -1;
}

int32_t LinkClass::LwpnHit()                                    //only here to check magic hits
{
    for(int32_t i=0; i<Lwpns.Count(); i++)
        if(Lwpns.spr(i)->hit(x+7,y+7,z,2,2,1))
        {
            weapon *lw = (weapon*)(Lwpns.spr(i));
            bool hitshield=false;
            
            if((lw->ignoreLink)==true)
                break;
                
            switch(dir)
            {
            case up:
                if(lw->dir==down || lw->dir==l_down || lw->dir==r_down)
                    hitshield=true;
                    
                break;
                
            case down:
                if(lw->dir==up || lw->dir==l_up || lw->dir==r_up)
                    hitshield=true;
                    
                break;
                
            case left:
                if(lw->dir==right || lw->dir==r_up || lw->dir==r_down)
                    hitshield=true;
                    
                break;
                
            case right:
                if(lw->dir==left || lw->dir==l_up || lw->dir==l_down)
                    hitshield=true;
                    
                break;
            }
            
            int32_t itemid = current_item_id(itype_shield);
            bool reflect = false;
            
            switch(lw->id)
            {
            case wRefFireball:
                if(itemid<0)
                    return i;
                    
                if(lw->type & 1)  //Boss fireball
                    return i;
                    
                if(!(itemsbuf[itemid].misc1 & (shFIREBALL)))
                    return i;
                    
                reflect = ((itemsbuf[itemid].misc2 & shFIREBALL) != 0);
                break;
                
            case wRefMagic:
                if(itemid<0)
                    return i;
                    
                if(!(itemsbuf[itemid].misc1 & shMAGIC))
                    return i;
                    
                reflect = ((itemsbuf[itemid].misc2 & shMAGIC) != 0);
                break;
                
            case wRefBeam:
                if(itemid<0)
                    return i;
                    
                if(!(itemsbuf[itemid].misc1 & shSWORD))
                    return i;
                    
                reflect = ((itemsbuf[itemid].misc2 & shSWORD) != 0);
                break;
                
            case wRefRock:
                if(itemid<0)
                    return i;
                    
                if(!(itemsbuf[itemid].misc1 & shROCK))
                    return i;
                    
                reflect = (itemsbuf[itemid].misc2 & shROCK);
                break;
                
            default:
                return -1;
            }
            
            if(!hitshield || (action==attacking||action==sideswimattacking) || action==swimming || action == sideswimming || action == sideswimattacking || hopclk==0xFF)
                return i;
                
            if(itemid<0 || !(checkbunny(itemid) && checkmagiccost(itemid))) return i;
            
            paymagiccost(itemid);
            
            lw->onhit(false, 1+reflect, dir);
            lw->ignoreLink=true;
            lw->ignorecombo=-1;
            sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
        }
        
    return -1;
}

void LinkClass::checkhit()
{
    if(checklink==true)
    {
        if(hclk>0)
        {
            --hclk;
        }
        
        if(NayrusLoveShieldClk>0)
        {
            --NayrusLoveShieldClk;
            
            if(NayrusLoveShieldClk == 0 && nayruitem != -1)
            {
                stop_sfx(itemsbuf[nayruitem].usesound);
                stop_sfx(itemsbuf[nayruitem].usesound+1);
                nayruitem = -1;
            }
            else if(get_bit(quest_rules,qr_MORESOUNDS) && !(NayrusLoveShieldClk&0xF00) && nayruitem != -1)
            {
                stop_sfx(itemsbuf[nayruitem].usesound);
                cont_sfx(itemsbuf[nayruitem].usesound+1);
            }
        }
    }
    
    if(hclk<39 && action==gothit)
    {
        action=none; FFCore.setLinkAction(none);
    }
        
    if(hclk<39 && (action==swimhit || action == sideswimhit))
    {
	SetSwim();
    }
        
    if(hclk>=40 && action==gothit)
    {
        if(((ladderx+laddery) && ((hitdir&2)==ladderdir))||(!(ladderx+laddery)))
        {
            for(int32_t i=0; i<4; i++)
            {
                switch(hitdir)
                {
                case up:
                    if(hit_walkflag(x,y+(bigHitbox?-1:7),2)||(x.getInt()&7?hit_walkflag(x+16,y+(bigHitbox?-1:7),1):0))    
		    {
			    action=none; FFCore.setLinkAction(none);
		    }
                    else --y;
                    
                    break;
                    
                case down:
                    if(hit_walkflag(x,y+16,2)||(x.getInt()&7?hit_walkflag(x+16,y+16,1):0))   
		    {
			    action=none; FFCore.setLinkAction(none);
		    }
                    else ++y;
                    
                    break;
                    
                case left:
                    if(hit_walkflag(x-1,y+(bigHitbox?0:8),1)||hit_walkflag(x-1,y+8,1)||(y.getInt()&7?hit_walkflag(x-1,y+16,1):0))  
		    {
			    action=none; FFCore.setLinkAction(none);
		    }
                    else --x;
                    
                    break;
                    
                case right:
                    if(hit_walkflag(x+16,y+(bigHitbox?0:8),1)||hit_walkflag(x+16,y+8,1)||(y.getInt()&7?hit_walkflag(x+16,y+16,1):0))
		    {
			    action=none; FFCore.setLinkAction(none);
		    }
                    else ++x;
                    
                    break;
                }
            }
        }
    }
    
    if(hclk>0 || inlikelike == 1 || action==inwind || action==drowning || action==lavadrowning || action==sidedrowning || inwallm || isDiving() || (action==hopping && hopclk<255))
    {
        return;
    }
    
    for(int32_t i=0; i<Lwpns.Count(); i++)
    {
        sprite *s = Lwpns.spr(i);
	int32_t itemid = ((weapon*)(Lwpns.spr(i)))->parentitem;
        //if ( itemdbuf[parentitem].flags&ITEM_FLAGS3 ) //can damage Link
	    //if ( itemsbuf[parentitem].misc1 > 0 ) //damages Link by this amount. 
        if((!(itemid==-1&&get_bit(quest_rules,qr_FIREPROOFLINK)||((itemid>-1&&itemsbuf[itemid].family==itype_candle||itemsbuf[itemid].family==itype_book)&&(itemsbuf[itemid].flags & ITEM_FLAG3)))) && (scriptcoldet&1) && !fallclk && (!superman || !get_bit(quest_rules,qr_FIREPROOFLINK2)))
        {
            if(s->id==wFire && (superman ? (diagonalMovement?s->hit(x+4,y+4,z,7,7,1):s->hit(x+7,y+7,z,2,2,1)) : s->hit(this))&&
                        (itemid < 0 || itemsbuf[itemid].family!=itype_dinsfire))
            {
                if(NayrusLoveShieldClk<=0)
                {
                    int32_t ringpow = ringpower(lwpn_dp(i));
                    game->set_life(zc_max(game->get_life()-ringpow,0));
                }
                
                hitdir = s->hitdir(x,y,16,16,dir);
                
                if (action != rafting && action != freeze && action != sideswimfreeze)
                {
			if (IsSideSwim())
			{
				action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
			}
			else if (action == swimming || hopclk == 0xFF)
			{
				action=swimhit; FFCore.setLinkAction(swimhit);
			}
			else
			{
				action=gothit; FFCore.setLinkAction(gothit);
			}
                }
                    
                if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
                {
                    spins = charging = attackclk = 0;
                    attack=none;
                    tapping = false;
                }
                
                hclk=48;
                sfx(getHurtSFX(),pan(x.getInt()));
                return;
            }
        }
        
        //   check enemy weapons true, 1, -1
        //
        if(itemsbuf[itemid].flags & ITEM_FLAG6)
        {
            if(s->id==wBrang || s->id==wHookshot)
            {
                int32_t itemid = ((weapon*)s)->parentitem>-1 ? ((weapon*)s)->parentitem :
                             directWpn>-1 ? directWpn : current_item_id(s->id==wHookshot ? itype_hookshot : itype_brang);
                itemid = vbound(itemid, 0, MAXITEMS-1);
                
                for(int32_t j=0; j<Ewpns.Count(); j++)
                {
                    sprite *t = Ewpns.spr(j);
                    
                    if(s->hit(t->x+7,t->y+7,t->z,2,2,1))
                    {
                        bool reflect = false;
                       // sethitLinkUID(HIT_BY_EWEAPON,j); //set that Link was hit by a specific eweapon index. 
                        switch(t->id)
                        {
                        case ewBrang:
                            if(!(itemsbuf[itemid].misc3 & shBRANG)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shBRANG) != 0);
                            goto killweapon;
                            
                        case ewArrow:
                            if(!(itemsbuf[itemid].misc3 & shARROW)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shARROW) != 0);
                            goto killweapon;
                            
                        case ewRock:
                            if(!(itemsbuf[itemid].misc3 & shROCK)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shROCK) != 0);
                            goto killweapon;
                            
                        case ewFireball2:
                        case ewFireball:
                        {
                            int32_t mask = (((weapon*)t)->type&1 ? shFIREBALL2 : shFIREBALL);
                            
                            if(!(itemsbuf[itemid].misc3 & mask)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & mask) != 0);
                            goto killweapon;
                        }
                        
                        case ewSword:
                            if(!(itemsbuf[itemid].misc3 & shSWORD)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shSWORD) != 0);
                            goto killweapon;
                            
                        case wRefMagic:
                        case ewMagic:
                            if(!(itemsbuf[itemid].misc3 & shMAGIC)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shMAGIC) != 0);
                            goto killweapon;
                            
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
                            if(!(itemsbuf[itemid].misc3 & shSCRIPT)) break;
                            
                            reflect = ((itemsbuf[itemid].misc4 & shSCRIPT) != 0);
                            goto killweapon;
                            
                        case ewLitBomb:
                        case ewLitSBomb:
killweapon:
                            ((weapon*)s)->dead=1;
                            weapon *ew = ((weapon*)t);
                            int32_t oldid = ew->id;
                            ew->onhit(true, reflect ? 2 : 1, s->dir);
                            
                            /*if (s->dummy_bool[0])
                            {
                              add_grenade(s->x,s->y,s->z,0,-1);
                              s->dummy_bool[0]=false;
                            }*/
                            if(ew->id != oldid || (ew->id>=wScript1 && ew->id<=wScript10)) // changed type from ewX to wX... Except for script weapons
                            {
                                Lwpns.add(ew);
                                Ewpns.remove(ew);
				ew->isLWeapon = true; //Make sure this gets set everywhere!
                            }
                            
                            if(ew->id==wRefMagic)
                            {
                                ew->ignoreLink=true;
                                ew->ignorecombo=-1;
                            }
                            
                            break;
                        }
                        
                        break;
                    }
                }
            }
        }
        
        if((itemsbuf[itemid].flags & ITEM_FLAG2)||(itemid==-1&&get_bit(quest_rules,qr_OUCHBOMBS)))
        {
            //     if(((s->id==wBomb)||(s->id==wSBomb)) && (superman ? s->hit(x+7,y+7,z,2,2,1) : s->hit(this)))
            if(((s->id==wBomb)||(s->id==wSBomb)) && s->hit(this) && !superman && (scriptcoldet&1) && !fallclk)
            {
                if(NayrusLoveShieldClk<=0)
                {
                    int32_t ringpow = ringpower(((((weapon*)s)->parentitem>-1 ? itemsbuf[((weapon*)s)->parentitem].misc3 : ((weapon*)s)->power) *game->get_hp_per_heart()));
                    game->set_life(zc_min(game->get_maxlife(), zc_max(game->get_life()-ringpow,0)));
                }
                
                hitdir = s->hitdir(x,y,16,16,dir);
                
                if (action != rafting && action != freeze && action != sideswimfreeze)
                {
			if (IsSideSwim())
			{
				action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
			}
			else if (action == swimming || hopclk == 0xFF)
			{
				action=swimhit; FFCore.setLinkAction(swimhit);
			}
			else
			{
				action=gothit; FFCore.setLinkAction(gothit);
			}
                }
                    
                if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
                {
                    spins = charging = attackclk = 0;
                    attack=none;
                    tapping = false;
                }
                
                hclk=48;
                sfx(getHurtSFX(),pan(x.getInt()));
                return;
            }
        }
        
        if(hclk==0 && s->id==wWind && s->hit(x+7,y+7,z,2,2,1) && !fairyclk)
        {
            reset_hookshot();
            xofs=1000;
            action=inwind; FFCore.setLinkAction(inwind);
            dir=s->dir;
            spins = charging = attackclk = 0;
            
            // In case Link used two whistles in a row, summoning two whirlwinds,
            // check which whistle's whirlwind picked him up so the correct
            // warp ring will be used
            int32_t whistle=((weapon*)s)->parentitem;
            
            if(whistle>-1 && itemsbuf[whistle].family==itype_whistle)
                whistleitem=whistle;
                
            return;
        }
    }
    
    if(action==rafting || action==freeze || action==sideswimfreeze ||
            action==casting || action==sideswimcasting || action==drowning || action==lavadrowning || action==sidedrowning || superman || !(scriptcoldet&1) || fallclk)
        return;
        
    int32_t hit2 = diagonalMovement?GuyHit(x+4,y+4,z,8,8,hzsz):GuyHit(x+7,y+7,z,2,2,hzsz);
    
    
    
    if(hit2!=-1)
    {
        hitlink(hit2);
        return;
    }
    
    hit2 = LwpnHit();
    
    if(hit2!=-1)
    {
        if(NayrusLoveShieldClk<=0)
        {
            int32_t ringpow = ringpower(lwpn_dp(hit2));
            game->set_life(zc_max(game->get_life()-ringpow,0));
	    sethitLinkUID(HIT_BY_LWEAPON,(hit2+1));  //this is first readable after waitdraw. 
		//Z_scripterrlog("lweapon hit2 is: %d\n", hit2*10000);
		//Z_scripterrlog("Link->HitBy[LWPN] is: %d\n", gethitLinkUID(HIT_BY_LWEAPON));
            
        }
        
        hitdir = Lwpns.spr(hit2)->hitdir(x,y,16,16,dir);
        ((weapon*)Lwpns.spr(hit2))->onhit(false);
        
        if (IsSideSwim())
	{
		action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
	}
        else if(action==swimming || hopclk==0xFF)
	{
            action=swimhit; FFCore.setLinkAction(swimhit);
	}
        else
	{
            action=gothit; FFCore.setLinkAction(gothit);
	}
            
        hclk=48;
        
        if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
        {
            spins = charging = attackclk = 0;
            attack=none;
            tapping = false;
        }
        
        sfx(getHurtSFX(),pan(x.getInt()));
        return;
    }
    
    //else  { sethitLinkUID(HIT_BY_LWEAPON,(0));  //fails to clear
    
    hit2 = EwpnHit();
    
    if(hit2!=-1)
    {
        if(NayrusLoveShieldClk<=0)
        {
            int32_t ringpow = ringpower(ewpn_dp(hit2));
            game->set_life(zc_max(game->get_life()-ringpow,0));
	    sethitLinkUID(HIT_BY_EWEAPON,(hit2+1)); //this is first readable after waitdraw. 
		//Z_scripterrlog("wweapon hit2 is: %d\n", hit2*10000);
		//Z_scripterrlog("Link->HitBy[EWPN] is: %d\n", gethitLinkUID(HIT_BY_EWEAPON));
        }
        
        hitdir = Ewpns.spr(hit2)->hitdir(x,y,16,16,dir);
        ((weapon*)Ewpns.spr(hit2))->onhit(false);
        
	if (IsSideSwim())
	{
		action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
	}
        else if(action==swimming || hopclk==0xFF)
	{
            action=swimhit; FFCore.setLinkAction(swimhit);
	}
        else
	{
            action=gothit; FFCore.setLinkAction(gothit);
	}
            
        hclk=48;
        
        if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
        {
            spins = charging = attackclk = 0;
            attack=none;
            tapping = false;
        }
        
        sfx(getHurtSFX(),pan(x.getInt()));
        return;
    }
    //else { sethitLinkUID(HIT_BY_EWEAPON,(0)); } //fails to clear
    
    // The rest of this method deals with damage combos, which can be jumped over.
    if(z>0 && !(tmpscr->flags2&fAIRCOMBOS)) return;
    
    int32_t dx1 = (int32_t)x+8-(tmpscr->csensitive);
    int32_t dx2 = (int32_t)x+8+(tmpscr->csensitive-1);
    int32_t dy1 = (int32_t)y+(bigHitbox?8:12)-(bigHitbox?tmpscr->csensitive:(tmpscr->csensitive+1)/2);
    int32_t dy2 = (int32_t)y+(bigHitbox?8:12)+(bigHitbox?tmpscr->csensitive-1:((tmpscr->csensitive+1)/2)-1);
    
    for(int32_t i=get_bit(quest_rules, qr_DMGCOMBOLAYERFIX) ? 1 : -1; i>=-1; i--)  // Layers 0, 1 and 2!!
        (void)checkdamagecombos(dx1,dx2,dy1,dy2,i);
}

bool LinkClass::checkdamagecombos(int32_t dx, int32_t dy)
{
    return checkdamagecombos(dx,dx,dy,dy);
}

bool LinkClass::checkdamagecombos(int32_t dx1, int32_t dx2, int32_t dy1, int32_t dy2, int32_t layer, bool solid, bool do_health_check) //layer = -1, solid = false, do_health_check = true
{
	if(hclk || superman || fallclk)
		return false;
		
	int32_t hp_mod[4] = {0};
	byte hasKB = 0;
	
	{
		newcombo& cmb = combobuf[layer>-1?MAPCOMBO2(layer,dx1,dy1):MAPCOMBO(dx1,dy1)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1) 
				hp_mod[0] = cmb.attributes[0] / -10000L;
			else 
				hp_mod[0]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<0;
		}
	}
	{
		newcombo& cmb = combobuf[layer>-1?MAPCOMBO2(layer,dx1,dy2):MAPCOMBO(dx1,dy2)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1) 
				hp_mod[1] = cmb.attributes[0] / -10000L;
			else 
				hp_mod[1]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<1;
		}
	}
	{
		newcombo& cmb = combobuf[layer>-1?MAPCOMBO2(layer,dx2,dy1):MAPCOMBO(dx2,dy1)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1) 
				hp_mod[2] = cmb.attributes[0] / -10000L;
			else 
				hp_mod[2]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<2;
		}
	}
	{
		newcombo& cmb = combobuf[layer>-1?MAPCOMBO2(layer,dx2,dy2):MAPCOMBO(dx2,dy2)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1) 
				hp_mod[3] = cmb.attributes[0] / -10000L;
			else 
				hp_mod[3]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<3;
		}
	}
	
	
	int32_t hp_modtotal=0;
	
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (!_effectflag(dx1,dy1,1, layer) || (combobuf[MAPCOMBO2(i,dx1,dy1)].type == cBRIDGE && !_walkflag_layer(dx1,dy1,1, &(tmpscr2[i])))) {hp_mod[0] = 0; hasKB &= ~(1<<0);}
			if (!_effectflag(dx1,dy2,1, layer) || (combobuf[MAPCOMBO2(i,dx1,dy2)].type == cBRIDGE && !_walkflag_layer(dx1,dy2,1, &(tmpscr2[i])))) {hp_mod[1] = 0; hasKB &= ~(1<<1);}
			if (!_effectflag(dx2,dy1,1, layer) || (combobuf[MAPCOMBO2(i,dx2,dy1)].type == cBRIDGE && !_walkflag_layer(dx2,dy1,1, &(tmpscr2[i])))) {hp_mod[2] = 0; hasKB &= ~(1<<2);}
			if (!_effectflag(dx2,dy2,1, layer) || (combobuf[MAPCOMBO2(i,dx2,dy2)].type == cBRIDGE && !_walkflag_layer(dx2,dy2,1, &(tmpscr2[i])))) {hp_mod[3] = 0; hasKB &= ~(1<<3);}
		}
	}
	
	for(int32_t i=0; i<4; i++)
	{
		if(get_bit(quest_rules,qr_DMGCOMBOPRI))
		{
			if(hp_modtotal >= 0) //Okay, if it's over 0, it's healing Link.
				hp_modtotal = zc_min(hp_modtotal, hp_mod[i]);
			else if(hp_mod[i] < 0) //If it's under 0, it's hurting Link.
				hp_modtotal = zc_max(hp_modtotal, hp_mod[i]);
		}
		else
			hp_modtotal = zc_min(hp_modtotal, hp_mod[i]);
	}
	
	{
		newcombo& cmb = combobuf[MAPFFCOMBO(dx1,dy1)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1 )
				hp_mod[0] = combobuf[MAPFFCOMBO(dx1,dy1)].attributes[0]/10000L;
			else
				hp_mod[0]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<4;
		}
	}
	{
		newcombo& cmb = combobuf[MAPFFCOMBO(dx1,dy2)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1 )
				hp_mod[1] = combobuf[MAPFFCOMBO(dx1,dy1)].attributes[0]/10000L;
			else
				hp_mod[1]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<5;
		}
	}
	{
		newcombo& cmb = combobuf[MAPFFCOMBO(dx2,dy1)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1 )
				hp_mod[2] = combobuf[MAPFFCOMBO(dx1,dy1)].attributes[0]/10000L;
			else
				hp_mod[2]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<6;
		}
	}
	{
		newcombo& cmb = combobuf[MAPFFCOMBO(dx2,dy2)];
		if ( combo_class_buf[cmb.type].modify_hp_amount)
		{
			if(cmb.usrflags&cflag1 )
				hp_mod[3] = combobuf[MAPFFCOMBO(dx1,dy1)].attributes[0]/10000L;
			else
				hp_mod[3]=combo_class_buf[cmb.type].modify_hp_amount;
			if(!(cmb.usrflags&cflag2))
				hasKB |= 1<<7;
		}
	}
	
	int32_t hp_modtotalffc = 0;
	
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,dx1,dy1)].type == cBRIDGE && !_walkflag_layer(dx1,dy1,1, &(tmpscr2[i]))) {hp_mod[0] = 0; hasKB &= ~(1<<4);}
			if (combobuf[MAPCOMBO2(i,dx1,dy2)].type == cBRIDGE && !_walkflag_layer(dx1,dy2,1, &(tmpscr2[i]))) {hp_mod[1] = 0; hasKB &= ~(1<<5);}
			if (combobuf[MAPCOMBO2(i,dx2,dy1)].type == cBRIDGE && !_walkflag_layer(dx2,dy1,1, &(tmpscr2[i]))) {hp_mod[2] = 0; hasKB &= ~(1<<6);}
			if (combobuf[MAPCOMBO2(i,dx2,dy2)].type == cBRIDGE && !_walkflag_layer(dx2,dy2,1, &(tmpscr2[i]))) {hp_mod[3] = 0; hasKB &= ~(1<<7);}
		}
	}
	
	for(int32_t i=0; i<4; i++)
	{
		if(get_bit(quest_rules,qr_DMGCOMBOPRI))
		{
			if(hp_modtotalffc >= 0)
				hp_modtotalffc = zc_min(hp_modtotalffc, hp_mod[i]);
			else if(hp_mod[i] < 0)
				hp_modtotalffc = zc_max(hp_modtotalffc, hp_mod[i]);
		}
		else
			hp_modtotalffc = zc_min(hp_modtotalffc, hp_mod[i]);
	}
	
	int32_t hp_modmin = zc_min(hp_modtotal, hp_modtotalffc);
	
	bool global_ring = ((itemsbuf[current_item_id(itype_ring)].flags & ITEM_FLAG1));
	bool current_ring = ((tmpscr->flags6&fTOGGLERINGDAMAGE) != 0);
	
	int32_t itemid = current_item_id(itype_boots);
	
	bool bootsnosolid = itemid >= 0 && 0 != (itemsbuf[itemid].flags & ITEM_FLAG1);
	
	if(hp_modmin<0)
	{
		if((itemid<0) || (tmpscr->flags5&fDAMAGEWITHBOOTS) || (4<<current_item_power(itype_boots)<(abs(hp_modmin))) || (solid && bootsnosolid) || !(checkbunny(itemid) && checkmagiccost(itemid)))
		{
			if (!do_health_check) return true;
			if(NayrusLoveShieldClk<=0)
			{
				int32_t ringpow = ringpower(-hp_modmin);
				game->set_life(zc_max(game->get_life()-(global_ring!=current_ring ? ringpow:-hp_modmin),0));
			}
			
			if(hasKB)
				hitdir = (dir^1);
			else
				hitdir = -1;
			
			if (action != rafting && action != freeze && action != sideswimfreeze)
			{
				if (IsSideSwim())
				{
					action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
				}
				else if (action == swimming || hopclk == 0xFF)
				{
					action=swimhit; FFCore.setLinkAction(swimhit);
				}
				else
				{
					action=gothit; FFCore.setLinkAction(gothit);
				}
			}
				
				
			hclk=48;
			
			if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
			{
				spins = charging = attackclk = 0;
				attack=none;
				tapping = false;
			}
			
			sfx(getHurtSFX(),pan(x.getInt()));
			return true;
		}
		else if (do_health_check) paymagiccost(itemid); // Boots are successful
	}
	
	return false;
}

void LinkClass::hitlink(int32_t hit2)
{
	//printf("Stomp check: %d <= 12, %d < %d\n", int32_t((y+16)-(((enemy*)guys.spr(hit2))->y)), (int32_t)falling_oldy, (int32_t)y);
	int32_t stompid = current_item_id(itype_stompboots);
    if(current_item(itype_stompboots) && checkbunny(stompid) && checkmagiccost(stompid) && (stomping ||
            (z > (((enemy*)guys.spr(hit2))->z)) ||
            ((isSideViewLink() && (y+16)-(((enemy*)guys.spr(hit2))->y)<=14) && falling_oldy<y)))
    {
        paymagiccost(stompid);
        hit_enemy(hit2,wStomp,itemsbuf[stompid].power*game->get_hero_dmgmult(),x,y,0,stompid);
        
        if(itemsbuf[stompid].flags & ITEM_DOWNGRADE)
            game->set_item(stompid,false);
            
        // Stomp Boots script
        if(itemsbuf[stompid].script != 0 && !item_doscript[stompid])
        {
		//clear the item script stack for a new script
		ri = &(itemScriptData[stompid]);
		for ( int32_t q = 0; q < 1024; q++ ) item_stack[stompid][q] = 0xFFFF;
		ri->Clear();
		//itemScriptData[(stompid & 0xFFF)].Clear();
		//for ( int32_t q = 0; q < 1024; q++ ) item_stack[(stompid & 0xFFF)][q] = 0;
		//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[stompid].script, stompid & 0xFFF);
		item_doscript[stompid] = 1;
		itemscriptInitialised[stompid] = 0;
		ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[stompid].script, stompid);
        }
        
        return;
    }
    else if(superman || !(scriptcoldet&1) || fallclk)
        return;
    else if(NayrusLoveShieldClk<=0)
    {
        int32_t ringpow = ringpower(enemy_dp(hit2));
        game->set_life(zc_max(game->get_life()-ringpow,0));
	sethitLinkUID(HIT_BY_NPC,(hit2+1)); //this is first readable after waitdraw. 
	    //Z_scripterrlog("lweapon hit2 is: %d\n", hit2*10000);
		//Z_scripterrlog("Link->HitBy[NPC] is: %d\n", gethitLinkUID(HIT_BY_NPC));
	    
    }
    
    hitdir = guys.spr(hit2)->hitdir(x,y,16,16,dir);
    if (IsSideSwim())
    {
	   action=sideswimhit; FFCore.setLinkAction(sideswimhit); 
    }
    else if(action==swimming || hopclk==0xFF)
    {
        action=swimhit; FFCore.setLinkAction(swimhit);
    }
    else
    {
        action=gothit; FFCore.setLinkAction(gothit);
    }
        
    hclk=48;
    sfx(getHurtSFX(),pan(x.getInt()));
    
    if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
    {
        spins = charging = attackclk = 0;
        attack=none;
        tapping = false;
    }
    
    enemy_scored(hit2);
    int32_t dm7 = ((enemy*)guys.spr(hit2))->dmisc7;
    int32_t dm8 = ((enemy*)guys.spr(hit2))->dmisc8;
    
    switch(((enemy*)guys.spr(hit2))->family)
    {
    case eeWALLM:
        if(((enemy*)guys.spr(hit2))->hp>0)
        {
            GrabLink(hit2);
            inwallm=true;
            action=none; FFCore.setLinkAction(none);
        }
        
        break;
        
        //case eBUBBLEST:
        //case eeBUBBLE:
    case eeWALK:
    {
        int32_t itemid = current_item_id(itype_whispring);
        //I can only assume these are supposed to be int32_t, not bool ~pkmnfrk
        int32_t sworddivisor = ((itemid>-1 && itemsbuf[itemid].misc1 & 1) ? itemsbuf[itemid].power : 1);
        int32_t itemdivisor = ((itemid>-1 && itemsbuf[itemid].misc1 & 2) ? itemsbuf[itemid].power : 1);
        
        switch(dm7)
        {
        case e7tTEMPJINX:
            if(dm8==0 || dm8==2)
                if(swordclk>=0 && !(sworddivisor==0))
                    swordclk=150;
                    
            if(dm8==1 || dm8==2)
                if(itemclk>=0 && !(itemdivisor==0))
                    itemclk=150;
                    
            break;
            
        case e7tPERMJINX:
            if(dm8==0 || dm8==2)
                if(sworddivisor) swordclk=(itemid >-1 && itemsbuf[itemid].flags & ITEM_FLAG1)? int32_t(150/sworddivisor) : -1;
                
            if(dm8==1 || dm8==2)
                if(itemdivisor) itemclk=(itemid >-1 && itemsbuf[itemid].flags & ITEM_FLAG1)? int32_t(150/itemdivisor) : -1;
                
            break;
            
        case e7tUNJINX:
            if(dm8==0 || dm8==2)
                swordclk=0;
                
            if(dm8==1 || dm8==2)
                itemclk=0;
                
            break;
            
        case e7tTAKEMAGIC:
            game->change_dmagic(-dm8*game->get_magicdrainrate());
            break;
            
        case e7tTAKERUPEES:
            game->change_drupy(-dm8);
            break;
            
        case e7tDRUNK:
            drunkclk += dm8;
            break;
        }
        
        if(dm7 >= e7tEATITEMS)
        {
            EatLink(hit2);
            inlikelike=(dm7 == e7tEATHURT ? 2:1);
            action=none; FFCore.setLinkAction(none);
        }
    }
    }
}

void LinkClass::addsparkle(int32_t wpn)
{
	//return;
    weapon *w = (weapon*)Lwpns.spr(wpn);
    int32_t itemid = w->parentitem;
    
    if(itemid<0)
        return;
        
    int32_t itemtype = itemsbuf[itemid].family;
    
    if(itemtype!=itype_cbyrna && frame%4)
        return;
        
    int32_t wpn2 = (itemtype==itype_cbyrna) ? itemsbuf[itemid].wpn4 : itemsbuf[itemid].wpn2;
    int32_t wpn3 = (itemtype==itype_cbyrna) ? itemsbuf[itemid].wpn5 : itemsbuf[itemid].wpn3;
    // Either one (wpn2) or the other (wpn3). If both are present, randomise.
    int32_t sparkle_type = (!wpn2 ? (!wpn3 ? 0 : wpn3) : (!wpn3 ? wpn2 : (zc_oldrand()&1 ? wpn2 : wpn3)));
    int32_t direction=w->dir;
    
    if(sparkle_type)
    {
        int32_t h=0;
        int32_t v=0;
        
        if(w->dir==right||w->dir==r_up||w->dir==r_down)
        {
            h=-1;
        }
        
        if(w->dir==left||w->dir==l_up||w->dir==l_down)
        {
            h=1;
        }
        
        if(w->dir==down||w->dir==l_down||w->dir==r_down)
        {
            v=-1;
        }
        
        if(w->dir==up||w->dir==l_up||w->dir==r_up)
        {
            v=1;
        }
        
        // Damaging boomerang sparkle?
        if(wpn3 && itemtype==itype_brang)
        {
            // If the boomerang just bounced, flip the sparkle direction so it doesn't hit
            // whatever it just bounced off of if it's shielded from that direction.
            if(w->misc==1 && w->clk2>256 && w->clk2<272)
                direction=oppositeDir[direction];
        }
	if(itemtype==itype_brang && get_bit(quest_rules, qr_WRONG_BRANG_TRAIL_DIR)) direction = 0;
        
        Lwpns.add(new weapon((zfix)(w->x+(itemtype==itype_cbyrna ? 2 : zc_oldrand()%4)+(h*4)),
                             (zfix)(w->y+(itemtype==itype_cbyrna ? 2 : zc_oldrand()%4)+(v*4)),
                             w->z,sparkle_type==wpn3 ? wFSparkle : wSSparkle,sparkle_type,0,direction,itemid,getUID(),false,false,true, 0, sparkle_type));
	weapon *w = (weapon*)(Lwpns.spr(Lwpns.Count()-1));
	}
}

// For wPhantoms
void LinkClass::addsparkle2(int32_t type1, int32_t type2)
{
    if(frame%4) return;
    
    int32_t arrow = -1;
    
    for(int32_t i=0; i<Lwpns.Count(); i++)
    {
        weapon *w = (weapon*)Lwpns.spr(i);
        
        if(w->id == wPhantom && w->type == type1)
        {
            arrow = i;
            break;
        }
    }
    
    if(arrow==-1)
    {
        return;
    }
    
    Lwpns.add(new weapon((zfix)((Lwpns.spr(arrow)->x-3)+(zc_oldrand()%7)),
                         (zfix)((Lwpns.spr(arrow)->y-3)+(zc_oldrand()%7)),
                         Lwpns.spr(arrow)->z, wPhantom, type2,0,0,((weapon*)Lwpns.spr(arrow))->parentitem,-1));
}

//cleans up decorations that exit the bounds of the screen for a int32_t time, to prevebt them wrapping around.
void LinkClass::PhantomsCleanup()
{
	if(Lwpns.idCount(wPhantom))
	{
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			weapon *w = ((weapon *)Lwpns.spr(i));
			if ( w->id == wPhantom && !w->isScriptGenerated() )
			{
				if ( w->x < -10000 || w->y > 10000 || w->x < -10000 || w->y > 10000 )
				{
					Lwpns.remove(w);
				}				
			}
		}	
	}
}

//Waitframe handler for refilling operations
static void do_refill_waitframe()
{
	put_passive_subscr(framebuf,&QMisc,0,passive_subscreen_offset,false,sspUP);
	if(get_bit(quest_rules, qr_PASSIVE_SUBSCRIPT_RUNS_WHEN_GAME_IS_FROZEN))
	{
		script_drawing_commands.Clear();
		if(DMaps[currdmap].passive_sub_script != 0)
			ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script, currdmap);
		if(passive_subscreen_waitdraw && DMaps[currdmap].passive_sub_script != 0 && passive_subscreen_doscript != 0)
		{
			ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script, currdmap);
			passive_subscreen_waitdraw = false;
		}	
		do_script_draws(framebuf, tmpscr, 0, playing_field_offset);
	}
	advanceframe(true);
}
//Special handler if it's a "fairy revive"
static void do_death_refill_waitframe()
{
	//!TODO Run a new script slot each frame here, before calling do_refill_waitframe()
	//This script should be able to draw a 'fairy saving the player' animation -Em
	do_refill_waitframe();
}

static size_t find_bottle_for_slot(size_t slot, bool unowned=false)
{
	int32_t found_unowned = -1;
	for(int q = 0; q < MAXITEMS; ++q)
	{
		if(itemsbuf[q].family == itype_bottle && itemsbuf[q].misc1 == slot)
		{
			if(game->get_item(q))
				return q;
			if(unowned)
				found_unowned = q;
		}
	}
	return found_unowned;
}

// returns true when game over
bool LinkClass::animate(int32_t)
{
	int32_t lsave=0;
	
	if (onpassivedmg)
	{
		onpassivedmg=false;
	}
	else if (damageovertimeclk != -1)
	{
		damageovertimeclk = -1;
	}
	
	if(do_cheat_goto)
	{
		didpit=true;
		pitx=x;
		pity=y;
		dowarp(3,0);
		do_cheat_goto=false;
		return false;
	}
	
	if(do_cheat_light)
	{
		naturaldark = !naturaldark;
		lighting(false, false, pal_litOVERRIDE);//Forcibly set permLit, overriding it's current setting
		do_cheat_light = false;
	}
	
	if(action!=climbcovertop&&action!=climbcoverbottom)
	{
		climb_cover_x=-1000;
		climb_cover_y=-1000;
	}
	
	if(mirror_portal)
	{
		mirror_portal->animate();
		if(abs(x - mirror_portal->x) < 12
			&& abs(y - mirror_portal->y) < 12)
		{
			if(can_mirror_portal)
			{
				x = mirror_portal->x;
				y = mirror_portal->y;
				
				FFCore.warp_link(wtIWARP, mirror_portal->destdmap, mirror_portal->destscr,
					-1, -1, mirror_portal->weffect, mirror_portal->wsfx, warpFlagNOSTEPFORWARD, -1);
				game->clear_portal(); //Remove portal once used
			}
		}
		else can_mirror_portal = true;
	}
	
	if(isGrassType(COMBOTYPE(x,y+15)) && isGrassType(COMBOTYPE(x+15,y+15))&& z<=8)
	{
		if(decorations.idCount(dTALLGRASS)==0)
		{
			decorations.add(new dTallGrass(x, y, dTALLGRASS, 0));
		}
	}
	
	if (get_bit(quest_rules, qr_SHALLOW_SENSITIVE))
	{
		if (z == 0 && action != swimming && action != isdiving && action != drowning && action!=lavadrowning && action!=sidedrowning && !IsSideSwim())
		{
			if (iswaterex(FFORCOMBO(x+11,y+15), currmap, currscr, -1, x+11,y+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(x+4,y+15), currmap, currscr, -1, x+4,y+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(x+11,y+9), currmap, currscr, -1, x+11,y+9, false, false, true, true)
			&& iswaterex(FFORCOMBO(x+4,y+9), currmap, currscr, -1, x+4,y+9, false, false, true, true))
			{
				if(decorations.idCount(dRIPPLES)==0)
				{
					decorations.add(new dRipples(x, y, dRIPPLES, 0));
				}
				int32_t watercheck = iswaterex(FFORCOMBO(x.getInt()+7.5,y.getInt()+12), currmap, currscr, -1, x.getInt()+7.5,y.getInt()+12, false, false, true, true);
				if (combobuf[watercheck].usrflags&cflag2)
				{
					if (!(current_item(combobuf[watercheck].attribytes[2]) > 0 && current_item(combobuf[watercheck].attribytes[2]) >= combobuf[watercheck].attribytes[3]))
					{
						onpassivedmg = true;
						if (damageovertimeclk == 0)
						{
							int32_t curhp = game->get_life();
							if (combobuf[watercheck].usrflags&cflag5) game->set_life(vbound(game->get_life()+ringpower(combobuf[watercheck].attributes[1]/10000L), 0, game->get_maxlife())); //Affected by rings
							else game->set_life(vbound(game->get_life()+combobuf[watercheck].attributes[1]/10000L, 0, game->get_maxlife()));
							if ((combobuf[watercheck].attributes[2]/10000L) && (game->get_life() != curhp || !(combobuf[watercheck].usrflags&cflag6))) sfx(combobuf[watercheck].attributes[2]/10000L);
							if (game->get_life() < curhp && combobuf[watercheck].usrflags&cflag7)
							{
								hclk = 48;
								hitdir = -1;
								action = gothit; FFCore.setLinkAction(gothit);
							}
						}
						if (combobuf[watercheck].attribytes[1] > 0)
						{
							if (damageovertimeclk <= 0 || damageovertimeclk > combobuf[watercheck].attribytes[1]) damageovertimeclk = combobuf[watercheck].attribytes[1];
							else --damageovertimeclk;
						}
						else damageovertimeclk = 0;
					}
					else damageovertimeclk = -1;
				}
				else damageovertimeclk = -1;
			}
		}
	}
	else
	{
		if((COMBOTYPE(x,y+15)==cSHALLOWWATER)&&(COMBOTYPE(x+15,y+15)==cSHALLOWWATER) && z==0)
		{
			if(decorations.idCount(dRIPPLES)==0)
			{
				decorations.add(new dRipples(x, y, dRIPPLES, 0));
			}
			int32_t watercheck = FFORCOMBO(x+7.5,y.getInt()+15);
			if (combobuf[watercheck].usrflags&cflag2)
			{
				if (!(current_item(combobuf[watercheck].attribytes[2]) > 0 && current_item(combobuf[watercheck].attribytes[2]) >= combobuf[watercheck].attribytes[3]))
				{
					onpassivedmg = true;
					if (damageovertimeclk == 0)
					{
						int32_t curhp = game->get_life();
						if (combobuf[watercheck].usrflags&cflag5) game->set_life(vbound(game->get_life()+ringpower(combobuf[watercheck].attributes[1]/10000L), 0, game->get_maxlife())); //Affected by rings
						else game->set_life(vbound(game->get_life()+(combobuf[watercheck].attributes[1]/10000L), 0, game->get_maxlife()));
						if ((combobuf[watercheck].attributes[2]/10000L) && (game->get_life() != curhp || !(combobuf[watercheck].usrflags&cflag6))) sfx(combobuf[watercheck].attributes[2]/10000L);
					}
					if (combobuf[watercheck].attribytes[1] > 0)
					{
						if (damageovertimeclk <= 0 || damageovertimeclk > combobuf[watercheck].attribytes[1]) damageovertimeclk = combobuf[watercheck].attribytes[1];
						else --damageovertimeclk;
					}
					else damageovertimeclk = 0;
				}
				else damageovertimeclk = -1;
			}
			else damageovertimeclk = -1;
		}
	}
	
	if(stomping)
		stomping = false;
	
	if(getOnSideviewLadder())
	{
		if(!canSideviewLadder() || jumping<0 || fall!=0)
		{
			setOnSideviewLadder(false);
		}
		else if(CANFORCEFACEUP)
		{
			setDir(up);
		}
	}
	
	if(action!=inwind && action!=drowning && action!=lavadrowning && action!= sidedrowning)
		if(!get_bit(quest_rules,qr_OLD_CHEST_COLLISION))
		{
			checkchest(cCHEST);
			checkchest(cLOCKEDCHEST);
			checkchest(cBOSSCHEST);
		}
	checksigns();
	
	if(isStanding())
	{
		if(extra_jump_count > 0)
			extra_jump_count = 0;
	}
	if(can_use_item(itype_hoverboots,i_hoverboots))
	{
		int32_t hoverid = current_item_id(itype_hoverboots);
		if(!(itemsbuf[hoverid].flags & ITEM_FLAG1))
		{
			if(hoverclk < 0) hoverclk = 0;
			hoverflags &= ~HOV_OUT;
		}
	}
	if(isSideViewLink() && (moveflags & FLAG_OBEYS_GRAV))  // Sideview gravity
	{
		//Handle falling through a platform
		if((y.getInt()%16==0) && (isSVPlatform(x+4,y+16) || isSVPlatform(x+12,y+16)) && !(on_sideview_solid(x,y)))
		{
			y+=1; //Fall down a pixel instantly, through the platform.
			if(fall < 0) fall = 0;
			if(jumping < 0) jumping = 0;
		}
		//Unless using old collision, run this check BEFORE moving Hero, to prevent clipping into the ceiling.
		if(!get_bit(quest_rules, qr_OLD_SIDEVIEW_CEILING_COLLISON))
		{
			if((_walkflag(x+4,y+((bigHitbox||!diagonalMovement)?(fall/100):(fall/100)+8),1,SWITCHBLOCK_STATE) || _walkflag(x+12,y+((bigHitbox||!diagonalMovement)?(fall/100):(fall/100)+8),1,SWITCHBLOCK_STATE)
				|| ((y+(fall/100)<=0) &&
				// Extra checks if Smart Screen Scrolling is enabled
				 (nextcombo_wf(up) || ((get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE)) &&
											   !(tmpscr->flags2&wfUP)) && (nextcombo_solid(up))))))
					&& fall < 0)
			{
				fall = jumping = 0; // Bumped his head
				y -= y.getInt()%8; //fix coords
				// ... maybe on spikes //this is the change from 2.50.1RC3 that Saffith made, that breaks some old quests. -Z
				if ( !get_bit(quest_rules, qr_OLDSIDEVIEWSPIKES) ) //fix for older sideview quests -Z
				{
					checkdamagecombos(x+4, x+12, y-1, y-1);
				}
			}
		}
		// Fall, unless on a ladder, sideview ladder, rafting, using the hookshot, drowning, sideswimming or cheating.
		if(!(toogam && Up()) && !drownclk && action!=rafting && !IsSideSwim() && !pull_link && !((ladderx || laddery) && fall>0) && !getOnSideviewLadder())
		{
			int32_t ydiff = fall/(spins && fall<0 ? 200:100);
			//zprint2("ydif is: %d\n", ydiff);
			//zprint2("ydif is: %d\n", (int32_t)fall);
			falling_oldy = y; // Stomp Boots-related variable
			if(fall > 0 && (checkSVLadderPlatform(x+4,y+ydiff+15)||checkSVLadderPlatform(x+12,y+ydiff+15)) && (((y.getInt()+ydiff+15)&0xF0)!=((y.getInt()+15)&0xF0)) && !platform_fallthrough())
			{
				ydiff -= (y.getInt()+ydiff)%16;
			}
			y+=ydiff;
			hs_starty+=ydiff;
			
			for(int32_t j=0; j<chainlinks.Count(); j++)
			{
				chainlinks.spr(j)->y+=ydiff;
			}
			
			if(Lwpns.idFirst(wHookshot)>-1)
			{
				Lwpns.spr(Lwpns.idFirst(wHookshot))->y+=ydiff;
			}
			
			if(Lwpns.idFirst(wHSHandle)>-1)
			{
				Lwpns.spr(Lwpns.idFirst(wHSHandle))->y+=ydiff;
			}
		}
		else if(IsSideSwim() && action != sidewaterhold1 && action != sidewaterhold2 && action != sideswimcasting && action != sideswimfreeze)
		{
			fall = hoverclk = jumping = 0;
			hoverflags = 0;
			if(!DrunkUp() && !DrunkDown() && !DrunkLeft() && !DrunkRight() && !autostep)
			{
				WalkflagInfo info;
				if (game->get_watergrav()<0)
				{
					info = walkflag(x,y+8-(bigHitbox*8)-2,2,up);
					execute(info);
				}
				else
				{
					info = walkflag(x,y+15+2,2,down);
					execute(info);
				}
			        if(!info.isUnwalkable() && (game->get_watergrav() > 0 || iswaterex(MAPCOMBO(x,y+8-(bigHitbox*8)-2), currmap, currscr, -1, x, y+8-(bigHitbox*8)-2, true, false))) y+=(game->get_watergrav()/10000.0);
			}
		}
		// Stop hovering/falling if you land on something.
		if((on_sideview_solid(x,y) || getOnSideviewLadder())  && !(pull_link && dir==down) && action!=rafting)
		{
			stop_item_sfx(itype_hoverboots);
			if(!getOnSideviewLadder() && (fall > 0 || get_bit(quest_rules, qr_OLD_SIDEVIEW_CEILING_COLLISON)))
				y-=(int32_t)y%8; //fix position
			fall = hoverclk = jumping = 0;
			hoverflags = 0;
			
			if(y>=160 && currscr>=0x70 && !(tmpscr->flags2&wfDOWN))  // Landed on the bottommost screen.
				y = 160;
		}
		// Stop hovering if you press down.
		else if((hoverclk>0 || ladderx || laddery) && DrunkDown())
		{
			stop_item_sfx(itype_hoverboots);
			hoverclk = -hoverclk;
			reset_ladder();
			fall = (zinit.gravity2 / 100);
		}
		// Continue falling.
		
		else if(fall <= (int32_t)zinit.terminalv)
		{
			
			if(fall != 0 || hoverclk>0)
				jumping++;
				
			// Bump head if: hit a solid combo from beneath, or hit a solid combo in the screen above this one.
			if(get_bit(quest_rules, qr_OLD_SIDEVIEW_CEILING_COLLISON))
			{
				if((_walkflag(x+4,y-(bigHitbox?9:1),0,SWITCHBLOCK_STATE)
					|| (y<=(bigHitbox?9:1) &&
					// Extra checks if Smart Screen Scrolling is enabled
					 (nextcombo_wf(up) || ((get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE)) &&
												   !(tmpscr->flags2&wfUP)) && (nextcombo_solid(up))))))
						&& fall < 0)
				{
					fall = jumping = 0; // Bumped his head
					
					// ... maybe on spikes //this is the change from 2.50.1RC3 that Saffith made, that breaks some old quests. -Z
					if ( !get_bit(quest_rules, qr_OLDSIDEVIEWSPIKES) ) //fix for older sideview quests -Z
					{
						checkdamagecombos(x+4, x+12, y-1, y-1);
					}
				}
			}
			else
			{
				if((_walkflag(x+4,y+((bigHitbox||!diagonalMovement)?-1:7),1,SWITCHBLOCK_STATE) || _walkflag(x+12,y+((bigHitbox||!diagonalMovement)?-1:7),1,SWITCHBLOCK_STATE)
					|| ((y<=0) &&
					// Extra checks if Smart Screen Scrolling is enabled
					 (nextcombo_wf(up) || ((get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE)) &&
												   !(tmpscr->flags2&wfUP)) && (nextcombo_solid(up))))))
						&& fall < 0)
				{
					fall = jumping = 0; // Bumped his head
					y -= y.getInt()%8; //fix coords
					// ... maybe on spikes //this is the change from 2.50.1RC3 that Saffith made, that breaks some old quests. -Z
					if ( !get_bit(quest_rules, qr_OLDSIDEVIEWSPIKES) ) //fix for older sideview quests -Z
					{
						checkdamagecombos(x+4, x+12, y-1, y-1);
					}
				}
			}
			
			if(hoverclk > 0)
			{
				if(hoverclk > 0 && !(hoverflags&HOV_INF))
				{
					--hoverclk;
				}
				
				if(!hoverclk && !ladderx && !laddery)
				{
					fall += (zinit.gravity2 / 100);
					hoverflags |= HOV_OUT | HOV_PITFALL_OUT;
				}
			}
			else if(fall+int32_t((zinit.gravity2 / 100)) > 0 && fall<=0 && can_use_item(itype_hoverboots,i_hoverboots) && !ladderx && !laddery && !(hoverflags & HOV_OUT))
			{
				int32_t itemid = current_item_id(itype_hoverboots);
				if(hoverclk < 0)
					hoverclk = -hoverclk;
				else
				{
					fall = jumping = 0;
					if(itemsbuf[itemid].misc1)
						hoverclk = itemsbuf[itemid].misc1;
					else
					{
						hoverclk = 1;
						hoverflags |= HOV_INF;
					}
					
						
					sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
				}
				if(itemsbuf[itemid].wpn)
					decorations.add(new dHover(x, y, dHOVER, 0));
			}
			else if(!ladderx && !laddery && !getOnSideviewLadder() && !IsSideSwim())
			{
				fall += (zinit.gravity2 / 100);
				
			}
		}
	}
	else // Topdown gravity
	{
		z-=fall/(spins && fall>0 ? 200:100);
		if(z>0)
		{
			switch(action)
			{
				case swimming:
				{
					diveclk=0;
					action=walking; FFCore.setLinkAction(walking);
				
					break;
				}
				case waterhold1:
				{
					action=landhold1; FFCore.setLinkAction(landhold1);
					break;
				}
					
				case waterhold2:
				{
					action=landhold2; FFCore.setLinkAction(landhold2);
					break;
				}
				
				default:
					break;
			}
		}
		
		for(int32_t j=0; j<chainlinks.Count(); j++)
		{
			chainlinks.spr(j)->z=z;
		}
		
		if(Lwpns.idFirst(wHookshot)>-1)
		{
			Lwpns.spr(Lwpns.idFirst(wHookshot))->z=z;
		}
		
		if(Lwpns.idFirst(wHSHandle)>-1)
		{
			Lwpns.spr(Lwpns.idFirst(wHSHandle))->z=z;
		}
		
		if(z<=0)
		{
			if(fall > 0)
			{
				if((iswaterex(MAPCOMBO(x,y+8), currmap, currscr, -1, x, y+8, true, false) && ladderx<=0 && laddery<=0) || COMBOTYPE(x,y+8)==cSHALLOWWATER)
					sfx(WAV_ZN1SPLASH,x.getInt());
					
				stomping = true;
			}
			
			z = fall = jumping = 0;
			if(check_pitslide(true) == -1)
			{
				hoverclk = 0;
				hoverflags = 0;
			}
			else if(hoverclk > 0 && !(hoverflags&HOV_INF))
			{
				if(!--hoverclk)
				{
					hoverflags |= HOV_OUT | HOV_PITFALL_OUT;
				}
			}
		}
		else if(fall <= (int32_t)zinit.terminalv)
		{
			if(fall != 0 || hoverclk>0)
				jumping++;
				
			if(hoverclk > 0)
			{
				if(hoverclk > 0 && !(hoverflags&HOV_INF))
				{
					--hoverclk;
				}
				
				if(!hoverclk)
				{
					fall += (zinit.gravity2 / 100);
					hoverflags |= HOV_OUT | HOV_PITFALL_OUT;
				}
			}
			else if(fall+(int32_t)(zinit.gravity2 / 100) > 0 && fall<=0 && can_use_item(itype_hoverboots,i_hoverboots) && !(hoverflags & HOV_OUT))
			{
				if(hoverclk < 0)
					hoverclk = -hoverclk;
				else
				{
					fall = 0;
					int32_t itemid = current_item_id(itype_hoverboots);
					if(itemsbuf[itemid].misc1)
						hoverclk = itemsbuf[itemid].misc1;
					else
					{
						hoverclk = 1;
						hoverflags |= HOV_INF;
					}
					sfx(itemsbuf[current_item_id(itype_hoverboots)].usesound,pan(x.getInt()));
				}
				decorations.add(new dHover(x, y, dHOVER, 0));
			}
			else fall += (zinit.gravity2 / 100);
		}
	}
	
	if(drunkclk)
	{
		--drunkclk;
	}
	
	if(lstunclock > 0)
	{
		// also cancel Link's attack
		attackclk = 0;
		
		if( FFCore.getLinkAction() != stunned )
		{
			tempaction=action; //update so future checks won't do this
			//action=freeze; //setting this makes the player invincible while stunned -V
			FFCore.setLinkAction(stunned);
		}
		--lstunclock;
	}
	//if the stun action is still set in FFCore, but he isn't stunned, then the timer reached 0
	//, so we unfreeze him here, and return him to the action that he had when he was stunned. 
	if ( FFCore.getLinkAction() == stunned && !lstunclock )
	{
		action=tempaction; FFCore.setLinkAction(tempaction);
		//zprint("Unfreezing link to action: %d\n", (int32_t)tempaction);
		//action=none; FFCore.setLinkAction(none);
	}
	
	if( lbunnyclock > 0 )
	{
		--lbunnyclock;
	}
	if(DMaps[currdmap].flags&dmfBUNNYIFNOPEARL)
	{
		int32_t itemid = current_item_id(itype_pearl);
		if(itemid > -1)
		{
			if(lbunnyclock == -1) //cure dmap-caused bunny effect
				lbunnyclock = 0;
		}
		else if(lbunnyclock > -1) //No pearl, force into bunny mode
		{
			lbunnyclock = -1;
		}
	}
	else if(lbunnyclock == -1) //dmap-caused bunny effect
	{
		lbunnyclock = 0;
	}
	
	if(!is_on_conveyor && !(diagonalMovement||NO_GRIDLOCK) && (fall==0 || z>0) && charging==0 && spins<=5
			&& action != gothit)
	{
		switch(dir)
		{
		case up:
		case down:
			x=(x.getInt()+4)&0xFFF8;
			break;
			
		case left:
		case right:
			y=(y.getInt()+4)&0xFFF8;
			break;
		}
	}
	
	if((watch==true) && clockclk)
	{
		--clockclk;
		
		if(!clockclk)
		{
			if(cheat_superman==false)
			{
				setClock(false);
			}
			
			watch=false;
			
			for(int32_t i=0; i<eMAXGUYS; i++)
			{
				for(int32_t zoras=0; zoras<clock_zoras[i]; zoras++)
				{
					addenemy(0,0,i,0);
				}
			}
		}
	}
	
	if(hookshot_frozen==true)
	{
		if(hookshot_used==true)
		{
			if (IsSideSwim()) {action=sideswimfreeze; FFCore.setLinkAction(sideswimfreeze);} 
			else {action=freeze; FFCore.setLinkAction(freeze);} //could be LA_HOOKSHOT for FFCore. -Z
			
			if(pull_link==true)
			{
				sprite *t;
				int32_t i;
				
				for(i=0; i<Lwpns.Count() && (Lwpns.spr(i)->id!=wHSHandle); i++)
				{
					/* do nothing */
				}
				
				t = Lwpns.spr(i);
				
				for(i=0; i<Lwpns.Count(); i++)
				{
					sprite *s = Lwpns.spr(i);
					
					if(s->id==wHookshot)
					{
						if (abs((s->y) - y) >= 1)
						{
							if((s->y)>y)
							{
								y+=4;
								
								if(Lwpns.idFirst(wHSHandle)!=-1)
								{
									t->y+=4;
								}
								
								hs_starty+=4;
							}
							
							if((s->y)<y)
							{
								y-=4;
								
								if(Lwpns.idFirst(wHSHandle)!=-1)
								{
									t->y-=4;
								}
								
								hs_starty-=4;
							}
						}
						else 
						{
							y = (s->y);
						}
						if (abs((s->x) - x) >= 1)
						{
							if((s->x)>x)
							{
								x+=4;
								
								if(Lwpns.idFirst(wHSHandle)!=-1)
								{
									t->x+=4;
								}
								
								hs_startx+=4;
							}
							
							if((s->x)<x)
							{
								x-=4;
								
								if(Lwpns.idFirst(wHSHandle)!=-1)
								{
									t->x-=4;
								}
								
								hs_startx-=4;
							}
						}
						else 
						{
							x = (s->x);
						}
					}
				}
			}
		}
		else
		{
			Lwpns.del(Lwpns.idFirst(wHSHandle));
			reset_hookshot();
		}
		
		if(hs_fix)
		{
			if(dir==up)
			{
				y=int32_t(y+7)&0xF0;
			}
			
			if(dir==down)
			{
				y=int32_t(y+7)&0xF0;
			}
			
			if(dir==left)
			{
				x=int32_t(x+7)&0xF0;
			}
			
			if(dir==right)
			{
				x=int32_t(x+7)&0xF0;
			}
			
			hs_fix=false;
		}
		
	}
	
	if(DrunkrLbtn() && /* !get_bit(quest_rules,qr_SELECTAWPN)*/ !get_bit(quest_rules,qr_NO_L_R_BUTTON_INVENTORY_SWAP))
	{
		selectNextBWpn(SEL_LEFT);
	}
	else if(DrunkrRbtn() && /* !get_bit(quest_rules,qr_SELECTAWPN)*/ !get_bit(quest_rules,qr_NO_L_R_BUTTON_INVENTORY_SWAP))
	{
		selectNextBWpn(SEL_RIGHT);
	}
	else if(rEx3btn() && /* !get_bit(quest_rules,qr_SELECTAWPN)*/ get_bit(quest_rules,qr_USE_EX1_EX2_INVENTORYSWAP))
	{
		selectNextAWpn(SEL_LEFT);
	}
	else if(rEx4btn() && /* !get_bit(quest_rules,qr_SELECTAWPN)*/ get_bit(quest_rules,qr_USE_EX1_EX2_INVENTORYSWAP))
	{
		selectNextAWpn(SEL_RIGHT);
	}
		
	if(rPbtn())
	{
		if( !FFCore.runOnMapScriptEngine() ) //OnMap script replaces the 'onViewMap()' call
			onViewMap();
	}   
	for(int32_t i=0; i<Lwpns.Count(); i++)
	{
		weapon *w = ((weapon*)Lwpns.spr(i));
		
		if(w->id == wArrow || w->id == wBrang || w->id == wCByrna)
			addsparkle(i);
	}
	
	if(Lwpns.idCount(wPhantom))
	{
		addsparkle2(pDINSFIREROCKET,pDINSFIREROCKETTRAIL);
		addsparkle2(pDINSFIREROCKETRETURN,pDINSFIREROCKETTRAILRETURN);
		addsparkle2(pNAYRUSLOVEROCKET1,pNAYRUSLOVEROCKETTRAIL1);
		addsparkle2(pNAYRUSLOVEROCKET2,pNAYRUSLOVEROCKETTRAIL2);
		addsparkle2(pNAYRUSLOVEROCKETRETURN1,pNAYRUSLOVEROCKETTRAILRETURN1);
		addsparkle2(pNAYRUSLOVEROCKETRETURN2,pNAYRUSLOVEROCKETTRAILRETURN2);
	}
	
	// Pay magic cost for Byrna beams
	
	//Byrna needs a secondary timer, for 254+, as do all items that reduce MP on a per-frae basis. Essentially, we will do % divisor == 0 for that. -Z
	if(Lwpns.idCount(wCByrna))
	{
		weapon *ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wCByrna)));
		int32_t itemid = ew->parentitem;
		
		if(!(checkbunny(itemid) && checkmagiccost(itemid)))
		{
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				weapon *w = ((weapon*)Lwpns.spr(i));
				
				if(w->id==wCByrna)
				{
					w->dead=1;
				}
		
			}
			//kill the sound effect for the orbits -Z 14FEB2019
			stop_sfx(itemsbuf[itemid].usesound);
		}
		else paymagiccost(itemid);
	}
	
	if(z==0)
	{
		switchblock_z = 0;
		if(switchblock_offset)
		{
			switchblock_offset=false;
			yofs += 8;
		}
	}
	if(!isSideViewLink())
	{
		int32_t tx = x.getInt()+8,
		    ty = y.getInt()+(bigHitbox?8:12);
		if(!(unsigned(ty)>175 || unsigned(tx) > 255))
		{
			for(int32_t q = 0; q < 3; ++q)
			{
				if(q && !tmpscr2[q-1].valid) continue;
				newcombo const& cmb = combobuf[FFCore.tempScreens[q]->data[COMBOPOS(tx,ty)]];
				if(cmb.type != cCSWITCHBLOCK || !(cmb.usrflags&cflag9)) continue;
				int32_t b = 1;
				if(tx&8) b <<= 2;
				if(ty&8) b <<= 1;
				b |= (b<<4); //check equivalent effect flag too
				if((cmb.walk&b)==b) //solid and effecting
				{
					if(z==0)
					{
						if(cmb.usrflags&cflag10)
						{
							if(!switchblock_offset)
							{
								switchblock_offset=true;
								yofs -= 8;
							}
						}
						else
						{
							if(switchblock_offset)
							{
								switchblock_offset=false;
								yofs += 8;
							}
						}
					}
					if(cmb.attributes[2]>0 && switchblock_z>=0)
					{
						if(z==0)
							switchblock_z = zc_max(switchblock_z,zslongToFix(cmb.attributes[2]));
						else if(SWITCHBLOCK_STATE < zslongToFix(cmb.attributes[2]))
						{
							switchblock_z += zslongToFix(cmb.attributes[2])-SWITCHBLOCK_STATE;
						}
					}
					else switchblock_z = -1;
					break;
				}
			}
		}
	}
	ClearhitLinkUIDs(); //clear them before we advance. 
	checkhit();
	
	if(game->get_life()<=0)
	{
		if ( FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			for(size_t slot = 0; slot < 256; ++slot)
			{
				if(size_t bind = game->get_bottle_slot(slot))
				{
					bottletype const* bt = &QMisc.bottle_types[bind-1];
					if(!(bt->flags&BTFLAG_AUTOONDEATH))
						continue;
					word toFill[3] = { 0 };
					for(size_t q = 0; q < 3; ++q)
					{
						char c = bt->counter[q];
						if(c > -1)
						{
							if(bt->flags & (1<<q))
							{
								toFill[q] = (bt->amount[q]==100)
									? game->get_maxcounter(c)
									: word((game->get_maxcounter(c)/100.0)*bt->amount[q]);
							}
							else toFill[q] = bt->amount[q];
							if(toFill[q] + game->get_counter(c) > game->get_maxcounter(c))
							{
								toFill[q] = game->get_maxcounter(c) - game->get_counter(c);
							}
						}
					}
					if(bt->flags & BTFLAG_CURESWJINX)
						swordclk = 0;
					if(bt->flags & BTFLAG_CUREITJINX)
						itemclk = 0;
					if(word max = std::max(toFill[0], std::max(toFill[1], toFill[2])))
					{
						int32_t itemid = find_bottle_for_slot(slot,true);
						stop_sfx(WAV_ER); //stop heart beep!
						if(itemid > -1)
							sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
						for(size_t q = 0; q < 20; ++q)
							do_death_refill_waitframe();
						double inc = max/60.0; //1 second
						double xtra[3]{ 0 };
						for(size_t q = 0; q < 60; ++q)
						{
							if(!(q%6) && (toFill[0]||toFill[1]||toFill[2]))
								sfx(WAV_MSG); //!TODO Need to make this configurable at some point... -Em
							for(size_t j = 0; j < 3; ++j)
							{
								xtra[j] += inc;
								word f = floor(xtra[j]);
								xtra[j] -= f;
								if(toFill[j] > f)
								{
									toFill[j] -= f;
									game->change_counter(f,bt->counter[j]);
								}
								else if(toFill[j])
								{
									game->change_counter(toFill[j],bt->counter[j]);
									toFill[j] = 0;
								}
							}
							do_death_refill_waitframe();
						}
						for(size_t j = 0; j < 3; ++j)
						{
							if(toFill[j])
							{
								game->change_counter(toFill[j],bt->counter[j]);
								toFill[j] = 0;
							}
						}
						for(size_t q = 0; q < 20; ++q)
							do_death_refill_waitframe();
					}
					game->set_bottle_slot(slot,bt->next_type);
					if(game->get_life() > 0)
					{
						break; //Revived! Stop drinking things...
					}
				}
			}
			if(game->get_life()<=0) //Not saved by fairy
			{
				// So scripts can have one frame to handle hp zero events
				if(false == (last_hurrah = !last_hurrah))
				{
					drunkclk=0;
					lstunclock = 0;
					FFCore.setLinkAction(dying);
					FFCore.deallocateAllArrays(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME);
					FFCore.deallocateAllArrays(SCRIPT_LINK, SCRIPT_LINK_ACTIVE);
					ALLOFF(true,true);
					Playing = false; //Disallow F6
					if(!get_bit(quest_rules,qr_ONDEATH_RUNS_AFTER_DEATH_ANIM))
					{
						FFCore.runOnDeathEngine();
						FFCore.deallocateAllArrays(SCRIPT_LINK, SCRIPT_LINK_DEATH);
					}
					heroDeathAnimation();
					if(get_bit(quest_rules,qr_ONDEATH_RUNS_AFTER_DEATH_ANIM))
					{
						FFCore.runOnDeathEngine();
						FFCore.deallocateAllArrays(SCRIPT_LINK, SCRIPT_LINK_DEATH);
					}
					ALLOFF(true,true);
					return true;
				}
			}
		}
		else //2.50.x
		{
			
			// So scripts can have one frame to handle hp zero events
			if(false == (last_hurrah = !last_hurrah))
			{
				drunkclk=0;
				heroDeathAnimation();
				
				return true;
			}
			
		}
	}
	else last_hurrah=false;
	
	if(swordclk>0)
		--swordclk;
		
	if(itemclk>0)
		--itemclk;
		
	if(inwallm)
	{
		attackclk=0;
		linkstep();
		
		if(CarryLink()==false)
			restart_level();
			
		return false;
	}
	
	if(ewind_restart)
	{
		attackclk=0;
		restart_level();
		xofs=0;
		action=none; FFCore.setLinkAction(none);
		ewind_restart=false;
		return false;
	}
	
	if(hopclk)
	{
		action=hopping; FFCore.setLinkAction(hopping);
	}
	if(fallclk)
	{
		action=falling; FFCore.setLinkAction(falling);
	}
		
	// get user input or do other animation
	freeze_guys=false;										// reset this flag, set it again if holding
	
	if(action != landhold1 && action != landhold2 && action != waterhold1 && action != waterhold2 && action != sidewaterhold1 && action != sidewaterhold2 && fairyclk==0 && holdclk>0)
	{
		holdclk=0;
	}
	
	bool isthissolid = false;
	switch(action)
	{
	case gothit:
		if(attackclk)
			if(!doattack())
			{
				attackclk=spins=0;
				tapping=false;
			}
			
		break;
		
	case drowning:
	case lavadrowning:
	case sidedrowning:
	{
		linkstep(); // maybe this line should be elsewhere?
		
		//!DROWN
		// Helpful comment to find drowning -Dimi
		
		if(--drownclk==0)
		{
			action=none; FFCore.setLinkAction(none);
			int32_t water = iswaterex(MAPCOMBO(x.getInt()+7.5,y.getInt()+12), currmap, currscr, -1, x.getInt()+7.5,y.getInt()+12, true, false);
			int32_t damage = combobuf[water].attributes[0]/10000L;
			//if (damage == 0 && !(combobuf[water].usrflags&cflag7)) damage = (game->get_hp_per_heart()/4);
			if (combobuf[water].type != cWATER) damage = 4;
			game->set_life(vbound(game->get_life()-damage,0, game->get_maxlife()));
			x=entry_x;
			y=entry_y;
			can_mirror_portal = false; //incase entry is on a portal!
			if (canSideviewLadder()) setOnSideviewLadder(true);
			warpx=x;
			warpy=y;
			hclk=48;
		}
		
		break;
	}
	case falling:
	{
		linkstep();
		pitfall();
		break;
	}
	case freeze:
	case sideswimfreeze:
	case scrolling:
		break;
		
	case casting:
	case sideswimcasting:
	{
		if(magicitem==-1)
		{
			action=none; FFCore.setLinkAction(none);
		}
		
		break;
	}
	case landhold1:
	case landhold2:
	{
		if(--holdclk <= 0)
		{
			//restart music
			if(get_bit(quest_rules, qr_HOLDNOSTOPMUSIC) == 0 && (specialcave < GUYCAVE))
				playLevelMusic();
				
			action=none; FFCore.setLinkAction(none);
		}
		else
			freeze_guys=true;
			
		break;
	}
	case waterhold1:
	case waterhold2:
	case sidewaterhold1:
	case sidewaterhold2:
	{
		diveclk=0;
		
		if(--holdclk <= 0)
		{
			//restart music
			if(get_bit(quest_rules, qr_HOLDNOSTOPMUSIC) == 0  && (specialcave < GUYCAVE))
				playLevelMusic();
				
			SetSwim();
		}
		else
			freeze_guys=true;
			
		break;
	}
	case hopping:
	{
		if(DRIEDLAKE)
		{
			action=none; FFCore.setLinkAction(none);
			hopclk = 0;
			diveclk = 0;
			break;
		}
		
		do_hopping();
		break;
	}
	case inwind:
	{
		int32_t i=Lwpns.idFirst(wWind);
		
		if(i<0)
		{
			bool exit=false;
			
			if(whirlwind==255)
			{
				exit=true;
			}
			else if(y<=0 && dir==up) y=-1;
			else if(y>=160 && dir==down) y=161;
			else if(x<=0 && dir==left) x=-1;
			else if(x>=240 && dir==right) x=241;
			else exit=true;
			
			if(exit)
			{
				action=none; FFCore.setLinkAction(none);
				xofs=0;
				whirlwind=0;
				lstep=0;
				if ( dontdraw < 2 ) dontdraw=0;
				entry_x=x;
				entry_y=y;
			}
		}
		/*
			  else if (((weapon*)Lwpns.spr(i))->dead==1)
			  {
				whirlwind=255;
			  }
		*/
		else
		{
			x=Lwpns.spr(i)->x;
			y=Lwpns.spr(i)->y;
			dir=Lwpns.spr(i)->dir;
		}
	}
	break;
	
	case sideswimming:
	case sideswimattacking:
	case sideswimhit:
	case swimhit:
	case swimming:
	{	
		if(DRIEDLAKE)
		{
			action=none; FFCore.setLinkAction(none);
			hopclk=0;
			break;
		}
		
		bool shouldbreak = (action == sideswimhit || action == swimhit); //!DIMITODO: "Can walk while hurt" compat needs to be added here.
		
		if((frame&1) && !shouldbreak)
			linkstep();
		
		if (_walkflag(x+7,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE)
                || _walkflag(x+7,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE)
		|| _walkflag(x+8,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE)
                || _walkflag(x+8,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE)) isthissolid = true;
		if ((get_bit(quest_rules, qr_NO_HOPPING) || CanSideSwim()) && !isthissolid) //Since hopping won't be set with this on, something needs to kick Link out of water...
		{
			if(!iswaterex(MAPCOMBO(x.getInt()+4,y.getInt()+9), currmap, currscr, -1, x.getInt()+4,y.getInt()+9, true, false)||!iswaterex(MAPCOMBO(x.getInt()+4,y.getInt()+15), currmap, currscr, -1, x.getInt()+4,y.getInt()+15, true, false)
			|| !iswaterex(MAPCOMBO(x.getInt()+11,y.getInt()+9), currmap, currscr, -1, x.getInt()+11,y.getInt()+9, true, false)||!iswaterex(MAPCOMBO(x.getInt()+11,y.getInt()+15), currmap, currscr, -1, x.getInt()+11,y.getInt()+15, true, false))
			{
				hopclk=0;
				diveclk=0;
				if (action != sideswimattacking && action != attacking) {action=none; FFCore.setLinkAction(none);}
				else {action=attacking; FFCore.setLinkAction(attacking);}
				hopdir=-1;
			}
		}
		if (shouldbreak) break;
		if (action == swimming || action == sideswimming || action == sideswimattacking)
		{
			int32_t watercheck = iswaterex(MAPCOMBO(x.getInt()+7.5,y.getInt()+12), currmap, currscr, -1, x.getInt()+7.5,y.getInt()+12, true, false);
			if (combobuf[watercheck].usrflags&cflag2)
			{
				if (!(current_item(combobuf[watercheck].attribytes[2]) > 0 && current_item(combobuf[watercheck].attribytes[2]) >= combobuf[watercheck].attribytes[3]))
				{
					onpassivedmg = true;
					if (damageovertimeclk == 0)
					{
						int32_t curhp = game->get_life();
						if (combobuf[watercheck].usrflags&cflag5) game->set_life(vbound(game->get_life()+ringpower(combobuf[watercheck].attributes[1]/10000L), 0, game->get_maxlife())); //Affected by rings
						else game->set_life(vbound(game->get_life()+(combobuf[watercheck].attributes[1]/10000L), 0, game->get_maxlife()));
						if ((combobuf[watercheck].attributes[2]/10000L) && (game->get_life() != curhp || !(combobuf[watercheck].usrflags&cflag6))) sfx(combobuf[watercheck].attributes[2]/10000L);
						if (game->get_life() < curhp && combobuf[watercheck].usrflags&cflag7)
						{
							hclk = 48;
							hitdir = -1;
							if (IsSideSwim()) {action = sideswimhit; FFCore.setLinkAction(sideswimhit);}
							else {action = swimhit; FFCore.setLinkAction(swimhit);}
						}
					}
					if (combobuf[watercheck].attribytes[1] > 0)
					{
						if (damageovertimeclk <= 0 || damageovertimeclk > combobuf[watercheck].attribytes[1]) damageovertimeclk = combobuf[watercheck].attribytes[1];
						else --damageovertimeclk;
					}
					else damageovertimeclk = 0;
				}
				else damageovertimeclk = -1;
			}
			else damageovertimeclk = -1;
			//combobuf[watercheck].attributes[0]
		}
			
		// fall through
	}	
	default:
		movelink();										   // call the main movement routine
	}
	
	// check for ladder removal
	if(diagonalMovement)
	{
		if(ladderx+laddery)
		{
			if(ladderdir==up)
			{
				if((laddery-y.getInt()>=(16+(ladderstart==dir?ladderstart==down?1:0:0))) || (laddery-y.getInt()<=(-16-(ladderstart==dir?ladderstart==up?1:0:0))) || (abs(ladderx-x.getInt())>8))
				{
					reset_ladder();
				}
			}
			else
			{
				if((abs(laddery-y.getInt())>8) || (ladderx-x.getInt()>=(16+(ladderstart==dir?ladderstart==right?1:0:0))) || (ladderx-x.getInt()<=(-16-(ladderstart==dir?ladderstart==left?1:0:0))))
				{
					reset_ladder();
				}
			}
		}
	}
	else
	{
		if((abs(laddery-y.getInt())>=16) || (abs(ladderx-x.getInt())>=16))
		{
			reset_ladder();
		}
	}
	
	if(ilswim)
		landswim++;
	else landswim=0;
	
	if(hopclk!=0xFF) ilswim=false;
	
	if((!loaded_guys) && (frame - newscr_clk >= 1))
	{
		if(tmpscr->room==rGANON)
		{
			ganon_intro();
		}
		else
		{
			loadguys();
		}
	}
	
	if((!loaded_enemies) && (frame - newscr_clk >= 2))
	{
		loadenemies();
	}
	
	// check lots of other things
	checkscroll();
	
	if(action!=inwind && action!=drowning && action != sidedrowning && action!=lavadrowning)
	{
		checkspecial();
		checkitems();
		checklocked(); //This has issues if Link's action is WALKING, in 8-way moveent. 
		checklockblock();
		checkbosslockblock();
		if(get_bit(quest_rules,qr_OLD_CHEST_COLLISION))
		{
			oldcheckchest(cCHEST);
			oldcheckchest(cLOCKEDCHEST);
			oldcheckchest(cBOSSCHEST);
		}
		checkpushblock();
		checkswordtap();
		
		if(hookshot_frozen==false)
		{
			checkspecial2(&lsave);
		}
		
		if(action==won)
		{
			return true;
		}
	}
	
	// Somehow Link was displaced from the fairy flag...
	if(fairyclk && action != freeze && action != sideswimfreeze)
	{
		fairyclk = holdclk = refill_why = 0;
	}
	
	if((!activated_timed_warp) && (tmpscr->timedwarptics>0))
	{
		tmpscr->timedwarptics--;
		
		if(tmpscr->timedwarptics==0)
		{
			activated_timed_warp=true;
			
			if(tmpscr->flags4 & fTIMEDDIRECT)
			{
				didpit=true;
				pitx=x;
				pity=y;
			}
			
			int32_t index2 = 0;
			
			if(tmpscr->flags5 & fRANDOMTIMEDWARP) index2=zc_oldrand()%4;
			
			sdir = dir;
			dowarp(1,index2);
		}
	}
	
	bool awarp = false;
	//!DIMI: Global Combo Effects (AUTO STUFF)
	for(int32_t i=0; i<176; i++)
	{
		for(int32_t layer=0; layer<=2; layer++)
		{
			if (layer == 1 && !get_bit(quest_rules,qr_AUTOCOMBO_LAYER_1)) continue;
			if (layer == 2 && !get_bit(quest_rules,qr_AUTOCOMBO_LAYER_2)) continue;
			int32_t ind=0;
			
			if(!awarp) //Putting stuff in here so it doesn't activate after an autowarp happens.
			{
				//AUTOMATIC TRIGGER CODE
				int32_t cid = ( layer ) ? MAPCOMBOL(layer,MAPCOMBOX(i),MAPCOMBOY(i)) : MAPCOMBO(MAPCOMBOX(i),MAPCOMBOY(i));
				newcombo const& cmb = combobuf[cid];
				if (cmb.triggerflags[1]&combotriggerAUTOMATIC)
				{
					int32_t flag = ( layer ) ? MAPFLAGL(layer, MAPCOMBOX(i),MAPCOMBOY(i)) : MAPFLAG(MAPCOMBOX(i),MAPCOMBOY(i));
					int32_t flag2 = ( layer ) ? MAPCOMBOFLAGL(layer,MAPCOMBOX(i),MAPCOMBOY(i)): MAPCOMBOFLAG(MAPCOMBOX(i),MAPCOMBOY(i));
					int32_t ft = cmb.attribytes[3];
					int32_t scombo=COMBOPOS(MAPCOMBOX(i),MAPCOMBOY(i));
					bool single16 = false;
					if ( cmb.type >= cSCRIPT1 && cmb.type <= cTRIGGERGENERIC )
					{
						do_generic_combo2(MAPCOMBOX(i),MAPCOMBOY(i), cid, flag, flag2, ft, scombo, single16, layer);
					}
					else if( cmb.type == cCSWITCH )
					{
						//byte* grid = (layer ? w->wscreengrid_layer[layer-1] : w->wscreengrid);
						//if (get_bit(grid,(((bx>>4) + by)))) return;
						//set_bit(grid,(((bx>>4) + by)),1);
						do_cswitch_combo2(cmb, layer, COMBOPOS(MAPCOMBOX(i),MAPCOMBOY(i)));
					}
					if (cmb.triggerflags[1]&combotriggerSECRETS)
					{
						//byte* grid = (layer ? w->wscreengrid_layer[layer-1] : w->wscreengrid);
						//if (get_bit(grid,(((bx>>4) + by)))) return;
						//set_bit(grid,(((bx>>4) + by)),1);
						hidden_entrance(0, true, false, -6);
					}
				}
				
				//AUTO WARP CODE
				if(cmb.type==cAWARPA)
				{
					awarp=true;
					ind=0;
				}
				else if(cmb.type==cAWARPB)
				{
					awarp=true;
					ind=1;
				}
				else if(cmb.type==cAWARPC)
				{
					awarp=true;
					ind=2;
				}
				else if(cmb.type==cAWARPD)
				{
					awarp=true;
					ind=3;
				}
				else if(cmb.type==cAWARPR)
				{
					awarp=true;
					ind=zc_oldrand()%4;
				}
				
				if(awarp)
				{
					if(tmpscr->flags5&fDIRECTAWARP)
					{
						didpit=true;
						pitx=x;
						pity=y;
					}
					
					sdir = dir;
					dowarp(1,ind);
				}
			}
		}
	}
	
	awarp=false;
	
	for(int32_t i=0; i<32; i++)
	{
		int32_t ind=0;
		
		if(!awarp)
		{
			if(combobuf[tmpscr->ffdata[i]].type==cAWARPA)
			{
				awarp=true;
				ind=0;
			}
			else if(combobuf[tmpscr->ffdata[i]].type==cAWARPB)
			{
				awarp=true;
				ind=1;
			}
			else if(combobuf[tmpscr->ffdata[i]].type==cAWARPC)
			{
				awarp=true;
				ind=2;
			}
			else if(combobuf[tmpscr->ffdata[i]].type==cAWARPD)
			{
				awarp=true;
				ind=3;
			}
			else if(combobuf[tmpscr->ffdata[i]].type==cAWARPR)
			{
				awarp=true;
				ind=zc_oldrand()%4;
			}
			
			if(awarp)
			{
				if(tmpscr->flags5&fDIRECTAWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir = dir;
				dowarp(1,ind);
			}
		}
		
	}
	
	if(ffwarp)
	{
		if(ffpit)
		{
			ffpit=false;
			didpit=true;
			pitx=x;
			pity=y;
		}
		
		ffwarp=false;
		dowarp(1,0);
	}
	
	//Link->WarpEx
	if ( FFCore.warpex[wexActive] )
	{
		if(DEVLOGGING) zprint("Running warpex from Link.cpp\n");
		FFCore.warpex[wexActive] = 0;
		int32_t temp_warpex[wexActive] = {0}; //to hold the values as we clear the FFCore array. -Z
		for ( int32_t q = 0; q < wexActive; q++ ) 
		{
			temp_warpex[q] = FFCore.warpex[q];
			FFCore.warpex[q] = 0;
		}
		FFCore.warp_link( temp_warpex[wexType], temp_warpex[wexDMap], temp_warpex[wexScreen], temp_warpex[wexX],
			temp_warpex[wexY], temp_warpex[wexEffect], temp_warpex[wexSound], temp_warpex[wexFlags], temp_warpex[wexDir]); 
	}
	
	// walk through bombed doors and fake walls
	bool walk=false;
	int32_t dtype=dBOMBED;
	
	if(pushing>=24) dtype=dWALK;
	
	if(isdungeon() && action!=freeze && action != sideswimfreeze && loaded_guys && !inlikelike && !diveclk && action!=rafting && !lstunclock)
	{
		if(((dtype==dBOMBED)?DrunkUp():dir==up) && ((diagonalMovement||NO_GRIDLOCK)?x>112&&x<128:x==120) && y<=32 && tmpscr->door[0]==dtype)
		{
			walk=true;
			dir=up;
		}
		
		if(((dtype==dBOMBED)?DrunkDown():dir==down) && ((diagonalMovement||NO_GRIDLOCK)?x>112&&x<128:x==120) && y>=128 && tmpscr->door[1]==dtype)
		{
			walk=true;
			dir=down;
		}
		
		if(((dtype==dBOMBED)?DrunkLeft():dir==left) && x<=32 && ((diagonalMovement||NO_GRIDLOCK)?y>72&&y<88:y==80) && tmpscr->door[2]==dtype)
		{
			walk=true;
			dir=left;
		}
		
		if(((dtype==dBOMBED)?DrunkRight():dir==right) && x>=208 && ((diagonalMovement||NO_GRIDLOCK)?y>72&&y<88:y==80) && tmpscr->door[3]==dtype)
		{
			walk=true;
			dir=right;
		}
	}
	
	if(walk)
	{
		hclk=0;
		drawguys=false;
		
		if(dtype==dWALK)
		{
			sfx(tmpscr->secretsfx);
		}
		
		action=none; FFCore.setLinkAction(none);
		stepforward(29, true);
		action=scrolling; FFCore.setLinkAction(scrolling);
		pushing=false;
	}
	
	if( game->get_life() <= (game->get_hp_per_heart()) && !(game->get_maxlife() <= (game->get_hp_per_heart())) && (heart_beep_timer > -3))
	{
		if(heart_beep)
		{
			cont_sfx(WAV_ER);
		}
		else
		{
			if ( heart_beep_timer == -1 )
			{
				heart_beep_timer = 70;
			}
			
			if ( heart_beep_timer > 0 )
			{
				--heart_beep_timer;
				cont_sfx(WAV_ER);
			}
			else
			{
				stop_sfx(WAV_ER);
			}
		}
	}
	else 
	{
	if ( heart_beep_timer > -2 )
	{
		heart_beep_timer = -1;
		stop_sfx(WAV_ER);
	}
	}
	
	if(rSbtn())
	{
		int32_t tmp_subscr_clk = frame;
		
		switch(lsave)
		{
		case 0:
		{
			if( FFCore.runActiveSubscreenScriptEngine() )
			{
				break;
			}
			else if ( !stopSubscreenFalling() )
			{
				conveyclk=3;
				dosubscr(&QMisc);
				newscr_clk += frame - tmp_subscr_clk;
			}
			break;
		}
			
			
		case 1:
			save_game((tmpscr->flags4&fSAVEROOM) != 0, 0);
			break;
			
		case 2:
			save_game((tmpscr->flags4&fSAVEROOM) != 0, 1);
			break;
		}
	}
	
	if (!checkstab() )
	{
		/*
		for(int32_t q=0; q<176; q++)
			{
				set_bit(screengrid,q,0); 
			}
			
			for(int32_t q=0; q<32; q++)
				set_bit(ffcgrid, q, 0);
		*/
	}
	
	check_conveyor();
	PhantomsCleanup();
	//Try to time the hammer pound so that Link can;t change direction while it occurs. 
	if(attack==wHammer)
	{
		if(attackclk==12 && z==0 && sideviewhammerpound())
	{
		switch(dir) //Link's dir
		{
			case up:
				decorations.add(new dHammerSmack(x-1, y-4, dHAMMERSMACK, 0));
				break;
				
			case down:
				decorations.add(new dHammerSmack(x+8, y+28, dHAMMERSMACK, 0));
				break;
				
			case left:
				decorations.add(new dHammerSmack(x-13, y+14, dHAMMERSMACK, 0));
				break;
				
			case right:
				decorations.add(new dHammerSmack(x+21, y+14, dHAMMERSMACK, 0));
				break;
		}
			
	}
	
	}
	
	handleSpotlights();
	
	if(getOnSideviewLadder())
	{
		if(!canSideviewLadder() || jumping<0 || fall!=0)
		{
			setOnSideviewLadder(false);
		}
		else if(CANFORCEFACEUP)
		{
			setDir(up);
		}
	}
	
	return false;
}

// A routine used exclusively by startwpn,
// to switch Link's weapon if his current weapon (bombs) was depleted.
void LinkClass::deselectbombs(int32_t super)
{
    if ( get_bit(quest_rules,qr_NEVERDISABLEAMMOONSUBSCREEN) || itemsbuf[game->forced_awpn].family == itype_bomb || itemsbuf[game->forced_bwpn].family == itype_bomb || itemsbuf[game->forced_xwpn].family == itype_bomb || itemsbuf[game->forced_ywpn].family == itype_bomb) return;
    if(getItemFamily(itemsbuf,Bwpn&0x0FFF)==(super? itype_sbomb : itype_bomb) && (directWpn<0 || Bwpn==directWpn))
    {
        int32_t temp = selectWpn_new(SEL_VERIFY_LEFT, game->bwpn, game->awpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
        Bwpn = Bweapon(temp);
        directItemB = directItem;
        game->bwpn = temp;
    }
    
    else if (getItemFamily(itemsbuf,Xwpn&0x0FFF)==(super? itype_sbomb : itype_bomb) && (directWpn<0 || Xwpn==directWpn))
    {
        int32_t temp = selectWpn_new(SEL_VERIFY_LEFT, game->xwpn, game->bwpn, game->awpn, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
        Xwpn = Bweapon(temp);
        directItemX = directItem;
        game->xwpn = temp;
    }
    else if (getItemFamily(itemsbuf,Ywpn&0x0FFF)==(super? itype_sbomb : itype_bomb) && (directWpn<0 || Ywpn==directWpn))
    {
        int32_t temp = selectWpn_new(SEL_VERIFY_LEFT, game->ywpn, game->bwpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1, game->awpn);
        Ywpn = Bweapon(temp);
        directItemY = directItem;
        game->ywpn = temp;
    }
    else
    {
        int32_t temp = selectWpn_new(SEL_VERIFY_LEFT, game->awpn, game->bwpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
        Awpn = Bweapon(temp);
        directItemA = directItem;
        game->awpn = temp;
    }
}

int32_t potion_life=0;
int32_t potion_magic=0;

void LinkClass::doMirror(int32_t mirrorid)
{
	if(z > 0) return; //No mirror in air
	if(mirrorid < 0)
		mirrorid = current_item_id(itype_mirror);
	if(mirrorid < 0) return;
	
	if(!(checkbunny(mirrorid) && checkmagiccost(mirrorid)))
		return;
	itemdata const& mirror = itemsbuf[mirrorid];
	if(DMaps[currdmap].flags & dmfMIRRORCONTINUE)
	{
		paymagiccost(mirrorid);
		sfx(mirror.usesound);
		
		doWarpEffect(mirror.misc1, true);
		if(mirror.flags & ITEM_FLAG2) //Act as F6->Continue
		{
			Quit = qCONT;
			skipcont = 1;
		}
		else //Act as Farore's Wind
		{
            int32_t nayrutemp=nayruitem;
            restart_level();
            nayruitem=nayrutemp;
            magicitem=-1;
            magiccastclk=0;
            if ( Link.getDontDraw() < 2 ) { Link.setDontDraw(0); }
		}
	}
	else
	{
		int32_t destdmap = DMaps[currdmap].mirrorDMap;
		int32_t offscr = currscr - DMaps[currdmap].xoff;
		if(destdmap < 0)
			return;
		int32_t destscr = DMaps[destdmap].xoff + offscr;
		if((offscr%16 != destscr%16) || unsigned(destscr) >= 0x80)
			return;
		paymagiccost(mirrorid);
		
		//Store some values to restore if 'warp fails'
		int32_t tLastEntrance = lastentrance,
				tLastEntranceDMap = lastentrance_dmap,
				tContScr = game->get_continue_scrn(),
				tContDMap = game->get_continue_dmap(),
				tPortalDMap = game->portalsrcdmap;
		game->portalsrcdmap = -1;
		int32_t sourcescr = currscr, sourcedmap = currdmap;
		zfix tx = x, ty = y, tz = z;
		action = none; FFCore.setLinkAction(none);
		
		//Warp to new dmap
		FFCore.warp_link(wtIWARP, destdmap, destscr, -1, -1, mirror.misc1,
			mirror.usesound, warpFlagNOSTEPFORWARD, -1);
		
		//Check for valid landing location
		WalkflagInfo info = walkflag(x,y+(bigHitbox?0:8),2,up);
		if(blockmoving)
			info = info || walkflagMBlock(x+8,y+(bigHitbox?0:8));
		execute(info);
		bool fail = info.isUnwalkable();
		
		if(!fail) //not solid, but check for water/pits...
		{
			//{ Check water collision.... GAAAAAAAH THIS IS A MESS
			int32_t water = 0;
			int32_t types[4] = {0};
			int32_t x1 = x+4, x2 = x+11,
				y1 = y+9, y2 = y+15;
			if (get_bit(quest_rules, qr_SMARTER_WATER))
			{
				if (iswaterex(0, currmap, currscr, -1, x1, y1, true, false) &&
				iswaterex(0, currmap, currscr, -1, x1, y2, true, false) &&
				iswaterex(0, currmap, currscr, -1, x2, y1, true, false) &&
				iswaterex(0, currmap, currscr, -1, x2, y2, true, false)) water = iswaterex(0, currmap, currscr, -1, (x2+x1)/2,(y2+y1)/2, true, false);
			}
			else
			{
				types[0] = COMBOTYPE(x1,y1);
				
				if(MAPFFCOMBO(x1,y1))
					types[0] = FFCOMBOTYPE(x1,y1);
					
				types[1] = COMBOTYPE(x1,y2);
				
				if(MAPFFCOMBO(x1,y2))
					types[1] = FFCOMBOTYPE(x1,y2);
					
				types[2] = COMBOTYPE(x2,y1);
				
				if(MAPFFCOMBO(x2,y1))
					types[2] = FFCOMBOTYPE(x2,y1);
					
				types[3] = COMBOTYPE(x2,y2);
				
				if(MAPFFCOMBO(x2,y2))
					types[3] = FFCOMBOTYPE(x2,y2);
					
				int32_t typec = COMBOTYPE((x2+x1)/2,(y2+y1)/2);
				if(MAPFFCOMBO((x2+x1)/2,(y2+y1)/2))
					typec = FFCOMBOTYPE((x2+x1)/2,(y2+y1)/2);
					
				if(combo_class_buf[types[0]].water && combo_class_buf[types[1]].water &&
						combo_class_buf[types[2]].water && combo_class_buf[types[3]].water && combo_class_buf[typec].water)
					water = typec;
			}
			if(water > 0)
			{
				if(current_item(itype_flippers) <= 0 || current_item(itype_flippers) < combobuf[water].attribytes[0] || ((combobuf[water].usrflags&cflag1) && !(itemsbuf[current_item_id(itype_flippers)].flags & ITEM_FLAG3))) 
				{
					fail = true;
				}
			}
			//}
			if(pitslide() || fallclk)
				fail = true;
			fallclk = 0;
		}
		if(fail) //Invalid landing, warp back!
		{
			action = none; FFCore.setLinkAction(none);
			lastentrance = tLastEntrance;
			lastentrance_dmap = tLastEntranceDMap;
			game->set_continue_scrn(tContScr);
			game->set_continue_dmap(tContDMap);
			x = tx;
			y = ty;
			z = tz;
			game->portalsrcdmap = tPortalDMap;
			FFCore.warp_link(wtIWARP, sourcedmap, sourcescr, -1, -1, mirror.misc1,
				mirror.usesound, warpFlagNOSTEPFORWARD, -1);
		}
		else if(mirror.flags & ITEM_FLAG1) //Place portal!
		{
			//Place the portal
			game->set_portal(sourcedmap, destdmap, offscr, x.getZLong(), y.getZLong(), mirror.usesound, mirror.misc1, mirror.wpn);
			//Since it was placed after loading this screen, load the portal object now
			game->load_portal();
			//Don't immediately trigger the warp back
			can_mirror_portal = false;
			
			//Set continue point
			if(currdmap != game->get_continue_dmap())
			{
				game->set_continue_scrn(DMaps[currdmap].cont + DMaps[currdmap].xoff);
			}
			game->set_continue_dmap(currdmap);
			lastentrance_dmap = currdmap;
			lastentrance = game->get_continue_scrn();
		}
	}
}
bool LinkClass::startwpn(int32_t itemid)
{
	if(itemid < 0) return false;
	
	if(((dir==up && y<24) || (dir==down && y>128) ||
			(dir==left && x<32) || (dir==right && x>208)) && !(get_bit(quest_rules,qr_ITEMSONEDGES) || inlikelike))
		return false;
		
	int32_t wx=x;
	int32_t wy=y;
	int32_t wz=z;
	bool ret = true;
	
	switch(dir)
	{
	case up:
		wy-=16;
		break;
		
	case down:
		wy+=16;
		break;
		
	case left:
		wx-=16;
		break;
		
	case right:
		wx+=16;
		break;
	}
	if (IsSideSwim() && (itemsbuf[itemid].flags & ITEM_SIDESWIM_DISABLED)) return false;
	
	bool use_hookshot=true;
	
	switch(itemsbuf[itemid].family)
	{
		case itype_potion:
		{
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			
			if(itemsbuf[itemid].misc1 || itemsbuf[itemid].misc2)
			{
				refill_what=REFILL_ALL;
				refill_why=itemid;
				StartRefill(REFILL_ALL);
				potion_life = game->get_life();
				potion_magic = game->get_magic();
				
				//add a quest rule or an item option that lets you specify whether or not to pause music during refilling
				//music_pause();
				stop_sfx(WAV_ER); //stop heart beep!
				while(refill())
				{
					do_refill_waitframe();
				}
				
				//add a quest rule or an item option that lets you specify whether or not to pause music during refilling
				//music_resume();
				ret = false;
			}
			
			break;
		}
		case itype_bottle:
		{
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
			if(itemsbuf[itemid].script!=0 && item_doscript[itemid])
				return false;
			
			size_t bind = game->get_bottle_slot(itemsbuf[itemid].misc1);
			bool paidmagic = false;
			if(itemsbuf[itemid].script)
			{
				paidmagic = true;
				paymagiccost(itemid);
			}
			
			if(itemsbuf[itemid].script)
			{
				ri = &(itemScriptData[itemid]);
				for ( int32_t q = 0; q < 1024; q++ )
					item_stack[itemid][q] = 0xFFFF;
				ri->Clear();
				item_doscript[itemid] = 1;
				itemscriptInitialised[itemid] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid);
				bind = game->get_bottle_slot(itemsbuf[itemid].misc1);
			}
			bottletype const* bt = bind ? &(QMisc.bottle_types[bind-1]) : NULL;
			if(bt)
			{
				word toFill[3] = { 0 };
				for(size_t q = 0; q < 3; ++q)
				{
					char c = bt->counter[q];
					if(c > -1)
					{
						if(bt->flags & (1<<q))
						{
							toFill[q] = (bt->amount[q]==100)
								? game->get_maxcounter(c)
								: word((game->get_maxcounter(c)/100.0)*bt->amount[q]);
						}
						else toFill[q] = bt->amount[q];
						if(toFill[q] + game->get_counter(c) > game->get_maxcounter(c))
						{
							toFill[q] = game->get_maxcounter(c) - game->get_counter(c);
						}
					}
				}
				word max = std::max(toFill[0], std::max(toFill[1], toFill[2]));
				bool run = max > 0;
				if(get_bit(quest_rules, qr_NO_BOTTLE_IF_ANY_COUNTER_FULL))
					run = ((bt->counter[0] > -1 && !toFill[0]) || (bt->counter[1] > -1 && !toFill[1]) || (bt->counter[2] > -1 && !toFill[2]));
				else
				{
					if((bt->flags & BTFLAG_CURESWJINX) && swordclk)
						run = true;
					else if((bt->flags & BTFLAG_CUREITJINX) && itemclk)
						run = true;
				}
				if(run || (bt->flags&BTFLAG_ALLOWIFFULL))
				{
					if(bt->flags & BTFLAG_CURESWJINX)
						swordclk = 0;
					if(bt->flags & BTFLAG_CUREITJINX)
						itemclk = 0;
					if(!paidmagic)
						paymagiccost(itemid);
					stop_sfx(WAV_ER); //stop heart beep!
					sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
					for(size_t q = 0; q < 20; ++q)
						do_refill_waitframe();
					double inc = max/60.0; //1 second
					double xtra[3]{ 0 };
					for(size_t q = 0; q < 60; ++q)
					{
						if(!(q%6) && (toFill[0]||toFill[1]||toFill[2]))
							sfx(WAV_MSG); //!TODO Need to make this configurable at some point... -Em
						for(size_t j = 0; j < 3; ++j)
						{
							xtra[j] += inc;
							word f = floor(xtra[j]);
							xtra[j] -= f;
							if(toFill[j] > f)
							{
								toFill[j] -= f;
								game->change_counter(f,bt->counter[j]);
							}
							else if(toFill[j])
							{
								game->change_counter(toFill[j],bt->counter[j]);
								toFill[j] = 0;
							}
						}
						do_refill_waitframe();
					}
					for(size_t j = 0; j < 3; ++j)
					{
						if(toFill[j])
						{
							game->change_counter(toFill[j],bt->counter[j]);
							toFill[j] = 0;
						}
					}
					for(size_t q = 0; q < 20; ++q)
						do_refill_waitframe();
					game->set_bottle_slot(itemsbuf[itemid].misc1, bt->next_type);
				}
			}
			
			dowpn = -1;
			ret = false;
			break;
		}
		
		case itype_mirror:
			doMirror(itemid);
			if(Quit)
				return false;
			ret = false;
			break;
		
		case itype_rocs:
		{
			if(!inlikelike && charging==0)
			{
				bool standing = isStanding(true);
				if(standing || extra_jump_count < itemsbuf[itemid].misc1)
				{
					if(!(checkbunny(itemid) && checkmagiccost(itemid)))
						return false;
					
					paymagiccost(itemid);
					
					
					if(!standing)
					{
						++extra_jump_count;
						fall = 0;
						if(hoverclk > 0)
							hoverclk = -hoverclk;
					}
					if(itemsbuf[itemid].flags & ITEM_FLAG1)
						setFall(fall - itemsbuf[itemid].power);
					else
					{
						setFall(fall - (FEATHERJUMP*(itemsbuf[itemid].power+2)));
					}
					
					setOnSideviewLadder(false);
					
					// Reset the ladder, unless on an unwalkable combo
					if((ladderx || laddery) && !(_walkflag(ladderx,laddery,0,SWITCHBLOCK_STATE)))
						reset_ladder();
						
					sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
					//zprint2("fall is: %d\n", (int32_t)fall);
				}
			}
			
			ret = false;
		}
		break;
		
		case itype_letter:
		{
			if(current_item(itype_letter)==i_letter &&
					tmpscr[currscr<128?0:1].room==rP_SHOP &&
					tmpscr[currscr<128?0:1].guy &&
					((currscr<128&&!(DMaps[currdmap].flags&dmfGUYCAVES))
						||(currscr>=128&&DMaps[currdmap].flags&dmfGUYCAVES)) &&
					checkbunny(itemid)
				)
			{
				int32_t usedid = getItemID(itemsbuf, itype_letter,i_letter+1);
				
				if(usedid != -1)
					getitem(usedid, true);
					
				sfx(tmpscr[currscr<128?0:1].secretsfx);
				setupscreen();
				action=none; FFCore.setLinkAction(none);
			}
			
			ret = false;
		}
		break;
		
		case itype_whistle:
		{
			bool whistleflag;
			
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			sfx(itemsbuf[itemid].usesound);
			
			if(dir==up || dir==right)
				++blowcnt;
			else
				--blowcnt;
				
			while(sfx_allocated(itemsbuf[itemid].usesound))
			{
				advanceframe(true);
				
				if(Quit)
					return false;
			}
			
			Lwpns.add(new weapon(x,y,z,wWhistle,0,0,dir,itemid,getUID(),false,0,1,0));
			
			if(whistleflag=findentrance(x,y,mfWHISTLE,get_bit(quest_rules, qr_PERMANENT_WHISTLE_SECRETS)))
				didstuff |= did_whistle;
				
			if((didstuff&did_whistle && itemsbuf[itemid].flags&ITEM_FLAG1) || currscr>=128)
				return false;
				
			if(itemsbuf[itemid].flags&ITEM_FLAG1) didstuff |= did_whistle;
			
			if((tmpscr->flags&fWHISTLE) || (tmpscr->flags7 & fWHISTLEWATER)
					|| (tmpscr->flags7&fWHISTLEPAL))
			{
				whistleclk=0;                                       // signal to start drying lake or doing other stuff
			}
			else
			{
				int32_t where = itemsbuf[itemid].misc1;
				
				if(where>right) where=dir^1;
				
				if(((DMaps[currdmap].flags&dmfWHIRLWIND && TriforceCount()) || DMaps[currdmap].flags&dmfWHIRLWINDRET) &&
						itemsbuf[itemid].misc2 >= 0 && itemsbuf[itemid].misc2 <= 8 && !whistleflag)
					Lwpns.add(new weapon((zfix)(where==left?240:where==right?0:x),
				(zfix)(where==down?0:where==up?160:y),
				(zfix)0,
				wWind,
				0, //type
				0,
				where,
				itemid,getUID(),false,false,true,0)); //last arg is byte special, used to override type for wWind for now. -Z 18JULY2020
										 
				whistleitem=itemid;
			}
			
			ret = false;
		}
		break;
		
		case itype_bomb:
		{
			//Remote detonation
			if(Lwpns.idCount(wLitBomb) >= zc_max(itemsbuf[itemid].misc2,1))
			{
				weapon *ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wLitBomb)));
				
				 while(Lwpns.idCount(wLitBomb) && ew->misc == 0)
				{
					//If this ever needs a version check, in the future. -z
					if ( FFCore.getQuestHeaderInfo(vZelda) > 0x250 || ( FFCore.getQuestHeaderInfo(vZelda) == 0x250 && FFCore.getQuestHeaderInfo(vBuild) > 31 ) )
					{
						if ( ew->power > 1 ) //Don't reduce 1 to 0. -Z
							ew->power *= 0.5; //Remote bombs were dealing double damage. -Z
					}
					ew->misc=50;
					ew->clk=ew->misc-3;
					ew->id=wBomb;
					ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wLitBomb)));
				}
				
				deselectbombs(false);
				return false;
			}
			
			//Remote bombs:
			//Even if you have no bombs, the icon remains so that you can detonate laid bombs.
			//But the remaining code requires at least one bomb.
			if(!game->get_bombs() && !current_item_power(itype_bombbag))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			
			if(!get_debug() && !current_item_power(itype_bombbag))
				game->change_bombs(-1);
				
			if(itemsbuf[itemid].misc1>0) // If not remote bombs
				deselectbombs(false);
				
			if(isdungeon())
			{
				wy=zc_max(wy,16);
			}
			
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wLitBomb,itemsbuf[itemid].fam_type,
								 itemsbuf[itemid].power*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
			sfx(WAV_PLACE,pan(wx));
		}
		break;
		
		case itype_sbomb:
		{
			//Remote detonation
			if(Lwpns.idCount(wLitSBomb) >= zc_max(itemsbuf[itemid].misc2,1))
			{
				weapon *ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wLitSBomb)));
				
				while(Lwpns.idCount(wLitSBomb) && ew->misc == 0)
				{
					ew->misc=50;
					ew->clk=ew->misc-3;
					ew->id=wSBomb;
					ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wLitSBomb)));
				}
				
				deselectbombs(true);
				return false;
			}
			
			//Remote bombs:
			//Even if you have no bombs, the icon remains so that you can detonate laid bombs.
			//But the remaining code requires at least one bomb.
			bool magicbag = (current_item_power(itype_bombbag)
							 && itemsbuf[current_item_id(itype_bombbag)].flags & ITEM_FLAG1);
							 
			if(!game->get_sbombs() && !magicbag)
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			
			if(!get_debug() && !magicbag)
				game->change_sbombs(-1);
				
			if(itemsbuf[itemid].misc1>0) // If not remote bombs
				deselectbombs(true);
				
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wLitSBomb,itemsbuf[itemid].fam_type,itemsbuf[itemid].power*game->get_hero_dmgmult(),dir, itemid,getUID(),false,false,true));
			sfx(WAV_PLACE,pan(wx));
		}
		break;
		
		case itype_wand:
		{
			if(Lwpns.idCount(wMagic))
			{
				misc_internal_link_flags &= ~LF_PAID_WAND_COST;
				return false;
			}
				
			int32_t bookid = current_item_id(itype_book);
			bool paybook = (bookid>-1 && checkbunny(bookid) && checkmagiccost(bookid));
			
			if(!(itemsbuf[itemid].flags&ITEM_FLAG1) && !paybook)  //Can the wand shoot without the book?
			{
				misc_internal_link_flags &= ~LF_PAID_WAND_COST;
				return false;
			}
				
			if(!checkbunny(itemid) || !(misc_internal_link_flags & LF_PAID_WAND_COST || checkmagiccost(itemid)))
				return false;
				
			if(Lwpns.idCount(wBeam))
				Lwpns.del(Lwpns.idFirst(wBeam));
			
			int32_t type, pow;
			if ( get_bit(quest_rules,qr_BROKENBOOKCOST) )
			{
				type = bookid != -1 ? current_item(itype_book) : itemsbuf[itemid].fam_type;
				pow = (bookid != -1 ? current_item_power(itype_book) : itemsbuf[itemid].power)*game->get_hero_dmgmult();
			}
			else
			{
				type = (bookid != -1 && paybook) ? current_item(itype_book) : itemsbuf[itemid].fam_type;
				pow = ((bookid != -1 && paybook) ? current_item_power(itype_book) : itemsbuf[itemid].power)*game->get_hero_dmgmult();
			}
			for(int32_t i=(spins==1?up:dir); i<=(spins==1 ? right:dir); i++)
				if(dir!=(i^1))
			{
				weapon *magic = new weapon((zfix)wx,(zfix)wy,(zfix)wz,wMagic,type,pow,i, itemid,getUID(),false,false,true);
				if(paybook)
					magic->linkedItem = bookid;
				//magic->dir = this->dir; //Save player dir for special weapons. 
				Lwpns.add(magic);
			}
			if(!(misc_internal_link_flags & LF_PAID_WAND_COST))
				paymagiccost(itemid);
			else misc_internal_link_flags &= ~LF_PAID_WAND_COST;
			
			if(paybook)
				paymagiccost(current_item_id(itype_book));
				
			if(bookid != -1)
			{
				if (( itemsbuf[bookid].flags & ITEM_FLAG4 ))
				{
					sfx(itemsbuf[bookid].misc2,pan(wx));
				}
				else 
				{
					sfx(itemsbuf[itemid].usesound,pan(wx));
				}
			}
			else
				sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		/*
		//    Fireball Wand
		Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wRefFireball,0,2*game->get_hero_dmgmult(),dir));
		switch (dir)
		{
		case up:
		  Lwpns.spr(Lwpns.Count()-1)->angle=-PI/2;
		  Lwpns.spr(Lwpns.Count()-1)->dir=up;
		  break;
		case down:
		  Lwpns.spr(Lwpns.Count()-1)->angle=PI/2;
		  Lwpns.spr(Lwpns.Count()-1)->dir=down;
		  break;
		case left:
		  Lwpns.spr(Lwpns.Count()-1)->angle=PI;
		  Lwpns.spr(Lwpns.Count()-1)->dir=left;
		  break;
		case right:
		  Lwpns.spr(Lwpns.Count()-1)->angle=0;
		  Lwpns.spr(Lwpns.Count()-1)->dir=right;
		  break;
		}
		Lwpns.spr(Lwpns.Count()-1)->clk=16;
		((weapon*)Lwpns.spr(Lwpns.Count()-1))->step=3.5;
		Lwpns.spr(Lwpns.Count()-1)->dummy_bool[0]=true; //homing
		*/
		break;
		
		case itype_sword:
		{
			if(!(checkbunny(itemid) || !(misc_internal_link_flags & LF_PAID_SWORD_COST || checkmagiccost(itemid))))
				return false;
				
			if((Lwpns.idCount(wBeam) && spins==0)||Lwpns.idCount(wMagic))
			{
				misc_internal_link_flags &= ~LF_PAID_SWORD_COST;
				return false;
			}
				
			if(!(misc_internal_link_flags & LF_PAID_SWORD_COST))//If already paid to use sword melee, don't charge again
				paymagiccost(itemid);
			else misc_internal_link_flags &= ~LF_PAID_SWORD_COST;
			float temppower;
			
			if(itemsbuf[itemid].flags & ITEM_FLAG2)
			{
				temppower=game->get_hero_dmgmult()*itemsbuf[itemid].power;
				temppower=temppower*itemsbuf[itemid].misc2;
				temppower=temppower/100;
			}
			else
			{
				temppower = game->get_hero_dmgmult()*itemsbuf[itemid].misc2;
			}
			
			//Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wBeam,itemsbuf[itemid].fam_type,int32_t(temppower),dir,itemid,getUID()));
			//Add weapon script to sword beams.
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wBeam,itemsbuf[itemid].fam_type,int32_t(temppower),dir,itemid,getUID(),false,false,true));
			//weapon *w = (weapon*)Lwpns.spr(Lwpns.Count()-1); //the pointer to this beam
			//w->weaponscript = itemsbuf[itemid].weaponscript;
			//w->canrunscript = 0;
			sfx(WAV_BEAM,pan(wx));
		}
		break;
		
		case itype_candle:
		{
			if(itemsbuf[itemid].flags&ITEM_FLAG1 && didstuff&did_candle)
			{
				return false;
			}
			
			if(Lwpns.idCount(wFire)>=2)
			{
				return false;
			}
			
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
			{
				return false;
			}
			
			paymagiccost(itemid);
			
			if(itemsbuf[itemid].flags&ITEM_FLAG1) didstuff|=did_candle;
			
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wFire,
								 //(itemsbuf[itemid].fam_type > 1), //To do with combo flags ... Needs to be changed to fix ->Level for wFire
								 (itemsbuf[itemid].fam_type), //To do with combo flags ... Needs to be changed to fix ->Level for wFire
								 itemsbuf[itemid].power*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
			sfx(itemsbuf[itemid].usesound,pan(wx));
			attack=wFire;
		}
		break;
		
		case itype_script1:
		{
			if(Lwpns.idCount(wScript1))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript1,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script2:
		{
			if(Lwpns.idCount(wScript2))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript2,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script3:
		{
			if(Lwpns.idCount(wScript3))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript3,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script4:
		{
			if(Lwpns.idCount(wScript4))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript4,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script5:
		{
			if(Lwpns.idCount(wScript5))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript5,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script6:
		{
			if(Lwpns.idCount(wScript6))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript6,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script7:
		{
			if(Lwpns.idCount(wScript7))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript7,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script8:
		{
			if(Lwpns.idCount(wScript8))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript8,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script9:
		{
			if(Lwpns.idCount(wScript9))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript9,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_script10:
		{
			if(Lwpns.idCount(wScript10))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wScript10,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_icerod:
		{
			if(Lwpns.idCount(wIce))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
		
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wIce,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step = itemsbuf[itemid].misc1;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		
		break;
		
		case itype_arrow:
		{
			if(Lwpns.idCount(wArrow) > itemsbuf[itemid].misc2)
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
			
			if(get_bit(quest_rules,qr_TRUEARROWS) && !current_item_power(itype_quiver))
			{
				if(game->get_arrows()<=0)
					return false;
					
				game->change_arrows(-1);
			}
			else if(!current_item_power(itype_quiver) && !current_item_power(itype_wallet))
			{
				if(game->get_drupy()+game->get_rupies()<=0)
					return false;
					
				game->change_drupy(-1);
			}
			
			paymagiccost(itemid);
			
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wArrow,itemsbuf[itemid].fam_type,game->get_hero_dmgmult()*itemsbuf[itemid].power,dir,itemid,getUID(),false,false,true));
			((weapon*)Lwpns.spr(Lwpns.Count()-1))->step*=(current_item_power(itype_bow)+1)/2;
			sfx(itemsbuf[itemid].usesound,pan(wx));
		}
		break;
		
		case itype_bait:
			if(Lwpns.idCount(wBait)) //TODO: More than one Bait per screen?
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			sfx(itemsbuf[itemid].usesound,pan(wx));
			
			if(tmpscr->room==rGRUMBLE && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
			{
				items.add(new item((zfix)wx,(zfix)wy,(zfix)0,iBait,ipDUMMY+ipFADE,0));
				fadeclk=66;
				dismissmsg();
				clear_bitmap(pricesdisplaybuf);
				set_clip_state(pricesdisplaybuf, 1);
				//    putscr(scrollbuf,0,0,tmpscr);
				setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
				removeItemsOfFamily(game,itemsbuf,itype_bait);
				verifyBothWeapons();
				sfx(tmpscr->secretsfx);
				return false;
			}
			
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wBait,0,0,dir,itemid,getUID(),false,false,true));
			break;
			
		case itype_brang:
		{
			if(Lwpns.idCount(wBrang) > itemsbuf[itemid].misc2)
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			current_item_power(itype_brang);
			Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wBrang,itemsbuf[itemid].fam_type,(itemsbuf[itemid].power*game->get_hero_dmgmult()),dir,itemid,getUID(),false,false,true));
		}
		break;
		
		case itype_hookshot:
			if(inlikelike || Lwpns.idCount(wHookshot))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			
			for(int32_t i=-1; i<2; i++)
			{
				if(dir==up)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x,y-7)])||
							(_walkflag(x+2,y+4,1,SWITCHBLOCK_STATE) && !ishookshottable(x.getInt(),int32_t(y+4))))
					{
						use_hookshot=false;
					}
				}
				else if(dir==down)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+12,y+23)]))
					{
						use_hookshot=false;
					}
				}
				else if(dir==left)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x-7,y+12)]))
					{
						use_hookshot=false;
					}
				}
				else if(dir==right)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+23,y+12)]))
					{
						use_hookshot=false;
					}
				}
				//Diagonal Hookshot (6)
				else if(dir==r_down)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+9,y+13)]))
					{
						use_hookshot=false;
					}
				}
				else if(dir==l_down)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+6,y+13)]))
					{
						use_hookshot=false;
					}
				}
				else if(dir==r_up)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+9,y+13)]))
					{
						use_hookshot=false;
					}
				}
				else if(dir==l_up)
				{
					if(isHSGrabbable(combobuf[MAPCOMBO2(i,x+6,y+13)]))
					{
						use_hookshot=false;
					}
				}
			}
			
			if(use_hookshot)
			{
				int32_t hookitem = itemsbuf[itemid].fam_type;
				int32_t hookpower = itemsbuf[itemid].power;
				byte allow_diagonal = (itemsbuf[itemid].flags & ITEM_FLAG2) ? 1 : 0; 
			
				if(!Lwpns.has_space())
				{
					Lwpns.del(0);
				}
				
				if(!Lwpns.has_space(2))
				{
					Lwpns.del(0);
				}
				
				if(dir==up)
				{
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)wx,(zfix)wy-4,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx;
					hs_starty=wy-4;
				}
				
				if(dir==down)
				{
					int32_t offset=get_bit(quest_rules,qr_HOOKSHOTDOWNBUG)?4:0;
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy+offset,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)wx,(zfix)wy+offset,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx;
					hs_starty=wy;
				}
				
				if(dir==left)
				{
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx-4),(zfix)wy,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx-4;
					hs_starty=wy;
				}
				
				if(dir==right)
				{
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx+4),(zfix)wy,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx+4;
					hs_starty=wy;
				}
				//Diagonal Hookshot (7)
				if(dir==r_down)
				{
					hookshot_used=true;
					int32_t offset=get_bit(quest_rules,qr_HOOKSHOTDOWNBUG)?4:0;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy+offset,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx+4),(zfix)wy+offset,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx+4;
					hs_starty=wy;
				}
				if(dir==r_up)
				{
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx+4),(zfix)wy,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx+4;
					hs_starty=wy;
				}
				if(dir==l_down)
				{
					hookshot_used=true;
					int32_t offset=get_bit(quest_rules,qr_HOOKSHOTDOWNBUG)?4:0;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy+offset,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx-4),(zfix)wy+offset,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx+4;
					hs_starty=wy;
				}
				if(dir==l_up)
				{
					hookshot_used=true;
					Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wHSHandle,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					Lwpns.add(new weapon((zfix)(wx-4),(zfix)wy,(zfix)wz,wHookshot,hookitem,
										 hookpower*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					hs_startx=wx+4;
					hs_starty=wy;
				}
				hookshot_frozen=true;
			}
			
			break;
			
		case itype_dinsfire:
			if(z!=0 || (isSideViewLink() && !(on_sideview_solid(x,y) || getOnSideviewLadder() || IsSideSwim())))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			if (IsSideSwim()) {action=sideswimcasting; FFCore.setLinkAction(sideswimcasting);}
			else {action=casting; FFCore.setLinkAction(casting);}
			magicitem=itemid;
			break;
			
		case itype_faroreswind:
			if(z!=0 || (isSideViewLink() && !(on_sideview_solid(x,y) || getOnSideviewLadder() || IsSideSwim())))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			if (IsSideSwim()) {action=sideswimcasting; FFCore.setLinkAction(sideswimcasting);}
			else {action=casting; FFCore.setLinkAction(casting);}
			magicitem=itemid;
			break;
			
		case itype_nayruslove:
			if(z!=0 || (isSideViewLink() && !(on_sideview_solid(x,y) || getOnSideviewLadder() || IsSideSwim())))
				return false;
				
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
				return false;
				
			paymagiccost(itemid);
			if (IsSideSwim()) {action=sideswimcasting; FFCore.setLinkAction(sideswimcasting);}
			else {action=casting; FFCore.setLinkAction(casting);}
			magicitem=itemid;
			break;
			
		case itype_cbyrna:
		{
			//Beams already deployed
			if(Lwpns.idCount(wCByrna))
			{
				stopCaneOfByrna();
				return false;
			}
			
			if(!(checkbunny(itemid) && checkmagiccost(itemid)))
			{
				stop_sfx(itemsbuf[itemid].usesound); //if we can't pay the cost, kill the sound. 
				//last_cane_of_byrna_item_id = -1; //no, we'd do this in a byrna cleanup function. 
				return false;
			}
				
			paymagiccost(itemid);
			last_cane_of_byrna_item_id = itemid; 
			//zprint("itemsbuf[itemid].misc3: %d\n", itemsbuf[itemid].misc3);
			for(int32_t i=0; i<itemsbuf[itemid].misc3; i++)
			{
				//byrna weapons are added here
				//space them apart
				//zprint("Added byrna weapon %d.\n", i);
				//the iterator isn passed to 'type'. weapons.cpp converts thisd to
				//'quantity_iterator' pn construction; and this is used for orbit initial spacing.
				Lwpns.add(new weapon((zfix)wx,(zfix)wy,(zfix)wz,wCByrna,i,itemsbuf[itemid].power*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
					//Lwpns.add(new weapon((zfix)wx+cos(2 * PI / (i+1)),(zfix)wy+sin(2 * PI / (i+1)),(zfix)wz,wCByrna,i,itemsbuf[itemid].power*game->get_hero_dmgmult(),dir,itemid,getUID(),false,false,true));
				//wx += cos(2 * PI / (itemsbuf[itemid].misc3-i));
				//wy += sin(2 * PI / (itemsbuf[itemid].misc3-i));
			}
			if(!(Lwpns.idCount(wCByrna)))
				stop_sfx(itemsbuf[itemid].usesound); //If we can't create the beams, kill the sound. 
		}
		break;
		
		default:
			ret = false;
	}
	
	if(itemsbuf[itemid].flags & ITEM_DOWNGRADE)
	{
		game->set_item(itemid,false);
		
		// Maybe Item Override has allowed the same item in both slots?
		if(Bwpn == itemid)
		{
			Bwpn = 0;
			game->forced_bwpn = -1;
			verifyBWpn();
		}
		
		if(Awpn == itemid)
		{
			Awpn = 0;
			game->forced_awpn = -1;
			verifyAWpn();
		}
		
		if(Xwpn == itemid)
		{
			Xwpn = 0;
			game->forced_xwpn = -1;
			verifyXWpn();
		}
		
		if(Ywpn == itemid)
		{
			Ywpn = 0;
			game->forced_ywpn = -1;
			verifyYWpn();
		}
	}

	return ret;
}


bool LinkClass::doattack()
{
	//int32_t s = BSZ ? 0 : 11;
	int32_t s = (zinit.linkanimationstyle==las_bszelda) ? 0 : 11;
	
	int32_t bugnetid = (directWpn>-1 && itemsbuf[directWpn].family==itype_bugnet) ? directWpn : current_item_id(itype_bugnet);
	if(attack==wBugNet && bugnetid!=-1)
	{
		if(++attackclk >= NET_CLK_TOTAL)
			return false;
		
		return true;
	}
	
	// Abort attack if attackclk has run out and:
	// * the attack is not Hammer, Sword with Spin Scroll, Candle, or Wand, OR
	// * you aren't holding down the A button, you're not charging, and/or you're still spinning
	
	if(attackclk>=(spins>0?8:14) && attack!=wHammer &&
			(((attack!=wSword || !current_item(itype_spinscroll) || inlikelike) && attack!=wWand && attack!=wFire && attack!=wCByrna) || !((attack==wSword && isWpnPressed(itype_sword) && spins==0) || charging>0)))
	{
		tapping=false;
		return false;
	}
	
	if(attackclk>29)
	{
		tapping=false;
		return false;
	}
	
	int32_t candleid = (directWpn>-1 && itemsbuf[directWpn].family==itype_candle) ? directWpn : current_item_id(itype_candle);
	int32_t byrnaid = (directWpn>-1 && itemsbuf[directWpn].family==itype_cbyrna) ? directWpn : current_item_id(itype_cbyrna);
	// An attack can be "walked out-of" after 8 frames, unless it's:
	// * a sword stab
	// * a hammer pound
	// * a wand thrust
	// * a candle thrust
	// * a cane thrust
	// In which case it should continue.
	if((attack==wCatching && attackclk>4)||(attack!=wWand && attack!=wSword && attack!=wHammer
											&& (attack!=wFire || (candleid!=-1 && !(itemsbuf[candleid].wpn)))
											&& (attack!=wCByrna || (byrnaid!=-1 && !(itemsbuf[byrnaid].wpn)))
											&& (attack != wBugNet) && attackclk>7))
	{
		if(DrunkUp()||DrunkDown()||DrunkLeft()||DrunkRight())
		{
			lstep = s;
			return false;
		}
	}
	
	if(charging==0)
	{
		lstep=0;
	}
	
	// Work out the sword charge-up delay
	int32_t magiccharge = 192, normalcharge = 64;
	int32_t itemid = current_item_id(itype_chargering);
	
	if(itemid>=0)
	{
		normalcharge = itemsbuf[itemid].misc1;
		magiccharge = itemsbuf[itemid].misc2;
	}
	
	itemid = current_item_id(attack==wHammer ? itype_quakescroll : itype_spinscroll);
	
	bool doCharge=true;
	if(z!=0)
		doCharge=false;
	if(attack==wSword)
	{
		if(!(attackclk==SWORDCHARGEFRAME && isWpnPressed(itype_sword)))
			doCharge=false;
		else if(charging<=normalcharge)
		{
			if(itemid<0 || !(checkbunny(itemid) && checkmagiccost(itemid)))
				doCharge=false;
		}
	}
	else if(attack==wHammer)
	{
		if(!(attackclk==HAMMERCHARGEFRAME && isWpnPressed(itype_hammer)))
			doCharge=false;
		else if(charging<=normalcharge)
		{
			if(itemid<0 || !(checkbunny(itemid) && checkmagiccost(itemid)))
				doCharge=false;
		}
	}
	else
		doCharge=false;
	
	// Now work out the magic cost
	itemid = current_item_id(attack==wHammer ? itype_quakescroll : itype_spinscroll);
	
	// charging up weapon...
	//
	if(doCharge)
	{
		// Increase charging while holding down button.
		if(spins==0 && charging<magiccharge)
			charging++;
			
		// Once a charging threshold is reached, play the sound.
		if(charging==normalcharge)
		{
			paymagiccost(itemid); //!DIMITODO: Can this underflow or even just do it even if you don't have magic?
			sfx(WAV_ZN1CHARGE,pan(x.getInt()));
		}
		else if(charging==magiccharge)
		{
			itemid = current_item_id(attack==wHammer ? itype_quakescroll2 : itype_spinscroll2);
			
			if(itemid>-1 && checkbunny(itemid) && checkmagiccost(itemid))
			{
				paymagiccost(itemid);
				charging++; // charging>magiccharge signifies a successful supercharge.
				sfx(WAV_ZN1CHARGE2,pan(x.getInt()));
			}
		}
	}
	else if(attack==wCByrna && byrnaid!=-1)
	{
		if(!(itemsbuf[byrnaid].wpn))
		{
			attack = wNone;
			return startwpn(attackid); // Beam if the Byrna stab animation WASN'T used.
		}
		
		bool beamcount = false;
		
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			weapon *w = ((weapon*)Lwpns.spr(i));
			
			if(w->id==wCByrna)
			{
				beamcount = true;
				break;
			}
		}
		
		// If beams already deployed, remove them
		if(!attackclk && beamcount)
		{
			return startwpn(attackid); // Remove beams instantly
		}
		
		// Otherwise, continue
		++attackclk;
	}
	else
	{
		++attackclk;
		
		if(attackclk==SWORDCHARGEFRAME && charging>0 && !tapping)  //Signifies a tapped enemy
		{
			++attackclk; // Won't continue charging
			charging=0;
		}
		
		// Faster if spinning.
		if(spins>0)
			++attackclk;
			
		// Even faster if hurricane spinning.
		if(spins>5)
			attackclk+=2;
			
		// If at a charging threshold, do a charged attack.
		if(charging>=normalcharge && (attack!=wSword || attackclk>=SWORDCHARGEFRAME) && !tapping)
		{
			if(attack==wSword)
			{
				spins=(charging>magiccharge ? (itemsbuf[current_item_id(itype_spinscroll2)].misc1*4)-3
					   : (itemsbuf[current_item_id(itype_spinscroll)].misc1*4)+1);
				attackclk=1;
				sfx(itemsbuf[current_item_id(spins>5 ? itype_spinscroll2 : itype_spinscroll)].usesound,pan(x.getInt()));
			}
			/*
			else if(attack==wWand)
			{
				//Not reachable.. yet
				spins=1;
			}
			*/
			else if(attack==wHammer && sideviewhammerpound())
			{
				spins=1; //signifies the quake hammer
				bool super = (charging>magiccharge && current_item(itype_quakescroll2));
				sfx(itemsbuf[current_item_id(super ? itype_quakescroll2 : itype_quakescroll)].usesound,pan(x.getInt()));
				quakeclk=(itemsbuf[current_item_id(super ? itype_quakescroll2 : itype_quakescroll)].misc1);
				
				// general area stun
				for(int32_t i=0; i<GuyCount(); i++)
				{
					if(!isflier(GuyID(i)))
					{
						StunGuy(i,(itemsbuf[current_item_id(super ? itype_quakescroll2 : itype_quakescroll)].misc2)-
								distance(x,y,GuyX(i),GuyY(i)));
					}
				}
			}
		}
		else if(tapping && attackclk<SWORDCHARGEFRAME && charging<magiccharge)
			charging++;
			
		if(!isWpnPressed(attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword))
			charging=0;
			
		if(attackclk>=SWORDCHARGEFRAME)
			tapping = false;
	}
	
	if(attackclk==1 && attack==wFire && candleid!=-1 && !(itemsbuf[candleid].wpn))
	{
		return startwpn(attackid); // Flame if the Candle stab animation WASN'T used.
	}
	
	int32_t crossid = current_item_id(itype_crossscroll);  //has Cross Beams scroll
	
	if(attackclk==13 || (attackclk==7 && spins>1 && crossid >=0 && checkbunny(crossid) && checkmagiccost(crossid)))
	{
	
		int32_t wpnid = (directWpn>-1 && itemsbuf[directWpn].family==itype_sword) ? directWpn : current_item_id(itype_sword);
		int64_t templife = wpnid>=0? itemsbuf[wpnid].misc1 : 0;
		
		if(wpnid>=0 && itemsbuf[wpnid].flags & ITEM_FLAG1)
		{
			templife=templife*game->get_maxlife();
			templife=templife/100;
		}
		else
		{
			templife*=game->get_hp_per_heart();
		}
		
		bool normalbeam = (game->get_life()+(get_bit(quest_rules,qr_QUARTERHEART)?((game->get_hp_per_heart()/4)-1):((game->get_hp_per_heart()/2)-1))>=templife);
		int32_t perilid = current_item_id(itype_perilscroll);
		bool perilbeam = (perilid>=0 && wpnid>=0 && game->get_life()<=itemsbuf[perilid].misc1*game->get_hp_per_heart()
						  && checkbunny(perilid) && checkmagiccost(perilid)
						  // Must actually be able to shoot sword beams
						  && ((itemsbuf[wpnid].flags & ITEM_FLAG1)
							  || itemsbuf[wpnid].misc1 <= game->get_maxlife()/game->get_hp_per_heart()));
							  
		if(attack==wSword && !tapping)
		{
			if(perilbeam || normalbeam)
			{
				if(attackclk==7)
					paymagiccost(crossid); // Pay the Cross Beams magic cost.
					
				if(perilbeam && !normalbeam)
					paymagiccost(perilid); // Pay the Peril Beam magic cost.
					
				// TODO: Something that would be cheap but disgraceful to hack in at this point is
				// a way to make the peril/cross beam item's power stat influence the strength
				// of the peril/cross beam...
				startwpn(attackid);
			}
			else misc_internal_link_flags &= ~LF_PAID_SWORD_COST;
		}
		
		if(attack==wWand)
			startwpn(attackid); // Flame if the Wand stab animation WAS used (it always is).
			
		if(attack==wFire && candleid!=-1 && itemsbuf[candleid].wpn) // Flame if the Candle stab animation WAS used.
			startwpn(attackid);
			
		if(attack==wCByrna && byrnaid!=-1 && itemsbuf[byrnaid].wpn) // Beam if the Byrna stab animation WAS used.
			startwpn(attackid);
	}
	
	if(attackclk==14)
		lstep = s;
		
	return true;
}

bool LinkClass::can_attack()
{
	int32_t currentSwordOrWand = (itemsbuf[dowpn].family == itype_wand || itemsbuf[dowpn].family == itype_sword)?dowpn:-1;
    if(action==hopping || action==swimming || action==freeze || action==sideswimfreeze || lstunclock > 0 ||
            ((action==attacking||action==sideswimattacking) && ((attack!=wSword && attack!=wWand) || !(itemsbuf[currentSwordOrWand].flags & ITEM_FLAG5)) && charging!=0) || spins>0)
    {
        return false;
    }
    
    int32_t r = (isdungeon()) ? 16 : 0;
    int32_t r2 = get_bit(quest_rules, qr_NOBORDER) ? 0 : 8;
    
    if(!get_bit(quest_rules, qr_ITEMSONEDGES)) switch(dir)
        {
        case up:
        case down:
            return !(y<(r2+r) || y>(160-r-r2));
            
        case left:
        case right:
            return !(x<(r2+r) || x>(240-r-r2));
        }
        
    return true;
}

bool isRaftFlag(int32_t flag)
{
    return (flag==mfRAFT || flag==mfRAFT_BRANCH || flag==mfRAFT_BOUNCE);
}

void do_lens()
{
	if ( FFCore.getQuestHeaderInfo(vZelda) >= 0x250 )
	{
		int32_t wpnPressed = getWpnPressed(itype_lens);
		int32_t itemid = lensid >= 0 ? lensid : wpnPressed>0 ? wpnPressed : Link.getLastLensID()>0 ? Link.getLastLensID() : current_item_id(itype_lens);
		if(itemid<0)
			return;
		//LinkItemClk is the item jinx.
		if(isWpnPressed(itype_lens) && !LinkItemClk() && !lensclk && checkbunny(itemid) && checkmagiccost(itemid))
		{
			if(lensid<0)
			{
				lensid=itemid;
				if(itemsbuf[itemid].family == itype_lens)
				Link.setLastLensID(itemid);
				if(get_bit(quest_rules,qr_MORESOUNDS)) sfx(itemsbuf[itemid].usesound);
			}
			
			paymagiccost(itemid, true); //Needs to ignore timer cause lensclk is our timer.
			
			if(itemid>=0 && itemsbuf[itemid].script != 0 && !did_scriptl && !item_doscript[itemid])
			{
				//clear the item script stack for a new script
				//itemScriptData[(itemid & 0xFFF)].Clear();
				//for ( int32_t q = 0; q < 1024; q++ ) item_stack[(itemid & 0xFFF)][q] = 0;
				ri = &(itemScriptData[itemid]);
				for ( int32_t q = 0; q < 1024; q++ ) item_stack[itemid][q] = 0xFFFF;
				ri->Clear();
				//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid & 0xFFF);
				item_doscript[itemid] = 1;
				itemscriptInitialised[itemid] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid);
					did_scriptl=true;
			}
			
			if (itemsbuf[itemid].magiccosttimer) lensclk = itemsbuf[itemid].magiccosttimer;
			else lensclk = 12;
		}
		else
		{
			did_scriptl=false;
			
			if(lensid>-1 && !(isWpnPressed(itype_lens) && !LinkItemClk() && checkbunny(itemid) && checkmagiccost(itemid)))
			{
				lensid=-1;
				lensclk = 0;
				
				if(get_bit(quest_rules,qr_MORESOUNDS)) sfx(WAV_ZN1LENSOFF);
			}
		}
	}
	else //2.10 or earlier
	{
		do_210_lens();
	}
}
//Add 2.10 version check to call this
void do_210_lens()
{
    int32_t itemid = lensid >= 0 ? lensid : directWpn>-1 ? directWpn : current_item_id(itype_lens);
    
    if(itemid<0)
        return;
        
    if(isWpnPressed(itype_lens) && !LinkItemClk() && !lensclk && checkmagiccost(itemid))
    {
        if(lensid<0)
        {
            lensid=itemid;
            
            if(get_bit(quest_rules,qr_MORESOUNDS)) sfx(itemsbuf[itemid].usesound);
        }
        
        paymagiccost(itemid, true);
        
        if(itemid>=0 && itemsbuf[itemid].script != 0 && !did_scriptl && !item_doscript[itemid])
        {
		//clear the item script stack for a new script
		//itemScriptData[(itemid & 0xFFF)].Clear();
		ri = &(itemScriptData[itemid]);
		for ( int32_t q = 0; q < 1024; q++ ) item_stack[itemid][q] = 0xFFFF;
		ri->Clear();
		//for ( int32_t q = 0; q < 1024; q++ ) item_stack[(itemid & 0xFFF)][q] = 0;
		//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid & 0xFFF);
		item_doscript[itemid] = 1;
		itemscriptInitialised[itemid] = 0;
		ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid);
		did_scriptl=true;
        }
        
        if (itemsbuf[itemid].magiccosttimer) lensclk = itemsbuf[itemid].magiccosttimer;
	else lensclk = 12;
    }
    else
    {
        did_scriptl=false;
        
        if(lensid>-1 && !(isWpnPressed(itype_lens) && !LinkItemClk() && checkmagiccost(itemid)))
        {
            lensid=-1;
            lensclk = 0;
            
            if(get_bit(quest_rules,qr_MORESOUNDS)) sfx(WAV_ZN1LENSOFF);
        }
    }
}

void LinkClass::do_hopping()
{
    do_lens();
    
    if(hopclk==0xFF) //|| (diagonalMovement && hopclk >= 0xFF) ))                                         // swimming
			//Possible fix for exiting water in diagonal movement. -Z
    {
		int32_t flippers_id = current_item_id(itype_flippers);
        if(diveclk>0)
		{
            --diveclk;
			if(flippers_id > -1 && itemsbuf[flippers_id].flags & ITEM_FLAG2 && DrunkrAbtn()) //Cancellable Diving -V
			{
				diveclk = itemsbuf[flippers_id].misc2;
			}
		}
        else if(DrunkrAbtn())
        {
            bool global_diving=(flippers_id > -1 && itemsbuf[flippers_id].flags & ITEM_FLAG1);
            bool screen_diving=(tmpscr->flags5&fTOGGLEDIVING) != 0;
            
            if(global_diving==screen_diving)
                diveclk = (flippers_id < 0 ? 80 : (itemsbuf[flippers_id].misc1 + itemsbuf[flippers_id].misc2));
        }
        
        if((!(x.getInt()&7) && !(y.getInt()&7)) || (diagonalMovement||NO_GRIDLOCK))
        {
		SetSwim();
		hopclk = 0;
		if (!IsSideSwim()) 
		{
			charging = attackclk = 0;
			tapping = false;
		}
        }
        else
        {
            linkstep();
            
            if(!isDiving() || (frame&1))
            {
                switch(dir)
                {
                case up:
                    y -= 1;
                    break;
                    
                case down:
                    y += 1;
                    break;
                    
                case left:
                    x -= 1;
                    break;
                    
                case right:
                    x += 1;
                    break;
                }
            }
        }
    }
    else                                                      // hopping in or out (need to separate the cases...)
    {
        if((diagonalMovement||NO_GRIDLOCK))
        {
            if(hopclk==1) //hopping out
		    //>= 1 possible fix for getting stuck on land edges. 
		//No, this is not a clock. it's a type. 1 == out, 2 == in. 
            {
                if(hopdir!=-1) dir=hopdir;
                
                landswim=0;
                
                if(dir==up)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(iswaterex(MAPCOMBO(x,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x,y+(bigHitbox?0:8)-1, true, false) && !iswaterex(MAPCOMBO(x+8,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+8,y+(bigHitbox?0:8)-1, true, false) && !iswaterex(MAPCOMBO(x+15,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+15,y+(bigHitbox?0:8)-1, true, false))
                        sidestep=1;
                    else if(!iswaterex(MAPCOMBO(x,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x,y+(bigHitbox?0:8)-1, true, false) && !iswaterex(MAPCOMBO(x+7,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+7,y+(bigHitbox?0:8)-1, true, false) && iswaterex(MAPCOMBO(x+15,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+15,y+(bigHitbox?0:8)-1, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) x++;
                    else if(sidestep==2) x--;
                    else y--;
                    
                    if(!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+15), currmap, currscr, -1, x.getInt(),y.getInt()+15, true, false))
                    {
                        hopclk=0;
                        diveclk=0;
                        action=none; FFCore.setLinkAction(none);
                        hopdir=-1;
                    }
                }
                
                if(dir==down)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(iswaterex(MAPCOMBO(x,y+16), currmap, currscr, -1, x,y+16, true, false) && !iswaterex(MAPCOMBO(x+8,y+16), currmap, currscr, -1, x+8,y+16, true, false) && !iswaterex(MAPCOMBO(x+15,y+16), currmap, currscr, -1, x+15,y+16, true, false))
                        sidestep=1;
                    else if(!iswaterex(MAPCOMBO(x,y+16), currmap, currscr, -1, x,y+16, true, false) && !iswaterex(MAPCOMBO(x+8,y+16), currmap, currscr, -1, x+8,y+16, true, false) && iswaterex(MAPCOMBO(x+15,y+16), currmap, currscr, -1, x+15,y+16, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) x++;
                    else if(sidestep==2) x--;
                    else y++;
                    
                    if(!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+15), currmap, currscr, -1, x.getInt(),y.getInt()+15, true, false))
                    {
                        hopclk=0;
                        diveclk=0;
                        action=none; FFCore.setLinkAction(none);
                        hopdir=-1;
                    }
                }
                
                if(dir==left)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(iswaterex(MAPCOMBO(x-1,y+(bigHitbox?0:8)), currmap, currscr, -1, x-1,y+(bigHitbox?0:8), true, false) && !iswaterex(MAPCOMBO(x-1,y+(bigHitbox?8:12)), currmap, currscr, -1, x-1,y+(bigHitbox?8:12), true, false) && !iswaterex(MAPCOMBO(x-1,y+15), currmap, currscr, -1, x-1,y+15, true, false))
                        sidestep=1;
                    else if(!iswaterex(MAPCOMBO(x-1,y+(bigHitbox?0:8)), currmap, currscr, -1, x-1,y+(bigHitbox?0:8), true, false) && !iswaterex(MAPCOMBO(x-1,y+(bigHitbox?7:11)), currmap, currscr, -1, x-1,y+(bigHitbox?7:11), true, false) && iswaterex(MAPCOMBO(x-1,y+15), currmap, currscr, -1, x-1,y+15, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) y++;
                    else if(sidestep==2) y--;
                    else x--;
                    
                    if(!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&!iswaterex(MAPCOMBO(x.getInt()+15,y.getInt()+8), currmap, currscr, -1, x.getInt()+15,y.getInt()+8, true, false))
                    {
                        hopclk=0;
                        diveclk=0;
                        action=none; FFCore.setLinkAction(none);
                        hopdir=-1;
                    }
                }
                
                if(dir==right)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(iswaterex(MAPCOMBO(x+16,y+(bigHitbox?0:8)), currmap, currscr, -1, x+16,y+(bigHitbox?0:8), true, false) && !iswaterex(MAPCOMBO(x+16,y+(bigHitbox?8:12)), currmap, currscr, -1, x+16,y+(bigHitbox?8:12), true, false) && !iswaterex(MAPCOMBO(x+16,y+15), currmap, currscr, -1, x+16,y+15, true, false))
                        sidestep=1;
                    else if(!iswaterex(MAPCOMBO(x+16,y+(bigHitbox?0:8)), currmap, currscr, -1, x+16,y+(bigHitbox?0:8), true, false) && !iswaterex(MAPCOMBO(x+16,y+(bigHitbox?7:11)), currmap, currscr, -1, x+16,y+(bigHitbox?7:11), true, false) && iswaterex(MAPCOMBO(x+16,y+15), currmap, currscr, -1, x+16,y+15, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) y++;
                    else if(sidestep==2) y--;
                    else x++;
                    
                    if(!iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&!iswaterex(MAPCOMBO(x.getInt()+15,y.getInt()+8), currmap, currscr, -1, x.getInt()+15,y.getInt()+8, true, false))
                    {
                        hopclk=0;
                        diveclk=0;
                        action=none; FFCore.setLinkAction(none);
                        hopdir=-1;
                    }
                }
            }
            
            if(hopclk==2) //hopping in
            {
                landswim=0;
                
                if(dir==up)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(!iswaterex(MAPCOMBO(x,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x,y+(bigHitbox?0:8)-1, true, false) && iswaterex(MAPCOMBO(x+8,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+8,y+(bigHitbox?0:8)-1, true, false) && iswaterex(MAPCOMBO(x+15,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+15,y+(bigHitbox?0:8)-1, true, false))
                        sidestep=1;
                    else if(iswaterex(MAPCOMBO(x,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x,y+(bigHitbox?0:8)-1, true, false) && iswaterex(MAPCOMBO(x+7,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+7,y+(bigHitbox?0:8)-1, true, false) && !iswaterex(MAPCOMBO(x+15,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+15,y+(bigHitbox?0:8)-1, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) x++;
                    else if(sidestep==2) x--;
                    else y--;
                    
		    if(iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&iswaterex(MAPCOMBO(x.getInt(),y.getInt()+15), currmap, currscr, -1, x.getInt(),y.getInt()+15, true, false))
                    {
                        hopclk=0xFF;
                        diveclk=0;
                        SetSwim();
                    }
                }
                
                if(dir==down)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(!iswaterex(MAPCOMBO(x,y+16), currmap, currscr, -1, x,y+16, true, false) && iswaterex(MAPCOMBO(x+8,y+16), currmap, currscr, -1, x+8,y+16, true, false) && iswaterex(MAPCOMBO(x+15,y+16), currmap, currscr, -1, x+15,y+16, true, false))
                        sidestep=1;
                    else if(iswaterex(MAPCOMBO(x,y+16), currmap, currscr, -1, x,y+16, true, false) && iswaterex(MAPCOMBO(x+8,y+16), currmap, currscr, -1, x+8,y+16, true, false) && !iswaterex(MAPCOMBO(x+15,y+16), currmap, currscr, -1, x+15,y+16, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) x++;
                    else if(sidestep==2) x--;
                    else y++;
                    
		    if(iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&iswaterex(MAPCOMBO(x.getInt(),y.getInt()+15), currmap, currscr, -1, x.getInt(),y.getInt()+15, true, false))
                    {
                        hopclk=0xFF;
                        diveclk=0;
                        SetSwim();
                        if (!IsSideSwim()) reset_swordcharge();
                    }
                }
                
                if(dir==left)
                {
                    linkstep();
                    linkstep();
                    int32_t sidestep=0;
                    
                    if(!iswaterex(MAPCOMBO(x-1,y+(bigHitbox?0:8)), currmap, currscr, -1, x-1,y+(bigHitbox?0:8), true, false) && iswaterex(MAPCOMBO(x-1,y+(bigHitbox?8:12)), currmap, currscr, -1, x-1,y+(bigHitbox?8:12), true, false) && iswaterex(MAPCOMBO(x-1,y+15), currmap, currscr, -1, x-1,y+15, true, false))
                        sidestep=1;
                    else if(iswaterex(MAPCOMBO(x-1,y+(bigHitbox?0:8)), currmap, currscr, -1, x-1,y+(bigHitbox?0:8), true, false) && iswaterex(MAPCOMBO(x-1,y+(bigHitbox?7:11)), currmap, currscr, -1, x-1,y+(bigHitbox?7:11), true, false) && !iswaterex(MAPCOMBO(x-1,y+15), currmap, currscr, -1, x-1,y+15, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) y++;
                    else if(sidestep==2) y--;
                    else x--;
                    
		    if(iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&iswaterex(MAPCOMBO(x.getInt()+15,y.getInt()+8), currmap, currscr, -1, x.getInt()+15,y.getInt()+8, true, false))
                    {
                        hopclk=0xFF;
                        diveclk=0;
                        SetSwim();
                    }
                }
                
                if(dir==right)
                {
                    linkstep();
                    linkstep();
                    
                    int32_t sidestep=0;
                    
                    if(!iswaterex(MAPCOMBO(x+16,y+(bigHitbox?0:8)), currmap, currscr, -1, x+16,y+(bigHitbox?0:8), true, false) && iswaterex(MAPCOMBO(x+16,y+(bigHitbox?8:12)), currmap, currscr, -1, x+16,y+(bigHitbox?8:12), true, false) && iswaterex(MAPCOMBO(x+16,y+15), currmap, currscr, -1, x+16,y+15, true, false))
                        sidestep=1;
                    else if(iswaterex(MAPCOMBO(x+16,y+(bigHitbox?0:8)), currmap, currscr, -1, x+16,y+(bigHitbox?0:8), true, false) && iswaterex(MAPCOMBO(x+16,y+(bigHitbox?7:11)), currmap, currscr, -1, x+16,y+(bigHitbox?7:11), true, false) && !iswaterex(MAPCOMBO(x+16,y+15), currmap, currscr, -1, x+16,y+15, true, false))
                        sidestep=2;
                        
                    if(sidestep==1) y++;
                    else if(sidestep==2) y--;
                    else x++;
                    
		    if(iswaterex(MAPCOMBO(x.getInt(),y.getInt()+(bigHitbox?0:8)), currmap, currscr, -1, x.getInt(),y.getInt()+(bigHitbox?0:8), true, false)&&iswaterex(MAPCOMBO(x.getInt()+15,y.getInt()+8), currmap, currscr, -1, x.getInt()+15,y.getInt()+8, true, false))
                    {
                        hopclk=0xFF;
                        diveclk=0;
                        SetSwim();
                    }
                }
            }
            
        }
        else
        {
            if((dir<left ? !(x.getInt()&7) && !(y.getInt()&15) : !(x.getInt()&15) && !(y.getInt()&7)))
            {
                action=none; FFCore.setLinkAction(none);
                hopclk = 0;
                diveclk = 0;
                
                if(iswaterex(MAPCOMBO(x.getInt(),y.getInt()+8), currmap, currscr, -1, x.getInt(),y.getInt()+8, true, false))
                {
                    // hopped in
                    SetSwim();
                    if (!IsSideSwim()) attackclk = charging = spins = 0;
                }
            }
            else
            {
                linkstep();
                linkstep();
                
                if(++link_count>(16*link_animation_speed))
                    link_count=0;
                    
                int32_t xofs2 = x.getInt()&15;
                int32_t yofs2 = y.getInt()&15;
                int32_t s = 1 + (frame&1);
                
                switch(dir)
                {
                case up:
                    if(yofs2<3 || yofs2>13) --y;
                    else y-=s;
                    
                    break;
                    
                case down:
                    if(yofs2<3 || yofs2>13) ++y;
                    else y+=s;
                    
                    break;
                    
                case left:
                    if(xofs2<3 || xofs2>13) --x;
                    else x-=s;
                    
                    break;
                    
                case right:
                    if(xofs2<3 || xofs2>13) ++x;
                    else x+=s;
                    
                    break;
                }
            }
        }
    }
}

void LinkClass::do_rafting()
{

    if(toogam)
    {
        action=none; FFCore.setLinkAction(none);
        return;
    }
    
    FFCore.setLinkAction(rafting);
    
    do_lens();
    
    linkstep();
    
	//Calculate rafting speed
	int32_t raft_item = current_item_id(itype_raft);
	int32_t raft_step = (raft_item < 0 ? 1 : itemsbuf[raft_item].misc1);
	raft_step = vbound(raft_step, -8, 5);
	int32_t raft_time = raft_step < 0 ? 1<<(-raft_step) : 1;
	if(raft_step < 0) raft_step = 1;
	int32_t step_inc = 1 << (raft_step - 1);
	// Fix position
	if(raft_step > 1)
	{
		if(x.getInt() & (step_inc-1))
		{
			x = x.getInt() & ~(step_inc-1);
		}
		if(y.getInt() & (step_inc-1))
		{
			y = y.getInt() & ~(step_inc-1);
		}
	}
	// Inc clock, check if we need to move this frame
	++raftclk;
	if((raftclk % raft_time) || raft_step == 0) return; //No movement this frame
	
    if(!(x.getInt()&15) && !(y.getInt()&15))
    {
        // this sections handles switching to raft branches
        if((MAPFLAG(x,y)==mfRAFT_BRANCH||MAPCOMBOFLAG(x,y)==mfRAFT_BRANCH))
        {
            if(dir!=down && DrunkUp() && (isRaftFlag(nextflag(x,y,up,false))||isRaftFlag(nextflag(x,y,up,true))))
            {
                dir = up;
                goto skip;
            }
            
            if(dir!=up && DrunkDown() && (isRaftFlag(nextflag(x,y,down,false))||isRaftFlag(nextflag(x,y,down,true))))
            {
                dir = down;
                goto skip;
            }
            
            if(dir!=right && DrunkLeft() && (isRaftFlag(nextflag(x,y,left,false))||isRaftFlag(nextflag(x,y,left,true))))
            {
                dir = left;
                goto skip;
            }
            
            if(dir!=left && DrunkRight() && (isRaftFlag(nextflag(x,y,right,false))||isRaftFlag(nextflag(x,y,right,true))))
            {
                dir = right;
                goto skip;
            }
        }
        else if((MAPFLAG(x,y)==mfRAFT_BOUNCE||MAPCOMBOFLAG(x,y)==mfRAFT_BOUNCE))
        {
            if(dir == left) dir = right;
            else if(dir == right) dir = left;
            else if(dir == up) dir = down;
            else if(dir == down) dir = up;
        }
        
        
        if(!isRaftFlag(nextflag(x,y,dir,false))&&!isRaftFlag(nextflag(x,y,dir,true)))
        {
            if(dir<left) //going up or down
            {
                if((isRaftFlag(nextflag(x,y,right,false))||isRaftFlag(nextflag(x,y,right,true))))
                    dir=right;
                else if((isRaftFlag(nextflag(x,y,left,false))||isRaftFlag(nextflag(x,y,left,true))))
                    dir=left;
                else if(y>0 && y<160) 
		{
                    action=none; FFCore.setLinkAction(none);
		}
            }
            else //going left or right
            {
                if((isRaftFlag(nextflag(x,y,down,false))||isRaftFlag(nextflag(x,y,down,true))))
                    dir=down;
                else if((isRaftFlag(nextflag(x,y,up,false))||isRaftFlag(nextflag(x,y,up,true))))
                    dir=up;
                else if(x>0 && x<240)
		{
                    action=none; FFCore.setLinkAction(none);
		}
            }
        }
    }
    
skip:

    switch(dir)
    {
    case up:
        if(x.getInt()&15)
        {
            if(x.getInt()&8)
                x++;
            else x--;
        }
        else y -= step_inc;
        
        break;
        
    case down:
        if(x.getInt()&15)
        {
            if(x.getInt()&8)
                x++;
            else x--;
        }
        else y += step_inc;
        
        break;
        
    case left:
        if(y.getInt()&15)
        {
            if(y.getInt()&8)
                y++;
            else y--;
        }
        else x -= step_inc;
        
        break;
        
    case right:
        if(y.getInt()&15)
        {
            if(y.getInt()&8)
                y++;
            else y--;
        }
        else x += step_inc;
        
        break;
    }
}

bool LinkClass::try_hover()
{
	if(hoverclk <= 0 && can_use_item(itype_hoverboots,i_hoverboots) && !ladderx && !laddery && !(hoverflags & HOV_OUT))
	{
		int32_t itemid = current_item_id(itype_hoverboots);
		if(hoverclk < 0)
			hoverclk = -hoverclk;
		else
		{
			fall = jumping = 0;
			if(itemsbuf[itemid].misc1)
				hoverclk = itemsbuf[itemid].misc1;
			else
			{
				hoverclk = 1;
				hoverflags |= HOV_INF;
			}
			
				
			sfx(itemsbuf[itemid].usesound,pan(x.getInt()));
		}
		if(itemsbuf[itemid].wpn)
			decorations.add(new dHover(x, y, dHOVER, 0));
		return true;
	}
	return false;
}

//Returns bitwise; lower 8 are dir pulled in, next 16 are combo ID, 25th bit is bool for if can be resisted
//Returns '-1' if not being pulled
//Returns '-2' if should be falling in
int32_t LinkClass::check_pitslide(bool ignore_hover)
{
	//Pitfall todo -Emily
	//Iron boots; can't fight slipping, 2px/frame
	//Scripted variables to read pull dir/clk (clk only for non-link)
	//Implement falling for all sprite types (npc AI)
	//    Fall/slipping tiles for enemies
	//    Fall/slipping SFX for enemies
	//    Fall SFX for items/weapons
	//    Weapons/Misc sprite shared for falling items/weapons
	//Maybe slip SFX for Link?
	// Weapons/Misc sprite override for falling sprite?
	//Update std.zh with relevant new stuff
	if(can_pitfall(ignore_hover))
	{
		bool can_diag = (diagonalMovement || get_bit(quest_rules,qr_DISABLE_4WAY_GRIDLOCK));
		int32_t ispitul = getpitfall(x,y+(bigHitbox?0:8));
		int32_t ispitbl = getpitfall(x,y+15);
		int32_t ispitur = getpitfall(x+15,y+(bigHitbox?0:8));
		int32_t ispitbr = getpitfall(x+15,y+15);
		int32_t ispitul_50 = getpitfall(x+8,y+(bigHitbox?8:12));
		int32_t ispitbl_50 = getpitfall(x+8,y+(bigHitbox?7:11));
		int32_t ispitur_50 = getpitfall(x+7,y+(bigHitbox?8:12));
		int32_t ispitbr_50 = getpitfall(x+7,y+(bigHitbox?7:11));
		int32_t ispitul_75 = getpitfall(x+12,y+(bigHitbox?12:14));
		int32_t ispitbl_75 = getpitfall(x+12,y+(bigHitbox?3:9));
		int32_t ispitur_75 = getpitfall(x+3,y+(bigHitbox?12:14));
		int32_t ispitbr_75 = getpitfall(x+3,y+(bigHitbox?3:9));
		static const int32_t flag_pit_irresistable = (1<<24);
		switch((ispitul?1:0) + (ispitur?1:0) + (ispitbl?1:0) + (ispitbr?1:0))
		{
			case 4: return -2; //Fully over pit; fall in
			case 3:
			{
				if(ispitul && ispitur && ispitbl) //UL_3
				{
					if(ispitul_50)
					{
						if(!ispitul_75 && (DrunkDown() || DrunkRight())) return -1;
						return (can_diag ? l_up : left) | (ispitul_75 ? flag_pit_irresistable : 0) | (ispitul << 8);
					}
				}
				else if(ispitul && ispitur && ispitbr) //UR_3
				{
					if(ispitur_50)
					{
						if(!ispitur_75 && (DrunkDown() || DrunkLeft())) return -1;
						return (can_diag ? r_up : right) | (ispitur_75 ? flag_pit_irresistable : 0) | (ispitur << 8);
					}
				}
				else if(ispitul && ispitbl && ispitbr) //BL_3
				{
					if(ispitbl_50)
					{
						if(!ispitbl_75 && (DrunkUp() || DrunkRight())) return -1;
						return (can_diag ? l_down : left) | (ispitbl_75 ? flag_pit_irresistable : 0) | (ispitbl << 8);
					}
				}
				else if(ispitbl && ispitur && ispitbr) //BR_3
				{
					if(ispitbr_50)
					{
						if(!ispitbr_75 && (DrunkUp() || DrunkLeft())) return -1;
						return (can_diag ? r_down : right) | (ispitbr_75 ? flag_pit_irresistable : 0) | (ispitbr << 8);
					}
				}
				break;
			}
			case 2:
			{
				if(ispitul && ispitur) //Up
				{
					if(DrunkDown())
					{
						if(ispitul_75 && ispitur_75) //Straight up
						{
							return up | flag_pit_irresistable | (ispitul << 8);
						}
						else if(ispitul_75)
						{
							return (can_diag ? l_up : left) | flag_pit_irresistable | (ispitul << 8);
						}
						else if(ispitur_75)
						{
							return (can_diag ? r_up : right) | flag_pit_irresistable | (ispitur << 8);
						}
						else return -1;
					}
					else
					{
						if(ispitul_50 && ispitur_50) //Straight up
						{
							return up | ((ispitul_75 || ispitur_75) ? flag_pit_irresistable : 0) | (ispitul << 8);
						}
						else if(ispitul_50)
						{
							if(DrunkRight() && !ispitul_75) return -1;
							return (can_diag ? l_up : left) | (ispitul_75 ? flag_pit_irresistable : 0) | (ispitul << 8);
						}
						else if(ispitur_50)
						{
							if(DrunkLeft() && !ispitur_75) return -1;
							return (can_diag ? r_up : right) | (ispitur_75 ? flag_pit_irresistable : 0) | (ispitur << 8);
						}
					}
				}
				else if(ispitbl && ispitbr) //Down
				{
					if(DrunkUp())
					{
						if(ispitbl_75 && ispitbr_75) //Straight down
						{
							return down | flag_pit_irresistable | (ispitbl << 8);
						}
						else if(ispitbl_75)
						{
							return (can_diag ? l_down : left) | flag_pit_irresistable | (ispitbl << 8);
						}
						else if(ispitbr_75)
						{
							return (can_diag ? r_down : right) | flag_pit_irresistable | (ispitbr << 8);
						}
						else return -1;
					}
					else
					{
						if(ispitbl_50 && ispitbr_50) //Straight down
						{
							return down | ((ispitbl_75 || ispitbr_75) ? flag_pit_irresistable : 0) | (ispitbl << 8);
						}
						else if(ispitbl_50)
						{
							if(DrunkRight() && !ispitbl_75) return -1;
							return (can_diag ? l_down : left) | (ispitbl_75 ? flag_pit_irresistable : 0) | (ispitbl << 8);
						}
						else if(ispitbr_50)
						{
							if(DrunkLeft() && !ispitbr_75) return -1;
							return (can_diag ? r_down : right) | (ispitbr_75 ? flag_pit_irresistable : 0) | (ispitbr << 8);
						}
					}
				}
				else if(ispitbl && ispitul) //Left
				{
					if(DrunkRight())
					{
						if(ispitul_75 && ispitbl_75) //Straight left
						{
							return left | flag_pit_irresistable | (ispitul << 8);
						}
						else if(ispitul_75)
						{
							return (can_diag ? l_up : up) | flag_pit_irresistable | (ispitul << 8);
						}
						else if(ispitbl_75)
						{
							return (can_diag ? l_down : down) | flag_pit_irresistable | (ispitbl << 8);
						}
						else return -1;
					}
					else
					{
						if(ispitul_50 && ispitbl_50) //Straight left
						{
							return left | ((ispitul_75 || ispitbl_75) ? flag_pit_irresistable : 0) | (ispitul << 8);
						}
						else if(ispitul_50)
						{
							if(DrunkDown() && !ispitul_75) return -1;
							return (can_diag ? l_up : up) | (ispitul_75 ? flag_pit_irresistable : 0) | (ispitul << 8);
						}
						else if(ispitbl_50)
						{
							if(DrunkUp() && !ispitbl_75) return -1;
							return (can_diag ? l_down : down) | (ispitbl_75 ? flag_pit_irresistable : 0) | (ispitbl << 8);
						}
					}
				}
				else if(ispitbr && ispitur) //Right
				{
					if(DrunkLeft())
					{
						if(ispitur_75 && ispitbr_75) //Straight right
						{
							return right | flag_pit_irresistable | (ispitur << 8);
						}
						else if(ispitur_75)
						{
							return (can_diag ? r_up : up) | flag_pit_irresistable | (ispitur << 8);
						}
						else if(ispitbr_75)
						{
							return (can_diag ? r_down : down) | flag_pit_irresistable | (ispitbr << 8);
						}
						else return -1;
					}
					else
					{
						if(ispitur_50 && ispitbr_50) //Straight right
						{
							return right | ((ispitur_75 || ispitbr_75) ? flag_pit_irresistable : 0) | (ispitur << 8);
						}
						else if(ispitur_50)
						{
							if(DrunkDown() && !ispitur_75) return -1;
							return (can_diag ? r_up : up) | (ispitur_75 ? flag_pit_irresistable : 0) | (ispitur << 8);
						}
						else if(ispitbr_50)
						{
							if(DrunkUp() && !ispitbr_75) return -1;
							return (can_diag ? r_down : down) | (ispitbr_75 ? flag_pit_irresistable : 0) | (ispitbr << 8);
						}
					}
				}
				break;
			}
			case 1:
			{
				if(ispitul && ispitul_50) //UL_1
				{
					if(!ispitul_75 && (DrunkDown() || DrunkRight())) return -1;
					return (can_diag ? l_up : left) | (ispitul_75 ? flag_pit_irresistable : 0) | (ispitul << 8);
				}
				if(ispitur && ispitur_50) //UR_1
				{
					if(!ispitur_75 && (DrunkDown() || DrunkLeft())) return -1;
					return (can_diag ? r_up : right) | (ispitur_75 ? flag_pit_irresistable : 0) | (ispitur << 8);
				}
				if(ispitbl && ispitbl_50) //BL_1
				{
					if(!ispitbl_75 && (DrunkUp() || DrunkRight())) return -1;
					return (can_diag ? l_down : left) | (ispitbl_75 ? flag_pit_irresistable : 0) | (ispitbl << 8);
				}
				if(ispitbr && ispitbr_50) //BR_1
				{
					if(!ispitbr_75 && (DrunkUp() || DrunkLeft())) return -1;
					return (can_diag ? r_down : right) | (ispitbr_75 ? flag_pit_irresistable : 0) | (ispitbr << 8);
				}
				break;
			}
		}
	}
	return -1;
}

bool LinkClass::pitslide() //Runs pitslide movement; returns true if pit is irresistable
{
	pitfall();
	if(fallclk) return true;
	int32_t val = check_pitslide();
	//Val should not be -2 here; if -2 would have been returned, the 'return true' above should have triggered!
	if(val == -1)
	{
		pit_pulldir = -1;
		pit_pullclk = 0;
		return false;
	}
	int32_t dir = val&0xFF;
	int32_t cmbid = (val&0xFFFF00)>>8;
	int32_t sensitivity = combobuf[cmbid].attribytes[2];
	if(combobuf[cmbid].usrflags&cflag5) //No pull at all
	{
		pit_pulldir = -1;
		pit_pullclk = 0;
		return false;
	}
	if(dir > -1 && !(hoverflags & HOV_PITFALL_OUT) && try_hover()) //Engage hovers
	{
		pit_pulldir = -1;
		pit_pullclk = 0;
		return false;
	}
	pit_pulldir = dir;
	int32_t step = 1;
	if(sensitivity == 0)
	{
		step = 2;
		sensitivity = 1;
	}
	if(pit_pullclk++ % sensitivity) //No pull this frame
		return (val&0x100);
	for(; step > 0 && !fallclk; --step)
	{
		switch(dir)
		{
			case l_up: case l_down: case left:
				--x; break;
			case r_up: case r_down: case right:
				++x; break;
		}
		switch(dir)
		{
			case l_up: case r_up: case up:
				--y; break;
			case l_down: case r_down: case down:
				++y; break;
		}
		pitfall();
	}
	return fallclk || (val&0x100);
}

void LinkClass::pitfall()
{
	if(fallclk)
	{
		if(fallclk == PITFALL_FALL_FRAMES && fallCombo) sfx(combobuf[fallCombo].attribytes[0], pan(x.getInt()));
		//Handle falling
		if(!--fallclk)
		{
			int32_t dmg = game->get_hp_per_heart()/4;
			bool dmg_perc = false;
			bool warp = false;
			
			action=none; FFCore.setLinkAction(none);
			newcombo* cmb = fallCombo ? &combobuf[fallCombo] : NULL;
			if(cmb)
			{
				dmg = cmb->attributes[0]/10000L;
				dmg_perc = cmb->usrflags&cflag3;
				warp = cmb->usrflags&cflag1;
			}
			if(dmg) //Damage
			{
				if(dmg > 0) hclk=48; //IFrames only if damaged, not if healed
				game->set_life(vbound(int32_t(dmg_perc ? game->get_life() - ((vbound(dmg,-100,100)/100.0)*game->get_maxlife()) : (game->get_life()-dmg)),0,game->get_maxlife()));
			}
			if(warp) //Warp
			{
				sdir = dir;
				if(cmb->usrflags&cflag2) //Direct Warp
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				dowarp(0,vbound(cmb->attribytes[1],0,3),0);
			}
			else //Reset to screen entry
			{
				x=entry_x;
				y=entry_y;
				can_mirror_portal = false; //incase entry is on a portal!
				warpx=x;
				warpy=y;
			}
		}
	}
	else if(can_pitfall())
	{
		bool ispitul = ispitfall(x,y+(bigHitbox?0:8));
		bool ispitbl = ispitfall(x,y+15);
		bool ispitur = ispitfall(x+15,y+(bigHitbox?0:8));
		bool ispitbr = ispitfall(x+15,y+15);
		int32_t pitctr = getpitfall(x+8,y+(bigHitbox?8:12));
		if(ispitul && ispitbl && ispitur && ispitbr && pitctr)
		{
			if(!(hoverflags & HOV_PITFALL_OUT) && try_hover()) return;
			if(!bigHitbox && !ispitfall(x,y)) y = (y.getInt() + 8 - (y.getInt() % 8)); //Make the falling sprite fully over the pit
			fallclk = PITFALL_FALL_FRAMES;
			fallCombo = pitctr;
			action=falling; FFCore.setLinkAction(falling);
			spins = 0;
			charging = 0;
		}
	}
}

void LinkClass::movelink()
{
	if(lstunclock) return;
	int32_t xoff=x.getInt()&7;
	int32_t yoff=y.getInt()&7;
	if(NO_GRIDLOCK)
	{
		xoff = 0;
		yoff = 0;
	}
	int32_t push=pushing;
	int32_t oldladderx=-1000, oldladdery=-1000; // moved here because linux complains "init crosses goto ~Koopa
	pushing=0;
	zfix temp_step(link_newstep);
	zfix temp_x(x);
	zfix temp_y(y);
	
	int32_t flippers_id = current_item_id(itype_flippers);
	if(diveclk>0)
	{
		if (isSideViewLink() && get_bit(quest_rules,qr_SIDESWIM)) diveclk = 0;
		--diveclk;
		if(isDiving() && flippers_id > -1 && itemsbuf[flippers_id].flags & ITEM_FLAG2 && DrunkrAbtn()) //Cancellable Diving -V
		{
			diveclk = itemsbuf[flippers_id].misc2;
		}
	}
	else if(action == swimming && DrunkrAbtn())
	{
		bool global_diving=(flippers_id > -1 && itemsbuf[flippers_id].flags & ITEM_FLAG1);
		bool screen_diving=(tmpscr->flags5&fTOGGLEDIVING) != 0;
		
		if(global_diving==screen_diving)
			diveclk = (flippers_id < 0 ? 80 : (itemsbuf[flippers_id].misc1 + itemsbuf[flippers_id].misc2));
	}
	
	if(action==rafting)
	{
		do_rafting();
		
		if(action==rafting)
		{
			return;
		}
		
		setEntryPoints(x,y);
	}
	
	int32_t olddirectwpn = directWpn; // To be reinstated if startwpn() fails
	int32_t btnwpn = -1;
	
	//&0xFFF removes the "bow & arrows" bitmask
	//The Quick Sword is allowed to interrupt attacks.
	int32_t currentSwordOrWand = (itemsbuf[dowpn].family == itype_wand || itemsbuf[dowpn].family == itype_sword)?dowpn:-1;
	if((!attackclk && action!=attacking && action != sideswimattacking) || ((attack==wSword || attack==wWand) && (itemsbuf[currentSwordOrWand].flags & ITEM_FLAG5)))
	{
		if(DrunkrBbtn())
		{
			btnwpn=getItemFamily(itemsbuf,Bwpn&0xFFF);
			dowpn = Bwpn&0xFFF;
			directWpn = directItemB;
		}
		else if(DrunkrAbtn())
		{
			btnwpn=getItemFamily(itemsbuf,Awpn&0xFFF);
			dowpn = Awpn&0xFFF;
			directWpn = directItemA;
		}
		else if(get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) && DrunkrEx1btn())
		{
			btnwpn=getItemFamily(itemsbuf,Xwpn&0xFFF);
			dowpn = Xwpn&0xFFF;
			directWpn = directItemX;
		}
		else if(get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) && DrunkrEx2btn())
		{
			btnwpn=getItemFamily(itemsbuf,Ywpn&0xFFF);
			dowpn = Ywpn&0xFFF;
			directWpn = directItemY;
		}
		
		if(directWpn > 255) directWpn = 0;
		
		// The Quick Sword only allows repeated sword or wand swings.
		if((action==attacking||action==sideswimattacking) && ((attack==wSword && btnwpn!=itype_sword) || (attack==wWand && btnwpn!=itype_wand)))
			btnwpn=-1;
	}
	
	if(can_attack() && (directWpn>-1 ? itemsbuf[directWpn].family==itype_sword : current_item(itype_sword)) && swordclk==0 && btnwpn==itype_sword && charging==0)
	{
		attackid=directWpn>-1 ? directWpn : current_item_id(itype_sword);
		if(checkbunny(attackid) && (checkmagiccost(attackid) || !(itemsbuf[attackid].flags & ITEM_FLAG6)))
		{
			if((itemsbuf[attackid].flags & ITEM_FLAG6) && !(misc_internal_link_flags & LF_PAID_SWORD_COST))
			{
				paymagiccost(attackid,true);
				misc_internal_link_flags |= LF_PAID_SWORD_COST;
			}
			SetAttack();
			attack=wSword;
			
			attackclk=0;
			sfx(itemsbuf[directWpn>-1 ? directWpn : current_item_id(itype_sword)].usesound, pan(x.getInt()));
			
			if(dowpn>-1 && itemsbuf[dowpn].script!=0 && !did_scripta && checkmagiccost(dowpn) && !item_doscript[dowpn])
			{
				//clear the item script stack for a new script
			
				ri = &(itemScriptData[dowpn]);
				for ( int32_t q = 0; q < 1024; q++ ) item_stack[dowpn][q] = 0xFFFF;
				ri->Clear();
				//itemScriptData[(dowpn & 0xFFF)].Clear();
				//for ( int32_t q = 0; q < 1024; q++ ) item_stack[(dowpn & 0xFFF)][q] = 0;
				//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[dowpn].script, dowpn & 0xFFF);
				item_doscript[dowpn] = 1;
				itemscriptInitialised[dowpn] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[dowpn].script, dowpn);
				did_scripta=true;
			}
		}
	}
	else
	{
		did_scripta=false;
	}
	
	if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && !getOnSideviewLadder())
	{
		if(DrunkUp() && canSideviewLadder())
		{
			setOnSideviewLadder(true);
		}
		else if(DrunkDown() && canSideviewLadder(true))
		{
			y+=1;
			setOnSideviewLadder(true);
		}
	}
	
	int32_t wx=x;
	int32_t wy=y;
	if((action==none || action==walking) && getOnSideviewLadder() && (get_bit(quest_rules,qr_SIDEVIEWLADDER_FACEUP)!=0)) //Allow DIR to change if standing still on sideview ladder, and force-face up.
	{
		if((xoff==0)||diagonalMovement)
		{
			if(DrunkUp()) dir=up;
			if(DrunkDown()) dir=down;
		}
		
		if((yoff==0)||diagonalMovement)
		{
			if(DrunkLeft()) dir=left;
			if(DrunkRight()) dir=right;
		}
	}
	
	switch(dir)
	{
	case up:
		wy-=16;
		break;
		
	case down:
		wy+=16;
		break;
		
	case left:
		wx-=16;
		break;
		
	case right:
		wx+=16;
		break;
	}
	
	do_lens();
	
	WalkflagInfo info;
	
	if(can_attack() && itemclk==0 && btnwpn>itype_sword && charging==0 && btnwpn!=itype_rupee) // This depends on item 0 being a rupee...
	{
		bool paidmagic = false;
		
		if(btnwpn==itype_wand && (directWpn>-1 ? (!item_disabled(directWpn) ? itemsbuf[directWpn].family==itype_wand : false) : current_item(itype_wand)))
		{
			attackid=directWpn>-1 ? directWpn : current_item_id(itype_wand);
			if(checkbunny(attackid) && ((!(itemsbuf[attackid].flags & ITEM_FLAG6)) || checkmagiccost(attackid)))
			{
				if((itemsbuf[attackid].flags & ITEM_FLAG6) && !(misc_internal_link_flags & LF_PAID_WAND_COST)){
					paymagiccost(attackid,true);
					misc_internal_link_flags |= LF_PAID_WAND_COST;
				}
				SetAttack();
				attack=wWand;
				attackclk=0;
			}
		}
		else if((btnwpn==itype_hammer)&&!((action==attacking||action==sideswimattacking) && attack==wHammer)
				&& (directWpn>-1 ? (!item_disabled(directWpn) ? itemsbuf[directWpn].family==itype_hammer : false) : current_item(itype_hammer)) && checkmagiccost(dowpn) && checkbunny(dowpn))
		{
			paymagiccost(dowpn);
			paidmagic = true;
			SetAttack();
			attack=wHammer;
			attackid=directWpn>-1 ? directWpn : current_item_id(itype_hammer);
			attackclk=0;
		}
		else if((btnwpn==itype_candle)&&!((action==attacking||action==sideswimattacking) && attack==wFire)
				&& (directWpn>-1 ? (!item_disabled(directWpn) ? itemsbuf[directWpn].family==itype_candle : false) : current_item(itype_candle)))
		{
			//checkbunny handled where magic cost is paid
			attackid=directWpn>-1 ? directWpn : current_item_id(itype_candle);
			SetAttack();
			attack=wFire;
			attackclk=0;
		}
		else if((btnwpn==itype_cbyrna)&&!((action==attacking||action==sideswimattacking) && attack==wCByrna)
				&& (directWpn>-1 ? (!item_disabled(directWpn) ? itemsbuf[directWpn].family==itype_cbyrna : false) : current_item(itype_cbyrna)))
		{
			attackid=directWpn>-1 ? directWpn : current_item_id(itype_cbyrna);
			if(checkbunny(attackid) && ((!(itemsbuf[attackid].flags & ITEM_FLAG6)) || checkmagiccost(attackid)))
			{
				if((itemsbuf[attackid].flags & ITEM_FLAG6) && !(misc_internal_link_flags & LF_PAID_CBYRNA_COST)){
					paymagiccost(attackid,true);
					misc_internal_link_flags |= LF_PAID_CBYRNA_COST;
				}
				SetAttack();
				attack=wCByrna;
				attackclk=0;
			}
		}
		else if((btnwpn==itype_bugnet)&&!((action==attacking||action==sideswimattacking) && attack==wBugNet)
				&& (directWpn>-1 ? (!item_disabled(directWpn) && itemsbuf[directWpn].family==itype_bugnet) : current_item(itype_bugnet)))
		{
			attackid = directWpn>-1 ? directWpn : current_item_id(itype_bugnet);
			if(checkbunny(attackid) && checkmagiccost(attackid))
			{
				paymagiccost(attackid);
				SetAttack();
				attack = wBugNet;
				attackclk = 0;
				sfx(itemsbuf[attackid].usesound);
			}
		}
		else
		{
			paidmagic = startwpn(directWpn>-1 ? directWpn : current_item_id(btnwpn));
			
			if(paidmagic)
			{
				if(action==casting || action==drowning || action==lavadrowning || action == sideswimcasting || action==sidedrowning)
				{
					;
				}
				else
				{
					SetAttack();
					attackclk=0;
					attack=none;
					
					if(btnwpn==itype_brang)
					{
						attack=wBrang;
					}
				}
			}
			else
			{
				// Weapon not started: directWpn should be reset to prev. value.
				directWpn = olddirectwpn;
			}
		}
		
		if(dowpn>-1 && itemsbuf[dowpn].script!=0 && !did_scriptb && (paidmagic || checkmagiccost(dowpn)) && !item_doscript[dowpn] && checkbunny(dowpn))
		{
			// Only charge for magic if item's magic cost wasn't already charged
			// for the item's main use.
			if(!paidmagic && attack!=wWand)
				paymagiccost(dowpn);
			//clear the item script stack for a new script
			//itemScriptData[(dowpn & 0xFFF)].Clear();
			ri = &(itemScriptData[dowpn]);
			for ( int32_t q = 0; q < 1024; q++ ) item_stack[dowpn][q] = 0xFFFF;
			ri->Clear();
			//for ( int32_t q = 0; q < 1024; q++ ) item_stack[(dowpn & 0xFFF)][q] = 0;
			//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[dowpn].script, dowpn & 0xFFF);
			item_doscript[dowpn] = 1;
			itemscriptInitialised[dowpn] = 0;
			ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[dowpn].script, dowpn);
			did_scriptb=true;
		}
		
		if(action==casting || action==drowning || action==lavadrowning || action == sideswimcasting || action==sidedrowning)
		{
			return;
		}
	}
	else
	{
		did_scriptb=false;
	}
	
	if(attackclk || action==attacking || action==sideswimattacking)
	{
		
		if((attackclk==0) && action!=sideswimattacking && getOnSideviewLadder() && (get_bit(quest_rules,qr_SIDEVIEWLADDER_FACEUP)!=0)) //Allow DIR to change if standing still on sideview ladder, and force-face up.
		{
			if((xoff==0)||diagonalMovement)
			{
				if(DrunkUp()) dir=up;
				if(DrunkDown()) dir=down;
			}
			
			if((yoff==0)||diagonalMovement)
			{
				if(DrunkLeft()) dir=left;
				if(DrunkRight()) dir=right;
			}
		}
		
		bool attacked = doattack();
		
		// This section below interferes with script-setting Link->Dir, so it comes after doattack
		if(!inlikelike && attackclk>4 && (attackclk&3)==0 && charging==0 && spins==0 && action!=sideswimattacking)
		{
			if((xoff==0)||diagonalMovement)
			{
				if(DrunkUp()) dir=up;
				
				if(DrunkDown()) dir=down;
			}
			
			if((yoff==0)||diagonalMovement)
			{
				if(DrunkLeft()) dir=left;
				
				if(DrunkRight()) dir=right;
			}
		}
		
		if(attacked && (charging==0 && spins<=5) && jumping<1 && action!=sideswimattacking)
		{
			return;
		}
		else if(!(attacked))
		{
			// Spin attack - change direction
			if(spins>1)
			{
				spins--;
				
				if(spins%5==0)
					sfx(itemsbuf[current_item_id(spins >5 ? itype_spinscroll2 : itype_spinscroll)].usesound,pan(x.getInt()));
					
				attackclk=1;
				
				switch(dir)
				{
				case up:
					dir=left;
					break;
					
				case right:
					dir=up;
					break;
					
				case down:
					dir=right;
					break;
					
				case left:
					dir=down;
					break;
				}
				
				return;
			}
			else
			{
				spins=0;
			}
			
			action=none; FFCore.setLinkAction(none);
			attackclk=0;
			charging=0;
		}
	}
	
	if(pitslide()) //Check pit's 'pull'. If true, then Link cannot fight the pull.
		return;
	
	if(action==walking) //still walking
	{
		if(!DrunkUp() && !DrunkDown() && !DrunkLeft() && !DrunkRight() && !autostep)
		{
			if(attackclk>0) SetAttack();
			else {action = none; FFCore.setLinkAction(none);}
			link_count=-1;
			return;
		}
		
		autostep=false;
		
		if(!(diagonalMovement || NO_GRIDLOCK))
		{
			if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
			{
				if(dir==up&&yoff)
				{
					info = walkflag(x,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]),2,up);
					if(blockmoving)
						info = info || walkflagMBlock(x+8,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]));
					execute(info);
					
					if(!info.isUnwalkable())
					{
						bool ffcwalk = true;
						//check for solid ffcs here -Z
						//This does work, however once the solif ffc stops Link from moving, the player can release the dpan, press again, and pass through it.
						for ( int32_t q = 0; q < 32; ++q )
						{
							//solid ffcs attampt -Z ( 30th March, 2019 )
							if ( !(tmpscr->ffflags[0]&ffSOLID) ) continue;
							{
								//al_trace("(int32_t)tmpscr->ffy[0] is %d\n",(int32_t)tmpscr->ffy[q]/10000);
								//al_trace("(int32_t)((tmpscr->ffheight[ri->ffcref]&0x3F)+1) is %d\n",(int32_t)((tmpscr->ffheight[q]&0x3F)+1));
								int32_t max_y = (((int32_t)tmpscr->ffy[q])/10000) + (int32_t)((tmpscr->ffheight[q]&0x3F)+1);
								//al_trace("max_y for ffc bottom edge is: %d\n", max_y);
								//al_trace("int32_t(lsteps[y.getInt()&7] is %d\n",int32_t(lsteps[y.getInt()&7]));
								//if ( (int32_t)y - int32_t(lsteps[y.getInt()&7]) == max_y ) //if the ffc bottom edge is in the step range
								if ( (int32_t)y == max_y ) //if the ffc bottom edge is in the step range
								{
									//al_trace("Link is under the ffc\n");
									int32_t linkwidthx = (int32_t)x+(int32_t)hxsz;
									//al_trace("linkwidthx is: %d\n",linkwidthx);
									if ( linkwidthx >= (((int32_t)tmpscr->ffx[q])/10000) && (int32_t)x < ( (((int32_t)tmpscr->ffx[q])/10000) + (int32_t)(tmpscr->ffwidth[q]&0x3F)+1) )
									{
										al_trace("Link is under X border of ffc\n");
										//Link is under the ffc
										ffcwalk = false;
									}
								}
							}
						}
						
						if ( ffcwalk ) move(up);
					}
					else
					{
						action=none; FFCore.setLinkAction(none);
					}
					
					return;
				}
				
				if(dir==down&&yoff)
				{
					info = walkflag(x,y+15+int32_t(lsteps[y.getInt()&7]),2,down);
					if(blockmoving)
						info = info || walkflagMBlock(x+8,y+15+int32_t(lsteps[y.getInt()&7]));
					execute(info);
					
					if(!info.isUnwalkable())
					{
						bool ffcwalk = true;
						//solid ffcs attampt -Z ( 30th March, 2019 )
						//check for solid ffcs here -Z
						for ( int32_t q = 0; q < 32; ++q )
						{
							if ( !(tmpscr->ffflags[0]&ffSOLID) ) continue;
							{
								int32_t min_y = (((int32_t)tmpscr->ffy[0])/10000);
								//if ( (int32_t)y+(int32_t)hysz + int32_t(lsteps[y.getInt()&7]) > min_y ) //if the ffc bottom edge is in the step range
								//if ( (int32_t)y+(int32_t)hysz + 1 > min_y ) //if the ffc bottom edge is in the step range
								if ( (int32_t)y+(int32_t)hysz == min_y ) //if the ffc bottom edge is in the step range
								{
									//al_trace("Link is under the ffc\n");
									int32_t linkwidthx = (int32_t)x+(int32_t)hxsz;
									//al_trace("linkwidthx is: %d\n",linkwidthx);
									if ( linkwidthx >= (((int32_t)tmpscr->ffx[0])/10000) && (int32_t)x < ( (((int32_t)tmpscr->ffx[0])/10000) + (int32_t)(tmpscr->ffwidth[0]&0x3F)+1) )
									{
									//	al_trace("Link is under X border of ffc\n");
										//Link is under the ffc
										ffcwalk = false;
									}
								}
							}
						}
						
						if ( ffcwalk )
							move(down);
					}
					else
					{
						action=none; FFCore.setLinkAction(none);
					}
					
					return;
				}
				
				if(dir==left&&xoff)
				{
					info = walkflag(x-int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,left) || walkflag(x-int32_t(lsteps[x.getInt()&7]),y+8,1,left);
					execute(info);
					
					if(!info.isUnwalkable())
					{
						move(left);
					}
					else
					{
						action=none; FFCore.setLinkAction(none);
					}
					
					return;
				}
				
				if(dir==right&&xoff)
				{
					info = walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,right) || walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+8,1,right);
					execute(info);
					
					if(!info.isUnwalkable())
					{
						move(right);
					}
					else
					{
						action=none; FFCore.setLinkAction(none);
					}
					
					return;
				}
			}
			else
			{
				if(dir==up&&yoff)
				{
					while(true)
					{
						info = walkflag(temp_x,temp_y+(bigHitbox?0:8)-temp_step,2,up);
						if(blockmoving)
							info = info || walkflagMBlock(temp_x+8,temp_y+(bigHitbox?0:8)-temp_step);
						execute(info);
						
						if(!info.isUnwalkable())
						{
							bool ffcwalk = true;
							//check for solid ffcs here -Z
							//This does work, however once the solif ffc stops Link from moving, the player can release the dpan, press again, and pass through it.
							for ( int32_t q = 0; q < 32; ++q )
							{
								//solid ffcs attampt -Z ( 30th March, 2019 )
								if ( !(tmpscr->ffflags[0]&ffSOLID) ) continue;
								//al_trace("(int32_t)tmpscr->ffy[0] is %d\n",(int32_t)tmpscr->ffy[q]/10000);
								//al_trace("(int32_t)((tmpscr->ffheight[ri->ffcref]&0x3F)+1) is %d\n",(int32_t)((tmpscr->ffheight[q]&0x3F)+1));
								int32_t max_y = (((int32_t)tmpscr->ffy[q])/10000) + (int32_t)((tmpscr->ffheight[q]&0x3F)+1);
								//al_trace("max_y for ffc bottom edge is: %d\n", max_y);
								//al_trace("int32_t(lsteps[y.getInt()&7] is %d\n",int32_t(lsteps[y.getInt()&7]));
								//if ( (int32_t)y - int32_t(lsteps[y.getInt()&7]) == max_y ) //if the ffc bottom edge is in the step range
								if ( temp_y.getInt() == max_y ) //if the ffc bottom edge is in the step range
								{
									//al_trace("Link is under the ffc\n");
									int32_t linkwidthx = temp_x.getInt()+(int32_t)hxsz;
									//al_trace("linkwidthx is: %d\n",linkwidthx);
									if ( linkwidthx >= (((int32_t)tmpscr->ffx[q])/10000) && temp_x.getInt() < ( (((int32_t)tmpscr->ffx[q])/10000) + (int32_t)(tmpscr->ffwidth[q]&0x3F)+1) )
									{
										al_trace("Link is under X border of ffc\n");
										//Link is under the ffc
										ffcwalk = false;
									}
								}
							}
							
							if ( ffcwalk )
							{
								link_newstep = temp_step;
								x = temp_x;
								y = temp_y;
								move(up);
								return;
							}
						}
						//Could not move, try moving less
						if(temp_y != int32_t(temp_y))
						{
							temp_y = floor((double)temp_y);
							continue;
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
							continue;
						}
						else //Can't move less, stop moving
						{
							action=none; FFCore.setLinkAction(none);
						}
						return;
					}
				}
				
				if(dir==down&&yoff)
				{
					while(true)
					{
						info = walkflag(temp_x,temp_y+15+temp_step,2,down);
						if(blockmoving)
							info = info || walkflagMBlock(temp_x+8,temp_y+15+temp_step);
						execute(info);
						
						if(!info.isUnwalkable())
						{
							bool ffcwalk = true;
							//solid ffcs attampt -Z ( 30th March, 2019 )
							//check for solid ffcs here -Z
							for ( int32_t q = 0; q < 32; ++q )
							{
								if ( !(tmpscr->ffflags[0]&ffSOLID) ) continue;
								{
									int32_t min_y = (((int32_t)tmpscr->ffy[0])/10000);
									//if ( temp_y.getInt()+(int32_t)hysz + temp_step > min_y ) //if the ffc bottom edge is in the step range
									//if ( temp_y.getInt()+(int32_t)hysz + 1 > min_y ) //if the ffc bottom edge is in the step range
									if ( temp_y.getInt()+(int32_t)hysz == min_y ) //if the ffc bottom edge is in the step range
									{
										//al_trace("Link is under the ffc\n");
										int32_t linkwidthx = temp_x.getInt()+(int32_t)hxsz;
										//al_trace("linkwidthx is: %d\n",linkwidthx);
										if ( linkwidthx >= (((int32_t)tmpscr->ffx[0])/10000) && temp_x.getInt() < ( (((int32_t)tmpscr->ffx[0])/10000) + (int32_t)(tmpscr->ffwidth[0]&0x3F)+1) )
										{
										//	al_trace("Link is under X border of ffc\n");
											//Link is under the ffc
											ffcwalk = false;
										}
									}
								}
							}
							
							if ( ffcwalk )
							{
								link_newstep = temp_step;
								x = temp_x;
								y = temp_y;
								move(down);
								return;
							}
						}
						//Could not move, try moving less
						if(temp_y != int32_t(temp_y))
						{
							temp_y = floor((double)temp_y);
							continue;
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
							continue;
						}
						else //Can't move less, stop moving
						{
							action=none; FFCore.setLinkAction(none);
						}
						return;
					}
				}
				
				if(dir==left&&xoff)
				{
					while(true)
					{
						info = walkflag(temp_x-temp_step,temp_y+(bigHitbox?0:8),1,left) || walkflag(temp_x-temp_step,temp_y+8,1,left);
						execute(info);
						
						if(!info.isUnwalkable())
						{
							link_newstep = temp_step;
							x = temp_x;
							y = temp_y;
							move(left);
							return;
						}
						//Could not move, try moving less
						if(temp_x != int32_t(temp_x))
						{
							temp_x = floor((double)temp_x);
							continue;
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
							continue;
						}
						else //Can't move less, stop moving
						{
							action=none; FFCore.setLinkAction(none);
						}
						return;
					}
				}
				
				if(dir==right&&xoff)
				{
					while(true)
					{
						info = walkflag(temp_x+15+temp_step,temp_y+(bigHitbox?0:8),1,right) || walkflag(temp_x+15+temp_step,temp_y+8,1,right);
						execute(info);
						
						if(!info.isUnwalkable())
						{
							link_newstep = temp_step;
							x = temp_x;
							y = temp_y;
							move(right);
							return;
						}
						//Could not move, try moving less
						if(temp_x != int32_t(temp_x))
						{
							temp_x = floor((double)temp_x);
							continue;
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
							continue;
						}
						else //Can't move less, stop moving
						{
							action=none; FFCore.setLinkAction(none);
						}
						return;
					}
				}
			}
		}
	
	} // endif (action==walking)
	
	if((action!=swimming)&&(action!=sideswimming)&&(action !=sideswimhit)&&(action !=sideswimattacking)&&(action!=casting)&&(action!=sideswimcasting)&&(action!=drowning)&&(action!=sidedrowning)&&(action!=lavadrowning) && charging==0 && spins==0 && jumping<1)
	{
		action=none; FFCore.setLinkAction(none);
	}
	
	if(diagonalMovement)
	{
		switch(holddir)
		{
		case up:
			if(!Up())
			{
				holddir=-1;
			}
			
			break;
			
		case down:
			if(!Down())
			{
				holddir=-1;
			}
			
			break;
			
		case left:
			if(!Left())
			{
				holddir=-1;
			}
			
			break;
			
		case right:
			if(!Right())
			{
				holddir=-1;
			}
			
			break;
			
		default:
			break;
		} //end switch
		
		if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim()) //!DIRECTION SET
		{
			walkable = false;
			if(DrunkUp()&&(holddir==-1||holddir==up))
			{
				if(isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0 && action != sideswimattacking && !(IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR)))
					{
						dir=up;
					}
					
					holddir=up;
					
					if(DrunkRight()&&shiftdir!=left)
					{
						shiftdir=right;
						if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (charging==0 && spins==0)) dir = right;
						if (!IsSideSwim() || (charging==0 && spins==0)) sideswimdir = right;
					}
					else if(DrunkLeft()&&shiftdir!=right)
					{
						shiftdir=left;
						if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (charging==0 && spins==0)) dir = left;
						if (!IsSideSwim() || (charging==0 && spins==0)) sideswimdir = left;
					}
					else
					{
						shiftdir=-1;
					}
					
					//walkable if Ladder can be placed or is already placed vertically
					if(isSideViewLink() && !toogam && (!get_bit(quest_rules, qr_OLD_LADDER_ITEM_SIDEVIEW) || !(can_deploy_ladder() || (ladderx && laddery && ladderdir==up))) && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						walkable=false;
					}
					else
					{
						do
						{
							info = walkflag(x,(bigHitbox?0:8)+(y-link_newstep),2,up);
							
							if(x.getFloor() & 7)
								info = info || walkflag(x+16,(bigHitbox?0:8)+(y-link_newstep),1,up);
							else if(blockmoving)
								info = info || walkflagMBlock(x+16, (bigHitbox?0:8)+(y-link_newstep));
								
							execute(info);
							
							if(info.isUnwalkable())
							{
								if(y != y.getInt())
								{
									y.doRound();
								}
								else if(link_newstep > 1)
								{
									if(link_newstep != int32_t(link_newstep)) //floor
										link_newstep = floor((double)link_newstep);
									else --link_newstep;
								}
								else
									break;
							}
							else walkable = true;
						}
						while(!walkable);
					}
					
					int32_t s=shiftdir;
					
					if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==left)
						{
							do
							{
								info = (walkflag(x-link_newstep_diag,y+(bigHitbox?0:8),1,left)||walkflag(x-link_newstep_diag,y+15,1,left));
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(x != x.getInt())
									{
										x.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x-link_newstep_diag,(bigHitbox?0:8)+(y-link_newstep),1,left);
										execute(info);
										if(info.isUnwalkable())
										{
											if(x != x.getInt())
											{
												x.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
						else if(s==right)
						{
							do
							{
								info = (walkflag(x+15+link_newstep_diag,y+(bigHitbox?0:8),1,right)||walkflag(x+15+link_newstep_diag,y+15,1,right));
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(x != x.getInt())
									{
										x.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x+15+link_newstep_diag,(bigHitbox?0:8)+(y-link_newstep),1,right);
										execute(info);
										if(info.isUnwalkable())
										{
											if(x != x.getInt())
											{
												x.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
					}
					
					move(up);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							x = x.getInt();
							y = y.getInt();
							if(!_walkflag(x,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
									!_walkflag(x+8, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
									_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+(bigHitbox?0:8)-1))
									sprite::move((zfix)-1,(zfix)0);
							}
							else
							{
								if(_walkflag(x,   y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
										!_walkflag(x+7, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
										!_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
								{
									if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+(bigHitbox?0:8)-1))
										sprite::move((zfix)1,(zfix)0);
								}
								else
								{
									pushing=push+1;
								}
							}
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
						}
					}
					
					return;
				}
			}
			
			if(DrunkDown()&&(holddir==-1||holddir==down))
			{
				if(isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0 && action != sideswimattacking && !(IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR)))
					{
						dir=down;
					}
					
					holddir=down;
					
					if(DrunkRight()&&shiftdir!=left)
					{
						shiftdir=right;
						if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (charging==0 && spins==0)) dir = right;
						if (!IsSideSwim() || (charging==0 && spins==0)) sideswimdir = right;
					}
					else if(DrunkLeft()&&shiftdir!=right)
					{
						shiftdir=left;
						if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (charging==0 && spins==0)) dir = left;
						if (!IsSideSwim() || (charging==0 && spins==0)) sideswimdir = left;
					}
					else
					{
						shiftdir=-1;
					}
					
					//bool walkable;
					if(isSideViewLink() && !toogam && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						walkable=false;
					}
					else
					{
						do
						{
							info = walkflag(x,15+(y+link_newstep),2,down);
							
							if(x.getFloor() & 7)
								info = info || walkflag(x+16,15+(y+link_newstep),1,down);
							else if(blockmoving)
								info = info || walkflagMBlock(x+16, 15+(y+link_newstep));
								
							execute(info);
							
							if(info.isUnwalkable())
							{
								if(y != y.getInt())
								{
									y.doRound();
								}
								else if(link_newstep > 1)
								{
									if(link_newstep != int32_t(link_newstep)) //floor
										link_newstep = floor((double)link_newstep);
									else --link_newstep;
								}
								else
									break;
							}
							else walkable = true;
						}
						while(!walkable);
					}
					
					int32_t s=shiftdir;
					
					if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==left)
						{
							do
							{
								info = (walkflag(x-link_newstep_diag,y+(bigHitbox?0:8),1,left)||walkflag(x-link_newstep_diag,y+15,1,left));
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(x != x.getInt())
									{
										x.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x-link_newstep_diag,15+(y+link_newstep),1,left);
										execute(info);
										if(info.isUnwalkable())
										{
											if(x != x.getInt())
											{
												x.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
						else if(s==right)
						{
							do
							{
								info = (walkflag(x+15+link_newstep_diag,y+(bigHitbox?0:8),1,right)||walkflag(x+15+link_newstep_diag,y+15,1,right));
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(x != x.getInt())
									{
										x.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x+15+link_newstep_diag,15+(y+link_newstep),1,right);
										execute(info);
										if(info.isUnwalkable())
										{
											if(x != x.getInt())
											{
												x.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
					}
					
					move(down);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							x = x.getInt();
							y = y.getInt();
							if(!_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+8, y+15+1,1,SWITCHBLOCK_STATE)&&
									_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+15+1))
									sprite::move((zfix)-1,(zfix)0);
							}
							else if(_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+7, y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+15+1))
									sprite::move((zfix)1,(zfix)0);
							}
							else
							{
								pushing=push+1;
							}
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
						}
					}
					
					return;
				}
			}
			
			if(DrunkLeft()&&(holddir==-1||holddir==left))
			{
				if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0 && action != sideswimattacking)
					{
						dir=left;
					}
					sideswimdir = left;
					
					holddir=left;
					
					if(DrunkUp()&&shiftdir!=down)
					{
						shiftdir=up;
					}
					else if(DrunkDown()&&shiftdir!=up)
					{
						shiftdir=down;
					}
					else
					{
						shiftdir=-1;
					}
					
					do
					{
						info = walkflag(x-link_newstep,y+(bigHitbox?0:8),1,left)||walkflag(x-link_newstep,y+8,1,left);
						
						if(y.getFloor() & 7)
							info = info || walkflag(x-link_newstep,y+16,1,left);
							
						execute(info);
						
						if(info.isUnwalkable())
						{
							if(x != x.getInt())
							{
								x.doRound();
							}
							else if(link_newstep > 1)
							{
								if(link_newstep != int32_t(link_newstep)) //floor
									link_newstep = floor((double)link_newstep);
								else --link_newstep;
							}
							else
								break;
						}
						else walkable = true;
					}
					while(!walkable);
					
					int32_t s=shiftdir;
					
					if((isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM)) || (isSideViewLink() && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==up)
						{
							do
							{
								info = walkflag(x,y+(bigHitbox?0:8)-link_newstep_diag,2,up)||walkflag(x+15,y+(bigHitbox?0:8)-link_newstep_diag,1,up);
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(y != y.getInt())
									{
										y.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x-link_newstep,y+(bigHitbox?0:8)-link_newstep_diag,1,up);
										execute(info);
										if(info.isUnwalkable())
										{
											if(y != y.getInt())
											{
												y.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
						else if(s==down)
						{
							do
							{
								info = walkflag(x,y+15+link_newstep_diag,2,down)||walkflag(x+15,y+15+link_newstep_diag,1,down);
								
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(y != y.getInt())
									{
										y.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x-link_newstep,y+15+link_newstep_diag,1,down);
										execute(info);
										if(info.isUnwalkable())
										{
											if(y != y.getInt())
											{
												y.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
					}
					
					move(left);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							x = x.getInt();
							y = y.getInt();
							int32_t v1=bigHitbox?0:8;
							int32_t v2=bigHitbox?8:12;
							
							if(!_walkflag(x-1,y+v1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+v2,1,SWITCHBLOCK_STATE)&&
									_walkflag(x-1,y+15,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+15))
									sprite::move((zfix)0,(zfix)-1);
							}
							else if(_walkflag(x-1,y+v1,  1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+v2-1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+15,  1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+v1))
									sprite::move((zfix)0,(zfix)1);
							}
							else
							{
								pushing=push+1;
							}
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							
							if(action!=swimming)
							{
							}
						}
					}
					
					return;
				}
			}
			
			if(DrunkRight()&&(holddir==-1||holddir==right))
			{
				if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0 && action != sideswimattacking)
					{
						dir=right;
					}
					sideswimdir = right;
					
					holddir=right;
					
					if(DrunkUp()&&shiftdir!=down)
					{
						shiftdir=up;
					}
					else if(DrunkDown()&&shiftdir!=up)
					{
						shiftdir=down;
					}
					else
					{
						shiftdir=-1;
					}
					
					do
					{
						info = walkflag(x+15+link_newstep,y+(bigHitbox?0:8),1,right)||walkflag(x+15+link_newstep,y+8,1,right);;
						
						if(y.getFloor() & 7)
							info = info || walkflag(x+15+link_newstep,y+16,1,right);
							
						execute(info);
						
						if(info.isUnwalkable())
						{
							if(x != x.getInt())
							{
								x.doRound();
							}
							else if(link_newstep > 1)
							{
								if(link_newstep != int32_t(link_newstep)) //floor
									link_newstep = floor((double)link_newstep);
								else --link_newstep;
							}
							else
								break;
						}
						else walkable = true;
					}
					while(!walkable);
					
					int32_t s=shiftdir;
					
					if((isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM)) || (isSideViewLink() && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==up)
						{
							do
							{
								info = walkflag(x,y+(bigHitbox?0:8)-link_newstep_diag,2,up)||walkflag(x+15,y+(bigHitbox?0:8)-link_newstep_diag,1,up);
								
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(y != y.getInt())
									{
										y.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x+15+link_newstep,y+(bigHitbox?0:8)-link_newstep_diag,1,up);
										execute(info);
										if(info.isUnwalkable())
										{
											if(y != y.getInt())
											{
												y.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
						else if(s==down)
						{
							do
							{
								info = walkflag(x,y+15+link_newstep_diag,2,down)||walkflag(x+15,y+15+link_newstep_diag,1,down);
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									if(y != y.getInt())
									{
										y.doRound();
									}
									else if(link_newstep_diag > 1)
									{
										if(link_newstep_diag != link_newstep_diag.getInt()) //floor
											link_newstep_diag.doFloor();
										else --link_newstep_diag;
									}
									else
										shiftdir = -1;
								}
								else if(walkable)
								{
									do
									{
										info = walkflag(x+15+link_newstep,y+15+link_newstep_diag,1,down);
										execute(info);
										if(info.isUnwalkable())
										{
											if(y != y.getInt())
											{
												y.doRound();
											}
											else if(link_newstep_diag > 1)
											{
												if(link_newstep_diag != link_newstep_diag.getInt()) //floor
													link_newstep_diag.doFloor();
												else --link_newstep_diag;
											}
											else
												shiftdir = -1;
										}
										else break;
									}
									while(shiftdir != -1);
									break;
								}
								else break;
							}
							while(shiftdir != -1);
						}
					}
					
					move(right);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							x = x.getInt();
							y = y.getInt();
							int32_t v1=bigHitbox?0:8;
							int32_t v2=bigHitbox?8:12;
								   
							if(!_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
								   !_walkflag(x+16,y+v2,1,SWITCHBLOCK_STATE)&&
								   _walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+15))
									sprite::move((zfix)0,(zfix)-1);
							}
							else if(_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
									   !_walkflag(x+16,y+v2-1,1,SWITCHBLOCK_STATE)&&
									   !_walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+v1))
									sprite::move((zfix)0,(zfix)1);
							}
							else
							{
								pushing=push+1;
								z3step=2;
							}
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							
							if(action!=swimming)
							{
							}
						}
					}
					
					return;
				}
			}
		}
		else
		{
			if(DrunkUp()&&(holddir==-1||holddir==up))
			{
				if(isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0)
					{
						dir=up;
					}
					
					holddir=up;
					
					if(DrunkRight()&&shiftdir!=left)
					{
						shiftdir=right;
					}
					else if(DrunkLeft()&&shiftdir!=right)
					{
						shiftdir=left;
					}
					else
					{
						shiftdir=-1;
					}
					
					//walkable if Ladder can be placed or is already placed vertically
					if(isSideViewLink() && !toogam && !(can_deploy_ladder() || (ladderx && laddery && ladderdir==up)) && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						walkable=false;
					}
					else
					{
						info = walkflag(x,y+(bigHitbox?0:8)-z3step,2,up);
						
						if(x.getInt() & 7)
							info = info || walkflag(x+16,y+(bigHitbox?0:8)-z3step,1,up);
						else if(blockmoving)
							info = info || walkflagMBlock(x+16, y+(bigHitbox?0:8)-z3step);
							
						execute(info);
						
						if(info.isUnwalkable())
						{
							if(z3step==2)
							{
								z3step=1;
								info = walkflag(x,y+(bigHitbox?0:8)-z3step,2,up);
								
								if(x.getInt()&7)
									info = info || walkflag(x+16,y+(bigHitbox?0:8)-z3step,1,up);
								else if(blockmoving)
									info = info || walkflagMBlock(x+16, y+(bigHitbox?0:8)-z3step);
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									walkable = false;
								}
								else
								{
									walkable=true;
								}
							}
							else
							{
								walkable=false;
							}
						}
						else
						{
							walkable = true;
						}
					}
					
					int32_t s=shiftdir;
					
					if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==left)
						{
							info = (walkflag(x-1,y+(bigHitbox?0:8),1,left)||walkflag(x-1,y+15,1,left));
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x-1,y+(bigHitbox?0:8)-1,1,left);
								execute(info);
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
						else if(s==right)
						{
							info = walkflag(x+16,y+(bigHitbox?0:8),1,right)||walkflag(x+16,y+15,1,right);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x+16,y+(bigHitbox?0:8)-1,1,right);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
					}
					
					move(up);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							if(!_walkflag(x,   y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
									!_walkflag(x+8, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
									_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+(bigHitbox?0:8)-1))
									sprite::move((zfix)-1,(zfix)0);
							}
							else
							{
								if(_walkflag(x,   y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
										!_walkflag(x+7, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
										!_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
								{
									if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+(bigHitbox?0:8)-1))
										sprite::move((zfix)1,(zfix)0);
								}
								else
								{
									pushing=push+1;
								}
							}
							
							z3step=2;
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							z3step=2;
						}
					}
					
					return;
				}
			}
			
			if(DrunkDown()&&(holddir==-1||holddir==down))
			{
				if(isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0)
					{
						dir=down;
					}
					
					holddir=down;
					
					if(DrunkRight()&&shiftdir!=left)
					{
						shiftdir=right;
					}
					else if(DrunkLeft()&&shiftdir!=right)
					{
						shiftdir=left;
					}
					else
					{
						shiftdir=-1;
					}
					
					//bool walkable;
					if(isSideViewLink() && !toogam && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						walkable=false;
					}
					else
					{
						info = walkflag(x,y+15+z3step,2,down);
						
						if(x.getInt()&7)
							info = info || walkflag(x+16,y+15+z3step,1,down);
						else if(blockmoving)
							info = info || walkflagMBlock(x+16, y+15+z3step);
						
						execute(info);
						
						if(info.isUnwalkable())
						{
							if(z3step==2)
							{
								z3step=1;
								info = walkflag(x,y+15+z3step,2,down);
								
								if(x.getInt()&7)
									info = info || walkflag(x+16,y+15+z3step,1,down);
								else if(blockmoving)
									info = info || walkflagMBlock(x+16, y+15+z3step);
									
								execute(info);
								
								if(info.isUnwalkable())
								{
									walkable = false;
								}
								else
								{
									walkable=true;
								}
							}
							else
							{
								walkable=false;
							}
						}
						else
						{
							walkable = true;
						}
					}
					
					int32_t s=shiftdir;
					
					if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==left)
						{
							info = walkflag(x-1,y+(bigHitbox?0:8),1,left)||walkflag(x-1,y+15,1,left);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x-1,y+16,1,left);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
						else if(s==right)
						{
							info = walkflag(x+16,y+(bigHitbox?0:8),1,right)||walkflag(x+16,y+15,1,right);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x+16,y+16,1,right);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
					}
					
					move(down);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							if(!_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+8, y+15+1,1,SWITCHBLOCK_STATE)&&
									_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+15+1))
									sprite::move((zfix)-1,(zfix)0);
							}
							else if(_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+7, y+15+1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+15+1))
									sprite::move((zfix)1,(zfix)0);
							}
							else //if(shiftdir==-1)
							{
								pushing=push+1;
								
								if(action!=swimming)
								{
								}
							}
							
							z3step=2;
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							
							if(action!=swimming)
							{
							}
							
							z3step=2;
						}
					}
					
					return;
				}
			}
			
			if(DrunkLeft()&&(holddir==-1||holddir==left))
			{
				if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0)
					{
						dir=left;
					}
					
					holddir=left;
					
					if(DrunkUp()&&shiftdir!=down)
					{
						shiftdir=up;
					}
					else if(DrunkDown()&&shiftdir!=up)
					{
						shiftdir=down;
					}
					else
					{
						shiftdir=-1;
					}
					
					//bool walkable;
					info = walkflag(x-z3step,y+(bigHitbox?0:8),1,left)||walkflag(x-z3step,y+8,1,left);
					
					if(y.getInt()&7)
						info = info || walkflag(x-z3step,y+16,1,left);
						
					execute(info);
					
					if(info.isUnwalkable())
					{
						if(z3step==2)
						{
							z3step=1;
							info = walkflag(x-z3step,y+(bigHitbox?0:8),1,left)||walkflag(x-z3step,y+8,1,left);
							
							if(y.getInt()&7)
								info = info || walkflag(x-z3step,y+16,1,left);
								
							execute(info);
							
							if(info.isUnwalkable())
							{
								walkable = false;
							}
							else
							{
								walkable=true;
							}
						}
						else
						{
							walkable=false;
						}
					}
					else
					{
						walkable = true;
					}
					
					int32_t s=shiftdir;
					
					if((isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM)) || (isSideViewLink() && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==up)
						{
							info = walkflag(x,y+(bigHitbox?0:8)-1,2,up)||walkflag(x+15,y+(bigHitbox?0:8)-1,1,up);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x-1,y+(bigHitbox?0:8)-1,1,up);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
						else if(s==down)
						{
							info = walkflag(x,y+16,2,down)||walkflag(x+15,y+16,1,down);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x-1,y+16,1,down);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
					}
					
					move(left);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							int32_t v1=bigHitbox?0:8;
							int32_t v2=bigHitbox?8:12;
							
							if(!_walkflag(x-1,y+v1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+v2,1,SWITCHBLOCK_STATE)&&
									_walkflag(x-1,y+15,1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+15))
									sprite::move((zfix)0,(zfix)-1);
							}
							else if(_walkflag(x-1,y+v1,  1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+v2-1,1,SWITCHBLOCK_STATE)&&
									!_walkflag(x-1,y+15,  1,SWITCHBLOCK_STATE))
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+v1))
									sprite::move((zfix)0,(zfix)1);
							}
							else //if(shiftdir==-1)
							{
								pushing=push+1;
								
								if(action!=swimming)
								{
								}
							}
							
							z3step=2;
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							
							if(action!=swimming)
							{
							}
							
							z3step=2;
						}
					}
					
					return;
				}
			}
			
			if(DrunkRight()&&(holddir==-1||holddir==right))
			{
				if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
				{
				}
				else
				{
					if(charging==0 && spins==0)
					{
						dir=right;
					}
					
					holddir=right;
					
					if(DrunkUp()&&shiftdir!=down)
					{
						shiftdir=up;
					}
					else if(DrunkDown()&&shiftdir!=up)
					{
						shiftdir=down;
					}
					else
					{
						shiftdir=-1;
					}
					
					//bool walkable;
					info = walkflag(x+15+z3step,y+(bigHitbox?0:8),1,right)||walkflag(x+15+z3step,y+8,1,right);
					
					if(y.getInt()&7)
						info = info || walkflag(x+15+z3step,y+16,1,right);
						
					execute(info);
					
					if(info.isUnwalkable())
					{
						if(z3step==2)
						{
							z3step=1;
							info = walkflag(x+15+z3step,y+(bigHitbox?0:8),1,right)||walkflag(x+15+z3step,y+8,1,right);
							
							if(y.getInt()&7)
								info = info || walkflag(x+15+z3step,y+16,1,right);
								
							execute(info);
							
							if(info.isUnwalkable())
							{
								walkable = false;
							}
							else
							{
								walkable=true;
							}
						}
						else
						{
							walkable=false;
						}
					}
					else
					{
						walkable = true;
					}
					
					int32_t s=shiftdir;
					
					if((isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM)) || (isSideViewLink() && !getOnSideviewLadder() && action != sideswimming && action != sideswimhit && action != sideswimattacking))
					{
						shiftdir=-1;
					}
					else
					{
						if(s==up)
						{
							info = walkflag(x,y+(bigHitbox?0:8)-1,2,up)||walkflag(x+15,y+(bigHitbox?0:8)-1,1,up);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x+16,y+(bigHitbox?0:8)-1,1,up);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
						else if(s==down)
						{
							info = walkflag(x,y+16,2,down)||walkflag(x+15,y+16,1,down);
							execute(info);
							
							if(info.isUnwalkable())
							{
								shiftdir=-1;
							}
							else if(walkable)
							{
								info = walkflag(x+16,y+16,1,down);
								execute(info);
								
								if(info.isUnwalkable())
								{
									shiftdir=-1;
								}
							}
						}
					}
					
					move(right);
					shiftdir=s;
					
					if(!walkable)
					{
						if(shiftdir==-1) //Corner-shove; prevent being stuck on corners -V
						{
							int32_t v1=bigHitbox?0:8;
							int32_t v2=bigHitbox?8:12;
							
							info = !walkflag(x+16,y+v1,1,right)&&
								   !walkflag(x+16,y+v2,1,right)&&
								   walkflag(x+16,y+15,1,right);
								   
							//do NOT execute these
							if(info.isUnwalkable())
							{
								if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+15))
									sprite::move((zfix)0,(zfix)-1);
							}
							else
							{
								info = walkflag(x+16,y+v1,  1,right)&&
									   !walkflag(x+16,y+v2-1,1,right)&&
									   !walkflag(x+16,y+15,  1,right);
									   
								if(info.isUnwalkable())
								{
									if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+v1))
										sprite::move((zfix)0,(zfix)1);
								}
								else //if(shiftdir==-1)
								{
									pushing=push+1;
									z3step=2;
									
									if(action!=swimming)
									{
									}
								}
							}
							
							z3step=2;
						}
						else
						{
							pushing=push+1; // L: This makes solid damage combos and diagonal-triggered Armoses work.
							
							if(action!=swimming)
							{
							}
							
							z3step=2;
						}
					}
					
					return;
				}
			}
		}
		
		int32_t wtry  = iswaterex(MAPCOMBO(x,y+15), currmap, currscr, -1, x,y+15, true, false);
		int32_t wtry8 = iswaterex(MAPCOMBO(x+15,y+15), currmap, currscr, -1, x+15,y+15, true, false);
		int32_t wtrx = iswaterex(MAPCOMBO(x,y+(bigHitbox?0:8)), currmap, currscr, -1, x,y+(bigHitbox?0:8), true, false);
		int32_t wtrx8 = iswaterex(MAPCOMBO(x+15,y+(bigHitbox?0:8)), currmap, currscr, -1, x+15,y+(bigHitbox?0:8), true, false);
		int32_t wtrc = iswaterex(MAPCOMBO(x+8,y+(bigHitbox?8:12)), currmap, currscr, -1, x+8,y+(bigHitbox?8:12), true, false);
		
		if(can_use_item(itype_flippers,i_flippers)&&current_item(itype_flippers) >= combobuf[wtrc].attribytes[0]&&(!(combobuf[wtrc].usrflags&cflag1) || (itemsbuf[current_item_id(itype_flippers)].flags & ITEM_FLAG3))&&!(ladderx+laddery)&&z==0)
		{
			if(wtrx&&wtrx8&&wtry&&wtry8 && !DRIEDLAKE)
			{
				//action=swimming;
				if(action !=none && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && !isSideViewLink())
				{
					hopclk = 0xFF;
				}
			}
		}
		
		return;
	} //endif (LTTPWALK)
	temp_step = link_newstep;
	temp_x = x;
	temp_y = y;
	
	if(isdungeon() && (x<=26 || x>=214) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
	{
		if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
			goto LEFTRIGHT_OLDMOVE;
		else goto LEFTRIGHT_NEWMOVE;
	}
	
	// make it easier to get in left & right doors
	
	//ignore ladder for this part. sigh sigh sigh -DD
	oldladderx = ladderx;
	oldladdery = laddery;
	if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
	{
		if(isdungeon() && DrunkLeft() && (temp_x==32 && temp_y==80))
		{
			do
			{
				info = walkflag(temp_x,temp_y+(bigHitbox?0:8),1,left) ||
					   walkflag(temp_x-temp_step,temp_y+(bigHitbox?0:8),1,left);
				
				if(info.isUnwalkable())
				{
					if(temp_x != int32_t(temp_x))
					{
						temp_x = floor((double)temp_x);
					}
					else if(temp_step > 1)
					{
						if(temp_step != int32_t(temp_step)) //floor
							temp_step = floor((double)temp_step);
						else --temp_step;
					}
					else
						break;
				}
			}
			while(info.isUnwalkable());
			
			if(!info.isUnwalkable())
			{
				x = temp_x;
				y = temp_y;
				link_newstep = temp_step;
				//ONLY process the side-effects of the above walkflag if Link will actually move
				//sigh sigh sigh... walkflag is a horrible mess :-/ -DD
				execute(info);
				move(left);
				return;
			}
			temp_x = x;
			temp_y = y;
			temp_step = link_newstep;
		}
		
		if(isdungeon() && DrunkRight() && temp_x==208 && temp_y==80)
		{
			do
			{
				info = walkflag(temp_x+15+temp_step,temp_y+(bigHitbox?0:8),1,right) ||
				   walkflag(temp_x+15+temp_step,temp_y+8,1,right);
				
				if(info.isUnwalkable())
				{
					if(temp_x != int32_t(temp_x))
					{
						temp_x = floor((double)temp_x);
					}
					else if(temp_step > 1)
					{
						if(temp_step != int32_t(temp_step)) //floor
							temp_step = floor((double)temp_step);
						else --temp_step;
					}
					else
						break;
				}
			}
			while(info.isUnwalkable());
			
			if(!info.isUnwalkable())
			{
				x = temp_x;
				y = temp_y;
				link_newstep = temp_step;
				execute(info);
				move(right);
				return;
			}
			temp_x = x;
			temp_y = y;
			temp_step = link_newstep;
		}
		
		ladderx = oldladderx;
		laddery = oldladdery;
		
		if(DrunkUp())
		{
			if(xoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=up && dir!=down)
				{
					if(xoff>2&&xoff<6)
					{
						move(dir);
					}
					else if(xoff>=6)
					{
						move(right);
					}
					else if(xoff>=1)
					{
						move(left);
					}
				}
				else
				{
					if(xoff>=4)
					{
						move(right);
					}
					else if(xoff<4)
					{
						move(left);
					}
				}
			}
			else
			{
				do
				{
					if(action==swimming || IsSideSwim() || action == swimhit)
					{
						info = walkflag(temp_x,temp_y+(bigHitbox?0:8)-temp_step,2,up);
						
						if(_walkflag(temp_x+15, temp_y+(bigHitbox?0:8)-temp_step, 1,SWITCHBLOCK_STATE) &&
								!(iswaterex(MAPCOMBO(temp_x, temp_y+(bigHitbox?0:8)-temp_step), currmap, currscr, -1, temp_x, temp_y+(bigHitbox?0:8)-temp_step, true, false) &&
								  iswaterex(MAPCOMBO(temp_x+15, temp_y+(bigHitbox?0:8)-temp_step), currmap, currscr, -1, temp_x+15, temp_y+(bigHitbox?0:8)-temp_step, true, false)))
							info.setUnwalkable(true);
					}
					else
					{
						info = walkflag(temp_x,temp_y+(bigHitbox?0:8)-temp_step,2,up);
						if(x.getInt() & 7)
							info = info || walkflag(temp_x+16,temp_y+(bigHitbox?0:8)-temp_step,1,up);
						else if(blockmoving)
							info = info || walkflagMBlock(temp_x+8,temp_y+(bigHitbox?0:8)-temp_step);
					}
					
					if(info.isUnwalkable())
					{
						if(temp_y != int32_t(temp_y))
						{
							temp_y = floor((double)temp_y);
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
						}
						else
							break;
					}
				}
				while(info.isUnwalkable());
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					x = temp_x;
					y = temp_y;
					link_newstep = temp_step;
					move(up);
					return;
				}
				
				if(!DrunkLeft() && !DrunkRight())
				{
					if(NO_GRIDLOCK)
					{
						x = x.getInt();
						y = y.getInt();
						if(!_walkflag(x,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+8, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+(bigHitbox?0:8)-1))
								sprite::move((zfix)-1,(zfix)0);
						}
						else if(_walkflag(x,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+7, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+(bigHitbox?0:8)-1))
								sprite::move((zfix)1,(zfix)0);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=up;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
				else
				{
					goto LEFTRIGHT_NEWMOVE;
				}
			}
			
			return;
		}
		
		if(DrunkDown())
		{
			if(xoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=up && dir!=down)
				{
					if(xoff>2&&xoff<6)
					{
						move(dir);
					}
					else if(xoff>=6)
					{
						move(right);
					}
					else if(xoff>=1)
					{
						move(left);
					}
				}
				else
				{
					if(xoff>=4)
					{
						move(right);
					}
					else if(xoff<4)
					{
						move(left);
					}
				}
			}
			else
			{
				do
				{
					if(action==swimming || IsSideSwim() || action == swimhit)
					{
						info=walkflag(temp_x,temp_y+15+temp_step,2,down);
						
						if(_walkflag(temp_x+15, temp_y+15+temp_step, 1,SWITCHBLOCK_STATE) &&
								!(iswaterex(MAPCOMBO(temp_x, temp_y+15+temp_step), currmap, currscr, -1, temp_x, temp_y+15+temp_step, true, false) &&
								  iswaterex(MAPCOMBO(temp_x+15, temp_y+15+temp_step), currmap, currscr, -1, temp_x+15, temp_y+15+temp_step, true, false)))
							info.setUnwalkable(true);
					}
					else
					{
						info=walkflag(temp_x,temp_y+15+temp_step,2,down);
						if(x.getInt() & 7)
							info = info || walkflag(temp_x+16,temp_y+15+temp_step,1,down);
						else if(blockmoving)
							 info = info || walkflagMBlock(temp_x+8,temp_y+15+temp_step);
					}
					
					if(info.isUnwalkable())
					{
						if(temp_y != int32_t(temp_y))
						{
							temp_y = floor((double)temp_y);
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
						}
						else
							break;
					}
				}
				while(info.isUnwalkable());
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					x = temp_x;
					y = temp_y;
					link_newstep = temp_step;
					move(down);
					return;
				}
				
				if(!DrunkLeft() && !DrunkRight())
				{
					if(NO_GRIDLOCK)
					{
						x = x.getInt();
						y = y.getInt();
						if(!_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+8, y+15+1,1,SWITCHBLOCK_STATE)&&
								_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+15+1))
								sprite::move((zfix)-1,(zfix)0);
						}
						else if(_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+7, y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+15+1))
								sprite::move((zfix)1,(zfix)0);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=down;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
				else goto LEFTRIGHT_NEWMOVE;
			}
			
			return;
		}
		
LEFTRIGHT_NEWMOVE:
		temp_x = x;
		temp_y = y;
		temp_step = link_newstep;
		if(isdungeon() && (temp_y<=26 || temp_y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
		{
			return;
		}
		
		if(DrunkLeft())
		{
			if(yoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=left && dir!=right)
				{
					if(yoff>2&&yoff<6)
					{
						move(dir);
					}
					else if(yoff>=6)
					{
						move(down);
					}
					else if(yoff>=1)
					{
						move(up);
					}
				}
				else
				{
					if(yoff>=4)
					{
						move(down);
					}
					else if(yoff<4)
					{
						move(up);
					}
				}
			}
			else
			{
				do
				{
					info = walkflag(temp_x-temp_step,temp_y+(bigHitbox?0:8),1,left) ||
						   walkflag(temp_x-temp_step,temp_y+(isSideViewLink() ?0:8), 1,left);
					   
					if(y.getInt() & 7)
						info = info || walkflag(temp_x-temp_step,temp_y+16,1,left);
					
					if(info.isUnwalkable())
					{
						if(temp_x != int32_t(temp_x))
						{
							temp_x = floor((double)temp_x);
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
						}
						else
							break;
					}
				}
				while(info.isUnwalkable());
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					x = temp_x;
					y = temp_y;
					link_newstep = temp_step;
					move(left);
					return;
				}
				
				if(!DrunkUp() && !DrunkDown())
				{
					if(NO_GRIDLOCK)
					{
						x = x.getInt();
						y = y.getInt();
						int32_t v1=bigHitbox?0:8;
						int32_t v2=bigHitbox?8:12;
						
						if(!_walkflag(x-1,y+v1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+v2,1,SWITCHBLOCK_STATE)&&
								_walkflag(x-1,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+15))
								sprite::move((zfix)0,(zfix)-1);
						}
						else if(_walkflag(x-1,y+v1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+v2-1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+15,  1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+v1))
								sprite::move((zfix)0,(zfix)1);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=left;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
			}
			
			return;
		}
		
		if(DrunkRight())
		{
			if(yoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=left && dir!=right)
				{
					if(yoff>2&&yoff<6)
					{
						move(dir);
					}
					else if(yoff>=6)
					{
						move(down);
					}
					else if(yoff>=1)
					{
						move(up);
					}
				}
				else
				{
					if(yoff>=4)
					{
						move(down);
					}
					else if(yoff<4)
					{
						move(up);
					}
				}
			}
			else
			{
				do
				{
					info = walkflag(temp_x+15+temp_step,temp_y+(bigHitbox?0:8),1,right) || 
							walkflag(temp_x+15+temp_step,temp_y+(isSideViewLink() ?0:8),1,right);
					
					if(y.getInt() & 7)
						info = info || walkflag(temp_x+15+temp_step,y+16,1,right);
					
					if(info.isUnwalkable())
					{
						if(temp_x != int32_t(temp_x))
						{
							temp_x = floor((double)temp_x);
						}
						else if(temp_step > 1)
						{
							if(temp_step != int32_t(temp_step)) //floor
								temp_step = floor((double)temp_step);
							else --temp_step;
						}
						else
							break;
					}
				}
				while(info.isUnwalkable());
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					x = temp_x;
					y = temp_y;
					link_newstep = temp_step;
					move(right);
					return;
				}
				
				if(!DrunkUp() && !DrunkDown())
				{
					if(NO_GRIDLOCK)
					{
						x = x.getInt();
						y = y.getInt();
						int32_t v1=bigHitbox?0:8;
						int32_t v2=bigHitbox?8:12;
							   
						if(!_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
							   !_walkflag(x+16,y+v2,1,SWITCHBLOCK_STATE)&&
							   _walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+15))
								sprite::move((zfix)0,(zfix)-1);
						}
						else if(_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
								   !_walkflag(x+16,y+v2-1,1,SWITCHBLOCK_STATE)&&
								   !_walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+v1))
								sprite::move((zfix)0,(zfix)1);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=right;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
			}
		}
	}
	else
	{
		info = walkflag(x-int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,left) ||
			   walkflag(x-int32_t(lsteps[x.getInt()&7]),y+8,1,left);
		
		if(isdungeon() && DrunkLeft() && !info.isUnwalkable() && (x==32 && y==80))
		{
			//ONLY process the side-effects of the above walkflag if Link will actually move
			//sigh sigh sigh... walkflag is a horrible mess :-/ -DD
			execute(info);
			move(left);
			return;
		}
		
		info = walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,right) ||
			   walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+8,1,right);
		
		if(isdungeon() && DrunkRight() && !info.isUnwalkable() && x==208 && y==80)
		{
			execute(info);
			move(right);
			return;
		}
		
		ladderx = oldladderx;
		laddery = oldladdery;
		
		if(DrunkUp())
		{
			if(xoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=up && dir!=down)
				{
					if(xoff>2&&xoff<6)
					{
						move(dir);
					}
					else if(xoff>=6)
					{
						move(right);
					}
					else if(xoff>=1)
					{
						move(left);
					}
				}
				else
				{
					if(xoff>=4)
					{
						move(right);
					}
					else if(xoff<4)
					{
						move(left);
					}
				}
			}
			else
			{
				if(action==swimming || IsSideSwim() || action == swimhit)
				{
					info = walkflag(x,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]),2,up);
					
					if(_walkflag(x+15, y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]), 1,SWITCHBLOCK_STATE) &&
							!(iswaterex(MAPCOMBO(x, y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7])), currmap, currscr, -1, x, y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7])) &&
							  iswaterex(MAPCOMBO(x+15, y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7])), currmap, currscr, -1, x+15, y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]))))
						info.setUnwalkable(true);
				}
				else
				{
					info = walkflag(x,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]),2,up);
					if(x.getInt() & 7)
						info = info || walkflag(x+16,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]),1,up);
					else if(blockmoving)
						info = info || walkflagMBlock(x+8,y+(bigHitbox?0:8)-int32_t(lsteps[y.getInt()&7]));
				}
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					move(up);
					return;
				}
				
				if(!DrunkLeft() && !DrunkRight())
				{
					if(NO_GRIDLOCK)
					{
						if(!_walkflag(x,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+8, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+(bigHitbox?0:8)-1))
								sprite::move((zfix)-1,(zfix)0);
						}
						else if(_walkflag(x,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+7, y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
								!_walkflag(x+15,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+(bigHitbox?0:8)-1))
								sprite::move((zfix)1,(zfix)0);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=up;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
				else
				{
					goto LEFTRIGHT_OLDMOVE;
				}
			}
			
			return;
		}
		
		if(DrunkDown())
		{
			if(xoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=up && dir!=down)
				{
					if(xoff>2&&xoff<6)
					{
						move(dir);
					}
					else if(xoff>=6)
					{
						move(right);
					}
					else if(xoff>=1)
					{
						move(left);
					}
				}
				else
				{
					if(xoff>=4)
					{
						move(right);
					}
					else if(xoff<4)
					{
						move(left);
					}
				}
			}
			else
			{
				if(action==swimming || IsSideSwim() || action == swimhit)
				{
					info=walkflag(x,y+15+int32_t(lsteps[y.getInt()&7]),2,down);
					
					if(_walkflag(x+15, y+15+int32_t(lsteps[y.getInt()&7]), 1,SWITCHBLOCK_STATE) &&
							!(iswaterex(MAPCOMBO(x, y+15+int32_t(lsteps[y.getInt()&7])), currmap, currscr, -1, x, y+15+int32_t(lsteps[y.getInt()&7])) &&
							  iswaterex(MAPCOMBO(x+15, y+15+int32_t(lsteps[y.getInt()&7])), currmap, currscr, -1, x+15, y+15+int32_t(lsteps[y.getInt()&7]))))
						info.setUnwalkable(true);
				}
				else
				{
					info=walkflag(x,y+15+int32_t(lsteps[y.getInt()&7]),2,down);
					if(x.getInt() & 7)
						info = info || walkflag(x+16,y+15+int32_t(lsteps[y.getInt()&7]),1,down);
					else if(blockmoving)
						 info = info || walkflagMBlock(x+8,y+15+int32_t(lsteps[y.getInt()&7]));
				}
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					move(down);
					return;
				}
				
				if(!DrunkLeft() && !DrunkRight())
				{
					if(NO_GRIDLOCK)
					{
						if(!_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+8, y+15+1,1,SWITCHBLOCK_STATE)&&
								_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+15,y+15+1))
								sprite::move((zfix)-1,(zfix)0);
						}
						else if(_walkflag(x,   y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+7, y+15+1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x+15,y+15+1,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x,y+15+1))
								sprite::move((zfix)1,(zfix)0);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=down;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
				else goto LEFTRIGHT_OLDMOVE;
			}
			
			return;
		}
		
LEFTRIGHT_OLDMOVE:

		if(isdungeon() && (y<=26 || y>=134) && !get_bit(quest_rules,qr_FREEFORM) && !toogam)
		{
			return;
		}
		
		if(DrunkLeft())
		{
			if(yoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=left && dir!=right)
				{
					if(yoff>2&&yoff<6)
					{
						move(dir);
					}
					else if(yoff>=6)
					{
						move(down);
					}
					else if(yoff>=1)
					{
						move(up);
					}
				}
				else
				{
					if(yoff>=4)
					{
						move(down);
					}
					else if(yoff<4)
					{
						move(up);
					}
				}
			}
			else
			{
				info = walkflag(x-int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,left) ||
					   walkflag(x-int32_t(lsteps[x.getInt()&7]),y+(isSideViewLink() ?0:8), 1,left);
					   
				if(y.getInt() & 7)
					info = info || walkflag(x-int32_t(lsteps[x.getInt()&7]),y+16,1,left);
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					move(left);
					return;
				}
				
				if(!DrunkUp() && !DrunkDown())
				{
					if(NO_GRIDLOCK)
					{
						int32_t v1=bigHitbox?0:8;
						int32_t v2=bigHitbox?8:12;
						
						if(!_walkflag(x-1,y+v1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+v2,1,SWITCHBLOCK_STATE)&&
								_walkflag(x-1,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+15))
								sprite::move((zfix)0,(zfix)-1);
						}
						else if(_walkflag(x-1,y+v1,  1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+v2-1,1,SWITCHBLOCK_STATE)&&
								!_walkflag(x-1,y+15,  1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x-1,y+v1))
								sprite::move((zfix)0,(zfix)1);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=left;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
			}
			
			return;
		}
		
		if(DrunkRight())
		{
			if(yoff && !is_on_conveyor && action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking && jumping<1)
			{
				if(dir!=left && dir!=right)
				{
					if(yoff>2&&yoff<6)
					{
						move(dir);
					}
					else if(yoff>=6)
					{
						move(down);
					}
					else if(yoff>=1)
					{
						move(up);
					}
				}
				else
				{
					if(yoff>=4)
					{
						move(down);
					}
					else if(yoff<4)
					{
						move(up);
					}
				}
			}
			else
			{
				info = walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+(bigHitbox?0:8),1,right)
					|| walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+(isSideViewLink()?0:8),1,right);
				
				if(y.getInt() & 7)
					info = info || walkflag(x+15+int32_t(lsteps[x.getInt()&7]),y+16,1,right);
				
				execute(info);
				
				if(!info.isUnwalkable())
				{
					move(right);
					return;
				}
				
				if(!DrunkUp() && !DrunkDown())
				{
					if(NO_GRIDLOCK)
					{
						int32_t v1=bigHitbox?0:8;
						int32_t v2=bigHitbox?8:12;
							   
						if(!_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
							   !_walkflag(x+16,y+v2,1,SWITCHBLOCK_STATE)&&
							   _walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+15))
								sprite::move((zfix)0,(zfix)-1);
						}
						else if(_walkflag(x+16,y+v1,1,SWITCHBLOCK_STATE)&&
								   !_walkflag(x+16,y+v2-1,1,SWITCHBLOCK_STATE)&&
								   !_walkflag(x+16,y+15,1,SWITCHBLOCK_STATE))
						{
							if(hclk || (z>0 && !(tmpscr->flags2&fAIRCOMBOS)) || !checkdamagecombos(x+16,y+v1))
								sprite::move((zfix)0,(zfix)1);
						}
						else
						{
							pushing=push+1;
						}
					}
					else pushing=push+1;
					
					if(charging==0 && spins==0)
					{
						dir=right;
					}
					
					if(action!=swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
					{
						linkstep();
					}
					
					return;
				}
			}
		}
	}
}

//solid ffc checking should probably be moved to here.
void LinkClass::move(int32_t d2, int32_t forceRate)
{
    if( inlikelike || lstunclock > 0 )
        return;
	
	if(!get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) && !IsSideSwim())
	{
		moveOld(d2);
		return;
	}
	
    bool slowcombo = (combo_class_buf[combobuf[MAPCOMBO(x+7,y+8)].type].slow_movement && _effectflag(x+7,y+8,1, -1) && (z==0 || tmpscr->flags2&fAIRCOMBOS)) ||
                     (isSideViewLink() && (on_sideview_solid(x,y)||getOnSideviewLadder()) && combo_class_buf[combobuf[MAPCOMBO(x+7,y+8)].type].slow_movement && _effectflag(x+7,y+8,1, -1));
		     //!DIMITODO: add QR for slow combos under link
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,x+7,y+8)].type == cBRIDGE && !_walkflag_layer(x+7,y+8,1, &(tmpscr2[i]))) slowcombo = false;
		}
	}
    bool slowcharging = charging>0 && (itemsbuf[getWpnPressed(itype_sword)].flags & ITEM_FLAG10);
    bool is_swimming = (action == swimming);
	bool fastSwim = (zinit.link_swim_speed>60);
	zfix dx, dy;
	zfix movepix(steprate / 100.0);
	zfix step(movepix);
	zfix step_diag(movepix);
	zfix up_step(game->get_sideswim_up() / -100.0);
	zfix left_step(game->get_sideswim_side() / -100.0);
	zfix right_step(game->get_sideswim_side() / 100.0);
	zfix down_step(game->get_sideswim_down() / 100.0);
	bool checkladder  = false;
	
	if(link_newstep > movepix) link_newstep = movepix;
	if(link_newstep_diag > movepix) link_newstep_diag = movepix;
	//2/3 speed
	if((is_swimming && fastSwim) || (!is_swimming && (slowcharging ^ slowcombo)))
	{
		step = ((step / 3.0) * 2);
		step_diag = ((step_diag / 3.0) * 2);
		up_step = ((up_step / 3.0) * 2);
		left_step = ((left_step / 3.0) * 2);
		right_step = ((right_step / 3.0) * 2);
		down_step = ((down_step / 3.0) * 2);
	}
	//1/2 speed
	else if((is_swimming && !fastSwim) || (slowcharging && slowcombo))
	{
		step /= 2;
		step_diag /= 2;
		up_step /= 2;
		left_step /= 2;
		right_step /= 2;
		down_step /= 2;
	}
	//normal speed
	else
	{
		//no modification
	}
	
	if(diagonalMovement)
	{
		//zprint2("Link's X is %d, Y is %d\n", x, y);
		if(((d2 == up || d2 == down) && (shiftdir == left || shiftdir == right)) ||
			(d2 == left || d2 == right) && (shiftdir == up || shiftdir == down))
		{
			if(link_newstep > 0 && link_newstep_diag > 0)
			{
				step = STEP_DIAGONAL(step);
				step_diag = STEP_DIAGONAL(step_diag);
				up_step = STEP_DIAGONAL(up_step);
				left_step = STEP_DIAGONAL(left_step);
				right_step = STEP_DIAGONAL(right_step);
				down_step = STEP_DIAGONAL(down_step);
			}
		}
		if(link_newstep < step) step = link_newstep; //handle collision
		if(link_newstep_diag < step_diag) step_diag = link_newstep_diag; //handle collision
		switch(d2)
		{
			case up:
				switch(shiftdir)
				{
					case left:
						dx = -step_diag;
						if (IsSideSwim()) dx = left_step;
						break;
					case right:
						dx = step_diag;
						if (IsSideSwim()) dx = right_step;
						break;
				}
				if(walkable)
				{
					if (!IsSideSwim()) dy = -step;
					if (IsSideSwim()) 
					{
						dy = up_step;
						if (!iswaterex(MAPCOMBO(x,y+8-(bigHitbox*8)+floor(up_step)), currmap, currscr, -1, x, y+8-(bigHitbox*8)-2, true, false)) checkladder = true;
					}
				}
				break;
			case down:
				switch(shiftdir)
				{
					case left:
						dx = -step_diag;
						if (IsSideSwim()) dx = left_step;
						break;
					case right:
						dx = step_diag;
						if (IsSideSwim()) dx = right_step;
						break;
				}
				if(walkable)
				{
					dy = step;
					if (IsSideSwim()) dy = down_step;
				}
				break;
			case left:
				switch(shiftdir)
				{
					case up:
						if (!IsSideSwim()) dy = -step_diag;
						if (IsSideSwim()) 
						{
							dy = up_step;
							if (!iswaterex(MAPCOMBO(x,y+8-(bigHitbox*8)+floor(up_step)), currmap, currscr, -1, x, y+8-(bigHitbox*8)-2, true, false)) checkladder = true;
						}
						break;
					case down:
						dy = step_diag;
						if (IsSideSwim()) dy = down_step;
						break;
				}
				if(walkable)
				{
					dx = -step;
					if (IsSideSwim()) dx = left_step;
				}
				break;
			case right:
				switch(shiftdir)
				{
					case up:
						if (!IsSideSwim()) dy = -step_diag;
						if (IsSideSwim()) 
						{
							dy = up_step;
							if (!iswaterex(MAPCOMBO(x,y+8-(bigHitbox*8)+floor(up_step)), currmap, currscr, -1, x, y+8-(bigHitbox*8)-2, true, false)) checkladder = true;
						}
						break;
					case down:
						dy = step_diag;
						if (IsSideSwim()) dy = down_step;
						break;
				}
				if(walkable)
				{
					dx = step;
					if (IsSideSwim()) dx = right_step;
				}
				break;
		};
	}
	else
	{
		if(link_newstep < step) step = link_newstep; //handle collision
		switch(d2)
		{
			case up:
				dy -= step;
				if (IsSideSwim()) dy = up_step;
				break;
			case down:
				dy += step;
				if (IsSideSwim()) dy = down_step;
				break;
			case left:
				dx -= step;
				if (IsSideSwim()) dx = left_step;
				break;
			case right:
				dx += step;
				if (IsSideSwim()) dx = right_step;
				break;
		};
	}
	link_newstep = movepix;
	link_newstep_diag = movepix;

	if((charging==0 || attack==wHammer) && spins==0 && attackclk!=HAMMERCHARGEFRAME && action != sideswimattacking && !(IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (d2 == up || d2 == down))) //!DIRECTION SET
	{
		dir=d2;
	}
	else if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (d2 == up || d2 == down) && (shiftdir == left || shiftdir == right) && (charging==0 && spins==0))
	{
		dir = shiftdir; 
	}
	if(forceRate > -1)
	{
		checkladder = false;
		switch(dir)
		{
			case right:
			case r_up:
			case r_down:
				dx = zfix(forceRate) / 100;
				break;
			case left:
			case l_up:
			case l_down:
				dx = zfix(-forceRate) / 100;
				break;
			default:
				dx = 0;
		}
		switch(dir)
		{
			case down:
			case r_down:
			case l_down:
				dy = zfix(forceRate) / 100;
				break;
			case up:
			case r_up:
			case l_up:
				dy = zfix(-forceRate) / 100;
				break;
			default:
				dy = 0;
		}
	}
	if(action != swimming && action != sideswimming && action != sideswimhit && action != sideswimattacking)
	{
		linkstep();
		
		//ack... don't walk if in midair! -DD
		if(charging==0 && spins==0 && z==0 && !(isSideViewLink() && !on_sideview_solid(x,y) && !getOnSideviewLadder()))
		{
			action=walking; FFCore.setLinkAction(walking);
		}
			
		if(++link_count > (16*link_animation_speed))
			link_count=0;
	}
	else if(!(frame & 1))
	{
		linkstep();
	}
	
	if(charging==0 || attack!=wHammer)
	{
		sprite::move(dx, dy);
		WalkflagInfo info;
		info = walkflag(x,y+8-(bigHitbox*8)-4,2,up);
		execute(info);
		if (checkladder && !canSideviewLadderRemote(x, y-4) && !info.isUnwalkable())
		{
			if (game->get_sideswim_jump() != 0)
			{
				setFall(zfix(0-(FEATHERJUMP*(game->get_sideswim_jump()/10000.0))));
				sfx(WAV_ZN1SPLASH,(int32_t)x);
				hopclk = 0;
				if (charging || spins) action = attacking;
				else action = none;
			}
			else
			{
				sprite::move(zfix(0), zfix(-1*dy));
			}
		}
	}
}

void LinkClass::moveOld(int32_t d2)
{
	//al_trace("%s\n",d2==up?"up":d2==down?"down":d2==left?"left":d2==right?"right":"?");
    static bool totalskip = false;
    
    if( inlikelike || lstunclock > 0 )
        return;
	
    int32_t dx=0,dy=0;
    int32_t xstep=lsteps[x.getInt()&7];
    int32_t ystep=lsteps[y.getInt()&7];
    int32_t z3skip=0;
    int32_t z3diagskip=0;
    bool slowcombo = (combo_class_buf[combobuf[MAPCOMBO(x+7,y+8)].type].slow_movement && (z==0 || tmpscr->flags2&fAIRCOMBOS)) ||
                     (isSideViewLink() && (on_sideview_solid(x,y)||getOnSideviewLadder()) && combo_class_buf[combobuf[MAPCOMBO(x+7,y+8)].type].slow_movement);
    bool slowcharging = charging>0 && (itemsbuf[getWpnPressed(itype_sword)].flags & ITEM_FLAG10);
    bool is_swimming = (action == swimming);
    
    //slow walk combo, or charging, moves at 2/3 speed
    if(
        (!is_swimming && (slowcharging ^ slowcombo))||
        (is_swimming && (zinit.link_swim_speed>60))
    )
    {
        totalskip = false;
        
        if(diagonalMovement)
        {
            skipstep=(skipstep+1)%6;
            
            if(skipstep%2==0) z3skip=1;
            else z3skip=0;
            
            if(skipstep%3==0) z3diagskip=1;
            else z3diagskip=0;
        }
        else
        {
            if(d2<left)
            {
                if(ystep>1)
                {
                    skipstep^=1;
                    ystep=skipstep;
                }
            }
            else
            {
                if(xstep>1)
                {
                    skipstep^=1;
                    xstep=skipstep;
                }
            }
        }
    }
//  else if(is_swimming || (slowcharging && slowcombo))
    else if(
        (is_swimming && (zinit.link_swim_speed<60))||
        (slowcharging && slowcombo)
    )
    {
        //swimming, or charging on a slow combo, moves at 1/2 speed
        totalskip = !totalskip;
        
        if(diagonalMovement)
        {
            skipstep=0;
        }
    }
    else
    {
        totalskip = false;
        
        if(diagonalMovement)
        {
            skipstep=0;
        }
    }
    
    if(!totalskip)
    {
        if(diagonalMovement)
        {
            switch(d2)
            {
            case up:
                if(shiftdir==left)
                {
                    if(walkable)
                    {
                        dy-=1-z3diagskip;
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                }
                else if(shiftdir==right)
                {
                    if(walkable)
                    {
                        dy-=1-z3diagskip;
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                }
                else
                {
                    if(walkable)
                    {
                        dy-=z3step-z3skip;
                        z3step=(z3step%2)+1;
                    }
                }
                
                break;
                
            case down:
                if(shiftdir==left)
                {
                    if(walkable)
                    {
                        dy+=1-z3diagskip;
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                }
                else if(shiftdir==right)
                {
                    if(walkable)
                    {
                        dy+=1-z3diagskip;
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                }
                else
                {
                    if(walkable)
                    {
                        dy+=z3step-z3skip;
                        z3step=(z3step%2)+1;
                    }
                }
                
                break;
                
            case right:
                if(shiftdir==up)
                {
                    if(walkable)
                    {
                        dy-=1-z3diagskip;
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dy-=1-z3diagskip;
                        z3step=2;
                    }
                }
                else if(shiftdir==down)
                {
                    if(walkable)
                    {
                        dy+=1-z3diagskip;
                        dx+=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dy+=1-z3diagskip;
                        z3step=2;
                    }
                }
                else
                {
                    if(walkable)
                    {
                        dx+=z3step-z3skip;
                        z3step=(z3step%2)+1;
                    }
                }
                
                break;
                
            case left:
                if(shiftdir==up)
                {
                    if(walkable)
                    {
                        dy-=1-z3diagskip;
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dy-=1-z3diagskip;
                        z3step=2;
                    }
                }
                else if(shiftdir==down)
                {
                    if(walkable)
                    {
                        dy+=1-z3diagskip;
                        dx-=1-z3diagskip;
                        z3step=2;
                    }
                    else
                    {
                        dy+=1-z3diagskip;
                        z3step=2;
                    }
                }
                else
                {
                    if(walkable)
                    {
                        dx-=z3step-z3skip;
                        z3step=(z3step%2)+1;
                    }
                }
                
                break;
            }
        }
        else
        {
            switch(d2)
            {
            case up:
                if(!isSideViewLink() || (ladderx && laddery && ladderdir==up) || getOnSideviewLadder() || action == sideswimming || action == sideswimhit || action == sideswimattacking) dy-=ystep;
                
                break;
                
            case down:
                if(!isSideViewLink() || (ladderx && laddery && ladderdir==up) || getOnSideviewLadder() || action == sideswimming || action == sideswimhit || action == sideswimattacking) dy+=ystep;
                
                break;
                
            case left:
                dx-=xstep;
                break;
                
            case right:
                dx+=xstep;
                break;
            }
        }
    }
    
    if((charging==0 || attack==wHammer) && spins==0 && attackclk!=HAMMERCHARGEFRAME && action != sideswimattacking && !(IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (d2 == up || d2 == down))) //!DIRECTION SET
    {
        dir=d2;
    }
    else if (IsSideSwim() && get_bit(quest_rules,qr_SIDESWIMDIR) && (d2 == up || d2 == down) && (shiftdir == left || shiftdir == right) && (charging==0 && spins==0))
    {
	dir = shiftdir; 
    }
    
    if(action != swimming && !IsSideSwim())
    {
        linkstep();
        
        //ack... don't walk if in midair! -DD
        if(charging==0 && spins==0 && z==0 && !(isSideViewLink() && !on_sideview_solid(x,y) && !getOnSideviewLadder()))
	{
            action=walking; FFCore.setLinkAction(walking);
	}
            
        if(++link_count > (16*link_animation_speed))
            link_count=0;
    }
    else if(!(frame & 1))
    {
        linkstep();
    }
    
    if(charging==0 || attack!=wHammer)
    {
        sprite::move((zfix)dx,(zfix)dy);
    }
}

LinkClass::WalkflagInfo LinkClass::walkflag(zfix fx,zfix fy,int32_t cnt,byte d2)
{
	return walkflag(fx.getInt(), fy.getInt(), cnt, d2);
}
LinkClass::WalkflagInfo LinkClass::walkflag(int32_t wx,int32_t wy,int32_t cnt,byte d2)
{
    WalkflagInfo ret;
    
    if(toogam)
    {
        ret.setUnwalkable(false);
        return ret;
    }
    
    if(blockpath && wy<(bigHitbox?80:88))
    {
        ret.setUnwalkable(true);
        return ret;
    }
    
    if(blockmoving && mblock2.hit(wx,wy,0,1,1,1))
    {
        ret.setUnwalkable(true);
        return ret;
    }
    
    if(isdungeon() && currscr<128 && wy<(bigHitbox?32:40) && (((diagonalMovement||NO_GRIDLOCK)?(x<=112||x>=128):x!=120) || _walkflag(120,24,2,SWITCHBLOCK_STATE))
            && !get_bit(quest_rules,qr_FREEFORM))
    {
        ret.setUnwalkable(true);
        return ret;
    }
    
    bool wf = _walkflag(wx,wy,cnt,SWITCHBLOCK_STATE);
    
    if(isdungeon() && currscr<128 && !get_bit(quest_rules,qr_FREEFORM))
    {
        if((diagonalMovement||NO_GRIDLOCK))
        {
            if(wx>=112&&wx<120&&wy<40&&wy>=32) wf=true;
            
            if(wx>=136&&wx<144&&wy<40&&wy>=32) wf=true;
        }
    }
    //All problems related to exiting water are probably here. -Z
    if(action==swimming || IsSideSwim())
    {
        if(!wf)
        {
	    bool isthissolid = false;
		if (_walkflag(x+7,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE)
                || _walkflag(x+7,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE)
		|| _walkflag(x+8,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE)
                || _walkflag(x+8,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE)) isthissolid = true;
		//This checks if Link is currently swimming in solid water (cause even if the QR "No Hopping" is enabled, he should still hop out of solid water) - Dimi
		
		
            if(landswim>= (get_bit(quest_rules,qr_DROWN) && isSwimming() ? 1
                           : (!diagonalMovement) ? 1 : (get_bit(quest_rules,qr_NO_HOPPING)?1:22)))
            {
                //Check for out of bounds for swimming
                bool changehop = true;
                
                if((diagonalMovement||NO_GRIDLOCK))
                {
                    if(wx<0||wy<0)
                        changehop = false;
                    else if(wx>248)
                        changehop = false;
                    else if(wx>240&&cnt==2)
                        changehop = false;
                    else if(wy>168)
                        changehop = false;
                }
		if ((get_bit(quest_rules, qr_NO_HOPPING) || CanSideSwim()) && !isthissolid) changehop = false;
                //This may be where the hang-up for exiting water exists. -Z
                // hop out of the water
                if(changehop)
                    ret.setHopClk(1);
            }
            else
            {
                if((!(get_bit(quest_rules, qr_NO_HOPPING) || CanSideSwim()) || isthissolid) && (dir==d2 || shiftdir==d2))
                {
                    //int32_t vx=((int32_t)x+4)&0xFFF8;
                    //int32_t vy=((int32_t)y+4)&0xFFF8;
                    if(d2==left)
                    {
                        if(!iswaterex(MAPCOMBO(x-1,y+(bigHitbox?6:11)), currmap, currscr, -1, x-1,y+(bigHitbox?6:11)) &&
                           !iswaterex(MAPCOMBO(x-1,y+(bigHitbox?9:12)), currmap, currscr, -1, x-1,y+(bigHitbox?9:12)) &&
                           !_walkflag(x-1,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE) &&
                           !_walkflag(x-1,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE))
                        {
                            ret.setHopDir(d2);
                            ret.setIlswim(true);
                        }
                        else ret.setIlswim(false);
                    }
                    else if(d2==right)
                    {
                        if(!iswaterex(MAPCOMBO(x+16,y+(bigHitbox?6:11)), currmap, currscr, -1, x+16,y+(bigHitbox?6:11)) &&
                           !iswaterex(MAPCOMBO(x+16,y+(bigHitbox?9:12)), currmap, currscr, -1, x+16,y+(bigHitbox?9:12)) &&
                           !_walkflag(x+16,y+(bigHitbox?6:11),1,SWITCHBLOCK_STATE) &&
                           !_walkflag(x+16,y+(bigHitbox?9:12),1,SWITCHBLOCK_STATE))
                        {
                            ret.setHopDir(d2);
                            ret.setIlswim(true);
                        }
                        else ret.setIlswim(false);
                    }
                    else if(d2==up)
                    {
                        if(!iswaterex(MAPCOMBO(x+7,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+7,y+(bigHitbox?0:8)-1) &&
                           !iswaterex(MAPCOMBO(x+8,y+(bigHitbox?0:8)-1), currmap, currscr, -1, x+8,y+(bigHitbox?0:8)-1) &&
                           !_walkflag(x+7,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE) &&
                           !_walkflag(x+8,y+(bigHitbox?0:8)-1,1,SWITCHBLOCK_STATE))
                        {
                            ret.setHopDir(d2);
                            ret.setIlswim(true);
                        }
                        else ret.setIlswim(false);
                    }
                    else if(d2==down)
                    {
                        if(!iswaterex(MAPCOMBO(x+7,y+16), currmap, currscr, -1, x+7,y+16) &&
                           !iswaterex(MAPCOMBO(x+8,y+16), currmap, currscr, -1, x+8,y+16) &&
                           !_walkflag(x+7,y+16,1,SWITCHBLOCK_STATE) &&
                           !_walkflag(x+8,y+16,1,SWITCHBLOCK_STATE))
                        {
                            ret.setHopDir(d2);
                            ret.setIlswim(true);
                        }
                        else ret.setIlswim(false);
                    }
                }
                
                if(wx<0||wy<0);
                else if(wx>248);
                else if(wx>240&&cnt==2);
                else if(wy>168);
                else if(get_bit(quest_rules, qr_DROWN) && !ilswim);
		//if(iswaterex(MAPCOMBO(wx,wy)) && iswaterex(MAPCOMBO(wx,wy)))
                else if(iswaterex(MAPCOMBO(wx,wy), currmap, currscr, -1, wx,wy)) //!DIMI: weird duplicate function here before. Was water bugged this whole time, or was it just an unneccessary duplicate?
                {
                    ret.setUnwalkable(false);
                    return ret;
                }
                else
                {
                    ret.setUnwalkable(true);
                    return ret;
                }
            }
        }
        else
        {
            int32_t wtrx  = iswaterex(MAPCOMBO(wx,wy), currmap, currscr, -1, wx,wy);
            int32_t wtrx8 = iswaterex(MAPCOMBO(x+8,wy), currmap, currscr, -1, x+8,wy); //!DIMI: Is x + 8 intentional???
            
            if((d2>=left && wtrx) || (d2<=down && wtrx && wtrx8))
            {
                ret.setUnwalkable(false);
                return ret;
            }
        }
    }
    else if(ladderx+laddery)                                  // ladder is being used
    {
        int32_t lx = !(get_bit(quest_rules, qr_DROWN)&&iswaterex(MAPCOMBO(x+4,y+11), currmap, currscr, -1, x+4,y+11)&&!_walkflag(x+4,y+11,1,SWITCHBLOCK_STATE)) ? wx : x;
        int32_t ly = !(get_bit(quest_rules, qr_DROWN)&&iswaterex(MAPCOMBO(x+4,y+11), currmap, currscr, -1, x+4,y+11)&&!_walkflag(x+4,y+11,1,SWITCHBLOCK_STATE)) ? wy : y;
        
        if((diagonalMovement||NO_GRIDLOCK))
        {
            if(ladderdir==up)
            {
                if(abs(ly-(laddery+8))<=8) // ly is between laddery (laddery+8-8) and laddery+16 (laddery+8+8)
                {
                    bool temp = false;
                    
                    if(!(abs(lx-(ladderx+8))<=8))
                        temp = true;
                        
                    if(cnt==2)
                        if(!(abs((lx+8)-(ladderx+8))<=8))
                            temp=true;
                            
                    if(!temp)
                    {
                        ret.setUnwalkable(false);
                        return ret;
                    }
                    
                    if(current_item_power(itype_ladder)<2 && (d2==left || d2==right) && !isSideViewLink())
                    {
                        ret.setUnwalkable(true);
                        return ret;
                    }
                }
            }
            else
            {
                if(abs(lx-(ladderx+8))<=8)
                {
                    if(abs(ly-(laddery+(bigHitbox?8:12)))<=(bigHitbox?8:4))
                    {
                        ret.setUnwalkable(false);
                        return ret;
                    }
                    
                    if(current_item_power(itype_ladder)<2 && (d2==up || d2==down))
                    {
                        ret.setUnwalkable(true);
                        return ret;
                    }
                    
                    if((abs(ly-laddery+8)<=8) && d2<=down)
                    {
                        ret.setUnwalkable(false);
                        return ret;
                    }
                }
            }
        } // diagonalMovement
        else
        {
            if((d2&2)==ladderdir)                                    // same direction
            {
                switch(d2)
                {
                case up:
                    if(y.getInt()<=laddery)
                    {
                        ret.setUnwalkable(_walkflag(ladderx,laddery-8,1,SWITCHBLOCK_STATE) ||
                                          _walkflag(ladderx+8,laddery-8,1,SWITCHBLOCK_STATE));
                        return ret;
                        
                    }
                    
                    // no break
                case down:
                    if((wy&0xF0)==laddery)
                    {
                        ret.setUnwalkable(false);
                        return ret;
                    }
                    
                    break;
                    
                default:
                    if((wx&0xF0)==ladderx)
                    {
                        ret.setUnwalkable(false);
                        return ret;
                    }
                }
                
                if(d2<=down)
                {
                    ret.setUnwalkable(_walkflag(ladderx,wy,1,SWITCHBLOCK_STATE) || _walkflag(ladderx+8,wy,1,SWITCHBLOCK_STATE));
                    return ret;
                }
                
                ret.setUnwalkable(_walkflag((wx&0xF0),wy,1,SWITCHBLOCK_STATE) || _walkflag((wx&0xF0)+8,wy,1,SWITCHBLOCK_STATE));
                return ret;
            }
            
            // different dir
            if(current_item_power(itype_ladder)<2 && !(isSideViewLink() && (d2==left || d2==right)))
            {
                ret.setUnwalkable(true);
                return ret;
            }
            
            if(wy>=laddery && wy<=laddery+16 && d2<=down)
            {
                ret.setUnwalkable(false);
                return ret;
            }
        }
    }
    else if(wf || isSideViewLink() || get_bit(quest_rules, qr_DROWN))
    {
        // see if it's a good spot for the ladder or for swimming
        bool unwalkablex  = _walkflag(wx,wy,1,SWITCHBLOCK_STATE); //will be used later for the ladder -DD
        bool unwalkablex8 = _walkflag(x+8,wy,1,SWITCHBLOCK_STATE);
        
        if(get_bit(quest_rules, qr_DROWN))
        {
            // Drowning changes the following attributes:
            // * Dangerous water is also walkable, so ignore the previous
            // definitions of unwalkablex and unwalkablex8.
            // * Instead, prevent the ladder from being used in the
            // one frame where Link has landed on water before drowning.
            unwalkablex = unwalkablex8 = !iswaterex(MAPCOMBO(x+4,y+11), currmap, currscr, -1, x+4,y+11);
        }
        
        // check if he can swim
        if(current_item(itype_flippers) && z==0)
        {
		int32_t wtrx  = iswaterex(MAPCOMBO(wx,wy), currmap, currscr, -1, wx,wy);
		int32_t wtrx8 = iswaterex(MAPCOMBO(x+8,wy), currmap, currscr, -1, x+8,wy); //!DIMI: Still not sure if this should be x + 8...
		if (current_item(itype_flippers) >= combobuf[wtrx8].attribytes[0] && (!(combobuf[wtrx8].usrflags&cflag1) || (itemsbuf[current_item_id(itype_flippers)].flags & ITEM_FLAG3))) //Don't swim if the water's required level is too high! -Dimi
		{
		//ladder ignores water combos that are now walkable thanks to flippers -DD
		    unwalkablex = unwalkablex && (!wtrx);
		    unwalkablex8 = unwalkablex8 && (!wtrx8);
		    
		    if(landswim >= 22)
		    {
			ret.setHopClk(2);
			ret.setUnwalkable(false);
			return ret;
		    }
		    else if((d2>=left && wtrx) || (d2<=down && wtrx && wtrx8))
		    {
			if(!(diagonalMovement||NO_GRIDLOCK))
			{
			    ret.setHopClk(2);
			    
			    if(charging || spins>5)
			    {
				//if Link is charging, he might be facing the wrong direction (we want him to
				//hop into the water, not in the facing direction)
				ret.setDir(d2);
				//moreover Link can't charge in the water -DD
				ret.setChargeAttack();
			    }
			    
			    ret.setUnwalkable(false);
			    return ret;
			}
			else if(dir==d2)
			{
			    ret.setIlswim(true);
			    ladderx = 0;
			    laddery = 0;
			}
		    }
		}
        }
        
        // check if he can use the ladder
        // "Allow Ladder Anywhere" is toggled by fLADDER
        if(can_deploy_ladder())
            // laddersetup
        {
            // Check if there's water to use the ladder over
            bool wtrx = (iswaterex(MAPCOMBO(wx,wy), currmap, currscr, -1, wx,wy) != 0);
            bool wtrx8 = (iswaterex(MAPCOMBO(x+8,wy), currmap, currscr, -1, x+8,wy) != 0);
			int32_t ldrid = current_item_id(itype_ladder);
			bool ladderpits = ldrid > -1 && (itemsbuf[ldrid].flags&ITEM_FLAG1);
            
            if(wtrx || wtrx8)
            {
                if(isSideViewLink())
                {
                    wtrx  = !_walkflag(wx, wy+8, 1,SWITCHBLOCK_STATE) && !_walkflag(wx, wy, 1,SWITCHBLOCK_STATE) && dir!=down;
                    wtrx8 = !_walkflag(wx+8, wy+8, 1,SWITCHBLOCK_STATE) && !_walkflag(wx+8, wy, 1,SWITCHBLOCK_STATE) && dir!=down;
                }
                // * walk on half-water using the ladder instead of using flippers.
                // * otherwise, walk on ladder(+hookshot) combos.
                else if(wtrx==wtrx8 && (isstepable(MAPCOMBO(wx, wy)) || isstepable(MAPCOMBO(wx+8,wy)) || wtrx==true))
                {
		    if(!get_bit(quest_rules, qr_OLD_210_WATER))
		    {
			//if Link could swim on a tile instead of using the ladder,
			//refuse to use the ladder to step over that tile. -DD
			wtrx  = isstepable(MAPCOMBO(wx, wy)) && unwalkablex;
			wtrx8 = isstepable(MAPCOMBO(wx+8,wy)) && unwalkablex8;
		    }
                }
            }
            else
            {
				// No water; check other things
				
				//Check pits
				if(ladderpits)
				{
					int32_t pit_cmb = getpitfall(wx,wy);
					wtrx = pit_cmb && (combobuf[pit_cmb].usrflags&cflag4);
					pit_cmb = getpitfall(x+8,wy);
					wtrx8 = pit_cmb && (combobuf[pit_cmb].usrflags&cflag4);
				}
				if(!ladderpits || (!(wtrx || wtrx8) || isSideViewLink())) //If no pit, check ladder combos
				{
					int32_t combo=combobuf[MAPCOMBO(wx, wy)].type;
					wtrx=(combo==cLADDERONLY || combo==cLADDERHOOKSHOT);
					combo=combobuf[MAPCOMBO(wx+8, wy)].type;
					wtrx8=(combo==cLADDERONLY || combo==cLADDERHOOKSHOT);
				}
			}
            
	     for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,wx,wy)].type == cBRIDGE && !_walkflag_layer(wx,wy,1, &(tmpscr2[i]))) wtrx = false;
				if (combobuf[MAPCOMBO2(i,wx+8,wy)].type == cBRIDGE && !_walkflag_layer(wx+8,wy,1, &(tmpscr2[i]))) wtrx8 = false;
			}
		}
            bool walkwater = (get_bit(quest_rules, qr_DROWN) && !iswaterex(MAPCOMBO(wx,wy), currmap, currscr, -1, wx,wy));
            
            if((diagonalMovement||NO_GRIDLOCK))
            {
                if(d2==dir)
                {
                    int32_t c = walkwater ? 0:8;
                    int32_t b = walkwater ? 8:0;
                    
                    if(d2>=left)
                    {
                        // If the difference between wy and y is small enough
                        if(abs((wy)-(int32_t(y+c)))<=(b) && wtrx)
                        {
                            // Don't activate the ladder if it would be entirely
                            // over water and Link has the flippers. This isn't
                            // a good way to do this, but it's too risky
                            // to make big changes to this stuff.
                            bool deployLadder=true;
                            int32_t lx=wx&0xF0;
                            if(current_item(itype_flippers) && current_item(itype_flippers) >= combobuf[iswaterex(MAPCOMBO(lx+8, y+8), currmap, currscr, -1, lx+8, y+8)].attribytes[0] && z==0)
                            {
                                if(iswaterex(MAPCOMBO(lx, y), currmap, currscr, -1, lx, y) && 
				iswaterex(MAPCOMBO(lx+15, y), currmap, currscr, -1, lx+15, y) &&
                                iswaterex(MAPCOMBO(lx, y+15), currmap, currscr, -1, lx, y+15) && 
				iswaterex(MAPCOMBO(lx+15, y+15), currmap, currscr, -1, lx+15, y+15))
                                    deployLadder=false;
                            }
                            if(deployLadder)
                            {
                                ladderx = wx&0xF0;
                                laddery = y;
                                ladderdir = left;
                                ladderstart = d2;
                                ret.setUnwalkable(laddery!=y.getInt());
                                return ret;
                            }
                        }
                    }
                    else if(d2<=down)
                    {
                        // If the difference between wx and x is small enough
                        if(abs((wx)-(int32_t(x+c)))<=(b) && wtrx)
                        {
                            ladderx = x;
                            laddery = wy&0xF0;
                            ladderdir = up;
                            ladderstart = d2;
                            ret.setUnwalkable(ladderx!=x.getInt());
                            return ret;
                        }
                        
                        if(cnt==2)
                        {
                            if(abs((wx+8)-(int32_t(x+c)))<=(b) && wtrx8)
                            {
                                ladderx = x;
                                laddery = wy&0xF0;
                                ladderdir = up;
                                ladderstart = d2;
                                ret.setUnwalkable(ladderx!=x.getInt());
                                return ret;
                            }
                        }
                    }
                }
            }
            else
            {
                bool flgx  = _walkflag(wx,wy,1,SWITCHBLOCK_STATE) && !wtrx; // Solid, and not steppable
                bool flgx8 = _walkflag(x+8,wy,1,SWITCHBLOCK_STATE) && !wtrx8; // Solid, and not steppable
                
                if((d2>=left && wtrx)
                        // Deploy the ladder vertically even if Link is only half on water.
                        || (d2<=down && ((wtrx && !flgx8) || (wtrx8 && !flgx))))
                {
                    if(((y.getInt()+15) < wy) || ((y.getInt()+8) > wy))
                        ladderdir = up;
                    else
                        ladderdir = left;
                        
                    if(ladderdir==up)
                    {
                        ladderx = x.getInt()&0xF8;
                        laddery = wy&0xF0;
                    }
                    else
                    {
                        ladderx = wx&0xF0;
                        laddery = y.getInt()&0xF8;
                    }
                    
                    ret.setUnwalkable(false);
                    return ret;
                }
            }
        }
    }
    
    ret.setUnwalkable(wf);
    return ret;
}

// Only checks for moving blocks. Apparently this is a thing we need.
LinkClass::WalkflagInfo LinkClass::walkflagMBlock(int32_t wx,int32_t wy)
{
    WalkflagInfo ret;
    ret.setUnwalkable(blockmoving && mblock2.hit(wx,wy,0,1,1,1));
    return ret;
}

bool LinkClass::checksoliddamage()
{
	if(toogam) return false;
    
	if(z!=0) return false;
	int32_t bx = x.getInt();
	int32_t by = y.getInt();
	int32_t initk = 0;
	switch(dir)
	{
	case up:
		
		by-=bigHitbox ? 4 : -4;
		
		if(by<0)
		{
			return false;
		}
		break;
		
	case down:
		
		by+=20;
		if(by>175)
		{
			return false;
		}
		
		break;
		
	case left:
		bx-=4;
		if (!bigHitbox) 
		{
			by+=8;
			initk = 1;
		}
		if(bx<0)
		{
			return false;
		}
		
		break;
		
	case right:
		
		bx+=20;
		if (!bigHitbox) 
		{
			by+=8;
			initk = 1;
		}
		if(bx>255)
		{
			return false;
		}
		
		break;
	}
	int32_t t = combobuf[MAPCOMBO(bx,by)].type;
	int32_t initbx = bx;
	int32_t initby = by;
	
	// Unlike push blocks, damage combos should be tested on layers 2 and under
	for(int32_t i=(get_bit(quest_rules,qr_DMGCOMBOLAYERFIX) ? 2 : 0); i>=0; i--)
	{
		bx = initbx;
		by = initby;
		for (int32_t k = initk; k <= 2; k++)
		{
			t = COMBOTYPE2(i-1,bx,by);
			
			// Solid damage combos use pushing>0, hence the code is here.
			if (!get_bit(quest_rules, qr_LESS_AWFUL_SIDESPIKES) || !isSideViewLink() || (dir != down && (dir != up || getOnSideviewLadder())))
			{
				if(combo_class_buf[t].modify_hp_amount && _walkflag(bx,by,1,SWITCHBLOCK_STATE) && pushing>0 && hclk<1 && action!=casting && action != sideswimcasting && !get_bit(quest_rules, qr_NOSOLIDDAMAGECOMBOS))
				{
					// Bite Link
					if (checkdamagecombos(bx, bx, by, by, i-1, true)) return true;
				}
			}
			if(isSideViewLink() && // Check for sideview damage combos
					hclk<1 && action!=casting && action!=sideswimcasting) // ... but only if Link could be hurt
			{
				if (get_bit(quest_rules, qr_LESS_AWFUL_SIDESPIKES))
				{
					if (on_sideview_solid(x,y) && (!getOnSideviewLadder() || DrunkDown()))
					{
						if(checkdamagecombos(x+4, x+4, y+16, y+18, i-1, false, false) && checkdamagecombos(x+12, x+12, y+16, y+18, i-1, false, false))
						{
							if (checkdamagecombos(x+4, x+12, y+16, y+18, i-1, false, true)) return true;
						}
					}
					if (checkdamagecombos(x+4, x+12, y+8, y+15, i-1, false, true)) return true;
				}
				else
				{
					//old 2.50.2-ish code for 2.50.0 sideview quests for er_OLDSIDEVIEWSPIKES
					if ( get_bit(quest_rules, qr_OLDSIDEVIEWSPIKES ) )
					{
						if (checkdamagecombos(x+8-(zfix)(tmpscr->csensitive),
							x+8+(zc_max(tmpscr->csensitive-1,0)),
							y+17-(get_bit(quest_rules,qr_LTTPCOLLISION)?tmpscr->csensitive:(tmpscr->csensitive+1)/2),
							y+17+zc_max((get_bit(quest_rules,qr_LTTPCOLLISION)?tmpscr->csensitive:(tmpscr->csensitive+1)/2)-1,0), i-1, true))
								return true;
					}
					else //2.50.1 and later
					{
						if(checkdamagecombos(x+4, x+12, y+16, y+24))
							return true;
					}
				}
				
			}
			if (dir < left) bx += (k % 2) ? 7 : 8;
			else by += (k % 2) ? 7 : 8;
		}
	}
	return false;
}
void LinkClass::checkpushblock()
{
	if(toogam) return;
	
	if(z!=0) return;
	
	// Return early in some cases..
	bool earlyReturn=false;
	
	if(!(diagonalMovement||NO_GRIDLOCK) || dir==left)
		if(x.getInt()&15) earlyReturn=true;
		
	// if(y<16) return;
	if(isSideViewLink() && !on_sideview_solid(x,y)) return;
	
	int32_t bx = x.getInt()&0xF0;
	int32_t by = (y.getInt()&0xF0);
	
	switch(dir)
	{
	case up:
		if(y<16)
		{
			earlyReturn=true;
			break;
		}
		
		if(!((int32_t)y&15)&&y!=0) by-=bigHitbox ? 16 : 0;
		
		if((int32_t)x&8) bx+=16;
		
		break;
		
	case down:
		if(y>128)
		{
			earlyReturn=true;
			break;
		}
		else
		{
			by+=16;
			
			if((int32_t)x&8) bx+=16;
		}
		
		break;
		
	case left:
		if(x<32)
		{
			earlyReturn=true;
			break;
		}
		else
		{
			bx-=16;
			
			if(y.getInt()&8)
			{
				by+=16;
			}
		}
		
		break;
		
	case right:
		if(x>208)
		{
			earlyReturn=true;
			break;
		}
		else
		{
			bx+=16;
			
			if(y.getInt()&8)
			{
				by+=16;
			}
		}
		
		break;
	}
	
	if (checksoliddamage()) return;
	
	if(earlyReturn)
		return;
	
	int32_t itemid=current_item_id(itype_bracelet);
	size_t combopos = (by&0xF0)+(bx>>4);
	
	for(int lyr = 2; lyr > -1; --lyr) //Top-down, in case of stacked push blocks
	{
		if(get_bit(quest_rules,qr_HESITANTPUSHBLOCKS)&&(pushing<4)) break;
		if(lyr && !get_bit(quest_rules, qr_PUSHBLOCK_LAYER_1_2))
			continue;
		mapscr* m = FFCore.tempScreens[lyr];
		int32_t f = MAPFLAG2(lyr-1,bx,by);
		int32_t f2 = MAPCOMBOFLAG2(lyr-1,bx,by);
		int32_t t = combobuf[MAPCOMBOL(lyr,bx,by)].type;
		if (lyr == 0) t = combobuf[MAPCOMBO(bx,by)].type;
		
		if((t==cPUSH_WAIT || t==cPUSH_HW || t==cPUSH_HW2) && (pushing<16 || hasMainGuy())) continue;
		
		if((t==cPUSH_HW || t==cPUSH_HEAVY || t==cPUSH_HEAVY2 || t==cPUSH_HW2)
				&& (itemid<0 || itemsbuf[itemid].power<((t==cPUSH_HEAVY2 || t==cPUSH_HW2)?2:1) ||
					((itemid>=0 && itemsbuf[itemid].flags & ITEM_FLAG1) && (didstuff&did_glove)))) continue;
		
		bool doit=false;
		bool changeflag=false;
		bool changecombo=false;
		
		if(((f==mfPUSHUD || f==mfPUSHUDNS|| f==mfPUSHUDINS) && dir<=down) ||
				((f==mfPUSHLR || f==mfPUSHLRNS|| f==mfPUSHLRINS) && dir>=left) ||
				((f==mfPUSHU || f==mfPUSHUNS || f==mfPUSHUINS) && dir==up) ||
				((f==mfPUSHD || f==mfPUSHDNS || f==mfPUSHDINS) && dir==down) ||
				((f==mfPUSHL || f==mfPUSHLNS || f==mfPUSHLINS) && dir==left) ||
				((f==mfPUSHR || f==mfPUSHRNS || f==mfPUSHRINS) && dir==right) ||
				f==mfPUSH4 || f==mfPUSH4NS || f==mfPUSH4INS)
		{
			changeflag=true;
			doit=true;
		}
		
		if((((f2==mfPUSHUD || f2==mfPUSHUDNS|| f2==mfPUSHUDINS) && dir<=down) ||
				((f2==mfPUSHLR || f2==mfPUSHLRNS|| f2==mfPUSHLRINS) && dir>=left) ||
				((f2==mfPUSHU || f2==mfPUSHUNS || f2==mfPUSHUINS) && dir==up) ||
				((f2==mfPUSHD || f2==mfPUSHDNS || f2==mfPUSHDINS) && dir==down) ||
				((f2==mfPUSHL || f2==mfPUSHLNS || f2==mfPUSHLINS) && dir==left) ||
				((f2==mfPUSHR || f2==mfPUSHRNS || f2==mfPUSHRINS) && dir==right) ||
				f2==mfPUSH4 || f2==mfPUSH4NS || f2==mfPUSH4INS)&&(f!=mfPUSHED))
		{
			changecombo=true;
			doit=true;
		}
		
		if(get_bit(quest_rules,qr_SOLIDBLK))
		{
			switch(dir)
			{
			case up:
				if(_walkflag(bx,by-8,2,SWITCHBLOCK_STATE)&&!(MAPFLAG2(lyr-1,bx,by-8)==mfBLOCKHOLE||MAPCOMBOFLAG2(lyr-1,bx,by-8)==mfBLOCKHOLE))    doit=false;
				
				break;
				
			case down:
				if(_walkflag(bx,by+24,2,SWITCHBLOCK_STATE)&&!(MAPFLAG2(lyr-1,bx,by+24)==mfBLOCKHOLE||MAPCOMBOFLAG2(lyr-1,bx,by+24)==mfBLOCKHOLE))   doit=false;
				
				break;
				
			case left:
				if(_walkflag(bx-16,by+8,2,SWITCHBLOCK_STATE)&&!(MAPFLAG2(lyr-1,bx-16,by+8)==mfBLOCKHOLE||MAPCOMBOFLAG2(lyr-1,bx-16,by+8)==mfBLOCKHOLE)) doit=false;
				
				break;
				
			case right:
				if(_walkflag(bx+16,by+8,2,SWITCHBLOCK_STATE)&&!(MAPFLAG2(lyr-1,bx+16,by+8)==mfBLOCKHOLE||MAPCOMBOFLAG2(lyr-1,bx+16,by+8)==mfBLOCKHOLE)) doit=false;
				
				break;
			}
		}
		
		switch(dir)
		{
		case up:
			if((MAPFLAG2(lyr-1,bx,by-8)==mfNOBLOCKS||MAPCOMBOFLAG2(lyr-1,bx,by-8)==mfNOBLOCKS))       doit=false;
			
			break;
			
		case down:
			if((MAPFLAG2(lyr-1,bx,by+24)==mfNOBLOCKS||MAPCOMBOFLAG2(lyr-1,bx,by+24)==mfNOBLOCKS))     doit=false;
			
			break;
			
		case left:
			if((MAPFLAG2(lyr-1,bx-16,by+8)==mfNOBLOCKS||MAPCOMBOFLAG2(lyr-1,bx-16,by+8)==mfNOBLOCKS)) doit=false;
			
			break;
			
		case right:
			if((MAPFLAG2(lyr-1,bx+16,by+8)==mfNOBLOCKS||MAPCOMBOFLAG2(lyr-1,bx+16,by+8)==mfNOBLOCKS)) doit=false;
			
			break;
		}
		
		if(doit)
		{
			if(itemid>=0 && itemsbuf[itemid].flags & ITEM_FLAG1) didstuff|=did_glove;
			
			//   for(int32_t i=0; i<1; i++)
			if(!blockmoving)
			{
				if(changeflag)
				{
					m->sflag[combopos]=0;
				}
				
				if(mblock2.clk<=0)
				{
					mblock2.blockLayer = lyr;
					mblock2.push((zfix)bx,(zfix)by,dir,f);
					
					if(get_bit(quest_rules,qr_MORESOUNDS))
						sfx(WAV_ZN1PUSHBLOCK,(int32_t)x);
				}
			}
			break;
		}
	}
}

bool usekey()
{
    int32_t itemid = current_item_id(itype_magickey);
    
    if(itemid<0 ||
            (itemsbuf[itemid].flags & ITEM_FLAG1 ? itemsbuf[itemid].power<dlevel
             : itemsbuf[itemid].power!=dlevel))
    {
        if(game->lvlkeys[dlevel]!=0)
        {
            game->lvlkeys[dlevel]--;
	    //run script for level key item
		int32_t key_item = 0; //current_item_id(itype_lkey); //not possible
		for ( int32_t q = 0; q < MAXITEMS; ++q )
		{
			if ( itemsbuf[q].family == itype_lkey )
			{
				key_item = q; break;
			}
		}
		//zprint2("key_item is: %d\n",key_item);
		//zprint2("key_item script is: %d\n",itemsbuf[key_item].script);
		if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
		{
			ri = &(itemScriptData[key_item]);
			for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
			ri->Clear();
			item_doscript[key_item] = 1;
			itemscriptInitialised[key_item] = 0;
			ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
			FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
		}
		return true;
        }
        
	else
	{
		if(game->get_keys()==0)
		{
		    return false;
		}
		else 
		{
			//run script for key item
			int32_t key_item = 0; //current_item_id(itype_key); //not possible
			for ( int32_t q = 0; q < MAXITEMS; ++q )
			{
				if ( itemsbuf[q].family == itype_key )
				{
					key_item = q; break;
				}
			}
			//zprint2("key_item is: %d\n",key_item);
			//zprint2("key_item script is: %d\n",itemsbuf[key_item].script);
			if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
			{
				ri = &(itemScriptData[key_item]);
				for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
				ri->Clear();
				item_doscript[key_item] = 1;
				itemscriptInitialised[key_item] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
				FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
			}
			game->change_keys(-1);
		}
	}
    }
    
    return true;
}

bool usekey(int32_t num)
{
    int32_t itemid = current_item_id(itype_magickey);
    
    if(itemid<0 ||
            (itemsbuf[itemid].flags & ITEM_FLAG1 ? itemsbuf[itemid].power<dlevel
             : itemsbuf[itemid].power!=dlevel))
    {
        if(game->lvlkeys[dlevel]>=num)
        {
            game->lvlkeys[dlevel]-=num;
            return true;
        }
        
        if(game->get_keys()<num)
            return false;
            
        game->change_keys(-num);
    }
    
    return true;
}

bool islockeddoor(int32_t x, int32_t y, int32_t lock)
{
    int32_t mc = (y&0xF0)+(x>>4);
    bool ret = (((mc==7||mc==8||mc==23||mc==24) && tmpscr->door[up]==lock)
                || ((mc==151||mc==152||mc==167||mc==168) && tmpscr->door[down]==lock)
                || ((mc==64||mc==65||mc==80||mc==81) && tmpscr->door[left]==lock)
                || ((mc==78||mc==79||mc==94||mc==95) && tmpscr->door[right]==lock));
    return ret;
}

void LinkClass::checklockblock()
{
    if(toogam) return;
    
    int32_t bx = x.getInt()&0xF0;
    int32_t bx2 = int32_t(x+8)&0xF0;
    int32_t by = y.getInt()&0xF0;
    
    switch(dir)
    {
    case up:
        if(!((int32_t)y&15)&&y!=0) by-=bigHitbox ? 16 : 0;
        
        break;
        
    case down:
        by+=16;
        break;
        
    case left:
        if((((int32_t)x)&0x0F)<8)
            bx-=16;
        
        if(y.getInt()&8)
        {
            by+=16;
        }
        
        bx2=bx;
        break;
        
    case right:
        bx+=16;
        
        if(y.getInt()&8)
        {
            by+=16;
        }
        
        bx2=bx;
        break;
    }
    
    bool found1=false;
    bool found2=false;
    int32_t foundlayer = -1;
    int32_t cid = 0;
    // Layer 0 is overridden by Locked Doors
    if((combobuf[MAPCOMBO(bx,by)].type==cLOCKBLOCK && _effectflag(bx,by,1, -1) && !islockeddoor(bx,by,dLOCKED)))
    {
	found1=true;
	cid = MAPCOMBO(bx,by);
	foundlayer = 0;
    }
    else if (combobuf[MAPCOMBO(bx2,by)].type==cLOCKBLOCK && _effectflag(bx2,by,1, -1) && !islockeddoor(bx2,by,dLOCKED))
    {
        found2=true;
	cid = MAPCOMBO(bx2,by);
	foundlayer = 0;
    }
    
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[i]))) found1 = false;
			if (combobuf[MAPCOMBO2(i,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[i]))) found2 = false;
		}
	}
    
   
    // Layers
    if(!(found1 || found2))
    {
	cid = 0;
	foundlayer = -1;
        for(int32_t i=0; i<2; i++)
        {
	    if (i == 0)
	    {
		if(tmpscr2[1].valid!=0)
		{
			if (combobuf[MAPCOMBO2(1,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[1]))) continue;
			if (combobuf[MAPCOMBO2(1,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[1]))) continue;
		} 
	    }
            if(combobuf[MAPCOMBO2(i,bx,by)].type==cLOCKBLOCK && _effectflag(bx,by,1, i))
	    {
                found1=true;
		foundlayer = i;
		cid = MAPCOMBO2(i,bx,by);
		//zprint("Found layer: %d \n", i);
                break;
            }
		    
	    else if(combobuf[MAPCOMBO2(i,bx2,by)].type==cLOCKBLOCK && _effectflag(bx2,by,1, i))
            {
                found2=true;
		foundlayer = i;
		cid = MAPCOMBO2(i,bx,by);
		//zprint("Found layer: %d \n", i);
                break;
            }
        }
    }
    
    if(!(found1 || found2) || pushing<8)
    {
        return;
    }
    //zprint("foundlayer: %d\n", foundlayer);
    //zprint("cid: %d\n", cid);
    //zprint("MAPCOMBO2(foundlayer,bx2,by): %d\n", MAPCOMBO2(foundlayer,bx2,by));
    int32_t requireditem = combobuf[cid].usrflags&cflag1 ? combobuf[cid].attribytes[0] : 0;
    int32_t itemonly = combobuf[cid].usrflags&cflag2;
    int32_t thecounter = combobuf[cid].attribytes[1];
    int32_t ctr_amount = combobuf[cid].attributes[0]/10000L;
    if( requireditem && game->item[requireditem]) 
    {
	    if ((combobuf[cid].usrflags&cflag5)) 
	    {
		    //zprint("Setting item %d false.\n", requireditem);
		    takeitem(requireditem);
	    }
	    goto unlock;
    }
    else if ( (combobuf[cid].usrflags&cflag4) )
    {
	if ( game->get_counter(thecounter) >= ctr_amount )
	{
		//flag 6 only checks the required count; it doesn't drain it
		if (!(combobuf[cid].usrflags&cflag7)) game->change_counter(-(ctr_amount), thecounter);
		goto unlock; 
	}
	else if ( game->get_counter(thecounter) < ctr_amount && (combobuf[cid].usrflags&cflag6) ) //eat counter even if insufficient, but don't unlock
	{
		//shadowtiger requested this on 29th Dec, 2019 -Z
		if (!(combobuf[cid].usrflags&cflag7)) game->change_counter(-(game->get_counter(thecounter)), thecounter);
		return;
	}
    }
    else if (ctr_amount && usekey(ctr_amount) ) goto unlock;
    else if(!ctr_amount && !requireditem && usekey() && !itemonly ) goto unlock;
    
    
    return;
     
    
    unlock:
    
    
    setmapflag(mLOCKBLOCK);
    remove_lockblocks((currscr>=128)?1:0);
    if ( combobuf[cid].usrflags&cflag3 )
    {
	if ( (combobuf[cid].attribytes[3]) )
		sfx(combobuf[cid].attribytes[3]);
    }
	    
    else sfx(WAV_DOOR);
}

void LinkClass::checkbosslockblock()
{
    if(toogam) return;
    
    int32_t bx = x.getInt()&0xF0;
    int32_t bx2 = int32_t(x+8)&0xF0;
    int32_t by = y.getInt()&0xF0;
    
    switch(dir)
    {
    case up:
        if(!((int32_t)y&15)&&y!=0) by-=bigHitbox ? 16 : 0;
        
        break;
        
    case down:
        by+=16;
        break;
        
    case left:
        if((((int32_t)x)&0x0F)<8)
            bx-=16;
        
        if(y.getInt()&8)
        {
            by+=16;
        }
        
        bx2=bx;
        break;
        
    case right:
        bx+=16;
        
        if(y.getInt()&8)
        {
            by+=16;
        }
        
        bx2=bx;
        break;
    }
    
    bool found=false;
    
    if((combobuf[MAPCOMBO(bx,by)].type==cBOSSLOCKBLOCK && _effectflag(bx,by,1, -1) && !islockeddoor(bx,by,dBOSS))||
            (combobuf[MAPCOMBO(bx2,by)].type==cBOSSLOCKBLOCK && _effectflag(bx2,by,1, -1) && !islockeddoor(bx,by,dBOSS)))
    {
        found=true;
    }
    
    for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[i]))) found = false;
			if (combobuf[MAPCOMBO2(i,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[i]))) found = false;
		}
	}
    
    if(!found)
    {
        for(int32_t i=0; i<2; i++)
        {
	    if (i == 0)
	    {
		if(tmpscr2[1].valid!=0)
		{
			if (combobuf[MAPCOMBO2(1,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[1]))) continue;
			if (combobuf[MAPCOMBO2(1,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[1]))) continue;
		} 
	    }
            if((combobuf[MAPCOMBO2(i,bx,by)].type==cBOSSLOCKBLOCK && _effectflag(bx,by,1, i))||
                    (combobuf[MAPCOMBO2(i,bx2,by)].type==cBOSSLOCKBLOCK && _effectflag(bx2,by,1, i)))
            {
                found=true;
                break;
            }
        }
    }
    
    if(!found || pushing<8)
    {
        return;
    }
    
    if(!(game->lvlitems[dlevel]&liBOSSKEY)) return;
	
    
	// Run Boss Key Script
	int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
	for ( int32_t q = 0; q < MAXITEMS; ++q )
	{
		if ( itemsbuf[q].family == itype_bosskey )
		{
			key_item = q; break;
		}
	}
	if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
	{
		ri = &(itemScriptData[key_item]);
		for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
		ri->Clear();
		item_doscript[key_item] = 1;
		itemscriptInitialised[key_item] = 0;
		ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
		FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
	}
    
    setmapflag(mBOSSLOCKBLOCK);
    remove_bosslockblocks((currscr>=128)?1:0);
    sfx(WAV_DOOR);
}

void LinkClass::oldcheckchest(int32_t type)
{
	// chests aren't affected by tmpscr->flags2&fAIRCOMBOS
	if(toogam || z>0) return;
	if(pushing<8) return;
	int32_t bx = x.getInt()&0xF0;
	int32_t bx2 = int32_t(x+8)&0xF0;
	int32_t by = y.getInt()&0xF0;
	
	switch(dir)
	{
		case up:
			if(isSideViewLink()) return;
			
			if(!((int32_t)y&15)&&y!=0) by-=bigHitbox ? 16 : 0;
			
			break;
			
		case left:
		case right:
			if(isSideViewLink()) break;
			
		case down:
			return;
	}
	
	bool found=false;
	bool itemflag=false;
	
	if((combobuf[MAPCOMBO(bx,by)].type==type && _effectflag(bx,by,1, -1))||
			(combobuf[MAPCOMBO(bx2,by)].type==type && _effectflag(bx2,by,1, -1)))
	{
		found=true;
	}
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[i]))) found = false;
			if (combobuf[MAPCOMBO2(i,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[i]))) found = false;
		}
	}
	
	if(!found)
	{
		for(int32_t i=0; i<2; i++)
		{
			if (i == 0)
			{
				if(tmpscr2[1].valid!=0)
				{
					if (combobuf[MAPCOMBO2(1,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[1]))) continue;
					if (combobuf[MAPCOMBO2(1,bx2,by)].type == cBRIDGE && !_walkflag_layer(bx2,by,1, &(tmpscr2[1]))) continue;
				}    
			}
			if((combobuf[MAPCOMBO2(i,bx,by)].type==type && _effectflag(bx,by,1, i))||
					(combobuf[MAPCOMBO2(i,bx2,by)].type==type && _effectflag(bx2,by,1, i)))
			{
				found=true;
				break;
			}
		}
	}
	
	if(!found)
	{
		return;
	}
	
	switch(type)
	{
		case cLOCKEDCHEST:
			if(!usekey()) return;
			
			setmapflag(mLOCKEDCHEST);
			break;
			
		case cCHEST:
			setmapflag(mCHEST);
			break;
			
		case cBOSSCHEST:
			if(!(game->lvlitems[dlevel]&liBOSSKEY)) return;
			// Run Boss Key Script
			int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
			for ( int32_t q = 0; q < MAXITEMS; ++q )
			{
				if ( itemsbuf[q].family == itype_bosskey )
				{
					key_item = q; break;
				}
			}
			if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
			{
				ri = &(itemScriptData[key_item]);
				for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
				ri->Clear();
				item_doscript[key_item] = 1;
				itemscriptInitialised[key_item] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
				FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
			}
			setmapflag(mBOSSCHEST);
			break;
	}
	
	itemflag |= MAPCOMBOFLAG(bx,by)==mfARMOS_ITEM;
	itemflag |= MAPCOMBOFLAG(bx2,by)==mfARMOS_ITEM;
	itemflag |= MAPFLAG(bx,by)==mfARMOS_ITEM;
	itemflag |= MAPFLAG(bx2,by)==mfARMOS_ITEM;
	itemflag |= MAPCOMBOFLAG(bx,by)==mfARMOS_ITEM;
	itemflag |= MAPCOMBOFLAG(bx2,by)==mfARMOS_ITEM;
	
	if(!itemflag)
	{
		for(int32_t i=0; i<2; i++)
		{
			itemflag |= MAPFLAG2(i,bx,by)==mfARMOS_ITEM;
			itemflag |= MAPFLAG2(i,bx2,by)==mfARMOS_ITEM;
			itemflag |= MAPCOMBOFLAG2(i,bx,by)==mfARMOS_ITEM;
			itemflag |= MAPCOMBOFLAG2(i,bx2,by)==mfARMOS_ITEM;
		}
	}
	
	if(itemflag && !getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW))
	{
		items.add(new item(x, y,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
	}
}

void LinkClass::checkchest(int32_t type)
{
	if(get_bit(quest_rules,qr_OLD_CHEST_COLLISION))
	{
		oldcheckchest(type);
		return;
	}
	if(toogam || z>0) return;
	zfix bx, by;
	zfix bx2, by2;
	zfix fx(-1), fy(-1);
	switch(dir)
	{
		case up:
			by = y + (bigHitbox ? -2 : 6);
			by2 = by;
			bx = x + 4;
			bx2 = bx + 8;
			break;
		case down:
			by = y + 17;
			by2 = by;
			bx = x + 4;
			bx2 = bx + 8;
			break;
		case left:
			by = y + (bigHitbox ? 0 : 8);
			by2 = y + 8;
			bx = x - 2;
			bx2 = x - 2;
			break;
		case right:
			by = y + (bigHitbox ? 0 : 8);
			by2 = y + 8;
			bx = x + 17;
			bx2 = x + 17;
			break;
	}
	
	int32_t found = -1;
	
	if(combobuf[MAPCOMBO(bx,by)].type==type && _effectflag(bx,by,1, -1))
	{
		found = MAPCOMBO(bx,by);
		fx = bx; fy = by;
		for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[i]))) found = -1;
			}
		}
	}
	if(combobuf[MAPCOMBO(bx2,by2)].type==type && _effectflag(bx2,by2,1, -1))
	{
		found = MAPCOMBO(bx2,by2);
		fx = bx2; fy = by2;
		for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,bx2,by2)].type == cBRIDGE && !_walkflag_layer(bx2,by2,1, &(tmpscr2[i]))) found = -1;
			}
		}
	}
	
	if(found<0)
	{
		for(int32_t i=0; i<2; i++)
		{
			if(combobuf[MAPCOMBO2(i,bx,by)].type==type && _effectflag(bx,by,1, i))
			{
				found = MAPCOMBO2(i,bx,by);
				fx = bx; fy = by;
				if (i == 0 && tmpscr2[1].valid!=0)
				{
					if (combobuf[MAPCOMBO2(1,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[1]))) found = -1;
				}
			}
			if(combobuf[MAPCOMBO2(i,bx2,by2)].type==type && _effectflag(bx2,by2,1, i))
			{
				found = MAPCOMBO2(i,bx2,by2);
				fx = bx2; fy = by2;
				if (i == 0 && tmpscr2[1].valid!=0)
				{
					if (combobuf[MAPCOMBO2(1,bx2,by2)].type == cBRIDGE && !_walkflag_layer(bx2,by2,1, &(tmpscr2[1]))) found = -1;
				}
			}
			if(found>-1) break;
		}
	}
	
	if(found<0) return;
	newcombo const& cmb = combobuf[found];
	switch(dir)
	{
		case up:
			if(cmb.usrflags&cflag10)
				return;
			break;
		case down:
			if(cmb.usrflags&cflag9)
				return;
			break;
		case left:
			if(cmb.usrflags&cflag12)
				return;
			break;
		case right:
			if(cmb.usrflags&cflag11)
				return;
			break;
	}
	int32_t intbtn = cmb.attribytes[2];
	
	if(intbtn) //
	{
		if(!getIntBtnInput(intbtn, true, true, false, true))
			return; //Button not pressed
	}
	else if(pushing < 8) return; //Not pushing against chest enough
	
	//!TODO Add attributes from lockblocks to locked/boss locked chests
	switch(type)
	{
		case cLOCKEDCHEST:
			if(!usekey()) return;
			
			setmapflag(mLOCKEDCHEST);
			break;
			
		case cCHEST:
			setmapflag(mCHEST);
			break;
			
		case cBOSSCHEST:
			if(!(game->lvlitems[dlevel]&liBOSSKEY)) return;
			// Run Boss Key Script
			int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
			for ( int32_t q = 0; q < MAXITEMS; ++q )
			{
				if ( itemsbuf[q].family == itype_bosskey )
				{
					key_item = q; break;
				}
			}
			if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
			{
				ri = &(itemScriptData[key_item]);
				for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
				ri->Clear();
				item_doscript[key_item] = 1;
				itemscriptInitialised[key_item] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
				FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
			}
			setmapflag(mBOSSCHEST);
			break;
	}
	
	bool itemflag = MAPCOMBOFLAG(fx,fy)==mfARMOS_ITEM;
	itemflag |= MAPFLAG(fx,fy)==mfARMOS_ITEM;
	if(!itemflag)
	{
		for(int32_t i=0; i<2; i++)
		{
			itemflag |= MAPFLAG2(i,fx,fy)==mfARMOS_ITEM;
			itemflag |= MAPCOMBOFLAG2(i,fx,fy)==mfARMOS_ITEM;
		}
	}
	
	if(itemflag && !getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW))
	{
		items.add(new item(x, y,(zfix)0, tmpscr->catchall, ipONETIME2 + ipBIGRANGE + ipHOLDUP | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0), 0));
	}
}

void LinkClass::checksigns()
{
	if(toogam || z>0) return;
	if(msg_active || (msg_onscreen && get_bit(quest_rules, qr_MSGDISAPPEAR)))
		return; //Don't overwrite a message waiting to be dismissed
	zfix bx, by;
	zfix bx2, by2;
	zfix fx(-1), fy(-1);
	switch(dir)
	{
		case up:
			by = y + (bigHitbox ? -2 : 6);
			by2 = by;
			bx = x + 4;
			bx2 = bx + 8;
			break;
		case down:
			by = y + 17;
			by2 = by;
			bx = x + 4;
			bx2 = bx + 8;
			break;
		case left:
			by = y + (bigHitbox ? 0 : 8);
			by2 = y + 8;
			bx = x - 2;
			bx2 = x - 2;
			break;
		case right:
			by = y + (bigHitbox ? 0 : 8);
			by2 = y + 8;
			bx = x + 17;
			bx2 = x + 17;
			break;
	}
	
	int32_t found = -1;
	
	if(combobuf[MAPCOMBO(bx,by)].type==cSIGNPOST && _effectflag(bx,by,1, -1))
	{
		found = MAPCOMBO(bx,by);
		fx = bx; fy = by;
		for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[i]))) found = -1;
			}
		}
	}
	if(combobuf[MAPCOMBO(bx2,by2)].type==cSIGNPOST && _effectflag(bx2,by2,1, -1))
	{
		found = MAPCOMBO(bx2,by2);
		fx = bx2; fy = by2;
		for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,bx2,by2)].type == cBRIDGE && !_walkflag_layer(bx2,by2,1, &(tmpscr2[i]))) found = -1;
			}
		}
	}
	
	if(found<0)
	{
		for(int32_t i=0; i<2; i++)
		{
			if(combobuf[MAPCOMBO2(i,bx,by)].type==cSIGNPOST && _effectflag(bx,by,1, i))
			{
				found = MAPCOMBO2(i,bx,by);
				fx = bx; fy = by;
				if (i == 0 && tmpscr2[1].valid!=0)
				{
					if (combobuf[MAPCOMBO2(1,bx,by)].type == cBRIDGE && !_walkflag_layer(bx,by,1, &(tmpscr2[1]))) found = -1;
				}
			}
			if(combobuf[MAPCOMBO2(i,bx2,by2)].type==cSIGNPOST && _effectflag(bx2,by2,1, i))
			{
				found = MAPCOMBO2(i,bx2,by2);
				fx = bx2; fy = by2;
				if (i == 0 && tmpscr2[1].valid!=0)
				{
					if (combobuf[MAPCOMBO2(1,bx2,by2)].type == cBRIDGE && !_walkflag_layer(bx2,by2,1, &(tmpscr2[1]))) found = -1;
				}
			}
			if(found>-1) break;
		}
	}
	
	if(found<0) return;
	newcombo const& cmb = combobuf[found];
	switch(dir)
	{
		case up:
			if(cmb.usrflags&cflag10)
				return;
			break;
		case down:
			if(cmb.usrflags&cflag9)
				return;
			break;
		case left:
			if(cmb.usrflags&cflag12)
				return;
			break;
		case right:
			if(cmb.usrflags&cflag11)
				return;
			break;
	}
	int32_t intbtn = cmb.attribytes[2];
	
	if(intbtn) //
	{
		if(!getIntBtnInput(intbtn, true, true, false, true))
			return; //Button not pressed
	}
	else if(pushing < 8 || pushing%8) return; //Not pushing against sign enough
	
	int32_t str = cmb.attributes[0]/10000L;
	switch(str)
	{
		case -1: //Special case: Use Screen String
			str = tmpscr->str;
			break;
		case -2: //Special case: Use Screen Catchall
			str = tmpscr->catchall;
			break;
		case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17: //Special case: Screen->D[]
			int32_t di = ((get_currdmap())<<7) + get_currscr()-(DMaps[get_currdmap()].type==dmOVERW ? 0 : DMaps[get_currdmap()].xoff);
			str = game->screen_d[di][abs(str)-10] / 10000L;
			break;
	}
	if(unsigned(str) >= MAXMSGS)
		str = 0;
	if(str)
		donewmsg(str);
}

void LinkClass::checklocked()
{
    if(toogam) return; //Walk through walls. 
    
    if(!isdungeon()) return;
    
    if( !diagonalMovement && pushing!=8) return;
	/*This is required to allow the player to open a door, while sliding along a wall (pressing in the direction of the door, and sliding left or right)
	*/
	if ( diagonalMovement && pushing < 8 ) return; //Allow wall walking Should I add a quest rule for this? -Z
    
	
	bool found = false;
	for ( int32_t q = 0; q < 4; q++ ) {
		if ( tmpscr->door[q] == dLOCKED || tmpscr->door[q] == dBOSS ) { found = true; }
	}
	
    if ( !found ) return;
    
    int32_t si = (currmap<<7) + currscr;
    int32_t di = NULL;
    
    
	
	if ( diagonalMovement || get_bit(quest_rules, qr_DISABLE_4WAY_GRIDLOCK)) 
	{
		//Up door
		if ( y <= 32 && x >= 112 && x <= 128 )
		{
			if (
				dir == up || dir == l_up || dir == r_up //|| Up() || ( Up()&&Left()) || ( Up()&&Right()) 
				
			)
			{
				di = nextscr(up);
				if(tmpscr->door[0]==dLOCKED)
				{
				    if(usekey())
				    {
					putdoor(scrollbuf,0,up,dUNLOCKED);
					tmpscr->door[0]=dUNLOCKED;
					setmapflag(si, mDOOR_UP);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_DOWN);
					sfx(WAV_DOOR);
					markBmap(-1);
				    }
				    else return;
				}
				else if(tmpscr->door[0]==dBOSS)
				{
				    if(game->lvlitems[dlevel]&liBOSSKEY)
				    {
					putdoor(scrollbuf,0,up,dOPENBOSS);
					tmpscr->door[0]=dOPENBOSS;
					setmapflag(si, mDOOR_UP);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_DOWN);
					sfx(WAV_DOOR);
					markBmap(-1);
					// Run Boss Key Script
					int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
					for ( int32_t q = 0; q < MAXITEMS; ++q )
					{
						if ( itemsbuf[q].family == itype_bosskey )
						{
							key_item = q; break;
						}
					}
					if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
					{
						ri = &(itemScriptData[key_item]);
						for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
						ri->Clear();
						item_doscript[key_item] = 1;
						itemscriptInitialised[key_item] = 0;
						ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
						FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
					}
				    }
				    else return;

				}
					
			}
		}
		//Down
		if ( y >= 128 && x >= 112 && x <= 128 ) 
		{
			if ( dir == down || dir == l_down || dir == r_down ) //|| Down() || ( Down()&&Left()) || ( Down()&&Right()))
			{
				di = nextscr(down);
				if(tmpscr->door[1]==dLOCKED)
				{
				    if(usekey())
				    {
					putdoor(scrollbuf,0,down,dUNLOCKED);
					tmpscr->door[1]=dUNLOCKED;
					setmapflag(si, mDOOR_DOWN);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_UP);
					sfx(WAV_DOOR);
					markBmap(-1);
				    }
				    else return;
				}
				else if(tmpscr->door[1]==dBOSS)
				{
				    if(game->lvlitems[dlevel]&liBOSSKEY)
				    {
					putdoor(scrollbuf,0,down,dOPENBOSS);
					tmpscr->door[1]=dOPENBOSS;
					setmapflag(si, mDOOR_DOWN);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_UP);
					sfx(WAV_DOOR);
					markBmap(-1);
					// Run Boss Key Script
					int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
					for ( int32_t q = 0; q < MAXITEMS; ++q )
					{
						if ( itemsbuf[q].family == itype_bosskey )
						{
							key_item = q; break;
						}
					}
					if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
					{
						ri = &(itemScriptData[key_item]);
						for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
						ri->Clear();
						item_doscript[key_item] = 1;
						itemscriptInitialised[key_item] = 0;
						ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
						FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
					}
				    }
				    else return;
				}
			}
		}
		//left
		if ( y > 72 && y < 88 && x <= 32 )
		{
			if ( dir == left || dir == l_up || dir == l_down )//|| Left()  || ( Up()&&Left()) || ( Down()&&Left() ) )
			{
				di = nextscr(left);
				if(tmpscr->door[2]==dLOCKED)
				{
				    if(usekey())
				    {
					putdoor(scrollbuf,0,left,dUNLOCKED);
					tmpscr->door[2]=dUNLOCKED;
					setmapflag(si, mDOOR_LEFT);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_RIGHT);
					sfx(WAV_DOOR);
					markBmap(-1);
				    }
				    else return;
				}
				else if(tmpscr->door[2]==dBOSS)
				{
				    if(game->lvlitems[dlevel]&liBOSSKEY)
				    {
					putdoor(scrollbuf,0,left,dOPENBOSS);
					tmpscr->door[2]=dOPENBOSS;
					setmapflag(si, mDOOR_LEFT);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_RIGHT);
					sfx(WAV_DOOR);
					markBmap(-1);
					// Run Boss Key Script
					int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
					for ( int32_t q = 0; q < MAXITEMS; ++q )
					{
						if ( itemsbuf[q].family == itype_bosskey )
						{
							key_item = q; break;
						}
					}
					if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
					{
						ri = &(itemScriptData[key_item]);
						for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
						ri->Clear();
						item_doscript[key_item] = 1;
						itemscriptInitialised[key_item] = 0;
						ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
						FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
					}
				    }
				    else return;
				}
			}
		}
		
		
		//right
		if ( ( y > 72 && y < 88 ) && x >= 208 )
			//!( (y<=72||y>=88) && x<206 ) )
			//y<=72||y>=88):y!=80) || x<208)
		{
			if ( dir == right || dir == r_up || dir == r_down ) //|| Right()  || ( Down()&&Right() ) || ( Up()&&Right()))
			{
				di  = nextscr(right);
				if(tmpscr->door[right]==dLOCKED)
				{
				    if(usekey())
				    {
					putdoor(scrollbuf,0,right,dUNLOCKED);
					tmpscr->door[3]=dUNLOCKED;
					setmapflag(si, mDOOR_RIGHT);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_LEFT);
					sfx(WAV_DOOR);
					markBmap(-1);
				    }
				    else return;
				}
				else if(tmpscr->door[right]==dBOSS)
				{
				    if(game->lvlitems[dlevel]&liBOSSKEY)
				    {
					putdoor(scrollbuf,0,right,dOPENBOSS);
					tmpscr->door[3]=dOPENBOSS;
					setmapflag(si, mDOOR_RIGHT);
					
					if(di != 0xFFFF)
					    setmapflag(di, mDOOR_LEFT);
					sfx(WAV_DOOR);
					markBmap(-1);
					// Run Boss Key Script
					int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
					for ( int32_t q = 0; q < MAXITEMS; ++q )
					{
						if ( itemsbuf[q].family == itype_bosskey )
						{
							key_item = q; break;
						}
					}
					if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
					{
						ri = &(itemScriptData[key_item]);
						for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
						ri->Clear();
						item_doscript[key_item] = 1;
						itemscriptInitialised[key_item] = 0;
						ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
						FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
					}
				    }
				    else return;
				}
			
			}
		}
	}
	else
	{
		//orthogonal movement
		//Up door
		if ( y<=32 && x == 120 )
			//!( y>32 && (x!=120) ))
		{
			switch ( dir ) 
			{
				case up:
				case r_up:
				case l_up:
				{
					di  = nextscr(up);
					if(tmpscr->door[0]==dLOCKED)
					{
					    if(usekey())
					    {
						putdoor(scrollbuf,0,up,dUNLOCKED);
						tmpscr->door[0]=dUNLOCKED;
						setmapflag(si, mDOOR_UP);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_DOWN);
						sfx(WAV_DOOR);
						markBmap(-1);
					    }
					    else return;
					}
					else if(tmpscr->door[0]==dBOSS)
					{
					    if(game->lvlitems[dlevel]&liBOSSKEY)
					    {
						putdoor(scrollbuf,0,up,dOPENBOSS);
						tmpscr->door[0]=dOPENBOSS;
						setmapflag(si, mDOOR_UP);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_DOWN);
						sfx(WAV_DOOR);
						markBmap(-1);
						// Run Boss Key Script
						int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
						for ( int32_t q = 0; q < MAXITEMS; ++q )
						{
							if ( itemsbuf[q].family == itype_bosskey )
							{
								key_item = q; break;
							}
						}
						if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
						{
							ri = &(itemScriptData[key_item]);
							for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
							ri->Clear();
							item_doscript[key_item] = 1;
							itemscriptInitialised[key_item] = 0;
							ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
							FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
						}
					    }
					    else return;
					}
					break;
				}
				default: break;
					
			}
		}
		//Down
		if ( y >= 128 && x == 120 )
			//!(y<128 && (x!=120) ) )
		{
			switch(dir)
			{
				case down:
				case l_down:
				case r_down:
				{
					di  = nextscr(down);
					
					if(tmpscr->door[1]==dLOCKED)
					{
					    if(usekey())
					    {
						putdoor(scrollbuf,0,down,dUNLOCKED);
						tmpscr->door[1]=dUNLOCKED;
						setmapflag(si, mDOOR_DOWN);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_UP);
						sfx(WAV_DOOR);
						markBmap(-1);
					    }
					    else return;
					}
					else if(tmpscr->door[1]==dBOSS)
					{
					    if(game->lvlitems[dlevel]&liBOSSKEY)
					    {
						putdoor(scrollbuf,0,down,dOPENBOSS);
						tmpscr->door[1]=dOPENBOSS;
						setmapflag(si, mDOOR_DOWN);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_UP);
						sfx(WAV_DOOR);
						markBmap(-1);
						// Run Boss Key Script
						int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
						for ( int32_t q = 0; q < MAXITEMS; ++q )
						{
							if ( itemsbuf[q].family == itype_bosskey )
							{
								key_item = q; break;
							}
						}
						if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
						{
							ri = &(itemScriptData[key_item]);
							for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
							ri->Clear();
							item_doscript[key_item] = 1;
							itemscriptInitialised[key_item] = 0;
							ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
							FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
						}
					    }
					    else return;
					}
					break;
				}
				default: break;
			}
		}
		//left
		if ( y == 80 && x <= 32 )
			//!( (y!=80) && x>32 ) )
		{
			switch(dir)
			{
				case left:
				case l_up:
				case l_down:
				{
					di  = nextscr(left);
					if(tmpscr->door[2]==dLOCKED)
					{
					    if(usekey())
					    {
						putdoor(scrollbuf,0,left,dUNLOCKED);
						tmpscr->door[2]=dUNLOCKED;
						setmapflag(si, mDOOR_LEFT);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_RIGHT);
						sfx(WAV_DOOR);
						markBmap(-1);
					    }
					    else return;
					}
					else if(tmpscr->door[2]==dBOSS)
					{
					    if(game->lvlitems[dlevel]&liBOSSKEY)
					    {
						putdoor(scrollbuf,0,left,dOPENBOSS);
						tmpscr->door[2]=dOPENBOSS;
						setmapflag(si, mDOOR_LEFT);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_RIGHT);
						sfx(WAV_DOOR);
						markBmap(-1);
						// Run Boss Key Script
						int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
						for ( int32_t q = 0; q < MAXITEMS; ++q )
						{
							if ( itemsbuf[q].family == itype_bosskey )
							{
								key_item = q; break;
							}
						}
						if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) 
						{
							ri = &(itemScriptData[key_item]);
							for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
							ri->Clear();
							item_doscript[key_item] = 1;
							itemscriptInitialised[key_item] = 0;
							ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
							FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
						}
					    }
					    else return;
					}
					
					break;	
					
				}
				default: break;
			}
		}
		//right
		if ( y == 80 && x >= 208 )
			//!((y!=80) && x<208 ) )
		{
			switch(dir)
			{
				case right:
				case r_down:
				case r_up:
				{
					di  = nextscr(right);
					if(tmpscr->door[3]==dLOCKED)
					{
					    if(usekey())
					    {
						putdoor(scrollbuf,0,right,dUNLOCKED);
						tmpscr->door[3]=dUNLOCKED;
						setmapflag(si, mDOOR_RIGHT);
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_LEFT);
						sfx(WAV_DOOR);
						markBmap(-1);
					    }
					    else return;
					}
					else if(tmpscr->door[3]==dBOSS)
					{
					    if(game->lvlitems[dlevel]&liBOSSKEY)
					    {
						putdoor(scrollbuf,0,right,dOPENBOSS);
						tmpscr->door[3]=dOPENBOSS;
						setmapflag(si, mDOOR_RIGHT);
						
						
						if(di != 0xFFFF)
						    setmapflag(di, mDOOR_LEFT);
						sfx(WAV_DOOR);
						markBmap(-1);
						// Run Boss Key Script
						int32_t key_item = 0; //current_item_id(itype_bosskey); //not possible
						for ( int32_t q = 0; q < MAXITEMS; ++q )
						{
							if ( itemsbuf[q].family == itype_bosskey )
							{
								key_item = q; break;
							}
						}
						if ( key_item > 0 && itemsbuf[key_item].script && !item_doscript[key_item] ) //
						{
							ri = &(itemScriptData[key_item]);
							for ( int32_t q = 0; q < 1024; q++ ) item_stack[key_item][q] = 0xFFFF;
							ri->Clear();
							item_doscript[key_item] = 1;
							itemscriptInitialised[key_item] = 0;
							ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[key_item].script, key_item);
							FFCore.deallocateAllArrays(SCRIPT_ITEM,(key_item));
						}

					    }
					    else return;
					}
					
					
					break;
				}
				default: break;
				
			}
		}
	}
}

void LinkClass::checkswordtap()
{
    if(attack!=wSword || charging<=0 || pushing<8) return;
    
    int32_t bx=x;
    int32_t by=y+8;
    
    switch(dir)
    {
    case up:
        if(!Up()) return;
        
        by-=16;
        break;
        
    case down:
        if(!Down()) return;
        
        by+=16;
        bx+=8;
        break;
        
    case left:
        if(!Left()) return;
        
        bx-=16;
        by+=8;
        break;
        
    case right:
        if(!Right()) return;
        
        bx+=16;
        by+=8;
        break;
    }
    
    if(!_walkflag(bx,by,0,SWITCHBLOCK_STATE)) return;
    
    attackclk=SWORDTAPFRAME;
    pushing=-8; //16 frames between taps
    tapping=true;
    
    int32_t type = COMBOTYPE(bx,by);
    
    if(!isCuttableType(type))
    {
        bool hollow = (MAPFLAG(bx,by) == mfBOMB || MAPCOMBOFLAG(bx,by) == mfBOMB ||
                       MAPFLAG(bx,by) == mfSBOMB || MAPCOMBOFLAG(bx,by) == mfSBOMB);
                       
        // Layers
        for(int32_t i=0; i < 6; i++)
            hollow = (hollow || MAPFLAG2(i,bx,by) == mfBOMB || MAPCOMBOFLAG2(i,bx,by) == mfBOMB ||
                      MAPFLAG2(i,bx,by) == mfSBOMB || MAPCOMBOFLAG2(i,bx,by) == mfSBOMB);
                      
        for(int32_t i=0; i<4; i++)
            if(tmpscr->door[i]==dBOMB && i==dir)
                switch(i)
                {
                case up:
                case down:
                    if(bx>=112 && bx<144 && (by>=144 || by<=32)) hollow=true;
                    
                    break;
                    
                case left:
                case right:
                    if(by>=72 && by<104 && (bx>=224 || bx<=32)) hollow=true;
                    
                    break;
                }
                
        sfx(hollow ? WAV_ZN1TAP2 : WAV_ZN1TAP,pan(x.getInt()));
    }
    
}

void LinkClass::fairycircle(int32_t type)
{
    if(fairyclk==0)
    {
        switch(type)
        {
        case REFILL_LIFE:
            if(didstuff&did_fairy) return;
            
            didstuff|=did_fairy;
            break;
            
        case REFILL_MAGIC:
            if(didstuff&did_magic) return;
            
            didstuff|=did_magic;
            break;
            
        case REFILL_ALL:
            if(didstuff&did_all) return;
            
            didstuff|=did_all;
        }
        
        refill_what=type;
        refill_why=REFILL_FAIRY;
        StartRefill(type);
        if (IsSideSwim()) {action=sideswimfreeze; FFCore.setLinkAction(sideswimfreeze);} 
	else {action=freeze; FFCore.setLinkAction(freeze);} 
        holdclk=0;
        hopclk=0;
    }
    
    ++fairyclk;
    
    if(refilling!=REFILL_FAIRYDONE)
    {
        if(!refill())
            refilling=REFILL_FAIRYDONE;
    }
    
    else if(++holdclk>80)
    {
        reset_swordcharge();
        attackclk=0;
        action=none; FFCore.setLinkAction(none);
        fairyclk=0;
        holdclk=0;
        refill_why = 0;
        refilling=REFILL_NONE;
        map_bkgsfx(true);
    }
}

int32_t touchcombo(int32_t x,int32_t y)
{
	for (int32_t i = 0; i <= 1; ++i)
	{
		if(tmpscr2[i].valid!=0)
		{
			if (combobuf[MAPCOMBO2(i,x,y)].type == cBRIDGE && !_walkflag_layer(x,y,1, &(tmpscr2[i]))) return 0;
		}
	}
	if (!_effectflag(x,y,1, -1)) return 0;
	switch(combobuf[MAPCOMBO(x,y)].type)
	{
		case cBSGRAVE:
		case cGRAVE:
			if(MAPFLAG(x,y)||MAPCOMBOFLAG(x,y)) //!DIMITODO: all flags break graves, not just push flags
			{
				break;
			}
        
		// fall through
		case cARMOS:
		{
			return combobuf[MAPCOMBO(x,y)].type;
		}
	}
    
	return 0;
}

static int32_t COMBOX(int32_t pos) { return ((pos)%16*16); }
static int32_t COMBOY(int32_t pos) { return ((pos)&0xF0); }

static int32_t GridX(int32_t x) 
{
	return (x >> 4) << 4;
}

//Snaps 'y' to the combo grid
//Equivalent to calling ComboY(ComboAt(foo,y));
static int32_t GridY(int32_t y) 
{
	return (y >> 4) << 4;
}

static int32_t typeMap[176];
static int32_t istrig[176];
static const int32_t SPTYPE_SOLID = -1;
#define SP_VISITED 0x1
#define SPFLAG(dir) (0x2<<dir)
#define BEAM_AGE_LIMIT 32
void LinkClass::handleBeam(byte* grid, size_t age, byte spotdir, int32_t curpos, byte set, bool block, bool refl)
{
	int32_t trigflag = set ? (1 << (set-1)) : ~0;
	if(spotdir > 3) return; //invalid dir
	bool doAge = true;
	byte f = 0;
	while(unsigned(curpos) < 176)
	{
		f = SPFLAG(spotdir);
		if((grid[curpos] & f) != f)
		{
			grid[curpos] |= f;
			istrig[curpos] |= trigflag;
			doAge = false;
			age = 0;
		}
		switch(spotdir)
		{
			case up:
				curpos -= 0x10;
				break;
			case down:
				curpos += 0x10;
				break;
			case left:
				if(!(curpos%0x10))
					curpos = -1;
				else --curpos;
				break;
			case right:
				++curpos;
				if(!(curpos%0x10))
					curpos = -1;
				break;
		}
		if(unsigned(curpos) >= 176) break;
		switch(typeMap[curpos])
		{
			case SPTYPE_SOLID: case cBLOCKALL:
				curpos = -1;
				break;
		}
		if((curpos==COMBOPOS(x.getInt()+8,y.getInt()+8)) && block && (spotdir == oppositeDir[dir]))
			curpos = -1;
		if(unsigned(curpos) >= 176) break;
		
		f = SPFLAG(oppositeDir[spotdir]);
		if((grid[curpos] & f) != f)
		{
			grid[curpos] |= f;
			istrig[curpos] |= trigflag;
			doAge = false;
			age = 0;
		}
		if(doAge)
		{
			if(++age > BEAM_AGE_LIMIT)
				break;
		}
		else doAge = true;
		
		if(curpos==COMBOPOS(x.getInt()+8,y.getInt() +8) && refl)
			spotdir = dir;
		else switch(typeMap[curpos])
		{
			case cMIRROR:
				spotdir = oppositeDir[spotdir];
				break;
			case cMIRRORSLASH:
				switch(spotdir)
				{
					case up:
						spotdir = right; break;
					case right:
						spotdir = up; break;
					case down:
						spotdir = left; break;
					case left:
						spotdir = down; break;
				}
				break;
			case cMIRRORBACKSLASH:
				switch(spotdir)
				{
					case up:
						spotdir = left; break;
					case left:
						spotdir = up; break;
					case down:
						spotdir = right; break;
					case right:
						spotdir = down; break;
				}
				break;
			case cMAGICPRISM:
				for(byte d = 0; d < 4; ++d)
				{
					if(d == oppositeDir[spotdir]) continue;
					handleBeam(grid, age, d, curpos, set, block, refl);
				}
				return;
			case cMAGICPRISM4:
				for(byte d = 0; d < 4; ++d)
				{
					handleBeam(grid, age, d, curpos, set, block, refl);
				}
				return;
		}
	}
}

void LinkClass::handleSpotlights()
{
	typedef byte spot_t;
	//Store each different tile/color as grids
	std::map<int32_t, spot_t*> maps;
	int32_t shieldid = current_item_id(itype_shield);
	bool refl = shieldid > -1 && (itemsbuf[shieldid].misc2 & shLIGHTBEAM);
	bool block = !refl && shieldid > -1 && (itemsbuf[shieldid].misc1 & shLIGHTBEAM);
	int32_t linkpos = COMBOPOS(x.getInt()+8,y.getInt()+8);
	memset(istrig, 0, sizeof(istrig));
	clear_bitmap(lightbeam_bmp);
	
	for(size_t pos = 0; pos < 176; ++pos)
	{
		typeMap[pos] = 0;
		for(int32_t lyr = 6; lyr > -1; --lyr)
		{
			newcombo const* cmb = &combobuf[FFCore.tempScreens[lyr]->data[pos]];
			switch(cmb->type)
			{
				case cMIRROR: case cMIRRORSLASH: case cMIRRORBACKSLASH:
				case cMAGICPRISM: case cMAGICPRISM4:
				case cBLOCKALL:
					typeMap[pos] = cmb->type;
					break;
				case cGLASS:
					typeMap[pos] = 0;
					break;
				default:
				{
					if(lyr < 3 && (cmb->walk & 0xF))
					{
						typeMap[pos] = SPTYPE_SOLID;
					}
					continue; //next layer
				}
			}
			break; //hit a combo type
		}
	}
	if(unsigned(linkpos) < 176)
	{
		switch(typeMap[linkpos])
		{
			case SPTYPE_SOLID: case cBLOCKALL:
				linkpos = -1; //Blocked from hitting player
		}
	}
	
	for(size_t layer = 0; layer < 7; ++layer)
	{
		mapscr* curlayer = FFCore.tempScreens[layer];
		for(size_t pos = 0; pos < 176; ++pos)
		{
			//For each spotlight combo on each layer...
			newcombo const& cmb = combobuf[curlayer->data[pos]];
			if(cmb.type == cSPOTLIGHT)
			{
				//Positive ID is a tile, negative is a color trio. 0 is nil in either case.
				int32_t id = (cmb.usrflags&cflag1)
					? std::max(0,cmb.attributes[0]/10000)|(cmb.attribytes[1]%12)<<24
					: -((cmb.attribytes[3]<<16)|(cmb.attribytes[2]<<8)|(cmb.attribytes[1]));
				if(!id) continue;
				// zprint2("ID = %ld\n", id);
				//Get the grid array for this tile/color
				spot_t* grid;
				if(maps[id])
					grid = maps[id];
				else
				{
					grid = new spot_t[176];
					memset(grid, 0, sizeof(spot_t)*176);
					maps[id] = grid;
				}
				byte spotdir = cmb.attribytes[0];
				int32_t curpos = pos;
				if(spotdir > 3)
				{
					grid[curpos] |= SP_VISITED;
					istrig[curpos] |= cmb.attribytes[4] ? (1 << (cmb.attribytes[4]-1)) : ~0;
				}
				if(refl && curpos == linkpos)
				{
					spotdir = dir;
				}
				handleBeam(grid, 0, spotdir, curpos, cmb.attribytes[4], block, refl);
			}
		}
	}
	
	//Draw visuals
	for(auto it = maps.begin(); it != maps.end();)
	{
		int32_t id = it->first;
		spot_t* grid = it->second;
		//
		enum {t_gr, t_up, t_down, t_left, t_right, t_uleft, t_uright, t_dleft, t_dright, t_vert, t_horz, t_notup, t_notdown, t_notleft, t_notright, t_all, t_max };
		int32_t tile = (id&0xFFFFFF);
		int32_t cs = (id>>24);
		byte c_inner = ((-id)&0x0000FF);
		byte c_middle = ((-id)&0x00FF00)>>8;
		byte c_outter = ((-id)&0xFF0000)>>16;
		//{ Setup color bitmap
		BITMAP* cbmp = NULL;
		if(id < 0)
		{
			//zprint2("Creating with colors: %02X,%02X,%02X\n", c_inner, c_middle, c_outter);
			cbmp = create_bitmap_ex(8, 16*t_max, 16);
			clear_bitmap(cbmp);
			for(size_t q = 1; q < t_max; ++q)
			{
				circlefill(cbmp, 16*q+8, 8, 3, c_outter);
				circlefill(cbmp, 16*q+7, 8, 3, c_outter);
				circlefill(cbmp, 16*q+8, 7, 3, c_outter);
				circlefill(cbmp, 16*q+7, 7, 3, c_outter);
				circlefill(cbmp, 16*q+8, 8, 1, c_middle);
				circlefill(cbmp, 16*q+7, 8, 1, c_middle);
				circlefill(cbmp, 16*q+8, 7, 1, c_middle);
				circlefill(cbmp, 16*q+7, 7, 1, c_middle);
				circlefill(cbmp, 16*q+8, 8, 0, c_inner);
				circlefill(cbmp, 16*q+7, 8, 0, c_inner);
				circlefill(cbmp, 16*q+8, 7, 0, c_inner);
				circlefill(cbmp, 16*q+7, 7, 0, c_inner);
			}
			//t_gr
			circlefill(cbmp, 16*t_gr+8, 8, 7, c_outter);
			circlefill(cbmp, 16*t_gr+7, 8, 7, c_outter);
			circlefill(cbmp, 16*t_gr+8, 7, 7, c_outter);
			circlefill(cbmp, 16*t_gr+7, 7, 7, c_outter);
			circlefill(cbmp, 16*t_gr+8, 8, 5, c_middle);
			circlefill(cbmp, 16*t_gr+7, 8, 5, c_middle);
			circlefill(cbmp, 16*t_gr+8, 7, 5, c_middle);
			circlefill(cbmp, 16*t_gr+7, 7, 5, c_middle);
			circlefill(cbmp, 16*t_gr+8, 8, 3, c_inner);
			circlefill(cbmp, 16*t_gr+7, 8, 3, c_inner);
			circlefill(cbmp, 16*t_gr+8, 7, 3, c_inner);
			circlefill(cbmp, 16*t_gr+7, 7, 3, c_inner);
			//t_up
			rectfill(cbmp, 16*t_up+4, 0, 16*t_up+11, 7, c_outter);
			rectfill(cbmp, 16*t_up+6, 0, 16*t_up+9, 7, c_middle);
			rectfill(cbmp, 16*t_up+7, 0, 16*t_up+8, 7, c_inner);
			//t_down
			rectfill(cbmp, 16*t_down+4, 8, 16*t_down+11, 15, c_outter);
			rectfill(cbmp, 16*t_down+6, 8, 16*t_down+9, 15, c_middle);
			rectfill(cbmp, 16*t_down+7, 8, 16*t_down+8, 15, c_inner);
			//t_left
			rectfill(cbmp, 16*t_left, 4, 16*t_left+7, 11, c_outter);
			rectfill(cbmp, 16*t_left, 6, 16*t_left+7, 9, c_middle);
			rectfill(cbmp, 16*t_left, 7, 16*t_left+7, 8, c_inner);
			//t_right
			rectfill(cbmp, 16*t_right+8, 4, 16*t_right+15, 11, c_outter);
			rectfill(cbmp, 16*t_right+8, 6, 16*t_right+15, 9, c_middle);
			rectfill(cbmp, 16*t_right+8, 7, 16*t_right+15, 8, c_inner);
			//t_uleft
			rectfill(cbmp, 16*t_uleft+4, 0, 16*t_uleft+11, 7, c_outter);
			rectfill(cbmp, 16*t_uleft, 4, 16*t_uleft+7, 11, c_outter);
			rectfill(cbmp, 16*t_uleft, 6, 16*t_uleft+7, 9, c_middle);
			rectfill(cbmp, 16*t_uleft+6, 0, 16*t_uleft+9, 7, c_middle);
			rectfill(cbmp, 16*t_uleft+7, 0, 16*t_uleft+8, 7, c_inner);
			rectfill(cbmp, 16*t_uleft, 7, 16*t_uleft+7, 8, c_inner);
			//t_uright
			rectfill(cbmp, 16*t_uright+4, 0, 16*t_uright+11, 7, c_outter);
			rectfill(cbmp, 16*t_uright+8, 4, 16*t_uright+15, 11, c_outter);
			rectfill(cbmp, 16*t_uright+8, 6, 16*t_uright+15, 9, c_middle);
			rectfill(cbmp, 16*t_uright+6, 0, 16*t_uright+9, 7, c_middle);
			rectfill(cbmp, 16*t_uright+7, 0, 16*t_uright+8, 7, c_inner);
			rectfill(cbmp, 16*t_uright+8, 7, 16*t_uright+15, 8, c_inner);
			//t_dleft
			rectfill(cbmp, 16*t_dleft+4, 8, 16*t_dleft+11, 15, c_outter);
			rectfill(cbmp, 16*t_dleft, 4, 16*t_dleft+7, 11, c_outter);
			rectfill(cbmp, 16*t_dleft, 6, 16*t_dleft+7, 9, c_middle);
			rectfill(cbmp, 16*t_dleft+6, 8, 16*t_dleft+9, 15, c_middle);
			rectfill(cbmp, 16*t_dleft+7, 8, 16*t_dleft+8, 15, c_inner);
			rectfill(cbmp, 16*t_dleft, 7, 16*t_dleft+7, 8, c_inner);
			//t_dright
			rectfill(cbmp, 16*t_dright+4, 8, 16*t_dright+11, 15, c_outter);
			rectfill(cbmp, 16*t_dright+8, 4, 16*t_dright+15, 11, c_outter);
			rectfill(cbmp, 16*t_dright+8, 6, 16*t_dright+15, 9, c_middle);
			rectfill(cbmp, 16*t_dright+6, 8, 16*t_dright+9, 15, c_middle);
			rectfill(cbmp, 16*t_dright+7, 8, 16*t_dright+8, 15, c_inner);
			rectfill(cbmp, 16*t_dright+8, 7, 16*t_dright+15, 8, c_inner);
			//t_vert
			rectfill(cbmp, 16*t_vert+4, 0, 16*t_vert+11, 15, c_outter);
			rectfill(cbmp, 16*t_vert+6, 0, 16*t_vert+9, 15, c_middle);
			rectfill(cbmp, 16*t_vert+7, 0, 16*t_vert+8, 15, c_inner);
			//t_horz
			rectfill(cbmp, 16*t_horz, 4, 16*t_horz+15, 11, c_outter);
			rectfill(cbmp, 16*t_horz, 6, 16*t_horz+15, 9, c_middle);
			rectfill(cbmp, 16*t_horz, 7, 16*t_horz+15, 8, c_inner);
			//t_notup
			rectfill(cbmp, 16*t_notup, 4, 16*t_notup+15, 11, c_outter);
			rectfill(cbmp, 16*t_notup+4, 8, 16*t_notup+11, 15, c_outter);
			rectfill(cbmp, 16*t_notup+6, 8, 16*t_notup+9, 15, c_middle);
			rectfill(cbmp, 16*t_notup, 6, 16*t_notup+15, 9, c_middle);
			rectfill(cbmp, 16*t_notup, 7, 16*t_notup+15, 8, c_inner);
			rectfill(cbmp, 16*t_notup+7, 8, 16*t_notup+8, 15, c_inner);
			//t_notdown
			rectfill(cbmp, 16*t_notdown, 4, 16*t_notdown+15, 11, c_outter);
			rectfill(cbmp, 16*t_notdown+4, 0, 16*t_notdown+11, 7, c_outter);
			rectfill(cbmp, 16*t_notdown+6, 0, 16*t_notdown+9, 7, c_middle);
			rectfill(cbmp, 16*t_notdown, 6, 16*t_notdown+15, 9, c_middle);
			rectfill(cbmp, 16*t_notdown, 7, 16*t_notdown+15, 8, c_inner);
			rectfill(cbmp, 16*t_notdown+7, 0, 16*t_notdown+8, 7, c_inner);
			//t_notleft
			rectfill(cbmp, 16*t_notleft+4, 0, 16*t_notleft+11, 15, c_outter);
			rectfill(cbmp, 16*t_notleft+8, 4, 16*t_notleft+15, 11, c_outter);
			rectfill(cbmp, 16*t_notleft+8, 6, 16*t_notleft+15, 9, c_middle);
			rectfill(cbmp, 16*t_notleft+6, 0, 16*t_notleft+9, 15, c_middle);
			rectfill(cbmp, 16*t_notleft+7, 0, 16*t_notleft+8, 15, c_inner);
			rectfill(cbmp, 16*t_notleft+8, 7, 16*t_notleft+15, 8, c_inner);
			//t_notright
			rectfill(cbmp, 16*t_notright+4, 0, 16*t_notright+11, 15, c_outter);
			rectfill(cbmp, 16*t_notright, 4, 16*t_notright+7, 11, c_outter);
			rectfill(cbmp, 16*t_notright, 6, 16*t_notright+7, 9, c_middle);
			rectfill(cbmp, 16*t_notright+6, 0, 16*t_notright+9, 15, c_middle);
			rectfill(cbmp, 16*t_notright+7, 0, 16*t_notright+8, 15, c_inner);
			rectfill(cbmp, 16*t_notright, 7, 16*t_notright+7, 8, c_inner);
			//t_all
			rectfill(cbmp, 16*t_all+4, 0, 16*t_all+11, 15, c_outter);
			rectfill(cbmp, 16*t_all, 4, 16*t_all+15, 11, c_outter);
			rectfill(cbmp, 16*t_all, 6, 16*t_all+15, 9, c_middle);
			rectfill(cbmp, 16*t_all+6, 0, 16*t_all+9, 15, c_middle);
			rectfill(cbmp, 16*t_all+7, 0, 16*t_all+8, 15, c_inner);
			rectfill(cbmp, 16*t_all, 7, 16*t_all+15, 8, c_inner);
		}
		//}
		//
		for(size_t pos = 0; pos < 176; ++pos)
		{
			int32_t offs = -1;
			switch(grid[pos]>>1)
			{
				case 0: default:
					if(grid[pos])
					{
						offs = t_gr;
						break;
					}
					continue; //no draw this pos
				case 0b0001:
					offs = t_up;
					break;
				case 0b0010:
					offs = t_down;
					break;
				case 0b0100:
					offs = t_left;
					break;
				case 0b1000:
					offs = t_right;
					break;
				case 0b0011:
					offs = t_vert;
					break;
				case 0b1100:
					offs = t_horz;
					break;
				case 0b0101:
					offs = t_uleft;
					break;
				case 0b1001:
					offs = t_uright;
					break;
				case 0b0110:
					offs = t_dleft;
					break;
				case 0b1010:
					offs = t_dright;
					break;
				case 0b1110:
					offs = t_notup;
					break;
				case 0b1101:
					offs = t_notdown;
					break;
				case 0b1011:
					offs = t_notleft;
					break;
				case 0b0111:
					offs = t_notright;
					break;
				case 0b1111:
					offs = t_all;
					break;
			}
			if(id > 0) //tile
			{
				//Draw 'tile' at 'pos'
				overtile16(lightbeam_bmp, tile+offs, COMBOX(pos), COMBOY(pos), cs, 0);
			}
			else //colors
			{
				masked_blit(cbmp, lightbeam_bmp, offs*16, 0, COMBOX(pos), COMBOY(pos), 16, 16);
			}
		}
		//
		if(cbmp) destroy_bitmap(cbmp);
		delete[] it->second;
		it = maps.erase(it);
	}
	//Check triggers
	bool hastrigs = false, istrigged = true;
	bool alltrig = getmapflag(mLIGHTBEAM);
	for(size_t layer = 0; layer < 7; ++layer)
	{
		mapscr* curlayer = FFCore.tempScreens[layer];
		for(size_t pos = 0; pos < 176; ++pos)
		{
			newcombo const* cmb = &combobuf[curlayer->data[pos]];
			if(cmb->type == cLIGHTTARGET)
			{
				int32_t trigflag = cmb->attribytes[4] ? (1 << (cmb->attribytes[4]-1)) : ~0;
				hastrigs = true;
				bool trigged = (istrig[pos]&trigflag);
				if(cmb->usrflags&cflag2) //Invert
					trigged = !trigged;
				if(cmb->usrflags&cflag1) //Solved Version
				{
					if(!(alltrig || trigged)) //Revert
					{
						curlayer->data[pos] -= 1;
						istrigged = false;
					}
				}
				else //Unsolved version
				{
					if(alltrig || trigged) //Light
						curlayer->data[pos] += 1;
					else istrigged = false;
				}
			}
		}
	}
	if(hastrigs && istrigged && !alltrig)
	{
		hidden_entrance(0,true,false,-7);
		sfx(tmpscr->secretsfx);
		if(!(tmpscr->flags5&fTEMPSECRETS))
		{
			setmapflag(mSECRET);
			setmapflag(mLIGHTBEAM);
		}
	}
}

void LinkClass::checktouchblk()
{
    if(toogam) return;
    
    if(!pushing)
        return;
        
    int32_t tdir = dir; //Bad hack #2. _L_, your welcome to fix this properly. ;)
    
    if(charging > 0 || spins > 0) //if not I probably will at some point...
    {
        if(Up()&&Left())tdir = (charging%2)*2;
        else if(Up()&&Right())tdir = (charging%2)*3;
        else if(Down()&&Left())tdir = 1+(charging%2)*1;
        else if(Down()&&Right())tdir = 1+(charging%2)*2;
        else
        {
            if(Up())tdir=0;
            else if(Down())tdir=1;
            else if(Left())tdir=2;
            else if(Right())tdir=3;
        }
    }
    
    int32_t tx=0,ty=-1;
    
    switch(tdir)
    {
    case up:
        if(touchcombo(x,y+(bigHitbox?0:7)))
        {
            tx=x;
            ty=y+(bigHitbox?0:7);
        }
        else if(touchcombo(x+8,y+(bigHitbox?0:7)))
        {
            tx=x+8;
            ty=y+(bigHitbox?0:7);
        }
        
        break;
        
    case down:
        if(touchcombo(x,y+16))
        {
            tx=x;
            ty=y+16;
        }
        else if(touchcombo(x+8,y+16))
        {
            tx=x+8;
            ty=y+16;
        }
        
        break;
        
    case left:
        if(touchcombo(x-1,y+15))
        {
            tx=x-1;
            ty=y+15;
        }
        
        break;
        
    case right:
        if(touchcombo(x+16,y+15))
        {
            tx=x+16;
            ty=y+15;
        }
        
        break;
    }
    
    if(ty>=0)
    {
        ty&=0xF0;
        tx&=0xF0;
        int32_t di = ty+(tx>>4);
        int32_t gc=0;
        int32_t eclk=-14;
        
        for(int32_t i=0; i<guys.Count(); ++i)
        {
            if(((enemy*)guys.spr(i))->mainguy)
            {
                ++gc;
            }
        }
        
        if(di<176 && !guygrid[di] && gc<11)
        {
            if((getAction() != hopping || isSideViewLink()))
            {
                guygrid[di]=61; //Note: not 60.
		//zprint2("oof: %d\n", di);
                int32_t id2=0; 
                int32_t cid = MAPCOMBO(tx,ty);
		int32_t cpos = COMBOPOS(tx,ty);
                switch(combobuf[MAPCOMBO(tx,ty)].type)
                {
                case cARMOS: //id2=eARMOS; break;
			
			if ( combobuf[cid].usrflags&cflag1 ) //custom enemy ids
			{
				int32_t r = (combobuf[cid].usrflags&cflag2) ? zc_oldrand()%2 : 0;
				id2 = combobuf[cid].attribytes[0+r];
				//if(guysbuf[id2].family==eeWALK)
				//{
				//	eclk=0;
				//}
				
				//! To-do Adjust for larger enemies, but we need it to be directional. 
				int32_t ypos = 0; int32_t xpos = 0;
				int32_t chy = 0; int32_t chx = 0;
				//nmew idea = check while the upper-left corner combo is armos
				///move up one and check if it is armos, check the next, and stop as soon as that is not armos
				///then do the same going left
				
				int32_t searching = 1;
				int32_t armosxsz = 1;
				int32_t armosysz = 1;
				switch(guysbuf[id2].family)
				{
					case eeGHOMA:
						armosxsz = 3;
						break;
					case eeAQUA: //jesus christ I'm not considering the logistics of manhandlas and gleeoks
					case eeDIG:
						armosxsz = 2;
						armosysz = 2;
						break;
					default:
						break;
				}
				if ((guysbuf[id2].SIZEflags&guyflagOVERRIDE_TILE_HEIGHT) != 0) armosxsz = guysbuf[id2].txsz;
				if ((guysbuf[id2].SIZEflags&guyflagOVERRIDE_TILE_WIDTH) != 0) armosysz = guysbuf[id2].tysz;
				
				if ( ( armosxsz > 1 ) || ( armosysz > 1 ) )
				{
					switch(dir)
					{
						case up: //touched armos from below
						{
							while(searching == 1) //find the top edge of an armos block
							{
								chy += 16;
								if ( cpos - chy < 0 ) break; //don't go out of bounds
								if ( ( combobuf[(tmpscr->data[cpos-chy])].type == cARMOS ) ) 
								{
									ypos -=16;
								}
								else searching = 2;
							}
							while(searching == 2) //find the left edge of an armos block
							{
								if ( (cpos % 16) == 0 || cpos == 0 ) break; //don't wrap rows
								++chx;
								if ( cpos - chx < 0 ) break; //don't go out of bounds
								if ( ( combobuf[(tmpscr->data[cpos-chx])].type == cARMOS ) ) 
								{
									xpos -=16;
								}
								else searching = 3;
							}
							
							break;
						}
						case down: //touched armos from above
						{
							//zprint("touched armos from above\n");
							//zprint("cpos: %d\n", cpos);
							//int32_t tx2 = (int32_t)x; //COMBOX(COMBOPOS(tx,ty));
							//int32_t ty2 = (int32_t)y+16; //COMBOY(COMBOPOS(tx,ty));
							//tx2 = GridX(tx2);
							//ty2 = GridY(ty2);
							while(searching == 1) //find the left edge of an armos block
							{
								//zprint("searching\n");
								if ( (cpos % 16) == 0 || cpos == 0 ) break; //don't wrap rows
								++chx;
								
								
								//zprint("chx: %d\n", chx);
								//zprint("tx2: %d\n", tx2);
								//zprint("ty2: %d\n", ty2);
								//zprint("MAPCOMBO(tx2,ty2): %d\n",MAPCOMBO(tx2,ty2));
								//zprint("MAPCOMBO(tx2-chx,ty2): %d\n",MAPCOMBO(GridX(tx2-chx),ty2));
								if ( ( combobuf[(tmpscr->data[cpos-chx])].type == cARMOS ) ) 
								{
									//zprint("found match\n");
									xpos -=16;
								}
								else searching = 3;
							}
							//zprint("xpos is: %d\n", xpos);
						}
						case left: //touched right edge of armos
						{
							while(searching == 1) //find the top edge of an armos block
							{
								chy += 16;
								if ( cpos - chy < 0 ) break; //don't go out of bounds
								if ( ( combobuf[(tmpscr->data[cpos-chy])].type == cARMOS ) ) 
								{
									ypos -=16;
								}
								else searching = 2;
							}
							while(searching == 2) //find the left edge of an armos block
							{
								if ( (cpos % 16) == 0 || cpos == 0 ) break; //don't wrap rows
								++chx;
								if ( cpos - chx < 0 ) break; //don't go out of bounds
								if ( ( combobuf[(tmpscr->data[cpos-chx])].type == cARMOS ) ) 
								{
									xpos -=16;
								}
								else searching = 3;
							}
							break;
						}
							
						case right: //touched left edge of armos
						{
							//zprint("touched armos on left\n");
							while(searching == 1) //find the top edge of an armos block
							{
								chy += 16;
								if ( cpos - chy < 0 ) break; //don't go out of bounds
								if ( ( combobuf[(tmpscr->data[cpos-chy])].type == cARMOS ) ) 
								{
									//zprint("found match\n");
									ypos -=16;
								}
								else searching = 2;
							}
							break;
						}
					
						
					}
				}
				//if ( guysbuf[id2].txsz > 1 ) xpos -= guysbuf[id2].txsz*16;
				//if ( guysbuf[id2].tysz > 1 ) ypos -= guysbuf[id2].tysz*16;
				int32_t xpos2 = tx+xpos;
				int32_t ypos2 = ty+ypos;
				int32_t id3 = COMBOPOS(xpos2, ypos2);
				for (int32_t n = 0; n < armosysz && id3 < 176; n++)
				{
					
					for (int32_t m = 0; m < armosxsz && id3 < 176; m++) 
					{
						guygrid[id3+m]=61;
						//zprint2("oof: %d\n", id3+m);
					}
					id3+=16;
				}
				if (guysbuf[id2].family == eeGHOMA) 
				{
					if ( ( combobuf[(tmpscr->data[cpos-chx+1])].type == cARMOS ) ) xpos += 16;
				}
				addenemy(tx+xpos,ty+1+ypos,id2,0);
				((enemy*)guys.spr(guys.Count()-1))->did_armos=false;
				((enemy*)guys.spr(guys.Count()-1))->fading=fade_flicker;
				((enemy*)guys.spr(guys.Count()-1))->flags2 |= cmbflag_armos;
				//((enemy*)guys.spr(guys.Count()-1))->dir = down;
				//((enemy*)guys.spr(guys.Count()-1))->xofs = 0;
				//((enemy*)guys.spr(guys.Count()-1))->yofs = 0;
				/*
				int32_t cd = (tx>>4)+ty;
				int32_t f = MAPFLAG(tx,ty);
				int32_t f2 = MAPCOMBOFLAG(tx,ty);
				tmpscr->data[cd] = tmpscr->undercombo;
				tmpscr->cset[cd] = tmpscr->undercset;
				tmpscr->sflag[cd] = 0;
				    
				if(f == mfARMOS_SECRET || f2 == mfARMOS_SECRET)
				{
					tmpscr->data[cd] = tmpscr->secretcombo[sSTAIRS];
					tmpscr->cset[cd] = tmpscr->secretcset[sSTAIRS];
					tmpscr->sflag[cd]=tmpscr->secretflag[sSTAIRS];
					sfx(tmpscr->secretsfx);
				}
				    
				if(f == mfARMOS_ITEM || f2 == mfARMOS_ITEM)
				{
					if(!getmapflag())
					{
					    additem(tx,ty,tmpscr->catchall, (ipONETIME2 + ipBIGRANGE) | ((tmpscr->flags3&fHOLDITEM) ? ipHOLDUP : 0));
					    sfx(tmpscr->secretsfx);
					}
				}
				    
				putcombo(scrollbuf,tx,ty,tmpscr->data[cd],tmpscr->cset[cd]);
				return;
				*/
				return;
			}
			
                    for(int32_t i=0; i<eMAXGUYS; i++)
                    {
                        if(guysbuf[i].flags2&cmbflag_armos)
                        {
                            id2=i;
                            
                            // This is mostly for backwards-compatability
                            if(guysbuf[i].family==eeWALK && guysbuf[i].misc9==e9tARMOS)
                            {
                                eclk=0;
                            }
                            
                            break;
                        }
                    }
                    
                    break;
                    
                case cBSGRAVE:
                    tmpscr->data[di]++;
                    
                    //fall through
                case cGRAVE:
                    for(int32_t i=0; i<eMAXGUYS; i++)
                    {
                        if(guysbuf[i].flags2&cmbflag_ghini)
                        {
                            id2=i;
                            eclk=0; // This is mostly for backwards-compatability
                            break;
                        }
                    }
                    
                    //id2=eGHINI2;
                    break;
                }
                
                addenemy(tx,ty+3,id2,eclk);
                ((enemy*)guys.spr(guys.Count()-1))->did_armos=false;
            }
        }
    }
}

int32_t LinkClass::nextcombo(int32_t cx, int32_t cy, int32_t cdir)
{
    switch(cdir)
    {
    case up:
        cy-=16;
        break;
        
    case down:
        cy+=16;
        break;
        
    case left:
        cx-=16;
        break;
        
    case right:
        cx+=16;
        break;
    }
    
    // off the screen
    if(cx<0 || cy<0 || cx>255 || cy>175)
    {
        int32_t ns = nextscr(cdir);
        
        if(ns==0xFFFF) return 0;
        
        // want actual screen index, not game->maps[] index
        ns = (ns&127) + (ns>>7)*MAPSCRS;
        
        switch(cdir)
        {
        case up:
            cy=160;
            break;
            
        case down:
            cy=0;
            break;
            
        case left:
            cx=240;
            break;
            
        case right:
            cx=0;
            break;
        }
        
        // from MAPCOMBO()
        int32_t cmb = (cy&0xF0)+(cx>>4);
        
        if(cmb>175)
            return 0;
            
        return TheMaps[ns].data[cmb];                           // entire combo code
    }
    
    return MAPCOMBO(cx,cy);
}

int32_t LinkClass::nextflag(int32_t cx, int32_t cy, int32_t cdir, bool comboflag)
{
    switch(cdir)
    {
    case up:
        cy-=16;
        break;
        
    case down:
        cy+=16;
        break;
        
    case left:
        cx-=16;
        break;
        
    case right:
        cx+=16;
        break;
    }
    
    // off the screen
    if(cx<0 || cy<0 || cx>255 || cy>175)
    {
        int32_t ns = nextscr(cdir);
        
        if(ns==0xFFFF) return 0;
        
        // want actual screen index, not game->maps[] index
        ns = (ns&127) + (ns>>7)*MAPSCRS;
        
        switch(cdir)
        {
        case up:
            cy=160;
            break;
            
        case down:
            cy=0;
            break;
            
        case left:
            cx=240;
            break;
            
        case right:
            cx=0;
            break;
        }
        
        // from MAPCOMBO()
        int32_t cmb = (cy&0xF0)+(cx>>4);
        
        if(cmb>175)
            return 0;
            
        if(!comboflag)
        {
            return TheMaps[ns].sflag[cmb];                          // flag
        }
        else
        {
            return combobuf[TheMaps[ns].data[cmb]].flag;                          // flag
        }
    }
    
    if(comboflag)
    {
        return MAPCOMBOFLAG(cx,cy);
    }
    
    return MAPFLAG(cx,cy);
}

bool did_secret;

void LinkClass::checkspecial()
{
    checktouchblk();
    
    bool hasmainguy = hasMainGuy();                           // calculate it once
    
    if(!(loaded_enemies && !hasmainguy))
        did_secret=false;
    else
    {
        // after beating enemies
        
        // if room has traps, guys don't come back
        for(int32_t i=0; i<eMAXGUYS; i++)
        {
            if(guysbuf[i].family==eeTRAP&&guysbuf[i].misc2)
                if(guys.idCount(i) && !getmapflag(mTMPNORET))
                    setmapflag(mTMPNORET);
        }
        
        // item
        if(hasitem&(4|2|1))
        {
            int32_t Item=tmpscr->item;
            
            //if(getmapflag())
            //  Item=0;
            if((!getmapflag(mITEM) || (tmpscr->flags9&fITEMRETURN)) && (tmpscr->hasitem != 0))
            {
                if(hasitem==1)
                    sfx(WAV_CLEARED);
                    
                items.add(new item((zfix)tmpscr->itemx,
                                   (tmpscr->flags7&fITEMFALLS && isSideViewLink()) ? (zfix)-170 : (zfix)tmpscr->itemy+1,
                                   (tmpscr->flags7&fITEMFALLS && !isSideViewLink()) ? (zfix)170 : (zfix)0,
                                   Item,ipONETIME|ipBIGRANGE|((itemsbuf[Item].family==itype_triforcepiece ||
                                           (tmpscr->flags3&fHOLDITEM)) ? ipHOLDUP : 0) | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0),0));
            }
            
            hasitem &= ~ (4|2|1);
        }
        
        // clear enemies and open secret
        if(!did_secret && (tmpscr->flags2&fCLEARSECRET))
        {
			bool only16_31 = get_bit(quest_rules,qr_ENEMIES_SECRET_ONLY_16_31)?true:false;
            hidden_entrance(0,true,only16_31,-2);
            
            if(tmpscr->flags4&fENEMYSCRTPERM && canPermSecret())
            {
                if(!(tmpscr->flags5&fTEMPSECRETS)) setmapflag(mSECRET);
            }
            
            sfx(tmpscr->secretsfx);
            did_secret=true;
        }
    }
    
    // doors
    for(int32_t i=0; i<4; i++)
        if(tmpscr->door[i]==dSHUTTER)
        {
            if(opendoors==0 && loaded_enemies)
            {
                if(!(tmpscr->flags&fSHUTTERS) && !hasmainguy)
                    opendoors=12;
            }
            else if(opendoors<0)
                ++opendoors;
            else if((--opendoors)==0)
                openshutters();
                
            break;
        }
        
    // set boss flag when boss is gone
    if(loaded_enemies && tmpscr->enemyflags&efBOSS && !hasmainguy)
    {
        game->lvlitems[dlevel]|=liBOSS;
        stop_sfx(tmpscr->bosssfx);
    }
    
    if(getmapflag(mCHEST))              // if special stuff done before
    {
        remove_chests((currscr>=128)?1:0);
    }
    
    if(getmapflag(mLOCKEDCHEST))              // if special stuff done before
    {
        remove_lockedchests((currscr>=128)?1:0);
    }
    
    if(getmapflag(mBOSSCHEST))              // if special stuff done before
    {
        remove_bosschests((currscr>=128)?1:0);
    }
	
	if((hasitem&8) && triggered_screen_secrets)
	{
		int32_t Item=tmpscr->item;
		
		if((!getmapflag(mITEM) || (tmpscr->flags9&fITEMRETURN)) && (tmpscr->hasitem != 0))
		{
			items.add(new item((zfix)tmpscr->itemx,
							   (tmpscr->flags7&fITEMFALLS && isSideViewLink()) ? (zfix)-170 : (zfix)tmpscr->itemy+1,
							   (tmpscr->flags7&fITEMFALLS && !isSideViewLink()) ? (zfix)170 : (zfix)0,
							   Item,ipONETIME|ipBIGRANGE|((itemsbuf[Item].family==itype_triforcepiece ||
									   (tmpscr->flags3&fHOLDITEM)) ? ipHOLDUP : 0) | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0),0));
		}
		
		hasitem &= ~8;
	}
}

void LinkClass::checkspecial2(int32_t *ls)
{
	if(get_bit(quest_rules,qr_OLDSTYLEWARP) && !(diagonalMovement||NO_GRIDLOCK))
	{
		if(y.getInt()&7)
			return;
			
		if(x.getInt()&7)
			return;
	}
	
	if(toogam) return;
	
	bool didstrig = false;
	
	for(int32_t i=bigHitbox?0:8; i<16; i+=bigHitbox?15:7)
	{
		for(int32_t j=0; j<16; j+=15) for(int32_t k=0; k<2; k++)
		{
			int32_t stype = combobuf[k>0 ? MAPFFCOMBO(x+j,y+i) : MAPCOMBO(x+j,y+i)].type;
		int32_t warpsound = combobuf[k>0 ? MAPFFCOMBO(x+j,y+i) : MAPCOMBO(x+j,y+i)].attribytes[0];
			
			if(stype==cSWARPA)
			{
				if(tmpscr->flags5&fDIRECTSWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir=dir;
				dowarp(0,0,warpsound);
				return;
			}
			
			if(stype==cSWARPB)
			{
				if(tmpscr->flags5&fDIRECTSWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir=dir;
				dowarp(0,1,warpsound);
				return;
			}
			
			if(stype==cSWARPC)
			{
				if(tmpscr->flags5&fDIRECTSWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir=dir;
				dowarp(0,2,warpsound);
				return;
			}
			
			if(stype==cSWARPD)
			{
				if(tmpscr->flags5&fDIRECTSWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir=dir;
				dowarp(0,3,warpsound);
				return;
			}
			
			if(stype==cSWARPR)
			{
				if(tmpscr->flags5&fDIRECTSWARP)
				{
					didpit=true;
					pitx=x;
					pity=y;
				}
				
				sdir=dir;
				dowarp(0,(zc_oldrand()%4),warpsound);
				return;
			}
			
			if((stype==cSTRIGNOFLAG || stype==cSTRIGFLAG) && stepsecret!=MAPCOMBO(x+j,y+i))
			{
				zprint("Step Secs\n");
				if(stype==cSTRIGFLAG && canPermSecret())
				{ 
					if(!didstrig)
					{
						stepsecret = ((int32_t)(y+i)&0xF0)+((int32_t)(x+j)>>4);
						
						if(!(tmpscr->flags5&fTEMPSECRETS))
						{
							setmapflag(mSECRET);
						}
						//int32_t thesfx = combobuf[MAPCOMBO(x+j,y+i)].attribytes[0];
						//zprint("Step Secrets SFX: %d\n", thesfx);
						sfx(warpsound,pan((int32_t)x));
						hidden_entrance(0,true,false); 
						didstrig = true;
					}
				}
				else
				{ 
					if(!didstrig)
					{
						stepsecret = ((int32_t)(y+i)&0xF0)+((int32_t)(x+j)>>4);
						bool only16_31 = get_bit(quest_rules,qr_STEPTEMP_SECRET_ONLY_16_31)?true:false;
						hidden_entrance(0,true,only16_31); 
						didstrig = true;
						//play trigger sound
						//int32_t thesfx = combobuf[MAPCOMBO(x+j,y+i)].attribytes[0];
						//zprint("Step Secrets SFX: %d\n", thesfx);
						//sfx(thesfx,pan((int32_t)x));
						sfx(warpsound,pan((int32_t)x));
					}
				}
			}
		}
	}
	
	// check if he's standing on a warp he just came out of
	// But if the QR is checked, it uses the old logic, cause some quests like Ballad of a Bloodline warp you onto a trigger and this new logic bricks that.
	if (!get_bit(quest_rules,qr_210_WARPRETURN))
	{
		if(((int32_t)y>=warpy-8&&(int32_t)y<=warpy+7)&&warpy!=-1)
		{
			if(((int32_t)x>=warpx-8&&(int32_t)x<=warpx+7)&&warpx!=-1)
			{
				return;
			}
		}
	}
	else
	{
		if((int(y)&0xF8)==warpy)
		{
			if(x==warpx) return;
		}
	}
	
	warpy=255;
	int32_t tx=x;
	int32_t ty=y;
	
	int32_t flag=0;
	int32_t flag2=0;
	int32_t flag3=0;
	int32_t type=0;
	int32_t water=0;
	int32_t index = 0;
	
	//bool gotpit=false;
	
	int32_t x1,x2,y1,y2;
	x1 = tx;
	x2 = tx+15;
	y1 = ty;
	y2 = ty+15;
	
	if((diagonalMovement||NO_GRIDLOCK))
	{
		x1 = tx+4;
		x2 = tx+11;
		y1 = ty+4;
		y2 = ty+11;
	}
	
	int32_t types[4];
	types[0]=types[1]=types[2]=types[3]=-1;
	int32_t cids[4];
	int32_t ffcids[4];
	cids[0]=cids[1]=cids[2]=cids[3]=-1;
	ffcids[0]=ffcids[1]=ffcids[2]=ffcids[3]=-1;
	//
	// First, let's find flag1 (combo flag), flag2 (inherent flag) and flag3 (FFC flag)...
	//
	types[0] = MAPFLAG(x1,y1);
	types[1] = MAPFLAG(x1,y2);
	types[2] = MAPFLAG(x2,y1);
	types[3] = MAPFLAG(x2,y2);
	
	
	//MAPFFCOMBO
	
	
	if(types[0]==types[1]&&types[2]==types[3]&&types[1]==types[2])
		flag = types[0];
		
	// 2.10 compatibility...
	else if(y.getInt()%16==8 && types[0]==types[2] && (types[0]==mfFAIRY || types[0]==mfMAGICFAIRY || types[0]==mfALLFAIRY))
		flag = types[0];
		
		
	types[0] = MAPCOMBOFLAG(x1,y1);
	types[1] = MAPCOMBOFLAG(x1,y2);
	types[2] = MAPCOMBOFLAG(x2,y1);
	types[3] = MAPCOMBOFLAG(x2,y2);
	
	if(types[0]==types[1]&&types[2]==types[3]&&types[1]==types[2])
		flag2 = types[0];
		
	types[0] = MAPFFCOMBOFLAG(x1,y1);
	types[1] = MAPFFCOMBOFLAG(x1,y2);
	types[2] = MAPFFCOMBOFLAG(x2,y1);
	types[3] = MAPFFCOMBOFLAG(x2,y2);
	
	
	//
	
	if(types[0]==types[1]&&types[2]==types[3]&&types[1]==types[2])
		flag3 = types[0];
		
	//
	// Now, let's check for warp combos...
	//
	
	//
	
	cids[0] = MAPCOMBO(x1,y1);
	cids[1] = MAPCOMBO(x1,y2);
	cids[2] = MAPCOMBO(x2,y1);
	cids[3] = MAPCOMBO(x2,y2);
	
	types[0] = COMBOTYPE(x1,y1);
	
	if(MAPFFCOMBO(x1,y1))
	{
		types[0] = FFCOMBOTYPE(x1,y1);
		cids[0] = MAPFFCOMBO(x1,y1);
	}
		
	types[1] = COMBOTYPE(x1,y2);
	
	if(MAPFFCOMBO(x1,y2))
	{
		types[1] = FFCOMBOTYPE(x1,y2);
		cids[1] = MAPFFCOMBO(x1,y2);
	}
		
	types[2] = COMBOTYPE(x2,y1);
	
	if(MAPFFCOMBO(x2,y1))
	{
		types[2] = FFCOMBOTYPE(x2,y1);
		cids[2] = MAPFFCOMBO(x2,y1);
	}
	types[3] = COMBOTYPE(x2,y2);
	
	if(MAPFFCOMBO(x2,y2))
	{
		types[3] = FFCOMBOTYPE(x2,y2);
		cids[3] = MAPFFCOMBO(x2,y2);
	}
	int32_t warpsfx2 = 0;
	// Change B, C and D warps into A, for the comparison below...
	for(int32_t i=0; i<4; i++)
	{
		if(types[i]==cCAVE)
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cCAVEB)
		{
			types[i]=cCAVE;
			index=1;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cCAVEC)
		{
			types[i]=cCAVE;
			index=2;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cCAVED)
		{
			types[i]=cCAVE;
			index=3;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		
		if(types[i]==cPIT) 
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cPITB)
		{
			types[i]=cPIT;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=1;
		}
		else if(types[i]==cPITC)
		{
			types[i]=cPIT;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=2;
		}
		else if(types[i]==cPITD)
		{
			types[i]=cPIT;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=3;
		}
		else if(types[i]==cPITR)
		{
			types[i]=cPIT;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=zc_oldrand()%4;
		}
		
		if(types[i]==cSTAIR)
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cSTAIRB)
		{
			types[i]=cSTAIR;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=1;
		}
		else if(types[i]==cSTAIRC)
		{
			types[i]=cSTAIR;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=2;
		}
		else if(types[i]==cSTAIRD)
		{
			types[i]=cSTAIR;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=3;
		}
		else if(types[i]==cSTAIRR)
		{
			types[i]=cSTAIR;
			index=zc_oldrand()%4;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		
		if(types[i]==cCAVE2)
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cCAVE2B)
		{
			types[i]=cCAVE2;
			index=1;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cCAVE2C)
		{
			types[i]=cCAVE2;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=2;
		}
		else if(types[i]==cCAVE2D)
		{
			types[i]=cCAVE2;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=3;
		}
		
		if(types[i]==cSWIMWARP) 
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cSWIMWARPB)
		{
			types[i]=cSWIMWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=1;
		}
		else if(types[i]==cSWIMWARPC)
		{
			types[i]=cSWIMWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=2;
		}
		else if(types[i]==cSWIMWARPD)
		{
			types[i]=cSWIMWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=3;
		}
		
		if(types[i]==cDIVEWARP) 
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cDIVEWARPB)
		{
			types[i]=cDIVEWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=1;
		}
		else if(types[i]==cDIVEWARPC)
		{
			types[i]=cDIVEWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=2;
		}
		else if(types[i]==cDIVEWARPD)
		{
			types[i]=cDIVEWARP;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
			index=3;
		}
		
		if(types[i]==cSTEP) warpsfx2 = combobuf[cids[i]].attribytes[0];
		else if(types[i]==cSTEPSAME) { types[i]=cSTEP; warpsfx2 = combobuf[cids[i]].attribytes[0];}
		else if(types[i]==cSTEPALL) { types[i]=cSTEP; warpsfx2 = combobuf[cids[i]].attribytes[0]; }
	}
	
	// Special case for step combos; otherwise, they act oddly in some cases
	if((types[0]==types[1]&&types[2]==types[3]&&types[1]==types[2])||(types[1]==cSTEP&&types[3]==cSTEP))
	{
		if(action!=freeze&&action!=sideswimfreeze&&(!msg_active || !get_bit(quest_rules,qr_MSGFREEZE)))
			type = types[1];
	}
	
	//
	// Now, let's check for Save combos...
	//
	x1 = tx+4;
	x2 = tx+11;
	y1 = ty+4;
	y2 = ty+11;
	
	types[0] = COMBOTYPE(x1,y1);
	
	if(MAPFFCOMBO(x1,y1))
		types[0] = FFCOMBOTYPE(x1,y1);
		
	types[1] = COMBOTYPE(x1,y2);
	
	if(MAPFFCOMBO(x1,y2))
		types[1] = FFCOMBOTYPE(x1,y2);
		
	types[2] = COMBOTYPE(x2,y1);
	
	if(MAPFFCOMBO(x2,y1))
		types[2] = FFCOMBOTYPE(x2,y1);
		
	types[3] = COMBOTYPE(x2,y2);
	
	if(MAPFFCOMBO(x2,y2))
		types[3] = FFCOMBOTYPE(x2,y2);
		
	bool setsave=false;
	
	for(int32_t i=0; i<4; i++)
	{
		if(types[i]==cSAVE) setsave=true;
		
		if(types[i]==cSAVE2) setsave=true;
	}
	
	if(setsave && types[0]==types[1]&&types[2]==types[3]&&types[1]==types[2])
		type = types[0];
		
	//
	// Now, let's check for Drowning combos...
	//
	if(get_bit(quest_rules,qr_DROWN))
	{
		y1 = ty+9;
		y2 = ty+15;
		if (get_bit(quest_rules, qr_SMARTER_WATER))
		{
			if (iswaterex(0, currmap, currscr, -1, x1, y1, true, false) &&
			iswaterex(0, currmap, currscr, -1, x1, y2, true, false) &&
			iswaterex(0, currmap, currscr, -1, x2, y1, true, false) &&
			iswaterex(0, currmap, currscr, -1, x2, y2, true, false)) water = iswaterex(0, currmap, currscr, -1, (x2+x1)/2,(y2+y1)/2, true, false);
		}
		else
		{
			types[0] = COMBOTYPE(x1,y1);
			
			if(MAPFFCOMBO(x1,y1))
				types[0] = FFCOMBOTYPE(x1,y1);
				
			types[1] = COMBOTYPE(x1,y2);
			
			if(MAPFFCOMBO(x1,y2))
				types[1] = FFCOMBOTYPE(x1,y2);
				
			types[2] = COMBOTYPE(x2,y1);
			
			if(MAPFFCOMBO(x2,y1))
				types[2] = FFCOMBOTYPE(x2,y1);
				
			types[3] = COMBOTYPE(x2,y2);
			
			if(MAPFFCOMBO(x2,y2))
				types[3] = FFCOMBOTYPE(x2,y2);
				
			int32_t typec = COMBOTYPE((x2+x1)/2,(y2+y1)/2);
			if(MAPFFCOMBO((x2+x1)/2,(y2+y1)/2))
				typec = FFCOMBOTYPE((x2+x1)/2,(y2+y1)/2);
				
			if(combo_class_buf[types[0]].water && combo_class_buf[types[1]].water &&
					combo_class_buf[types[2]].water && combo_class_buf[types[3]].water && combo_class_buf[typec].water)
				water = typec;
		}
	}
	
	
	// Pits have a bigger 'hitbox' than stairs...
	x1 = tx+7;
	x2 = tx+8;
	y1 = ty+7+(bigHitbox?0:4);
	y2 = ty+8+(bigHitbox?0:4);
	
	types[0] = COMBOTYPE(x1,y1);
	cids[0] = MAPCOMBO(x1,y1);
	
	if(MAPFFCOMBO(x1,y1))
	{
		types[0] = FFCOMBOTYPE(x1,y1);
		cids[0] = MAPFFCOMBO(x1,y1);
	}
		
	types[1] = COMBOTYPE(x1,y2);
	cids[1] = MAPCOMBO(x1,y2);
	
	if(MAPFFCOMBO(x1,y2))
	{
		types[1] = FFCOMBOTYPE(x1,y2);
		cids[1] = MAPFFCOMBO(x1,y2);
	}
	types[2] = COMBOTYPE(x2,y1);
	cids[2] = MAPCOMBO(x2,y1);
	
	if(MAPFFCOMBO(x2,y1))
	{
		types[2] = FFCOMBOTYPE(x2,y1);
		cids[2] = MAPFFCOMBO(x2,y1);
	}
		
	types[3] = COMBOTYPE(x2,y2);
	cids[3] = MAPCOMBO(x2,y2);
	
	if(MAPFFCOMBO(x2,y2))
	{
		types[3] = FFCOMBOTYPE(x2,y2);
		cids[3] = MAPFFCOMBO(x2,y2);
	}
		
	for(int32_t i=0; i<4; i++)
	{
		if(types[i]==cPIT) 
		{
			index=0;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cPITB)
		{
			types[i]=cPIT;
			index=1;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cPITC)
		{
			types[i]=cPIT;
			index=2;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
		else if(types[i]==cPITD)
		{
			types[i]=cPIT;
			index=3;
			warpsfx2 = combobuf[cids[i]].attribytes[0];
		}
	}
	
	if(types[0]==cPIT||types[1]==cPIT||types[2]==cPIT||types[3]==cPIT)
		if(action!=freeze&&action!=sideswimfreeze&& (!msg_active || !get_bit(quest_rules,qr_MSGFREEZE)))
			type=cPIT;
			
	//
	// Time to act on our results for type, flag, flag2 and flag3...
	//
	if(type==cSAVE&&currscr<128)
		*ls=1;
		
	if(type==cSAVE2&&currscr<128)
		*ls=2;
		
	if(refilling==REFILL_LIFE || flag==mfFAIRY||flag2==mfFAIRY||flag3==mfFAIRY)
	{
		fairycircle(REFILL_LIFE);
		
		if(fairyclk!=0) return;
	}
	
	if(refilling==REFILL_MAGIC || flag==mfMAGICFAIRY||flag2==mfMAGICFAIRY||flag3==mfMAGICFAIRY)
	{
		fairycircle(REFILL_MAGIC);
		
		if(fairyclk!=0) return;
	}
	
	if(refilling==REFILL_ALL || flag==mfALLFAIRY||flag2==mfALLFAIRY||flag3==mfALLFAIRY)
	{
		fairycircle(REFILL_ALL);
		
		if(fairyclk!=0) return;
	}
	
	// Just in case Link was moved off of the fairy flag
	if(refilling==REFILL_FAIRYDONE)
	{
		fairycircle(REFILL_NONE);
		
		if(fairyclk!=0) return;
	}
	
	if(flag==mfZELDA||flag2==mfZELDA||flag3==mfZELDA || combo_class_buf[type].win_game)
	{
	attackclk = 0; //get rid of Link's sword if it was stuck out, charged. 
		saved_Zelda();
		return;
	}
	
	if(z>0 && !(tmpscr->flags2&fAIRCOMBOS))
		return;
		
	if((type==cTRIGNOFLAG || type==cTRIGFLAG))
	{ 
	
		if((((ty+8)&0xF0)+((tx+8)>>4))!=stepsecret || get_bit(quest_rules,qr_TRIGGERSREPEAT))
		{
			stepsecret = (((ty+8)&0xF0)+((tx+8)>>4)); 
			sfx(combobuf[tmpscr->data[stepsecret]].attribytes[0],pan((int32_t)x));
			//zprint("Step Secrets Sound: %d\n", combobuf[tmpscr->data[stepsecret]].attribytes[0]);
			
			if(type==cTRIGFLAG && canPermSecret())
			{ 
				if(!(tmpscr->flags5&fTEMPSECRETS)) setmapflag(mSECRET);
				
				hidden_entrance(0,true,false);
			}
			else 
			{
				bool only16_31 = get_bit(quest_rules,qr_STEPTEMP_SECRET_ONLY_16_31)?true:false;
				hidden_entrance(0,true,only16_31);
			}
		}
	}
	else if(!didstrig)
	{
		stepsecret = -1; 
	}
	
	// Drown if:
	// * Water (obviously walkable),
	// * Quest Rule allows it,
	// * Not on stepladder,
	// * Not jumping,
	// * Not hovering,
	// * Not rafting,
	// * Not swallowed,
	// * Not a dried lake.
	
	// This used to check for swimming too, but I moved that into the block so that you can drown in higher-leveled water. -Dimi
	
	if(water > 0 && get_bit(quest_rules,qr_DROWN) && z==0  && fall>=0 && !ladderx && hoverclk==0 && action!=rafting && !inlikelike && !DRIEDLAKE)
	{
		if(current_item(itype_flippers) <= 0 || current_item(itype_flippers) < combobuf[water].attribytes[0] || ((combobuf[water].usrflags&cflag1) && !(itemsbuf[current_item_id(itype_flippers)].flags & ITEM_FLAG3))) 
		{
			if (combobuf[water].usrflags&cflag1) Drown(1);
			else Drown();
		}
		else if (!isSwimming())
		{
			SetSwim();
			if (!IsSideSwim()) attackclk = charging = spins = 0;
			landswim=0;
			return;
		}
	}
	
	
	if(type==cSTEP)
	{ 
	//zprint2("Link.HasHeavyBoots(): is: %s\n", ( (Link.HasHeavyBoots()) ? "true" : "false" ));
		if((((ty+8)&0xF0)+((tx+8)>>4))!=stepnext)
		{
			stepnext=((ty+8)&0xF0)+((tx+8)>>4);
			
			if
		(
		COMBOTYPE(tx+8,ty+8)==cSTEP && /*required item*/
			(!combobuf[tmpscr->data[stepnext]].attribytes[1] || (combobuf[tmpscr->data[stepnext]].attribytes[1] && game->item[combobuf[tmpscr->data[stepnext]].attribytes[1]]) )
			&& /*HEAVY*/
			( ( !combobuf[tmpscr->data[stepnext]].usrflags&cflag1 ) || ((combobuf[tmpscr->data[stepnext]].usrflags&cflag1) && Link.HasHeavyBoots() ) )
		)
		
		{
		sfx(combobuf[tmpscr->data[stepnext]].attribytes[0],pan((int32_t)x));
				tmpscr->data[stepnext]++;
		
			}
			
			if
		(
		COMBOTYPE(tx+8,ty+8)==cSTEPSAME && /*required item*/
			(!combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1] || (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1] && game->item[combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1]]) )
			&& /*HEAVY*/
			( ( !combobuf[tmpscr->data[stepnext]].usrflags&cflag1 ) || ((combobuf[tmpscr->data[stepnext]].usrflags&cflag1) && Link.HasHeavyBoots() ) )
		)
			{
				int32_t stepc = tmpscr->data[stepnext];
				sfx(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[0],pan((int32_t)x));
				for(int32_t k=0; k<176; k++)
				{
					if(tmpscr->data[k]==stepc)
					{
						tmpscr->data[k]++;
					}
				}
			}
			
			if
		(
			COMBOTYPE(tx+8,ty+8)==cSTEPALL && /*required item*/
			(!combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1] || (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1] && game->item[combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1]]) )
			&& /*HEAVY*/
			( ( !combobuf[tmpscr->data[stepnext]].usrflags&cflag1 ) || ((combobuf[tmpscr->data[stepnext]].usrflags&cflag1) && Link.HasHeavyBoots() ) )
		)
			{
		sfx(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[0],pan((int32_t)x));
				for(int32_t k=0; k<176; k++)
				{
					if(
						(combobuf[tmpscr->data[k]].type==cSTEP)||
						(combobuf[tmpscr->data[k]].type==cSTEPSAME)||
						(combobuf[tmpscr->data[k]].type==cSTEPALL)||
						(combobuf[tmpscr->data[k]].type==cSTEPCOPY)
					)
					{
						tmpscr->data[k]++;
					}
				}
			}
		}
	}
	else if(type==cSTEPSFX)
	{ 
	//We probably only want to do this when the player is moving.
	int32_t thesfx = combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[0];
	if ( thesfx > 0 && !sfx_allocated(thesfx) && action==walking )
		sfx(thesfx,pan((int32_t)x));
	if ( combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag1 && action==walking ) //landmine
	{
		int32_t wpn = combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[1];
		int32_t wpdir = combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2];
		if ( ((unsigned)wpdir) > r_down )
		{
			wpdir = zc_oldrand()&3;
		}
		int32_t damg = combobuf[MAPCOMBO(tx+8,ty+8)].attributes[0]/10000L;
		switch(wpn)
		{
			//eweapons
			case ewFireball:
			case ewArrow:
			case ewBrang:
			case ewSword:
			case ewRock:
			case ewMagic:
			case ewBomb:
			case ewSBomb:
			case ewLitBomb:
			case ewLitSBomb:
			case ewFireTrail:
			case ewFlame:
			case ewWind:
			case ewFlame2:
			case ewFlame2Trail:
			case ewIce:
			case ewFireball2:
			
				Ewpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir, -1,-1,false)); 
				if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
				{
					weapon *w = (weapon*)Ewpns.spr(Ewpns.Count()-1); //last created
					w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
				}
				break;
			
			case wBeam:
			case wBrang:
			case wBomb:
			case wSBomb:
			case wLitBomb:
			case wLitSBomb:
			case wArrow:
			
			case wWhistle:
			case wBait:
			case wMagic:
			case wWind:
			case wRefMagic:
			case wRefFireball:
			case wRefRock:
			case wRefBeam:
			case wIce:
			case wFlame: 
			case wSound: // -Z: sound + defence split == digdogger, sound + one hit kill == pols voice -Z
			//case wThrowRock: 
			//case wPot: //Thrown pot or rock -Z
			//case wLit: //Lightning or Electric -Z
			//case wBombos: 
			//case wEther: 
			//case wQuake:// -Z
			//case wSword180: 
			//case wSwordLA:
				Lwpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir,-1,getUID(),false,0,1,0)); 
				if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
				{
					weapon *w = (weapon*)Lwpns.spr(Lwpns.Count()-1); //last created
					w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
				}
				break;
			
			case wFire:
				//if (combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag2) //wscript lwpn
				//{
					//:weapon(zfix X,zfix Y,zfix Z,int32_t Id,int32_t Type,int32_t pow,int32_t Dir, int32_t Parentitem, int32_t prntid, bool isDummy, byte script_gen, byte isLW, byte special)
					Lwpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir,-1,getUID(),false,0,1,0)); break;
				//}
				//else //wscript ewpn
				//{
				//	Ewpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir,-1,-1,false,0,0,0)); break;
				//}
				
			//lweacase wScript1:
			case wScript2: 
			case wScript3:
			case wScript4:
			case wScript5:
			case wScript6:
			case wScript7:
			case wScript8:
			case wScript9:
			case wScript10:
			
			//custo weapons (could be either type)
			
				if (combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag2) //wscript lwpn
				{
					//:weapon(zfix X,zfix Y,zfix Z,int32_t Id,int32_t Type,int32_t pow,int32_t Dir, int32_t Parentitem, int32_t prntid, bool isDummy, byte script_gen, byte isLW, byte special)
					Lwpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir,-1,getUID(),false,0,1,0)); 
					if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
					{
						weapon *w = (weapon*)Lwpns.spr(Lwpns.Count()-1); //last created
						w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
					}
					if (combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag4) //direct damage from custom/script lweapons
					{
						hitdir = -1;
						if (action != rafting && action != freeze && action!=sideswimfreeze && !hclk)
						{
							if (IsSideSwim())
							{
								action=sideswimhit; FFCore.setLinkAction(sideswimhit);
							}
							else if (action == swimming || hopclk == 0xFF)
							{
								action=swimhit; FFCore.setLinkAction(swimhit);
							}
							else
							{
								action=gothit; FFCore.setLinkAction(gothit);
							}
							if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
							{
								spins = charging = attackclk = 0;
								attack=none;
								tapping = false;
							}
							int32_t dmgamt = ((damg > 0) ? damg : 4);
		
							game->set_life(game->get_life()-ringpower(dmgamt));
							
							hclk=48;
							sfx(getHurtSFX(),pan(x.getInt()));
						}
					}
					break;
				}
				else //wscript ewpn
				{
					Ewpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir, -1,-1,false)); 
					if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
					{
						weapon *w = (weapon*)Ewpns.spr(Ewpns.Count()-1); //last created
						w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
					}
					break;
				}
				
			case wSSparkle:
			case wFSparkle:
				Lwpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,wpn,0,((damg > 0) ? damg : 4),wpdir,-1,getUID(),false,0,1,0)); 
				if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
				{
					weapon *w = (weapon*)Lwpns.spr(Lwpns.Count()-1); //last created
					w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
				}
				if (combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag4) //direct damage from custom/script lweapons
				{
					hitdir = -1;
					if (action != rafting && action != freeze && action != sideswimfreeze && !hclk)
					{
						if (IsSideSwim())
						{
							action=sideswimhit; FFCore.setLinkAction(sideswimhit);
						}
						else if (action == swimming || hopclk == 0xFF)
						{
							action=swimhit; FFCore.setLinkAction(swimhit);
						}
						else
						{
							action=gothit; FFCore.setLinkAction(gothit);
						}
						if(charging > 0 || spins > 0 || attack == wSword || attack == wHammer)
						{
							spins = charging = attackclk = 0;
							attack=none;
							tapping = false;
						}
						int32_t dmgamt = ((damg > 0) ? damg : 4);

						game->set_life(game->get_life()-ringpower(dmgamt));
						
						hclk=48;
						sfx(getHurtSFX(),pan(x.getInt()));
					}
				}
				break;
			
			default: //enemy bomb
				//(zfix X,zfix Y,zfix Z,int32_t Id,int32_t Type,int32_t pow,int32_t Dir, int32_t Parentitem, int32_t prntid, bool isDummy, byte script_gen, byte isLW, byte special) : sprite(), parentid(
				//Ewpns.add(new weapon((zfix)tx+8,(zfix)ty+8,(zfix)0,ewLitBomb,16,0,0, -1,-1,false)); break;
				Ewpns.add(new weapon((zfix)tx,(zfix)ty,(zfix)0,ewLitBomb,0,((damg > 0) ? damg : 4),up, -1,-1,false)); 
				if (combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3] > 0 && combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2] < 256 )
				{
					weapon *w = (weapon*)Ewpns.spr(Ewpns.Count()-1); //last created
					w->LOADGFX(combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[3]);
				}
				break;
			
			//x,y,z, wpn, 0, dmisc4, dir,-1,getUID(),false);
		}
		if (!(combobuf[MAPCOMBO(tx+8,ty+8)].usrflags&cflag3) ) //Don't Advance
		{
			stepnext=((ty+8)&0xF0)+((tx+8)>>4);
			tmpscr->data[stepnext]++;
		}
		
		
		
		
	}
	}
	else if(type==cTALLGRASS||type==cTALLGRASSTOUCHY||type==cTALLGRASSNEXT)
	{ 
	//We probably only want to do this when the player is moving.
	int32_t thesfx = combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[2];
	if ( thesfx > 0 && !sfx_allocated(thesfx) && action==walking )
		sfx(thesfx,pan((int32_t)x));
	}
	else if(type==cSHALLOWWATER)
	{ 
	//We probably only want to do this when the player is moving.
	int32_t thesfx = combobuf[MAPCOMBO(tx+8,ty+8)].attribytes[0];
	if ( thesfx > 0 && !sfx_allocated(thesfx) && action==walking )
		sfx(thesfx,pan((int32_t)x));
	}
	else stepnext = -1;
	
	detail_int[0]=tx;
	detail_int[1]=ty;
	
	
	if(!((type==cCAVE || type==cCAVE2) && z==0) && type!=cSTAIR &&
			type!=cPIT && type!=cSWIMWARP && type!=cRESET &&
			!(type==cDIVEWARP && isDiving()))
	{
		switch(flag)
		{
		case mfDIVE_ITEM:
			if(isDiving() && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
			{
				additem(x, y, tmpscr->catchall,
						ipONETIME2 | ipBIGRANGE | ipHOLDUP | ipNODRAW | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0));
				sfx(tmpscr->secretsfx);
			}
			
			return;
			
		case mfRAFT:
		case mfRAFT_BRANCH:
		
			//		if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && type==cOLD_DOCK)
			if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && action!=sideswimhit && z==0 && combo_class_buf[type].dock)
			{
				if(isRaftFlag(nextflag(tx,ty,dir,false))||isRaftFlag(nextflag(tx,ty,dir,true)))
				{
					reset_swordcharge();
					action=rafting; FFCore.setLinkAction(rafting);
					raftclk=0;
					sfx(tmpscr->secretsfx);
				}
			}
			
			return;
			
		default:
			break;
			//return;
		}
		
		switch(flag2)
		{
		case mfDIVE_ITEM:
			if(isDiving() && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
			{
				additem(x, y, tmpscr->catchall,
						ipONETIME2 | ipBIGRANGE | ipHOLDUP | ipNODRAW | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0));
				sfx(tmpscr->secretsfx);
			}
			
			return;
			
		case mfRAFT:
		case mfRAFT_BRANCH:
		
			//		if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && type==cOLD_DOCK)
			if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && action!=sideswimhit && z==0 && combo_class_buf[type].dock)
			{
				if((isRaftFlag(nextflag(tx,ty,dir,false))||isRaftFlag(nextflag(tx,ty,dir,true))))
				{
					reset_swordcharge();
					action=rafting; FFCore.setLinkAction(rafting);
					raftclk=0;
					sfx(tmpscr->secretsfx);
				}
			}
			
			return;
			
		default:
			break;
			//return;
		}
		
		switch(flag3)
		{
		case mfDIVE_ITEM:
			if(isDiving() && (!getmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW) || (tmpscr->flags9&fBELOWRETURN)))
			{
				additem(x, y, tmpscr->catchall,
						ipONETIME2 | ipBIGRANGE | ipHOLDUP | ipNODRAW | ((tmpscr->flags8&fITEMSECRET) ? ipSECRETS : 0));
				sfx(tmpscr->secretsfx);
			}
			
			return;
			
		case mfRAFT:
		case mfRAFT_BRANCH:
		
			//	  if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && type==cOLD_DOCK)
			if(current_item(itype_raft) && action!=rafting && action!=swimhit && action!=gothit && action!=sideswimhit && z==0 && combo_class_buf[type].dock)
			{
				if((isRaftFlag(nextflag(tx,ty,dir,false))||isRaftFlag(nextflag(tx,ty,dir,true))))
				{
					reset_swordcharge();
					action=rafting; FFCore.setLinkAction(rafting);
					raftclk=0;
					sfx(tmpscr->secretsfx);
				}
			}
			
			return;
			
		default:
			return;
		}
	}
	
	
	int32_t t=(currscr<128)?0:1;
	
	if((type==cCAVE || type==cCAVE2) && (tmpscr[t].tilewarptype[index]==wtNOWARP)) return;
	
	//don't do this for canceled warps -DD
	//I have no idea why we do this skip, but I'll dutifully propagate it to all cases below...
	/*if(tmpscr[t].tilewarptype[index] != wtNOWARP)
	{
		draw_screen(tmpscr);
		advanceframe(true);
	}*/
	
	bool skippedaframe=false;
	
	if(type==cCAVE || type==cCAVE2 || type==cSTAIR)
	{
		// Stop music only if:
		// * entering a Guy Cave
		// * warping to a DMap whose music is different.
		
		int32_t tdm = tmpscr[t].tilewarpdmap[index];
		
		if(tmpscr[t].tilewarptype[index]<=wtPASS)
		{
			if((DMaps[currdmap].flags&dmfCAVES) && tmpscr[t].tilewarptype[index] == wtCAVE)
				music_stop();
		}
		else
		{
			if(zcmusic!=NULL)
			{
				if(strcmp(zcmusic->filename, DMaps[tdm].tmusic) != 0 ||
						zcmusic->track!=DMaps[tdm].tmusictrack)
					music_stop();
			}
			else if(DMaps[tmpscr->tilewarpdmap[index]].midi != (currmidi-ZC_MIDI_COUNT+4) &&
					TheMaps[(DMaps[tdm].map*MAPSCRS + (tmpscr[t].tilewarpscr[index] + DMaps[tdm].xoff))].screen_midi != (currmidi-ZC_MIDI_COUNT+4))
				music_stop();
		}
		
		stop_sfx(WAV_ER);
		bool opening = (tmpscr[t].tilewarptype[index]<=wtPASS && !(DMaps[currdmap].flags&dmfCAVES && tmpscr[t].tilewarptype[index]==wtCAVE)
						? false : COOLSCROLL);
						
		FFCore.warpScriptCheck();
		draw_screen(tmpscr);
		advanceframe(true);
		
		skippedaframe=true;
		
		if(type==cCAVE2) walkup2(opening);
		else if(type==cCAVE) walkdown(opening);
	}
	
	if(type==cPIT)
	{
		didpit=true;
		pitx=x;
		pity=y;
	warp_sound = warpsfx2;
	}
	
	if(DMaps[currdmap].flags&dmf3STAIR && (currscr==129 || !(DMaps[currdmap].flags&dmfGUYCAVES))
			&& tmpscr[specialcave > 0 && DMaps[currdmap].flags&dmfGUYCAVES ? 1:0].room==rWARP && type==cSTAIR)
	{
		if(!skippedaframe)
		{
			FFCore.warpScriptCheck();
			draw_screen(tmpscr);
			advanceframe(true);
		}
		
		// "take any road you want"
		int32_t dw = x<112 ? 1 : (x>136 ? 3 : 2);
		int32_t code = WARPCODE(currdmap,homescr,dw);
		
		if(code>-1)
		{
			currdmap = code>>8;
			dlevel  = DMaps[currdmap].level;
			currmap = DMaps[currdmap].map;
			homescr = (code&0xFF) + DMaps[currdmap].xoff;
			init_dmap();
			
			if(canPermSecret())
				setmapflag(mSECRET);
		}
		
		if(specialcave==STAIRCAVE) exitcave();
		
		return;
	}
	
	if(type==cRESET)
	{
		if(!skippedaframe)
		{
			FFCore.warpScriptCheck();
			draw_screen(tmpscr);
			advanceframe(true);
		}
		
		if(!(tmpscr->noreset&mSECRET)) unsetmapflag(mSECRET);
		
		if(!(tmpscr->noreset&mITEM)) unsetmapflag(mITEM);
		
		if(!(tmpscr->noreset&mBELOW)) unsetmapflag(mBELOW);
		
		if(!(tmpscr->noreset&mNEVERRET)) unsetmapflag(mNEVERRET);
		
		if(!(tmpscr->noreset&mCHEST)) unsetmapflag(mCHEST);
		
		if(!(tmpscr->noreset&mLOCKEDCHEST)) unsetmapflag(mLOCKEDCHEST);
		
		if(!(tmpscr->noreset&mBOSSCHEST)) unsetmapflag(mBOSSCHEST);
		
		if(!(tmpscr->noreset&mLOCKBLOCK)) unsetmapflag(mLOCKBLOCK);
		
		if(!(tmpscr->noreset&mBOSSLOCKBLOCK)) unsetmapflag(mBOSSLOCKBLOCK);
		
		if(isdungeon())
		{
			if(!(tmpscr->noreset&mDOOR_LEFT)) unsetmapflag(mDOOR_LEFT);
			
			if(!(tmpscr->noreset&mDOOR_RIGHT)) unsetmapflag(mDOOR_RIGHT);
			
			if(!(tmpscr->noreset&mDOOR_DOWN)) unsetmapflag(mDOOR_DOWN);
			
			if(!(tmpscr->noreset&mDOOR_UP)) unsetmapflag(mDOOR_UP);
		}
		
		didpit=true;
		pitx=x;
		pity=y;
		sdir=dir;
		dowarp(4,0, warpsfx2);
	}
	else
	{
		if(!skippedaframe && (tmpscr[t].tilewarptype[index]!=wtNOWARP))
		{
			FFCore.warpScriptCheck();
			draw_screen(tmpscr);
			advanceframe(true);
		}
		
		sdir = dir;
		dowarp(0,index, warpsfx2);
	}
}

int32_t selectWlevel(int32_t d)
{
    if(TriforceCount()==0)
        return 0;
        
    word l = game->get_wlevel();
    
    do
    {
        if(d==0 && (game->lvlitems[l+1] & liTRIFORCE))
            break;
        else if(d<0)
            l = (l==0) ? 7 : l-1;
        else
            l = (l==7) ? 0 : l+1;
    }
    while(!(game->lvlitems[l+1] & liTRIFORCE));
    
    game->set_wlevel(l);
    return l;
}

// Would someone tell the Dodongos to shut their yaps?!
void kill_enemy_sfx()
{
    for(int32_t i=0; i<guys.Count(); i++)
    {
        if(((enemy*)guys.spr(i))->bgsfx)
            stop_sfx(((enemy*)guys.spr(i))->bgsfx);
    }
    if (Link.action!=inwind) stop_sfx(WAV_ZN1WHIRLWIND);
    if(tmpscr->room==rGANON)
        stop_sfx(WAV_ROAR);
}

bool LinkClass::HasHeavyBoots()
{
	for ( int32_t q = 0; q < MAXITEMS; ++q )
	{
		if ( game->item[q] && ( itemsbuf[q].family == itype_boots ) && /*iron*/ (itemsbuf[q].flags&ITEM_FLAG2) ) return true;
	}
	return false;
}

void LinkClass::setEntryPoints(int32_t x2, int32_t y2)
{
    mapscr *m = &tmpscr[0];
	m->entry_x = x2;
	m->entry_y = y2;
	//m->entry_x = entry_x;
	//m->entry_y = entry_y;
    entry_x=warpx=x2;
    entry_y=warpy=y2;
}

const char *roomtype_string[rMAX] =
{
    "(None)","Special Item","Pay for Info","Secret Money","Gamble",
    "Door Repair","Red Potion or Heart Container","Feed the Goriya","Triforce Check",
    "Potion Shop","Shop","More Bombs","Leave Money or Life","10 Rupees",
    "3-Stair Warp","Ganon","Zelda", "-<item pond>", "1/2 Magic Upgrade", "Learn Slash", "More Arrows","Take One Item"
};

bool LinkClass::dowarp(int32_t type, int32_t index, int32_t warpsfx)
{
	byte reposition_sword_postwarp = 0;
	if(index<0)
	{
		return false;
	}
	is_warping = true;
	for ( int32_t q = 0; q < Lwpns.Count(); ++q )
	{
		weapon *swd=NULL;
		swd = (weapon*)Lwpns.spr(q);
		if(swd->id == (attack==wSword ? wSword : wWand))
		{
			Lwpns.del(q);
		}
	}
	
	attackclk = charging = spins = tapping = 0;
	attack = none;
	if ( warp_sound > 0 ) warpsfx = warp_sound;
	word wdmap=0;
	byte wscr=0,wtype=0,t=0;
	bool overlay=false;
	t=(currscr<128)?0:1;
	int32_t wrindex = 0;
	bool wasSideview = isSideViewGravity(t); // (tmpscr[t].flags7 & fSIDEVIEW)!=0 && !ignoreSideview;
	
	// Drawing commands probably shouldn't carry over...
	if ( !get_bit(quest_rules,qr_SCRIPTDRAWSINWARPS) )
		script_drawing_commands.Clear();
	
	switch(type)
	{
		case 0:                                                 // tile warp
			wtype = tmpscr[t].tilewarptype[index];
			wdmap = tmpscr[t].tilewarpdmap[index];
			wscr = tmpscr[t].tilewarpscr[index];
			overlay = get_bit(&tmpscr[t].tilewarpoverlayflags,index)?1:0;
			wrindex=(tmpscr->warpreturnc>>(index*2))&3;
			break;
			
		case 1:                                                 // side warp
			wtype = tmpscr[t].sidewarptype[index];
			wdmap = tmpscr[t].sidewarpdmap[index];
			wscr = tmpscr[t].sidewarpscr[index];
			overlay = get_bit(&tmpscr[t].sidewarpoverlayflags,index)?1:0;
			wrindex=(tmpscr->warpreturnc>>(8+(index*2)))&3;
			//tmpscr->doscript = 0; //kill the currebt screen's script so that it does not continue to run during the scroll.
			//there is no doscript for screen scripts. They run like ffcs. 

			break;
			
		case 2:                                                 // whistle warp
		{
			wtype = wtWHISTLE;
			int32_t wind = whistleitem>-1 ? itemsbuf[whistleitem].misc2 : 8;
			int32_t level=0;
			
			if(blowcnt==0)
				level = selectWlevel(0);
			else
			{
				for(int32_t i=0; i<abs(blowcnt); i++)
					level = selectWlevel(blowcnt);
			}
			
			if(level > QMisc.warp[wind].size && QMisc.warp[wind].size>0)
			{
				level %= QMisc.warp[wind].size;
				game->set_wlevel(level);
			}
			
			wdmap = QMisc.warp[wind].dmap[level];
			wscr = QMisc.warp[wind].scr[level];
		}
		break;
		
		case 3:
			wtype = wtIWARP;
			wdmap = cheat_goto_dmap;
			wscr = cheat_goto_screen;
			break;
			
		case 4:
			wtype = wtIWARP;
			wdmap = currdmap;
			wscr = homescr-DMaps[currdmap].xoff;
			break;
	}
	
	bool intradmap = (wdmap == currdmap);
	int32_t olddmap = currdmap;
	rehydratelake(type!=wtSCROLL);
	
	switch(wtype)
	{
	case wtCAVE:
	{
		// cave/item room
		ALLOFF();
		homescr=currscr;
		currscr=0x80;
		
		if(DMaps[currdmap].flags&dmfCAVES)                                         // cave
		{
			music_stop();
			kill_sfx();
			
			if(tmpscr->room==rWARP)
			{
				currscr=0x81;
				specialcave = STAIRCAVE;
			}
			else specialcave = GUYCAVE;
			
			//lighting(2,dir);
			lighting(false, true);
			loadlvlpal(10);
			bool b2 = COOLSCROLL&&
					  ((combobuf[MAPCOMBO(x,y-16)].type==cCAVE)||(combobuf[MAPCOMBO(x,y-16)].type==cCAVE2)||
					   (combobuf[MAPCOMBO(x,y-16)].type==cCAVEB)||(combobuf[MAPCOMBO(x,y-16)].type==cCAVE2B)||
					   (combobuf[MAPCOMBO(x,y-16)].type==cCAVEC)||(combobuf[MAPCOMBO(x,y-16)].type==cCAVE2C)||
					   (combobuf[MAPCOMBO(x,y-16)].type==cCAVED)||(combobuf[MAPCOMBO(x,y-16)].type==cCAVE2D));
			blackscr(30,b2?false:true);
			loadscr(0,wdmap,currscr,up,false);
			loadscr(1,wdmap,homescr,up,false);
			//preloaded freeform combos
			ffscript_engine(true);
			putscr(scrollbuf,0,0,tmpscr);
			putscrdoors(scrollbuf,0,0,tmpscr);
			dir=up;
			x=112;
			y=160;
			if(didpit)
			{
				didpit=false;
				x=pitx;
				y=pity;
			}
			
			reset_hookshot();
			if(reposition_sword_postwarp)
			{
				weapon *swd=NULL;
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					swd = (weapon*)Lwpns.spr(i);
					
					if(swd->id == (attack==wSword ? wSword : wWand))
					{
					int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
					int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
					positionSword(swd,item_id);
					break;
					}
				}
			}
			stepforward(diagonalMovement?5:6, false);
		}
		else                                                  // item room
		{
			specialcave = ITEMCELLAR;
			map_bkgsfx(false);
			kill_enemy_sfx();
			draw_screen(tmpscr,false);
			
			//unless the room is already dark, fade to black
			if(!darkroom)
			{
				darkroom = true;
				fade(DMaps[currdmap].color,true,false);
			}
			
			blackscr(30,true);
			loadscr(0,wdmap,currscr,down,false);
			loadscr(1,wdmap,homescr,-1,false);
			if ( dontdraw < 2 ) {  dontdraw=1; }
			draw_screen(tmpscr);
			fade(11,true,true);
			darkroom = false;
			dir=down;
			x=48;
			y=0;
			
			// is this didpit check necessary?
			if(didpit)
			{
				didpit=false;
				x=pitx;
				y=pity;
			}
			
			reset_hookshot();
			if(reposition_sword_postwarp)
			{
				weapon *swd=NULL;
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					swd = (weapon*)Lwpns.spr(i);
					
					if(swd->id == (attack==wSword ? wSword : wWand))
					{
					int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
					int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
					positionSword(swd,item_id);
					break;
					}
				}
			}
			lighting(false, true);
			if ( dontdraw < 2 ) { dontdraw=0; }
			stepforward(diagonalMovement?16:18, false);
		}
		if (get_bit(quest_rules,qr_SCREEN80_OWN_MUSIC)) playLevelMusic();
		break;
	}
	
	case wtPASS:                                            // passageway
	{
		map_bkgsfx(false);
		kill_enemy_sfx();
		ALLOFF();
		//play sound
		if(warpsfx > 0) sfx(warpsfx,pan(x.getInt()));
		homescr=currscr;
		currscr=0x81;
		specialcave = PASSAGEWAY;
		byte warpscr2 = wscr + DMaps[wdmap].xoff;
		draw_screen(tmpscr,false);
		
		if(!darkroom)
			fade(DMaps[currdmap].color,true,false);
			
		darkroom=true;
		blackscr(30,true);
		loadscr(0,wdmap,currscr,down,false);
		loadscr(1,wdmap,homescr,-1,false);
		//preloaded freeform combos
		ffscript_engine(true);
		if ( dontdraw < 2 ) { dontdraw=1; }
		draw_screen(tmpscr);
		lighting(false, true);
		dir=down;
		x=48;
		
		if((homescr&15) > (warpscr2&15))
		{
			x=192;
		}
		
		if((homescr&15) == (warpscr2&15))
		{
			if((currscr>>4) > (warpscr2>>4))
			{
				x=192;
			}
		}
		
		// is this didpit check necessary?
		if(didpit)
		{
			didpit=false;
			x=pitx;
			y=pity;
		}
		
		setEntryPoints(x,y=0);
		reset_hookshot();
		if(reposition_sword_postwarp)
		{
			weapon *swd=NULL;
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				swd = (weapon*)Lwpns.spr(i);
				
				if(swd->id == (attack==wSword ? wSword : wWand))
				{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
				}
			}
		}
		if ( dontdraw < 2 ) { dontdraw=0; }
		stepforward(diagonalMovement?16:18, false);
		newscr_clk=frame;
		activated_timed_warp=false;
		stepoutindex=index;
		stepoutscr = warpscr2;
		stepoutdmap = wdmap;
		stepoutwr=wrindex;
	}
	break;
	
	case wtEXIT: // entrance/exit
	{
		lighting(false,false,pal_litRESETONLY);//Reset permLit, and do nothing else; lighting was not otherwise called on a wtEXIT warp.
		ALLOFF();
		music_stop();
		kill_sfx();
		blackscr(30,false);
		currdmap = wdmap;
		dlevel=DMaps[currdmap].level;
		currmap=DMaps[currdmap].map;
		init_dmap();
		update_subscreens(wdmap);
		loadfullpal();
		ringcolor(false);
		loadlvlpal(DMaps[currdmap].color);
		//lastentrance_dmap = currdmap;
		homescr = currscr = wscr + DMaps[currdmap].xoff;
		loadscr(0,currdmap,currscr,-1,overlay);
		
		if(tmpscr->flags&fDARK)
		{
			if(get_bit(quest_rules,qr_FADE))
			{
				interpolatedfade();
			}
			else
			{
				loadfadepal((DMaps[currdmap].color)*pdLEVEL+poFADE3);
			}
			
			darkroom=naturaldark=true;
		}
		else
		{
			darkroom=naturaldark=false;
		}
		
		int32_t wrx,wry;
		
		if(get_bit(quest_rules,qr_NOARRIVALPOINT))
		{
			wrx=tmpscr->warpreturnx[0];
			wry=tmpscr->warpreturny[0];
		}
		else
		{
			wrx=tmpscr->warparrivalx;
			wry=tmpscr->warparrivaly;
		}
		
		if(((wrx>0||wry>0)||(get_bit(quest_rules,qr_WARPSIGNOREARRIVALPOINT)))&&(!(tmpscr->flags6&fNOCONTINUEHERE)))
		{
			if(dlevel)
			{
				lastentrance = currscr;
			}
			else
			{
				lastentrance = DMaps[currdmap].cont + DMaps[currdmap].xoff;
			}
			
			lastentrance_dmap = wdmap;
		}
		
		if(dlevel)
		{
			if(get_bit(quest_rules,qr_NOARRIVALPOINT))
			{
				x=tmpscr->warpreturnx[wrindex];
				y=tmpscr->warpreturny[wrindex];
			}
			else
			{
				x=tmpscr->warparrivalx;
				y=tmpscr->warparrivaly;
			}
		}
		else
		{
			x=tmpscr->warpreturnx[wrindex];
			y=tmpscr->warpreturny[wrindex];
		}
		
		if(didpit)
		{
			didpit=false;
			x=pitx;
			y=pity;
		}
		
		dir=down;
		
		if(x==0)   dir=right;
		
		if(x==240) dir=left;
		
		if(y==0)   dir=down;
		
		if(y==160) dir=up;
		
		if(dlevel)
		{
			// reset enemy kill counts
			for(int32_t i=0; i<128; i++)
			{
				game->guys[(currmap*MAPSCRSNORMAL)+i] = 0;
				game->maps[(currmap*MAPSCRSNORMAL)+i] &= ~mTMPNORET;
			}
		}
		
		markBmap(dir^1);
		//preloaded freeform combos
		ffscript_engine(true);
	
		reset_hookshot();
		if(reposition_sword_postwarp)
		{
			weapon *swd=NULL;
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				swd = (weapon*)Lwpns.spr(i);
				
				if(swd->id == (attack==wSword ? wSword : wWand))
				{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
				}
			}
		}

		if(isdungeon())
		{
			openscreen();
			if(get_bit(extra_rules, er_SHORTDGNWALK)==0 && get_bit(quest_rules, qr_SHORTDGNWALK)==0)
				stepforward(diagonalMovement?11:12, false);
			else
				// Didn't walk as far pre-1.93, and some quests depend on that
				stepforward(8, false);
		}
		else
		{
			if(!COOLSCROLL)
				openscreen();
				
			int32_t type1 = combobuf[MAPCOMBO(x,y-16)].type; // Old-style blue square placement
			int32_t type2 = combobuf[MAPCOMBO(x,y)].type;
			int32_t type3 = combobuf[MAPCOMBO(x,y+16)].type; // More old-style blue square placement
			
			if((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED))
			{
				reset_pal_cycling();
				putscr(scrollbuf,0,0,tmpscr);
				putscrdoors(scrollbuf,0,0,tmpscr);
				walkup(COOLSCROLL);
			}
			else if((type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D))
			{
				reset_pal_cycling();
				putscr(scrollbuf,0,0,tmpscr);
				putscrdoors(scrollbuf,0,0,tmpscr);
				walkdown2(COOLSCROLL);
			}
			else if(COOLSCROLL)
			{
				openscreen();
			}
		}
		
		show_subscreen_life=true;
		show_subscreen_numbers=true;
		playLevelMusic();
		currcset=DMaps[currdmap].color;
		dointro();
		setEntryPoints(x,y);
		
		for(int32_t i=0; i<6; i++)
			visited[i]=-1;
			
		break;
	}
	
	case wtSCROLL:                                          // scrolling warp
	{
		int32_t c = DMaps[currdmap].color;
		scrolling_map = currmap;
		currmap = DMaps[wdmap].map;
		update_subscreens(wdmap);
		
		dlevel = DMaps[wdmap].level;
		//check if Link has the map for the new location before updating the subscreen. ? -Z
		//This works only in one direction, if Link had a map, to not having one.
		//If Link does not have a map, and warps somewhere where he does, then the map still briefly shows. 
		update_subscreens(wdmap);
		
		if ( has_item(itype_map, dlevel) ) 
		{
			//Blank the map during an intra-dmap scrolling warp. 
			dlevel = -1; //a hack for the minimap. This works!! -Z
		}
		
		// fix the scrolling direction, if it was a tile or instant warp
		if(type==0 || type>=3)
		{
			sdir = dir;
		}
		
		scrollscr(sdir, wscr+DMaps[wdmap].xoff, wdmap);
		dlevel = DMaps[wdmap].level; //Fix dlevel and draw the map (end hack). -Z
	
		reset_hookshot();
		if(reposition_sword_postwarp)
		{
			weapon *swd=NULL;
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				swd = (weapon*)Lwpns.spr(i);
				
				if(swd->id == (attack==wSword ? wSword : wWand))
				{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
				}
			}
		}
		if(!intradmap)
		{
			currdmap = wdmap;
			dlevel = DMaps[currdmap].level;
			homescr = currscr = wscr + DMaps[wdmap].xoff;
			init_dmap();
			
			int32_t wrx,wry;
			
			if(get_bit(quest_rules,qr_NOARRIVALPOINT))
			{
				wrx=tmpscr->warpreturnx[0];
				wry=tmpscr->warpreturny[0];
			}
			else
			{
				wrx=tmpscr->warparrivalx;
				wry=tmpscr->warparrivaly;
			}
			
			if(((wrx>0||wry>0)||(get_bit(quest_rules,qr_WARPSIGNOREARRIVALPOINT)))&&(!get_bit(quest_rules,qr_NOSCROLLCONTINUE))&&(!(tmpscr->flags6&fNOCONTINUEHERE)))
			{
				if(dlevel)
				{
					lastentrance = currscr;
				}
				else
				{
					lastentrance = DMaps[currdmap].cont + DMaps[currdmap].xoff;
				}
				
				lastentrance_dmap = wdmap;
			}
		}
		if(DMaps[currdmap].color != c)
		{
			lighting(false, true);
		}
		
		playLevelMusic();
		currcset=DMaps[currdmap].color;
		dointro();
	}
	break;
	
	case wtWHISTLE:                                         // whistle warp
	{
		scrolling_map = currmap;
		currmap = DMaps[wdmap].map;
		scrollscr(index, wscr+DMaps[wdmap].xoff, wdmap);
		reset_hookshot();
		currdmap=wdmap;
		dlevel=DMaps[currdmap].level;
		lighting(false, true);
		init_dmap();
		
		playLevelMusic();
		currcset=DMaps[currdmap].color;
		dointro();
		action=inwind; FFCore.setLinkAction(inwind);
		int32_t wry;
		
		if(get_bit(quest_rules,qr_NOARRIVALPOINT))
			wry=tmpscr->warpreturny[0];
		else wry=tmpscr->warparrivaly;
		
		int32_t wrx;
		
		if(get_bit(quest_rules,qr_NOARRIVALPOINT))
			wrx=tmpscr->warpreturnx[0];
		else wrx=tmpscr->warparrivalx;
		
		Lwpns.add(new weapon((zfix)(index==left?240:index==right?0:wrx),(zfix)(index==down?0:index==up?160:wry),
							 (zfix)0,wWind,1,0,index,whistleitem,getUID(),false,false,true,1));
		whirlwind=255;
		whistleitem=-1;
	}
	break;
	
	case wtIWARP:
	case wtIWARPBLK:
	case wtIWARPOPEN:
	case wtIWARPZAP:
	case wtIWARPWAVE:                                       // insta-warps
	{
		bool old_192 = false;
		if (get_bit(quest_rules,qr_192b163_WARP))
		{
			if ( wtype == wtIWARPWAVE )
			{
				wtype = wtIWARPWAVE;
				old_192 = true;
			}
			if ( old_192 )
			{
				al_trace("Encountered a warp in a 1.92b163 style quest, that was set as a Wave Warp.\n %s\n", "Trying to redirect it into a Cancel Effect");
				didpit=false;
				update_subscreens();
				warp_sound = 0;
				is_warping = false;
				return false;
			}
		}
		//for determining whether to exit cave
		int32_t type1 = combobuf[MAPCOMBO(x,y-16)].type;
		int32_t type2 = combobuf[MAPCOMBO(x,y)].type;
		int32_t type3 = combobuf[MAPCOMBO(x,y+16)].type;
		
		bool cavewarp = ((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED)
						 ||(type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D));
					
		if(!(tmpscr->flags3&fIWARPFULLSCREEN))
		{
			//ALLOFF kills the action, but we want to preserve Link's action if he's swimming or diving -DD
			bool wasswimming = (action == swimming);
			int32_t olddiveclk = diveclk;
			ALLOFF();
			
			if(wasswimming)
			{
				Link.SetSwim();
				diveclk = olddiveclk;
			}
			
			kill_sfx();
		}
		//play sound
		if(warpsfx > 0) sfx(warpsfx,pan(x.getInt()));
		if(wtype==wtIWARPZAP)
		{
			zapout();
		}
		else if(wtype==wtIWARPWAVE)
		{
			//only draw Link if he's not in a cave -DD
			wavyout(!cavewarp);
		}
		else if(wtype!=wtIWARP)
		{
			bool b2 = COOLSCROLL&&cavewarp;
			blackscr(30,b2?false:true);
		}
		
		int32_t c = DMaps[currdmap].color;
		currdmap = wdmap;
		dlevel = DMaps[currdmap].level;
		currmap = DMaps[currdmap].map;
		init_dmap();
		update_subscreens(wdmap);
		
		ringcolor(false);
		
		if(DMaps[currdmap].color != c)
			loadlvlpal(DMaps[currdmap].color);
			
		homescr = currscr = wscr + DMaps[currdmap].xoff;
		
		lightingInstant(); // Also sets naturaldark
		
		loadscr(0,currdmap,currscr,-1,overlay);
		
		x = tmpscr->warpreturnx[wrindex];
		y = tmpscr->warpreturny[wrindex];
		
		if(didpit)
		{
			didpit=false;
			x=pitx;
			y=pity;
		}
		
		type1 = combobuf[MAPCOMBO(x,y-16)].type;
		type2 = combobuf[MAPCOMBO(x,y)].type;
		type3 = combobuf[MAPCOMBO(x,y+16)].type;
		
		if(x==0)   dir=right;
		
		if(x==240) dir=left;
		
		if(y==0)   dir=down;
		
		if(y==160) dir=up;
		
		markBmap(dir^1);
		
		int32_t checkwater = iswaterex(MAPCOMBO(x,y+8), currmap, currscr, -1, x,y+(bigHitbox?8:12)); //iswaterex can be intensive, so let's avoid as many calls as we can.
		
		if(checkwater && _walkflag(x,y+(bigHitbox?8:12),0,SWITCHBLOCK_STATE) && current_item(itype_flippers) > 0 && current_item(itype_flippers) >= combobuf[checkwater].attribytes[0] && (!(combobuf[checkwater].usrflags&cflag1) || (itemsbuf[current_item_id(itype_flippers)].flags & ITEM_FLAG3)))
		{
			hopclk=0xFF;
			SetSwim();
			if (!IsSideSwim()) attackclk = charging = spins = 0;
		}
		else
		{
			action = none; FFCore.setLinkAction(none);
		}
		//preloaded freeform combos
		ffscript_engine(true);
		
		putscr(scrollbuf,0,0,tmpscr);
		putscrdoors(scrollbuf,0,0,tmpscr);
		
		if((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED))
		{
			reset_pal_cycling();
			putscr(scrollbuf,0,0,tmpscr);
			putscrdoors(scrollbuf,0,0,tmpscr);
			walkup(COOLSCROLL);
		}
		else if((type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D))
		{
			reset_pal_cycling();
			putscr(scrollbuf,0,0,tmpscr);
			putscrdoors(scrollbuf,0,0,tmpscr);
			walkdown2(COOLSCROLL);
		}
		else if(wtype==wtIWARPZAP)
		{
			zapin();
		}
		else if(wtype==wtIWARPWAVE)
		{
			wavyin();
		}
		else if(wtype==wtIWARPOPEN)
		{
			openscreen();
		}
		if(reposition_sword_postwarp)
		{
			weapon *swd=NULL;
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				swd = (weapon*)Lwpns.spr(i);
				
				if(swd->id == (attack==wSword ? wSword : wWand))
				{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
				}
			}
		}
		show_subscreen_life=true;
		show_subscreen_numbers=true;
		playLevelMusic();
		currcset=DMaps[currdmap].color;
		dointro();
		setEntryPoints(x,y);
	}
	break;
	
	
	case wtNOWARP:
	{
		bool old_192 = false;
		if (get_bit(quest_rules,qr_192b163_WARP))
		{
			wtype = wtIWARPWAVE;
			old_192 = true;
		}
		if ( old_192 )
		{
			al_trace("Encountered a warp in a 1.92b163 style quest, that was set as a Cancel Warp.\n %s\n", "Trying to redirect it into a Wave Effect");
			//for determining whether to exit cave
			int32_t type1 = combobuf[MAPCOMBO(x,y-16)].type;
			int32_t type2 = combobuf[MAPCOMBO(x,y)].type;
			int32_t type3 = combobuf[MAPCOMBO(x,y+16)].type;
			
			bool cavewarp = ((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED)
					 ||(type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D));
					 
			if(!(tmpscr->flags3&fIWARPFULLSCREEN))
			{
				//ALLOFF kills the action, but we want to preserve Link's action if he's swimming or diving -DD
				bool wasswimming = (action == swimming);
				int32_t olddiveclk = diveclk;
				ALLOFF();
				
				if(wasswimming)
				{
					Link.SetSwim();
					diveclk = olddiveclk;
				}
				
				kill_sfx();
			}
			//play sound
			if(warpsfx > 0) sfx(warpsfx,pan(x.getInt()));	
			if(wtype==wtIWARPZAP)
			{
				zapout();
			}
			else if(wtype==wtIWARPWAVE)
			{
				//only draw Link if he's not in a cave -DD
				wavyout(!cavewarp);
			}
			else if(wtype!=wtIWARP)
			{
				bool b2 = COOLSCROLL&&cavewarp;
				blackscr(30,b2?false:true);
			}
			
			int32_t c = DMaps[currdmap].color;
			currdmap = wdmap;
			dlevel = DMaps[currdmap].level;
			currmap = DMaps[currdmap].map;
			init_dmap();
			update_subscreens(wdmap);
			
			ringcolor(false);
			
			if(DMaps[currdmap].color != c)
				loadlvlpal(DMaps[currdmap].color);
				
			homescr = currscr = wscr + DMaps[currdmap].xoff;
			
			lightingInstant(); // Also sets naturaldark
			
			loadscr(0,currdmap,currscr,-1,overlay);
			
			x = tmpscr->warpreturnx[wrindex];
			y = tmpscr->warpreturny[wrindex];
			
			if(didpit)
			{
				didpit=false;
				x=pitx;
				y=pity;
			}
				
			type1 = combobuf[MAPCOMBO(x,y-16)].type;
			type2 = combobuf[MAPCOMBO(x,y)].type;
			type3 = combobuf[MAPCOMBO(x,y+16)].type;
			
			if(x==0)   dir=right;
			
			if(x==240) dir=left;
			
			if(y==0)   dir=down;
			
			if(y==160) dir=up;
			
			markBmap(dir^1);
			
			if(iswaterex(MAPCOMBO(x,y+8), currmap, currscr, -1, x,y+8) && _walkflag(x,y+8,0,SWITCHBLOCK_STATE) && current_item(itype_flippers))
			{
				hopclk=0xFF;
				SetSwim();
				if (!IsSideSwim()) attackclk = charging = spins = 0;
			}
			else
			{
				action = none;
				FFCore.setLinkAction(none);
			}
			//preloaded freeform combos
			ffscript_engine(true);
			
			putscr(scrollbuf,0,0,tmpscr);
			putscrdoors(scrollbuf,0,0,tmpscr);
			
			if((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED))
			{
				reset_pal_cycling();
				putscr(scrollbuf,0,0,tmpscr);
				putscrdoors(scrollbuf,0,0,tmpscr);
				walkup(COOLSCROLL);
			}
			else if((type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D))
			{
				reset_pal_cycling();
				putscr(scrollbuf,0,0,tmpscr);
				putscrdoors(scrollbuf,0,0,tmpscr);
				walkdown2(COOLSCROLL);
			}
			else if(wtype==wtIWARPZAP)
			{
				zapin();
			}
			else if(wtype==wtIWARPWAVE)
			{
				wavyin();
			}
			else if(wtype==wtIWARPOPEN)
			{
				openscreen();
			}
			if(reposition_sword_postwarp)
			{
				weapon *swd=NULL;
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					swd = (weapon*)Lwpns.spr(i);
					
					if(swd->id == (attack==wSword ? wSword : wWand))
					{
						int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
						int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
						positionSword(swd,item_id);
						break;
					}
				}
			}
			show_subscreen_life=true;
			show_subscreen_numbers=true;
			playLevelMusic();
			currcset=DMaps[currdmap].color;
			dointro();
			setEntryPoints(x,y);
			break;
		}
		else
		{
			if(reposition_sword_postwarp)
			{
				weapon *swd=NULL;
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					swd = (weapon*)Lwpns.spr(i);
					
					if(swd->id == (attack==wSword ? wSword : wWand))
					{
						int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
						int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
						positionSword(swd,item_id);
						break;
					}
				}
			}
			didpit=false;
			update_subscreens();
			warp_sound = 0;
			is_warping = false;
			return false;
		}
	}
	default:
		didpit=false;
		update_subscreens();
		warp_sound = 0;
		is_warping = false;
		if(reposition_sword_postwarp)
		{
			weapon *swd=NULL;
			for(int32_t i=0; i<Lwpns.Count(); i++)
			{
				swd = (weapon*)Lwpns.spr(i);
				
				if(swd->id == (attack==wSword ? wSword : wWand))
				{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
				}
			}
		}
		return false;
	}
	
	
	
	// Stop Link from drowning!
	if(action==drowning || action==lavadrowning || action==sidedrowning)
	{
		drownclk=0;
		action=none; FFCore.setLinkAction(none);
	}
	
	int32_t checkwater = iswaterex(MAPCOMBO(x,y+(bigHitbox?8:12)), currmap, currscr, -1, x,y+(bigHitbox?8:12));
	// But keep him swimming if he ought to be!
	// Unless the water is too high levelled, in which case... well, he'll drown on transition probably anyways. -Dimi
	if(action!=rafting && checkwater && (_walkflag(x,y+(bigHitbox?8:12),0,SWITCHBLOCK_STATE) || get_bit(quest_rules,qr_DROWN))
			//&& (current_item(itype_flippers) >= combobuf[checkwater].attribytes[0]) 
		&& (action!=inwind))
	{
		hopclk=0xFF;
		SetSwim();
	}
	
	newscr_clk=frame;
	activated_timed_warp=false;
	eat_buttons();
	
	if(wtype!=wtIWARP)
		attackclk=0;
		
	didstuff=0;
	map_bkgsfx(true);
	loadside=dir^1;
	whistleclk=-1;
	
	if(z>0 && isSideViewLink())
	{
		y-=z;
		z=0;
	}
	else if(!isSideViewLink())
	{
		fall=0;
	}
	
	// If warping between top-down and sideview screens,
	// fix enemies that are carried over by Full Screen Warp
	const bool tmpscr_is_sideview = isSideViewLink();
	
	if(!wasSideview && tmpscr_is_sideview)
	{
		for(int32_t i=0; i<guys.Count(); i++)
		{
			if(guys.spr(i)->z > 0)
			{
				guys.spr(i)->y -= guys.spr(i)->z;
				guys.spr(i)->z = 0;
			}
			
			if(((enemy*)guys.spr(i))->family!=eeTRAP && ((enemy*)guys.spr(i))->family!=eeSPINTILE)
				guys.spr(i)->yofs += 2;
		}
	}
	else if(wasSideview && !tmpscr_is_sideview)
	{
		for(int32_t i=0; i<guys.Count(); i++)
		{
			if(((enemy*)guys.spr(i))->family!=eeTRAP && ((enemy*)guys.spr(i))->family!=eeSPINTILE)
				guys.spr(i)->yofs -= 2;
		}
	}
	
	if((DMaps[currdmap].type&dmfCONTINUE) || (currdmap==0))
	{
		if(dlevel)
		{
			int32_t wrx,wry;
			
			if(get_bit(quest_rules,qr_NOARRIVALPOINT))
			{
				wrx=tmpscr->warpreturnx[0];
				wry=tmpscr->warpreturny[0];
			}
			else
			{
				wrx=tmpscr->warparrivalx;
				wry=tmpscr->warparrivaly;
			}
			
			if((wtype == wtEXIT)
					|| (((wtype == wtSCROLL) && !intradmap) && ((wrx>0 || wry>0)||(get_bit(quest_rules,qr_WARPSIGNOREARRIVALPOINT)))))
			{
				if(!(wtype==wtSCROLL)||!(get_bit(quest_rules,qr_NOSCROLLCONTINUE)))
				{
					game->set_continue_scrn(homescr);
					//Z_message("continue_scrn = %02X e/e\n",game->get_continue_scrn());
				}
				else if(currdmap != game->get_continue_dmap())
				{
					game->set_continue_scrn(DMaps[currdmap].cont + DMaps[currdmap].xoff);
				}
			}
			else
			{
				if(currdmap != game->get_continue_dmap())
				{
					game->set_continue_scrn(DMaps[currdmap].cont + DMaps[currdmap].xoff);
					//Z_message("continue_scrn = %02X dlevel\n",game->get_continue_scrn());
				}
			}
		}
		else
		{
			game->set_continue_scrn(DMaps[currdmap].cont + DMaps[currdmap].xoff);
			//Z_message("continue_scrn = %02X\n !dlevel\n",game->get_continue_scrn());
		}
		
		game->set_continue_dmap(currdmap);
		lastentrance_dmap = currdmap;
		lastentrance = game->get_continue_scrn();
		//Z_message("continue_map = %d\n",game->get_continue_dmap());
	}
	
	if(tmpscr->flags4&fAUTOSAVE)
	{
		save_game(true,0);
	}
	
	if(tmpscr->flags6&fCONTINUEHERE)
	{
		lastentrance_dmap = currdmap;
		lastentrance = homescr;
	}
	
	update_subscreens();
	verifyBothWeapons();
	
	if(wtype==wtCAVE)
	{
		if(DMaps[currdmap].flags&dmfGUYCAVES)
			Z_eventlog("Entered %s containing %s.\n",DMaps[currdmap].flags&dmfCAVES ? "Cave" : "Item Cellar",
					   (char *)moduledata.roomtype_names[tmpscr[1].room]);
		else
			Z_eventlog("Entered %s.",DMaps[currdmap].flags&dmfCAVES ? "Cave" : "Item Cellar");
	}
	else Z_eventlog("Warped to DMap %d: %s, screen %d, via %s.\n", currdmap, DMaps[currdmap].name,currscr,
						wtype==wtPASS ? "Passageway" :
						wtype==wtEXIT ? "Entrance/Exit" :
						wtype==wtSCROLL ? "Scrolling Warp" :
						wtype==wtWHISTLE ? "Whistle Warp" :
						"Insta-Warp");
						
	eventlog_mapflags();
	if(reposition_sword_postwarp)
	{
		weapon *swd=NULL;
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			swd = (weapon*)Lwpns.spr(i);
			
			if(swd->id == (attack==wSword ? wSword : wWand))
			{
				int32_t itype = (attack==wFire ? itype_candle : attack==wCByrna ? itype_cbyrna : attack==wWand ? itype_wand : attack==wHammer ? itype_hammer : itype_sword);
				int32_t item_id = (directWpn>-1 && itemsbuf[directWpn].family==itype) ? directWpn : current_item_id(itype);
				positionSword(swd,item_id);
				break;
			}
		}
	}
	FFCore.init_combo_doscript();
	FFCore.initZScriptDMapScripts();
	FFCore.initZScriptActiveSubscreenScript();
	is_warping = false;
	return true;
}

void LinkClass::exitcave()
{
    stop_sfx(WAV_ER);
    currscr=homescr;
    loadscr(0,currdmap,currscr,255,false);                                   // bogus direction
    x = tmpscr->warpreturnx[0];
    y = tmpscr->warpreturny[0];
    
    if(didpit)
    {
        didpit=false;
        x=pitx;
        y=pity;
    }
    
    if(x+y == 0)
        x = y = 80;
        
    int32_t type1 = combobuf[MAPCOMBO(x,y-16)].type;
    int32_t type2 = combobuf[MAPCOMBO(x,y)].type;
    int32_t type3 = combobuf[MAPCOMBO(x,y+16)].type;
    bool b = COOLSCROLL &&
             ((type1==cCAVE) || (type1>=cCAVEB && type1<=cCAVED) ||
              (type2==cCAVE) || (type2>=cCAVEB && type2<=cCAVED) ||
              (type3==cCAVE2) || (type3>=cCAVE2B && type3<=cCAVE2D) ||
              (type2==cCAVE2) || (type2>=cCAVE2B && type2<=cCAVE2D));
    ALLOFF();
    blackscr(30,b?false:true);
    ringcolor(false);
    loadlvlpal(DMaps[currdmap].color);
    lighting(false, true);
    music_stop();
    kill_sfx();
    putscr(scrollbuf,0,0,tmpscr);
    putscrdoors(scrollbuf,0,0,tmpscr);
    
    if((type1==cCAVE)||(type1>=cCAVEB && type1<=cCAVED) || (type2==cCAVE)||(type2>=cCAVEB && type2<=cCAVED))
    {
        walkup(COOLSCROLL);
    }
    else if((type3==cCAVE2)||(type3>=cCAVE2B && type3<=cCAVE2D) || (type2==cCAVE2)||(type2>=cCAVE2B && type2<=cCAVE2D))
    {
        walkdown2(COOLSCROLL);
    }
    
    show_subscreen_life=true;
    show_subscreen_numbers=true;
    playLevelMusic();
    currcset=DMaps[currdmap].color;
    dointro();
    newscr_clk=frame;
    activated_timed_warp=false;
    dir=down;
    setEntryPoints(x,y);
    eat_buttons();
    didstuff=0;
    map_bkgsfx(true);
    loadside=dir^1;
}


void LinkClass::stepforward(int32_t steps, bool adjust)
{
	if ( FFCore.nostepforward ) return;
	if ( FFCore.temp_no_stepforward ) { FFCore.temp_no_stepforward = 0; return; }
    zfix tx=x;           //temp x
    zfix ty=y;           //temp y
    zfix tstep(0);        //temp single step distance
    zfix s(0);            //calculated step distance for all steps
    z3step=2;
    int32_t sh=shiftdir;
    shiftdir=-1;
    
    for(int32_t i=steps; i>0; --i)
    {
		if(diagonalMovement)
        {
			if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
			{
				tstep = 1.5;
			}
			else
			{
				tstep=z3step;
				z3step=(z3step%2)+1;
			}
        }
        else
        {
			if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT))
			{
				tstep = 1.5;
			}
			else
			{
				tstep=lsteps[int32_t((dir<left)?ty:tx)&7];
				
				switch(dir)
				{
				case up:
					ty-=tstep;
					break;
					
				case down:
					ty+=tstep;
					break;
					
				case left:
					tx-=tstep;
					break;
					
				case right:
					tx+=tstep;
					break;
				}
			}
        }
        
        s+=tstep;
    }
    
    z3step=2;
    
	x = x.getInt();
	y = y.getInt();
    while(s>=0)
    {
        if(diagonalMovement)
        {
            if((dir<left?x.getInt()&7:y.getInt()&7)&&adjust==true)
            {
				if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
				{
					walkable = false;
					shiftdir = -1;
					int32_t tdir=dir<left?(x.getInt()&8?left:right):(y.getInt()&8?down:up);
					switch(tdir)
					{
						case left:
							--x;
							break;
						case right:
							++x;
							break;
						case up:
							--y;
							break;
						case down:
							++y;
							break;
					}
				}
				else
				{
					walkable=false;
					shiftdir=dir<left?(x.getInt()&8?left:right):(y.getInt()&8?down:up);
					move(dir, 150);
				}
            }
            else
            {
				if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
				{
					s-=1.5;
				}
				else
				{
					s-=z3step;
				}
                walkable=true;
				move(dir, 150);
            }
            
            shiftdir=-1;
        }
        else
        {
			if((dir<left?x.getInt()&7:y.getInt()&7)&&adjust==true)
            {
                walkable=false;
                int32_t tdir=dir<left?(x.getInt()&8?left:right):(y.getInt()&8?down:up);
				switch(tdir)
				{
					case left:
						--x;
						break;
					case right:
						++x;
						break;
					case up:
						--y;
						break;
					case down:
						++y;
						break;
				}
            }
            else
			{
				if(get_bit(quest_rules, qr_NEW_HERO_MOVEMENT) || IsSideSwim())
				{
					s-=1.5;
				}
				else if(dir<left)
				{
					s-=lsteps[y.getInt()&7];
				}
				else
				{
					s-=lsteps[x.getInt()&7];
				}
				
				move(dir, 150);
			}
        }
        
        if(s<0)
        {
            // Not quite sure how this is actually supposed to work.
            // There have to be two cases for each direction or Link
            // either walks too far onto the screen or may get stuck
            // going through walk-through walls.
            switch(dir)
            {
            case up:
                if(y<8) // Leaving the screen
                    y+=s;
                else // Entering the screen
                    y-=s;
                    
                break;
                
            case down:
                if(y>152)
                    y-=s;
                else
                    y+=s;
                    
                break;
                
            case left:
                if(x<8)
                    x+=s;
                else
                    x-=s;
                    
                break;
                
            case right:
                if(x>=232)
                    x-=s;
                else
                    x+=s;
                    
                break;
            }
        }
        
	if ( get_bit(quest_rules,qr_SCRIPTSRUNINLINKSTEPFORWARD) )
	{
		if((!( FFCore.system_suspend[susptGLOBALGAME] )) && (g_doscript & (1<<GLOBAL_SCRIPT_GAME)) )
		{
			ZScriptVersion::RunScript(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
		}
		if ((!( FFCore.system_suspend[susptLINKACTIVE] )) && link_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255)
		{
			ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_ACTIVE, SCRIPT_LINK_ACTIVE);
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && dmap_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 ) 
		{
			ZScriptVersion::RunScript(SCRIPT_DMAP, DMaps[currdmap].script,currdmap);
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && passive_subscreen_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 ) 
		{
			ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script,currdmap);
		}
		if ( (!( FFCore.system_suspend[susptSCREENSCRIPTS] )) && tmpscr->script != 0 && tmpscr->doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_SCREEN, tmpscr->script, 0);    
		}
	}
	
        draw_screen(tmpscr);
	if (canSideviewLadder()) setOnSideviewLadder(true);
        advanceframe(true);
        
        if(Quit)
            return;
    }
	if(dir==right||dir==down)
	{
		x=int32_t(x);
		y=int32_t(y);
	}
	else
	{
		x = x.getInt();
		y = y.getInt();
	}
    setEntryPoints(x,y);
    draw_screen(tmpscr);
    eat_buttons();
    shiftdir=sh;
}

void LinkClass::walkdown(bool opening) //entering cave
{
    if(opening)
    {
        close_black_opening(x+8, y+8+playing_field_offset, false);
    }
    
    hclk=0;
    stop_item_sfx(itype_brang);
    sfx(WAV_STAIRS,pan(x.getInt()));
    clk=0;
    //  int32_t cmby=(y.getInt()&0xF0)+16;
    // Fix Link's position to the grid
    y=y.getInt()&0xF0;
    action=climbcoverbottom; FFCore.setLinkAction(climbcoverbottom);
    attack=wNone;
    attackid=-1;
    reset_swordcharge();
    climb_cover_x=x.getInt()&0xF0;
    climb_cover_y=(y.getInt()&0xF0)+16;
    
    guys.clear();
    chainlinks.clear();
    Lwpns.clear();
    Ewpns.clear();
    items.clear();
    
    for(int32_t i=0; i<64; i++)
    {
        linkstep();
        
        if(zinit.linkanimationstyle==las_zelda3 || zinit.linkanimationstyle==las_zelda3slow)
            link_count=(link_count+1)%16;
            
        if((i&3)==3)
            ++y;
            
        draw_screen(tmpscr);
        advanceframe(true);
        
        if(Quit)
            break;
    }
    
    action=none; FFCore.setLinkAction(none);
}

void LinkClass::walkdown2(bool opening) //exiting cave 2
{
    int32_t type = combobuf[MAPCOMBO(x,y)].type;
    
    if((type==cCAVE2)||(type>=cCAVE2B && type<=cCAVE2D))
        y-=16;
        
    dir=down;
    // Fix Link's position to the grid
    y=y.getInt()&0xF0;
    z=fall=0;
    
    if(opening)
    {
        open_black_opening(x+8, y+8+playing_field_offset+16, false);
    }
    
    hclk=0;
    stop_item_sfx(itype_brang);
    sfx(WAV_STAIRS,pan(x.getInt()));
    clk=0;
    //  int32_t cmby=y.getInt()&0xF0;
    action=climbcovertop; FFCore.setLinkAction(climbcovertop);
    attack=wNone;
    attackid=-1;
    reset_swordcharge();
    climb_cover_x=x.getInt()&0xF0;
    climb_cover_y=y.getInt()&0xF0;
    
    guys.clear();
    chainlinks.clear();
    Lwpns.clear();
    Ewpns.clear();
    items.clear();
    
    for(int32_t i=0; i<64; i++)
    {
        linkstep();
        
        if(zinit.linkanimationstyle==las_zelda3 || zinit.linkanimationstyle==las_zelda3slow)
            link_count=(link_count+1)%16;
            
        if((i&3)==3)
            ++y;
            
        draw_screen(tmpscr);
        advanceframe(true);
        
        if(Quit)
            break;
    }
    
    action=none; FFCore.setLinkAction(none);
}

void LinkClass::walkup(bool opening) //exiting cave
{
    int32_t type = combobuf[MAPCOMBO(x,y)].type;
    
    if((type==cCAVE)||(type>=cCAVEB && type<=cCAVED))
        y+=16;
        
    // Fix Link's position to the grid
    y=y.getInt()&0xF0;
    z=fall=0;
    
    if(opening)
    {
        open_black_opening(x+8, y+8+playing_field_offset-16, false);
    }
    
    hclk=0;
    stop_item_sfx(itype_brang);
    sfx(WAV_STAIRS,pan(x.getInt()));
    dir=down;
    clk=0;
    //  int32_t cmby=y.getInt()&0xF0;
    action=climbcoverbottom; FFCore.setLinkAction(climbcoverbottom);
    attack=wNone;
    attackid=-1;
    reset_swordcharge();
    climb_cover_x=x.getInt()&0xF0;
    climb_cover_y=y.getInt()&0xF0;
    
    guys.clear();
    chainlinks.clear();
    Lwpns.clear();
    Ewpns.clear();
    items.clear();
    
    for(int32_t i=0; i<64; i++)
    {
        linkstep();
        
        if(zinit.linkanimationstyle==las_zelda3 || zinit.linkanimationstyle==las_zelda3slow)
            link_count=(link_count+1)%16;
            
        if((i&3)==0)
            --y;
            
        draw_screen(tmpscr);
        advanceframe(true);
        
        if(Quit)
            break;
    }
    
    map_bkgsfx(true);
    loadside=dir^1;
    action=none; FFCore.setLinkAction(none);
}

void LinkClass::walkup2(bool opening) //entering cave2
{
    if(opening)
    {
        close_black_opening(x+8, y+8+playing_field_offset, false);
    }
    
    hclk=0;
    stop_item_sfx(itype_brang);
    sfx(WAV_STAIRS,pan(x.getInt()));
    dir=up;
    clk=0;
    //  int32_t cmby=y.getInt()&0xF0;
    action=climbcovertop; FFCore.setLinkAction(climbcovertop);
    attack=wNone;
    attackid=-1;
    reset_swordcharge();
    climb_cover_x=x.getInt()&0xF0;
    climb_cover_y=(y.getInt()&0xF0)-16;
    
    guys.clear();
    chainlinks.clear();
    Lwpns.clear();
    Ewpns.clear();
    items.clear();
    
    for(int32_t i=0; i<64; i++)
    {
        linkstep();
        
        if(zinit.linkanimationstyle==las_zelda3 || zinit.linkanimationstyle==las_zelda3slow)
            link_count=(link_count+1)%16;
            
        if((i&3)==0)
            --y;
            
        draw_screen(tmpscr);
        advanceframe(true);
        
        if(Quit)
            break;
    }
    
    map_bkgsfx(true);
    loadside=dir^1;
    action=none; FFCore.setLinkAction(none);
}

void LinkClass::stepout() // Step out of item cellars and passageways
{
    int32_t sc = specialcave; // This gets erased by ALLOFF()
    ALLOFF();
    stop_sfx(WAV_ER);
    map_bkgsfx(false);
    kill_enemy_sfx();
    draw_screen(tmpscr,false);
    fade(sc>=GUYCAVE?10:11,true,false);
    blackscr(30,true);
    ringcolor(false);
    
    if(sc==PASSAGEWAY && abs(x-warpx)>16) // How did Link leave the passageway?
    {
        currdmap=stepoutdmap;
        currmap=DMaps[currdmap].map;
        dlevel=DMaps[currdmap].level;
        
        //we might have just left a passage, so be sure to update the CSet record -DD
        currcset=DMaps[currdmap].color;
        
        init_dmap();
        homescr=stepoutscr;
    }
    
    currscr=homescr;
    loadscr(0,currdmap,currscr,255,false);                                   // bogus direction
    draw_screen(tmpscr,false);
    
    if((tmpscr->flags&fDARK) == 0)
    {
        darkroom = naturaldark = false;
        fade(DMaps[currdmap].color,true,true);
    }
    else
    {
        darkroom = naturaldark = true;
        
        if(get_bit(quest_rules,qr_FADE))
        {
            interpolatedfade();
        }
        else
        {
            loadfadepal((DMaps[currdmap].color)*pdLEVEL+poFADE3);
        }
	byte *si = colordata + CSET(DMaps[currdmap].color*pdLEVEL+poLEVEL)*3;
	si+=3*48;
	    
	for(int32_t i=0; i<16; i++)
	{
		RAMpal[CSET(9)+i] = _RGB(si);
		tempgreypal[CSET(9)+i] = _RGB(si); //preserve monochrome
		si+=3;
	}
    }
    
    x = tmpscr->warpreturnx[stepoutwr];
    y = tmpscr->warpreturny[stepoutwr];
    
    if(didpit)
    {
        didpit=false;
        x=pitx;
        y=pity;
    }
    
    if(x+y == 0)
        x = y = 80;
        
    dir=down;
    
    setEntryPoints(x,y);
    
    // Let's use the 'exit cave' animation if we entered this cellar via a cave combo.
    int32_t type = combobuf[MAPCOMBO(tmpscr->warpreturnx[stepoutwr],tmpscr->warpreturny[stepoutwr])].type;
    
    if((type==cCAVE)||(type>=cCAVEB && type<=cCAVED))
    {
        walkup(false);
    }
    else if((type==cCAVE2)||(type>=cCAVE2B && type<=cCAVE2D))
    {
        walkdown2(false);
    }
    
    newscr_clk=frame;
    activated_timed_warp=false;
    didstuff=0;
    eat_buttons();
    markBmap(-1);
    map_bkgsfx(true);
    
    if(!get_bit(quest_rules, qr_CAVEEXITNOSTOPMUSIC))
    {
        music_stop();
        playLevelMusic();
    }
	else if(get_bit(quest_rules,qr_SCREEN80_OWN_MUSIC))
	{
		playLevelMusic();
	}
    
    loadside=dir^1;
}

bool LinkClass::nextcombo_wf(int32_t d2)
{
    if(toogam || (action!=swimming && !IsSideSwim() && action != swimhit) || hopclk==0) //!DIMITODO: ...does swimming just let you ignore smart scrolling entirely!?
        return false;
        
    // assumes Link is about to scroll screens
    
    int32_t ns = nextscr(d2);
    
    if(ns==0xFFFF)
        return false;
        
    // want actual screen index, not game->maps[] index
    ns = (ns&127) + (ns>>7)*MAPSCRS;
    
    int32_t cx = x;
    int32_t cy = y;
    
    switch(d2)
    {
    case up:
        cy=160;
        break;
        
    case down:
        cy=0;
        break;
        
    case left:
        cx=240;
        break;
        
    case right:
        cx=0;
        break;
    }
    
    // check lower half of combo
    cy += 8;
    
    // from MAPCOMBO()
    int32_t cmb = (cy&0xF0)+(cx>>4);
    
    if(cmb>175)
        return true;
        
    newcombo c = combobuf[TheMaps[ns].data[cmb]];
    bool dried = iswater_type(c.type) && DRIEDLAKE;
    bool swim = iswater_type(c.type) && (current_item(itype_flippers)) && !dried;
    int32_t b=1;
    
    if(cx&8) b<<=2;
    
    if(cy&8) b<<=1;
    
    if((c.walk&b) && !dried && !swim)
        return true;
        
    // next block (i.e. cnt==2)
    if(!(cx&8))
    {
        b<<=2;
    }
    else
    {
        c = combobuf[TheMaps[ns].data[++cmb]];
        dried = iswater_type(c.type) && DRIEDLAKE;
        swim = iswater_type(c.type) && (current_item(itype_flippers)) && !dried;
        b=1;
        
        if(cy&8)
        {
            b<<=1;
        }
    }
    
    return (c.walk&b) ? !dried && !swim : false;
}

bool LinkClass::nextcombo_solid(int32_t d2)
{
	if(toogam || currscr>=128)
		return false;
		
	// assumes Link is about to scroll screens
	
	int32_t ns = nextscr(d2);
	
	if(ns==0xFFFF)
		return false;
		
	// want actual screen index, not game->maps[] index
	ns = (ns&127) + (ns>>7)*MAPSCRS;
	int32_t screen = (ns%MAPSCRS);
	int32_t map = (ns - screen) / MAPSCRS;
	
	int32_t cx = x;
	int32_t cy = y;
	
	switch(d2)
	{
	case up:
		cy=160;
		break;
		
	case down:
		cy=0;
		break;
		
	case left:
		cx=240;
		break;
		
	case right:
		cx=0;
		break;
	}
	
	if(d2==up) cy += 8;
	
	if(d2==left||d2==right) cy+=bigHitbox?0:8;
	
	int32_t initcx = cx;
	int32_t initcy = cy;
	// from MAPCOMBO()
	
	for(int32_t i=0; i<=((bigHitbox&&!(d2==up||d2==down))?((initcy&7)?2:1):((initcy&7)?1:0)) && cy < 176; cy+=(cy%2)?7:8,i++)
	{
		cx = initcx;
		for(int32_t k=0; k<=(get_bit(quest_rules, qr_SMARTER_SMART_SCROLL)?((initcx&7)?2:1):0) && cx < 256; cx+=(cx%2)?7:8,k++)
		{
			int32_t cmb = (cy&0xF0)+(cx>>4);
			
			if(cmb>175)
			{
				return true;
			}
			
			newcombo const& c = combobuf[MAPCOMBO3(map, screen, -1,cx,cy, get_bit(quest_rules, qr_SMARTER_SMART_SCROLL))];
		
			int32_t b=1;
			
			if(cx&8) b<<=2;
			
			if(cy&8) b<<=1;
		
			//bool bridgedetected = false;
		
			int32_t walk = c.walk;
			if (get_bit(quest_rules, qr_SMARTER_SMART_SCROLL))
			{
				for (int32_t m = 0; m <= 1; m++)
				{
					newcombo const& cmb = combobuf[MAPCOMBO3(map, screen, m,cx,cy, true)];
					if (cmb.type == cBRIDGE) 
					{
						walk &= cmb.walk;
					}
					else walk |= cmb.walk;
				}
			}
			/*
			if (bridgedetected)
			{
				continue;
			}*/
			
			//bool swim = iswater_type(c.type) && (current_item(itype_flippers) || action==rafting);
			bool swim = iswaterex(MAPCOMBO3(map, screen, -1,cx,cy, get_bit(quest_rules, qr_SMARTER_SMART_SCROLL)), map, screen, -1, cx, cy, true, false, true) && (current_item(itype_flippers) || action==rafting);
			
			if((walk&b) && !swim)
			{
				return true;
			}
		}
		
		/*
		#if 0
		
		//
		// next block (i.e. cnt==2)
		if(!(cx&8))
		{
			b<<=2;
		}
		else
		{
			c = combobuf[TheMaps[ns].data[++cmb]];
			dried = iswater_type(c.type) && DRIEDLAKE;
			//swim = iswater_type(c.type) && (current_item(itype_flippers));
			b=1;
			
			if(cy&8)
			{
				b<<=1;
			}
		}
	
		swim = iswaterex(c, map, screen, -1, cx+8, cy, true, false, true) && current_item(itype_flippers);
		
		if((c.walk&b) && !dried && !swim)
		{
			return true;
		}
		
		cx+=8;
		
		if(cx&7)
		{
			if(!(cx&8))
			{
				b<<=2;
			}
			else
			{
				c = combobuf[TheMaps[ns].data[++cmb]];
				dried = iswater_type(c.type) && DRIEDLAKE;
				//swim = iswaterex(cmb, map, screen, -1, cx+8, cy, true, false, true) && current_item(itype_flippers);
				b=1;
				
				if(cy&8)
				{
					b<<=1;
				}
			}
		
		swim = iswaterex(c, map, screen, -1, cx+8, cy, true, false, true) && current_item(itype_flippers);
			
			if((c.walk&b) && !dried && !swim)
				return true;
		}
			
		#endif
		*/
	}
	
	return false;
}

void LinkClass::checkscroll()
{
    //DO NOT scroll if Link is vibrating due to Farore's Wind effect -DD
    if(action == casting||action==sideswimcasting)
        return;
        
    if(toogam)
    {
        if(x<0 && (currscr&15)==0) x=0;
        
        if(y<0 && currscr<16) y=0;
        
        if(x>240 && (currscr&15)==15) x=240;
        
        if(y>160 && currscr>=112) y=160;
    }
    
    if(y<0)
    {
        bool doit=true;
        y=0;
		
		if((z > 0 || stomping) && get_bit(quest_rules, qr_NO_SCROLL_WHILE_IN_AIR))
			doit = false;
		
        if(nextcombo_wf(up))
            doit=false;
            
        if(get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE))&&action!=inwind &&action!=scrolling && !(tmpscr->flags2&wfUP))
        {
            if(nextcombo_solid(up))
                doit=false;
        }
        
        if(doit || action==inwind)
        {
            if(currscr>=128)
            {
                if(specialcave >= GUYCAVE)
                    exitcave();
                else stepout();
            }
            else if(action==inwind)
            {
                if(DMaps[currdmap].flags&dmfWHIRLWINDRET)
                {
                    action=none; FFCore.setLinkAction(none);
                    restart_level();
                }
                else
                {
                    dowarp(2,up);
                }
            }
            else if(tmpscr->flags2&wfUP && (!(tmpscr->flags8&fMAZEvSIDEWARP) || checkmaze(tmpscr,false)))
            {
                sdir=up;
                dowarp(1,(tmpscr->sidewarpindex)&3);
            }
            else if(!edge_of_dmap(up))
            {
				scrolling_map = currmap;
                scrollscr(up);
                
                if(tmpscr->flags4&fAUTOSAVE)
                {
                    save_game(true,0);
                }
                
                if(tmpscr->flags6&fCONTINUEHERE)
                {
                    lastentrance_dmap = currdmap;
                    lastentrance = homescr;
                }
            }
        }
    }
    
    if(y>160)
    {
        bool doit=true;
        y=160;
		
		if((z > 0 || stomping) && get_bit(quest_rules, qr_NO_SCROLL_WHILE_IN_AIR))
			doit = false;
		
        if(nextcombo_wf(down))
            doit=false;
            
        if(get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE))&&action!=inwind &&action!=scrolling &&!(tmpscr->flags2&wfDOWN))
        {
            if(nextcombo_solid(down))
                doit=false;
        }
        
        if(doit || action==inwind)
        {
            if(currscr>=128)
            {
                if(specialcave >= GUYCAVE)
                    exitcave();
                else stepout();
            }
            else if(action==inwind)
            {
                if(DMaps[currdmap].flags&dmfWHIRLWINDRET)
                {
                    action=none; FFCore.setLinkAction(none);
                    restart_level();
                }
                else
                {
                    dowarp(2,down);
                }
            }
            else if(tmpscr->flags2&wfDOWN && (!(tmpscr->flags8&fMAZEvSIDEWARP) || checkmaze(tmpscr,false)))
            {
                sdir=down;
                dowarp(1,(tmpscr->sidewarpindex>>2)&3);
            }
            else if(!edge_of_dmap(down))
            {
				scrolling_map = currmap;
                scrollscr(down);
                
                if(tmpscr->flags4&fAUTOSAVE)
                {
                    save_game(true,0);
                }
                
                if(tmpscr->flags6&fCONTINUEHERE)
                {
                    lastentrance_dmap = currdmap;
                    lastentrance = homescr;
                }
            }
        }
    }
    
    if(x<0)
    {
        bool doit=true;
        x=0;
		
		if((z > 0 || stomping) && get_bit(quest_rules, qr_NO_SCROLL_WHILE_IN_AIR))
			doit = false;
		
        if(nextcombo_wf(left))
            doit=false;
            
        if(get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE))&&action!=inwind &&action!=scrolling &&!(tmpscr->flags2&wfLEFT))
        {
            if(nextcombo_solid(left))
                doit=false;
        }
        
        if(doit || action==inwind)
        {
            if(currscr>=128)
            {
                if(specialcave >= GUYCAVE)
                    exitcave();
                else stepout();
            }
            
            if(action==inwind)
            {
                if(DMaps[currdmap].flags&dmfWHIRLWINDRET)
                {
                    action=none; FFCore.setLinkAction(none);
                    restart_level();
                }
                else
                {
                    dowarp(2,left);
                }
            }
            else if(tmpscr->flags2&wfLEFT && (!(tmpscr->flags8&fMAZEvSIDEWARP) || checkmaze(tmpscr,false)))
            {
                sdir=left;
                dowarp(1,(tmpscr->sidewarpindex>>4)&3);
            }
            else if(!edge_of_dmap(left))
            {
				scrolling_map = currmap;
                scrollscr(left);
                
                if(tmpscr->flags4&fAUTOSAVE)
                {
                    save_game(true,0);
                }
                
                if(tmpscr->flags6&fCONTINUEHERE)
                {
                    lastentrance_dmap = currdmap;
                    lastentrance = homescr;
                }
            }
        }
    }
    
    if(x>240)
    {
        bool doit=true;
        x=240;
		
		if((z > 0 || stomping) && get_bit(quest_rules, qr_NO_SCROLL_WHILE_IN_AIR))
			doit = false;
		
        if(nextcombo_wf(right))
            doit=false;
            
        if(get_bit(quest_rules, qr_SMARTSCREENSCROLL)&&(!(tmpscr->flags&fMAZE))&&action!=inwind &&action!=scrolling &&!(tmpscr->flags2&wfRIGHT))
        {
            if(nextcombo_solid(right))
                doit=false;
        }
        
        if(doit || action==inwind)
        {
            if(currscr>=128)
            {
                if(specialcave >= GUYCAVE)
                    exitcave();
                else stepout();
            }
            
            if(action==inwind)
            {
                if(DMaps[currdmap].flags&dmfWHIRLWINDRET)
                {
                    action=none; FFCore.setLinkAction(none);
                    restart_level();
                }
                else
                {
                    dowarp(2,right);
                }
            }
            else if(tmpscr->flags2&wfRIGHT && (!(tmpscr->flags8&fMAZEvSIDEWARP) || checkmaze(tmpscr,false)))
            {
                sdir=right;
                dowarp(1,(tmpscr->sidewarpindex>>6)&3);
            }
            else if(!edge_of_dmap(right))
            {
				scrolling_map = currmap;
                scrollscr(right);
                
                if(tmpscr->flags4&fAUTOSAVE)
                {
                    save_game(true,0);
                }
                
                if(tmpscr->flags6&fCONTINUEHERE)
                {
                    lastentrance_dmap = currdmap;
                    lastentrance = homescr;
                }
            }
        }
    }
}

// assumes current direction is in lastdir[3]
// compares directions with scr->path and scr->exitdir
bool LinkClass::checkmaze(mapscr *scr, bool sound)
{
    if(!(scr->flags&fMAZE))
        return true;
        
    if(lastdir[3]==scr->exitdir)
        return true;
        
    for(int32_t i=0; i<4; i++)
        if(lastdir[i]!=scr->path[i])
            return false;
            
    if(sound)
        sfx(scr->secretsfx);
        
    return true;
}

bool LinkClass::edge_of_dmap(int32_t side)
{
    if(checkmaze(tmpscr,false)==false)
        return false;
        
    // needs fixin'
    // should check dmap style
    switch(side)
    {
    case up:
        return currscr<16;
        
    case down:
        return currscr>=112;
        
    case left:
        if((currscr&15)==0)
            return true;
            
        if((DMaps[currdmap].type&dmfTYPE)!=dmOVERW)
            //    if(dlevel)
            return (((currscr&15)-DMaps[currdmap].xoff)<=0);
            
        break;
        
    case right:
        if((currscr&15)==15)
            return true;
            
        if((DMaps[currdmap].type&dmfTYPE)!=dmOVERW)
            //    if(dlevel)
            return (((currscr&15)-DMaps[currdmap].xoff)>=7);
            
        break;
    }
    
    return false;
}

bool LinkClass::lookaheadraftflag(int32_t d2)
{
    // Helper for scrollscr that gets next combo on next screen.
    // Can use destscr for scrolling warps,
    // but assumes currmap is correct.
    
    int32_t cx = x;
    int32_t cy = y + 8;
	
	bound(cx, 0, 240); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	bound(cy, 0, 168); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	//y+8 could be 168  //Attempt to fix a frash where scrolling through the lower-left corner could crassh ZC as reported by Lut. -Z
	//Applying this here, too. -Z
    
    switch(d2)
    {
    case up:
        cy=160;
        break;
        
    case down:
        cy=0;
        break;
        
    case left:
        cx=240;
        break;
        
    case right:
        cx=0;
        break;
    }
    
    int32_t combo = (cy&0xF0)+(cx>>4);
    
    if(combo>175)
        return 0;
    return ( isRaftFlag(combobuf[tmpscr[0].data[combo]].flag) || isRaftFlag(tmpscr[0].sflag[combo]));
    
}
int32_t LinkClass::lookahead(int32_t d2)                       // Helper for scrollscr that gets next combo on next screen.
{
    // Can use destscr for scrolling warps,
    // but assumes currmap is correct.
    
	int32_t cx = vbound(x,0,240); //var = vbound(val, n1, n2), not bound(var, n1, n2) -Z
	int32_t cy = vbound(y + 8,0,160);
	//bound(cx, 0, 240); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	//bound(cy, 0, 168); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	//y+8 could be 168 //Attempt to fix a frash where scrolling through the lower-left corner could crassh ZC as reported by Lut. -Z
    switch(d2)
    {
    case up:
        cy=160;
        break;
        
    case down:
        cy=0;
        break;
        
    case left:
        cx=240;
        break;
        
    case right:
        cx=0;
        break;
    }
    
    int32_t combo = (cy&0xF0)+(cx>>4);
    
    if(combo>175)
        return 0;
        
    return tmpscr[0].data[combo];            // entire combo code
}

int32_t LinkClass::lookaheadflag(int32_t d2)
{
    // Helper for scrollscr that gets next combo on next screen.
    // Can use destscr for scrolling warps,
    // but assumes currmap is correct.
    
    int32_t cx = vbound(x,0,240);
    int32_t cy = vbound(y + 8,0,160);
	
	//bound(cx, 0, 240); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	//bound(cy, 0, 168); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
	//y+8 could be 168  //Attempt to fix a frash where scrolling through the lower-left corner could crassh ZC as reported by Lut. -Z
	//Applying this here, too. -Z
    
    switch(d2)
    {
    case up:
        cy=160;
        break;
        
    case down:
        cy=0;
        break;
        
    case left:
        cx=240;
        break;
        
    case right:
        cx=0;
        break;
    }
    
    int32_t combo = (cy&0xF0)+(cx>>4);
    
    if(combo>175)
        return 0;
        
    if(!tmpscr[0].sflag[combo])
    {
        return combobuf[tmpscr[0].data[combo]].flag;           // flag
    }
    
    return tmpscr[0].sflag[combo];           // flag
}

//Bit of a messy kludge to give the correct Link->X/Link->Y in the script
void LinkClass::run_scrolling_script(int32_t scrolldir, int32_t cx, int32_t sx, int32_t sy, bool end_frames, bool waitdraw)
{
	// For rafting (and possibly other esoteric things)
	// Link's action should remain unchanged while scrolling,
	// but for the sake of scripts, here's an eye-watering kludge.
	actiontype lastaction = action;
	action=scrolling; FFCore.setLinkAction(scrolling);
	
	zfix storex = x, storey = y;
	switch(scrolldir)
	{
	case up:
		if(y < 160) y = 176;
		else if(cx > 0 && !end_frames) y = sy + 156;
		else y = 160;
		
		break;
		
	case down:
		if(y > 0) y = -16;
		else if(cx > 0 && !end_frames) y = sy - 172;
		else y = 0;
		
		break;
		
	case left:
		if(x < 240) x = 256;
		else if(cx > 0) x = sx + 236;
		else x = 240;
		
		break;
		
	case right:
		if(x > 0) x = -16;
		else if(cx > 0)	x = sx - 252;
		else x = 0;
		
		break;
	}
	if(waitdraw)
	{
		if((!( FFCore.system_suspend[susptGLOBALGAME] )) && (global_wait & (1<<GLOBAL_SCRIPT_GAME)))
		{
			ZScriptVersion::RunScript(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
			global_wait&= ~(1<<GLOBAL_SCRIPT_GAME);
		}
		if ( !FFCore.system_suspend[susptITEMSCRIPTENGINE] )
		{
			FFCore.itemScriptEngineOnWaitdraw();
		}
		if ( (!( FFCore.system_suspend[susptLINKACTIVE] )) && link_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_ACTIVE, SCRIPT_LINK_ACTIVE);
			link_waitdraw = false;
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && dmap_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_DMAP, DMaps[currdmap].script,currdmap);
			dmap_waitdraw = false;
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && passive_subscreen_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script,currdmap);
			passive_subscreen_waitdraw = false;
		}
		//if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && active_subscreen_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		//{
		//	ZScriptVersion::RunScript(SCRIPT_SUBSCREEN, DMaps[currdmap].activesubscript,currdmap);
		//	passive_subscreen_waitdraw = false;
		//}
		//no doscript check here, becauseb of preload? Do we want to write doscript here? -Z 13th July, 2019
		if ( (!( FFCore.system_suspend[susptSCREENSCRIPTS] )) && tmpscr->script != 0 && tmpscr->screen_waitdraw && tmpscr->preloadscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_SCREEN, tmpscr->script, 0);  
			tmpscr->screen_waitdraw = 0;		
		}
	}
	else
	{
		if((!( FFCore.system_suspend[susptGLOBALGAME] )) && (g_doscript & (1<<GLOBAL_SCRIPT_GAME)))
		{
			ZScriptVersion::RunScript(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
		}
		if ( !FFCore.system_suspend[susptITEMSCRIPTENGINE] )
		{
			FFCore.itemScriptEngine();
		}
		if ((!( FFCore.system_suspend[susptLINKACTIVE] )) && link_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255)
		{
			ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_ACTIVE, SCRIPT_LINK_ACTIVE);
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && dmap_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 ) 
		{
			ZScriptVersion::RunScript(SCRIPT_DMAP, DMaps[currdmap].script,currdmap);
		}
		if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && passive_subscreen_doscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 ) 
		{
			ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script,currdmap);
		}
		if ( (!( FFCore.system_suspend[susptSCREENSCRIPTS] )) && tmpscr->script != 0 && tmpscr->preloadscript && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
		{
			ZScriptVersion::RunScript(SCRIPT_SCREEN, tmpscr->script, 0);
		}
	}
	
	x = storex, y = storey;
	
	action=lastaction; FFCore.setLinkAction(lastaction);
}

//Has solving the maze enabled a side warp?
//Only used just before scrolling screens
// Note: since scrollscr() calls this, and dowarp() calls scrollscr(),
// return true to abort the topmost scrollscr() call. -L
bool LinkClass::maze_enabled_sizewarp(int32_t scrolldir)
{
    for(int32_t i = 0; i < 3; i++) lastdir[i] = lastdir[i+1];
    
    lastdir[3] = tmpscr->flags&fMAZE ? scrolldir : 0xFF;
    
    if(tmpscr->flags8&fMAZEvSIDEWARP && tmpscr->flags&fMAZE && scrolldir != tmpscr->exitdir)
    {
        switch(scrolldir)
        {
        case up:
            if(tmpscr->flags2&wfUP && checkmaze(tmpscr,true))
            {
                lastdir[3] = 0xFF;
                sdir=up;
                dowarp(1,(tmpscr->sidewarpindex)&3);
                return true;
            }
            
            break;
            
        case down:
            if(tmpscr->flags2&wfDOWN && checkmaze(tmpscr,true))
            {
                lastdir[3] = 0xFF;
                sdir=down;
                dowarp(1,(tmpscr->sidewarpindex>>2)&3);
                return true;
            }
            
            break;
            
        case left:
            if(tmpscr->flags2&wfLEFT && checkmaze(tmpscr,true))
            {
                lastdir[3] = 0xFF;
                sdir=left;
                dowarp(1,(tmpscr->sidewarpindex>>4)&3);
                return true;
            }
            
            break;
            
        case right:
            if(tmpscr->flags2&wfRIGHT && checkmaze(tmpscr,true))
            {
                lastdir[3] = 0xFF;
                sdir=right;
                dowarp(1,(tmpscr->sidewarpindex)&3);
                return true;
            }
            
            break;
        }
    }
    
    return false;
}

int32_t LinkClass::get_scroll_step(int32_t scrolldir)
{
	// For side-scrollers, where the relative speed of 'fast' scrolling is a bit slow.
	if(get_bit(quest_rules, qr_VERYFASTSCROLLING))
		return 16;

    if(get_bit(quest_rules, qr_SMOOTHVERTICALSCROLLING) != 0)
    {
        return (isdungeon() && !get_bit(quest_rules,qr_FASTDNGN)) ? 2 : 4;
    }
    else
    {
        if(scrolldir == up || scrolldir == down)
        {
            return 8;
        }
        else
        {
            return (isdungeon() && !get_bit(quest_rules,qr_FASTDNGN)) ? 2 : 4;
        }
    }
}

int32_t LinkClass::get_scroll_delay(int32_t scrolldir)
{
	if(get_bit(quest_rules, qr_NOSCROLL))
		return 0;
        
	if( (get_bit(quest_rules, qr_VERYFASTSCROLLING) != 0) ||
	    (get_bit(quest_rules, qr_SMOOTHVERTICALSCROLLING) != 0) )
	{
		return 1;
	}
	else
	{
		if(scrolldir == up || scrolldir == down)
		{
			return (isdungeon() && !get_bit(quest_rules,qr_FASTDNGN)) ? 4 : 2;
		}
		else
		{
			return 1;
		}
	}
}

void LinkClass::calc_darkroom_link(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t itemid = current_item_id(itype_lantern);
	if(itemid < 0) return; //no lantern light circle
	int32_t hx1 = x.getInt() - x1 + 8;
	int32_t hy1 = y.getInt() - y1 + 8;
	int32_t hx2 = x.getInt() - x2 + 8;
	int32_t hy2 = y.getInt() - y2 + 8;
	
	itemdata& lamp = itemsbuf[itemid];
	switch(lamp.misc1) //Shape
	{
		case 0: //Circle
			doDarkroomCircle(hx1, hy1, lamp.misc2, darkscr_bmp_curscr);
			doDarkroomCircle(hx2, hy2, lamp.misc2, darkscr_bmp_scrollscr);
			break;
		case 1: //Lamp Cone
			doDarkroomCone(hx1, hy1, lamp.misc2, dir, darkscr_bmp_curscr);
			doDarkroomCone(hx2, hy2, lamp.misc2, dir, darkscr_bmp_scrollscr);
			break;
	}
}

void LinkClass::scrollscr(int32_t scrolldir, int32_t destscr, int32_t destdmap)
{
	if(action==freeze||action==sideswimfreeze)
	{
		return;
	}
	
	bool overlay = false;
	
	if(scrolldir >= 0 && scrolldir <= 3)
	{
		overlay = get_bit(&tmpscr[(currscr < 128) ? 0 : 1].sidewarpoverlayflags, scrolldir) ? true : false;
	}
	
	if(destdmap == -1)
	{
		if(ZCMaps[currmap].tileWidth  != ZCMaps[DMaps[currdmap].map].tileWidth
				|| ZCMaps[currmap].tileHeight != ZCMaps[DMaps[currdmap].map].tileHeight)
			return;
	}
	else
	{
		if(ZCMaps[currmap].tileWidth  != ZCMaps[DMaps[destdmap].map].tileWidth
				|| ZCMaps[currmap].tileHeight != ZCMaps[DMaps[destdmap].map].tileHeight)
			return;
	}
	
	if(maze_enabled_sizewarp(scrolldir))  // dowarp() was called
		return;
		
	kill_enemy_sfx();
	stop_sfx(WAV_ER);
	screenscrolling = true;
	FFCore.ScrollingData[SCROLLDATA_DIR] = scrolldir;
	switch(scrolldir)
	{
		case up:
			FFCore.ScrollingData[SCROLLDATA_NX] = 0;
			FFCore.ScrollingData[SCROLLDATA_NY] = -176;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			break;
		case down:
			FFCore.ScrollingData[SCROLLDATA_NX] = 0;
			FFCore.ScrollingData[SCROLLDATA_NY] = 176;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			break;
		case left:
			FFCore.ScrollingData[SCROLLDATA_NX] = -256;
			FFCore.ScrollingData[SCROLLDATA_NY] = 0;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			break;
		case right:
			FFCore.ScrollingData[SCROLLDATA_NX] = 256;
			FFCore.ScrollingData[SCROLLDATA_NY] = 0;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			break;
	}
	FFCore.init_combo_doscript();
	tmpscr[1] = tmpscr[0];
	
	const int32_t _mapsSize = ZCMaps[currmap].tileWidth * ZCMaps[currmap].tileHeight;
	tmpscr[1].data.resize(_mapsSize, 0);
	tmpscr[1].sflag.resize(_mapsSize, 0);
	tmpscr[1].cset.resize(_mapsSize, 0);
	
	for(int32_t i = 0; i < 6; i++)
	{
		tmpscr3[i] = tmpscr2[i];
		tmpscr3[1].data.resize(_mapsSize, 0);
		tmpscr3[1].sflag.resize(_mapsSize, 0);
		tmpscr3[1].cset.resize(_mapsSize, 0);
	}
	
	conveyclk = 2;
	
	mapscr *newscr = &tmpscr[0];
	mapscr *oldscr = &tmpscr[1];
	
	//scroll x, scroll y, old screen x, old screen y, new screen x, new screen y
	int32_t sx = 0, sy = 0, tx = 0, ty = 0, tx2 = 0, ty2 = 0;
	int32_t cx = 0;
	int32_t step = get_scroll_step(scrolldir);
	int32_t delay = get_scroll_delay(scrolldir);
	bool end_frames = false;
	
	int32_t scx = get_bit(quest_rules,qr_FASTDNGN) ? 30 : 0;
	if(get_bit(quest_rules, qr_VERYFASTSCROLLING)) //just a minor adjustment.

	  scx = 32; //for sideview very fast screolling. 
  
	actiontype lastaction = action;
	ALLOFF(false, false);
	// for now, restore Link's previous action
	action=lastaction; FFCore.setLinkAction(lastaction);
	
	lstep = (lstep + 6) % 12;
	cx = scx;
	
	if((!( FFCore.system_suspend[susptGLOBALGAME] )) && (global_wait & (1<<GLOBAL_SCRIPT_GAME)))
	{
		ZScriptVersion::RunScript(SCRIPT_GLOBAL, GLOBAL_SCRIPT_GAME, GLOBAL_SCRIPT_GAME);
		global_wait &= ~(1<<GLOBAL_SCRIPT_GAME);
	}
	if ( (!( FFCore.system_suspend[susptLINKACTIVE] )) && link_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
	{
		ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_ACTIVE, SCRIPT_LINK_ACTIVE);
		link_waitdraw = false;
	}
	if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && dmap_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
	{
		ZScriptVersion::RunScript(SCRIPT_DMAP, DMaps[currdmap].script,currdmap);
		dmap_waitdraw = false;
	}
	if ( (!( FFCore.system_suspend[susptDMAPSCRIPT] )) && passive_subscreen_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
	{
		ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script,currdmap);
		passive_subscreen_waitdraw = false;
	}
	if ( (!( FFCore.system_suspend[susptSCREENSCRIPTS] )) && tmpscr->script != 0 && tmpscr->screen_waitdraw && FFCore.getQuestHeaderInfo(vZelda) >= 0x255 )
	{
		ZScriptVersion::RunScript(SCRIPT_SCREEN, tmpscr->script, 0);  
		tmpscr->screen_waitdraw = 0;		
	}
	
	for ( int32_t q = 0; q < 32; ++q )
	{
	//Z_scripterrlog("tmpscr->ffcswaitdraw is: %d\n", tmpscr->ffcswaitdraw);
	if ( tmpscr->ffcswaitdraw&(1<<q) )
	{
		//Z_scripterrlog("FFC (%d) called Waitdraw()\n", q);
		if(tmpscr->ffscript[q] != 0)
		{
			ZScriptVersion::RunScript(SCRIPT_FFC, tmpscr->ffscript[q], q);
			tmpscr->ffcswaitdraw &= ~(1<<q);
		}
	}
	}
	//Waitdraw for item scripts. 
	FFCore.itemScriptEngineOnWaitdraw();
	
	//Sprite scripts on Waitdraw
	FFCore.eweaponScriptEngineOnWaitdraw();
	FFCore.itemSpriteScriptEngineOnWaitdraw();
	
	//This is no longer a do-while, as the first iteration is now slightly different. -V
	draw_screen(tmpscr);
	
	if(cx == scx)
		rehydratelake(false);
		
	advanceframe(true);
	
	if(Quit)
	{
		screenscrolling = false;
		return;
	}
	
	++cx;
	while(cx < 32)
	{
		if(get_bit(quest_rules,qr_FIXSCRIPTSDURINGSCROLLING))
		{
			ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, false); //Prewaitdraw
			ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, true); //Waitdraw
		}
		draw_screen(tmpscr);
		
		if(cx == scx)
			rehydratelake(false);
			
		advanceframe(true);
		
		if(Quit)
		{
			screenscrolling = false;
			return;
		}
		
		++cx;
	}
	script_drawing_commands.Clear();
	
	
	//clear Link's last hits 
	//for ( int32_t q = 0; q < 4; q++ ) sethitLinkUID(q, 0);
	
	switch(DMaps[currdmap].type&dmfTYPE)
	{
		case dmDNGN:
			if(!get_bit(quest_rules, qr_DUNGEONS_USE_CLASSIC_CHARTING))
			{
				markBmap(scrolldir);
			}
			break;
		case dmOVERW: case dmBSOVERW:
			if(get_bit(quest_rules, qr_NO_OVERWORLD_MAP_CHARTING))
				break;
			
		case dmCAVE:
			markBmap(scrolldir);
			break;
	}
	
	if(fixed_door)
	{
		unsetmapflag(mSECRET);
		fixed_door = false;
	}
	//Z_scripterrlog("Setting 'scrolling_scr' from %d to %d\n", scrolling_scr, currscr);
	scrolling_scr = currscr;
	
	switch(scrolldir)
	{
	case up:
	{
		if(destscr != -1)
			currscr = destscr;
		else if(checkmaze(oldscr,true) && !edge_of_dmap(scrolldir))
			currscr -= 16;
			
		loadscr(0,destdmap,currscr,scrolldir,overlay);
		blit(scrollbuf,scrollbuf,0,0,0,176,256,176);
		putscr(scrollbuf,0,0,newscr);
		putscrdoors(scrollbuf,0,0,newscr);
		sy=176;
		
		if(get_bit(quest_rules, qr_SMOOTHVERTICALSCROLLING) == 0)
			sy+=3;
			
		cx=176/step;
	FFCore.init_combo_doscript();
	}
	break;
	
	case down:
	{
		if(destscr != -1)
			currscr = destscr;
		else if(checkmaze(oldscr,true) && !edge_of_dmap(scrolldir))
			currscr += 16;
			
		loadscr(0,destdmap,currscr,scrolldir,overlay);
		putscr(scrollbuf,0,176,newscr);
		putscrdoors(scrollbuf,0,176,newscr);
		sy = 0;
		
		if(get_bit(quest_rules, qr_SMOOTHVERTICALSCROLLING) == 0)
			sy+=3;
			
		cx = 176 / step;
	FFCore.init_combo_doscript();
	}
	break;
	
	case left:
	{
		if(destscr!=-1)
			currscr = destscr;
		else if(checkmaze(oldscr,true) && !edge_of_dmap(scrolldir))
			--currscr;
			
		loadscr(0,destdmap,currscr,scrolldir,overlay);
		blit(scrollbuf,scrollbuf,0,0,256,0,256,176);
		putscr(scrollbuf,0,0,newscr);
		putscrdoors(scrollbuf,0,0,newscr);
		sx = 256;
		cx = 256 / step;
	FFCore.init_combo_doscript();
	}
	break;
	
	case right:
	{
		if(destscr != -1)
			currscr = destscr;
		else if(checkmaze(oldscr,true) && !edge_of_dmap(scrolldir))
			++currscr;
			
		loadscr(0,destdmap,currscr,scrolldir,overlay);
		putscr(scrollbuf,256,0,newscr);
		putscrdoors(scrollbuf,256,0,tmpscr);
		sx = 0;
		cx = 256 / step;
	FFCore.init_combo_doscript();
	}
	break;
	}

	// change Link's state if entering water
	int32_t ahead = lookahead(scrolldir);
	int32_t aheadflag = lookaheadflag(scrolldir);
	int32_t lookaheadx = vbound(x+8,0,240); //var = vbound(val, n1, n2), not bound(var, n1, n2) -Z
	int32_t lookaheady = vbound(y + (bigHitbox?8:12),0,160);
		//bound(cx, 0, 240); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
		//bound(cy, 0, 168); //Fix crash during screen scroll when Link is moving too quickly through a corner - DarkDragon
		//y+8 could be 168 //Attempt to fix a frash where scrolling through the lower-left corner could crassh ZC as reported by Lut. -Z
	switch(scrolldir)
	{
		case up:
			lookaheady=160;
			break;
			
		case down:
			lookaheady=0;
			break;
			
		case left:
			lookaheadx=240;
			break;
			
		case right:
			lookaheadx=0;
			break;
	}

	bool nowinwater = false;

	if(lastaction != inwind)
	{
		if(lastaction == rafting ) //&& isRaftFlag(aheadflag))
		{
			if (lookaheadraftflag(scrolldir))
			{
				action=rafting; FFCore.setLinkAction(rafting);
				raftclk=0;
			}
		}
		else if(iswaterex(ahead, currmap, currscr, -1, lookaheadx,lookaheady) && (current_item(itype_flippers)))
		{
			if(lastaction==swimming || lastaction == sideswimming || lastaction == sideswimattacking || lastaction == sideswimhit || lastaction == swimhit || lastaction == sideswimcasting || lastaction == sidewaterhold1 || lastaction == sidewaterhold2)
			{
				SetSwim();
				hopclk = 0xFF;
				nowinwater = true;
			}
			else
			{
				action=hopping; FFCore.setLinkAction(hopping);
				hopclk = 2;
				nowinwater = true;
			}
		}
		else
		{
			action=none; FFCore.setLinkAction(none);
		}
	}
	
	// The naturaldark state can be read/set by an FFC script before
	// fade() or lighting() is called.
	naturaldark = ((TheMaps[currmap*MAPSCRS+currscr].flags & fDARK) != 0);
	
	if(newscr->oceansfx != oldscr->oceansfx)	adjust_sfx(oldscr->oceansfx, 128, false);
	
	if(newscr->bosssfx != oldscr->bosssfx)	adjust_sfx(oldscr->bosssfx, 128, false);
	
	//Preloaded ffc scripts
	if(destdmap >= 0)
	{
		int32_t dmap = currdmap; // Kludge
		currdmap = destdmap;
		ffscript_engine(true);
		currdmap = dmap;
	}
	else
		ffscript_engine(true);
		
	// There are two occasions when scrolling must be darkened:
	// 1) When scrolling into a dark room.
	// 2) When scrolling between DMaps of different colours.
	if(destdmap != -1 && DMaps[destdmap].color != currcset)
	{
		fade((specialcave > 0) ? (specialcave >= GUYCAVE) ? 10 : 11 : currcset, true, false);
		darkroom = true;
	}
	else if(!darkroom)
		lighting(false, false); // NES behaviour: fade to dark before scrolling
		
	if(action != rafting)  // Is this supposed to be here?!
	
		cx++; //This was the easiest way to re-arrange the loop so drawing is in the middle
		
	cx *= delay; //so we can have drawing re-done every frame,
	//previously it was for(0 to delay) advanceframes at end of loop
	int32_t no_move = 0;
	
	for(word i = 0; cx >= 0 && delay != 0; i++, cx--) //Go!
	{
		if(Quit)
		{
			screenscrolling = false;
			return;
		}
		
		script_drawing_commands.Clear();
		
		ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, false);
		
		if(no_move > 0)
			no_move--;
			
		//Don't want to move things on the first or last iteration, or between delays
		if(i == 0 || cx == 0 || cx % delay != 0)
			no_move++;
			
		if(scrolldir == up || scrolldir == down)
		{
			if(!get_bit(quest_rules, qr_SMOOTHVERTICALSCROLLING))
			{
				//Add a few extra frames if on the second loop and cool scrolling is not set
				if(i == 1)
				{
					cx += (scrolldir == down) ? 3 : 2;
					no_move += (scrolldir == down) ? 3 : 2;
				}
			}
			else
			{
				//4 frames after we've finished scrolling of being still
				if(cx == 0 && !end_frames)
				{
					cx += 4;
					no_move += 4;
					end_frames = true;
				}
			}
		}
		
		//Move Link and the scroll position
		if(!no_move)
		{
			switch(scrolldir)
			{
			case up:
				sy -= step;
				y += step;
				break;
				
			case down:
				sy += step;
				y -= step;
				break;
				
			case left:
				sx -= step;
				x += step;
				break;
				
			case right:
				sx += step;
				x -= step;
				break;
			}
			
			//bound Link when me move him off the screen in the last couple of frames of scrolling
			if(y > 160) y = 160;
			
			if(y < 0)   y = 0;
			
			if(x > 240) x = 240;
			
			if(x < 0)   x = 0;
			
			if(ladderx > 0 || laddery > 0)
			{
				// If the ladder moves on both axes, the player can
				// gradually shift it by going back and forth
				if(scrolldir==up || scrolldir==down)
					laddery = y.getInt();
				else
					ladderx = x.getInt();
			}
		}
		
		//Drawing
		tx = sx;
		ty = sy;
		tx2 = sx;
		ty2 = sy;
		
		switch(scrolldir)
		{
		case right:
			FFCore.ScrollingData[SCROLLDATA_NX] = 256-tx2;
			FFCore.ScrollingData[SCROLLDATA_NY] = 0;
			FFCore.ScrollingData[SCROLLDATA_OX] = -tx2;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			tx -= 256;
			break;
			
		case down:
			FFCore.ScrollingData[SCROLLDATA_NX] = 0;
			FFCore.ScrollingData[SCROLLDATA_NY] = 176-ty2;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = -ty2;
			ty -= 176;
			break;
			
		case left:
			FFCore.ScrollingData[SCROLLDATA_NX] = -tx2;
			FFCore.ScrollingData[SCROLLDATA_NY] = 0;
			FFCore.ScrollingData[SCROLLDATA_OX] = 256-tx2;
			FFCore.ScrollingData[SCROLLDATA_OY] = 0;
			tx2 -= 256;
			break;
			
		case up:
			FFCore.ScrollingData[SCROLLDATA_NX] = 0;
			FFCore.ScrollingData[SCROLLDATA_NY] = -ty2;
			FFCore.ScrollingData[SCROLLDATA_OX] = 0;
			FFCore.ScrollingData[SCROLLDATA_OY] = 176-ty2;
			ty2 -= 176;
			break;
		}
		
		//FFScript.OnWaitdraw()
		ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, true); //Waitdraw
		
		clear_bitmap(scrollbuf);
		clear_bitmap(framebuf);
		
		switch(scrolldir)
		{
		case up:
			if(newscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, newscr, 0, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, oldscr, 0, -176+playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, newscr, 0, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, oldscr, 0, -176+playing_field_offset, 3);
			
			// Draw both screens' background layer primitives together, after both layers' combos.
			// Not ideal, but probably good enough for all realistic purposes.
			if(newscr->flags7&fLAYER2BG || oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_primitives(scrollbuf, 2, newscr, sx, sy);
			
			if(newscr->flags7&fLAYER3BG || oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_primitives(scrollbuf, 3, newscr, sx, sy);
			
			putscr(scrollbuf, 0, 0, newscr);
			putscr(scrollbuf, 0, 176, oldscr);
			break;
			
		case down:
			if(newscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, newscr, 0, -176+playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, oldscr, 0, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, newscr, 0, -176+playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, oldscr, 0, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER2BG || oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_primitives(scrollbuf, 2, newscr, sx, sy);
			
			if(newscr->flags7&fLAYER3BG || oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_primitives(scrollbuf, 3, newscr, sx, sy);
			
			putscr(scrollbuf, 0, 0, oldscr);
			putscr(scrollbuf, 0, 176, newscr);
			break;
			
		case left:
			if(newscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, newscr, 0, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, oldscr, -256, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, newscr, 0, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, oldscr, -256, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER2BG || oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_primitives(scrollbuf, 2, newscr, sx, sy);
			
			if(newscr->flags7&fLAYER3BG || oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_primitives(scrollbuf, 3, newscr, sx, sy);
			
			putscr(scrollbuf, 0, 0, newscr);
			putscr(scrollbuf, 256, 0, oldscr);
			break;
			
		case right:
			if(newscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, newscr, -256, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, oldscr, 0, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, newscr, -256, playing_field_offset, 2);
			
			if(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, oldscr, 0, playing_field_offset, 3);
			
			if(newscr->flags7&fLAYER2BG || oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_primitives(scrollbuf, 2, newscr, sx, sy);
			
			if(newscr->flags7&fLAYER3BG || oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_primitives(scrollbuf, 3, newscr, sx, sy);
			
			putscr(scrollbuf, 0, 0, oldscr);
			putscr(scrollbuf, 256, 0, newscr);
			break;
		}
		
		blit(scrollbuf, framebuf, sx, sy, 0, playing_field_offset, 256, 168);
		do_primitives(framebuf, 0, newscr, 0, playing_field_offset);
		
		do_layer(framebuf, 0, oldscr, tx2, ty2, 3);
		
		if(!(oldscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG) ) do_layer(framebuf,1, oldscr, tx2, ty2, 3);
		
		do_layer(framebuf, 0, newscr, tx, ty, 2, false, true);
		
		if(!(newscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG)) do_layer(framebuf,1, newscr, tx, ty, 2, false, !(oldscr->flags7&fLAYER2BG));
		
		do_layer(framebuf, -2, oldscr, tx2, ty2, 3); //push blocks
		do_layer(framebuf, -2, newscr, tx, ty, 2);
		
		do_walkflags(framebuf, oldscr, tx2, ty2,3); //show walkflags if the cheat is on
		do_walkflags(framebuf, newscr, tx, ty,2);
		
		do_effectflags(framebuf, oldscr, tx2, ty2,3); //show effectflags if the cheat is on
		do_effectflags(framebuf, newscr, tx, ty,2);
		
		if(get_bit(quest_rules, qr_FFCSCROLL))
		{
			do_layer(framebuf, -3, oldscr, tx2, ty2, 3, true); //ffcs
			do_layer(framebuf, -3, newscr, tx, ty, 2, true);
		}
		
		putscrdoors(framebuf, 0-tx2, 0-ty2+playing_field_offset, oldscr);
		putscrdoors(framebuf, 0-tx,  0-ty+playing_field_offset, newscr);
		linkstep();
		
		if(z > 0 && (!get_bit(quest_rules,qr_SHADOWSFLICKER) || frame&1))
		{
			drawshadow(framebuf, get_bit(quest_rules, qr_TRANSSHADOWS) != 0);
		}
		
		if(!isdungeon() || get_bit(quest_rules,qr_FREEFORM))
		{
			draw_under(framebuf); //draw the ladder or raft
			decorations.draw2(framebuf, true);
			draw(framebuf); //Link
			decorations.draw(framebuf,  true);
		}
		
		if(!(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG )) do_layer(framebuf,2, oldscr, tx2, ty2, 3);
		
		do_layer(framebuf, 3, oldscr, tx2, ty2, 3); //layer 3
		do_layer(framebuf,-1, oldscr, tx2, ty2, 3); //overhead combos
		do_layer(framebuf, 4, oldscr, tx2, ty2, 3); //layer 4
		do_layer(framebuf,-4, oldscr, tx2, ty2, 3, true); //overhead FFCs
		do_layer(framebuf, 5, oldscr, tx2, ty2, 3); //layer 5
		
		if(!(newscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG )) do_layer(framebuf,2, newscr, tx, ty, 2, false, !(oldscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ));
		
		do_layer(framebuf, 3, newscr, tx, ty, 2, false, true); //layer 3
		do_layer(framebuf,-1, newscr, tx, ty, 2); //overhead combos
		do_layer(framebuf, 4, newscr, tx, ty, 2, false, true); //layer 4
		do_layer(framebuf,-4, newscr, tx, ty, 2, true); //overhead FFCs
		do_layer(framebuf, 5, newscr, tx, ty, 2, false, true); //layer 5
		
		
		if(msg_bg_display_buf->clip == 0)
		{
			blit_msgstr_bg(framebuf, tx2, ty2, 0, playing_field_offset, 256, 168);
		}
		if(msg_portrait_display_buf->clip == 0)
		{
			blit_msgstr_prt(framebuf, tx2, ty2, 0, playing_field_offset, 256, 168);
		}
		if(msg_txt_display_buf->clip == 0)
		{
			blit_msgstr_fg(framebuf, tx2, ty2, 0, playing_field_offset, 256, 168);
		}
			
		if(get_bit(quest_rules, qr_NEW_DARKROOM) && ((newscr->flags&fDARK)||(oldscr->flags&fDARK)))
		{
			clear_to_color(darkscr_bmp_curscr, game->get_darkscr_color());
			clear_to_color(darkscr_bmp_curscr_trans, game->get_darkscr_color());
			clear_to_color(darkscr_bmp_scrollscr, game->get_darkscr_color());
			clear_to_color(darkscr_bmp_scrollscr_trans, game->get_darkscr_color());
			calc_darkroom_combos(true);
			calc_darkroom_link(FFCore.ScrollingData[SCROLLDATA_NX], FFCore.ScrollingData[SCROLLDATA_NY],FFCore.ScrollingData[SCROLLDATA_OX], FFCore.ScrollingData[SCROLLDATA_OY]);
		}
		
		if(get_bit(quest_rules, qr_NEW_DARKROOM) && get_bit(quest_rules, qr_NEWDARK_L6))
		{
			set_clip_rect(framebuf, 0, playing_field_offset, 256, 168+playing_field_offset);
			int32_t dx1 = FFCore.ScrollingData[SCROLLDATA_NX], dy1 = FFCore.ScrollingData[SCROLLDATA_NY]+playing_field_offset;
			int32_t dx2 = FFCore.ScrollingData[SCROLLDATA_OX], dy2 = FFCore.ScrollingData[SCROLLDATA_OY]+playing_field_offset;
			if(newscr->flags & fDARK)
			{
				if(newscr->flags9 & fDARK_DITHER) //dither the entire bitmap
				{
					ditherblit(darkscr_bmp_curscr,darkscr_bmp_curscr,0,game->get_dither_type(),game->get_dither_arg());
					ditherblit(darkscr_bmp_curscr_trans,darkscr_bmp_curscr_trans,0,game->get_dither_type(),game->get_dither_arg());
				}
				
				color_map = &trans_table2;
				if(newscr->flags9 & fDARK_TRANS) //draw the dark as transparent
					draw_trans_sprite(framebuf, darkscr_bmp_curscr, dx1, dy1);
				else 
					masked_blit(darkscr_bmp_curscr, framebuf, 0, 0, dx1, dy1, 256, 176);
				draw_trans_sprite(framebuf, darkscr_bmp_curscr_trans, dx1, dy1);
				color_map = &trans_table;
			}
			if(oldscr->flags & fDARK)
			{
				if(oldscr->flags9 & fDARK_DITHER) //dither the entire bitmap
				{
					ditherblit(darkscr_bmp_scrollscr,darkscr_bmp_scrollscr,0,game->get_dither_type(),game->get_dither_arg());
					ditherblit(darkscr_bmp_scrollscr_trans,darkscr_bmp_scrollscr_trans,0,game->get_dither_type(),game->get_dither_arg());
				}
				
				color_map = &trans_table2;
				if(oldscr->flags9 & fDARK_TRANS) //draw the dark as transparent
					draw_trans_sprite(framebuf, darkscr_bmp_scrollscr, dx2, dy2);
				else 
					masked_blit(darkscr_bmp_scrollscr, framebuf, 0, 0, dx2, dy2, 256, 176);
				draw_trans_sprite(framebuf, darkscr_bmp_scrollscr_trans, dx2, dy2);
				color_map = &trans_table;
			}
			set_clip_rect(framebuf, 0, 0, framebuf->w, framebuf->h);
		}
		
		put_passive_subscr(framebuf, &QMisc, 0, passive_subscreen_offset, false, sspUP);
		if(get_bit(quest_rules,qr_SUBSCREENOVERSPRITES))
			do_primitives(framebuf, 7, newscr, 0, playing_field_offset);
		
		if(get_bit(quest_rules, qr_NEW_DARKROOM) && !get_bit(quest_rules, qr_NEWDARK_L6))
		{
			set_clip_rect(framebuf, 0, playing_field_offset, 256, 168+playing_field_offset);
			int32_t dx1 = FFCore.ScrollingData[SCROLLDATA_NX], dy1 = FFCore.ScrollingData[SCROLLDATA_NY]+playing_field_offset;
			int32_t dx2 = FFCore.ScrollingData[SCROLLDATA_OX], dy2 = FFCore.ScrollingData[SCROLLDATA_OY]+playing_field_offset;
			if(newscr->flags & fDARK)
			{
				if(newscr->flags9 & fDARK_DITHER) //dither the entire bitmap
				{
					ditherblit(darkscr_bmp_curscr,darkscr_bmp_curscr,0,game->get_dither_type(),game->get_dither_arg());
					ditherblit(darkscr_bmp_curscr_trans,darkscr_bmp_curscr_trans,0,game->get_dither_type(),game->get_dither_arg());
				}
				
				color_map = &trans_table2;
				if(newscr->flags9 & fDARK_TRANS) //draw the dark as transparent
					draw_trans_sprite(framebuf, darkscr_bmp_curscr, dx1, dy1);
				else 
					masked_blit(darkscr_bmp_curscr, framebuf, 0, 0, dx1, dy1, 256, 176);
				draw_trans_sprite(framebuf, darkscr_bmp_curscr_trans, dx1, dy1);
				color_map = &trans_table;
			}
			if(oldscr->flags & fDARK)
			{
				if(oldscr->flags9 & fDARK_DITHER) //dither the entire bitmap
				{
					ditherblit(darkscr_bmp_scrollscr,darkscr_bmp_scrollscr,0,game->get_dither_type(),game->get_dither_arg());
					ditherblit(darkscr_bmp_scrollscr_trans,darkscr_bmp_scrollscr_trans,0,game->get_dither_type(),game->get_dither_arg());
				}
				
				color_map = &trans_table2;
				if(oldscr->flags9 & fDARK_TRANS) //draw the dark as transparent
					draw_trans_sprite(framebuf, darkscr_bmp_scrollscr, dx2, dy2);
				else 
					masked_blit(darkscr_bmp_scrollscr, framebuf, 0, 0, dx2, dy2, 256, 176);
				draw_trans_sprite(framebuf, darkscr_bmp_scrollscr_trans, dx2, dy2);
				color_map = &trans_table;
			}
			set_clip_rect(framebuf, 0, 0, framebuf->w, framebuf->h);
		}
		
		//end drawing
		advanceframe(true/*,true,false*/);
		actiontype lastaction = action;
		action=scrolling; FFCore.setLinkAction(scrolling);
		FFCore.runF6Engine();
		//FFCore.runF6EngineScrolling(newscr,oldscr,tx,ty,tx2,ty2,sx,sy,scrolldir);
		action=lastaction; FFCore.setLinkAction(lastaction);
	}//end main scrolling loop (2 spaces tab width makes me sad =( )
	
	
	clear_bitmap(msg_txt_display_buf);
	set_clip_state(msg_txt_display_buf, 1);
	clear_bitmap(msg_bg_display_buf);
	set_clip_state(msg_bg_display_buf, 1);
	clear_bitmap(msg_portrait_display_buf);
	set_clip_state(msg_portrait_display_buf, 1);
	
	//Move link to the other side of the screen if scrolling's not turned on
	if(get_bit(quest_rules, qr_NOSCROLL))
	{
		switch(scrolldir)
		{
		case up:
			y = 160;
			break;
			
		case down:
			y = 0;
			break;
			
		case left:
			x = 240;
			break;
			
		case right:
			x = 0;
			break;
		}
	}
	
	if(z > 0 && isSideViewLink())
	{
		y -= z;
		z = 0;
	}
	
	entry_x = x;
	entry_y = y;
	warpx   = -1;
	warpy   = -1;
	
	screenscrolling = false;
	FFCore.ScrollingData[SCROLLDATA_DIR] = -1;
	FFCore.ScrollingData[SCROLLDATA_NX] = 0;
	FFCore.ScrollingData[SCROLLDATA_NY] = 0;
	FFCore.ScrollingData[SCROLLDATA_OX] = 0;
	FFCore.ScrollingData[SCROLLDATA_OY] = 0;
	
	if(destdmap != -1)
		currdmap = destdmap;
		
	//if Link is going from non-water to water, and we set his animation to "hopping" above, we must now
	//change it to swimming - since we have manually moved Link onto the first tile, the hopping code
	//will get confused and try to hop Link onto the next (possibly nonexistant) water tile in his current
	//direction. -DD
	
	if(nowinwater)
	{
		SetSwim();
		hopclk = 0xFF;
	}
	
	// NES behaviour: Fade to light after scrolling
	lighting(false, false); // No, we don't need to set naturaldark...
	
	homescr=currscr;
	init_dmap();
	putscr(scrollbuf,0,0,newscr);
	putscrdoors(scrollbuf,0,0,newscr);
	
	// Check for raft flags
	if(action!=rafting && hopclk==0 && !toogam)
	{
		if(MAPFLAG(x,y)==mfRAFT||MAPCOMBOFLAG(x,y)==mfRAFT)
		{
			sfx(tmpscr->secretsfx);
			action=rafting; FFCore.setLinkAction(rafting);
			raftclk=0;
		}
		
		// Half a tile off?
		else if((dir==left || dir==right) && (MAPFLAG(x,y+8)==mfRAFT||MAPCOMBOFLAG(x,y+8)==mfRAFT))
		{
			sfx(tmpscr->secretsfx);
			action=rafting; FFCore.setLinkAction(rafting);
			raftclk=0;
		}
	}
	
	opendoors=0;
	markBmap(-1);
	
	if(isdungeon())
	{
		switch(tmpscr->door[scrolldir^1])
		{
		case dOPEN:
		case dUNLOCKED:
		case dOPENBOSS:
			dir = scrolldir;
			
			if(action!=rafting)
				stepforward(diagonalMovement?11:12, false);
				
			break;
			
		case dSHUTTER:
		case d1WAYSHUTTER:
			dir = scrolldir;
			
			if(action!=rafting)
				stepforward(diagonalMovement?21:24, false);
				
			putdoor(scrollbuf,0,scrolldir^1,tmpscr->door[scrolldir^1]);
			opendoors=-4;
			sfx(WAV_DOOR);
			break;
			
		default:
			dir = scrolldir;
			
			if(action!=rafting)
				stepforward(diagonalMovement?21:24, false);
		}
	}
	
	if(action == scrolling)
	{
		action=none; FFCore.setLinkAction(none);
	}
		
	if(action != attacking && action != sideswimattacking)
	{
		charging = 0;
		tapping = false;
	}
	
	map_bkgsfx(true);
	
	if(newscr->flags2&fSECRET)
	{
		sfx(newscr->secretsfx);
	}
	
	playLevelMusic();
	
	newscr_clk = frame;
	activated_timed_warp=false;
	loadside = scrolldir^1;
	FFCore.init_combo_doscript();
	eventlog_mapflags();
	decorations.animate(); //continue to animate tall grass during scrolling
	if(get_bit(quest_rules,qr_FIXSCRIPTSDURINGSCROLLING))
	{
		script_drawing_commands.Clear();
		ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, false); //Prewaitdraw
		ZScriptVersion::RunScrollingScript(scrolldir, cx, sx, sy, end_frames, true); //Waitdraw
	}
}



// How much to reduce Link's damage, taking into account various rings.
int32_t LinkClass::ringpower(int32_t dmg)
{
	if ( get_bit(quest_rules,qr_BROKEN_RING_POWER) )
	{
		int32_t divisor = 1;
		float percentage = 1;
		int32_t itemid = current_item_id(itype_ring);
		bool usering = false;
    
		if(itemid>-1)  // current_item_id checks magic cost for rings
		{
			usering = true;
			paymagiccost(itemid);
			if(itemsbuf[itemid].flags & ITEM_FLAG2)//"Divisor is Percentage Multiplier" flag
			{
				percentage *= itemsbuf[itemid].power/100.0;
			}
			else
			{
				divisor *= itemsbuf[itemid].power;
			}
		}
	    
		/* Now for the Peril Ring */
		itemid = current_item_id(itype_perilring);
    
		if(itemid>-1 && game->get_life()<=itemsbuf[itemid].misc1*game->get_hp_per_heart() && checkmagiccost(itemid) && checkbunny(itemid))
		{
			usering = true;
			paymagiccost(itemid);
			if(itemsbuf[itemid].flags & ITEM_FLAG2)//"Divisor is Percentage Multiplier" flag
			{
				percentage *= itemsbuf[itemid].power/100.0;
			}
			else
			{
				divisor *= itemsbuf[itemid].power;
			}
		}
	
		// Ring divisor of 0 = no damage. -L
		if(usering && (divisor==0 || percentage==0)) //Change dto allow negative power rings. -Z
		return 0;
	
		if( percentage < 0 ) percentage = (percentage * -1) + 1; //Negative percentage = that percent MORE damage -V
	
		if ( divisor < 0 ) return dmg * percentage * (divisor*-1);
		return dmg*percentage/( divisor != 0 ? divisor : 1 ); //zc_max(divisor, 1); // well, better safe...
		
	}
	else
	{
		double divisor = 1;
		double percentage = 1;
		int32_t itemid = current_item_id(itype_ring);
		bool usering = false;
		    
		if(itemid>-1)  // current_item_id checks magic cost for rings
		{
		usering = true;
		paymagiccost(itemid);
			if(itemsbuf[itemid].flags & ITEM_FLAG2)//"Divisor is Percentage Multiplier" flag
			{
				double perc = itemsbuf[itemid].power/100.0;
				if(perc < 0) perc = -perc + 1; //Negative percentage = that percent MORE damage -V
				percentage *= perc;
			}
			else
			{
				if(itemsbuf[itemid].power < 0)
					divisor /= -(itemsbuf[itemid].power);
				else divisor *= itemsbuf[itemid].power;
			}
		}
	    
		/* Now for the Peril Ring */
		itemid = current_item_id(itype_perilring);
	    
		if(itemid>-1 && game->get_life()<=itemsbuf[itemid].misc1*game->get_hp_per_heart() && checkmagiccost(itemid) && checkbunny(itemid))
		{
			usering = true;
			paymagiccost(itemid);
			if(itemsbuf[itemid].flags & ITEM_FLAG2)//"Divisor is Percentage Multiplier" flag
			{
				double perc = itemsbuf[itemid].power/100.0;
				if(perc < 0) perc = -perc + 1; //Negative percentage = that percent MORE damage -V
				percentage *= perc;
			}
			else
			{
				if(itemsbuf[itemid].power < 0)
					divisor /= -(itemsbuf[itemid].power);
				else divisor *= itemsbuf[itemid].power;
			}
		}
	    
		// Ring divisor of 0 = no damage. -L
		if(usering && (divisor==0 || percentage==0)) //Change dto allow negative power rings. -Z
			return 0;
		
		//if ( divisor < 0 ) return dmg * percentage * (divisor*-1); //handle this further up now
		return dmg*percentage/( divisor != 0 ? divisor : 1 ); //zc_max(divisor, 1); // well, better safe...
	}
}

// Should swinging the hammer make the 'pound' sound?
// Or is Link just hitting air?
bool LinkClass::sideviewhammerpound()
{
    int32_t wx=0,wy=0;
    
    switch(dir)
    {
    case up:
        wx=-1;
        wy=-15;
        
        if(isSideViewLink())  wy+=8;
        
        break;
        
    case down:
        wx=8;
        wy=28;
        
        if(isSideViewLink())  wy-=8;
        
        break;
        
    case left:
        wx=-8;
        wy=14;
        
        if(isSideViewLink()) wy+=8;
        
        break;
        
    case right:
        wx=21;
        wy=14;
        
        if(isSideViewLink()) wy+=8;
        
        break;
    }
    
    if(!isSideViewLink())
    {
        return (COMBOTYPE(x+wx,y+wy)!=cSHALLOWWATER && !iswaterex(MAPCOMBO(x+wx,y+wy), currmap, currscr, -1, x+wx,y+wy));
    }
    
    if(_walkflag(x+wx,y+wy,0,SWITCHBLOCK_STATE)) return true;
    
    if(dir==left || dir==right)
    {
        wx+=16;
        
        if(_walkflag(x+wx,y+wy,0,SWITCHBLOCK_STATE)) return true;
    }
    
    return false;
}

/************************************/
/********  More Items Code  *********/
/************************************/

// The following are only used for Link damage. Damage is in quarter hearts.
int32_t enemy_dp(int32_t index)
{
    return (((enemy*)guys.spr(index))->dp)*(game->get_ene_dmgmult());
}

int32_t ewpn_dp(int32_t index)
{
    return (((weapon*)Ewpns.spr(index))->power)*(game->get_ene_dmgmult());
}

int32_t lwpn_dp(int32_t index)
{
    return (((weapon*)Lwpns.spr(index))->power)*(game->get_ene_dmgmult());
}

bool checkbunny(int32_t itemid)
{
	return !Link.BunnyClock() || (itemid > 0 && itemsbuf[itemid].flags&ITEM_BUNNY_ENABLED);
}

bool checkmagiccost(int32_t itemid)
{
	if(itemid < 0)
	{
		return false;
	}
	
	switch (itemsbuf[itemid].cost_counter )
	{
		case 1: //rupees
		{
			if ( current_item_power(itype_wallet) ) return true;
			return (game->get_rupies()+game->get_drupy()>=itemsbuf[itemid].magic);
		}
		case 4: //magic
		{
			if (get_bit(quest_rules,qr_ENABLEMAGIC))
			{
				return (((current_item_power(itype_magicring) > 0)
					 ? game->get_maxmagic()
					 : game->get_magic()+game->get_dmagic())>=itemsbuf[itemid].magic*game->get_magicdrainrate());
			}
			return true;
		}
	}
	return (game->get_counter(itemsbuf[itemid].cost_counter)+game->get_dcounter(itemsbuf[itemid].cost_counter)>=itemsbuf[itemid].magic);
}

void paymagiccost(int32_t itemid, bool ignoreTimer)
{
	if(itemid < 0)
	{
		return;
	}
	else if(itemsbuf[itemid].magic <= 0)
	{
		return;
	}
	else if(itemsbuf[itemid].flags&ITEM_VALIDATEONLY) //Only validate, not pay, the cost. -V
	{
		return;
	}
	
	switch(itemsbuf[itemid].cost_counter)
	{
		case 4: //magic
		{
			if(!get_bit(quest_rules,qr_ENABLEMAGIC))
				return;
			if(current_item_power(itype_magicring) > 0)
				return;
			if ( itemsbuf[itemid].magiccosttimer > 0 && !ignoreTimer) 
			{
				if ( frame % itemsbuf[itemid].magiccosttimer == 0 )
					game->change_magic(-(itemsbuf[itemid].magic*game->get_magicdrainrate()));
				return;
			}
			else 
			{	game->change_magic(-(itemsbuf[itemid].magic*game->get_magicdrainrate()));
				return;
			}
			break;
		}
		case 1:
		{
			if ( current_item_power(itype_wallet) )
				return;
			if(get_bit(quest_rules,qr_OLDINFMAGIC) && current_item_power(itype_magicring) > 0)
				return;
			if ( itemsbuf[itemid].magiccosttimer > 0 && !ignoreTimer) 
			{
				if ( frame % itemsbuf[itemid].magiccosttimer == 0 )
					game->change_drupy(-itemsbuf[itemid].magic);
				return;
			}
			else 
			{
				game->change_drupy(-itemsbuf[itemid].magic);
				return;
			}
			break;
		}
	}
	if ( itemsbuf[itemid].magiccosttimer > 0 && !ignoreTimer) 
	{
		//game->set_counter
		if ( frame % itemsbuf[itemid].magiccosttimer == 0 )
			game->change_counter(-(itemsbuf[itemid].magic), itemsbuf[itemid].cost_counter);
	}
	else 
	{
		game->change_counter(-(itemsbuf[itemid].magic), itemsbuf[itemid].cost_counter);
	}
}

int32_t Bweapon(int32_t pos)
{
    if(pos < 0 || current_subscreen_active == NULL)
    {
        return 0;
    }
    
    int32_t p=-1;
    
    for(int32_t i=0; current_subscreen_active->objects[i].type!=ssoNULL; ++i)
    {
        if(current_subscreen_active->objects[i].type==ssoCURRENTITEM && current_subscreen_active->objects[i].d3==pos)
        {
            p=i;
            break;
        }
    }
    
    if(p==-1)
    {
        return 0;
    }
    
    int32_t actualItem = current_subscreen_active->objects[p].d8;
    //int32_t familyCheck = actualItem ? itemsbuf[actualItem].family : current_subscreen_active->objects[p].d1
    int32_t family = -1;
    bool bow = false;
    
    if(actualItem)
    {
        bool select = false;
        
        switch(itemsbuf[actualItem-1].family)
        {
        case itype_bomb:
            if((game->get_bombs() ||
                    // Remote Bombs: the bomb icon can still be used when an undetonated bomb is onscreen.
                    (actualItem-1>-1 && itemsbuf[actualItem-1].misc1==0 && findWeaponWithParent(actualItem-1, wLitBomb))) ||
                    current_item_power(itype_bombbag))
            {
                select=true;
            }
            
            break;
            
        case itype_bowandarrow:
        case itype_arrow:
            if(actualItem-1>-1 && current_item_id(itype_bow)>-1)
            {
                //bow=(current_subscreen_active->objects[p].d1==itype_bowandarrow);
                select=true;
            }
            
            break;
            
        case itype_letterpotion:
            /*if(current_item_id(itype_potion)>-1)
            {
              select=true;
            }
            else if(current_item_id(itype_letter)>-1)
            {
              select=true;
            }*/
            break;
            
        case itype_sbomb:
        {
            int32_t bombbagid = current_item_id(itype_bombbag);
            
            if((game->get_sbombs() ||
                    // Remote Bombs: the bomb icon can still be used when an undetonated bomb is onscreen.
                    (actualItem-1>-1 && itemsbuf[actualItem-1].misc1==0 && findWeaponWithParent(actualItem-1, wLitSBomb))) ||
                    (current_item_power(itype_bombbag) && bombbagid>-1 && (itemsbuf[bombbagid].flags & ITEM_FLAG1)))
            {
                select=true;
            }
            
            break;
        }
        
        case itype_sword:
        {
            if(!get_bit(quest_rules,qr_SELECTAWPN))
                break;
                
            select=true;
        }
        break;
        
        default:
            select=true;
        }
        
        if(!item_disabled(actualItem-1) && game->get_item(actualItem-1) && select)
        {
            directItem = actualItem-1;
            
            if(directItem>-1 && itemsbuf[directItem].family == itype_arrow) bow=true;
            
            return actualItem-1+(bow?0xF000:0);
        }
        else return 0;
    }
    
    directItem = -1;
    
    switch(current_subscreen_active->objects[p].d1)
    {
    case itype_bomb:
    {
        int32_t bombid = current_item_id(itype_bomb);
        
        if((game->get_bombs() ||
                // Remote Bombs: the bomb icon can still be used when an undetonated bomb is onscreen.
                (bombid>-1 && itemsbuf[bombid].misc1==0 && Lwpns.idCount(wLitBomb)>0)) ||
                current_item_power(itype_bombbag))
        {
            family=itype_bomb;
        }
        
        break;
    }
    
    case itype_bowandarrow:
    case itype_arrow:
        if(current_item_id(itype_bow)>-1 && current_item_id(itype_arrow)>-1)
        {
            bow=(current_subscreen_active->objects[p].d1==itype_bowandarrow);
            family=itype_arrow;
        }
        
        break;
        
    case itype_letterpotion:
        if(current_item_id(itype_potion)>-1)
        {
            family=itype_potion;
        }
        else if(current_item_id(itype_letter)>-1)
        {
            family=itype_letter;
        }
        
        break;
        
    case itype_sbomb:
    {
        int32_t bombbagid = current_item_id(itype_bombbag);
        int32_t sbombid = current_item_id(itype_sbomb);
        
        if((game->get_sbombs() ||
                // Remote Bombs: the bomb icon can still be used when an undetonated bomb is onscreen.
                (sbombid>-1 && itemsbuf[sbombid].misc1==0 && Lwpns.idCount(wLitSBomb)>0)) ||
                (current_item_power(itype_bombbag) && bombbagid>-1 && (itemsbuf[bombbagid].flags & ITEM_FLAG1)))
        {
            family=itype_sbomb;
        }
        
        break;
    }
    
    case itype_sword:
    {
        if(!get_bit(quest_rules,qr_SELECTAWPN))
            break;
            
        family=itype_sword;
    }
    break;
    
    default:
        family=current_subscreen_active->objects[p].d1;
    }
    
    if(family==-1)
        return 0;
        
    for(int32_t j=0; j<MAXITEMS; j++)
    {
        // Find the item that matches this subscreen object.
        if(itemsbuf[j].family==family && j == current_item_id(family,false) && !item_disabled(j))
        {
            return j+(bow?0xF000:0);
        }
    }
    
    return 0;
}

void stopCaneOfByrna()
{
	for(int32_t i=0; i<Lwpns.Count(); i++)
	{
		weapon *w = ((weapon*)Lwpns.spr(i));
		if(w->id==wCByrna)
			w->dead=1;
	}
}

/* Crashes if used by ffscript.cpp, in case LINKITEMD
void stopCaneOfByrna()
{
	byte prnt_cane = -1; 
	weapon *ew = (weapon*)(Lwpns.spr(Lwpns.idFirst(wCByrna)));
        prnt_cane = ew->parentitem;
	for(int32_t i=0; i<Lwpns.Count(); i++)
	{
		weapon *w = ((weapon*)Lwpns.spr(i));
        
		if(w->id==wCByrna)
		{
			w->dead=1;
		}
	}
	if ( prnt_cane > -1 )
	{
		stop_sfx(itemsbuf[prnt_cane].usesound);
	}
}
*/
//Check if there are no beams, kill sfx, and reset last_cane_of_byrna_item_id
void LinkClass::cleanupByrna()
{
	if ( last_cane_of_byrna_item_id > -1 )
	{
		//al_trace("Last cane id is: %d\n", last_cane_of_byrna_item_id);
		if ( !(Lwpns.idCount(wCByrna)) )
		{
			stop_sfx(itemsbuf[last_cane_of_byrna_item_id].usesound);
			last_cane_of_byrna_item_id = -1; 
		}
	}
}

// Used to find out if an item family is attached to one of the buttons currently pressed.
bool isWpnPressed(int32_t itype)
{
    if((itype==getItemFamily(itemsbuf,Bwpn&0xFFF)) && DrunkcBbtn()) return true; //0xFFF for subscreen overrides
	//Will crash on win10 without it! -Z
    
    if((itype==getItemFamily(itemsbuf,Awpn&0xFFF)) && DrunkcAbtn()) return true;
	
    if((itype==getItemFamily(itemsbuf,Xwpn&0xFFF)) && DrunkcEx1btn()) return true;
    if((itype==getItemFamily(itemsbuf,Ywpn&0xFFF)) && DrunkcEx2btn()) return true;
    
    return false;
}

int32_t getWpnPressed(int32_t itype)
{
    if((itype==getItemFamily(itemsbuf,Bwpn&0xFFF)) && DrunkcBbtn()) return Bwpn; //0xFFF for subscreen overrides
	//Will crash on win10 without it! -Z

    if((itype==getItemFamily(itemsbuf,Awpn&0xFFF)) && DrunkcAbtn()) return Awpn;
    
    return -1;
}

void selectNextAWpn(int32_t type)
{
    if(!get_bit(quest_rules,qr_SELECTAWPN))
        return;
        
    int32_t ret = selectWpn_new(type, game->awpn, game->bwpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
    Awpn = Bweapon(ret);
    directItemA = directItem;
    game->awpn = ret;
}

void selectNextBWpn(int32_t type)
{
	int32_t ret = selectWpn_new(type, game->bwpn, game->awpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
	Bwpn = Bweapon(ret);
	directItemB = directItem;
	game->bwpn = ret;
}

void selectNextXWpn(int32_t type)
{
	if(!get_bit(quest_rules,qr_SET_XBUTTON_ITEMS)) return;
	int32_t ret = selectWpn_new(type, game->xwpn, game->awpn, game->bwpn, get_bit(quest_rules,qr_SET_YBUTTON_ITEMS) ? game->ywpn : -1);
	Xwpn = Bweapon(ret);
	directItemX = directItem;
	game->xwpn = ret;
}

void selectNextYWpn(int32_t type)
{
	if(!get_bit(quest_rules,qr_SET_YBUTTON_ITEMS)) return;
	int32_t ret = selectWpn_new(type, game->ywpn, game->awpn, game->bwpn, get_bit(quest_rules,qr_SET_XBUTTON_ITEMS) ? game->xwpn : -1);
	Ywpn = Bweapon(ret);
	directItemY = directItem;
	game->ywpn = ret;
}

void verifyAWpn()
{
	if ( (game->forced_awpn != -1) ) 
	{ 
		//zprint2("verifyAWpn(); returning early. game->forced_awpn is: %d\n",game->forced_awpn); 
		return;
	}
    if(!get_bit(quest_rules,qr_SELECTAWPN))
    {
        Awpn = selectSword();
        game->awpn = 0xFF;
    }
    else
    {
        game->awpn = selectWpn_new(SEL_VERIFY_RIGHT, game->awpn, game->bwpn, game->xwpn, game->ywpn);
        Awpn = Bweapon(game->awpn);
        directItemA = directItem;
    }
}

void verifyBWpn()
{
	if ( (game->forced_bwpn != -1) ) 
	{ 
		//zprint2("verifyAWpn(); returning early. game->forced_awpn is: %d\n",game->forced_awpn); 
		return;
	}
    game->bwpn = selectWpn_new(SEL_VERIFY_RIGHT, game->bwpn, game->awpn, game->xwpn, game->ywpn);
    Bwpn = Bweapon(game->bwpn);
    directItemB = directItem;
}

void verifyXWpn()
{
	if ( (game->forced_xwpn != -1) ) 
	{ 
		//zprint2("verifyAWpn(); returning early. game->forced_awpn is: %d\n",game->forced_awpn); 
		return;
	}
	if(get_bit(quest_rules,qr_SET_XBUTTON_ITEMS))
		game->xwpn = selectWpn_new(SEL_VERIFY_RIGHT, game->xwpn, game->awpn, game->bwpn, game->ywpn);
	else game->xwpn = -1;
    Xwpn = Bweapon(game->xwpn);
    directItemX = directItem;
}

void verifyYWpn()
{
	if ( (game->forced_ywpn != -1) ) 
	{ 
		//zprint2("verifyAWpn(); returning early. game->forced_awpn is: %d\n",game->forced_awpn); 
		return;
	}
	if(get_bit(quest_rules,qr_SET_YBUTTON_ITEMS))
		game->ywpn = selectWpn_new(SEL_VERIFY_RIGHT, game->ywpn, game->awpn, game->xwpn, game->bwpn);
	else game->ywpn = -1;
    Ywpn = Bweapon(game->ywpn);
    directItemY = directItem;
}

void verifyBothWeapons()
{
    verifyAWpn();
    verifyBWpn();
    verifyXWpn();
    verifyYWpn();
}

int32_t selectWpn_new(int32_t type, int32_t startpos, int32_t forbiddenpos, int32_t fp2, int32_t fp3)
{
    //what will be returned when all else fails.
    //don't return the forbiddenpos... no matter what -DD
    
    int32_t failpos(0);
    
    if(startpos == forbiddenpos || startpos == fp2 || startpos == fp3)
        failpos = 0xFF;
    else failpos = startpos;
    
    // verify startpos
    if(startpos < 0 || startpos >= 0xFF)
        startpos = 0;
        
    if(current_subscreen_active == NULL)
        return failpos;
        
    if(type==SEL_VERIFY_RIGHT || type==SEL_VERIFY_LEFT)
    {
        int32_t wpn = Bweapon(startpos);
        
        if(wpn != 0 && startpos != forbiddenpos && startpos != fp2 && startpos != fp3)
        {
            return startpos;
        }
    }
    
    int32_t p=-1;
    int32_t curpos = startpos;
    int32_t firstValidPos=-1;
    
    for(int32_t i=0; current_subscreen_active->objects[i].type!=ssoNULL; ++i)
    {
        if(current_subscreen_active->objects[i].type==ssoCURRENTITEM)
        {
            if(firstValidPos==-1 && current_subscreen_active->objects[i].d3>=0)
            {
                firstValidPos=i;
            }
            
            if(current_subscreen_active->objects[i].d3==curpos)
            {
                p=i;
                break;
            }
        }
    }
    
    if(p == -1)
    {
        //can't find the current position
        // Switch to a valid weapon if there is one; otherwise,
        // the selector can simply disappear
        if(firstValidPos>=0)
        {
            return current_subscreen_active->objects[firstValidPos].d3;
        }
        //FAILURE
        else
        {
            return failpos;
        }
    }
    
    //remember we've been here
    set<int32_t> oldPositions;
    oldPositions.insert(curpos);
    
    //1. Perform any shifts required by the above
    //2. If that's not possible, go to position 1 and reset the b weapon.
    //2a.  -if we arrive at a position we've already visited, give up and stay there
    //3. Get the weapon at the new slot
    //4. If it's not possible, go to step 1.
    
    for(;;)
    {
        //shift
        switch(type)
        {
        case SEL_LEFT:
        case SEL_VERIFY_LEFT:
            curpos = current_subscreen_active->objects[p].d6;
            break;
            
        case SEL_RIGHT:
        case SEL_VERIFY_RIGHT:
            curpos = current_subscreen_active->objects[p].d7;
            break;
            
        case SEL_DOWN:
            curpos = current_subscreen_active->objects[p].d5;
            break;
            
        case SEL_UP:
            curpos = current_subscreen_active->objects[p].d4;
            break;
        }
        
        //find our new position
        p = -1;
        
        for(int32_t i=0; current_subscreen_active->objects[i].type!=ssoNULL; ++i)
        {
            if(current_subscreen_active->objects[i].type==ssoCURRENTITEM)
            {
                if(current_subscreen_active->objects[i].d3==curpos)
                {
                    p=i;
                    break;
                }
            }
        }
        
        if(p == -1)
        {
            //can't find the current position
            //FAILURE
            return failpos;
        }
        
        //if we've already been here, give up
        if(oldPositions.find(curpos) != oldPositions.end())
        {
            return failpos;
        }
        
        //else, remember we've been here
        oldPositions.insert(curpos);
        
        //see if this weapon is acceptable
        if(Bweapon(curpos) != 0 && curpos != forbiddenpos && curpos != fp2 && curpos != fp3)
            return curpos;
            
        //keep going otherwise
    }
}

// Select the sword for the A button if the 'select A button weapon' quest rule isn't set.
int32_t selectSword()
{
    int32_t ret = current_item_id(itype_sword);
    
    if(ret == -1)
        ret = 0;
        
    return ret;
}

// Adding code here for allowing hardcoding a button to a specific itemclass.
int32_t selectItemclass(int32_t itemclass)
{
    int32_t ret = current_item_id(itemclass);
    
    if(ret == -1)
        ret = 0;
        
    return ret;
}

// Used for the 'Pickup Hearts' item pickup condition.
bool canget(int32_t id)
{
    return id>=0 && (game->get_maxlife()>=(itemsbuf[id].pickup_hearts*game->get_hp_per_heart()));
}

void dospecialmoney(int32_t index)
{
    int32_t tmp=currscr>=128?1:0;
    int32_t priceindex = ((item*)items.spr(index))->PriceIndex;
    
    switch(tmpscr[tmp].room)
    {
    case rINFO:                                             // pay for info
        if(prices[priceindex]!=100000 ) // 100000 is a placeholder price for free items
        {
            
                
            if(!current_item_power(itype_wallet))
	    {
		if (game->get_spendable_rupies() < abs(prices[priceindex])) 
			return;
		int32_t tmpprice = -abs(prices[priceindex]);
		int32_t total = game->get_drupy()-tmpprice;
		total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
		game->set_drupy(game->get_drupy()-total);
		//game->change_drupy(-abs(prices[priceindex]));
	    }
	    if ( current_item_power(itype_wallet)>0 )
	    {
		 game->change_drupy(0);   
	    }
        }
        rectfill(msg_bg_display_buf, 0, 0, msg_bg_display_buf->w, 80, 0);
        rectfill(msg_txt_display_buf, 0, 0, msg_txt_display_buf->w, 80, 0);
        donewmsg(QMisc.info[tmpscr[tmp].catchall].str[priceindex]);
        clear_bitmap(pricesdisplaybuf);
        set_clip_state(pricesdisplaybuf, 1);
        items.del(0);
        
        for(int32_t i=0; i<items.Count(); i++)
            ((item*)items.spr(i))->pickup=ipDUMMY;
            
        // Prevent the prices from being displayed anymore
        for(int32_t i=0; i<3; i++)
        {
            prices[i] = 0;
        }
        
        break;
        
    case rMONEY:                                            // secret money
    {
        ((item*)items.spr(0))->pickup = ipDUMMY;

        prices[0] = tmpscr[tmp].catchall;
        if (!current_item_power(itype_wallet))
            game->change_drupy(prices[0]);
	//game->set_drupy(game->get_drupy()+price); may be needed everywhere

        putprices(false);
        setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
        break;
    }
        
    case rGAMBLE:                                           // gamble
    {
        if(game->get_spendable_rupies()<10 && !current_item_power(itype_wallet)) return; //Why 10? 
        
        unsigned si=(zc_oldrand()%24)*3;
        
        for(int32_t i=0; i<3; i++)
            prices[i]=gambledat[si++];
            
	game->set_drupy(game->get_drupy()+prices[priceindex]);
        putprices(true);
        
        for(int32_t i=1; i<4; i++)
            ((item*)items.spr(i))->pickup=ipDUMMY;
    }
    break;
    
    case rBOMBS:
	{
        if(game->get_spendable_rupies()<abs(tmpscr[tmp].catchall) && !current_item_power(itype_wallet))
            return;
            
		int32_t price = -abs(tmpscr[tmp].catchall);
		int32_t wmedal = current_item_id(itype_wealthmedal);
		if(wmedal >= 0)
		{
			if(itemsbuf[wmedal].flags & ITEM_FLAG1)
				price*=(itemsbuf[wmedal].misc1/100.0);
			else
				price+=itemsbuf[wmedal].misc1;
		}
		
		int32_t total = game->get_drupy()-price;
		total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
		game->set_drupy(game->get_drupy()-total);
        //game->set_drupy(game->get_drupy()+price);
        setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
        game->change_maxbombs(4);
        game->set_bombs(game->get_maxbombs());
        {
            int32_t div = zinit.bomb_ratio;
            
            if(div > 0)
                game->change_maxcounter(4/div, 6);
        }
        
        //also give Link an actual Bomb item
        for(int32_t i=0; i<MAXITEMS; i++)
        {
            if(itemsbuf[i].family == itype_bomb && itemsbuf[i].fam_type == 1)
                getitem(i, true);
        }
        
        ((item*)items.spr(index))->pickup=ipDUMMY+ipFADE;
        fadeclk=66;
        dismissmsg();
        clear_bitmap(pricesdisplaybuf);
        set_clip_state(pricesdisplaybuf, 1);
        //    putscr(scrollbuf,0,0,tmpscr);
        verifyBothWeapons();
        break;
	}
        
    case rARROWS:
	{
        if(game->get_spendable_rupies()<abs(tmpscr[tmp].catchall) && !current_item_power(itype_wallet))
            return;
            
        int32_t price = -abs(tmpscr[tmp].catchall);
		int32_t wmedal = current_item_id(itype_wealthmedal);
		if(wmedal >= 0)
		{
			if(itemsbuf[wmedal].flags & ITEM_FLAG1)
				price*=(itemsbuf[wmedal].misc1/100.0);
			else
				price+=itemsbuf[wmedal].misc1;
		}
		
	int32_t total = game->get_drupy()-price;
	total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
	game->set_drupy(game->get_drupy()-total);

	//game->set_drupy(game->get_drupy()+price);
        setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
        game->change_maxarrows(10);
        game->set_arrows(game->get_maxarrows());
        ((item*)items.spr(index))->pickup=ipDUMMY+ipFADE;
        fadeclk=66;
        dismissmsg();
        clear_bitmap(pricesdisplaybuf);
        set_clip_state(pricesdisplaybuf, 1);
        //    putscr(scrollbuf,0,0,tmpscr);
        verifyBothWeapons();
        break;
	}
        
    case rSWINDLE:
        if(items.spr(index)->id==iRupy)
        {
            if(game->get_spendable_rupies()<abs(tmpscr[tmp].catchall) && !current_item_power(itype_wallet))
                return;
	    int32_t tmpprice = -abs(tmpscr[tmp].catchall);
	    int32_t total = game->get_drupy()-tmpprice;
	    total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
	    game->set_drupy(game->get_drupy()-total);
        }
        else
        {
            if(game->get_maxlife()<=game->get_hp_per_heart())
                return;
                
            game->set_life(zc_max(game->get_life()-game->get_hp_per_heart(),0));
            game->set_maxlife(zc_max(game->get_maxlife()-game->get_hp_per_heart(),(game->get_hp_per_heart())));
        }
        
        setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
        ((item*)items.spr(0))->pickup=ipDUMMY+ipFADE;
        ((item*)items.spr(1))->pickup=ipDUMMY+ipFADE;
        fadeclk=66;
        dismissmsg();
        clear_bitmap(pricesdisplaybuf);
        set_clip_state(pricesdisplaybuf, 1);
        //    putscr(scrollbuf,0,0,tmpscr);
        break;
    }
}

void getitem(int32_t id, bool nosound)
{
    if(id<0)
    {
        return;
    }
    itemdata const& idat = itemsbuf[id&0xFF];
    if(idat.family!=0xFF)
    {
        if(idat.flags & ITEM_GAMEDATA && idat.family != itype_triforcepiece)
        {
            // Fix boomerang sounds.
            int32_t itemid = current_item_id(idat.family);
            
            if(itemid>=0 && (idat.family == itype_brang || idat.family == itype_nayruslove
                             || idat.family == itype_hookshot || idat.family == itype_cbyrna)
                    && sfx_allocated(itemsbuf[itemid].usesound)
                    && idat.usesound != itemsbuf[itemid].usesound)
            {
                stop_sfx(itemsbuf[itemid].usesound);
                cont_sfx(idat.usesound);
            }
            
            game->set_item(id,true);
            
            if(!(idat.flags & ITEM_KEEPOLD))
            {
                if(current_item(idat.family)<idat.fam_type)
                {
                    removeLowerLevelItemsOfFamily(game,itemsbuf,idat.family, idat.fam_type);
                }
            }
            
            // NES consistency: replace all flying boomerangs with the current boomerang.
            if(idat.family==itype_brang)
                for(int32_t i=0; i<Lwpns.Count(); i++)
                {
                    weapon *w = ((weapon*)Lwpns.spr(i));
                    
                    if(w->id==wBrang)
                    {
                        w->LOADGFX(idat.wpn);
                    }
                }
        }
        
        if(idat.count!=-1)
        {
            if(idat.setmax)
            {
                // Bomb bags are a special case; they may be set not to increase super bombs
                if(idat.family==itype_bombbag && idat.count==2 && (idat.flags&16)==0)
                {
                    int32_t max = game->get_maxbombs();
                    
                    if(max<idat.max) max=idat.max;
                    
                    game->set_maxbombs(zc_min(game->get_maxbombs()+idat.setmax,max), false);
                }
                else
                {
                    int32_t max = game->get_maxcounter(idat.count);
                    
                    if(max<idat.max) max=idat.max;
                    
                    game->set_maxcounter(zc_min(game->get_maxcounter(idat.count)+idat.setmax,max), idat.count);
                }
            }
            
            // Amount is an uint16_t, but the range is -9999 to 16383
            // -1 is actually 16385 ... -9999 is 26383, and 0x8000 means use the drain counter
            if(idat.amount&0x3FFF)
            {
                if(idat.amount&0x8000)
                    game->set_dcounter(
                        game->get_dcounter(idat.count)+((idat.amount&0x4000)?-(idat.amount&0x3FFF):idat.amount&0x3FFF), idat.count);
                else
                {
                    if(idat.amount>=16385 && game->get_counter(0)<=idat.amount-16384)
                        game->set_counter(0, idat.count);
                    else
                        // This is too confusing to try and change...
                        game->set_counter(zc_min(game->get_counter(idat.count)+((idat.amount&0x4000)?-(idat.amount&0x3FFF):idat.amount&0x3FFF),game->get_maxcounter(idat.count)), idat.count);
                }
            }
        }
    }
    
    if(idat.playsound&&!nosound)
    {
        sfx(idat.playsound);
    }
    
    //add lower-level items
    if(idat.flags&ITEM_GAINOLD)
    {
        for(int32_t i=idat.fam_type-1; i>0; i--)
        {
            int32_t potid = getItemID(itemsbuf, idat.family, i);
            
            if(potid != -1)
            {
                game->set_item(potid, true);
            }
        }
    }
    
    switch(idat.family)
    {
		case itype_bottlefill:
		{
			if(idat.misc1)
			{
				game->fillBottle(idat.misc1);
			}
		}
		break;
		
		case itype_clock:
		{
			setClock(watch=true);
			
			for(int32_t i=0; i<eMAXGUYS; i++)
				clock_zoras[i]=0;
				
			clockclk=itemsbuf[id&0xFF].misc1;
		}
		break;
		
		case itype_lkey:
			if(game->lvlkeys[dlevel]<255) game->lvlkeys[dlevel]++;
			
			break;
			
		case itype_ring:
		case itype_magicring:
			if((get_bit(quest_rules,qr_OVERWORLDTUNIC) != 0) || (currscr<128 || dlevel))
			{
				ringcolor(false);
			}
			
			break;
			
		case itype_whispring:
		{
			if(idat.flags & ITEM_FLAG1)
			{
				if(LinkSwordClk()==-1) setSwordClk(150);  // Let's not bother applying the divisor.
				
				if(LinkItemClk()==-1) setItemClk(150);  // Let's not bother applying the divisor.
			}
			
			if(idat.power==0)
			{
				setSwordClk(0);
				setItemClk(0);
			}
			
			break;
		}
		
		
		case itype_map:
			game->lvlitems[dlevel]|=liMAP;
			break;
			
		case itype_compass:
			game->lvlitems[dlevel]|=liCOMPASS;
			break;
			
		case itype_bosskey:
			game->lvlitems[dlevel]|=liBOSSKEY;
			break;
			
		case itype_fairy:
		
			game->set_life(zc_min(game->get_life()+(idat.flags&ITEM_FLAG1 ?(int32_t)(game->get_maxlife()*(idat.misc1/100.0)):((idat.misc1*game->get_hp_per_heart()))),game->get_maxlife()));
			game->set_magic(zc_min(game->get_magic()+(idat.flags&ITEM_FLAG2 ?(int32_t)(game->get_maxmagic()*(idat.misc2/100.0)):((idat.misc2*game->get_mp_per_block()))),game->get_maxmagic()));
			break;
			
		case itype_heartpiece:
			game->change_HCpieces(1);
			
			if(game->get_HCpieces()<game->get_hcp_per_hc())
				break;
				
			game->set_HCpieces(0);
			
			for(int32_t i=0; i<MAXITEMS; i++)
			{
				if(itemsbuf[i].family == itype_heartcontainer)
				{
					getitem(i);
					break;
				}
			}
			
			break;
			
		case itype_killem:
			kill_em_all();
			break;
    }
    
    update_subscreens();
    load_Sitems(&QMisc);
    verifyBothWeapons();
}

void takeitem(int32_t id)
{
    game->set_item(id, false);
    
    /* Lower the counters! */
    if(itemsbuf[id].count!=-1)
    {
        if(itemsbuf[id].setmax)
        {
            game->set_maxcounter(game->get_maxcounter(itemsbuf[id].count)-itemsbuf[id].setmax, itemsbuf[id].count);
        }
        
        if(itemsbuf[id].amount&0x3FFF)
        {
            if(itemsbuf[id].amount&0x8000)
                game->set_dcounter(game->get_dcounter(itemsbuf[id].count)-((itemsbuf[id].amount&0x4000)?-(itemsbuf[id].amount&0x3FFF):itemsbuf[id].amount&0x3FFF), itemsbuf[id].count);
            else game->set_counter(game->get_counter(itemsbuf[id].count)-((itemsbuf[id].amount&0x4000)?-(itemsbuf[id].amount&0x3FFF):itemsbuf[id].amount&0x3FFF), itemsbuf[id].count);
        }
    }
    
    switch(itemsbuf[id&0xFF].family)
    {
        // NES consistency: replace all flying boomerangs with the current boomerang.
    case itype_brang:
        if(current_item(itype_brang)) for(int32_t i=0; i<Lwpns.Count(); i++)
            {
                weapon *w = ((weapon*)Lwpns.spr(i));
                
                if(w->id==wBrang)
                {
                    w->LOADGFX(itemsbuf[current_item_id(itype_brang)].wpn);
                }
            }
            
        break;
        
    case itype_heartpiece:
        if(game->get_maxlife()>game->get_hp_per_heart())
        {
            if(game->get_HCpieces()==0)
            {
                game->set_HCpieces(game->get_hcp_per_hc());
                takeitem(iHeartC);
            }
            
            game->change_HCpieces(-1);
        }
        break;
        
    case itype_map:
        game->lvlitems[dlevel]&=~liMAP;
        break;
        
    case itype_compass:
        game->lvlitems[dlevel]&=~liCOMPASS;
        break;
        
    case itype_bosskey:
        game->lvlitems[dlevel]&=~liBOSSKEY;
        break;
        
    case itype_lkey:
        if(game->lvlkeys[dlevel]) game->lvlkeys[dlevel]--;
        break;
		
	case itype_ring:
		if((get_bit(quest_rules,qr_OVERWORLDTUNIC) != 0) || (currscr<128 || dlevel))
		{
			ringcolor(false);
		}
		break;
    }
}

// Attempt to pick up an item. (-1 = check items touching Link.)
void LinkClass::checkitems(int32_t index)
{
	int32_t tmp=currscr>=128?1:0;
	
	if(index==-1)
	{
		if(diagonalMovement)
		{
			index=items.hit(x,y+(bigHitbox?0:8),z,6,6,1);
		}
		else index=items.hit(x,y+(bigHitbox?0:8),z,1,1,1);
	}
	
	if(index==-1)
		return;
		
	// if (tmpscr[tmp].room==rSHOP && boughtsomething==true)
	//   return;
	item* ptr = (item*)items.spr(index);
	int32_t pickup = ((item*)items.spr(index))->pickup;
	int32_t PriceIndex = ((item*)items.spr(index))->PriceIndex;
	int32_t id2 = ((item*)items.spr(index))->id;
	int32_t pstr = ((item*)items.spr(index))->pstring;
	int32_t pstr_flags = ((item*)items.spr(index))->pickup_string_flags;
	//int32_t tempnextmsg;
	bool bottledummy = (pickup&ipCHECK) && tmpscr[tmp].room == rBOTTLESHOP;
	
	if(ptr->fallclk > 0) return; //Don't pick up a falling item
	if(bottledummy) //Dummy bullshit! 
	{
		if(!game->canFillBottle())
			return;
		if(prices[PriceIndex]!=100000) // 100000 is a placeholder price for free items
		{
			if(!current_item_power(itype_wallet))
			{
				if( game->get_spendable_rupies()<abs(prices[PriceIndex]) ) return;
				int32_t tmpprice = -abs(prices[PriceIndex]);
				//game->change_drupy(-abs(prices[priceindex]));
				int32_t total = game->get_drupy()-tmpprice;
				total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
				game->set_drupy(game->get_drupy()-total);
			}
			else //infinite wallet
			{
				game->change_drupy(0);
			}
		}
		boughtsomething=true;
		//make the other shop items untouchable after
		//you buy something
		int32_t count = 0;
		
		for(int32_t i=0; i<3; i++)
		{
			if(QMisc.bottle_shop_types[tmpscr[tmp].catchall].fill[i] != 0)
			{
				++count;
			}
		}
		
		for(int32_t i=0; i<items.Count(); i++)
		{
			if(((item*)items.spr(i))->PriceIndex >-1 && i!=index)
				((item*)items.spr(i))->pickup=ipDUMMY+ipFADE;
		}
		
		int32_t slot = game->fillBottle(QMisc.bottle_shop_types[tmpscr[tmp].catchall].fill[PriceIndex]);
		id2 = find_bottle_for_slot(slot);
		ptr->id = id2;
		pstr = 0;
		pickup |= ipHOLDUP;
	}
	else
	{
		if(itemsbuf[id2].family == itype_bottlefill && !game->canFillBottle())
			return; //No picking these up unless you have a bottle to fill!
		
		if(((pickup&ipTIMER) && (((item*)items.spr(index))->clk2 < 32))&& !(ptr->pickup & ipCANGRAB))
			if(items.spr(index)->id!=iFairyMoving)
				// wait for it to stop flashing, doesn't check for other items yet
				return;
				
		if(pickup&ipENEMY)                                        // item was being carried by enemy
			if(more_carried_items()<=1)  // 1 includes this own item.
				hasitem &= ~2;
				
		if(pickup&ipDUMMY)                                        // dummy item (usually a rupee)
		{
			if(pickup&ipMONEY)
				dospecialmoney(index);
				
			return;
		}
		
		if(get_bit(quest_rules,qr_HEARTSREQUIREDFIX) && !canget(id2))
			return;
			
		if((itemsbuf[id2].flags & ITEM_COMBINE) && current_item(itemsbuf[id2].family)==itemsbuf[id2].fam_type)
			// Item upgrade routine.
		{
			int32_t nextitem = -1;
			
			for(int32_t i=0; i<MAXITEMS; i++)
			{
				// Find the item which is as close to this item's fam_type as possible.
				if(itemsbuf[i].family==itemsbuf[id2].family && itemsbuf[i].fam_type>itemsbuf[id2].fam_type
						&& (nextitem>-1 ? itemsbuf[i].fam_type<=itemsbuf[nextitem].fam_type : true))
				{
					nextitem = i;
				}
			}
			
			if(nextitem>-1)
				id2 = nextitem;
		}
		
		if(pickup&ipCHECK)                                        // check restrictions
			switch(tmpscr[tmp].room)
			{
			case rSP_ITEM:                                        // special item
				if(!canget(id2)) // These ones always need the Hearts Required check
					return;
					
				break;
				
			case rP_SHOP:                                         // potion shop
				if(msg_active)
					return;
					
			case rSHOP:                                           // shop
				if(prices[PriceIndex]!=100000) // 100000 is a placeholder price for free items
				{
					if(!current_item_power(itype_wallet))
					{
						if( game->get_spendable_rupies()<abs(prices[PriceIndex]) ) return;
						int32_t tmpprice = -abs(prices[PriceIndex]);
						//game->change_drupy(-abs(prices[priceindex]));
						int32_t total = game->get_drupy()-tmpprice;
						total = vbound(total, 0, game->get_maxcounter(1)); //Never overflow! Overflow here causes subscreen bugs! -Z
						game->set_drupy(game->get_drupy()-total);
					}
					else //infinite wallet
					{
						game->change_drupy(0);
					}
				}
				boughtsomething=true;
				//make the other shop items untouchable after
				//you buy something
				int32_t count = 0;
				
				for(int32_t i=0; i<3; i++)
				{
					if(QMisc.shop[tmpscr[tmp].catchall].hasitem[i] != 0)
					{
						++count;
					}
				}
				
				for(int32_t i=0; i<items.Count(); i++)
				{
					if(((item*)items.spr(i))->PriceIndex >-1 && i!=index)
						((item*)items.spr(i))->pickup=ipDUMMY+ipFADE;
				}
				
				break;
			}
			
		if(pickup&ipONETIME)    // set mITEM for one-time-only items
		{
			setmapflag(mITEM);

			//Okay so having old source files is a godsend. You wanna know why?
			//Because the issue here was never to so with the wrong flag being set; no it's always been setting the right flag.
			//The problem here is that guy rooms were always checking for getmapflag, which used to have an internal check for the default.
			//The default would be mITEM if currscr was under 128 (AKA not in a cave), and mBELOW if in a cave.
			//However, now the check just always defaults to mBELOW, which causes this bug.
			//This means that this section of code is no longer a bunch of eggshells, cause none of these overcomplicated compats actually solved shit lmao - Dimi
			
			/*
			// WARNING - Item pickups are very volatile due to crazy compatability hacks, eg., supporting
			// broken behavior from early ZC versions. If you change things here please comment on it's purpose.

			// some old quests need picking up a screen item to also disable the BELOW flag (for hunger rooms, etc)
			// What is etc?! We need to check for every valid state here. ~Gleeok
			if(get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW))
			{
				// Most older quests need one-time-pickups to not remove special items, etc.
				if(tmpscr->room==rGRUMBLE)
				{
					setmapflag(mBELOW);
				}
			}
			*/
		}
		else if(pickup&ipONETIME2)                                // set mBELOW flag for other one-time-only items
			setmapflag((currscr < 128 && get_bit(quest_rules, qr_ITEMPICKUPSETSBELOW)) ? mITEM : mBELOW);
		
		if(pickup&ipSECRETS)                                // Trigger secrets if this item has the secret pickup
		{
			if(tmpscr->flags9&fITEMSECRETPERM) setmapflag(mSECRET);
			hidden_entrance(0, true, false, -5);
		}
			
		if(itemsbuf[id2].collect_script)
		{
			//clear the item script stack for a new script
			ri = &(itemCollectScriptData[id2]);
				for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[id2][q] = 0xFFFF;
			ri->Clear();
				//itemCollectScriptData[(id2 & 0xFFF)].Clear();
			//for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[(id2 & 0xFFF)][q] = 0;
			//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, ((id2 & 0xFFF)*-1));
			if ( id2 > 0 && !item_collect_doscript[id2] ) //No collect script on item 0. 
			{
				item_collect_doscript[id2] = 1;
				itemscriptInitialised[id2] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, ((id2)*-1));
				//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
					FFCore.deallocateAllArrays(SCRIPT_ITEM,-(id2));
			}
			else if (!id2 && !item_collect_doscript[id2]) //item 0
			{
				item_collect_doscript[id2] = 1;
				itemscriptInitialised[id2] = 0;
				ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, COLLECT_SCRIPT_ITEM_ZERO);
				//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
					FFCore.deallocateAllArrays(SCRIPT_ITEM,COLLECT_SCRIPT_ITEM_ZERO);
			}
		}
		//Passive item scripts on colelction
		if(itemsbuf[id2].script && ( (itemsbuf[id2].flags&ITEM_PASSIVESCRIPT) && (get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING)) ))
		{
			ri = &(itemScriptData[id2]);
			for ( int32_t q = 0; q < 1024; q++ ) item_stack[id2][q] = 0xFFFF;
			ri->Clear();
			//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[itemid].script, itemid & 0xFFF);
			item_doscript[id2] = 1;
			itemscriptInitialised[id2] = 0;
			//Z_scripterrlog("Link.cpp starting a passive item script.\n");
			ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].script, id2);
						
		}
		getitem(id2);
	}
	
	if(pickup&ipHOLDUP)
	{
		attackclk=0;
		reset_swordcharge();
		
		if(action!=swimming && !IsSideSwim())
			reset_hookshot();
			
		if(msg_onscreen)
		{
			dismissmsg();
		}
		
		clear_bitmap(pricesdisplaybuf);
		
		if(get_bit(quest_rules, qr_OLDPICKUP) || ((tmpscr[tmp].room==rSP_ITEM || tmpscr[tmp].room==rRP_HC || tmpscr[tmp].room==rTAKEONE) && (pickup&ipONETIME2)) || 
		(get_bit(quest_rules, qr_SHOP_ITEMS_VANISH) && (tmpscr[tmp].room==rBOTTLESHOP || tmpscr[tmp].room==rSHOP) && (pickup&ipCHECK)))
		{
			fadeclk=66;
		}
		
		if(id2!=iBombs || action==swimming || get_bit(quest_rules,qr_BOMBHOLDFIX))
		{
			// don't hold up bombs unless swimming or the bomb hold fix quest rule is on
			if(action==swimming)
			{
				action=waterhold1; FFCore.setLinkAction(waterhold1);
			}
			else if(IsSideSwim())
			{
				action=sidewaterhold1; FFCore.setLinkAction(sidewaterhold1);
			}
			else
			{
				action=landhold1; FFCore.setLinkAction(landhold1);
			}
			
			if(ptr->twohand)
			{
				if(action==waterhold1)
				{
					action=waterhold2; FFCore.setLinkAction(waterhold2);
				}
				else if(action==sidewaterhold1)
				{
					action=sidewaterhold2; FFCore.setLinkAction(sidewaterhold2);
				}
				else
				{
					action=landhold2; FFCore.setLinkAction(landhold2);
				}
			}
			
			holdclk=130;
			
			//restart music
			if(get_bit(quest_rules, qr_HOLDNOSTOPMUSIC) == 0)
				music_stop();
				
			holditem=ptr->id; // NES consistency: when combining blue potions, hold up the blue potion.
			freeze_guys=true;
			//show the info string
			 
			
			//if (pstr > 0 ) //&& itemsbuf[index].pstring < msg_count && ( ( itemsbuf[index].pickup_string_flags&itemdataPSTRING_ALWAYS || itemsbuf[index].pickup_string_flags&itemdataPSTRING_IP_HOLDUP ) ) )
			
			int32_t shop_pstr = 0;
			switch(tmpscr[tmp].room)
			{
				case rSHOP:
					shop_pstr = QMisc.shop[tmpscr[tmp].catchall].str[PriceIndex];
					break;
				case rBOTTLESHOP:
					shop_pstr = QMisc.bottle_shop_types[tmpscr[tmp].catchall].str[PriceIndex];
					break;
			}
			if ( (pstr > 0 && pstr < msg_count) || (shop_pstr > 0 && shop_pstr < msg_count) )
			{
				if ( (pstr > 0 && pstr < msg_count) && ( ( ( pstr_flags&itemdataPSTRING_ALWAYS || pstr_flags&itemdataPSTRING_NOMARK || pstr_flags&itemdataPSTRING_IP_HOLDUP || (!(FFCore.GetItemMessagePlayed(id2)))  ) ) ) )
				{
					if ( (!(pstr_flags&itemdataPSTRING_NOMARK)) ) FFCore.SetItemMessagePlayed(id2);
				}
				else pstr = 0;
				if(shop_pstr)
				{
					donewmsg(shop_pstr);
					enqueued_str = pstr;
				}
				else if(pstr)
				{
					donewmsg(pstr);
				}
			}
			
		}
		
		if(itemsbuf[id2].family!=itype_triforcepiece || !(itemsbuf[id2].flags & ITEM_GAMEDATA))
		{
			sfx(tmpscr[0].holdupsfx);
		}
		
		items.del(index);
		
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			weapon *w = (weapon*)Lwpns.spr(i);
			
			if(w->dragging==index)
			{
				w->dragging=-1;
			}
			else if(w->dragging>index)
			{
				w->dragging-=1;
			}
		}
		
		// clear up shop stuff
		if((isdungeon()==0)&&(index!=0))
		{
			if(boughtsomething)
			{
				fadeclk=66;
				
				if(((item*)items.spr(0))->id == iRupy && ((item*)items.spr(0))->pickup & ipDUMMY)
					items.del(0);
					
				for(int32_t i=0; i<Lwpns.Count(); i++)
				{
					weapon *w = (weapon*)Lwpns.spr(i);
					
					if(w->dragging==0)
					{
						w->dragging=-1;
					}
					else if(w->dragging>0)
					{
						w->dragging-=1;
					}
				}
			}
			
			if(msg_onscreen)
			{
				dismissmsg();
			}
			
			clear_bitmap(pricesdisplaybuf);
			set_clip_state(pricesdisplaybuf, 1);
		}
		
		//   items.del(index);
	}
	else
	{
		items.del(index);
		
		for(int32_t i=0; i<Lwpns.Count(); i++)
		{
			weapon *w = (weapon*)Lwpns.spr(i);
			
			if(w->dragging==index)
			{
				w->dragging=-1;
			}
			else if(w->dragging>index)
			{
				w->dragging-=1;
			}
		}
		
		if(msg_onscreen)
		{
			dismissmsg();
		}
	
		//general item pickup message
		//show the info string
		//non-held
		//if ( pstr > 0 ) //&& itemsbuf[index].pstring < msg_count && ( ( itemsbuf[index].pickup_string_flags&itemdataPSTRING_ALWAYS || (!(FFCore.GetItemMessagePlayed(index))) ) ) )
		int32_t shop_pstr = ( tmpscr[tmp].room == rSHOP && PriceIndex>=0 && QMisc.shop[tmpscr[tmp].catchall].str[PriceIndex] > 0 ) ? QMisc.shop[tmpscr[tmp].catchall].str[PriceIndex] : 0;
		if ( (pstr > 0 && pstr < msg_count) || (shop_pstr > 0 && shop_pstr < msg_count) )
		{
			if ( (pstr > 0 && pstr < msg_count) && ( (!(pstr_flags&itemdataPSTRING_IP_HOLDUP)) && ( pstr_flags&itemdataPSTRING_NOMARK || pstr_flags&itemdataPSTRING_ALWAYS || (!(FFCore.GetItemMessagePlayed(id2))) ) ) )
			{
				if ( (!(pstr_flags&itemdataPSTRING_NOMARK)) ) FFCore.SetItemMessagePlayed(id2);
			}
			else pstr = 0;
			if(shop_pstr)
			{
				donewmsg(shop_pstr);
				enqueued_str = pstr;
			}
			else if(pstr)
			{
				donewmsg(pstr);
			}
		}
		
		
		clear_bitmap(pricesdisplaybuf);
		set_clip_state(pricesdisplaybuf, 1);
	}
	
	if(itemsbuf[id2].family==itype_triforcepiece)
	{
		if(itemsbuf[id2].misc2>0) //Small TF Piece
		{
				getTriforce(id2);
		}
		else
		{
				getBigTri(id2);
		}
	}
}

void LinkClass::StartRefill(int32_t refillWhat)
{
    if(!refilling)
    {
        refillclk=21;
        stop_sfx(WAV_ER);
        sfx(WAV_REFILL,128,true);
        refilling=refillWhat;
	if(FFCore.quest_format[vZelda] < 0x255) 
	{
		//Yes, this isn't a QR check. This was implemented before the QRs got bumped up.
		//I attempted to change this check to a quest rule, but here's the issue: this affects
		//triforces and potions as well, not just fairy flags. This means that having a compat rule
		//would result in a rule that is checked by default for every tileset or quest made before
		//2.55, one in a place most people won't check. That means that if they were to go to use
		//the new potion or triforce flags for jinx curing behavior, they'd find that it doesn't work,
		//all because of an obscure compat rule being checked. Most peoples instincts are sadly not
		//"go through the compat rules and turn them all off", so this remains a version check instead
		//of a qr check. Don't make my mistake and waste time trying to change this in vain. -Deedee
		Start250Refill(refillWhat);
	}
	else //use 2.55+ behavior
	{
		if(refill_why>=0) // Item index
		{
			if(itemsbuf[refill_why].family==itype_potion)
			{
				if(itemsbuf[refill_why].flags & ITEM_FLAG3)swordclk=0;
				if(itemsbuf[refill_why].flags & ITEM_FLAG4)itemclk=0;
			}
			else if((itemsbuf[refill_why].family==itype_triforcepiece))
			{
				if(itemsbuf[refill_why].flags & ITEM_FLAG3)swordclk=0;
				if(itemsbuf[refill_why].flags & ITEM_FLAG4)itemclk=0;
			}
		}
		else if(refill_why==REFILL_FAIRY)
		{
			if(!get_bit(quest_rules,qr_NONBUBBLEFAIRIES))swordclk=0;
			if(get_bit(quest_rules,qr_ITEMBUBBLE))itemclk=0;
		}
	}
    }
}

void LinkClass::Start250Refill(int32_t refillWhat){
	if(!refilling)
	{
		refillclk=21;
		stop_sfx(WAV_ER);
		sfx(WAV_REFILL,128,true);
		refilling=refillWhat;
		
		if(refill_why>=0) // Item index
		{
			if((itemsbuf[refill_why].family==itype_potion)&&(!get_bit(quest_rules,qr_NONBUBBLEMEDICINE)))
			{
				swordclk=0;
				
				if(get_bit(quest_rules,qr_ITEMBUBBLE)) itemclk=0;
			}

			if((itemsbuf[refill_why].family==itype_triforcepiece)&&(!get_bit(quest_rules,qr_NONBUBBLETRIFORCE)))
			{
				swordclk=0;
				
				if(get_bit(quest_rules,qr_ITEMBUBBLE)) itemclk=0;
			}
		}
		else if((refill_why==REFILL_FAIRY)&&(!get_bit(quest_rules,qr_NONBUBBLEFAIRIES)))
			{
			swordclk=0;
			
			if(get_bit(quest_rules,qr_ITEMBUBBLE)) itemclk=0;
		}
	}
}

bool LinkClass::refill()
{
    if(refilling==REFILL_NONE || refilling==REFILL_FAIRYDONE)
    {
        return false;
    }
    
    ++refillclk;
    int32_t speed = get_bit(quest_rules,qr_FASTFILL) ? 6 : 22;
    int32_t refill_heart_stop=game->get_maxlife();
    int32_t refill_magic_stop=game->get_maxmagic();
    
    if(refill_why>=0 && itemsbuf[refill_why].family==itype_potion)
    {
        refill_heart_stop=zc_min(potion_life+(itemsbuf[refill_why].flags & ITEM_FLAG1 ?int32_t(game->get_maxlife()*(itemsbuf[refill_why].misc1 /100.0)):((itemsbuf[refill_why].misc1 *game->get_hp_per_heart()))),game->get_maxlife());
        refill_magic_stop=zc_min(potion_magic+(itemsbuf[refill_why].flags & ITEM_FLAG2 ?int32_t(game->get_maxmagic()*(itemsbuf[refill_why].misc2 /100.0)):((itemsbuf[refill_why].misc2 *game->get_mp_per_block()))),game->get_maxmagic());
    }
    
    if(refillclk%speed == 0)
    {
        //   game->life&=0xFFC;
        switch(refill_what)
        {
        case REFILL_LIFE:
            game->set_life(zc_min(refill_heart_stop, (game->get_life()+game->get_hp_per_heart()/2)));
            
            if(game->get_life()>=refill_heart_stop)
            {
                game->set_life(refill_heart_stop);
                //kill_sfx(); //this 1. needs to be pause resme, and 2. needs an item flag.
                for ( int32_t q = 0; q < WAV_COUNT; q++ )
				{
					if ( q == (int32_t)tmpscr->oceansfx ) continue;
					if ( q == (int32_t)tmpscr->bosssfx ) continue;
					stop_sfx(q);
				}
				sfx(WAV_MSG);
                refilling=REFILL_NONE;
                return false;
            }
            
            break;
            
        case REFILL_MAGIC:
            game->set_magic(zc_min(refill_magic_stop, (game->get_magic()+game->get_mp_per_block()/4)));
            
            if(game->get_magic()>=refill_magic_stop)
            {
                game->set_magic(refill_magic_stop);
                //kill_sfx(); //this 1. needs to be pause resme, and 2. needs an item flag.
                for ( int32_t q = 0; q < WAV_COUNT; q++ )
				{
					if ( q == (int32_t)tmpscr->oceansfx ) continue;
					if ( q == (int32_t)tmpscr->bosssfx ) continue;
					stop_sfx(q);
				}
                sfx(WAV_MSG);
                refilling=REFILL_NONE;
                return false;
            }
            
            break;
            
        case REFILL_ALL:
            game->set_life(zc_min(refill_heart_stop, (game->get_life()+game->get_hp_per_heart()/2)));
            game->set_magic(zc_min(refill_magic_stop, (game->get_magic()+game->get_mp_per_block()/4)));
            
            if((game->get_life()>=refill_heart_stop)&&(game->get_magic()>=refill_magic_stop))
            {
                game->set_life(refill_heart_stop);
                game->set_magic(refill_magic_stop);
                //kill_sfx(); //this 1. needs to be pause resme, and 2. needs an item flag.
                for ( int32_t q = 0; q < WAV_COUNT; q++ )
				{
					if ( q == (int32_t)tmpscr->oceansfx ) continue;
					if ( q == (int32_t)tmpscr->bosssfx ) continue;
					stop_sfx(q);
				}
                sfx(WAV_MSG);
                refilling=REFILL_NONE;
                return false;
            }
            
            break;
        }
    }
    
    return true;
}

void LinkClass::getTriforce(int32_t id2)
{
		
	PALETTE flash_pal;
	int32_t refill_frame = ( (itemsbuf[id2].misc5 > 0) ? itemsbuf[id2].misc5 : 88 );
	
	for(int32_t i=0; i<256; i++)
	{
		flash_pal[i] = get_bit(quest_rules,qr_FADE) ? _RGB(63,63,0) : _RGB(63,63,63); 
	}



	//get rid off all sprites but Link
	guys.clear();
	items.clear();
	Ewpns.clear();
	Lwpns.clear();
	Sitems.clear();
	chainlinks.clear();
    
	//decorations.clear();
	if(!COOLSCROLL)
	{
		show_subscreen_items=false;
	}
    
	sfx(itemsbuf[id2].playsound);
	if ( !(itemsbuf[id2].flags & ITEM_FLAG11) ) music_stop();
	
	//If item flag six is enabled, and a sound is set to attributes[2], play that sound.
	if ( (itemsbuf[id2].flags & ITEM_FLAG14) )
	{
		uint8_t playwav = itemsbuf[id2].misc3;
		//zprint2("playwav is: %d\n", playwav);
		sfx(playwav);
		
	}
		
	//itemsbuf[id2].flags & ITEM_FLAG9 : Don't dismiss Messages
	//itemsbuf[id2].flags & ITEM_FLAG10 : Cutscene interrupts action script..
	//itemsbuf[id2].flags & ITEM_FLAG11 : Don't change music.
	//itemsbuf[id2].flags & ITEM_FLAG12 : Run Collect Script Script On Collection
	//itemsbuf[id2].flags & ITEM_FLAG13 : Run Action Script On Collection
	//itemsbuf[id2].flags & ITEM_FLAG14 : Play second sound (WAV) from Attributes[2] (misc2)
	//itemsbuf[id2].flags & ITEM_FLAG15 : No MIDI
    
	if(!(itemsbuf[id2].flags & ITEM_FLAG15)) //No MIDI flag
	{
		if(itemsbuf[id2].misc1)
			jukebox(itemsbuf[id2].misc1+ZC_MIDI_COUNT-1);
		else
			try_zcmusic((char*)moduledata.base_NSF_file,moduledata.tf_track, ZC_MIDI_TRIFORCE);
	}
	if(itemsbuf[id2].flags & ITEM_GAMEDATA)
	{
		game->lvlitems[dlevel]|=liTRIFORCE;
	}
    
	int32_t f=0;
	int32_t x2=0;
	int32_t curtain_x=0;
	int32_t c=0;
	/*if ( (itemsbuf[id2].flags & ITEM_FLAG12) ) //Run collect script This happens w/o the flag. 
		{
			if(itemsbuf[id2].collect_script && !item_collect_doscript[id2])
			{
				//clear the item script stack for a new script
				ri = &(itemCollectScriptData[id2]);
				for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[id2][q] = 0xFFFF;
				ri->Clear();
				//itemCollectScriptData[(id2 & 0xFFF)].Clear();
				//for ( int32_t q = 0; q < 1024; q++ ) item_collect_stack[(id2 & 0xFFF)][q] = 0;
				//ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, ((id2 & 0xFFF)*-1));
				if ( id2 > 0 && !item_collect_doscript[id2] ) //No collect script on item 0. 
				{
					item_collect_doscript[id2] = 1;
					itemscriptInitialised[id2] = 0;
					ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, ((id2)*-1));
					//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
					FFCore.deallocateAllArrays(SCRIPT_ITEM,-(id2));
				}
				else if (!id2 && !item_collect_doscript[id2]) //item 0
				{
					item_collect_doscript[id2] = 1;
					itemscriptInitialised[id2] = 0;
					ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].collect_script, COLLECT_SCRIPT_ITEM_ZERO);
					//if ( !get_bit(quest_rules, qr_ITEMSCRIPTSKEEPRUNNING) )
					FFCore.deallocateAllArrays(SCRIPT_ITEM,COLLECT_SCRIPT_ITEM_ZERO);
				}
			}
		}
		*/
	do
	{
		
		
		if ( (itemsbuf[id2].flags & ITEM_FLAG13) ) //Run action script on collection.
		{
			if ( itemsbuf[id2].script )
			{
				if ( !item_doscript[id2] ) 
				{
					ri = &(itemScriptData[id2]);
					for ( int32_t q = 0; q < 1024; q++ ) item_stack[id2][q] = 0xFFFF;
					ri->Clear();
					item_doscript[id2] = 1;
					itemscriptInitialised[id2] = 0;
					ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].script, id2);
					FFCore.deallocateAllArrays(SCRIPT_ITEM,(id2));
				}
				else
				{
					if ( !(itemsbuf[id2].flags & ITEM_FLAG10) ) //Cutscene halts the script it resumes after cutscene.
						ZScriptVersion::RunScript(SCRIPT_ITEM, itemsbuf[id2].script, id2); //if flag is off, run the script every frame of the cutscene.
				}
			}
		}
		//if ( itemsbuf[id2].misc2 == 2 ) //No cutscene; what if people used '2' on older quests?
		if ( (itemsbuf[id2].flags & ITEM_FLAG12) ) //No cutscene
		{
			return;
		}
		if(f==40)
		{
			actiontype oldaction = action;
			ALLOFF((!(itemsbuf[id2].flags & ITEM_FLAG9)), false);
			action=oldaction;                                    // have to reset this flag
			FFCore.setLinkAction(oldaction);
		}
	
	
		if(f>=40 && f<88)
		{
		    if(get_bit(quest_rules,qr_FADE))
		    {
			//int32_t flashbit = ;
			if((f&(((get_bit(quest_rules,qr_EPILEPSY) || epilepsyFlashReduction)) ? 6 : 3))==0)
			{
			    fade_interpolate(RAMpal,flash_pal,RAMpal,42,0,CSET(6)-1);
			    refreshpal=true;
			}
			
			if((f&3)==2)
			{
			    loadpalset(0,0);
			    loadpalset(1,1);
			    loadpalset(5,5);
			    
			    if(currscr<128) loadlvlpal(DMaps[currdmap].color);
			    else loadlvlpal(0xB); // TODO: Cave/Item Cellar distinction?
			}
		    }
		    else
		    {
			if((f&((get_bit(quest_rules,qr_EPILEPSY)) ? 10 : 7))==0)
			{
			    for(int32_t cs2=2; cs2<5; cs2++)
			    {
				for(int32_t i=1; i<16; i++)
				{
				    RAMpal[CSET(cs2)+i]=flash_pal[CSET(cs2)+i];
				}
			    }
			    
			    refreshpal=true;
			}
			
			if((f&7)==4)
			{
			    if(currscr<128) loadlvlpal(DMaps[currdmap].color);
			    else loadlvlpal(0xB);
			    
			    loadpalset(5,5);
			}
		    }
		}

	
		if(itemsbuf[id2].flags & ITEM_GAMEDATA)
		{
			if(f==refill_frame)
			{
				refill_what=REFILL_ALL;
				refill_why=id2;
				StartRefill(REFILL_ALL);
				refill();
			}
	    
			if(f==(refill_frame+1))
			{
				if(refill())
				{
					--f;
				}
			}
		}
	
		if(itemsbuf[id2].flags & ITEM_FLAG1) // Warp out flag
		{
			if(f>=208 && f<288)
			{
				++x2;
		
				switch(++c)
				{
					case 5:
						c=0;
		    
					case 0:
					case 2:
					case 3:
						++x2;
						break;
				}
			}
	    
			do_dcounters();
	    
			if(f<288)
			{
				curtain_x=x2&0xF8;
				draw_screen_clip_rect_x1=curtain_x;
				draw_screen_clip_rect_x2=255-curtain_x;
				draw_screen_clip_rect_y1=0;
				draw_screen_clip_rect_y2=223;
				//draw_screen_clip_rect_show_link=true;
				//draw_screen(tmpscr);
			}
		}
	
		draw_screen(tmpscr);
		//this causes bugs
		//the subscreen appearing over the curtain effect should now be fixed in draw_screen
		//so this is not necessary -DD
		//put_passive_subscr(framebuf,&QMisc,0,passive_subscreen_offset,false,false);
		
		//Run Triforce Script
		advanceframe(true);
		++f;
	}
	while
	(
		(f < ( (itemsbuf[id2].misc4 > 0) ? itemsbuf[id2].misc4 : 408)) 
		|| (!(itemsbuf[id2].flags & ITEM_FLAG15) /*&& !(itemsbuf[id2].flags & ITEM_FLAG11)*/ && (midi_pos > 0)) 
		|| (/*!(itemsbuf[id2].flags & ITEM_FLAG15) &&*/ !(itemsbuf[id2].flags & ITEM_FLAG11) && (zcmusic!=NULL) && (zcmusic->position<800) ) 
	);   // 800 may not be just right, but it works

	action=none; FFCore.setLinkAction(none);
	holdclk=0;
	draw_screen_clip_rect_x1=0;
	draw_screen_clip_rect_x2=255;
	draw_screen_clip_rect_y1=0;
	draw_screen_clip_rect_y2=223;
	//draw_screen_clip_rect_show_link=true;
	show_subscreen_items=true;
    
	//Warp Link out of item cellars, in 2.10 and earlier quests. -Z ( 16th January, 2019 )
	//Added a QR for this, to Other->2, as `Triforce in Cellar Warps Link Out`. -Z 15th March, 2019 
	if(itemsbuf[id2].flags & ITEM_FLAG1 && ( get_bit(quest_rules,qr_SIDEVIEWTRIFORCECELLAR) ? ( currscr < MAPSCRS192b136 ) : (currscr < MAPSCRSNORMAL) ) )
	{
		sdir=dir;
		dowarp(1,0); //side warp
	}
	else
	{
		if ( !(itemsbuf[id2].flags & ITEM_FLAG11) ) playLevelMusic();
	}
}

void red_shift()
{
    int32_t tnum=176;
    
    // set up the new palette
    for(int32_t i=CSET(2); i < CSET(4); i++)
    {
        int32_t r = (i-CSET(2)) << 1;
        RAMpal[i+tnum].r = r;
        RAMpal[i+tnum].g = r >> 3;
        RAMpal[i+tnum].b = r >> 4;
    }
    
    // color scale the game screen
    for(int32_t y=0; y<168; y++)
    {
        for(int32_t x=0; x<256; x++)
        {
            int32_t c = framebuf->line[y+playing_field_offset][x];
            int32_t r = zc_min(int32_t(RAMpal[c].r*0.4 + RAMpal[c].g*0.6 + RAMpal[c].b*0.4)>>1,31);
            framebuf->line[y+playing_field_offset][x] = (c ? (r+tnum+CSET(2)) : 0);
        }
    }
    
    refreshpal = true;
}



void setup_red_screen_old()
{
    clear_bitmap(framebuf);
    rectfill(scrollbuf, 0, 0, 255, 167, 0);
    
    if(tmpscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG ) do_layer(scrollbuf,1, tmpscr, 0, playing_field_offset, 2);
    
    if(tmpscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ) do_layer(scrollbuf,2, tmpscr, 0, playing_field_offset, 2);
    
    putscr(scrollbuf, 0, 0, tmpscr);
    putscrdoors(scrollbuf,0,0,tmpscr);
    blit(scrollbuf, framebuf, 0, 0, 0, playing_field_offset, 256, 168);
    do_layer(framebuf,0, tmpscr, 0, 0, 2);
    
    if(!(tmpscr->flags7&fLAYER2BG || DMaps[currdmap].flags&dmfLAYER2BG )) do_layer(framebuf,1, tmpscr, 0, 0, 2);
    
    do_layer(framebuf,-2, tmpscr, 0, 0, 2);
    
    if(!(msg_bg_display_buf->clip))
    {
		blit_msgstr_bg(framebuf, 0, 0, 0, playing_field_offset, 256, 168);
    }
    
    if(!(msg_portrait_display_buf->clip))
    {
		blit_msgstr_prt(framebuf, 0, 0, 0, playing_field_offset, 256, 168);
    }
    
    if(!(msg_txt_display_buf->clip))
    {
		blit_msgstr_fg(framebuf, 0, 0, 0, playing_field_offset, 256, 168);
    }
    
    if(!(pricesdisplaybuf->clip))
    {
        masked_blit(pricesdisplaybuf, framebuf,0,0,0,playing_field_offset, 256,168);
    }
    
    //red shift
    // color scale the game screen
    for(int32_t y=0; y<168; y++)
    {
        for(int32_t x=0; x<256; x++)
        {
            int32_t c = framebuf->line[y+playing_field_offset][x];
            int32_t r = zc_min(int32_t(RAMpal[c].r*0.4 + RAMpal[c].g*0.6 + RAMpal[c].b*0.4)>>1,31);
            framebuf->line[y+playing_field_offset][x] = (c ? (r+CSET(2)) : 0);
        }
    }
    
    //  Link->draw(framebuf);
    blit(framebuf,scrollbuf, 0, playing_field_offset, 256, playing_field_offset, 256, 168);
    
    clear_bitmap(framebuf);
    
    if(!((tmpscr->layermap[2]==0||(tmpscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG ))
            && tmpscr->layermap[3]==0
            && tmpscr->layermap[4]==0
            && tmpscr->layermap[5]==0
            && !overheadcombos(tmpscr)))
    {
        if(!(tmpscr->flags7&fLAYER3BG || DMaps[currdmap].flags&dmfLAYER3BG )) do_layer(framebuf,2, tmpscr, 0, 0, 2);
        
        do_layer(framebuf,3, tmpscr, 0, 0, 2);
        do_layer(framebuf,-1, tmpscr, 0, 0, 2);	
        do_layer(framebuf,4, tmpscr, 0, 0, 2);
        do_layer(framebuf,5, tmpscr, 0, 0, 2);
        
        //do an AND masked blit for messages on top of layers
        if(!(msg_txt_display_buf->clip) || !(msg_bg_display_buf->clip) || !(pricesdisplaybuf->clip) || !(msg_portrait_display_buf->clip))
        {
			BITMAP* subbmp = create_bitmap_ex(8,256,168);
			clear_bitmap(subbmp);
			if(!(msg_txt_display_buf->clip) || !(msg_bg_display_buf->clip) || !(msg_portrait_display_buf->clip))
			{
				masked_blit(framebuf, subbmp, 0, playing_field_offset, 0, 0, 256, 168);
				if(!(msg_bg_display_buf->clip)) blit_msgstr_bg(subbmp, 0, 0, 0, 0, 256, 168);
				if(!(msg_portrait_display_buf->clip)) blit_msgstr_prt(subbmp, 0, 0, 0, 0, 256, 168);
				if(!(msg_txt_display_buf->clip)) blit_msgstr_fg(subbmp, 0, 0, 0, 0, 256, 168);
			}
            for(int32_t y=0; y<168; y++)
            {
                for(int32_t x=0; x<256; x++)
                {
                    int32_t c1 = framebuf->line[y+playing_field_offset][x];
                    int32_t c2 = subbmp->line[y][x];
                    int32_t c3 = pricesdisplaybuf->clip ? 0 : pricesdisplaybuf->line[y][x];
                    
                    if(c1 && c3)
                    {
                        framebuf->line[y+playing_field_offset][x] = c3;
                    }
                    else if(c1 && c2)
                    {
                        framebuf->line[y+playing_field_offset][x] = c2;
                    }
                }
            }
			destroy_bitmap(subbmp);
		}
        
        //red shift
        // color scale the game screen
        for(int32_t y=0; y<168; y++)
        {
            for(int32_t x=0; x<256; x++)
            {
                int32_t c = framebuf->line[y+playing_field_offset][x];
                int32_t r = zc_min(int32_t(RAMpal[c].r*0.4 + RAMpal[c].g*0.6 + RAMpal[c].b*0.4)>>1,31);
                framebuf->line[y+playing_field_offset][x] = r+CSET(2);
            }
        }
    }
    
    blit(framebuf,scrollbuf, 0, playing_field_offset, 0, playing_field_offset, 256, 168);
    
    // set up the new palette
    for(int32_t i=CSET(2); i < CSET(4); i++)
    {
        int32_t r = (i-CSET(2)) << 1;
        RAMpal[i].r = r;
        RAMpal[i].g = r >> 3;
        RAMpal[i].b = r >> 4;
    }
    
    refreshpal = true;
}



void slide_in_color(int32_t color)
{
    for(int32_t i=1; i<16; i+=3)
    {
        RAMpal[CSET(2)+i+2] = RAMpal[CSET(2)+i+1];
        RAMpal[CSET(2)+i+1] = RAMpal[CSET(2)+i];
        RAMpal[CSET(2)+i]   = NESpal(color);
    }
    
    refreshpal=true;
}


void LinkClass::heroDeathAnimation()
{
	int32_t f=0;
	int32_t deathclk=0,deathfrm=0;
    
	action=none; FFCore.setLinkAction(dying); //mayhaps a new action of 'gameover'? -Z
	
	kill_sfx();  //call before the onDeath script.
	
	//do
	//{
		
	//	ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_DEATH, SCRIPT_LINK_DEATH);
	//	FFCore.Waitframe();
	//}while(link_doscript);
	//ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_DEATH, SCRIPT_LINK_DEATH);
	//while(link_doscript) { advanceframe(true); } //Not safe. The script runs for only one frame at present.
	
	//Playing=false;
    
	if(!debug_enabled)
	{
		Paused=false;
	}
    /*
	game->set_deaths(zc_min(game->get_deaths()+1,999));
	dir=down;
	music_stop();
	
	attackclk=hclk=superman=0;
	scriptcoldet = 1;
    
	for(int32_t i=0; i<32; i++) miscellaneous[i] = 0;
    
	
    
	playing_field_offset=56; // otherwise, red_shift() may go past the bottom of the screen
	quakeclk=wavy=0;
    
	//in original Z1, Link marker vanishes at death.
	//code in subscr.cpp, put_passive_subscr checks the following value.
	//color 255 is a GUI color, so quest makers shouldn't be using this value.
	//Also, subscreen is static after death in Z1.
	int32_t tmp_link_dot = QMisc.colors.link_dot;
	QMisc.colors.link_dot = 255;
	//doesn't work
	//scrollbuf is tampered with by draw_screen()
	//put_passive_subscr(scrollbuf, &QMisc, 256, passive_subscreen_offset, false, false);//save this and reuse it.
	BITMAP *subscrbmp = create_bitmap_ex(8, framebuf->w, framebuf->h);
	clear_bitmap(subscrbmp);
	put_passive_subscr(subscrbmp, &QMisc, 0, passive_subscreen_offset, false, sspUP);
	QMisc.colors.link_dot = tmp_link_dot;
    */
    
	BITMAP *subscrbmp = create_bitmap_ex(8, framebuf->w, framebuf->h);
				clear_bitmap(subscrbmp);
				//get rid off all sprites but Link
				guys.clear();
				items.clear();
				Ewpns.clear();
				Lwpns.clear();
				Sitems.clear();
				chainlinks.clear();
				decorations.clear();
				Playing = false;
					
				game->set_deaths(zc_min(game->get_deaths()+1,USHRT_MAX));
				dir=down;
				music_stop();
				
				attackclk=hclk=superman=0;
				scriptcoldet = 1;
			    
				for(int32_t i=0; i<32; i++) miscellaneous[i] = 0;
			    
				
			    
				playing_field_offset=56; // otherwise, red_shift() may go past the bottom of the screen
				quakeclk=wavy=0;
			    
				//in original Z1, Link marker vanishes at death.
				//code in subscr.cpp, put_passive_subscr checks the following value.
				//color 255 is a GUI color, so quest makers shouldn't be using this value.
				//Also, subscreen is static after death in Z1.
				int32_t tmp_link_dot = QMisc.colors.link_dot;
				QMisc.colors.link_dot = 255;
				//doesn't work
				//scrollbuf is tampered with by draw_screen()
				//put_passive_subscr(scrollbuf, &QMisc, 256, passive_subscreen_offset, false, false);//save this and reuse it.
				
				put_passive_subscr(subscrbmp, &QMisc, 0, passive_subscreen_offset, false, sspUP);
				//Don't forget passive subscreen scripts!
				if(get_bit(quest_rules, qr_PASSIVE_SUBSCRIPT_RUNS_WHEN_GAME_IS_FROZEN))
				{
					script_drawing_commands.Clear(); //We only want draws from this script
					if(DMaps[currdmap].passive_sub_script != 0)
						ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script, currdmap);
					if(passive_subscreen_waitdraw && DMaps[currdmap].passive_sub_script != 0 && passive_subscreen_doscript != 0)
					{
						ZScriptVersion::RunScript(SCRIPT_PASSIVESUBSCREEN, DMaps[currdmap].passive_sub_script, currdmap);
						passive_subscreen_waitdraw = false;
					}
					BITMAP* tmp = framebuf;
					framebuf = subscrbmp; //Hack; force draws to subscrbmp
					do_script_draws(framebuf, tmpscr, 0, playing_field_offset); //Draw the script draws
					framebuf = tmp;
					script_drawing_commands.Clear(); //Don't let these draws repeat during 'draw_screen()'
				}
				QMisc.colors.link_dot = tmp_link_dot;
        bool clearedit = false;
	do
	{
		//if ( link_doscript ) 
		//{
		//	ZScriptVersion::RunScript(SCRIPT_LINK, SCRIPT_LINK_WIN, SCRIPT_LINK_WIN);
			//if ( f ) --f; 
		//	load_control_state(); //goto adv;
		//}
		//else
		//{
		//	if ( !clearedit )
		//	{
				
				
		//		clearedit = true;
				
		//	}
		//}
		//else Playing = false;
		if(f<254)
		{
			if(f<=32)
			{
				hclk=(32-f);
			}
            
			if(f>=62 && f<138)
			{
				switch((f-62)%20)
				{
				case 0:
					dir=right;
					break;
                    
				case 5:
					dir=up;
					break;
                    
				case 10:
					dir=left;
					break;
                    
				case 15:
					dir=down;
					break;
				}
                
				linkstep();
			}
            
			if(f>=194 && f<208)
			{
				if(f==194)
				{
					action=dying;
					FFCore.setLinkAction(dying);
				}
                    
				extend = 0;
				cs = wpnsbuf[spr_death].csets&15;
				tile = wpnsbuf[spr_death].newtile;
				if(!get_bit(quest_rules,qr_HARDCODED_ENEMY_ANIMS))
				{
					tile += deathfrm;
					f = 206;
					if(++deathclk >= wpnsbuf[spr_death].speed)
					{
						deathclk=0;
						if(++deathfrm >= wpnsbuf[spr_death].frames)
						{
							f = 208;
							deathfrm = 0;
						}
					}
				}
				else if(BSZ)
				{
					tile += (f-194)/3;
				}
				else if(f>=204)
				{
					++tile;
				}
			}
            
			if(f==208)
			{
				if ( dontdraw < 2 ) { dontdraw = 1; }
			}
			if(get_bit(quest_rules,qr_FADE))
			{
				if(f < 170)
				{
					if(f<60)
					{
						draw_screen(tmpscr);
						//reuse our static subscreen
						set_clip_rect(framebuf, 0, 0, framebuf->w, framebuf->h);
						blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
					}
                    
					if(f==60)
					{
						red_shift();
						create_rgb_table_range(&rgb_table, RAMpal, 208, 239, NULL);
						create_zc_trans_table(&trans_table, RAMpal, 128, 128, 128);
						memcpy(&trans_table2, &trans_table, sizeof(COLOR_MAP));
                        
						for(int32_t q=0; q<PAL_SIZE; q++)
						{
							trans_table2.data[0][q] = q;
							trans_table2.data[q][q] = q;
						}
					}
                    
					if(f>=60 && f<=169)
					{
						draw_screen(tmpscr);
						//reuse our static subscreen
						blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
						red_shift();
                        
					}
                    
					if(f>=139 && f<=169)//fade from red to black
					{
						fade_interpolate(RAMpal,black_palette,RAMpal, (f-138)<<1, 224, 255);
						create_rgb_table_range(&rgb_table, RAMpal, 208, 239, NULL);
						create_zc_trans_table(&trans_table, RAMpal, 128, 128, 128);
						memcpy(&trans_table2, &trans_table, sizeof(COLOR_MAP));
                        
						for(int32_t q=0; q<PAL_SIZE; q++)
						{
							trans_table2.data[0][q] = q;
							trans_table2.data[q][q] = q;
						}
                        
						refreshpal=true;
					}
				}
				else //f>=170
				{
					if(f==170)//make Link grayish
					{
						fade_interpolate(RAMpal,black_palette,RAMpal,64, 224, 255);
                        
						for(int32_t i=CSET(6); i < CSET(7); i++)
						{
							int32_t g = (RAMpal[i].r + RAMpal[i].g + RAMpal[i].b)/3;
							RAMpal[i] = _RGB(g,g,g);
						}
                        
						refreshpal = true;
					}
                    
					//draw only link. otherwise black layers might cover him.
					rectfill(framebuf,0,playing_field_offset,255,167+playing_field_offset,0);
					draw(framebuf);
					blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
				}
			}
			else //!qr_FADE
			{
				if(f==58)
				{
					for(int32_t i = 0; i < 96; i++)
						tmpscr->cset[i] = 3;
                        
					for(int32_t j=0; j<6; j++)
						if(tmpscr->layermap[j]>0)
							for(int32_t i=0; i<96; i++)
								tmpscr2[j].cset[i] = 3;
				}
                
				if(f==59)
				{
					for(int32_t i = 96; i < 176; i++)
						tmpscr->cset[i] = 3;
                        
					for(int32_t j=0; j<6; j++)
						if(tmpscr->layermap[j]>0)
							for(int32_t i=96; i<176; i++)
								tmpscr2[j].cset[i] = 3;
				}
                
				if(f==60)
				{
					for(int32_t i=0; i<176; i++)
					{
						tmpscr->cset[i] = 2;
					}
                    
					for(int32_t j=0; j<6; j++)
						if(tmpscr->layermap[j]>0)
							for(int32_t i=0; i<176; i++)
								tmpscr2[j].cset[i] = 2;
                                
					for(int32_t i=1; i<16; i+=3)
					{
						RAMpal[CSET(2)+i]   = NESpal(0x17);
						RAMpal[CSET(2)+i+1] = NESpal(0x16);
						RAMpal[CSET(2)+i+2] = NESpal(0x26);
					}
                    
					refreshpal=true;
				}
                
				if(f==139)
					slide_in_color(0x06);
                    
				if(f==149)
					slide_in_color(0x07);
                    
				if(f==159)
					slide_in_color(0x0F);
                    
				if(f==169)
				{
					slide_in_color(0x0F);
					slide_in_color(0x0F);
				}
                
				if(f==170)
				{
					for(int32_t i=1; i<16; i+=3)
					{
						RAMpal[CSET(6)+i]   = NESpal(0x10);
						RAMpal[CSET(6)+i+1] = NESpal(0x30);
						RAMpal[CSET(6)+i+2] = NESpal(0x00);
						refreshpal = true;
					}
				}
                
				if(f < 169)
				{
					draw_screen(tmpscr);
					//reuse our static subscreen
					blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
				}
				else
				{
					//draw only link. otherwise black layers might cover him.
					rectfill(framebuf,0,playing_field_offset,255,167+playing_field_offset,0);
					draw(framebuf);
					blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
				}
			}
		}
		
		else if(f<350)//draw 'GAME OVER' text
		{
			if(get_bit(quest_rules, qr_INSTANT_RESPAWN) && !get_bit(quest_rules, qr_INSTANT_CONTINUE))
			{
				Quit = qRELOAD;
				skipcont = 1;
				clear_bitmap(framebuf);
				blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
			}
			else if(!get_bit(quest_rules, qr_INSTANT_RESPAWN) && get_bit(quest_rules, qr_INSTANT_CONTINUE))
			{
				Quit = qCONT;
				skipcont = 1;
				clear_bitmap(framebuf);
				blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
			}
			else
			{
				clear_to_color(framebuf,SaveScreenSettings[SAVESC_BACKGROUND]);
				blit(subscrbmp,framebuf,0,0,0,0,256,passive_subscreen_height);
				textout_ex(framebuf,zfont,"GAME OVER",96,playing_field_offset+80,SaveScreenSettings[SAVESC_TEXT],-1);
			}
		}
		else
		{
			clear_bitmap(framebuf);
		}
        
		//SFX... put them all here
		switch(f)
		{
		case   0:
			sfx(getHurtSFX(),pan(x.getInt()));
			break;
			//Death sound.
		case  60:
			sfx(WAV_SPIRAL);
			break;
			//Message sound. 
		case 194:
			sfx(WAV_MSG);
			break;
		}
		//adv:
		advanceframe(true);
		++f;
		//if (!link_doscript ) ++f;
	}
	while(f<353 && !Quit);
    
	destroy_bitmap(subscrbmp);
	action=none; FFCore.setLinkAction(none);
	if ( dontdraw < 2 ) { dontdraw=0; }
}


void LinkClass::ganon_intro()
{
    /*
    ************************
    * GANON INTRO SEQUENCE *
    ************************
    -25 DOT updates
    -24 LINK in
    0 TRIFORCE overhead - code begins at this point (f == 0)
    47 GANON in
    58 LIGHT step
    68 LIGHT step
    78 LIGHT step
    255 TRIFORCE out
    256 TRIFORCE in
    270 TRIFORCE out
    271 GANON out, LINK face up
    */
    loaded_guys=true;
    loaditem();
    
    if(game->lvlitems[dlevel]&liBOSS)
    {
        return;
    }
    
    dir=down;
    if ( !isSideViewLink() )
    {
	fall = 0; //Fix midair glitch on holding triforce. -Z
	z = 0;
    }
    action=landhold2; FFCore.setLinkAction(landhold2);
    holditem=getItemID(itemsbuf,itype_triforcepiece, 1);
    //not good, as this only returns the highest level that Link possesses. -DD
    //getHighestLevelOfFamily(game, itemsbuf, itype_triforcepiece, false));
    
    for(int32_t f=0; f<271 && !Quit; f++)
    {
        if(f==47)
        {
            music_stop();
            stop_sfx(WAV_ROAR);
            sfx(WAV_GASP);
            sfx(WAV_GANON);
            int32_t Id=0;
            
            for(int32_t i=0; i<eMAXGUYS; i++)
            {
                if(guysbuf[i].flags2&eneflag_ganon)
                {
                    Id=i;
                    break;
                }
            }
            
            if(current_item(itype_ring))
            {
                addenemy(160,96,Id,0);
            }
            else
            {
                addenemy(80,32,Id,0);
            }
        }
        
        if(f==48)
        {
            lighting(true,true); // Hmm. -L
            f += 30;
        }
        
        //NES Z1, the triforce vanishes for one frame in two cases
        //while still showing Link's two-handed overhead sprite.
	//This should be a Quest Rule for NES Accuracy. -Z
        if(f==255 || f==270)
        {
            holditem=-1;
        }
        
        if(f==256)
        {
            holditem=getItemID(itemsbuf,itype_triforcepiece,1);
        }
        
        draw_screen(tmpscr);
        advanceframe(true);
        
        if(rSbtn())
        {
            conveyclk=3;
            int32_t tmp_subscr_clk = frame;
            dosubscr(&QMisc);
            newscr_clk += frame - tmp_subscr_clk;
        }
        
    }
    
    action=none; FFCore.setLinkAction(none);
    dir=up;
    
    if((!getmapflag() || (tmpscr->flags9&fBELOWRETURN)) && (tunes[MAXMIDIS-1].data))
        jukebox(MAXMIDIS-1);
    else
        playLevelMusic();
        
    currcset=DMaps[currdmap].color;
    if (get_bit(quest_rules, qr_GANONINTRO) ) 
    {
	dointro();
	//Yes, I checked. This is literally in 2.10 (minus this if statement of course).
	//I have no clue why it's here; Literally the only difference between dointro in 2.10 and dointro in this version is an 'else' that sets introclk and intropos to 74.
	//I have no idea what was going through the original devs heads and I'm extremely worried I'm missing something, cause at first glance this looks like 
	//a hack solution to an underlying bug, but no! There's just a fucking dointro() call in older versions and I don't know *why*. -Deedee
    }
    //dointro(); //This is likely what causes Ganon Rooms to repeat the DMap intro.  
    //I suppose it is to allow the user to make Gaanon rooms have their own dialogue, if they are
    //on a different DMap. 
    //~ Otherwise, why is it here?! -Z
    
    
    //if ( !(DMaps[currdmap].flags&dmfALWAYSMSG) ) { dointro(); } //This is likely what causes Ganon Rooms to repeat the DMap intro.  
    //If we try it this way: The dmap flag /always display intro string/ is probably why James had this issue. 
    
    //The only fix that I can think of, off the top of me head, is either a QR or a Screen Flag to disable the intro text.
    //Users who use that dmap rule should put ganons room on its own DMap! -Z 
    cont_sfx(WAV_ROAR);	
}

void LinkClass::saved_Zelda()
{
    Playing=Paused=false;
    action=won; FFCore.setLinkAction(won);
    Quit=qWON;
    hclk=0;
    x = 136;
    y = (isdungeon() && currscr<128) ? 75 : 73;
    z = fall = spins = 0;
    dir=left;
}

void LinkClass::reset_swordcharge()
{
    charging=spins=tapping=0;
}

void LinkClass::reset_hookshot()
{
    if(action!=rafting && action!=landhold1 && action!=landhold2 && action!=sidewaterhold1 && action!=sidewaterhold2)
    {
        action=none; FFCore.setLinkAction(none);
    }
    
    hookshot_frozen=false;
    hookshot_used=false;
    pull_link=false;
    add_chainlink=false;
    del_chainlink=false;
    hs_fix=false;
    Lwpns.del(Lwpns.idFirst(wHSHandle));
    Lwpns.del(Lwpns.idFirst(wHookshot));
    chainlinks.clear();
    int32_t index=directItem>-1 ? directItem : current_item_id(itype_hookshot);
    
    if(index>=0)
    {
        stop_sfx(itemsbuf[index].usesound);
    }
    
    hs_xdist=0;
    hs_ydist=0;
}


bool LinkClass::can_deploy_ladder()
{
    bool ladderallowed = ((!get_bit(quest_rules,qr_LADDERANYWHERE) && tmpscr->flags&fLADDER) || isdungeon()
                          || (get_bit(quest_rules,qr_LADDERANYWHERE) && !(tmpscr->flags&fLADDER)));
    return (current_item_id(itype_ladder)>-1 && ladderallowed && !ilswim && z==0 &&
            (!isSideViewLink() || on_sideview_solid(x,y)));
}

void LinkClass::reset_ladder()
{
    ladderx=laddery=0;
}

void LinkClass::check_conveyor()
{
	if(action==casting||action==sideswimcasting||action==drowning || action==sidedrowning||action==lavadrowning||inlikelike||pull_link||(z>0 && !(tmpscr->flags2&fAIRCOMBOS)))
	{
		return;
	}
	
	WalkflagInfo info;
	int32_t xoff,yoff;
	zfix deltax(0), deltay(0);
	int32_t cmb = MAPCOMBO(x+7,y+(bigHitbox?8:12));
	++newconveyorclk;
	if (newconveyorclk < 0) newconveyorclk = 0;
	if((combobuf[cmb].usrflags&cflag2) || (!(combobuf[cmb].usrflags&cflag2) && conveyclk<=0)) //!DIMITODO: let player be on multiple conveyors at once
	{
		is_on_conveyor=false;
		int32_t ctype;
		ctype=(combobuf[cmb].type);
		for (int32_t i = 0; i <= 1; ++i)
		{
			if(tmpscr2[i].valid!=0)
			{
				if (combobuf[MAPCOMBO2(i,x+7,y+(bigHitbox?8:12))].type == cBRIDGE && !_walkflag_layer(x+7,y+(bigHitbox?8:12),1, &(tmpscr2[i]))) return;
			}
		}
		if (!_effectflag(x+7,y+(bigHitbox?8:12),1, -1)) return;
		if((combobuf[cmb].usrflags&cflag2) && (newconveyorclk % zc_max(combobuf[cmb].attribytes[0], 1))) return;
		
		deltax=combo_class_buf[ctype].conveyor_x_speed;
		deltay=combo_class_buf[ctype].conveyor_y_speed;
	
		if ((deltax!=0 || deltay!=0) && combobuf[cmb].usrflags&cflag2)
		{
			deltax = zslongToFix(combobuf[cmb].attributes[0]);
			deltay = zslongToFix(combobuf[cmb].attributes[1]);
		}
		
		if((deltax==0&&deltay==0)&&(isSideViewLink() && on_sideview_solid(x,y)))
		{
			cmb = MAPCOMBO(x+8,y+16);
			ctype=(combobuf[cmb].type);
			deltax=combo_class_buf[ctype].conveyor_x_speed;
			deltay=combo_class_buf[ctype].conveyor_y_speed;
			if (combobuf[cmb].usrflags&cflag2)
			{
				deltax = zslongToFix(combobuf[cmb].attributes[0]);
				deltay = zslongToFix(combobuf[cmb].attributes[1]);
			}
		}
		
		if(deltax!=0||deltay!=0)
		{
			is_on_conveyor=true;
		}
		
		if(deltay<0)
		{
			info = walkflag(x,y+8-(bigHitbox*8)-2,2,up);
			execute(info);
			
			if(!info.isUnwalkable())
			{
				zfix step(0);
				
				if((DrunkRight()||DrunkLeft())&&dir!=left&&dir!=right&&!(diagonalMovement||NO_GRIDLOCK))
				{
					while(step<(abs(deltay)*(isSideViewLink()?2:1)))
					{
						yoff=int32_t(y-step)&7;
						
						if(!yoff) break;
						
						step++;
					}
				}
				else
				{
					step=abs(deltay);
				}
				
				y=y-step;
				hs_starty-=step.getInt();
				
				for(int32_t j=0; j<chainlinks.Count(); j++)
				{
					chainlinks.spr(j)->y-=step;
				}
				
				if(Lwpns.idFirst(wHookshot)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHookshot))->y-=step;
				}
				
				if(Lwpns.idFirst(wHSHandle)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHSHandle))->y-=step;
				}
			}
			else checkdamagecombos(x,y+8-(bigHitbox ? 8 : 0)-2);
		}
		else if(deltay>0)
		{
			info = walkflag(x,y+15+2,2,down);
			execute(info);
			
			if(!info.isUnwalkable())
			{
				zfix step(0);
				
				if((DrunkRight()||DrunkLeft())&&dir!=left&&dir!=right&&!(diagonalMovement||NO_GRIDLOCK))
				{
					while(step<abs(deltay))
					{
						yoff=int32_t(y+step)&7;
						
						if(!yoff) break;
						
						step++;
					}
				}
				else
				{
					step=abs(deltay);
				}
				
				y=y+step;
				hs_starty+=step.getInt();
				
				for(int32_t j=0; j<chainlinks.Count(); j++)
				{
					chainlinks.spr(j)->y+=step;
				}
				
				if(Lwpns.idFirst(wHookshot)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHookshot))->y+=step;
				}
				
				if(Lwpns.idFirst(wHSHandle)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHSHandle))->y+=step;
				}
			}
			else checkdamagecombos(x,y+15);
		}
		
		if(deltax<0)
		{
			info = walkflag(x-int32_t(lsteps[x.getInt()&7]),y+8-(bigHitbox ? 8 : 0),1,left);
			execute(info);
			
			if(!info.isUnwalkable())
			{
				zfix step(0);
				
				if((DrunkUp()||DrunkDown())&&dir!=up&&dir!=down&&!(diagonalMovement||NO_GRIDLOCK))
				{
					while(step<abs(deltax))
					{
						xoff=int32_t(x-step)&7;
						
						if(!xoff) break;
						
						step++;
					}
				}
				else
				{
					step=abs(deltax);
				}
				
				x=x-step;
				hs_startx-=step.getInt();
				
				for(int32_t j=0; j<chainlinks.Count(); j++)
				{
					chainlinks.spr(j)->x-=step;
				}
				
				if(Lwpns.idFirst(wHookshot)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHookshot))->x-=step;
				}
				
				if(Lwpns.idFirst(wHSHandle)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHSHandle))->x-=step;
				}
			}
			else checkdamagecombos(x-int32_t(lsteps[x.getInt()&7]),y+8-(bigHitbox ? 8 : 0));
		}
		else if(deltax>0)
		{
			info = walkflag(x+15+2,y+8-(bigHitbox ? 8 : 0),1,right);
			execute(info);
			
			if(!info.isUnwalkable())
			{
				zfix step(0);
				
				if((DrunkUp()||DrunkDown())&&dir!=up&&dir!=down&&!(diagonalMovement||NO_GRIDLOCK))
				{
					while(step<abs(deltax))
					{
						xoff=int32_t(x+step)&7;
						
						if(!xoff) break;
						
						step++;
					}
				}
				else
				{
					step=abs(deltax);
				}
				
				x=x+step;
				hs_startx+=step.getInt();
				
				for(int32_t j=0; j<chainlinks.Count(); j++)
				{
					chainlinks.spr(j)->x+=step;
				}
				
				if(Lwpns.idFirst(wHookshot)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHookshot))->x+=step;
				}
				
				if(Lwpns.idFirst(wHSHandle)>-1)
				{
					Lwpns.spr(Lwpns.idFirst(wHSHandle))->x+=step;
				}
			}
			else checkdamagecombos(x+15+2,y+8-(bigHitbox ? 8 : 0));
		}
	}
}

void LinkClass::setNayrusLoveShieldClk(int32_t newclk)
{
    NayrusLoveShieldClk=newclk;
    
    if(decorations.idCount(dNAYRUSLOVESHIELD)==0)
    {
        decoration *dec;
        decorations.add(new dNayrusLoveShield(LinkX(), LinkY(), dNAYRUSLOVESHIELD, 0));
        decorations.spr(decorations.Count()-1)->misc=0;
        decorations.add(new dNayrusLoveShield(LinkX(), LinkY(), dNAYRUSLOVESHIELD, 0));
        dec=(decoration *)decorations.spr(decorations.Count()-1);
        decorations.spr(decorations.Count()-1)->misc=1;
    }
}

int32_t LinkClass::getNayrusLoveShieldClk()
{
    return NayrusLoveShieldClk;
}

int32_t LinkClass::getHoverClk()
{
    return hoverclk;
}

int32_t LinkClass::getHoldClk()
{
    return holdclk;
}

int32_t LinkClass::getLastLensID(){
	return last_lens_id;
}

void LinkClass::setLastLensID(int32_t p_item){
	last_lens_id = p_item;
}

bool LinkClass::getOnSideviewLadder()
{
	return on_sideview_ladder;
}

void LinkClass::setOnSideviewLadder(bool val)
{
	if(val)
	{
		fall = hoverclk = jumping = 0;
		hoverflags = 0;
	}
	on_sideview_ladder = val;
}

bool LinkClass::canSideviewLadder(bool down)
{
	if(!isSideViewLink()) return false;
	if(jumping < 0) return false;
	if(down && get_bit(quest_rules, qr_DOWN_DOESNT_GRAB_LADDERS))
	{
		bool onSolid = on_sideview_solid(x,y,true);
		return ((isSVLadder(x+4,y+16) && (!isSVLadder(x+4,y)||onSolid)) || (isSVLadder(x+12,y+16) && (!isSVLadder(x+12,y)||onSolid)));
	}
	//Are you presently able to climb a sideview ladder?
	//x+4 / +12 are the offsets used for detecting a platform below you in sideview
	//y+0 checks your top-half for large hitbox; y+8 for small
	//y+15 checks if you are on one at all. This is necessary so you don't just fall off before reaching the top.
	//y+16 check is for going down onto a ladder you are standing on.
	return (isSVLadder(x+4,y+(bigHitbox?0:8)) || isSVLadder(x+12,y+(bigHitbox?0:8)))
		|| isSVLadder(x+4,y+15) || isSVLadder(x+12,y+15)
		|| (down && (isSVLadder(x+4,y+16) || isSVLadder(x+12,y+16)));
}

bool LinkClass::canSideviewLadderRemote(int32_t wx, int32_t wy, bool down)
{
	if(!isSideViewLink()) return false;
	if(jumping < 0) return false;
	if(down && get_bit(quest_rules, qr_DOWN_DOESNT_GRAB_LADDERS))
	{
		bool onSolid = on_sideview_solid(x,y,true);
		return ((isSVLadder(wx+4,wy+16) && (!isSVLadder(wx+4,wy)||onSolid)) || (isSVLadder(wx+12,wy+16) && (!isSVLadder(wx+12,wy)||onSolid)));
	}
	//Are you presently able to climb a sideview ladder?
	//x+4 / +12 are the offsets used for detecting a platform below you in sideview
	//y+0 checks your top-half for large hitbox; y+8 for small
	//y+15 checks if you are on one at all. This is necessary so you don't just fall off before reaching the top.
	//y+16 check is for going down onto a ladder you are standing on.
	return (isSVLadder(wx+4,wy+(bigHitbox?0:8)) || isSVLadder(wx+12,wy+(bigHitbox?0:8)))
		|| isSVLadder(wx+4,wy+15) || isSVLadder(wx+12,wy+15)
		|| (down && (isSVLadder(wx+4,wy+16) || isSVLadder(wx+12,wy+16)));
}

void LinkClass::execute(LinkClass::WalkflagInfo info)
{
    int32_t flags = info.getFlags();
    
    if(flags & WalkflagInfo::CLEARILSWIM)
        ilswim =false;
    else if(flags & WalkflagInfo::SETILSWIM)
        ilswim = true;
        
    if(flags & WalkflagInfo::CLEARCHARGEATTACK)
    {
        charging = 0;
        attackclk = 0;
    }
    
    if(flags & WalkflagInfo::SETDIR)
    {
        dir = info.getDir();
    }
    
    if(flags & WalkflagInfo::SETHOPCLK)
    {
        hopclk = info.getHopClk();
    }
    
    if(flags & WalkflagInfo::SETHOPDIR)
    {
        hopdir = info.getHopDir();
    }
    
}

LinkClass::WalkflagInfo LinkClass::WalkflagInfo::operator ||(LinkClass::WalkflagInfo other)
{
    LinkClass::WalkflagInfo ret;
    ret.newhopclk = newhopclk;
    ret.newdir = newdir;
    ret.newhopdir = (other.newhopdir >-1 ? other.newhopdir : newhopdir);
    
    int32_t flags1 = (flags & ~UNWALKABLE) & (other.flags & ~UNWALKABLE);
    int32_t flags2 = (flags & UNWALKABLE) | (other.flags & UNWALKABLE);
    ret.flags = flags1 | flags2;
    return ret;
}

LinkClass::WalkflagInfo LinkClass::WalkflagInfo::operator &&(LinkClass::WalkflagInfo other)
{
    LinkClass::WalkflagInfo ret;
    ret.newhopclk = newhopclk;
    ret.newdir = newdir;
    ret.newhopdir = (other.newhopdir >-1 ? other.newhopdir : newhopdir);
    
    ret.flags = flags & other.flags;
    return ret;
}

LinkClass::WalkflagInfo LinkClass::WalkflagInfo::operator !()
{
    LinkClass::WalkflagInfo ret;
    ret.newhopclk = newhopclk;
    ret.newdir = newdir;
    ret.newhopdir = newhopdir;
    
    ret.flags = flags ^ UNWALKABLE;
    return ret;
}

void LinkClass::explode(int32_t type)
{
	static int32_t tempx, tempy;
	static byte linktilebuf[256];
	int32_t ltile=0;
	int32_t lflip=0;
	bool shieldModify=true;
	unpack_tile(newtilebuf, tile, flip, true);
	memcpy(linktilebuf, unpackbuf, 256);
	tempx=Link.getX();
	tempy=Link.getY();
	for(int32_t i=0; i<16; ++i)
	{
                for(int32_t j=0; j<16; ++j)
                {
                    if(linktilebuf[i*16+j])
                    {
                        if(type==0)  // Twilight
                        {
                            particles.add(new pTwilight(Link.getX()+j, Link.getY()-Link.getZ()+i, 5, 0, 0, (zc_oldrand()%8)+i*4));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=3;
                        }
                        else if(type ==1)  // Sands of Hours
                        {
                            particles.add(new pTwilight(Link.getX()+j, Link.getY()-Link.getZ()+i, 5, 1, 2, (zc_oldrand()%16)+i*2));
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
                            particles.add(new pFaroresWindDust(Link.getX()+j, Link.getY()-Link.getZ()+i, 5, 6, linktilebuf[i*16+j], zc_oldrand()%96));
                            
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->angular=true;
                            p->angle=zc_oldrand();
                            p->step=(((double)j)/8);
                            p->yofs=Link.getYOfs();
                        }
                    }
                }
	}
}

void LinkClass::SetSwim()
{
	if (CanSideSwim()) 
	{
		if (action != sideswimattacking && action != attacking) {action=sideswimming; FFCore.setLinkAction(sideswimming);}
		else {action=sideswimattacking; FFCore.setLinkAction(sideswimattacking);}
		if (get_bit(quest_rules,qr_SIDESWIMDIR) && spins <= 0 && dir != left && dir != right) dir = sideswimdir;
	}
        else {action=swimming; FFCore.setLinkAction(swimming);}
}

void LinkClass::SetAttack()
{
	if (IsSideSwim()) {action=sideswimattacking; FFCore.setLinkAction(sideswimattacking);}
        else {action=attacking; FFCore.setLinkAction(attacking);}
}

bool LinkClass::IsSideSwim()
{
	return (action==sideswimming || action==sideswimhit || action == sideswimattacking || action == sidewaterhold1 || action == sidewaterhold2 || action == sideswimcasting || action == sideswimfreeze);
}

bool LinkClass::CanSideSwim()
{
	return (isSideViewLink() && get_bit(quest_rules,qr_SIDESWIM));
}

int32_t LinkClass::getTileModifier()
{
	return item_tile_mod() + bunny_tile_mod();
}
/*** end of link.cpp ***/




