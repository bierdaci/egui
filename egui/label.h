#ifndef __GUI_LABEL_H__
#define __GUI_LABEL_H__

#include <egal/egal.h>

#define GTYPE_LABEL						(egui_genetype_label())
#define GTYPE_SIMPLE_LABEL				(egui_genetype_simple_label())
#define GUI_LABEL_DATA(hobj)			((GuiLabel *)e_object_type_data(hobj, GTYPE_LABEL))
#define GUI_LABEL_ORDERS(hobj)			((GuiLabelOrders *)e_object_type_orders(hobj, GTYPE_LABEL))

#define DEFAULT_TABLE_SIZE				8

typedef struct _GuiLabel				GuiLabel;
typedef struct _GuiLabelOrders			GuiLabelOrders;

struct _GuiLabel {
	const echar *strings;
	eint nchars;
};

struct _GuiLabelOrders {
	void (*set_strings)(eHandle, GuiLabel *, const echar *);
};

eHandle egui_simple_label_new(const echar *);
eGeneType egui_genetype_simple_label(void);
eHandle egui_label_new(const echar *);
eGeneType egui_genetype_label(void);

void egui_label_set_text_with_mnemonic(eHandle, const echar *, eint);
void egui_label_insert_text(eHandle, const echar *, eint);
void egui_label_draw(GalDrawable, GalPB, eHandle, GalRect *);
void egui_label_set_table_size(eHandle, eint);
void egui_label_set_extent(eHandle, eint, eint);
void egui_label_set_spacing(eHandle, eint);
void egui_label_set_spacing(eHandle, eint);

#endif
