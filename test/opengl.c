#include <stdio.h>

#include <egui/egui.h>

#include "image.h"

#include <GL/glew.h>

typedef float vec2_t[2];
typedef float vec3_t[3];

static float win_w, win_h;

static eint win_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	vec2_t p1 = {10, 10};
	vec2_t p2 = {100, 30};
	vec2_t p3 = {50, 100};

	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, win_w, win_h, 0, 0, 1);

	glColor3f(1, 1, 1);

	glBegin(GL_LINE_LOOP);
	glVertex2fv(p1);
	glVertex2fv(p2);
	glVertex2fv(p3);
	glEnd();

	GalSwapBuffers();

	return 0;
}

static eint win_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	win_w = resize->w;
	win_h = resize->h;
	egui_make_GL(hobj);
	glViewport(0, 0, win_w, win_h);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, drawable;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_START);
	egui_set_expand(vbox, etrue);
	egui_box_set_spacing(vbox, 20);
	egui_add(win, vbox);

    drawable  = egui_drawable_new(500, 400);
	egui_make_GL(drawable);
	egui_add(vbox, drawable);
	egui_set_focus(drawable);
    e_signal_connect(drawable, SIG_EXPOSE, win_expose);
	e_signal_connect(drawable, SIG_RESIZE, win_resize);
	
	egui_main();

	return 0;
}
