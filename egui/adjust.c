#include "gui.h"
#include "adjust.h"

esig_t adjust_signal_update = 0;

static void adjust_init_orders(eGeneType, ePointer);
static void adjust_init_gene(eGeneType);

eGeneType egui_genetype_adjust(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiAdjustOrders),
			adjust_init_orders,
			sizeof(GuiAdjust),
			NULL,
			NULL,
			adjust_init_gene,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

static void adjust_init_gene(eGeneType new)
{
	adjust_signal_update = e_signal_new("adjust_update",
			new,
			STRUCT_OFFSET(GuiAdjustOrders, update),
			false, 0, "%f");
}

static void adjust_reset_hook(eHandle hobj, efloat value, efloat min, efloat max, efloat step_inc, efloat page_inc)
{
	GuiAdjust *adj = GUI_ADJUST_DATA(hobj);
	adj->min   = min;
	adj->max   = max;
	adj->value = value;
	adj->step_inc = step_inc;
	adj->page_inc = page_inc;
	if (adj->hook)
		GUI_ADJUST_HOOK_ORDERS(adj->hook)->reset(adj->hook);
}

static void adjust_set_hook(eHandle hobj, efloat value)
{
	GuiAdjust *adj = GUI_ADJUST_DATA(hobj);
	if (value > adj->max - adj->min)
		value = adj->max - adj->min;
	if (adj->value != value) {
		adj->value  = value;
		if (adj->hook)
			GUI_ADJUST_HOOK_ORDERS(adj->hook)->set(adj->hook);
	}
}

static eint adjust_init(eHandle hobj, eValist vp)
{
	efloat value    = e_va_arg(vp, edouble);
	efloat min      = e_va_arg(vp, edouble);
	efloat max      = e_va_arg(vp, edouble);
	efloat step_inc = e_va_arg(vp, edouble);
	efloat page_inc = e_va_arg(vp, edouble);

	adjust_reset_hook(hobj, value, min, max, step_inc, page_inc);

	return 0;
}

static void adjust_init_orders(eGeneType new, ePointer this)
{
	GuiAdjustOrders *a = this;
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init       = adjust_init;
	a->set_hook   = adjust_set_hook;
	a->reset_hook = adjust_reset_hook;
}

eHandle egui_adjust_new(efloat value, efloat min, efloat max, efloat step_inc, efloat page_inc)
{
	return e_object_new(GTYPE_ADJUST, value, min, max, step_inc, page_inc);
}

eGeneType egui_genetype_adjust_hook(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiAdjustHookOrders),
			NULL,
			sizeof(GuiAdjustHook),
			NULL,
			NULL,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_hook(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiHookOrders),
			NULL, 0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

void egui_adjust_set_owner(eHandle adjust, eHandle owner)
{
	GUI_ADJUST_DATA(adjust)->owner = owner;
}

void egui_adjust_hook(eHandle adjust, eHandle hook)
{
	GUI_ADJUST_DATA(adjust)->hook = hook;
	GUI_ADJUST_HOOK_DATA(hook)->adjust = adjust;

	GUI_ADJUST_HOOK_ORDERS(hook)->reset(hook);
}

void egui_adjust_reset_hook(eHandle hobj, efloat value, efloat base, efloat span, efloat step_inc, efloat page_inc)
{
	GUI_ADJUST_ORDERS(hobj)->reset_hook(hobj, value, base, span, step_inc, page_inc);
}

void egui_adjust_set_hook(eHandle hobj, efloat value)
{
	GUI_ADJUST_ORDERS(hobj)->set_hook(hobj, value);
}

