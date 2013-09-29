#include <stdio.h>
#include <egui/egui.h>

static eint pos_v_clicked(eHandle hobj, ePointer data)
{
	egui_notebook_set_pos((eHandle)data, GUI_POS_TOP);
	return 0;
}

static eint pos_h_clicked(eHandle hobj, ePointer data)
{
	egui_notebook_set_pos((eHandle)data, GUI_POS_LEFT);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, hbox, notebook;
	eHandle button1, button2, button3, frame;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	egui_request_resize(win, 400, 400);
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);

	notebook = egui_notebook_new();

	button1 = egui_button_new(100, 100);
	frame   = egui_frame_new(_("frame1"));
	egui_set_expand(frame, true);
	egui_add(frame, button1);
	egui_notebook_append_page(notebook, frame, _("tab1"));
	button1 = egui_button_new(100, 100);
	frame   = egui_frame_new(_("frame2"));
	//egui_set_expand(frame, true);
	egui_add(frame, button1);
	egui_notebook_append_page(notebook, frame, _("tab2"));
	button1 = egui_button_new(100, 100);
	frame   = egui_frame_new(_("frame3"));
	//egui_set_expand(frame, true);
	egui_add(frame, button1);
	egui_notebook_append_page(notebook, frame, _("tab3"));
	button1 = egui_button_new(100, 100);
	frame   = egui_frame_new(_("frame4"));
	//egui_set_expand(frame, true);
	egui_add(frame, button1);
	egui_notebook_append_page(notebook, frame, _("tab4"));

	//egui_notebook_set_current_page(notebook, 1);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_SPREAD);
	egui_box_set_spacing(hbox, 10);
	egui_set_expand(hbox, true);
	button1 = egui_label_button_new(_("top"));
	button2 = egui_label_button_new(_("left"));
	button3 = egui_label_button_new(_("...."));
	egui_add(hbox, button1);
	egui_add(hbox, button2);
	egui_add(hbox, button3);
	e_signal_connect1(button1, SIG_CLICKED, pos_v_clicked, (ePointer)notebook);
	e_signal_connect1(button2, SIG_CLICKED, pos_h_clicked, (ePointer)notebook);

	egui_add(vbox, notebook);
	egui_add(vbox, hbox);
	egui_add(win,  vbox);

	egui_main();
	return 0;
}
