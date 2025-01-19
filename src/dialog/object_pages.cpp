#include "object_pages.h"
#include "base/cpool.h"
#include "base/autocombo.h"
#include "base/combo.h"
#include "tiles.h"
#include "base/gui.h"
#include "gui/jwin.h"
#include "dialog/info.h"
#include "dialog/alert.h"
#include <fmt/format.h>
#include "zq/zq_class.h"
#include "zq/zq_misc.h"
#include "zq/zquest.h"

extern bool saved;
extern int32_t zq_screen_w, zq_screen_h, TilePgCursorCol, CSet;
extern uint8_t InvalidBG;

using std::pair;

int ObjectTemplate::_rect_col(uint ind) const
{
	if(get_cb(OBJPG_CB_COLMODE))
		return (ind%4)+((ind%per_page)/(4*13))*4;
	return ind%per_row;
}
int ObjectTemplate::_rect_row(uint ind) const
{
	if(get_cb(OBJPG_CB_COLMODE))
		return _rect_row_pg(ind)+(ind/per_page*row_per_page);
	return ind / per_row;
}
int ObjectTemplate::_rect_row_pg(uint ind) const
{
	if(get_cb(OBJPG_CB_COLMODE))
		return (ind % (4*13)) / 4;
	return _rect_row(ind) % row_per_page;
}
int ObjectTemplate::_rect_ind(uint col, uint row) const
{
	if(get_cb(OBJPG_CB_COLMODE))
		return ((col%4)+(col/4)*(4*13))+(((row-(row%row_per_page))*per_row)+((row%row_per_page)*4));
	return col + row*per_row;
}
void ObjectTemplate::draw_page_window()
{
	jwin_draw_win(screen, 0, 0, window_w, window_h, FR_WIN);
	jwin_draw_frame(screen, lmargin-2, tmargin-2, 32*per_row+4, 32*row_per_page+4, FR_DEEP);
	
	FONT *oldfont = font;
	font = get_zc_font(font_lfont);
	jwin_draw_titlebar(screen, 3, 3, window_w-6, 18, title.c_str(), true, true);
	font=oldfont;
}
void ObjectTemplate::draw_grid()
{
	int pgoffs = sel-(sel%per_page);
	for(int q = 0; q < per_page; ++q)
	{
		int x,y;
		x = _rect_col(q)*32;
		y = _rect_row(q)*32;
		if (q + pgoffs >= size())
			rectfill(screen, lmargin + x, tmargin + y, lmargin + x + 31, tmargin + y + 31, jwin_pal[jcBOX]);
		else do_draw(screen,lmargin+x,tmargin+y,32,32,q+pgoffs);
	}
	if(get_cb(OBJPG_CB_COLMODE))
	{
		for(int div = 1; div < 5; ++div)
		{
			int dx = lmargin+(div*32*4);
			rectfill(screen,dx-1,tmargin,dx+1,tmargin+(32*13)-1,vc(15));
		}
	}
}
void ObjectTemplate::draw_info()
{
	rectfill(screen,lmargin,tmargin+420,window_w-3,window_h-3,jwin_pal[jcBOX]);
	
	FONT* tfont = get_zc_font(font_lfont_l);
	FONT* ofont = font;
	font = tfont;
	
	string txt1, txt2;
	const int th = text_height(tfont);
	
	{ //Copy frame/current copied obj
		jwin_draw_frame(screen, cpyx-2, cpyy-2, 32+4, 32+4, FR_DEEP);
		if(copyind)
		{
			auto [c1,c2] = *copyind;
			if(get_cb(OBJPG_CB_RECTSEL))
			{
				int xs[2] = {_rect_col(c1),_rect_col(c2)};
				if(xs[1] < xs[0])
				{
					int ys[2] = {_rect_row(c1),_rect_row(c2)};
					c1 = _rect_ind(std::min(xs[0],xs[1]),std::min(ys[0],ys[1]));
					c2 = _rect_ind(std::max(xs[0],xs[1]),std::max(ys[0],ys[1]));
					copyind = {c1,c2};
				}
			}
			else if(c1 > c2)
			{
				zc_swap(c1,c2);
				copyind = {c1,c2};
			}
			do_draw(screen,cpyx,cpyy,32,32,c1);
			
			txt1 = "Copied:";
			if(c1 == c2)
				txt2 = to_string(c1);
			else txt2 = fmt::format("{} - {}",c1,c2);
		}
		else
		{
			txt1 = "No Copy";
			txt2 = "'C' to copy'";
			draw_null(screen,cpyx,cpyy,32,32);
		}
		textprintf_right_ex(screen,tfont,cpytxt_x,panelcy-th,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",txt1.c_str());
		textprintf_right_ex(screen,tfont,cpytxt_x,panelcy,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",txt2.c_str());
	}
	{ //Current selected obj
		jwin_draw_frame(screen, curx-2, cury-2, 32+4, 32+4, FR_DEEP);
		do_draw(screen,curx,cury,32,32,sel);
		
		int s1 = sel, s2 = sel;
		if(sel2)
		{
			s2 = *sel2;
			if(get_cb(OBJPG_CB_RECTSEL))
			{
				int xs[2] = {_rect_col(s1),_rect_col(s2)};
				if(xs[1] < xs[0])
				{
					int ys[2] = {_rect_row(s1),_rect_row(s2)};
					s1 = _rect_ind(std::min(xs[0],xs[1]),std::min(ys[0],ys[1]));
					s2 = _rect_ind(std::max(xs[0],xs[1]),std::max(ys[0],ys[1]));
				}
			}
			else if(s1 > s2)
				zc_swap(s1,s2);
		}
		txt1 = "Selected:";
		txt2 = sel2 ? fmt::format("{} - {}",s1,s2) : to_string(sel);
		textprintf_ex(screen,tfont,curtxt_x,panelcy-th,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",txt1.c_str());
		textprintf_ex(screen,tfont,curtxt_x,panelcy,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",txt2.c_str());
	}
	{ //Current CSet text
		txt1 = fmt::format("CSet: {}", CSet);
		textprintf_ex(screen,tfont,csettxt_x,panelcy-th/2,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",txt1.c_str());
	}
	{ //Checkbox(s)
		uint cb_x = cbox_x, cb_y = cbox_y;
		const uint wid = 30, hei = cbox_h+2;
		uint indx = 0;
		while(auto name = cb_get_name(indx++))
		{
			if(disabled_cb(indx-1))
			{
				draw_dis_checkbox(screen, cb_x, cb_y, cbox_h, cbox_h, false);
				textprintf_ex(screen, tfont, cb_x+cbox_h+2+1, cb_y+1, jwin_pal[jcLIGHT], jwin_pal[jcBOX], "%s", name->c_str());
				textprintf_ex(screen, tfont, cb_x+cbox_h+2, cb_y, jwin_pal[jcDISABLED_FG], -1, "%s", name->c_str());
			}
			else
			{
				draw_checkbox(screen, cb_x, cb_y, cbox_h, get_cb(indx-1));
				textprintf_ex(screen, tfont, cb_x+cbox_h+2, cb_y, jwin_pal[jcBOXFG], jwin_pal[jcBOX], "%s", name->c_str());
			}
			cb_y += hei;
			if(cb_y+hei >= window_h)
			{
				cb_y = cbox_y;
				cb_x += wid;
			}
		}
	}
	{ //Page num
		jwin_draw_icon(screen, pgx, panelcy-th/2-8, jwin_pal[jcBOXFG], BTNICON_ARROW_UP, 6, true);
		textprintf_centre_ex(screen, tfont, pgx, panelcy-th/2, jwin_pal[jcBOXFG],jwin_pal[jcBOX],"p: %d", sel/per_page);
		jwin_draw_icon(screen, pgx, panelcy+th/2+12, jwin_pal[jcBOXFG], BTNICON_ARROW_DOWN, 6, true);
	}
	{ //Buttons
		uint btnx = btn1x;
		for(auto it = buttons.begin(); it != buttons.end(); ++it)
		{
			jwin_draw_text_button(screen, btnx, btny, btnwid, btnhei, it->first.c_str(), 0, true);
			btnx -= btnwid;
		}
	}
	font = ofont;
}
void ObjectTemplate::init(optional<int> start_val)
{
	if(start_val)
		sel = *start_val;
	sel = bound(sel);
	sel2.reset();
	title = fmt::format("Select {}",name());
	for(uint q = 0; q < OBJPG_CB_TOTAL; ++q)
		if(auto name = cb_get_cfg(q))
		{
			int dv = 0;
			if(auto d = cb_get_default(q))
				dv = *d ? 1 : 0;
			cb[q] = zc_get_config("misc",name->c_str(),dv);
		}
	copyind.reset();
	clk = 0;
	buttons.clear();
}
void ObjectTemplate::gui_redraw(bool force_cursor)
{
	draw_page_window();
	draw_grid();
	if(force_cursor || (clk&8))
	{
		if(sel2)
		{
			int s1 = sel, s2 = *sel2;
			int pg = sel/per_page;
			if(get_cb(OBJPG_CB_RECTSEL))
			{
				int x1 = _rect_col(s1);
				int x2 = _rect_col(s2);
				int y1 = _rect_row(s1);
				int y2 = _rect_row(s2);
				if(x1 > x2)
					zc_swap(x1,x2);
				if(y1 > y2)
					zc_swap(y1,y2);
				if(y1 < pg*row_per_page)
					y1 = pg*row_per_page;
				if(y2 >= (pg+1)*row_per_page)
					y2 = (pg+1)*row_per_page-1;
				for_area(_rect_ind(x1,y1),_rect_ind(x2,y2),[&](int s){
					int x = lmargin+_rect_col(s)*32;
					int y = tmargin+(_rect_row_pg(s)%row_per_page)*32;
					safe_rect(screen, x, y, x+31, y+31, vc(TilePgCursorCol), 2);
				});
			}
			else
			{
				if(s1 > s2)
					zc_swap(s1,s2);
				if(s1 < pg*per_page)
					s1 = pg*per_page;
				if(s2 >= (pg+1)*per_page)
					s2 = (pg+1)*per_page-1;
				for_area(s1,s2,[&](int s){
					int x = lmargin+_rect_col(s)*32;
					int y = tmargin+(_rect_row_pg(s)%row_per_page)*32;
					safe_rect(screen, x, y, x+31, y+31, vc(TilePgCursorCol), 2);
				});
			}
		}
		else
		{
			int x = lmargin+_rect_col(sel)*32;
			int y = tmargin+(_rect_row_pg(sel)%row_per_page)*32;
			safe_rect(screen, x, y, x+31, y+31, vc(TilePgCursorCol), 2);
		}
	}
	draw_info();
}
void ObjectTemplate::call_dlg(optional<int> start_val)
{
	reset_combo_animations();
	reset_combo_animations2();
	position_mouse_z(0);
	clear_backup(); //undo stack, only a single undo deep, no redo
	int cache_cs = CSet;
	
	popup_zqdialog_start((zq_screen_w-window_w)/2,(zq_screen_h-window_h)/2,window_w,window_h);
	
	init(start_val);
	postinit();
	if(buttons.empty())
		buttons.emplace_back("Done", [&](){return 1;});
	
	draw_page_window();
	draw_grid();
	draw_info();
	update_hw_screen();
	
	while(gui_mouse_b())
		rest(1);
	int done = 0;
	bool bdown = false;
	
	FONT* tfont = get_zc_font(font_lfont_l);
	
	do
	{
		HANDLE_CLOSE_ZQDLG();
		if(exiting_program) break;
		bool redraw = false, resel = false;
		auto osel = sel;
		auto osel2 = sel2;
		if(mouse_z)
		{
			if(mouse_z > 0)
				sel -= per_page;
			else sel += per_page;
			position_mouse_z(0);
			resel = true;
			redraw = true;
		}
		if(keypressed())
		{
			auto k = readkey()>>8;
			switch(k)
			{
				case KEY_ENTER_PAD:
				case KEY_ENTER:
					done = 2;
					break;
				case KEY_ESC:
					done = 1;
					break;
				case KEY_EQUALS:
				case KEY_PLUS_PAD:
					CSet = WRAP_CS(CSet+1);
					redraw = true;
					break;
				case KEY_MINUS:
				case KEY_MINUS_PAD:
					CSet = WRAP_CS(CSet-1);
					redraw = true;
					break;
				case KEY_UP:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(_rect_col(sel),_rect_row(sel)-_rect_row_pg(sel));
					else if(get_cb(OBJPG_CB_COLMODE))
						sel -= 4;
					else sel -= per_row;
					resel = true;
					break;
				case KEY_DOWN:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(_rect_col(sel),_rect_row(sel)-_rect_row_pg(sel)+row_per_page-1);
					else if(get_cb(OBJPG_CB_COLMODE))
						sel += 4;
					else sel += per_row;
					resel = true;
					break;
				case KEY_LEFT:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(0,_rect_row(sel));
					else --sel;
					resel = true;
					break;
				case KEY_RIGHT:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(per_row-1,_rect_row(sel));
					else ++sel;
					resel = true;
					break;
				case KEY_PGUP:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(_rect_col(sel),0);
					else sel -= per_page;
					resel = true;
					break;
				case KEY_PGDN:
					if(CHECK_CTRL_CMD)
						sel = _rect_ind(_rect_col(sel),_rect_row(size()-1));
					else sel += per_page;
					resel = true;
					break;
				case KEY_HOME:
					if(CHECK_CTRL_CMD)
						sel = 0;
					else sel = sel%per_page;
					resel = true;
					break;
				case KEY_END:
					if(CHECK_CTRL_CMD)
						sel = size()-1;
					else sel = size()-1-(size()-1%per_page)+(sel%per_page);
					resel = true;
					break;
				case KEY_P:
					if(optional<int> pg = call_get_num("Goto Page",sel/per_page,(size()-1)/per_page,0))
					{
						sel = (sel%per_page) + (*pg * per_page);
						sel2.reset();
					}
					break;
				case KEY_E:
					try_edit();
					redraw = true;
					break;
				case KEY_C:
					try_copy();
					redraw = true;
					break;
				case KEY_SPACE:
					toggle_cb(OBJPG_CB_RECTSEL);
					redraw = true;
					copyind.reset();
					break;
				case KEY_V:
					if(CHECK_CTRL_CMD)
					{
						if(try_adv_paste())
							redraw = true;
					}
					else if(try_paste())
						redraw = true;
					break;
				case KEY_S:
					if(try_swap())
						redraw = true;
					break;
				case KEY_I:
					if(CHECK_SHIFT)
					{
						if(try_remove())
							redraw = true;
					}
					else if(try_insert())
						redraw = true;
					break;
				case KEY_DEL:
					if(try_delete())
						redraw = true;
					break;
				case KEY_U:
					restore_backup();
					break;
				case KEY_F1:
					show_help();
					break;
				case KEY_SLASH:
					if(CHECK_CTRL_CMD || CHECK_SHIFT)
						show_help();
					break;
				//!TODO Insert/Remove? Move/Swap?
			}
			clear_keybuf();
		}
		int mx = gui_mouse_x(), my = gui_mouse_y();
		int mx_ind = (mx-lmargin)/32, my_ind = (my-tmargin)/32;
		optional<int> hovered;
		if(mx_ind >= 0 && mx_ind < per_row && my_ind >= 0 && my_ind < row_per_page)
		{
			hovered = _rect_ind(mx_ind,my_ind)+(sel-sel%per_page);
			if(*hovered >= size())
				hovered.reset();
		}
		bool lclick = gui_mouse_b()&1, rclick = gui_mouse_b()&2;
		bool click = lclick || rclick;
		if(click)
		{
			if(hovered)
			{
				sel = *hovered;
				resel = redraw = true;
			}
		}
		if(resel)
		{
			if(!CHECK_SHIFT)
				sel2.reset();
			else if(!sel2)
				sel2 = bound(osel);
			resel = false;
		}
		sel = bound(sel);
		if(!bdown)
		{
			if(click)
			{
				FONT* ofont = font;
				font = tfont;
				
				uint btnx = btn1x;
				for(auto it = buttons.begin(); it != buttons.end(); ++it)
				{
					if(lclick && isinRect(mx, my, btnx, btny, btnx+btnwid-1, btny+btnhei-1))
						if(do_text_button(btnx, btny, btnwid, btnhei, it->first.c_str()))
							done = it->second();
					btnx -= btnwid;
				}
				font = ofont;
				
				if(isinRect(mx, my, window_w-4-18, 5, window_w-6, 5+13))
					if(do_x_button(screen, window_w-4-18, 5))
						done=1;
				if(isinRect(mx, my, window_w-4-18-18, 5, window_w-6-18, 5+13))
					if(do_question_button(screen, window_w-4-18-18, 5))
						show_help();
				
				{ //Checkboxes
					uint cb_x = cbox_x, cb_y = cbox_y;
					const uint wid = 30, hei = cbox_h+2;
					uint indx = 0;
					bool _selrect = get_cb(OBJPG_CB_RECTSEL), _selcols = get_cb(OBJPG_CB_COLMODE);
					while(auto name = cb_get_name(indx++))
					{
						if(!disabled_cb(indx-1))
						{
							int state = cb[indx-1];
							auto twid = text_length(tfont,name->c_str())+2;
							if(isinRect(mx, my, cb_x, cb_y, cb_x+cbox_h-1+twid, cb_y+cbox_h-1))
							{
								if(lclick)
								{
									if(do_checkbox_tx(screen, cb_x, cb_y, cbox_h, cbox_h, state, twid))
										toggle_cb(indx-1);
								}
								else if(rclick)
									cb_do_rclick(indx-1);
								break;
							}
						}
						
						cb_y += hei;
						if(cb_y+hei >= window_h)
						{
							cb_y = cbox_y;
							cb_x += wid;
						}
					}
					if(_selrect != get_cb(OBJPG_CB_RECTSEL) || _selcols != get_cb(OBJPG_CB_COLMODE))
						copyind.reset();
				}
				
				if(rclick && hovered)
				{
					gui_redraw(true);
					if(do_rclick(*hovered))
						redraw = true;
				}
				redraw = true;
			}
		}
		bdown = click;
		sel = bound(sel);
		
		if(!(clk%8) || InvalidBG == 1)
			redraw = true;
		if(sel != osel || sel2 != osel2)
			redraw = true;
		if(do_tick())
			redraw = true;
		
		if(redraw)
			gui_redraw();
		
		++clk;;
		update_hw_screen();
	}
	while(!done);
	
	popup_zqdialog_end();
	clear_backup();
	
	CSet = cache_cs;
}

void ObjectTemplate::show_help() const
{
	InfoDialog(fmt::format("{} Pages Hotkeys",name()),
		fmt::format("Enter: Close dialog (confirm)"
		"\nEsc: Close dialog (cancel)"
		"\n+ / -: change CSet"
		"\nArrows: Move cursor"
		"\nCtrl+Arrows: Move cursor to edge"
		"\nPgUp/PgDn: Move cursor up/down 1 page"
		"\nCtrl+(PgUp/PgDn): Move cursor to start/end"
		"\nHome/End: Move cursor to start/end of current page"
		"\nCtrl+(Home/End): Move cursor to start/end"
		"\nHold 'Shift' when you 'Move cursor' to select an area"
		"\nP: GoTo Page"
		"\nE: Edit"
		"\nC: Copy"
		"\nV: Paste (if something copied)"
		"\nCtrl+V: Paste (without clearing clipboard)"
		"\nShift+V: Paste (to full area)"
		"\nDelete: Delete selected {}(s)"
		"\nU: Undo (or redo, if the last change was an undo)"
		"\nF1 or Ctrl+/ or Shift+/: show this help text"
		"{}"
		,name(),custom_info())).show();
}
void ObjectTemplate::try_copy()
{
	copyind = { sel, sel2 ? *sel2 : sel };
}
bool ObjectTemplate::try_paste()
{
	if(copyind)
	{
		clear_backup();
		auto [c1,c2] = *copyind;
		if(CHECK_SHIFT)
		{
			for_area(sel,sel2,[&](int s){
				backup(s);
				do_copy(s,copyind->first);
			});
		}
		else
		{
			for_area(c1,c2,sel,[&](int d, int s){
				backup(d);
				do_copy(d,s);
			});
		}
		saved = false;
		copyind.reset();
		return true;
	}
	return false;
}
bool ObjectTemplate::try_swap()
{
	if(copyind)
	{
		clear_backup();
		auto [c1,c2] = *copyind;
		for_area(c1,c2,sel,[&](int d, int s){
			backup(d);
			backup(s);
		});
		for_area(c1,c2,sel,[&](int d, int s){
			do_swap(d,s);
		});
		saved = false;
		copyind.reset();
		return true;
	}
	return false;
}
bool ObjectTemplate::try_adv_paste()
{
	return false;
}
void ObjectTemplate::try_edit()
{
	clear_backup();
	backup(sel);
	do_edit(sel);
}
bool ObjectTemplate::try_delete()
{
	string toclear = fmt::format("Are you sure you want to clear {}{}?{}",
		name(), sel2 ? fmt::format("s {} - {}", std::min(sel, *sel2),
			std::max(sel, *sel2)) : fmt::format(" {}", sel),
			get_cb(OBJPG_CB_RECTSEL) ? (get_cb(OBJPG_CB_COLMODE) ?
				" (Rectangularly, 4-column)" : " (Rectangularly)") : "");
	bool did = false;
	AlertDialog(fmt::format("Delete {}s", name()),
		toclear,
		[&](bool ret,bool)
		{
			if(ret)
				did = true;
		}).show();
	if(did)
	{
		clear_backup();
		for_area(sel,sel2,[&](int s){
			backup(s);
			do_delete(s);
		});
		
		saved = false;
		return true;
	}
	return false;
}
bool ObjectTemplate::try_insert()
{
	string toclear = sel2 ? fmt::format("Are you sure you want to insert {} {}s?", abs(*sel2-sel), name())
		: fmt::format("Are you sure you want to insert 1 {}?", name());;
	bool did = false;
	AlertDialog(fmt::format("Insert {}s", name()),
		toclear,
		[&](bool ret,bool)
		{
			if(ret)
				did = true;
		}).show();
	if(did)
	{
		//!TODO INSERT CODE
		// clear_backup();
		// for_area(sel,sel2,[&](int s){
			// backup(s);
			// do_delete(s);
		// });
		
		saved = false;
		return true;
	}
	return false;
}
bool ObjectTemplate::try_remove()
{
	string toclear = sel2 ? fmt::format("Are you sure you want to remove {} {}s?", abs(*sel2-sel), name())
		: fmt::format("Are you sure you want to remove 1 {}?", name());;
	bool did = false;
	AlertDialog(fmt::format("Remove {}s", name()),
		toclear,
		[&](bool ret,bool)
		{
			if(ret)
				did = true;
		}).show();
	if(did)
	{
		//!TODO REMOVE CODE
		// clear_backup();
		// for_area(sel,sel2,[&](int s){
			// backup(s);
			// do_delete(s);
		// });
		
		saved = false;
		return true;
	}
	return false;
}

bool ObjectTemplate::get_cb(uint indx) const
{
	if(indx >= OBJPG_CB_TOTAL)
		return false;
	return cb[indx];
}
void ObjectTemplate::set_cb(uint indx, bool val)
{
	if(indx >= OBJPG_CB_TOTAL)
		return;
	cb[indx] = val ? 1 : 0;
	write_cb(indx);
}
void ObjectTemplate::toggle_cb(uint indx)
{
	set_cb(indx,!cb[indx]);
}
void ObjectTemplate::write_cb(uint indx)
{
	if(indx >= OBJPG_CB_TOTAL)
		return;
	if(auto name = cb_get_cfg(indx))
		zc_set_config("misc",name->c_str(),cb[indx]);
}

void ObjectTemplate::for_area(int s1, optional<int> opts2, std::function<void(int)> callback)
{
	if(!opts2 || s1 == *opts2)
		callback(s1);
	else
	{
		int s2 = *opts2;
		if(get_cb(OBJPG_CB_RECTSEL))
		{
			if(get_cb(OBJPG_CB_COLMODE))
			{
				int lx = std::min(_rect_col(s1),_rect_col(s2));
				int rx = std::max(_rect_col(s1),_rect_col(s2));
				int ty = std::min(_rect_row(s1),_rect_row(s2));
				int by = std::max(_rect_row(s1),_rect_row(s2));
				for(int y = ty; y <= by; ++y)
					for(int x = lx; x <= rx; ++x)
						callback(_rect_ind(x,y));
			}
			else
			{
				int lx = std::min(_rect_col(s1),_rect_col(s2));
				int rx = std::max(_rect_col(s1),_rect_col(s2));
				int ty = std::min(_rect_row(s1),_rect_row(s2));
				int by = std::max(_rect_row(s1),_rect_row(s2));
				for(int y = ty; y <= by; ++y)
					for(int x = lx; x <= rx; ++x)
						callback(_rect_ind(x,y));
			}
		}
		else
		{
			if(s1 > s2) zc_swap(s1,s2);
			for(int q = s1; q <= s2; ++q)
				callback(q);
		}
	}
}
void ObjectTemplate::for_area(int s1, optional<int> opts2, int dest, std::function<void(int,int)> callback)
{
	if(!opts2 || s1 == *opts2)
		callback(dest,s1);
	else
	{
		int s2 = *opts2;
		if(get_cb(OBJPG_CB_RECTSEL))
		{
			int lx = std::min(_rect_col(s1),_rect_col(s2));
			int rx = std::max(_rect_col(s1),_rect_col(s2));
			int ty = std::min(_rect_row(s1),_rect_row(s2));
			int by = std::max(_rect_row(s1),_rect_row(s2));
			int wid = rx-lx+1, hei = by-ty+1;
			int dx = _rect_col(dest), dy = _rect_row(dest);
			if(_rect_ind(lx,ty) < dest)
				for(int y = hei-1; y >= 0; --y)
					for(int x = wid-1; x >= 0; --x)
						callback(_rect_ind(x+dx,y+dy),_rect_ind(x+lx,y+ty));
			else
				for(int y = 0; y < hei; ++y)
					for(int x = 0; x < wid; ++x)
						callback(_rect_ind(x+dx,y+dy),_rect_ind(x+lx,y+ty));
		}
		else
		{
			if(s1 > s2) zc_swap(s1,s2);
			int wid = s2-s1;
			if(s1 < dest)
				for(int q = wid; q >= 0; --q)
					callback(dest+q,s1+q);
			else
				for(int q = 0; q <= wid; ++q)
					callback(dest+q,s1+q);
		}
	}
}
void ObjectTemplate::draw_null(BITMAP* dest, int x, int y, int w, int h) const
{
	switch(InvalidBG)
	{
		case 2:
		{
			auto sz = std::max(w,h);
			draw_checkerboard(dest, x, y, sz);
			break;
		}
		case 1:
			for(int dy = 0; dy < h; ++dy)
				for(int dx = 0; dx < w; ++dx)
					dest->line[dy+y][dx+x]=vc((((zc_oldrand()%100)/50)?0:8)+(((zc_oldrand()%100)/50)?0:7));
			break;
		default:
			for(int dy = 0; dy < h; ++dy)
				for(int dx = 0; dx < w; ++dx)
					dest->line[dy+y][dx+x]=vc(0);
			rect(dest, x+1,y+1, x+w-1, y+h-1, vc(15));
			line(dest, x+1,y+1, x+w-1, y+h-1, vc(15));
			line(dest, x+1,y+h-1, x+w-1, y+1,  vc(15));
			break;
	}
}

bool ObjectTemplate::disabled_cb(uint indx) const
{
	switch(indx)
	{
		case OBJPG_CB_RECTSEL:
			return false;
		case OBJPG_CB_COLMODE:
			return false;
	}
	return false;
}
optional<string> ObjectTemplate::cb_get_name(uint indx) const
{
	switch(indx)
	{
		case OBJPG_CB_RECTSEL:
			return "Rect Select";
		case OBJPG_CB_COLMODE:
			return "4-Columns";
	}
	return nullopt;
}
optional<string> ObjectTemplate::cb_get_cfg(uint indx) const
{
	switch(indx)
	{
		case OBJPG_CB_RECTSEL:
			return fmt::format("rect_sel_{}_pages",cfgname());
		case OBJPG_CB_COLMODE:
			return fmt::format("column_mode_{}_pages",cfgname());
	}
	return nullopt;
}
optional<bool> ObjectTemplate::cb_get_default(uint indx) const
{
	switch(indx)
	{
		case OBJPG_CB_RECTSEL:
			return true;
		case OBJPG_CB_COLMODE:
			return true;
	}
	return nullopt;
}
void ObjectTemplate::cb_do_rclick(uint indx)
{
	switch(indx)
	{
		case OBJPG_CB_RECTSEL:
			break;
		case OBJPG_CB_COLMODE:
			break;
	}
}

//
// Combo Pages
//
int32_t onComboLocationReport();

ComboPageObj ComboPageObj::inst;
bool ComboPageObj::try_adv_paste()
{
	if(!copyind) return false;
	static bitstring pasteflags;
	static const vector<CheckListInfo> advp_names =
	{
		{ "Tile" },
		{ "CSet2" },
		{ "Solidity" },
		{ "Animation" },
		{ "Type" },
		{ "Inherent Flag" },
		{ "Attribytes" },
		{ "Attrishorts" },
		{ "Attributes" },
		{ "Flags", "The 16 Flags on the 'Flags' tab" },
		{ "Gen. Flags", "The 2 'General Flags' on the 'Flags' tab" },
		{ "Label" },
		{ "Script" },
		{ "Effect" },
		{ "Triggers Tab" },
		{ "Lifting Tab" },
		{ "Gen: Movespeed", "The Movespeed related values from the 'General' tab" },
		{ "Gen: SFX", "The SFX related values from the 'General' tab" },
		{ "Gen: Sprites", "The Sprites related values from the 'General' tab" },
	};
	if(!call_checklist_dialog("Advanced Paste",advp_names,pasteflags))
		return false;
	clear_backup();
	auto [c1,c2] = *copyind;
	if(CHECK_SHIFT)
	{
		for_area(sel,sel2,[&](int s){
			backup(s);
			do_adv_paste(s,copyind->first,pasteflags);
		});
	}
	else
	{
		for_area(c1,c2,sel,[&](int d, int s){
			backup(d);
			do_adv_paste(d,s,pasteflags);
		});
	}
	saved = false;
	if(pasteflags.get(CMB_ADVP_TILE) || pasteflags.get(CMB_ADVP_ANIM)) //reset animations if needed
	{
		setup_combo_animations();
		setup_combo_animations2();
	}
	return true;
}
void ComboPageObj::do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const
{
	newcombo const& cmb = combobuf[index];
	if(cmb.is_blank())
	{
		draw_null(dest,x,y,w,h);
		return;
	}
	int32_t cid = index; int8_t cs = CSet;
	if(w == h && !(w%16))
		put_combo(dest,x,y,cid,cs,0,0,w/16);
	else
	{
		BITMAP* tmpbmp = create_bitmap_ex(8,16,16);
		put_combo(tmpbmp,0,0,cid,cs,0,0,1);
		stretch_blit(tmpbmp, dest, 0, 0, 16, 16, x, y, w, h);
		destroy_bitmap(tmpbmp);
	}
}
void ComboPageObj::do_copy(int dest, int src) const
{
	combobuf[dest] = combobuf[src];
}
void ComboPageObj::do_swap(int dest, int src) const
{
	zc_swap(combobuf[dest], combobuf[src]);
}
void ComboPageObj::do_adv_paste(int dest, int src, bitstring const& flags) const
{
	combobuf[dest].advpaste(combobuf[src], flags);
}
bool call_combo_editor(int32_t index);
void ComboPageObj::do_edit(int index)
{
	call_combo_editor(index);
}
void ComboPageObj::do_delete(int index)
{
	combobuf[index].clear();
}
size_t ComboPageObj::size() const
{
	return MAXCOMBOS;
}

bool ComboPageObj::do_rclick(int indx)
{
	bool ret = false;
	NewMenu rcmenu
	{
		{ "&Copy", [&](){try_copy();} },
		{ "Paste", "&v", [&](){ret = try_paste();}, nullopt, !copyind },
		{ "&Adv. Paste", [&](){ret = try_adv_paste();}, nullopt, !copyind },
		{ "&Swap", [&](){ret = try_swap();}, nullopt, !copyind},
		{ "&Delete", [&](){ret = try_delete();} },
		{ "" },
		{ "&Edit", [&](){try_edit(); ret = true;} },
		{ "&Insert", [&](){ret = try_insert();} },
		{ "&Remove", "Shift+I", [&](){ret = try_remove();} },
		{ "" },
		{ "&Locations", [&](){
			auto tmp = Combo;
			Combo = indx;
			onComboLocationReport();
			Combo = tmp;
		} },
	};
	rcmenu.pop(window_mouse_x(),window_mouse_y());
	return ret;
}
bool ComboPageObj::do_tick()
{
	if(get_cb(CMBPG_CB_ANIM))
	{
		animate_combos();
		return true;
	}
	else
	{
		reset_combo_animations();
		reset_combo_animations2();
	}
	return false;
}
void ComboPageObj::postinit()
{
	buttons.emplace_back("Done", [&](){return 1;});
	buttons.emplace_back("&Edit", [&](){try_edit(); return 0;});
}

bool ComboPageObj::disabled_cb(uint indx) const
{
	switch(indx)
	{
		case CMBPG_CB_ANIM:
			return false;
	}
	return ObjectTemplate::disabled_cb(indx);
}
optional<string> ComboPageObj::cb_get_name(uint indx) const
{
	switch(indx)
	{
		case CMBPG_CB_ANIM:
			return "Animate";
	}
	return ObjectTemplate::cb_get_name(indx);
}
optional<string> ComboPageObj::cb_get_cfg(uint indx) const
{
	switch(indx)
	{
		case CMBPG_CB_ANIM:
			return "combopage_animate";
	}
	return ObjectTemplate::cb_get_cfg(indx);
}
optional<bool> ComboPageObj::cb_get_default(uint indx) const
{
	switch(indx)
	{
		case CMBPG_CB_ANIM:
			return true;
	}
	return ObjectTemplate::cb_get_default(indx);
}

static map<int,newcombo> backup_cmb;
void ComboPageObj::clear_backup() const
{
	backup_cmb.clear();
}
void ComboPageObj::backup(int index) const
{
	backup_cmb[index] = combobuf[index];
}
void ComboPageObj::restore_backup() const
{
	if(backup_cmb.empty())
		return;
	saved = false;
	map<int,newcombo> tmp;
	for(auto [ind,cmb] : backup_cmb)
	{
		tmp[ind] = combobuf[ind];
		combobuf[ind] = cmb;
	}
	backup_cmb = tmp; //undo again to redo
}

//
// Combo Pool Pages
//
ComboPoolPageObj ComboPoolPageObj::inst;
void ComboPoolPageObj::do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const
{
	combo_pool const& cpool = combo_pools[index];
	if(!cpool.valid())
	{
		draw_null(dest,x,y,w,h);
		return;
	}
	int32_t cid; int8_t cs = CSet;
	int previndx = get_cb(CPOOLPG_CB_CYCLE) ? clk/pool_cycle_rate : 0;
	cpool.get_w_wrap(cid,cs,previndx);
	if(w == h && !(w%16))
		put_combo(dest,x,y,cid,cs,0,0,w/16);
	else
	{
		BITMAP* tmpbmp = create_bitmap_ex(8,16,16);
		put_combo(tmpbmp,0,0,cid,cs,0,0,1);
		stretch_blit(tmpbmp, dest, 0, 0, 16, 16, x, y, w, h);
		destroy_bitmap(tmpbmp);
	}
}
void ComboPoolPageObj::do_copy(int dest, int src) const
{
	combo_pools[dest] = combo_pools[src];
}
void ComboPoolPageObj::do_swap(int dest, int src) const
{
	zc_swap(combo_pools[dest], combo_pools[src]);
}
void call_cpool_dlg(int32_t index);
void ComboPoolPageObj::do_edit(int index)
{
	call_cpool_dlg(index);
}
void ComboPoolPageObj::do_delete(int index)
{
	combo_pools[index].clear();
}
size_t ComboPoolPageObj::size() const
{
	return MAXCOMBOPOOLS;
}

bool ComboPoolPageObj::do_rclick(int indx)
{
	bool ret = false;
	NewMenu rcmenu
	{
		{ "&Copy", [&](){try_copy();} },
		{ "Paste", "&v", [&](){ret = try_paste();}, nullopt, !copyind },
		{ "&Swap", [&](){ret = try_swap();}, !copyind},
		{ "&Edit", [&](){try_edit(); ret = true;} },
		{ "&Delete", [&](){ret = try_delete();} },
	};
	rcmenu.pop(window_mouse_x(),window_mouse_y());
	return ret;
}
bool ComboPoolPageObj::do_tick()
{
	return get_cb(CPOOLPG_CB_CYCLE) && !(clk%pool_cycle_rate);
}
void ComboPoolPageObj::postinit()
{
	pool_cycle_rate = std::max(1,zc_get_config("misc","cpool_cycle_rate",16));
	buttons.emplace_back("Done", [&](){return 1;});
	buttons.emplace_back("&Edit", [&](){try_edit(); return 0;});
}

bool ComboPoolPageObj::disabled_cb(uint indx) const
{
	switch(indx)
	{
		case CPOOLPG_CB_CYCLE:
			return false;
	}
	return ObjectTemplate::disabled_cb(indx);
}
optional<string> ComboPoolPageObj::cb_get_name(uint indx) const
{
	switch(indx)
	{
		case CPOOLPG_CB_CYCLE:
			return "Cycle Pools";
	}
	return ObjectTemplate::cb_get_name(indx);
}
optional<string> ComboPoolPageObj::cb_get_cfg(uint indx) const
{
	switch(indx)
	{
		case CPOOLPG_CB_CYCLE:
			return "pool_pages_cycle";
	}
	return ObjectTemplate::cb_get_cfg(indx);
}
optional<bool> ComboPoolPageObj::cb_get_default(uint indx) const
{
	switch(indx)
	{
		case CPOOLPG_CB_CYCLE:
			return false;
	}
	return ObjectTemplate::cb_get_default(indx);
}
void ComboPoolPageObj::cb_do_rclick(uint indx)
{
	switch(indx)
	{
		case CPOOLPG_CB_CYCLE:
		{
			if(auto v = call_get_num("Pool Cycle Rate","The rate that the"
				" combo pool previews cycle. Higher value is slower.",
				pool_cycle_rate, 9999, 1))
			{
				pool_cycle_rate = *v;
				zc_set_config("misc","cpool_cycle_rate",(int)pool_cycle_rate);
			}
			break;
		}
		default:
			ObjectTemplate::cb_do_rclick(indx);
			break;
	}
}

static map<int,combo_pool> backup_cpool;
void ComboPoolPageObj::clear_backup() const
{
	backup_cpool.clear();
}
void ComboPoolPageObj::backup(int index) const
{
	backup_cpool[index] = combo_pools[index];
}
void ComboPoolPageObj::restore_backup() const
{
	if(backup_cpool.empty())
		return;
	saved = false;
	map<int,combo_pool> tmp;
	for(auto [ind,cpool] : backup_cpool)
	{
		tmp[ind] = combo_pools[ind];
		combo_pools[ind] = cpool;
	}
	backup_cpool = tmp; //undo again to redo
}

//
// Auto ComboPages
//
AutoComboPageObj AutoComboPageObj::inst;
void AutoComboPageObj::do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const
{
	combo_auto const& autoc = combo_autos[index];
	if(!autoc.valid())
	{
		draw_null(dest,x,y,w,h);
		return;
	}
	int32_t cid = autoc.getDisplay();
	int8_t cs = CSet;
	if(w == h && !(w%16))
		put_combo(dest,x,y,cid,cs,0,0,w/16);
	else
	{
		BITMAP* tmpbmp = create_bitmap_ex(8,16,16);
		put_combo(tmpbmp,0,0,cid,cs,0,0,1);
		stretch_blit(tmpbmp, dest, 0, 0, 16, 16, x, y, w, h);
		destroy_bitmap(tmpbmp);
	}
}
void AutoComboPageObj::do_copy(int dest, int src) const
{
	combo_autos[dest] = combo_autos[src];
}
void AutoComboPageObj::do_swap(int dest, int src) const
{
	zc_swap(combo_autos[dest], combo_autos[src]);
}
void call_autocombo_dlg(int32_t index);
void AutoComboPageObj::do_edit(int index)
{
	call_autocombo_dlg(index);
}
void AutoComboPageObj::do_delete(int index)
{
	combo_autos[index].clear(true);
}
size_t AutoComboPageObj::size() const
{
	return MAXAUTOCOMBOS;
}

bool AutoComboPageObj::do_rclick(int indx)
{
	bool ret = false;
	NewMenu rcmenu
	{
		{ "&Copy", [&](){try_copy();} },
		{ "Paste", "&v", [&](){ret = try_paste();}, nullopt, !copyind },
		{ "&Swap", [&](){ret = try_swap();}, !copyind},
		{ "&Edit", [&](){try_edit(); ret = true;} },
		{ "&Delete", [&](){ret = try_delete();} },
	};
	rcmenu.pop(window_mouse_x(),window_mouse_y());
	return ret;
}
bool AutoComboPageObj::do_tick()
{
	return false;
}
void AutoComboPageObj::postinit()
{
	buttons.emplace_back("Done", [&](){return 1;});
	buttons.emplace_back("&Edit", [&](){try_edit(); return 0;});
}

static map<int,combo_auto> backup_autoc;
void AutoComboPageObj::clear_backup() const
{
	backup_autoc.clear();
}
void AutoComboPageObj::backup(int index) const
{
	backup_autoc[index] = combo_autos[index];
}
void AutoComboPageObj::restore_backup() const
{
	if(backup_autoc.empty())
		return;
	saved = false;
	map<int,combo_auto> tmp;
	for(auto [ind,autoc] : backup_autoc)
	{
		tmp[ind] = combo_autos[ind];
		combo_autos[ind] = autoc;
	}
	backup_autoc = tmp; //undo again to redo
}

//
// Combo Alias Pages
//
AliasPageObj AliasPageObj::inst;
void draw_combo_alias_thumbnail(BITMAP *dest, combo_alias const* combo, int32_t x, int32_t y, int32_t size);
void AliasPageObj::do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const
{
	combo_alias const& alias = combo_aliases[index];
	if(!alias.valid())
	{
		draw_null(dest,x,y,w,h);
		return;
	}
	if(w == h && !(w%16))
		draw_combo_alias_thumbnail(dest,&alias,x,y,w/16);
	else
	{
		BITMAP* tmpbmp = create_bitmap_ex(8,16,16);
		draw_combo_alias_thumbnail(tmpbmp,&alias,0,0,1);
		stretch_blit(tmpbmp, dest, 0, 0, 16, 16, x, y, w, h);
		destroy_bitmap(tmpbmp);
	}
}
void AliasPageObj::do_copy(int dest, int src) const
{
	combo_aliases[dest] = combo_aliases[src];
}
void AliasPageObj::do_swap(int dest, int src) const
{
	zc_swap(combo_aliases[dest], combo_aliases[src]);
}
void call_calias_dlg(int index);
void AliasPageObj::do_edit(int index)
{
	call_calias_dlg(index);
}
void AliasPageObj::do_delete(int index)
{
	combo_aliases[index].clear();
}
size_t AliasPageObj::size() const
{
	return MAXCOMBOALIASES;
}

bool AliasPageObj::do_rclick(int indx)
{
	bool ret = false;
	NewMenu rcmenu
	{
		{ "&Copy", [&](){try_copy();} },
		{ "Paste", "&v", [&](){ret = try_paste();}, nullopt, !copyind },
		{ "&Swap", [&](){ret = try_swap();}, !copyind},
		{ "&Edit", [&](){try_edit(); ret = true;} },
		{ "&Delete", [&](){ret = try_delete();} },
	};
	rcmenu.pop(window_mouse_x(),window_mouse_y());
	return ret;
}
bool AliasPageObj::do_tick()
{
	return false;
}
void AliasPageObj::postinit()
{
	buttons.emplace_back("Done", [&](){return 1;});
	buttons.emplace_back("&Edit", [&](){try_edit(); return 0;});
}

static map<int,combo_alias> backup_alias;
void AliasPageObj::clear_backup() const
{
	backup_alias.clear();
}
void AliasPageObj::backup(int index) const
{
	backup_alias[index] = combo_aliases[index];
}
void AliasPageObj::restore_backup() const
{
	if(backup_alias.empty())
		return;
	saved = false;
	map<int,combo_alias> tmp;
	for(auto [ind,alias] : backup_alias)
	{
		tmp[ind] = combo_aliases[ind];
		combo_aliases[ind] = alias;
	}
	backup_alias = tmp; //undo again to redo
}



//
// Callers
//
void call_cmb_pages(optional<int> val)
{
	reset_combo_animations();
	reset_combo_animations2();
	ComboPageObj::get().call_dlg(val);
	setup_combo_animations();
	setup_combo_animations2();
}

void call_cpool_pages(optional<int> val)
{
	ComboPoolPageObj::get().call_dlg(val);
}

void call_autoc_pages(optional<int> val)
{
	AutoComboPageObj::get().call_dlg(val);
}

void call_alias_pages(optional<int> val)
{
	AliasPageObj::get().call_dlg(val);
}

