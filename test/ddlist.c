#include <stdio.h>
#include <egui/egui.h>

static eint item_clicked(eHandle hobj, ePointer data)
{
	e_printf(_("%s\n"), data);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, ddlist, item;

	egui_init(argc, argv);

	win = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	ddlist = egui_ddlist_new();
	egui_request_resize(ddlist, 100, 0);
	egui_add(vbox, ddlist);

	item = egui_menu_item_new_with_label(_("aaa"));
	e_signal_connect1(item, SIG_CLICKED, item_clicked, "aaaaaaaa");
	egui_add(ddlist, item);
	item = egui_menu_item_new_with_label(_("bbb asd"));
	egui_add(ddlist, item);
	item = egui_menu_item_new_with_label(_("aasdfa"));
	egui_add(ddlist, item);

	egui_main();

	return 0;
}
