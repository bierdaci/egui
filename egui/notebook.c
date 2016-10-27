#include "gui.h"
#include "bin.h"
#include "box.h"
#include "event.h"
#include "notebook.h"

#define VARBOX_DATA(hobj)			((VarBox *)e_object_type_data((hobj), GTYPE_VARBOX))
#define PAGEBN_DATA(hobj)			((PageBn *)e_object_type_data((hobj), GTYPE_PAGEBN))

#define DIRECT_V		0
#define DIRECT_H		1

typedef struct _PageBn PageBn;
struct _PageBn {
	const echar *label;
	eint ow, oh;
	eHandle page;
};

typedef struct _VarBox VarBox;
struct _VarBox {
	eint pos;
};

static eGeneType GTYPE_VARBOX;
static eGeneType GTYPE_TABBAR;
static eGeneType GTYPE_PAGEBN;
static eGeneType GTYPE_PAGEBOX;

static eint notebook_init_data(eHandle, ePointer);
static void notebook_set_current_page(GuiNotebook *, eHandle);

static void varbox_init_orders(eGeneType, ePointer);
static void pagebn_init_orders(eGeneType, ePointer);
static void tabbar_init_orders(eGeneType, ePointer);
static void pagebox_init_orders(eGeneType, ePointer);

static void (*vbox_request_layout)(eHandle, eHandle, eint, eint, ebool, ebool);
static void (*hbox_request_layout)(eHandle, eHandle, eint, eint, ebool, ebool);
static eint (*vbox_keydown)(eHandle, GalEventKey *);
static eint (*hbox_keydown)(eHandle, GalEventKey *);
static eint (*hbox_resize)(eHandle, GuiWidget *, GalEventResize *);
static eint (*vbox_resize)(eHandle, GuiWidget *, GalEventResize *);
static void (*hbox_add)(eHandle, eHandle);
static void (*vbox_add)(eHandle, eHandle);
static eHandle (*vbox_next_child)(GuiBin *, eHandle, eint, eint);
static eHandle (*hbox_next_child)(GuiBin *, eHandle, eint, eint);

eGeneType egui_genetype_notebook(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0, NULL,
			sizeof(GuiNotebook),
			notebook_init_data,
			NULL, NULL,
		};

		eGeneInfo varinfo = {
			0, varbox_init_orders,
			sizeof(VarBox),
			NULL, NULL, NULL,
		};

		eGeneInfo tabinfo = {
			0, tabbar_init_orders,
			0, NULL, NULL, NULL,
		};

		eGeneInfo btninfo = {
			0,
			pagebn_init_orders,
			sizeof(PageBn),
			NULL, NULL, NULL,
		};

		eGeneInfo boxinfo = {
			0, pagebox_init_orders,
			0, NULL, NULL, NULL,
		};

		GTYPE_VARBOX  = e_register_genetype(&varinfo, GTYPE_BOX, NULL);
		GTYPE_TABBAR  = e_register_genetype(&tabinfo, GTYPE_VARBOX, NULL);
		GTYPE_PAGEBN  = e_register_genetype(&btninfo, GTYPE_WIDGET, GTYPE_EVENT, NULL);
		GTYPE_PAGEBOX = e_register_genetype(&boxinfo, GTYPE_BOX, NULL);

		gtype = e_register_genetype(&info, GTYPE_VARBOX, NULL);
	}
	return gtype;
}

static eint pagebn_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiNotebook *nb = wid->extra_data;
	PageBn *pbn = PAGEBN_DATA(hobj);
	GalRect rc  = {wid->offset_x, wid->offset_y, wid->rect.w, wid->rect.h};
	LayoutFlags flags = LF_VCenter;

	if (nb->curpage == hobj) {
		if (WIDGET_STATUS_FOCUS(wid)) {
			egal_set_foreground(wid->pb, 0x1c1b17);
			if (nb->direct == GUI_POS_TOP)
				egal_draw_rect(wid->drawable, wid->pb, rc.x + 1, rc.y + 2, rc.w - 4, rc.h - 14);
			else
				egal_draw_rect(wid->drawable, wid->pb, rc.x + 1, rc.y + 7, rc.w - 8, rc.h - 14);
		}
	}
	else {
		egal_set_foreground(wid->pb, 0x2c2b27);
		egal_fill_rect(wid->drawable, wid->pb, rc.x, rc.y, rc.w, rc.h);
		egal_set_foreground(wid->pb, 0x3c3b37);
		if (nb->direct == GUI_POS_TOP)
			egal_draw_line(wid->drawable, wid->pb, rc.x + rc.w, 0, rc.x + rc.w, rc.y + rc.h);
		else
			egal_draw_line(wid->drawable, wid->pb, 0, rc.y + rc.h, rc.x + rc.w, rc.y + rc.h);
	}

	if (nb->direct == GUI_POS_TOP && nb->curpage == hobj)
		flags  = LF_VTop;
	else if (nb->direct == GUI_POS_LEFT && nb->curpage != hobj) {
		rc.x += 5;
		rc.w -= 5;
	}

	egal_set_foreground(wid->pb, 0xffffff);
	egui_draw_strings(wid->drawable, wid->pb, 0, pbn->label, &rc, flags);

	return 0;
}

static eint pagebn_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiNotebook *nb = GUI_WIDGET_DATA(hobj)->extra_data;
	notebook_set_current_page(nb, hobj);
	return 0;
}

static eint pagebn_init(eHandle hobj, eValist vp)
{
	GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
	PageBn  *pagebn = PAGEBN_DATA(hobj);

	wid->extra_data = e_va_arg(vp, ePointer);
	pagebn->page    = e_va_arg(vp, eHandle);
	pagebn->label   = e_va_arg(vp, ePointer);

	egui_strings_extent(0, pagebn->label, &wid->min_w, &wid->min_h);
	pagebn->ow  = wid->min_w;
	pagebn->oh  = wid->min_h;
	wid->min_h += 10;
	wid->min_w += 5;
	wid->rect.w = wid->min_w;
	wid->rect.h = wid->min_h;
	widget_set_status(wid, GuiStatusVisible | GuiStatusTransparent | GuiStatusActive | GuiStatusMouse);
	return 0;
}

static void pagebn_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init        = pagebn_init;
	w->expose      = pagebn_expose;
	e->lbuttondown = pagebn_lbuttondown;
	e->focus_in    = (eint(*)(eHandle))egui_update;
	e->focus_out   = (eint(*)(eHandle))egui_update;
}

static eint pagebox_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	GuiNotebook *nb = wid->extra_data;

	wid->rect.w = resize->w;
	wid->rect.h = resize->h;

	if (nb->curpage) {
		PageBn *bn = PAGEBN_DATA(nb->curpage);
		GuiBox *box = GUI_BOX_DATA(hobj);
		GuiWidget *cw = GUI_WIDGET_DATA(bn->page);
		GuiWidgetOrders *ws = GUI_WIDGET_ORDERS(bn->page);
		if (ws->request_resize) {
			ws->request_resize(bn->page,
					resize->w - box->border_width * 2,
					resize->h - box->border_width * 2);
		}
		egui_move(bn->page,
				(resize->w - box->border_width * 2 - cw->rect.w) / 2 + box->border_width,
				(resize->h - box->border_width * 2 - cw->rect.h) / 2 + box->border_width);
	}
	return 0;
}

static void pagebox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool up, ebool a)
{
	GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
	GuiNotebook *nb = wid->extra_data;

	if (nb->curpage) {
		GalEventResize resize = {wid->rect.w, wid->rect.h};
		pagebox_resize(hobj, wid, &resize);
	}
}

static void pagebox_add(eHandle pobj, eHandle cobj)
{
	GuiBox    *box = GUI_BOX_DATA(pobj);
	GuiWidget *wid = GUI_WIDGET_DATA(pobj);
	GuiWidget *cw  = GUI_WIDGET_DATA(cobj);
	eint old_w = wid->rect.w;
	eint old_h = wid->rect.h;

	egui_box_add(pobj, cobj);

	if (wid->min_w < box->border_width * 2 + cw->min_w)
		wid->min_w = box->border_width * 2 + cw->min_w;
	if (wid->min_h < box->border_width * 2 + cw->min_h)
		wid->min_h = box->border_width * 2 + cw->min_h;
	if (wid->rect.w < box->border_width * 2 + cw->rect.w)
		wid->rect.w = box->border_width * 2 + cw->rect.w;
	if (wid->rect.h < box->border_width * 2 + cw->rect.h)
		wid->rect.h = box->border_width * 2 + cw->rect.h;

	if (old_w != wid->rect.w || old_h != wid->rect.h)
		egui_request_layout(wid->parent, cobj, wid->rect.w, wid->rect.h, efalse, etrue);
}

static void pagebox_init_orders(eGeneType new, ePointer this)
{
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	b->request_layout = pagebox_request_layout;
	w->add      = pagebox_add;
	w->resize   = pagebox_resize;
}

static eint tabbar_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiNotebook *nb = GUI_WIDGET_DATA(hobj)->extra_data;
	GuiWidget   *cw = GUI_WIDGET_DATA(nb->curpage);
	eint pos = 0;

	if (ent->code == GAL_KC_Tab)
		return 0;

#define PAGE_PREV  1
#define PAGE_NEXT  2
	if (nb->direct == GUI_POS_TOP || nb->direct == GUI_POS_BOTTOM) {
		if (ent->code == GAL_KC_Left)
			pos = PAGE_PREV;
		else if (ent->code == GAL_KC_Right)
			pos = PAGE_NEXT;
	}
	else if (nb->direct == GUI_POS_LEFT || nb->direct == GUI_POS_RIGHT) {
		if (ent->code == GAL_KC_Up)
			pos = PAGE_PREV;
		else if (ent->code == GAL_KC_Down)
			pos = PAGE_NEXT;
	}

	if (pos == PAGE_PREV) {
		if (cw->prev == NULL)
			cw = GUI_BIN_DATA(hobj)->tail;
		else
			cw = cw->prev;
	}
	else if (pos == PAGE_NEXT) {
		if (cw->next == NULL)
			cw = GUI_BIN_DATA(hobj)->head;
		else
			cw = cw->next;
	}

#undef PAGE_PREV
#undef PAGE_NEXT

	if (pos && OBJECT_OFFSET(cw) != hobj) {
		egui_set_focus(OBJECT_OFFSET(cw));
		notebook_set_current_page(nb, OBJECT_OFFSET(cw));
		return -1;
	}

	if (nb->direct == GUI_POS_RIGHT || nb->direct == GUI_POS_LEFT)
		return vbox_keydown(hobj, ent);
	else
		return hbox_keydown(hobj, ent);
}

static eHandle tabbar_next_child(GuiBin *bin, eHandle child, eint dir, eint axis)
{
	GuiNotebook *nb = GUI_WIDGET_DATA(OBJECT_OFFSET(bin))->extra_data;
	if (nb->curpage)
		return nb->curpage;
	return 0;
}

static void tabbar_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders   *b = e_genetype_orders(new, GTYPE_BIN);

	e->keydown    = tabbar_keydown;
	b->next_child = tabbar_next_child;
}

static eint varbox_keydown(eHandle hobj, GalEventKey *key)
{
	VarBox *vb = VARBOX_DATA(hobj);
	if (vb->pos == DIRECT_V)
		return vbox_keydown(hobj, key);
	else
		return hbox_keydown(hobj, key);
}

static eint varbox_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	VarBox *vb = VARBOX_DATA(hobj);
	if (vb->pos == DIRECT_V)
		return vbox_resize(hobj, wid, resize);
	else
		return hbox_resize(hobj, wid, resize);
}

static eHandle varbox_next_child(GuiBin *bin, eHandle cobj, eint dir, eint axis)
{
	VarBox *vb = VARBOX_DATA(OBJECT_OFFSET(bin));
	if (vb->pos == DIRECT_V)
		return vbox_next_child(bin, cobj, dir, axis);
	else
		return hbox_next_child(bin, cobj, dir, axis);
}

static void varbox_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, ebool up, ebool a)
{
	VarBox *vb = VARBOX_DATA(hobj);
	if (vb->pos == DIRECT_V)
		vbox_request_layout(hobj, 0, req_w, req_h, efalse, a);
	else
		hbox_request_layout(hobj, 0, req_w, req_h, efalse, a);
}

static void varbox_add(eHandle pobj, eHandle cobj)
{
	VarBox *vb = VARBOX_DATA(pobj);
	if (vb->pos == DIRECT_V)
		vbox_add(pobj, cobj);
	else
		hbox_add(pobj, cobj);
}

static void varbox_init_orders(eGeneType new, ePointer this)
{
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);

	GuiBinOrders    *vb = e_genetype_orders(GTYPE_VBOX, GTYPE_BIN);
	GuiBinOrders    *hb = e_genetype_orders(GTYPE_HBOX, GTYPE_BIN);
	GuiEventOrders  *ve = e_genetype_orders(GTYPE_VBOX, GTYPE_EVENT);
	GuiEventOrders  *he = e_genetype_orders(GTYPE_HBOX, GTYPE_EVENT);
	GuiWidgetOrders *vw = e_genetype_orders(GTYPE_VBOX, GTYPE_WIDGET);
	GuiWidgetOrders *hw = e_genetype_orders(GTYPE_HBOX, GTYPE_WIDGET);

	vbox_add        = vw->add;
	vbox_resize     = vw->resize;
	vbox_keydown    = ve->keydown;
	vbox_next_child = vb->next_child;
	vbox_request_layout = vb->request_layout;

	hbox_add        = hw->add;
	hbox_resize     = hw->resize;
	hbox_keydown    = he->keydown;
	hbox_next_child = hb->next_child;
	hbox_request_layout = hb->request_layout;

	w->add          = varbox_add;
	w->resize       = varbox_resize;
	e->keydown      = varbox_keydown;
	b->next_child   = varbox_next_child;
	b->request_layout = varbox_request_layout;
}

static void notebook_set_current_page(GuiNotebook *nb, eHandle bn)
{
	PageBn *pagebn = PAGEBN_DATA(bn);

	if (nb->curpage && nb->curpage != bn) {
		eHandle o = nb->curpage;
		nb->curpage = 0;
		egui_hide(PAGEBN_DATA(o)->page, etrue);
		egui_update(o);
	}

	nb->curpage = bn;
	egui_show(pagebn->page, etrue);
	egui_update(bn);
	egui_update(nb->pagebox);
}

static eint box_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	egal_set_foreground(wid->pb, 0x3c3b37);
	egal_fill_rect(wid->drawable, wid->pb, wid->offset_x, wid->offset_y, wid->rect.w, wid->rect.h);
	return 0;
}

static eint notebook_init_data(eHandle hobj, ePointer this)
{
	GuiNotebook *nb = this;

	nb->direct = GUI_POS_TOP;
	nb->tabbar = e_object_new(GTYPE_TABBAR);
	GUI_WIDGET_DATA(nb->tabbar)->extra_data = nb;
	VARBOX_DATA(nb->tabbar)->pos = DIRECT_H;

	nb->pagebox = e_object_new(GTYPE_PAGEBOX);
	GUI_WIDGET_DATA(nb->pagebox)->extra_data = nb;
	e_signal_connect(nb->pagebox, SIG_EXPOSE_BG, box_expose_bg);

	egui_set_expand(hobj, etrue);
	egui_box_set_align(hobj, BoxAlignStart);
	egui_box_set_layout(hobj, BoxLayout_START);
	egui_set_expand(nb->pagebox, etrue);
	egui_box_set_border_width(nb->pagebox, 3);

	egui_add(hobj, nb->tabbar);
	egui_add(hobj, nb->pagebox);

	return 0;
}

eHandle egui_notebook_new(void)
{
	return e_object_new(GTYPE_NOTEBOOK);
}

eint egui_notebook_append_page(eHandle hobj, eHandle page, const echar *label)
{
	GuiNotebook *nb = GUI_NOTEBOOK_DATA(hobj);
	eHandle pbn_obj = e_object_new(GTYPE_PAGEBN, nb, page, label);

	egui_add(nb->tabbar,  pbn_obj);
	egui_add(nb->pagebox, page);

	if (!nb->curpage) {
		nb->curpage = pbn_obj;
		egui_show(page, etrue);
	}
	else egui_hide(page, etrue);

	return 0;
}

void egui_notebook_set_current_page(eHandle hobj, eint num)
{
	GuiNotebook *nb = GUI_NOTEBOOK_DATA(hobj);
	GuiWidget  *wid = GUI_BIN_DATA(nb->tabbar)->head;

	eint i = 0;
	while (wid && i < num) {
		wid = wid->next;
		i++;
	}
	if (!wid || i != num)
		return;

	notebook_set_current_page(nb, OBJECT_OFFSET(wid));
}

void egui_notebook_set_pos(eHandle hobj, GuiPositionType pos)
{
	GuiNotebook *nb = GUI_NOTEBOOK_DATA(hobj);

	if (nb->direct != pos) {
		GuiWidget *tail = GUI_BIN_DATA(nb->tabbar)->tail;
		GuiWidget *head = NULL;

		while (tail) {
			GuiWidget *t = tail;
			tail = t->prev;
			egui_remove(OBJECT_OFFSET(t), efalse);
			t->next = head;
			head = t;
		}

		if (pos == GUI_POS_LEFT || pos == GUI_POS_RIGHT) {
			VARBOX_DATA(hobj)->pos = DIRECT_H;
			VARBOX_DATA(nb->tabbar)->pos = DIRECT_V;
		}
		else {
			VARBOX_DATA(hobj)->pos = DIRECT_V;
			VARBOX_DATA(nb->tabbar)->pos = DIRECT_H;
		}

		while (head) {
			GuiWidget   *t = head;
			PageBn *pagebn = PAGEBN_DATA(OBJECT_OFFSET(t));

			head = head->next;

			if (pos == GUI_POS_LEFT || pos == GUI_POS_RIGHT) {
				t->rect.w = pagebn->ow + 10;
				t->min_w  = pagebn->ow + 10;
			}
			else {
				t->rect.w = pagebn->ow + 5;
				t->min_w  = pagebn->ow + 5;
			}

			egui_add(nb->tabbar, OBJECT_OFFSET(t));
		}

		nb->direct = pos;
		egui_update(hobj);
	}
}

