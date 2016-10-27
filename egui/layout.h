#ifndef __GUI_LAYOUT_H__
#define __GUI_LAYOUT_H__

#include <egal/egal.h>

#define GTYPE_LAYOUT					(egui_genetype_layout())
#define GUI_LAYOUT_DATA(hobj)			((GuiLayout *)e_object_type_data(hobj, GTYPE_LAYOUT))
#define GUI_LAYOUT_ORDERS(hobj)			((GuiLayoutOrders *)e_object_type_orders(hobj, GTYPE_LAYOUT))

#define DEFAULT_TABLE_SIZE				8

typedef struct _GuiLayout		GuiLayout;
typedef struct _LayoutLine		LayoutLine;
typedef struct _GuiLayoutOrders	GuiLayoutOrders;

typedef enum {
	LF_HLeft    = 1,
	LF_HCenter  = 2,
	LF_HRight   = 3,
	LF_VTop     = 1 << 2,
	LF_VCenter  = 2 << 2,
	LF_VBottom  = 3 << 2,
	LF_Wrap     = 1 << 4,
} LayoutFlags;

struct _GuiLayout {
	eint w, h;
	eint max_w;
	eint total_h;
	eint nchar;
	eint nline;
	eint spacing;
	eint hline;
	LayoutLine *line_head;
	LayoutLine *line_tail;
	LayoutLine *top;
	eint table_size;
	eint table_w;
	eint scroll_x;
	eint scroll_y;
	eint offset_x;
	eint offset_y;
	GalFont font;
	eHandle vadj, hadj;

	ebool configure:1;
	ebool is_wrap:1;
	LayoutFlags align:6;
};

struct _GuiLayoutOrders {
	void (*set_table_size)(eHandle, eint);
	void (*set_extent)(eHandle, eint, eint);
	void (*set_offset)(eHandle, eint, eint);
	void (*set_spacing)(eHandle, eint);
	void (*set_align)(eHandle, LayoutFlags);
	void (*set_wrap)(eHandle);
	void (*unset_wrap)(eHandle);

	void (*draw)(eHandle, GalDrawable drawable, GalPB pb, GalRect *prc);
};

eGeneType egui_genetype_layout(void);
void layout_draw(eHandle, GalDrawable, GalPB, GalRect *);
void layout_set_text(eHandle, const echar *, eint);
void layout_set_strings(eHandle, const echar *);
void layout_set_table_size(eHandle, eint);
void layout_set_extent(eHandle, eint, eint);
void layout_set_offset(eHandle, eint, eint);
void layout_set_spacing(eHandle, eint);
void layout_set_wrap(eHandle);
void layout_unset_wrap(eHandle);
void layout_set_flags(eHandle, LayoutFlags);
void layout_set_align(eHandle, LayoutFlags);
void layout_get_extents(GuiLayout *, eint *, eint *);

#endif
