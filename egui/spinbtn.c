#include "gui.h"
#include "box.h"
#include "char.h"
#include "event.h"
#include "entry.h"
#include "adjust.h"
#include "spinbtn.h"
#include "res.h"

static GuiResItem *common_res = NULL;
static GalImage   *bn_up      = NULL;
static GalImage   *bn_down    = NULL;

static eint (*hbox_keydown)(eHandle, GalEventKey *);

static eint spinbtn_init(eHandle, eValist);
static void spinbn_init_orders(eGeneType, ePointer);
static void bn_init_orders(eGeneType, ePointer);

static eGeneType GTYPE_BN = 0;
typedef struct {
	eint n;
	bool down;
	GalImage *img;
} SpinBN;
#define BN_DATA(hobj)  ((SpinBN *)e_object_type_data(hobj, GTYPE_BN))

eGeneType egui_genetype_spinbtn(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			spinbn_init_orders,
			sizeof(GuiSpinBtn),
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_HBOX, GTYPE_STRINGS, NULL);

		info.orders_size  = 0;
		info.init_orders  = bn_init_orders;
		info.object_size  = sizeof(SpinBN);
		info.init_data    = NULL;
		info.free_data    = NULL;
		info.init_gene    = NULL;
		GTYPE_BN = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);

		common_res = egui_res_find(GUI_RES_HANDLE,   _("common"));
		bn_up      = egui_res_find_item(common_res,  _("up"));
		bn_down    = egui_res_find_item(common_res,  _("down"));
	}
	return gtype;
}

static void spinbn_set_value(GuiSpinBtn *spin, efloat value)
{
	GuiEntry *entry = GUI_ENTRY_DATA(spin->entry);
	echar buf[200], *p;
	eint  len;

	e_sprintf(buf, _("%f"), spin->value);
	p = e_strchr(buf, '.');
	if (spin->digits > 0)
		p[spin->digits + 1] = 0;
	else
		p[spin->digits] = 0;
	len = e_strlen(buf);

	if (value == spin->value
			&& len == entry->nchar
			&& !e_strncmp(buf, entry->chars, len))
		return;

	if (value < spin->min)
		spin->value = spin->max;
	else if (value > spin->max)
		spin->value = spin->min;
	else
		spin->value = value;

	e_sprintf(buf, _("%f"), spin->value);
	p = e_strchr(buf, '.');
	if (spin->digits > 0)
		p[spin->digits + 1] = 0;
	else
		p[spin->digits] = 0;

	egui_set_strings(spin->entry, buf);
}

static void spinbn_right_value(GuiSpinBtn *spin)
{
	GuiEntry *entry = GUI_ENTRY_DATA(spin->entry);
#ifdef linux
	echar buf[entry->nchar + 1];
#else
	echar buf[1024];
#endif

	e_memcpy(buf, entry->chars, entry->nchar);
	buf[entry->nchar] = 0;

	spinbn_set_value(spin, (efloat)e_atof(buf));
}

static eint spinbn_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiSpinBtn *spin = GUI_SPINBTN_DATA(hobj);

	switch (ent->code) {
		case GAL_KC_Up:
			spinbn_right_value(spin);
			spinbn_set_value(spin, spin->value + spin->step_inc);
			return -1;

		case GAL_KC_Down:
			spinbn_right_value(spin);
			spinbn_set_value(spin, spin->value - spin->step_inc);
			return -1;

		default: break;
	}
	return hbox_keydown(hobj, ent);
}

static eint spinbn_focus_in(eHandle hobj)
{
	return 0;
}

static eint spinbn_focus_out(eHandle hobj)
{
	spinbn_right_value(GUI_SPINBTN_DATA(hobj));
	return 0;
}

static void spinbn_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init            = spinbtn_init;

	hbox_keydown       = e->keydown;
	e->keydown         = spinbn_keydown;
	e->focus_in        = spinbn_focus_in;
	e->focus_out       = spinbn_focus_out;
}

static eint timer_down_cb(eTimer timer, euint num, ePointer args)
{
	GuiSpinBtn *spin = args;
	spin->time += num;
	if (spin->time > 5) {
		efloat value = spin->value - spin->step_inc * num;
		spinbn_set_value(spin, value);
	}
	return 0;
}

static eint timer_up_cb(eTimer timer, euint num, ePointer args)
{
	GuiSpinBtn *spin = args;
	spin->time += num;
	if (spin->time > 5) {
		efloat value = spin->value + spin->step_inc * num;
		spinbn_set_value(spin, value);
	}
	return 0;
}

static eint bn_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiSpinBtn *spin = GUI_WIDGET_DATA(hobj)->extra_data;
	SpinBN       *bn = BN_DATA(hobj);
	efloat     value = spin->value;

	spinbn_right_value(spin);

	value = spin->value;
	bn->down = true;
	egui_update(hobj);

	if (bn->n == 1) {
		value += spin->step_inc;
		spin->time  = 0;
		spin->timer = e_timer_add(50, timer_up_cb, spin);
	}
	else {
		value -= spin->step_inc;
		spin->time  = 0;
		spin->timer = e_timer_add(50, timer_down_cb, spin);
	}

	spinbn_set_value(spin, value);
	egui_set_focus(spin->entry);
#ifdef WIN32
	egal_grab_pointer(GUI_WIDGET_DATA(hobj)->window, true, 0);
#endif
	return 0;
}

static eint bn_lbuttonup(eHandle hobj, GalEventMouse *ent)
{
	GuiSpinBtn *spin = GUI_WIDGET_DATA(hobj)->extra_data;
	SpinBN     *bn   = BN_DATA(hobj);

	if (spin->timer) {
		e_timer_del(spin->timer);
		spin->timer = 0;
	}
	bn->down = false;
	egui_update(hobj);
#ifdef WIN32
	egal_ungrab_pointer(GUI_WIDGET_DATA(hobj)->window);
#endif
	return 0;
}

static eint bn_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	SpinBN *bn = BN_DATA(hobj);

	if (bn->down)
		egal_set_foreground(exp->pb, 0x9a958d);
	else
		egal_set_foreground(exp->pb, 0xeee8df);

	egal_fill_rect(wid->drawable, exp->pb, 0, 0, wid->rect.w, wid->rect.h);

	egal_composite_image(wid->drawable, exp->pb,
			(wid->rect.w - bn->img->w) / 2,
			(wid->rect.h - bn->img->h) / 2,
			bn->img, 0, 0, bn->img->w, bn->img->h);

	return 0;
}

static eint bn_init(eHandle hobj, eValist vp)
{
	SpinBN    *bn  = BN_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	bn->n           = e_va_arg(vp, eint);
	bn->img         = e_va_arg(vp, GalImage *);
	wid->min_h      = e_va_arg(vp, eint);
	wid->min_w      = wid->min_h + 2;
	wid->rect.w     = wid->min_w;
	wid->rect.h     = wid->min_h;
	wid->extra_data = e_va_arg(vp, ePointer);
	widget_set_status(wid, GuiStatusVisible | GuiStatusMouse);

	return e_signal_emit(hobj, SIG_REALIZE);
}

static void bn_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init        = bn_init;
	w->expose      = bn_expose;
	e->lbuttonup   = bn_lbuttonup;
	e->lbuttondown = bn_lbuttondown;
}

static eint spinbtn_init(eHandle hobj, eValist vp)
{
	GuiSpinBtn *spin = GUI_SPINBTN_DATA(hobj);
	echar *p;
	echar buf[100];
	eint  bn_size;
	eint  s1, s2;

	spin->value    = (efloat)e_va_arg(vp, edouble);
	spin->min      = (efloat)e_va_arg(vp, edouble);
	spin->max      = (efloat)e_va_arg(vp, edouble);
	spin->step_inc = (efloat)e_va_arg(vp, edouble);
	spin->page_inc = (efloat)e_va_arg(vp, edouble);
	spin->digits   = e_va_arg(vp, eint);

	e_sprintf(buf, _("%d"), (eint)spin->max);
	s1 = e_strlen(buf);
	e_sprintf(buf, _("%d"), (eint)spin->min);
	s2 = e_strlen(buf);
	s1 = s1 >= s2 ? s1 : s2;

	GUI_WIDGET_DATA(hobj)->max_w = 0;
	spin->entry = egui_entry_new(20);
	spin->vbox  = egui_vbox_new();
	egui_unset_status(spin->vbox, GuiStatusActive);

	e_sprintf(buf, _("%f"), spin->value);
	p = e_strchr(buf, '.');
	if (spin->digits > 0) {
		p[spin->digits + 1] = 0;
		s1 += spin->digits + 1;
	}
	else
		p[spin->digits] = 0;

	egui_set_max_text(spin->entry, s1);
	egui_set_strings(spin->entry, buf);

	bn_size   = GUI_ENTRY_DATA(spin->entry)->h / 2;
	spin->bn1 = e_object_new(GTYPE_BN, 1, bn_up,   bn_size, spin);
	spin->bn2 = e_object_new(GTYPE_BN, 2, bn_down, bn_size, spin);

	egui_add(spin->vbox, spin->bn1);
	if (GUI_ENTRY_DATA(spin->entry)->h % 2)
		egui_add_spacing(spin->vbox, 1);
	egui_add(spin->vbox, spin->bn2);

	egui_add(hobj, spin->entry);
	egui_add(hobj, spin->vbox);

	return 0;
}

eHandle egui_spinbtn_new(efloat value, efloat min, efloat max, efloat step_inc, efloat page_inc, eint digits)
{
	return e_object_new(GTYPE_SPINBTN, value, min, max, step_inc, page_inc, digits);
}
