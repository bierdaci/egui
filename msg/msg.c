#include <ctype.h>

#include <egui/egui.h>

#include "common.h"
#include "msg.h"

#define GTYPE_CHAT          			egui_genetype_chat()
#define CHAT_WIN_DATA(hobj) ((ChatWin *)e_object_type_data(hobj, GTYPE_CHAT))

#define MAX_USERNAME	20

#define SYSMSG_TYPE_ADD			0
#define SYSMSG_TYPE_TRANSFER	1

#define DIRECT_SEND		0
#define DIRECT_RECV		1
eGeneType egui_genetype_chat(void);

typedef struct _UserProfile UserProfile;
typedef struct _ChatWin     ChatWin;
typedef struct _UnreadData  UnreadData;
typedef struct _SysmsgList  SysmsgList;
struct _ChatWin {
	eHandle view_hbox;
	eHandle view_text;
	eHandle view_vbar;
	eHandle edit_vbar;
	eHandle edit_text;
	eHandle edit_hbox;
	eHandle vbox;
	eHandle hbox;
	eHandle bn;
	UserProfile *user;
};

struct _UserProfile {
	echar   name[MAX_USERNAME];
	eHandle chat;
	ChatWin *cwin;
	bool online;
	bool outlander;
	UnreadData *uhead;
	UnreadData *utail;
	eTimer timer;
	euint  count;
};

struct _UnreadData {
	eint type;
	eint size;
	ePointer data;
	UnreadData *next;
};

struct _SysmsgList {
	eint type;
	eint size;
	ePointer data;
	SysmsgList *next;
};

typedef struct {
	Cred cred;
	eint status;
	eint direct;
	eint total_size;
	eint progress_size;
} TransferStatus;

static eint sfd;
static euint SIG_RECV_MSG = 0;
static eHandle fri_clist  = 0;
static eTimer  sys_timer  = 0;
static euint   sys_count  = 0;
static eHandle sys_bn     = 0;
static SysmsgList *sys_head = NULL;
static SysmsgList *sys_tail = NULL;

static eHandle transfer_dlg   = 0;
static eHandle transfer_clist = 0;

static echar save_path[MAX_PATH];

static void view_text_append(eHandle hobj, const echar *text, eint n)
{
	GuiText *t = GUI_TEXT_DATA(hobj);
	egui_text_append(hobj, (const echar *)text, n);
	egui_adjust_set_hook(t->vadj, 0xfffff);
	egui_update(hobj);
}

static eint send_message(eHandle hobj, ePointer data)
{
	ChatWin *chat = data;
	echar buf[256];
	eint n = egui_get_text(chat->edit_text, buf, 256);

	buf[n] = 0;
	egui_text_append(chat->view_text, _("self:\n"), 6);
	view_text_append(chat->view_text, (const echar *)buf, n);
	egui_text_clear(chat->edit_text);
	egui_set_focus(chat->edit_text);
	client_send_realmsg(sfd, (char *)chat->user->name, (char *)buf);
	return 0;
}

static eint append_text_cb(eHandle hobj, eint type, ePointer data)
{
	echar *msg = data;
	view_text_append(hobj, msg, e_strlen(msg));
	e_free(msg);
	return 0;
}

static void chat_win_new(eHandle win)
{
	ChatWin *chat = CHAT_WIN_DATA(win);
	
	chat->vbox = egui_vbox_new();
	egui_set_expand(chat->vbox, true);
	egui_add(win, chat->vbox);

	chat->view_hbox = egui_hbox_new();
	chat->view_text = egui_text_new(450, 200);
	chat->view_vbar = egui_vscrollbar_new(true);
	egui_add(chat->view_hbox, chat->view_text);
	egui_add(chat->view_hbox, chat->view_vbar);
	egui_set_expand(chat->view_hbox, true);
	egui_hook_v(chat->view_text, chat->view_vbar);
	e_signal_connect1(chat->view_text, SIG_RECV_MSG, append_text_cb, chat);

	chat->edit_hbox = egui_hbox_new();
	chat->edit_text = egui_text_new(300, 200);
	chat->edit_vbar = egui_vscrollbar_new(true);
	egui_add(chat->edit_hbox, chat->edit_text);
	egui_add(chat->edit_hbox, chat->edit_vbar);
	egui_set_expand(chat->edit_hbox, true);
	egui_hook_v(chat->edit_text, chat->edit_vbar);

	egui_set_min(chat->view_text, 100, 100);
	egui_set_min(chat->edit_text, 100, 100);

	egui_add(chat->vbox, chat->view_hbox);
	egui_add_spacing(chat->vbox, 20);
	egui_add(chat->vbox, chat->edit_hbox);
	chat->hbox = egui_hbox_new();
	egui_set_expand_h(chat->hbox, true);
	egui_box_set_layout(chat->hbox, BoxLayout_END);
	chat->bn  = egui_label_button_new(_("send"));
	e_signal_connect1(chat->bn, SIG_CLICKED, send_message, chat);
	egui_add(chat->hbox, chat->bn);
	egui_add_spacing(chat->vbox, 5);
	egui_add(chat->vbox, chat->hbox);
	egui_add_spacing(chat->vbox, 5);

	egui_text_set_only_read(chat->view_text, true);
}

static eint (*window_init)(eHandle, eValist);
static eint chat_init(eHandle hobj, eValist va)
{
	window_init(hobj, va);

	chat_win_new(hobj);

	return 0;
}

static void chat_destroy(eHandle hobj)
{
	egui_hide(hobj, false);
}

static void chat_init_orders(eGeneType new, ePointer data)
{
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	window_init = c->init;
	c->init     = chat_init;
	w->destroy  = chat_destroy;
}

eGeneType egui_genetype_chat(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			chat_init_orders,
			sizeof(ChatWin),
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WINDOW, NULL);
	}
	return gtype;
}

static eHandle add_dlg    = 0;
static eHandle add_clist  = 0;
static eHandle search_dlg = 0;
static void add_clist_draw_bk(eHandle hobj, GuiClist *cl,
		GalDrawable drawable, GalPB pb,
		int x, int w, int h, bool light, bool focus, int index)
{
	if (light) {
		if (focus)
			egal_set_foreground(pb, 0xc4c4ff);
		else
			egal_set_foreground(pb, 0xc4c4c4);
	}
	else
		egal_set_foreground(pb, 0xffffff);

	egal_fill_rect(drawable, pb, x, 0, w, h);
}

static eint add_user_cb(eHandle hobj, ePointer data)
{
	ClsItemBar *ibar = egui_clist_get_hlight((eHandle)data);

	client_add_friend(sfd, clist_get_grid_data((eHandle)data, ibar, 0));

	egui_hide(add_dlg, false);

	return 0;
}

static eint clicked_hide(eHandle hobj, ePointer data)
{
	egui_hide((eHandle)data, false);
	return 0;
}

static eint system_timer_cb(eTimer timer, euint n, ePointer data)
{
	sys_count += n;
	egui_update(sys_bn);
	return 0;
}

int ui_notify_add_dlg(char *data)
{
	echar *name = (echar *)data;
	ClsItemBar*ibar = egui_clist_find(fri_clist, name);

	if (!ibar) {
		ibar = egui_clist_ibar_new_valist(fri_clist, name);
		UserProfile *user = e_calloc(1, sizeof(UserProfile));
		ibar->add_data    = user;
		user->online      = true;
		user->outlander   = false;
		e_strcpy(user->name, (const echar *)name);
		egui_clist_insert_ibar(fri_clist, ibar);
	}
	return 0;
}

int ui_accept_add_dlg(char *name)
{
	SysmsgList *t = e_malloc(sizeof(SysmsgList));

	t->type = SYSMSG_TYPE_ADD;
	t->data = e_strdup(_(name));
	t->next = NULL;
	if (!sys_head) {
		sys_head = t;
		sys_tail = t;
	}
	else {
		sys_tail->next = t;
		sys_tail = t;
	}

	if (sys_timer == 0) {
		sys_count = 0;
		sys_timer = e_timer_add(500, system_timer_cb, NULL);
	}

	return 0;
}

int ui_add_user(const char *name, bool online)
{
	static eHandle bn1;
	if (add_dlg == 0) {
		eHandle vbox, hbox, bn2, label;
		const echar *titles[1] = {_("user name")};
		add_dlg = egui_dialog_new();
		egui_request_resize(add_dlg, 350, 150);
		egui_box_set_border_width(add_dlg, 5);
		vbox = egui_vbox_new();
		egui_box_set_align(vbox, BoxAlignStart);
		egui_box_set_spacing(vbox, 5);
		egui_set_expand(vbox, true);
		egui_add(add_dlg, vbox);

		label = egui_simple_label_new(_("search result:"));
		egui_add(vbox, label);

		add_clist = egui_clist_new(titles, 1);
		egui_clist_title_hide(add_clist);
		e_signal_connect(add_clist, SIG_CLIST_DRAW_GRID_BK, add_clist_draw_bk);
		egui_add(vbox, add_clist);

		hbox = egui_hbox_new();
		egui_set_expand_h(hbox, true);
		egui_box_set_spacing(hbox, 10);
		egui_box_set_layout(hbox, BoxLayout_END);

		bn1 = egui_label_button_new(_("add"));
		e_signal_connect1(bn1, SIG_CLICKED, add_user_cb, (ePointer)add_clist);
		egui_add(hbox, bn1);
		bn2 = egui_label_button_new(_("cancel"));
		e_signal_connect1(bn2, SIG_CLICKED, clicked_hide, (ePointer)add_dlg);
		egui_add(hbox, bn2);

		egui_add(vbox, hbox);
	}
	egui_hide_async(search_dlg, false);
	egui_clist_empty(add_clist);
	if (name) {
		ClsItemBar *ibar = egui_clist_ibar_new_valist(add_clist, name);
		egui_clist_insert_ibar(add_clist, ibar);
	}
	egui_set_focus(bn1);
	egui_show_async(add_dlg, false);
	return 0;
}

static eint search_user_cb(eHandle hobj, ePointer data)
{
	echar buf[MAX_USERNAME];
	eint n = egui_get_text((eHandle)data, buf, MAX_USERNAME);
	buf[n] = 0;
	if (n < 1 || !isalpha(buf[0]))
		return -1;
	client_search_user(sfd, (char *)buf);
	return 0;
}

static eint cancel_cb(eHandle hobj, ePointer data)
{
	egui_hide(search_dlg, false);
	return 0;
}

static eint entry_hide_cb(eHandle hobj)
{
	egui_set_strings(hobj, _(""));
	return 0;
}

static eint clicked_bn_change_focus(eHandle hobj, ePointer data)
{
	GalEventKey ent;
	egui_set_focus((eHandle)data);
	ent.code = GAL_KC_Enter;
	e_signal_emit((eHandle)data, SIG_KEYDOWN, &ent);
	return 0;
}

static eint dlg_search_user(eHandle hobj, ePointer data)
{
	static eHandle entry = 0;
	if (search_dlg == 0) {
		eHandle vbox, vbox1, hbox2, label, bn1, bn2;

		search_dlg = egui_dialog_new();
		egui_request_resize(search_dlg, 350, 150);
		egui_box_set_border_width(search_dlg, 5);
		vbox = egui_vbox_new();
		egui_box_set_layout(vbox, BoxLayout_SPREAD);
		egui_set_expand(vbox, true);
		egui_add(search_dlg, vbox);

		vbox1 = egui_vbox_new();
		egui_box_set_align(vbox1, BoxAlignStart);
		egui_box_set_layout(vbox1, BoxLayout_CENTER);
		egui_set_expand(vbox1, true);

		label = egui_simple_label_new(_("entry username:"));
		egui_add(vbox1, label);
		entry = egui_entry_new(100);
		e_signal_connect(entry, SIG_HIDE, entry_hide_cb);
		egui_add(vbox1, entry);
		egui_set_max_text(entry, MAX_USERNAME);

		egui_add(vbox, vbox1);

		egui_add_spacing(vbox, 10);

		hbox2 = egui_hbox_new();
		egui_set_expand_h(hbox2, true);
		egui_box_set_spacing(hbox2, 10);
		egui_box_set_layout(hbox2, BoxLayout_END);
		bn1 = egui_label_button_new(_("search"));
		e_signal_connect1(bn1,   SIG_CLICKED, search_user_cb,          (ePointer)entry);
		e_signal_connect1(entry, SIG_CLICKED, clicked_bn_change_focus, (ePointer)bn1);
		egui_add(hbox2, bn1);
		bn2 = egui_label_button_new(_("cancel"));
		e_signal_connect1(bn2, SIG_CLICKED, cancel_cb, (ePointer)entry);
		egui_add(hbox2, bn2);

		egui_add(vbox, hbox2);

	}
	egui_set_focus(entry);
	egui_show(search_dlg, entry);
	return 0;
}

static GuiMenuEntry menu_entrys[] = {
	{ _("/Menu"),          _("<ALT>M"),  _("<Branch>"),       NULL,             NULL},
	{ _("/Menu/Find"),     _("<CTRL>F"), _("<StockItem>"),    dlg_search_user, "ctrl+n"},
	{ _("/Menu/Open"),     NULL,         _("<StockItem>"),    NULL,             NULL},
};

static eint open_chat_win(ClsItemBar *ibar, UserProfile *user)
{
	if (user->chat == 0) {
		user->chat = e_object_new(GTYPE_CHAT, GUI_WINDOW_POPUP);
		user->cwin = CHAT_WIN_DATA(user->chat);
		CHAT_WIN_DATA(user->chat)->user = user;
		egui_window_set_name(user->chat, user->name);
	}
	if (user->uhead) {
		while (user->uhead) {
			UnreadData *udata = user->uhead;
			user->uhead = udata->next;
			egui_text_append(user->cwin->view_text, udata->data, udata->size);
		}
		user->utail = NULL;
	}
	egui_show(user->chat, false);
	egui_move(user->chat, 500, 200);
	egui_request_resize(user->chat, 450, 450);
	if (user->timer) {
		e_timer_del(user->timer);
		user->timer = 0;
		user->count = 0;
	}

	return 0;
}

static eint clicked_open_chat_win(eHandle hobj, ePointer data)
{
	ClsItemBar  *ibar = data;
	UserProfile *user = ibar->add_data;
	return open_chat_win(ibar, user);
}

static eint menu_open_chat_win(eHandle hobj, ePointer data)
{
	ClsItemBar  *ibar = egui_clist_get_hlight(fri_clist);
	UserProfile *user = ibar->add_data;
	return open_chat_win(ibar, user);
}

static eint menu_delete_friend(eHandle hobj, ePointer data)
{
	ClsItemBar  *ibar = egui_clist_get_hlight(fri_clist);
	UserProfile *user = ibar->add_data;
	client_send_del_friend(sfd, (char *)user->name);
	//e_object_unref(user->chat);
	e_free(user);
	egui_clist_delete_hlight(fri_clist);
	return 0;
}

void *ui_friend_to_clist(const char *name, bool online, bool outlander)
{
	ClsItemBar*ibar = egui_clist_find(fri_clist, (ePointer)name);

	if (!ibar) {
		ibar = egui_clist_ibar_new_valist(fri_clist, name);
		UserProfile *user = e_calloc(1, sizeof(UserProfile));
		ibar->add_data    = user;
		user->online      = online;
		user->outlander   = outlander;
		e_strcpy(user->name, (const echar *)name);
		egui_clist_insert_ibar(fri_clist, ibar);
	}
	else {
		UserProfile *user = ibar->add_data;
		if (user->outlander != outlander || user->online != online) {
			user->outlander  = outlander;
			user->online     = online;
			egui_clist_update_ibar(fri_clist, ibar);
		}
	}

	return ibar;
}

void ui_change_online(const char *name, bool online)
{
	ClsItemBar  *ibar = egui_clist_find(fri_clist, (ePointer)name);
	UserProfile *user = ibar->add_data;
	user->online      = online;
	egui_clist_update_ibar(fri_clist, ibar);
}

/*
int ui_del_friend(const char *name)
{
	ClsItemBar  *ibar = egui_clist_find(fri_clist, (ePointer)name);
	if (ibar) {
		e_free(ibar->add_data);
		egui_clist_delete(fri_clist, ibar);
	}
	return 0;
}
*/

static eint recv_timer_cb(eTimer timer, euint n, ePointer data)
{
	ClsItemBar  *ibar = data;
	UserProfile *user = ibar->add_data;
	user->count += n;
	egui_clist_update_ibar(fri_clist, ibar);
	return 0;
}

void ui_recv_realmsg(const char *name, const char *msg)
{
	ClsItemBar *ibar = egui_clist_find(fri_clist, (ePointer)name);
	if (!ibar)
		ibar = ui_friend_to_clist(name, true, true);

	if (ibar) {
		UserProfile *user = ibar->add_data;
		if (user->cwin && GUI_STATUS_VISIBLE(user->chat)) {
			echar *buf = e_malloc(MAX_MESSAGE + MAX_USERNAME + 2);

			e_strcpy(buf, _(name ));
			e_strcat(buf, _(":\n"));
			e_strcat(buf, _(msg  ));
			egui_signal_emit2(user->cwin->view_text, SIG_RECV_MSG, 0, buf);
		}
		else {
			eint  size = strlen(name) + 2 + strlen(msg);
			echar *buf = e_malloc(size + 1);
			UnreadData *udata = e_calloc(1, sizeof(UnreadData));
			e_strcpy(buf, (echar *)name);
			e_strcat(buf, _(":\n"));
			e_strcat(buf, (const echar *)msg);
			udata->data = buf;
			udata->size = size;
			if (!user->uhead) {
				user->uhead = udata;
				user->utail = udata;
			}
			else {
				user->utail->next = udata;
				user->utail = udata;
			}
			if (user->timer == 0) {
				user->count =  0;
				user->timer = e_timer_add(500, recv_timer_cb, ibar);
			}
		}
	}
}

static eint frlist_find_user(eHandle hobj, ClsItemBar *ibar, ePointer data)
{
	UserProfile *user = ibar->add_data;
	if (!e_strcmp(user->name, data))
		return 0;
	return -1;
}

static void clist_grid_draw(eHandle hobj,
		GalDrawable drawable, GalPB pb, GalFont font,
		ClsItemBar *ibar, eint i, eint x, eint w, eint h)
{
	GalRect   rc = {x, 0, w, h};
	GalImage *icon = ibar->grids[i].icon;
	UserProfile *user = (UserProfile *)ibar->add_data;
	euint32 color;

	if (icon) {
		int oy;

		if (icon->h < h)
			oy = (h - icon->h) / 2;
		else
			oy = 0;
		if (icon->alpha)
			egal_composite_image(drawable, pb, 1, oy, icon, 0, 0, icon->w, icon->h);
		else
			egal_draw_image(drawable, pb, 1, oy, icon, 0, 0, icon->w, icon->h);
		rc.x  += icon->w + 3;
	}

	if (user->online)
		color = 0x0000ff;
	else
		color = 0;

	if (user->count % 2 == 0)
		egal_set_foreground(pb, color);
	else
		egal_set_foreground(pb, 0xffffff);

	egui_draw_strings(drawable, pb, font, ibar->grids[i].chars, &rc, LF_HLeft | LF_VCenter);
}

static GalImage *sys_img = NULL;
static eGeneType GTYPE_SYSBN = 0;
static eint sysmsg_bn_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	GuiButton *button = GUI_BUTTON_DATA(hobj);

	if (GUI_STATUS_FOCUS(hobj)) {
	}

	if (button->enter) {
		egal_fill_rect(wid->drawable, ent->pb,
				wid->offset_x, wid->offset_y,
				wid->rect.w, wid->rect.h);
	}

	if (sys_count % 2 == 0) {
		egal_composite_image(wid->drawable, wid->pb,
				wid->offset_x, wid->offset_y + (wid->min_h - sys_img->h) / 2,
				sys_img, 0, 0, sys_img->w, sys_img->h);
	}

	return 0;
}

static eint sysmsg_bn_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	sys_img = egal_image_new_from_file(_("./sysbn.png"));

	wid->min_w  = sys_img->w;
	wid->min_h  = sys_img->h;
	wid->max_w  = 1;
	wid->max_h  = 1;
	wid->rect.w = wid->min_w;
	wid->rect.h = wid->min_h;

	widget_set_status(wid, GuiStatusTransparent);
	widget_unset_status(wid, GuiStatusActive);

	return 0;
}

static eint sysmsg_bn_enter(eHandle hobj, eint x, eint y)
{
	GUI_BUTTON_DATA(hobj)->enter = true;
	return 0;
}

static eint sysmsg_bn_leave(eHandle hobj)
{
	GUI_BUTTON_DATA(hobj)->enter = false;
	return 0;
}

static void sysmsg_bn_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	w->expose         = sysmsg_bn_expose;
	c->init           = sysmsg_bn_init;
	e->leave          = sysmsg_bn_leave;
	e->enter          = sysmsg_bn_enter;
	e->focus_in       = NULL;
	e->focus_out      = NULL;
}

static eGeneType sysmsg_bn_genetype(void)
{
	eGeneInfo info = {
		0,
		sysmsg_bn_init_orders,
		0,
		NULL, NULL, NULL,
	};

	return e_register_genetype(&info, GTYPE_BUTTON, NULL);
}

typedef struct {
	const char *cmd;	
	char name[MAX_USERNAME];
	eHandle dlg;
	eHandle vbox;
	eHandle label;
	eHandle vbox1;
	eHandle hbox;
	eHandle radio1;
	eHandle radio2;
	eHandle radio3;
	eHandle bn1, bn2;
} AcceptDlgStruct;

static eint radio_clicked_cb(eHandle hobj, ePointer data)
{
	AcceptDlgStruct *ads = data;
	ads->cmd = e_signal_get_data2(hobj, SIG_CLICKED);
	return 0;
}

static eint accept_clicked_ok(eHandle hobj, ePointer data)
{
	AcceptDlgStruct *ads = data;
	client_accept_add_friend(sfd, ads->name, ads->cmd);
	e_signal_emit(ads->dlg, SIG_DESTROY);
	return 0;
}

static eint accept_clicked_cancel(eHandle hobj, ePointer data)
{
	AcceptDlgStruct *ads = data;
	e_signal_emit(ads->dlg, SIG_DESTROY);
	return 0;
}

static void destroy_accept_dlg(eHandle hobj)
{
	AcceptDlgStruct *ads = e_signal_get_data(hobj, SIG_DESTROY);
	e_object_unref(ads->dlg);
	e_free(ads);
}

static void sysmsg_accept_add(char *name)
{
	echar buf[100];
	AcceptDlgStruct *ads = e_malloc(sizeof(AcceptDlgStruct));

	ads->cmd = ACCEPTANDADD;
	strcpy(ads->name, name);

	ads->dlg = egui_dialog_new();
	e_signal_connect1(ads->dlg, SIG_DESTROY, destroy_accept_dlg, ads);
	egui_request_resize(ads->dlg, 350, 200);
	egui_box_set_border_width(ads->dlg, 5);
	ads->vbox = egui_vbox_new();
	egui_box_set_align(ads->vbox, BoxAlignStart);
	egui_box_set_spacing(ads->vbox, 5);
	egui_set_expand(ads->vbox, true);
	egui_add(ads->dlg, ads->vbox);

	egui_add_spacing(ads->vbox, 10);

	buf[0] = '\"';
	e_strcpy(buf + 1, (echar *)name);
	e_strcat(buf, _("\" request add friend"));
	ads->label = egui_simple_label_new(buf);
	egui_set_expand_h(ads->label, true);
	egui_add(ads->vbox, ads->label);

	ads->vbox1  = egui_vbox_new();
	egui_set_expand_h(ads->vbox1, true);
	egui_box_set_align(ads->vbox1, BoxAlignStart);
	egui_box_set_spacing(ads->vbox1, 5);
	ads->radio1 = egui_radio_button_new(_("accept and add"), 0);
	e_signal_connect2(ads->radio1, SIG_CLICKED, radio_clicked_cb, ads, ACCEPTANDADD);
	ads->radio2 = egui_radio_button_new(_("accept"), ads->radio1);
	e_signal_connect2(ads->radio2, SIG_CLICKED, radio_clicked_cb, ads, ACCEPTADD);
	ads->radio3 = egui_radio_button_new(_("refuse"), ads->radio1);
	e_signal_connect2(ads->radio3, SIG_CLICKED, radio_clicked_cb, ads, REFUSEADD);
	egui_add(ads->vbox1, ads->radio1);
	egui_add(ads->vbox1, ads->radio2);
	egui_add(ads->vbox1, ads->radio3);
	egui_add(ads->vbox, ads->vbox1);

	ads->hbox = egui_hbox_new();
	egui_set_expand_h(ads->hbox, true);
	egui_box_set_spacing(ads->hbox, 5);
	egui_box_set_layout(ads->hbox, BoxLayout_END);
	ads->bn1  = egui_label_button_new(_("OK"));
	e_signal_connect1(ads->bn1, SIG_CLICKED, accept_clicked_ok, ads);
	ads->bn2  = egui_label_button_new(_("CANCEL"));
	e_signal_connect1(ads->bn2, SIG_CLICKED, accept_clicked_cancel, ads);
	egui_add(ads->hbox, ads->bn1);
	egui_add(ads->hbox, ads->bn2);
	egui_add(ads->vbox, ads->hbox);

	egui_show_async(ads->dlg, false);
}

static void destroy_hide_dlg(eHandle hobj)
{
	egui_hide(hobj, false);
}

static void destroy_transfer_dlg(eHandle hobj)
{
	e_object_unref(hobj);
}

static eint clicked_accept_recv_file(eHandle hobj, ePointer data)
{
	eHandle        dlg = (eHandle)data;
	DataInfoHead   *dh = e_signal_get_data2(hobj, SIG_CLICKED);
	TransferStatus *ts = e_calloc(1, sizeof(TransferStatus));

	ClsItemBar *ibar;
	echar buf[20];

	if (dh->size > 1024 * 1024)
		e_sprintf(buf, _("%dM"), dh->size / (1024 * 1024));
	else if (dh->size > 1024)
		e_sprintf(buf, _("%dK"), dh->size / 1024);
	else
		e_sprintf(buf, _("%d"), dh->size);

	ibar = egui_clist_ibar_new_valist(transfer_clist,
			_("0"), NULL, NULL, buf, dh->name, dh->file);
	ts->cred   = dh->cred;
	ts->direct = DIRECT_RECV;
	ts->total_size = dh->size;
	ibar->add_data = ts;
	egui_signal_emit1(transfer_clist, SIG_CLIST_INSERT, ibar);
	egui_show_async(transfer_dlg, false);
	client_accept_transfer(sfd, dh, (const char *)save_path);

	e_signal_emit(dlg, SIG_DESTROY);
	return 0;
}

static eint clicked_refuse_recv_file(eHandle hobj, ePointer data)
{
	e_signal_emit((eHandle)data, SIG_DESTROY);
	return 0;
}

static void sysmsg_recv_file_dlg(ePointer data)
{
	DataInfoHead *dh = data;
	eHandle dlg;
	echar   buf[256];
	eHandle vbox, label, vbox1, hbox, bn1, bn2;

	dlg = egui_dialog_new();
	egui_box_set_border_width(dlg, 5);
	e_signal_connect(dlg, SIG_DESTROY, destroy_transfer_dlg);

	vbox = egui_vbox_new();
	egui_box_set_spacing(vbox, 5);
	egui_box_set_align(vbox, BoxAlignStart);
	egui_set_expand(vbox, true);
	egui_add(dlg, vbox);

	buf[0] = '\"';
	e_strcpy(buf + 1, _(dh->name));
	e_strcat(buf, _("\""));
	e_strcat(buf, _(" request to send files"));
	label = egui_simple_label_new(buf);
	egui_add(vbox, label);

	vbox1 = egui_vbox_new();
	egui_box_set_align(vbox1, BoxAlignStart);
	egui_set_expand(vbox, true);
	egui_add(vbox, vbox1);

	e_strcpy(buf, _("name: "));
	e_strcat(buf, _(dh->file));
	label = egui_simple_label_new(buf);
	egui_add(vbox1, label);

	e_strcpy(buf, _("size: "));
	if (dh->size > 1024 * 1024)
		e_sprintf(buf + 6, _("%uM"), dh->size / (1024 * 1024));
	else if (dh->size > 1024)
		e_sprintf(buf + 6, _("%uK"), dh->size / 1024);
	else
		e_sprintf(buf + 6, _("%u"), dh->size);
	label = egui_simple_label_new(buf);
	egui_add(vbox1, label);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_END);
	egui_box_set_spacing(hbox, 5);
	egui_set_expand_h(hbox, true);
	egui_add(vbox, hbox);

	bn1 = egui_label_button_new(_("Accept"));
	e_signal_connect2(bn1, SIG_CLICKED, clicked_accept_recv_file, (ePointer)dlg, dh);
	egui_add(hbox, bn1);
	bn2 = egui_label_button_new(_("Refuse"));
	e_signal_connect1(bn2, SIG_CLICKED, clicked_refuse_recv_file, (ePointer)dlg);
	egui_add(hbox, bn2);

	egui_request_resize(dlg, 300, 160);
	egui_show(dlg, false);
}

static eint sysmsg_bn_cb(eHandle hobj, ePointer data)
{
	if (sys_head) {
		SysmsgList *t = sys_head;

		if (t->type == SYSMSG_TYPE_ADD)
			sysmsg_accept_add(t->data);
		else if (t->type == SYSMSG_TYPE_TRANSFER)
			sysmsg_recv_file_dlg(t->data);

		sys_head = t->next;
		if (sys_head == NULL)
			sys_tail =  NULL;
		if (t->type == SYSMSG_TYPE_ADD)
			e_free(t->data);
		e_free(t);
	}

	if (!sys_head && sys_timer) {
		e_timer_del(sys_timer);
		sys_timer = 0;
		sys_count = 0;
		egui_update(hobj);
	}
	return 0;
}

static eint send_file_cb(eHandle hobj, ePointer data)
{
	UserProfile  *user = egui_clist_get_hlight(fri_clist)->add_data;
	GuiFilesel     *fs = GUI_FILESEL_DATA(hobj);
	TransferStatus *ts = e_calloc(1, sizeof(TransferStatus));
	const echar *filename;
	ClsItemBar  *ibar;

	eint size;
	echar path[MAX_PATH];

	e_strcpy(path, fs->path);
	filename = filesel_get_hlight_filename(hobj);
	e_strcat(path, filename);

	if (client_send_file(sfd, (char *)user->name, (char *)path, &size, &ts->cred) < 0) {
		e_free(ts);
		return -1;
	}

	if (size > 1024 * 1024)
		e_sprintf(path, _("%dM"), size / (1024 * 1024));
	else if (size > 1024)
		e_sprintf(path, _("%dK"), size / 1024);
	else
		e_sprintf(path, _("%d"), size);

	ibar = egui_clist_ibar_new_valist(transfer_clist, 
			_("0"), NULL, NULL, path, user->name, filename);
	ts->direct = DIRECT_SEND;
	ts->total_size = size;
	ibar->add_data = ts;
	egui_clist_insert_ibar(transfer_clist, ibar);
	egui_show_async(transfer_dlg, false);
	return 0;
}

void ui_set_transfer_size(void *cred, int size)
{
	ClsItemBar *ibar = egui_clist_find(transfer_clist, cred);
	if (ibar) {
		TransferStatus *ts = ibar->add_data;
		efloat fraction = size / (efloat)ts->total_size;
		eint   progress = (150. - 2) * fraction;
		if (progress != ts->progress_size) {
			ts->progress_size = progress;
			egui_clist_update_ibar(transfer_clist, ibar);
		}
	}
}

void ui_set_transfer_status(void *cred, int status)
{
	ClsItemBar *ibar = egui_clist_find(transfer_clist, cred);
	if (ibar) {
		TransferStatus *ts = ibar->add_data;
		ts->status = status;
		egui_clist_update_ibar(transfer_clist, ibar);
	}
}

void ui_request_file_notify(DataInfoHead *dh)
{
	SysmsgList *t = e_malloc(sizeof(SysmsgList));

	t->type = SYSMSG_TYPE_TRANSFER;
	t->data = dh;
	t->next = NULL;
	if (!sys_head) {
		sys_head = t;
		sys_tail = t;
	}
	else {
		sys_tail->next = t;
		sys_tail = t;
	}

	if (sys_timer == 0) {
		sys_count = 0;
		sys_timer = e_timer_add(500, system_timer_cb, NULL);
	}
}

static eint menu_send_file_cb(eHandle hobj, ePointer data)
{
	static eHandle filesel = 0;
	if (!filesel) {
		filesel = egui_filesel_new();
		egui_request_resize(filesel, 500, 400);
		egui_move(filesel, 400, 250);
		e_signal_connect(filesel, SIG_CLICKED, send_file_cb);
	}
	egui_show(filesel, false);
	return 0;
}

static void transfer_clist_draw_bk(eHandle hobj, GuiClist *cl,
		GalDrawable drawable, GalPB pb,
		int x, int w, int h, bool light, bool focus, int index)
{
	if (light) {
		if (focus)
			egal_set_foreground(pb, 0xc4c4ff);
		else
			egal_set_foreground(pb, 0xc4c4c4);
	}
	else
		egal_set_foreground(pb, 0xffffff);

	egal_fill_rect(drawable, pb, x, 0, w, h);
}

static void transfer_clist_title(eHandle hobj, GuiClist *cl, GalDrawable drawable, GalPB pb)
{
	eint i, x;

	egal_set_foreground(pb, 0x8fb6fb);
	egal_fill_rect(drawable, pb, 0, 0, cl->view_w, cl->tbar_h);

	egal_set_foreground(pb, 0x0);
	for (x = 0, i = 0; i < cl->col_n; i++) {
		ClsTitle *title = cl->titles + i;
		GalRect rc = {x, 0, title->width, cl->tbar_h};

		if (i == 3 || i == 4)
			rc.x += 10;

		egui_draw_strings(drawable, pb, 0, title->name, &rc, LF_VCenter | LF_HLeft);
		x += title->width;
	}
}

static void transfer_clist_draw(eHandle hobj,
		GalDrawable drawable, GalPB pb, GalFont font,
		ClsItemBar *ibar, eint i, eint x, eint w, eint h)
{
	GalRect rc = {x, 0, w, h};
	TransferStatus *ts = ibar->add_data;

	egal_set_foreground(pb, 0x0);
	if (i == 0) {
		egui_draw_strings(drawable, pb, font, ibar->grids[i].chars, &rc, LF_HLeft | LF_VCenter);
	}
	else if (i == 1) {
	}
	else if (i == 2) {
		egal_set_foreground(pb, 0x708199);
		egal_draw_rect(drawable, pb, x, 0, w, h);
		if (ts->progress_size > 0) {
			egal_set_foreground(pb, 0x0000ff);
			egal_fill_rect(drawable, pb, x + 1, 10, ts->progress_size, h - 20);
		}
	}
	else if (i == 3 || i == 4) {
		rc.x += 10;
		egui_draw_strings(drawable, pb, font, ibar->grids[i].chars, &rc, LF_HLeft | LF_VCenter);
	}
	else if (i == 5) {
		egui_draw_strings(drawable, pb, font, ibar->grids[i].chars, &rc, LF_HLeft | LF_VCenter);
	}
}

static eint transfer_find_cb(eHandle hobj, ClsItemBar *ibar, ePointer data)
{
	TransferStatus *status = ibar->add_data;
	if (!e_memcmp(&status->cred, data, sizeof(Cred)))
		return 0;
	return -1;
}

static eHandle transfer_file_dlg_new(void)
{
	eHandle dlg, vbox, hbox, bn;
	const echar *titles[6] = {_("d"), _("s"), _("prossce"), _("size"), _("user"), _("filename")};

	dlg  = egui_dialog_new();
	e_signal_connect(dlg, SIG_DESTROY, destroy_hide_dlg);
	vbox = egui_vbox_new();
	egui_box_set_spacing(vbox, 5);
	egui_set_expand(vbox, true);
	egui_add(dlg, vbox);

	transfer_clist = egui_clist_new(titles, 6);
	egui_clist_set_column_width(transfer_clist, 0, 30);
	egui_clist_set_column_width(transfer_clist, 1, 20);
	egui_clist_set_column_width(transfer_clist, 2, 150);
	egui_clist_set_column_width(transfer_clist, 3, 80);
	egui_clist_set_column_width(transfer_clist, 4, 100);

	egui_clist_set_item_height(transfer_clist, 35);
	e_signal_connect(transfer_clist, SIG_CLIST_DRAW_TITLE,   transfer_clist_title);
	e_signal_connect(transfer_clist, SIG_CLIST_DRAW_GRID,    transfer_clist_draw);
	e_signal_connect(transfer_clist, SIG_CLIST_DRAW_GRID_BK, transfer_clist_draw_bk);
	e_signal_connect(transfer_clist, SIG_CLIST_FIND,         transfer_find_cb);
	egui_add(vbox, transfer_clist);
	
	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_END);
	egui_set_expand_h(hbox, true);
	egui_add(vbox, hbox);

	bn = egui_label_button_new(_("close"));	
	e_signal_connect1(bn, SIG_CLICKED, clicked_hide, (ePointer)dlg);
	egui_add(hbox, bn);
	egui_request_resize(dlg, 500, 400);

	return dlg;
}

static eint main_window(const char *name, const char *passwd)
{
	eHandle win, vbox, bar, menu, item, hbox;
	const echar *titles[1] = {_("")};

	sfd = client_login(name, passwd);
	if (sfd < 0)
		return -1;

	e_strcpy(save_path, _("./"));

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	SIG_RECV_MSG = e_signal_new1("recv_msg", GTYPE_TEXT, "%d %p");

	egui_window_set_name(win, _(name));
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_box_set_spacing(vbox, 5);
	egui_add(win, vbox);

	bar  = menu_bar_new_with_entrys(win, menu_entrys, sizeof(menu_entrys) / sizeof(GuiMenuEntry));

	egui_add(vbox, bar);

	menu = egui_menu_new(win, NULL);
	item = egui_menu_item_new_with_label(_("Send Message"));
	e_signal_connect(item, SIG_CLICKED, menu_open_chat_win);
	egui_add(menu, item);
	item = egui_menu_item_new_with_label(_("Send File"));
	e_signal_connect(item, SIG_CLICKED, menu_send_file_cb);
	egui_add(menu, item);
	item = egui_menu_item_new_with_label(_("Delete Friend"));
	e_signal_connect(item, SIG_CLICKED, menu_delete_friend);
	egui_add(menu, item);

	fri_clist = egui_clist_new(titles, 1);
	GUI_CLIST_DATA(fri_clist)->menu = menu;
	e_signal_connect(fri_clist, SIG_CLIST_DRAW_GRID, clist_grid_draw);
	egui_set_min(fri_clist, 150, 200);
	e_signal_connect(fri_clist, SIG_CLIST_FIND, frlist_find_user);
	e_signal_connect(fri_clist, SIG_2CLICKED,   clicked_open_chat_win);
	egui_clist_title_hide(fri_clist);
	egui_request_resize(fri_clist, 150, 400);
	egui_add(vbox, fri_clist);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_START);
	egui_set_expand_h(hbox, true);
	egui_add(vbox, hbox);
	egui_add_spacing(vbox, 5);

	GTYPE_SYSBN = sysmsg_bn_genetype();
	sys_bn = e_object_new(GTYPE_SYSBN);
	e_signal_connect(sys_bn, SIG_CLICKED, sysmsg_bn_cb);
	egui_add(hbox, sys_bn);

	client_search_friend(sfd);

	client_read_thread(sfd);

	transfer_dlg = transfer_file_dlg_new();

	return 0;
}

static eint clicked_login_account(eHandle hobj, ePointer data)
{
	eHandle dlg    = (eHandle)data;
	eHandle entry1 = (eHandle)e_signal_get_data2(hobj, SIG_CLICKED);
	eHandle entry2 = (eHandle)e_signal_get_data3(hobj, SIG_CLICKED);
	echar name[MAX_USERNAME];
	echar passwd[MAX_USERNAME];
	eint n = egui_get_text(entry1, name, MAX_USERNAME);
	name[n] = 0;
	n = egui_get_text(entry2, passwd, MAX_USERNAME);
	passwd[n] = 0;
	if (main_window((char *)name, (char *)passwd) < 0)
		return -1;
	e_object_unref(dlg);
	return 0;
}

static eint clicked_login_close(eHandle hobj, ePointer data)
{
	return e_signal_emit((eHandle)data, SIG_DESTROY);
}

static eint clicked_change_focus(eHandle hobj, ePointer data)
{
	egui_set_focus((eHandle)data);
	return 0;
}

static void login_account_dlg(void)
{
	eHandle dlg, vbox, frame, vbox1, hbox, label, entry1, entry2, bn;

	dlg = egui_window_new(GUI_WINDOW_TOPLEVEL);
	egui_box_set_border_width(dlg, 5);
	e_signal_connect(dlg, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_spacing(vbox, 5);
	egui_set_expand(vbox, true);
	egui_add(dlg, vbox);

	frame = egui_frame_new(_(""));
	egui_set_expand(frame, true);
	egui_add(vbox, frame);

	vbox1 = egui_vbox_new();
	egui_box_set_spacing(vbox1, 10);
	egui_set_expand(vbox1, true);
	egui_add(frame, vbox1);

	hbox = egui_hbox_new();
	egui_set_expand_h(hbox, true);
	egui_add(vbox1, hbox);

	label = egui_simple_label_new(_("username:"));
	egui_add(hbox, label);
	entry1 = egui_entry_new(200);
	egui_set_max_text(entry1, 18);
	egui_add(hbox, entry1);

	hbox = egui_hbox_new();
	egui_set_expand_h(hbox, true);
	egui_add(vbox1, hbox);

	label  = egui_simple_label_new(_("password:"));
	egui_add(hbox, label);
	entry2 = egui_entry_new(200);
	egui_entry_set_visibility(entry2, false);
	egui_set_max_text(entry2, 18);
	egui_add(hbox, entry2);
	e_signal_connect1(entry1, SIG_CLICKED, clicked_change_focus, (ePointer)entry2);

	hbox = egui_hbox_new();
	egui_box_set_layout (hbox, BoxLayout_END);
	egui_box_set_spacing(hbox, 5);
	egui_set_expand_h(hbox, true);
	egui_add(vbox, hbox);

	bn = egui_label_button_new(_("login"));
	e_signal_connect1(entry2, SIG_CLICKED, clicked_bn_change_focus, (ePointer)bn);
	e_signal_connect3(bn, SIG_CLICKED, clicked_login_account, (ePointer)dlg, (ePointer)entry1, (ePointer)entry2);
	egui_add(hbox, bn);
	bn = egui_label_button_new(_("cancel"));
	e_signal_connect1(bn, SIG_CLICKED, clicked_login_close, (ePointer)dlg);
	egui_add(hbox, bn);
	egui_add_spacing(hbox, 15);

	egui_set_focus(entry1);
	egui_show(dlg, false);
}

int main(int argc, char* const argv[])
{
	egui_init(argc, argv);

	login_account_dlg();

	egui_main();

	return 0;
}
