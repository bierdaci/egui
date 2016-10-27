#include "gui.h"
#include "widget.h"
#include "event.h"
#include "bin.h"
#include "box.h"

static void vbox_request_layout(eHandle, eHandle, eint, eint, ebool, ebool);
static void vbox_init_orders(eGeneType new, ePointer this);
static void vbox_add(eHandle pobj, eHandle cobj);
static void vbox_add_spacing(eHandle pobj, eint size);
static eint vbox_keydown(eHandle, GalEventKey *);
static eint vbox_resize(eHandle, GuiWidget *, GalEventResize *);
static eHandle vbox_next_child(GuiBin *, eHandle, eint, eint);
static void vbox_set_min(eHandle hobj, eint w, eint h);

eGeneType egui_genetype_vbox(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			vbox_init_orders,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BOX, NULL);
	}
	return gtype;
}

static eint    (*bin_keydown)(eHandle, GalEventKey *);
static eHandle (*bin_next_child)(GuiBin *, eHandle, eint, eint);

static void vbox_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);

	bin_keydown    = e->keydown;
	bin_next_child = b->next_child;

	b->next_child  = vbox_next_child;
	b->add_spacing = vbox_add_spacing;
	b->request_layout = vbox_request_layout;

	e->keydown     = vbox_keydown;

	w->add         = vbox_add;
	w->resize      = vbox_resize;
	w->set_min     = vbox_set_min;
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

static eHandle vbox_next_child(GuiBin *bin, eHandle child, eint dir, eint axis)
{
	if (axis & BIN_X_AXIS) {
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

static eint vbox_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (bin->focus &&
			(ent->code == GAL_KC_Right ||
			 ent->code == GAL_KC_Left)) {

		eint oy = e_signal_emit(bin->focus, SIG_KEYDOWN, ent);
		if (oy < 0) return -1;

		if (!GUI_TYPE_BIN(bin->focus)) {
			GuiWidget *fw = GUI_WIDGET_DATA(bin->focus);
			oy = fw->rect.y + fw->rect.h / 2;
		}

		return BIN_Y_AXIS | (GUI_WIDGET_DATA(hobj)->rect.y + oy);
	}

	return bin_keydown(hobj, ent);
}

static eint vbox_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiBox *box = GUI_BOX_DATA(hobj);

	eint offset_y = box->border_width;
	eint offset_x = box->border_width;
	eint _w = resize->w - box->border_width * 2;
	eint _h = resize->h - box->border_width * 2;
	eint spacing      = 0;
	eint children_h   = 0;
	eint   expand_h;
	BoxPack *p;

	wid->rect.w = resize->w;
	wid->rect.h = resize->h;
	switch (box->layout) {
		case BoxLayout_SPREAD:
		case BoxLayout_START_SPACING:
		case BoxLayout_END_SPACING:
		case BoxLayout_CENTER:
			_h -= box->spacing * box->child_num + box->spacing;
			break;
		default:
			_h -= box->spacing * box->child_num - box->spacing;
	}
	expand_h = _h - ((eint)box->children_min - (eint)box->expand_min);

	p = box->head;
	while (p) {
		if (p->type == BoxPack_SPACING) {
			children_h += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			if (p->wid->max_w == 0)
				p->req_w = _w;
			else
				p->req_w = p->wid->min_w;

			if (p->wid->max_h == 0)
				p->req_h = (eint)(expand_h * (p->wid->min_h / box->expand_min));
			else
				p->req_h = p->wid->min_h;

			children_h += p->req_h;
		}
		p = p->next;
	}
	spacing = _h - children_h;

	switch (box->layout) {
		case BoxLayout_EDGE:
			if (box->child_num == 1) {
				spacing  += box->spacing;
				offset_y += spacing / 2;
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
			offset_y += spacing;
			break;

		case BoxLayout_START:
			spacing   = box->spacing;
			break;

		case BoxLayout_START_SPACING:
			spacing   = box->spacing;
			offset_y += spacing;
			break;

		case BoxLayout_END:
			offset_y += spacing;
			spacing   = box->spacing;
			break;

		case BoxLayout_END_SPACING:
			offset_y += spacing + box->spacing;
			spacing   = box->spacing;
			break;

		case BoxLayout_CENTER:
			offset_y += spacing / 2 + box->spacing;
			spacing   = box->spacing;
			break;
	}

	p = box->head;
	while (p) {
		if (p->type == BoxPack_SPACING) {
			offset_y += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			eint ow;
			if (box->align == BoxAlignStart)
				ow = 0;
			else if (box->align == BoxAlignEnd)
				ow = _w - p->req_w;
			else
				ow = (_w - p->req_w) / 2;
			if (ow < 0) ow = -ow;
			if (!p->next && offset_y + p->req_h < resize->h)
				egui_move_resize(p->obj.hobj, offset_x + ow, offset_y + 1, p->req_w, p->req_h);
			else
				egui_move_resize(p->obj.hobj, offset_x + ow, offset_y,     p->req_w, p->req_h);
			offset_y += p->req_h + spacing;
		}
		p = p->next;
	}

	return 0;
}

static eint get_expand_h(GuiWidget *pw, GuiBox *box, GuiWidget *cw, eint req_h)
{
	eint max_h = 0;
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

	if (pw->max_h == 0 && box->expand_min > 0) {
		BoxPack *p = box->head;
		eint m;
		while (p) {
			if (p->type != BoxPack_SPACING && p->wid->max_h == 0) {
				if (p->wid == cw)
					m = (eint)(box->expand_min * req_h / (efloat)cw->min_h);
				else
					m = (eint)(box->expand_min * p->wid->rect.h / (efloat)p->wid->min_h);
				if (max_h < m)
					max_h = m;
			}
			p = p->next;
		}
		max_h += box->border_width * 2 + spacing_min + ((eint)box->children_min - (eint)box->expand_min);
	}
	else
		max_h = pw->min_h;

	return max_h;
}

static eint get_expand_w(GuiWidget *pw, GuiBox *box, GuiWidget *cw, eint req_w)
{
	eint max_w = 0;

	if (pw->max_w == 0) {
		BoxPack *p = box->head;
		while (p) {
			if (p->type == BoxPack_WIDGET && WIDGET_STATUS_VISIBLE(p->wid)) {
				if (p->wid == cw) {
					if (req_w > max_w)
						max_w = req_w;
				}
				else if (max_w < p->wid->rect.w)
					max_w = p->wid->rect.w;
			}
			p = p->next;
		}
		max_w += box->border_width * 2;
	}
	else
		max_w = pw->min_w;

	return max_w;
}

static void __vbox_set_min(GuiWidget *wid, GuiBox *box)
{
	BoxPack *p = box->head;

	wid->min_w        = 0;
	box->expand_min   = 0;
	box->children_min = 0;

	while (p) {
		if (p->type == BoxPack_SPACING) {
			box->children_min += p->obj.spacing;
		}
		else if (WIDGET_STATUS_VISIBLE(p->wid)) {
			if (p->wid->max_h == 0)
				box->expand_min += p->wid->min_h;

			if (wid->min_w < p->wid->min_w)
				wid->min_w = p->wid->min_w;

			box->children_min += p->wid->min_h;
		}
		p = p->next;
	}

	switch (box->layout) {
		case BoxLayout_SPREAD:
		case BoxLayout_START_SPACING:
		case BoxLayout_END_SPACING:
		case BoxLayout_CENTER:
			wid->min_h = box->spacing * box->child_num + box->spacing;
			break;
		default:
			wid->min_h = box->spacing * box->child_num - box->spacing;
	}

	wid->min_w += box->border_width * 2;
	wid->min_h += box->border_width * 2 + (eint)box->children_min;
}

static void vbox_set_min(eHandle hobj, eint w, eint h)
{
	__vbox_set_min(GUI_WIDGET_DATA(hobj), GUI_BOX_DATA(hobj));
}

static void __vbox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool fixed, ebool add)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiBox    *box = GUI_BOX_DATA(hobj);

	eHandle tmp;
	eint max_w, max_h;

	__vbox_set_min(wid, box);

	if (add || !fixed) {
		GuiWidget *cw;

		if (cobj)
			cw = GUI_WIDGET_DATA(cobj);
		else
			cw = NULL;

		max_h = get_expand_h(wid, box, cw, req_h);
		max_w = get_expand_w(wid, box, cw, req_w);
		tmp   = hobj;
	}
	else {
		tmp   = 0;
		max_w = 0;
		max_h = 0;
	}

	if (wid->parent)
		egui_request_layout(wid->parent, tmp, max_w, max_h, fixed, add);
	else {
		GalEventResize resize = {max_w, max_h};
		vbox_resize(hobj, wid, &resize);
	}
}

static void vbox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool fixed, ebool add)
{
	__vbox_request_layout(hobj, cobj, req_w, req_h, fixed, add);
}

static void vbox_add(eHandle pobj, eHandle cobj)
{
	egui_box_add(pobj, cobj);

	__vbox_request_layout(pobj, 0, 0, 0, efalse, etrue);
}

static void vbox_add_spacing(eHandle pobj, eint size)
{
	egui_box_add_spacing(pobj, size);

	__vbox_request_layout(pobj, 0, 0, 0, efalse, etrue);
}

eHandle egui_vbox_new(void)
{
	return e_object_new(GTYPE_VBOX);
}
