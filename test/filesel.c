#include <stdio.h>

#include <egui/egui.h>

static eint filesel_clicked(eHandle hobj, ePointer data)
{
	egui_show((eHandle)data, true);
	return 0;
}


int main(int argc, char *const argv[])
{
	eHandle win, vbox, button, filesel;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	egui_move(win, 200, 200);
	vbox = egui_vbox_new();
	egui_add(win, vbox);

	button = egui_button_new(80, 35);
	egui_add(vbox, button);

	filesel = egui_filesel_new();
	egui_request_resize(filesel, 400, 400);
	e_signal_connect1(button, SIG_CLICKED, filesel_clicked, (ePointer)filesel);

	egui_main();

	return 0;
}
