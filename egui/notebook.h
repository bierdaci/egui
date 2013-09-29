#ifndef __GUI_NOTEBOOK_H
#define __GUI_NOTEBOOK_H

#include <egui/gui.h>

#define GTYPE_NOTEBOOK				(egui_genetype_notebook())

#define GUI_NOTEBOOK_DATA(hobj)		((GuiNotebook *)e_object_type_data((hobj), GTYPE_NOTEBOOK))

typedef struct _GuiNotebook 		GuiNotebook;

struct _GuiNotebook {
	eHandle tabbar;
	eHandle pagebox;
	eHandle curpage;
	eint direct;
};

eGeneType egui_genetype_notebook(void);
eHandle egui_notebook_new(void);
eint egui_notebook_append_page(eHandle, eHandle, const echar *);
void egui_notebook_set_current_page(eHandle, eint);
void egui_notebook_set_pos(eHandle, GuiPositionType);

#endif
