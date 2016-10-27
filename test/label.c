#include <egui/egui.h>
#include <egal/egal.h>

#include <egal/image.h>
#include <egal/pixbuf.h>

#include <math.h>

#include <X11/extensions/Xrender.h>

static void vbox_add_label(eHandle vbox, const echar *text)
{
	eHandle hbox, label;

	hbox = egui_hbox_new();
	egui_set_expand(hbox, etrue);
	egui_add_spacing(hbox, 20);

	label = egui_label_new(text);
	layout_set_flags(label, LF_VCenter);
	egui_set_transparent(label);

	egui_add(hbox, label);

	egui_add_spacing(hbox, 20);

	egui_add(vbox, hbox);
}

static eint draw_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	static GalImage *image = NULL;
	GalRect *prc = &exp->rect;

	if (!image)
		image = egal_image_new_from_file(_("1.png"));

	egal_draw_image(wid->drawable, exp->pb, 
			prc->x, prc->y,
			image,
			prc->x, prc->y, prc->w, prc->h);

	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox;

	egui_init(argc, argv);

	win = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_set_expand(vbox, etrue);

	vbox_add_label(vbox, _("hello"));
	vbox_add_label(vbox, _("world 123"));
	vbox_add_label(vbox, _("zhang"));
	vbox_add_label(vbox, _("xin"));
	vbox_add_label(vbox, _("hua"));

	egui_add(win, vbox);

	e_signal_connect(win, SIG_EXPOSE_BG, draw_expose_bg);

	egui_main();

	return 0;
}
