#ifndef __GUI_PROGRESS_BAR_H
#define __GUI_PROGRESS_BAR_H

#include <elib/object.h>

#define GTYPE_PROGRESS_BAR						(egui_genetype_progress_bar())

#define GUI_PROGRESS_BAR_DATA(hobj)				((GuiProgressbar *)e_object_type_data(hobj, GTYPE_PROGRESS_BAR))

typedef struct _GuiProgressbar  GuiProgressbar;

struct _GuiProgressbar {
	efloat fraction;
	efloat old_fraction;
};

eGeneType egui_genetype_progress_bar(void);

eHandle   egui_progress_bar_new(void);

#endif
