#include <stdio.h>
#include <egui/egui.h>

static eHandle menu1_new(eHandle);

static eHandle menu1_level1_new(void);

static eint button_clicked(eHandle hobj, ePointer data)
{
	egui_menu_popup((eHandle)data);
	return 0;
}

static eint test_AccelKeyCB(eHandle hobj, ePointer data)
{
	printf("....................%x\n", hobj);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, bar, button, vbox;
	eHandle root1, root2, root3;
	eHandle menu1, menu2;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	egui_accelkey_connect(win, _("<Ctrl><shift>A"), test_AccelKeyCB, 0);

	bar    = egui_menu_bar_new();
	button = egui_button_new(200, 50);
	root1  = egui_menu_item_new_with_label(_("Root Menu"));
	egui_add(bar, root1);
	root2  = egui_menu_item_new_with_label(_("Bar Menu"));
	root3  = egui_menu_item_new_with_label(_("System"));

	egui_add(vbox, bar);
	egui_add(vbox, button);

	egui_add(bar, root2);
	egui_add(bar, root3);

	menu1 = menu1_new(win);
	egui_menu_item_set_submenu(root1, menu1);
	e_signal_connect1(button, SIG_CLICKED, button_clicked, (ePointer)menu1);

	menu2 = menu1_new(win);
	egui_menu_item_set_submenu(root2, menu2);

	menu2 = menu1_new(win);
	egui_menu_item_set_submenu(root3, menu2);

	egui_main();

	return 0;
}

static eHandle menu1_level21_new(void)
{
	eHandle menu, item;

	menu = egui_menu_new(0, NULL);

	item = egui_menu_item_new_with_label(_("aaaa"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("ahasdfxin as"));
	egui_add(menu, item);

	return menu;
}

static eHandle menu1_level22_new(void)
{
	eHandle menu, item;

	menu = egui_menu_new(0, NULL);

	item = egui_menu_item_new_with_label(_("zhang xin hua"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("gao di"));
	egui_add(menu, item);

	return menu;
}

static eHandle menu1_level1_new(void)
{
	eHandle menu, item;

	menu = egui_menu_new(0, NULL);

	item = egui_menu_item_new_with_label(_("_New file"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("_Open file"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("_Save"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("_Save as"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("_Copy"));
	egui_add(menu, item);
	egui_menu_item_set_submenu(item, menu1_level21_new());

	item = egui_menu_item_new_with_label(_("_Paste"));
	egui_add(menu, item);
	//egui_unset_status(item, GuiStatusActive | GuiStatusMouse);
	egui_menu_item_set_submenu(item, menu1_level22_new());

	item = egui_menu_item_new_with_label(_("_Cut"));
	egui_add(menu, item);

	return menu;
}

static eHandle menu1_new(eHandle win)
{
	eHandle menu, item, radio;
	eint i;

	menu  = egui_menu_new(win, NULL);
	item = egui_menu_item_new_with_label(_("New file"));
	egui_add(menu, item);

	egui_menu_item_set_submenu(item, menu1_level1_new());

	item = egui_menu_item_new_with_label(_("Open file"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("Save"));
	egui_add(menu, item);

	item = egui_menu_item_new_with_label(_("Save as"));
	egui_add(menu, item);
	egui_menu_item_set_check(item, true);

	item = egui_menu_item_new_with_label(_("Copy"));
	egui_add(menu, item);
	radio = item;
	egui_menu_item_set_radio_group(radio, 0);

	item = egui_menu_item_new_with_label(_("Paste"));
	egui_add(menu, item);
	egui_menu_item_set_radio_group(item, radio);

	item = egui_menu_item_new_with_label(_("Cut"));
	egui_add(menu, item);
	egui_menu_item_set_radio_group(item, radio);

	egui_menu_add_separator(menu);

	for (i = 0; i < 30; i++) {
		echar buf[10];
		e_sprintf(buf, _("test%d"), i);
		item = egui_menu_item_new_with_label(buf);
		egui_add(menu, item);
	}

	return menu;
}
