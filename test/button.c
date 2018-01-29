#include <stdio.h>
#include <egui/egui.h>
#include <egal/image.h>
#include <egal/pixbuf.h>

static eHandle win;
static eTimer timer;
static eint timer_test_cb(eTimer timer, euint n, ePointer data)
{
	eHandle hobj = (eHandle)data;
	if (GUI_STATUS_VISIBLE(hobj))
		egui_hide(hobj, etrue);
	else
		egui_show(hobj, etrue);
	return -1;
}

static eint show_clicked(eHandle hobj, ePointer data)
{
	GalRect rc = {100, 0, 100, 30};
	GuiWidget *wid = GUI_WIDGET_DATA(win);
	timer = e_timer_add(300, timer_test_cb, data);
	egal_set_foreground(wid->pb, 0);
	egui_draw_strings(wid->drawable, wid->pb, 0, _("hello world"), &rc, LF_VCenter);
	rc.x = 350;
	egui_draw_strings(wid->drawable, wid->pb, 0, _("drawable center"), &rc, LF_VCenter);
	return 0;
}

static eint test_printf(eHandle hobj, ePointer data)
{
	printf("..............%s\n", (char *)data);
	return 0;
}

static ebool test_clicked(eHandle hobj, ePointer data)
{
	GuiWidget *wid = GUI_WIDGET_DATA(data);
	//egui_update(hobj);
	int x = wid->rect.x;
	int y = wid->rect.y;
	if (y > 760)
		y = 0;
	else
		y += 10;
	if (x >= 710)
		x = 400;
	else
		x += 10;
	egui_move((eHandle)data, x, y);
	//printf("%d\n", y);
	//if (y == 25)
	egui_raise((eHandle)data);
/*
	if (egui_is_show(data))
		egui_hide(data);
	else
		egui_show(data);
*/
	return etrue;
}
void egui_desk_set_expose(GuiWidget *wid, void (*expose)(GuiWidget *, GalEventExpose *));
static GalImage *image = NULL;
static GalPixbuf *pixbuf1, *pixbuf2 = NULL;
static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	egal_draw_image(wid->drawable, exp->pb, exp->rect.x, exp->rect.y, 
			image, exp->rect.x, exp->rect.y, exp->rect.w, exp->rect.h);
}

int main(int argc, char *const argv[])
{
	eHandle desk, cdesk, desk1, desk2, desk3, desk4; 
	eHandle button1, button2, button3, fixed;
	egui_init(argc, argv);

	win = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	fixed = egui_fixed_new(800, 800);
	egui_add(win, fixed);

	desk = egui_fixed_new(400, 400);
	//egui_set_transparent(desk);
	egui_put(fixed, desk, 0, 0);

	desk1 = egui_fixed_new(100, 200);
	//egui_set_transparent(desk1);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_put(desk1, button1, 10, 0);
	egui_put(desk1, button2, 10, 50);
	egui_put(desk1, button3, 10, 100);
	e_signal_connect1(button1, SIG_CLICKED, test_printf, "desk1 button1");
	e_signal_connect1(button2, SIG_CLICKED, test_printf, "desk1 button2");
	e_signal_connect1(button3, SIG_CLICKED, test_printf, "desk1 button3");
	egui_put(desk, desk1, 0, 0);

	desk2 = egui_fixed_new(100, 200);
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_put(desk2, button1, 10, 0);
	egui_put(desk2, button2, 10, 50);
	egui_put(desk2, button3, 10, 100);
	e_signal_connect1(button1, SIG_CLICKED, test_printf, "desk2 button1");
	e_signal_connect1(button2, SIG_CLICKED, test_printf, "desk2 button2");
	e_signal_connect1(button3, SIG_CLICKED, test_printf, "desk2 button3");
	egui_put(desk, desk2, 0, 200);
	//egui_hide(button2);
	//egui_set_transparent(button3);

	cdesk = egui_fixed_new(200, 200);
	desk3 = egui_fixed_new(200, 400);
	desk4 = egui_fixed_new(100, 200);
	egui_put(desk3, desk4, 100, 0);
	button1 = egui_button_new(70, 40);
	button2 = egui_button_new(70, 40);
	button3 = egui_button_new(70, 40);
	egui_put(desk4, button1, 0, 0);
	egui_put(desk4, button2, 0, 50);
	egui_put(desk4, button3, 0, 100);
	e_signal_connect1(button1, SIG_CLICKED, test_printf, "desk4 button1");
	e_signal_connect1(button2, SIG_CLICKED, test_printf, "desk4 button2");
	e_signal_connect1(button3, SIG_CLICKED, test_printf, "desk4 button3");
	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_put(desk3, button1, 0, 0);
	egui_put(desk3, button2, 0, 50);
	egui_put(desk3, button3, 0, 100);
	e_signal_connect1(button1, SIG_CLICKED, test_printf, "desk3 button1");
	e_signal_connect1(button2, SIG_CLICKED, test_printf, "desk3 button2");
	e_signal_connect1(button3, SIG_CLICKED, test_printf, "desk3 button3");

	egui_put(cdesk, desk3, 0, 0);
	egui_put(fixed, cdesk, 200, 500);

	button1 = egui_button_new(80, 40);
	button2 = egui_button_new(80, 40);
	button3 = egui_button_new(80, 40);
	egui_put(desk, button1, 300, 100);
	egui_put(desk, button2, 300, 150);
	egui_put(desk, button3, 300, 200);
	e_signal_connect1(button1, SIG_CLICKED, test_printf, "desk button1");
	e_signal_connect1(button2, SIG_CLICKED, test_printf, "desk button2");
	e_signal_connect1(button3, SIG_CLICKED, test_printf, "desk button3");

	button1 = egui_button_new(90, 40);
	button2 = egui_button_new(90, 40);
	button3 = egui_button_new(90, 40);
	egui_put(fixed, button1, 600, 200);
	egui_put(fixed, button2, 600, 250);
	egui_put(fixed, button3, 600, 300);

	e_signal_connect1(button1, SIG_CLICKED, show_clicked, (ePointer)desk1);
	e_signal_connect1(button2, SIG_CLICKED, show_clicked, (ePointer)desk2);
	e_signal_connect1(button3, SIG_CLICKED, show_clicked, (ePointer)desk3);
	button3 = egui_button_new(90, 40);
	egui_put(fixed, button3, 600, 350);
	e_signal_connect1(button3, SIG_CLICKED, show_clicked, (ePointer)desk4);


	pixbuf1 = egal_pixbuf_new_from_file(_("1.jpg"), 1.0, 1.0);
	//pixbuf2 = egal_pixbuf_new_from_file(_("mush.png"), 1.0, 1.0);

	//egal_pixbuf_composite(pixbuf1, 450, 450, pixbuf2->w * 2, pixbuf2->h* 2, pixbuf2, 0, 0, 2.0, 2.0, PIXOPS_INTERP_HYPER);
	image = egal_image_new_from_pixbuf(pixbuf1);

	e_signal_connect(fixed, SIG_EXPOSE_BG, win_expose_bg);

	egui_main();

	return 0;
}
