#include "gui.h"
#include "event.h"
#include "widget.h"
#include "bin.h"

static void bin_init_orders(eGeneType, ePointer);
static eint bin_init_data(eHandle, ePointer);
static eint bin_mousemove(eHandle, GalEventMouse *);
static eint bin_lbuttonup(eHandle, GalEventMouse *);
static eint bin_rbuttonup(eHandle, GalEventMouse *);
static eint bin_mbuttonup(eHandle, GalEventMouse *);
static eint bin_lbuttondown(eHandle, GalEventMouse *);
static eint bin_rbuttondown(eHandle, GalEventMouse *);
static eint bin_mbuttondown(eHandle, GalEventMouse *);
static eint bin_wheelforward(eHandle, GalEventMouse *);
static eint bin_wheelbackward(eHandle, GalEventMouse *);
static eint bin_keydown(eHandle, GalEventKey *);
static eint bin_keyup(eHandle, GalEventKey *);
static eint bin_imeinput(eHandle hobj, GalEventImeInput *);
static void bin_put(eHandle, eHandle);
static void bin_hide(eHandle);
static void bin_remove(eHandle, eHandle);
static eint bin_expose(eHandle, GuiWidget *, GalEventExpose *);
static void bin_unset_focus(eHandle);
static void bin_switch_focus(eHandle, eHandle);
static eint bin_configure(eHandle, GuiWidget *, GalEventConfigure *);
static void bin_move(eHandle hobj);
static void bin_show(eHandle hobj);
static eHandle bin_next_child(GuiBin *, eHandle, eint, eint);

static eint switch_next_focus(GuiBin *, eint);
static eint switch_prev_focus(GuiBin *, eint);

eGeneType egui_genetype_bin(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiBinOrders),
			bin_init_orders,
			sizeof(GuiBin),
			bin_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}


static eint bin_init_data(eHandle hobj, ePointer this)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiBin    *bin = (GuiBin *)this;

	bin->is_bin = true;
	egal_region_init(&bin->region_mask);

	widget_set_status(wid, GuiStatusVisible | GuiStatusMouse | GuiStatusActive);
	return 0;
}

static void bin_destroy(eHandle hobj)
{
	GuiWidget *cw = GUI_BIN_DATA(hobj)->head;
	while (cw) {
		GuiWidget *t = cw;
		cw = cw->next;
		egui_remove(OBJECT_OFFSET(t), true);
		e_signal_emit(OBJECT_OFFSET(t), SIG_DESTROY);
		e_object_unref(OBJECT_OFFSET(t));
	}
}

static void bin_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *o = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = this;

	w->put           = bin_put;
	w->hide          = bin_hide;
	w->show          = bin_show;
	w->move          = bin_move;
	w->remove        = bin_remove;
	w->expose        = bin_expose;
	w->destroy       = bin_destroy;
	w->configure     = bin_configure;
	w->unset_focus   = bin_unset_focus;

	o->keyup         = bin_keyup;
	o->keydown       = bin_keydown;
	o->mousemove     = bin_mousemove;
	o->lbuttonup     = bin_lbuttonup;
	o->lbuttondown   = bin_lbuttondown;
	o->rbuttonup     = bin_rbuttonup;
	o->rbuttondown   = bin_rbuttondown;
	o->mbuttonup     = bin_mbuttonup;
	o->mbuttondown   = bin_mbuttondown;
	o->wheelforward  = bin_wheelforward;
	o->wheelbackward = bin_wheelbackward;
	o->imeinput      = bin_imeinput;

	b->next_child    = bin_next_child;
	b->switch_focus  = bin_switch_focus;
}

static eint bin_expose(eHandle hobj, GuiWidget *pw, GalEventExpose *expose)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cw;

	GalRegion *region = egal_region_rect(&pw->rect);
	GalRegionSec *sec;

	egal_region_offset(region, -pw->rect.x, -pw->rect.y);

	if (REGION_NO_EMPTY(&bin->region_mask))
		egal_region_subtract(region, region, &bin->region_mask);

	egal_region_intersect_rect(region, &expose->rect);

	expose->pb = pw->pb;

	sec = region->head;
	while (sec) {
		GalEventExpose exp;
		exp.pb = expose->pb;
		exp.rect.x = sec->sec.x1;
		exp.rect.y = sec->sec.y1;
		exp.rect.w = sec->sec.x2 - exp.rect.x;
		exp.rect.h = sec->sec.y2 - exp.rect.y;
		e_signal_emit(hobj, SIG_EXPOSE_BG, &exp);
		sec = sec->next;
	}
	egal_region_destroy(region);

	for (cw = bin->head; cw; cw = cw->next) {
		if (WIDGET_STATUS_VISIBLE(cw)) {
			GalEventExpose exp;
			GalRect prc = {0, 0, pw->rect.w, pw->rect.h};

			if (!egal_rect_intersect(&exp.rect, &expose->rect, &cw->rect))
				continue;

			if (!egal_rect_intersect(&exp.rect, &exp.rect, &prc))
				continue;

			exp.rect.x -= cw->rect.x;
			exp.rect.y -= cw->rect.y;

			if (cw->pb)
				exp.pb = cw->pb;
			else
				exp.pb = expose->pb;

			if (WIDGET_STATUS_TRANSPARENT(cw) && cw->drawable == pw->drawable) {
				exp.rect.x += cw->offset_x;
				exp.rect.y += cw->offset_y;
			}

			if (!GUI_TYPE_BIN(OBJECT_OFFSET(cw)))
				e_signal_emit(OBJECT_OFFSET(cw), SIG_EXPOSE_BG, &exp);

			e_signal_emit(OBJECT_OFFSET(cw), SIG_EXPOSE, &exp);

			if (!cw->window && cw->drawable != pw->drawable) {
				if (WIDGET_STATUS_TRANSPARENT(cw))
					egal_composite(pw->drawable, pw->pb,
							cw->offset_x + exp.rect.x,
							cw->offset_y + exp.rect.y,
							cw->drawable, cw->pb,
							exp.rect.x, exp.rect.y,
							exp.rect.w, exp.rect.h);
				else
					egal_draw_drawable(pw->drawable, pw->pb,
							cw->offset_x + exp.rect.x,
							cw->offset_y + exp.rect.y,
							cw->drawable, cw->pb,
							exp.rect.x, exp.rect.y,
							exp.rect.w, exp.rect.h);
			}
		}
	}
	return 0;
}

static eint bin_configure(eHandle hobj, GuiWidget *wid, GalEventConfigure *ent)
{
	GuiBin   *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cw = bin->head;

	while (cw) {
		GalEventConfigure configure;
		configure.rect = cw->rect;
		e_signal_emit(OBJECT_OFFSET(cw), SIG_CONFIGURE, &configure);
		cw = cw->next;
	}
	return 0;
}

static void leave_signal_emit(eHandle hobj)
{
	e_signal_emit(hobj, SIG_LEAVE);

	if (GUI_TYPE_BIN(hobj)) {
		GuiBin *bin = GUI_BIN_DATA(hobj);
		if (bin->enter) {
			leave_signal_emit(bin->enter);
			bin->enter = 0;
		}
	}
}

static eint mouse_signal_emit(eHandle hobj, esig_t sig, GalEventMouse *ent)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (WIDGET_STATUS_MOUSE(wid)) {
		GalEventMouse mevent;
		mevent.point.x = ent->point.x - wid->rect.x;
		mevent.point.y = ent->point.y - wid->rect.y;
		mevent.state   = ent->state;
		mevent.time    = ent->time;
		return e_signal_emit(hobj, sig, &mevent);
	}
	return 0;
}

static GuiWidget *grab_in_point(GuiBin *bin, eint x, eint y)
{
	GuiWidget *cw;

	for (cw = bin->head; cw; cw = cw->next) {
		if (!WIDGET_STATUS_VISIBLE(cw))
			continue;

		if (egal_rect_point_in(&cw->rect, x, y)) {
			if (WIDGET_STATUS_MOUSE(cw))
				return cw;
			break;
		}
	}
	return 0;
}

static eint bin_buttondown(eHandle hobj, GalEventMouse *mevent, esig_t sig)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	eint x = mevent->point.x;
	eint y = mevent->point.y;
	GuiWidget *wid = grab_in_point(bin, x, y);
	if (wid) {
		eint ret;
		bool t = GUI_TYPE_BIN(OBJECT_OFFSET(wid));

		if (!t && WIDGET_STATUS_ACTIVE(wid))
			egui_set_focus(OBJECT_OFFSET(wid));

		ret = mouse_signal_emit(OBJECT_OFFSET(wid), sig, mevent);

		if ((t && ret) || (!t && WIDGET_STATUS_MOUSE(wid))) {
			bin->grab = OBJECT_OFFSET(wid);
			return 1;
		}
	}
	return 0;
}

static eint bin_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttondown(hobj, mevent, SIG_LBUTTONDOWN);
}

static eint bin_rbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttondown(hobj, mevent, SIG_RBUTTONDOWN);
}

static eint bin_mbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttondown(hobj, mevent, SIG_MBUTTONDOWN);
}

static eint bin_wheelforward(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttondown(hobj, mevent, SIG_WHEELFORWARD);
}

static eint bin_wheelbackward(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttondown(hobj, mevent, SIG_WHEELBACKWARD);
}

static eHandle point_in_active(GuiBin *bin, eint x, eint y)
{
	GuiWidget *cw = bin->head;

	while (cw) {
		if (WIDGET_STATUS_VISIBLE(cw)
				&& egal_rect_point_in(&cw->rect, x, y))
			return OBJECT_OFFSET(cw);
		cw = cw->next;
	}
	return 0;
}

static eint bin_buttonup(eHandle hobj, GalEventMouse *mevent, esig_t sig)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	eint x = mevent->point.x;
	eint y = mevent->point.y;
	eHandle grab = bin->grab;
	eHandle c;

	if (grab) {
		mouse_signal_emit(grab, sig, mevent);
		bin->grab = 0;
	}
	c = point_in_active(bin, x, y);
	if (c && c != grab)
		e_signal_emit(hobj, SIG_MOUSEMOVE, mevent);

	return 0;
}

static eint bin_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttonup(hobj, mevent, SIG_LBUTTONUP);
}

static eint bin_rbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttonup(hobj, mevent, SIG_RBUTTONUP);
}

static eint bin_mbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return bin_buttonup(hobj, mevent, SIG_MBUTTONUP);
}

static eint bin_mousemove(eHandle hobj, GalEventMouse *mevent)
{
	eint x = mevent->point.x;
	eint y = mevent->point.y;
	GuiBin    *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cw;

	if (bin->grab) {
		cw = GUI_WIDGET_DATA(bin->grab);
		if (egal_rect_point_in(&cw->rect, x, y)) {
			if (bin->enter == 0) {
				bin->enter = bin->grab;
				e_signal_emit(bin->grab, SIG_ENTER, x - cw->rect.x, y - cw->rect.y);
			}
		}
		else if (bin->enter) {
			bin->enter = 0;
			e_signal_emit(bin->grab, SIG_LEAVE);
		}
		mouse_signal_emit(bin->grab, SIG_MOUSEMOVE, mevent);
		return 0;
	}

	if (bin->enter) {
		cw = GUI_WIDGET_DATA(bin->enter);
		if (egal_rect_point_in(&cw->rect, x, y)) {
			mouse_signal_emit(bin->enter, SIG_MOUSEMOVE, mevent);
			return 0;
		}
		else {
			leave_signal_emit(bin->enter);
			bin->enter = 0;
		}
	}

	cw = bin->head;
	while (cw) {
		if (WIDGET_STATUS_VISIBLE(cw)) {
			if (egal_rect_point_in(&cw->rect, x, y)) {
				bin->enter = OBJECT_OFFSET(cw);
				if (WIDGET_STATUS_MOUSE(cw))
					e_signal_emit(bin->enter, SIG_ENTER, x - cw->rect.x, y - cw->rect.y);
				mouse_signal_emit(bin->enter, SIG_MOUSEMOVE, mevent);
			}
		}
		cw = cw->next;
	}
	return 0;
}

static eint bin_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	eint    val = 0;

	if (bin->focus &&
			((val = e_signal_emit(bin->focus, SIG_KEYDOWN, ent))) < 0)
		return -1;

	if (bin->focus && !GUI_TYPE_BIN(bin->focus)) {
		GalRect *rect = &GUI_WIDGET_DATA(bin->focus)->rect;
		if (ent->code == GAL_KC_Right || ent->code == GAL_KC_Left)
			val = (BIN_Y_AXIS | rect->y) + rect->h / 2;
		else if (ent->code == GAL_KC_Up || ent->code == GAL_KC_Down)
			val = (BIN_X_AXIS | rect->x) + rect->w / 2;
	}

	switch (ent->code) {
		case GAL_KC_Tab:
			if (ent->state & GAL_KS_SHIFT)
				return switch_prev_focus(bin, 0);
			return switch_next_focus(bin, 0);

		case GAL_KC_Down:
		case GAL_KC_Right:
			return switch_next_focus(bin, val);

		case GAL_KC_Up:
		case GAL_KC_Left:
			return switch_prev_focus(bin, val);

		default: break;
	}

	return -1;
}

static eint bin_keyup(eHandle hobj, GalEventKey *ent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->focus)
		e_signal_emit(bin->focus, SIG_KEYUP, ent);
	return 0;
}

static eint bin_imeinput(eHandle hobj, GalEventImeInput *ent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->focus)
		e_signal_emit(bin->focus, SIG_IME_INPUT, ent);
	return 0;
}

bool bin_can_focus(GuiWidget *wid)
{
	GuiBin *bin;

	if (!WIDGET_STATUS_VISIBLE(wid))
		return false;

	if (!WIDGET_STATUS_ACTIVE(wid))
		return false;

	if ((bin = GUI_BIN_DATA(OBJECT_OFFSET(wid))) && bin->is_bin) {
		GuiWidget *cw = bin->head;
		while (cw) {
			if (bin_can_focus(cw))
				return true;
			cw = cw->next;
		}
		return false;
	}

	return true;
}

static eHandle child_next(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->next;
	else
		cw = bin->head;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eHandle child_prev(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->prev;
	else
		cw = bin->tail;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eint get_oppose_oxis(eHandle child, eint axis)
{
	eint val = 0xffff & axis;
	if (axis & BIN_X_AXIS) {
		val -= GUI_WIDGET_DATA(child)->rect.x;
		if (val < 0)
			val = 0;
		val |= BIN_X_AXIS;
	}
	else if (axis & BIN_Y_AXIS) {
		val -= GUI_WIDGET_DATA(child)->rect.y;
		if (val < 0)
			val = 0;
		val |= BIN_Y_AXIS;
	}
	return val;
}

static eHandle bin_next_child(GuiBin *bin, eHandle child, eint dir, eint axis)
{
	GuiWidget *ow = NULL;
	GuiWidget *cw = NULL;
	eint       od = 0xffff;
	eHandle  Near = 0;
	eHandle (*cb)(GuiBin *, eHandle);

	eint diff = 0xffff;
	bool loop;

	if (dir == BIN_DIR_NEXT)
		cb = child_next;
	else
		cb = child_prev;

	do {
		child = cb(bin, child);
		if (!child)
			break;

		cw = GUI_WIDGET_DATA(child);
		loop = !bin_can_focus(cw);
		if (!loop && axis > 0) {
			eint d = 0;

			if (dir == BIN_DIR_NEXT) {
				if (axis & BIN_X_AXIS)
					d = (axis & 0xffff) - (cw->rect.x + cw->rect.w / 2);
				else
					d = (axis & 0xffff) - (cw->rect.y + cw->rect.h / 2);

				if (d < 0) {
					eint t;
					if (axis & BIN_X_AXIS)
						t = (axis & 0xffff) - cw->rect.x;
					else
						t = (axis & 0xffff) - cw->rect.y;
					d = -d;
					if (t < 0) t = -t;
					if (t < d) d =  t;
					if (ow) {
						eint e = 0xffff;
						if (axis & BIN_X_AXIS)
							e = (axis & 0xffff) - (ow->rect.x + ow->rect.w);
						else
							e = (axis & 0xffff) - (ow->rect.y + ow->rect.h);
						if (e  < 0) e = -e;
						if (od < e) e = od;
						if (e < d) {
							d = e;
							child = OBJECT_OFFSET(ow);
						}
					}
					loop = false;
				}
				else if (d == 0)
					loop = false;
				else
					loop = true;

				loop = cw->next ? loop : false;
			}
			else {
				if (axis & BIN_X_AXIS)
					d = cw->rect.x + cw->rect.w / 2 - (axis & 0xffff);
				else
					d = cw->rect.y + cw->rect.h / 2 - (axis & 0xffff);

				if (d < 0) {
					eint t;
					if (axis & BIN_X_AXIS)
						t = cw->rect.x + cw->rect.w - (axis & 0xffff);
					else
						t = cw->rect.y + cw->rect.h - (axis & 0xffff);
					d = -d;
					if (t < 0) t = -t;
					if (t < d) d =  t;
					if (ow) {
						eint e = 0xffff;
						if (axis & BIN_X_AXIS)
							e = ow->rect.x - (axis & 0xffff);
						else
							e = ow->rect.y - (axis & 0xffff);
						if (e  < 0) e = -e;
						if (od < e) e = od;
						if (e < d) {
							d = e;
							child = OBJECT_OFFSET(ow);
						}
					}
					loop = false;
				}
				else if (d == 0)
					loop = false;
				else
					loop = true;

				loop = cw->prev ? loop : false;
			}

			if (diff > d) {
				diff = d;
				Near = child;
			}
			ow = cw;
			od = d;
		}

	} while (loop);

	if (axis == 0)
		return Near ? Near : child;
	return Near;
}

#define BIN_RETURN_AXIS \
do { \
		if (axis & BIN_X_AXIS) { \
			return GUI_WIDGET_DATA(OBJECT_OFFSET(bin))->rect.x + axis; \
		} \
		else if (axis & BIN_Y_AXIS) { \
			return GUI_WIDGET_DATA(OBJECT_OFFSET(bin))->rect.y + axis; \
		} \
		return 0; \
} while (1)

static INLINE eHandle GET_NEXT_CHILD(GuiBin *bin, eHandle cobj, eint dir, eint axis)
{
	GuiBinOrders *bs = GUI_BIN_ORDERS(OBJECT_OFFSET(bin));
	return bs->next_child(bin, cobj, dir, axis);
}

static eint key_set_focus(GuiBin *bin, eint dir, eint axis)
{
	eHandle child = GET_NEXT_CHILD(bin, 0, dir, axis);

	if (!child)
		BIN_RETURN_AXIS;

	bin->focus = child;

	if (GUI_TYPE_BIN(child)) {
		eint val = get_oppose_oxis(child, axis);
		key_set_focus(GUI_BIN_DATA(child), dir, val);
	}
	else {
		GuiWidgetOrders *w = GUI_WIDGET_ORDERS(child);
		if (w && w->set_focus)
			w->set_focus(child);
	}
	egui_set_status(child, GuiStatusFocus);
	e_signal_emit(child, SIG_FOCUS_IN);

	return -1;
}

static eint key_switch_focus(GuiBin *bin, eint dir, eint axis)
{
	eHandle child;
	GuiWidgetOrders *ws;

	if (!bin->head)
		BIN_RETURN_AXIS;

	if (!bin->focus)
		return key_set_focus(bin, dir, axis);

	child = GET_NEXT_CHILD(bin, bin->focus, dir, axis);

	if (!child)
		BIN_RETURN_AXIS;

	if (bin->focus) {
		ws = GUI_WIDGET_ORDERS(bin->focus);
		if (ws->unset_focus)
			ws->unset_focus(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
	}

	bin->focus = child;

	ws = GUI_WIDGET_ORDERS(child);
	if (GUI_TYPE_BIN(child)) {
		eint val = get_oppose_oxis(child, axis);
		key_set_focus(GUI_BIN_DATA(child), dir, val);
	}
	else if (ws->set_focus)
		ws->set_focus(child);
	egui_set_status(child, GuiStatusFocus);
	e_signal_emit(child, SIG_FOCUS_IN);

	return -1;
}

static eint switch_next_focus(GuiBin *bin, eint val)
{
	return key_switch_focus(bin, BIN_DIR_NEXT, val);
}

static eint switch_prev_focus(GuiBin *bin, eint val)
{
	return key_switch_focus(bin, BIN_DIR_PREV, val);
}

static void bin_switch_focus(eHandle pobj, eHandle child)
{
	GuiBin *bin = GUI_BIN_DATA(pobj);

	if (bin->focus == child)
		return;

	if (bin->focus) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(bin->focus);
		if (ws->unset_focus)
			ws->unset_focus(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
	}

	if (GUI_STATUS_ACTIVE(pobj)) {
		eHandle pp = GUI_WIDGET_DATA(pobj)->parent;
		if (pp)
			GUI_BIN_ORDERS(pp)->switch_focus(pp, pobj);
	}

	if ((bin->focus = child)) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(child);
		if (ws->set_focus)
			ws->set_focus(child);
		egui_set_status(child, GuiStatusFocus);
		e_signal_emit(child, SIG_FOCUS_IN);
	}
}

static void bin_unset_focus(eHandle hobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->focus) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(bin->focus);
		if (ws->unset_focus)
			ws->unset_focus(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
		bin->focus = 0;
	}
}

static void bin_put(eHandle pobj, eHandle cobj)
{
	GuiWidget *pw = GUI_WIDGET_DATA(pobj);
	GuiWidget *cw = GUI_WIDGET_DATA(cobj);
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(cobj);

	if (pw->drawable == 0)
		return;

	if (pw->window) {
		cw->offset_x = cw->rect.x;
		cw->offset_y = cw->rect.y;
	}
	else {
		cw->offset_x = cw->rect.x + pw->offset_x;
		cw->offset_y = cw->rect.y + pw->offset_y;
		if (ws->set_offset)
			ws->set_offset(cobj);
	}

	if (cw->window)
		egal_window_put(pw->drawable, cw->window, cw->offset_x, cw->offset_y);
	else if (!cw->drawable) {
		cw->drawable = e_object_refer(pw->drawable);
		cw->pb       = e_object_refer(pw->pb);
	}

	if (GUI_TYPE_BIN(cobj)) {
		GuiBin    *cbin = GUI_BIN_DATA(cobj);
		GuiWidget *ccw  = cbin->head;

		while (ccw) {
			GUI_WIDGET_ORDERS(cobj)->put(cobj, OBJECT_OFFSET(ccw));
			ccw = ccw->next;
		}
	}

	if (WIDGET_STATUS_VISIBLE(cw))
		ws->show(cobj);
	else
		ws->hide(cobj);

	e_object_refer(pobj);
}

static void bin_show(eHandle hobj)
{
	GuiBin   *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cw = bin->head;

	while (cw) {
		if (WIDGET_STATUS_VISIBLE(cw))
			e_signal_emit(OBJECT_OFFSET(cw), SIG_SHOW);
		cw = cw->next;
	}
}

static void bin_hide(eHandle hobj)
{
	GuiBin   *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cw = bin->head;

	if (bin->focus == hobj)
		bin->focus = 0;

	while (cw) {
		if (WIDGET_STATUS_VISIBLE(cw))
			e_signal_emit(OBJECT_OFFSET(cw), SIG_HIDE);
		cw = cw->next;
	}
}

static void bin_move(eHandle hobj)
{
	GuiBin    *bin = GUI_BIN_DATA(hobj);
	GuiWidget *pw  = GUI_WIDGET_DATA(hobj);
	GuiWidget *cw  = bin->head;

	if (pw->window)
		egal_window_move(pw->window, pw->offset_x, pw->offset_y);

	while (cw) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(OBJECT_OFFSET(cw));
		if (pw->window) {
			cw->offset_x = cw->rect.x;
			cw->offset_y = cw->rect.y;
		}
		else {
			cw->offset_x = cw->rect.x + pw->offset_x;
			cw->offset_y = cw->rect.y + pw->offset_y;
		}
		if (ws->move)
			ws->move(OBJECT_OFFSET(cw));

		cw = cw->next;
	}
}

static void remove_sub_window(eHandle hobj, GuiWidget *wid, GuiBin *bin)
{
	GuiWidget *cw;

	if (!bin) return;

	cw = bin->head;

	while (cw) {
		GuiBin *cb;

		if (cw->window)
			egal_window_remove(cw->window);
		else if ((cb = GUI_BIN_DATA((OBJECT_OFFSET(cw)))))
			remove_sub_window(OBJECT_OFFSET(cw), cw, cb);

		cw = cw->next;
	}
}

static void bin_remove(eHandle pobj, eHandle cobj)
{
	GuiBin   *bin = GUI_BIN_DATA(pobj);
	GuiWidget *pw = GUI_WIDGET_DATA(pobj);
	GuiWidget *cw = GUI_WIDGET_DATA(cobj);

	if (bin->focus == cobj)
		bin->focus = 0;

	STRUCT_LIST_DELETE(bin->head, bin->tail, cw, prev, next);

	if (!pw->window && !cw->window)
		remove_sub_window(cobj, cw, GUI_BIN_DATA(cobj));

	e_object_unref(pobj);
	cw->parent = 0;
}

eint bin_prev_focus(eHandle hobj, eint val)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->focus) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(bin->focus);
		if (ws->unset_focus)
			ws->unset_focus(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
	}
	return key_set_focus(GUI_BIN_DATA(hobj), BIN_DIR_PREV, val);
}

eint bin_next_focus(eHandle hobj, eint val)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->focus) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(bin->focus);
		if (ws->unset_focus)
			ws->unset_focus(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
	}
	return key_set_focus(GUI_BIN_DATA(hobj), BIN_DIR_NEXT, val);
}

