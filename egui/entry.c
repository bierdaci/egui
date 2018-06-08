#include "gui.h"
#include "widget.h"
#include "event.h"
#include "char.h"
#include "entry.h"

#define SIDE_SPACING	2

static eint entry_init(eHandle, eValist);
static eint entry_init_data(eHandle, ePointer);
static void entry_free_data(eHandle, ePointer);
static void entry_init_orders(eGeneType, ePointer);
static eint entry_expose(eHandle, GuiWidget *, GalEventExpose *);
static eint entry_expose_bg(eHandle, GuiWidget *, GalEventExpose *);
static eint entry_keydown(eHandle, GalEventKey *);
static eint entry_char(eHandle, echar);
static void set_cursor_pos(eHandle, GuiEntry *, eint);
static void entry_show_cursor(eHandle, GuiEntry *, ebool);
static void entry_hide_cursor(eHandle, GuiEntry *);
static eint entry_configure(eHandle, GuiWidget *, GalEventConfigure *);
static void entry_set_font(eHandle, GalFont);
static void entry_insert_to_cursor(eHandle hobj, GuiEntry *entry, const echar *, eint);
static void entry_backspace_from_cursor(eHandle hobj, GuiEntry *entry);

static int (*entry_char_keydown)(eHandle, GalEventKey *) = NULL;
eGeneType egui_genetype_entry(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			entry_init_orders,
			sizeof(GuiEntry),
			entry_init_data,
			entry_free_data,
			NULL,
		};

		gtype = e_register_genetype(&info,
				GTYPE_WIDGET, GTYPE_STRINGS,
				GTYPE_EVENT, GTYPE_FONT, GTYPE_CHAR, NULL);
	}
	return gtype;
}

static eint entry_focus_in(eHandle hobj)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	GuiWidget *wid  = GUI_WIDGET_DATA(hobj);

	egal_set_foreground(wid->pb, 0xff0000);
	egal_draw_line(wid->drawable, wid->pb, 0, 0, 1000, 0);
	egal_draw_line(wid->drawable, wid->pb, 0, entry->h - 1, 1000, entry->h - 1);
	egal_draw_line(wid->drawable, wid->pb, 0, 0, 0, entry->h);
	egal_draw_line(wid->drawable, wid->pb, wid->rect.w - 1, 0, wid->rect.w - 1, entry->h);

	if (!entry->is_show && !entry->is_sel)
		entry_show_cursor(wid->drawable, entry, etrue);

	return 0;
}

static eint entry_focus_out(eHandle hobj)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	GuiWidget *wid  = GUI_WIDGET_DATA(hobj);

	egal_set_foreground(wid->pb, 0xa29276);
	egal_draw_line(wid->drawable, wid->pb, 0, 0, wid->rect.w, 0);
	egal_draw_line(wid->drawable, wid->pb, 0, entry->h - 1, wid->rect.w, entry->h - 1);
	egal_draw_line(wid->drawable, wid->pb, 0, 0, 0, entry->h);
	egal_draw_line(wid->drawable, wid->pb, wid->rect.w - 1, 0, wid->rect.w - 1, entry->h);

	if (entry->is_show && !entry->is_sel)
		entry_hide_cursor(wid->drawable, entry);

	return 0;
}

static void entry_set_cursor(eHandle hobj, GuiEntry  *entry, eint ioff)
{
	set_cursor_pos(hobj, entry, ioff);
	entry_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, entry, !entry->is_show);
}

static void update_select_area(eHandle hobj, GuiEntry *entry)
{
	if (entry->s_ioff != entry->e_ioff) {
		GalRect rc;
		rc.y = 0;
		rc.h = entry->h;
		if (entry->e_ioff < entry->s_ioff) {
			rc.x = entry->offsets[entry->e_ioff].x - entry->offset_x;
			rc.w = entry->offsets[entry->s_ioff].x - entry->offset_x - rc.x;
		}
		else {
			rc.x = entry->offsets[entry->s_ioff].x - entry->offset_x;
			rc.w = entry->offsets[entry->e_ioff].x - entry->offset_x - rc.x;
		}
		egui_update_rect(hobj, &rc);
	}
}

static eint entry_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);

	eint i;
	if (entry->visible) {
		eint w = 0;
		for (i = 0; i < entry->nichar; i++) {
			if (w + entry->glyphs[i].w / 2 > entry->offset_x + ent->point.x)
				break;
			w += entry->glyphs[i].w;
		}
	}
	else {
		i = (entry->offset_x + ent->point.x + entry->glyph.w / 2) / entry->glyph.w;
		if (i > entry->nichar)
			i = entry->nichar;
	}

	if (entry->is_sel) {
		entry->is_sel = efalse;
		update_select_area(hobj, entry);
	}

	entry_set_cursor(hobj, entry, i);
	entry->e_ioff = i;

	entry->bn_down = etrue;
#ifdef WIN32
	egal_grab_pointer(GUI_WIDGET_DATA(hobj)->window, etrue, 0);
#endif
	return 0;
}

static eint entry_lbuttonup(eHandle hobj, GalEventMouse *ent)
{
	GUI_ENTRY_DATA(hobj)->bn_down = efalse;
#ifdef WIN32
	egal_ungrab_pointer(GUI_WIDGET_DATA(hobj)->window);
#endif
	return 0;
}

static void cursor_offset_left(eHandle hobj, GuiEntry *entry)
{
	entry->offset_x = entry->offsets[entry->s_ioff].x;
	entry->x = 0;
}

static void cursor_offset_right(eHandle hobj, GuiEntry *entry)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (entry->offsets[entry->s_ioff].x >= wid->rect.w) {
		entry->offset_x = entry->offsets[entry->s_ioff].x - wid->rect.w + 1;
		entry->x = wid->rect.w - 1;
	}
	else {
		entry->offset_x = 0;
		entry->x = entry->total_w;
	}
}

static void entry_key_select(eHandle hobj, GuiEntry *entry, eint n)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	eint ox, nx;

	if (!entry->is_sel) {
		entry->is_sel = etrue;
		entry->e_ioff = entry->s_ioff;
	}

	ox = entry->offsets[entry->e_ioff].x - entry->offset_x;

	if (entry->e_ioff + n > entry->nichar)
		n = entry->nichar - entry->e_ioff;
	else if (entry->e_ioff + n < 0)
		n = -entry->e_ioff;

	if (n == 0) {
		if (entry->e_ioff == entry->s_ioff)
			entry->is_sel = efalse;
		return;
	}

	if (entry->e_ioff == entry->s_ioff)
		entry_hide_cursor(wid->drawable, entry);
	else if (entry->e_ioff + n != entry->s_ioff)
		entry->is_show = efalse;
	else {
		entry->is_sel  = efalse;
		entry->is_show = etrue;
	}

	entry->e_ioff += n;
	nx = entry->offsets[entry->e_ioff].x - entry->offset_x;

	if (nx < 0) {
		entry->offset_x = entry->offsets[entry->e_ioff].x;
		egui_update(hobj);
	}
	else if (nx > wid->rect.w) {
		entry->offset_x = entry->offsets[entry->e_ioff].x - wid->rect.w;
		egui_update(hobj);
	}
	else if (nx != ox) {
		GalRect rc;
		rc.y = 0;
		rc.h = entry->h;
		if (nx > ox) {
			rc.x = ox;
			rc.w = nx - ox;
		}
		else {
			rc.x = nx;
			rc.w = ox - nx;
		}
		egui_update_rect(hobj, &rc);
	}
}

static eint entry_mousemove(eHandle hobj, GalEventMouse *ent)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	eint w = 0;
	eint i = 0;
	eint e_ioff;
	GalRect rc;

	if (!entry->bn_down)
		return 0;

	if (entry->visible) {
		for (; i < entry->nichar; i++) {
			if (w + entry->glyphs[i].w > entry->offset_x + ent->point.x)
				break;
			w += entry->glyphs[i].w;
		}
	}
	else {
		i = (entry->offset_x + ent->point.x) / entry->glyph.w;
		if (i > entry->nichar)
			i = entry->nichar;
		w = i * entry->glyph.w;
	}

	if (i == entry->e_ioff)
		return 0;

	e_ioff = entry->e_ioff;
	entry->e_ioff = i;

	if (w - entry->offset_x > wid->rect.w) {
		entry->offset_x = w - wid->rect.w;
		if (e_ioff < i)
			egui_update(hobj);
		return 0;
	}
	else if (w < entry->offset_x) {
		entry->offset_x = w;
		egui_update(hobj);
		return 0;
	}
	else if (entry->s_ioff != i && e_ioff != i) {
		eint old_x = entry->offsets[e_ioff].x - entry->offset_x;
		eint ex = entry->offsets[entry->e_ioff].x - entry->offset_x;

		if (!entry->is_sel) {
			entry_hide_cursor(wid->drawable, entry);
			entry->is_sel = etrue;
		}

		rc.y = 0;
		rc.h = entry->h;
		if (old_x > ex) {
			rc.x = ex;
			rc.w = old_x - ex;
		}
		else {
			rc.x = old_x;
			rc.w = ex - old_x;
		}

		egui_update_rect(hobj, &rc);
	}
	else if (entry->s_ioff == i) {
		rc.y = 0;
		rc.h = entry->h;
		if (e_ioff < entry->e_ioff) {
			rc.x = entry->offsets[e_ioff].x - entry->offset_x;
			rc.w = entry->offsets[entry->e_ioff].x - entry->offset_x - rc.x;
		}
		else {
			rc.x = entry->offsets[entry->e_ioff].x - entry->offset_x;
			rc.w = entry->offsets[e_ioff].x - entry->offset_x - rc.x;
		}

		egui_update_rect(hobj, &rc);
	}

	if (entry->s_ioff == entry->e_ioff) {
		entry->is_sel = efalse;
		entry_show_cursor(GUI_WIDGET_DATA(hobj)->drawable, entry, !entry->is_show);
	}
	else if (entry->offsets[entry->s_ioff].x - entry->offset_x == rc.x)
		entry->is_show = efalse;
	else
		entry_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, entry);

	return 0;
}

static void entry_insert_text(eHandle hobj, const echar *chars, eint nchar)
{
	entry_insert_to_cursor(hobj, GUI_ENTRY_DATA(hobj), chars, nchar);
}

static eint entry_imeinput(eHandle hobj, GalEventImeInput *ent)
{
	entry_insert_to_cursor(hobj, GUI_ENTRY_DATA(hobj), (echar *)ent->data, ent->len);
	return 0;
}

static eint entry_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	eint cw;

	if (entry->font == 0) {
		entry->font = egal_default_font();
		set_cursor_pos(hobj, entry, entry->s_ioff);
		entry->old_w = wid->rect.w;
		return 0;
	}

	cw = entry->offsets[entry->s_ioff].x;
	if (resize->w > entry->old_w && entry->offset_x > 0) {
		if (entry->total_w - entry->offset_x < resize->w) {
			entry->offset_x = entry->total_w - resize->w + 1;
			if (entry->offset_x < 0)
				entry->offset_x = 0;
		}
	}
	else if (entry->total_w > resize->w && resize->w < entry->old_w) {
		if (cw - entry->offset_x > resize->w)
			entry->offset_x = cw - resize->w + 1;
	}

	wid->rect.w = resize->w;
	entry->x = cw - entry->offset_x;
	entry->old_w = resize->w;

	return 0;
}

static void entry_set_max(eHandle hobj, eint size)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	if (entry->max > size) {
	}
	entry->max = size;
}

static void entry_set_text(eHandle hobj, const echar *chars, eint clen)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	euchar *p = (euchar *)chars;

	eint i;
	eint nchar = 0;
	eunichar ichar;
	eint old_w = entry->total_w;

	eint old_nichar = entry->nichar;
	GalGlyph *glyphs;
	GalRect   rc;

	if (clen > entry->max)
		clen = entry->max;

	entry_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, entry);

	entry->s_ioff = 0;
	entry->nichar = 0;
	entry->nchar  = 0;
	do {
		nchar += e_uni_char_len(p + nchar);
		if (nchar <= clen) {
			entry->nchar = nchar;
			entry->nichar++;
		}
	} while (nchar < clen);

	if (entry->offsets)
		entry->offsets = e_realloc(entry->offsets, (entry->nichar + 1) * sizeof(CharOffset));
	else
		entry->offsets = e_malloc((entry->nichar + 1) * sizeof(CharOffset));

	if (entry->chars)
		entry->chars = e_realloc(entry->chars, entry->nchar);
	else
		entry->chars = e_malloc(entry->nchar);

	glyphs = e_malloc(entry->nichar * sizeof(GalGlyph));

	e_memcpy(entry->chars, chars, entry->nchar);

	entry->total_w = 0;
	for (i = 0, nchar = 0; i < entry->nichar; i++) {
		entry->offsets[i].c = nchar;
		ichar  = e_uni_get_char(p + nchar);
		nchar += e_uni_char_len(p + nchar);
		egal_get_glyph(entry->font, ichar, &glyphs[i]);
		if (entry->visible) {
			entry->total_w += glyphs[i].w;
			entry->offsets[i].x = entry->total_w - glyphs[i].w;
		}
		else {
			entry->total_w += entry->glyph.w;
			entry->offsets[i].x = entry->glyph.w * i;
		}
	}
	entry->offsets[i].c = nchar;
	entry->offsets[i].x = entry->total_w;

	rc.y = 0;
	rc.h = entry->h;
	if (entry->glyphs) {
		eint w = old_w > entry->total_w ? old_w : entry->total_w;
		for (i = 0; i < entry->nichar && i < old_nichar; i++) {
			if (glyphs[i].glyph != entry->glyphs[i].glyph)
				break;
		}
		rc.x = entry->offsets[i].x;
		rc.w = w - rc.x;
		e_free(entry->glyphs);
	}
	else {
		rc.x = 0;
		rc.w = entry->total_w;
	}

	entry->glyphs = glyphs;
	egui_update_rect(hobj, &rc);
}

static eint entry_get_text(eHandle hobj, echar *buf, eint size)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	eint len = entry->nchar < size ? entry->nchar : size;
	e_memcpy(buf, entry->chars, len);
	return len;
}

static eint entry_enter(eHandle hobj, eint x, eint y)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
	egal_set_cursor(wid->window, entry->cursor);
	return 0;
}

static eint entry_leave(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	egal_set_cursor(wid->window, 0);
	return 0;
}

static void entry_init_orders(eGeneType new, ePointer this)
{
	GuiCharOrders    *c = e_genetype_orders(new, GTYPE_CHAR);
	GuiFontOrders    *f = e_genetype_orders(new, GTYPE_FONT);
	GuiEventOrders   *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders  *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiStringsOrders *s = e_genetype_orders(new, GTYPE_STRINGS);
	eCellOrders      *o = e_genetype_orders(new, GTYPE_CELL);

	entry_char_keydown = e->keydown;

	w->resize         = entry_resize;
	w->expose         = entry_expose;
	w->expose_bg      = entry_expose_bg;
	w->configure      = entry_configure;

	e->enter          = entry_enter;
	e->leave          = entry_leave;
	e->focus_in       = entry_focus_in;
	e->focus_out      = entry_focus_out;
	e->keydown        = entry_keydown;
	e->lbuttonup      = entry_lbuttonup;
	e->mousemove      = entry_mousemove;
	e->lbuttondown    = entry_lbuttondown;
	e->imeinput       = entry_imeinput;

	s->insert_text    = entry_insert_text;
	s->set_text       = entry_set_text;
	s->get_text       = entry_get_text;
	s->set_max        = entry_set_max;

	c->achar          = entry_char;
	f->set_font       = entry_set_font;

	o->init           = entry_init;
}

static eint entry_configure(eHandle hobj, GuiWidget *wid, GalEventConfigure *ent)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);

	if (entry->font == 0) {
		entry->font = egal_default_font();
		set_cursor_pos(hobj, entry, entry->s_ioff);
		entry->old_w = wid->rect.w;
	}
	return 0;
}

static eint entry_char(eHandle hobj, echar c)
{
	GuiEntry    *entry = GUI_ENTRY_DATA(hobj);
	const echar *buf;
	eint n;

	if (c == '\b') {
		entry_backspace_from_cursor(hobj, entry);
		return 0;
	}
	else if (c == '\n')
		return 0;
	else {
		buf = &c;
		n = 1;
	}

	entry_insert_to_cursor(hobj, entry, buf, n);

	return -1;
}

static void cursor_move_left(eHandle hobj, GuiEntry *entry)
{
	GalRect *rect = &GUI_WIDGET_DATA(hobj)->rect;

	if (entry->is_sel) {
		entry->is_sel = efalse;
		update_select_area(hobj, entry);
	}

	if (entry->s_ioff > 0)
		entry_set_cursor(hobj, entry, entry->s_ioff - 1);

	if (entry->offsets[entry->s_ioff].x < entry->offset_x) {
		cursor_offset_left(hobj, entry);
		egui_update(hobj);
	}
	else if (entry->offsets[entry->s_ioff].x - entry->offset_x >= rect->w) {
		cursor_offset_right(hobj, entry);
		egui_update(hobj);
	}
}

static void cursor_move_right(eHandle hobj, GuiEntry *entry)
{
	GalRect *rect = &GUI_WIDGET_DATA(hobj)->rect;

	if (entry->is_sel) {
		entry->is_sel = efalse;
		update_select_area(hobj, entry);
	}

	if (entry->s_ioff < entry->nichar)
		entry_set_cursor(hobj, entry, entry->s_ioff + 1);

	if (entry->offsets[entry->s_ioff].x < entry->offset_x) {
		cursor_offset_left(hobj, entry);
		egui_update(hobj);
	}
	else if (entry->offsets[entry->s_ioff].x - entry->offset_x >= rect->w) {
		cursor_offset_right(hobj, entry);
		egui_update(hobj);
	}
}

static eint entry_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);

	switch (ent->code) {
		case GAL_KC_Up:
		case GAL_KC_Down:
		case GAL_KC_Tab:
			return 0;

		case GAL_KC_Left:
			if (ent->state & GAL_KS_SHIFT)
				entry_key_select(hobj, entry, -1);
			else
				cursor_move_left(hobj, entry);
			break;
		case GAL_KC_Right:
			if (ent->state & GAL_KS_SHIFT)
				entry_key_select(hobj, entry, 1);
			else
				cursor_move_right(hobj, entry);
			break;

		case GAL_KC_Enter:
			e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
			break;

		default:
			entry_char_keydown(hobj, ent);
	}
	return -1;
}

static eint entry_init_data(eHandle hobj, ePointer this)
{
	GuiEntry *entry = this;

	entry->cursor   = egal_cursor_new(GAL_XTERM);
	entry->nchar    = 0;
	entry->offset_x = 0;
	entry->is_sel   = efalse;
	entry->bn_down  = efalse;
	entry->max      = 0xffff;
	entry->offsets  = e_calloc(sizeof(CharOffset), 1);

	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);

	return 0;
}

static void entry_free_data(eHandle hobj, ePointer this)
{
}

static void entry_set_font(eHandle hobj, GalFont font)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	entry->font     = font;
}

static eint entry_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);

	if (entry->is_sel) {
		eint sx,  ex;
		eint eex, esx;
		eint exp_ex;

		if (entry->s_ioff > entry->e_ioff) {
			eex = entry->offsets[entry->s_ioff].x - entry->offset_x;
			esx = entry->offsets[entry->e_ioff].x - entry->offset_x;
		}
		else {
			eex = entry->offsets[entry->e_ioff].x - entry->offset_x;
			esx = entry->offsets[entry->s_ioff].x - entry->offset_x;
		}

		exp_ex = exp->rect.x + exp->rect.w;
		sx = esx < exp->rect.x ? exp->rect.x : esx;
		ex = exp_ex < eex ? exp_ex : eex;

		if (ex > sx) {
			egal_set_foreground(exp->pb, 0x9a958d);
			egal_fill_rect(wid->drawable, exp->pb,
					sx, SIDE_SPACING, ex - sx,
					entry->h - SIDE_SPACING * 2 - 1);
		}

		egal_set_foreground(exp->pb, wid->bg_color);

		if (exp->rect.x < esx) {
			egal_fill_rect(wid->drawable, exp->pb,
					exp->rect.x, SIDE_SPACING,
					esx - exp->rect.x,
					entry->h - SIDE_SPACING * 2);
		}

		if (exp_ex > eex) {
			egal_fill_rect(wid->drawable, exp->pb,
					eex, SIDE_SPACING,
					exp_ex - eex,
					entry->h - SIDE_SPACING * 2);
		}
	}
	else {
		egal_set_foreground(exp->pb, wid->bg_color);
		egal_fill_rect(wid->drawable, exp->pb,
				exp->rect.x, 0, exp->rect.w, entry->h);

		if (WIDGET_STATUS_FOCUS(wid))
			egal_set_foreground(exp->pb, 0xff0000);
		else
			egal_set_foreground(exp->pb, 0xa29276);
		egal_draw_line(wid->drawable, exp->pb, exp->rect.x,
				0, exp->rect.x + exp->rect.w, 0);
		egal_draw_line(wid->drawable, exp->pb, exp->rect.x,
				entry->h - 1, exp->rect.x + exp->rect.w, entry->h - 1);

		if (exp->rect.x <= 0)
			egal_draw_line(wid->drawable, wid->pb, 0, 0, 0, entry->h);
		if (exp->rect.x + exp->rect.w >= wid->rect.w)
			egal_draw_line(wid->drawable, wid->pb, wid->rect.w - 1, 0, wid->rect.w - 1, entry->h);
	}

	return 0;
}

static void no_visible_expose(GuiWidget *wid, GuiEntry *entry, GalEventExpose *exp)
{
	if ((entry->offset_x + exp->rect.x) / entry->glyph.w < entry->nichar) {
		eint i, n, x;
		eint over = (entry->offset_x + exp->rect.x) % entry->glyph.w;
		eint _n   = (entry->offset_x + exp->rect.x) / entry->glyph.w;

		eint ew = exp->rect.w;

		if (exp->rect.x + ew > wid->rect.w)
			ew = wid->rect.w - exp->rect.x;

		ew += over;
		n = ew / entry->glyph.w;

		if (ew % entry->glyph.w > 0)
			n ++;

		if (n > entry->nichar - _n)
			n = entry->nichar - _n;
		x = exp->rect.x - over;
		egal_set_foreground(exp->pb, wid->fg_color);

		for (i = 0; i < n; i++, x += entry->glyph.w)
			egal_draw_glyphs(wid->drawable, exp->pb,
					entry->font, &exp->rect, x, SIDE_SPACING, &entry->glyph, 1);
	}

	if (WIDGET_STATUS_FOCUS(wid))
		entry_show_cursor(wid->drawable, entry, entry->is_show);
}


static void visible_expose(GuiWidget *wid, GuiEntry *entry, GalEventExpose *exp)
{
	eint w = 0;
	eint i = 0;
	eint x, n, over;

	if (entry->nichar == 0) {
		if (WIDGET_STATUS_FOCUS(wid))
			entry_show_cursor(wid->drawable, entry, entry->is_show);
		return;
	}

	for (; i < entry->nichar; i++) {
		if (w + entry->glyphs[i].w > entry->offset_x)
			break;
		w += entry->glyphs[i].w;
	}
	over = w + entry->glyphs[i].w - entry->offset_x;
	x = over - entry->glyphs[i].w;

	for (w = 0; i < entry->nichar; i++) {
		if (w + entry->glyphs[i].w > exp->rect.x)
			break;
		w += entry->glyphs[i].w;
	}

	if (i < entry->nichar) {
		eint ew = exp->rect.w;
		if (exp->rect.x + ew > wid->rect.w)
			ew = wid->rect.w - exp->rect.x;
		x += w;
		n  = i + 1;
		w  = over;
		for (; n < entry->nichar && w < ew; n++)
			w += entry->glyphs[n].w;

		if (w > ew && n < entry->nichar)
			n++;

		egal_set_foreground(exp->pb, wid->fg_color);
		egal_draw_glyphs(wid->drawable, exp->pb,
				entry->font, &exp->rect, x, SIDE_SPACING, entry->glyphs + i, n - i);
	}

	if (WIDGET_STATUS_FOCUS(wid))
		entry_show_cursor(wid->drawable, entry, entry->is_show);
}

static eint entry_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	if (entry->visible)
		visible_expose(wid, entry, exp);
	else
		no_visible_expose(wid, entry, exp);

	return 0;
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
		e_memmove((echar *)chars + o + d, (echar *)chars + o, c - o);
	}

	if (data) e_memcpy((echar *)chars + o, data, d);

	return chars;
}

static void entry_delete_from_offset(eHandle hobj, GuiEntry *entry, eint ioff, eint ni)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	eint tn  = entry->offsets[entry->nichar].c - entry->offsets[ioff + ni].c;
	eint tni = entry->nichar - (ioff + ni);
	eint n   = entry->nchar - tn - entry->offsets[ioff].c;

	eint total_w  = entry->total_w;
	eint offset_x = entry->offset_x;

	eint i, nw;
	if (entry->visible) {
		for (i = 0, nw = 0; i < ni; i++)
			nw += entry->glyphs[ioff + i].w;
	}
	else
		nw = entry->glyph.w * ni;
	entry->total_w -= nw;

	if (entry->total_w - entry->offset_x < wid->rect.w) {
		entry->offset_x = entry->total_w - wid->rect.w + 1;
		if (entry->offset_x < 0)
			entry->offset_x = 0;
	}

	e_memmove(entry->chars + entry->offsets[ioff].c, entry->chars + entry->offsets[ioff+ni].c, tn);
	e_memmove(entry->glyphs + ioff, entry->glyphs + ioff + ni, tni * sizeof(GalGlyph));
	e_memmove(entry->offsets + ioff, entry->offsets + ioff + ni, (tni + 1) * sizeof(CharOffset));

	for (i = 0; i <= tni; i++) {
		entry->offsets[ioff + i].c -= n;
		entry->offsets[ioff + i].x -= nw;
	}

	entry->nchar  -= n;
	entry->nichar -= ni;

	entry->chars   = e_realloc(entry->chars, entry->nchar);
	entry->glyphs  = e_realloc(entry->glyphs, entry->nichar * sizeof(GalGlyph));
	entry->offsets = e_realloc(entry->offsets, (entry->nichar + 1) * sizeof(CharOffset));

	set_cursor_pos(hobj, entry, ioff);
	entry->is_show = etrue;
	if (ioff == 0) {
		entry->offset_x = 0;
		entry->x = 0;
	}
	else if (entry->visible && entry->x < entry->glyphs[ioff - 1].w) {
		entry->offset_x = entry->offsets[ioff - 1].x;
		entry->x = entry->offsets[ioff].x - entry->offset_x;
	}
	else if (!entry->visible && entry->x < entry->glyph.w) {
		entry->offset_x = entry->offsets[ioff - 1].x;
		entry->x = entry->offsets[ioff].x - entry->offset_x;
	}

	if (offset_x != entry->offset_x) {
		if (entry->s_ioff != entry->o_ioff || entry->offset_x == 0)
			egui_update(hobj);
		else {
			GalRect rc = {0, 0, entry->x, entry->h};
			egui_update_rect(hobj, &rc);
		}
	}
	else {
		GalRect rc = {entry->x, 0, total_w - offset_x - entry->x, entry->h};
		egui_update_rect(hobj, &rc);
	}
}

static void insert_to_cursor(eHandle hobj, GuiEntry *entry, const echar *chars, eint clen)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	euchar *p = (euchar *)chars;

	eint nchar, i, nw, pw;
	eint ioff, coff;

	eint in_nichar = 0;
	eint in_nchar  = 0;

	eint offset_x = entry->offset_x;
	eint old_x    = entry->offsets[entry->s_ioff].x - offset_x;

	if (entry->nchar + clen > entry->max)
		clen = entry->max - entry->nchar;

	if (clen == 0)
		return;

	nchar = 0;
	do {
		nchar += e_uni_char_len(p + nchar);
		if (nchar <= clen) {
			in_nchar = nchar;
			in_nichar++;
		}
	} while (nchar < clen);

	ioff = entry->s_ioff;
	coff = entry->offsets[ioff].c;
	pw   = entry->offsets[ioff].x;

	entry->chars = insert_data(entry->chars,
					entry->nchar, coff,
					(ePointer)chars,
					in_nchar,
					sizeof(echar));
	entry->glyphs = insert_data(entry->glyphs,
					entry->nichar, ioff,
					NULL, in_nichar,
					sizeof(GalGlyph));
	entry->offsets = insert_data(entry->offsets,
					entry->nichar + 1, ioff,
					NULL, in_nichar,
					sizeof(CharOffset));

	entry->nchar  += in_nchar;
	entry->nichar += in_nichar;

	for (nchar = 0, nw = 0, i = ioff; i < ioff + in_nichar; i++) {
		eunichar ichar;
		entry->offsets[i].c = coff + nchar;
		ichar  = e_uni_get_char(p + nchar);
		nchar += e_uni_char_len(p + nchar);
		egal_get_glyph(entry->font, ichar, &entry->glyphs[i]);
		if (entry->visible) {
			nw += entry->glyphs[i].w;
			entry->offsets[i].x = pw + nw - entry->glyphs[i].w;
		}
		else {
			nw += entry->glyph.w;
			entry->offsets[i].x = pw + nw - entry->glyph.w;
		}
	}

	for (; i < entry->nichar; i++) {
		entry->offsets[i].c += in_nchar;
		entry->offsets[i].x += nw;
	}
	entry->offsets[entry->nichar].c += in_nchar;
	entry->offsets[entry->nichar].x += nw;
	entry->total_w += nw;

	set_cursor_pos(hobj, entry, ioff + in_nichar);
	if (entry->x >= wid->rect.w)
		cursor_offset_right(hobj, entry);

	if (offset_x != entry->offset_x)
		egui_update(hobj);
	else {
		GalRect rc = {old_x, 0, entry->total_w - offset_x - old_x, entry->h};
		egui_update_rect(hobj, &rc);
	}
}

static void entry_insert_to_cursor(eHandle hobj, GuiEntry *entry, const echar *chars, eint clen)
{
	if (entry->is_sel) {
		eint n = entry->e_ioff - entry->s_ioff;
		entry->is_sel = efalse;
		if (n < 0)
			entry_delete_from_offset(hobj, entry, entry->e_ioff, -n);
		else
			entry_delete_from_offset(hobj, entry, entry->s_ioff, n);
	}

	insert_to_cursor(hobj, entry, chars, clen);
}

static void entry_backspace_from_cursor(eHandle hobj, GuiEntry *entry)
{
	if (entry->is_sel) {
		eint n = entry->e_ioff - entry->s_ioff;
		entry->is_sel  = efalse;
		if (n < 0)
			entry_delete_from_offset(hobj, entry, entry->e_ioff, -n);
		else
			entry_delete_from_offset(hobj, entry, entry->s_ioff, n);
	}
	else if (entry->s_ioff > 0)
		entry_delete_from_offset(hobj, entry, entry->s_ioff - 1, 1);
}

static void set_cursor_pos(eHandle hobj, GuiEntry *entry, eint ioff)
{
	entry_hide_cursor(GUI_WIDGET_DATA(hobj)->drawable, entry);
	entry->s_ioff = ioff;
	entry->x = entry->offsets[ioff].x - entry->offset_x;
}

static void entry_show_cursor(GalDrawable draw, GuiEntry *entry, ebool show)
{
	if (show || entry->o_ioff != entry->s_ioff) {
		if (!entry->is_sel || entry->s_ioff == entry->e_ioff) {
			eint x = entry->offsets[entry->s_ioff].x - entry->offset_x;
			egal_draw_line(draw, entry->cpb, x, SIDE_SPACING, x, entry->h - SIDE_SPACING * 2);
		}
		entry->is_show = etrue;
		if (entry->o_ioff != entry->s_ioff)
			entry->o_ioff  = entry->s_ioff;
	}
}

static INLINE void entry_hide_cursor(eHandle draw, GuiEntry *entry)
{
	if (entry->is_show) {
		eint x = entry->offsets[entry->s_ioff].x - entry->offset_x;
		entry->is_show = efalse;
		if (!entry->is_sel || entry->s_ioff == entry->e_ioff)
			egal_draw_line(draw, entry->cpb, x, SIDE_SPACING, x, entry->h - SIDE_SPACING * 2);
	}
}

static eint entry_init(eHandle hobj, eValist vp)
{
	GuiEntry  *entry = GUI_ENTRY_DATA(hobj);
	GuiWidget *wid   = GUI_WIDGET_DATA(hobj);

	eint w = e_va_arg(vp, eint);
	eint retval;

	entry->font = egal_default_font();
	entry->h    = egal_font_height(entry->font) + SIDE_SPACING * 2;
	entry->visible = etrue;

	wid->rect.w = w;
	wid->rect.h = entry->h;
	wid->bg_color = 0xffffff;
	wid->fg_color = 0;
	wid->min_w = w;
	wid->min_h = entry->h;
	wid->max_w = 0;
	wid->max_h = 1;
	retval = e_signal_emit(hobj, SIG_REALIZE);
	#ifdef WIN32
	{
		GalPBAttr  attribute;
		attribute.func = GalPBnor;
		entry->cpb = egal_pb_new(wid->window, &attribute);
	}
#else
	entry->cpb = egal_type_pb(GalPBnor);
#endif
	return retval;
}

eHandle egui_entry_new(eint w)
{
	return e_object_new(GTYPE_ENTRY, w);
}

void egui_entry_set_visibility(eHandle hobj, ebool visible)
{
	GuiEntry *entry = GUI_ENTRY_DATA(hobj);
	eint i;

	eunichar ichar = e_uni_get_char((euchar *)"*");

	egal_get_glyph(entry->font, ichar, &entry->glyph);
	entry->glyph_w = entry->glyph.w;
	entry->visible = visible;

	entry->total_w = 0;
	for (i = 0; i < entry->nichar; i++) {
		entry->total_w += entry->glyph.w;
		entry->offsets[i].x = entry->glyph.w * i;
	}
}
