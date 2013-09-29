#ifndef __GUI_BOX_H
#define __GUI_BOX_H

#include <egal/region.h>

#define GTYPE_BOX				(egui_genetype_box())
#define GTYPE_VBOX				(egui_genetype_vbox())
#define GTYPE_HBOX				(egui_genetype_hbox())

#define GUI_BOX_DATA(hobj)		((GuiBox *)e_object_type_data((hobj), GTYPE_BOX))

typedef struct _GuiBox  GuiBox;
typedef struct _BoxPack BoxPack;

typedef enum {
	BoxLayout_EDGE,
	BoxLayout_SPREAD,
	BoxLayout_START,
	BoxLayout_START_SPACING,
	BoxLayout_END,
	BoxLayout_END_SPACING,
	BoxLayout_CENTER,
} GuiBoxLayout;

typedef enum {
	BoxAlignCenter,
	BoxAlignStart,
	BoxAlignEnd,
} GuiBoxAlign;

typedef enum {
	BoxPack_WIDGET,
	BoxPack_SPACING,
} BoxPackType;

struct _BoxPack {
	BoxPackType type;
	eint req_w, req_h;
	union {
		eHandle hobj;
		eint    spacing;
	} obj;
	GuiWidget *wid;
	BoxPack   *next;
};

struct _GuiBox {
	GuiBoxAlign  align  : 2;
	GuiBoxLayout layout : 4;
	eint empty          : 10;
	eint spacing        : 16;
	eint border_width   : 16;
	eint child_num      : 16;
	efloat expand_min;
	efloat children_min;
	BoxPack *head;
};

eGeneType egui_genetype_box(void);
eGeneType egui_genetype_vbox(void);
eGeneType egui_genetype_hbox(void);

eHandle egui_box_new(void);
eHandle egui_vbox_new(void);
eHandle egui_hbox_new(void);
void egui_box_add(eHandle, eHandle);
void egui_box_add_spacing(eHandle, eint);
void egui_box_set_border_width(eHandle, eint);
void egui_box_set_spacing(eHandle, eint);
void egui_box_set_layout  (eHandle, GuiBoxLayout);
void egui_box_set_align   (eHandle, GuiBoxAlign);

#endif
