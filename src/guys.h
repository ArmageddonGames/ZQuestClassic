//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  guys.cc
//
//  "Guys" code (and other related stuff) for zelda.cc
//
//  Still has some hardcoded stuff that should be moved
//  out into defdata.cc for customizing the enemies.
//
//--------------------------------------------------------

//#include "guys.h"
#ifndef _GUYS_H_
#define _GUYS_H_
#include <list>
#include "sprite.h"
#include "enemyWeapon.h"
#include "weapons.h"
#include "zfix.h"

class item;

extern int32_t repaircharge;
extern bool adjustmagic;
extern bool learnslash;
extern int32_t itemindex;
extern int32_t wallm_load_clk;
extern int32_t sle_x,sle_y,sle_cnt,sle_clk;
extern int32_t vhead;
extern int32_t guycarryingitem;

int32_t random_layer_enemy();
int32_t count_layer_enemies();
bool can_do_clock();
int32_t link_on_wall();
bool tooclose(int32_t x,int32_t y,int32_t d);
bool isflier(int32_t id);
bool never_in_air(int32_t id);

// Start spinning tiles - called by load_default_enemies
void awaken_spinning_tile(mapscr *s, int32_t pos);

// Used to remove/add fireball shooter enemies
void screen_combo_modify_preroutine(mapscr *s, int32_t pos);
void screen_combo_modify_postroutine(mapscr *s, int32_t pos);

// Find the IDs of enemies spawned by combos and flags. Called once on loading a quest.
void identifyCFEnemies();

byte get_def_ignrflag(int32_t edef);
int32_t conv_edef_unblockable(int32_t edef, byte unblockable);

/**********************************/
/*******  Enemy Base Class  *******/
/**********************************/

class enemy : public sprite
{
public:
	guydata *d;
	// Approximately all of these variables are accessed by either ffscript.cpp or inherited classes
	int32_t o_tile, frate, hp, hclk, clk3, stunclk, timer, fading, superman, mainguy, did_armos;
	byte movestatus, item_set, grumble, posframe;
	bool itemguy, count_enemy, dying, ceiling, leader, scored, script_spawned;
	zfix  step, floor_y;
	
	//d variables
	dword flags;
	dword flags2;
	
	int16_t  family, dcset, anim;
	int16_t  dp, wdp, wpn;
	EnemyWeapon eWpn;
	
	int16_t  rate, hrate, homing;
	zfix dstep;
	int32_t dmisc1, dmisc2, dmisc3, dmisc4, dmisc5, dmisc6, dmisc7, dmisc8, dmisc9, dmisc10, dmisc11, dmisc12, dmisc13, dmisc14, dmisc15;
	int16_t bgsfx, bosspal;
	byte defense[edefLAST255];
	byte hitsfx,deadsfx;
	byte submerged;

	int32_t  clk2,sclk;
	int32_t  starting_hp;
	int32_t  ox, oy;
	word  s_tile; //secondary (additional) tile(s)
	
	int32_t hitby[NUM_HIT_TYPES_USED];
	int16_t firesfx;
	bool isCore;
	int16_t parentCore; 
	int32_t script_UID; 	//used to determine the UID of an enemy by the user (in scripts), plus
			//in-engine assignment of children to a parent and
			//for future use in npc scripts (as the ref for `this`)--the easy way to determine to
			//what npc a script on the stack is bound.
	
	int32_t wpnsprite; //wpnsprite is new for 2.6 -Z
	int32_t SIZEflags; //Flags for size panel offsets. The user must enable these to override defaults. 
	int32_t frozentile, frozencset, frozenclock;
	
	bool haslink;
	
	int16_t frozenmisc[10];
	
	int32_t dmisc16, dmisc17, dmisc18, dmisc19, dmisc20, dmisc21, dmisc22, dmisc23, 
	dmisc24, dmisc25, dmisc26, dmisc27, dmisc28, dmisc29, dmisc30, dmisc31, dmisc32;
	int32_t movement[32]; //Reserved for npc movement types and args. 
	int32_t new_weapon[32]; //Reserved for weapon patterns and args.
	word npcscript; 
	int32_t initD[8], initA[2];
	word weaponscript;
	int32_t weap_initiald[8];
	int32_t weap_initiala[2];
	byte stickclk;
	int32_t parent_script_UID;
   
	int32_t dialogue_str; //WIll be set in spawn flags. 
	int32_t editorflags; //Enemy editor flags 1 to 16
	
	bool immortal;
	bool noSlide;
	int16_t hitdir;
	
	int32_t getScriptUID();
	void setScriptUID(int32_t new_id);
	//void explode(int32_t type);
	
	zfix  getX();
	zfix  getY();
	int32_t  getID();

    enemy(enemy const & other);
	
	enemy(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	enemy(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : sprite()
	virtual ~enemy();
	
	// Handle pitfalls
	bool do_falling(int32_t index);
	// Handle drowning
	bool do_drowning(int32_t index);
	// Supplemental animation code that all derived classes should call
	// as a return value for animate().
	// Handles the death animation and returns true when enemy is finished.
	virtual bool Dead(int32_t index);
	// Basic animation code that all derived classes should call.
	// The one with an index is the one that is called by
	// the guys sprite list; index is the enemy's index in the list.
	virtual bool animate(int32_t index);
	
	// auomatically kill off enemy (for rooms with ringleaders)
	virtual void kickbucket();
	virtual bool isSubmerged();
	// Stop BG SFX only if no other enemy is playing it
	void stop_bgsfx(int32_t index);
	bool m_walkflag_simple(int32_t dx,int32_t dy);
	bool m_walkflag(int32_t dx,int32_t dy,int32_t special, int32_t dir, int32_t x=-1000,int32_t y=-1000, bool kb = false);
	bool m_walkflag_old(int32_t dx,int32_t dy,int32_t special, int32_t x=-1000, int32_t y=-1000);
	// Take damage or ignore it
	virtual int32_t takehit(weapon *w);
	// override hit detection to check for invicibility, stunned, etc
	virtual bool hit(sprite *s);
	virtual bool hit(int32_t tx,int32_t ty,int32_t tz,int32_t txsz,int32_t tysz,int32_t tzsz);
	virtual bool hit(weapon *w);
	virtual void break_shield() {}; // Overridden by types that can have shields
	
	bool can_pitfall(bool checkspawning = true);
	void try_death(bool force_kill = false);
	 // returns true if next step is ok, false if there is something there
	bool canmove(int32_t ndir,zfix s,int32_t special,int32_t dx1,int32_t dy1,int32_t dx2,int32_t dy2, bool kb);
	bool canmove_old(int32_t ndir,zfix s,int32_t special,int32_t dx1,int32_t dy1,int32_t dx2,int32_t dy2);
	bool canmove(int32_t ndir,zfix s,int32_t special, bool kb);
	bool canmove(int32_t ndir,int32_t special, bool kb);
	bool canmove(int32_t ndir, bool kb);
	bool enemycanfall(int32_t id);
	// 8-directional
	void newdir_8_old(int32_t rate,int32_t homing, int32_t special,int32_t dx1,int32_t dy1,int32_t dx2,int32_t dy2);
	void newdir_8(int32_t rate,int32_t homing, int32_t special,int32_t dx1,int32_t dy1,int32_t dx2,int32_t dy2);
	void newdir_8_old(int32_t rate,int32_t homing, int32_t special);
	void newdir_8(int32_t rate,int32_t homing, int32_t special);
	// makes the enemy slide backwards when hit
	// sclk: first byte is clk, second byte is dir
	int32_t slide();
	bool can_slide();
	bool fslide();
	bool overpit(enemy *e);
	bool shadow_overpit(enemy *e);
	virtual bool knockback(int32_t time, int32_t dir, int32_t speed);
	virtual bool runKnockback();
	// changes enemy's direction, checking restrictions
	// rate:   0 = no random changes, 16 = always random change
	// homing: 0 = none, 256 = always
	// grumble 0 = none, 4 = strongest appetite
	void newdir(int32_t rate,int32_t homing,int32_t special);
	void newdir();
	zfix distance_left();
	// keeps walking around
	void constant_walk(int32_t rate,int32_t homing,int32_t special);
	void constant_walk();
	// falls through the Z axis;
	void falldown();
	int32_t pos(int32_t x,int32_t y);
	// for variable step rates
	void variable_walk(int32_t rate,int32_t homing,int32_t special);
	// pauses for a while after it makes a complete move (to a new square)
	void halting_walk(int32_t rate,int32_t homing,int32_t special,int32_t hrate, int32_t haltcnt);
	// 8-directional movement, aligns to 8 pixels
	void constant_walk_8_old(int32_t rate,int32_t homing,int32_t special);
	void constant_walk_8(int32_t rate,int32_t homing,int32_t special);
	// 8-directional movement, halting
	void halting_walk_8(int32_t newrate,int32_t newhoming, int32_t newclk,int32_t special,int32_t newhrate, int32_t haltcnt);
	// 8-directional movement, no alignment
	void variable_walk_8(int32_t rate,int32_t homing,int32_t newclk,int32_t special);
	// same as above but with variable enemy size
	void variable_walk_8(int32_t rate,int32_t homing,int32_t newclk,int32_t special,int32_t dx1,int32_t dy1,int32_t dx2,int32_t dy2);
	// the variable speed floater movement
	// ms is max speed
	// ss is step speed
	// s is step count
	// p is pause count
	// g is graduality :)
	void floater_walk(int32_t rate,int32_t newclk,zfix ms,zfix ss,int32_t s,int32_t p, int32_t g);
	void floater_walk(int32_t rate,int32_t newclk,zfix s);
	// Checks if enemy is lined up with Link. If so, returns direction Link is
	// at as compared to enemy. Returns -1 if not lined up. Range is inclusive.
	int32_t lined_up(int32_t range, bool dir8) const;
	// returns true if Link is within 'range' pixels of the enemy
	bool LinkInRange(int32_t range);
	// Breathe fire
	void FireBreath(bool seeklink);
	// Shoot weapons
	void FireWeapon();
	void update_enemy_frame();
	void n_frame_n_dir(int32_t frames, int32_t dir, int32_t f4);
	void tiledir_small(int32_t ndir, bool fourdir);
	void tiledir_three(int32_t ndir);
	void tiledir(int32_t ndir, bool fourdir);
	void tiledir_big(int32_t ndir, bool fourdir);
	// Enemies that cannot ever be penetrated by weapons
	bool cannotpenetrate();
	bool isOnSideviewPlatform(); //This handles large enemies, too!
	
	// Returns true iff a combo type or flag precludes enemy movement.
	bool groundblocked(int32_t dx, int32_t dy, bool isKB = false);
	// Returns true iff enemy is floating and blocked by a combo type or flag.
	bool flyerblocked(int32_t dx, int32_t dy, int32_t special, bool isKB = false);
	virtual bool ignore_msg_freeze()
	{
		return false;
	}
	
	virtual int32_t run_script(int32_t mode);

protected:
	
	
	// to allow for different sfx on defeating enemy
	virtual void death_sfx();
	virtual void move(zfix dx,zfix dy);
	virtual void removearmos(int32_t ax,int32_t ay);
	virtual void move(zfix s);
	void leave_item();
	
	// take damage or ignore it
	virtual bool hitshield(int32_t wpnx, int32_t wpny, int32_t xdir);
	virtual int32_t defend(int32_t wpnId, int32_t *power, int32_t edef);
//New 2.55 Weapon System
	int32_t weaponToDefence(int32_t wid);
	int32_t getWeaponID(weapon *w);
	int32_t resolveEnemyDefence(weapon *w);
	virtual int32_t defendNew(int32_t wpnId, int32_t *power, int32_t edef, byte unblockable);
	//virtual int32_t defend_wdmg(int32_t wpnId, int32_t dmg, int32_t edef);
	bool candamage(int32_t power, int32_t edef, byte unblockable);
	int32_t defenditemclass(int32_t wpnId, int32_t *power);
	int32_t defenditemclassNew(int32_t wpnId, int32_t *power, weapon *w);
	
	bool dont_draw();
	// base drawing function to be used by all derived classes instead of
	// sprite::draw()
	virtual void draw(BITMAP *dest);
	virtual void drawzcboss(BITMAP *dest);
	virtual void old_draw(BITMAP *dest);
	// similar to the overblock function--can do up to a 32x32 sprite
	void drawblock(BITMAP *dest,int32_t mask);
	virtual void drawshadow(BITMAP *dest, bool translucent);
	void masked_draw(BITMAP *dest,int32_t mx,int32_t my,int32_t mw,int32_t mh);
	
	//                         --==**==--
	//   Movement routines that can be used by derived classes as needed
	//                         --==**==--
	void fix_coords(bool bound=false);
   
	
	
private:
	bool shieldCanBlock;
};

/********************************/
/*********  Guy Class  **********/
/********************************/

// good guys, fires, fairy, and other non-enemies
// based on enemy class b/c guys in dungeons act sort of like enemies
// also easier to manage all the guys this way
class guy : public enemy
{
public:
	guy(zfix X,zfix Y,int32_t Id,int32_t Clk,bool mg);                // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual bool ignore_msg_freeze()
	{
		return true;
	}
};

/*******************************/
/*********   Enemies   *********/
/*******************************/

class eFire : public enemy
{
public:
	int32_t clk4;
	bool shield;
	eFire(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	virtual void break_shield();
	eFire(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
};

class eOther : public enemy
{
public:
	int32_t clk4;
	bool shield;
	eOther(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eOther(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	virtual void break_shield();
};

class eScript : public enemy
{
public:
	int32_t clk4;
	bool shield;
	eScript(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eScript(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	virtual void break_shield();
};

class eFriendly : public enemy
{
public:
	int32_t clk4;
	bool shield;
	eFriendly(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eFriendly(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	virtual void break_shield();
};

void removearmos(int32_t ax,int32_t ay);

class eGhini : public enemy
{
public:
	int32_t clk4, ox, oy, c;
	eGhini(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eGhini(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual void kickbucket();
};

class eTektite : public enemy
{
	zfix old_y; // a kludge used to check for ZScript modification of Y
public:
	int32_t c;
	int32_t clk2start;
	int32_t cstart;
	eTektite(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);

	eTektite(zfix X,zfix Y,int32_t Id,int32_t Clk);                   // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void drawshadow(BITMAP *dest,bool translucent);
	virtual void draw(BITMAP *dest);
};

class eItemFairy : public enemy
{
public:
	eItemFairy(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eItemFairy(zfix X,zfix Y,int32_t Id,int32_t Clk);                 // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class ePeahat : public enemy
{
public:
	int32_t ox, oy, c;
	ePeahat(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	ePeahat(zfix X,zfix Y,int32_t Id,int32_t Clk);                    // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void drawshadow(BITMAP *dest, bool translucent);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	virtual void kickbucket();
};

class eLeever : public enemy
{
public:
	bool temprule;
	eLeever(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eLeever(zfix X,zfix Y,int32_t Id,int32_t Clk);                    // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	bool canplace(int32_t d);
	virtual void draw(BITMAP *dest);
	virtual bool isSubmerged();
};

class eWallM : public enemy
{
public:
	eWallM(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eWallM(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	void wallm_crawl();
	void grablink();
	virtual void draw(BITMAP *dest);
	virtual bool isSubmerged();
};

class eTrap : public enemy
{
public:
	int32_t  ox, oy;
	eTrap(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eTrap(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	bool trapmove(int32_t ndir);
	bool clip();
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon* w);
};

class eTrap2 : public enemy                                 //trap that goes back and forth constantly
{
public:
	eTrap2(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eTrap2(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	bool trapmove(int32_t ndir);
	bool clip();
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

// Helper for launching fireballs from statues, etc.
// It's invisible and can't be hit.
// Pass the range value through the clk variable in the constuctor.
class eProjectile : public enemy
{
public:
	eProjectile(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eProjectile(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	
	const int32_t minRange;
};

class eTrigger : public enemy
{
public:
	eTrigger(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eTrigger(zfix X,zfix Y,int32_t Id,int32_t Clk);                   // : enemy(X,Y,Id,Clk)
	virtual void draw(BITMAP *dest);
	virtual void death_sfx();
};

class eNPC : public enemy
{
public:
	eNPC(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eNPC(zfix X,zfix Y,int32_t Id,int32_t Clk);                       // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

class eZora : public enemy
{
public:
	eZora(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eZora(zfix X,zfix Y,int32_t Id,int32_t Clk);                      // : enemy(X,Y,Id,0)
	virtual void facelink();
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual bool isSubmerged();
};

class eStalfos : public enemy
{
public:
	int32_t clk4, //Tribble clock
		clk5; // Like Like clock
	bool fired; // Darknut5, Bombchu
	bool shield; // Darknut
	bool dashing; // Rope
	int32_t multishot; // Shoot twice/thrice/n times in one halt
	zfix fy, shadowdistance; // Pols Voice
	eStalfos(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eStalfos(zfix X,zfix Y,int32_t Id,int32_t Clk);                   // : enemy(X,Y,Id,Clk)
	bool WeaponOut();
	void KillWeapon();
	void charge_attack();
	void eatlink();
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
	void vire_hop();
	virtual void drawshadow(BITMAP *dest, bool translucent);
	virtual void break_shield();
};

class eKeese : public enemy
{
public:
	int32_t ox, oy, c;
	int32_t clk4;
	eKeese(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eKeese(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void drawshadow(BITMAP *dest, bool translucent);
	virtual void draw(BITMAP *dest);
};

/*********************************/
/**********   Bosses   ***********/
/*********************************/

class eDodongo : public enemy
{
public:
	eDodongo(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eDodongo(zfix X,zfix Y,int32_t Id,int32_t Clk);                   // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

class eDodongo2 : public enemy
{
public:
	int32_t previous_dir;
	eDodongo2(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eDodongo2(zfix X,zfix Y,int32_t Id,int32_t Clk);                  // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

class eGohma : public enemy
{
public:
	int32_t clk4; // Movement clk must be separate from animation clk because of the Clock item
	eGohma(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eGohma(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy((zfix)128,(zfix)48,Id,0)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

class eLilDig : public enemy
{
public:
	eLilDig(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eLilDig(zfix X,zfix Y,int32_t Id,int32_t Clk);                    // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class eBigDig : public enemy
{
public:
	eBigDig(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eBigDig(zfix X,zfix Y,int32_t Id,int32_t Clk);                    // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual int32_t takehit(weapon *w);
};

class eGanon : public enemy
{
public:
	int32_t Stunclk;
	eGanon(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eGanon(zfix X,zfix Y,int32_t Id,int32_t Clk);                     // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual int32_t takehit(weapon *w);
	virtual void draw(BITMAP *dest);
	void draw_guts(BITMAP *dest);
	void draw_flash(BITMAP *dest);
};

void getBigTri(int32_t id2);

/**********************************/
/***  Multiple-Segment Enemies  ***/
/**********************************/

// segment manager
class eMoldorm : public enemy
{
public:
	int32_t segcnt;
	int32_t segid;
	// tell it the segment count through the clk param
	eMoldorm(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eMoldorm(zfix X,zfix Y,int32_t Id,int32_t Clk);                   // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
};

// segment class
class esMoldorm : public enemy
{
public:
	esMoldorm(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	esMoldorm(zfix X,zfix Y,int32_t Id,int32_t Clk);                  // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual int32_t takehit(weapon *w);
	virtual void draw(BITMAP *dest);
	int32_t parentclk; //because of custom step speed, clk is not sufficient to keep track
	//of when to check the parent Moldorm's direction, since the frequency
	//of the parent's checking might not divide the Moldorm segment's
	//number of animation frames. -DD
};

class eManhandla : public enemy
{
public:
	char arm[8];                                            // side the arm is on
	int32_t armcnt;
	bool adjusted;
	//eManhandla(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eManhandla(zfix X,zfix Y,int32_t Id,int32_t Clk);                 // : enemy(X,Y,Id,0)
	virtual bool animate(int32_t index);
	virtual int32_t takehit(weapon *w);
	virtual void draw(BITMAP *dest);
};

class esManhandla : public enemy
{
public:
	// use clk param to tell which arm it is
	//esManhandla(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	esManhandla(zfix X,zfix Y,int32_t Id,int32_t Clk);                // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class eGleeok : public enemy
{
public:
	int32_t flameclk, flamehead;
	int32_t necktile;
	//eGleeok(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	eGleeok(zfix X,zfix Y,int32_t Id,int32_t Clk);                    // : enemy((zfix)120,(zfix)48,Id,Clk)
	virtual bool animate(int32_t index);
	virtual int32_t takehit(weapon *w);
	virtual void draw(BITMAP *dest);
	virtual void draw2(BITMAP *dest);
};

// head class
class esGleeok : public enemy
{
public:
	int32_t nx[255],ny[255];                                        //neck x and y?
	int32_t headtile;
	int32_t flyingheadtile;
	int32_t necktile;
	sprite * parent; //parent Gleeok to whose neck this segment is attached
	zfix xoffset, yoffset; //coords relative to the parent Gleeok
	int32_t nxoffset[255], nyoffset[255]; //random "jiggling" of the next tiles
	//esGleeok(enemy const & other, bool new_script_uid, bool clear_parent_script_UID);
	esGleeok(zfix X,zfix Y,int32_t Id,int32_t Clk, sprite * prnt);                   // : enemy(X,Y,Id,Clk)
	virtual bool animate(int32_t index);
	virtual int32_t takehit(weapon *w);
	virtual void draw(BITMAP *dest);
	virtual void draw2(BITMAP *dest);
};

/**********************************/
/**********  Misc Code  ***********/
/**********************************/
void addEwpn(int32_t x,int32_t y,int32_t z,int32_t id,int32_t type,int32_t power,int32_t dir, int32_t parentid, byte script_gen = 0);
// Used by Link's swords & stomp boots
int32_t hit_enemy(int32_t index,int32_t wpnId,int32_t power,int32_t wpnx,int32_t wpny,int32_t dir, int32_t enemyHitWeapon);
void enemy_scored(int32_t index);
void addguy(int32_t x,int32_t y,int32_t id,int32_t clk,bool mainguy);
void additem(int32_t x,int32_t y,int32_t id,int32_t pickup);
void additem(int32_t x,int32_t y,int32_t id,int32_t pickup,int32_t clk);
void kill_em_all();
// For Link's hit detection. Don't count them if they are stunned or are guys.
int32_t GuyHit(int32_t tx,int32_t ty,int32_t tz,int32_t txsz,int32_t tysz,int32_t tzsz);
// For Link's hit detection. Count them if they are dying.
int32_t GuyHit(int32_t index,int32_t tx,int32_t ty,int32_t tz,int32_t txsz,int32_t tysz,int32_t tzsz);
bool hasMainGuy();
void EatLink(int32_t index);
void GrabLink(int32_t index);
bool CarryLink();
void movefairy(zfix &x,zfix &y,int32_t misc);
void movefairy2(zfix x,zfix y,int32_t misc);
void movefairynew(zfix &x,zfix &y,item const &itemfairy);
void movefairynew2(zfix x,zfix y,item const &itemfairy);
void killfairy(int32_t misc);
void killfairynew(item const &itemfairy);
int32_t addenemy(int32_t x,int32_t y,int32_t id,int32_t clk);
int32_t addenemy(int32_t x,int32_t y,int32_t z,int32_t id,int32_t clk);
int32_t addchild(int32_t x,int32_t y,int32_t id,int32_t clk, int32_t parent_scriptUID);
int32_t addchild(int32_t x,int32_t y,int32_t z,int32_t id,int32_t clk, int32_t parent_scriptUID);
bool isjumper(int32_t id);
bool canfall(int32_t id);
void addfires();
void loadguys();
void loaditem();
void never_return(int32_t index);
bool hasBoss();
bool slowguy(int32_t id);
bool countguy(int32_t id);
bool ok2add(int32_t id);
void load_default_enemies();
void nsp(bool random);
int32_t next_side_pos(bool random);
bool can_side_load(int32_t id);
void side_load_enemies();
void loadenemies();
void moneysign();
void putprices(bool sign);
void setupscreen();
FONT *setmsgfont();
bool parsemsgcode();
void putmsg();
int32_t message_more_y();
int32_t wpnsfx(int32_t wpn); 

/***  Collision detection & handling  ***/

void check_collisions();
void clear_script_one_frame_conditions();
void dragging_item();
void roaming_item();
int32_t more_carried_items();
#endif
/*** end of guys.cc ***/

