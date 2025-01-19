#include "gui/scrolling_pane.h"
#include "gui/dialog_runner.h"
#include "base/zc_alleg.h"
#include "gui/jwin.h"
#include <algorithm>

extern int32_t jwin_pal[jcMAX];

#define START_CLIP(d) \
	int clip = screen->clip;\
	int cl = screen->cl;\
	int cr = screen->cr;\
	int ct = screen->ct;\
	int cb = screen->cb;\
	set_clip_rect(screen, d->x+2,d->y+2, d->x+d->w-4, d->y+d->h-4)
#define START_CLIP_BYOB(d) \
	clip = screen->clip;\
	cl = screen->cl;\
	cr = screen->cr;\
	ct = screen->ct;\
	cb = screen->cb;\
	set_clip_rect(screen, d->x+2,d->y+2, d->x+d->w-4, d->y+d->h-4)

#define END_CLIP() \
	screen->clip = clip;\
	screen->cl = cl;\
	screen->cr = cr;\
	screen->ct = ct;\
	screen->cb = cb;

namespace GUI
{

static int32_t minusOneHundred()
{
	return -100;
}

/* When Allegro is searching the array for a mouse handler, mouseBreakerProc
 * checks the mouse position. If the mouse is not in the pane, it replaces
 * gui_mouse_x and gui_mouse_y so that Allegro won't find anything in the pane.
 * The only thing it will find is mouseFixerProc, which restores the old
 * mouse functions.
 */

int32_t ScrollingPane::mouseBreakerProc(int32_t msg, DIALOG* d, int32_t c)
{
	switch(msg)
	{
	case MSG_WANTMOUSE:
	{
		auto* sp=static_cast<ScrollingPane*>(d->dp);
		sp->oldMouseX=gui_mouse_x;
		sp->oldMouseY=gui_mouse_y;
		int32_t mx=gui_mouse_x();
		int32_t my=gui_mouse_y();
		DIALOG& spd=*sp->alDialog;
		if(mx<spd.x || mx>=spd.x+spd.w || my<spd.y || my>=spd.y+spd.h)
		{
			gui_mouse_x=minusOneHundred;
			gui_mouse_y=minusOneHundred;
		}
		return D_DONTWANTMOUSE;
	}
	}

	return D_O_K;
}

int32_t ScrollingPane::mouseFixerProc(int32_t msg, DIALOG* d, int32_t c)
{
	switch(msg)
	{
	case MSG_WANTMOUSE:
	{
		auto* sp=static_cast<ScrollingPane*>(d->dp);
		gui_mouse_x=sp->oldMouseX;
		gui_mouse_y=sp->oldMouseY;
		sp->oldMouseX=nullptr;
		sp->oldMouseY=nullptr;
		return D_DONTWANTMOUSE;
	}
	case MSG_DRAW:
		clear_clip_rect(screen);
		break;
	}
	return D_O_K;
}

int32_t scrollProc(int32_t msg, DIALOG* d, int32_t c)
{
	ScrollingPane* sp = static_cast<ScrollingPane*>(d->dp);
	switch(msg)
	{
		case MSG_DRAWCLIPPED:
		{
			auto* child=&sp->alDialog[c];
			START_CLIP(d);
			child->flags |= D_ISCLIPPED;
			child->proc(MSG_DRAW, child, 0);
			child->flags &= ~D_ISCLIPPED;
			END_CLIP();
			return D_O_K;
		}
		case MSG_CHILDFOCUSED:
			if(sp->scrollToShowChild(c))
				return D_REDRAW;
			else
				return D_O_K;
		
		case MSG_WANTFOCUS:
			if(gui_mouse_b())
				return D_WANTFOCUS|D_REDRAW;
			else return D_O_K;
		case MSG_GOTFOCUS:
		case MSG_LOSTFOCUS:
			return D_O_K;

		case MSG_DRAW:
		{
			rectfill(screen, d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->bg);
			d->flags &= ~D_GOTFOCUS;
			_jwin_draw_scrollable_frame(d, sp->contentHeight, sp->scrollPos, d->h, 0);
			START_CLIP(d);
			if(d->d1)
			{
				// The scrollbar is being dragged; we need to scroll and redraw
				// everything in the pane.
				int32_t scrollAmount=d->d2-sp->scrollPos;
				d->d2=sp->scrollPos;
				for(size_t i = 1; i < sp->childrenEnd; ++i)
				{
					DIALOG* child=&sp->alDialog[i];
					child->y += scrollAmount;
					object_message(child, MSG_DRAW, 0);
				}
			}
			END_CLIP();
			break;
		}
		case MSG_CLICK:
		{
			if(gui_mouse_x() >= d->x+d->w-18)
			{
				// This emits MSG_DRAW as it scrolls
				d->d1=1;
				d->d2=sp->scrollPos;
				_handle_jwin_scrollable_scroll_click(d, sp->contentHeight, &sp->scrollPos, nullptr);
				if(sp->scrollptr) *(sp->scrollptr) = sp->scrollPos;
				d->d1=0;
			}
			return D_O_K;
		}

		case MSG_WHEEL:
			sp->scroll(-8*c);
			return D_REDRAW;

		case MSG_XCHAR:
			switch(c>>8)
			{
				case KEY_PGDN:
				{
					if(sp->maxScrollPos < d->h*2)
						sp->scroll(d->h/3);
					else sp->scroll(d->h);
					return D_USED_CHAR|D_REDRAW;
				}
				case KEY_PGUP:
				{
					if(sp->maxScrollPos < d->h*2)
						sp->scroll(-d->h/3);
					else sp->scroll(-d->h);
					return D_USED_CHAR|D_REDRAW;
				}
				case KEY_HOME:
				{
					sp->scroll(-sp->maxScrollPos);
					return D_USED_CHAR|D_REDRAW;
				}
				case KEY_END:
				{
					sp->scroll(sp->maxScrollPos);
					return D_USED_CHAR|D_REDRAW;
				}
			}
	}

	return D_O_K;
}

ScrollingPane::ScrollingPane(): childrenEnd(0), scrollPos(0), maxScrollPos(0),
	contentHeight(0), oldMouseX(nullptr), oldMouseY(nullptr), scrollptr(nullptr),
	targHei(0_px)
{
	bgColor=jwin_pal[jcBOX];
}

void ScrollingPane::scroll(int32_t amount) noexcept
{
	if(maxScrollPos < 0) return; //No scrolling
	int32_t newPos=std::clamp(scrollPos+amount, 0, maxScrollPos);
	amount=newPos-scrollPos;
	scrollPos=newPos;
	if(scrollptr && alDialog) *scrollptr = scrollPos;
	for(size_t i = 1; i < childrenEnd; ++i)
		alDialog[i].y-=amount;
}

bool ScrollingPane::scrollToShowChild(int32_t childPos)
{
	DIALOG& pane=*alDialog;
	DIALOG& child=alDialog[childPos];
	if(child.y < pane.y)
	{
		scroll(child.y-pane.y);
		return true;
	}
	else if(child.y+child.h > pane.y+pane.h)
	{
		scroll((child.y+child.h)-(pane.y+pane.h));
		return true;
	}
	else
		return false;
}

void ScrollingPane::applyVisibility(bool visible)
{
	Widget::applyVisibility(visible);
	if(alDialog) alDialog.applyVisibility(visible);
	if(content)
	{
		if (screen)
		{
			int clip, cl, cr, ct, cb;
			if(isConstructed()) START_CLIP_BYOB(alDialog);
			content->applyVisibility(visible);
			if(isConstructed()) END_CLIP();
		}
		else content->applyVisibility(visible);
	}
}

void ScrollingPane::applyDisabled(bool dis)
{
	Widget::applyDisabled(dis);
	if(alDialog) alDialog.applyDisabled(dis);
	if(content)
	{
		content->applyDisabled(dis);
	}
}

void ScrollingPane::setPtr(int32_t* ptr)
{
	scrollptr = ptr;
	if(ptr && alDialog)
		scroll(*ptr-scrollPos);
}

void ScrollingPane::calculateSize()
{
	setPreferredWidth(25_em);
	setPreferredHeight(10_em);
	growToTarget();
	Widget::calculateSize();
}

void ScrollingPane::growToTarget()
{
	if(!content)
	{
		if(targHei > 0 && targHei > getHeight())
			setPreferredHeight(targHei);
		return;
	}
	content->calculateSize();
	setPreferredWidth(Size::pixels(content->getTotalWidth())+4_em);
	contentHeight=content->getTotalHeight();
	auto hei = getHeight();
	if(hei > contentHeight)
	{
		setPreferredHeight(Size::pixels(contentHeight));
	}
	else if(targHei > 0 && targHei > hei)
	{
		if(contentHeight < targHei)
			setPreferredHeight(Size::pixels(contentHeight));
		else setPreferredHeight(targHei);
	}
	
	maxScrollPos=contentHeight-getHeight();
}

void ScrollingPane::arrange(int32_t contX, int32_t contY, int32_t contW, int32_t contH)
{
	// We want to be about as big as possible...
	setPreferredWidth(Size::pixels(contW));
	setPreferredHeight(Size::pixels(contH));
	
	growToTarget();
	Widget::arrange(contX, contY, contW, contH);
	if(content)
	{
		Size widoffs = 0_px;
		if(getHeight() < content->getTotalHeight())
			widoffs = 18_px;
		content->arrange(x, y, getWidth()-widoffs, std::max(getHeight(),content->getTotalHeight()));
		contentHeight=content->getTotalHeight();
		maxScrollPos=contentHeight-getHeight();
	}
}

void ScrollingPane::realize(DialogRunner& runner)
{
	oldMouseX=gui_mouse_x;
	oldMouseY=gui_mouse_y;
	/*
	runner.push(shared_from_this(), DIALOG {
		jwin_frame_proc,
		x, y, getWidth(), getHeight(),
		fgColor, bgColor,
		0, // key
		getFlags(), // flags
		3, 0, // d1, d2
		nullptr, nullptr, nullptr // dp, dp2, dp3
	});
	*/
	runner.push(shared_from_this(), DIALOG {
		mouseBreakerProc,
		0, 0, 2000, 2000, // As int32_t as it covers the screen
		0, 0,
		0, // key
		getFlags(), // flags
		0, 0, // d1, d2
		this, nullptr, nullptr // dp, dp2, dp3
	});
	alDialog = runner.push(shared_from_this(), DIALOG {
		scrollProc,
		x, y, getWidth(), getHeight(),
		fgColor, bgColor,
		0, // key
		getFlags(), // flags
		0, 0, // d1, d2
		this, nullptr, nullptr // dp, dp2, dp3
	});

	size_t sizeBefore=runner.size();
	content->realize(runner);
	childrenEnd=runner.size()+1-sizeBefore;

	// Every child needs D_SCROLLING set so its proc knows to use
	// some special event handling.
	for(size_t i=1; i<childrenEnd; i++)
		alDialog[i].flags|=D_SCROLLING;

	runner.push(shared_from_this(), DIALOG {
		mouseFixerProc,
		-100, -100, 1, 1, // Exactly where the replacement functions point
		0, 0,
		0, // key
		getFlags(), // flags
		0, 0, // d1, d2
		this, nullptr, nullptr // dp, dp2, dp3
	});
	
	if(scrollptr)
		scroll(*scrollptr-scrollPos);
}

}
