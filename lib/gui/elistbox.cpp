#include <lib/gui/elistbox.h>
#include <lib/gui/elistboxcontent.h>
#include <lib/gui/eslider.h>
#include <lib/actions/action.h>
#include <lib/base/nconfig.h>
#ifdef USE_LIBVUGLES2
#include "vuplus_gles.h"
#endif

int eListbox::defaultScrollBarWidth = eListbox::DefaultScrollBarWidth;
int eListbox::defaultScrollBarOffset = eListbox::DefaultScrollBarOffset;
int eListbox::defaultScrollBarBorderWidth = eListbox::DefaultScrollBarBorderWidth;
int eListbox::defaultScrollBarScroll = eListbox::DefaultScrollBarScroll;
int eListbox::defaultScrollBarMode = eListbox::DefaultScrollBarMode;
int eListbox::defaultPageSize = eListbox::DefaultPageSize;
bool eListbox::defaultWrapAround = eListbox::DefaultWrapAround;
eRect eListbox::defaultPadding = eRect(1,1,1,1);

eListbox::eListbox(eWidget *parent) :
	eWidget(parent), m_list_orientation(listVertical), m_scrollbar_mode(showNever), m_prev_scrollbar_page(-1), m_scrollbar_scroll(byPage),
	m_content_changed(false), m_enabled_wrap_around(false), m_scrollbar_width(10),
	m_top(0), m_selected(0), m_itemheight(25), m_itemwidth(25),
	m_items_per_page(0), m_selection_enabled(1), m_page_size(0), m_native_keys_bound(false), m_first_selectable_item(-1),m_last_selectable_item(-1),m_scrollbar(nullptr)
{
	m_scrollbar_width = eListbox::defaultScrollBarWidth;
	m_scrollbar_offset = eListbox::defaultScrollBarOffset;
	m_scrollbar_border_width = eListbox::defaultScrollBarBorderWidth;
	m_scrollbar_scroll = eListbox::defaultScrollBarScroll;
	m_enabled_wrap_around = eListbox::defaultWrapAround;
	m_scrollbar_mode = eListbox::defaultScrollBarMode;
	m_page_size = eListbox::defaultPageSize;

	memset(static_cast<void*>(&m_style), 0, sizeof(m_style));
	m_style.m_text_padding = eListbox::defaultPadding;
//	setContent(new eListboxStringContent());

	allowNativeKeys(true);

	if(m_scrollbar_mode != showNever)
		setScrollbarMode(m_scrollbar_mode);

}

eListbox::~eListbox()
{
	if (m_scrollbar)
		delete m_scrollbar;

	allowNativeKeys(false);
}

void eListbox::setScrollbarMode(int mode)
{
	m_scrollbar_mode = mode;
	if (m_scrollbar)
	{
		if (m_scrollbar_mode == showNever)
		{
			delete m_scrollbar;
			m_scrollbar=0;
		}
	}
	else
	{
		m_scrollbar = new eSlider(this);
		m_scrollbar->setIsScrollbar();
		m_scrollbar->hide();
		m_scrollbar->setBorderWidth(m_scrollbar_border_width);
		m_scrollbar->setOrientation(eSlider::orVertical);
		m_scrollbar->setRange(0, 100);
		if (m_scrollbarbackgroundpixmap) m_scrollbar->setBackgroundPixmap(m_scrollbarbackgroundpixmap);
		if (m_scrollbarpixmap) m_scrollbar->setPixmap(m_scrollbarpixmap);
		if (m_style.m_scollbarborder_color_set) m_scrollbar->setBorderColor(m_style.m_scollbarborder_color);
		if (m_style.m_scrollbarforeground_color_set) m_scrollbar->setForegroundColor(m_style.m_scrollbarforeground_color);
		if (m_style.m_scrollbarbackground_color_set) m_scrollbar->setBackgroundColor(m_style.m_scrollbarbackground_color);
	}
}


void eListbox::setScrollbarScroll(int scroll)
{
	if (m_scrollbar && m_scrollbar_scroll != scroll)
	{
		m_scrollbar_scroll = scroll;
		updateScrollBar();
		return;
	}
	m_scrollbar_scroll = scroll;
}

void eListbox::setWrapAround(bool state)
{
	m_enabled_wrap_around = state;
}

void eListbox::setContent(iListboxContent *content)
{
	m_content = content;
	if (content)
		m_content->setListbox(this);
	entryReset();
}

void eListbox::allowNativeKeys(bool allow)
{
	if (m_native_keys_bound != allow)
	{
		ePtr<eActionMap> ptr;
		eActionMap::getInstance(ptr);
		if (allow)
			ptr->bindAction("ListboxActions", (int64_t)0, 0, this);
		else
			ptr->unbindAction(this, 0);
		m_native_keys_bound = allow;
	}
}

bool eListbox::atBegin()
{
	if (m_content && !m_selected)
		return true;
	return false;
}

bool eListbox::atEnd()
{
	if (m_content && m_content->size() == m_selected+1)
		return true;
	return false;
}

// Deprecated
void eListbox::moveToEnd()
{
	eWarning("[eListbox] moveToEnd is deprecated. Use moveSelection or goBottom instead.");
	if (!m_content)
		return;
	/* move to last existing one ("end" is already invalid) */
	m_content->cursorEnd(); m_content->cursorMove(-1);
	/* current selection invisible? */
	if (m_top + m_items_per_page <= m_content->cursorGet())
	{
		int rest = m_content->size() % m_items_per_page;
		if (rest)
			m_top = m_content->cursorGet() - rest + 1;
		else
			m_top = m_content->cursorGet() - m_items_per_page + 1;
		if (m_top < 0)
			m_top = 0;
	}
}

void eListbox::moveSelectionTo(int index)
{
	if (m_content)
	{
		m_content->cursorSet(index);
		m_content_changed = true;
		moveSelection(justCheck + 100);
	}
}

int eListbox::getCurrentIndex()
{
	if (m_content && m_content->cursorValid())
		return m_content->cursorGet();
	return 0;
}

void eListbox::updateScrollBar()
{
	if (!m_scrollbar || !m_content || m_scrollbar_mode == showNever )
		return;
	int entries = m_content->size();
	bool scrollbarvisible = m_scrollbar->isVisible();
	if (m_content_changed)
	{
		int width = size().width();
		int height = size().height();

		if (m_scrollbar_scroll == byLine) {
			m_scrollbar->setRange(0,height-(m_scrollbar_border_width*2));
		}

		m_content_changed = false;
		if (m_scrollbar_mode == showLeftOnDemand || m_scrollbar_mode == showLeftAlways)
		{
			m_content->setSize(eSize(width-m_scrollbar_width-m_scrollbar_offset, m_itemheight));
			m_scrollbar->move(ePoint(0, 0));
			m_scrollbar->resize(eSize(m_scrollbar_width, height));
			if (entries > m_items_per_page || m_scrollbar_mode == showLeftAlways)
			{
				m_scrollbar->show();
				scrollbarvisible = true;
			}
			else
			{
				m_scrollbar->hide();
				scrollbarvisible = false;
			}
		}
		else if (entries > m_items_per_page || m_scrollbar_mode == showAlways)
		{
			m_scrollbar->move(ePoint(width-m_scrollbar_width, 0));
			m_scrollbar->resize(eSize(m_scrollbar_width, height));
			m_content->setSize(eSize(width-m_scrollbar_width-m_scrollbar_offset, m_itemheight));
			m_scrollbar->show();
			scrollbarvisible = true;
		}
		else
		{
			m_content->setSize(eSize(width, m_itemheight));
			m_scrollbar->hide();
			scrollbarvisible = false;
		}
	}

	// Don't set Start/End if scollbar not visible or entries/m_items_per_page = 0
	if (m_items_per_page && entries && scrollbarvisible)
	{

		if(m_scrollbar_scroll == byLine) {

			if(m_prev_scrollbar_page != m_selected) {
				m_prev_scrollbar_page = m_selected;

				int start = 0;
				int range = size().height() - (m_scrollbar_border_width * 2);
				int end = range;
				// calculate thumb only if needed
				if (entries > 1 && entries > m_items_per_page)
				{
					float fthumb = (float)m_items_per_page / (float)entries * range;
					float fsteps = ((float)(range - fthumb) / (float)entries);
					float fstart = (float)m_selected * fsteps;
					fthumb = (float)range - (fsteps * (float)(entries - 1));
					int visblethumb = fthumb < 4 ? 4 : (int)(fthumb + 0.5);
					start = (int)(fstart + 0.5);
					end = start + visblethumb;
					if (end > range) {
						end = range;
						start = range - visblethumb;
					}
					//eDebug("[eListbox] updateScrollBar thumb=%d steps=%d start=%d end=%d range=%d m_items_per_page=%d entries=%d m_selected=%d", thumb, steps, start, end, range, m_items_per_page, entries, m_selected);
				}

				m_scrollbar->setStartEnd(start, end, true);

			} 
			return;
		}

		int curVisiblePage = m_top / m_items_per_page;

		if (m_prev_scrollbar_page != curVisiblePage)
		{
			m_prev_scrollbar_page = curVisiblePage;
			int pages = entries / m_items_per_page;
			if ((pages*m_items_per_page) < entries)
				++pages;
			int start=(m_top*100)/(pages*m_items_per_page);
			int vis=(m_items_per_page*100+pages*m_items_per_page-1)/(pages*m_items_per_page);
			if (vis < 3)
				vis=3;
			m_scrollbar->setStartEnd(start,start+vis);
		}
	}
}

int eListbox::getEntryTop()
{
	return (m_selected - m_top) * m_itemheight;
}

int eListbox::event(int event, void *data, void *data2)
{
	switch (event)
	{
	case evtPaint:
	{
		ePtr<eWindowStyle> style;

		if (!m_content)
			return eWidget::event(event, data, data2);
		ASSERT(m_content);

		getStyle(style);

		if (!m_content)
			return 0;

		gPainter &painter = *(gPainter*)data2;

		m_content->cursorSave();
		m_content->cursorMove(m_top - m_selected);

		gRegion entryrect = eRect(0, 0, size().width(), m_itemheight);
		const gRegion &paint_region = *(gRegion*)data;

		int xoffset = 0;
		if (m_scrollbar && (m_scrollbar_mode == showLeftOnDemand || m_scrollbar_mode == showLeftAlways))
		{
			xoffset = m_scrollbar->size().width() + m_scrollbar_offset;
		}

		int line = 0;

		for (int y = 0, i = 0; i <= m_items_per_page; y += m_itemheight, ++i)
		{
			gRegion entry_clip_rect = paint_region & entryrect;

			bool sel = (m_selected == m_content->cursorGet() && m_content->size() && m_selection_enabled);

			if(sel)
				line = i;

			if (!entry_clip_rect.empty())
				m_content->paint(painter, *style, ePoint(xoffset, y), sel);
#ifdef USE_LIBVUGLES2
			if (sel) {
				ePoint pos = getAbsolutePosition();
				painter.sendShowItem(m_dir, ePoint(pos.x(), pos.y() + y), eSize(m_scrollbar && m_scrollbar->isVisible() ? size().width() - m_scrollbar->size().width() : size().width(), m_itemheight));
				gles_set_animation_listbox_current(pos.x(), pos.y() + y, m_scrollbar && m_scrollbar->isVisible() ? size().width() - m_scrollbar->size().width() : size().width(), m_itemheight);
				m_dir = justCheck;
			}
#endif
				/* (we could clip with entry_clip_rect, but
				   this shouldn't change the behavior of any
				   well behaving content, so it would just
				   degrade performance without any gain.) */

			m_content->cursorMove(+1);
			entryrect.moveBy(ePoint(0, m_itemheight));

		}

		// clear/repaint empty/unused space between scrollbar and listboxentrys
		if (m_scrollbar)
		{
			if (m_scrollbar_mode == showLeftOnDemand || m_scrollbar_mode == showLeftAlways)
			{
				style->setStyle(painter, eWindowStyle::styleListboxNormal);
				if (m_scrollbar->isVisible())
				{
					painter.clip(eRect(m_scrollbar->position() + ePoint(m_scrollbar->size().width(), 0), eSize(m_scrollbar_offset,m_scrollbar->size().height())));
				}
				else
				{
					painter.clip(eRect(m_scrollbar->position(), eSize(m_scrollbar->size().width() + m_scrollbar_offset, m_scrollbar->size().height())));
				}
				painter.clear();
				painter.clippop();
			}
			else if (m_scrollbar->isVisible())
			{
				style->setStyle(painter, eWindowStyle::styleListboxNormal);
				painter.clip(eRect(m_scrollbar->position() - ePoint(m_scrollbar_offset,0), eSize(m_scrollbar_offset,m_scrollbar->size().height())));
				painter.clear();
				painter.clippop();
			}

		}

		m_content->cursorSaveLine(line);
		m_content->cursorRestore();

		return 0;
	}

	case evtChangedSize:
		recalcSize();
		return eWidget::event(event, data, data2);

	case evtAction:
		if (isVisible() && !isLowered())
		{
			moveSelection((long)data2);
			return 1;
		}
		return 0;
	default:
		return eWidget::event(event, data, data2);
	}
}

void eListbox::recalcSize()
{
	m_content_changed=true;
	m_prev_scrollbar_page=-1;
	if (m_content)
		m_content->setSize(eSize(size().width(), m_itemheight));
	m_items_per_page = size().height() / m_itemheight;

	if (m_items_per_page < 0) /* TODO: whyever - our size could be invalid, or itemheigh could be wrongly specified. */
 		m_items_per_page = 0;

	moveSelection(justCheck);
}

void eListbox::setItemHeight(int h)
{
	if (h)
		m_itemheight = h;
	else
		m_itemheight = 20; // TODO : why 20
	recalcSize();
}

void eListbox::setSelectionEnable(int en)
{
	if (m_selection_enabled == en)
		return;
	m_selection_enabled = en;
	entryChanged(m_selected); /* redraw current entry */
}

void eListbox::entryAdded(int index)
{
	m_first_selectable_item = -1;
	m_last_selectable_item = -1;
	if (m_content && (m_content->size() % m_items_per_page) == 1)
		m_content_changed=true;
	/* manage our local pointers. when the entry was added before the current position, we have to advance. */

		/* we need to check <= - when the new entry has the (old) index of the cursor, the cursor was just moved down. */
	if (index <= m_selected)
		++m_selected;
	if (index <= m_top)
		++m_top;

		/* we have to check wether our current cursor is gone out of the screen. */
		/* moveSelection will check for this case */
	moveSelection(justCheck);

		/* now, check if the new index is visible. */
	if ((m_top <= index) && (index < (m_top + m_items_per_page)))
	{
			/* todo, calc exact invalidation... */
		invalidate();
	}
}

void eListbox::entryRemoved(int index)
{
	m_first_selectable_item = -1;
	m_last_selectable_item = -1;
	if (m_content && !(m_content->size() % m_items_per_page))
		m_content_changed=true;

	if (index == m_selected && m_content)
		m_selected = m_content->cursorGet();

	if (m_content && m_content->cursorGet() >= m_content->size())
		moveSelection(moveUp);
	else
		moveSelection(justCheck);

	if ((m_top <= index) && (index < (m_top + m_items_per_page)))
	{
			/* todo, calc exact invalidation... */
		invalidate();
	}
}

void eListbox::entryChanged(int index)
{
	if ((m_top <= index) && (index < (m_top + m_items_per_page)))
	{
		gRegion inv = eRect(0, m_itemheight * (index-m_top), size().width(), m_itemheight);
		invalidate(inv);
	}
}

void eListbox::entryReset(bool selectionHome)
{
	m_first_selectable_item = -1;
	m_last_selectable_item = -1;
	m_content_changed = true;
	m_prev_scrollbar_page = -1;
	int oldsel;

	if (selectionHome)
	{
		if (m_content)
			m_content->cursorHome();
		m_top = 0;
		m_selected = 0;
	}

	if (m_content && (m_selected >= m_content->size()))
	{
		if (m_content->size())
			m_selected = m_content->size() - 1;
		else
			m_selected = 0;
		m_content->cursorSet(m_selected);
	}

	oldsel = m_selected;
	moveSelection(justCheck);
		/* if oldsel != m_selected, selectionChanged was already
		   emitted in moveSelection. we want it in any case, so otherwise,
		   emit it now. */
	if (oldsel == m_selected)
		/* emit */ selectionChanged();
	invalidate();
}

void eListbox::setFont(gFont *font)
{
	m_style.m_font = font;
}

void eListbox::setEntryFont(gFont *font)
{
	m_style.m_font = font;
}

void eListbox::setValueFont(gFont *font)
{
	m_style.m_valuefont = font;
}

void eListbox::setVAlign(int align)
{
	m_style.m_valign = align;
}

void eListbox::setHAlign(int align)
{
	m_style.m_halign = align;
}

void eListbox::setTextPadding(const eRect &padding)
{
	m_style.m_text_padding = padding;
}

void eListbox::setUseVTIWorkaround(void)
{
	m_style.m_use_vti_workaround = 1;
}

void eListbox::setBackgroundColor(gRGB &col)
{
	m_style.m_background_color = col;
	m_style.m_background_color_set = 1;
}

void eListbox::setBackgroundColorSelected(gRGB &col)
{
	m_style.m_background_color_selected = col;
	m_style.m_background_color_selected_set = 1;
}

void eListbox::setForegroundColor(gRGB &col)
{
	m_style.m_foreground_color = col;
	m_style.m_foreground_color_set = 1;
}

void eListbox::setForegroundColorSelected(gRGB &col)
{
	m_style.m_foreground_color_selected = col;
	m_style.m_foreground_color_selected_set = 1;
}

void eListbox::setBorderColor(const gRGB &col)
{
	m_style.m_border_color = col;
}

void eListbox::setBorderWidth(int size)
{
	m_style.m_border_size = size;
	if (m_scrollbar) m_scrollbar->setBorderWidth(size);
}

void eListbox::setScrollbarBorderWidth(int width)
{
	m_style.m_scrollbarborder_width = width;
	m_style.m_scrollbarborder_width_set = 1;
	if (m_scrollbar) m_scrollbar->setBorderWidth(width);
}

void eListbox::setScrollbarWidth(int size)
{
	m_scrollbar_width = size;
}

void eListbox::setScrollbarOffset(int size)
{
	m_scrollbar_offset = size;
}

void eListbox::setBackgroundPixmap(ePtr<gPixmap> &pm)
{
	m_style.m_background = pm;
}

void eListbox::setSelectionPixmap(ePtr<gPixmap> &pm)
{
	m_style.m_selection = pm;
}

void eListbox::setScrollbarForegroundPixmap(ePtr<gPixmap> &pm)
{
	m_scrollbarpixmap = pm;
	if (m_scrollbar && m_scrollbarpixmap) m_scrollbar->setPixmap(pm);
}

void eListbox::setScrollbarBackgroundColor(gRGB &col)
{
	m_style.m_scrollbarbackground_color = col;
	m_style.m_scrollbarbackground_color_set = 1;
	if (m_scrollbar) m_scrollbar->setBackgroundColor(col);
}

void eListbox::setScrollbarForegroundColor(gRGB &col)
{
	m_style.m_scrollbarforeground_color = col;
	m_style.m_scrollbarforeground_color_set = 1;
	if (m_scrollbar) m_scrollbar->setForegroundColor(col);
}

void eListbox::setScrollbarBorderColor(const gRGB &col)
{
	m_style.m_scollbarborder_color = col;
	m_style.m_scollbarborder_color_set = 1;
	if (m_scrollbar) m_scrollbar->setBorderColor(col);
}

void eListbox::setScrollbarBackgroundPixmap(ePtr<gPixmap> &pm)
{
	m_scrollbarbackgroundpixmap = pm;
	if (m_scrollbar && m_scrollbarbackgroundpixmap) m_scrollbar->setBackgroundPixmap(pm);
}

void eListbox::invalidate(const gRegion &region)
{
	gRegion tmp(region);
	if (m_content)
		m_content->updateClip(tmp);
	eWidget::invalidate(tmp);
}

struct eListboxStyle *eListbox::getLocalStyle(void)
{
		/* transparency is set directly in the widget */
	m_style.m_transparent_background = isTransparent();
	return &m_style;
}

void eListbox::setItemWidth(int w)
{
	if (w)
		m_itemwidth = w;
	else
		m_itemwidth = 20; // TODO : why 20
	recalcSize();
}

void eListbox::setOrientation(int o)
{
	m_list_orientation = o;
	invalidate();
}

void eListbox::moveSelection(long dir)
{
	/* refuse to do anything without a valid list. */
	if (!m_content)
		return;
	/* if our list does not have one entry, don't do anything. */
	if (!m_items_per_page || !m_content->size())
		return;

	/* we need the old top/sel to see what we have to redraw */
	int oldtop = m_top;
	int oldsel = m_selected;
	int prevsel = oldsel;
	int newsel;
	int pageOffset = (m_page_size > 0 && m_scrollbar_scroll == byLine) ? m_page_size : m_items_per_page;
	bool indexchanged = dir > 100;
	if(indexchanged) {
		dir -= 100;
	}

#ifdef USE_LIBVUGLES2
	m_dir = dir;
#endif
	switch (dir)
	{
	case moveBottom:
		m_content->cursorEnd();
		[[fallthrough]];
	case moveUp:
		do
		{
			m_content->cursorMove(-1);
			newsel = m_content->cursorGet();
			if (newsel == prevsel) {  // cursorMove reached top and left cursor position the same. Must wrap around ?
				if (m_enabled_wrap_around)
				{
					m_content->cursorEnd();
					m_content->cursorMove(-1);
					newsel = m_content->cursorGet();
				}
				else
				{
					m_content->cursorSet(oldsel);
					break;
				}
			}
			prevsel = newsel;
		}
		while (newsel != oldsel && !m_content->currentCursorSelectable());
		break;
	case refresh:
		oldsel = ~m_selected;
		break;
	case moveTop:
		m_content->cursorHome();
		[[fallthrough]];
	case justCheck:
		if (m_content->cursorValid() && m_content->currentCursorSelectable())
			break;
		[[fallthrough]];
	case moveDown:
		do
		{
			m_content->cursorMove(1);
			if (!m_content->cursorValid()) { //cursorMove reached end and left cursor position past the list. Must wrap around ?
				if (m_enabled_wrap_around)
					m_content->cursorHome();
				else
					m_content->cursorSet(oldsel);
			}
			newsel = m_content->cursorGet();
		}
		while (newsel != oldsel && !m_content->currentCursorSelectable());
		break;
	case movePageUp:
	{
		int pageind;
		do
		{
			m_content->cursorMove(-pageOffset);
			newsel = m_content->cursorGet();
			pageind = newsel % m_items_per_page; // rememer were we land in thsi page (could be different on topmost page)
			prevsel = newsel - pageind; // get top of page index
			// find first selectable entry in new page. First check bottom part, than upper part
			while (newsel != prevsel + m_items_per_page && m_content->cursorValid() && !m_content->currentCursorSelectable())
			{
				m_content->cursorMove(1);
				newsel = m_content->cursorGet();
			}
			if (!m_content->currentCursorSelectable()) // no selectable found in bottom part of page
			{
				m_content->cursorSet(prevsel + pageind);
				while (newsel != prevsel && !m_content->currentCursorSelectable())
				{
					m_content->cursorMove(-1);
					newsel = m_content->cursorGet();
				}
			}
			if (m_content->currentCursorSelectable())
				break;
			if (newsel == 0) // at top and nothing found . Go down till something selectable or old location
			{
				while (newsel != oldsel && !m_content->currentCursorSelectable())
				{
					m_content->cursorMove(1);
					newsel = m_content->cursorGet();
				}
				break;
			}
			m_content->cursorSet(prevsel + pageind);
		}
		while (newsel == prevsel);
		break;
	}
	case movePageDown:
	{
		int pageind;
		do
		{
			m_content->cursorMove(pageOffset);
			if (!m_content->cursorValid())
				m_content->cursorMove(-1);
			newsel = m_content->cursorGet();
			pageind = newsel % m_items_per_page;
			prevsel = newsel - pageind; // get top of page index
			// find a selectable entry in the new page. first look up then down from current screenlocation on the page
			while (newsel != prevsel && !m_content->currentCursorSelectable())
			{
				m_content->cursorMove(-1);
				newsel = m_content->cursorGet();
			}
			if (!m_content->currentCursorSelectable()) // no selectable found in top part of page
			{
				m_content->cursorSet(prevsel + pageind);
				do {
					m_content->cursorMove(1);
					newsel = m_content->cursorGet();
				}
			    while (newsel != prevsel + m_items_per_page && m_content->cursorValid() && !m_content->currentCursorSelectable());
			}
			if (!m_content->cursorValid())
			{
				// we reached the end of the list
				// Back up till something selectable or we reach oldsel again
				// E.g this should bring us back to the last selectable item on the original page
				do
				{
					m_content->cursorMove(-1);
					newsel = m_content->cursorGet();
				}
				while (newsel != oldsel && !m_content->currentCursorSelectable());
				break;
			}
			if (newsel != prevsel + m_items_per_page)
				break;
			m_content->cursorSet(prevsel + pageind); // prepare for next page down
		}
		while (newsel == prevsel + m_items_per_page);
		break;
	}
	}

	/* now, look wether the current selection is out of screen */
	m_selected = m_content->cursorGet();
	m_top = m_selected - (m_selected % m_items_per_page);

	/*  new scollmode by line if not on the first page */
	if(m_scrollbar_scroll == byLine && m_content->size() > m_items_per_page)
	{

		int oldline = m_content->cursorRestoreLine();
		int max = m_content->size() - m_items_per_page;
		bool customPageSize = pageOffset != m_items_per_page;
		//eDebug("[eListbox] moveSelection 1 dir=%d oldline=%d oldsel=%d m_selected=%d m_items_per_page=%d sz=%d max=%d", dir, oldline, oldsel, m_selected, m_items_per_page, m_content->size(), max);

		bool jumpBottom = (dir == moveBottom);

		if(dir == movePageDown && m_selected > max && !customPageSize) {
			jumpBottom = true;
		}

		if(dir == moveUp || (customPageSize && dir == movePageUp) ) {
			if(m_selected > oldsel) {
				jumpBottom = true;
			}
			else if (oldline > 0)
				oldline-= oldsel - m_selected;

			if(oldline < 0 && m_selected > m_items_per_page)
				oldline = 0;

		}

		if(m_last_selectable_item == -1 && dir == justCheck)
		{
			m_content->cursorEnd();
			do
			{
				m_content->cursorMove(-1);
				m_last_selectable_item = m_content->cursorGet();
			}
			while (!m_content->currentCursorSelectable());
			m_content->cursorSet(m_selected);
		}

		if(dir == moveDown || (customPageSize && dir == movePageDown)) {

			int newline = oldline + (m_selected - oldsel);
			if(newline < m_items_per_page && newline > 0) {
				m_top = oldsel - oldline;
			}
			else {
				m_top = m_selected - (m_items_per_page - 1);
				if(m_selected < m_items_per_page )
					m_top=0;
			}

			if (m_last_selectable_item != m_content->size() - 1 && m_selected >= m_last_selectable_item)
				jumpBottom = true;
		}

		if(jumpBottom) {
			m_top = max;
		}
		else if (dir == justCheck)
		{
			if(m_first_selectable_item==-1)
			{
				m_first_selectable_item = 0;
				if(m_selected>0)
				{
					m_content->cursorHome();
					if (!m_content->currentCursorSelectable()) {
						do
						{
							m_content->cursorMove(1);
							m_first_selectable_item = m_content->cursorGet();
						}
						while (!m_content->currentCursorSelectable());
					}
					m_content->cursorSet(m_selected);
					if(oldline==0)
						oldline = m_selected;
				}
			}
			if(indexchanged && m_selected < m_items_per_page) {
				oldline = m_selected;
			}

			m_top = m_selected - oldline;
		}
		else if (dir == moveUp || (customPageSize && dir == movePageUp))
		{
			if(m_first_selectable_item > 0 && m_selected == m_first_selectable_item)
			{
				oldline = m_selected;
			}
			m_top = m_selected - oldline;
		}

		if(m_top < 0 || oldline < 0) {
			m_top = 0;
		}

		//eDebug("[eListbox] moveSelection 2 dir=%d oldline=%d oldtop=%d m_top=%d m_selected=%d m_items_per_page=%d sz=%d jumpBottom=%d", dir, oldline, oldtop, m_top, m_selected, m_items_per_page, m_content->size(),jumpBottom);

	}

	// if it is, then the old selection clip is irrelevant, clear it or we'll get artifacts
	if (m_top != oldtop && m_content)
		m_content->resetClip();

	if (oldsel != m_selected)
		/* emit */ selectionChanged();

	updateScrollBar();

	if (m_top != oldtop) {
		invalidate();
	}
	else if (m_selected != oldsel)
	{
		/* redraw the old and newly selected */
		gRegion inv = eRect(0, m_itemheight * (m_selected-m_top), size().width(), m_itemheight);
		inv |= eRect(0, m_itemheight * (oldsel-m_top), size().width(), m_itemheight);
		invalidate(inv);
	}

}
