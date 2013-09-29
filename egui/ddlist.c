#include "gui.h"
#include "res.h"
#include "bin.h"
#include "event.h"
#include "menu.h"
#include "ddlist.h"

static GalImage *down_img = NULL;

static eint (*bin_keydown)(eHandle, GalEventKey *);

static eint ddlist_init_data(eHandle, ePointer);
static void ddlist_init_orders(eGeneType, ePointer);

eGeneType egui_genetype_ddlist(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GuiResItem *res;

		eGeneInfo info = {
			0,
			ddlist_init_orders,
			sizeof(GuiDDList),
			ddlist_init_data,
			NULL, NULL,
		};

		res = egui_res_find(GUI_RES_HANDLE, _("common"));
		down_img = egui_res_find_item(res,  _("down"));

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, GTYPE_STRINGS, NULL);
	}
	return gtype;
}

static eint ddlist_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiDDList *ddls = GUI_DDLIST_DATA(hobj);
	GuiWidget *iwid = GUI_WIDGET_DATA(ddls->item);

	if (ent->code == GAL_KC_Up) {
		if (iwid->prev) {
			ddls->item = OBJECT_OFFSET(iwid->prev);
			ddls->strings = egui_get_strings(ddls->item);
			egui_update(hobj);
		}
		return -1;
	}
	else if (ent->code == GAL_KC_Down) {
		if (iwid->next) {
			ddls->item = OBJECT_OFFSET(iwid->next);
			ddls->strings = egui_get_strings(ddls->item);
			egui_update(hobj);
		}
		return -1;
	}
	else if (ent->code == GAL_KC_Enter || ent->code == GAL_KC_space) {
		egui_menu_popup(ddls->menu);
		return -1;
	}

	return 0;
}

static eint ddlist_enter(eHandle hobj, eint x, eint y)
{
	GUI_DDLIST_DATA(hobj)->enter = true;
	egui_update(hobj);
	return 0;
}

static eint ddlist_leave(eHandle hobj)
{
	GUI_DDLIST_DATA(hobj)->enter = false;
	egui_update(hobj);
	return 0;
}

static eint ddlist_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	egui_menu_popup(GUI_DDLIST_DATA(hobj)->menu);
	return 0;
}

static eint ddlist_lbuttonup(eHandle hobj, GalEventMouse *ent)
{
	return 0;
}

static eint ddlist_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiDDList *ddlist = GUI_DDLIST_DATA(hobj);

	if (ddlist->enter)
		egal_set_foreground(wid->pb, 0xcfc3b3);
	else
		egal_set_foreground(wid->pb, 0xa7977f);

	egal_fill_rect(wid->drawable, wid->pb,
			wid->offset_x + 1, wid->offset_y + 1,
			wid->rect.w - 2, wid->rect.h - 2);

	if (WIDGET_STATUS_FOCUS(wid)) {
		egal_set_foreground(wid->pb, 0);
		egal_draw_rect(wid->drawable, wid->pb,
				wid->offset_x + 3, wid->offset_y + 3,
				wid->rect.w - 6, wid->rect.h - 6);
	}

	egal_set_foreground(wid->pb, 0x877c6c);
	egal_draw_rect(wid->drawable, wid->pb, wid->offset_x, wid->offset_y, wid->rect.w, wid->rect.h);

	if (ddlist->strings) {
		GalRect rc = {wid->offset_x + 5, wid->offset_y, wid->rect.w, wid->rect.h};
		egal_set_foreground(wid->pb, 0);
		egui_draw_strings(wid->drawable, wid->pb, 0, ddlist->strings, &rc, LF_HLeft | LF_VCenter);
	}

	egal_composite_image(wid->drawable, wid->pb,
			wid->offset_x + wid->rect.w - down_img->w - 5,
			wid->offset_y + (wid->rect.h - down_img->h) / 2,
			down_img,
			0, 0, down_img->w, down_img->h);

	return 0;
}

static eint item_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	//ePointer   func = e_signal_get_func(hobj, SIG_LBUTTONDOWN);
	GuiDDList *ddls = e_signal_get_data(hobj, SIG_LBUTTONDOWN);
	ddls->strings = egui_get_strings(hobj);
	ddls->item = hobj;
	egui_update(OBJECT_OFFSET(ddls));
	e_signal_emit(OBJECT_OFFSET(ddls), SIG_CLICKED, hobj);
	return e_signal_emit_default(hobj, SIG_LBUTTONDOWN, mevent);
}

static void ddlist_add(eHandle hobj, eHandle cobj)
{
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);
	GuiDDList *ddlist = GUI_DDLIST_DATA(hobj);
	GuiMenu   *menu   = GUI_MENU_DATA(ddlist->menu);

	e_signal_connect1(cobj, SIG_LBUTTONDOWN, item_lbuttondown, ddlist);
	e_signal_lock(cobj, SIG_LBUTTONDOWN);
	egui_add(ddlist->menu, cobj);

	if (!ddlist->head) {
		ddlist->head = cobj;
		ddlist->item = cobj;
		ddlist->strings = egui_get_strings(cobj);
	}
	ddlist->tail = cobj;

	widget->min_w = GUI_WIDGET_DATA(menu->box)->rect.w - 2;
	egui_request_layout(widget->parent, 0, 0, 0, false, true);
}

static void ddlist_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);

	bin_keydown    = e->keydown;

	e->enter       = ddlist_enter;
	e->leave       = ddlist_leave;
	e->keydown     = ddlist_keydown;
	e->lbuttonup   = ddlist_lbuttonup;
	e->lbuttondown = ddlist_lbuttondown;
	e->focus_in    = (ePointer)egui_update;
	e->focus_out   = (ePointer)egui_update;

	w->add         = ddlist_add;
	w->expose      = ddlist_expose;
}

static eint ddlist_menu_pos_cb(eHandle hobj, eint *_x, eint *_y, eint *_h)
{
	eint x, y, h;
	GalVisualInfo vinfo;
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);
	GuiDDList *ddlist = GUI_DDLIST_DATA(hobj);
	GuiMenu *menu = GUI_MENU_DATA(ddlist->menu);
	GalRect *rect = &GUI_WIDGET_DATA(menu->box)->rect;

	egal_get_visual_info(egal_root_window(), &vinfo);
	egal_window_get_origin(widget->drawable, &x, &y);

	x += widget->offset_x;
	y += widget->offset_y;
	if (x + rect->w > vinfo.w)
		x -= rect->w;

	if (rect->h > vinfo.h) {
		y = 0;
		h = vinfo.h;
	}
	else if (y + rect->h > vinfo.h) {
		h  = rect->h;
		y -= rect->h;
		if (y < 0)
			y = vinfo.h - h;
	}
	else h = rect->h;

	*_x = x - 1;
	*_y = y - 1;
	*_h = h + 2;

	return 0;
}

static eint ddlist_init_data(eHandle hobj, ePointer this)
{
	GuiDDList *ddlist = GUI_DDLIST_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->min_w = down_img->w;
	wid->min_h = 35;
	wid->max_w = 1;
	wid->max_h = 1;
	widget_set_status(wid, GuiStatusVisible | GuiStatusMouse | GuiStatusActive);

	ddlist->menu = egui_menu_new(hobj, ddlist_menu_pos_cb);

	return 0;
}

eHandle egui_ddlist_new(void)
{
	return e_object_new(GTYPE_DDLIST);
}
