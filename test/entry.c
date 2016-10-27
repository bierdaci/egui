#include <stdio.h>

#include <egui/egui.h>


static eint entry_clicked(eHandle hobj, ePointer data)
{
	echar buf[20];
	eint len = egui_get_text((eHandle)data, buf, 20);

	buf[len] = 0;
	printf("%d  %s\n", len, buf);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, entry, bn, spin;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	vbox = egui_vbox_new();
	egui_set_expand(vbox, etrue);
	egui_add(win, vbox);

	entry = egui_entry_new(100);
	egui_entry_set_visibility(entry, efalse);
	egui_add(vbox, entry);
	egui_set_max_text(entry, 20);
	egui_set_strings(entry, _("我温泉你"));

	egui_box_set_spacing(vbox, 10);
	//egui_add_spacing(vbox, 10);

	bn = egui_label_button_new(_("gettext"));
	egui_add(vbox, bn);
	e_signal_connect1(bn, SIG_CLICKED, entry_clicked, (ePointer)entry);

	spin = egui_spinbtn_new(10., -10, 1000., 1.05, 0, 2);
	egui_add(vbox, spin);

	egui_main();

	return 0;
}
