#ifndef DECORATIONS_H_
#define DECORATIONS_H_

#include "base/zfix.h"
#include "sprite.h"

class decoration : public sprite
{
public:
	decoration(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	int32_t the_deco_sprite;
	virtual ~decoration();
	// If returns true, this sprite is drawn relative to some other entity (like
	// the player).
	virtual bool is_drawn_with_offset() {
		return false;
	};
};

class comboSprite : public decoration
{
public:
	comboSprite(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
	virtual void draw2(BITMAP *dest);
	virtual void realdraw(BITMAP *dest, int32_t draw_what);
};

class statusSprite : public decoration
{
public:
	sprite* target;
	int32_t plain_tile;
	byte tile_width, tile_height;
	
	statusSprite(zfix X,zfix Y, int32_t spr, int32_t tile);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class dBushLeaves : public decoration
{
public:
	static int32_t ft[4][8][3]; //[leaf][frame][x/y/flip]
	int32_t ox, oy;
	dBushLeaves(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class dFlowerClippings : public decoration
{
public:
	static int32_t ft[4][8][3]; //[leaf][frame][x/y/flip]
	int32_t ox, oy;
	dFlowerClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class dGrassClippings : public decoration
{
public:
	static int32_t ft[3][4][4]; //[leaf][frame][x/y/flip/size(0=large, 1=small)]
	int32_t ox, oy;
	dGrassClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class dHammerSmack : public decoration
{
public:
	static int32_t ft[2][4][3]; //[leaf][frame][x/y/icon(0=spark, 1=star)]
	int32_t ox, oy, wpnid;
	dHammerSmack(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index);
	virtual void draw(BITMAP *dest);
};

class dTallGrass : public decoration
{
public:
	dTallGrass(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index) override;
	virtual void draw(BITMAP *dest) override;
	bool is_drawn_with_offset() override {
		return true;
	};
};

class dRipples : public decoration
{
public:
	dRipples(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index) override;
	virtual void draw(BITMAP *dest) override;
	bool is_drawn_with_offset() override {
		return true;
	};
};

class dHover : public decoration
{
	int32_t wpnid;
public:
	dHover(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index) override;
	virtual void draw(BITMAP *dest) override;
	bool is_drawn_with_offset() override {
		return true;
	};
};

class dDivineProtectionShield : public decoration
{
public:
	dDivineProtectionShield(zfix X,zfix Y,int32_t Id,int32_t Clk);
	virtual bool animate(int32_t index) override;
	virtual void realdraw(BITMAP *dest, int32_t draw_what);
	virtual void draw(BITMAP *dest) override;
	virtual void draw2(BITMAP *dest) override;
	bool is_drawn_with_offset() override {
		return true;
	};
};

class customWalkSprite : public comboSprite
{
public:
	byte bits;
	customWalkSprite(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr=0);
	virtual bool animate(int32_t index) override;
	virtual void realdraw(BITMAP *dest, int32_t draw_what) override;
	void run_sprite(int32_t newSprite);
	bool is_drawn_with_offset() override {
		return true;
	};
};

#endif
