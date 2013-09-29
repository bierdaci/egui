#include <stdio.h>
#include <egui/egui.h>

static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_set_foreground(exp->pb, 0);
	egal_fill_rect(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
}

static void frame_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_set_foreground(exp->pb, 0xffffff);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + 5, wid->offset_y + 5,
			wid->offset_x + wid->rect.w - 5, wid->offset_y + 5);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + 5, wid->offset_y + 5, 
			wid->offset_x + 5, wid->offset_y + wid->rect.h - 5);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + 5, wid->offset_y + wid->rect.h - 5,
			wid->offset_x + wid->rect.w - 5, wid->offset_y + wid->rect.h - 5);
	egal_draw_line(wid->drawable, exp->pb,
			wid->offset_x + wid->rect.w - 5, wid->offset_y + 5,
			wid->offset_x + wid->rect.w - 5, wid->offset_y + wid->rect.h - 5);
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, hbox, frame;
	eHandle button1, button2, button3;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	frame = egui_frame_new(_("frame vbox"));
	egui_set_expand(frame, true);
	egui_add(win, frame);

	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_box_set_layout(vbox, BoxLayout_SPREAD);
	egui_box_set_spacing(vbox, 10);
	//egui_box_set_align(vbox, BoxAlignStart);
	egui_box_set_border_width(vbox, 10);
	egui_add(frame, vbox);

	frame = egui_frame_new(_("frame hbox4"));
	egui_set_expand_h(frame, true);
	egui_add(vbox, frame);

	hbox = egui_hbox_new();
	egui_box_set_spacing(hbox, 10);
	button1 = egui_button_new(80, 40);
	egui_add(hbox, button1);
	button2 = egui_button_new(80, 40);
	egui_add(hbox, button2);
	egui_add(frame, hbox);
	button3 = egui_button_new(80, 40);
	egui_add(hbox, button3);
	egui_add(frame, hbox);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_SPREAD);
	egui_box_set_spacing(hbox, 10);
	egui_set_expand(hbox, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(hbox, button1);
	egui_add(hbox, button2);
	egui_add(hbox, button3);

	frame = egui_frame_new(_("frame hbox1"));
	egui_set_expand_h(frame, true);
	egui_add(frame, hbox);
	egui_add(vbox, frame);

	hbox = egui_hbox_new();
	//egui_box_set_layout(hbox, BoxLayout_END);
	egui_box_set_spacing(hbox, 10);
	egui_set_expand(hbox, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(hbox, button1);
	egui_add(hbox, button2);
	egui_add(hbox, button3);

	frame = egui_frame_new(_("frame hbox2"));
	egui_set_expand(frame, true);
	egui_add(frame, hbox);
	egui_add(vbox, frame);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_SPREAD);
	egui_box_set_spacing(hbox, 10);
	egui_set_expand(hbox, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(hbox, button1);
	egui_add(hbox, button2);
	egui_add(hbox, button3);

	frame = egui_frame_new(_("frame hbox3"));
	//egui_set_expand(frame, true);
	egui_add(frame, hbox);
	egui_add(vbox, frame);

	e_signal_connect(win, SIG_EXPOSE_BG, win_expose_bg);

	egui_main();

	return 0;
}
