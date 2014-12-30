#include <egui/gui.h>
#include <egui/widget.h>
#include <egui/event.h>

#include "drawable.h"

static void drawable_init_orders(eGeneType new, ePointer this);
static eint drawable_init(eHandle, eValist vp);
eGeneType egui_genetype_drawable(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			drawable_init_orders,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

static void drawable_init_orders(eGeneType new, ePointer this)
{
	eCellOrders *c = e_genetype_orders(new, GTYPE_CELL);
	c->init = drawable_init;
}

static eint drawable_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	
	eint w = e_va_arg(vp, eint);
	eint h = e_va_arg(vp, eint);

	wid->rect.w = w;
	wid->rect.h = h;
	wid->min_w  = w;
	wid->min_h  = h;
	wid->max_w  = 0;
	wid->max_h  = 0;
	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);

	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_drawable_new(eint w, eint h)
{
	return e_object_new(GTYPE_DRAWABLE, w, h);
}

