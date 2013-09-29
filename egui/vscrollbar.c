#include "gui.h"
#include "widget.h"
#include "vscrollbar.h"
#include "adjust.h"
#include "res.h"

#define SCALE	16

static GuiResItem *vscr_res = NULL;

static GalImage *up_image     = NULL;
static GalImage *down_image   = NULL;
static GalImage *slot_image   = NULL;
static GalImage *drag_image   = NULL;
static GalImage *top_image    = NULL;
static GalImage *bottom_image = NULL;

static eint vscrollbar_init(eHandle, eValist);
static void vscrollbar_init_orders(eGeneType, ePointer);
static eint vscrollbar_expose(eHandle, GuiWidget *, GalEventExpose *);

eGeneType egui_genetype_vscrollbar(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			vscrollbar_init_orders,
			0, NULL, NULL, NULL,
		};

		vscr_res     = egui_res_find(GUI_RES_HANDLE, _("vscrollbar")); 
		up_image     = egui_res_find_item(vscr_res, _("up"));
		down_image   = egui_res_find_item(vscr_res, _("down"));
		slot_image   = egui_res_find_item(vscr_res, _("slot"));
		drag_image   = egui_res_find_item(vscr_res, _("drag"));
		top_image    = egui_res_find_item(vscr_res, _("top"));
		bottom_image = egui_res_find_item(vscr_res, _("bottom"));

		gtype = e_register_genetype(&info, GTYPE_SCROLLBAR, NULL);
	}
	return gtype;
}

static bool __vscrollbar_update(eHandle hobj, GuiScrollBar *bar, eint val)
{
	GalRect rc;

	if (bar->steps + val < 0)
		val = -bar->steps;
	else if (bar->steps + val > bar->bar_span << SCALE)
		val = (bar->bar_span << SCALE) - bar->steps;

	if (val == 0)
		return false;

	bar->steps += val;
	bar->drag_rn.rect.y = bar->bn_size + (bar->steps >> SCALE);
	bar->drag_rn.rect.h = bar->drag_size;

	rc.x = 0;
	rc.w = bar->slot_rn.rect.w;
	if (val < 0) {
		rc.y = bar->drag_rn.rect.y;
		val  = -val;
	}
	else {
		rc.y = bar->drag_rn.rect.y - ((val + 0xffff) >> SCALE);
	}
	rc.h = bar->drag_size + ((val + 0xffff) >> SCALE);

	egui_update_rect(hobj, &rc);
	return true;
}

static bool vscrollbar_update(eHandle hobj, GuiScrollBar *bar, eint steps)
{
	return __vscrollbar_update(hobj, bar, steps - bar->steps);
}

static eint vscrollbar_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
	GuiAdjustHookOrders *hook = GUI_ADJUST_HOOK_ORDERS(hobj);

	b->slot_rn.rect.x = 0;
	b->slot_rn.rect.y = b->bn_size;
	b->slot_rn.rect.w = slot_image->w;
	b->slot_rn.rect.h = resize->h - b->bn_size * 2;
	b->drag_rn.rect.w = b->slot_rn.rect.w;
	b->drag_rn.rect.h = b->slot_rn.rect.h;

	b->bn_rn2.rect.x  = 0;
	b->bn_rn2.rect.y  = resize->h - b->bn_size;
	b->bn_rn2.rect.w  = slot_image->w;
	b->bn_rn2.rect.h  = b->bn_size;

	b->slot_size = b->slot_rn.rect.h;
	b->drag_size = b->slot_size;

	wid->rect.h  = resize->h;

	hook->reset(hobj);
	return 0;
}

static void vscrollbar_init_orders(eGeneType new, ePointer this)
{
	GuiScrollBarOrders  *s = e_genetype_orders(new, GTYPE_SCROLLBAR);
	GuiWidgetOrders     *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders         *c = e_genetype_orders(new, GTYPE_CELL);

	w->resize = vscrollbar_resize;
	w->expose = vscrollbar_expose;
	s->update = vscrollbar_update;
	c->init   = vscrollbar_init;
}

static eint vscrollbar_expose(eHandle hobj, GuiWidget *widget, GalEventExpose *exp)
{
	GuiScrollBar *scroll = GUI_SCROLLBAR_DATA(hobj);
	GuiScrollRegion *rn;

	euint x, y, h;
	eint steps = scroll->steps >> SCALE;
	GalRect rc = scroll->bn_rn1.rect;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		rn = &scroll->bn_rn1;
		if (rn->is_down)
			x = rc.x + widget->rect.w;
		else
			x = rc.x;
		 egal_draw_image(widget->drawable, exp->pb,
				 rc.x, rc.y,
				 up_image, x, rc.y, rc.w, rc.h);
	}

	rc = scroll->bn_rn2.rect;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		rn = &scroll->bn_rn2;
		if (rn->is_down)
			x = rc.x + widget->rect.w;
		else
			x = rc.x;
		y = rc.y - rn->rect.y;
		egal_draw_image(widget->drawable, exp->pb,
				rc.x, rc.y,
				down_image, x, y, rc.w, rc.h);
	}

	rc.x = 0;
	rc.y = scroll->bn_size;
	rc.w = slot_image->w;
	rc.h = steps;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		egui_draw_vbar(widget->drawable, exp->pb,
				NULL, slot_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
	}

	y = rc.y = scroll->bn_size + steps;
	h = rc.h = scroll->drag_size;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		if (rc.y == y && rc.h == h)
			egui_draw_vbar(widget->drawable, exp->pb,
					top_image, drag_image, bottom_image, rc.x, rc.y, rc.w, rc.h, 0);
		else if (rc.y > y && rc.y + rc.h < y + h)
			egui_draw_vbar(widget->drawable, exp->pb,
					NULL, drag_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
		else if (rc.y > y)
			egui_draw_vbar(widget->drawable, exp->pb,
					NULL, drag_image, bottom_image, rc.x, rc.y, rc.w, rc.h, 0);
		else
			egui_draw_vbar(widget->drawable, exp->pb,
					top_image, drag_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);

	}

	rc.y = scroll->bn_size + steps + scroll->drag_size;
	rc.h = scroll->slot_size - steps - scroll->drag_size;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		egui_draw_vbar(widget->drawable, exp->pb,
				NULL, slot_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
	}

	return 0;
}

static bool bn1_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	if (!bar->bn_rn1.is_down) {
		bar->bn_rn1.is_down = true;
		egui_update_rect(hobj, &bar->bn_rn1.rect);
	}
	return __vscrollbar_update(hobj, bar, -bar->bn_steps);
}

static bool bn2_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	if (!bar->bn_rn2.is_down) {
		bar->bn_rn2.is_down = true;
		egui_update_rect(hobj, &bar->bn_rn2.rect);
	}
	return __vscrollbar_update(hobj, bar, bar->bn_steps);
}

static void bn1_up(eHandle hobj, GuiScrollBar *bar)
{
	bar->bn_rn1.is_down = false;
	egui_update_rect(hobj, &bar->bn_rn1.rect);
}

static void bn2_up(eHandle hobj, GuiScrollBar *bar)
{
	bar->bn_rn2.is_down = false;
	egui_update_rect(hobj, &bar->bn_rn2.rect);
}

static bool slot_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	return false;
}

static bool  drag_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	bar->old = y;
	return false;
}

static bool drag_move(eHandle hobj, GuiScrollBar *b, eint x, eint y)
{
	eint val = (y - b->old) << SCALE;
	if (__vscrollbar_update(hobj, b, val)) {
		b->old = y;
		return true;
	}
	return false;
}

static eint vscrollbar_init(eHandle hobj, eValist vp)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
	GuiWidget    *w = GUI_WIDGET_DATA(hobj);

	b->auto_hide = e_va_arg(vp, bool);

	b->bn_size = up_image->h;
	b->bn_rn1.rect.x  = 0;
	b->bn_rn1.rect.y  = 0;
	b->bn_rn1.rect.w  = slot_image->w;
	b->bn_rn1.rect.h  = b->bn_size;

	b->slot_rn.rect.x = 0;
	b->slot_rn.rect.y = b->bn_size;
	b->slot_rn.rect.w = slot_image->w;
	b->slot_rn.rect.h = DRAG_SIZE_MIN;
	b->drag_rn.rect.w = b->slot_rn.rect.w;
	b->drag_rn.rect.h = b->slot_rn.rect.h;

	b->bn_rn2.rect.x  = 0;
	b->bn_rn2.rect.y  = DRAG_SIZE_MIN + b->bn_size;
	b->bn_rn2.rect.w  = slot_image->w;
	b->bn_rn2.rect.h  = b->bn_size;

	b->bn_rn1.up = bn1_up;
	b->bn_rn2.up = bn2_up;
	b->bn_rn1.down = bn1_down;
	b->bn_rn2.down = bn2_down;
	b->slot_rn.down = slot_down;
	b->drag_rn.down = drag_down;
	b->drag_rn.move = drag_move;

	b->slot_size = b->slot_rn.rect.h;
	b->drag_size = b->slot_size;

	w->min_w = slot_image->w;
	w->max_w = slot_image->w;
	w->min_h = b->bn_size * 2 + DRAG_SIZE_MIN + 2;
	w->rect.w = slot_image->w;
	w->rect.h = w->min_h;

	e_signal_emit(hobj, SIG_REALIZE);

	return 0;
}

eHandle egui_vscrollbar_new(bool a)
{
	return e_object_new(GTYPE_VSCROLLBAR, a);
}

