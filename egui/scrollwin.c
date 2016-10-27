#include "gui.h"
#include "bin.h"
#include "event.h"
#include "widget.h"
#include "adjust.h"
#include "scrollwin.h"

static eint scrollwin_init(eHandle, eValist);
static eint scrollwin_init_data(eHandle, ePointer);
static void scrollwin_init_orders(eGeneType, ePointer);

eGeneType egui_genetype_scrollwin(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			scrollwin_init_orders,
			sizeof(GuiScrollWin),
			scrollwin_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, GTYPE_HOOK, NULL);
	}
	return gtype;
}

static void scrollwin_hook_v(eHandle hobj, eHandle hook)
{
	egui_adjust_hook(GUI_SCROLLWIN_DATA(hobj)->vadj, hook);
}

static void scrollwin_hook_h(eHandle hobj, eHandle hook)
{
	egui_adjust_hook(GUI_SCROLLWIN_DATA(hobj)->hadj, hook);
}

static void scrollwin_reset_adjust(eHandle hobj, GuiWidget *cw)
{
	GuiWidget    *pw  = GUI_WIDGET_DATA(hobj);
	GuiScrollWin *scr = GUI_SCROLLWIN_DATA(hobj);

	if (cw->rect.w > pw->rect.w) {
		egui_adjust_reset_hook(scr->hadj,
				(efloat)scr->offset_x, (efloat)pw->rect.w,
				(efloat)cw->rect.w, 10, 0);
	}
	else
		egui_adjust_reset_hook(scr->hadj, 0, 1, 1, 0, 0);

	if (cw->rect.h > pw->rect.h) {
		egui_adjust_reset_hook(scr->vadj,
				(efloat)scr->offset_y, (efloat)pw->rect.h,
				(efloat)cw->rect.h, 10, 0);
	}
	else
		egui_adjust_reset_hook(scr->vadj, 0, 1, 1, 0, 0);
}

static eint scrollwin_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	if (bin->head)
		scrollwin_reset_adjust(hobj, bin->head);
	return 0;
}

static void scrollwin_add(eHandle hobj, eHandle cobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (!bin->head) {
		GuiWidget *cw = GUI_WIDGET_DATA(cobj);
		cw->parent = hobj;
		cw->rect.x = 0;
		cw->rect.y = 0;

		STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, cw, prev, next);

		GUI_WIDGET_ORDERS(hobj)->put(hobj, cobj);

		scrollwin_reset_adjust(hobj, cw);
	}
}

static void scrollwin_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool fixed, ebool a)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (bin->head) {
		GuiWidget     *wid = bin->head;
		GalEventResize res;
		if (!fixed) {
			res.w = req_w;
			res.h = req_h;
		}
		else {
			res.w = wid->rect.w;
			res.h = wid->rect.h;
		}
		e_signal_emit(OBJECT_OFFSET(wid), SIG_RESIZE, &res);

		scrollwin_reset_adjust(hobj, wid);
	}
}

static void scrollwin_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	GuiHookOrders   *h = e_genetype_orders(new, GTYPE_HOOK);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	b->request_layout  = scrollwin_request_layout;

	w->add             = scrollwin_add;
	w->resize          = scrollwin_resize;

	h->hook_v          = scrollwin_hook_v;
	h->hook_h          = scrollwin_hook_h;

	c->init            = scrollwin_init;
}

static eint scrollwin_vadjust_update(eHandle hobj, efloat value)
{
	eHandle own = GUI_ADJUST_DATA(hobj)->owner;
	GuiScrollWin *scr = GUI_SCROLLWIN_DATA(own);
	GuiBin       *bin = GUI_BIN_DATA(own);

	scr->offset_y = (eint)value;

	if (bin->head) {
		eHandle cobj = OBJECT_OFFSET(bin->head);
		egui_move(cobj, -scr->offset_x, -scr->offset_y);
		egui_update(cobj);
	}
    return 0;
}

static eint scrollwin_hadjust_update(eHandle hobj, efloat value)
{
	eHandle own = GUI_ADJUST_DATA(hobj)->owner;
	GuiScrollWin *scr = GUI_SCROLLWIN_DATA(own);
	GuiBin       *bin = GUI_BIN_DATA(own);

	scr->offset_x = (eint)value;

	if (bin->head) {
		eHandle cobj = OBJECT_OFFSET(bin->head);
		egui_move(cobj, -scr->offset_x, -scr->offset_y);
		egui_update(cobj);
	}
    return 0;
}

static eint scrollwin_init_data(eHandle hobj, ePointer this)
{
	GuiScrollWin *scrollwin = this;

	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);
	egui_unset_status(hobj, GuiStatusTransparent);

	scrollwin->vadj = egui_adjust_new(0, 1, 1, 0, 0);
	scrollwin->hadj = egui_adjust_new(0, 1, 1, 0, 0);
	egui_adjust_set_owner(scrollwin->vadj, hobj);
	egui_adjust_set_owner(scrollwin->hadj, hobj);
	e_signal_connect(scrollwin->vadj, SIG_ADJUST_UPDATE, scrollwin_vadjust_update);
	e_signal_connect(scrollwin->hadj, SIG_ADJUST_UPDATE, scrollwin_hadjust_update);

	return 0;
}

static eint scrollwin_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->rect.w = 1;
	wid->rect.h = 1;
	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_scrollwin_new(void)
{
	return e_object_new(GTYPE_SCROLLWIN);
}
