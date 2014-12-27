#include <stdio.h>
#include <stdlib.h>

#include "gui.h"
#include "box.h"
#include "vscrollbar.h"
#include "hscrollbar.h"
#include "adjust.h"
#include "clist.h"
#include "event.h"
#include "menu.h"
#include "res.h"

#define ITEM_STATUS_SELECT				(1 << 0)

#define TITLE_BAR_H						30
#define ITEM_BAR_H						25

esig_t signal_clist_cmp          = 0;
esig_t signal_clist_find         = 0;
esig_t signal_clist_insert       = 0;
esig_t signal_clist_update       = 0;
esig_t signal_clist_draw_title   = 0;
esig_t signal_clist_draw_grid    = 0;
esig_t signal_clist_draw_grid_bk = 0;

static eGeneType GTYPE_VIEW = 0;
static eGeneType GTYPE_TBAR = 0;

static eint clist_init(eHandle, eValist);
static eint clist_init_data(eHandle, ePointer);
static void clist_init_gene(eGeneType);
static void view_init_orders(eGeneType, ePointer);
static void tbar_init_orders(eGeneType, ePointer);
static void clist_init_orders(eGeneType, ePointer);
static void clist_draw_title (eHandle, GuiClist *, GalDrawable, GalPB);
static void clist_grid_draw(eHandle,
		GalDrawable, GalPB, GalFont, ClsItemBar *, int, int, int, int);
static void clist_grid_draw_bk(eHandle, GuiClist *,
		GalDrawable, GalPB, int, int, int, bool, bool, int);
static void update_ibar(GuiClist *, ClsItemBar *);
static eint clist_update(eHandle, GuiClist *, ClsItemBar *);
static eint clist_insert_ibar(eHandle,  GuiClist *, ClsItemBar *);

eGeneType egui_genetype_clist(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiClistOrders),
			clist_init_orders,
			sizeof(GuiClist),
			clist_init_data,
			NULL,
			clist_init_gene,
		};

		eGeneInfo vinfo = {
			0, view_init_orders,
			0, NULL, NULL, NULL,
		};

		eGeneInfo tinfo = {
			0, tbar_init_orders,
			0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_HBOX, GTYPE_HOOK, NULL);

		GTYPE_VIEW = e_register_genetype(&vinfo, GTYPE_WIDGET, GTYPE_EVENT, NULL);
		GTYPE_TBAR = e_register_genetype(&tinfo, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

static void clist_init_gene(eGeneType new)
{
	signal_clist_insert = e_signal_new("clist_insert",
			new,
			STRUCT_OFFSET(GuiClistOrders, insert),
			true, 0, "%p");
	signal_clist_update = e_signal_new("clist_update",
			new,
			STRUCT_OFFSET(GuiClistOrders, update),
			true, 0, "%p");
	signal_clist_cmp = e_signal_new("clist_cmp",
			new,
			STRUCT_OFFSET(GuiClistOrders, cmp),
			false, 0,
			"%n %p %p");
	signal_clist_draw_title = e_signal_new("draw_title",
			new,
			STRUCT_OFFSET(GuiClistOrders, draw_title),
			true, 0,
			"%n %n");
	signal_clist_draw_grid = e_signal_new("draw_grid",
			new,
			STRUCT_OFFSET(GuiClistOrders, draw_grid),
			false, 0,
			"%n %n %n %n %d %d %d %d");
	signal_clist_draw_grid_bk = e_signal_new("draw_grid_bk",
			new,
			STRUCT_OFFSET(GuiClistOrders, draw_grid_bk),
			true, 0,
			"%n %n %d %d %d %d %d %d");
	signal_clist_find = e_signal_new("clist_find",
			new,
			STRUCT_OFFSET(GuiClistOrders, find),
			false, 0, "%p %p");
}

static void (*hbox_set_min)(eHandle, eint, eint);
static void clist_set_min(eHandle hobj, eint w, eint h)
{
	GuiClist  *cl = GUI_CLIST_DATA(hobj);
	GuiWidget *vw = GUI_WIDGET_DATA(cl->view);
	if (w > 0)
		vw->min_w = w;
	if (h > 0)
		vw->min_h = h;
	egui_set_min(cl->vbox, 0, 0);
	hbox_set_min(hobj, 0, 0);
}

static void clist_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *e = e_genetype_orders(new, GTYPE_CELL);
	GuiClistOrders  *c = this;
	c->insert          = clist_insert_ibar;
	c->update          = clist_update;
	c->draw_title      = clist_draw_title;
	c->draw_grid       = clist_grid_draw;
	c->draw_grid_bk    = clist_grid_draw_bk;
	hbox_set_min       = w->set_min;
	w->set_min         = clist_set_min;
	e->init            = clist_init;
}

static eint clist_init_data(eHandle hobj, ePointer this)
{
	GuiClist *cl  = this;

	INIT_LIST_HEAD(&cl->item_head);
	return 0;
}

static ClsItemBar *ibar_next(GuiClist *cl, ClsItemBar *ibar)
{
	list_t *pos;

	if (list_empty(&cl->item_head))
		return NULL;

	if (ibar == NULL)
		return list_entry(cl->item_head.next, ClsItemBar, list);

	pos = ibar->list.next;
	if (pos == &cl->item_head)
		return NULL;

	return list_entry(pos, ClsItemBar, list);
}

static ClsItemBar *ibar_prev(GuiClist *cl, ClsItemBar *ibar)
{
	list_t *pos;

	if (list_empty(&cl->item_head))
		return NULL;

	if (ibar == NULL)
		return list_entry(cl->item_head.prev, ClsItemBar, list);

	pos = ibar->list.prev;
	if (pos == &cl->item_head)
		return NULL;

	return list_entry(pos, ClsItemBar, list);
}

static ClsItemBar *index_to_item(GuiClist *cl, eint id)
{
	list_t *pos;
	list_for_each(pos, &cl->item_head) {
		ClsItemBar *ibar = list_entry(pos, ClsItemBar, list);
		if (ibar->index == id)
			return ibar;
	}
	return NULL;
}

static ClsItemBar *point_to_item(GuiClist *cl, GalPoint *point)
{
	ClsItemBar *ibar = cl->top;
	eint iy, y;

	if (!ibar) return NULL;

	y  = point->y;
	iy = ibar->index  * cl->ibar_h;

	while (ibar && y + cl->offset_y >= iy + cl->ibar_h) {
		ibar = ibar_next(cl, ibar);
		iy  += cl->ibar_h;
	}

	return ibar;
}

static void set_hlight(GuiClist *cl, ClsItemBar *ibar, bool update)
{
	ClsItemBar *old;
	eint pos;

	if (!ibar) return;

	pos = ibar->index * cl->ibar_h;

	if (pos < cl->offset_y)
		cl->offset_y = pos;
	else if (pos + cl->ibar_h > cl->offset_y + cl->view_h)
		cl->offset_y = pos + cl->ibar_h - cl->view_h;
	else if ((cl->item_n * cl->ibar_h - cl->offset_y < cl->view_h) && 
			cl->offset_y > 0) {
		cl->offset_y =  cl->item_n * cl->ibar_h - cl->view_h;
		if (cl->offset_y < 0)
			cl->offset_y = 0;
	}

	old = cl->hlight;
	cl->hlight = ibar;
	cl->top    = index_to_item(cl, cl->offset_y / cl->ibar_h);

	egui_adjust_set_hook(cl->vadj, (efloat)cl->offset_y);

	if (update) {
		update_ibar(cl, ibar);
		if (old)
			update_ibar(cl, old);
	}
}

static void ibar_next_hlight(GuiClist *cl)
{
	ClsItemBar *ibar;

	if (!(ibar = ibar_next(cl, cl->hlight)))
		return;

	set_hlight(cl, ibar, true);

	e_signal_emit(OBJECT_OFFSET(cl), SIG_CLICKED, ibar);
}

static void ibar_prev_hlight(GuiClist *cl)
{
	ClsItemBar *ibar;

	if (!(ibar = ibar_prev(cl, cl->hlight)))
		return;

	set_hlight(cl, ibar, true);

	e_signal_emit(OBJECT_OFFSET(cl), SIG_CLICKED, ibar);
}

static void reset_adjust(GuiClist *cl)
{
	eint total_h = cl->item_n * cl->ibar_h;
	if (total_h > cl->view_h) {
		if (total_h - cl->offset_y < cl->view_h) {
			cl->offset_y = total_h - cl->view_h;
		}
		egui_adjust_reset_hook(cl->vadj,
			(efloat)cl->offset_y, (efloat)cl->view_h,
			(efloat)cl->item_n * cl->ibar_h,
			(efloat)cl->ibar_h, 0);
	}
	else {
		egui_adjust_reset_hook(cl->vadj, 0, 1, 1, 0, 0);
		cl->offset_y   = 0;
		cl->top        = index_to_item(cl, 0);
		egui_update_async(cl->view);
	}
}

static void clist_grid_draw_bk(eHandle hobj, GuiClist *cl,
		GalDrawable drawable, GalPB pb,
		int x, int w, int h, bool light, bool focus, int index)
{
	if (light) {
		if (focus)
			egal_set_foreground(pb, 0xc4c4ff);
		else
			egal_set_foreground(pb, 0xc4c4c4);
	}
	else if (index % 2)
		egal_set_foreground(pb, 0xe1f2f9);
	else
		egal_set_foreground(pb, 0xffffff);

	egal_fill_rect(drawable, pb, x, 0, w, h);
}

static void clist_draw_title(eHandle hobj, GuiClist *cl, GalDrawable drawable, GalPB pb)
{
	eint i, x;

	egal_set_foreground(pb, 0x8fb6fb);
	egal_fill_rect(drawable, pb, 0, 0, cl->view_w, cl->tbar_h);

	for (x = 0, i = 0; i < cl->col_n; i++) {
		ClsTitle *title = cl->titles + i;
		if (title->name) {
			GalRect rc = {x, 0, title->width, cl->tbar_h};

			egal_set_foreground(pb, 0x0);
			egui_draw_strings(drawable, pb, 0, title->name, &rc, LF_VCenter | LF_HLeft);
		}
		x += title->width;
	}
}

static void clist_grid_draw(eHandle hobj,
		GalDrawable drawable, GalPB pb, GalFont font,
		ClsItemBar *ibar, eint i, eint x, eint w, eint h)
{
	GalRect   rc = {x, 0, w, h};
	GalImage *icon = ibar->grids[i].icon;

	if (icon) {
		int oy;

		if (icon->h < h)
			oy = (h - icon->h) / 2;
		else
			oy = 0;
		if (icon->alpha)
			egal_composite_image(drawable, pb, 1, oy, icon, 0, 0, icon->w, icon->h);
		else
			egal_draw_image(drawable, pb, 1, oy, icon, 0, 0, icon->w, icon->h);
		rc.x  += icon->w + 3;
	}

	egal_set_foreground(pb, 0);
	egui_draw_strings(drawable, pb, font, ibar->grids[i].chars, &rc, LF_HLeft | LF_VCenter);
}

static void update_ibar(GuiClist *cl, ClsItemBar *ibar)
{
	GuiWidget *wid = GUI_WIDGET_DATA(cl->view);

	eint y = ibar->index * cl->ibar_h - cl->offset_y;
	eint x = 0;
	eint i = 0;

	e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_GRID_BK,
			cl->drawable, cl->pb,
			0,
			cl->view_w, cl->ibar_h,
			cl->hlight == ibar,
			WIDGET_STATUS_FOCUS(wid), ibar->index % 2);

	for (; i < cl->col_n; i++) {
		e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_GRID,
				cl->drawable, cl->pb, cl->font,
				ibar, i, x, cl->titles[i].width, cl->ibar_h);
		x += cl->titles[i].width;
	}

	egal_draw_drawable(wid->drawable, wid->pb, cl->offset_x, y, 
			cl->drawable, cl->pb, 0, 0, cl->view_w, cl->ibar_h);
}

static eint clist_update(eHandle hobj, GuiClist *cl, ClsItemBar *ibar)
{
	update_ibar(cl, ibar);
	return 0;
}

static void view_draw(eHandle hobj, GuiWidget *wid, GalRect *area, bool bk)
{
	GuiClist   *cl = wid->extra_data;
	ClsItemBar *ib = cl->top;
	eint ox = cl->offset_x;
	eint oy = cl->offset_y;
	eint iy = 0;
	eint index = 0;

	if (ib) iy = ib->index * cl->ibar_h;

	while (ib && iy < oy + cl->view_h) {
		eint ix = 0;
		eint id = 0;

		index   = ib->index;
		e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_GRID_BK,
				cl->drawable, cl->pb,
				0,
				cl->view_w, cl->ibar_h,
				cl->hlight == ib,
				GUI_STATUS_FOCUS(hobj), index);

		for (; id < cl->col_n; id++) {
			e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_GRID,
					cl->drawable, cl->pb, cl->font,
					ib, id, ix, cl->titles[id].width, cl->ibar_h);
			ix += cl->titles[id].width;
		}
		egal_draw_drawable(wid->drawable, wid->pb,
				ox,
				iy - oy,
				cl->drawable, cl->pb,
				0, 0, cl->view_w, cl->ibar_h);

		ib  = ibar_next(cl, ib);
		iy += cl->ibar_h;
	}

	index = cl->item_n;
	if (bk && iy < cl->view_h) {
		eint view_h = cl->view_h;
		while (iy < view_h) {
			e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_GRID_BK,
					cl->drawable, cl->pb,
					0, cl->view_w, cl->ibar_h,
					0, 0, index++);
			egal_draw_drawable(wid->drawable, wid->pb, 0, iy, 
					cl->drawable, cl->pb, 0, 0, cl->view_w, cl->ibar_h);
			iy += cl->ibar_h;
		}
	}

	cl->old_oy = cl->offset_y;
}

static eint view_expose(eHandle hobj, GuiWidget *widget, GalEventExpose *ent)
{
	view_draw(hobj, widget, &ent->rect, true);
	return 0;
}

static eint view_resize(eHandle hobj, GuiWidget *widget, GalEventResize *resize)
{
	GuiClist  *cl = widget->extra_data;
	GalVisualInfo info;
	eint fixed_w = 0;
	eint col_n   = cl->col_n;
	eint i;

	egal_get_visual_info(cl->drawable, &info);

	if (info.w < resize->w) {
		e_object_unref(cl->drawable);
		e_object_unref(cl->pb);
		cl->drawable = egal_drawable_new(info.w + 500, cl->ibar_h, false);
		cl->pb       = egal_pb_new(cl->drawable, NULL);
	}

	widget->rect.w = resize->w;
	widget->rect.h = resize->h;
	cl->view_w     = resize->w;
	cl->view_h     = resize->h;
	for (i = 0; i < cl->col_n; i++) {
		if (cl->titles[i].fixed_w > 0) {
			fixed_w += cl->titles[i].fixed_w;
			col_n --;
		}
	}

	for (i = 0; i < cl->col_n; i++) {
		if (cl->titles[i].fixed_w)
			cl->titles[i].width = cl->titles[i].fixed_w;
		else
			cl->titles[i].width = (resize->w - fixed_w) / col_n;
	}

	reset_adjust(cl);
	cl->top = index_to_item(cl, (int)(GUI_ADJUST_DATA(cl->vadj)->value / cl->ibar_h));
	return 0;
}

static eint view_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiClist   *cl   = GUI_WIDGET_DATA(hobj)->extra_data;
	ClsItemBar *ibar = point_to_item(cl, &ent->point);

	if (ibar && cl->ck_ibar == ibar
			&& cl->ck_num == 2
			&& ent->time - cl->ck_time < 200) {
		cl->ck_num  = 0;
		return e_signal_emit(OBJECT_OFFSET(cl), SIG_2CLICKED, ibar);
	}
	else if (ibar) {
		cl->ck_ibar = ibar;
		cl->ck_num  = 1;
		cl->ck_time = ent->time;
	}
	else {
		cl->ck_ibar = NULL;
		cl->ck_num  = 0;
	}

#ifdef __TOUCHSCREEN
	if (ibar == cl->hlight)
		cl->touch_sel = ibar;
	else 
#endif
	if (ibar && cl->hlight != ibar) {
		ClsItemBar *old = cl->hlight;
		cl->hlight = ibar;
		update_ibar(cl, ibar);
		if (old)
			update_ibar(cl, old);
		e_signal_emit(OBJECT_OFFSET(cl), SIG_CLICKED, ibar);
	}
	return 0;
}

static eint view_lbuttonup(eHandle hobj, GalEventMouse *ent)
{
	GuiClist   *cl   = GUI_WIDGET_DATA(hobj)->extra_data;
	ClsItemBar *ibar = point_to_item(cl, &ent->point);

	if (cl->ck_ibar == ibar
			&& cl->ck_num == 1
			&& ent->time - cl->ck_time < 200) {
		cl->ck_num ++;
		cl->ck_time = ent->time;
	}
	else {
		cl->ck_ibar = NULL;
		cl->ck_num  = 0;
		cl->ck_time = 0;
	}

#ifdef __TOUCHSCREEN
	if (cl->touch_sel) {
		ClsItemBar *ibar;

		ibar = point_to_item(cl, &ent->point);
		if (ibar == cl->touch_sel && cl->clicked) {
			cl->clicked(ibar, cl->clicked_args);
		}
		cl->touch_sel = NULL;
	}
#endif
	return 0;
}

static eint view_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiClist *cl = GUI_WIDGET_DATA(hobj)->extra_data;

	switch (ent->code) {
		case GAL_KC_Tab:
			return 0;
		case GAL_KC_Up:
			ibar_prev_hlight(cl);
			break;
		case GAL_KC_Down:
			ibar_next_hlight(cl);
			break;
		case GAL_KC_Enter:
			break;
		default:
			break;
	}
	return -1;
}

static eint view_focus_in(eHandle hobj)
{
	GuiClist *cl = GUI_WIDGET_DATA(hobj)->extra_data;
	if (cl->hlight == NULL)
		cl->hlight = cl->top;
	if (cl->hlight)
		update_ibar(cl, cl->hlight);
	return 0;
}

static eint view_focus_out(eHandle hobj)
{
	GuiClist *cl = GUI_WIDGET_DATA(hobj)->extra_data;
	if (cl->hlight) {
		eint _h = cl->hlight->index * cl->ibar_h;
		if (_h < cl->offset_y) {
			cl->offset_y = cl->hlight->index * cl->ibar_h;
			cl->top      = cl->hlight;
			egui_adjust_set_hook(cl->vadj, (float)cl->offset_y);
			egui_update(hobj);
		}
		else if (_h + cl->ibar_h > cl->offset_y + cl->view_h) {
			cl->offset_y += _h + cl->ibar_h - (cl->offset_y + cl->view_h);
			cl->top       = index_to_item(cl, cl->offset_y / cl->ibar_h);
			egui_adjust_set_hook(cl->vadj, (efloat)cl->offset_y);
			egui_update(hobj);
		}
		else
			update_ibar(cl, cl->hlight);
	}
	return 0;
}

static eint view_rbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiClist   *cl   = GUI_WIDGET_DATA(hobj)->extra_data;
	ClsItemBar *ibar = point_to_item(cl, &ent->point);
	if (ibar) {
		if (cl->hlight != ibar) {
			ClsItemBar *old = cl->hlight;
			cl->hlight = ibar;
			update_ibar(cl, ibar);
			if (old)
				update_ibar(cl, old);
		}
		if (cl->menu)
			egui_menu_popup(cl->menu);
	}
	return 0;
}

static void view_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);

	w->expose         = view_expose;
	w->resize         = view_resize;

	e->keydown        = view_keydown;
	e->lbuttonup      = view_lbuttonup;
	e->lbuttondown    = view_lbuttondown;
	e->rbuttondown    = view_rbuttondown;
	e->focus_in       = view_focus_in;
	e->focus_out      = view_focus_out;
}

static eint tbar_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiClist *cl = GUI_WIDGET_DATA(hobj)->extra_data;
	egui_set_focus(cl->view);
	return 0;
}

static eint tbar_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiClist *cl = wid->extra_data;
	e_signal_emit(OBJECT_OFFSET(cl), SIG_CLIST_DRAW_TITLE, wid->drawable, wid->pb);
	return 0;
}

static void tbar_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	w->expose          = tbar_expose;
	e->lbuttondown     = tbar_lbuttondown;
}

static eint clist_vadjust_update(eHandle hobj, efloat value)
{
	eHandle    own = GUI_ADJUST_DATA(hobj)->owner;
	GuiClist   *cl = GUI_WIDGET_DATA(own)->extra_data;
	cl->offset_y   = (eint)value;
	cl->top        = index_to_item(cl, cl->offset_y / cl->ibar_h);
	egui_update(cl->view);
    return 0;
}   

static eint clist_hadjust_update(eHandle hobj, efloat value)
{
	//eHandle    own = GUI_ADJUST_DATA(hobj)->owner;
	//GuiClist   *cl = GUI_CLIST_DATA(own);
	//cl->offset_x   = value;
	//egui_update(cl->view);
    return 0;
}

static void ibar_free(GuiClist *cl, ClsItemBar *ibar)
{
	eint i;
	if (cl->clear)
		cl->clear(ibar);
	for (i = 0; i < cl->col_n; i++)
		e_free(ibar->grids[i].chars);
	e_free(ibar);
}

static void __clist_delete(eHandle hobj, GuiClist *cl, ClsItemBar *ibar)
{
	eint  index = ibar->index;
	list_t *pos = &ibar->list;
	list_t *del;

	if (cl->hlight == ibar) {
		ClsItemBar *t;
		if ((t = ibar_next(cl, ibar)))
			cl->hlight = t;
		else if ((t = ibar_prev(cl, ibar)))
			cl->hlight = t;
		else {
			cl->top    = NULL;
			cl->hlight = NULL;
		}
	}

	del = pos;
	list_del(pos);
	list_for_each_after(pos, &cl->item_head) {
		ClsItemBar *t = list_entry(pos, ClsItemBar, list);
		t->index = index++;
	}
	cl->item_n = index;

	ibar_free(cl, list_entry(del, ClsItemBar, list));

	if (cl->hlight)
		set_hlight(cl, cl->hlight, false);

	reset_adjust(cl);
}

void egui_clist_delete_hlight(eHandle hobj)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	if (cl->hlight)
		__clist_delete(hobj, cl, cl->hlight);
}

void egui_clist_delete(eHandle hobj, ClsItemBar *ibar)
{
	__clist_delete(hobj, GUI_CLIST_DATA(hobj), ibar);
}

ClsItemBar *egui_clist_get_hlight(eHandle hobj)
{
	return GUI_CLIST_DATA(hobj)->hlight;
}

ClsItemBar *egui_clist_get_next(eHandle hobj, ClsItemBar *ibar)
{
	return ibar_next(GUI_CLIST_DATA(hobj), ibar);
}

ClsItemBar *egui_clist_get_first(eHandle hobj)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	list_t    *pos = cl->item_head.next;
	if (pos != &cl->item_head)
		return list_entry(pos, ClsItemBar, list);
	return NULL;
}

ClsItemBar *egui_clist_get_last(eHandle hobj)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	list_t    *pos = cl->item_head.prev;
	if (pos != &cl->item_head)
		return list_entry(pos, ClsItemBar, list);
	return NULL;
}

static ClsItemBar *ibar_new_valist(GuiClist *cl, eValist vp)
{
	ClsItemBar *ibar;
	eint        i;
	ibar = e_malloc(sizeof(ClsItemBar) + sizeof(ClsItemGrid) * cl->col_n);
	for (i = 0; i < cl->col_n; i++) {
		echar *chars = e_va_arg(vp, ePointer);
		if (chars)
			ibar->grids[i].chars = e_strdup(chars);
		else
			ibar->grids[i].chars = NULL;
		ibar->grids[i].icon  = NULL;
	}
	return ibar;
}

static ClsItemBar *ibar_new(GuiClist *cl, ClsItemGrid *grids)
{
	ClsItemBar *ibar;
	eint        i;
	ibar = e_malloc(sizeof(ClsItemBar) + sizeof(ClsItemGrid) * cl->col_n);
	ibar->add_data = 0;
	for (i = 0; i < cl->col_n; i++) {
		ibar->grids[i].icon  = grids[i].icon;
		if (grids[i].chars)
			ibar->grids[i].chars = e_strdup(grids[i].chars);
		else
			ibar->grids[i].chars = NULL;
	}
	return ibar;
}

ClsItemBar *egui_clist_ibar_new(eHandle hobj, ClsItemGrid *grids)
{
	return ibar_new(GUI_CLIST_DATA(hobj), grids);
}

ClsItemBar *egui_clist_ibar_new_valist(eHandle hobj, ...)
{
	GuiClist   *cl = GUI_CLIST_DATA(hobj);
	eValist     vp;
	ClsItemBar *ibar;

	e_va_start(vp, hobj);
	ibar = ibar_new_valist(cl, vp);
	e_va_end(vp);

	return ibar;
}

ClsItemBar *egui_clist_append(eHandle hobj, ClsItemGrid *grids)
{
	GuiClist   *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *ibar;

	ibar = ibar_new(cl, grids);

	ibar->index = cl->item_n++;
	list_add_tail(&ibar->list, &cl->item_head);
	if (!cl->top)
		cl->top = ibar;

	if (!cl->hlight) {
		cl->hlight = ibar;
		set_hlight(cl, cl->hlight, false);
	}
	reset_adjust(cl);

	return ibar;
}

ClsItemBar *egui_clist_append_valist(eHandle hobj, ...)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *ibar;
	eValist     vp;

	e_va_start(vp, hobj);
	ibar = ibar_new_valist(cl, vp);
	e_va_end(vp);

	ibar->index = cl->item_n++;
	list_add_tail(&ibar->list, &cl->item_head);
	if (!cl->top)
		cl->top = ibar;

	if (!cl->hlight) {
		cl->hlight = ibar;
		set_hlight(cl, cl->hlight, false);
	}
	reset_adjust(cl);

	return ibar;
}

ClsItemBar *egui_clist_find(eHandle hobj, ePointer data)
{
	GuiClist    *cl   = GUI_CLIST_DATA(hobj);
	eSignalSlot *slot = e_signal_get_slot(hobj, SIG_CLIST_FIND);
	ClsItemBar  *ibar = NULL;

	if (slot && slot->func) {
		eint (*cmp)(eHandle, ClsItemBar *, ePointer) = (ePointer)slot->func;
		list_t     *pos;
		list_for_each(pos, &cl->item_head) {
			ibar = list_entry(pos, ClsItemBar, list);
			if (cmp(hobj, ibar, data) >= 0)
				return ibar;
		}
	}
	return NULL;
}

#define INSERT_BEFORE		0
#define INSERT_AFTER		1
static void ibar_cmp_insert(eHandle hobj, GuiClist *cl, ClsItemBar *new, eint p)
{
	eSignalSlot *slot = e_signal_get_slot(hobj, SIG_CLIST_CMP);

	if (slot && slot->func) {
		eint (*cmp)(eHandle, ePointer, ClsItemBar *, ClsItemBar *) = (ePointer)slot->func;

		ClsItemBar *ibar = NULL;
		list_t     *pos;
		list_for_each(pos, &cl->item_head) {
			ibar = list_entry(pos, ClsItemBar, list);
			if (cmp(hobj, slot->data, ibar, new) > 0)
				break;
		}

		list_add_tail(&new->list, pos);

		if (!ibar)
			cl->item_n = 0;
		else if (pos != &cl->item_head)
			cl->item_n = ibar->index;

		for (pos = &new->list; pos != &cl->item_head; pos = pos->next) {
			ibar = list_entry(pos, ClsItemBar, list);
			ibar->index = cl->item_n++;
		}
	}
	else {
		new->index = cl->item_n++;
		list_add_tail(&new->list, &cl->item_head);
	}

	if (!cl->hlight) {
		cl->hlight = new;
		set_hlight(cl, cl->hlight, false);
	}
	reset_adjust(cl);
}

static eint clist_insert_ibar(eHandle hobj, GuiClist *cl, ClsItemBar *ibar)
{
	ibar_cmp_insert(hobj, cl, ibar, INSERT_BEFORE);
	return 0;
}

void egui_clist_insert_ibar(eHandle hobj, ClsItemBar *ibar)
{
	ibar_cmp_insert(hobj, GUI_CLIST_DATA(hobj), ibar, INSERT_BEFORE);
}

ClsItemBar *egui_clist_insert(eHandle hobj, ClsItemGrid *grids)
{
	GuiClist   *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *ibar = ibar_new(cl, grids);

	ibar_cmp_insert(hobj, cl, ibar, INSERT_BEFORE);

	return ibar;
}

ClsItemBar *egui_clist_insert_valist(eHandle hobj, ...)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *ibar;
	eValist     vp;

	e_va_start(vp, hobj);
	ibar = ibar_new_valist(cl, vp);
	e_va_end(vp);

	ibar_cmp_insert(hobj, cl, ibar, INSERT_BEFORE);

	return ibar;
}

ClsItemBar *egui_clist_hlight_insert(eHandle hobj, ClsItemGrid *grids)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *new;
	list_t     *pos;

	new = ibar_new(cl, grids);

	if (cl->hlight)
		list_add_tail(&new->list, &cl->hlight->list);
	else
		list_add(&new->list, &cl->item_head);

	cl->item_n = 0;
	list_for_each(pos, &cl->item_head) {
		ClsItemBar *ibar = list_entry(pos, ClsItemBar, list);
		ibar->index = cl->item_n++;
	}

	cl->hlight = new;
	reset_adjust(cl);
	set_hlight(cl, cl->hlight, false);

	return new;
}

void egui_clist_set_hlight(eHandle hobj, ClsItemBar *ibar)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);

	cl->hlight = ibar;
	set_hlight(cl, cl->hlight, true);
}

void egui_clist_unset_hlight(eHandle hobj)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	if (cl->hlight) {
		ClsItemBar *ibar = cl->hlight;
		cl->hlight = NULL;
		e_signal_emit(hobj, SIG_CLIST_UPDATE, ibar);
	}
}

ClsItemBar *egui_clist_hlight_insert_valist(eHandle hobj, ...)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	ClsItemBar *new;
	list_t     *pos;
	eValist     vp;

	e_va_start(vp, hobj);
	new = ibar_new_valist(cl, vp);
	e_va_end(vp);

	if (cl->hlight)
		list_add_tail(&new->list, &cl->hlight->list);
	else
		list_add(&new->list, &cl->item_head);

	cl->item_n = 0;
	list_for_each(pos, &cl->item_head) {
		ClsItemBar *ibar = list_entry(pos, ClsItemBar, list);
		ibar->index = cl->item_n++;
	}

	cl->hlight = new;
	set_hlight(cl, cl->hlight, false);
	reset_adjust(cl);

	return new;
}

void egui_clist_empty(eHandle hobj)
{
	GuiClist *cl  = GUI_CLIST_DATA(hobj);
	list_t   *pos = cl->item_head.next;
	list_t   *t;

	if (cl->item_n == 0)
		return;

	while (pos != &cl->item_head) {
		t = pos;
		pos = t->next;
		list_del(t);
		ibar_free(cl, list_entry(t, ClsItemBar, list));
	}

	cl->top      = NULL;
	cl->hlight   = NULL;
	cl->item_n   = 0;
	cl->offset_y = 0;

	egui_update_async(hobj);
}

ePointer clist_get_grid_data(eHandle hobj, ClsItemBar *ibar, eint index)
{
	return ibar->grids[index].chars;
}

void egui_clist_set_clear(eHandle hobj, void (*clear)(ClsItemBar *))
{
	GUI_CLIST_DATA(hobj)->clear = clear;
}

void egui_clist_set_column_width(eHandle hobj, eint index, eint w)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	cl->titles[index].fixed_w = w;
}

void egui_clist_set_item_height(eHandle hobj, eint h)
{
	GuiClist *cl = GUI_CLIST_DATA(hobj);
	GalVisualInfo info;

	egal_get_visual_info(cl->drawable, &info);

	e_object_unref(cl->drawable);
	e_object_unref(cl->pb);
	cl->drawable = egal_drawable_new(info.w, h, false);
	cl->pb       = egal_pb_new(cl->drawable, NULL);
	cl->ibar_h   = h;
	reset_adjust(cl);
}

void egui_clist_title_show(eHandle hobj)
{
	egui_show(GUI_CLIST_DATA(hobj)->tbar, true);
}

void egui_clist_title_hide(eHandle hobj)
{
	egui_hide(GUI_CLIST_DATA(hobj)->tbar, true);
}

static eint clist_init(eHandle hobj, eValist vp)
{
	GuiClist  *cl = GUI_CLIST_DATA(hobj);
	GuiWidget *wid;
	eint i;

	const echar **titles = e_va_arg(vp, const echar **);
	eint             num = e_va_arg(vp, eint);

	cl->tbar_h   = TITLE_BAR_H;
	cl->ibar_h   = ITEM_BAR_H;
	cl->col_n    = num;
	cl->item_n   = 0;
	cl->offset_x = 0;
	cl->offset_y = 0;
	cl->old_oy   = -1;

	cl->titles = e_calloc(num, sizeof(ClsTitle));
	for (i = 0; i < num; i++) {
		cl->titles[i].width = 1;
		e_strncpy(cl->titles[i].name, titles[i], TITLE_MAX);
	}

#ifdef __TOUCHSCREEN
	cl->touch_sel = NULL;
#endif
	cl->drawable = egal_drawable_new(1000, cl->ibar_h, false);
	cl->pb       = egal_pb_new(cl->drawable, NULL);

	egui_set_expand(hobj, true);
	cl->vbox = egui_vbox_new();
	egui_set_expand(cl->vbox, true);

	cl->tbar = e_object_new(GTYPE_TBAR);
	wid = GUI_WIDGET_DATA(cl->tbar);
	wid->min_w  = 1;
	wid->min_h  = TITLE_BAR_H;
	wid->max_h  = TITLE_BAR_H;
	wid->rect.w = 1;
	wid->rect.h = TITLE_BAR_H;
	wid->extra_data = cl;
	widget_set_status(wid, GuiStatusVisible | GuiStatusMouse);
	e_signal_emit(cl->tbar, SIG_REALIZE);

	cl->view = e_object_new(GTYPE_VIEW);
	wid = GUI_WIDGET_DATA(cl->view);
	wid->rect.w = 1;
	wid->rect.h = 1;
	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->extra_data = cl;
	widget_set_status(wid, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);
	e_signal_emit(cl->view, SIG_REALIZE);

	cl->hadj  = egui_adjust_new(0, 1, 1, 0, 0);
	cl->hsbar = egui_hscrollbar_new(true);
	egui_hide(cl->hsbar, true);
	egui_adjust_set_owner(cl->hadj, cl->view);
	e_signal_connect(cl->hadj, SIG_ADJUST_UPDATE, clist_hadjust_update);
	egui_adjust_hook(cl->hadj, cl->hsbar);

	cl->vadj  = egui_adjust_new(0, 1, 1, 0, 0);
	cl->vsbar = egui_vscrollbar_new(true);
	egui_adjust_set_owner(cl->vadj, cl->view);
	e_signal_connect(cl->vadj, SIG_ADJUST_UPDATE, clist_vadjust_update);
	egui_adjust_hook(cl->vadj, cl->vsbar);

	egui_add(cl->vbox, cl->tbar);
	egui_add(cl->vbox, cl->view);
	egui_add(cl->vbox, cl->hsbar);
	egui_add(hobj, cl->vbox);
	egui_add(hobj, cl->vsbar);
	return 0;
}

eHandle egui_clist_new(const echar *titles[], eint num)
{
	return e_object_new(GTYPE_CLIST, titles, num);
}

void egui_clist_update_ibar(eHandle hobj, ClsItemBar *ibar)
{
	egui_signal_emit1(hobj, SIG_CLIST_UPDATE, ibar);
}

void egui_clist_set_menu(eHandle hobj, eHandle menu)
{
	GuiClist  *cl = GUI_CLIST_DATA(hobj);
	cl->menu = menu;
}
