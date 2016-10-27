#include "gui.h"
#include "widget.h"
#include "layout.h"
#include "adjust.h"

typedef struct _LayoutWrap LayoutWrap;
struct _LayoutWrap {
	eint begin;
	eint nchar;
	eint nichar;
	eint width;
	eint id;
	LayoutWrap *next;
	LayoutWrap *prev;
};

struct _LayoutLine {
	echar *chars;
	eint nchar;
	eint nichar;
	eint nwrap;
	eint id;
	eint max_w;
	ebool is_lf;
	eint over;
	eint lack;
	eint offset_x;
	eint offset_y;
	eint load_nchar;
	eint load_nichar;
	struct _LayoutLine *top;
	struct _LayoutLine *next;
	struct _LayoutLine *prev;

	GalGlyph *glyphs;
	eint     *coffsets;
	echar    *underlines;
	LayoutWrap  wrap_head;
	LayoutWrap *wrap_tail;
	LayoutWrap *wrap_top;
};

static eint layout_init_data(eHandle, ePointer);
static void layout_free_data(eHandle, ePointer);
static void layout_init_orders(eGeneType, ePointer);
static void layout_clear(GuiLayout *);
static void __layout_unset_wrap(eHandle hobj);
static void __layout_set_wrap(eHandle hobj);
static void __layout_set_align(eHandle hobj, LayoutFlags);
static void __layout_set_table_size(eHandle hobj, eint);
static void __layout_set_extent(eHandle, eint, eint);
static void __layout_set_spacing(eHandle, eint);
static void __layout_set_offset(eHandle, eint, eint);
static void __layout_draw(eHandle, GalDrawable, GalPB, GalRect *);
static void __layout_set_text(eHandle, const echar *, eint);
static void __layout_insert_text(eHandle, const echar *, eint);
static void __layout_set_font(eHandle, GalFont);

eGeneType egui_genetype_layout(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiLayoutOrders),
			layout_init_orders,
			sizeof(GuiLayout),
			layout_init_data,
			layout_free_data,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_STRINGS, GTYPE_FONT, NULL);
	}
	return gtype;
}

static void layout_init_orders(eGeneType new, ePointer this)
{
	GuiLayoutOrders  *l = this;
	GuiStringsOrders *s = e_genetype_orders(new, GTYPE_STRINGS);
	GuiFontOrders    *f = e_genetype_orders(new, GTYPE_FONT);

	s->set_text       = __layout_set_text;
	s->insert_text    = __layout_insert_text;

	l->set_spacing    = __layout_set_spacing;
	l->set_extent     = __layout_set_extent;
	l->set_offset     = __layout_set_offset;
	l->set_align      = __layout_set_align;
	l->set_wrap       = __layout_set_wrap;
	l->unset_wrap     = __layout_unset_wrap;
	l->draw           = __layout_draw;
	l->set_table_size = __layout_set_table_size;
	
	f->set_font       = __layout_set_font;
}

static LayoutLine *y_offset_to_line(GuiLayout *layout)
{
	LayoutLine *top;
	LayoutLine *line = layout->line_head;

	 do  {
		top  = line;
		line = line->next;
	 } while (line && line->offset_y < layout->scroll_y);

	 return top;
}

static eint layout_vadjust_update(eHandle hobj, efloat value)
{
	eHandle       obj = GUI_ADJUST_DATA(hobj)->owner;
	GuiLayout *layout = GUI_LAYOUT_DATA(obj);
	layout->scroll_y  = (eint)value;
	layout->top = y_offset_to_line(layout);
	egui_update(obj);
    return 0;
}   

static eint layout_hadjust_update(eHandle hobj, efloat value)
{
	eHandle       obj = GUI_ADJUST_DATA(hobj)->owner;
	GuiLayout *layout = GUI_LAYOUT_DATA(obj);
	layout->scroll_x  = (eint)value;
	egui_update(obj);
    return 0;
}

static eint layout_init_data(eHandle hobj, ePointer this)
{
	GuiLayout  *layout = this;
	layout->is_wrap    = efalse;
	layout->table_size = DEFAULT_TABLE_SIZE;
	layout->vadj       = egui_adjust_new(0, 1, 1, 0, 0);
	layout->hadj       = egui_adjust_new(0, 1, 1, 0, 0);
	egui_adjust_set_owner(layout->vadj, hobj);
	egui_adjust_set_owner(layout->hadj, hobj);
	e_signal_connect(layout->vadj, SIG_ADJUST_UPDATE, layout_vadjust_update);
	e_signal_connect(layout->hadj, SIG_ADJUST_UPDATE, layout_hadjust_update);

	return 0;
}

static void layout_free_data(eHandle hobj, ePointer this)
{
	layout_clear(this);
}

static LayoutLine *layout_line_new(void)
{
	LayoutLine *line = e_slice_new(LayoutLine);
	line->nichar      = 0;
	line->chars       = NULL;
	line->nwrap       = 0;
	line->is_lf       = efalse;
	line->nchar       = 0;
	line->id          = 0;
	line->over        = 0;
	line->lack        = 0;
	line->max_w       = 0;
	line->offset_x    = 0;
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

	return line;
}

#define ioffset_char(line, ioff) line->chars[line->coffsets[ioff]]

static ebool ioffset_line_end(LayoutLine *line, eint ioff)
{
	if (line->chars[line->coffsets[ioff]] == '\n'
			|| ioff >= line->nichar)
		return etrue;
	return efalse;
}

static ebool ioffset_is_latin(LayoutLine *line, eint ioff)
{
	echar c = line->chars[line->coffsets[ioff]];
	if (c < 128
			&& c != ' '
			&& c != '\t'
			&& !ioffset_line_end(line, ioff))
		return etrue;
	return efalse;
}

static eint latin_word_width(LayoutLine *line, eint o, eint *j, eint max_w)
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

static void line_load_glyphs(LayoutLine *line, GalFont font)
{
	eint nchar, i, n;
	const echar *p;
	eunichar ichar;

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
		ichar  = e_uni_get_char(p + nchar);
		nchar += e_uni_char_len(p + nchar);
		egal_get_glyph(font, ichar, &line->glyphs[i]);
	}

	line->load_nchar  = line->nchar - line->over;
	line->coffsets[n] = line->load_nchar;
	line->load_nichar = n;
}

static void line_clear(LayoutLine *line)
{
	LayoutWrap *wrap = line->wrap_head.next;
	while (wrap) {
		LayoutWrap *t = wrap;
		wrap = wrap->next;
		e_slice_free(LayoutWrap, t);
	}

	line->nwrap = 0;
	line->wrap_top  = NULL;
	line->wrap_tail = NULL;

	wrap = &line->wrap_head;
	wrap->next   = NULL;
	wrap->id     = 0;
	wrap->begin  = 0;
	wrap->nchar  = 0;
	wrap->nichar = 0;
	wrap->width  = 0;
}

static void layout_clear(GuiLayout *layout)
{
	LayoutLine *line = layout->line_head;

	while (line) {
		LayoutLine *t = line;
		line = line->next;
		e_free(t->chars);
		e_free(t->glyphs);
		e_free(t->coffsets);
		e_free(t->underlines);
		line_clear(t);
		e_slice_free(LayoutLine, t);
	}

	layout->nchar = 0;
	layout->nline = 0;
	layout->line_head = NULL;
	layout->line_tail = NULL;
}

static void __layout_wrap(GuiLayout *layout, LayoutLine *line)
{
	LayoutWrap *wrap = &line->wrap_head;
	eint i = 0;

	line_clear(line);

	line->wrap_tail = wrap;
	line->nwrap  = 1;
	line->max_w  = 0;
	wrap->id     = 0;
	wrap->begin  = 0;
	wrap->width  = 0;
	wrap->nchar  = 0;
	wrap->nichar = 0;
	wrap->next   = NULL;
	wrap->prev   = NULL;

	while (!ioffset_line_end(line, i)) {
		eint w, j = 1;

		if (layout->is_wrap && ioffset_is_latin(line, i)) {
			w = latin_word_width(line, i, &j, layout->w);
		}
		else if (ioffset_char(line, i) == '\t') {
			if (layout->is_wrap && wrap->width == layout->w) {
				line->glyphs[i].w = layout->table_w;
				w = layout->table_w;
			}
			else {
				w = layout->table_w - wrap->width % layout->table_w;
				if (layout->is_wrap && (wrap->width + w > layout->w))
					w = layout->w - wrap->width;
				line->glyphs[i].w = w;
			}
		}
		else
			w = line->glyphs[i].w;

		if (layout->is_wrap && wrap->width + w > layout->w) {
			LayoutWrap *new = e_slice_new(LayoutWrap);
			if (line->max_w < wrap->width)
				line->max_w = wrap->width;

			new->id     = wrap->id + 1;
			new->begin  = i;
			new->width  = 0;
			new->nchar  = 0;
			new->nichar = 0;
			new->next   = NULL;
			new->prev   = wrap;

			wrap->next  = new;
			wrap        = new;

			line->wrap_tail = new;
			line->nwrap++;
		}
		wrap->nchar += line->coffsets[i + j] - line->coffsets[i];
		wrap->nichar+= j;
		wrap->width += w;
		i += j;
	}
	if (line->max_w < wrap->width)
		line->max_w = wrap->width;

	if (!layout->is_wrap) {
		LayoutFlags align = layout->align & LF_HRight;
		if (align == LF_HCenter)
			line->offset_x = (layout->w - line->max_w) / 2;
		else if (align == LF_HRight)
			line->offset_x = layout->w - line->max_w;
		else
			line->offset_x = 0;
	}
}

static eint layout_line_insert(GuiLayout *layout, const echar *text, eint clen)
{
	const echar *p = text;
	eunichar ichar = 0;
	eint       len = 0;
	LayoutLine *line;

	line = layout->line_tail;
	if (!line || line->is_lf) {
		line = layout_line_new();
		line->id = layout->nline;
		if (!layout->line_head) {
			layout->line_head = line;
			layout->line_tail = line;
		}
		else {
			line->prev = layout->line_tail;
			layout->line_tail->next = line;
			layout->line_tail = line;
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
		eint l = e_uni_char_len(p + len);
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
		line->is_lf = etrue;
		layout->nline ++;
	}

finish:
	if (line->nchar > 0)
		line->chars = e_realloc(line->chars, line->nchar + len);
	else
		line->chars = e_malloc(len);

	e_memcpy(line->chars + line->nchar, p, len);
	line->nchar   += len;
	layout->nchar += len;

	return len;
}

static void __insert_text(GuiLayout *layout, const echar *text, eint nchar)
{
	while (nchar > 0) {
		eint n = layout_line_insert(layout, text, nchar);
		text  += n;
		nchar -= n;
	}
}

static void __layout_insert_text(eHandle hobj, const echar *text, eint nchar)
{
	__insert_text(GUI_LAYOUT_DATA(hobj), text, nchar);
}

static eint layout_wrap(GuiLayout *layout, LayoutLine *line)
{
	line_load_glyphs(line, layout->font);

	__layout_wrap(layout, line);

	return line->nwrap * layout->hline;
}

static INLINE eint get_space_width(GalFont font)
{
	GalGlyph glyph;
	egal_get_glyph(font, e_uni_get_char((echar *)" "), &glyph);
	return glyph.w;
}

void layout_get_extents(GuiLayout *layout, eint *w, eint *h)
{
	LayoutLine *line;
	eint total_h = 0;
	eint max_w   = 0;

	if (layout->font == 0)
		layout->font = egal_default_font();

	line = layout->line_head;
	layout->hline   = egal_font_height(layout->font) + layout->spacing;
	layout->table_w = get_space_width(layout->font)  * layout->table_size;
	while (line) {
		total_h   += layout_wrap(layout, line);
		if (line->max_w > max_w)
			max_w = line->max_w;
		line = line->next;
	}

	if (w) *w = max_w;
	if (h) *h = total_h;
}

static void layout_configure(GuiLayout *layout)
{
	LayoutLine *line;
	eint total_h = 0;

	if (layout->font == 0)
		layout->font = egal_default_font();

	line = layout->line_head;
	layout->max_w   = 0;
	layout->hline   = egal_font_height(layout->font) + layout->spacing;
	layout->table_w = get_space_width(layout->font)  * layout->table_size;
	while (line) {
		line->offset_y = total_h;
		total_h   += layout_wrap(layout, line);
		if (line->max_w > layout->max_w)
			layout->max_w = line->max_w;
		line = line->next;
	}
	layout->configure = etrue;

	if (layout->hadj) {
		if (layout->max_w > layout->w) {
			if (layout->max_w - layout->scroll_x < layout->w)
				layout->scroll_x = layout->max_w - layout->w;
			egui_adjust_reset_hook(layout->hadj,
				(efloat)layout->scroll_x, (efloat)layout->w,
				(efloat)layout->max_w, 1, 0);
		}
		else
			egui_adjust_reset_hook(layout->hadj, 0, 1, 1, 0, 0);
	}
	if (layout->vadj && layout->total_h != total_h) {
		if (total_h > layout->h) {
			if (total_h - layout->scroll_y < layout->h)
				layout->scroll_y = total_h - layout->h;
			egui_adjust_reset_hook(layout->vadj,
				(efloat)layout->scroll_y, (efloat)layout->h,
				(efloat)total_h, (efloat)layout->hline, 0);
		}
		else
			egui_adjust_reset_hook(layout->vadj, 0, 1, 1, 0, 0);
	}
	layout->total_h = total_h;

	if (layout->total_h < layout->h) {
		LayoutFlags align = layout->align & LF_VBottom;
		line = layout->line_head;
		while (line) {
			if (align == LF_VCenter)
				line->offset_y += (layout->h - layout->total_h) / 2;
			else if (align == LF_VBottom)
				line->offset_y += layout->h - layout->total_h;
			line = line->next;
		}
	}
}

static void __layout_set_text(eHandle hobj, const echar *text, eint len)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);

	layout_clear(layout);
	__insert_text(layout, text, len);

	layout->top       = NULL;
	layout->scroll_y  = 0;
	layout->configure = efalse;
}

static void __layout_set_font(eHandle hobj, GalFont font)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	layout->font      = font;
	layout->configure = efalse;
}

static void __layout_set_table_size(eHandle hobj, eint size)
{
	GuiLayout *layout  = GUI_LAYOUT_DATA(hobj);
	layout->table_size = size;
	layout->configure  = efalse;
}

static void __layout_set_wrap(eHandle hobj)
{
	GuiLayout *layout  = GUI_LAYOUT_DATA(hobj);
	if (!layout->is_wrap) {
		layout->is_wrap   = etrue;
		layout->configure = efalse;
	}
}

static void __layout_unset_wrap(eHandle hobj)
{
	GuiLayout *layout  = GUI_LAYOUT_DATA(hobj);
	if (layout->is_wrap) {
		layout->is_wrap   = efalse;
		layout->configure = efalse;
	}
}

static void line_set_align(GuiLayout *layout, LayoutFlags align)
{
	ebool is_halign = !layout->is_wrap;
	ebool is_valign = layout->total_h < layout->h;
	LayoutFlags valign = align & LF_VBottom;
	LayoutFlags halign = align & LF_HRight;

	LayoutLine *line = layout->line_head;
	while (line) {
		if (is_halign && line->max_w < layout->w) {
			if (halign == LF_HCenter)
				line->offset_x = (layout->w - line->max_w) / 2;
			else if (halign == LF_HRight)
				line->offset_x = layout->w - line->max_w;
			else
				line->offset_x = 0;
		}
		if (is_valign) {
			if (valign == LF_VCenter)
				line->offset_y += (layout->h - layout->total_h) / 2;
			else if (valign == LF_VBottom)
				line->offset_y += layout->h - layout->total_h;
		}
		line = line->next;
	}
}

static void __layout_set_align(eHandle hobj, LayoutFlags flags)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	if (layout->align != flags) {
		layout->align = flags;
		line_set_align(layout, flags);
		layout->configure = efalse;
	}
}

static void __layout_set_extent(eHandle hobj, eint w, eint h)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	layout->w = w;
	layout->h = h;
	layout->configure = efalse;
}

static void __layout_set_offset(eHandle hobj, eint x, eint y)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	layout->offset_x  = x;
	layout->offset_y  = y;
	layout->configure = efalse;
}

static void __layout_set_spacing(eHandle hobj, eint spacing)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	layout->spacing   = spacing;
	layout->configure = efalse;
}

static void layout_draw_wrap(GalDrawable drawable, GalPB pb,
		GuiLayout *layout, LayoutWrap *wrap, GalGlyph *glyphs, echar *underlines,
		GalRect *prc, eint offset_x, eint offset_y)
{
	eint i, j;
	eint w = offset_x;
	eint x = 0;
	eint len = 0;

	if (prc->x == 0 && prc->w >= wrap->width) {
		egal_draw_glyphs(drawable, pb, layout->font, prc,
				w - layout->scroll_x, offset_y, glyphs, wrap->nichar);

		i = 0;
		x = w - layout->scroll_x;
		w = 0;
		len = wrap->nichar;
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
		egal_draw_line(drawable, pb,
				x,     offset_y + layout->hline + 1,
				x + w, offset_y + layout->hline + 1);
		return;
	}

	for (i = 0; i < wrap->nichar; i++) {
		w += glyphs[i].w;
		if (w > prc->x + layout->scroll_x) {
			x = w - layout->scroll_x - glyphs[i].w;
			len = 1;
			break;
		}
	}

	for (j = i + 1; j < wrap->nichar; j++) {
		if (w + glyphs[j].x > prc->x + prc->w + layout->scroll_x)
			break;
		w  += glyphs[j].w;
		len++;
	}

	egal_draw_glyphs(drawable, pb, layout->font, prc, x, offset_y, &glyphs[i], len);

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
	egal_draw_line(drawable, pb,
			x,     offset_y + layout->hline + 1,
			x + w, offset_y + layout->hline + 1);
}

static void __layout_draw(eHandle hobj, GalDrawable drawable, GalPB pb, GalRect *prc)
{
	GuiLayout  *layout = GUI_LAYOUT_DATA(hobj);
	LayoutLine *line;
	eint        h;

	if (!layout->configure)
		layout_configure(layout);

	h    =  layout->hline;
	line = layout->top ? layout->top : layout->line_head;
	while (line) {
		eint x = line->offset_x - layout->scroll_x + layout->offset_x;
		eint y = line->offset_y - layout->scroll_y + layout->offset_y;
		GalRect lrc = {x, y, line->max_w, line->nwrap * h};

		if (y >= prc->y + prc->h)
			break;

		if (egal_rect_is_intersect(&lrc, prc)) {
			LayoutWrap *wrap = &line->wrap_head;
			GalRect tmp;
			lrc.h = h;
			while (wrap) {
				lrc.w = wrap->width;
				if (egal_rect_intersect(&tmp, &lrc, prc))
					layout_draw_wrap(drawable, pb, layout, wrap,
							line->glyphs + wrap->begin, line->underlines + wrap->begin,
							&tmp, layout->offset_x + line->offset_x, y);
				y    += layout->hline;
				lrc.y = y;
				wrap  = wrap->next;
			}
		}
		line = line->next;
	}
}

void layout_draw(eHandle hobj, GalDrawable drawable, GalPB pb, GalRect *prc)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->draw)
		o->draw(hobj, drawable, pb, prc);
}

void layout_set_text(eHandle hobj, const echar *text, eint len)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->set_text)
		o->set_text(hobj, text, len);
}

void layout_set_strings(eHandle hobj, const echar *strings)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->set_text)
		o->set_text(hobj, strings, e_strlen(strings));
}

void layout_set_offset(eHandle hobj, eint x, eint y)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_offset)
		o->set_offset(hobj, x, y);
}

void layout_set_extent(eHandle hobj, eint w, eint h)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_extent)
		o->set_extent(hobj, w, h);
}

void layout_set_table_size(eHandle hobj, eint size)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_table_size)
		o->set_table_size(hobj, size);
}

void layout_set_spacing(eHandle hobj, eint spacing)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_spacing)
		o->set_spacing(hobj, spacing);
}

void layout_set_wrap(eHandle hobj)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_wrap)
		o->set_wrap(hobj);
}

void layout_unset_wrap(eHandle hobj)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->unset_wrap)
		o->unset_wrap(hobj);
}

void layout_set_align(eHandle hobj, LayoutFlags flags)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o && o->set_align)
		o->set_align(hobj, flags & 0xf);
}

void layout_set_flags(eHandle hobj, LayoutFlags flags)
{
	GuiLayoutOrders *o = GUI_LAYOUT_ORDERS(hobj);
	if (o) {
		o->set_align(hobj, flags & 0xf);

		if (flags & LF_Wrap)
			o->set_wrap(hobj);
		else
			o->unset_wrap(hobj);
	}
}

