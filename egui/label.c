#include "gui.h"
#include "widget.h"
#include "layout.h"
#include "adjust.h"
#include "label.h"

static eint label_init_data(eHandle hobj, ePointer this);
static void label_init_orders(eGeneType, ePointer this);

eGeneType egui_genetype_label(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiLabelOrders),
			label_init_orders,
			sizeof(GuiLabel),
			label_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_LAYOUT, GTYPE_HOOK, NULL);
	}
	return gtype;
}

static eint label_init_data(eHandle hobj, ePointer this)
{
	egui_set_status(hobj, GuiStatusVisible);
	return 0;
}

static void label_set_transparent(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (wid->window) {
		e_object_unref(wid->window);
		e_object_unref(wid->pb);
		wid->window = 0;
		wid->drawable = 0;
		wid->pb = 0;
	}

	if (wid->parent) {
		GuiWidget *pw = GUI_WIDGET_DATA(wid->parent);
		wid->drawable = e_object_refer(pw->drawable);
		wid->pb       = e_object_refer(pw->pb);
		layout_set_offset(hobj, wid->offset_x, wid->offset_y);
	}
}

static void label_unset_transparent(eHandle hobj)
{
	GuiWidget *cw = GUI_WIDGET_DATA(hobj);

	if (cw->drawable) {
		e_object_unref(cw->drawable);
		e_object_unref(cw->pb);
		cw->drawable = 0;
		cw->pb       = 0;
	}
	if (cw->parent) {
		GuiWidget *pw = GUI_WIDGET_DATA(cw->parent);
		e_signal_emit(hobj, SIG_REALIZE, cw->rect.w, cw->rect.h);
		egal_window_put(pw->drawable, cw->drawable, cw->offset_x, cw->offset_y);
	}
	layout_set_offset(hobj, 0, 0);
}

static void label_set_offset(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	layout_set_offset(hobj, wid->offset_x, wid->offset_y);
}

static void label_put(eHandle hobj, eHandle cobj)
{
	GuiWidget *cw = GUI_WIDGET_DATA(cobj);
	if (cw->window) {
		GuiWidget *pw = GUI_WIDGET_DATA(hobj);
		egal_window_put(pw->drawable, cw->drawable, cw->offset_x, cw->offset_y);
	}
}

static void label_show(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (!WIDGET_STATUS_TRANSPARENT(wid)) {
		GuiWidgetOrders *ws = e_type_orders(GTYPE_WIDGET);
		ws->show(hobj);
	}
}

static void label_hide(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (!WIDGET_STATUS_TRANSPARENT(wid)) {
		GuiWidgetOrders *ws = e_type_orders(GTYPE_WIDGET);
		ws->hide(hobj);
	}
}

static void label_move(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->window)
		egal_window_move(wid->window, wid->offset_x, wid->offset_y);

	if (WIDGET_STATUS_TRANSPARENT(wid))
		layout_set_offset(hobj, wid->offset_x, wid->offset_y);
}

static eint label_expose(eHandle hobj, GuiWidget *widget, GalEventExpose *ent)
{
	egal_set_foreground(widget->pb, widget->fg_color);
	layout_draw(hobj, widget->drawable, widget->pb, &ent->rect);
	return 0;
}

static void label_set_min(eHandle hobj, eint w, eint h)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (w > 0) wid->min_w = w;
	if (h > 0) wid->min_h = h;

	if (wid->window) {
		GalGeometry geom;
		geom.min_width  = wid->min_w;
		geom.min_height = wid->min_h;
		egal_window_set_geometry_hints(wid->window, &geom, GAL_HINT_MIN_SIZE);
	}
}

static eint label_expose_bg(eHandle hobj, GuiWidget *widget, GalEventExpose *ent)
{
	if (!WIDGET_STATUS_TRANSPARENT(widget)) {
		egal_set_foreground(widget->pb, widget->bg_color);
		egal_fill_rect(widget->drawable, widget->pb,
				ent->rect.x, ent->rect.y,
				ent->rect.w, ent->rect.h);
	}
	return 0;
}

static eint label_resize(eHandle hobj, GuiWidget *widget, GalEventResize *resize)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	widget->rect.w = resize->w;
	widget->rect.h = resize->h;
	layout->w = resize->w;
	layout->h = resize->h;
	layout->configure = efalse;
	egui_update(hobj);
	return 0;
}

static void label_hook_v(eHandle hobj, eHandle hook)
{
	egui_adjust_hook(GUI_LAYOUT_DATA(hobj)->vadj, hook);
}

static void label_hook_h(eHandle hobj, eHandle hook)
{
	egui_adjust_hook(GUI_LAYOUT_DATA(hobj)->hadj, hook);
}

static eint label_init(eHandle hobj, eValist vp)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);

	const echar *text = e_va_arg(vp, const echar *);

	layout_set_strings(hobj, text);
	layout_get_extents(layout, &layout->w, &layout->h);

	widget->min_w    = layout->w;
	widget->min_h    = layout->h;
	widget->rect.w   = layout->w;
	widget->rect.h   = layout->h;
	widget->fg_color = 0;
	widget->bg_color = 0xffffff;

	return e_signal_emit(hobj, SIG_REALIZE);
}

static void label_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiHookOrders   *h = e_genetype_orders(new, GTYPE_HOOK);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init              = label_init;

	w->put               = label_put;
	w->show              = label_show;
	w->hide              = label_hide;
	w->move              = label_move;
	w->resize            = label_resize;
	w->expose            = label_expose;
	w->set_min           = label_set_min;
	w->expose_bg         = label_expose_bg;
	w->set_offset        = label_set_offset;
	w->set_transparent   = label_set_transparent;
	w->unset_transparent = label_unset_transparent;

	h->hook_v            = label_hook_v;
	h->hook_h            = label_hook_h;
}

eHandle egui_label_new(const echar *text)
{
	return e_object_new(GTYPE_LABEL, text);
}

void egui_label_set_text_with_mnemonic(eHandle hobj, const echar *text, eint len)
{
}

static eint simple_label_init(eHandle hobj, eValist vp)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);

	const echar *text = e_va_arg(vp, const echar *);

	layout_set_strings(hobj, text);
	layout_get_extents(layout, &layout->w, &layout->h);

	widget->min_w    = layout->w;
	widget->min_h    = layout->h;
	widget->max_w    = 1;
	widget->max_h    = 1;
	widget->rect.w   = layout->w;
	widget->rect.h   = layout->h;
	widget->fg_color = 0xffffff;
	widget_set_status(widget, GuiStatusVisible | GuiStatusTransparent);

	return 0;
}

static eint simple_label_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GalRect rc = {wid->offset_x, wid->offset_y, wid->rect.w, wid->rect.h};
	egal_set_foreground(wid->pb, wid->fg_color);
	layout_draw(hobj, wid->drawable, wid->pb, &rc);
	return 0;
}

static eint simple_label_resize(eHandle hobj, GuiWidget *widget, GalEventResize *resize)
{
	layout_set_offset(hobj, widget->offset_x, widget->offset_y);
	return 0;
}

static void (*__layout_set_text)(eHandle, const echar *, eint);

static void simple_label_set_text(eHandle hobj, const echar *text, eint len)
{
	GuiLayout *layout = GUI_LAYOUT_DATA(hobj);
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);

	layout_get_extents(layout, &layout->w, &layout->h);

	widget->min_w  = layout->w;
	widget->rect.w = layout->w;
	__layout_set_text(hobj, text, len);
}

static void simple_label_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders  *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders      *c = e_genetype_orders(new, GTYPE_CELL);
	GuiStringsOrders *s = e_genetype_orders(new, GTYPE_STRINGS);

	c->init   = simple_label_init;
	w->expose = simple_label_expose;
	w->resize = simple_label_resize;

	__layout_set_text = s->set_text;
	s->set_text = simple_label_set_text;
}

eGeneType egui_genetype_simple_label(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0, simple_label_init_orders,
			0, NULL, NULL, NULL };

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_LAYOUT, NULL);
	}
	return gtype;
}

eHandle egui_simple_label_new(const echar *text)
{
	return e_object_new(GTYPE_SIMPLE_LABEL, text);
}
