#include "gui.h"
#include "bin.h"
#include "box.h"
#include "event.h"
#include "label.h"
#include "scrollwin.h"
#include "menu.h"
#include "res.h"

#define GTYPE_MENU_SHELL				(egui_genetype_menu_shell())
#define GTYPE_MENU_V        			(egui_genetype_menu_v())

#define GUI_MENU_BAR_DATA(hobj)		    ((GuiMenuBar *)e_object_type_data((hobj), GTYPE_MENU_BAR))
#define GUI_MENU_SHELL_DATA(hobj)		((GuiMenuShell  *)e_object_type_data((hobj), GTYPE_MENU_SHELL))
#define GUI_MENU_BUTTON_DATA(hobj)		((GuiMenuButton *)e_object_type_data((hobj), GTYPE_MENU_BUTTON))

#define MENU_BN_SIZE					15
#define MENU_BRINK_SIZE					20

#define BN_UP		1
#define BN_DOWN		2
#define BN_LEFT		3
#define BN_RIGHT	4

static eGeneType GTYPE_MENU_VBOX;
static eGeneType GTYPE_MENU_HBOX;
static eGeneType GTYPE_MENU_BUTTON;
static eGeneType GTYPE_MENU_SEPARATOR;
static eGeneType GTYPE_MENU_SCROLLWIN;
static eGeneType GTYPE_MENU_BAR_SCROLLWIN;

static GuiResItem *menu_res      = NULL;
static GalImage   *menu_bn_up    = NULL;
static GalImage   *menu_bn_down  = NULL;
static GalImage   *menu_bn_right = NULL;
static GalImage   *menu_bn_check = NULL;
static GalImage   *menu_bn_radio = NULL;

static void menu_scrwin_init_orders(eGeneType, ePointer);
static void bar_scrwin_init_orders(eGeneType, ePointer);
static void menu_v_init_orders(eGeneType new, ePointer this);
static void menu_bar_init_orders(eGeneType new, ePointer this);
static eint menu_bar_init_data(eHandle hobj, ePointer this);
static void menu_shell_init_orders(eGeneType, ePointer);
static void menu_bn_init_orders(eGeneType, ePointer);
static void menu_vbox_init_orders(eGeneType new, ePointer);
static void menu_hbox_init_orders(eGeneType new, ePointer);

static void (*vbox_add)(eHandle, eHandle);
static void (*hbox_add)(eHandle, eHandle);
static void (*vbox_request_layout)(eHandle, eHandle, eint, eint, bool, bool);
void egui_menu_append(eHandle hobj, eHandle item);

static void separator_init_orders(eGeneType, ePointer);

typedef struct {
	eint type;
	bool enter;
	bool active;
	eTimer timer;
} GuiMenuButton;

typedef struct {
	bool active;
} GuiMenuBar;

eGeneType egui_genetype_menu_shell(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			menu_shell_init_orders,
			sizeof(GuiMenuShell),
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_menu(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0, NULL,
			sizeof(GuiMenu),
			NULL, NULL, NULL,
		};

		eGeneInfo sepinfo = {
			0, separator_init_orders,
			0, NULL, NULL, NULL,
		};

		eGeneInfo scrinfo = {
			0,
			menu_scrwin_init_orders,
			0, NULL, NULL, NULL,
		};

		eGeneInfo bninfo = {
			0,
			menu_bn_init_orders,
			sizeof(GuiMenuButton), 
			NULL, NULL, NULL,
		};

		eGeneInfo vboxinfo = {
			0, menu_vbox_init_orders,
			0, NULL, NULL, NULL,
		};

		eGeneInfo hboxinfo = {
			0,
			menu_hbox_init_orders,
			0, 
			NULL, NULL, NULL,
		};

		GTYPE_MENU_VBOX      = e_register_genetype(&vboxinfo, GTYPE_VBOX, NULL);
		GTYPE_MENU_HBOX      = e_register_genetype(&hboxinfo, GTYPE_HBOX, NULL);
		GTYPE_MENU_BUTTON    = e_register_genetype(&bninfo, GTYPE_WIDGET, GTYPE_EVENT, NULL);
		GTYPE_MENU_SEPARATOR = e_register_genetype(&sepinfo, GTYPE_WIDGET, NULL);
		GTYPE_MENU_SCROLLWIN = e_register_genetype(&scrinfo, GTYPE_SCROLLWIN, NULL);

		menu_res      = egui_res_find(GUI_RES_HANDLE, _("menu")); 
		menu_bn_up    = egui_res_find_item(menu_res,  _("up"));
		menu_bn_down  = egui_res_find_item(menu_res,  _("down"));
		menu_bn_right = egui_res_find_item(menu_res,  _("right"));
		menu_bn_check = egui_res_find_item(menu_res,  _("check"));
		menu_bn_radio = egui_res_find_item(menu_res,  _("radio"));

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_menu_v(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			menu_v_init_orders,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_MENU, GTYPE_VBOX, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_menu_bar(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			menu_bar_init_orders,
			sizeof(GuiMenuBar),
			menu_bar_init_data,
			NULL, NULL,
		};

		eGeneInfo binfo = {
			0, bar_scrwin_init_orders,
			0, NULL, NULL, NULL,
		};

		GTYPE_MENU_BAR_SCROLLWIN = e_register_genetype(&binfo, GTYPE_SCROLLWIN, NULL);

		gtype = e_register_genetype(&info, GTYPE_MENU, NULL);
	}
	return gtype;
}

static eHandle item_to_menu(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	while (wid->parent) {
		hobj = wid->parent;
		if (e_object_type_check(hobj, GTYPE_MENU))
			break;
		wid  = GUI_WIDGET_DATA(hobj);
	}
	return hobj;
}

static void close_popup_menu(GuiMenu *menu)
{
	eHandle hobj = OBJECT_OFFSET(menu);
	GuiMenuShell *shell = GUI_MENU_SHELL_DATA(menu->shell);

	eHandle super = menu->super;
	if (super && shell->active == hobj)
		shell->active = super;

	egui_hide(hobj, true);

	while (menu->popup) {
		hobj = menu->popup;
		menu->popup = 0;

		if (shell->active == hobj && super)
			shell->active = super;

		egui_hide(hobj, true);
		menu = GUI_MENU_DATA(hobj);
	}
}

static void leave_signal_emit(eHandle hobj)
{
	e_signal_emit(hobj, SIG_LEAVE);

	if (GUI_TYPE_BIN(hobj)) {
		GuiBin *bin = GUI_BIN_DATA(hobj);
		if (bin->enter) {
			leave_signal_emit(bin->enter);
			bin->enter = 0;
		}
	}
}

static void menu_shell_realize(eHandle hobj, GuiWidget *wid)
{
	GalWindowAttr attr = {0};

	attr.type    = GalWindowTemp;
	attr.x       = 0;
	attr.y       = 0;
	attr.width   = 4000;
	attr.height  = 4000;
	attr.wclass  = GAL_INPUT_ONLY;
	attr.wa_mask = GAL_WA_NOREDIR;
	attr.input_event = true;
	wid->window  = egal_window_new(&attr);
	wid->drawable = wid->window;
	egal_window_set_attachment(wid->drawable, hobj);
}

static void menu_unactive(GuiMenuShell *shell, GalEventMouse *mevent)
{
	GalRect    rc;
	GuiWidget *wid = NULL;

	if (e_object_type_check(shell->root_menu, GTYPE_MENU_BAR)) {
		eHandle focus;

		wid = GUI_WIDGET_DATA(shell->root_menu);
		while (wid->parent) {
			GUI_BIN_DATA(wid->parent)->grab = 0;
			wid = GUI_WIDGET_DATA(wid->parent);
		}
		focus = GUI_BIN_DATA(shell->root_menu)->focus;
		if (focus) {
			egui_unset_focus(focus);
			egui_update(focus);
		}
	}
	else if (shell->source) {
		wid = GUI_WIDGET_DATA(shell->source);
		shell->source = 0;
	}

	if (wid) {
		rc.w = wid->rect.w;
		rc.h = wid->rect.h;
		egal_window_get_origin(wid->drawable, &rc.x, &rc.y);
		if (egal_rect_point_in(&rc, mevent->point.x, mevent->point.y)) {
			mevent->point.x -= rc.x;
			mevent->point.y -= rc.y;
			e_signal_emit(OBJECT_OFFSET(wid), SIG_MOUSEMOVE, mevent);
		}
		else
			leave_signal_emit(OBJECT_OFFSET(wid));
	}
}

static eint menu_shell_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuShell *shell = GUI_MENU_SHELL_DATA(hobj);
	eHandle       popup = shell->root_menu;
	GalEventMouse me = *mevent;

	while (popup) {
		GuiWidget *wid = GUI_WIDGET_DATA(popup);
		GalRect   rect;
		if (wid->parent)
			egal_window_get_origin(wid->drawable, &rect.x, &rect.y);
		else {
			rect.x = wid->rect.x;
			rect.y = wid->rect.y;
		}
		rect.w = wid->rect.w;
		rect.h = wid->rect.h;
		if (egal_rect_point_in(&rect, mevent->point.x, mevent->point.y)) {
			mevent->point.x -= rect.x;
			mevent->point.y -= rect.y;
			return e_signal_emit(popup, SIG_LBUTTONDOWN, mevent);
		}
		popup = GUI_MENU_DATA(popup)->popup;
	}

	egui_hide(hobj, true);

	shell->source = 0;
	menu_unactive(shell, &me);

	return 0;
}

static eint menu_shell_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuShell *shell = GUI_MENU_SHELL_DATA(hobj);
	eHandle menu = shell->root_menu;

	while (menu) {
		GuiWidget *wid = GUI_WIDGET_DATA(menu);
		GalRect   rect;
		if (wid->parent)
			egal_window_get_origin(wid->drawable, &rect.x, &rect.y);
		else {
			rect.x = wid->rect.x;
			rect.y = wid->rect.y;
		}
		rect.w = wid->rect.w;
		rect.h = wid->rect.h;
		if (egal_rect_point_in(&rect, mevent->point.x, mevent->point.y)) {
			mevent->point.x -= rect.x;
			mevent->point.y -= rect.y;
			return e_signal_emit(menu, SIG_LBUTTONUP, mevent);
		}
		menu = GUI_MENU_DATA(menu)->popup;
	}
	return 0;
}

static eint menu_shell_mousemove(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuShell *shell = GUI_MENU_SHELL_DATA(hobj);
	eHandle menu = shell->root_menu;

	while (menu) {
		GuiWidget *wid = GUI_WIDGET_DATA(menu);
		GalRect   rect;
		if (wid->parent)
			egal_window_get_origin(wid->drawable, &rect.x, &rect.y);
		else {
			rect.x = wid->rect.x;
			rect.y = wid->rect.y;
		}
		rect.w = wid->rect.w;
		rect.h = wid->rect.h;
		if (egal_rect_point_in(&rect, mevent->point.x, mevent->point.y)) {
			if (shell->enter && shell->enter != menu) {
				leave_signal_emit(shell->enter);
				shell->enter = 0;
			}
			if (shell->enter == 0) {
				e_signal_emit(menu, SIG_ENTER);
				shell->enter = menu;
			}
			mevent->point.x -= rect.x;
			mevent->point.y -= rect.y;
			e_signal_emit(menu, SIG_MOUSEMOVE, mevent);
			return 0;
		}

		menu = GUI_MENU_DATA(menu)->popup;
	}

	if (shell->enter) {
		leave_signal_emit(shell->enter);
		shell->enter = 0;
	}

	return 0;
}

static bool can_snatch_control(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);
	GuiBin  *bin  = GUI_BIN_DATA(menu->box);

	if (bin->focus) {
		GuiMenuItem *item = GUI_MENU_ITEM_DATA(bin->focus);
		if (item->type == MenuItemBranch && item->p.submenu)
			return false;
	}

	return true;
}

static eint menu_shell_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiMenuShell *shell = GUI_MENU_SHELL_DATA(hobj);

	if (ent->code == GAL_KC_Tab)
		return -1;

	if (ent->code == GAL_KC_Escape) {
		GuiWidget   *wid = NULL;
		GalEventMouse me;

		egui_hide(hobj, true);

		if (e_object_type_check(shell->root_menu, GTYPE_MENU_BAR)) {
			wid = GUI_WIDGET_DATA(shell->root_menu);
			while (wid->parent)
				wid = GUI_WIDGET_DATA(wid->parent);
		}
		else if (shell->source)
			wid = GUI_WIDGET_DATA(shell->source);

		if (wid) {
			egal_get_pointer(wid->drawable, &me.point.x, &me.point.y, NULL, NULL, NULL);
			menu_unactive(shell, &me);
		}
	}
	else {
		GuiBin *bin;

		if (!shell->active)
			shell->active = shell->root_menu;

		if (e_object_type_check(shell->root_menu, GTYPE_MENU_BAR)) {
			if (ent->code == GAL_KC_Left) {
				if (shell->active == shell->root_menu ||
						GUI_MENU_DATA(shell->active)->super == shell->root_menu) {
					e_signal_emit(shell->root_menu, SIG_KEYDOWN, ent);
					return 0;
				}
			}
			else if (ent->code == GAL_KC_Right
					&& can_snatch_control(shell->active)) {
				shell->active = shell->root_menu;
				e_signal_emit(shell->root_menu, SIG_KEYDOWN, ent);
				return 0;
			}
		}

		bin = GUI_BIN_DATA(GUI_MENU_DATA(shell->active)->box);
		if (bin->focus
				|| ent->code == GAL_KC_Up
				|| ent->code == GAL_KC_Down)
			e_signal_emit(shell->active, SIG_KEYDOWN, ent);
	}

	return 0;
}

static void menu_shell_show(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	egal_window_show(wid->drawable);
	egal_grab_keyboard(wid->drawable, false);
	egal_grab_pointer(wid->drawable, false, 0);
}

static void menu_shell_hide(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	GuiMenuShell *shell;
	GuiMenu *menu;

	egal_ungrab_keyboard(wid->drawable);
	egal_ungrab_pointer(wid->drawable);
	egal_window_hide(wid->drawable);

	shell = GUI_MENU_SHELL_DATA(hobj);
	menu  = GUI_MENU_DATA(shell->root_menu);

	if (menu->is_bar) {
		GUI_MENU_BAR_DATA(shell->root_menu)->active = false;
		if (menu->popup)
			close_popup_menu(GUI_MENU_DATA(menu->popup));
		menu->popup = 0;
	}
	else close_popup_menu(menu);
}

static void menu_shell_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	e->keydown      = menu_shell_keydown;
	e->mousemove    = menu_shell_mousemove;
	e->lbuttonup    = menu_shell_lbuttonup;
	e->lbuttondown  = menu_shell_lbuttondown;
	e->rbuttondown  = menu_shell_lbuttondown;
	w->realize      = menu_shell_realize;
	w->show         = menu_shell_show;
	w->hide         = menu_shell_hide;
}

static void menu_bn_request_resize(eHandle hobj, eint w, eint h)
{
	GuiWidget *widget = GUI_WIDGET_DATA(hobj);
	GuiMenuButton *bn = GUI_MENU_BUTTON_DATA(hobj);
	if (bn->type == BN_UP || bn->type == BN_DOWN) {
		widget->rect.w = w;
		widget->rect.h = MENU_BN_SIZE;
	}
	else {
		widget->rect.w = MENU_BN_SIZE;
		widget->rect.h = w;
	}
	egal_window_resize(widget->drawable, widget->rect.w, widget->rect.h);
}

static eint menu_bn1_timer_cb(eTimer timer, euint num, ePointer args)
{
	GuiMenu      *menu = GUI_MENU_DATA((eHandle)args);
	GuiScrollWin *scr  = GUI_SCROLLWIN_DATA(menu->scrwin);

	if (scr->offset_y + MENU_BN_SIZE < 0) {
		scr->offset_y += MENU_BN_SIZE;
		GUI_MENU_BUTTON_DATA(menu->bn2)->active = true;
	}
	else {
		GUI_MENU_BUTTON_DATA(menu->bn1)->active = false;
		egui_update(menu->bn1);
		scr->offset_y = 0;
	}

	if (menu->old_offset_y != scr->offset_y) {
		egui_move(menu->box, 0, scr->offset_y);
		egui_update(menu->box);
		egui_update(menu->bn1);
		egui_update(menu->bn2);
		menu->old_offset_y = scr->offset_y;
		return 0;
	}

	return -1;
}

static eint menu_bn2_timer_cb(eTimer timer, euint num, ePointer args)
{
	GuiMenu      *menu = GUI_MENU_DATA((eHandle)args);
	GalRect      *rect = &GUI_WIDGET_DATA(menu->scrwin)->rect;
	GuiScrollWin *swin = GUI_SCROLLWIN_DATA(menu->scrwin);

	eint vh = GUI_WIDGET_DATA(menu->box)->rect.h;

	if (vh + (swin->offset_y - MENU_BN_SIZE) > rect->h) {
		GUI_MENU_BUTTON_DATA(menu->bn1)->active = true;
		swin->offset_y -= MENU_BN_SIZE;
	}
	else {
		swin->offset_y = -(vh - rect->h);
		GUI_MENU_BUTTON_DATA(menu->bn2)->active = false;
	}

	if (menu->old_offset_y != swin->offset_y) {
		menu->old_offset_y  = swin->offset_y;
		egui_move(menu->box, 0, swin->offset_y);
		egui_update(menu->box);
		egui_update(menu->bn1);
		egui_update(menu->bn2);
		return 0;
	}

	return -1;
}

static eint menu_bn_enter(eHandle hobj, eint x, eint y)
{
	GuiMenu       *menu = GUI_MENU_DATA(item_to_menu(hobj));
	GuiBin        *bin  = GUI_BIN_DATA(menu->box);
	GuiMenuButton *bn   = GUI_MENU_BUTTON_DATA(hobj);

	if (bn->type == BN_LEFT || bn->type == BN_RIGHT)
		return 0;

	bn->enter = true;
	egui_update(hobj);

	if (bn->active && bin->focus)
		egui_unset_focus(bin->focus);

	if (bn->active && menu->popup) {
		close_popup_menu(GUI_MENU_DATA(menu->popup));
		menu->popup = 0;
	}

	if (bn->type == BN_UP)
		bn->timer = e_timer_add(100, menu_bn1_timer_cb,
				(ePointer)GUI_WIDGET_DATA(hobj)->parent);
	else
		bn->timer = e_timer_add(100, menu_bn2_timer_cb,
				(ePointer)GUI_WIDGET_DATA(hobj)->parent);
	return 0;
}

static eint menu_bn_leave(eHandle hobj)
{
	GuiMenuButton *bn = GUI_MENU_BUTTON_DATA(hobj);

	if (bn->type == BN_LEFT || bn->type == BN_RIGHT)
		return 0;

	bn->enter = false;
	egui_update(hobj);
	if (bn->timer) {
		e_timer_del(bn->timer);
		bn->timer = 0;
	}

	return 0;
}

static eint menu_bn_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiMenuButton *bn = GUI_MENU_BUTTON_DATA(hobj);

	if (bn->type == BN_UP || bn->type == BN_DOWN) {
		if (bn->active) {
			if (bn->enter)
				egal_set_foreground(exp->pb, 0xb4a590);
			else
				egal_set_foreground(exp->pb, 0x3c3b37);
		}
		else
			egal_set_foreground(exp->pb, 0x3c3b37);

		egal_fill_rect(wid->drawable, exp->pb, 0, 0, wid->rect.w, MENU_BN_SIZE);
		egal_set_foreground(exp->pb, 0x282825);
		if (bn->type == BN_DOWN) {
			egal_draw_line(wid->drawable, exp->pb, 2, 0, wid->rect.w-4, 0);
			egal_composite_image(wid->drawable, exp->pb,
					(wid->rect.w - menu_bn_down->h) / 2, 
					4, menu_bn_down, 0, 0, menu_bn_down->w, menu_bn_down->h);
		}
		else {
			egal_draw_line(wid->drawable, exp->pb, 2, wid->rect.h-1, wid->rect.w-4, wid->rect.h-1);
			egal_composite_image(wid->drawable, exp->pb,
					(wid->rect.w - menu_bn_up->h) / 2, 
					wid->rect.h - menu_bn_up->h - 4,
					menu_bn_up, 0, 0, menu_bn_up->w, menu_bn_up->h);
		}
	}
	return 0;
}

static eint menu_bn_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->min_w  = 1;
	wid->min_h  = MENU_BN_SIZE;
	wid->max_w  = 0;
	wid->max_h  = 1;
	wid->rect.w = 1;
	wid->rect.h = MENU_BN_SIZE;
	widget_set_status(wid, GuiStatusMouse);

	GUI_MENU_BUTTON_DATA(hobj)->type = e_va_arg(vp, eint);

	return e_signal_emit(hobj, SIG_REALIZE);
}

static void menu_bn_init_orders(eGeneType new, ePointer this)
{
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);

	c->init   = menu_bn_init;
	e->enter  = menu_bn_enter;
	e->leave  = menu_bn_leave;
	w->expose = menu_bn_expose;
}

static eHandle menu_point_in(GuiBin *bin, eint x, eint y)
{
	GuiWidget *cw;

	for (cw = bin->head; cw; cw = cw->next) {
		if (!WIDGET_STATUS_VISIBLE(cw))
			continue;

		if (egal_rect_point_in(&cw->rect, x, y)) {
			if (WIDGET_STATUS_MOUSE(cw))
				return OBJECT_OFFSET(cw);
			break;
		}
	}
	return 0;
}

static eint mouse_signal_emit(eHandle hobj, esig_t sig, int x, int y)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	if (GUI_TYPE_BIN(hobj) || WIDGET_STATUS_MOUSE(wid)) {
		GalEventMouse mevent;
		mevent.point.x = x - wid->rect.x;
		mevent.point.y = y - wid->rect.y;
		return e_signal_emit(hobj, sig, &mevent);
	}
	return 0;
}

static eint menu_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	eint x = mevent->point.x;
	eint y = mevent->point.y;
	eHandle grab = menu_point_in(bin, x, y);
	if (grab) {
		mouse_signal_emit(grab, SIG_LBUTTONDOWN, x, y);
		if (GUI_STATUS_ACTIVE(grab))
			egui_set_focus(grab);
	}
	return 0;
}

static eint menu_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);

	if (e_signal_emit(menu->scrwin, SIG_KEYDOWN, ent) < 0)
		return -1;

	switch (ent->code) {
		case GAL_KC_Down:
			return bin_next_focus(menu->scrwin, 0);
		case GAL_KC_Up:
			return bin_prev_focus(menu->scrwin, 0);
		default:
			break;
	}

	return 0;
}

static void menu_scrwin_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool up, bool a)
{
	GalEventResize res = {req_w, req_h};
	e_signal_emit(cobj, SIG_RESIZE, &res);
}

static void menu_scrwin_add(eHandle hobj, eHandle cobj)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);

	if (!bin->head) {
		GuiWidget *cw = GUI_WIDGET_DATA(cobj);
		cw->parent = hobj;
		cw->rect.x = 0;
		cw->rect.y = 0;

		STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, cw, prev, next);

		GUI_WIDGET_ORDERS(hobj)->put(hobj, cobj);
	}
}

static void menu_scrwin_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);

	w->add            = menu_scrwin_add;
	e->lbuttondown    = menu_lbuttondown;
	b->request_layout = menu_scrwin_request_layout;
}

static void menu_vbox_move(eHandle hobj)
{
	GuiBin    *bin = GUI_BIN_DATA(hobj);
	GuiScrollWin *scr = GUI_SCROLLWIN_DATA(GUI_WIDGET_DATA(hobj)->parent);
	GuiWidget *cw  = bin->head;

	while (cw) {
		cw->offset_y = cw->rect.y + scr->offset_y;
		cw = cw->next;
	}
}

static void menu_hbox_move(eHandle hobj)
{
	GuiBin    *bin = GUI_BIN_DATA(hobj);
	GuiScrollWin *scr = GUI_SCROLLWIN_DATA(GUI_WIDGET_DATA(hobj)->parent);
	GuiWidget *cw  = bin->head;

	while (cw) {
		cw->offset_x = cw->rect.x + scr->offset_x;
		cw = cw->next;
	}
}

static eint up_skip_h(GuiBin *bin, GuiWidget *wid)
{
	GuiWidget *cw = wid->prev;
	eint        h = 0;
	while (cw) {
		if (WIDGET_STATUS_ACTIVE(cw))
			break;
		h += cw->rect.h;
		cw = cw->prev;
	}
	return h;
}

static eint down_skip_h(GuiBin *bin, GuiWidget *wid)
{
	GuiWidget *cw = wid->next;
	eint        h = 0;
	while (cw) {
		if (WIDGET_STATUS_ACTIVE(cw))
			break;
		h += cw->rect.h;
		cw = cw->next;
	}
	return h;
}

static eint menu_vbox_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiBin       *bin  = GUI_BIN_DATA(hobj);
	GuiMenu      *menu = GUI_MENU_DATA(item_to_menu(hobj));
	GuiWidget    *vw   = GUI_WIDGET_DATA(menu->box);
	GuiWidget    *sw   = GUI_WIDGET_DATA(menu->scrwin);
	GuiScrollWin *scr  = GUI_SCROLLWIN_DATA(menu->scrwin);

	if (vw->rect.h > sw->rect.h && !bin->focus) {
		if (ent->code == GAL_KC_Up)
			scr->offset_y  = sw->rect.h - vw->rect.h;
		else if (ent->code == GAL_KC_Down)
			scr->offset_y  = 0;
	}

	if (vw->rect.h > sw->rect.h && bin->focus) {
		GuiWidget *iw = GUI_WIDGET_DATA(bin->focus);
		eint iy = iw->rect.y;
		eint ih = iw->rect.h;
		if (ent->code == GAL_KC_Up) {
			eint skip_h = up_skip_h(bin, iw);
			if (iy - skip_h == 0) {
				scr->offset_y = sw->rect.h - vw->rect.h;
				GUI_MENU_BUTTON_DATA(menu->bn1)->active = true;
				GUI_MENU_BUTTON_DATA(menu->bn2)->active = false;
				egui_update(menu->bn1);
				egui_update(menu->bn2);
			}
			else if (iy - skip_h - ih + scr->offset_y < 0) {
				GuiMenuButton *bn = GUI_MENU_BUTTON_DATA(menu->bn2);
				scr->offset_y -= iy - skip_h - ih + scr->offset_y;
				if (!bn->active) {
					bn->active = true;
					egui_update(menu->bn2);
				}
				if (scr->offset_y == 0) {
					GUI_MENU_BUTTON_DATA(menu->bn1)->active = false;
					egui_update(menu->bn1);
				}
			}
		}
		else if (ent->code == GAL_KC_Down) {
			eint skip_h = down_skip_h(bin, iw);
			if (iy + ih + skip_h == vw->rect.h) {
				scr->offset_y  = 0;
				GUI_MENU_BUTTON_DATA(menu->bn1)->active = false;
				GUI_MENU_BUTTON_DATA(menu->bn2)->active = true;
				egui_update(menu->bn1);
				egui_update(menu->bn2);
			}
			else if (iy + ih + ih + skip_h > sw->rect.h - scr->offset_y) {
				GuiMenuButton *bn = GUI_MENU_BUTTON_DATA(menu->bn1);
				scr->offset_y -= ih + skip_h - sw->rect.h + scr->offset_y + iy + ih;
				if (!bn->active) {
					bn->active = true;
					egui_update(menu->bn1);
				}
				if (sw->rect.h - scr->offset_y == vw->rect.h) {
					GUI_MENU_BUTTON_DATA(menu->bn2)->active = false;
					egui_update(menu->bn2);
				}
			}
		}
	}

	if (menu->old_offset_y != scr->offset_y) {
		menu->old_offset_y  = scr->offset_y;
		egui_move(hobj, 0, scr->offset_y);
		egui_update(hobj);
	}

	if (bin->focus || ent->code == GAL_KC_Up || ent->code == GAL_KC_Down) {
		GuiEventOrders *es = e_genetype_orders(GTYPE_BIN, GTYPE_EVENT);
		return es->keydown(hobj, ent);
	}
	return -1;
}

static void menu_realize(eHandle hobj, GuiWidget *wid)
{
	GalWindowAttr attr = {0};

	attr.type    = GalWindowTemp;
	attr.width   = wid->rect.w;
	attr.height  = wid->rect.h;
	attr.wclass  = GAL_INPUT_OUTPUT;
	attr.wa_mask = GAL_WA_NOREDIR | GAL_WA_TOPMOST;

	wid->window   = egal_window_new(&attr);
	wid->drawable = wid->window;
#ifdef _GAL_SUPPORT_CAIRO
	{
		GalPBAttr attr;
		attr.func = GalPBcopy;
		attr.use_cairo = true;
		wid->pb   = egal_pb_new(wid->drawable, &attr);
	}
#else
	wid->pb     = egal_pb_new(wid->drawable, NULL);
#endif
	egal_window_set_attachment(wid->drawable, hobj);
}

static void menu_move(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->window)
		egal_window_move(wid->window, wid->rect.x, wid->rect.y);
}

static void menu_show(eHandle hobj)
{
	GuiMenu      *menu    = GUI_MENU_DATA(hobj);
	GuiBin       *bin     = GUI_BIN_DATA(menu->box);
	GuiScrollWin *scrwin  = GUI_SCROLLWIN_DATA(menu->scrwin);
	GuiWidgetOrders *wors = e_genetype_orders(GTYPE_BIN, GTYPE_WIDGET);

	if (bin->focus) {
		egui_unset_status(bin->focus, GuiStatusFocus);
		bin->focus = 0;
	}

	scrwin->offset_y = 0;
	menu->old_offset_y = 0;
	egui_move(menu->box, 0, 0);

	wors->show(hobj);
	egal_window_show(GUI_WIDGET_DATA(hobj)->drawable);
}

static void menu_hide(eHandle hobj)
{
	GuiWidgetOrders *wors = e_genetype_orders(GTYPE_BIN, GTYPE_WIDGET);
	wors->hide(hobj);
	egal_window_hide(GUI_WIDGET_DATA(hobj)->drawable);
}

static void menu_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool up, bool a)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);
	GalRect  rect = GUI_WIDGET_DATA(menu->box)->rect;

	GalEventResize res = {rect.w, rect.h};
	e_signal_emit(menu->box, SIG_RESIZE, &res);

	GUI_WIDGET_DATA(hobj)->rect.w = req_w;
}

static eint menu_resize(eHandle hobj, GuiWidget *pw, GalEventResize *resize)
{
	GuiMenu   *menu = GUI_MENU_DATA(hobj);
	GuiWidget *swid = GUI_WIDGET_DATA(menu->scrwin);

	eint h = resize->h;
	if (GUI_STATUS_VISIBLE(menu->bn1)) {
		swid->rect.y = MENU_BN_SIZE;
		h -= MENU_BN_SIZE * 2;
		menu_bn_request_resize(menu->bn1, resize->w, 0);
		menu_bn_request_resize(menu->bn2, resize->w, 0);
		egui_move(menu->bn1, 0, 0);
		egui_move(menu->bn2, 0, resize->h - MENU_BN_SIZE);
	}
	else
		swid->rect.y = 0;

	egui_move_resize(menu->scrwin, 0, swid->rect.y, resize->w, h);

	return 0;
}

static void menu_v_request_resize(eHandle hobj, eint w, eint h)
{
	GuiWidget     *wid = GUI_WIDGET_DATA(hobj);
	GalEventResize res = {w, h};

	if (h != wid->rect.h) {
		wid->rect.w = w;
		wid->rect.h = h;
		egal_window_resize(wid->drawable, w, h);
		menu_resize(hobj, wid, &res);
	}
}

static eint menu_leave(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(GUI_WIDGET_DATA(hobj)->parent);

	if (menu->popup_timer) {
		e_timer_del(menu->popup_timer);
		menu->popup_timer = 0;
		menu->temp = 0;
	}
	return 0;
}

static eint menu_enter(eHandle hobj, eint x, eint y)
{
	GuiMenu *menu = GUI_MENU_DATA(GUI_WIDGET_DATA(hobj)->parent);

	if (menu->popup) {
		GuiMenu *pm = GUI_MENU_DATA(menu->popup);
		if (pm->popup) {
			close_popup_menu(GUI_MENU_DATA(pm->popup));
			pm->popup = 0;
		}
	}

	if (menu->close_timer) {
		GuiMenu *smenu = GUI_MENU_DATA(menu->super);
		e_timer_del(menu->close_timer);
		menu->close_timer = 0;
		smenu->popup = OBJECT_OFFSET(menu);
		egui_set_focus(menu->super_item);
	}

	return 0;
}

static void menu_add(eHandle hobj, eHandle item)
{
	GuiWidget *iw = GUI_WIDGET_DATA(item);
	iw->rect.w += MENU_BRINK_SIZE * 2;
	iw->min_w   = iw->rect.w;
	egui_add(GUI_MENU_DATA(hobj)->box, item);
}

static void menu_vbox_add(eHandle pobj, eHandle cobj)
{
	GuiWidget *pw = GUI_WIDGET_DATA(pobj);
	vbox_add(pobj, cobj);
	e_object_refer(cobj);
	if (pw->drawable)
		egal_window_resize(pw->drawable, pw->rect.w, pw->rect.h);
}

static eint menu_v_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid  = GUI_WIDGET_DATA(hobj);
	GuiMenu   *menu = GUI_MENU_DATA(hobj);

	menu->owner = e_va_arg(vp, eHandle);
	menu->cb    = e_va_arg(vp, MenuPositionCB);

	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->rect.w = 1;
	wid->rect.h = 1;
	widget_unset_status(wid, GuiStatusVisible);

	menu_realize(hobj, wid);

	menu->bn1 = e_object_new(GTYPE_MENU_BUTTON, BN_UP);
	menu_vbox_add(hobj, menu->bn1);

	menu->scrwin = e_object_new(GTYPE_MENU_SCROLLWIN);
	e_signal_connect(menu->scrwin, SIG_ENTER, menu_enter);
	e_signal_connect(menu->scrwin, SIG_LEAVE, menu_leave);
	menu_vbox_add(hobj, menu->scrwin);

	menu->bn2 = e_object_new(GTYPE_MENU_BUTTON, BN_DOWN);
	menu_vbox_add(hobj, menu->bn2);

	menu->box = e_object_new(GTYPE_MENU_VBOX);
	e_signal_connect(menu->box, SIG_LBUTTONDOWN, menu_lbuttondown);
	egui_add(menu->scrwin, menu->box);

	GUI_BIN_DATA(hobj)->focus = menu->scrwin;
	GUI_BIN_DATA(menu->scrwin)->focus = menu->box;

	return 0;
}

static void menu_v_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w  = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e  = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b  = e_genetype_orders(new, GTYPE_BIN);
	eCellOrders     *c  = e_genetype_orders(new, GTYPE_CELL);

	vbox_request_layout = b->request_layout;
	b->request_layout   = menu_request_layout;

	vbox_add        = w->add;
	w->add          = menu_add;
	w->move         = menu_move;
	w->show         = menu_show;
	w->hide         = menu_hide;
	w->realize      = menu_realize;
	w->set_min      = NULL;

	e->lbuttondown  = menu_lbuttondown;
	e->keydown      = menu_keydown;

	c->init         = menu_v_init;
}

static void menu_set_focus(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);
	GuiBin  *bin  = GUI_BIN_DATA(menu->box);

	if (bin->head) {
		eHandle cobj = OBJECT_OFFSET(bin->head);
		egui_set_focus(cobj);
		GUI_MENU_SHELL_DATA(menu->shell)->active = hobj;
	}
}

eHandle egui_menu_new(eHandle owner, MenuPositionCB cb)
{
	return e_object_new(GTYPE_MENU_V, owner, cb);
}

void egui_menu_popup(eHandle hobj)
{
	GuiMenuShell *shell;
	eint x, y, h;

	GuiMenu *menu = GUI_MENU_DATA(hobj);
	GalRect *rect  = &GUI_WIDGET_DATA(menu->box)->rect;

	if (!menu->shell) {
		menu->shell = e_object_new(GTYPE_MENU_SHELL);
		e_signal_emit(menu->shell, SIG_REALIZE);
	}
	shell = GUI_MENU_SHELL_DATA(menu->shell);
	shell->root_menu = hobj;
	shell->active    = hobj;
	shell->source    = egui_get_top(menu->owner);

	if (menu->cb) {
		menu->cb(menu->owner, &x, &y, &h);
	}
	else {
		GalVisualInfo vinfo;

		egal_get_visual_info(egal_root_window(), &vinfo);
		egal_get_pointer(GUI_WIDGET_DATA(hobj)->drawable, &x, &y, NULL, NULL, NULL);

		if (x + rect->w > vinfo.w)
			x -= rect->w;

		if (rect->h > vinfo.h) {
			y = 0;
			h = vinfo.h;
		}
		else if (y + rect->h > vinfo.h) {
			h  = rect->h;
			y -= rect->h;
			if (y < 0)
				y = vinfo.h - h;
		}
		else h = rect->h;
	}

	if (h < rect->h) {
		egui_set_status(menu->bn1, GuiStatusVisible);
		egui_set_status(menu->bn2, GuiStatusVisible);
		GUI_MENU_BUTTON_DATA(menu->bn1)->active = false;
		GUI_MENU_BUTTON_DATA(menu->bn2)->active = true;
	}
	else {
		egui_unset_status(menu->bn1, GuiStatusVisible);
		egui_unset_status(menu->bn2, GuiStatusVisible);
	}

	menu_v_request_resize(hobj, rect->w, h);

	egui_move(hobj, x, y);
	egui_show(menu->shell, true);
	egui_show(hobj, true);
}

static void item_init_orders(eGeneType new, ePointer this);

eGeneType egui_genetype_menu_item(void)
{
	static eGeneType gtype = 0;
	if (gtype == 0) {
		eGeneInfo info = {
			0,
			item_init_orders,
			sizeof(GuiMenuItem),
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, GTYPE_STRINGS, NULL);
	}
	return gtype;
}

static eint item_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);

	GalRect rc = exp->rect;

	if (!wid->drawable)
		wid->drawable = GUI_WIDGET_DATA(wid->parent)->drawable;

	if (WIDGET_STATUS_FOCUS(wid))
		egal_set_foreground(exp->pb, 0xb4a590);
	else
		egal_set_foreground(exp->pb, 0x3c3b37);

	egal_fill_rect(wid->drawable, exp->pb, rc.x, rc.y, rc.w, rc.h);

	rc.x = MENU_BRINK_SIZE;
	rc.y = wid->offset_y;
	rc.w = wid->rect.w - MENU_BRINK_SIZE * 2;
	rc.h = wid->rect.h;
	egal_set_foreground(exp->pb, 0xffffff);
	egui_draw_strings(wid->drawable, exp->pb, 0, item->label, &rc, LF_HLeft | LF_VCenter);

	if (item->accelkey) {
		rc.x = wid->offset_x;
		rc.w = wid->rect.w - 5;
		egui_draw_strings(wid->drawable, exp->pb, 0, item->accelkey, &rc, LF_HRight | LF_VCenter);
	}

	switch (item->type) {
	case MenuItemBranch:
		egal_composite_image(wid->drawable, exp->pb,
				wid->offset_x + (wid->rect.w - 10),
				wid->offset_y + 7,
				menu_bn_right, 0, 0, menu_bn_right->w, menu_bn_right->h);
		break;
	case MenuItemRadio:
		if (item->p.radio->selected == hobj) {
			egal_composite_image(wid->drawable, exp->pb,
					4,
					wid->offset_y + 7,
					menu_bn_radio, 0, 0, menu_bn_radio->w, menu_bn_radio->h);
		}
		break;
	case MenuItemCheck:
		if (item->p.check) {
			egal_composite_image(wid->drawable, exp->pb,
					4,
					wid->offset_y + 7,
					menu_bn_check, 0, 0, menu_bn_check->w, menu_bn_check->h);
		}
		break;

	case MenuItemDefault:
	default:
		break;
	}

	return 0;
}

static void item_popup_submenu(eHandle hobj, eHandle sub)
{
	GuiMenu *menu = GUI_MENU_DATA(item_to_menu(hobj));
	GuiMenu *subm = GUI_MENU_DATA(sub);
	GalRect *irc  = &GUI_WIDGET_DATA(hobj)->rect;
	GalRect *mrc  = &GUI_WIDGET_DATA(OBJECT_OFFSET(menu))->rect;
	GalRect *brc  = &GUI_WIDGET_DATA(menu->box)->rect;
	GalRect *src  = &GUI_WIDGET_DATA(subm->box)->rect;
	GuiScrollWin *scrwin = GUI_SCROLLWIN_DATA(menu->scrwin);

	eint x, y, _h;

	GalVisualInfo vinfo;
	egal_get_visual_info(egal_root_window(), &vinfo);

	if (src->h > vinfo.h) {
		_h = vinfo.h;
		egui_set_status(subm->bn1, GuiStatusVisible);
		egui_set_status(subm->bn2, GuiStatusVisible);
		GUI_MENU_BUTTON_DATA(subm->bn1)->active = false;
		GUI_MENU_BUTTON_DATA(subm->bn2)->active = true;
	}
	else {
		_h = src->h;
		egui_unset_status(subm->bn1, GuiStatusVisible);
		egui_unset_status(subm->bn2, GuiStatusVisible);
	}

	x = mrc->x + mrc->w;
	if (x + src->w > vinfo.w)
		x = mrc->x - src->w;

	if (GUI_STATUS_VISIBLE(menu->bn1))
		y = irc->y + mrc->y + brc->y + scrwin->offset_y + MENU_BN_SIZE;
	else
		y = irc->y + mrc->y + brc->y + scrwin->offset_y;

	if (y + src->h > vinfo.h)
		y = vinfo.h - src->h;

	menu_v_request_resize(sub, src->w, _h);

	egui_move(sub, x, y);
	egui_show(sub, true);

	menu->popup = sub;
	GUI_MENU_DATA(sub)->shell = menu->shell;
}

static eint popup_timer_cb(eTimer timer, euint num, ePointer args)
{
	GuiMenuItem *item = args;
	eHandle  hobj = item_to_menu(OBJECT_OFFSET(item));
	GuiMenu *menu = GUI_MENU_DATA(hobj);
	item_popup_submenu(OBJECT_OFFSET(item), item->p.submenu);
	menu->popup_timer = 0;
	menu->popup = menu->temp;
	menu->temp  = 0;
	return -1;
}

static eint close_timer_cb(eTimer timer, euint num, ePointer args)
{
	GuiMenu *menu  = args;
	GuiMenu *super = GUI_MENU_DATA(menu->super);
	close_popup_menu(menu);
	if (super->popup == OBJECT_OFFSET(menu))
		super->popup  = 0;
	menu->close_timer = 0;
	return -1;
}

static eint item_focus_in(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(item_to_menu(hobj));
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);

	if (menu->popup) {
		GuiMenu *pm = GUI_MENU_DATA(menu->popup);
		if (item->p.submenu != menu->popup) {
			if (!pm->close_timer)
				pm->close_timer = e_timer_add(200, close_timer_cb, pm);
		}
		else if (pm->close_timer) {
			e_timer_del(pm->close_timer);
			pm->close_timer = 0;
		}
	}

	if (menu->temp && item->p.submenu != menu->temp) {
		e_timer_del(menu->popup_timer);
		menu->popup_timer = 0;
		menu->temp = 0;
	}

	if (item->type == MenuItemBranch
			&& !menu->temp
			&& item->p.submenu
			&& menu->popup != item->p.submenu) {
		menu->temp = item->p.submenu;
		menu->popup_timer = e_timer_add(300, popup_timer_cb, item);
	}

	egui_update(hobj);
	return 0;
}

static eint item_focus_out(eHandle hobj)
{
	egui_update(hobj);
	return 0;
}

static eint item_enter(eHandle hobj, eint x, eint y)
{
	egui_set_focus(hobj);
	return 0;
}

static eint item_leave(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(item_to_menu(hobj));
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);
	
	if (item->type != MenuItemBranch ||
			!item->p.submenu || item->p.submenu != menu->popup) {
		GuiBin *bin = GUI_BIN_DATA(menu->box);
		if (bin->focus)
			egui_unset_focus(bin->focus);
	}

	return 0;
}

static void menu_item_ok(GuiMenu *menu, GuiMenuItem *item)
{
	eHandle    hobj = OBJECT_OFFSET(item);
	GalEventKey ent = {GAL_KC_Escape};

	switch (item->type) {
	case MenuItemBranch:
		if (item->p.submenu)
			item_popup_submenu(hobj, item->p.submenu);
		return;
	case MenuItemRadio:
		item->p.radio->selected = hobj;
		break;
	case MenuItemCheck:
		item->p.check = !item->p.check;
		break;

	case MenuItemDefault:
	default:
		break;
	}

	if (!menu)
		menu = GUI_MENU_DATA(item_to_menu(hobj));
	e_signal_emit(hobj, SIG_CLICKED, e_signal_get_data(hobj, SIG_CLICKED));
	e_signal_emit(menu->shell, SIG_KEYDOWN, &ent);
}

static eint item_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);
	GuiMenu     *menu = GUI_MENU_DATA(item_to_menu(hobj));

	if (menu->temp) {
		e_timer_del(menu->popup_timer);
		menu->popup_timer = 0;
		menu->temp        = 0;
	}

	menu_item_ok(menu, item);
	return 0;
}

static bool menu_close(eHandle hobj)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);

	if (menu->super && !GUI_MENU_DATA(menu->super)->is_bar) {
		GUI_MENU_SHELL_DATA(menu->shell)->active = menu->super;
		return true;
	}
	return false;
}

static eint item_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);

	switch (ent->code) {
		case GAL_KC_Left:
			if (menu_close(item_to_menu(hobj))) {
				egui_unset_focus(hobj);
				if (item->type == MenuItemBranch && item->p.submenu)
					egui_hide(item->p.submenu, true);
			}
			return -1;

		case GAL_KC_Enter:
			menu_item_ok(NULL, item);
			return -1;

		case GAL_KC_Right:
			if (item->type == MenuItemBranch && item->p.submenu) {
				item_popup_submenu(hobj, item->p.submenu);
				menu_set_focus(item->p.submenu);
			}
			return -1;

		default:
			return 0;
	}
	return 0;
}

static const echar *item_get_strings(eHandle hobj)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);
	return item->label;
}

static eint item_init(eHandle hobj, eValist vp)
{
	GuiWidget   *iwid = GUI_WIDGET_DATA(hobj);
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);

	item->label  = e_strdup(e_va_arg(vp, const echar *));
	egui_strings_extent(0, item->label, &iwid->rect.w, &iwid->rect.h);
	iwid->min_w  = iwid->rect.w;
	iwid->min_h  = iwid->rect.h;
	item->l_size = iwid->rect.w;

	widget_set_status(iwid, 
			GuiStatusVisible | GuiStatusTransparent | GuiStatusActive | GuiStatusMouse);

	return 0;
}

static void item_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders   *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders  *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiStringsOrders *s = e_genetype_orders(new, GTYPE_STRINGS);
	eCellOrders      *c = e_genetype_orders(new, GTYPE_CELL);

	c->init           = item_init;
	w->expose         = item_expose;
	e->enter          = item_enter;
	e->leave          = item_leave;
	e->keydown        = item_keydown;
	e->focus_in       = item_focus_in;
	e->focus_out      = item_focus_out;
	e->lbuttondown    = item_lbuttondown;
	s->get_strings    = item_get_strings;
}

eHandle egui_menu_item_new_with_label(const echar *label)
{
	return e_object_new(GTYPE_MENU_ITEM, label);
}

static INLINE GuiMenuRadio *egui_menu_radio_new(eHandle hobj)
{
	GuiMenuRadio *radio = e_malloc(sizeof(GuiMenuRadio));
	radio->selected = hobj;
	return radio;
}

void egui_menu_item_set_accelkey(eHandle win, eHandle iobj,
		const echar *accelkey, AccelKeyCB callback, ePointer data)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(iobj);
	GuiWidget   *iwid = GUI_WIDGET_DATA(iobj);
	eint w;

	if (callback)
		egui_accelkey_connect(win, accelkey, callback, data);

	item->accelkey = e_strdup(accelkey);
	egui_strings_extent(0, accelkey, &w, NULL);
	egui_request_resize(iobj, iwid->rect.w + w + 50, iwid->rect.h);
}

void egui_menu_item_set_radio_group(eHandle iobj, eHandle radio)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(iobj);

	if (item->type != MenuItemDefault)
		return;

	item->type = MenuItemRadio;

	if (radio) {
		GuiMenuItem *r = GUI_MENU_ITEM_DATA(radio);
		if (r->type == MenuItemRadio)
			item->p.radio = r->p.radio;
		return;
	}

	item->p.radio = egui_menu_radio_new(iobj);
}

void egui_menu_item_set_check(eHandle iobj, bool check)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(iobj);

	if (item->type == MenuItemDefault)
		item->type = MenuItemCheck;
	else if (item->type != MenuItemCheck)
		return;

	item->type = MenuItemCheck;
	item->p.check = check;
}

void egui_menu_item_set_submenu(eHandle menu_item, eHandle submenu)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(menu_item);
	GuiMenu     *menu = GUI_MENU_DATA(submenu);

	if (item->type != MenuItemDefault)
		return;

	item->type = MenuItemBranch;
	menu->super      = item_to_menu(menu_item);
	menu->super_item = menu_item;
	item->p.submenu  = submenu;
}

void egui_menu_item_radio_select(eHandle iobj)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(iobj);
	if (item->type == MenuItemRadio)
		item->p.radio->selected = iobj;
}

static void menu_vbox_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	w->move     = menu_vbox_move;
	e->keydown  = menu_vbox_keydown;
}

static void menu_hbox_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	w->move = menu_hbox_move;
}

static eint separator_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GalRect rc = exp->rect;

	if (!wid->drawable)
		wid->drawable = GUI_WIDGET_DATA(wid->parent)->drawable;

	egal_set_foreground(exp->pb, 0x282825);

	egal_fill_rect(wid->drawable, exp->pb,
			wid->offset_x + rc.x,
			wid->offset_y + rc.y,
			rc.w, rc.h);
	return 0;
}

static void separator_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	w->expose          = separator_expose;
	w->move            = NULL;
	w->put             = NULL;
}

void egui_menu_add_separator(eHandle hobj)
{
	eHandle    sep = e_object_new(GTYPE_MENU_SEPARATOR);
	GuiWidget *wid = GUI_WIDGET_DATA(sep);

	wid->rect.w = 1;
	wid->rect.h = 1;
	wid->min_w  = 1;
	wid->min_h  = 1;
	wid->max_w  = 0;
	wid->max_h  = 1;
	widget_set_status(wid, GuiStatusVisible);

	egui_add(GUI_MENU_DATA(hobj)->box, sep);
}

static eint menu_bbn_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	return 0;
}

static eint menu_bbn_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	return 0;
}

static eint menu_bar_enter(eHandle hobj, eint x, eint y)
{
	return 0;
}

static eint menu_bar_leave(eHandle hobj)
{
	return 0;
}

static eint menu_bar_keyup(eHandle hobj, GalEventKey *ent)
{
	GuiMenuBar *bar = GUI_MENU_BAR_DATA(hobj);
	if (!bar->active)
		return -1;
	return 0;
}

static void bar_item_popup_submenu(GuiMenu *menu, eHandle hobj, eHandle sub)
{
	GuiWidget *mwid = GUI_WIDGET_DATA(menu->scrwin);
	GuiWidget *iwid = GUI_WIDGET_DATA(hobj);

	GuiMenu *submenu = GUI_MENU_DATA(sub);
	GalRect *src = &GUI_WIDGET_DATA(submenu->box)->rect;

	eint x, y, h;
	GalVisualInfo vinfo;

	egal_get_visual_info(egal_root_window(), &vinfo);
	egal_window_get_origin(mwid->drawable, &x, &y);
	x += iwid->rect.x;
	if (src->h > (vinfo.h - y - mwid->rect.h)
			&& y > (vinfo.h - mwid->rect.h) / 2) {
		h  = y;
		y -= src->h;
		if (y < 0) y = 0;
	}
	else {
		y += mwid->rect.h;
		h  = vinfo.h - y;
	}

	if (src->h > h) {
		egui_set_status(submenu->bn1, GuiStatusVisible);
		egui_set_status(submenu->bn2, GuiStatusVisible);
		GUI_MENU_BUTTON_DATA(submenu->bn1)->active = false;
		GUI_MENU_BUTTON_DATA(submenu->bn2)->active = true;
	}
	else {
		egui_unset_status(submenu->bn1, GuiStatusVisible);
		egui_unset_status(submenu->bn2, GuiStatusVisible);
		h = src->h;
	}

	menu_v_request_resize(sub, src->w, h);

	egui_move(sub, x, y);
	egui_show(sub, true);

	if (!menu->shell) {
		if (submenu->shell)
			menu->shell = submenu->shell;
		else {
			menu->shell = e_object_new(GTYPE_MENU_SHELL);
			e_signal_emit(menu->shell, SIG_REALIZE);
		}
	}
	GUI_MENU_SHELL_DATA(menu->shell)->root_menu = OBJECT_OFFSET(menu);

	menu->popup = sub;
	egui_show(menu->shell, true);
	GUI_MENU_DATA(sub)->shell = menu->shell;
	GUI_MENU_SHELL_DATA(menu->shell)->active = sub;
}

static void menu_bar_item_active(GuiMenu *menu, eHandle iobj)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(iobj);

	if (menu->popup && menu->popup != item->p.submenu) {
		close_popup_menu(GUI_MENU_DATA(menu->popup));
		menu->popup = 0;
	}

	if (item->p.submenu && menu->popup != item->p.submenu)
		bar_item_popup_submenu(menu, iobj, item->p.submenu);

	egui_set_focus(iobj);
}

static eint menu_bar_keydown(eHandle hobj, GalEventKey *ent)
{
	GuiMenuBar *bar = GUI_MENU_BAR_DATA(hobj);
	if (bar->active &&
			(ent->code == GAL_KC_Right || ent->code == GAL_KC_Left)) {
		GuiMenu *menu = GUI_MENU_DATA(hobj);
		GuiBin  *bin  = GUI_BIN_DATA(menu->box);
		if (bin->focus) {
			GuiWidget *wid = GUI_WIDGET_DATA(bin->focus);
			if (ent->code == GAL_KC_Right) {
				if (wid->next)
					wid = wid->next;
				else
					wid = bin->head;
			}
			else if (ent->code == GAL_KC_Left) {
				if (wid->prev)
					wid = wid->prev;
				else
					wid = bin->tail;
			}
			menu_bar_item_active(menu, OBJECT_OFFSET(wid));
		}
	}

	return 0;
}

static eint menu_bar_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuBar *bar = GUI_MENU_BAR_DATA(hobj);
	if (!bar->active)
		return -1;
	return 0;
}

static eint __menu_bar_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiBin *bin = GUI_BIN_DATA(hobj);
	eint x = mevent->point.x;
	eint y = mevent->point.y;
	eHandle grab = menu_point_in(bin, x, y);
	if (grab) {
		mouse_signal_emit(grab, SIG_LBUTTONDOWN, x, y);
	}
	return 0;
}

static eint menu_bar_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenuBar *bar = GUI_MENU_BAR_DATA(hobj);

	if (!bar->active)
		bar->active = true;

	__menu_bar_lbuttondown(hobj, mevent);

	return 0;
}

static eint menu_bar_mousemove(eHandle hobj, GalEventMouse *mevent)
{
	if (!GUI_STATUS_ACTIVE(hobj))
		return -1;

	return 0;
}

static void menu_bar_add(eHandle hobj, eHandle cobj)
{
	GuiWidget *cw  = GUI_WIDGET_DATA(cobj);
	GuiBin    *bin = GUI_BIN_DATA(hobj);

	cw->rect.x = 0;
	cw->rect.y = 0;
	cw->parent = hobj;
	STRUCT_LIST_INSERT_TAIL(GuiWidget, bin->head, bin->tail, cw, prev, next);

	GUI_WIDGET_ORDERS(hobj)->put(hobj, cobj);
	e_object_refer(cobj);
}

static eint bar_item_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GuiMenuItem *item = GUI_MENU_ITEM_DATA(hobj);

	GalRect rc = exp->rect;

	if (!wid->drawable)
		wid->drawable = GUI_WIDGET_DATA(wid->parent)->drawable;

	if (WIDGET_STATUS_FOCUS(wid)) {
		egal_set_foreground(exp->pb, 0xb4a590);
		egal_fill_rect(wid->drawable, exp->pb, rc.x, rc.y, rc.w, rc.h);
	}
	rc.x = wid->offset_x + (wid->rect.w - item->l_size) / 2;
	rc.y = wid->offset_y;
	rc.w = item->l_size;
	rc.h = wid->rect.h;
	egal_set_foreground(exp->pb, 0xffffff);
	egui_draw_strings(wid->drawable, exp->pb, 0, item->label, &rc, LF_HLeft | LF_VCenter);

	return 0;
}

static eint bar_item_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiMenu *menu = GUI_MENU_DATA(item_to_menu(hobj));

	menu_bar_item_active(menu, hobj);

	return 0;
}

static eint bar_item_focus_in(eHandle hobj)
{
	egui_update(hobj);
	return 0;
}

static eint bar_item_focus_out(eHandle hobj)
{
	egui_update(hobj);
	return 0;
}

static void menu_bar_hbox_add(eHandle hobj, eHandle cobj)
{
	GuiMenu *menu = GUI_MENU_DATA(hobj);
	GuiWidget *cw = GUI_WIDGET_DATA(cobj);

	cw->rect.w += 10;
	cw->min_w  += 10;
	hbox_add(menu->box, cobj);
	e_signal_connect(cobj, SIG_EXPOSE, bar_item_expose);
	e_signal_connect(cobj, SIG_LBUTTONDOWN, bar_item_lbuttondown);
	e_signal_connect(cobj, SIG_FOCUS_IN, bar_item_focus_in);
	e_signal_connect(cobj, SIG_FOCUS_OUT, bar_item_focus_out);
	//e_signal_connect(cobj, SIG_KEYDOWN, bar_item_keydown);

	if (GUI_WIDGET_DATA(menu->box)->rect.w >
			GUI_WIDGET_DATA(hobj)->rect.w) {
		egui_show(menu->bn1, true);
		egui_show(menu->bn2, true);
		GUI_MENU_BUTTON_DATA(menu->bn2)->active = true;
	}
}

static void menu_bar_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool up, bool a)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	if (wid->min_h < req_h) {
		wid->min_h = req_h;
		wid->rect.h = req_h;
		if (wid->parent)
			egui_request_layout(wid->parent, hobj, req_w, req_h, false, a);
	}
}
/*
static eint bar_scrwin_resize(eHandle hobj, GuiWidget *wid, GalEventResize *conf)
{
	wid->rect.w = conf->rect.w;
	return 0;
}
*/
static void bar_scrwin_request_layout(eHandle hobj, eHandle cobj, eint req_w, eint req_h, bool up, bool a)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->min_h  = req_h;
	wid->rect.h = req_h;
	if (wid->parent) {
		GalEventResize res = {req_w, req_h};
		egui_request_layout(wid->parent, hobj, req_w, req_h, false, a);
		e_signal_emit(cobj, SIG_RESIZE, &res);
	}
}

static void bar_scrwin_init_orders(eGeneType new, ePointer this)
{
	//GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);

	//w->resize         = bar_scrwin_resize;
	e->lbuttondown    = menu_lbuttondown;
	b->request_layout = bar_scrwin_request_layout;
}

static void menu_bar_move_resize(eHandle hobj, GalRect *orc, GalRect *nrc)
{
	GuiMenu   *menu = GUI_MENU_DATA(hobj);
	GuiWidget *swid = GUI_WIDGET_DATA(menu->scrwin);
	swid->rect.w    = GUI_WIDGET_DATA(hobj)->rect.w;
	egal_window_resize(swid->window, swid->rect.w, swid->rect.h);
}

static void menu_bar_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiBinOrders    *b = e_genetype_orders(new, GTYPE_BIN);
	GuiWidgetOrders *h = e_genetype_orders(GTYPE_HBOX, GTYPE_WIDGET);

	b->request_layout = menu_bar_request_layout;

	hbox_add        = h->add;
	w->add          = menu_bar_hbox_add;
	w->show         = menu_show;
	w->hide         = menu_hide;
	w->move_resize  = menu_bar_move_resize;

	e->enter        = menu_bar_enter;
	e->leave        = menu_bar_leave;
	e->keyup        = menu_bar_keyup;
	e->keydown      = menu_bar_keydown;
	e->lbuttonup    = menu_bar_lbuttonup;
	e->lbuttondown  = menu_bar_lbuttondown;
	e->mousemove    = menu_bar_mousemove;
}

static eint menu_bar_draw_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GalRect rc = exp->rect;
	egal_set_foreground(exp->pb, 0x3c3b37);
	egal_fill_rect(wid->drawable, exp->pb, rc.x, rc.y, rc.w, rc.h);
    return 0;
}

static eint menu_bar_init_data(eHandle hobj, ePointer this)
{
	GuiWidget *wid  = GUI_WIDGET_DATA(hobj);
	GuiMenu   *menu = GUI_MENU_DATA(hobj);

	menu->is_bar = true;
	widget_set_status(wid, GuiStatusVisible);
	widget_unset_status(wid, GuiStatusActive);

	menu->bn1 = e_object_new(GTYPE_MENU_BUTTON, BN_LEFT);
	e_signal_connect(menu->bn1, SIG_LBUTTONDOWN, menu_bbn_lbuttondown);
	e_signal_connect(menu->bn1, SIG_LBUTTONUP,   menu_bbn_lbuttonup);
	menu_bar_add(hobj, menu->bn1);

	menu->scrwin = e_object_new(GTYPE_MENU_BAR_SCROLLWIN);
	e_signal_connect(menu->scrwin, SIG_EXPOSE_BG, menu_bar_draw_expose_bg);
	menu_bar_add(hobj, menu->scrwin);

	menu->bn2 = e_object_new(GTYPE_MENU_BUTTON, BN_RIGHT);
	e_signal_connect(menu->bn2, SIG_LBUTTONDOWN, menu_bbn_lbuttondown);
	e_signal_connect(menu->bn2, SIG_LBUTTONUP,   menu_bbn_lbuttonup);
	menu_bar_add(hobj, menu->bn2);

	menu->box = e_object_new(GTYPE_MENU_HBOX);
	//egui_box_set_spacing(menu->box, 10);
	e_signal_connect(menu->box, SIG_LBUTTONDOWN, menu_lbuttondown);
	egui_add(menu->scrwin, menu->box);

	GUI_BIN_DATA(hobj)->focus = menu->scrwin;
	GUI_BIN_DATA(menu->scrwin)->focus = menu->box;

	wid->min_w = 1;
	wid->min_h = 1;
	wid->max_h = 1;
	return 0;
}

eHandle egui_menu_bar_new(void)
{
	return e_object_new(GTYPE_MENU_BAR);
}

typedef struct _PathNode PathNode;
struct _PathNode {
	MenuItemType type;

	const echar *name;
	const echar *accelkey;

	AccelKeyCB   callback;
	ePointer     data;
	eHandle      item;

	PathNode *superior;
	PathNode *next;
	PathNode *head;

	const echar *radio_path;
};

static PathNode *name_to_node(PathNode *parent, const echar *name, eint n)
{
	PathNode *node = parent->head;
	while (node) {
		if (!e_strncmp(node->name, name, n))
			break;
		node = node->next;
	}
	return node;
}

static MenuItemType type_name_cmp(const echar *name)
{
	const echar *p = name;
	eint n = 0;
	for (; p[n] && p[n] != '>'; n++);

	if (n == 0  || p[n] != '>')
		return -1;

	if (!e_strncasecmp(p, _("Separator"), n))
		return MenuItemSeparator;
	else if (!e_strncasecmp(p, _("RadioItem"), n))
		return MenuItemRadio;
	else if (!e_strncasecmp(p, _("CheckItem"), n))
		return MenuItemCheck;
	else if (!e_strncasecmp(p, _("Branch"), n))
		return MenuItemBranch;
	else if (!e_strncasecmp(p, _("Tearoff"), n))
		return MenuItemTearoff;

	return MenuItemDefault;
}

static MenuItemType type_name_convert(const echar *name)
{
	const echar *p = name;

	if (*p == '<')
		return type_name_cmp(p + 1);

	return MenuItemRadio;
}

static PathNode *create_node_with_entry(PathNode *parent, const echar *name, GuiMenuEntry *entry)
{
	PathNode *node = e_malloc(sizeof(PathNode));
	node->name = name;
	node->head = NULL;
	node->type = type_name_convert(entry->type);
	if (node->type == MenuItemRadio) {
		if (*entry->type != '<')
			node->radio_path = entry->type;
		else
			node->radio_path = NULL;
	}
	node->data = entry->data;
	node->accelkey = entry->accelkey;
	node->callback = entry->callback;
	node->superior = parent;

	olist_add_tail(PathNode, parent->head, node, next);

	return node;
}

static void item_entry_create_node(PathNode *root, GuiMenuEntry *entry)
{
	PathNode *curpath = root;

	const echar *p = entry->path;

	while (*p) {
		PathNode *node;
		eint n;

		while (*p == '/') p++;

		for (n = 0; p[n] && p[n] != '/'; n++);

		if (n == 0) break;

		node = name_to_node(curpath, p, n);
		if (!node && !p[n]) {
			const echar *name = e_strndup(p, n);
			node = create_node_with_entry(curpath, name, entry);
		}

		if (node)
			curpath = node;
		else
			break;

		p += n;
	}
}

static PathNode *path_to_node(PathNode *root, const echar *path)
{
	PathNode *node = root;
	const echar *p = path;

	while (*p) {
		eint n;

		while (*p == '/') p++;

		for (n = 0; p[n] && p[n] != '/'; n++);

		if (n == 0) break;

		if (!(node = name_to_node(node, p, n)))
			break;

		p += n;
	}

	return node;
}

static void menu_create_from_node(eHandle win, PathNode *root, PathNode *node, eHandle super)
{
	eHandle menu = egui_menu_new(win, NULL);

	while (node) {
		if (node->type == MenuItemTearoff)
			;
		else if (node->type == MenuItemSeparator)
			egui_menu_add_separator(menu);
		else {
			node->item = egui_menu_item_new_with_label(node->name);

			if (node->callback)
				e_signal_connect1(node->item, SIG_CLICKED, node->callback, node->data);

			if (node->accelkey)
				egui_menu_item_set_accelkey(win, node->item,
						node->accelkey, node->callback, node->data);

			if (node->type == MenuItemCheck)
				egui_menu_item_set_check(node->item, true);
			else if (node->type == MenuItemRadio) {
				if (!node->radio_path)
					egui_menu_item_set_radio_group(node->item, 0);
				else {
					PathNode *t = path_to_node(root, node->radio_path);
					egui_menu_item_set_radio_group(node->item, t->item);
				}
			}

			egui_add(menu, node->item);
			if (node->head)
				menu_create_from_node(win, root, node->head, node->item);
		}
		node = node->next;
	}

	egui_menu_item_set_submenu(super, menu);
}

static eint menu_popup_cb(eHandle iobj, ePointer data)
{
	GuiMenuBar *bar = GUI_MENU_BAR_DATA(item_to_menu((eHandle)data));
	if (!bar->active)
		bar->active = true;
	return mouse_signal_emit((eHandle)data, SIG_LBUTTONDOWN, 0, 0);
}

static eHandle menu_bar_create_from_path(eHandle win, PathNode *root)
{
	eHandle menubar = e_object_new(GTYPE_MENU_BAR);

	PathNode  *node = root->head;
	while (node) {
		node->item = egui_menu_item_new_with_label(node->name);
		if (node->accelkey) {
			egui_accelkey_connect(win, node->accelkey, menu_popup_cb, (ePointer)node->item);
		}
		egui_add(menubar, node->item);
		if (node->head) {
			menu_create_from_node(win, root, node->head, node->item);
		}
		node = node->next;
	}

	return menubar;
}

eHandle menu_bar_new_with_entrys(eHandle win, GuiMenuEntry *entrys, eint num)
{
	PathNode node = {0};

	eint i;
	for (i = 0; i < num; i++) {
		item_entry_create_node(&node, entrys + i);
	}

	return menu_bar_create_from_path(win, &node);
}

