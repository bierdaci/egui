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
		egui_hide(obj, etrue);
	else
		egui_show(obj, etrue);
	return 0;
}

static eint clicked_new_window(eHandle hobj, ePointer data)
{
	static eHandle subwin = 0;
	if (!subwin) {
		eHandle box = egui_vbox_new();
		eHandle button = egui_button_new(80, 40);
		subwin = egui_window_new(GUI_WINDOW_POPUP);
		egui_add(subwin, box);
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
	/*
	box = egui_vbox_new();
	egui_set_expand(box, etrue);
	egui_box_set_layout(box, BoxLayout_SPREAD);
	egui_box_set_spacing(box, 10);
	egui_box_set_align(box, BoxAlignStart);
	egui_box_set_border_width(box, 10);
	egui_add(win, box);
	*/

	button3 = egui_button_new(80, 40);
	printf("..........%p  %p\n", win, button3);
	egui_add(win, button3);

	egui_main();

	return 0;
}

