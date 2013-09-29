#include "gui.h"
#include "widget.h"
#include "event.h"
#include "layout.h"
#include "text.h"
#include "adjust.h"

static eint text_init(eHandle, eValist);
static eint text_init_data(eHandle, ePointer);
static void text_free_data(eHandle, ePointer);
static void text_init_orders(eGeneType, ePointer);
static eint text_expose(eHandle, GuiWidget *, GalEventExpose *);
static eint text_expose_bg(eHandle, GuiWidget *, GalEventExpose *);
static eint text_keydown(eHandle, GalEventKey *);
static eint text_char(eHandle, echar);
static eint text_wrap_line(GuiText *, TextLine *);
static void text_cursor_setpos(eHandle, GuiText *, TextLine *, TextWrap *, eint, bool);
static void text_show_cursor(eHandle, GuiText *, TextLine *, TextWrap *, eint, bool);
static void text_hide_cursor(eHandle, GuiText *);
static eint text_configure(eHandle, GuiWidget *, GalEventConfigure *);
static eint text_resize(eHandle, GuiWidget *, GalEventResize *);
static void text_wrap(eHandle, GuiText *);
static eint text_delete_select(eHandle, GuiText *, eint *, eint *);
static void text_set_font(eHandle, GalFont);
static void insert_text_to_cursor(eHandle, GuiText *, const echar *, eint);
static void egui_show_cursor(GalDrawable, TextCursor *, bool);
static void egui_hide_cursor(GalDrawable, TextCursor *);
static eint text_line_insert(GuiText *, const echar *, eint);

static int (*text_char_keydown)(eHandle, GalEventKey *) = NULL;

eGeneType egui_genetype_text(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			text_init_orders,
			sizeof(GuiText),
			text_init_data,
			text_free_data,
			NULL,
		};

		gtype = e_register_genetype(&info,
				GTYPE_WIDGET, GTYPE_CHAR,
				GTYPE_LAYOUT, GTYPE_HOOK, NULL);
	}
	return gtype;
}

static void egui_update_region(eHandle hobj, GalRegion *region)
{
	GalRegionSec *head = region->head;
	while (head) {
		GalRect rc;
		rc.x = head->sec.x1;
		rc.y = head->sec.y1;
		rc.w = head->sec.x2 - rc.x;
		rc.h = head->sec.y2 - rc.y;
		egui_update_rect(hobj, &rc);

		head = head->next;
	}
}

static eint text_focus_in(eHandle hobj)
{
	GuiText *text = GUI_TEXT_DATA(hobj);
	if (!text->cursor.show && !text->is_sel) {
		if (text->cursor.y >= 0 && text->cursor.y < text->height - text->hline)
			egui_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor, true);
	}
	return 0;
}

static eint text_focus_out(eHandle hobj)
{
	GuiText *text = GUI_TEXT_DATA(hobj);
	if (text->cursor.show && !text->is_sel)
		egui_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, &GUI_TEXT_DATA(hobj)->cursor);
	return 0;
}

static TextLine *find_line_by_id(GuiText *text, eint id)
{
	TextLine *line = text->line_head;

	while (line) {
		if (line->id == id)
			break;
		line = line->next;
	}

	return line;
}

static TextWrap *ioffset_to_wrap(TextLine *line, eint ioff)
{
	TextWrap *wrap = &line->wrap_head;
	while (wrap) {
		if (ioff < wrap->begin + wrap->nichar)
			return wrap;
		wrap = wrap->next;
	}

	return line->wrap_tail;
}

static eint xcoor_to_ioffset(GuiText *text, TextWrap *wrap, eint x)
{
	eint _x = x + text->offset_x;
	eint  w = 0, o = 0;
	eint  i;

	TextLine *line = wrap->line;
	for (i = wrap->begin; i < wrap->begin + wrap->nichar; i++) {
		w += line->glyphs[i].w;
		if (_x >= o && _x < w)
			break;
		o = w;
	}

	if (i == wrap->begin + wrap->nichar &&
			wrap->id != line->nwrap - 1)
		i--;
	return i;
}

static TextWrap *ycoor_to_wrap(GuiText *text, eint y)
{
	TextLine *line = text->line_head;
	TextWrap *wrap;

	eint h = text->hline;
	eint t = line->offset_y - text->offset_y;
	eint b = line->nwrap * h - text->offset_y;

	while (true) {
		if (y >= t && y < b)
			break;
		t    = b;
		if (!line->next)
			return line->wrap_tail;
		line = line->next;
		b += line->nwrap * h;
	}

	t = b = line->offset_y - text->offset_y;

	wrap = &line->wrap_head;
	while (wrap) {
		b += h;
		if (y >= t && y < b)
			break;
		t = b;
		wrap = wrap->next;
	}

	return wrap;
}

static eint text_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiText  *text = GUI_TEXT_DATA(hobj);
	TextWrap *wrap;
	eint ioff;

	if (text->is_sel && REGION_NO_EMPTY(text->sel_rgn)) {
		text->is_sel = false;
		egui_update_region(hobj, text->sel_rgn);
		egal_region_empty(text->sel_rgn);
	}

	wrap = ycoor_to_wrap(text, ent->point.y);
	ioff = xcoor_to_ioffset(text, wrap, ent->point.x);

	text_show_cursor(hobj, text, wrap->line, wrap, ioff, true);

	text->bn_down = true;
	text->select.s_id   = wrap->line->id;
	text->select.s_ioff = ioff;

	egal_grab_pointer(GUI_WIDGET_DATA(hobj)->window, true, text->gal_cursor);

	return 0;
}

static eint text_lbuttonup(eHandle hobj, GalEventMouse *ent)
{
	GUI_TEXT_DATA(hobj)->bn_down = false;
	egal_ungrab_pointer(GUI_WIDGET_DATA(hobj)->window);
	return 0;
}

static inline void ioffset_to_coor(GuiText *text, TextLine *line, eint ioff, eint *x, eint *y, eint *b)
{
	TextWrap *wrap = ioffset_to_wrap(line, ioff);
	eint i, w = 0;
	eint _x, _y;

	for (i = wrap->begin; i < ioff; i++)
		w += line->glyphs[i].w;

	_x = w - text->offset_x;
	_y = line->offset_y + wrap->id * text->hline - text->offset_y;
	if (_y < 0) {
		if (_y + text->hline > 0)
			*b = _y + text->hline;
		else
			_x = 0;
		_y = 0;
	}
	else
		*b = text->hline;

	*x = _x; *y = _y;
}

static void select_normalize(GuiText *text, eint *sid, eint *sioff, eint *eid, eint *eioff)
{
	if (text->select.s_id > text->select.e_id) {
		*sid   = text->select.e_id;
		*eid   = text->select.s_id;
		*sioff = text->select.e_ioff;
		*eioff = text->select.s_ioff;
	}
	else {
		*sid   = text->select.s_id;
		*eid   = text->select.e_id;
		if (text->select.s_id == text->select.e_id &&
				text->select.s_ioff > text->select.e_ioff) {
			*sioff = text->select.e_ioff;
			*eioff = text->select.s_ioff;
		}
		else {
			*sioff = text->select.s_ioff;
			*eioff = text->select.e_ioff;
		}
	}
}

static void set_select_region(eHandle hobj, GuiText *text)
{
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);
	GalRegion *region = text->sel_rgn;
	TextLine  *line;
	GalRect   rc;

	eint sid, sioff, eid, eioff;
	eint x1, y1, x2, y2;
	eint b1 = 0;
	eint b2 = 0;

	select_normalize(text, &sid, &sioff, &eid, &eioff);

	line = find_line_by_id(text, sid);
	ioffset_to_coor(text, line, sioff, &x1, &y1, &b1);
	if (sid != eid)
		line = find_line_by_id(text, eid);
	ioffset_to_coor(text, line, eioff, &x2, &y2, &b2);

	if (y1 == 0 && y2 == 0 && x1 == x2) {
		egal_region_empty(region);
	}
	else if (y2 > y1) {
		rc.x = 0;
		rc.y = y1;
		rc.w = widget->rect.w;
		rc.h = y2 - y1 + b2;
		egal_region_set_rect(region, &rc);
		rc.w = x1;
		rc.h = b1;
		egal_region_subtract_rect(region, &rc);
		rc.x = x2;
		rc.w = widget->rect.w - x2;
		rc.y = y2;
		rc.h = b2;
		egal_region_subtract_rect(region, &rc);
	}
	else {
		rc.x = x1;
		rc.y = y1;
		rc.w = x2 - x1;
		rc.h = b2;
		egal_region_set_rect(region, &rc);
	}
}

static void text_set_scrollbar(GuiText *text, eint y)
{
	eint offset_y = text->offset_y;
	if (y < 0) {
		offset_y += y;
		if (offset_y < 0)
			offset_y = 0;
		offset_y -= offset_y % text->hline;
		egui_adjust_set_hook(text->vadj, offset_y);
	}
	else if (y > text->height - text->hline) {
		offset_y += y - (text->height - text->hline);
		offset_y += text->hline - offset_y % text->hline;
		if (offset_y > text->total_h - text->height)
			offset_y = text->total_h - text->height;
		egui_adjust_set_hook(text->vadj, offset_y);
	}
}

static eint text_mousemove(eHandle hobj, GalEventMouse *ent)
{
	GuiText  *text = GUI_TEXT_DATA(hobj);
	TextWrap *wrap;
	eint ioff, old;
	eint x, y;

	if (!text->bn_down)
		return 0;

	old = text->offset_y;
	x   = (ent->point.x < 0) ? 0 : ent->point.x;
	if (ent->point.y < 0 && text->offset_y == 0)
		y = 0;
	else
		y = ent->point.y;
	if (y > text->total_h)
		return 0;

	wrap = ycoor_to_wrap(text, y);
	ioff = xcoor_to_ioffset(text, wrap, x);

	if (text->select.e_id == wrap->line->id &&
			text->select.e_ioff == ioff) {
	}
	else if (text->select.s_id != wrap->line->id ||
			text->select.s_ioff != ioff) {

		text->select.e_id   = wrap->line->id;
		text->select.e_ioff = ioff;

		if (!text->is_sel) {
			text->is_sel = true;
			text_hide_cursor(hobj, text);
		}

		if (old == text->offset_y) {
			GalRegion *region = egal_region_copy1(text->sel_rgn);

			set_select_region(hobj, text);
			egal_region_xor(region, region, text->sel_rgn);

			if (REGION_NO_EMPTY(region))
				egui_update_region(hobj, region);

			egal_region_destroy(region);
		}
	}
	else if (text->is_sel) {
		text->is_sel = false;
		if (REGION_NO_EMPTY(text->sel_rgn)) {
			egui_update_region(hobj, text->sel_rgn);
			egal_region_empty(text->sel_rgn);
		}
		egui_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor, true);
	}

	text_set_scrollbar(text, ent->point.y);
	return 0;
}

static void text_set_extent(eHandle hobj, eint w, eint h)
{
}

static void text_set_table_size(eHandle hobj, eint size)
{
}

static void text_set_spacing(eHandle hobj, eint spacing)
{
}

static void text_set_text(eHandle hobj, const echar *text, eint len)
{
}

static eint text_get_text(eHandle hobj, echar *buf, eint size)
{
	GuiText  *text = GUI_TEXT_DATA(hobj);
	TextLine *line = text->line_head;
	eint n = 0;
	if (line) {
		if (line->nchar >= size)
			n = size - 1;
		else
			n = line->nchar;
		e_memcpy(buf, line->chars, n);
		line = line->next;
	}
	while (n < size && line) {
		eint l;
		if (line->nchar + n >= size)
			l = size - n - 1;
		else
			l = line->nchar;
		e_memcpy(buf + n, line->chars, l);
		n += l;
		line = line->next;
	}
	return n;
}

static void text_insert_text(eHandle hobj, const echar *text, eint nchar)
{
	GuiText *t = GUI_TEXT_DATA(hobj);
	insert_text_to_cursor(hobj, GUI_TEXT_DATA(hobj), text, nchar);
	text_set_scrollbar(t, t->cursor.y);
}

static void text_hook_v(eHandle hobj, eHandle hook)
{
	egui_adjust_hook(GUI_TEXT_DATA(hobj)->vadj, hook);
}

static eint text_enter(eHandle hobj, eint x, eint y)
{
	GuiText   *text = GUI_TEXT_DATA(hobj);
	GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
	egal_window_set_cursor(wid->window, text->gal_cursor);
	return 0;
}

static eint text_leave(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	egal_window_set_cursor(wid->window, 0);
	return 0;
}

static void text_init_orders(eGeneType new, ePointer this)
{
	GuiCharOrders    *c = e_genetype_orders(new, GTYPE_CHAR);
	GuiFontOrders    *f = e_genetype_orders(new, GTYPE_FONT);
	GuiHookOrders    *h = e_genetype_orders(new, GTYPE_HOOK);
	GuiEventOrders   *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders  *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiLayoutOrders  *l = e_genetype_orders(new, GTYPE_LAYOUT);
	GuiStringsOrders *s = e_genetype_orders(new, GTYPE_STRINGS);
	eCellOrders      *o = e_genetype_orders(new, GTYPE_CELL);

	text_char_keydown = e->keydown;

	w->resize         = text_resize;
	w->expose         = text_expose;
	w->expose_bg      = text_expose_bg;
	w->configure      = text_configure;

	e->enter          = text_enter;
	e->leave          = text_leave;
	e->focus_in       = text_focus_in;
	e->focus_out      = text_focus_out;
	e->keydown        = text_keydown;
	e->lbuttonup      = text_lbuttonup;
	e->mousemove      = text_mousemove;
	e->lbuttondown    = text_lbuttondown;

	s->insert_text    = text_insert_text;
	s->set_text       = text_set_text;
	s->get_text       = text_get_text;

	l->set_spacing    = text_set_spacing;
	l->set_extent     = text_set_extent;
	l->set_table_size = text_set_table_size;

	c->achar          = text_char;
	f->set_font       = text_set_font;

	h->hook_v         = text_hook_v;
	o->init           = text_init;
}

static inline eint get_space_width(GalFont font)
{
	GalGlyph glyph;
	egal_get_glyph(font, e_utf8_get_char((echar *)" "), &glyph);
	return glyph.w;
}

static void text_set_cursor(eHandle hobj,
		GuiText  *text, TextLine *line,
		TextWrap *wrap, eint ioff, bool set)
{
	text_cursor_setpos(hobj, text, line, wrap, ioff, set);
	text_set_scrollbar(text, text->cursor.y);
	text_show_cursor(hobj, text, line, wrap, ioff, false);
}

static eint text_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	if (text->font == 0)
		return 0;

	wid->rect.w  = resize->w;
	wid->rect.h  = resize->h;
	text->width  = resize->w;
	text->height = resize->h;
	text_hide_cursor(hobj, text);
	text_wrap(hobj, text);
	if (text->cursor.line)
		text_set_cursor(hobj, text, text->cursor.line, NULL, text->cursor.ioff, false);

	return 0;
}

static eint text_configure(eHandle hobj, GuiWidget *wid, GalEventConfigure *conf)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	if (text->font == 0) {
		text->font  = egal_default_font();
		text->hline = egal_font_height(text->font) + text->spacing;
		text->table_w = get_space_width(text->font)  * text->space_mul;
		if (text->nchar == 0)
			text_line_insert(text, _("\n"), 1);
		text_wrap(hobj, text);
		text_cursor_setpos(hobj, text, text->line_head, NULL, 0, true);
	}
	return 0;
}

static void backspace_from_cursor(eHandle hobj, GuiText *text);
static eint text_char(eHandle hobj, echar c)
{
	GuiText      *text = GUI_TEXT_DATA(hobj);
	TextCursor *cursor = &text->cursor;
	const echar *buf;
	eint n;

	if (text->only_read)
		return -1;

	if (c == '\b') {
		backspace_from_cursor(hobj, text);
		text_set_cursor(hobj, text, cursor->line, cursor->wrap, cursor->ioff, false);
		return 0;
	}
	else {
		buf = &c;
		n = 1;
	}

	insert_text_to_cursor(hobj, text, buf, n);
	text_set_cursor(hobj, text, cursor->line, cursor->wrap, cursor->ioff, false);

	return -1;
}

static void cursor_move_up(eHandle hobj, GuiText *text)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *line   = cursor->line;
	TextWrap   *wrap   = NULL;
	eint ioff;

	if (cursor->wrap->prev) {
		wrap = cursor->wrap->prev;
		ioff = xcoor_to_ioffset(text, wrap, cursor->offset_x);
	}
	else if (line->prev) {
		line = line->prev;
		wrap = line->wrap_tail;
		ioff = xcoor_to_ioffset(text, wrap, cursor->offset_x);
	}
	else
		return;

	text_set_cursor(hobj, text, line, wrap, ioff, false);
}

static void cursor_move_down(eHandle hobj, GuiText *text)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *line   = cursor->line;
	TextWrap   *wrap;
	eint ioff;

	if (cursor->wrap->next) {
		wrap = cursor->wrap->next;
		ioff = xcoor_to_ioffset(text, wrap, cursor->offset_x);
	}
	else if (line->next) {
		line = line->next;
		wrap = &line->wrap_head;
		ioff = xcoor_to_ioffset(text, wrap, cursor->offset_x);
	}
	else
		return;

	text_set_cursor(hobj, text, line, wrap, ioff, false);
}

static void cursor_move_left(eHandle hobj, GuiText *text)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *line   = cursor->line;
	eint ioff;
	
	if (cursor->ioff > 0) {
		ioff = cursor->ioff - 1;
	}
	else if (line->prev) {
		line = line->prev;
		ioff = line->nichar - 1;
	}
	else
		return;

	text_set_cursor(hobj, text, line, NULL, ioff, true);
}

static void cursor_move_right(eHandle hobj, GuiText *text)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *line   = cursor->line;
	eint len, ioff;

	if (line->is_lf)
		len = line->nichar - 1;
	else
		len = line->nichar;

	if (cursor->ioff < len)
		ioff = cursor->ioff + 1;
	else if (line->next) {
		line = line->next;
		ioff = 0;
	}
	else
		return;

	text_set_cursor(hobj, text, line, NULL, ioff, true);
}

static inline void text_cancel_select(eHandle hobj, GuiText *text)
{
	if (text->is_sel) {
		text->is_sel = false;
		egui_update_region(hobj, text->sel_rgn);
		egal_region_empty(text->sel_rgn);
	}
}

static eint text_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	switch (ent->code) {
		case GAL_KC_Up:
			text_cancel_select(hobj, text);
			cursor_move_up(hobj, text);
			break;
		case GAL_KC_Down:
			text_cancel_select(hobj, text);
			cursor_move_down(hobj, text);
			break;
		case GAL_KC_Left:
			text_cancel_select(hobj, text);
			cursor_move_left(hobj, text);
			break;
		case GAL_KC_Right:
			text_cancel_select(hobj, text);
			cursor_move_right(hobj, text);
			break;
		case GAL_KC_Tab:
			if (text->only_read)
				return 0;
		default:
			text_char_keydown(hobj, ent);
	}
	return -1;
}

static TextLine *y_offset_to_line(GuiText *text)
{
	TextLine *top;
	TextLine *line = text->line_head;

	 do  {
		top  = line;
		line = line->next;
	 } while (line && line->offset_y < text->offset_y);

	 return top;
}

static eint text_adjust_update(eHandle hobj, eint value)
{
	eHandle   obj = GUI_ADJUST_DATA(hobj)->owner;
	GuiText *text = GUI_TEXT_DATA(obj);

	if (text->offset_y == value)
		return 0;

	text->offset_y = value;
	text->top      = y_offset_to_line(text);
	if (!text->is_sel)
		text_hide_cursor(obj, text);
	else
		set_select_region(obj, text);

	egui_update(obj);

	if (!text->is_sel && text->cursor.line) {
		TextCursor *cursor = &text->cursor;
		text_show_cursor(obj, text, cursor->line, cursor->wrap, cursor->ioff, false);
	}
    return 0;
}

static eint text_init_data(eHandle hobj, ePointer this)
{
	GuiText *text = this;

	text->gal_cursor  = egal_cursor_new(GAL_XTERM);

	text->vadj = egui_adjust_new(0, 1.0, 1.0, 0, 0);
	egui_adjust_set_owner(text->vadj, hobj);
	e_signal_connect(text->vadj, SIG_ADJUST_UPDATE, text_adjust_update);

	text->width       = 0;
	text->height      = 0;
	text->nchar       = 0;
	text->nline       = 0;
	text->offset_x    = 0;
	text->offset_y    = 0;
	text->spacing     = 5;
	text->space_mul   = 4;
	text->line_head   = NULL;
	text->line_tail   = NULL;
	text->top         = NULL;
	text->cursor.line = NULL;
	text->cursor.ioff = 0;
	text->cursor.show = false;
	text->is_sel      = false;
	text->bn_down     = false;

	text->sel_rgn = egal_region_new();

	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);

	return 0;
}

static void text_line_free(TextLine *line)
{
	if (line->nchar > 0)
		e_free(line->chars);
	e_slice_free(TextLine, line);
}

static void text_free_data(eHandle hobj, ePointer this)
{
	TextLine *line = ((GuiText *)this)->line_head;
	while (line) {
		TextLine *t = line;
		line = t->next;
		text_line_free(t);
	}
}

static void text_set_font(eHandle hobj, GalFont font)
{
	GuiText *text = GUI_TEXT_DATA(hobj);
	text->font    = font;
	text->hline   = egal_font_height(font) + text->spacing;
	text->table_w = get_space_width(font) * text->space_mul;
	text_wrap(hobj, text);
}

static void text_wrap(eHandle hobj, GuiText *text)
{
	TextLine *line = text->line_head;
	eint y = 0;

	while (line) {
		line->offset_y = y;
		y += text_wrap_line(text, line);
		line = line->next;
	}
	text->total_h = y;

	if (text->total_h > text->height) {
		egui_adjust_reset_hook(text->vadj,
				text->offset_y, text->height,
				text->total_h, text->hline, 0);
	}
	else
		egui_adjust_reset_hook(text->vadj, 0, 1, 1, 1, 0);
}

static void draw_wrap(GalDrawable drawable, GalPB pb,
		GuiText *text, TextWrap *wrap, 
		GalGlyph *glyphs, echar *underlines, GalRect *prc, eint y)
{
	eint i, j;
	eint w = 0, x = 0;
	eint len = 0;

	if (prc->x == 0 && prc->w >= wrap->width) {
		egal_draw_glyphs(drawable, pb,
				text->font, prc, 0, y, glyphs, wrap->nichar);

		i   = 0;
		x   = 0;
		w   = 0;
		len = wrap->nichar;
		while (len > 0 && !underlines[i]) {
			x += glyphs[i].w;
			i ++;
			len--;
		}
		while (len > 0 && underlines[i]) {
			w += glyphs[i].w;
			i ++;
			len--;
		}
		egal_draw_line(drawable, pb, x, y + text->cursor.h + 1, x + w, y + text->cursor.h + 1);
		return;
	}

	for (i = 0; i < wrap->nichar; i++) {
		w += glyphs[i].w;
		if (w > prc->x + text->offset_x) {
			x = w - text->offset_x - glyphs[i].w;
			len = 1;
			break;
		}
	}

	for (j = i+1; j < wrap->nichar; j++) {
		if (w + glyphs[j].x >
				prc->x + prc->w + text->offset_x)
			break;
		w += glyphs[j].w;
		len++;
	}

	egal_draw_glyphs(drawable, pb,
			text->font, prc, x, y, &glyphs[i], len);

	while (len > 0 && underlines[i] == 0) {
		x += glyphs[i].w;
		i ++;
		len--;
	}
	w = 0;
	while (len > 0 && underlines[i]) {
		w += glyphs[i].w;
		i ++;
		len--;
	}
	egal_draw_line(drawable, pb, x, y + text->cursor.h + 1, x + w, y + text->cursor.h + 1);
}

static void draw_text(GalDrawable drawable, GalPB pb, GuiText *text, GalRect *prc)
{
	TextLine *line = text->top ? text->top : text->line_head;

	eint x = -text->offset_x, y;
	eint h =  text->hline;

	while (line) {
		y = line->offset_y - text->offset_y;
		GalRect lrc = {x, y, line->max_w, line->nwrap * h};

		if (y >= prc->y + prc->h)
			break;

		if (egal_rect_is_intersect(&lrc, prc)) {
			TextWrap *wrap = &line->wrap_head;
			GalRect tmp;
			lrc.h = h;
			while (wrap) {
				lrc.w = wrap->width;
				if (egal_rect_intersect(&tmp, &lrc, prc))
					draw_wrap(drawable, pb, text, wrap, 
							line->glyphs + wrap->begin, 
							line->underlines + wrap->begin, 
							&tmp, y);
				y    += text->hline;
				lrc.y = y;
				wrap  = wrap->next;
			}
		}

		line = line->next;
	}
}

static eint text_expose_bg(eHandle hobj, GuiWidget *widget, GalEventExpose *expose)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	if (text->is_sel) {
		GalRegion    *region  = egal_region_rect(&expose->rect);
		GalRegionSec *head;

		egal_region_intersect(region, region, text->sel_rgn);

		egal_set_foreground(expose->pb, 0x9a958d);
		head = region->head;
		while (head) {
			eint x = head->sec.x1;
			eint y = head->sec.y1;
			eint w = head->sec.x2 - x;
			eint h = head->sec.y2 - y;
			egal_fill_rect(widget->drawable, expose->pb, x, y, w, h);
			head = head->next;
		}
		egal_region_destroy(region);

		region = egal_region_rect(&expose->rect);
		egal_region_subtract(region, region, text->sel_rgn);

		egal_set_foreground(expose->pb, widget->bg_color);
		head = region->head;
		while (head) {
			eint x = head->sec.x1;
			eint y = head->sec.y1;
			eint w = head->sec.x2 - x;
			eint h = head->sec.y2 - y;
			egal_fill_rect(widget->drawable, expose->pb, x, y, w, h);
			head = head->next;
		}
		egal_region_destroy(region);
	}
	else {
		egal_set_foreground(expose->pb, widget->bg_color);
		egal_fill_rect(widget->drawable, expose->pb,
				expose->rect.x, expose->rect.y, expose->rect.w, expose->rect.h);
	}

	return 0;
}

static eint text_expose(eHandle hobj, GuiWidget *widget, GalEventExpose *expose)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	egal_set_foreground(expose->pb, widget->fg_color);
	draw_text(widget->drawable, widget->pb, text, &expose->rect);

	if (GUI_STATUS_FOCUS(hobj) && text->cursor.show)
		egui_show_cursor(widget->drawable, &text->cursor, true);

	return 0;
}

static TextLine *textline_new(void)
{
	TextLine *line = e_slice_new(TextLine);
	line->nichar      = 0;
	line->chars       = NULL;
	line->nwrap       = 0;
	line->is_lf       = false;
	line->nchar       = 0;
	line->id          = 0;
	line->over        = 0;
	line->lack        = 0;
	line->max_w       = 0;
	line->offset_y    = 0;
	line->load_nchar  = 0;
	line->load_nichar = 0;
	line->next        = NULL;
	line->prev        = NULL;
	line->glyphs      = NULL;
	line->wrap_top    = NULL;
	line->coffsets    = NULL;
	line->underlines  = NULL;
	line->wrap_tail   = NULL;
	e_memset(&line->wrap_head, 0, sizeof(line->wrap_head));
	line->wrap_head.line = line;

	return line;
}

static TextWrap *textwrap_new(TextLine *line)
{
	TextWrap *wrap = e_slice_new(TextWrap);
	wrap->line    = line;
	wrap->coffset = 0;
	wrap->nchar   = 0;
	wrap->nichar  = 0;
	wrap->id      = 0;
	wrap->width   = 0;
	wrap->next    = NULL;
	wrap->prev    = NULL;
	return wrap;
}

static void textwrap_free(TextWrap *wrap)
{
	e_slice_free(TextWrap, wrap);
}

#define ioffset_char(line, ioff) line->chars[line->coffsets[ioff]]

static bool ioffset_line_end(TextLine *line, eint ioff)
{
	if (line->chars[line->coffsets[ioff]] == '\n'
			|| ioff >= line->nichar)
		return true;
	return false;
}

static bool ioffset_is_latin(TextLine *line, eint ioff)
{
	echar c = line->chars[line->coffsets[ioff]];
	if (c < 128
			&& c != ' '
			&& c != '\t'
			&& !ioffset_line_end(line, ioff))
		return true;
	return false;
}

static eint latin_word_width(TextLine *line, eint o, eint *j, eint max_w)
{
	eint i, w = 0;

	for (i = 0; ioffset_is_latin(line, o + i); i++) {
		eint _w = line->glyphs[o + i].w;
		if (w + _w > max_w)
			break;
		w += _w;
	}

	if (j) *j = i;

	return w;
}

static void line_load_glyphs(TextLine *line, GalFont font)
{
	eint nchar, ichar, i, n;
	const echar *p;

	if (line->nichar == 0 || line->load_nichar == line->nichar)
		return;

	n = line->nichar;
	if (line->load_nichar > 0) {
		line->glyphs     = e_realloc(line->glyphs, sizeof(GalGlyph) * n);
		line->coffsets   = e_realloc(line->coffsets, sizeof(eint)  * n + 1);
		line->underlines = e_realloc(line->coffsets, n);
	}
	else {
		line->glyphs     = e_malloc(sizeof(GalGlyph) * n);
		line->coffsets   = e_malloc(sizeof(eint)  * n + 1);
		line->underlines = e_malloc(n);
	}
	e_memset(line->underlines, 0, n);

	p     = line->chars;
	i     = line->load_nichar;
	nchar = line->load_nchar;
	for ( ; i < n; i++) {
		line->coffsets[i] = nchar;
		ichar  = e_utf8_get_char(p + nchar);
		nchar += e_utf8_char_len(p + nchar);
		egal_get_glyph(font, ichar, &line->glyphs[i]);
	}

	line->load_nchar  = line->nchar - line->over;
	line->coffsets[n] = line->load_nchar;
	line->load_nichar = n;
}

static void wrap_clear_after(TextCursor *cursor, TextLine *line, TextWrap *wrap)
{
	while (wrap) {
		TextWrap *t = wrap;
		wrap = wrap->next;
		textwrap_free(t);
		if (cursor->wrap == t)
			cursor->wrap = NULL;
		line->nwrap--;
	}
}

static eint __text_wrap_line(GuiText *text, TextLine *line, TextWrap *wrap)
{
	eint box_w = text->width;
	eint nwrap = line->nwrap;
	eint i;

	if (!line->wrap_tail) {
		wrap = &line->wrap_head;
		line->wrap_tail = wrap;
		line->nwrap = 1;
		wrap->begin = 0;
	}
	else if (!wrap)
		wrap = &line->wrap_head;

	wrap->width_old = wrap->width;
	wrap->begin_old = wrap->begin;
	i = wrap->begin;
	wrap->nchar  = 0;
	wrap->nichar = 0;
	wrap->width  = 0;

	while (!ioffset_line_end(line, i)) {
		eint w, j = 1;

		if (ioffset_is_latin(line, i)) {
			w = latin_word_width(line, i, &j, box_w);
		}
		else if (ioffset_char(line, i) == '\t') {
			if (wrap->width == box_w) {
				line->glyphs[i].w = text->table_w;
				w = text->table_w;
			}
			else {
				w = text->table_w - wrap->width % text->table_w;
				if (wrap->width + w > box_w)
					w = box_w - wrap->width;
				line->glyphs[i].w = w;
			}
		}
		else
			w = line->glyphs[i].w;

		if (wrap->width + w > box_w) {
			if (line->max_w < wrap->width)
				line->max_w = wrap->width;

			if (!wrap->next) {
				TextWrap *new = textwrap_new(line);
				new->id    = wrap->id + 1;
				new->prev  = wrap;
				wrap->next = new;
				line->wrap_tail = new;
				line->nwrap++;
			}
			wrap = wrap->next;

			wrap->width_old = wrap->width;
			wrap->begin_old = wrap->begin;
			wrap->nchar  = 0;
			wrap->nichar = 0;
			wrap->width  = 0;
			wrap->begin  = i;
			wrap->coffset = line->coffsets[i];
		}
		wrap->nchar += line->coffsets[i + j] - line->coffsets[i];
		wrap->nichar+= j;
		wrap->width += w;
		i += j;
	}

	if (line->max_w < wrap->width)
		line->max_w = wrap->width;

	if (wrap->next) {
		wrap_clear_after(&text->cursor, line, wrap->next);
		wrap->next      = NULL;
		line->wrap_tail = wrap;
	}

	return line->nwrap - nwrap;
}

static eint text_wrap_line(GuiText *text, TextLine *line)
{
	line_load_glyphs(line, text->font);

	__text_wrap_line(text, line, NULL);

	return line->nwrap * text->hline;
}

static eint text_line_insert(GuiText *text, const echar *chars, eint clen)
{
	const echar *p = chars;
	eunichar ichar = 0;
	eint       len = 0;
	TextLine *line;

	line = text->line_tail;
	if (!line || line->is_lf) {
		line = textline_new();
		line->id = text->nline;
		if (!text->line_head) {
			text->line_head = line;
			text->line_tail = line;
		}
		else {
			line->prev = text->line_tail;
			text->line_tail->next = line;
			text->line_tail = line;
		}
	}
	else if (line->lack > 0) {
		if (clen >= line->lack) {
			len   = line->lack;
			line->lack = 0;
			line->over = 0;
			line->nichar++;
		}
		else {
			len = clen;
			line->lack -= len;
			line->over += len;
		}
	}

	if (clen == len)
		goto finish;

	do {
		eint l = e_utf8_char_len(p + len);
		if (len + l > clen) {
			line->lack = len + l - clen;
			line->over = l - line->lack;
			len        = clen;
			break;
		}
		if (l == 1)
			ichar = p[len];
		else
			ichar = 0;
		len  += l;
		line->nichar++;
	} while (ichar != '\n' && len < clen);

	if (ichar == '\n') {
		line->is_lf = true;
		text->nline ++;
	}

finish:
	if (line->nchar > 0)
		line->chars = e_realloc(line->chars, line->nchar + len);
	else
		line->chars = e_malloc(len);

	e_memcpy(line->chars + line->nchar, p, len);
	line->nchar += len;
	text->nchar += len;

	return len;
}

static ePointer insert_data(ePointer chars, eint c_size,
		eint offset, ePointer data, eint d_size, eint nmemb)
{
	eint c = c_size * nmemb;
	eint d = d_size * nmemb;
	eint o = offset * nmemb;

	if (!chars)
		chars = e_malloc(c + d);
	else {
		chars = e_realloc(chars, c + d);
		e_memmove(chars + o + d, chars + o, c - o);
	}

	if (data) e_memcpy(chars + o, data, d);

	return chars;
}

#define INSERT_MAX		100
typedef struct {
	eint coff;
	eint nchar;
	eint nichar;
	bool is_lf;
} LineNode;

static void text_update_area(eHandle hobj, GuiText *text, TextCursor *cursor,
		TextLine *line, eint inc, eint nichar, bool text_sel, eint bear_x)
{
	if (text->old_y != text->offset_y) {
		text->old_y  = text->offset_y;
		egui_update(hobj);
		return;
	}

	GuiWidget *widget = GUI_WIDGET_DATA(hobj);

	TextWrap *wrap = ioffset_to_wrap(cursor->line, cursor->ioff);
	TextWrap *prev = cursor->wrap ? cursor->wrap->prev : wrap;
	TextWrap *next = cursor->wrap ? cursor->wrap->next : NULL;

	GalRect rc, sel_rc;

	if (wrap == next && nichar > 0) {
		wrap  = cursor->wrap;
		rc.x  = wrap->width - text->offset_x;
		rc.y  = cursor->y;
		rc.w  = wrap->width_old - wrap->width;
	}
	else if (prev && prev->width_old != prev->width) {
		rc.y  = cursor->y - text->hline;
			rc.x  = (prev->width < prev->width_old) ? prev->width : prev->width_old;
			rc.x -= text->offset_x;
			rc.w  = prev->width_old - prev->width;
		wrap  = prev;
	}
	else {
		eint min_w = (wrap->width < wrap->width_old) ? wrap->width: wrap->width_old;
		if (min_w < cursor->x)
			rc.x = min_w;
		else
			rc.x = cursor->x;
		rc.y  = cursor->y;
		rc.w  = (wrap->width > wrap->width_old) ? wrap->width : wrap->width_old;
		rc.w -= rc.x;
	}

	if (rc.w < 0)
		rc.w = -rc.w;
	rc.h = text->hline;

	if (bear_x < -1) {
		rc.x += bear_x;
		rc.w -= bear_x;
	}
	egui_update_rect(hobj, &rc);
	if (text_sel) {
		sel_rc   = rc;
		sel_rc.x = rc.x + rc.w;
		sel_rc.w = widget->rect.w - rc.w;
	}

	rc.x  = 0;
	rc.y += text->hline;
	if (inc != 0) {
		rc.w = text->width;
		if (text->total_h < text->height) {
			if (text->update_h > text->total_h)
				rc.h = text->update_h - rc.y;
			else
				rc.h = text->total_h  - rc.y;
		}
		else
			rc.h = text->height - rc.y;
	}
	else if (cursor->line->id != line->id) {
		TextLine *l = cursor->line;
		rc.w = widget->rect.w;
		rc.h = 0;
		while (l && l->id <= line->id) {
			rc.h += text->hline * l->nwrap;
			l = l->next;
		}
	}
	else if (wrap->next) {
		wrap = wrap->next;
		rc.h = 0;
		do {
			if (nichar != 0
					&& wrap->begin - nichar == wrap->begin_old
					&& wrap->width == wrap->width_old)
				break;
			if (rc.w < wrap->width)
				rc.w = wrap->width;
			if (wrap->width_old == 0)
				rc.w = widget->rect.w;
			else if (rc.w < wrap->width_old)
				rc.w = wrap->width_old;
			rc.h += text->hline;
			wrap  = wrap->next;
		} while (wrap);
	}
	else return;

	if (text_sel)
		egui_update_rect(hobj, &sel_rc);
	egui_update_rect(hobj, &rc);
}

static void line_insert_after(GuiText *text, TextLine *line, TextLine *insert)
{
	insert->id   = line->id + 1;
	insert->prev = line;
	insert->next = line->next;
	if (line->next)
		line->next->prev = insert;
	else
		text->line_tail  = insert;
	line->next = insert;
	text->nline++;
}

static void adjust_line_series(GuiText *text, TextLine *begin)
{
	if (!begin) {
		begin = text->line_head;
		begin->offset_y = 0;
		begin->id = 0;
	}

	if (begin->next) {
		TextLine *p = begin;
		TextLine *n = p->next;
		while (n) {
			n->offset_y = p->offset_y + text->hline * p->nwrap;
			n->id = p->id + 1;
			p = n;
			n = n->next;
			text->line_tail = p;
		}
	}
	else
		text->line_tail = begin;

	text->total_h = text->line_tail->offset_y +
		text->hline * text->line_tail->nwrap;
	if (text->total_h > text->height) {
		if (text->offset_y > text->total_h - text->height) {
			text->old_y    = text->offset_y;
			text->offset_y = text->total_h - text->height;
			text->top = y_offset_to_line(text);
		}
		egui_adjust_reset_hook(text->vadj,
				text->offset_y, text->height,
				text->total_h, text->hline, 0);
	}
	else
		egui_adjust_reset_hook(text->vadj, 0, 1, 1, 1, 0);
}

static void text_insert_line_node(eHandle hobj, GuiText *text, const echar *chars, LineNode *nds, eint node_num)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *trline = NULL;
	TextLine   *pline  = NULL;
	TextLine   *line   = cursor->line;

	eint count    = 0;
	eint inc_wrap = 0;
	eint del_nr   = 0;
	bool text_sel = false;
	eint bear_x   = 0;

	text->update_h = text->height;
	if (text->is_sel) {
		text_sel     = true;
		text->is_sel = false;
		egal_region_empty(text->sel_rgn);
		inc_wrap = text_delete_select(hobj, text, &del_nr, &bear_x);
		cursor   = &text->cursor;
		line     = cursor->line;
	}
	else {
		bear_x = line->glyphs[cursor->ioff].x;
	}

	if (nds->is_lf) {
		eint coff = line->coffsets[cursor->ioff];
		eint i;

		trline = textline_new();
		trline->is_lf  = line->is_lf;
		trline->nchar  = line->nchar - coff;
		trline->nichar = line->nichar - cursor->ioff;

		trline->chars      = e_malloc(trline->nchar);
		trline->glyphs     = e_malloc(trline->nichar * sizeof(GalGlyph));
		trline->coffsets   = e_malloc((trline->nichar + 1) * sizeof(eint));
		trline->underlines = e_malloc(trline->nichar);
		e_memcpy(trline->chars, line->chars + coff, trline->nchar);
		e_memcpy(trline->glyphs, line->glyphs + cursor->ioff, trline->nichar * sizeof(GalGlyph));
		e_memcpy(trline->coffsets, line->coffsets + cursor->ioff, (trline->nichar + 1) * sizeof(eint));
		e_memcpy(trline->underlines, line->underlines + cursor->ioff, trline->nichar);

		for (i = 0; i < trline->nichar + 1; i++)
			trline->coffsets[i] -= coff;
	}

	do {
		LineNode *node = &nds[count];

		const echar *p;
		eint i, ioff, coff, nchar;
		bool is_underline;

		if (count == 0) {
			ioff = cursor->ioff;
			coff = line->coffsets[ioff];
			if (node->is_lf) {
				line->nchar  = coff;
				line->nichar = ioff;
			}
		}
		else if (count + 1 == node_num && !node->is_lf) {
			line_insert_after(text, line, trline);
			pline  = line;
			line   = trline;
			ioff   = 0;
			coff   = 0;
			trline = NULL;
		}
		else if (count == node_num && trline) {
			line_insert_after(text, line, trline);
			pline  = line;
			line   = trline;
			trline = NULL;
			goto cont;
		}
		else {
			TextLine *new = textline_new();
			line_insert_after(text, line, new);
			pline  = line;
			line   = new;
			ioff   = 0;
			coff   = 0;
			line->is_lf = true;
		}

		line->chars   = insert_data(line->chars,
						line->nchar, coff,
						(ePointer)(chars + node->coff),
						node->nchar,
						sizeof(echar));
		line->glyphs  = insert_data(line->glyphs,
						line->nichar, ioff,
						NULL, node->nichar,
						sizeof(GalGlyph));
		line->coffsets = insert_data(line->coffsets,
						line->nichar + 1, ioff,
						NULL, node->nichar,
						sizeof(eint));
		line->underlines = insert_data(line->underlines,
						line->nichar, ioff,
						NULL, node->nichar,
						sizeof(echar));

		for (i = 0; i < line->nichar - ioff; i++)
			line->coffsets[ioff + node->nichar + i] += node->nchar;

		line->nchar  += node->nchar;
		text->nchar  += node->nchar;
		line->nichar += node->nichar;
		line->coffsets[line->nichar] = line->nchar;

		nchar = 0;
		p = chars + node->coff;
		is_underline = (text->is_underline || (ioff > 0 && line->underlines[ioff - 1])) ? 1 : 0;
		for (i = 0; i < node->nichar; i++) {
			eunichar ichar;
			line->coffsets[ioff + i] = coff + nchar;
			ichar  = e_utf8_get_char(p + nchar);
			nchar += e_utf8_char_len(p + nchar);
			egal_get_glyph(text->font, ichar, &line->glyphs[ioff + i]);
			if (is_underline)
				line->underlines[ioff + i] = 1;
			else
				line->underlines[ioff + i] = 0;
		}

		if (count == 0 && line->glyphs[ioff].x < bear_x)
			bear_x = line->glyphs[ioff].x;
cont:
		if (pline)
			line->offset_y = pline->offset_y + pline->nwrap * text->hline;

		if (cursor->wrap && count == 0) {
			TextWrap *wrap = cursor->wrap;
			if (wrap->prev)
				wrap  = wrap->prev;
			inc_wrap += __text_wrap_line(text, line, wrap);
		}
		else
			inc_wrap += __text_wrap_line(text, line, NULL);

	} while (++count < node_num || trline);

	if (inc_wrap || nds->is_lf)
		adjust_line_series(text, line);

	text_hide_cursor(hobj, text);
	if (cursor->line->id == 0)
		text->top = cursor->line;
	text_update_area(hobj, text, cursor, line, inc_wrap, nds->nichar + del_nr, text_sel, bear_x);
	if (!nds->is_lf)
		text_show_cursor(hobj, text, line, NULL, cursor->ioff + nds->nichar, true);
	else if (count > node_num)
		text_show_cursor(hobj, text, line, NULL, 0, true);
	else
		text_show_cursor(hobj, text, line, NULL, nds[node_num-1].nichar, true);
}

static void insert_text_to_cursor(eHandle hobj, GuiText *text, const echar *chars, eint clen)
{
	eint max = INSERT_MAX;
	LineNode  tmp[max];
	LineNode *node_head = tmp;

	eint         n = 0;
	const echar *p = chars;

	e_memset(tmp, 0, sizeof(tmp));
	do {
		LineNode *node;
		eint nchar = 0;
		eint ichar = 0;

		if (n >= max) {
			max *= 2;
			if (node_head == tmp) {
				node_head = e_malloc(max);
				e_memcpy(node_head, tmp, sizeof(tmp));
			}
			else {
				node_head = e_realloc(node_head, max);
			}
		}

		node = &node_head[n];
		node->coff = p - chars;
		do {
			ichar  = e_utf8_get_char(p + nchar);
			nchar += e_utf8_char_len(p + nchar);
			node->nichar++;
		} while (ichar != '\n' && nchar < clen);

		if (ichar == '\n')
			node->is_lf = true;

		p    += nchar;
		clen -= nchar;
		node->nchar = nchar;
		nchar = 0;
		n++;
	} while (clen > 0);

	text_insert_line_node(hobj, text, chars, tmp, n);

	if (node_head != tmp)
		e_free(node_head);
}

static inline void __del_line_list(TextLine *prev, TextLine *next)
{
	if (prev) prev->next = next;
	if (next) next->prev = prev;
}

static inline void del_line_list(TextLine *line)
{
	__del_line_list(line->prev, line->next);
}

static void line_clear_wrap(TextLine *line)
{
	TextWrap *wrap = line->wrap_head.next;
	while (wrap) {
		TextWrap *t = wrap;
		wrap = wrap->next;
		textwrap_free(t);
		line->nwrap--;
	}
}

static eint del_line_series(GuiText *text, TextLine *begin, TextLine *end)
{
	TextLine *n = begin;
	TextLine *p, *e;
	eint nwrap = 0;

	p = begin->prev;
	if (end)
		e = end->next;
	else
		e = begin->next;
	do {
		TextLine *t = n;
		n = n->next;
		del_line_list(t);
		text->nchar -= t->nchar;
		text->nline--;

		nwrap += t->nwrap;
		e_free(t->chars);
		e_free(t->glyphs);
		e_free(t->coffsets);
		e_free(t->underlines);
		line_clear_wrap(t);
		e_slice_free(TextLine, t);
	} while (n && end && n->id <= end->id);

	if (!p)
		text->line_head = e;

	return nwrap;
}

static eint _text_delete_area(eHandle hobj, GuiText *text,
		eint sid, eint sioff, eint eid, eint eioff,
		eint *del_nr, eint *bear_x, bool is_wrap)
{
	TextLine  *sl = find_line_by_id(text, sid);
	eint nichar, nchar;
	eint inc_wrap = 0;

	if (bear_x)
		*bear_x = sl->glyphs[sioff].x;

	if (sid == eid) {
		eint scoff = sl->coffsets[sioff];
		eint ecoff = sl->coffsets[eioff];
		nichar = eioff - sioff;
		nchar  = ecoff - scoff;
		eint i;

		if (del_nr)
			*del_nr = -nichar;

		e_memmove(sl->chars + scoff, sl->chars + ecoff, sl->nchar - ecoff);
		e_memmove(sl->glyphs + sioff, sl->glyphs + eioff, (sl->nichar - eioff) * sizeof(GalGlyph));
		e_memmove(sl->coffsets + sioff, sl->coffsets + eioff, (sl->nichar - eioff + 1) * sizeof(eint));
		e_memmove(sl->underlines + sioff, sl->underlines + eioff, sl->nichar - eioff);

		for (i = 0; i < sl->nichar - eioff + 1; i++)
			sl->coffsets[sioff + i] -= nchar;

		sl->nichar    -= nichar;
		sl->nchar     -= nchar;
		text->nchar   -= nchar;
		sl->chars      = e_realloc(sl->chars, sl->nchar);
		sl->glyphs     = e_realloc(sl->glyphs, sl->nichar * sizeof(GalGlyph));
		sl->coffsets   = e_realloc(sl->coffsets, (sl->nichar + 1) * sizeof(eint));
		sl->underlines = e_realloc(sl->underlines, sl->nichar);
	}
	else if ((eid - sid) == 1 && (sl->nichar == 1 || sl->next->nichar == 1)) {
		if (sl->nichar == 1) {
			sl = sl->next;
			sl->id--;
			sl->offset_y = sl->prev->offset_y;
			inc_wrap -= del_line_series(text, sl->prev, NULL);
		}
		else
			inc_wrap -= del_line_series(text, sl->next, NULL);
	}
	else {
		TextLine *el = find_line_by_id(text, eid);
		eint scoff   = sl->coffsets[sioff];
		eint ecoff   = el->coffsets[eioff];
		nichar       = sl->nichar - sioff;
		eint i;

		sl->chars      = e_realloc(sl->chars, scoff + el->nchar - ecoff);
		sl->glyphs     = e_realloc(sl->glyphs, (sioff + el->nichar - eioff) * sizeof(GalGlyph));
		sl->coffsets   = e_realloc(sl->coffsets, (sioff + el->nichar - eioff + 1) * sizeof(eint));
		sl->underlines = e_realloc(sl->underlines, sioff + el->nichar - eioff);

		e_memcpy(sl->chars + scoff, el->chars + ecoff, el->nchar - ecoff);
		e_memcpy(sl->glyphs + sioff, el->glyphs + eioff, (el->nichar - eioff) * sizeof(GalGlyph));
		e_memcpy(sl->coffsets + sioff, el->coffsets + eioff, (el->nichar - eioff + 1) * sizeof(eint));
		e_memcpy(sl->underlines + sioff, el->underlines + eioff, el->nichar - eioff);

		for (i = 0; i < el->nichar - eioff + 1; i++) {
			sl->coffsets[sioff + i] -= ecoff;
			sl->coffsets[sioff + i] += scoff;
		}

		text->nchar -= sl->nchar;
		sl->nichar   = sioff + el->nichar - eioff;
		sl->nchar    = scoff + el->nchar  - ecoff;
		text->nchar += sl->nchar;

		inc_wrap -= del_line_series(text, sl->next, el);
	}

	if (is_wrap) {
		inc_wrap += __text_wrap_line(text, sl, NULL);
		if (inc_wrap != 0 || sid != eid)
			adjust_line_series(text, sl);
	}

	text_cursor_setpos(hobj, text, sl, NULL, sioff, true);

	if (bear_x && sl->glyphs[sioff].x < *bear_x)
		*bear_x = sl->glyphs[sioff].x;

	return inc_wrap;
}

static eint text_delete_select(eHandle hobj, GuiText *text, eint *del_nr, eint *bear_x)
{
	eint sid, sioff, eid, eioff;

	select_normalize(text, &sid, &sioff, &eid, &eioff);

	return _text_delete_area(hobj, text, sid, sioff, eid, eioff, del_nr, bear_x, false);
}

static void backspace_from_cursor(eHandle hobj, GuiText *text)
{
	TextCursor *cursor = &text->cursor;
	TextLine   *line   = cursor->line;
	eint inc_wrap;
	eint nchar    = 0;
	eint bear_x   = 0;
	bool text_sel = false;

	text->update_h = text->total_h;
	if (text->is_sel) {
		text_sel     = true;
		text->is_sel = false;
		egal_region_empty(text->sel_rgn);
		eint sid, sioff, eid, eioff;
		select_normalize(text, &sid, &sioff, &eid, &eioff);
		inc_wrap  = _text_delete_area(hobj, text, sid, sioff, eid, eioff, &nchar, &bear_x, true);
	}
	else if (cursor->ioff == 0) {
		if (!line->prev)
			return;
		inc_wrap = _text_delete_area(hobj, text, 
				line->prev->id, line->prev->nichar - 1, line->id, 0, NULL, &bear_x, true);
	}
	else {
		inc_wrap = _text_delete_area(hobj, text,
				line->id, cursor->ioff - 1, line->id, cursor->ioff, NULL, &bear_x, true);
		nchar = -1;
	}
	if (line != cursor->line)
		line  = cursor->line;
	if (cursor->line->id == 0)
		text->top = cursor->line;
	text_update_area(hobj, text, &text->cursor, line, inc_wrap, nchar, text_sel, bear_x);
	egui_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor, false);
}

static void text_cursor_setpos(eHandle hobj, GuiText *text, TextLine *line, TextWrap *wrap, eint ioff, bool set)
{
	TextCursor *cursor = &text->cursor;

	eint w = 0, h, i;

	if (wrap == NULL)
		wrap = ioffset_to_wrap(line, ioff);

	h = (line->offset_y - text->offset_y) + wrap->id * text->hline;

	for (i = wrap->begin; i < ioff; i++)
		w += line->glyphs[i].w;
	w -= text->offset_x;

	cursor->line = line;
	cursor->wrap = wrap;
	cursor->ioff = ioff;
	egui_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, cursor);
	cursor->x = w;
	cursor->y = h;
	cursor->h = egal_font_height(text->font);

	if (set) cursor->offset_x = w;
}

static void text_show_cursor(eHandle hobj, GuiText *text, TextLine *line, TextWrap *wrap, eint ioff, bool set)
{
	text_cursor_setpos(hobj, text, line, wrap, ioff, set);
	if (GUI_STATUS_FOCUS(hobj))
		egui_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor, false);
}

static inline void text_hide_cursor(eHandle hobj, GuiText *text)
{
	egui_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor);
}

static void egui_hide_cursor(GalDrawable window, TextCursor *cursor)
{
	if (cursor->show) {
		GalPB pb = egal_type_pb(GalPBnor);
		egal_draw_line(window, pb, cursor->x, cursor->y, cursor->x, cursor->y + cursor->h);
		cursor->show = false;
	}
}

static void egui_show_cursor(GalDrawable window, TextCursor *cursor, bool show)
{
	if (show || !cursor->show ||
			cursor->ox != cursor->x ||
			cursor->oy != cursor->y ||
			cursor->oh != cursor->h) {
		GalPB pb = egal_type_pb(GalPBnor);
		if (cursor->ox != cursor->x)
			cursor->ox  = cursor->x;
		if (cursor->oy != cursor->y)
			cursor->oy  = cursor->y;
		if (cursor->oh != cursor->h)
			cursor->oh  = cursor->h;
		egal_draw_line(window, pb,
				cursor->x, cursor->y, cursor->x, cursor->y + cursor->h);
		cursor->show = true;
	}
}

static eint text_init(eHandle hobj, eValist vp)
{
	GuiText   *txt = GUI_TEXT_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	eint w = e_va_arg(vp, eint);
	eint h = e_va_arg(vp, eint);

	wid->bg_color = 0xffffff;
	wid->fg_color = 0;
	wid->min_w = 50;
	wid->min_h = 50;
	wid->rect.w = w;
	wid->rect.h = h;
	txt->width  = w;
	txt->height = h;

	return e_signal_emit(hobj, SIG_REALIZE);
}

void egui_text_set_only_read(eHandle hobj, bool only)
{
	GUI_TEXT_DATA(hobj)->only_read = only;
}

eHandle egui_text_new(eint w, eint h)
{
	return e_object_new(GTYPE_TEXT, w, h);
}

void egui_text_clear(eHandle hobj)
{
	GuiText  *text = GUI_TEXT_DATA(hobj);

	_text_delete_area(hobj, text, 0, 0,
			text->line_tail->id, text->line_tail->nichar - 1, NULL, NULL, true);

	if (text->is_sel) {
		text->is_sel = false;
		egal_region_empty(text->sel_rgn);
	}
	if (text->cursor.line->id == 0)
		text->top = text->cursor.line;

	egui_update(hobj);
	egui_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, &text->cursor, false);
}

void egui_text_append(eHandle hobj, const echar *chars, eint nchar)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	if (text->is_sel) {
		text->is_sel = false;
		egal_region_empty(text->sel_rgn);
	}

	while (nchar > 0) {
		eint n = text_line_insert(text, chars, nchar);
		chars += n;
		nchar -= n;
	}

	if (text->font)
		text_wrap(hobj, text);
}

static void set_underline(GuiText *text)
{
	TextLine *l;
	eint sid, sioff, eid, eioff;
	eint j;

	select_normalize(text, &sid, &sioff, &eid, &eioff);

	j = sid;
	l = find_line_by_id(text, sid);
	do {
		eint e, i;
		if (j == sid)
			i = sioff;
		else
			i = 0;
		if (j == eid)
			e = eioff;
		else
			e = l->nichar;

		for (; i < e; i++)
			l->underlines[i] = 1;

	} while (j++ < eid && (l = l->next));
}

void egui_text_set_underline(eHandle hobj)
{
	GuiText *text = GUI_TEXT_DATA(hobj);

	if (text->is_sel && REGION_NO_EMPTY(text->sel_rgn)) {
		set_underline(text);
		text->is_sel = false;
		egui_update_region(hobj, text->sel_rgn);
		egal_region_empty(text->sel_rgn);
	}
}
