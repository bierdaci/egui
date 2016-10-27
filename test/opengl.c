#include <stdio.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <egui/egui.h>

#include "image.h"

static eint image_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1., 1., -1., 1., 1., 20.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

	glBegin(GL_QUADS);
	glColor3f(1., 0., 0.); glVertex3f(-.75, -.75, 0.);
	glColor3f(0., 1., 0.); glVertex3f( .75, -.75, 0.);
	glColor3f(0., 0., 1.); glVertex3f( .75,  .75, 0.);
	glColor3f(1., 1., 0.); glVertex3f(-.75,  .75, 0.);
	glEnd();
	glFlush();

	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, image;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_START);
	egui_set_expand(vbox, etrue);
	egui_box_set_spacing(vbox, 20);
	egui_add(win, vbox);

    image  = egui_image_new(500, 400);
	egui_make_GL(image);
	egui_add(vbox, image);
	egui_set_focus(image);
    e_signal_connect(image, SIG_EXPOSE, image_expose);
	
	egui_main();

	return 0;
}
