#include "gui.h"
#include "widget.h"
#include "bin.h"
#include "box.h"

static void box_init_orders(eGeneType new, ePointer this);
static eint box_init_data(eHandle hobj, ePointer data);

eGeneType egui_genetype_box(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			box_init_orders,
			sizeof(GuiBox),
			box_init_data,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

static eint box_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiBox  *box  = GUI_BOX_DATA(hobj);
	BoxPack *p = box->head;

	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	if (p) {
		GuiWidget *cw = GUI_WIDGET_DATA(p->obj.hobj);

		if (cw->max_w == 0)
			p->req_w = resize->w - box->border_width * 2;
		else
			p->req_w = cw->min_w;

		if (cw->max_h == 0)
			p->req_h = resize->h - box->border_width * 2;
		else
			p->req_h = cw->min_h;

		egui_move_resize(p->obj.hobj,
				(resize->w - box->border_width * 2 - p->req_w) / 2 + box->border_width,
				(resize->h - box->border_width * 2 - p->req_h) / 2 + box->border_width,
				p->req_w, p->req_h);
	}
	return 0;
}

static void box_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool fixed, bool a)
{
	GuiBox *box = GUI_BOX_DATA(hobj);

	if (box->head) {
		GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
		wid->min_w = box->border_width * 2 + box->head->wid->min_w;
		wid->min_h = box->border_width * 2 + box->head->wid->min_h;

		wid->rect.w = req_w + box->border_width * 2;
		wid->rect.h = req_h + box->border_width * 2;
		if (wid->parent)
			egui_request_layout(wid->parent, hobj, wid->rect.w, wid->rect.h, fixed, a);
		else {
			GalEventResize resize = {wid->rect.w, wid->rect.h};
			box_resize(hobj, GUI_WIDGET_DATA(hobj), &resize);
		}
	}
}

static void box_add(eHandle hobj, eHandle cobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (!bin->head) {
		GuiWidget *cw = GUI_WIDGET_DATA(cobj);
		egui_box_add(hobj, cobj);
		box_request_layout(hobj, cobj, cw->min_w, cw->min_h, false, true);
	}
}

static void (*bin_remove)(eHandle, eHandle);
static void box_remove(eHandle pobj, eHandle cobj)
{
	GuiBox  *box = GUI_BOX_DATA(pobj);
	BoxPack **pp = &box->head;
	
	while (*pp) {
		if ((*pp)->obj.hobj == cobj) {
			BoxPack *p = *pp;
			*pp = (*pp)->next;
			e_free(p);
			break;
		}
		pp = &(*pp)->next;
	}

	bin_remove(pobj, cobj);
}

static void box_init_orders(eGeneType new, ePointer this)
{
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	bin_remove         = w->remove;
	w->add             = box_add;
	w->resize          = box_resize;
	w->remove          = box_remove;
	b->request_layout  = box_request_layout;
}

static eint box_init_data(eHandle hobj, ePointer data)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->max_w = 1;
	wid->max_h = 1;
	return 0;
}

eHandle egui_box_new(void)
{
	return e_object_new(GTYPE_BOX);
}

void egui_box_set_border_width(eHandle hobj, eint width)
{
	GuiBox *box = GUI_BOX_DATA(hobj);
	box->border_width = width;
}

void egui_box_set_spacing(eHandle hobj, eint spacing)
{
	GuiBox  *box = GUI_BOX_DATA(hobj);
	box->spacing = spacing;
}

void egui_box_set_layout(eHandle hobj, GuiBoxLayout layout)
{
	GuiBox *box = GUI_BOX_DATA(hobj);
	box->layout = layout;
}

void egui_box_set_align(eHandle hobj, GuiBoxAlign align)
{
	GuiBox *box = GUI_BOX_DATA(hobj);
	box->align  = align;
}

void egui_box_add(eHandle pobj, eHandle cobj)
{
	GuiWidget *cw  = GUI_WIDGET_DATA(cobj);
	GuiBin    *bin = GUI_BIN_DATA(pobj);
	GuiBox    *box = GUI_BOX_DATA(pobj);
	BoxPack   *new_pack;

	new_pack = e_malloc(sizeof(BoxPack));
	new_pack->type     = BoxPack_WIDGET;
	new_pack->wid      = cw;
	new_pack->obj.hobj = cobj;
	new_pack->next     = NULL;
	box->child_num++;
	olist_add_tail(BoxPack, box->head, new_pack, next);

	cw->parent = pobj;
	STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, cw, prev, next);
	GUI_WIDGET_ORDERS(pobj)->put(pobj, cobj);
}

void egui_box_add_spacing(eHandle pobj, eint spacing)
{
	GuiBox    *box = GUI_BOX_DATA(pobj);
	BoxPack   *new_pack;

	new_pack = e_malloc(sizeof(BoxPack));
	new_pack->type = BoxPack_SPACING;
	new_pack->obj.spacing = spacing;
	olist_add_tail(BoxPack, box->head, new_pack, next);
}
