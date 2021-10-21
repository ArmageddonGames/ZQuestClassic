//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  pal.h
//
//  Palette code for ZC.
//
//--------------------------------------------------------

#ifndef _ZC_PAL_H_
#define _ZC_PAL_H_

#include "zc_alleg.h"
#include "zdefs.h"
#include "colors.h"

extern int32_t CSET_SIZE;                                       // this is only changed to 4 in the NES title screen
extern int32_t CSET_SHFT;                                       // log2 of CSET_SIZE

#define CSET(x)         ((x)<<CSET_SHFT)
#define csBOSS          14

extern RGB _RGB(byte *si);
extern RGB _RGB(int32_t r,int32_t g,int32_t b);
extern RGB invRGB(RGB s);
extern RGB mixRGB(int32_t r1,int32_t g1,int32_t b1,int32_t r2,int32_t g2,int32_t b2,int32_t ratio);

extern void copy_pal(RGB *src,RGB *dest);
extern void loadfullpal();
extern void loadlvlpal(int32_t level);
extern void loadpalset(int32_t cset,int32_t dataset);
extern void loadfadepal(int32_t dataset);
extern void interpolatedfade();

extern void ringcolor(bool forceDefault);
extern void fade(int32_t level,bool blackall,bool fromblack);
extern void lighting(bool existslight, bool setnaturaldark = false, int32_t specialstate = pal_litRESET);
extern void lightingInstant();
extern void dryuplake();
extern void rehydratelake(bool instant);
extern void reset_pal_cycling();
extern void cycle_palette();

INLINE RGB NESpal(int32_t i)
{
    return _RGB(nes_pal+i*3);
}
extern int32_t reverse_NESpal(RGB c);

#endif                                                      // _ZC_PAL_H_

