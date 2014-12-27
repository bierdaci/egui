#ifndef __GUI_TEXT_H
#define __GUI_TEXT_H

#include <egal/egal.h>
#include <egui/char.h>

#define GTYPE_TEXT					(egui_genetype_text())

#define GUI_TEXT_DATA(hobj)			((GuiText *)e_object_type_data(hobj, GTYPE_TEXT))

typedef struct _GuiText    GuiText;
typedef struct _TextCursor TextCursor;
typedef struct _TextSelect TextSelect;
typedef struct _TextLine   TextLine;
typedef struct _TextWrap   TextWrap;

struct _TextSelect {
	eint s_id;
	eint s_ioff;
	eint e_id;
	eint e_ioff;
};

struct _TextWrap {
	TextLine *line;
	eint   begin;
	eint   coffset;
	eint   nchar;
	eint   nichar;
	eint   id;
	eint   left_x;
	eint16 width;
	eint16 width_old;
	eint   begin_old; 
	TextWrap *next;
	TextWrap *prev;
};

struct _TextLine {
	eint16 id;
	eint16 nwrap;
	eint16 over;
	eint16 lack;
	eint   nchar;
	eint   nichar;
	eint   max_w:31;
	bool   is_lf:1;
	eint   offset_y;
	eint   load_nchar;
	eint   load_nichar;

	echar     *chars;
	GalGlyph  *glyphs;
	eint      *coffsets;
	echar     *underlines;
	TextWrap  *wrap_tail;
	TextWrap  *wrap_top;
	TextWrap   wrap_head;
	struct _TextLine *next;
	struct _TextLine *prev;
};

struct _TextCursor {
	TextLine *line;
	eint ioff;
	eint offset_x;
	TextWrap *wrap;
	GalPB pb;

	eint x, y, h;
	eint ox, oy, oh;
	bool show;
};

struct _GuiText {
	eint width;
	eint height;
	eint total_h;
	eint update_h;
	eint old_y;
	eint nchar;
	eint nline;
	eint spacing;
	eint hline;
	TextLine *top;
	TextLine *line_head;
	TextLine *line_tail;
	eint space_mul;
	eint table_w;
	eint offset_x;
	eint offset_y;
	GalFont font;
	TextCursor cursor;
	TextSelect select;
	GalRegion *sel_rgn;

	eHandle vadj, hadj;
	GalCursor gal_cursor;

	bool only_read    : 1;
	bool is_wrap      : 1;
	bool is_sel       : 1;
	bool bn_down      : 1;
	bool is_underline : 1;
};

eGeneType egui_genetype_text(void);
eHandle egui_text_new(eint w, eint h);
void egui_text_append(eHandle, const echar *, eint);
void egui_text_clear(eHandle hobj);
void egui_text_set_only_read(eHandle, bool);
void egui_text_set_underline(eHandle hobj);

#endif
