#ifndef SPRITE_H_
#define SPRITE_H_

#include "base/zc_alleg.h"
#include "base/headers.h"
#include "base/zdefs.h"
#include <set>
#include <map>
#include "solidobject.h"
#include "base/cpos_info.h"

struct itemdata;
struct newcombo;
using std::map;
// this code needs some patching for use in zquest.cc

extern itemdata *itemsbuf;
extern wpndata  *wpnsbuf;
extern bool     freeze_guys;
extern int32_t fadeclk;
extern int32_t frame;
extern bool BSZ;
extern int32_t conveyclk;
extern byte newconveyorclk;

extern byte sprite_flicker_color;
extern byte sprite_flicker_transp_passes;
extern byte sprite_mask_color;
void sprite_push_flicker();
void sprite_pop_flicker();

//Sprite Offscreen Bits
#define SPRITE_MOVESOFFSCREEN 0x01

// Forward reference
class refInfo;

class sprite : public solid_object
{
public:
	static sprite* getByUID(int32_t uid);

    //unique sprite ID
    int32_t uid;
    int32_t getUID()
    {
		if (!uid)
			registerUID();
        return uid;
    }
	void registerUID();
	void reassignUid(int32_t new_uid);
    
   
    
    zfix z,fall,fakefall,fakez;
    int32_t tile,shadowtile,cs,flip,c_clk,clk,misc;
    int16_t flickercolor;
	int16_t flickertransp;
	zfix xofs,yofs,zofs;
    zfix shadowxofs,shadowyofs;
    // no hzofs - it's always equal to zofs.
    int32_t hzsz; // hit z-height
	// tile width
    int32_t txsz = -1;
	// tile height
	int32_t tysz = -1;
    /*
     * Explanation of hzsz:
     * Hzsz is how "tall" a sprite is.
     * To jump over a sprite, you must be higher than its z+hzsz.
     * Enemies have hzsz of 0 so that Hero can jump over them (see LA.)
     */
    int32_t id;
    zfix slopeid;
    byte onplatid = 0;
    bool angular,canfreeze;
    double angle; // TODO: bad for replays
    int32_t lasthit, lasthitclk;
    int32_t dummy_int[10];
    zfix dummy_fix[10];
    float dummy_float[10];
    bool dummy_bool[10];
    int32_t drawstyle;                                          //0=normal, 1=translucent, 2=cloaked
    int32_t extend;
    // Scripting additions
    int32_t miscellaneous[32];
    bool scriptcoldet;
    int32_t wpnsprite; //wpnsprite is new for 2.6 -Z
    dword scriptflag;
    word script;
    word weaponscript;
    int32_t initD[8];
    int32_t weap_initd[8];
    int32_t scripttile;
    signed char scriptflip;
    bool do_animation;
    int32_t rotation;
    int32_t scale; 
    move_flags moveflags;
    byte drawflags;
	byte knockbackflags;
	byte screenedge;
	int32_t scriptshadowtile;
#define FLAG_NOSLIDE 0x01
#define FLAG_NOSCRIPTKNOCKBACK 0x02
	byte knockbackSpeed;
	int32_t script_knockback_clk;
	int32_t script_knockback_speed;
	int32_t pit_pulldir; // Pitfall pull direction
	int32_t pit_pullclk; // Pitfall pull clk
	int32_t fallclk; // Pitfall fall clk
	int32_t fallCombo; // Pitfall fallen combo
	int32_t o_tile;
	int32_t o_cset; // Storage var for an old cset; used by pitfalls
	int32_t drownclk; // Pitfall fall clk
	int32_t drownCombo; // Pitfall fallen combo
	bool isspawning;
	bool can_flicker;
	bool hide_hitbox;
	
	byte spr_shadow, spr_death, spr_spawn;
	int16_t spr_death_anim_clk, spr_spawn_anim_clk;
	byte spr_death_anim_frm, spr_spawn_anim_frm;
	
	byte glowRad, glowShape;
	
	int32_t ignore_delete;
    
    sprite();
    sprite(sprite const & other);
    sprite(zfix X,zfix Y,int32_t T,int32_t CS,int32_t F,int32_t Clk,int32_t Yofs);
    virtual ~sprite();
	virtual void handle_sprlighting();
    virtual void draw(BITMAP* dest);                        // main layer
    virtual void drawzcboss(BITMAP* dest);                        // main layer
    virtual void draw8(BITMAP* dest);                       // main layer
    virtual void drawcloaked(BITMAP* dest);                 // main layer
    virtual void drawshadow(BITMAP* dest, bool translucent);// main layer
    virtual void draw2(BITMAP* dest);                       // top layer for special needs
    virtual void drawcloaked2(BITMAP* dest);                // top layer for special needs
    virtual bool animate(int32_t index);
	virtual void post_animate();
	//virtual void solid_push(solid_object* pusher);
    virtual void check_conveyor();
	virtual int32_t get_pit(); //Returns combo ID of pit that sprite WOULD fall into; no side-effects
	virtual int32_t check_pits(); //Returns combo ID of pit fallen into; 0 for not fallen.
	virtual int32_t get_water(); //Returns combo ID of pit that sprite WOULD fall into; no side-effects
	virtual int32_t check_water(); //Returns combo ID of pit fallen into; 0 for not fallen.
    int32_t real_x(zfix fx);
    int32_t real_y(zfix fy);
    int32_t real_ground_y(zfix fy);
    int32_t real_z(zfix fz);
    int32_t fake_z(zfix fz);
    virtual bool hit();
    virtual bool hit(sprite *s);
    virtual bool hit(int32_t tx,int32_t ty,int32_t tz,int32_t txsz,int32_t tysz,int32_t tzsz);
    virtual bool hit(int32_t tx,int32_t ty,int32_t txsz,int32_t tysz);
    
    
    virtual int32_t hitdir(int32_t tx,int32_t ty,int32_t txsz,int32_t tysz,int32_t dir);
    virtual void move(zfix dx,zfix dy);
    virtual void move(zfix s);
	virtual bool knockback(int32_t time, int32_t dir, int32_t speed);
	virtual bool runKnockback();
    void explode(int32_t mode);
	bool getCanFlicker();
	void setCanFlicker(bool v);
	
	virtual int32_t run_script(int32_t mode);
	
	virtual ALLEGRO_COLOR hitboxColor(byte opacity = 255) const;
	virtual void draw_hitbox();
	
	//Script helper funcs
	virtual optional<ScriptType> get_scrtype() const {return nullopt;}
	
};

enum //run_script modes
{
	MODE_NORMAL,
	MODE_WAITDRAW
};

#define SLMAX 255*(511*4)+1

class sprite_list
{
    sprite *sprites[SLMAX];
    int32_t count;
	int32_t active_iterator;
    bool delete_active_iterator;
	int32_t max_sprites;
    map<int32_t, int32_t> containedUIDs;
    // Cache requests from scripts
    mutable int32_t lastUIDRequested;
    mutable sprite* lastSpriteRequested;
    
public:
    sprite_list();
    
    sprite *getByUID(int32_t uid);
    void clear(bool force = false);
    sprite *spr(int32_t index);
	int32_t find(sprite *spr);
    bool swap(int32_t a,int32_t b);
    bool add(sprite *s);
    // removes pointer from list but doesn't delete it
    bool remove(sprite *s);
    zfix getX(int32_t j);
    zfix getY(int32_t j);
    int32_t getID(int32_t j);
    int32_t getMisc(int32_t j);
	int32_t getMax() {return max_sprites;}
	void setMax(int32_t max) {max_sprites = (max < SLMAX ? max : SLMAX);}
    bool del(int32_t j, bool force = false, bool may_defer = true);
    void draw(BITMAP* dest,bool lowfirst);
    void drawshadow(BITMAP* dest,bool translucent, bool lowfirst);
    void draw2(BITMAP* dest,bool lowfirst);
    void drawcloaked2(BITMAP* dest,bool lowfirst);
    void animate();
	void solid_push(solid_object* pusher);
    void check_conveyor();
	void run_script(int32_t mode);
    int32_t Count();
	bool has_space(int32_t space = 1);
    int32_t hit(sprite *s);
    int32_t hit(int32_t x,int32_t y,int32_t z,int32_t xsize, int32_t ysize, int32_t zsize);
    int32_t hit(int32_t x,int32_t y,int32_t xsize, int32_t ysize);
    // returns the number of sprites with matching id
    int32_t idCount(int32_t id, int32_t mask);
    // returns index of first sprite with matching id, -1 if none found
    int32_t idFirst(int32_t id, int32_t mask);
    // returns index of nth sprite with matching id, -1 if none found
    int32_t idNth(int32_t id, int32_t n, int32_t mask);
    // returns index of last sprite with matching id, -1 if none found
    int32_t idLast(int32_t id, int32_t mask);
    // returns the number of sprites with matching id
    int32_t idCount(int32_t id);
	// returns the number of sprites matching any id in the set
	int32_t idCount(std::set<int32_t> const& ids);
    // returns index of first sprite with matching id, -1 if none found
    int32_t idFirst(int32_t id);
    // returns index of nth sprite with matching id, -1 if none found
    int32_t idNth(int32_t id, int32_t n);
    // returns index of last sprite with matching id, -1 if none found
    int32_t idLast(int32_t id);
    
	void forEach(std::function<bool(sprite&)> proc);
	
private:

    void checkConsistency(); //for debugging
};

class movingblock : public sprite
{
public:
    int32_t bcombo;
    int32_t oldflag;
    int32_t oldcset;
    int32_t endx, endy;
    bool trigger, bhole;
    byte undercset;
	byte blockLayer;
	zfix step;
	bool force_many;
	bool no_icy;
	bool new_block;
    
	cpos_info blockinfo;
	
    movingblock();
	void clear();
	void set(int32_t X, int32_t Y, int32_t combo, int32_t cset, int32_t layer, int32_t placedfl);
    void push(zfix bx,zfix by,int32_t d,int32_t f);
    void push_new(zfix bx,zfix by,int32_t d,int32_t f,zfix spd);
	bool check_hole() const;
	bool check_trig() const;
	bool active() const;
    virtual bool animate(int32_t index);
    virtual void draw(BITMAP *dest);
    bool draw(BITMAP *dest, int layer);
};

class portal : public sprite
{
public:
	int32_t destdmap = -1, destscr, weffect, wsfx;
    int32_t aclk, aframe, aspd, frames;
	int32_t saved_data;
	bool prox_active;
	portal();
	portal(int32_t dm, int32_t scr, int32_t gfx, int32_t sfx, int32_t spr);
	virtual bool animate(int32_t);
	void LOADGFX(int32_t spr);
	void clear();
};

class breakable : public sprite
{
public:
    int32_t aclk, aframe, aspd, frames;
	newcombo const& cmb;
	int32_t dropitem, breaktimer, fromdropset;
	int8_t breaksprtype;
	byte breaksfx,breakspr;
	
	breakable(zfix X, zfix Y, zfix Z, newcombo const& cmb, int32_t cset);
	breakable(zfix X, zfix Y, zfix Z, newcombo const& cmb, int32_t cset, int32_t dropitem,
		int32_t fromdropset, byte breaksfx, int8_t breaksprtype, byte breakspr, int32_t breaktimer);
	
	virtual bool animate(int32_t);
};

bool insideRotRect(double x, double y, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t x4, int32_t y4);
bool lineLineColl(zfix x1, zfix y1, zfix x2, zfix y2, zfix x3, zfix y3, zfix x4, zfix y4);
bool lineBoxCollision(zfix linex1, zfix liney1, zfix linex2, zfix liney2, zfix boxx, zfix boxy, zfix boxwidth, zfix boxheight);
double comparePointLine(double x, double y, double x1, double x2, double y1, double y2);

#endif
