#include <egal/image.h>
#include <egui/egui.h>

static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_set_foreground(exp->pb, 0);
	egal_fill_rect(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
}

static ebool bn1_clicked(eHandle hobj, ePointer data)
{
	//egui_remove((eHandle)data);
	//e_object_unref((eHandle)data);
	if (GUI_STATUS_VISIBLE((eHandle)data))
		egui_hide((eHandle)data, etrue);
	else
		egui_show((eHandle)data, etrue);
	return etrue;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, anim, bn;
	GalPixbufAnim *gif;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	e_signal_connect(win, SIG_EXPOSE_BG, win_expose_bg);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_CENTER);
	egui_box_set_spacing(vbox, 10);
	egui_set_expand(vbox, etrue);
	egui_add(win, vbox);

	gif  = egal_pixbuf_anim_new_from_file(_("2.gif"), 1.0, 1.0);
	anim = egui_anim_new(gif);
	egui_add(vbox, anim);

	bn = egui_button_new(80, 40);
	e_signal_connect1(bn, SIG_CLICKED, bn1_clicked, (ePointer)anim);
	egui_add(vbox, bn);

	egui_main();

	return 0;
}
