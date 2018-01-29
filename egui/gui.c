#include <egal/egal.h>

#include "gui.h"
#include "res.h"
#include "widget.h"
#include "bin.h"
#include "box.h"
#include "fixed.h"
#include "event.h"
#include "label.h"
#include "adjust.h"
#include "window.h"

#define GUI_ASYNC_SIGNAL  			0
#define GUI_ASYNC_UPDATE  			1
#define GUI_ASYNC_RELAYOUT			2

static void __request_layout_event(eHandle, GalEvent *);

eGeneType egui_genetype(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {0, NULL, 0, NULL, NULL, NULL};
		gtype = e_register_genetype(&info, GTYPE_CELL, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_strings(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiStringsOrders),
			NULL, 0, NULL, NULL, NULL,
		};
		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_inout(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {0, NULL, 0, NULL, NULL, NULL};
		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

int egui_init(int argc, char *const args[])
{
	if (egal_init((eint)argc, args) < 0)
		return -1;

	if (egui_init_res() < 0)
		return -1;

	return 0;
}

static esig_t event_to_signal(GalEvent *event)
{
	esig_t signal;

	switch (event->type) {
		case GAL_ET_CONFIGURE:
			signal = SIG_CONFIGURE;
			break;
		case GAL_ET_RESIZE:
			signal = SIG_RESIZE;
			break;
		case GAL_ET_EXPOSE:
			signal = SIG_EXPOSE;
			break;
		case GAL_ET_KEYDOWN:
			signal = SIG_KEYDOWN;
			break;
		case GAL_ET_KEYUP:
			signal = SIG_KEYUP;
			break;
		case GAL_ET_MOUSEMOVE:
			signal = SIG_MOUSEMOVE;
			break;
		case GAL_ET_LBUTTONDOWN:
			signal = SIG_LBUTTONDOWN;
			break;
		case GAL_ET_LBUTTONUP:
			signal = SIG_LBUTTONUP;
			break;
		case GAL_ET_RBUTTONDOWN:
			signal = SIG_RBUTTONDOWN;
			break;
		case GAL_ET_RBUTTONUP:
			signal = SIG_RBUTTONUP;
			break;
		case GAL_ET_MBUTTONDOWN:
			signal = SIG_MBUTTONDOWN;
			break;
		case GAL_ET_MBUTTONUP:
			signal = SIG_MBUTTONUP;
			break;
		case GAL_ET_WHEELFORWARD:
			signal = SIG_WHEELFORWARD;
			break;
		case GAL_ET_WHEELBACKWARD:
			signal = SIG_WHEELBACKWARD;
			break;
		case GAL_ET_IME_INPUT:
			signal = SIG_IME_INPUT;
			break;
		case GAL_ET_ENTER:
			signal = SIG_ENTER;
			break;
		case GAL_ET_LEAVE:
			signal = SIG_LEAVE;
			break;
		case GAL_ET_FOCUS_IN:
			signal = SIG_FOCUS_IN;
			break;
		case GAL_ET_FOCUS_OUT:
			signal = SIG_FOCUS_OUT;
			break;
		case GAL_ET_DESTROY:
			signal = SIG_DESTROY;
			break;
		default:
			signal = 0;
	}
	return signal;
}

static ebool is_accel_key(eHandle hobj, GalEventKey *event)
{
	GuiWindow   *win = GUI_WINDOW_DATA(hobj);
	GuiAccelKey *hook;
	GalKeyCode  code = event->code;
	AccelKeyCB    cb = NULL;

	if (!win) return efalse;

	if (code >= 'A' && code <= 'Z')
		code -= 'A' - 'a';

	e_thread_mutex_lock(&win->lock);
	hook = win->head;
	while (hook) {
		if (!(hook->state ^ event->state) && hook->code == code) {
			cb = hook->cb;
			break;
		}
		hook = hook->next;
	}
	e_thread_mutex_unlock(&win->lock);

	if (cb && cb(hobj, hook->data) >= 0)
		return etrue;

	return efalse;
}

static void do_private_event(GalEvent *event)
{
	switch (event->private_type) {
		case GUI_ASYNC_SIGNAL:
		{
			esig_t sig  = (esig_t)event->e.private.args[0];
			elong nargs = (elong)event->e.private.args[1];
			if (nargs == 0)
				e_signal_emit(event->window, sig);
			if (nargs == 1)
				e_signal_emit(event->window, sig,
						event->e.private.args[2]);
			else if (nargs == 2)
				e_signal_emit(event->window, sig,
						event->e.private.args[2],
						event->e.private.args[3]);
			else if (nargs == 3)
				e_signal_emit(event->window, sig,
						event->e.private.args[2], 
						event->e.private.args[3], 
						event->e.private.args[4]);
			else if (nargs == 4)
				e_signal_emit(event->window, sig,
						event->e.private.args[2],
						event->e.private.args[3],
						event->e.private.args[4],
						event->e.private.args[5]);
			else if (nargs == 5)
				e_signal_emit(event->window, sig,
						event->e.private.args[2],
						event->e.private.args[3],
						event->e.private.args[4],
						event->e.private.args[5],
						event->e.private.args[6]);
			else if (nargs == 6)
				e_signal_emit(event->window, sig,
						event->e.private.args[2],
						event->e.private.args[3],
						event->e.private.args[4],
						event->e.private.args[5],
						event->e.private.args[6],
						event->e.private.args[7]);
			break;
		}
		case GUI_ASYNC_RELAYOUT:
			__request_layout_event(event->window, event);
			break;
		case GUI_ASYNC_UPDATE:
		{
			GalRect rc;
			rc.x = (eint)event->e.private.args[0];
			rc.y = (eint)event->e.private.args[1];
			rc.w = (eint)event->e.private.args[2];
			rc.h = (eint)event->e.private.args[3];
			egui_update_rect(event->window, &rc);
			break;
		}
		default:
			break;
	}
}

static void dispatch_event(GalEvent *event)
{
	esig_t signal;
	eHandle hobj;

	e_timer_loop();

	if (event->window == 0)
		return;

	if (event->type == GAL_ET_PRIVATE
			|| event->type == GAL_ET_PRIVATE_SAFE) {
		do_private_event(event);
		return;
	}

	hobj = egal_window_get_attachment(event->window);
	if (hobj == 0)
		return;

	signal = event_to_signal(event);
	if (signal == SIG_EXPOSE) {
		egui_update_rect(hobj, &event->e.expose.rect);
	}
	else if (signal != 0) {
		if (signal == SIG_KEYDOWN &&
				event->e.key.state &&
				is_accel_key(hobj, &event->e.key))
			return;
		e_signal_emit(hobj, signal, &event->e);
	}
}

int egui_main()
{
	Queue *queue;
	GalEvent e;
	int n, i;

	egal_event_init();

	queue = e_queue_new(sizeof(GalEvent) * 100);
	if (!queue)
		return -1;

	while ((n = egal_wait_event(queue)) > 0) {
		for (i = 0; i < n; i++) {
			e_queue_read(queue, &e, sizeof(GalEvent));
			if (e.type == GAL_ET_QUIT)
				exit(0);
			dispatch_event(&e);
		}
	}

	return 0;
}

void egui_quit(void)
{
	GalEvent event;

	event.type   = GAL_ET_QUIT;
	event.window = 0;

	egal_add_async_event(&event);
}

void egui_signal_emit(eHandle hobj, esig_t sig)
{
	GalEvent event;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_SIGNAL;
	event.e.private.args[0] = (eHandle)sig;
	event.e.private.args[1] = (eHandle)0;
	egal_add_async_event(&event);
}

void egui_signal_emit1(eHandle hobj, esig_t sig, ePointer args)
{
	GalEvent event;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_SIGNAL;
	event.e.private.args[0] = (eHandle)sig;
	event.e.private.args[1] = (eHandle)1;
	event.e.private.args[2] = (eHandle)args;
	egal_add_async_event(&event);
}

void egui_signal_emit2(eHandle hobj, esig_t sig, ePointer args1, ePointer args2)
{
	GalEvent event;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_SIGNAL;
	event.e.private.args[0] = (eHandle)sig;
	event.e.private.args[1] = (eHandle)2;
	event.e.private.args[2] = (eHandle)args1;
	event.e.private.args[3] = (eHandle)args2;
	egal_add_async_event(&event);
}

void egui_signal_emit3(eHandle hobj, esig_t sig, ePointer args1, ePointer args2, ePointer args3)
{
	GalEvent event;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_SIGNAL;
	event.e.private.args[0] = (eHandle)sig;
	event.e.private.args[1] = (eHandle)2;
	event.e.private.args[2] = (eHandle)args1;
	event.e.private.args[3] = (eHandle)args2;
	event.e.private.args[4] = (eHandle)args3;
	egal_add_async_event(&event);
}

void egui_signal_emit4(eHandle hobj, esig_t sig, ePointer args1, ePointer args2, ePointer args3, ePointer args4)
{
	GalEvent event;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_SIGNAL;
	event.e.private.args[0] = (eHandle)sig;
	event.e.private.args[1] = (eHandle)2;
	event.e.private.args[2] = (eHandle)args1;
	event.e.private.args[3] = (eHandle)args2;
	event.e.private.args[4] = (eHandle)args3;
	event.e.private.args[5] = (eHandle)args4;
	egal_add_async_event(&event);
}

static eint get_key_state(const echar *buf, eint len, GalKeyState *state)
{
	eint retval = 0;

	if (len < 4)
		return 0;

	switch (*buf) {
		case 'w':
		case 'W':
			if (len >= 4 && buf[3] == '>' &&
					!e_strncasecmp(buf, _("win"), 3)) {
				*state |= GAL_KS_WIN;
				retval = 4;
			}
			break;

		case 'a':
		case 'A':
			if (len >= 4 && buf[3] == '>' &&
					!e_strncasecmp(buf, _("alt"), 3)) {
				*state |= GAL_KS_ALT;
				retval = 4;
			}
			break;

		case 'c':
		case 'C':
			if (len >= 5 && buf[4] == '>' &&
					!e_strncasecmp(buf, _("ctrl"), 4)) {
				*state |= GAL_KS_CTRL;
				retval = 5;
			}
			break;

		case 's':
		case 'S':
			if (len >= 6 && buf[5] == '>' &&
					!e_strncasecmp(buf, _("shift"), 5)) {
				*state |= GAL_KS_SHIFT;
				retval = 6;
			}
			break;

		default:
			retval = 0;
	}

	return retval;
}

void egui_accelkey_connect(eHandle hobj, const echar *accelkey, AccelKeyCB cb, ePointer data)
{
	GuiWindow      *win = GUI_WINDOW_DATA(hobj);
	eint            len = e_strlen(accelkey);
	const echar      *p = accelkey;
	GalKeyState   state = 0;
	GalKeyCode    code  = 0;
	GuiAccelKey  *hook;

	while (*p) {
		eint n;

		if (*p == '<') {
			n = get_key_state(p + 1, len - 1, &state);
			if (n == 0) return;
		}
		else {
			code = *p;
			if (code >= 'A' && code <= 'z')
				code -= 'A' - 'a';
			n = 1;
		}

		p   += n;
		len -= n;
	}

	e_thread_mutex_lock(&win->lock);
	hook = win->head;
	while (hook) {
		if (!(hook->state ^ state) && hook->code == code)
			break;
		hook = hook->next;
	}

	if (!hook) {
		hook = e_calloc(sizeof(GuiAccelKey), 1);
		hook->code  = code;
		hook->state = state;
		hook->next  = win->head;
		win->head   = hook;
	}
	e_thread_mutex_unlock(&win->lock);

	hook->cb   = cb;
	hook->data = data;
}

static void union_region_mask(GuiWidget *cw)
{
	while (cw->parent
			&& cw->window
			&& WIDGET_STATUS_VISIBLE(cw)
			&& !WIDGET_STATUS_TRANSPARENT(cw)) {

		GuiBin *pbin = GUI_BIN_DATA(cw->parent);
		GuiBin *cbin = GUI_BIN_DATA(OBJECT_OFFSET(cw));
		if (cbin) {
			GalRegion *region = egal_region_copy1(&cbin->region_mask); 
			egal_region_offset(region, cw->rect.x, cw->rect.y);
			egal_region_union(&pbin->region_mask, &pbin->region_mask, region);
			egal_region_destroy(region);
		}
		else
			egal_region_union_rect(&pbin->region_mask, &cw->rect);

		cw = GUI_WIDGET_DATA(cw->parent);
	}
}

static void reunion_region_mask(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GalRegion region;

	egal_region_init(&region);
	while (wid->parent) {
		GuiBin  *pbin = GUI_BIN_DATA(wid->parent);
		GuiWidget *cw = pbin->head;
		egal_region_empty(&pbin->region_mask);

		while (cw) {
			if (!cw->window && WIDGET_STATUS_VISIBLE(cw)) {
				GuiBin *cbin = GUI_BIN_DATA(OBJECT_OFFSET(cw));
				if (cbin) {
					egal_region_copy(&region, &cbin->region_mask); 
					egal_region_offset(&region, cw->rect.x, cw->rect.y);
					egal_region_union(&pbin->region_mask, &pbin->region_mask, &region);
					egal_region_empty(&region);
				}
				else if (!WIDGET_STATUS_TRANSPARENT(cw))
					egal_region_union_rect(&pbin->region_mask, &cw->rect);
			}
			cw = cw->next;
		}
		wid = GUI_WIDGET_DATA(wid->parent);
	}
}

void egui_set_transparent(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (!WIDGET_STATUS_TRANSPARENT(wid)) {
		GuiWidgetOrders *wos = GUI_WIDGET_ORDERS(hobj);
		widget_set_status(wid, GuiStatusTransparent);
		egui_hide(hobj, etrue);
		if (wos->set_transparent)
			wos->set_transparent(hobj);
		egui_show(hobj, etrue);
	}
}

void egui_unset_transparent(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (WIDGET_STATUS_TRANSPARENT(wid)) {
		GuiWidgetOrders *wos = GUI_WIDGET_ORDERS(hobj);
		widget_unset_status(wid, GuiStatusTransparent);
		egui_hide(hobj, etrue);
		if (wos->unset_transparent)
			wos->unset_transparent(hobj);
		egui_show(hobj, etrue);
	}
}

static void __update_rect(GuiWidget *cw, GalRect *prc)
{
	GalEventExpose exp;

	if (!cw->drawable)
		return;

	exp.pb   =  0;
	exp.rect = *prc;
	while (cw->parent) {
		GuiWidget *pw = GUI_WIDGET_DATA(cw->parent);
		exp.rect.x += cw->rect.x;
		exp.rect.y += cw->rect.y;
		if (cw->window) {
			cw = pw;
			break;
		}
		cw = pw;
	}

	if (cw->drawable && GUI_TYPE_BIN(OBJECT_OFFSET(cw)))
		e_signal_emit(OBJECT_OFFSET(cw), SIG_EXPOSE, &exp);
}

void egui_update_rect(eHandle hobj, GalRect *prc)
{
	__update_rect(GUI_WIDGET_DATA(hobj), prc);
}

void egui_update(eHandle hobj)
{
	GuiWidget *w = GUI_WIDGET_DATA(hobj);
	GalRect   rc = {0, 0, w->rect.w, w->rect.h};
	__update_rect(w, &rc);
}

void egui_update_rect_async(eHandle hobj, GalRect *prc)
{
	GalEvent event;

	event.type   = GAL_ET_PRIVATE;
	event.window = hobj;
	event.private_type = GUI_ASYNC_UPDATE;
	event.e.private.args[0] = (eHandle)prc->x;
	event.e.private.args[1] = (eHandle)prc->y;
	event.e.private.args[2] = (eHandle)prc->w;
	event.e.private.args[3] = (eHandle)prc->h;

	egal_add_async_event(&event);
}

void egui_update_async(eHandle hobj)
{
	GuiWidget *w = GUI_WIDGET_DATA(hobj);
	GalRect   rc = {0, 0, w->rect.w, w->rect.h};
	egui_update_rect_async(hobj, &rc);
}

void egui_show(eHandle hobj, ebool fixed)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (!WIDGET_STATUS_VISIBLE(wid)) {
		widget_set_status(wid, GuiStatusVisible);

		if (wid->parent)
			egui_request_layout(wid->parent, 0, 0, 0, fixed, efalse);

		e_signal_emit(hobj, SIG_SHOW);
	}
}

void egui_hide(eHandle hobj, ebool fixed)
{
	if (GUI_STATUS_VISIBLE(hobj)) {
		GuiWidget *wid = GUI_WIDGET_DATA(hobj);

		egui_unset_status(hobj, GuiStatusVisible);

		if (GUI_STATUS_FOCUS(hobj))
			egui_unset_focus(hobj);

		reunion_region_mask(hobj);
		e_signal_emit(hobj, SIG_HIDE);

		if (wid->parent)
			egui_request_layout(wid->parent, 0, 0, 0, fixed, efalse);
	}
}

void egui_show_async(eHandle hobj, ebool fixed)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (!WIDGET_STATUS_VISIBLE(wid)) {
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);

		widget_set_status(wid, GuiStatusVisible);

		if (ws->show)
			ws->show(hobj);

		if (wid->parent)
			egui_request_layout_async(wid->parent, 0, 0, 0, fixed, efalse);
	}
}

void egui_hide_async(eHandle hobj, ebool fixed)
{
	if (GUI_STATUS_VISIBLE(hobj)) {
		GuiWidget      *wid = GUI_WIDGET_DATA(hobj);
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);

		egui_unset_status(hobj, GuiStatusVisible);

		if (ws->hide)
			ws->hide(hobj);

		if (GUI_STATUS_FOCUS(hobj))
			egui_unset_focus(hobj);

		if (wid->parent)
			egui_request_layout_async(wid->parent, 0, 0, 0, fixed, efalse);
	}
}

void egui_put(eHandle pobj, eHandle cobj, eint x, eint y)
{
	GuiWidget *wid = GUI_WIDGET_DATA(cobj);

	if (wid->rect.w < wid->min_w)
		wid->rect.w = wid->min_w;
	if (wid->rect.h < wid->min_h)
		wid->rect.h = wid->min_h;

	if (wid->parent == 0 && e_object_type_check(pobj, GTYPE_FIXED)) {
		GuiBin *bin = GUI_BIN_DATA(pobj);
		wid->parent = pobj;
		wid->rect.x = x;
		wid->rect.y = y;
		STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, wid, prev, next);

		GUI_WIDGET_ORDERS(pobj)->put(pobj, cobj);

		union_region_mask(wid);

		__update_rect(GUI_WIDGET_DATA(pobj), &wid->rect);
	}
}

void egui_add_spacing(eHandle hobj, eint size)
{
	GuiBinOrders *ors = GUI_BIN_ORDERS(hobj);
	if (ors->add_spacing)
		ors->add_spacing(hobj, size);
}

void egui_add(eHandle pobj, eHandle cobj)
{
	GuiWidget       *cw = GUI_WIDGET_DATA(cobj);
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(pobj);

	if (cw->rect.w < cw->min_w)
		cw->rect.w = cw->min_w;
	if (cw->rect.h < cw->min_h)
		cw->rect.h = cw->min_h;

	if (cw->parent == 0 && ws->add) {
		ws->add(pobj, cobj);
		union_region_mask(GUI_WIDGET_DATA(cobj));
	}
}

void egui_request_layout_async(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool up, ebool add)
{
	GalEvent event;

	event.window = hobj;
	event.type   = GAL_ET_PRIVATE_SAFE;
	event.private_type = GUI_ASYNC_RELAYOUT;
	event.e.private.args[0] = (eHandle)cobj;
	event.e.private.args[1] = (eHandle)req_w;
	event.e.private.args[2] = (eHandle)req_h;
	event.e.private.args[3] = (eHandle)up;
	event.e.private.args[4] = (eHandle)add;
	egal_add_async_event(&event);
}

static void __request_layout_event(eHandle hobj, GalEvent *ent)
{
	GuiBinOrders *bs = GUI_BIN_ORDERS(hobj);

	if (bs) {
		eHandle cobj = ent->e.private.args[0];
		eint w = (eint)ent->e.private.args[1];
		eint h = (eint)ent->e.private.args[2];
		ebool b = (ebool)ent->e.private.args[3];
		ebool a = (ebool)ent->e.private.args[4];
		if (bs->request_layout)
			bs->request_layout(hobj, cobj, w, h, b, a);
	}
}

void egui_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool up, ebool add)
{
	GuiBinOrders *bs = GUI_BIN_ORDERS(hobj);
	if (bs && bs->request_layout)
		bs->request_layout(hobj, cobj, req_w, req_h, up, add);
}

void egui_request_resize(eHandle hobj, eint w, eint h)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (w < wid->min_w)
		w = wid->min_w;
	if (h < wid->min_h)
		h = wid->min_h;

	if (wid->parent)
		egui_request_layout_async(wid->parent, hobj, w, h, efalse, efalse);
	else {
		wid->min_w = w;
		wid->min_h = h;
		GUI_WIDGET_ORDERS(hobj)->request_resize(hobj, w, h);
	}
}

void egui_move_resize(eHandle hobj, eint x, eint y, eint w, eint h)
{
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GalRect    nrc = {0, 0, w, h};
	GalRect    orc = {0, 0, wid->rect.w, wid->rect.h};
	eint  offset_x = 0;
	eint  offset_y = 0;

	if (w < wid->min_w || wid->max_w != 0)
		nrc.w = wid->min_w;
	if (h < wid->min_h || wid->max_h != 0)
		nrc.h = wid->min_h;

	wid->rect.x = x;
	wid->rect.y = y;
	wid->rect.w = nrc.w;
	wid->rect.h = nrc.h;

	if (wid->parent) {
		GuiWidget *pw = GUI_WIDGET_DATA(wid->parent);
		if (pw->window) {
			offset_x = x;
			offset_y = y;
		}
		else {
			offset_x = x + pw->offset_x;
			offset_y = y + pw->offset_y;
		}
	}
	orc.x = wid->offset_x - offset_x;
	orc.y = wid->offset_y - offset_y;

	wid->offset_x = offset_x;
	wid->offset_y = offset_y;
	if (ws->set_offset)
		ws->set_offset(hobj);

	reunion_region_mask(hobj);
	if (wid->window)
		egal_window_move_resize(wid->window, wid->offset_x, wid->offset_y, nrc.w, nrc.h);

	if (ws->move_resize)
		ws->move_resize(hobj, &orc, &nrc);
}

void egui_move(eHandle hobj, eint x, eint y)
{
	GuiWidget      *wid = GUI_WIDGET_DATA(hobj);
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);

	wid->rect.x = x;
	wid->rect.y = y;

	if (wid->parent) {
		GuiWidget  *p = GUI_WIDGET_DATA(wid->parent);
		if (p->window) {
			wid->offset_x = wid->rect.x;
			wid->offset_y = wid->rect.y;
		}
		else {
			wid->offset_x = wid->rect.x + p->offset_x;
			wid->offset_y = wid->rect.y + p->offset_y;
		}
	}
	else {
		wid->offset_x = x;
		wid->offset_y = y;
	}

	if (ws->move)
		ws->move(hobj);
}

void egui_remove(eHandle hobj, ebool relay)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->parent) {
		GuiWidgetOrders *o = GUI_WIDGET_ORDERS(wid->parent);

		if (o->remove)
			o->remove(wid->parent, hobj);

		if (wid->window)
			egal_window_remove(wid->window);
		else {
			e_object_unref(wid->drawable);
			wid->drawable = 0;
		}

		if (wid->pb) {
			e_object_unref(wid->pb);
			wid->pb = 0;
		}

		if (relay && WIDGET_STATUS_VISIBLE(wid))
			egui_request_layout_async(wid->parent, 0, 0, 0, efalse, etrue);
	}
}

void egui_raise(eHandle hobj)
{
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);
	if (ws->raise)
		ws->raise(hobj);
}

void egui_lower(eHandle hobj)
{
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);
	if (ws->lower)
		ws->lower(hobj);
}

void egui_set_focus(eHandle hobj)
{
	GuiWidget *w;

	if (!GUI_STATUS_ACTIVE(hobj))
		return;

	w = GUI_WIDGET_DATA(hobj);
	if (w->parent) {
		GUI_BIN_ORDERS(w->parent)->switch_focus(w->parent, hobj);
	}
}

void egui_unset_focus(eHandle hobj)
{
	GuiWidgetOrders *ws;
	eHandle p;

	if (!GUI_STATUS_ACTIVE(hobj))
		return;

	p = GUI_WIDGET_DATA(hobj)->parent;
	if (p) {
		GUI_BIN_DATA(p)->focus = 0;
	}
	egui_unset_status(hobj, GuiStatusFocus);

	ws = GUI_WIDGET_ORDERS(hobj);
	if (ws->unset_focus)
		ws->unset_focus(hobj);

	e_signal_emit(hobj, SIG_FOCUS_OUT);
}

void egui_set_font(eHandle hobj, GalFont font)
{
	GuiFontOrders *o = GUI_FONT_ORDERS(hobj);
	if (o && o->set_font)
		o->set_font(hobj, font);
}

void egui_set_fg_color(eHandle hobj, euint32 color)
{
	GuiWidgetOrders *w = GUI_WIDGET_ORDERS(hobj);
	if (w && w->set_fg_color)
		w->set_fg_color(hobj, color);
}

void egui_set_bg_color(eHandle hobj, euint32 color)
{
	GuiWidgetOrders *w = GUI_WIDGET_ORDERS(hobj);
	if (w && w->set_bg_color)
		w->set_bg_color(hobj, color);
}

void egui_set_max(eHandle hobj, eint w, eint h)
{
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);
	if (ws->set_max)
		ws->set_max(hobj, w, h);
}

void egui_set_min(eHandle hobj, eint w, eint h)
{
	GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(hobj);
	if (ws->set_min)
		ws->set_min(hobj, w, h);
}

void egui_insert_text(eHandle hobj, const echar *text, eint nchar)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->insert_text)
		o->insert_text(hobj, text, nchar);
}

void egui_set_strings(eHandle hobj, const echar *strings)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->set_text)
		o->set_text(hobj, strings, e_strlen(strings));
}

const echar *egui_get_strings(eHandle hobj)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->get_strings)
		return o->get_strings(hobj);
	return NULL;
}

eint egui_get_text(eHandle hobj, echar *buf, eint len)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->get_text)
		return o->get_text(hobj, buf, len);
	return 0;
}

void egui_set_max_text(eHandle hobj, eint size)
{
	GuiStringsOrders *o = GUI_STRINGS_ORDERS(hobj);
	if (o && o->set_max)
		o->set_max(hobj, size);
}

static eHandle public_layout = 0;
static void egui_public_layout_init(void)
{
	if (public_layout == 0)
		public_layout = e_object_new(GTYPE_LAYOUT);
}

void egui_draw_strings(GalDrawable drawable,
		GalPB pb, GalFont font, const echar *strings, GalRect *area, LayoutFlags flags)
{
	if (public_layout == 0)
		egui_public_layout_init();

	if (font)
		egui_set_font(public_layout, font);
	layout_set_offset(public_layout, area->x, area->y);
	layout_set_extent(public_layout, area->w, area->h);
	layout_set_flags(public_layout, flags);
	layout_set_strings(public_layout, strings);
	layout_draw(public_layout, drawable, pb, area);
}

void egui_strings_extent(GalFont font, const echar *strings, eint *w, eint *h)
{
	GuiLayout *layout = NULL;
	if (!layout) {
		egui_public_layout_init();
		layout = GUI_LAYOUT_DATA(public_layout);
	}

	if (font)
		egui_set_font(public_layout, font);

	layout_set_strings(public_layout, strings);
	layout_get_extents(layout, w, h);
}

void egui_hook_v(eHandle hobj, eHandle hook)
{
	GuiHookOrders *h = GUI_HOOK_ORDERS(hobj);
	if (h->hook_v)
		h->hook_v(hobj, hook);
}

void egui_hook_h(eHandle hobj, eHandle hook)
{
	GuiHookOrders *h = GUI_HOOK_ORDERS(hobj);
	if (h->hook_h)
		h->hook_h(hobj, hook);
}

void egui_set_expand(eHandle hobj, ebool expand)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (expand) {
		wid->max_w = 0;
		wid->max_h = 0;
	}
	else {
		wid->max_w = 1;
		wid->max_h = 1;
	}
}

void egui_set_expand_v(eHandle hobj, ebool expand)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (expand)
		wid->max_h = 0;
	else
		wid->max_h = 1;
}

void egui_set_expand_h(eHandle hobj, ebool expand)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (expand)
		wid->max_w = 0;
	else
		wid->max_w = 1;
}

void egui_draw_vbar(GalWindow window, GalPB pb,
		GalImage *head, GalImage *mid, GalImage *tail,
		int x, int y, int w, int h, int ox)
{
	euint oh;

	if (head) {
		egal_draw_image(window, pb, x, y, head, ox + x, 0, w - x, head->h);
		y += head->h;
		h -= head->h;
	}

	if (tail)
		h -= tail->h;

	while (h > 0) {
		if (h < mid->h)
			oh = h;
		else
			oh = mid->h;
		egal_draw_image(window, pb, x, y, mid, ox + x, 0, w - x, oh);
		y += oh;
		h -= oh;
	}

	if (tail)
		egal_draw_image(window, pb, x, y, tail, ox + x, 0, w - x, tail->h);
}

void egui_draw_hbar(GalWindow window, GalPB pb,
		GalImage *head, GalImage *mid, GalImage *tail,
		int x, int y, int w, int h, int oy)
{
	euint ow;

	if (head) {
		egal_draw_image(window, pb, x, y, head, 0, oy + y, head->w, h -  y);
		x += head->w;
		w -= head->w;
	}

	if (tail)
		w -= tail->w;

	while (w > 0) {
		if (w < mid->w)
			ow = w;
		else
			ow = mid->w;
		egal_draw_image(window, pb, x, y, mid, 0, oy + y, ow, h - y);
		x += ow;
		w -= ow;
	}

	if (tail)
		egal_draw_image(window, pb, x, y, tail, 0, oy + y, tail->w, h - y);
}

void egui_make_GL(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->window)
		egal_window_make_GL(wid->window);
}

