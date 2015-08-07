#include "gui.h"
#include "widget.h"
#include "bin.h"
#include "box.h"
#include "window.h"
#include "event.h"

static eint dialog_init(eHandle, eValist);
static void dialog_init_orders(eGeneType, ePointer);

static eint window_init(eHandle, eValist);
static void window_init_orders(eGeneType, ePointer);

static void window_add(eHandle, eHandle);
static void window_request_layout(eHandle, eHandle, eint, eint, bool, bool);
static void window_move(eHandle hobj);
static eint window_expose_bg(eHandle, GuiWidget *, GalEventExpose *);
static void window_lower(eHandle);
static void window_raise(eHandle);
static eint window_resize(eHandle, GuiWidget *, GalEventResize *);
static eint window_configure(eHandle, GuiWidget *, GalEventConfigure *);
static void window_request_resize(eHandle, eint, eint);
static eint window_keydown(eHandle, GalEventKey *);

eGeneType egui_genetype_window(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			window_init_orders,
			sizeof(GuiWindow),
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BOX, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_dialog(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			dialog_init_orders,
			0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WINDOW, NULL);
	}
	return gtype;
}

static void leave_signal_emit(eHandle hobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin->enter) {
		e_signal_emit(bin->enter, SIG_LEAVE);
		if (GUI_TYPE_BIN(bin->enter))
			leave_signal_emit(bin->enter);
		bin->enter = 0;
	}
}

static eint window_leave(eHandle hobj)
{
	e_signal_emit_connect(hobj, SIG_LEAVE);
	leave_signal_emit(hobj);
	return 0;
}

static eint window_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiBin     *bin = GUI_BIN_DATA(hobj);
	GuiWidget *cwid = bin->head;
	eint val = 0;

	if (!cwid) return -1;

	val = e_signal_emit(OBJECT_OFFSET(cwid), SIG_KEYDOWN, ent);
	if (val < 0)
		return -1;

	if (!GUI_TYPE_BIN(OBJECT_OFFSET(cwid)))
		return 0;

	switch (ent->code) {
		case GAL_KC_Tab:
			if (ent->state & GAL_KS_SHIFT)
				return bin_prev_focus(OBJECT_OFFSET(cwid), 0);
			return bin_next_focus(OBJECT_OFFSET(cwid), 0);

		case GAL_KC_Down:
		case GAL_KC_Right:
			return bin_next_focus(OBJECT_OFFSET(cwid), val);

		case GAL_KC_Up:
		case GAL_KC_Left:
			return bin_prev_focus(OBJECT_OFFSET(cwid), val);
		default:
			break;
	}

	return 0;
}

static eint window_focus_out(eHandle hobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin && bin->focus) {
		window_focus_out(bin->focus);
		egui_unset_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_OUT);
	}
	return 0;
}

static eint window_focus_in(eHandle hobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	if (bin && bin->focus) {
		window_focus_in(bin->focus);
		egui_set_status(bin->focus, GuiStatusFocus);
		e_signal_emit(bin->focus, SIG_FOCUS_IN);
	}
	return 0;
}

static void (*bin_hide)(eHandle);
static void (*bin_show)(eHandle);
static void window_hide(eHandle hobj)
{
	egal_window_hide(GUI_WIDGET_DATA(hobj)->window);
	bin_hide(hobj);
}

static void window_show(eHandle hobj)
{
	egal_window_show(GUI_WIDGET_DATA(hobj)->window);
	bin_show(hobj);
}

static void window_destroy(eHandle hobj)
{
	GuiWidget *cw = GUI_BIN_DATA(hobj)->head;
	while (cw) {
		GuiWidget *t = cw;
		cw = cw->next;
		egui_remove(OBJECT_OFFSET(t), true);
		e_signal_emit(OBJECT_OFFSET(t), SIG_DESTROY);
		e_object_unref(OBJECT_OFFSET(t));
	}
	e_object_unref(hobj);
}

static void window_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	b->request_layout = window_request_layout;

	e->leave          = window_leave;
	e->keydown        = window_keydown;
	e->focus_in       = window_focus_in;
	e->focus_out      = window_focus_out;

	bin_show          = w->show;
	bin_hide          = w->hide;
	w->add            = window_add;
	w->hide           = window_hide;
	w->show           = window_show;
	w->move           = window_move;
	w->raise          = window_raise;
	w->lower          = window_lower;
	w->resize         = window_resize;
	w->destroy        = window_destroy;
	w->configure      = window_configure;
	w->expose_bg      = window_expose_bg;
	w->request_resize = window_request_resize;

	c->init           = window_init;
}

static eint __window_init(eHandle hobj, GalWindowType type, GuiWindowType win_type)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GalWindowAttr attr = {0};

	e_thread_mutex_init(&GUI_WINDOW_DATA(hobj)->lock, NULL);

	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->rect.w = 1;
	wid->rect.h = 1;
	wid->bg_color = 0x3c3b37;

	attr.type    = type;
	attr.width   = 1;
	attr.height  = 1;
	attr.wclass  = GAL_INPUT_OUTPUT;
	attr.input_event  = true;
	attr.output_event = true;
	if (win_type == GUI_WINDOW_TOPLEVEL) {
		widget_set_status(wid, GuiStatusVisible);
		attr.visible = true;
	}
	else {
		widget_unset_status(wid, GuiStatusVisible);
		attr.visible = false;
	}

	wid->window   = egal_window_new(&attr);
	wid->drawable = wid->window;
#ifdef _GAL_SUPPORT_CAIRO
	{
		GalPBAttr attr;
		attr.func = GalPBcopy;
		attr.use_cairo = true;
		wid->pb = egal_pb_new(wid->window, &attr);
	}
#else
	wid->pb = egal_pb_new(wid->window, NULL);
#endif
	egal_window_set_attachment(wid->window, hobj);
	return 0;
}

static eint window_init(eHandle hobj, eValist va)
{
	GuiWindow *win = GUI_WINDOW_DATA(hobj);
	win->type = e_va_arg(va, GuiWindowType);
	return __window_init(hobj, GalWindowTop, win->type);
}

static void window_move(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	egal_window_move(wid->window, wid->rect.x, wid->rect.y);
}

static eint window_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_set_foreground(exp->pb, wid->bg_color);
	egal_fill_rect(wid->window, exp->pb, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
	return 0;
}

static void window_request_resize(eHandle hobj, eint w, eint h)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiBox    *box = GUI_BOX_DATA(hobj);

	if (w < wid->min_w)
		w = wid->min_w;
	if (h < wid->min_h)
		h = wid->min_h;

	wid->rect.w = w;
	wid->rect.h = h;
	egal_window_resize(wid->window, w, h);

	if (box->head) {
		eHandle cobj = box->head->obj.hobj;
		GUI_WIDGET_ORDERS(cobj)->request_resize(cobj,
				w - box->border_width * 2,
				h - box->border_width * 2);
	}
}

static eint window_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiBox *box = GUI_BOX_DATA(hobj);

#ifdef WIN32
	bool update = false;
	if (wid->rect.w != resize->w || wid->rect.h != resize->h)
		update = true;
#endif

	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	if (box->head) {
		eHandle cobj = box->head->obj.hobj;
		GUI_WIDGET_ORDERS(cobj)->request_resize(cobj,
				resize->w - box->border_width * 2,
				resize->h - box->border_width * 2);
	}

#ifdef WIN32
	if (update)
#endif
		egui_update(hobj);

	return 0;
}

static eint window_configure(eHandle hobj, GuiWidget *wid, GalEventConfigure *ent)
{
	GalEventResize resize = {wid->rect.w, wid->rect.h};
	window_resize(hobj, wid, &resize);
	return 0;
}

static void window_add(eHandle hobj, eHandle cobj)
{
	GuiBox *box = GUI_BOX_DATA(hobj);

	if (!box->head) {
		GuiWidget *pw  = GUI_WIDGET_DATA(hobj);
		GuiWidget *cw  = GUI_WIDGET_DATA(cobj);
		GuiBin    *bin = GUI_BIN_DATA(hobj);
		BoxPack   *new_pack;

		GalGeometry geom;
		eint req_w = cw->rect.w + box->border_width * 2;
		eint req_h = cw->rect.h + box->border_width * 2;

		new_pack = e_malloc(sizeof(BoxPack));
		new_pack->obj.hobj = cobj;
		new_pack->next = NULL;
		box->child_num = 1;
		box->head = new_pack;

		pw->min_w = cw->min_w + box->border_width * 2;
		pw->min_h = cw->min_h + box->border_width * 2;
		geom.min_width  = pw->min_w;
		geom.min_height = pw->min_h;
		egal_window_set_geometry_hints(pw->window, &geom, GAL_HINT_MIN_SIZE);

		if (req_w > pw->rect.w)
			pw->rect.w = req_w;
		if (req_h > pw->rect.h)
			pw->rect.h = req_h;

		cw->parent = hobj;
		cw->rect.x = box->border_width;
		cw->rect.y = box->border_width;
		STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, cw, prev, next);

		GUI_WIDGET_ORDERS(hobj)->put(hobj, cobj);

		egal_window_resize(pw->window, pw->rect.w, pw->rect.h);
		bin->focus = cobj;
	}
}

static void window_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool fixed, bool add)
{
	GuiBox    *box = GUI_BOX_DATA(hobj);
	GuiWidget *pw  = GUI_WIDGET_DATA(hobj);
	GuiWidget *cw;
	GalGeometry geom;

	if (!box->head) return;

	cw  = GUI_WIDGET_DATA(box->head->obj.hobj);

	pw->min_w = cw->min_w + box->border_width * 2;
	pw->min_h = cw->min_h + box->border_width * 2;
	geom.min_width  = pw->min_w;
	geom.min_height = pw->min_h;
	egal_window_set_geometry_hints(pw->window, &geom, GAL_HINT_MIN_SIZE);

	if (add || fixed) {
		if (pw->rect.w < pw->min_w)
			pw->rect.w = pw->min_w;
		if (pw->rect.h < pw->min_h)
			pw->rect.h = pw->min_h;
	}
	else {
		req_w += box->border_width * 2;
		req_h += box->border_width * 2;
		if (req_w < pw->min_w)
			pw->rect.w = pw->min_w;
		else
			pw->rect.w = req_w;

		if (req_h < pw->min_h)
			pw->rect.h = pw->min_h;
		else
			pw->rect.h = req_h;
	}

	egal_window_resize(pw->window, pw->rect.w, pw->rect.h);

	if (box->head) {
		eHandle cobj = box->head->obj.hobj;
		GUI_WIDGET_ORDERS(cobj)->request_resize(cobj,
				pw->rect.w - box->border_width * 2,
				pw->rect.h - box->border_width * 2);
	}
}

static void window_raise(eHandle hobj)
{
	egal_window_raise(GUI_WIDGET_DATA(hobj)->window);
}

static void window_lower(eHandle hobj)
{
	egal_window_lower(GUI_WIDGET_DATA(hobj)->window);
}

eHandle egui_window_new(GuiWindowType type)
{
	return e_object_new(GTYPE_WINDOW, type);
}

static void dialog_init_orders(eGeneType new, ePointer this)
{
	eCellOrders *c = e_genetype_orders(new, GTYPE_CELL);
	c->init = dialog_init;
}

static eint dialog_init(eHandle hobj, eValist va)
{
	GUI_WINDOW_DATA(hobj)->type = GUI_WINDOW_POPUP;
	return __window_init(hobj, GalWindowDialog, GUI_WINDOW_POPUP);
}

eHandle egui_dialog_new(void)
{
	return e_object_new(GTYPE_DIALOG);
}

eint egui_window_set_name(eHandle hobj, const echar *name)
{
	return egal_window_set_name(GUI_WIDGET_DATA(hobj)->window, name);
}
