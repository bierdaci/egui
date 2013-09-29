#include "gui.h"
#include "event.h"
#include "widget.h"
#include "progressbar.h"
#include "res.h"

static eint progress_bar_init(eHandle, eValist);
static void progress_bar_init_orders(eGeneType, ePointer);
static eint progress_bar_expose(eHandle, GuiWidget *, GalEventExpose *);

static GalImage *slot_image = NULL;
static GalImage *drag_image = NULL;
static GalImage *l_image    = NULL;
static GalImage *r_image    = NULL;

eGeneType egui_genetype_progress_bar(void)
{
	static eGeneType gtype = 0;
	GuiResItem *res = NULL;

	if (!gtype) {
		eGeneInfo info = {
			0,
			progress_bar_init_orders,
			sizeof(GuiProgressbar),
			NULL, NULL, NULL,
		};

		res         = egui_res_find(GUI_RES_HANDLE, _("hscrollbar")); 
		l_image     = egui_res_find_item(res, _("l"));
		r_image     = egui_res_find_item(res, _("r"));
		slot_image  = egui_res_find_item(res, _("slot"));
		drag_image  = egui_res_find_item(res, _("drag"));

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

static void progress_bar_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init    = progress_bar_init;
	w->expose  = progress_bar_expose;
}

static eint progress_bar_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiProgressbar *bar = GUI_PROGRESS_BAR_DATA(hobj);
	GalRect *prc = &ent->rect;

	egal_set_foreground(wid->pb, 0x303030);

	if (bar->fraction == 0.) {
		egui_draw_hbar(wid->drawable, wid->pb,
				NULL, slot_image, NULL, prc->x, 0, prc->w, wid->rect.h, 0);
	}
	else if (bar->fraction >= 1.) {
		egui_draw_hbar(wid->drawable, wid->pb,
				l_image, drag_image, r_image, prc->x, 0, prc->w, wid->rect.h, 0);
	}
	else {
		eint p = bar->fraction * wid->rect.w;
		egui_draw_hbar(wid->drawable, wid->pb,
				l_image, drag_image, r_image, prc->x, 0, p, wid->rect.h, 0);
		egui_draw_hbar(wid->drawable, wid->pb,
				NULL, slot_image, NULL, prc->x, 0, wid->rect.w - p, wid->rect.h, 0);
	}

	return 0;
}

static eint progress_bar_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	wid->rect.w = 1;
	wid->rect.h = slot_image->h;
	wid->min_w  = 1;
	wid->min_h  = slot_image->h;
	wid->max_w  = 0;
	wid->max_h  = 1;

	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_progress_bar_new(void)
{
	return e_object_new(GTYPE_PROGRESS_BAR);
}
