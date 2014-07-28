#include <egui/gui.h>
#include <egui/widget.h>
#include <egui/event.h>

#include "drawable.h"

static void drawable_init_orders(eGeneType new, ePointer this);
static eint drawable_init  (eHandle, eValist vp);
static eint drawable_expose_bg(eHandle, GuiWidget *, GalEventExpose *);

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
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	w->expose_bg  = drawable_expose_bg;
	c->init       = drawable_init;
}

static eint drawable_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	egal_set_foreground(ent->pb, 0xaaaaaa);
	egal_fill_rect(wid->drawable, ent->pb, ent->rect.x, ent->rect.y, ent->rect.w, ent->rect.h);
	return 0;
}

static eint drawable_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	wid->rect.w = 1;
	wid->rect.h = 1;
	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->max_w  = 0;
	wid->max_h  = 0;
	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);

	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_drawable_new(void)
{
	return e_object_new(GTYPE_DRAWABLE);
}

