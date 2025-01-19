#ifndef HERO_H_
#define HERO_H_

#include "base/zc_alleg.h"
#include "sound/zcmusic.h"
#include "base/zdefs.h"
#include "zc/zelda.h"
#include "zc/maps.h"
#include "tiles.h"
#include "base/colors.h"
#include "pal.h"
#include "base/zsys.h"
#include "base/qst.h"
#include "zc/matrix.h"
#include "gui/jwin.h"
#include "base/jwinfsel.h"
#include "zc/weapons.h"
//#include "save_gif.h"
#include "sprite.h"
#include "zc/zc_custom.h"
#include "subscr.h"
#include "base/zfix.h"
#include "status_fx.h"
#include <vector>

extern movingblock mblock2;                                 //mblock[4]?
extern sprite_list  guys, items, Ewpns, Lwpns, chainlinks, decorations, portals;

extern byte lsteps[8];

enum { did_fairy=1, did_candle=2, did_whistle=4, did_magic=8, did_glove=16, did_all=32 };

enum actiontype
{
	none, walking, attacking, freeze, landhold1, landhold2,
	rafting, gothit, inwind, scrolling, won, swimming, hopping,
	swimhit, waterhold1, waterhold2, casting, climbcovertop,
	climbcoverbottom, dying, drowning, 
	climbing, //not used -Z.
	// Fake actiontypes: used by ZScripts
	ischarging, isspinning, isdiving, gameover, hookshotout, stunned, ispushing,
	// New 2.55 ActionTypes
	falling, lavadrowning, sideswimming, sideswimhit, sideswimattacking, 
	sidewaterhold1, sidewaterhold2, sideswimcasting, sideswimfreeze, sidedrowning,
	sideswimisspinning, sideswimischarging, lifting, la_max
};

typedef struct tilesequence
{
	word tile;
	byte frames;                                              // animation frame count
	byte speed;                                               // animation speed
	byte exp;                                                 // not used
} tilesequence;

#define SWORDCHARGEFRAME 20
#define SWORDTAPFRAME SWORDCHARGEFRAME-8
#define HAMMERCHARGEFRAME 11
#define WANDCHARGEFRAME 12

// Values for specialcave
#define ITEMCELLAR 1
#define PASSAGEWAY 2
#define GUYCAVE 3
#define STAIRCAVE 4

#define HOV_INF 0x01
#define HOV_OUT 0x02
#define HOV_PITFALL_OUT 0x04

#define DYING_FORCED 0x01
#define DYING_NOREV  0x02

class HeroClass : public sprite
{
	class WalkflagInfo
	{
	public:
		WalkflagInfo() : flags(0), newhopclk(0), newhopdir(0), newdir(0), newladderdir(0),
			newladderx(0), newladdery(0), newladderstart(0) {}
		static const int32_t UNWALKABLE = 1;
		static const int32_t SETILSWIM = 2;
		static const int32_t CLEARILSWIM = 4;
		static const int32_t SETHOPCLK = 8;
		static const int32_t SETDIR = 16;
		static const int32_t CLEARCHARGEATTACK = 32;
		static const int32_t SETHOPDIR = 64;
		
		int32_t getHopClk()
		{
			return newhopclk;
		}
	
	
	
		int32_t getHopDir()
		{
			return newhopdir;
		}
		int32_t getDir()
		{
			return newdir;
		}
		int32_t getLadderdir()
		{
			return newladderdir;
		}
		int32_t getLadderx()
		{
			return newladderx;
		}
		int32_t getLaddery()
		{
			return newladdery;
		}
		int32_t getLadderstart()
		{
			return newladderstart;
		}
		
		void setUnwalkable(bool val)
		{
			if(val) flags |= UNWALKABLE;
			else flags &= ~UNWALKABLE;
		}
		
		void setIlswim(bool val)
		{
			if(val) flags |= SETILSWIM;
			else flags |= CLEARILSWIM;
		}
		
		void setHopClk(int32_t val)
		{
			flags |= SETHOPCLK;
			newhopclk = val;
		}
		
		void setHopDir(int32_t val)
		{
			flags |= SETHOPDIR;
			newhopdir = val;
		}
		
		void setDir(int32_t val)
		{
			flags |= SETDIR;
			newdir = val;
		}
		
		void setChargeAttack()
		{
			flags |= CLEARCHARGEATTACK;
		}
		
		int32_t getFlags()
		{
			return flags;
		}
		
		void setFlags(int32_t val)
		{
			flags = val;;
		}
		
		bool isUnwalkable()
		{
			return (flags & UNWALKABLE) != 0;
		}
		
		WalkflagInfo operator||(WalkflagInfo other);
		WalkflagInfo operator&&(WalkflagInfo other);
		WalkflagInfo operator!();
		
	private:
		int32_t flags;
		int32_t newhopclk;
		int32_t newhopdir;
		int32_t newdir;
		int32_t newladderdir;
		int32_t newladderx;
		int32_t newladdery;
		int32_t newladderstart;
	};
	
	void execute(WalkflagInfo info);
	
	
public:
	std::map<int16_t, int32_t> usecounts;
	bool autostep,superman,inwallm,tapping,stomping,last_hurrah,onpassivedmg,inair;
	int32_t refilling,
		ladderx,
		laddery,
		stepnext,  //location of step->next just triggered (don't recursively trigger it)
		stepsecret,  //location of step->secrets just triggered (don't recursively trigger it)
		warpx,warpy, //location of warp just arrived at (don't recursively enter it)
		raftwarpx,raftwarpy, //location of dock just arrived at (don't recursively restart raft)
		pushing,  //incremental time spent pushing.
		jumping,  //incremental time spent airborne.
		charging, //incremental time spent charging weapon.
		spins,    //1 + number of sword spins left to do. 0 = not spinning.
		fairyclk, //fairy circle timeout.
		refillclk,//life refill timeout.
		drunkclk, //intoxication timeout.
		DivineProtectionShieldClk, // Divine Protection timeout.
		hoverclk, //hover boots timeout.
		hclk,     //damage blinking timeout.
		holdclk,  //"hold up item" timeout.
		holditem, //item being held.
		attackclk,//attack timeout.
		attack,   //current attack wpnid.
		attackid, //current attack itemid.
		didstuff, //played the whistle? used the blue candle?
		blowcnt,  //number of times whistle blown on this screen.
		stepoutindex, // where to step out when in a passageway
		stepoutwr, // which warp return to use for a passageway
		stepoutdmap, // which dmap the passageway exits to
		stepoutscr, // which screen the passageway exits to
		slashxofs, slashyofs; // used by positionSword() and draw()
	//spacing so no confusion between byte and int32_t
	byte skipstep,lstep, 
		hopclk, // hopping into water timeout.
		diveclk, // diving timeout.
		whirlwind, // is Hero inside an arriving whirlwind? (yes = 255)
		specialcave, // is Hero inside a special cave?
		hitdir, // direction from which damage was taken.
		ladderdir, // direction of ladder
		lastdir[4], // used in Maze Path screens
		ladderstart, // starting direction of ladder...?
		inlikelike, // 1 = Like Like. 2 = Taking damage while trapped
		damageovertimeclk, // clock for determining when Hero takes passive damage from combos beneath him.
		switchhookclk, //clock for switchhook animation timing
		switchhookmaxtime, //the switchhookclk starting value
		switchhookstyle, //the switchhook animation style
		switchhookarg; //a parameter based on the switchhook style
	int32_t shiftdir, // shift direction when walking into corners of solid combos
		sdir, // scrolling direction
		sideswimdir,  //for forcing hero to face left or right in sideview
		immortal; //Timer for being unable to die
	int32_t hammer_swim_up_offset,
		hammer_swim_down_offset,
		hammer_swim_left_offset,
		hammer_swim_right_offset,
		swimjump; //jump amount when leaving sideview water from the top
	int32_t hopdir;  // direction to hop out of water (diagonal movement only)
	int32_t holddir;
	int32_t landswim; // incremental time spent swimming against land
	bool ilswim; // is land swimming?
	bool walkable;
	int32_t justmoved;
	actiontype action, tempaction; // current action, cached action.
	int32_t hshandle_id, hshead_id;
	byte conveyor_flags;
	byte raftclk; // for slow rafting movement
	zfix climb_cover_x, climb_cover_y;
	
	// Respawn point when drowning/etc
	zfix respawn_x, respawn_y;
	uint16_t respawn_dmap, respawn_scr;
	
	zfix convey_forcex, convey_forcey;
	
	zfix falling_oldy; // Used by the Stomp Boots in sideview
	byte dontdraw;
	byte warp_sound;
	bool diagonalMovement;
	bool bigHitbox;
	int32_t steprate;
	int32_t swimuprate;
	int32_t swimsiderate;
	int32_t swimdownrate;
	zfix shove_offset;
	byte defence[wMax];
	int32_t subscr_speed;
	bool is_warping;
	byte dying_flags;
	int32_t prompt_combo, prompt_x, prompt_y;
	byte prompt_cset;
	bool shield_active;
	int8_t shield_forcedir, conv_forcedir;
	int32_t active_shield_id;
	word coyotetime;
	
	void set_respawn_point(bool setwarp = true);
	void go_respawn_point();
	bool can_pitfall(bool ignore_hover = false);
	
	void check_slash_block(weapon *w);
	void check_slash_block_layer2(int32_t bx, int32_t by, weapon *w, int32_t layer);
	void check_wand_block(weapon *w);
	void check_slash_block_layer(int32_t bx, int32_t by, int32_t layer);
	
	void SetSwim();
	void SetAttack();
	bool IsSideSwim();
	bool CanSideSwim();
	
	bool flickerorflash, preventsubscreenfalling; // Enable invincibility effects, disable dropping the subscreen.
	int32_t walkspeed; //Hero's walking speed.
	int32_t lastHitBy[NUM_HIT_TYPES_USED][2]; //[enemy, eweapon, combo, flag
	
	int32_t last_lens_id;// The item ID of the last Lens of Truth type item used
	int currentscroll; //currently active spin/quake scroll
	word last_savepoint_id; //combo id of save point
	
	int32_t misc_internal_hero_flags;// Flags to hold data temporarily for misc handling
	int32_t last_cane_of_byrna_item_id;
	bool on_sideview_ladder;
	zfix switchblock_z;
	bool switchblock_offset;
	byte hoverflags;
	int32_t extra_jump_count;
	
	weapon* lift_wpn;
	byte liftclk;
	byte tliftclk;
	zfix liftheight;
	uint32_t liftflags;
	optional<byte> last_lift_id;
	
	zfix ice_vx, ice_vy;
	int32_t ice_combo;
	int32_t script_ice_combo;
	int sliding;
	byte ice_entry_count, ice_entry_mcount;

	StatusData status;
private:
	ffcdata const* platform_ffc;
	bool lamp_paid;
public:
	
	// Methods below here.
	void clear_ice();
	bool isLifting();
	void set_liftflags(int liftid);
	void doSwitchHook(byte style);
	bool isStanding(bool forJump = false);
	void explode(int32_t type);
	int32_t getTileModifier();
	void setTileModifier(int32_t ntemod);
	void setImmortal(int32_t nimmortal);
	void kill(bool bypassFairy);
	bool try_hover();
	int32_t check_pitslide(bool ignore_hover = false);
	bool pitslide();
	void pitfall();
	void moveheroOld();
	void handle_slide(newcombo const& icecmb, zfix& dx, zfix& dy);
	void mod_steps(std::vector<zfix*>& v);
	void get_move(int movedir, zfix& dx, zfix& dy, int32_t& facedir);
	bool scr_walkflag(zfix_round dx,zfix_round dy,int dir,bool kb,int* canladder = nullptr);
	optional<zfix> get_solid_coord(zfix tx, zfix ty, byte dir, byte mdir, bool kb, zfix earlyterm, bool doladder);
	bool scr_canmove(zfix dx, zfix dy, bool kb, bool ign_sv,int* canladder = nullptr);
	bool movexy(zfix dx, zfix dy, bool kb = false, bool ign_sv = false, bool shove = false, bool earlyret = false);
	bool can_movexy(zfix dx, zfix dy, bool kb = false, bool ign_sv = false, bool shove = false);
	bool moveAtAngle(zfix degrees, zfix px, bool kb = false, bool ign_sv = false, bool shove = false, bool earlyret = false);
	bool can_moveAtAngle(zfix degrees, zfix px, bool kb = false, bool ign_sv = false, bool shove = false);
	bool moveDir(int dir, zfix px, bool kb = false, bool ign_sv = false, bool shove = false, bool earlyret = false);
	bool can_moveDir(int dir, zfix px, bool kb = false, bool ign_sv = false, bool shove = false);
	void snap_platform();
	bool premove();
	void movehero();
	bool new_engine_move(zfix dx, zfix dy);
	void moveOld(int32_t d2);
	void moveOld2(int32_t d2, int32_t forceRate = -1);
	int32_t hithero(int32_t hit, int32_t force_hdir = -1);
	int32_t  nextcombo(int32_t cx,int32_t cy,int32_t cdir);
	int32_t  nextflag(int32_t cx,int32_t cy,int32_t cdir, bool comboflag);
	bool nextcombo_wf(int32_t d);
	bool nextcombo_solid(int32_t d);
	int32_t  lookahead(int32_t d);
	int32_t  lookaheadflag(int32_t d);
	bool  lookaheadraftflag(int32_t d);
	
	bool check_ewpn_collide(weapon* w);
	bool try_lwpn_hit(weapon* w);
	bool try_ewpn_hit(weapon* w, bool force = false);
	void checkhit();
	
	void doHit(int32_t hitdir, int iframes = 48);
	bool checkdamagecombos(int32_t dx, int32_t dy);
	bool checkdamagecombos(int32_t dx1, int32_t dx2, int32_t dy1, int32_t dy2, int32_t layer = -1, bool solid = false, bool do_health_check = true);
	
	void checkscroll();
	void checkspecial();
	void checkspecial2(int32_t *ls);
	void checkspecial3();
	void checkpushblock();
	bool checksoliddamage();
	void oldcheckbosslockblock();
	void oldchecklockblock();
	void checkswordtap();
	void oldcheckchest(int32_t type);
	void checkchest(int32_t type);
	void checkgenpush();
	void checksigns();
	void checktouchblk();
	void checklocked();
	void deselectbombs(int32_t super); // switch Hero's weapon if his current weapon (bombs) was depleted.
	bool startwpn(int32_t itemid);
	bool onWater(bool drownonly);
	bool mirrorBonk();
	void doMirror(int32_t mirrorid);
	void handle_passive_buttons();
	void land_on_ground();
	bool do_jump(int32_t jumpid, bool passive);
	void drop_liftwpn();
	void do_liftglove(int32_t liftid, bool passive);
	bool can_lift(int32_t gloveid);
	void handle_lift(bool dec = true);
	void lift(weapon* w, byte timer, zfix height);
	void check_on_hit();
	bool doattack();
	bool can_attack();
	void do_rafting();
	void do_hopping();
	WalkflagInfo walkflag(zfix fx,zfix fy,int32_t cnt,byte d);
	WalkflagInfo walkflag(int32_t wx,int32_t wy,int32_t cnt,byte d);
	WalkflagInfo walkflagMBlock(int32_t wx,int32_t wy);
	bool edge_of_dmap(int32_t side);
	bool checkmaze(mapscr *scr, bool sound);
	bool maze_enabled_sizewarp(int32_t scrolldir);
	bool HasHeavyBoots();
	int32_t get_scroll_step(int32_t scrolldir);
	int32_t get_scroll_delay(int32_t scrolldir);
	void run_scrolling_script_int(bool waitdraw);
	void run_scrolling_script(int32_t scrolldir, int32_t cx, int32_t sx, int32_t sy, bool end_frames, bool waitdraw);
	void calc_darkroom_hero(int32_t x1 = 0, int32_t y1 = 0, int32_t x2 = 0, int32_t y2 = 0);
	void scrollscr(int32_t dir,int32_t destscr = -1, int32_t destdmap = -1);
	int32_t defend(weapon *w);
	virtual ALLEGRO_COLOR hitboxColor(byte opacity = 255) const;
	int getHammerState() const;
	bool handle_portal_collide(portal* p);
	void handle_portal_prox(portal* p);
private:
	void handleSpotlights();
	void walkdown(bool opening);
	void walkup(bool opening);
	void walkdown2(bool opening);
	void walkup2(bool opening);
	void exitcave();
	void stepout();
	void masked_draw(BITMAP *dest);
	void prompt_draw(BITMAP *dest);
	void handle_triforce(int32_t id);
	void getTriforce(int32_t id);
	int32_t weaponattackpower(int32_t itid = -1);
	void positionNet(weapon* w,int32_t itemid);
	void positionSword(weapon* w,int32_t itemid);
	bool checkstab();
	void fairycircle(int32_t type);
	void StartRefill(int32_t refillWhat);
	void Start250Refill(int32_t refillWhat);
	
#define CMPDIR_FRONT 0x1
#define CMPDIR_BACK  0x2
#define CMPDIR_LEFT  0x4
#define CMPDIR_RIGHT 0x8
	int32_t compareDir(int32_t other);
	
	int32_t  EwpnHit();
	int32_t  LwpnHit();
	void heroDeathAnimation();
	void win_game();
   
	void check_conveyor();
	bool sideviewhammerpound();
	bool agonyflag(int32_t flag);
	void addsparkle(int32_t wpn);
	void addsparkle2(int32_t type1, int32_t type2);
	void PhantomsCleanup();
	
public:
	int32_t ringpower(int32_t dmg, bool noPeril = false, bool noRing = false);
	void ganon_intro();
	void checkitems(int32_t index = -1);
	int32_t DrunkClock();
	void setDrunkClock(int32_t newdrunkclk);
	int32_t getStunClock();
	void setStunClock(int32_t v);
	int32_t getBunnyClock();
	void setBunnyClock(int32_t v);
	HeroClass();
	void init();
	virtual void drawshadow(BITMAP* dest, bool translucent);
	virtual void draw(BITMAP* dest);
	virtual bool animate(int32_t index);
	bool push_pixel(zfix dx, zfix dy);
	int32_t push_move(zfix dx, zfix dy);
	virtual bool setSolid(bool set);
	virtual void solid_push(solid_object* pusher);
	bool dowarp(int32_t type, int32_t index, int32_t warpsfx=0);
	
	void herostep();
	void stepforward(int32_t steps, bool adjust);
	void draw_under(BITMAP* dest);
	void check_slash_block(int32_t bx, int32_t by);
	void check_wpn_triggers(int32_t bx, int32_t by, weapon *w);
	void check_slash_block2(int32_t bx, int32_t by, weapon *w);
	void check_wand_block2(int32_t bx, int32_t by, weapon *w);
	
	void check_wand_block(int32_t bx, int32_t by);
	void check_pound_block(int bx, int by, weapon* w = nullptr);
	void check_pound_block_layer(int bx, int by, int lyr, weapon* w = nullptr);
	
	// called by ALLOFF()
	void resetflags(bool all);
	void Freeze();
	void unfreeze();
	void finishedmsg();
	void Drown(int32_t state = 0);
	int32_t getEaten();
	void setEaten(int32_t i);
	zfix  getX();
	zfix  getY();
	zfix  getZ();
	zfix  getFakeZ();
	zfix  getFall();
	zfix  getFakeFall();
	zfix  getXOfs();
	zfix  getYOfs();
	void setXOfs(int32_t newxofs);
	void setYOfs(int32_t newyofs);
	int32_t  getHXOfs();
	int32_t  getHYOfs();
	int32_t  getHXSz();
	int32_t  getHYSz();
	zfix  getClimbCoverX();
	zfix  getClimbCoverY();
	int32_t  getLadderX();
	int32_t  getLadderY();
	void setX(int32_t new_x);
	void setY(int32_t new_y);
	void setZ(int32_t new_Z);
	void setFakeZ(int32_t new_Z);
	
	void setXfix(zfix new_x);
	void setYfix(zfix new_y);
	void setZfix(zfix new_Z);
	void setFakeZfix(zfix new_Z);
	void setFall(zfix new_fall);
	void setFakeFall(zfix new_fall);
	void setClimbCoverX(int32_t new_x);
	void setClimbCoverY(int32_t new_y);
	int32_t  getLStep();
	int32_t  getCharging();
	void setCharging(int32_t new_charging);
	bool isCharged();
	int32_t  getAttackClk();
	void  setAttackClk(int32_t new_clk);
	int32_t  getSwordClk();
	int32_t  getItemClk();
	int32_t  getShieldClk();
	void  setSwordClk(int32_t newclk);
	void  setItemClk(int32_t newclk);
	void  setShieldClk(int32_t newclk);
	zfix  getModifiedX();
	zfix  getModifiedY();
	int32_t  getDir();
	void setDir(int32_t new_dir);
	int32_t  getHitDir();
	void setHitDir(int32_t new_dir);
	int32_t  getClk();
	int32_t  getPushing();
	void reset_swordcharge();
	void reset_hookshot();
	bool can_deploy_ladder();
	void reset_ladder();
	bool refill();
	void Catch();
	bool getClock();
	void setClock(bool state);
	int32_t getFlashingCSet();
	bool is_hitflickerframe();
	int32_t  getAction();
	int32_t  getAction2(); //used by the new ffcore.actions
	void setAction(actiontype new_action);
	int32_t getHeldItem();
	void setHeldItem(int32_t newitem);
	bool isDiving();
	bool isSwimming();
	void setDontDraw(byte new_dontdraw);
	byte getDontDraw();
	void setHClk(int32_t newhclk);
	int32_t getHClk();
	void setDivineProtectionShieldClk(int32_t newclk);
	int32_t getDivineProtectionShieldClk();
	int32_t getHoverClk();
	int32_t getHoldClk();
	int32_t getSpecialCave(); // used only by maps.cpp
	bool ffwarp;
	bool ffpit;
	void setscriptnohit(bool);
	bool getscriptnohit();
	
	void sethitHeroUID(int32_t type, int32_t screen_index);
	void ClearhitHeroUIDs();
	void set_defence(int32_t def, int32_t v);
	int32_t get_defence(int32_t def);
	int32_t gethitHeroUID(int32_t type);
	
	void setHurtSFX(int32_t sfx); //Set Hero;s hurt sfx
	int32_t getHurtSFX();
	
	  //Prevent the subscreen from falling by script.
	bool stopSubscreenFalling();
	void stopSubscreenFalling(bool v);
	
	bool getDiagMove(); //Diagonal movement.
	void setDiagMove(bool newdiag);
	bool getBigHitbox(); //Large H-itbox
	void setBigHitbox(bool newbighitbox);
	int32_t getStepRate();
	void setStepRate(int32_t newrate);
	int32_t getSwimUpRate();
	void setSwimUpRate(int32_t newrate);
	int32_t getSwimSideRate();
	void setSwimSideRate(int32_t newrate);
	int32_t getSwimDownRate();
	void setSwimDownRate(int32_t newrate);

	
	
	int32_t getLastLensID();	
	void setLastLensID(int32_t p_item);
	void cleanupByrna();
	bool getOnSideviewLadder();
	void setOnSideviewLadder(bool val);
	bool canSideviewLadder(bool down = false);
	void trySideviewLadder();
	bool canSideviewLadderRemote(int32_t wx, int32_t wy, bool down = false);
	virtual bool sideview_mode() const;
	virtual bool is_unpushable() const;
	bool on_ffc_platform(ffcdata const& ffc, bool old);
	bool on_ffc_platform();
	void check_platform_ffc();
	void clear_platform_ffc();
	
	void update_status(EntityStatus const& stat, word clk, int32_t dur, word indx);
};

bool usingActiveShield(int32_t itmid = -1);
int32_t getCurrentShield(bool requireActive = true);
bool isRaftFlag(int32_t flag);
void do_lens();
void do_210_lens();
int32_t touchcombo(int32_t x,int32_t y);
extern bool did_secret;
int32_t selectWlevel(int32_t d);
void computeMaxArrows();
bool usekey();
bool canUseKey(int32_t num = 1);
bool usekey(int32_t num);

int32_t enemy_dp(int32_t index);
int32_t ewpn_dp(int32_t index);
int32_t lwpn_dp(int32_t index);
bool checkbunny(int32_t itemid);
bool usesSwordJinx(int32_t itemid);
bool checkitem_jinx(int32_t itemid);
void stopCaneOfByrna();
bool isWpnPressed(int32_t wpn);
int32_t getWpnPressed(int32_t wpn);
int32_t getRocsPressed();
bool isItmPressed(int32_t itmid);
int32_t selectSword();
int32_t selectItemclass(int32_t itemclass);
void selectNextAWpn(int32_t type);
void selectNextBWpn(int32_t type);
void selectNextXWpn(int32_t type);
void selectNextYWpn(int32_t type);
void verifyBothWeapons();
void verifyAWpn();
void verifyBWpn();
void verifyXWpn();
void verifyYWpn();
bool canget(int32_t id);
void dospecialmoney(int32_t index);
void getitem(int32_t id, bool nosound=false, bool doRunPassive=false);
void takeitem(int32_t id);
void red_shift();
void slide_in_color(int32_t color);
#endif
