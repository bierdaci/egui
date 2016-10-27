#include <egui/egui.h>
#include <egal/egal.h>
#include <egal/pixbuf.h>

#include <math.h>

#include <X11/extensions/Xrender.h>

static GalImage *image = NULL;
static GalPixbuf *pixbuf1, *pixbuf2 = NULL;
eint pic1, mask;
static void test_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_draw_image(wid->drawable, exp->pb,
			exp->rect.x, exp->rect.y,
			image,
			exp->rect.x, exp->rect.y,
			exp->rect.w, exp->rect.h);
}

static void btn1_set_strings(eHandle hobj, ePointer data)
{
	layout_set_table_size((eHandle)data, 4);
	layout_set_spacing((eHandle)data, 0);
	//layout_set_extent((eHandle)data, 100, 100);
	layout_set_wrap((eHandle)data);
	layout_set_align((eHandle)data, LF_VCenter | LF_HLeft);
	egui_update((eHandle)data);
}

static void btn2_set_strings(eHandle hobj, ePointer data)
{
	static ebool set = efalse;
	if (!set) {
		set = etrue;
		layout_set_strings((eHandle)data, _("12345\nasdfklasdjf;a你功阿辣四嘉阿斯顿立刻就ie\n你委屈大1324123412341234\n"
					"\n\nlksdfj;aslkdfjaskldfjaslkdfjas\ndkfasd124jlkjadsfkj低回是阿斯看风景阿斯可叫阿斯蒂芬........"));
	}
	//layout_set_table_size((eHandle)data, 8);
	//layout_set_spacing((eHandle)data, 4);
	//layout_set_extent((eHandle)data, 80, 100);
	layout_unset_wrap((eHandle)data);
	egui_update((eHandle)data);
}

static void btn3_set_transparent(eHandle hobj, ePointer data)
{
	if (GUI_STATUS_TRANSPARENT((eHandle)data))
		egui_unset_transparent((eHandle)data);
	else
		egui_set_transparent((eHandle)data);
}

int main(int argc, char *const argv[])
{
	eHandle win, label, btn1, btn2, btn3, vscrollbar, hscrollbar;
	eHandle scrwin, fixed;
	GalPixbuf pixbuf;

	egui_init(argc, argv);

	win   = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	fixed = egui_fixed_new(800, 800);
	egui_add(win, fixed);

	label = egui_label_new(_("asdfkla\n\nsjdfklajsd\n\n;fajksdf\n\n你好吗啊啊a？\n"));
	egui_set_transparent(label);
	egui_request_resize(label, 400, 100);

	egui_put(fixed, label, 50, 100);

	btn1   = egui_button_new(80, 40);
	egui_put(fixed, btn1, 600, 400);
	btn2   = egui_button_new(80, 40);
	egui_put(fixed, btn2, 600, 500);
	btn3   = egui_button_new(80, 40);
	egui_put(fixed, btn3, 600, 600);

	vscrollbar = egui_vscrollbar_new(efalse);
	hscrollbar = egui_hscrollbar_new(efalse);

	egui_request_resize(vscrollbar, 0, 500);
	egui_request_resize(hscrollbar, 400, 0);

	egui_hook_v(label, vscrollbar);
	egui_hook_h(label, hscrollbar);

	egui_put(fixed, vscrollbar, 750, 10);
	egui_put(fixed, hscrollbar, 50, 71);

	e_signal_connect1(btn1, SIG_CLICKED, btn1_set_strings, (ePointer)label);
	e_signal_connect1(btn2, SIG_CLICKED, btn2_set_strings, (ePointer)label);
	e_signal_connect1(btn3, SIG_CLICKED, btn3_set_transparent, (ePointer)label);

	//egal_pixbuf_load_header(_("mush.png"), &pixbuf);

	//pixbuf1 = egal_pixbuf_new_from_file(_("1.jpg"), 0, 0);
	//pixbuf2 = egal_pixbuf_new_from_file(_("mush.png"), pixbuf.w, pixbuf.h);

	//egal_pixbuf_composite(pixbuf1, 450, 450, pixbuf.w * 2, pixbuf.h * 2, pixbuf2, 0, 0, 1.0, 1.0, PIXOPS_INTERP_HYPER);

	float f = 1.5;
	pixbuf1 = egal_pixbuf_new_from_file(_("1.jpg"), f, f);
	image = egal_image_new_from_pixbuf(pixbuf1);
	GalPB pb = egal_pb_new(GUI_WIDGET_DATA(fixed)->drawable, NULL);
	//egal_draw_image(GUI_WIDGET_DATA(fixed)->window, pb, 0, 0, image, 0, 0, image->w, image->h);
	e_signal_connect(fixed, SIG_EXPOSE_BG, test_expose_bg);

	egui_put(fixed, hscrollbar, 50, 71);

	egui_main();

	return 0;
}
