#include "gui.h"
#include "bin.h"
#include "box.h"
#include "frame.h"

static eint frame_init_data(eHandle, ePointer);
static void frame_init_orders(eGeneType, ePointer);

eGeneType egui_genetype_frame(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			frame_init_orders,
			sizeof(GuiFrame),
			frame_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BOX, NULL);
	}
	return gtype;
}

static eint frame_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiBox   *box = GUI_BOX_DATA(hobj);
	GuiFrame *frame = GUI_FRAME_DATA(hobj);
	GalRect   rect;

	eint bw = box->border_width / 2;

	egal_set_foreground(exp->pb, 0xffffff);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + bw, wid->offset_y + bw,
			wid->offset_x + bw + 2, wid->offset_y + bw);

	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + bw + 6 + frame->title_width, wid->offset_y + bw,
			wid->offset_x + wid->rect.w - bw, wid->offset_y + bw);

	rect.x = wid->offset_x + bw + 4;
	rect.y = wid->offset_y;
	rect.w = frame->title_width;
	rect.h = box->border_width;
	egui_draw_strings(wid->drawable, exp->pb, frame->font, frame->title, &rect, 0);

	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + bw, wid->offset_y + bw,
			wid->offset_x + bw, wid->offset_y + wid->rect.h - bw);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + bw, wid->offset_y + wid->rect.h - bw,
			wid->offset_x + wid->rect.w - bw, wid->offset_y + wid->rect.h - bw);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + wid->rect.w - bw, wid->offset_y + bw,
			wid->offset_x + wid->rect.w - bw, wid->offset_y + wid->rect.h - bw);
	return 0;
}

static eint frame_init(eHandle hobj, eValist vp)
{
	const echar *title = e_va_arg(vp, const echar *);

	if (title) {
		GuiFrame *frame = GUI_FRAME_DATA(hobj);
		egui_strings_extent(frame->font, title, &frame->title_width, NULL);
		frame->title = title;
	}
	return 0;
}

static void frame_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init            = frame_init;
	w->expose_bg       = frame_expose_bg;
}

static eint frame_init_data(eHandle hobj, ePointer this)
{
	GuiFrame *frame = this;
	frame->font = egal_default_font();
	GUI_BOX_DATA(hobj)->border_width = egal_font_height(frame->font) + 2;
	return 0;
}

eHandle egui_frame_new(const echar *title)
{
	return e_object_new(GTYPE_FRAME, title);
}
