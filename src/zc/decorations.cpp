#include "base/qrs.h"
#include "sprite.h"
#include "zc/decorations.h"
#include "zc/zc_custom.h"
#include "zc/zelda.h"
#include "zc/maps.h"
#include "base/zsys.h"
#include "zc/hero.h"

decoration::decoration(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : sprite()
{
	x=X;
	y=Y;
	id=Id;
	clk=Clk;
	misc = 0;
	yofs = (get_qr(qr_OLD_DRAWOFFSET)?playing_field_offset:original_playing_field_offset);
	if(get_qr(qr_DECO_2_YOFFSET)) yofs -= 2;
	the_deco_sprite = vbound(wpnSpr,0,255);
}

decoration::~decoration() {}

int32_t dBushLeaves::ft[4][8][3];
int32_t dFlowerClippings::ft[4][8][3];
int32_t dGrassClippings::ft[3][4][4];
int32_t dHammerSmack::ft[2][4][3];

dBushLeaves::dBushLeaves(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=6;
		ft[0][0][1]=6;
		ft[0][0][2]=0;
		ft[0][1][0]=6;
		ft[0][1][1]=9;
		ft[0][1][2]=0;
		ft[0][2][0]=7;
		ft[0][2][1]=11;
		ft[0][2][2]=1;
		ft[0][3][0]=7;
		ft[0][3][1]=13;
		ft[0][3][2]=0;
		ft[0][4][0]=9;
		ft[0][4][1]=15;
		ft[0][4][2]=0;
		ft[0][5][0]=10;
		ft[0][5][1]=16;
		ft[0][5][2]=0;
		ft[0][6][0]=11;
		ft[0][6][1]=18;
		ft[0][6][2]=0;
		ft[0][7][0]=4;
		ft[0][7][1]=19;
		ft[0][7][2]=1;
		
		ft[1][0][0]=-4;
		ft[1][0][1]=3;
		ft[1][0][2]=0;
		ft[1][1][0]=-1;
		ft[1][1][1]=4;
		ft[1][1][2]=0;
		ft[1][2][0]=0;
		ft[1][2][1]=5;
		ft[1][2][2]=0;
		ft[1][3][0]=1;
		ft[1][3][1]=5;
		ft[1][3][2]=1;
		ft[1][4][0]=-2;
		ft[1][4][1]=5;
		ft[1][4][2]=1;
		ft[1][5][0]=-6;
		ft[1][5][1]=5;
		ft[1][5][2]=0;
		ft[1][6][0]=-7;
		ft[1][6][1]=4;
		ft[1][6][2]=0;
		ft[1][7][0]=-9;
		ft[1][7][1]=2;
		ft[1][7][2]=1;
		
		ft[2][0][0]=10;
		ft[2][0][1]=2;
		ft[2][0][2]=1;
		ft[2][1][0]=7;
		ft[2][1][1]=3;
		ft[2][1][2]=1;
		ft[2][2][0]=4;
		ft[2][2][1]=5;
		ft[2][2][2]=1;
		ft[2][3][0]=5;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][4][0]=8;
		ft[2][4][1]=9;
		ft[2][4][2]=0;
		ft[2][5][0]=9;
		ft[2][5][1]=9;
		ft[2][5][2]=0;
		ft[2][6][0]=12;
		ft[2][6][1]=9;
		ft[2][6][2]=1;
		ft[2][7][0]=6;
		ft[2][7][1]=16;
		ft[2][7][2]=0;
		
		ft[3][0][0]=4;
		ft[3][0][1]=-4;
		ft[3][0][2]=0;
		ft[3][1][0]=4;
		ft[3][1][1]=-6;
		ft[3][1][2]=0;
		ft[3][2][0]=2;
		ft[3][2][1]=-7;
		ft[3][2][2]=0;
		ft[3][3][0]=1;
		ft[3][3][1]=-8;
		ft[3][3][2]=3;
		ft[3][4][0]=0;
		ft[3][4][1]=-9;
		ft[3][4][2]=0;
		ft[3][5][0]=-1;
		ft[3][5][1]=-11;
		ft[3][5][2]=0;
		ft[3][6][0]=-2;
		ft[3][6][1]=-14;
		ft[3][6][2]=0;
		ft[3][7][0]=-3;
		ft[3][7][1]=-16;
		ft[3][7][2]=0;
	}
}

bool dBushLeaves::animate(int32_t)
{
	return (clk++>=24);
}

void dBushLeaves::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	int32_t t=0;
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].tile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwBushLeaves].tile;
		cs=wpnsbuf[iwBushLeaves].csets&15;
		
	}
	
	for(int32_t i=0; i<4; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=t*4+i;
		decoration::draw8(dest);
	}
}


comboSprite::comboSprite(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) :
	decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
	
	the_deco_sprite = vbound(wpnSpr,0,255);
}

bool comboSprite::animate(int32_t)
{
	int32_t dur = zc_max(1,wpnsbuf[the_deco_sprite].frames) * zc_max(1,wpnsbuf[the_deco_sprite].speed);
	return (clk++>=dur);
}

void comboSprite::realdraw(BITMAP *dest, int32_t draw_what)
{
	if(misc!=draw_what || the_deco_sprite < 0)
		return;
	
	int32_t fb=the_deco_sprite;
	int32_t t=wpnsbuf[fb].tile;
	int32_t fr=zc_max(1,wpnsbuf[fb].frames);
	int32_t spd=zc_max(1,wpnsbuf[fb].speed);
	cs=wpnsbuf[fb].csets&15;
	flip=0;
	
	tile = t+(((clk-1)/spd)%fr);
	
	decoration::draw(dest);
}

void comboSprite::draw(BITMAP *dest)
{
	realdraw(dest,0);
}

void comboSprite::draw2(BITMAP *dest)
{
	realdraw(dest,1);
}


statusSprite::statusSprite(zfix X,zfix Y, int32_t spr, int32_t tile) :
	decoration(X,Y,dSTATUSSPRITE,0)
{
	id=dSTATUSSPRITE;
	clk=0;
	
	the_deco_sprite = vbound(spr,0,255);
	plain_tile = spr ? 0 : tile;
}

bool statusSprite::animate(int32_t)
{
	if(plain_tile <= 0 && the_deco_sprite >= 0 && the_deco_sprite < MAXWPNS)
	{
		int32_t dur = zc_max(1,wpnsbuf[the_deco_sprite].frames) * zc_max(1,wpnsbuf[the_deco_sprite].speed);
		if(clk++ >= dur)
			clk = 1; //animation completed
	}
	return false;
}

void statusSprite::draw(BITMAP *dest)
{
	if(plain_tile > 0)
		tile = plain_tile;
	else if(the_deco_sprite >= 0 && the_deco_sprite < MAXWPNS)
	{
		auto const& wpn = wpsnbuf[the_deco_sprite];
		int32_t t=wpn.tile;
		int32_t fr=zc_max(1,wpn.frames);
		int32_t spd=zc_max(1,wpn.speed);
		
		tile = t+(((clk-1)/spd)%fr);
		cs=wpn.csets&15;
		flip = ((wpn.misc & WF_HFLIP) ? 1 : 0) | ((wpn.misc & WF_VFLIP) ? 2 : 0);
	}
	else return;
	
	zfix ox = x, oy = y;
	if(target)
	{
		x += target->x;
		y += target->y;
	}
	decoration::draw(dest);
	x = ox; y = oy;
}

dFlowerClippings::dFlowerClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=6;
		ft[0][0][1]=6;
		ft[0][0][2]=0;
		ft[0][1][0]=6;
		ft[0][1][1]=9;
		ft[0][1][2]=0;
		ft[0][2][0]=7;
		ft[0][2][1]=11;
		ft[0][2][2]=1;
		ft[0][3][0]=7;
		ft[0][3][1]=13;
		ft[0][3][2]=0;
		ft[0][4][0]=9;
		ft[0][4][1]=15;
		ft[0][4][2]=0;
		ft[0][5][0]=10;
		ft[0][5][1]=16;
		ft[0][5][2]=0;
		ft[0][6][0]=11;
		ft[0][6][1]=18;
		ft[0][6][2]=0;
		ft[0][7][0]=4;
		ft[0][7][1]=19;
		ft[0][7][2]=1;
		
		ft[1][0][0]=-4;
		ft[1][0][1]=3;
		ft[1][0][2]=0;
		ft[1][1][0]=-1;
		ft[1][1][1]=4;
		ft[1][1][2]=0;
		ft[1][2][0]=0;
		ft[1][2][1]=5;
		ft[1][2][2]=0;
		ft[1][3][0]=1;
		ft[1][3][1]=5;
		ft[1][3][2]=1;
		ft[1][4][0]=-2;
		ft[1][4][1]=5;
		ft[1][4][2]=1;
		ft[1][5][0]=-6;
		ft[1][5][1]=5;
		ft[1][5][2]=0;
		ft[1][6][0]=-7;
		ft[1][6][1]=4;
		ft[1][6][2]=0;
		ft[1][7][0]=-9;
		ft[1][7][1]=2;
		ft[1][7][2]=1;
		
		ft[2][0][0]=10;
		ft[2][0][1]=2;
		ft[2][0][2]=1;
		ft[2][1][0]=7;
		ft[2][1][1]=3;
		ft[2][1][2]=1;
		ft[2][2][0]=4;
		ft[2][2][1]=5;
		ft[2][2][2]=1;
		ft[2][3][0]=5;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][4][0]=8;
		ft[2][4][1]=9;
		ft[2][4][2]=0;
		ft[2][5][0]=9;
		ft[2][5][1]=9;
		ft[2][5][2]=0;
		ft[2][6][0]=12;
		ft[2][6][1]=9;
		ft[2][6][2]=1;
		ft[2][7][0]=6;
		ft[2][7][1]=16;
		ft[2][7][2]=0;
		
		ft[3][0][0]=4;
		ft[3][0][1]=-4;
		ft[3][0][2]=0;
		ft[3][1][0]=4;
		ft[3][1][1]=-6;
		ft[3][1][2]=0;
		ft[3][2][0]=2;
		ft[3][2][1]=-7;
		ft[3][2][2]=0;
		ft[3][3][0]=1;
		ft[3][3][1]=-8;
		ft[3][3][2]=3;
		ft[3][4][0]=0;
		ft[3][4][1]=-9;
		ft[3][4][2]=0;
		ft[3][5][0]=-1;
		ft[3][5][1]=-11;
		ft[3][5][2]=0;
		ft[3][6][0]=-2;
		ft[3][6][1]=-14;
		ft[3][6][2]=0;
		ft[3][7][0]=-3;
		ft[3][7][1]=-16;
		ft[3][7][2]=0;
	}
}

bool dFlowerClippings::animate(int32_t)
{
	return (clk++>=24);
}

void dFlowerClippings::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].tile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwFlowerClippings].tile;
		cs=wpnsbuf[iwFlowerClippings].csets&15;
		
	}
	
	
	for(int32_t i=0; i<4; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=t*4+i;
		decoration::draw8(dest);
	}
}

dGrassClippings::dGrassClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=1;
		ft[0][0][1]=0;
		ft[0][0][2]=1;
		ft[0][0][3]=0;
		ft[0][1][0]=-1;
		ft[0][1][1]=4;
		ft[0][1][2]=1;
		ft[0][1][3]=0;
		ft[0][2][0]=-5;
		ft[0][2][1]=2;
		ft[0][2][2]=0;
		ft[0][2][3]=1;
		ft[0][3][0]=-8;
		ft[0][3][1]=0;
		ft[0][3][2]=0;
		ft[0][3][3]=1;
		
		ft[1][0][0]=3;
		ft[1][0][1]=-4;
		ft[1][0][2]=0;
		ft[1][0][3]=1;
		ft[1][1][0]=8;
		ft[1][1][1]=-5;
		ft[1][1][2]=3;
		ft[1][1][3]=1;
		ft[1][2][0]=8;
		ft[1][2][1]=-5;
		ft[1][2][2]=3;
		ft[1][2][3]=0;
		ft[1][3][0]=8;
		ft[1][3][1]=-5;
		ft[1][3][2]=0;
		ft[1][3][3]=1;
		
		ft[2][0][0]=8;
		ft[2][0][1]=1;
		ft[2][0][2]=0;
		ft[2][0][3]=1;
		ft[2][1][0]=10;
		ft[2][1][1]=4;
		ft[2][1][2]=1;
		ft[2][1][3]=1;
		ft[2][2][0]=15;
		ft[2][2][1]=3;
		ft[2][2][2]=0;
		ft[2][2][3]=0;
		ft[2][3][0]=16;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][3][3]=1;
	}
}

bool dGrassClippings::animate(int32_t)
{
	return (clk++>=12);
}

void dGrassClippings::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].tile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwGrassClippings].tile;
		cs=wpnsbuf[iwGrassClippings].csets&15;
		
	}
	
	for(int32_t i=0; i<3; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=(t+(ft[i][int32_t(float(clk-1)/3)][3]))*4+i;
		decoration::draw8(dest);
	}
}

dHammerSmack::dHammerSmack(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=-5;
		ft[0][0][1]=-4;
		ft[0][0][2]=0;
		ft[0][1][0]=-4;
		ft[0][1][1]=-3;
		ft[0][1][2]=1;
		ft[0][2][0]=-5;
		ft[0][2][1]=-4;
		ft[0][2][2]=1;
		ft[0][3][0]=-8;
		ft[0][3][1]=-7;
		ft[0][3][2]=1;
		
		ft[1][0][0]=3;
		ft[1][0][1]=-4;
		ft[1][0][2]=0;
		ft[1][1][0]=5;
		ft[1][1][1]=-3;
		ft[1][1][2]=1;
		ft[1][2][0]=6;
		ft[1][2][1]=-4;
		ft[1][2][2]=1;
		ft[1][3][0]=9;
		ft[1][3][1]=-7;
		ft[1][3][2]=1;
	}
	
	wpnid=itemsbuf[current_item_id(itype_hammer)].wpn2;
}

bool dHammerSmack::animate(int32_t)
{
	return (clk++>=12);
}

void dHammerSmack::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=wpnsbuf[wpnid].tile;
	cs=wpnsbuf[wpnid].csets&15;
	flip=0;
	
	for(int32_t i=0; i<2; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		tile=t*4+i+(ft[i][int32_t(float(clk-1)/3)][2]*2);
		decoration::draw8(dest);
	}
}

dTallGrass::dTallGrass(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk,wpnSpr)
{
	id=Id;
	clk=Clk;
}

bool dTallGrass::animate(int32_t)
{
	if(HeroZ()>8 || HeroFakeZ()>8) return true;
	bool g1 = isGrassType(COMBOTYPE(HeroX(),HeroY()+15)), g2 = isGrassType(COMBOTYPE(HeroX()+15,HeroY()+15));
	if(get_qr(qr_BUSHESONLAYERS1AND2))
	{
		g1 = g1 || isGrassType(COMBOTYPEL(1,HeroX(),HeroY()+15)) || isGrassType(COMBOTYPEL(2,HeroX(),HeroY()+15));
		g2 = g2 || isGrassType(COMBOTYPEL(1,HeroX()+15,HeroY()+15)) || isGrassType(COMBOTYPEL(2,HeroX()+15,HeroY()+15));
	}
	return !(g1&&g2);
}

void dTallGrass::draw(BITMAP *dest)
{
	if(HeroGetDontDraw())
		return;
		
	int32_t t=0;
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].tile*4;
		cs=wpnsbuf[the_deco_sprite].csets&15;
	}
	else
	{
		t=wpnsbuf[iwTallGrass].tile*4;
		cs=wpnsbuf[iwTallGrass].csets&15;
	}
	
	flip=0;
	x=HeroX();
	y=HeroY()+10;
	
//  if (BSZ)
	if(zinit.heroAnimationStyle==las_bszelda)
	{
		tile=t+(anim_3_4(HeroLStep(),7)*2);
	}
	else
	{
		tile=t+((HeroLStep()>=6)?2:0);
	}
	
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

dRipples::dRipples(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk,wpnSpr)
{
	id=Id;
	clk=Clk;
}

bool dRipples::animate(int32_t)
{
	clk++;
	if (get_qr(qr_SHALLOW_SENSITIVE))
	{
		if (HeroZ() == 0 && HeroFakeZ() == 0 && HeroAction() != swimming && HeroAction() != sideswimming && HeroAction() != sideswimhit && HeroAction() != sideswimattacking && HeroAction() != isdiving && HeroAction() != drowning)
		{
			/*
			return !((FFORCOMBOTYPE(HeroX()+11,HeroY()+15)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+11,HeroY()+15)))
			&& (FFORCOMBOTYPE(HeroX()+4,HeroY()+15)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+4,HeroY()+15)))
			&& (FFORCOMBOTYPE(HeroX()+11,HeroY()+9)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+11,HeroY()+9)))
			&& (FFORCOMBOTYPE(HeroX()+4,HeroY()+9)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+4,HeroY()+9))));
			*/
			
			return !(iswaterex(FFORCOMBO(HeroX()+11,HeroY()+15), currmap, currscr, -1, HeroX()+11,HeroY()+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+4,HeroY()+15), currmap, currscr, -1, HeroX()+4,HeroY()+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+11,HeroY()+9), currmap, currscr, -1, HeroX()+11,HeroY()+9, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+4,HeroY()+9), currmap, currscr, -1, HeroX()+4,HeroY()+9, false, false, true, true));
		}
		return true;
	}
	else
	{
		return ((COMBOTYPE(HeroX(),HeroY()+15)!=cSHALLOWWATER)||
			(COMBOTYPE(HeroX()+15,HeroY()+15)!=cSHALLOWWATER) || HeroZ() != 0 || HeroFakeZ() != 0);
	}
}

void dRipples::draw(BITMAP *dest)
{
	if(HeroGetDontDraw())
		return;
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].tile*4;
		cs=wpnsbuf[the_deco_sprite].csets&15;
	}
	else
	{
		t=wpnsbuf[iwRipples].tile*4;
		cs=wpnsbuf[iwRipples].csets&15;
	}
	
	flip=0;
	x=HeroX();
	y=HeroY()+10;
	tile=t+(((clk/8)%3)*2);
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

dHover::dHover(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
	wpnid = itemsbuf[current_item_id(itype_hoverboots)].wpn;
}

void dHover::draw(BITMAP *dest)
{
	int32_t t=wpnsbuf[wpnid].tile*4;
	cs=wpnsbuf[wpnid].csets&15;
	flip=0;
	x=HeroX();
	y=HeroY()+10-HeroZ()-HeroFakeZ();
	tile=t+(((clk/8)%3)*2);
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

bool dHover::animate(int32_t)
{
	clk++;
	return HeroHoverClk()<=0;
}

dDivineProtectionShield::dDivineProtectionShield(zfix X,zfix Y,int32_t Id,int32_t Clk) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
}

bool dDivineProtectionShield::animate(int32_t)
{
	clk++;
	return HeroDivineProtectionShieldClk()<=0;
}

void dDivineProtectionShield::realdraw(BITMAP *dest, int32_t draw_what)
{
	if(misc!=draw_what)
	{
		return;
	}
	
	int32_t fb=(misc==0?
	        (itemsbuf[current_item_id(itype_divineprotection)].wpn5 ?
	         itemsbuf[current_item_id(itype_divineprotection)].wpn5 : (byte) iwDivineProtectionShieldFront) :
	            (itemsbuf[current_item_id(itype_divineprotection)].wpn10 ?
	             itemsbuf[current_item_id(itype_divineprotection)].wpn10 : (byte) iwDivineProtectionShieldBack));
	int32_t t=wpnsbuf[fb].tile;
	int32_t fr=wpnsbuf[fb].frames;
	int32_t spd=wpnsbuf[fb].speed;
	cs=wpnsbuf[fb].csets&15;
	flip=0;
	bool flickering = (itemsbuf[current_item_id(itype_divineprotection)].flags & item_flag4) != 0;
	bool translucent = (itemsbuf[current_item_id(itype_divineprotection)].flags & item_flag3) != 0;
	
	if(((HeroDivineProtectionShieldClk()&0x20)||(HeroDivineProtectionShieldClk()&0xF00))&&(!flickering ||((misc==1)?(frame&1):(!(frame&1)))))
	{
		drawstyle=translucent?1:0;
		x=HeroX()-8;
		y=HeroY()-8-HeroZ()-HeroFakeZ();
		tile=t;
		
		if(fr>0&&spd>0)
		{
			tile+=((clk/spd)%fr);
		}
		
		decoration::draw(dest);
		x+=16;
		tile+=fr;
		decoration::draw(dest);
		x-=16;
		y+=16;
		tile+=fr;
		decoration::draw(dest);
		x+=16;
		tile+=fr;
		decoration::draw(dest);
	}
}

void dDivineProtectionShield::draw(BITMAP *dest)
{
	realdraw(dest,0);
}

void dDivineProtectionShield::draw2(BITMAP *dest)
{
	realdraw(dest,1);
}

//

customWalkSprite::customWalkSprite(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) :
	comboSprite(X,Y,Id,Clk,wpnSpr), bits(0)
{}

bool customWalkSprite::animate(int32_t)
{
	if(bits&0b01)
		bits &= ~0b01;
	else return true;
	if(!(bits & 0b10))
		return false;
	auto const& wspr = wpnsbuf[the_deco_sprite];
	int32_t dur = zc_max(1,wspr.frames) * zc_max(1,wspr.speed);
	clk = (clk+1)%dur;
	return false;
}

void customWalkSprite::realdraw(BITMAP *dest, int32_t draw_what)
{
	if(misc!=draw_what || the_deco_sprite < 0)
		return;
	bits |= 0b10;
	
	auto const& wspr = wpnsbuf[the_deco_sprite];
	int32_t t=wspr.tile;
	int32_t fr=zc_max(1,wspr.frames);
	int32_t spd=zc_max(1,wspr.speed);
	cs=wspr.csets&15;
	flip=0;
	x=HeroX();
	y=HeroY()+2;
	
	tile = t+((clk/spd)%fr);
	
	decoration::draw(dest);
}

void customWalkSprite::run_sprite(int32_t newSprite)
{
	bits |= 0b01; //ready to animate once more
	if(newSprite != the_deco_sprite)
	{
		clk = 0;
		bits &= ~0b10; //has not drawn first frame of new sprite yet
		the_deco_sprite = newSprite;
	}
}

