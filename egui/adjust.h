#ifndef __GUI_ADJUST_H
#define __GUI_ADJUST_H

#include <elib/object.h>

#define GTYPE_HOOK							(egui_genetype_hook())
#define GTYPE_ADJUST						(egui_genetype_adjust())
#define GTYPE_ADJUST_HOOK					(egui_genetype_adjust_hook())

#define GUI_ADJUST_DATA(obj)				((GuiAdjust       *)e_object_type_data(obj, GTYPE_ADJUST))
#define GUI_ADJUST_ORDERS(obj)				((GuiAdjustOrders *)e_object_type_orders(obj, GTYPE_ADJUST))

#define GUI_ADJUST_HOOK_DATA(obj)			((GuiAdjustHook        *)e_object_type_data(obj, GTYPE_ADJUST_HOOK))
#define GUI_ADJUST_HOOK_ORDERS(obj)			((GuiAdjustHookOrders  *)e_object_type_orders(obj, GTYPE_ADJUST_HOOK))

#define GUI_HOOK_ORDERS(obj)				((GuiHookOrders *)e_object_type_orders(obj, GTYPE_HOOK))

#define SIG_ADJUST_UPDATE					adjust_signal_update

typedef struct _GuiAdjust				GuiAdjust;
typedef struct _GuiAdjustOrders			GuiAdjustOrders;
typedef struct _GuiAdjustHook			GuiAdjustHook;
typedef struct _GuiAdjustHookOrders		GuiAdjustHookOrders;
typedef struct _GuiHookOrders			GuiHookOrders;

struct _GuiAdjust {
	efloat min;
	efloat max;
	efloat value;
	efloat step_inc;
	efloat page_inc;

	eHandle hook;
	eHandle owner;
};

struct _GuiAdjustOrders {
	void (*reset_hook)(eHandle, efloat, efloat, efloat, efloat, efloat);
	void (*set_hook)(eHandle, efloat);

	eint (*update)(eHandle, efloat);
};

struct _GuiAdjustHook {
	eHandle adjust;
};

struct _GuiAdjustHookOrders {
	void (*reset)(eHandle);
	void (*set  )(eHandle);
};

struct _GuiHookOrders {
	void (*hook_v)(eHandle, eHandle);
	void (*hook_h)(eHandle, eHandle);
};

eGeneType egui_genetype_hook(void);
eGeneType egui_genetype_adjust(void);
eGeneType egui_genetype_adjust_hook(void);

eHandle egui_adjust_new(efloat, efloat, efloat, efloat, efloat);
void egui_adjust_hook(eHandle, eHandle);
void egui_adjust_reset_hook(eHandle, efloat, efloat, efloat, efloat, efloat);
void egui_adjust_set_hook(eHandle, efloat);
void egui_adjust_set_owner(eHandle, eHandle);

extern esig_t adjust_signal_update;

#endif
