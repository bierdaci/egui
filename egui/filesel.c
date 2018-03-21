#include "gui.h"
#include "widget.h"
#include "event.h"
#include "bin.h"
#include "box.h"
#include "window.h"
#include "label.h"
#include "entry.h"
#include "clist.h"
#include "button.h"
#include "menu.h"
#include "ddlist.h"
#include "filesel.h"
#include "res.h"

static eint filesel_init_data(eHandle, ePointer);
static void filesel_init_orders(eGeneType, ePointer);
static void sepbar_init_orders(eGeneType, ePointer);

static eGeneType GTYPE_SEPBAR;
static GalImage *icon_dir;

eGeneType egui_genetype_filesel(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GuiResItem *res;

		eGeneInfo info = {
			0,
			filesel_init_orders,
			sizeof(GuiFilesel),
			NULL,
			NULL,
		};

		eGeneInfo sepbar_info = {
			0,
			sepbar_init_orders,
			0,
			NULL, NULL, NULL,
		};

		GTYPE_SEPBAR = e_register_genetype(&sepbar_info, GTYPE_INOUT_WIDGET, NULL);

		gtype = e_register_genetype(&info, GTYPE_DIALOG, NULL);

		res      = egui_res_find(GUI_RES_HANDLE, _("filesel"));
		icon_dir = egui_res_find_item(res, _("dir"));
	}
	return gtype;
}

static eint sepbar_enter(eHandle hobj, eint x, eint y)
{
	GuiFilesel *fs = GUI_WIDGET_DATA(hobj)->extra_data;
	egal_set_cursor(GUI_WIDGET_DATA(hobj)->drawable, fs->cursor);
	return 0;
}

static eint sepbar_leave(eHandle hobj)
{
	egal_set_cursor(GUI_WIDGET_DATA(hobj)->drawable, 0);
	return 0;
}

static eint sepbar_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GuiFilesel *fs = GUI_WIDGET_DATA(hobj)->extra_data;
	fs->grab  = etrue;
	egal_grab_pointer(GUI_WIDGET_DATA(hobj)->drawable, efalse, fs->cursor);
	return 0;
}

static eint sepbar_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	GuiFilesel *fs = GUI_WIDGET_DATA(hobj)->extra_data;
	fs->grab = efalse;
	egal_ungrab_pointer(GUI_WIDGET_DATA(hobj)->drawable);
	return 0;
}

static eint sepbar_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	return 0;
}

static eint sepbar_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	wid->min_w  = 7;
	wid->max_w  = 7;
	wid->rect.w = 7;
	widget_set_status(wid, GuiStatusVisible | GuiStatusTransparent | GuiStatusMouse);
	return 0;
}

static void sepbar_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	c->init   = sepbar_init;
	w->expose = sepbar_expose;

	e->enter = sepbar_enter;
	e->leave = sepbar_leave;
	e->lbuttonup   = sepbar_lbuttonup;
	e->lbuttondown = sepbar_lbuttondown;
}

static void add_hlight_file_list(GuiFilesel *fs, const echar *path, ebool empty)
{
	DIR *dir;
	struct dirent *ent;
	ClsItemBar *ibar;

	if (!(dir = e_opendir(path)))
		return;

	fs->len = e_strlen(path);
	if (fs->path != path)
		e_strcpy(fs->path, path);
#ifdef linux
	if (fs->path[fs->len - 1] != '/') {
		e_strcat(fs->path, _("/"));
		fs->len++;
	}
#elif WIN32
	if (fs->path[fs->len - 1] != '\\') {
		e_strcat(fs->path, _("\\"));
		fs->len++;
	}
#endif
	egui_set_strings(fs->addr, fs->path);
	if (empty)
		egui_clist_empty(fs->clist);

	while ((ent = e_readdir(dir))) {
#ifdef linux
		if (ent->d_name[0] == '.' &&
				(!ent->d_name[1] || ent->d_name[1] == '.' || !fs->is_hide))
			continue;
#endif
		ibar = egui_clist_ibar_new_valist(fs->clist, (echar *)ent->d_name, _(" "));
		ibar->add_data = (ePointer)(ent->d_type == DT_DIR);
		if (ibar->add_data)
			ibar->grids[0].icon = icon_dir;
		egui_clist_insert_ibar(fs->clist, ibar);
	}
	egui_clist_set_hlight(fs->clist, egui_clist_get_first(fs->clist));

	e_closedir(dir);
}

static eint (*bin_lbuttondown)(eHandle, GalEventMouse *);
static eint (*bin_mousemove)(eHandle, GalEventMouse *);
static void (*bin_show)(eHandle);
static void (*bin_hide)(eHandle);
static void filesel_show(eHandle hobj)
{
	GuiFilesel *fs  = GUI_FILESEL_DATA(hobj);
	GuiWidget  *wid = GUI_WIDGET_DATA(hobj);
	ClsItemBar *bar = egui_clist_get_hlight(fs->locat_list);

	if (bar)
		add_hlight_file_list(fs, bar->add_data, efalse);

	bin_show(hobj);
	egal_window_show(wid->window);
}

static void filesel_hide(eHandle hobj)
{
	GuiFilesel *fs = GUI_FILESEL_DATA(hobj);
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);

	bin_hide(hobj);

	if (fs->is_hide)
		fs->is_hide = efalse;

	egal_window_hide(wid->window);

	egui_clist_empty(fs->clist);
	egui_clist_set_hlight(fs->locat_list, egui_clist_get_first(fs->locat_list));
}

static eint filesel_mousemove(eHandle hobj, GalEventMouse *ent)
{
	GuiFilesel *fs = GUI_FILESEL_DATA(hobj);
	if (fs->grab) {
		GuiWidget *fwid = GUI_WIDGET_DATA(hobj);
		GuiWidget *lwid = GUI_WIDGET_DATA(fs->locat_list);
		GuiWidget *cwid = GUI_WIDGET_DATA(fs->clist);
		eint min_w  = lwid->min_w + (ent->point.x - fs->old_x);
		if (min_w < 50)
			min_w = 50;
		if (min_w + cwid->min_w > fwid->rect.w - 20)
			min_w = fwid->rect.w - cwid->min_w - 20;
		if (min_w != lwid->min_w) {
			egui_set_min(fs->locat_list, min_w, 0);
			egui_request_layout(fs->locat_list, 0, 0, 0, etrue, efalse);
			fs->old_x = ent->point.x;
		}
		return 0;
	}
	return bin_mousemove(hobj, ent);
}

static eint filesel_lbuttondown(eHandle hobj, GalEventMouse *ent)
{
	GuiFilesel *fs = GUI_FILESEL_DATA(hobj);
	fs->old_x = ent->point.x;
	return bin_lbuttondown(hobj, ent);
}

static eint (*cell_init)(eHandle, eValist);
static eint filesel_init(eHandle hobj, eValist va)
{
	cell_init(hobj, va);

	return filesel_init_data(hobj, NULL);
}

static void filesel_destroy(eHandle hobj)
{
	egui_hide(hobj, efalse);
}

static void filesel_init_orders(eGeneType new, ePointer this)
{
	GuiWidgetOrders *w = e_genetype_orders(new, GTYPE_WIDGET);
	GuiEventOrders  *e = e_genetype_orders(new, GTYPE_EVENT);
	eCellOrders     *c = e_genetype_orders(new, GTYPE_CELL);

	bin_mousemove   = e->mousemove;
	bin_lbuttondown = e->lbuttondown;
	e->mousemove    = filesel_mousemove;
	e->lbuttondown  = filesel_lbuttondown;

	bin_show   = w->show;
	bin_hide   = w->hide;
	w->show    = filesel_show;
	w->hide    = filesel_hide;
	w->destroy = filesel_destroy;

	cell_init  = c->init;
	c->init    = filesel_init;
}

static eint cancel_bn_clicked(eHandle hobj, ePointer data)
{
	egui_hide((eHandle)data, etrue);
	return 0;
}

static eint ok_bn_clicked(eHandle hobj, ePointer data)
{
	if (e_signal_emit((eHandle)data, SIG_CLICKED, 
				e_signal_get_data((eHandle)data, SIG_CLICKED)) < 0)
		return -1;
	egui_hide((eHandle)data, etrue);
	return 0;
}

static void clist_grid_draw_bk(eHandle hobj, GuiClist *cl,
		GalDrawable drawable, GalPB pb,
		int x, int w, int h, ebool light, ebool focus, int index)
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

static eint file_name_cmp(eHandle hobj, ePointer data, ClsItemBar *ibar, ClsItemBar *new)
{
	if (ibar->add_data && !new->add_data)
		return -1;
	else if (!ibar->add_data && new->add_data)
		return 1;
	return e_strcasecmp(ibar->grids[0].chars, new->grids[0].chars);
}

static eint select_dir_func(eHandle hobj, ePointer data)
{
	GuiFilesel *fs = e_signal_get_data(hobj, SIG_CLICKED);
	ClsItemBar *ibar = data;
	add_hlight_file_list(fs, ibar->add_data, etrue);
	return 0;
}

static eint open_dir_func(eHandle hobj, ePointer data)
{
	GuiFilesel *fs = e_signal_get_data(hobj, SIG_2CLICKED);
	ClsItemBar *ibar = data;

	if (hobj == fs->locat_list) {
		if (e_strcmp(ibar->add_data, fs->path))
			add_hlight_file_list(fs, ibar->add_data, etrue);
	}
	else if (ibar->add_data) {
		echar path[256];
		e_strcpy(path , fs->path);
		e_strcat(path, ibar->grids[0].chars);
		add_hlight_file_list(fs, path, etrue);
	}

	return 0;
}

static eint up_dir_func(eHandle hobj, ePointer data)
{
	GuiFilesel *fs = data;
	if (fs->len > 1) {
		echar *p;
#ifdef linux
		if (fs->path[fs->len - 1] == '/')
			fs->path[fs->len - 1] = 0;
		p = e_strrchr(fs->path, '/');
#elif WIN32
		if (fs->path[fs->len - 1] == '\\')
			fs->path[fs->len - 1] = 0;
		p = e_strrchr(fs->path, '\\');
#endif
		if (p == fs->path)
			p[1] = 0;
		else
			p[0] = 0;
		add_hlight_file_list(fs, fs->path, etrue);
	}
	return 0;
}

static eint hide_file_func(eHandle hobj, ePointer data)
{
	GuiFilesel *fs = data;
	fs->is_hide = !fs->is_hide;
	add_hlight_file_list(fs, fs->path, etrue);
	return 0;
}

static eint filesel_init_data(eHandle hobj, ePointer this)
{
	GuiFilesel  *fs = GUI_FILESEL_DATA(hobj);
	const echar *titles[2]   = {_("name"), _("date")};
	const echar *location[1] = {_("location")};
	ClsItemBar  *ibar;

	egui_unset_status(hobj, GuiStatusVisible);
	egui_box_set_border_width(hobj, 5);

	fs->vbox = egui_vbox_new();
	egui_set_expand(fs->vbox, etrue);

	fs->addr_hbox = egui_hbox_new();
	egui_set_expand_h(fs->addr_hbox, etrue);
	fs->up_bn = egui_button_new(20, 20);
	e_signal_connect1(fs->up_bn, SIG_CLICKED, up_dir_func, fs);
	egui_add(fs->addr_hbox, fs->up_bn);
	egui_add_spacing(fs->addr_hbox, 5);

	fs->location = egui_label_new(_("Path:"));
	egui_set_fg_color(fs->location, 0xffffff);
	egui_set_bg_color(fs->location, 0x3c3b37);
	egui_set_expand_h(fs->location, efalse);
	egui_add(fs->addr_hbox, fs->location);
	fs->addr = egui_entry_new(100);
	egui_add(fs->addr_hbox, fs->addr);

	fs->clist_hbox = egui_hbox_new();
	egui_set_expand(fs->clist_hbox, etrue);
	fs->locat_list = egui_clist_new(location, 1);
	egui_clist_set_item_height(fs->locat_list, 50);
	e_signal_connect1(fs->locat_list, SIG_CLICKED,  select_dir_func, fs);
	e_signal_connect1(fs->locat_list, SIG_2CLICKED, open_dir_func, fs);
	e_signal_connect(fs->locat_list, SIG_CLIST_DRAW_GRID_BK, clist_grid_draw_bk);
	ibar = egui_clist_append_valist(fs->locat_list, _("Home"));
	ibar->add_data = e_strdup((echar *)getenv("HOME"));
	{
		eint size = e_strlen(ibar->add_data);
		ibar->add_data = e_realloc(ibar->add_data, size + 2);
#ifdef linux
		e_strcat(ibar->add_data, _("/"));
#elif WIN32
		e_strcat(ibar->add_data, _("\\"));
#endif
	}
	ibar = egui_clist_append_valist(fs->locat_list, _("Root"));
#ifdef linux
	ibar->add_data = "/";
#elif WIN32
	ibar->add_data = "\\";
#endif
	egui_set_min(fs->locat_list, 100, 50);
	egui_request_resize(fs->locat_list, 150, 100);
	egui_set_expand_h(fs->locat_list, efalse);
	egui_add(fs->clist_hbox, fs->locat_list);

	fs->sepbar = e_object_new(GTYPE_SEPBAR);
	fs->cursor = egal_cursor_new(GAL_SB_H_DOUBLE_ARROW);
	egui_set_extra_data(fs->sepbar, fs);
	egui_add(fs->clist_hbox, fs->sepbar);

	fs->clist = egui_clist_new(titles, 2);
	e_signal_connect1(fs->clist, SIG_CLIST_CMP, file_name_cmp, fs);
	e_signal_connect1(fs->clist, SIG_2CLICKED,  open_dir_func, fs);
	egui_set_min(fs->clist, 50, 50);
	egui_request_resize(fs->clist, 200, 100);
	egui_add(fs->clist_hbox, fs->clist);

	fs->dl_hbox = egui_hbox_new();
	egui_set_expand_h(fs->dl_hbox, etrue);

	fs->hide_bn = egui_button_new(25, 25);
	e_signal_connect1(fs->hide_bn, SIG_CLICKED, hide_file_func, fs);
	egui_add(fs->dl_hbox, fs->hide_bn);

	fs->ddlist  = egui_ddlist_new();
	egui_add(fs->dl_hbox, fs->ddlist);
	egui_add(fs->ddlist, egui_menu_item_new_with_label(_("all file")));
	egui_add(fs->ddlist, egui_menu_item_new_with_label(_("all text file")));

	fs->bn_hbox = egui_hbox_new();
	egui_box_set_layout (fs->bn_hbox, BoxLayout_END);
	egui_box_set_spacing(fs->bn_hbox, 20);
	egui_set_expand_h   (fs->bn_hbox, etrue);
	fs->ok_bn     = egui_label_button_new(_("open"));
	fs->cancel_bn = egui_label_button_new(_("cancel"));
	egui_add(fs->bn_hbox, fs->cancel_bn);
	egui_add(fs->bn_hbox, fs->ok_bn);
	e_signal_connect1(fs->ok_bn, SIG_CLICKED, ok_bn_clicked, (ePointer)hobj);
	e_signal_connect1(fs->cancel_bn, SIG_CLICKED, cancel_bn_clicked, (ePointer)hobj);

	egui_add(fs->vbox, fs->addr_hbox);
	egui_add_spacing(fs->vbox, 5);
	egui_add(fs->vbox, fs->clist_hbox);
	egui_add_spacing(fs->vbox, 5);
	egui_add(fs->vbox, fs->dl_hbox);
	egui_add_spacing(fs->vbox, 10);
	egui_add(fs->vbox, fs->bn_hbox);

	egui_add(hobj, fs->vbox);

	return 0;
}

const echar *filesel_get_hlight_filename(eHandle hobj)
{
	GuiFilesel *fs = GUI_FILESEL_DATA(hobj);
	return (const echar *)clist_get_grid_data(fs->clist, egui_clist_get_hlight(fs->clist), 0);
}

eHandle egui_filesel_new(void)
{
	return e_object_new(GTYPE_FILESEL);
}

