#include <stdio.h>

#include <egui/egui.h>

static GalImage *image = NULL;
static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_draw_image(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, 
			image, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
}
eHandle egui_image_new(const echar *filename);

int main(int argc, char *const argv[])
{
	eHandle win, vbox, bn;
	static GalPixbuf *pixbuf;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_CENTER);
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	bn = egui_label_button_new(_("hello"));
	egui_add(vbox, bn);

	pixbuf = egal_pixbuf_new_from_file(_("1.jpg"), 1.0, 1.0);
	image  = egal_image_new_from_pixbuf(pixbuf);
	egui_request_resize(win, image->w, image->h);

	e_signal_connect(win, SIG_EXPOSE_BG, win_expose_bg);

	egui_main();

	return 0;
}
