#include "gui.h"
#include "event.h"
#include "widget.h"
#include "scrollbar.h"
#include "adjust.h"

#define SCALE							16
#define SCALE_SHIFT						(1 << SCALE)

static eint scrollbar_init_data(eHandle, ePointer);
static void scrollbar_init_orders(eGeneType, ePointer);
static eint scrollbar_mousemove(eHandle, GalEventMouse *);
static eint scrollbar_lbuttondown(eHandle, GalEventMouse *);
static eint scrollbar_lbuttonup(eHandle, GalEventMouse *);
static eint scrollbar_enter(eHandle, eint, eint);
static eint scrollbar_leave(eHandle);
static void scrollbar_reset(eHandle);
static void scrollbar_set(eHandle);

eGeneType egui_genetype_scrollbar(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiScrollBarOrders),
			scrollbar_init_orders,
			sizeof(GuiScrollBar),
			scrollbar_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, GTYPE_ADJUST_HOOK, NULL);
	}
	return gtype;
}

static void scrollbar_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders      *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiAdjustHookOrders *h = e_genetype_orders(new, GTYPE_ADJUST_HOOK);

	h->set   = scrollbar_set;
	h->reset = scrollbar_reset;

	e->leave       = scrollbar_leave;
	e->enter       = scrollbar_enter;
	e->mousemove   = scrollbar_mousemove;
	e->lbuttonup   = scrollbar_lbuttonup;
	e->lbuttondown = scrollbar_lbuttondown;
}

static eint scrollbar_init_data(eHandle hobj, ePointer this)
{
	GuiScrollBar *b = this;
	b->move_val     = -1;
	egui_set_status(hobj, GuiStatusVisible | GuiStatusMouse);
	return 0;
}

static eint scrollbar_enter(eHandle hobj, eint x, eint y)
{
	return 0;
}

static eint scrollbar_leave(eHandle hobj)
{
	return 0;
}

static eint timer_down_cb(eTimer timer, euint num, ePointer args)
{
	GuiScrollBar *b = args;
	b->time += num;
	if (b->time > 5) {
		eHandle hobj = OBJECT_OFFSET(args);
		GuiAdjustHook *h = GUI_ADJUST_HOOK_DATA(hobj);
		if (b->act_rn->down(hobj, b, 0, 0) && h->adjust) {
			efloat val = (efloat)(((b->steps >> SCALE) * b->adj_step + b->adj_recoup) >> SCALE);
			GUI_ADJUST_DATA(h->adjust)->value = val;
			e_signal_emit(h->adjust, SIG_ADJUST_UPDATE, val);
		}
	}
	return 0;
}

static eint timer_drag_cb(eTimer timer, euint num, ePointer args)
{
	GuiScrollBar *b = args;
	b->time += num;
	if (b->time > 5 && b->move_val >= 0) {
		eHandle     hobj = OBJECT_OFFSET(args);
		GuiAdjustHook *h = GUI_ADJUST_HOOK_DATA(hobj);
		GUI_ADJUST_DATA(h->adjust)->value = (efloat)b->move_val;
		e_signal_emit(h->adjust, SIG_ADJUST_UPDATE, (efloat)b->move_val);
		b->move_val = -1;
		b->time     = 0;
	}
	return 0;
}

static eint scrollbar_lbuttondown(eHandle hobj, GalEventMouse *event)
{
	GuiScrollBar  *b = GUI_SCROLLBAR_DATA(hobj);
	GuiAdjustHook *h = GUI_ADJUST_HOOK_DATA(hobj);
	eint x = event->point.x;
	eint y = event->point.y;

	if (egal_rect_point_in(&b->bn_rn1.rect, x, y)) {
		b->act_rn = &b->bn_rn1;
		b->timer = e_timer_add(50, timer_down_cb, b);
	}
	else if (egal_rect_point_in(&b->bn_rn2.rect, x, y)) {
		b->act_rn = &b->bn_rn2;
		b->timer = e_timer_add(50, timer_down_cb, b);
	}
	else if (egal_rect_point_in(&b->drag_rn.rect, x, y)) {
		b->act_rn = &b->drag_rn;
		b->time   = 5;
		b->timer  = e_timer_add(20, timer_drag_cb, b);
	}
	else
		b->act_rn = &b->slot_rn;

	if (b->act_rn->down(hobj, b, x, y) && h->adjust) {
		eint64 val = (eint64)(((b->steps >> SCALE) * b->adj_step + b->adj_recoup) >> SCALE);
		GUI_ADJUST_DATA(h->adjust)->value = (efloat)val;
		e_signal_emit(h->adjust, SIG_ADJUST_UPDATE, (efloat)val);
	}

	if (h->adjust) {
		GuiAdjust *adj = GUI_ADJUST_DATA(h->adjust);
		if (adj->owner && GUI_STATUS_ACTIVE(adj->owner))
			egui_set_focus(adj->owner);
	}
#ifdef WIN32
	egal_grab_pointer(GUI_WIDGET_DATA(hobj)->window, true, 0);
#endif
	return 0;
}

static eint scrollbar_lbuttonup(eHandle hobj, GalEventMouse *event)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);

	if (b->timer) {
		GuiAdjustHook *h = GUI_ADJUST_HOOK_DATA(hobj);
		e_timer_del(b->timer);
		b->timer = 0;
		b->time  = 0;
		if (b->move_val >= 0) {
			GUI_ADJUST_DATA(h->adjust)->value = (efloat)b->move_val;
			e_signal_emit(h->adjust, SIG_ADJUST_UPDATE, (efloat)b->move_val);
			b->move_val = -1;
		}
	}
	if (b->act_rn) {
		if (b->act_rn->up) {
			b->act_rn->up(hobj, b);
		}
		b->act_rn = NULL;
	}
#ifdef WIN32
	egal_ungrab_pointer(GUI_WIDGET_DATA(hobj)->window);
#endif
	return 0;
}

static eint scrollbar_mousemove(eHandle hobj, GalEventMouse *event)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);

	if (!b->act_rn)
		return 0;

	if (b->act_rn->move) {
		GuiAdjustHook *h = GUI_ADJUST_HOOK_DATA(hobj);

		if (b->act_rn->move(hobj, b, event->point.x, event->point.y)
				&& h->adjust) {
			b->move_val = (eint)(((b->steps >> SCALE) * b->adj_step + b->adj_recoup) >> SCALE);
		}
	}

	return 0;
}

static void scrollbar_reset(eHandle hobj)
{
	GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
	eHandle  adjust = GUI_ADJUST_HOOK_DATA(hobj)->adjust;
	if (adjust) {
		GuiAdjust  *a = GUI_ADJUST_DATA(adjust);

		b->drag_size  = (euint)(b->slot_size * (a->min / a->max));
		if (b->drag_size < DRAG_SIZE_MIN)
			b->drag_size = DRAG_SIZE_MIN;
		b->bar_span   = b->slot_size - b->drag_size;
		b->adj_span   = (eint)(a->max - a->min);

		b->adj_step   = (euint)((efloat)b->adj_span / b->bar_span * SCALE_SHIFT);
		b->adj_recoup = (b->adj_span << SCALE) - b->adj_step * b->bar_span;

		b->bar_step   = (euint)((efloat)b->bar_span / b->adj_span * SCALE_SHIFT);
		b->bar_recoup = (b->bar_span << SCALE) - b->bar_step * b->adj_span;
		b->bn_steps   = (eint)(a->step_inc * b->bar_step + b->bar_recoup);

		if (GUI_SCROLLBAR_ORDERS(hobj)->update(hobj, b, (eint)(a->value * b->bar_step + b->bar_recoup)))
			e_signal_emit(adjust, SIG_ADJUST_UPDATE, (efloat)a->value);

		if (b->auto_hide && b->adj_span == 0)
			egui_hide_async(hobj, true);
		else {
			if (b->auto_hide && b->adj_span > 0)
				egui_show_async(hobj, true);
			egui_update_async(hobj);
		}
	}
	else if (b->auto_hide)
		egui_hide_async(hobj, true);
}

static void scrollbar_set(eHandle hobj)
{
	eHandle adjust = GUI_ADJUST_HOOK_DATA(hobj)->adjust;
	if (adjust) {
		GuiAdjust    *a = GUI_ADJUST_DATA(adjust);
		GuiScrollBar *b = GUI_SCROLLBAR_DATA(hobj);
		if (GUI_SCROLLBAR_ORDERS(hobj)->update(hobj, b, (eint)(a->value * b->bar_step + b->bar_recoup)))
			e_signal_emit(adjust, SIG_ADJUST_UPDATE, (efloat)a->value);
	}
}
