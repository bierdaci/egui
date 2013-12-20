#include "gui.h"
#include "widget.h"
#include "window.h"

static void widget_init_orders(eGeneType, ePointer);
static void widget_free_data(eHandle, ePointer);
static void widget_init_gene(eGeneType);

esig_t widget_signal_hide           = 0;
esig_t widget_signal_show           = 0;
esig_t widget_signal_realize        = 0;
esig_t widget_signal_configure      = 0;
esig_t widget_signal_expose         = 0;
esig_t widget_signal_resize         = 0;
esig_t widget_signal_expose_bg      = 0;
esig_t widget_signal_destroy        = 0;

eGeneType egui_genetype_widget(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiWidgetOrders),
			widget_init_orders,
			sizeof(GuiWidget),
			NULL,
			widget_free_data,
			widget_init_gene,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

static bool widget_can_map(GuiWidget *wid)
{
	while (wid->parent) {
		wid = GUI_WIDGET_DATA(wid->parent);
		if (!WIDGET_STATUS_VISIBLE(wid))
			return false;
		if (wid->window)
			return true;
	}

	return false;
}

static void widget_set_min(eHandle hobj, eint w, eint h)
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

/*
static void widget_set_max(eHandle hobj, eint w, eint h)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (w > wid->min_w)
		wid->max_w = w;
	if (h > wid->min_h)
		wid->max_h = h;
	if (wid->window) {
		GalGeometry geom;
		geom.max_width  = wid->max_w;
		geom.max_height = wid->max_h;
		egal_window_set_geometry_hints(wid->window, &geom, GAL_HINT_MAX_SIZE);
	}
}
*/

static void widget_show(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (WIDGET_STATUS_VISIBLE(wid) && widget_can_map(wid)) {
		if (wid->window)
			egal_window_show(wid->window);
	}
}

static void widget_hide(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->window)
		egal_window_hide(wid->window);
}

static void widget_realize(eHandle hobj, GuiWidget *wid)
{
	GalWindowAttr attr = {0};

	if (wid->rect.w == 0 || wid->rect.h == 0)
		return;

	attr.width  = wid->rect.w;
	attr.height = wid->rect.h;
	attr.type   = GalWindowChild;
	attr.wclass = GAL_INPUT_OUTPUT;
	attr.output_event = true;
	if (GUI_STATUS_VISIBLE(hobj))
		attr.visible = true;

	wid->window   = egal_window_new(&attr);
	wid->drawable = wid->window;
	wid->pb       = egal_default_pb();
	egal_window_set_attachment(wid->drawable, hobj);
}

static void widget_move(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->window)
		egal_window_move(wid->window, wid->offset_x, wid->offset_y);
}

static void widget_move_resize(eHandle hobj, GalRect *orc, GalRect *nrc)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GalEventResize resize = {wid->rect.w, wid->rect.h};
	e_signal_emit(hobj, SIG_RESIZE, &resize);
}

static void widget_request_resize(eHandle hobj, eint w, eint h)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GalEventResize resize;

	if (w < wid->min_w || wid->max_w != 0)
		resize.w = wid->min_w;
	else
		resize.w = w;

	if (h < wid->min_h || wid->max_h != 0)
		resize.h = wid->min_h;
	else
		resize.h = h;

	if (wid->window)
		egal_window_resize(wid->window, resize.w, resize.h);

	e_signal_emit(hobj, SIG_RESIZE, &resize);
}

static eint widget_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	return 0;
}

static void widget_set_fg_color(eHandle hobj, euint32 color)
{
	GUI_WIDGET_DATA(hobj)->fg_color = color;
}

static void widget_set_bg_color(eHandle hobj, euint32 color)
{
	GUI_WIDGET_DATA(hobj)->bg_color = color;
}

static void widget_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = this;
	w->show            = widget_show;
	w->hide            = widget_hide;
	w->move            = widget_move;
	w->resize          = widget_resize;
	w->realize         = widget_realize;
	w->set_min         = widget_set_min;
	w->set_fg_color    = widget_set_fg_color;
	w->set_bg_color    = widget_set_bg_color;
	w->move_resize     = widget_move_resize;
	w->request_resize  = widget_request_resize;
}

static void widget_free_data(eHandle hobj, ePointer data)
{
	GuiWidget *wid = data;
	if (wid->drawable)
		e_object_unref(wid->drawable);
	if (wid->pb)
		e_object_unref(wid->pb);
}

static void widget_init_gene(eGeneType new)
{
	widget_signal_hide = e_signal_new("hide",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, hide),
			false, 0, NULL);
	widget_signal_show = e_signal_new("show",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, show),
			false, 0, NULL);

	widget_signal_realize = e_signal_new("realize",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, realize),
			true, 0, NULL);
	widget_signal_resize = e_signal_new("resize",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, resize),
			true, 0, "%p");
	widget_signal_expose = e_signal_new("expose",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, expose),
			true, 0, "%p");
	widget_signal_configure = e_signal_new("configure",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, configure),
			true, 0, "%p");
	widget_signal_expose_bg = e_signal_new("expose_bg",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, expose_bg),
			true, 0, "%p");
	widget_signal_destroy = e_signal_new("destroy",
			new,
			STRUCT_OFFSET(GuiWidgetOrders, destroy),
			false, 0, NULL);
}

eGeneType egui_genetype_font(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiFontOrders),
			NULL,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}
