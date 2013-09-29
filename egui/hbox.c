#include "gui.h"
#include "widget.h"
#include "event.h"
#include "bin.h"
#include "box.h"

static void hbox_request_layout(eHandle, eHandle, eint, eint, bool, bool);
static void hbox_init_orders(eGeneType new, ePointer this);
static void hbox_add(eHandle pobj, eHandle cobj);
static void hbox_add_spacing(eHandle pobj, eint size);
static eint hbox_keydown(eHandle, GalEventKey *);
static eint hbox_resize(eHandle, GuiWidget *, GalEventResize *);
static eHandle hbox_next_child(GuiBin *, eHandle, eint, eint);
static void hbox_set_min(eHandle hobj, eint w, eint h);

eGeneType egui_genetype_hbox(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			hbox_init_orders,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BOX, NULL);
	}
	return gtype;
}

static eint    (*bin_keydown)(eHandle, GalEventKey *);
static eHandle (*bin_next_child)(GuiBin *, eHandle, eint, eint);

static void hbox_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);

	bin_keydown    = e->keydown;
	bin_next_child = b->next_child;

	b->next_child  = hbox_next_child;
	b->add_spacing = hbox_add_spacing;
	b->request_layout = hbox_request_layout;

	e->keydown     = hbox_keydown;

	w->add         = hbox_add;
	w->resize      = hbox_resize;
	w->set_min     = hbox_set_min;
}

static eHandle child_next(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->next;
	else
		cw = bin->head;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eHandle child_prev(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->prev;
	else
		cw = bin->tail;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eHandle hbox_next_child(GuiBin *bin, eHandle child, eint dir, eint axis)
{
	if (axis & BIN_Y_AXIS) {
		eHandle (*cb)(GuiBin *, eHandle);

		if (dir == BIN_DIR_NEXT)
			cb = child_next;
		else
			cb = child_prev;

		do {
			child = cb(bin, child);
		} while (child && !bin_can_focus(GUI_WIDGET_DATA(child)));

		return child;
	}

	return bin_next_child(bin, child, dir, axis);
}

static eint hbox_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (bin->focus &&
			(ent->code == GAL_KC_Up ||
			 ent->code == GAL_KC_Down)) {

		eint ox = e_signal_emit(bin->focus, SIG_KEYDOWN, ent);
		if (ox < 0) return -1;

		if (!GUI_TYPE_BIN(bin->focus)) {
			GuiWidget *fw = GUI_WIDGET_DATA(bin->focus);
			ox = fw->rect.x + fw->rect.w / 2;
		}

		return BIN_X_AXIS | (GUI_WIDGET_DATA(hobj)->rect.x + ox);
	}

	return bin_keydown(hobj, ent);
}

static eint hbox_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiBox *box = GUI_BOX_DATA(hobj);

	eint offset_y = box->border_width;
	eint offset_x = box->border_width;
	eint _w = resize->w - box->border_width * 2;
	eint _h = resize->h - box->border_width * 2;
	eint spacing      = 0;
	eint children_w   = 0;
	eint   expand_w;
	BoxPack *p;

	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	switch (box->layout) {
		case BoxLayout_SPREAD:
		case BoxLayout_START_SPACING:
		case BoxLayout_END_SPACING:
		case BoxLayout_CENTER:
			_w -= box->spacing * box->child_num + box->spacing;
			break;
		default:
			_w -= box->spacing * box->child_num - box->spacing;
	}
	expand_w = _w - (box->children_min - box->expand_min);

	p = box->head;
	while (p) {
		if (p->type == BoxPack_SPACING) {
			children_w += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			if (p->wid->max_h == 0)
				p->req_h = _h;
			else
				p->req_h = p->wid->min_h;

			if (p->wid->max_w == 0)
				p->req_w = expand_w * (p->wid->min_w / box->expand_min);
			else
				p->req_w = p->wid->min_w;

			children_w += p->req_w;
		}
		p = p->next;
	}
	spacing = _w - children_w;

	switch (box->layout) {
		case BoxLayout_EDGE:
			if (box->child_num == 1) {
				spacing  += box->spacing;
				offset_x += spacing / 2;
			}
			else {
				spacing = spacing / (box->child_num - 1) + box->spacing;
			}
			break;

		case BoxLayout_SPREAD:
			if (spacing > 0)
				spacing = spacing / (box->child_num + 1) + box->spacing;
			else
				spacing = box->spacing;
			offset_x += spacing;
			break;

		case BoxLayout_START:
			spacing   = box->spacing;
			break;

		case BoxLayout_START_SPACING:
			spacing   = box->spacing;
			offset_x += spacing;
			break;

		case BoxLayout_END:
			offset_x += spacing;
			spacing   = box->spacing;
			break;

		case BoxLayout_END_SPACING:
			offset_x += spacing + box->spacing;
			spacing   = box->spacing;
			break;

		case BoxLayout_CENTER:
			offset_x += spacing / 2 + box->spacing;
			spacing   = box->spacing;
			break;
	}

	p = box->head;
	while (p) {
		if (p->type == BoxPack_SPACING) {
			offset_x += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			eint oh;
			if (box->align == BoxAlignStart)
				oh = 0;
			else if (box->align == BoxAlignEnd)
				oh = _h - p->req_h;
			else
				oh = (_h - p->req_h) / 2;
			if (oh < 0) oh = -oh;
			if (!p->next && offset_x + p->req_w < resize->w)
				egui_move_resize(p->obj.hobj, offset_x + 1, offset_y + oh, p->req_w, p->req_h);
			else
				egui_move_resize(p->obj.hobj, offset_x,     offset_y + oh, p->req_w, p->req_h);
			offset_x += p->req_w + spacing;
		}
		p = p->next;
	}

	return 0;
}

static eint get_expand_w(GuiWidget *pw, GuiBox *box, GuiWidget *cw, eint req_w)
{
	eint max_w = 0;
	eint spacing_min;

	switch (box->layout) {
		case BoxLayout_SPREAD:
		case BoxLayout_START_SPACING:
		case BoxLayout_END_SPACING:
		case BoxLayout_CENTER:
			spacing_min = box->spacing * box->child_num + box->spacing;
			break;
		default:
			spacing_min = box->spacing * box->child_num - box->spacing;
	}

	if (pw->max_w == 0 && box->expand_min > 0) {
		BoxPack *p = box->head;
		eint m;
		while (p) {
			if (p->type != BoxPack_SPACING && p->wid->max_w == 0) {
				if (p->wid == cw)
					m = box->expand_min * req_w / cw->min_w;
				else
					m = box->expand_min * p->wid->rect.w / p->wid->min_w;
				if (max_w < m)
					max_w = m;
			}
			p = p->next;
		}
		max_w += box->border_width * 2 + spacing_min + (box->children_min - box->expand_min);
	}
	else
		max_w = pw->min_w;

	return max_w;
}

static eint get_expand_h(GuiWidget *pw, GuiBox *box, GuiWidget *cw, eint req_h)
{
	eint max_h = 0;

	if (pw->max_h == 0) {
		BoxPack *p = box->head;
		while (p) {
			if (p->type == BoxPack_WIDGET && WIDGET_STATUS_VISIBLE(p->wid)) {
				if (p->wid == cw) {
					if (req_h > max_h)
						max_h = req_h;
				}
				else if (max_h < p->wid->rect.h)
					max_h = p->wid->rect.h;
			}
			p = p->next;
		}
		max_h += box->border_width * 2;
	}
	else
		max_h = pw->min_h;

	return max_h;
}

static void __hbox_set_min(GuiWidget *wid, GuiBox *box)
{
	BoxPack *p = box->head;

	wid->min_h        = 0;
	box->expand_min   = 0;
	box->children_min = 0;

	while (p) {
		if (p->type == BoxPack_SPACING) {
			box->children_min += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			if (p->wid->max_w == 0)
				box->expand_min += p->wid->min_w;

			if (wid->min_h < p->wid->min_h)
				wid->min_h = p->wid->min_h;

			box->children_min += p->wid->min_w;
		}
		p = p->next;
	}

	switch (box->layout) {
		case BoxLayout_SPREAD:
		case BoxLayout_START_SPACING:
		case BoxLayout_END_SPACING:
		case BoxLayout_CENTER:
			wid->min_w = box->spacing * box->child_num + box->spacing;
			break;
		default:
			wid->min_w = box->spacing * box->child_num - box->spacing;
	}

	wid->min_w += box->border_width * 2 + box->children_min;
	wid->min_h += box->border_width * 2;
}

static void hbox_set_min(eHandle hobj, eint w, eint h)
{
	__hbox_set_min(GUI_WIDGET_DATA(hobj), GUI_BOX_DATA(hobj));
}

static void __hbox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool fixed, bool a)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiBox    *box = GUI_BOX_DATA(hobj);

	eHandle tmp;
	eint max_w, max_h;

	__hbox_set_min(wid, box);

	if (a || !fixed) {
		GuiWidget *cw;

		if (cobj)
			cw = GUI_WIDGET_DATA(cobj);
		else
			cw = NULL;

		max_w = get_expand_w(wid, box, cw, req_w);
		max_h = get_expand_h(wid, box, cw, req_h);
		tmp   = hobj;
	}
	else {
		max_w = 0;
		max_h = 0;
		tmp   = 0;
	}

	if (wid->parent)
		egui_request_layout(wid->parent, tmp, max_w, max_h, fixed, a);
	else {
		GalEventResize resize = {max_w, max_h};
		hbox_resize(hobj, wid, &resize);
	}
}

static void hbox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool fixed, bool a)
{
	__hbox_request_layout(hobj, cobj, req_w, req_h, fixed, a);
}

static void hbox_add(eHandle pobj, eHandle cobj)
{
	egui_box_add(pobj, cobj);

	__hbox_request_layout(pobj, 0, 0, 0, false, true);
}

static void hbox_add_spacing(eHandle pobj, eint size)
{
	egui_box_add_spacing(pobj, size);

	__hbox_request_layout(pobj, 0, 0, 0, false, true);
}

eHandle egui_hbox_new(void)
{
	return e_object_new(GTYPE_HBOX);
}
