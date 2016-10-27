#include <sys/types.h>
#include <sys/stat.h>
#ifdef linux
#include <unistd.h>
#endif

#include <egal/egal.h>

#include "res.h"

eHandle egui_res_handle = 0;
static const echar *res_path = NULL;

elong egui_init_res(void)
{
	return 0;
}

GuiResItem *egui_res_find(eHandle hobj, const echar *name)
{
	if (!e_strcmp(name, _("common")))
		return (GuiResItem *)0;
	else if (!e_strcmp(name, _("vscrollbar")))
		return (GuiResItem *)1;
	else if (!e_strcmp(name, _("hscrollbar")))
		return (GuiResItem *)2;
	else if (!e_strcmp(name, _("listview")))
		return (GuiResItem *)3;
	else if (!e_strcmp(name, _("menu")))
		return (GuiResItem *)4;
	else if (!e_strcmp(name, _("filesel")))
		return (GuiResItem *)5;
	else if (!e_strcmp(name, _("button")))
		return (GuiResItem *)6;

	return (GuiResItem *)-1;
}

#ifdef WIN32

ePointer egui_res_find_item(GuiResItem *item, const echar *name)
{
	echar path[256];
	struct stat st;

	if (res_path) {
		e_strcpy(path, res_path);
	}
	else if (stat("res", &st) == 0) {
		res_path = _("res\\");
		e_strcpy(path, res_path);
	}
	else {
		char *p = getenv("HOME");
		if (p)
			e_strcpy(path, (echar *)p);
		else
			path[0] = 0;
		e_strcat(path, _("\\egui\\res\\"));
		res_path = e_strdup(path);
	}

	if (((elong)item) == 0)
		e_strcat(path, _("common\\"));
	else if (((elong)item) == 1)
		e_strcat(path, _("v\\"));
	else if (((elong)item) == 2)
		e_strcat(path, _("h\\"));
	else if (((elong)item) == 3)
		e_strcat(path, _("listview\\"));
	else if (((elong)item) == 4)
		e_strcat(path, _("menu\\"));
	else if (((elong)item) == 5)
		e_strcat(path, _("filesel\\"));
	else if (((elong)item) == 6)
		e_strcat(path, _("button\\"));

	e_strcat(path, name);
	if (((elong)item) == 4
			||  ((elong)item) == 0
			||  ((elong)item) == 5
			|| (((elong)item) == 6 && e_strstr(name, _("radio"))))
		e_strcat(path, _(".png"));
	else
		e_strcat(path, _(".bmp"));

	return egal_image_new_from_file(path);
}

#else

ePointer egui_res_find_item(GuiResItem *item, const echar *name)
{
	echar path[256];
	struct stat st;

	if (res_path) {
		e_strcpy(path, res_path);
	}
	else if (stat("./res/", &st) == 0) {
		res_path = _("./res");
		e_strcpy(path, res_path);
	}
	else {
		e_strcpy(path, _(getenv("HOME")));
		e_strcat(path, _("/.egui/res/"));
		if (stat((const char *)path, &st) < 0) {
			e_strcpy(path, _(getenv("HOME")));
			e_strcat(path, _("/egui/egui/res/"));
		}
		res_path = e_strdup(path);
	}
	
	if (((elong)item) == 0)
		e_strcat(path, _("common/"));
	else if (((elong)item) == 1)
		e_strcat(path, _("v/"));
	else if (((elong)item) == 2)
		e_strcat(path, _("h/"));
	else if (((elong)item) == 3)
		e_strcat(path, _("listview/"));
	else if (((elong)item) == 4)
		e_strcat(path, _("menu/"));
	else if (((elong)item) == 5)
		e_strcat(path, _("filesel/"));
	else if (((elong)item) == 6)
		e_strcat(path, _("button/"));

	e_strcat(path, name);
	if (((elong)item) == 4
			||  ((elong)item) == 0
			||  ((elong)item) == 5
			|| (((elong)item) == 6 && e_strstr(name, _("radio"))))
		e_strcat(path, _(".png"));
	else
		e_strcat(path, _(".bmp"));

	return egal_image_new_from_file(path);
}
#endif
