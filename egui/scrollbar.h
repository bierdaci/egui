#ifndef __GUI_SCROLLBAR_H
#define __GUI_SCROLLBAR_H

#include <egal/egal.h>

#define GTYPE_SCROLLBAR						(egui_genetype_scrollbar())

#define GUI_SCROLLBAR_ORDERS(obj)			((GuiScrollBarOrders *)e_object_type_orders(obj, GTYPE_SCROLLBAR))
#define GUI_SCROLLBAR_DATA(obj)				((GuiScrollBar       *)e_object_type_data(obj, GTYPE_SCROLLBAR))

#define DRAG_SIZE_MIN						5

typedef struct _GuiScrollBar		GuiScrollBar;
typedef struct _GuiScrollBarOrders	GuiScrollBarOrders;

typedef struct _GuiScrollRegion		GuiScrollRegion;

struct _GuiScrollRegion {
	GalRect rect;
	ebool is_down;

	ebool (*down)(eHandle, GuiScrollBar *, eint x, eint y);
	void (*up)  (eHandle, GuiScrollBar *);
	ebool (*move)(eHandle, GuiScrollBar *, eint x, eint y);
};

struct _GuiScrollBar {
	eHandle connect;

	eint steps, old;
	eint bn_steps;
	eint move_val;

	euint slot_size, drag_size;

	euint bar_span;
	euint adj_span;

#ifdef WIN32
	euint adj_step;
	euint adj_recoup;

	euint bar_step;
	euint bar_recoup;
#else
	euint64 adj_step;
	euint64 adj_recoup;

	euint64 bar_step;
	euint64 bar_recoup;
#endif

	euint bn_size;
	GuiScrollRegion bn_rn1, bn_rn2;
	GuiScrollRegion drag_rn, slot_rn;
	GuiScrollRegion *act_rn;

	euint   time;
	eTimer  timer;

	ebool auto_hide;
};

struct _GuiScrollBarOrders {
	ebool (*update)(eHandle, GuiScrollBar *, eint);
};

eGeneType egui_genetype_scrollbar(void);

#endif
