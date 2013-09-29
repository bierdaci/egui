#include "gui.h"
#include "widget.h"
#include "anim.h"

static eint anim_init(eHandle, eValist);
static eint anim_init_data(eHandle, ePointer);
static void anim_free_data(eHandle, ePointer);
static void anim_init_orders(eGeneType, ePointer);
static void anim_show(eHandle);
static void anim_hide(eHandle);

eGeneType egui_genetype_anim(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			anim_init_orders,
			sizeof(GuiAnimData),
			anim_init_data,
			anim_free_data,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, NULL);
	}
	return gtype;
}

static void anim_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	c->init  = anim_init;
	w->hide  = anim_hide;
	w->show  = anim_show;
}

static eint anim_init_data(eHandle hobj, ePointer this)
{
	egui_set_status(hobj, GuiStatusVisible);
	return 0;
}

static void anim_free_data(eHandle hobj, ePointer this)
{
	GuiAnimData *d = this;
	if (d->timer != 0)
		e_timer_del(d->timer);
	egal_image_free(d->image);
	e_object_unref(d->pb);
}

static eint anim_timer_cb(eTimer timer, euint num, ePointer args)
{
	GuiAnimData *d  = GUI_ANIM_DATA((eHandle)args);

	d->elapsed += num * 20;
	while (d->frame && d->elapsed >= d->frame->elapsed) {
		egal_image_copy_from_pixbuf(d->image,
				d->frame->x_offset, d->frame->y_offset,
				d->frame->pixbuf, 0, 0, d->frame->pixbuf->w, d->frame->pixbuf->h);
		egal_draw_image(d->drawable, d->pb,
				d->frame->x_offset, d->frame->y_offset,
				d->image, 0, 0, d->frame->pixbuf->w, d->frame->pixbuf->h);
		d->frame = d->frame->next;
	}

	if (d->frame == NULL) {
		d->frame = d->anim->frames;
		d->elapsed = 0;
	}
	return 0;
}

static void anim_show(eHandle hobj)
{
	GuiAnimData *d  = GUI_ANIM_DATA(hobj);
	if (d->timer == 0)
		d->timer = e_timer_add(20, anim_timer_cb, (ePointer)hobj);
}

static void anim_hide(eHandle hobj)
{
	GuiAnimData *d  = GUI_ANIM_DATA(hobj);
	if (d->timer) {
		e_timer_del(d->timer);
		d->timer  = 0;
	}
}

static eint anim_init(eHandle hobj, eValist vp)
{
	GuiAnimData *d = GUI_ANIM_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	GalPixbufAnim *anim = e_va_arg(vp, GalPixbufAnim *);

	wid->rect.w  = 1;
	wid->rect.h  = 1;
	wid->max_w  = 1;
	wid->max_h  = 1;
	wid->min_w  = anim->width;
	wid->min_h  = anim->height;
	e_signal_emit(hobj, SIG_REALIZE, anim->width, anim->height);

	d->anim     = anim;
	d->frame    = anim->frames;
	d->drawable = wid->drawable;
	d->pb       = egal_pb_new(d->drawable, NULL);
	d->image    = egal_image_new(anim->width, anim->height, false);
	d->timer    = e_timer_add(20, anim_timer_cb, (ePointer)hobj);

	return 0;
}

eHandle egui_anim_new(GalPixbufAnim *anim)
{
	return e_object_new(GTYPE_ANIM, anim);
}
