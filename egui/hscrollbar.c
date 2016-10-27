#include <egal/egal.h>

#include "gui.h"
#include "widget.h"
#include "hscrollbar.h"
#include "adjust.h"
#include "res.h"

#define SCALE	16

static GuiResItem *hscr_res = NULL;

static GalImage *left_image     = NULL;
static GalImage *right_image   = NULL;
static GalImage *slot_image   = NULL;
static GalImage *drag_image   = NULL;
static GalImage *l_image    = NULL;
static GalImage *r_image = NULL;

static eint hscrollbar_init(eHandle, eValist);
static void hscrollbar_init_orders(eGeneType, ePointer);
static eint hscrollbar_expose(eHandle, GuiWidget *, GalEventExpose *);

eGeneType egui_genetype_hscrollbar(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			hscrollbar_init_orders,
			0, NULL, NULL, NULL,
		};

		hscr_res    = egui_res_find(GUI_RES_HANDLE, _("hscrollbar")); 
		l_image     = egui_res_find_item(hscr_res, _("l"));
		r_image     = egui_res_find_item(hscr_res, _("r"));
		slot_image  = egui_res_find_item(hscr_res, _("slot"));
		drag_image  = egui_res_find_item(hscr_res, _("drag"));
		left_image  = egui_res_find_item(hscr_res, _("left"));
		right_image = egui_res_find_item(hscr_res, _("right"));

		gtype = e_register_genetype(&info, GTYPE_SCROLLBAR, NULL);
	}
	return gtype;
}

static ebool __hscrollbar_update(eHandle hobj, GuiScrollBar *b, eint val)
{
	GalRect rc;

	if (b->steps + val < 0)
		val = -b->steps;
	else if ((b->steps + val) > (eint)(b->bar_span << SCALE))
		val = (b->bar_span << SCALE) - b->steps;

	if (val == 0)
		return efalse;

	b->steps += val;
	b->drag_rn.rect.x = b->bn_size + (b->steps >> SCALE);
	b->drag_rn.rect.w = b->drag_size;

	rc.y = 0;
	rc.h = b->slot_rn.rect.h;
	if (val < 0) {
		rc.x = b->drag_rn.rect.x;
		val  = -val;
	}
	else {
		rc.x = b->drag_rn.rect.x - ((val + 0xffff) >> SCALE);
	}
	rc.w = b->drag_size + ((val + 0xffff) >> SCALE);

	egui_update_rect(hobj, &rc);
	return etrue;
}

static ebool hscrollbar_update(eHandle hobj, GuiScrollBar *b, eint steps)
{
	return __hscrollbar_update(hobj, b, steps - b->steps);
}

static eint hscrollbar_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
	GuiAdjustHookOrders *hook = GUI_ADJUST_HOOK_ORDERS(hobj);

	b->slot_rn.rect.x = b->bn_size;
	b->slot_rn.rect.y = 0;
	b->slot_rn.rect.w = resize->w - b->bn_size * 2;
	b->slot_rn.rect.h = slot_image->h;
	b->drag_rn.rect.w = b->slot_rn.rect.w;
	b->drag_rn.rect.h = b->slot_rn.rect.h;

	b->bn_rn2.rect.x  = resize->w - b->bn_size;
	b->bn_rn2.rect.y  = 0;
	b->bn_rn2.rect.w  = b->bn_size;
	b->bn_rn2.rect.h  = slot_image->h;

	b->slot_size = b->slot_rn.rect.w;
	b->drag_size = b->slot_size;

	wid->rect.w  = resize->w;

	hook->reset(hobj);
	return 0;
}

static void hscrollbar_init_orders(eGeneType new, ePointer this)
{
	GuiScrollBarOrders  *s = e_genetype_orders(new, GTYPE_SCROLLBAR);
	GuiWidgetOrders     *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders         *c = e_genetype_orders(new, GTYPE_CELL);

	w->resize = hscrollbar_resize;
	w->expose = hscrollbar_expose;
	s->update = hscrollbar_update;
	c->init   = hscrollbar_init;
}

static eint hscrollbar_expose(eHandle hobj, GuiWidget *widget, GalEventExpose *exp)
{
	GuiScrollBar *scroll = GUI_SCROLLBAR_DATA(hobj);
	GuiScrollRegion *rn;

	eint x, y, w;
	eint steps = scroll->steps >> SCALE;
	GalRect rc = scroll->bn_rn1.rect;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		rn = &scroll->bn_rn1;
		if (rn->is_down)
			y = rc.y + widget->rect.h;
		else
			y = rc.y;
		 egal_draw_image(widget->drawable, exp->pb,
				 rc.x, rc.y,
				 left_image, rc.x, y, rc.w, rc.h);
	}

	rc = scroll->bn_rn2.rect;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		rn = &scroll->bn_rn2;
		if (rn->is_down)
			y = rc.y + widget->rect.h;
		else
			y = rc.y;
		x = rc.x - rn->rect.x;
		egal_draw_image(widget->drawable, exp->pb,
				 rc.x, rc.y,
				 right_image, x, y, rc.w, rc.h);
	}

	rc.x = scroll->bn_size;
	rc.y = 0;
	rc.w = steps;
	rc.h = slot_image->h;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		egui_draw_hbar(widget->drawable, exp->pb,
				NULL, slot_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
	}

	x = rc.x = scroll->bn_size + steps;
	w = rc.w = scroll->drag_size;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		if (rc.x == x && rc.w == w)
			egui_draw_hbar(widget->drawable, exp->pb,
					l_image, drag_image, r_image, rc.x, rc.y, rc.w, rc.h, 0);
		else if (rc.x > x && rc.x + rc.w < x + w)
			egui_draw_hbar(widget->drawable, exp->pb,
					NULL, drag_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
		else if (rc.x > x)
			egui_draw_hbar(widget->drawable, exp->pb,
					NULL, drag_image, r_image, rc.x, rc.y, rc.w, rc.h, 0);
		else
			egui_draw_hbar(widget->drawable, exp->pb,
					l_image, drag_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
	}

	rc.x = scroll->bn_size + steps + scroll->drag_size;
	rc.w = scroll->slot_size - steps - scroll->drag_size;
	if (egal_rect_intersect(&rc, &rc, &exp->rect)) {
		egui_draw_hbar(widget->drawable, exp->pb,
				NULL, slot_image, NULL, rc.x, rc.y, rc.w, rc.h, 0);
	}

	return 0;
}

static ebool bn1_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	if (!bar->bn_rn1.is_down) {
		bar->bn_rn1.is_down = etrue;
		egui_update_rect(hobj, &bar->bn_rn1.rect);
	}
	return __hscrollbar_update(hobj, bar, -bar->bn_steps);
}

static ebool bn2_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	if (!bar->bn_rn2.is_down) {
		bar->bn_rn2.is_down = etrue;
		egui_update_rect(hobj, &bar->bn_rn2.rect);
	}
	return __hscrollbar_update(hobj, bar, bar->bn_steps);
}

static void bn1_up(eHandle hobj, GuiScrollBar *bar)
{
	bar->bn_rn1.is_down = efalse;
	egui_update_rect(hobj, &bar->bn_rn1.rect);
}

static void bn2_up(eHandle hobj, GuiScrollBar *bar)
{
	bar->bn_rn2.is_down = efalse;
	egui_update_rect(hobj, &bar->bn_rn2.rect);
}

static ebool slot_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	return efalse;
}

static ebool  drag_down(eHandle hobj, GuiScrollBar *bar, eint x, eint y)
{
	bar->old = x;
	return efalse;
}

static ebool drag_move(eHandle hobj, GuiScrollBar *b, eint x, eint y)
{
	eint val = (x - b->old) << SCALE;
	if (__hscrollbar_update(hobj, b, val)) {
		b->old = x;
		return etrue;
	}
	return efalse;
}

static eint hscrollbar_init(eHandle hobj, eValist vp)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
	GuiWidget    *w = GUI_WIDGET_DATA(hobj);

	b->auto_hide = e_va_arg(vp, ebool);

	b->bn_size = left_image->w;
	b->bn_rn1.rect.x  = 0;
	b->bn_rn1.rect.y  = 0;
	b->bn_rn1.rect.w  = b->bn_size;
	b->bn_rn1.rect.h  = slot_image->h;

	b->slot_rn.rect.x = b->bn_size;
	b->slot_rn.rect.y = 0;
	b->slot_rn.rect.w = DRAG_SIZE_MIN;
	b->slot_rn.rect.h = slot_image->h;
	b->drag_rn.rect.w = b->slot_rn.rect.w;
	b->drag_rn.rect.h = b->slot_rn.rect.h;

	b->bn_rn2.rect.x  = DRAG_SIZE_MIN + b->bn_size;
	b->bn_rn2.rect.y  = 0;
	b->bn_rn2.rect.w  = b->bn_size;
	b->bn_rn2.rect.h  = slot_image->h;

	b->bn_rn1.up = bn1_up;
	b->bn_rn2.up = bn2_up;
	b->bn_rn1.down = bn1_down;
	b->bn_rn2.down = bn2_down;
	b->slot_rn.down = slot_down;
	b->drag_rn.down = drag_down;
	b->drag_rn.move = drag_move;

	b->slot_size = b->slot_rn.rect.w;
	b->drag_size = b->slot_size;

	w->min_h = slot_image->h;
	w->max_h = slot_image->h;
	w->min_w = b->bn_size * 2 + DRAG_SIZE_MIN;
	w->rect.w = w->min_w;
	w->rect.h = slot_image->h;

	e_signal_emit(hobj, SIG_REALIZE);

	return 0;
}

eHandle egui_hscrollbar_new(ebool a)
{
	return e_object_new(GTYPE_HSCROLLBAR, a);
}

