#include "base/zdefs.h"
#include "base/zsys.h"
#include "base/qrs.h"
#include "sprite.h"
#include "tiles.h"
#include "particles.h"
#include "zc/maps.h"
#include "zc/replay.h"
#include "zc/guys.h"
#include "base/zc_math.h"
#include <fmt/format.h>
#include "base/misctypes.h"
#include "drawing.h"

#ifdef IS_PLAYER
#include "zc/hero.h"
#include "zc/decorations.h"
#include "items.h"
#include "zc/render.h"
extern HeroClass Hero;
extern sprite_list decorations;
#endif
extern particle_list particles;

extern bool get_debug();
extern bool halt;
extern bool show_sprites;
extern bool show_hitboxes;
extern void debugging_box(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
#include "zc/ffscript.h"

static std::map<int32_t, sprite*> all_sprites;
byte sprite_flicker_color = 0;
byte sprite_flicker_transp_passes = 0;
byte sprite_mask_color = 0;
std::vector<std::tuple<byte,byte,byte>> flicker_storage;
void sprite_push_flicker()
{
	flicker_storage.emplace_back(sprite_flicker_color,
		sprite_flicker_transp_passes, sprite_mask_color);
	sprite_clear_flicker();
}
void sprite_pop_flicker()
{
	auto [sfc, sftp, smc] = flicker_storage.back();
	
	sprite_flicker_color = sfc;
	sprite_flicker_transp_passes = sftp;
	sprite_mask_color = smc;
	
	flicker_storage.pop_back();
}
void sprite_clear_flicker()
{
	sprite_flicker_color = 0;
	sprite_flicker_transp_passes = 0;
	sprite_mask_color = 0;
}

#define degtoFix(d)     ((d)*0.7111111111111)
#define radtoFix(d)     ((d)*40.743665431525)

template<class T> inline
fixed deg_to_fixed(T d)
{
    return ftofix(degtoFix(d));
}
template<class T> inline
fixed rad_to_fixed(T d)
{
    return ftofix(radtoFix(d));
}

sprite::sprite(): solid_object()
{
    uid = 0;
	isspawning = false;
    x=y=z=tile=shadowtile=cs=flip=c_clk=clk=xofs=yofs=shadowxofs=shadowyofs=zofs=fall=fakefall=fakez=0;
    slopeid = 0;
    onplatid = 0;
    txsz=1;
    tysz=1;
    id=-1;
    hzsz=1;
    yofs=(get_qr(qr_OLD_DRAWOFFSET)?playing_field_offset:original_playing_field_offset);
    dir=down;
    angular=canfreeze=false;
    drawstyle=0;
    extend=0;
    wpnsprite = 0;
    lasthitclk=0;
    lasthit=0;
    angle=0;
    misc=0;
	flickercolor = -1;
	flickertransp = -1;
    pit_pulldir = -1;
	pit_pullclk = 0;
	fallclk = 0;
	fallCombo = 0;
	o_cset = 0;
	drownclk = 0;
	drownCombo = 0;
	
    for(int32_t i=0; i<10; i++)
    {
        dummy_int[i]=0;
        dummy_fix[i]=0;
        dummy_float[i]=0;
        dummy_bool[i]=false;
    }
    
    scriptflag=0;
    for(int32_t i=0; i<32; i++) miscellaneous[i] = 0;
    
    scriptcoldet = true;
    script = 0;
    weaponscript = 0;
    scripttile = -1;
    scriptflip = -1;
    do_animation = true;
    rotation = 0;
    scale = 0;
    moveflags = move_none;
    drawflags = 0;
	knockbackflags = 0;
	knockbackSpeed = 4; //default speed
	script_knockback_clk = 0;
	script_knockback_speed = 0;
	screenedge = 0;
	scriptshadowtile = -1;
    for ( int32_t q = 0; q < 8; q++ )
    {
	    initD[q] = 0;
	    weap_initd[q] = 0;
    }
	glowRad = 0;
	glowShape = 0;
	switch_hooked = false;
	ignore_delete = 0;
	can_flicker = true;
	spr_shadow = iwShadow;
	spr_death = iwDeath;
	spr_spawn = iwSpawn;
	spr_death_anim_clk = 0;
	spr_death_anim_frm = 0;
	spr_spawn_anim_clk = 0;
	spr_spawn_anim_frm = 0;
	hide_hitbox = false;
}

sprite::sprite(sprite const & other):
	solid_object(other),
    z(other.z), fall(other.fall), fakez(other.fakez), tile(other.tile),
    shadowtile(other.shadowtile), cs(other.cs), flip(other.flip),
    c_clk(other.c_clk), clk(other.clk), misc(other.misc), xofs(other.xofs),
    yofs(other.yofs), zofs(other.zofs), hzsz(other.hzsz), txsz(other.txsz),
    tysz(other.tysz), id(other.id), slopeid(other.slopeid),
    onplatid(other.onplatid), angular(other.angular), canfreeze(other.canfreeze),
    angle(other.angle), lasthit(other.lasthit), lasthitclk(other.lasthitclk),
    drawstyle(other.drawstyle), extend(other.extend), wpnsprite(other.wpnsprite),
	scriptflag(other.scriptflag), script(other.script), weaponscript(other.weaponscript),
	scripttile(other.scripttile), scriptflip(other.scriptflip),
	do_animation(other.do_animation), rotation(other.rotation),
	scale(other.scale), moveflags(other.moveflags), drawflags(other.drawflags),
	knockbackflags(other.knockbackflags), screenedge(other.screenedge),
	scriptshadowtile(other.scriptshadowtile), knockbackSpeed(other.knockbackSpeed),
	script_knockback_clk(other.script_knockback_clk),
	script_knockback_speed(other.script_knockback_speed),
	pit_pulldir(other.pit_pulldir), pit_pullclk(other.pit_pullclk),
	fallclk(other.fallclk), fallCombo(other.fallCombo),
	o_tile(other.o_tile), o_cset(other.o_cset), drownclk(other.drownclk),
	drownCombo(other.drownCombo), can_flicker(other.can_flicker),
	spr_shadow(other.spr_shadow), spr_death(other.spr_death),
	spr_spawn(other.spr_spawn), spr_death_anim_clk(other.spr_death_anim_clk),
	spr_spawn_anim_clk(other.spr_spawn_anim_clk),
	spr_death_anim_frm(other.spr_death_anim_frm),
	spr_spawn_anim_frm(other.spr_spawn_anim_frm),
	glowRad(other.glowRad), glowShape(other.glowShape),
	ignore_delete(other.ignore_delete)
{
    uid = 0;
	all_sprites[uid] = this;
	isspawning = other.isspawning;
    
    for(int32_t i=0; i<10; ++i)
    {
        dummy_int[i]=other.dummy_int[i];
        dummy_fix[i]=other.dummy_fix[i];
        dummy_float[i]=other.dummy_float[i];
        dummy_bool[i]=other.dummy_bool[i];
    }

    for(int32_t i=0; i<32; i++) miscellaneous[i] = other.miscellaneous[i];
    
    scriptcoldet = other.scriptcoldet;
    
    
	for (int32_t i=0; i<8; ++i)
	{
		initD[i]=other.initD[i];
		weap_initd[i]=other.weap_initd[i];
	}
}

sprite::sprite(zfix X,zfix Y,int32_t T,int32_t CS,int32_t F,int32_t Clk,int32_t Yofs):
    tile(T),cs(CS),flip(F),clk(Clk),yofs(Yofs)
{
	x = X;
	y = Y;
    uid = 0;
	all_sprites[uid] = this;
    isspawning = false;
    slopeid = 0;
    onplatid = 0;
    hit_width=hit_height=16;
    hxofs=hyofs=xofs=0;
    txsz=1;
    tysz=1;
    id=-1;
    dir=down;
    angular=canfreeze=false;
    extend=0;
    
    scriptflag=0;
    for(int32_t i=0; i<32; i++) miscellaneous[i] = 0;
    
    scriptcoldet = true;
    script = 0;
    weaponscript = 0;
    scripttile = -1;
    scriptflip = -1;
    rotation = 0;
    moveflags = move_none;
	knockbackflags = 0;
	knockbackSpeed = 4; //default speed
	script_knockback_clk = 0;
	script_knockback_speed = 0;
    scale = 0;
    do_animation = true;
    drawstyle=0;
    lasthitclk=0;
    lasthit=0;
    angle=0;
    hzsz=1;
    misc=0;
	flickercolor = -1;
	flickertransp = -1;
    c_clk=0;
    shadowtile=0;
    screenedge = 0;
    scriptshadowtile = -1;
    pit_pulldir = -1;
    pit_pullclk = 0;
    fallclk = 0;
    fallCombo = 0;
    o_cset = 0;
    drownclk = 0;
    drownCombo = 0;
    drawflags = 0;
    for ( int32_t q = 0; q < 8; q++ ) 
    {
        initD[q] = 0;
        weap_initd[q] = 0;
    }
	glowRad = 0;
	glowShape = 0;
	switch_hooked = false;
	ignore_delete = 0;
	can_flicker = true;
	
	//Defaults for old hardcoded sprites
	spr_shadow = iwShadow;
	spr_death = iwDeath;
	spr_spawn = iwSpawn;
	spr_death_anim_clk = 0;
	spr_death_anim_frm = 0;
	spr_spawn_anim_clk = 0;
	spr_spawn_anim_frm = 0;
}

sprite::~sprite()
{
	// The order dtors run for globals across translation units is undefined, so to prevent
	// possibly accessing `all_sprites` post-delete (which can happen in MSVC), do nothing here if
	// shutting down.
	if (is_exiting())
		return;

	#ifdef IS_PLAYER
	all_sprites.erase(uid);
	if(auto scrty = get_scrtype())
	{
		FFCore.clear_script_engine_data(*scrty, getUID());
	}
	#endif
}

// 0 is an unassigned uid.
// 1 is reserved for Hero.
static int32_t next_uid = 2;

sprite* sprite::getByUID(int32_t uid)
{
#ifdef IS_PLAYER
	if (uid == 1)
		return &Hero;
#endif

	auto it = all_sprites.find(uid);
    return it == all_sprites.end() ? nullptr : it->second;
}
void sprite::registerUID()
{
	if (!uid)
		uid = next_uid++;

	all_sprites.erase(uid);
	all_sprites[uid] = this;
}
void sprite::reassignUid(int32_t new_uid)
{
	all_sprites.erase(uid);
	uid = new_uid;
	all_sprites[uid] = this;
}

void sprite::draw2(BITMAP *)                            // top layer for special needs
{
}

void sprite::drawcloaked2(BITMAP *)                     // top layer for special needs
{
}

bool sprite::animate(int32_t)
{
    ++c_clk;
    return false;
}
void sprite::post_animate()
{
	updateSolid();
	solid_update();
}
int32_t sprite::real_x(zfix fx)
{
    int32_t rx = fx.getInt();
    switch(dir)
    {
    case 9:
    case 13:
        if(fx.getZLong() & 0xFFFF) //No idea what the hell this is doing, but it sure isn't just getting the decimal portion.
            ++rx;
            
        break;
    }
    return rx;
}

int32_t sprite::real_y(zfix fy)
{
    return fy.getInt();
}

int32_t sprite::real_z(zfix fz)
{
    return fz.getInt();
}

int32_t sprite::fake_z(zfix fz)
{
    return fz.getInt();
}

int32_t sprite::get_pit() //Returns combo ID of pit that sprite WOULD fall into; no side-effects
{
	int32_t ispitul = getpitfall(x,y);
	int32_t ispitbl = getpitfall(x,y+15);
	int32_t ispitur = getpitfall(x+15,y);
	int32_t ispitbr = getpitfall(x+15,y+15);
	int32_t ispitul_50 = getpitfall(x+8,y+8);
	int32_t ispitbl_50 = getpitfall(x+8,y+7);
	int32_t ispitur_50 = getpitfall(x+7,y+8);
	int32_t ispitbr_50 = getpitfall(x+7,y+7);
	switch((ispitul?1:0) + (ispitur?1:0) + (ispitbl?1:0) + (ispitbr?1:0))
	{
		case 4:
		{
			return ispitul_50 ? ispitul_50 : ispitul;
		}
		case 3:
		{
			if(ispitul && ispitur && ispitbl) //UL_3
			{
				return ispitul_50;
			}
			else if(ispitul && ispitur && ispitbr) //UR_3
			{
				return ispitur_50;
			}
			else if(ispitul && ispitbl && ispitbr) //BL_3
			{
				return ispitbl_50;
			}
			else if(ispitbl && ispitur && ispitbr) //BR_3
			{
				return ispitbr_50;
			}
			break;
		}
		case 2:
		{
			if(ispitul && ispitur) //Up
			{
				return ispitul_50 ? ispitul_50 : ispitur_50;
			}
			if(ispitbl && ispitbr) //Down
			{
				return ispitbl_50 ? ispitbl_50 : ispitbr_50;
			}
			if(ispitul && ispitbl) //Left
			{
				return ispitul_50 ? ispitul_50 : ispitbl_50;
			}
			if(ispitur && ispitbr) //Right
			{
				return ispitur_50 ? ispitur_50 : ispitbr_50;
			}
			break;
		}
		case 1:
		{
			if(ispitul) //UL_1
			{
				return ispitul_50;
			}
			if(ispitur) //UR_1
			{
				return ispitur_50;
			}
			if(ispitbl) //BL_1
			{
				return ispitbl_50;
			}
			if(ispitbr) //BR_1
			{
				return ispitbr_50;
			}
			break;
		}
	}
	return 0;
}

int32_t sprite::check_pits() //Returns combo ID of pit fallen into; 0 for not fallen.
{
	int32_t safe_cnt = 0;
	bool has_fallen = false;
	int32_t ispitul, ispitbl, ispitur, ispitbr, ispitul_50, ispitbl_50, ispitur_50, ispitbr_50;
	while(++safe_cnt < 16) //Prevent softlocks
	{
		ispitul = getpitfall(x,y);
		ispitbl = getpitfall(x,y+15);
		ispitur = getpitfall(x+15,y);
		ispitbr = getpitfall(x+15,y+15);
		ispitul_50 = getpitfall(x+7,y+7);
		ispitbl_50 = getpitfall(x+7,y+8);
		ispitur_50 = getpitfall(x+8,y+7);
		ispitbr_50 = getpitfall(x+8,y+8);
		int32_t dir = -1;
		size_t cnt = (ispitul?1:0) + (ispitur?1:0) + (ispitbl?1:0) + (ispitbr?1:0);
		switch(cnt)
		{
			case 4:
			{
				fallclk = PITFALL_FALL_FRAMES; //Fall
				has_fallen = true;
				o_cset = cs;
				return ispitul_50 ? ispitul_50 : ispitul;
			}
			case 3:
			{
				if(ispitul && ispitur && ispitbl) //UL_3
				{
					if(ispitul_50)
					{
						dir=l_up; break;
					}
				}
				else if(ispitul && ispitur && ispitbr) //UR_3
				{
					if(ispitur_50)
					{
						dir=r_up; break;
					}
				}
				else if(ispitul && ispitbl && ispitbr) //BL_3
				{
					if(ispitbl_50)
					{
						dir=l_down; break;
					}
				}
				else if(ispitbl && ispitur && ispitbr) //BR_3
				{
					if(ispitbr_50)
					{
						dir=r_down; break;
					}
				}
				break;
			}
			case 2:
			{
				if(ispitul && ispitur) //Up
				{
					if(ispitul_50 && ispitur_50) //Straight up
					{
						dir = up; break;
					}
					else if(ispitul_50)
					{
						dir = l_up; break;
					}
					else if(ispitur_50)
					{
						dir = r_up; break;
					}
				}
				if(ispitbl && ispitbr) //Down
				{
					if(ispitbl_50 && ispitbr_50) //Straight down
					{
						dir = down; break;
					}
					else if(ispitbl_50)
					{
						dir = l_down; break;
					}
					else if(ispitbr_50)
					{
						dir = r_down; break;
					}
				}
				if(ispitul && ispitbl) //Left
				{
					if(ispitul_50 && ispitbl_50) //Straight left
					{
						dir = left; break;
					}
					else if(ispitul_50)
					{
						dir = l_up; break;
					}
					else if(ispitbl_50)
					{
						dir = l_down; break;
					}
				}
				if(ispitur && ispitbr) //Right
				{
					if(ispitur_50 && ispitbr_50) //Straight right
					{
						dir = right; break;
					}
					else if(ispitur_50)
					{
						dir = r_up; break;
					}
					else if(ispitbr_50)
					{
						dir = r_down; break;
					}
				}
				break;
			}
			case 1:
			{
				if(ispitul && ispitul_50) //UL_1
				{
					dir = l_up; break;
				}
				if(ispitur && ispitur_50) //UR_1
				{
					dir = r_up; break;
				}
				if(ispitbl && ispitbl_50) //BL_1
				{
					dir = l_down; break;
				}
				if(ispitbr && ispitbr_50) //BR_1
				{
					dir = r_down; break;
				}
				break;
			}
		}
		if(dir == -1) return 0; //Not falling
		has_fallen = true;
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
	}
	if(has_fallen)
	{
		int32_t old_fall = fallclk; //sanity check
		fallclk = PITFALL_FALL_FRAMES;
		o_cset = cs;
		if(ispitul_50) return ispitul_50;
		if(ispitur_50) return ispitur_50;
		if(ispitbl_50) return ispitbl_50;
		if(ispitbr_50) return ispitbr_50;
		if(ispitul) return ispitul;
		if(ispitur) return ispitur;
		if(ispitbl) return ispitbl;
		if(ispitbr) return ispitbr;
		fallclk = old_fall; //sanity check
	}
	return 0;
}

int32_t sprite::get_water()  //Returns combo ID of water that sprite WOULD fall into; no side-effects
{
	int32_t ispitul = iswaterexzq(MAPCOMBOzq(x,y), get_currmap(), get_currscr(), -1, x, y, false, false, true);
	int32_t ispitbl = iswaterexzq(MAPCOMBOzq(x,y+15), get_currmap(), get_currscr(), -1, x, y+15, false, false, true);
	int32_t ispitur = iswaterexzq(MAPCOMBOzq(x+15,y), get_currmap(), get_currscr(), -1, x+15, y, false, false, true);
	int32_t ispitbr = iswaterexzq(MAPCOMBOzq(x+15,y+15), get_currmap(), get_currscr(), -1, x+15, y+15, false, false, true);
	int32_t ispitul_50 = iswaterexzq(MAPCOMBOzq(x+8,y+8), get_currmap(), get_currscr(), -1, x+8, y+8, false, false, true);
	int32_t ispitbl_50 = iswaterexzq(MAPCOMBOzq(x+8,y+7), get_currmap(), get_currscr(), -1, x+8, y+7, false, false, true);
	int32_t ispitur_50 = iswaterexzq(MAPCOMBOzq(x+7,y+8), get_currmap(), get_currscr(), -1, x+7, y+8, false, false, true);
	int32_t ispitbr_50 = iswaterexzq(MAPCOMBOzq(x+7,y+7), get_currmap(), get_currscr(), -1, x+7, y+7, false, false, true);
	switch((ispitul?1:0) + (ispitur?1:0) + (ispitbl?1:0) + (ispitbr?1:0))
	{
		case 4:
		{
			return ispitul_50 ? ispitul_50 : ispitul;
		}
		case 3:
		{
			if(ispitul && ispitur && ispitbl) //UL_3
			{
				return ispitul_50;
			}
			else if(ispitul && ispitur && ispitbr) //UR_3
			{
				return ispitur_50;
			}
			else if(ispitul && ispitbl && ispitbr) //BL_3
			{
				return ispitbl_50;
			}
			else if(ispitbl && ispitur && ispitbr) //BR_3
			{
				return ispitbr_50;
			}
			break;
		}
		case 2:
		{
			if(ispitul && ispitur) //Up
			{
				return ispitul_50 ? ispitul_50 : ispitur_50;
			}
			if(ispitbl && ispitbr) //Down
			{
				return ispitbl_50 ? ispitbl_50 : ispitbr_50;
			}
			if(ispitul && ispitbl) //Left
			{
				return ispitul_50 ? ispitul_50 : ispitbl_50;
			}
			if(ispitur && ispitbr) //Right
			{
				return ispitur_50 ? ispitur_50 : ispitbr_50;
			}
			break;
		}
		case 1:
		{
			if(ispitul) //UL_1
			{
				return ispitul_50;
			}
			if(ispitur) //UR_1
			{
				return ispitur_50;
			}
			if(ispitbl) //BL_1
			{
				return ispitbl_50;
			}
			if(ispitbr) //BR_1
			{
				return ispitbr_50;
			}
			break;
		}
	}
	return 0;
}

int32_t sprite::check_water() //Returns combo ID of water fallen into; 0 for not fallen.
{
	int32_t safe_cnt = 0;
	bool has_fallen = false;
	int32_t ispitul, ispitbl, ispitur, ispitbr, ispitul_50, ispitbl_50, ispitur_50, ispitbr_50;
	while(++safe_cnt < 16) //Prevent softlocks
	{
		ispitul = iswaterexzq(MAPCOMBOzq(x,y), get_currmap(), get_currscr(), -1, x, y, false, false, true);
		ispitbl = iswaterexzq(MAPCOMBOzq(x,y+15), get_currmap(), get_currscr(), -1, x, y+15, false, false, true);
		ispitur = iswaterexzq(MAPCOMBOzq(x+15,y), get_currmap(), get_currscr(), -1, x+15, y, false, false, true);
		ispitbr = iswaterexzq(MAPCOMBOzq(x+15,y+15), get_currmap(), get_currscr(), -1, x+15, y+15, false, false, true);
		ispitul_50 = iswaterexzq(MAPCOMBOzq(x+8,y+8), get_currmap(), get_currscr(), -1, x+8, y+8, false, false, true);
		ispitbl_50 = iswaterexzq(MAPCOMBOzq(x+8,y+7), get_currmap(), get_currscr(), -1, x+8, y+7, false, false, true);
		ispitur_50 = iswaterexzq(MAPCOMBOzq(x+7,y+8), get_currmap(), get_currscr(), -1, x+7, y+8, false, false, true);
		ispitbr_50 = iswaterexzq(MAPCOMBOzq(x+7,y+7), get_currmap(), get_currscr(), -1, x+7, y+7, false, false, true);
		int32_t dir = -1;
		switch((ispitul?1:0) + (ispitur?1:0) + (ispitbl?1:0) + (ispitbr?1:0))
		{
			case 4:
			{
				drownclk = WATER_DROWN_FRAMES; //Fall
				o_cset = cs;
				return ispitul_50 ? ispitul_50 : ispitul;
			}
			case 3:
			{
				if(ispitul && ispitur && ispitbl) //UL_3
				{
					if(ispitul_50)
					{
						dir=l_up; break;
					}
				}
				else if(ispitul && ispitur && ispitbr) //UR_3
				{
					if(ispitur_50)
					{
						dir=r_up; break;
					}
				}
				else if(ispitul && ispitbl && ispitbr) //BL_3
				{
					if(ispitbl_50)
					{
						dir=l_down; break;
					}
				}
				else if(ispitbl && ispitur && ispitbr) //BR_3
				{
					if(ispitbr_50)
					{
						dir=r_down; break;
					}
				}
				break;
			}
			case 2:
			{
				if(ispitul && ispitur) //Up
				{
					if(ispitul_50 && ispitur_50) //Straight up
					{
						dir = up; break;
					}
					else if(ispitul_50)
					{
						dir = l_up; break;
					}
					else if(ispitur_50)
					{
						dir = r_up; break;
					}
				}
				if(ispitbl && ispitbr) //Down
				{
					if(ispitbl_50 && ispitbr_50) //Straight down
					{
						dir = down; break;
					}
					else if(ispitbl_50)
					{
						dir = l_down; break;
					}
					else if(ispitbr_50)
					{
						dir = r_down; break;
					}
				}
				if(ispitul && ispitbl) //Left
				{
					if(ispitul_50 && ispitbl_50) //Straight left
					{
						dir = left; break;
					}
					else if(ispitul_50)
					{
						dir = l_up; break;
					}
					else if(ispitbl_50)
					{
						dir = l_down; break;
					}
				}
				if(ispitur && ispitbr) //Right
				{
					if(ispitur_50 && ispitbr_50) //Straight right
					{
						dir = right; break;
					}
					else if(ispitur_50)
					{
						dir = r_up; break;
					}
					else if(ispitbr_50)
					{
						dir = r_down; break;
					}
				}
				break;
			}
			case 1:
			{
				if(ispitul && ispitul_50) //UL_1
				{
					dir = l_up; break;
				}
				if(ispitur && ispitur_50) //UR_1
				{
					dir = r_up; break;
				}
				if(ispitbl && ispitbl_50) //BL_1
				{
					dir = l_down; break;
				}
				if(ispitbr && ispitbr_50) //BR_1
				{
					dir = r_down; break;
				}
				break;
			}
		}
		if(dir == -1) return 0; //Not falling
		has_fallen = true;
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
	}
	if(has_fallen)
	{
		int32_t old_drown = drownclk; //sanity check
		drownclk = WATER_DROWN_FRAMES;
		o_cset = cs;
		if(ispitul_50) return ispitul_50;
		if(ispitur_50) return ispitur_50;
		if(ispitbl_50) return ispitbl_50;
		if(ispitbr_50) return ispitbr_50;
		if(ispitul) return ispitul;
		if(ispitur) return ispitur;
		if(ispitbl) return ispitbl;
		if(ispitbr) return ispitbr;
		fallclk = old_drown; //sanity check
	}
	return 0;
}

bool sprite::hit()
{
	if(!scriptcoldet || fallclk || drownclk) return false;
	if(id<0 || clk<0) return false;
	return true;
}

bool sprite::hit(sprite *s)
{
    if(!hit() || !s->hit()) return false;
    
    return hit(s->x+s->hxofs,s->y+s->hyofs-s->fakez,s->z+s->zofs,s->hit_width,s->hit_height,s->hzsz);
}

bool sprite::hit(int32_t tx,int32_t ty,int32_t txsz2,int32_t tysz2)
{
    if(!hit()) return false;
    
    return tx+txsz2>x+hxofs &&
           ty+tysz2>y+hyofs &&
           
           tx<x+hxofs+hit_width &&
           ty<y+hyofs+hit_height;
}

bool sprite::hit(int32_t tx,int32_t ty,int32_t tz,int32_t txsz2,int32_t tysz2,int32_t tzsz2)
{
    if(!hit()) return false;
    
    return tx+txsz2>x+hxofs &&
           ty+tysz2>y+hyofs-fakez &&
           tz+tzsz2>z+zofs &&
           
           tx<x+hxofs+hit_width &&
           ty<y+hyofs+hit_height-fakez &&
           tz<z+zofs+hzsz;
}

int32_t sprite::hitdir(int32_t tx,int32_t ty,int32_t txsz2,int32_t tysz2,int32_t dir2)
{
    if(!scriptcoldet || fallclk || drownclk) return 0xFF;
    
    int32_t cx1=x+hxofs+(hit_width>>1);
    int32_t cy1=y+hyofs+(hit_height>>1)-fakez;
    int32_t cx2=tx+(txsz2>>1);
    int32_t cy2=ty+(tysz2>>1);
    
    if(dir2>=left && abs(cy1-cy2)<=8)
        return (cx2-cx1<0)?left:right;
        
    return (cy2-cy1<0)?up:down;
}

void sprite::move(zfix dx,zfix dy)
{
    x+=dx;
    y+=dy;
}

void sprite::move(zfix s)
{
    if(angular)
    {
        x += zc::math::Cos(angle)*s;
        y += zc::math::Sin(angle)*s;
		return;
    }
    
    switch(dir)
    {
    case 8:
    case up:
        y-=s;
        break;
        
    case 12:
    case down:
        y+=s;
        break;
        
    case 14:
    case left:
        x-=s;
        break;
        
    case 10:
    case right:
        x+=s;
        break;
        
    case 15:
    case l_up:
        x-=s;
        y-=s;
        break;
        
    case 9:
    case r_up:
        x+=s;
        y-=s;
        break;
        
    case 13:
    case l_down:
        x-=s;
        y+=s;
        break;
        
    case 11:
    case r_down:
        x+=s;
        y+=s;
        break;
        
    case -1:
        break;
    }
}

bool sprite::knockback(int32_t time, int32_t dir, int32_t speed)
{
	if(knockbackflags & FLAG_NOSCRIPTKNOCKBACK) return false;
	script_knockback_clk = (time&0xFF) | ((dir&0xFF)<<8);
	script_knockback_speed = speed;
	return true;
}

bool sprite::runKnockback()
{
	return false; //Virtual; must be overridden for each class, for proper collision checking.
}

void doSpriteDraw(int32_t drawstyle, BITMAP* dest, BITMAP* src, int32_t x, int32_t y)
{
	if(drawstyle==1)
	{
		color_map = &trans_table2;
		draw_trans_sprite(dest, src, x, y);
		color_map = &trans_table;
	}
	else if(drawstyle==2)
		draw_cloaked_sprite(dest, src, x, y);
	else draw_sprite(dest, src, x, y);
}

#define SPRITE_MONOCOLOR(bmp) \
do \
{ \
	if(sprite_mask_color) \
		monocolor(bmp, sprite_mask_color, 0); \
	if(sprite_flicker_color) \
		monocolor(bmp, sprite_flicker_color, sprite_flicker_transp_passes); \
} while(false)

void sprite::draw(BITMAP* dest)
{
	//Handle glowing sprites
	handle_sprlighting();
	if(!show_sprites)
	{
		return;
	}
	zfix tyoffs = yofs;
#ifdef IS_PLAYER
	if(switch_hooked)
	{
		switch(Hero.switchhookstyle)
		{
			default: case swPOOF:
				break; //Nothing special here
			case swFLICKER:
			{
				if(abs(Hero.switchhookclk-33)&0b1000)
					break; //Drawn this frame
				return; //Not drawn this frame
			}
			case swRISE:
			{
				//Draw rising up
				yofs -= 8-(abs(Hero.switchhookclk-32)/4);
				break;
			}
		}
	}
#endif
	if (FFCore.getQuestHeaderInfo(0) < 0x255 || ( FFCore.getQuestHeaderInfo(0) == 0x255 && FFCore.getQuestHeaderInfo(2) < 42 ) || get_qr(qr_OLDSPRITEDRAWS) || (drawflags & sprdrawflagALWAYSOLDDRAWS))
	{
		drawzcboss(dest);
		yofs = tyoffs;
		return; //don't run the rest, use the old code
	}
	int32_t sx = real_x(x+xofs);
	int32_t sy = real_y(y+yofs)-real_z(z+zofs);
	sy -= fake_z(fakez);
	
    
	if(id<0)
	{
		yofs = tyoffs;
		sprite_clear_flicker();
		return;
	}
	int32_t e = extend>=3 ? 3 : extend;
	int32_t flip_type = ((scriptflip > -1) ? scriptflip : flip);
	isspawning = false;
	#define TILEBOUND(t) vbound(t,0,NEWMAXTILES)
	if(clk>=0)
	{
		static BITMAP* sprBMP2 = create_bitmap_ex(8, 256, 256);
		clear_bitmap(sprBMP2);

		switch(e)
		{
			case 1:
			{
				static BITMAP *temp = create_bitmap_ex(8, 16, 32);
				blit(dest, temp, sx, sy-16, 0, 0, 16, 32);

				static BITMAP *temp2 = create_bitmap_ex(8, 16, 32);
				clear_bitmap(temp2);

				//Draw sprite tiles to the temp (scratch) bitmap.
				overtile16(temp2,TILEBOUND(((scripttile > -1) ? scripttile : tile)-TILES_PER_ROW),0,0,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp2,TILEBOUND((scripttile > -1) ? scripttile : tile),0,16,cs,((scriptflip > -1) ? scriptflip : flip));

				//Recolor for flicker animations
				SPRITE_MONOCOLOR(temp2);

				masked_blit(temp2, temp, 0, 0, 0, 0, 16, 32);

				//Blit to the screen...
				if ( rotation )
				{	
					//First rotating and scaling as needed to a scratch-bitmap.
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
					}
					else rotate_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(rotation));
					doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
				}
				else
				{
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(0),ftofix(new_scale));
						doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
					}
					else doSpriteDraw(drawstyle, dest, temp, sx, sy-16);
				}
				break;
			}
			case 2:
			{
				static BITMAP *temp = create_bitmap_ex(8, 48, 32);
				blit(dest, temp, sx-16, sy-16, 0, 0, 48, 32);

				static BITMAP *temp2 = create_bitmap_ex(8, 32, 32);
				static BITMAP* temp3 = create_bitmap_ex(8, 32, 32);

				clear_bitmap(temp3);
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)-TILES_PER_ROW),16,0,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)-TILES_PER_ROW-( ( scriptflip > -1 ) ? ( scriptflip ? -1 : 1 ) : ( flip?-1:1 ) )),0,0,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)-TILES_PER_ROW+( ( scriptflip > -1 ) ? ( scriptflip ? -1 : 1 ) : ( flip?-1:1 ) )),32,0,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)),16,16,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)-( ( scriptflip > -1 ) ? ( scriptflip ? -1 : 1 ) : ( flip?-1:1 ) )),0,16,cs,((scriptflip > -1) ? scriptflip : flip));
				overtile16(temp3,TILEBOUND(((scripttile > -1) ? scripttile : tile)+( ( scriptflip > -1 ) ? ( scriptflip ? -1 : 1 ) : ( flip?-1:1 ) )),32,16,cs,((scriptflip > -1) ? scriptflip : flip));
				
				//Recolor for flicker animations
				SPRITE_MONOCOLOR(temp3);

				masked_blit(temp3, temp, 0, 0, 0, 0, 32, 32);

				if ( rotation )
				{
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
					}
					else rotate_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(rotation));
					blit(sprBMP2, temp2, 8, 0, 0, 0, 32, 32);
					doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
				}
				else
				{
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, temp, 0, 0, deg_to_fixed(0),ftofix(new_scale));
						blit(sprBMP2, temp2, 8, 0, 0, 0, 32, 32);
						doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
					}
					else 
					{
						blit(temp, temp2, 8, 0, 0, 0, 32, 32);
						doSpriteDraw(drawstyle, dest, temp2, sx-8, sy-16);
					}
				}
				break;
			}
			case 3:
			{
				int32_t tileToDraw;
            
				switch(flip_type)
				{
					case 1:
					{
						BITMAP* sprBMP = create_bitmap_ex(8,txsz*16,tysz*16);
						clear_bitmap(sprBMP);
						for(int32_t i=0; i<tysz; i++)
						{
							for(int32_t j=txsz-1; j>=0; j--)
							{
								tileToDraw=((scripttile > -1) ? scripttile : tile)+(i*TILES_PER_ROW)+j;
							
								if(tileToDraw%TILES_PER_ROW<j) // Wrapped around
									tileToDraw+=TILES_PER_ROW*(tysz-1);
							    
								overtile16(sprBMP,tileToDraw,0+(txsz-j-1)*16,0+i*16,cs,((scriptflip > -1) ? scriptflip : flip));
							}
						}

						//Recolor for flicker animations
						SPRITE_MONOCOLOR(sprBMP);

						if ( rotation )
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
							}
							else rotate_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation));
							doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
							
						}
						else
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(0),ftofix(new_scale));
								doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
							}
							else doSpriteDraw(drawstyle, dest, sprBMP, sx, sy);
						}
							
						destroy_bitmap(sprBMP);
					} //end extend == 3 && flip == 1
					break;
                
					case 2:
					{
						BITMAP* sprBMP = create_bitmap_ex(8,txsz*16,tysz*16);
						clear_bitmap(sprBMP);
						for(int32_t i=tysz-1; i>=0; i--)
						{
							for(int32_t j=0; j<txsz; j++)
							{
								tileToDraw=((scripttile > -1) ? scripttile : tile)+(i*TILES_PER_ROW)+j;
							
								if(tileToDraw%TILES_PER_ROW<j)
									tileToDraw+=TILES_PER_ROW*(tysz-1);
							    
								overtile16(sprBMP,tileToDraw,0+j*16,0+(tysz-i-1)*16,cs,((scriptflip > -1) ? scriptflip : flip));
							}
						}

						//Recolor for flicker animations
						SPRITE_MONOCOLOR(sprBMP);

						if ( rotation )
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
							}
							else rotate_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation));
							doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
						}
						else
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(0),ftofix(new_scale));
								doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
							}
							else doSpriteDraw(drawstyle, dest, sprBMP, sx, sy);
						}
							
						destroy_bitmap(sprBMP);
					}//end extend == 3 &7 flip == 2
					break;
                
					case 3:
					{
						BITMAP* sprBMP = create_bitmap_ex(8,txsz*16,tysz*16);
						clear_bitmap(sprBMP);
						for(int32_t i=tysz-1; i>=0; i--)
						{
							for(int32_t j=txsz-1; j>=0; j--)
							{
								tileToDraw=((scripttile > -1) ? scripttile : tile)+(i*TILES_PER_ROW)+j;
							
								if(tileToDraw%TILES_PER_ROW<j)
									tileToDraw+=TILES_PER_ROW*(tysz-1);
							    
								overtile16(sprBMP,tileToDraw,0+(txsz-j-1)*16,0+(tysz-i-1)*16,cs,((scriptflip > -1) ? scriptflip : flip));
							}
						}

						//Recolor for flicker animations
						SPRITE_MONOCOLOR(sprBMP);

						if ( rotation )
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
							}
							else rotate_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation));
							doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
						}
						else
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(0),ftofix(new_scale));
								doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
							}
							else doSpriteDraw(drawstyle, dest, sprBMP, sx, sy);
						}
							
						destroy_bitmap(sprBMP);
					} //end extend == 3 && flip == 3
					break;
                
					case 0:
					{
						BITMAP* sprBMP = create_bitmap_ex(8,txsz*16,tysz*16);
						clear_bitmap(sprBMP);
						
						for(int32_t i=0; i<tysz; i++)
						{
							for(int32_t j=0; j<txsz; j++)
							{
								tileToDraw=((scripttile > -1) ? scripttile : tile)+(i*TILES_PER_ROW)+j;

								if(tileToDraw%TILES_PER_ROW<j)
									tileToDraw+=TILES_PER_ROW*(tysz-1);

								overtile16(sprBMP,tileToDraw,0+j*16,0+i*16,cs,((scriptflip > -1) ? scriptflip : flip));
							}
						}

						//Recolor for flicker animations
						SPRITE_MONOCOLOR(sprBMP);

						//rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, 0,ftofix(new_scale));
						if ( rotation )
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
							}
							else rotate_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation));
							doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
						}
						else
						{
							if ( scale ) 
							{
								double new_scale = scale / 100.0;
								rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(0),ftofix(new_scale));
								doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
							}
							else doSpriteDraw(drawstyle, dest, sprBMP, sx, sy);
						}
						
						destroy_bitmap(sprBMP);
                
						break;
					} //end extend == 0 && flip == 3
				}
				break;
			}
			case 0:
			{
				BITMAP* sprBMP = create_bitmap_ex(8,txsz*16,tysz*16);
				clear_bitmap(sprBMP);
				overtile16(sprBMP,TILEBOUND(scripttile > -1 ? scripttile : tile),0,0,cs,((scriptflip > -1) ? scriptflip : flip));
				
				//Recolor for flicker animations
				SPRITE_MONOCOLOR(sprBMP);

				if ( rotation )
				{
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation),ftofix(new_scale));
					}
					else rotate_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(rotation));
					doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
				}
				else
				{
					if ( scale ) 
					{
						double new_scale = scale / 100.0;
						rotate_scaled_sprite(sprBMP2, sprBMP, 0, 0, deg_to_fixed(0),ftofix(new_scale));
						doSpriteDraw(drawstyle, dest, sprBMP2, sx, sy);
					}
					else doSpriteDraw(drawstyle, dest, sprBMP, sx, sy);
				}
				if ( sprBMP ) destroy_bitmap(sprBMP);
				break;
			}
			break; //Aye, we break switch(e) here.
		}
	} //end if(clk>=0)
	else if (!fallclk && !drownclk) //I'm unsure when the clk is < 0 -Z
	{
		isspawning = true;
		if(e!=3) //if extend != 3 
		{
			int32_t t  = wpnsbuf[spr_spawn].tile;
			int32_t cs2 = wpnsbuf[spr_spawn].csets&15;
            
			if(!get_qr(qr_HARDCODED_ENEMY_ANIMS))
			{
				if(clk < -2)
				{
					spr_spawn_anim_clk=0;
					clk=-1;
				}
				if(clk==-1 && spr_spawn_anim_clk>-1)
				{
					--clk;
					spr_spawn_anim_frm=(spr_spawn_anim_clk/zc_max(wpnsbuf[spr_spawn].speed,1));
					if(++spr_spawn_anim_clk >= (zc_max(wpnsbuf[spr_spawn].speed,1) * zc_max(wpnsbuf[spr_spawn].frames,1)))
					{
						spr_spawn_anim_clk=-1;
						clk=-1;
					}
				}
				t += spr_spawn_anim_frm;
			}
			else if(BSZ)
			{
				if(clk>=-10) ++t;
                
				if(clk>=-5) ++t;
			}
			else
			{
				if(clk>=-12) ++t;
                
				if(clk>=-6) ++t;
			}
            
			overtile16(dest,t,sx,sy,cs2,0);
		}
		else //extend == 3?
		{
			sprite w((zfix)sx,(zfix)sy,wpnsbuf[extend].tile,wpnsbuf[extend].csets&15,0,0,0);
			w.xofs = xofs;
			w.yofs = yofs;
			w.zofs = zofs;
			w.txsz = txsz;
			w.tysz = tysz;
			w.extend = 3;
            
			if ( w.scripttile <= -1 ) 
			{
				if(BSZ)
				{
					if(clk>=-10)
					{
						if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
							w.tile+=txsz;
						else
							w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
					}
				
					if(clk>=-5)
					{
						if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
							w.tile+=txsz;
						else
							w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
					}
				}
				else
				{
					if(clk>=-12)
					{
						if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
							w.tile+=txsz;
						else
							w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
					}
			
					if(clk>=-6)
					{
						if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
							w.tile+=txsz;
						else
							w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
					}
				}
			}
            
			w.draw(dest);
		}
	}
    
	if(show_hitboxes)
		draw_hitbox();
	
	yofs = tyoffs;

	sprite_clear_flicker();
}

void sprite::draw_hitbox()
{
	if(hide_hitbox) return;
#ifdef IS_PLAYER
	start_info_bmp();
	al_draw_rectangle(x+hxofs,y+playing_field_offset+hyofs-(z+zofs)-fakez,x+hxofs+hit_width,(y+playing_field_offset+hyofs+hit_height-(z+zofs)-fakez),hitboxColor(info_opacity),1);
	end_info_bmp();
#endif
}


//Z1 bosses draw tiles from guys.cpp, direct to the 'dest'
void sprite::drawzcboss(BITMAP* dest)
{
	if(!show_sprites)
	{
		return;
	}
    
	int32_t sx = real_x(x+xofs);
	// Pain. https://discord.com/channels/876899628556091432/1193381178716196864
	// syz was simply inlined on the declaration of sy, but somehow MSVC RelWithDebInfo win32 breaks this so badly that
	// the hero y position is always negative, making the hero invisible. The compiler doesn't do the bad thing when an
	// intermediate variable is introduced.
	int32_t syz = real_z(z+zofs);
	int32_t sy = real_y(y+yofs) - syz;
	sy -= fake_z(fakez);
    
	if(id<0)
		return;
        
	if (!(FFCore.getQuestHeaderInfo(0) < 0x255 || (FFCore.getQuestHeaderInfo(0) == 0x255 && FFCore.getQuestHeaderInfo(2) < 42) || get_qr(qr_OLDSPRITEDRAWS) || (drawflags & sprdrawflagALWAYSOLDDRAWS)))
	{
		sprite::draw(dest);
		return; //don't run the rest, use the old code
	}
	if (sprite_flicker_color || sprite_mask_color)
	{
		sprite_clear_flicker();
		return;
	}

	int32_t e = extend>=3 ? 3 : extend;
    
	if(clk>=0)
	{
		BITMAP *temp;
		switch(e)
		{
		case 1:
			temp = create_bitmap_ex(8,16,32);
			blit(dest, temp, sx, sy-16, 0, 0, 16, 32);
            
			if(drawstyle==0 || drawstyle==3)
			{
				int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
				if ( scripttile > -1 )
				{
					overtile16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),0,0,cs,tmpflip);
					overtile16(temp,vbound(scripttile,0,NEWMAXTILES),0,16,cs,tmpflip);
				}
				else
				{
					overtile16(temp,tile-TILES_PER_ROW,0,0,cs,tmpflip);
					overtile16(temp,tile,0,16,cs,tmpflip);
				}
			}
            
			if(drawstyle==1)
			{
				int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
				if ( scripttile > -1 )
				{
					overtiletranslucent16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),0,16,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile,0,NEWMAXTILES),0,16,cs,tmpflip,128);
				}
				else
				{
					overtiletranslucent16(temp,tile-TILES_PER_ROW,0,0,cs,tmpflip,128);
					overtiletranslucent16(temp,tile,0,16,cs,tmpflip,128);
				}
			}
            
			if(drawstyle==2)
			{
				int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
				if ( scripttile > -1 )
				{
					overtilecloaked16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),0,16,tmpflip);
					overtilecloaked16(temp,vbound(scripttile,0,NEWMAXTILES),0,16,tmpflip);
				}
				else
				{
					overtilecloaked16(temp,tile-TILES_PER_ROW,0,0,tmpflip);
					overtilecloaked16(temp,tile,0,16,tmpflip);
				}
			}
            
			masked_blit(temp, dest, 0, 0, sx, sy-16, 16, 32);
			destroy_bitmap(temp);
			break;
            
		case 2:
			temp = create_bitmap_ex(8,48,32);
			blit(dest, temp, sx-16, sy-16, 0, 0, 48, 32);
            
			if(drawstyle==0 || drawstyle==3)
			{
				if ( scripttile > -1 )
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtile16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),16,0,cs,tmpflip);
					overtile16(temp,vbound(scripttile-TILES_PER_ROW-(tmpflip?-1:1),0,NEWMAXTILES),0,0,cs,tmpflip);
					overtile16(temp,vbound(scripttile-TILES_PER_ROW+(tmpflip?-1:1),0,NEWMAXTILES),32,0,cs,tmpflip);
					overtile16(temp,vbound(scripttile,0,NEWMAXTILES),16,16,cs,tmpflip);
					overtile16(temp,vbound(scripttile-(tmpflip?-1:1),0,NEWMAXTILES),0,16,cs,tmpflip);
					overtile16(temp,vbound(scripttile+(tmpflip?-1:1),0,NEWMAXTILES),32,16,cs,tmpflip);
				}
				else
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtile16(temp,tile-TILES_PER_ROW,16,0,cs,tmpflip);
					overtile16(temp,tile-TILES_PER_ROW-(tmpflip?-1:1),0,0,cs,tmpflip);
					overtile16(temp,tile-TILES_PER_ROW+(tmpflip?-1:1),32,0,cs,tmpflip);
					overtile16(temp,tile,16,16,cs,tmpflip);
					overtile16(temp,tile-(tmpflip?-1:1),0,16,cs,tmpflip);
					overtile16(temp,tile+(tmpflip?-1:1),32,16,cs,tmpflip);
				}
			}
            
			if(drawstyle==1)
			{
				if ( scripttile > -1 )
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtiletranslucent16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),16,0,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile-TILES_PER_ROW-(tmpflip?-1:1),0,NEWMAXTILES),0,0,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile-TILES_PER_ROW+(tmpflip?-1:1),0,NEWMAXTILES),32,0,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile,0,NEWMAXTILES),16,16,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile-(tmpflip?-1:1),0,NEWMAXTILES),0,16,cs,tmpflip,128);
					overtiletranslucent16(temp,vbound(scripttile+(tmpflip?-1:1),0,NEWMAXTILES),32,16,cs,tmpflip,128);
				}
				else
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtiletranslucent16(temp,tile-TILES_PER_ROW,16,0,cs,tmpflip,128);
					overtiletranslucent16(temp,tile-TILES_PER_ROW-(tmpflip?-1:1),0,0,cs,tmpflip,128);
					overtiletranslucent16(temp,tile-TILES_PER_ROW+(tmpflip?-1:1),32,0,cs,tmpflip,128);
					overtiletranslucent16(temp,tile,16,16,cs,tmpflip,128);
					overtiletranslucent16(temp,tile-(tmpflip?-1:1),0,16,cs,tmpflip,128);
					overtiletranslucent16(temp,tile+(tmpflip?-1:1),32,16,cs,tmpflip,128);
				}
			}
            
			if(drawstyle==2)
			{
				if ( scripttile > -1 )
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtilecloaked16(temp,vbound(scripttile-TILES_PER_ROW,0,NEWMAXTILES),16,0,tmpflip);
					overtilecloaked16(temp,vbound(scripttile-TILES_PER_ROW-(tmpflip?-1:1),0,NEWMAXTILES),0,0,tmpflip);
					overtilecloaked16(temp,vbound(scripttile-TILES_PER_ROW+(tmpflip?-1:1),0,NEWMAXTILES),32,0,tmpflip);
					overtilecloaked16(temp,vbound(scripttile,0,NEWMAXTILES),16,16,tmpflip);
					overtilecloaked16(temp,vbound(scripttile-(tmpflip?-1:1),0,NEWMAXTILES),0,16,tmpflip);
					overtilecloaked16(temp,vbound(scripttile+(tmpflip?-1:1),0,NEWMAXTILES),32,16,tmpflip);
				}
				else
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtilecloaked16(temp,tile-TILES_PER_ROW,16,0,tmpflip);
					overtilecloaked16(temp,tile-TILES_PER_ROW-(tmpflip?-1:1),0,0,tmpflip);
					overtilecloaked16(temp,tile-TILES_PER_ROW+(tmpflip?-1:1),32,0,tmpflip);
					overtilecloaked16(temp,tile,16,16,tmpflip);
					overtilecloaked16(temp,tile-(tmpflip?-1:1),0,16,tmpflip);
					overtilecloaked16(temp,tile+(tmpflip?-1:1),32,16,tmpflip);
				}
			}
            
			masked_blit(temp, dest, 8, 0, sx-8, sy-16, 32, 32);
			destroy_bitmap(temp);
			break;
            
		case 3:
		{
			int32_t tileToDraw;

			switch(flip)
			{
			case 1:
				for(int32_t i=0; i<tysz; i++)
				{
					for(int32_t j=txsz-1; j>=0; j--)
					{
						tileToDraw = vbound( (scripttile > -1 ) ? ( scripttile+(i*TILES_PER_ROW)+j ) : (tile+(i*TILES_PER_ROW)+j) ,0, NEWMAXTILES );
                        
						if(tileToDraw%TILES_PER_ROW<j) // Wrapped around
							tileToDraw+=TILES_PER_ROW*(tysz-1);
			
						tileToDraw = vbound(tileToDraw, 0, NEWMAXTILES);
						int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
						if(drawstyle==0 || drawstyle==3) overtile16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+i*16,cs,tmpflip);
						else if(drawstyle==1) overtiletranslucent16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+i*16,cs,tmpflip,128);
						else if(drawstyle==2) overtilecloaked16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+i*16,tmpflip);
					}
				}
                
				break;
                
			case 2:
				for(int32_t i=tysz-1; i>=0; i--)
				{
					for(int32_t j=0; j<txsz; j++)
					{
						tileToDraw = vbound( (scripttile > -1 ) ? ( scripttile+(i*TILES_PER_ROW)+j ) : (tile+(i*TILES_PER_ROW)+j) ,0, NEWMAXTILES );
                        
						if(tileToDraw%TILES_PER_ROW<j)
							tileToDraw+=TILES_PER_ROW*(tysz-1);
			
						tileToDraw = vbound(tileToDraw, 0, NEWMAXTILES);
						int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
						if(drawstyle==0 || drawstyle==3) overtile16(dest,tileToDraw,sx+j*16,sy+(tysz-i-1)*16,cs,tmpflip);
						else if(drawstyle==1) overtiletranslucent16(dest,tileToDraw,sx+j*16,sy+(tysz-i-1)*16,cs,tmpflip,128);
						else if(drawstyle==2) overtilecloaked16(dest,tileToDraw,sx+j*16,sy+(tysz-i-1)*16,tmpflip);
					}
				}
                
				break;
                
			case 3:
				for(int32_t i=tysz-1; i>=0; i--)
				{
					for(int32_t j=txsz-1; j>=0; j--)
					{
						tileToDraw = vbound( (scripttile > -1 ) ? ( scripttile+(i*TILES_PER_ROW)+j ) : (tile+(i*TILES_PER_ROW)+j) ,0, NEWMAXTILES );
                        
						if(tileToDraw%TILES_PER_ROW<j)
							tileToDraw+=TILES_PER_ROW*(tysz-1);
			
						tileToDraw = vbound(tileToDraw, 0, NEWMAXTILES);
						int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
						if(drawstyle==0 || drawstyle==3) overtile16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+(tysz-i-1)*16,cs,tmpflip);
						else if(drawstyle==1) overtiletranslucent16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+(tysz-i-1)*16,cs,tmpflip,128);
						else if(drawstyle==2) overtilecloaked16(dest,tileToDraw,sx+(txsz-j-1)*16,sy+(tysz-i-1)*16,tmpflip);
					}
				}
                
				break;
                
			case 0:
				for(int32_t i=0; i<tysz; i++)
				{
					for(int32_t j=0; j<txsz; j++)
					{
						tileToDraw = vbound( (scripttile > -1 ) ? ( scripttile+(i*TILES_PER_ROW)+j ) : (tile+(i*TILES_PER_ROW)+j) ,0, NEWMAXTILES );
                        
						if(tileToDraw%TILES_PER_ROW<j)
							tileToDraw+=TILES_PER_ROW*(tysz-1);
			
						tileToDraw = vbound(tileToDraw, 0, NEWMAXTILES);
						int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
						if(drawstyle==0 || drawstyle==3) overtile16(dest,tileToDraw,sx+j*16,sy+i*16,cs,tmpflip);
						else if(drawstyle==1) overtiletranslucent16(dest,tileToDraw,sx+j*16,sy+i*16,cs,tmpflip,128);
						else if(drawstyle==2) overtilecloaked16(dest,tileToDraw,sx+j*16,sy+i*16,tmpflip);
					}
				}
                
				break;
			}
            
			case 0:
			default:
				if(drawstyle==0 || drawstyle==3)
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtile16(dest,vbound(((scripttile > -1) ? scripttile : tile), 0, NEWMAXTILES),sx,sy,cs,tmpflip);
				}
				else if(drawstyle==1)
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtiletranslucent16(dest,vbound(((scripttile > -1) ? scripttile : tile), 0, NEWMAXTILES),sx,sy,cs,tmpflip,128);
				}
				else if(drawstyle==2)
				{
					int32_t tmpflip = ( (scriptflip > -1) ? scriptflip : flip );
					overtilecloaked16(dest,vbound(((scripttile > -1) ? scripttile : tile), 0, NEWMAXTILES),sx,sy,tmpflip);
				}
				break;
			}
			break;
		}
	}
	else
	{
		if(e!=3)
		{
			int32_t t  = wpnsbuf[spr_spawn].tile;
			int32_t cs2 = wpnsbuf[spr_spawn].csets&15;
            
			if(!get_qr(qr_HARDCODED_ENEMY_ANIMS))
			{
				if(clk < -2)
				{
					spr_spawn_anim_clk=0;
					clk=-1;
				}
				if(clk==-1 && spr_spawn_anim_clk>-1)
				{
					--clk;
					spr_spawn_anim_frm=(spr_spawn_anim_clk/zc_max(wpnsbuf[spr_spawn].speed,1));
					if(++spr_spawn_anim_clk >= (zc_max(wpnsbuf[spr_spawn].speed,1) * zc_max(wpnsbuf[spr_spawn].frames,1)))
					{
						spr_spawn_anim_clk=-1;
						clk=-1;
					}
				}
				t += spr_spawn_anim_frm;
			}
			else if(BSZ)
			{
				if(clk>=-10) ++t;
                
				if(clk>=-5) ++t;
			}
			else
			{
				if(clk>=-12) ++t;
                
				if(clk>=-6) ++t;
			}
            
			overtile16(dest,t,sx,sy,cs2,0);
		}
		else
		{
			sprite w((zfix)sx,(zfix)sy,wpnsbuf[extend].tile,wpnsbuf[extend].csets&15,0,0,0);
			w.xofs = xofs;
			w.yofs = yofs;
			w.zofs = zofs;
			w.txsz = txsz;
			w.tysz = tysz;
			w.extend = 3;
            
			if(BSZ)
			{
				if(clk>=-10)
				{
					if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
						w.tile+=txsz;
					else
						w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
				}
                
				if(clk>=-5)
				{
					if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
						w.tile+=txsz;
					else
						w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
				}
			}
			else
			{
				if(clk>=-12)
				{
					if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
						w.tile+=txsz;
					else
						w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
				}
                
				if(clk>=-6)
				{
					if(tile/TILES_PER_ROW==(tile+txsz)/TILES_PER_ROW)
						w.tile+=txsz;
					else
						w.tile+=txsz+(tysz-1)*TILES_PER_ROW;
				}
			}
            
			w.draw(dest);
		}
	}
    
	if(show_hitboxes)
		draw_hitbox();
}

void sprite::draw8(BITMAP* dest)
{
    int32_t sx = real_x(x+xofs);
    int32_t sy = real_y(y+yofs)-real_z(z+zofs);
	sy -= fake_z(fakez);
    
    if(id<0)
        return;
        
    if(clk>=0)
    {
        switch(drawstyle)
        {
        case 0:                                               //normal
            overtile8(dest,((scripttile > -1) ? scripttile : tile),sx,sy,cs,((scriptflip > -1) ? scriptflip : flip));
            break;
            
        case 1:                                               //phantom
            overtiletranslucent8(dest,((scripttile > -1) ? scripttile : tile),sx,sy,cs,((scriptflip > -1) ? scriptflip : flip),128);
            break;
        }
    }
}

void sprite::drawcloaked(BITMAP* dest)
{
    int32_t sx = real_x(x+xofs);
    int32_t sy = real_y(y+yofs)-real_z(z+zofs);
    sy -= fake_z(fakez);
    
    if(id<0)
        return;
        
    if(clk>=0)
    {
        overtilecloaked16(dest,((scripttile > -1) ? scripttile : tile),sx,sy,((scriptflip > -1) ? scriptflip : flip));
    }
    else
    {
        int32_t t  = wpnsbuf[spr_spawn].tile;
        int32_t cs2 = wpnsbuf[spr_spawn].csets&15;
        
		if(!get_qr(qr_HARDCODED_ENEMY_ANIMS))
		{
			if(clk < -2)
			{
				spr_spawn_anim_clk=0;
				clk=-1;
			}
			if(clk==-1 && spr_spawn_anim_clk>-1)
			{
				--clk;
				spr_spawn_anim_frm=(spr_spawn_anim_clk/zc_max(wpnsbuf[spr_spawn].speed,1));
				if(++spr_spawn_anim_clk >= (zc_max(wpnsbuf[spr_spawn].speed,1) * zc_max(wpnsbuf[spr_spawn].frames,1)))
				{
					spr_spawn_anim_clk=-1;
					clk=-1;
				}
			}
			t += spr_spawn_anim_frm;
		}
        else if(BSZ)
        {
            if(clk>=-10) ++t;
            
            if(clk>=-5) ++t;
        }
        else
        {
            if(clk>=-12) ++t;
            
            if(clk>=-6) ++t;
        }
        
        overtile16(dest,t,x,sy,cs2,0);
    }
    
    if(get_debug() && key[KEY_O])
        rectfill(dest,x+hxofs,sy+hyofs-fakez,x+hxofs+hit_width-1,sy+hyofs+hit_height-fakez-1,vc(id));
}

void sprite::drawshadow(BITMAP* dest,bool translucent)
{
	if(extend == 4 || shadowtile==0 || id<0)
	{
		return;
	}
	
	int32_t sx = real_x(x+xofs+shadowxofs)+(txsz-1)*8;
	int32_t sy = real_y(y+yofs+shadowyofs)+(tysz-1)*16;
	//int32_t sy1 = sx-56; //subscreen offset
	//if ( ispitfall(x+xofs, y+yofs+16) || ispitfall(x+xofs+8, y+yofs+16) || ispitfall(x+xofs+15, y+yofs+16)  ) return;
	//sWTF, why is this offset by half the screen. Can't do this right now. Sanity. -Z
	int32_t shadowcs = wpnsbuf[spr_shadow].csets & 0xFFFF;
	int32_t shadowflip = wpnsbuf[spr_shadow].misc & 0xFF;
	//if ( !ispitfall(sx,sy+4) && !ispitfall(sx+8,sy+4) )
	{
		if(clk>=0)
		{
			if(translucent)
			{
				overtiletranslucent16(dest,shadowtile,sx,sy,shadowcs,shadowflip,128);
			}
			else
			{
				overtile16(dest,shadowtile,sx,sy,shadowcs,shadowflip);
			}
		}
	}
}

int32_t sprite::run_script(int32_t mode)
{
	return RUNSCRIPT_OK; //Default implementation; override in subclasses
}

ALLEGRO_COLOR sprite::hitboxColor(byte opacity) const
{
	return al_map_rgba(255,0,255,opacity);
}

sprite_list::sprite_list() : count(0), active_iterator(0), max_sprites(255),
	lastUIDRequested(0), lastSpriteRequested(0) {}
void sprite_list::clear(bool force)
{
	int32_t ind = 0;
    while(count>ind)
	{
		if(!del(ind, force))
			++ind;
	}
}

sprite *sprite_list::spr(int32_t index)
{
    if(index<0 || index>=count)
        return NULL;
        
    return sprites[index];
}

int32_t sprite_list::find(sprite *spr)
{
	for(int32_t ind = 0; ind < count; ++ind)
	{
		if(spr == sprites[ind])
			return ind;
	}
	return -1;
}

bool sprite_list::swap(int32_t a,int32_t b)
{
    if(a<0 || a>=count || b<0 || b>=count)
        return false;
        
    sprite *c = sprites[a];
    sprites[a] = sprites[b];
    sprites[b] = c;
    containedUIDs[sprites[a]->getUID()] = a;
    containedUIDs[sprites[b]->getUID()] = b;
// checkConsistency();
    return true;
}

bool sprite_list::add(sprite *s)
{
    if(count>=max_sprites)
    {
        delete s;
        return false;
    }
    
    s->registerUID();
    containedUIDs[s->getUID()] = count;
    sprites[count++]=s;
    //checkConsistency();
    return true;
}

bool sprite_list::remove(sprite *s)
// removes pointer from list but doesn't delete it
{
	if(s->ignore_delete) return false;
    if(s==lastSpriteRequested)
    {
        lastUIDRequested=0;
        lastSpriteRequested=0;
    }
    
    map<int32_t, int32_t>::iterator it = containedUIDs.find(s->getUID());
    
    if(it != containedUIDs.end())
        containedUIDs.erase(it);
        
    int32_t j=0;
    
    for(; j<count; j++)
        if(sprites[j] == s)
            goto gotit;
            
    return false;
    
gotit:

    for(int32_t i=j; i<count-1; i++)
    {
        sprites[i]=sprites[i+1];
        containedUIDs[sprites[i]->getUID()] = i;
    }
    
    --count;
    if(j<=active_iterator) --active_iterator;
    return true;
}

zfix sprite_list::getX(int32_t j)
{
    if((j>=count)||(j<0))
    {
        return (zfix)1000000;
    }
    
    return sprites[j]->x;
}

zfix sprite_list::getY(int32_t j)
{
    if((j>=count)||(j<0))
    {
        return (zfix)1000000;
    }
    
    return sprites[j]->y;
}

int32_t sprite_list::getID(int32_t j)
{
    if((j>=count)||(j<0))
    {
        return -1;
    }
    
    return sprites[j]->id;
}

int32_t sprite_list::getMisc(int32_t j)
{
    if((j>=count)||(j<0))
    {
        return -1;
    }
    
    return sprites[j]->misc;
}

bool sprite_list::del(int32_t j, bool force, bool may_defer)
{
	if(j<0||j>=count)
		return false;
	
	if(!force && sprites[j]->ignore_delete) return false;

	// If this sprite is currently running its `animate` function, just mark it for deletion
	// to avoid crashing. This allows the sprite to finish its logic even after being told to
	// go away.
	if(may_defer && active_iterator == j)
	{
		delete_active_iterator = true;
		// Remove from list so fewer things interact with it.
		// TODO: better to remove by index.
		remove(sprites[j]);
		return false;
	}
	
	map<int32_t, int32_t>::iterator it = containedUIDs.find(sprites[j]->getUID());
	
	if(it != containedUIDs.end())
		containedUIDs.erase(it);
	
	if(sprites[j]==lastSpriteRequested)
	{
		lastUIDRequested=0;
		lastSpriteRequested=0;
	}
	
	delete sprites[j];
	
	for(int32_t i=j; i<count-1; i++)
	{
		sprites[i]=sprites[i+1];
		containedUIDs[sprites[i]->getUID()] = i;
	}
	
	--count;
	if(j<=active_iterator) --active_iterator;
	//checkConsistency();
	return true;
}

void sprite_list::draw(BITMAP* dest,bool lowfirst)
{
	if(lowfirst)
		for(int32_t i=0; i<count; i++)
		{
			sprites[i]->draw(dest);
		}
	else
		for(int32_t i=count-1; i>=0; i--)
		{
			sprites[i]->draw(dest);
		}
}

void sprite_list::drawshadow(BITMAP* dest,bool translucent, bool lowfirst)
{
	if(lowfirst)
		for(int32_t i=0; i<count; i++)
		{
			sprites[i]->drawshadow(dest,translucent);
		}
	else
		for(int32_t i=count-1; i>=0; i--)
		{
			sprites[i]->drawshadow(dest,translucent);
		}
}

void sprite_list::draw2(BITMAP* dest,bool lowfirst)
{
	if(lowfirst)
		for(int32_t i=0; i<count; i++)
		{
			sprites[i]->draw2(dest);
		}
	else
		for(int32_t i=count-1; i>=0; i--)
		{
			sprites[i]->draw2(dest);
		}
}

void sprite_list::drawcloaked2(BITMAP* dest,bool lowfirst)
{
	if(lowfirst)
		for(int32_t i=0; i<count; i++)
		{
			sprites[i]->drawcloaked2(dest);
		}
	else
		for(int32_t i=count-1; i>=0; i--)
		{
			sprites[i]->drawcloaked2(dest);
		}
}

extern char *guy_string[];

void sprite_list::animate()
{
	active_iterator = 0;
	
	while(active_iterator<count)
	{
		// TODO: add a higher debug mode for this. For now, just comment out as-needed for debugging.
		// if (replay_is_active() && replay_is_debug() && dynamic_cast<enemy*>(sprites[active_iterator]) != nullptr)
		// {
		// 	enemy* as_enemy = dynamic_cast<enemy*>(sprites[active_iterator]);
		// 	replay_step_comment(fmt::format("enemy {} id: {} x: {} y: {} clk: {} {} {}",
		// 		guy_string[as_enemy->id&0xFFF], as_enemy->id, as_enemy->x.getInt(), as_enemy->y.getInt(),
		// 		as_enemy->clk, as_enemy->clk2, as_enemy->clk3)
		// 	);
		// }

		if(!(freeze_guys && sprites[active_iterator]->canfreeze))
		{
			setCurObject(sprites[active_iterator]);
			auto tmp_iter = active_iterator;
			sprite* cur_sprite = sprites[active_iterator];
			if (cur_sprite->animate(active_iterator) || delete_active_iterator)
			{
#ifdef IS_PLAYER
				if (replay_is_active() && dynamic_cast<enemy*>(cur_sprite) != nullptr)
				{
					enemy* as_enemy = dynamic_cast<enemy*>(cur_sprite);
					replay_step_comment(fmt::format("enemy died {}", guy_string[as_enemy->id&0xFFF]));
				}
#endif
				if (delete_active_iterator)
				{
					delete cur_sprite;
					delete_active_iterator = false;
				}
				else
				{
					del(active_iterator, false, false);
				}
			}
			else if(tmp_iter == active_iterator)
			{
				sprites[active_iterator]->post_animate();
			}
			setCurObject(NULL);
		}
		
		++active_iterator;
	}
	active_iterator = -1;
}

void sprite_list::solid_push(solid_object* pusher)
{
    for(int32_t i=0; i<count; i++)
        sprites[i]->solid_push(pusher);
}

void sprite_list::run_script(int32_t mode)
{
	active_iterator = 0;
	
	while(active_iterator<count)
	{
		sprite* cur_sprite = sprites[active_iterator];
		if(!(freeze_guys && cur_sprite->canfreeze))
		{
			cur_sprite->run_script(mode);
			if (delete_active_iterator)
			{
				delete cur_sprite;
				delete_active_iterator = false;
			}
		}
		
		++active_iterator;
	}
	active_iterator = -1;
}

void sprite_list::check_conveyor()
{
    int32_t i=0;
    
    while(i<count)
    {
        sprites[i]->check_conveyor();
        ++i;
    }
}

int32_t sprite_list::Count()
{
    return count;
}

bool sprite_list::has_space(int32_t space)
{
	return (count+space) <= max_sprites;
}

int32_t sprite_list::hit(sprite *s)
{
    for(int32_t i=0; i<count; i++)
        if(sprites[i]->hit(s))
            return i;
            
    return -1;
}

int32_t sprite_list::hit(int32_t x,int32_t y,int32_t z, int32_t xsize, int32_t ysize, int32_t zsize)
{
    for(int32_t i=0; i<count; i++)
        if(sprites[i]->hit(x,y,z,xsize,ysize,zsize))
            return i;
            
    return -1;
}
int32_t sprite_list::hit(int32_t x,int32_t y,int32_t xsize, int32_t ysize)
{
    for(int32_t i=0; i<count; i++)
        if(sprites[i]->hit(x,y,xsize,ysize))
            return i;
            
    return -1;
}

// returns the number of sprites with matching id
int32_t sprite_list::idCount(int32_t id, int32_t mask)
{
    int32_t c=0;
    
    for(int32_t i=0; i<count; i++)
    {
        if(((sprites[i]->id)&mask) == (id&mask))
        {
            ++c;
        }
    }
    
    return c;
}

int32_t sprite_list::idCount(std::set<int32_t> const& ids)
{
    int32_t c=0;
    
    for(int32_t i=0; i<count; i++)
    {
        if(ids.contains(sprites[i]->id))
            ++c;
    }
    
    return c;
}

// returns index of first sprite with matching id, -1 if none found
int32_t sprite_list::idFirst(int32_t id, int32_t mask)
{
    for(int32_t i=0; i<count; i++)
    {
        if(((sprites[i]->id)&mask) == (id&mask))
        {
            return i;
        }
    }
    
    return -1;
}

// returns index of nth sprite with matching id, -1 if none found
int32_t sprite_list::idNth(int32_t id, int32_t n, int32_t mask)
{
    for(int32_t i=0; i<count; i++)
    {
        if(((sprites[i]->id)&mask) == (id&mask))
        {
	    if (n > 1) --n;
            else return i;
        }
    }
    
    return -1;
}

// returns index of last sprite with matching id, -1 if none found
int32_t sprite_list::idLast(int32_t id, int32_t mask)
{
    for(int32_t i=count-1; i>=0; i--)
    {
        if(((sprites[i]->id)&mask) == (id&mask))
        {
            return i;
        }
    }
    
    return -1;
}

// returns the number of sprites with matching id
int32_t sprite_list::idCount(int32_t id)
{
    return idCount(id,0xFFFF);
}

// returns index of first sprite with matching id, -1 if none found
int32_t sprite_list::idFirst(int32_t id)
{
    return idFirst(id,0xFFFF);
}

// returns index of first sprite with matching id, -1 if none found
int32_t sprite_list::idNth(int32_t id, int32_t n)
{
    return idNth(id,n,0xFFFF);
}

// returns index of last sprite with matching id, -1 if none found
int32_t sprite_list::idLast(int32_t id)
{
    return idLast(id,0xFFFF);
}

sprite * sprite_list::getByUID(int32_t uid)
{
    if(uid==lastUIDRequested)
        return lastSpriteRequested;
    
    map<int32_t, int32_t>::iterator it = containedUIDs.find(uid);
    
    if(it != containedUIDs.end())
    {
        // Only update cache if requested sprite was found
        lastUIDRequested=uid;
        lastSpriteRequested=spr(it->second);
        return lastSpriteRequested;
    }
        
    return NULL;
}

void sprite_list::checkConsistency()
{
    assert((int32_t)containedUIDs.size() == count);
    assert(lastUIDRequested==0 || containedUIDs.find(lastUIDRequested)!=containedUIDs.end());
    
    for(int32_t i=0; i<count; i++)
        assert(sprites[i] == getByUID(sprites[i]->getUID()));
}

void sprite_list::forEach(std::function<bool(sprite&)> proc)
{
	for(int q = 0; q < count; ++q)
	{
		if(proc(*sprites[q]))
			return;
	}
}

void sprite::explode(int32_t type)
{
	al_trace("Trying to explode enemy tile: %d\n",tile);
	
	/*
	tiledata *temptilebuf = NULL;
	memset(temptilebuf, 0, sizeof(temptilebuf));
	static int32_t tempx, tempy;
	static byte herotilebuf[256];
	int32_t ltile=0;
	int32_t lflip=0;
	unpack_tile(temptilebuf, tile, flip, true);
	//unpack_tile(temptilebuf, tile, flip, true);
	//unpack_tile(temptilebuf, o_tile, 0, true);
	memcpy(herotilebuf, temptilebuf, 256);
	tempx=x;
	tempy=y;
	*/
	
	byte spritetilebuf[256];
	int32_t ltile=0;
	int32_t lflip=0;
	unpack_tile(newtilebuf, tile, flip, true);
	memcpy(spritetilebuf, unpackbuf, 256);	
	
	for(int32_t i=0; i<16; ++i)
	{
                for(int32_t j=0; j<16; ++j)
                {
                    if(spritetilebuf[i*16+j])
                    {
                        if(type==0)  // Twilight
                        {
                            particles.add(new pTwilight(x+j, y-z-fakez+i, 5, 0, 0, (zc_oldrand()%8)+i*4));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=3;
			    p->cset=cs;
			    p->color= zc_oldrand()%4+1;
                        }
                        
			else if(type ==1)  // Sands of Hours
                        {
                            particles.add(new pTwilight(x+j, y-z-fakez+i, 5, 1, 2, (zc_oldrand()%16)+i*2));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=4;
                            
                            if(zc_oldrand()%10 < 2)
                            {
                                p->color= zc_oldrand()%4+1;
                                p->cset=cs;
                            }
                        }
                        else //explode
                        {
                            particles.add(new pDivineEscapeDust(x+j, y-z-fakez+i, 5, 6, spritetilebuf[i*16+j], zc_oldrand()%96));
                            
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->angular=true;
                            p->angle=zc_oldrand();
                            p->step=(((double)j)/8);
                            p->yofs=yofs;
			    p->color= zc_oldrand()%4+1;
			    p->cset=cs;
                        }
			
                    }
                }
	}
	
}

bool sprite::getCanFlicker()
{
	return can_flicker;
}
void sprite::setCanFlicker(bool v)
{
	can_flicker = v;
}


/*
void sprite::explode(int32_t type)
{
	static int32_t tempx, tempy;
	static byte herotilebuf[256];
	int32_t ltile=0;
	int32_t lflip=0;
	unpack_tile(newtilebuf, tile, flip, true);
	memcpy(herotilebuf, unpackbuf, 256);
	tempx=x;
	tempy=y;
	for(int32_t i=0; i<16; ++i)
	{
                for(int32_t j=0; j<16; ++j)
                {
                    if(herotilebuf[i*16+j])
                    {
                        if(type==0)  // Twilight
                        {
                            particles.add(new pTwilight(x+j, y-z+i, 5, 0, 0, (zc_oldrand()%8)+i*4));
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->step=3;
                        }
                        else if(type ==1)  // Sands of Hours
                        {
                            particles.add(new pTwilight(x+j, y-z()+i, 5, 1, 2, (zc_oldrand()%16)+i*2));
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
                            particles.add(new pDivineEscapeDust(x+j, y-z+i, 5, 6, herotilebuf[i*16+j], zc_oldrand()%96));
                            
                            int32_t k=particles.Count()-1;
                            particle *p = (particles.at(k));
                            p->angular=true;
                            p->angle=zc_oldrand();
                            p->step=(((double)j)/8);
                            p->yofs=yofs;
                        }
                    }
                }
	}
}
*/

//Moving Block 

movingblock::movingblock() : sprite(), blockLayer(0), step(0.5)
{
    id=1;
}

void movingblock::draw(BITMAP *dest)
{
	if(fallclk)
	{
		int32_t old_cs = cs;
		int32_t old_tile = tile;
		
		int sprid = QMisc.sprites[sprFALL];
		newcombo const& cmb = combobuf[bcombo];
		if(cmb.spr_falling)
			sprid = cmb.spr_falling;
		if(unsigned(sprid) < MAXWPNS)
		{
			wpndata& spr = wpnsbuf[sprid];
			cs = spr.csets & 0xF;
			int32_t fr = spr.frames ? spr.frames : 1;
			int32_t spd = spr.speed ? spr.speed : 1;
			int32_t animclk = (PITFALL_FALL_FRAMES-fallclk);
			tile = spr.tile + zc_min(animclk / spd, fr-1);
			sprite::draw(dest);
		}
		
		cs = old_cs;
		tile = old_tile;
	}
	else if(drownclk)
	{
		int32_t old_cs = cs;
		int32_t old_tile = tile;
		
		bool lava = (drownCombo && combobuf[drownCombo].usrflags&cflag1);
		int sprid = QMisc.sprites[lava ? sprLAVADROWN : sprDROWN];
		newcombo const& cmb = combobuf[bcombo];
		if(int cspr = lava ? cmb.spr_lava_drowning : cmb.spr_drowning)
			sprid = cspr;
		if(unsigned(sprid) < MAXWPNS)
		{
			wpndata &spr = wpnsbuf[sprid];
			cs = spr.csets & 0xF;
			int32_t fr = spr.frames ? spr.frames : 1;
			int32_t spd = spr.speed ? spr.speed : 1;
			int32_t animclk = (WATER_DROWN_FRAMES-drownclk);
			tile = spr.tile + zc_min(animclk / spd, fr-1);
			sprite::draw(dest);
		}
		
		cs = old_cs;
		tile = old_tile;
	}
    else if(clk)
    {
        //    sprite::draw(dest);
        overcombo(dest,real_x(x+xofs),real_y(y+yofs),bcombo ,cs);
    }
}

bool movingblock::draw(BITMAP* dest, int layer)
{
	if(!(fallclk || drownclk || clk)) return false;
	if(layer < 0 || blockLayer == layer)
	{
		draw(dest);
		return true;
	}
	return false;
}

//Portal
portal::portal()
{
	id = 0; //negative id doesn't draw!
}
portal::portal(int32_t dm, int32_t scr, int32_t gfx, int32_t sfx, int32_t spr)
	: destdmap(dm), destscr(scr), weffect(gfx), wsfx(sfx)
{
	LOADGFX(spr);
	id = 0; //negative id doesn't draw!
}
void portal::LOADGFX(int32_t spr)
{
	wpndata const& portalsprite = wpnsbuf[spr];
	o_tile = portalsprite.tile;
	aspd = portalsprite.speed ? portalsprite.speed : 1;
	frames = portalsprite.frames ? portalsprite.frames : 1;
	aframe = 0;
	aclk = 0;
	cs = portalsprite.csets & 0xF;
	tile = o_tile;
}

bool portal::animate(int32_t)
{
	if(++aclk >= aspd)
	{
		aclk = 0;
		if(++aframe >= frames)
		{
			aframe = 0;
		}
	}
	tile = o_tile + aframe;
	return false;
}
void portal::clear()
{
	*this = portal();
}

//BreakableCombo

breakable::breakable(zfix X, zfix Y, zfix Z, newcombo const& cmb, int32_t cset)
	: cmb(cmb), dropitem(-1), breaktimer(0), fromdropset(-1), breaksprtype(0),
	breaksfx(0), breakspr(0)
{
	x = X; y = Y; z = Z;
	o_tile = cmb.o_tile;
	aspd = zc_max(1,cmb.speed);
	frames = zc_max(1,cmb.frames);
	aframe = 0;
	aclk = 0;
	cs = cset;
	tile = cmb.tile;
	id = 0; //negative id doesn't draw!
}

breakable::breakable(zfix X, zfix Y, zfix Z, newcombo const& cmb, int32_t cset, int32_t dropitem, int32_t fromdropset,
	byte breaksfx, int8_t breaksprtype, byte breakspr, int32_t breaktimer)
	: cmb(cmb), dropitem(dropitem), breaktimer(breaktimer), fromdropset(fromdropset), breaksprtype(breaksprtype),
	breaksfx(breaksfx), breakspr(breakspr)
{
	x = X; y = Y; z = Z;
	o_tile = cmb.o_tile;
	aspd = zc_max(1,cmb.speed);
	frames = zc_max(1,cmb.frames);
	aframe = 0;
	aclk = 0;
	cs = cset;
	tile = cmb.tile;
	id = 0; //negative id doesn't draw!
}

bool breakable::animate(int32_t)
{
#ifdef IS_PLAYER
	if(++aclk >= aspd)
	{
		aclk = 0;
		if(++aframe >= frames)
		{
			aframe = 0;
		}
	}
	tile = o_tile + aframe;
	if(breaktimer > 0)
	{
		if(!--breaktimer)
		{
			switch(breaksprtype)
			{
				case -1:
					decorations.add(new comboSprite(x, y, dCOMBOSPRITE, 0, breakspr));
					break;
				case 1: decorations.add(new dBushLeaves(x, y, dBUSHLEAVES, 0, 0)); break;
				case 2: decorations.add(new dFlowerClippings(x, y, dFLOWERCLIPPINGS, 0, 0)); break;
				case 3: decorations.add(new dGrassClippings(x, y, dGRASSCLIPPINGS, 0, 0)); break;
			}
			if(breaksfx) sfx(breaksfx,int32_t(x));
			if(dropitem > -1)
			{
				item* itm = (new item(x, y, z, dropitem, ipBIGRANGE + ipTIMER, 0));
				itm->from_dropset = fromdropset;
				items.add(itm);
			}
			return true; //sprite dead
		}
	}
#endif
	return false;
}

//x1,y1 = top of left line
//x2,y2 = bottom of left line
//x3,y3 = top of right line
//x4,y4 = bottom of right line
bool insideRotRect(double x, double y, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	int32_t x3, int32_t y3, int32_t x4, int32_t y4)
{
	if(y < y1 && y < y3) return false;
	if(y > y2 && y > y4) return false;

	double slope1 = (y2-y1)/double(x2-x1);
	double b1 = y1 - (slope1*x1);
	double slope2 = (y4-y3)/double(x4-x3);
	double b2 = y3 - (slope2*x3);
	double slope3 = (y3-y1)/double(x3-x1);
	double b3 = y3 - (slope3*x3);
	double slope4 = (y4-y2)/double(x4-x2);
	double b4 = y4 - (slope4*x4);
	double l1y = slope1*x + b1;
	double l2y = slope2*x + b2;
	double l3y = slope3*x + b3;
	double l4y = slope4*x + b4;
	if(y < l1y && y < l2y) return false;
	if(y > l1y && y > l2y) return false;
	if(y < l3y && y < l4y) return false;
	if(y > l3y && y > l4y) return false;
	return true;
}

bool lineLineColl(zfix x1, zfix y1, zfix x2, zfix y2, zfix x3, zfix y3, zfix x4, zfix y4)
{
	 float denominator = ((x2 - x1) * (y4 - y3)) - ((y2 - y1) * (x4 - x3));
	float numerator1 = ((y1 - y3) * (x4 - x3)) - ((x1 - x3) * (y4 - y3));
	float numerator2 = ((y1 - y3) * (x2 - x1)) - ((x1 - x3) * (y2 - y1));
	
	if (denominator == 0) 
	{
		if (x3 >= x1 && x3 <= x2 || x3 <= x1 && x3 >= x2 
		|| x4 >= x1 && x4 <= x2 || x4 <= x1 && x4 >= x2)
		{
			return numerator1 == 0 && numerator2 == 0;
		}
		else return false;
	}
	    
	float r = numerator1 / denominator;
	float s = numerator2 / denominator;

	return (r >= 0 && r <= 1) && (s >= 0 && s <= 1);
}

//Line box collision is just 4 lineline collisions
bool lineBoxCollision(zfix linex1, zfix liney1, zfix linex2, zfix liney2, zfix boxx, zfix boxy, zfix boxwidth, zfix boxheight)
{
	if (lineLineColl(linex1, liney1, linex2, liney2, boxx, boxy, boxx+boxwidth-0.0001_zf, boxy)) return true;
	if (lineLineColl(linex1, liney1, linex2, liney2, boxx, boxy, boxx, boxy+boxheight-0.0001_zf)) return true;
	if (lineLineColl(linex1, liney1, linex2, liney2, boxx+boxwidth-0.0001_zf, boxy, boxx+boxwidth-0.0001_zf, boxy+boxheight-0.0001_zf)) return true;
	if (lineLineColl(linex1, liney1, linex2, liney2, boxx, boxy+boxheight-0.0001_zf, boxx+boxwidth-0.0001_zf, boxy+boxheight-0.0001_zf)) return true;
	return false;
}

double comparePointLine(double x, double y, double x1, double y1, double x2, double y2)
{
	if (x1 == x2)
	{
		if (y1 < y2) return x1 - x;
		else return x - x1;
	}
	if (y1 == y2)
	{
		if (x1 < x2) return y - y1;
		else return y1 - y;
	}
    double slope = (y2-y1)/(x2-x1);
    double b = y1 - (slope*x1);
    double ly = slope*x + b;
    return y-ly;
}
