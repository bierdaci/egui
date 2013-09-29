#include <stdio.h>
#include <egui/egui.h>

static GalImage *image = NULL;
static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	//egal_set_foreground(exp->pb, 0);
	//egal_fill_rect(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
	egal_draw_image(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, 
			image, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
}

static eint bn_clicked(eHandle hobj, ePointer data)
{
	eHandle obj = (eHandle)data;
	if (GUI_STATUS_VISIBLE(obj))
		egui_hide(obj, true);
	else
		egui_show(obj, true);
	return 0;
}

static eint clicked_new_window(eHandle hobj, ePointer data)
{
	static eHandle subwin = 0;
	if (!subwin) {
		subwin = egui_window_new(GUI_WINDOW_POPUP);
		eHandle box = egui_vbox_new();
		egui_add(subwin, box);
		eHandle button = egui_button_new(80, 40);
		egui_add(box, button);
		button = egui_button_new(80, 40);
		egui_add(box, button);
	}

	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, box, vbox, vbox1, vbox2, vbox3, hbox, hbox1;
	eHandle button1, button2, button3, button4;
	eHandle bn1, bn2, bn3;
	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	box = egui_vbox_new();
	egui_set_expand(box, true);
	egui_box_set_layout(box, BoxLayout_SPREAD);
	egui_box_set_spacing(box, 10);
	egui_box_set_align(box, BoxAlignStart);
	egui_box_set_border_width(box, 10);
	egui_add(win, box);

	vbox = egui_hbox_new();
	egui_box_set_layout(vbox, BoxLayout_SPREAD);
	egui_box_set_spacing(vbox, 10);
	egui_set_expand(vbox, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(vbox, button1);
	egui_add(vbox, button2);
	egui_add(vbox, button3);
	bn3 = button1;

	hbox1 = egui_hbox_new();
	egui_box_set_layout(hbox1, BoxLayout_CENTER);
	egui_box_set_spacing(hbox1, 20);
	egui_box_set_align(hbox1, BoxAlignEnd);
	egui_set_expand_h(hbox1, true);

	vbox1 = egui_vbox_new();
	egui_box_set_layout(vbox1, BoxLayout_EDGE);
	egui_box_set_spacing(vbox1, 10);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(vbox1, button1);
	egui_add(vbox1, button2);
	egui_add(vbox1, button3);

	bn1 = button1;
	bn2 = button2;
	e_signal_connect(button3, SIG_CLICKED, clicked_new_window);

	vbox3 = egui_hbox_new();
	egui_box_set_layout(vbox3, BoxLayout_SPREAD);
	egui_box_set_spacing(vbox3, 10);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(vbox3, button1);
	egui_add(vbox3, button2);
	egui_add(vbox3, button3);

	egui_add(hbox1, vbox1);
	egui_add(hbox1, vbox3);

	hbox = egui_hbox_new();
	egui_set_expand(hbox, true);
	egui_box_set_layout(hbox, BoxLayout_SPREAD);
	egui_box_set_spacing(hbox, 10);

	vbox2 = egui_hbox_new();
	egui_box_set_layout(vbox2, BoxLayout_EDGE);
	egui_box_set_spacing(vbox2, 20);
	egui_set_expand(vbox2, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	button4 = egui_button_new(80, 40);
	egui_add(vbox2, button1);
	egui_add(vbox2, button2);
	egui_add(vbox2, button3);
	egui_add(vbox2, button4);

	vbox3 = egui_vbox_new();
	egui_box_set_align(vbox3, BoxAlignStart);
	egui_box_set_layout(vbox3, BoxLayout_SPREAD);
	egui_box_set_spacing(vbox3, 10);
	egui_set_expand(vbox3, true);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_add(vbox3, button1);
	egui_add(vbox3, button2);
	egui_add(vbox3, button3);

	egui_add(hbox, vbox3);
	egui_add(hbox, vbox2);

	egui_add(box, hbox1);
	egui_add(box, vbox);
	egui_add(box, hbox);

	e_signal_connect1(bn1, SIG_CLICKED, bn_clicked, (ePointer)vbox);
	e_signal_connect1(bn2, SIG_CLICKED, bn_clicked, (ePointer)hbox);
	e_signal_connect1(bn3, SIG_CLICKED, bn_clicked, (ePointer)hbox1);

	image = egal_image_new_from_pixbuf(egal_pixbuf_new_from_file(_("1.jpg"), 1.0, 1.0));

	//e_signal_connect(win, SIG_EXPOSE_BG, win_expose_bg);

	button1 = egui_radio_button_new(_("aaa"), 0);
	button2 = egui_radio_button_new(_("bbbaaaaa"), button1);
	button3 = egui_radio_button_new(_("ccc"), button1);
	egui_add(vbox3, button1);
	egui_add(vbox3, button2);
	egui_add(vbox3, button3);

	egui_main();

	return 0;
}

