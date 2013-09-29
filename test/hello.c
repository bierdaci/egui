#include <stdio.h>

#include <egui/egui.h>

int main(int argc, char *const argv[])
{
	eHandle win, vbox, label;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_CENTER);
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	label = egui_simple_label_new(_("hello world!"));
	egui_add(vbox, label);

	egui_main();

	return 0;
}
