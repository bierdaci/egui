#include <egui/gui.h>
#include <egui/widget.h>
#include <egui/event.h>

#include "image.h"

static void image_init_orders(eGeneType new, ePointer this);
static eint image_init  (eHandle, eValist vp);
static eint image_expose(eHandle, GuiWidget *, GalEventExpose *);

eGeneType egui_genetype_image(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			image_init_orders,
			sizeof(GuiImage),
			NULL,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

static void image_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

//	w->expose         = image_expose;
	c->init           = image_init;
}

static eint image_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
//	egal_draw_image(wid->drawable, ent->pb, ent->rect.x, ent->rect.y,
//			GUI_IMAGE_DATA(hobj)->img, 
//			ent->rect.x, ent->rect.y, ent->rect.w, ent->rect.h);
	return 0;
}

static eint image_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid   = GUI_WIDGET_DATA(hobj);
//	GuiImage  *image = GUI_IMAGE_DATA(hobj);
//	GalPixbuf *pixbuf;

//	const echar *filename = e_va_arg(vp, echar *);

//	pixbuf = egal_pixbuf_new_from_file(filename, 1.0, 1.0);

//	image->img = egal_image_new_from_pixbuf(pixbuf);
		
	int w = e_va_arg(vp, int);
	int h = e_va_arg(vp, int);

	wid->rect.w = w;
	wid->rect.h = h;
	wid->min_w  = w;
	wid->min_h  = h;
	wid->max_w  = 1;
	wid->max_h  = 1;
	egui_set_status(hobj, GuiStatusVisible|GuiStatusActive);

	return e_signal_emit(hobj, SIG_REALIZE);
}

eHandle egui_image_new(int w, int h)
{
	return e_object_new(GTYPE_IMAGE, w, h);
}

