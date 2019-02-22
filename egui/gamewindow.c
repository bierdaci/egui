#include "gui.h"
#include "widget.h"
#include "event.h"
#include "gamewindow.h"

static eint window_init(eHandle, eValist);
static void window_init_orders(eGeneType, ePointer);

static eint window_expose_bg(eHandle, GuiWidget *, GalEventExpose *);
static eint window_resize(eHandle, GuiWidget *, GalEventResize *);
static eint window_configure(eHandle, GuiWidget *, GalEventConfigure *);
static eint window_keydown(eHandle, GalEventKey *);

eGeneType egui_genetype_game_window(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			window_init_orders,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

static eint window_leave(eHandle hobj)
{
	return e_signal_emit_connect(hobj, SIG_LEAVE);
}

static eint window_enter(eHandle hobj, eint x, eint y)
{
	return e_signal_emit_connect(hobj, SIG_ENTER, x, y);
}

static eint window_keydown(eHandle hobj, GalEventKey *ent)
{
	return e_signal_emit_connect(hobj, SIG_KEYDOWN, ent);
}

static eint window_keyup(eHandle hobj, GalEventKey *ent)
{
	return e_signal_emit_connect(hobj, SIG_KEYUP, ent);
}

static eint window_focus_out(eHandle hobj)
{
	return e_signal_emit_connect(hobj, SIG_FOCUS_OUT);
}

static eint window_focus_in(eHandle hobj)
{
	return e_signal_emit_connect(hobj, SIG_FOCUS_IN);
}

static eint window_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_LBUTTONDOWN, mevent);
}

static eint window_rbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_RBUTTONDOWN, mevent);
}

static eint window_mbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_MBUTTONDOWN, mevent);
}

static eint window_wheelforward(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_WHEELFORWARD, mevent);
}

static eint window_wheelbackward(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_WHEELBACKWARD, mevent);
}

static eint window_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_LBUTTONUP, mevent);
}

static eint window_rbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_RBUTTONUP, mevent);
}

static eint window_mbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_MBUTTONUP, mevent);
}

static eint window_mousemove(eHandle hobj, GalEventMouse *mevent)
{
	return e_signal_emit_connect(hobj, SIG_MOUSEMOVE, mevent);
}

static void window_destroy(eHandle hobj)
{
	e_object_unref(hobj);
}

static eint window_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	return 0;
}

static eint window_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	return e_signal_emit_connect(hobj, SIG_EXPOSE, exp);
}

static eint window_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	return e_signal_emit_connect(hobj, SIG_REALIZE, resize);
}

static eint window_configure(eHandle hobj, GuiWidget *wid, GalEventConfigure *ent)
{
	GalEventResize resize = {wid->rect.w, wid->rect.h};
	return window_resize(hobj, wid, &resize);
}

static void window_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	e->enter          = window_enter;
	e->leave          = window_leave;
	e->keyup          = window_keyup;
	e->keydown        = window_keydown;
	e->focus_in       = window_focus_in;
	e->focus_out      = window_focus_out;
	e->mousemove      = window_mousemove;
	e->lbuttonup      = window_lbuttonup;
	e->lbuttondown    = window_lbuttondown;
	e->rbuttonup      = window_rbuttonup;
	e->rbuttondown    = window_rbuttondown;
	e->mbuttonup      = window_mbuttonup;
	e->mbuttondown    = window_mbuttondown;
	e->wheelforward   = window_wheelforward;
	e->wheelbackward  = window_wheelbackward;

	w->resize         = window_resize;
	w->destroy        = window_destroy;
	w->configure      = window_configure;
	w->expose_bg      = window_expose_bg;
	w->expose         = window_expose;

	c->init           = window_init;
}

static eint window_init(eHandle hobj, eValist va)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	GalVisualInfo info;
	GalWindowAttr attr = {0};

	egal_get_visual_info(0, &info);

	wid->min_w  = info.w;
	wid->min_h  = info.h;
	wid->rect.w = info.w;
	wid->rect.h = info.h;
	wid->bg_color = 0;

	attr.type    = GalWindowFull;
	attr.width   = info.w;
	attr.height  = info.h;
	attr.wclass  = GAL_INPUT_OUTPUT;
	attr.wa_mask = GAL_WA_NOREDIR;
	attr.input_event  = etrue;
	attr.output_event = etrue;
	attr.visible = etrue;

	wid->window   = egal_window_new(&attr);
	wid->drawable = wid->window;
#ifdef _GAL_SUPPORT_CAIRO
	{
		GalPBAttr attr;
		attr.func = GalPBcopy;
		attr.use_cairo = etrue;
		wid->pb = egal_pb_new(wid->window, &attr);
	}
#else
	wid->pb = egal_pb_new(wid->window, NULL);
#endif
	egal_window_set_attachment(wid->window, hobj);
	return 0;
}

eHandle egui_game_window_new(void)
{
	return e_object_new(GTYPE_GAME_WINDOW);
}
