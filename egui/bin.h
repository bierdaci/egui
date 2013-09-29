#ifndef __GUI_BIN_H
#define __GUI_BIN_H

#include <egal/region.h>

#define GTYPE_BIN						(egui_genetype_bin())

typedef struct _GuiBin       GuiBin;
typedef struct _GuiBinOrders GuiBinOrders;

#define GUI_BIN_ORDERS(wid)			((GuiBinOrders *)e_object_type_orders((wid), GTYPE_BIN))
#define GUI_BIN_DATA(wid)			((GuiBin       *)e_object_type_data((wid), GTYPE_BIN))

#define BIN_X_AXIS				0x10000
#define BIN_Y_AXIS				0x20000

#define BIN_DIR_NEXT			0
#define BIN_DIR_PREV			1

struct _GuiBin {
	eHandle focus;
	eHandle enter;
	eHandle grab;

	GalRegion  region_mask;
	GuiWidget *head;
	GuiWidget *tail;
	bool is_bin;
};

struct _GuiBinOrders {
	void (*add_spacing)(eHandle, eint);
	void (*switch_focus)(eHandle, eHandle);
	void (*request_layout)(eHandle, eHandle, eint, eint, bool, bool);
	eHandle (*next_child)(GuiBin *, eHandle, eint, eint);
};

eGeneType egui_genetype_bin(void);
eHandle egui_bin_new(int w, int h);

eint bin_prev_focus(eHandle, eint);
eint bin_next_focus(eHandle, eint);
bool bin_can_focus(GuiWidget *);

#endif
