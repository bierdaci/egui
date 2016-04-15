#include "gui.h"
#include "event.h"
#include "widget.h"
#include "button.h"
#include "res.h"

static eint button_init(eHandle, eValist);
static eint button_init_data(eHandle, ePointer);
static void button_init_orders(eGeneType, ePointer);
static eint button_expose(eHandle, GuiWidget *, GalEventExpose *);
static eint button_lbuttondown(eHandle, GalEventMouse *);
static eint button_lbuttonup(eHandle, GalEventMouse *);
static eint button_enter(eHandle, eint, eint);
static eint button_leave(eHandle);
static eint button_keydown(eHandle, GalEventKey *);
static eint button_keyup(eHandle, GalEventKey *);
static void label_button_init_orders(eGeneType, ePointer);

eGeneType gtype_label_button;

static GalImage *img_enter = NULL;
static GalImage *img_leave = NULL;
static GalImage *img_down  = NULL;

static GalImage *radio_img_ye = NULL;
static GalImage *radio_img_ne = NULL;

static GalImage *check_img_y = NULL;
static GalImage *check_img_n = NULL;

eGeneType egui_genetype_button(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			button_init_orders,
			sizeof(GuiButton),
			button_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_label_button(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GuiResItem *res = egui_res_find(GUI_RES_HANDLE, _("button"));
		eGeneInfo info = {
			0,
			label_button_init_orders,
			0,
			NULL, NULL, NULL,
		};

		img_enter    = egui_res_find_item(res,  _("enter"));
		img_leave    = egui_res_find_item(res,  _("leave"));
		img_down     = egui_res_find_item(res,  _("down"));

		gtype = e_register_genetype(&info, GTYPE_BUTTON, NULL);
	}
	return gtype;
}

static void button_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	w->expose         = button_expose;
	e->leave          = button_leave;
	e->enter          = button_enter;
	e->keyup          = button_keyup;
	e->keydown        = button_keydown;
	e->lbuttonup      = button_lbuttonup;
	e->lbuttondown    = button_lbuttondown;
	e->focus_in       = (ePointer)egui_update;
	e->focus_out      = (ePointer)egui_update;

	c->init           = button_init;
}

static eint button_init_data(eHandle hobj, ePointer this)
{
	egui_set_status(hobj, GuiStatusVisible | GuiStatusActive | GuiStatusMouse);
	return 0;
}

static eint button_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiButton *bn = GUI_BUTTON_DATA(hobj);
	if (!bn->key_down && 
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		bn->key_down = true;
		egui_update(hobj);
	}
	return 0;
}

static eint button_keyup(eHandle hobj, GalEventKey *ent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);

	if (button->key_down &&
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		button->key_down = false;
		egui_update(hobj);
		e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
	}
	return 0;
}

static eint button_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);

	GalRect *erc = &ent->rect;

	if (button->key_down) {
		egal_set_foreground(wid->pb, 0x303030);
	}
	else if (button->down) {
		if (button->enter)
			egal_set_foreground(wid->pb, 0x303030);
		else
			egal_set_foreground(wid->pb, 0x0000ff);
	}
	else if (GUI_STATUS_FOCUS(hobj)) {
		if (button->enter)
			egal_set_foreground(wid->pb, 0x3030ff);
		else
			egal_set_foreground(wid->pb, 0x0000ff);
	}
	else {
		if (button->enter)
			egal_set_foreground(wid->pb, 0x00ffb0);
		else
			egal_set_foreground(wid->pb, 0x00e0b0);
	}

	egal_fill_rect(wid->drawable, wid->pb, erc->x, erc->y, erc->w, erc->h);

	return 0;
}

static eint button_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
#ifdef WIN32
	eHandle top = egui_get_top(hobj);
	egal_grab_pointer(GUI_WIDGET_DATA(top)->window, true, 0);
#endif
	GUI_BUTTON_DATA(hobj)->down = true;
	egui_update(hobj);
	return 0;
}

static eint button_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);
#ifdef WIN32
	eHandle top = egui_get_top(hobj);
	egal_ungrab_pointer(GUI_WIDGET_DATA(top)->window);
#endif
	if (button->down) {
		button->down = false;
		if (button->enter)
			e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
		egui_update(hobj);
	}

	return 0;
}

static eint button_enter(eHandle hobj, eint x, eint y)
{
	GUI_BUTTON_DATA(hobj)->enter = true;
	egui_update(hobj);
	return 0;
}

static eint button_leave(eHandle hobj)
{
	GUI_BUTTON_DATA(hobj)->enter = false;
	egui_update(hobj);
	return 0;
}

static eint button_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	eint w = e_va_arg(vp, eint);
	eint h = e_va_arg(vp, eint);

	wid->rect.w = w;
	wid->rect.h = h;
	wid->min_w  = w;
	wid->min_h  = h;
	wid->max_w  = 1;
	wid->max_h  = 1;

	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_button_new(int w, int h)
{
	return e_object_new(GTYPE_BUTTON, w, h);
}

static eint label_button_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	GUI_BUTTON_DATA(hobj)->label = e_va_arg(vp, const echar *);

	wid->min_w  = img_enter->w;
	wid->min_h  = img_enter->h;
	wid->max_w  = 1;
	wid->max_h  = 1;
	wid->rect.w = img_enter->w;
	wid->rect.h = img_enter->h;
	wid->fg_color = 0;

	wid->drawable = egal_drawable_new(wid->min_w, wid->min_h, false);
	wid->pb       = egal_pb_new(wid->drawable, NULL);

	return 0;
}

static eint label_button_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiButton *btn = GUI_BUTTON_DATA(hobj);
	GalImage  *img;
	GalRect rc = {0, 0, wid->rect.w, wid->rect.h};

	if (btn->key_down || btn->down) {
		img = img_down;
		rc.x += 1;
		rc.y += 1;
	}
	else if (btn->enter)
		img = img_enter;
	else
		img = img_leave;

	egal_draw_image(wid->drawable, wid->pb, 0, 0, img, 0, 0, img->w, img->h);

	if (GUI_STATUS_FOCUS(hobj)) {
		egal_set_foreground(wid->pb, 0xff00ff);
		egal_draw_rect(wid->drawable, wid->pb, 2, 2, img->w - 4, img->h - 4);
	}

	egal_set_foreground(wid->pb, wid->fg_color);
	egui_draw_strings(wid->drawable, wid->pb, 0, btn->label, &rc,  LF_HCenter | LF_VCenter);
	return 0;
}

static void button_move_resize(eHandle hobj, GalRect *orc, GalRect *nrc)
{
	if (egal_rect_is_intersect(orc, nrc)) {
		egal_rect_get_bound(orc, orc, nrc);
		egui_update_rect_async(hobj, orc);
	}
	else {
		egui_update_rect_async(hobj, orc);
		egui_update_rect_async(hobj, nrc);
	}
}

static void label_button_init_orders(eGeneType new, ePointer this)
{
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	c->init        = label_button_init;
	w->expose      = label_button_expose;
	w->move_resize = button_move_resize;
}

eHandle egui_label_button_new(const echar *label)
{
	return e_object_new(GTYPE_LABEL_BUTTON, label);
}

static eint radio_button_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiButton *bn  = GUI_BUTTON_DATA(hobj);
	eint w, h;
	eHandle group_bn;

	bn->label = e_va_arg(vp, const echar *);
	group_bn  = e_va_arg(vp, eHandle);

	if (group_bn == 0) {
		bn->p.group = e_malloc(sizeof(RadioGroup));
		bn->p.group->bn = hobj;
	}
	else {
		bn->p.group = GUI_BUTTON_DATA(group_bn)->p.group;
	}

	egui_strings_extent(0, bn->label, &w, &h);
	wid->min_w  = radio_img_ne->w + w + 7;
	wid->min_h  = radio_img_ne->h > h ? radio_img_ne->h : h + 2;
	wid->max_w  = 1;
	wid->max_h  = 1;
	wid->rect.w = wid->min_w;
	wid->rect.h = wid->min_h;
	wid->fg_color = 0xffffff;

	widget_set_status(wid, GuiStatusTransparent);

	return 0;
}

static eint radio_button_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiButton *btn = GUI_BUTTON_DATA(hobj);
	GalRect    rc  = {wid->offset_x + radio_img_ye->w + 5, wid->offset_y, wid->min_w, wid->min_h};
	GalImage  *img;

	if (btn->enter) {
		egal_set_foreground(wid->pb, 0xc7baa2);
		egal_fill_rect(wid->drawable, ent->pb, wid->offset_x, wid->offset_y, wid->min_w, wid->min_h);
	}

	if (btn->p.group->bn == hobj)
		img = radio_img_ye;
	else
		img = radio_img_ne;

	egal_composite_image(wid->drawable, wid->pb,
			wid->offset_x, wid->offset_y + (wid->min_h - img->h) / 2, img, 0, 0, img->w, img->h);

	if (GUI_STATUS_FOCUS(hobj)) {
		egal_set_foreground(wid->pb, 0xff00ff);
		egal_draw_rect(wid->drawable, wid->pb,
				wid->offset_x + img->w + 4, wid->offset_y + 1,
				wid->min_w - img->w - 5,  wid->min_h - 2);
	}

	egal_set_foreground(wid->pb, wid->fg_color);
	egui_draw_strings(wid->drawable, wid->pb, 0, btn->label, &rc,  LF_HLeft | LF_VCenter);
	return 0;
}

static eint radio_button_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GUI_BUTTON_DATA(hobj)->down = true;
	egui_update(hobj);
	return 0;
}

static eint radio_button_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);
	if (button->down) {
		button->down = false;
		if (button->enter && button->p.group->bn != hobj) {
			eHandle bn = button->p.group->bn;
			button->p.group->bn = hobj;
			if (bn) egui_update(bn);
			egui_update(hobj);
			e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
		}
	}
	return 0;
}

static eint radio_button_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiButton *bn = GUI_BUTTON_DATA(hobj);
	if (!bn->key_down && 
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		bn->key_down = true;
	}
	return 0;
}

static eint radio_button_keyup(eHandle hobj, GalEventKey *ent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);

	if (button->key_down &&
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		button->key_down = false;
		if (button->p.group->bn != hobj) {
			eHandle bn = button->p.group->bn;
			button->p.group->bn = hobj;
			if (bn) egui_update_async(bn);
			egui_update(hobj);
			e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
		}
	}
	return 0;
}

static void radio_button_init_orders(eGeneType new, ePointer this)
{
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	c->init        = radio_button_init;
	w->expose      = radio_button_expose;
	w->move_resize = button_move_resize;

	e->keyup       = radio_button_keyup;
	e->keydown     = radio_button_keydown;
	e->lbuttonup   = radio_button_lbuttonup;
	e->lbuttondown = radio_button_lbuttondown;
}

eGeneType egui_genetype_radio_button(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GuiResItem *res = egui_res_find(GUI_RES_HANDLE, _("button"));
		eGeneInfo info = {
			0,
			radio_button_init_orders,
			0,
			NULL, NULL, NULL,
		};

		radio_img_ye = egui_res_find_item(res,  _("radio_ye"));
		radio_img_ne = egui_res_find_item(res,  _("radio_no"));

		gtype = e_register_genetype(&info, GTYPE_BUTTON, NULL);
	}
	return gtype;
}

static eint check_button_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiButton *bn  = GUI_BUTTON_DATA(hobj);
	eint w, h;

	bn->label   = e_va_arg(vp, const echar *);
	bn->p.check = e_va_arg(vp, bool);

	egui_strings_extent(0, bn->label, &w, &h);
	wid->min_w  = check_img_y->w + w + 7;
	wid->min_h  = check_img_y->h > h ? check_img_y->h : h + 2;
	wid->max_w  = 1;
	wid->max_h  = 1;
	wid->rect.w = wid->min_w;
	wid->rect.h = wid->min_h;
	wid->fg_color = 0xffffff;

	widget_set_status(wid, GuiStatusTransparent);

	return 0;
}

static eint check_button_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiButton *btn = GUI_BUTTON_DATA(hobj);
	GalRect    rc  = {wid->offset_x + check_img_y->w + 5, wid->offset_y, wid->min_w, wid->min_h};
	GalImage  *img;

	if (btn->enter) {
		egal_set_foreground(wid->pb, 0xc7baa2);
		egal_fill_rect(wid->drawable, ent->pb, wid->offset_x, wid->offset_y, wid->min_w, wid->min_h);
	}

	if (btn->p.check)
		img = check_img_y;
	else
		img = check_img_n;

	egal_composite_image(wid->drawable, wid->pb,
			wid->offset_x, wid->offset_y + (wid->min_h - img->h) / 2, img, 0, 0, img->w, img->h);

	if (GUI_STATUS_FOCUS(hobj)) {
		egal_set_foreground(wid->pb, 0xff00ff);
		egal_draw_rect(wid->drawable, wid->pb,
				wid->offset_x + img->w + 4, wid->offset_y + 1,
				wid->min_w - img->w - 5,  wid->min_h - 2);
	}

	egal_set_foreground(wid->pb, wid->fg_color);
	egui_draw_strings(wid->drawable, wid->pb, 0, btn->label, &rc,  LF_HLeft | LF_VCenter);
	return 0;
}

static eint check_button_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GUI_BUTTON_DATA(hobj)->down = true;
	egui_update(hobj);
	return 0;
}

static eint check_button_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);
	if (button->down) {
		button->down = false;
		if (button->enter) {
			button->p.check = !button->p.check;
			egui_update(hobj);
			e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
		}
	}
	return 0;
}

static eint check_button_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiButton *bn = GUI_BUTTON_DATA(hobj);
	if (!bn->key_down && 
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		bn->key_down = true;
	}
	return 0;
}

static eint check_button_keyup(eHandle hobj, GalEventKey *ent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);

	if (button->key_down &&
			(ent->code == GAL_KC_space || ent->code == GAL_KC_Enter)) {
		button->key_down = false;
		button->p.check = !button->p.check;
		egui_update(hobj);
		e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
	}
	return 0;
}

static void check_button_init_orders(eGeneType new, ePointer this)
{
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	c->init        = check_button_init;
	w->expose      = check_button_expose;
	w->move_resize = button_move_resize;

	e->keyup       = check_button_keyup;
	e->keydown     = check_button_keydown;
	e->lbuttonup   = check_button_lbuttonup;
	e->lbuttondown = check_button_lbuttondown;
}

eGeneType egui_genetype_check_button(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GuiResItem *res = egui_res_find(GUI_RES_HANDLE, _("button"));
		eGeneInfo info = {
			0,
			check_button_init_orders,
			0,
			NULL, NULL, NULL,
		};

		check_img_y = egui_res_find_item(res,  _("radio_ye"));
		check_img_n = egui_res_find_item(res,  _("radio_no"));

		gtype = e_register_genetype(&info, GTYPE_BUTTON, NULL);
	}
	return gtype;
}

eHandle egui_radio_button_new(const echar *label, eHandle group)
{
	return e_object_new(GTYPE_RADIO_BUTTON, label, group);
}

eHandle egui_check_button_new(const echar *label, bool is_check)
{
	return e_object_new(GTYPE_CHECK_BUTTON, label, is_check);
}
