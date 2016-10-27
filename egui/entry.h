#ifndef __GUI_ENTRY_H
#define __GUI_ENTRY_H

#include <egal/egal.h>

#define GTYPE_ENTRY					(egui_genetype_entry())

#define GUI_ENTRY_DATA(hobj)		((GuiEntry *)e_object_type_data(hobj, GTYPE_ENTRY))

typedef struct _GuiEntry    GuiEntry;
typedef struct _CharOffset  CharOffset;

struct _CharOffset {
	eint16 c, x;
}; 

struct _GuiEntry {
	echar *chars;
	GalGlyph    *glyphs;
	CharOffset  *offsets;
	eint max;
	eint nchar;
	eint nichar;
	eint o_ioff;
	eint s_ioff, e_ioff;

	eint old_w;
	eint total_w;
	eint offset_x;
	eint x, h;
	eint glyph_w;
	GalGlyph glyph;

	ebool visible;
	ebool bn_down;
	ebool is_show;
	ebool is_sel;

	GalPB     cpb;
	GalFont   font;
	GalCursor cursor;
};

eGeneType egui_genetype_entry(void);

eHandle egui_entry_new(eint w);
void egui_entry_set_visibility(eHandle, ebool);

#endif
