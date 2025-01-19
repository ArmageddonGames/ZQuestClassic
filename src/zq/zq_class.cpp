#include <chrono>
#include <cstring>
#include <exception>
#include <string>
#include <stdexcept>
#include <map>

#include "base/general.h"
#include "base/version.h"
#include "base/zapp.h"
#include "dialog/info.h"
#include "metadata/metadata.h"

#include "base/qrs.h"
#include "base/dmap.h"
#include "base/packfile.h"
#include "base/cpool.h"
#include "base/autocombo.h"
#include "base/gui.h"
#include "base/msgstr.h"
#include "zc/zelda.h"
#include "zq/zq_class.h"
#include "zq/zq_misc.h"
#include "zq/zquest.h"
#include "base/qst.h"
#include "base/colors.h"
#include "tiles.h"
#include "base/zsys.h"
#include "sprite.h"
#include "items.h"
#include "zc/zc_sys.h"
#include "md5.h"
#include "zc/zc_custom.h"
#include "subscr.h"
#include "zq/zq_strings.h"
#include "zq/zq_subscr.h"
#include "zc/ffscript.h"
#include "base/util.h"
#include "zq/zq_files.h"
#include "dialog/alert.h"
#include "slopes.h"
#include "drawing.h"
#include "zinfo.h"
#include "zq/render_minimap.h"
#include "base/mapscr.h"
#include <fmt/format.h>
#include <filesystem>

#ifdef __EMSCRIPTEN__
#include "base/emscripten_utils.h"
#endif

namespace fs = std::filesystem;

using namespace util;

extern ZModule zcm;
extern zcmodule moduledata;
extern uint8_t ViewLayer3BG, ViewLayer2BG;
extern int32_t LayerDitherBG, LayerDitherSz;
extern bool NoHighlightLayer0;

using std::string;
using std::pair;
#define EPSILON 0.01 // Define your own tolerance
#define FLOAT_EQ(x,v) (((v - EPSILON) < x) && (x <( v + EPSILON)))

#define COLOR_SOLID  vc(4)
#define COLOR_SLOPE  vc(13)
#define COLOR_LADDER vc(6)
//#define COLOR_EFFECT vc(10)

//const char zqsheader[30]="ZQuest Classic String Table\n\x01";
extern char msgbuf[MSG_NEW_SIZE*8];

extern string zScript;

zmap Map;
int32_t prv_mode=0;
int16_t ffposx[MAXFFCS];
int16_t ffposy[MAXFFCS];
int32_t ffprvx[MAXFFCS];
int32_t ffprvy[MAXFFCS];
void init_ffpos()
{
    for (word q = 0; q < MAXFFCS; ++q)
    {
        ffposx[q] = -1000;
        ffposy[q] = -1000;
        ffprvx[q] = -10000000;
        ffprvy[q] = -10000000;
    }
}

bool save_warn=true;

int32_t COMBOPOS(int32_t x, int32_t y)
{
    return (((y) & 0xF0) + ((x) >> 4));
}
int32_t COMBOPOS_B(int32_t x, int32_t y)
{
	if(unsigned(x) >= 256 || unsigned(y) >= 176)
		return -1;
	return COMBOPOS(x,y);
}
int32_t COMBOX(int32_t pos)
{
    return ((pos) % 16 * 16);
}
int32_t COMBOY(int32_t pos)
{
    return ((pos) & 0xF0);
}

void reset_dmap(int32_t index)
{
    bound(index,0,MAXDMAPS-1);
	DMaps[index].clear();
	DMaps[index].title = "";
    sprintf(DMaps[index].intro, "                                                                        ");
}

void reset_dmaps()
{
    for(int32_t i=0; i<MAXDMAPS; i++)
        reset_dmap(i);
}

void truncate_dmap_title(std::string& title)
{
	title.resize(21, ' ');
}

mapscr* zmap::get_prvscr()
{
    return &prvscr;
}

zmap::zmap()
{
    can_paste=false;
    prv_cmbcycle=0;
    prv_advance=0;
    prv_freeze=0;
    copyffc=-1;

    memset(scrpos, 0, sizeof(scrpos));
	memset(scrview, 0, sizeof(scrview));
    screens=NULL;
    prv_time=0;
    prv_scr=0;
    prv_map=0;
    copyscr=0;
    cursor={};
    copymap=0;
    layer_target_map = 0;
    layer_target_scr = 0;
    layer_target_multiple = 0;
}

void zmap::clear()
{
	*this = zmap();
}
void zmap::force_refr_pointer()
{
	if(unsigned(cursor.map) > map_count || (cursor.map*MAPSCRS > TheMaps.size()))
		screens = nullptr;
	else screens = &TheMaps[cursor.map*MAPSCRS];
}
bool zmap::CanUndo()
{
    return undo_stack.size() > 0;
}
bool zmap::CanRedo()
{
    return redo_stack.size() > 0;
}
bool zmap::CanPaste()
{
    return can_paste;
}
int32_t  zmap::CopyScr()
{
    return (copymap<<8)+copyscr;
}
int32_t zmap::getCopyFFC()
{
    return copyffc;
}
set_ffc_command::data_t zmap::getCopyFFCData()
{
    return set_ffc_command::create_data(copymapscr.ffcs[copyffc]);
}
int32_t zmap::getMapCount()
{
    return map_count;
}
int32_t zmap::getLayerTargetMap()
{
    return layer_target_map;
}
int32_t zmap::getLayerTargetScr()
{
    return layer_target_scr;
}
int32_t zmap::getLayerTargetMultiple()
{
    return layer_target_multiple;
}
bool zmap::isDungeon(int32_t scr)
{
    for(int32_t i=0; i<4; i++)
    {
        if(screens[scr].data[i]!=screens[TEMPLATE].data[i])
        {
            return false;
        }
    }
    
    return true;
}

bool zmap::clearall(bool validate)
{
    Color=0;
    char tbuf[10];
    
    if((header.templatepath[0]!=0)&&validate)
    {
        if(!valid_zqt(header.templatepath))
        {
            jwin_alert("Error","Invalid Quest Template",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
            return false;
        }
    }
    
    for(int32_t i=0; i<map_count; i++)
    {
        setCurrMap(i);
        sprintf(tbuf, "%d", i);
        clearmap(true);
    }
    
    setCurrMap(0);
    return true;
}

bool zmap::reset_templates(bool validate)
{
    //why are we doing this?
    if(colordata==NULL)
    {
        return false;
    }
    
    //int32_t ret;
    word version, build, dummy, sversion=0;
	byte dummyc;
	word dummyw;
    //int32_t section_size;
    word temp_map_count;
    mapscr temp_mapscr;
    PACKFILE *f=NULL;
    
//  setPackfilePassword(datapwd);
    f=open_quest_template(&header, "modules/classic/default.qst", validate);
    get_version_and_build(f, &version, &build);
    
    if(!find_section(f, ID_MAPS))
    {
//	  setPackfilePassword(NULL);
        return false;
    }
    
    //section version info
    if(!p_igetw(&sversion,f))
    {
        return false;
    }
    
    if(!p_igetw(&dummy,f))
    {
        return false;
    }
    
    //section size
    dword dummy_size;
    if(!p_igetl(&dummy_size,f))
    {
        return false;
    }
    
    //finally...  section data
    if(!p_igetw(&temp_map_count,f))
    {
        return false;
    }
    
    if(version>12)
    {
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
    }
    
    for(int32_t i=0; i<MAPSCRSNORMAL; ++i)
    {
        readmapscreen(f, &header, &temp_mapscr, sversion);
    }
    
    readmapscreen(f, &header, &TheMaps[128], sversion);
    readmapscreen(f, &header, &TheMaps[129], sversion);
    
    for(int32_t i=0; i<(MAPSCRS-(MAPSCRSNORMAL+2)); ++i)
    {
        readmapscreen(f, &header, &temp_mapscr, sversion);
    }
    
    if(version>12)
    {
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_igetw(&dummyw,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
        
        if(!p_getc(&dummyc,f))
            return qe_invalid;
    }
    
    for(int32_t i=0; i<MAPSCRSNORMAL; ++i)
    {
        readmapscreen(f, &header, &temp_mapscr, sversion);
    }
    
    readmapscreen(f, &header, &TheMaps[MAPSCRS+128], sversion);
    readmapscreen(f, &header, &TheMaps[MAPSCRS+129], sversion);
    
    pack_fclose(f);
	clear_quest_tmpfile();

    return true;
}

bool zmap::clearmap(bool newquest)
{
    if(cursor.map<map_count)
    {
        for(int32_t i=0; i<MAPSCRS-(newquest?0:TEMPLATES); i++)
        {
            clearscr(i);
        }
        
        setCurrScr(0);
        
        if(newquest)
        {
            if(!reset_templates(false))
            {
                jwin_alert("Error","Error resetting","template screens.",NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
            }
        }
    }
    
    return true;
}

MapCursor zmap::getCursor() const
{
	return cursor;
}

void zmap::setCursor(MapCursor new_cursor)
{
	if (cursor == new_cursor)
		return;

	optional<int> oldcolor;
	if (screens)
		oldcolor = getcolor();

	cursor = new_cursor;

	int newcolor = getcolor();
	loadlvlpal(newcolor);
	if (!oldcolor || *oldcolor != newcolor)
		rebuild_trans_table();
	
	reset_combo_animations2();
	mmap_mark_dirty();
	refresh(rALL);
}

bool zmap::isValidPosition(ComboPosition pos) const
{
    return pos.is_valid(cursor);
}

int zmap::getScreenForPosition(ComboPosition pos) const
{
	return cursor.viewscr + pos.screen_offset();
}

mapscr* zmap::CurrScr()
{
    return screens+cursor.screen;
}
mapscr* zmap::Scr(int32_t scr)
{
    return screens+scr;
}
mapscr* zmap::Scr(ComboPosition pos)
{
	if (!pos.is_valid(cursor))
		return nullptr;

	int screen_offset = pos.screen_offset();
	int screen = cursor.viewscr + screen_offset;
	return AbsoluteScr(cursor.map, screen);
}
mapscr* zmap::Scr(ComboPosition pos, int layer)
{
	int map = cursor.map;
	int screen = cursor.viewscr + pos.screen_offset();
	if (layer)
	{
		mapscr* scr = Map.AbsoluteScr(map, screen);
		map = scr->layermap[CurrentLayer-1]-1;
		screen = scr->layerscreen[CurrentLayer-1];
	}

	return AbsoluteScr(map, screen);
}
mapscr* zmap::ScrMakeValid(ComboPosition pos, int layer)
{
	mapscr* scr = Scr(pos, layer);
	if (scr && !(scr->valid&mVALID))
	{
		scr->valid |= mVALID;
		setcolor(Color, scr);
	}
	return scr;
}
mapscr* zmap::AbsoluteScr(int32_t scr)
{
	if(unsigned(scr) >= MAPSCRS*getMapCount())
		return nullptr;
    return &TheMaps[scr];
}
mapscr* zmap::AbsoluteScr(int32_t map, int32_t scr)
{
	if(map < 0 || map >= getMapCount() || scr < 0 || scr >= MAPSCRS)
		return nullptr;
    return AbsoluteScr((map*MAPSCRS)+scr);
}
mapscr* zmap::AbsoluteScrMakeValid(int32_t map, int32_t screen)
{
	mapscr* scr = AbsoluteScr(map, screen);
	if (scr && !(scr->valid&mVALID))
	{
		scr->valid |= mVALID;
		setcolor(Color, scr);
	}
	return scr;
}
void zmap::set_prvscr(int32_t map, int32_t scr)
{
	prvscr=TheMaps[(map*MAPSCRS)+scr];
	
	for(int32_t i=0; i<6; i++)
	{
		if(prvscr.layermap[i]>0)
		{
			prvlayers[i]=TheMaps[(prvscr.layermap[i]-1)*MAPSCRS+prvscr.layerscreen[i]];
		}
		else
			prvlayers[i].valid = 0;
	}
	
	prv_map=map;
	prv_scr=scr;
}
int32_t  zmap::getCurrMap()
{
    return cursor.map;
}
bool zmap::isDark(int scr)
{
    return (screens[scr].flags&fDARK)!=0;
}

void zmap::setCurrMap(int32_t index)
{
	optional<int> oldcolor;
	if(screens)
		oldcolor = getcolor();
	scrpos[cursor.map] = cursor.screen;
	scrview[cursor.map] = cursor.viewscr;
	cursor.map = bound(index,0,map_count);
	screens = &TheMaps[cursor.map*MAPSCRS];

	cursor.viewscr = scrview[cursor.map];
	cursor.setScreen(scrpos[cursor.map]);

	int newcolor = getcolor();
	loadlvlpal(newcolor);
	if(!oldcolor || *oldcolor != newcolor)
		rebuild_trans_table();
	
	reset_combo_animations2();
	mmap_mark_dirty();
}

int32_t  zmap::getCurrScr()
{
    return cursor.screen;
}
void zmap::setCurrScr(int32_t scr)
{
    if (scr == cursor.screen)
		return;

    int32_t oldcolor=getcolor();
	cursor.setScreen(scr);
    if (!(screens[cursor.screen].valid&mVALID))
    {
        oldcolor=-1;
    }

    int32_t newcolor=getcolor();
    loadlvlpal(newcolor);
    if (newcolor!=oldcolor)
    {
        rebuild_trans_table();
    }

    reset_combo_animations2();
    setlayertarget();
    mmap_mark_dirty();
}

int32_t zmap::getViewScr()
{
	return cursor.viewscr;
}

void zmap::setViewSize(int32_t size)
{
	cursor.setSize(size);
}

int32_t zmap::getViewSize()
{
	return cursor.size;
}

void zmap::setlayertarget()
{
    layer_target_map = 0;
    layer_target_multiple = 0;
    
    for(int32_t m=0; m<getMapCount(); ++m)
    {
        for(int32_t s=0; s<MAPSCRS; ++s)
        {
            int32_t i=(m*MAPSCRS+s);
            mapscr *ts=&TheMaps[i];
            
            // Search through each layer
            for(int32_t w=0; w<6; ++w)
            {
                if(ts->layerscreen[w]==cursor.screen && (ts->layermap[w]-1)==cursor.map)
                {
                    if(layer_target_map > 0)
                    {
                        layer_target_multiple += 1;
                        continue;
                    }
                    
                    layer_target_map = m+1;
                    layer_target_scr = s;
                }
            }
        }
    }
}

void zmap::setcolor(int color, mapscr* scr)
{
	if (!scr) scr = CurrScr();
	scr->valid |= mVALID;
	scr->color = color;

	if(Color!=color)
	{
		Color = color;
		loadlvlpal(color);
		rebuild_trans_table();
	}

	mmap_mark_dirty();
}

int32_t zmap::getcolor()
{
    if(prv_mode)
    {
        return prvscr.color;
    }
    
    return screens[cursor.screen].color;
}

void zmap::resetflags()
{
    byte *di=&(screens[cursor.screen].valid);
    
    for(int32_t i=1; i<48; i++)
    {
        *(di+i)=0;
    }
}

word zmap::tcmbdat(int32_t pos)
{
    return screens[TEMPLATE].data[pos];
}

word zmap::tcmbcset(int32_t pos)
{
    return screens[TEMPLATE].cset[pos];
}

int32_t zmap::tcmbflag(int32_t pos)
{
    return screens[TEMPLATE].sflag[pos];
}

word zmap::tcmbdat2(int32_t pos)
{
    return screens[TEMPLATE2].data[pos];
}

word zmap::tcmbcset2(int32_t pos)
{
    return screens[TEMPLATE2].cset[pos];
}

int32_t zmap::tcmbflag2(int32_t pos)
{
    return screens[TEMPLATE2].sflag[pos];
}

void zmap::TemplateAll()
{
    StartListCommand();
    for(int32_t i=0; i<128; i++)
    {
        if((screens[i].valid&mVALID) && isDungeon(i))
            DoTemplateCommand(-1, -1, i);
    }
    FinishListCommand();
}

void zmap::Template(int32_t floorcombo, int32_t floorcset, int32_t scr)
{
    if(scr==TEMPLATE)
        return;
        
    if(!(screens[scr].valid&mVALID))
        screens[scr].color=Color;
        
    screens[scr].valid|=mVALID;
    
    for(int32_t i=0; i<32; i++)
    {
        screens[scr].data[i]=screens[TEMPLATE].data[i];
        screens[scr].cset[i]=screens[TEMPLATE].cset[i];
        screens[scr].sflag[i]=screens[TEMPLATE].sflag[i];
    }
    
    for(int32_t i=144; i<176; i++)
    {
        screens[scr].data[i]=screens[TEMPLATE].data[i];
        screens[scr].cset[i]=screens[TEMPLATE].cset[i];
        screens[scr].sflag[i]=screens[TEMPLATE].sflag[i];
    }
    
    for(int32_t y=2; y<=9; y++)
    {
        int32_t j=y<<4;
        screens[scr].data[j]=screens[TEMPLATE].data[j];
        screens[scr].cset[j]=screens[TEMPLATE].cset[j];
        screens[scr].sflag[j]=screens[TEMPLATE].sflag[j];
        ++j;
        screens[scr].data[j]=screens[TEMPLATE].data[j];
        screens[scr].cset[j]=screens[TEMPLATE].cset[j];
        screens[scr].sflag[j]=screens[TEMPLATE].sflag[j];
        ++j;
        j+=12;
        screens[scr].data[j]=screens[TEMPLATE].data[j];
        screens[scr].cset[j]=screens[TEMPLATE].cset[j];
        screens[scr].sflag[j]=screens[TEMPLATE].sflag[j];
        ++j;
        screens[scr].data[j]=screens[TEMPLATE].data[j];
        screens[scr].cset[j]=screens[TEMPLATE].cset[j];
        
        screens[scr].sflag[j]=screens[TEMPLATE].sflag[j];
        ++j;
    }
    
    if(floorcombo!=-1)
    {
        for(int32_t y=2; y<9; y++)
            for(int32_t x=2; x<14; x++)
            {
                int32_t i=(y<<4)+x;
                screens[scr].data[i] = floorcombo;
                screens[scr].cset[i] = floorcset;
            }
    }
    
    for(int32_t i=0; i<4; i++)
        putdoor(scr,i,screens[scr].door[i]);
}


void zmap::clearscr(int32_t scr)
{
    screens[scr].zero_memory();
    screens[scr].valid=mVERSION;
	for(int q = 0; q < 6; ++q)
	{
		auto layer = map_autolayers[cursor.map*6+q];
		screens[scr].layermap[q] = layer;
		screens[scr].layerscreen[q] = layer ? scr : 0;
	}
	mmap_mark_dirty();
}

const char *loaderror[] =
{

    "OK","File not found","Incomplete data",
    "Invalid version","Invalid file"
    
};

int32_t zmap::load(const char *path)
{
	PACKFILE *f=pack_fopen_password(path,F_READ, "");
	
	if(!f)
		return 1;
		
		
	int16_t version;
	byte build;
	
	//get the version
	if(!p_igetw(&version,f))
	{
		goto file_error;
	}
	
	//get the build
	if(!p_getc(&build,f))
	{
		goto file_error;
	}
	
	for(int32_t i=0; i<MAPSCRS; i++)
	{
		mapscr tmpimportscr;
		tmpimportscr.zero_memory();
		if(readmapscreen(f,&header,&tmpimportscr,version)==qe_invalid)
		{
			al_trace("failed zmap::load\n");
				goto file_error;
		}
		
		switch(ImportMapBias)
		{
			case 0:
				*(screens+i) = tmpimportscr;
				break;
				
			case 1:
				if(!(screens[i].valid&mVALID))
				{
					*(screens+i) = tmpimportscr;
				}
				break;
				
			case 2:
				if(tmpimportscr.valid&mVALID)
				{
					*(screens+i) = tmpimportscr;
				}
				break;
		}
	}
	
	
	pack_fclose(f);
	
	setCurrScr(0);
	mmap_mark_dirty();
	return 0;
	
file_error:
	pack_fclose(f);
	clearmap(false);
	return 2;
}

int32_t zmap::save(const char *path)
{
	PACKFILE *f=pack_fopen_password(path,F_WRITE, "");
	
	if(!f)
		return 1;
	
	if(!p_iputw(V_MAPS,f))
	{
		pack_fclose(f);
		return 3;
	}
	
	// This was the "build number", but that's totally useless here. Keep this junk byte
	// so as not to totally break exports between ZC versions.
	if(!p_putc(0,f))
	{
		pack_fclose(f);
		return 3;
	}
	
	for(int32_t i=0; i<MAPSCRS; i++)
	{
		if(writemapscreen(f,this->getCurrMap(),i) == qe_invalid)
		{
			pack_fclose(f);
			return 2;
		}
	}
	
	pack_fclose(f);
	return 0;
}


bool zmap::ishookshottable(int32_t bx, int32_t by, int32_t i)
{
	// Hookshots can be blocked by solid combos on all 3 ground layers.
	const newcombo* c = &combobuf[MAPCOMBO(bx,by)];
	
	if(c->type == cHOOKSHOTONLY || c->type == cLADDERHOOKSHOT)
		return true;
	if (c->walk&(1<<i))
		return false;
	
	for(int32_t k=0; k<2; k++)
	{
		c = &combobuf[MAPCOMBO2(k+1,bx,by)];
		
		if(c->type != cHOOKSHOTONLY && c->type != cLADDERHOOKSHOT && c->walk&(1<<i))
		{
			return false;
		}
	}
	
	return true;
}

bool zmap::ishookshottable(int32_t map, int32_t screen, int32_t bx, int32_t by, int32_t i)
{
	// Hookshots can be blocked by solid combos on all 3 ground layers.
	const newcombo* c = &combobuf[MAPCOMBO3(map, screen, -1, bx,by)];
	
	if(c->type == cHOOKSHOTONLY || c->type == cLADDERHOOKSHOT)
		return true;
	if (c->walk&(1<<i))
		return false;
	
	for(int32_t k=0; k<2; k++)
	{
		c = &combobuf[MAPCOMBO3(map, screen, k+1,bx,by)];
		
		if(c->type != cHOOKSHOTONLY && c->type != cLADDERHOOKSHOT && c->walk&(1<<i))
		{
			return false;
		}
	}
	
	return true;
}

bool zmap::isstepable(int32_t combo)
{
    // This is kind of odd but it's true to the engine (see maps.cpp)
    return (combo_class_buf[combobuf[combo].type].ladder_pass);
}

// Returns the letter of the warp combo.
int32_t zmap::warpindex(int32_t combo)
{
    switch(combobuf[combo].type)
    {
    case cCAVE:
    case cPIT:
    case cSTAIR:
    case cCAVE2:
    case cSWIMWARP:
    case cDIVEWARP:
    case cSWARPA:
        return 0;
        
    case cCAVEB:
    case cPITB:
    case cSTAIRB:
    case cCAVE2B:
    case cSWIMWARPB:
    case cDIVEWARPB:
    case cSWARPB:
        return 1;
        
    case cCAVEC:
    case cPITC:
    case cSTAIRC:
    case cCAVE2C:
    case cSWIMWARPC:
    case cDIVEWARPC:
    case cSWARPC:
        return 2;
        
    case cCAVED:
    case cPITD:
    case cSTAIRD:
    case cCAVE2D:
    case cSWIMWARPD:
    case cDIVEWARPD:
    case cSWARPD:
        return 3;
        
    case cPITR:
    case cSTAIRR:
    case cSWARPR:
        return 4;
    }
    
    return -1;
    
}

void draw_ladder(BITMAP* dest, int32_t x, int32_t y, int32_t c, bool top = false)
{
	if(top)
		line(dest,x,y,x+15,y,c);
	rectfill(dest,x,y,x+3,y+15,c);
	rectfill(dest,x+12,y,x+15,y+15,c);
	rectfill(dest,x+4,y+2,x+11,y+5,c);
	rectfill(dest,x+4,y+10,x+11,y+13,c);
}

void draw_platform(BITMAP* dest, int32_t x, int32_t y, int32_t c)
{
	line(dest,x,y,x+15,y,c);
}

void zmap::put_walkflags_layered(BITMAP *dest,int32_t x,int32_t y,int32_t pos,int32_t layer)
{
	int32_t cx = COMBOX(pos);
	int32_t cy = COMBOY(pos);
	
	newcombo const& c = combobuf[ MAPCOMBO2(layer,cx,cy) ];
	
	if (c.type == cBRIDGE && get_qr(qr_OLD_BRIDGE_COMBOS)) return;
	
	int32_t bridgedetected = 0;
	
	for(int32_t i=0; i<4; i++)
	{
		int32_t tx=((i&2)<<2)+x;
		int32_t ty=((i&1)<<3)+y;
		int32_t tx2=((i&2)<<2)+cx;
		int32_t ty2=((i&1)<<3)+cy;
		for (int32_t m = layer; m <= 1; m++)
		{
			if (get_qr(qr_OLD_BRIDGE_COMBOS))
			{
				if (combobuf[MAPCOMBO2(m,tx2,ty2)].type == cBRIDGE && !(combobuf[MAPCOMBO2(m,tx2,ty2)].walk&(1<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
			else
			{
				if (combobuf[MAPCOMBO2(m,tx2,ty2)].type == cBRIDGE && (combobuf[MAPCOMBO2(m,tx2,ty2)].walk&(0x10<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
		}
		if (bridgedetected & (1<<i))
		{
			if (i >= 3) break;
			else continue;
		}
		if ((c.walk&(1<<(i+4))) && ((c.walk&(1<<i) && ((c.usrflags&cflag4)) && c.type == cWATER) || c.type == cSHALLOWWATER)) 
		{
			for(int32_t k=0; k<8; k+=2)
				for(int32_t j=0; j<8; j+=2)
					if(((k+j)/2)%2) rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(11));
		}
		if (!(c.walk&(1<<i) && ((c.usrflags&cflag3) || (c.usrflags&cflag4))) && (layer==-1 || (get_qr(qr_WATER_ON_LAYER_1) && layer == 0) || (get_qr(qr_WATER_ON_LAYER_2) && layer == 1)) && combo_class_buf[c.type].water!=0 && get_qr(qr_DROWN))
			rectfill(dest,tx,ty,tx+7,ty+7,vc(11));
			
		if(c.walk&(1<<i) && !(combo_class_buf[c.type].water!=0 && ((c.usrflags&cflag3) || (c.usrflags&cflag4))))
		{
			if(c.type==cLADDERHOOKSHOT && isstepable(MAPCOMBO(cx,cy)) && ishookshottable(cx,cy,i))
			{
				for(int32_t k=0; k<8; k+=2)
					for(int32_t j=0; j<8; j+=2)
						rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(6+((k+j)/2)%2));
			}
			else
			{
				int32_t color = COLOR_SOLID;
				
				if(isstepable(MAPCOMBO(cx,cy)) && (!get_qr(qr_NO_SOLID_SWIM) || (combo_class_buf[combobuf[MAPCOMBO(cx,cy)].type].water==0 && combo_class_buf[c.type].water==0)))
					color=vc(6);
				else if((c.type==cHOOKSHOTONLY || c.type==cLADDERHOOKSHOT) && ishookshottable(cx,cy,i))
					color=vc(7);
					
				rectfill(dest,tx,ty,tx+7,ty+7,color);
			}
		}
	}
	
	bridgedetected = 0;
	for(int32_t i=0; i<4; i++)
	{
		int32_t tx2=((i&2)<<2)+cx;
		int32_t ty2=((i&1)<<3)+cy;
		for (int32_t m = 0; m <= 1; m++)
		{
			if (get_qr(qr_OLD_BRIDGE_COMBOS))
			{
				if (combobuf[MAPCOMBO2(m,tx2,ty2)].type == cBRIDGE && !(combobuf[MAPCOMBO2(m,tx2,ty2)].walk&(1<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
			else
			{
				if (combobuf[MAPCOMBO2(m,tx2,ty2)].type == cBRIDGE && (combobuf[MAPCOMBO2(m,tx2,ty2)].walk&(0x10<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
		}
	}
	
	// Draw damage combos
	newcombo const& c0 = combobuf[MAPCOMBO2(-1,cx,cy)];
	newcombo const& c1 = combobuf[MAPCOMBO2(0,cx,cy)];
	newcombo const& c2 = combobuf[MAPCOMBO2(1,cx,cy)];
	bool dmg = combo_class_buf[c0.type].modify_hp_amount
	   || combo_class_buf[c1.type].modify_hp_amount
	   || combo_class_buf[c2.type].modify_hp_amount;
		   
	if (combo_class_buf[c2.type].modify_hp_amount) bridgedetected = 0;
			   
	if(dmg)
	{
		if (bridgedetected <= 0)
		{
			for(int32_t k=0; k<16; k+=2)
				for(int32_t j=0; j<16; j+=2)
				if(((k+j)/2)%2)
					rectfill(dest,x+k,y+j,x+k+1,y+j+1,vc(14));
		}
		else
		{
			for(int32_t i=0; i<4; i++)
			{
				if (!(bridgedetected & (1<<i)))
				{
					int32_t tx=((i&2)<<2)+x;
					int32_t ty=((i&1)<<3)+y;
					for(int32_t k=0; k<8; k+=2)
						for(int32_t j=0; j<8; j+=2)
						if(((k+j)/2)%2)
							rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(14));
				}
			}
		}
	}
	
	if(c.type == cSLOPE)
	{
		slope_info s(c, x, y);
		s.draw(dest, 0, 0, COLOR_SLOPE);
	}
	auto fl0 = MAPFLAG2(-1,cx,cy);
	auto fl1 = MAPFLAG2(0,cx,cy);
	auto fl2 = MAPFLAG2(1,cx,cy);
	if(fl0 == mfSIDEVIEWLADDER || fl1 == mfSIDEVIEWLADDER || fl2 == mfSIDEVIEWLADDER
		|| c0.flag == mfSIDEVIEWLADDER || c1.flag == mfSIDEVIEWLADDER || c2.flag == mfSIDEVIEWLADDER)
	{
		bool top = false;
		if(cy)
		{
			top = true;
			if(combobuf[MAPCOMBO2(-1,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| combobuf[MAPCOMBO2(0,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| combobuf[MAPCOMBO2(1,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| MAPFLAG2(-1,cx,cy) == mfSIDEVIEWLADDER
				|| MAPFLAG2(0,cx,cy) == mfSIDEVIEWLADDER
				|| MAPFLAG2(1,cx,cy) == mfSIDEVIEWLADDER)
			{
				top = false;
			}
		}
		draw_ladder(dest,x,y,COLOR_LADDER,top);
	}
	else if(fl0 == mfSIDEVIEWPLATFORM || fl1 == mfSIDEVIEWPLATFORM || fl2 == mfSIDEVIEWPLATFORM
		|| c0.flag == mfSIDEVIEWPLATFORM || c1.flag == mfSIDEVIEWPLATFORM || c2.flag == mfSIDEVIEWPLATFORM)
	{
		draw_platform(dest,x,y,COLOR_LADDER);
	}
}

void zmap::put_walkflags_layered_external(BITMAP *dest,int32_t x,int32_t y,int32_t pos,int32_t layer, int32_t map, int32_t screen)
{
	int32_t cx = COMBOX(pos);
	int32_t cy = COMBOY(pos);
	
	if (screen < 0) return;
	if (map < 0) return;
	
	newcombo const& c = combobuf[MAPCOMBO3(map, screen, layer, pos)];
	
	if (c.type == cBRIDGE && get_qr(qr_OLD_BRIDGE_COMBOS)) return;
	
	int32_t bridgedetected = 0;
	for(int32_t i=0; i<4; i++)
	{
		int32_t tx=((i&2)<<2)+x;
		int32_t ty=((i&1)<<3)+y;
		int32_t tx2=((i&2)<<2)+cx;
		int32_t ty2=((i&1)<<3)+cy;
		for (int32_t m = layer; m <= 1; m++)
		{
			newcombo const& cmb = combobuf[MAPCOMBO3(map, screen, m,tx2,ty2)];
			if (get_qr(qr_OLD_BRIDGE_COMBOS))
			{
				if (cmb.type == cBRIDGE && !(cmb.walk&(1<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
			else
			{
				if (cmb.type == cBRIDGE && (cmb.walk&(0x10<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
		}
		if (bridgedetected & (1<<i))
		{
			continue;
		}
		if(!(c.walk&(1<<i) && ((c.usrflags&cflag3) || (c.usrflags&cflag4))) && (layer==-1 || (get_qr(qr_WATER_ON_LAYER_1) && layer == 0) || (get_qr(qr_WATER_ON_LAYER_2) && layer == 1)) && combo_class_buf[c.type].water!=0 && get_qr(qr_DROWN))
			rectfill(dest,tx,ty,tx+7,ty+7,vc(11));
		
			
		if ((c.walk&(1<<(i+4))) && ((c.walk&(1<<i) && ((c.usrflags&cflag4)) && c.type == cWATER) || c.type == cSHALLOWWATER)) 
		{
			for(int32_t k=0; k<8; k+=2)
				for(int32_t j=0; j<8; j+=2)
					if(((k+j)/2)%2) rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(11));
		}
		if(c.walk&(1<<i) && !(combo_class_buf[c.type].water!=0 && ((c.usrflags&cflag3) || (c.usrflags&cflag4))))
		{
			if(c.type==cLADDERHOOKSHOT && isstepable(MAPCOMBO3(map, screen, layer, cx,cy)) && ishookshottable(map, screen, cx,cy,i) && layer < 0)
			{
				for(int32_t k=0; k<8; k+=2)
					for(int32_t j=0; j<8; j+=2)
						rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(6+((k+j)/2)%2));
			}
			else
			{
				int32_t color = COLOR_SOLID;
				
				if(isstepable(MAPCOMBO3(map, screen, -1, cx,cy)) && (!get_qr(qr_NO_SOLID_SWIM) || combo_class_buf[combobuf[MAPCOMBO3(map, screen, -1, cx,cy)].type].water==0))
					color=vc(6);
				else if((c.type==cHOOKSHOTONLY || c.type==cLADDERHOOKSHOT) && ishookshottable(map, screen, cx,cy,i))
					color=vc(7);
					
				rectfill(dest,tx,ty,tx+7,ty+7,color);
			}
		}
	}
	
	bridgedetected = 0;
	for(int32_t i=0; i<4; i++)
	{
		int32_t tx2=((i&2)<<2)+cx;
		int32_t ty2=((i&1)<<3)+cy;
		for (int32_t m = 0; m <= 1; m++)
		{
			newcombo const& cmb = combobuf[MAPCOMBO3(map, screen, m,tx2,ty2)];
			if (get_qr(qr_OLD_BRIDGE_COMBOS))
			{
				if (cmb.type == cBRIDGE && !(cmb.walk&(1<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
			else
			{
				if (cmb.type == cBRIDGE && (cmb.walk&(0x10<<i))) 
				{
					bridgedetected |= (1<<i);
				}
			}
		}
	}
	
	// Draw damage combos
	newcombo const& c0 = combobuf[MAPCOMBO3(map, screen, -1,pos)];
	newcombo const& c1 = combobuf[MAPCOMBO3(map, screen, 0,pos)];
	newcombo const& c2 = combobuf[MAPCOMBO3(map, screen, 1,pos)];
	bool dmg = combo_class_buf[c0.type].modify_hp_amount
	   || combo_class_buf[c1.type].modify_hp_amount
	   || combo_class_buf[c2.type].modify_hp_amount;
		   
	if (combo_class_buf[c2.type].modify_hp_amount) bridgedetected = 0;
			   
	if(dmg)
	{
		if (bridgedetected <= 0)
		{
			for(int32_t k=0; k<16; k+=2)
				for(int32_t j=0; j<16; j+=2)
				if(((k+j)/2)%2)
					rectfill(dest,x+k,y+j,x+k+1,y+j+1,vc(14));
		}
		else
		{
			for(int32_t i=0; i<4; i++)
			{
				if (!(bridgedetected & (1<<i)))
				{
					int32_t tx=((i&2)<<2)+x;
					int32_t ty=((i&1)<<3)+y;
					for(int32_t k=0; k<8; k+=2)
						for(int32_t j=0; j<8; j+=2)
						if(((k+j)/2)%2)
							rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(14));
				}
			}
		}
	}
	
	if(c.type == cSLOPE)
	{
		slope_info s(c, x, y);
		s.draw(dest, 0, 0, COLOR_SLOPE);
	}
	auto fl0 = MAPFLAG3(map,screen,-1,pos);
	auto fl1 = MAPFLAG3(map,screen,0,pos);
	auto fl2 = MAPFLAG3(map,screen,1,pos);
	if(fl0 == mfSIDEVIEWLADDER || fl1 == mfSIDEVIEWLADDER || fl2 == mfSIDEVIEWLADDER
		|| c0.flag == mfSIDEVIEWLADDER || c1.flag == mfSIDEVIEWLADDER || c2.flag == mfSIDEVIEWLADDER)
	{
		bool top = false;
		if(cy)
		{
			top = true;
			if(combobuf[MAPCOMBO3(map,screen,-1,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| combobuf[MAPCOMBO3(map,screen,0,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| combobuf[MAPCOMBO3(map,screen,1,cx,cy-16)].flag == mfSIDEVIEWLADDER
				|| MAPFLAG3(map,screen,-1,cx,cy-16) == mfSIDEVIEWLADDER
				|| MAPFLAG3(map,screen,0,cx,cy-16) == mfSIDEVIEWLADDER
				|| MAPFLAG3(map,screen,1,cx,cy-16) == mfSIDEVIEWLADDER)
			{
				top = false;
			}
		}
		draw_ladder(dest,x,y,COLOR_LADDER,top);
	}
	else if(fl0 == mfSIDEVIEWPLATFORM || fl1 == mfSIDEVIEWPLATFORM || fl2 == mfSIDEVIEWPLATFORM
		|| c0.flag == mfSIDEVIEWPLATFORM || c1.flag == mfSIDEVIEWPLATFORM || c2.flag == mfSIDEVIEWPLATFORM)
	{
		draw_platform(dest,x,y,COLOR_LADDER);
	}
}

void put_walkflags(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t layer)
{
	const newcombo& c = combobuf[cmbdat];
	
	if (c.type == cBRIDGE && get_qr(qr_OLD_BRIDGE_COMBOS)) return;
	
	for(int32_t i=0; i<4; i++)
	{
		int32_t tx=((i&2)<<2)+x;
		int32_t ty=((i&1)<<3)+y;
		
		if(!(c.walk&(1<<i) && ((c.usrflags&cflag3) || (c.usrflags&cflag4))) && combo_class_buf[c.type].water!=0)
		{
			if ((layer==0 || (get_qr(qr_WATER_ON_LAYER_1) && layer == 1) || (get_qr(qr_WATER_ON_LAYER_2) && layer == 2)) && get_qr(qr_DROWN))
			{
				rectfill(dest,tx,ty,tx+7,ty+7,vc(11));
			}
			else
			{
				rectfill(dest,tx,ty,tx+7,ty+7,vc(11));
			}
		}
		
			
		if ((c.walk&(1<<(i+4))) && ((c.walk&(1<<i) && ((c.usrflags&cflag4)) && c.type == cWATER) || c.type == cSHALLOWWATER)) 
		{
			for(int32_t k=0; k<8; k+=2)
				for(int32_t j=0; j<8; j+=2)
					if(((k+j)/2)%2) rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(11));
		}
		if(c.walk&(1<<i) && !(combo_class_buf[c.type].water!=0 && ((c.usrflags&cflag3) || (c.usrflags&cflag4))))
		{
			if(c.type==cLADDERHOOKSHOT)
			{
				for(int32_t k=0; k<8; k+=2)
					for(int32_t j=0; j<8; j+=2)
						rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(6+((k+j)/2)%2));
			}
			else
			{
				int32_t color = COLOR_SOLID;
				
				if(c.type==cLADDERONLY)
					color=vc(6);
				else if(c.type==cHOOKSHOTONLY)
					color=vc(7);
					
				rectfill(dest,tx,ty,tx+7,ty+7,color);
			}
		}
		
		// Draw damage combos
		if(combo_class_buf[c.type].modify_hp_amount != 0)
		{
			for(int32_t k=0; k<8; k+=2)
				for(int32_t j=0; j<8; j+=2)
					if(((k+j)/2)%2) rectfill(dest,tx+k,ty+j,tx+k+1,ty+j+1,vc(4));
		}
	}
	
	if(c.type == cSLOPE)
	{
		slope_info s(c, 0, 0);
		zfix const& slope = s.slope();
		
		BITMAP* sub = create_bitmap_ex(8,16,16);
		clear_bitmap(sub);
		s.draw(sub, 0, 0, COLOR_SLOPE);
		masked_blit(sub, dest, 0, 0, x, y, 16, 16);
		destroy_bitmap(sub);
	}
	if(c.flag == mfSIDEVIEWLADDER)
	{
		draw_ladder(dest,x,y,COLOR_LADDER);
	}
	else if(c.flag == mfSIDEVIEWPLATFORM)
	{
		draw_platform(dest,x,y,COLOR_LADDER);
	}
}

void put_flag(BITMAP* dest, int32_t x, int32_t y, int32_t flag)
{
	rectfill(dest,x,y,x+15,y+15,vc(flag&15));
	textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+1,vc(15-(flag&15)),-1,"%d",flag);
}
void put_flags(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag)
{

	newcombo const& c = combobuf[cmbdat];
	
	if((flags&cFLAGS)&&(sflag||combobuf[cmbdat].flag))
	{
		//    rectfill(dest,x,y,x+15,y+15,vc(cmbdat>>10+1));
		//    text_mode(-1);
		//    textprintf_ex(dest,get_zc_font(font_sfont),x+1,y+1,(sflag)==0x7800?vc(0):vc(15),-1,"%d",sflag);
		if(sflag)
		{
			rectfill(dest,x,y,x+15,y+15,vc(sflag&15));
			textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+1,vc(15-(sflag&15)),-1,"%d",sflag);
		}
		
		if(c.flag)
		{
			rectfill(dest,x,y+(sflag?8:0),x+15,y+15,vc((combobuf[cmbdat].flag)&15));
			textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+9,vc(15-((combobuf[cmbdat].flag)&15)),-1,"%d",combobuf[cmbdat].flag);
		}
	}
	
	if(flags&cCSET)
	{
		bool inv = (((cmbdat&0x7800)==0x7800)&&(flags&cFLAGS));
		//    text_mode(inv?vc(15):vc(0));
		textprintf_ex(dest,get_zc_font(font_z3smallfont),x+9,y+9,inv?vc(0):vc(15),inv?vc(15):vc(0),"%d",cset);
	}
	else if(flags&cCTYPE)
	{
		bool inv = (((cmbdat&0x7800)==0x7800)&&(flags&cFLAGS));
		//    text_mode(inv?vc(15):vc(0));
		textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+9,inv?vc(0):vc(15),inv?vc(15):vc(0),"%d",c.type);
	}
}

void put_combo(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag,int32_t scale)
{
	bool repos = combotile_override_x < 0 && combotile_override_y < 0;
	
	BITMAP* b = create_bitmap_ex(8,scale*16,scale*16);
	if(repos)
	{
		combotile_override_x = x+(8*(scale-1));
		combotile_override_y = y+(8*(scale-1));
	}
	put_combo(b,0,0,cmbdat,cset,flags,sflag);
	if(repos) combotile_override_x = combotile_override_y = -1;
	masked_stretch_blit(b,dest,0,0,16,16,x,y,16*scale,16*scale);
	destroy_bitmap(b);
}
void put_combo(BITMAP *dest,int32_t x,int32_t y,word cmbdat,int32_t cset,int32_t flags,int32_t sflag)
{
	static newcombo nilcombo;
	nilcombo.tile = 0;
	
	newcombo const& c = cmbdat < MAXCOMBOS ? combobuf[cmbdat] : nilcombo;
	
	if(c.tile==0)
	{
		rectfill(dest,x,y,x+15,y+15,vc(0));
		rectfill(dest,x+3,y+3,x+12,y+12,vc(4));
		return;
	}
	
	putcombo(dest,x,y,cmbdat,cset);
	
	/* moved to put_walkflags
	  for(int32_t i=0; i<4; i++) {
	
	  int32_t tx=((i&2)<<2)+x;
	  int32_t ty=((i&1)<<3)+y;
	  if((flags&cWALK) && (c.walk&(1<<i)))
	  rectfill(dest,tx,ty,tx+7,ty+7,COLOR_SOLID);
	  }
	  */
	
	//  if((flags&cFLAGS)&&(cmbdat&0xF800))
	if((flags&cFLAGS)&&(sflag||combobuf[cmbdat].flag))
	{
		//    rectfill(dest,x,y,x+15,y+15,vc(cmbdat>>10+1));
		//    text_mode(-1);
		//    textprintf_ex(dest,get_zc_font(font_sfont),x+1,y+1,(sflag)==0x7800?vc(0):vc(15),-1,"%d",sflag);
		if(sflag)
		{
			rectfill(dest,x,y,x+15,y+15,vc(sflag&15));
			textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+1,vc(15-(sflag&15)),-1,"%d",sflag);
		}
		
		if(combobuf[cmbdat].flag)
		{
			rectfill(dest,x,y+(sflag?8:0),x+15,y+15,vc((combobuf[cmbdat].flag)&15));
			textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+1,vc(15-((combobuf[cmbdat].flag)&15)),-1,"%d",combobuf[cmbdat].flag);
		}
	}
	
	if(flags&cWALK)
	{
		put_walkflags(dest,x,y,cmbdat,0);
	}
	
	if(flags&cCSET)
	{
		bool inv = (((cmbdat&0x7800)==0x7800)&&(flags&cFLAGS));
		//    text_mode(inv?vc(15):vc(0));
		textprintf_ex(dest,get_zc_font(font_z3smallfont),x+9,y+9,inv?vc(0):vc(15),inv?vc(15):vc(0),"%d",cset);
	}
	else if(flags&cCTYPE)
	{
		bool inv = (((cmbdat&0x7800)==0x7800)&&(flags&cFLAGS));
		//    text_mode(inv?vc(15):vc(0));
		textprintf_ex(dest,get_zc_font(font_z3smallfont),x+1,y+9,inv?vc(0):vc(15),inv?vc(15):vc(0),"%d",c.type);
	}
}
void put_engraving(BITMAP* dest, int32_t x, int32_t y, int32_t slot, int32_t scale)
{
	auto blitx = 1 + (slot % 16) * 17;
	auto blity = 1 + (slot / 16) * 17;
	masked_stretch_blit(asset_engravings_bmp, dest, blitx, blity, 16, 16, x, y, 16 * scale, 16 * scale);
}


void copy_mapscr(mapscr *dest, const mapscr *src)
{
	if(!dest || !src) return;
	*dest = *src;
}

void zmap::put_door(BITMAP *dest,int32_t pos,int32_t side,int32_t type,int32_t xofs,int32_t yofs,bool ignorepos, int32_t scr)
{
    int32_t x=0,y=0;
    mapscr *doorscreen=(prv_mode?get_prvscr():screens+scr);
    
    switch(side)
    {
    case up:
    case down:
        x=((pos&15)<<4)+xofs;
        y=(ignorepos?0:(pos&0xF0))+yofs;
        break;
        
    case left:
    case right:
        x=(ignorepos?0:((pos&15)<<4))+xofs;
        y=(pos&0xF0)+yofs;
        break;
    }
    
    switch(type)
    {
    case dt_lock:
    case dt_shut:
    case dt_boss:
    case dt_bomb:
        switch(side)
        {
        case up:
            put_combo(dest,x,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_u[type][0],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_u[type][0],0,0);
            put_combo(dest,x+16,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_u[type][1],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_u[type][1],0,0);
            put_combo(dest,x,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_u[type][2],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_u[type][2],0,0);
            put_combo(dest,x+16,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_u[type][3],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_u[type][3],0,0);
            break;
            
        case down:
            put_combo(dest,x,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_d[type][0],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_d[type][0],0,0);
            put_combo(dest,x+16,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_d[type][1],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_d[type][1],0,0);
            put_combo(dest,x,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_d[type][2],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_d[type][2],0,0);
            put_combo(dest,x+16,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_d[type][3],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_d[type][3],0,0);
            break;
            
        case left:
            put_combo(dest,x,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][0],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][0],0,0);
            put_combo(dest,x,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][2],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][2],0,0);
            put_combo(dest,x,y+32,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][4],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][4],0,0);
                      
            if(x+16 >= dest->w)
                break;
                
            put_combo(dest,x+16,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][1],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][1],0,0);
            put_combo(dest,x+16,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][3],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][3],0,0);
            put_combo(dest,x+16,y+32,DoorComboSets[doorscreen->door_combo_set].doorcombo_l[type][5],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_l[type][5],0,0);
            break;
            
        case right:
        
            put_combo(dest,x+16,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][1],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][1],0,0);
            put_combo(dest,x+16,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][3],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][3],0,0);
            put_combo(dest,x+16,y+32,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][5],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][5],0,0);
                      
            if(x+16 <= 0)
                break;
                
            put_combo(dest,x,y,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][0],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][0],0,0);
            put_combo(dest,x,y+16,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][2],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][2],0,0);
            put_combo(dest,x,y+32,DoorComboSets[doorscreen->door_combo_set].doorcombo_r[type][4],
                      DoorComboSets[doorscreen->door_combo_set].doorcset_r[type][4],0,0);
            break;
        }
        
        break;
        
    case dt_pass:
    case dt_wall:
    case dt_walk:
    default:
        break;
    }
}

void zmap::over_door(BITMAP *dest,int32_t pos,int32_t side,int32_t xofs,int32_t yofs,bool, int32_t scr)
{
    int32_t x=((pos&15)<<4)+xofs;
    int32_t y=(pos&0xF0)+yofs;
    mapscr *doorscreen=(prv_mode?get_prvscr():screens+scr);
    
    
    switch(side)
    {
    case up:
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_u[0]!=0)
        {
            overcombo(dest,x,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_u[0],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_u[0]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_u[1]!=0)
        {
            overcombo(dest,x+16,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_u[1],
                      
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_u[1]);
        }
        
        break;
        
    case down:
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_d[0]!=0)
        {
            overcombo(dest,x,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_d[0],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_d[0]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_d[1]!=0)
        {
            overcombo(dest,x+16,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_d[1],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_d[1]);
        }
        
        break;
        
    case left:
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[0]!=0)
        {
            overcombo(dest,x,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[0],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_l[0]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[1]!=0)
        {
            overcombo(dest,x,y+16,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[1],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_l[1]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[2]!=0)
        {
            overcombo(dest,x,y+32,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_l[2],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_l[2]);
        }
        
        break;
        
    case right:
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[0]!=0)
        {
            overcombo(dest,x,y,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[0],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_r[0]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[1]!=0)
        {
            overcombo(dest,x,y+16,
            
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[1],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_r[1]);
        }
        
        if(DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[2]!=0)
        {
            overcombo(dest,x,y+32,
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcombo_r[2],
                      DoorComboSets[doorscreen->door_combo_set].bombdoorcset_r[2]);
        }
        
        break;
    }
}

bool zmap::misaligned(int32_t map, int32_t scr, int32_t i, int32_t dir)
{
	word cmbcheck1, cmbcheck2;
	newcombo combocheck1, combocheck2;
	combocheck1 = combobuf[0];
	combocheck2 = combobuf[0];
	combocheck1.walk = 0;
	combocheck2.walk = 0;
	
	int32_t layermap, layerscreen;
	
	switch(dir)
	{
		case up:
		{
			if(i>15)											  //not top row of combos
			{
				return false;
			}
			
			if(scr<16)											//top row of screens
			{
				return false;
				
			}
			
			//check main screen
			cmbcheck1 = vbound(AbsoluteScr(map, scr)->data[i], 0, MAXCOMBOS-1);
			cmbcheck2 = vbound(AbsoluteScr(map, scr-16)->data[i+160], 0, MAXCOMBOS-1);
			if (combobuf[cmbcheck1].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck1.walk|=combobuf[cmbcheck1].walk;
			if (combobuf[cmbcheck2].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck2.walk|=combobuf[cmbcheck2].walk;
			
			//check layer 1
			layermap=AbsoluteScr(map, scr)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[0];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr-16)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr-16)->layerscreen[0];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i+160];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			//check layer 2
			layermap=AbsoluteScr(map, scr)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[1];
				
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr-16)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr-16)->layerscreen[1];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i+160];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			if(((combocheck1.walk&5)*2)!=(combocheck2.walk&10))
			{
				return true;
			}
			
			break;
		}
		case down:
		{
			if(i<160)											 //not bottom row of combos
			{
				return false;
			}
			
			if(scr>111)										   //bottom row of screens
			{
				return false;
			}
			
			//check main screen
			cmbcheck1 = vbound(AbsoluteScr(map, scr)->data[i], 0, MAXCOMBOS-1);
			cmbcheck2 = vbound(AbsoluteScr(map, scr+16)->data[i-160], 0, MAXCOMBOS-1);
			if (combobuf[cmbcheck1].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck1.walk|=combobuf[cmbcheck1].walk;
			if (combobuf[cmbcheck2].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck2.walk|=combobuf[cmbcheck2].walk;
			
			
			//check layer 1
			layermap=AbsoluteScr(map, scr)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[0];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr+16)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr+16)->layerscreen[0];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i-160];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			//check layer 2
			layermap=AbsoluteScr(map, scr)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[1];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr+16)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr+16)->layerscreen[1];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i-160];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			if((combocheck1.walk&10)!=((combocheck2.walk&5)*2))
			{
				return true;
			}
			
			break;
		}
		case left:
		{
			if((i&0xF)!=0)										//not left column of combos
			{
				return false;
			}
			
			if((scr&0xF)==0)									  //left column of screens
			{
				return false;
			}
			
			//check main screen
			cmbcheck1 = AbsoluteScr(map, scr)->data[i];
			cmbcheck2 = AbsoluteScr(map, scr-1)->data[i+15];
			if (combobuf[cmbcheck1].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck1.walk|=combobuf[cmbcheck1].walk;
			if (combobuf[cmbcheck2].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck2.walk|=combobuf[cmbcheck2].walk;
			
			//check layer 1
			layermap=AbsoluteScr(map, scr)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[0];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr-1)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr-1)->layerscreen[0];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i+15];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			//check layer 2
			layermap=AbsoluteScr(map, scr)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[1];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr-1)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr-1)->layerscreen[1];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i+15];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			if(((combocheck1.walk&3)*4)!=(combocheck2.walk&12))
			{
				return true;
			}
			
			break;
		}
		case right:
		{
			if((i&0xF)!=15)									   //not right column of combos
			{
				return false;
			}
			
			if((scr&0xF)==15)									 //right column of screens
			{
				return false;
			}
			
			//check main screen
			cmbcheck1 = AbsoluteScr(map, scr)->data[i];
			cmbcheck2 = AbsoluteScr(map, scr+1)->data[i-15];
			if (combobuf[cmbcheck1].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck1.walk|=combobuf[cmbcheck1].walk;
			if (combobuf[cmbcheck2].type != cBRIDGE || !get_qr(qr_OLD_BRIDGE_COMBOS)) combocheck2.walk|=combobuf[cmbcheck2].walk;
			
			//check layer 1
			layermap=AbsoluteScr(map, scr)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[0];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr+1)->layermap[0]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr+1)->layerscreen[0];
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i-15];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			//check layer 2
			layermap=AbsoluteScr(map, scr)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr)->layerscreen[1];
				cmbcheck1 = AbsoluteScr(layermap, layerscreen)->data[i];
				if (combobuf[cmbcheck1].type == cBRIDGE) 
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck1].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck1].walk & 0xF);
						combocheck1.walk = ((newsolid | combocheck1.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck1.walk&=combobuf[cmbcheck1].walk;
				}
				else combocheck1.walk|=combobuf[cmbcheck1].walk;
			}
			
			layermap=AbsoluteScr(map, scr+1)->layermap[1]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=AbsoluteScr(map, scr+1)->layerscreen[1];
				
				cmbcheck2 = AbsoluteScr(layermap, layerscreen)->data[i-15];
				if (combobuf[cmbcheck2].type == cBRIDGE)
				{
					if (!get_qr(qr_OLD_BRIDGE_COMBOS))
					{
						int efflag = (combobuf[cmbcheck2].walk & 0xF0)>>4;
						int newsolid = (combobuf[cmbcheck2].walk & 0xF);
						combocheck2.walk = ((newsolid | combocheck2.walk) & (~efflag)) | (newsolid & efflag);
					}
					else combocheck2.walk&=combobuf[cmbcheck2].walk;
				}
				else combocheck2.walk|=combobuf[cmbcheck2].walk;
			}
			
			if((combocheck1.walk&12)!=((combocheck2.walk&3)*4))
			{
				return true;
			}
			
			break;
		}
	}
	
	return false;
}

void zmap::check_alignments(BITMAP* dest,int32_t x,int32_t y,int32_t scr)
{
    int32_t checkcombo;
    
    if(alignment_arrow_timer>31)
    {
        if(scr<0)
        {
            scr=cursor.screen;
        }
        
        if((scr<128))                                           //do the misalignment arrows
        {
            for(checkcombo=1; checkcombo<15; checkcombo++)        //check the top row (except the corners)
            {
                if(misaligned(cursor.map, scr, checkcombo, up))
                {
                    masked_blit(arrow_bmp[0],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                }
            }
            
            for(checkcombo=161; checkcombo<175; checkcombo++)     //check the top row (except the corners)
            {
                if(misaligned(cursor.map, scr, checkcombo, down))
                {
                    masked_blit(arrow_bmp[1],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                }
            }
            
            for(checkcombo=16; checkcombo<160; checkcombo+=16)    //check the left side (except the corners)
            {
                if(misaligned(cursor.map, scr, checkcombo, left))
                {
                    masked_blit(arrow_bmp[2],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                }
            }
            
            for(checkcombo=31; checkcombo<175; checkcombo+=16)    //check the right side (except the corners)
            {
                if(misaligned(cursor.map, scr, checkcombo, right))
                {
                    masked_blit(arrow_bmp[3],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                }
            }
            
            int32_t tempalign;
            
            //check top left corner
            checkcombo=0;
            tempalign=0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, up))?1:0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, left))?2:0;
            
            switch(tempalign)
            {
            case 0:
                break;
                
            case 1:                                             //up
                masked_blit(arrow_bmp[0],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 2:                                             //left
                masked_blit(arrow_bmp[2],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 3:                                             //up-left
                masked_blit(arrow_bmp[4],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
            }
            
            //check top right corner
            checkcombo=15;
            tempalign=0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, up))?1:0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, right))?2:0;
            
            switch(tempalign)
            {
            case 0:
                break;
                
            case 1:                                             //up
                masked_blit(arrow_bmp[0],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 2:                                             //right
                masked_blit(arrow_bmp[3],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 3:                                             //up-right
                masked_blit(arrow_bmp[5],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
            }
            
            //check bottom left corner
            checkcombo=160;
            tempalign=0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, down))?1:0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, left))?2:0;
            
            switch(tempalign)
            {
            case 0:
                break;
                
            case 1:                                             //down
                masked_blit(arrow_bmp[1],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 2:                                             //left
                masked_blit(arrow_bmp[2],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 3:                                             //down-left
                masked_blit(arrow_bmp[6],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
            }
            
            //check bottom right corner
            
            checkcombo=175;
            tempalign=0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, down))?1:0;
            tempalign+=(misaligned(cursor.map, scr, checkcombo, right))?2:0;
            
            switch(tempalign)
            {
            case 0:
                break;
                
            case 1:                                             //down
                masked_blit(arrow_bmp[1],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 2:                                             //right
                masked_blit(arrow_bmp[3],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
                
            case 3:                                             //down-right
                masked_blit(arrow_bmp[7],dest,0,0,((checkcombo&15)<<4)+x,(checkcombo&0xF0)+y,16,16);
                break;
            }
        }
    }
}

int32_t zmap::MAPCOMBO3(int32_t map, int32_t screen, int32_t layer, int32_t x,int32_t y)
{
	return MAPCOMBO3(map, screen, layer, COMBOPOS(x,y));
}

int32_t zmap::MAPCOMBO3(int32_t map, int32_t screen, int32_t layer, int32_t pos)
{ 
	if (map < 0 || screen < 0) return 0;
	
	if(pos>175 || pos < 0)
		return 0;
		
	mapscr const* m = &TheMaps[(map*MAPSCRS)+screen];
    
	if(m->valid==0) return 0;
	
	int32_t mapid = (layer < 0 ? -1 : ((m->layermap[layer] - 1) * MAPSCRS + m->layerscreen[layer]));
	
	if (layer >= 0 && (mapid < 0 || mapid > MAXMAPS*MAPSCRS)) return 0;
	
	mapscr const* scr = ((mapid < 0 || mapid > MAXMAPS*MAPSCRS) ? m : &TheMaps[mapid]);
    
	if(scr->valid==0) return 0;
		
	return scr->data[pos];						// entire combo code
}

// Takes array index layer num., not actual layer num.
int32_t zmap::MAPCOMBO2(int32_t lyr,int32_t x,int32_t y, int32_t map, int32_t scr)
{
    if(lyr<=-1) return MAPCOMBO(x,y,map,scr);
    
    if(map<0)
        map=cursor.map;
        
    if(scr<0)
        scr=cursor.screen;
        
    mapscr *screen1;
    
    if(prv_mode)
    {
        screen1=get_prvscr();
    }
    else
    {
        screen1=AbsoluteScr(cursor.map,cursor.screen);
    }
    
    int32_t layermap;
    layermap=screen1->layermap[lyr]-1;
    
    if(layermap<0 || layermap >= map_count) return 0;
    
    mapscr *layer;
    
    if(prv_mode)
        layer = &prvlayers[lyr];
    else
        layer = AbsoluteScr(layermap,screen1->layerscreen[lyr]);
        
    int32_t combo = COMBOPOS(x,y);
    
    if(combo>175 || combo < 0)
        return 0;
        
    return layer->data[combo];
}

int32_t zmap::MAPCOMBO(int32_t x,int32_t y, int32_t map, int32_t scr) //map=-1,scr=-1
{
    if(map<0)
        map=cursor.map;
        
    if(scr<0)
        scr=cursor.screen;
        
    mapscr *screen1;
    
    if(prv_mode)
    {
        screen1=get_prvscr();
    }
    else
    {
        screen1=AbsoluteScr(cursor.map,cursor.screen);
    }
    
    x = vbound(x, 0, 16*16);
    y = vbound(y, 0, 11*16);
    int32_t combo = COMBOPOS(x,y);
    
    if(combo>175 || combo < 0)
        return 0;
        
    return screen1->data[combo];
}

int32_t zmap::MAPFLAG3(int32_t map, int32_t screen, int32_t layer, int32_t x,int32_t y)
{
	return MAPFLAG3(map, screen, layer, COMBOPOS(x,y));
}

int32_t zmap::MAPFLAG3(int32_t map, int32_t screen, int32_t layer, int32_t pos)
{ 
	if (map < 0 || screen < 0) return 0;
	
	if(pos>175 || pos < 0)
		return 0;
		
	mapscr const* m = &TheMaps[(map*MAPSCRS)+screen];
    
	if(m->valid==0) return 0;
	
	int32_t mapid = (layer < 0 ? -1 : ((m->layermap[layer] - 1) * MAPSCRS + m->layerscreen[layer]));
	
	if (layer >= 0 && (mapid < 0 || mapid > MAXMAPS*MAPSCRS)) return 0;
	
	mapscr const* scr = ((mapid < 0 || mapid > MAXMAPS*MAPSCRS) ? m : &TheMaps[mapid]);
    
	if(scr->valid==0) return 0;
		
	return scr->sflag[pos];						// entire combo code
}

int32_t zmap::MAPFLAG2(int32_t lyr,int32_t x,int32_t y, int32_t map, int32_t scr)
{
    if(lyr<=-1) return MAPFLAG(x,y,map,scr);
    
    if(map<0)
        map=cursor.map;
        
    if(scr<0)
        scr=cursor.screen;
        
    mapscr *screen1;
    
    if(prv_mode)
    {
        screen1=get_prvscr();
    }
    else
    {
        screen1=AbsoluteScr(cursor.map,cursor.screen);
    }
    
    int32_t layermap;
    layermap=screen1->layermap[lyr]-1;
    
    if(layermap<0 || layermap >= map_count) return 0;
    
    mapscr *layer;
    
    if(prv_mode)
        layer = &prvlayers[lyr];
    else
        layer = AbsoluteScr(layermap,screen1->layerscreen[lyr]);
        
    int32_t combo = COMBOPOS(x,y);
    
    if(combo>175 || combo < 0)
        return 0;
        
    return layer->sflag[combo];
}

int32_t zmap::MAPFLAG(int32_t x,int32_t y, int32_t map, int32_t scr) //map=-1,scr=-1
{
    if(map<0)
        map=cursor.map;
        
    if(scr<0)
        scr=cursor.screen;
        
    mapscr *screen1;
    
    if(prv_mode)
    {
        screen1=get_prvscr();
    }
    else
    {
        screen1=AbsoluteScr(cursor.map,cursor.screen);
    }
    
    x = vbound(x, 0, 16*16);
    y = vbound(y, 0, 11*16);
    int32_t combo = COMBOPOS(x,y);
    
    if(combo>175 || combo < 0)
        return 0;
        
    return screen1->sflag[combo];
}

void zmap::draw_darkness(BITMAP* dest, BITMAP* transdest)
{
	mapscr *layers[7];
	mapscr *basescr;
	if(prv_mode)
	{
		layers[0] = &prvscr;
		basescr = layers[0];
		for(auto q = 1; q < 7; ++q)
		{
			if(prvlayers[q-1].valid)
				layers[q] = &(prvlayers[q-1]);
			else layers[q] = NULL;
		}
	}
	else
	{
		layers[0] = AbsoluteScr(cursor.map, cursor.screen);
		basescr = layers[0];
		for(auto q = 1; q < 7; ++q)
		{
			int32_t lmap = basescr->layermap[q-1]-1;
			int32_t lscr = basescr->layerscreen[q-1];
			if(lmap < 0)
				layers[q] = NULL;
			else layers[q] = AbsoluteScr(lmap, lscr);
		}
	}
	for(auto q = 0; q < 7; ++q)
	{
		if(!layers[q]) continue;
		for(auto pos = 0; pos < 176; ++pos)
		{
			newcombo const& cmb = combobuf[layers[q]->data[pos]];
			if(cmb.type == cTORCH)
				do_torch_combo(cmb, COMBOX(pos)+8, COMBOY(pos)+8, dest, transdest);
		}
	}
	word maxffc = basescr->numFFC();
	for(auto q = 0; q < maxffc; ++q)
	{
		newcombo const& cmb = combobuf[basescr->ffcs[q].data];
		if(cmb.type == cTORCH)
			do_torch_combo(cmb, (basescr->ffcs[q].x.getInt())+(basescr->ffEffectWidth(q)/2), (basescr->ffcs[q].y.getInt())+(basescr->ffEffectHeight(q)/2), dest, transdest);
	}
}

void drawcombo(BITMAP* dest, int32_t x, int32_t y, int32_t cid, int32_t cset, int32_t flags,
	int32_t sflag, bool over = true, bool transp = false, bool dither = false)
{
	newcombo const& cmb = combobuf[cid];
	if(cmb.animflags & AF_TRANSPARENT) transp = !transp;
	if(dither)
	{
		if (LayerDitherSz == 0)
			return;
		BITMAP* buf = create_bitmap_ex(8,16,16);
		clear_bitmap(buf);
		overcombo(buf,0,0,cid,cset);
		ditherblit(buf,nullptr,0,dithChecker,LayerDitherSz,x,y);
		if(over)
		{
			if(transp)
			{
				color_map = &trans_table2;
				draw_trans_sprite(dest, buf, x, y);
				color_map = &trans_table;
			}
			else masked_blit(buf, dest, 0, 0, x, y, 16, 16);
		}
		else masked_blit(buf, dest, 0, 0, x, y, 16, 16);
		destroy_bitmap(buf);
	}
	else if(over)
	{
		if(transp)
			overcombotranslucent(dest,x,y,cid,cset,0);
		else overcombo(dest,x,y,cid,cset);
	}
	else put_combo(dest,x,y,cid,cset,flags,sflag);
}
static void _zmap_drawlayer(BITMAP* dest, int32_t x, int32_t y, mapscr* md, int32_t flags, bool trans, bool over, bool dither, bool passflag = false)
{
	if(!md) return;
	for (int32_t i = 0; i < 176; i++)
		drawcombo(dest, ((i&15)<<4)+x, (i&0xF0)+y, md->data[i], md->cset[i], flags, passflag ? md->sflag[i] : 0, over, trans, dither);
}
static void _zmap_drawlayer_ohead(BITMAP* dest, int32_t x, int32_t y, mapscr* md, int32_t flags, bool trans, bool dither)
{
	if(!md) return;
	for (int32_t i = 0; i < 176; i++)
	{
		int data = md->data[i];
		if(combo_class_buf[combobuf[data].type].overhead)
			drawcombo(dest, ((i&15)<<4)+x, (i&0xF0)+y, data, md->cset[i], 0, 0, true, trans, dither);
	}
}
static mapscr* _zmap_get_lyr_checked(int lyr, mapscr* basescr)
{
	if(!LayerMaskInt[lyr])
		return nullptr;
	if(lyr == 0)
		return basescr;
	int layermap = basescr->layermap[lyr-1]-1;
	
	if(layermap>-1 && layermap<map_count)
	{
		int layerscreen = layermap*MAPSCRS+basescr->layerscreen[lyr-1];
		return &TheMaps[layerscreen];
	}
	return nullptr;
}
void zmap::draw(BITMAP* dest,int32_t x,int32_t y,int32_t flags,int32_t map,int32_t scr,int32_t hl_layer)
{
	#define HL_LAYER(lyr) (!(NoHighlightLayer0 && hl_layer == 0) && hl_layer > -1 && hl_layer != lyr)
	int32_t antiflags=(flags&~cFLAGS)&~cWALK;
	
	if(map<0)
		map=cursor.map;
		
	if(scr<0)
		scr=cursor.screen;
		
	mapscr *basescr;
	mapscr* layers[7] = {nullptr};
	
	if(prv_mode)
	{
		hl_layer = -1;
		basescr=get_prvscr();
	}
	else
	{
		basescr=AbsoluteScr(map,scr);
	}
	layers[0] = _zmap_get_lyr_checked(0,basescr);
	for(int lyr = 1; lyr < 7; ++lyr)
	{
		layers[lyr] = prv_mode ? ((&prvlayers[lyr-1])->valid ? &prvlayers[lyr - 1] : nullptr)
			: _zmap_get_lyr_checked(lyr,basescr);
	}
	
	int32_t layermap, layerscreen;
	if(CurrentLayer < 1)
		layermap = -1;
	else
		layermap=basescr->layermap[CurrentLayer-1]-1;
	
	if(!(basescr->valid&mVALID))
	{
		//  rectfill(dest,x,y,x+255,y+175,dvc(0+1));
		rectfill(dest,x,y,x+255,y+175,vc(1));
		
		if(ShowMisalignments)
		{
			check_alignments(dest,x,y,scr);
		}
		
		return;
	}
	
	if(LayerMaskInt[0]==0)
	{
		byte bgfill = 0;
		if (LayerDitherBG > -1)
			bgfill = vc(LayerDitherBG);
		rectfill(dest,x,y,x+255,y+175,bgfill);
	}
	
	if(XOR(basescr->flags7&fLAYER2BG,ViewLayer2BG))
		_zmap_drawlayer(dest, x, y, layers[2], antiflags, false, false, HL_LAYER(2));
	
	if(XOR(basescr->flags7&fLAYER3BG,ViewLayer3BG))
		_zmap_drawlayer(dest, x, y, layers[3], antiflags, basescr->layeropacity[3-1]!=255, XOR(basescr->flags7&fLAYER2BG,ViewLayer2BG), HL_LAYER(3));
	
	_zmap_drawlayer(dest, x, y, layers[0], antiflags, false, (XOR(basescr->flags7&fLAYER2BG,ViewLayer2BG)||XOR(basescr->flags7&fLAYER3BG,ViewLayer3BG)), HL_LAYER(0), true);
	
	_zmap_drawlayer(dest, x, y, layers[1], antiflags, basescr->layeropacity[1-1]!=255, true, HL_LAYER(1));
	
	int num_ffcs = basescr->numFFC();
	for(int32_t i=num_ffcs-1; i>=0; i--)
	{
		if(basescr->ffcs[i].data)
		{
			if(!(basescr->ffcs[i].flags&ffc_changer))
			{
				if(!(basescr->ffcs[i].flags&ffc_overlay))
				{
					int32_t tx=(basescr->ffcs[i].x.getInt())+x;
					int32_t ty=(basescr->ffcs[i].y.getInt())+y;
					
					if(basescr->ffcs[i].flags&ffc_trans)
					{
						overcomboblocktranslucent(dest, tx, ty, basescr->ffcs[i].data, basescr->ffcs[i].cset,basescr->ffTileWidth(i), basescr->ffTileHeight(i),128);
						//overtiletranslucent16(dest, combo_tile(basescr->ffcs[i].data,tx,ty)+(j*20)+(l), tx, ty, basescr->ffcs[i].cset, combobuf[basescr->ffcs[i].data].flip, 128);
					}
					else
					{
						overcomboblock(dest, tx, ty, basescr->ffcs[i].data, basescr->ffcs[i].cset, basescr->ffTileWidth(i), basescr->ffTileHeight(i));
						//overtile16(dest, combo_tile(basescr->ffcs[i].data,tx,ty)+(j*20)+(l), tx, ty, basescr->ffcs[i].cset, combobuf[basescr->ffcs[i].data].flip);
					}
				}
			}
		}
	}
	
	if(!XOR(basescr->flags7&fLAYER2BG,ViewLayer2BG))
		_zmap_drawlayer(dest, x, y, layers[2], antiflags, basescr->layeropacity[2-1]!=255, true, HL_LAYER(2));
	
	int32_t doortype[4];
	
	for(int32_t i=0; i<4; i++)
	{
		switch(basescr->door[i])
		{
		case dOPEN:
			doortype[i]=dt_pass;
			break;
			
		case dLOCKED:
			doortype[i]=dt_lock;
			break;
			
		case d1WAYSHUTTER:
		case dSHUTTER:
			doortype[i]=dt_shut;
			break;
			
		case dBOSS:
			doortype[i]=dt_boss;
			break;
			
		case dBOMB:
			doortype[i]=dt_bomb;
			break;
		}
	}
	
	switch(basescr->door[up])
	{
	case dBOMB:
		over_door(dest,39,up,x,y,false, scr);
		[[fallthrough]];
	case dOPEN:
	case dLOCKED:
	case d1WAYSHUTTER:
	case dSHUTTER:
	case dBOSS:
		put_door(dest,7,up,doortype[up],x,y,false,scr);
		break;
		
	case dWALK:
		if(get_bit(DoorComboSets[screens[cursor.screen].door_combo_set].flags,df_walktrans))
		{
			overcombo(dest,((23&15)<<4)+8+x,(23&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[0],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[0]);
		}
		else
		
		{
			put_combo(dest,((23&15)<<4)+8+x,(23&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[0],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[0],0,0);
		}
		
		break;
	}
	
	switch(basescr->door[down])
	{
	case dBOMB:
		over_door(dest,135,down,x,y,false,scr);
		[[fallthrough]];
	case dOPEN:
	case dLOCKED:
	case d1WAYSHUTTER:
	case dSHUTTER:
	case dBOSS:
		put_door(dest,151,down,doortype[down],x,y,false,scr);
		break;
		
	case dWALK:
		if(get_bit(DoorComboSets[screens[cursor.screen].door_combo_set].flags,df_walktrans))
		{
			overcombo(dest,((151&15)<<4)+8+x,(151&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[1],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[1]);
		}
		else
		{
			put_combo(dest,((151&15)<<4)+8+x,(151&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[1],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[1],0,0);
		}
		
		break;
	}
	
	switch(basescr->door[left])
	{
	case dBOMB:
		over_door(dest,66,left,x,y,false,scr);
		[[fallthrough]];
	case dOPEN:
	case dLOCKED:
	case d1WAYSHUTTER:
	case dSHUTTER:
	case dBOSS:
		put_door(dest,64,left,doortype[left],x,y,false,scr);
		break;
		
	case dWALK:
		if(get_bit(DoorComboSets[screens[cursor.screen].door_combo_set].flags,df_walktrans))
		{
			overcombo(dest,((81&15)<<4)+x,(81&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[2],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[2]);
		}
		else
		{
			put_combo(dest,((81&15)<<4)+x,(81&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[2],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[2],0,0);
		}
		
		break;
	}
	
	switch(basescr->door[right])
	{
	
	case dBOMB:
		over_door(dest,77,right,x,y,false,scr);
		[[fallthrough]];
	case dOPEN:
	case dLOCKED:
	case d1WAYSHUTTER:
	case dSHUTTER:
	case dBOSS:
		put_door(dest,78,right,doortype[right],x,y,false,scr);
		break;
		
	case dWALK:
		if(get_bit(DoorComboSets[screens[cursor.screen].door_combo_set].flags,df_walktrans))
		{
			overcombo(dest,((94&15)<<4)+x,(94&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[3],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[3]);
		}
		else
		{
			put_combo(dest,((94&15)<<4)+x,(94&0xF0)+y,
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcombo[3],
					  DoorComboSets[screens[cursor.screen].door_combo_set].walkthroughcset[3],0,0);
		}
		
		break;
	}
	
	if((basescr->hasitem != 0) && !(flags&cNOITEM))
	{
		putitem2(dest,basescr->itemx+x,basescr->itemy+y+1-(get_qr(qr_NOITEMOFFSET)),basescr->item,lens_hint_item[basescr->item][0],lens_hint_item[basescr->item][1], 0);
	}
	
	if(!XOR(basescr->flags7&fLAYER3BG,ViewLayer3BG))
		_zmap_drawlayer(dest, x, y, layers[3], antiflags, basescr->layeropacity[3-1]!=255, true, HL_LAYER(3));
	_zmap_drawlayer(dest, x, y, layers[4], antiflags, basescr->layeropacity[4-1]!=255, true, HL_LAYER(4));
	
	_zmap_drawlayer_ohead(dest, x, y, layers[0], antiflags, false, HL_LAYER(0));
	
	if(get_qr(qr_OVERHEAD_COMBOS_L1_L2))
	{
		_zmap_drawlayer_ohead(dest, x, y, layers[1], antiflags, basescr->layeropacity[1-1]!=255, HL_LAYER(1));
		_zmap_drawlayer_ohead(dest, x, y, layers[2], antiflags, basescr->layeropacity[2-1]!=255, HL_LAYER(2));
	}
	_zmap_drawlayer(dest, x, y, layers[5], antiflags, basescr->layeropacity[5-1]!=255, true, HL_LAYER(5));
	
	for(int32_t i=num_ffcs-1; i>=0; i--)
	{
		if(basescr->ffcs[i].data)
		{
			if(!(basescr->ffcs[i].flags&ffc_changer))
			{
				int32_t tx=(basescr->ffcs[i].x.getInt())+x;
				int32_t ty=(basescr->ffcs[i].y.getInt())+y;
				
				if(basescr->ffcs[i].flags&ffc_overlay)
				{
					if(basescr->ffcs[i].flags&ffc_trans)
					{
						//overtiletranslucent16(dest, combo_tile(basescr->ffcs[i].data,tx,ty)+(j*20)+(l), tx, ty, basescr->ffcs[i].cset, combobuf[basescr->ffcs[i].data].flip, 128);
						overcomboblocktranslucent(dest,tx,ty,basescr->ffcs[i].data, basescr->ffcs[i].cset, basescr->ffTileWidth(i), basescr->ffTileHeight(i),128);
					}
					else
					{
						//overtile16(dest, combo_tile(basescr->ffcs[i].data,tx,ty)+(j*20)+(l), tx, ty, basescr->ffcs[i].cset, combobuf[basescr->ffcs[i].data].flip);
						overcomboblock(dest, tx, ty, basescr->ffcs[i].data, basescr->ffcs[i].cset, basescr->ffTileWidth(i), basescr->ffTileHeight(i));
					}
				}
			}
		}
	}
	
	_zmap_drawlayer(dest, x, y, layers[6], antiflags, basescr->layeropacity[6-1]!=255, true, HL_LAYER(6));
	
	for(int32_t i=num_ffcs-1; i>=0; i--)
		if(basescr->ffcs[i].data)
			if(basescr->ffcs[i].flags&ffc_changer)
				putpixel(dest,(basescr->ffcs[i].x.getInt())+x,(basescr->ffcs[i].y.getInt())+y,vc(zc_oldrand()%16));
	
	if(flags&cWALK)
	{
		if(layers[0])
			for(int32_t i=0; i<176; i++)
				put_walkflags(dest,((i&15)<<4)+x,(i&0xF0)+y,layers[0]->data[i], 0);
		
		for(int32_t k=0; k<2; k++)
		{
			if(layers[k+1])
				for(int32_t i=0; i<176; i++)
					put_walkflags(dest,((i&15)<<4)+x,(i&0xF0)+y,layers[k+1]->data[i], 0);
		}
		for(int32_t i=num_ffcs-1; i>=0; i--)
		{
			if(auto data = basescr->ffcs[i].data)
			{
				if(!(basescr->ffcs[i].flags&ffc_changer))
				{
					newcombo const& cmb = combobuf[data];
					int32_t tx=(basescr->ffcs[i].x.getInt())+x;
					int32_t ty=(basescr->ffcs[i].y.getInt())+y;
					
					if(basescr->ffcs[i].flags&ffc_solid)
					{
						rectfill(dest, tx, ty, tx + basescr->ffEffectWidth(i) - 1, ty + basescr->ffEffectHeight(i) - 1, COLOR_SOLID);
					}
					
					if(cmb.type == cSLOPE)
					{
						slope_info s(cmb, tx, ty);
						s.draw(dest, 0, 0, COLOR_SLOPE);
					}
				}
			}
		}
	}
	
	if(flags&cFLAGS)
	{
		if(LayerMaskInt[CurrentLayer]!=0)
		{
			for(int32_t i=0; i<176; i++)
			{
				if(CurrentLayer==0)
				{
					put_flags(dest,((i&15)<<4)+x,(i&0xF0)+y,basescr->data[i],basescr->cset[i],flags,basescr->sflag[i]);
				}
				else
				{
					if(prv_mode)
					{
						put_flags(dest,((i&15)<<4)+x,(i&0xF0)+y,prvlayers[CurrentLayer-1].data[i],prvlayers[CurrentLayer-1].cset[i],flags,prvlayers[CurrentLayer-1].sflag[i]);
					}
					else if(basescr->layermap[CurrentLayer-1] > 0)
					{
						int32_t _lscr=(basescr->layermap[CurrentLayer-1]-1)*MAPSCRS+basescr->layerscreen[CurrentLayer-1];
						
						if(_lscr>-1 && _lscr<map_count*MAPSCRS)
						{
							put_flags(dest,((i&15)<<4)+x,(i&0xF0)+y,
									  TheMaps[_lscr].data[i],
									  TheMaps[_lscr].cset[i], flags,
									  TheMaps[_lscr].sflag[i]);
						}
					}
				}
			}
		}
	}
	
	int32_t dark = basescr->flags&cDARK;
	
	if(dark && !(flags&cNODARK)
		&& !((Flags&cNEWDARK) && get_qr(qr_NEW_DARKROOM)))
	{
		for(int32_t j=0; j<80; j++)
		{
			for(int32_t i=0; i<(80)-j; i++)
			{
				if(((i^j)&1)==0)
				{
					putpixel(dest,x+i,y+j,vc(blackout_color));
				}
			}
		}
	}
	
	if(ShowMisalignments)
	{
		check_alignments(dest,x,y,scr);
	}
}

void zmap::drawrow(BITMAP* dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr)
{
	if(map<0)
		map=cursor.map;
		
	if(scr<0)
		scr=cursor.screen;
		
	mapscr* layer=AbsoluteScr(map,scr);
	int32_t layermap=0, layerscreen=0;
	
	if(!(layer->valid&mVALID))
	{
		//  rectfill(dest,x,y,x+255,y+15,dvc(0+1));
		rectfill(dest,x,y,x+255,y+15,vc(1));
		return;
	}
	
	int32_t dark = layer->flags&4;
	
	if(LayerMaskInt[0]==0)
	{
		rectfill(dest,x,y,x+255,y+15,0);
	}
	
	
	for(int32_t k=1; k<3; k++)
	{
		if(LayerMaskInt[k+1]!=0 && (k==1)?(layer->flags7&fLAYER2BG):(layer->flags7&fLAYER3BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[2-1];
				
				for(int32_t i=c; i<(c&0xF0)+16; i++)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,((i&15)<<4)+x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	if(LayerMaskInt[0]!=0)
	{
		for(int32_t i=c; i<(c&0xF0)+16; i++)
		{
			word cmbdat = (i < 176 ? layer->data[i] : 0);
			byte cmbcset = (i < 176 ? layer->cset[i] : 0);
			int32_t cmbflag = (i < 176 ? layer->sflag[i] : 0);
			drawcombo(dest,((i&15)<<4)+x,y,cmbdat,cmbcset,((flags|dark)&~cWALK),
				cmbflag,(layer->flags7&fLAYER3BG||layer->flags7&fLAYER2BG));
		}
	}
	
	for(int32_t k=0; k<2; k++)
	{
		if(LayerMaskInt[k+1]!=0 && !(k==1 && layer->flags7&fLAYER2BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<(c&0xF0)+16; i++)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,((i&15)<<4)+x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	int32_t doortype[4];
	
	for(int32_t i=0; i<4; i++)
	{
		switch(layer->door[i])
		{
		case dOPEN:
			doortype[i]=dt_pass;
			break;
			
		case dLOCKED:
			doortype[i]=dt_lock;
			break;
			
		case d1WAYSHUTTER:
		case dSHUTTER:
			doortype[i]=dt_shut;
			break;
			
		case dBOSS:
			doortype[i]=dt_boss;
			break;
			
		case dBOMB:
			doortype[i]=dt_bomb;
			break;
		}
	}
	
	if(c<16)
	{
		switch(layer->door[up])
		{
		case dBOMB:
		case dOPEN:
		case dLOCKED:
		case d1WAYSHUTTER:
		case dSHUTTER:
		case dBOSS:
			put_door(dest,7,up,doortype[up],x,y+176,true,scr);
			break;
		}
	}
	else if(c>159)
	{
		switch(layer->door[down])
		{
		case dBOMB:
		case dOPEN:
		case dLOCKED:
		case d1WAYSHUTTER:
		case dSHUTTER:
		case dBOSS:
			put_door(dest,151,down,doortype[down],x,y-16,true,scr);
			break;
		}
	}
	
	for(int32_t k=2; k<4; k++)
	{
		if(LayerMaskInt[k+1]!=0 && !(k==2 && layer->flags7&fLAYER3BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<(c&0xF0)+16; i++)
				{
					if(layer->layeropacity[k]<255)
					{
						overcombotranslucent(dest,((i&15)<<4)+x,y,TheMaps[layerscreen].data[i],TheMaps[layerscreen].cset[i],layer->layeropacity[k]);
					}
					else
					{
						overcombo(dest,((i&15)<<4)+x,y,TheMaps[layerscreen].data[i],TheMaps[layerscreen].cset[i]);
					}
				}
			}
		}
	}
	
	//Overhead L0
	if(LayerMaskInt[0]!=0)
	{
		for(int32_t i=c; i<(c&0xF0)+16; i++)
		{
			int32_t ct1=layer->data[i];
			int32_t ct3=combobuf[ct1].type;
			
			if(combo_class_buf[ct3].overhead)
			{
				drawcombo(dest,((i&15)<<4)+x,y,layer->data[i],layer->cset[i],0,0);
			}
		}
	}
	
	//Overhead L1/2
	if(get_qr(qr_OVERHEAD_COMBOS_L1_L2))
	{
		for(int32_t k = 0; k < 2; ++k)
		{
			if(LayerMaskInt[k+1]!=0)
			{
				layermap=layer->layermap[k]-1;
				
				if(layermap>-1 && layermap<map_count)
				{
					layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
					for(int32_t i=c; i<(c&0xF0)+16; i++)
					{
						auto data = TheMaps[layerscreen].data[i];
						if(!combo_class_buf[combobuf[data].type].overhead) continue;
						auto cs = TheMaps[layerscreen].cset[i];
						drawcombo(dest,((i&15)<<4)+x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
					}
				}
			}
		}
	}
	
	for(int32_t k=4; k<6; k++)
	{
		if(LayerMaskInt[k+1]!=0)
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<(c&0xF0)+16; i++)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,((i&15)<<4)+x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	if(flags&cWALK)
	{
		if(LayerMaskInt[0]!=0)
		{
			for(int32_t i=c; i<(c&0xF0)+16; i++)
			{
				put_walkflags_layered_external(dest,((i&15)<<4)+x,y,i, -1, map,scr);
			}
		}
		
		for(int32_t k=0; k<2; k++)
		{
			if(LayerMaskInt[k+1]!=0)
			{ 
				for(int32_t i=c; i<(c&0xF0)+16; i++)
				{
					put_walkflags_layered_external(dest,((i&15)<<4)+x,y,i, k, map,scr);
				}
			}
		}
	}
	
	if(flags&cFLAGS)
	{
		if(LayerMaskInt[CurrentLayer]!=0)
		{
			for(int32_t i=c; i<(c&0xF0)+16; i++)
			{
				if(CurrentLayer==0)
				{
					put_flags(dest,((i&15)<<4)+x,/*(i&0xF0)+*/y,layer->data[i],layer->cset[i],flags|dark,layer->sflag[i]);
				}
				else
				{
					int32_t _lscr=(layer->layermap[CurrentLayer-1]-1)*MAPSCRS+layer->layerscreen[CurrentLayer-1];
					
					if(_lscr>-1 && _lscr<map_count*MAPSCRS)
					{
						if(i < 176)
						{
							put_flags(dest,((i&15)<<4)+x,/*(i&0xF0)+*/y,
									  TheMaps[_lscr].data[i],
									  TheMaps[_lscr].cset[i], flags|dark,
									  TheMaps[_lscr].sflag[i]);
						}
						else
						{
							put_flags(dest,((i&15)<<4)+x,/*(i&0xF0)+*/y,0,0, flags|dark,0);
						}
					}
				}
			}
		}
		
		/*
		  if (LayerMaskInt[0]!=0) {
		  for(int32_t i=c; i<(c&0xF0)+16; i++) {
		  put_flags(dest,((i&15)<<4)+x,y,layer->data[i],layer->cset[i],flags|dark,layer->sflag[i]);
		  }
		  }
		  */
	}
	
	if(ShowMisalignments)
	{
		if(c<16)
		{
			check_alignments(dest,x,y,scr);
		}
		else if(c>159)
		{
			check_alignments(dest,x,y-160,scr);
		}
	}
}

void zmap::drawcolumn(BITMAP* dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr)
{
	if(map<0)
		map=cursor.map;
		
	if(scr<0)
		scr=cursor.screen;
		
	mapscr* layer=AbsoluteScr(map,scr);
	int32_t layermap=0, layerscreen=0;
	
	if(!(layer->valid&mVALID))
	{
		//  rectfill(dest,x,y,x+15,y+175,dvc(0+1));
		rectfill(dest,x,y,x+15,y+175,vc(1));
		return;
	}
	
	int32_t dark = layer->flags&4;
	
	if(LayerMaskInt[0]==0)
	{
		rectfill(dest,x,y,x+15,y+175,0);
	}
	
	
	for(int32_t k=1; k<3; k++)
	{
		if(LayerMaskInt[k+1]!=0 && (k==1)?(layer->flags7&fLAYER2BG):(layer->flags7&fLAYER3BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[2-1];
				
				for(int32_t i=c; i<176; i+=16)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	if(LayerMaskInt[0]!=0)
	{
		for(int32_t i=c; i<176; i+=16)
		{
			word cmbdat = layer->data[i];
			byte cmbcset = layer->cset[i];
			int32_t cmbflag = layer->sflag[i];
			drawcombo(dest,x,(i&0xF0)+y,cmbdat,cmbcset,((flags|dark)&~cWALK),cmbflag,
				(layer->flags7&fLAYER3BG||layer->flags7&fLAYER2BG));
		}
	}
	
	for(int32_t k=0; k<2; k++)
	{
		if(LayerMaskInt[k+1]!=0 && !(k==1 && layer->flags7&fLAYER2BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<176; i+=16)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	int32_t doortype[4];
	
	for(int32_t i=0; i<4; i++)
	{
		switch(layer->door[i])
		{
		case dOPEN:
			doortype[i]=dt_pass;
			break;
			
		case dLOCKED:
			doortype[i]=dt_lock;
			break;
			
		case d1WAYSHUTTER:
		case dSHUTTER:
			doortype[i]=dt_shut;
			break;
			
		case dBOSS:
			doortype[i]=dt_boss;
			break;
			
		case dBOMB:
			doortype[i]=dt_bomb;
			break;
		}
	}
	
	if((c&0x0F)==0)
	{
		switch(layer->door[left])
		{
		
		case dBOMB:
		case dOPEN:
		case dLOCKED:
		case d1WAYSHUTTER:
		case dSHUTTER:
		case dBOSS:
			//       put_door(dest,64,left,doortype[left],x+256,y,true);
			put_door(dest,64,left,doortype[left],x,y,true,scr);
			break;
		}
	}
	else if((c&0x0F)==15)
	{
		switch(layer->door[right])
		{
		case dBOMB:
		case dOPEN:
		case dLOCKED:
		case d1WAYSHUTTER:
		case dSHUTTER:
		case dBOSS:
			put_door(dest,78,right,doortype[right],x-16,y,true,scr);
			break;
		}
	}
	
	for(int32_t k=2; k<4; k++)
	{
		if(LayerMaskInt[k+1]!=0 && !(k==2 && layer->flags7&fLAYER3BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<176; i+=16)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	//Overhead L0
	if(LayerMaskInt[0]!=0)
	{
		for(int32_t i=c; i<176; i+=16)
		{
			auto data = TheMaps[layerscreen].data[i];
			if(!combo_class_buf[combobuf[data].type].overhead) continue;
			auto cs = TheMaps[layerscreen].cset[i];
			drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0);
		}
	}
	//Overhead L1/2
	if(get_qr(qr_OVERHEAD_COMBOS_L1_L2))
	{
		for(int32_t k = 0; k < 2; ++k)
		{
			if(LayerMaskInt[k+1]!=0)
			{
				layermap=layer->layermap[k]-1;
				
				if(layermap>-1 && layermap<map_count)
				{
					layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
					for(int32_t i=c; i<176; i+=16)
					{
						auto data = TheMaps[layerscreen].data[i];
						if(!combo_class_buf[combobuf[data].type].overhead) continue;
						auto cs = TheMaps[layerscreen].cset[i];
						drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0,true,layer->layeropacity[k]!=255);
					}
				}
			}
		}
	}
	
	
	for(int32_t k=4; k<6; k++)
	{
		if(LayerMaskInt[k+1]!=0)
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				for(int32_t i=c; i<176; i+=16)
				{
					auto data = TheMaps[layerscreen].data[i];
					auto cs = TheMaps[layerscreen].cset[i];
					drawcombo(dest,x,(i&0xF0)+y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	if(flags&cWALK)
	{
		if(LayerMaskInt[0]!=0)
		{
			for(int32_t i=c&0xF; i<176; i+=16)
			{
				put_walkflags_layered_external(dest,x,y+(i&0xF0),i, -1, map,scr);
			}
		}
		
		for(int32_t k=0; k<2; k++)
		{
			if(LayerMaskInt[k+1]!=0)
			{
				for(int32_t i=c&0xF; i<176; i+=16)
				{
					put_walkflags_layered_external(dest,x,y+(i&0xF0),i, k, map,scr);
				}
			}
		}
	}
	
	if(flags&cFLAGS)
	{
		if(LayerMaskInt[CurrentLayer]!=0)
		{
			for(int32_t i=c; i<176; i+=16)
			{
				if(CurrentLayer==0)
				{
					put_flags(dest,/*((i&15)<<4)+*/x,(i&0xF0)+y,layer->data[i],layer->cset[i],flags|dark,layer->sflag[i]);
				}
				else
				{
					int32_t _lscr=(layer->layermap[CurrentLayer-1]-1)*MAPSCRS+layer->layerscreen[CurrentLayer-1];
					
					if(_lscr>-1 && _lscr<map_count*MAPSCRS)
					{
						put_flags(dest,/*((i&15)<<4)+*/x,(i&0xF0)+y,
								  TheMaps[_lscr].data[i],
								  TheMaps[_lscr].cset[i], flags|dark,
								  TheMaps[_lscr].sflag[i]);
					}
				}
			}
		}
	}
	
	if(ShowMisalignments)
	{
		if((c&0x0F)==0)
		{
			check_alignments(dest,x,y,scr);
		}
		else if((c&0x0F)==15)
		{
			check_alignments(dest,x-240,y,scr);
		}
	}
}

void zmap::drawblock(BITMAP* dest,int32_t x,int32_t y,int32_t flags,int32_t c,int32_t map,int32_t scr)
{
	if(map<0)
		map=cursor.map;
		
	if(scr<0)
		scr=cursor.screen;
		
	mapscr* layer=AbsoluteScr(map,scr);
	int32_t layermap=0, layerscreen=0;
	
	if(!(layer->valid&mVALID))
	{
		//  rectfill(dest,x,y,x+15,y+15,dvc(0+1));
		rectfill(dest,x,y,x+15,y+15,vc(1));
		return;
	}
	
	int32_t dark = layer->flags&4;
	
	if(LayerMaskInt[0]!=0)
	{
		rectfill(dest,x,y,x+15,y+15,0);
	}
	
	for(int32_t k=1; k<3; k++)
	{
		if(LayerMaskInt[k+1]!=0 && (k==1)?(layer->flags7&fLAYER2BG):(layer->flags7&fLAYER3BG))
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[2-1];
				
				auto data = TheMaps[layerscreen].data[c];
				auto cs = TheMaps[layerscreen].cset[c];
				drawcombo(dest,x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
			}
		}
	}
	
	if(LayerMaskInt[0]!=0)
	{
		word cmbdat = layer->data[c];
		byte cmbcset = layer->cset[c];
		int32_t cmbflag = layer->sflag[c];
		drawcombo(dest,x,y,cmbdat,cmbcset,((flags|dark)&~cWALK),cmbflag,
			(layer->flags7&fLAYER3BG||layer->flags7&fLAYER2BG));
	}
	
	
	for(int32_t k=0; k<2; k++)
	{
		if(LayerMaskInt[k+1]!=0)
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				
				auto data = TheMaps[layerscreen].data[c];
				auto cs = TheMaps[layerscreen].cset[c];
				drawcombo(dest,x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
			}
		}
	}
	
	for(int32_t k=2; k<4; k++)
	{
		if(LayerMaskInt[k+1]!=0)
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				auto data = TheMaps[layerscreen].data[c];
				auto cs = TheMaps[layerscreen].cset[c];
				drawcombo(dest,x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
			}
		}
	}
	
	//Overhead L0
	if(LayerMaskInt[0]!=0)
	{
		auto data = TheMaps[layerscreen].data[c];
		if(combo_class_buf[combobuf[data].type].overhead)
		{
			auto cs = TheMaps[layerscreen].cset[c];
			drawcombo(dest,x,y,data,cs,0,0);
		}
	}
	//Overhead L1/2
	if(get_qr(qr_OVERHEAD_COMBOS_L1_L2))
	{
		for(int32_t k = 0; k < 2; ++k)
		{
			if(LayerMaskInt[k+1]!=0)
			{
				layermap=layer->layermap[k]-1;
				
				if(layermap>-1 && layermap<map_count)
				{
					layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
					auto data = TheMaps[layerscreen].data[c];
					if(!combo_class_buf[combobuf[data].type].overhead) continue;
					auto cs = TheMaps[layerscreen].cset[c];
					drawcombo(dest,x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
				}
			}
		}
	}
	
	
	for(int32_t k=4; k<6; k++)
	{
		if(LayerMaskInt[k+1]!=0)
		{
			layermap=layer->layermap[k]-1;
			
			if(layermap>-1 && layermap<map_count)
			{
				layerscreen=layermap*MAPSCRS+layer->layerscreen[k];
				auto data = TheMaps[layerscreen].data[c];
				auto cs = TheMaps[layerscreen].cset[c];
				drawcombo(dest,x,y,data,cs,0,0,true,layer->layeropacity[k]!=255);
			}
		}
	}
	
	if(flags&cWALK)
	{
		if(LayerMaskInt[0]!=0)
		{
			put_walkflags_layered_external(dest,x,y,c,-1, map,scr);
		}
		
		for(int32_t k=0; k<2; k++)
		{
			if(LayerMaskInt[k+1]!=0)
			{
				put_walkflags_layered_external(dest,x,y,c,k, map,scr);
			}
		}
	}
	
	if(flags&cFLAGS)
	{
		if(LayerMaskInt[CurrentLayer]!=0)
		{
			int32_t i = c;
			//for(int32_t i=c; i==c; i++)
			{
				if(CurrentLayer==0)
				{
					put_flags(dest,/*((i&15)<<4)+*/x,/*(i&0xF0)+*/y,layer->data[i],layer->cset[i],flags|dark,layer->sflag[i]);
				}
				else
				{
					int32_t _lscr=(layer->layermap[CurrentLayer-1]-1)*MAPSCRS+layer->layerscreen[CurrentLayer-1];
					
					if(_lscr>-1 && _lscr<map_count*MAPSCRS)
					{
						put_flags(dest,/*((i&15)<<4)+*/x,/*(i&0xF0)+*/y,
								  TheMaps[_lscr].data[i],
								  TheMaps[_lscr].cset[i], flags|dark,
								  TheMaps[_lscr].sflag[i]);
					}
				}
			}
		}
	}
	
	if(ShowMisalignments)
	{
		switch(c)
		{
		case 0:
			check_alignments(dest,x,y,scr);
			break;
			
		case 15:
			check_alignments(dest,x-240,y,scr);
			break;
			
		case 160:
			check_alignments(dest,x,y-160,scr);
			break;
			
		case 175:
			check_alignments(dest,x-240,y-160,scr);
			break;
		}
	}
}

void zmap::drawstaticblock(BITMAP* dest,int32_t x,int32_t y)
{
	if (InvalidBG == 2)
	{
		draw_checkerboard(dest, x, y, 16);
	}
	else if(InvalidBG == 1)
    {
        for(int32_t dy=0; dy<16; dy++)
        {
            for(int32_t dx=0; dx<16; dx++)
            {
                dest->line[y+dy][x+dx]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
            }
        }
    }
    else
    {
        rectfill(dest, x, y, x+15, y+15, vc(0));
        rect(dest, x, y, x+15, y+15, vc(15));
        line(dest, x, y, x+15, y+15, vc(15));
        line(dest, x, y+15, x+15, y, vc(15));
    }
}

void zmap::drawstaticcolumn(BITMAP* dest,int32_t x,int32_t y)
{
	if (InvalidBG == 2)
	{
		for(int32_t q = 0; q < 11; ++q)
			draw_checkerboard(dest, x, y + q * 16, 16);
	}
    else if(InvalidBG == 1)
    {
        for(int32_t dy=0; dy<176; dy++)
        {
            for(int32_t dx=0; dx<16; dx++)
            {
                dest->line[y+dy][x+dx]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
            }
        }
    }
    else
    {
        rectfill(dest, x, y, x+15, y+175, vc(0));
        rect(dest, x, y, x+15, y+175, vc(15));
        line(dest, x, y, x+15, y+175, vc(15));
        line(dest, x, y+175, x+15, y, vc(15));
    }
}

void zmap::drawstaticrow(BITMAP* dest,int32_t x,int32_t y)
{
	if (InvalidBG == 2)
	{
		for (int32_t q = 0; q < 16; ++q)
			draw_checkerboard(dest, x + q * 16, y, 16);
	}
    else if(InvalidBG == 1)
    {
        for(int32_t dy=0; dy<16; dy++)
        {
            for(int32_t dx=0; dx<256; dx++)
            {
                dest->line[y+dy][x+dx]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
            }
        }
    }
    else
    {
        rectfill(dest, x, y, x+255, y+15, vc(0));
        rect(dest, x, y, x+255, y+15, vc(15));
        line(dest, x, y, x+255, y+15, vc(15));
        line(dest, x, y+15, x+255, y, vc(15));
    }
}

void zmap::draw_template(BITMAP* dest,int32_t x,int32_t y)
{
    for(int32_t i=0; i<176; i++)
    {
        word cmbdat = screens[TEMPLATE].data[i];
        byte cmbcset = screens[TEMPLATE].cset[i];
        int32_t cmbflag = screens[TEMPLATE].sflag[i];
        put_combo(dest,((i&15)<<4)+x,(i&0xF0)+y,cmbdat,cmbcset,0,cmbflag);
    }
}

void zmap::draw_template2(BITMAP* dest,int32_t x,int32_t y)
{
    for(int32_t i=0; i<176; i++)
    {
        word cmbdat = screens[TEMPLATE2].data[i];
        byte cmbcset = screens[TEMPLATE2].cset[i];
        int32_t cmbflag = screens[TEMPLATE2].sflag[i];
        put_combo(dest,((i&15)<<4)+x,(i&0xF0)+y,cmbdat,cmbcset,0,cmbflag);
    }
}

void zmap::draw_secret(BITMAP *dest, int32_t pos)
{
    word cmbdat = screens[TEMPLATE].data[pos];
    byte cmbcset = screens[TEMPLATE].cset[pos];
    int32_t cmbflag = screens[TEMPLATE].sflag[pos];
    put_combo(dest,0,0,cmbdat,cmbcset,0,cmbflag);
}

void zmap::draw_secret2(BITMAP *dest, int32_t scombo)
{
    word cmbdat =  screens[cursor.screen].secretcombo[scombo];
    byte cmbcset = screens[cursor.screen].secretcset[scombo];
    byte cmbflag = screens[cursor.screen].secretflag[scombo];
    put_combo(dest,0,0,cmbdat,cmbcset,0,cmbflag);
}

void zmap::scroll(int32_t dir, bool warp)
{
    if(cursor.map<map_count)
    {
        switch(dir)
        {
        case up:
            if(warp && Map.CurrScr()->flags2&wfUP)
            {
                dowarp(1,Map.CurrScr()->sidewarpindex&3);
            }
            else if(cursor.screen>15)
            {
                setCurrScr(cursor.screen - 16);
            }
            
            break;
            
        case down:
            if(warp && Map.CurrScr()->flags2&wfDOWN)
            {
                dowarp(1,(Map.CurrScr()->sidewarpindex>>2)&3);
            }
            else if(cursor.screen<MAPSCRS-16)
            {
				setCurrScr(cursor.screen + 16);
            }
            
            break;
            
        case left:
            if(warp && Map.CurrScr()->flags2&wfLEFT)
            {
                dowarp(1,(Map.CurrScr()->sidewarpindex>>4)&3);
            }
            else if(cursor.screen&15)
            {
				setCurrScr(cursor.screen - 1);
            }
            
            break;
            
        case right:
            if(warp && Map.CurrScr()->flags2&wfRIGHT)
            {
                dowarp(1,(Map.CurrScr()->sidewarpindex>>6)&3);
            }
            else if((cursor.screen&15)<15 && cursor.screen<MAPSCRS-1)
            {
				setCurrScr(cursor.screen + 1);
            }
            
            break;
        }
    }
}

void fetch_door(int side, int door, int dcs, word data[176], byte cset[176])
{
	switch(side)
	{
		case up:
			switch(door)
			{
				case dWALL:
				case dBOMB:
				case dWALK:
					data[7]   = DoorComboSets[dcs].doorcombo_u[dt_wall][0];
					cset[7]  = DoorComboSets[dcs].doorcset_u[dt_wall][0];
					data[8]   = DoorComboSets[dcs].doorcombo_u[dt_wall][1];
					cset[8]  = DoorComboSets[dcs].doorcset_u[dt_wall][1];
					data[23]   = DoorComboSets[dcs].doorcombo_u[dt_wall][2];
					cset[23]  = DoorComboSets[dcs].doorcset_u[dt_wall][2];
					data[24]   = DoorComboSets[dcs].doorcombo_u[dt_wall][3];
					cset[24]  = DoorComboSets[dcs].doorcset_u[dt_wall][3];
					break;
					
				default:
					data[7]   = DoorComboSets[dcs].doorcombo_u[dt_pass][0];
					cset[7]  = DoorComboSets[dcs].doorcset_u[dt_pass][0];
					data[8]   = DoorComboSets[dcs].doorcombo_u[dt_pass][1];
					cset[8]  = DoorComboSets[dcs].doorcset_u[dt_pass][1];
					data[23]   = DoorComboSets[dcs].doorcombo_u[dt_pass][2];
					cset[23]  = DoorComboSets[dcs].doorcset_u[dt_pass][2];
					data[24]   = DoorComboSets[dcs].doorcombo_u[dt_pass][3];
					cset[24]  = DoorComboSets[dcs].doorcset_u[dt_pass][3];
					break;
			}
			
			break;
			
		case down:
			switch(door)
			{
				case dWALL:
				case dBOMB:
				case dWALK:
					data[151]   = DoorComboSets[dcs].doorcombo_d[dt_wall][0];
					cset[151]  = DoorComboSets[dcs].doorcset_d[dt_wall][0];
					data[152]   = DoorComboSets[dcs].doorcombo_d[dt_wall][1];
					cset[152]  = DoorComboSets[dcs].doorcset_d[dt_wall][1];
					data[167]   = DoorComboSets[dcs].doorcombo_d[dt_wall][2];
					cset[167]  = DoorComboSets[dcs].doorcset_d[dt_wall][2];
					data[168]   = DoorComboSets[dcs].doorcombo_d[dt_wall][3];
					cset[168]  = DoorComboSets[dcs].doorcset_d[dt_wall][3];
					break;
					
				default:
					data[151]   = DoorComboSets[dcs].doorcombo_d[dt_pass][0];
					cset[151]  = DoorComboSets[dcs].doorcset_d[dt_pass][0];
					data[152]   = DoorComboSets[dcs].doorcombo_d[dt_pass][1];
					cset[152]  = DoorComboSets[dcs].doorcset_d[dt_pass][1];
					data[167]   = DoorComboSets[dcs].doorcombo_d[dt_pass][2];
					cset[167]  = DoorComboSets[dcs].doorcset_d[dt_pass][2];
					data[168]   = DoorComboSets[dcs].doorcombo_d[dt_pass][3];
					cset[168]  = DoorComboSets[dcs].doorcset_d[dt_pass][3];
					break;
			}
			
			break;
			
		case left:
			switch(door)
			{
				case dWALL:
				case dBOMB:
				case dWALK:
					data[64]   = DoorComboSets[dcs].doorcombo_l[dt_wall][0];
					cset[64]  = DoorComboSets[dcs].doorcset_l[dt_wall][0];
					data[65]   = DoorComboSets[dcs].doorcombo_l[dt_wall][1];
					cset[65]  = DoorComboSets[dcs].doorcset_l[dt_wall][1];
					data[80]   = DoorComboSets[dcs].doorcombo_l[dt_wall][2];
					cset[80]  = DoorComboSets[dcs].doorcset_l[dt_wall][2];
					data[81]   = DoorComboSets[dcs].doorcombo_l[dt_wall][3];
					cset[81]  = DoorComboSets[dcs].doorcset_l[dt_wall][3];
					data[96]   = DoorComboSets[dcs].doorcombo_l[dt_wall][4];
					cset[96]  = DoorComboSets[dcs].doorcset_l[dt_wall][4];
					data[97]   = DoorComboSets[dcs].doorcombo_l[dt_wall][5];
					cset[97]  = DoorComboSets[dcs].doorcset_l[dt_wall][5];
					break;
					
				default:
					data[64]   = DoorComboSets[dcs].doorcombo_l[dt_pass][0];
					cset[64]  = DoorComboSets[dcs].doorcset_l[dt_pass][0];
					data[65]   = DoorComboSets[dcs].doorcombo_l[dt_pass][1];
					cset[65]  = DoorComboSets[dcs].doorcset_l[dt_pass][1];
					data[80]   = DoorComboSets[dcs].doorcombo_l[dt_pass][2];
					cset[80]  = DoorComboSets[dcs].doorcset_l[dt_pass][2];
					data[81]   = DoorComboSets[dcs].doorcombo_l[dt_pass][3];
					cset[81]  = DoorComboSets[dcs].doorcset_l[dt_pass][3];
					data[96]   = DoorComboSets[dcs].doorcombo_l[dt_pass][4];
					cset[96]  = DoorComboSets[dcs].doorcset_l[dt_pass][4];
					data[97]   = DoorComboSets[dcs].doorcombo_l[dt_pass][5];
					cset[97]  = DoorComboSets[dcs].doorcset_l[dt_pass][5];
					break;
			}
			
			break;
			
		case right:
			switch(door)
			{
				case dWALL:
				case dBOMB:
				case dWALK:
					data[78]   = DoorComboSets[dcs].doorcombo_r[dt_wall][0];
					cset[78]  = DoorComboSets[dcs].doorcset_r[dt_wall][0];
					data[79]   = DoorComboSets[dcs].doorcombo_r[dt_wall][1];
					cset[79]  = DoorComboSets[dcs].doorcset_r[dt_wall][1];
					data[94]   = DoorComboSets[dcs].doorcombo_r[dt_wall][2];
					cset[94]  = DoorComboSets[dcs].doorcset_r[dt_wall][2];
					data[95]   = DoorComboSets[dcs].doorcombo_r[dt_wall][3];
					cset[95]  = DoorComboSets[dcs].doorcset_r[dt_wall][3];
					data[110]   = DoorComboSets[dcs].doorcombo_r[dt_wall][4];
					cset[110]  = DoorComboSets[dcs].doorcset_r[dt_wall][4];
					data[111]   = DoorComboSets[dcs].doorcombo_r[dt_wall][5];
					cset[111]  = DoorComboSets[dcs].doorcset_r[dt_wall][5];
					break;
					
				default:
					data[78]   = DoorComboSets[dcs].doorcombo_r[dt_pass][0];
					cset[78]  = DoorComboSets[dcs].doorcset_r[dt_pass][0];
					data[79]   = DoorComboSets[dcs].doorcombo_r[dt_pass][1];
					cset[79]  = DoorComboSets[dcs].doorcset_r[dt_pass][1];
					data[94]   = DoorComboSets[dcs].doorcombo_r[dt_pass][2];
					cset[94]  = DoorComboSets[dcs].doorcset_r[dt_pass][2];
					data[95]   = DoorComboSets[dcs].doorcombo_r[dt_pass][3];
					cset[95]  = DoorComboSets[dcs].doorcset_r[dt_pass][3];
					data[110]   = DoorComboSets[dcs].doorcombo_r[dt_pass][4];
					cset[110]  = DoorComboSets[dcs].doorcset_r[dt_pass][4];
					data[111]   = DoorComboSets[dcs].doorcombo_r[dt_pass][5];
					cset[111]  = DoorComboSets[dcs].doorcset_r[dt_pass][5];
					break;
			}
			
			break;
	}
}
void zmap::DoPutDoorCommand(int side, int door, bool force)
{
	if(!force && screens[cursor.screen].door[side] == door)
		return;
	bool already_list = InListCommand();
	if(!already_list)
		StartListCommand();
	DoSetDoorCommand(cursor.screen,side,door);
	if(door != dNONE)
	{
		word data[176] = {0};
		byte cset[176] = {0};
		fetch_door(side, door, screens[cursor.screen].door_combo_set, data, cset);
		for(int q = 0; q < 176; ++q)
			if(data[q])
				DoSetComboCommand(cursor.map,cursor.screen,q,data[q],cset[q]);
	}
	if(!already_list)
		FinishListCommand();
}
void zmap::putdoor(int32_t scr,int32_t side,int32_t door)
{
	if(screens[scr].door[side] == door)
		return;
	screens[scr].door[side] = door;
	if(door != dNONE)
	{
		word data[176] = {0};
		byte cset[176] = {0};
		fetch_door(side, door, screens[scr].door_combo_set, data, cset);
		for(int q = 0; q < 176; ++q)
			if(data[q])
			{
				screens[scr].data[q] = data[q];
				screens[scr].cset[q] = cset[q];
			}
	}
}

void list_command::execute()
{
    for (auto command : commands)
    {
        command->execute();
    }
}

void list_command::undo()
{
    for (int i = commands.size() - 1; i >= 0; i--)
    {
        commands[i]->undo();
    }
}

int list_command::size()
{
    int s = 0;
    for (auto command : commands)
    {
        s += command->size();
    }
    return s;
}

void set_combo_command::execute()
{
    mapscr* scr_ptr = Map.AbsoluteScrMakeValid(map, scr);
	if (!scr_ptr) return;

    if (combo != -1) scr_ptr->data[pos] = combo;
    scr_ptr->cset[pos] = cset;
}

void set_combo_command::undo()
{
    mapscr* mapscr_ptr = Map.AbsoluteScr(map, scr);
	if(!mapscr_ptr) return;
    if (combo != -1) mapscr_ptr->data[pos] = prev_combo;
    mapscr_ptr->cset[pos] = prev_cset;
}

set_ffc_command::data_t set_ffc_command::create_data(const ffcdata& ffc)
{
	std::array<int, 8> initd_arr;
	std::copy(std::begin(ffc.initd), std::end(ffc.initd), initd_arr.begin());

	return {
		.x = ffc.x,
		.y = ffc.y,
		.vx = ffc.vx,
		.vy = ffc.vy,
		.ax = ffc.ax,
		.ay = ffc.ay,
		.data = ffc.data,
		.cset = ffc.cset,
		.delay = ffc.delay,
		.link = ffc.link,
		.script = ffc.script,
		.tw = ffc.txsz,
		.th = ffc.tysz,
		.ew = ffc.hit_width,
		.eh = ffc.hit_height,
		.flags = ffc.flags,
		.initd = initd_arr,
	};
}

void set_ffc_command::execute()
{
    mapscr* mapscr_ptr = Map.AbsoluteScrMakeValid(map, scr);
	if(!mapscr_ptr) return;
	
	mapscr_ptr->valid |= mVALID;
    mapscr_ptr->ffcs[i].x = data.x;
	mapscr_ptr->ffcs[i].y = data.y;
	mapscr_ptr->ffcs[i].vx = data.vx;
	mapscr_ptr->ffcs[i].vy = data.vy;
	mapscr_ptr->ffcs[i].ax = data.ax;
	mapscr_ptr->ffcs[i].ay = data.ay;
	mapscr_ptr->ffcs[i].data = data.data;
	mapscr_ptr->ffcs[i].cset = data.cset;
	mapscr_ptr->ffcs[i].delay = data.delay;
	mapscr_ptr->ffcs[i].link = data.link;
	mapscr_ptr->ffcs[i].script = data.script;
	mapscr_ptr->ffcs[i].flags = data.flags;
	mapscr_ptr->ffEffectWidth(i, data.ew);
	mapscr_ptr->ffEffectHeight(i, data.eh);
	mapscr_ptr->ffTileWidth(i, data.tw);
	mapscr_ptr->ffTileHeight(i, data.th);
	std::copy(std::begin(data.initd), std::end(data.initd), std::begin(mapscr_ptr->ffcs[i].initd));
	mapscr_ptr->ffcCountMarkDirty();
	mapscr_ptr->ffcs[i].updateSolid();
}

void set_ffc_command::undo()
{
    mapscr* mapscr_ptr = Map.AbsoluteScr(map, scr);
	if(!mapscr_ptr) return;

    mapscr_ptr->ffcs[i].x = prev_data.x;
	mapscr_ptr->ffcs[i].y = prev_data.y;
	mapscr_ptr->ffcs[i].vx = prev_data.vx;
	mapscr_ptr->ffcs[i].vy = prev_data.vy;
	mapscr_ptr->ffcs[i].ax = prev_data.ax;
	mapscr_ptr->ffcs[i].ay = prev_data.ay;
	mapscr_ptr->ffcs[i].data = prev_data.data;
	mapscr_ptr->ffcs[i].cset = prev_data.cset;
	mapscr_ptr->ffcs[i].delay = prev_data.delay;
	mapscr_ptr->ffcs[i].link = prev_data.link;
	mapscr_ptr->ffcs[i].script = prev_data.script;
	mapscr_ptr->ffcs[i].flags = prev_data.flags;
	mapscr_ptr->ffEffectWidth(i, prev_data.ew);
	mapscr_ptr->ffEffectHeight(i, prev_data.eh);
	mapscr_ptr->ffTileWidth(i, prev_data.tw);
	mapscr_ptr->ffTileHeight(i, prev_data.th);
	std::copy(std::begin(prev_data.initd), std::end(prev_data.initd), std::begin(mapscr_ptr->ffcs[i].initd));
	mapscr_ptr->ffcCountMarkDirty();
	mapscr_ptr->ffcs[i].updateSolid();
}

void set_flag_command::execute()
{
    mapscr* mapscr_ptr = Map.AbsoluteScrMakeValid(map, scr);
	if(!mapscr_ptr) return;
	
	mapscr_ptr->valid |= mVALID;
    mapscr_ptr->sflag[pos] = flag;
}

void set_flag_command::undo()
{
	mapscr* mapscr_ptr = Map.AbsoluteScr(map, scr);
	if(!mapscr_ptr) return;
    mapscr_ptr->sflag[pos] = prev_flag;
}

void set_door_command::execute()
{
	auto* mapscr_ptr = Map.AbsoluteScrMakeValid(cursor.map, cursor.screen);
	if(!mapscr_ptr) return;
	
	mapscr_ptr->valid |= mVALID;
	mapscr_ptr->door[side] = door;
}

void set_door_command::undo()
{
	Map.AbsoluteScr(cursor.map, cursor.screen)->door[side] = prev_door;
}

void set_dcs_command::execute()
{
	auto* mapscr_ptr = Map.AbsoluteScrMakeValid(cursor.map, cursor.screen);
	if(!mapscr_ptr) return;
	
	mapscr_ptr->valid |= mVALID;
	mapscr_ptr->door_combo_set = dcs;
}

void set_dcs_command::undo()
{
	Map.AbsoluteScr(cursor.map, cursor.screen)->door_combo_set = prev_dcs;
}

void paste_screen_command::execute()
{
    perform(screen.get());
}

void paste_screen_command::undo()
{
    if (prev_screens.size() > 1)
    {
        ASSERT(type == PasteCommandType::ScreenPartialToEveryScreen || type == PasteCommandType::ScreenAllToEveryScreen);
        ASSERT(prev_screens.size() == 128);
        for (int i = 0; i < 128; i++)
        {
            copy_mapscr(Map.AbsoluteScrMakeValid(cursor.map, i), prev_screens[i].get());
            // TODO: why not just this?
            // If this changes, also change the line in PasteAllToAll and PasteAll to use simply copy assignment.
            // *Map.AbsoluteScr(map, i) = *prev_screens[i].get();
        }
        return;
    }

    perform(prev_screens[0].get());
}

int paste_screen_command::size()
{
    return prev_screens.size() + 1;
}

void paste_screen_command::perform(mapscr* to)
{
    if (to)
    {
        switch (type) {
            case ScreenAll:                  Map.PasteAll(*to, screen_index); break;
            case ScreenAllToEveryScreen:     Map.PasteAllToAll(*to); break;
            case ScreenData:                 Map.PasteScreenData(*to, screen_index); break;
            case ScreenDoors:                Map.PasteDoors(*to, screen_index); break;
            case ScreenEnemies:              Map.PasteEnemies(*to, screen_index); break;
            case ScreenFFCombos:             Map.PasteFFCombos(*to, screen_index); break;
            case ScreenGuy:                  Map.PasteGuy(*to, screen_index); break;
            case ScreenLayers:               Map.PasteLayers(*to, screen_index); break;
            case ScreenPalette:              Map.PastePalette(*to, screen_index); break;
            case ScreenPartial:              Map.Paste(*to, screen_index); break;
            case ScreenPartialToEveryScreen: Map.PasteToAll(*to); break;
            case ScreenRoom:                 Map.PasteRoom(*to, screen_index); break;
            case ScreenSecretCombos:         Map.PasteSecretCombos(*to, screen_index); break;
            case ScreenUnderCombo:           Map.PasteUnderCombo(*to, screen_index); break;
            case ScreenWarpLocations:        Map.PasteWarpLocations(*to, screen_index); break;
            case ScreenWarps:                Map.PasteWarps(*to, screen_index); break;
        }
    }
    else
    {
        Map.clearscr(screen_index);
    }
    refresh(rALL);
}

void set_screen_command::execute()
{
    if (screen)
    {
        copy_mapscr(Map.AbsoluteScrMakeValid(cursor.map, screen_index), screen.get());
    }
    else
    {
        Map.clearscr(screen_index);
    }
    refresh(rALL);
}

void set_screen_command::undo()
{
    if (prev_screen)
    {
        copy_mapscr(Map.AbsoluteScrMakeValid(cursor.map, screen_index), prev_screen.get());
    }
    else
    {
        Map.clearscr(screen_index);
    }
    refresh(rALL);
}

int set_screen_command::size()
{
    return (prev_screen ? 1 : 0) + (screen ? 1 : 0);
}

static std::shared_ptr<list_command> current_list_command;
void zmap::StartListCommand()
{
    ASSERT(!current_list_command);
    current_list_command.reset(new list_command);
}

void zmap::FinishListCommand()
{
    if (current_list_command->commands.size() == 1)
    {
        undo_stack.push_back(current_list_command->commands[0]);
    }
    else if (current_list_command->commands.size() > 1)
    {
        undo_stack.push_back(current_list_command);
    }
    CapCommandHistory();
    current_list_command = nullptr;
}

void zmap::RevokeListCommand()
{
    current_list_command->undo();
    current_list_command = nullptr;
}

bool zmap::InListCommand() const
{
	return current_list_command ? true : false;
}

void zmap::ExecuteCommand(std::shared_ptr<user_input_command> command, bool skip_execute)
{
    redo_stack = std::stack<std::shared_ptr<user_input_command>>();
    if (!skip_execute) command->execute();
    if (current_list_command)
    {
        current_list_command->commands.push_back(command);
        if (current_list_command->commands.size() == 1)
        {
			current_list_command->cursor = command->cursor;
        }
    }
    else
    {
        undo_stack.push_back(command);
        CapCommandHistory();
    }
    saved = false;
}

void zmap::UndoCommand()
{
    if (undo_stack.size() <= 0) return;

    // If not currently looking at the associated screen, first change the view
    // and wait for the next call to actually undo this command.
    auto command = undo_stack.back();
    if (command->cursor.map != Map.getCurrMap() || command->cursor.screen != Map.getCurrScr())
    {
        setCursor(command.get()->cursor);
        return;
    }

    command->undo();
    redo_stack.push(command);
    undo_stack.pop_back();
    saved = false;
}

void zmap::RedoCommand()
{
    if (redo_stack.size() <= 0) return;

    // If not currently selected the associated screen, first change the cursor
    // and wait for the next call to actually execute this command.
    auto command = redo_stack.top();
    if (command->cursor.map != Map.getCurrMap() || command->cursor.screen != Map.getCurrScr())
    {
        setCursor(command.get()->cursor);
        return;
    }

    command->execute();
    undo_stack.push_back(command);
    redo_stack.pop();
    saved = false;
}

void zmap::ClearCommandHistory()
{
    current_list_command = nullptr;
    undo_stack = std::deque<std::shared_ptr<user_input_command>>();
    redo_stack = std::stack<std::shared_ptr<user_input_command>>();
}

// Extra amount is from mapscr's vectors.
static int size_of_mapscr = sizeof(mapscr) + 4*176;
// Allow the undo system to use roughly 100 MB of memory.
// This doesn't count the memory used by commands that don't store a mapscr,
// but that should be negligible.
static int max_command_size = 100e6 / size_of_mapscr;
void zmap::CapCommandHistory()
{
    int size;
    do
    {
        size = 0;
        for (auto command : undo_stack)
        {
            size += command->size();
        }
        if (size > max_command_size) undo_stack.pop_front();
    } while (size > max_command_size);
}

void zmap::DoSetComboCommand(ComboPosition pos, int combo, int cset)
{
	if (!pos.is_valid(cursor))
		return;

	int map = cursor.map;
	int screen = cursor.viewscr + pos.screen_offset();
	if (!AbsoluteScr(map, screen))
		return;

	if (CurrentLayer)
	{
		mapscr* scr = AbsoluteScrMakeValid(map, screen);
		map = scr->layermap[CurrentLayer-1]-1;
		screen = scr->layerscreen[CurrentLayer-1];
	}
	DoSetComboCommand(map, screen, pos.truncate(), combo, cset);
}

void zmap::DoSetComboCommand(int map, int scr, int pos, int combo, int cset)
{
	mapscr* mapscr_ptr = AbsoluteScrMakeValid(map, scr);
	if (!mapscr_ptr) return;

    std::shared_ptr<set_combo_command> command(new set_combo_command);
    command->cursor = cursor;
    command->map = map;
    command->scr = scr;
    command->pos = pos;
    command->combo = combo;
    command->cset = cset;
    command->prev_combo = mapscr_ptr->data[pos];
    command->prev_cset = mapscr_ptr->cset[pos];
    if ((command->combo != -1 && command->prev_combo == command->combo) && command->cset == command->prev_cset)
    {
        // nothing to do...
        return;
    }

    ExecuteCommand(command);
}

void zmap::DoSetFFCCommand(int map, int scr, int i, set_ffc_command::data_t data)
{
	mapscr* mapscr_ptr = Map.AbsoluteScr(map, scr);
	if(!mapscr_ptr) return;

	mapscr_ptr->ensureFFC(i);

	std::shared_ptr<set_ffc_command> command(new set_ffc_command);

	std::array<int, 8> initd_arr;
	std::copy(std::begin(mapscr_ptr->ffcs[i].initd), std::end(mapscr_ptr->ffcs[i].initd), initd_arr.begin());

	auto prev_data = set_ffc_command::create_data(mapscr_ptr->ffcs[i]);

    command->cursor = cursor;
    command->map = map;
    command->scr = scr;
    command->i = i;
    command->data = data;
    command->prev_data = prev_data;
    if (data == prev_data)
    {
        // nothing to do...
        return;
    }

    ExecuteCommand(command);
}

void zmap::DoSetFlagCommand(ComboPosition pos, int flag)
{
	if (!pos.is_valid(cursor))
		return;

	int map = cursor.map;
	int screen = cursor.viewscr + pos.screen_offset();
	if (!AbsoluteScr(map, screen))
		return;

	if (CurrentLayer)
	{
		mapscr* scr = AbsoluteScrMakeValid(map, screen);
		map = scr->layermap[CurrentLayer-1]-1;
		screen = scr->layerscreen[CurrentLayer-1];
	}
	DoSetFlagCommand(map, screen, pos.truncate(), flag);
}

void zmap::DoSetFlagCommand(int map, int scr, int pos, int flag)
{
	mapscr* mapscr_ptr = Map.AbsoluteScr(map, scr);
	if(!mapscr_ptr) return;

    std::shared_ptr<set_flag_command> command(new set_flag_command);
    command->cursor = cursor;
    command->map = map;
    command->scr = scr;
    command->pos = pos;
    command->flag = flag;
    command->prev_flag = mapscr_ptr->sflag[pos];
    if (command->flag == command->prev_flag)
    {
        // nothing to do...
        return;
    }

    ExecuteCommand(command);
}

void zmap::DoSetDoorCommand(int scr, int side, int door)
{
	if(screens[scr].door[side] == door)
		return;
    std::shared_ptr<set_door_command> command(new set_door_command);
    command->cursor = cursor;
    command->side = side;
    command->door = door;
    command->prev_door = screens[scr].door[side];

    ExecuteCommand(command);
}
void zmap::DoSetDCSCommand(int dcs)
{
	if(screens[cursor.screen].door_combo_set == dcs)
		return;
    std::shared_ptr<set_dcs_command> command(new set_dcs_command);
    command->cursor = cursor;
    command->dcs = dcs;
    command->prev_dcs = screens[cursor.screen].door_combo_set;

    ExecuteCommand(command);
}

void zmap::DoPasteScreenCommand(PasteCommandType type, int screen)
{
	if (screen == -1)
		screen = cursor.screen;

    std::shared_ptr<paste_screen_command> command(new paste_screen_command);
    command->cursor = cursor;
    command->type = type;
    command->screen = std::shared_ptr<mapscr>(new mapscr(copymapscr));
	command->screen_index = screen;

    if (type == PasteCommandType::ScreenPartialToEveryScreen || type == PasteCommandType::ScreenAllToEveryScreen)
    {
        for (int i=0; i < 128; i++)
        {
            command->prev_screens.push_back(std::shared_ptr<mapscr>(new mapscr(screens[i])));
        }
    }
    else
    {
        command->prev_screens.push_back(std::shared_ptr<mapscr>(new mapscr(screens[screen])));
    }

    ExecuteCommand(command);
}

void zmap::DoClearScreenCommand(int screen)
{
    std::shared_ptr<set_screen_command> command(new set_screen_command);
    command->cursor = cursor;
    command->prev_screen = std::shared_ptr<mapscr>(new mapscr(screens[screen]));
    command->screen = std::shared_ptr<mapscr>(nullptr);
	command->screen_index = screen;

    ExecuteCommand(command);
}

void zmap::DoTemplateCommand(int floorcombo, int floorcset, int screen)
{
    std::shared_ptr<set_screen_command> command(new set_screen_command);
    command->cursor = cursor;
    command->prev_screen = std::shared_ptr<mapscr>(new mapscr(*Map.Scr(screen)));
    Template(floorcombo, floorcset, screen);
    command->screen = std::shared_ptr<mapscr>(new mapscr(*Map.Scr(screen)));

    ExecuteCommand(command, true);
}

void zmap::Copy(int scr)
{
    if(screens[scr].valid&mVALID)
    {
        copy_mapscr(&copymapscr, &screens[scr]);
        //copymapscr=screens[scr];
        can_paste=true;
        copymap=cursor.map;
        copyscr=scr;
		copyscrdata = zinit.screen_data[cursor.map*MAPSCRS+scr];
        copyffc = -1;
    }
}

void zmap::CopyFFC(int32_t screen, int32_t n)
{
    if(screens[screen].valid&mVALID)
    {
        copy_mapscr(&copymapscr, &screens[screen]);
        // Can't paste the screen itself
        can_paste = false;
        copymap=cursor.map;
        copyscr=screen;
        copyffc = n;
    }
}

void zmap::Paste(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        int32_t oldcolor=getcolor();
        
        if(!(screens[screen].valid&mVALID))
        {
            screens[screen].valid |= mVALID;
            screens[screen].color = copymapscr.color;
        }
        
        screens[screen].door_combo_set = copymapscr.door_combo_set;
        
        for(int32_t i=0; i<4; i++)
        {
            screens[screen].door[i]=copymapscr.door[i];
        }
        
        for(int32_t i=0; i<176; i++)
        {
            screens[screen].data[i] = copymapscr.data[i];
            screens[screen].cset[i] = copymapscr.cset[i];
            screens[screen].sflag[i] = copymapscr.sflag[i];
        }
        
        int32_t newcolor=getcolor();
        loadlvlpal(newcolor);
        
        if(newcolor!=oldcolor)
        {
            rebuild_trans_table();
        }
        
        saved=false;
    }
}

void zmap::PasteUnderCombo(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        screens[screen].undercombo = copymapscr.undercombo;
        screens[screen].undercset = copymapscr.undercset;
        saved=false;
    }
}

void zmap::PasteSecretCombos(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        for(int32_t i=0; i<128; i++)
        {
            screens[screen].secretcombo[i] = copymapscr.secretcombo[i];
            screens[screen].secretcset[i] = copymapscr.secretcset[i];
            screens[screen].secretflag[i] = copymapscr.secretflag[i];
        }
        
        saved=false;
    }
}

// TODO const mapscr& copymapscr
void zmap::PasteFFCombos(mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
		screens[screen].ffcs = copymapscr.ffcs;
		screens[screen].ffcCountMarkDirty();
        saved=false;
    }
}

void zmap::PasteWarps(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        screens[screen].sidewarpindex = copymapscr.sidewarpindex;
        
        for(int32_t i=0; i<4; i++)
        {
            screens[screen].tilewarptype[i] = copymapscr.tilewarptype[i];
            screens[screen].tilewarpdmap[i] = copymapscr.tilewarpdmap[i];
            screens[screen].tilewarpscr[i] = copymapscr.tilewarpscr[i];
            screens[screen].sidewarptype[i] = copymapscr.sidewarptype[i];
            screens[screen].sidewarpdmap[i] = copymapscr.sidewarpdmap[i];
            screens[screen].sidewarpscr[i] = copymapscr.sidewarpscr[i];
            screens[screen].flags2 &= ~(wfUP|wfDOWN|wfLEFT|wfRIGHT);
            screens[screen].flags2 |= copymapscr.flags2 & (wfUP|wfDOWN|wfLEFT|wfRIGHT);
            screens[screen].sidewarpoverlayflags = copymapscr.sidewarpoverlayflags;
            screens[screen].tilewarpoverlayflags = copymapscr.tilewarpoverlayflags;
        }
        
        saved=false;
    }
}

void zmap::PasteScreenData(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        screens[screen].csensitive = copymapscr.csensitive;
        screens[screen].oceansfx = copymapscr.oceansfx;
        screens[screen].bosssfx = copymapscr.bosssfx;
        screens[screen].secretsfx = copymapscr.secretsfx;
        screens[screen].holdupsfx = copymapscr.holdupsfx;
        screens[screen].flags = copymapscr.flags;
        screens[screen].flags2 &= (wfUP|wfDOWN|wfLEFT|wfRIGHT);
        screens[screen].flags2 |= copymapscr.flags2 & ~(wfUP|wfDOWN|wfLEFT|wfRIGHT);
        screens[screen].flags3 = copymapscr.flags3;
        screens[screen].flags4 = copymapscr.flags4;
        screens[screen].flags5 = copymapscr.flags5;
        screens[screen].flags6 = copymapscr.flags6;
        screens[screen].flags7 = copymapscr.flags7;
        screens[screen].flags8 = copymapscr.flags8;
        screens[screen].flags9 = copymapscr.flags9;
        screens[screen].flags10 = copymapscr.flags10;
        screens[screen].item = copymapscr.item;
        screens[screen].hasitem = copymapscr.hasitem;
        screens[screen].itemx = copymapscr.itemx;
        screens[screen].itemy = copymapscr.itemy;
        screens[screen].nextmap = copymapscr.nextmap;
        screens[screen].nextscr = copymapscr.nextscr;
        screens[screen].nocarry = copymapscr.nocarry;
        screens[screen].noreset = copymapscr.noreset;
        screens[screen].path[0] = copymapscr.path[0];
        screens[screen].path[1] = copymapscr.path[1];
        screens[screen].path[2] = copymapscr.path[2];
        screens[screen].path[3] = copymapscr.path[3];
        screens[screen].pattern = copymapscr.pattern;
        screens[screen].exitdir = copymapscr.exitdir;
        screens[screen].enemyflags = copymapscr.enemyflags;
        screens[screen].screen_midi = copymapscr.screen_midi;
        screens[screen].stairx = copymapscr.stairx;
        screens[screen].stairy = copymapscr.stairy;
        screens[screen].timedwarptics = copymapscr.timedwarptics;
        saved=false;
    }
}

void zmap::PasteWarpLocations(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        screens[screen].warpreturnc = copymapscr.warpreturnc;
        screens[screen].warparrivalx = copymapscr.warparrivalx;
        screens[screen].warparrivaly = copymapscr.warparrivaly;
        
        for(int32_t i=0; i<4; i++)
        {
            screens[screen].warpreturnx[i] = copymapscr.warpreturnx[i];
            screens[screen].warpreturny[i] = copymapscr.warpreturny[i];
        }
        
        saved=false;
    }
}

void zmap::PasteDoors(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        for(int32_t i=0; i<4; i++)
            screens[screen].door[i] = copymapscr.door[i];
            
        screens[screen].door_combo_set = copymapscr.door_combo_set;
        saved=false;
    }
}

void zmap::PasteLayers(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        for(int32_t i=0; i<6; i++)
        {
            screens[screen].layermap[i] = copymapscr.layermap[i];
            screens[screen].layerscreen[i] = copymapscr.layerscreen[i];
            screens[screen].layeropacity[i] = copymapscr.layeropacity[i];
        }
        
        saved=false;
    }
}

void zmap::PasteRoom(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        screens[screen].room = copymapscr.room;
        screens[screen].catchall = copymapscr.catchall;
        saved=false;
    }
}

void zmap::PasteGuy(const mapscr& copymapscr, int screen)
{
	if(can_paste)
	{
		screens[screen].guy = copymapscr.guy;
		screens[screen].guytile = copymapscr.guytile;
		screens[screen].guycs = copymapscr.guycs;
		SETFLAG(screens[screen].roomflags,RFL_ALWAYS_GUY,copymapscr.roomflags&RFL_ALWAYS_GUY);
		SETFLAG(screens[screen].roomflags,RFL_GUYFIRES,copymapscr.roomflags&RFL_GUYFIRES);
		screens[screen].str = copymapscr.str;
		saved=false;
	}
}

void zmap::PastePalette(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        int32_t oldcolor=getcolor();
        screens[screen].color = copymapscr.color;
        int32_t newcolor=getcolor();
        loadlvlpal(newcolor);
        
        screens[screen].valid|=mVALID;
        
        if(newcolor!=oldcolor)
        {
            rebuild_trans_table();
        }
        
        saved=false;
    }
}

void zmap::PasteAll(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        int32_t oldcolor=getcolor();
        copy_mapscr(&screens[screen], &copymapscr);
		zinit.screen_data[cursor.map*MAPSCRS+cursor.screen] = copyscrdata;
        //screens[screen]=copymapscr;
        int32_t newcolor=getcolor();
        loadlvlpal(newcolor);
        
        screens[screen].valid|=mVALID;
        
        if(newcolor!=oldcolor)
        {
            rebuild_trans_table();
        }
        
        saved=false;
    }
}


void zmap::PasteToAll(const mapscr& copymapscr)
{
    if(can_paste)
    {
        int32_t oldcolor=getcolor();
        
        for(int32_t x=0; x<128; x++)
        {
            if(!(screens[x].valid&mVALID))
            {
                screens[x].valid |= mVALID;
                screens[x].color = copymapscr.color;
            }
            
            for(int32_t i=0; i<176; i++)
            {
                screens[x].data[i] = copymapscr.data[i];
                screens[x].cset[i] = copymapscr.cset[i];
                screens[x].sflag[i] = copymapscr.sflag[i];
            }
        }
        
        int32_t newcolor=getcolor();
        loadlvlpal(newcolor);
        
        if(!(screens[cursor.screen].valid&mVALID))
        {
            newcolor=-1;
        }
        
        if(newcolor!=oldcolor)
        {
            rebuild_trans_table();
        }
        
        saved=false;
    }
}

void zmap::PasteAllToAll(const mapscr& copymapscr)
{
    if(can_paste)
    {
        int32_t oldcolor=getcolor();
        
        for(int32_t x=0; x<128; x++)
        {
            copy_mapscr(&screens[x], &copymapscr);
			zinit.screen_data[cursor.map*MAPSCRS+x] = copyscrdata;
            //screens[x]=copymapscr;
        }
        
        int32_t newcolor=getcolor();
        loadlvlpal(newcolor);
        
        if(!(screens[cursor.screen].valid&mVALID))
        {
            newcolor=-1;
        }
        
        if(newcolor!=oldcolor)
        {
            rebuild_trans_table();
        }
        
        saved=false;
    }
}

void zmap::PasteEnemies(const mapscr& copymapscr, int screen)
{
    if(can_paste)
    {
        for(int32_t i=0; i<10; i++)
            screens[screen].enemy[i]=copymapscr.enemy[i];
    }
}

void zmap::setCopyFFC(int32_t n)
{
	copyffc = n;
}

void zmap::update_combo_cycling()
{
    if(!prv_mode||!prv_cmbcycle)
    {
        return;
    }
    
    int32_t x;
    int32_t newdata[176];
    int32_t newcset[176];
    bool restartanim[MAXCOMBOS] = {0};
    
    for(int32_t i=0; i<176; i++)
    {
        newdata[i]=-1;
        newcset[i]=-1;
        
        x=prvscr.data[i];
        
        //time to restart
        if((combobuf[x].aclk>=combobuf[x].speed) &&
                (combobuf[x].tile-combobuf[x].frames>=combobuf[x].o_tile-1) &&
                combobuf[x].can_cycle())
        {
			bool cycle_under = (combobuf[x].animflags & AF_CYCLEUNDERCOMBO);
            newdata[i] = cycle_under ? prvscr.undercombo : combobuf[x].nextcombo;
				
			if(!(combobuf[x].animflags & AF_CYCLENOCSET))
				newcset[i] = cycle_under ? prvscr.undercset : combobuf[x].nextcset;
            int32_t c = newdata[i];
            
            if(combobuf[c].animflags & AF_CYCLE)
            {
                restartanim[c]=true;
            }
        }
    }
    
    for(int32_t i=0; i<176; i++)
    {
        x=prvscr.data[i];
        
        //time to restart
        if((combobuf[x].aclk>=combobuf[x].speed) &&
                (combobuf[x].tile-combobuf[x].frames>=combobuf[x].o_tile-1) &&
                combobuf[x].can_cycle())
        {
			bool cycle_under = (combobuf[x].animflags & AF_CYCLEUNDERCOMBO);
            newdata[i] = cycle_under ? prvscr.undercombo : combobuf[x].nextcombo;
				
			if(!(combobuf[x].animflags & AF_CYCLENOCSET))
				newcset[i] = cycle_under ? prvscr.undercset : combobuf[x].nextcset;
            int32_t c = newdata[i];
            
            if(combobuf[c].animflags & AF_CYCLE)
            {
                restartanim[c]=true;
            }
        }
    }
    
    for(int32_t i=0; i<176; i++)
    {
        if(newdata[i]==-1)
            continue;
            
        prvscr.data[i]=newdata[i];
        prvscr.cset[i]=newcset[i];
    }
    
	word maxffc = prvscr.numFFC();
    for(word i=0; i<maxffc; i++)
    {
		ffcdata& ffc = prvscr.ffcs[i];
        newcombo const& cmb = combobuf[ffc.data];
        
        //time to restart
        if((cmb.aclk>=cmb.speed) &&
                (cmb.tile-cmb.frames>=cmb.o_tile-1) &&
                cmb.can_cycle())
        {
			bool cycle_under = (cmb.animflags & AF_CYCLEUNDERCOMBO);
            ffc.data = cycle_under ? prvscr.undercombo : cmb.nextcombo;
				
			if(!(cmb.animflags & AF_CYCLENOCSET))
				newcset[i] = cycle_under ? prvscr.undercset : cmb.nextcset;
            
            if(combobuf[ffc.data].animflags & AF_CYCLE)
            {
                restartanim[ffc.data]=true;
            }
			prvscr.ffcs[i].data = ffc.data;
			prvscr.ffcs[i].cset=ffc.cset;
        }
    }
    
    
    if(get_qr(qr_CMBCYCLELAYERS))
    {
        for(int32_t j=0; j<6; j++)
        {
            if(!prvlayers[j].valid)
                continue;
                
            for(int32_t i=0;	i<176; i++)
            {
                newdata[i]=-1;
                newcset[i]=-1;
                
                x=(prvlayers[j]).data[i];
                
                //time to restart
                if((combobuf[x].aclk>=combobuf[x].speed) &&
                        (combobuf[x].tile-combobuf[x].frames>=combobuf[x].o_tile-1)	&&
                        combobuf[x].can_cycle())
                {
					bool cycle_under = (combobuf[x].animflags & AF_CYCLEUNDERCOMBO);
					newdata[i] = cycle_under ? prvscr.undercombo : combobuf[x].nextcombo;
						
					if(!(combobuf[x].animflags & AF_CYCLENOCSET))
						newcset[i] = cycle_under ? prvscr.undercset : combobuf[x].nextcset;
                    int32_t c = newdata[i];
                    
                    if(combobuf[c].animflags & AF_CYCLE)
                    {
                        restartanim[c]=true;
                    }
                }
            }
            
            for(int32_t i=0; i<176; i++)
            {
                x=(prvlayers[j]).data[i];
                
                //time to restart
                if((combobuf[x].aclk>=combobuf[x].speed) &&
                        (combobuf[x].tile-combobuf[x].frames>=combobuf[x].o_tile-1) &&
                        combobuf[x].can_cycle())
                {
					bool cycle_under = (combobuf[x].animflags & AF_CYCLEUNDERCOMBO);
					newdata[i] = cycle_under ? prvscr.undercombo : combobuf[x].nextcombo;
						
					if(!(combobuf[x].animflags & AF_CYCLENOCSET))
						newcset[i] = cycle_under ? prvscr.undercset : combobuf[x].nextcset;
                    int32_t c = newdata[i];
                    
                    if(combobuf[c].animflags & AF_CYCLE)
                    {
                        restartanim[c]=true;
                    }
                }
            }
            
            for(int32_t i=0; i<176; i++)
            {
                if(newdata[i]==-1)
                    continue;
                    
                prvlayers[j].data[i]=newdata[i];
                prvlayers[j].cset[i]=newcset[i];
            }
        }
    }
    
    for(int32_t i=0; i<MAXCOMBOS; i++)
    {
        if(restartanim[i])
        {
            combobuf[i].tile = combobuf[i].o_tile;
			combobuf[i].cur_frame=0;
            combobuf[i].aclk = 0;
        }
    }
}

void zmap::update_freeform_combos()
{
    if(!prv_mode||!prv_cmbcycle)
    {
        return;
    }
    
	// TODO: this changer code is a duplicated here and for zplayer. Should fix that.
	word maxffc = prvscr.numFFC();
    for(int32_t i=0; i<maxffc; i++)
    {
        if(!(prvscr.ffcs[i].flags&ffc_changer) && prvscr.ffcs[i].data!=0 && !(prvscr.ffcs[i].flags&ffc_stationary))
        {
            for(int32_t j=0; j<maxffc; j++)
            {
                if(i!=j)
                {
                    if(prvscr.ffcs[j].flags&ffc_changer && prvscr.ffcs[j].data != 0)
                    {
                        if((((prvscr.ffcs[j].x.getInt())!=ffposx[i])||((prvscr.ffcs[j].y.getInt())!=ffposy[i]))&&(prvscr.ffcs[i].link==0))
                        {
                            if((isonline(prvscr.ffcs[i].x.getZLong(),prvscr.ffcs[i].y.getZLong(),ffprvx[i],ffprvy[i],prvscr.ffcs[j].x.getZLong(),prvscr.ffcs[j].y.getZLong())||
                                    ((prvscr.ffcs[i].x.getZLong()==prvscr.ffcs[j].x.getZLong())&&(prvscr.ffcs[i].y.getZLong()==prvscr.ffcs[j].y.getZLong())))&&(ffprvx[i]>-10000000&&ffprvy[i]>-10000000))
                            {
                                //prvscr.ffcs[i].data=prvscr.ffcs[j].data;
                                //prvscr.ffcs[i].cset=prvscr.ffcs[j].cset;
                                if(prvscr.ffcs[j].flags&ffc_changethis)
                                {
                                    prvscr.ffcs[i].data = prvscr.ffcs[j].data;
                                    prvscr.ffcs[i].cset = prvscr.ffcs[j].cset;
                                }
                                
                                if(prvscr.ffcs[j].flags&ffc_changenext)
                                    prvscr.ffcs[i].data += 1;
                                    
                                if(prvscr.ffcs[j].flags&ffc_changeprev)
                                    prvscr.ffcs[i].data -= 1;
                                    
                                prvscr.ffcs[i].delay=prvscr.ffcs[j].delay;
                                prvscr.ffcs[i].x=prvscr.ffcs[j].x;
                                prvscr.ffcs[i].y=prvscr.ffcs[j].y;
				
								prvscr.ffcs[i].vx=prvscr.ffcs[j].vx;
                                prvscr.ffcs[i].vy=prvscr.ffcs[j].vy;
								prvscr.ffcs[i].ax=prvscr.ffcs[j].ax;
                                prvscr.ffcs[i].ay=prvscr.ffcs[j].ay;
                                
                                prvscr.ffcs[i].link=prvscr.ffcs[j].link;
                                prvscr.ffcs[i].hit_width=prvscr.ffcs[j].hit_width;
                                prvscr.ffcs[i].hit_height=prvscr.ffcs[j].hit_height;
								prvscr.ffcs[i].txsz=prvscr.ffcs[j].txsz;
                                prvscr.ffcs[i].tysz=prvscr.ffcs[j].tysz;
                                
                                if(prvscr.ffcs[i].flags&ffc_carryover)
                                    prvscr.ffcs[i].flags=prvscr.ffcs[j].flags&ffc_carryover;
                                else prvscr.ffcs[i].flags=prvscr.ffcs[j].flags;
                                
                                prvscr.ffcs[i].flags&=~ffc_changer;
                                ffposx[i]=(prvscr.ffcs[j].x.getInt());
                                ffposy[i]=(prvscr.ffcs[j].y.getInt());
                                
                                if(combobuf[prvscr.ffcs[j].data].flag>15 && combobuf[prvscr.ffcs[j].data].flag<32)
                                {
                                    prvscr.ffcs[j].data = prvscr.secretcombo[combobuf[prvscr.ffcs[j].data].flag - 16 + 4];
                                }
                                
                                if((prvscr.ffcs[j].flags&ffc_swapnext)||(prvscr.ffcs[j].flags&ffc_swapprev))
                                {
                                    int32_t k=0;
                                    
                                    if(prvscr.ffcs[j].flags&ffc_swapnext)
                                        k=j<(MAXFFCS-1)?j+1:0;
                                        
                                    if(prvscr.ffcs[j].flags&ffc_swapprev)
                                        k=j>0?j-1:(MAXFFCS-1);
                                        
                                    zc_swap(prvscr.ffcs[j].vx,prvscr.ffcs[k].vx);
                                    zc_swap(prvscr.ffcs[j].vy,prvscr.ffcs[k].vy);
                                    zc_swap(prvscr.ffcs[j].ax,prvscr.ffcs[k].ax);
                                    zc_swap(prvscr.ffcs[j].ay,prvscr.ffcs[k].ay);
                                    zc_swap(prvscr.ffcs[j].link,prvscr.ffcs[k].link);
                                    zc_swap(prvscr.ffcs[j].hit_width,prvscr.ffcs[k].hit_width);
                                    zc_swap(prvscr.ffcs[j].hit_height,prvscr.ffcs[k].hit_height);
				    zc_swap(prvscr.ffcs[j].txsz,prvscr.ffcs[k].txsz);
                                    zc_swap(prvscr.ffcs[j].tysz,prvscr.ffcs[k].tysz);
                                    zc_swap(prvscr.ffcs[j].flags,prvscr.ffcs[k].flags);
                                }
                            }
                        }
                    }
                }
            }
            
            if(prvscr.ffcs[i].link ? !prvscr.ffcs[prvscr.ffcs[i].link].delay : !prvscr.ffcs[i].delay)
            {
                if(prvscr.ffcs[i].link&&(prvscr.ffcs[i].link-1)!=i)
                {
                    ffprvx[i] = prvscr.ffcs[i].x.getZLong();
                    ffprvy[i] = prvscr.ffcs[i].y.getZLong();
                    prvscr.ffcs[i].x+=prvscr.ffcs[prvscr.ffcs[i].link-1].vx;
                    prvscr.ffcs[i].y+=prvscr.ffcs[prvscr.ffcs[i].link-1].vy;
                }
                else
                {
                    ffprvx[i] = prvscr.ffcs[i].x.getZLong();
                    ffprvy[i] = prvscr.ffcs[i].y.getZLong();
                    prvscr.ffcs[i].x+=prvscr.ffcs[i].vx;
                    prvscr.ffcs[i].y+=prvscr.ffcs[i].vy;
                    prvscr.ffcs[i].vx+=prvscr.ffcs[i].ax;
                    prvscr.ffcs[i].vy+=prvscr.ffcs[i].ay;
                    
					if(get_qr(qr_OLD_FFC_SPEED_CAP))
					{
						if(prvscr.ffcs[i].vx>128) prvscr.ffcs[i].vx=128;
						
						if(prvscr.ffcs[i].vx<-128) prvscr.ffcs[i].vx=-128;
						
						if(prvscr.ffcs[i].vy>128) prvscr.ffcs[i].vy=128;
						
						if(prvscr.ffcs[i].vy<-128) prvscr.ffcs[i].vy=-128;
					}
                }
            }
            else
            {
                if(!prvscr.ffcs[i].link || (prvscr.ffcs[i].link-1)==i)
                    prvscr.ffcs[i].delay--;
            }
            
            if(prvscr.ffcs[i].x<-32)
            {
                if(prvscr.flags6&fWRAPAROUNDFF)
                {
                    prvscr.ffcs[i].x = (288+(prvscr.ffcs[i].x+32));
                    ffprvy[i] = prvscr.ffcs[i].y.getZLong();
                }
                else
                {
                    prvscr.ffcs[i].data = 0;
                    prvscr.ffcs[i].flags&=~ffc_carryover;
                }
            }
            
            if(prvscr.ffcs[i].y<-32)
            {
                if(prvscr.flags6&fWRAPAROUNDFF)
                {
                    prvscr.ffcs[i].y = 208+(prvscr.ffcs[i].y+32);
                    ffprvx[i] = prvscr.ffcs[i].x.getZLong();
                }
                else
                {
                    prvscr.ffcs[i].data = 0;
                    prvscr.ffcs[i].flags&=~ffc_carryover;
                }
            }
            
            if(prvscr.ffcs[i].x>=288)
            {
                if(prvscr.flags6&fWRAPAROUNDFF)
                {
                    prvscr.ffcs[i].x = prvscr.ffcs[i].x-288-32;
                    ffprvy[i] = prvscr.ffcs[i].y.getZLong();
                }
                else
                {
                    prvscr.ffcs[i].data = 0;
                    prvscr.ffcs[i].flags&=~ffc_carryover;
                }
            }
            
            if(prvscr.ffcs[i].y>=208)
            {
                if(prvscr.flags6&fWRAPAROUNDFF)
                {
                    prvscr.ffcs[i].y = prvscr.ffcs[i].y-208-32;
                    ffprvy[i] = prvscr.ffcs[i].x.getZLong();
                }
                else
                {
                    prvscr.ffcs[i].data = 0;
                    prvscr.ffcs[i].flags&=~ffc_carryover;
                }
            }
            
        }
    }
}

void zmap::goto_dmapscr(int dmap, int scr)
{
	setCurrMap(DMaps[dmap].map);
	setCurrScr(scr+DMaps[dmap].xoff);
}
void zmap::goto_mapscr(int map, int scr)
{
	setCurrMap(map);
	setCurrScr(scr);
}

void zmap::dowarp(int32_t type, int32_t index)
{
	set_warpback();
    if(type==0)
    {
    
        int32_t dmap=screens[cursor.screen].tilewarpdmap[index];
        int32_t scr=screens[cursor.screen].tilewarpscr[index];
        
        switch(screens[cursor.screen].tilewarptype[index])
        {
        case wtCAVE:
        case wtNOWARP:
            break;
            
        default:
			goto_dmapscr(dmap, scr);
            break;
        }
    }
    else if(type==1)
    {
        int32_t dmap=screens[cursor.screen].sidewarpdmap[index];
        int32_t scr=screens[cursor.screen].sidewarpscr[index];
        
        switch(screens[cursor.screen].sidewarptype[index])
        {
        case wtCAVE:
        case wtNOWARP:
            break;
            
        default:
			goto_dmapscr(dmap, scr);
            break;
        }
    }
}

extern int32_t prv_twon;

void zmap::prv_dowarp(int32_t type, int32_t index)
{
    if(type==0)
    {
    
        int32_t dmap=prvscr.tilewarpdmap[index];
        int32_t scr=prvscr.tilewarpscr[index];
        
        switch(prvscr.tilewarptype[index])
        {
        case wtCAVE:
        case wtNOWARP:
            break;
            
        default:
            //setCurrMap(DMaps[dmap].map);
            //setCurrScr(scr+DMaps[dmap].xoff);
            set_prvscr(DMaps[dmap].map,scr+DMaps[dmap].xoff);
            loadlvlpal(getcolor());
            rebuild_trans_table();
            //prv_cmbcycle=0;
            break;
        }
    }
    else if(type==1)
    {
        int32_t dmap=prvscr.sidewarpdmap[index];
        int32_t scr=prvscr.sidewarpscr[index];
        
        switch(prvscr.sidewarptype[index])
        {
        case wtCAVE:
        case wtNOWARP:
            break;
            
        default:
            //setCurrMap(DMaps[dmap].map);
            //setCurrScr(scr+DMaps[dmap].xoff);
            set_prvscr(DMaps[dmap].map,scr+DMaps[dmap].xoff);
            loadlvlpal(getcolor());
            rebuild_trans_table();
            //prv_cmbcycle=0;
            break;
        }
    }
    
    if(prv_twon)
    {
        prv_time=get_prvscr()->timedwarptics;
    }
    
    //also reset FFC information (so that changers will work correctly) -DD
    memset(ffposx,0xFF,sizeof(int16_t)*32);
    memset(ffposy,0xFF,sizeof(int16_t)*32);
    memset(ffprvx,0xFF,sizeof(float)*32);
    memset(ffprvy,0xFF,sizeof(float)*32);
}

void zmap::dowarp2(int32_t ring,int32_t index)
{
	set_warpback();
	goto_dmapscr(QMisc.warp[ring].dmap[index], QMisc.warp[ring].scr[index]);
}

void zmap::set_warpback()
{
	warpbackmap = cursor.map;
	warpbackscreen = cursor.screen;
}
bool zmap::has_warpback()
{
	return warpbackmap && warpbackscreen
		&& !(warpbackmap == cursor.map && warpbackscreen == cursor.screen);
}
void zmap::warpback()
{
	if(!has_warpback())
		return;

	int m = cursor.map, s = cursor.screen;
	goto_mapscr(*warpbackmap, *warpbackscreen);
	warpbackmap = m;
	warpbackscreen = s;
}

bool save_msgstrs(const char *path)
{
    PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    if(writestrings(f, ZELDA_VERSION, VERSION_BUILD, 0, MAXMSGS)==0)
    {
        pack_fclose(f);
        return true;
    }
    
    pack_fclose(f);
    return false;
}

bool save_strings_tsv(const char *path)
{
	PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    if(writestrings_tsv(f)==0)
    {
        pack_fclose(f);
        return true;
    }

    pack_fclose(f);
    return false;
}

bool save_msgstrs_text(const char *path)
{
    PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    if(writestrings_text(f)==0)
    {
        pack_fclose(f);
        return true;
    }
    
    pack_fclose(f);
    return false;
}

bool load_msgstrs(const char *path, int32_t startstring)
{
    //these are here to bypass compiler warnings about unused arguments
    startstring=startstring;
    
    dword section_id;
    PACKFILE *f = pack_fopen_password(path,F_READ, "");
    
    if(!f)
    {
        return false;
    }
    
    if(!p_mgetl(&section_id,f))
    {
        return false;
    }
    
    if(section_id==ID_STRINGS)
    {
        if(readstrings(f, &header)==0)
        {
            pack_fclose(f);
            return true;
        }
        else
        {
            pack_fclose(f);
            return false;
        }
    }
    
    pack_fclose(f);
    return false;
}

bool load_strings_tsv(const char *path)
{
	try
	{
		parse_strings_tsv(util::read_text_file(path));
	}
	catch (std::exception& ex)
	{
		InfoDialog("Import .tsv Error", ex.what()).show();
		return false;
	}
	return true;
}

bool save_pals(const char *path)
{
    PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    if(writecolordata(f, ZELDA_VERSION, VERSION_BUILD, 0, newerpdTOTAL)==0)
    {
        pack_fclose(f);
        return true;
    }
    
    pack_fclose(f);
    return false;
}

bool load_pals(const char *path, int32_t startcset)
{
    dword section_id;
    PACKFILE *f = pack_fopen_password(path,F_READ, "");
    
    if(!f)
    {
        return false;
    }
    
    if(!p_mgetl(&section_id,f))
    {
        return false;
    }
    
    if(section_id==ID_CSETS)
    {
        if(readcolordata(f, &QMisc, 0x250, 33, startcset, newerpdTOTAL-startcset)==0)
        {
            pack_fclose(f);
            loadlvlpal(Color);
            return true;
        }
        else
        {
            pack_fclose(f);
            return false;
        }
    }
    
    return false;
}

int32_t writeguys(PACKFILE *f, zquestheader *Header);
bool save_guys(const char *path)
{
    PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    /*
    int32_t id = ID_GUYS;
    if(!p_mputl(id,f))
    {
      return false;
    }
    */
    
    zquestheader h;
    h.zelda_version = 0x250;
    h.build = 21;
    
    if(writeguys(f, &h)==0)
    {
        pack_fclose(f);
        return true;
    }
    
    pack_fclose(f);
    return false;
}

bool load_guys(const char *path)
{
    dword section_id;
    PACKFILE *f = pack_fopen_password(path,F_READ, "");
    
    if(!f)
    {
        return false;
    }
    
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    zquestheader h;
    h.zelda_version = 0x250;
    h.build = 21;
    
    if(section_id==ID_GUYS)
    {
        if(readguys(f, &h)==0)
        {
            pack_fclose(f);
            return true;
        }
    }
    
    pack_fclose(f);
    return false;
}


//int32_t writeguys(PACKFILE *f, zquestheader *Header);
bool save_combo_alias(const char *path)
{
    PACKFILE *f = pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
    {
        return false;
    }
    
    zquestheader h;
    h.zelda_version = 0x250;
    h.build = 21;
    
    if(writecomboaliases(f, 0, 0)==0)
    {
        pack_fclose(f);
        return true;
    }
    
    pack_fclose(f);
    return false;
}

bool load_combo_alias(const char *path)
{
    dword section_id;
    PACKFILE *f = pack_fopen_password(path,F_READ, "");
    
    if(!f)
    {
        return false;
    }
    
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    zquestheader h;
    h.zelda_version = 0x250;
    h.build = 21;
    
    if(section_id==ID_COMBOALIASES)
    {
        if(readcomboaliases(f, &h, 0, 0)==0)
        {
            pack_fclose(f);
            return true;
        }
    }
    
    pack_fclose(f);
    return false;
}

bool load_zgp(const char *path)
{
    dword section_id;
    dword section_version;
    dword section_cversion;
    PACKFILE *f=pack_fopen_password(path,F_READ,"");
    
    if(!f)
        return false;
        
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id!=ID_GRAPHICSPACK)
    {
        pack_fclose(f);
        return false;
    }
    
    //section version info
    if(!p_igetw(&section_version,f))
    {
        return 2;
    }
    
    if(!p_igetw(&section_cversion,f))
    {
        return 3;
    }
    
    //tiles
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_TILES)
    {
        if(readtiles(f, newtilebuf, NULL, ZELDA_VERSION, VERSION_BUILD, 0, NEWMAXTILES, false)!=0)
        {
            pack_fclose(f);
            init_tiles(true, &header);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //combos
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_COMBOS)
    {
        if(readcombos(f, NULL, ZELDA_VERSION, VERSION_BUILD, 0, MAXCOMBOS)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //palettes
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_CSETS)
    {
        if(readcolordata(f, &QMisc, ZELDA_VERSION, VERSION_BUILD, 0, newerpdTOTAL)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //items
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_ITEMS)
    {
        if(readitems(f, ZELDA_VERSION, VERSION_BUILD)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //weapons
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_WEAPONS)
    {
        if(readweapons(f, &header)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //read the triforce pieces info and make sure it worked
    //really do this?
    
    //read the game icons info and make sure it worked
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_ICONS)
    {
        if(readgameicons(f, &header, &QMisc)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //read the misc colors info and map styles info and make sure it worked
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_COLORS)
    {
        if(readmisccolors(f, &header, &QMisc)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //read the door combo sets and make sure it worked
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id==ID_DOORS)
    {
        if(readdoorcombosets(f, &header)!=0)
        {
            pack_fclose(f);
            return false;
        }
    }
    else
    {
        pack_fclose(f);
        return false;
    }
    
    //read the template screens and make sure it worked
    //really do this?
    
    //yay!  it worked!  close the file and say everything was ok.
    loadlvlpal(Color);
    setup_combo_animations();
    setup_combo_animations2();
    pack_fclose(f);
    return true;
}

bool save_zgp(const char *path)
{
    reset_combo_animations();
    reset_combo_animations2();
    
    //open the file
    PACKFILE *f=pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
        return false;
        
    dword section_id=ID_GRAPHICSPACK;
    dword section_version=V_GRAPHICSPACK;
    dword section_cversion=CV_GRAPHICSPACK;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        return 1;
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        return 2;
    }
    
    if(!p_iputw(section_cversion,f))
    {
        return 3;
    }
    
    //tiles
    if(writetiles(f, ZELDA_VERSION, VERSION_BUILD, 0, NEWMAXTILES)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //combos
    if(writecombos(f, ZELDA_VERSION, VERSION_BUILD, 0, MAXCOMBOS)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //palettes
    if(writecolordata(f, ZELDA_VERSION, VERSION_BUILD, 0, newerpdTOTAL)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //items
    if(writeitems(f, &header)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //weapons
    if(writeweapons(f, &header)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //write the triforce pieces info and make sure it worked
    //really do this?
    
    //write the game icons info and make sure it worked
    if(writegameicons(f, &header)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //write the misc colors info and map styles info and make sure it worked
    if(writemisccolors(f, &header)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //write the door combo sets and make sure it worked
    if(writedoorcombosets(f, &header)!=0)
    {
        pack_fclose(f);
        return false;
    }
    
    //write the template screens and make sure it worked
    //really do this?
    
    pack_fclose(f);
    return true;
}

bool save_subscreen(const char *path, ZCSubscreen const& savefrom)
{
    //open the file
    PACKFILE *f=pack_fopen_password(path,F_WRITE, "");
    
    if(!f)
        return false;
        
    dword section_id=ID_SUBSCREEN;
    dword s_version=V_SUBSCREEN;
    dword s_cversion=CV_SUBSCREEN;
    
    if(!p_mputl(section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(!p_iputw(s_version,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(!p_iputw(s_cversion,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(savefrom.write(f))
    {
        pack_fclose(f);
        return false;
    }
    
    pack_fclose(f);
    return true;
}

bool load_subscreen(const char *path, ZCSubscreen& loadto)
{
    //open the file
    PACKFILE *f=pack_fopen_password(path,F_READ, "");
    
    if(!f)
        return false;
        
    dword section_id;
    dword s_version;
    dword s_cversion;
    
    if(!p_mgetl(&section_id,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(section_id!=ID_SUBSCREEN)
    {
        pack_fclose(f);
        return false;
    }
    
    if(!p_igetw(&s_version,f))
    {
        pack_fclose(f);
        return false;
    }
    
    if(!p_igetw(&s_cversion,f))
    {
        pack_fclose(f);
        return false;
    }
    
	if(s_version < 8)
	{
		subscreen_group g;
		memset(&g,0,sizeof(subscreen_group));
		if(read_one_old_subscreen(f,&g,s_version)!=0)
		{
			pack_fclose(f);
			return false;
		}
		if(g.ss_type != loadto.sub_type)
		{
			pack_fclose(f);
			displayinfo("Failure!",fmt::format("Found subscreen type '{}', expecting type '{}'",
				subscr_names[g.ss_type], subscr_names[loadto.sub_type]));
			return false;
		}
		loadto.clear();
		if(g.objects[0].type != ssoNULL)
			loadto.load_old(g);
	}
	else
	{
		ZCSubscreen tmp = ZCSubscreen();
		if (tmp.read(f, s_version))
		{
			pack_fclose(f);
			return false;
		}
		if(tmp.sub_type != loadto.sub_type)
		{
			pack_fclose(f);
			displayinfo("Failure!",fmt::format("Found subscreen type '{}', expecting type '{}'",
				subscr_names[tmp.sub_type], subscr_names[loadto.sub_type]));
			return false;
		}
		loadto.clear();
		loadto = tmp;
	}
    
    pack_fclose(f);
    return true;
}

bool setMapCount2(int32_t c)
{
    int32_t oldmapcount=map_count;
    int32_t currmap=Map.getCurrMap();
    
    bound(c,1,MAXMAPS);
    map_count=c;
    
    try
    {
        TheMaps.resize(c*MAPSCRS);
		Map.force_refr_pointer();
		map_autolayers.resize(c*6);
    }
    catch(...)
    {
        jwin_alert("Error","Failed to change map count.",NULL,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
        return false;
    }
    
    bound(currmap,0,c-1);
    if(map_count>oldmapcount)
    {
        for(int32_t mc=oldmapcount; mc<map_count; mc++)
        {
            Map.setCurrMap(mc);
            
            for(int32_t ms=0; ms<MAPSCRS; ms++)
            {
                Map.clearscr(ms);
            }
        }
        Map.setCurrMap(currmap);
    }
    else
    {
        Map.setCurrMap(currmap);
        if(!layers_valid(Map.CurrScr()))
            fix_layers(Map.CurrScr(), false);
            
        for(int32_t i=0; i<MAXDMAPS; i++)
        {
            if(DMaps[i].map>=map_count)
            {
                DMaps[i].map=map_count-1;
            }
        }
    }
    
    return true;
}

extern BITMAP *bmap;

static bool loading_file_new = false;
int32_t init_quest(std::string tileset_path)
{
	if (tileset_path.empty())
		tileset_path = DEFAULT_TILESET;

	loading_file_new = true;
    load_quest(tileset_path.c_str());
    loading_file_new = false;

    set_window_title("ZC Editor - Untitled Quest");
    zinit.last_map = 0;
    zinit.last_screen = 0;

    if(bmap != NULL)
    {
        destroy_bitmap(bmap);
        bmap=NULL;
    }
    
    return 0;
}

void set_questpwd(std::string_view pwd, bool use_keyfile)
{
	header.use_keyfile=use_keyfile;

	// string_view actually has some quirks that make it less than ideal here.
	// It'd probably be best to replace it, but this works for now.
	memset(header.password, 0, 256);
	strcpy(header.password, pwd.data());
	header.dirty_password=true;

	cvs_MD5Context ctx;
	cvs_MD5Init(&ctx);
	cvs_MD5Update(&ctx, (const uint8_t*)header.password, strlen(header.password));
	cvs_MD5Final(header.pwd_hash, &ctx);
}


bool is_null_pwd_hash(uint8_t *pwd_hash)
{
    cvs_MD5Context ctx;
    uint8_t md5sum[16];
    char pwd[2]="";
    
    cvs_MD5Init(&ctx);
    cvs_MD5Update(&ctx, (const uint8_t*)pwd, (unsigned)strlen(pwd));
    cvs_MD5Final(md5sum, &ctx);
    
    return (memcmp(md5sum,pwd_hash,16)==0);
}

static DIALOG pwd_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,        0,   0,  224+22+1,   88+10+1,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "Requires Authorization", NULL, NULL },
    { jwin_text_proc,       16,   28,  96,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "File name:", NULL, NULL },
    // 2 (filename)
    { jwin_text_proc,       72,  28, 128,   8,    vc(11),  vc(1),  0,       0,          24,            0,       NULL, NULL, NULL },
    { jwin_text_proc,       16, 38, 0, 8,    vc(15),  vc(1),  0,       0,          0,             0, (void *) "Challenge:", NULL, NULL },
    // 4 (challenge hash)
    { jwin_text_proc,       72, 38, 0, 8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       16,  42+10,  96,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Password:", NULL, NULL },
    // 6 (password)
    { jwin_edit_proc,       72,  38+10,  120+39,   16,    vc(12),  vc(1),  0,       0,          255,            0,       NULL, NULL, NULL },
    { jwin_button_proc,     42,   62+10,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     122,  62+10,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,      0,    0,    0,       0,      0,       0,          0,             0,       NULL, NULL, NULL }
};

int32_t reverse_string(char* str)
{

    if(NULL==str)
    {
        return -1; //no string
    }
    
    int32_t l=(int32_t)strlen(str)-1; //get the string length
    
    if(1==l)
    {
        return 1;
    }
    
    char c;
    
    for(int32_t x=0; x < l; x++,l--)
    {
        c = str[x];
        str[x] = str[l];
        str[l] = c;
    }
    
    return 0;
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunreachable-code"
#endif

int32_t quest_access(const char *filename, zquestheader *hdr)
{
	if (is_headless())
		return 1;

#ifdef __EMSCRIPTEN__
    return 1;
#endif

    //Protection against compiling a release version with password protection off.
    static bool passguard = false;
    
#if ( !(defined _DEBUG) || (defined _RELEASE || defined NDEBUG || defined _NDEBUG) )
#define MUST_HAVE_PASSWORD
    passguard = true;
#endif
    
#if ( !(defined MUST_HAVE_PASSWORD) || defined _NPASS )
#if (defined _MSC_VER || defined _NPASS)
    return 1;
#endif
#endif
    if(devpwd()) return 1;
    
    char hash_string[33];
    
    if((get_debug() && (!(CHECK_CTRL_CMD))) || is_null_pwd_hash(hdr->pwd_hash))
    {
        return 1;
    }
    
	if(check_keyfiles(filename, {KEYFILE_MASTER,KEYFILE_ZPWD}, hdr))
		return true;
	
    pwd_dlg[0].dp2=get_zc_font(font_lfont);
    pwd_dlg[2].dp=get_filename(filename);
    cvs_MD5Context ctx;
    uint8_t md5sum[16]={0};
    char response[33]="";
    char prompt[256]="";
    
    memcpy(md5sum, hdr->pwd_hash, 16);
    
    for(int32_t i=0; i<300; ++i)
    {
        for(int32_t j=0; j<16; ++j)
        {
            sprintf(response+j*2, "%02x", md5sum[j]);
        }
        
        if(i&1)
        {
            reverse_string(response);
        }
        
        if(i==149)
        {
            strcpy(hash_string, response);
        }
        
        cvs_MD5Init(&ctx);
        cvs_MD5Update(&ctx, (const uint8_t*)response, (unsigned)strlen(response));
        cvs_MD5Final(md5sum, &ctx);
    }
    
    pwd_dlg[4].dp=hash_string;
    
    if(get_debug() && (CHECK_CTRL_CMD)) //...what is this doing?
    {
        sprintf(prompt,"%s",response);
    }
    
    pwd_dlg[6].dp=prompt;
    
    large_dialog(pwd_dlg);
        
    int32_t cancel = do_zqdialog(pwd_dlg,6);
    
    if(cancel == 8)
        return 2;
        
    bool ret=check_questpwd(hdr, prompt);
    
    if(!ret)
    {
        ret=(strcmp(response,prompt)==0);
    }
    return ret ? 1 : 0;
}

void set_rules(byte* newrules);
void popup_bugfix_dlg(const char* cfg)
{
	bool dont_show_again = zc_get_config("zquest",cfg,0);
	if(!dont_show_again && hasCompatRulesEnabled())
	{
		AlertDialog("Apply New Bugfixes",
			"New bugfixes found that can be applied to this quest!"
			"\nWould you like to apply them?"
			"\n(Applies 'Bugfix' rule template, un-checking compat rules)",
			[&](bool ret,bool dsa)
			{
				if(ret)
				{
					applyRuleTemplate(ruletemplateFixCompat);
				}
				if(dsa)
				{
					zc_set_config("zquest",cfg,1);
				}
			},
			"Yes","No",
			0,false, //timeout - none
			true //"Don't show this again"
		).show();
	}
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

int32_t load_quest(const char *filename, bool show_progress)
{
	char buf[2048];
	byte skip_flags[4];

	// For File>New, don't clear the maps because tilesets (like Cambria) have a more friendly
	// starter experience that way.
	dword tileset_flags = 0;
	if (loading_file_new)
		tileset_flags = TILESET_CLEARSCRIPTS | TILESET_CLEARHEADER;

	for(int32_t i=0; i<4; ++i)
	{
		skip_flags[i]=0;
	}
	for(int32_t i=0; i<qr_MAX; i++)
				set_qr(i,0);
	int32_t ret=loadquest(filename,&header,&QMisc,customtunes,show_progress,skip_flags,1,true,0,tileset_flags);

	if(ret!=qe_OK)
	{
		init_quest(DEFAULT_TILESET);
	}
	else
	{
		int32_t accessret = quest_access(filename, &header);
		
		if(accessret != 1)
		{
			init_quest(DEFAULT_TILESET);
			
			if(accessret == 0)
				ret=qe_pwd;
			else
				ret=qe_cancel;
		}
		else
		{
			Map.clear();
			Map.setCurrMap(vbound(zinit.last_map,0,map_count-1));
			Map.setCurrScr(zinit.last_screen);

			std::string qst_cfg_header = qst_cfg_header_from_path(filepath);
			Map.setViewSize(zc_get_config(qst_cfg_header.c_str(), "zoom_num_screens", 1));

			extern int32_t current_mappage;
			current_mappage = 0;
			bool found_default = false;
			for(int q = 0; q < MAX_MAPPAGE_BTNS; ++q)
			{
				auto &pg = map_page[q];
				if(pg.map == Map.getCurrMap() && pg.screen == Map.getCurrScr())
				{
					current_mappage = q;
					break;
				}
				else if(found_default || pg.map > 1 || (pg.map == 1 && pg.screen > 0))
					continue;
				else
				{
					current_mappage = q;
					found_default = true;
				}
			}
			refresh(rALL);
			refresh_pal();
			set_rules(quest_rules);
			saved = true;
			if(!(loading_file_new && zc_get_config("zquest","auto_filenew_bugfixes",1)))
				popup_bugfix_dlg("dsa_compatrule");
			
			if(bmap != NULL)
			{
				destroy_bitmap(bmap);
				bmap=NULL;
			}
			
			if (show_progress)
			{
				sprintf(buf,"ZC Editor - [%s]", get_filename(filename));
				set_window_title(buf);
			}
		}
	}

    Map.ClearCommandHistory();

	if (!is_headless())
	{
		void load_size_poses();
		load_size_poses();
	}

	return ret;
}

int32_t load_tileset(const char *filename, dword tsetflags)
{
	char buf[2048];
	byte skip_flags[4];
	
	for(int32_t i=0; i<4; ++i)
		skip_flags[i]=0;
	for(int32_t i=0; i<qr_MAX; i++)
		set_qr(i,0);
	int32_t ret=loadquest(filename,&header,&QMisc,customtunes,true,skip_flags,1,true,0,tsetflags);

	if(ret!=qe_OK)
		init_quest(DEFAULT_TILESET);
	else
	{
		int32_t accessret = quest_access(filename, &header);
		
		if(accessret != 1)
		{
			init_quest(DEFAULT_TILESET);
			
			if(accessret == 0)
				ret=qe_pwd;
			else
				ret=qe_cancel;
		}
		else
		{
			Map.clear();
			Map.setCurrMap(vbound(zinit.last_map,0,map_count-1));
			Map.setCurrScr(zinit.last_screen);
			extern int32_t current_mappage;
			current_mappage = 0;
			bool found_default = false;
			for(int q = 0; q < MAX_MAPPAGE_BTNS; ++q)
			{
				auto &pg = map_page[q];
				if(pg.map == Map.getCurrMap() && pg.screen == Map.getCurrScr())
				{
					current_mappage = q;
					break;
				}
				else if(found_default || pg.map > 1 || (pg.map == 1 && pg.screen > 0))
					continue;
				else
				{
					current_mappage = q;
					found_default = true;
				}
			}
			refresh(rALL);
			refresh_pal();
			set_rules(quest_rules);
			if(!zc_get_config("zquest","auto_filenew_bugfixes",1))
				popup_bugfix_dlg("dsa_compatrule");
			
			if(bmap != NULL)
			{
				destroy_bitmap(bmap);
				bmap=NULL;
			}
			
			set_window_title("ZC Editor - Untitled Quest");
			first_save = saved = false;
			memset(filepath,0,255);
			memset(temppath,0,255);
		}
	}

	Map.ClearCommandHistory();
	
	return ret;
}

bool write_midi(MIDI *m,PACKFILE *f)
{
    int32_t c;
    
    if(!p_mputw(m->divisions,f)) return false;
    
    for(c=0; c<MIDI_TRACKS; c++)
    {
        if(!p_mputl(m->track[c].len,f)) return false;
        
        if(m->track[c].len > 0)
        {
            if(!pfwrite(m->track[c].data,m->track[c].len,f))
                return false;
        }
    }
    
    return true;
}

int32_t writeheader(PACKFILE *f, zquestheader *Header)
{
    dword section_id=ID_HEADER;
    dword section_version=V_HEADER;
    dword section_cversion=CV_HEADER;
    dword section_size=0;
    
    //file header string
    if(!pfwrite(Header->id_str,sizeof(Header->id_str),f))
    {
        new_return(1);
    }
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(2);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(3);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(4);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(5);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(Header->zelda_version,f))
        {
            new_return(6);
        }
        
        if(!p_putc(Header->build,f))
        {
            new_return(7);
        }
        
        if(!pfwrite(Header->pwd_hash,sizeof(Header->pwd_hash),f))
        {
            new_return(8);
        }
        
        if(!p_iputw(Header->internal,f))
        {
            new_return(10);
        }
        
        if(!p_putc(Header->quest_number,f))
        {
            new_return(11);
        }
        
        if(!pfwrite(Header->version,16,f))
        {
            new_return(12);
        }
        
        if(!pfwrite(Header->minver,16,f))
        {
            new_return(13);
        }
        
        if(!pfwrite(Header->title,sizeof(Header->title),f))
        {
            new_return(14);
        }
        
        if(!pfwrite(Header->author,sizeof(Header->author),f))
        {
            new_return(15);
        }
        
        if(!p_putc(Header->use_keyfile,f))
        {
            new_return(16);
        }
        
        if(!pfwrite(Header->data_flags,sizeof(Header->data_flags),f))
        {
            new_return(17);
        }
        
        if(!pfwrite(Header->templatepath,sizeof(Header->templatepath),f))
        {
            new_return(19);
        }
        
        if(!p_putc(0,f)) //why are we doing this? 
		//this is for map count, it seems. -Z
        {
            new_return(20);
        }
		
		auto version = getVersion();
		
		if(!p_iputl(version.major,f))
		{
			new_return(21);
		}
		if(!p_iputl(version.minor,f))
		{
			new_return(22);
		}
		if(!p_iputl(version.patch,f))
		{
			new_return(23);
		}
		// Fourth component is deprecated.
		if(!p_iputl(0,f))
		{
			new_return(24);
		}

		// Numerous prerelease stages is deprecated.
		if(!p_iputl(0,f))
		{
			new_return(25);
		}
		if(!p_iputl(0,f))
		{
			new_return(26);
		}
		if(!p_iputl(0,f))
		{
			new_return(27);
		}
		if(!p_iputl(0,f))
		{
			new_return(28);
		}

		if(!p_iputw(BUILDTM_YEAR,f))
		{
			new_return(29);
		}
		if(!p_putc(BUILDTM_MONTH,f))
		{
			new_return(30);
		}
		if(!p_putc(BUILDTM_DAY,f))
		{
			new_return(31);
		}
		if(!p_putc(BUILDTM_HOUR,f))
		{
			new_return(32);
		}
		if(!p_putc(BUILDTM_MINUTE,f))
		{
			new_return(33);
		}
		
		
		
		char tempsig[256];
		memset(tempsig, 0, 256);
		strcpy(tempsig, DEV_SIGNOFF);
		
		if(!pfwrite(&tempsig,256,f))
		{
			new_return(34);
		}
		
		char tempcompilersig[256];
		memset(tempcompilersig, 0, 256);
		strcpy(tempcompilersig, COMPILER_NAME);
		
		if(!pfwrite(&tempcompilersig,256,f))
		{
			new_return(35);
		}
		
		char tempcompilerversion[256];
		memset(tempcompilerversion, 0, 256); 
		#ifdef _MSC_VER
		zc_itoa(_MSC_VER,tempcompilerversion,10);
		#else
		strcpy(tempcompilerversion, COMPILER_VERSION);
		#endif
		
		
		if(!pfwrite(&tempcompilerversion,256,f))
        {
            new_return(36);
        }
		
		if(!pfwrite("ZQuest Classic",1024,f))
        {
            new_return(37);
        }
		
		if(!p_putc(V_ZC_COMPILERSIG,f))
		{
			new_return(38);
		}
		#ifdef _MSC_VER
		if(!p_iputl((_MSC_VER / 100),f))
		{
		    new_return(39);
		}
		#else
		if(!p_iputl(COMPILER_V_FIRST,f))
		{
		    new_return(39);
		}
		#endif
		
		
		
		#ifdef _MSC_VER
		if(!p_iputl((_MSC_VER % 100),f)) 
		{
			new_return(41);
		}
		#else
		if(!p_iputl(COMPILER_V_SECOND,f)) 
		{
			new_return(41);
		}
		#endif
		
		#ifdef _MSC_VER
			# if _MSC_VER >= 1400
			if(!p_iputl((_MSC_FULL_VER % 100000),f))
			{
				new_return(40);
			}
			# else
			if(!p_iputl((_MSC_FULL_VER % 10000),f))
			{
				new_return(40);
			}
			#endif
		#else	
		if(!p_iputl(COMPILER_V_THIRD,f))
		{
				new_return(40);
		}
		#endif
		
		#ifdef _MSC_VER
		if(!p_iputl((_MSC_BUILD),f))
		{
			new_return(42);
		}
		#else
		if(!p_iputl(COMPILER_V_FOURTH,f))
		{
			new_return(42);
		}
		#endif
		if(!p_iputw(0,f)) //was V_ZC_DEVSIG, no longer used
		{
			new_return(43);
		}
		
		char tempmodulename[1024];
		memset(tempmodulename, 0, 1024);
		strcpy(tempmodulename, moduledata.module_name);
		
		if(!pfwrite(&tempmodulename,1024,f))
        {
            new_return(44);
        }
		
		char tempdate[256];
		memset(tempdate, 0, 256);
		strcpy(tempdate, __DATE__);
		
		if(!pfwrite(&tempdate,256,f))
        {
            new_return(45);
        }
		char temptime[256];
		memset(temptime, 0, 256);
		strcpy(temptime, __TIME__);
		
		if(!pfwrite(&temptime,256,f))
        {
            new_return(46);
        }
		
		
		char temptimezone[6];
		memset(temptimezone, 0, 6);
		strcpy(temptimezone, __TIMEZONE__);
		if(!pfwrite(&temptimezone,6,f))
        {
            new_return(47);
        }
		
		if(!p_putc(Header->external_zinfo ? 1 : 0, f))
		{
			new_return(48);
		}
		
		if(!p_putc(isStableRelease() ? 0 : 1, f))
		{
			new_return(49);
		}

		if(!p_putcstr(version.version_string, f))
		{
			new_return(50);
		}

        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeheader()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writerules(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_RULES;
    dword section_version=V_RULES;
    dword section_cversion=CV_RULES;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
	
	if(!p_iputl(V_COMPATRULE,f))
	{
		new_return(6);
	}
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!pfwrite(quest_rules,QUESTRULES_NEW_SIZE,f))
        {
            new_return(5);
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writerules()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}


int32_t writedoorcombosets(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_DOORS;
    dword section_version=V_DOORS;
    dword section_cversion=CV_DOORS;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(door_combo_set_count,f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<door_combo_set_count; i++)
        {
            //name
            char name[21];
            memset(name, 21, (char)0);
            strcpy(name, DoorComboSetNames[i].c_str());
            if(!pfwrite(name,sizeof(name),f))
            {
                new_return(6);
            }
            
            //up door
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<4; k++)
                {
                    if(!p_iputw(DoorComboSets[i].doorcombo_u[j][k],f))
                    {
                        new_return(7);
                    }
                }
            }
            
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<4; k++)
                {
                    if(!p_putc(DoorComboSets[i].doorcset_u[j][k],f))
                    {
                        new_return(8);
                    }
                }
            }
            
            //down door
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<4; k++)
                {
                    if(!p_iputw(DoorComboSets[i].doorcombo_d[j][k],f))
                    {
                        new_return(9);
                    }
                }
            }
            
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<4; k++)
                {
                    if(!p_putc(DoorComboSets[i].doorcset_d[j][k],f))
                    {
                        new_return(10);
                    }
                }
            }
            
            
            //left door
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<6; k++)
                {
                    if(!p_iputw(DoorComboSets[i].doorcombo_l[j][k],f))
                    
                    {
                        new_return(11);
                    }
                }
            }
            
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<6; k++)
                {
                    if(!p_putc(DoorComboSets[i].doorcset_l[j][k],f))
                    {
                        new_return(12);
                    }
                }
            }
            
            //right door
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<6; k++)
                {
                    if(!p_iputw(DoorComboSets[i].doorcombo_r[j][k],f))
                    {
                        new_return(13);
                    }
                }
            }
            
            for(int32_t j=0; j<9; j++)
            {
                for(int32_t k=0; k<6; k++)
                {
                    if(!p_putc(DoorComboSets[i].doorcset_r[j][k],f))
                    {
                        new_return(14);
                    }
                }
            }
            
            
            //up bomb rubble
            for(int32_t j=0; j<2; j++)
            {
                if(!p_iputw(DoorComboSets[i].bombdoorcombo_u[j],f))
                {
                    new_return(15);
                }
            }
            
            for(int32_t j=0; j<2; j++)
            {
                if(!p_putc(DoorComboSets[i].bombdoorcset_u[j],f))
                {
                    new_return(16);
                }
            }
            
            //down bomb rubble
            for(int32_t j=0; j<2; j++)
            {
                if(!p_iputw(DoorComboSets[i].bombdoorcombo_d[j],f))
                {
                    new_return(17);
                }
            }
            
            for(int32_t j=0; j<2; j++)
            {
                if(!p_putc(DoorComboSets[i].bombdoorcset_d[j],f))
                {
                    new_return(18);
                }
            }
            
            //left bomb rubble
            for(int32_t j=0; j<3; j++)
            {
                if(!p_iputw(DoorComboSets[i].bombdoorcombo_l[j],f))
                {
                    new_return(19);
                }
            }
            
            for(int32_t j=0; j<3; j++)
            {
                if(!p_putc(DoorComboSets[i].bombdoorcset_l[j],f))
                {
                    new_return(20);
                }
            }
            
            //right bomb rubble
            for(int32_t j=0; j<3; j++)
            {
                if(!p_iputw(DoorComboSets[i].bombdoorcombo_r[j],f))
                {
                    new_return(21);
                }
            }
            
            for(int32_t j=0; j<3; j++)
            {
                if(!p_putc(DoorComboSets[i].bombdoorcset_r[j],f))
                {
                    new_return(22);
                }
            }
            
            //walkthrough stuff
            for(int32_t j=0; j<4; j++)
            {
                if(!p_iputw(DoorComboSets[i].walkthroughcombo[j],f))
                {
                    new_return(23);
                }
            }
            
            for(int32_t j=0; j<4; j++)
            {
                if(!p_putc(DoorComboSets[i].walkthroughcset[j],f))
                {
                    new_return(24);
                }
            }
            
            //flags
            for(int32_t j=0; j<2; j++)
            {
                if(!p_putc(DoorComboSets[i].flags[j],f))
                {
                    new_return(25);
                }
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writedoorcombosets()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writedmaps(PACKFILE *f, word version, word build, word start_dmap, word max_dmaps)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    
    word dmap_count=count_dmaps();
    dword section_id=ID_DMAPS;
    dword section_version=V_DMAPS;
    dword section_cversion=CV_DMAPS;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        dmap_count=zc_min(dmap_count, max_dmaps);
        dmap_count=zc_min(dmap_count, MAXDMAPS-start_dmap);
        
        //finally...  section data
        if(!p_iputw(dmap_count,f))
        {
            new_return(5);
        }
        
        
        for(int32_t i=start_dmap; i<start_dmap+dmap_count; i++)
        {
			DMaps[i].validate_subscreens();
			
            if(!p_putc(DMaps[i].map,f))
            {
                new_return(6);
            }
            
            if(!p_iputw(DMaps[i].level,f))
            {
                new_return(7);
            }
            
            if(!p_putc(DMaps[i].xoff,f))
            {
                new_return(8);
            }
            
            if(!p_putc(DMaps[i].compass,f))
            {
                new_return(9);
            }
            
            if(!p_iputw(DMaps[i].color,f))
            {
                new_return(10);
            }
            
            if(!p_putc(DMaps[i].midi,f))
            {
                new_return(11);
            }
            
            if(!p_putc(DMaps[i].cont,f))
            {
                new_return(12);
            }
            
            if(!p_putc(DMaps[i].type,f))
            {
                new_return(13);
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_putc(DMaps[i].grid[j],f))
                {
                    new_return(14);
                }
            }
            
            if(!pfwrite(&DMaps[i].name,sizeof(DMaps[0].name)-1,f))
            {
                new_return(15);
            }
            
            if(!p_putwstr(DMaps[i].title,f))
            {
                new_return(16);
            }
            
            if(!pfwrite(&DMaps[i].intro,sizeof(DMaps[0].intro)-1,f))
            {
                new_return(17);
            }
            
            if(!p_iputl(DMaps[i].minimap_1_tile,f))
            {
                new_return(18);
            }
            
            if(!p_putc(DMaps[i].minimap_1_cset,f))
            {
                new_return(19);
            }
            
            if(!p_iputl(DMaps[i].minimap_2_tile,f))
            {
                new_return(20);
            }
            
            if(!p_putc(DMaps[i].minimap_2_cset,f))
            {
                new_return(21);
            }
            
            if(!p_iputl(DMaps[i].largemap_1_tile,f))
            {
                new_return(22);
            }
            
            if(!p_putc(DMaps[i].largemap_1_cset,f))
            {
                new_return(23);
            }
            
            if(!p_iputl(DMaps[i].largemap_2_tile,f))
            {
                new_return(24);
            }
            
            if(!p_putc(DMaps[i].largemap_2_cset,f))
            {
                new_return(25);
            }
            
            if(!pfwrite(&DMaps[i].tmusic,sizeof(DMaps[0].tmusic)-1,f))
            {
                new_return(26);
            }
            
            if(!p_putc(DMaps[i].tmusictrack,f))
            {
                new_return(25);
            }
            
            if(!p_putc(DMaps[i].active_subscreen,f))
            {
                new_return(26);
            }
            
            if(!p_putc(DMaps[i].passive_subscreen,f))
            {
                new_return(27);
            }
            
            byte disabled[32];
            memset(disabled,0,32);
            
            for(int32_t j=0; j<MAXITEMS; j++)
            {
                if(DMaps[i].disableditems[j])
                {
                    disabled[j/8] |= (1 << (j%8));
                }
            }
            
            if(!pfwrite(disabled,32,f))
            {
                new_return(28);
            }
            
            if(!p_iputl(DMaps[i].flags,f))
            {
                new_return(29);
            }
	    if(!p_putc(DMaps[i].sideview,f))
            {
                new_return(30);
            }
	    if(!p_iputw(DMaps[i].script,f))
            {
                new_return(31);
            }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		if(!p_iputl(DMaps[i].initD[q],f))
	        {
			new_return(32);
		}
		    
	    }
	    for ( int32_t q = 0; q < 8; q++ )
	    {
		    for ( int32_t w = 0; w < 65; w++ )
		    {
			if (!p_putc(DMaps[i].initD_label[q][w],f))
			{
				new_return(33);
			}
		}
	    }
			if(!p_iputw(DMaps[i].active_sub_script,f))
			{
				new_return(34);
			}
			if(!p_iputw(DMaps[i].passive_sub_script,f))
			{
				new_return(35);
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				if(!p_iputl(DMaps[i].sub_initD[q],f))
				{
					new_return(36);
				}
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				for(int32_t w = 0; w < 65; ++w)
				{
					if(!p_putc(DMaps[i].sub_initD_label[q][w],f))
					{
						new_return(37);
					}
				}
			}
			if(!p_iputw(DMaps[i].onmap_script,f))
			{
				new_return(38);
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				if(!p_iputl(DMaps[i].onmap_initD[q],f))
				{
					new_return(39);
				}
			}
			for(int32_t q = 0; q < 8; ++q)
			{
				for(int32_t w = 0; w < 65; ++w)
				{
					if(!p_putc(DMaps[i].onmap_initD_label[q][w],f))
					{
						new_return(40);
					}
				}
			}
			if(!p_iputw(DMaps[i].mirrorDMap,f))
			{
				new_return(41);
			}
            if (!p_iputl(DMaps[i].tmusic_loop_start, f))
            {
                new_return(42);
            }
            if (!p_iputl(DMaps[i].tmusic_loop_end, f))
            {
                new_return(43);
            }
            if (!p_iputl(DMaps[i].tmusic_xfade_in, f))
            {
                new_return(44);
            }
            if (!p_iputl(DMaps[i].tmusic_xfade_out, f))
            {
                new_return(45);
            }
			if(!p_putc(DMaps[i].overlay_subscreen, f))
				new_return(46);
			if (!p_iputl(DMaps[i].intro_string_id, f))
				new_return(47);

			// Reserved for z3.
			for(int32_t j=0; j<8; j++)
			{
				for(int32_t k=0; k<8; k++)
				{
					if(!p_putc(0,f))
					{
						new_return(48);
					}
				}
			}
		}
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writedmaps()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writemisccolors(PACKFILE *f, zquestheader *Header)
{
	//these are here to bypass compiler warnings about unused arguments
	Header=Header;
	
	dword section_id=ID_COLORS;
	dword section_version=V_COLORS;
	dword section_cversion=CV_COLORS;
	dword section_size = 0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
		{
			new_return(4);
		}
		
		writesize=0;
		
		if(!p_putc(QMisc.colors.text,f))
		{
			new_return(5);
		}
		
		if(!p_putc(QMisc.colors.caption,f))
		{
			new_return(6);
		}
		
		if(!p_putc(QMisc.colors.overw_bg,f))
		{
			new_return(7);
		}
		
		if(!p_putc(QMisc.colors.dngn_bg,f))
		{
			new_return(8);
		}
		
		if(!p_putc(QMisc.colors.dngn_fg,f))
		{
			new_return(9);
		}
		
		if(!p_putc(QMisc.colors.cave_fg,f))
		{
			new_return(10);
		}
		
		if(!p_putc(QMisc.colors.bs_dk,f))
		{
			new_return(11);
		}
		
		if(!p_putc(QMisc.colors.bs_goal,f))
		{
			new_return(12);
		}
		
		if(!p_putc(QMisc.colors.compass_lt,f))
		{
			new_return(13);
		}
		
		if(!p_putc(QMisc.colors.compass_dk,f))
		{
			new_return(14);
		}
		
		if(!p_putc(QMisc.colors.subscr_bg,f))
		{
			new_return(15);
		}
		
		if(!p_putc(QMisc.colors.triframe_color,f))
		{
			new_return(16);
		}
		
		if(!p_putc(QMisc.colors.hero_dot,f))
		{
			new_return(17);
		}
		
		if(!p_putc(QMisc.colors.bmap_bg,f))
		{
			new_return(18);
		}
		
		if(!p_putc(QMisc.colors.bmap_fg,f))
		{
			new_return(19);
		}
		
		if(!p_putc(QMisc.colors.triforce_cset,f))
		{
			new_return(20);
		}
		
		if(!p_putc(QMisc.colors.triframe_cset,f))
		{
			new_return(21);
		}
		
		if(!p_putc(QMisc.colors.overworld_map_cset,f))
		{
			new_return(22);
		}
		
		if(!p_putc(QMisc.colors.dungeon_map_cset,f))
		{
			new_return(23);
		}
		
		if(!p_putc(QMisc.colors.blueframe_cset,f))
		{
			new_return(24);
		}
		
		if(!p_putc(QMisc.colors.HCpieces_cset,f))
		{
			new_return(31);
		}
		
		if(!p_putc(QMisc.colors.subscr_shadow,f))
		{
			new_return(32);
		}
		
		if(!p_putc(QMisc.colors.msgtext,f))
		{
			new_return(33);
		}
	
		if(!p_iputl(QMisc.colors.triforce_tile,f))
		{
			new_return(34);
		}
		
		if(!p_iputl(QMisc.colors.triframe_tile,f))
		{
			new_return(35);
		}
		
		if(!p_iputl(QMisc.colors.overworld_map_tile,f))
		{
			new_return(36);
		}
		
		if(!p_iputl(QMisc.colors.dungeon_map_tile,f))
		{
			new_return(37);
		}
		
		if(!p_iputl(QMisc.colors.blueframe_tile,f))
		{
			new_return(38);
		}
		
		if(!p_iputl(QMisc.colors.HCpieces_tile,f))
		{
			new_return(39);
		}
		
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writemisccolors()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

int32_t writegameicons(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_ICONS;
    dword section_version=V_ICONS;
    dword section_cversion=CV_ICONS;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(QMisc.icons[i],f))
            {
                new_return(5);
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writegameicons()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writemisc(PACKFILE *f, zquestheader *Header)
{
	//these are here to bypass compiler warnings about unused arguments
	Header=Header;
	
	dword section_id=ID_MISC;
	dword section_version=V_MISC;
	dword section_cversion=CV_MISC;
	word shops=count_shops(&QMisc);
	word infos=count_infos(&QMisc);
	word warprings=count_warprings(&QMisc);
	word triforces=8;
	dword section_size = 0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
		{
			new_return(4);
		}
		
		writesize=0;
		
		//shops
		if(!p_iputw(shops,f))
		{
			new_return(5);
		}
		
		for(int32_t i=0; i<shops; i++)
		{
			if(!pfwrite(QMisc.shop[i].name,sizeof(QMisc.shop[i].name)-1,f))
			{
				new_return(6);
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_putc(QMisc.shop[i].item[j],f))
				{
					new_return(7);
				}
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_iputw(QMisc.shop[i].price[j],f))
				{
					new_return(8);
				}
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_putc(QMisc.shop[i].hasitem[j],f))
				{
					new_return(9);
				}
			}
		}
		
		//infos
		if(!p_iputw(infos,f))
		{
			new_return(10);
		}
		
		for(int32_t i=0; i<infos; i++)
		{
			if(!pfwrite(QMisc.info[i].name,sizeof(QMisc.info[i].name)-1,f))
			{
				new_return(11);
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_iputw(QMisc.info[i].str[j],f))
				{
					new_return(12);
				}
			}
			
			for(int32_t j=0; j<3; j++)
			{
				if(!p_iputw(QMisc.info[i].price[j],f))
				{
					new_return(13);
				}
			}
		}
		
		//warp rings
		if(!p_iputw(warprings,f))
		{
			new_return(14);
		}
		
		for(int32_t i=0; i<warprings; i++)
		{
			for(int32_t j=0; j<9; j++)
			{
				if(!p_iputw(QMisc.warp[i].dmap[j],f))
				{
					new_return(15);
				}
			}
			
			for(int32_t j=0; j<9; j++)
			{
				if(!p_putc(QMisc.warp[i].scr[j],f))
				{
					new_return(16);
				}
			}
			
			if(!p_putc(QMisc.warp[i].size,f))
			{
				new_return(17);
			}
		}
		
		//triforce pieces
		for(int32_t i=0; i<triforces; i++)
		{
			if(!p_putc(QMisc.triforce[i],f))
			{
				new_return(18);
			}
		}
		
		//end string
		if(!p_iputw(QMisc.endstring,f))
		{
			new_return(19);
		}
		
		//V_MISC >= 8
		for(int32_t i=0; i<shops; i++)
		{
			for(int32_t j=0; j<3; j++)
			{
				if(!p_iputw(QMisc.shop[i].str[j],f))
				{
					new_return(20);
				}
			}
		}
		//V_MISC >= 9
		for ( int32_t q = 0; q < 32; q++ ) 
		{
			if(!p_iputl(QMisc.questmisc[q],f))
						new_return(21);
		}
		for ( int32_t q = 0; q < 32; q++ ) 
		{
			for ( int32_t j = 0; j < 128; j++ )
				if(!p_putc(0,f))
					new_return(22);
		}
		//V_MISC >= 11
		if(!p_iputl(QMisc.zscript_last_compiled_version,f))
			new_return(23);
		
		//V_MISC >= 12
		for(int32_t q = 0; q < sprMAX; ++q)
		{
			if(!p_putc(QMisc.sprites[q],f))
				new_return(24);
		}
		
		//V_MISC >= 13
		for(size_t q = 0; q < 64; ++q)
		{
			bottletype* bt = &(QMisc.bottle_types[q]);
            if (!pfwrite(bt->name, 32, f))
                new_return(25);
			for(size_t j = 0; j < 3; ++j)
			{
                if (!p_putc(bt->counter[j], f))
                    new_return(25);
                if (!p_iputw(bt->amount[j], f))
                    new_return(25);
			}
            if (!p_putc(bt->flags, f))
                new_return(25);
            if (!p_putc(bt->next_type, f))
                new_return(25);
		}
		for(size_t q = 0; q < 256; ++q)
		{
			bottleshoptype* bst = &(QMisc.bottle_shop_types[q]);
            if (!pfwrite(bst->name, 32, f))
                new_return(26);
			for(size_t j = 0; j < 3; ++j)
			{
                if (!p_putc(bst->fill[j], f))
                    new_return(26);
                if (!p_iputw(bst->comb[j], f))
                    new_return(26);
                if (!p_putc(bst->cset[j], f))
                    new_return(26);
                if (!p_iputw(bst->price[j], f))
                    new_return(26);
                if (!p_iputw(bst->str[j], f))
                    new_return(26);
			}
		}
		
		//V_MISC >= 14
		for(int32_t q = 0; q < sfxMAX; ++q)
		{
			if(!p_putc(QMisc.miscsfx[q],f))
				new_return(27);
		}
		
		for(int q = 0; q < NUM_STATUSES; ++q)
		{
			if(!p_putcstr(QMisc.status_names[q], f))
				new_return(27);
			auto ret = QMisc.status_effects[q].write(f);
			fake_pack_writing=(writecycle==0);
			
			if(ret)
				new_return(ret);
		}
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writemisc()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

int32_t writeitems(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_ITEMS;
    dword section_version=V_ITEMS;
    dword section_cversion=CV_ITEMS;
    //  dword section_size=0;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(MAXITEMS,f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<MAXITEMS; i++)
        {
            if(!pfwrite(item_string[i], 64, f))
            {
                new_return(5);
            }
        }
        
        for(int32_t i=0; i<MAXITEMS; i++)
        {
            if(!p_iputl(itemsbuf[i].tile,f))
            {
                new_return(6);
            }
            
            if(!p_putc(itemsbuf[i].misc_flags,f))
            {
                new_return(7);
            }
            
            if(!p_putc(itemsbuf[i].csets,f))
            {
                new_return(8);
            }
            
            if(!p_putc(itemsbuf[i].frames,f))
            {
                new_return(9);
            }
            
            if(!p_putc(itemsbuf[i].speed,f))
            {
                new_return(10);
            }
            
            if(!p_putc(itemsbuf[i].delay,f))
            {
                new_return(11);
            }
            
            if(!p_iputl(itemsbuf[i].ltm,f))
            {
                new_return(12);
            }
            
            if(!p_iputl(itemsbuf[i].family,f))
            {
                new_return(13);
            }
            
            if(!p_putc(itemsbuf[i].fam_type,f))
            {
                new_return(14);
            }
            
            if(!p_iputl(itemsbuf[i].power,f))
            {
                new_return(14);
            }
            
            if(!p_iputl(itemsbuf[i].flags,f))
            {
                new_return(15);
            }
            
            if(!p_iputw(itemsbuf[i].script,f))
            {
                new_return(16);
            }
            
            if(!p_putc(itemsbuf[i].count,f))
            {
                new_return(17);
            }
            
            if(!p_iputw(itemsbuf[i].amount,f))
            {
                new_return(18);
            }
            
            if(!p_iputw(itemsbuf[i].collect_script,f))
            {
                new_return(19);
            }
            
            if(!p_iputw(itemsbuf[i].setmax,f))
            {
                new_return(21);
            }
            
            if(!p_iputw(itemsbuf[i].max,f))
            {
                new_return(22);
            }
            
            if(!p_putc(itemsbuf[i].playsound,f))
            {
                new_return(23);
            }
            
            for(int32_t j=0; j<8; j++)
            {
                if(!p_iputl(itemsbuf[i].initiald[j],f))
                {
                    new_return(24);
                }
            }
            
            for(int32_t j=0; j<2; j++)
            {
                if(!p_putc(0,f))
                {
                    new_return(25);
                }
            }
            
            if(!p_putc(itemsbuf[i].wpn,f))
            {
                new_return(26);
            }
            
            if(!p_putc(itemsbuf[i].wpn2,f))
            {
                new_return(27);
            }
            
            if(!p_putc(itemsbuf[i].wpn3,f))
            {
                new_return(28);
            }
            
            if(!p_putc(itemsbuf[i].wpn4,f))
            {
                new_return(29);
            }
            
            if(!p_putc(itemsbuf[i].wpn5,f))
            {
                new_return(30);
            }
            
            if(!p_putc(itemsbuf[i].wpn6,f))
            {
                new_return(31);
            }
            
            if(!p_putc(itemsbuf[i].wpn7,f))
            {
                new_return(32);
            }
            
            if(!p_putc(itemsbuf[i].wpn8,f))
            {
                new_return(33);
            }
            
            if(!p_putc(itemsbuf[i].wpn9,f))
            {
                new_return(34);
            }
            
            if(!p_putc(itemsbuf[i].wpn10,f))
            {
                new_return(35);
            }
            
            if(!p_putc(itemsbuf[i].pickup_hearts,f))
            {
                new_return(36);
            }
            
            if(!p_iputl(itemsbuf[i].misc1,f))
            {
                new_return(37);
            }
            
            if(!p_iputl(itemsbuf[i].misc2,f))
            {
                new_return(38);
            }
            
			for(auto q = 0; q < 2; ++q)
			{
				if(!p_iputw(itemsbuf[i].cost_amount[q],f))
				{
					new_return(39);
				}
			}
            
            if(!p_iputl(itemsbuf[i].misc3,f))
            {
                new_return(40);
            }
            
            if(!p_iputl(itemsbuf[i].misc4,f))
            {
                new_return(41);
            }
            
            if(!p_iputl(itemsbuf[i].misc5,f))
            {
                new_return(42);
            }
            
            if(!p_iputl(itemsbuf[i].misc6,f))
            {
                new_return(43);
            }
            
            if(!p_iputl(itemsbuf[i].misc7,f))
            {
                new_return(44);
            }
            
            if(!p_iputl(itemsbuf[i].misc8,f))
            {
                new_return(45);
            }
            
            if(!p_iputl(itemsbuf[i].misc9,f))
            {
                new_return(46);
            }
            
            if(!p_iputl(itemsbuf[i].misc10,f))
            {
                new_return(47);
            }
            
            if(!p_putc(itemsbuf[i].usesound,f))
            {
                new_return(48);
            }
            
            if(!p_putc(itemsbuf[i].usesound2,f))
            {
                new_return(48);
            }
			
			//New itemdata vars -Z
			//! version 27
			
			if(!p_putc(itemsbuf[i].useweapon,f))
			{
				new_return(49);
			}
			if(!p_putc(itemsbuf[i].usedefence,f))
			{
				new_return(50);
			}
			if(!p_iputl(itemsbuf[i].weaprange,f))
			{
				new_return(51);
			}
			if(!p_iputl(itemsbuf[i].weapduration,f))
			{
				new_return(52);
			}
			for ( int32_t q = 0; q < ITEM_MOVEMENT_PATTERNS; q++ ) {
				if(!p_iputl(itemsbuf[i].weap_pattern[q],f))
				{
					new_return(53);
				}
			}
			//version 28
			if(!p_iputl(itemsbuf[i].duplicates,f))
			{
				new_return(54);
			}
			for ( int32_t q = 0; q < INITIAL_D; q++ )
			{
				if(!p_iputl(itemsbuf[i].weap_initiald[q],f))
				{
					new_return(55);
				}
			}
			for ( int32_t q = 0; q < 2; q++ )
			{
				if(!p_putc(0,f))
				{
					new_return(56);
				}
			}

			if(!p_putc(itemsbuf[i].drawlayer,f))
			{
				new_return(57);
			}


			if(!p_iputl(itemsbuf[i].hxofs,f))
			{
				new_return(58);
			}
			if(!p_iputl(itemsbuf[i].hyofs,f))
			{
				new_return(59);
			}
			if(!p_iputl(itemsbuf[i].hxsz,f))
			{
				new_return(60);
			}
			if(!p_iputl(itemsbuf[i].hysz,f))
			{
				new_return(61);
			}
			if(!p_iputl(itemsbuf[i].hzsz,f))
			{
				new_return(62);
			}
			if(!p_iputl(itemsbuf[i].xofs,f))
			{
				new_return(63);
			}
			if(!p_iputl(itemsbuf[i].yofs,f))
			{
				new_return(64);
			}
			if(!p_iputl(itemsbuf[i].weap_hxofs,f))
			{
				new_return(65);
			}
			if(!p_iputl(itemsbuf[i].weap_hyofs,f))
			{
				new_return(66);
			}
			if(!p_iputl(itemsbuf[i].weap_hxsz,f))
			{
				new_return(67);
			}
			if(!p_iputl(itemsbuf[i].weap_hysz,f))
			{
				new_return(68);
			}
			if(!p_iputl(itemsbuf[i].weap_hzsz,f))
			{
				new_return(69);
			}
			if(!p_iputl(itemsbuf[i].weap_xofs,f))
			{
				new_return(70);
			}
			if(!p_iputl(itemsbuf[i].weap_yofs,f))
			{
				new_return(71);
			}
			if(!p_iputw(itemsbuf[i].weaponscript,f))
			{
				new_return(72);
			}
			if(!p_iputl(itemsbuf[i].wpnsprite,f))
			{
				new_return(73);
			}
			for(auto q = 0; q < 2; ++q)
			{
				if(!p_iputl(itemsbuf[i].magiccosttimer[q],f))
				{
					new_return(74);
				}
			}
			if(!p_iputl(itemsbuf[i].overrideFLAGS,f))
			{
				new_return(75);
			}
			if(!p_iputl(itemsbuf[i].tilew,f))
			{
				new_return(76);
			}
			if(!p_iputl(itemsbuf[i].tileh,f))
			{
				new_return(77);
			}
			if(!p_iputl(itemsbuf[i].weapoverrideFLAGS,f))
			{
				new_return(78);
			}
			if(!p_iputl(itemsbuf[i].weap_tilew,f))
			{
				new_return(79);
			}
			if(!p_iputl(itemsbuf[i].weap_tileh,f))
			{
				new_return(80);
			}
			if(!p_iputl(itemsbuf[i].pickup,f))
			{
				new_return(81);
			}
			if(!p_iputw(itemsbuf[i].pstring,f))
			{
				new_return(82);
			}
			if(!p_iputw(itemsbuf[i].pickup_string_flags,f))
			{
				new_return(83);
			}
			
			for(auto q = 0; q < 2; ++q)
			{
				if(!p_putc(itemsbuf[i].cost_counter[q],f))
				{
					new_return(84);
				}
			}
			
			//InitD[] labels
			for ( int32_t q = 0; q < 8; q++ )
			{
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_putc(itemsbuf[i].initD_label[q][w],f))
					{
						new_return(85);
					} 
				}
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_putc(itemsbuf[i].weapon_initD_label[q][w],f))
					{
						new_return(86);
					} 
				}
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_putc(itemsbuf[i].sprite_initD_label[q][w],f))
					{
						new_return(87);
					} 
				}
				if(!p_iputl(itemsbuf[i].sprite_initiald[q],f))
				{
					new_return(88);
				} 
			}
			for ( int32_t q = 0; q < 2; q++ )
			{
				if(!p_putc(0,f))
				{
					new_return(89);
				} 
				
			}
			if(!p_iputw(itemsbuf[i].sprite_script,f))
			{
				new_return(90);
			}
			if(!p_putc(itemsbuf[i].pickupflag,f))
			{
				new_return(91);
			}
			std::string dispname(itemsbuf[i].display_name);
			if(!p_putcstr(dispname,f))
				new_return(92);
			for(int q = 0; q < WPNSPR_MAX; ++q)
			{
				if(!p_putc(itemsbuf[i].burnsprs[q], f))
					new_return(93);
				if(!p_putc(itemsbuf[i].light_rads[q], f))
					new_return(94);
			}
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeitems()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writeweapons(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_WEAPONS;
    dword section_version=V_WEAPONS;
    dword section_cversion=CV_WEAPONS;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(MAXWPNS,f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<MAXWPNS; i++)
        {
            if(!pfwrite((char *)weapon_string[i], 64, f))
            {
                new_return(5);
            }
        }
        
        for(int32_t i=0; i<MAXWPNS; i++)
        {            
            if(!p_putc(wpnsbuf[i].misc,f))
            {
                new_return(7);
            }
            
            if(!p_putc(wpnsbuf[i].csets,f))
            {
                new_return(8);
            }
            
            if(!p_putc(wpnsbuf[i].frames,f))
            {
                new_return(9);
            }
            
            if(!p_putc(wpnsbuf[i].speed,f))
            {
                new_return(10);
            }
            
            if(!p_putc(wpnsbuf[i].type,f))
            {
                new_return(11);
            }
	    
	        if(!p_iputw(wpnsbuf[i].script,f))
            {
                new_return(12);
            }
	    
	        if(!p_iputl(wpnsbuf[i].tile,f))
            {
                new_return(12);
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeweapons()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writemapscreen(PACKFILE *f, int32_t i, int32_t j)
{
	if((i*MAPSCRS+j)>=int32_t(TheMaps.size()))
		return qe_invalid;
	
	mapscr& screen=TheMaps.at(i*MAPSCRS+j);
	bool is_0x80_screen = j >= 0x80;
	
	if(!p_putc(screen.valid,f))
		return qe_invalid;
	if(!(screen.valid & mVALID))
		return qe_OK;
	//Calculate what needs writing
	uint32_t scr_has_flags = 0;
	if(screen.guytile || screen.guy || screen.roomflags || screen.str
		|| screen.room || screen.catchall)
		scr_has_flags |= SCRHAS_ROOMDATA;
	if(screen.hasitem || (is_0x80_screen && (screen.itemx||screen.itemy)))
		scr_has_flags |= SCRHAS_ITEM;
	if((screen.warpreturnc&0x00FF) || screen.tilewarpoverlayflags)
		scr_has_flags |= SCRHAS_TWARP;
	else for(auto q = 0; q < 4; ++q)
	{
		if(screen.tilewarptype[q]
			|| screen.tilewarpdmap[q]
			|| screen.tilewarpscr[q])
		{
			scr_has_flags |= SCRHAS_TWARP;
			break;
		}
	}
	if((screen.warpreturnc&0xFF00) || screen.sidewarpindex
		|| screen.sidewarpoverlayflags)
		scr_has_flags |= SCRHAS_SWARP;
	else for(auto q = 0; q < 4; ++q)
	{
		if(screen.sidewarptype[q] != wtSCROLL
			|| screen.sidewarpdmap[q]
			|| screen.sidewarpscr[q])
		{
			scr_has_flags |= SCRHAS_SWARP;
			break;
		}
	}
	if(screen.warparrivalx || screen.warparrivaly)
		scr_has_flags |= SCRHAS_WARPRET;
	else for(auto q = 0; q < 4; ++q)
	{
		if(screen.warpreturnx[q] || screen.warpreturny[q])
		{
			scr_has_flags |= SCRHAS_WARPRET;
			break;
		}
	}
	
	if(screen.hidelayers || screen.hidescriptlayers)
		scr_has_flags |= SCRHAS_LAYERS;
	else for(auto q = 0; q < 6; ++q)
	{
		if(screen.layermap[q] || screen.layerscreen[q]
			|| screen.layeropacity[q]!=255)
		{
			scr_has_flags |= SCRHAS_LAYERS;
			break;
		}
	}
	
	if(screen.exitdir)
		scr_has_flags |= SCRHAS_MAZE;
	else for(auto q = 0; q < 4; ++q)
	{
		if(screen.path[q])
		{
			scr_has_flags |= SCRHAS_MAZE;
			break;
		}
	}
	
	if(screen.door_combo_set || screen.stairx
		|| screen.stairy || screen.undercombo
		|| screen.undercset)
		scr_has_flags |= SCRHAS_D_S_U;
	else for(auto q = 0; q < 4; ++q)
	{
		if(screen.door[q] != dNONE)
		{
			scr_has_flags |= SCRHAS_D_S_U;
			break;
		}
	}
	
	if(screen.flags || screen.flags2
		|| screen.flags3 || screen.flags4
		|| screen.flags5 || screen.flags6
		|| screen.flags7 || screen.flags8
		|| screen.flags9 || screen.flags10
		|| screen.enemyflags)
		scr_has_flags |= SCRHAS_FLAGS;
	
	if(screen.pattern)
		scr_has_flags |= SCRHAS_ENEMY;
	else for(auto q = 0; q < 10; ++q)
	{
		if(screen.enemy[q])
		{
			scr_has_flags |= SCRHAS_ENEMY;
			break;
		}
	}
	
	if(screen.noreset || screen.nocarry
		|| screen.nextmap || screen.nextscr)
		scr_has_flags |= SCRHAS_CARRY;
	
	if(screen.script || screen.preloadscript)
		scr_has_flags |= SCRHAS_SCRIPT;
	else for(auto q = 0; q < 8; ++q)
	{
		if(screen.screeninitd[q])
		{
			scr_has_flags |= SCRHAS_SCRIPT;
			break;
		}
	}
	
	for(auto q = 0; q < 128; ++q)
	{
		if(screen.secretcombo[q]
			|| screen.secretcset[q]
			|| screen.secretflag[q])
		{
			scr_has_flags |= SCRHAS_SECRETS;
			break;
		}
	}
	
	for(auto q = 0; q < 176; ++q)
	{
		if(screen.data[q] || screen.cset[q]
			|| screen.sflag[q])
		{
			scr_has_flags |= SCRHAS_COMBOFLAG;
			break;
		}
	}
	
	if(screen.color || screen.csensitive != 1
		|| screen.oceansfx || screen.bosssfx
		|| screen.secretsfx || screen.holdupsfx
		|| screen.timedwarptics || screen.screen_midi != -1
		|| screen.lens_layer || screen.lens_show || screen.lens_hide)
		scr_has_flags |= SCRHAS_MISC;
	
	if(!p_iputl(scr_has_flags,f))
		return qe_invalid;
	
	//Write stuff
	if(scr_has_flags & SCRHAS_ROOMDATA)
	{
		if(!p_putc(screen.guy,f))
			return qe_invalid;
		if(!p_iputl(screen.guytile,f))
			return qe_invalid;
		if(!p_putc(screen.guycs,f))
			return qe_invalid;
		if(!p_iputw(screen.roomflags,f))
			return qe_invalid;
		if(!p_iputw(screen.str,f))
			return qe_invalid;
		if(!p_putc(screen.room,f))
			return qe_invalid;
		if(!p_iputw(screen.catchall,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_ITEM)
	{
		if(!p_putc(screen.item,f))
			return qe_invalid;
		if(!p_putc(screen.hasitem,f))
			return qe_invalid;
		if(!p_putc(screen.itemx,f))
			return qe_invalid;
		if(!p_putc(screen.itemy,f))
			return qe_invalid;
	}
	if(scr_has_flags & (SCRHAS_SWARP|SCRHAS_TWARP))
	{
		if(!p_iputw(screen.warpreturnc,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_TWARP)
	{
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.tilewarptype[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<4; k++)
		{
			if(!p_iputw(screen.tilewarpdmap[k],f))
				return qe_invalid;
		}
		
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.tilewarpscr[k],f))
				return qe_invalid;
		}
		
		if(!p_putc(screen.tilewarpoverlayflags,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_SWARP)
	{
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.sidewarptype[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<4; k++)
		{
			if(!p_iputw(screen.sidewarpdmap[k],f))
				return qe_invalid;
		}
		
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.sidewarpscr[k],f))
				return qe_invalid;
		}
		
		if(!p_putc(screen.sidewarpoverlayflags,f))
			return qe_invalid;
		if(!p_putc(screen.sidewarpindex,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_WARPRET)
	{
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.warpreturnx[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.warpreturny[k],f))
				return qe_invalid;
		}
		if(!p_putc(screen.warparrivalx,f))
			return qe_invalid;
		if(!p_putc(screen.warparrivaly,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_LAYERS)
	{
		for(int32_t k=0; k<6; k++)
		{
			if(!p_putc(screen.layermap[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<6; k++)
		{
			if(!p_putc(screen.layerscreen[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<6; k++)
		{
			if(!p_putc(screen.layeropacity[k],f))
				return qe_invalid;
		}
		if(!p_putc(screen.hidelayers,f))
			return qe_invalid;
		if(!p_putc(screen.hidescriptlayers,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_MAZE)
	{
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.path[k],f))
				return qe_invalid;
		}
		if(!p_putc(screen.exitdir,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_D_S_U)
	{
		if(!p_iputw(screen.door_combo_set,f))
			return qe_invalid;
		for(int32_t k=0; k<4; k++)
		{
			if(!p_putc(screen.door[k],f))
				return qe_invalid;
		}
		if(!p_putc(screen.stairx,f))
			return qe_invalid;
		if(!p_putc(screen.stairy,f))
			return qe_invalid;
		if(!p_iputw(screen.undercombo,f))
			return qe_invalid;
		if(!p_putc(screen.undercset,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_FLAGS)
	{
		if(!p_putc(screen.flags,f))
			return qe_invalid;
		if(!p_putc(screen.flags2,f))
			return qe_invalid;
		if(!p_putc(screen.flags3,f))
			return qe_invalid;
		if(!p_putc(screen.flags4,f))
			return qe_invalid;
		if(!p_putc(screen.flags5,f))
			return qe_invalid;
		if(!p_putc(screen.flags6,f))
			return qe_invalid;
		if(!p_putc(screen.flags7,f))
			return qe_invalid;
		if(!p_putc(screen.flags8,f))
			return qe_invalid;
		if(!p_putc(screen.flags9,f))
			return qe_invalid;
		if(!p_putc(screen.flags10,f))
			return qe_invalid;
		if(!p_putc(screen.enemyflags,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_ENEMY)
	{
		for(int32_t k=0; k<10; k++)
		{
			if(!p_iputw(screen.enemy[k],f))
				return qe_invalid;
		}
		if(!p_putc(screen.pattern,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_CARRY)
	{
		if(!p_iputw(screen.noreset,f))
			return qe_invalid;
		if(!p_iputw(screen.nocarry,f))
			return qe_invalid;
		if(!p_putc(screen.nextmap,f))
			return qe_invalid;
		if(!p_putc(screen.nextscr,f))
			return qe_invalid;
	}
	if(scr_has_flags & SCRHAS_SCRIPT)
	{
		if(!p_iputw(screen.script,f))
			return qe_invalid;
		if(!p_putc(screen.preloadscript,f))
			return qe_invalid;
		for ( int32_t q = 0; q < 8; q++ )
		{
			if(!p_iputl(screen.screeninitd[q],f))
				return qe_invalid;
		}
	}
	if(scr_has_flags & SCRHAS_SECRETS)
	{
		for(int32_t k=0; k<128; k++)
		{
			if(!p_iputw(screen.secretcombo[k],f))
				return qe_invalid;
		}
		
		for(int32_t k=0; k<128; k++)
		{
			if(!p_putc(screen.secretcset[k],f))
				return qe_invalid;
		}
		
		for(int32_t k=0; k<128; k++)
		{
			if(!p_putc(screen.secretflag[k],f))
				return qe_invalid;
		}
	}
	if(scr_has_flags & SCRHAS_COMBOFLAG)
	{
		for(int32_t k=0; k<176; ++k)
		{
			if(!p_iputw(screen.data[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<176; ++k)
		{
			if(!p_putc(screen.sflag[k],f))
				return qe_invalid;
		}
		for(int32_t k=0; k<176; ++k)
		{
			if(!p_putc(screen.cset[k],f))
				return qe_invalid;
		}
	}
	if(scr_has_flags & SCRHAS_MISC)
	{
		if(!p_iputw(screen.color,f))
			return qe_invalid;
		if(!p_putc(screen.csensitive,f))
			return qe_invalid;
		if(!p_putc(screen.oceansfx,f))
			return qe_invalid;
		if(!p_putc(screen.bosssfx,f))
			return qe_invalid;
		if(!p_putc(screen.secretsfx,f))
			return qe_invalid;
		if(!p_putc(screen.holdupsfx,f))
			return qe_invalid;
		if(!p_iputw(screen.timedwarptics,f))
			return qe_invalid;
		if(!p_iputw(screen.screen_midi,f))
			return qe_invalid;
		if(!p_putc(screen.lens_layer,f))
			return qe_invalid;
		if(!p_putc(screen.lens_show,f))
			return qe_invalid;
		if(!p_putc(screen.lens_hide,f))
			return qe_invalid;
	}
	
	dword numffc = screen.numFFC();
	if(!p_iputw(numffc,f))
		return qe_invalid;
	for(int32_t k=0; k<numffc; ++k)
	{
		ffcdata const& tempffc = screen.ffcs[k];
		
		if(!p_iputw(tempffc.data,f))
			return qe_invalid;
		
		if(!tempffc.data) //don't save the rest of the ffc
			continue;
		
		if(!p_putc(tempffc.cset,f))
			return qe_invalid;
		
		if(!p_iputw(tempffc.delay,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.x,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.y,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.vx,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.vy,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.ax,f))
			return qe_invalid;
		
		if(!p_iputzf(tempffc.ay,f))
			return qe_invalid;
		
		if(!p_putc(tempffc.link,f))
			return qe_invalid;
		
		if(!p_iputl(tempffc.hit_width,f))
			return qe_invalid;
		
		if(!p_iputl(tempffc.hit_height,f))
			return qe_invalid;
		
		if(!p_putc(tempffc.txsz,f))
			return qe_invalid;
		
		if(!p_putc(tempffc.tysz,f))
			return qe_invalid;
		
		if(!p_iputl(tempffc.flags,f))
			return qe_invalid;
		
		if(!p_iputw(tempffc.script,f))
			return qe_invalid;
		
		for(auto q = 0; q < 8; ++q)
		{
			if(!p_iputl(tempffc.initd[q],f))
				return qe_invalid;
		}

		if(!p_putc(0,f))
			return qe_invalid;
		if(!p_putc(0,f))
			return qe_invalid;
	}
	
	if(!p_putlstr(screen.usr_notes, f))
		return qe_invalid;
	
	return qe_OK;
}

int32_t writemaps(PACKFILE *f, zquestheader *)
{
	dword section_id=ID_MAPS;
	dword section_version=V_MAPS;
	dword section_cversion=CV_MAPS;
	dword section_size = 0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
		{
			new_return(4);
		}
		
		writesize=0;
		
		if(!p_iputw(map_count,f))
		{
			new_return(5);
		}
		map_autolayers.resize(map_count*6);
		for(int32_t i=0; i<map_count && i<MAXMAPS; i++)
		{
			byte valid = 0;
			for(int32_t j=0; j<MAPSCRS; j++)
			{
				if((i*MAPSCRS+j)>=int32_t(TheMaps.size()))
					break;
				mapscr& screen=TheMaps.at(i*MAPSCRS+j);
				if(screen.valid & mVALID)
				{
					valid = 1;
					break;
				}
			}
			if(!p_putc(valid,f))
			{
				new_return(6);
			}
			if(!valid) continue;
			
			{ //per-map info
				for(int q = 0; q < 6; ++q)
				{
					size_t ind = i*6+q;
					if(!p_iputw(map_autolayers[ind],f))
						new_return(7);
				}
			}
			
			for(int32_t j=0; j<MAPSCRS; j++)
				writemapscreen(f,i,j);
		}
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writemaps()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

int32_t writecombo_loop(PACKFILE *f, word section_version, newcombo const& tmp_cmb)
{
	//Check what needs writing
	byte combo_has_flags = 0;
	for(auto q = 0; q < 8; ++q)
	{
		if(tmp_cmb.attribytes[q] || tmp_cmb.attrishorts[q]
			|| (q < 4 && tmp_cmb.attributes[q]))
		{
			combo_has_flags |= CHAS_ATTRIB;
			break;
		}
	}
	if (tmp_cmb.triggerflags[0] || tmp_cmb.triggerflags[1]
		|| tmp_cmb.triggerflags[2] || tmp_cmb.triggerflags[3]
		|| tmp_cmb.triggerflags[4] || tmp_cmb.triggerflags[5]
		|| tmp_cmb.triggerlevel || tmp_cmb.trig_lstate
		|| tmp_cmb.trig_gstate || tmp_cmb.trig_statetime
		|| tmp_cmb.triggerbtn || tmp_cmb.triggeritem
		|| tmp_cmb.trigtimer || tmp_cmb.trigsfx
		|| tmp_cmb.trigchange || tmp_cmb.trigprox
		|| tmp_cmb.trigctr || tmp_cmb.trigctramnt
		|| tmp_cmb.triglbeam || tmp_cmb.trigcschange
		|| tmp_cmb.spawnitem || tmp_cmb.spawnenemy
		|| tmp_cmb.exstate > -1 || tmp_cmb.spawnip
		|| tmp_cmb.exdoor_dir > -1
		|| tmp_cmb.trigcopycat || tmp_cmb.trigcooldown
		|| tmp_cmb.trig_genscr || tmp_cmb.trig_group
		|| tmp_cmb.trig_group_val || tmp_cmb.trig_levelitems
		|| tmp_cmb.trigdmlevel > -1
		|| tmp_cmb.triglvlpalette > -1 || tmp_cmb.trigbosspalette > -1
		|| tmp_cmb.trigquaketime > -1 || tmp_cmb.trigwavytime > -1
		|| tmp_cmb.trig_swjinxtime > -2 || tmp_cmb.trig_itmjinxtime > -2
		|| tmp_cmb.trig_stuntime > -2 || tmp_cmb.trig_bunnytime > -2
		|| tmp_cmb.trig_pushtime != 8 || tmp_cmb.trig_shieldjinxtime > -2
		|| tmp_cmb.prompt_cid || tmp_cmb.prompt_cs
		|| tmp_cmb.prompt_x != 12 || tmp_cmb.prompt_y != -8)
		combo_has_flags |= CHAS_TRIG;
	else for(int q = 0; q < 3; ++q)
		if(tmp_cmb.trigtint[q])
			combo_has_flags |= CHAS_TRIG;
	if(tmp_cmb.usrflags || tmp_cmb.genflags)
		combo_has_flags |= CHAS_FLAG;
	if(tmp_cmb.frames || tmp_cmb.speed || tmp_cmb.nextcombo
		|| tmp_cmb.nextcset || tmp_cmb.skipanim || tmp_cmb.skipanimy
		|| tmp_cmb.animflags)
		combo_has_flags |= CHAS_ANIM;
	if(tmp_cmb.script || tmp_cmb.label.size())
		combo_has_flags |= CHAS_SCRIPT;
	else for(auto q = 0; q < 8; ++q)
	{
		if(tmp_cmb.initd[q])
		{
			combo_has_flags |= CHAS_SCRIPT;
			break;
		}
	}
	if(tmp_cmb.o_tile || tmp_cmb.flip || tmp_cmb.walk != 0xF0
		|| tmp_cmb.type || tmp_cmb.csets)
		combo_has_flags |= CHAS_BASIC;
	if(tmp_cmb.liftcmb || tmp_cmb.liftcs || tmp_cmb.liftdmg
		|| tmp_cmb.liftlvl || tmp_cmb.liftitm || tmp_cmb.liftflags
		|| tmp_cmb.liftgfx || tmp_cmb.liftsprite || tmp_cmb.liftsfx
		|| tmp_cmb.liftundercmb || tmp_cmb.liftundercs
		|| tmp_cmb.liftbreaksprite!=-1 || tmp_cmb.liftbreaksfx
		|| tmp_cmb.lifthei!=8 || tmp_cmb.lifttime!=16
		|| tmp_cmb.lift_parent_item)
		combo_has_flags |= CHAS_LIFT;
	if(tmp_cmb.speed_mult != 1 || tmp_cmb.speed_div != 1 || tmp_cmb.speed_add
		|| tmp_cmb.sfx_appear || tmp_cmb.sfx_disappear || tmp_cmb.sfx_loop || tmp_cmb.sfx_walking || tmp_cmb.sfx_standing
		|| tmp_cmb.spr_appear || tmp_cmb.spr_disappear || tmp_cmb.spr_walking || tmp_cmb.spr_standing || tmp_cmb.sfx_tap
		|| tmp_cmb.sfx_landing || tmp_cmb.spr_falling || tmp_cmb.spr_drowning || tmp_cmb.spr_lava_drowning || tmp_cmb.sfx_falling
		|| tmp_cmb.sfx_drowning || tmp_cmb.sfx_lava_drowning)
		combo_has_flags |= CHAS_GENERAL;
	
	if(!p_putc(combo_has_flags,f))
	{
		return 50;
	}
	if(!combo_has_flags) return 0; //Valid, done reading
	//Write the combo
	if(combo_has_flags&CHAS_BASIC)
	{
		if(!p_iputl(tmp_cmb.o_tile,f))
			return 6;
		
		if(!p_putc(tmp_cmb.flip,f))
			return 7;
		
		if(!p_putc(tmp_cmb.walk,f))
			return 8;
		
		if(!p_putc(tmp_cmb.type,f))
			return 9;
		
		if(!p_putc(tmp_cmb.flag,f))
			return 15;
		
		if(!p_putc(tmp_cmb.csets,f))
			return 10;
	}
	if(combo_has_flags&CHAS_SCRIPT)
	{
		p_putcstr(tmp_cmb.label, f);

		if(!p_iputw(tmp_cmb.script,f))
			return 26;
		for ( int32_t q = 0; q < 8; q++ )
			if(!p_iputl(tmp_cmb.initd[q],f))
				return 27;
	}
	if(combo_has_flags&CHAS_ANIM)
	{
		if(!p_putc(tmp_cmb.frames,f))
			return 11;
		
		if(!p_putc(tmp_cmb.speed,f))
			return 12;
		
		if(!p_iputw(tmp_cmb.nextcombo,f))
			return 13;
		
		if(!p_putc(tmp_cmb.nextcset,f))
			return 14;
		
		if(!p_putc(tmp_cmb.skipanim,f))
			return 16;
					
		if(!p_putc(tmp_cmb.skipanimy,f))
			return 18;
		
		if(!p_putc(tmp_cmb.animflags,f))
			return 19;
	}
	if(combo_has_flags&CHAS_ATTRIB)
	{
		for ( int32_t q = 0; q < 4; q++ )
			if(!p_iputl(tmp_cmb.attributes[q],f))
				return 20;
		for ( int32_t q = 0; q < 8; q++ )
			if(!p_putc(tmp_cmb.attribytes[q],f))
				return 25;
		for ( int32_t q = 0; q < 8; q++ ) //I also added attrishorts -Dimi
			if(!p_iputw(tmp_cmb.attrishorts[q],f))
				return 32;
	}
	if(combo_has_flags&CHAS_FLAG)
	{
		if(!p_iputl(tmp_cmb.usrflags,f))
			return 21;
		if(!p_iputw(tmp_cmb.genflags,f))
			return 33;
	}
	if(combo_has_flags&CHAS_TRIG)
	{
		for ( int32_t q = 0; q < 6; q++ ) 
			if(!p_iputl(tmp_cmb.triggerflags[q],f))
				return 22;
		if(!p_iputl(tmp_cmb.triggerlevel,f))
			return 23;
		if(!p_putc(tmp_cmb.triggerbtn,f))
			return 34;
		if(!p_putc(tmp_cmb.triggeritem,f))
			return 35;
		if(!p_putc(tmp_cmb.trigtimer,f))
			return 36;
		if(!p_putc(tmp_cmb.trigsfx,f))
			return 37;
		if(!p_iputl(tmp_cmb.trigchange,f))
			return 38;
		if(!p_iputw(tmp_cmb.trigprox,f))
			return 39;
		if(!p_putc(tmp_cmb.trigctr,f))
			return 40;
		if(!p_iputl(tmp_cmb.trigctramnt,f))
			return 41;
		if(!p_putc(tmp_cmb.triglbeam,f))
			return 42;
		if(!p_putc(tmp_cmb.trigcschange,f))
			return 43;
		if(!p_iputw(tmp_cmb.spawnitem,f))
			return 44;
		if(!p_iputw(tmp_cmb.spawnenemy,f))
			return 45;
		if(!p_putc(tmp_cmb.exstate,f))
			return 46;
		if(!p_iputl(tmp_cmb.spawnip,f))
			return 47;
		if(!p_putc(tmp_cmb.trigcopycat,f))
			return 48;
		if(!p_putc(tmp_cmb.trigcooldown,f))
			return 49;
		if(!p_iputw(tmp_cmb.prompt_cid,f))
			return 50;
		if(!p_putc(tmp_cmb.prompt_cs,f))
			return 51;
		if(!p_iputw(tmp_cmb.prompt_x,f))
			return 52;
		if(!p_iputw(tmp_cmb.prompt_y,f))
			return 53;
		if(!p_putc(tmp_cmb.trig_lstate,f))
			return 69;
		if(!p_putc(tmp_cmb.trig_gstate,f))
			return 70;
		if(!p_iputl(tmp_cmb.trig_statetime,f))
			return 71;
		if(!p_iputw(tmp_cmb.trig_genscr,f))
			return 72;
		if(!p_putc(tmp_cmb.trig_group,f))
			return 76;
		if(!p_iputw(tmp_cmb.trig_group_val,f))
			return 77;
		if(!p_putc(tmp_cmb.exdoor_dir,f))
			return 89;
		if(!p_putc(tmp_cmb.exdoor_ind,f))
			return 90;
		if(!p_putc(tmp_cmb.trig_levelitems,f))
			return 91;
		if(!p_iputw(tmp_cmb.trigdmlevel,f))
			return 92;
		for(int q = 0; q < 3; ++q)
			if(!p_iputw(tmp_cmb.trigtint[q],f))
				return 93;
		if(!p_iputw(tmp_cmb.triglvlpalette,f))
			return 94;
		if(!p_iputw(tmp_cmb.trigbosspalette,f))
			return 95;
		if(!p_iputw(tmp_cmb.trigquaketime,f))
			return 96;
		if(!p_iputw(tmp_cmb.trigwavytime,f))
			return 97;
		if(!p_iputw(tmp_cmb.trig_swjinxtime,f))
			return 98;
		if(!p_iputw(tmp_cmb.trig_itmjinxtime,f))
			return 99;
		if(!p_iputw(tmp_cmb.trig_stuntime,f))
			return 100;
		if(!p_iputw(tmp_cmb.trig_bunnytime,f))
			return 101;
		if(!p_putc(tmp_cmb.trig_pushtime,f))
			return 102;
		if (!p_iputw(tmp_cmb.trig_shieldjinxtime, f))
			return 103;
	}
	if(combo_has_flags&CHAS_LIFT)
	{
		if(!p_iputw(tmp_cmb.liftcmb,f))
			return 54;
		if(!p_putc(tmp_cmb.liftcs,f))
			return 55;
		if(!p_iputw(tmp_cmb.liftundercmb,f))
			return 56;
		if(!p_putc(tmp_cmb.liftundercs,f))
			return 57;
		if(!p_putc(tmp_cmb.liftdmg,f))
			return 58;
		if(!p_putc(tmp_cmb.liftlvl,f))
			return 59;
		if(!p_putc(tmp_cmb.liftitm,f))
			return 60;
		if(!p_putc(tmp_cmb.liftflags,f))
			return 61;
		if(!p_putc(tmp_cmb.liftgfx,f))
			return 62;
		if(!p_putc(tmp_cmb.liftsprite,f))
			return 63;
		if(!p_putc(tmp_cmb.liftsfx,f))
			return 64;
		if(!p_iputw(tmp_cmb.liftbreaksprite,f))
			return 65;
		if(!p_putc(tmp_cmb.liftbreaksfx,f))
			return 66;
		if(!p_putc(tmp_cmb.lifthei,f))
			return 67;
		if(!p_putc(tmp_cmb.lifttime,f))
			return 68;
		if(!p_putc(tmp_cmb.lift_parent_item,f))
			return 78;
	}
	if(combo_has_flags&CHAS_GENERAL)
	{
		if(!p_putc(tmp_cmb.speed_mult,f))
			return 73;
		if(!p_putc(tmp_cmb.speed_div,f))
			return 74;
		if(!p_iputzf(tmp_cmb.speed_add,f))
			return 75;
		if(!p_putc(tmp_cmb.sfx_appear,f))
			return 79;
		if(!p_putc(tmp_cmb.sfx_disappear,f))
			return 80;
		if(!p_putc(tmp_cmb.sfx_loop,f))
			return 81;
		if(!p_putc(tmp_cmb.sfx_walking,f))
			return 82;
		if(!p_putc(tmp_cmb.sfx_standing,f))
			return 83;
		if(!p_putc(tmp_cmb.spr_appear,f))
			return 84;
		if(!p_putc(tmp_cmb.spr_disappear,f))
			return 85;
		if(!p_putc(tmp_cmb.spr_walking,f))
			return 86;
		if(!p_putc(tmp_cmb.spr_standing,f))
			return 87;
		if(!p_putc(tmp_cmb.sfx_tap,f))
			return 88;
		if(!p_putc(tmp_cmb.sfx_landing,f))
			return 89;
		if(!p_putc(tmp_cmb.spr_falling,f))
			return 90;
		if(!p_putc(tmp_cmb.spr_drowning,f))
			return 91;
		if(!p_putc(tmp_cmb.spr_lava_drowning,f))
			return 92;
		if(!p_putc(tmp_cmb.sfx_falling,f))
			return 93;
		if(!p_putc(tmp_cmb.sfx_drowning,f))
			return 94;
		if(!p_putc(tmp_cmb.sfx_lava_drowning,f))
			return 95;
	}
	return 0;
}

int32_t writecombos(PACKFILE *f, word version, word build, word start_combo, word max_combos)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    
    word combos_used;
    dword section_id=ID_COMBOS;
    dword section_version=V_COMBOS;
    dword section_cversion=CV_COMBOS;
    //  dword section_size=0;
    combos_used = count_combos()-start_combo;
    combos_used = zc_min(combos_used, max_combos);
    combos_used = zc_min(combos_used, MAXCOMBOS);
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        combos_used=count_combos()-start_combo;
        combos_used=zc_min(combos_used, max_combos);
        combos_used=zc_min(combos_used, MAXCOMBOS);
        
        if(!p_iputw(combos_used,f))
        {
            new_return(5);
        }
        
		size_t end_combo = start_combo+combos_used;
		for(size_t q = start_combo; q < end_combo; ++q)
		{
			auto ret = writecombo_loop(f, section_version, combobuf[q]);
			if(ret) new_return(ret);
		}
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writecombos()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writecomboaliases(PACKFILE *f, word version, word build)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    
    dword section_id=ID_COMBOALIASES;
    dword section_version=V_COMBOALIASES;
    dword section_cversion=CV_COMBOALIASES;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        for(int32_t j=0; j<MAXCOMBOALIASES; j++)
        {
            if(!p_iputw(combo_aliases[j].combo,f))
            {
                new_return(5);
            }
            
            if(!p_putc(combo_aliases[j].cset,f))
            {
                new_return(6);
            }
            
            int32_t count = ((combo_aliases[j].width+1)*(combo_aliases[j].height+1))*(comboa_lmasktotal(combo_aliases[j].layermask)+1);
            
            if(!p_putc(combo_aliases[j].width,f))
            {
                new_return(7);
            }
            
            if(!p_putc(combo_aliases[j].height,f))
            {
                new_return(8);
            }
            
            if(!p_putc(combo_aliases[j].layermask,f))
            {
                new_return(9);
            }
            
            for(int32_t k=0; k<count; k++)
            {
                if(!p_iputw(combo_aliases[j].combos[k],f))
                {
                    new_return(10);
                }
            }
            
            for(int32_t k=0; k<count; k++)
            {
                if(!p_putc(combo_aliases[j].csets[k],f))
                {
                    new_return(11);
                }
            }
        }
		
		//Combo pools!
		int16_t num_cpools;
		for(num_cpools = MAXCOMBOPOOLS-1; num_cpools >= 0; --num_cpools)
		{
			if(combo_pools[num_cpools].valid()) //found a used pool
			{
				++num_cpools;
				break;
			}
		}
		if(num_cpools < 0) num_cpools = 0;
		
        if(!p_iputw(num_cpools,f))
        {
            new_return(12);
        }
		
		for(auto cp = 0; cp < num_cpools; ++cp)
		{
			combo_pool const& pool = combo_pools[cp];
			int32_t num_combos = pool.combos.size();
			if(!p_iputl(num_combos,f))
			{
				new_return(13);
			}
			
			for(auto q = 0; q < num_combos; ++q)
			{
				cpool_entry const& entry = pool.combos.at(q);
				if(!p_iputl(entry.cid,f))
				{
					new_return(14);
				}
				if(!p_putc(entry.cset,f))
				{
					new_return(15);
				}
				if(!p_iputw(entry.quant,f))
				{
					new_return(16);
				}
			}
		}
		
		//Autocombos!
		int16_t num_cautos;
		for (num_cautos = MAXAUTOCOMBOS - 1; num_cautos >= 0; --num_cautos)
		{
			if (combo_autos[num_cautos].valid()) //found a used autocombo
			{
				++num_cautos;
				break;
			}
		}
		if (num_cautos < 0) num_cautos = 0;

		if (!p_iputw(num_cautos, f))
		{
			new_return(17);
		}

		for (auto ca = 0; ca < num_cautos; ++ca)
		{
			combo_auto const& cauto = combo_autos[ca];
			if (!p_putc(cauto.getType(), f))
			{
				new_return(18);
			}
			if (!p_iputl(cauto.getIconDisplay(), f))
			{
				new_return(19);
			}
			if (!p_iputl(cauto.getEraseCombo(), f))
			{
				new_return(20);
			}
			if (!p_putc(cauto.getFlags(), f))
			{
				new_return(21);
			}
			if (!p_putc(cauto.getArg(), f))
			{
				new_return(22);
			}
			int32_t num_combos = cauto.combos.size();
			if (!p_iputl(num_combos, f))
			{
				new_return(23);
			}

			for (auto q = 0; q < num_combos; ++q)
			{
				autocombo_entry const& entry = cauto.combos.at(q);
				if (!p_putc(entry.ctype, f))
				{
					new_return(24);
				}
				if (!p_iputl(entry.cid, f))
				{
					new_return(25);
				}
			}
		}

        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writecomboaliases()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writecolordata(PACKFILE *f, word version, word build, word start_cset, word max_csets)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    start_cset=start_cset;
    max_csets=max_csets;
    
    dword section_id=ID_CSETS;
    dword section_version=V_CSETS;
    dword section_cversion=CV_CSETS;
    int32_t palcycles = count_palcycles(&QMisc);
// int32_t palcyccount = count_palcycles(&QMisc);
    dword section_size = 0;
    
    //section id
    
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!pfwrite(colordata,psTOTAL255,f))
        {
            new_return(5);
        }
        
        if(!pfwrite(palnames,MAXLEVELS*PALNAMESIZE,f))
        {
            new_return(6);
        }
        
        if(!p_iputw(palcycles,f))
        {
            new_return(15);
        }
        
        for(int32_t i=0; i<palcycles; i++)
        {
            for(int32_t j=0; j<3; j++)
            {
                if(!p_putc(QMisc.cycles[i][j].first,f))
                {
                    new_return(16);
                }
            }
            
            for(int32_t j=0; j<3; j++)
            {
                if(!p_putc(QMisc.cycles[i][j].count,f))
                {
                    new_return(17);
                }
            }
            
            for(int32_t j=0; j<3; j++)
            {
                if(!p_putc(QMisc.cycles[i][j].speed,f))
                {
                    new_return(18);
                }
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writecolordata()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writestrings(PACKFILE *f, word version, word build, word start_msgstr, word max_msgstrs)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    start_msgstr=start_msgstr;
    max_msgstrs=max_msgstrs;
    
    dword section_id=ID_STRINGS;
    dword section_version=V_STRINGS;
    dword section_cversion=CV_STRINGS;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(msg_count,f))
        {
            return qe_invalid;
        }
        
        for(int32_t i=0; i<msg_count; i++)
        {
			int32_t sz = MsgStrings[i].s.size();
			if(sz > 8192) sz = 8192;
			if(!p_iputl(sz, f))
			{
				return qe_invalid;
			}
			
            char const* tmpstr = MsgStrings[i].s.c_str();
            if (sz > 0)
            {
                if (!pfwrite((void*)tmpstr,sz, f))
                {
                    return qe_invalid;
                }
            }
            
            if(!p_iputw(MsgStrings[i].nextstring,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputl(MsgStrings[i].tile,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].cset,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].trans?1:0,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].font,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputw(MsgStrings[i].x,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputw(MsgStrings[i].y,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputw(MsgStrings[i].w,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputw(MsgStrings[i].h,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].hspace,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].vspace,f))
            {
                return qe_invalid;
            }
            
            if(!p_putc(MsgStrings[i].stringflags,f))
            {
                return qe_invalid;
            }
			
			for(int32_t q = 0; q < 4; ++q)
			{
				if(!p_putc(MsgStrings[i].margins[q],f))
				{
					return qe_invalid;
				}
			}
			
			if(!p_iputl(MsgStrings[i].portrait_tile,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].portrait_cset,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].portrait_x,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].portrait_y,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].portrait_tw,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].portrait_th,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].shadow_type,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].shadow_color,f))
			{
				return qe_invalid;
			}
			
			if(!p_putc(MsgStrings[i].drawlayer,f))
			{
				return qe_invalid;
			}
            
            if(!p_putc(MsgStrings[i].sfx,f))
            {
                return qe_invalid;
            }
            
            if(!p_iputw(MsgStrings[i].listpos,f))
            {
                return qe_invalid;
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writestrings()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writestrings_text(PACKFILE *f)
{
    std::map<int32_t, int32_t> msglistcache;
    
    for(int32_t index = 1; index<msg_count; index++)
    {
        for(int32_t i=1; i<msg_count; i++)
        {
            if(MsgStrings[i].listpos==index)
            {
                msglistcache[index-1]=i;
                break;
            }
        }
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        char ebuf[32];
        
        sprintf(ebuf,"Total strings: %d\n", msg_count-1);
        
        if(!pfwrite(&ebuf,(int32_t)strlen(ebuf),f))
        {
            return qe_invalid;
        }
        
        for(int32_t i=1; i<msg_count; i++)
        {
            int32_t str = msglistcache[i-1];
            
            if(!str)
                continue;
                
            if(MsgStrings[str].nextstring != 0)
                sprintf(ebuf,"\n\n___%d(->%d)___\n", str,MsgStrings[str].nextstring);
            else
                sprintf(ebuf,"\n\n___%d___\n", str);
                
            if(!pfwrite(&ebuf,(int32_t)strlen(ebuf),f))
            {
                return qe_invalid;
            }
            
            encode_msg_str(str);
            
            if(!pfwrite(&msgbuf,(int32_t)strlen(msgbuf),f))
            {
                return qe_invalid;
            }
        }
    }
    
    new_return(0);
}

int32_t writestrings_tsv(PACKFILE *f)
{
	std::stringstream ss;

	int32_t str;
	std::vector<std::pair<std::string, std::function<std::string(const MsgStr&)>>> fields = {
		{ "message", [&](auto& msg){
			encode_msg_str(str);
			return msgbuf;
		}},
		{ "next", [](auto& msg){ return std::to_string(msg.nextstring); } },
		{ "tile", [](auto& msg){ return std::to_string(msg.tile); } },
		{ "cset", [](auto& msg){ return std::to_string(msg.cset); } },
		{ "trans", [](auto& msg){ return std::to_string(msg.trans); } },
		{ "font", [](auto& msg){ return std::to_string(msg.font); } },
		{ "x", [](auto& msg){ return std::to_string(msg.x); } },
		{ "y", [](auto& msg){ return std::to_string(msg.y); } },
		{ "w", [](auto& msg){ return std::to_string(msg.w); } },
		{ "h", [](auto& msg){ return std::to_string(msg.h); } },
		{ "sfx", [](auto& msg){ return std::to_string(msg.sfx); } },
		{ "pos", [](auto& msg){ return std::to_string(msg.listpos); } },
		{ "vspace", [](auto& msg){ return std::to_string(msg.vspace); } },
		{ "hspace", [](auto& msg){ return std::to_string(msg.hspace); } },
		{ "flags", [](auto& msg){ return std::to_string(msg.stringflags); } },
		{ "margin", [](auto& msg){ return fmt::format("{} {} {} {}", msg.margins[0], msg.margins[3], msg.margins[1], msg.margins[2]); } },
		{ "portrait_tile", [](auto& msg){ return std::to_string(msg.portrait_tile); } },
		{ "portrait_cset", [](auto& msg){ return std::to_string(msg.portrait_cset); } },
		{ "portrait_x", [](auto& msg){ return std::to_string(msg.portrait_x); } },
		{ "portrait_y", [](auto& msg){ return std::to_string(msg.portrait_y); } },
		{ "portrait_tw", [](auto& msg){ return std::to_string(msg.portrait_tw); } },
		{ "portrait_th", [](auto& msg){ return std::to_string(msg.portrait_th); } },
		{ "shadow_type", [](auto& msg){ return std::to_string(msg.shadow_type); } },
		{ "shadow_color", [](auto& msg){ return std::to_string(msg.shadow_color); } },
		{ "drawlayer", [](auto& msg){ return std::to_string(msg.drawlayer); } },
	};

	for (auto& [name, fn] : fields)
	{
		ss << name;
		if (name == fields.back().first)
			break;
		ss << '\t';
	}
	ss << '\n';

	// First entry is a fake string, to help frame the lines. Should be helpful if manually editing these in a text editor or spreadsheet.
	ss << "|IT'S DANGEROUS TO GO  || ALONE! TAKE THIS.    ||LOREM IPSUM LOREM IPSU|\n";

	for(int32_t i=1; i<msg_count; i++)
	{
		str = i;
		auto& msg = MsgStrings[str];
		for (auto& [name, fn] : fields)
		{
			std::string text = fn(msg);
			ss << text;
			if (name == fields.back().first)
				break;
			ss << '\t';
		}
		ss << '\n';
	}

	std::string text = ss.str();
	if (!pack_fwrite(text.c_str(), text.size(), f))
	{
		return qe_invalid;
	}
    
    new_return(0);
}

std::string parse_msg_str(std::string const& s);

void parse_strings_tsv(std::string tsv)
{
	std::map<std::string, std::function<void(MsgStr&, const std::string&)>> fields = {
		{ "message", [](auto& msg, auto& text){ msg.s = parse_msg_str(text); } },
		{ "next", [](auto& msg, auto& text){ msg.nextstring = std::stoi(text); } },
		{ "tile", [](auto& msg, auto& text){ msg.tile = std::stoi(text); } },
		{ "cset", [](auto& msg, auto& text){ msg.cset = std::stoi(text); } },
		{ "trans", [](auto& msg, auto& text){ msg.trans = std::stoi(text); } },
		{ "font", [](auto& msg, auto& text){ msg.font = std::stoi(text); } },
		{ "x", [](auto& msg, auto& text){ msg.x = std::stoi(text); } },
		{ "y", [](auto& msg, auto& text){ msg.y = std::stoi(text); } },
		{ "w", [](auto& msg, auto& text){ msg.w = std::stoi(text); } },
		{ "h", [](auto& msg, auto& text){ msg.h = std::stoi(text); } },
		{ "sfx", [](auto& msg, auto& text){ msg.sfx = std::stoi(text); } },
		{ "pos", [](auto& msg, auto& text){ msg.listpos = std::stoi(text); } },
		{ "vspace", [](auto& msg, auto& text){ msg.vspace = std::stoi(text); } },
		{ "hspace", [](auto& msg, auto& text){ msg.hspace = std::stoi(text); } },
		{ "flags", [](auto& msg, auto& text){ msg.stringflags = std::stoi(text); } },
		{ "margin", [&](auto& msg, auto& text){
			std::vector<std::string> strs;
			util::split(text, strs, ' ');
			if (strs.size() != 4)
				throw std::runtime_error("margin field must have 4 components");
			msg.margins[0] = std::stoi(strs[0]);
			msg.margins[1] = std::stoi(strs[1]);
			msg.margins[2] = std::stoi(strs[2]);
			msg.margins[3] = std::stoi(strs[3]);
		} },
		{ "portrait_tile", [](auto& msg, auto& text){ msg.portrait_tile = std::stoi(text); } },
		{ "portrait_cset", [](auto& msg, auto& text){ msg.portrait_cset = std::stoi(text); } },
		{ "portrait_x", [](auto& msg, auto& text){ msg.portrait_x = std::stoi(text); } },
		{ "portrait_y", [](auto& msg, auto& text){ msg.portrait_y = std::stoi(text); } },
		{ "portrait_tw", [](auto& msg, auto& text){ msg.portrait_tw = std::stoi(text); } },
		{ "portrait_th", [](auto& msg, auto& text){ msg.portrait_th = std::stoi(text); } },
		{ "shadow_type", [](auto& msg, auto& text){ msg.shadow_type = std::stoi(text); } },
		{ "shadow_color", [](auto& msg, auto& text){ msg.shadow_color = std::stoi(text); } },
		{ "drawlayer", [](auto& msg, auto& text){ msg.drawlayer = std::stoi(text); } },
	};

	std::vector<std::string> rows;
	util::split(tsv, rows, '\n');
	if (rows.size())
	{
		std::string last = rows.back();
		util::trimstr(last);
		if (last.empty())
			rows.pop_back();
	}
	if (rows.size() <= 1)
		throw std::runtime_error("missing header row");

	std::vector<std::string> columns;
	util::split(rows[0], columns, '\t');
	for (auto name : columns)
	{
		if (!fields.contains(name))
			throw std::runtime_error(fmt::format("invalid field: {}", name));
	}

	int start_index = 1;
	if (rows[start_index].find("||LOREM IPSUM") != std::string::npos)
		start_index += 1;

	int num_strings = rows.size() - start_index + 1;
	if (num_strings > MAXMSGS-1)
		throw std::runtime_error(fmt::format("too many strings, max is {}", MAXMSGS-1));

	std::vector<MsgStr> msgs;
	msgs.reserve(num_strings);
	for (int i = start_index; i < rows.size(); i++)
	{
		std::vector<std::string> strs;
		util::split(rows[i], strs, '\t');
		if (strs.size() != columns.size())
			throw std::runtime_error(fmt::format("row {} has {} fields, expected {}", i, strs.size(), columns.size()));

		int j = 0;
		auto& msg = msgs.emplace_back();
		for (auto& name : columns)
		{
			auto& fn = fields[name];
			try
			{
				fn(msg, strs[j++]);
			}
			catch (std::exception& ex)
			{
				throw std::runtime_error(fmt::format("error parsing row {} field {}: {}", i, name, ex.what()));
			}
		}
	}

	init_msgstrings(0, msgs.size());
	for (int i = 0; i < msgs.size(); i++)
		MsgStrings[i + 1] = msgs[i];
	msg_count = msgs.size();
	msglistcache.clear();
}

bool isblanktile(tiledata *buf, int32_t i);
int32_t writetiles(PACKFILE *f, word version, word build, int32_t start_tile, int32_t max_tiles)
{
    //these are here to bypass compiler warnings about unused arguments
    version=version;
    build=build;
    
    int32_t tiles_used;
    dword section_id=ID_TILES;
    dword section_version=V_TILES;
    dword section_cversion=CV_TILES;
	al_trace("Counting tiles used\n");
    tiles_used = count_tiles(newtilebuf)-start_tile;
    tiles_used = zc_min(tiles_used, max_tiles);
    tiles_used = zc_min(tiles_used, NEWMAXTILES);
	al_trace("writetiles counted %dtiles used.\n",tiles_used); 
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        tiles_used=count_tiles(newtilebuf)-start_tile;
        tiles_used=zc_min(tiles_used, max_tiles);
        tiles_used=zc_min(tiles_used, NEWMAXTILES);
        
        if(!p_iputl(tiles_used,f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<tiles_used; ++i)
        {
			if(isblanktile(newtilebuf, start_tile+i))
			{
				if(!p_putc(0,f))
					new_return(8);
			}
			else
			{
				int format = newtilebuf[start_tile+i].format;
				if(!p_putc(format,f))
				{
					new_return(6);
				}
				
				if (format == tf4Bit)
				{
					byte temp_tile[128];
					byte *di = temp_tile;
					byte *src = newtilebuf[start_tile+i].data;
					for (int32_t si=0; si<256; si+=2)
					{
						*di = (src[si]&15) + ((src[si+1]&15) << 4);
						++di;
					}
					if (!pfwrite(temp_tile,128,f))
					{
						new_return(7);
					}
				}
				else if (!pfwrite(newtilebuf[start_tile+i].data,tilesize(format),f))
				{
					new_return(7);
				}
			}
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writetiles()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

/* MIDI Format
section_id			LONG
section_version		WORD
section_cversion	WORD
section_size		LONG
midi_flags			32 Byte ? BITFIELD[252]

[
title		36
start 		 4
loop_start	 4
loop_end	 4
loop		 2
volume		 2
midi		 *
]

*/

int32_t writemidis(PACKFILE *f)
{
    dword section_id=ID_MIDIS;
    dword section_version=V_MIDIS;
    dword section_cversion=CV_MIDIS;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!pfwrite(midi_flags,sizeof(midi_flags),f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<MAXCUSTOMMIDIS; i++)
        {
            if(get_bit(midi_flags,i))
            {
                if(!pfwrite(&customtunes[i].title,sizeof(customtunes[0].title)-1,f))
                {
                    new_return(6);
                }
                
                if(!p_iputl(customtunes[i].start,f))
                {
                    new_return(7);
                }
                
                if(!p_iputl(customtunes[i].loop_start,f))
                {
                    new_return(8);
                }
                
                if(!p_iputl(customtunes[i].loop_end,f))
                {
                    new_return(9);
                }
                
                if(!p_iputw(customtunes[i].loop,f))
                {
                    new_return(10);
                }
                
                if(!p_iputw(customtunes[i].volume,f))
                {
                    new_return(11);
                }
                
                if(!pfwrite(&customtunes[i].flags, sizeof(customtunes[i].flags),f))
                {
                    new_return(12);
                }

				byte format = MFORMAT_MIDI;
                if(!pfwrite(&format, sizeof(format),f))
                {
                    new_return(13);
                }

				if (!write_midi(customtunes[i].data, f)) new_return(14);
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writemidis()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writecheats(PACKFILE *f, zquestheader *Header)
{
    dword section_id=ID_CHEATS;
    dword section_version=V_CHEATS;
    dword section_cversion=CV_CHEATS;
    dword section_size = 0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_putc(Header->data_flags[ZQ_CHEATS2],f))
        {
            new_return(5);
        }
        
        if(Header->data_flags[ZQ_CHEATS2])
        {
            if(!p_iputl(zcheats.flags,f))
            {
                new_return(6);
            }
            
            if(!pfwrite(&zcheats.codes, sizeof(zcheats.codes), f))
            {
                new_return(7);
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writecheats()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writeguys(PACKFILE *f, zquestheader *Header)
{
	//these are here to bypass compiler warnings about unused arguments
	Header=Header;
	
	dword section_id=ID_GUYS;
	dword section_version=V_GUYS;
	dword section_cversion=CV_GUYS;
	dword section_size=0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
		{
			new_return(4);
		}
		
		writesize=0;
		
		//finally...  section data
		for(int32_t i=0; i<MAXGUYS; i++)
		{
			if(!pfwrite((char *)guy_string[i], 64, f))
			{
				new_return(5);
			}
		}
		
		for(int32_t i=0; i<MAXGUYS; i++)
		{
			uint32_t flags1 = uint32_t(guysbuf[i].flags);
			uint32_t flags2 = uint32_t(guysbuf[i].flags >> 32ULL);
			if (!p_iputl(flags1, f))
			{
				new_return(6);
			}
			if (!p_iputl(flags2, f))
			{
				new_return(7);
			}
			
			if(!p_iputl(guysbuf[i].tile,f))
			{
				new_return(8);
			}
			
			if(!p_putc(guysbuf[i].width,f))
			{
				new_return(9);
			}
			
			if(!p_putc(guysbuf[i].height,f))
			{
				new_return(10);
			}
			
			if(!p_iputl(guysbuf[i].s_tile,f))
			{
				new_return(11);
			}
			
			if(!p_putc(guysbuf[i].s_width,f))
			{
				new_return(12);
			}
			
			if(!p_putc(guysbuf[i].s_height,f))
			{
				new_return(13);
			}
			
			if(!p_iputl(guysbuf[i].e_tile,f))
			{
				new_return(14);
			}
			
			if(!p_putc(guysbuf[i].e_width,f))
			{
				new_return(15);
			}
			
			if(!p_putc(guysbuf[i].e_height,f))
			{
				new_return(16);
			}
			
			if(!p_iputw(guysbuf[i].hp,f))
			{
				new_return(17);
			}
			
			if(!p_iputw(guysbuf[i].family,f))
			{
				new_return(18);
			}
			
			if(!p_iputw(guysbuf[i].cset,f))
			{
				new_return(19);
			}
			
			if(!p_iputw(guysbuf[i].anim,f))
			{
				new_return(20);
			}
			
			if(!p_iputw(guysbuf[i].e_anim,f))
			{
				new_return(21);
			}
			
			if(!p_iputw(guysbuf[i].frate,f))
			{
				new_return(22);
			}
			
			if(!p_iputw(guysbuf[i].e_frate,f))
			{
				new_return(23);
			}
			
			if(!p_iputw(guysbuf[i].dp,f))
			{
				new_return(24);
			}
			
			if(!p_iputw(guysbuf[i].wdp,f))
			{
				new_return(25);
			}
			
			if(!p_iputw(guysbuf[i].weapon,f))
			{
				new_return(26);
			}
			
			if(!p_iputw(guysbuf[i].rate,f))
			{
				new_return(27);
			}
			
			if(!p_iputw(guysbuf[i].hrate,f))
			{
				new_return(28);
			}
			
			if(!p_iputw(guysbuf[i].step,f))
			{
				new_return(29);
			}
			
			if(!p_iputw(guysbuf[i].homing,f))
			{
				new_return(30);
			}
			
			if(!p_iputw(guysbuf[i].grumble,f))
			{
				new_return(31);
			}
			
			if(!p_iputw(guysbuf[i].item_set,f))
			{
				new_return(32);
			}
			
			if(!p_iputl(guysbuf[i].attributes[0], f))
			{
				new_return(33);
			}
			
			if(!p_iputl(guysbuf[i].attributes[1],f))
			{
				new_return(34);
			}
			
			if(!p_iputl(guysbuf[i].attributes[2],f))
			{
				new_return(35);
			}
			
			if(!p_iputl(guysbuf[i].attributes[3],f))
			{
				new_return(36);
			}
			
			if(!p_iputl(guysbuf[i].attributes[4],f))
			{
				new_return(37);
			}
			
			if(!p_iputl(guysbuf[i].attributes[5],f))
			{
				new_return(38);
			}
			
			if(!p_iputl(guysbuf[i].attributes[6],f))
			{
				new_return(39);
			}
			
			if(!p_iputl(guysbuf[i].attributes[7],f))
			{
				new_return(40);
			}
			
			if(!p_iputl(guysbuf[i].attributes[8],f))
			{
				new_return(41);
			}
			
			if(!p_iputl(guysbuf[i].attributes[9],f))
			{
				new_return(42);
			}
			
			if(!p_iputw(guysbuf[i].bgsfx,f))
			{
				new_return(43);
			}
			
			if(!p_iputw(guysbuf[i].bosspal,f))
			{
				new_return(44);
			}
			
			if(!p_iputw(guysbuf[i].extend,f))
			{
				new_return(45);
			}
			
			for(int32_t j=0; j < edefLAST; j++)
			{
				if(!p_putc(guysbuf[i].defense[j],f))
				{
					new_return(46);
				}
			}
			
			if ( FFCore.getQuestHeaderInfo(vZelda) < 0x250 || (( FFCore.getQuestHeaderInfo(vZelda) == 0x250 ) && FFCore.getQuestHeaderInfo(vBuild) < 32 ) )
			{
				//If no user-set hit sound was in place, and the quest was made in a version before 2.53.0 Gamma 2:
				if ( guysbuf[i].hitsfx == 0 ) guysbuf[i].hitsfx = WAV_EHIT; //Fix quests using the wrong hit sound when loading this. 
				//Force SFX_HIT here. 
			
			}
		
			if(!p_putc(guysbuf[i].hitsfx,f))
			{
				new_return(47);
			}
			
			if(!p_putc(guysbuf[i].deadsfx,f))
			{
				new_return(48);
			}
			
			if(!p_iputl(guysbuf[i].attributes[10],f))
			{
				new_return(49);
			}
			
			if(!p_iputl(guysbuf[i].attributes[11],f))
			{
				new_return(50);
			}
			
			//New 2.6 defences
			for(int32_t j=edefLAST; j < edefLAST255; j++)
			{
				if(!p_putc(guysbuf[i].defense[j],f))
				{
					new_return(51);
				}
			}
			
			//tilewidth, tileheight, hitwidth, hitheight, hitzheight, hitxofs, hityofs, hitzofs
			if(!p_iputl(guysbuf[i].txsz,f))
			{
				new_return(52);
			}
			if(!p_iputl(guysbuf[i].tysz,f))
			{
				new_return(53);
			}
			if(!p_iputl(guysbuf[i].hxsz,f))
			{
				new_return(54);
			}
			if(!p_iputl(guysbuf[i].hysz,f))
			{
				new_return(55);
			}
			if(!p_iputl(guysbuf[i].hzsz,f))
			{
				new_return(56);
			}
			// These are not fixed types, but ints, so they are safe to use here. 
			if(!p_iputl(guysbuf[i].hxofs,f))
			{
				new_return(57);
			}
			if(!p_iputl(guysbuf[i].hyofs,f))
			{
				new_return(58);
			}
			if(!p_iputl(guysbuf[i].xofs,f))
			{
				new_return(59);
			}
			if(!p_iputl(guysbuf[i].yofs,f))
			{
				new_return(60);
			}
			if(!p_iputl(guysbuf[i].zofs,f))
			{
				new_return(61);
			}
			if(!p_iputl(guysbuf[i].wpnsprite,f))
			{
				new_return(62);
			}
			if(!p_iputl(guysbuf[i].SIZEflags,f))
			{
				new_return(63);
			}
			if(!p_iputl(guysbuf[i].frozentile,f))
			{
				new_return(64);
			}
			if(!p_iputl(guysbuf[i].frozencset,f))
			{
				new_return(65);
			}
			if(!p_iputl(guysbuf[i].frozenclock,f))
			{
				new_return(66);
			}
			
			for ( int32_t q = 0; q < 10; q++ ) 
			{
				if(!p_iputw(guysbuf[i].frozenmisc[q],f))
				{
					new_return(67);
				}
			}
			if(!p_iputw(guysbuf[i].firesfx,f))
			{
				new_return(68);
			}
			//misc 16->31
			if(!p_iputl(guysbuf[i].attributes[15],f))
			{
				new_return(69);
			}
			if(!p_iputl(guysbuf[i].attributes[16],f))
			{
				new_return(70);
			}
			if(!p_iputl(guysbuf[i].attributes[17],f))
			{
				new_return(71);
			}
			if(!p_iputl(guysbuf[i].attributes[18],f))
			{
				new_return(72);
			}
			if(!p_iputl(guysbuf[i].attributes[19],f))
			{
				new_return(73);
			}
			if(!p_iputl(guysbuf[i].attributes[20],f))
			{
				new_return(74);
			}
			if(!p_iputl(guysbuf[i].attributes[21],f))
			{
				new_return(75);
			}
			if(!p_iputl(guysbuf[i].attributes[22],f))
			{
				new_return(76);
			}
			if(!p_iputl(guysbuf[i].attributes[23],f))
			{
				new_return(77);
			}
			if(!p_iputl(guysbuf[i].attributes[24],f))
			{
				new_return(78);
			}
			if(!p_iputl(guysbuf[i].attributes[25],f))
			{
				new_return(79);
			}
			if(!p_iputl(guysbuf[i].attributes[26],f))
			{
				new_return(80);
			}
			if(!p_iputl(guysbuf[i].attributes[27],f))
			{
				new_return(81);
			}
			if(!p_iputl(guysbuf[i].attributes[28],f))
			{
				new_return(82);
			}
			if(!p_iputl(guysbuf[i].attributes[29],f))
			{
				new_return(83);
			}
			if(!p_iputl(guysbuf[i].attributes[30],f))
			{
				new_return(84);
			}
			if(!p_iputl(guysbuf[i].attributes[31],f))
			{
				new_return(85);
			}
			for ( int32_t q = 0; q < 32; q++ )
			{
				if(!p_iputl(guysbuf[i].movement[q],f))
				{
					new_return(86);
				}
			}
			for ( int32_t q = 0; q < 32; q++ )
			{
				if(!p_iputl(guysbuf[i].new_weapon[q],f))
				{
					new_return(87);
				}
			}
			if(!p_iputw(guysbuf[i].script,f))
			{
				new_return(88);
			}
			for ( int32_t q = 0; q < 8; q++ )
			{
				if(!p_iputl(guysbuf[i].initD[q],f))
				{
					new_return(89);
				}
			}
			for ( int32_t q = 0; q < 2; q++ )
			{
				if(!p_iputl(0,f))
				{
					new_return(90);
				}
			}
			if(!p_iputl(guysbuf[i].editorflags,f))
			{
				new_return(91);
			}
			//somehow forgot these in the older builds -Z
			if(!p_iputl(guysbuf[i].attributes[12],f))
			{
				new_return(92);
			}
			if(!p_iputl(guysbuf[i].attributes[13],f))
			{
				new_return(93);
			}
			if(!p_iputl(guysbuf[i].attributes[14],f))
			{
				new_return(94);
			}
			
			//Enemy Editor InitD[] labels
			for ( int32_t q = 0; q < 8; q++ )
			{
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_putc(guysbuf[i].initD_label[q][w],f))
					{
						new_return(95);
					} 
				}
				for ( int32_t w = 0; w < 65; w++ )
				{
					if(!p_putc(guysbuf[i].weapon_initD_label[q][w],f))
					{
						new_return(96);
					} 
				}
			}
			if(!p_iputw(guysbuf[i].weaponscript,f))
			{
				new_return(97);
			}
			//eweapon initD
			for ( int32_t q = 0; q < 8; q++ )
			{
				if(!p_iputl(guysbuf[i].weap_initiald[q],f))
				{
					new_return(98);
				}
			}
			if(!p_iputl(guysbuf[i].moveflags,f))
				new_return(99);
			if(!p_putc(guysbuf[i].spr_shadow,f))
				new_return(100);
			if(!p_putc(guysbuf[i].spr_death,f))
				new_return(101);
			if(!p_putc(guysbuf[i].spr_spawn,f))
				new_return(102);
			if (!p_putc(guysbuf[i].wunblockable, f))
				new_return(103);
			if (!p_iputl(guysbuf[i].wmoveflags, f))
				new_return(104);
			if (!p_iputl(guysbuf[i].weapoverrideFLAGS, f))
				new_return(105);
			if (!p_iputl(guysbuf[i].weap_tilew, f))
				new_return(106);
			if (!p_iputl(guysbuf[i].weap_tileh, f))
				new_return(107);
			if (!p_iputl(guysbuf[i].weap_hxsz, f))
				new_return(108);
			if (!p_iputl(guysbuf[i].weap_hysz, f))
				new_return(109);
			if (!p_iputl(guysbuf[i].weap_hzsz, f))
				new_return(110);
			if (!p_iputl(guysbuf[i].weap_hxofs, f))
				new_return(111);
			if (!p_iputl(guysbuf[i].weap_hyofs, f))
				new_return(112);
			if (!p_iputl(guysbuf[i].weap_xofs, f))
				new_return(113);
			if (!p_iputl(guysbuf[i].weap_yofs, f))
				new_return(114);
			if (!p_iputl(guysbuf[i].wstep, f))
				new_return(115);
			for(int32_t q = 0; q < WPNSPR_MAX; ++q)
			{
				if (!p_iputw(guysbuf[i].burnsprs[q], f))
					new_return(116);
				if (!p_iputw(guysbuf[i].light_rads[q], f))
					new_return(117);
			}
			if (!p_putc(guysbuf[i].specialsfx, f))
				new_return(118);
		}
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writeguys()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

int32_t writeherosprites(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_HEROSPRITES;
    dword section_version=V_HEROSPRITES;
    dword section_cversion=CV_HEROSPRITES;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(walkspr[i][spr_tile],f))
            {
                new_return(5);
            }
            
            if(!p_putc((byte)walkspr[i][spr_flip],f))
            {
                new_return(5);
            }
            
            if(!p_putc((byte)walkspr[i][spr_extend],f))
            {
                new_return(5);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(stabspr[i][spr_tile],f))
            {
                new_return(6);
            }
            
            if(!p_putc((byte)stabspr[i][spr_flip],f))
            {
                new_return(6);
            }
            
            if(!p_putc((byte)stabspr[i][spr_extend],f))
            {
                new_return(6);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(slashspr[i][spr_tile],f))
            {
                new_return(7);
            }
            
            if(!p_putc((byte)slashspr[i][spr_flip],f))
            {
                new_return(7);
            }
            
            if(!p_putc((byte)slashspr[i][spr_extend],f))
            {
                new_return(7);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(floatspr[i][spr_tile],f))
            {
                new_return(8);
            }
            
            if(!p_putc((byte)floatspr[i][spr_flip],f))
            {
                new_return(8);
            }
            
            if(!p_putc((byte)floatspr[i][spr_extend],f))
            {
                new_return(8);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(swimspr[i][spr_tile],f))
            {
                new_return(8);
            }
            
            if(!p_putc((byte)swimspr[i][spr_flip],f))
            {
                new_return(8);
            }
            
            if(!p_putc((byte)swimspr[i][spr_extend],f))
            {
                new_return(8);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(divespr[i][spr_tile],f))
            {
                new_return(9);
            }
            
            if(!p_putc((byte)divespr[i][spr_flip],f))
            {
                new_return(9);
            }
            
            if(!p_putc((byte)divespr[i][spr_extend],f))
            {
                new_return(9);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(poundspr[i][spr_tile],f))
            {
                new_return(10);
            }
            
            if(!p_putc((byte)poundspr[i][spr_flip],f))
            {
                new_return(10);
            }
            
            if(!p_putc((byte)poundspr[i][spr_extend],f))
            {
                new_return(10);
            }
        }
        
        if(!p_iputl(castingspr[spr_tile],f))
        {
            new_return(11);
        }
        
        if(!p_putc((byte)castingspr[spr_flip],f))
        {
            new_return(11);
        }
        
        if(!p_putc((byte)castingspr[spr_extend],f))
        {
            new_return(11);
        }
        
        for(int32_t i=0; i<2; i++)
        {
            for(int32_t j=0; j<spr_holdmax; j++)
            {
                if(!p_iputl(holdspr[i][j][spr_tile],f))
                {
                    new_return(12);
                }
                
                if(!p_putc((byte)holdspr[i][j][spr_flip],f))
                {
                    new_return(12);
                }
                
                if(!p_putc((byte)holdspr[i][j][spr_extend],f))
                {
                    new_return(12);
                }
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(jumpspr[i][spr_tile],f))
            {
                new_return(13);
            }
            
            if(!p_putc((byte)jumpspr[i][spr_flip],f))
            {
                new_return(13);
            }
            
            if(!p_putc((byte)jumpspr[i][spr_extend],f))
            {
                new_return(13);
            }
        }
        
        for(int32_t i=0; i<4; i++)
        {
            if(!p_iputl(chargespr[i][spr_tile],f))
            {
                new_return(13);
            }
            
            if(!p_putc((byte)chargespr[i][spr_flip],f))
            {
                new_return(13);
            }
            
            if(!p_putc((byte)chargespr[i][spr_extend],f))
            {
                new_return(13);
            }
        }
        
        if(!p_putc((byte)zinit.hero_swim_speed,f))
        {
            new_return(14);
        }
		
		//{ V_HEROSPRITES >= 7
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(frozenspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)frozenspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)frozenspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(frozen_waterspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)frozen_waterspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)frozen_waterspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(onfirespr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)onfirespr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)onfirespr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(onfire_waterspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)onfire_waterspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)onfire_waterspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(diggingspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)diggingspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)diggingspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(usingrodspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)usingrodspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)usingrodspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(usingcanespr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)usingcanespr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)usingcanespr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(pushingspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)pushingspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)pushingspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(liftingspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)liftingspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)liftingspr[q][spr_extend],f))
				new_return(15);
			if(!p_putc((byte)liftingspr[q][spr_frames],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(liftingwalkspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)liftingwalkspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)liftingwalkspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(stunnedspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)stunnedspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)stunnedspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(stunned_waterspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)stunned_waterspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)stunned_waterspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(drowningspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)drowningspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)drowningspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(drowning_lavaspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)drowning_lavaspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)drowning_lavaspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(fallingspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)fallingspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)fallingspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(shockedspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)shockedspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)shockedspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(shocked_waterspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)shocked_waterspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)shocked_waterspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(pullswordspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)pullswordspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)pullswordspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(readingspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)readingspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)readingspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(slash180spr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)slash180spr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)slash180spr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(slashZ4spr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)slashZ4spr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)slashZ4spr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(dashspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)dashspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)dashspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(bonkspr[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)bonkspr[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)bonkspr[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 3; ++q) //Not directions; number of medallion sprs
		{
			if(!p_iputl(medallionsprs[q][spr_tile],f))
				new_return(15);
			if(!p_putc((byte)medallionsprs[q][spr_flip],f))
				new_return(15);
			if(!p_putc((byte)medallionsprs[q][spr_extend],f))
				new_return(15);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sideswimspr[q][spr_tile],f))
				new_return(16);
			if(!p_putc((byte)sideswimspr[q][spr_flip],f))
				new_return(16);
			if(!p_putc((byte)sideswimspr[q][spr_extend],f))
				new_return(16);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sideswimslashspr[q][spr_tile],f))
				new_return(17);
			if(!p_putc((byte)sideswimslashspr[q][spr_flip],f))
				new_return(17);
			if(!p_putc((byte)sideswimslashspr[q][spr_extend],f))
				new_return(17);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sideswimstabspr[q][spr_tile],f))
				new_return(17);
			if(!p_putc((byte)sideswimstabspr[q][spr_flip],f))
				new_return(17);
			if(!p_putc((byte)sideswimstabspr[q][spr_extend],f))
				new_return(17);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sideswimpoundspr[q][spr_tile],f))
				new_return(17);
			if(!p_putc((byte)sideswimpoundspr[q][spr_flip],f))
				new_return(17);
			if(!p_putc((byte)sideswimpoundspr[q][spr_extend],f))
				new_return(17);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sideswimchargespr[q][spr_tile],f))
				new_return(18);
			if(!p_putc((byte)sideswimchargespr[q][spr_flip],f))
				new_return(18);
			if(!p_putc((byte)sideswimchargespr[q][spr_extend],f))
				new_return(18);
		}
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(hammeroffsets[q],f))
				new_return(19);
		}
		for(int32_t q = 0; q < 3; ++q)
		{
			if(!p_iputl(sideswimholdspr[q][spr_tile],f))
				new_return(20);
			if(!p_putc((byte)sideswimholdspr[q][spr_flip],f))
				new_return(20);
			if(!p_putc((byte)sideswimholdspr[q][spr_extend],f))
				new_return(20);
		}
		
		if(!p_iputl(sideswimcastingspr[spr_tile],f))
		{
		    new_return(21);
		}
		
		if(!p_putc((byte)sideswimcastingspr[spr_flip],f))
		{
		    new_return(21);
		}
		
		if(!p_putc((byte)sideswimcastingspr[spr_extend],f))
		{
		    new_return(21);
		}
		
		for(int32_t q = 0; q < 4; ++q)
		{
			if(!p_iputl(sidedrowningspr[q][spr_tile],f))
				new_return(22);
			if(!p_putc((byte)sidedrowningspr[q][spr_flip],f))
				new_return(22);
			if(!p_putc((byte)sidedrowningspr[q][spr_extend],f))
				new_return(22);
		}
		
		for(int32_t i=0; i<4; i++)
		{
			if(!p_iputl(revslashspr[i][spr_tile],f))
			{
				new_return(23);
			}
		    
			if(!p_putc((byte)revslashspr[i][spr_flip],f))
			{
				new_return(23);
			}
		    
			if(!p_putc((byte)revslashspr[i][spr_extend],f))
			{
				new_return(23);
			}
		}
        
		
        for (int32_t q = 0; q < wMax; q++) // Hero defense values
        {
            if (!p_putc(hero_defence[q], f))
                new_return(15);
        }
		//}
		
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    //More data will come here
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeherosprites()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writesubscreens(PACKFILE *f, zquestheader *Header)
{
	dword section_id=ID_SUBSCREEN;
	dword section_version=V_SUBSCREEN;
	dword section_cversion=CV_SUBSCREEN;
	dword section_size=0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
		{
			new_return(4);
		}
		
		writesize=0;
		
		byte sz = subscreens_active.size();
		if(!p_putc(sz,f))
			new_return(5);
		for(int32_t i=0; i<sz; i++)
		{
			int32_t ret = subscreens_active[i].write(f);
			fake_pack_writing=(writecycle==0);
			
			if(ret!=0)
				new_return(ret);
		}
		
		sz = subscreens_passive.size();
		if(!p_putc(sz,f))
			new_return(5);
		for(int32_t i=0; i<sz; i++)
		{
			int32_t ret = subscreens_passive[i].write(f);
			fake_pack_writing=(writecycle==0);
			
			if(ret!=0)
				new_return(ret);
		}
		
		sz = subscreens_overlay.size();
		if(!p_putc(sz,f))
			new_return(5);
		for(int32_t i=0; i<sz; i++)
		{
			int32_t ret = subscreens_overlay[i].write(f);
			fake_pack_writing=(writecycle==0);
			
			if(ret!=0)
				new_return(ret);
		}
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writesubscreens()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

extern script_data *ffscripts[NUMSCRIPTFFC];
extern script_data *itemscripts[NUMSCRIPTITEM];
extern script_data *guyscripts[NUMSCRIPTGUYS];
extern script_data *lwpnscripts[NUMSCRIPTWEAPONS];
extern script_data *ewpnscripts[NUMSCRIPTWEAPONS];
extern script_data *globalscripts[NUMSCRIPTGLOBAL];
extern script_data *genericscripts[NUMSCRIPTSGENERIC];
extern script_data *playerscripts[NUMSCRIPTHERO];
extern script_data *screenscripts[NUMSCRIPTSCREEN];
extern script_data *dmapscripts[NUMSCRIPTSDMAP];
extern script_data *itemspritescripts[NUMSCRIPTSITEMSPRITE];
extern script_data *comboscripts[NUMSCRIPTSCOMBODATA];
extern script_data *subscreenscripts[NUMSCRIPTSSUBSCREEN];

int32_t writeffscript(PACKFILE *f, zquestheader *Header)
{
    dword section_id       = ID_FFSCRIPT;
    dword section_version  = V_FFSCRIPT;
    dword section_cversion = CV_FFSCRIPT;
    dword section_size     = 0;
	dword zasmmeta_version = METADATA_V;
    byte numscripts        = 0;
    numscripts = numscripts; //to avoid unused variables warnings
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    if(!p_iputw(zasmmeta_version,f))
    {
        new_return(4);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(5);
        }
        
        writesize=0;
        
        for(int32_t i=0; i<NUMSCRIPTFFC; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &ffscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
        for(int32_t i=0; i<NUMSCRIPTITEM; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &itemscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
        for(int32_t i=0; i<NUMSCRIPTGUYS; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &guyscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
		script_data *fake = new script_data(ScriptType::None, 0);
        for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &fake);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		delete fake;
        
        for(int32_t i=0; i<NUMSCRIPTSCREEN; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &screenscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
        for(int32_t i=0; i<NUMSCRIPTGLOBAL; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &globalscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
        for(int32_t i=0; i<NUMSCRIPTHERO; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &playerscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
        for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &lwpnscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
		for(int32_t i=0; i<NUMSCRIPTWEAPONS; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &ewpnscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
		for(int32_t i=0; i<NUMSCRIPTSDMAP; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &dmapscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
		for(int32_t i=0; i<NUMSCRIPTSITEMSPRITE; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &itemspritescripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
		for(int32_t i=0; i<NUMSCRIPTSCOMBODATA; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &comboscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
		if(!p_iputw(NUMSCRIPTSGENERIC,f))
		{
			new_return(2000);
		}
		for(int32_t i=0; i<NUMSCRIPTSGENERIC; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &genericscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
		
		if(!p_iputw(NUMSCRIPTSSUBSCREEN,f))
		{
			new_return(2001);
		}
		for(int32_t i=0; i<NUMSCRIPTSSUBSCREEN; i++)
        {
            int32_t ret = write_one_ffscript(f, Header, i, &subscreenscripts[i]);
            fake_pack_writing=(writecycle==0);
            
            if(ret!=0)
            {
                new_return(ret);
            }
        }
        
        if(!p_iputl((int32_t)zScript.size(), f))
        {
            new_return(2001);
        }
        
        if(!pfwrite((void *)zScript.c_str(), (int32_t)zScript.size(), f))
        {
            new_return(2002);
        }
        
        word numffcbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = ffcmap.begin(); it != ffcmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numffcbindings++;
            }
        }
        
        if(!p_iputw(numffcbindings, f))
        {
            new_return(2003);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = ffcmap.begin(); it != ffcmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2004);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2005);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2006);
                }
            }
        }
        
        word numglobalbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = globalmap.begin(); it != globalmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numglobalbindings++;
            }
        }
        
        if(!p_iputw(numglobalbindings, f))
        {
            new_return(2007);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = globalmap.begin(); it != globalmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2008);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2009);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2010);
                }
            }
        }
        
        word numitembindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = itemmap.begin(); it != itemmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numitembindings++;
            }
        }
        
        if(!p_iputw(numitembindings, f))
        {
            new_return(2011);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = itemmap.begin(); it != itemmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2012);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2013);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2014);
                }
            }
        }
        
        //new script types
        //npc scripts
        word numnpcbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = npcmap.begin(); it != npcmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numnpcbindings++;
            }
        }
        
        if(!p_iputw(numnpcbindings, f))
        {
            new_return(2015);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = npcmap.begin(); it != npcmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2016);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2017);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2018);
                }
            }
        }
        
        //lweapon
	
	word numlwpnbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = lwpnmap.begin(); it != lwpnmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numlwpnbindings++;
            }
        }
        
        if(!p_iputw(numlwpnbindings, f))
        {
            new_return(2019);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = lwpnmap.begin(); it != lwpnmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2020);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2021);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2022);
                }
            }
        }
	
	//////
	
	//eweapon
	
	
        word numewpnbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = ewpnmap.begin(); it != ewpnmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numewpnbindings++;
            }
        }
        
        if(!p_iputw(numewpnbindings, f))
        {
            new_return(2023);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = ewpnmap.begin(); it != ewpnmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2024);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2025);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2026);
                }
            }
        }
	
	//player scripts
	word numherobindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = playermap.begin(); it != playermap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numherobindings++;
            }
        }
        
        if(!p_iputw(numherobindings, f))
        {
            new_return(2027);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = playermap.begin(); it != playermap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2028);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2029);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2030);
                }
            }
        }
	
	//dmap scripts
	word numdmapbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = dmapmap.begin(); it != dmapmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numdmapbindings++;
            }
        }
        
        if(!p_iputw(numdmapbindings, f))
        {
            new_return(2031);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = dmapmap.begin(); it != dmapmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2032);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2033);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2034);
                }
            }
        }
	
	//screen scripts
	word numscreenbindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = screenmap.begin(); it != screenmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numscreenbindings++;
            }
        }
        
        if(!p_iputw(numscreenbindings, f))
        {
            new_return(2035);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = screenmap.begin(); it != screenmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2036);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2037);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2038);
                }
            }
        }
        //item sprite scripts
	word numitemspritebindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = itemspritemap.begin(); it != itemspritemap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numitemspritebindings++;
            }
        }
        
        if(!p_iputw(numitemspritebindings, f))
        {
            new_return(2039);
        }
        
        for(std::map<int32_t, script_slot_data >::iterator it = itemspritemap.begin(); it != itemspritemap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2040);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2041);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2042);
                }
            }
        }
		
		//combo scripts
		word numcombobindings=0;
        
        for(std::map<int32_t, script_slot_data >::iterator it = comboscriptmap.begin(); it != comboscriptmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numcombobindings++;
            }
        }
		if(!p_iputw(numcombobindings, f))
        {
            new_return(2043);
        }
		
        for(std::map<int32_t, script_slot_data >::iterator it = comboscriptmap.begin(); it != comboscriptmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2044);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2045);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2046);
                }
            }
        }
		//subscreen scripts
		word numgenericbindings=0;
        
        for(auto it = genericmap.begin(); it != genericmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numgenericbindings++;
            }
        }
		if(!p_iputw(numgenericbindings, f))
        {
            new_return(2043);
        }
		
        for(auto it = genericmap.begin(); it != genericmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2044);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2045);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2046);
                }
            }
        }
        
		//generic scripts
		word numsubscreenbindings=0;
        
        for(auto it = subscreenmap.begin(); it != subscreenmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                numsubscreenbindings++;
            }
        }
		if(!p_iputw(numsubscreenbindings, f))
        {
            new_return(2047);
        }
		
        for(auto it = subscreenmap.begin(); it != subscreenmap.end(); it++)
        {
            if(it->second.scriptname != "")
            {
                if(!p_iputw(it->first,f))
                {
                    new_return(2048);
                }
                
                if(!p_iputl((int32_t)it->second.scriptname.size(), f))
                {
                    new_return(2049);
                }
                
                if(!pfwrite((void *)it->second.scriptname.c_str(), (int32_t)it->second.scriptname.size(),f))
                {
                    new_return(2050);
                }
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
        
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeffscript()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
    //return 0;  //this is just here to stomp the compiler from whining.
    //the irony is that it causes an "unreachable code" warning.
}

int32_t write_one_ffscript(PACKFILE *f, zquestheader *Header, int32_t i, script_data **script)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    i=i;
    
    size_t num_commands = (*script)->zasm_script ? (*script)->zasm_script->size : 0;
    
    if(!p_iputl(num_commands,f))
    {
        new_return(6);
    }
	
	//Metadata
	zasm_meta const& tmeta = (*script)->meta;
	if(!p_iputw(tmeta.zasm_v,f))
	{
		new_return(7);
	}
	
	if(!p_iputw(tmeta.meta_v,f))
	{
		new_return(8);
	}
	
	if(!p_iputw(tmeta.ffscript_v,f))
	{
		new_return(9);
	}
	
	if(!p_putc((int)tmeta.script_type,f))
	{
		new_return(10);
	}
	
	for(int32_t q = 0; q < 8; ++q)
	{
		if(!p_putcstr(tmeta.run_idens[q],f))
			new_return(11);
	}
	
	for(int32_t q = 0; q < 8; ++q)
	{
		if(!p_putc(tmeta.run_types[q],f))
		{
			new_return(12);
		}
	}
	
	if(!p_putc(tmeta.flags,f))
	{
		new_return(13);
	}
	
	if(!p_iputw(tmeta.compiler_v1,f))
	{
		new_return(14);
	}
	
	if(!p_iputw(tmeta.compiler_v2,f))
	{
		new_return(15);
	}
	
	if(!p_iputw(tmeta.compiler_v3,f))
	{
		new_return(16);
	}
	
	if(!p_iputw(tmeta.compiler_v4,f))
	{
		new_return(17);
	}
	
	if(!p_putcstr(tmeta.script_name,f))
		new_return(18);
	if(!p_putcstr(tmeta.author,f))
		new_return(19);
	for(auto q = 0; q < 10; ++q)
	{
		if(!p_putcstr(tmeta.attributes[q],f))
			new_return(27);
		if(!p_putwstr(tmeta.attributes_help[q],f))
			new_return(28);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(!p_putcstr(tmeta.attribytes[q],f))
			new_return(29);
		if(!p_putwstr(tmeta.attribytes_help[q],f))
			new_return(30);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(!p_putcstr(tmeta.attrishorts[q],f))
			new_return(31);
		if(!p_putwstr(tmeta.attrishorts_help[q],f))
			new_return(32);
	}
	for(auto q = 0; q < 16; ++q)
	{
		if(!p_putcstr(tmeta.usrflags[q],f))
			new_return(33);
		if(!p_putwstr(tmeta.usrflags_help[q],f))
			new_return(34);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(!p_putcstr(tmeta.initd[q],f))
			new_return(35);
		if(!p_putwstr(tmeta.initd_help[q],f))
			new_return(36);
	}
	for(auto q = 0; q < 8; ++q)
	{
		if(!p_putc(tmeta.initd_type[q],f))
			new_return(37);
	}
	
    for(int32_t j=0; j<num_commands; j++)
    {
        auto& zas = (*script)->zasm_script->zasm[j];
        if(!p_iputw(zas.command,f))
        {
            new_return(20);
        }
        
        if(zas.command==0xFFFF)
        {
            break;
        }
        else
        {
            if(!p_iputl(zas.arg1,f))
            {
                new_return(21);
            }
            
            if(!p_iputl(zas.arg2,f))
            {
                new_return(22);
            }
            
            if(!p_iputl(zas.arg3,f))
            {
                new_return(23);
            }
			
			uint32_t sz = 0;
			if(zas.strptr)
				sz = zas.strptr->size();
			if(!p_iputl(sz,f))
			{
                new_return(23);
			}
			if(sz)
			{
				auto& str = *zas.strptr;
				for(size_t q = 0; q < sz; ++q)
				{
					if(!p_putc(str[q],f))
					{
						new_return(24);
					}
				}
			}
			sz = 0;
			if(zas.vecptr)
				sz = zas.vecptr->size();
			if(!p_iputl(sz,f))
			{
                new_return(25);
			}
			if(sz) //vector found
			{
				auto& vec = *zas.vecptr;
				for(size_t q = 0; q < sz; ++q)
				{
					if(!p_iputl(vec[q],f))
					{
						new_return(26);
					}
				}
			}
        }
    }
    
    new_return(0);
}

extern SAMPLE customsfxdata[WAV_COUNT];
extern uint8_t customsfxflag[WAV_COUNT>>3];

int32_t writesfx(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_SFX;
    dword section_version=V_SFX;
    dword section_cversion=CV_SFX;
    dword section_size=0;
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        for(int32_t i=0; i<WAV_COUNT>>3; i++)
        {
            if(!p_putc(customsfxflag[i],f))
            {
                new_return(5);
            }
        }
        
        for(int32_t i=1; i<WAV_COUNT; i++)
        {
            if(get_bit(customsfxflag, i-1) == 0)
                continue;
                
            if(!pfwrite(sfx_string[i], 36, f))
            {
                new_return(5);
            }
        }
        
        for(int32_t i=1; i<WAV_COUNT; i++)
        {
            if(get_bit(customsfxflag, i-1) == 0)
                continue;
                
            if(!p_iputl(customsfxdata[i].bits,f))
            {
                new_return(5);
            }
            
            if(!p_iputl(customsfxdata[i].stereo,f))
            {
                new_return(6);
            }
            
            if(!p_iputl(customsfxdata[i].freq,f))
            {
                new_return(7);
            }
            
            if(!p_iputl(customsfxdata[i].priority,f))
            {
                new_return(8);
            }
            
            if(!p_iputl(customsfxdata[i].len,f))
            {
                new_return(9);
            }
            
            if(!p_iputl(customsfxdata[i].loop_start,f))
            {
                new_return(10);
            }
            
            if(!p_iputl(customsfxdata[i].loop_end,f))
            {
                new_return(11);
            }
            
            if(!p_iputl(customsfxdata[i].param,f))
            {
                new_return(12);
            }
            
            //de-endianfy the data
            int32_t wordstowrite = (customsfxdata[i].bits==8?1:2)*(customsfxdata[i].stereo==0?1:2)*customsfxdata[i].len/sizeof(word);
            
            for(int32_t j=0; j<wordstowrite; j++)
            {
                if(!p_iputw(((word *)customsfxdata[i].data)[j],f))
                {
                    new_return(13);
                }
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writesfx()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writeinitdata(PACKFILE *f, zquestheader *)
{
	dword section_id=ID_INITDATA;
	dword section_version=V_INITDATA;
	dword section_cversion=CV_INITDATA;
	dword section_size = 0;
	
	zinit.last_map=Map.getCurrMap();
	zinit.last_screen=Map.getCurrScr();
	zinit.normalize();
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	//TODO
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
			new_return(4);
		
		writesize=0;
		
		for(int q = 0; q < MAXITEMS/8; ++q)
			if(!p_putc(zinit.items[q], f))
				new_return(5);
		for(int q = 0; q < MAXLEVELS/8; ++q)
		{
			if(!p_putc(zinit.map[q], f))
				new_return(6);
			if(!p_putc(zinit.compass[q], f))
				new_return(7);
			if(!p_putc(zinit.boss_key[q], f))
				new_return(8);
			if(!p_putc(zinit.mcguffin[q], f))
				new_return(9);
		}
		if(!p_putbvec(zinit.level_keys, f))
			new_return(10);
		if(!p_putc(MAX_COUNTERS,f))
			new_return(11);
		for(int q = 0; q < MAX_COUNTERS; ++q)
			if(!p_iputw(zinit.counter[q],f))
				new_return(12);
		for(int q = 0; q < MAX_COUNTERS; ++q)
			if(!p_iputw(zinit.mcounter[q],f))
				new_return(13);
		if(!p_putc(zinit.bomb_ratio,f))
			new_return(14);
		if(!p_putc(zinit.hcp,f))
			new_return(15);
		if(!p_putc(zinit.hcp_per_hc,f))
			new_return(16);
		if(!p_iputw(zinit.cont_heart,f))
			new_return(17);
		if(!p_putc(zinit.hp_per_heart,f))
			new_return(18);
		if(!p_putc(zinit.magic_per_block,f))
			new_return(19);
		if(!p_putc(zinit.hero_damage_multiplier,f))
			new_return(20);
		if(!p_putc(zinit.ene_damage_multiplier,f))
			new_return(21);
		if(!p_putc(zinit.dither_type,f))
			new_return(22);
		if(!p_putc(zinit.dither_arg,f))
			new_return(23);
		if(!p_putc(zinit.dither_percent,f))
			new_return(24);
		if(!p_putc(zinit.def_lightrad,f))
			new_return(25);
		if(!p_putc(zinit.transdark_percent,f))
			new_return(26);
		if(!p_putc(zinit.darkcol,f))
			new_return(27);
		if(!p_iputl(zinit.ss_grid_x,f))
			new_return(28);
		if(!p_iputl(zinit.ss_grid_y,f))
			new_return(29);
		if(!p_iputl(zinit.ss_grid_xofs,f))
			new_return(30);
		if(!p_iputl(zinit.ss_grid_yofs,f))
			new_return(31);
		if(!p_iputl(zinit.ss_grid_color,f))
			new_return(32);
		if(!p_iputl(zinit.ss_bbox_1_color,f))
			new_return(33);
		if(!p_iputl(zinit.ss_bbox_2_color,f))
			new_return(34);
		if(!p_iputl(zinit.ss_flags,f))
			new_return(35);
		if(!p_putbitstr(zinit.flags,f))
			new_return(36);
		if(!p_putc(zinit.last_map,f))
			new_return(37);
		if(!p_putc(zinit.last_screen,f))
			new_return(38);
		if(!p_putc(zinit.msg_more_x,f))
			new_return(39);
		if(!p_putc(zinit.msg_more_y,f))
			new_return(40);
		if(!p_putc(zinit.msg_more_is_offset,f))
			new_return(41);
		if(!p_putc(zinit.msg_speed,f))
			new_return(42);
		if(!p_iputl(zinit.gravity,f))
			new_return(43);
		if(!p_iputl(zinit.swimgravity,f))
			new_return(44);
		if(!p_iputw(zinit.terminalv,f))
			new_return(45);
		if(!p_putc(zinit.hero_swim_speed,f))
			new_return(46);
		if(!p_putc(zinit.hero_swim_mult,f))
			new_return(47);
		if(!p_putc(zinit.hero_swim_div,f))
			new_return(48);
		if(!p_iputw(zinit.heroSideswimUpStep,f))
			new_return(49);
		if(!p_iputw(zinit.heroSideswimSideStep,f))
			new_return(50);
		if(!p_iputw(zinit.heroSideswimDownStep,f))
			new_return(51);
		if(!p_iputl(zinit.exitWaterJump,f))
			new_return(52);
		if(!p_iputw(zinit.heroStep,f))
			new_return(53);
		if(!p_putc(zinit.heroAnimationStyle,f))
			new_return(54);
		if(!p_putc(zinit.jump_hero_layer_threshold,f))
			new_return(55);
		if(!p_iputl(zinit.bunny_ltm,f))
			new_return(56);
		if(!p_iputw(zinit.start_dmap,f))
			new_return(57);
		if(!p_iputw(zinit.subscrSpeed,f))
			new_return(58);
		if(!p_putc(zinit.switchhookstyle,f))
			new_return(59);
		if(!p_putc(zinit.magicdrainrate,f))
			new_return(60);
		if(!p_iputzf(zinit.shove_offset,f))
			new_return(61);
		if(!p_putbitstr(zinit.gen_doscript, f))
			new_return(62);
		if(!p_putbmap(zinit.gen_exitState, f))
			new_return(63);
		if(!p_putbmap(zinit.gen_reloadState, f))
			new_return(64);
		if(!p_putbmap(zinit.gen_initd, f))
			new_return(65);
		if(!p_putbmap(zinit.gen_eventstate, f))
			new_return(66);
		if(!p_putbmap(zinit.gen_data, f))
			new_return(67);
		if(!p_putbmap(zinit.screen_data, f))
			new_return(68);
		if (!p_putc(zinit.spriteflickerspeed, f))
			new_return(69);
		if (!p_putc(zinit.spriteflickercolor, f))
			new_return(70);
		if (!p_putc(zinit.spriteflickertransp, f))
			new_return(71);
		if (!p_iputzf(zinit.air_drag, f))
			new_return(72);
		if (!p_iputw(zinit.light_wave_rate, f))
			new_return(73);
		if (!p_iputw(zinit.light_wave_size, f))
			new_return(74);
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writeinitdata()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

int32_t writeitemdropsets(PACKFILE *f, zquestheader *Header)
{
    //these are here to bypass compiler warnings about unused arguments
    Header=Header;
    
    dword section_id=ID_ITEMDROPSETS;
    dword section_version=V_ITEMDROPSETS;
    dword section_cversion=CV_ITEMDROPSETS;
    //  dword section_size=0;
    dword section_size = 0;
    word num_item_drop_sets=count_item_drop_sets();
    
    //section id
    if(!p_mputl(section_id,f))
    {
        new_return(1);
    }
    
    //section version info
    if(!p_iputw(section_version,f))
    {
        new_return(2);
    }
    
    if(!p_iputw(section_cversion,f))
    {
        new_return(3);
    }
    
    for(int32_t writecycle=0; writecycle<2; ++writecycle)
    {
        fake_pack_writing=(writecycle==0);
        
        //section size
        if(!p_iputl(section_size,f))
        {
            new_return(4);
        }
        
        writesize=0;
        
        //finally...  section data
        if(!p_iputw(num_item_drop_sets,f))
        {
            new_return(5);
        }
        
        for(int32_t i=0; i<num_item_drop_sets; i++)
        {
            if(!pfwrite(item_drop_sets[i].name, sizeof(item_drop_sets[i].name)-1, f))
            {
                new_return(6);
            }
            
            for(int32_t j=0; j<10; ++j)
            {
                if(!p_iputw(item_drop_sets[i].item[j],f))
                {
                    new_return(7);
                }
            }
            
            for(int32_t j=0; j<11; ++j)
            {
                if(!p_iputw(item_drop_sets[i].chance[j],f))
                {
                    new_return(8);
                }
            }
        }
        
        if(writecycle==0)
        {
            section_size=writesize;
        }
    }
    
    if(writesize!=int32_t(section_size) && save_warn)
    {
        char ebuf[80];
        sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
        jwin_alert("Error:  writeitemdropsets()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
    }
    
    new_return(0);
}

int32_t writefavorites(PACKFILE *f, zquestheader*)
{
	dword section_id=ID_FAVORITES;
	dword section_version=V_FAVORITES;
	dword section_cversion=CV_FAVORITES;
	dword section_size = 0;
	
	//section id
	if(!p_mputl(section_id,f))
	{
		new_return(1);
	}
	
	//section version info
	if(!p_iputw(section_version,f))
	{
		new_return(2);
	}
	
	if(!p_iputw(section_cversion,f))
	{
		new_return(3);
	}
	
	for(int32_t writecycle=0; writecycle<2; ++writecycle)
	{
		fake_pack_writing=(writecycle==0);
		
		//section size
		if(!p_iputl(section_size,f))
			new_return(4);
		
		writesize=0;
		
		if(!p_iputw(FAVORITECOMBO_PER_ROW,f))
			new_return(16);
		if(!p_iputw(FAVORITECOMBO_PER_PAGE,f)) // Just in case pages get resized again
			new_return(17);
		
		word favcmb_cnt = 0;
		for(int q = MAXFAVORITECOMBOS-1; q >= 0; --q)
			if(favorite_combos[q] != -1)
			{
				favcmb_cnt = q+1;
				break;
			}
		
		if(!p_iputw(favcmb_cnt,f)) // This'll probably never change, huh?
			new_return(5);
		
		for(int i=0; i<favcmb_cnt; ++i)
		{
			if (!p_putc(favorite_combo_modes[i], f))
				new_return(6);
			if (!p_iputl(favorite_combos[i], f))
				new_return(7);
		}
		
		
		word max_combo_cols = MAX_COMBO_COLS;
		if(!p_iputw(max_combo_cols,f))
			new_return(9);
		for(int q = 0; q < max_combo_cols; ++q)
		{
			if(!p_iputl(First[q],f))
				new_return(10);
			if(!p_iputl(combo_alistpos[q],f))
				new_return(11);
			if(!p_iputl(combo_pool_listpos[q],f))
				new_return(12);
		}
		word max_mappages = MAX_MAPPAGE_BTNS;
		if(!p_iputw(max_mappages,f))
			new_return(13);
		for(int q = 0; q < max_mappages; ++q)
		{
			if(!p_iputl(map_page[q].map,f))
				new_return(14);
			if(!p_iputl(map_page[q].screen,f))
				new_return(15);
		}
		
		if(writecycle==0)
		{
			section_size=writesize;
		}
	}
	
	if(writesize!=int32_t(section_size) && save_warn)
	{
		char ebuf[80];
		sprintf(ebuf, "%d != %d", writesize, int32_t(section_size));
		jwin_alert("Error:  writefavorites()","writesize != section_size",ebuf,NULL,"O&K",NULL,'k',0,get_zc_font(font_lfont));
	}
	
	new_return(0);
}

static int32_t _save_unencoded_quest_int(const char *filename, bool compressed, const char *afname)
{
	if(!afname) afname = filename;
	reset_combo_animations();
	reset_combo_animations2();
	strcpy(header.id_str,QH_NEWIDSTR);
	header.zelda_version = ZELDA_VERSION;
	header.internal = INTERNAL_VERSION;
	header.data_flags[ZQ_TILES] = true;
	header.data_flags[ZQ_CHEATS2] = 1;
	header.build=VERSION_BUILD;
	
	for(int32_t i=0; i<MAXCUSTOMMIDIS; i++)
	{
		set_bit(midi_flags,i,int32_t(customtunes[i].data!=NULL));
	}
	
	char zinfofilename[2048];
	replace_extension(zinfofilename, afname, "zinfo", 2047);
	
	box_start(1, "Saving Quest", get_zc_font(font_lfont), font, true);
	box_out("Saving Quest...");
	box_eol();
	box_eol();
	
	std::string tmp_filename = util::create_temp_file_path(filename);
	PACKFILE *f = pack_fopen_password(tmp_filename.c_str(),compressed?F_WRITE_PACKED:F_WRITE, "");
	
	if(!f)
		return 1;
	
	box_out("Writing Header...");
	
	if(writeheader(f,&header)!=0)
		return 2;
	
	box_out("okay.");
	box_eol();
	
	
	if(header.external_zinfo)
	{
		PACKFILE *inf = pack_fopen_password(zinfofilename, F_WRITE, "");
		
		box_out("Writing ZInfo...");
		if(inf)
		{
			if(writezinfo(inf,ZI)!=0)
				return 2;
			
			pack_fclose(inf);
			box_out("okay.");
		}
		else box_out(" ...file failure");
		box_eol();
	}
	else
	{
		box_out("Writing ZInfo...");
		if(writezinfo(f,ZI)!=0)
			return 2;
		box_out("okay.");
		box_eol();
	}
	
	
	box_out("Writing Rules...");
	
	if(writerules(f,&header)!=0)
		return 3;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Strings...");
	
	if(writestrings(f, ZELDA_VERSION, VERSION_BUILD, 0, MAXMSGS)!=0)
		return 4;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Doors...");
	
	if(writedoorcombosets(f,&header)!=0)
		return 5;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing DMaps...");
	
	if(writedmaps(f,header.zelda_version,header.build,0,MAXDMAPS)!=0)
		return 6;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing &QMisc. Data...");
	
	if(writemisc(f,&header)!=0)
		return 7;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing &QMisc. Colors...");
	
	if(writemisccolors(f,&header)!=0)
		return 8;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Game Icons...");
	
	if(writegameicons(f,&header)!=0)
		return 9;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Items...");
	
	if(writeitems(f,&header)!=0)
		return 10;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Weapons...");
	
	if(writeweapons(f,&header)!=0)
		return 11;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Maps...");
	
	if(writemaps(f,&header)!=0)
		return 12;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Combos...");
	
	if(writecombos(f,header.zelda_version,header.build,0,MAXCOMBOS)!=0)
		return 13;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Combo Aliases...");
	
	if(writecomboaliases(f,header.zelda_version,header.build)!=0)
		return 14;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Color Data...");
	
	if(writecolordata(f,header.zelda_version,header.build,0,newerpdTOTAL)!=0)
		return 15;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Tiles...");
	
	if(writetiles(f,header.zelda_version,header.build,0,NEWMAXTILES)!=0)
		return 16;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing MIDIs...");
	
	if(writemidis(f)!=0)
		return 17;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Cheat Codes...");
	
	if(writecheats(f,&header)!=0)
		return 18;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Init. Data...");
	
	if(writeinitdata(f,&header)!=0)
		return 19;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Custom Guy Data...");
	
	if(writeguys(f,&header)!=0)
		return 20;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Custom Hero Sprite Data...");
	
	if(writeherosprites(f,&header)!=0)
		return 21;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Custom Subscreen Data...");
	
	if(writesubscreens(f,&header)!=0)
		return 22;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing FF Script Data...");
	
	if(writeffscript(f,&header)!=0)
		return 23;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing SFX Data...");
	
	if(writesfx(f,&header)!=0)
		return 24;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Item Drop Sets...");
	
	if(writeitemdropsets(f, &header)!=0)
		return 25;
	
	box_out("okay.");
	box_eol();
	
	box_out("Writing Favorite Combos...");
	
	if(writefavorites(f, &header)!=0)
		return 26;
	
	box_out("okay.");
	box_eol();
	
	pack_fclose(f);
	
   
	if(header.use_keyfile&&header.dirty_password)
	{
		char const* kfname = filename;
		char keyfilename[2048]={0};
		zprint2("Writing key files for '%s'\n", kfname, ".zpwd", ".zcheat");
		
		char temp_pw[QSTPWD_LEN] = {0};
		uint ind = 0;
		for(char const* ext : {"key","zpwd","zcheat"})
		{
			replace_extension(keyfilename, kfname, ext, 2047);
			PACKFILE *fp = pack_fopen_password(keyfilename, F_WRITE, "");
			char msg[80] = {0};
			sprintf(msg, "ZQuest Auto-Generated Quest Password Key File.  DO NOT EDIT!");
			msg[78]=13;
			msg[79]=10;
			pfwrite(msg, 80, fp);
			p_iputw(header.zelda_version,fp);
			p_putc(header.build,fp);
			char const* pwd = header.password;
			if(ind == 2) //.zcheat, hashed pwd
			{
				char hashmap = 'Z';
				hashmap += 'Q';
				hashmap += 'U';
				hashmap += 'E';
				hashmap += 'S';
				hashmap += 'T';
				for ( int q = 0; q < QSTPWD_LEN; ++q )
				{
					temp_pw[q] = header.password[q];
					temp_pw[q] += hashmap;
				}
				pwd = temp_pw;
			}
			pfwrite(pwd, strlen(pwd), fp);
			pack_fclose(fp);
			++ind;
		}
	}

	// Move file to destination at end, to avoid issues with file being unavailable to test mode.
	std::error_code ec;
	fs::rename(tmp_filename, filename, ec);
	if (ec)
	{
		al_trace("Error saving: %s\n", std::strerror(ec.value()));
		return ec.value();
	}

	strncpy(header.zelda_version_string, getVersionString(), sizeof(header.zelda_version_string));

#ifdef __EMSCRIPTEN__
	em_sync_fs();
#endif

	return 0;
}

// #ifdef _WIN32
// static std::time_t to_time_t(FILETIME const& ft) {
//     uint64_t t = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
//     t -= 116444736000000000ull;
//     t /= 10000000u;
//     return static_cast<std::time_t>(t);
// }
// #else
// #endif
template<typename TP>
static std::time_t to_time_t(TP tp) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

std::string get_time_last_modified_string(std::string path)
{
	auto write_time = fs::last_write_time(path);
	// TODO: C++20 but not supported yet.
	// auto tt = std::chrono::clock_cast<std::chrono::system_clock>(write_time);
    std::time_t tt = to_time_t(write_time);
	std::tm *gmt = std::gmtime(&tt);
	std::stringstream buffer;
	buffer << std::put_time(gmt, "%Y-%m-%d");
	std::string formattedFileTime = buffer.str();
	return formattedFileTime;
}

int32_t save_unencoded_quest(const char *filename, bool compressed, const char *afname)
{
	// Always backup quest if it was last saved in a different version of the editor,
	// or if this a new file and is overwritting another qst file.
	if (exists(filename) && std::string(getVersionString()) != header.zelda_version_string)
	{
		std::string backup_name;
		std::string last_mod = get_time_last_modified_string(filename);
		if (strlen(header.zelda_version_string) > 0)
		{
			backup_name = fmt::format("{}-v{}", last_mod, header.zelda_version_string);
		}
		else
		{
			backup_name = fmt::format("{}", last_mod);
		}
		std::string backup_fname = fmt::format("{}-{}{}", fs::path(filename).stem().string(), backup_name, fs::path(filename).extension().string());
		fs::path backup_path = fs::path("backups") / backup_fname;
		if (!fs::exists(backup_path))
		{
			fs::create_directories(fs::path("backups"));
			if (fs::copy_file(filename, backup_path))
			{
				InfoDialog("Quest Backup", fmt::format("A backup has been saved to {}", backup_path.string())).show();
			}
			else
			{
				InfoDialog("Quest Backup", fmt::format("Failed to save backup at {}", backup_path.string())).show();
			}
		}
	}

	auto ret = _save_unencoded_quest_int(filename,compressed,afname);
	fake_pack_writing = false;
	if(ret)
	{
		box_out("-- Error saving quest file! --");
		box_end(true);
	}
	else box_end(false);
	return ret;
}

int32_t save_quest(const char *filename, bool timed_save)
{
	int32_t retention=timed_save?AutoSaveRetention:AutoBackupRetention;
	bool compress=!(timed_save&&UncompressedAutoSaves);
	char ext1[5];
	ext1[0]=0;
	
	if(timed_save)
	{
		sprintf(ext1, "qt");
	}
	else
	{
		sprintf(ext1, "qb");
	}
	
	if(retention)
	{
		char backupname[2048];
		char backupname2[2048];
		char ext[12];
		
		for(int32_t i=retention-1; i>0; --i)
		{
			sprintf(ext, "%s%d", ext1, i-1);
			replace_extension(backupname, filepath, ext, 2047);
			
			if(exists(backupname))
			{
				sprintf(ext, "%s%d", ext1, i);
				replace_extension(backupname2, filepath, ext, 2047);
				
				if(exists(backupname2))
				{
					remove(backupname2);
				}
				
				rename(backupname, backupname2);
			}
		}
		
		//don't do this if we're not saving to the same name -DD
		if(!timed_save && !strcmp(filepath, filename))
		{
			sprintf(ext, "%s%d", ext1, 0);
			replace_extension(backupname, filepath, ext, 2047);
			rename(filepath, backupname);
		}
	}
	
	int32_t ret;
	ret  = save_unencoded_quest(filename, compress, filename);

	return ret;
}

void center_zq_class_dialogs()
{
    jwin_center_dialog(pwd_dlg);
}

void zmap::prv_secrets(bool high16only)
{
    mapscr *s = &prvscr;
    mapscr *t = prvlayers;
    int32_t ft=0;
    
    for(int32_t i=0; i<176; i++)
    {
        bool putit;
        
        if(!high16only)
        {
            for(int32_t j=-1; j<6; j++)
            {
                int32_t newflag = -1;
                
                for(int32_t iter=0; iter<2; ++iter)
                {
                    putit=true;
                    
                    if(!t[j].valid)
                        continue;
                        
                    int32_t checkflag=combobuf[t[j].data[i]].flag;
                    
                    if(iter==1)
                    {
                        checkflag=t[j].sflag[i];
                    }
                    
                    switch(checkflag)
                    {
                    case mfANYFIRE:
                        ft=sBCANDLE;
                        break;
                        
                    case mfSTRONGFIRE:
                        ft=sRCANDLE;
                        break;
                        
                    case mfMAGICFIRE:
                        ft=sWANDFIRE;
                        break;
                        
                    case mfDIVINEFIRE:
                        ft=sDIVINEFIRE;
                        break;
                        
                    case mfARROW:
                        ft=sARROW;
                        break;
                        
                    case mfSARROW:
                        ft=sSARROW;
                        break;
                        
                    case mfGARROW:
                        ft=sGARROW;
                        break;
                        
                    case mfSBOMB:
                        ft=sSBOMB;
                        break;
                        
                    case mfBOMB:
                        ft=sBOMB;
                        break;
                        
                    case mfBRANG:
                        ft=sBRANG;
                        break;
                        
                    case mfMBRANG:
                        ft=sMBRANG;
                        break;
                        
                    case mfFBRANG:
                        ft=sFBRANG;
                        break;
                        
                    case mfWANDMAGIC:
                        ft=sWANDMAGIC;
                        break;
                        
                    case mfREFMAGIC:
                        ft=sREFMAGIC;
                        break;
                        
                    case mfREFFIREBALL:
                        ft=sREFFIREBALL;
                        break;
                        
                    case mfSWORD:
                        ft=sSWORD;
                        break;
                        
                    case mfWSWORD:
                        ft=sWSWORD;
                        break;
                        
                    case mfMSWORD:
                        ft=sMSWORD;
                        break;
                        
                    case mfXSWORD:
                        ft=sXSWORD;
                        break;
                        
                    case mfSWORDBEAM:
                        ft=sSWORDBEAM;
                        break;
                        
                    case mfWSWORDBEAM:
                        ft=sWSWORDBEAM;
                        break;
                        
                    case mfMSWORDBEAM:
                        ft=sMSWORDBEAM;
                        break;
                        
                    case mfXSWORDBEAM:
                        ft=sXSWORDBEAM;
                        break;
                        
                    case mfHOOKSHOT:
                        ft=sHOOKSHOT;
                        break;
                        
                    case mfWAND:
                        ft=sWAND;
                        break;
                        
                    case mfHAMMER:
                        ft=sHAMMER;
                        break;
                        
                    case mfSTRIKE:
                        ft=sSTRIKE;
                        break;
                        
                    default:
                        putit = false;
                        break;
                    }
                    
                    if(putit)
                    {
                        if(j==-1)
                        {
                            s->data[i] = s->secretcombo[ft];
                            s->cset[i] = s->secretcset[ft];
                            newflag = s->secretflag[ft];
                        }
                        else
                        {
                            t[j].data[i] = t[j].secretcombo[ft];
                            t[j].cset[i] = t[j].secretcset[ft];
                            newflag = t[j].secretflag[ft];
                        }
                    }
                }
                
                if(newflag >-1)
                {
                    ((j==-1) ? s->sflag[i] : t[j].sflag[i]) = newflag;
                }
            }
        }
        
        //if(true)
        //{
        int32_t newflag = -1;
        
        for(int32_t iter=0; iter<2; ++iter)
        {
            int32_t checkflag=combobuf[s->data[i]].flag;
            
            if(iter==1)
            {
                checkflag=s->sflag[i];
            }
            
            if((checkflag > 15)&&(checkflag < 32))
            {
                s->data[i] = s->secretcombo[(checkflag)-16+4];
                s->cset[i] = s->secretcset[(checkflag)-16+4];
                newflag = s->secretflag[(checkflag)-16+4];
                //        putit = true;
            }
        }
        
        if(newflag >-1) s->sflag[i] = newflag;
        
        for(int32_t j=0; j<6; j++)
        {
            if(!t[j].valid) continue;
            
            int32_t newflag2 = -1;
            
            for(int32_t iter=0; iter<2; ++iter)
            {
                int32_t checkflag=combobuf[t[j].data[i]].flag;
                
                if(iter==1)
                {
                    checkflag=t[j].sflag[i];
                }
                
                if((checkflag > 15)&&(checkflag < 32))
                {
                    t[j].data[i] = t[j].secretcombo[(checkflag)-16+4];
                    t[j].cset[i] = t[j].secretcset[(checkflag)-16+4];
                    newflag2 = t[j].secretflag[(checkflag)-16+4];
                }
            }
            
            if(newflag2 >-1) t[j].sflag[i] = newflag2;
        }
    }
    
    //FFCs
	word num_ffcs = s->numFFC();
    for(word i=0; i<num_ffcs; ++i)
    {
        bool putit;
        
        if(!high16only)
        {
            for(int32_t iter=0; iter<1; ++iter)
            {
                putit=true;
                int32_t checkflag=combobuf[s->ffcs[i].data].flag;
                
                if(iter==1)
                {
                    checkflag=s->sflag[i];
                }
                
                switch(checkflag)
                {
                case mfANYFIRE:
                    ft=sBCANDLE;
                    break;
                    
                case mfSTRONGFIRE:
                    ft=sRCANDLE;
                    break;
                    
                case mfMAGICFIRE:
                    ft=sWANDFIRE;
                    break;
                    
                case mfDIVINEFIRE:
                    ft=sDIVINEFIRE;
                    break;
                    
                case mfARROW:
                    ft=sARROW;
                    break;
                    
                case mfSARROW:
                    ft=sSARROW;
                    break;
                    
                case mfGARROW:
                    ft=sGARROW;
                    break;
                    
                case mfSBOMB:
                    ft=sSBOMB;
                    break;
                    
                case mfBOMB:
                    ft=sBOMB;
                    break;
                    
                case mfBRANG:
                    ft=sBRANG;
                    break;
                    
                case mfMBRANG:
                    ft=sMBRANG;
                    break;
                    
                case mfFBRANG:
                    ft=sFBRANG;
                    break;
                    
                case mfWANDMAGIC:
                    ft=sWANDMAGIC;
                    break;
                    
                case mfREFMAGIC:
                    ft=sREFMAGIC;
                    break;
                    
                case mfREFFIREBALL:
                    ft=sREFFIREBALL;
                    break;
                    
                case mfSWORD:
                    ft=sSWORD;
                    break;
                    
                case mfWSWORD:
                    ft=sWSWORD;
                    break;
                    
                case mfMSWORD:
                    ft=sMSWORD;
                    break;
                    
                case mfXSWORD:
                    ft=sXSWORD;
                    break;
                    
                case mfSWORDBEAM:
                    ft=sSWORDBEAM;
                    break;
                    
                case mfWSWORDBEAM:
                    ft=sWSWORDBEAM;
                    break;
                    
                case mfMSWORDBEAM:
                    ft=sMSWORDBEAM;
                    break;
                    
                case mfXSWORDBEAM:
                    ft=sXSWORDBEAM;
                    break;
                    
                case mfHOOKSHOT:
                    ft=sHOOKSHOT;
                    break;
                    
                case mfWAND:
                    ft=sWAND;
                    break;
                    
                case mfHAMMER:
                    ft=sHAMMER;
                    break;
                    
                case mfSTRIKE:
                    ft=sSTRIKE;
                    break;
                    
                default:
                    putit = false;
                    break;
                }
                
                if(putit)
                {
                    s->ffcs[i].data = s->secretcombo[ft];
                    s->ffcs[i].cset = s->secretcset[ft];
                }
            }
        }
        
        if(!(s->flags2&fCLEARSECRET) || high16only || s->flags4&fENEMYSCRTPERM)
        {
            for(int32_t iter=0; iter<1; ++iter)
            {
                int32_t checkflag=combobuf[s->ffcs[i].data].flag;
                
                if(iter==1)
                {
                    // FFCs can't have flags! Yet...
                }
                
                if((checkflag > 15)&&(checkflag < 32))
                {
                    s->ffcs[i].data = s->secretcombo[checkflag - 16 + 4];
                    s->ffcs[i].cset = s->secretcset[checkflag-16+4];
                }
            }
        }
    }
}
